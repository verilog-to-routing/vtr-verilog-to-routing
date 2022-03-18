NOTE:

- vtr_flow/arch -> contains FPGA architectures to benchmark
- vtr_flow/benchmarks/verilog -> contains circuits to describe functions to produce
- vtr_flow/tasks -> contains list of combination of architecturs and circuits to run multiple benchmarkings

To run a benchmark:

- Run $VTR_ROOT/vtr_flow/scripts/run_vtr_task.py "Name of the tasks folder"
- This creates a folder in $VTR_ROOT/vtr_flow/tasks/"Name of the tasks folder"/run###/ which contains all files of the vtr_flow which can be analysed
- The flow can then be displayed by rooting to the run### folder and using the following to reprocess which the capability of using GUI (I suppose the temp directory specified): $VTR_ROOT/vtr_flow/scripts/run_vtr_flow.py     $VTR_ROOT/vtr_flow/benchmarks/verilog/"Name of verilog to use"     $VTR_ROOT/vtr_flow/arch/"Folder and name of architecture"     -temp_dir .     --route_chan_width 100
- Finally to display using this command: $VTR_ROOT/vpr/vpr     $VTR_ROOT/vtr_flow/arch/timing/EArch.xml     "name of .v file used (without the .v)" --circuit_file "name of .v file used (without the .v)".pre-vpr.blif     --route_chan_width 100     --analysis --disp on


VTR -> Suite of all following software (which are not in order of execution)
FPGA Architecture Description -> XML file describing architecture
VPR -> Place and Route, place every gate according to the layout of a given architecture and give the wiring
Odin II -> Logic Synthesis and Elaboration, converts the hardware description (.v) into netlist (.blif) meaning into FF, LUT or other small block with info on their interconnection
ABC -> Optimization and Technology Mapping, optimizes the functions from the netlist to reduce the amount of gates to use
