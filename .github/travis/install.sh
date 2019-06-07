#!/bin/bash

source .github/travis/common.sh
set -e

# Git repo fixup
start_section "environment.git" "Setting up ${YELLOW}git checkout${NC}"
set -x
git fetch --tags
git submodule update --recursive --init
git submodule foreach git submodule update --recursive --init
set +x
end_section "environment.git"

$SPACER

