# Mosaic Flow

Mosaic is an opt-in synthesis frontend for the vtr flow (use `-start mosaic`; parmys remains the default).

| Design        | Flow         | LUTs (packed) | BRAMs (packed) | DSPs (packed) | Adders (packed) | CLBs | Wire length | CPD ns | Fmax MHz | WNS ns |
| ------------- | ------------ | ------------: | -------------: | ------------: | --------------: | ---: | ----------: | -----: | -------: | -----: |
| LU32PEEng     | mosaic |         44076 |            168 |            64 |           17192 | 5245 |      989996 |   71.1 |    14.06 |   -161 |
| LU32PEEng     | vanilla_vtr  |         69139 |            168 |            32 |           13291 | 7431 |     1213168 |   75.4 |    13.26 |   -195 |
| LU8PEEng      | mosaic |         14339 |             45 |            16 |            5313 | 1619 |      256915 |   76.9 |    13.01 |   -149 |
| LU8PEEng      | vanilla_vtr  |         19611 |             45 |             8 |            4381 | 2133 |      287638 |   73.3 |    13.63 |   -145 |
| arm_core      | mosaic |         11556 |             40 |             0 |             357 | 1122 |      210414 |   19.8 |    50.59 |  -34.6 |
| arm_core      | vanilla_vtr  |          9409 |             24 |             0 |             384 |  862 |      152188 |   23.7 |     42.2 |    -39 |
| bgm           | mosaic |         13559 |              0 |            22 |            3623 | 1564 |      217153 |   17.7 |    56.43 |  -39.4 |
| bgm           | vanilla_vtr  |         25836 |              0 |            11 |            2270 | 2734 |      335778 |   19.9 |    50.17 |  -62.8 |
| mcml          | mosaic |         83831 |             83 |            53 |           29259 | 7345 |     1046014 |     46 |    21.74 |   -155 |
| mcml          | vanilla_vtr  |         81220 |            159 |            27 |           25074 | 6621 |      878277 |     48 |    20.83 |   -130 |
| stereovision0 | mosaic |          7047 |              0 |             0 |            2637 |  705 |       49206 |   3.24 |    308.7 |   -6.9 |
| stereovision0 | vanilla_vtr  |          6878 |              0 |             0 |            2659 |  719 |       50292 |   3.56 |    280.6 |  -9.26 |
| stereovision1 | mosaic |          7414 |              0 |            76 |            2126 |  858 |      125096 |   5.16 |    193.7 |  -9.79 |
| stereovision1 | vanilla_vtr  |          6363 |              0 |            40 |            2174 |  765 |      103906 |   5.16 |      194 |   -7.7 |
| stereovision2 | mosaic |          9471 |              0 |           234 |           12069 | 1673 |      344030 |   13.6 |    73.34 |  -30.5 |
| stereovision2 | vanilla_vtr  |          8430 |              0 |           179 |           12081 | 1686 |      363322 |     15 |    66.48 |  -33.6 |

| Flow                              | LUTs (packed) | BRAMs (packed) | DSPs (packed) | Adders (packed) |    CLBs | Wire length | CPD ns | Fmax MHz |
| --------------------------------- | ------------: | -------------: | ------------: | --------------: | ------: | ----------: | -----: | -------: |
| mosaic                      |      15911.15 |           8.41 |         19.54 |         4689.49 | 1801.14 |   266890.02 |   19.4 |    51.54 |
| vanilla_vtr                       |      17755.21 |           8.56 |         12.36 |         4155.27 | 1972.72 |   271742.66 |  20.76 |    48.14 |
| % diff (mosaic/vanilla_vtr) |       -10.39% |         -1.73% |       +58.08% |         +12.86% |  -8.70% |      -1.79% | -6.56% |   +7.05% |
| x diff (mosaic/vanilla_vtr) |         0.90x |          0.98x |         1.58x |           1.13x |   0.91x |       0.98x |  0.93x |    1.07x |

## Building Mosaic
Requires a built vtr tree (`build/bin/yosys-config` must exist). The mosaic plugin only needs yosys headers; it does not link any vtr library. It compiles wildebeest-originated sources under `mosaic/wildebeest/` together with mosaic-only sources under `mosaic/src/`.

```shell
make -j$(nproc)
bash mosaic/build_mosaic.sh
```

This builds the mosaic plugin against the vtr yosys and installs it as `wildebeest.so` into `build/share/yosys/plugins/` (yosys loads it with `plugin -i wildebeest`).

## Running Mosaic
```shell
./vtr_flow/scripts/run_vtr_flow.py <circuit.v> <arch.xml> -start mosaic
```

Options are the following:

- `-mosaic_script <path>`: custom yosys template script (default `vtr_flow/misc/mosaic/template/synthesis.tcl`)
- `-top_module <name>`: top module; leaving it empty means yosys `-auto-top`

The stage writes `<circuit>.mosaic.blif`, logs to `mosaic.out`, and post-processes the blif (prunes unused blackbox model declarations yosys emits but the design never instantiates, instantiated models are always kept, then applies `vtr_flow/misc/mosaic/template/fix_blif_for_vpr.py` for ram addr pads, hierarchical net dots, and latch-q uniquify). After that the flow continues through abc and vpr exactly like the odin and parmys legs.

## Primitive profiles

`template/profiles.tcl` defines profiles as data (`requireClassicRams`, `forceStubAll`, `inferClassicMulAdd`):

| Profile | Behavior |
|---------|----------|
| `vtr_classic` | Infer `$mul`/`$add`/`$sub` onto classic models when present |
| `passthrough_exotics` | Forces `stubAllHardblocks`; rtl-instantiated exotics keep via identity maps |

Unknown `primitiveProfile` values error. Missing classic multiply without a role/template warns (inferred `$mul` stays soft).

## Classic model contract

Required for a normal mosaic run: `single_port_ram` and `dual_port_ram` (or aliases). Optional: `multiply`, `adder`. Carry-chain `add_sub_map` is emitted only when the adder has `cin`/`cout`/`sumout`; otherwise `$add`/`$sub` stay soft (including when an adder model exists but is not carry-style).

If the arch uses different model names, set in `arch_config.tcl`:

```tcl
set aliasMultiply my_dsp_mult
set aliasAdder my_carry
set aliasSinglePortRam my_spram
set aliasDualPortRam my_dpram
```

These become `vtr_arch_rules -alias role=model`. Multiply/adder maps instantiate the aliased names. Ram aliases flow through generated whitebox / bit-lib stubs, `chtype`, keep lists, and `fix_blif_for_vpr.py` so BLIF `.subckt` names match the arch. Fixture: `mosaic/tests/fixtures/min_aliased_ram.xml` with `vtr_flow/misc/mosaic/min_aliased_ram/`.

## Exotic hardblocks

Full guide (decision table, fixtures, token list, koios vs classic): [`docs/doc-mosaic-exotic-hardblocks.md`](../docs/doc-mosaic-exotic-hardblocks.md).

Exotics are every hardblock model that is not classic `multiply` / `adder` / `single_port_ram` / `dual_port_ram` (after aliases). Mosaic never silently maps `$mul`/`$add` onto them. Pick one targeting mode in the arch support `arch_config.tcl`:

| Mode | `arch_config.tcl` | Binds inferred `$mul`/`$add`? | Typical use |
|------|-------------------|------------------------------|-------------|
| Identity passthrough | `set stubAllHardblocks 1` or `set primitiveProfile passthrough_exotics` | No — RTL must instantiate the cell | Koios `hard_block_include.v` |
| Per-model template | `set exoticTemplatePairs {{model path/to.tmpl}}` | Only if the `.tmpl` maps that op | Custom / FP cells |
| Role inference | `set exoticRoles {{model integer_mul}}` | Yes, for stock roles when ports match | Exotic with `a`/`b`/`out` |

Stock roles: `template/rules/roles/` (`integer_mul` → `$mul`, `integer_mac` → `$macc`). Example templates (not auto-loaded): `template/rules/examples/` (see that folder’s README). Role notes for the complex-DSP arch: `vtr_flow/misc/mosaic/k6FracN10LB_mem20K_complexDSP_customSB_22nm/model_roles.example.tcl`.

Fixtures (arch XML stem → policy dir under `vtr_flow/misc/mosaic/<stem>/`):

| Fixture | What it proves |
|---------|----------------|
| `mosaic/tests/fixtures/min_aliased_ram.xml` + `min_aliased_ram/` | Classic ram **aliases** (`my_spram`/`my_dpram`) emit matching BLIF names — not exotic DSP |
| `mosaic/tests/fixtures/min_exotic_integer_mul.xml` + `min_exotic_integer_mul/` | `exoticRoles {{my_mul integer_mul}}` binds `$mul` when classic `multiply` is absent |

When classic `multiply` is present, `integer_mul` roles are skipped so behavioral mul keeps using the classic map. Koios FP DSP cells usually need passthrough or a custom template, not `integer_mul`.

## Layout
- `mosaic/wildebeest/src/`: wildebeest-originated sources (`clk_domains.cc` / `max_level`, with the `-vtr_arch` patch)
- `mosaic/src/`: mosaic-only sources (`vtr_arch_*`, `arch_rule_gen`) compiled into the same plugin
- `mosaic/build_mosaic.sh`: builds and installs the plugin
- `vtr_flow/misc/mosaic/template/`: shared mosaic support (`synthesis.tcl`, `fix_blif_for_vpr.py`)
- `vtr_flow/misc/mosaic/template/rules/`: `-tpldir` inputs for `vtr_arch_rules` (`.tmpl` maps, roles, examples)
- `vtr_flow/misc/mosaic/template/abc/`: shared delay abc scripts (`build_delay_scr.py`)
- `vtr_flow/misc/mosaic/template/lut_models/`: lut techmap library used by synthesis
- `vtr_flow/misc/mosaic/<arch_xml_stem>/`: optional per-arch policy (`arch_config.tcl`)
- `vtr_flow/scripts/python_libs/vtr/mosaic/`: the vtr flow stage module (stem-named support dir only)
- `mosaic/scripts/dump_arch_info.py`: dump parsed arch summary (optional; compare against `mosaic/tests/golden/`)
- `mosaic/scripts/run_vtr_batch.py`: batch `run_vtr_flow.py` (1 core per run), live csv/status, optional `--watch`
- `mosaic/scripts/watch_compare.py`: live status table (used by `--watch`, or run alone in a second terminal)

The synthesis template tokens (`XXX`, `TTT`, `ZZZ`, `YYY`, `VVV`, `ARCH_SUPPORT_DIR`, `TDIR`) are replaced by the python flow stage before the template is passed to yosys. `ARCH_SUPPORT_DIR` is `vtr_flow/misc/mosaic/<arch_xml_stem>/` when that dir has `arch_config.tcl`; otherwise the run is facts-only.

`vtr_arch_rules` writes `arch_facts.tcl` (dsp/ram geometry from the arch xml). Keep `arch_config.tcl` for policy only (costs, abc scripts, `dspMinWidth`, `minHardMulWidth`, `minHardMemAbits`, `stubAllHardblocks`, aliases, exotic roles). Do not put dsp/ram widths in `arch_config.tcl`. Shared abc scripts live under `template/abc/` (rebuild with `abc/build_delay_scr.py`).

Soft/hard policy knobs (Parmys-adjacent, not a full mixer):

- `minHardMulWidth` — `$mul` with either operand at or below this width stays soft (default `0`; Parmys `min_hard_multiplier`)
- `minHardMemAbits` — drop shallower bram modes from libmap so those memories soft-map (default `0`)
- `hardAdderThreshold` / `dspMinWidth` — unchanged
- When `lutCost` / `cmpLutWidth` are left at defaults, synthesis derives them from scanned `lutK` / `lutK1`; empty abc scripts auto-select shared delay scripts only for fracturable K6-like (`lutK==6` and `lutK1` in range)

## QoR Compare (vanilla_vtr vs mosaic)
`run_vtr_batch.py` wraps the default `run_vtr_flow.py` call. Each run is pinned
to 1 core (`--num_workers 1`); `--jobs N` means N concurrent single-core runs.
Results land in `compare_output_<arch_stem>/` (`runs/`, `logs/`, `status/`,
`compare_results_<YYYYMMDD_HHMMSS>.csv`).

```shell
python3 mosaic/scripts/run_vtr_batch.py \
  --arch vtr_flow/arch/COFFE_22nm/k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml \
  --benchmark-dir vtr_flow/benchmarks/verilog/koios \
  --designs eltwise_layer conv_layer gemm_layer lenet \
  --include hard_block_include.v \
  --jobs 4 --watch
```

`--watch` spawns `watch_compare.py` in the same terminal. Or run the watcher
yourself (defaults to newest `compare_output*` if `--dir` omitted):

```shell
python3 mosaic/scripts/watch_compare.py --dir compare_output_<arch_stem>
```

Useful flags: `--flows mosaic`, `--no-rerun`. Omit `--designs` to take
every `*.v` in the bench dir (except `*_include.v`).

Single-circuit smoke (after rebuilding mosaic if C++ changed):
```shell
./vtr_flow/scripts/run_vtr_flow.py vtr_flow/benchmarks/verilog/diffeq1.v \
  vtr_flow/arch/COFFE_22nm/k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml \
  -start mosaic

./vtr_flow/scripts/run_vtr_flow.py vtr_flow/benchmarks/verilog/koios/lenet.v \
  vtr_flow/arch/COFFE_22nm/k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml \
  -start mosaic -include vtr_flow/benchmarks/verilog/koios/hard_block_include.v
```

## Regression Tests
The `vtr_reg_basic_mosaic` suite runs basic_timing circuits (`ch_intrinsics.v`, `diffeq1.v`, `multiclock_reader_writer.v`) on `k6_frac_N10_frac_chain_mem32K_40nm.xml` with `-start mosaic`.

```shell
./run_reg_test.py vtr_reg_basic_mosaic -j4
```

Parmys vs Mosaic hardblock BLIF model compare (also run in `RegressionWithMosaic` CI):

```shell
python3 mosaic/scripts/compare_frontend_blif_models.py --run \
  --circuit vtr_flow/benchmarks/verilog/diffeq1.v \
  --arch vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml
```

A separate koios mosaic smoke task lives at `vtr_flow/tasks/regression_tests/vtr_reg_basic_mosaic/koios` (`test.v` + `hard_block_include.v` on the complex-DSP arch). Run it with:

```shell
./vtr_flow/scripts/run_vtr_task.py regression_tests/vtr_reg_basic_mosaic/koios
```

The `RegressionWithMosaic` job in `.github/workflows/test.yml` builds mosaic on top of the regular release build artifact and runs the basic suite.

## Verilator Check
The `mosaic/verilator_check/` flow checks functional equivalence between the original rtl and the generated post-synthesis and post-abc blifs.

```shell
python3 mosaic/verilator_check/run_random_check.py \
  --run-dir <harness_run_dir> --vectors 200000 --seed 1
```

Optional flags: `--check-mem-init` (fail if rtl memory init cannot survive hard ram blackboxes), `--directed-ram` (same-addr read/write and dual-port write/write when ports match), `--ram-zero-init` (force sim rams to 0; default leaves mem uninitialized so dropped init can surface).

The checker converts both blifs back to verilog with yosys, builds a three-DUT testbench containing the rtl, post-synth design, and post-abc design, and drives identical random input vectors to all three. Hardblock simulation models are provided in `verilator_check/models/sim_hardblocks.v`.


