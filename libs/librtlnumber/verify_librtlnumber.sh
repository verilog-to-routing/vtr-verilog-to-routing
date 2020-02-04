#!/usr/bin/env bash
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

# # Check if Library 'file' "${0%/*}/librtlnumber.a" exists
if [ ! -f ./librtlnumber.a ] && [ ! -f ./rtl_number ]; 
then
		exit_code 99 "${0%/*}rtl number is nowhere to be found :o !\n" 
fi

# Dynamically load in inputs and results from
#  file(s) on disk.
for INPUT in ${0%/*}/regression_tests/*.csv; do
	[ ! -f $INPUT ] && exit_code 99 "$INPUT regression test file not found!\n"

	echo -e "\nRunning Test File: $INPUT:"

	LINE=0

	while IFS= read -r input_line; do

		LINE=$((LINE + 1))

		#glob whitespace from line and remove everything after comment
		input_line=$(echo ${input_line} | tr -d '[:space:]' | cut -d '#' -f1)

		#flip escaped commas to 'ESCAPED_COMMA' to safeguard agains having them as csv separator
		input_line=$(echo ${input_line} | sed 's/\\\,/ESCAPED_COMMA/g')

		#skip empty lines
		[  "_" ==  "_${input_line}" ] && continue

		#split csv
		IFS="," read -ra arr <<< ${input_line}
		len=${#arr[@]}

		if 	[ ${len} != "4" ] &&		# unary
			[ ${len} != "5" ] &&		# binary
			[ ${len} != "7" ] &&		# ternary
			[ ${len} != "8" ]; then		# replicate
				[ ! -z ${DEBUG} ] && echo -e "\nWARNING: Malformed Line in CSV File ($INPUT:$LINE) Input Line: ${input_line}! Skipping...\n"
				continue
		fi

		

		TOTAL_TEST_RAN=$((TOTAL_TEST_RAN + 1))

		#deal with multiplication
		set -f

		# everything between is the operation to pipe in so we slice the array and concatenate with space
		TEST_LABEL=${arr[0]}
		EXPECTED_RESULT=${arr[$(( len -1 ))]}
		
		# build the command and get back our escaped commas 
		RTL_CMD_IN=$(printf "%s " "${arr[@]:1:$(( len -2 ))}")
		RTL_CMD_IN=$( echo ${RTL_CMD_IN} | sed 's/ESCAPED_COMMA/,/g' )

		# Check for Anything on standard out and any non-'0' exit codes:
		OUTPUT_AND_RESULT=$(${0%/*}/rtl_number ${RTL_CMD_IN})
		EXIT_CODE=$?

		if [[ 0 -ne $EXIT_CODE ]]
		then
			FAILURE_COUNT=$((FAILURE_COUNT+1))

			echo -e "\nERROR: Non-Zero Exit Code from ${0%/*}/rtl_number (on $INPUT:$LINE)\n"

			echo -e "-X- FAILED == $TEST_LABEL\t  ./rtl_number ${RTL_CMD_IN}\t Output:<$OUTPUT_AND_RESULT> != Expected:<$EXPECTED_RESULT>"

		elif [ "${OUTPUT_AND_RESULT}" == "${EXPECTED_RESULT}" ]
		then
			echo "--- PASSED == $TEST_LABEL ( ${OUTPUT_AND_RESULT} ) "

		elif [ "pass" == "$(${0%/*}/rtl_number is_true $(${0%/*}/rtl_number ${OUTPUT_AND_RESULT} == ${EXPECTED_RESULT}))" ]
		then
			echo "--- PASSED == $TEST_LABEL ( ${OUTPUT_AND_RESULT} )"

		else
			FAILURE_COUNT=$((FAILURE_COUNT+1))

			# echo -e "${0##*/}@${HOSTNAME}: DEBUG: FAILURE_COUNT: $FAILURE_COUNT\n"

			echo -e "\nERROR: Expected Result Didn't match what we got back from ${0%/*}/rtl_number (on $INPUT:$LINE)\n"

			echo -e "-X- FAILED == $TEST_LABEL\t  ./rtl_number ${RTL_CMD_IN}\t Output:<$OUTPUT_AND_RESULT> != Expected:<$EXPECTED_RESULT>"

		fi

		#unset the multiplication token override
		unset -f

	done < "$INPUT"
	#  Re-Enable Bash Wildcard Expanstion '*' 
	set +f
done

exit_code ${FAILURE_COUNT} "Completed Tests\n"
