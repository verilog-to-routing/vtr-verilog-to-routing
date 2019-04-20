#!/bin/bash
#1
trap ctrl_c INT SIGINT SIGTERM
SHELL=/bin/bash

QUIT=0

INPUT=$@

BASE_DIR="regression_test/benchmark"
TEST_DIR_LIST=$(find regression_test/benchmark -mindepth 1 -maxdepth 1 -type d | cut -d '/' -f 3 | tr '\n' ' ')  

#include more generic names here for better vector generation
HOLD_LOW_RESET="-L reset rst"
HOLD_HIGH_WE="-H we"

HOLD_PARAM="${HOLD_LOW_RESET} ${HOLD_HIGH_WE}"

#if you want to pass in a new adder definition file
ADDER_DEFINITION="--adder_type default"

#if you want to change the default number of vectors to generate
GENERATE_VECTOR_COUNT="-g 10"

#if you want to change the default number of vectors to generate
REGEN_VECTOR_CMD="--best_coverage -g 100"

DEFAULT_ARCH="../libs/libarchfpga/arch/sample_arch.xml"

MEM_ARCH="../vtr_flow/arch/timing/k6_N10_mem32K_40nm.xml"

SMALL_ARCH_SWEEP="\
../vtr_flow/arch/timing/k6_N10_mem32K_40nm.xml \
../libs/libarchfpga/arch/sample_arch.xml \
../vtr_flow/arch/timing/k6_frac_N10_mem32K_40nm.xml"

FULL_ARCH_SWEEP=$(find ../vtr_flow/arch/timing -maxdepth 1 | grep xml)

DEFAULT_CMD_PARAM="${ADDER_DEFINITION}"

EXEC="./wrapper_odin.sh"

FAILURE=0

new_run=regression_test/run001

NB_OF_PROC=1
REGENERATE_OUTPUT=0
REGENERATE_BENCH=0
TEST_TYPE=""

TIME_LIMIT="1200s"

USING_LOW_RESSOURCE=""

function help() {

printf "
Called program with $INPUT

Usage: ./verify_odin 
			--test [test name]    * test name is one of ( ${TEST_DIR_LIST}full_suite vtr_basic vtr_strong pre_commit )
			--generate_bench      * generate input and output vector for test
			--generate_output     * generate output vector for test given its input vector
			--clean               * clean temporary directory
			--nb_of_process [n]   * n = nb of process requested to be used
			--limit_ressource     * force higher nice value and set hard limit for hardware memory to force swap more ***not always respected by system
"
}

function config_help() {
printf "
config.txt expects a single line of argument wich can be one of:
			--custom_args_file
			--arch_list	[list_name]*
			                *memories        use VTR k6_N10_mem32k architecture
			                *small_sweep     use a small set of timing architecture
			                *full_sweep  	 sweep the whole vtr directory *** WILL FAIL ***
			                *default         use the sample_arch.xml
			--simulate                       request simulation to be ran
			--no_threading                   do not use multithreading for this test ** useful if you have large test **
"
}

function get_current_time() {
	echo $(date +%s%3N)
}

#1 start #2 end
function print_time_since() {
	BEGIN=$1
	NOW=`get_current_time`
	TIME_TO_RUN=$(( ${NOW} - ${BEGIN} ))

	Mili=$(( ${TIME_TO_RUN} %1000 ))
	Sec=$(( ( ${TIME_TO_RUN} /1000 ) %60 ))
	Min=$(( ( ( ${TIME_TO_RUN} /1000 ) /60 ) %60 ))
	Hour=$(( ( ( ${TIME_TO_RUN} /1000 ) /60 ) /60 ))

	echo "ran test in: $Hour:$Min:$Sec.$Mili"
}

function init_temp() {
	RUN_NAME="run001"
	last_run=$(find regression_test/run* -maxdepth 0 -type d 2>/dev/null | tail -1 )
	if [ "_${last_run}" != "_" ]
	then
		last_run_id=${last_run##regression_test/run}
		n=$(echo $last_run_id | awk '{print $0 + 1}')
		RUN_NAME="run$(printf "%03d" $n)"
	fi
	new_run=regression_test/${RUN_NAME}
	echo "running benchmark @${new_run}"
	mkdir -p ${new_run}
	unlink regression_test/latest 
	ln -s ${RUN_NAME} regression_test/latest
}

function cleanup_temp() {
	for runs in regression_test/run*
	do 
		rm -Rf ${runs}
		unlink regression_test/latest || /bin/true
	done
}

function mv_failed() {
	failed_dir=$1
	log_file="${failed_dir}.log"

	if [ -e ${log_file} ]
	then
		echo "Failed benchmark have been move to ${failed_dir}"
		for failed_benchmark in $(cat ${log_file})
		do
			parent_dir=$(dirname ${failed_dir}/${failed_benchmark})
			mkdir -p ${parent_dir}
			mv ${new_run}/${failed_benchmark} ${parent_dir}
			FAILURE=$(( ${FAILURE} + 1 ))
		done
		cat ${log_file} >> ${new_run}/test_failures.log
		rm -f ${log_file}
	fi
}

function exit_program() {

	if [ "_${FAILURE}" != "_0" ]
	then
		echo "Failed ${FAILURE} benchmarks"
		echo "View Failure log in ${new_run}/test_failures.log"

	else
		echo "no run failure!"
	fi

	exit ${FAILURE}
}

function ctrl_c() {
	QUIT=1
	while [ "${QUIT}" != "0" ]
	do
		echo "** REGRESSION TEST EXITED FORCEFULLY **"
		jobs -p | xargs kill &> /dev/null
		pkill odin_II &> /dev/null
		pkill ${EXEC} &> /dev/null
		#should be dead by now
		exit 120
	done
}

function sim() {

	####################################
	# parse the function commands passed
	with_custom_args="0"
	arch_list="no_arch"
	with_sim="0"
	threads=${NB_OF_PROC}

	if [ ! -e "$1" ]; then
		echo "invalid benchmark directory passed to simulation function $1"
		ctrl_c
	fi
	benchmark_dir="$1"
	shift

	while [[ "$#" > 0 ]]
	do 
		case $1 in
			--custom_args_file) 
				with_custom_args=1
				;;

			--arch_list)
				case $2 in
					memories)
						arch_list="${MEM_ARCH}"
						;;

					small_sweep)
						arch_list="${SMALL_ARCH_SWEEP}"
						;;

					full_sweep)
						arch_list="${FULL_ARCH_SWEEP}"
						;;

					default)
						arch_list="${DEFAULT_ARCH}"
						;;
					*)
						;;
				esac
				shift
				;;

			--simulate)
				with_sim=1
				;;

			--no_threading)
				echo "This test will not be multithreaded"
				threads="1"
				;;

			*)
				echo "Unknown internal parameter passed: $1"
				config_help 
				ctrl_c
				;;
		esac
		shift
	done

	###########################################
	# run custom benchmark
	bench_type=${benchmark_dir##*/}
	echo " BENCHMARK IS: ${bench_type}"

	if [ "_${with_custom_args}" == "_1" ]; then

		global_odin_failure="${new_run}/odin_failures"

		for dir in ${benchmark_dir}/*
		do
			if [ -e ${dir}/odin.args ]; then
				test_name=${dir##*/}
				TEST_FULL_REF="${bench_type}/${test_name}"

				DIR="${new_run}/${bench_type}/$test_name"
				blif_file="${DIR}/odin.blif"


				#build commands
				mkdir -p $DIR
				wrapper_odin_command="./wrapper_odin.sh
											--log_file ${DIR}/odin.log
											--test_name ${TEST_FULL_REF}
											--failure_log ${global_odin_failure}.log
											--time_limit ${TIME_LIMIT}
											${USING_LOW_RESSOURCE}
											${RUN_WITH_VALGRIND}"

				odin_command="${DEFAULT_CMD_PARAM}
								$(cat ${dir}/odin.args | tr '\n' ' ') 
								-o ${blif_file} 
								-sim_dir ${DIR}"

				echo $(echo "${wrapper_odin_command} ${odin_command}" | tr '\n' ' ' | tr -s ' ' ) > ${DIR}/odin_param
			fi
		done

		#run the custon command
		echo " ========= Synthesizing Circuits"
		find ${new_run}/${bench_type}/ -name odin_param | xargs -n1 -P$threads -I test_cmd ${SHELL} -c '$(cat test_cmd)'
		mv_failed ${global_odin_failure}

	############################################
	# run benchmarks
	else

		global_synthesis_failure="${new_run}/synthesis_failures"
		global_simulation_failure="${new_run}/simulation_failures"

		for benchmark in ${benchmark_dir}/*.v
		do
			basename=${benchmark%.v}
			test_name=${basename##*/}

			input_vector_file="${basename}_input"
			output_vector_file="${basename}_output"

			for arches in ${arch_list}
			do

				arch_cmd=""
				if [ -e ${arches} ]
				then
					arch_cmd="-a ${arches}"
				fi

				arch_basename=${arches%.xml}
				arch_name=${arch_basename##*/}

				TEST_FULL_REF="${bench_type}/${test_name}/${arch_name}"

				DIR="${new_run}/${TEST_FULL_REF}"
				blif_file="${DIR}/odin.blif"


				#build commands
				mkdir -p $DIR

				wrapper_synthesis_command="./wrapper_odin.sh
											--log_file ${DIR}/synthesis.log
											--test_name ${TEST_FULL_REF}
											--failure_log ${global_synthesis_failure}.log
											--time_limit ${TIME_LIMIT}
											${USING_LOW_RESSOURCE}
											${RUN_WITH_VALGRIND}"

				synthesis_command="${DEFAULT_CMD_PARAM}
									${arch_cmd}
									-V ${benchmark}
									-o ${blif_file}
									-sim_dir ${DIR}"

				echo $(echo "${wrapper_synthesis_command} ${synthesis_command}"  | tr '\n' ' ' | tr -s ' ') > ${DIR}/cmd_param

				if [ "_$with_sim" == "_1" ] || [ -e ${input_vector_file} ]
				then
					#force trigger simulation if input file exist
					with_sim="1"

					wrapper_simulation_command="./wrapper_odin.sh
											--log_file ${DIR}/simulation.log
											--test_name ${TEST_FULL_REF}
											--failure_log ${global_simulation_failure}.log
											--time_limit ${TIME_LIMIT}
											${USING_LOW_RESSOURCE}
											${RUN_WITH_VALGRIND}"

					simulation_command="${DEFAULT_CMD_PARAM}
											${arch_cmd}
											-b ${blif_file}
											-sim_dir ${DIR}
											${HOLD_PARAM}"

					if [ "_$REGENERATE_BENCH" == "_1" ]; then
						simulation_command="${simulation_command} ${REGEN_VECTOR_CMD}"

					elif [ -e ${input_vector_file} ]; then
						simulation_command="${simulation_command} -t ${input_vector_file}"

						if [ "_$REGENERATE_OUTPUT" != "_1" ] && [ -e ${output_vector_file} ]; then
							simulation_command="${simulation_command} -T ${output_vector_file}"
						fi
						
					else
						simulation_command="${simulation_command} ${GENERATE_VECTOR_COUNT}"

					fi

					echo $(echo "${wrapper_simulation_command} ${simulation_command}" | tr '\n' ' ' | tr -s ' ') > ${DIR}/sim_param
				fi
			done
		done

		#synthesize the circuits
		echo " ========= Synthesizing Circuits"
		find ${new_run}/${bench_type}/ -name cmd_param | xargs -n1 -P$threads -I test_cmd ${SHELL} -c '$(cat test_cmd)'
		mv_failed ${global_synthesis_failure}

		if [ "_$with_sim" == "_1" ]
		then
			#run the simulation
			echo " ========= Simulating Circuits"
			find ${new_run}/${bench_type}/ -name sim_param | xargs -n1 -P$threads -I sim_cmd ${SHELL} -c '$(cat sim_cmd)'
			mv_failed ${global_simulation_failure}
		fi

	fi
	

}

function parse_args() {
	while [[ "$#" > 0 ]]
	do 
		case $1 in
			-h|--help)
				help
				exit_program
				;;

			--generate_output) 
				if [ $REGENERATE_BENCH != "0" ] || [ $REGENERATE_OUTPUT != "0" ]
				then
					echo "can only specify one of --generate_output or --generate_bench"
					help
					ctrl_c
				fi
				REGENERATE_OUTPUT=1
				echo "regenerating output vector for test given predefined input"
				;;

			--generate_bench) 
				if [ $REGENERATE_BENCH != "0" ] || [ $REGENERATE_OUTPUT != "0" ]
				then
					echo "can only specify one of --generate_output or --generate_bench"
					help
					ctrl_c
				fi
				REGENERATE_BENCH=1
				echo "regenerating input and output vector for test"
				;;

			--limit_ressource) 
				USING_LOW_RESSOURCE="--limit_ressource"
				echo "limiting ressources for benchmark, this can help with small hardware"
				;;

			--clean)
				cleanup_temp
				;;

			--nb_of_process)
				if [ $NB_OF_PROC != "1" ]
				then
					echo "can only specify the number of processes once"
					help
					ctrl_c
				fi

				if [[ "$2" -gt "0" ]]
				then
					NB_OF_PROC=$2
					echo "Running benchmark on $NB_OF_PROC processes"
				fi
				shift
				;;
			--valgrind)
				RUN_WITH_VALGRIND="--valgrind"
				;;

			--test)
				if [ "_$TEST_TYPE" != "_" ]
				then
					echo "can only specify one test for this script"
					help
					ctrl_c
				fi

				TEST_TYPE=$2
				shift
				;;

			*) 
				echo "Unknown parameter passed: $1"
				help 
				ctrl_c
				;;
		esac 
		shift 
	done
}

RUN_LIST=(
"operators"
"arch"
"other"
"micro"
"syntax"
"full"
"large"
)

function run_all() {
	for test_dir in ${RUN_LIST[@]}; do
		sim ${BASE_DIR}/${test_dir} $(cat ${BASE_DIR}/${test_dir}/config.txt)
	done
}
#########################################################
#	START HERE

START=`get_current_time`

parse_args $INPUT

if [ "_${TEST_TYPE}" == "_" ]; then
	echo "No input test!"
	help
	print_time_since $START
	exit_program
fi


echo "Benchmark is: $TEST_TYPE"

init_temp
case $TEST_TYPE in

	"full_suite")
		run_all
		;;

	"vtr_basic")
		cd ..
		/usr/bin/perl run_reg_test.pl -j $NB_OF_PROC vtr_reg_basic
		cd ODIN_II
		;;

	"vtr_strong")
		cd ..
		/usr/bin/perl run_reg_test.pl -j $NB_OF_PROC vtr_reg_strong
		cd ODIN_II
		;;

	"pre_commit")
		run_all
		cd ..
		/usr/bin/perl run_reg_test.pl -j $NB_OF_PROC vtr_reg_basic
		/usr/bin/perl run_reg_test.pl -j $NB_OF_PROC vtr_reg_strong
		cd ODIN_II
		;;

	*)
		if [ ! -e ${BASE_DIR}/${TEST_TYPE} ]; then
			echo "${BASE_DIR}/${TEST_TYPE} Not Found! exiting."
			config_help
			ctrl_c
		fi

		if [ ! -e ${BASE_DIR}/${TEST_TYPE}/config.txt ]; then
			echo "no config file found in the directory."
			echo "please make sure a config.txt exist"
			config_help
			ctrl_c
		fi

		sim ${BASE_DIR}/${TEST_TYPE} $(cat ${BASE_DIR}/${TEST_TYPE}/config.txt)
		;;
esac

print_time_since $START

exit_program
### end here
