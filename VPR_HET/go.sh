#!/bin/sh
# Example of how to run t-vpack and vpr

# Run t-vpack to pack the 4-input lookup tables in the blif file to a cluster size 10
../T-VPACK_HET/t-vpack.exe iir1.map4.latren.blif iir1.map4.latren.net -inputs_per_cluster 22 -cluster_size 10 -lut_size 4

# Run VPR on packed netlist
vpr iir1.map4.latren.net k4-n10.xml place.out route.out
