VPR Placement Constraints
=========================

VPR supports running flows with placement constraints. Placement constraints are set on primitives to lock them down to specified regions on the FPGA chip. For example, a user may use placement constraints to lock down pins to specific locations on the chip. Also, groups of primitives may be locked down to regions on the chip in CAD flows that use floorplanning or modular design, or to hand-place a timing critical piece.

The placement constraints should be specified by the user using an XML constraints file format, as described in the section below. When VPR is run with placement constraints, both the packing and placement flows are performed in such a way that the constraints are respected. The packing stage does not pack any primitives together that have conflicting floorplan constraints. The placement stage considers the floorplan constraints when choosing a location for each clustered block during initial placement, and does not move any block outside of its constraint boundaries during place moves.

A Constraints File Example
--------------------------

.. code-block:: xml
:caption: An example of a placement constraints file in XML format.
:linenos:

<vpr_constraints tool_name="vpr">
     <partition_list>
	  <partition name="Part0">
	       <add_atom name_pattern="li354">
	       <add_atom name_pattern="alu*"> <!-- Regular expressions can be used to provide name patterns of the primitives to be added -->
	       <add_atom name_pattern="n877">
	       <add_region x_low="3" y_low="1" x_high="7" y_high="2"> <!-- Two rectangular regions are specified, together describing an L-shaped region -->
	       <add_region x_low="7" y_low="3" x_high="7" y_high="6"
	  </partition>
	  <partition name="Part1">
	       <add_region x_low="3" y_low="3" x_high="7" y_high="7" subtile="0"> <!-- One specific location is specified -->
	       <add_atom name_pattern="n4917">
	       <add_atom name_pattern="n6010">
	  </partition>
     </partition_list>
</vpr_constraints>

.. _end:

Constraints File Format
-----------------------

VPR has a specific XML format which must be used when creating a placement constraints file. The purpose of this constraints file is to specify 

#. Which primitives are to have placement constraints
#. The regions on the FPGA chip to which those primitives must be constrained

The file is passed as an input to VPR when running with placement constraints. When the file is read in, its information is used during the packing and placement stages of VPR. The hierarchy of the file is set up as follows.

.. note:: Use the VPR :option: `--read_vpr_constraints` to specify the VPR placement constraints file that is to be loaded. 

The top level tag is the ``<vpr_constraints>`` tag. This tag contains one ``<partition_list>`` tag. The ``<partition_list>`` tag can be made up of an unbounded number of ``<partition>`` tags. The ``<partition>`` tags contains all of the detailed information of the placement constraints, and is described in detail below.

Partitions, Atoms, and Regions
------------------------------

Partition
^^^^^^^^^

A partition is made up of two components - a group of primitives (a.k.a. atoms) that must be constrained to the same area on the chip, and a set of one or more regions specifying where those primitives must be constrained. The information for each partition is contained within a ``<partition>`` tag, and the number of ``partition`` tags that the partition_list tag can contain is unbounded. 

:req_param name:
   A name for the partition.

:req_param add_atom:
   A tag used for adding an atom primitive to the partition.

:req_param add_region:
   A tag used for specifying a region for the partition.

Atom 
^^^^

An ``<add_atom>`` tag is used to add an atom that must be constrained to the partition. Each partition can contain any number of atoms from the circuit. The ``<add_atom>`` tag has the following attribute:

:req_param name_pattern:
   The name of the atom.

The ``name_pattern`` can be the exact name of the atom from the input atom netlist that was passed to VPR. It can also be a regular expression, in which case VPR will add all atoms from the netlist which have a portion of their name matching the regular expression to the partition. For example, if a module contains primitives named in the pattern of "alu[0]", "alu[1]", and "alu[2]", the regular expression "alu*" would add all of the primitives from that module.

Region
^^^^^^

An ``<add_region>`` tag is used to add a region to the partition. A ``region`` is a rectangular area on the chip. A partition can contain any number of independent regions - the regions within one partition must not overlap with each other (in order to ease processing when loading in the file). An ``<add_region>`` tag has the following attributes.

:req_param x_low:
   The x value of the lower left point of the rectangle.

:req_param y_low:
   The y value of the lower left point of the rectangle.

:req_param x_high:
   The x value of the upper right point of the rectangle.

:req_param y_high:
   The y value of the upper right point of the rectangle.

:opt_param subtile:
   Each x, y location on the grid may contain multiple locations known as subtiles. This paramter is an optional value specifying the subtile location that the atom(s) of the partition shall be constrained to.

The optional ``subtile`` attribute is commonly used when constraining an atom to a specific location on the chip (e.g. an exact I/O location). It is legal to use with larger regions, but uncommon.

If a user would like to specify an area on the chip with an unusual shape (e.g. L-shaped or T-shaped), they can simply add multiple ``<add_region>`` tags to cover the area specified.








