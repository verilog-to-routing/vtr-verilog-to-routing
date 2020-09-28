#!/bin/bash

make get_titan_benchmarks
make get_ispd_benchmarks
make get_symbiflow_benchmarks

dev/upgrade_vtr_archs.sh
