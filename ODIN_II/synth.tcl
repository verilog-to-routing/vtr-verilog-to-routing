if { $argc != 2 } {
    puts "The synth.tcl script requires a foldername filename to be inputed.\nFor example, synth.tcl reg reg.\nPlease try again."
} else {
	
	yosys -import
	
	# Read Yosys baseline library first.
    #read_verilog -lib -specify +/xilinx/cells_sim.v
    #read_verilog -lib +/xilinx/cells_xtra.v

	read_verilog ~/Desktop/yosys+odin/[lindex $argv 0]/[lindex $argv 1].v; 

	procs; 

	#opt;
#-param
	write_blif -top top_module -impltf ~/Desktop/yosys+odin/[lindex $argv 0]/yosys_[lindex $argv 1].blif;
}
## ~/Desktop/yosys+odin/reg/synth.tcl
