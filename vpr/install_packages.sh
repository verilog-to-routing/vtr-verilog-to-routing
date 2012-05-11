#!/bin/bash

set PACKAGES=dpkg-query -W --showformat='${Status}\n' libx11-dev | grep "install ok installed"

if [ -n $PACKAGES ]; then sudo apt-get install libx11-dev; fi

