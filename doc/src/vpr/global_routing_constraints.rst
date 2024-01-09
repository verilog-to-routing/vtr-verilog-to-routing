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


