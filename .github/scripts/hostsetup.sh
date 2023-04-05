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

apt install -y \
  autoconf \
  automake \
  bash \
  bison \
  binutils \
  binutils-gold \
  build-essential \
  capnproto \
  clang \
  exuberant-ctags \
  curl \
  doxygen \
  flex \
  fontconfig \
  gdb \
  git \
  gperf \
  gzip \
  libcairo2-dev \
  libcapnp-dev \
  libgtk-3-dev \
  libevent-dev \
  libfontconfig1-dev \
  liblist-moreutils-perl \
  libncurses5-dev \
  libx11-dev \
  libxft-dev \
  libxml++2.6-dev \
  libreadline-dev \
  python2 \
  python3 \
  python3-dev \
  python3-pip \
  python3-virtualenv \
  python3-yaml \
  tcllib \
  tcl8.6-dev \
  libffi-dev \
  perl \
  texinfo \
  time \
  valgrind \
  zip \
  qtbase5-dev \
  uuid-dev \
  default-jdk \
  g++-9 \
  gcc-9 \
  wget \
  libtbb-dev

# installing the latest version of cmake
apt install -y apt-transport-https ca-certificates gnupg
wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc |apt-key add -

apt-add-repository 'deb https://apt.kitware.com/ubuntu/ jammy main'
apt update
apt install -y cmake


export PATH="$PATH:/home/kbuilder/.local/bin"

export CC=gcc-9
export CXX=g++-9

python3 -m pip install -U pip
python3 -m pip install -r requirements.txt

echo "----------------------------------------"
