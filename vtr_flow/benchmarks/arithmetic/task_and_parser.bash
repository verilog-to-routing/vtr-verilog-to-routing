
pushd .
cd "$ABS_TASK_PATH"

maxrun=1
if stat -t glob* >/dev/null 2>&1; then
	for rundir in run*; do
		runnum=${rundir:3}
		if (( runnum > maxrun )); then
			maxrun=$runnum
		fi
	done
fi

popd

if [ -z "$1" ] || [ "$1" == "task" ]; then
	pushd .
	"$VTR_SCRIPTS_DIR/run_vtr_task.pl" "$TASK_PATH" -p 4 || exit 1

	pushd .
	cd "$ABS_TASK_PATH"
	maxrun=-1
	for rundir in run*; do
		runnum=${rundir:3}
		if (( runnum > maxrun )); then
			maxrun=$runnum
		fi
	done
	popd

	# archive config file with data
	cp -r "$ABS_TASK_PATH/config" "$ABS_TASK_PATH/run${maxrun}/"

	popd
fi

if [ -z "$1" ] || [ "$1" == "parse" ]; then
	pushd .
	echo "running parser"
	"$VTR_SCRIPTS_DIR/parse_vtr_task.pl" "$TASK_PATH" || exit 1
	echo "parser done"
	popd
fi
