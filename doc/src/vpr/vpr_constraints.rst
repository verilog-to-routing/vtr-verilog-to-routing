VPR Constraints
=========================
.. _vpr_constraints:

VPR allows users to run the flow with placement constraints that enable primitives to be locked down to a specific region on the chip and global routing constraints that facilitate the routing of global nets through clock networks.

Users can specify these constraints through a constraints file in XML format, as shown in the format below.

.. code-block:: xml
   :caption: The overall format of a VPR constraints file
   :linenos:

   <vpr_constraints tool_name="vpr">
       <partition_list>
         <!-- Placement constraints are specified inside this tag -->
       </partition_list>
       <global_route_constraints>
         <!-- Global routing constraints are specified inside this tag -->
       </global_route_constraints>
   </vpr_constraints>

.. note:: Use the VPR option :option:`vpr --read_vpr_constraints` to specify the VPR constraints file that is to be loaded.

The top-level tag is the ``<vpr_constraints>`` tag. This tag can contain ``<partition_list>`` and ``<global_route_constraints>`` tags. The ``<partition_list>`` tag contains information related to placement constraints, while ``<global_route_constraints>`` contains information about global routing constraints. The details for each of these constraints are given in the respective sections :ref:`Placement Constraints <placement_constraints>` and :ref:`Global Route Constraints <global_routing_constraints>`.

.. toctree::
   :maxdepth: 2

   placement_constraints
   global_routing_constraints


