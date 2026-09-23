# Mosaic Synthesis Configuration
This directory contains the synthesis configuration files used by the Mosaic frontend.



## Layout
- `template/`: shared synthesis driver and rule templates used by every architecture.
  - `rules/`: shared Verilog/template files for techmapping (BRAM, multiply, adder, hardblock stubs).
  - `abc/`: ABC delay scripts used during in-Yosys LUT mapping.
  - `lut_models/`: LUT Verilog models for `max_level`.
  - `synthesis.tcl`: main Yosys script copied into each run directory.
  - `fix_blif_for_vpr.py`: post-synthesis BLIF cleanup (ram addr pads, hierarchical net dots, latch-Q uniquify).
- `<arch_xml_stem>/`: optional per-architecture policy directories. Each contains:
  - `arch_config.tcl`: architecture-specific knobs (hardblock aliases, soft/hard thresholds, costs, ABC script overrides).
  - `rules/` (optional): rule template overlays that replace the corresponding shared template file for that architecture.

Per-architecture directories are matched by the stem of the architecture XML filename passed to `run_vtr_flow.py`. When no matching directory exists, synthesis runs using defaults from `template/synthesis.tcl`.

See `mosaic/README.md` at the repository root for full technical details.
