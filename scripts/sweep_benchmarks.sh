#!/bin/bash

for benchmark in $(ls -hSr /project/work/timing_analysis/skew/*/vpr_timing_graph.echo) 
do
    echo "==============================================================="
    benchmark_name=$(basename $(dirname $benchmark))
    echo $benchmark

    $1 $benchmark >& ${benchmark_name}.log

    exit_code=$?

    if [ $exit_code -ne 0 ]; then
        echo "Failed $benchmark"
        exit 1
    fi
done

echo "Passed All Benchmarks"
exit 0
