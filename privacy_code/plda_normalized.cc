// privacy_code/plda_normalized.cc
// TODO modified by Mahdi

// Copyright 2013     Daniel Povey
//           2015     David Snyder

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

#include <vector>
#include "plda_normalized.h"

namespace kaldi {

float Plda_normalized::GetMeanX(const VectorBase<float> &ivector,
                       int32 n,
                       VectorBase<float> *normalized_mean) const {
  int32 dim = Dim();
  Vector<double> psi_normalized(psi_);
  for (int32 i=0; i<dim; i++)
    psi_normalized(i) = floorf(psi_normalized(i) * pow(2.0,normalizing_multiplier));
  Vector<double> mean(dim, kUndefined);
  for (int32 i = 0; i < dim; i++) {
    mean(i) = n * psi_normalized(i) / (n * psi_normalized(i) + 1.0) * ivector(i);
    // mean(i) = floorf((mean(i) + normalizing_adder) * normalizing_multiplier);
      mean(i) = floorf((ivector(i) + normalizing_adder) * normalizing_multiplier);  // TODO this is not computing mean. reutrns x !!!
  }
  normalized_mean->CopyFromVec(mean);

  // std::cout<<"plda_normalized: ivector="<<ivector<<std::endl;
  // std::cout<<"plda_normalized: normalized_mean="<<*normalized_mean<<std::endl;

  return 0;
}

float Plda_normalized::NormalizeY(const VectorBase<float> &ivector,
                       VectorBase<float> *normalized_ivector) const {
  int32 dim = Dim();
  Vector<double> transformed_test_ivector_normalized(ivector);
  for (int32 i=0; i<dim; i++) {
    transformed_test_ivector_normalized(i) = floorf((transformed_test_ivector_normalized(i) + normalizing_adder) * normalizing_multiplier);
    if (transformed_test_ivector_normalized(i) < 0) transformed_test_ivector_normalized(i) = 0;
    if (transformed_test_ivector_normalized(i) > 255) transformed_test_ivector_normalized(i) = 255;
  }
  normalized_ivector->CopyFromVec(transformed_test_ivector_normalized);
  return 0;
}

double Plda_normalized::LogLikelihoodRatioGivenMeanVector(
    VectorBase<double> &mean,  // normalized mean of x
    int32 n, // number of training utterances.
    VectorBase<double> &transformed_test_ivector_normalized) { // normalized y
  int32 dim = Dim();
  // no need to normalize transformed_train_ivector. we only normalize the mean later on
  Vector<double> psi_normalized(psi_);
  for (int32 i=0; i<dim; i++)
    psi_normalized(i) = floorf(psi_normalized(i) * pow(2.0,normalizing_multiplier));
  double loglike_given_class, loglike_without_class;
  { // work out loglike_given_class.
    // "mean" will be the mean of the distribution if it comes from the
    // training example.  The mean is \frac{n \Psi}{n \Psi + I} \bar{u}^g
    // "variance" will be the variance of that distribution, equal to
    // I + \frac{\Psi}{n\Psi + I}.
    Vector<double> variance(dim, kUndefined);
    for (int32 i = 0; i < dim; i++)
      variance(i) = 1.0 + psi_normalized(i) / (n * psi_normalized(i) + 1.0);
    double logdet = variance.SumLog();
    logdet = floorf(logdet * pow(2.0,normalizing_multiplier));
    //Vector<double> sqdiff(transformed_test_ivector);
    Vector<double> sqdiff(transformed_test_ivector_normalized);
    sqdiff.AddVec(-1.0, mean);
    sqdiff.ApplyPow(2.0);
    // variance.InvertElements();
    double M_LOG_2PI_normalized = floorf(M_LOG_2PI * pow(2.0,normalizing_multiplier));
    loglike_given_class = -(logdet * 2 + M_LOG_2PI_normalized * dim  * 2+   // *2 comes from removing variance
                                  sqdiff.Sum());
                                  //VecVec(sqdiff, variance)); // the variance is approximated as (1/2,1/2,...)
    // std::cout<<"Plda::LogLikelihoodRatio: psi_normalized="<<psi_normalized<<"mean="<<mean<<" logdet="<<logdet<<" variance="<<variance<<std::endl;
  }
  { // work out loglike_without_class.  Here the mean is zero and the variance
    // is I + \Psi.
    //Vector<double> sqdiff(transformed_test_ivector); // there is no offset.
    Vector<double> sqdiff(transformed_test_ivector_normalized); // there is no offset.
    sqdiff.ApplyPow(2.0);
    Vector<double> variance(psi_normalized);
    variance.Add(1.0); // I + \Psi.
    double logdet = variance.SumLog();
    logdet = floorf(logdet * pow(2.0,normalizing_multiplier));
    variance.InvertElements();
    double M_LOG_2PI_normalized = floorf(M_LOG_2PI * pow(2.0,normalizing_multiplier));
    loglike_without_class = -(logdet + M_LOG_2PI_normalized * dim +
                                    floorf(VecVec(sqdiff, variance)));
    // std::cout<<"Plda::LogLikelihoodRatio: M_LOG_2PI_normalized="<<M_LOG_2PI_normalized<<" logdet="<<logdet<<" floorf(VecVec(sqdiff, variance))="<<floorf(VecVec(sqdiff, variance))<<std::endl;
    // std::cout<<"Plda::LogLikelihoodRatio: sqdiff="<<sqdiff<<" floorf(VecVec(sqdiff, variance))="<<floorf(VecVec(sqdiff, variance))<<std::endl;
  }
  double loglike_ratio = loglike_given_class - loglike_without_class * 2; // *2 comes from removing variance
  return loglike_ratio;
}


} // namespace kaldi
