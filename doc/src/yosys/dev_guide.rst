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


How to add new changes?
-----------------------

The Yosys synthesis commands, including the generic synthesis and additional VTR specific configurations, are provided
in `synthesis.ys <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/misc/yosyslib/synthesis.ys>`_. To make changes in the overall Yosys synthesis flow, the `synthesis.ys <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/misc/yosyslib/synthesis.ys>`_
script is perhaps the first file developers may require to change.

Moreover, the `yosys_models.v <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/misc/yosyslib/yosys_models.v>`_ file includes the required definitions for Yosys to how it should infer implicit
memories and instantiate arithmetic operations, such as addition, subtraction, and multiplication. Therefore, to alter these 
behaviours or add more regulations such as how Yosys should behave when facing other arithmetic operations, for example modulo and division,
the `yosys_models.v <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/misc/yosyslib/yosys_models.v>`_ Verilog file is required to be modified.

Except for `single_port_ram.v <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/misc/yosyslib/single_port_ram.v>`_ and `dual_port_ram.v <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/misc/yosyslib/dual_port_ram.v>`_ Verilog files that perform the depth splitting
process, the other files are defined as black-box, i.e., their declarations are required while no definition is needed. To add new black-box
components, developers should first provide the corresponding Verilog files similar to the `adder.v <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/misc/yosyslib/adder.v>`_. Then, a new  `read_verilog -lib TTT/NEW_BB.v`
command should be added to the Yosys synthesis script. If there is an implicit inference of the new black-box component, the `yosys_models.v <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/misc/yosyslib/yosys_models.v>`_
Verilog file must also be modified, as mentioned earlier.


Yosys Synthesis Script File
---------------------------

.. code-block:: tcl

	# XXX (input circuit) is replaced with filename by the run_vtr_flow script
	read_verilog -nolatches XXX

	# These commands follow the generic `synth`
	# command script inside Yosys
	# The -libdir argument allows Yosys to search the current 
	# directory for any definitions to modules it doesn't know
	# about, such as hand-instantiated (not inferred) memories
	hierarchy -check -auto-top -libdir .
	proc

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
	techmap -map YYY */t:$mul */t:$mem */t:$sub */t:$add
	opt -fast -full

	memory_map
	# Taking care to remove any undefined muxes that
	# are introduced to promote resource sharing
	opt -full

	# Then techmap all other `complex` blocks into basic
	# (lookup table) logic
	techmap 
	opt -fast

	# We read the definitions for all the VTR primitives
	# as blackboxes
	read_verilog -lib TTT/adder.v
	read_verilog -lib TTT/multiply.v
	read_verilog -lib SSS     #(SSS) will be replaced by single_port_ram.v by python script
	read_verilog -lib DDD     #(DDD) will be replaced by dual_port_ram.v by python script

	# Rename singlePortRam to single_port_ram
	# Rename dualPortRam to dualZ_port_ram
	# rename function of Yosys not work here
	# since it may outcome hierarchy error
	read_verilog SSR         #(SSR) will be replaced by spram_rename.v by python script
	read_verilog DDR         #(DDR) will be replaced by dpram_rename.v by python script

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
	# the internal `$true`/`$false` signals vcc/gnd, but
	# switch `-impltf` doesn't output them
	# ZZZ will be replaced by run_vtr_flow.pl
	write_blif -true + vcc -false + gnd -undef + unconn -blackbox ZZZ

**Algorithm 1** - The Yosys Tcl Script File