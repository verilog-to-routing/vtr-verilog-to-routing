#!/bin/bash
#Authors: Aaron Graham (aaron.graham@unb.ca, aarongraham9@gmail.com),
#         Jean-Philippe Legault (jlegault@unb.ca, jeanphilippe.legault@gmail.com) and
#          Dr. Kenneth B. Kent (ken@unb.ca)
#          for the Reconfigurable Computing Research Lab at the
#           Univerity of New Brunswick in Fredericton, New Brunswick, Canada

# trap ctrl-c and call ctrl_c()
trap ctrl_c INT

TOTAL_TEST_RAN=0
FAILURE_COUNT=0
DEBUG=0

function ctrl_c() {
    FAILURE_COUNT=$((FAILURE_COUNT+1))
	exit_code ${FAILURE_COUNT} "\n\n** EXITED FORCEFULLY **\n\n"
}

function exit_code() {
	#print passed in value
	echo -e $2
	my_failed_count=$1
	echo -e "$TOTAL_TEST_RAN Tests Ran; $my_failed_count Test Failures.\n"
	[ "$my_failed_count" -gt "127" ] && echo "WARNING: Return Code may be unreliable: More than 127 Failures!"
	echo "End."
	exit ${my_failed_count}
}

# Check if Library 'file' "${0%/*}/librtlnumber.a" exists
[ ! -f ${0%/*}/librtlnumber.a ] && exit_code 99 "${0%/*}/librtlnumber.a library file not found!\n"

# Check if test harness binary "${0%/*}/rtl_number" exists
[ ! -f ${0%/*}/rtl_number ] && exit_code 99 "${0%/*}/rtl_number test harness file not found!\n" 

# Dynamically load in inputs and results from
#  file(s) on disk.
for INPUT in ${0%/*}/regression_tests/*.csv; do
	[ ! -f $INPUT ] && exit_code 99 "$INPUT regression test file not found!\n"

	echo -e "\nRunning Test File: $INPUT:"

	LINE=0

	while IFS= read -r input_line; do

		LINE=$((LINE + 1))

		# echo -e "${0##*/}@${HOSTNAME}: DEBUG: LINE: $LINE\n"

		#glob whitespace from line and remove everything after comment
		input_line=$(echo ${input_line} | tr -d '[:space:]' | cut -d '#' -f1)

		#skip empty lines
		[  "_" ==  "_${input_line}" ] && continue

		#split csv
		IFS="," read -ra arr <<< ${input_line}
		len=${#arr[@]}

		if 	[ ${len} != "4" ] &&		# unary
			[ ${len} != "5" ] &&		# binary
			[ ${len} != "7" ]; then		# ternary
				[ ! -z ${DEBUG} ] && echo -e "\nWARNING: Malformed Line in CSV File ($INPUT:$LINE) Input Line: ${input_line}! Skipping...\n"
				continue
		fi

		TOTAL_TEST_RAN=$((TOTAL_TEST_RAN + 1))

		# echo -e "${0##*/}@${HOSTNAME}: DEBUG: TOTAL_TEST_RAN: $TOTAL_TEST_RAN\n"

		#deal with multiplication
		set -f

		# everything between is the operation to pipe in so we slice the array and concatenate with space
		TEST_LABEL=${arr[0]}
		EXPECTED_RESULT=${arr[$(( len -1 ))]}
		RTL_CMD_IN=$(printf "%s " "${arr[@]:1:$(( len -2 ))}")

		# echo -e "${0##*/}@${HOSTNAME}: DEBUG: TEST_LABEL: $TEST_LABEL\n"
		# echo -e "${0##*/}@${HOSTNAME}: DEBUG: EXPECTED_RESULT: $EXPECTED_RESULT\n"
		# echo -e "${0##*/}@${HOSTNAME}: DEBUG: RTL_CMD_IN: $RTL_CMD_IN\n"

		# Check for Anything on standard out and any non-'0' exit codes:
		OUTPUT_AND_RESULT=$(${0%/*}/rtl_number ${RTL_CMD_IN})
		EXIT_CODE=$?

		# echo -e "${0##*/}@${HOSTNAME}: DEBUG: OUTPUT_AND_RESULT: $OUTPUT_AND_RESULT\n"
		# echo -e "${0##*/}@${HOSTNAME}: DEBUG: EXIT_CODE: $EXIT_CODE\n"

		if [[ 0 -ne $EXIT_CODE ]]
		then
			FAILURE_COUNT=$((FAILURE_COUNT+1))

			# echo -e "${0##*/}@${HOSTNAME}: DEBUG: FAILURE_COUNT: $FAILURE_COUNT\n"

			echo -e "\nERROR: Non-Zero Exit Code from ${0%/*}/rtl_number (on $INPUT:$LINE)\n"

			echo -e "-X- FAILED == $TEST_LABEL\t  ./rtl_number ${RTL_CMD_IN}\t sOutput:<$OUTPUT_AND_RESULT> != <$EXPECTED_RESULT>"

		elif [ "pass" == "$(${0%/*}/rtl_number is_true $(${0%/*}/rtl_number ${OUTPUT_AND_RESULT} == ${EXPECTED_RESULT}))" ]
		then
			echo "--- PASSED == $TEST_LABEL"

		else
			FAILURE_COUNT=$((FAILURE_COUNT+1))

			# echo -e "${0##*/}@${HOSTNAME}: DEBUG: FAILURE_COUNT: $FAILURE_COUNT\n"

			echo -e "\nERROR: Expected Result Didn't match what we got back from ${0%/*}/rtl_number (on $INPUT:$LINE)\n"

			echo -e "-X- FAILED == $TEST_LABEL\t  ./rtl_number ${RTL_CMD_IN}\t sOutput:<$OUTPUT_AND_RESULT> != <$EXPECTED_RESULT>"

		fi

		#unset the multiplication token override
		unset -f

	done < "$INPUT"
	#  Re-Enable Bash Wildcard Expanstion '*' 
	set +f
done

exit_code ${FAILURE_COUNT} "Completed Tests\n"
