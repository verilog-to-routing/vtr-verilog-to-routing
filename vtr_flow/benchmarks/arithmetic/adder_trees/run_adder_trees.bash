#!/bin/bash
EXP_NAME="adder_trees"

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do
  DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
source "../config.bash" # cd's into EXP_DIR

### end prologe ###

ARCHES="$ALL_ARCHES"

if [ -z "$1" ] || [ "$1" == "dup_sources" ]; then
	pushd .
	
	# duplicate verilog files
	cd $EXP_DIR/verilog
	rm adder_tree_*
	./duplicate_adder_trees.bash  || exit
	echo "verilog files:"
	ls adder_tree_*

	# duplicate architecture files
	cd $ARCH_DIR
	./make_resized_arches.bash 40 40
	echo "architecture files:"
	ls "40x40"

	popd
fi

source "$TASK_AND_PARSE"

if [ -z "$1" ] || [ "$1" == "extract" ]; then
	pushd .
	cd "$ABS_TASK_PATH"

	if [ -z $2 ]; then
		dothisrun="$maxrun"
	else
		dothisrun="$2"
	fi

	echo "reparsing results from run #$dothisrun..."

	cd "run$dothisrun"

	HEADER_SPACING="\t\t\t\t\t"
	COL_DEFS="bits\tlevel\tcrit_path_delay\ttotal_area\t\t\n"

	for arch in $ARCHES; do
		if grep "$arch" parse_results.txt > /dev/null; then
			archresult_file="$EXP_DIR/$arch.result"
			echo "reparsing $arch"
			ARCH_HEADER="$arch$HEADER_SPACING\n"
			ARCH_HEADER+="$COL_DEFS"
			echo "run #$dothisrun" > "$archresult_file.tmp"
			printf "$ARCH_HEADER" >> "$archresult_file.tmp"
				grep "$arch" parse_results.txt | awk '{OFS="\t";print $2,$3,$4,"",""}' >> "$archresult_file.tmp"
			if (( dothisrun >= 9)); then
				sed -r "s/adder_tree_([0-9])L_0*([0-9]*)bits\.v/\\2\t\\1/g" "$archresult_file.tmp" > "$archresult_file"
			else
				# old naming scheme (for before run 9)
				sed -r "s/adder_tree_0*([0-9]*)_([0-9])L_bits\.v/\\1\t\\2/g" "$archresult_file.tmp" > "$archresult_file"
			fi
			rm "$archresult_file.tmp"
			
			for level in {1..16}; do
				result_file="$EXP_DIR/$arch.L${level}.result"
				printf "${level}L with $ARCH_HEADER" > "$result_file"
				if grep -P "[0-9]+\t${level}\t" "$archresult_file" >> "$result_file"; then
					result_files="$result_files $result_file" # add to resultfile set
				else
					rm "$result_file"
				fi
			done
		fi
	done

	echo "recombining..."

	echo "run #$dothisrun" > "$FINAL_RESULTFILE"

	if [ -z "$result_files" ]; then
		echo "no results?"
	else
		paste $result_files >> "$FINAL_RESULTFILE"
	fi

	rm "${EXP_DIR}"/k6_N8_*.xml.L[0-9]*.result

	echo "done reparsing"

	popd
fi

## run log ##
# run#	note
# 1 	initial
# 2 	without routing failure predictor
# 9 	same as #2 but with new naming scheme
# 10 	same as #9 but also with 40x40 FPGAs
# 
