Timing Constraints
==================
VPR supports setting timing constraints using Synopsys Design Constraints (SDC), an industry-standard format for specifying timing constraints.

VPR's default timing constraints are explained in :ref:`default_timing_constraints`.
The subset of SDC supported by VPR is described in :ref:`sdc_commands`.
Additional SDC examples are shown in :ref:`sdc_examples`.


.. _default_timing_constraints:

Default Timing Constraints
--------------------------
If no timing constriants are specified, VPR assumes default constraints based on the type of circuit being analyzed.

Combinational Circuits
~~~~~~~~~~~~~~~~~~~~~~
Constrain all I/Os on a virtual clock ``virtual_io_clock``, and optimize this clock to run as fast as possible.

*Equivalent SDC File:*

.. code-block:: tcl

    create_clock -period 0 -name virtual_io_clock
    set_input_delay -clock virtual_io_clock -max 0 [get_ports {*}]
    set_output_delay -clock virtual_io_clock -max 0 [get_ports {*}]

Single-Clock Circuits
~~~~~~~~~~~~~~~~~~~~~
Constrain all I/Os on the netlist clock, and optimize this clock to run as fast as possible.

*Equivalent SDC File:*

.. code-block:: tcl

    create_clock -period 0 *
    set_input_delay -clock * -max 0 [get_ports {*}]
    set_output_delay -clock * -max 0 [get_ports {*}]


Multi-Clock Circuits
~~~~~~~~~~~~~~~~~~~~~
Constrain all I/Os a virtual clock ``virtual_io_clock``.
Does not analyse paths between netlist clock domains, but analyses all paths from I/Os to any netlist domain.
Optimizes all clocks, including I/O clocks, to run as fast as possible.

.. warning:: By default VPR does not analyze paths between netlist clock domains.

*Equivalent SDC File:*

.. code-block:: tcl

    create_clock -period 0 *
    create_clock -period 0 -name virtual_io_clock
    set_clock_groups -exclusive -group {clk} -group {clk2}
    set_input_delay -clock virtual_io_clock -max 0 [get_ports {*}]
    set_output_delay -clock virtual_io_clock -max 0 [get_ports {*}]

Where ``clk`` and ``clk2`` are the netlist clocks in the design.
This is similarily extended if there are more than two netlist clocks.

