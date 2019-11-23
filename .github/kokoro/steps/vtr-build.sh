#!/bin/bash

export FAILURE=0
make -k CMAKE_PARAMS="-DVTR_ASSERT_LEVEL=3" -j$MAX_CORES || export FAILURE=1

echo
echo
echo
echo

# When the build fails, produce the failure output in a clear way
if [ $FAILURE -ne 0 ]; then
        make CMAKE_PARAMS="-DVTR_ASSERT_LEVEL=3" -j1
        exit 1
fi
