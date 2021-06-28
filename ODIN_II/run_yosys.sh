#!/usr/bin/env bash
SHELL=$(type -P bash)

# GLOBAL PATH VARIABLES
THIS_SCRIPT_PATH=$(readlink -f "$0")
THIS_DIR=$(dirname "${THIS_SCRIPT_PATH}")
VTR_DIR=$(readlink -f "${THIS_DIR}/..")
ODIN_DIR="${VTR_DIR}/ODIN_II"
REGRESSION_DIR="${THIS_DIR}/regression_test"

##############################################
# grab the input args
INPUT="$*"

# disable stdin
exec 0<&-

BENCHMARK_DIR="${REGRESSION_DIR}/benchmark"

VTR_REG_DIR="${VTR_DIR}/vtr_flow/tasks/regression_tests"

SUITE_DIR="${BENCHMARK_DIR}/suite"

TASK_DIR="${BENCHMARK_DIR}/task"
BLIF_TASK_DIR="${BENCHMARK_DIR}/task/_BLIF"

RTL_REG_PREFIX="rtl_reg"



# COLORS
BLUE=$'\033[0;33m'
GREEN=$'\033[0;32m'
RED=$'\033[31;1m'
NC=$'\033[0m' # No Color

# defaults
_YOSYS_EXEC="/usr/local/bin/yosys"
_TEST_INPUT_LIST=()
_REGENERATE_BLIF="off"
_CLEAN="off"

###############################################
# Time Helper Functions
function get_current_time() {
	date +%s%3N
}

# needs start time $1
function print_time_since() {
	BEGIN="$1"
	NOW=$(get_current_time)
	TIME_TO_RUN=$(( NOW - BEGIN ))

	Mili=$(( TIME_TO_RUN %1000 ))
	Sec=$(( ( TIME_TO_RUN /1000 ) %60 ))
	Min=$(( ( ( TIME_TO_RUN /1000 ) /60 ) %60 ))
	Hour=$(( ( ( TIME_TO_RUN /1000 ) /60 ) /60 ))

	echo "ran test in: $Hour:$Min:$Sec.$Mili"
}

# print the status of the testbench
function print_test_stat() {
    STAT="$1"
    START_TIME="$2"

    if [ _${STAT} == "_E" ]; then
        echo "[${BLUE}EXIST${NC}] . . . . . . . . . . . . . . . . . . _VERILOG/${TASK_DIR}/${TCL_BLIF_NAME}"
    elif [ _${STAT} == "_C" ]; then
        echo "[${GREEN}CREATED${NC}] . . . . . . . . . . . . . . . . . _VERILOG/${TASK_DIR}/${TCL_BLIF_NAME} - [${GREEN}$(print_time_since "${START_TIME}")${NC}]"
    elif [ _${STAT} == "_F" ]; then
        echo "[${RED}FAILED${NC}]${RED}  . . . . . . . . . . . . . . . . . ${NC}_VERILOG/${TASK_DIR}/${TCL_BLIF_NAME}"
    fi
}

#initialization
function init() {
    export BLIF_PATH="${REGRESSION_DIR}/benchmark/_BLIF"
    export PRIMITIVES="${VTR_DIR}/vtr_flow/primitives.v"
}

# run yosys
function run_yosys() {
    TCL_FILE="$1"
    START=$(get_current_time)

    LOG_FILE="${OUTPUT_BLIF_PATH}/failures/${TCL_BLIF_NAME%.*}.log"

    ${_YOSYS_EXEC} -c ${TCL_FILE} > ${LOG_FILE} 2>&1 #/dev/null 2>&1

    if [ -f "${OUTPUT_BLIF_PATH}/${TCL_BLIF_NAME}"  ]; then
        print_test_stat "C" "${START}"
        rm ${LOG_FILE}
    else
        print_test_stat "F" "${START}"
    fi
}

# Help Print helper
function _prt_cur_arg() {
	arg="[ $1 ]"
	line="\t\t"
	printf "%s%s" "${arg}" "${line:${#arg}}"
}

#to print help of the current script
function help() {
    printf "Called program with $INPUT
        Usage: 
            $0 [ OPTIONS ] [ TEST / SUBTEST_LIST ... ]

        SUBTEST_LIST
            should be a path to a single test or list of the form < task_name >
            passing this in will limit a task to a subset of test
        
    FLAGS
		    --regenerate_blif               $(_prt_cur_arg ${_REGENERATE_BLIF}) regenerate the blifs of verilog files located in benchmark/_BLIF
	
    OPTIONS
            -h|--help                       $(_prt_cur_arg off) print this
            -t|--test < test name >         A path to a single test file
            -T|--task                       Test name is either a absolute or relative path to 
                                            a directory containing a task.ycfg, task_list.conf 

        AVAILABLE_TASK:
    "

    printf "\n\t\t%s\n" "${RELAPATH_SUITE_DIR}/"
    for bm in "${SUITE_DIR}"/*; do printf "\t\t\t%s\n" "$(basename "${bm}")"; done

    printf "\n\t\t%s\n" "${RELAPATH_TASK_DIR}/"
    for bm in "${TASK_DIR}"/*; do printf "\t\t\t%s\n" "$(basename "${bm}")"; done

    printf "\n\t\t%s\n" "${VTR_REG_PREFIX}"
    for bm in "${VTR_REG_DIR}/${VTR_REG_PREFIX}"*; do printf "\t\t\t%s\n" "$(basename "${bm}" | sed "s+${VTR_REG_PREFIX}++g")"; done

    printf "\n\t\t%s\n" "${RTL_REG_PREFIX}"

}

# to check whether failure path exist or not
# if yes clean the failure path from previous logs
function check() {
    FILE_PATH="$1"
    FILE_NAME="$2"

    if [ ! -d "${FILE_PATH}" ]; then
            mkdir -p ${FILE_PATH}
    else    
        if [ "_${_REGENERATE_BLIF}" == "_on" ]; then
            find "${FILE_PATH}" -name "${FILE_NAME}.blif" -delete
        fi
    fi

    FAILURE_PATH="${FILE_PATH}/failures"

    if [ ! -d "${FAILURE_PATH}" ]; then
        mkdir -p ${FAILURE_PATH}
    else    
        find "${FAILURE_PATH}" -name "${FILE_NAME}.log" -delete
    fi
}

# to parse shell arguments
function parse_args() {
	PARSE_SUBTEST="off"
	while [ "_$*" != "_" ]
	do 
		if [ ${PARSE_SUBTEST} == "on" ];
		then
		# parse subtest
			_SUBTEST_LIST+=( "$1" )
			shift
		else
		# parse [ OPTIONS / FLAGS ] 
			case $1 in 

			# Help Desk
				-h|--help)
					echo "Printing Help information"
					help
					_exit_with_code "0"

				
			## directory in benchmark
				;;-t|--test)
					# this is handled down stream
					if [ "_$2" == "_" ]
					then 
						echo "empty argument for $1"
						_exit_with_code "-1"
					fi
					# concat tests
					_TEST_INPUT_LIST+=( "$2" )
					shift

            ;;--regenerate_blif)
					_REGENERATE_BLIF="on"
					echo "regenerating blifs of benchmark/_BLIF"
            
            ;;--clean)
					_CLEAN="on"
					echo "deleting all blif file in the specified task"

			esac

			# keep the subtest in case we caught the end of options and flags
			[ ${PARSE_SUBTEST} != "on" ] && shift
		fi
	done
}

function format_line() {
	echo "$@" \
		| cut -d '#' -f 1	`# trim the # signs` \
		| sed 's/\s+/ /g'	`# trim duplicate whitespace` \
		| sed 's/\s*$//g'	`# trim the tail end whitespace` \
		| sed 's/^\s*//g'	`# trim the front white space`
}

task_name_list=()

function populate_arg_from_file() {

	_circuits_dir=""
	_circuit_list_add=()

    export TASK_PATH=$(dirname "${1}")
    export TASK_DIR=( $(basename "${TASK_PATH}") )

	if [ "_$1" == "_" ] || [ ! -f "$1" ]
	then
		echo "Config file $1 does not exist"
	else
		OLD_IFS=${IFS}
		while IFS="" read -r current_line || [ -n "${current_line}" ]
		do
			formatted_line=$(format_line "${current_line}")

			_key="$(echo "${formatted_line}" | cut -d '=' -f1 )"
			_value="$(echo "${formatted_line}" | cut -d '=' -f2 )"

			if [ "_${_key}" != "_" ] && [ "_${_value}" == "_" ] 
			then
				echo "Specifying empty value for ${_key}, skipping assignment"
			elif [ "_${_key}" == "_" ] && [ "_${_value}" != "_" ] 
			then
				echo "Specifying empty key for value: ${_value}, skipping assignment"
			elif [ "_${_key}" != "_" ] && [ "_${_value}" != "_" ] 
			then
				case _${_key} in

					_circuits_dir)
						if [ ! -d "${_value}" ]
						then
							_value=${THIS_DIR}/${_value}
						fi
						_circuits_dir="${_value}"

					;;_circuit_list_add)
						# glob the value
						_circuit_list_add+=( "${_circuits_dir}"/${_value} )					

					;;_)
						echo "skip" > /dev/null

					;;*)
						echo "Unsupported value: ${_key} ${_value}, skipping"

				esac
			fi
		done < "$1"
		IFS=${OLD_IFS}
	fi

	
	for circuit_list_item in "${_circuit_list_add[@]}"
	do
		if [ ! -f "${circuit_list_item}" ]
		then
			echo "file ${circuit_list_item} not found, skipping"
		else
			_circuit_list+=( "${circuit_list_item}" )
		fi
	done

	if [ "_${#_circuit_list[*]}" == "_" ]
	then
		echo "Passed a config file with no circuit to test"
		_exit_with_code "-1"
	fi
}

function run_task() {
    directory="$1"
    if [ "_${_CLEAN}" == "_on" ]; then
        find "${ODIN_DIR}/${directory/task\/}" -name "*.blif" -delete
    fi

    populate_arg_from_file "${directory}/task.ycfg"
   
    for circuit in "${_circuit_list[@]}"
	do
        CIRCUIT_DIR=${circuit/regression_test\/benchmark\/_VERILOG\/}
        CIRCUIT_FILE=$(basename "${circuit}")
                            
        RELAPATH_TASK_DIR=$(realpath --relative-to="${BLIF_TASK_DIR}" "${TASK_PATH}")
        export OUTPUT_BLIF_PATH="${BLIF_PATH}/${RELAPATH_TASK_DIR}"

        # run yosys for the current circuit
        CIRCUIT_NAME="${CIRCUIT_FILE%.*}"

        # to check the required path and files
        check "${OUTPUT_BLIF_PATH}" "${CIRCUIT_NAME}"

        export TCL_CIRCUIT="${circuit}"
        export TCL_BLIF_NAME="${CIRCUIT_NAME}.blif"

        if [ -f "${OUTPUT_BLIF_PATH}/${TCL_BLIF_NAME}" ]; then
            print_test_stat "E"
            continue
        fi
        

        run_yosys "${ODIN_DIR}/synth.tcl"

        unset _circuit_list

    done

}

task_list=()

function run_suite() {
	current_test_list=( "${_TEST_INPUT_LIST[@]}" )
	while [ "_${current_test_list[*]}" != "_" ]
	do
		current_input="${current_test_list[0]}"
		current_test_list=( "${current_test_list[@]:1}" )

		# try globbing
        current_input_list=( ${current_input} )
        if [ ! -d "${current_input_list[0]}" ]
        then
            # try globbing with absolute
            current_input_list=( "${THIS_DIR}/"${current_input} )
        fi
        # bash expand when possible
        for possible_test in "${current_input_list[@]}"
        do
            if [ ! -d "${possible_test}" ]
            then
                echo "no such Directory for task: ${possible_test}"
            else
                if [ -f "${possible_test}/task_list.conf" ]
                then
                    mapfile -t test_list <"${possible_test}/task_list.conf"
                    current_test_list+=( "${test_list[@]}" )
                    _TEST_INPUT_LIST+=( "${test_list[@]}" )
                elif [ -f "${possible_test}/task.ycfg" ]
                then
                    task_list+=( "${possible_test}" )
                else
                    echo "Invalid Directory for task: ${possible_test}"
                fi
            fi
        done
				
	done

	TEST_COUNT="0"
	for (( i = 0; i < ${#task_list[@]}; i++ ));
	do
        echo ""
        echo "Task: $(basename ${task_list[$i]})"
        run_task "${task_list[$i]}"
		TEST_COUNT=$(( TEST_COUNT + 1 ))
	done

	if [ "_${TEST_COUNT}" == "_0" ];
	then
		echo "No test is passed in must pass a test directory containing either a task_list.conf or a task.ycfg, see --help"
		_exit_with_code "-1"
	fi
}


##############################################################################################
######################################### START HERE #########################################
##############################################################################################

init

parse_args "$@"

run_suite