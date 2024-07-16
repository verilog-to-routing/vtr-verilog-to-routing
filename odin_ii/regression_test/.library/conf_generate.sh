#!/bin/bash

function echo_bm_conf() {

    _input_bm_dir="$1"
    _reg_exec_name="verify_odin.sh"
    if [ "_${_input_bm_dir}" == "_" ]
    then
        echo ""
    elif [ ! -d "${_input_bm_dir}" ]
    then
        echo ""
    else
        # get the full name
        _input_bm_dir=$(readlink -f ${_input_bm_dir})
        _temp_bm_dir="${_input_bm_dir}"
        # now find the location of the reg root
        while [ "_${_temp_bm_dir}" != "_" ] && [ "_${_temp_bm_dir}" != "_/" ] && [ "_$(find ${_temp_bm_dir} -name ${_reg_exec_name})" == "_" ]
        do
            _temp_bm_dir="$(dirname ${_temp_bm_dir})"
        done

        if [ "_${_temp_bm_dir}" == "_" ] || [ "_${_temp_bm_dir}" == "_/" ]
        then
            echo ""
        else
            path_to_exec=$(find ${_temp_bm_dir} -name ${_reg_exec_name})
            path_to_exec=$(dirname ${path_to_exec})
            realative_path_from_exec=$(realapath_from ${_input_bm_dir} ${path_to_exec} )

            _bm_name=$(basename ${realative_path_from_exec})


            circuit_list_input=""
            for files in $(find ${_input_bm_dir} -type f -name "*.v")
            do
                circuit_path=$(realapath_from ${files} ${_input_bm_dir} )
                circuit_list_input="\
circuit_list_add=${circuit_path}
${circuit_list_input}"
            done

            for files in $(find ${_input_bm_dir} -type f -name "*.blif")
            do
                circuit_path=$(realapath_from ${files} ${_input_bm_dir} )
                circuit_list_input="\
circuit_list_add=${circuit_path}
${circuit_list_input}"
            done

            # circuit_list_input=$(printf "${circuit_list_input}" | sort | sed 's/\s+/\\n/g')
            # circuit_list_input=$(printf "${circuit_list_input}")

            echo "\
########################
# ${_bm_name} benchmarks config
########################

regression_params=--include_default_arch
script_simulation_params= --verbose --time_limit 3600s
script_synthesis_params= --verbose --time_limit 3600s
simulation_params= -L reset rst -H we -g 2

# setup the architecture
archs_dir=../vtr_flow/arch/timing

arch_list_add=k6_N10_40nm.xml
arch_list_add=k6_N10_mem32K_40nm.xml
#arch_list_add=k6_frac_N10_frac_chain_mem32K_40nm.xml

# setup the circuits
circuits_dir=${realative_path_from_exec}

${circuit_list_input}"
        fi
    fi
}