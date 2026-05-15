.. _arch_modeling_concepts:

Architecture Modeling Concepts
-------------------------------

This guide describes fundamental concepts that govern how a VTR architecture description is interpreted by the CAD tools.
Understanding these concepts before writing or modifying architecture files prevents structural mistakes that are difficult to fix after the fact.

.. _arch_two_layers:

Physical and Logical Layers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A VTR architecture description has two distinct layers that serve different purposes in the CAD flow.

The **physical layer** consists of ``<tiles>``, ``<layout>``, ``<device>``, ``<switchlist>``, and ``<segmentlist>``.
It describes the FPGA grid: which block types exist, how large they are, where they appear on the chip, and how the global routing fabric is structured.
Physical layer sections control pin locations (which edge of a tile each port appears on) and connection block density.

The **logical layer** consists of ``<models>`` and ``<complexblocklist>``.
It describes the internal logic hierarchy of each block type using ``<pb_type>`` trees.
It specifies which primitives exist (LUTs, flip-flops, hard blocks) and how they interconnect internally.
The logical layer carries no physical position or layout information.

.. figure:: physical_logical_layers.*

    .. figure comment: A diagram showing the two-layer structure. Left panel labeled "Physical Layer" shows tiles on a grid with pins on edges, connected by routing channels. Right panel labeled "Logical Layer" shows a pb_type tree (CLB -> BLE -> LUT/FF) with interconnect. An arrow labeled "equivalent_sites" connects a tile in the left panel to the top-level pb_type in the right panel, illustrating the bridge between layers.

The ``<tiles>`` section bridges the two layers.
Each ``<sub_tile>`` within a tile declares which ``<pb_type>`` (complex block) can be placed inside it via ``<equivalent_sites>``.
A tile can contain multiple sub-tile types to describe a heterogeneous tile — one that hosts different kinds of complex blocks at the same grid location.

.. seealso::
    :ref:`arch_tiles` for the complete ``<tiles>`` syntax.
    :ref:`arch_complex_blocks` for the complete ``<complexblocklist>`` syntax.

.. _arch_cad_flow_order:

CAD Flow Order and Its Consequences for Architecture Modeling
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

VTR's CAD flow runs in this order: **Pack → Place → Route**.

.. figure:: cad_flow_arch_impact.*

    .. figure comment: A horizontal flow diagram showing three boxes: "Packer" -> "Placer" -> "Router". Below the Packer box: "Groups atoms into clusters using pb_type hierarchy". Below the Placer box: "Assigns clusters to tile locations on the grid". Below the Router box: "Connects clusters through the routing fabric". A dashed boundary around the Packer box labeled "No placement information available here" emphasizes the key constraint.

The packer runs first and has no knowledge of where on the device clusters will eventually be placed.
It creates cluster instances of each logical block type (each top-level ``<pb_type>``), but the placer assigns those clusters to physical tile locations later.

This has a critical implication for architecture modeling: **complex blocks must be physically agnostic**.
A complex block of type ``X`` can be placed in any tile that lists ``X`` as an equivalent site, anywhere on the device.
The packer cannot reason about which tile location a cluster will land in.

.. _arch_positional_invariance:

Positional Invariance
~~~~~~~~~~~~~~~~~~~~~

When a tile type is defined and placed on the grid, any complex block compatible with that tile can be placed in *any* instance of that tile, without restriction.
This is **positional invariance**: the packer treats all tiles of a given type as interchangeable.

.. figure:: positional_invariance.*

    .. figure comment: A device grid showing four IO tiles on the left edge and four on the right edge. All are labeled "io" (same type). A CLB cluster with a question mark is shown attempting to choose between left-side and right-side tiles, with a caption "Packer cannot distinguish — both are type 'io'". An X mark shows the incorrect approach of two separate complex block types (io_left, io_right). A checkmark shows the correct approach of one complex block type (io) with the physical difference handled in the tile's pinlocations.

If different physical behavior is needed at different grid locations (for example, IO tiles on different chip edges), that variation must be handled in the **tile** layer, not in the complex block.
The complex block stays uniform.

A common mistake is creating separate ``io_left`` and ``io_right`` pb_types for side-specific IO.
The packer cannot know at pack time which side a cluster will land on, so it cannot choose between them correctly.
The right approach is one ``io`` pb_type whose internal logic is side-agnostic, with side-specific pin placement handled by ``<pinlocations>`` in the tile.

.. _arch_routing_domains:

Intra-Cluster vs. Global Routing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

VTR has two distinct routing domains that are resolved at different stages of the CAD flow.

**Intra-cluster routing** is defined by the ``<interconnect>`` tags inside ``<pb_type>`` nodes.
It describes the connections between primitives within a single cluster.
This routing is resolved by the packer: when the packer assembles atoms into a cluster, it determines which internal connections are used and verifies they are legal according to the ``<pb_type>`` hierarchy.

**Global routing** is defined by the channels, switch blocks, and connection blocks described in ``<segmentlist>`` and ``<switchlist>``.
It describes the connections between clusters across the device.
This routing is resolved by the router, after placement is complete.

.. figure:: routing_domains.*

    .. figure comment: Two side-by-side panels. Left panel labeled "Intra-Cluster Routing (resolved by packer)" shows a CLB outline containing a LUT connected to a FF via an internal direct wire — the interconnect is fully inside the cluster boundary. Right panel labeled "Global Routing (resolved by router)" shows two CLB boxes connected by a wire that travels through a routing channel with a switch block. A dashed boundary on each panel emphasizes the separation.

These two routing domains are entirely separate.
Do not attempt to model intra-cluster connectivity using global routing constructs, or vice versa.

.. _arch_carry_chains:

Carry Chains and Placement Macros
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Some structures — carry chains being the canonical example — span cluster boundaries.
Their correct modeling requires three coordinated pieces across separate sections of the architecture file.

.. figure:: carry_chain_overview.*

    .. figure comment: A vertical stack of three CLB tiles. Inside each CLB, an adder primitive is shown with a "cin" input at the bottom and "cout" output at the top. A thick arrow labeled "direct connection (no routing fabric)" connects each CLB's cout to the cin of the CLB below it, crossing the tile boundary. Alongside the figure, three callouts indicate: (1) pack_pattern tags on carry edges inside complexblocklist, (2) a directlist entry specifying y_offset=-1, (3) fc_val="0" on cin/cout in the tile's fc tag. Below, a placement macro bracket groups the three CLBs together with the label "placed as a unit by the placer".

**Pack patterns (complexblocklist)**

Inside the ``<complexblocklist>``, each ``<direct>`` edge along the carry path is annotated with a ``<pack_pattern>`` tag using a consistent pattern name.
This tells VPR's prepacker to group matching netlist atoms into *molecules* — units the packer must keep together in the same cluster.

When a pack pattern chain has exactly one connection touching a block-level input pin (e.g., ``cin``) and exactly one touching a block-level output pin (e.g., ``cout``), VPR treats it as a cross-cluster chain.
The prepacker breaks long adder chains into cluster-sized molecules connected in order by the carry links.

.. code-block:: xml

    <!-- Inside the arithmetic pb_type, carry path annotations -->
    <direct name="carry_in" input="arithmetic.cin" output="adder.cin">
      <pack_pattern name="chain" in_port="arithmetic.cin" out_port="adder.cin"/>
    </direct>
    <direct name="carry_out" input="adder.cout" output="arithmetic.cout">
      <pack_pattern name="chain" in_port="adder.cout" out_port="arithmetic.cout"/>
    </direct>

**Directlist entry**

A ``<direct>`` entry in ``<directlist>`` defines the physical inter-tile wire: it names the tile ports and the ``x_offset``/``y_offset`` to the neighboring tile.
This is the only path carry signals may travel between clusters.

.. code-block:: xml

    <directlist>
      <direct name="adder_carry" from_pin="clb.cout" to_pin="clb.cin"
              x_offset="0" y_offset="-1" z_offset="0"/>
    </directlist>

**fc_val="0" on carry ports**

The carry ports must have ``<fc_override>`` entries in the tile's ``<fc>`` tag with ``fc_val="0"``.
This removes them from the connection block so the router cannot route carry signals through general interconnect — only the directlist wire is available.

.. code-block:: xml

    <sub_tile name="clb">
      <input name="cin" num_pins="1"/>
      <output name="cout" num_pins="1"/>
      <!-- other ports ... -->
      <fc in_type="frac" in_val="0.15" out_type="frac" out_val="0.10">
        <fc_override port_name="cin" fc_type="frac" fc_val="0"/>
        <fc_override port_name="cout" fc_type="frac" fc_val="0"/>
      </fc>
    </sub_tile>

**Placement macros**

At placement time, the placer detects clusters linked by a directlist connection and forms a *placement macro*: a group of clusters that must land at specific relative tile offsets matching the directlist ``x_offset``/``y_offset``.
The whole macro is placed as a unit, ensuring the physical direct connection is realizable.

.. note::
    All three pieces — pack patterns, directlist entry, and ``fc_val="0"`` — must be present.
    A directlist entry alone does not constrain placement.
    A pack pattern chain alone has no physical wire to use.
    Missing ``fc_val="0"`` allows the router to reach carry ports through general routing, bypassing the direct connection entirely.

The flagship architecture ``vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml`` implements this pattern and is the best reference for carry chain modeling.

.. seealso::
    :ref:`arch_complex_blocks` for ``<pack_pattern>`` syntax.
    :ref:`direct_interconnect` for ``<directlist>`` syntax.
    :ref:`arch_tiles` for ``<fc_override>`` syntax.
