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
