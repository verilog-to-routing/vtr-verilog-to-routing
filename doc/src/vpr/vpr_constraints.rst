VPR Constraints
=========================
.. _vpr_constraints:

Users can specify placement and/or global routing constraints on all or part of a design through a constraints file in XML format, as shown in the format below. These constraints are optional and allow detailed control of the region on the chip in which parts of the design are placed, and of the routing of global nets through dedicated (usually clock) networks. 

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


