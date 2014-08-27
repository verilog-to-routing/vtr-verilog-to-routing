arch=""

echo "# F_c"$'\t'"num_clb"$'\t'"min W"$'\t'"totwire"$'\t'"crit path(ns)"

# for file in mwalker_arch-with_bypass{.xml,_x2_v1.xml{,.0{1..9},.10}}/$1.v/vpr_stdout.log; do
for i in {0{1..9}{0,5},100}; do
	folder=$arch.$i

	if [[ ! -d $(pwd)/$folder ]]; then
		# echo "skipping $(pwd)/$folder"
		continue
	fi

	file=$folder/$1.v/vpr_stdout.log;
	# echo -n "$file "
	echo -n ${i:0:1}.${i#?}$'\t'
	grep "Architecture[ \t]*[0-9].*[ \t]*blocks of type: clb" $file \
		| cut -f2 \
		| cut -f2 --delimiter=' ' \
		| head -c -1;
	echo -n $'\t'

	grep "with a channel width factor" $file \
		| cut -f10 --delimiter=' ' \
		| head -c -2;

	echo -n $'\t'

	grep -i "total wirelength" $file \
		| cut -f3 --delimiter=' ' \
		| head -c -2

	echo -n $'\t'

	grep -i "final critical path" $file \
		| cut -f4 --delimiter=' ' \
		| head -c -1

	echo -n $'\n'
done