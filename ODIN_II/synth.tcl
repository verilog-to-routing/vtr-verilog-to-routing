yosys -import
	
# Read Yosys baseline library first.
read_verilog -lib -specify +/xilinx/cells_sim.v;
read_verilog -lib +/xilinx/cells_xtra.v

read_verilog $env(FILE_PATH); 
hierarchy -check -auto-top

autoname;
#expose -dff;
procs; 

opt;

fsm;

opt;

check;

flatten;

write_blif -top top_module -param -impltf $env(OUTPUT_BLIF_PATH)/$env(BLIF_NAME);

exit;