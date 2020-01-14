#!/usr/bin/env bash
# moves the newest file matching expression recursively from specified directory to destination
# mover.sh EXPRESSION SEARCH_FROM DESTINATION
# Author: Johnson Zhong
hits=$(find $2 -wholename "$1")

if [ -z "$hits" ]; then
    exit 1
fi

inorder=$(ls -t $hits)
newest=$(echo $inorder | head -n1 | cut -d " " -f1)
mv $newest $3
