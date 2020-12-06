// ivectorbin/compare_one_file_to_db.cc
// TODO edited by Mahdi

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
// #include "ivector/plda.h"


int main(int argc, char *argv[]) {
  using namespace kaldi;
  typedef kaldi::int32 int32;
  typedef std::string string;
  try {
    const char *usage =
        "Computes log-likelihood ratios for trials using PLDA model\n"
        "Note: the output will have the form\n"
        "<key1> <key2> [<dot-product>]\n"
        "(if either key could not be found, the dot-product field in the output\n"
        "will be absent, and this program will print a warning)\n"
        "\n"
        "Usage: compare_one_file_to_db <plda> <db-rspecifier> <one-file-rspecifier>\n"
        " <scores-wxfilename>\n"
        "\n"
        "e.g.: compare_one_file_to_db plda "
        "ark:db_vectors.ark ark:vector.ark scores\n";

    ParseOptions po(usage);

    std::string num_utts_rspecifier;

    PldaConfig plda_config;
    plda_config.Register(&po);
    po.Register("num-utts", &num_utts_rspecifier, "Table to read the number of "
                "utterances per speaker, e.g. ark:num_utts.ark\n");

    po.Read(argc, argv);

    if (po.NumArgs() != 4) {
      po.PrintUsage();
      exit(1);
    }

    std::string plda_rxfilename = po.GetArg(1),
        db_vector_rspecifier = po.GetArg(2),
        file_vector_rspecifier = po.GetArg(3),
        scores_wxfilename = po.GetArg(4);

    //  diagnostics:
    int64 num_train_errs = 0, num_trials_done = 0;

    Plda_normalized plda;
    ReadKaldiObject(plda_rxfilename, &plda);

    SequentialBaseFloatVectorReader db_vector_reader(db_vector_rspecifier);
    SequentialBaseFloatVectorReader file_vector_reader(file_vector_rspecifier);
    RandomAccessInt32Reader num_utts_reader(num_utts_rspecifier);

    std::string file_spk = file_vector_reader.Key();
    const Vector<BaseFloat> &file_vector = file_vector_reader.Value();
    Vector<double> file_vector_dbl(file_vector);

    bool binary = false;
    Output ko(scores_wxfilename, binary);

    double sum = 0.0, sumsq = 0.0;
    std::string line;

    unsigned int threshold = 36203300;

    for (; !db_vector_reader.Done(); db_vector_reader.Next()) {
      std::string db_spk = db_vector_reader.Key();
      const Vector<BaseFloat> &db_vector = db_vector_reader.Value();
      int32 num_examples;
      if (!num_utts_rspecifier.empty()) {
        if (!num_utts_reader.HasKey(db_spk)) {
          KALDI_WARN << "Number of utterances not given for speaker " << db_spk;
          num_train_errs++;
          continue;
        }
        num_examples = num_utts_reader.Value(db_spk);
      } else {
        num_examples = 1;
      }
      Vector<double> db_vector_dbl(db_vector);

      BaseFloat score = threshold-plda.LogLikelihoodRatioGivenMeanVector(db_vector_dbl, num_examples, file_vector_dbl);

      sum += score;
      sumsq += score * score;
      num_trials_done++;
      ko.Stream() << file_spk << ' ' << db_spk << ' ' << score << std::endl;
    }

    KALDI_LOG << "Processed " << num_trials_done << " trials, " << num_train_errs
              << " had errors.";
    return (num_trials_done != 0 ? 0 : 1);
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return -1;
  }
}
