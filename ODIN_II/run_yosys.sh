#!/usr/bin/env bash

# GLOBAL PATH VARIABLES
THIS_SCRIPT_PATH=$(readlink -f "$0")
THIS_DIR=$(dirname "${THIS_SCRIPT_PATH}")
VTR_DIR=$(readlink -f "${THIS_DIR}/..")
ODIN_DIR="${VTR_DIR}/ODIN_II"
REGRESSION_DIR="${THIS_DIR}/regression_test"

# COLORS
RED=$'\033[31;1m'
GREEN=$'\033[0;32m'
NC=$'\033[0m' # No Color

YOSYS_EXEC="yosys"

###############################################
# Time Helper Functions
function get_current_time() {
	date +%s%3N
}

# needs start time $1
function print_time_since() {
	BEGIN="$1"
	NOW=$(get_current_time)
	TIME_TO_RUN=$(( NOW - BEGIN ))

	Mili=$(( TIME_TO_RUN %1000 ))
	Sec=$(( ( TIME_TO_RUN /1000 ) %60 ))
	Min=$(( ( ( TIME_TO_RUN /1000 ) /60 ) %60 ))
	Hour=$(( ( ( TIME_TO_RUN /1000 ) /60 ) /60 ))

	echo "ran test in: $Hour:$Min:$Sec.$Mili"
}

# print the status of the testbench
function print_test_stat() {
    STAT="$1"
    START_TIME="$2"

    if [ _${STAT} == "_E" ]; then
        echo "[${RED}EXISTED${NC}] .................................... ${BLIF_NAME}"
    elif [ _${STAT} == "_C" ]; then
        echo "[${GREEN}CREATED${NC}] .................................... ${BLIF_NAME} - [${GREEN}$(print_time_since "${START_TIME}")${NC}]"
    fi
}

# run yosys
function run_yosys() {
    TCL_FILE="$1"
    START=$(get_current_time)

    ${YOSYS_EXEC} -c ${TCL_FILE} > /dev/null 2>&1

    print_test_stat "C" "${START}"
}


##############################################################################################
######################################### START HERE #########################################
##############################################################################################

# ITERATING THROUGH VERILOG FILES TO CREATE THEIR BLIF FILE USING YOSYS
TESTS_PATH="${REGRESSION_DIR}/benchmark/blif/_VERILOGS/*"
export BLIF_PATH="${REGRESSION_DIR}/benchmark/blif"

for directory in ${TESTS_PATH}
do 
    DIR_NAME=$(basename "${directory}")

    VERILOGS_PATH="${directory}/*.v"
    export OUTPUT_BLIF_PATH="${BLIF_PATH}/${DIR_NAME}"

    mkdir -p ${OUTPUT_BLIF_PATH}
    
    for file_path in ${VERILOGS_PATH}
    do
        FILE_NAME="$(basename ${file_path})"
        FILE_NAME="${FILE_NAME%.*}"

        export FILE_PATH="${file_path}"
        export BLIF_NAME="${FILE_NAME}.blif"

        if [ -f "${OUTPUT_BLIF_PATH}/${BLIF_NAME}" ]; then
            print_test_stat "E"
            continue
        fi
        
        run_yosys "${ODIN_DIR}/synth.tcl"

    done
done

