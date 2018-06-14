.. _vtr_cad_flow:

VTR CAD Flow
------------

:numref:`fig_vtr_cad_flow` illustrates the CAD flow typically used in VTR.

.. _fig_vtr_cad_flow:

.. graphviz::
    :caption: Typical VTR CAD Flow

    digraph {
        compound=true;
        splines=ortho;

        #Files
        node [shape=Mrecord, style=filled, fillcolor="/greys4/1:/greys4/2", gradientangle=270, fontname="arial", width=3];
        verilog_file[label="Behavioural Circuit\nDescription (Verilog HDL)", group=g1];
        odin_blif[label="BLIF Netlist\n(logic, flip-flops, blackboxes)", group=g1];
        opt_blif[label="Optimized BLIF Netlist\n(LUTs, flip-flops, blackboxes)", group=g1];
        arch[label="FPGA Architecture\nDescription (XML)"];
        pnr_files[label="Pack, Place, Route\nOutput Files", group=g1];
        report_files[label="Reports and Statistics"];
        existing_pnr_files[label="Existing Pack, Place, Route Files\n(from VPR or other tools)", style="dashed,filled"];

        #Tools
        node [shape=record, fillcolor="/blues4/2:/blues4/3"];
        odin[label="ODIN\n(Front-end Synthesis)", group=g1];
        abc[label="ABC\n(Logic Optimization,\nTechnology Map to LUTs)", group=g1];

        subgraph cluster_vpr {
            style="filled,solid";
            color="/blues4/2:/blues4/3";
            gradientangle=270;
            node [style=filled,color=white];

            vpr[label="VPR", group=g1, shape=plaintext, style=""];
            vpr_pack[label="Packing", group=g1];
            vpr_place[label="Placement", group=g1];
            vpr_route[label="Routing", group=g1];
            vpr_analysis[label="Analysis", group=g1];

            vpr -> vpr_pack[style=invis];
            vpr_pack -> vpr_place -> vpr_route -> vpr_analysis;
        }

        #Edges
        arch -> odin:w;
        verilog_file -> odin -> odin_blif;
        odin_blif -> abc -> opt_blif;

        arch -> vpr:w [lhead=cluster_vpr];
        opt_blif -> vpr [lhead=cluster_vpr];

        vpr_analysis -> pnr_files [ltail=cluster_vpr];
        vpr_analysis -> report_files [ltail=cluster_vpr];

        existing_pnr_files -> vpr [style=dashed, lhead=cluster_vpr];
    }

First, :ref:`odin_II` converts a Verilog Hardware Destription Language (HDL) design into a flattened netlist consisting of logic gates and blackboxes which represent heterogeneous blocks :cite:`jamieson_odin_II`.

Next, the :ref:`abc`  synthesis package is used to perform technology-independent logic optimization, and then technology-maps the circuit into LUTs and flip-flops :cite:`abc_cite,pistorius_benchmarking_method_fpga_synthesis,cho_priority_cuts`.
The output of ABC is a :ref:`.blif format <blif_format>` netlist of LUTs, flip flops, and blackboxes.

:ref:`vpr` then packs this netlist into more coarse-grained logic blocks, places and then routes the circuit :cite:`betz_arch_cad,betz_phd,betz_directional_bias_routing_arch,betz_biased_global_routing_tech_report,betz_vpr,betz_cluster_based_logic_blocks,marquardt_timing_driven_packing,marquardt_timing_driven_placement,betz_automatic_generation_of_fpga_routing`.
Generating :ref:`output files <vpr_file_formats>` for each stage.
VPR will analyze the resulting implementation, producing various statistics such as the minimum number of tracks per channel required to successfully route, the total wirelength, circuit speed, area and power.

CAD Flow Variations
~~~~~~~~~~~~~~~~~~~

Many variations on this CAD flow are possible.
It is possible to use other high-level synthesis tools to generate the blif files that are passed into ABC.
Also, one can use different logic optimizers and technology mappers than ABC; just put the output netlist from your technology-mapper into .blif format and feed it into VPR.

Alternatively, if the logic block you are interested in is not supported by VPR, your CAD flow can bypass VPR's packer by outputting a netlist of logic blocks in :ref:`.net format <vpr_net_file>`.
VPR can place and route netlists of any type of logic block -- you simply have to create the netlist and describe the logic block in the FPGA architecture description file.

If you want only to route a placement produced by another CAD tool you can create a :ref:`.place file <vpr_place_file>`, and have VPR route this pre-existing placement.

If you want only to analyze an implementation produced by another tool with VPR, you can create a :ref:`.route file <vpr_route_file>`, and have VPR analyze the implementation, to produce area/delay/power results.

Finally, if your routing architecture is not supported by VPR's architecture generator, you can create an :ref:`rr_graph.xml file <vpr_route_resource_file>`, which can be loaded directly into VPR.
