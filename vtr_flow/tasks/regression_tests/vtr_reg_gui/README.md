# vtr_reg_gui — VPR Qt GUI Regression Suite

A small regression-test set covering the VPR Qt GUI flow paths that
run today. Every task pulls VPR through the full pack/place/route
pipeline under `QT_QPA_PLATFORM=offscreen` and asserts that the
graphics-command parser, the offscreen Qt loader, and the VPR exit
path remain non-regressed.

Authority for this suite is
`doc/src/dev/vpr_gui_test_implementation_plan.rst` §7.5; defects
discovered while building it are recorded in
`doc/src/dev/vpr_gui_defects_discovered.rst`.

## Tasks

| Task                       | Purpose                                                      |
|----------------------------|--------------------------------------------------------------|
| `gui_disp_off_smoke`       | Full flow with `--disp off`, no `--graphics_commands`. The   |
|                            | basement of the suite — everything else assumes this passes. |
| `gui_set_nets`             | Graphics-command parser accepts `set_nets 1; exit 0`.        |
| `gui_set_routing_util`     | Graphics-command parser accepts `set_routing_util 1; exit 0`.|
| `gui_set_draw_block_text`  | Graphics-command parser accepts                              |
|                            | `set_draw_block_text 1; exit 0`.                             |

The `set_*` tasks **do not** assert on rendered output: DEF-009 in
`vpr_gui_defects_discovered.rst` documents that those state setters
are inert under `--disp off`. The tasks therefore guard parser
acceptance + clean exit only; rendered-output assertions live in
Layer 5 (`vpr/test/gui/run_visual_regression.sh`) and will become
meaningful once DEF-009 is fixed.

## Running

```bash
# Build first (any layout that produces build/vpr/vpr will do):
make CMAKE_PARAMS="-DVPR_USE_EZGL=on" -j$(nproc) vpr

# Then the suite:
source .venv/bin/activate
QT_QPA_PLATFORM=offscreen ./run_reg_test.py vtr_reg_gui
```

`QT_QPA_PLATFORM=offscreen` must be exported before invoking
`run_reg_test.py`; the task framework forwards the parent shell
environment to each `vpr` invocation. S0 §S0.2 verified this
propagation works (offscreen plugin shows up in `QT_DEBUG_PLUGINS=1`
output).

## Risk-7 mitigation (macOS parmys)

Every `config.txt` pins `script_params_common=-start vpr ...` so VPR
consumes the prepared `.blif` directly and the front-end (parmys/odin)
is bypassed. This is required on macOS because `parmys.so` does not
link with Apple `ld64` (DEF-008 in the discovered-defects log; see
`vpr_gui_s0_validation.rst` §S0.3 for the link-failure signature).

## Add a new task

1. `mkdir -p new_task_name/config && cd new_task_name/config`
2. Copy any of the existing `config.txt` files; adjust
   `script_params_common` for the desired command flags.
3. Append `regression_tests/vtr_reg_gui/<new_task_name>` to
   `task_list.txt`.
4. The first run will produce a `golden_results.txt` under
   `<task>/config/`; review and commit it.

## Out-of-scope (Session S10)

`save_graphics`-based tasks (`gui_pack_place_save_png`,
`gui_route_save_png`) are deferred to S10 because they are
blocked by DEF-004 (see `vpr_gui_defect_log.rst`) and DEF-009
(state setters inert; see discovered-defects log).
