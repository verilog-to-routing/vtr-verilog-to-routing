#!/usr/bin/env bash

set -xe

PATH_FROM_ODIN_ROOT="regression_test/benchmark/third_party/SymbiFlow"

# init the output directory
if [ ! -d sv-tests ];
then
    mkdir sv-tests
    pushd sv-tests

    # sparse checkout the tests directory
    git init .
    git remote add -f origin https://github.com/SymbiFlow/sv-tests.git
    git config core.sparseCheckout true
    echo "tests" >> .git/info/sparse-checkout
    popd
fi

# fetch changes
cd sv-tests; git pull --depth=1 origin master; cd ..

# generate the tasks and the tasklist
printf "" > task_list.conf
for dir in sv-tests/tests/chapter-*
do 
    cat << EOF > ${dir}/task.conf
########################
# large benchmarks config
########################

regression_params=--disable_simulation
script_synthesis_params=--limit_ressource --time_limit 3600s
synthesis_params=--permissive

# setup the architecture
archs_dir=../vtr_flow/arch/timing

# one arch allows it to run faster
arch_list_add=k6_frac_N10_frac_chain_mem32K_40nm.xml

circuits_dir=${PATH_FROM_ODIN_ROOT}/${dir}

# glob all the benchmark!
circuit_list_add=*.sv

synthesis_parse_file=regression_test/parse_result/conf/synth.conf
simulation_parse_file=regression_test/parse_result/conf/sim.conf

EOF
    echo "${PATH_FROM_ODIN_ROOT}/${dir}" >> task_list.conf
done