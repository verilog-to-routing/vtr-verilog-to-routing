.. _vpr_file_formats:

File Formats
============
VPR consumes and produces several files representing the packing, placement, and routing results.

FPGA Architecture (.xml)
--------------------------
The target FPGA architecture is specified as an architecture file.
For details of this file format see :ref:`fpga_architecture_description`.

.. _blif_format:
.. _vpr_blif_file:

BLIF Netlist (.blif)
--------------------------
The technology mapped circuit to be implement on the target FPGA is specified as a Berkely Logic Interchange Format (BLIF) netlist.
The netlist must be flattened and consist of only primitives (e.g. ``.names``, ``.latch``, ``.subckt``).

For a detailed description of the BLIF file format see the :download:`BLIF Format Description <../../../libs/EXTERNAL/libblifparse/doc/blif.pdf>`.

Note that VPR supports only the structural subset of BLIF, and does not support the following BLIF features:

 * Subfile References (``.search``).
 * Finite State Machine Descriptions (``.start_kiss``, ``.end_kiss`` etc.).
 * Clock Constraints (``.cycle``, ``.clock_event``).
 * Delay Constraints (``.delay`` etc.).

Clock and delay constraints can be specified with an :ref:`SDC File <vpr_sdc_file>`.

.. note:: By default VPR assumes file with ``.blif`` are in structural BLIF format. The format can be controlled with :option:`vpr --circuit_format`.

Black Box Primitives
~~~~~~~~~~~~~~~~~~~~
Black-box architectural primitives (RAMs, Multipliers etc.) should be instantiated in the netlist using BLIF's ``.subckt`` directive.
The BLIF file should also contain a black-box ``.model`` definition which defines the input and outputs of each ``.subckt`` type.

VPR will check that blackbox ``.model``\s are consistent with the :ref:`<models> section <arch_models>` of the architecture file.

Unconnected Primitive Pins
~~~~~~~~~~~~~~~~~~~~~~~~~~
Unconnected primitive pins can be specified through several methods.

#. The ``unconn`` net (input pins only).

    VPR treats any **input pin** connected to a net named ``unconn`` as disconnected.

    For example:

    .. code-block:: none

        .names unconn out
        0 1

    specifies an inverter with no connected input.

    .. note:: ``unconn`` should only be used for **input pins**. It may cause name conflicts and create multi-driven nets if used with output pins.

#. Implicitly disconnected ``.subckt`` pins.

    For ``.subckt`` instantiations VPR treats unlisted primitive pins as implicitly disconnected.
    This works for both input and output pins.

    For example the following ``.subckt`` instantiations are equivalent:

    .. code-block:: none

        .subckt single_port_ram \
            clk=top^clk \
            data=top.memory_controller+memtroll^MULTI_PORT_MUX~8^MUX_2~546 \
            addr[0]=top.memory_controller+memtroll^MULTI_PORT_MUX~8^MUX_2~541 \
            addr[1]=top.memory_controller+memtroll^MULTI_PORT_MUX~8^MUX_2~542 \
            addr[2]=top.memory_controller+memtroll^MULTI_PORT_MUX~8^MUX_2~543 \
            addr[3]=top.memory_controller+memtroll^MULTI_PORT_MUX~8^MUX_2~544 \
            addr[4]=top.memory_controller+memtroll^MULTI_PORT_MUX~8^MUX_2~545 \
            addr[5]=unconn \
            addr[6]=unconn \
            addr[7]=unconn \
            addr[8]=unconn \
            addr[9]=unconn \
            addr[10]=unconn \
            addr[11]=unconn \
            addr[12]=unconn \
            addr[13]=unconn \
            addr[14]=unconn \
            we=top.memory_controller+memtroll^MULTI_PORT_MUX~8^MUX_2~554 \
            out=top.memory_controller+memtroll.single_port_ram+str^out~0

    .. code-block:: none

        .subckt single_port_ram \
            clk=top^clk \
            data=top.memory_controller+memtroll^MULTI_PORT_MUX~8^MUX_2~546 \
            addr[0]=top.memory_controller+memtroll^MULTI_PORT_MUX~8^MUX_2~541 \
            addr[1]=top.memory_controller+memtroll^MULTI_PORT_MUX~8^MUX_2~542 \
            addr[2]=top.memory_controller+memtroll^MULTI_PORT_MUX~8^MUX_2~543 \
            addr[3]=top.memory_controller+memtroll^MULTI_PORT_MUX~8^MUX_2~544 \
            addr[4]=top.memory_controller+memtroll^MULTI_PORT_MUX~8^MUX_2~545 \
            we=top.memory_controller+memtroll^MULTI_PORT_MUX~8^MUX_2~554 \
            out=top.memory_controller+memtroll.single_port_ram+str^out~0


#. Dummy nets with no sinks (output pins only)

    By default VPR sweeps away nets with no sinks (see :option:`vpr --sweep_dangling_nets`). As a result output pins can be left 'disconnected' by connecting them to dummy nets.

    For example:

    .. code-block:: none

        .names in dummy_net1
        0 1

    specifies an inverter with no connected output (provided ``dummy_net1`` is connected to no other pins).

    .. note:: This method requires that every disconnected output pin should be connected to a **uniquely named** dummy net.

BLIF File Format Example
~~~~~~~~~~~~~~~~~~~~~~~~
The following is an example BLIF file. It implements a 4-bit ripple-carry ``adder`` and some simple logic.

The main ``.model`` is named ``top``, and its input and output pins are listed using the ``.inputs`` and ``.outputs`` directives.

The 4-bit ripple-cary adder is built of 1-bit ``adder`` primitives which are instantiated using the ``.subckt`` directive.
Note that the adder primitive is defined as its own ``.model`` (which describes its pins), and is marked as ``.blackbox`` to indicate it is an architectural primitive.

The signal ``all_sum_high_comb`` is computed using combinational logic (``.names``) which ANDs all the sum bits together.

The ``.latch`` directive instantiates a rising-edge (``re``) latch (i.e. an edge-triggered Flip-Flop) clocked by ``clk``.
It takes in the combinational signal ``all_sum_high_comb`` and drives the primary output ``all_sum_high_reg``.

Also note that the last ``.subckt adder`` has it's ``cout`` output left implicitly disconnected.

.. code-block:: none

    .model top
    .inputs clk a[0] a[1] a[2] a[3] b[0] b[1] b[2] b[3]
    .outputs sum[0] sum[1] sum[2] sum[3] cout all_sum_high_reg

    .names gnd
     0

    .subckt adder a=a[0] b=b[0] cin=gnd    cout=cin[1]     sumout=sum[0]
    .subckt adder a=a[1] b=b[1] cin=cin[1] cout=cin[2]     sumout=sum[1]
    .subckt adder a=a[2] b=b[2] cin=cin[2] cout=cin[3]     sumout=sum[2]
    .subckt adder a=a[3] b=b[3] cin=cin[3]                 sumout=sum[3]

    .names sum[0] sum[1] sum[2] sum[3] all_sum_high_comb
    1111 1

    .latch all_sum_high_comb all_sum_high_reg re clk  0

    .end


    .model adder
    .inputs a b cin
    .outputs cout sumout
    .blackbox
    .end

.. _vpr_eblif_file:

Extended BLIF (.eblif)
----------------------
VPR also supports several extentions to :ref:`structural BLIF <vpr_blif_file>` to address some of its limitations.

.. note:: By default VPR assumes file with ``.eblif`` are in extneded BLIF format. The format can be controlled with :option:`vpr --circuit_format`.

.conn
~~~~~
The ``.conn`` statement allows direct connections between two wires.

For example:

.. code-block:: none

    .model top
    .input a
    .output b

    #Direct connection
    .conn a b

    .end

specifies that 'a' and 'b' are direct connected together.
This is analogous to Verilog's ``assign b = a;``.

This avoids the insertion of a ``.names`` buffer which is required in standard BLIF, for example:

.. code-block:: none

    .model top
    .input a
    .output b

    #Buffer LUT required in standard BLIF
    .names a b
    1 1

    .end


.cname
~~~~~~
The ``.cname`` statement allows names to be specified for BLIF primitives (e.g. ``.latch``, ``.names``, ``.subckt``).


.. note:: ``.cname`` statements apply to the previous primitive instantiation.

For example:

.. code-block:: none

    .names a b c
    11 1
    .cname my_and_gate

Would name of the above ``.names`` instance ``my_and_gate``.

.param
~~~~~~
The ``.param`` statement allows parameters (e.g. primitive modes) to be tagged on BLIF primitives.

.. note:: ``.param`` statements apply to the previous primitive instantiation.

For example:

.. code-block:: none

    .subckt dsp a=a_in b=b_in cin=c_in cout=c_out s=sum_out
    .param mode adder

Would set the parameter ``mode`` of the above ``dsp`` ``.subckt`` to ``adder``.

``.param`` statements propagate to ``<parameter>`` elements in the packed netlist.

.attr
~~~~~
The ``.attr`` statement allows attributes (e.g. source file/line) to be tagged on BLIF primitives.

.. note:: ``.attr`` statements apply to the previous primitive instantiation.

For example:

.. code-block:: none

    .latch a_and_b dff_q re clk 0
    .attr src my_design.v:42

Would set the attribute ``src`` of the above ``.latch`` to ``my_design.v:42``.

``.attr`` statements propagate to ``<attribute>`` elements in the packed netlist.

Extended BLIF File Format Example
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: none

    .model top
    .inputs a b clk
    .outputs o_dff

    .names a b a_and_b
    11 1
    .cname lut_a_and_b
    .param test_names_param "test_names_param_value"
    .attr test_names_attrib "test_names_param_attrib"

    .latch a_and_b dff_q re clk 0
    .cname my_dff
    .param test_latch_param "test_latch_param_value"
    .attr test_latch_attrib "test_latch_param_attrib"

    .conn dff_q o_dff

    .end

.. _vpr_sdc_file:

Timing Constraints (.sdc)
-------------------------
Timing constraints are specified using SDC syntax.
For a description of VPR's SDC support see :ref:`sdc_commands`.

.. note:: Use :option:`vpr --sdc_file` to specify the SDC file used by VPR.

Timing Constraints File Format Example
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
See :ref:`sdc_examples`.

.. _vpr_net_file:
.. _vpr_pack_file:

Packed Netlist Format (.net)
----------------------------
The circuit .net file is an xml file that describes a post-packed user circuit.
It represents the user netlist in terms of the complex logic blocks of the target architecture.
This file is generated from the packing stage and used as input to the placement stage in VPR.

The .net file is constructed hierarchically using ``block`` tags.
The top level ``block`` tag contains the I/Os and complex logic blocks used in the user circuit.
Each child ``block`` tag of this top level tag represents a single complex logic block inside the FPGA.
The ``block`` tags within a complex logic block tag describes, hierarchically, the clusters/modes/primitives used internally within that logic block.

A ``block`` tag has the following attributes:

 * ``name``
    A name to identify this component of the FPGA.
    This name can be completely arbitrary except in two situations.
    First, if this is a primitive (leaf) block that implements an atom in the input technology-mapped netlist (eg. LUT, FF, memory slice, etc), then the name of this block must match exactly with the name of the atom in that netlist so that one can later identify that mapping.
    Second, if this block is not used, then it should be named with the keyword open.
    In all other situations, the name is arbitrary.

 * ``instance``
    The phyiscal block in the FPGA architecture that the current block represents.
    Should be of format: architecture_instance_name[instance #].
    For example, the 5th index BLE in a CLB should have ``instance="ble[5]"``

 * ``mode``
    The mode the block is operating in.

A block connects to other blocks via pins which are organized based on a hierarchy.
All block tags contains the children tags: inputs, outputs, clocks.
Each of these tags in turn contain port tags.
Each port tag has an attribute name that matches with the name of a corresponding port in the FPGA architecture.
Within each port tag is a list of named connections where the first name corresponds to pin 0, the next to pin 1, and so forth.
The names of these connections use the following format:

#. Unused pins are identified with the keyword open.
#. The name of an input pin to a complex logic block is the same as the name of the net using that pin.
#. The name of an output pin of a primitve (leaf block) is the same as the name of the net using that pin.
#. The names of all other pins are specified by describing their immediate drivers.  This format is ``[name_of_immediate_driver_block].[port_name][pin#]->interconnect_name``.

For primitives with equivalent inputs VPR may rotate the input pins.
The resulting rotation is specified with the ``<port_rotation_map>`` tag.
For example, consider a netlist contains a 2-input LUT named ``c``, which is implemented in a 5-LUT:

.. code-block:: xml
    :caption: Example of ``<port_rotation_map>`` tag.
    :linenos:

    ...
    <block name="c" instance="lut[0]">
        <inputs>
            <port name="in">open open lut5.in[2]->direct:lut5  open lut5.in[4]->direct:lut5  </port>
            <port_rotation_map name="in">open open 1 open 0 </port_rotation_map>
        </inputs>
        <outputs>
            <port name="out">c </port>
        </outputs>
        <clocks>
        </clocks>
    </block>
    ...

In the original netlist the two LUT inputs were connected to pins at indicies 0 and 1 (the only input pins).
However during clustering the inputs were rotated, and those nets now connect to the pins at indicies 2 and 4 (line 4).
The ``<port_rotation_map>`` tag specified the port name it applies to (``name`` attribute), and its contents lists the pin indicies each pin in the port list is associated with in the original netlist (i.e. the pins ``lut5.in[2]->direct:lut5`` and ``lut5.in[4]->direct:lut5`` respectively correspond to indicies 1 and 0 in the original netlist).

.. note:: Use :option:`vpr --net_file` to override the default net file name.

Packing File Format Example
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following is an example of what a .net file would look like.
In this circuit there are 3 inputs (pa, pb, pc) and 4 outputs (out:pd, out:pe, out:pf, out:pg).
The io pad is set to inpad mode and is driven by the inpad:

.. code-block:: xml
    :caption: Example packed netlist file (trimmed for brevity).
    :linenos:

    <block name="b1.net" instance="FPGA_packed_netlist[0]">
        <inputs>
                pa pb pc
        </inputs>

        <outputs>
                out:pd out:pe out:pf out:pg
        </outputs>

        <clocks>
        </clocks>

        <block name="pa" instance="io[0]" mode="inpad">
                <inputs>
                        <port name="outpad">open </port>
                </inputs>

                <outputs>
                        <port name="inpad">inpad[0].inpad[0]->inpad  </port>
                </outputs>

                <clocks>
                        <port name="clock">open </port>
                </clocks>

                <block name="pa" instance="inpad[0]">
                        <inputs>
                        </inputs>

                        <outputs>
                                <port name="inpad">pa </port>
                        </outputs>

                        <clocks>
                        </clocks>

                        <attributes>
                                <attribute name="vccio">3.3</attribute>
                        </attributes>

                        <parameters>
                                <parameter name="iostandard">LVCMOS33</parameter>
                        </parameters>
                </block>
        </block>
    ...

.. _vpr_place_file:

Placement File Format (.place)
------------------------------
The first line of the placement file lists the netlist (.net) and architecture (.xml) files used to create this placement.
This information is used to ensure you are warned if you accidentally route this placement with a different architecture or netlist file later.
The second line of the file gives the size of the logic block array used by this placement.
All the following lines have the format::

    block_name    x        y   subblock_number

The ``block_name`` is the name of this block, as given in the input .net formatted netlist.
``x`` and ``y`` are the row and column in which the block is placed, respectively.

.. note:: The blocks in a placement file can be listed in any order.

The ``subblock number`` is meaningful only for I/O pads.
Since we can have more than one pad in a row or column when io_rat is set to be greater than 1 in the architecture file, the subblock number specifies which of the several possible pad locations in row x and column y contains this pad.
Note that the first pads occupied at some (x, y) location are always those with the lowest subblock numbers -- i.e. if only one pad at (x, y) is used, the subblock number of the I/O placed there will be zero.
For CLBs, the subblock number is always zero.

The placement files output by VPR also include (as a comment) a fifth field:  the block number.
This is the internal index used by VPR to identify a block -- it may be useful to know this index if you are modifying VPR and trying to debug something.

.. _fig_fpga_coord_system:

.. figure:: fpga_coordinate_system.*

    FPGA co-ordinate system.

:numref:`fig_fpga_coord_system` shows the coordinate system used by VPR for a small 2 x 2 CLB FPGA.
The number of CLBs in the x and y directions are denoted by ``nx`` and ``ny``, respectively.
CLBs all go in the area with x between ``1`` and ``nx`` and y between ``1`` and ``ny``, inclusive.
All pads either have x equal to ``0`` or ``nx + 1`` or y equal to ``0`` or ``ny + 1``.

.. note:: Use :option:`vpr --place_file` to override the default place file name.

Placement File Format Example
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
An example placement file is:

.. code-block:: none
    :caption: Example placement file.
    :linenos:

    Netlist file: xor5.net   Architecture file: sample.xml
    Array size: 2 x 2 logic blocks

    #block name x       y       subblk  block number
    #---------- --      --      ------- -----------
    a           0       1       0       #0  -- NB: block number is a comment.
    b           1       0       0       #1
    c           0       2       1       #2
    d           1       3       0       #3
    e           1       3       1       #4
    out:xor5    0       2       0       #5
    xor5        1       2       0       #6
    [1]         1       1       0       #7


.. _vpr_route_file:

Routing File Format (.route)
----------------------------
The first line of the routing file gives the array size, ``nx`` x ``ny``.
The remainder of the routing file lists the global or the detailed routing for each net, one by one.
Each routing begins with the word net, followed by the net index used internally by VPR to identify the net and, in brackets, the name of the net given in the netlist file.
The following lines define the routing of the net.
Each begins with a keyword that identifies a type of routing segment.
The possible keywords are ``SOURCE`` (the source of a certain output pin class), ``SINK`` (the sink of a certain input pin class), ``OPIN`` (output pin), ``IPIN`` (input pin), ``CHANX`` (horizontal channel), and ``CHANY`` (vertical channel).
Each routing begins on a ``SOURCE`` and ends on a ``SINK``.
In brackets after the keyword is the (x, y) location of this routing resource.
Finally, the pad number (if the ``SOURCE``, ``SINK``, ``IPIN`` or ``OPIN`` was on an I/O pad), pin number (if the ``IPIN`` or ``OPIN`` was on a clb), class number (if the ``SOURCE`` or ``SINK`` was on a clb) or track number (for ``CHANX`` or ``CHANY``) is listed -- whichever one is appropriate.
The meaning of these numbers should be fairly obvious in each case.
If we are attaching to a pad, the pad number given for a resource is the subblock number defining to which pad at location (x, y) we are attached.
See :numref:`fig_fpga_coord_system` for a diagram of the coordinate system used by VPR.
In a horizontal channel (``CHANX``) track ``0`` is the bottommost track, while in a vertical channel (``CHANY``) track ``0`` is the leftmost track.
Note that if only global routing was performed the track number for each of the ``CHANX`` and ``CHANY`` resources listed in the routing will be ``0``, as global routing does not assign tracks to the various nets.

For an N-pin net, we need N-1 distinct wiring “paths” to connect all the pins.
The first wiring path will always go from a ``SOURCE`` to a ``SINK``.
The routing segment listed immediately after the ``SINK`` is the part of the existing routing to which the new path attaches.

.. note:: It is important to realize that the first pin after a ``SINK`` is the connection into the already specified routing tree; when computing routing statistics be sure that you do not count the same segment several times by ignoring this fact.

.. note:: Use :option:`vpr --route_file` to override the default route file name.

Routing File Format Examples
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
An example routing for one net is listed below:

.. code-block:: none
    :caption: Example routing for a non-global net.
    :linenos:

    Net 5 (xor5)

    Node:  1   SOURCE (1,2)  Class: 1  Switch: 1       # Source for pins of class 1.
    Node:  2   OPIN (1,2)    Pin: 4    clb.O[12]  Switch:0   #Output pin the O port of clb block, pin number 12
    Node:  4   CHANX (1,1) to (4,1)  Track: 1  Switch: 1
    Node:  6   CHANX (4,1) to (7,1)  Track: 1  Switch: 1
    Node:  8   IPIN (7,1)  Pin: 0  clb.I[0]  Switch: 2
    Node:  9   SINK (7,1)  Class: 0  Switch: -1      # Sink for pins of class 0 on a clb.
    Node:  4   CHANX (7,1) to (10,1)  Track: 1  Switch: 1      # Note:  Connection to existing routing!
    Node:  5   CHANY (10,1) to (10,4)  Track: 1  Switch: 0
    Node:  4   CHANX (10,4) to (13,4)  Track: 1  Switch: 1
    Node:  10  CHANX (13,4) to (16,4)  Track: 1  Switch: 1
    Node:  11  IPIN (16,4)  Pad: 1  clb.I[1]  Switch: 2
    Node:  12  SINK (16,4)  Pad: 1  Switch: -1      # This sink is an output pad at (16,4), subblock 1.


Nets which are specified to be global in the netlist file (generally clocks) are not routed.
Instead, a list of the blocks (name and internal index) which this net must connect is printed out.
The location of each block and the class of the pin to which the net must connect at each block is also printed.
For clbs, the class is simply whatever class was specified for that pin in the architecture input file.
For pads the pinclass is always -1; since pads do not have logically-equivalent pins, pin classes are not needed.
An example listing for a global net is given below.

.. code-block:: none
    :caption: Example routing for a global net.
    :linenos:

    Net 146 (pclk): global net connecting:
    Block pclk (#146) at (1,0), pinclass -1
    Block pksi_17_ (#431) at (3,26), pinclass 2
    Block pksi_185_ (#432) at (5,48), pinclass 2
    Block n_n2879 (#433) at (49,23), pinclass 2

.. _vpr_route_resource_file:

Routing Resource Graph File Format (.xml)
-----------------------------------------
The routing resource graph (rr graph) file is an XML file that describes the routing resources within the FPGA.
This file is generated through the last stage of the rr graph generation during routing with the final channel width.
When reading in rr graph from an external file, the rr graph is used during the placement and routing section of VPR.
The file is constructed using tags. The top level is the ``rr_graph`` tag.
This tag contains all the channel, switches, segments, block, grid, node, and edge information of the FPGA.
It is important to keep all the values as high precision as possible. Sensitive values include capacitance and Tdel. As default, these values are printed out with a precision of 30 digits.
Each of these sections are separated into separate tags as described below.

.. note:: Use :option:`vpr --read_rr_graph` to specify an RR graph file to be load.
.. note:: Use :option:`vpr --write_rr_graph` to specify where the RR graph should be written.

Top Level Tags
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The first tag in all rr graph files is the ``<rr_graph>`` tag that contains detailed subtags for each catagory in the rr graph.
Each tag has their subsequent subtags that describes one entity. For example, ``<segments>`` includes all the segments in the graph where each ``<segment>`` tag outlines one type of segment.

The ``rr_graph`` tag contains the following tags:

* ``<channels>``
        * ``<channel>``content``</channel>``
* ``<switches>``
        * ``<switch>``content``</switch>``
* ``<segments>``
        * ``<segment>``content``</segment>``
* ``<block_types>``
        * ``<block_type>``content``</block_type>``
* ``<grid>``
        * ``<grid_loc>``content``</grid_loc>``
* ``<rr_nodes>``
        * ``<node>``content``</node>``
* ``<rr_edges>``
        * ``<edge>``content``</edge>``

.. note:: The rr graph is based on the architecture, so more detailed description of each section of the rr graph can be found at :ref:`FPGA architecture description <fpga_architecture_description>`

Detailed Tag Information
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Channel
^^^^^^^

The channel information is contained within the ``channels`` subtag. This describes the minimum and maximum channel width within the architecture. Each ``channels`` tag has the following subtags:

.. rrgraph:tag:: <channel chan_width_max="int" x_min="int" y_min="int" x_max="int" y_max="int"/>

    This is a required subtag that contains information about the general channel width information. This stores the channel width between x or y directed channels.

    :req_param chan_width_max:
        Stores the maximum channel width value of x or y channels.

    :req_param x_min y_min x_max y_max:
        Stores the minimum and maximum value of x and y coordinate within the lists.

.. rrgraph:tag:: <x_list index="int" info="int"/>  <y_list index="int" info="int"/>

        These are a required subtags that lists the contents of an x_list and y_list array which stores the width of each channel. The x_list array size as large as the size of the y dimension of the FPGA itself while the y_list has the size of the x_dimension. This x_list tag is repeated for each index within the array.

    :req_param index:
        Describes the index within the array.

    :req_param info:
        The width of each channel. The minimum is one track per channel.
        The input and output channels are io_rat * maximum in interior tracks wide.
        The channel distributions read from the architecture file are scaled by a constant factor.

Switches
^^^^^^^^

A ``switches`` tag contains all the switches and its information within the FPGA. It should be noted that for values such as capacitance, Tdel, and sizing info all have high precision. This ensures a more accurate calculation when reading in the routing resource graph. Each switch tag has a ``switch`` subtag.

.. rrgraph:tag:: <switch id="int" name="unique_identifier" type="{mux|tristate|pass_gate|short|buffer}">

    :req_param id:
        A unique identifier for that type of switch.

    :req_param name:
        An optional general identifier for the switch.

    :req_param type:
        See :ref:`architecture switch description <arch_switches>`.

.. rrgraph:tag:: <timing R="float" cin="float" Cout="float" Tdel="float/>

        This optional subtag contains information used for timing analysis. Without it, the program assums all subtags to contain a value of 0.

    :opt_param R, Cin, Cout:
        The resistance, input capacitance and output capacitance of the switch.

    :opt_param Tdel:
        Switch's intrinsic delay. It can be outlined that the delay through an unloaded switch is Tdel + R * Cout.

.. rrgraph:tag:: <sizing mux_trans_size="int" buf_size="float"/>

        The sizing information contains all the information needed for area calculation.

    :req_param mux_trans_size:
        The area of each transistor in the segment's driving mux. This is measured in minimum width transistor units.

    :req_param buf_size:
        The area of the buffer. If this is set to zero, the area is calculated from the resistance.

Segments
^^^^^^^^

The ``segments`` tag contains all the segments and its information. Note again that the capacitance has a high decimal precision. Each segment is then enclosed in its own ``segment`` tag.

.. rrgraph:tag:: <segment id="int" name="unique_identifier">

    :req_param id:
        The index of this segment.

    :req_param name:
        The name of this segment.

.. rrgraph:tag:: <timing R_per_meter="float" C_per_meter="float">

        This optional tag defines the timing information of this segment.

    :opt_param R_per_meter, C_per_meter:
        The resistance and capacitance of a routing track, per unit logic block length.

Blocks
^^^^^^

The ``block_types`` tag outlines the information of a placeable complex logic block. This includes generation, pin classes, and pins within each block. Information here is checked to make sure it corresponds with the architecture. It contains the following subtags:

.. rrgraph:tag:: <block_type id="int" name="unique_identifier" width="int" height="int">

        This describes generation information about the block using the following attributes:

    :req_param id:
        The index of the type of the descriptor in the array. This is used for index referencing

    :req_param name:
        A unique identifier for this type of block. Note that an empty block type must be denoted ``"EMPTY"`` without the brackets ``<>`` to prevent breaking the xml format. Input and output blocks must be named "io". Other blocks can have any name.

    :req_param width, height:
        The width and height of a large block in grid tiles.

.. rrgraph:tag:: <pin_class type="pin_type">

        This optional subtag of ``block_type`` describes groups of pins in configurable logic blocks that share common properties.

    :req_param type:
        This describes whether the pin class is a driver or receiver. Valid inputs are ``OPEN``, ``OUTPUT``, and ``INPUT``.

.. rrgraph:tag:: <pin ptc="block_pin_index">name</pin>

        This required subtag of ``pin_class`` describes its pins.

    :req_param ptc:
        The index of the pin within the ``block_type``.

    :req_param name:
        Human readable pin name.

Grid
^^^^

The ``grid`` tag contains information about the grid of the FPGA. Information here is checked to make sure it corresponds with the architecture. Each grid tag has one subtag as outlined below:

.. rrgraph:tag:: <grid_loc x="int" y="int" block_type_id="int" width_offset="int" height_offset="int">

    :req_param x, y:
        The x and y  coordinate location of this grid tile.

    :req_param block_type_id:
        The index of the type of logic block that resides here.

    :req_param width_offset, height_offset:
        The number of grid tiles reserved based on the width and height of a block.

Nodes
^^^^^

The ``rr_nodes`` tag stores information about each node for the routing resource graph. These nodes describe each wire and each logic block pin as represented by nodes.

.. rrgraph:tag:: <node id="int" type="unique_type" direction="unique_direction" capacity="int">

    :req_param id:
        The index of the particular routing resource node

    :req_param type:
        Indicates whether the node is a wire or a logic block.
        Valid inputs for class types are { ``CHANX`` | ``CHANY`` | ``SOURCE`` | ``SINK`` | ``OPIN`` | ``IPIN`` }.
        Where ``CHANX`` and ``CHANY`` describe a horizontal and vertical channel.
        Sources and sinks describes where nets begin and end.
        ``OPIN`` represents an output pin and ``IPIN`` representd an input pin

    :opt_param direction:
        If the node represents a track (``CHANX`` or ``CHANY``), this field represents its direction as {``INC_DIR`` | ``DEC_DIR`` | ``BI_DIR``}.
        In other cases this attribute should not be specified.

    :req_param capacity:
        The number of routes that can use this node.

.. rrgraph:tag:: <loc xlow="int" ylow="int" xhigh="int" yhigh="int" side="{LEFT|RIGHT|TOP|BOTTOM}" ptc="int">

    Contains location information for this node. For pins or segments of length one, xlow = xhigh and ylow = yhigh.

    :req_param xlow, xhigh, ylow, yhigh:
        Integer coordinates of the ends of this routing source.

    :opt_param side:
        For ``IPIN`` and ``OPIN`` nodes specifies the side of the grid tile on which the node is located.
        Valid values are { ``LEFT`` | ``RIGHT`` | ``TOP`` | ``BOTTOM`` }.
        In other cases this attribute should not be specified.

    :req_param ptc:
        This is the pin, track, or class number that depends on the rr_node type.

.. rrgraph:tag:: <timing R="float" C="float">

    This optional subtag contains information used for timing analysis

    :req_param R:
        The resistance that goes through this node. This is only the metal resistance, it does not include the resistance of the switch that leads to another routing resource node.

    :req_param C:
        The total capacitance of this node. This includes the metal capacitance, input capacitance of all the switches hanging off the node, the output capacitance of all the switches to the node, and the connection box buffer capacitances that hangs off it.

.. rrgraph:tag:: <segment segment_id="int">

      This optional subtag describes the information of the segment that connects to the node.

    :req_param segment_id:
        This describes the index of the segment type. This value only applies to horizontal and vertical channel types. It can be left empty, or as -1 for other types of nodes.

Edges
^^^^^

The final subtag is the ``rr_edges`` tag that encloses information about all the edges between nodes. Each ``rr_edges`` tag contains multiple subtags:

.. rrgraph:tag:: <edge src_node="int" sink_node="int" switch_id="int"/>

    This subtag repeats every edge that connects nodes together in the graph.

    :req_param src_node, sink_node:
        The index for the source and sink node that this edge connects to.

    :req_param switch_id:
        The type of switch that connects the two nodes.

Node and Edge Metadata
^^^^^^^^^^^^^^^^^^^^^^

``metadata`` blocks (see :ref:`arch_metadata`) are supported under both ``node`` and ``edge`` tags.


Routing Resource Graph Format Example
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

An example of what a generated routing resource graph file would look like is shown below:

.. code-block:: xml
    :caption: Example of a routing resource graph in XML format
    :linenos:

    <rr_graph tool_name="vpr" tool_version="82a3c72" tool_comment="Based on my_arch.xml">
        <channels>
            <channel chan_width_max="2" x_min="2" y_min="2" x_max="2" y_max="2"/>
            <x_list index="1" info="5"/>
            <x_list index="2" info="5"/>
            <y_list index="1" info="5"/>
            <y_list index="2" info="5"/>
        </channels>
        <switches>
            <switch id="0" name="my_switch" buffered="1">
                <timing R="100" Cin="1233-12" Cout="123e-12" Tdel="1e-9"/>
                <sizing mux_trans_size="2.32" buf_size="23.54"/>
            </switch>
        </switches>
        <segments>
            <segment id="0" name="L4">
                <timing R_per_meter="201.7" C_per_meter="18.110e-15"/>
            </segment>
        </segments>
        <block_types>
            <block_type id="0" name="io" width="1" height="1">
                <pin_class type="input">
                    <pin ptc="0">DATIN[0]</pin>
                    <pin ptc="1">DATIN[1]</pin>
                    <pin ptc="2">DATIN[2]</pin>
                    <pin ptc="3">DATIN[3]</pin>
                </pin_class>
                <pin_class type="output">
                    <pin ptc="4">DATOUT[0]</pin>
                    <pin ptc="5">DATOUT[1]</pin>
                    <pin ptc="6">DATOUT[2]</pin>
                    <pin ptc="7">DATOUT[3]</pin>
                </pin_class>
            </block_type>
            <block_type id="1" name="buf" width="1" height="1">
                <pin_class type="input">
                    <pin ptc="0">IN</pin>
                </pin_class>
                <pin_class type="output">
                    <pin ptc="1">OUT</pin>
                </pin_class>
            </block_type>
        </block_types>
        <grid>
            <grid_loc x="0" y="0" block_type_id="0" width_offset="0" height_offset="0"/>
            <grid_loc x="1" y="0" block_type_id="1" width_offset="0" height_offset="0"/>
        </grid>
        <rr_nodes>
            <node id="0" type="SOURCE" direction="NONE" capacity="1">
                <loc xlow="0" ylow="0" xhigh="0" yhigh="0" ptc="0"/>
                <timing R="0" C="0"/>
            </node>
            <node id="1" type="CHANX" direction="INC" capacity="1">
                <loc xlow="0" ylow="0" xhigh="2" yhigh="0" ptc="0"/>
                <timing R="100" C="12e-12"/>
                <segment segment_id="0"/>
            </node>
        </rr_nodes>
        <rr_edges>
            <edge src_node="0" sink_node="1" switch_id="0"/>
            <edge src_node="1" sink_node="2" switch_id="0"/>
        </rr_edges>
    </rr_graph>
.. _end:
