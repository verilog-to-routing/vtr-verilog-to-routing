.. _vtr:

VTR
===

.. image:: https://www.verilogtorouting.org/img/neuron_placement_macros.gif
    :width: 24%
    :alt: Placement 

.. image:: https://www.verilogtorouting.org/img/neuron_cpd.gif
    :width: 24% 
    :alt: Critical Path 

.. image:: https://www.verilogtorouting.org/img/neuron_nets.gif
    :width: 24%
    :alt: Wiring

.. image:: https://www.verilogtorouting.org/img/neuron_routing_util.gif
    :width: 24% 
    :alt: Routing Usage

The Verilog-to-Routing (VTR) project :cite:`luu_vtr,luu_vtr_7` is a world-wide collaborative effort to provide a open-source framework for conducting FPGA architecture and CAD research and development.
The VTR design flow takes as input a Verilog description of a digital circuit, and a description of the target FPGA architecture.

It then perfoms:

 * Elaboration & Synthesis (:ref:`odin_II`)
 * Logic Optimization & Technology Mapping (:ref:`abc`)
 * Packing, Placement, Routing & Timing Analysis (:ref:`vpr`)

Generating FPGA speed and area results.

VTR also includes a set of benchmark designs known to work with the design flow.


.. toctree::
   :maxdepth: 2

   cad_flow
   get_vtr
   ../BUILDING
   optional_build_info
   running_vtr
   benchmarks
   power_estimation/index.rst
   tasks
   run_vtr_flow
   run_vtr_task
   parse_vtr_flow
   parse_vtr_task
   parse_config
   pass_requirements
   python_libs/vtr



