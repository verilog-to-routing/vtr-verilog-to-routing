#!/bin/bash

find vpr libs/libarchfpga ODIN_II vtr_flow/arch -name '*.xml' | xargs -n 1 python2 ./vtr_flow/scripts/upgrade_arch.py
