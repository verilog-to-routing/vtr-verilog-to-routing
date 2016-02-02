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
* ``<layout>``
* ``<device>``
* ``<switchlist>``
* ``<segmentlist>``
* ``<directlist>``
* ``<complexblocklist>``

.. _arch_blif_models:

Recognized BLIF Models (<models>)
---------------------------------
The ``<models>`` tag contains ``<model name="string">`` tags.
Each ``<model>`` tag describes the BLIF ``.subckt`` model names that are accepted by the FPGA architecture.
The name of the model must match the corresponding name of the BLIF model.

.. note:: 
    Standard blif structures (``.names``, ``.latch``, ``.input``, ``.output``) are accepted by default, so these models should not be described in the <models> tag.

Each model tag must contain 2 tags: ``<input_ports>`` and ``<output_ports>``.
Each of these contains ``<port>`` tags:

.. arch:tag:: <port name="string" is_clock="{0 | 1}/>

    :req_param name: The port name.
    :opt_param is_clock: Indicates if the port is a clock. Default: ``0``

    Defines the port for a model. 

.. _arch_global_info:

Global FPGA Information
-----------------------

.. arch:tag:: <layout {auto="float" | width="int" height="int"}/>

    :opt_param auto: Aspect ratio fo auto-sized FPGA
    :opt_param width: FPGA width in grid-units
    :opt_param height: FPGA height in grid-units

    This tag specifies the size and shape of the FPGA in grid units.

    The keyword ``auto`` indicates that the size should be chosen to be the minimal dimensions that fits the given circuit.
    The size is determined from the number of grid tiles used by the circuit as well as the number of IO pins that it uses.
    The aspect ratio of the FPGA is given after the auto keyword and is the ratio width/height. 

    Alternately, the size can be explicitly given as the size in the x direction (``width``) followed by the size in the y direction (``height``).

.. arch:tag:: <device>content</device>

    Content inside this tag specifies device information.

    .. seealso:: :ref:`arch_device_info`

.. arch:tag:: <switchlist>content</switchlist>

    Content inside this tag contains a group of <switch> tags that specify the types of switches and their properties.

.. arch:tag:: <segmentlist>content</segmentlist>

    Content inside this tag contains a group of <segment> tags that specify the types of wire segments and their properties.

.. arch:tag:: <complexblocklist>content</complexblocklist>

    Content inside this tag contains a group of <pb_type> tags that specify the types of functional blocks and their properties.
    

.. _arch_device_info:

FPGA Device Information
-----------------------
The tags within the ``<device>`` tag are:

.. arch:tag:: <sizing R_minW_nmos="float" R_minW_pmos="float" ipin_mux_trans_size="int"/>

    :req_param R_minW_nmos: 
        The resistance of minimum-width nmos transistor.  
        This data is used only by the area model built into VPR.

    :req_param R_minW_pmos: 
        The resistance of minimum-width pmos transistor.  
        This data is used only by the area model built into VPR.

    :req_param ipin_mux_trans_size:
        This specifies the size of each transistor in the ipin muxes.
        Given in minimum transistor units.
        The mux is implemented as a two-level mux.

    :required: Yes

    Specifies parameters used by the area model built into VPR.


.. arch:tag:: <timing C_ipin_cblock="float" T_ipin_cblock="float"/>

    :req_param C_ipin_cblock: 
        Input capacitance of the buffer isolating a routing track from the connection boxes (multiplexers) that select the signal to be connected to a logic block input pin.
        One of these buffers is inserted in the FPGA for each track at each location at which it connects to a connection box.
        For example, a routing segment that spans three logic blocks, and connects to logic blocks at two of these three possible locations would have two isolation buffers attached to it.
        If a routing track connects to the logic blocks both above and below it at some point, only one isolation buffer is inserted at that point.
        If your connection from routing track to connection block does not include a buffer, set this parameter to the capacitive loading a track would see at each point where it connects to a logic block or blocks.

    :req_param T_ipin_cblock:
        Delay to go from a routing track, through the isolation buffer (if your architecture contains these) and a connection block (typically a multiplexer) to a logic block input pin.

    :required: Yes (for timing analysis), optional otherwise.

    Attributes specifying timing information general to the device.


    .. figure:: ipin_diagram.*

        Input Pin Diagram.

.. arch:tag:: <area grid_logic_tile_area="float"/>

    :required: Yes

    Used for an area estimate of the amount of area taken by all the functional blocks.
    This specifies the area of a 1x1 tile excluding routing.


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
    For segments that pass through the switch block that can also turn there, we cannot use the wilton pattern because a undirectional segment cannot be driven at an intermediate point, so we assign connections to starting segments following a round robin scheme (to balance mux size).

    .. note:: The round robin scheme is not tileable.

    **Custom Switch Blocks:**

    Specifying ``custom`` allows custom switch blocks to be described under the ``<switchblocklist>`` XML node, the format for which is described in Section 3.1.8.
    If the switch block is specified as ``custom``, the ``fs`` field does not have to be specified, and will be ignored if present. 

.. arch:tag:: <chan_width_distr>content</chan_width_distr>

    Content inside this tag is only used when VPR is in global routing mode.
    The contents of this tag are described in :ref:`global_routing_info`.


.. _global_routing_info:

Global Routing Information
~~~~~~~~~~~~~~~~~~~~~~~~~~
If global routing is to be performed, channels in different directions and in different parts of the FPGA can be set to different relative widths.
This is specified in the content within the ``<chan_width_distr>`` tag.

.. note:: If detailed routing is to be performed, all the channels in the FPGA must have the same width.

.. arch:tag:: <io width= "float"/>

    :req_param width: The relative channel width.

    Specifies the width of the channels between the pads and core relative to the widest core channel.

.. arch:tag:: <x distr="{gaussian|uniform|pulse|delta}" peak="float" width=" float" xpeak=" float" dc=" float"/>

    :req_param distr: The channel width distribution function
    :req_param peak: The peak value of the distribution
    :opt_param width: The width of the distribution. Required for ``pulse`` and ``gaussian``.
    :opt_param xpeak: Peak location horizontally. Required for ``pulse``, ``gaussian`` and ``delta``.
    :opt_param dc: The DC level of the distribution. Required for ``pulse``, ``gaussian`` and ``delta``.

    .. warning:: Shaped channels for global routing is rarely used so it is unknown if this feature still works.

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


.. _arch_complex_logic_blocks:

Complex Logic Blocks
--------------------

.. seealso:: For a step-by-step walkthrough on building a complex block see :ref:`arch_tutorial`.

The content within the ``<complexblocklist>`` tag describes the complex logic blocks found within the FPGA.
Each type of complex logic block is specified by a ``<pb_type name="string" height="int">`` tag within the ``<complexblocklist>`` tag.
The name attribute is the name for the complex block.
The height attribute specifies how many grid tiles the block takes up.

The internals of a complex block is described using a hierarchy of ``<pb_type>`` tags.
The top-level ``<pb_type>`` tag specifies the complex block.
Children ``<pb_type>`` tags are either clusters (which contain other ``<pb_type>`` tags) or primitives (leaves that do not contain other ``<pb_type>`` tags).
Clusters can contain other clusters and primitives so there is no restriction on the hierarchy that can be specified using this language.
All children ``<pb_type>`` tags contain the attribute ``num_pb="int"`` which describes the number of instances of that particular type of cluster or leaf block in that section of the hierarchy.
All children ``<pb_type>`` tags must have a ``name ="string"`` attribute where the name must be unique with respect to any parent, sibling, or child ``<pb_type>`` tag.
Leaf ``<pb_type>`` tags may optionally have a ``blif_model="string"`` attribute.
This attribute describes the type of block in the blif file that this particular leaf can implement.
For example, a leaf that implements a LUT should have ``blif_model=".names"``.
Similarly, a leaf that implements ``.subckt user_block_A`` should have attribute ``blif_model=".subckt user_block_A"``.
The input, output, and/or clock, ports for these leaves must match the ports specified in the ``<models>`` section of the architecture file.
There is a special attribute for leaf nodes called class that will be described in more detail later.

The following tags are common to all <pb_type> tags:

.. arch:tag:: <input name="string" num_pins="int" equivalent="true|false" is_non_clock_global="{true|false}"/>

    :req_param name: Name of the input port.
    :req_param num_pins: Number of pins the input port has.

    :opt_param equivalent: 
        *Applies only to top-level pb_type.* 
        Describes if the pins of the port are logically equivalent.
        Input logical equivalence means that the pin order can be swapped without changing functionality.
        For example, an AND gate has logically equivalent inputs because you can swap the order of the inputs and it’s still correct; an adder, on the otherhand, is not logically equivalent because if you swap the MSB with the LSB, the results are completely wrong.

    :opt_param is_non_clock_global: 
        *Applies only to top-level pb_type.*
        Describes if this input pin is a global signal that is not a clock.
        Very useful for signals such as FPGA-wide asychronous resets.
        These signals have their own dedicated routing channels and so should not use the general interconnect fabric on the FPGA.

    Defines an input port.  
    Multple input ports are described using multiple <input> tags.

.. arch:tag:: <output name="string" num_pins="int" equivalent="{true|false}"/>

    :req_param name: Name of the output port.
    :req_param num_pins: Number of pins the output port has.

    :opt_param equivalent: 
        *Applies only to top-level pb_type.*
        Describes if the pins of the port are logically equivalent.
        *See above description for inputs.*
    

    Defines an output port.
    Multple output ports are described using multiple <output> tags

.. arch:tag:: <clock name="string" num_pins="int" equivalent="{true|false}"/>

    Describes a clock port.  
    Multple clock ports are described using multiple <clock> tags.
    *See above descriptions on inputs/outputs*

.. arch:tag:: <mode name="string">
    
    :req_param name: 
        Name for this mode.
        Must be unique compared to other modes.

    Specifies a mode of operation for the <pb_type>.
    Each child mode tag denotes a different mode of operation for that <pb_type>.
    A mode tag contains <pb_type> tags and <interconnect> tags.
    If a ``<pb_type>`` has only one mode of operation, then this mode tag can be omitted.
    More on interconnect later.

The following tags are unique to the top level <pb_type> of a complex logic block.
They describe how a complex block interfaces with the inter-block world.

.. arch:tag:: <fc default_in_type="{frac|abs}" default_in_val="{int|float}" default_out_type="{frac|abs}" default_out_val="{int|float}">

    :req_param default_in_type:
        Indicates how the default :math:`F_c` values for input pins should be interpreted.
        
        ``frac``: The fraction of tracks in the channel from which each input pin connects.

        ``abs``: Inpterpretted as the absolute number of tracks from which each input pin connects.

    :req_param default_in_val:
        Fraction or number of tracks in a channel from which each input pin connects.

    :req_param default_out_type:
        Indicates how the default :math:`F_c` values for output pins should be interpreted.
        
        ``frac``: The fraction of tracks in the channel to which each output pin connects.

        ``abs``: Inpterpretted as the absolute number of tracks to which each output pin connects.

    :req_param default_out_val:
        Fraction or number of tracks in a channel to which each output pin connects.

    Sets the number of tracks to which each logic block pin connects in each channel bordering the pin.
    The :math:`F_c` value :cite:`brown_fpgas` used is always the minimum of the specified :math:`F_c` and the channel width, :math:`W`.


    **Special Case:**

    If you have complex block pins that do not connect to general interconnect (eg. carry chains), you would use the ``<pin>`` tag, within the ``<fc>`` tag, to specify them

    .. code-block:: xml

        <pin name="<string>" fc_type="frac" fc_val="0"/>

    Where the attribute ``name`` is the name of the pin.  
    Note: ``fc_val`` must be 0.
    Other values are not yet supported.


    **Special Case:**

    An optional segment-based override can be specified within the ``<fc>`` tag as:

    .. code-block:: xml

        <segment name="string" in_val="float" out_val="float"/>

    The name corresponds to one of the segment types defined under the ``<segmentlist>`` tag.
    The ``in_val`` and ``out_val`` fields specify the new connection block flexibilities with which the block pins will connect to the specified segments.
    The ``default_in_type`` and ``default_out_type`` values specified in the <fc> tag will apply to this segment-based override.
    Note that the segment-based override cannot be used in combination with the pin-based override above – this is currently not supported.

.. arch:tag:: <pinlocations pattern="{spread|custom}">

    :req_param pattern:
        ``spread`` denotes that the pins are to be spread evenly on all sides of the complex block.

        ``custom`` allows the architect to specify specifically where the pins are to be placed using ``<loc>`` tags.
        
    Describes the locations where the input, output, and clock pins are distributed in a complex logic block.

    .. arch:tag:: <loc side="{left|right|bottom|top}" offset="int">name_of_complex_logic_block.port_name[int:int] ... </loc>

        .. note:: ``...`` represents repeat as needed. Do not put ``...`` in the architecture file.

        :req_param side: Specifies which of the four directions the pins in the contents are located on
        :opt_param offset: 
            Specifies the grid distance from the bottom grid tile that the pin is specified for.
            Pins on the bottom grid tile have an offset value of ``0``.  
            The offset value must be less than the height of the functional block.

            **Default:** ``0``


        Physical equivalence for a pin is specified by listing a pin more than once for different locations.
        For example, a LUT whose output can exit from the top and bottom of a block will have its output pin specified twice: once for the top and once for the bottom.

.. arch:tag:: <gridlocations>

    Specifies the columns on the FPGA that will consist of this complex logic block.  
    The columns are specified by a group of <loc> tags and there are three ways to use this tag:

    .. arch:tag:: <loc type="col" start="int" repeat="int" priority="int"/>

        This specifies an absolute column assignment.
        The first column to contain this complex logic block is specified in start.
        Every column that satisfies :math:`x = start_x + k*repeat`, where :math:`k` is any integer, will be composed of this complex logic block.

    .. arch:tag:: <loc type="rel" pos="float"  priority="int"/> 

        This specifies a single column to be composed of this complex logic block where ``pos`` specifies a fraction of device the width.

    .. arch:tag:: <loc type="fill"  priority="int"/>

        This is a special specification such that all unspecified columns get assigned this complex logic block.

    For all three <loc> tags, the ``priority`` attribute is used to resolve collisions when two different functional blocks would otherwise use the same column.  
    The larger integer specified for priority gets the location.



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


Classes
~~~~~~~
Using these structures, we believe that one can describe any digital complex logic block.
However, we believe that certain kinds of logic structuers are common enough in FPGAs that special shortcuts should be available to make their specification easier.
These logic structures are: flip-flops, LUTs, and memories.
These structures are described using a ``class=string`` attribute in the ``<pb_type>`` primitive.
The classes we offer are:

.. arch:tag:: class="lut"

    Describes a K-input lookup table.

    The unique characteristic of a lookup table is that all inputs to the lookup table are logically equivalent.
    When this class is used, the input port must have a ``port_class="lut_in"`` attribute and the output port must have a ``port_class="lut_out"`` attribute.

.. arch:tag:: class="flipflop"

    Describes a flipflop.

    Input port must have a port_class="D" attribute added.
    Output port must have a port_class="Q" attribute added.
    Clock port must have a port_class="clock" attribute added.

.. arch:tag:: class="memory"

    Describes a memory.  
    
    Memories are unique in that a single memory physical primitive can hold multiple, smaller, logical memories as long as:

    #. The address, clock, and control inputs are identical and 
    #. There exists sufficient physical data pins to satisfy the netlist memories when the different netlist memories are merged together into one physical memory.

    Different types of memeories require different attributes.

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
Timing is specified through tags contained in pb_type, complete, direct, or mux tags as follows:

.. arch:tag:: <delay_constant max="float" in_port="string" out_port="string"/>

    :req_param max: The maximum delay value.
    :req_param in_port: The input port name.
    :req_param out_port: The output port name.

    Specifies a maximum delay equal from in_port to out_port.


.. arch:tag:: <delay_matrix type="max" in_port="string" out_port="string"> matrix </delay>

    :req_param type: Specifies the delay type. Currently on ``max`` is supported.
    :req_param in_port: The input port name.
    :req_param out_port: The output port name.
    :req_param matrix: The delay matrix.

    Describe a timing matrix for all edges going from ``in_port`` to ``out_port``.
    Number of rows of matrix should equal the number of inputs, number of columns should equal the number of outputs.

    **Example:**
    The following defines a delay matrix for a 4-bit input port ``in``, and 3-bit output port ``out``:

    .. code-block:: xml

        <delay_matrix type="max" in_port="in" out_port="out">
            1.2e-10 1.4e-10 3.2e-10
            4.6e-10 1.9e-10 2.2e-10
            4.5e-10 6.7e-10 3.5e-10
            7.1e-10 2.9e-10 8.7e-10
        </delay>
    
.. arch:tag:: <T_setup value="float" port="string" clock="string"/>

    :req_param value: The setup time value.
    :req_param port: The port name the setup constraint applies to.
    :req_param clock: The port name of the clock the setup constraint is specified relative to.

    Specifies a ports setup constraint.

    .. note:: Applies to sequential ``<pb_type>`` only.     

.. arch:tag:: <T_clock_to_Q max="float" port="string" clock="string"/>

    :req_param max: The maximum clock-to-Q delay value. 
    :req_param port: The port name the delay value applies to.
    :req_param clock: The port name of the clock the clock-to-Q delay is specified relative to.

    Specifies a ports clock-to-Q delay.

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

Wire Segments
-------------

The content within the ``<segmentlist>`` tag consists of a group of ``<segment>`` tags.
The ``<segment>`` tag and its contents are described below.

.. arch:tag:: <segment name="unique_name" length="int" type="{bidir|unidir}" freq="float" Rmetal="float" Cmetal="float">content</segment>

    :req_param name:  
        A unique alphanumeric name to identify this segment type.

    :req_param length:  
        Either the number of logic blocks spanned by each segment, or the keyword longline.
        Longline means segments of this type span the entire FPGA array.

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

.. arch:tag:: <sb type="pattern">int list</sb>

    This tag describes the switch block depopulation (as illustrated in the figure below) for this particular wire segment.
    For example, the firsth length 6 wire in the figure below has an sb pattern of ``1 0 1 0 1 0 1``.
    The second wire has a pattern of ``0 1 0 1 0 1 0``.
    A ``1`` indicates the existance of a switch block and a ``0`` indicates that there is no switch box at that point.
    Note that there a 7 entries in the integer list for a length 6 wire.
    For a length L wire there must be L+1 entries seperated by spaces.

    .. figure sb_pattern.*

        Switch Block Pattern

.. arch:tag:: <cb type="pattern">int list</cb>

    This tag describes the connection block depopulation (as illustrated by the circles in the figure above) for this particular wire segment.
    For example, the firsth length 6 wire in the figure below has an sb pattern of ``1 1 1 1 1 1``.
    The third wire has a pattern of ``1 0 0 1 1 0``.
    A ``1`` indicates the existance of a connection block and a ``0`` indicates that there is no connection box at that point.
    Note that there a 6 entries in the integer list for a length 6 wire.
    For a length L wire there must be L entries seperated by spaces.

.. arch:tag:: <mux name="string"/>

    .. warning:: Option for UNIDIRECTIONAL only.  

    Tag must be included and ``name`` must be the same as the name you give in ``<switch type="mux" name="...``

.. arch:tag:: <wire_switch name="string"/>

    .. warning:: Option for BIDIRECTIONAL only.
    
    Tag must be included and the name must be the same as the name you give in ``<switch type="buffer" name="...`` for the switch which represents the wire switch in your architecture.

.. arch:tag:: <opin_switch name="string"/>

    .. warning:: Option for BIDIRECTIONAL only.

    :req_param name: The index of the switch type used by clb and pad output pins to drive this type of segment.

    Tag must be included and ``name`` must be the same as the name you give in ``<switch type="buffer" name="...`` for the switch which represents the output pin switch in your architecture.

    .. note:: 
        In unidirectional segment mode, there is only a single buffer on the segment.
        Its type is specified by assigning the same switch index to both wire_switch and opin_switch.
        VPR will error out if these two are not the same.

    .. note::
        The switch used in unidirectional segment mode must be buffered.
 
Switches
--------
The content within the ``<switchlist>`` tag consists of a group of ``<switch>`` tags.
The ``<switch>`` tag and its contents are described below.

.. arch:tag:: 
    <switch type="{buffered|mux}" name="unique name" R="float" Cin="float" Cout="float" Tdel=" float" buf_size="float" mux_trans_size="float", power_buf_size="int"/>

    :req_param name: A unique alphanumeric string which needs to match the segment definition (see above)
    :req_param buffered: Indicates if this switch is a tri-state buffer
    :req_param mux: Indicates if this is a multiplexer
    :req_param R: Resistance of the switch.
    :req_param Cin:  Input capacitance of the switch.
    :req_param Cout:  Output capacitance of the switch.
    :req_param Tdel:  
        Intrinsic delay through the switch.
        If this switch was driven by a zero resistance source, and drove a zero capacitance load, its delay would be Tdel + R * Cout.
        The ‘switch’ includes both the mux and buffer when in unidirectional mode. 

    :opt_param buf_size:  
        *Only for unidirectional routing.*
        May only be used in unidirectional mode.
        This is an optional parameter that specifies area of the buffer in minimum-width transistor area units.
        If not given, this value will be determined automatically from R values.
        This allows you to use timing models without R’s and C’s and still be able to measure area.

    :opt_param mux_trans_size: 
        *Only for unidirectional routing.*
        This parameter must be used if and only if unidirectional segments are used since bidirectional mode switches don’t have muxes.
        The value controls the size of each transistor in the mux, measured in minimum width transistors.
        The mux is a two-level mux.

    :opt_param power_buf_size: *Used for power estimation.* The size is the drive strength of the buffer, relative to a minimum-sized inverter.

    Describes a type of switch.
    This statement defines what a certain type of switch is -- segment statements refer to a switch types by their number (the number right after the switch keyword).

.. arch:tag:: <Tdel num_inputs="int" delay="float"/>

    :req_param num_inputs: The number of switch inputs
    :req_param delay: The intrinsic switch delay when the switch topology has the specified number of switch inputs

    Instead of specifying a single Tdel value, a list of Tdel values may be specified for different values of switch fan-in.
    
    **Example:**

    .. code-block:: xml

        <switch type="mux" name="my_mux" R="522" Cin="3.1e-15" Cout="3e-15" mux_trans_size="1.7" buf_size="23">
            <Tdel num_inputs="12" delay="8.00e-11"/>
            <Tdel num_inputs="15" delay="8.4e-11"/>
            <Tdel num_inputs="20" delay="9.4e-11"/>
        </switch>

Clocks
------
The clocking configuration is specified with ``<clock>`` tags within the ``<clocks>`` section.

.. note:: Currently the information in the ``<clocks>`` section is only used for power estimation.

.. seealso:: :ref:`power_estimation` for more details.

.. arch:tag:: <clock C_wire="float" C_wire_per_m="float" buffer_size={"float"|"auto"}/>

    :opt_param C_wire: The absolute capacitance, in Farads, of the wire between each clock buffer.
    :opt_param C_wire_per_m: The wire capacitance, in Farads per Meter.
    :opt_param buffer_size: The size of each clock buffer.


Power
-----
Additional power options are specified within the ``<architecture>`` level ``<power>`` section.

.. seealso:: See :ref:`power_estimation` for full documentation on how to perform power estimation.

.. arch:tag:: <local_interconnect C_wire="float" factor="float"/>

    :req_param C_wire: The local interconnect capacitance in Farads/Meter.
    :opt_param factor: The local interconnect scaling factor. **Default:** ``0.5``.

.. arch:tag:: <buffers logical_effort_factor="float"/>

    :req_param logical_effort_factor: **Default:** ``4``.
    

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
    :req_param switch_name: [Optional, defaults to delay-less switch if not specified] The name of the <switch> from <switchlist> to be used for this direct connection.


    Describes a dedicated connection between two complex block pins that skips general interconnect.
    This is useful for describing structures such as carry chains as well as adjacent neighbour connections.

    **Example:**
    Consider a carry chain where the cout of each CLB drives the cin of the CLB immediately below it, using the delay-less switch one would enter the following:

    .. code-block:: xml

        <direct name="adder_carry" from_pin="clb.cout" to_pin="clb.cin" x_offset="0" y_offset="-1" z_offset="0"/>

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
The full format is documented below.

**Overall Notes:**

#. The ``<sb type="pattern">`` tag on a wire segment (described under ``<segmentlist>``) is applied as a mask on the patterns created by this switch block format; anywhere along a wire’s length where a switch block has not been requested (set to 0 in this tag), no switches will be added.
#. You can specify multiple switchblock tags, and the switches described by the union of all those switch blocks will be created.

.. arch:tag:: <switchblock name="string" type="string">

    :req_param name: A unique alphanumeric string
    :req_param type: 
        unidir or bidir.
        A bidirectional switch block will implicitly mirror the specified permutation functions – e.g. if a permutation function of type "lr" (function used to connect wires from the left to the right side of a switch block) has been specified, a reverse permutation function of type "rl" (right-to-left) is automatically assumed.
        A unidirectional switch block makes no such implicit assumptions.
        The type of switch block must match the directionality of the segments defined under the ``<segmentlist>`` node.

    <switchblock> is the top-level XML node used to describe connections between different segment types.

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
        .. note:: In a bidirectional architecture the reverse connection is implicit).

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


.. arch:tag:: <wireconn from_type="string, string, string, ..." to_type="string, string, string, ..." from_switchpoint="int, int, int, ..." to_switchpoint="int, int, int, ..."/>
    
    :req_param from_type: 
        A comma-separated list segment names that defines which segment types will be a source of a connection.
        The segment names specified must match the names of the segments defined under the ``<segmentlist>`` XML node.

    :req_param to_type:     
        A comma-separated list of segment names that defines which segment types will be the destination of the connections specified.
        Each segment name must match an entry in the ``<segmentlist>`` XML node.

    :req_param from_switchpoint: 
        A comma-separated list of integers that defines which switchpoints will be a source of a connection.

    :req_param to_switchpoint: 
        A comma-separated list of integers that defines which switchpoints will be the destination of the connections specified.
        .. note:: In a unidirectional architecture wires can only be driven at their start point so ``to_switchpoint="0"`` is the only legal specification in this case.

