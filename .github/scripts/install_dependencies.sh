#!/usr/bin/env bash

sudo apt update

# Required packages specifically for the CI and not VTR in general.
sudo apt install -y \
  autoconf \
  automake \
  bash \
  binutils \
  binutils-gold \
  capnproto \
  exuberant-ctags \
  curl \
  doxygen \
  fontconfig \
  gdb \
  gperf \
  libcairo2-dev \
  libcapnp-dev \
  libevent-dev \
  libfontconfig1-dev \
  liblist-moreutils-perl \
  libncurses5-dev \
  libxft-dev \
  libxml2-utils \
  libxml++2.6-dev \
  tcllib \
  tcl8.6-dev \
  perl \
  texinfo \
  time \
  valgrind \
  zip \
  qtbase5-dev \
  uuid-dev \
  default-jdk \
  g++-11 \
  gcc-11 \
  g++-12 \
  gcc-12 \
  g++-13 \
  gcc-13 \
  g++-14 \
  gcc-14 \
  g++-15 \
  gcc-15 \
  clang-15 \
  clang-16 \
  clang-17 \
  clang-18

# Standard packages install script.
./install_apt_packages.sh

pip install -r requirements.txt

git clone https://github.com/capnproto/capnproto-java.git $GITHUB_WORKSPACE/env/capnproto-java
pushd $GITHUB_WORKSPACE/env/capnproto-java
make
sudo make install
popd
