#!/bin/bash

set -e
. ./cmd.sh
. ./path.sh

#db_files=/home/mahdi/Downloads/kaldi-master/egs/voxceleb/biggest_files_count/db2
#db_metadata=db2_metadata
db_files=/home/mahdi/Downloads/kaldi-master/egs/voxceleb/biggest_files_count/one_file_spkr_db1_id10986
db_metadata=one_file_spkr_db1_id10986_metadata
# db_files=/home/mahdi/Downloads/kaldi-master/egs/voxceleb/biggest_files_count/one_file_db
# db_metadata=one_file_db_metadata
nj=1  # has to be <= nbr of speakers

nnet_dir=xvector_nnet_1a
mfcc_config=conf/mfcc.conf
mfcc_dir=$db_metadata/mfcc
make_mfcc_dir=$db_metadata/make_mfcc
make_vad_dir=$db_metadata/make_vad
xvectors_dir=$db_metadata/generated_xvectors
normalized_xmeans_ark=$db_metadata/normalized_xmeans.ark
normalized_xmeans_scp=$db_metadata/normalized_xmeans.scp

generate_wav.pl $db_files $db_metadata

steps/make_mfcc.sh --write-utt2num-frames false --mfcc-config $mfcc_config --nj $nj --cmd "$train_cmd" $db_metadata $make_mfcc_dir $mfcc_dir

utils/fix_data_dir.sh $db_metadata

sid/compute_vad_decision.sh --nj $nj --cmd "$train_cmd" $db_metadata $make_vad_dir $mfcc_dir

utils/fix_data_dir.sh $db_metadata

sid/nnet3/xvector/extract_xvectors.sh --nj $nj --cmd "$train_cmd" $nnet_dir $db_metadata $xvectors_dir

lda_transform_xvectors --normalize-length=true "ivector-copy-plda --smoothing=0.0 $nnet_dir/xvectors_train/plda - |" \
    "ark:ivector-subtract-global-mean $nnet_dir/xvectors_train/mean.vec scp:$xvectors_dir/xvector.scp ark:- | transform-vec $nnet_dir/xvectors_train/transform.mat ark:- ark:- | ivector-normalize-length ark:- ark:- |" \
    ark,scp:$normalized_xmeans_ark,$normalized_xmeans_scp

# copy-vector ark:$normalized_xmeans_ark ark,t:-