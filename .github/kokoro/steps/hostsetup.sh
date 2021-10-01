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
        cmake \
        colordiff \
        coreutils \
        curl \
        flex \
        git \
        graphviz \
        inkscape \
        jq \
        libgtk-3-dev \
        libx11-dev \
        make \
        ninja-build \
        nodejs \
        psmisc \
        python \
        python3 \
        python3-dev \
        python3-virtualenv \
        python3-yaml \
        qt5-default \
        virtualenv \
        clang \
	    libreadline-dev \
        gawk \
        tcl-dev \
        libffi-dev \
        xdot \
        pkg-config \
        libboost-system-dev \
	    libboost-python-dev \
        libboost-filesystem-dev \
        zlib1g-dev \
        #Don't include libtbb-dev since it may increase memory usage
        #libtbb-dev \

export PATH="$PATH:/home/kbuilder/.local/bin"

pyenv install -f 3.6.3
pyenv global 3.6.3
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
python3 get-pip.py   
rm get-pip.py
python3 -m pip install -r requirements.txt

if [ -z "${BUILD_TOOL}" ]; then
    export BUILD_TOOL=make
fi

echo "----------------------------------------"
