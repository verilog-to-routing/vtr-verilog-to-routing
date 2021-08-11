#!/usr/bin/env bash

# local binaries location
export PATH="$HOME/.local/bin:$PATH"

if [ ! -e $HOME/.local/bin/yosys ]; then
    echo '================================================================'
    echo '                        Building yosys'
    echo '================================================================'
    mkdir -p ~/.local
    # Yosys is added to VTR fixed at the b96eb888 commit
    # mkdir -p ~/.local/src
    # cd ~/.local/src
    # git clone https://github.com/YosysHQ/yosys.git

    # cd to VTR_ROOT/yosys submodule
    cd ./yosys
    make -j$(nproc) PREFIX=$HOME/.local
    make install PREFIX=$HOME/.local 
    # cd back to VTR_ROOT
    cd ../
fi

echo $(which yosys)