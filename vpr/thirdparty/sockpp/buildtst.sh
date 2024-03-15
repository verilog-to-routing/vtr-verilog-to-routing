#!/bin/bash
#
# Test the build with a few compilers.
#
# Local script for continuous integration (CI). This just runs the build 
# using a number of different compilers, then runs the unit tests for 
# each. If the build or unit tests fail, it stops and reports the error.

BUILD_JOBS=4

# Add or remove any compilers here. If any are not found on the local system,
# it reports the missing compiler and continues to the next one.

GCC="g++-5 g++-6 g++-7 g++-8 g++-9"
CLANG="clang++-4.0 clang++-5.0 clang++-6.0 clang++-7 clang++-8"

for COMPILER in ${GCC} ${CLANG}
do
  if [ -z "$(which ${COMPILER})" ]; then
    printf "Compiler not found: %s\n" "${COMPILER}"
  else
    printf "Testing: %s\n" "${COMPILER}"
    rm -rf buildtst/
    mkdir buildtst ; pushd buildtst

    # Configure the build
    if ! cmake -DSOCKPP_BUILD_EXAMPLES=ON -DSOCKPP_BUILD_TESTS=ON -DCMAKE_CXX_COMPILER=${COMPILER} .. ; then
      printf "\nCMake configuration failed for %s\n" "${COMPILER}"
      exit 1
    fi

    # Build the library, examples, and unit tests
    if ! make -j${BUILD_JOBS} ; then
      printf "\nCompilation failed for %s\n" "${COMPILER}"
      exit 2
    fi

    # Run the unit tests
    if ! ./tests/unit/unit_tests ; then
      printf "\nUnit tests failed for %s\n" "${COMPILER}"
      exit 3
    fi

    popd
  fi
  printf "\n"
done

rm -rf buildtst/
exit 0


