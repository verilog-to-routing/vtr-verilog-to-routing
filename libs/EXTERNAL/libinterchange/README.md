# FPGA interchange schema definitions

This repository contains the capnp schema for the FPGA interchange.

## Repositories that implement tools around the FPGA interchange

[RapidWright](https://github.com/Xilinx/RapidWright/):
 - Provides support for 7-series, UltraScale and UltraScale+ parts
 - Generate device database for parts
 - Can convert DCPs to logical and physical FPGA interchange files.
 - Can convert logical and physical FPGA interchange files to DCPs

[python-fpga-interchange](https://github.com/SymbiFlow/python-fpga-interchange):
 - Implements textual format conversions for FPGA interchange files.
 - Provides (partial) generator for nextpnr to generate a nextpnr architecture
   for a FPGA interchange device database.
