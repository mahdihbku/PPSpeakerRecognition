// privacy_code/ec_elgamal.h

#ifndef EC_ELGAMAL_H_
#define EC_ELGAMAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <openssl/obj_mac.h>
#include <openssl/ec.h>
#include <openssl/rand.h>
#include <openssl/bn.h>

class EC_elgamal {
public:
	EC_elgamal();
	int prepare(const char *pub_filename, const char *priv_filename);
	int prepare_for_enc(const char *pub_filename);
	int encrypt_ec(unsigned char *cipherText, unsigned char *mess);
	int load_encryption_file();
	int score_is_positive(unsigned char *ciphert);
	int decrypt_ec(unsigned char *message, unsigned char *ciphert);
	int generate_decrypt_file();
	int add2(unsigned char *result, unsigned char *ct1, unsigned char *ct2);
	int add3(unsigned char *result, unsigned char *ct1, unsigned char *ct2, unsigned char *ct3);
	int add4(unsigned char *result, unsigned char *ct1, unsigned char *ct2, unsigned char *ct3, unsigned char *ct4);
	int mult(unsigned char *result, unsigned char *scalar, unsigned char *ct1);
	int generate_keys(char *pub_filename, char *priv_filename);
	int test();
private:
	BN_CTX *ctx;
	EC_GROUP *curve;
	const size_t priv_len = 32;
	const size_t pub_len = 65;
	const size_t POINT_UNCOMPRESSED_len = 65;	// z||x||y 		z is the octet 0x04
	const size_t POINT_COMPRESSED_len = 33;		// z||x			z specifies which solution of the quadratic equation y is (0x02 or 0x03)
	EC_POINT *h, *g;	//h: pub_key
	BIGNUM *x, *q;		//x: priv_key
	unsigned char* decryption_buf;
};

#endif