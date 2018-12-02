#!/bin/bash
#1
trap ctrl_c INT SIGINT SIGTERM
SHELL=/bin/bash

QUIT=0

TIME_EXEC=$($SHELL -c "which time") 
INPUT=$@

VALGRIND_EXEC=""
LOG=""
LOG_FILE=""
TEST_NAME="odin"
FAILURE_FILE=""
EXIT_STATUS=3
TIME_LIMIT="86400s" #default to a full day

export TIME="\
	elapsed: %E 
	CPU:     %P
	max:     %M KiB
	swaps:   %s
"

EXEC="./odin_II"

function help() {
printf "
Called program with $[INPUT]

Usage: ./wrapper_odin.sh [options] CMD
			--valgrind								* run with valgrind
			--log_file                              * output status to a log file
			--test_name                             * label the test for pretty print
			--failure_log                           * output the display label to a file if there was a failure
			--time_limit                            * stops Odin after X seconds
			--limit_ressource						* limit ressource usage using ulimit -m (25% of hrdw memory) and nice value of 19
"
}

function log_it {
	LOG="${LOG}$1"
}

function dump_log {
	#print to destination log if set
	if [ "_${LOG}" != "_" ]
	then
		if [ "_${LOG_FILE}" != "_" ]
		then
			echo "${LOG}" > ${LOG_FILE}
			echo "" > ${LOG_FILE}
		else
			echo "${LOG}"
			echo ""
		fi
		LOG=""
	fi

}

function ctrl_c() {
	if [ "_${QUIT}" == "_0" ]
	then
		QUIT=1
		echo "** ODIN WRAPPER EXITED FORCEFULLY **"
		jobs -p | xargs kill &> /dev/null
		pkill odin_II &> /dev/null
		#should be dead by now
		exit 1
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
	log_it "Virtual Memory Limit:\t$(( $(ulimit -a | grep "virtual memory" | tr -s ' ' | cut -d ')' -f2) /1024 ))MB\n" 
	log_it "Physical Memory Limit:\t$(( $(ulimit -a | grep "max memory size" | tr -s ' ' | cut -d ')' -f2) /1024 ))MB\n"
	dump_log
}


function display() {
	# we display status to std out if there is a log file
	case $1 in
		running)
			echo "       --- ${TEST_NAME}"
			;;

		passed)
			echo "--- PASSED ${TEST_NAME}"
			;;

		failed)
			echo "-X- FAILED ${TEST_NAME}"
			[ "_${FAILURE_FILE}" != "_" ] && echo "${TEST_NAME}" >> ${FAILURE_FILE}
			;;

		timeout)
			echo "-X- TIMEUP ${TEST_NAME}"
			[ "_${FAILURE_FILE}" != "_" ] && echo "${TEST_NAME}" >> ${FAILURE_FILE}
			;;

		*)
		;;
	esac
}

#########################################################
#	START HERE

if [[ "$#" == 0 ]]
then
	help
	exit 0
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
			restrict_ressource 
			;;

		--valgrind)
			VALGRIND_EXEC="valgrind --leak-check=full"
			;;

		*) 
			cmd=$@
			cmd="${VALGRIND_EXEC} ${EXEC} ${cmd}"
			log_it "Starting Odin with: $cmd"
			dump_log

			display "running"

			if [ "_${LOG_FILE}" != "_" ]; then 
				timeout ${TIME_LIMIT} /bin/bash -c "${TIME_EXEC} --output=${LOG_FILE} --append ${cmd} &>> ${LOG_FILE}" &> /dev/null && EXIT_STATUS=0 || EXIT_STATUS=1 
			else
				timeout ${TIME_LIMIT} /bin/bash -c "${TIME_EXEC} ${cmd}" && EXIT_STATUS=0 || EXIT_STATUS=1
			fi
			break
			;;
	esac 
	shift 
done

if [ "_${EXIT_STATUS}" == "_0" ]
then
	display "passed"
else
	display "failed"
fi

dump_log
exit ${EXIT_STATUS}
### end here