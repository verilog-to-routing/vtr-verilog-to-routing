#!/bin/sh
# Example of how to run AAPack and vpr

# Run AAPack then placement and routing to pack a heterogeneous FPGA
#./vpr sample_arch.xml or1200.latren

./vpr ../vtr_flow/arch/timing/k6_N10_memDepth16384_memData64_40nm_timing_molecule.xml ../../vqm_to_blif/REG_TEST/BENCHMARKS/BLIF/Stratix_IV_default/RS_dec_stratixiv.blif
