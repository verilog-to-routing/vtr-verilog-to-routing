###################################################
# NoC Synthetic Designs Files
###################################################

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
	
	Running single instance:
		- Below is the general command which can be used to run any synthetic benchmark:
		  
		  $VTR_ROOT/build/vpr/vpr $VTR_ROOT/vtr_flow/arch/noc/mesh_noc_topology/stratixiv_arch.timing_with_a_embedded_10X10_mesh_noc_topology.xml \
                  <circuit_netlist_file_location>.blif  --device EP4SE20 --route_chan_width 300 --noc on --noc_flows_file <flows_file_location>.flows \
		  --noc_routing_algorithm xy_routing

		- For example, to run VPR on the complex_2_noc_1D_chain benchmark the command is as follows:

		  $VTR_ROOT/build/vpr/vpr $VTR_ROOT/vtr_flow/arch/noc/mesh_noc_topology/stratixiv_arch.timing_with_a_embedded_10X10_mesh_noc_topology.xml \
                  $VTR_ROOT/vtr_flow/benchmarks/noc/Synthetic_Designs/complex_2_noc_1D_chain/complex_2_noc_1D_chain.blif  --device EP4SE20 --route_chan_width 300 \
                  --noc on --noc_flows_file $VTR_ROOT/vtr_flow/benchmarks/noc/Synthetic_Designs/complex_2_noc_1D_chain/complex_2_noc_1D_chain.flows \
		  --noc_routing_algorithm xy_routing

	Running multiple instances:
		- The script ($VTR_ROOT/vtr_flow/scripts/noc/noc_benchmark_test.py) runs multiple runs of VPR on a given benchmark and outputs averaged metric
	          information over all runs
		- For example, the command to run 5 iterations of VPR (using 1 thread) on the complex_2_noc_1D_chain benchmark using the above script is as follows:
		
		  <path_to_python_interpreter> $VTR_ROOT/vtr_flow/scripts/noc/noc_benchmark_test.py \
		  -tests_folder_location $VTR_ROOT/vtr_flow/benchmarks/noc/Synthetic_Designs/complex_2_noc_1D_chain/complex_2_noc_1D_chain.blif \
		  -arch_file $VTR_ROOT/vtr_flow/arch/noc/mesh_noc_topology/stratixiv_arch.timing_with_a_embedded_10X10_mesh_noc_topology.xml \
		  -vpr_executable $VTR_ROOT/build/vpr/vpr --device EP4SE20 -flow_file $VTR_ROOT/vtr_flow/benchmarks/noc/Synthetic_Designs/complex_2_noc_1D_chain/complex_2_noc_1D_chain.flows \
		  -noc_routing_algorithm xy_routing -noc_swap_percentage 40 -number_of_seeds 5 -number_of_threads 1

	        - The above command will generate an output file in the run directory that contains all the place and route metrics. This is a txt file with a name which matches the
		  the flows file provided. So for the command shown above the outout file is 'complex_2_noc_1D_chain.txt'

	Expected run time:
		- These benchmarks are quite small so the maximum expected run time for a single run is ~30 minutes
		- To speed up the run time with multiple VPR runs the thread count can be increased from 1. Set thread count equal to number seeds for fastest run time.
		  