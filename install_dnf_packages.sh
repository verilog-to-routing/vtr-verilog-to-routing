#!/bin/bash

# if --dev is specified, also install packages needed by vtr developers (e.g. verilator
# for functional simulation). otherwise only install packages needed to run vtr.
install_dev=false
for arg in "$@"; do
    if [ "$arg" = "--dev" ]; then
        install_dev=true
    fi
done

# Base packages to compile and run basic regression tests
sudo dnf install --refresh -y \
    make \
    cmake \
    automake \
    gcc \
    gcc-c++ \
    kernel-devel \
    pkg-config \
    bison \
    flex \
    python3-devel \
    tbb-devel

# Required for parsing SDC files (see LibSDCParse)
sudo dnf install --refresh -y \
    tcl8-devel \
    swig

# Required for graphics
sudo dnf install --refresh -y \
    qt6-qtbase-devel \
    qt6-qtsvg-devel \
    qt6-qtbase-private-devel \
    qt6-qtshadertools \
    qt6-qtshadertools-devel \
    libxkbcommon-devel \
    mesa-libGL-devel \
    mesa-libEGL-devel \
    libglvnd-opengl \
    mesa-dri-drivers

# Required for parmys front-end from https://github.com/YosysHQ/yosys
sudo dnf install --refresh -y \
    make \
    automake \
    gcc \
    gcc-c++ \
    kernel-devel \
    clang \
    bison \
    flex \
    readline-devel \
    gawk \
    tcl-devel \
    libffi-devel \
    git \
    graphviz \
    python-xdot \
    pkg-config \
    python3-devel \
    boost-system \
    boost-python3 \
    boost-filesystem \
    zlib-ng-devel

# Required to run the analytical placement flow
sudo dnf install --refresh -y \
    eigen3-devel

if [ "$install_dev" = true ]; then
    # required for functional simulation (run_func_sim_flow.py)
    sudo dnf install --refresh -y \
        verilator
fi