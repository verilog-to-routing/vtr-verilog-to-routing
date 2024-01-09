Global Routing Constraints
==========================
.. _global_routing_constraints:

VPR allows users to designate specific nets in the input netlist as global and define the routing model for the global nets by utilizing a VPR constraints XML file. These routing constraints for global nets are specified inside the VPR constraints file in XML format, as described in the following section.  

A Global Routing Constraints File Example
------------------------------------------

.. code-block:: xml
    :caption: An example of a global routing constraints file in XML format.
    :linenos:

    <vpr_constraints tool_name="vpr">
        <global_route_constraints>
            <set_global_signal name="clock*" route_model="dedicated_network" network_name="clock_network"/>
        </global_route_constraints>
    </vpr_constraints>


Global Routing Constraints File Format
---------------------------------------
.. _global_routing_constraints_file_format:

.. vpr_constraints:tag:: <global_route_constraints>content</global_route_constraints>

    Content inside this tag contains a group of ``<set_global_signal>`` tags that specify the global nets and their assigned routing methods.

.. vpr_constraints:tag:: <set_global_signal name="string" route_model="{ideal|route|dedicated_network}" network_name="string"/>

    :req_param name: The name of the net to be assigned as global. Regular expressions are also accepted. 
    :req_param route_model: The route model for the specified net.
       
        * ``ideal``: The net is not routed. There would be no delay for the global net. 

        * ``route``: The net is routed similarly to other nets using generic routing resources.

        * ``dedicated_network``: The net will be routed through a dedicated clock network.

    :req_param network_name: The name of the clock network through which the net is routed. This parameter is required when ``route_model="dedicated_network"``.


Dedicated Clock Networks
--------------------------

Users can define a clock architecture in two ways. First, through the architecture description outlined in section :ref:`Clocks <clock_architecture>`. By using the ``<clocknetworks>`` tag, users can establish a single clock architecture with the fixed default name "clock_network". When routing a net through the dedicated network described with this tag, the network name must be set to ``"clock_network"``.

Alternatively, users can define a custom clock network architecture by inputting a custom resource routing graph. In this approach, users can specify various clock routing networks, such as a global clock network and multiple regional clock networks.  There are three main considerations for defining a clock network through a custom RR graph definition, as described in the following sections.

Virtual Sinks Definition for Clock Networks
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
For VPR to route global nets within defined clock networks, there needs to be a virtual sink node defined in the RR graph per each clock network. This virtual sink, which is of the type ``"SINK"``, must have incoming edges from all drive points of the clock network. The two-stage router used for global net routing will initially route to the virtual sink (which serves as the entry point of the clock network) in the first stage and then from the entry point to the actual sink of the net in the second stage.

To indicate that a node represents a clock network virtual sink, users can utilize the ``"clk_res_type"`` attribute on a node setting it to ``"VIRTUAL_SINK"``.

Distinguishing Between Clock Networks
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Given the support for multiple clock networks, VPR needs a way to distinguish between different virtual sinks belonging to various clock networks. This is achieved through the optional ``"name"`` attribute for the rr_node, accepting a string used as the clock network name. Therefore, when the ``"clk_res_type"`` is set to  ``"VIRTUAL_SINK"``, the attribute ``"name"`` becomes a requried parameter to enable VPR to determine which clock netwrok the virtual sink belongs to. 

When specifying the network_name in a global routing constraints file for routing a global net through a desired clock network, as described in the :ref:`above <global_routing_constraints_file_format>` section, the name defined as an attribute in the virtual sink of the clock network should be used to reference that clock network.

Segment Definition for Clock Networks
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The node types ``"CHANX"`` and ``"CHANY"`` can construct the clock routing architecture, similar to a generic routing network. However, to identify nodes that are part of a clock network, one can define unique segments for clock networks. To indicate that a segment defined is a clock network resource, users can use the optional attribute ``res_type="GCLK"``. Therefore, nodes with a segment ID of this defined segment are considered to be part of a clock network.

While VPR currently does not leverage this distinction of clock network resources, it is recommended to use the ``res_type="GCLK"`` attribute, as this preparation ensures compatibility for future support.