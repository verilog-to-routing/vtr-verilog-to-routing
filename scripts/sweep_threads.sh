#!/usr/bin/env bash

tg_file=$1
max_nthreads=$2


for nthreads in $(seq $max_nthreads)
do
    export OMP_NUM_THREADS=$nthreads
    echo "$OMP_NUM_THREADS thread(s)"
    ./sta $1 | grep -P 'AVG|Speed-Up'
    echo
done
