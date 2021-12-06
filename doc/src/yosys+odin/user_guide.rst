.. _user_guide:

User guide
==========


Synthesis Arguments
-------------------

.. table::

    =======================   ==============================    =================================================================================================
             Arg                    Following Argument                                                          Description
    =======================   ==============================    =================================================================================================
     `-c`                      XML Configuration File            XML runtime directives for the syntesizer such as the Verilog file, and parametrized synthesis
     `-V`                      Verilog HDL FIle                  You may specify multiple Verilog HDL files for synthesis									   
     `-b`                      BLIF File                                                                               									
     **`-S/--tcl`**            **Tcl Script File**               **You may utilize additional commands for the Yosys elaborator**        						   
     `-o`                      BLIF Output File                  full output path and file name for the BLIF output file                           		
     `-a`                      Architecture File                 You may specify multiple verilog HDL files for synthesis                        		       
     `-G`                                                        Output netlist graph in GraphViz viewable .dot format. (net.dot, opens with dotty)  		   
     `-A`                                                        Output AST graph in in GraphViz viewable .dot format.                               		   
     `-W`                                                        Print all warnings. (Can be substantial.)                                           		   
     `-h`                                                        Print help                                                                          		   
     **`--coarsen`**           **Coarse-grained BLIF File**      **Performing the partial mapping for a given coarse-grained netlist in BLIF** 			     
     **`--elaborator`**        **[odin (default), yosys]**       **Specify the tool that should perform the HDL elaboration**  				 	         
     **`--fflegalize`**                                          **Converts latches' sensitivity to the rising edge as required by VPR** 						 
     **`--show_yosys_log`**                                      **Showing the Yosys elaboration logs in the console window**           
    =======================   ==============================    =================================================================================================



Additional Examples using Odin-II with the Yosys elaborator
-----------------------------------------------------------

.. code-block:: bash

    ./odin_II --elaborator yosys -V <Path/to/Verilog/file> --fflegalize


Passes a Verilog file to Yosys for performing elaboration. 
The BLIF elaboration and partial mapping phases will be executed on the generated netlist.
However, all latches in the Yosys+Odin-II output file will be rising edge.

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
	# Translate processes to netlist components such as MUXs, FFs and latches
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
    # To resolve Yosys internal indexed part-select circuitry
    techmap */t:$shift */t:$shiftx;

	## Utilizing the "memory_bram" command and the Verilog design provided at "$VTR_ROOT/ODIN_II/techlib/mem_map.v"
	## we could map Yosys memory blocks to BRAMs and ROMs before the Odin-II partial mapping phase.
	## However, Yosys complains about expression widths more than 24 bits.
	## E.g. reg [63:0] memory [18:0] ==> ERROR: Expression width 33554432 exceeds implementation limit of 16777216!
	## Although we provided the required design files for this process (located in ODIN_II/techlib), we will handle
	## memory blocks in the Odin-II BLIF elaborator and partial mapper. 
	# memory_bram -rules $VTR_ROOT/ODIN_II/techlib/mem_rules.txt
	# techmap -map $VTR_ROOT/ODIN_II/techlib/mem_map.v; 

	# Transform the design into a new one with single top module
	flatten;
	# Transforms pmux into trees of regular multiplexers
	pmuxtree;
    # To possibly reduce words size
    wreduce;
	# "undirven" to ensure there is no wire without drive
    # "opt_muxtree" removes dead branches, "opt_expr" performs constant folding,
    # removes "undef" inputs from mux cells, and replaces muxes with buffers and inverters.
    # "-noff" a potential option to remove all sdff and etc. Only dff will remain
	opt -undriven -full; opt_muxtree; opt_expr -mux_undef -mux_bool -fine;;;
	# Make name convention more readable
	autoname;
	# Print statistics
	stat;
	# Output BLIF
	write_blif -param -impltf TCL_BLIF;


.. note::

	The output BLIF command, i.e., ``write_blif``, is not required except for the user usage. Indeed, Odin-II automatically handles the Yosys outputting process.


Simulation Arguments
--------------------

.. note::
    Yosys+Odin-II makes use of the Odin-II simulator. 
    For more information please see the Odin-II `Simulation Arguments <https://docs.verilogtorouting.org/en/latest/odin/user_guide/#simulation-arguments>`_.

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
	mentioned in the Odin-II `simulation examples <https://docs.verilogtorouting.org/en/latest/odin/user_guide/#examples>`_.

Examples using input/output vector files
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

	./odin_II --elaborator yosys -V <Path/to/verilog/file> -t <Path/to/Input/Vector/File> -T <Path/to/Output/Vector/File>


A mismatch error will arise if the output vector files do not match with the benchmark output vector, located in the `verilog` directory.

Getting Help
------------

.. note::

    For more information please see Odin-II `Getting Help <https://docs.verilogtorouting.org/en/latest/odin/user_guide/#getting-help>`_.


Reporting Bugs and Feature Requests
-----------------------------------

**Creating an Issue on GitHub**

.. note::

    For more information please see `Issue on GitHub <https://docs.verilogtorouting.org/en/latest/odin/user_guide/#creating-an-issue-on-github>`_.


**Feature Requests**

If there are any features that the Yosys+Odin-II system overlooks or would be a great addition, please make a `feature request <https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/new/choose>`_ in the GitHub repository. There is a template provided and be as in-depth as possible.