yosys -import

# the environment variable VTR_ROOT is set by Odin-II.
# Feel free to specify file paths using "$env(VTR_ROOT)/ ..."

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

# Transform the design into a new one with single top module
flatten;

	
# Transforming all RTLIL components into LUTs except for memories, adders, subtractors, 
# multipliers, DFFs with set (VCC) and clear (GND) signals, and DFFs with the set (VCC),
# clear (GND), and enable signals The Odin-II partial mapper will perform the technology
# mapping for the above-mentioned circuits

# [NOTE]: the purpose of using this pass is to keep the connectivity of internal signals  
#         in the coarse-grained BLIF file, as they were not properly connected in the 
#         initial implementation of Yosys+Odin-II, which did not use this pass
techmap */t:\$mem */t:\$memrd */t:\$add */t:\$sub */t:\$mul */t:\$dffsr */t:\$dffsre */t:\$sr */t:\$dlatch */t:\$adlatch %% %n;

# Collects memories, their port and create multiport memory cells
memory_collect; memory_dff;

# convert mem block to bram/rom

# [NOTE]: Yosys complains about expression width more than 24 bits.
# E.g. [63:0] memory [18:0] ==>  ERROR: Expression width 33554432 exceeds implementation limit of 16777216!
# mem will be handled using Odin-II
# memory_bram -rules $env(ODIN_TECHLIB)/mem_rules.txt
# techmap -map $env(ODIN_TECHLIB)/mem_map.v; 

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