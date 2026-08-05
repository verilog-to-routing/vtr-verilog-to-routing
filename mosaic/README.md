# MOSAIC
![Mosaic](mosaic.png)

Mosaic is the third synthesis flow for VTR, it is currently under developlment. The name Mosaic refers to the use of templates for techmapping.



## 1. Building Mosaic
You need a built VTR tree first so `build/bin/yosys-config` exists. The mosaic plugin only needs Yosys headers and does not link any VTR library. It compiles wildebeest-originated sources under `mosaic/wildebeest/` together with mosaic-only sources under `mosaic/src/`.

```shell
make -j$(nproc)
bash mosaic/build_mosaic.sh
```

The script builds the plugin against the VTR Yosys and installs it as `wildebeest.so` under `build/share/yosys/plugins/`. Yosys loads it with `plugin -i wildebeest`.



## 2. Running Mosaic
### 2a. Running directly
```shell
./vtr_flow/scripts/run_vtr_flow.py <circuit.v> <arch.xml> -start mosaic
```

Optional flags:

- `-mosaic_script <path>` chooses a custom Yosys template. The default is `vtr_flow/misc/mosaic/template/synthesis.tcl`.
- `-top_module <name>` sets the top. Leave it empty for Yosys `-auto-top`.
- `-include <file>` adds extra Verilog, which Koios-style hardblock includes need.

The stage writes `<circuit>.mosaic.blif`, logs to `mosaic.out`, prunes unused blackbox model declarations, then runs `fix_blif_for_vpr.py` for ram addr pads, hierarchical net dots, and latch-q uniquify. After that the flow continues through ABC and VPR like the other synthesis frontends.

Examples:

```shell
./vtr_flow/scripts/run_vtr_flow.py vtr_flow/benchmarks/verilog/diffeq1.v vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml -start mosaic
./vtr_flow/scripts/run_vtr_flow.py vtr_flow/benchmarks/verilog/koios/lenet.v vtr_flow/arch/COFFE_22nm/k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml -start mosaic -include vtr_flow/benchmarks/verilog/koios/hard_block_include.v
```

Titan arches use the same entry point. Policy dirs under `vtr_flow/misc/mosaic/stratixiv_arch.timing/` and `stratix10_arch.timing/` soft-map memories and stub exotic hardblocks. Smoke with VTR Verilog, not Titan BLIF:

```shell
./vtr_flow/scripts/run_vtr_flow.py vtr_flow/benchmarks/verilog/diffeq1.v vtr_flow/arch/titan/stratixiv_arch.timing.xml -start mosaic
```

### 2b. Running with the batch command
`mosaic/scripts/run_vtr_batch.py` wraps many `run_vtr_flow.py` calls. Each run is pinned to one core. `--jobs N` means N of those single-core runs in parallel. Results land under `mosaic/scripts/compare_output_<arch_stem>/` with run dirs, logs, status files, and a timestamped csv.

```shell
python3 mosaic/scripts/run_vtr_batch.py --arch vtr_flow/arch/COFFE_22nm/k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml --benchmark-dir vtr_flow/benchmarks/verilog/koios --designs eltwise_layer conv_layer gemm_layer lenet --include hard_block_include.v --jobs 4 --watch
```

`--watch` spawns `watch_compare.py` in the same terminal. You can also run the watcher alone against an output dir:

```shell
python3 mosaic/scripts/watch_compare.py --dir mosaic/scripts/compare_output_<arch_stem>
```

Useful flags include `--flows mosaic` to skip vanilla VTR, and `--no-rerun` to skip jobs that already have a success marker. Omit `--designs` to take every `*.v` in the benchmark dir except `*_include.v`.



## 3. Technical details
### 3a. Layout
- `mosaic/wildebeest/` holds wildebeest-originated sources, mainly `max_level` in `clk_domains.cc`, including the `-vtr_arch` patch.
- `mosaic/src/` holds mosaic-only sources. Arch scanning lives in `vtr_arch_info.*` and `vtr_arch_clocks.*`. The Yosys pass is `vtr_arch_rules.cc`. Rule generators live under `arch_rule_gen/` with the public API in `arch_rule_gen.h`.
- `mosaic/build_mosaic.sh` configures, builds, and installs the plugin.
- `vtr_flow/misc/mosaic/template/` is the shared synthesis support tree (`synthesis.tcl`, `profiles.tcl`, `fix_blif_for_vpr.py`, `rules/`, `abc/`, `lut_models/`).
- `vtr_flow/misc/mosaic/<arch_xml_stem>/` is optional per-arch policy. It needs an `arch_config.tcl`. An optional `rules/` dir overlays selected `.tmpl` files onto the shared template set.
- `vtr_flow/scripts/python_libs/vtr/mosaic/` is the VTR flow stage that copies the template, fills tokens, and runs Yosys.
- `mosaic/scripts/` has maintainer tools such as `dump_arch_info.py`, `test_tpl_overlay.py`, `run_vtr_batch.py`, and `watch_compare.py`.

### 3b. Arch policy and hardblocks
Per-arch knobs live in `arch_config.tcl`. Geometry facts such as dsp widths and ram abits come from `arch_facts.tcl`, which `vtr_arch_rules` generates from the arch xml. Do not put those widths in `arch_config.tcl`.

A normal classic run expects `single_port_ram` and `dual_port_ram`, or aliases of those roles. `multiply` and `adder` are optional. Carry-chain `add_sub_map` is emitted only when the adder has `cin`, `cout`, and `sumout`. Otherwise `$add` and `$sub` stay soft.

If the arch uses different model names, set aliases in `arch_config.tcl`:

```tcl
set aliasMultiply my_dsp_mult
set aliasAdder my_carry
set aliasSinglePortRam my_spram
set aliasDualPortRam my_dpram
```

`template/profiles.tcl` defines named packs. `vtr_classic` is the default. `passthrough_exotics` forces `stubAllHardblocks` so rtl-instantiated exotic cells keep through identity maps.

Exotics are every hardblock that is not classic multiply, adder, or the classic rams after aliases. Mosaic never silently maps `$mul` or `$add` onto them. You choose one targeting mode in `arch_config.tcl`:

- Identity passthrough with `set stubAllHardblocks 1` or `set primitiveProfile passthrough_exotics`. Rtl must instantiate the cell.
- Per-model template with `set exoticTemplatePairs {{model path/to.tmpl}}`.
- Role inference with `set exoticRoles {{model integer_mul}}` when ports match a stock role under `template/rules/roles/`.

There is a longer exotic hardblocks guide in `docs-env/docs/doc-mosaic-exotic-hardblocks.md`. Small fixtures live under `mosaic/tests/fixtures/` with matching policy dirs under `vtr_flow/misc/mosaic/`.

Soft and hard knobs that show up often:

- `minHardMulWidth` keeps `$mul` soft when both operand widths are at or below the threshold. `0` disables the limit.
- `minHardMemAbits` drops shallower bram modes from libmap so those memories soft-map.
- `softOnlyMemory` soft-maps memories when classic sp or dp modes are absent. Titan policy uses this.
- `hardAdderThreshold` and `dspMinWidth` control adder hardness and mul2dsp chunking.
- When `lutCost` and `cmpLutWidth` stay at defaults, synthesis can derive them from scanned `lutK` and `lutK1`. Empty abc scripts auto-select shared delay scripts only for fracturable K6-like arches.

Shared abc scripts live under `template/abc/`. Rebuild them with `template/abc/build_delay_scr.py` when the upstream delay script changes.

### 3c. How a synthesis run is wired
The Python stage resolves `vtr_flow/misc/mosaic/<arch_xml_stem>/` when that directory has `arch_config.tcl`. Otherwise the run is facts-only from the arch xml. It copies `synthesis.tcl` and replaces tokens before Yosys sees the script. The important tokens are `XXX` for circuit Verilog, `TTT` for the top module, `ZZZ` for the output blif, `VVV` for the arch xml, `TDIR` for the template dir, `ARCH_SUPPORT_DIR` for the policy dir, and `YYY` for the optional `max_level` `-vtr_arch` flag.

`synthesis.tcl` sources policy, runs `vtr_arch_rules` with `-tpldir` pointed at `template/rules`, and passes `-overlay-tpldir` when the arch support dir has a `rules/` folder. Only listed overlay files replace shared templates. The pass emits maps and stubs into the run dir, then the rest of the script techmaps and writes the blif.



## 4. Verilator check
`mosaic/verilator_check/` checks functional equivalence between the original rtl and the post-synthesis and post-abc blifs from a harness run directory.

```shell
python3 mosaic/verilator_check/run_random_check.py --run-dir <harness_run_dir> --vectors 200000 --seed 1
```

The checker converts both blifs back to Verilog with Yosys, builds a three-DUT testbench with the rtl, post-synth design, and post-abc design, and drives the same random vectors into all three. Hardblock simulation models live in `verilator_check/models/sim_hardblocks.v`.

Optional flags:

- `--check-mem-init` fails if rtl memory init cannot survive hard ram blackboxes.
- `--directed-ram` adds same-addr read/write and dual-port write/write cases when ports match.
- `--ram-zero-init` forces sim rams to zero. The default leaves memory uninitialized so dropped init can surface.



## 5. Regression testing
The `vtr_reg_basic_mosaic` suite runs basic timing circuits on `k6_frac_N10_frac_chain_mem32K_40nm.xml` with `-start mosaic`:

```shell
./run_reg_test.py vtr_reg_basic_mosaic -j4
```

There is also a Koios smoke task under `vtr_flow/tasks/regression_tests/vtr_reg_basic_mosaic/koios`:

```shell
./vtr_flow/scripts/run_vtr_task.py regression_tests/vtr_reg_basic_mosaic/koios
```

To compare Parmys and Mosaic hardblock BLIF model names on one circuit:

```shell
python3 mosaic/scripts/compare_frontend_blif_models.py --run --circuit vtr_flow/benchmarks/verilog/diffeq1.v --arch vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml
```

The `RegressionWithMosaic` job in `.github/workflows/test.yml` builds mosaic on top of the release build artifact and runs the basic suite.

Optional maintainer checks:

```shell
python3 mosaic/scripts/dump_arch_info.py --update
python3 mosaic/scripts/test_tpl_overlay.py
```

`dump_arch_info.py` refreshes goldens under `mosaic/tests/golden/`. `test_tpl_overlay.py` needs a built mosaic plugin and checks `-overlay-tpldir` merge.