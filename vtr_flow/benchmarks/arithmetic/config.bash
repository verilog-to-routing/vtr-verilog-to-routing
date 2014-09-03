# The root of your vtr repository
VTR_SVN_DIR="root of your vtr repository"
if [[ $VTR_SVN_DIR == /* ]]; then
	printf ""
else
	echo "config.bash: Set VTR_SVN_DIR to a directory please."
	exit 1;
fi

FLOW_DIR="${VTR_SVN_DIR}/vtr_flow"
VERILOG_PREPROCESSOR="${VTR_SVN_DIR}/verilog_preprocessor/verilog_preprocessor"

# the root of the carry chains experiments directory
ROOT_OF_CCE="${FLOW_DIR}/benchmarks/arithmetic"

TASK_AND_PARSE="$ROOT_OF_CCE"/task_and_parser.bash

ARCH_DIR="$ROOT_OF_CCE/arch"

source $ARCH_DIR/arches.bash

VTR_SCRIPTS_DIR=$FLOW_DIR/scripts

TASKS_DIR="${FLOW_DIR}/tasks"
TASK_PATH="arithmetic_tasks/${EXP_NAME}"
ABS_TASK_PATH="$TASKS_DIR/$TASK_PATH"

EXP_DIR="$ROOT_OF_CCE"/"$EXP_NAME"
FINAL_RESULTFILE="$EXP_DIR/$EXP_NAME"_final_results.tsv

alias popd="popd > /dev/null"
alias pushd="pushd > /dev/null"

shopt -s expand_aliases

cd "$EXP_DIR"
