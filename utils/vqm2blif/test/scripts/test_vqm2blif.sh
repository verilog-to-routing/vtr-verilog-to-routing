#!/bin/bash

# The purpose of this script is to go through a number of .vqm files
# and generate the corresponding .blif files using the vqm2blif program.
# The generated blif files will be compared to a golden set of outputs
# to validate the vqm2blif program changes.

# This program accepts two arguments:
# Argument 1: Location of the directory that contains all the netlists
# Argument 2: Location of the architecture file that will be used when running the vqm2blif program

# if any of the arguments are not supplied, then default values will be used (see below for further information)

# This script assumes that the corresponding "golden" blif file for any vqm netlist is in the same directory and it is the ONLY blif file in the directory.
# See below for a sample directory structure:
# - netlists
#    - netlist_1
#       - circuit_1.vqm
#       - circuit_1.blif
#    - netlist_2
#       - circuit_2.vqm
#       - circuit_2.blif
#    - netlist_3

# please note that the original Titan benchmarks were arranged this way

# Finally, the generated blif files have the extension ".test.blif", so please do not name any other files with this extension

## usefule functions ##
# prints information regarding how to use the script
print_usage() 
{
    echo "Usage: test_vqm2blif.sh [-h | --help] [-vqm2blif_location] V [-netlists] N [-arch] A [-create_golden]"
    echo ""
    echo "Evaluating the vqm2blif program."
    echo ""
    echo "Positional Arguments:"
    echo "V             Location of the vqm2blif program executable"
    echo "N             Location of the folder containing the netlists"
    echo "A             Location of the architecture file to use"
    echo ""
    echo "Optional Arguments:"
    echo "-h, --help            show the usage of the test script"
    echo "-vqm2blif_location    specify the vqm2blif program executable (defalt: \$VTR_ROOT/build/utils/vqm2blif/vqm2blif"
    echo "-netlists             specify the netlists used to evalute the vqm2blif program (default: \$VTR_ROOT/utils/vqm2blif/test/netlists/"
    echo "-arch                 specify the architecture file that will be used to evalute the vqm2blif program" 
    echo "                      (default: \$VTR_ROOT/vtr_flow/arch/titan/stratixiv_arch.timing.xml)"    
    echo "-create_golden        creates a set of \"golden\" .blif netlists using the vqm2blif program. The generated netlists will have the same"
    echo "                      name as the provided .vqm netlists with the extension \"golden.blif\". The generated netlists can be found in the same"
    echo "                      location as provided .vqm netlists."
}


# has useful utilities
source "$(dirname "$0")/../../../../.github/scripts/common.sh"

### some useful directories ###
# location of the VTR project root directory
VTR_ROOT_DIR="$(dirname "$0")/../../../../"

# location of the vqm program directory
VQM_PROGRAM="$VTR_ROOT_DIR/build/utils/vqm2blif/vqm2blif"

# location of the deafult architecture file used when testing the vqm2blif program
ARCH_FILE="$(dirname "$0")/../../../../vtr_flow/arch/titan/stratixiv_arch.timing.xml"

# location of the default folder where all the test netlists are located (basic becnchmak tests)
TEST_FOLDER="$(dirname "$0")/../netlists/"

### useful file extensions ###
# file extensions of the created blif netlists (these files will be compared to the golden netlists)
VQM_OUTPUT_EXT=".test.blif"

# file extension for the generated golden blif netlists
VQM_GOLDEN_OUTPUT_EXT=".golden.blif"

# variable to keep track of the test status
# initially the status is successful
TEST_STATUS=0

# variable to determine whether the user wanted to generate a set of golden netlists (initially we assume the golden netlists are not being generated)
GEN_GOLDEN=0

# we process the command line inputs below
while test $# -gt 0;
do
    case "$1" in
    -h | --help)
        print_usage
        exit 0
        ;;
    -vqm2blif_location)
        if ! [ -z $2 ]; then

            # update the test folder to the user input
            VQM_PROGRAM=$2

        else
            echo "Did not supply vqm program location"
            echo "-----------------------------------"
            print_usage
            exit 1
        fi
        shift
        shift
        ;;
    -netlists)
        if ! [ -z $2 ]; then

            # update the test folder to the user input
            TEST_FOLDER=$2

        else
            echo "Did not supply the folder location for netlists"
            echo "-----------------------------------------------"
            print_usage
            exit 1
        fi
        shift
        shift
        ;;
    -arch)
       if ! [ -z $2 ]; then

            # update the test folder to the user input
            ARCH_FILE=$2

        else
            echo "Did not supply architecture file location"
            echo "-----------------------------------------"
            print_usage
            exit 1
        fi
        shift
        shift
        ;;
    -create_golden)
        GEN_GOLDEN=1
        shift
        ;;
    *) # handle all other options
        echo "Invalid Argument Provided"
        echo "-------------------------"
        print_usage
        exit 1        
        ;;
    esac
done


echo "VQM2BLIF Program Check"
echo "----------------------"


# start by going through the user supplied directory and identify all the .vqm files. We will recursively go throught the directory and identify these .vqm files. We assume that any file found is going to be a test file.
find $TEST_FOLDER -name "*.vqm" -print0 | while read -d $'\0' vqm_file
do
    # "create_golden" related variables
    CREATE_GOLDEN_FILE_NAME=""
    CREATE_GOLDEN_OUTPUT=""


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

    # if the user chose the "create_golden" option, then we need to generate the file/location names for them
    if [ $GEN_GOLDEN -eq 1 ]; then

        CREATE_GOLDEN_FILE_NAME="${CURR_VQM_FILE_NAME}${VQM_GOLDEN_OUTPUT_EXT}"
        CREATE_GOLDEN_OUTPUT="${CURR_DIR_NAME}/${CREATE_GOLDEN_FILE_NAME}"
  
    fi

    # uncomment for debugging
    #echo "Starting test $CURR_VQM_FILE_NAME"

    # now we need to run the vqm to blif program.
    # we only care about any outputs related to errors, so ignore standard output.
    # We are running this process in a seperate sub shell to make sure that unwanted error messags are not reported in the terminal
    # we have two cases, one for testing and one for generating a golden set of outputs
    if [ $GEN_GOLDEN -eq 0 ]; then

        ($VTR_ROOT_DIR/build/utils/vqm2blif/vqm2blif -vqm $vqm_file -arch $ARCH_FILE -out $VQM_OUTPUT || false) > /dev/null 2>&1
    
    else

        ($VTR_ROOT_DIR/build/utils/vqm2blif/vqm2blif -vqm $vqm_file -arch $ARCH_FILE -out $CREATE_GOLDEN_OUTPUT || false) > /dev/null 2>&1

    fi
    
    # if we errored during the program execution
    if [ $? -ne 0 ]; then

        echo "ERROR: The vqm2blif program failed while converting \"$CURR_VQM_FILE_NAME.vqm\". Please refer to the failure messages below:"
        echo "-----------------------------------------------------------------------------------------------------------------------"

        $VTR_ROOT_DIR/build/utils/vqm2blif/vqm2blif -vqm $vqm_file -arch $ARCH_FILE -out $VQM_OUTPUT

        # once again, we cover the cases of testing and creating a golden set of outputs
        if [ $GEN_GOLDEN -eq 0]; then

            $VTR_ROOT_DIR/build/utils/vqm2blif/vqm2blif -vqm $vqm_file -arch $ARCH_FILE -out $VQM_OUTPUT
        
        else

            $VTR_ROOT_DIR/build/utils/vqm2blif/vqm2blif -vqm $vqm_file -arch $ARCH_FILE -out $CREATE_GOLDEN_OUTPUT 

        fi

        exit 1
    fi

    #uncomment for debugging
    #echo "Comparing $CURR_VQM_FILE_NAME output"

    # variable to keep track of the total number of differences between a generated nelist and its "golden" counterpart
    NUM_OF_DIFFERENCES=0

    # we only need to calculate the number of differences, if we are testing the vqm2blif program
    # this is not needed when we are just generating a golden set of outputs
    if [ $GEN_GOLDEN -eq 0 ]; then
        
        NUM_OF_DIFFERENCES="$(diff $GOLDEN_BLIF_OUTPUT $VQM_OUTPUT | grep "^>" | wc -l)"
    
    fi

    #uncomment for debugging
    #echo "Number of differences: $NUM_OF_DIFFERENCES"

    # by default, the newly created blif file should have a different name, so we should only have 1 difference
    # so below we check to see if there was more than one difference and if there was we throw an error
    if [ $NUM_OF_DIFFERENCES -gt 1 ]; then

        echo "ERROR: The generated blif file \"$TEST_BLIF_FILE_NAME\" did not match the golden reference \"$GOLDEN_BLIF_OUTPUT_FILE_NAME.blif\"."
        echo "The differences are shown below:"
        echo "-------------------------------------------------------------------------------------------------------------------------------------------------------"

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

    # delete the blif files generated if there was an error
    # this is for the case where golden blif files are generated
    find $TEST_FOLDER -name "*$VQM_OUTPUT_EXT" -type f -delete

else

    # if we are here then the test passed or the golden blif netlists were successfully generated, so report to the user (no need to update the test status)

    if [ $GEN_GOLDEN -eq 0 ]; then
        
        # case where we are testing
        echo "VQM2BLIF Checks Successfully Passed!"
    
    else    

        # case where we are generating a set of golden blif outputs
        echo "VQM2BLIF has Successfully created all golden blif netlists!"
    
    fi
fi

# find all the newly created blif files above (used for verification) and delete them (don't need them anymore)
# we only delete the generated blif files if we are testing, when we are generating a new set of golden blif outputs, we do not want to delete them

if [ $GEN_GOLDEN -eq 0 ]; then

    find $TEST_FOLDER -name "*$VQM_OUTPUT_EXT" -type f -delete

fi

# exit with the status of the test (failure=1 and pass=0)
exit $TEST_STATUS


