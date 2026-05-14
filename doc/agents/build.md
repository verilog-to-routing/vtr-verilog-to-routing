# VTR Build Guide

Always build from the root directory using `make` — never invoke `cmake` directly, as the Makefile wrapper handles configuration correctly and prevents in-source builds.

```shell
# Standard build
make

# Debug build
make BUILD_TYPE=debug

# Build only a specific target, with parallel jobs
make -j8 vpr

# Build with specific CMake options
make CMAKE_PARAMS="-DVTR_ENABLE_SANITIZE=true"
```

Build artifacts go in `build/`. CMake options are sticky — once set, subsequent `make` calls reuse them until explicitly changed.

## Key CMake Options

Pass via `CMAKE_PARAMS="-DOPTION=value"`:

- `VTR_ASSERT_LEVEL` — 0–4, controls assertion density (default 2)
- `VTR_ENABLE_SANITIZE` — address/leak/UB sanitizers
- `VTR_ENABLE_PROFILING` — gprof profiling
- `VPR_USE_EZGL` — graphics library (auto/on/off)
- `WITH_PARMYS` — build Yosys frontend (default ON)
- `WITH_ODIN` — build legacy Odin frontend (default OFF)
- `VTR_IPO_BUILD` — interprocedural optimization (default auto/on); set to `off` during development to reduce build times and improve debuggability
