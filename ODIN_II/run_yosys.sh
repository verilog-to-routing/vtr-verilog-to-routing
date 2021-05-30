#!/usr/bin/env bash

# GLOBAL PATH VARIABLES
THIS_SCRIPT_PATH=$(readlink -f "$0")
THIS_DIR=$(dirname "${THIS_SCRIPT_PATH}")
VTR_DIR=$(readlink -f "${THIS_DIR}/..")
ODIN_DIR="${VTR_DIR}/ODIN_II"
REGRESSION_DIR="${THIS_DIR}/regression_test"

# COLORS
RED=$'\033[31;1m'
GREEN=$'\033[0;32m'
NC=$'\033[0m' # No Color

# defaults
_YOSYS_EXEC="/usr/local/bin/yosys"
_TEST_INPUT_LIST=()

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
        echo "[${RED}EXISTED${NC}] . . . . . . . . . . . . . . . . . ${BLIF_NAME}"
    elif [ _${STAT} == "_C" ]; then
        echo "[${GREEN}CREATED${NC}] . . . . . . . . . . . . . . . . . ${BLIF_NAME} - [${GREEN}$(print_time_since "${START_TIME}")${NC}]"
    fi
}

#initialization
function init() {
    export BLIF_PATH="${REGRESSION_DIR}/benchmark/blif"
}

# run yosys
function run_yosys() {
    TCL_FILE="$1"
    START=$(get_current_time)

    ${_YOSYS_EXEC} -c ${TCL_FILE} > /dev/null 2>&1

    print_test_stat "C" "${START}"
}

#to print help of the current script
function help() {
    printf "Called program with $INPUT
        Usage: 
            $0 [ OPTIONS / FLAGS ] [ SUBTEST_LIST ... ]

        SUBTEST_LIST
            should be a list of the form < task_name/test_file_name/architecture_file_name >
            passing this in will limit a task to a subset of test
            current: $(_prt_cur_arg ${_SUBTEST_LIST[*]})

        FLAGS
            -g|--generate_bench             $(_prt_cur_arg ${_GENERATE_BENCH}) Generate input and output vector for test
            -o|--generate_output            $(_prt_cur_arg ${_GENERATE_OUTPUT}) Generate output vector for test given its input vector
            -b|--build_config               $(_prt_cur_arg ${_GENERATE_CONFIG}) Generate a config file for a given directory
            -c|--clean                      $(_prt_cur_arg off ) Clean temporary directory
            -f|--force_simulate             $(_prt_cur_arg ${_FORCE_SIM}) Force the simulation to be executed regardless of the config
            --override						$(_prt_cur_arg ${_OVERRIDE_CONFIG}) if a config file is passed in, override arguments rather than append
            --dry_run                       $(_prt_cur_arg ${_DRY_RUN}) performs a dry run to check the validity of the task and flow 
            --randomize                     $(_prt_cur_arg ${_RANDOM_DRY_RUN}) performs a dry run randomly to check the validity of the task and flow 
            --regenerate_expectation        $(_prt_cur_arg ${_REGENERATE_EXPECTATION}) regenerate the expectation and overrides the expected value mismatches only
            --generate_expectation          $(_prt_cur_arg ${_GENERATE_EXPECTATION}) generate the expectation and overrides the expectation file
            --continue						$(_prt_cur_arg ${_CONTINUE}) continue running test in the same directory as the last run
            --no_report						printing report is: $(_prt_cur_arg ${_REPORT})
            --status_only					$(_prt_cur_arg ${_STATUS_ONLY}) print the report and exit
        OPTIONS
            -h|--help                       $(_prt_cur_arg off) print this
            -j|--nb_of_process < N >        $(_prt_cur_arg ${_NUMBER_OF_PROCESS}) Number of process requested to be used
            -d|--output_dir < /abs/path >   $(_prt_cur_arg ${_RUN_DIR_OVERRIDE}) Change the run directory output
            -C|--config <path/to/config>	$(_prt_cur_arg ${_EXTRA_CONFIG}) Add a config file to append to the config for the tests
            -t|--test < test name >         $(_prt_cur_arg ${_TEST_INPUT_LIST[*]}) Test name is either a absolute or relative path to 
                                                                a directory containing a task.conf, task_list.conf 
                                                                (see CONFIG FILE HELP) or one of the following predefined test

        AVAILABLE_TEST:
    "

    # printf "\n\t\t%s\n" "${RELAPATH_SUITE_DIR}/"
    # for bm in "${SUITE_DIR}"/*; do printf "\t\t\t%s\n" "$(basename "${bm}")"; done

    # printf "\n\t\t%s\n" "${RELAPATH_TASK_DIR}/"
    # for bm in "${TASK_DIR}"/*; do printf "\t\t\t%s\n" "$(basename "${bm}")"; done

    # printf "\n\t\t%s\n" "${VTR_REG_PREFIX}"
    # for bm in "${VTR_REG_DIR}/${VTR_REG_PREFIX}"*; do printf "\t\t\t%s\n" "$(basename "${bm}" | sed "s+${VTR_REG_PREFIX}++g")"; done

    # printf "\n\t\t%s\n" "${RTL_REG_PREFIX}"

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

			# 	;;-d|--output_dir)

			# 		if [ "_$2" == "_" ]
			# 		then 
			# 			echo "empty argument for $1"
			# 			_exit_with_code "-1"
			# 		fi
					
			# 		_RUN_DIR_OVERRIDE=$2

			# 		if [ ! -d "${_RUN_DIR_OVERRIDE}" ]
			# 		then
			# 			echo "Directory ${_RUN_DIR_OVERRIDE} does not exist"
			# 			_exit_with_code "-1"
			# 		fi

			# 		shift

			# 	;;-C|--config)

			# 		if [ "_$2" == "_" ]
			# 		then 
			# 			echo "empty argument for $1"
			# 			_exit_with_code "-1"
			# 		fi
					
			# 		_EXTRA_CONFIG=$2
			# 		echo "Reading extra config directive from ${_EXTRA_CONFIG}"

			# 		shift

			# ## number
			# 	;;-j|--nb_of_process)
			# 		_NUMBER_OF_PROCESS=$(_flag_is_number "$1" "$2")
			# 		echo "Using [$2] processors for this benchmarking suite"
			# 		shift

			# # Boolean flags
			# 	;;-g|--generate_bench)		
			# 		_GENERATE_BENCH="on"
			# 		echo "generating output vector for test given predefined input"

			# 	;;-o|--generate_output)		
			# 		_GENERATE_OUTPUT="on"
			# 		echo "generating input and output vector for test"

			# 	;;-b|--build_config)		
			# 		_GENERATE_CONFIG="on"
			# 		echo "generating a config file for test directory"

			# 	;;-c|--clean)				
			# 		echo "Cleaning temporary run in directory"
			# 		cleanup_temp

			# 	;;-f|--force_simulate)   
			# 		_FORCE_SIM="on"
			# 		echo "Forcing Simulation"   

			# 	;;--override)
			# 		_OVERRIDE_CONFIG="on"
			# 		echo "Forcing override of config"    

			# 	;;--dry_run)
			# 		_DRY_RUN="on"
			# 		echo "Performing a dry run"

			# 	;;--randomize)
			# 		_RANDOM_DRY_RUN="on"
			# 		echo "random dry run"

			# 	;;--regenerate_expectation)
			# 		_REGENERATE_EXPECTATION="on"
			# 		echo "regenerating expected values for changes outside the defined ranges"

			# 	;;--generate_expectation)
			# 		_GENERATE_EXPECTATION="on"
			# 		echo "generating new expected values"

			# 	;;--no_report)
			# 		_REPORT="off"
			# 		echo "don't generate the report at the end"

			# 	;;--continue)
			# 		_CONTINUE="on"
			# 		echo "run this test in the same directory as the previous test"

			# 	;;--status_only)
			# 		_STATUS_ONLY="on"
			# 		_CONTINUE="on"
			# 		echo "print the previous test report"

			# 	;;*) 
			# 		PARSE_SUBTEST="on"
			esac

			# keep the subtest in case we caught the end of options and flags
			[ ${PARSE_SUBTEST} != "on" ] && shift
		fi
	done
}

function run_task() {
    directory="$1"
    DIR_NAME=$(basename "${directory}")

    RELATIVE_PATH="${directory/regression_test\/benchmark\/task\/blif}"
    VERILOGS_PATH="${BLIF_PATH}/_VERILOGS/${RELATIVE_PATH}/*.v"
    export OUTPUT_BLIF_PATH="${BLIF_PATH}/${DIR_NAME}"

    mkdir -p ${OUTPUT_BLIF_PATH}
    
    for file_path in ${VERILOGS_PATH}
    do
        FILE="$(basename ${file_path})"
        FILE_NAME="${FILE%.*}"

        export FILE_PATH="${file_path}"
        export BLIF_NAME="${FILE_NAME}.blif"

        if [ -f "${OUTPUT_BLIF_PATH}/${BLIF_NAME}" ]; then
            print_test_stat "E"
            continue
        fi
        
        run_yosys "${ODIN_DIR}/synth.tcl"

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
                elif [ -f "${possible_test}/task.conf" ]
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
        echo "===== $(basename ${task_list[$i]}) ====="
        run_task "${task_list[$i]}"
		TEST_COUNT=$(( TEST_COUNT + 1 ))
	done

	if [ "_${TEST_COUNT}" == "_0" ];
	then
		echo "No test is passed in must pass a test directory containing either a task_list.conf or a task.conf, see --help"
		_exit_with_code "-1"
	fi
}


##############################################################################################
######################################### START HERE #########################################
##############################################################################################

init

parse_args "$@"


run_suite
