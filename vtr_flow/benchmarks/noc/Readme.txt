###################################################
# NoC Benchmark Design Files
###################################################

This directory contains sample benchmark design files that incorporate
and embedded Network-on-Chip (NoC). 

Each benchmark design consists of a design file (.blif circuit netlist) and a 
noc traffic flows file (.flows file). The names of the two files for
each benchmark design are the same. For example, below is an example of two
files for a sample benchmark design.

test_benchmark_circuit:
	test_benchmark_circuit.blif
	test_benchmark_circuit.flows
	

# Test Benchmark Designs

The benchmark designs found under the "Test_Designs" folder are relatively
simple designs that are mainly used to verify components NoC CAD flow in the VPR software.
Whenever a new or existing feature needs to be tested, the benchmark designs found here
should be used.


# Synthetic Benchmark Designs

The benchmark designs found under the "Synthetic_Designs" folder are small to medium
sized designs that are used verify the correctness of the VPR software. These designs
are manually created to have understandable structures. For example, a mesh with traffic 
flows to nearest neighbours where we know the optimal solution for NoC traffic minimization. 
By running these designs through VPR, we can determine the correctness of the NoC CAD flow
based on the reference solution.

# Large Benchmark Designs

The benchmark designs found under the "Large Designs" folder and medium to large complex
designs to determine the performance of the NoC flow within the VPR software. Changes made
to the NoC CAD flow in VPR can be evaluated by running these benchmark designs and comparing 
the results with previous versions.

# Adding New Benchmark Designs

First determine what type of benchmark this is (one of the three types above). Create a new folder 
for this benchmark within one of the three sections described above. Add the circuit netlist and
noc traffic_flows files (ensure the files are properly named, as shown in the example above). 
