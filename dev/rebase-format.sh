#!/usr/bin/env bash

#Reformat and amend each commit since the provided 
#commit-sh argument
#
#NOTE: Only use on local branches!
git rebase -i --exec dev/ammend-format.sh $1
