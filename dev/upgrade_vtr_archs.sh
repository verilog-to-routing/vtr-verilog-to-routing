#!/usr/bin/env bash

find vpr libs/libarchfpga ODIN_II vtr_flow/arch utils/fasm/test -name '*.xml' | xargs -n 1 ./vtr_flow/scripts/upgrade_arch.py
