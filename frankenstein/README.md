# Frankenstein Flow
Needs to be renamed!

Frankenstein is a third synthesis frontend for the vtr flow, alongside Odin II and parmys. It uses the wildebeest yosys plugin to synthesize verilog into a blif netlist that then goes through the standard abc and vpr stages.

| Design        | Flow         | LUTs (packed) | BRAMs (packed) | DSPs (packed) | Adders (packed) | CLBs | Wire length | CPD ns | Fmax MHz | WNS ns |
| ------------- | ------------ | ------------: | -------------: | ------------: | --------------: | ---: | ----------: | -----: | -------: | -----: |
| LU32PEEng     | frankenstein |         44076 |            168 |            64 |           17192 | 5245 |      989996 |   71.1 |    14.06 |   -161 |
| LU32PEEng     | vanilla_vtr  |         69139 |            168 |            32 |           13291 | 7431 |     1213168 |   75.4 |    13.26 |   -195 |
| LU8PEEng      | frankenstein |         14339 |             45 |            16 |            5313 | 1619 |      256915 |   76.9 |    13.01 |   -149 |
| LU8PEEng      | vanilla_vtr  |         19611 |             45 |             8 |            4381 | 2133 |      287638 |   73.3 |    13.63 |   -145 |
| arm_core      | frankenstein |         11556 |             40 |             0 |             357 | 1122 |      210414 |   19.8 |    50.59 |  -34.6 |
| arm_core      | vanilla_vtr  |          9409 |             24 |             0 |             384 |  862 |      152188 |   23.7 |     42.2 |    -39 |
| bgm           | frankenstein |         13559 |              0 |            22 |            3623 | 1564 |      217153 |   17.7 |    56.43 |  -39.4 |
| bgm           | vanilla_vtr  |         25836 |              0 |            11 |            2270 | 2734 |      335778 |   19.9 |    50.17 |  -62.8 |
| mcml          | frankenstein |         83831 |             83 |            53 |           29259 | 7345 |     1046014 |     46 |    21.74 |   -155 |
| mcml          | vanilla_vtr  |         81220 |            159 |            27 |           25074 | 6621 |      878277 |     48 |    20.83 |   -130 |
| stereovision0 | frankenstein |          7047 |              0 |             0 |            2637 |  705 |       49206 |   3.24 |    308.7 |   -6.9 |
| stereovision0 | vanilla_vtr  |          6878 |              0 |             0 |            2659 |  719 |       50292 |   3.56 |    280.6 |  -9.26 |
| stereovision1 | frankenstein |          7414 |              0 |            76 |            2126 |  858 |      125096 |   5.16 |    193.7 |  -9.79 |
| stereovision1 | vanilla_vtr  |          6363 |              0 |            40 |            2174 |  765 |      103906 |   5.16 |      194 |   -7.7 |
| stereovision2 | frankenstein |          9471 |              0 |           234 |           12069 | 1673 |      344030 |   13.6 |    73.34 |  -30.5 |
| stereovision2 | vanilla_vtr  |          8430 |              0 |           179 |           12081 | 1686 |      363322 |     15 |    66.48 |  -33.6 |

| Flow                              | LUTs (packed) | BRAMs (packed) | DSPs (packed) | Adders (packed) |    CLBs | Wire length | CPD ns | Fmax MHz |
| --------------------------------- | ------------: | -------------: | ------------: | --------------: | ------: | ----------: | -----: | -------: |
| frankenstein                      |      15911.15 |           8.41 |         19.54 |         4689.49 | 1801.14 |   266890.02 |   19.4 |    51.54 |
| vanilla_vtr                       |      17755.21 |           8.56 |         12.36 |         4155.27 | 1972.72 |   271742.66 |  20.76 |    48.14 |
| % diff (frankenstein/vanilla_vtr) |       -10.39% |         -1.73% |       +58.08% |         +12.86% |  -8.70% |      -1.79% | -6.56% |   +7.05% |
| x diff (frankenstein/vanilla_vtr) |         0.90x |          0.98x |         1.58x |           1.13x |   0.91x |       0.98x |  0.93x |    1.07x |

## Building the Plugin
Requires a built vtr tree (`build/bin/yosys-config` must exist). The plugin only needs yosys headers; it does not link any vtr library.

```shell
make -j$(nproc)
bash frankenstein/build_frankenstein.sh
```

This builds `wildebeest/` against the vtr yosys and installs `wildebeest.so` into `build/share/yosys/plugins/`.

## Running the Plugin
```shell
./vtr_flow/scripts/run_vtr_flow.py <circuit.v> <arch.xml> -start frankenstein
```

Options are thhe following:
- `-frankenstein_script <path>`: custom yosys template script (default `vtr_flow/misc/frankenstein/template/synthesis.tcl`)
- `-top_module <name>`: top module; leaving it empty means yosys `-auto-top`

The stage writes `<circuit>.frankenstein.blif`, logs to `frankenstein.out`, and post-processes the blif (prunes unused blackbox model declarations yosys emits but the design never instantiates — instantiated models are always kept — then applies `vtr_flow/misc/frankenstein/template/fix_blif_for_vpr.py` for ram addr pads, hierarchical net dots, and latch-q uniquify). After that the flow continues through abc and vpr exactly like the odin and parmys legs.

## Layout
- `frankenstein/wildebeest/src/`: wildebeest-originated sources (`clk_domains.cc` / `max_level`, with the `-vtr_arch` patch)
- `frankenstein/src/`: frankenstein-only sources (`vtr_arch_*`, `arch_rule_gen`) compiled into the same plugin
- `frankenstein/build_frankenstein.sh`: builds and installs the plugin
- `vtr_flow/misc/frankenstein/template/`: architecture-agnostic yosys synthesis template + rule templates
- `vtr_flow/misc/frankenstein/template/templates/*.tmpl`: templates used by `vtr_arch_rules -tpldir` to generate BRAM, multiply, and hardblock stub files
- `vtr_flow/misc/frankenstein/k6/`: K6 per-arch knob config (`arch_config.tcl`) — the only per-arch artifact; everything else is generated from `template/templates/*.tmpl` or shared under `template/`
- `vtr_flow/scripts/python_libs/vtr/frankenstein/`: the vtr flow stage module
- `frankenstein/scripts/`: `compare_flow.py` (vanilla_vtr vs frankenstein) and `watch_compare.py` (live status table)

The synthesis template is architecture agnostic. Its tokens (`XXX`, `TTT`, `ZZZ`, `YYY`, `VVV`, `K6D`, `TDIR`) are replaced by the python flow stage before the template is passed to yosys.

## QoR Compare (vanilla_vtr vs frankenstein)
Runs the eight README circuits on `k6_frac_N10_frac_chain_mem32K_40nm.xml` through both front-ends:

```shell
# terminal 1 — launch the compare
python3 frankenstein/scripts/compare_flow.py --jobs 4

# terminal 2 — live status table
python3 frankenstein/scripts/watch_compare.py
```

Useful flags: `--arch <other_arch.xml>`, `--flows frankenstein`, `--designs arm_core bgm`, `--jobs 8 --large-jobs 3`, `--no-rerun`. Results land in `compare_output_<arch_stem>/` (`runs/`, `logs/`, `status/`, `compare_results.csv`).

## Regression Tests
The `vtr_reg_basic_frankenstein` suite runs basic_timing circuits (`ch_intrinsics.v`, `diffeq1.v`, `multiclock_reader_writer.v`) on `k6_frac_N10_frac_chain_mem32K_40nm.xml` with `-start frankenstein`.

```shell
./run_reg_test.py vtr_reg_basic_frankenstein -j4
```

The `RegressionWithFrankenstein` job in `.github/workflows/test.yml` builds the plugin on top of the regular release build artifact and runs this suite.

## Verilator Check
The `frankenstein/verilator_check/` flow checks functional equivalence between the original rtl and the generated post-synthesis and post-abc blifs.

```shell
python3 verilator_check/run_random_check.py ...
```

The checker converts both blifs back to verilog with yosys, builds a three-DUT testbench containing the rtl, post-synth design, and post-abc design, and drives identical random input vectors to all three. Hardblock simulation models are provided in `verilator_check/models/sim_hardblocks.v`.


