//privacy_code/generate_client_files.cc

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "privacy_code/ec_elgamal.h"
#include <sstream>
#include <string>
#include <fstream>
#include <ios>
#include <ctype.h>
#include <pthread.h>

using namespace std;
using namespace kaldi;

#define MAXTHREADS 1024
unsigned int max_r3_value = 10500000;
int starting_r3 = -8005000;
unsigned int nbr_threads = 4;
unsigned int elems_per_thread = 0;
unsigned int N = 0;
unsigned int dby_portion = 1;
unsigned int first_index = 0, last_index = 256;
unsigned char *enc_db, *enc_db_y, *enc_r3;
string pub_key_file = "";

// EC_elgamal ec_elgamal;

void *sub_func(void *vargp) {
    EC_elgamal ec_elgamal;
    ec_elgamal.prepare_for_enc(pub_key_file.c_str()); // TODO uncomment
    // ec_elgamal.prepare(pub_key_file.c_str(), "/home/mahdi/Downloads/kaldi-master/egs/voxceleb/biggest_files_count/client_dir/ec_priv.txt"); //TODO delete

    unsigned int myid = *((unsigned int *)vargp);
    unsigned int start = myid * elems_per_thread;
    unsigned int end = (myid != nbr_threads - 1) ? start + elems_per_thread : N;
    unsigned char *db_parser = enc_db + start*130*200;
    unsigned char *db_y_parser = enc_db_y + start*130*200*(last_index-first_index);
    unsigned char buf[20];
    for (unsigned int i=start*200; i<end*200; i++) {
        for (unsigned int y=first_index; y<last_index; y++) {
            sprintf((char *)buf, "%d\n", y);
            ec_elgamal.mult(db_y_parser, buf, db_parser);
            db_y_parser += 130;
        }
        db_parser += 130;
    }
    return 0;
}


void *enc_r3_value(void *vargp) {
    EC_elgamal ec_elgamal;
    ec_elgamal.prepare_for_enc(pub_key_file.c_str()); // TODO uncomment
    // ec_elgamal.prepare(pub_key_file.c_str(), "/home/mahdi/Downloads/kaldi-master/egs/voxceleb/biggest_files_count/client_dir/ec_priv.txt"); //TODO delete

    unsigned int myid = *((unsigned int *)vargp);
    unsigned int start = myid * elems_per_thread;
    unsigned int end = (myid != nbr_threads - 1) ? start + elems_per_thread : max_r3_value;
    unsigned char buf[20];
    unsigned char *enc_r3_parser = enc_r3 + start*130;
    for (int i=start; i<end; i++) {
        if (i+starting_r3 >= 0)
            sprintf((char *)buf, "%d\n", i+starting_r3);
        else
            sprintf((char *)buf, "-%d\n", -(i+starting_r3));
        ec_elgamal.encrypt_ec(enc_r3_parser, buf);
        enc_r3_parser += 130;
    }
    return 0;
}

int main(int argc, char *argv[]) {
	const char *usage =
        "Generate encrypted client files\n"
        "\n"
        "Usage: generate_client_files <working_dir> <pub_key_file> <enc_db_file> <enc_db_y_file> <enc_r3_file> <nbr_threads> <dby_portion>}\n"
        "\n"
        "e.g.: generate_client_files client_dir ec_pub.txt rec_enc_db.bin enc_db_y.bin enc_r3.bin 4 1\n"
        "\n";
    ParseOptions po(usage);

    po.Read(argc, argv);

    if (po.NumArgs() != 7) {
      po.PrintUsage();
      exit(1);
    }

    string working_dir, enc_db_file, enc_db_y_file, enc_r3_file;
    working_dir     = po.GetArg(1);
    pub_key_file    = po.GetArg(2);
    enc_db_file     = po.GetArg(3);
    enc_db_y_file   = po.GetArg(4);
    enc_r3_file     = po.GetArg(5);
    nbr_threads     = std::stoi(po.GetArg(6));
    dby_portion     = std::stoi(po.GetArg(7));

    first_index = (dby_portion == 1) ? 0 : (dby_portion == 2) ? 64 : (dby_portion == 3) ? 85 : (dby_portion == 4) ? 96 : 0;
    last_index = (dby_portion == 1) ? 256 : (dby_portion == 2) ? 192 : (dby_portion == 3) ? 171 : (dby_portion == 4) ? 160 : 0;

    ifstream file(enc_db_file.c_str(), ios::binary | ios::ate);
    streamsize db_size = file.tellg();
    file.seekg(0, ios::beg);
    enc_db = (unsigned char *) malloc(db_size);
    if (!file.read((char *)enc_db, db_size)) {
        cout << "generate_client_files: Error! Couldn't read encrypted db file" << endl;
        abort();
    }

    N = db_size/130/200;
    elems_per_thread = N/nbr_threads;
    if (N%nbr_threads != 0) elems_per_thread++;
    pthread_t tid[MAXTHREADS];
    unsigned int myid[MAXTHREADS];

    enc_db_y = (unsigned char *) malloc(db_size * (last_index-first_index));  // depends on the loading factor //TODO to be changed later on!!!
    
    int v=0;
    for (v=0; v<nbr_threads; v++) {
        myid[v] = v;
        pthread_create(&tid[v], NULL, sub_func, &myid[v]);
    }
    for (v=0; v<nbr_threads; v++)
        pthread_join(tid[v], NULL);

    // Writing encrypted file to disk
    FILE *enc_db_y_file_p = fopen(enc_db_y_file.c_str(), "wb");
    fwrite(enc_db_y, db_size * (last_index-first_index), 1, enc_db_y_file_p);
    fclose(enc_db_y_file_p);


    // // TODO UNCOMMENT !!! This is to generate the r3 table (1.4GB)
    // enc_r3 = (unsigned char *) malloc((size_t)max_r3_value * 130); // depends on the loading factor //TODO to be changed later

    // elems_per_thread = max_r3_value/nbr_threads;
    // if (max_r3_value%nbr_threads != 0) elems_per_thread++;

    // for (v=0; v<nbr_threads; v++) {
    //     myid[v] = v;
    //     pthread_create(&tid[v], NULL, enc_r3_value, &myid[v]);
    // }
    // for (v=0; v<nbr_threads; v++)
    //     pthread_join(tid[v], NULL);

    // // Writing encrypted file to disk
    // FILE *enc_r3_file_p = fopen(enc_r3_file.c_str(), "wb");
    // fwrite(enc_r3, max_r3_value * 130, 1, enc_r3_file_p);
    // fclose(enc_r3_file_p);

    return db_size;
}