Command-line Options
====================
.. program:: vpr

.. |des90_place| image:: https://www.verilogtorouting.org/img/des90_placement_macros.gif
    :width: 200px
    :alt: Placement

.. |des90_cpd| image:: https://www.verilogtorouting.org/img/des90_cpd.gif
    :width: 200px
    :alt: Critical Path

.. |des90_nets| image:: https://www.verilogtorouting.org/img/des90_nets.gif
    :width: 200px
    :alt: Wiring

.. |des90_routing| image:: https://www.verilogtorouting.org/img/des90_routing_util.gif
    :width: 200px
    :alt: Routing Usage

+---------------------------------------+---------------------------------------+---------------------------------------+---------------------------------------+
| |des90_place|                         + |des90_cpd|                           | |des90_nets|                          + |des90_routing|                       +
|                                       +                                       |                                       +                                       +
| Placement                             + Critical Path                         | Logical Connections                   + Routing Utilization                   +
+---------------------------------------+---------------------------------------+---------------------------------------+---------------------------------------+

Basic Usage
-----------

At a minimum VPR requires two command-line arguments::

    vpr architecture circuit

where:

.. option:: architecture

    is an :ref:`FPGA architecture description file <fpga_architecture_description>`

.. option:: circuit

    is the technology mapped netlist in :ref:`BLIF format <vpr_blif_file>` to be implemented

VPR will then pack, place, and route the circuit onto the specified architecture.

By default VPR will perform a binary search routing to find the minimum channel width required to route the circuit.

Detailed Command-line Options
-----------------------------
VPR has a lot of options. Running :option:`vpr --help` will display all the available options and their usage information.

.. option:: -h, --help

    Display help message then exit.

The options most people will be interested in are:

* :option:`--route_chan_width` (route at a fixed channel width), and
* :option:`--disp` (turn on/off graphics).

In general for the other options the defaults are fine, and only people looking at how different CAD algorithms perform will try many of them.
To understand what the more esoteric placer and router options actually do, see :cite:`betz_arch_cad` or download :cite:`betz_directional_bias_routing_arch,betz_biased_global_routing_tech_report,betz_vpr,marquardt_timing_driven_placement` from the author’s `web page <http://www.eecg.toronto.edu/~vaughn>`_.

In the following text, values in angle brackets e.g. ``<int>`` ``<float>`` ``<string>`` ``<file>``, should be replaced by the appropriate number, string, or file path.
Values in curly braces separated by vertical bars, e.g. ``{on | off}``, indicate all the permissible choices for an option.

.. _stage_options:

Stage Options
^^^^^^^^^^^^^
VPR runs all stages of (pack, place, route, and analysis) if none of :option:`--pack`, :option:`--place`, :option:`--route` or :option:`--analysis` are specified.

.. option:: --pack

    Run packing stage

    **Default:** ``off``

.. option:: --place

    Run placement stage

    **Default:** ``off``

.. option:: --analytical_place

    Run the analytical placement flow.
    This flows uses an integrated packing and placement algorithm which uses information from the primitive level to improve clustering and placement;
    as such, the :option:`--pack` and :option:`--place` options should not be set when this option is set.
    This flow requires that the device has a fixed size and some of the primitive blocks are fixed somewhere on the device grid.

    .. seealso:: See :ref:`analytical_placement_options` for the options for this flow.

    .. seealso:: See :ref:`Fixed FPGA Grid Layout <fixed_arch_grid_layout>` and :option:`--device` for how to fix the device size.

    .. seealso:: See :ref:`VPR Placement Constraints <placement_constraints>` for how to fix primitive blocks in a design to the device grid.

    .. warning::

        This analytical placement flow is experimental and under active development.

    **Default:** ``off``

.. option:: --route

    Run routing stage
    This also implies --analysis if routing was successful.

    **Default:** ``off``

.. option:: --analysis

    Run final analysis stage (e.g. timing, power).

    **Default:** ``off``

.. _graphics_options:

Graphics Options
^^^^^^^^^^^^^^^^

.. option:: --disp {on | off}

    Controls whether :ref:`VPR's interactive graphics <vpr_graphics>` are enabled.
    Graphics are very useful for inspecting and debugging the FPGA architecture and/or circuit implementation.

    **Default:** ``off``

.. option:: --auto <int>

    Can be 0, 1, or 2.
    This sets how often you must click Proceed to continue execution after viewing the graphics.
    The higher the number, the more infrequently the program will pause.

    **Default:** ``1``

.. option:: --save_graphics {on | off}

    If set to on, this option will save an image of the final placement and the final routing created by vpr to pdf files on disk, with no need for any user interaction. The files are named vpr_placement.pdf and vpr_routing.pdf.

    **Default:** ``off``

.. option:: --graphics_commands <string>

    A set of semi-colon seperated graphics commands.
    Graphics commands must be surrounded by quotation marks (e.g. --graphics_commands "save_graphics place.png;")

    * save_graphics <file>
         Saves graphics to the specified file (.png/.pdf/
         .svg). If <file> contains ``{i}``, it will be
         replaced with an integer which increments
         each time graphics is invoked.
    * set_macros <int>
         Sets the placement macro drawing state
    * set_nets <int>
         Sets the net drawing state
    * set_cpd <int>
         Sets the criticla path delay drawing state
    * set_routing_util <int>
         Sets the routing utilization drawing state
    * set_clip_routing_util <int>
         Sets whether routing utilization values are clipped to [0., 1.]. Useful when a consistent scale is needed across images
    * set_draw_block_outlines <int>
         Sets whether blocks have an outline drawn around them
    * set_draw_block_text <int>
         Sets whether blocks have label text drawn on them
    * set_draw_block_internals <int>
         Sets the level to which block internals are drawn
    * set_draw_net_max_fanout <int>
         Sets the maximum fanout for nets to be drawn (if fanout is beyond this value the net will not be drawn)
    * set_congestion <int>
         Sets the routing congestion drawing state
    * exit <int>
         Exits VPR with specified exit code

    Example:

    .. code-block:: none

        "save_graphics place.png; \
        set_nets 1; save_graphics nets1.png;\
        set_nets 2; save_graphics nets2.png; set_nets 0;\
        set_cpd 1; save_graphics cpd1.png; \
        set_cpd 3; save_graphics cpd3.png; set_cpd 0; \
        set_routing_util 5; save_graphics routing_util5.png; \
        set_routing_util 0; \
        set_congestion 1; save_graphics congestion1.png;"

    The above toggles various graphics settings (e.g. drawing nets, drawing critical path) and then saves the results to .png files.

    Note that drawing state is reset to its previous state after these commands are invoked.

    Like the interactive graphics :option`<--disp>` option, the :option:`--auto` option controls how often the commands specified with this option are invoked.

.. _general_options:

General Options
^^^^^^^^^^^^^^^
.. option:: --version

    Display version information then exit.

.. option:: --device <string>

    Specifies which device layout/floorplan to use from the architecture file.  Valid values are:

    * ``auto`` VPR uses the smallest device satisfying the circuit's resource requirements.  This option will use the ``<auto_layout>`` tag if it is present in the architecture file in order to construct the smallest FPGA that has sufficient resources to fit the design. If the ``<auto_layout>`` tag is not present, the ``auto`` option chooses the smallest device amongst all the architecture file's ``<fixed_layout>`` specifications into which the design can be packed.
    * Any string matching ``name`` attribute of a device layout defined with a ``<fixed_layout>`` tag in the :ref:`arch_grid_layout` section of the architecture file.

    If the value specified is neither ``auto`` nor matches the ``name`` attribute value of a ``<fixed_layout>`` tag, VPR issues an error.

    .. note:: If the only layout in the architecture file is a single device specified using ``<fixed_layout>``, it is recommended to always specify the ``--device`` option; this prevents the value ``--device auto`` from interfering with operations supported only for ``<fixed_layout>`` grids.

    **Default:** ``auto``

.. option:: -j, --num_workers <int>

    Controls how many parallel workers VPR may use:

    * ``1`` implies VPR will execute serially,
    * ``>1`` implies VPR may execute in parallel with up to the specified concurency
    * ``0`` implies VPR may execute with up to the maximum concurrency supported by the host machine

    If this option is not specified it may be set from the ``VPR_NUM_WORKERS`` environment variable; otherwise the default is used.

    If this option is set to something other than 1, the following algorithms can be run in parallel:
    
    * Timing Analysis
    * Routing (If routing algorithm is set to parallel or parallel_decomp; See :option:`--router_algorithm`)
    * Portions of analytical placement (If using the analytical placement flow and compiled VPR with Eigen enabled; See :option:`--analytical_place`)

    .. note:: To compile VPR to allow the usage of parallel workers, ``libtbb-dev`` must be installed in the system.

    **Default:** ``1``

.. option:: --timing_analysis {on | off}

    Turn VPR timing analysis off.
    If it is off, you don’t have to specify the various timing analysis parameters in the architecture file.

    **Default:**  ``on``

.. option:: --echo_file {on | off}

    Generates echo files of key internal data structures.
    These files are generally used for debugging vpr, and typically end in ``.echo``

    **Default:** ``off``

.. option:: --verify_file_digests {on | off}

    Checks that any intermediate files loaded (e.g. previous packing/placement/routing) are consistent with the current netlist/architecture.

    If set to ``on`` will error if any files in the upstream dependancy have been modified.
    If set to ``off`` will warn if any files in the upstream dependancy have been modified.

    **Default:** ``on``

.. option:: --target_utilization <float>

    Sets the target device utilization.
    This corresponds to the maximum target fraction of device grid-tiles to be used.
    A value of 1.0 means the smallest device (which fits the circuit) will be used.

    **Default:** ``1.0``


.. option:: --constant_net_method {global | route}

    Specifies how constant nets (i.e. those driven to a constant value) are handled:

     * ``global``: Treat constant nets as globals (not routed)
     * ``route``: Treat constant nets as normal nets (routed)

     **Default:** ``global``

.. option:: --clock_modeling {ideal | route | dedicated_network}

    Specifies how clock nets are handled:

     * ``ideal``: Treat clock pins as ideal (i.e. no routing delays on clocks)
     * ``route``: Treat clock nets as normal nets (i.e. routed using inter-block routing)
     * ``dedicated_network``: Use the architectures dedicated clock network (experimental)

     **Default:** ``ideal``

.. option:: --two_stage_clock_routing {on | off}

    Routes clock nets in two stages using a dedicated clock network.

     * First stage: From the net source (e.g. an I/O pin) to a dedicated clock network root (e.g. center of chip)
     * Second stage: From the clock network root to net sinks.

    Note this option only works when specifying a clock architecture, see :ref:`Clock Architecture Format <clock_architecture_format>`; it does not work when reading a routing resource graph (i.e. :option:`--read_rr_graph`).

     **Default:** ``off``

.. option:: --exit_before_pack {on | off}

    Causes VPR to exit before packing starts (useful for statistics collection).

    **Default:** ``off``

.. option:: --strict_checks {on, off}

    Controls whether VPR enforces some consistency checks strictly (as errors) or treats them as warnings.

    Usually these checks indicate an issue with either the targetted architecture, or consistency issues with VPR's internal data structures/algorithms (possibly harming optimization quality).
    In specific circumstances on specific architectures these checks may be too restrictive and can be turned off.

    .. warning:: Exercise extreme caution when turning this option off -- be sure you completely understand why the issue is being flagged, and why it is OK to treat as a warning instead of an error.

    **Default:** ``on``

.. option:: --terminate_if_timing_fails {on, off}

    Controls whether VPR should terminate if timing is not met after routing.

    **Default:** ``off``

.. _filename_options:

Filename Options
^^^^^^^^^^^^^^^^
VPR by default appends .blif, .net, .place, and .route to the circuit name provided by the user, and looks for an SDC file in the working directory with the same name as the circuit.
Use the options below to override this default naming behaviour.

.. option:: --circuit_file <file>

    Path to technology mapped user circuit in :ref:`BLIF format <vpr_blif_file>`.

    .. note:: If specified the :option:`circuit` positional argument is treated as the circuit name.

    .. seealso:: :option:`--circuit_format`

.. option:: --circuit_format {auto | blif | eblif}

    File format of the input technology mapped user circuit.

    * ``auto``: File format inferred from file extension (e.g. ``.blif`` or ``.eblif``)
    * ``blif``: Strict :ref:`structural BLIF <vpr_blif_file>`
    * ``eblif``: Structural :ref:`BLIF with extensions <vpr_eblif_file>`

    **Default:** ``auto``

.. option:: --net_file <file>

    Path to packed user circuit in :ref:`net format <vpr_net_file>`.

    **Default:** :option:`circuit <circuit>`.net

.. option:: --place_file <file>

    Path to final :ref:`placement file <vpr_place_file>`.

    **Default:** :option:`circuit <circuit>`.place

.. option:: --route_file <file>

    Path to final :ref:`routing file <vpr_route_file>`.

    **Default:** :option:`circuit <circuit>`.route

.. option:: --sdc_file <file>

    Path to SDC timing constraints file.

    If no SDC file is found :ref:`default timing constraints <default_timing_constraints>` will be used.

    **Default:** :option:`circuit <circuit>`.sdc

.. option:: --write_rr_graph <file>

    Writes out the routing resource graph generated at the last stage of VPR in the :ref:`RR Graph file format <vpr_route_resource_file>`. The output can be read into VPR using :option:`--read_rr_graph`.

    <file> describes the filename for the generated routing resource graph. Accepted extensions are ``.xml`` and ``.bin`` to write the graph in XML or binary (Cap'n Proto) format.

.. option:: --read_rr_graph <file>

    Reads in the routing resource graph named <file> loads it for use during the placement and routing stages. Expects a file extension of either ``.xml`` or ``.bin``.

    The routing resource graph overthrows all the architecture definitions regarding switches, nodes, and edges. Other information such as grid information, block types, and segment information are matched with the architecture file to ensure accuracy.

    The file can be obtained through :option:`--write_rr_graph`.

    .. seealso:: :ref:`Routing Resource XML File <vpr_route_resource_file>`.

.. option:: --read_rr_edge_override <file>

    Reads a file that overrides the intrinsic delay of specific edges in RR graph.

    This option should be used with both :option:`--read_rr_graph` and :option:`--write_rr_graph`. When used this way,
    VPR reads the RR graph, updates the delays of selected edges using :option:`--read_rr_edge_override`,
    and writes the updated RR graph. The modified RR graph can then be used in later VPR runs.

.. option:: --read_vpr_constraints <file>

    Reads the :ref:`VPR constraints <vpr_constraints>` that the flow must respect from the specified XML file.

.. option:: --write_vpr_constraints <file>

    Writes out new :ref:`floorplanning constraints <placement_constraints>` based on the current placement to the specified XML file.

.. option:: --read_router_lookahead <file>

    Reads the lookahead data from the specified file instead of computing it. Expects a file extension of either ``.capnp`` or ``.bin``.

.. option:: --write_router_lookahead <file>

    Writes the lookahead data to the specified file. Accepted file extensions are ``.capnp``, ``.bin``, and ``.csv``.

.. option:: --read_placement_delay_lookup <file>

    Reads the placement delay lookup from the specified file instead of computing it. Expects a file extension of either ``.capnp`` or ``.bin``.

.. option:: --write_placement_delay_lookup <file>

    Writes the placement delay lookup to the specified file. Expects a file extension of either ``.capnp`` or ``.bin``.

.. option:: --read_initial_place_file <file>

    Reads in the initial cluster-level placement (in :ref:`.place file format <vpr_place_file>`) from the specified file and uses it as the starting point for annealing improvement, instead of generating an initial placement internally.

.. option:: --write_initial_place_file <file>

    Writes out the clustered netlist placement chosen by the initial placement algorithm to the specified file, in :ref:`.place file format <vpr_place_file>`.

.. option:: --outfile_prefix <string>

    Prefix for output files

.. option:: --read_flat_place <file>

    Reads a file containing the locations of each atom on the FPGA.
    This is used by the packer to better cluster atoms together.

    The flat placement file (which often ends in ``.fplace``) is a text file
    where each line describes the location of an atom. Each line in the flat
    placement file should have the following syntax:

    .. code-block:: none

        <atom_name : str> <x : float> <y : float> <layer : float> <atom_sub_tile : int> <atom_site_idx? : int>

    For example:

    .. code-block:: none

        n523  6 8 0 0 3
        n522  6 8 0 0 5
        n520  6 8 0 0 2
        n518  6 8 0 0 16

    The position of the atom on the FPGA is given by 3 floating point values
    (``x``, ``y``, ``layer``). We allow for the positions of atom to be not
    quite legal (ok to be off-grid) since this flat placement will be fed into
    the packer and placer, which will snap the positions to grid locations. By
    allowing for off-grid positions, the packer can better trade-off where to
    move atom blocks if they cannot be placed at the given position.
    For 2D FPGA architectures, the ``layer`` should be 0.

    The ``sub_tile`` is a clustered placement construct: which cluster-level
    location at a given (x, y, layer) should these atoms go at (relevant when
    multiple clusters can be stacked there). A sub-tile of -1 may be used when
    the sub-tile of an atom is unkown (allowing the packing algorithm to choose
    any sub-tile at the given (x, y, layer) location).

    The ``site_idx`` is an optional index into a linearized list of primitive
    locations within a cluster-level block which may be used as a hint to
    reconstruct clusters.

    .. warning::

        This interface is currently experimental and under active development.

.. option:: --write_flat_place <file>

    Writes the post-placement locations of each atom into a flat placement file.

    For each atom in the netlist, the following information is stored into the
    flat placement file:

    * The x, y, and sub_tile location of the cluster that contains this atom.
    * The flat site index of this atom in its cluster. The flat site index is a
      linearized ID of primitive locations in a cluster. This may be used as a
      hint to reconstruct clusters.

.. _netlist_options:

Netlist Options
^^^^^^^^^^^^^^^
By default VPR will remove buffer LUTs, and iteratively sweep the netlist to remove unused primary inputs/outputs, nets and blocks, until nothing else can be removed.

.. option:: --absorb_buffer_luts {on | off}

    Controls whether LUTs programmed as wires (i.e. implementing logical identity) should be absorbed into the downstream logic.

    Usually buffer LUTS are introduced in BLIF circuits by upstream tools in order to rename signals (like ``assign`` statements in Verilog).
    Absorbing these buffers reduces the number of LUTs required to implement the circuit.

    Ocassionally buffer LUTs are inserted for other purposes, and this option can be used to preserve them.
    Disabling buffer absorption can also improve the matching between the input and post-synthesis netlist/SDF.

    **Default**: ``on``

.. option:: --const_gen_inference {none | comb | comb_seq}

    Controls how constant generators are inferred/detected in the input circuit.
    Constant generators and the signals they drive are not considered during timing analysis.

    * ``none``: No constant generator inference will occur. Any signals which are actually constants will be treated as non-constants.
    * ``comb``: VPR will infer constant generators from combinational blocks with no non-constant inputs (always safe).
    * ``comb_seq``: VPR will infer constant generators from combinational *and* sequential blocks with only constant inputs (usually safe).

    .. note:: In rare circumstances ``comb_seq`` could incorrectly identify certain blocks as constant generators.
              This would only occur if a sequential netlist primitive has an internal state which evolves *completely independently* of any data input (e.g. a hardened LFSR block, embedded thermal sensor).

    **Default**: ``comb_seq``

.. option:: --sweep_dangling_primary_ios {on | off}

    Controls whether the circuits dangling primary inputs and outputs (i.e. those who do not drive, or are not driven by anything) are swept and removed from the netlist.

    Disabling sweeping of primary inputs/outputs can improve the matching between the input and post-synthesis netlists.
    This is often useful when performing formal verification.

    .. seealso:: :option:`--sweep_constant_primary_outputs`

    **Default**: ``on``

.. option:: --sweep_dangling_nets {on | off}

    Controls whether dangling nets (i.e. those who do not drive, or are not driven by anything) are swept and removed from the netlist.

    **Default**: ``on``

.. option:: --sweep_dangling_blocks {on | off}

    Controls whether dangling blocks (i.e. those who do not drive anything) are swept and removed from the netlist.

    **Default**: ``on``

.. option:: --sweep_constant_primary_outputs {on | off}

    Controls whether primary outputs driven by constant values are swept and removed from the netlist.

    .. seealso:: :option:`--sweep_dangling_primary_ios`

    **Default**: ``off``

.. option:: --netlist_verbosity <int>

    Controls the verbosity of netlist processing (constant generator detection, swept netlist components).
    High values produce more detailed output.

    **Default**: ``1``

.. _packing_options:

Packing Options
^^^^^^^^^^^^^^^
AAPack is the packing algorithm built into VPR.
AAPack takes as input a technology-mapped blif netlist consisting of LUTs, flip-flops, memories, mulitpliers, etc and outputs a .net formatted netlist composed of more complex logic blocks.
The logic blocks available on the FPGA are specified through the FPGA architecture file.
For people not working on CAD, you can probably leave all the options to their default values.

.. option:: --connection_driven_clustering {on | off}

    Controls whether or not AAPack prioritizes the absorption of nets with fewer connections into a complex logic block over nets with more connections.

    **Default**: ``on``

.. option:: --allow_unrelated_clustering {on | off | auto}

    Controls whether primitives with no attraction to a cluster may be packed into it.

    Unrelated clustering can increase packing density (decreasing the number of blocks required to implement the circuit), but can significantly impact routability.

    When set to ``auto`` VPR automatically decides whether to enable unrelated clustring based on the targetted device and achieved packing density.

    **Default**:  ``auto``

.. option:: --timing_gain_weight <float>

    A parameter that weights the optimization of timing vs area.

    A value of 0 focuses solely on area, a value of 1 focuses entirely on timing.

    **Default**: ``0.75``

.. option:: --connection_gain_weight <float>

    A tradeoff parameter that controls the optimization of smaller net absorption vs. the optimization of signal sharing.

    A value of 0 focuses solely on signal sharing, while a value of 1 focuses solely on absorbing smaller nets into a cluster.
    This option is meaningful only when connection_driven_clustering is on.

    **Default**:  ``0.9``

.. option:: --timing_driven_clustering {on|off}

    Controls whether or not to do timing driven clustering

    **Default**: ``on``

.. option:: --cluster_seed_type {blend | timing | max_inputs}

    Controls how the packer chooses the first primitive to place in a new cluster.

    ``timing`` means that the unclustered primitive with the most timing-critical connection is used as the seed.

    ``max_inputs`` means the unclustered primitive that has the most connected inputs is used as the seed.

    ``blend`` uses a weighted sum of timing criticality, the number of tightly coupled blocks connected to the primitive, and the number of its external inputs.

    ``max_pins`` selects primitives with the most number of pins (which may be used, or unused).

    ``max_input_pins`` selects primitives with the most number of input pins (which may be used, or unused).

    ``blend2`` An alternative blend formulation taking into account both used and unused pin counts, number of tightly coupled blocks and criticality.

    **Default**: ``blend2`` if timing_driven_clustering is on; ``max_inputs`` otherwise.

.. option:: --clustering_pin_feasibility_filter {on | off}

    Controls whether the pin counting feasibility filter is used during clustering.
    When enabled the clustering engine counts the number of available pins in groups/classes of mutually connected pins within a cluster.
    These counts are used to quickly filter out candidate primitives/atoms/molecules for which the cluster has insufficient pins to route (without performing a full routing).
    This reduces packing run-time.

    **Default:** ``on``

.. option:: --balance_block_type_utilization {on, off, auto}

    Controls how the packer selects the block type to which a primitive will be mapped if it can potentially map to multiple block types.

     * ``on``  : Try to balance block type utilization by picking the block type with the (currenty) lowest utilization.
     * ``off`` : Do not try to balance block type utilization
     * ``auto``: Dynamically enabled/disabled (based on density)

    **Default:** ``auto``

.. option:: --target_ext_pin_util { auto | <float> | <float>,<float> | <string>:<float> | <string>:<float>,<float> }

    Sets the external pin utilization target (fraction between 0.0 and 1.0) during clustering.
    This determines how many pin the clustering engine will aim to use in a given cluster before closing it and opening a new cluster.

    Setting this to ``1.0`` guides the packer to pack as densely as possible (i.e. it will keep adding molecules to the cluster until no more can fit).
    Setting this to a lower value will guide the packer to pack less densely, and instead creating more clusters.
    In the limit setting this to ``0.0`` will cause the packer to create a new cluster for each molecule.

    Typically packing less densely improves routability, at the cost of using more clusters.

    This option can take several different types of values:

    * ``auto`` VPR will automatically determine appropriate target utilizations.

    * ``<float>`` specifies the target input pin utilization for all block types.

        For example:

          * ``0.7`` specifies that all blocks should aim for 70% input pin utilization.

    * ``<float>,<float>`` specifies the target input and output pin utilizations respectively for all block types.

        For example:

          * ``0.7,0.9`` specifies that all blocks should aim for 70% input pin utilization, and 90% output pin utilization.

    * ``<string>:<float>`` and ``<string>:<float>,<float>`` specify the target pin utilizations for a specific block type (as above).

        For example:

          * ``clb:0.7`` specifies that only ``clb`` type blocks should aim for 70% input pin utilization.
          * ``clb:0.7,0.9`` specifies that only ``clb`` type blocks should aim for 70% input pin utilization, and 90% output pin utilization.

    .. note::

        If some pin utilizations are specified, ``auto`` mode is turned off and the utilization target for any unspecified pin types defaults to 1.0 (i.e. 100% utilization).

        For example:

          * ``0.7`` leaves the output pin utilization unspecified, which is equivalent to ``0.7,1.0``.
          * ``clb:0.7,0.9`` leaves the pin utilizations for all other block types unspecified, so they will assume a default utilization of ``1.0,1.0``.

    This option can also take multiple space-separated values.
    For example::

        --target_ext_pin_util clb:0.5 dsp:0.9,0.7 0.8

    would specify that ``clb`` blocks use a target input pin utilization of 50%, ``dsp`` blocks use a targets of 90% and 70% for inputs and outputs respectively, and all other blocks use an input pin utilization target of 80%.

    .. note::

        This option is only a guideline.
        If a molecule  (e.g. a carry-chain with many inputs) would not otherwise fit into a cluster type at the specified target utilization the packer will fallback to using all pins (i.e. a target utilization of ``1.0``).

    .. note::

        This option requires :option:`--clustering_pin_feasibility_filter` to be enabled.

    **Default:** ``auto``


.. option:: --pack_prioritize_transitive_connectivity {on | off}

    Controls whether transitive connectivity is prioritized over high-fanout connectivity during packing.

    **Default:** ``on``

.. option:: --pack_high_fanout_threshold {auto | <int> | <string>:<int>}

    Defines the threshold for high fanout nets within the packer.

    This option can take several different types of values:

    * ``auto`` VPR will automatically determine appropriate thresholds.

    * ``<int>`` specifies the fanout threshold for all block types.

        For example:

          * ``64`` specifies that a threshold of 64 should be used for all blocks.

    * ``<string>:<float>`` specifies the the threshold for a specific block type.

        For example:

          * ``clb:16`` specifies that ``clb`` type blocks should use a threshold of 16.

    This option can also take multiple space-separated values.
    For example::

        --pack_high_fanout_threshold 128 clb:16

    would specify that ``clb`` blocks use a threshold of 16, while all other blocks (e.g. DSPs/RAMs) would use a threshold of 128.

    **Default:** ``auto``

.. option::  --pack_transitive_fanout_threshold <int>

    Packer transitive fanout threshold.

    **Default:** ``4``

.. option::  --pack_feasible_block_array_size <int>

    This value is used to determine the max size of the priority queue for candidates that pass the early filter legality test
    but not the more detailed routing filter.

    **Default:** ``30``

.. option:: --pack_verbosity <int>

    Controls the verbosity of clustering output.
    Larger values produce more detailed output, which may be useful for debugging architecture packing problems.

    **Default:** ``2``

.. option:: --write_block_usage <file>

    Writes out to the file under path <file> cluster-level block usage summary in machine
    readable (JSON or XML) or human readable (TXT) format. Format is selected
    based on the extension of <file>.

.. _placer_options:

Placer Options
^^^^^^^^^^^^^^
The placement engine in VPR places logic blocks using simulated annealing.
By default, the automatic annealing schedule is used :cite:`betz_arch_cad,betz_vpr`.
This schedule gathers statistics as the placement progresses, and uses them to determine how to update the temperature, when to exit, etc.
This schedule is generally superior to any user-specified schedule.
If any of init_t, exit_t or alpha_t is specified, the user schedule, with a fixed initial temperature, final temperature and temperature update factor is used.

.. seealso:: :ref:`timing_driven_placer_options`

.. option:: --seed <int>

    Sets the initial random seed used by the placer.

    **Default:** ``1``

.. option:: --enable_timing_computations {on | off}

    Controls whether or not the placement algorithm prints estimates of the circuit speed of the placement it generates.
    This setting affects statistics output only, not optimization behaviour.

    **Default:** ``on`` if timing-driven placement is specified, ``off`` otherwise.

.. option:: --inner_num <float>

    The number of moves attempted at each temperature in placement can be calculated from inner_num scaled with circuit size or device-circuit size as specified in ``place_effort_scaling``.

    Changing inner_num is the best way to change the speed/quality tradeoff of the placer, as it leaves the highly-efficient automatic annealing schedule on and simply changes the number of moves per temperature.

    Specifying ``-inner_num 10`` will slow the placer by a factor of 10 while typically improving placement quality only by 10% or less (depends on the architecture).
    Hence users more concerned with quality than CPU time may find this a more appropriate value of inner_num.

    **Default:** ``0.5``

.. option:: --place_effort_scaling {circuit | device_circuit}

    Controls how the number of placer moves level scales with circuit and device size:

    * ``circuit``: The number of moves attempted at each temperature is inner_num *  num_blocks^(4/3) in the circuit.
    * ``device_circuit``: The number of moves attempted at each temperature is inner_num * grid_size^(2/3) * num_blocks^(4/3) in the circuit.

    The number of blocks in a circuit is the number of pads plus the number of clbs.

    **Default:** ``circuit``

.. option:: --init_t <float>

    The starting temperature of the anneal for the manual annealing schedule.

    **Default:** ``100.0``

.. option:: --exit_t <float>

    The manual anneal will terminate when the temperature drops below the exit temperature.

    **Default:** ``0.01``

.. option:: --alpha_t <float>

    The temperature is updated by multiplying the old temperature by alpha_t when the manual annealing schedule is enabled.

    **Default:** ``0.8``

.. option:: --fix_pins {free | random}

    Controls how the placer handles I/O pads during placement.

    * ``free``: The placer can move I/O locations to optimize the placement.
    * ``random``: Fixes I/O pads to arbitrary locations and does not allow the placer to move them during the anneal (models the effect of poor board-level I/O constraints).

    Note: the fix_pins option also used to accept a third argument - a place file that specified where I/O pins should be placed. This argument is no longer accepted by         fix_pins. Instead, the fix_clusters option can now be used to lock down I/O pins.

    **Default:** ``free``.

.. option:: --fix_clusters {<file.place>}

    Controls how the placer handles blocks (of any type) during placement.

    * ``<file.place>``: A path to a file listing the desired location of clustered blocks in the netlist.

    This place location file is in the same format as a :ref:`.place file <vpr_place_file>`, but does not require the first two lines which are normally at the top     of a placement file that specify the netlist file, netlist ID, and array size.

    **Default:** ````.

.. option:: --place_algorithm {bounding_box | criticality_timing | slack_timing}

    Controls the algorithm used by the placer.

    ``bounding_box`` Focuses purely on minimizing the bounding box wirelength of the circuit. Turns off timing analysis if specified.

    ``criticality_timing`` Focuses on minimizing both the wirelength and the connection timing costs (criticality * delay).

    ``slack_timing`` Focuses on improving the circuit slack values to reduce critical path delay.

    **Default:**  ``criticality_timing``

.. option:: --place_quench_algorithm {bounding_box | criticality_timing | slack_timing}

    Controls the algorithm used by the placer during placement quench.
    The algorithm options have identical functionality as the ones used by the option ``--place_algorithm``. If specified, it overrides the option ``--place_algorithm`` during placement quench.

    **Default:**  ``criticality_timing``

.. option:: --place_bounding_box_mode {auto_bb | cube_bb | per_layer_bb}

    Specifies the type of the wirelength estimator used during placement. For single layer architectures, cube_bb (a 3D bounding box) is always used (and is the same as per_layer_bb).
    For 3D architectures, cube_bb is appropriate if you can cross between layers at switch blocks, while if you can only cross between layers at output pins per_layer_bb (one bouding box per layer) is more accurate and appropriate.

    ``auto_bb``: The bounding box type is determined automatically based on the cross-layer connections.

    ``cube_bb``: ``cube_bb`` bounding box is used to estimate the wirelength.

    ``per_layer_bb``: ``per_layer_bb`` bounding box is used to estimate the wirelength

    **Default:** ``auto_bb``

.. option:: --place_chan_width <int>

    Tells VPR how many tracks a channel of relative width 1 is expected to need to complete routing of this circuit.
    VPR will then place the circuit only once, and repeatedly try routing the circuit as usual.

    **Default:** ``100``

.. option:: --place_rlim_escape <float>

    The fraction of moves which are allowed to ignore the region limit.
    For example, a value of 0.1 means 10% of moves are allowed to ignore the region limit.

    **Default:** ``0.0``

.. option:: --RL_agent_placement {on | off}

    Uses a Reinforcement Learning (RL) agent in choosing the appropriate move type in placement.
    It activates the RL agent placement instead of using a fixed probability for each move type.

    **Default:** ``on``

.. option:: --place_agent_multistate {on | off}

    Enable a multistate agent in the placement. A second state will be activated late in
    the annealing and in the Quench that includes all the timing driven directed moves.

    **Default:** ``on``

.. option:: --place_agent_algorithm {e_greedy | softmax}

    Controls which placement RL agent is used.

    **Default:** ``softmax``

.. option:: --place_agent_epsilon <float>

    Placement RL agent's epsilon for the epsilon-greedy agent. Epsilon represents
    the percentage of exploration actions taken vs the exploitation ones.

    **Default:** ``0.3``

.. option:: --place_agent_gamma <float>

    Controls how quickly the agent's memory decays. Values between [0., 1.] specify
    the fraction of weight in the exponentially weighted reward average applied to moves
    which occurred greater than moves_per_temp moves ago. Values < 0 cause the
    unweighted reward sample average to be used (all samples are weighted equally)

    **Default:** ``0.05``

.. option:: --place_reward_fun {basic | nonPenalizing_basic | runtime_aware | WLbiased_runtime_aware}

    The reward function used by the placement RL agent to learn the best action at each anneal stage.

    .. note:: The latter two are only available for timing-driven placement.

    **Default:** ``WLbiased_runtime_aware``

.. option:: --place_agent_space {move_type | move_block_type}

    The RL Agent exploration space can be either based on only move types or also consider different block types moved.

    **Default:** ``move_block_type``

.. option:: --place_quench_only {on | off}

    If this option is set to ``on``, the placement will skip the annealing phase and only perform the placement quench.
    This option is useful when the the quality of initial placement is good enough and there is no need to perform the
    annealing phase.

    **Default:** ``off``


.. option:: --placer_debug_block <int>

    .. note:: This option is likely only of interest to developers debugging the placement algorithm

    Controls which block the placer produces detailed debug information for.

    If the block being moved has the same ID as the number assigned to this parameter, the placer will print debugging information about it.

    * For values >= 0, the value is the block ID for which detailed placer debug information should be produced.
    * For value == -1, detailed placer debug information is produced for all blocks.
    * For values < -1, no placer debug output is produced.

    .. warning:: VPR must have been compiled with `VTR_ENABLE_DEBUG_LOGGING` on to get any debug output from this option.

    **Default:** ``-2``

.. option:: --placer_debug_net <int>

    .. note:: This option is likely only of interest to developers debugging the placement algorithm

    Controls which net the placer produces detailed debug information for.

    If a net with the same ID assigned to this parameter is connected to the block that is being moved, the placer will print debugging information about it.

    * For values >= 0, the value is the net ID for which detailed placer debug information should be produced.
    * For value == -1, detailed placer debug information is produced for all nets.
    * For values < -1, no placer debug output is produced.

    .. warning:: VPR must have been compiled with `VTR_ENABLE_DEBUG_LOGGING` on to get any debug output from this option.

    **Default:** ``-2``


.. _timing_driven_placer_options:

Timing-Driven Placer Options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The following options are only valid when the placement engine is in timing-driven mode (timing-driven placement is used by default).

.. option:: --timing_tradeoff <float>

    Controls the trade-off between bounding box minimization and delay minimization in the placer.

    A value of 0 makes the placer focus completely on bounding box (wirelength) minimization, while a value of 1 makes the placer focus completely on timing optimization.

    **Default:**  ``0.5``

.. option:: --recompute_crit_iter <int>

    Controls how many temperature updates occur before the placer performs a timing analysis to update its estimate of the criticality of each connection.

    **Default:**  ``1``

.. option:: --inner_loop_recompute_divider <int>

    Controls how many times the placer performs a timing analysis to update its criticality estimates while at a single temperature.

    **Default:** ``0``

.. option:: --quench_recompute_divider <int>

    Controls how many times the placer performs a timing analysis to update its criticality estimates during a quench.
    If unspecified, uses the value from --inner_loop_recompute_divider.

    **Default:** ``0``

.. option:: --td_place_exp_first <float>

    Controls how critical a connection is considered as a function of its slack, at the start of the anneal.

    If this value is 0, all connections are considered equally critical.
    If this value is large, connections with small slacks are considered much more critical than connections with small slacks.
    As the anneal progresses, the exponent used in the criticality computation gradually changes from its starting value of td_place_exp_first to its final value of :option:`--td_place_exp_last`.

    **Default:** ``1.0``

.. option:: --td_place_exp_last <float>

    Controls how critical a connection is considered as a function of its slack, at the end of the anneal.

    .. seealso:: :option:`--td_place_exp_first`

    **Default:** ``8.0``

.. option:: --place_delay_model {simple, delta, delta_override}

    Controls how the timing-driven placer estimates delays.

     * ``simple`` The placement delay estimator is built from the router lookahead. This takes less CPU time to build and it and still as accurate as the ``delta` model.
     * ``delta`` The router is used to profile delay from various locations in the grid for various differences in position.
     * ``delta_override`` Like ``delta`` but also includes special overrides to ensure effects of direct connects between blocks are accounted for.
       This is potentially more accurate but is more complex and depending on the architecture (e.g. number of direct connects) may increase place run-time.

    **Default:** ``simple``

.. option:: --place_delay_model_reducer {min, max, median, arithmean, geomean}

    When calculating delta delays for the placement delay model how are multiple values combined?

    **Default:** ``min``

.. option:: --place_delay_offset <float>

    A constant offset (in seconds) applied to the placer's delay model.

    **Default:** ``0.0``

.. option:: --place_delay_ramp_delta_threshold <float>

    The delta distance beyond which --place_delay_ramp is applied.
    Negative values disable the placer delay ramp.

    **Default:** ``-1``

.. option:: --place_delay_ramp_slope <float>

    The slope of the ramp (in seconds per grid tile) which is applied to the placer delay model for delta distance beyond :option:`--place_delay_ramp_delta_threshold`.

    **Default:** ``0.0e-9``

.. option:: --place_tsu_rel_margin <float>

    Specifies the scaling factor for cell setup times used by the placer.
    This effectively controls whether the placer should try to achieve extra margin on setup paths.
    For example a value of 1.1 corresponds to requesting 10%% setup margin.

    **Default:** ``1.0``

.. option:: --place_tsu_abs_margin <float>

    Specifies an absolute offset added to cell setup times used by the placer.
    This effectively controls whether the placer should try to achieve extra margin on setup paths.
    For example a value of 500e-12 corresponds to requesting an extra 500ps of setup margin.

    **Default:** ``0.0``

.. option:: --post_place_timing_report <file>

    Name of the post-placement timing report file to generate (not generated if unspecified).


.. _noc_placement_options:

NoC Options
^^^^^^^^^^^^^^
The following options are only used when FPGA device and netlist contain a NoC router.

.. option:: --noc {on | off}

    Enables a NoC-driven placer that optimizes the placement of routers on the NoC. Also, it enables an option in the graphical display that can be used to
    display the NoC on the FPGA.

    **Default:** ``off``

.. option:: --noc_flows_file <file>

    XML file containing the list of traffic flows within the NoC (communication between routers).

    .. note:: noc_flows_file are required to specify if NoC optimization is turned on (--noc on).

.. option:: --noc_routing_algorithm {xy_routing | bfs_routing | west_first_routing | north_last_routing | negative_first_routing | odd_even_routing}

    Controls the algorithm used by the NoC to route packets.

    * ``xy_routing`` Uses the direction oriented routing algorithm. This is recommended to be used with mesh NoC topologies.
    * ``bfs_routing`` Uses the breadth first search algorithm. The objective is to find a route that uses a minimum number of links. This algorithm is not guaranteed to generate deadlock-free traffic flow routes, but can be used with any NoC topology.
    * ``west_first_routing`` Uses the west-first routing algorithm. This is recommended to be used with mesh NoC topologies.
    * ``north_last_routing`` Uses the north-last routing algorithm. This is recommended to be used with mesh NoC topologies.
    * ``negative_first_routing`` Uses the negative-first routing algorithm. This is recommended to be used with mesh NoC topologies.
    * ``odd_even_routing`` Uses the odd-even routing algorithm. This is recommended to be used with mesh NoC topologies.

    **Default:** ``bfs_routing``

.. option:: --noc_placement_weighting <float>

    Controls the importance of the NoC placement parameters relative to timing and wirelength of the design.

    * ``noc_placement_weighting = 0`` means the placement is based solely on timing and wirelength.
    * ``noc_placement_weighting = 1`` means noc placement is considered equal to timing and wirelength.
    * ``noc_placement_weighting > 1`` means the placement is increasingly dominated by NoC parameters.

    **Default:** ``5.0``

.. option:: --noc_aggregate_bandwidth_weighting <float>

    Controls the importance of minimizing the NoC aggregate bandwidth. This value can be >=0, where 0 would mean the aggregate bandwidth has no relevance to placement.
    Other positive numbers specify the importance of minimizing the NoC aggregate bandwidth compared to other NoC-related cost terms.
    Weighting factors for NoC-related cost terms are normalized internally. Therefore, their absolute values are not important, and
    only their relative ratios determine the importance of each cost term.

    **Default:** ``0.38``

.. option:: --noc_latency_constraints_weighting <float>

    Controls the importance of meeting all the NoC traffic flow latency constraints. This value can be >=0, where 0 would mean latency constraints have no relevance to placement.
    Other positive numbers specify the importance of meeting latency constraints compared to other NoC-related cost terms.
    Weighting factors for NoC-related cost terms are normalized internally. Therefore, their absolute values are not important, and
    only their relative ratios determine the importance of each cost term.

    **Default:** ``0.6``

.. option:: --noc_latency_weighting <float>

    Controls the importance of reducing the latencies of the NoC traffic flows.
    This value can be >=0, where 0 would mean the latencies have no relevance to placement
    Other positive numbers specify the importance of minimizing aggregate latency compared to other NoC-related cost terms.
    Weighting factors for NoC-related cost terms are normalized internally. Therefore, their absolute values are not important, and
    only their relative ratios determine the importance of each cost term.

    **Default:** ``0.02``

.. option:: --noc_congestion_weighting <float>

    Controls the importance of reducing the congestion of the NoC links.
    This value can be >=0, where 0 would mean the congestion has no relevance to placement.
    Other positive numbers specify the importance of minimizing congestion compared to other NoC-related cost terms.
    Weighting factors for NoC-related cost terms are normalized internally. Therefore, their absolute values are not important, and
    only their relative ratios determine the importance of each cost term.

    **Default:** ``0.25``

.. option:: --noc_swap_percentage <float>

    Sets the minimum fraction of swaps attempted by the placer that are NoC blocks.
    This value is an integer ranging from [0-100].

    * ``0`` means NoC blocks will be moved at the same rate as other blocks.
    * ``100`` means all swaps attempted by the placer are NoC router blocks.

    **Default:** ``0``

.. option:: --noc_placement_file_name <file>

    Name of the output file that contains the NoC placement information.

    **Default:** ``vpr_noc_placement_output.txt``


.. _analytical_placement_options:

Analytical Placement Options
^^^^^^^^^^^^^^^
Instead of Packing atoms into clusters and placing the clusters into valid tile
sites on the FPGA, Analytical Placement uses analytical techniques to place atoms
on the FPGA device by relaxing the constraints on where they can be placed. This
atom-level placement is then legalized into a clustered placement and passed into
the router in VPR.

Analytical Placement is generally split into three stages:

* Global Placement: Uses analytical techniques to place atoms on the FPGA grid.

* Full Legalization: Legalizes a flat (atom) placement into legal clusters placed on the FPGA grid.

* Detailed Placement: While keeping the clusters legal, performs optimizations on the clustered placement.

.. warning::

    Analytical Placement is experimental and under active development.

.. option:: --ap_analytical_solver {qp-hybrid | lp-b2b}

    Controls which Analytical Solver the Global Placer will use in the AP Flow.
    The Analytical Solver solves for a placement which optimizes some objective
    function, ignorant of the FPGA legality constraints. This provides a "lower-
    bound" solution. The Global Placer will legalize this solution and feed it
    back to the analytical solver to make its solution more legal.

    * ``qp-hybrid`` Solves for a placement that minimizes the quadratic HPWL of
      the flat placement using a hybrid clique/star net model (as described in
      FastPlace :cite:`Viswanathan2005_FastPlace`).
      Uses the legalized solution as anchor-points to pull the solution to a
      more legal solution (similar to the approach from SimPL :cite:`Kim2013_SimPL`).

    * ``lp-b2b`` Solves for a placement that minimizes the linear HPWL of the
      flat placement using the Bound2Bound net model (as described in Kraftwerk2 :cite:`Spindler2008_Kraftwerk2`).
      Uses the legalized solution as anchor-points to pull the solution to a
      more legal solution (similar to the approach from SimPL :cite:`Kim2013_SimPL`).

    **Default:** ``lp-b2b``

.. option:: --ap_partial_legalizer {bipartitioning | flow-based}

    Controls which Partial Legalizer the Global Placer will use in the AP Flow.
    The Partial Legalizer legalizes a placement generated by an Analytical Solver.
    It is used within the Global Placer to guide the solver to a more legal
    solution.

    * ``bipartitioning`` Creates minimum windows around over-dense regions of
      the device bi-partitions the atoms in these windows such that the region
      is no longer over-dense and the atoms are in tiles that they can be placed
      into.

    * ``flow-based`` Flows atoms from regions that are overfilled to regions that
      are underfilled.

    **Default:** ``bipartitioning``

.. option:: --ap_full_legalizer {naive | appack}

    Controls which Full Legalizer to use in the AP Flow.

    * ``naive`` Use a Naive Full Legalizer which will try to create clusters exactly where their atoms are placed.

    * ``appack`` Use APPack, which takes the Packer in VPR and uses the flat atom placement to create better clusters.

    **Default:** ``appack``

.. option:: --ap_detailed_placer {none | annealer}

    Controls which Detailed Placer to use in the AP Flow.

    * ``none`` Do not use any Detailed Placer.

    * ``annealer`` Use the Annealer from the Placement stage as a Detailed Placer. This will use the same Placer Options from the Place stage to configure the annealer.

    **Default:** ``annealer``

.. option:: --ap_timing_tradeoff <float>

    Controls the trade-off between wirelength (HPWL) and delay minimization in the AP flow.

    A value of 0.0 makes the AP flow focus completely on wirelength minimization,
    while a value of 1.0 makes the AP flow focus completely on timing optimization.

    **Default:** ``0.5``

.. option:: --appack_max_dist_th { auto | <regex>:<float>,<float> }

   Sets the maximum candidate distance thresholds for the logical block types
   used by APPack. APPack uses the primitive-level placement produced by the
   global placer to cluster primitives together. APPack uses the thresholds
   here to ignore primitives which are too far away from the cluster being formed.

   When this option is set to "auto", VPR will select good values for these
   thresholds based on the primitives contained within each logical block type.

   Using this option, the user can set the maximum candidate distance threshold
   of logical block types to something else. The strings passed in by the user
   should be of the form ``<regex>:<float>,<float>`` where the regex string is
   used to match the name of the logical block type to set, the first float
   is a scaling term, and the second float is an offset. The threshold will
   be set to max(scale * (W + H), offset), where W and H are the width and height
   of the device. This allows the user to specify a threshold based on the
   size of the device, while also preventing the number from going below "offset".
   When multiple strings are provided, the thresholds are set from left to right,
   and any logical block types which have been unset will be set to their "auto"
   values.

   For example:

     .. code-block:: none

        --appack_max_dist_th .*:0.1,0 "clb|memory:0,5"

   Would set all logical block types to be 0.1 * (W + H), except for the clb and
   memory block, which will be set to a fixed value of 5.

   Another example:

     .. code-block:: none

        --appack_max_dist_th "clb|LAB:0.2,5"

   This will set all of the logical block types to their "auto" thresholds, except
   for logical blocks with the name clb/LAB which will be set to 0.2 * (W + H) or
   5 (whichever is larger).

    **Default:** ``auto``

.. option:: --ap_high_fanout_threshold <int>

    Defines the threshold for high fanout nets within AP flow.

    Ignores the nets that have higher fanouts than the threshold for the analytical solver.

    **Default:** ``256``

.. option:: --ap_verbosity <int>

    Controls the verbosity of the AP flow output.
    Larger values produce more detailed output, which may be useful for
    debugging the algorithms in the AP flow.

    * ``1 <= verbosity < 10`` Print standard, stage-level messages. This will
      print messages at the GP, FL, or DP level.

    * ``10 <= verbosity < 20`` Print more detailed messages of what is happening
      within stages. For example, show high-level information on the legalization
      iterations within the Global Placer.

    * ``20 <= verbosity`` Print very detailed messages on intra-stage algorithms.

    **Default:** ``1``

.. option:: --ap_generate_mass_report {on | off}

    Controls whether to generate a report on how the partial legalizer
    within the AP flow calculates the mass of primitives and the
    capacity of tiles on the device. This report is useful when
    debugging the partial legalizer.

    **Default:** ``off``


.. _router_options:

Router Options
^^^^^^^^^^^^^^
VPR uses a negotiated congestion algorithm (based on Pathfinder) to perform routing.

.. note:: By default the router performs a binary search to find the minimum routable channel width.  To route at a fixed channel width use :option:`--route_chan_width`.

.. seealso:: :ref:`timing_driven_router_options`

.. option:: --flat_routing {on | off}

    If this option is enabled, the *run-flat* router is used instead of the *two-stage* router.
    This means that during the routing stage, all nets, both intra- and inter-cluster, are routed directly from one primitive pin to another primitive pin.
    This increases routing time but can improve routing quality by re-arranging LUT inputs and exposing additional optimization opportunities in architectures with local intra-cluster routing that is not a full crossbar.

    **Default:** ``off``

.. option:: --max_router_iterations <int>

    The number of iterations of a Pathfinder-based router that will be executed before a circuit is declared unrouteable (if it hasn’t routed successfully yet) at a given channel width.

    *Speed-quality trade-off:* reducing this number can speed up the binary search for minimum channel width, but at the cost of some increase in final track count.
    This is most effective if -initial_pres_fac is simultaneously increased.
    Increase this number to make the router try harder to route heavily congested designs.

    **Default:** ``50``

.. option:: --first_iter_pres_fac <float>

    Similar to :option:`--initial_pres_fac`.
    This sets the present overuse penalty factor for the very first routing iteration.
    :option:`--initial_pres_fac` sets it for the second iteration.

    .. note:: A value of ``0.0`` causes congestion to be ignored on the first routing iteration.

    **Default:** ``0.0``

.. option:: --initial_pres_fac <float>

    Sets the starting value of the present overuse penalty factor.

    *Speed-quality trade-off:* increasing this number speeds up the router, at the cost of some increase in final track count.
    Values of 1000 or so are perfectly reasonable.

    **Default:** ``0.5``

.. option:: --pres_fac_mult <float>

    Sets the growth factor by which the present overuse penalty factor is multiplied after each router iteration.

    **Default:** ``1.3``

.. option:: --max_pres_fac <float>

    Sets the maximum present overuse penalty factor that can ever result during routing. Should always be less than 1e25 or so to prevent overflow.
    Smaller values may help prevent circuitous routing in difficult routing problems, but may increase
    the number of routing iterations needed and hence runtime.

    **Default:** ``1000.0``

.. option:: --acc_fac <float>

    Specifies the accumulated overuse factor (historical congestion cost factor).

    **Default:** ``1``

.. option:: --bb_factor <int>

    Sets the distance (in channels) outside of the bounding box of its pins a route can go.
    Larger numbers slow the router somewhat, but allow for a more exhaustive search of possible routes.

    **Default:** ``3``

.. option:: --base_cost_type {demand_only | delay_normalized | delay_normalized_length | delay_normalized_frequency | delay_normalized_length_frequency}

    Sets the basic cost of using a routing node (resource).

    * ``demand_only`` sets the basic cost of a node according to how much demand is expected for that type of node.

    * ``delay_normalized`` is similar to ``demand_only``, but normalizes all these basic costs to be of the same magnitude as the typical delay through a routing resource.

    * ``delay_normalized_length`` like ``delay_normalized``, but scaled by routing resource length.

    * ``delay_normalized_frequency`` like ``delay_normalized``, but scaled inversely by routing resource frequency.

    * ``delay_normalized_length_frequency`` like ``delay_normalized``, but scaled by routing resource length and scaled inversely by routing resource frequency.

    **Default:** ``delay_normalized_length``

.. option:: --bend_cost <float>

    The cost of a bend.
    Larger numbers will lead to routes with fewer bends, at the cost of some increase in track count.
    If only global routing is being performed, routes with fewer bends will be easier for a detailed router to subsequently route onto a segmented routing architecture.

    **Default:** ``1`` if global routing is being performed, ``0`` if combined global/detailed routing is being performed.

.. option:: --route_type {global | detailed}

    Specifies whether global routing or combined global and detailed routing should be performed.

    **Default:**  ``detailed`` (i.e. combined global and detailed routing)

.. option:: --route_chan_width <int>

    Tells VPR to route the circuit at the specified channel width.

    .. note:: If the channel width is >= 0, no binary search on channel capacity will be performed to find the minimum number of tracks required for routing. VPR simply reports whether or not the circuit will route at this channel width.

    **Default:** ``-1`` (perform binary search for minimum routable channel width)

.. option:: --min_route_chan_width_hint <int>

    Hint to the router what the minimum routable channel width is.

    The value provided is used to initialize the binary search for minimum channel width.
    A good hint may speed-up the binary search by avoiding time spent at congested channel widths which are not routable.

    The algorithm is robust to incorrect hints (i.e. it continues to binary search), so the hint does not need to be precise.

    This option may ocassionally produce a different minimum channel width due to the different initialization.

    .. seealso:: :option:`--verify_binary_search`

.. option:: --verify_binary_search {on | off}

    Force the router to check that the channel width determined by binary search is the minimum.

    The binary search ocassionally may not find the minimum channel width (e.g. due to router sub-optimality, or routing pattern issues at a particular channel width).

    This option attempts to verify the minimum by routing at successively lower channel widths until two consecutive routing failures are observed.

.. option:: --router_algorithm {timing_driven | parallel | parallel_decomp}

    Selects which router algorithm to use.

    * ``timing_driven`` is the default single-threaded PathFinder algorithm.

    * ``parallel`` partitions the device to route non-overlapping nets in parallel. Use with the ``-j`` option to specify the number of threads.

    * ``parallel_decomp`` decomposes nets for aggressive parallelization :cite:`kosar2024parallel`. This imposes additional constraints and may result in worse QoR for difficult circuits.

    Note that both ``parallel`` and ``parallel_decomp`` are timing-driven routers.

    **Default:** ``timing_driven``

.. option:: --min_incremental_reroute_fanout <int>

    Incrementally re-route nets with fanout above the specified threshold.

    This attempts to re-use the legal (i.e. non-congested) parts of the routing tree for high fanout nets, with the aim of reducing router execution time.

    To disable, set value to a value higher than the largest fanout of any net.

    **Default:** ``16``

.. option:: --max_logged_overused_rr_nodes <int>

    Prints the information on overused RR nodes to the VPR log file after the each failed routing attempt.

    If the number of overused nodes is above the given threshold ``N``, then only the first ``N`` entries are printed to the logfile.

    **Default:** ``20``

.. option:: --generate_rr_node_overuse_report {on | off}

    Generates a detailed report on the overused RR nodes' information: **report_overused_nodes.rpt**.

    This report is generated only when the final routing attempt fails (i.e. the whole routing process has failed).

    In addition to the information that can be seen via ``--max_logged_overused_rr_nodes``, this report prints out all the net ids that are associated with each overused RR node. Also, this report does not place a threshold upon the number of RR nodes printed.

    **Default:** ``off``

.. option:: --write_timing_summary <file>

    Writes out to the file under path <file> final timing summary in machine
    readable (JSON or XML) or human readable (TXT) format. Format is selected
    based on the extension of <file>. The summary consists of parameters:

    * `cpd` - Final critical path delay (least slack) [ns]
    * `fmax` - Maximal frequency of the implemented circuit [MHz]
    * `swns` - setup Worst Negative Slack (sWNS) [ns]
    * `stns` - Setup Total Negative Slack (sTNS) [ns]


.. option:: --generate_net_timing_report {on | off}

    Generates a report that lists the bounding box, slack, and delay of every routed connection in a design in CSV format (``report_net_timing.csv``). Each row in the CSV corresponds to a single net.

    The report can later be used by other tools to enable further optimizations. For example, the Synopsys synthesis tool (Synplify) can use this information to re-synthesize the design and improve the Quality of Results (QoR).

    Fields in the report are:

    .. code-block:: none
        
        netname         : The name assigned to the net in the atom netlist
        Fanout          : Net's fanout (number of sinks)
        bb_xmin         : X coordinate of the net's bounding box's bottom-left corner
        bb_ymin         : Y coordinate of the net's bounding box's bottom-left corner
        bb_layer_min    : Lowest layer number of the net's bounding box
        bb_xmax         : X coordinate of the net's bounding box's top-right corner
        bb_ymax         : Y coordinate of the net's bounding box's top-right corner
        bb_layer_max    : Highest layer number of the net's bounding box
        src_pin_name    : Name of the net's source pin
        src_pin_slack   : Setup slack of the net's source pin
        sinks           : A semicolon-separated list of sink pin entries, each in the format:
                          <sink_pin_name>,<sink_pin_slack>,<sink_pin_delay>

    Example value for the ``sinks`` field:
    ``"U2.B,0.12,0.5;U3.C,0.10,0.6;U4.D,0.08,0.7"``

    **Default:** ``off``

.. option:: --route_verbosity <int>

    Controls the verbosity of routing output.
    High values produce more detailed output, which can be useful for debugging or understanding the routing process.

    **Default**: ``1``

.. _timing_driven_router_options:

Timing-Driven Router Options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The following options are only valid when the router is in timing-driven mode (the default).

.. option:: --astar_fac <float>

    Sets how aggressive the directed search used by the timing-driven router is.

    Values between 1 and 2 are reasonable, with higher values trading some quality for reduced CPU time.

    **Default:** ``1.2``

.. option:: --astar_offset <float>

    Sets how aggressive the directed search used by the timing-driven router is.
    It is a subtractive adjustment to the lookahead heuristic.

    Values between 0 and 1e-9 are resonable; higher values may increase quality at the expense of run-time.

    **Default:** ``0.0``

.. option:: --router_profiler_astar_fac <float>

    Controls the directedness of the timing-driven router's exploration when doing router delay profiling of an architecture.
    The router delay profiling step is currently used to calculate the place delay matrix lookup.
    Values between 1 and 2 are resonable; higher values trade some quality for reduced run-time.

    **Default:** ``1.2``

.. option:: --enable_parallel_connection_router {on | off}

    Controls whether the MultiQueue-based parallel connection router is used during a single connection routing.

    When enabled, the parallel connection router accelerates the path search for individual source-sink connections using
    multi-threading without altering the net routing order.

    **Default:** ``off``

.. option:: --post_target_prune_fac <float>

    Controls the post-target pruning heuristic calculation in the parallel connection router.

    This parameter is used as a multiplicative factor applied to the VPR heuristic (not guaranteed to be admissible, i.e.,
    might over-predict the cost to the sink) to calculate the 'stopping heuristic' when pruning nodes after the target has
    been reached. The 'stopping heuristic' must be admissible for the path search algorithm to guarantee optimal paths and
    be deterministic.

    Values of this parameter are architecture-specific and have to be empirically found.

    This parameter has no effect if :option:`--enable_parallel_connection_router` is not set.

    **Default:** ``1.2``

.. option:: --post_target_prune_offset <float>

    Controls the post-target pruning heuristic calculation in the parallel connection router.

    This parameter is used as a subtractive offset together with :option:`--post_target_prune_fac` to apply an affine
    transformation on the VPR heuristic to calculate the 'stopping heuristic'. The 'stopping heuristic' must be admissible
    for the path search algorithm to guarantee optimal paths and be deterministic.

    Values of this parameter are architecture-specific and have to be empirically found.

    This parameter has no effect if :option:`--enable_parallel_connection_router` is not set.

    **Default:** ``0.0``

.. option:: --multi_queue_num_threads <int>

    Controls the number of threads used by MultiQueue-based parallel connection router.

    If not explicitly specified, defaults to 1, implying the parallel connection router works in 'serial' mode using only
    one main thread to route.

    This parameter has no effect if :option:`--enable_parallel_connection_router` is not set.

    **Default:** ``1``

.. option:: --multi_queue_num_queues <int>

    Controls the number of queues used by MultiQueue in the parallel connection router.

    Must be set >= 2. A common configuration for this parameter is the number of threads used by MultiQueue * 4 (the number
    of queues per thread).

    This parameter has no effect if :option:`--enable_parallel_connection_router` is not set.

    **Default:** ``2``

.. option:: --multi_queue_direct_draining {on | off}

    Controls whether to enable queue draining optimization for MultiQueue-based parallel connection router.

    When enabled, queues can be emptied quickly by draining all elements if no further solutions need to be explored after
    the target is reached in the path search.

    Note: For this optimization to maintain optimality and deterministic results, the 'ordering heuristic' (calculated by
    :option:`--astar_fac` and :option:`--astar_offset`) must be admissible to ensure emptying queues of entries with higher
    costs does not prune possibly superior solutions. However, you can still enable this optimization regardless of whether
    optimality and determinism are required for your specific use case (in such cases, the 'ordering heuristic' can be
    inadmissible).

    This parameter has no effect if :option:`--enable_parallel_connection_router` is not set.

    **Default:** ``off``

.. option:: --max_criticality <float>

    Sets the maximum fraction of routing cost that can come from delay (vs. coming from routability) for any net.

    A value of 0 means no attention is paid to delay; a value of 1 means nets on the critical path pay no attention to congestion.

    **Default:** ``0.99``

.. option:: --criticality_exp <float>

    Controls the delay - routability tradeoff for nets as a function of their slack.

    If this value is 0, all nets are treated the same, regardless of their slack.
    If it is very large, only nets on the critical path will be routed with attention paid to delay. Other values produce more moderate tradeoffs.

    **Default:** ``1.0``

.. option:: --router_init_wirelength_abort_threshold <float>

    The first routing iteration wirelength abort threshold.
    If the first routing iteration uses more than this fraction of available wirelength routing is aborted.

    **Default:** ``0.85``

.. option:: --incremental_reroute_delay_ripup {on | off | auto}

    Controls whether incremental net routing will rip-up (and re-route) a critical connection for delay, even if the routing is legal.
    ``auto`` enables delay-based rip-up unless routability becomes a concern.

    **Default:** ``auto``

.. option:: --routing_failure_predictor {safe | aggressive | off}

    Controls how aggressive the router is at predicting when it will not be able to route successfully, and giving up early.
    Using this option can significantly reduce the runtime of a binary search for the minimum channel width.

    ``safe`` only declares failure when it is extremely unlikely a routing will succeed, given the amount of congestion existing in the design.

    ``aggressive`` can further reduce the CPU time for a binary search for the minimum channel width but can increase the minimum channel width by giving up on some routings that would succeed.

    ``off`` disables this feature, which can be useful if you suspect the predictor is declaring routing failure too quickly on your architecture.

    .. seealso:: :option:`--verify_binary_search`

    **Default:** ``safe``

.. option:: --routing_budgets_algorithm { disable | minimax | yoyo | scale_delay }

    .. warning:: Experimental

    Controls how the routing budgets are created. Routing budgets are used to guid VPR's routing algorithm to consider both short path and long path timing constraints :cite:`RCV_algorithm`.

    ``disable`` is used to disable the budget feature. This uses the default VPR and ignores hold time constraints.

    ``minimax`` sets the minimum and maximum budgets by distributing the long path and short path slacks depending on the the current delay values. This uses the Minimax-PERT algorithm :cite:`minimax_pert`.

    ``yoyo`` allocates budgets using minimax algorithm (as above), and enables hold slack resolution in the router using the Routing Cost Valleys (RCV) algorithm :cite:`RCV_algorithm`.

    ``scale_delay`` has the minimum budgets set to 0 and the maximum budgets is set to the delay of a net scaled by the pin criticality (net delay/pin criticality).

    **Default:** ``disable``

.. option:: --save_routing_per_iteration {on | off}

    Controls whether VPR saves the current routing to a file after each routing iteration.
    May be helpful for debugging.

    **Default:** ``off``

.. option:: --congested_routing_iteration_threshold CONGESTED_ROUTING_ITERATION_THRESHOLD

    Controls when the router enters a high effort mode to resolve lingering routing congestion.
    Value is the fraction of max_router_iterations beyond which the routing is deemed congested.

    **Default:** ``1.0`` (never)

.. option:: --route_bb_update {static, dynamic}

    Controls how the router's net bounding boxes are updated:

     * ``static`` : bounding boxes are never updated
     * ``dynamic``: bounding boxes are updated dynamically as routing progresses (may improve routability of congested designs)

     **Default:** ``dynamic``

.. option:: --router_high_fanout_threshold ROUTER_HIGH_FANOUT_THRESHOLD

    Specifies the net fanout beyond which a net is considered high fanout.
    Values less than zero disable special behaviour for high fanout nets.

    **Default:** ``64``

.. option:: --router_lookahead {classic, map}

    Controls what lookahead the router uses to calculate cost of completing a connection.

     * ``classic``: The classic VPR lookahead
     * ``map``: A more advanced lookahead which accounts for diverse wire types and their connectivity

     **Default:** ``map``

.. option:: --router_max_convergence_count <float>

    Controls how many times the router is allowed to converge to a legal routing before halting.
    If multiple legal solutions are found the best quality implementation is used.

    **Default:** ``1``

.. option:: --router_reconvergence_cpd_threshold <float>

    Specifies the minimum potential CPD improvement for which the router will continue to attempt re-convergent routing.

    For example, a value of 0.99 means the router will not give up on reconvergent routing if it thinks a > 1% CPD reduction is possible.

     **Default:** ``0.99``

.. option:: --router_initial_timing {all_critical | lookahead}

    Controls how criticality is determined at the start of the first routing iteration.

     * ``all_critical``: All connections are considered timing critical.
     * ``lookahead``: Connection criticalities are determined from timing analysis assuming (best-case) connection delays as estimated by the router's lookahead.

     **Default:** ``all_critical`` for the classic :option:`--router_lookahead`, otherwise ``lookahead``

.. option:: --router_update_lower_bound_delays {on | off}

    Controls whether the router updates lower bound connection delays after the 1st routing iteration.

    **Default:** ``on``

.. option:: --router_first_iter_timing_report <file>

    Name of the timing report file to generate after the first routing iteration completes (not generated if unspecfied).

.. option:: --router_debug_net <int>

    .. note:: This option is likely only of interest to developers debugging the routing algorithm

    Controls which net the router produces detailed debug information for.

    * For values >= 0, the value is the net ID for which detailed router debug information should be produced.
    * For value == -1, detailed router debug information is produced for all nets.
    * For values < -1, no router debug output is produced.

    .. warning:: VPR must have been compiled with `VTR_ENABLE_DEBUG_LOGGING` on to get any debug output from this option.

    **Default:** ``-2``

.. option:: --router_debug_sink_rr ROUTER_DEBUG_SINK_RR

    .. note:: This option is likely only of interest to developers debugging the routing algorithm

    Controls when router debugging is enabled for the specified sink RR.

     * For values >= 0, the value is taken as the sink RR Node ID for which to enable router debug output.
     * For values < 0, sink-based router debug output is disabled.

    .. warning:: VPR must have been compiled with `VTR_ENABLE_DEBUG_LOGGING` on to get any debug output from this option.

    **Default:** ``-2``

.. _analysis_options:

Analysis Options
^^^^^^^^^^^^^^^^

.. option:: --full_stats

    Print out some extra statistics about the circuit and its routing useful for wireability analysis.

    **Default:** off

.. option:: --gen_post_synthesis_netlist { on | off }

    Generates the Verilog and SDF files for the post-synthesized circuit.
    The Verilog file can be used to perform functional simulation and the SDF file enables timing simulation of the post-synthesized circuit.

    The Verilog file contains instantiated modules of the primitives in the circuit.
    Currently VPR can generate Verilog files for circuits that only contain LUTs, Flip Flops, IOs, Multipliers, and BRAMs.
    The Verilog description of these primitives are in the primitives.v file.
    To simulate the post-synthesized circuit, one must include the generated Verilog file and also the primitives.v Verilog file, in the simulation directory.

    .. seealso:: :ref:`timing_simulation_tutorial`

    If one wants to generate the post-synthesized Verilog file of a circuit that contains a primitive other than those mentioned above, he/she should contact the VTR team to have the source code updated.
    Furthermore to perform simulation on that circuit the Verilog description of that new primitive must be appended to the primitives.v file as a separate module.

    **Default:** ``off``

.. option:: --gen_post_implementation_merged_netlist { on | off }

    This option is based on ``--gen_post_synthesis_netlist``.
    The difference is that ``--gen_post_implementation_merged_netlist`` generates a single verilog file with merged top module multi-bit ports of the implemented circuit.
    The name of the file is ``<basename>_merged_post_implementation.v``

    **Default:** ``off``

.. option:: --gen_post_implementation_sdc { on | off }

    Generates an SDC file including a list of constraints that would
    replicate the timing constraints that the timing analysis within
    VPR followed during the flow. This can be helpful for flows that
    use external timing analysis tools that have additional capabilities
    or more detailed delay models than what VPR uses.

    **Default:** ``off``

.. option:: --post_synth_netlist_unconn_inputs { unconnected | nets | gnd | vcc }

    Controls how unconnected input cell ports are handled in the post-synthesis netlist

     * unconnected: leave unconnected
     * nets: connect each unconnected input pin to its own separate undriven net named: ``__vpr__unconn<ID>``, where ``<ID>`` is index assigned to this occurrence of unconnected port in design
     * gnd: tie all to ground (``1'b0``)
     * vcc: tie all to VCC (``1'b1``)

    **Default:** ``unconnected``

.. option:: --post_synth_netlist_unconn_outputs { unconnected | nets }

    Controls how unconnected output cell ports are handled in the post-synthesis netlist

     * unconnected: leave unconnected
     * nets: connect each unconnected output pin to its own separate undriven net named: ``__vpr__unconn<ID>``, where ``<ID>`` is index assigned to this occurrence of unconnected port in design

    **Default:** ``unconnected``

.. option:: --post_synth_netlist_module_parameters { on | off }

    Controls whether the post-synthesis netlist output by VTR can use Verilog parameters
    or not. When using the post-synthesis netlist for external timing analysis,
    some tools cannot accept the netlist if it contains parameters. By setting
    this option to ``off``, VPR will try to represent the netlist using non-parameterized
    modules.

    **Default:** ``on``

.. option:: --timing_report_npaths <int>

    Controls how many timing paths are reported.

    .. note:: The number of paths reported may be less than the specified value, if the circuit has fewer paths.

    **Default:** ``100``

.. option:: --timing_report_detail { netlist | aggregated | detailed }

    Controls the level of detail included in generated timing reports.

    We obtained the following results using the k6_frac_N10_frac_chain_mem32K_40nm.xml architecture and multiclock.blif circuit.

        * ``netlist``: Timing reports show only netlist primitive pins.

          For example:

            .. code-block:: none

                #Path 2
                Startpoint: FFC.Q[0] (.latch clocked by clk)
                Endpoint  : out:out1.outpad[0] (.output clocked by virtual_io_clock)
                Path Type : setup

                Point                                                             Incr      Path
                --------------------------------------------------------------------------------
                clock clk (rise edge)                                            0.000     0.000
                clock source latency                                             0.000     0.000
                clk.inpad[0] (.input)                                            0.000     0.000
                FFC.clk[0] (.latch)                                              0.042     0.042
                FFC.Q[0] (.latch) [clock-to-output]                              0.124     0.166
                out:out1.outpad[0] (.output)                                     0.550     0.717
                data arrival time                                                          0.717

                clock virtual_io_clock (rise edge)                               0.000     0.000
                clock source latency                                             0.000     0.000
                clock uncertainty                                                0.000     0.000
                output external delay                                            0.000     0.000
                data required time                                                         0.000
                --------------------------------------------------------------------------------
                data required time                                                         0.000
                data arrival time                                                         -0.717
                --------------------------------------------------------------------------------
                slack (VIOLATED)                                                          -0.717


        * ``aggregated``: Timing reports show netlist pins, and an aggregated summary of intra-block and inter-block routing delays.

          For example:

            .. code-block:: none

                #Path 2
                Startpoint: FFC.Q[0] (.latch at (3,3) clocked by clk)
                Endpoint  : out:out1.outpad[0] (.output at (3,4) clocked by virtual_io_clock)
                Path Type : setup

                Point                                                             Incr      Path
                --------------------------------------------------------------------------------
                clock clk (rise edge)                                            0.000     0.000
                clock source latency                                             0.000     0.000
                clk.inpad[0] (.input at (4,2))                                   0.000     0.000
                | (intra 'io' routing)                                           0.042     0.042
                | (inter-block routing)                                          0.000     0.042
                | (intra 'clb' routing)                                          0.000     0.042
                FFC.clk[0] (.latch at (3,3))                                     0.000     0.042
                | (primitive '.latch' Tcq_max)                                   0.124     0.166
                FFC.Q[0] (.latch at (3,3)) [clock-to-output]                     0.000     0.166
                | (intra 'clb' routing)                                          0.045     0.211
                | (inter-block routing)                                          0.491     0.703
                | (intra 'io' routing)                                           0.014     0.717
                out:out1.outpad[0] (.output at (3,4))                            0.000     0.717
                data arrival time                                                          0.717

                clock virtual_io_clock (rise edge)                               0.000     0.000
                clock source latency                                             0.000     0.000
                clock uncertainty                                                0.000     0.000
                output external delay                                            0.000     0.000
                data required time                                                         0.000
                --------------------------------------------------------------------------------
                data required time                                                         0.000
                data arrival time                                                         -0.717
                --------------------------------------------------------------------------------
                slack (VIOLATED)                                                          -0.717

            where each line prefixed with ``|`` (pipe character) represent a sub-delay of an edge within the timing graph.

            For instance:

            .. code-block:: none

                FFC.Q[0] (.latch at (3,3)) [clock-to-output]                     0.000     0.166
                | (intra 'clb' routing)                                          0.045     0.211
                | (inter-block routing)                                          0.491     0.703
                | (intra 'io' routing)                                           0.014     0.717
                out:out1.outpad[0] (.output at (3,4))                            0.000     0.717

            indicates that between the netlist pins ``FFC.Q[0]`` and ``out:out1.outpad[0]`` there are delays of:

              * ``45`` ps from the ``.latch`` output pin to an output pin of a ``clb`` block,
              * ``491`` ps through the general inter-block routing fabric, and
              * ``14`` ps from the input pin of a ``io`` block to ``.output``.

            Also note that a connection between two pins can be contained within the same ``clb`` block, and does not use the general inter-block routing network. As an example from a completely different circuit-architecture pair:

            .. code-block:: none

                n1168.out[0] (.names)                                            0.000     0.902
                | (intra 'clb' routing)                                          0.000     0.902
                top^finish_FF_NODE.D[0] (.latch)                                 0.000     0.902

        * ``detailed``: Like ``aggregated``, the timing reports show netlist pins, and an aggregated summary of intra-block. In addition, it includes a detailed breakdown of the inter-block routing delays.

          It is important to note that detailed timing report can only list the components of a non-global
          net, otherwise, it reports inter-block routing as well as an incremental delay of 0, just as in the
          aggregated and netlist reports.


          For example:

            .. code-block:: none

                #Path 2
                Startpoint: FFC.Q[0] (.latch at (3,3) clocked by clk)
                Endpoint  : out:out1.outpad[0] (.output at (3,4) clocked by virtual_io_clock)
                Path Type : setup

                Point                                                             Incr      Path
                --------------------------------------------------------------------------------
                clock clk (rise edge)                                            0.000     0.000
                clock source latency                                             0.000     0.000
                clk.inpad[0] (.input at (4,2))                                   0.000     0.000
                | (intra 'io' routing)                                           0.042     0.042
                | (inter-block routing:global net)                               0.000     0.042
                | (intra 'clb' routing)                                          0.000     0.042
                FFC.clk[0] (.latch at (3,3))                                     0.000     0.042
                | (primitive '.latch' Tcq_max)                                   0.124     0.166
                FFC.Q[0] (.latch at (3,3)) [clock-to-output]                     0.000     0.166
                | (intra 'clb' routing)                                          0.045     0.211
                | (OPIN:1479 side:TOP (3,3))                                     0.000     0.211
                | (CHANX:2073 unnamed_segment_0 length:1 (3,3)->(2,3))           0.095     0.306
                | (CHANY:2139 unnamed_segment_0 length:0 (1,3)->(1,3))           0.075     0.382
                | (CHANX:2040 unnamed_segment_0 length:1 (2,2)->(3,2))           0.095     0.476
                | (CHANY:2166 unnamed_segment_0 length:0 (2,3)->(2,3))           0.076     0.552
                | (CHANX:2076 unnamed_segment_0 length:0 (3,3)->(3,3))           0.078     0.630
                | (IPIN:1532 side:BOTTOM (3,4))                                  0.072     0.703
                | (intra 'io' routing)                                           0.014     0.717
                out:out1.outpad[0] (.output at (3,4))                            0.000     0.717
                data arrival time                                                          0.717

                clock virtual_io_clock (rise edge)                               0.000     0.000
                clock source latency                                             0.000     0.000
                clock uncertainty                                                0.000     0.000
                output external delay                                            0.000     0.000
                data required time                                                         0.000
                --------------------------------------------------------------------------------
                data required time                                                         0.000
                data arrival time                                                         -0.717
                --------------------------------------------------------------------------------
                slack (VIOLATED)                                                          -0.717

            where each line prefixed with ``|`` (pipe character) represent a sub-delay of an edge within the timing graph.
            In the detailed mode, the inter-block routing has now been replaced by the net components.

            For OPINS and IPINS, this is the format of the name:
            | (``ROUTING_RESOURCE_NODE_TYPE:ROUTING_RESOURCE_NODE_ID`` ``side:SIDE`` ``(START_COORDINATES)->(END_COORDINATES)``)

            For CHANX and CHANY, this is the format of the name:
            | (``ROUTING_RESOURCE_NODE_TYPE:ROUTING_RESOURCE_NODE_ID`` ``SEGMENT_NAME`` ``length:LENGTH`` ``(START_COORDINATES)->(END_COORDINATES)``)

            Here is an example of the breakdown:

            .. code-block:: none

                FFC.Q[0] (.latch at (3,3)) [clock-to-output]                     0.000     0.166
                | (intra 'clb' routing)                                          0.045     0.211
                | (OPIN:1479 side:TOP (3,3))                                     0.000     0.211
                | (CHANX:2073 unnamed_segment_0 length:1 (3,3)->(2,3))           0.095     0.306
                | (CHANY:2139 unnamed_segment_0 length:0 (1,3)->(1,3))           0.075     0.382
                | (CHANX:2040 unnamed_segment_0 length:1 (2,2)->(3,2))           0.095     0.476
                | (CHANY:2166 unnamed_segment_0 length:0 (2,3)->(2,3))           0.076     0.552
                | (CHANX:2076 unnamed_segment_0 length:0 (3,3)->(3,3))           0.078     0.630
                | (IPIN:1532 side:BOTTOM (3,4))                                  0.072     0.703
                | (intra 'io' routing)                                           0.014     0.717
                out:out1.outpad[0] (.output at (3,4))                            0.000     0.717

            indicates that between the netlist pins ``FFC.Q[0]`` and ``out:out1.outpad[0]`` there are delays of:

              * ``45`` ps from the ``.latch`` output pin to an output pin of a ``clb`` block,
              * ``0`` ps from the ``clb`` output pin to the ``CHANX:2073`` wire,
              * ``95`` ps from the ``CHANX:2073`` to the ``CHANY:2139`` wire,
              * ``75`` ps from the ``CHANY:2139`` to the ``CHANX:2040`` wore,
              * ``95`` ps from the ``CHANX:2040`` to the ``CHANY:2166`` wire,
              * ``76`` ps from the ``CHANY:2166`` to the ``CHANX:2076`` wire,
              * ``78`` ps from the ``CHANX:2076`` to the input pin of a ``io`` block,
              * ``14`` ps input pin of a ``io`` block to ``.output``.

            In the initial description we referred to the existence of global nets, which also occur in this net:

            .. code-block:: none

                clk.inpad[0] (.input at (4,2))                                   0.000     0.000
                | (intra 'io' routing)                                           0.042     0.042
                | (inter-block routing:global net)                               0.000     0.042
                | (intra 'clb' routing)                                          0.000     0.042
                FFC.clk[0] (.latch at (3,3))                                     0.000     0.042

            Global nets are unrouted nets, and their route trees happen to be null.

            Finally, is interesting to note that the consecutive channel components may not seem to connect. There are two types of occurences:

            1. The preceding channel's ending coordinates extend past the following channel's starting coordinates (example from a different path):

            .. code-block:: none

                | (chany:2113 unnamed_segment_0 length:2 (1, 3) -> (1, 1))       0.116     0.405
                | (chanx:2027 unnamed_segment_0 length:0 (1, 2) -> (1, 2))       0.078     0.482

            It is possible that by opening a switch between (1,2) to (1,1), CHANY:2113 actually only extends from (1,3) to (1,2).

            1. The preceding channel's ending coordinates have no relation to the following channel's starting coordinates.
               There is no logical contradiction, but for clarification, it is best to see an explanation of the VPR coordinate system.
               The path can also be visualized by VPR graphics, as an illustration of this point:

            .. _fig_path_2:

            .. figure:: path_2.*

             Illustration of Path #2 with insight into the coordinate system.

            :numref:`fig_path_2` shows the routing resources used in Path #2 and their locations on the FPGA.

            1. The signal emerges from near the top-right corner of the block to_FFC (OPIN:1479)  and joins the topmost horizontal segment of length 1 (CHANX:2073).

            2. The signal proceeds to the left, then connects to the outermost, blue vertical segment of length 0 (CHANY:2139).

            3. The signal continues downward and attaches to the horizontal segment of length 1 (CHANX:2040).

            4. Of the aforementioned horizontal segment, after travelling one linear unit to the right, the signal jumps on a vertical segment of length 0 (CHANY:2166).

            5. The signal travels upward and promptly connects to a horizontal segment of length 0 (CHANX:2076).

            6. This segment connects to the green destination io (3,4).

        * ``debug``: Like ``detailed``, but includes additional VPR internal debug information such as timing graph node IDs (``tnode``) and routing SOURCE/SINK nodes.

    **Default:** ``netlist``

.. option:: --echo_dot_timing_graph_node { string | int }

    Controls what subset of the timing graph is echoed to a GraphViz DOT file when :option:`vpr --echo_file` is enabled.

    Value can be a string (corresponding to a VPR atom netlist pin name), or an integer representing a timing graph node ID.
    Negative values mean the entire timing graph is dumped to the DOT file.

    **Default:** ``-1``

.. option:: --timing_report_skew { on | off }

    Controls whether clock skew timing reports are generated.

    **Default:** ``off``


.. _power_estimation_options:

Power Estimation Options
^^^^^^^^^^^^^^^^^^^^^^^^
The following options are used to enable power estimation in VPR.

.. seealso:: :ref:`power_estimation` for more details.

.. option:: --power

    Enable power estimation

    **Default:** ``off``

.. option:: --tech_properties <file>

    XML File containing properties of the CMOS technology (transistor capacitances, leakage currents, etc).
    These can be found at ``$VTR_ROOT/vtr_flow/tech/``, or can be created for a user-provided SPICE technology (see :ref:`power_estimation`).

.. option:: --activity_file <file>

    File containing signal activites for all of the nets in the circuit.  The file must be in the format::

        <net name1> <signal probability> <transition density>
        <net name2> <signal probability> <transition density>
        ...

    Instructions on generating this file are provided in :ref:`power_estimation`.

Server Mode Options
^^^^^^^^^^^^^^^^^^^^^^^^

If VPR is in server mode, it listens on a socket for commands from a client. Currently, this is used to enable interactive timing analysis and visualization of timing paths in the VPR UI under the control of a separate client.

The following options are used to enable server mode in VPR.

.. seealso:: :ref:`server_mode` for more details.

.. option:: --server

    Run in server mode. Accept single client application connection and respond to client requests

    **Default:** ``off``

.. option:: --port PORT

    Server port number.

    **Default:** ``60555``

.. seealso:: :ref:`interactive_path_analysis_client`


Show Architecture Resources
^^^^^^^^^^^^^^^^^^^^^^^^
.. option:: --show_arch_resources

    Print the architecture resource report for each device layout and exit normally.

    **Default:** ``off``


Command-line Auto Completion
----------------------------

To simplify using VPR on the command-line you can use the ``dev/vpr_bash_completion.sh`` script, which will enable TAB completion for VPR commandline arguments (based on the output of `vpr -h`).

Simply add:

.. code-block:: bash

    source $VTR_ROOT/dev/vpr_bash_completion.sh

to your ``.bashrc``. ``$VTR_ROOT`` refers to the root of the VTR source tree on your system.
