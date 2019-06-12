#!/bin/bash
#1
trap ctrl_c INT SIGINT SIGTERM
SHELL=/bin/bash
QUIT=0
FAILURE=0

##############################################
# grab the input args
INPUT=$@

##############################################
# grab the absolute Paths
THIS_SCRIPT=$(readlink -f $0)
THIS_SCRIPT_EXEC=$(basename ${THIS_SCRIPT})
ODIN_ROOT_DIR=$(dirname ${THIS_SCRIPT})
VTR_ROOT_DIR=$(readlink -f ${ODIN_ROOT_DIR}/..)

WRAPPER_EXEC="${ODIN_ROOT_DIR}/wrapper_odin.sh"

REGRESSION_DIR="${ODIN_ROOT_DIR}/regression_test/"
BENCHMARK_DIR="${REGRESSION_DIR}/benchmark/"
TEST_DIR_LIST=$(ls -d ${BENCHMARK_DIR}/*/ | sed "s/\/$//g" | xargs -n1 -I TEST_FILE /bin/bash -c 'printf "$(basename TEST_FILE) "')
NEW_RUN_DIR="${REGRESSION_DIR}/run001/"

##############################################
# Arch Sweep Arrays to use during benchmarking
DEFAULT_ARCH="${VTR_ROOT_DIR}/libs/libarchfpga/arch/sample_arch.xml"
MEM_ARCH="${VTR_ROOT_DIR}/vtr_flow/arch/timing/k6_N10_mem32K_40nm.xml"
SMALL_ARCH_SWEEP="${DEFAULT_ARCH} ${MEM_ARCH}"
FULL_ARCH_SWEEP=$(find ${VTR_ROOT_DIR}/vtr_flow/arch/timing -maxdepth 1 | grep xml)

##############################################
# Include more generic names here for better vector generation
HOLD_LOW="-L reset rst"
HOLD_HIGH="-H we"
HOLD_PARAM="${HOLD_LOW} ${HOLD_HIGH}"

##############
# defaults
_TEST=""
_NUMBER_OF_PROCESS="1"
_SIM_THREADS="1"
_VECTORS="100"
_TIMEOUT="1200"
_ADDER_DEF="default"
_SIM_COUNT="1"
_RUN_DIR_OVERRIDE=""

_GENERATE_BENCH="off"
_GENERATE_OUTPUT="off"
_LIMIT_RESSOURCE="off"
_VALGRIND="off"
_BEST_COVERAGE_OFF="on"
_BATCH_SIM="off"
_USE_PERF="off"
_FORCE_SIM="off"

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

function ctrl_c() {
	trap '' INT SIGINT SIGTERM
	QUIT=1
	while [ "${QUIT}" != "0" ]
	do
		echo "** REGRESSION TEST EXITED FORCEFULLY **"
		jobs -p | xargs kill &> /dev/null
		pkill odin_II &> /dev/null
		pkill ${THIS_SCRIPT_EXEC} &> /dev/null
		#should be dead by now
		exit 120
	done
}

##############################################
# Help Print helper
_prt_cur_arg() {
	arg="[ $1 ]"
	line="                      "
	printf "%s%s" $arg "${line:${#arg}}"
}

function help() {

printf "Called program with $INPUT
	Usage: 
		${THIS_SCRIPT_EXEC} [ OPTIONS / FLAGS ]


	OPTIONS:
		-h|--help                       $(_prt_cur_arg off) print this
		-t|--test < test name >         $(_prt_cur_arg ${_TEST}) Test name is one of ( ${TEST_DIR_LIST} heavy_suite light_suite full_suite vtr_basic vtr_strong pre_commit failures debug_sim debug_synth)
		-j|--nb_of_process < N >        $(_prt_cur_arg ${_NUMBER_OF_PROCESS}) Number of process requested to be used
		-s|--sim_threads < N >          $(_prt_cur_arg ${_SIM_THREADS}) Use multithreaded simulation using N threads
		-V|--vectors < N >              $(_prt_cur_arg ${_VECTORS}) Use N vectors to generate per simulation
		-T|--timeout < N sec >          $(_prt_cur_arg ${_TIMEOUT}) Timeout a simulation/synthesis after N seconds
		-a|--adder_def < /abs/path >    $(_prt_cur_arg ${_ADDER_DEF}) Use template to build adders
		-n|--simulation_count < N >     $(_prt_cur_arg ${_SIM_COUNT}) Allow to run the simulation N times to benchmark the simulator
		-d|--output_dir < /abs/path >   $(_prt_cur_arg ${_RUN_DIR_OVERRIDE}) Change the run directory output

	FLAGS:
		-g|--generate_bench             $(_prt_cur_arg ${_GENERATE_BENCH}) Generate input and output vector for test
		-o|--generate_output            $(_prt_cur_arg ${_GENERATE_OUTPUT}) Generate output vector for test given its input vector
		-c|--clean                      $(_prt_cur_arg off ) Clean temporary directory
		-l|--limit_ressource            $(_prt_cur_arg ${_LIMIT_RESSOURCE}) Force higher nice value and set hard limit for hardware memory to force swap more ***not always respected by system
		-v|--valgrind                   $(_prt_cur_arg ${_VALGRIND}) Run with valgrind
		-B|--best_coverage_off          $(_prt_cur_arg ${_BEST_COVERAGE_OFF}) Generate N vectors from --vector size batches until best node coverage is achieved
		-b|--batch_sim                  $(_prt_cur_arg ${_BATCH_SIM}) Use Batch mode multithreaded simulation
		-p|--perf                       $(_prt_cur_arg ${_USE_PERF}) Use Perf for monitoring execution
		-f|--force_simulate             $(_prt_cur_arg ${_FORCE_SIM}) Force the simulation to be executed regardless of the config

"
}

function config_help() {
printf "
	config.txt expects a single line of argument wich can be one of:
				--custom_args_file
				--arch_list	[list_name]*
								*memories        use VTR k6_N10_mem32k architecture
								*small_sweep     use a small set of timing architecture
								*full_sweep  	 sweep the whole vtr directory *** WILL FAIL ***
								*default         use the sample_arch.xml
				--simulate                       request simulation to be ran
				--no_threading                   do not use multithreading for this test ** useful if you have large test **
"
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
		echo "running benchmark @${NEW_RUN_DIR}"
		mkdir -p ${NEW_RUN_DIR}

		unlink regression_test/latest &> /dev/null || /bin/true
		rm -Rf regression_test/latest || /bin/true

		ln -s ${NEW_RUN_DIR} regression_test/latest
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

	if [ -e regression_test/latest ]; then
		unlink regression_test/latest || /bin/true
		rm -Rf regression_test/latest || /bin/true
	fi

}

function disable_failed() {
	failed_dir=$1
	log_file="${failed_dir}.log"

	if [ -e ${log_file} ]
	then

		for failed_benchmark in $(cat ${log_file})
		do
			THIS_BM="${NEW_RUN_DIR}/${failed_benchmark}/sim_param"
			if [ -f ${THIS_BM} ]
			then
				mv ${THIS_BM} ${THIS_BM}_disabled
			fi
		done
	fi
}

function mv_failed() {
	failed_dir=$1
	log_file="${failed_dir}.log"

	if [ -e ${log_file} ]
	then
		echo "Linking failed benchmark in failures $(basename ${failed_dir})"

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

#########################################
# Helper Functions
function flag_is_number() {
	case "_$2" in
		_) 
			echo "Passed an empty value for $1"
			help
			exit 120
		;;
		*)
			case $2 in
				''|*[!0-9]*) 
					echo "Passed a non number value [$2] for $1"
					help
					exit 120
				;;
				*)
					echo $2
				;;
			esac
		;;
	esac
}


# boolean type flags
_low_ressource_flag=""
_valgrind_flag=""
_batch_sim_flag=""
_use_best_coverage_flag=""
_perf_flag=""

# number type flags
_vector_flag=""
_timeout_flag=""
_simulation_threads_flag=""

_adder_definition_flag=""

function _set_if() {
	[ "$1" == "on" ] && echo "$2" || echo ""
}

function _set_flag() {
	_low_ressource_flag=$(_set_if ${_LIMIT_RESSOURCE} "--limit_ressource")
	_valgrind_flag=$(_set_if ${_VALGRIND} "--tool valgrind")
	_batch_sim_flag=$(_set_if ${_BATCH_SIM} "--batch")
	_use_best_coverage_flag=$(_set_if ${_BEST_COVERAGE_OFF} "--best_coverage")
	_perf_flag=$(_set_if ${_USE_PERF} "--tool perf")
	
	_vector_flag="-g ${_VECTORS}"
	_timeout_flag="--time_limit ${_TIMEOUT}s"
	_simulation_threads_flag=$([ "${_SIM_THREADS}" != "1" ] && echo "-j ${_SIM_THREADS}")

	_adder_definition_flag="--adder_type ${_ADDER_DEF}"

}

function parse_args() {
	while [[ "$#" > 0 ]]
	do 
		case $1 in 

		# Help Desk
			-h|--help)
				echo "Printing Help information"
				help
				exit_program
			
		## directory in benchmark
			;;-t|--test)
				# this is handled down stream
				if [ "_$2" == "_" ]
				then 
					echo "empty argument for $1"
					exit 120
				fi

				_TEST="$2"
				echo "Running test $2"
				shift

		## absolute path
			;;-a|--adder_def)

				if [ "_$2" == "_" ]
				then 
					echo "empty argument for $1"
					exit 120
				fi
				
				_ADDER_DEF=$2

				if [ "${_ADDER_DEF}" != "default" ] && [ "${_ADDER_DEF}" != "optimized" ] && [ ! -f "$(readlink -f ${_ADDER_DEF})" ]
				then
					echo "invalid adder definition passed in ${_ADDER_DEF}"
					exit 120
				fi

				shift

			;;-d|--output_dir)

				if [ "_$2" == "_" ]
				then 
					echo "empty argument for $1"
					exit 120
				fi
				
				_RUN_DIR_OVERRIDE=$2

				if [ ! -d "${_RUN_DIR_OVERRIDE}" ]
				then
					echo "Directory ${_RUN_DIR_OVERRIDE} does not exist"
					exit 120
				fi

				shift

		## number
			;;-j|--nb_of_process)
				_NUMBER_OF_PROCESS=$(flag_is_number $1 $2)
				echo "Using [$2] processors for this benchmarking suite"
				shift

			;;-s|--sim_threads)
				_SIM_THREADS=$(flag_is_number $1 $2)
				echo "Using [$2] processors for synthesis and simulation"
				shift

			;;-V|--vectors)
				_VECTORS=$(flag_is_number $1 $2)
				echo "Using [$2] vectors for synthesis and simulation"
				shift

			;;-T|--timeout)
				_TIMEOUT=$(flag_is_number $1 $2)
				echo "Using timeout [$2] seconds for synthesis and simulation"
				shift

			;;-n|--simulation_count)
				_SIM_COUNT=$(flag_is_number $1 $2)
				echo "Simulating [$2] times"
				shift

		# Boolean flags
			;;-g|--generate_bench)		
				_GENERATE_BENCH="on"
				echo "generating output vector for test given predefined input"

			;;-o|--generate_output)		
				_GENERATE_OUTPUT="on"
				echo "generating input and output vector for test"

			;;-c|--clean)				
				echo "Cleaning temporary run in directory"
				cleanup_temp

			;;-l|--limit_ressource)		
				_LIMIT_RESSOURCE="on"
				echo "limiting ressources for benchmark, this can help with small hardware"

			;;-v|--valgrind)			
				_VALGRIND="on"
				echo "Using Valgrind for benchmarks"

			;;-B|--best_coverage_off)	
				_BEST_COVERAGE_OFF="off"
				echo "turning off using best coverage for benchmark vector generation"

			;;-b|--batch_sim)			
				_BATCH_SIM="on"
				echo "Using Batch multithreaded simulation with -j threads"

			;;-p|--perf)
				_USE_PERF="on"
				echo "Using perf for synthesis and simulation"
			
			;;-f|--force_simulate)   
				_FORCE_SIM="on"
				echo "Forcing Simulation"         

			;;*) 
				echo "Unknown parameter passed: $1"
				help 
				ctrl_c
		esac
		shift
	done
}


function sim() {

	####################################
	# parse the function commands passed
	with_custom_args="0"
	arch_list="no_arch"
	with_sim="0"
	threads=${_NUMBER_OF_PROCESS}
	DEFAULT_CMD_PARAM="${_adder_definition_flag} ${_simulation_threads_flag} ${_batch_sim_flag}"

	_SYNTHESIS="on"
	_SIMULATE=${_FORCE_SIM}

	if [ ! -d "$1" ]
	then
		echo "invalid benchmark directory passed to simulation function $1"
		ctrl_c
	fi
	benchmark_dir="$1"
	shift

	while [[ "$#" > 0 ]]
	do 
		case $1 in
			--custom_args_file) 
				with_custom_args=1
				;;

			--arch_list)
				case $2 in
					memories)
						arch_list="${MEM_ARCH}"
						;;

					small_sweep)
						arch_list="${SMALL_ARCH_SWEEP}"
						;;

					full_sweep)
						arch_list="${FULL_ARCH_SWEEP}"
						;;

					default)
						arch_list="${DEFAULT_ARCH}"
						;;
					*)
						;;
				esac
				shift
				;;

			--simulate)
				_SIMULATE="on"
				;;

			--no_threading)
				echo "This test will not be multithreaded"
				threads="1"
				;;

			*)
				echo "Unknown internal parameter passed: $1"
				config_help 
				ctrl_c
				;;
		esac
		shift
	done

	###########################################
	# run custom benchmark
	bench_type=${benchmark_dir##*/}
	echo " BENCHMARK IS: ${bench_type}"

	if [ "_${with_custom_args}" == "_1" ]
	then

		global_odin_failure="${NEW_RUN_DIR}/odin_failures"

		for dir in ${benchmark_dir}/*
		do
			if [ -e ${dir}/odin.args ]
			then
				test_name=${dir##*/}
				TEST_FULL_REF="${bench_type}/${test_name}"

				DIR="${NEW_RUN_DIR}/${bench_type}/$test_name"
				blif_file="${DIR}/odin.blif"

				#build commands
				mkdir -p $DIR
				wrapper_odin_command="${WRAPPER_EXEC}
											--log_file ${DIR}/odin.log
											--test_name ${TEST_FULL_REF}
											--failure_log ${global_odin_failure}.log
											${_timeout_flag}
											${_low_ressource_flag}
											${_valgrind_flag}"
											
				if [ "${_USE_PERF}" == "on" ]
				then
					wrapper_odin_command="${wrapper_odin_command} ${_perf_flag} ${DIR}/perf.data"
				fi

				odin_command="${DEFAULT_CMD_PARAM}
								$(cat ${dir}/odin.args | tr '\n' ' ') 
								-o ${blif_file} 
								-sim_dir ${DIR}"

				echo $(echo "${wrapper_odin_command} ${odin_command}" | tr '\n' ' ' | tr -s ' ' ) > ${DIR}/odin_param
			fi
		done

		#run the custon command
		echo " ========= Synthesizing Circuits"
		find ${NEW_RUN_DIR}/${bench_type}/ -name odin_param | xargs -n1 -P$threads -I test_cmd ${SHELL} -c '$(cat test_cmd)'
		mv_failed ${global_odin_failure}

	############################################
	# run benchmarks
	else

		global_synthesis_failure="${NEW_RUN_DIR}/synthesis_failures"
		global_simulation_failure="${NEW_RUN_DIR}/simulation_failures"


		for benchmark in $(ls ${benchmark_dir} | grep -e ".v" -e ".blif")
		do
			benchmark="${benchmark_dir}/${benchmark}"
			basename=""
			case "${benchmark}" in
				*.v)
					_SYNTHESIS="on"
					basename=${benchmark%.v}
				;;
				*.blif)
					# this is a blif file
					_SYNTHESIS="off"
					basename=${benchmark%.blif}
				;;
				*)
					continue
				;;
			esac

			test_name=${basename##*/}

			input_vector_file="${basename}_input"
			output_vector_file="${basename}_output"

			for arches in ${arch_list}
			do

				arch_cmd=""
				if [ -e ${arches} ]
				then
					arch_cmd="-a ${arches}"
				fi

				arch_basename=${arches%.xml}
				arch_name=${arch_basename##*/}

				TEST_FULL_REF="${bench_type}/${test_name}/${arch_name}"

				DIR="${NEW_RUN_DIR}/${TEST_FULL_REF}"
				blif_file="${DIR}/odin.blif"


				#build commands
				mkdir -p $DIR

				###############################
				# Synthesis
				if [ "${_SYNTHESIS}" == "on" ]
				then
					wrapper_synthesis_command="${WRAPPER_EXEC}
												--log_file ${DIR}/synthesis.log
												--test_name ${TEST_FULL_REF}
												--failure_log ${global_synthesis_failure}.log
												${_timeout_flag}
												${_low_ressource_flag}
												${_valgrind_flag}"

					if [ "${_USE_PERF}" == "on" ]
					then
						wrapper_synthesis_command="${wrapper_synthesis_command} ${_perf_flag} ${DIR}/perf.data"
					fi

					synthesis_command="${DEFAULT_CMD_PARAM}
										${arch_cmd}
										-V ${benchmark}
										-o ${blif_file}
										-sim_dir ${DIR}"

					echo $(echo "${wrapper_synthesis_command} ${synthesis_command}"  | tr '\n' ' ' | tr -s ' ') > ${DIR}/cmd_param
				fi

				if [ "${_SIMULATE}" == "on" ]
				then
					wrapper_simulation_command="${WRAPPER_EXEC}
											--log_file ${DIR}/simulation.log
											--test_name ${TEST_FULL_REF}
											--failure_log ${global_simulation_failure}.log
											${_timeout_flag}
											${_low_ressource_flag}
											${_valgrind_flag}"

					if [ "${_USE_PERF}" == "on" ]
					then
						wrapper_simulation_command="${wrapper_simulation_command} ${_perf_flag} ${DIR}/perf.data"
					fi

					simulation_command="${DEFAULT_CMD_PARAM}
											${arch_cmd}
											-b ${blif_file}
											-sim_dir ${DIR}
											${HOLD_PARAM}"

					if [ "${_GENERATE_BENCH}" == "on" ] || [ ! -f ${input_vector_file} ]
					then
						simulation_command="${simulation_command} ${_use_best_coverage_flag} ${_vector_flag}"
					else
						simulation_command="${simulation_command} -t ${input_vector_file}"
						if [ "${_GENERATE_OUTPUT}" == "off" ] && [ -f ${output_vector_file} ]
						then
							simulation_command="${simulation_command} -T ${output_vector_file}"
						fi
					fi

					echo $(echo "${wrapper_simulation_command} ${simulation_command}" | tr '\n' ' ' | tr -s ' ') > ${DIR}/sim_param
				fi

			done
		done

		#synthesize the circuits
		SYNTH_LIST=$(find ${NEW_RUN_DIR}/${bench_type}/ -name cmd_param)
		if [ "${_SYNTHESIS}" == "on" ] && [ "_${SYNTH_LIST}" != "_" ]
		then
			echo " ========= Synthesizing Circuits"
			find ${NEW_RUN_DIR}/${bench_type}/ -name cmd_param | xargs -n1 -P$threads -I test_cmd ${SHELL} -c '$(cat test_cmd)'
			# disable simulations on failure
			disable_failed ${global_synthesis_failure}

			mv_failed ${global_synthesis_failure}
		fi

		SIM_LIST=$(find ${NEW_RUN_DIR}/${bench_type}/ -name sim_param)
		if [ "${_SIMULATE}" == "on" ] && [ "_${SIM_LIST}" != "_" ]
		then
			echo " ========= Simulating Circuits"

			for i in $(seq 1 1 ${_SIM_COUNT}); do
				echo " Itteration: ${i}"

				#run the simulation
				find ${NEW_RUN_DIR}/${bench_type}/ -name sim_param | xargs -n1 -P$threads -I sim_cmd ${SHELL} -c '$(cat sim_cmd)'
				
				# move the log
				for sim_log in $(find ${NEW_RUN_DIR}/${bench_type}/ -name "simulation.log")
				do
					mv ${sim_log} "${sim_log}_${i}"
				done

				disable_failed ${global_simulation_failure}

			done
			
			mkdir -p ${NEW_RUN_DIR}/${bench_type}/vectors

			# move the vectors
			for sim_input_vectors in $(find ${NEW_RUN_DIR}/${bench_type}/ -name "input_vectors")
			do
				BM_DIR=$(dirname ${sim_input_vectors})
				BM_NAME="$(basename $(readlink -f ${BM_DIR}/..))_input"

				cp ${sim_input_vectors} ${NEW_RUN_DIR}/${bench_type}/vectors/${BM_NAME}
				mv ${sim_input_vectors} ${BM_DIR}/${BM_NAME}
				
			done


			# move the vectors
			for sim_output_vectors in $(find ${NEW_RUN_DIR}/${bench_type}/ -name "output_vectors")
			do
				BM_DIR=$(dirname ${sim_output_vectors})
				BM_NAME="$(basename $(readlink -f ${BM_DIR}/..))_output"

				cp ${sim_output_vectors} ${NEW_RUN_DIR}/${bench_type}/vectors/${BM_NAME}
				mv ${sim_output_vectors} ${BM_DIR}/${BM_NAME}

			done

			# move the failed runs
			mv_failed ${global_simulation_failure}
		fi

	fi
}

function run_failed() {
	FAILED_RUN_DIR="regression_test/latest"

	#synthesize the circuits
	if [ -d ${FAILED_RUN_DIR}/synthesis_failures ]; then
		echo " ========= Synthesizing Circuits"
		find -L ${FAILED_RUN_DIR}/synthesis_failures/ -name cmd_param | xargs -n1 -I test_cmd ${SHELL} -c '$(cat test_cmd)'
	fi

	#run the simulation
	if [ -d ${FAILED_RUN_DIR}/simulation_failures ]; then
		echo " ========= Simulating Circuits"
		find -L ${FAILED_RUN_DIR}/simulation_failures/ -name sim_param | xargs -n1 -I sim_cmd ${SHELL} -c '$(cat sim_cmd)'
	fi
}

function debug_failures() {
	FAILED_RUN_DIR="regression_test/latest"
	FAILURE_LOG=""
	CMD_FILE_NAME=""
	FAILURES_LIST=""

	case $1 in
		simulation)
			FAILURE_LOG="${FAILED_RUN_DIR}/simulation_failures.log"
			CMD_FILE_NAME="sim_param"
			;;
		synthesis)
			FAILURE_LOG="${FAILED_RUN_DIR}/synthesis_failures.log"
			CMD_FILE_NAME="cmd_param"
			;;
		*)
			echo "Wrong input"
			exit 1
			;;
	esac


	if [ ! -f ${FAILURE_LOG} ]
	then
		echo "Nothing to debug in ${FAILED_RUN_DIR}, Exiting"
	else
		FAILURES_LIST="$(cat ${FAILURE_LOG})"
		echo "Entering interactive mode"
		while /bin/true; do

			echo "Which benchmark would you like to debug (type 'quit' or 'q' to exit)?"
			echo "============"
			echo "${FAILURES_LIST}"	
			echo "============"
			printf "enter a substring: "

			read INPUT_BM
			case ${INPUT_BM} in
				quit|q)
					echo "exiting"
					break
					;;
				*)					
					BM="${FAILED_RUN_DIR}/$(echo "${FAILURES_LIST}" | grep ${INPUT_BM} | tail -n 1)"

					if [ "_${BM}" != "_" ] && [ -f "${BM}/${CMD_FILE_NAME}" ]
					then
						CMD_PARAMS="${WRAPPER_EXEC} --tool gdb"
						START_REBUILD="off"
						for tokens in $(cat ${BM}/${CMD_FILE_NAME}); do
							case ${tokens} in
								--adder_type)
									START_REBUILD="on"
									;;
								*)
									;;
							esac
							if [ "${START_REBUILD}" == "on" ];
							then
								CMD_PARAMS="${CMD_PARAMS} ${tokens}"
							fi
						done

						echo " ---- Executing: ${CMD_PARAMS}"
						/bin/bash -c "${CMD_PARAMS}"
					else
						echo " ---- Invalid input: ${INPUT_BM} where we did not find ${BM}"
					fi
					;;
			esac
		done
	fi
}

HEAVY_LIST=(
	"full"
	"large"
)

LIGHT_LIST=(
	"operators"
	"arch"
	"other"
	"micro"	
	"syntax"
	"FIR"
)

function run_sim_on_directory() {
	test_dir=$1

	if [ ! -d ${BENCHMARK_DIR}/${test_dir} ]
	then
		echo "${BENCHMARK_DIR}/${test_dir} Not Found! Skipping this test"
	elif [ ! -f ${BENCHMARK_DIR}/${test_dir}/config.txt ]
	then
		echo "no config file found in the directory ${BENCHMARK_DIR}/${test_dir}"
		echo "please make sure a config.txt exist"
		config_help
	else
		create_temp
		sim ${BENCHMARK_DIR}/${test_dir} $(cat ${BENCHMARK_DIR}/${test_dir}/config.txt)
	fi
}

function run_light_suite() {
	for test_dir in ${LIGHT_LIST[@]}; do
		run_sim_on_directory ${test_dir}
	done
}

function run_heavy_suite() {
	for test_dir in ${HEAVY_LIST[@]}; do
		run_sim_on_directory ${test_dir}
	done
}

function run_all() {
	run_light_suite
	run_heavy_suite
}

function run_vtr_reg() {
	cd ${VTR_ROOT_DIR}
	/usr/bin/perl run_reg_test.pl -j ${_NUMBER_OF_PROCESS} $1
	cd ${ODIN_ROOT_DIR}
}

#########################################################
#	START HERE

START=`get_current_time`

init_temp

parse_args $INPUT
_set_flag

if [ "_${_TEST}" == "_" ]
then
	echo "No input test!"
	help
	print_time_since $START
	exit_program
fi

echo "Benchmark is: ${_TEST}"
case ${_TEST} in

	failures)
		run_failed
		;;

	debug_sim)
		debug_failures "simulation"
		;;

	debug_synth)
		debug_failures "synthesis"
		;;

	full_suite)
		run_all
		;;	
		
	heavy_suite)
		run_heavy_suite
		;;

	light_suite)
		run_light_suite
		;;

	vtr_reg_*)
		run_vtr_reg ${_TEST}
		;;

	pre_commit)
		run_all
		run_vtr_reg vtr_reg_basic
		run_vtr_reg vtr_reg_valgrind_small
		;;

	pre_merge)
		run_all
		run_vtr_reg vtr_reg_basic
		run_vtr_reg vtr_reg_strong
		run_vtr_reg vtr_reg_valgrind_small
		run_vtr_reg vtr_reg_valgrind
		;;

	*)
		run_sim_on_directory ${_TEST}
		;;
esac

print_time_since $START

exit_program
### end here
