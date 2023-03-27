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
  libxml++2.6-dev \
  libreadline-dev \
  tcl-dev \
  libffi-dev \
  perl \
  texinfo \
  time \
  valgrind \
  zip \
  qtbase5-dev \
  uuid-dev \
  default-jdk \
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
  clang-10 \
  libtbb-dev

pip install -r requirements.txt

# installing the latest version of cmake
apt install -y apt-transport-https ca-certificates gnupg
wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc |apt-key add -

apt-add-repository 'deb https://apt.kitware.com/ubuntu/ jammy main'
apt update
apt install -y cmake

git clone https://github.com/capnproto/capnproto-java.git $GITHUB_WORKSPACE/env/capnproto-java
pushd $GITHUB_WORKSPACE/env/capnproto-java
make
sudo make install
popd
