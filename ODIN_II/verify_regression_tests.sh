#!/bin/bash 

TEST_DIR="FULL_REGRESSION_TESTS"
ARCH="../libarchfpga/arch/sample_arch.xml"

for benchmark in $TEST_DIR/*.v
do 
	basename=${benchmark%.v}
	input_vectors="$basename"_input
	output_vectors="$basename"_output	
	
	echo $benchmark

	rm output_vectors
	#rm temp.blif
        ./odin_II.exe -E -a "$ARCH" -V "$benchmark" -t "$input_vectors" -T "$output_vectors" || exit 1
        [ -e "output_vectors" ] || exit 1

	#rm output_vectors
        #./odin_II.exe -E -a "$ARCH" -b "temp.blif" -t "$input_vectors" -T "$output_vectors" || exit 1
        #[ -e "output_vectors" ] || exit 1
done

echo "--------------------------------------" 
echo "$0: All tests completed successfully." 
