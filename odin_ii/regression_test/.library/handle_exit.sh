#!/bin/bash

export EXIT_NAME=""

function _kill_process_tree() {
	for children in $(ps -o pid= --ppid "$1")
	do
		_kill_process_tree "${children}"
		kill -s SIGINT "${children}" &> /dev/null
	done
}

function _kill_all() {
	for job_id in $(jobs -p)
	do
		_kill_process_tree "${job_id}"
		kill -s SIGINT "${job_id}" &> /dev/null
	done

	_kill_process_tree $$
}

function _exit_with_code() {
	_kill_all
	exit $1
}

function _hard_exit_with_code() {	
    exit_status=$(( $? - 128 ))
    trap '' INT SIGINT SIGTERM
	echo "** ${EXIT_NAME} EXITED FORCEFULLY **"
	_exit_with_code ${exit_status}
}

trap _hard_exit_with_code SIGTERM INT SIGINT

