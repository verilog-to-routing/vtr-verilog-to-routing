Basic flow
==========

The Place and Route process in VPR consists of a few steps:

- Packing (combining primitives into complex logic blocks)
- Placing (placement of complex block inside FPGA)
- Routing (planning interconnections between blocks)

Each of these steps provides additional configuration options that can be used
for customizing the whole process.

Packing
-------

The packing algorithm tries to combine primitive logic blocks into groups,
called Complex Logic Blocks. The results from the packing process are written
into a ``.net`` file. It contains a description of complex blocks with their
inputs, outputs, used clocks and relations to other signals.
It can be useful in analyzing how VPR packs primitives together.

A detailed description of the ``.net`` file format can be found in the :ref:`vpr_pack_file` section.

Placing
-------

This step assigns a location to the Complex Logic Block onto the FPGA.
The output from this step is written to the ``.place`` file, which contains
the physical location of the blocks from the ``.net`` file.

The file has the following format:

.. code-block:: bash

    block_name    x        y   subblock_number

where ``x`` and ``y`` are positions in the VPR grid and ``block_name`` comes from
the ``.net`` file.

Example of a placing file:

.. code-block::

    Netlist_File: top.net Netlist_ID: SHA256:ce5217d251e04301759ee5a8f55f67c642de435b6c573148b67c19c5e054c1f9
    Array size: 149 x 158 logic blocks

    #block name	x	y	subblk	block number
    #----------	--	--	------	------------
    $auto$alumacc.cc:474:replace_alu$24.slice[1].carry4_full	53	32	0	#0
    $auto$alumacc.cc:474:replace_alu$24.slice[2].carry4_full	53	31	0	#1
    $auto$alumacc.cc:474:replace_alu$24.slice[3].carry4_full	53	30	0	#2
    $auto$alumacc.cc:474:replace_alu$24.slice[4].carry4_full	53	29	0	#3
    $auto$alumacc.cc:474:replace_alu$24.slice[5].carry4_full	53	28	0	#4
    $auto$alumacc.cc:474:replace_alu$24.slice[6].carry4_part	53	27	0	#5
    $auto$alumacc.cc:474:replace_alu$24.slice[0].carry4_1st_full	53	33	0	#6
    out:LD7		9	5	0	#7
    clk		42	26	0	#8
    $false		35	26	0	#9

A detailed description of the ``.place`` file format can be found in the :ref:`vpr_place_file` section.

Routing
-------

This step connects the placed Complex Logic Blocks together,
according to the netlist specifications and the routing resources
of the FPGA chip. The description of the routing resources is
provided in the :ref:`architecture definition file
<arch_reference>`.
Starting from the architecture definition, VPR generates the Resource
Routing Graph. SymbiFlow provides a complete graph file for each architecture.
This ``precompiled`` file can be directly injected into the routing process.
The output from this step is written into ``.route`` file.

The file describes each connection from input to its output through
different routing resources of the FPGA.
Each net starts with a ``SOURCE`` node and ends in a ``SINK`` node.
The node name describes the kind of routing resource.
The pair of numbers in round brackets provides information on the (x, y)
resource location on the VPR grid. The additional field provides information
for a specific kind of node.

An example routing file could look as follows:

.. code-block::

    Placement_File: top.place Placement_ID: SHA256:88d45f0bf7999e3f9331cdfd3497d0028be58ffa324a019254c2ae7b4f5bfa7a
    Array size: 149 x 158 logic blocks.

    Routing:

    Net 0 (counter[4])

    Node:	203972	SOURCE (53,32)  Class: 40  Switch: 0
    Node:	204095	  OPIN (53,32)  Pin: 40   BLK-TL-SLICEL.CQ[0] Switch: 189
    Node:	1027363	 CHANY (52,32)  Track: 165  Switch: 7
    Node:	601704	 CHANY (52,32)  Track: 240  Switch: 161
    Node:	955959	 CHANY (52,32) to (52,33)  Track: 90  Switch: 130
    Node:	955968	 CHANY (52,32)  Track: 238  Switch: 128
    Node:	955976	 CHANY (52,32)  Track: 230  Switch: 131
    Node:	601648	 CHANY (52,32)  Track: 268  Switch: 7
    Node:	1027319	 CHANY (52,32)  Track: 191  Switch: 183
    Node:	203982	  IPIN (53,32)  Pin: 1   BLK-TL-SLICEL.A2[0] Switch: 0
    Node:	203933	  SINK (53,32)  Class: 1  Switch: -1

   Net 1 ($auto$alumacc.cc:474:replace_alu$24.O[6])
   ...

A detailed description of the ``.route`` file format can be found in the :ref:`vpr_route_file` section.
