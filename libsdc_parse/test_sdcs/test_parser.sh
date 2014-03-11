#!/usr/bin/env bash

for path in "$@"
do
    sdc_files=`find $path -name '*.sdc'`
    for sdc_file in ${sdc_files[@]}
    do
        echo
        echo "File: $sdc_file"
        ./sdc_parse_test $sdc_file
    done

done
