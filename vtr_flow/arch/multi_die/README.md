# 3D FPGA Architectures

This directory contains architecture files for 3D FPGAs. The architectures are divided into three sub-directories:

1. **koios_3d:**
- Contains architecture files based on the [k6FracN10LB_mem20K_complexDSP_customSB_22nm](../COFFE_22nm/k6FracN10LB_mem20K_complexDSP_customSB_22nm.clustered.xml) architecture, utilized in Koios benchmarks.
- Inside the architecture file, the fabric with multiple sizes based on the sector size is defined.
- Routing resource and switch delays in this architecture are configured for 7 nm technology. The inter-die connection delay is 73 ps.
- The empty blocks in the middle of the fabric at the base die, which have the chess pattern, represent the through-silicon via (TSV) holes used to deliver power and ground to the upper die.
- Detailed information on how these delays are obtained can be found in the paper "Into the Third Dimension: Architecture Exploration Tools for 3D Reconfigurable Acceleration Devices," presented at FPT '23.
- **Architectures:**
   - `3d_full_OPIN_inter_die_k6FracN10LB_mem20k_complexDSP_customSB_7nm.xml`
     - The inter-die connection is only from output pins, with all output pins connected to the channel on the same die and the other.
     - Inside the architecture file, the fabric with multiple sizes based on the sector size is defined.
   - `3d_limited_OPIN_inter_die_k6FracN10LB_mem20k_complexDSP_customSB_7nm.xml`
   - The inter-die connection is only from output pins, with all OPINs of BRAMs and DSPs connected to the channels on the same die and the other die. 
   - Only 60% of the OPINs of CLBs are connected to the channels on the other die and all of them can get connected to the channels on the same die.

2. **stratixiv_3d:**
   - Contains architecture files based on a [StratixIV-like](../titan/stratixiv_arch.timing.xml) architecture.
   - Delays of switches and routing resources are similar to those reported in the capture of the StratixIV architecture.
   - For the inter-die connection, we multiply the L4 wire delay of SIV by the ratio of the inter-die connection delay to the L4 wire delay of the Koios_3d benchmark.
   - **Architectures:**
     - `3d_10x10_noc_base_stratixiv_up.xml`
       - A 10x10 NoC mech is put on the base die.
       - The upper die is SIV-like FPGA fabric.
     - `3d_full_inter_die_stratixiv_arch.timing.xml`
       - The architecture has two dice.
       - Both dice are SIV-like FPGA fabric.
       - All pins can cross die.
       - This is a completely hypothetical architecture, as the area required to place drivers on every channel segment to drive an IPIN on the other die would be too large. For the purpose of this scenario, we assume an inter-die connection delay of 0.
     - `3d_full_OPIN_inter_die_stratixiv_arch.timing.xml`
       - The architecture has two dice.
       - Both dice are SIV-like FPGA fabric.
       - Only OPINs can cross die.

3. **simple_arch:**
   - Simple architectures primarily used for quick testing in the flow.
   - The inter-die delay of architectures in this file is considered to be zero.
   - Contains two dice and both have the simple fabric.
   - All pins can cross die.
