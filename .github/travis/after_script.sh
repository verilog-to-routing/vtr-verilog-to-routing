#!/bin/bash

source .github/travis/common.sh
set -e

$SPACER

start_section "vtr.ccache.2" "${GREEN}ccache info..${NC}"
ccache --show-stats
end_section "vtr.ccache.2"

$SPACER
