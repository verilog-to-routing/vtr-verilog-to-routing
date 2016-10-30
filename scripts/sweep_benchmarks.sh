#!/bin/bash

for benchmark in $(ls -hSr /project/work/timing_analysis/skew/*/vpr_timing_graph.echo) 
do
    echo "==============================================================="
    echo $benchmark

    $1 $benchmark

    exit_code=$?

    if [ $exit_code -ne 0 ]; then
        echo "Failed"
        exit 1
    fi
done

echo "Passed"
exit 0
