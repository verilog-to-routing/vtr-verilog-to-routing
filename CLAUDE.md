# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

VTR (Verilog-to-Routing) is an open-source FPGA CAD framework. It takes Verilog as input and produces place-and-route results for FPGA architectures. The full flow runs:

1. **PARMYS** (Yosys-based) — Elaboration, synthesis, partial mapping
2. **ABC** — Logic optimization and technology mapping
3. **VPR** — Packing, placement, routing, and timing analysis

## Building

```shell
# Standard build (uses build/ directory, never in-source)
make

# Debug build
make BUILD_TYPE=debug

# Build with specific CMake options
make CMAKE_PARAMS="-DVTR_ENABLE_SANITIZE=true"

# Build only specific target
make vpr
```

Build artifacts go in `build/`. The Makefile is a thin wrapper around CMake; the actual build dir is `build/`. Use `-j N` to compile in parallel (e.g. `make -j8 vpr`).

Key CMake options (pass via `CMAKE_PARAMS`):
- `VTR_ASSERT_LEVEL` — 0–4, controls assertion density (default 2)
- `VTR_ENABLE_SANITIZE` — address/leak/UB sanitizers
- `VTR_ENABLE_PROFILING` — gprof profiling
- `VPR_USE_EZGL` — graphics library (auto/on/off)
- `WITH_PARMYS` — build Yosys frontend (default ON)
- `WITH_ODIN` — build legacy Odin frontend (default OFF)
- `VTR_IPO_BUILD` — interprocedural optimization (default auto/on); set to `off` during development to reduce build times and improve debuggability

## Code Formatting

```shell
# Format C/C++ (requires clang-format)
make format

# Format Python (requires black)
make format-py

# Python lint check
./dev/pylint_check.py                    # Check non-grandfathered files
./dev/pylint_check.py path/to/file.py   # Check specific file
```

C/C++ formatting applies to: `vpr/`, `libs/libarchfpga`, `libs/libvtrutil`, `libs/libpugiutil`, `libs/liblog`, `libs/librtlnumber`, `odin_ii/`.

## Python Environment

Many scripts in `vtr_flow/scripts/` require the Python virtual environment. Before running any flow scripts or regression tests, activate it with:

```shell
source .venv/bin/activate
```

The virtual environment is set up by the user once (`make env` + `pip install -r requirements.txt`). Do not run those setup commands — assume the environment already exists.

## Running Tests

### Regression Tests

```shell
# Fast smoke test (~1 min)
./run_reg_test.py vtr_reg_basic

# Full functionality check (~20 min, use -j4 to parallelize)
./run_reg_test.py vtr_reg_strong -j4

# Run both
./run_reg_test.py vtr_reg_basic vtr_reg_strong -j4

```

### Unit Tests

```shell
# Build and run all unit tests
make && make test

# Run a specific unit test binary directly
cd build/vpr && ./test_vpr --list-tests
cd build/vpr && ./test_vpr connection_router binary_heap

# Other unit test binaries:
# build/libs/libarchfpga/test_archfpga
# build/libs/libvtrutil/test_vtrutil
# build/utils/fasm/test_fasm
```

Unit tests use Catch2. Source lives in:
- `vpr/test/` → `build/vpr/test_vpr`
- `libs/libarchfpga/test/` → `build/libs/libarchfpga/test_archfpga`
- `libs/libvtrutil/test/` → `build/libs/libvtrutil/test_vtrutil`
- `utils/fasm/test/` → `build/utils/fasm/test_fasm`

## Running the VTR Flow

```shell
# Run a regression task
cd vtr_flow/tasks
../scripts/run_vtr_task.py regression_tests/vtr_reg_strong/strong_timing

# Parse QoR results
../scripts/python_libs/vtr/parse_vtr_task.py regression_tests/vtr_reg_strong/strong_timing

# Compare QoR across two runs
./vtr_flow/scripts/qor_compare.py parse_results_baseline.txt parse_results_new.txt -o comparison.xlsx
```

### Adding or Updating Regression Tests

New features **must** include a regression test in `vtr_reg_strong` (small benchmarks, < 1 min runtime):

```shell
cd vtr_flow/tasks/regression_tests/vtr_reg_strong
mkdir -p strong_mytest/config
cp strong_timing/config/config.txt strong_mytest/config/.
# Edit the config, then from vtr_flow/tasks/:
../scripts/run_vtr_task.py regression_tests/vtr_reg_strong/strong_mytest
../scripts/python_libs/vtr/parse_vtr_task.py regression_tests/vtr_reg_strong/strong_mytest -create_golden
../scripts/python_libs/vtr/parse_vtr_task.py regression_tests/vtr_reg_strong/strong_mytest -check_golden
# Add path to vtr_flow/tasks/regression_tests/vtr_reg_strong/task_list.txt
```

To update a golden result after an intentional QoR change, re-run `-create_golden` followed by `-check_golden`.

## Coding Conventions

### Naming

| Element | Convention | Example |
|---|---|---|
| Classes | `PascalCase` | `DecompNetlistRouter` |
| Functions and variables | `snake_case` | `estimate_wirelength()`, `net_count` |
| Private members and methods | trailing `_` | `count_`, `update_state_()` |
| Enum classes | `e_` prefix | `e_heap_type` |
| Data structs (trivial) | `t_` prefix | `t_dijkstra_data` |
| Source/header filenames | `snake_case` | `router_lookahead_map.cpp` |

### Headers

Always put `#pragma once` as the very first line of every header file, before any comments or code.

### Comments and Documentation

VTR requires Doxygen comments on all non-trivial functions, classes, and structs — this overrides any general default to avoid comments.

- `/** */` — Doxygen only (API docs for classes, structs, functions, file headers)
- `//` — all inline implementation comments; always include a space after `//`
- Never use `/* */` block comments inside function bodies

All non-trivial functions must have a Doxygen `@brief` comment in the header (or at the static declaration in `.cpp`). Document every member variable with `///<`.

### Assertions

Prefer `VTR_ASSERT` and `VTR_ASSERT_SAFE` over bare `assert()`:
- `VTR_ASSERT(cond)` — inexpensive checks, always on including release builds
- `VTR_ASSERT_SAFE(cond)` — expensive checks, disabled in release builds

Each has a `_MSG` variant that takes a message string — use it when the condition alone is not self-explanatory:
- `VTR_ASSERT_MSG(cond, "message")`
- `VTR_ASSERT_SAFE_MSG(cond, "message")`

### Logging

Use VTR logging macros instead of `printf`, `std::cerr`, or `exit()`:
- `VTR_LOG(...)` — informational
- `VTR_LOG_WARN(...)` — warnings
- `VTR_LOG_ERROR(...)` — errors

### Use of `auto`

Use `auto` only when the type is long or complex (iterators, lambdas, long template return types). Write out short, clear types explicitly (`int`, `std::string`, `RRNodeId`, etc.).

## Architecture

### Top-Level Components

| Directory | Purpose |
|-----------|---------|
| `vpr/` | Core FPGA CAD tool: pack, place, route, timing |
| `parmys/` | Yosys-based synthesis frontend (replaces Odin) |
| `odin_ii/` | Legacy synthesis frontend (disabled by default) |
| `abc/` | Logic optimization (submodule) |
| `ace2/` | Activity estimation for power analysis |
| `libs/` | Shared libraries used by VPR and other tools |
| `vtr_flow/` | Flow scripts, architectures, benchmarks, regression tasks |
| `utils/` | Standalone utilities (FASM, VQM, route diagnostics) |
| `doc/` | RST documentation (served at docs.verilogtorouting.org) |

### VPR Internal Structure (`vpr/src/`)

| Subdirectory | Responsibility |
|--------------|---------------|
| `base/` | Core data structures: atom netlist, clustered netlist, device grid. **Start here when exploring data structures** — `vpr_context.h` defines all major contexts and `globals.h` exposes the global `g_vpr_ctx` accessor. |
| `pack/` | Technology mapping and clustering (LUT/FF packing into CLBs) |
| `place/` | Simulated annealing placement. Subdirectories: `delay_model/` (placement delay models), `move_generators/` (SA move types), `timing/` (placement timing criticalities) |
| `route/` | Maze routing and connection router. Subdirectories: `router_lookahead/` (lookahead cost computation), `rr_graph_generation/` (building the routing resource graph) |
| `timing/` | Static timing analysis integration (uses Tatum library in `libs/EXTERNAL/libtatum`) |
| `analysis/` | Post-route analysis and reporting |
| `draw/` | OpenGL/EZGL graphical visualization |
| `noc/` | Network-on-Chip placement support |
| `analytical_place/` | Analytical placement (AP) engine |
| `power/` | Power estimation |
| `util/` | Shared VPR utilities (`vpr_utils.h`) used across multiple stages |
| `server/` | VPR server mode for external tool integration |

### External Subtrees (`libs/EXTERNAL/`)

`libs/EXTERNAL/` contains code pulled in from upstream repositories via `git subtree`: `libargparse`, `libblifparse`, `libsdcparse`, and `libtatum`. Do not modify files under `libs/EXTERNAL/` directly — changes must be made in the upstream repository and then synced into VTR using `dev/external_subtrees.py`. The `abc/` directory at the repo root is similarly external and should not be modified directly.

### Shared Libraries (`libs/`)

| Library | Purpose |
|---------|---------|
| `libarchfpga` | FPGA architecture XML parser and data structures (`arch_types.h`) |
| `libvtrutil` | Common utilities: logging, data structures (`vtr_vector`, `vtr_ndmatrix`, etc.) |
| `librrgraph` | Routing resource graph (RRG) data structures and serialization |
| `libpugiutil` | XML parsing helpers wrapping pugixml |
| `libvtrcapnproto` | Cap'n Proto binary serialization for RR graphs and router lookahead |
| `liblog` | Logging infrastructure |
| `librtlnumber` | RTL number representation |

### VTR Flow (`vtr_flow/`)

| Subdirectory | Content |
|---|---|
| `scripts/` | Python flow orchestration (`run_vtr_flow.py`, `run_vtr_task.py`, `parse_vtr_task.py`) |
| `tasks/regression_tests/` | Regression task configs and golden results |
| `arch/` | FPGA architecture XML files |
| `benchmarks/` | Benchmark circuits (Verilog, BLIF) including VTR, Koios, Titan |

## AI Usage Policy

VTR requires that humans author all commits and pull requests. Do not create commits or open pull requests autonomously. When helping a developer, produce code and explanations for them to review, and let them decide what to commit and how to phrase the commit message. Do not add `Co-Authored-By: Claude` or similar lines to commit messages.

## Quality of Results (QoR) Policy

Any change to algorithms must be evaluated for QoR impact using the VTR benchmarks (`vtr_reg_qor_chain` in nightly). Key metrics: `num_post_packed_blocks`, `device_grid_tiles`, `min_chan_width`, `crit_path_routed_wirelength`, `critical_path_delay`, and runtime/memory. Compare using GEOMEAN across all benchmarks. A broken build or regression test failure must be fixed at top priority.
