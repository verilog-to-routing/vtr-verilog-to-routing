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

set +e
./run_reg_test.py $VTR_TEST $VTR_TEST_OPTIONS -j$NUM_CORES
TEST_RESULT=$?
set -e
kill $MONITOR

echo "========================================"
echo "Cleaning benchmarks files"
echo "========================================"
# Removing Symbiflow archs and benchmarks
find vtr_flow/arch/symbiflow/ -type f -not -name 'README.*' -delete
find vtr_flow/benchmarks/symbiflow/ -type f -not -name 'README.*' -delete

# Removing ISPD benchmarks
find vtr_flow/benchmarks/ispd_blif/ -type f -not -name 'README.*' -delete

# Removing Titan benchmarks
find vtr_flow/benchmarks/titan_blif/ -type f -not -name 'README.*' -delete

# Removing ISPD, Titan and Symbiflow tarballs
find . -type f -regex ".*\.tar\.\(gz\|xz\)" -delete

#Gzip output files from vtr_reg_nightly tests to lower working directory disk space
find vtr_flow/tasks/regression_tests/vtr_reg_nightly_test1/ -type f -print0 | xargs -0 -P $(nproc) gzip
find vtr_flow/tasks/regression_tests/vtr_reg_nightly_test2/ -type f -print0 | xargs -0 -P $(nproc) gzip
find vtr_flow/tasks/regression_tests/vtr_reg_nightly_test3/ -type f -print0 | xargs -0 -P $(nproc) gzip
find vtr_flow/tasks/regression_tests/vtr_reg_nightly_test4/ -type f -print0 | xargs -0 -P $(nproc) gzip

# Make sure working directory doesn't exceed disk space limit!
echo "Working directory size: $(du -sh)"
if [[ $(du -s | cut -d $'\t' -f 1) -gt $(expr 1024 \* 1024 \* 90) ]]; then
    echo "Working directory too large!"
    exit 1
fi

exit $TEST_RESULT
