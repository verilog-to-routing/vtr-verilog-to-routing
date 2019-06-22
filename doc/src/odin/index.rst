.. _odin_II:

#############
Odin II
#############

Odin II is used for logic synthesis and elaboration, converting a subset of the Verilog Hardware Description Language (HDL) into a BLIF netlist.

.. seealso:: :cite:`jamieson_odin_II`

*************
INSTALL
*************

=============
Prerequisites
=============

1. ctags
2. bison
3. flex
4. gcc 5.x
5. cmake 2.8.12 (minimum version)
6. time
7. cairo

===========
Build
===========

To build you may use the Makefile wrapper in the ODIN_ROOT ``make build``
To build with debug symbols you may use the Makefile wrapper in the ODIN_ROOT ``make debug``

.. note::
	ODIN uses CMake as it's build system. CMake provides a protable cross-platform build systems with many useful features.
	For unix-like systems we provide a wrapper Makefile which supports the traditional make and make clean commands,
	but calls CMake behind the scenes.

.. warning::
	After you build Odin, please run from the ODIN_ROOT ``make test``
This will simulate, and verify all of the included microbenchmark circuits to ensure that Odin is working correctly on your system.



*************
USAGE
*************

./odin_II [args]

===============
Required [args]
===============

.. list-table::

 * - ``-c``
   - <XML Configuration File>
   - fpga_architecture_file.xml format is specified from VPR
 * - ``-V``
   - <Verilog HDL File>
   - You may specify multiple verilog HDL files for synthesis
 * - ``-b``
   - <BLIF File>
   -

===============
Optional [args]
===============

.. list-table::

 * - ``-o``
   - <output file>
   - full output path and file name for the blif output file
 * - ``-a``
   - <architecture file>
   - an FPGA architecture file in VPR format to map to
 * - ``-G``
   -
   - Output netlist graph in GraphViz viewable .dot format. (net.dot, opens with dotty)
 * - ``-A``
   -
   - Output AST graph in in GraphViz viewable .dot format.
 * - ``-W``
   -
   - Print all warnings. (Can be substantial.)
 * - ``-h``
   -
   - Print help


===========
Simulation
===========

.. note::

	Simulation always produces files:

	    - input_vectors
	    - output_vectors
	    - test.do (ModelSim)

Activate Simulation with [args]
*******************************

.. list-table::

 * - ``-g``
   - <Number of random test vectors>
   - will simulate the generated netlist with the entered number of clock cycles using pseudo-random test vectors. These vectors and the resulting output vectors are written to "input_vectors" and "output_vectors" respectively. You can supply a predefined input vector using ``-t``
 * - ``-L``
   - <Comma-separated list>
   - Comma-separated list of primary inputs to hold high at cycle 0, and low for all subsequent cycles.
 * - ``-3``
   -
   - Generate three valued logic. (Default is binary.)
 * - ``-t``
   - <input vector file>
   - Supply a predefined input vector file
 * - ``-U0``
   -
   - initial register value to 0
 * - ``-U1``
   -
   - initial register value to 1
 * - ``-UX``
   -
   - initial register value to X(unknown) (DEFAULT)

Simulation Optional [args]
**************************

.. list-table::

 * - ``-T``
   - <output vector file>
   - The output vectors is verified against the supplied predefined output vector file
 * - ``-E``
   -
   - Output after both edges of the clock. (Default is to output only after the falling edge.)
 * - ``-R``
   -
   - Output after rising edge of the clock only. (Default is to output only after the falling edge.)
 * - ``-p``
   - <Comma-separated list>
   - Comma-separated list of additional pins/nodes to monitor during simulation. (view NOTES)

===========
NOTES
===========

Example for ``-p``:
*******************

.. list-table::

 * - ``-p input~0,input~1``
   - monitors pin 0 and 1 of input
 * - ``-p input``
   - monitors all pins of input as a single port
 * - ``-p input~``
   - monitors all pins of input as separate ports. (split)

.. note::

	Matching for ``-p`` is done via strstr so general strings will match all similar pins and nodes. (Eg: FF_NODE will create a single port with all flipflops)


Examples .xml configuration file for ``-c``
*******************************************

.. code-block:: xml

	<config>
		<verilog_files>
			<!-- Way of specifying multiple files in a project! -->
			<verilog_file>verilog_file.v</verilog_file>
		</verilog_files>
		<output>
			<!-- These are the output flags for the project -->
			<output_type>blif</output_type>
			<output_path_and_name>./output_file.blif</output_path_and_name>
			<target>
				<!-- This is the target device the output is being built for -->
				<arch_file>fpga_architecture_file.xml</arch_file>
			</target>
		</output>
		<optimizations>
			<!-- This is where the optimization flags go -->
		</optimizations>
		<debug_outputs>
			<!-- Various debug options -->
			<debug_output_path>.</debug_output_path>
			<output_ast_graphs>1</output_ast_graphs>
			<output_netlist_graphs>1</output_netlist_graphs>
		</debug_outputs>
	</config>

.. note::
  Hard blocks can be simulated; given a hardblock named ``block`` in the architecture file with an instance of it named ``instance`` in the verilog file, write a C method with signature defined in ``SRC/sim_block.h`` and compile it with an output filename of ``block+instance.so`` in the directory you plan to invoke Odin_II from.

  When compiling the file, you'll need to specify the following arguments to the compiler (assuming that you're in the SANBOX directory):

  ``cc -I../../libarchfpga_6/include/ -L../../libarchfpga_6 -lvpr_6 -lm --shared -o block+instance.so block.c.``

  If the netlist generated by Odin II contains the definition of a hardblock which doesn't have a shared object file defined for it in the working directory, Odin II will not work if you specify it to use the simulator with the ``-g`` or ``-t`` options.

.. warning::
  Use of static memory within the simulation code necessitates compiling a distinct shared object file for each instance of the block you wish to simulate. The method signature the simulator expects contains only int and int[] parameters, leaving the code provided to simulate the hard blokc agnostic of the internal Odin II data structures. However, a cycle parameter is included to provide researchers with the ability to delay results of operations performed by the simulation code.

Examples vector file for ``-t`` or ``-T``
*****************************************

.. code-block:: none

  # Example vector file
  intput_1 input_2 output_1 output_2 output_3
  # Comment
  0 0XA 1 0XD 1101

.. note::
  Each line represents a vector. Each value must be specified in binary or hex. Comments may be included by placing an # at the start of the line. Blank lines are ignored. Values may be separated by non-newline whitespace. (tabs and spaces) Hex values must be prefixed with 0X.

  Each line in the vector file represents one cycle, or one falling edge and one rising edge. Input vectors are read on a falling edge, while output vectors are written on a rising edge.


Verilog Synthesizable Keyword Support:
*********************************

+-------------------+------------------+---------------------+--------------------+
| Supported Keyword | NOT Sup. Keyword | Supported Operators | NOT Sup. Operators |
+-------------------+------------------+---------------------+--------------------+
| @()               | automatic        | !=                  |                    |
+-------------------+------------------+---------------------+--------------------+
| @*                | deassign         | !==                 |                    |
+-------------------+------------------+---------------------+--------------------+
| `define           | disable          | ==                  | \>\>\>             |
+-------------------+------------------+---------------------+--------------------+
| always            | edge             | ===                 |                    |
+-------------------+------------------+---------------------+--------------------+
| and               | endtask          | =\>                 |                    |
+-------------------+------------------+---------------------+--------------------+
| assign            | forever          | **                  |                    |
+-------------------+------------------+---------------------+--------------------+
| case              | task    	       | ^~                  |                    |
+-------------------+------------------+---------------------+--------------------+
| defparam          | repeat           | <<<                 |                    |
+-------------------+------------------+---------------------+--------------------+
| end               | signed           | \>=                 |                    |
+-------------------+------------------+---------------------+--------------------+
| endfunction       | task             | ||                  |                    |
+-------------------+------------------+---------------------+--------------------+
| endmodule         | generate         | ~&                  |                    |
+-------------------+------------------+---------------------+--------------------+
| begin             | genvar           | &&                  |                    |
+-------------------+------------------+---------------------+--------------------+
| default           |                  | <<                  |                    |
+-------------------+------------------+---------------------+--------------------+
| else              |                  | <=                  |                    |
+-------------------+------------------+---------------------+--------------------+
| endcase           |                  | >>                  |                    |
+-------------------+------------------+---------------------+--------------------+
| endspecify        |                  | ~^                  |                    |
+-------------------+------------------+---------------------+--------------------+
| for               |                  | ~|                  |                    |
+-------------------+------------------+---------------------+--------------------+
| function          |                  | $clog()             |                    |
+-------------------+------------------+---------------------+--------------------+
| if                |                  | -:                  |                    |
+-------------------+------------------+---------------------+--------------------+
| inout             |                  | +:                  |                    |
+-------------------+------------------+---------------------+--------------------+
| input             |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+
| integer           |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+
| localparam        |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+
| module            |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+
| nand              |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+
| negedge           |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+
| nor               |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+
| not               |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+
| or                |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+
| output            |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+
| parameter         |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+
| posedge           |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+
| reg               |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+
| specify           |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+
| while             |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+
| wire              |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+
| xnor              |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+
| xor               |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+
| macromodule       |                  |                     |                    |
+-------------------+------------------+---------------------+--------------------+

Verilog Syntax support:
*********************************

inline port declaration in the module declaration
i.e:

.. code-block:: verilog

	module a(input clk)
	...
	endmodule


Verilog NON-Synthesizable Keyword Support:
*********************************

+-------------------+------------------+---------------------+--------------------+
| Supported Keyword | NOT Sup. Keyword | Supported Operators | NOT Sup. Operators |
+-------------------+------------------+---------------------+--------------------+
| initial           | casex            |                     | &&&                |
+-------------------+------------------+---------------------+--------------------+
| specparam         | casez            |                     |                    |
+-------------------+------------------+---------------------+--------------------+
|                   | endprimitive     |                     |                    |
+-------------------+------------------+---------------------+--------------------+
|                   | endtable         |                     |                    |
+-------------------+------------------+---------------------+--------------------+
|                   | event            |                     |                    |
+-------------------+------------------+---------------------+--------------------+
|                   | force            |                     |                    |
+-------------------+------------------+---------------------+--------------------+
|                   | fork             |                     |                    |
+-------------------+------------------+---------------------+--------------------+
|                   | join             |                     |                    |
+-------------------+------------------+---------------------+--------------------+
|                   | primitive        |                     |                    |
+-------------------+------------------+---------------------+--------------------+
|                   | release          |                     |                    |
+-------------------+------------------+---------------------+--------------------+
|                   | table            |                     |                    |
+-------------------+------------------+---------------------+--------------------+
|                   | time             |                     |                    |
+-------------------+------------------+---------------------+--------------------+
|                   | wait             |                     |                    |
+-------------------+------------------+---------------------+--------------------+


Verilog Gate Level Modeling Support:
*********************************

+-------------------+------------------+
| Supported Keyword | NOT Sup. Keyword |
+-------------------+------------------+
|                   | buf              |
+-------------------+------------------+
|                   | bufif0           |
+-------------------+------------------+
|                   | bufif1           |
+-------------------+------------------+
|                   | cmos             |
+-------------------+------------------+
|                   | highz0           |
+-------------------+------------------+
|                   | highz0           |
+-------------------+------------------+
|                   | highz1           |
+-------------------+------------------+
|                   | highz1           |
+-------------------+------------------+
|                   | large            |
+-------------------+------------------+
|                   | medium           |
+-------------------+------------------+
|                   | nmos             |
+-------------------+------------------+
|                   | notif0           |
+-------------------+------------------+
|                   | notif1           |
+-------------------+------------------+
|                   | pmos             |
+-------------------+------------------+
|                   | pull0            |
+-------------------+------------------+
|                   | pull1            |
+-------------------+------------------+
|                   | pulldown         |
+-------------------+------------------+
|                   | pullup           |
+-------------------+------------------+
|                   | rcmos            |
+-------------------+------------------+
|                   | rnmos            |
+-------------------+------------------+
|                   | rpmos            |
+-------------------+------------------+
|                   | rtran            |
+-------------------+------------------+
|                   | rtranif0         |
+-------------------+------------------+
|                   | rtranif1         |
+-------------------+------------------+
|                   | scalared         |
+-------------------+------------------+
|                   | small            |
+-------------------+------------------+
|                   | strong0          |
+-------------------+------------------+
|                   | strong0          |
+-------------------+------------------+
|                   | strong1          |
+-------------------+------------------+
|                   | strong1          |
+-------------------+------------------+
|                   | supply0          |
+-------------------+------------------+
|                   | supply1          |
+-------------------+------------------+
|                   | tran             |
+-------------------+------------------+
|                   | tranif0          |
+-------------------+------------------+
|                   | tranif1          |
+-------------------+------------------+
|                   | tri              |
+-------------------+------------------+
|                   | tri0             |
+-------------------+------------------+
|                   | tri1             |
+-------------------+------------------+
|                   | triand           |
+-------------------+------------------+
|                   | trior            |
+-------------------+------------------+
|                   | vectored         |
+-------------------+------------------+
|                   | wand             |
+-------------------+------------------+
|                   | weak0            |
+-------------------+------------------+
|                   | weak0            |
+-------------------+------------------+
|                   | weak1            |
+-------------------+------------------+
|                   | weak1            |
+-------------------+------------------+
|                   | wor              |
+-------------------+------------------+

*******************
DOCUMENTING ODIN II
*******************

Any new command line options added to Odin II should be fully documented by
the print_usage() function within odin_ii.c before checking in the changes.

***************
TESTING ODIN II
***************

The verify_odin.sh scripts simulate the microbenchmarks and a larger set of benchmark
circuits. These scripts use simulation results which have been verified
against ModelSim.

After you build Odin II, run verify_odin.sh to ensure that
everything is working correctly on your system. Unlike the
verify_regression_tests.sh script, verify_odin.sh also
simulates the blif output, as well as simulating the verilog with and
without the architecture file.

Before checking in any changes to Odin II, please run both of these
scripts to ensure that both of these scripts execute correctly. If there
is a failure, use ModelSim to verify that the failure is within Odin II
and not a faulty regression test. The Odin II simulator will produce
a test.do file containing clock and input vector information for ModelSim.

When additional circuits are found to agree with ModelSim, they should
be added to these test sets. When new features are added to Odin II, new
microbenchmarks should be developed which test those features for
regression.  Use existing circuits as a template for the addition of
new circuits.

******************************
USING MODELSIM TO TEST ODIN II
******************************

ModelSim may be installed as part of the Quartus II Web Edition IDE. Load
the Verilog circuit into a new project in ModelSim. Compile the circuit,
and load the resulting library for simulation.

You may use random vectors via the -g option,
or specify your own input vectors using the -t option. When simulation is
complete, load the resulting test.do file into your ModelSim project and
execute it. You may now directly compare the vectors in the output_vectors
file with those produced by ModelSim.

To add the verified vectors and circuit to an existing test set, move the
verilog file (eg: test_circuit.v) to the test set folder. Next, move the
input_vectors file to the test set folder, and rename it test_circuit_input.
Finally, move the output_vectors file to the test set folder and rename
it test_circuit_output.

*************
CONTACT
*************

jamieson dot peter at gmail dot com
ken at unb dot ca
- We will service all requests as timely as possible, but
please explain the problem with enough detail to help.
