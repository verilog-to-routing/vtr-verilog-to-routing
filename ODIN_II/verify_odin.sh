#!/bin/bash
SHELL=/bin/bash
FAILURE=0

THIS_SCRIPT_PATH=$(readlink -f $0)
THIS_DIR=$(dirname ${THIS_SCRIPT_PATH})
REGRESSION_DIR="${THIS_DIR}/regression_test"
REG_LIB="${REGRESSION_DIR}/.library"

source ${REG_LIB}/handle_exit.sh
source ${REG_LIB}/helper.sh
source ${REG_LIB}/conf_generate.sh

export EXIT_NAME="$0"

##############################################
# grab the input args
INPUT=$@


WRAPPER_EXEC="${THIS_DIR}/exec_wrapper.sh"
ODIN_EXEC="${THIS_DIR}/odin_II"


BENCHMARK_DIR="${REGRESSION_DIR}/benchmark"
TEST_DIR_LIST=$(ls -d ${BENCHMARK_DIR}/*/ | sed "s/\/$//g" | xargs -n1 -I TEST_FILE /bin/bash -c 'printf "$(basename TEST_FILE) "')
NEW_RUN_DIR="${REGRESSION_DIR}/run001/"

##############################################
# Exit Functions
function exit_program() {
	
	FAIL_COUNT="0"
	if [ -f ${NEW_RUN_DIR}/test_failures.log ]; then
		FAIL_COUNT=$(wc -l ${NEW_RUN_DIR}/test_failures.log | cut -d ' ' -f 1)
	fi

	FAILURE=$(( ${FAIL_COUNT} ))
	
	if [ "_${FAILURE}" != "_0" ]
	then
		echo "Failed ${FAILURE} benchmarks"
		echo ""
		cat ${NEW_RUN_DIR}/test_failures.log
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
	printf "%s%s" $arg "${line:${#arg}}"
}

##############
# defaults
_TEST=""
_NUMBER_OF_PROCESS="1"
_SIM_COUNT="1"
_RUN_DIR_OVERRIDE=""

_CONFIG_OVERRIDE=""

_GENERATE_BENCH="off"
_GENERATE_OUTPUT="off"
_GENERATE_CONFIG="off"
_FORCE_SIM="off"

function help() {

printf "Called program with $INPUT
	Usage: 
		$0 [ OPTIONS / FLAGS ]


	OPTIONS:
		-h|--help                       $(_prt_cur_arg off) print this
		-t|--test < test name >         $(_prt_cur_arg ${_TEST}) Test name is one of ( ${TEST_DIR_LIST} heavy_suite light_suite full_suite vtr_basic vtr_strong pre_commit pre_merge)
		-j|--nb_of_process < N >        $(_prt_cur_arg ${_NUMBER_OF_PROCESS}) Number of process requested to be used
		-d|--output_dir < /abs/path >   $(_prt_cur_arg ${_RUN_DIR_OVERRIDE}) Change the run directory output
		-C|--config <path/to/config>	$(_prt_cur_arg ${_CONFIG_OVERRIDE}) Add a config override file

	FLAGS:
		-g|--generate_bench             $(_prt_cur_arg ${_GENERATE_BENCH}) Generate input and output vector for test
		-o|--generate_output            $(_prt_cur_arg ${_GENERATE_OUTPUT}) Generate output vector for test given its input vector
		-b|--build_config               $(_prt_cur_arg ${_GENERATE_CONFIG}) Generate a config file for a given directory
		-c|--clean                      $(_prt_cur_arg off ) Clean temporary directory
		-f|--force_simulate             $(_prt_cur_arg ${_FORCE_SIM}) Force the simulation to be executed regardless of the config

	CONFIG FILE HELP:
"

config_help

}

###############################################
# Time Helper Functions
function get_current_time() {
	echo $(date +%s%3N)
}

# needs start time $1
function print_time_since() {
	BEGIN=$1
	NOW=`get_current_time`
	TIME_TO_RUN=$(( ${NOW} - ${BEGIN} ))

	Mili=$(( ${TIME_TO_RUN} %1000 ))
	Sec=$(( ( ${TIME_TO_RUN} /1000 ) %60 ))
	Min=$(( ( ( ${TIME_TO_RUN} /1000 ) /60 ) %60 ))
	Hour=$(( ( ( ${TIME_TO_RUN} /1000 ) /60 ) /60 ))

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

	last_run=$(find ${OUTPUT_DIRECTORY}/run* -maxdepth 0 -type d 2>/dev/null | tail -1 )
	n="1"
	if [ "_${last_run}" != "_" ]
	then
		n=$(echo ${last_run##${OUTPUT_DIRECTORY}/run} | awk '{print $0 + 1}')
	fi

	NEW_RUN_DIR=${OUTPUT_DIRECTORY}/run$(printf "%03d" $n)
}

function create_temp() {
	if [ ! -d ${NEW_RUN_DIR} ]; then
		echo "Benchmark result location: ${NEW_RUN_DIR}"
		mkdir -p ${NEW_RUN_DIR}

		unlink ${REGRESSION_DIR}/latest &> /dev/null || /bin/true
		rm -Rf ${REGRESSION_DIR}/latest || /bin/true

		ln -s ${NEW_RUN_DIR} ${REGRESSION_DIR}/latest
	fi
}

function cleanup_temp() {
	OUTPUT_DIRECTORY=${REGRESSION_DIR}
	if [ "_${_RUN_DIR_OVERRIDE}" != "_" ]
	then
		OUTPUT_DIRECTORY=${_RUN_DIR_OVERRIDE}
	fi

	for runs in ${OUTPUT_DIRECTORY}/run*
	do 
		rm -Rf ${runs}
	done

	if [ -e ${REGRESSION_DIR}/latest ]; then
		unlink ${REGRESSION_DIR}/latest || /bin/true
		rm -Rf ${REGRESSION_DIR}/latest || /bin/true
	fi

}

function disable_failed() {
	failed_dir=$1
	log_file="${failed_dir}.log"

	if [ -e ${log_file} ]
	then
		for failed_benchmark in $(cat ${log_file})
		do
			for cmd_params in $(find ${NEW_RUN_DIR}/${failed_benchmark} -name 'wrapper_*')
			do
				[ "_${cmd_params}" != "_" ] && [ -f ${cmd_params}  ] && mv ${cmd_params} ${cmd_params}_disabled
			done
		done
	fi
}

function mv_failed() {
	failed_dir=$1
	log_file="${failed_dir}.log"

	if [ -e ${log_file} ]
	then

		for failed_benchmark in $(cat ${log_file})
		do
			target="${failed_dir}/${failed_benchmark}"
			target_dir=$(dirname ${target})
			target_link=$(basename ${target})

			mkdir -p ${target_dir}

			if [ ! -L "${target}" ]
			then
				ln -s ${NEW_RUN_DIR}/${failed_benchmark} ${target}
				echo "${failed_benchmark}" >> ${NEW_RUN_DIR}/test_failures.log
			fi
		done
	fi
}

function parse_args() {
	while [[ "$#" > 0 ]]
	do 
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

				_TEST="$2"
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
				
				_CONFIG_OVERRIDE=$2
				echo "Reading override from ${_CONFIG_OVERRIDE}"

				shift

		## number
			;;-j|--nb_of_process)
				_NUMBER_OF_PROCESS=$(_flag_is_number $1 $2)
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

			;;*) 
				echo "Unknown parameter passed: $1"
				help 
				_exit_with_code "-1"
		esac
		shift
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
_circuit_list=""
_arch_list=""

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
	_circuit_list=""
	_arch_list=""
}

function populate_arg_from_file() {

	_circuit_dir=""
	_arch_dir=""
	_circuit_list_add=""
	_circuit_list_remove=""
	_arch_list_add=""
	_arch_list_remove=""
	_local_script_synthesis_params=""
	_local_script_simulation_params=""
	_local_synthesis_params=""
	_local_simulation_params=""
	_local_regression_params=""

	if [ "_$1" == "_" ] || [ ! -f "$1" ]
	then
		echo "Config file $1 does not exist"
	else
		FILE=$(cat $1)
		OLD_IFS=${IFS}
		while IFS="" read -r current_line || [ -n "${current_line}" ]
		do

			formatted_line=$(format_line ${current_line})

			_key="$(echo ${formatted_line} | cut -d '=' -f1 )"
			_value="$(echo ${formatted_line} | cut -d '=' -f2 )"

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
						warn_is_defined "${_circuit_dir}" "${_key}"
						_circuit_dir="${_value}"

					;;_circuit_list_add)
						_circuit_list_add="${_circuit_list_add} ${_value}"					

					;;_arch_dir)
						warn_is_defined "${_arch_dir}" "${_key}"
						_arch_dir="${_value}"

					;;_arch_list_add)
						_arch_list_add="${_arch_list_add} ${_value}"

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
						echo "Unsupported value: ${_key} ${value}, skipping"

				esac
			fi
		done < $1
		IFS=${OLD_IFS}
	fi

	_regression_params=$(echo "${_local_regression_params} ")
	_script_simulation_params=$(echo "${_local_script_simulation_params} ")
	_script_synthesis_params=$(echo "${_local_script_synthesis_params} ")
	_synthesis_params=$(echo "${_local_synthesis_params} ")
	_simulation_params=$(echo "${_local_simulation_params} ")
	_circuit_list=$(echo "${_circuit_list} ")
	_arch_list=$(echo "${_arch_list} ")
	_circuit_dir=$(echo "${THIS_DIR}/${_circuit_dir}")
	_arch_dir=$(echo "${THIS_DIR}/${_arch_dir}")
	_circuit_list_add=$(echo "${_circuit_list_add} ")
	_arch_list_add=$(echo "${_arch_list_add} ")

	if [ "_${_circuit_list_add}" == "_" ]
	then
		echo "Passed a config file with no circuit to test ${_circuit_list_add}"
		_exit_with_code "-1"
	fi

	if [ "_${_circuit_dir}" != "_" ]
	then
		_circuit_dir=$(readlink -f ${_circuit_dir})
		if [ ! -d "${_circuit_dir}" ]
		then
			echo "Passed an invalid directory for your circuit files ${_circuit_dir}"
			_exit_with_code "-1"
		fi
	fi

	if [ "${_circuit_dir}" == "_" ]
	then
		echo "Passed an invalid directory for your circuit files"
		_exit_with_code "-1"
	fi

	for circuit_list_items in ${_circuit_list_add}
	do
		circuit_relative_path="${_circuit_dir}/${circuit_list_items}"
		circuit_real_path=$(readlink -f ${circuit_relative_path})
		for circuit_list_items in ${circuit_real_path}
		do
			if [ ! -f "${circuit_list_items}" ]
			then
				echo "file ${circuit_list_items} not found, skipping"
			else
				_circuit_list="${_circuit_list} ${circuit_list_items}"
			fi
		done

	done

	_circuit_list=$(echo ${_circuit_list})



	if [ "_${_arch_dir}" != "_" ]
	then
		_arch_dir=$(readlink -f ${_arch_dir})
		if [ ! -d "${_arch_dir}" ]
		then
			echo "Passed an invalid directory for your architecture files"
			_exit_with_code "-1"
		fi
	fi
	
	if [ "_${_arch_list_add}" == "_" ]
	then
		echo "Passed a config file with no architecture, defaulting to no_arch"
		_arch_list="no_arch"
	fi
	
	if [ "${_arch_dir}" == "_" ]
	then
		echo "Passed an invalid directory for your architecture files"
		_exit_with_code "-1"
	fi
	

	for arch_list_items in ${_arch_list_add}
	do
		arch_relative_path="${_arch_dir}/${arch_list_items}"
		arch_real_path=$(readlink -f ${arch_relative_path})
		for arch_list_item in ${arch_real_path}
		do
			if [ ! -f "${arch_list_item}" ]
			then
				echo "file ${arch_list_item} not found, skipping"
			else
				_arch_list="${_arch_list} ${arch_list_item}"
			fi
		done
	done

	_arch_list=$(echo ${_arch_list})

}

function formated_find() {
	find $1/ -name $2 | sed 's:\s+|\n+: :g' | sed 's:^\s*|\s*$::g'
}

function run_bench_in_parallel() {
	header=$1
	thread_count=$2
	failure_dir=$3
	_LIST="${@:4}"

	if [ "_${_LIST}" != "_" ]
	then
		echo " ========= ${header} Tests"

		#run the simulation in parallel
		echo ${_LIST} | xargs -d ' ' -l1 -n1 -P${thread_count} -I cmd_file ${SHELL} -c '$(cat cmd_file)'
		# disable the test on failure
		disable_failed ${failure_dir}
		mv_failed ${failure_dir}
	fi
}

function sim() {


	###########################################
	# find the benchmark
	benchmark_dir=$1
	if [ "_${benchmark_dir}" == "_" ] || [ ! -d ${benchmark_dir} ]
	then
		echo "invalid benchmark directory parameter passed: ${benchmark_dir}"
		_exit_with_code "-1"
	elif [ ! -f ${benchmark_dir}/task.conf ]
	then
		echo "invalid benchmark directory parameter passed: ${benchmark_dir}, contains no task.conf file"
		config_help
		_exit_with_code "-1"
	fi

	benchmark_dir=$(readlink -f "${benchmark_dir}")
	bench_name=$(basename ${benchmark_dir})
	echo "Task is: ${bench_name}"

	##########################################
	# setup the parameters

	init_args_for_test
	populate_arg_from_file "${benchmark_dir}/task.conf"

	##########################################
	# use the overrides from the user
	if [ "_${_CONFIG_OVERRIDE}" != "_" ]
	then
		_CONFIG_OVERRIDE=$(readlink -f ${_CONFIG_OVERRIDE})
		if [ ! -f ${_CONFIG_OVERRIDE} ] 
		then
			echo "Passed in an invalid global configuration file ${_CONFIG_OVERRIDE}"
			_exit_with_code "-1"
		else
			populate_arg_from_file "${_CONFIG_OVERRIDE}"
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
				if [ "_${_FORCE_SIM}" == "on" ] 
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
				_arch_list="no_arch ${_arch_list}"
				;;

			*)
				echo "Unknown internal parameter passed: ${_regression_param}"
				config_help 
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
		circuit_list_temp="$(echo ${_circuit_list} | sed 's/\n/ /g')"
		_circuit_list=${bench_name}
	fi

	for circuit in $(echo ${_circuit_list})
	do		
		circuit_dir=$(dirname ${circuit})
		circuit_file=$(basename ${circuit})
		input_verilog_file=""
		input_blif_file=""
		
		case "${circuit_file}" in
			*.v)
				input_verilog_file="${circuit}"
				_synthesis="on"
			;;
			*.blif)
				input_blif_file="${circuit}"
				# disable synthesis for blif files
				_synthesis="off"
			;;
			*)
				if [ ${_concat_circuit_list} == "on" ]
				then
					input_verilog_file="${circuit_list_temp}"
					_synthesis="on"
				else
					echo "Invalid circuit passed in: ${circuit}, skipping"
					continue
				fi
			;;
		esac
		circuit_name="${circuit_file%.*}"


		# lookup for input and output vector files to do comparison
		input_vector_file="${circuit_dir}/${circuit_name}_input"
		output_vector_file="${circuit_dir}/${circuit_name}_output"

		for arches in $(echo ${_arch_list})
		do
			arch_cmd=""
			if [ -e ${arches} ]
			then
				arch_cmd="-a ${arches}"
			fi

			arch_name=$(basename ${arches%.*})

			TEST_FULL_REF="${bench_name}/${circuit_name}/${arch_name}"
			DIR="${NEW_RUN_DIR}/${TEST_FULL_REF}"
			mkdir -p $DIR

			###############################
			# Synthesis
			if [ "${_synthesis}" == "on" ]
			then
			
				# if synthesis was on, we need to specify a blif output name
				input_blif_file="${DIR}/${circuit_name}.blif"

				synthesis_params_file=${DIR}/synthesis_params

				wrapper_command="${WRAPPER_EXEC}
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

				_echo_args "${synthesis_command}" > ${synthesis_params_file}
				_echo_args "${wrapper_command}"  > ${DIR}/${wrapper_synthesis_file_name}
			fi
			###############################
			# Simulation
			if [ "${_simulation}" == "on" ]
			then
				simulation_params_file=${DIR}/simulation_params

				wrapper_command="${WRAPPER_EXEC}
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

				if [ "${_GENERATE_BENCH}" == "on" ] || [ ! -f ${input_vector_file} ]
				then
					_echo_args "${simulation_command}" > ${simulation_params_file}
					_echo_args "${wrapper_command}" > ${DIR}/${wrapper_simulation_generate_io_file_name}

				elif [ "${_GENERATE_OUTPUT}" == "on" ] || [ ! -f ${output_vector_file} ]
				then
					_echo_args "${simulation_command} -t ${input_vector_file}" > ${simulation_params_file}
					_echo_args "${wrapper_command}" > ${DIR}/${wrapper_simulation_generate_output_file_name}

				else
					_echo_args "${simulation_command} -t ${input_vector_file} -T ${output_vector_file}" > ${simulation_params_file}
					_echo_args "${wrapper_command}" > ${DIR}/${wrapper_simulation_predefined_io_file_name}

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
			"$(formated_find ${NEW_RUN_DIR}/${bench_name} ${wrapper_synthesis_file_name})"
	fi

	if [ "${_simulation}" == "on" ]
	then

		run_bench_in_parallel \
			"Generate_IO_Simulation" \
			"${_threads}" \
			"${global_simulation_failure}" \
			"$(formated_find ${NEW_RUN_DIR}/${bench_name} ${wrapper_simulation_generate_io_file_name})"
			
		run_bench_in_parallel \
			"Generate_Output_Simulation" \
			"${_threads}" \
			"${global_simulation_failure}" \
			"$(formated_find ${NEW_RUN_DIR}/${bench_name} ${wrapper_simulation_generate_output_file_name})"

		run_bench_in_parallel \
			"Predefined_IO_Simulation" \
			"${_threads}" \
			"${global_simulation_failure}" \
			"$(formated_find ${NEW_RUN_DIR}/${bench_name} ${wrapper_simulation_predefined_io_file_name})"

	fi

	INPUT_VECTOR_LIST="$(find ${NEW_RUN_DIR}/${bench_name}/ -name input_vectors)"
	if [ "${_simulation}" == "on" ] && [ "_${INPUT_VECTOR_LIST}" != "_" ]
	then
		mkdir -p ${NEW_RUN_DIR}/${bench_name}/vectors

		# move the input vectors
		for sim_input_vectors in $(find ${NEW_RUN_DIR}/${bench_name}/ -name "input_vectors")
		do
			BM_DIR=$(dirname ${sim_input_vectors})
			BM_NAME="$(basename $(readlink -f ${BM_DIR}/..))_input"

			cp ${sim_input_vectors} ${NEW_RUN_DIR}/${bench_name}/vectors/${BM_NAME}
			mv ${sim_input_vectors} ${BM_DIR}/${BM_NAME}
			
		done
	fi

	OUTPUT_VECTOR_LIST="$(find ${NEW_RUN_DIR}/${bench_name}/ -name output_vectors)"
	if [ "${_simulation}" == "on" ] && [ "_${OUTPUT_VECTOR_LIST}" != "_" ]
	then
		mkdir -p ${NEW_RUN_DIR}/${bench_name}/vectors

		# move the output vectors
		for sim_output_vectors in $(find ${NEW_RUN_DIR}/${bench_name}/ -name "output_vectors")
		do
			BM_DIR=$(dirname ${sim_output_vectors})
			BM_NAME="$(basename $(readlink -f ${BM_DIR}/..))_output"

			cp ${sim_output_vectors} ${NEW_RUN_DIR}/${bench_name}/vectors/${BM_NAME}
			mv ${sim_output_vectors} ${BM_DIR}/${BM_NAME}

		done
	fi

}

function run_task() {
	test_dir=$1

	if [ ! -d ${test_dir} ]
	then
		echo "${test_dir} Not Found! Skipping this test"
	elif [ ! -f "${test_dir}/task.conf" ] 
	then
		if [ ${_GENERATE_CONFIG} == "on" ]
		then
			new_test_dir="${BENCHMARK_DIR}/task/generated_$(basename ${test_dir})"
			echo "generating config file for ${test_dir} @ ${new_test_dir}"
			mkdir -p ${new_test_dir}
			echo_bm_conf ${test_dir} > ${new_test_dir}/task.conf
		else
			echo "no config file found in the directory ${test_dir}"
			echo "please make sure a .conf file exist, you can use '--build_config' to generate one"
			config_help
		fi
	else
		create_temp
		sim "${test_dir}"
	fi
}

function run_vtr_reg() {
	cd ${THIS_DIR}/..
	/usr/bin/perl run_reg_test.pl -j ${_NUMBER_OF_PROCESS} $1
	cd ${THIS_DIR}
}

input_list=()
task_list=()
vtr_reg_list=()

function run_suite() {
	while [ "_${input_list}" != "_" ]
	do
		current_input="${input_list[0]}"
		input_list=( "${input_list[@]:1}" )

		if [ -d "${current_input}" ]
		then
			if [ -f "${current_input}/task_list.conf" ]
			then

				for input_path in $(cat ${current_input}/task_list.conf)
				do
					case "_${input_path}" in
						_);;
						*vtr_reg_*);;
						*)
							# bash expand when possible
							input_path=$(ls -d -1 ${THIS_DIR}/${input_path} 2> /dev/null)
							;;
					esac
					
					input_list=( ${input_list[@]} ${input_path[@]} )
				done
				
			elif [ -f "${current_input}/task.conf" ]
			then
				task_list=( ${task_list[@]} ${current_input} )
			else
				echo "Invalid Directory for task: ${current_input}"
			fi
		else
			case "_${current_input}" in
				*vtr_reg_*)
					vtr_reg_list=( ${vtr_reg_list[@]} ${current_input} )
					;;

				*)
					# bash expand when possible
					echo "no such Directory for task: ${current_input}"
					;;
			esac
		fi

	done

	for task in "${task_list[@]}"
	do
		run_task "${task}"
	done

	for vtr_reg in "${vtr_reg_list[@]}"
	do
		run_vtr_reg ${vtr_reg}
	done
}
#########################################################
#	START HERE

START=`get_current_time`

init_temp

parse_args $INPUT

if [ ! -x ${ODIN_EXEC} ]
then
	echo "Unable to find ${ODIN_EXEC}"
	_exit_with_code "-1"
fi

if [ ! -x ${WRAPPER_EXEC} ]
then
	echo "Unable to find ${WRAPPER_EXEC}"
	_exit_with_code "-1"
fi

if [ "_${_TEST}" == "_" ]
then
	echo "No test is passed in must pass a test directory containing either a task_list.conf or a task.conf"
	help
	_exit_with_code "-1"
fi

_TEST=$(readlink -f ${_TEST})

echo "Task: ${_TEST}"

input_list=( "${_TEST}" )

run_suite

print_time_since $START

exit_program
### end here
