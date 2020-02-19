#!/usr/bin/env bash
SHELL=$(type -P bash)
FAILURE=0

THIS_SCRIPT_PATH=$(readlink -f "$0")
THIS_DIR=$(dirname "${THIS_SCRIPT_PATH}")
REGRESSION_DIR="${THIS_DIR}/regression_test"
REG_LIB="${REGRESSION_DIR}/.library"

source "${REG_LIB}/handle_exit.sh"
source "${REG_LIB}/helper.sh"
source "${REG_LIB}/conf_generate.sh"

export EXIT_NAME="$0"

##############################################
# grab the input args
INPUT=$@


WRAPPER_EXEC="${THIS_DIR}/exec_wrapper.sh"
ODIN_EXEC="${THIS_DIR}/odin_II"


BENCHMARK_DIR="${REGRESSION_DIR}/benchmark"

VTR_REG_PREFIX="vtr_reg_"
VTR_REG_DIR="${THIS_DIR}/../vtr_flow/tasks/regression_tests"

SUITE_DIR="${BENCHMARK_DIR}/suite"
RELAPATH_SUITE_DIR=$(realapath_from "${SUITE_DIR}" "${PWD}")

TASK_DIR="${BENCHMARK_DIR}/task"
RELAPATH_TASK_DIR=$(realapath_from "${TASK_DIR}" "${PWD}")

RTL_REG_PREFIX="rtl_reg"

PREVIOUS_RUN_DIR=""
NEW_RUN_DIR="${REGRESSION_DIR}/run001/"

##############################################
# Exit Functions
function exit_program() {
	
	FAIL_COUNT="0"
	if [ -f "${NEW_RUN_DIR}/test_failures.log" ]; then
		FAIL_COUNT=$(wc -l "${NEW_RUN_DIR}/test_failures.log" | cut -d ' ' -f 1)
	fi

	FAILURE=$(( FAIL_COUNT ))
	
	if [ "_${FAILURE}" != "_0" ]
	then
		echo "Failed ${FAILURE} benchmarks"
		echo ""
		cat "${NEW_RUN_DIR}/test_failures.log"
		echo ""
		echo "View Failure log in ${NEW_RUN_DIR}/test_failures.log"

	else
		echo "no run failure!"
	fi

	exit ${FAILURE}
}

##############################################
# Help Print helper
_prt_cur_arg() {
	arg="[ $1 ]"
	line="                      "
	printf "%s%s" "${arg}" "${line:${#arg}}"
}

##############
# defaults
_TEST_INPUT_LIST=()
_SUBTEST_LIST=""
_NUMBER_OF_PROCESS="1"
_RUN_DIR_OVERRIDE=""
_EXTRA_CONFIG=""

_OVERRIDE_CONFIG="off"
_GENERATE_BENCH="off"
_GENERATE_OUTPUT="off"
_GENERATE_CONFIG="off"
_FORCE_SIM="off"
_DRY_RUN_ARG=""

###################
# you can adjust this to test this suite, this allow to test different exit code,
# Don't expose the variable since it simply adds complexity for no reason
_DRY_RUN_EXIT_CODE="0"

function help() {

printf "Called program with $INPUT
	Usage: 
		$0 [ OPTIONS / FLAGS ] [ SUBTEST_LIST ... ]

	SUBTEST_LIST
		should be a list of the form < task_name/test_file_name/architecture_file_name >
		passing this in will limit a task to a subset of test
		current: $(_prt_cur_arg ${_SUBTEST_LIST})

	FLAGS
		-g|--generate_bench             $(_prt_cur_arg ${_GENERATE_BENCH}) Generate input and output vector for test
		-o|--generate_output            $(_prt_cur_arg ${_GENERATE_OUTPUT}) Generate output vector for test given its input vector
		-b|--build_config               $(_prt_cur_arg ${_GENERATE_CONFIG}) Generate a config file for a given directory
		-c|--clean                      $(_prt_cur_arg off ) Clean temporary directory
		-f|--force_simulate             $(_prt_cur_arg ${_FORCE_SIM}) Force the simulation to be executed regardless of the config
		--override						$(_prt_cur_arg ${_OVERRIDE_CONFIG}) if a config file is passed in, override arguments rather than append
		--dry_run                       $(_prt_cur_arg ${_DRY_RUN_ARG}) performs a dry run to check the validity of the task and flow 

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

printf "\n\t\t%s\n" "${RELAPATH_SUITE_DIR}/"
for bm in "${SUITE_DIR}"/*; do printf "\t\t\t%s\n" $(basename "${bm}"); done

printf "\n\t\t%s\n" "${RELAPATH_TASK_DIR}/"
for bm in "${TASK_DIR}"/*; do printf "\t\t\t%s\n" $(basename "${bm}"); done

printf "\n\t\t%s\n" "${VTR_REG_PREFIX}"
for bm in "${VTR_REG_DIR}/${VTR_REG_PREFIX}"*; do printf "\t\t\t%s\n" $(basename "${bm}" | sed "s+${VTR_REG_PREFIX}++g"); done

printf "\n\t\t%s\n" "${RTL_REG_PREFIX}"

echo "CONFIG FILE HELP:"

config_help

}

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

################################################
# Init Directories and cleanup
function init_temp() {
	OUTPUT_DIRECTORY=${REGRESSION_DIR}
	if [ "_${_RUN_DIR_OVERRIDE}" != "_" ] && [ -d "${_RUN_DIR_OVERRIDE}" ]
	then
		OUTPUT_DIRECTORY=${_RUN_DIR_OVERRIDE}
	fi

	PREVIOUS_RUN_DIR=$(find ${OUTPUT_DIRECTORY}/run* -maxdepth 0 -type d 2>/dev/null | tail -1 )
	n="1"
	if [ "_${PREVIOUS_RUN_DIR}" != "_" ]
	then
		n=$(echo "${PREVIOUS_RUN_DIR##${OUTPUT_DIRECTORY}/run}" | awk '{print $0 + 1}')
	fi

	NEW_RUN_DIR=${OUTPUT_DIRECTORY}/run$(printf "%03d" "${n}")
}

function create_temp() {
	if [ ! -d "${NEW_RUN_DIR}" ]; then
		echo "Benchmark result location: ${NEW_RUN_DIR}"
		mkdir -p "${NEW_RUN_DIR}"

		unlink "${REGRESSION_DIR}/latest" &> /dev/null || /bin/true
		rm -Rf "${REGRESSION_DIR}/latest" || /bin/true

		ln -s "${NEW_RUN_DIR}" "${REGRESSION_DIR}/latest"

		# put in the passed parameter for keepsake 
		echo "tests ${_TEST_INPUT_LIST[*]}" > "${NEW_RUN_DIR}/cmd.task"
		echo "========="
		echo "$0 ${INPUT}" >> "${NEW_RUN_DIR}/cmd.task"
	fi
}

function cleanup_temp() {
	OUTPUT_DIRECTORY=${REGRESSION_DIR}
	if [ "_${_RUN_DIR_OVERRIDE}" != "_" ]
	then
		OUTPUT_DIRECTORY=${_RUN_DIR_OVERRIDE}
	fi

	for runs in "${OUTPUT_DIRECTORY}"/run*
	do 
		rm -Rf "${runs}"
	done

	if [ -e "${REGRESSION_DIR}/latest" ]; then
		unlink "${REGRESSION_DIR}/latest" || /bin/true
		rm -Rf "${REGRESSION_DIR}/latest" || /bin/true
	fi

}

function disable_failed() {
	failed_dir=$1
	log_file="${failed_dir}.log"

	if [ -f "${log_file}" ]
	then
		OLD_IFS=${IFS}
		while IFS="" read -r failed_benchmark || [ -n "${failed_benchmark}" ]
		do
			for cmd_params in "${NEW_RUN_DIR}/${failed_benchmark}"/wrapper_*
			do
				[ "_${cmd_params}" != "_" ] && [ -f "${cmd_params}"  ] && mv "${cmd_params}" "${cmd_params}_disabled"
			done

			target="${failed_dir}/${failed_benchmark}"
			target_dir=$(dirname "${target}")

			mkdir -p "${target_dir}"

			if [ ! -L "${target}" ]
			then
				ln -s "${NEW_RUN_DIR}/${failed_benchmark}" "${target}"
				echo "${failed_benchmark}" >> "${NEW_RUN_DIR}/test_failures.log"
			fi

		done < "${log_file}"
		IFS=${OLD_IFS}
	fi
}

function parse_args() {
	PARSE_SUBTEST="off"
	while [ "_$*" != "_" ]
	do 
		if [ ${PARSE_SUBTEST} == "on" ];
		then
		# parse subtest
			_SUBTEST_LIST="$1 ${_SUBTEST_LIST}"
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

				;;-d|--output_dir)

					if [ "_$2" == "_" ]
					then 
						echo "empty argument for $1"
						_exit_with_code "-1"
					fi
					
					_RUN_DIR_OVERRIDE=$2

					if [ ! -d "${_RUN_DIR_OVERRIDE}" ]
					then
						echo "Directory ${_RUN_DIR_OVERRIDE} does not exist"
						_exit_with_code "-1"
					fi

					shift

				;;-C|--config)

					if [ "_$2" == "_" ]
					then 
						echo "empty argument for $1"
						_exit_with_code "-1"
					fi
					
					_EXTRA_CONFIG=$2
					echo "Reading extra config directive from ${_EXTRA_CONFIG}"

					shift

			## number
				;;-j|--nb_of_process)
					_NUMBER_OF_PROCESS=$(_flag_is_number "$1" "$2")
					echo "Using [$2] processors for this benchmarking suite"
					shift

			# Boolean flags
				;;-g|--generate_bench)		
					_GENERATE_BENCH="on"
					echo "generating output vector for test given predefined input"

				;;-o|--generate_output)		
					_GENERATE_OUTPUT="on"
					echo "generating input and output vector for test"

				;;-b|--build_config)		
					_GENERATE_CONFIG="on"
					echo "generating a config file for test directory"

				;;-c|--clean)				
					echo "Cleaning temporary run in directory"
					cleanup_temp

				;;-f|--force_simulate)   
					_FORCE_SIM="on"
					echo "Forcing Simulation"   

				;;--override)
					_OVERRIDE_CONFIG="on"
					echo "Forcing override of config"    

				;;--dry_run)
					# we pass the dry_run flag to the wrapper
					_DRY_RUN_ARG="--dry_run ${_DRY_RUN_EXIT_CODE}"
					echo "Performing a dry run"

				;;*) 
					PARSE_SUBTEST="on"
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

function warn_is_defined() {
	[ "_$1" != "_" ] && echo "Specifying more than one ${2} in config file"
}

_regression_params=""
_script_synthesis_params=""
_script_simulation_params=""
_synthesis_params=""
_simulation_params=""
_circuit_list=()
_arch_list=()

function config_help() {
printf "
	*.conf is a list of key=value set
	'#' are used for comments

	the following key=value, ... are available:

			circuit_dir             = < path/to/circuit/dir >
			circuit_list_add        = < circuit file path relative to [circuit_dir] >
			arch_dir                = < path/to/arch/dir >
			arch_list_add           = < architecture file path relative to [arch_dir] >
			script_synthesis_params = [see exec_wrapper.sh options]
			script_simulation_params= [see exec_wrapper.sh options]
			synthesis_params        = [see Odin options]	
			simulation_params       = [see Odin options]
			regression_params       = 
			{
				--concat_circuit_list    # concatenate the circuit list and pass it straight through to odin
				--generate_bench         # generate input and output vectors from scratch
				--generate_output        # generate output vectors only if input vectors already exist
				--disable_simulation     # disable the simulation for this task
				--disable_parallel_jobs  # disable running circuit/task pairs in parralel
				--include_default_arch   # run odin also without architecture file
			}

"
}

init_args_for_test() {
	_regression_params=""
	_script_synthesis_params=""
	_script_simulation_params=""
	_synthesis_params=""
	_simulation_params=""
	_circuit_list=()
	_arch_list=()
}

function populate_arg_from_file() {

	_circuit_dir=""
	_arch_dir=""
	_circuit_list_add=()
	_arch_list_add=()
	_local_script_synthesis_params=""
	_local_script_simulation_params=""
	_local_synthesis_params=""
	_local_simulation_params=""
	_local_regression_params=""

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

					_circuit_dir)
						if [ ! -d "${_value}" ]
						then
							_value=${THIS_DIR}/${_value}
						fi
						_circuit_dir="${_value}"

					;;_circuit_list_add)
						# glob the value
						_circuit_list_add+=( "${_circuit_dir}"/${_value} )					

					;;_arch_dir)
						if [ ! -d "${_value}" ]
						then
							_value=${THIS_DIR}/${_value}
						fi
						_arch_dir="${_value}"

					;;_arch_list_add)
						# glob the value
						_arch_list_add+=( "${_arch_dir}"/${_value} )

					;;_script_synthesis_params)
						_local_script_synthesis_params="${_local_script_synthesis_params} ${_value}"

					;;_script_simulation_params)
						_local_script_simulation_params="${_local_script_simulation_params} ${_value}"

					;;_synthesis_params)
						_local_synthesis_params="${_local_synthesis_params} ${_value}"					
						
					;;_simulation_params)
						_local_simulation_params="${_local_simulation_params} ${_value}"

					;;_regression_params)
						_local_regression_params="${_local_regression_params} ${_value}"

					;;_)
						echo "skip" > /dev/null

					;;*)
						echo "Unsupported value: ${_key} ${_value}, skipping"

				esac
			fi
		done < "$1"
		IFS=${OLD_IFS}
	fi

	if [ "${_OVERRIDE_CONFIG}" == "on" ];
	then
		_regression_params="${_local_regression_params}"
		_script_simulation_params="${_local_script_simulation_params}"
		_script_synthesis_params="${_local_script_synthesis_params}"
		_synthesis_params="${_local_synthesis_params}"
		_simulation_params="${_local_simulation_params}"
	else
		_regression_params="${_local_regression_params} ${_regression_params}"
		_script_simulation_params="${_local_script_simulation_params} ${_script_simulation_params}"
		_script_synthesis_params="${_local_script_synthesis_params} ${_script_synthesis_params}"
		_synthesis_params="${_local_synthesis_params} ${_synthesis_params}"
		_simulation_params="${_local_simulation_params} ${_simulation_params}"
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
	

	for arch_list_item in "${_arch_list_add[@]}"
	do
		if [ ! -f "${arch_list_item}" ]
		then
			echo "file ${arch_list_item} not found, skipping"
		else
			_arch_list+=( "${arch_list_item}" )
		fi
	done

	if [ "_${#_arch_list[*]}" == "_" ]
	then
		echo "Passed a config file with no architecture, defaulting to no_arch"
		_arch_list+=( "no_arch" )
	fi

}

function formated_find() {
	# sort the output alphabeticaly
	find "$1/" -name "$2" | sed 's:\s+|\n+: :g' | sed 's:^\s*|\s*$::g' | sort
}

function run_bench_in_parallel() {
	header=$1
	thread_count=$2
	failure_dir=$3
	_LIST=( "${@:4}" )

	if [[ "_${_LIST[*]}" != "_" ]]
	then
		echo " ========= ${header} Tests"

		#run the simulation in parallel
		echo "${_LIST[@]}" | xargs -n1 -P"${thread_count}" -I cmd_file "${SHELL}" -c '$(cat cmd_file)'
		# disable the test on failure
		disable_failed "${failure_dir}"
	fi
}

function sim() {

	###########################################
	# find the benchmark
	benchmark_dir=$1
	if [ "_${benchmark_dir}" == "_" ] || [ ! -d "${benchmark_dir}" ]
	then
		echo "invalid benchmark directory parameter passed: ${benchmark_dir}"
		_exit_with_code "-1"
	elif [ ! -f "${benchmark_dir}/task.conf" ]
	then
		echo "invalid benchmark directory parameter passed: ${benchmark_dir}, contains no task.conf file, see CONFIG_HELP in --help"
		_exit_with_code "-1"
	fi

	benchmark_dir=$(readlink -f "${benchmark_dir}")
	bench_name=$(basename "${benchmark_dir}")
	##########################################
	# check if we only run some subtask
	run_benchmark="off"
	if [ "_${_SUBTEST_LIST}" == "_" ];
	then
		run_benchmark="on"
	else
		for subtest in ${_SUBTEST_LIST};
		do
			if [ "_${subtest%%/*}" == "_${bench_name}" ]
			then
				run_benchmark="on"
				break;
			fi
		done
	fi
	
	if [ "${run_benchmark}" == "on" ];
	then
		echo "Task is: ${bench_name}"

		##########################################
		# setup the parameters

		init_args_for_test
		populate_arg_from_file "${benchmark_dir}/task.conf"

		##########################################
		# use the overrides from the user
		if [ "_${_EXTRA_CONFIG}" != "_" ]
		then
			_EXTRA_CONFIG=$(readlink -f "${_EXTRA_CONFIG}")
			if [ ! -f "${_EXTRA_CONFIG}" ] 
			then
				echo "Passed in an invalid global configuration file ${_EXTRA_CONFIG}"
				_exit_with_code "-1"
			else
				populate_arg_from_file "${_EXTRA_CONFIG}"
			fi
		fi

		####################################
		# parse the function commands passed
		_threads=${_NUMBER_OF_PROCESS}
		_generate_bench="off"
		_generate_output="off"
		_concat_circuit_list="off"
		_synthesis="on"
		_simulation="on"

		##########################################
		# populate the wrapper command using the configs
		for _regression_param in ${_regression_params}
		do
			case ${_regression_param} in

				--concat_circuit_list)
					_concat_circuit_list="on"
					;;

				--generate_bench)
					echo "This test will have the input and output regenerated"
					_generate_bench="on"
					;;

				--generate_output)
					echo "This test will have the output regenerated"
					_generate_output="on"
					;;

				--disable_simulation)
					echo "This test will not be simulated"
					if [ "_${_FORCE_SIM}" == "_on" ] 
					then
						echo "WARNING: This test will be forcefully simulated, unexpected results may occur"
						_simulation="on"
					else
						_simulation="off"
					fi
					;;
		
				--disable_parallel_jobs)
					echo "This test will not be multithreaded"
					_threads="1"
					;;
					
				--include_default_arch)
					_arch_list+=( "no_arch" )
					;;

				*)
					echo "Unknown internal parameter passed: ${_regression_param}, see CONFIG_HELP in --help"
					_exit_with_code "-1"
					;;
			esac
		done

		##########################################
		# setup defaults
		global_synthesis_failure="${NEW_RUN_DIR}/synthesis_failures"
		global_simulation_failure="${NEW_RUN_DIR}/simulation_failures"

		wrapper_synthesis_file_name="wrapper_synthesis_params"
		wrapper_simulation_generate_io_file_name="wrapper_simulation_generate_io_file_name"
		wrapper_simulation_generate_output_file_name="wrapper_simulation_generate_output_file_name"
		wrapper_simulation_predefined_io_file_name="wrapper_simulation_predefined_io_file_name"

		circuit_list_temp=""
		if [ ${_concat_circuit_list} == "on" ]
		then
			circuit_list_temp="${_circuit_list[*]}"
			_circuit_list=( "${bench_name}" )
		fi

		for circuit in "${_circuit_list[@]}"
		do		
			circuit_dir=$(dirname "${circuit}")
			circuit_file=$(basename "${circuit}")
			input_verilog_file=""
			input_blif_file=""
			
			case "${circuit_file}" in
				*.blif)
					input_blif_file="${circuit}"
					# disable synthesis for blif files
					_synthesis="off"
				;;
				*)
					_synthesis="on"
					if [ ${_concat_circuit_list} == "on" ]
					then
						input_verilog_file="${circuit_list_temp}"
					else
						input_verilog_file="${circuit}"
					fi
				;;
			esac
			circuit_name="${circuit_file%.*}"


			# lookup for input and output vector files to do comparison
			input_vector_file="${circuit_dir}/${circuit_name}_input"
			output_vector_file="${circuit_dir}/${circuit_name}_output"

			for arches in "${_arch_list[@]}"
			do
				arch_cmd=""
				if [ -e "${arches}" ]
				then
					arch_cmd="-a ${arches}"
				fi

				arch_name=$(basename "${arches%.*}")

				TEST_FULL_REF="${bench_name}/${circuit_name}/${arch_name}"

				run_this_test="on"

				if 	echo " ${_SUBTEST_LIST}" | grep "${TEST_FULL_REF}" &> /dev/null ||
					[ -d "${NEW_RUN_DIR}/${TEST_FULL_REF}" ]
				then
					# skip duplicate tests
					run_this_test="off"
				fi

				if [ "${run_this_test}" == "on" ];
				then

					DIR="${NEW_RUN_DIR}/${TEST_FULL_REF}"
					mkdir -p "$DIR"

					###############################
					# Synthesis
					if [ "${_synthesis}" == "on" ]
					then
					
						# if synthesis was on, we need to specify a blif output name
						input_blif_file="${DIR}/${circuit_name}.blif"

						synthesis_params_file=${DIR}/synthesis_params

						wrapper_command="${WRAPPER_EXEC}
								${_DRY_RUN_ARG}
								${_script_synthesis_params}
								--log_file ${DIR}/synthesis.log
								--test_name ${TEST_FULL_REF}
								--failure_log ${global_synthesis_failure}.log
								${synthesis_params_file}"

						synthesis_command="${ODIN_EXEC} 
											${_synthesis_params}
											${arch_cmd}
											-V ${input_verilog_file}
											-o ${input_blif_file}
											-sim_dir ${DIR}"

						_echo_args "${synthesis_command}" > "${synthesis_params_file}"
						_echo_args "${wrapper_command}"  > "${DIR}/${wrapper_synthesis_file_name}"
					fi
					###############################
					# Simulation
					if [ "${_simulation}" == "on" ]
					then
						simulation_params_file=${DIR}/simulation_params

						wrapper_command="${WRAPPER_EXEC}
											${_DRY_RUN_ARG}
											${_script_simulation_params}
											--log_file ${DIR}/simulation.log
											--test_name ${TEST_FULL_REF}
											--failure_log ${global_simulation_failure}.log
											${simulation_params_file}"

						simulation_command="${ODIN_EXEC} 
												${_simulation_params}
												${arch_cmd}
												-b ${input_blif_file}
												-sim_dir ${DIR}"										

						if [ "_${_GENERATE_BENCH}" == "_on" ] || [ "_${_generate_bench}" == "_on" ] || [ ! -f "${input_vector_file}" ]
						then
							_echo_args "${simulation_command}" > "${simulation_params_file}"
							_echo_args "${wrapper_command}" > "${DIR}/${wrapper_simulation_generate_io_file_name}"

						elif [ "_${_GENERATE_OUTPUT}" == "_on" ] || [ "_${_generate_output}" == "_on" ]|| [ ! -f "${output_vector_file}" ]
						then
							_echo_args "${simulation_command} -t ${input_vector_file}" > "${simulation_params_file}"
							_echo_args "${wrapper_command}" > "${DIR}/${wrapper_simulation_generate_output_file_name}"

						else
							_echo_args "${simulation_command} -t ${input_vector_file} -T ${output_vector_file}" > "${simulation_params_file}"
							_echo_args "${wrapper_command}" > "${DIR}/${wrapper_simulation_predefined_io_file_name}"

						fi
					fi
				fi
			done
		done	

		#synthesize the circuits
		if [ "${_synthesis}" == "on" ]
		then
			run_bench_in_parallel \
				"Synthesis" \
				"${_threads}" \
				"${global_synthesis_failure}" \
				"$(formated_find "${NEW_RUN_DIR}/${bench_name}" "${wrapper_synthesis_file_name}")"
		fi

		if [ "${_simulation}" == "on" ]
		then

			run_bench_in_parallel \
				"Generate_IO_Simulation" \
				"${_threads}" \
				"${global_simulation_failure}" \
				"$(formated_find "${NEW_RUN_DIR}/${bench_name}" "${wrapper_simulation_generate_io_file_name}")"
				
			run_bench_in_parallel \
				"Generate_Output_Simulation" \
				"${_threads}" \
				"${global_simulation_failure}" \
				"$(formated_find "${NEW_RUN_DIR}/${bench_name}" "${wrapper_simulation_generate_output_file_name}")"

			run_bench_in_parallel \
				"Predefined_IO_Simulation" \
				"${_threads}" \
				"${global_simulation_failure}" \
				"$(formated_find "${NEW_RUN_DIR}/${bench_name}" "${wrapper_simulation_predefined_io_file_name}")"

		fi

		INPUT_VECTOR_LIST=$(find "${NEW_RUN_DIR}/${bench_name}/" -name "input_vectors")
		if [ "${_simulation}" == "on" ] && [ "_${INPUT_VECTOR_LIST}" != "_" ]
		then
			mkdir -p "${NEW_RUN_DIR}/${bench_name}/vectors"

			# move the input vectors
			for sim_input_vectors in ${INPUT_VECTOR_LIST}
			do
				BM_DIR=$(dirname "${sim_input_vectors}")
				BM_NAME="$(basename $(readlink -f ${BM_DIR}/..))_input"

				cp "${sim_input_vectors}" "${NEW_RUN_DIR}/${bench_name}/vectors/${BM_NAME}"
				mv "${sim_input_vectors}" "${BM_DIR}/${BM_NAME}"
				
			done
		fi

		OUTPUT_VECTOR_LIST=$(find "${NEW_RUN_DIR}/${bench_name}/" -name output_vectors)
		if [ "${_simulation}" == "on" ] && [ "_${OUTPUT_VECTOR_LIST[*]}" != "_" ]
		then
			mkdir -p "${NEW_RUN_DIR}/${bench_name}/vectors"

			# move the output vectors
			for sim_output_vectors in ${OUTPUT_VECTOR_LIST}
			do
				BM_DIR=$(dirname "${sim_output_vectors}")
				BM_NAME="$(basename $(readlink -f ${BM_DIR}/..))_output"

				cp "${sim_output_vectors}" "${NEW_RUN_DIR}/${bench_name}/vectors/${BM_NAME}"
				mv "${sim_output_vectors}" "${BM_DIR}/${BM_NAME}"

			done
		fi
	fi

}

function run_task() {
	test_dir=$1

	if [ ! -d "${test_dir}" ]
	then
		echo "${test_dir} Not Found! Skipping this test"
	elif [ ! -f "${test_dir}/task.conf" ] 
	then
		if [ ${_GENERATE_CONFIG} == "on" ]
		then
			new_test_dir="${BENCHMARK_DIR}/task/generated_$(basename ${test_dir})"
			echo "generating config file for ${test_dir} @ ${new_test_dir}"
			mkdir -p "${new_test_dir}"
			echo_bm_conf "${test_dir}" > "${new_test_dir}/task.conf"
		else
			echo "no config file found in the directory ${test_dir}"
			echo "please make sure a .conf file exist, you can use '--build_config' to generate one, see CONFIG_HELP in --help"
		fi
	else
		create_temp
		sim "${test_dir}"
	fi
}

function run_vtr_reg() {
	pushd ${THIS_DIR}/..
	/usr/bin/env perl run_reg_test.pl -j "${_NUMBER_OF_PROCESS}" "$1"
	popd
}

function run_rtl_reg() {
	pushd ${THIS_DIR}/../libs/librtlnumber
	./verify_librtlnumber.sh
	popd
}

function build_test() {
	pushd $1
	make -f task.mk build
	popd
}

task_list=()
vtr_reg_list=()
rtl_lib_test="off"

function run_suite() {
	while [ "_${_TEST_INPUT_LIST[*]}" != "_" ]
	do
		current_input="${_TEST_INPUT_LIST[0]}"
		_TEST_INPUT_LIST=( "${_TEST_INPUT_LIST[@]:1}" )

		case "_${current_input}" in
			_);;
			_${VTR_REG_PREFIX}*)
				vtr_reg_list+=( "${current_input}" )
				;;
			_${RTL_REG_PREFIX})
				rtl_lib_test="on"
				;;
			*)
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
						# build test when necessary
						if [ -f "${possible_test}/task.mk" ]
						then
							build_test "${possible_test}"
						fi

						if [ -f "${possible_test}/task_list.conf" ]
						then
							OLD_IFS=${IFS}
							while IFS="" read -r possible_sub_test || [ -n "${possible_sub_test}" ]
							do
								_TEST_INPUT_LIST+=( "${possible_sub_test}" )
							done < "${possible_test}/task_list.conf"
							IFS=${OLD_IFS}

						elif [ -f "${possible_test}/task.conf" ]
						then
							task_list+=( "${possible_test}" )
						else
							echo "Invalid Directory for task: ${possible_test}"
						fi
					fi
				done
				;;
		esac
	done

	TEST_COUNT="0"

	if [ "_${rtl_lib_test}" == "_on" ];
	then
		run_rtl_reg
		TEST_COUNT=$(( TEST_COUNT + 1 ))
	fi

	for task in "${task_list[@]}"
	do
		run_task "${task}"
		TEST_COUNT=$(( TEST_COUNT + 1 ))
	done

	for vtr_reg in "${vtr_reg_list[@]}"
	do
		run_vtr_reg "${vtr_reg}"
		TEST_COUNT=$(( TEST_COUNT + 1 ))
	done

	if [ "_${TEST_COUNT}" == "_0" ];
	then
		echo "No test is passed in must pass a test directory containing either a task_list.conf or a task.conf, see --help"
		_exit_with_code "-1"
	fi
}
#########################################################
#	START HERE

START=$(get_current_time)

init_temp

parse_args ${INPUT}

if [ ! -x "${ODIN_EXEC}" ]
then
	echo "Unable to find ${ODIN_EXEC}"
	_exit_with_code "-1"
fi

if [ ! -x "${WRAPPER_EXEC}" ]
then
	echo "Unable to find ${WRAPPER_EXEC}"
	_exit_with_code "-1"
fi

echo "Task: ${_TEST_INPUT_LIST[*]}"

run_suite

print_time_since "${START}"

exit_program
### end here
