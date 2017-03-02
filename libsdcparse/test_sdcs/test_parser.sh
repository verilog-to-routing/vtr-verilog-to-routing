#!/usr/bin/env bash

if [ -x ./sdcparse_test ]; then
    echo "Found test executable"
else
    echo "Failed to find test executabe"
    exit 1
fi

test_cnt=0
for path in "$@"
do
    sdc_files=`find $path -name '*.sdc' | sort -V`
    for sdc_file in ${sdc_files[@]}
    do
        test_cnt=$((test_cnt + 1))
        echo
        echo "File: $sdc_file"
        #valgrind --leak-check=full --track-origins=yes ./sdcparse_test $sdc_file
        ./sdcparse_test $sdc_file
        exit_code=$?
        if [[ $sdc_file != *"invalid"* ]]; then
            if [ $exit_code -ne 0 ]; then
                    #Should have parsed ok
                    echo "Error: $sdc_file should have parsed correctly!"
                    exit 1
            fi
        else
            if [ $exit_code -eq 0 ]; then
                echo "Error: $sdc_file should have failed to parse!"
                exit 1
            else
                echo "PASS: invalid sdc $sdc_file did not parse (as expected)"
            fi
        fi
    done

done

echo "ALL $test_cnt TESTS PASSED"
exit 0
