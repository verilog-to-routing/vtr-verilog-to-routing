#!/bin/bash
#
# travis_install_catch2.sh
#
# Travis CI build/test script for the sockpp library.
# This installs Catch2 into the VM.
#

set -ex

# Install Catch2 from sources
wget https://github.com/catchorg/Catch2/archive/v2.5.0.tar.gz
tar -xf v2.5.0.tar.gz
cd Catch2-2.5.0/
cmake -Bbuild -H. -DBUILD_TESTING=OFF

# CMake bin is installed in a strange place where
# sudo can not find by default.
sudo env "PATH=$PATH" cmake --build build/ --target install

