#!/usr/bin/env bash

set -euo pipefail

echo "Installing OpenFPGA macOS dependencies..."

brew update

brew install \
    autoconf \
    automake \
    libtool \
    bison \
    ccache \
    cmake \
    ninja \
    pkgconf \
    gperf \
    flex \
    git \
    curl \
    wget \
    zip \
    perl \
    boost \
    eigen@3 \
    openssl@3 \
    zlib \
    libffi \
    readline \
    libxml2 \
    ncurses \
    python \
    tcl-tk \
    swig \
    graphviz \
    icarus-verilog \
    googletest \
    gawk \
    llvm \
    qt

# Apple Clang
export CC="$(xcrun --find clang)"
export CXX="$(xcrun --find clang++)"

# Qt6
export QT_PREFIX="$(brew --prefix qt)"
export PATH="${QT_PREFIX}/bin:${PATH}"
export CMAKE_PREFIX_PATH="${QT_PREFIX}:${CMAKE_PREFIX_PATH:-}"

# ccache
export CCACHE_DIR="${HOME}/.ccache"
mkdir -p "${CCACHE_DIR}"
ccache --set-config=max_size=2G

echo "CC=${CC}"
echo "CXX=${CXX}"
echo "Qt=${QT_PREFIX}"

clang++ --version
cmake --version
ninja --version
ccache --version
python3 --version
qmake --version
iverilog -V

echo "OpenFPGA macOS dependencies installed."
