#!/bin/bash

set -e
. ./cmd.sh
. ./path.sh

nnet_dir="xvector_nnet_1a"

min_chunk_size=25
max_chunk_size=10000
chunk_size=$max_chunk_size
cache_capacity=64

[ ! -f "$nnet_dir/new_final.raw" ] && nnet3-copy --nnet-config=$nnet_dir/extract.config $nnet_dir/final.raw $nnet_dir/new_final.raw
[ ! -f "$nnet_dir/xvectors_train/new_plda" ] && ivector-copy-plda --smoothing=0.0 $nnet_dir/xvectors_train/plda $nnet_dir/xvectors_train/new_plda

feats=$(echo "$1 $2" | compute-mfcc-feats --verbose=2 --sample-frequency=16000 --frame-length=25 --low-freq=20 --high-freq=7600 --num-mel-bins=30 --num-ceps=30 --snip-edges=false scp,p:- ark,t:- 2> /dev/null)
vad=$(echo "$feats" | compute-vad --vad-energy-threshold=5.5 --vad-energy-mean-scale=0.5 --vad-proportion-threshold=0.12 --vad-frames-context=2 ark,t:- ark,t:- 2> /dev/null)
cmvn_sliding=$(echo "$feats" | apply-cmvn-sliding --norm-vars=false --center=true --cmn-window=300 ark,t:- ark,t:- 2> /dev/null)
voiced_frames=$({ echo "$cmvn_sliding" ; echo "$vad" ; } | select-voiced-frames ark,t:- ark,t:- ark,t:- 2> /dev/null)
xvector=$({ cat $nnet_dir/new_final.raw ; echo "$voiced_frames" ; } | nnet3-xvector-compute --use-gpu=no --min-chunk-size=$min_chunk_size --chunk-size=$chunk_size --cache-capacity=$cache_capacity - ark,t:- ark,t:- 2> /dev/null)
xvector_subtracted_global_mean=$(echo "$xvector" | ivector-subtract-global-mean $nnet_dir/xvectors_train/mean.vec ark,t:- ark,t:- 2> /dev/null)
xvector_transformed=$(echo "$xvector_subtracted_global_mean" | transform-vec $nnet_dir/xvectors_train/transform.mat ark,t:- ark,t:- 2> /dev/null)
xvector_normalized_length=$(echo "$xvector_transformed" | ivector-normalize-length ark,t:- ark,t:- 2> /dev/null)

quantized_xvector=$({ cat "$nnet_dir/xvectors_train/new_plda" ; echo "$xvector_normalized_length" ; } | lda_transform_xvectors --normalize-length=true - ark,t:- ark,t:- y NULL 2> /dev/null)

echo "$quantized_xvector"

# echo "$quantized_xvector" | copy-vector ark,t:- ark,t:normalized_y.txt