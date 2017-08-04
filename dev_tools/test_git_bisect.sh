#!/bin/bash

#This script is useful to find the offending commit using git bisec
#
# Modify the running tool and error check appropriately, then
#
# $ git bisect BAD GOOD
#
#    where BAD is a known bad commit ID, and GOOD a known GOOD commit ID
# 
# then run from the VTR root
#
# $ git bisect run ./dev_tools/test_git_bisect.sh
#
# and git will automatically search for the first bad commit

echo "----------------------"
echo $(git describe --always)

make -j8 odin_II >& make.log

make_res=$?

if [ $make_res -ne 0 ]; then
    #Failed to build
    echo "Failed to Build"
    tail make.log
    exit 125
fi

#Test

pushd test >& /dev/null

#Run the tool
../ODIN_II/odin_II -c odin_config.xml >& odin.log

#Check for the error
diff ref.blif LU8PEEng.odin.blif >& /dev/null

diff_res=$?

if [ $diff_res -eq 0 ]; then
    echo "PASS"
    exit 0
else
    echo "FAIL"
    exit 1 
fi

popd

