yosys -import

# Read VTR baseline library first
read_verilog -lib $env(PRIMITIVES);

# Read the hardware decription Verilog
read_verilog -nomem2reg $env(TCL_CIRCUIT);
# Check that cells match libraries and find top module
hierarchy -check -auto-top;

# Make name convention more readable
autoname;
# Translate processes to entlist components such as MUXs, FFs and latches
procs; opt;
# Extraction and optimization of finite state machines
fsm; opt;
# Collects memories, their port and create multiport memory cells
memory_collect; opt;
# Looking for combinatorial loops, wires with multiple drivers and used wires without any driver.
check;
# Transform the design into a new one with single top module
flatten;
# Make name convention more readable
autoname;
# Transforms pmux into trees of regular multiplexers
pmuxtree;
# undirven to ensure there is no wire without drive
opt -undriven;# -noff;

# param is to print non-standard cells attributes
# impltf is also used not to show the definition of primary netlist ports, i.e. VCC, GND and PAD, in the output.
write_blif -param -impltf $env(OUTPUT_BLIF_PATH)/$env(TCL_BLIF_NAME);

exit;