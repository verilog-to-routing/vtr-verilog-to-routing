#!/bin/bash

source .github/travis/common.sh
set -e

$SPACER

start_section "vtr.test.0" "${GREEN}Testing..${NC} ${CYAN}C++ unit tests${NC}"
make test
end_section "vtr.test.0"

$SPACER
