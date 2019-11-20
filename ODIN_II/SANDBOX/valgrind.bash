#!/bin/bash

#valgrind --tool=memcheck --db-attach=yes --num-callers=7 ../odin_II.exe -V test.v
valgrind --tool=memcheck --db-attach=yes --num-callers=7 ../odin_II.exe -V /homes/pjamieso/PROGRAMS/ODIN_II/REGRESSION_TESTS/BENCHMARKS/MICROBENCHMARKS//bm_DL_16_1_mux.v
#valgrind --tool=memcheck --db-attach=yes --num-callers=7 ../odin_II.exe -V /homes/pjamieso/PROGRAMS/ODIN_II/REGRESSION_TESTS/BENCHMARKS/FULL_BENCHMARKS//des_area.v
