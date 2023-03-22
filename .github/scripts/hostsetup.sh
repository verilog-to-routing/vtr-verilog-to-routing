#!/bin/bash

set -e
echo "========================================"
echo "Host updating packages"
echo "----------------------------------------"
apt update
echo "----------------------------------------"


echo "========================================"
echo "Add repositories"
echo "----------------------------------------"
apt install -y software-properties-common
add-apt-repository ppa:ubuntu-toolchain-r/test
echo "----------------------------------------"

echo
echo "========================================"
echo "Host install packages"
echo "----------------------------------------"

apt-get update
apt-get install -y \
    autoconf \
    automake \
    bison \
    ccache \
    cmake \
    exuberant-ctags \
    curl \
    doxygen \
    flex \
    fontconfig \
    gdb \
    git \
    gperf \
    iverilog \
    libc6-dev \
    libcairo2-dev \
    libevent-dev \
    libffi-dev \
    libfontconfig1-dev \
    liblist-moreutils-perl \
    libncurses5-dev \
    libreadline-dev \
    libreadline8 \
    libx11-dev \
    libxft-dev \
    libxml++2.6-dev \
    make \
    perl \
    pkg-config \
    python3 \
    python3-setuptools \
    python3-lxml \
    python3-pip \
    qtbase5-dev \
    tcllib \
    tcl8.6-dev \
    texinfo \
    time \
    valgrind \
    wget \
    zip \
    swig \
    expect \
    g++-9 \
    gcc-9 \
    g++-10 \
    gcc-10 \
    g++-11 \
    gcc-11 \
    clang \
    clang-12 \
    clang-format-12 \
    libxml2-utils

# installing the latest version of cmake
apt install -y apt-transport-https ca-certificates gnupg
wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc |apt-key add -

apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
apt update
apt install -y cmake


export PATH="$PATH:/home/kbuilder/.local/bin"

export CC=gcc-9
export CXX=g++-9

python3 -m pip install -U pip
python3 -m pip install -r requirements.txt

echo "----------------------------------------"
