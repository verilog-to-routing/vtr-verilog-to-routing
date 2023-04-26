###########################################################
# Design MLP_2_phase_optimization_step_2 file locations
###########################################################

Overview:
	- This benchmark is very similar to the MLP_co_optimization_benchmark found in
	  '$VTR_ROOT/vtr_flow/benchmarks/noc/Large_Designs/MLP/MLP-co_optimization/'
	
	- The only difference is that this benchmark has a constraints file which locks
	  down the NoC routers (the router positions are derived from the
	  MLP_2_phase_optimization_step_1 benchmark, which can be found in 
	  '$VTR_ROOT/vtr_flow/benchmarks/noc/Large_Designs/MLP/MLP_2_phase_optimization/MLP_2_phase_optimization_step_1/')
	
	- Due to the similarities, the traffic flows file (.flows) for this benchmark
	  can taken from '$VTR_ROOT/vtr_flow/benchmarks/noc/Large_Designs/MLP/MLP_co_optimization/mlp_co_optimization.flows'
	  
	- The netlist for this benchmark is also the exact same as the MLP_co_optimization benchmark,
	  so the verilog source file for this benchmark can be taken from
	  '$VTR_ROOT/vtr_flow/benchmarks/noc/Large_Designs/MLP/MLP_co_optimization/verilog/mlp_co_optimization.v'
