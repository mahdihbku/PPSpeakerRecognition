// This code is to compare between between our privacy-preserving speaker recognition method
// method to Treiber et al.'s

//gcc -O3 sim2.c -lcrypto -pthread -lm
//./a.out M threads_number
//./a.out 8 4

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/bn.h>
#include <openssl/aes.h>
#include <openssl/bn.h>
#include <math.h>

#define MAXTHREADS 1024		// to be changed if more threads are needed

unsigned int server_t = 4;	// nbr of parallel threads
unsigned int total_elems_count = 0;	// nbr of elements to process
unsigned int server_elems_per_thread = 0;	// will be =total_elems_count/server_t

unsigned int M = 1;			// nbr of suspects in DB
unsigned int l = 64;		// feature bit-length size
unsigned int k = 128;		// symmetric security parameter
unsigned int dim = 200;		// dimensionality of embeddings
unsigned int  comm_per_suspect = 8;		// in MB
unsigned int rounds = 4;
unsigned char *random_buffer, *buf1, *buf2, *buf3;
size_t random_buffer_size = 0;
unsigned int nbr_symm_crypto;
unsigned int nbr_products;
unsigned int nbr_additions;
double comp_time[MAXTHREADS];

AES_KEY wctx;

static const unsigned char key[] = {
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

double print_time(struct timeval *start, struct timeval *end);
void *step1(void *vargp);

typedef enum {big, little} endianess_t;
// I need these  because of a problem with OpenSsl on my machine.
// If not needed change bn2binpad to BN_bn2binpad !
static int bn2binpad(const BIGNUM *a, unsigned char *to, int tolen, endianess_t endianess);

int min(unsigned int a, unsigned int b) {
	if (a<b)	return a;
	else return b;
}

double max_comp_time(unsigned int count) {
	double max = 0.0;
	for (int i=0; i<count; i++)
		if (comp_time[i] > max)
			max = comp_time[i];
	return max;
}

int main(int argc, char **argv)
{
	struct timeval start, end;

	if (RAND_load_file("/dev/urandom", 1024) < 64) {
		printf("PNRG not seeded!\n");
		abort();
	}
	if (argc != 3) {
		printf("Usage: %s <M>\n", argv[0]);
		exit(0);
	}
	M = atoi(argv[1]);
	server_t = atoi(argv[2]);

	nbr_symm_crypto = 15*l;
	nbr_products = 8*(4*dim*dim + 5*dim);
	nbr_additions = 64*dim*dim + 74*dim;

	random_buffer_size = 32*M*sizeof(unsigned char);
	random_buffer = (unsigned char *) malloc(random_buffer_size);
	buf1 = (unsigned char *) malloc(random_buffer_size);
	FILE *fp = fopen("/dev/urandom","rb");
	int size = fread(random_buffer, sizeof(unsigned char), random_buffer_size, fp);
	size += fread(buf1, sizeof(unsigned char), random_buffer_size, fp);
	if (size != 2*random_buffer_size)
		printf("urandom not big enough\n");

	AES_set_encrypt_key(key, 128, &wctx);

	buf2 = (unsigned char *) malloc(2*random_buffer_size);

	pthread_t tid[MAXTHREADS];
	unsigned int myid[MAXTHREADS];
	int v = 0;

	total_elems_count = M;
	server_elems_per_thread = total_elems_count/server_t;
	if (total_elems_count%server_t != 0) server_elems_per_thread++;
	gettimeofday(&start,NULL);
	for (v=0; v<min(server_t, total_elems_count); v++) {
		myid[v] = v;
		pthread_create(&tid[v], NULL, step1, &myid[v]);
	}
	for (v=0; v<min(server_t, total_elems_count); v++)
		pthread_join(tid[v], NULL);
	gettimeofday(&end,NULL);

	printf("buf2[0]=%02X buf2[last]=%02X\n", buf2[0], buf2[2*random_buffer_size-1]);

	printf("M=\t%d\tcomp=\t%lf\tcomm=\t%d\tMB\n", M, max_comp_time(total_elems_count), comm_per_suspect*M);
	FILE *fptr = fopen("sim2_results.txt","a");
	fprintf(fptr, "M=\t%d\tcomp=\t%lf\tcomm=\t%d\tMB\n", M, max_comp_time(total_elems_count), comm_per_suspect*M);
	fclose(fptr);
}

void *step1(void *vargp) {
	unsigned int myid = *((unsigned int *)vargp);
	unsigned int start = myid * server_elems_per_thread;
	unsigned int end = (myid != server_t - 1) ? start + server_elems_per_thread : total_elems_count;
	BN_CTX *ctx = BN_CTX_new();
	BIGNUM *ct1 = BN_new();
	BIGNUM *ct2 = BN_new();

	double enc_time = 0.0, mult_time = 0.0, add_time = 0.0;
	struct timeval start_time, end_time;
		
	for (unsigned int i=start; i<end; i++) {
		
		gettimeofday(&start_time,NULL);
		for (unsigned int j=0; j<nbr_symm_crypto; j++)
			AES_encrypt(&random_buffer[i*32], &random_buffer[i*32], &wctx);
		gettimeofday(&end_time,NULL);
		enc_time += print_time(&start_time, &end_time);

		for (unsigned int j=0; j<nbr_products; j++) {
			BN_bin2bn(&random_buffer[i*32], 16, ct1);
			BN_bin2bn(&buf1[i*32], 16, ct2);
		gettimeofday(&start_time,NULL);
			BN_mul(ct1, ct1, ct2, ctx);
		gettimeofday(&end_time,NULL);
		mult_time += print_time(&start_time, &end_time);
			bn2binpad(ct1, &buf2[i*32*2], 16*2, little);
		}

		for (unsigned int j=0; j<nbr_additions; j++) {
			BN_bin2bn(&buf1[i*32], 16, ct1);
			BN_bin2bn(&buf2[i*32*2], 16*2, ct2);
		gettimeofday(&start_time,NULL);
			BN_add(ct1, ct1, ct2);
		gettimeofday(&end_time,NULL);
		add_time += print_time(&start_time, &end_time);
			bn2binpad(ct1, &buf2[i*32*2], 16*2, little);
		}

	}

	comp_time[myid] = enc_time + mult_time + add_time;
	// printf("M=\t%d\tenc_time=\t%lf\tmult_time=\t%lf\tadd_time=\t%lf\tcomp_time[%d]=%lf\n", M, enc_time, mult_time, add_time, myid, comp_time[myid]);	

}

double print_time(struct timeval *start, struct timeval *end) {
	double usec = (end->tv_sec*1000000 + end->tv_usec) - (start->tv_sec*1000000 + start->tv_usec);
	return usec/1000.0;
}

static int bn2binpad(const BIGNUM *a, unsigned char *to, int tolen, endianess_t endianess) {
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
    if (endianess == big)
        to += tolen; /* start from the end of the buffer */
    for (i = 0, j = 0; j < (size_t)tolen; j++) {
        unsigned char val;
        l = a->d[i / BN_BYTES];
        mask = 0 - ((j - atop) >> (8 * sizeof(i) - 1));
        val = (unsigned char)(l >> (8 * (i % BN_BYTES)) & mask);
        if (endianess == big)
            *--to = val;
        else
            *to++ = val;
        i += (i - lasti) >> (8 * sizeof(i) - 1); /* stay on last limb */
    }

    return tolen;
}