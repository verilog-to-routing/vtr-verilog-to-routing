# VTR Codebase Layout

## Top-Level Components

| Directory | Purpose |
|-----------|---------|
| `vpr/` | Core FPGA CAD tool: pack, place, route, timing |
| `parmys/` | Yosys-based synthesis frontend (replaces Odin) |
| `odin_ii/` | Legacy synthesis frontend (disabled by default) |
| `abc/` | Logic optimization (external, do not modify directly) |
| `ace2/` | Activity estimation for power analysis |
| `libs/` | Shared libraries used by VPR and other tools |
| `vtr_flow/` | Flow scripts, architectures, benchmarks, regression tasks |
| `utils/` | Standalone utilities (FASM, VQM, route diagnostics) |
| `doc/` | RST documentation (served at docs.verilogtorouting.org) |

## VPR Internal Structure (`vpr/src/`)

| Subdirectory | Responsibility |
|--------------|---------------|
| `base/` | Core data structures: atom netlist, clustered netlist, device grid. **Start here when exploring data structures** — `vpr_context.h` defines all major contexts and `globals.h` exposes the global `g_vpr_ctx` accessor. |
| `pack/` | Technology mapping and clustering (LUT/FF packing into CLBs). Entry point: `pack.h` / `try_pack()` |
| `place/` | Simulated annealing placement. Entry point: `place.h` / `try_place()`. Subdirectories: `delay_model/` (placement delay models), `move_generators/` (SA move types), `timing/` (placement timing criticalities) |
| `route/` | Maze routing and connection router. Entry point: `route.h` / `route()`. Subdirectories: `router_lookahead/` (lookahead cost computation), `rr_graph_generation/` (building the routing resource graph) |
| `timing/` | Static timing analysis integration (uses Tatum library in `libs/EXTERNAL/libtatum`) |
| `analysis/` | Post-route analysis and reporting |
| `draw/` | OpenGL/EZGL graphical visualization |
| `noc/` | Network-on-Chip placement support |
| `analytical_place/` | Analytical placement (AP) engine. Entry point: `analytical_placement_flow.h` / `run_analytical_placement_flow()` |
| `power/` | Power estimation |
| `util/` | Shared VPR utilities (`vpr_utils.h`) used across multiple stages |
| `server/` | VPR server mode for external tool integration |

## Shared Libraries (`libs/`)

| Library | Purpose |
|---------|---------|
| `libarchfpga` | FPGA architecture XML parser and data structures (`arch_types.h`) |
| `libvtrutil` | Common utilities: logging, data structures (`vtr_vector`, `vtr_ndmatrix`, etc.) |
| `librrgraph` | Routing resource graph (RRG) data structures and serialization |
| `libpugiutil` | XML parsing helpers wrapping pugixml |
| `libvtrcapnproto` | Cap'n Proto binary serialization for RR graphs and router lookahead |
| `liblog` | Logging infrastructure |
| `librtlnumber` | RTL number representation |

## External Subtrees (`libs/EXTERNAL/`)

`libs/EXTERNAL/` contains code pulled in from upstream repositories via `git subtree`: `libargparse`, `libblifparse`, `libsdcparse`, and `libtatum`. Do not modify files under `libs/EXTERNAL/` directly — changes must be made in the upstream repository and then synced into VTR using `dev/external_subtrees.py`. The `abc/` directory at the repo root is similarly external and should not be modified directly.

## VTR Flow (`vtr_flow/`)

| Subdirectory | Content |
|---|---|
| `scripts/` | Python flow orchestration (`run_vtr_flow.py`, `run_vtr_task.py`, `parse_vtr_task.py`) |
| `tasks/regression_tests/` | Regression task configs and golden results |
| `arch/` | FPGA architecture XML files |
| `benchmarks/` | Benchmark circuits (Verilog, BLIF) including VTR, Koios, Titan |
