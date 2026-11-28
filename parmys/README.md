# Parmys plugin (Partial Mapper for Yosys)

The Parmys plugin performs VTR's partial technology mapping inside Yosys: it takes the
elaborated RTLIL from Yosys, maps arithmetic (`$add`, `$sub`, `$mul`) and memories
(`$mem`, `$mem_v2`) onto the hard blocks described in a VTR architecture file, or into soft
logic when no suitable hard block exists, and hands the result back to Yosys for LUT mapping.
It descends from the partial mapper of [Odin-II](../odin_ii).

The Parmys plugin lives in the VTR tree and is built by VTR's CMake only; there is no standalone build.

## Layout

| Path | Contents |
|---|---|
| `parmys-plugin/` | plugin sources (`parmys.cc` is the Yosys pass, `core/` the hard-block mappers, `netlist/` the Odin netlist data structures, `mapping/` partial mapping) |
| `parmys-plugin/techlibs/` | Yosys techmap libraries and `vtr_primitives.v`, read by `vtr_flow/misc/yosys/synthesis.tcl` |
| `regression_test/` | the Parmys regression suite: benchmark tasks with stored `synthesis_result.json` expectations, run through `verify_parmys.sh` |

## Building

```shell
make                 # full VTR build, includes the plugin
make -j8 parmys      # the plugin only
```

The build installs `build/share/yosys/plugins/parmys.so` and the files of `parmys-plugin/techlibs/`
(the techmap libraries and `vtr_primitives.v`, the primitive models the synthesis script reads with
`read_verilog +/parmys/vtr_primitives.v`) into `build/share/yosys/`. Disable with `-DWITH_PARMYS=OFF`.

`vtr_primitives.v` is a copy of `vtr_flow/primitives.v` with the `DFF` clock port still named `clk`.
Switching it to the `vtr_flow` version changes the order of top-level ports in the generated BLIF
(ABC and VPR results are order sensitive), so that reconciliation is left for a change that
regenerates golden results.

## Running

Within the VTR flow (the default):

```shell
vtr_flow/scripts/run_vtr_flow.py my_design.v vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml
```

Directly in Yosys:

```
plugin -i parmys
read_verilog my_design.v
parmys -a vtr_architecture.xml
help parmys
```

Options of the `parmys` pass:

```
    -a ARCHITECTURE_FILE    VTR FPGA architecture description file (XML)
    -c XML_CONFIGURATION_FILE
                            Configuration file (see vtr_flow/misc/basic_odin_config_split.xml)
    -top top_module         set the specified module as design top module
    -nopass                 run only the mapper, none of the Yosys passes it normally schedules
    -exact_mults N / -mults_ratio R
                            mix hard-block and soft-logic multipliers
    -vtr_prim               load the VTR primitives as modules first
    -viz                    write the netlist as graphviz at the elaborated, optimized and mapped stages
```

## Testing

```shell
./run_reg_test.py parmys_reg_basic -j8            # light suite, as run by CI
./run_reg_test.py parmys_reg_strong -j8           # heavy suite
parmys/verify_parmys.sh --help                    # run a single task, regenerate expectations, ...
```

The suites compare Yosys/Parmys synthesis statistics (cell counts, warnings) of the designs under
`regression_test/benchmark/task/*` against stored `synthesis_result.json` expectations. To add a
design, put it in `regression_test/benchmark/verilog/<task>/`, list it in the task's
`task_list.conf`, and record its expectation with `verify_parmys.sh --test <task> --generate_expectation`
(run from `parmys/`, with `--nb_of_process 1`). The VTR flow regressions (`vtr_reg_*`) exercise the
plugin end to end and are described in `doc/agents/testing.md`.
