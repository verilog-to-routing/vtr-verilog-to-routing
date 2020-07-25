Debugging Aids
==============

.. program:: vpr

.. note:: This section is most relevant to developers modifying VPR

The ``report_timing.setup.rpt`` file lists details about the critical path of a circuit, and is very useful for determining why your circuit is so fast or so slow.

To access detailed echo files from VPR’s operation, use the command-line option :vpr:option:`--echo_file` ``on``.
After parsing the netlist and architecture files, VPR dumps out an image of its internal data structures into echo files (typically ending in ``.echo``).
These files can be examined to be sure that VPR is parsing the input files as you expect.

To display and debug the placement move generator, enable the f_placer_flag in move_utils.cpp file. The new location of the moving block for each proposed move will be highlighted with GREEN and the old location will be highlighted with GOLD. The fanin and fanout blocks will also be highlighted. The move type, move outcome and delta cost will be printed in the status bar.
.. warning:: VPR must have been compiled with `VTR_ENABLE_DEBUG_LOGGING` on to get any debug output from this flag.   

If the preprocessor flag ``DEBUG`` is defined in ``vpr_types.h``, some additional sanity checks are performed during a run.
``DEBUG`` only slows execution by 1 to 2%.
The major sanity checks are always enabled, regardless of the state of ``DEBUG``.
Finally, if ``VERBOSE`` is set in vpr_types.h, a great deal of intermediate data will be printed to the screen as VPR runs.
If you set verbose, you may want to redirect screen output to a file.

The initial and final placement costs provide useful numbers for regression testing the netlist parsers and the placer, respectively.
VPR generates and prints out a routing serial number to allow easy regression testing of the router.

Finally, if you need to route an FPGA whose routing architecture cannot be described in VPR’s architecture description file, don’t despair!
The router, graphics, sanity checker, and statistics routines all work only with a graph that defines all the available routing resources in the FPGA and the permissible connections between them.
If you change the routines that build this graph (in ``rr_graph*.c``) so that they create a graph describing your FPGA, you should be able to route your FPGA.
If you want to read a text file describing the entire routing resource graph, call the ``dump_rr_graph`` subroutine.
