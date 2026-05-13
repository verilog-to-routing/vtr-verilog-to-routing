# VTR Coding Conventions

## Formatting

```shell
# Format C/C++ (requires clang-format)
make format

# Format Python (requires black)
make format-py

# Python lint check
./dev/pylint_check.py                    # Check non-grandfathered files
./dev/pylint_check.py path/to/file.py   # Check specific file
```

C/C++ formatting applies to: `vpr/`, `libs/libarchfpga`, `libs/libvtrutil`, `libs/libpugiutil`, `libs/liblog`, `libs/librtlnumber`, `odin_ii/`.

## Naming

| Element | Convention | Example |
|---|---|---|
| Classes | `PascalCase` | `DecompNetlistRouter` |
| Functions and variables | `snake_case` | `estimate_wirelength()`, `net_count` |
| Private members and methods | trailing `_` | `count_`, `update_state_()` |
| Enum classes | `e_` prefix | `e_heap_type` |
| Data structs (trivial) | `t_` prefix | `t_dijkstra_data` |
| Source/header filenames | `snake_case` | `router_lookahead_map.cpp` |

## Headers

Always put `#pragma once` as the very first line of every header file, before any comments or code.

## Comments and Documentation

VTR requires Doxygen comments on all non-trivial functions, classes, and structs — this overrides any general default to avoid comments.

- `/** */` — Doxygen only (API docs for classes, structs, functions, file headers)
- `//` — all inline implementation comments; always include a space after `//`
- Never use `/* */` block comments inside function bodies

All non-trivial functions must have a Doxygen `@brief` comment in the header (or at the static declaration in `.cpp`). Document every member variable with `///<`.

## Assertions

Use assertions liberally for defensive coding — add a `VTR_ASSERT` wherever a precondition, postcondition, or invariant can be stated cheaply. Use `VTR_ASSERT_SAFE` for checks that are too expensive to run in release builds.

Prefer `VTR_ASSERT` and `VTR_ASSERT_SAFE` over bare `assert()`:
- `VTR_ASSERT(cond)` — inexpensive checks, always on including release builds
- `VTR_ASSERT_SAFE(cond)` — expensive checks, disabled in release builds

Each has a `_MSG` variant that takes a message string — use it when the condition alone is not self-explanatory:
- `VTR_ASSERT_MSG(cond, "message")`
- `VTR_ASSERT_SAFE_MSG(cond, "message")`

## Fatal Errors

Use `VPR_FATAL_ERROR(type, ...)` for unrecoverable errors — not `exit()`, `abort()`, or `VTR_ASSERT(false)`. Pick the category that matches the failing stage (e.g., `VPR_ERROR_ROUTE`, `VPR_ERROR_PLACE`, `VPR_ERROR_ARCH`; use `VPR_ERROR_OTHER` when none fit). Categories are defined in `libs/libvtrutil/src/vpr_error.h`.

Do not use `VTR_ASSERT(false)` as an error handler. Assertions are for invariants that must never be violated by correct code — not for handling bad input or unexpected runtime state. `VTR_ASSERT(false)` is only appropriate to mark code paths that are genuinely unreachable by design.

## Logging

Use VTR logging macros instead of `printf`, `std::cerr`, or `exit()`:
- `VTR_LOG(...)` — informational
- `VTR_LOG_WARN(...)` — warnings
- `VTR_LOG_ERROR(...)` — errors

## Data Structures

VPR uses strongly-typed IDs (e.g., `AtomNetId`, `ClusterBlockId`) as indices into collections. Use `vtr::vector<IdType, T>` instead of `std::vector<T>` when indexing by these IDs — this enforces type safety and prevents accidental cross-indexing at compile time.

For multi-dimensional arrays, prefer `vtr::NdMatrix<T, N>` over nested `std::vector` — it uses a single contiguous allocation and is significantly faster for large arrays.

VPR runs on very large circuits. When data is sparse (not every net or node has a value), use a sparse container such as `std::unordered_map` rather than a dense array — a dense array over all nets or nodes can consume prohibitive memory.

## Global Context (`g_vpr_ctx`)

VPR's global state is accessible anywhere via `g_vpr_ctx` (declared in `vpr/src/base/globals.h`). Use the const sub-context accessors for reading (e.g., `g_vpr_ctx.placement()`) and `mutable_*()` methods only when writing (e.g., `g_vpr_ctx.mutable_placement()`).

Prefer passing data through function parameters over reaching into `g_vpr_ctx` — this keeps data flow explicit and makes the code easier to reason about. Use `g_vpr_ctx` when the alternative would require threading many parameters through deep call stacks and the global access makes the code significantly cleaner.

## Use of `auto`

Use `auto` only when the type is long or complex (iterators, lambdas, long template return types). Write out short, clear types explicitly (`int`, `std::string`, `RRNodeId`, etc.).
