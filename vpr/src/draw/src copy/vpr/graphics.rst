.. _vpr_graphics:

Graphics
========
VPR includes easy-to-use graphics for visualizing both the targetted FPGA architecture, and the circuit VPR has implementation on the architecture.

.. image:: https://www.verilogtorouting.org/img/des90_routing_util.gif
    :align: center

Enabling Graphics
-----------------

Compiling with Graphics Support
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The build system will attempt to build VPR with graphics support by default.

If all the required libraries are found the build system will report::

    -- EZGL: graphics enabled

If the required libraries are not found cmake will report::

    -- EZGL: graphics disabled

and list the missing libraries::

    -- EZGL: Failed to find required X11 library (on debian/ubuntu try 'sudo apt-get install libx11-dev' to install)
    -- EZGL: Failed to find required Xft library (on debian/ubuntu try 'sudo apt-get install libxft-dev' to install)
    -- EZGL: Failed to find required fontconfig library (on debian/ubuntu try 'sudo apt-get install fontconfig' to install)
    -- EZGL: Failed to find required cairo library (on debian/ubuntu try 'sudo apt-get install libcairo2-dev' to install)

Enabling Graphics at Run-time
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
When running VPR provide :option:`vpr --disp` ``on`` to enable graphics.

Saving Graphics at Run-time
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
When running VPR provide :option:`vpr --save_graphics` ``on`` to enable graphics.

A graphical window will now pop up when you run VPR.

Navigation
----------
Click on **Zoom-Fit** buttons to zoom the view.
Click and drag with the left mouse button to pan the view, or scroll the mouse wheel to zoom in and out.
Click on the **Window**, then on the diagonally opposite corners of a box, to zoom in on a particular area.

Click on **Save** to save the image on screen to PDF, PNG, or SVG file.

**Proceed** tells VPR to continue with the next step in placing and routing the circuit.


.. note:: Menu buttons will be greyed out when they are not selectable (e.g. VPR is working).

Visualizing Placement
--------------------------------
By default VPR's graphics displays the FPGA floorplan (block grid) and current placement.

.. figure:: https://www.verilogtorouting.org/img/neuron_placement_macros.gif
    :align: center

    Placement with macros (carry chains) highlighted

If the **Placement Macros** drop down is set, any placement macros (e.g. carry chains, which require specific relative placements between some blocks) will be highlighted.

Visualizing Netlist Connectivity
--------------------------------
The **Toggle Nets** drop-down list toggles the nets in the circuit visible/invisible.

When a placement is being displayed, routing information is not yet known so nets are simply drawn as a “star;” that is, a straight line is drawn from the net source to each of its sinks.
Click on any clb in the display, and it will be highlighted in green, while its fanin and fanout are highlighted in blue and red, respectively.
Once a circuit has been routed the true path of each net will be shown.

.. figure:: https://www.verilogtorouting.org/img/des90_nets.gif
    :align: center

    Logical net connectivity during placement

If the nets routing are shown, click on a clb or pad to highlight its fanins and fanouts, or click on a pin or channel wire to highlight a whole net in magenta.
Multiple nets can be highlighted by pressing ctrl + mouse click.

Visualizing the Critical Path
-----------------------------
During placement and routing you can click on the **Crit. Path** drop-down menu to visualize the critical path.
Each stage between primitive pins is shown in a different colour.
Cliking the **Crit. Path** button again will toggle through the various visualizations:
* During placement the critical path is shown only as flylines.
* During routing the critical path can be shown as both flylines and routed net connections.

.. figure:: https://www.verilogtorouting.org/img/des90_cpd.gif
    :align: center

    Critical Path flylines during placement and routing

Visualizing Routing Architecture
--------------------------------
When a routing is on-screen, clicking on **Toggle RR** lets you to choose between various views of the routing resources available in the FPGA.

.. figure:: https://github.com/verilog-to-routing/verilog-to-routing.github.io/raw/master/img/routing_arch.gif
    :align: center

    Routing Architecture Views

The routing resource view can be very useful in ensuring that you have correctly described your FPGA in the architecture description file -- if you see switches where they shouldn’t be or pins on the wrong side of a clb, your architecture description needs to be revised.

Wiring segments are drawn in black, input pins are drawn in sky blue, and output pins are drawn in pink.
Sinks are drawn in dark slate blue, and sources in plum.
Direct connections between output and input pins are shown in medium purple.
Connections from wiring segments to input pins are shown in sky blue, connections from output pins to wiring segments are shown in pink, and connections between wiring segments are shown in green.
The points at which wiring segments connect to clb pins (connection box switches) are marked with an ``x``.

Switch box connections will have buffers (triangles) or pass transistors (circles) drawn on top of them, depending on the type of switch each connection uses.
Clicking on a clb or pad will overlay the routing of all nets connected to that block on top of the drawing of the FPGA routing resources, and will label each of the pins on that block with its pin number.
Clicking on a routing resource will highlight it in magenta, and its fanouts will be highlighted in red and fanins in blue.
Multiple routing resources can be highlighted by pressing ctrl + mouse click.

Visualizing Routing Congestion
------------------------------
When a routing is shown on-screen, clicking on the **Congestion** drop-down menu will show a heat map of any overused routing resources (wires or pins).
Lighter colours (e.g. yellow) correspond to highly overused resources, while darker colours (e.g. blue) correspond to lower overuse.
The overuse range shown at the bottom of the window.

.. figure:: https://www.verilogtorouting.org/img/bitcoin_congestion.gif
    :align: center

    Routing Congestion during placement and routing

Visualizing Routing Utilization
-------------------------------
When a routing is shown on-screen, clicking on the **Routing Util** drop-down menu will show a heat map of routing wire utilization (i.e. fraction of wires used in each channel).
Lighter colours (e.g. yellow) correspond to highly utilized channels, while darker colours (e.g. blue) correspond to lower utilization.

.. figure:: https://www.verilogtorouting.org/img/bitcoin_routing_util.gif
    :align: center

    Routing Utilization during placement and routing

Toggle Block Internal
-------------------------------
During placement and routing you can adjust the level of block detail you visualize by using the **Toggle Block Internal**. Each block can contain a number of flip flops (ff), look up tables (lut), and other primitives. The higher the number, the deeper into the hierarchy within the cluster level block you see. 

.. figure:: https://www.verilogtorouting.org/img/ToggleBlockInternal.gif
    :align: center

    Visualizing Block Internals


Button Description Table
------------------------
+-------------------+-------------------+------------------------------+------------------------------+
|      Buttons      |      Stages       |        Functionalities       |     Detailed Descriptions    |
+-------------------+-------------------+------------------------------+------------------------------+
| Blk Internal      | Placement/Routing | Controls depth of sub-blocks | Click multiple times to show |
|                   |                   | shown                        | more details; Click to reset |
|                   |                   |                              | when reached maximum level   |
|                   |                   |                              | of detail                    |
+-------------------+-------------------+------------------------------+------------------------------+
| Toggle Block      | Placement/Routing | Adjusts the level of         | Click multiple times to      |
| Internal          |                   | visualized block detail      | go deeper into the           |
|                   |                   |                              | hierarchy within the cluster |
|                   |                   |                              | level block                  |
|                   |                   |                              |                              |
+-------------------+-------------------+------------------------------+------------------------------+
| Blk Pin Util      | Placement/Routing | Visualizes block pin         | Click multiple times to      |
|                   |                   | utilization                  | visualize all block pin      |
|                   |                   |                              | utilization, input block pin |
|                   |                   |                              | utilization, or output block |
|                   |                   |                              | pin utilization              |
+-------------------+-------------------+------------------------------+------------------------------+
| Cong. Cost        | Routing           | Visualizes the congestion    |                              |
|                   |                   | costs of routing resouces    |                              |
|                   |                   |                              |                              |
|                   |                   |                              |                              |
+-------------------+-------------------+------------------------------+------------------------------+
| Congestion        | Routing           | Visualizes a heat map of     |                              |
|                   |                   | overused routing resources   |                              |
|                   |                   |                              |                              |
|                   |                   |                              |                              |
+-------------------+-------------------+------------------------------+------------------------------+
| Crit. Path        | Placement/Routing | Visualizes the critical path |                              |
|                   |                   | of the circuit               |                              |
|                   |                   |                              |                              |
|                   |                   |                              |                              |
+-------------------+-------------------+------------------------------+------------------------------+
| Place Macros      | Placement/Routing | Visualizes placement macros  |                              |
|                   |                   |                              |                              |
|                   |                   |                              |                              |
|                   |                   |                              |                              |
+-------------------+-------------------+------------------------------+------------------------------+
| Route BB          | Routing           | Visualizes net bounding      | Click multiple times to      |
|                   |                   | boxes one by one             | sequence through the net     |
|                   |                   |                              | being shown                  |
|                   |                   |                              |                              |
+-------------------+-------------------+------------------------------+------------------------------+
| Router Cost       | Routing           | Visualizes the router costs  |                              |
|                   |                   | of different routing         |                              |
|                   |                   | resources                    |                              |
|                   |                   |                              |                              |
+-------------------+-------------------+------------------------------+------------------------------+
| Routing Util      | Routing           | Visualizes routing channel   |                              |
|                   |                   | utilization with colors      |                              |
|                   |                   | indicating the fraction of   |                              |
|                   |                   | wires used within a channel  |                              |
+-------------------+-------------------+------------------------------+------------------------------+
| Toggle Nets       | Placement/Routing | Visualizes the nets in the   | Click multiple times to      |
|                   |                   | circuit                      | set the nets to be visible / |
|                   |                   |                              | invisible                    |
|                   |                   |                              |                              |
+-------------------+-------------------+------------------------------+------------------------------+
| Toggle RR         | Placement/Routing | Visualizes different views   | Click multiple times to      |
|                   |                   | of the routing resources     | switch between routing       |
|                   |                   |                              | resources available in the   |
|                   |                   |                              | FPGA                         |
+-------------------+-------------------+------------------------------+------------------------------+

Manual Moves
------------

The manual moves feature allows the user to specify the next move in placement. If the move is legal, blocks are swapped and the new move is shown on the architecture. 

.. figure:: https://www.verilogtorouting.org/img/manual_move_toggle_button.png
   :align: center

To enable the feature, activate the Manual Move toggle button and press Proceed. Alternatively, the user can active the Manual Move toggle button and click on the block to be moved.

.. figure:: https://www.verilogtorouting.org/img/draw_manual_moves_window.png
   :align: center

On the manual move window, the user can specify the Block ID/Block name of the block to move and the To location, with the x position, y position and subtile position. For the manual move to be valid:

- The To location requested by the user should be within the grid's dimensions.
- The block to be moved is found, valid and not fixed.
- The blocks to be swapped are compatible.
- The location choosen by the user is different from the block's current location.
  
If the manual move is legal, the cost summary window will display the delta cost, delta timing, delta bounding box cost and the placer's annealing decision that would result from this move. 

.. figure:: https://www.verilogtorouting.org/img/manual_move_cost_dialog.png
   :align: center

The user can Accept or Reject the manual move based on the values provided. If accepted the block's new location is shown. 

