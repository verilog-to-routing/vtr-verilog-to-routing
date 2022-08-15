#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

set -x
set -e

export VTR_DIR=$(pwd)
source $SCRIPT_DIR/hostsetup.sh

if ! { [ $VTR_TEST == "vtr_reg_strong" ] || [ $VTR_TEST == "odin_reg_strong" ] \
|| [ $VTR_TEST == "odin_tech_strong" ] || [ $VTR_TEST == "vtr_reg_yosys_odin" ]; }; then
	source $SCRIPT_DIR/vtr-full-setup.sh
fi

# Build VtR
source $SCRIPT_DIR/vtr-build.sh
# Run the reg test.
source $SCRIPT_DIR/vtr-test.sh
