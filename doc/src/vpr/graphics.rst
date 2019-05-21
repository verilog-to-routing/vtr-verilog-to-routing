.. _vpr_graphics:

Graphics
========
VPR includes easy-to-use graphics for visualizing both the targetted FPGA architecture, and the circuit VPR has implementation on the architecture.

Enabling Graphics
-----------------

Compiling with Graphics Support
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The build system will attempt to build VPR with graphics support by default.

If all the required libraries are found the build system will report::

    -- EasyGL: graphics enabled

If the required libraries are not found cmake will report::

    -- EasyGL: graphics disabled

and list the missing libraries::

    -- EasyGL: Failed to find required X11 library (on debian/ubuntu try 'sudo apt-get install libx11-dev' to install)
    -- EasyGL: Failed to find required Xft library (on debian/ubuntu try 'sudo apt-get install libxft-dev' to install)
    -- EasyGL: Failed to find required fontconfig library (on debian/ubuntu try 'sudo apt-get install fontconfig' to install)
    -- EasyGL: Failed to find required cairo library (on debian/ubuntu try 'sudo apt-get install libcairo2-dev' to install)

Enabling Graphics at Run-time
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
When running VPR provide :option:`vpr --disp` ``on`` to enable graphics.

A graphical window will now pop up when you run VPR.

Navigation
----------
Click any mouse button on the **arrow** keys to pan the view, or click on the **Zoom-In**, **Zoom-Out** and **Zoom-Fit** buttons to zoom the view.
Alternatively, click and drag the mouse wheel to pan the view, or scroll the mouse wheel to zoom in and out.
Click on the **Window button**, then on the diagonally opposite corners of a box, to zoom in on a particular area.

Selecting **PostScript** creates a PostScript file (in pic1.ps, pic2.ps, etc.) of the image on screen.

**Proceed** tells VPR to continue with the next step in placing and routing the circuit.
**Exit** aborts the program.

.. note:: Menu buttons will be greyed out when they are not selectable (e.g. VPR is working).

Visualizing Netlist Connectivity
--------------------------------
The **Toggle Nets** button toggles the nets in the circuit visible/invisible.

When a placement is being displayed, routing information is not yet known so nets are simply drawn as a “star;” that is, a straight line is drawn from the net source to each of its sinks.
Click on any clb in the display, and it will be highlighted in green, while its fanin and fanout are highlighted in blue and red, respectively.
Once a circuit has been routed the true path of each net will be shown.

Again, you can click on Toggle Nets to make net routings visible or invisible.
If the nets routing are shown, click on a clb or pad to highlight its fanins and fanouts, or click on a pin or channel wire to highlight a whole net in magenta.
Multiple nets can be highlighted by pressing ctrl + mouse click.

Visualizing Routing Architecture
--------------------------------
When a routing is on-screen, clicking on **Toggle RR** will switch between various views of the routing resources available in the FPGA.

The routing resource view can be very useful in ensuring that you have correctly described your FPGA in the architecture description file -- if you see switches where they shouldn’t be or pins on the wrong side of a clb, your architecture description needs to be revised.

Wiring segments are drawn in black, input pins are drawn in sky blue, and output pins are drawn in pink.
Direct connections between output and input pins are shown in medium purple.
Connections from wiring segments to input pins are shown in sky blue, connections from output pins to wiring segments are shown in pink, and connections between wiring segments are shown in green.
The points at which wiring segments connect to clb pins (connection box switches) are marked with an ``x``.

Switch box connections will have buffers (triangles) or pass transistors (circles) drawn on top of them, depending on the type of switch each connection uses.
Clicking on a clb or pad will overlay the routing of all nets connected to that block on top of the drawing of the FPGA routing resources, and will label each of the pins on that block with its pin number.
Clicking on a routing resource will highlight it in magenta, and its fanouts will be highlighted in red and fanins in blue.
Multiple routing resources can be highlighted by pressing ctrl + mouse click.

Visualizing Routing Congestion
------------------------------
When a routing is shown on-screen, clicking on the **Congestion** button will show a heat map of any overused routing resources (wires or pins).
Lighter colours (e.g. yellow) correspond to highly overused resources, while darker colours (e.g. blue) correspond to lower overuse.
The overuse range shown at the bottom of the window.

Visualizing the Critical Path
-----------------------------
During placement and routing you can click on the **Crit. Path** button to visualize the critical path.
Each stage between primitive pins is shown in a different colour.
Cliking the **Crit. Path** button again will toggle through the various visualizations:
* During placement the critical path is shown only as flylines.
* During routing the critical path can be shown as both flylines and routed net connections.

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
| Route BB          | Routing    	| Visualizes net bounding      | Click multiple times to      |
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


