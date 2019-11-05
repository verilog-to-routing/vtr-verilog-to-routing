.. _route_diag:

Router Diagnosis Tool
=====================

The Router Diagnosis tool (``route_diag``) is an utility that helps developers understand the issues related to the routing phase of VPR.
Instead of running the whole routing step, ``route_diag`` performs one step of routing, aimed at analyzing specific connections between a SINK/SOURCE nodes pair.
Moreover, it is able also to profile all the possible paths of a given SOURCE node.

To correctly run the utility tool, the user needs to compile VTR with the ``VTR_ENABLE_DEBUG_LOGGING`` set to ON and found in the CMakeLists.txt_ configuration file.

.. _CMakeLists.txt: https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/01ff7e174d9d53753a2f981d7be0052b612b5874/CMakeLists.txt#L36

The tool is compiled with all the other targets when running the full build of VtR. It is also possible, though, to build the ``route_diag`` utility standalone, by running the following command:

.. code-block:: bash

  make route_diag

To use the Route Diagnosis tool, the users has different parameters at disposal:

.. option:: --sink_rr_node <int>

  Specifies the SINK RR NODE part of the pair that needs to be analyzed

.. option:: --source_rr_node <int>

  Specifies the SOURCE RR NODE part of the pair that needs to be analyzed

.. option:: --router_debug_sink_rr <int>

  Controls when router debugging is enabled for the specified sink RR.

  * For values >= 0, the value is taken as the sink RR Node ID for which to enable router debug output.
  * For values < 0, sink-based router debug output is disabled.

The Router Diagnosis tool must be provided at least with the RR GRAPH and the architecture description file to correctly function.
