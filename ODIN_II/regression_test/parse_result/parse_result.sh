#!/usr/bin/env bash

THIS_SCRIPT_PATH=$(readlink -f "$0")
THIS_DIR=$(dirname "${THIS_SCRIPT_PATH}")
REGRESSION_DIR="${THIS_DIR}/.."
REG_LIB="${REGRESSION_DIR}/.library"

source ${REG_LIB}/handle_exit.sh
source  ${REG_LIB}/time_format.sh
source  ${REG_LIB}/helper.sh

export EXIT_NAME="$0"

RUN_TYPE=0
RUN_TYPE_NAME=(
    "INVALID"
    "parse"
    "join"
    "compare"
)

# add falback default to initialize the entries, 
# these can be overriden using the [default] tag
RULE_HEADER=( "default" )
RULE_THRESHOLD=( "skip" )
RULE_QUICK_COMPARE=( "" )
RULE_FULL_REGEX=( "" )
RULE_SORTING_KEY=( "false" )
RULE_DEFAULT_VALUE=( "n/a" )

RULE_NAME_ID=()
RULE_SIZE=1
CURRENT_ID=0

PARSE_FILE=()
PARSE_RULE_FILE=()
GOLDEN_FILE=""

V_VALUES=()
G_VALUES=()

OUTPUT_FILE=""

COLORIZE="on"

function trim() {
    echo "$*" | tr -s '[:blank:]' | tr -d '\n' | tr -d ',' | sed -e 's/^[[:blank:]]*//g' -e 's/[[:blank:]]*$//g' 
}

function parse_rule_file() {
    filename="$*"
    if [ "_${filename}" == "_" ] || [ ! -f "${filename}" ];
    then
        # echo "skipping parse rule file $1"
        return 1
    else
        # echo "Reading rule file ${filename}"
        mapfile -t rule_file_arr < "${filename}"
        # parse parsefile
        for line in "${rule_file_arr[@]}"
        do
            line="$(trim "${line}")"
            # drop comments
            case "_${line}" in
                _) # do nothing
                    ;;
                '_;include'*)
                    next_file="${line/;include[[:blank:]]/}"
                    filename="$(readlink -f "$*")"
                    filepath="$(dirname "${filename}")"
                    parse_rule_file  "${next_file}" || parse_rule_file  "${filepath}/${next_file}" 
                    ;;
                '_;'*)
                    # comments
                    ;;
                '_['*']')
                    header="$(echo "${line}" | sed -e 's/\[//g' -e 's/\]//g')"
                    header="$(trim "${header}")"
                    if [ "_${header}" == "_${RULE_HEADER[0]}" ]
                    then
                        CURRENT_ID=0
                    else
                        CURRENT_ID=${RULE_SIZE}
                        RULE_SIZE=$(( RULE_SIZE + 1 ))
                    fi
                    RULE_HEADER+=( "${header}" )
                    RULE_FULL_REGEX+=( "${RULE_FULL_REGEX[0]}" )
                    RULE_QUICK_COMPARE+=( "${RULE_QUICK_COMPARE[0]}" )
                    RULE_THRESHOLD+=( "${RULE_THRESHOLD[0]}" )
                    RULE_SORTING_KEY+=( "${RULE_SORTING_KEY[0]}" )
                    RULE_DEFAULT_VALUE+=( "${RULE_DEFAULT_VALUE[0]}" )

                    ;;
                '_'*'='*)
                    if [ "_${RULE_SIZE}" == "_0" ]
                    then
                        echo "You did not initialize the entry"
                        echo "please add a header [header] first"
                    else
                        k="$( trim "${line/=*/}" )"
                        v="$( trim "${line/[^=]*=/}" )"
                        case "_${k}" in
                            _regex)
                                RULE_FULL_REGEX["${CURRENT_ID}"]="${v}"
                                RULE_QUICK_COMPARE["${CURRENT_ID}"]="${v/[^[:alnum:][:blank:]]*/}"
                                ;;
                            _threshold)
                                RULE_THRESHOLD["${CURRENT_ID}"]="${v}"
                                ;;
                            _sorting_key)
                                RULE_SORTING_KEY["${CURRENT_ID}"]="${v}"

                                if [ "_${v}" == "_true" ] || [ "_${RULE_SORTING_KEY[0]}" == "_true" ]
                                then
                                    RULE_NAME_ID+=( "${CURRENT_ID}" )
                                fi
                                ;;  
                            _default)
                                RULE_DEFAULT_VALUE["${CURRENT_ID}"]="${v}"
                                ;;                             
                            _) #nothing
                            ;;
                            *)
                                echo "Invalid entry ${k} = ${v}"
                                echo "key are regex and threshold"
                                ;;
                            esac
                        fi
                    ;;

                *)
                    #malformed line
                    echo "dropping malformed line: ${line}" 
                    ;;
            esac
        done
    fi
}

function parse_rules() {
    for rule_file in "$@"
    do
        parse_rule_file "${rule_file}"
    done

    if [ "_${#RULE_HEADER[@]}" == "_1" ]
    then
        echo "Must pass a valid parse rule
                A parse rule is a file containing a list k:v of header:regexes with a single capture group in each
                i.e. <csv header>=<regex>
                it may contain a '#include path/to/file' directive to include other rule
        "
        exit 1
    fi
}

function array_to_csv() {
    OLD_IFS=${IFS}
    IFS=,
    printf '%s' "$*"
    IFS=${OLD_IFS}
}

function parse_arg() {

    for (( i = 0; i < ${#RUN_TYPE_NAME[@]} && RUN_TYPE == 0 ; i++ ))
    do
        if [ "_${RUN_TYPE_NAME[$i]}" == "_$1" ]
        then
            RUN_TYPE=$i
        fi
    done

    if [ -z ${RUN_TYPE} ]
    then
        echo "expected to specify one of: ${RUN_TYPE_NAME[*]}"
        exit 1
    fi
}

function parse_arg_for_parse() {
    while [ "_$1" != "_" ];
    do
        case $1 in
            --rule|-r)
                PARSE_RULE_FILE+=( "$2" )
                shift
            ;;
            --output|-o)
                OUTPUT_FILE="$2"
                shift
            ;;
            --color)
                COLORIZE="on"
            ;;
            *)
                if [ -f "$1" ]
                then
                    PARSE_FILE+=( "$1" )
                else
                    echo "invalid input $1"
                fi
            ;;
        esac

        shift
    done


    if [ "_${OUTPUT_FILE}" == "_" ]
    then
        echo "Expected an output file"
        exit 1
    fi
    if [ "_${PARSE_RULE_FILE[*]}" == "_" ]
    then
        echo "Expected a rule file"
        exit 1
    fi
    if [ "_${PARSE_FILE[*]}" == "_" ]
    then
        echo "Expected at least 1 log file to parse"
        exit 1
    fi
}

function do_parse() {
    parse_rules "${PARSE_RULE_FILE[@]}"

    csv_header="$(array_to_csv "${RULE_HEADER[@]:1}")"

    for file in "${PARSE_FILE[@]}";
    do 
        VALUE=()
        for (( i = 1; i < ${#RULE_HEADER[@]}; i++ ));
        do
            VALUE+=( "${RULE_DEFAULT_VALUE[$i]}" )
        done

        mapfile -t input <"${file}"
        for line in "${input[@]}";
        do 
            line="$(trim "${line}")"
            if [ "_${line}" != "_" ]
            then 
                for (( i = 1; i < ${#RULE_QUICK_COMPARE[@]}; i++ ));
                do
                    j=$(( i - 1 ))
                    case "${line}" in 
                        ${RULE_QUICK_COMPARE[$i]}*)
                            output="$(echo "${line}" | sed -r -n -e "s/${RULE_FULL_REGEX[$i]}/\1/p" 2> /dev/null)"
                            if [ "_${output}" != "_" ]
                            then
                                if [ "_${VALUE[$j]}" != "_${RULE_DEFAULT_VALUE[$i]}" ]
                                then
                                    echo "value already set ${RULE_HEADER[$i]}=${VALUE[$j]} with ${output}"
                                else
                                    VALUE[$j]="${output}"
                                fi
                            fi
                        ;;
                    esac
                done
            fi
        done
        V_VALUES+=( "$(array_to_csv "${VALUE[@]}")" )
    done

    ( for i in "${V_VALUES[@]}"; do echo "$i"; done ) \
        | column -o ', ' -s ',' -t --table --table-columns "${csv_header}" \
            > "${OUTPUT_FILE}"
    return 0
}

function compare_with_threshold() {
    new_value="$1"
    golden_value="$2"
    threshold="$3"

    case "_${threshold}" in
        _|_skip)
            true ;;
        _abs)
            [ "${new_value}" == "${golden_value}" ];;
        _*)     
            echo "${new_value}" "${golden_value}" "${threshold}" | \
                awk '( $1 > ($2 + ($2*$3)) || $1 < ($2 - ($2*$3))) {exit 1}';;
    esac

    return $?
}

function parse_arg_for_compare() {
    while [ "_$1" != "_" ];
    do
        case $1 in
            --rule|-r)
                PARSE_RULE_FILE+=( "$2" )
                shift
            ;;
            --golden|-g)
                GOLDEN_FILE="$2"
                shift  
            ;;
            --failure_log|-f)
                FAILURE_LOG_FILE="$2"
                shift
            ;;
            --color)
                COLORIZE="on"
            ;;
            *)
                if [ -f "$1" ]
                then
                    PARSE_FILE+=( "$1" )
                else
                    echo "invalid input $1"
                fi
            ;;
        esac

        shift
    done

    if [ "_${GOLDEN_FILE}" == "_" ]
    then
        echo "Expected an golden result file"
        exit 1
    fi
    if [ "_${PARSE_RULE_FILE[*]}" == "_" ]
    then
        echo "Expected a rule file"
        exit 1
    fi
    if [ "_${#PARSE_FILE[@]}" != "_1" ]
    then
        echo "Expected 1 files to compare to golden"
        exit 1
    fi
}

function do_compare() {
    parse_rules "${PARSE_RULE_FILE[@]}"

    fail_count=0
    mapfile -t V_VALUES <"${PARSE_FILE[0]}"
    mapfile -t G_VALUES <"${GOLDEN_FILE}"

    # skip header
    for (( i = 1; i < ${#V_VALUES[@]}; i++ ))
    do 
        mapfile -d ',' -t new_values < <( printf "%s" "${V_VALUES[$i]}" )
        mapfile -d ',' -t golden_values < <( printf "%s" "${G_VALUES[$i]}")

        failed_hdr=()
        for (( j = 1; j < ${#RULE_HEADER[@]}; j++ ))
        do
            k=$(( j - 1 ))
            golden_values[$k]="$( trim "${golden_values[$k]}" )"
            new_values[$k]="$( trim "${new_values[$k]}" )"
            if [ "_${new_values[$k]}" != "_${golden_values[$k]}" ] \
            && ! compare_with_threshold "${new_values[$k]}" "${golden_values[$k]}" "${RULE_THRESHOLD[$j]}";
            then
                failed_hdr+=( "$j" )
            fi
        done
        
        test_name=""
        for j in "${RULE_NAME_ID[@]}"
        do
            k=$(( j - 1 ))
            test_name="${test_name} ${new_values[$k]}"
        done
        test_name="$(trim "${test_name}")"

        if [ "_${#failed_hdr[@]}" != "_0" ]
        then
            fail_count=$(( fail_count + 1))

            if [ "${FAILURE_LOG_FILE}" != "_" ]
            then
                echo "${test_name}" >> "${FAILURE_LOG_FILE}"
            fi
            pretty_print_status "${COLORIZE}" red "${test_name}" "Failed" 
        
            for j in "${failed_hdr[@]}"
            do
                k=$(( j - 1 ))
                printf "%-36s" "${RULE_HEADER[$j]}"

                [ "_${COLORIZE}" == "_on" ] && printf "\033[0;31m"
                printf "[-%s-]" "${golden_values[$k]}"

                [ "_${COLORIZE}" == "_on" ] && printf "\033[0;32m"
                printf "{+%s+}" "${new_values[$k]}"

                [ "_${COLORIZE}" == "_on" ] && printf "\033[0m"
                
            done

        else
            pretty_print_status "${COLORIZE}" green "${test_name}" "Ok"  
        fi
    done
    return ${fail_count}
}

function parse_arg_for_join() {
    while [ "_$1" != "_" ];
    do
        case $1 in
            --output|-o)
                OUTPUT_FILE="$2"
                shift
            ;;
            --color)
                COLORIZE="on"
            ;;
            *)
                if [ -f "$1" ]
                then
                    PARSE_FILE+=( "$1" )
                else
                    echo "invalid input $1"
                fi
            ;;
        esac

        shift
    done

    if [ "_${OUTPUT_FILE}" == "_" ]
    then
        echo "Expected an output file"
        exit 1
    fi
     if [ "_${PARSE_FILE[*]}" == "_" ]
    then
        echo "Expected at least 1 log file to parse"
        exit 1
    fi
}

function do_join() {

    mapfile -t V_VALUES <"${PARSE_FILE[0]}"
    csv_header="${V_VALUES[0]}"
    V_VALUES=( "${V_VALUES[@]:1}" )

    for files in "${PARSE_FILE[@]}"
    do
        mapfile -t G_VALUES <"${files}"
        # skip header
        V_VALUES+=( "${G_VALUES[@]:1}" )
    done

    ( for i in "${V_VALUES[@]}"; do echo "$i"; done ) \
        | column -o ', ' -s ',' -t --table --table-columns "${csv_header}" \
        > "${OUTPUT_FILE}"

}

parse_arg "$1"

"parse_arg_for_${RUN_TYPE_NAME[${RUN_TYPE}]}" "${@:2}"
"do_${RUN_TYPE_NAME[${RUN_TYPE}]}"

exit $?

