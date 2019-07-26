#!/bin/bash

#########################################
# Helper Functions
function _flag_is_number() {
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

function _set_if() {
	[ "$1" == "on" ] && echo "$2" || echo ""
}

function _echo_args() {
	echo $@ | tr '\n' ' ' | tr -s ' '
}

function _cat_args() {
	_echo_args "$(cat $1)"
}

function fetch_cursor_position() {
  local pos

  IFS='[;' read -p $'\e[6n' -d R -a pos -rs || echo "failed with error: $? ; ${pos[*]}"
  echo "${pos[1]}:${pos[2]}"
}

function realapath_from() {
	original_path="$1"
	relative_to="$2"

	reconstructed_path=""

	# we now have the full path of both, 
	# we trim original_path until it has a common path with relative_to
	# a: /path/to/dir/a/somewhere
	# b: /path/to/dir/b/somewhere
	while [ "_${original_path}" != "_" ] && [ "${original_path}" != "/" ] && [ "_$(echo ${relative_to} | grep ${original_path})" == "_" ]
	do
		if [ "_${reconstructed_path}" == "_" ]
		then
			reconstructed_path="$( basename ${original_path} )"
		else
			reconstructed_path="$( basename ${original_path} )/${reconstructed_path}"
		fi
		original_path=$( dirname ${original_path} )
	done

	# original path now holds the common path to the relative_path
	# a: /path/to/dir
	# b: /path/to/dir/b/somewhere
	# c: a/somewhere

	if [ "_${original_path}" != "_" ] && [ "${original_path}" != "/" ]
	then 
		relative_to="$(echo ${relative_to} | sed s+`echo ${original_path}`++g)"
	fi

	# original path now holds the path above
	# b: b/somewhere
	# c: a/somewhere

	while [ "_${relative_to}" != "_" ] && [ "${relative_to}" != "/" ]
	do
		reconstructed_path="../${reconstructed_path}"
		relative_to=$( dirname ${relative_to} )
	done

	# reconstructed_path is complete
	# c: ../../a/somewhere

	echo ${reconstructed_path}
}