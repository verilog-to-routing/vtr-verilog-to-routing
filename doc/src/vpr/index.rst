.. _vpr:

VPR
===

VPR (Versatile Place and Route) is an open source academic CAD tool designed for the exploration of new FPGA architectures and CAD algorithms, at the packing, placement and routing phases of the CAD flow :cite:`betz_vpr,luu_vpr_5`.
Since its public introduction, VPR has been used extensively in many academic projects partly because it is robust, well documented, easy-to-use, and can flexibly target a range of architectures.

VPR takes, as input, a description of an FPGA architecture along with a technology-mapped user circuit.
It then performs packing, placement, and routing to map the circuit onto the FPGA.
The output of VPR includes the FPGA configuration needed to implement the circuit and statistics about the final mapped design (eg. critical path delay, area, etc).

**Motivation**

The study of FPGA CAD and architecture can be a challenging process partly because of the difficulty in conducting high quality experiments.
A quality CAD/architecture experiment requires realistic benchmarks, accurate architectural models, and robust CAD tools that can appropriately map the benchmark to the particular architecture in question.
This is a lot of work.
Fortunately, this work can be made easier if open source tools are available as a starting point.

The purpose of VPR is to make the packing, placement, and routing stages of the FPGA CAD flow robust and flexible so that it is easier for researchers to investigate future FPGAs.


.. toctree::
   :maxdepth: 2

   command_line_usage

   graphics

   timing_constraints
   sdc_commands

   file_formats
   debug_aids
