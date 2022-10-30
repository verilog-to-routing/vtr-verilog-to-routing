.. _user_guide:

User guide
==========


Synthesis Arguments
-------------------

.. table::

    =======================   ==============================    =====================================================================================================================================================================
             Arg                    Following Argument                                                          Description
    =======================   ==============================    =====================================================================================================================================================================
     `-c`                      XML Configuration File            XML runtime directives for the syntesizer such as the Verilog file, and parametrized synthesis
     `-v`                      Verilog HDL File                  You may specify multiple space-separated Verilog HDL files for synthesis									   
     **`-s`**                  **SystemVerilog HDL File**        **You may specify multiple space-separated SystemVerilog HDL files for synthesis**									   
     **`-u`**                  **UHDM File**                     **You may specify multiple space-separated UHDM files for synthesis**									   
     `-b`                      BLIF File                         You may **not** specify multiple BLIF files as only single input BLIF file is accepted                                                      									
     **`-S/--tcl`**            **Tcl Script File**               **You may utilize additional commands for the Yosys elaborator**        						   
     `-o`                      BLIF Output File                  full output path and file name for the BLIF output file                           		
     `-a`                      Architecture File                 You may specify multiple space-separated verilog HDL files for synthesis                        		       
     `-G`                                                        Output netlist graph in GraphViz viewable .dot format. (net.dot, opens with dotty)  		   
     `-A`                                                        Output AST graph in in GraphViz viewable .dot format.                               		   
     `-W`                                                        Print all warnings. (Can be substantial.)                                           		   
     `-h`                                                        Print help                                                                          		   
     **`--coarsen`**           **Coarse-grained BLIF File**      **Performing the partial mapping for a given coarse-grained netlist in BLIF** 			     
     **`--elaborator`**        **[odin (default), yosys]**       **Specify the tool that should perform the HDL elaboration**  				 	         
     **`--fflegalize`**                                          **Converts latches' sensitivity to the rising edge as required by VPR** 						 
     **`--show_yosys_log`**                                      **Showing the Yosys elaboration logs in the console window**           
     **`--decode_names`**                                        **Enable extracting hierarchical information from Yosys coarse-grained BLIF file for signal naming (the VTR flow scripts take advantage of this option by default)**
    =======================   ==============================    =====================================================================================================================================================================



Additional Examples using Odin-II with the Yosys elaborator
-----------------------------------------------------------

.. code-block:: bash

    # Elaborate the input file using Yosys conventional Verilog parser
    ./odin_II --elaborator yosys -v <path/to/Verilog/File> --fflegalize

    # Elaborate the input file using the Yosys-SystemVerilog plugin if installed, otherwise the Yosys conventional Verilog parser 
    ./odin_II --elaborator yosys -s <path/to/SystemVerilog/File> --fflegalize

    # Elaborate the input file using the Surelog plugin if installed, otherwise failure on the unsupported type
    ./odin_II --elaborator yosys -u <path/to/UHDM/File> --fflegalize


Passes a Verilog/SystemVerilog/UHDM file to Yosys to perform elaboration. 
The BLIF elaboration and partial mapping phases will be executed on the generated netlist.
However, all latches in the Yosys+Odin-II output file will be rising edge.

.. code-block:: bash

    ./odin_II --elaborator yosys -v <Path/to/Verilog/file> --decode_names


Performs the design elaboration by Yosys parsers and generates a coarse-grained netlist in BLIF.
Odin-II then extracts the hierarchical information of subcircuits to use for signal naming when reading the coarse-grained BLIF file.
The BLIF elaboration and partial mapping phases will be executed on the generated netlist.

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
    
    # Read VTR baseline library first
    read_verilog -nomem2reg $env(ODIN_TECHLIB)/../../vtr_flow/primitives.v
    setattr -mod -set keep_hierarchy 1 single_port_ram
    setattr -mod -set keep_hierarchy 1 dual_port_ram
    
    # Read the HDL file with pre-defined parer in the "run_yosys.sh" script
    if {$env(PARSER) == "surelog" } {
    	puts "Using Yosys read_uhdm command"
    	plugin -i systemverilog;
    	yosys -import
    	read_uhdm -debug $env(TCL_CIRCUIT);
    } elseif {$env(PARSER) == "yosys-plugin" } {
    	puts "Using Yosys read_systemverilog command"
    	plugin -i systemverilog;
    	yosys -import
    	read_systemverilog -debug $env(TCL_CIRCUIT)
    } elseif {$env(PARSER) == "yosys" } {
    	puts "Using Yosys read_verilog command"
    	read_verilog -sv -nomem2reg -nolatches $env(TCL_CIRCUIT);
    } else {
    	error "Invalid PARSER"
    }
    
    # Check that cells match libraries and find top module
    hierarchy -check -auto-top -purge_lib;
    
     
    # Translate processes to netlist components such as MUXs, FFs and latches
    # Transform the design into a new one with single top module
    proc; flatten; opt_expr; opt_clean;
    
    # Looking for combinatorial loops, wires with multiple drivers and used wires without any driver.
    # "-nodffe" to disable dff -> dffe conversion, and other transforms recognizing clock enable
    # "-nosdff" to disable dff -> sdff conversion, and other transforms recognizing sync resets
    check; opt -nodffe -nosdff;
    
    # Extraction and optimization of finite state machines
    fsm; opt;
    # To possibly reduce word sizes by Yosys
    wreduce;
    # To applies a collection of peephole optimizers to the current design.
    peepopt; opt_clean;
     
    # To merge shareable resources into a single resource. A SAT solver
    # is used to determine if two resources are share-able
    share; opt;
    
    # Use a readable name convention
    # [NOTE]: the 'autoname' process has a high memory footprint for giant netlists
    # we run it after basic optimization passes to reduce the overhead (see issue #2031)
    autoname;    

    # Looking for combinatorial loops, wires with multiple drivers and used wires without any driver.
    check;
    # resolve asynchronous dffs
    techmap -map $env(ODIN_TECHLIB)/adff2dff.v;
    techmap -map $env(ODIN_TECHLIB)/adffe2dff.v;
    
    # Yosys performs various optimizations on memories in the design. Then, it detects DFFs at
    # memory read ports and merges them into the memory port. I.e. it consumes an asynchronous
    # memory port and the flip-flops at its interface and yields a synchronous memory port.
    # Afterwards, Yosys detects cases where an asynchronous read port is only connected via a mux
    # tree to a write port with the same address. When such a connection is found, it is replaced
    # with a new condition on an enable signal, allowing for removal of the read port. Finally
    # Yosys collects memories, their port and create multiport memory cells.
    opt_mem; memory_dff; opt_clean; opt_mem_feedback; opt_clean; memory_collect;
    
    # convert mem block to bram/rom
    
    # [NOTE]: Yosys complains about expression width more than 24 bits.
    # E.g. [63:0] memory [18:0] ==>  ERROR: Expression width 33554432 exceeds implementation limit of 16777216!
    # mem will be handled using Odin-II
    # memory_bram -rules $env(ODIN_TECHLIB)/mem_rules.txt
    # techmap -map $env(ODIN_TECHLIB)/mem_map.v; 
    
    # Transforming all RTLIL components into LUTs except for memories, adders, subtractors, 
    # multipliers, DFFs with set (VCC) and clear (GND) signals, and DFFs with the set (VCC),
    # clear (GND), and enable signals The Odin-II partial mapper will perform the technology
    # mapping for the above-mentioned circuits
    
    # [NOTE]: the purpose of using this pass is to keep the connectivity of internal signals  
    #         in the coarse-grained BLIF file, as they were not properly connected in the 
    #         initial implementation of Yosys+Odin-II, which did not use this pass
    techmap */t:\$mem */t:\$memrd */t:\$add */t:\$sub */t:\$mul */t:\$dffsr */t:\$dffsre */t:\$sr */t:\$dlatch */t:\$adlatch %% %n;
    
    # Transform the design into a new one with single top module
    flatten;
    
    # To possibly reduce word sizes by Yosys and fine-graining the basic operations
    wreduce; simplemap */t:\$dffsr */t:\$dffsre */t:\$sr */t:\$dlatch */t:\$adlatch %% %n;
    # Turn all DFFs into simple latches
    dffunmap; opt -fast -noff;
    
    # Check the hierarchy for any unknown modules, and purge all modules (including blackboxes) that aren't used
    hierarchy -check -purge_lib;
    
    # "undirven" to ensure there is no wire without drive
    # "opt_muxtree" removes dead branches, "opt_expr" performs constant folding,
    # removes "undef" inputs from mux cells, and replaces muxes with buffers and inverters.
    # "-noff" a potential option to remove all sdff and etc. Only dff will remain
    opt -undriven -full; opt_muxtree; opt_expr -mux_undef -mux_bool -fine;;;
    # Make name convention more readable
    autoname;
    # Print statistics
    stat;
    
    write_blif -param -impltf $env(TCL_BLIF);


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
			<!-- These are the input flags for the project -->
			<!-- possible types: [verilog, verilog_header, systemverilog, systemverilog_header, uhdm, blif] -->
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

    # Elaborate the input file using Yosys conventional Verilog parser
    ./odin_II --elaborator yosys -v <Path/to/Verilog/file> -t <Path/to/Input/Vector/File> -T <Path/to/Output/Vector/File>

    # Elaborate the input file using the Yosys-SystemVerilog plugin if installed, otherwise the Yosys conventional Verilog parser 
    ./odin_II --elaborator yosys -s <Path/to/SystemVerilog/file> -t <Path/to/Input/Vector/File> -T <Path/to/Output/Vector/File>
    
    # Elaborate the input file using the Surelog plugin if installed, otherwise failure on the unsupported type
    ./odin_II --elaborator yosys -u <Path/to/UHDM/file> -t <Path/to/Input/Vector/File> -T <Path/to/Output/Vector/File>


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