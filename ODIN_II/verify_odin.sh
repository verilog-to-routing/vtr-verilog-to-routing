#!/bin/bash
#1
trap ctrl_c INT SIGINT SIGTERM
SHELL=/bin/bash

QUIT=0

INPUT=$@

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
			--test [test name]    * test name is one of ( arch, syntax, other, micro, regression, vtr_basic, vtr_strong or pre_commit )
			--generate_bench      * generate input and output vector for test
			--generate_output     * generate output vector for test given its input vector
			--clean               * clean temporary directory
			--nb_of_process [n]   * n = nb of process requested to be used
			--limit_ressource     * force higher nice value and set hard limit for hardware memory to force swap more ***not always respected by system
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
	last_run=$(find regression_test/run* -maxdepth 0 -type d 2>/dev/null | tail -1 )
	if [ "_${last_run}" != "_" ]
	then
		last_run_id=${last_run##regression_test/run}
		n=$(echo $last_run_id | awk '{print $0 + 1}')
		new_run=regression_test/run$(printf "%03d" $n)
	fi
	echo "running benchmark @${new_run}"
	mkdir -p ${new_run}
}

function cleanup_temp() {
	for runs in regression_test/run*
	do 
		rm -Rf ${runs}
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
		done
	fi
}

function exit_program() {
  
	fail_count=0

	for fail_log in ${new_run}/*failures.log
	do
		if [ -e ${fail_log} ]
		then
			failed_in_test=$(wc -l < ${fail_log})
			fail_count=$(( ${fail_count} + ${failed_in_test} ))
		fi
	done

	if [ $fail_count -gt "0" ]
	then
		echo "Failed $fail_count"
		echo "View Failure log in ${new_run}/failure.log"

	else
		echo "no run failure!"
	fi

	exit $fail_count
}

function ctrl_c() {
	if [ "_${QUIT}" == "_0" ]
	then
		QUIT=1
		echo "** REGRESSION TEST EXITED FORCEFULLY **"
		jobs -p | xargs kill &> /dev/null
		pkill odin_II &> /dev/null
		pkill ${EXEC} &> /dev/null
		#should be dead by now
		exit 120
	fi
}

function sim() {
	threads=$1
	bench_type=$2
	with_input_vector=$3
	with_output_vector=$4
	use_sim=$5
	use_arch_list=$6
	passing_args=$7

	benchmark_dir=regression_test/benchmark/${bench_type}

	echo " BENCHMARK IS: ${bench_type}"

	if [ "_${passing_args}" == "_1" ]; then

		global_odin_failure="${new_run}/odin_failures"

		for dir in ${benchmark_dir}/*
		do

			test_name=${dir##*/}
			TEST_FULL_REF="${bench_type}/${test_name}"

			DIR="${new_run}/${bench_type}/$test_name"
			blif_file="${DIR}/odin.blif"


			#build commands
			mkdir -p $DIR
			verilog_command="./wrapper_odin.sh --log_file ${DIR}/odin.log --test_name ${TEST_FULL_REF} --failure_log ${global_odin_failure}.log --time_limit ${TIME_LIMIT} ${USING_LOW_RESSOURCE}"
			echo "${verilog_command} ${DEFAULT_CMD_PARAM} $(cat ${dir}/odin.args | tr '\n' ' ') -o ${blif_file} -sim_dir ${DIR}/" > ${DIR}/sim_param

		done

		#run the custon command
		echo " ========= Synthesizing Circuits"
		find ${new_run}/${bench_type}/ -name cmd_param | xargs -n1 -P$1 -I test_cmd ${SHELL} -c '$(cat test_cmd)'
		mv_failed ${global_odin_failure}

	else

		global_synthesis_failure="${new_run}/synthesis_failures"
		global_simulation_failure="${new_run}/simulation_failures"

		#include a no arch directive 
		MY_ARCH="no_arch"

		case $use_arch_list in
			memories)
				MY_ARCH="${MEM_ARCH}"
				;;

			small_sweep)
				MY_ARCH="${SMALL_ARCH_SWEEP}"
				;;

			full_sweep)
				MY_ARCH="${FULL_ARCH_SWEEP}"
				;;

			default)
				MY_ARCH="${DEFAULT_ARCH}"
				;;

			*);;
			esac



		for benchmark in ${benchmark_dir}/*.v
		do
			basename=${benchmark%.v}
			test_name=${basename##*/}

			for arches in ${MY_ARCH}
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
				verilog_file=${benchmark}
				blif_file="${DIR}/odin.blif"

				#build commands
				mkdir -p $DIR

				verilog_command="./wrapper_odin.sh --log_file ${DIR}/synthesis.log --test_name ${TEST_FULL_REF} --failure_log ${global_synthesis_failure}.log --time_limit ${TIME_LIMIT} ${USING_LOW_RESSOURCE}"
				verilog_command="${verilog_command} ${DEFAULT_CMD_PARAM} ${arch_cmd} -V ${verilog_file} -o ${blif_file} -sim_dir ${DIR}/"
				echo "${verilog_command}" > ${DIR}/cmd_param

				if [ "_$use_sim" == "_1" ]
				then
					simulation_command="./wrapper_odin.sh --log_file ${DIR}/simulation.log --test_name ${TEST_FULL_REF} --failure_log ${global_simulation_failure}.log --time_limit ${TIME_LIMIT} ${USING_LOW_RESSOURCE}"
					simulation_command="${simulation_command} ${DEFAULT_CMD_PARAM} ${arch_cmd} -b ${blif_file} -sim_dir ${DIR}/"

					if [ "_$with_input_vector" == "_1" ] && [ "_$REGENERATE_BENCH" != "_1" ]; then
						simulation_command="${simulation_command} -t ${benchmark_dir}/${test_name}_input"
						
						if [ "_$with_output_vector" == "_1" ] && [ "_$REGENERATE_OUTPUT" != "_1" ]; then
							simulation_command="${simulation_command} -T ${benchmark_dir}/${test_name}_output"
						fi
					else
						simulation_command="${simulation_command} ${HOLD_PARAM}"

						if [ "_$REGENERATE_BENCH" != "_1" ]; then
							simulation_command="${simulation_command} ${GENERATE_VECTOR_COUNT}"
						else
							simulation_command="${simulation_command} ${REGEN_VECTOR_CMD}"
						fi
					fi
					echo "${simulation_command}" > ${DIR}/sim_param
				fi
			done
		done

		#synthesize the circuits
		echo " ========= Synthesizing Circuits"
		find ${new_run}/${bench_type}/ -name cmd_param | xargs -n1 -P$1 -I test_cmd ${SHELL} -c '$(cat test_cmd)'
		mv_failed ${global_synthesis_failure}

		if [ "_$use_sim" == "_1" ]
		then
			#run the simulation
			echo " ========= Simulating Circuits"
			find ${new_run}/${bench_type}/ -name sim_param | xargs -n1 -P$1 -I sim_cmd ${SHELL} -c '$(cat sim_cmd)'
			mv_failed ${global_simulation_failure}
		fi

	fi
	

}

function other_test() {
	threads=$NB_OF_PROC
	bench_type=other
	with_input_vector=0
	with_output_vector=0
	use_sim=1
	use_arch_list="-"
	with_input_args=1

	sim $threads $bench_type $with_input_vector $with_output_vector $use_sim $use_arch_list $with_input_args
}

function micro_test() {
	threads=$NB_OF_PROC
	bench_type=micro
	with_input_vector=1
	with_output_vector=1
	use_sim=1
	use_arch_list="small_sweep"
	with_input_args=0

	sim $threads $bench_type $with_input_vector $with_output_vector $use_sim $use_arch_list $with_input_args
}

function regression_test() {
	threads=1
	bench_type=full
	with_input_vector=1
	with_output_vector=1
	use_sim=1
	use_arch_list="memories"
	with_input_args=0

	sim $threads $bench_type $with_input_vector $with_output_vector $use_sim $use_arch_list $with_input_args
}

function arch_test() {
	threads=$NB_OF_PROC
	bench_type=arch
	with_input_vector=1
	with_output_vector=1
	use_sim=1
	use_arch_list="small_sweep"
	with_input_args=0

	sim $threads $bench_type $with_input_vector $with_output_vector $use_sim $use_arch_list $with_input_args
}

function syntax_test() {
	threads=$NB_OF_PROC
	bench_type=syntax
	with_input_vector=0
	with_output_vector=0
	use_sim=1
	use_arch_list="-"
	with_input_args=0

	sim $threads $bench_type $with_input_vector $with_output_vector $use_sim $use_arch_list $with_input_args
}

function functional_test() {
	threads=$NB_OF_PROC
	bench_type=functional
	with_input_vector=1
	with_output_vector=0
	use_sim=1
	use_arch_list="small_sweep"
	with_input_args=0

	sim $threads $bench_type $with_input_vector $with_output_vector $use_sim $use_arch_list $with_input_args
}

function operators_test() {
	threads=$NB_OF_PROC
	bench_type=operators
	with_input_vector=1
	with_output_vector=1
	use_sim=1
	use_arch_list="small_sweep"
	with_input_args=0

	sim $threads $bench_type $with_input_vector $with_output_vector $use_sim $use_arch_list $with_input_args
}

function parse_args() {
	while [[ "$#" > 0 ]]
	do 
		case $1 in
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

#########################################################
#	START HERE

START=`get_current_time`
parse_args $INPUT
echo "Benchmark is: $TEST_TYPE"

case $TEST_TYPE in

	"operators")
		init_temp
		operators_test
		;;

	"functional")
		init_temp
		functional_test
		;;

	"arch")
		init_temp
		arch_test
		;;

	"syntax")
		init_temp
		syntax_test
		;;

	"micro")
		init_temp
		micro_test
		;;

	"regression")
		init_temp
		regression_test
		;;

	"other")
		init_temp
		other_test
		;;

	"full_suite")
		init_temp
		arch_test
		syntax_test
		other_test
		micro_test
		regression_test
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
		init_temp
		arch_test
		syntax_test
		other_test
		micro_test
		regression_test
		cd ..
		/usr/bin/perl run_reg_test.pl -j $NB_OF_PROC vtr_reg_basic
		/usr/bin/perl run_reg_test.pl -j $NB_OF_PROC vtr_reg_strong
		cd ODIN_II
		;;

	*)
		echo "Unknown test passed: $TEST_TYPE"
		help 
		;;
esac

print_time_since $START

exit_program
### end here
