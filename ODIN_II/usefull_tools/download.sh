#!/bin/bash

if [ $# -ne 1 ];then
  echo "Usage: `basename $0` FILE"
  exit 1
fi

set -e
CURRENT_DIR=${PWD}

extract () {
    FILENAME=$1
    if [ "_${FILENAME}" != "_" ] && [ -f ${FILENAME} ] ; then
        case $1 in
            *.tar.bz2)  mv ${FILENAME} ${FILENAME%.*.*}.tbz  ;;
            *.tar.gz)   mv ${FILENAME} ${FILENAME%.*.*}.tgz  ;;
        esac

        case ${FILENAME} in
            *.tar)      tar xf ${FILENAME}; rm -f ${FILENAME};;
            *.tbz2)     tar xjf ${FILENAME}; rm -f ${FILENAME};;
            *.tgz)      tar xzf ${FILENAME}; rm -f ${FILENAME};;
            *.zip)      unzip ${FILENAME}; rm -f ${FILENAME};;
            *.gz)       gunzip ${FILENAME}; rm -f ${FILENAME};;
            *.bz2)      bunzip2 ${FILENAME}; rm -f ${FILENAME};;
        esac
    fi
}

file_exist() {
    ( [ "_$1" != "_" ] && timeout 5 curl --output /dev/null --silent --fail -r 0-0 $1 ) && echo "success" || echo "failed" 
}

clean_up_directory() {
    # files to remove
    ripe_for_removal="__MACOSX"

    # check if its in a subdirectory
    FILE_COUNT=$(ls -1 | wc -l)
    while /bin/true; do
        if [ "${FILE_COUNT}" == "0" ]
        then
            echo "failed"
            exit 1
        else
            rm -Rf ${ripe_for_removal}

            if [ "${FILE_COUNT}" == "1" ]
            then
                DOWNLOADED_FILE="$(ls)"
                if [ -d "${DOWNLOADED_FILE}" ]
                then
                    mv ${DOWNLOADED_FILE}/* ./
                    rm -Rf ${DOWNLOADED_FILE}
                else
                    break
                fi
            else
                break
            fi
        fi
    done
}

download() {
    url=$1
    output_dir=$(basename ${url} | cut -d '.' -f 1)
    valid_url=$(file_exist ${url})

    if [ ${valid_url} == "success" ]; then
        TEMP_DIR=$(mktemp -d)
        cd ${TEMP_DIR}
        wget ${url}
        
        DOWNLOADED_FILE="$(ls)"

        extract "${DOWNLOADED_FILE}"

        clean_up_directory

        cd ${CURRENT_DIR}
        mv ${TEMP_DIR}/* ${output_dir}
    else
        exit 1
    fi
}

download $1