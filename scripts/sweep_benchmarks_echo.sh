#!/bin/bash

for benchmark in $(ls -hSr /project/work/timing_analysis/skew/*/vpr_timing_graph.echo) # | grep -v "gauss\|mes_noc") 
do
    benchmark_name=$(basename $(dirname $benchmark))

    echo "$1 $benchmark >& ${benchmark_name}.log && echo 'PASSED $benchmark_name' || echo 'FAILED $benchmark_name'"

done
