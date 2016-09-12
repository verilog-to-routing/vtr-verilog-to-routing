.. _odin_II:

Odin II
=======

Odin II is used for logic synthesis and elaboration, converting a subset of the Verilog Hardware Description Language (HDL) into a BLIF netlist.

.. seealso:: :cite:`jamieson_odin_II`

.. todo:: More documentation!

.. code-block:: none

    USAGE: odin_II.exe [-c <Configuration> | -b <BLIF> | -V <Verilog HDL>]
      -c <XML Configuration File>
      -V <Verilog HDL File>
      -b <BLIF File>
     Other options:
      -o <output_path and file name>
      -a <architecture_file_in_VPR6.0_form>
      -G Output netlist graph in graphviz .dot format. (net.dot, opens with dotty)
      -A Output AST graph in .dot format.
      -W Print all warnings. (Can be substantial.) 
      -h Print help

     SIMULATION: Always produces input_vectors, output_vectors,
                 and ModelSim test.do file.
      Activate simulation with either: 
      -g <Number of random test vectors to generate>
         -L <Comma-separated list of primary inputs to hold 
             high at cycle 0, and low for all subsequent cycles.>
         -H <Comma-separated list of primary inputs to hold low at 
             cycle 0, and high for all subsequent cycles.>
         -3 Generate three valued logic. (Default is binary.)
      -t <input vector file>: Supply a predefined input vector file
      -U Default initial register value. Set to -U0, -U1 or -UX (unknown). Default: X
     Other Simulation Options: 
      -T <output vector file>: Supply an output vector file to check output
                                vectors against.
      -E Output after both edges of the clock.
         (Default is to output only after the falling edge.)
      -R Output after rising edge of the clock only.
         (Default is to output only after the falling edge.)
      -p <Comma-separated list of additional pins/nodes to monitor
          during simulation.>
         Eg: "-p input~0,input~1" monitors pin 0 and 1 of input, 
           or "-p input" monitors all pins of input as a single port. 
           or "-p input~" monitors all pins of input as separate ports. (split) 
         - Note: Non-existent pins are ignored. 
         - Matching is done via strstr so general strings will match 
           all similar pins and nodes.
             (Eg: FF_NODE will create a single port with all flipflops) 

