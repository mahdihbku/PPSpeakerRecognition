//privacy_code/generate_encrypted_db.cc

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "privacy_code/ec_elgamal.h"
#include <string>
#include <fstream>
#include <pthread.h>

using namespace std;
using namespace kaldi;

#define MAXTHREADS 1024
string pub_key_file = "";
unsigned int nbr_threads = 4;
unsigned int elems_per_thread = 0;
unsigned int N = 0; // number of suspects in db
unsigned char *encrypted_db, *encrypted_sum_x2;
vector<string> names_list;
vector<Vector<BaseFloat>> vectors_tab;
vector<int> n_list; // number of utterances for every speaker (set to 1 for now)
const int dim = 200;
double Psi[dim];
double Teta[dim];

const double alpha = 13.5, beta = 135.0, tau = -40.44;

void *sub_func(void *);

int main(int argc, char *argv[]) {
	const char *usage =
        "Generate encrypted database files\n"
        "\n"
        "Usage: generate_encrypted_db <db-rspecifier> <pub_key> <encrypted_db_file> <encrypted_sum_x2_file> <names_list_file> <n_list_file> <psi_file> <nj>} \n"
        "\n"
        "e.g.: generate_encrypted_db ark:db_vectors.ark ec_pub.txt encrypted_db.bin encrypted_sum_x2.bin names_list.txt n_list.txt psi.data 4\n"
        "\n";
    ParseOptions po(usage);

    po.Read(argc, argv);

    if (po.NumArgs() != 8) {
      po.PrintUsage();
      exit(1);
    }

    string db_rspecifier = po.GetArg(1),
        encrypted_db_file = po.GetArg(3),
        encrypted_sum_x2_file = po.GetArg(4),
        names_list_file = po.GetArg(5),
        n_list_file = po.GetArg(6),
        psi_file = po.GetArg(7);
    pub_key_file = po.GetArg(2);
    nbr_threads = stoi(po.GetArg(8));

    SequentialBaseFloatVectorReader db_reader(db_rspecifier);
    int num_examples = 1;
    for (; !db_reader.Done(); db_reader.Next()) {
        names_list.push_back(db_reader.Key());
        const Vector<BaseFloat> &vector = db_reader.Value();
        vectors_tab.push_back(vector);
        n_list.push_back(num_examples);
    }
    db_reader.Close();
    N = names_list.size();

    std::ifstream infile;
    infile.open(psi_file, std::ofstream::binary);
    infile.read((char*)Psi, dim*sizeof(double));
    infile.close();

    encrypted_db = (unsigned char *) malloc(N * 130 * dim * sizeof(unsigned char));
    encrypted_sum_x2 = (unsigned char *) malloc(N * 130 * sizeof(unsigned char));

    elems_per_thread = N/nbr_threads;
    if (N % nbr_threads != 0) elems_per_thread++;
    pthread_t tid[MAXTHREADS];
    unsigned int myid[MAXTHREADS];
    int v=0;
    for (v=0; v<nbr_threads; v++) {
        myid[v] = v;
        pthread_create(&tid[v], NULL, sub_func, &myid[v]);
    }
    for (v=0; v<nbr_threads; v++)
        pthread_join(tid[v], NULL);

    //Writing encrypted files to disk
    FILE *encrypted_db_file_p = fopen(encrypted_db_file.c_str(), "wb");
    fwrite(encrypted_db, N*130*dim, 1, encrypted_db_file_p);
    fclose(encrypted_db_file_p);
    FILE *encrypted_sum_x2_file_p = fopen(encrypted_sum_x2_file.c_str(), "wb");
    fwrite(encrypted_sum_x2, N*130, 1, encrypted_sum_x2_file_p);
    fclose(encrypted_sum_x2_file_p);
    bool binary = false;
    Output ko1(names_list_file, binary);
    unsigned int n_buf[N];
    for(int i=0; i<N; i++) {
        ko1.Stream() << names_list.at(i) << endl;
        n_buf[i] = n_list.at(i);
    }
    ko1.Close();
    std::ofstream n_file_p;
    n_file_p.open(n_list_file, std::ofstream::binary);
    n_file_p.write((char*)n_buf, N*sizeof(unsigned int));
    n_file_p.close();

    // for(int i=0; i<N; i++) {
    //     printf("suspect %i: ", i);
    //     for (int j=0; j<dim; j++)
    //         printf("%f ", (double)vectors_tab.at(i).Data()[j]);
    //     printf("\n");
    // }

    return 0;
}

void *sub_func(void *vargp) {

    EC_elgamal ec_elgamal;
    ec_elgamal.prepare_for_enc(pub_key_file.c_str());    // TODO uncomment!
    
    //TODO to delete //////////////
    // ec_elgamal.prepare(pub_key_file.c_str(), "/home/mahdi/workspace/speaker_recognition/tools/kaldi-master/egs/voxceleb/pp/ec_priv.txt");
    ///////////////////////////////

    unsigned int myid = *((unsigned int *)vargp);
    unsigned int start = myid * elems_per_thread;
    unsigned int end = (myid != nbr_threads - 1) ? start + elems_per_thread : N;
    double Xi[dim];

    // std::cout << "generate_encrypted_db.cc: sub_func: id=" << myid << std::endl;

    unsigned char *db_parser = encrypted_db + start*130*dim;
    unsigned char *x2_sum_parser = encrypted_sum_x2 + start*130;

    unsigned char buf[20];
    for(int i=start; i<end; i++) {
        for (int k=0; k<dim; k++)
            Xi[k] = 1 / (1 + Psi[k] / (n_list.at(i)*Psi[k] + 1));
        Vector<double> vector_dbl(vectors_tab.at(i));
        int j;
        double m, mprime, r1, r2=0;
        for (j=0; j<dim; j++) {
            m = (double)vector_dbl.Data()[j];
            mprime = m*alpha + beta;
            r1 = Xi[j]*mprime;

                // // TODO to delete ////////////////////////
                // if (j % 30 == 0)
                //     printf("i=%d, j=%d, r1=%f, int_r1=%d\n", i, j, r1, (int)(2*round(r1)));
                // // TODO to delete ////////////////////////
            
            sprintf((char *)buf, "%d\n", (int)(2*round(r1)));
            ec_elgamal.encrypt_ec(db_parser, buf);
            db_parser += 130;
            r2 += Xi[j]*mprime*mprime;
        }
        sprintf((char *)buf, "-%d\n", (int)round(r2));     // NOTE the minus
        ec_elgamal.encrypt_ec(x2_sum_parser, buf);
        x2_sum_parser += 130;

        // TODO to delete/////////////
        // printf("----------------i=%d r2=%f\n", i, r2);

        // TODO to delete ////////////////////////
        // unsigned char tempbuf1[20];
        // unsigned char tempbuf2[20];
        // unsigned char tempct[130];
        // unsigned char tempct1[130];
        // unsigned char ct5M[130];
        // unsigned char toAdd[] = "5000000";
        // int pt1;
        // ec_elgamal.encrypt_ec(ct5M, toAdd);
        // ec_elgamal.add2(tempct1, ct5M, &encrypted_sum_x2[i*130]);
        // ec_elgamal.decrypt_ec(tempbuf1, tempct1);
        // pt1 = atoi((char *)tempbuf1) - 5000000;
        // // TODO to delete ////////////////////////
        // ec_elgamal.decrypt_ec(tempbuf1, &encrypted_db[i*200*130 + 0*130]);
        // ec_elgamal.decrypt_ec(tempbuf2, &encrypted_db[i*200*130 + 1*130]);
        // printf("i=%d, enc_db[0]=%s, enc_db[1]=%s encrypted_sum[%d]=%d\n", i, tempbuf1, tempbuf2, i, pt1);
        //////////////////////////////////////////
        //////////////////////////////////////////
        //////////////////////////////

    }
    return 0;
}
