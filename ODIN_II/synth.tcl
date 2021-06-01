yosys -import
	
# Read Yosys baseline library first.
read_verilog -lib -specify +/xilinx/cells_sim.v;
read_verilog -lib +/xilinx/cells_xtra.v

read_verilog $env(TCL_CIRCUIT); 
hierarchy -check -auto-top

autoname;
#expose -dff;
procs; 

opt;

fsm;

opt;

check;

flatten;

autoname;

write_blif -param -impltf $env(OUTPUT_BLIF_PATH)/$env(TCL_BLIF_NAME);

exit;