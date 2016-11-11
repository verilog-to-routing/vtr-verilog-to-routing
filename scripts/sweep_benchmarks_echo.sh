#!/bin/bash

if [ "$1" == "" ]; then
    echo "Executable required for first argument"
    exit 1
fi
myexec=$(realpath $1)

WORK_DIR=sweep_run

mkdir -p $WORK_DIR

for benchmark in $(ls -hSr /project/work/timing_analysis/skew/*/vpr_timing_graph.echo)
do
    benchmark_name=$(basename $(dirname $benchmark))
    run_dir=${WORK_DIR}/${benchmark_name}

    echo "mkdir -p $run_dir && cd $run_dir && $myexec $benchmark >& ${benchmark_name}.log && mv timing_graph.echo ${benchmark_name}.echo && echo 'PASSED $benchmark_name' || echo 'FAILED $benchmark_name'"

done

exit 0
