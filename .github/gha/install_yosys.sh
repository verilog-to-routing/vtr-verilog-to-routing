#!/usr/bin/env bash

# local binaries location
export PATH="$HOME/.local/bin:$PATH"

if [ ! -e $HOME/.local/bin/yosys ]; then
    echo '================================================================'
    echo '                        Building yosys'
    echo '================================================================'
    mkdir -p ~/.local
    mkdir -p ~/.local/src
    cd ~/.local/src
    git clone https://github.com/YosysHQ/yosys.git

    cd yosys
    make -j$(nproc) PREFIX=$HOME/.local
    make install PREFIX=$HOME/.local 
fi

echo $(which yosys)