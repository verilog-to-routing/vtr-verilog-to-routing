Process with basic architecture:

Process:
cd $VTR_ROOT/workspace/Basic_Architecture/
$VTR_ROOT/vtr_flow/scripts/run_vtr_flow.py     $VTR_ROOT/workspace/basic_arch.v     $VTR_ROOT/workspace/basic_arch.xml     -temp_dir .     --route_chan_width 100

Display:
cd $VTR_ROOT/workspace/Basic_Architecture/
$VTR_ROOT/vpr/vpr     $VTR_ROOT/workspace/basic_arch.xml     Basic_Architecture --circuit_file Basic_Architecture.pre-vpr.blif     --route_chan_width 100     --analysis --disp on

