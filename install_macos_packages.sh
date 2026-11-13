#!/usr/bin/env bash

set -euo pipefail

echo "Installing VTR macOS dependencies..."

brew update

brew install \
    autoconf \
    automake \
    libtool \
    bison \
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
    zlib \
    libffi \
    readline \
    libxml2 \
    ncurses \
    python \
    tcl-tk \
    swig \
    graphviz \
    googletest \
    gawk \
    qt

echo "VTR macOS dependencies installed."
