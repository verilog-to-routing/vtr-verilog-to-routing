# MOSAIC
![Mosaic](mosaic.png)

Mosaic is the third synthesis flow for VTR, it is currently under developlment. The name Mosaic refers to the use of templates for techmapping.

## 1. Building Mosaic
The default VTR build includes mosaic. Sources live in `mosaic/src/`; `mosaic/wildebeest/` is a small object library pulled into that build.

```shell
make -j$(nproc)
```

The plugin is at `build/share/yosys/plugins/mosaic.so`. Yosys loads it with `plugin -i mosaic`.

`WITH_MOSAIC` (default ON) and `WITH_PARMYS` (default ON) select the Yosys plugins. Yosys is built when either is on. At least one must be on.

```shell
make CMAKE_PARAMS="-DWITH_MOSAIC=OFF" -j$(nproc)
```



## 2. Running Mosaic
### 2a. Running directly
```shell
./vtr_flow/scripts/run_vtr_flow.py <circuit.v> <arch.xml> -start mosaic
```

Optional flags:

- `-mosaic_script <path>` chooses a custom Yosys template. The default is `vtr_flow/misc/mosaic/template/synthesis.tcl`.
- `-top_module <name>` sets the top. Leave it empty for Yosys `-auto-top`.
- `-include <file>` adds extra Verilog, which Koios-style hardblock includes need.
- `-verilator_check` runs an rtl vs post-synth (after abc) Verilator random-check after synthesis.

The stage writes `<circuit>.mosaic.blif`, logs to `mosaic.out`, prunes unused blackbox model declarations, then runs `fix_blif_for_vpr.py` for ram addr pads, hierarchical net dots, and latch-q uniquify. LUT mapping already happened inside Yosys (`abc` with `ENABLE_ABC=1`), so the flow skips the external VTR ABC stage and continues into VPR.

Examples:

```shell
./vtr_flow/scripts/run_vtr_flow.py vtr_flow/benchmarks/verilog/diffeq1.v vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml -start mosaic
./vtr_flow/scripts/run_vtr_flow.py vtr_flow/benchmarks/verilog/koios/lenet.v vtr_flow/arch/COFFE_22nm/k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml -start mosaic -include vtr_flow/benchmarks/verilog/koios/hard_block_include.v
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

Useful flags include `--flows mosaic` to skip vanilla VTR, `--verilator-check mosaic` to compare rtl vs post-synth with Verilator on Mosaic only, and `--no-rerun` to skip jobs that already have a success marker. Omit `--designs` to take every `*.v` in the benchmark dir except `*_include.v`. See `mosaic/verilator_check/README.md` for the random-check details.



## 3. Technical details
### 3a. How synthesis runs
`-start mosaic` selects the Mosaic Synthesis flow, the following then happens:
1. `-start mosaic` selects the mosaic stage instead of Parmys or ODIN II.
2. Python looks for `vtr_flow/misc/mosaic/<arch_xml_stem>/arch_config.tcl`.
   - If that file is missing, the run is facts-only:
     1. Geometry still comes from the architecture xml.
     2. Knobs stay at the `synthesis.tcl` defaults (no aliases, no `stubAllHardblocks`, default costs).
     3. Only the shared `template/rules/` templates are used.
     4. A warning is logged so the missing policy dir is visible in the log.
3. Python copies `template/synthesis.tcl` into the run dir, fills path/circuit tokens, and launches Yosys with `mosaic.so`.
   - The same shared script runs for every architecture.
   - Arch-specific behavior enters later through `arch_config.tcl` and `rules/` overlays.
   - Only `-mosaic_script <path>` replaces the whole driver script.
4. `synthesis.tcl` starts running in Yosys.
5. Knobs are layered in order:
   1. `synthesis.tcl` sets shared defaults (`lutCost`, empty aliases, `stubAllHardblocks 0`, etc.).
   2. `arch_config.tcl` is sourced when present. It overlays the defaults (aliases for hardblocks, exotic targeting, thresholds, ABC scripts, `stubAllHardblocks`, etc.).
6. `synthesis.tcl` calls `vtr_arch_rules` on the architecture xml.
7. Rule generators in `mosaic/src/arch_rule_gen/` scan the xml and emit files into the run dir (`mult_map.v`, `tech_bram.v`, stubs, `arch_facts.tcl`, etc.). For each output they:
   1. Resolve which `.tmpl` to use, per file:
      1. base layer: `template/rules/` (`-tpldir`)
      2. overlay layer: `<arch_xml_stem>/rules/<same_filename>` when it exists (`-overlay-tpldir`)
      3. files missing from the arch `rules/` dir fall back to the shared template
      - Example: Titan comes with a specific `rules/vtr_hardblock_lib.v.tmpl`; that file comes from the architecture dir, whereas every other map still comes from `template/rules/`.
   2. Fill the chosen template with scanned arch values.
   3. Write the filled file into the run dir.
8. With the generated files on disk, `synthesis.tcl` starts synthesis on the design:
   1. `read_verilog -lib` loads the generated hardblock stubs (`vtr_hardblock_lib.v`).
   2. `read_verilog -sv` reads the circuit Verilog (plus any `-include` files).
   3. `hierarchy -check` locks the top (`-top` or `-auto-top`) and purges unused lib modules.
   4. The classic bram whitebox is elaborated when memories are not soft-only.
   5. Techmap passes use the generated rule files: `memory_libmap` with `bram_memory_map.txt` / `tech_bram.v`, `mul2dsp_map.v`, `mult_map.v`, `add_sub_map.v`, and any exotic maps.
   6. In-Yosys ABC runs for LUT mapping (`abc -luts, etc.`), using the shared or policy-selected scripts.
   7. Sweep / opt / keep handling cleans up unused hardblocks.
   8. `write_blif` emits `<circuit>.mosaic.blif`.
9. Python prunes unused blackbox model declarations and runs `fix_blif_for_vpr.py` (ram addr pads, hierarchical net dots, latch-q uniquify).
10. External VTR ABC is skipped because LUT mapping already happened inside Yosys. The flow continues into VPR.

### 3b. Options and wiring
CLI flags that reach the mosaic stage:
- `-mosaic_script <path>` replaces the shared Yosys template.
- `-top_module <name>` sets `TTT`. Empty means Yosys `-auto-top`.
- `-include <file>` adds Verilog alongside the circuit (Koios hardblock includes use this).

Tokens filled into the template before Yosys runs:

| Token | Meaning |
|-------|---------|
| `XXX` | circuit Verilog |
| `TTT` | top module |
| `ZZZ` | output BLIF |
| `VVV` | architecture XML |
| `TDIR` | shared template dir |
| `ARCH_SUPPORT_DIR` | per-arch policy dir (empty when absent) |
| `YYY` | optional `max_level` `-vtr_arch` flag |

`vtr_arch_rules` always takes `-tpldir template/rules`. When `ARCH_SUPPORT_DIR/rules/` exists, it also gets `-overlay-tpldir` so only listed files override the shared set.

### 3c. Policy vs facts
There is also a split in how we provide context to Mosaic for Synthesis.
- `arch_config.tcl` holds choices for a specific architecture (i.e. aliases, soft/hard thresholds, costs, exotic targeting, `stubAllHardblocks`). When adding new architectures to VTR it could be useful to sweep the knobs in this file to further tune synthesis to a specific architecture if additional QoR is needed.
- `arch_facts.tcl` holds what `vtr_arch_rules` scanned from the architecture xml (i.e. dsp widths, ram abits, lutK, whether multiply / adder exist). It is generated fresh every run.

### 3d. Classic hardblocks
A normal classic run expects `single_port_ram` and `dual_port_ram`, or aliases of those roles. `multiply` and `adder` are optional. Carry-chain `add_sub_map` is emitted only when the adder has `cin`, `cout`, and `sumout`. Otherwise `$add` and `$sub` stay soft.

If the architecture uses different model names, set aliases in `arch_config.tcl`:

```tcl
set aliasMultiply my_dsp_mult
set aliasAdder my_carry
set aliasSinglePortRam my_spram
set aliasDualPortRam my_dpram
```

### 3e. Exotic hardblocks
Exotics are every hardblock that is not classic multiply, adder, or the classic rams after aliases. Mosaic never silently maps `$mul` or `$add` onto them. Choose one targeting mode in `arch_config.tcl`:
- Identity passthrough: `set stubAllHardblocks 1`. Rtl must instantiate the cell by name.
- Per-model template: `set exoticTemplatePairs {{model path/to.tmpl}}`.
- Role inference: `set exoticRoles {{model integer_mul}}` when ports match a stock role under `template/rules/roles/`.

Longer notes live in `docs-env/docs/doc-mosaic-exotic-hardblocks.md`. Small fixtures are under `mosaic/tests/fixtures/` with matching policy dirs under `vtr_flow/misc/mosaic/`.

### 3f. Soft / hard knobs and ABC scripts
Common knobs in `arch_config.tcl`:
- `minHardMulWidth` keeps `$mul` soft when both operand widths are at or below the threshold. `0` disables the limit.
- `minHardMemAbits` drops shallower bram modes from libmap so those memories soft-map.
- `softOnlyMemory` soft-maps memories when classic sp or dp modes are absent. Titan policy uses this.
- `hardAdderThreshold` and `dspMinWidth` control adder hardness and mul2dsp chunking.
- When `lutCost` and `cmpLutWidth` stay at defaults, synthesis can derive them from scanned `lutK` and `lutK1`. Empty ABC scripts auto-select shared delay scripts only for fracturable K6-like arches.

Shared ABC scripts live under `template/abc/`. Rebuild them with `template/abc/build_delay_scr.py` when the upstream delay script changes. These run inside Yosys during mosaic synthesis, not as the external VTR ABC stage.

### 3g. Layout
- `mosaic/wildebeest/` is a small wildebeest-originated object library (mainly `max_level` in `clk_domains.cc`, including the `-vtr_arch` patch).
- `mosaic/src/` holds mosaic-only sources. Architecture scanning lives in `vtr_arch_info.*` and `vtr_arch_clocks.*`. The Yosys pass is `vtr_arch_rules.cc`. Rule generators live under `arch_rule_gen/` with the public API in `arch_rule_gen.h`.
- `build/share/yosys/plugins/mosaic.so` is the installed plugin (`WITH_MOSAIC`, default ON).
- `vtr_flow/misc/mosaic/template/` is the shared synthesis support tree (`synthesis.tcl`, `fix_blif_for_vpr.py`, `rules/`, `abc/`, `lut_models/`).
- `vtr_flow/misc/mosaic/<arch_xml_stem>/` is optional per-architecture policy (`arch_config.tcl`, optional `rules/` overlay).
- `vtr_flow/scripts/python_libs/vtr/mosaic/` is the VTR flow stage that copies the template, fills tokens, and runs Yosys.
- `mosaic/scripts/` has maintainer tools such as `dump_arch_info.py`, `test_tpl_overlay.py`, `run_vtr_batch.py`, and `watch_compare.py`.



## 4. Verilator check
`mosaic/verilator_check/` checks functional equivalence between the original rtl and the post-synth blif (after synth + abc, or mosaic in-yosys abc) from a harness run directory.

```shell
python3 mosaic/verilator_check/run_random_check.py --run-dir <harness_run_dir> --vectors 200000 --seed 1
```

The checker converts the post-synth blif back to Verilog with Yosys, builds a two-DUT testbench with the rtl and post-synth design, and drives the same random vectors into both. Hardblock simulation models live in `verilator_check/models/sim_hardblocks.v`.

Optional flags:
- `--check-mem-init` fails if rtl memory init cannot survive hard ram blackboxes.
- `--directed-ram` adds same-addr read/write and dual-port write/write cases when ports match.
- `--ram-zero-init` forces sim rams to zero. The default leaves memory uninitialized so dropped init can surface.

See `mosaic/verilator_check/README.md` for information.



## 5. CI Testing
The `vtr_reg_basic_mosaic` suite runs the `k6` task (basic circuits on `k6_frac_N10_frac_chain_mem32K_40nm.xml` with `-start mosaic`) plus the `koios` hardblock passthrough smoke (`reduction_layer.v` + `hard_block_include.v` on the complex-DSP architecture). Those runs also enable `-verilator_check` (20000 vectors). Every mosaic case must pass rtl vs post-synth:

```shell
./run_reg_test.py vtr_reg_basic_mosaic -j4
python3 mosaic/scripts/verify_regression_vcheck.py --suite vtr_reg_basic_mosaic
```

Needs Verilator on `PATH`. The `RegressionWithMosaic` job in `.github/workflows/test.yml` runs this suite (and the vcheck gate) against the release build.

To compare Parmys and Mosaic hardblock BLIF model names on one circuit:

```shell
python3 mosaic/scripts/compare_frontend_blif_models.py --run --circuit vtr_flow/benchmarks/verilog/diffeq1.v --arch vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml
```

Optional maintainer checks:

```shell
python3 mosaic/scripts/dump_arch_info.py --update
python3 mosaic/scripts/test_tpl_overlay.py
```

`dump_arch_info.py` refreshes goldens under `mosaic/tests/golden/`. `test_tpl_overlay.py` needs a built mosaic plugin and checks if `-overlay-tpldir` has been applied.
