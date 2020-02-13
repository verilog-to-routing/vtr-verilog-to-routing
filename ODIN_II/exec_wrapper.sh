#!/usr/bin/env bash
SHELL=$(type -P bash)

THIS_SCRIPT_PATH=$(readlink -f $0)
THIS_DIR=$(dirname ${THIS_SCRIPT_PATH})
REGRESSION_DIR="${THIS_DIR}/regression_test"
REG_LIB="${REGRESSION_DIR}/.library"

source ${REG_LIB}/handle_exit.sh
source  ${REG_LIB}/time_format.sh
source  ${REG_LIB}/helper.sh

export EXIT_NAME="$0"

##############################################
# grab the input args
INPUT=$@

TIMEOUT_EXEC="timeout"
TIME_EXEC=$($SHELL -c "which time") 
VALGRIND_EXEC="valgrind --leak-check=full --max-stackframe=128000000 --error-exitcode=1 --track-origins=yes"
PERF_EXEC="perf stat record -a -d -d -d -o"
GDB_EXEC="gdb --args"
EXEC_PREFIX=""

TOOL_LIST=""

TEST_NAME="N/A"
LOG=""
LOG_FILE=""
FAILURE_FILE=""

USE_TEMP_LOG="off"
RESTRICT_RESSOURCE="off"
TIME_LIMIT="86400s" #default to a full day
TOOL_SPECIFIED="off"
USE_TIMEOUT="on"
USE_TIME="on"
USE_LOGS="on"
COLORIZE_OUTPUT="off"
VERBOSE="0"
DRY_RUN=""

function print_exit_type() {
	CODE="$1"
	if [[ $1 -ge 128 ]]
	then
		CODE="$(( $1 - 128 ))"
	fi

	case $CODE in
		0)		echo "NO_ERROR"
		;;1)	echo "SIGHUP"
		;;2)	echo "SIGINT"
		;;3)	echo "SIGQUIT"
		;;4)	echo "SIGILL"
		;;5)	echo "SIGTRAP"
		;;6)	echo "SIGABRT"
		;;7)	echo "SIGBUS"
		;;8)	echo "SIGFPE"
		;;9)	echo "SIGKILL"
		;;10)	echo "SIGUSR1"
		;;11)	echo "SIGSEGV"
		;;12)	echo "SIGUSR2"
		;;13)	echo "SIGPIPE"
		;;14)	echo "SIGALRM"
		;;15)	echo "SIGTERM"
		;;16)	echo "SIGSTKFLT"
		;;17)	echo "SIGCHLD"
		;;18)	echo "SIGCONT"
		;;19)	echo "SIGSTOP"
		;;20)	echo "SIGTSTP"
		;;21)	echo "SIGTTIN"
		;;22)	echo "SIGTTOU"
		;;23)	echo "SIGURG"
		;;24)	echo "SIGXCPU"
		;;25)	echo "SIGXFSZ"
		;;26)	echo "SIGVTALRM"
		;;27)	echo "SIGPROF"
		;;28)	echo "SIGWINCH"
		;;29)	echo "SIGIO"
		;;30)	echo "SIGPWR"
		;;31)	echo "SIGSYS"
		;;124)	echo "Timeout"
		;;127)	echo "Errored"
		;;*)	echo "${CODE}"
		;;
	esac
}


function help() {
printf "
Called program with $[INPUT]

Usage: ./exec_wrapper.sh [options] <path/to/arguments.file>
			--tool [ gdb, valgrind, perf ]              * run with one of the specified tool and only one
			--log_file                                  * output status to a log file
			--test_name                                 * label the test for pretty print
			--failure_log                               * output the display label to a file if there was a failure
			--time_limit                                * stops Odin after X seconds
			--limit_ressource				            * limit ressource usage using ulimit -m (25% of hrdw memory) and nice value of 19
			--verbosity [0, 1, 2]						* [0] no output, [1] output on error, [2] output the log to stdout
			--no_color                                  * force no color on output
			--dry_run  [exit_code]                      * performs a dry run, without actually run the tool and return the defined exit_code
"
}

function dry_runner() {
	# this simply prints the argument and return passed
	echo "$*"
	return $(( ${DRY_RUN} + 0 ))
}

function log_it {
	INPUT="$@"
	LOG="${LOG}${INPUT}"
}

function dump_log {
	#print to destination log if set
	
	if [ "_${LOG}" != "_" ]
	then
		if [ "${USE_LOGS}" == "on" ]
		then
			printf "${LOG}\n" >> ${LOG_FILE}
		else
			printf "${LOG}\n"
		fi

		LOG=""
	fi

}

#this hopefully will force to swap more
function restrict_ressource {
	#some benchmark will eat all your ressource and OOM. setting a limit prevents this from happening, 
	# LUPEEG64 can use up to 36 Gb of Memory in total, We recommend growing your swap space

	PERCENT_LIMIT_FOR_LOW_RESSOURCE=20
	NICE_VALUE=19

	MEMORY_SIZE=$(grep MemTotal /proc/meminfo |awk '{print $2}')
	MEMORY_SIZE=$(( $(( $(( ${MEMORY_SIZE} )) * ${PERCENT_LIMIT_FOR_LOW_RESSOURCE} )) / 100 ))

	ulimit -m ${MEMORY_SIZE}
	renice -n ${NICE_VALUE}  -p $$ &> /dev/null

	log_it "Setting Nice value to ${NICE_VALUE}\n"
	log_it "Virtual Memory Limit:\t$(ulimit -a | grep "virtual memory" | tr -s ' ' | cut -d ')' -f2)\n" 
	log_it "Physical Memory Limit:\t$(ulimit -a | grep "max memory size" | tr -s ' ' | cut -d ')' -f2)\n"
	dump_log
}

function pretty_print_status() {

	RESULT=$1
	line=$(printf '\040%.0s\056%.0s' {1..36})
	empty_line=$(printf '\040%.0s\040%.0s' {1..36})

	if [ "_$RESULT" == "_" ]
	then
		printf "  ${empty_line} ${TEST_NAME}\n"
	elif [ "_${COLORIZE_OUTPUT}" == "_off" ]
	then
		printf "  ${RESULT}${line:${#RESULT}} ${TEST_NAME}\n"
	else
		if [ "_$RESULT" == "_Ok" ]
		then
			printf "  \033[0;32m${RESULT}${line:${#RESULT}}\033[0m ${TEST_NAME}\n"
		else
			printf "  \033[0;31m${RESULT}${line:${#RESULT}}\033[0m ${TEST_NAME}\n"
		fi
	fi
}
function display() {

	CAUGHT_EXIT_CODE="$1"
	LEAK_MESSAGE=""
	
	# check for valgrind leaks
	LEAK_COUNT="$(cat ${LOG_FILE} | grep 'ERROR SUMMARY:' | awk '{print $4}' | grep -E '^\-?[0-9]+$')"
	case "_${LEAK_COUNT}" in
		_|_0)	LEAK_MESSAGE=""
		;;_1)	LEAK_MESSAGE="[${LEAK_COUNT}]Leak"
		;;*)	LEAK_MESSAGE="[${LEAK_COUNT}]Leaks"
	esac

	# check for uncaught errors
	if [ "_${CAUGHT_EXIT_CODE}" == "_0" ]
	then
		ERROR_CATCH="$(cat ${LOG_FILE} | grep 'Program Exit Code:' | awk '{print $4}' | grep -E '^\-?[0-9]+$')"
		[ "_${ERROR_CATCH}" != "_" ] && CAUGHT_EXIT_CODE="${ERROR_CATCH}"
	fi

	# check for uncaught errors
	if [ "_${CAUGHT_EXIT_CODE}" == "_0" ]
	then
		ERROR_CATCH="$(cat ${LOG_FILE} | grep 'Command terminated by signal:' | awk '{print $5}' | grep -E '^\-?[0-9]+$')"
		[ "_${ERROR_CATCH}" != "_" ] && CAUGHT_EXIT_CODE="${ERROR_CATCH}"
	fi

	EXIT_ERROR_TYPE=$( print_exit_type "${CAUGHT_EXIT_CODE}" )


	if [ "_${CAUGHT_EXIT_CODE}" == "_0" ] && [ "_${LEAK_MESSAGE}" == "_" ]
	then
		pretty_print_status "Ok"
	else
		pretty_print_status "Failed ${LEAK_MESSAGE} exit:$1 \"${EXIT_ERROR_TYPE}\""
		[ "_${FAILURE_FILE}" != "_" ] && echo "${TEST_NAME}" >> ${FAILURE_FILE}
	fi

}

#########################################################
#	START HERE

if [[ "$#" == 0 ]]
then
	help
	_exit_with_code "-1"
fi

if [[ -t 1 ]] && [[ -t 2 ]] && [[ ! -p /dev/stdout ]] && [[ ! -p /dev/stderr ]]
then
	COLORIZE_OUTPUT="on"
	log_it "Using colorized output\n"
fi

while [[ "$#" > 0 ]]
do 
	case $1 in

		--log_file)
			LOG_FILE=$2
			shift
			;;

		--test_name)
			TEST_NAME=$2
			export EXIT_NAME="${TEST_NAME}"
			shift
			;;

		--failure_log)
			FAILURE_FILE=$2
			shift
			;;
		
		--time_limit)
			TIME_LIMIT=$2
			shift
			;;

		--limit_ressource) 
			RESTRICT_RESSOURCE="on" 
			;;

		--no_color)
			COLORIZE_OUTPUT="off"
			;;
			
		--verbosity)
			case "_$2" in
				_0)	VERBOSE="0";;
				_1) VERBOSE="1";;
				_2) VERBOSE="2";;
				*)
					echo "verbosity level is invalid: $2"
					help
					_exit_with_code "-1"
				;;
			esac
			log_it "Using verbose output level ${VERBOSE}\n"
			
			shift
			;;

		--tool)

			if [ ${TOOL_SPECIFIED} == "on" ]; then
				echo "can only run one tool at a time"
				help
				_exit_with_code "-1"
			else
				case $2 in
					valgrind)
						TOOL_LIST="valgrind ${TOOL_LIST}"
						EXEC_PREFIX="${VALGRIND_EXEC} ${EXEC_PREFIX}"
						;;
					gdb)
						TOOL_LIST="gdb ${TOOL_LIST}"
						USE_TIMEOUT="off"
						USE_LOGS="off"
						EXEC_PREFIX="${GDB_EXEC} ${EXEC_PREFIX}"
						;;
					perf)
						TOOL_LIST="perf ${TOOL_LIST}"
						EXEC_PREFIX="${PERF_EXEC} ${EXEC_PREFIX}"
						;;
					*)
						echo "Invalid tool $2 passed in"
						help
						_exit_with_code "-1"
						;;
				esac
				TOOL_SPECIFIED="on"
				shift
			fi
			;;
		--dry_run)
			DRY_RUN="$2"
			shift
			;;

		*) 
			break
			;;
	esac 
	shift 
done

ARG_FILE=$1
	
if [ "${RESTRICT_RESSOURCE}" == "on" ]
then
	restrict_ressource
fi

if [ "${USE_LOGS}" == "on" ]
then
	if [ "_${LOG_FILE}" == "_" ]
	then
		LOG_FILE=$(mktemp)
		USE_TEMP_LOG="on"
		log_it "using temporary logs\n"
	elif [ -f ${LOG_FILE} ]
	then
		rm -f ${LOG_FILE}
		log_it "removing old log file\n"
	fi

	if [ ! -f ${LOG_FILE} ]
	then
		touch ${LOG_FILE}
		log_it "creating new log file\n"
	fi
fi

if [ "${USE_TIME}" == "on" ]
then
	TOOL_LIST="time ${TOOL_LIST}"
	EXEC_PREFIX="${TIME_EXEC} --output=${LOG_FILE} --append ${EXEC_PREFIX}"
	log_it "running with /bin/time\n"
fi

if [ "${USE_TIMEOUT}" == "on" ]
then
	TOOL_LIST="timeout ${TOOL_LIST}"
	EXEC_PREFIX="timeout ${TIME_LIMIT} ${EXEC_PREFIX}"
	log_it "running with timeout ${TIME_LIMIT}\n"
fi

if [ "_${DRY_RUN}" != "_" ]
then
	EXEC_PREFIX="dry_runner ${EXEC_PREFIX}"
	log_it "running a dry run with exit code ${DRY_RUN}\n"
fi

dump_log

pretty_print_status ""


_ARGS=""
EXIT_CODE="-1"
if [ "_${ARG_FILE}" == "_" ] || [ ! -f "${ARG_FILE}" ]
then
	log_it "Must define a path to a valid argument file"
	dump_log
else
	failed_requirements=""
	# test all necessary tool
	for tool_used in ${TOOL_LIST}
	do
		which ${tool_used} &> /dev/null
		if [ "$?" != "0" ]; 
		then
			failed_requirements="${tool_used} ${failed_requirements}"
		fi
	done

	if [ "_${failed_requirements}" != "_" ];
	then
		if [ "${USE_LOGS}" == "on" ]
		then
			if [ "${VERBOSE}" == "2" ]
			then
				echo "missing \"${failed_requirements}\"" | tee ${LOG_FILE}
			else
				echo "missing \"${failed_requirements}\"" &>> ${LOG_FILE}
			fi	
		else
			echo "missing \"${failed_requirements}\""
		fi

		EXIT_CODE="-1"
		pretty_print_status "Missing package: ${failed_requirements}"

	else
		_ARGS=$(cat ${ARG_FILE})
		if [ "${USE_LOGS}" == "on" ]
		then
			if [ "${VERBOSE}" == "2" ]
			then
				${EXEC_PREFIX} ${_ARGS} 2>&1 | tee ${LOG_FILE}
			else
				${EXEC_PREFIX} ${_ARGS} &>> ${LOG_FILE}
			fi
		else
			${EXEC_PREFIX} ${_ARGS}
		fi
		EXIT_CODE=$?
		display "${EXIT_CODE}"
	fi
fi

EXIT_STATUS=0
if [ "${EXIT_CODE}" != "0" ]
then
	EXIT_STATUS=1
fi

if [ "${EXIT_STATUS}" != "0" ] && [ "${USE_LOGS}" == "on" ] && [ "${VERBOSE}" == "1" ]
then
	cat ${LOG_FILE}
fi

if [ ${USE_TEMP_LOG} == "on" ]
then
	rm -f ${LOG_FILE}
fi

exit ${EXIT_STATUS}
### end here