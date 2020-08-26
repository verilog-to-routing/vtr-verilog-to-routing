#!/usr/bin/env bash

set -x
set -e

# Switch to python 3.6.3
pyenv install -f 3.6.3
pyenv global 3.6.3

# Install python modules
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
python3 get-pip.py
rm get-pip.py
python3 -m pip install \
  pylint \
  black
