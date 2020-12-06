%module ec_elgamal
%include "cdata.i"
%{
/* Put header files here or function declarations like below */
#define SWIG_FILE_WITH_INIT
#define SWIG_PYTHON_STRICT_BYTE_CHAR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <openssl/obj_mac.h>
#include <openssl/ec.h>
#include <openssl/rand.h>
#include <openssl/bn.h>

extern BN_CTX *ctx;
extern EC_GROUP *curve;
extern EC_POINT *h, *g;		//h: pub_key
extern BIGNUM *x, *q;		//x: priv_key

extern int prepare(char *pub, char *priv);
extern int prepare_for_enc(char *pub);
extern void generate_decrypt_file();
extern void load_encryption_file();
extern void load_scoring_files(char *enc_db_file, char *enc_sum_x2_file, char *enc_db_y_file, char *enc_y2_var_file, char *enc_y2_file, char *psi_inversed_file, char *rand_numbers_file, double threshold);
extern void get_y_from_file(char *y_file);
extern void set_y_from_buf(char *y_file_buff);
extern void generate_keys(char *pub_filename, char *priv_filename);
extern int score_is_positive(char *ciphert);
//extern int dec_zero_nonzero(char *ciphert);
extern void decrypt_ec(char *message, char *ciphert);
extern void encrypt_ec(char *cipherText, char *mess);
extern void decrypt_ec_time_only(char *message, char *ciphert);
extern void add2(char *result, char *ct1, char *ct2);
extern void add3(char *result, char *ct1, char *ct2, char *ct3);
extern void add4(char *result, char *ct1, char *ct2, char *ct3, char *ct4);
extern void mult(char *result, char *scalar, char *ct1);
extern void compute_encrypted_score_y_set(char *enc_score, int index, int with_obfuscation);
extern void test();
%}

%include "typemaps.i"
%include "cstring.i"

extern int prepare(char *pub, char *priv);
extern int prepare_for_enc(char *pub);
extern void generate_decrypt_file();
extern void load_encryption_file();
extern void load_scoring_files(char *enc_db_file, char *enc_sum_x2_file, char *enc_db_y_file, char *enc_y2_var_file, char *enc_y2_file, char *psi_inversed_file, char *rand_numbers_file, double threshold);
extern void get_y_from_file(char *y_file);
extern void set_y_from_buf(char *y_file_buff);
extern int score_is_positive(char *ciphert);
//extern int dec_zero_nonzero(char *ciphert);
extern void generate_keys(char *pub_filename, char *priv_filename);
extern void test();

%cstring_bounded_output(char *message, 30);
extern void decrypt_ec(char *message, char *ciphert);

%cstring_chunk_output(char *cipherText, 130);
extern void encrypt_ec(char *cipherText, char *mess);

%cstring_chunk_output(char *result, 130);
extern void add2(char *result, char *ct1, char *ct2);

%cstring_chunk_output(char *result, 130);
extern void add3(char *result, char *ct1, char *ct2, char *ct3);

%cstring_chunk_output(char *result, 130);
extern void add4(char *result, char *ct1, char *ct2, char *ct3, char *ct4);

%cstring_chunk_output(char *result, 130);
extern void mult(char *result, char *scalar, char *ct1);

%cstring_chunk_output(char *enc_score, 130);
extern void compute_encrypted_score_y_set(char *enc_score, int index, int with_obfuscation);