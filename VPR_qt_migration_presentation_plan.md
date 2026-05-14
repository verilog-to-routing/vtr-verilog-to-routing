# VPR Qt Migration — Presentation Plan

**Audience:** internal tech group (engineers familiar with VPR, FPGA P&R, mixed Qt/GTK background).
**Goal:** convey current state of the Qt migration, the three-renderer architecture, the GUI test framework, and the delta vs upstream master.
**Format:** slide deck. Prefer diagrams and tables over prose. Each slide title below is a suggested deck slide; bullets are speaker-note hints, not slide body text.

---

## 0. Title & agenda (1 slide)

- Title: *"VPR GUI: the Qt migration"*
- Agenda: (1) migration status, (2) renderers, (3) test framework, (4) upstream changes.
- One screenshot of the VPR GUI rendered with the RHI backend on a non-trivial benchmark.

---

## 1. Qt migration status

### Slide 1.1 — Qt 6.9.3, why this version

- **Required version: Qt 6.9.3** (enforced by `find_package` check in [libs/EXTERNAL/libezgl/CMakeLists.txt:25-36](libs/EXTERNAL/libezgl/CMakeLists.txt#L25-L36)).
- **Why we care about a recent Qt**: our default renderer is built on **QRhi**, Qt's modern Rendering Hardware Interface — a portable GPU abstraction over OpenGL / Vulkan / Direct3D / Metal. QRhi has been **part of Qt since 6.7** (it was a private API earlier; promoted to public/stable in 6.7), so we need a Qt new enough to expose it as a supported surface.
- **Reason for the 6.9.3 floor specifically**: Qt 6.8 has a bug in the RHI backend — geometry buffers are not invalidated after the MVP matrix changes, producing rendering artifacts. Fixed in 6.9.3.
- Mental model for the slide: *"QRhi gives us one renderer that works on every desktop OS, but it only matured into a public API in Qt 6.7, and the first version where it renders our scene correctly is 6.9.3."*
- Suggested visual: a small timeline bar — `Qt 6.7 (QRhi public) → Qt 6.8 (RHI MVP bug) → Qt 6.9.3 (our floor, ✓)` — followed by a side-by-side screenshot (artifacts on 6.8 vs clean on 6.9.3) if we still have one captured.

### Slide 1.2 — Target platforms

| Platform | Status | CI | Notes |
|---|---|---|---|
| Linux (Ubuntu 22.04 + 24.04) | **Active** | yes — `.github/workflows/test.yml` | Qt 6.9.3 via `aqtinstall` to `/opt/qt6/6.9.3/gcc_64` |
| macOS | **In Progress** — automatic GUI test framework run manually + quick manual smoke test | currently disabled | `install_brew_packages.sh` exists; libezgl ctor uses `std::construct_at` in a way Apple libc++ rejects — re-enables once fixed |
| Windows (win32) | **Planned** | none yet | no install script / no CI matrix; intended target via Qt's MinGW / MSYS2 build (MSVC explicitly out of scope for now) |

- Compilers exercised in CI: GCC 11–14, Clang 16–18.
- Suggested visual: traffic-light table (green / amber / red) for each OS.

### Slide 1.3 — Migration limitations / what's left

Three concrete items still on the work list. One bullet per slide.

#### 1.3.1 — QtGladeLoader

- We **kept the GTK `.glade` XML** as the single source of truth for the widget tree, and we parse it ourselves at runtime into Qt widgets — instead of porting the .glade files to Qt Designer `.ui` files.
- **Why**: upstream master continues to maintain `main.ui` as a Glade file. Forking to a different XML dialect would force a manual three-way merge every time upstream touches the UI. Parsing the Glade XML lets upstream changes flow in unchanged.

**Suggested diagram:**
```
   upstream main.ui (Glade XML)
            │
            ▼
   QtGladeLoader::loadFile()
            │
            ▼
       QMainWindow + QWidget tree
```

**Future state** (once upstream migrates the source-of-truth UI file):

```
   upstream main.ui (Qt Designer XML)
            │
            ▼
       Qt UI loader (built-in)
            │
            ▼
       QMainWindow + QWidget tree
```

In other words: once `main.ui` upstream is itself a Qt Designer file, we drop the custom GTK-XML parser entirely and use Qt's native UI loader — QtGladeLoader becomes dead code we can delete.

#### 1.3.2 — Code-comment coverage

- The Qt port introduced new modules (RHI backend, deferred renderer, RHI canvas widget, scene renderer) — public-API headers are documented, but many private helpers are not.
- Particularly thin: `rhi_renderer.cpp` (tile grid, band scheduling, MVP-only path) and `deferred_backend.cpp`.
- Next step: pass over hot files to bring them to Doxygen-friendly comments.

#### 1.3.3 — More automated GUI test coverage

- Cross-reference with section 3. Today we have ~220–250 test cases across 5 layers, but several gates are skipped:
  - Layer 5 (visual regression) goldens are blocked on **DEF-004** (Qt offscreen RHI heap corruption during `save_graphics`).
  - Coverage gate stays at baseline (35 % line / 22 % branch) rather than the 80 % target.
- Specific gaps to call out on the slide:
  - layer-3 widget coverage of recently-added popovers / menu buttons,
  - layer-4 keyboard navigation depth,
  - layer-5 goldens once DEF-004 lands.

---

## 2. Renderers

### Slide 2.0 — What QRhi actually is

- Qt's **Rendering Hardware Interface**: a thin portable abstraction over GPU APIs.
- Backends QRhi can map to, per platform:
  - **Linux**: OpenGL (GLES2/3), Vulkan
  - **Windows**: Direct3D 11/12, OpenGL, Vulkan
  - **macOS / iOS**: Metal
- We write one path in `rhi_renderer.cpp`; QRhi picks the right backend at runtime.

**Suggested diagram (a fan-out):**
```
        Our RHI renderer
              │
              ▼
            QRhi
   ┌─────┬───┴───┬─────┐
   ▼     ▼       ▼     ▼
OpenGL Vulkan  D3D11  Metal
```

### Slide 2.1 — Three renderers, side by side

This is the core comparison slide. Render as one table on the slide.

| Aspect | Immediate | Deferred | RHI |
|---|---|---|---|
| Backend | QPainter, synchronous | QPainter, batched | GPU via QRhi |
| Source | [immediate_renderer.cpp](libs/EXTERNAL/libezgl/src/qt/immediate_renderer.cpp) | [deferred_renderer.cpp](libs/EXTERNAL/libezgl/src/qt/deferred_renderer.cpp) | [rhi_renderer.cpp](libs/EXTERNAL/libezgl/src/qt/rhi_renderer.cpp) |
| **CPU usage** | high (one QPainter call per primitive) | medium (collect → flush batches) | low–medium (CPU does binning; GPU does draw) |
| **GPU usage** | none | none | high (intended path) |
| **VRAM usage** | none | none | non-trivial (vertex/index buffers per tile) |
| **Batching** | none | by `(primitive, color, line-width, dash, cap)` | tile-binned (32×32 = 1024 tiles) + style |
| **Multithreading** | none | none | **batching workers per band** (`std::thread::hardware_concurrency`) during scene composition |
| **Offscreen-visibility culling** | none | per-primitive viewport tests (line/rect/poly/arc/text/surface) | tile-band culling: only tiles intersecting the viewport are emitted |
| **Approx. throughput on 100 M-line stress scene** | ~0.1 fps | ~2 fps | **60+ fps** |

Notes for the speaker:
- RHI multithread batching is **scene composition**, not the GPU submit — workers slice commands into bands, each band fills its own tile-local buffers, then the main thread submits.
- Per-primitive visibility tests in deferred: see `screen_*_visible()` methods in [deferred_renderer.cpp:286-412](libs/EXTERNAL/libezgl/src/qt/deferred_renderer.cpp#L286-L412).
- Tile grid constant in RHI: `kTileGridDimension = 32` (1024 tiles total).

**Suggested visual on this slide:** small bar chart of fps from the stress-bench results, with one bar per renderer.

### Slide 2.2 — `redraw()` split idea (geometry vs camera-move)

- Most user interaction is **pan / zoom** — the geometry of the scene doesn't change, only the camera does.
- Today, RHI already has this split:
  - `flush()` — full path: re-records commands, re-bins into tiles, re-uploads buffers.
  - `flush_mvp_only()` — camera path: pushes a new 64-byte MVP matrix only; geometry buffers are reused, overlay is replayed from cache (`m_overlay_deferred->replay_overlay()`).
  - Selected by `rhi_backend.cpp:27-66`.
- Proposal: **mirror this split in the deferred renderer** — keep the batch cache across camera-only changes, redo only the screen-space projection.
- Suggested slide visual: two paths, one wide ("full redraw") and one narrow ("camera-only") feeding the same final frame.

```
   user action ──► redraw_geometry()  ──►  rebuild batches  ──┐
                                                              ▼
   pan/zoom   ──► on_camera_move() ──► update MVP only ───► present
```

### Slide 2.3 — How a renderer is chosen

- CLI: `--renderer <immediate|deferred|rhi>`
- Default: **`rhi`** (falls back to immediate if QRhi init fails on the host).
- Plumbed via `ezgl::renderer_type` enum in `render_backend.hpp`, propagated through `QtGladeLoader::setRendererType()` so the `GtkDrawingArea` element is materialised as either `RhiCanvasWidget` or `DrawingAreaWidget`.
- VPR-side: same flag, parsed in `vpr/src/base/read_options.cpp`.

---

## 3. GUI test framework

### Slide 3.1 — Five-layer architecture

Render as a pyramid or a layered stack. Layers go from cheapest/fastest (top) to slowest/highest fidelity (bottom).

| Layer | Kind | ~Cases | Where | Mechanism | Status |
|---|---|---|---|---|---|
| 1 | Headless smoke (CLI + `--graphics_commands`) | ~24 | [vpr/test/gui/run_headless_smoke.sh](vpr/test/gui/run_headless_smoke.sh) | bash + VPR CLI under `QT_QPA_PLATFORM=offscreen` | active |
| 2 | libezgl unit (Qt-only) | — | inside the libezgl submodule | Qt Test / Catch2 | **runs in the libezgl repo, not wired into VPR CTest yet** |
| 3 | Catch2 unit/integration (widget tree, draw state, renderer plumbing) | ~156 | `vpr/test/gui/test_*.cpp` (22 files) | Catch2 + fixtures (`EzglAppFixture`, `VprRunStageFixture`, `DrawStructsScope`) | active |
| 4 | Interactive (mouse / keyboard via `QTest`) | ~30–40 | same files, tagged `[layer4]` | Catch2 + `QTest::mouseMove`/`keyClick` | active (subset of L3 binary) |
| 5 | Visual regression (PNG + SSIM) | 14 cases | [vpr/test/gui/run_visual_regression.sh](vpr/test/gui/run_visual_regression.sh), goldens in [vpr/test/gui/golden/](vpr/test/gui/golden/) | bash + Python `skimage.metrics.structural_similarity` | **blocked on DEF-004** (no goldens shipped) |

### Slide 3.2 — Why layer 2 isn't wired yet

- Layer 2 is GUI tests that are **Qt-only** — they exercise the canvas, renderers, controls, theme loading. These have no concept of VPR data structures.
- They live in `libs/EXTERNAL/libezgl/test/` and are run from the libezgl repository's own CI.
- They are **deliberately not registered** in VPR's CTest, because (a) they duplicate coverage already provided by VPR layer 3, and (b) the submodule build doesn't always pull the test sources.
- Plan: revisit once libezgl stabilises — either lift the most useful ones into VPR layer 3, or wire the submodule's tests through `add_test()` directly.

### Slide 3.3 — Coverage map

- Show a heatmap or numeric chart: for each VPR GUI subsystem (canvas, draw_rr, draw_noc, search, breakpoint, popovers, status bar), which layer covers it today and which doesn't.
- Highlight the "no layer 5" row in red until DEF-004 is fixed.

---

## 4. Upstream changes (VPR delta vs `ql_main`)

### Slide 4.1 — `wait_for_stage`

- Commit **97709a7ad** (May 2026) — *"draw: add wait_for_stage <stage>_<initial|done> barrier with stage-completion tracking"*.
- New graphics-command verb: `wait_for_stage placement_initial | placement_done | routing_initial | routing_done`.
- Semantics:
  - `*_initial` — resume on first `update_screen()` call at that stage (per-iteration state is available).
  - `*_done` — resume only after `notify_stage_complete()` fires, so the data is settled.
- Implementation: two static sets `initial_stages` / `completed_stages` in [vpr/src/draw/draw.cpp:1401-1451](vpr/src/draw/draw.cpp#L1401-L1451); `notify_stage_complete()` declared in `vpr/src/draw/draw.h`.
- **Why it matters**: lets `--graphics_commands` scripts synchronise with the P&R pipeline deterministically — no more "command fired before the data existed" race.

### Slide 4.2 — Duplication of render calls bug

- Commit **d62d4cdfd** (April 2026) — *"fix doubled text rendering in deffered and rhi renderers"*.
- Root cause: upstream commit `b53c8583b` ("Reapply Feature ap draw") double-invoked `draw_block_pin_util()`, `draw_place(g)`, and `draw_internal_draw_subblk(g)`.
- Invisible under the **immediate** renderer because the second pass fills over the first.
- **Visible** under deferred/RHI because batching reorders fills before overlays — both text passes survive, offset by ~2 px (14 vs 16), producing visible double labels.
- Fix: duplicated calls commented out at [vpr/src/draw/draw.cpp:222-234](vpr/src/draw/draw.cpp#L222-L234) with a note explaining the backend-dependent visibility.
- **Speaker point**: this is a good example of how the new renderers expose latent bugs that immediate-mode masked.

### Slide 4.3 — Graphics-command surface, full delta

List the commands currently supported and call out what's new/changed.

| Command | Status |
|---|---|
| `wait_for_stage <stage>_<initial\|done>` | **new** (97709a7ad) |
| `save_graphics <filename>` | existing |
| `set_macros <state>` | existing |
| `set_nets <0\|1\|2>` | **redefined** as documented states (d6aa9adcc) |
| `set_cpd <bitmask>` | **extended** — bit 0 flylines, bit 1 delay labels, bit 2 routed-wire highlight (d6aa9adcc) |
| `set_routing_util <state>` | existing |
| `set_clip_routing_util <state>` | existing |
| `set_congestion <0\|1\|2>` | existing (0=off, 1=nodes, 2=nodes+nets) |
| `set_draw_block_outlines <state>` | existing |
| `set_draw_block_text <state>` | existing |
| `set_draw_block_internals <state>` | existing |
| `set_draw_net_max_fanout <int>` | existing |
| `exit <int>` | **deferred** to next render checkpoint (19cae2df6) |

Plus supporting infra:
- **caf2004f2** — moved `--graphics_commands` scheduling inside the Qt event loop, so `--disp on` works under `QT_QPA_PLATFORM=offscreen`.
- **55c2fade7** — added `--renderer {immediate|deferred|rhi}` to VPR (stored in `draw_state->renderer_type`).
- **19cae2df6** — `exit N` now flips `pending_graphics_exit` and is honoured at the next render checkpoint, preventing mid-iteration termination. `run_vtr_flow.py` recognises the "Graphics-command 'exit" log prefix to skip downstream consistency checks.
- Several RHI / deferred stability fixes (7a996714e, 621e88f50, 310ecf1d1, ace28de13) for offscreen and headless world-coord init.

**Speaker prompt at end of slide:** *"anything I missed from your branch — let me know now and I'll patch the slide"*. This is the "did I forget anything" hook the user asked for.

---

## 5. Closing

- One-slide recap: where Qt migration is solid (Linux + Qt 6.9.3 + RHI default + 220+ tests), what's left (macOS unblock, win32 first run, Layer 5 goldens, comment coverage).
- Suggested closing visual: a flow showing user → `main.ui` → QtGladeLoader → QMainWindow → renderer (immediate/deferred/RHI) → frame, with the test layers overlaid on the side.

---

## Appendix — sources to cite while preparing slides

Cite these inline on the slides as small URLs / footers, not in the speaker notes:

- [libs/EXTERNAL/libezgl/CMakeLists.txt:25-36](libs/EXTERNAL/libezgl/CMakeLists.txt#L25-L36) — Qt 6.9.3 enforcement.
- [libs/EXTERNAL/libezgl/src/qt/qtgladeloader.cpp:135-161](libs/EXTERNAL/libezgl/src/qt/qtgladeloader.cpp#L135-L161) — supported widget classes / dispatch.
- [libs/EXTERNAL/libezgl/src/qt/rhi_renderer.cpp](libs/EXTERNAL/libezgl/src/qt/rhi_renderer.cpp) — `flush()` vs `flush_mvp_only()`, tile grid, band workers.
- [libs/EXTERNAL/libezgl/src/qt/deferred_renderer.cpp:286-412](libs/EXTERNAL/libezgl/src/qt/deferred_renderer.cpp#L286-L412) — `screen_*_visible()` culling.
- [vpr/src/draw/draw.cpp:1401-1451](vpr/src/draw/draw.cpp#L1401-L1451) — `wait_for_stage` implementation.
- [vpr/src/draw/draw.cpp:222-234](vpr/src/draw/draw.cpp#L222-L234) — double-render fix.
- [vpr/test/gui/](vpr/test/gui/) — layers 1, 3, 4, 5 entry points.
- [doc/src/dev/vpr_gui_complete_test_plan.rst](doc/src/dev/vpr_gui_complete_test_plan.rst) — layered test plan reference.

---

## Open questions before final draft

(Things to confirm with Oleksandr before turning this into slides — left intentionally so the deck doesn't ship stale facts.)

1. Win32 status: do we have a concrete target date / scoped work item, or is it still "intended"?
2. Layer 2 (libezgl unit tests) — do we want to lift them into VPR CTest or keep them separate? The answer drives the slide-3.2 framing.
3. `redraw_geometry` / `on_camera_move` split for the deferred renderer — proposal or already prototyped? If prototyped, point me at the branch so I can include numbers.
4. Anything in `graphic_commands` you remember adding/changing that's missing from the table on slide 4.3?
