# Frankenstein Flow
Needs to be renamed!

Frankenstein is a third synthesis frontend for the vtr flow, alongside Odin II and parmys. It uses the wildebeest yosys plugin to synthesize verilog into a blif netlist that then goes through the standard abc and vpr stages.

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

The stage writes `<circuit>.frankenstein.blif`, logs to `frankenstein.out`, and post-processes the blif (prunes blackbox models the arch never declares, then applies `vtr_flow/misc/frankenstein/k6/fix_blif_for_vpr.py` for ram addr pads, hierarchical net dots, and latch-q uniquify). after that the flow continues through abc and vpr exactly like the odin and parmys legs.

## Layout
- `frankenstein/wildebeest/src/`: minimized wildebeest source with the frankenstein modifications applied
- `frankenstein/build_frankenstein.sh`: builds and installs the plugin
- `vtr_flow/misc/frankenstein/template/`: architecture-agnostic yosys synthesis template + rule templates
- `vtr_flow/misc/frankenstein/template/templates/*.tmpl`: templates used by `vtr_arch_rules -tpldir` to generate BRAM, multiply, and hardblock stub files
- `vtr_flow/misc/frankenstein/k6/`: K6 arch support files including `arch_config.tcl`, hand-written techmaps, fallback statics, abc scripts, and `fix_blif_for_vpr.py`
- `vtr_flow/scripts/python_libs/vtr/frankenstein/`: the vtr flow stage module
- `frankenstein/verilator_check/`: random-vector equivalence checking between rtl, post-synthesis blif, and post-abc blif

The synthesis template is architecture agnostic. Its tokens (`XXX`, `TTT`, `ZZZ`, `YYY`, `VVV`, `K6D`, `TDIR`) are replaced by the python flow stage before the template is passed to yosys.

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


