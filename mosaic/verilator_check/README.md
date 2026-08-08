# Verilator check
Random-vector check that RTL, post-mosaic blif, and post-VTR-abc blif agree. These scripts have not been hardened very well and so may have issues during runs as they were build for uses outside of the VTR-flow, the scripts are provided here for future CI integration or for use quick local checks for future Mosaic Development.

To run the random checking, do the following:
```shell
python3 mosaic/verilator_check/run_random_check.py --run-dir <harness_run_dir> --vectors 200000 --seed 1
```

Or pass paths explicitly: `--rtl`, `--synth-blif`, `--abc-blif`.

Needs Yosys and Verilator on `PATH` (or a VTR build with `build/bin/yosys`). Hardblock sim models are in `models/sim_hardblocks.v`.

Optional flags:
- `--check-mem-init` fails if rtl memory init cannot survive hard ram blackboxes.
- `--directed-ram` adds same-addr read/write and dual-port write/write cases when ports match.
- `--ram-zero-init` forces sim rams to zero. The default leaves memory uninitialized so dropped init can surface.
