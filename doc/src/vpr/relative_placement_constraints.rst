
Relative Placement Constraints
==============================
.. _relative_placement_constraints:

VPR supports *relative placement macros*: constraints that fix the placement of groups of primitives **relative to each other**, without pinning them to absolute locations on the chip.
Each macro consists of one *reference group* and one or more *relative groups* of primitives (a.k.a. atoms).
The constraints affect both packing and placement:

* **Packing:** the atoms of each group are packed together into a single cluster, and atoms of different groups (of any macro) are never packed into the same cluster. Unconstrained atoms may share a cluster with a group.
* **Placement:** the clusters created from the groups of one macro form a placement macro (like an architectural carry chain): each relative group's cluster is placed at exactly its ``(x_offset, y_offset, sub_tile_offset)`` from the reference group's cluster, and the whole macro moves as a rigid unit during initial placement and simulated annealing. The placer chooses *where* the macro goes; the constraints fix only the internal geometry.

Relative placement constraints are specified inside the same XML constraints file as :ref:`placement (region) constraints <placement_constraints>` and are read with :option:`vpr --read_vpr_constraints`.
They compose with region constraints: an atom may belong to both a partition and a relative group, in which case the macro is placed such that the region constraints of all its members are satisfied.

A Relative Placement Constraints File Example
---------------------------------------------

.. code-block:: xml
	:caption: An example of relative placement macros in a VPR constraints file.
	:linenos:

	<vpr_constraints tool_name="vpr">
		<relative_macro_list>
			<relative_macro name="dsp_with_ctrl_logic">
				<reference_group>
					<add_atom name_pattern="mult_36.out_reg"/> <!-- The anchor of the macro -->
				</reference_group>
				<relative_group x_offset="1" y_offset="0" sub_tile_offset="0">
					<add_atom name_pattern="^ctrl_lut_[0-4]$" is_regex="true"/> <!-- These LUTs are packed into one cluster, placed directly right of the anchor -->
				</relative_group>
			</relative_macro>
			<relative_macro name="bit_sliced_datapath">
				<reference_group>
					<add_atom name_pattern="slice0_.*" is_regex="true"/>
				</reference_group>
				<relative_group x_offset="0" y_offset="1" sub_tile_offset="0">
					<add_atom name_pattern="slice1_.*" is_regex="true"/>
				</relative_group>
				<relative_group x_offset="0" y_offset="2" sub_tile_offset="0">
					<add_atom name_pattern="slice2_.*" is_regex="true"/>
				</relative_group>
			</relative_macro>
		</relative_macro_list>
	</vpr_constraints>

.. _end:

Relative Placement Constraints File Format
------------------------------------------

The ``<vpr_constraints>`` top-level tag may contain one ``<relative_macro_list>`` tag, which contains an unbounded number of ``<relative_macro>`` tags.

.. arch:tag:: <relative_macro name="string">

	A relative macro is made up of exactly one ``<reference_group>`` (which must come first) followed by one or more ``<relative_group>`` tags.

	:req_param name:
		A unique name for the macro, used in log and error messages.

	.. arch:tag:: <reference_group>

		The anchor of the macro. The atoms added to this group are packed into one cluster, and all relative groups' offsets are measured from this cluster's location. Contains one or more ``<add_atom>`` tags (same syntax and matching semantics as in a ``<partition>``, see :ref:`placement_constraints`; the ``logical_block_location`` attribute is not supported here and is ignored with a warning).

	.. arch:tag:: <relative_group x_offset="int" y_offset="int" sub_tile_offset="int" layer_offset="int">

		A group of atoms packed into one cluster, placed at the given offset from the reference group's cluster. Contains one or more ``<add_atom>`` tags.

		:req_param x_offset:
			Grid-tile x offset of this group's cluster from the reference cluster. May be negative.

		:req_param y_offset:
			Grid-tile y offset of this group's cluster from the reference cluster. May be negative.

		:req_param sub_tile_offset:
			Sub-tile offset of this group's cluster from the reference cluster. Use ``0`` when the tiles have a single sub-tile. This attribute is currently required; a future extension will allow omitting it to mean "any compatible sub-tile".

		:opt_param layer_offset:
			Layer (die) offset. Must currently be ``0`` (cross-layer relative macros are not supported and are rejected when the file is loaded).
			**Default:** ``0``

Semantics and Restrictions
--------------------------

* **One group, one cluster.** All atoms of a group must fit into a single cluster; this is enforced. If a group cannot be packed into one cluster (e.g. it is larger than the cluster type allows, or conflicts with architectural pack patterns), packing fails with an error naming the group. Prepacked molecules (e.g. a LUT and the FF it drives, or carry-chain segments) that mix a constrained atom with unconstrained atoms are pulled into the group's cluster as a whole. A molecule whose atoms belong to two *different* groups is reported as an error: a molecule is indivisible (all its atoms always pack into one cluster), so it can never satisfy constraints that require its atoms to be in different clusters.
* **Groups never share a cluster.** This holds between groups of the same macro and between groups of different macros. Unconstrained atoms may fill the remaining capacity of any group's cluster.
* **Offsets must match the device grid.** The placer never adjusts an offset: it only uses positions where every member of the macro lands on a tile that can host its cluster. For example, ``x_offset="1"`` between a RAM cluster and a CLB cluster only works if the device has a CLB column immediately to the right of a RAM column — and if the RAM tile spans four grid rows, the RAM must sit on a RAM tile's first row, so the macro can only move vertically in steps of four. If no position on the device satisfies all members, placement fails after an exhaustive search.
* **Members may have different block types** (e.g. a CLB next to a DSP).
* **Interaction with carry chains:** a cluster cannot belong to both a relative macro and an architecture-derived placement macro (carry chain); this is reported as an error.
* Relative macro constraints are validated when a packed netlist (``.net``) or placement (``.place``) file is loaded, so stale files that do not satisfy the constraints are rejected.
