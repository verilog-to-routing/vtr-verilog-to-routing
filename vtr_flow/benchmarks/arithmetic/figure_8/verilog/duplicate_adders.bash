#!/bin/bash

filename="adder"

for i in {0{0{1..9},{10..99}},{100..160}}; do

	# replace ADDER_BITS with i
	sed "s/ADDER_BITS/$i/g" "$filename.v" > "$filename"_"$i"bits.v
done
