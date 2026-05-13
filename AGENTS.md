# VTR Agent Guide

This file provides guidance to AI coding agents working in this repository.

## Quick Reference

Detailed guides are in `doc/agents/`:

- **Build details**: See `doc/agents/build.md`
- **Codebase layout**: See `doc/agents/codebase.md`
- **Coding conventions**: See `doc/agents/coding.md`
- **Testing guide**: See `doc/agents/testing.md`

## Project Overview

VTR (Verilog-to-Routing) is an open-source FPGA CAD framework. It takes Verilog as input and produces place-and-route results for FPGA architectures. The full flow runs:

1. **PARMYS** (Yosys-based) — Elaboration, synthesis, partial mapping (maintained here; Yosys itself is external)
2. **ABC** — Logic optimization and technology mapping (external, do not modify)
3. **VPR** — Packing, placement, routing, and timing analysis (**primary development target**)

A defining feature: VTR targets many different FPGA architectures, with the target architecture passed in as an XML file at runtime. CAD algorithms must therefore avoid hardcoding architecture-specific assumptions — the architecture is parsed at startup into data structures defined in `libarchfpga`.

See `doc/agents/codebase.md` for the full directory layout and VPR internal structure.

## Building

Always run `make` from the root directory — never invoke `cmake` directly. Build artifacts go in `build/`.

```shell
make          # full build
make -j8 vpr  # specific target with parallel jobs
```

See `doc/agents/build.md` for debug builds, CMake options, and build speed tips.

## Python Environment

Many scripts in `vtr_flow/scripts/` require the Python virtual environment. Before running any flow scripts or regression tests, activate it with:

```shell
source .venv/bin/activate
```

The virtual environment is set up by the user once (`make env` + `pip install -r requirements.txt`). Do not run those setup commands — assume the environment already exists.

## Running Tests

```shell
# Fast smoke test (~1 min)
./run_reg_test.py vtr_reg_basic

# Full functionality check (~20 min)
./run_reg_test.py vtr_reg_strong -j4

# Run both
./run_reg_test.py vtr_reg_basic vtr_reg_strong -j4

# All unit tests
make && make test
```

New features **must** include a regression test in `vtr_reg_strong`. See `doc/agents/testing.md` for unit test details and how to add or update regression tests.

## AI Usage Policy

VTR requires that humans author all commits and pull requests. Do not create commits or open pull requests autonomously. When helping a developer, produce code and explanations for them to review, and let them decide what to commit and how to phrase the commit message. Do not add `Co-Authored-By: Claude` or similar lines to commit messages.

## Quality of Results (QoR) Policy

Any change to algorithms must be evaluated for QoR impact using the VTR benchmarks (`vtr_reg_qor_chain` in nightly). Key metrics: `num_post_packed_blocks`, `device_grid_tiles`, `min_chan_width`, `crit_path_routed_wirelength`, `critical_path_delay`, and runtime/memory. Compare using GEOMEAN across all benchmarks. A broken build or regression test failure must be fixed at top priority.
