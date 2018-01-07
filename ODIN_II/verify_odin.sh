#!/bin/bash 

#ripple, log, fixed

ADDER_TYPE="ripple"
INITIAL_SIZE="0"
SKIP_SIZE="0"

ADDER_DEF="--adder_type $ADDER_TYPE,$INITIAL_SIZE,$SKIP_SIZE"

ADDER=$ADDER_DEF
case $ADDER_TYPE in
  "ripple") ADDER="" ;;
  "log") ;;
  "fixed") ;;
  *) echo 'wrong type of $ADDER in your script, one of ripple, log or fixed' ;;
esac

function finish() {
	echo "=============================================================================="
	echo "..%%%%..%%%%%..%%%%%%.%%..%%........%%%%%%..%%%%..%%%%%%.%%.....%%%%%%.%%%%%.."
	echo ".%%..%%.%%..%%...%%...%%%.%%........%%.....%%..%%...%%...%%.....%%.....%%..%%."
	echo ".%%..%%.%%..%%...%%...%%.%%%........%%%%...%%%%%%...%%...%%.....%%%%...%%..%%."
	echo ".%%..%%.%%..%%...%%...%%..%%........%%.....%%..%%...%%...%%.....%%.....%%..%%."
	echo "..%%%%..%%%%%..%%%%%%.%%..%%........%%.....%%..%%.%%%%%%.%%%%%%.%%%%%%.%%%%%.."
	echo "=============================================================================="
	echo "@ $benchmark" 
	clean_up
    exit 1;
}
trap finish INT

function pass() {
	echo "--------------------------------------" 
	echo "PPPPPPPPPPPPPPPPP        AAA                 SSSSSSSSSSSSSSS   SSSSSSSSSSSSSSS "
	echo "P::::::::::::::::P      A:::A              SS:::::::::::::::SSS:::::::::::::::S"
	echo "P::::::PPPPPP:::::P    A:::::A            S:::::SSSSSS::::::S:::::SSSSSS::::::S"
	echo "PP:::::P     P:::::P  A:::::::A           S:::::S     SSSSSSS:::::S     SSSSSSS"
	echo "  P::::P     P:::::P A:::::::::A          S:::::S           S:::::S            "
	echo "  P::::P     P:::::PA:::::A:::::A         S:::::S           S:::::S            "
	echo "  P::::PPPPPP:::::PA:::::A A:::::A         S::::SSSS         S::::SSSS         "
	echo "  P:::::::::::::PPA:::::A   A:::::A         SS::::::SSSSS     SS::::::SSSSS    "
	echo "  P::::PPPPPPPPP A:::::A     A:::::A          SSS::::::::SS     SSS::::::::SS  "
	echo "  P::::P        A:::::AAAAAAAAA:::::A            SSSSSS::::S       SSSSSS::::S "
	echo "  P::::P       A:::::::::::::::::::::A                S:::::S           S:::::S"
	echo "  P::::P      A:::::AAAAAAAAAAAAA:::::A               S:::::S           S:::::S"
	echo "PP::::::PP   A:::::A             A:::::A  SSSSSSS     S:::::SSSSSSS     S:::::S"
	echo "P::::::::P  A:::::A               A:::::A S::::::SSSSSS:::::S::::::SSSSSS:::::S"
	echo "P::::::::P A:::::A                 A:::::AS:::::::::::::::SSS:::::::::::::::SS "
	echo "PPPPPPPPPPAAAAAAA                   AAAAAAASSSSSSSSSSSSSSS   SSSSSSSSSSSSSSS   "
	clean_up
}

function clean_up() {
	rm "temp.blif"
    rm "output_vectors"
    rm "output_activity"
    rm "input_vectors"
    rm "default_out.blif"
    rm "test.do"
}

TEST_DIR_REG="FULL_REGRESSION_TESTS"
TEST_DIR="REGRESSION_TESTS/BENCHMARKS/MICROBENCHMARKS"
ARCH="../libs/libarchfpga/arch/sample_arch.xml"

function microbench() {
	for benchmark in $TEST_DIR/*.v
	do 
		basename=${benchmark%.v}
		input_vectors="$basename"_input
		output_vectors="$basename"_output	
	
		echo $benchmark 
	
		############################
		# Simulate using verilog. 
		############################
		# With the arch file
		rm output_vectors
		echo ./odin_II -E -a "$ARCH" -V "$benchmark" -t "$input_vectors" -T "$output_vectors" $ADDER
		./odin_II -E -a "$ARCH" -V "$benchmark" -t "$input_vectors" -T "$output_vectors" $ADDER 2> log || exit 1
		[ -e "output_vectors" ] || exit 1
	
		# Without the arch file
		rm output_vectors
		echo ./odin_II -E -V "$benchmark" -t "$input_vectors" -T "$output_vectors" $ADDER
		./odin_II -E -V "$benchmark" -t "$input_vectors" -T "$output_vectors" $ADDER 2> log || exit 1
		[ -e "output_vectors" ] || exit 1
	
		############################
		# Simulate using the blif file. 
		############################
		# With the arch file. 	
		rm "temp.blif"
		echo ./odin_II -E -a $ARCH -V "$benchmark" -o "temp.blif" $ADDER
		./odin_II -E -a $ARCH -V "$benchmark" -o "temp.blif" $ADDER 2> log || exit 1
		[ -e "temp.blif" ] || exit 1
	
		rm output_vectors
		echo ./odin_II -E -a $ARCH -b "temp.blif" -t "$input_vectors" -T "$output_vectors"
		./odin_II -E -a $ARCH -b "temp.blif" -t "$input_vectors" -T "$output_vectors" 2> log || exit 1
		[ -e "output_vectors" ] || exit 1
	
		# Without the arch file. 	
		rm "temp.blif"
		echo ./odin_II -E -V "$benchmark" -o "temp.blif" $ADDER
		./odin_II -E -V "$benchmark" -o "temp.blif" $ADDER 2> log || exit 1
		[ -e "temp.blif" ] || exit 1
	
		rm output_vectors
		echo ./odin_II -E -b "temp.blif" -t "$input_vectors" -T "$output_vectors" 
		./odin_II -E -b "temp.blif" -t "$input_vectors" -T "$output_vectors" 2> log || exit 1
		[ -e "output_vectors" ] || exit 1
	
	
	done
}

function regression() {
	for benchmark in $TEST_DIR_REG/*.v
	do 
		basename=${benchmark%.v}
		input_vectors="$basename"_input
		output_vectors="$basename"_output	
		
		echo $benchmark
	
		rm output_vectors
		echo ./odin_II -E -a "$ARCH" -V "$benchmark" -t "$input_vectors" -T "$output_vectors" $ADDER
	    ./odin_II -E -a "$ARCH" -V "$benchmark" -t "$input_vectors" -T "$output_vectors" $ADDER 2> log || exit 1
	    [ -e "output_vectors" ] || exit 1
	done
}

function vtr_suite() {
	cd ..
	/usr/bin/perl run_reg_test.pl vtr_reg_strong
}





### starts here

case $1 in
  "microbenchmark") 
	microbench 
	pass
	;;
  "regression") 
	regression 
	pass
	;;
  "vtr_reg") 
	vtr_suite 
	;;
  "full_suite")  
	microbench
	regression
	pass
	vtr_suite 
	;;
  *) echo 'nothing to run, use microbenchmark, regression, vtr_reg or full_suite' ;;
esac




