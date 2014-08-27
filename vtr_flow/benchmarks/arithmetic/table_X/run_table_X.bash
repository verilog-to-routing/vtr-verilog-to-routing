EXP_NAME="table_X"

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

source "$TASK_AND_PARSE"

if [ -z "$1" ] || [ "$1" == "extract" ]; then
	pushd .
	cd "$ABS_TASK_PATH"

	if [ -z "$2" ]; then
		dothisrun=-1
		for rundir in run*; do
			runnum=${rundir:3}
			if [ $runnum -gt $dothisrun ]; then
				dothisrun=$runnum
			fi
		done
	else
		dothisrun="$2"
	fi

	echo "reparsing results from run #$dothisrun"

	cd "run$dothisrun"

	for arch in $ARCHES; do
		if grep "$arch" parse_results.txt > /dev/null; then
			archresult_file="$EXP_DIR/$arch.result"
			archresult_files="$archresult_files $archresult_file" # add to resultfile set
			echo "reparsing $arch"
			printf "$arch\t\t\t\t\t\n"                             > "$archresult_file.tmp"
			printf "circuit\tcrit_path_delay\tnum_clb\tarea_per_clb\t\t\n" >> "$archresult_file.tmp"
			grep "$arch" parse_results.txt | awk '{OFS="\t";print $2,$3,$4,$5,"",""}' >> "$archresult_file.tmp"
			sed -r "s/\.v//g" "$archresult_file.tmp" > "$archresult_file"
			rm "$archresult_file.tmp"
		fi
	done

	cd $EXP_DIR

	echo "run #$dothisrun" > "$FINAL_RESULTFILE"
	paste $archresult_files >> "$FINAL_RESULTFILE"

	echo "done reparsing"

	popd
fi