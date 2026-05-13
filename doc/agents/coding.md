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

## Logging

Use VTR logging macros instead of `printf`, `std::cerr`, or `exit()`:
- `VTR_LOG(...)` — informational
- `VTR_LOG_WARN(...)` — warnings
- `VTR_LOG_ERROR(...)` — errors

## Use of `auto`

Use `auto` only when the type is long or complex (iterators, lambdas, long template return types). Write out short, clear types explicitly (`int`, `std::string`, `RRNodeId`, etc.).
