#!/bin/bash

filename="adder_tree"

for i in {2,3}; do 

	for j in {0{0{4..9},{10..24},28,32,48,64,96},128}; do
		# replace %%LEVELS_OF_ADDER%% with i
		# and %%ADDER_BITS%% with j
		sed \
			-e "s/%%LEVELS_OF_ADDER%%/$i/g" \
			-e "s/%%ADDER_BITS%%/$j/g" \
		"$filename.v" > "${filename}_${i}L_${j}bits.v"
	done
done
