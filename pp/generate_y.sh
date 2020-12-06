#!/bin/bash

set -e
. ./cmd.sh
. ./path.sh

wav_dir=$1
working_dir=$2
temp_dir=$working_dir/temp_y
nnet_dir=$3
mfcc_config=$4

nj=1  # has to be <= nbr of speakers
mfcc_dir=$temp_dir/mfcc
make_mfcc_dir=$temp_dir/make_mfcc
make_vad_dir=$temp_dir/make_vad
xvectors_dir=$temp_dir/generated_xvector
normalized_y_ark=$working_dir/normalized_y.ark
normalized_y_txt=$working_dir/normalized_y.txt

[ ! -d "$temp_dir" ] && mkdir $temp_dir

generate_wav.pl $wav_dir $temp_dir

steps/make_mfcc.sh --write-utt2num-frames false --mfcc-config $mfcc_config --nj $nj --cmd "$train_cmd" $temp_dir $make_mfcc_dir $mfcc_dir

utils/fix_data_dir.sh $temp_dir

sid/compute_vad_decision.sh --nj $nj --cmd "$train_cmd" $temp_dir $make_vad_dir $mfcc_dir

utils/fix_data_dir.sh $temp_dir

sid/nnet3/xvector/extract_xvectors.sh --nj $nj --cmd "$train_cmd" $nnet_dir $temp_dir $xvectors_dir

lda_transform_xvectors --normalize-length=true "ivector-copy-plda --smoothing=0.0 $nnet_dir/xvectors_train/plda - |" \
    "ark:ivector-subtract-global-mean $nnet_dir/xvectors_train/mean.vec scp:$xvectors_dir/xvector.scp ark:- | transform-vec $nnet_dir/xvectors_train/transform.mat ark:- ark:- | ivector-normalize-length ark:- ark:- |" \
    ark,t:$normalized_y_txt y

rm -r $temp_dir

# copy-vector ark:$normalized_y_ark ark,t:-