#!/bin/bash
EXP_NAME="open_cores"

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
	
	# preproces verilog files
	cd $EXP_DIR/verilog/processed_files || exit 1

	VPP_OUT_DIR="output"
	VPP_IN_DIR="input"
	mkdir -p "${VPP_OUT_DIR}"

	"${VERILOG_PREPROCESSOR}" -DITERATE < "${VPP_IN_DIR}/cordic.vv"           > "${VPP_OUT_DIR}/cordic.v"
	"${VERILOG_PREPROCESSOR}"           < "${VPP_IN_DIR}/Md5Core.vv"          > "${VPP_OUT_DIR}/Md5Core.v"
	"${VERILOG_PREPROCESSOR}"           < "${VPP_IN_DIR}/rc4.vv"              > "${VPP_OUT_DIR}/rc4.v"
	"${VERILOG_PREPROCESSOR}"           < "${VPP_IN_DIR}/pipelined_fft_64.vv" > "${VPP_OUT_DIR}/pipelined_fft_64.v"

	echo ""
	echo "verilog files:"
	ls "${VPP_OUT_DIR}"

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
	EXTRA_SPACING="" # "$HEADER_SPACING\t"
	COL_DEFS="circuit\tcrit_path_delay\tnum_clb\tarea_per_clb\tgrid_tile_area\t"

	for arch in $ARCHES; do
		if grep "$arch" parse_results.txt > /dev/null; then
			archresult_file="$EXP_DIR/$arch.result"
			echo "reparsing $arch"
			ARCH_HEADER="$arch$HEADER_SPACING$EXTRA_SPACING\n"
			ARCH_HEADER+="$COL_DEFS$EXTRA_SPACING\n"
			grid_tile_area=$(grep "grid_logic_tile_area" "$ARCH_DIR/$arch" | grep -oEi "[0-9.]+")
			# echo "run #$dothisrun" > "$archresult_file.tmp"
			printf "$ARCH_HEADER" > "$archresult_file.tmp"
			grep "$arch" parse_results.txt | awk '{OFS="\t";print $2,$5,$23,$24,'"\"$grid_tile_area\""',""}' >> "$archresult_file.tmp"
			# sed -r "s///g" "$archresult_file.tmp" > "$archresult_file"
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
		rm "${EXP_DIR}"/*k6_N8_*.xml.result*
	fi

	echo "done reparsing"

	popd
fi

## run log ##
# run#	note
# 