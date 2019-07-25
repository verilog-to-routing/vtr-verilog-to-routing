#!/bin/bash
set -e

OUTPUT_FILE="./coallesced.csv"
rm -f ${OUTPUT_FILE} && touch ${OUTPUT_FILE}

print_line() {
    echo $@ >> ${OUTPUT_FILE}
}

HEADER=""

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
					for LOGS in ${ARCH}/*.csv
					do
                        if [ "_${HEADER}" == "_" ]
                        then
                            HEADER="ITTER,$(head -n1 ${LOGS})"
                            print_line "${HEADER}"
                        fi
                        ItterNb=$(basename ${LOGS})
						# echo ${ItterNb%.csv}
                        LINE="${ItterNb%.csv},$(tail -n1 ${LOGS})"
                        print_line "${LINE}"
					done	
				done
			done
		done	
	done
done
