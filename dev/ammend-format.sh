#!/usr/bin/env bash
#Reformats and then amends the currently checked-out commit
#
#Usually used by rebase-format.sh

#Re-format the code
make format

#Update the commit to include the formatting
git commit -a --amend --no-edit
