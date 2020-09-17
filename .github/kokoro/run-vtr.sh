#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

set -x
set -e

# Set up the host
ls -l
cd github
ls -l
cd vtr-verilog-to-routing
source $SCRIPT_DIR/steps/hostsetup.sh
source $SCRIPT_DIR/steps/hostinfo.sh

# Output git information
source $SCRIPT_DIR/steps/git.sh

if [ $VTR_TEST == "vtr_reg_strong" ]; then
	source $SCRIPT_DIR/steps/vtr-min-setup.sh
else
	source $SCRIPT_DIR/steps/vtr-full-setup.sh
fi

# Build VtR
source $SCRIPT_DIR/steps/vtr-build.sh
# Run the reg test.
source $SCRIPT_DIR/steps/vtr-test.sh
