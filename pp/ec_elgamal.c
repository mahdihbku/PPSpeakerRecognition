#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <openssl/obj_mac.h>
#include <openssl/ec.h>
#include <openssl/rand.h>
#include <openssl/bn.h>
#include <ctype.h>


#define dim 200

BN_CTX *ctx;
EC_GROUP *curve;
const size_t priv_len = 32;
const size_t pub_len = 65;
const size_t POINT_UNCOMPRESSED_len = 65; // z||x||y    z is the octet 0x04
const size_t POINT_COMPRESSED_len = 33;   // z||x     z specifies which solution of the quadratic equation y is (0x02 or 0x03)
EC_POINT *h, *g;  //h: pub_key
BIGNUM *x, *q;    //x: priv_key
unsigned char* decryption_buf;

// PLDA scoring buffers;
int N = 0;
unsigned int dby_portion = 1;
unsigned int first_index = 0, last_index = 256;
unsigned char* enc_db;
unsigned char* enc_sum_x2;
unsigned char* enc_db_y;
// unsigned char* enc_y2_var;
// unsigned char* enc_y2;
unsigned char* enc_r3;
unsigned char* rand_numbers;
unsigned char** r1_list;
unsigned char** r2_list;
unsigned int* n_list;
double* psi;
double teta[dim];
// size_t max_y2_var_value = 6750;
double y[dim];
double yprime[dim];       // y_i * alpha + beta
int normalized_y[dim];    // round(yprime)
// int y2_var_int = 0;
const unsigned int max_r3_value = 10500000;
const int starting_r3 = -8005000;
const double alpha = 13.5, beta = 135.0;
double tau = -40.44;
// char similarity_threshold[130];

static int bn2binpad(const BIGNUM *a, unsigned char *to, int tolen){
    int n;
    size_t i, lasti, j, atop, mask;
    BN_ULONG l;

    /*
     * In case |a| is fixed-top, BN_num_bytes can return bogus length,
     * but it's assumed that fixed-top inputs ought to be "nominated"
     * even for padded output, so it works out...
     */
    n = BN_num_bytes(a);
    if (tolen == -1) {
        tolen = n;
    } else if (tolen < n) {     /* uncommon/unlike case */
        BIGNUM temp = *a;

        bn_correct_top(&temp);
        n = BN_num_bytes(&temp);
        if (tolen < n)
            return -1;
    }

    /* Swipe through whole available data and don't give away padded zero. */
    atop = a->dmax * BN_BYTES;
    if (atop == 0) {
        OPENSSL_cleanse(to, tolen);
        return tolen;
    }

    lasti = atop - 1;
    atop = a->top * BN_BYTES;
    for (i = 0, j = 0, to += tolen; j < (size_t)tolen; j++) {
        l = a->d[i / BN_BYTES];
        mask = 0 - ((j - atop) >> (8 * sizeof(i) - 1));
        *--to = (unsigned char)(l >> (8 * (i % BN_BYTES)) & mask);
        i += (i - lasti) >> (8 * sizeof(i) - 1); /* stay on last limb */
    }

    return tolen;
}

void file_to_buffer(char *file_name, unsigned char **buffer) {
	FILE *file = fopen(file_name, "rb");
	fseek(file, 0L, SEEK_END);
	size_t file_size = ftell(file);
	fseek(file, 0L, SEEK_SET);
	*buffer = (unsigned char*) malloc(file_size * sizeof(unsigned char));
	fread(*buffer, file_size, 1, file);
	fclose(file);
}

int prepare(char *pub_filename, char *priv_filename) {
  // seed PRNG
  if (RAND_load_file("/dev/urandom", 256) < 64) {
    printf("Can't seed PRNG!\n");
    abort();
  }
  ctx = BN_CTX_new();
  curve = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
  g = EC_POINT_new(curve);
  q = BN_new();
  g = (EC_POINT *) EC_GROUP_get0_generator(curve);
  EC_GROUP_get_order(curve, q, ctx);
  x = BN_new();
  h = EC_POINT_new(curve);

  FILE *pub_file_p = fopen(pub_filename, "rb");
  unsigned char *pub_buf = malloc(sizeof(char)*POINT_UNCOMPRESSED_len);
  fread(pub_buf, POINT_UNCOMPRESSED_len, 1, pub_file_p);
  fclose(pub_file_p);
  FILE *priv_file_p = fopen(priv_filename, "rb");
  unsigned char *priv_buf = malloc(sizeof(char)*priv_len);
  fread(priv_buf, priv_len, 1, priv_file_p);
  fclose(priv_file_p);

  BN_bin2bn(priv_buf, priv_len, x);
  EC_POINT_oct2point(curve, h, pub_buf, POINT_UNCOMPRESSED_len, ctx);

  return 0;
}

int prepare_for_enc(char *pub_filename) {
  // seed PRNG
  if (RAND_load_file("/dev/urandom", 256) < 64) {
    printf("Can't seed PRNG!\n");
    abort();
  }
  ctx = BN_CTX_new();
  curve = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
  g = EC_POINT_new(curve);
  q = BN_new();
  g = (EC_POINT *) EC_GROUP_get0_generator(curve);
  EC_GROUP_get_order(curve, q, ctx);
  h = EC_POINT_new(curve);

  // char *pub_filename = "ec_pub.txt";
  FILE *pub_file_p = fopen(pub_filename, "rb");
  unsigned char *pub_buf = malloc(sizeof(char)*pub_len);
  fread(pub_buf, pub_len, 1, pub_file_p);
  fclose(pub_file_p);

  EC_POINT_oct2point(curve, h, pub_buf, pub_len, ctx);

  return 0;
}

void encrypt_ec(char *cipherText, char *mess) {
  EC_POINT *c1, *c2;
  BIGNUM *m, *r;
  c2 = EC_POINT_new(curve);
  c1 = EC_POINT_new(curve);
  m = BN_new();
  r = BN_new();
  BN_dec2bn(&m, mess);
  BN_rand_range(r, q);
  EC_POINT_mul(curve, c1, NULL, g, r, ctx);
  BN_add(r, r, m);
  EC_POINT_mul(curve, c2, NULL, h, r, ctx);
    if (!EC_POINT_point2oct(curve, c1, POINT_CONVERSION_UNCOMPRESSED, cipherText, POINT_UNCOMPRESSED_len, ctx))
    printf("encrypt:error!\n");
  if (!EC_POINT_point2oct(curve, c2, POINT_CONVERSION_UNCOMPRESSED, cipherText+POINT_UNCOMPRESSED_len, POINT_UNCOMPRESSED_len, ctx))
    printf("encrypt:error!\n");
}

void load_encryption_file() {
  FILE *dec_file_p = fopen("decrypt_file.bin", "rb");
  size_t file_size = (size_t)1073741824 * 17 * 2;
  decryption_buf = malloc(file_size * sizeof(unsigned char));
  fread(decryption_buf, file_size, 1, dec_file_p);
  fclose(dec_file_p);
}

int score_is_positive(char *ciphert) {
  EC_POINT *c1, *c2, *h1;
  BIGNUM *m;
  c1 = EC_POINT_new(curve);
  c2 = EC_POINT_new(curve);
  h1 = EC_POINT_new(curve);
  m = BN_new();
  EC_POINT_oct2point(curve, c1, ciphert, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_oct2point(curve, c2, ciphert+POINT_UNCOMPRESSED_len, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_mul(curve, c1, NULL, c1, x, ctx);
  EC_POINT_invert(curve, c1, ctx);
  EC_POINT_add(curve, c1, c1, c2, ctx);

  unsigned char* buf = malloc(POINT_COMPRESSED_len * sizeof(unsigned char));
  size_t c1_size = EC_POINT_point2oct(curve, c1, POINT_CONVERSION_COMPRESSED, buf, POINT_COMPRESSED_len, ctx);
  unsigned char x_buf[4];
  x_buf[0] = buf[1];  // these values gave the least collisions
  x_buf[1] = buf[3];
  x_buf[2] = buf[11];
  x_buf[3] = buf[14];
  size_t index = (unsigned long long)*((unsigned int*) x_buf);  // 4 bytes to be used as address in the hashing table
  index = index >> 2; // index is 30 bits not 32
  index = index * 2 * 17;
  while (decryption_buf[index] != 0x0 && (decryption_buf[index] != buf[0] || memcmp(&decryption_buf[index+1], &buf[5], 16)))
    index+=17;
  if (decryption_buf[index] == 0x0) // not found in table => negative
    return 0;
  else  // exists in table => positive
    return 1;
}

void decrypt_ec(char *message, char *ciphert) {
  EC_POINT *c1, *c2, *h1;
  BIGNUM *m;
  c1 = EC_POINT_new(curve);
  c2 = EC_POINT_new(curve);
  h1 = EC_POINT_new(curve);
  m = BN_new();
  EC_POINT_oct2point(curve, c1, ciphert, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_oct2point(curve, c2, ciphert+POINT_UNCOMPRESSED_len, POINT_UNCOMPRESSED_len, ctx);

  EC_POINT_mul(curve, c1, NULL, c1, x, ctx);
  EC_POINT_invert(curve, c1, ctx);
  EC_POINT_add(curve, c1, c1, c2, ctx);
  EC_POINT_set_to_infinity(curve, h1);
  int i = 0;
  for (; i < 5000000; i++) {  // can decrypt only values that are < 1000000
    if (!EC_POINT_cmp(curve, h1, c1, ctx))
      break;
    EC_POINT_add(curve, h1, h1, h, ctx);
  }
  snprintf(message, 30, "%d", i);
}

void generate_decrypt_file() {
  unsigned char* buf = malloc(POINT_COMPRESSED_len * sizeof(unsigned char));
  unsigned char x_buf[4];
  size_t index, h1_size, collisions = 0, collision_moves, max_collision_moves=0, file_size = (size_t)1073741824 * 17 * 2; //hash[0] in buf[0] for the sign, and hash[5..20] in buf[1..16] for the hash
  EC_POINT *h1 = EC_POINT_new(curve);
  EC_POINT_set_to_infinity(curve, h1);
  EC_POINT_add(curve, h1, h1, h, ctx);

  decryption_buf = (unsigned char*) calloc (file_size, sizeof(unsigned char));
  // Each ct is of size 32 bytes. 4 bytes will be used as @ => and 16+1 bytes as data. Bucket size = 2
  if (decryption_buf == NULL) {
    printf("generate_decrypt_file: ERROR: malloc failed\n");
    exit(0);
  }
  for (unsigned int i=0; i<1073741824; i++) {
    h1_size = EC_POINT_point2oct(curve, h1, POINT_CONVERSION_COMPRESSED, buf, POINT_COMPRESSED_len, ctx);
    x_buf[0] = buf[1];  // these values gave the least collisions
    x_buf[1] = buf[3];
    x_buf[2] = buf[11];
    x_buf[3] = buf[14];
    index = (unsigned long long)*((unsigned int*) x_buf);
    index = index >> 2; // index is 30 bits not 32
    index = index * 2 * 17; // 2 entries per bucket
    collision_moves = 0;
    while (decryption_buf[index] != 0x0) {  // not empty entries, if not empty it must be 0x2 or 0x3
      index+=17;
      collisions++;
      collision_moves++;
    }
    if (max_collision_moves < collision_moves) {
      max_collision_moves = collision_moves;
      printf("generate_decrypt_file: at index=%lu, collisions=%lu, max_collision_moves=%lu, i=%u/%u, table occupation=%lu%%\n", index, collisions, collision_moves, i/1024, 1073741824/1024, (size_t)i*100/1073741824);
    }
    decryption_buf[index] = buf[0];
    memcpy(&decryption_buf[index+1], &buf[5], 16);
    EC_POINT_add(curve, h1, h1, h, ctx);
  }
  printf("generate_decrypt_file: writing to file...\n");
  FILE *pfile;  // writing decryption_buf to file
  pfile = fopen("decrypt_file.bin", "wb");
  fwrite(decryption_buf, sizeof(unsigned char), file_size, pfile);
  fclose(pfile);
  printf("generate_decrypt_file: file generated successfully!\n");
}

void add2(char *result, char *ct1, char *ct2) {
  EC_POINT *c1, *c2, *c3, *c4;
  c1 = EC_POINT_new(curve);
  c2 = EC_POINT_new(curve);
  c3 = EC_POINT_new(curve);
  c4 = EC_POINT_new(curve);
  EC_POINT_oct2point(curve, c1, ct1, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_oct2point(curve, c2, ct1+POINT_UNCOMPRESSED_len, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_oct2point(curve, c3, ct2, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_oct2point(curve, c4, ct2+POINT_UNCOMPRESSED_len, POINT_UNCOMPRESSED_len, ctx);

  EC_POINT_add(curve, c1, c1, c3, ctx);
  EC_POINT_add(curve, c2, c2, c4, ctx);

    if (!EC_POINT_point2oct(curve, c1, POINT_CONVERSION_UNCOMPRESSED, result, POINT_UNCOMPRESSED_len, ctx))
    printf("add2:error!\n");
  if (!EC_POINT_point2oct(curve, c2, POINT_CONVERSION_UNCOMPRESSED, result+POINT_UNCOMPRESSED_len, POINT_UNCOMPRESSED_len, ctx))
    printf("add2:error!\n");
}

void add3(char *result, char *ct1, char *ct2, char *ct3) {
  EC_POINT *c11, *c12, *c21, *c22, *c31, *c32;
  c11 = EC_POINT_new(curve);
  c12 = EC_POINT_new(curve);
  c21 = EC_POINT_new(curve);
  c22 = EC_POINT_new(curve);
  c31 = EC_POINT_new(curve);
  c32 = EC_POINT_new(curve);
  EC_POINT_oct2point(curve, c11, ct1, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_oct2point(curve, c12, ct1+POINT_UNCOMPRESSED_len, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_oct2point(curve, c21, ct2, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_oct2point(curve, c22, ct2+POINT_UNCOMPRESSED_len, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_oct2point(curve, c31, ct3, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_oct2point(curve, c32, ct3+POINT_UNCOMPRESSED_len, POINT_UNCOMPRESSED_len, ctx);

  EC_POINT_add(curve, c11, c11, c21, ctx);
  EC_POINT_add(curve, c11, c11, c31, ctx);
  EC_POINT_add(curve, c12, c12, c22, ctx);
  EC_POINT_add(curve, c12, c12, c32, ctx);

  if (!EC_POINT_point2oct(curve, c11, POINT_CONVERSION_UNCOMPRESSED, result, POINT_UNCOMPRESSED_len, ctx))
    printf("add3:error!\n");
  if (!EC_POINT_point2oct(curve, c12, POINT_CONVERSION_UNCOMPRESSED, result+POINT_UNCOMPRESSED_len, POINT_UNCOMPRESSED_len, ctx))
    printf("add3:error!\n");
}

void add4(char *result, char *ct1, char *ct2, char *ct3, char *ct4) {
  EC_POINT *c11, *c12, *c21, *c22, *c31, *c32, *c41, *c42;
  c11 = EC_POINT_new(curve);
  c12 = EC_POINT_new(curve);
  c21 = EC_POINT_new(curve);
  c22 = EC_POINT_new(curve);
  c31 = EC_POINT_new(curve);
  c32 = EC_POINT_new(curve);
  c41 = EC_POINT_new(curve);
  c42 = EC_POINT_new(curve);
  EC_POINT_oct2point(curve, c11, ct1, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_oct2point(curve, c12, ct1+POINT_UNCOMPRESSED_len, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_oct2point(curve, c21, ct2, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_oct2point(curve, c22, ct2+POINT_UNCOMPRESSED_len, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_oct2point(curve, c31, ct3, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_oct2point(curve, c32, ct3+POINT_UNCOMPRESSED_len, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_oct2point(curve, c41, ct4, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_oct2point(curve, c42, ct4+POINT_UNCOMPRESSED_len, POINT_UNCOMPRESSED_len, ctx);

  EC_POINT_add(curve, c11, c11, c21, ctx);
  EC_POINT_add(curve, c11, c11, c31, ctx);
  EC_POINT_add(curve, c11, c11, c41, ctx);
  EC_POINT_add(curve, c12, c12, c22, ctx);
  EC_POINT_add(curve, c12, c12, c32, ctx);
  EC_POINT_add(curve, c12, c12, c42, ctx);

  if (!EC_POINT_point2oct(curve, c11, POINT_CONVERSION_UNCOMPRESSED, result, POINT_UNCOMPRESSED_len, ctx))
    printf("add4:error!\n");
  if (!EC_POINT_point2oct(curve, c12, POINT_CONVERSION_UNCOMPRESSED, result+POINT_UNCOMPRESSED_len, POINT_UNCOMPRESSED_len, ctx))
    printf("add4:error!\n");
}

void mult(char *result, char *scalar, char *ct1) {
  EC_POINT *c1, *c2;
  BIGNUM *m;
  c1 = EC_POINT_new(curve);
  c2 = EC_POINT_new(curve);
  m = BN_new();
  EC_POINT_oct2point(curve, c1, ct1, POINT_UNCOMPRESSED_len, ctx);
  EC_POINT_oct2point(curve, c2, ct1+POINT_UNCOMPRESSED_len, POINT_UNCOMPRESSED_len, ctx);
  BN_dec2bn(&m, scalar);

  EC_POINT_mul(curve, c1, NULL, c1, m, ctx);
  EC_POINT_mul(curve, c2, NULL, c2, m, ctx);

  if (!EC_POINT_point2oct(curve, c1, POINT_CONVERSION_UNCOMPRESSED, result, POINT_UNCOMPRESSED_len, ctx))
    printf("add2:error!\n");
  if (!EC_POINT_point2oct(curve, c2, POINT_CONVERSION_UNCOMPRESSED, result+POINT_UNCOMPRESSED_len, POINT_UNCOMPRESSED_len, ctx))
    printf("add2:error!\n");
}

void generate_keys(char *pub_filename, char *priv_filename) {
  BN_CTX *ctx;
  EC_GROUP *curve;
  BIGNUM *x;
  EC_POINT *g, *h;
  EC_KEY *key;
  if (RAND_load_file("/dev/urandom", 256) < 64) {
    printf("Can't seed PRNG!\n");
    abort();
  }
  ctx = BN_CTX_new();
  x = BN_new();
  // create a standard 256-bit prime curve
  curve = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
  g = EC_POINT_new(curve);
  h = EC_POINT_new(curve);
  g = (EC_POINT *) EC_GROUP_get0_generator(curve);
  key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
  EC_KEY_generate_key(key);
  x = (BIGNUM *) EC_KEY_get0_private_key(key);
  h = (EC_POINT *) EC_KEY_get0_public_key(key);
  unsigned char *pub_buf = malloc(sizeof(char)*pub_len);
    if (EC_POINT_point2oct(curve, h, POINT_CONVERSION_UNCOMPRESSED, pub_buf, POINT_UNCOMPRESSED_len, ctx) != POINT_UNCOMPRESSED_len) {
    printf("error!");
    exit(0);
  }
  size_t priv_len = BN_num_bytes(x);
  unsigned char *priv_buf = malloc(sizeof(char)*priv_len);
  // BN_bn2binpad(x, priv_buf, priv_len);
  bn2binpad(x, priv_buf, priv_len);
  FILE *pub_file_p = fopen(pub_filename, "wb");
  fwrite(pub_buf, pub_len, 1, pub_file_p);
  fclose(pub_file_p);
  FILE *priv_file_p = fopen(priv_filename, "wb");
  fwrite(priv_buf, priv_len, 1, priv_file_p);
  fclose(priv_file_p);
}

void load_scoring_files(char *enc_db_file, char *enc_sum_x2_file, char *enc_db_y_file,
           char *enc_r3_file, char *psi_file, char *rand_numbers_file, char *n_list_file, double threshold) {
  file_to_buffer(enc_db_file, &enc_db);
  file_to_buffer(enc_sum_x2_file, &enc_sum_x2);
  file_to_buffer(enc_db_y_file, &enc_db_y);
  file_to_buffer(enc_r3_file, &enc_r3);
  file_to_buffer(psi_file, (unsigned char**)&psi);

  // STOPPED HERE !! why psi[0] is zero ??? must be 25.442683
  // TODO go to file plda.cc. Read from the file to buffer, then check psi[0] !!!!!!!!!!!!
  // printf("%s \n", psi_file);
  // printf("psi[0]=%f\n", psi[0]);
  // printf("psi= [");
  // for (int i=0; i<200; i++)
  //   printf("%f, ", psi[i]);
  // printf("]\n");

  file_to_buffer(rand_numbers_file, &rand_numbers);
  file_to_buffer(n_list_file, (unsigned char**)&n_list);
  FILE *file = fopen(enc_sum_x2_file, "rb");
  fseek(file, 0L, SEEK_END);
  N = ftell(file) / 130;
  fclose(file);
  file = fopen(enc_db_y_file, "rb");
  fseek(file, 0L, SEEK_END);
  size_t stored_indices = ftell(file)/130/200/N;
  fclose(file);
  dby_portion = (stored_indices == 256) ? 1 : (stored_indices == 128) ? 2 : (stored_indices == 86) ? 3 : (stored_indices == 64) ? 4 : 0;
  first_index = (dby_portion == 1) ? 0 : (dby_portion == 2) ? 64 : (dby_portion == 3) ? 85 : (dby_portion == 4) ? 96 : 0;
  last_index = (dby_portion == 1) ? 256 : (dby_portion == 2) ? 192 : (dby_portion == 3) ? 171 : (dby_portion == 4) ? 160 : 0;
  char *parser = rand_numbers;
  r1_list = (unsigned char**) malloc(N * sizeof(unsigned char*));
  r2_list = (unsigned char**) malloc(N * sizeof(unsigned char*));
  for (int i=0; i<N; i++) {
    r1_list[i] = (unsigned char*) malloc(20 * sizeof(unsigned char));
    sprintf(r1_list[i], "%s", parser);
    parser += strlen(r1_list[i]) + 1;
    r2_list[i] = (unsigned char*) malloc(130 * sizeof(unsigned char));
    memcpy(r2_list[i], parser, 130);
    parser += 130;
  }
  // char buf[20];
  //   sprintf(buf, "-%d\n", threshold);
  // encrypt_ec(similarity_threshold, buf);
  tau = threshold;
  for (int i=0; i<dim; i++)
    teta[i] = 1.0 / (1.0 + psi[i]);
}

void set_y_from_buf(char *y_file_buff) {
  while(!isspace(*y_file_buff))
    y_file_buff++;
  y_file_buff += 4;
  for(int i=0; i<200; i++) {
    y[i] = atof(y_file_buff);
  while(!isspace(*y_file_buff))
    y_file_buff++;
    y_file_buff++;
  }

  for(int i=0; i<200; i++) {
    yprime[i] = y[i]*alpha + beta;
    normalized_y[i] = (int) round(yprime[i]);
  }

 // double y2_var = 0.0;
 // for(int i=0; i<200; i++)
 //   y2_var += y[i]*y[i]*psi_inversed[i]/1000000.0;
 // y2_var = max_y2_var_value-2*y2_var;
 // y2_var_int = (int) floorf(y2_var);
}

void compute_encrypted_score_y_set(char *enc_score, int index, int with_obfuscation) {
  double xi[dim];
  double yprime[dim];
  for (int k=0; k<dim; k++) {
    xi[k] = 1 / (1 + psi[k] / (n_list[index]*psi[k] + 1));
    yprime[k] = y[k] * alpha + beta;
    if (yprime[k] > 255) yprime[k] = 255;
  }

  double r3 = 0.0;
  for (int k=0; k<dim; k++) 
    r3 += - xi[k]*pow(yprime[k],2) + pow(alpha,2)*teta[k]*pow(y[k],2);
  r3 -= pow(alpha,2) * tau;
  int r3_int = (int)round(r3);

  // r3_int += 100;  // TODO delete
  // printf("ec_elgamal.c: psi[0]= %f xi[0]= %f y[0]= %f yprime[0]= %f n_list[0]= %d r3= %f\n", psi[0], xi[0], y[0], yprime[0], n_list[0], r3);
  // printf("ec_elgamal.c: psi[1]= %f xi[1]= %f y[1]= %f yprime[1]= %f n_list[1]= %d r3= %f\n", psi[1], xi[1], y[1], yprime[1], n_list[1], r3);
  // printf("ec_elgamal.c: psi[7]= %f xi[7]= %f y[7]= %f yprime[7]= %f n_list[7]= %d r3= %f\n", psi[7], xi[7], y[7], yprime[7], n_list[7], r3);
  // printf("ec_elgamal.c: r3_int= %d i= %d index= %d\n", r3_int, r3_int - starting_r3, index);

  add2(enc_score, &enc_r3[(r3_int - starting_r3)*130], &enc_sum_x2[index*130]);

  if (dby_portion == 1) {

    for(int i=0; i<200; i++) {

      add2(enc_score, enc_score, &enc_db_y[(size_t)index*6656000 + i*33280 + normalized_y[i]*130]);  //[index*200*256*130 + i*256*130 + y[i]*130])
      

    }
      
  } else {
    int row_len = 200*(last_index-first_index)*130;   // instead of 200*256*130
    int cell_len = (last_index-first_index)*130;    // instead of 256*130
    char buf[20];
    char ct[130];

    for(int i=0; i<200; i++) {
      if (first_index <= normalized_y[i] && normalized_y[i] < last_index) {
        add2(enc_score, enc_score, &enc_db_y[index*row_len + i*cell_len + (normalized_y[i]-first_index)*130]);

      }
      else {
        sprintf((char *)buf, "%d\n", normalized_y[i]);
        mult(ct, buf, &enc_db[index*26000 + i*130]);  //[index*200*130 + i*130]
        add2(enc_score, enc_score, ct);
        // add2(enc_score, enc_score, &enc_y2[y[i]*130]);
        // add2(enc_score, enc_score, &enc_r3[(r3_int - starting_r3)*130]);
      }
    }
  }
  
    // TODO to delete ////////////////////////
    // char ct5M[130];
    // char ct10M[130];
    // int pt1, pt2;
    // char tempbuf1[20];
    // char tempbuf2[20];
    // char tempct1[130];
    // char tempct2[130];
    //////////////////////////////////////////
    // // TODO to delete ////////////////////////
    // encrypt_ec(ct5M, "5000000");
    // add2(tempct1, ct5M, &enc_sum_x2[index*130]);
    // decrypt_ec(tempbuf1, tempct1);
    // pt1 = atoi(tempbuf1) - 5000000;

    // add2(tempct2, ct5M, &enc_r3[(r3_int - starting_r3)*130]);
    // decrypt_ec(tempbuf2, tempct2);
    // pt2 = atoi(tempbuf2) - 5000000;
    // printf("index=%d, enc_sum_x2=%d, r3_int=%d, r3_index=%d, enc_r3=%s, pt2=%d\n", index, pt1, r3_int, r3_int - starting_r3, tempbuf2, pt2);

    // encrypt_ec(ct10M, "10000000");
    // add2(tempct1, ct10M, enc_r3);
    // decrypt_ec(tempbuf2, tempct1);
    // pt2 = atoi(tempbuf2) - 10000000;
    // printf("should get -8M: dec_r3[0]=%d\n", pt2);
    // add2(tempct1, ct10M, enc_r3+130);
    // decrypt_ec(tempbuf2, tempct1);
    // pt2 = atoi(tempbuf2) - 10000000;
    // printf("should get -8M+1: dec_r3[1]=%d\n", pt2);

    // //////////////////////////////////////////

  // add2(enc_score, enc_score, similarity_threshold); // can be precomputed with enc_y2_var[y2_var_int*130]
  if (with_obfuscation) {
    mult(enc_score, r1_list[index], enc_score);
    add2(enc_score, enc_score, r2_list[index]);
  }

  // printf("ec_elgamal.c: score computed for index=%d\n", index);
}

// void compute_encrypted_score_y_set(char *enc_score, int index, int with_obfuscation) {
// 	add2(enc_score, &enc_y2_var[y2_var_int*130], &enc_sum_x2[index*130]);
// 	if (dby_portion == 1) {
// 		for(int i=0; i<200; i++)
// 			add2(enc_score, enc_score, &enc_db_y[index*6656000 + i*33280 + y[i]*130]);	//[index*200*256*130 + i*256*130 + y[i]*130])
// 	} else {
// 		int row_len = 200*(last_index-first_index)*130;		// instead of 200*256*130
// 		int cell_len = (last_index-first_index)*130;		// instead of 256*130
// 		char buf[20];
//     	char ct[130];
// 		for(int i=0; i<200; i++) {
// 			if (first_index <= y[i] && y[i] < last_index)
// 				add2(enc_score, enc_score, &enc_db_y[index*row_len + i*cell_len + (y[i]-first_index)*130]);
// 			else {
// 				sprintf((char *)buf, "%d\n", y[i]);
// 				mult(ct, buf, &enc_db[index*26000 + i*130]);	//[index*200*130 + i*130]
// 				add2(enc_score, enc_score, ct);
// 				add2(enc_score, enc_score, &enc_y2[y[i]*130]);
// 			}
// 		}
// 	}
// 	add2(enc_score, enc_score, similarity_threshold);	// can be precomputed with enc_y2_var[y2_var_int*130]
// 	if (with_obfuscation) {
// 		mult(enc_score, r1_list[index], enc_score);
// 		add2(enc_score, enc_score, r2_list[index]);
// 	}
// }

// void load_scoring_files(char *enc_db_file, char *enc_sum_x2_file, char *enc_db_y_file,
// 						char *enc_y2_var_file, char *enc_y2_file, char *psi_inversed_file, char *rand_numbers_file, int threshold) {
// 	file_to_buffer(enc_db_file, &enc_db);
// 	file_to_buffer(enc_sum_x2_file, &enc_sum_x2);
// 	file_to_buffer(enc_db_y_file, &enc_db_y);
// 	file_to_buffer(enc_y2_var_file, &enc_y2_var);
// 	file_to_buffer(enc_y2_file, &enc_y2);
// 	file_to_buffer(psi_inversed_file, (unsigned char**)&psi_inversed);
// 	file_to_buffer(rand_numbers_file, &rand_numbers);
// 	FILE *file = fopen(enc_sum_x2_file, "rb");
// 	fseek(file, 0L, SEEK_END);
// 	N = ftell(file) / 130;
// 	fclose(file);
// 	file = fopen(enc_db_y_file, "rb");
// 	fseek(file, 0L, SEEK_END);
// 	size_t stored_indices = ftell(file)/130/200/N;
// 	fclose(file);
// 	dby_portion = (stored_indices == 256) ? 1 : (stored_indices == 128) ? 2 : (stored_indices == 86) ? 3 : (stored_indices == 64) ? 4 : 0;
//     first_index = (dby_portion == 1) ? 0 : (dby_portion == 2) ? 64 : (dby_portion == 3) ? 85 : (dby_portion == 4) ? 96 : 0;
//     last_index = (dby_portion == 1) ? 256 : (dby_portion == 2) ? 192 : (dby_portion == 3) ? 171 : (dby_portion == 4) ? 160 : 0;
// 	char *parser = rand_numbers;
// 	r1_list = (unsigned char**) malloc(N * sizeof(unsigned char*));
// 	r2_list = (unsigned char**) malloc(N * sizeof(unsigned char*));
// 	for (int i=0; i<N; i++) {
// 		r1_list[i] = (unsigned char*) malloc(20 * sizeof(unsigned char));
// 		sprintf(r1_list[i], "%s", parser);
// 		parser += strlen(r1_list[i]) + 1;
// 		r2_list[i] = (unsigned char*) malloc(130 * sizeof(unsigned char));
// 		memcpy(r2_list[i], parser, 130);
// 		parser += 130;
// 	}
// 	char buf[20];
//     sprintf(buf, "-%d\n", threshold);
// 	encrypt_ec(similarity_threshold, buf);
// }

// void set_y_from_buf(char *y_file_buff) {
// 	while(!isspace(*y_file_buff))
// 		y_file_buff++;
// 	y_file_buff += 4;
// 	for(int i=0; i<200; i++) {
// 		y[i] = atoi(y_file_buff);
// 		while(!isspace(*y_file_buff))
// 			y_file_buff++;
// 		y_file_buff++;
// 	}
// 	double y2_var = 0.0;
// 	for(int i=0; i<200; i++)
// 		y2_var += y[i]*y[i]*psi_inversed[i]/1000000.0;
// 	y2_var = max_y2_var_value-2*y2_var;
// 	y2_var_int = (int) floorf(y2_var);
// }

void get_y_from_file(char *y_file) {
	unsigned char *y_file_buff;
	file_to_buffer(y_file, &y_file_buff);
	set_y_from_buf((char *)y_file_buff);
}

void test() {
  prepare("ec_pub.txt", "ec_priv.txt");
  // generate_decrypt_file();
  // load_encryption_file();
  char* ct1 = (char*) malloc (130 * sizeof(char));
  encrypt_ec(ct1, "1234");
  char pt1[20];
  decrypt_ec(pt1, ct1);
  printf("pt1=%s\n", pt1);
  // printf("dec_zero_nonzero(ct1)=%d\n", dec_zero_nonzero(ct1));
  // // printf("score_is_positive(ct1)=%d\n", score_is_positive(ct1));

  // char* ct2 = (char*) malloc (130 * sizeof(char));
  // encrypt_ec(ct2, "28734");
  // char pt2[20];
  // decrypt_ec(pt2, ct2);
  // printf("pt2=%s\n", pt2);
  // printf("dec_zero_nonzero(ct2)=%d\n", dec_zero_nonzero(ct2));
  // // printf("score_is_positive(ct2)=%d\n", score_is_positive(ct2));

  // encrypt_ec(ct1, "128734");
  // decrypt_ec(pt1, ct1);
  // printf("pt1=%s\n", pt1);
  // // printf("score_is_positive(ct1)=%d\n", score_is_positive(ct1));

  // encrypt_ec(ct2, "-128734");
  // decrypt_ec(pt2, ct2);
  // printf("pt2=%s\n", pt2);
  // // printf("score_is_positive(ct2)=%d\n", score_is_positive(ct2));

  // add2(ct2, ct2, ct1);
  // decrypt_ec(pt2, ct2);
  // printf("pt2=%s\n", pt2);
  // printf("dec_zero_nonzero(ct2)=%d\n", dec_zero_nonzero(ct2));
  // // printf("score_is_positive(ct2)=%d\n", score_is_positive(ct2));
}

int main() {
}