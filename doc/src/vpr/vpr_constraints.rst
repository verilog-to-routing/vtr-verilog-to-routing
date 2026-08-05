VPR Constraints
=========================
.. _vpr_constraints:

Users can specify placement and/or global routing constraints on all or part of a design through a constraints file in XML format, as shown in the format below. These constraints are optional and allow detailed control of the region on the chip in which parts of the design are placed, of the placement of parts of the design relative to each other, and of the routing of global nets through dedicated (usually clock) networks.

.. code-block:: xml
   :caption: The overall format of a VPR constraints file
   :linenos:

   <vpr_constraints tool_name="vpr">
       <partition_list>
         <!-- Placement (region) constraints are specified inside this tag -->
       </partition_list>
       <relative_macro_list>
         <!-- Relative placement constraints are specified inside this tag -->
       </relative_macro_list>
       <global_route_constraints>
         <!-- Global routing constraints are specified inside this tag -->
       </global_route_constraints>
   </vpr_constraints>

.. note:: Use the VPR option :option:`vpr --read_vpr_constraints` to specify the VPR constraints file that is to be loaded.

The top-level tag is the ``<vpr_constraints>`` tag. This tag can contain ``<partition_list>``, ``<relative_macro_list>``, and ``<global_route_constraints>`` tags. The ``<partition_list>`` tag contains information related to placement (region) constraints, the ``<relative_macro_list>`` tag contains relative placement constraints, while ``<global_route_constraints>`` contains information about global routing constraints. The details for each of these constraints are given in the respective sections :ref:`Placement Constraints <placement_constraints>`, :ref:`Relative Placement Constraints <relative_placement_constraints>`, and :ref:`Global Route Constraints <global_routing_constraints>`.

.. toctree::
   :maxdepth: 2

   placement_constraints
   relative_placement_constraints
   global_routing_constraints


