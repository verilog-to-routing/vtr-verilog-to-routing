.. _vtr_cad_flow:

VTR CAD Flow
------------

:numref:`fig_vtr_cad_flow` illustrates the CAD flow typically used in VTR.

.. _fig_vtr_cad_flow:

.. figure:: vtr_cad_flow.png

    Typicall VTR CAD Flow

First, :ref:`odin_II` converts a Verilog Hardware Destription Language (HDL) design into a flattened netlist consisting of logic gates and blackboxes that represent heterogeneous blocks :cite:`jamieson_odin_II`.

Next, the :ref:`abc`  synthesis package is used to perform technology-independent logic optimization of each circuit, and then each circuit is technology-mapped into LUTs and flip flops :cite:`abc_cite,pistorius_benchmarking_method_fpga_synthesis,cho_priority_cuts`.
The output of ABC is a .blif format netlist of LUTs, flip flops, and blackboxes.

:ref:`vpr` then packs this netlist into more coarse-grained logic blocks, places the circuit, and routes it :cite:`betz_arch_cad,betz_phd,betz_directional_bias_routing_arch,betz_biased_global_routing_tech_report,betz_vpr,marquardt_timing_driven_placement,betz_automatic_generation_of_fpga_routing`.
Generating :ref:`output files <vpr_file_formats>` for each stage.
VPR will produce various statistics such as the minimum number of tracks per channel required to successfully route, the total wirelength, circuit speed, area and power.

Many variations on this CAD flow are possible.
It is possible to use other high-level synthesis tools to generate the blif files that are passed into ABC.
Also, one can use different logic optimizers and technology mappers than ABC; just put the output netlist from your technology-mapper into .blif format and feed it into VPR.

Alternatively, if the logic block you are interested in is not supported by VPR, your CAD flow can bypass VPR's packer altogether by outputting a netlist of logic blocks in .net format.
VPR can place and route netlists of any type of logic block -- you simply have to create the netlist and describe the logic block in the FPGA architecture description file.

Finally, if you want only to route a placement produced by another CAD tool you can create a placement file in VPR format, and have VPR route this pre-existing placement.

VPR also supports timing analysis and power estimation.

