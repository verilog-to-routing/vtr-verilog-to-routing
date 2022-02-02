#!/bin/bash

set -e

echo
echo "========================================"
echo "Removing older packages"
echo "----------------------------------------"
sudo apt-get remove -y cmake
echo "----------------------------------------"

echo
echo "========================================"
echo "Update the CA certificates"
echo "----------------------------------------"
sudo apt-get install -y ca-certificates
echo "----------------------------------------"
sudo update-ca-certificates
echo "----------------------------------------"

echo
echo "========================================"
echo "Remove the expired letsencrypt.org cert "
echo "----------------------------------------"
sudo rm /usr/share/ca-certificates/mozilla/DST_Root_CA_X3.crt
echo "----------------------------------------"
sudo update-ca-certificates
echo "----------------------------------------"
wget https://helloworld.letsencrypt.org/ || true
echo "----------------------------------------"


echo
echo "========================================"
echo "Host adding PPAs"
echo "----------------------------------------"
wget --no-check-certificate -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ xenial main'
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
echo "----------------------------------------"

echo
echo "========================================"
echo "Host updating packages"
echo "----------------------------------------"
sudo apt-get update
echo "----------------------------------------"

echo
echo "========================================"
echo "Host install packages"
echo "----------------------------------------"

sudo apt-get install -y \
        bash \
        bison \
        build-essential \
        ca-certificates \
        clang \
        cmake \
        colordiff \
        coreutils \
        curl \
        flex \
        gawk \
        gcc-9 \
        g++-9 \
        git \
        graphviz \
        inkscape \
        jq \
        libboost-filesystem-dev \
        libboost-python-dev \
        libboost-system-dev \
        libffi-dev \
        libgtk-3-dev \
        libreadline-dev \
        libx11-dev \
        make \
        ninja-build \
        nodejs \
        pkg-config \
        psmisc \
        python \
        python3 \
        python3-dev \
        python3-virtualenv \
        python3-yaml \
        qt5-default \
        tcl-dev \
        virtualenv \
        xdot \
        zlib1g-dev
        #Don't include libtbb-dev since it may increase memory usage
        #libtbb-dev \

export PATH="$PATH:/home/kbuilder/.local/bin"

export CC=gcc-9
export CXX=g++-9

pyenv install -f 3.7.0
pyenv global 3.7.0
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
python3 get-pip.py
rm get-pip.py
python3 -m pip install -r requirements.txt

if [ -z "${BUILD_TOOL}" ]; then
    export BUILD_TOOL=make
fi

echo "----------------------------------------"
