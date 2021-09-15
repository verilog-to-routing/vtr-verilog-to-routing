#!/bin/bash

# the purpose of this script is to go through a number of .vqm files
# and generate the corresponding .blif files using the vqm2blif program.
# The generated blif files will be compared to a golden set of outputs
# to validate the vqm2blif program changes.


# has useful utilities
source "$(dirname "$0")/../../../../.github/scripts/common.sh"

### some useful directories ###
# location of the VTR project root directory
VTR_ROOT_DIR="$(dirname "$0")/../../../../"

# location of the deafult architecture file used when testing the vqm2blif program
ARCH_FILE="$(dirname "$0")/../../../../vtr_flow/arch/titan/stratixiv_arch.timing.xml"

# location of the default folder where all the test netlists are located
TEST_FOLDER="$(dirname "$0")/../netlists/"

### useful file extensions ###
# file extensions of the created blif netlists (these files will be compared to the golden netlists)
VQM_OUTPUT_EXT=".test.blif"

# variable to keep track of the test status
# initially the status is successful
TEST_STATUS=0 

# check if the user provided a different folder for the test netlist locations
# if they did, the update the test folder
if ! [ -z $1 ]; then

    # update the test folder to the user input
    TEST_FOLDER=$1

fi

# check if the user provided a different architecture file to use.
# if they did, then update the architecture file.
if ! [ -z $2 ]; then

    # upadte the architecture file to the user input
    ARCH_FILE=$2

fi


echo "VQM2BLIF Program Check"
echo "----------------------"


# start by going through the user supplied directory and identify all the .vqm files. We will recursively go throught the directory and identify these .vqm files. We assume that any file found is going to be a test file.
find $TEST_FOLDER -name "*.vqm" -print0 | while read -d $'\0' vqm_file
do
    
    # identify the directory of the current vqm file and its name
    CURR_DIR_NAME="$(dirname "$vqm_file")"
    CURR_VQM_FILE_NAME="$(basename "$vqm_file" .vqm)"

    # generate the output file name
    TEST_BLIF_FILE_NAME="${CURR_VQM_FILE_NAME}${VQM_OUTPUT_EXT}"
    VQM_OUTPUT="${CURR_DIR_NAME}/${TEST_BLIF_FILE_NAME}"

    # we need to find the blif file to compare our test to
    # we will assume that the blif file will be located in the same location as the input file
    GOLDEN_BLIF_OUTPUT="$(find "$CURR_DIR_NAME" -name *.blif)"
    GOLDEN_BLIF_OUTPUT_FILE_NAME="$(basename "$GOLDEN_BLIF_OUTPUT" .blif)"

    # uncomment for debugging
    #echo "Starting test $CURR_VQM_FILE_NAME"

    # now we need to run the vqm to blif program.
    # we only care about any outputs related to errors, so ignore standard output.
    # We are running this process in a seperate sub shell to make sure that unwanted error messags are not reported in the terminal
    ($VTR_ROOT_DIR/build/utils/vqm2blif/vqm2blif -vqm $vqm_file -arch $ARCH_FILE -out $VQM_OUTPUT || false) > /dev/null 2>&1

    if [ $? -ne 0 ]; then

        echo "ERROR: The vqm2blif program failed while converting \"$CURR_VQM_FILE_NAME.vqm\". Please refer to the failure messages below:"
        echo "-----------------------------------------------------------------------------------------------------------------------"

        $VTR_ROOT_DIR/build/utils/vqm2blif/vqm2blif -vqm $vqm_file -arch $ARCH_FILE -out $VQM_OUTPUT

        exit 1
    fi

    #uncomment for debugging
    #echo "Comparing $CURR_VQM_FILE_NAME output"

    NUM_OF_DIFFERENCES="$(diff $GOLDEN_BLIF_OUTPUT $VQM_OUTPUT | grep "^>" | wc -l)"

    #uncomment for debugging
    #echo "Number of differences: $NUM_OF_DIFFERENCES"

    # by default, the newly created blif file should have a different name, so we should only have 1 difference
    # so below we check to see if there was more than one difference and if there was we throw an error
    if [ $NUM_OF_DIFFERENCES -gt 1 ]; then

        echo "ERROR: The generated blif file \"$TEST_BLIF_FILE_NAME\" did not match the golden reference \"$GOLDEN_BLIF_OUTPUT_FILE_NAME.blif\"."
        echo "The differences are shown below:"
        echo "------------------------------------------------------------------------------------------------------------------------------------------"

        diff $GOLDEN_BLIF_OUTPUT $VQM_OUTPUT

        exit 1
    fi
    
    # uncomment for debugging
    #echo "Finished test $CURR_VQM_FILE_NAME"
    
    # delete the newly created test blif netlist (we dont need it anymore)
    #rm $VQM_OUTPUT

done

# we create seperate subshells to process each iteration of the loops above.
# So below we check the result of the last shell.
if [ $? -eq 1 ]; then

    # if we are here then the test failed, so update the test status with an error code
    TEST_STATUS=1

else

    # if we are here then the test passed, so report to the user (no need to update the test status)
    echo "VQM2BLIF Checks Successfully Passed!"    
fi

#find all the newly created blif files above (used for verification) and delete them (don't need them anymore)
find $TEST_FOLDER -name "*.test.blif" -type f -delete

# exit with the status of the test (failure=1 and pass=0)
exit $TEST_STATUS


