// privacy_code/plda_normalized.h
// TODO modified by Mahdi

// Copyright 2013    Daniel Povey
//           2015    David Snyder


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

#ifndef KALDI_PLDA_NORMALIZED_H_
#define KALDI_PLDA_NORMALIZED_H_

#include <vector>
#include <algorithm>
#include "base/kaldi-common.h"
#include "matrix/matrix-lib.h"
#include "gmm/model-common.h"
#include "gmm/diag-gmm.h"
#include "gmm/full-gmm.h"
#include "itf/options-itf.h"
#include "util/common-utils.h"

#include "ivector/plda.h"

namespace kaldi {

/* This code implements Probabilistic Linear Discriminant Analysis: see
   "Probabilistic Linear Discriminant Analysis" by Sergey Ioffe, ECCV 2006.
   At least, that was the inspiration.  The E-M is an efficient method
   that I derived myself (note: it could be made even more efficient but
   it doesn't seem to be necessary as it's already very fast).

   This implementation of PLDA only supports estimating with a between-class
   dimension equal to the feature dimension.  If you want a between-class
   covariance that has a lower dimension, you can just remove the smallest
   elements of the diagonalized between-class covariance matrix.  This is not
   100% exact (wouldn't give you as good likelihood as E-M estimation with that
   dimension) but it's close enough.  */

class Plda_normalized: public Plda {
 public:
  // Plda_normalized() { }

  // explicit Plda_normalized(const Plda_normalized &other):
  //   mean_(other.mean_),
  //   transform_(other.transform_),
  //   psi_(other.psi_),
  //   offset_(other.offset_) {
  // };

  /// Returns the log-likelihood ratio
  /// log (p(test_ivector | same) / p(test_ivector | different)).
  /// transformed_enroll_ivector is an average over utterances for
  /// that speaker.  Both transformed_enroll_vector and transformed_test_ivector
  /// are assumed to have been transformed by the function TransformIvector().
  /// Note: any length normalization will have been done while computing
  /// the transformed iVectors.
  // double LogLikelihoodRatio(const VectorBase<double> &transformed_enroll_ivector,
  //                           int32 num_enroll_utts,
  //                           const VectorBase<double> &transformed_test_ivector)
  //                           const;


  /// This function smooths the within-class covariance by adding to it,
  /// smoothing_factor (e.g. 0.1) times the between-class covariance (it's
  /// implemented by modifying transform_).  This is to compensate for
  /// situations where there were too few utterances per speaker get a good
  /// estimate of the within-class covariance, and where the leading elements of
  /// psi_ were as a result very large.
  // void SmoothWithinClassCovariance(double smoothing_factor);

  /// Apply a transform to the PLDA model.  This is mostly used for
  /// projecting the parameters of the model into a lower dimensional space,
  /// i.e. in_transform.NumRows() <= in_transform.NumCols(), typically for
  /// speaker diarization with a PCA transform.
  // void ApplyTransform(const Matrix<double> &in_transform);

  /// TODO added by Mahdi///
  /// returns the normalized mean of vector x
  double LogLikelihoodRatioGivenMeanVector(VectorBase<double> &mean,
                                            int32 n,
                                            VectorBase<double> &transformed_test_ivector_normalized);
  float GetMeanX(const VectorBase<float> &ivector,
                         int32 n, 
                         VectorBase<float> *normalized_mean) const;
  float NormalizeY(const VectorBase<float> &ivector,
                       VectorBase<float> *normalized_ivector) const;

  ////////////////////////////////////////////////////


  const double normalizing_adder = 10;
  const double normalizing_multiplier = 13.5;
  // int32 Dim() const { return mean_.Dim(); }
  // void Write(std::ostream &os, bool binary) const;
  // void Read(std::istream &is, bool binary);
 // protected:
 //  void ComputeDerivedVars(); // computes offset_.
 //  friend class PldaEstimator;
 //  friend class PldaUnsupervisedAdaptor;

 //  Vector<double> mean_;  // mean of samples in original space.
 //  Matrix<double> transform_; // of dimension Dim() by Dim();
 //                             // this transform makes within-class covar unit
 //                             // and diagonalizes the between-class covar.
 //  Vector<double> psi_; // of dimension Dim().  The between-class
 //                       // (diagonal) covariance elements, in decreasing order.

 //  Vector<double> offset_;  // derived variable: -1.0 * transform_ * mean_

 // private:

  /// This returns a normalization factor, which is a quantity we
  /// must multiply "transformed_ivector" by so that it has the length
  /// that it "should" have.  We assume "transformed_ivector" is an
  /// iVector in the transformed space (i.e., mean-subtracted, and
  /// multiplied by transform_).  The covariance it "should" have
  /// in this space is \Psi + I/num_examples.
  // double GetNormalizationFactor(const VectorBase<double> &transformed_ivector,
  //                               int32 num_examples) const;

};

}  // namespace kaldi

#endif
