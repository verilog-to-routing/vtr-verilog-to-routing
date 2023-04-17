###################################################
# NoC MLP Designs Files
###################################################

This directory contains benchmarks which handle the parallel processing of
Multilayer Perceptrons (MLPs). Each benchmark is designed to handle different MLP
configurations.

Benchmark Structure:
	|---<Benchmark>
	|	|---<Benchmark>.blif  - Is the netlist file for the given benchmark
		|---<Benchmark>.flows - Is the NoC traffic flows file associated with the given benchmark
					(A benchmark can have multiple traffic flows files)
		|---verilog	      - Contains design files needed to generate the netlist file for the benchmark
	|---shared_verilog	      - Contains design files needed by all benchmarks to generate thier netlist files

Running the benchmarks:
	Pre-requisite
		- Ensure VPR is built (refer to 'https://docs.verilogtorouting.org/en/latest/' for build instructions)
		- Set 'VTR_ROOT' as environment variable pointing to the location of the VTR source tree
		- Ensure python version 3.6.9 or higher is installed
		- Copy over the netlist files from 'https://drive.google.com/drive/folders/135QhmfgUaGnK2ZEfbfEXtdm1BfS7YoG7?usp=sharing'.
	          The file structure in the previous link is similiar to structure found in '$VTR_ROOT/vtr_flow/benchmarks/noc/Large_Designs/MLP'.
		  Place the netlist files in the appropriate folder locations.
	
	Running single instance:
		- Below is the general command which can be used to run any MLP benchmark except MLP_2_phase_optimization/:
		  
		  $VTR_ROOT/build/vpr/vpr $VTR_ROOT/vtr_flow/arch/noc/mesh_noc_topology/mlp_benchmarks.stratixiv_arch.timing_with_a_embedded_4X4_mesh_noc_topology.xml \
                  <circuit_netlist_file_location>.blif  --device EP4SE820 --route_chan_width 300 --noc on --noc_flows_file <flows_file_location>.flows \
		  --noc_routing_algorithm xy_routing

		- For example, to run VPR on the MLP_1 benchmark the command is as follows:

		  $VTR_ROOT/build/vpr/vpr $VTR_ROOT/vtr_flow/arch/noc/mesh_noc_topology/mlp_benchmarks.stratixiv_arch.timing_with_a_embedded_4X4_mesh_noc_topology.xml \
                  $VTR_ROOT/vtr_flow/benchmarks/noc/Large_Designs/MLP/MLP_1/mlp_1.blif  --device EP4SE820 --route_chan_width 300 \
                  --noc on --noc_flows_file $VTR_ROOT/vtr_flow/benchmarks/noc/Large_Designs/MLP/MLP_1/mlp_1.flows \
		  --noc_routing_algorithm xy_routing

	Running multiple instances:
		- The script ($VTR_ROOT/vtr_flow/scripts/noc/noc_benchmark_test.py) runs multiple runs of VPR on a given benchmark and outputs averaged metric
	          information over all runs
		- For example, the command to run 5 iterations of VPR (using 1 thread) on the MLP_1 benchmark using the above script is as follows:
		
		  <path_to_python_interpreter> $VTR_ROOT/vtr_flow/scripts/noc/noc_benchmark_test.py \
		  -tests_folder_location $VTR_ROOT/vtr_flow/benchmarks/noc/Large_Designs/MLP/MLP_1/mlp_1.blif \
		  -arch_file $VTR_ROOT/vtr_flow/arch/noc/mesh_noc_topology/mlp_benchmarks.stratixiv_arch.timing_with_a_embedded_4X4_mesh_noc_topology.xml \
		  -vpr_executable $VTR_ROOT/build/vpr/vpr --device EP4SE820 -flow_file $VTR_ROOT/vtr_flow/benchmarks/noc/Large_Designs/MLP/MLP_1/mlp_1.flows \
		  -noc_routing_algorithm xy_routing -number_of_seeds 5 -number_of_threads 1 -route

	        - The above command will generate an output file in the run directory that contains all the place and route metrics. This is a txt file with a name which matches the
		  the flows file provided. So for the command shown above the output file is 'mlp_1.txt'

	Special benchmarks:
		MLP_2_phase_optimization/MLP_2_phase_optimization_step_1
			- This benchmark is unique in that it mainly consists of NoC routers to prioritize NoC placement and uses a custom architecture file
			- To run a single instance of this benchmark just change the arch file from the command above to 
			  '$VTR_ROOT/vtr_flow/arch/noc/mesh_noc_topology/mlp_2_phase_optimization_step_1.stratixiv_arch.timing_with_a_embedded_4X4_mesh_noc_topology.xml'
			- To run the benchmark using the automated script just pass in the arch file below with the '-arch_file' option:
			  '$VTR_ROOT/vtr_flow/arch/noc/mesh_noc_topology/mlp_2_phase_optimization_step_1.stratixiv_arch.timing_with_a_embedded_4X4_mesh_noc_topology.xml'
		
		MLP_2_phase_optimization/MLP_2_phase_optimization_step_2
			- This benchmark is unique in that the NoC placement is already provided through the MLP_2_phase_optimization_step_1 benchmark. So the placement
			  of the NoC routers needs to be locked. A 
			- To run a single instance of this benchmark, pass in the following command line parameter and its value to the command shown above:
			  '--fix_clusters $VTR_ROOT/vtr_flow/benchmarks/noc/Large_Designs/MLP/MLP_2_phase_optimization/MLP_2_phase_optimization_step_2/MLP_two_phase_optimization_step_two_constraints.place'
			- To run the benchmarkusing the automated script just pass in the following command line parameter and its value to the script command above:
			  '-fix_clusters $VTR_ROOT/vtr_flow/benchmarks/noc/Large_Designs/MLP/MLP_2_phase_optimization/MLP_2_phase_optimization_step_2/MLP_two_phase_optimization_step_two_constraints.place'
	Expected run time:
		- These benchmarks are quite large so the maximum expected run time for a single run is a few hours
		- To speed up the run time with multiple VPR runs the thread count can be increased from 1. Set thread count equal to number seeds for fastest run time.
		  