// privacy_code/lda_transform_xvectors.cc
// Added by Mahdi

// Copyright 2013  Daniel Povey

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.


#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "privacy_code/plda_normalized.h"


int main(int argc, char *argv[]) {
  using namespace kaldi;
  typedef kaldi::int32 int32;
  typedef std::string string;
  try {
    const char *usage =
        "Generates transformed and quantized xvectors given a plda model. If the input is x, the quantized\n"
        "output will be mean_x, of y, the output is the quantized y\n"
        "\n"
        "Usage: lda_transform_xvectors <plda> <train-ivector-rspecifier> <transformed-wspecifier> <x or y> <n_list_file>\n";

    ParseOptions po(usage);

    std::string num_utts_rspecifier;

    PldaConfig plda_config;
    plda_config.Register(&po);
    po.Register("num-utts", &num_utts_rspecifier, "Table to read the number of "
                "utterances per speaker, e.g. ark:num_utts.ark\n");

    po.Read(argc, argv);

    if (po.NumArgs() != 5) {
      po.PrintUsage();
      exit(1);
    }

    std::string plda_rxfilename = po.GetArg(1),
        train_ivector_rspecifier = po.GetArg(2),
        transformed_wspecifier = po.GetArg(3),
        x_or_y = po.GetArg(4),
        n_list_file = po.GetArg(5);

    //TODO mahdi_added////////////
    // std::cout << "ivector-plda-scoring.cc: plda_rxfilename="<<plda_rxfilename<<" test_ivector_rspecifier="<< test_ivector_rspecifier<<" test_ivector_rspecifier"<<test_ivector_rspecifier<<" trials_rxfilename="<<trials_rxfilename
    //   <<" scores_wxfilename="<<scores_wxfilename<<std::endl;
    //////////////////////////////

    double num_train_errs = 0, tot_train_renorm_scale = 0.0;
    int64 num_train_ivectors = 0;

    Plda_normalized plda;
    ReadKaldiObject(plda_rxfilename, &plda);

    int32 dim = plda.Dim();

    //TODO mahdi_added////////////
    // std::cout << "ivector-plda-scoring.cc: dim=plda.Dim()="<<dim<<std::endl;
    //////////////////////////////

    SequentialBaseFloatVectorReader train_ivector_reader(train_ivector_rspecifier);
    BaseFloatVectorWriter vec_writer(transformed_wspecifier);
    RandomAccessInt32Reader num_utts_reader(num_utts_rspecifier);

    // typedef unordered_map<string, Vector<BaseFloat>*, StringHasher> HashType;

    // These hashes will contain the iVectors in the PLDA subspace
    // (that makes the within-class variance unit and diagonalizes the
    // between-class covariance).  They will also possibly be length-normalized,
    // depending on the config.
    // HashType train_ivectors;
 
    //TODO mahdi_added////////////
    // std::cout << "ivector-plda-scoring.cc: Reading train iVectors"<<std::endl;
    // float min_value = 10^10, max_value = -10^10;

    FILE *n_list_file_p;
    if (x_or_y.compare("x") == 0)
      n_list_file_p = fopen(n_list_file.c_str(), "w");
    //////////////////////////////

    KALDI_LOG << "Reading train iVectors";
    for (; !train_ivector_reader.Done(); train_ivector_reader.Next()) {
      std::string spk = train_ivector_reader.Key();
      const Vector<BaseFloat> &ivector = train_ivector_reader.Value();
      int32 num_examples;
      if (!num_utts_rspecifier.empty()) {
        if (!num_utts_reader.HasKey(spk)) {
          KALDI_WARN << "Number of utterances not given for speaker " << spk;
          num_train_errs++;
          continue;
        }
        num_examples = num_utts_reader.Value(spk);
      } else {
        num_examples = 1;
      }
      Vector<BaseFloat> *transformed_ivector = new Vector<BaseFloat>(dim);

      tot_train_renorm_scale += plda.TransformIvector(plda_config, ivector, num_examples, transformed_ivector);

      // Vector<BaseFloat> *normalized_vector = new Vector<BaseFloat>(dim);
      // if (x_or_y.compare("x") == 0)
      //   plda.GetMeanX(*transformed_ivector, num_examples, normalized_vector);
      // else if (x_or_y.compare("y") == 0)
      //   plda.NormalizeY(*transformed_ivector, normalized_vector);
      // if(transformed_ivector->Min() < min_value) min_value = transformed_ivector->Min();
      // if(transformed_ivector->Max() > max_value) max_value = transformed_ivector->Max();

      // vec_writer.Write(spk, *normalized_vector);
      vec_writer.Write(spk, *transformed_ivector);

      if (x_or_y.compare("x") == 0)
        fprintf(n_list_file_p, "%d\n", num_examples);

      num_train_ivectors++;
    }

    if (x_or_y.compare("x") == 0)
      fclose(n_list_file_p);

    KALDI_LOG << "Read " << num_train_ivectors << " training iVectors, "
              << "errors on " << num_train_errs;
    if (num_train_ivectors == 0)
      KALDI_ERR << "No training iVectors present.";
    KALDI_LOG << "Average renormalization scale on training iVectors was "
              << (tot_train_renorm_scale / num_train_ivectors);

    return (num_train_ivectors != 0 ? 0 : 1);
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return -1;
  }
}
