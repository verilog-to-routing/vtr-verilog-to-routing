###################################################################
# Modified Version of Odin-II and VTR Benchmarks (Yosys Compatible)
###################################################################

This directory contains the modified version of Odin-II and VTR flow Verilog benchmarks.
Benchmarks are changed to be compatible with the Yosys elaborator.

Changes including:

    1. Syntax error, including the registered assignment for net types and continuous assignment to data types
    2. Reorder the inputs and outputs in modules declarations in order to match with headers of
       Odin-II simulation vector files
    3. Specifying the address and data width of hard block memories
    
NOTE: Currently the _include_ feature is not supported with the Yosys elaborator. As a result, users can only pass one Verilog file to Odin-II using Yosys as an elaborator. For instance, Koios benchmarks only run without _complex_dsp_include.v_ header file.
