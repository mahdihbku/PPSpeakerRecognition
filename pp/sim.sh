#!/bin/bash

set -e
. ./cmd.sh
. ./path.sh

db_metadata=db_spkr_db1_id10986_metadata
one_file_metadata=one_file_spkr_db1_id10986_metadata
output_file=encrypted_scores.bin
nnet_dir=xvector_nnet_1a

sim scp:$db_metadata/normalized_xmeans.scp scp:$one_file_metadata/normalized_xmeans.scp $output_file