#! /bin/bash

for filename in *.h
do
    echo "$filename" >> t
    echo "" >> t
    echo ".. doxygenfile:: $filename" >> t
    echo "   :project: vpr" >> t
    echo "" >> t
done
