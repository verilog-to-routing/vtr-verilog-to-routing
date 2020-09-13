#!/bin/bash

if [ -z ${VTR_TEST+x} ]; then
	echo "Missing $$VTR_TEST value"
	exit 1
fi

if [ -z ${VTR_TEST_OPTIONS+x} ]; then
	echo "Missing $$VTR_TEST_OPTIONS value"
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

echo "========================================"
echo "VPR Build Info"
echo "========================================"
./vpr/vpr --version

echo "========================================"
echo "Running Tests"
echo "========================================"
export VPR_NUM_WORKERS=1
./run_reg_test.py $VTR_TEST $VTR_TEST_OPTIONS -j$NUM_CORES
kill $MONITOR
