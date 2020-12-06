#!/bin/bash

set -e
. ./cmd.sh
. ./path.sh

generate_client_files $1 $2 $3 $4 $5 $6 $7 $8 || exit 1