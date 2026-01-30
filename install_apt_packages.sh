#!/bin/bash

# Base packages to compile and run basic regression tests
packages_to_install=(
    make
    cmake
    build-essential
    pkg-config
    bison
    flex
    python3-dev
    python3-venv
)

# Packages for more complex features of VTR that most people will use.
packages_to_install+=(
    libxml2-utils
    libtbb-dev
    libeigen3-dev
)

# Required for graphics
packages_to_install+=(
    libgtk-3-dev
    libx11-dev
)

# Required for parmys front-end from https://github.com/YosysHQ/yosys
packages_to_install+=(
    build-essential
    clang
    bison
    flex
    libreadline-dev
    gawk
    tcl-dev
    libffi-dev
    git
    graphviz
    xdot
    pkg-config
    python3-dev
    libboost-system-dev
    libboost-python-dev
    libboost-filesystem-dev
    default-jre
    zlib1g-dev
)

# Required for code formatting
# NOTE: clang-format-18 may only be found on specific distributions. Only
#       install it if the distribution has this version of clang format.
if apt-cache search '^clang-format-18$' | grep -q 'clang-format-18'; then
    packages_to_install+=(
        clang-format-18
    )
else
    echo "clang-format-18 not found in apt-cache. Skipping installation."
fi

sudo apt-get update
sudo apt-get install -y "${packages_to_install[@]}"

