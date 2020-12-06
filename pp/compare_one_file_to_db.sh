#!/bin/bash

set -e
. ./cmd.sh
. ./path.sh

db_metadata=db_spkr_db1_id10018_metadata
one_file_metadata=one_file_spkr_db1_id10986_metadata
output_file=scores_id10986_id10018.txt
nnet_dir=xvector_nnet_1a

compare_one_file_to_db --normalize-length=true "ivector-copy-plda --smoothing=0.0 $nnet_dir/xvectors_train/plda - |" \
	scp:$db_metadata/normalized_xmeans.scp \
	scp:$one_file_metadata/normalized_xmeans.scp \
	$output_file || exit 1;