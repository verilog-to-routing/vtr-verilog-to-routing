.. _user_guide:

User guide
==========


Synthesis Arguments
-------------------

.. table::

    ===================    ==============================    =================================================================================================
           Arg                   Following Argument                                                          Description
    ===================    ==============================    =================================================================================================
     `-c`                   XML Configuration File            XML runtime directives for the syntesizer such as the Verilog file, and parametrized synthesis
     `-V`                   Verilog HDL FIle                  You may specify multiple Verilog HDL files for synthesis									   
     `-b`                   BLIF File                                                                               									
     `-S/--tcl`             Tcl Script File                   You may utilize additional commands for the Yosys elaborator        						   
     `-o`                   BLIF Output File                  full output path and file name for the BLIF output file                           		
     `-a`                   Architecture File                 You may specify multiple verilog HDL files for synthesis                        		       
     `-G`                                                     Output netlist graph in GraphViz viewable .dot format. (net.dot, opens with dotty)  		   
     `-A`                                                     Output AST graph in in GraphViz viewable .dot format.                               		   
     `-W`                                                     Print all warnings. (Can be substantial.)                                           		   
     `-h`                                                     Print help                                                                          		   
     `--coarsen`            Coarse-grained BLIF File          Performing the partial mapping for a given coarse-grained netlist in BLIF 			       
     `--elaborator`         [odin (default), yosys]           Specify the tool that should perform the HDL elaboration.      				 	           
     `--fflegalize`                                           Converts latches' sensitivity to the rising edge as required by VPR 						   
     `--show_yosys_log`                                       Showing the Yosys elaboration logs in the console window           		
    ===================    ==============================    =================================================================================================



Additional Examples using Odin-II with the Yosys elaborator
-----------------------------------------------------------

.. code-block:: bash

    ./odin_II --elaborator yosys -V <Path/to/Verilog/file>


Passes a Verilog file to Yosys for performing elaboration. 
Then, a Yosys generated coarse-grained BLIF file will be parsed by Odin-II.
The BLIF elaboration and partial mapping phases will be executed on the netlist.

.. code-block:: bash

    ./odin_II --elaborator yosys -V <Path/to/Verilog/file> --fflegalize


The same as above, except for all latches in the Yosys+Odin-II output files that will be rising edge.

.. code-block:: bash

    ./odin_II -b <Path/to/BLIF/file> --coarsen --fflegalize


Performs the BLIF elaboration and partial mapping phases on the given coarse-grained BLIF file.

.. note::

	The coarse-grained BLIF file must follow the same style and syntax as what it would be when the Yosys synthesizer generates it.  

.. code-block:: bash

	./odin_II -S <Path/to/Tcl/file> --fflegalize


Implicitly performs the synthesis by Yosys+Odin-II. The Tcl script should include Yosys commands similar to the following Tcl script to perform the elaboration. Then, the BLIF elaboration and partial mapping phases will be performed on the coarse-grained BLIF netlist.


Example of Tcl script for Yosys+Odin-II
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: tcl
 
	# FILE: $VTR_ROOT/ODIN_II/regression_test/tools/synth.tcl #
	yosys -import

	# the environment variable VTR_ROOT is set by Odin-II.

	# Read the hardware decription Verilog
	read_verilog -nomem2reg -nolatches PATH_TO_VERILOG_FILE.v;
	# Check that cells match libraries and find top module
	hierarchy -check -auto-top;

	# Make name convention more readable
	autoname;
	# Translate processes to entlist components such as MUXs, FFs and latches
	procs; opt;
	# Extraction and optimization of finite state machines
	fsm; opt;
	# Collects memories, their port and create multiport memory cells
	memory_collect; memory_dff; opt;

	# Looking for combinatorial loops, wires with multiple drivers and used wires without any driver.
	check;
	# resolve asynchronous dffs
	techmap -map $VTR_ROOT/ODIN_II/techlib/adff2dff.v;
	techmap -map $VTR_ROOT/ODIN_II/techlib/adffe2dff.v;

	# convert mem block to bram/rom
	# [NOTE]: Yosys complains about expression width more than 24 bits.
	# E.g. [63:0] memory [18:0] ==>  ERROR: Expression width 33554432 exceeds implementation limit of 16777216!
	# mem will be handled using Odin-II
	# memory_bram -rules $VTR_ROOT/ODIN_II/techlib/mem_rules.txt
	# techmap -map $VTR_ROOT/ODIN_II/techlib/mem_map.v; 

	# Transform the design into a new one with single top module
	flatten;
	# Transforms pmux into trees of regular multiplexers
	pmuxtree;
	# undirven to ensure there is no wire without drive
	opt -undriven -full; # -noff #potential option to remove all sdff and etc. Only dff will remain
	# Make name convention more readable
	autoname;
	# Print statistics
	stat;


.. note::

	The output BLIF command, i.e., ``write_blif``, is not required except for the user usage. Indeed, Odin-II automatically handles the Yosys outputting process.


Simulation Arguments
--------------------

*To activate simulation you must pass one and only one of the following argument:*

- `-g <number of random vector>`
- `-t <input vector file>`
  
Simulation always produces the folowing files:

- input\_vectors
- output\_vectors
- test.do (ModelSim)

.. table::

    ======    ==============================    ================================================================================================================
     Arg      Following Argument                Description
    ======    ==============================    ================================================================================================================
    `-g`      Number of random test vectors     Will simulate the generated netlist with the entered number of clock cycles using pseudo-random test vectors. These vectors and the resulting output vectors are written to `input_vectors` and `output_vectors` respectively.
    `-t`      Input Vector File                 Supply a predefined input vector file           											   	
    `-T`      Output Vector File                The output vectors is verified against the supplied predefined output vector file              	
    `-E`                                        Output after both edges of the clock. (Default is to output only after the falling edge.)      	
    `-R`                                        Output after rising edge of the clock only. (Default is to output only after the falling edge.)	
    `-p`      Comma Seperated List              Comma-separated list of additional pins/nodes to monitor during simulation. (view NOTES)       	
    `-U0`                                       initial register value to 0      				 											   	
    `-U1`                                       initial resigster value to 1 					 											   	
    `-UX`                                       initial resigster value to X (unknown) (DEFAULT) 											   	
    `-L`      Comma Seperated List              Comma-separated list of primary inputs to hold high at cycle 0, and low for all subsequent cycles.
    `-3`                                        Generate three valued logic. (Default is binary) 			
    ======    ==============================    ================================================================================================================



Examples
--------

Example for `-p`
~~~~~~~~~~~~~~~~

.. table::

    ======================    =======================================================
              Arg                                Following Argument       
    ======================    =======================================================
    `-p input~0,input~1`      monitors pin 0 and 1 of input                        
    `-p input`                monitors all pins of input as a single port          
    `-p input~`               monitors all pins of input as separate ports. (split)
    ======================    =======================================================


.. note::

	Matching for `-p` is done via `strstr` so general strings will match all
	similar pins and nodes. (Eg: FF\_NODE will create a single port with
	all flipflops)


Example of .xml configuration file for `-c`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: xml

	<config>
		<inputs>
			<!-- These are the output flags for the project -->
			<!-- possible types: verilog, verilog_header and blif -->
			<input_type>Verilog</input_type>
			<!-- Way of specifying multiple files in a project -->
			<input_path_and_name>PATH_TO_CIRCUIT.v</input_path_and_name>
		</inputs>
		<output>
			<!-- These are the output flags for the project -->
			<output_type>blif</output_type>
			<output_path_and_name>PATH_TO_OUTPUT_FILE</output_path_and_name>
			<target>
				<!-- This is the target device the output is being built for -->
				<arch_file>PATH_TO_ARCHITECTURE_FILE.xml</arch_file>
			</target>
		</output>
		<optimizations>
			<!-- This is where the optimization flags go -->
			<multiply size="MMM" fixed="1" fracture="0" padding="-1"/>
			<memory split_memory_width="1" split_memory_depth="PPP"/>
			<adder size="0" threshold_size="AAA"/>
		</optimizations>
		<debug_outputs>
			<!-- Various debug options -->
			<debug_output_path>.</debug_output_path>
			<output_ast_graphs>1</output_ast_graphs>
			<output_netlist_graphs>1</output_netlist_graphs>
		</debug_outputs>
	</config>


.. note::

	Hard blocks can be simulated; given a hardblock named `block` in the architecture file with an instance of it named `instance` in the file.
	First, a Verilog module including the hard block signture, similar to ``single_port_ram`` and ``dual_port_ram``, should be added to the `$VTR_ROOT/vtr_flow/primitives.v` file. Note, ``(* keep_hierarchy *)`` must be defined exactly a line before the hard block module.
	Then, write a C method with signature defined in `SRC/sim_block.h` and compile it with an output filename of `block+instance.so` in the directory you plan to invoke Yosys+Odin\_II from.

.. note::

	Additional information regarding how to compile the aforementioned file, 
	what are the restriction for a C method signature, etc. are 
	already mentioned in the Odin-II `user guide <https://docs.verilogtorouting.org/en/latest/odin/user_guide/#examples>`_.


Examples vector file for `-t` or `-T`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


.. code-block:: bash

	## Example vector input file
	GLOBAL_SIM_BASE_CLK intput_1 input_2 input_3 clk_input
	## Comment
	0 0XA 1011 0XD 0
	0 0XB 0011 0XF 1
	0 0XC 1100 0X2 0


.. code-block:: bash

	## Example vector output file
	output_1 output_2
	## Comment
	1011 0Xf
	0110 0X4
	1000 0X5

.. note::

	Please see the Odin-II `user guide <https://docs.verilogtorouting.org/en/latest/odin/user_guide/#examples>`_ for more information about the simulator.


Examples using vector files `-t` and `-T`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

	./odin_II --elaborator yosys -V <Path/to/verilog/file> -t <Path/to/Input/Vector/File> -T <Path/to/Output/Vector/File>


An error will arise if the output vector files do not match.

Without an expected vector output file the command line would be:

.. code-block:: bash

	./odin_II --elaborator yosys -V <Path/to/verilog/file> -t <Path/to/Input/Vector/File>


The generated output file can be found in the current directory under the name output_vectors.

**Example using vector files `-g`**

This function generates N amount of random input vectors for Odin-II to simulate with.

.. code-block:: bash

	./odin_II --elaborator yosys -V <Path/to/verilog/file> -g 10


This example will produce 10 autogenerated input vectors. These vectors can be found in the current directory under input_vectors and the resulting output vectors can be found under output_vectors.

Getting Help
------------

If you have any questions or concerns there are multiple outlets to express them.
There is a `google group <https://groups.google.com/forum/#!forum/vtr-users>`_ for users who have questions that is checked regularly by Odin-II team members.
If you have found a bug please make an issue in the `vtr-verilog-to-routing GitHub repository <https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues?q=is%3Aopen+is%3Aissue+label%3AOdin>`_.

Reporting Bugs and Feature Requests
-----------------------------------

**Creating an Issue on GitHub**

Yosys+Odin-II is still in development and there may be bugs present.
If Yosys+Odin-II doesn't perform as expected, it is important to create a `bug report <https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/new/choose>`_ in the GitHub repository.
There is a template included, but make sure to include micro-benchmark(s) that reproduces the bug. This micro-benchmark should be as simple as possible.
It is important to link some documentation that provides insight on what Yosys+Odin-II is doing that differs from the Standard.
Linked below is a pdf of the IEEE Standard of Verilog (2005) that could help.

`IEEE Standard for Verilog Hardware Description Language <http://staff.ustc.edu.cn/~songch/download/IEEE.1364-2005.pdf>`_

**Feature Requests**

If there are any features that the Yosys+Odin-II system overlooks or would be a great addition, please make a `feature request <https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/new/choose>`_ in the GitHub repository. There is a template provided and be as in-depth as possible.