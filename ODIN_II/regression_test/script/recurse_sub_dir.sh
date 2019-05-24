#!/bin/bash
set -e

BASE=$(readlink -f "./logs")
for BUFFER_SIZE in ${BASE}/*
do
	BS=$(basename ${BUFFER_SIZE})
	#echo "buffer size: ${BS}"
	for SIM_TYPE in ${BUFFER_SIZE}/*
	do
		ST=$(basename ${SIM_TYPE})
		#echo "sim available: ${ST}"
		for THRD_CNT in ${SIM_TYPE}/*
		do
			TC=$(basename ${THRD_CNT})
			#echo "n thread:${TC}"
			for TEST_NAME in ${THRD_CNT}/*
			do
				TN=$(basename ${TEST_NAME})
				#echo "test name: ${TN}"
				for ARCH in ${TEST_NAME}/*
				do
					AN=$(basename ${ARCH})
					#echo "arch name: ${AN}"
					I=0
					for LOGS in ${ARCH}/*.log*
					do
						I=$(( ${I} + 1 ))
						/bin/python parse_odin_result.py ${LOGS} $(dirname ${LOGS})/${I}.csv Buffer_Size ${BS} Sim_Type ${ST} Thread_Count ${TC} Verilog_File ${TN} Architecture ${AN} 
					done	
				done
			done
		done	
	done
done
