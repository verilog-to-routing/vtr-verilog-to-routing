#!/bin/bash 

TEST_DIR="REGRESSION_TESTS/BENCHMARKS/MICROBENCHMARKS"
ARCH="../libarchfpga/arch/sample_arch.xml"

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
	echo ./odin_II.exe -E -a "$ARCH" -V "$benchmark" -t "$input_vectors" -T "$output_vectors"
	./odin_II.exe -E -a "$ARCH" -V "$benchmark" -t "$input_vectors" -T "$output_vectors" 2> log || exit 1
	[ -e "output_vectors" ] || exit 1

	# Without the arch file
	rm output_vectors
	echo ./odin_II.exe -E -V "$benchmark" -t "$input_vectors" -T "$output_vectors"
	./odin_II.exe -E -V "$benchmark" -t "$input_vectors" -T "$output_vectors" 2> log || exit 1
	[ -e "output_vectors" ] || exit 1

	############################
	# Simulate using the blif file. 
	############################
	# With the arch file. 	
	rm "temp.blif"
	echo ./odin_II.exe -E -a $ARCH -V "$benchmark" -o "temp.blif"
	./odin_II.exe -E -a $ARCH -V "$benchmark" -o "temp.blif" 2> log || exit 1
	[ -e "temp.blif" ] || exit 1

	rm output_vectors
	echo ./odin_II.exe -E -a $ARCH -b "temp.blif" -t "$input_vectors" -T "$output_vectors"
	./odin_II.exe -E -a $ARCH -b "temp.blif" -t "$input_vectors" -T "$output_vectors" 2> log || exit 1
	[ -e "output_vectors" ] || exit 1

	# Without the arch file. 	
	rm "temp.blif"
	echo ./odin_II.exe -E -V "$benchmark" -o "temp.blif"
	./odin_II.exe -E -V "$benchmark" -o "temp.blif" 2> log || exit 1
	[ -e "temp.blif" ] || exit 1

	rm output_vectors
	echo ./odin_II.exe -E -b "temp.blif" -t "$input_vectors" -T "$output_vectors" 
	./odin_II.exe -E -b "temp.blif" -t "$input_vectors" -T "$output_vectors" 2> log || exit 1
	[ -e "output_vectors" ] || exit 1


done

echo "--------------------------------------" 
echo "$0: All tests completed successfully." 
