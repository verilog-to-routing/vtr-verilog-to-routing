#! /bin/bash
#Generate Configuration files for the benchmarks in .../ODIN_II/REGRESSSION_TESTS/BENCHMARKS

python ./odin_config_maker.py -a ../../../libvpr/arch/sample_arch.xml -i ../BENCHMARKS/MICROBENCHMARKS/ -o ../BASIC_TESTS/ --both --size 3 --split_width --debug_path ../BENCHMARKS/MICROBENCHMARKS/ ../BENCHMARKS/MICROBENCHMARKS/
python ./odin_config_maker.py -a ../../../libvpr/arch/sample_arch.xml -i ../BENCHMARKS/FULL_BENCHMARKS/ -o ../BASIC_TESTS/ --both --size 3 --split_width --debug_path ../BENCHMARKS/FULL_BENCHMARKS/ ../BENCHMARKS/FULL_BENCHMARKS/
python ./odin_config_maker.py -a ../../../libvpr/arch/sample_arch.xml -i ../BENCHMARKS/ARCH_BENCHMARKS/ -o ../BASIC_TESTS/ --both --size 3 --split_width --debug_path ../BENCHMARKS/ARCH_BENCHMARKS/ ../BENCHMARKS/ARCH_BENCHMARKS/