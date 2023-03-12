
VPR Placement Constraints
=========================
.. _vpr_constraints_file:
VPR supports running flows with route constraints. Route constraints are set on global signals to specify if they should be routed or not. For example, a user may want to route a specific internal clock even clock modeling option is set to not route it.

The route constraints should be specified by the user using an XML constraints file format, as described in the section below. 

A Constraints File Example
--------------------------

.. code-block:: xml
	:caption: An example of a route constraints file in XML format.
	:linenos:

	<vpr_constraints tool_name="vpr">
                <global_route_constraints>
                        <!-- specify route method for a global pin that needs to be connected globally -->
                        <set_global_signal name="(int_clk)(.*)" type="clock" route_model="route"/>
                        <set_global_signal name="clk_ni" type="clock" route_model="ideal"/>
                </global_route_constraints>
	</vpr_constraints>

.. _end:

Constraints File Format
-----------------------

VPR has a specific XML format which must be used when creating a route constraints file. The purpose of this constraints file is to specify 

#. The signals that should be constrained for routing
#. The route model for such signals 

The file is passed as an input to VPR when running with route constraints. When the file is read in, its information is used to guide VPR route or not route such signals.

.. note:: Use the VPR option :vpr:option:`--read_vpr_constraints` to specify the VPR route constraints file that is to be loaded. 

.. note:: Wildcard names of signals are supported to specify a list of signals. The wildcard expression should follow the C/C++ regexpr rule.

