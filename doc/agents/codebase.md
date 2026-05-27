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
| `analytical_place/` | Analytical placement (AP) engine. Entry point: `analytical_placement_flow.h` / `run_analytical_placement_flow()`. Subdirectories: `common/` (APNetlist and PartialPlacement data structures shared across stages), `global_placement/` (Stage 1: analytical solver + iterative partial legalization), `full_legalization/` (Stage 2: flat-to-clustered placement legalization), `detailed_placement/` (Stage 3: post-legalization quality optimization) |
| `power/` | Power estimation |
| `util/` | Shared VPR utilities (`vpr_utils.h`) used across multiple stages |
| `server/` | VPR server mode for external tool integration |

## VPR Data Flow

Understanding how data moves through VPR is essential before touching any stage. The traditional flow is:

```
AtomNetlist → Prepacker → Packer → ClusteredNetlist → Placer → Router
```

- **AtomNetlist** (`base/`) — primitive-level netlist parsed from the input circuit
- **Prepacker** (`pack/`) — groups atoms into molecules based on pack patterns; runs before both packing and analytical placement
- **Packer** (`pack/`) — clusters molecules into logic blocks, producing a **ClusteredNetlist**
- **Placer** (`place/`) — assigns each cluster to a physical tile on the device grid, populating **BlkLocRegistry**
- **Router** (`route/`) — routes all nets through the routing resource graph

The analytical placement flow (`analytical_place/`) is an alternative path that integrates global placement before legalization into clusters, bypassing or wrapping the traditional packer depending on configuration.

All major VPR data structures are held in contexts accessible via `g_vpr_ctx` (declared in `vpr/src/base/globals.h`). `vpr_context.h` in the same directory defines the full set of contexts and is the best starting point for understanding what data exists at each stage.

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
