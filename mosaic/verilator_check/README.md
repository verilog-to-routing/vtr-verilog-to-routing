# Verilator check

Random-vector check that RTL and the post-synth blif (after synth + abc, or mosaic in-yosys abc) agree.

## Standalone

```shell
python3 mosaic/verilator_check/run_random_check.py --run-dir <vtr_temp_or_batch_run_dir> --vectors 200000 --seed 1
```

Or pass paths explicitly:

```shell
python3 mosaic/verilator_check/run_random_check.py \
  --rtl path/to/top.v \
  --post-synth-blif path/to/design.pre-vpr.blif
```

Needs Yosys and Verilator on `PATH` (or a VTR build with `build/bin/yosys`). Hardblock sim models are in `models/sim_hardblocks.v`.

Optional flags:
- `--check-mem-init` fails if rtl memory init cannot survive hard ram blackboxes.
- `--directed-ram` adds same-addr read/write and dual-port write/write cases when ports match.
- `--ram-zero-init` forces sim rams to zero. The default leaves memory uninitialized so dropped init can surface.

## Via run_vtr_flow

```shell
./vtr_flow/scripts/run_vtr_flow.py circuit.v arch.xml -start mosaic -end mosaic -verilator_check
./vtr_flow/scripts/run_vtr_flow.py circuit.v arch.xml -start parmys -end abc -verilator_check
```

## Via batch runner

```shell
python3 mosaic/scripts/run_vtr_batch.py \
  --arch <arch.xml> --benchmark-dir <dir> --designs <names...> \
  --flows mosaic vtr --verilator-check mosaic --jobs 4 --watch
```

`--verilator-check` requires one or more flow names (`mosaic`, `vanilla_vtr`; aliases `frank`, `parmys`, `vtr`). CSV column `verilator_status` is `pass`, `fail`, or `missing`.

## Mosaic regression

`vtr_reg_basic_mosaic` runs mosaic CAD with `-verilator_check`. After the suite:

```shell
python3 mosaic/scripts/verify_regression_vcheck.py --suite vtr_reg_basic_mosaic
```
