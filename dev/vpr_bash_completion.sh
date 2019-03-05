#!/usr/bin/env bash

_vpr_completions() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local prev="${COMP_WORDS[COMP_CWORD-1]}"

    #Determine the valid VPR completions by parsing VPR's help message
    #This ensures we don't need to hard-code the possible completions here
    local vpr_opts="$(VPR_LOG_FILE='' vpr -h | grep '^  \-' | sed 's/, -/\n  -/' | awk '{print $1}' | sed 's/ //g' | paste -s -d ' ')"
    local vpr_cur_opt_choices=$(VPR_LOG_FILE='' vpr -h | grep '^  \-' | grep -w -- "${prev}" | grep '{' | sed 's/.*{//' | sed 's/}.*//' | sed 's/,//g')

    if [[ ${cur} == -* ]] ; then
        #Complete matching option
        COMPREPLY+=( $(compgen -W "${vpr_opts}" -- ${cur}) )
    elif [[ ${prev} == -* ]] && [[ -n "$vpr_cur_opt_choices" ]]; then
        #Previous is an option with a non-empty set of fixed completions
        COMPREPLY+=( $(compgen -W "${vpr_cur_opt_choices}" -- ${cur}) )
    else
        #Default to file completion
        COMPREPLY+=( $(compgen -f ${cur}) )
    fi
}

complete -o filenames -F _vpr_completions vpr
