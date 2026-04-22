# Hecate 2.5D and 3D FPGA Architecture Family

This directory contains architectures exploring 2.5D and 3D FPGA integration within the Verilog-to-Routing (VTR) flow.

## 1. FPGA Fabric

The base FPGA fabric is designed around a modern, high-performance process node:
* **Process Node:** Based on a 7nm fabric defined in [1], [2].
* **Logic & Memory:** Features Agilex-like Complex Logic Blocks (CLBs), Digital Signal Processing (DSP) blocks, and Block RAM (BRAM).
* **Intra-die Routing:** Utilizes a Stratix-IV-like intra-die routing architecture.

## 2. 2.5D Architectures

The 2.5D architectures explore various interposer configurations:
* **Bonding Technology:** 10µm microbump (µbump) and 5µm hybrid-bonding pitch.
* **Inter-die Routing:** Routing-based inter-die routing architecture, similar to Xilinx 7-Series.
* **Architecture Variations:** The architectures differ in:
  * **Interposer Segment Length:** Higher length increases connectivity, while lower length reduces delay.
  * **Interposer Wire Driver:** Variations in fan-in and fan-out.
  * **Bump Pitch:** Lower pitch results in more interposer wires per CLB.
* **Architecture Naming Scheme:**
  - hecate_25d_L[WIRE_LENGTH]_int_[BUMP_PITCH]um_bump_fanin_[FANIN].xml
  - [WIRE_LENGTH]: length of interposer segments in tiles
  - [BUMP_PITCH]: pitch of microbumps in um
  - [FANIN]: fan-in of interposer wire drivers
* **Delay Models:** Delay values have been extracted using SPICE simulations (ASAP-7 7nm node for the active die and 45nm for the interposer based on the ITRS 2005 interconnect report).

## 3. 3D Architectures

<!-- TODO: Add details about 3D Architectures -->

---

## References
* **[1]** A. Arora et al., "Koios 2.0: Open-Source Deep Learning Benchmarks for FPGA Architecture and CAD Research," TCAD 2023
* **[2]** A. Boutros, F. Mahmoudi, A. Mohaghegh, S. More and V. Betz, "Into the Third Dimension: Architecture Exploration Tools for 3D Reconfigurable Acceleration Devices," FPT 2023