#!/bin/bash

clean=$(git status -s -uno | wc -l) #Short ignore untracked

if [ $clean -ne 0 ]; then
    echo "Current working tree was not clean! This tool only works on clean checkouts"
    exit 2
else
    echo "Code Formatting Check"
    echo "====================="
    make format > /dev/null

    valid_format=$(git diff | wc -l)

    if [ $valid_format -ne 0 ]; then
        echo "FAILED"
        echo ""
        echo "You *must* make the following changes to match the formatting style"
        echo "-------------------------------------------------------------------"
        echo ""

        git diff

        echo ""
        echo "Run 'make format' to apply these changes"

        git reset --hard > /dev/null
        exit 1
    else
        echo "OK"
    fi
fi
exit 0
