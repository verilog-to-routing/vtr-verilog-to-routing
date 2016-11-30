#!/bin/bash

if [ "$1" == "" ]; then
    echo "Executable required for first argument"
    exit 1
fi
myexec=$(realpath $1)

WORK_DIR=sweep_run

mkdir -p $WORK_DIR

for benchmark in $(ls -hSr /project/work/timing_analysis/tatum_echo_split_arr_req/*.echo)
do
    benchmark_name=$(basename $benchmark | sed 's/.echo//')
    run_dir=${WORK_DIR}/${benchmark_name}

    #No echo moving
    #echo "mkdir -p $run_dir && cd $run_dir && $myexec $benchmark >& ${benchmark_name}.log && echo 'PASSED $benchmark_name' || echo 'FAILED $benchmark_name'"

    #With echo moving
    echo "mkdir -p $run_dir && cd $run_dir && $myexec $benchmark >& ${benchmark_name}.log && if [ -f timing_graph.echo ]; then mv timing_graph.echo ${benchmark_name}.echo; fi && echo 'PASSED $benchmark_name' || echo 'FAILED $benchmark_name'"

done

exit 0
