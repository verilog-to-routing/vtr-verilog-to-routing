EXP_NAME="figure_8"

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do
  DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
source "../config.bash" # cd's into EXP_DIR

### end prologe ###


if [ -z "$1" ] || [ "$1" == "dup_sources" ]; then
	pushd .
	cd $EXP_DIR/verilog
	rm adder_*
	./duplicate_adders.bash  || exit
	echo "verilog files:"
	ls adder_*

	# duplicate architecture files
	cd $ARCH_DIR
	echo "architecture files:"
	for i in {24..36}; do
		let "w = i/2"
		./make_resized_arches.bash "$w" "$i" "for_fixing_routing_predictor"
	done
	ls "for_fixing_routing_predictor"

	popd

fi

source "$TASK_AND_PARSE"

# ARCHES=""
# ARCHES="k6_N8_gate_boost_0.2V_22nm.xml k6_N8_lookahead_chain_gate_boost_0.2V_22nm_higher_Fs.xml"

if [ -z "$1" ] || [ "$1" == "extract" ]; then
	pushd .
	cd "$ABS_TASK_PATH"

	if [ -z $2 ]; then
		dothisrun="$maxrun"
	else
		dothisrun="$2"
	fi

	echo "extracting results from run #$dothisrun"

	cd "run$dothisrun"

	for arch in *k6_N8*; do
		archresult_file="$EXP_DIR/$arch.result"
		archresult_files="$archresult_files $archresult_file" # add to resultfile set
		echo "extracting $arch"
		printf "$arch\t\t\t\t\nbits\tcrit_path_delay\tminW\tpredictor\t\n" > "$archresult_file.tmp"
		grep "^$arch" parse_results.txt | awk '{OFS="\t";print $2,$5,$4,$25,""}' >> "$archresult_file.tmp"
		sed -r "s/((adder_0*)|(bits\.v))//g" "$archresult_file.tmp" > "$archresult_file"
	done

	cd $EXP_DIR

	echo "run #$dothisrun" > "$FINAL_RESULTFILE"
	
	if [ -z "$archresult_files" ]; then
		echo "no results?"
	else
		paste $archresult_files >> "$FINAL_RESULTFILE"
		rm *k6_N8*.xml.result*
	fi

	echo "done extracting"

	popd
fi

## run log ##
# run#	note
# 11    first retained results
# 12-23	invalid
# 24 	valid but not sure about routing failure predictor, 12x24?
# 25 	predictor is off, 12x24?
# 26 	predictor is on, 12x24
# 27 	predictor is on, VPR determines size
# 30    predictor on, VPR determines size, with xbar architectures
# 32    predictor off, 12x24, with xbar architectures
# 36 	predictor off, 12x24, with soft inter-clb carry chains
#
#       (37 - ?? are for fixing predictor)
# 38    predictor on, 12x24, with soft inter-clb carry chains, with many more circuits

# 40 	predictor on, 12x24, with soft inter-clb carry chains, with mostly larger circuits
# 41 	same as #40 but with predictor off
# 42 	same as #40 but with different circuits
# 43 	same as #41 but with different circuits
# 48 	few circuits, arch is varied from 4x8 to 12x24