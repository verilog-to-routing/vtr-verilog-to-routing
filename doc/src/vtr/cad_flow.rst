.. _vtr_cad_flow:

VTR CAD Flow
============

.. _fig_vtr_cad_flow:

.. figure:: vtr_flow_fig.*

    VTR CAD flow (and variants)

In the standard VTR Flow (:numref:`fig_vtr_cad_flow`), :ref:`odin_II` converts a Verilog Hardware Destription Language (HDL) design into a flattened netlist consisting of logic gates, flip-flops, and blackboxes representing heterogeneous blocks (e.g. adders, multipliers, RAM slices) :cite:`jamieson_odin_II`.

Next, the :ref:`abc`  synthesis package is used to perform technology-independent logic optimization, and technology-maps the circuit into LUTs :cite:`abc_cite,pistorius_benchmarking_method_fpga_synthesis,cho_priority_cuts`.
The output of ABC is a :ref:`.blif format <blif_format>` netlist of LUTs, flip flops, and blackboxes.

:ref:`vpr` then packs this netlist into more coarse-grained logic blocks, places and then routes the circuit :cite:`betz_arch_cad,betz_phd,betz_directional_bias_routing_arch,betz_biased_global_routing_tech_report,betz_vpr,betz_cluster_based_logic_blocks,marquardt_timing_driven_packing,marquardt_timing_driven_placement,betz_automatic_generation_of_fpga_routing`.
Generating :ref:`output files <vpr_file_formats>` for each stage.
VPR will analyze the resulting implementation, producing various statistics such as the minimum number of tracks per channel required to successfully route, the total wirelength, circuit speed, area and power.
VPR can also produce a post-implementation netlist for simulation and formal verification.

CAD Flow Variations
-------------------

Titan CAD Flow
~~~~~~~~~~~~~~
The Titan CAD Flow :cite:`murray_titan,murray_timing_driven_titan` interfaces Intel's Quartus tool with VPR.
This allows designs requiring industrial strength language coverage and IP to be brought into VPR.

Other CAD Flow Variants
~~~~~~~~~~~~~~~~~~~~~~~
Many other CAD flow variations are possible.

For instance, it is possible to use other logic synthesis tools like Yosys :cite:`yosys_cite` to generate the design netlist.
One could also use logic optimizers and technology mappers other than ABC; just put the output netlist from your technology-mapper into .blif format and pass it into VPR.

It is also possible to use tools other than VPR to perform the different stages of the implementation.

For example, if the logic block you are interested in is not supported by VPR, your CAD flow can bypass VPR's packer by outputting a netlist of logic blocks in :ref:`.net format <vpr_net_file>`.
VPR can place and route netlists of any type of logic block -- you simply have to create the netlist and describe the logic block in the FPGA architecture description file.

Similarly, if you want only to route a placement produced by another CAD tool you can create a :ref:`.place file <vpr_place_file>`, and have VPR route this pre-existing placement.

If you only need to analyze an implementation produced by another tool, you can create a :ref:`.route file <vpr_route_file>`, and have VPR analyze the implementation, to produce area/delay/power results.

Finally, if your routing architecture is not supported by VPR's architecture generator, you can describe your routing architecture in an :ref:`rr_graph.xml file <vpr_route_resource_file>`, which can be loaded directly into VPR.

Bitstream Generation
--------------------
The technology mapped netlist and packing/placement/routing results produced by VPR contain the information needed to generate a device programming bitstreams.

VTR focuses on the core physical design optimization tools and evaluation capabilities for new architectures and does not directly support generating device programming bitstreams.
Bitstream generators can either ingest the implementation files directly or make use of VTR utilities to emit :ref:`FASM <genfasm>`.
