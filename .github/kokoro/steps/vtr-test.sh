#!/bin/bash

if [ -z $VTR_TEST ]; then
	echo "Missing $$VTR_TEST value"
	exit 1
fi

if [ -z $MAX_CORES ]; then
	echo "Missing $$MAX_CORES value"
	exit 1
fi

if [ -z $NUM_CORES ]; then
	echo "Missing $$NUM_CORES value"
	exit 1
fi

echo $PWD
pwd
pwd -L
pwd -P
cd $(realpath $(pwd))
echo $PWD
pwd
pwd -L
pwd -P

(
    while :
    do
        date
        uptime
        free -h
        sleep 300
    done
) &
MONITOR=$!

export VPR_NUM_WORKERS=1
./run_reg_test.pl $VTR_TEST -show_failures -j$NUM_CORES
kill $MONITOR
