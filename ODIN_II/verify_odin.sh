#!/bin/bash 
#1
#failed_benchmark
FAILURE_LOG="regression_test/runs/failure.log"

function failed() {
	echo " -X- FAILED == $1" 
	echo $1 >> $FAILURE_LOG
}

function pass() {
	echo " --- PASSED == $1"
}

exit_finish() {
	
	failed $1
	exit 1
}

function clean_up() {
	rm -Rf regression_test/runs/regression_test
}

REGR_BENCH="regression_test/benchmark/full"
ARCH_BENCH="regression_test/benchmark/arch"
SYNT_BENCH="regression_test/benchmark/syntax"
MICR_BENCH="regression_test/benchmark/micro"

ARCH="../libs/libarchfpga/arch/sample_arch.xml"

#1
#bench
function extensive_vector() {
	
	for benchmark in $1/*.v
	do 
		(
			basename=${benchmark%.v}
			input_vectors="$basename"_input
			output_vectors="$basename"_output	
			
			DIR="regression_test/runs/$basename"
			
			############################
			# Simulate using verilog. 
			############################
			# With the arch file
			V_ARCH=$DIR/verilog_arch && mkdir -p $V_ARCH
			./odin_II -W -E -a "$ARCH" -V "$benchmark" -t "$input_vectors" -T "$output_vectors" -o "$V_ARCH/temp.blif" -sim_dir "$V_ARCH/" &> "$V_ARCH/log" \
			&& pass $benchmark || failed $benchmark
			
			# Without the arch file
			V_NO_ARCH=$DIR/verilog_no_arch && mkdir -p $V_NO_ARCH
			./odin_II -W -E -V "$benchmark" -t "$input_vectors" -T "$output_vectors"  -o "$V_NO_ARCH/temp.blif" -sim_dir "$V_NO_ARCH/" &> "$V_NO_ARCH/log" \
			&& pass $benchmark || failed $benchmark
			
			############################
			# Simulate using the blif file. 
			############################
			B_ARCH=$DIR/bliff_arch && mkdir -p $B_ARCH
			./odin_II -E -W -a $ARCH -V "$benchmark" -o "$B_ARCH/temp.blif" -sim_dir "$B_ARCH/" &> "$B_ARCH/log" && \
			./odin_II -E -W -a $ARCH -b "$B_ARCH/temp.blif" -t "$input_vectors" -T "$output_vectors" -sim_dir "$B_ARCH/" &>> "$B_ARCH/log" \
			&& pass $benchmark || failed $benchmark
			
			# Without the arch file. 
			B_NO_ARCH=$DIR/bliff_no_arch && mkdir -p $B_NO_ARCH
			./odin_II -E -W -V "$benchmark" -o "$B_NO_ARCH/temp.blif" -sim_dir "$B_NO_ARCH/" &> "$B_NO_ARCH/log" && \
			./odin_II -E -W -b "$B_NO_ARCH/temp.blif" -t "$input_vectors" -T "$output_vectors" -sim_dir "$B_NO_ARCH/" &>> "$B_NO_ARCH/log" \
			&& pass $benchmark || failed $benchmark
		) &
	
	    [[ $(jobs -r -p | wc -l) -le $2 ]] || wait -n
	
	
	done
	wait
}

#1
#bench
function basic_vector() {
	pass_count=$3 
	fail_count=$4
	
	for benchmark in $1/*.v
	do 
		(
			basename=${benchmark%.v}
			DIR="regression_test/runs/$basename"
			mkdir -p $DIR
			
			input_vectors="$basename"_input
			output_vectors="$basename"_output	

		    ./odin_II -E -W -a "$ARCH" -V "$benchmark" -t "$input_vectors" -T "$output_vectors" -o "$DIR/temp.blif" -sim_dir "$DIR/" &> "$DIR/log" \
			&& pass $benchmark || failed $benchmark
			
	    ) &
	
	    [[ $(jobs -r -p | wc -l) -le $2 ]] || wait -n
	done
	wait
}

#1				#2
#benchmark dir	N_trhead
function simple_test() {
	pass_count=$3 
	fail_count=$4
	
	for benchmark in $1/*.v
	do 
	    (
	    	basename=${benchmark%.v}
			DIR="regression_test/runs/$basename"
			mkdir -p $DIR
			
			./odin_II -E -W -a "$ARCH" -V "$benchmark" -o "$DIR/temp.blif" -sim_dir "$DIR/" &> "$DIR/log" \
			&& pass $benchmark || failed $benchmark
			
		) &
	
	    [[ $(jobs -r -p | wc -l) -le $2 ]] || wait -n

	done
	wait
}

function simple_file() {
	pass_count=$1 
	fail_count=$2
	
	cat FAILURE_LOG | while read p;
	do 
		if [ -z $p ]
		then
			break
		fi
    	basename=${p%.v}
		DIR=$basename
		mkdir -p $DIR
		
		./odin_II -E -W -a "$ARCH" -V "$p" -o "$DIR/temp.blif" -sim_dir "$DIR/" &> "$DIR/log" \
		&& pass $p || failed $p
			
	done
}

### starts here
export pass_count=0
export fail_count=0

clean_up

NB_OF_PROC=1

if [[ "$2" -gt "0" ]]
then
	NB_OF_PROC=$2
	echo "running benchmark on $NB_OF_PROC thread"
fi

case $1 in
	"rerun")
		simple_file 
		exit 0
		;;
	*)
		rm -f $FAILURE_LOG
		;;
esac

START=$(date +%s%3N)

case $1 in

	"arch")
		simple_test $ARCH_BENCH $NB_OF_PROC 
		;;
	
	"syntax")
		simple_test $SYNT_BENCH $NB_OF_PROC 
		;;
		
	"micro") 
		extensive_vector $MICR_BENCH $NB_OF_PROC 
		;;
		
	"regression") 
		basic_vector $REGR_BENCH $NB_OF_PROC 
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
		simple_test $ARCH_BENCH $NB_OF_PROC 
		simple_test $SYNT_BENCH $NB_OF_PROC 
		extensive_vector $MICR_BENCH $NB_OF_PROC 
		basic_vector $REGR_BENCH $NB_OF_PROC 
		cd ..
		/usr/bin/perl run_reg_test.pl -j $NB_OF_PROC vtr_reg_basic 
		/usr/bin/perl run_reg_test.pl -j $NB_OF_PROC vtr_reg_strong
		cd ODIN_II
		;;
		
	*) 
		echo 'nothing to run, ./verify_odin [ arch, syntax, micro, full, regression, vtr_basic, vtr_strong or pre_commit ] [ nb_of_process ]' 
		;;
esac
wait

END=$(date +%s%3N)
echo "ran test in: $(( ((END-START)/1000)/60 )):$(( ((END-START)/1000)%60 )).$(( (END-START)%1000 )) [m:s.ms]"

fail_count=0
if [ -e $FAILURE_LOG ]
then
	fail_count=$(wc -l < $FAILURE_LOG)
fi

if [ $fail_count -gt "0" ]
then
	echo "Failed $fail_count"
	echo "View Failure log in regression_test/runs/failure.log, "
	echo "once the issue is fixed, you can retry only the failed test by calling \'./verify_odin rerun\' "
else
	echo "no run failure!"
fi

### end here


