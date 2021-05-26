#!/bin/sh

# GLOBAL PATH VARIABLES
THIS_SCRIPT_PATH=$(readlink -f "$0")
THIS_DIR=$(dirname "${THIS_SCRIPT_PATH}")
VTR_DIR=$(readlink -f "${THIS_DIR}/..")
ODIN_DIR="${VTR_DIR}/ODIN_II"
REGRESSION_DIR="${THIS_DIR}/regression_test"

# COLORS
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color


# ITERATING THROUGH VERILOG FILES TO CREATE THEIR BLIF FILE USING YOSYS
VERILOGS_PATH="${REGRESSION_DIR}/benchmark/blif/verilogs/*.v"
export OUTPUT_BLIF_PATH="${REGRESSION_DIR}/benchmark/blif"
for file_path in ${VERILOGS_PATH}
do
    FILE_NAME="$(basename ${file_path})"
    FILE_NAME="${FILE_NAME%.*}"

    export FILE_PATH="${file_path}"
    export BLIF_NAME="${FILE_NAME}.blif"

    if [ -f "${OUTPUT_BLIF_PATH}/${BLIF_NAME}" ]; then
        echo "[${RED}EXISTED${NC}] .................................... ${BLIF_NAME}"
        continue
    fi
    
    yosys -c ${ODIN_DIR}/synth.tcl > /dev/null 2>&1

    echo "[${GREEN}CREATED${NC}] .................................... ${BLIF_NAME}"
done