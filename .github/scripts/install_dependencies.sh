#!/usr/bin/env bash

sudo apt update

sudo apt install -y \
  autoconf \
  automake \
  bash \
  bison \
  binutils \
  binutils-gold \
  build-essential \
  capnproto \
  cmake \
  ctags \
  curl \
  doxygen \
  flex \
  fontconfig \
  gdb \
  git \
  gperf \
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
  tcl-dev \
  libffi-dev \
  perl \
  texinfo \
  time \
  valgrind \
  zip \
  qt5-default \
  clang-format-7 \
  g++-7 \
  gcc-7 \
  g++-8 \
  gcc-8 \
  g++-9 \
  gcc-9 \
  g++-10 \
  gcc-10 \
  g++-11 \
  gcc-11 \
  clang-6.0 \
  clang-7 \
  clang-10
#  libtbb-dev

pip install -r requirements.txt

git clone https://github.com/capnproto/capnproto-java.git $GITHUB_WORKSPACE/env/capnproto-java
pushd $GITHUB_WORKSPACE/env/capnproto-java
make
sudo make install
popd
