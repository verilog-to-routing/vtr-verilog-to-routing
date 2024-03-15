#!/bin/bash
#
# travis_build.sh
#
# Travis CI build/test script for the sockpp library.
#

set -e

echo "travis build dir $TRAVIS_BUILD_DIR pwd $PWD"
cmake -Bbuild -H. -DSOCKPP_BUILD_EXAMPLES=ON -DSOCKPP_BUILD_TESTS=ON
cmake --build build/

# Run the unit tests
./build/tests/unit/unit_tests --success

#ctest -VV --timeout 600
#cpack --verbose

