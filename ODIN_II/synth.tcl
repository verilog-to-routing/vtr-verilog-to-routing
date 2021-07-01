yosys -import
	
# Read VTR baseline library first.
read_verilog -lib $env(PRIMITIVES); 

read_verilog -nomem2reg $env(TCL_CIRCUIT); 
hierarchy -check -auto-top

autoname;

procs; opt;

fsm; opt;

memory_collect; opt;

check;

flatten;

autoname;

pmuxtree;

opt -undriven -full; # -noff: potential option to remove all sdff, dffe and etc. Only dff will remain

write_blif -param -impltf $env(OUTPUT_BLIF_PATH)/$env(TCL_BLIF_NAME);

exit;