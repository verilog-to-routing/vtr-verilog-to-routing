#!/bin/bash
#1
trap ctrl_c INT
SHELL=/bin/bash

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

DEFAULT_ARCH="-a ../libs/libarchfpga/arch/sample_arch.xml"

DEFAULT_CMD_PARAM="${ADDER_DEFINITION}"

EXEC="./odin_II -U0 ${DEFAULT_CMD_PARAM}"

fail_count=0
new_run=regression_test/run001

NB_OF_PROC=1
REGENERATE_OUTPUT=0
REGENERATE_BENCH=0
TEST_TYPE=""

function help() {
printf "
Called program with $INPUT

Usage: ./verify_odin 
			--test [test name]    * test name is one of ( arch, syntax, other, micro, regression, vtr_basic, vtr_strong or pre_commit )
			--generate_bench      * generate input and output vector for test
			--generate_output     * generate output vector for test given its input vector
			--clean               * clean temporary directory
			--nb_of_process [n]   * n = nb of process requested to be used
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

function exit_program() {

    if [ -e ${new_run}/failure.log ]
    then
	    line_count=$(wc -l < ${new_run}/failure.log)
		fail_count=$(( ${fail_count} + ${line_count} ))
	fi

	if [ $fail_count -gt "0" ]
	then
		echo "Failed $fail_count"
		echo "View Failure log in ${new_run}/failure.log, "
	else
		echo "no run failure!"
	fi

	exit $fail_count
}

function ctrl_c() {

	echo ""
	echo "** EXITED FORCEFULLY **"
    fail_count=123456789
	exit_program
}

function sim() {
	threads=$1
	bench_type=$2
	with_input_vector=$3
	with_output_vector=$4
	with_blif=$5 #unused
	with_arch=$6
	passing_args=$7
	
	benchmark_dir=regression_test/benchmark/${bench_type}

	if [ "_${passing_args}" == "_1" ]; then
		for dir in ${benchmark_dir}/*
		do

			test_name=${dir##*/}
			DIR="${new_run}/${bench_type}/$test_name"
			blif_file="${DIR}/odin.blif"


			#build commands
			mkdir -p $DIR

			echo "${EXEC} $(cat ${dir}/odin.args | tr '\n' ' ') -o ${blif_file} -sim_dir ${DIR}/ &>> ${DIR}/log \
				&& echo --- PASSED == ${bench_type}/$test_name \
				|| (echo -X- FAILED == ${bench_type}/$test_name \
					&& echo ${bench_type}/$test_name >> ${new_run}/failure.log)" > ${DIR}/log

		done
	else
		for benchmark in ${benchmark_dir}/*.v
		do
			basename=${benchmark%.v}
			test_name=${basename##*/}
			TEST_FULL_REF="${bench_type}/${test_name}"

			DIR="${new_run}/${TEST_FULL_REF}"
			verilog_file="${benchmark_dir}/${test_name}.v"
			blif_file="${DIR}/odin.blif"

			#build commands
			mkdir -p $DIR


			verilog_command="${EXEC} -V ${verilog_file} -o ${blif_file}"

			[ "_$with_arch" == "_1" ] &&
				verilog_command="${verilog_command} ${DEFAULT_ARCH}"
			

			if [ "_$with_input_vector" == "_1" ] && [ "_$REGENERATE_BENCH" != "_1" ]; then
				verilog_command="${verilog_command} -t ${benchmark_dir}/${test_name}_input"
				
				if [ "_$with_output_vector" == "_1" ] && [ "_$REGENERATE_OUTPUT" != "_1" ]; then
					verilog_command="${verilog_command} -T ${benchmark_dir}/${test_name}_output"
				fi
			else
				verilog_command="${verilog_command} ${HOLD_PARAM}"

				if [ "_$REGENERATE_BENCH" != "_1" ]; then
					verilog_command="${verilog_command} ${GENERATE_VECTOR_COUNT}"
				else
					verilog_command="${verilog_command} ${REGEN_VECTOR_CMD}"
				fi
			fi

			echo "${verilog_command} -sim_dir ${DIR}/ &>> ${DIR}/log \
				&& echo --- PASSED == ${TEST_FULL_REF} \
				|| (echo -X- FAILED == ${TEST_FULL_REF} | tee ${new_run}/failure.log)" > ${DIR}/log
		done
	fi

	if [ $1 -gt "1" ]
	then
		find ${new_run}/${bench_type}/ -maxdepth 1 -mindepth 1 | xargs -n1 -P$1 -I test_dir /bin/bash -c \
			'eval $(cat "test_dir/log" | tr "\n" " ")'
	else
		for tests in ${new_run}/${bench_type}/*; do 
			eval $(cat "${tests}/log" | tr "\n" " ")
		done
	fi

	if [ "_$REGENERATE_BENCH" == "_1" ] || [ "_$REGENERATE_OUTPUT" == "_1" ]
	then
		#rename input and output vectors and move to vector directory
		mkdir -p ${new_run}/VECTORS/
		for tests in ${new_run}/${bench_type}/*; do 
			test_name=${tests##*/}
			if [ -e ${tests}/input_vectors ]; then
				cp ${tests}/input_vectors ${new_run}/VECTORS/${test_name}_input
				cp ${tests}/output_vectors ${new_run}/VECTORS/${test_name}_output
				echo -e "$(cat ${tests}/log | grep Coverage: | cut -d '(' -f2 | cut -d ')' -f1) <= ${test_name} " >> ${new_run}/VECTORS/report.coverage
			fi
		done
	fi
}

function other_test() {
	threads=$NB_OF_PROC
	bench_type=other
	with_input_vector=0
	with_output_vector=0
	with_blif=0
	with_arch=0
	with_input_args=1

	sim $threads $bench_type $with_input_vector $with_output_vector $with_blif $with_arch $with_input_args
}

function micro_test() {
	threads=$NB_OF_PROC
	bench_type=micro
	with_input_vector=1
	with_output_vector=1
	with_blif=1
	with_arch=0
	with_input_args=0

	sim $threads $bench_type $with_input_vector $with_output_vector $with_blif $with_arch $with_input_args
}

function regression_test() {
	threads=1
	bench_type=full
	with_input_vector=1
	with_output_vector=1
	with_blif=0
	with_arch=1
	with_input_args=0

	sim $threads $bench_type $with_input_vector $with_output_vector $with_blif $with_arch $with_input_args
}

function arch_test() {
	threads=$NB_OF_PROC
	bench_type=arch
	with_input_vector=0
	with_output_vector=0
	with_blif=0
	with_arch=1
	with_input_args=0

	sim $threads $bench_type $with_input_vector $with_output_vector $with_blif $with_arch $with_input_args
}

function syntax_test() {
	threads=$NB_OF_PROC
	bench_type=syntax
	with_input_vector=0
	with_output_vector=0
	with_blif=0
	with_arch=0
	with_input_args=0

	sim $threads $bench_type $with_input_vector $with_output_vector $with_blif $with_arch $with_input_args
}

function functional_test() {
	threads=$NB_OF_PROC
	bench_type=functional
	with_input_vector=1
	with_output_vector=0
	with_blif=0
	with_arch=0

	sim $threads $bench_type $with_input_vector $with_output_vector $with_blif $with_arch $with_input_args
}

function operators_test() {
	threads=$NB_OF_PROC
	bench_type=operators
	with_input_vector=1
	with_output_vector=1
	with_blif=1
	with_arch=0
	with_input_args=0

	sim $threads $bench_type $with_input_vector $with_output_vector $with_blif $with_arch $with_input_args
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
		ctrl_c
		;;
esac

print_time_since $START

exit_program
### end here
