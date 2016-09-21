#!/usr/bin/env bash

for path in "$@"
do
    sdc_files=`find $path -name '*.sdc'`
    for sdc_file in ${sdc_files[@]}
    do
        echo
        echo "File: $sdc_file"
        #valgrind --leak-check=full --track-origins=yes ./sdcparse_test $sdc_file
        ./sdcparse_test $sdc_file
        exit_code=$?
        if [ $exit_code -ne 0 ]; then
            echo "Error"
            exit 1
        fi
    done

done
