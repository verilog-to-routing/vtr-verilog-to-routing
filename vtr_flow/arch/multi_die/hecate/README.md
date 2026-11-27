# Hecate 2.5D and 3D FPGA Architecture Family

This directory contains architectures exploring 2.5D and 3D FPGA integration within the Verilog-to-Routing (VTR) flow.
Some architectures are already in the folders, but due to the large number of total architectures we auto-generate the
architectures using the base architectures and parameters inside a csv file.

## How to generate the Hecate architectures

### 2.5D interposer architectures

Run the following command to generate the 2.5D Hecate architectures:

```bash
cd interposer
python generate_hecate_interposer_archs.py
```

### 3D switch-block architectures

Run the following command to generate the 3D switch-block architecture variants:

```bash
cd 3d
python generate_hecate_3d_sb_archs.py
```

### Interposer Connectivity Data and Generation Script

`interposer/generate_hecate_interposer_archs.py` generates both fixed-length and variable-length 2.5D interposer variants across three die configurations from the base templates:
* `interposer/hecate_25d_L18_int_10um_bump_unidir_fanin_32.xml` (2 dies, 1 cut -> `hecate_25D/`, `hecate_varlen_25D/`)
* `interposer/hecate_25d_L18_int_10um_bump_unidir_1x4_fanin_32.xml` (1x4 dies, 3 cuts -> `hecate_25D_1x4/`, `hecate_varlen_25D_1x4/`)
* `interposer/hecate_25d_L18_int_10um_bump_unidir_2x2_fanin_32.xml` (2x2 dies, 2 cuts -> `hecate_25D_2x2/`, `hecate_varlen_25D_2x2/`)

* **Fixed-length variants** sweep interposer wire lengths (`L2` through `L34`, step 2), bump pitches (`10um` -> `num=3`, `5um` -> `num=12`), directionalities (`unidir`, `bidir`), and gather/scatter fan-in/fan-out sizes (`4`, `8`, `16`, `32`, `64`).
* **Variable-length variants** read per-`Max_L*` delay tables from `interposer/int_varlen_connectivity.json`, combining multiple centered wire lengths (`L2` through `Max_L*`) across each cut with per-length drivers, segments, and scatter-gather links.

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
  - `hecate_25d_L[WIRE_LENGTH]_int_[BUMP_PITCH]um_bump_[unidir|bidir][_1x4|_2x2]_fanin_[FANIN].xml`
  - `[WIRE_LENGTH]`: length of interposer segments in tiles
  - `[BUMP_PITCH]`: pitch of microbumps in um
  - `[unidir|bidir]`: directionality of interposer links
  - `[_1x4|_2x2]`: optional die configuration tag (omitted for 2-die)
  - `[FANIN]`: fan-in of interposer wire drivers
* **Delay Models:** Delay values have been extracted using SPICE simulations (ASAP-7 7nm node for the active die and 45nm for the interposer based on the ITRS 2005 interconnect report).

## 3. 3D Architectures

The 3D architectures use the same FPGA fabric described in Section 1 with switch-block inter-layer connectivity.
Variants explore different pitches and fanin/fanouts:

* **Inter-die Connectivity:** 3D switch-blocks.
* **Architecture Variations:** The architectures differ in:
  * **Pitch:** Lower pitch allows more inter-layer connections per block.
  * **Directionality:** Bidirectional/unidirectional inter-layer links.
  * **Fan-in/Fan-out:** The number of wires driving/being driven by each inter-layer connection.
* **Architecture Naming Scheme:**
  - `hecate_3d_sb_[variant_id]_fanin_[FANIN]_fanout_[FANOUT]_[ESD]_7nm.xml`


### 3D CSV and Generation Script

The 3D switch-block architecture variants are generated from `3d/hecate_3d_sb_connectivity.csv` and the base architecture
`3d/hecate_3d_sb_10um_bidir_fanin_16_fanout_16_with_esd_7nm.xml`. This file can also be used directly as the default Hecate 3D
architecture without running the generator. Each CSV row describes one variant, and each variant is generated once with
`with_esd` and once with `without_esd` inter-die switches:

* `variant_id`: output architecture identifier.
* `pitch_um`: bump pitch in µm (5, 10, or 25).
* `connectivity`: inter-die link directionality (`bidir` or `unidir`).
* `fanin`: The number of wires that drive each inter-layer connection.
* `fanout`: The number of wires that are driven by each inter-layer connection.
* `clb_num`, `dsp_num`, `mem_num`: inter-layer connection counts per block type and per
  direction (`L_UP` and `L_DOWN`). Each value times two must be an integer.

`3d/generate_hecate_3d_sb_archs.py` reads each CSV row, reloads the template XML, and replaces the
`scatter_gather_list` section with the variant-specific mux names, directionality, and connection
counts. Generated files are written to `3d/hecate_3D/`.

## 4. 2D Architecture

A 2D FPGA fabric (same as 2.5D but without interposer cuts) is available in the 2d folder for reference and comparison
with 2.5D and 3D architectures.

---

## References
* **[1]** A. Arora et al., "Koios 2.0: Open-Source Deep Learning Benchmarks for FPGA Architecture and CAD Research," TCAD 2023
* **[2]** A. Boutros, F. Mahmoudi, A. Mohaghegh, S. More and V. Betz, "Into the Third Dimension: Architecture Exploration Tools for 3D Reconfigurable Acceleration Devices," FPT 2023
