#!/bin/bash

valgrind --show-reachable=yes --leak-check=full --tool=memcheck --db-attach=yes --num-callers=7 ../odin_II.exe -V /homes/pjamieso/PROGRAMS/ODIN_II/REGRESSION_TESTS/BENCHMARKS/MICROBENCHMARKS//bm_sfifo_rtl.v
