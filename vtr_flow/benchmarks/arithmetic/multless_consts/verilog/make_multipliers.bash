cd firgen || exit 1

for i in {00{1..9},0{10..99},{100..128}}; do
	constant=$RANDOM
	let "constant %= 1024*1024"
	filename="../mult_${i}.v"
	./multBlockGen.pl "$constant" -fractionalBits 0 > $filename
	cat "../surround_with_regs.v" >> $filename
done
