// TODO ...

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "privacy_code/ec_elgamal.h"
#include <sstream>
#include <string>
#include <fstream>
#include <chrono>

using namespace std;
using namespace kaldi;

EC_elgamal ec_elgamal;

int get_vector_values(std::string vector_rspecifier, std::string &spkr_name, int **spkr_vector, int &spkr_y2_sum);
size_t generate_encrypted_db(std::string db_rspecifier, std::vector<std::string> &names_list, unsigned char **encrypted_db, unsigned char **encrypted_sum_x2);

int main(int argc, char *argv[]) {
	const char *usage =
        "Computes encrypted log-likelihood ratios for trials using PLDA model\n"
        "\n"
        "Usage: sim <db-rspecifier> <vector-rspecifier> <scores-wxfilename>\n"
        "\n"
        "e.g.: sim plda ark:db_vectors.ark ark:vector.ark scores.bin\n"
        "\n";
    ParseOptions po(usage);

    po.Read(argc, argv);

    if (po.NumArgs() != 3) {
      po.PrintUsage();
      exit(1);
    }

    std::string db_rspecifier = po.GetArg(1),
        vector_rspecifier = po.GetArg(2),
        scores_wxfilename = po.GetArg(3);

    cout << "main: preparing the cryptosystem..." << endl;
	char pub_file[] = "ec_pub.txt";
	char priv_file[] = "ec_priv.txt";
	// ec_elgamal.generate_keys(pub_file, priv_file);
	ec_elgamal.prepare(pub_file, priv_file);
	// ec_elgamal.test();


    cout << "main: generating encrypted db..." << endl;
	char encrypted_db_file[] = "encrypted_db.bin";
	char encrypted_sum_x2_file[] = "encrypted_sum_x2.bin";
	char names_list_file[] = "names_list.txt";
	unsigned char *encrypted_db, *encrypted_sum_x2;
	std::vector<std::string> names_list;
	size_t db_size, i=0;

	auto start = chrono::high_resolution_clock::now();
	db_size = generate_encrypted_db(db_rspecifier, names_list, &encrypted_db, &encrypted_sum_x2);
	auto end = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
	cout << "main: db_size=" << db_size << endl;
	cout << "main: encrypt db computation: " << duration.count() << " msec" << endl;

    cout << "main: getting the y vector (new speaker)..." << endl;
	std::string spkr_name;
	int *spkr_vector;
	int spkr_y2_sum;

	get_vector_values(vector_rspecifier, spkr_name, &spkr_vector, spkr_y2_sum);

	//write encrypted files to disk//////////////////////
    cout << "main: writing data to disk..." << endl;
	FILE *encrypted_db_file_p = fopen(encrypted_db_file, "wb");
	fwrite(encrypted_db, db_size*130*200, 1, encrypted_db_file_p);
	fclose(encrypted_db_file_p);
	FILE *encrypted_sum_x2_file_p = fopen(encrypted_sum_x2_file, "wb");
	fwrite(encrypted_sum_x2, db_size*130, 1, encrypted_sum_x2_file_p);
	fclose(encrypted_sum_x2_file_p);
	bool binary = false;
    Output ko(names_list_file, binary);
	for(; i<db_size; i++)
		ko.Stream() << names_list.at(i) << std::endl;
	ko.Close();
	//////////////////////////////////////////////////////

    cout << "main: computing encrypted scores..." << endl;
	unsigned char encrypted_y2_sum[130];
	unsigned char y2_sum_ex[] = "50";		// TODO just an example
	ec_elgamal.encrypt_ec(encrypted_y2_sum, y2_sum_ex);

	start = chrono::high_resolution_clock::now();

	unsigned char encrypted_scores[130*db_size];
	unsigned char *encrypted_scores_parser = encrypted_scores;
	unsigned char encrypted_score[130];
	unsigned char *encrypted_db_parser = encrypted_db;
	unsigned char *encrypted_sum_x2_parser = encrypted_sum_x2;

	for (int i=0; i<db_size; i++) {
		memcpy(encrypted_score, encrypted_y2_sum, 130);
		for (int j=0; j<200; j++) {
			ec_elgamal.add2(encrypted_score, encrypted_score, encrypted_db_parser);
			encrypted_db_parser += 130;
		}
		ec_elgamal.add2(encrypted_score, encrypted_score, encrypted_sum_x2_parser);
		encrypted_sum_x2_parser += 130;
		memcpy(encrypted_scores_parser, encrypted_score, 130);
		encrypted_scores_parser += 130;
	}

	end = chrono::high_resolution_clock::now();
	duration = chrono::duration_cast<chrono::milliseconds>(end - start);
	cout << "score computation: " << duration.count() << " msec" << endl;
    cout << "main: done." << endl;

	//prints for verification/////////////////////////////
    cout << "main: verifications:" << endl;
	unsigned char pt1[20];
	// cout << "main_verification:x0=";
	// for (i=0; i<200; i++) {
	// 	ec_elgamal.decrypt_ec(pt1, &encrypted_db[130*i]);
	// 	cout << pt1 << ",";
	// }
	// ec_elgamal.decrypt_ec(pt1, encrypted_sum_x2);
	// cout << endl << "main_verification:sum(x0i^2)=" << pt1 << endl;
	// cout << "main_verification:y=";
	// for (i=0; i<200; i++)
	// 	cout << spkr_vector[i] << ",";
	// cout << endl << "main_verification:sum(yi^2)=" << spkr_y2_sum << endl;
	// for (int i=0; i<db_size; i++) {
	for (int i=0; i<1; i++) {
		ec_elgamal.decrypt_ec(pt1, &encrypted_scores[130*i]);
		cout << "main:   score " << i << " = " << pt1 << endl;
	}
	//////////////////////////////////////////////////////

    // bool binary = false;
    // Output ko(scores_wxfilename, binary);
    // for (; i<db_size; i++) {
    //   ko.Stream() << file_spk << ' ' << db_spk << ' ' << score << std::endl;
    // }
}

int get_vector_values(std::string vector_rspecifier, std::string &spkr_name, int **spkr_vector, int &spkr_y2_sum) {
	SequentialBaseFloatVectorReader vector_reader(vector_rspecifier);
    spkr_name = vector_reader.Key();
    const Vector<BaseFloat> &vector = vector_reader.Value();
    Vector<double> vector_dbl(vector);
    *spkr_vector = (int *) malloc(200 * sizeof(int));
	// cout << "get_vector_values:y=";
    int i=0, y, sum=0;
    for (; i<200; i++) {
    	y = (int)vector_dbl.Data()[i];
		// cout << y << ",";
    	(*spkr_vector)[i] = y;
    	sum += y*y;
    }
    cout << endl;
    spkr_y2_sum = sum;
	// cout << "get_vector_values:spkr_y2_sum=" << spkr_y2_sum << endl;
    return 0;
}

size_t generate_encrypted_db(std::string db_rspecifier, std::vector<std::string> &names_list, unsigned char **encrypted_db, unsigned char **encrypted_sum_x2) {
	SequentialBaseFloatVectorReader db_reader(db_rspecifier);
	for (; !db_reader.Done(); db_reader.Next())
		names_list.push_back(db_reader.Key());
	db_reader.Close();
    *encrypted_db = (unsigned char *) malloc(names_list.size() * 130 * 200 * sizeof(unsigned char));
    *encrypted_sum_x2 = (unsigned char *) malloc(names_list.size() * 130 * sizeof(unsigned char));
    unsigned char *db_parser = *encrypted_db, *x2_sum_parser = *encrypted_sum_x2;
    db_reader.Open(db_rspecifier);
    // bool print_bool = true;	//TODO to delete
    // if(print_bool) cout << "generate_encrypted_db:x0=";	//TODO to delete
    for (; !db_reader.Done(); db_reader.Next()) {
    	const Vector<BaseFloat> &vector = db_reader.Value();
		Vector<double> vector_dbl(vector);
		int i, x, sum=0;
		for (i=0; i<200; i++) {
			std::ostringstream oss;
			x = (int)vector_dbl.Data()[i];
			oss << x;
			ec_elgamal.encrypt_ec(db_parser, (unsigned char *) oss.str().c_str());
			db_parser += 130;
			sum += x*x;
    		// if(print_bool) cout << x << ",";	//TODO to delete
		}
		// if(print_bool) cout << endl << "generate_encrypted_db:sum(x0i^2)=" << sum << endl;	//TODO to delete
		sum = 100; // TODO TO DELETE !!!!!!!!
		std::ostringstream oss;
		oss << sum;
		ec_elgamal.encrypt_ec(x2_sum_parser, (unsigned char *) oss.str().c_str());
		x2_sum_parser += 130;
    	// print_bool = false;	//TODO to delete
    }
	db_reader.Close();
    return names_list.size();
}
