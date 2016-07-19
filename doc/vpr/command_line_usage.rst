Basic Command-line Usage
========================
.. program:: vpr

At a minimum VPR requires two command-line arguments::

    vpr architecture.xml circuit.blif

where:

  * ``architecture.xml`` is an :ref:`FPGA architecture description <fpga_architecture_description>`
  * ``circuit.blif`` is the technology mapped netlist to be implemented

VPR will then pack, place, and route the circuit onto the specified architecture.

By default VPR will perform a binary search routing to find the minimum channel width required to route the circuit.

Detailed Command-line Options
=============================
VPR has a lot of options.
The options most people will be interested in are:

* :option:`-route_chan_width` (route at a fixed channel width), and 
* :option:`-nodisp` (turn off graphics).

In general for the other options the defaults are fine, and only people looking at how different CAD algorithms perform will try many of them.
To understand what the more esoteric placer and router options actually do, see :cite:`betz_arch_cad` or download :cite:`betz_directional_bias_routing_arch,betz_biased_global_routing_tech_report,betz_vpr,marquardt_timing_driven_placement` from the author’s `web page <http://www.eecg.toronto.edu/~vaughn>`.

In the following text, values in angle brackets e.g. ``<int>`` ``<float>`` ``<string>`` ``<file>``, should be replaced by the appropriate number, string, or file path.
Values in curly braces separated by vertical bars, e.g. ``{on | off}``, indicate all the permissible choices for an option.


.. _filename_options:

Filename Options
----------------
VPR by default appends .blif, .net, .place, and .route to the circuit name provided by the user, and looks for an SDC file in the working directory with the same name as the circuit.
Use the options below to override this default naming behaviour.

.. option:: -blif_file <file>

    Path to technology mapped user circuit in blif format

.. option:: -net_file <file>

    Path to packed user circuit in net format

.. option:: -place_file <file>

    Path to final placement file

.. option:: -route_file <file>

    Path to final routing file

.. option:: -sdc_file <file>

    Path to SDC timing constraints file

.. option:: -outfile_prefix <string>

    Prefix for output files

.. _general_options:

General Options
----------------
VPR runs all three stages of pack, place, and route if none of :option:`-pack`, :option:`-place`, or :option:`-route` are specified.

.. option:: -nodisp

    Disables all graphics. Useful for scripting purposes or if you're not running X Windows.

    **Default:** graphics is enabled (if VPR is compiled with graphics_enabled).

.. option:: -auto <int>

    Can be 0, 1, or 2. 
    This sets how often you must click Proceed to continue execution after viewing the graphics. 
    The higher the number, the more infrequently the program will pause. 

    **Default:** ``1``

.. option:: -pack

    Run packing stage

    **Default:** off

.. option:: -place

    Run placement stage

    **Default:** off

.. option:: -route

    Run routing stage

    **Default:** off

.. option:: -timing_analysis { on | off }

    Turn VPR timing analysis off.  
    If it is off, you don’t have to specify the various timing analysis parameters in the architecture file.  

    **Default:**  ``on``

.. option:: -slack_definition { R | I | S | G | C | N }

    The slack definition used in timing analysis.  
    This option is for experimentation only; the default is fine for ordinary usage.  
    See path_delay.c for details.

    **Default:** ``R``

.. option:: -timing_analyze_only_with_net_delay <float>

    .. deprecated:: 7.0

    Perform timing analysis on netlist assuming all edges have the same specified delay

    **Default:** off

.. option:: -full_stats

    Print out some extra statistics about the circuit and its routing useful for wireability analysis.  

    **Default:** off
    
.. option:: -echo_file { on | off }

    Generates echo files of key internal data structures.
    These files are generally used for debugging vpr, and typically end in ``.echo``

    **Default:** ``off``

.. option:: -gen_postsynthesis_netlist { on | off }

    Generates the Verilog and SDF files for the post-synthesized circuit. 
    The Verilog file can be used to perform functional simulation and the SDF file enables timing simulation of the post-synthesized circuit.

    The Verilog file contains instantiated modules of the primitives in the circuit.
    Currently VPR can generate Verilog files for circuits that only contain LUTs, Flip Flops, IOs, Multipliers, and BRAMs.
    The Verilog description of these primitives are in the primitives.v file.
    To simulate the post-synthesized circuit, one must include the generated Verilog file and also the primitives.v Verilog file, in the simulation directory.

    If one wants to generate the post-synthesized Verilog file of a circuit that contains a primitive other than those mentioned above, he/she should contact the VTR team to have the source code updated.
    Furthermore to perform simulation on that circuit the Verilog description of that new primitive must be appended to the primitives.v file as a separate module.

    **Default:** ``off``

.. _packing_options:

Packing Options
---------------
AAPack is the packing tool built into VPR.
AAPack takes as input a technology-mapped blif netlist consisting of LUTs, flip-flops, memories, mulitpliers, etc and outputs a .net formatted netlist composed of more complex logic blocks.
The logic blocks available on the FPGA are specified through the FPGA architecture file.
For people not working on CAD, you can probably leave all the options to their default values.

.. option:: -connection_driven_clustering {on | off}

    Controls whether or not AAPack prioritizes the absorption of nets with fewer connections into a complex logic block over nets with more connections.

    **Default**: ``on``

.. option:: -allow_unrelated_clustering {on | off}

    Controls whether or not primitives with no attraction to the current cluster can be packed into it.

    **Default**:  ``on``

.. option:: -alpha_clustering <float>

    A parameter that weights the optimization of timing vs area.

    A value of 0 focuses solely on area, a value of 1 focuses entirely on timing. 

    **Default**: ``0.75``

.. option:: -beta_clustering <float>

    A tradeoff parameter that controls the optimization of smaller net absorption vs. the optimization of signal sharing.

    A value of 0 focuses solely on signal sharing, while a value of 1 focuses solely on absorbing smaller nets into a cluster.
    This option is meaningful only when connection_driven_clustering is on.

    **Default**:  ``0.9``

.. option:: -timing_driven_clustering {on|off}

    Controls whether or not to do timing driven clustering

    **Default**: ``on``

.. option:: -cluster_seed_type {blend | timing | max_inputs}

    Controls how the packer chooses the first primitive to place in a new cluster.

    ``timing`` means that the unclustered primitive with the most timing-critical connection is used as the seed.

    ``max_inputs`` means the unclustered primitive that has the most connected inputs is used as the seed.

    ``blend`` uses a weighted sum of timing criticality, the number of tightly coupled blocks connected to the primitive, and the number of its external inputs.

    **Default**: ``blend`` if timing_driven_clustering is on; ``max_inputs`` otherwise.


.. option:: -sweep_hanging_nets_and_inputs {on | off}

    Controls whether hanging/dangling nets and inputs (i.e. those that do not drive anything)) are swept and removed from the netlist.

    **Default**: ``on``

.. option:: -absorb_buffer_luts {on | off}

    Controls whether LUTs programmed as wires (i.e. implementing logical identity) should be absorbed into the downstream logic.

    Usually buffer LUTS are introduced in BLIF circuits by upstream tools in order to rename signals (like ``assign`` statements in Verilog). 
    Absorbing these buffers reduces the number of LUTs required to implement the circuit.

    Ocassionally buffer LUTs are inserted for other purposes, and this option can be used to preserve them.
    Disabling buffer absorption can also improve the matching between the input and post-synthesis netlist/SDF.

    **Default**: ``on``

.. _placer_options:

Placer Options
--------------
The placement engine in VPR places logic blocks using simulated annealing.
By default, the automatic annealing schedule is used :cite:`betz_arch_cad,betz_vpr`.
This schedule gathers statistics as the placement progresses, and uses them to determine how to update the temperature, when to exit, etc.
This schedule is generally superior to any user-specified schedule.
If any of init_t, exit_t or alpha_t is specified, the user schedule, with a fixed initial temperature, final temperature and temperature update factor is used. 

.. seealso:: :ref:`timing_driven_placer_options`

.. option:: -seed <int>

    Sets the initial random seed used by the placer. 

    **Default:** ``1``

.. option:: -enable_timing_computations {on | off} 

    Controls whether or not the placement algorithm prints estimates of the circuit speed of the placement it generates.
    This setting affects statistics output only, not optimization behaviour. 

    **Default:** ``on`` if timing-driven placement is specified, ``off`` otherwise.

.. option:: -block_dist <int> 

    .. deprecated:: 7.0

    Specifies that the placement algorithm should print out an estimate of the circuit critical path, assuming that each inter-block connection is between blocks a (horizontal) distance of block_dist logic blocks apart.
    This setting affects statistics output only, not optimization  behaviour.

    **Default:** ``1`` (Currently the code that prints out this lower bound is #ifdef ’ed out in place.c -- define PRINT_LOWER_BOUND in place.c to reactivate it.)

.. option:: -inner_num <float>

    The number of moves attempted at each temperature is inner_num *  num_blocks^(4/3) in the circuit.
    The number of blocks in a circuit is the number of pads plus the number of clbs.
    Changing inner_num is the best way to change the speed/quality tradeoff of the placer, as it leaves the highly-efficient automatic annealing schedule on and simply changes the number of moves per temperature.

    Specifying ``-inner_num 1`` will speed up the placer by a factor of 10 while typically reducing placement quality only by 10% or less (depends on the architecture).
    Hence users more concerned with CPU time than quality may find this a more appropriate value of inner_num.

    **Default:** ``10.0``

.. option:: -init_t <float>

    The starting temperature of the anneal for the manual annealing schedule. 

    **Default:** ``100.0``

.. option:: -exit_t <float>

    The manual anneal will terminate when the temperature drops below the exit temperature.

    **Default:** ``0.01``

.. option:: -alpha_t <float>

    The temperature is updated by multiplying the old temperature by alpha_t when the manual annealing schedule is enabled. 

    **Default:** ``0.8``

.. option:: -fix_pins {random | <file.pads>}

    Do not allow the placer to move the I/O locations about during the anneal.
    Instead, lock each I/O pad to some location at the start of the anneal. 
    If -fix_pins random is specified, each I/O block is locked to a random pad location to model the effect of poor board-level I/O constraints.
    If any word other than random is specified after -fix_pins, that string is taken to be the name of a file listing the desired location of each I/O block in the netlist (i.e. -fix_pins <file.pads>).
    This pad location file is in the same format as a normal placement file, but only specifies the locations of I/O pads, rather than the locations of all blocks.

    **Default:** off (i.e. placer chooses pad locations).

.. option:: -place_algorithm {bounding_box | net_timing_driven | path_timing_driven}

    Controls the algorithm used by the placer.

    ``bounding_box`` focuses purely on minimizing the bounding box wirelength of the circuit.

    ``path_timing_driven`` focuses on minimizing both wirelength and the critical path delay.

    ``net_timing_driven`` is similar to path_timing_driven, but assumes that all nets have the same delay when estimating the critical path during placement, rather than using the current placement to obtain delay estimates.

    **Default:**  ``path_timing_driven``

.. option:: -place_chan_width <int>

    Tells VPR how many tracks a channel of relative width 1 is expected to need to complete routing of this circuit.
    VPR will then place the circuit only once, and repeatedly try routing the circuit as usual. 

    **Default:** ``100``

.. _timing_driven_placer_options:

Timing-Driven Placer Options
----------------------------
The following options are only valid when the placement engine is in timing-driven mode (timing-driven placement is used by default).

.. option:: -timing_tradeoff <float>

    Controls the trade-off between bounding box minimization and delay minimization in the placer.

    A value of 0 makes the placer focus completely on bounding box (wirelength) minimization, while a value of 1 makes the placer focus completely on timing optimization.

    **Default:**  ``0.5``

.. option:: -recompute_crit_iter <int>

    Controls how many temperature updates occur before the placer performs a timing analysis to update its estimate of the criticality of each connection.

    **Default:**  ``1``

.. option:: -inner_loop_recompute_divider <int>

    Controls how many times the placer performs a timing analysis to update its criticality estimates while at a single temperature. 

    **Default:** ``0``

.. option:: -td_place_exp_first <float>

    Controls how critical a connection is considered as a function of its slack, at the start of the anneal.

    If this value is 0, all connections are considered equally critical.
    If this value is large, connections with small slacks are considered much more critical than connections with small slacks.
    As the anneal progresses, the exponent used in the criticality computation gradually changes from its starting value of td_place_exp_first to its final value of :option:`-td_place_exp_last`. 

    **Default:** ``1.0``

.. option:: -td_place_exp_last <float>

    Controls how critical a connection is considered as a function of its slack, at the end of the anneal.

    .. seealso:: :option:`-td_place_exp_first`

    **Default:** ``8.0``

.. _router_options:

Router Options
--------------
VPR uses a negotiated congestion algorithm (based on Pathfinder) to perform routing.

.. note:: By default the router performs a binary search to find the minimum routable channel width.  To route at a fixed channel width use :option:`-route_chan_width`.

.. seealso:: :ref:`timing_driven_router_options`

.. option:: -max_router_iterations <int>

    The number of iterations of a Pathfinder-based router that will be executed before a circuit is declared unrouteable (if it hasn’t routed successfully yet) at a given channel width. 

    *Speed-quality trade-off:* reducing this number can speed up the binary search for minimum channel width, but at the cost of some increase in final track count. 
    This is most effective if -initial_pres_fac is simultaneously increased.
    Increase this number to make the router try harder to route heavily congested designs.

    **Default:** ``50``

.. option:: -initial_pres_fac <float>

    Sets the starting value of the present overuse penalty factor. 

    *Speed-quality trade-off:* increasing this number speeds up the router, at the cost of some increase in final track count.
    Values of 1000 or so are perfectly reasonable.

    **Default:** ``0.5``

.. option:: -first_iter_pres_fac <float>

    Similar to :option:`-initial_pres_fac`.
    This sets the present overuse penalty factor for the very first routing iteration.
    :option:`-initial_pres_fac` sets it for the second iteration. 

    **Default:** ``0.5``

.. option:: -pres_fac_mult <float>

    Sets the growth factor by which the present overuse penalty factor is multiplied after each router iteration. 

    **Default:** ``1.3``

.. option:: -acc_fac <float>

    Specifies the accumulated overuse factor (historical congestion cost factor).

    **Default:** ``1``

.. option:: -bb_factor <int>

    Sets the distance (in channels) outside of the bounding box of its pins a route can go.
    Larger numbers slow the router somewhat, but allow for a more exhaustive search of possible routes.

    **Default:** ``3``

.. option:: -base_cost_type {demand_only | delay_normalized | intrinsic_delay} 

    Sets the basic cost of using a routing node (resource).

    ``demand_only`` sets the basic cost of a node according to how much demand is expected for that type of node.

    ``delay_normalized`` is similar, but normalizes all these basic costs to be of the same magnitude as the typical delay through a routing resource.

    ``intrinsic_delay`` sets the basic cost of a node to its intrinsic delay.

    .. warning:: ``intrinsic_delay`` is no longer supported and may give unusual results

    **Default:** ``delay_normalized`` for the timing-driven router and ``demand_only`` for the breadth-first router

.. option:: -bend_cost <float>

    The cost of a bend.
    Larger numbers will lead to routes with fewer bends, at the cost of some increase in track count.
    If only global routing is being performed, routes with fewer bends will be easier for a detailed router to subsequently route onto a segmented routing architecture. 

    **Default:** ``1`` if global routing is being performed, ``0`` if combined global/detailed routing is being performed.

.. option:: -route_type {global | detailed}

    Specifies whether global routing or combined global and detailed routing should be performed.

    **Default:**  ``detailed`` (i.e. combined global and detailed routing)

.. option:: -route_chan_width <int>

    Tells VPR to route the circuit with a fixed channel width.

    .. note:: No binary search on channel capacity will be performed to find the minimum number of tracks required for routing. VPR simply reports whether or not the circuit will route at this channel width.

.. option:: -router_algorithm {breadth_first | timing_driven} 

    Selects which router algorithm to use.
    
    The ``breadth_first`` router focuses solely on routing a design successfully, while the ``timing_driven`` router focuses both on achieving a successful route and achieving good circuit speed.  
    
    The breadth-first router is capable of routing a design using slightly fewer tracks than the timing-driving router (typically 5% if the timing-driven router uses its default parameters. 
    This can be reduced to about 2% if the router parameters are set so the timing-driven router pays more attention to routability and less to area).  
    The designs produced by the timing-driven router are much faster, however, (2x - 10x) and it uses less CPU time to route.

    **Default:** ``timing_driven``

.. option:: -min_incremental_reroute_fanout <int>

    Incrementally re-route nets with fanout above the specified threshold.

    This attempts to re-use the legal (i.e. non-congested) parts of the routing tree for high fanout nets, with the aim of reducing router execution time.

    To disable, set value to a value higher than the largest fanout of any net.

    **Default:** 64

.. _timing_driven_router_options:

Timing-Driven Router Options
----------------------------
The following options are only valid when the router is in timing-driven mode (the default).

.. option:: -astar_fac <float>

    Sets how aggressive the directed search used by the timing-driven router is.

    Values between 1 and 2 are reasonable, with higher values trading some quality for reduced CPU time.

    **Default:** ``1.2``

.. option:: -max_criticality <float>

    Sets the maximum fraction of routing cost that can come from delay (vs. coming from routability) for any net.

    A value of 0 means no attention is paid to delay; a value of 1 means nets on the critical path pay no attention to congestion. 

    **Default:** ``0.99``

.. option:: -criticality_exp <float>

    Controls the delay - routability tradeoff for nets as a function of their slack.

    If this value is 0, all nets are treated the same, regardless of their slack.
    If it is very large, only nets on the critical path will be routed with attention paid to delay. Other values produce more moderate tradeoffs. 

    **Default:** ``1.0``

.. option:: -routing_failure_predictor {safe | aggressive | off}

    Controls how aggressive the router is at predicting when it will not be able to route successfully, and giving up early.
    Using this option can significantly reduce the runtime of a binary search for the minimum channel width.

    ``safe`` only declares failure when it is extremely unlikely a routing will succeed, given the amount of congestion existing in the design.

    ``aggressive`` can further reduce the CPU time for a binary search for the minimum channel width but can increase the minimum channel width by giving up on some routings that would succeed.

    ``off`` disables this feature, which can be useful if you suspect the predictor is declaring routing failure too quickly on your architecture.

    **Default:** ``safe``

.. _power_estimation_options:

Power Estimation Options
----------------------------
The following options are used to enable power estimation in VPR.

.. seealso: ref:`power_estimation` for more details.

.. option:: --power

    Enable power estimation

    **Default:** off

.. option:: --tech_properties <file>

    XML File containing properties of the CMOS technology (transistor capacitances, leakage currents, etc).  
    These can be found at ``<vtr_installation>/vtr_flow/tech/``, or can be created for a user-provided SPICE technology (see :ref:`power_estimation`).

.. option:: --activity_file <file>

    File containing signal activites for all of the nets in the circuit.  The file must be in the format::

        <net name1> <signal probability> <transition density>
        <net name2> <signal probability> <transition density>
        ...

    Instructions on generating this file are provided in ref:`power_estimation`.

