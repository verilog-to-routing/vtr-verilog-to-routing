.. _Yosys+Odin-II:

#############
Yosys+Odin-II
#############

Yosys+Odin-II is the combination of Yosys as the elaborator and the Odin-II partial mapper and logic optimizer to perform logic synthesis for a subset of the Verilog Hardware Description Language (HDL) into a BLIF netlist.
Odin-II lacks support for the Verilog-2005 standard and SystemVerilog, but it provides complex partial mapping for balancing soft logic and hard blocks such as embedded multipliers, adders, and memories in FPGA architectures.
Yosys, an open framework for RTL synthesis, provides extensive support for HDLs.
However, the Yosys flow forces the user to decide the discrete circuit implementation manually.

.. image:: ./dev_guide/YosysOdinFlow.png
    :width: 100%    
    :alt: The Odin-II, Yosys, and Yosys+Odin-II Synthesis Flows


The approach taken by the Yosys synthesizer is to map all discrete components into available hard blocks or to explode them in low-level logic when not available.
The Yosys+Odin-II improves device utilization and simplifying the flow by automating hard logic decisions with architecture awareness.
Yosys acts as the front-end HDL elaborator, while Odin-II is fed by Yosys generated BLIF files to perform the partial mapping.
As a result, hard/soft logic trade-off decisions and heterogeneous logic inference become available for Yosys coarse-grained BLIF files.

    
.. toctree::
   :glob:
   :maxdepth: 2
   :Caption: Quickstart

   quickstart


.. toctree::
   :glob:
   :maxdepth: 2
   :Caption: User Guide

   user_guide

.. toctree::
   :glob:
   :maxdepth: 2
   :Caption: Verilog Support

   ../yosys/verilog_support

.. toctree::
   :glob:
   :maxdepth: 2
   :Caption: Developer Guide

   dev_guide/index
