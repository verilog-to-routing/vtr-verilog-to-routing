#!/bin/bash

source .github/travis/common.sh
set -e

# Close the after_success.1 fold travis has created already.
travis_time_finish
travis_fold end after_failure.1

start_section "failure.tail" "${RED}Failure output...${NC}"
tail -n 1000 output.log
end_section "failure.tail"
