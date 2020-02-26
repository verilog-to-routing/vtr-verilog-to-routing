#!/usr/bin/env bash

THIS_SCRIPT_PATH=$(readlink -f $0)
THIS_DIR=$(dirname ${THIS_SCRIPT_PATH})
REGRESSION_DIR="${THIS_DIR}/../regression_test"
REG_LIB="${REGRESSION_DIR}/.library"

source ${REG_LIB}/conf_generate.sh

if [ "_$1" == "_" ] || [ ! -d "$1" ]
then
    echo "Invalid input $1, expecting a benchmark directory"
else
    echo_bm_conf $1
fi

