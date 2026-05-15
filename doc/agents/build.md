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

## Optional Dependencies

VPR has several optional system libraries that are auto-detected at configure time via `find_package`. When a library is not found, the features it enables are compiled out behind an `#ifdef` — no CMake flag is needed, and no error is raised. Look for `Found` / `Not Found` lines in the CMake output to confirm what was detected.

| Library | Feature gated | System package (Debian/Ubuntu) |
|---------|--------------|-------------------------------|
| Eigen3 | LP/QP analytical solvers in AP (`EIGEN_INSTALLED`) | `libeigen3-dev` |
| Cap'n Proto | Binary RR graph serialization (`VTR_ENABLE_CAPNPROTO`) | `libcapnp-dev capnproto` |

If a feature appears missing at runtime or a code path is `#ifdef`-gated and inactive, check whether its library was found during configuration.

## Key CMake Options

Pass via `CMAKE_PARAMS="-DOPTION=value"`:

- `VTR_ASSERT_LEVEL` — 0–4, controls assertion density (default 2)
- `VTR_ENABLE_SANITIZE` — address/leak/UB sanitizers
- `VTR_ENABLE_PROFILING` — gprof profiling
- `VPR_USE_EZGL` — graphics library (auto/on/off)
- `WITH_PARMYS` — build Yosys frontend (default ON)
- `WITH_ODIN` — build legacy Odin frontend (default OFF)
- `VTR_IPO_BUILD` — interprocedural optimization (default auto/on); set to `off` during development to reduce build times and improve debuggability
