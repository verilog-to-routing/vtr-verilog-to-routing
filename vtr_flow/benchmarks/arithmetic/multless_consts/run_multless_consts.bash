#!/bin/bash
EXP_NAME="multless_consts"

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
	cd $EXP_DIR/verilog || exit 1
	rm mult_*
	./make_multipliers.bash || exit 1
	echo "verilog files:"
	ls mult_*
	echo -e "\n"

	# # duplicate architecture files
	cd $ARCH_DIR
	./make_resized_arches.bash 12 24
	echo "architecture files:"
	ls "12x24"

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
	EXTRA_SPACING="\t\t\t\t\t\t"
	COL_DEFS="circuit\tcrit_path_delay\tarea_per_clb\tnum_clb\tgrid_tile_area\t"

	for arch in $ARCHES; do
		if grep "$arch" parse_results.txt > /dev/null; then
			archresult_file="$EXP_DIR/$arch.result"
			echo "reparsing $arch"
			ARCH_HEADER="$arch$HEADER_SPACING$EXTRA_SPACING\n"
			ARCH_HEADER+="$COL_DEFS$EXTRA_SPACING\n"
			grid_tile_area=$(grep "grid_logic_tile_area" "$ARCH_DIR/$arch" | grep -oEi "[0-9.]+")
			# echo "run #$dothisrun" > "$archresult_file.tmp"
			printf "$ARCH_HEADER" >> "$archresult_file.tmp"
			grep "$arch" parse_results.txt | awk '{OFS="\t";print $2,$3,$5,$4,'"\"$grid_tile_area\""',"", "","","","","",""}' >> "$archresult_file.tmp"
			# sed -r "s/adder_tree_0*([0-9]*)_([0-9])L_bits\.v/\\1\t\\2/g" "$archresult_file.tmp" > "$archresult_file"
			# sed -r "s/adder_tree_([0-9])L_0*([0-9]*)bits\.v/\\2\t\\1/g" "$archresult_file.tmp" > "$archresult_file"
			cat "$archresult_file.tmp" > "$archresult_file"
			# rm "$archresult_file.tmp"
			
			result_files="$result_files $archresult_file" # add to resultfile set
		fi
	done

	echo "recombining..."

	echo "run #$dothisrun" > "$FINAL_RESULTFILE"

	if [ -z "$result_files" ]; then
		echo "no results?"
	else
		paste $result_files >> "$FINAL_RESULTFILE"
		rm "${EXP_DIR}"/k6_N8_*.xml.result*
	fi

	echo "done reparsing"

	popd
fi

## run log ##
# run#	note
#  7 	16x, %1024, 12x24, not registered
#  9 	48x, %1024, 12x24, not registered
# 10 	128x, %1024, 12x24, not registered
# 13 	128x, %1024^2, 12x24, registered.