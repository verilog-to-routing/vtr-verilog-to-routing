.. _vpr_graphics:

Graphics
========
VPR includes easy-to-use graphics for visualizing both the targetted FPGA architecture, and the circuit VPR has implementation on the architecture.

Enabling Graphics
-----------------

Compiling with Graphics Support
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The build system will attempt to build VPR with graphics support by default.

If all the reuired libraries are found the build system will report::

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
When running VPR provide :option:`vpr -disp` ``on`` to enable graphics.

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
When a routing is shown on-screen, clicking on the **Congestion** button will show any overused routing resources (wires or pins) in red, if any overused resources exist.
Clicking on the same button the second time will show overused routing resources in red and all other used routing resources in blue.

Visualizing the Critical Path
-----------------------------
Finally, when a routing is on screen you can click on the **Crit. Path** button to see each of the nets on the critical path in turn.
The current net on the critical path is highlighted in cyan; its source block is shown in yellow and the critical sink is shown in green.
