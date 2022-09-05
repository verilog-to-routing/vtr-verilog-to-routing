.. _dev_guide:

Developers Guide
================

The approach of utilizing Yosys in VTR is mainly driven by what Eddie Hung proposed
for the `VTR-to-Bitstream <http://eddiehung.github.io/vtb.html>`_ (VTB), based upon VTR 7.
Although some files, such as `yosys_models.v <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/misc/yosyslib/yosys_models.v>`_
and `multiply.v <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/misc/yosyslib/multiply.v>`_, are directly
copied from the VTB project, the other files have been subjected to a few changes due to significant 
alterations from VTR 7 to the current version of VTR. Additionally, Hung's approach was specifically 
proposed for Xilinx Vertix-6 architecture. As a result, relevant changes to the remainder
of Yosys library files have been applied to make them compatible with the current VTR version and support routine architectures
used in the VTR regression tests.

What is new compared to the VTB files?
--------------------------------------

Changes applied to the VTB files are outlined as follows:

 1. Replacing Vertix-6 adder black-box (`xadder`) with the conventional adder used in the current version of VTR.
 2. If required, performing a recursive depth splitting for memory hard blocks, i.e., `single_port_ram` and `dual_port_ram`, to make them adaptable with the VTR flow configurations.
 3. Converting DFFs with asynchronous reset to synchronous form using `adff2dffe.v <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/ODIN_II/techlib/adff2dff.v>`_ and `adffe2dffe.v <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/ODIN_II/techlib/adffe2dff.v>`_.
 4. Adding the `dffunmap` command to Yosys synthesis script to transform complex DFF sub-circuits, such as SDFFE (DFF with synchronous reset and enable), to their soft logic implementation, i.e., the combination of multiplexers and latches.
 5. Removing the ABC commands from the Yosys synthesis script and letting the VTR flow's ABC stage performs the technology mapping. 

.. note:: 
	The LUT size is considered the one defined in the architecture file as the same as the regular VTR flow


Working with Complex Blocks and How to Instantiate them?
-------------------------------------------------------

The Yosys synthesis commands, including the generic synthesis and additional VTR specific configurations, are provided
in `synthesis.tcl <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/misc/yosyslib/synthesis.tcl>`_. To make changes in the overall Yosys synthesis flow, the `synthesis.tcl <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/misc/yosyslib/synthesis.tcl>`_
script is perhaps the first file developers may be required to change.

Moreover, the `yosys_models.v <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/misc/yosyslib/yosys_models.v>`_ file includes the required definitions for Yosys to how it should infer implicit
memories and instantiate arithmetic operations, such as addition, subtraction, and multiplication. Therefore, to alter these 
behaviours or add more regulations such as how Yosys should behave when facing other arithmetic operations, for example modulo and division,
the `yosys_models.v <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/misc/yosyslib/yosys_models.v>`_ Verilog file is required to be modified.

Except for `single_port_ram.v <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/misc/yosyslib/single_port_ram.v>`_ and `dual_port_ram.v <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/misc/yosyslib/dual_port_ram.v>`_ Verilog files that perform the depth splitting
process, the other files are defined as black-box, i.e., their declarations are required while no definition is needed. To add new black-box
components manually, developers should first provide the corresponding Verilog files similar to the `adder.v <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/misc/yosyslib/adder.v>`_. Then, a new  `read_verilog -lib TTT/NEW_BB.v`
command should be added to the Yosys synthesis script. If there is an implicit inference of the new black-box component, the `yosys_models.v <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/misc/yosyslib/yosys_models.v>`_
Verilog file must also be modified, as mentioned earlier.

It is worth noting that the VTR flow scripts for running Yosys standalone as the VTR frontend are designed to automatically provide the black box declaration of complex blocks defined in the architecture XML file for Yosys.
Technically, by running the ``run_vtr_flow.py`` script with the Yosys frontend, the ``write_arch_bb`` routine, defined in the ``libarchfpga``, is executed initially to extract the information of complex blocks defined in the architecture file.
Then, the routine generates a file, including the black box declaration of the complex blocks in the Verilog format.
The output file is named ``arch_dsps.v`` by default, found in the project destination directory.

Instantiation of complex blocks is similar to the explicit instantiation of VTR primitives in HDL format.
The ``write_arch_bb`` generates a Verilog module with the same name as the complex block model.
Module ports are also defined according to the port declaration provided in the architecture file.
For instance, the HDL instantiation of the ``multiply_fp_clk`` complex block defined in the ``COFFE_22nm/k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml`` architecture file is as follows:

.. code-block:: verilog
	...
	multiply_fp_clk instance_name(
		.b(i_b),		// input	[31:0]	b
		.a(i_a), 		// input	[31:0]	a
		.clk(i_clk), 	// input	[0:0]	clk
		.out(i_out)		// output	[31:0]	out
	);
	...

**Algorithm 1** - Custom Complex Blocks HDL Instantiation

Yosys Synthesis Script File
---------------------------

.. code-block:: Tcl

    yosys -import
    
    # Read the HDL file with pre-defined parser in the run_vtr_flow script
    # XXX (input circuit) is replaced with filename by the run_vtr_flow script
    if {$env(PARSER) == "surelog" } {
    	puts "Using Yosys read_uhdm command"
    	plugin -i systemverilog
    	yosys -import
    	read_uhdm -debug XXX
    } elseif {$env(PARSER) == "yosys-plugin" } {
    	puts "Using Yosys read_systemverilog command"
    	plugin -i systemverilog
    	yosys -import
    	read_systemverilog -debug XXX
    } elseif {$env(PARSER) == "yosys" } {
    	puts "Using Yosys read_verilog command"
    	read_verilog -sv -nolatches XXX
    } else {
    	error "Invalid PARSER"
    }
    
    # read the custom complex blocks in the architecture
    read_verilog -lib CCC
    
    # These commands follow the generic `synth'
    # command script inside Yosys
    # The -libdir argument allows Yosys to search the current 
    # directory for any definitions to modules it doesn't know
    # about, such as hand-instantiated (not inferred) memories
    hierarchy -check -auto-top -libdir .
    procs
    
    # Check that there are no combinational loops
    scc -select
    select -assert-none %
    select -clear
    
    
    opt_expr
    opt_clean
    check
    opt -nodffe -nosdff
    fsm
    opt
    wreduce
    peepopt
    opt_clean
    share
    opt
    memory -nomap
    opt -full
    
    # Transform all async FFs into synchronous ones
    techmap -map +/adff2dff.v
    techmap -map TTT/../../../ODIN_II/techlib/adffe2dff.v
    
    # Map multipliers, DSPs, and add/subtracts according to yosys_models.v
    techmap -map YYY */t:\$mul */t:\$mem */t:\$sub */t:\$add
    opt -fast -full
    
    memory_map
    # Taking care to remove any undefined muxes that
    # are introduced to promote resource sharing
    opt -full
    
    # Then techmap all other `complex' blocks into basic
    # (lookup table) logic
    techmap 
    opt -fast
    
    # read the definitions for all the VTR primitives
    # as blackboxes
    read_verilog -lib TTT/adder.v
    read_verilog -lib TTT/multiply.v
    #(SSS) will be replaced by single_port_ram.v by python script
    read_verilog -lib SSS
    #(DDD) will be replaced by dual_port_ram.v by python script
    read_verilog -lib DDD
    
    # Rename singlePortRam to single_port_ram
    # Rename dualPortRam to dual_port_ram
    # rename function of Yosys not work here
    # since it may outcome hierarchy error
    #(SSR) will be replaced by spram_rename.v by python script
    read_verilog SSR
    #(DDR) will be replaced by dpram_rename.v by python script
    read_verilog DDR
    
    # Flatten the netlist
    flatten
    # Turn all DFFs into simple latches
    dffunmap
    opt -fast -noff
    
    # Lastly, check the hierarchy for any unknown modules,
    # and purge all modules (including blackboxes) that
    # aren't used
    hierarchy -check -purge_lib
    tee -o /dev/stdout stat
    
    autoname
    
    # Then write it out as a blif file, remembering to call
    # the internal `$true'/`$false' signals vcc/gnd, but
    # switch `-impltf' doesn't output them
    # ZZZ will be replaced by run_vtr_flow.pl
    write_blif -true + vcc -false + gnd -undef + unconn -blackbox ZZZ
    
**Algorithm 2** - The Yosys Tcl Script File