#!/bin/bash

make get_titan_benchmarks
make get_ispd_benchmarks

dev/upgrade_vtr_archs.sh

# Symbiflow archs do not require update
make get_symbiflow_benchmarks
