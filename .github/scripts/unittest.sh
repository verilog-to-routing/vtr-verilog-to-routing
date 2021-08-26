#!/bin/bash

set -e

source $(dirname "$0")/common.sh

$(dirname "$0")/build.sh

$SPACER

start_section "vtr.test.0" "${GREEN}Testing..${NC} ${CYAN}C++ unit tests${NC}"
make test
end_section "vtr.test.0"

$SPACER
