yosys -import
	
# Read VTR baseline library first.
read_verilog -lib $env(PRIMITIVES); 

read_verilog $env(TCL_CIRCUIT); 
hierarchy -check -auto-top

autoname;

procs; opt;

fsm; opt;

memory_collect; memory_nordff; opt;

check;

flatten;

autoname;

pmuxtree;

opt -undriven;

write_blif -param -impltf $env(OUTPUT_BLIF_PATH)/$env(TCL_BLIF_NAME);

exit;