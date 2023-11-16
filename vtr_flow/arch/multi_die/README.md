# 3D FPGA Architectures

This directory contains architecture files for 3D FPGAs. The architectures are divided into three sub-directories:

1. **koios_3d:**
    - Contains architecture files based on the [k6FracN10LB_mem20K_complexDSP_customSB_22nm](../COFFE_22nm/k6FracN10LB_mem20K_complexDSP_customSB_22nm.clustered.xml) architecture, as used in Koios benchmarks.
    - Routing resource and switch delays in this architecture are based on 7 nm technology, with a delay of 73 ps.
    - Details on how these delays are obtained can be found in the paper "Into the Third Dimension: Architecture Exploration Tools for 3D Reconfigurable Acceleration Devices," presented at FPT '23.

2. **stratixiv_3d:**
    - Contains architecture files based on a [StratixIV-like](../titan/stratixiv_arch.timing.xml) architecture.
    - Delays of switches and routing resources are similar to those reported in the capture of the StratixIV architecture.
    - For the inter-die connection, we multiply the L4 wire delay of SIV by the ratio of the inter-die connection delay to the L4 wire delay of the Koios benchmark.

3. **simple_arch:**
    - Simple architectures primarily used for quick testing in the flow.
    - The inter-die delay of architectures in this file is considered to be zero.
