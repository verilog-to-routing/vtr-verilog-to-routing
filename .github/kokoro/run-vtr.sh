#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

set -x
set -e

# Set up the host
ls -l
cd github
ls -l
cd vtr-verilog-to-routing
export VTR_DIR=$( pwd )
source $SCRIPT_DIR/steps/hostsetup.sh
source $SCRIPT_DIR/steps/hostinfo.sh

# Output git information
source $SCRIPT_DIR/steps/git.sh

if [ $VTR_TEST == "vtr_reg_strong" ] || [ $VTR_TEST == "odin_reg_strong" ] \
|| [ $VTR_TEST == "odin_tech_strong" ] || [ $VTR_TEST == "vtr_reg_yosys_odin" ]; then
	source $SCRIPT_DIR/steps/vtr-min-setup.sh
else
	source $SCRIPT_DIR/steps/vtr-full-setup.sh
fi

# Build VtR
source $SCRIPT_DIR/steps/vtr-build.sh
# Run the reg test.
source $SCRIPT_DIR/steps/vtr-test.sh
