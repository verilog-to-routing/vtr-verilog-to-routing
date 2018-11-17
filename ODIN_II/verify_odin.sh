#!/bin/bash
#1
trap ctrl_c INT
SHELL=/bin/bash

#include more generic names here for better vector generation
HOLD_LOW_RESET="-L reset rst"
HOLD_HIGH_WE="-H we"

HOLD_PARAM="${HOLD_LOW_RESET} ${HOLD_HIGH_WE}"

#if you want to pass in a new adder definition file
ADDER_DEFINITION="--adder_type default"

#if you want to change the default number of vectors to generate
GENERATE_VECTOR_COUNT="-g 1000"

DEFAULT_ARCH="-a ../libs/libarchfpga/arch/sample_arch.xml"

EXEC="./odin_II"

fail_count=0
new_run=regression_test/run001
NB_OF_PROC=1
if [[ "$2" -gt "0" ]]
then
	NB_OF_PROC=$2
	echo "Trying to run benchmark on $NB_OF_PROC processes"
fi

REGENERATE_OUTPUT=0
REGENERATE_BENCH=0

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
	with_blif=$5
	with_arch=$6
	passing_args=$7
	
	benchmark_dir=regression_test/benchmark/${bench_type}

	if [ "_${passing_args}" == "_1" ]; then
		for dir in ${benchmark_dir}/*
		do

			test_name=${dir##*/}
			DIR="${new_run}/${bench_type}/$test_name"

			#build commands
			mkdir -p $DIR

			echo "${EXEC} $(cat ${dir}/odin.args | tr '\n' ' ') -o ${DIR}/odin.blif -sim_dir ${DIR}/ &>> ${DIR}/log \
				&& echo --- PASSED == ${bench_type}/$test_name \
				|| (echo -X- FAILED == ${bench_type}/$test_name \
					&& echo ${bench_type}/$test_name >> ${new_run}/failure.log)" > ${DIR}/log

		done
	else
		for benchmark in ${benchmark_dir}/*.v
		do
			basename=${benchmark%.v}
			test_name=${basename##*/}
			DIR="${new_run}/${bench_type}/$test_name"

			#build commands
			mkdir -p $DIR

			verilog_command=""
			blif_command=""

			verilog_command="${EXEC} ${ADDER_DEFINITION} -V ${benchmark_dir}/${test_name}.v -o ${DIR}/odin.blif"

			[ "_$with_blif" == "_1" ] &&
				blif_command=" && ${EXEC} ${ADDER_DEFINITION} -b ${DIR}/odin.blif"

			[ "_$with_arch" == "_1" ] &&
				verilog_command="${verilog_command} ${DEFAULT_ARCH}"
			
			[ "_$with_blif" == "_1" ] && [ "_$with_arch" == "_1" ] &&
				blif_command="${blif_command} ${DEFAULT_ARCH}"

			if [ "_$with_input_vector" == "_1" ] && [ "_$REGENERATE_BENCH" != "_1" ]; then
				verilog_command="${verilog_command} -t ${benchmark_dir}/${test_name}_input"

				[ "_$with_blif" == "_1" ] &&
					blif_command="${blif_command} -t ${benchmark_dir}/${test_name}_input"
				
				if [ "_$with_output_vector" == "_1" ] && [ "_$REGENERATE_OUTPUT" != "_1" ]; then
					verilog_command="${verilog_command} -T ${benchmark_dir}/${test_name}_output"

					[ "_$with_blif" == "_1" ] &&
						blif_command="${blif_command} -T ${benchmark_dir}/${test_name}_output"
				fi
			else
				verilog_command="${verilog_command} ${HOLD_PARAM} ${GENERATE_VECTOR_COUNT}"

				[ "_$with_blif" == "_1" ] &&
					blif_command="${blif_command} ${HOLD_PARAM} ${GENERATE_VECTOR_COUNT}"
			fi

			verilog_command="${verilog_command} -sim_dir ${DIR}/ &>> ${DIR}/log"
			
			[ "_$with_blif" == "_1" ] &&
				blif_command="${blif_command} -sim_dir ${DIR}/ &>> ${DIR}/log"

			echo "${verilog_command} ${blif_command} \
				&& echo --- PASSED == ${bench_type}/$test_name \
				|| (echo -X- FAILED == ${bench_type}/$test_name \
					&& echo ${bench_type}/$test_name >> ${new_run}/failure.log)" > ${DIR}/log
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


#1				#2
#benchmark dir	N_trhead
function other_test() {
	threads=$1
	bench_type=other
	with_input_vector=0
	with_output_vector=0
	with_blif=0
	with_arch=0
	with_input_args=1

	sim $threads $bench_type $with_input_vector $with_output_vector $with_blif $with_arch $with_input_args
}


function micro_test() {
	threads=$1
	bench_type=micro
	with_input_vector=1
	with_output_vector=1
	with_blif=1
	with_arch=0
	with_input_args=0

	sim $threads $bench_type $with_input_vector $with_output_vector $with_blif $with_arch $with_input_args
}

#1
#bench
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


#1		
#N_trhead
function arch_test() {
	threads=$1
	bench_type=arch
	with_input_vector=0
	with_output_vector=0
	with_blif=0
	with_arch=1
	with_input_args=0

	sim $threads $bench_type $with_input_vector $with_output_vector $with_blif $with_arch $with_input_args
}

#1				#2
#benchmark dir	N_trhead
function syntax_test() {
	threads=$1
	bench_type=syntax
	with_input_vector=0
	with_output_vector=0
	with_blif=0
	with_arch=0
	with_input_args=0

	sim $threads $bench_type $with_input_vector $with_output_vector $with_blif $with_arch $with_input_args
}

#1				#2
#benchmark dir	N_trhead
function functional_test() {
	threads=$1
	bench_type=functional
	with_input_vector=1
	with_output_vector=0
	with_blif=0
	with_arch=0

	sim $threads $bench_type $with_input_vector $with_output_vector $with_blif $with_arch $with_input_args
}

function operators_test() {
	threads=$1
	bench_type=operators
	with_input_vector=1
	with_output_vector=1
	with_blif=1
	with_arch=0
	with_input_args=0

	sim $threads $bench_type $with_input_vector $with_output_vector $with_blif $with_arch $with_input_args
}

START=$(date +%s%3N)

case $3 in 
	"generate_output")
		echo regenerating output vectors
		REGENERATE_OUTPUT=1
		;;

	"generate_bench")
		echo regenerating input and output vectors
		REGENERATE_BENCH=1
		;;
esac


case $1 in

	"operators")
		init_temp
		operators_test $NB_OF_PROC
		;;

	"functional")
		init_temp
		functional_test $NB_OF_PROC
		;;

	"arch")
		init_temp
		arch_test $NB_OF_PROC
		;;

	"syntax")
		init_temp
		syntax_test $NB_OF_PROC
		;;

	"micro")
		init_temp
		micro_test $NB_OF_PROC
		;;

	"regression")
		init_temp
		regression_test $NB_OF_PROC
		;;

	"other")
		init_temp
		other_test $NB_OF_PROC
		;;

	"full_suite")
		init_temp
		arch_test $NB_OF_PROC
		syntax_test $NB_OF_PROC
		other_test $NB_OF_PROC
		micro_test $NB_OF_PROC
		regression_test $NB_OF_PROC
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
		arch_test $NB_OF_PROC
		syntax_test $NB_OF_PROC
		other_test $NB_OF_PROC
		micro_test $NB_OF_PROC
		regression_test $NB_OF_PROC
		cd ..
		/usr/bin/perl run_reg_test.pl -j $NB_OF_PROC vtr_reg_basic
		/usr/bin/perl run_reg_test.pl -j $NB_OF_PROC vtr_reg_strong
		cd ODIN_II
		;;

	"clean")
		for runs in regression_test/run*; do rm -Rf ${runs}; done
		echo cleaned temporary folders
		exit 0
		;;
	*)
		echo 'nothing to run, ./verify_odin [ clean, arch, syntax, other, micro, regression, vtr_basic, vtr_strong or pre_commit ] [ nb_of_process ]'
		;;
esac

END=$(date +%s%3N)
TIME_TO_RUN=$(( ${END} - ${START} ))

Mili=$(( ${TIME_TO_RUN} %1000 ))
Sec=$(( ( ${TIME_TO_RUN} /1000 ) %60 ))
Min=$(( ( ( ${TIME_TO_RUN} /1000 ) /60 ) %60 ))
Hour=$(( ( ( ${TIME_TO_RUN} /1000 ) /60 ) /60 ))

echo "ran test in: $Hour:$Min:$Sec.$Mili"
exit_program
### end here
