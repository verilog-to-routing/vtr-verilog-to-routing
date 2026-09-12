
Relative Placement Constraints
==============================
.. _relative_placement_constraints:

.. warning::

	Relative placement constraints are experimental and under active development.
	The constraints file format and its loader described here are in place, but packing and placement support for relative macros has not been merged yet.
	VPR validates a ``<relative_macro_list>`` when it reads the constraints file and then stops with an error, so the constraints are never silently ignored.

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

Relative Placement Constraints File Format
------------------------------------------

The ``<vpr_constraints>`` top-level tag may contain one ``<relative_macro_list>`` tag, which contains an unbounded number of ``<relative_macro>`` tags.

.. arch:tag:: <relative_macro name="string">

	A relative macro is made up of exactly one ``<reference_group>`` (which must come first) followed by one or more ``<relative_group>`` tags.

	:req_param name:
		A unique name for the macro, used in log and error messages.

	.. arch:tag:: <reference_group>

		The anchor of the macro. The atoms added to this group are packed into one cluster, and all relative groups' offsets are measured from this cluster's location. Contains one or more ``<add_atom>`` tags.

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

	.. arch:tag:: <add_atom name_pattern="string" is_regex="bool" site_path="string">

		Adds the atoms matching ``name_pattern`` to the enclosing reference or relative group.
		``name_pattern`` and ``is_regex`` have the same syntax and matching semantics as in a ``<partition>`` (see :ref:`placement constraints <placement_constraints>`); a pattern that matches no atom is skipped with a warning.
		The ``logical_block_location`` attribute of a partition's ``<add_atom>`` is not supported here and is ignored with a warning.

		:opt_param site_path:
			The primitive site inside the group's cluster this atom must be placed on.

			A site is given as the **hierarchical path** of the primitive within its cluster (``t_pb_graph_node::hierarchical_type_name()``), for example::

				clb[0][default]/lab[0][default]/fle[3][n1_lut6]/ble6[0][default]/lut6[0]

			The path must name a primitive of one of the architecture's logical block types, and that primitive must be able to implement the atom; since a site holds one atom, a pattern carrying a ``site_path`` may match at most one atom.
			An atom matched both by a pattern with a ``site_path`` and by one without keeps the lock; two patterns locking the same atom to different sites are an error.

			The site paths of a placed design can be obtained by running VPR with
			:option:`vpr --write_flat_place` and :option:`vpr --flat_place_verbosity` set to ``2``.
			**Default:** none (the packer picks the site)
