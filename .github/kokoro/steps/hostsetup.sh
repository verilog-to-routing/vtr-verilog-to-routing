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
echo "Host adding PPAs"
echo "----------------------------------------"
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
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
        libtbb-dev \
        virtualenv \

if [ -z "${BUILD_TOOL}" ]; then
    export BUILD_TOOL=make
fi

echo "----------------------------------------"
