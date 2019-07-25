#!/bin/bash
#1
trap ctrl_c INT SIGINT SIGTERM
SHELL=/bin/bash
QUIT=0

##############################################
# grab the absolute Paths
THIS_SCRIPT=$(readlink -f $0)
THIS_SCRIPT_EXEC=$(basename ${THIS_SCRIPT})
THIS_SCRIPT_DIR=$(dirname ${THIS_SCRIPT})

ODIN_ROOT_DIR="$(readlink -f ${THIS_SCRIPT_DIR}/../..)"
ODIN_BENCHMARK_EXEC="${ODIN_ROOT_DIR}/verify_odin.sh"
RESULT_DIR="${ODIN_ROOT_DIR}/regression_test"


function ctrl_c() {
	trap '' INT SIGINT SIGTERM
	QUIT=1
	while [ "${QUIT}" != "0" ]
	do
		echo "** Benchmark TEST EXITED FORCEFULLY **"
		pkill -2 verify_odin.sh &> /dev/null
		#should be dead by now
		exit 120
	done
}

EXECUTION_COUNT="3"
VECTOR_COUNT="800"
TIMEOUT="43200" #12 hours
NUMBER_OF_THREAD="8"


DEFAULT_ARGS="\
    --simulation_count ${EXECUTION_COUNT} \
    --test heavy_suite \
    --perf \
    --generate_bench \
    --vectors ${VECTOR_COUNT} \
    --timeout ${TIMEOUT} \
    --best_coverage_off \
    --force_simulate \
"

#################################################
# START !


ulimit -s unlimited
ulimit -n 8096
ulimit -u $(( 8 * 1024 * 1024 ))
ulimit -l $(( 400 * 1024 ))

MY_DIR="${RESULT_DIR}/single_thread"
rm -Rf ${MY_DIR} && mkdir -p ${MY_DIR} &&
/bin/bash -c "${ODIN_BENCHMARK_EXEC} ${DEFAULT_ARGS} --output_dir ${MY_DIR}"

NUMBER_OF_THREAD="32"
MY_DIR="${RESULT_DIR}/multi_thread_${NUMBER_OF_THREAD}"
rm -Rf ${MY_DIR} && mkdir -p ${MY_DIR} &&
/bin/bash -c "${ODIN_BENCHMARK_EXEC} ${DEFAULT_ARGS} --output_dir ${MY_DIR} --sim_threads ${NUMBER_OF_THREAD}"

MY_DIR="${RESULT_DIR}/batch_thread_${NUMBER_OF_THREAD}"
rm -Rf ${MY_DIR} && mkdir -p ${MY_DIR} &&
/bin/bash -c "${ODIN_BENCHMARK_EXEC} ${DEFAULT_ARGS} --output_dir ${MY_DIR} --sim_threads ${NUMBER_OF_THREAD} --batch_sim"

NUMBER_OF_THREAD="16"
MY_DIR="${RESULT_DIR}/multi_thread_${NUMBER_OF_THREAD}"
rm -Rf ${MY_DIR} && mkdir -p ${MY_DIR} &&
/bin/bash -c "${ODIN_BENCHMARK_EXEC} ${DEFAULT_ARGS} --output_dir ${MY_DIR} --sim_threads ${NUMBER_OF_THREAD}"

MY_DIR="${RESULT_DIR}/batch_thread_${NUMBER_OF_THREAD}"
rm -Rf ${MY_DIR} && mkdir -p ${MY_DIR} &&
/bin/bash -c "${ODIN_BENCHMARK_EXEC} ${DEFAULT_ARGS} --output_dir ${MY_DIR} --sim_threads ${NUMBER_OF_THREAD} --batch_sim"

NUMBER_OF_THREAD="8"
MY_DIR="${RESULT_DIR}/multi_thread_${NUMBER_OF_THREAD}"
rm -Rf ${MY_DIR} && mkdir -p ${MY_DIR} &&
/bin/bash -c "${ODIN_BENCHMARK_EXEC} ${DEFAULT_ARGS} --output_dir ${MY_DIR} --sim_threads ${NUMBER_OF_THREAD}"

MY_DIR="${RESULT_DIR}/batch_thread_${NUMBER_OF_THREAD}"
rm -Rf ${MY_DIR} && mkdir -p ${MY_DIR} &&
/bin/bash -c "${ODIN_BENCHMARK_EXEC} ${DEFAULT_ARGS} --output_dir ${MY_DIR} --sim_threads ${NUMBER_OF_THREAD} --batch_sim"

