#!/bin/bash

set PACKAGES=dpkg-query -W --showformat='${Status}\n' exuberant-ctags | grep "install ok installed"
if [ -n $PACKAGES ]; then sudo apt-get install exuberant-ctags; fi

set PACKAGES=dpkg-query -W --showformat='${Status}\n' bison | grep "install ok installed"
if [ -n $PACKAGES ]; then sudo apt-get install bison; fi

set PACKAGES=dpkg-query -W --showformat='${Status}\n' flex | grep "install ok installed"
if [ -n $PACKAGES ]; then sudo apt-get install flex; fi

set PACKAGES=dpkg-query -W --showformat='${Status}\n' g++ | grep "install ok installed"
if [ -n $PACKAGES ]; then sudo apt-get install g++; fi

set PACKAGES=dpkg-query -W --showformat='${Status}\n' libx11-dev | grep "install ok installed"
if [ -n $PACKAGES ]; then sudo apt-get install libx11-dev; fi





