# Wildebeest
This tree holds a small slice of [Wildebeest](https://github.com/zeroasiccorp/wildebeest) that Mosaic links into the Yosys plugin (`mosaic.so`). The CMake project root is `mosaic/`; this folder only adds the `wildebeest` object library. Mosaic-only sources live in `mosaic/src/`.

## What is used
Mosaic does not take the full upstream tree. Only these pieces are in the build today:
- `src/clk_domains.cc` provides `max_level`, with Mosaic's `-vtr_arch` patch
- `src/abc_scripts/LUT6/BEST/delay_lut6.scr` is a ABC script used from the mosaic synthesis templates

## Upstream pin
Last synced from [zeroasiccorp/wildebeest@f38169c](https://github.com/zeroasiccorp/wildebeest/commit/f38169c47c11c53604d4d88ba330d7a16b423bb1).

The sources are lightly modified so they compile and hook into Mosaic. There have been some modifications to the Wildebeest source code to allow to compile with Mosaic but the behaviour remains almost the same.
