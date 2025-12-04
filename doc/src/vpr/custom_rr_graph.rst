.. _custom_rr_graph:

Custom RR Graph
===============

For users who want more control over how connections are made in the routing resource graph, VPR provides a way to describe them through a Custom RR Graph (CRR) generator.

Currently, the CRR Generator is only based on the tileable RR Graph. Support for the default VPR RR Graph Generator is in progress. To generate a CRR, the following files are required:

* Switch block map file
* Switch block template files

Switch Block Map File
~~~~~~~~~~~~~~~~~~~~~~

This is a YAML file that specifies which switch block template should be used for each tile. The mapping format supports the following patterns:

* ``SB_1__1_: [sb_template.csv]`` - Tile at location (1, 1) uses switch block template ``sb_template.csv``
* ``SB_1__*_: [sb_template.csv]`` - All tiles with x coordinate 1 use switch block template ``sb_template.csv``
* ``SB_[7,20]__[2:32:3]_: [sb_template.csv]`` - Tiles with x equal to 7 or 20, and y coordinates from 2 to 32 (inclusive) with step 3 use switch block template ``sb_template.csv``

**Important:** The order in which patterns are defined matters, as the first matching pattern is used.

Switch Block Template Files
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Terminology
-----------

Before describing the template files, let's define some key terminology:

**Lane:** A group of wires with the same length. The starting points of consecutive wires in a lane are one switch block apart.

**Tap:** A switch block location where a wire passes through and can have fan-out connections.

See the figure below for an illustration:

.. figure:: images/lane_and_tap.png
   :alt: Lane and Tap
   :width: 50%
   :align: center

   In the figure above, the taps for the red wire are shown, and the lanes are separated by dotted lines. Note that the figure is simplified for illustration purposes.

In the actual RR Graph, when a wire passes through a switch block, its track number changes. Below is a more realistic example:

.. figure:: images/lane_and_tap_realistic.png
   :alt: Lane and Tap Realistic
   :width: 50%
   :align: center

   A more realistic example of a lane and a tap. Each box contains a lane.

Template File Format
--------------------

There should be a directory containing the pattern files specified in the switch block maps file. Each template is a CSV file with the following format:

* **Rows** represent source nodes
* **Columns** represent sink nodes
* An **'x' mark** at an intersection indicates that the source and sink nodes are connected
* A **number** at an intersection indicates that the nodes are connected with the switch delay specified by that number

**Note:** The pattern currently only supports uni-directional segments. Therefore, wires can only be driven from their starting point.

Row Headers (Source Nodes)
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Each row has four header columns describing the source node:

1. **Column 1 - Direction:** The side from which the source node is entering the switch block (e.g., 'left', 'right', 'top', 'bottom')
2. **Column 2 - Segment Type:** The segment length to which the source node belongs
3. **Column 3 - Lane Number:** The lane to which the source node belongs (see terminology above)
4. **Column 4 - Tap Number:** Which tap of the source node is at this switch block

Column Headers (Sink Nodes)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Each column has header rows describing the sink node:

1. **Row 1 - Direction:** The side from which the sink node is exiting the switch block
2. **Row 2 - Segment Type:** The segment length to which the sink node belongs
3. **Row 3 - Fan-in:** The fan-in of the sink node (optional)
4. **Row 4 - Lane Number:** The lane to which the sink node belongs

Example
^^^^^^^

Let’s consider an architecture with a channel width of 16 that contains only wire segments of length 4.

The number of rows should be calculated as:

- ``4`` (number of sides) × ``160/2`` (number of tracks in one direction) = ``320`` rows.

On each side, there should be:

- ``20`` lanes (``80 / 4``), and
- each lane requires ``4`` rows (because length-4 wires require 4 tap positions).

For the columns, the count should be:

- ``4`` (number of sides) × ``160/2`` (number of tracks in one direction) ÷ ``4`` (number of starting tracks per lane) = ``80`` columns.

Each switch block template file must contain the number of rows and columns
determined in the previous section. After creating the template file with the
correct dimensions, you can populate the spreadsheet by placing the switch
delay values in the appropriate cells (i.e., in the cells representing valid
connections between source and sink nodes).

Once the switch block map file and template files are created, include the
following arguments in the VPR command line:

- ``--sb_maps <switch_block_map_file>``
- ``--sb_templates <switch_block_template_directory>``
- ``--sb_count_dir <switch_block_count_directory>`` (optional):  
  If provided, VPR will generate a CSV file for each switch block template,
  indicating how many times each switch defined in the template is used in the
  final routing results.

For additional arguments, refer to the command-line usage section on the
:ref:`vpr_command_line_usage` page.