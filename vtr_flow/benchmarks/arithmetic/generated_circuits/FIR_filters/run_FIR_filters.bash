#!/bin/bash
EXP_NAME="FIR_filters"

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
	
	# make verilog file
	cd $EXP_DIR/verilog || exit 1
	
	mkdir -p "fir-gen"
	rm fir-gen/fir_*.v

	make generate_fir || exit 1
	for p in pipe nopipe; do
		for i in {10..52}; do
			./generate_fir "$p" "$i" > "fir-gen/fir_${p}_${i}.v"
		done
	done
	
	ls fir-gen/fir_*.v

	# # duplicate architecture files
	# cd $ARCH_DIR
	# ./make_resized_arches.bash 40 40
	# echo "architecture files:"
	# ls "40x40"

	popd
fi

source "$TASK_AND_PARSE"

if [ -z "$1" ] || [ "$1" == "extract" ]; then
	pushd .
	cd "$ABS_TASK_PATH"

	if [ -z "$2" ]; then
		dothisrun="$maxrun"
	else
		dothisrun="$2"
	fi

	echo "reparsing results from run #$dothisrun..."

	cd "run$dothisrun"

	HEADER_SPACING="\t\t\t\t\t"
	EXTRA_SPACING="" # "\t\t\t\t\t\t"
	COL_DEFS="num_coeff\tcrit_path_delay\tnum_clb\tarea_per_clb\tgrid_tile_area\t"

	for arch in $ARCHES; do
		for piped in pipe nopipe; do
			if grep "$arch" parse_results.txt | grep "fir_${piped}" > /dev/null; then
				archresult_file="$EXP_DIR/$piped.$arch.result"
				echo "reparsing $piped $arch"
				ARCH_HEADER="$piped $arch$HEADER_SPACING$EXTRA_SPACING\n"
				ARCH_HEADER+="$COL_DEFS$EXTRA_SPACING\n"
				grid_tile_area=$(grep "grid_logic_tile_area" "$ARCH_DIR/$arch" | grep -oEi "[0-9.]+")
				# echo "run #$dothisrun" > "$archresult_file.tmp"
				printf "$ARCH_HEADER" >> "$archresult_file.tmp"
				grep "$arch" parse_results.txt | grep "fir_${piped}" | awk '{OFS="\t";print $2,$5,$23,$24,'"\"$grid_tile_area\""',""}' >> "$archresult_file.tmp"
				sed -r "s/fir_${piped}_(.*)\.v/\\1/g" "$archresult_file.tmp" > "$archresult_file"
				# cat "$archresult_file.tmp" > "$archresult_file"
				# rm "$archresult_file.tmp"
				
				result_files="$result_files $archresult_file" # add to resultfile set
			fi
		done
	done

	echo "recombining..."

	echo "run #$dothisrun" > "$FINAL_RESULTFILE"

	if [ -z "$result_files" ]; then
		echo "no results?"
	else
		paste $result_files >> "$FINAL_RESULTFILE"
		rm "${EXP_DIR}"/*k6_N8_*.xml.result*
	fi

	echo "done reparsing"

	popd
fi

## run log ##
# run#	note
# 