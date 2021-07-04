#!/usr/bin/env bash

# local binaries location
export PATH="$HOME/.local/bin:$PATH"
# VTR path
export VTR_HOME=$(pwd)

if [ ! -e $HOME/.local/bin/yosys ]; then
    echo '=========================='
    echo 'Building yosys'
    echo '=========================='
    mkdir -p ~/.local
    mkdir -p ~/.local/src
    cd ~/.local/src
    git clone https://github.com/YosysHQ/yosys.git

    cd yosys
    make -j$(nproc) PREFIX=$HOME/.local
    make install PREFIX=$HOME/.local 
fi

echo $(which yosys)
cd $VTR_HOME

# generates benchmark BLIF files using Yosys
./ODIN_II/run_yosys.sh -t regression_test/benchmark/suite/_BLIF/techmap_suite/ --show_failure