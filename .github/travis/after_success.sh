#!/bin/bash

source .github/travis/common.sh
set -e

# Close the after_success fold travis has created already.
travis_fold end after_success

