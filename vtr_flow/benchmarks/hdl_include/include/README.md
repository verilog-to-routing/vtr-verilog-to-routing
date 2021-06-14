# include files

This folder contains _include_ files for running the modified version of ch_intrisinc.

_include_ files can be either a Verilog file or a Verilog header file. The main point worth mentioning is that the union of include files and the circuit design includes the top module should not result in any conflict like having multiple top modules or declaring different variables with the same name.

To create a task config file, the syntax for _include_ files is pretty much like the circuits or architectures. 
In the beginning, _includes_dir_, a path to _include_ files should be specified. In the following, specifiers include_add_list adds specific _include_ files, using a relative path that will be pre-pended by the _includes_dir_ path.
If config.txt file lists multiple benchmark circuits and multiple include files, all include files will be considered (unioned into) each circuit design. In other words, _include_ files are shared among given benchmark circuits.

___________________________hdl_include task config file____________________________
##############################################
\# Configuration file for running experiments
##############################################

\# Path to directory of circuits to use
circuits_dir=benchmarks/hdl_include

\# Path to directory of includes circuits to use
includes_dir=benchmarks/hdl_include/include

\# Path to directory of architectures to use
archs_dir=arch/no_timing/memory_sweep

\# Add circuits to list to sweep
circuit_list_add=ch_intrinsics_top.v

\# Add circuits to includes list to sweep
include_list_add=generic_definitions1.vh
include_list_add=generic_definitions2.vh
include_list_add=memory_controller.vh

\# Add architectures to list to sweep
arch_list_add=k4_N10_memSize16384_memData64.xml

\# Parse info and how to parse
parse_file=vpr_no_timing.txt

\# How to parse QoR info
qor_parse_file=qor_no_timing.txt

\# Script parameters
script_params_common=-track_memory_usage --timing_analysis off
___________________________________________________________________________________
