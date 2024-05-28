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
  exuberant-ctags \
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
  libxml2-utils \
  libxml++2.6-dev \
  libreadline-dev \
  tcllib \
  tcl8.6-dev \
  libffi-dev \
  perl \
  pkg-config \
  texinfo \
  time \
  valgrind \
  zip \
  qtbase5-dev \
  uuid-dev \
  default-jdk \
  g++-9 \
  gcc-9 \
  g++-10 \
  gcc-10 \
  g++-11 \
  gcc-11 \
  g++-12 \
  gcc-12 \
  g++-13 \
  gcc-13 \
  clang-14 \
  clang-18 \
  clang-format-14 \
  libtbb-dev

pip install -r requirements.txt

git clone https://github.com/capnproto/capnproto-java.git $GITHUB_WORKSPACE/env/capnproto-java
pushd $GITHUB_WORKSPACE/env/capnproto-java
make
sudo make install
popd
