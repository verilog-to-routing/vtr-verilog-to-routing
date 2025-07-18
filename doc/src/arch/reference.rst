.. _arch_reference:

Architecture Reference
======================
This section provides a detailed reference for the FPGA Architecture description used by VTR.
The Architecture description uses XML as its representation format.

As a convention, curly brackets ``{`` ``}`` represents an option with each option separated by ``|``.  For example, ``a={1 | 2 | open}`` means field ``a`` can take a value of ``1``, ``2``, or ``open``.

.. _arch_top_level_tags:

Top Level Tags
--------------
The first tag in all architecture files is the ``<architecture>`` tag.
This tag contains all other tags in the architecture file.
The architecture tag contains the following tags:

* ``<models>``
* ``<tiles>``
* ``<layout>``
* ``<device>``
* ``<switchlist>``
* ``<segmentlist>``
* ``<directlist>``
* ``<complexblocklist>``
* ``<noc>``

.. _arch_models:

Recognized BLIF Models (<models>)
---------------------------------
The ``<models>`` tag contains ``<model name="string" never_prune="string">`` tags.
Each ``<model>`` tag describes the BLIF ``.subckt`` model names that are accepted by the FPGA architecture.
The name of the model must match the corresponding name of the BLIF model.

The never_prune flag is optional and can be either:

* false (default)
* true

Normally blocks with no output nets are pruned away by the netlist sweepers in vpr (removed from the netlist); this is the default behaviour. If never_prune = "true" is set on a model, then blocks that are instances of that model will not be swept away during netlist cleanup. This can be helpful for some special blocks that do have only input nets and are required to be placed on the device for some features to be active, so space on the chip is still reserved for them, despite them not driving any connection.
One example is the IDELAYCTRL of the Series7 devices, which takes as input a reference clock and internally controls and synchronizes all the IDELAYs in a specific clock region, with no output net necessary for it to function correctly.

.. note::
    Standard blif structures (``.names``, ``.latch``, ``.input``, ``.output``) are accepted by default, so these models should not be described in the <models> tag.

Each model tag must contain 2 tags: ``<input_ports>`` and ``<output_ports>``.
Each of these contains ``<port>`` tags:

.. arch:tag:: <port name="string" is_clock="{0 | 1} clock="string" combinational_sink_ports="string1 string2 ..."/>

    :req_param name: The port name.

    :opt_param is_clock: Identifies if the port as a clock port.

        .. seealso:: The :ref:`Primitive Timing Modelling Tutorial <arch_model_timing_tutorial>` for usage of ``is_clock`` to model clock control blocks  such as clock generators, clock buffers/gates and clock muxes.

        Default: ``0``

    :opt_param clock: Indicates the port is sequential and controlled by the specified clock (which must be another port on the model marked with ``is_clock=1``). Default: port is treated as combinational (if unspecified)

    :opt_param combinational_sink_ports: A space-separated list of output ports which are combinationally connected to the current input port. Default: No combinational connections (if unspecified)

    Defines the port for a model.

An example models section containing a combinational primitive ``adder`` and a sequential primitive ``single_port_ram`` follows:

.. code-block:: xml

    <models>
      <model name="single_port_ram">
        <input_ports>
          <port name="we" clock="clk" />
          <port name="addr" clock="clk" combinational_sink_ports="out"/>
          <port name="data" clock="clk" combinational_sink_ports="out"/>
          <port name="clk" is_clock="1"/>
        </input_ports>
        <output_ports>
          <port name="out" clock="clk"/>
        </output_ports>
      </model>

      <model name="adder">
        <input_ports>
          <port name="a" combinational_sink_ports="cout sumout"/>
          <port name="b" combinational_sink_ports="cout sumout"/>
          <port name="cin" combinational_sink_ports="cout sumout"/>
        </input_ports>
        <output_ports>
          <port name="cout"/>
          <port name="sumout"/>
        </output_ports>
      </model>
    </models>

Note that for ``single_port_ram`` above, the ports ``we``, ``addr``, ``data``, and ``out`` are sequential since they have a clock specified.
Additionally ``addr`` and ``data`` are shown to be combinationally connected to ``out``; this corresponds to an internal timing path between the ``addr`` and ``data`` input registers, and the ``out`` output registers.

For the ``adder`` the input ports ``a``, ``b`` and ``cin`` are each combinationally connected to the output ports ``cout`` and ``sumout`` (the adder is a purely combinational primitive).

.. seealso:: For more examples of primitive timing modeling specifications see the :ref:`arch_model_timing_tutorial`

.. _arch_global_info:

Global FPGA Information
-----------------------

.. arch:tag:: <tiles>content</tiles>

    Content inside this tag contains a group of ``<pb_type>`` tags that specify the types of functional blocks and their properties.

.. arch:tag:: <layout/>

    Content inside this tag specifies device grid layout.

    .. seealso:: :ref:`arch_grid_layout`

.. arch:tag:: <layer die='int'>content</layer>
    
    Content inside this tag specifies the layout of a single (2D) die; using multiple layer tags one can describe multi-die FPGAs (e.g. 3D stacked FPGAs).

.. arch:tag:: <device>content</device>

    Content inside this tag specifies device information.

    .. seealso:: :ref:`arch_device_info`

.. arch:tag:: <switchlist>content</switchlist>

    Content inside this tag contains a group of <switch> tags that specify the types of switches and their properties.

.. arch:tag:: <segmentlist>content</segmentlist>

    Content inside this tag contains a group of <segment> tags that specify the types of wire segments and their properties.

.. arch:tag:: <complexblocklist>content</complexblocklist>

    Content inside this tag contains a group of ``<pb_type>`` tags that specify the types of functional blocks and their properties.

.. arch:tag:: <noc link_bandwidth="float" link_latency="float" router_latency="float" noc_router_tile_name="string">content</noc>
    
    Content inside this tag specifies the Network-on-Chip (NoC) architecture on the FPGA device and its properties.

.. _arch_grid_layout:

FPGA Grid Layout
----------------
The valid tags within the ``<layout>`` tag are:

.. arch:tag:: <auto_layout aspect_ratio="float">

    :opt_param aspect_ratio:
        The device grid's target aspect ratio (:math:`width / height`)

        **Default**: ``1.0``

    Defines a scalable device grid layout which can be automatically scaled to a desired size.

    .. note:: At most one ``<auto_layout>`` can be specified.

.. _fixed_arch_grid_layout:

.. arch:tag:: <fixed_layout name="string" width="int" height="int">

    :req_param name:
        The unique name identifying this device grid layout.

    :req_param width:
        The device grid width

    :req_param height:
        The device grid height

    Defines a device grid layout with fixed dimensions.

    .. note:: Multiple ``<fixed_layout>`` tags can be specified.

Each ``<auto_layout>`` or ``<fixed_layout>`` tag should contain a set of grid location tags.

FPGA Layer Information
----------------
The layer tag is an optional tag to specify multi-die FPGAs. If not specified, a single-die FPGA with a single die (with index 0) is assumed.

.. arch:tag:: <layer die="int">
    
    :opt_param die:
        Specifies the index of the die; index 0 is assumed to be at the bottom of a stack. 

        **Default**: ``0``

    .. note:: If die number left unspecified, a single-die FPGA (die number = 0) is assumed.
    
    .. code-block:: xml

        <!-- Describe 3D FPGA using layer tag -->
        <fixed_layout name="3D-FPGA" width="device_width" height="device_height">
            <!-- First die (base die) -->
            <layer die="0"/>
                <!-- Specifiy base die Grid layout (e.g., fill with Network-on-Chips) -->
                <fill type="NoC">
            </layer>
            <!-- Second die (upper die) -->
            <layer die="1">
                <!-- Specifiy upper die Grid layout (e.g., fill with logic blocks) -->
                <fill type="LAB">
            </layer>
        </fixed_layout>

    .. note:: Note that all dice have the same width and height. Since we can always fill unused parts of a die with EMPTY blocks this does not restrict us to have the same usable area on each die.

Grid Location Priorities
~~~~~~~~~~~~~~~~~~~~~~~~
Each grid location specification has an associated numeric *priority*.
Larger priority location specifications override those with lower priority.

.. note:: If a grid block is partially overlapped by another block with higher priority the entire lower priority block is removed from the grid.

Empty Grid Locations
~~~~~~~~~~~~~~~~~~~~
Empty grid locations can be specified using the special block type ``EMPTY``.

.. note:: All grid locations default to ``EMPTY`` unless otherwise specified.

.. _grid_expressions:

Grid Location Expressions
~~~~~~~~~~~~~~~~~~~~~~~~~
Some grid location tags have attributes (e.g. ``startx``) which take an *expression* as their argument.
An *expression* can be an integer constant, or simple mathematical formula evaluated when constructing the device grid.

Supported operators include: ``+``, ``-``, ``*``, ``/``, along with ``(`` and ``)`` to override the default evaluation order.
Expressions may contain numeric constants (e.g. ``7``) and the following special variables:

* ``W``: The width of the device
* ``H``: The height of the device
* ``w``: The width of the current block type
* ``h``: The height of the current block type

.. warning:: All expressions are evaluated as integers, so operations such as division may have their result truncated.

As an example consider the expression ``W/2 - w/2``.
For a device width of 10 and a block type of width 3, this would be evaluated as :math:`\lfloor \frac{W}{2} \rfloor - \lfloor \frac{w}{2} \rfloor  = \lfloor \frac{10}{2} \rfloor - \lfloor \frac{3}{2} \rfloor = 5 - 1 = 4`.

Grid Location Tags
~~~~~~~~~~~~~~~~~~

.. arch:tag:: <fill type="string" priority="int"/>

    :req_param type:
        The name of the top-level complex block type (i.e. ``<pb_type>``) being specified.

    :req_param priority:
        The priority of this layout specification.
        Tags with higher priority override those with lower priority.

    Fills the device grid with the specified block type.

    Example:

    .. code-block:: xml

        <!-- Fill the device with CLB blocks -->
        <fill type="CLB" priority="1"/>

    .. figure:: fill_fpga_grid.*

        <fill> CLB example

.. arch:tag:: <perimeter type="string" priority="int"/>

    :req_param type:
        The name of the top-level complex block type (i.e. ``<pb_type>``) being specified.

    :req_param priority:
        The priority of this layout specification.
        Tags with higher priority override those with lower priority.

    Sets the perimeter of the device (i.e. edges) to the specified block type.

    .. note:: The perimeter includes the corners

    Example:

    .. code-block:: xml

        <!-- Create io blocks around the device perimeter -->
        <perimeter type="io" priority="10"/>

    .. figure:: perimeter_fpga_grid.*

        <perimeter> io example

.. arch:tag:: <corners type="string" priority="int"/>

    :req_param type:
        The name of the top-level complex block type (i.e. ``<pb_type>``) being specified.

    :req_param priority:
        The priority of this layout specification.
        Tags with higher priority override those with lower priority.

    Sets the corners of the device to the specified block type.

    Example:

    .. code-block:: xml

        <!-- Create PLL blocks at all corners -->
        <corners type="PLL" priority="20"/>

    .. figure:: corners_fpga_grid.*

        <corners> PLL example

.. arch:tag:: <single type="string" priority="int" x="expr" y="expr"/>

    :req_param type:
        The name of the top-level complex block type (i.e. ``<pb_type>``) being specified.

    :req_param priority:
        The priority of this layout specification.
        Tags with higher priority override those with lower priority.

    :req_param x:
        The horizontal position of the block type instance.

    :req_param y:
        The vertical position of the block type instance.

    Specifies a single instance of the block type at a single grid location.

    Example:

    .. code-block:: xml

        <!-- Create a single instance of a PCIE block (width 3, height 5)
             at location (1,1)-->
        <single type="PCIE" x="1" y="1" priority="20"/>

    .. figure:: single_fpga_grid.*

        <single> PCIE example

.. arch:tag:: <col type="string" priority="int" startx="expr" repeatx="expr" starty="expr" incry="expr"/>

    :req_param type:
        The name of the top-level complex block type (i.e. ``<pb_type>``) being specified.

    :req_param priority:
        The priority of this layout specification.
        Tags with higher priority override those with lower priority.

    :req_param startx:
        An expression specifying the horizontal starting position of the column.

    :opt_param repeatx:
        An expression specifying the horizontal repeat factor of the column.

    :opt_param starty:
        An expression specifying the vertical starting offset of the column.

        **Default:** ``0``

    :opt_param incry:
        An expression specifying the vertical increment between block instantiations within the region.

        **Default:** ``h``

    Creates a column of the specified block type at ``startx``.

    If ``repeatx`` is specified the column will be repeated wherever :math:`x = startx + k \cdot repeatx`, is satisfied for any positive integer :math:`k`.

    A non-zero ``starty`` is typically used if a ``<perimeter>`` tag is specified to adjust the starting position of blocks with height > 1.

    Example:

    .. code-block:: xml

        <!-- Create a column of RAMs starting at column 2, and
             repeating every 3 columns -->
        <col type="RAM" startx="2" repeatx="3" priority="3"/>

    .. figure:: col_fpga_grid.*

        <col> RAM example

    Example:

    .. code-block:: xml

        <!-- Create IO's around the device perimeter -->
        <perimeter type="io" priority=10"/>

        <!-- Create a column of RAMs starting at column 2, and
             repeating every 3 columns. Note that a vertical offset
             of 1 is needed to avoid overlapping the IOs-->
        <col type="RAM" startx="2" repeatx="3" starty="1" priority="3"/>

    .. figure:: col_perim_fpga_grid.*

        <col> RAM and <perimeter> io example

.. arch:tag:: <row type="string" priority="int" starty="expr" repeaty="expr" startx="expr"/>

    :req_param type:
        The name of the top-level complex block type (i.e. ``<pb_type>``) being specified.

    :req_param priority:
        The priority of this layout specification.
        Tags with higher priority override those with lower priority.

    :req_param starty:
        An expression specifying the vertical starting position of the row.

    :opt_param repeaty:
        An expression specifying the vertical repeat factor of the row.

    :opt_param startx:
        An expression specifying the horizontal starting offset of the row.

        **Default:** ``0``

    :opt_param incrx:
        An expression specifying the horizontal increment between block instantiations within the region.

        **Default:** ``w``

    Creates a row of the specified block type at ``starty``.

    If ``repeaty`` is specified the column will be repeated wherever :math:`y = starty + k \cdot repeaty`, is satisfied for any positive integer :math:`k`.

    A non-zero ``startx`` is typically used if a ``<perimeter>`` tag is specified to adjust the starting position of blocks with width > 1.

    Example:

    .. code-block:: xml

        <!-- Create a row of DSPs (width 1, height 3) at
             row 1 and repeating every 7th row -->
        <row type="DSP" starty="1" repeaty="7" priority="3"/>

    .. figure:: row_fpga_grid.*

        <row> DSP example

.. arch:tag:: <region type="string" priority="int" startx="expr" endx="expr repeatx="expr" incrx="expr" starty="expr" endy="expr" repeaty="expr" incry="expr"/>

    :req_param type:
        The name of the top-level complex block type (i.e. ``<pb_type>``) being specified.

    :req_param priority:
        The priority of this layout specification.
        Tags with higher priority override those with lower priority.

    :opt_param startx:
        An expression specifying the horizontal starting position of the region (inclusive).

        **Default:** ``0``

    :opt_param endx:
        An expression specifying the horizontal ending position of the region (inclusive).

        **Default:** ``W - 1``

    :opt_param repeatx:
        An expression specifying the horizontal repeat factor of the column.

    :opt_param incrx:
        An expression specifying the horizontal increment between block instantiations within the region.

        **Default:** ``w``

    :opt_param starty:
        An expression specifying the vertical starting position of the region (inclusive).

        **Default:** ``0``

    :opt_param endy:
        An expression specifying the vertical ending position of the region (inclusive).

        **Default:** ``H - 1``

    :opt_param repeaty:
        An expression specifying the vertical repeat factor of the column.

    :opt_param incry:
        An expression specifying the vertical increment between block instantiations within the region.

        **Default:** ``h``


    Fills the rectangular region defined by (``startx``, ``starty``) and (``endx``, ``endy``) with the specified block type.

    .. note:: ``endx`` and ``endy`` are included in the region

    If ``repeatx`` is specified the region will be repeated wherever :math:`x = startx + k_1*repeatx`, is satisified for any positive integer :math:`k_1`.

    If ``repeaty`` is specified the region will be repeated wherever :math:`y = starty + k_2*repeaty`, is satisified for any positive integer :math:`k_2`.


    Example:

    .. code-block:: xml

        <!-- Fill RAMs withing the rectangular region bounded by (1,1) and (5,4) -->
        <region type="RAM" startx="1" endx="5" starty="1" endy="4" priority="4"/>

    .. figure:: region_single_fpga_grid.*

        <region> RAM example

    Example:

    .. code-block:: xml

        <!-- Create RAMs every 2nd column withing the rectangular region bounded
             by (1,1) and (5,4) -->
        <region type="RAM" startx="1" endx="5" starty="1" endy="4" incrx="2" priority="4"/>

    .. figure:: region_incr_fpga_grid.*

        <region> RAM increment example

    Example:

    .. code-block:: xml

        <!-- Fill RAMs within a rectangular 2x4 region and repeat every 3 horizontal
             and 5 vertical units -->
        <region type="RAM" startx="1" endx="2" starty="1" endy="4" repeatx="3" repeaty="5" priority="4"/>

    .. figure:: region_repeat_fpga_grid.*

        <region> RAM repeat example

    Example:

    .. code-block:: xml

        <!-- Create a 3x3 mesh of NoC routers (width 2, height 2) whose relative positions
             will scale with the device dimensions -->
        <region type="NoC" startx="W/4 - w/2" starty="W/4 - w/2" incrx="W/4" incry="W/4" priority="3"/>

    .. figure:: region_incr_mesh_fpga_grid.*

        <region> NoC mesh example

Grid Layout Example
~~~~~~~~~~~~~~~~~~~

.. code-block:: xml

    <layout>
        <!-- Specifies an auto-scaling square FPGA floorplan -->
        <auto_layout aspect_ratio="1.0">
            <!-- Create I/Os around the device perimeter -->
            <perimeter type="io" priority=10"/>

            <!-- Nothing in the corners -->
            <corners type="EMPTY" priority="100"/>

            <!-- Create a column of RAMs starting at column 2, and
                 repeating every 3 columns. Note that a vertical offset (starty)
                 of 1 is needed to avoid overlapping the IOs-->
            <col type="RAM" startx="2" repeatx="3" starty="1" priority="3"/>

            <!-- Create a single PCIE block along the bottom, overriding
                 I/O and RAM slots -->
            <single type="PCIE" x="3" y="0" priority="20"/>

            <!-- Create an additional row of I/Os just above the PCIE,
                 which will not override RAMs -->
            <row type="io" starty="5" priority="2"/>

            <!-- Fill remaining with CLBs -->
            <fill type="CLB" priority="1"/>
        </auto_layout>
    </layout>

.. figure:: fpga_grid_example.*

    Example FPGA grid

.. _arch_device_info:

FPGA Device Information
-----------------------
The tags within the ``<device>`` tag are:

.. arch:tag:: <sizing R_minW_nmos="float" R_minW_pmos="float"/>

    :req_param R_minW_nmos:
        The resistance of minimum-width nmos transistor.
        This data is used only by the area model built into VPR.

    :req_param R_minW_pmos:
        The resistance of minimum-width pmos transistor.
        This data is used only by the area model built into VPR.

    :required: Yes

    Specifies parameters used by the area model built into VPR.


.. arch:tag:: <connection_block input_switch_name="string"/>

        .. figure:: ipin_diagram.*

            Input Pin Diagram.


    :req_param switch_name:
        Specifies the name of the ``<switch>`` in the ``<switchlist>`` used to connect routing tracks to block input pins (i.e. the input connection block switch).

    :required: Yes


.. arch:tag:: <area grid_logic_tile_area="float"/>

    :required: Yes

    Specifies the default area used by each 1x1 grid logic tile (in :term:`MWTAs<MWTA>`), *excluding routing*.

    Used for an area estimate of the amount of area taken by all the functional blocks.

    .. note:: This value can be overriden for specific ``<pb_type>``s with the ``area`` attribute.


.. arch:tag:: <switch_block type="{wilton | subset | universal | custom}" fs="int"/>

    :req_param type: The type of switch block to use.
    :req_param fs: The value of :math:`F_s`


    :required: Yes

    This parameter controls the pattern of switches used to connect the (inter-cluster) routing segments. Three fairly simple patterns can be specified with a single keyword each, or more complex custom patterns can be specified.

    **Non-Custom Switch Blocks:**

    When using bidirectional segments, all the switch blocks have :math:`F_s` = 3 :cite:`brown_fpgas`.
    That is, whenever horizontal and vertical channels intersect, each wire segment can connect to three other wire segments.
    The exact topology of which wire segment connects to which can be one of three choices.
    The subset switch box is the planar or domain-based switch box used in the Xilinx 4000 FPGAs -- a wire segment in track 0 can only connect to other wire segments in track 0 and so on.
    The wilton switch box is described in :cite:`wilton_phd`, while the universal switch box is described in :cite:`chang_universal_switch_modules`.
    To see the topology of a switch box, simply hit the "Toggle RR" button when a completed routing is on screen in VPR.
    In general the wilton switch box is the best of these three topologies and leads to the most routable FPGAs.

    When using unidirectional segments, one can specify an :math:`F_s` that is any multiple of 3.
    We use a modified wilton switch block pattern regardless of the specified switch_block_type.
    For all segments that start/end at that switch block, we follow the wilton switch block pattern.
    For segments that pass through the switch block that can also turn there, we cannot use the wilton pattern because a unidirectional segment cannot be driven at an intermediate point, so we assign connections to starting segments following a round robin scheme (to balance mux size).

    .. note:: The round robin scheme is not tileable.

    **Custom Switch Blocks:**

    Specifying ``custom`` allows custom switch blocks to be described under the ``<switchblocklist>`` XML node, the format for which is described in :ref:`custom_switch_blocks`.
    If the switch block is specified as ``custom``, the ``fs`` field does not have to be specified, and will be ignored if present.

.. arch:tag:: <chan_width_distr>content</chan_width_distr>

    Content inside this tag is only used when VPR is in global routing mode.
    The contents of this tag are described in :ref:`global_routing_info`.

.. arch:tag:: <default_fc in_type="{frac|abs}" in_val="{int|float}" out_type="{frac|abs}" out_val="{int|float}"/>

    This defines the default Fc specification, if it is not specified within a ``<fc>`` tag inside a top-level complex block.
    The attributes have the same meaning as the :ref:`\<fc\> tag attributes <arch_fc>`.

.. _arch_switches:

Switches
--------
The tags within the ``<switchlist>`` tag specifies the switches used to connect wires and pins together.

.. arch:tag::
    <switch type="{mux|tristate|pass_gate|short|buffer}" name="string" R="float" Cin="float" Cout="float" Cinternal="float" Tdel="float" buf_size="{auto|float}" mux_trans_size="float", power_buf_size="int"/>

    Describes a switch in the routing architecture.

    **Example:**

    .. code-block:: xml

        <switch type="mux" name="my_awesome_mux" R="551" Cin=".77e-15" Cout="4e-15" Cinternal="5e-15" Tdel="58e-12" mux_trans_size="2.630740" buf_size="27.645901"/>


    :req_param type:

        The type of switch:

        * ``mux``: An isolating, configurable multiplexer

        * ``tristate``: An isolating, configurable tristate-able buffer

        * ``pass_gate``: A *non-isolating*, configurable pass gate

        * ``short``: A *non-isolating*, *non-configurable* electrical short (e.g. between two segments).

        * ``buffer``: An isolating, *non-configurable* non-tristate-able buffer (e.g. in-line along a segment).

        **Isolation**

        Isolating switches include a buffer which partition their input and output into separate DC-connected sub-circuits.
        This helps reduce RC wire delays.

        *Non-isolating* switch do **not** isolate their input and output, which can increase RC wire delays.

        **Configurablity**

        Configurable switches can be turned on/off at configuration time.

        *Non-configurable* switches can **not** be controlled at configuration time.
        These are typically used to model non-optional connections such as electrical shorts and in-line buffers.

    :req_param name: A unique name identifying the switch
    :req_param R: Resistance of the switch.
    :req_param Cin:  Input capacitance of the switch.
    :req_param Cout:  Output capacitance of the switch.

    :opt_param Cinternal: 
        Since multiplexers and tristate buffers are modeled as a       
        parallel stream of pass transistors feeding into a buffer,     
        we would expect an additional "internal capacitance" to arise when the    
        pass transistor is enabled and the signal must propogate to    
        the buffer. See diagram of one stream below:: 
        
            Pass Transistor                                          
                      |                                                   
                    -----                                                 
                    -----      Buffer                                     
                   |     |       |\                                       
             ------       -------| \--------                              
               |             |   | /    |                                 
             =====         ===== |/   =====                               
             =====         =====      =====                               
               |             |          |                                 
             Input C    Internal C    Output C                             
    
        .. note:: Only specify a value for multiplexers and/or tristate switches.

    :opt_param Tdel:

        Intrinsic delay through the switch.
        If this switch was driven by a zero resistance source, and drove a zero capacitance load, its delay would be: :math:`T_{del} + R \cdot C_{out}`.

        The ‘switch’ includes both the mux and buffer ``mux`` type switches.

        .. note:: Required if no ``<Tdel>`` tags are specified

        .. note:: A ``<switch>``'s resistance (``R``) and output capacitance (``Cout``) have no effect on delay when used for the input connection block, since VPR does not model the resistance/capacitance of block internal wires.

    :opt_param buf_size:

        Specifies the buffer size in minimum-width transistor area (:term`MWTA`) units.

        If set to ``auto``, sized automatically from the R value.
        This allows you to use timing models without R’s and C’s and still be able to measure area.

        .. note:: Required for all **isolating** switch types.

        **Default:** ``auto``

    :opt_param mux_trans_size:
        Specifies the size (in minimum width transistors) of each transistor in the two-level mux used by ``mux`` type switches.

        .. note:: Valid only for ``mux`` type switches.

    :opt_param power_buf_size: *Used for power estimation.* The size is the drive strength of the buffer, relative to a minimum-sized inverter.

    .. arch:tag:: <Tdel num_inputs="int" delay="float"/>

        Instead of specifying a single Tdel value, a list of Tdel values may be specified for different values of switch fan-in.
        Delay is linearly extrapolated/interpolated for any unspecified fanins based on the two closest fanins.


        :req_param num_inputs: The number of switch inputs (fan-in)
        :req_param delay: The intrinsic switch delay when the switch topology has the specified number of switch inputs

        **Example:**

        .. code-block:: xml

            <switch type="mux" name="my_mux" R="522" Cin="3.1e-15" Cout="3e-15" Cinternal="5e-15" mux_trans_size="1.7" buf_size="23">
                <Tdel num_inputs="12" delay="8.00e-11"/>
                <Tdel num_inputs="15" delay="8.4e-11"/>
                <Tdel num_inputs="20" delay="9.4e-11"/>
            </switch>


.. _global_routing_info:

Global Routing Information
~~~~~~~~~~~~~~~~~~~~~~~~~~
If global routing is to be performed, channels in different directions and in different parts of the FPGA can be set to different relative widths.
This is specified in the content within the ``<chan_width_distr>`` tag.

.. note:: If detailed routing is to be performed, only uniform distributions may be used

.. arch:tag:: <x distr="{gaussian|uniform|pulse|delta}" peak="float" width=" float" xpeak=" float" dc=" float"/>

    :req_param distr: The channel width distribution function
    :req_param peak: The peak value of the distribution
    :opt_param width: The width of the distribution. Required for ``pulse`` and ``gaussian``.
    :opt_param xpeak: Peak location horizontally. Required for ``pulse``, ``gaussian`` and ``delta``.
    :opt_param dc: The DC level of the distribution. Required for ``pulse``, ``gaussian`` and ``delta``.

    Sets the distribution of tracks for the x-directed channels -- the channels that run horizontally.

    Most values are from 0 to 1.

    If uniform is specified, you simply specify one argument, peak.
    This value (by convention between 0 and 1) sets the width of the x-directed core channels relative to the y-directed channels and the channels between the pads and core.
    :numref:`fig_arch_channel_distribution` should clarify the specification of uniform (dashed line) and pulse (solid line) channel widths.
    The gaussian keyword takes the same four parameters as the pulse keyword, and they are all interpreted in exactly the same manner except that in the gaussian case width is the standard deviation of the function.

    .. _fig_arch_channel_distribution:

    .. figure:: channel_distribution.*

        Channel Distribution

    The delta function is used to specify a channel width distribution in which all the channels have the same width except one.
    The syntax is chan_width_x delta peak xpeak dc.
    Peak is the extra width of the single wide channel.
    Xpeak is between 0 and 1 and specifies the location within the FPGA of the extra-wide channel -- it is the fractional distance across the FPGA at which this extra-wide channel lies.
    Finally, dc specifies the width of all the other channels.
    For example, the statement chan_width_x delta 3 0.5 1 specifies that the horizontal channel in the middle of the FPGA is four times as wide as the other channels.

    Examples::

        <x distr="uniform" peak="1"/>
        <x distr="gaussian" width="0.5" peak="0.8" xpeak="0.6" dc="0.2"/>

.. arch:tag:: <y distr="{gaussian|uniform|pulse|delta}" peak=" float" width=" float" xpeak=" float" dc=" float"/>

    Sets the distribution of tracks for the y-directed channels.

    .. seealso:: <x distr>

.. _arch_tiles:

Physical Tiles
--------------

The content within the ``<tiles>`` describes the physical tiles available in the FPGA.
Each tile type is specified with the ``<tile>`` tag withing the ``<tiles>`` tag.

Tile
~~~~
.. arch:tag:: <tile name="string" width="int" height="int" area="float"/>

    A tile refers to a placeable element within an FPGA architecture and describes its physical compositions on the grid.
    The following attributes are applicable to each tile.
    The only required one is the name of the tile.

    **Attributes:**

    :req_param name: The name of this tile.

        The name must be unique with respect to any other sibling ``<tile>`` tag.

    :opt_param width: The width of the block type in grid tiles

        **Default:** ``1``

    :opt_param height: The height of the block type in grid tiles

        **Default:** ``1``

    :opt_param area: The logic area (in :term:`MWTA`) of the block type

        **Default:** from the ``<area>`` tag

The following tags are common to all ``<tile>`` tags:


.. arch:tag:: <sub_tile name"string" capacity="{int}">

    .. seealso:: For a tutorial on describing the usage of sub tiles for ``heterogeneous tiles`` (tiles which support multiple instances of the same or different :ref:`arch_complex_blocks`) definition see :ref:`heterogeneous_tiles_tutorial`.

    Describes one or many sub tiles corresponding to the physical tile.
    Each sub tile is identifies a set of one or more stack location on a specific x, y grid location.

    **Attributes:**

    :req_param name: The name of this tile.

        The name must be unique with respect to any other sibling ``<tile>`` tag.

    :opt_param capacity: The number of instances of this block type at each grid location.

        **Default:** ``1``

        For example:

        .. code-block:: xml

            <sub_tile name="IO" capacity="2"/>
                ...
            </sub_tile>

        specifies there are two instances of the block type ``IO`` at each of its grid locations.

    .. note:: It is mandatory to have at least one sub tile definition for each physical tile.

    .. arch:tag:: <input name="string" num_pins="int" equivalent="{none|full}" is_non_clock_global="{true|false}"/>

        Defines an input port.
        Multple input ports are described using multiple ``<input>`` tags.

        :req_param name: Name of the input port.
        :req_param num_pins: Number of pins the input port has.

        :opt_param equivalent:

            Describes if the pins of the port are logically equivalent.
            Input logical equivalence means that the pin order can be swapped without changing functionality.
            For example, an AND gate has logically equivalent inputs because you can swap the order of the inputs and it’s still correct; an adder, on the otherhand, is not logically equivalent because if you swap the MSB with the LSB, the results are completely wrong.
            LUTs are also considered logically equivalent since the logic function (LUT mask) can be rotated to account for pin swapping.

            * ``none``: No input pins are logically equivalent.

                Input pins can not be swapped by the router. (Generates a unique SINK rr-node for each block input port pin.)

            * ``full``: All input pins are considered logically equivalent (e.g. due to logical equivalance or a full-crossbar within the cluster).

                All input pins can be swapped without limitation by the router. (Generates a single SINK rr-node shared by each input port pin.)

            **default:** ``none``

        :opt_param is_non_clock_global:

            .. note:: Applies only to top-level pb_type.

            Describes if this input pin is a global signal that is not a clock.
            Very useful for signals such as FPGA-wide asynchronous resets.
            These signals have their own dedicated routing channels and so should not use the general interconnect fabric on the FPGA.


    .. arch:tag:: <output name="string" num_pins="int" equivalent="{none|full|instance}"/>

        Defines an output port.
        Multple output ports are described using multiple ``<output>`` tags

        :req_param name: Name of the output port.
        :req_param num_pins: Number of pins the output port has.

        :opt_param equivalent:

            Describes if the pins of the output port are logically equivalent:

            * ``none``: No output pins are logically equivalent.

                Output pins can not be swapped by the router. (Generates a unique SRC rr-node for each block output port pin.)

            * ``full``: All output pins are considered logically equivalent.

                All output pins can be swapped without limitation by the router. For example, this option would be appropriate to model an output port which has a full crossbar between it and the logic within the block that drives it. (Generates a single SRC rr-node shared by each output port pin.)

            * ``instance``: Models that sub-instances within a block (e.g. LUTs/BLEs) can be swapped to achieve a limited form of output pin logical equivalence.

                Like ``full``, this generates a single SRC rr-node shared by each output port pin. However, each net originating from this source can use only one output pin from the equivalence group. This can be useful in modeling more complex forms of equivalence in which you can swap which BLE implements which function to gain access to different inputs.

                .. warning:: When using ``instance`` equivalence you must be careful to ensure output swapping would not make the cluster internal routing (previously computed by the clusterer) illegal; the tool does not update the cluster internal routing due to output pin swapping.

            **Default:** ``none``


    .. arch:tag:: <clock name="string" num_pins="int" equivalent="{none|full}"/>

        Describes a clock port.
        Multple clock ports are described using multiple ``<clock>`` tags.
        *See above descriptions on inputs*

    .. arch:tag:: <equivalent_sites>

        .. seealso:: For a step-by-step walkthrough on describing equivalent sites see :ref:`equivalent_sites_tutorial`.

        Describes the Complex Blocks that can be placed within a tile.
        Each physical tile can comprehend a number from 1 to N of possible Complex Blocks, or ``sites``.
        A ``site`` corresponds to a top-level Complex Block that must be placeable in at least 1 physical tile locations.

        .. arch:tag:: <site pb_type="string" pin_mapping="string"/>

        :req_param pb_type: Name of the corresponding pb_type.

        :opt_param pin_mapping: Specifies whether the pin mapping between physical tile and logical pb_type:

                * ``direct``: the pin mapping does not need to be specified as the tile pin definition is equal to the corresponding pb_type one;
                * ``custom``: the pin mapping is user-defined.


                **Default:** ``direct``

            **Example: Equivalent Sites**

            .. code-block:: xml

                <equivalent_sites>
                    <site pb_type="MLAB_SITE" pin_mapping="direct"/>
                </equivalent_sites>

            .. arch:tag:: <direct from="string" to="string">

                Desctibes the mapping of a physical tile's port on the logical block's (pb_type) port.
                ``direct`` is an option sub-tag of ``site``.

                .. note:: This tag is needed only if the pin_mapping of the ``site`` is defined as ``custom``

                Attributes:
                    - ``from`` is relative to the physical tile pins
                    - ``to`` is relative to the logical block pins

                    .. code-block:: xml

                        <direct from="MLAB_TILE.CX" to="MLAB_SITE.BX"/>


    .. arch:tag:: <fc in_type="{frac|abs}" in_val="{int|float}" out_type="{frac|abs}" out_val="{int|float}">

        :req_param in_type:
            Indicates how the :math:`F_c` values for input pins should be interpreted.

            ``frac``: The fraction of tracks of each wire/segment type.

            ``abs``: The absolute number of tracks of each wire/segment type.

        :req_param in_val:
            Fraction or absolute number of tracks to which each input pin is connected.

        :req_param out_type:
            Indicates how the :math:`F_c` values for output pins should be interpreted.

            ``frac``: The fraction of tracks of each wire/segment type.

            ``abs``: The absolute number of tracks of each wire/segment type.

        :req_param out_val:
            Fraction or absolute number of wires/segments to which each output pin connects.


        Sets the number of tracks/wires to which each logic block pin connects in each channel bordering the pin.

        The :math:`F_c` value :cite:`brown_fpgas` is interpreted as applying to each wire/segment type *individually* (see example).

        When generating the FPGA routing architecture VPR will try to make 'good' choices about how pins and wires interconnect; for more details on the criteria and methods used see :cite:`betz_automatic_generation_of_fpga_routing`.


        .. note:: If ``<fc>`` is not specified for a complex block, the architecture's ``<default_fc>`` is used.

        .. note:: For unidirection routing architectures absolute :math:`F_c` values must be a multiple of 2.

        **Example:**

        Consider a routing architecture with 200 length 4 (L4) wires and 50 length 16 (L16) wires per channel, and the following Fc specification:

        .. code-block:: xml

            <fc in_type="frac" in_val="0.1" out_type="abs" out_val="25">

        The above specifies that each:

        * input pin connects to 20 L4 tracks (10% of the 200 L4s) and 5 L16 tracks (10% of the 50 L16s), and

        * output pin connects to 25 L4 tracks and 25 L16 tracks.



        **Overriding Values:**

        .. arch:tag:: <fc_override fc_type="{frac|abs}" fc_val="{int|float}", port_name="{string}" segment_name="{string}">

            Allows :math:`F_c` values to be overriden on a port or wire/segment type basis.

            :req_param fc_type:
                Indicates how the override :math:`F_c` value should be interpreted.

                ``frac``: The fraction of tracks of each wire/segment type.

                ``abs``: The absolute number of tracks of each wire/segment type.

            :req_param fc_val:
                Fraction or absolute number of tracks in a channel.

            :opt_param port_name:
                The name of the port to which this override applies.
                If left unspecified this override applies to all ports.

            :opt_param segment_name:
                The name of the segment (defined under ``<segmentlist>``) to which this override applies.
                If left unspecified this override applies to all segments.

            .. note:: At least one of ``port_name`` or ``segment_name`` must be specified.


            **Port Override Example: Carry Chains**

            If you have complex block pins that do not connect to general interconnect (eg. carry chains), you would use the ``<fc_override>`` tag, within the ``<fc>`` tag, to specify them:

            .. code-block:: xml

                <fc_override fc_type="frac" fc_val="0" port_name="cin"/>
                <fc_override fc_type="frac" fc_val="0" port_name="cout"/>

            Where the attribute ``port_name`` is the name of the pin (``cin`` and ``cout`` in this example).


            **Segment Override Example:**

            It is also possible to specify per ``<segment>`` (i.e. routing wire) overrides:

            .. code-block:: xml

                <fc_override fc_type="frac" fc_val="0.1" segment_name="L4"/>

            Where the above would cause all pins (both inputs and outputs) to use a fractional :math:`F_c` of ``0.1`` when connecting to segments of type ``L4``.

            **Combined Port and Segment Override Example:**

            The ``port_name`` and ``segment_name`` attributes can be used together.
            For example:

            .. code-block:: xml

                <fc_override fc_type="frac" fc_val="0.1" port_name="my_input" segment_name="L4"/>
                <fc_override fc_type="frac" fc_val="0.2" port_name="my_output" segment_name="L4"/>

            specifies that port ``my_input`` use a fractional :math:`F_c` of ``0.1`` when connecting to segments of type ``L4``, while the port ``my_output`` uses a fractional :math:`F_c` of ``0.2`` when connecting to segments of type ``L4``.
            All other port/segment combinations would use the default :math:`F_c` values.

    .. arch:tag:: <pinlocations pattern="{spread|perimeter|custom}">

        :req_param pattern:
            * ``spread`` denotes that the pins are to be spread evenly on all sides of the complex block.

                .. note:: *Includes* internal sides of blocks with width > 1 and/or height > 1.

            * ``perimeter`` denotes that the pins are to be spread evenly on perimeter sides of the complex block.

                .. note:: *Excludes* the internal sides of blocks with width > 1 and/or height > 1.

            * ``spread_inputs_perimeter_outputs`` denotes that inputs pins are to be spread on all sides of the complex block, but output pins are to be spread only on perimeter sides of the block.

                .. note:: This is useful for ensuring outputs do not connect to wires which fly-over a width > 1 and height > 1 block (e.g. if using ``short`` or ``buffer`` connections instead of a fully configurable switch block within the block).

            * ``custom`` allows the architect to specify specifically where the pins are to be placed using ``<loc>`` tags.

        Describes the locations where the input, output, and clock pins are distributed in a complex logic block.

        .. arch:tag:: <loc side="{left|right|bottom|top}" xoffset="int" yoffset="int">name_of_complex_logic_block.port_name[int:int] ... </loc>

            .. note:: ``...`` represents repeat as needed. Do not put ``...`` in the architecture file.

            :req_param side: Specifies which of the four sides of a grid location the pins in the contents are located.

            :opt_param xoffset:
                Specifies the horizontal offset (in grid units) from block origin (bottom left corner).
                The offset value must be less than the width of the block.

                **Default:** ``0``

            :opt_param yoffset:
                Specifies the vertical offset (in grid units) from block origin (bottom left corner).
                The offset value must be less than the height of the block.

                **Default:** ``0``

            If the subtile capacity is greater than 1, you can specify the capacity range when defining the pin locations. For example:

            .. code-block:: xml

                <sub_tile name="io_bottom" capacity="6">
                    <equivalent_sites>
                        <site pb_type="io"/>
                    </equivalent_sites>
                    <input name="outpad" num_pins="1"/>
                    <output name="inpad" num_pins="1"/>
                    <fc in_type="frac" in_val="0.15" out_type="frac" out_val="0.10"/>
                    <pinlocations pattern="custom">
                        <loc side="top">io_bottom[0:1].outpad io_bottom[0:3].inpad io_bottom[2:5].outpad io_bottom[4:5].inpad</loc>
                    </pinlocations>
                </sub_tile>
            
            If no capacity range is specified, it is assumed that the location applies to all capacity instances.

        Physical equivalence for a pin is specified by listing a pin more than once for different locations.
        For example, a LUT whose output can exit from the top and bottom of a block will have its output pin specified twice: once for the top and once for the bottom.

        .. note:: If the ``<pinlocations>`` tag is missing, a ``spread`` pattern is assumed.

.. arch:tag:: <switchblock_locations pattern="{external_full_internal_straight|all|external|internal|none|custom}" internal_switch="string">

    Describes where global routing switchblocks are created in relation to the complex block.

    .. note:: If the ``<switchblock_locations>`` tag is left unspecified the default pattern is assumed.

    :opt_param pattern:

        * ``external_full_internal_straight``: creates *full* switchblocks outside and *straight* switchblocks inside the complex block

        * ``all``: creates switchblocks wherever routing channels cross

        * ``external``: creates switchblocks wherever routing channels cross *outside* the complex block

        * ``internal``: creates switchblocks wherever routing channels cross *inside* the complex block

        * ``none``: denotes that no switchblocks are created for the complex block

        * ``custom``: allows the architect to specify custom switchblock locations and types using ``<sb_loc>`` tags

        **Default:** ``external_full_internal_straight``


    .. _fig_sb_locations:

    .. figure:: sb_locations.*

        Switchblock Location Patterns for a width = 2, height = 3 complex block

    :opt_param internal_switch:

        The name of a switch (from ``<switchlist>``) which should be used for internal switch blocks.

        **Default:** The default switch for the wire ``<segment>``

        .. note:: This is typically used to specify that internal wire segments are electrically shorted together using a ``short`` type ``<switch>``.


    **Example: Electrically Shorted Internal Straight Connections**

    In some architectures there are no switch blocks located 'within' a block, and the wires crossing over the block are instead electrcially shorted to their 'straight-through' connections.

    To model this we first define a special ``short`` type switch to electrically short such segments together:

    .. code-block:: xml

        <switchlist>
            <switch type="short" name="electrical_short" R="0" Cin="0" Tdel="0"/>
        </switchlist>

    Next, we use the pre-defined ``external_full_internal_straight`` pattern, and that such connections should use our ``electrical_short`` switch.

    .. code-block:: xml

        <switchblock_locations pattern="external_full_internal_straight" internal_switch="electrical_short"/>



    .. arch:tag:: <sb_loc type="{full|straight|turns|none}" xoffset="int" yoffset="int", switch_override="string">

        Specifies the type of switchblock to create at a particular location relative to a complex block for the ``custom`` switchblock location pattern.

        :req_param type:
            Specifies the type of switchblock to be created at this location:

            * ``full``: denotes that a full switchblock will be created (i.e. both ``staight`` and ``turns``)
            * ``straight``: denotes that a switchblock with only straight-through connections will be created (i.e. no ``turns``)
            * ``turns``: denotes that a switchblock with only turning connections will be created (i.e. no ``straight``)
            * ``none``: denotes that no switchblock will be created

            **Default:** ``full``

            .. figure:: sb_types.*

                Switchblock Types


        :opt_param xoffset:
            Specifies the horizontal offset (in grid units) from block origin (bottom left corner).
            The offset value must be less than the width of the block.

            **Default:** ``0``

        :opt_param yoffset:
            Specifies the vertical offset (in grid units) from block origin (bottom left corner).
            The offset value must be less than the height of the block.

            **Default:** ``0``

        :opt_param switch_override:
            The name of a switch (from ``<switchlist>``) which should be used to construct the switch block at this location.

            **Default:** The default switch for the wire ``<segment>``

        .. note:: The switchblock associated with a grid tile is located to the top-right of the grid tile


        **Example: Custom Description of Electrically Shorted Internal Straight Connections**

        If we assume a width=2, height=3 block (e.g. :numref:`fig_sb_locations`), we can use a custom pattern to specify an architecture equivalent to the 'Electrically Shorted Internal Straight Connections' example:

        .. code-block:: xml

            <switchblock_locations pattern="custom">
                <!-- Internal: using straight electrical shorts -->
                <sb_loc type="straight" xoffset="0" yoffset="0" switch_override="electrical_short">
                <sb_loc type="straight" xoffset="0" yoffset="1" switch_override="electrical_short">

                <!-- External: using default switches -->
                <sb_loc type="full" xoffset="0" yoffset="2"> <!-- Top edge -->
                <sb_loc type="full" xoffset="1" yoffset="0"> <!-- Right edge -->
                <sb_loc type="full" xoffset="1" yoffset="1"> <!-- Right edge -->
                <sb_loc type="full" xoffset="1" yoffset="2"> <!-- Top Right -->
            <switchblock_locations/>

.. _arch_complex_blocks:

Complex Blocks
--------------

.. seealso:: For a step-by-step walkthrough on building a complex block see :ref:`arch_tutorial`.

The content within the ``<complexblocklist>`` describes the complex blocks found within the FPGA.
Each type of complex block is specified with a top-level ``<pb_type>`` tag within the ``<complexblocklist>`` tag.

PB Type
~~~~~~~
.. arch:tag:: <pb_type name="string" num_pb="int" blif_model="string"/>

    Specifies a top-level complex block, or a complex block's internal components (sub-blocks).
    Which attributes are applicable depends on where the ``<pb_type>`` tag falls within the hierarchy:

    * Top Level: A child of the ``<complexblocklist>``
    * Intermediate: A child of another ``<pb_type>``
    * Primitive/Leaf: Contains no ``<pb_type>`` children

    For example:

    .. code-block:: xml

        <complexblocklist>
            <pb_type name="CLB"/> <!-- Top level -->
                ...
                <pb_type name="ble"/> <!-- Intermediate -->
                    ...
                    <pb_type name="lut"/> <!-- Primitive -->
                        ...
                    </pb_type>
                    <pb_type name="ff"/> <!-- Primitive -->
                        ...
                    </pb_type>
                    ...
                </pb_type>
                ...
            </pb_type>
            ...
        </complexblocklist>

    .. note: Intermediate pb_types can contain other intermediate or primitive pb_types so arbitrary hierarchies can be specified.

    **General:**

    :req_param name: The name of this pb_type.

        The name must be unique with respect to any parent, sibling, or child ``<pb_type>``.


    **Top-level, Intermediate or Primitive:**

    :opt_param num_pb: The number of instances of this pb_type at the current hierarchy level.

        **Default:** ``1``

        For example:

        .. code-block:: xml

            <pb_type name="CLB">
                ...
                <pb_type name="ble" num_pb="10"/>
                   ...
                </pb_type>
                ...
            </pb_type>

        would specify that the pb_type ``CLB`` contains 10 instances of the ``ble`` pb_type.

    **Primitive Only:**

    :req_param blif_model: Specifies the netlist primitive which can be implemented by this pb_type.

        Accepted values:

        * ``.input``: A BLIF netlist input

        * ``.output``: A BLIF netlist output

        * ``.names``: A BLIF .names (LUT) primitive

        * ``.latch``: A BLIF .latch (DFF) primitive

        * ``.subckt <custom_type>``: A user defined black-box primitive.

        For example:

        .. code-block:: xml

            <pb_type name="my_adder" blif_model=".subckt adder"/>
               ...
            </pb_type>

        would specify that the pb_type ``my_adder`` can implement a black-box BLIF primitive named ``adder``.

        .. note:: The input/output/clock ports for primitive pb_types must match the ports specified in the ``<models>`` section.

    :opt_param class: Specifies that this primitive is of a specialized type which should be treated specially.

        .. seealso:: :ref:`arch_classes` for more details.

The following tags are common to all <pb_type> tags:

.. arch:tag:: <input name="string" num_pins="int" equivalent="{none|full}" is_non_clock_global="{true|false}"/>

    Defines an input port.
    Multple input ports are described using multiple ``<input>`` tags.

    :req_param name: Name of the input port.
    :req_param num_pins: Number of pins the input port has.

    :opt_param equivalent:

        .. note:: Applies only to top-level pb_type.

        Describes if the pins of the port are logically equivalent.
        Input logical equivalence means that the pin order can be swapped without changing functionality.
        For example, an AND gate has logically equivalent inputs because you can swap the order of the inputs and it’s still correct; an adder, on the otherhand, is not logically equivalent because if you swap the MSB with the LSB, the results are completely wrong.
        LUTs are also considered logically equivalent since the logic function (LUT mask) can be rotated to account for pin swapping.

        * ``none``: No input pins are logically equivalent.

            Input pins can not be swapped by the router. (Generates a unique SINK rr-node for each block input port pin.)

        * ``full``: All input pins are considered logically equivalent (e.g. due to logical equivalance or a full-crossbar within the cluster).

            All input pins can be swapped without limitation by the router. (Generates a single SINK rr-node shared by each input port pin.)

        **default:** ``none``

    :opt_param is_non_clock_global:

        .. note:: Applies only to top-level pb_type.

        Describes if this input pin is a global signal that is not a clock.
        Very useful for signals such as FPGA-wide asynchronous resets.
        These signals have their own dedicated routing channels and so should not use the general interconnect fabric on the FPGA.


.. arch:tag:: <output name="string" num_pins="int" equivalent="{none|full|instance}"/>

    Defines an output port.
    Multple output ports are described using multiple ``<output>`` tags

    :req_param name: Name of the output port.
    :req_param num_pins: Number of pins the output port has.

    :opt_param equivalent:

        .. note:: Applies only to top-level pb_type.

        Describes if the pins of the output port are logically equivalent:

        * ``none``: No output pins are logically equivalent.

            Output pins can not be swapped by the router. (Generates a unique SRC rr-node for each block output port pin.)

        * ``full``: All output pins are considered logically equivalent.

            All output pins can be swapped without limitation by the router. For example, this option would be appropriate to model an output port which has a full crossbar between it and the logic within the block that drives it. (Generates a single SRC rr-node shared by each output port pin.)

        * ``instance``: Models that sub-instances within a block (e.g. LUTs/BLEs) can be swapped to achieve a limited form of output pin logical equivalence.

            Like ``full``, this generates a single SRC rr-node shared by each output port pin. However, each net originating from this source can use only one output pin from the equivalence group. This can be useful in modeling more complex forms of equivalence in which you can swap which BLE implements which function to gain access to different inputs.

            .. warning:: When using ``instance`` equivalence you must be careful to ensure output swapping would not make the cluster internal routing (previously computed by the clusterer) illegal; the tool does not update the cluster internal routing due to output pin swapping.

        **Default:** ``none``


.. arch:tag:: <clock name="string" num_pins="int" equivalent="{none|full}"/>

    Describes a clock port.
    Multple clock ports are described using multiple ``<clock>`` tags.
    *See above descriptions on inputs*

.. arch:tag:: <mode name="string" disable_packing="bool">

    :req_param name:
        Name for this mode.
        Must be unique compared to other modes.

    Specifies a mode of operation for the ``<pb_type>``.
    Each child mode tag denotes a different mode of operation for the ``<pb_type>``.
    Each mode tag may contains other ``<pb_type>`` and ``<interconnect>`` tags.

    .. note:: Modes within the same parent ``<pb_type>`` are mutually exclusive.

    .. note:: If a ``<pb_type>`` has only one mode of operation the mode tag can be omitted.

    :opt_param disable_packing:
        Specify if a mode is disabled or not for VPR packer.
        When a mode is defined to be disabled for packing (``disable_packing="true"``), packer will not map any logic to the mode.
        This optional syntax aims to help debugging of multi-mode ``<pb_type>`` so that users can spot bugs in their XML definition quickly. 
        By default, it is set to ``false``.

    .. note:: When a mode is specified to be disabled for packing, its child ``<pb_type>`` and the ``<mode>`` of child ``<pb_type>`` will be considered as disabled for packing automatically. There is no need to specify ``disable_packing`` for every ``<mode>`` in the tree of ``<pb_type>``.

    .. warning:: This is a power-user debugging option. See :ref:`multi_mode_logic_block_tutorial` for a detailed how-to-use.

    For example:

    .. code-block:: xml

        <!--A fracturable 6-input LUT-->
        <pb_type name="lut">
            ...
            <mode name="lut6">
                <!--Can be used as a single 6-LUT-->
                <pb_type name="lut6" num_pb="1">
                    ...
                </pb_type>
                ...
            </mode>
            ...
            <mode name="lut5x2">
                <!--Or as two 5-LUTs-->
                <pb_type name="lut5" num_pb="2">
                    ...
                </pb_type>
                ...
            </mode>
        </pb_type>

    specifies the ``lut`` pb_type can be used as either a single 6-input LUT, or as two 5-input LUTs (but not both).

Interconnect
~~~~~~~~~~~~

As mentioned earlier, the mode tag contains ``<pb_type>`` tags and an ``<interconnect>`` tag.
The following describes the tags that are accepted in the ``<interconnect>`` tag.

.. arch:tag:: <complete name="string" input="string" output="string"/>

    :req_param name: Identifier for the interconnect.
    :req_param input: Pins that are inputs to this interconnect.
    :req_param output: Pins that are outputs of this interconnect.

    Describes a fully connected crossbar.
    Any pin in the inputs can connect to any pin at the output.

    **Example:**

    .. code-block:: xml

        <complete input="Top.in" output="Child.in"/>

    .. figure:: complete_example.*

        Complete interconnect example.

.. arch:tag:: <direct name="string" input="string" output="string"/>

    :req_param name: Identifier for the interconnect.
    :req_param input: Pins that are inputs to this interconnect.
    :req_param output: Pins that are outputs of this interconnect.

    Describes a 1-to-1 mapping between input pins and output pins.

    **Example:**

    .. code-block:: xml

        <direct input="Top.in[2:1]" output="Child[1].in"/>

    .. figure:: direct_example.*

        Direct interconnect example.

.. arch:tag:: <mux name="string" input="string" output="string"/>

    :req_param name: Identifier for the interconnect.
    :req_param input: Pins that are inputs to this interconnect. Different data lines are separated by a space.
    :req_param output: Pins that are outputs of this interconnect.

    Describes a bus-based multiplexer.

    .. note:: Buses are not yet supported so all muxes must use one bit wide data only!

    **Example:**

    .. code-block:: xml

        <mux input="Top.A Top.B" output="Child.in"/>

    .. figure:: mux_example.*

        Mux interconnect example.



A ``<complete>``, ``<direct>``, or ``<mux>`` tag may take an additional, optional, tag called ``<pack_pattern>`` that is used to describe *molecules*.
A pack pattern is a power user feature directing that the CAD tool should group certain netlist atoms (eg. LUTs, FFs, carry chains) together during the CAD flow.
This allows the architect to help the CAD tool recognize structures that have limited flexibility so that netlist atoms that fit those structures be kept together as though they are one unit.
This tag impacts the CAD tool only, there is no architectural impact from defining molecules.

.. arch:tag:: <pack_pattern name="string" in_port="string" out_port="string"/>

    .. warning:: This is a power user option. Unless you know why you need it, you probably shouldn't specify it.

    :req_param name: The name of the pattern.
    :req_param in_port: The input pins of the edges for this pattern.
    :req_param out_port: Which output pins of the edges for this pattern.

    This tag gives a hint to the CAD tool that certain architectural structures should stay together during packing.
    The tag labels interconnect edges with a pack pattern name.
    All primitives connected by the same pack pattern name becomes a single pack pattern.
    Any group of atoms in the user netlist that matches a pack pattern are grouped together by VPR to form a molecule.
    Molecules are kept together as one unit in VPR.
    This is useful because it allows the architect to help the CAD tool assign atoms to complex logic blocks that have interconnect with very limited flexibility.
    Examples of architectural structures where pack patterns are appropriate include: optionally registered inputs/outputs, carry chains, multiply-add blocks, etc.

    There is a priority order when VPR groups molecules.
    Pack patterns with more primitives take priority over pack patterns with less primitives.
    In the event that the number of primitives is the same, the pack pattern with less inputs takes priority over pack patterns with more inputs.

    **Special Case:**

    To specify carry chains, we use a special case of a pack pattern.
    If a pack pattern has exactly one connection to a logic block input pin and exactly one connection to a logic block output pin, then that pack pattern takes on special properties.
    The prepacker will assume that this pack pattern represents a structure that spans multiple logic blocks using the logic block input/output pins as connection points.
    For example, lets assume that a logic block has two, 1-bit adders with a carry chain that links adjacent logic blocks.
    The architect would specify those two adders as a pack pattern with links to the logic block cin and cout pins.
    Lets assume the netlist has a group of 1-bit adder atoms chained together to form a 5-bit adder.
    VPR will break that 5-bit adder into 3 molecules: two 2-bit adders and one 1-bit adder connected in order by a the carry links.

    **Example:**

    Consider a classic basic logic element (BLE) that consists of a LUT with an optionally registered flip-flop.
    If a LUT is followed by a flip-flop in the netlist, the architect would want the flip-flop to be packed with the LUT in the same BLE in VPR.
    To give VPR a hint that these blocks should be connected together, the architect would label the interconnect connecting the LUT and flip-flop pair as a pack_pattern:

    .. code-block:: xml

        <pack_pattern name="ble" in_port="lut.out" out_port="ff.D"/>

    .. figure:: pack_pattern_example.*

        Pack Pattern Example.

.. _arch_classes:

Classes
~~~~~~~
Using these structures, we believe that one can describe any digital complex logic block.
However, we believe that certain kinds of logic structures are common enough in FPGAs that special shortcuts should be available to make their specification easier.
These logic structures are: flip-flops, LUTs, and memories.
These structures are described using a ``class=string`` attribute in the ``<pb_type>`` primitive.
The classes we offer are:

.. arch:tag:: class="lut"

    Describes a K-input lookup table.

    The unique characteristic of a lookup table is that all inputs to the lookup table are logically equivalent.
    When this class is used, the input port must have a ``port_class="lut_in"`` attribute and the output port must have a ``port_class="lut_out"`` attribute.

.. arch:tag:: class="flipflop"

    Describes a flipflop.

    Input port must have a ``port_class="D"`` attribute added.
    Output port must have a ``port_class="Q"`` attribute added.
    Clock port must have a ``port_class="clock"`` attribute added.

.. arch:tag:: class="memory"

    Describes a memory.

    Memories are unique in that a single memory physical primitive can hold multiple, smaller, logical memories as long as:

    #. The address, clock, and control inputs are identical and
    #. There exists sufficient physical data pins to satisfy the netlist memories when the different netlist memories are merged together into one physical memory.

    Different types of memories require different attributes.

    **Single Port Memories Require:**

    * An input port with ``port_class="address"`` attribute
    * An input port with ``port_class="data_in"`` attribute
    * An input port with ``port_class="write_en"`` attribute
    * An output port with ``port_class="data_out"`` attribute
    * A clock port with ``port_class="clock"`` attribute


    **Dual Port Memories Require:**

    * An input port with ``port_class="address1"`` attribute
    * An input port with ``port_class="data_in1"`` attribute
    * An input port with ``port_class="write_en1"`` attribute
    * An input port with ``port_class="address2"`` attribute
    * An input port with ``port_class="data_in2"`` attribute
    * An input port with ``port_class="write_en2"`` attribute
    * An output port with ``port_class="data_out1"`` attribute
    * An output port with ``port_class="data_out2"`` attribute
    * A clock port with ``port_class="clock"`` attribute


Timing
~~~~~~

.. seealso:: For examples of primitive timing modeling specifications see the :ref:`arch_model_timing_tutorial`

Timing is specified through tags contained with in ``pb_type``, ``complete``, ``direct``, or ``mux`` tags as follows:

.. arch:tag:: <delay_constant max="float" min="float" in_port="string" out_port="string"/>

    :opt_param max: The maximum delay value.
    :opt_param min: The minimum delay value.
    :req_param in_port: The input port name.
    :req_param out_port: The output port name.

    Specifies a maximum and/or minimum delay from in_port to out_port.

    * If ``in_port`` and ``out_port`` are non-sequential (i.e combinational) inputs specifies the combinational path delay between them.
    * If ``in_port`` and ``out_port`` are sequential (i.e. have ``T_setup`` and/or ``T_clock_to_Q`` tags) specifies the combinational delay between the primitive's input and/or output registers.

    .. note:: At least one of the ``max`` or ``min`` attributes must be specified

    .. note:: If only one of ``max`` or ``min`` are specified the unspecified value is implicitly set to the same value

.. arch:tag:: <delay_matrix type="{max | min}" in_port="string" out_port="string"> matrix </delay>

    :req_param type: Specifies the delay type.
    :req_param in_port: The input port name.
    :req_param out_port: The output port name.
    :req_param matrix: The delay matrix.

    Describe a timing matrix for all edges going from ``in_port`` to ``out_port``.
    Number of rows of matrix should equal the number of inputs, number of columns should equal the number of outputs.

    * If ``in_port`` and ``out_port`` are non-sequential (i.e combinational) inputs specifies the combinational path delay between them.
    * If ``in_port`` and ``out_port`` are sequential (i.e. have ``T_setup`` and/or ``T_clock_to_Q`` tags) specifies the combinational delay between the primitive's input and/or output registers.

    **Example:**
    The following defines a delay matrix for a 4-bit input port ``in``, and 3-bit output port ``out``:

    .. code-block:: xml

        <delay_matrix type="max" in_port="in" out_port="out">
            1.2e-10 1.4e-10 3.2e-10
            4.6e-10 1.9e-10 2.2e-10
            4.5e-10 6.7e-10 3.5e-10
            7.1e-10 2.9e-10 8.7e-10
        </delay>

    .. note:: To specify both ``max`` and ``min`` delays two ``<delay_matrix>`` should be used.

.. arch:tag:: <T_setup value="float" port="string" clock="string"/>

    :req_param value: The setup time value.
    :req_param port: The port name the setup constraint applies to.
    :req_param clock: The port name of the clock the setup constraint is specified relative to.

    Specifies a port's setup constraint.

    * If ``port`` is an input specifies the external setup time of the primitive's input register (i.e. for paths terminating at the input register).
    * If ``port`` is an output specifies the internal setup time of the primitive's output register (i.e. for paths terminating at the output register) .

    .. note:: Applies only to primitive ``<pb_type>`` tags

.. arch:tag:: <T_hold value="float" port="string" clock="string"/>

    :req_param value: The hold time value.
    :req_param port: The port name the setup constraint applies to.
    :req_param clock: The port name of the clock the setup constraint is specified relative to.

    Specifies a port's hold constraint.

    * If ``port`` is an input specifies the external hold time of the primitive's input register (i.e. for paths terminating at the input register).
    * If ``port`` is an output specifies the internal hold time of the primitive's output register (i.e. for paths terminating at the output register) .

    .. note:: Applies only to primitive ``<pb_type>`` tags

.. arch:tag:: <T_clock_to_Q max="float" min="float" port="string" clock="string"/>

    :opt_param max: The maximum clock-to-Q delay value.
    :opt_param min: The minimum clock-to-Q delay value.
    :req_param port: The port name the delay value applies to.
    :req_param clock: The port name of the clock the clock-to-Q delay is specified relative to.

    Specifies a port's clock-to-Q delay.

    * If ``port`` is an input specifies the internal clock-to-Q delay of the primitive's input register (i.e. for paths starting at the input register).
    * If ``port`` is an output specifies the external clock-to-Q delay of the primitive's output register (i.e. for paths starting at the output register) .

    .. note:: At least one of the ``max`` or ``min`` attributes must be specified

    .. note:: If only one of ``max`` or ``min`` are specified the unspecified value is implicitly set to the same value

    .. note:: Applies only to primitive ``<pb_type>`` tags


Modeling Sequential Primitive Internal Timing Paths
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. seealso:: For examples of primitive timing modeling specifications see the :ref:`arch_model_timing_tutorial`

By default, if only ``<T_setup>`` and ``<T_clock_to_Q>`` are specified on a primitive ``pb_type`` no internal timing paths are modeled.
However, such paths can be modeled by using ``<delay_constant>`` and/or ``<delay_matrix>`` can be used in conjunction with ``<T_setup>`` and ``<T_clock_to_Q>``.
This is useful for modeling the speed-limiting path of an FPGA hard block like a RAM or DSP.

As an example, consider a sequential black-box primitive named ``seq_foo`` which has an input port ``in``, output port ``out``, and clock ``clk``:

.. code-block:: xml

    <pb_type name="seq_foo" blif_model=".subckt seq_foo" num_pb="1">
        <input name="in" num_pins="4"/>
        <output name="out" num_pins="1"/>
        <clock name="clk" num_pins="1"/>

        <!-- external -->
        <T_setup value="80e-12" port="seq_foo.in" clock="clk"/>
        <T_clock_to_Q max="20e-12" port="seq_foo.out" clock="clk"/>

        <!-- internal -->
        <T_clock_to_Q max="10e-12" port="seq_foo.in" clock="clk"/>
        <delay_constant max="0.9e-9" in_port="seq_foo.in" out_port="seq_foo.out"/>
        <T_setup value="90e-12" port="seq_foo.out" clock="clk"/>
    </pb_type>

To model an internal critical path delay, we specify the internal clock-to-Q delay of the input register (10ps), the internal combinational delay (0.9ns) and the output register's setup time (90ps). The sum of these delays corresponds to a 1ns critical path delay.

.. note:: Primitive timing paths with only one stage of registers can be modeled by specifying ``<T_setup>`` and ``<T_clock_to_Q>`` on only one of the ports.

Power
~~~~~

.. seealso:: :ref:`power_estimation`, for the complete list of options, their descriptions, and required sub-fields.

.. arch:tag:: <power method="string">contents</power>

    :opt_param method:

        Indicates the method of power estimation used for the given pb_type.

        Must be one of:

            * ``specify-size``
            * ``auto-size``
            * ``pin-toggle``
            * ``C-internal``
            * ``absolute``
            * ``ignore``
            * ``sum-of-children``

        **Default:** ``auto-size``.

        .. seealso:: :ref:`Power Architecture Modelling <power_arch_modeling>` for a detailed description of the various power estimation methods.

    The ``contents`` of the tag can consist of the following tags:

      * ``<dynamic_power>``
      * ``<static_power>``
      * ``<pin>``


.. arch:tag:: <dynamic_power power_per_instance="float" C_internal="float"/>

    :opt_param power_per_instance: Absolute power in Watts.
    :opt_param C_internal: Block capacitance in Farads.

.. arch:tag:: <static_power power_per_instance="float"/>

    :opt_param power_per_instance: Absolute power in Watts.

.. arch:tag:: <port name="string" energy_per_toggle="float" scaled_by_static_prob="string" scaled_by_static_prob_n="string"/>

    :req_param name: Name of the port.
    :req_param energy_per_toggle: Energy consumed by a toggle on the port specified in ``name``.
    :opt_param scaled_by_static_prob: Port name by which to scale ``energy_per_toggle`` based on its logic high probability.
    :opt_param scaled_by_static_prob_n: Port name by which to scale ``energy_per_toggle`` based on its logic low probability.

NoC Description
---------------

The ``<noc>`` tag is an optional tag and its contents allows designers to describe a NoC on an FPGA device.
The ``<noc>`` tag is the top level tag for the NoC description and its attributes define the overall properties
of the NoC; refer below for its contents.

.. arch:tag:: <noc link_bandwidth="float" link_latency="float" router_latency="float" noc_router_tile_name="string">

    :req_param link_bandwidth:
        Specifies the maximum bandwidth in bits-per-second (bps) that a link in the NoC can support

    :req_param link_latency:
        Specifies the delay in seconds seen by a flit as it travels from one physical NoC router to another using a NoC link.

    :req_param router_latency:
        Specifies the un-loaded delays in seconds as it travels through a physical router.
    
    :req_param noc_router_tile_name:
        Specifies a string which represents the name used to identify a NoC router tile (physical hard block) in the 
        corresponding FPGA architecture. This information is needed to create a model of the NoC.

The ``<noc>`` tag contains a single ``<topology>`` tag which describes the topology of the NoC.

NoC topology
~~~~~~~~~~~~

As mentioned above the ``<topology>`` tag can be used to specify the topology or how the routers in the NoC
are connected to each other. The ``<topology>`` tag consists of a group ``<router>``tags.

Below is an example of how the ``<topology>`` tag is used.

.. code-block:: xml

    <topology>
        <!--A number of <router> tags go here-->
    </topology>

The ``<router>`` tag and its contents are described below.

.. arch:tag:: <router id="int" positionx="float" positiony="float" connections="int int int int ...">

    This tag represents a single physical NoC router on the FPGA device and specifies how it is connected within the NoC.

    :req_param  id:
        Specifies a user identification (ID) number which is associate to the physical
        router that this tag is identifying. This ID is used to report errors and
        warnings to the user.
    
    :req_param positionx:
        Specifies the horizontal position of the physical router block that this
        tag is identifying. This position does not have to be exact, it can
        be an approximate value.

    :req_param positiony:
        Specifies the vertical position of the physical router block that this
        tag is identifying. This position does not have to be exact, it can
        be an approximate value.

    :req_param connections:
        Specifies a list of numbers seperated by spaces which are the user IDs supplied to other
        ``<router>`` tags. This describes how the current physical Noc router
        that this tag is identifying is connected to the other physical NoC routers on the device.
    
    Below is an example of the ``<router>`` tag which identifies a physical router located near (0,0) with ID 0. This router
    is also connected to two other routers identified by IDs 1 and 2.

    .. code-block:: xml

        <router id="0" positionx="0" positiony="0" connections="1 2"/>

NoC Description Example
~~~~~~~~~~~~~~~~~~~~~~~

Below is an example which describes a NoC architecture which has 4 physical routers that are connected to each other to form a 
2x2 mesh topology.

.. code-block:: xml
    
    <!-- Description of a 2x2 mesh NoC-->
    <noc link_bandwidth="1.2e9" router_latency="1e-9" link_latency="1e-9" noc_router_tile_name="noc_router_adapter">
        <topology>
                <router id="0" positionx="0" positiony="0" connections="1 2"/>
                <router id="1" positionx="5" positiony="0" connections="0 3"/>
                <router id="2" positionx="0" positiony="5" connections="0 3"/>
                <router id="3" positionx="5" positiony="5" connections="1 2"/>
        </topology>
    </noc>

Wire Segments
-------------

The content within the ``<segmentlist>`` tag consists of a group of ``<segment>`` tags.
The ``<segment>`` tag and its contents are described below.

.. arch:tag:: <segment axis="{x|y}" name="unique_name" length="int" type="{bidir|unidir}" res_type="{GCLK|GENERAL}" freq="float" Rmetal="float" Cmetal="float">content</segment>


    :opt_param axis:
        Specifies if the given segment applies to either x or y channels only. If this tag is not given, it is assumed that the given segment
        description applies to both x-directed and y-directed channels.

        .. note:: It is required that both x and y segment axis details are given or that at least one segment within ``segmentlist`` 
            is specified without the ``axis`` tag (i.e. at least one segment applies to both x-directed and y-directed 
            chanels). 

    :req_param name:
        A unique alphanumeric name to identify this segment type.

    :req_param length:
        Either the number of logic blocks spanned by each segment, or the keyword ``longline``.
        Longline means segments of this type span the entire FPGA array.

        .. note:: ``longline`` is only supported on with ``bidir`` routing

    :opt_param res_type:
        Specifies whether the segment belongs to the general or a clock routing network. If this tag is not specified, the resource type for
        the segment is considered to be GENERAL (i.e. regular routing).
  
    :req_param freq:
        The supply of routing tracks composed of this type of segment.
        VPR automatically determines the percentage of tracks for each segment type by taking the frequency for the type specified and dividing with the sum of all frequencies.
        It is recommended that the sum of all segment frequencies be in the range 1 to 100.

    :req_param Rmetal:
        Resistance per unit length (in terms of logic blocks) of this wiring track, in Ohms.
        For example, a segment of length 5 with Rmetal = 10 Ohms / logic block would have an end-to-end resistance of 50 Ohms.

    :req_param Cmetal:
        Capacitance per unit length (in terms of logic blocks) of this wiring track, in Farads.
        For example, a segment of length 5 with Cmetal = 2e-14 F / logic block would have a total metal capacitance of 10e-13F.

    :req_param directionality:
        This is either unidirectional or bidirectional and indicates whether a segment has multiple drive points (bidirectional), or a single driver at one end of the wire segment (unidirectional).
        All segments must have the same directionality value.
        See :cite:`lemieux_directional_and_singale_driver_wires` for a description of unidirectional single-driver wire segments.

    :req_param content:
        The switch names and the depopulation pattern as described below.

.. _fig_sb_pattern:

.. figure:: sb_pattern.*

    Switch block and connection block pattern example with four tracks per channel

.. arch:tag:: <sb type="pattern">int list</sb>

    This tag describes the switch block depopulation (as illustrated in :numref:`fig_sb_pattern`) for this particular wire segment.
    For example, the first length 6 wire in the figure below has an sb pattern of ``1 0 1 0 1 0 1``.
    The second wire has a pattern of ``0 1 0 1 0 1 0``.
    A ``1`` indicates the existence of a switch block and a ``0`` indicates that there is no switch box at that point.
    Note that there a 7 entries in the integer list for a length 6 wire.
    For a length L wire there must be L+1 entries separated by spaces.

    .. note:: Can not be specified for ``longline`` segments (which assume full switch block population)

.. arch:tag:: <cb type="pattern">int list</cb>

    This tag describes the connection block depopulation (as illustrated by the circles in :numref:`fig_sb_pattern`) for this particular wire segment.
    For example, the first length 6 wire in the figure below has an sb pattern of ``1 1 1 1 1 1``.
    The third wire has a pattern of ``1 0 0 1 1 0``.
    A ``1`` indicates the existence of a connection block and a ``0`` indicates that there is no connection box at that point.
    Note that there a 6 entries in the integer list for a length 6 wire.
    For a length L wire there must be L entries separated by spaces.

    .. note:: Can not be specified for ``longline`` segments (which assume full connection block population)

.. arch:tag:: <mux name="string"/>

    :req_param name: Name of the mux switch type used to drive this type of segment by default, from both block outputs and other wires. This information is used during rr-graph construction, and a custom switch block can override this switch type for specific connections if desired.
                     The switch type specified with the <mux> tag will be used for both the incrementing and decrementing wires within this segment. 
                     If more control is needed, the mux_inc and mux_dec tags can be used to assign different muxes to drive incremental and decremental wires within the segment.

    .. note:: For UNIDIRECTIONAL only.

    Tag must be included and ``name`` must be the same as the name you give in ``<switch type="mux" name="...``

.. arch:tag:: <mux_inc name="string"/>

    :req_param name: 
        Name of the mux switch type used to drive the incremental wires in this segment from both block outputs and other wires.
        Incremental wires are tracks within this segment that are heading in the "right" direction on the x-axis and the "top" direction on the y-axis.
        This information is used during rr-graph construction, and a custom switch block can override this switch type for specific connections if desired.

    .. note:: For UNIDIRECTIONAL only.

.. arch:tag:: <mux_dec name="string">
    
    :req_param name: 
        Name of the mux switch type used to drive the decremental wires in this segment from both block outputs and other wires.
        Incremental wires are tracks within this segment that are heading in the "left" direction on the x-axis and the "bottom" direction on the y-axis.
        This information is used during rr-graph construction, and a custom switch block can override this switch type for specific connections if desired.

    .. note:: For UNIDIRECTIONAL only.

    .. note:: For unidirectional segments, either <mux> tag or both <mux_inc> and <mux_dec> should be defined in the architecture file. If only the <mux> tag is defined, we assume that the same mux drives both incremental and decremental wires within this segment.     

.. arch:tag:: <mux_inter_die name="string"/>

    :req_param name: Name of the mux switch type used to drive this segment type when the driver (block outputs and other wires) is located on a different die than the segment. This information is utilized during rr-graph construction.

    Tag must be included and ``name`` must be the same as the name you give in ``<switch type="mux" name="...``


.. arch:tag:: <wire_switch name="string"/>

    :req_param name: Name of the switch type used by other wires to drive this type of segment by default. This information is used during rr-graph construction, and a custom switch block can override this switch type for specific connections if desired.

    .. note:: For BIDIRECTIONAL only.

    Tag must be included and the name must be the same as the name you give in ``<switch type="tristate|pass_gate" name="...`` for the switch which represents the wire switch in your architecture.

.. arch:tag:: <opin_switch name="string"/>

    .. note:: For BIDIRECTIONAL only.

    :req_param name: Name of the switch type used by block pins to drive this type of segment.

    Tag must be included and ``name`` must be the same as the name you give in ``<switch type="tristate|pass_gate" name="...`` for the switch which represents the output pin switch in your architecture.

    .. note::

        In unidirectional segment mode, there is only a single buffer on the segment.
        Its type is specified by assigning the same switch index to both wire_switch and opin_switch.
        VPR will error out if these two are not the same.

    .. note::

        The switch used in unidirectional segment mode must be buffered.

.. _clock_architecture:

Clocks
------

There are two options for describing clocks.
One method allows you to specify clocking purely for power estimation, see :ref:`clock_power_format`.
The other method allows you to specify a clock architecture that is used as part of the routing resources, see :ref:`clock_architecture_format`.
Both methods should not be used in tandem.

.. _clock_power_format:

Specifing Clocking Purely for Power Estimation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The clocking configuration is specified with ``<clock>`` tags within the ``<clocks>`` section.

.. note:: Currently the information in the ``<clocks>`` section is only used for power estimation.

.. seealso:: :ref:`power_estimation` for more details.

.. arch:tag:: <clock C_wire="float" C_wire_per_m="float" buffer_size={"float"|"auto"}/>

    :opt_param C_wire: The absolute capacitance, in Farads, of the wire between each clock buffer.
    :opt_param C_wire_per_m: The wire capacitance, in Farads per Meter.
    :opt_param buffer_size: The size of each clock buffer.

.. _clock_architecture_format:

Specifing a Clock Architecture
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The element ``<clocknetworks>`` contains three sub-elements that collectively describe the clock architecture: the wiring parameters ``<metal_layers>``, the clock distribution ``<clock_network>``, and the clock connectivity ``<clock_routing>``.

    .. note:: The clock network architecture defined in this structure is assigned the fixed default name ``"clock_network"``. When the user wants to specify a net to be routed through the defined clock architecture using a :ref:`global routing constraints file <global_routing_constraints>`, the network name attribute in the constraint tag must be set to ``"clock_network"``.

.. _clock_arch_example:

Clock Architecture Example
^^^^^^^^^^^^^^^^^^^^^^^^^^

The following example shows how a rib-spine (row/column) style clock architecture can be defined.

.. code-block:: xml

    <clocknetworks>
        <metal_layers >
            <metal_layer name="global_spine" Rmetal="50.42" Cmetal="20.7e-15"/>
            <metal_layer name="global_rib" Rmetal="50.42" Cmetal="20.7e-15"/>
        </metal_layers >

        <!-- Full Device: Center Spine -->
        <clock_network  name="spine1" num_inst="2">
            <spine metal_layer="global_spine" x="W/2" starty="0" endy="H">
                <switch_point type="drive" name="drive_point" yoffset="H/2" buffer="drive_buff"/>
                <switch_point type="tap" name="taps" yoffset="0" yincr="1"/>
            </spine>
        </clock_network>

        <!-- Full Device: Each Grid Row -->
        <clock_network  name="rib1" num_inst="2">
            <rib metal_layer="global_rib" y="0" startx="0" endx="W" repeatx="W" repeaty="1">
                <switch_point type="drive" name="drive_point" xoffset="W/2" buffer="drive_buff"/>
                <switch_point type="tap" name="taps" xoffset="0" xincr="1"/>
            </rib>
        </clock_network>

        <clock_routing>
            <!-- connections from inter-block routing to central spine -->
            <tap from="ROUTING" to="spine1.drive_point" locationx="W/2" locationy="H/2" switch="general_routing_switch" fc_val="1.0"/>

            <!-- connections from spine to rib -->
            <tap from="spine1.taps" to="rib1.drive_point" switch="general_routing_switch" fc_val="0.5"/>

            <!-- connections from rib to clock pins -->
            <tap from="rib1.taps" to="CLOCK" switch="ipin_cblock" fc_val="1.0"/>
        </clock_routing >
    </clocknetworks >

.. _spine_visual:

.. figure:: vertical_distribution.*

    ``<spine>`` "spine1" vertical clock wire example.
    The two spines (``num_inst="2"``) are located horizontally at ``W/2`` (in the middle of the device), and spans the entire height of the device (0..H).
    The drive points are located at ``H/2``, with tap points located at unit increments along their length.
    Buffers of  ``drive_buff`` type (would be defined in ``<switches>``) are used to drive the two halves of the spines.

.. _rib_visual:

.. figure:: horizontal_distribution.*

    ``<rib>`` "rib1" horizontal clock wire example.
    Each rib spans the full width of the device (0..W), with the drive points located at the mid-point (``W/2``), and tap points in unit increments along each rib.
    There are two ribs at each vertical location (``num_inst="2"``), and pairs of ribs are stamped out at each row of the device (``repeaty="1"``).

.. _clock_architecture_tags:

Clock Architecture Tags
^^^^^^^^^^^^^^^^^^^^^^^

The ``<metal_layers>`` element describes the per unit length electrical parameters, resistance (``Rmetal``) and capacitance (``Cmetel``), used to implement the clock distribution wires.
Wires are modeled soley based on ``Rmetal`` and ``Cmetal`` parameters which are derived from the physical implementation of the metal layer width and spacing.
There can be one or more wiring implementation options (metal layer, width and spacing) that are used by the later clock network specification and each is described in a separate ``<metal_layer>`` sub-element.
The syntax of the wiring electrical information is:

.. arch:tag:: <metal_layer name="string" Rmetal="float" Cmetal="float"/>

    :req_param name: A unique string for reference.
    :req_param Rmetal: The resistance in Ohms of the wire per unit block in the FPGA architecture; a unit block usually corresponds to a logic cluster.
    :req_pram Cmetal: The capacitance in Farads of the wire per unit block.

The ``<clock_network>`` element contains sub-elements that describe the clock distribution wires for the clock architecture.
There could be more than one ``<clock_network>`` element to describe separate types of distribution wires.
The high-level start tag for a clock network is as follows:

.. arch:tag:: <clock_network name="string" num_inst="integer">

    :req_param name: A unique string for reference.
    :req_param num_inst: which describes the number of parallel instances of the clock distribution types described in the ``<clock_network>`` sub-elements.

    .. note::
        Many paramters used in the following clock architecture tags take an espression (``expr``) as an argument simular to :ref:`grid_expressions`.
        However, only a subset of special variables are suported: ``W`` (device width) and ``H`` (device height).

    The supported clock distribution types are ``<spine>`` and ``<rib>``.
    *Spines* are used to describe vertical clock distribution wires.
    Whereas, *Ribs* is used to describe a horizontal clock distribution wire.
    See :ref:`clock_arch_example` and accompanying figures :numref:`spine_visual` and :numref:`rib_visual` for example use of ``<spine>`` and ``<rib>`` parameters.

    .. arch:tag:: <spine metal_layer="string" x="expr" starty="expr" endy="expr" repeatx="expr" repeaty="expr"/>

        :req_param metal_layer: A referenced metal layer that sets the unit resistance and capacitance of the distribution wire over the length of the wire.
        :req_param starty: 
            The start y grid location, of the wire which runs parallel to the y-axis from starty and ends at endy, inclusive.
            Value can be relative to the device size.
        :req_param endy: The end of y grid location of the wire. Value can be relative to the device size.
        :req_param x: The location of the spine with respect to the x-axis. Value can be relative to the device size.
        :opt_param repeatx: The horizontal repeat factor of the spine along the device. Value can be relative to the device size.
        :opt_param repeaty: The vertical repeat factor of the spine along the device. Value can be relative to the device size.

    The provided example clock network (:ref:`clock_arch_example`) defines two spines, and neither repeats as each spans the entire height of the device and is locally at the horizontal midpoint of the device.

    .. arch:tag:: <rib metal_layer="string" y="expr" startx="expr" endx="expr" repeatx="expr" repeaty="expr"/>

        :req_param metal_layer: A referenced metal layer that sets the unit resistance and capacitance of the distribution wire over the length of the wire.
        :req_param startx:
            The start x grid location, of the wire which runs parallel to the x-axis from startx and ends at endx, inclusive.
            Value can be relative to the device size.
        :req_param endx: The end of x grid location of the wire. Value can be relative to the device size.
        :req_param y: The location of the rib with respect to the y-axis. Value can be relative to the device size.
        :opt_param repeatx: The horizontal repeat factor of the rib along the device. Value can be relative to the device size.
        :opt_param repeaty: The vertical repeat factor of the rib along the device. Value can be relative to the device size.

    Along each spine and rib is a group of switch points.
    Switch points are used to describe drive or tap locations along the clock distribution wire, and are enclosed in the relevant ``<rib>`` or ``<spine>`` tags:

    .. arch:tag:: <switch_point type="{drive | tap}" name="string" yoffset="expr" xoffset="expr" xinc="expr" yinc="expr" buffer="string">

        :req_param type:
            * ``drive`` -- Drive points are where the clock distribution wire can be driven by a routing switch or buffer.
            * ``tap`` --  Tap points are where it can drive a routing switch or buffer to send a signal to a different ``clock_network`` or logicblock.
        :opt_param xoffset: (Only for ``rib`` network) Offset from the ``startx`` of a rib network.
        :opt_param yoffset: (Only for ``spine`` network) Offset from the ``starty`` of a spine network.
        :opt_param xinc: (Only for rib ``tap`` points) Descibes the repeat factor of a series of evenly spaced tap points.
        :opt_param yinc: (Only for spine ``tap`` points) Descibes the repeat factor of a series of evenly spaced tap points.
        :req_param buffer:
            (Required only for ``drive`` points) A reference to a pre-defined routing switch; specfied by ``<switch>`` tag, see Section :ref:`arch_switches`.
            This switch will be used at the drive point.
            The clock architecture generator uses two of these buffers to drive the two portions of this ``clock_network`` wire when it is split at the drive point, see Figures :numref:`rib_visual` and :numref:`spine_visual`.

        .. note::

            A single ``<switch_point>`` specification may define a *set* of tap points (``type="tap"``, with either ``xincr`` or ``yincr``), or a single drive point (``type="drive"``)

Lastly the ``<clock_routing>`` element consists of a group of ``tap`` statements which separately describe the connectivity between clock-related routing resources (pin or wire).
The tap element and its attribute sare as follows:

.. arch:tag:: <tap from="string" to="string" locationx="expr" locationy="expr" switch="string" fc_val="float">

    :req_param from:
        The set of routing resources to make connections *from*.
        This can be either:
            * ``clock_name.tap_points_name``: A set of clock network ``tap``-type switchpoints. The format is clock network name, followed by the tap points name and delineated by a period (e.g. ``spine1.taps``), or
            * ``ROUTING``: a special literal which references a connection from general inter-block routing (at a location specified by ``locationx`` and ``locationy`` parameters).
        Examples can be see in :ref:`clock_arch_example`.
    :req_param to:
        The set of routing resources to make connections *to*.
        Can be a unique name or special literal:
            * ``clock_name.drive_point_name``: A clock network ``drive``-type switchpoint. The format is clock network name, followed by the drive point name and delineated by a period (e.g. ``rib1.drive_point``).
            * ``CLOCK``: a special literal which describes connections from clock network tap points that supply the clock to clock pins on blocks at the tap locations; these are clock inputs are already specified on blocks (top-level ``<pb_type>``/``<tile>``) in the VTR architecture file.
        Examples can be see in :ref:`clock_arch_example`.
    :req_param switch: The routing switch (defined in ``<switches>``) used for this connection.
    :req_param fc_val:
        A decimal value between 0 and 1 representing the connection block flexibility between the connecting routing resources; a value of 0.5 for example means that only 50% of the switches necessary to connect all the matching tap and drive points would be implemented.
    :opt_param locationx:
        (Required when using the special literal ``"ROUTING"``)
        The x grid location of inter-block routing.
    :opt_param locationy:
        (Required when using the special literal ``"ROUTING"``)
        The y grid location of inter-block routing.

    .. note:: 
    
        A single ``<tap>`` statement may create multiple connections if either the of the ``from`` or ``to`` correspond to multiple routing resources.
        In such cases the ``fc_val`` can control how many connections are created.

    .. note::

        ``locationx`` and ``locationy`` describe an (x,y) grid loction where all the wires passing this location source the source the clock network connection depending on the ``fc_val``

For more information you may wish to consult :cite:`mustafa_masc` which introduces the clock modeling language.

Power
-----
Additional power options are specified within the ``<architecture>`` level ``<power>`` section.

.. seealso:: See :ref:`power_estimation` for full documentation on how to perform power estimation.

.. arch:tag:: <local_interconnect C_wire="float" factor="float"/>

    :req_param C_wire: The local interconnect capacitance in Farads/Meter.
    :opt_param factor: The local interconnect scaling factor. **Default:** ``0.5``.

.. arch:tag:: <buffers logical_effort_factor="float"/>

    :req_param logical_effort_factor: **Default:** ``4``.

.. _direct_interconnect:

Direct Inter-block Connections
------------------------------

The content within the ``<directlist>`` tag consists of a group of ``<direct>`` tags.
The ``<direct>`` tag and its contents are described below.

.. arch:tag:: <direct name="string" from_pin="string" to_pin="string" x_offset="int" y_offset="int" z_offset="int" switch_name="string"/>

    :req_param name: is a unique alphanumeric string to name the connection.
    :req_param from_pin: pin of complex block that drives the connection.
    :req_param to_pin: pin of complex block that receives the connection.
    :req_param x_offset:  The x location of the receiving CLB relative to the driving CLB.
    :req_param y_offset: The y location of the receiving CLB relative to the driving CLB.
    :req_param z_offset: The z location of the receiving CLB relative to the driving CLB.
    :opt_param switch_name: [Optional, defaults to delay-less switch if not specified] The name of the ``<switch>`` from ``<switchlist>`` to be used for this direct connection.
    :opt_param from_side: The associated from_pin's block side (must be one of ``left``, ``right``, ``top``, ``bottom`` or left unspecified)
    :opt_param to_side: The associated to_pin's block side (must be one of ``left``, ``right``, ``top``, ``bottom`` or left unspecified)

    Describes a dedicated connection between two complex block pins that skips general interconnect.
    This is useful for describing structures such as carry chains as well as adjacent neighbour connections.

    The ``from_side`` and ``to_side`` options can usually be left unspecified.
    However they can be used to explicitly control how directs to physically equivalent pins (which may appear on multiple sides) are handled.

    **Example:**
    Consider a carry chain where the ``cout`` of each CLB drives the ``cin`` of the CLB immediately below it, using the delay-less switch one would enter the following:

    .. code-block:: xml

        <direct name="adder_carry" from_pin="clb.cout" to_pin="clb.cin" x_offset="0" y_offset="-1" z_offset="0"/>

.. _custom_switch_blocks:

Custom Switch Blocks
--------------------

The content under the ``<switchblocklist>`` tag consists of one or more ``<switchblock>`` tags that are used to specify connections between different segment types. An example is shown below:

    .. code-block:: xml

        <switchblocklist>
          <switchblock name="my_switchblock" type="unidir">
            <switchblock_location type="EVERYWHERE"/>
            <switchfuncs>
              <func type="lr" formula="t"/>
              <func type="lt" formula="W-t"/>
              <func type="lb" formula="W+t-1"/>
              <func type="rt" formula="W+t-1"/>
              <func type="br" formula="W-t-2"/>
              <func type="bt" formula="t"/>
              <func type="rl" formula="t"/>
              <func type="tl" formula="W-t"/>
              <func type="bl" formula="W+t-1"/>
              <func type="tr" formula="W+t-1"/>
              <func type="rb" formula="W-t-2"/>
              <func type="tb" formula="t"/>
            </switchfuncs>
            <wireconn from_type="l4" to_type="l4" from_switchpoint="0,1,2,3" to_switchpoint="0"/>
            <wireconn from_type="l8_global" to_type="l8_global" from_switchpoint="0,4"
                  to_switchpoint="0"/>
            <wireconn from_type="l8_global" to_type="l4" from_switchpoint="0,4"
                  to_switchpoint="0"/>
          </switchblock>

          <switchblock name="another_switch_block" type="unidir">
            ... another switch block description ...
          </switchblock>
        </switchblocklist>

This switch block format allows a user to specify mathematical permutation functions that describe how different types of segments (defined in the architecture file under the ``<segmentlist>`` tag) will connect to each other at different switch points.
The concept of a switch point is illustrated below for a length-4 unidirectional wire heading in the "left" direction.
The switch point at the start of the wire is given an index of 0 and is incremented by 1 at each subsequent switch block until the last switch point.
The last switch point has an index of 0 because it is shared between the end of the current segment and the start of the next one (similarly to how switch point 3 is shared by the two wire subsegments on each side).

.. figure:: switch_point_diagram.*

    Switch point diagram.

A collection of wire types and switch points defines a set of wires which will be connected to another set of wires with the specified permutation functions (the ‘sets’ of wires are defined using the ``<wireconn>`` tags).
This format allows for an abstract but very flexible way of specifying different switch block patterns.
For additional discussion on interconnect modeling see :cite:`petelin_masc`.
The full format is documented below.

**Overall Notes:**

#. The ``<sb type="pattern">`` tag on a wire segment (described under ``<segmentlist>``) is applied as a mask on the patterns created by this switch block format; anywhere along a wire’s length where a switch block has not been requested (set to 0 in this tag), no switches will be added.
#. You can specify multiple switchblock tags, and the switches described by the union of all those switch blocks will be created.

.. arch:tag:: <switchblock name="string" type="string">

    :req_param name: A unique alphanumeric string
    :req_param type: ``unidir`` or ``bidir``.
        A bidirectional switch block will implicitly mirror the specified permutation functions – e.g. if a permutation function of type ``lr`` (function used to connect wires from the left to the right side of a switch block) has been specified, a reverse permutation function of type ``rl`` (right-to-left) is automatically assumed.
        A unidirectional switch block makes no such implicit assumptions.
        The type of switch block must match the directionality of the segments defined under the ``<segmentlist>`` node.

    ``<switchblock>`` is the top-level XML node used to describe connections between different segment types.

.. arch:tag:: <switchblock_location type="string"/>

    :req_param type:
        Can be one of the following strings:

        * ``EVERYWHERE`` – at each switch block of the FPGA
        * ``PERIMETER`` – at each perimeter switch block (x-directed and/or y-directed channel segments may terminate here)
        * ``CORNER`` – only at the corner switch blocks (both x and y-directed channels terminate here)
        * ``FRINGE`` – same as PERIMETER but excludes corners
        * ``CORE`` – everywhere but the perimeter

    Sets the location on the FPGA where the connections described by this switch block will be instantiated.

.. arch:tag:: <switchfuncs>

    The switchfuncs XML node contains one or more entries that specify the permutation functions with which different switch block sides should be connected, as described below.

.. arch:tag:: <func type="string" formula="string"/>


    :req_param type:
        Specifies which switch block sides this function should connect.
        With the switch block sides being left, top, right and bottom, the allowed entries are one of {``lt``, ``lr``, ``lb``, ``tr``, ``tb``, ``tl``, ``rb``, ``rl``, ``rt``, ``bl``, ``bt``, ``br``} where ``lt`` means that the specified permutation formula will be used to connect the left-top sides of the switch block.

        .. note:: In a bidirectional architecture the reverse connection is implicit.

    :req_param formula:
        Specifies the mathematical permutation function that determines the pattern with which the source/destination sets of wires (defined using the <wireconn> entries) at the two switch block sides will be connected.
        For example, ``W-t`` specifies a connection where the ``t``’th wire in the source set will connect to the ``W-t`` wire in the destination set where ``W`` is the number of wires in the destination set and the formula is implicitly treated as modulo ``W``.

        Special characters that can be used in a formula are:

        * ``t`` -- the index of a wire in the source set
        * ``W`` -- the number of wires in the destination set (which is not necessarily the total number of wires in the channel)

        The operators that can be used in the formula are:

        * Addition (``+``)
        * Subtraction (``-``)
        * Multiplication (``*``)
        * Division (``/``)
        * Brackets ``(`` and ``)`` are allowed and spaces are ignored.

    Defined under the ``<switchfuncs>`` XML node, one or more ``<func...>`` entries is used to specify permutation functions that connect different sides of a switch block.


.. arch:tag:: <wireconn num_conns="expr" from_type="string, string, string, ..." to_type="string, string, string, ..." from_switchpoint="int, int, int, ..." to_switchpoint="int, int, int, ..." from_order="{fixed | shuffled}" to_order="{fixed | shuffled}" switch_override="string"/>

    :req_param num_conns:
        Specifies how many connections should be created between the from_type/from_switchpoint set and the to_type/to_switchpoint set.
        The value of this parameter is an expression which is evaluated when the switch block is constructed.

        The expression can be a single number or formula using the variables:

        * ``from`` -- The number of switchblock edges equal to the 'from' set size.

            This ensures that each element in the 'from' set is connected to an element of the 'to' set.
            However it may leave some elements of the 'to' set either multiply-connected or disconnected.

            .. figure:: wireconn_num_conns_type_from.*
                :width: 100%

        * ``to`` -- The number of switchblock edges equal to the 'to' set size size.

            This ensures that each element of the 'to' set is connected to precisely one element of the 'from' set.
            However it may leave some elements of the 'from' set either multiply-connected or disconnected.

            .. figure:: wireconn_num_conns_type_to.*
                :width: 100%

        Examples:

        * ``min(from,to)`` --  Creates number of switchblock edges equal to the minimum of the 'from' and 'to' set sizes.

            This ensures *no* element of the 'from' or 'to' sets is connected to multiple elements in the opposing set.
            However it may leave some elements in the larger set disconnected.

            .. figure:: wireconn_num_conns_type_min.*
                :width: 100%

        * ``max(from,to)`` -- Creates number of switchblock edges equal to the maximum of the 'from' and 'to' set sizes.

            This ensures *all* elements of the 'from' or 'to' sets are connected to at least one element in the opposing set.
            However some elements in the smaller set may be multiply-connected.

            .. figure:: wireconn_num_conns_type_max.*
                :width: 100%

        * ``3*to`` -- Creates number of switchblock edges equal to three times the 'to' set sizes.

    :req_param from_type:
        A comma-separated list segment names that defines which segment types will be a source of a connection.
        The segment names specified must match the names of the segments defined under the ``<segmentlist>`` XML node.
        Required if no ``<from>`` or ``<to>`` nodes are specified within the ``<wireconn>``.

    :req_param to_type:
        A comma-separated list of segment names that defines which segment types will be the destination of the connections specified.
        Each segment name must match an entry in the ``<segmentlist>`` XML node.
        Required if no ``<from>`` or ``<to>`` nodes are specified within the ``<wireconn>``.

    :req_param from_switchpoint:
        A comma-separated list of integers that defines which switchpoints will be a source of a connection.
        Required if no ``<from>`` or ``<to>`` nodes are specified within the ``<wireconn>``.

    :req_param to_switchpoint:
        A comma-separated list of integers that defines which switchpoints will be the destination of the connections specified.
        Required if no ``<from>`` or ``<to>`` nodes are specified within the ``<wireconn>``.

        .. note:: In a unidirectional architecture wires can only be driven at their start point so ``to_switchpoint="0"`` is the only legal specification in this case.

    :opt_param from_order:
        Specifies the order in which ``from_switchpoint``s are selected when creating edges.

        * ``fixed`` -- Switchpoints are selected in the order specified

            This is useful to specify a preference for connecting to specific switchpoints.
            For example,

            .. code-block:: xml

                <wireconn num_conns="1*to" from_type="L16" from_switchpoint="0,12,8,4" from_order="fixed" to_type="L4" to_switchpoint="0"/>

            specifies L4 wires should be connected first to L16 at switchpoint 0, then at switchpoints 12, 8, and 4.
            This is primarily useful when we want to ensure that some switchpoints are 'used-up' first.


        * ``shuffled`` -- Switchpoints are selected in a (randomly) shuffled order

            This is useful to ensure a diverse set of switchpoints are used.
            For example,

            .. code-block:: xml

                <wireconn num_conns="1*to" from_type="L4" from_switchpoint="0,1,2,3" from_order="shuffled" to_type="L4" to_switchpoint="0"/>

            specifies L4 wires should be connected to other L4 wires at any of switchpoints 0, 1, 2, or 3.
            Shuffling the switchpoints is useful if one of the sets (e.g. from L4's) is much larger than the other (e.g. to L4's), and we wish to ensure a variety of switchpoints from the larger set are used.

        **Default:** ``shuffled``


    :opt_param to_order:
        Specifies the order in which ``to_switchpoint``s are selected when creating edges.

        .. note:: See ``from_switchpoint_order`` for value descritpions.

    :opt_param switch_override:

        Specifies the name of a switch to be used to override the wire_switch of the segments in the ``to`` set. 
        Can be used to create switch patterns where different switches are used for different types of connections. 
        By using a zero-delay and zero-resistance switch one can also create T and L shaped wire segments.
        
        **Default:** If no override is specified, the usual wire_switch that drives the ``to`` wire will be used. 

    .. arch:tag:: <from type="string" switchpoint="int, int, int, ..."/>

        :req_param type:

            The name of a segment specified in the ``<segmentlist>``.

        :req_param switchpoint:

            A comma-separated list of integers that defines switchpoints.

            .. note:: In a unidirectional architecture wires can only be driven at their start point so ``to_switchpoint="0"`` is the only legal specification in this case.

        Specifies a subset of *source* wire switchpoints.

        This tag can be specified multiple times.
        The surrounding ``<wireconn>``'s source set is the union of all contained ``<from>`` tags.

    .. arch:tag:: <to type="string" switchpoint="int, int, int, ..."/>

        Specifies a subset of *destination* wire switchpoints.

        This tag can be specified multiple times.
        The surrounding ``<wireconn>``'s destination set is the union of all contained ``<to>`` tags.

        .. seealso:: ``<from>`` for attribute descriptions.


    As an example, consider the following ``<wireconn>`` specification:

    .. code-block:: xml

        <wireconn num_conns_type="to"/>
            <from type="L4" switchpoint="0,1,2,3"/>
            <from type="L16" switchpoint="0,4,8,12"/>
            <to type="L4" switchpoint="0/>
        </wireconn>

    This specifies that the 'from' set is the union of L4 switchpoints 0, 1, 2 and 3; and L16 switchpoints 0, 4, 8 and 12.
    The 'to' set is all L4 switchpoint 0's.
    Note that since different switchpoints are selected from different segment types it is not possible to specify this without using ``<from>`` sub-tags.

.. _arch_metadata:

Architecture metadata
---------------------

Architecture metadata enables tagging of architecture or routing graph
information that exists outside of the normal VPR flow (e.g. pack, place,
route, etc).  For example this could be used to enable bitstream generation by
tagging routing edges and pb_type features.

The metadata will not be used by the vpr executable, but can be leveraged by
new tools using the libvpr library.  These new tools can access the metadata
on the various VPR internal data structures.

To enable tagging of architecture structures with metadata, the ``<metadata>``
tag can be inserted under the following XML tags:

* ``<pb_type>``
* Any tag under ``<interconnect>`` (``<direct>``, ``<mux>``, etc).
* ``<mode>``
* Any grid location type (``<perimeter>``, ``<fill>``, ``<corners>``, ``<single>``, ``<col>``, ``<row>``, ``<region>``)

.. arch:tag:: <metadata>

Specifies the root of a metadata block.  Can have 0 or more ``<meta>`` Children.

.. arch:tag:: <meta name="string" >

    :req_param name: Key name of this metadata value.

Specifies a value within a metadata block.  The name is a key
for looking up the value contained within the ``<meta>`` tag.  Keys can be
repeated, and will be stored in a vector in order of occurrence.

The value of the ``<meta>`` is the text in the block. Both the ``name`` and
``<meta>`` value will be stored as a string.  XML children are not supported
in the ``<meta>`` tag.

Example of a metadata block with 2 keys:

    .. code-block:: xml

      <metadata>
        <meta name="some_key">Some value</meta>
        <meta name="other key!">Other value!</meta>
      </metadata>

.. _openfpga_arch_syntax:

Additional Syntax for Tileable Architecture
-------------------------------------------

When tileable architecture is enabled, the following options are available in the architecture file:

Layout
~~~~~~

``<layout>`` may include additioinal attributes to enable tileable routing resource graph generation

.. option:: tileable="<bool>"

  Turn ``on``/ ``off`` the tileable routing resource graph generator.
  
  The tileable routing architecture can minimize the number of unique modules in FPGA fabric to be physically implemented.

  Technical details can be found in :cite:`XTang_FPT_2019`. 

  .. note:: It is strongly recommended to enable the tileable routing architecture when you want to PnR large FPGA fabrics, which can effectively reduce the runtime.

.. option:: through_channel="<bool>"
  
  Allow routing channels to pass through multi-width and multi-height programmable blocks. This is mainly used in heterogeneous FPGAs to increase routability, as illustrated in :numref:`fig_thru_channel`.
  By default, it is ``false``.

  .. _fig_thru_channel:
  
  .. figure:: thru_channel.png
     :width: 100%
     :alt: Impact of through channel
  
     Impact on routing architecture when through channel in multi-width and multi-height programmable blocks: (a) disabled; (b) enabled.

  .. warning:: Do NOT enable ``through_channel`` if you are not using the tileable routing resource graph generator!
  
  .. warning:: You cannot use ``spread`` pin location for the ``height > 1`` or ``width >1`` tiles when using the tileable routing resource graph. Otherwise, it will cause undriven pins in your device!!!

.. option:: shrink_boundary="<bool>"
  
  Remove all the routing wires in empty regions. This is mainly used in non-rectangular FPGAs to avoid redundant routing wires in blank area, as illustrated in :numref:`fig_shrink_boundary`.
  By default, it is ``false``.

  .. _fig_shrink_boundary:
  
  .. figure:: shrink_boundary.png
     :width: 100%
     :alt: Impact of shrink boundary
  
     Impact on routing architecture when shrink-boundary: (a) disabled; (b) enabled.

  .. warning:: Do NOT enable ``shrink_boundary`` if you are not using the tileable routing resource graph generator!

.. option:: perimeter_cb="<bool>"
  
  Allow connection blocks to appear around the blocks on the perimeter of the device (mainly I/Os). This is designed to enhance routability of I/Os on perimeter. It is also strongly recommended when a programmable clock network is required to touch clock pins on I/Os. As illustrated in :numref:`fig_perimeter_cb`, routing tracks can access three sides of each I/O when perimeter connection blocks are created. 

  **Default:** ``false``

.. warning:: When enabled, please only place outputs at one side of I/Os. For example, outputs of an I/O on the top side can only occur on the bottom side of the I/O tile. Otherwise, routability loss may be expected, leading to some pins being unreachable. Enable the ``opin2all_sides`` to recover routability loss. 

  .. _fig_perimeter_cb:
  
  .. figure:: perimeter_cb.png
     :width: 100%
     :alt: Impact of perimeter_cb
  
     Impact on routing architecture when perimeter connection blocks are : (a) disabled; (b) enabled.

  .. warning:: Do NOT enable ``perimeter_cb`` if you are not using the tileable routing resource graph generator!

.. option:: opin2all_sides="<bool>"

  Allow each output pin of a programmable block to drive the routing tracks on all the sides of its adjacent switch block (see an illustrative example in :numref:`fig_opin2all_sides`). This can improve the routability of an FPGA fabric with an increase in the sizes of routing multiplexers in each switch block. 
  
  **Default:** ``false``

  .. _fig_opin2all_sides:
  
  .. figure:: opin2all_sides.svg
     :width: 100%
     :alt: Impact of opin2all_sides
  
     Impact on routing architecture when the opin-to-all-sides: (a) disabled; (b) enabled.

  .. warning:: Do NOT enable ``opin2all_sides`` if you are not using the tileable routing resource graph generator!

.. option:: concat_wire="<bool>"

  In each switch block, allow each routing track which ends to drive another routing track on the opposite side, as such a wire can be continued in the same direction (see an illustrative example in :numref:`fig_concat_wire`). In other words, routing wires can be concatenated in the same direction across an FPGA fabric. This can improve the routability of an FPGA fabric with an increase in the sizes of routing multiplexers in each switch block. 
  
  **Default:** ``false``

  .. _fig_concat_wire:
  
  .. figure:: concat_wire.svg
     :width: 100%
     :alt: Impact of concat_wire
  
     Impact on routing architecture when the wire concatenation: (a) disabled; (b) enabled.

  .. warning:: Do NOT enable ``concat_wire`` if you are not using the tileable routing resource graph generator!

.. option:: concat_pass_wire="<bool>"

  In each switch block, allow each routing track which passes to drive another routing track on the opposite side, as such a pass wire can be continued in the same direction (see an illustrative example in :numref:`fig_concat_pass_wire`). This can improve the routability of an FPGA fabric with an increase in the sizes of routing multiplexers in each switch block. 
  
  **Default:** ``false``

  .. _fig_concat_wire:
  
  .. figure:: concat_pass_wire.svg
     :width: 100%
     :alt: Impact of concat_pass_wire
  
     Impact on routing architecture when the pass wire concatenation: (a) disabled; (b) enabled.

  .. warning:: Do NOT enable ``concat_pass_wire`` if you are not using the tileable routing resource graph generator!

A quick example to show tileable routing is enabled, other options, e.g., through channels are disabled:

.. code-block:: xml

  <layout tileable="true" through_channel="false" shrink_boundary="false" opin2all_sides="false" concat_wire="false" concat_pass_wire="false">
  </layout>

Switch Block
~~~~~~~~~~~~

``<switch_block>`` may include addition syntax to enable different connectivity for pass tracks

.. option:: sub_type="<string>"
  
  Connecting type for pass tracks in each switch block.
  The supported connecting patterns are ``subset``, ``universal`` and ``wilton``, being the same as the capability of VPR.
  If not specified, the pass tracks will have the same connecting patterns as start/end tracks, which are defined in ``type``

.. option:: sub_Fs="<int>"

  Connectivity parameter for pass tracks in each switch block. Must be a multiple of 3.
  If not specified, the pass tracks will have the same connectivity as start/end tracks, which are defined in ``fs``.

A quick example which defines a switch block
  - Starting/ending routing tracks are connected in the ``wilton`` pattern
  - Each starting/ending routing track can drive 3 other starting/ending routing tracks
  - Passing routing tracks are connected in the ``subset`` pattern
  - Each passing routing track can drive 6 other starting/ending routing tracks

.. code-block:: xml

  <device>
    <switch_block type="wilton" fs="3" sub_type="subset" sub_fs="6"/>
  </device>

Routing Segments
~~~~~~~~~~~~~~~~

When using tileable architecture, it is strongly recommended to give explicit names
for each routing segment in ``<segmentlist>``.
This is used to link ``circuit_model`` to routing segments.

A quick example which defines a length-4 uni-directional routing segment called ``L4`` :

.. code-block:: xml

  <segmentlist>
    <segment name="L4" freq="1" length="4" type="undir"/>
  </segmentlist>

.. note:: Currently, tileable architecture only supports uni-directional routing architectures.

Direct Interconnect
~~~~~~~~~~~~~~~~~~~

This section introduces extensions on the architecture description file about direct connections between programmable blocks.

Syntax
~~~~~~

The original direct connections in the directlist section are documented in :ref:`direct_interconnect`. Its description is given below:

.. code-block:: xml

  <directlist>
    <direct name="string" from_pin="string" to_pin="string" x_offset="int" y_offset="int" z_offset="int" switch_name="string"/>
  </directlist>

.. note:: These options are required

In the tileable architecture file, you may define additional attributes for each VPR's direct connection:

.. code-block:: xml

  <direct_connection>
    <direct name="string" circuit_model_name="string" interconnection_type="string" x_dir="string" y_dir="string"/>
  </directlist>

.. note:: these options are optional. However, if ``interconnection_type`` is set to ``inter_column`` or ``inter_row``, then ``x_dir`` and ``y_dir`` are required.

.. option:: interconnection_type="<string>"

  Available types are ``inner_column_or_row`` | ``part_of_cb`` | ``inter_column`` | ``inter_row``

  - ``inner_column_or_row`` indicates the direct connections are between tiles in the same column or row. This is the default value.
  - ``part_of_cb`` indicates the direct connections will drive routing multiplexers in connection blocks. Therefore, it is no longer a strict point-to-point direct connection.
  - ``inter_column`` indicates the direct connections are between tiles in two columns
  - ``inter_row`` indicates the direct connections are between tiles in two rows

.. note:: The following syntax is only applicable to ``inter_column`` and ``inter_row``

.. option:: x_dir="<string>"

  Available directionalities are ``positive`` | ``negative``, specifies if the next cell to connect has a bigger or lower ``x`` value.
  Considering a coordinate system where (0,0) is the origin at the bottom left and ``x`` and ``y`` are positives: 

    - x_dir="positive": 

        - interconnection_type="inter_column": a column will be connected to a column on the ``right``, if it exists.

        - interconnection_type="inter_row": the most on the ``right`` cell from a row connection will connect the most on the ``left`` cell of next row, if it exists.

    - x_dir="negative": 

        - interconnection_type="inter_column": a column will be connected to a column on the ``left``, if it exists.

        - interconnection_type="inter_row": the most on the ``left`` cell from a row connection will connect the most on the ``right`` cell of next row, if it exists.

.. option:: y_dir="<string>"

  Available directionalities are ``positive`` | ``negative``, specifies if the next cell to connect has a bigger or lower x value.
  Considering a coordinate system where (0,0) is the origin at the bottom left and `x` and `y` are positives:

    - y_dir="positive": 

        - interconnection_type="inter_column": the ``bottom`` cell of a column will be connected to the next column ``top`` cell, if it exists.

        - interconnection_type="inter_row": a row will be connected on an ``above`` row, if it exists.

    - y_dir="negative": 

        - interconnection_type="inter_column": the ``top`` cell of a column will be connected to the next column ``bottom`` cell, if it exists.

        - interconnection_type="inter_row": a row will be connected on a row ``below``, if it exists.

Enhanced Connection Block
~~~~~~~~~~~~~~~~~~~~~~~~~

A direct connection can also drive routing multiplexers of connection blocks. When such connection occurs in a connection block, it is called an enhanced connection block.
:numref:`fig_ecb` illustrates the difference between a regular connection block and an enhanced connection block.

.. _fig_ecb:

.. figure:: ecb.png

    Enhanced connection block vs. Regular connection block

In such scenario, the type ``part_of_cb`` is required.

.. warning:: Restrictions may be applied when building the direct connections as part of a connection block. 

Direct connections can be inside a tile or across two tiles. Currently, across more than two tiles are not supported!
:numref:`fig_ecb_allowed_direct_connection` illustrates the region (in red) where any input pin is allowed to be driven by any output pin.

.. _fig_ecb_allowed_direct_connection:

.. figure:: ecb_allowed_direct_connection.png

    Allowed connections inside a tile for enhanced connection block (see the highlighted region)

:numref:`fig_ecb_allowed_direct_connection_inner_tile_example` shows a few feedback connections which can be built inside connection blocks. Note that feedback connections are fully allowed between any pins on the same side of a programmable block.

.. _fig_ecb_allowed_direct_connection_inner_tile_example:

.. figure:: ecb_allowed_direct_connection_inner_tile_example.png

    Example of feedback connections inside a tile for enhanced connection block

For instance, a non-tileable VPR architecture defines:

.. code-block:: xml

  <directlist>
    <!-- Add 2 inputs to the routing multiplexers inside a connection block which drives pin 'clb.I_top[0]' -->
    <direct name="feedback" from_pin="clb.O_top[0:0]" to_pin="clb.I_top[0:0]" x_offset="0" y_offset="0" z_offset="0"/>
    <direct name="feedback" from_pin="clb.O_top[1:1]" to_pin="clb.I_top[0:0]" x_offset="0" y_offset="0" z_offset="0"/>
  </directlist>

:numref:`fig_ecb_allowed_direct_connection_inter_tile_example` shows a few inter-tile connections which can be built inside connection blocks. Note that inter-tile connections are subjected to the restrictions depicted in :numref:`fig_ecb_allowed_direct_connection`

.. _fig_ecb_allowed_direct_connection_inter_tile_example:

.. figure:: ecb_allowed_direct_connection_inter_tile_example.png

    Example of connections across two tiles for enhanced connection block

:numref:`fig_ecb_forbid_direct_connection_example` illustrates some inner-tile and inter-tile connections which are not allowed. Note that feedback connections across different sides are restricted! 

.. _fig_ecb_forbid_direct_connection_example:

.. figure:: ecb_forbid_direct_connection_example.png

    Restrictions on building direct connections as part of a connection block

Inter-tile Connections
~~~~~~~~~~~~~~~~~~~~~~

For this example, we will study a scan-chain implementation. The description could be:

In a non-tileable architecture:

.. code-block:: xml

  <directlist>
    <direct name="scff_chain" from_pin="clb.sc_out" to_pin="clb.sc_in" x_offset="0" y_offset="-1" z_offset="0"/>
  </directlist>

In a tileable architecture:

.. code-block:: xml

  <direct_connection>
    <direct name="scff_chain" interconnection_type="column" x_dir="positive" y_dir="positive"/>
  </direct_connection>

:numref:`fig_p2p_exple` is the graphical representation of the above scan-chain description on a 4x4 FPGA.

.. _fig_p2p_exple:

.. figure:: point2point_example.png

    An example of scan-chain implementation


In this figure, the red arrows represent the initial direct connection. The green arrows represent the point to point connection to connect all the columns of CLB.

A point to point connection can be applied in different ways than showed in the example section. To help the designer implement their point to point connection, a truth table with our new parameters is provided below.

:numref:`fig_p2p_trtable` provides all possible variable combination and the connection it will generate.

.. _fig_p2p_trtable:

.. figure:: point2point_truthtable.png

    Point to point truth table

VIB Architecture
~~~~~~~~~~~~~~~~

.. include:: VIB.rst

