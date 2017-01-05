.. _vpr_file_formats:

File Formats
============
VPR consumes and produces several files representing the packing, placement, and routing results.

FPGA Architecture (.xml)
--------------------------
The target FPGA architecture is specified as an architecture file.
For details of this file format see :ref:`fpga_architecture_description`.

.. _vpr_blif_file:

BLIF Netlist (.blif)
--------------------------
The technology mapped circuit to be implement on the target FPGA is specified as a Berkely Logic Interchange Format (BLIF) netlist.
The netlist must be flattened and consist of only primitives (e.g. ``.names``, ``.latch``, ``.subckt``).

For a detailed description of the BLIF file format see the :download:`BLIF Format Description <../../../libblifparse/doc/blif.pdf>`.

Note that VPR supports only the structural subset of BLIF, and does not support the following BLIF features:

 * Subfile References (``.search``).
 * Finite State Machine Descriptions (``.start_kiss``, ``.end_kiss`` etc.).
 * Clock Constraints (``.cycle``, ``.clock_event``).
 * Delay Constraints (``.delay`` etc.). 

Clock and delay constraints can be specified with an :ref:`SDC File <vpr_sdc_file>`.

Black Box Primitives
~~~~~~~~~~~~~~~~~~~~
Black-box architectural primitives (RAMs, Multipliers etc.) should be instantiated in the netlist using BLIF's ``.subckt`` directive.
The BLIF file should also contain a black-box ``.model`` definition which defines the input and outputs of each ``.subckt`` type.

VPR will check that blackbox ``.model``\s are consistent with the :ref:`<models> section <arch_blif_models>` of the architecture file.

Unconnected Nets
~~~~~~~~~~~~~~~~
To specify unconnected pins on primitives VPR supports two syntaxes:

#. The ``unconn`` net.

    VPR treats any pin connected to a net named ``unconn`` as disconnected.
    
    For example:

    .. code-block:: none

        .names a unconn
        0 1

    specifies an inverter with no connected output.


#. Implicitly disconnected ``.subckt`` ports.

    For ``.subckt`` instantiations VPR treats primitive pins which are not listed as being implicitly disconnected.

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

BLIF File Format Example
~~~~~~~~~~~~~~~~~~~~~~~~
The following is an example BLIF file. It implements a 4-bit ripple-carry `adder` and some simple logic.

The main `.model` is named `top`, and its input and output pins are listed using the `.inputs` and `.outputs` directives. 

The 4-bit ripple-cary adder is built of 1-bit `adder` primitives which are instantiated using the `.subckt` directive.
Note that the adder primitive is defined as its own `.model` (which describes its pins), and is marked as `.blackbox` to indicate it is an architectural primitive.

The signal `all_sum_high_comb` is computed using combinational logic (`.names`) which ANDs all the sum bits together.

The `.latch` directive instantiates a rising-edge (`re`) latch (i.e. an edge-triggered Flip-Flop) clocked by `clk`.
It takes in the combinational signal `all_sum_high_comb` and drives the primary output `all_sum_high_reg`.

Also not that the last `.subckt adder` has it's `cout` output left disconnected using the special `unconn` net.

.. code-block:: none

    .model top
    .inputs clk a[0] a[1] a[2] a[3] b[0] b[1] b[2] b[3]
    .outputs sum[0] sum[1] sum[2] sum[3] cout all_sum_high_reg

    .names gnd
     0
    .names unconn
     0

    .subckt adder a=a[0] b=b[0] cin=gnd    cout=cin[1] sumout=sum[0]
    .subckt adder a=a[1] b=b[1] cin=cin[1] cout=cin[2] sumout=sum[1]
    .subckt adder a=a[2] b=b[2] cin=cin[2] cout=cin[3] sumout=sum[2]
    .subckt adder a=a[3] b=b[3] cin=cin[3] cout=unconn sumout=sum[3]

    .names sum[0] sum[1] sum[2] sum[3] all_sum_high_comb
    1111 1

    .latch all_sum_high_comb all_sum_high_reg re clk  0

    .end


    .model adder
    .inputs a b cin
    .outputs cout sumout
    .blackbox
    .end

.. _vpr_sdc_file:

Timing Constraints (.sdc)
-------------------------
Timing constraints are specified using SDC syntax.
For a description of VPR's SDC support see :ref:`sdc_commands`.

.. note:: Use :option:`vpr -sdc_file` to specify the SDC file used by VPR.

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

.. note:: Use :option:`vpr -net_file` to override the default net file name.

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

.. note:: Use :option:`vpr -place_file` to override the default place file name.

Placement File Format Example
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
An example placement file is:

.. code-block:: none
    :caption: Example placement file.
    :linenos:

    Netlist file: xor5.net   Architecture file: sample.xml
    Array size: 2 x 2 logic blocks

    #block name	x	y	subblk	block number
    #----------	--	--	------	------------
    a		    0	1	0		#0  -- NB: block number is a comment.
    b		    1	0	0		#1
    c		    0	2	1		#2
    d		    1	3	0		#3
    e		    1	3	1		#4
    out:xor5	0	2	0		#5
    xor5		1	2	0		#6
    [1]		    1	1	0		#7


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

.. note:: Use :option:`vpr -route_file` to override the default place file name.

Routing File Format Examples
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
An example routing for one net is listed below:

.. code-block:: none
    :caption: Example routing for a non-global net.
    :linenos:

    Net 5 (xor5)

    SOURCE (1,2)  Class: 1        # Source for pins of class 1.
     OPIN (1,2)  Pin: 4
     CHANX (1,1)  Track: 1
     CHANX (2,1)  Track: 1
     IPIN (2,2)  Pin: 0
     SINK (2,2)  Class: 0        # Sink for pins of class 0 on a clb.
     CHANX (1,1)  Track: 1        # Note:  Connection to existing routing!
     CHANY (1,2)  Track: 1
     CHANX (2,2)  Track: 1
     CHANX (1,2)  Track: 1
     IPIN (1,3)  Pad: 1
     SINK (1,3)  Pad: 1      # This sink is an output pad at (1,3), subblock 1.


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
    Block pclk (#146) at (1, 0), pinclass -1.
    Block pksi_17_ (#431) at (3, 26), pinclass 2.
    Block pksi_185_ (#432) at (5, 48), pinclass 2.
    Block n_n2879 (#433) at (49, 23), pinclass 2.

