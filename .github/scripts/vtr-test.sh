#!/bin/bash

set +x
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

set +e

echo "========================================"
echo "VPR Build Info"
echo "========================================"
./vpr/vpr --version

echo "========================================"
echo "Running Tests"
echo "========================================"
export VPR_NUM_WORKERS=1

./run_reg_test.py $VTR_TEST $VTR_TEST_OPTIONS -j$NUM_CORES
TEST_RESULT=$?
set -e
kill $MONITOR

echo "========================================"
echo "Packing benchmarks files"
echo "========================================"

results=qor_results_$VTR_TEST.tar
# Create archive with output files from the test
find vtr_flow -type f \( -name "*.out" -o -name "*.log" -o -name "*.txt" -o -name "*.csv" \) -exec tar -rf $results {} \;
gzip $results

exit $TEST_RESULT
