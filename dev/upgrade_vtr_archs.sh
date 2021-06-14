#!/usr/bin/env bash

find vpr libs/libarchfpga vtr_flow/arch utils/fasm/test -name '*.xml' | xargs -n 1 python3 ./vtr_flow/scripts/upgrade_arch.py
