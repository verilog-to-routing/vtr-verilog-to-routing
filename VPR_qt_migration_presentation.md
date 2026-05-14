---
marp: true
theme: default
paginate: true
size: 16:9
header: 'VPR Qt Migration'
footer: '2026-05-14 — internal tech talk'
style: |
  section { font-size: 22px; -webkit-print-color-adjust: exact; print-color-adjust: exact; }
  section h1 { font-size: 40px; }
  section h2 { font-size: 32px; }
  section h3 { font-size: 28px; }
  table { font-size: 16px; }
  pre { font-size: 18px; }
  code { font-size: 0.9em; }
  section.lead h1 { font-size: 56px; }
  section.lead p { font-size: 26px; }
  span[style*="color"] { -webkit-print-color-adjust: exact; print-color-adjust: exact; }
---

<!-- _class: lead -->

# VPR GUI: the Qt migration

Internal tech talk
2026-05-14

---

## Agenda

1. **Migration status** — Qt version, platforms, what's left
2. **Renderers** — Immediate / Deferred / RHI
3. **Test framework** — five-layer architecture
4. **Upstream changes** — VPR delta vs `ql_main`

---

<!-- _class: lead -->

# Part 1 — Migration status

---

## Slide 1.1 — Qt 6.9.3, why this version

- **Required version: Qt 6.9.3**.
- **Why we care about a recent Qt**: our default renderer is built on **QRhi**, Qt's modern Rendering Hardware Interface — a portable GPU abstraction over OpenGL / Vulkan / Direct3D / Metal. QRhi has been **part of Qt since 6.7** (it was a private API earlier; promoted to public/stable in 6.7), so we need a Qt new enough to expose it as a supported surface.
- **Reason for the 6.9.3 floor specifically**: Qt 6.8 has a bug in the RHI backend — geometry buffers are not invalidated after the MVP matrix changes, producing rendering artifacts. Fixed in 6.9.3.

---

## Slide 1.2 — Target platforms

Across **every** target platform, Qt is installed the same way: **Qt 6.9.3 via `aqtinstall` to `/path/to/qt6/6.9.3/gcc_64`** (or the equivalent per-platform suffix — `clang_64` on macOS, `mingw_64` on Windows). Same install script, same paths — reproducible across CI and developer hosts.

| Platform | Status | CI |
|---|---|---|
| Linux (Ubuntu 22.04 + 24.04) | **Active** | yes — `.github/workflows/test.yml` |
| macOS | **In Progress** — automatic GUI test framework run manually + quick manual smoke test | currently disabled |
| Windows (win32) | **Planned** | none yet |

Compilers exercised in CI: GCC 11–14, Clang 16–18.

---

## Slide 1.3 — Migration limitations / what's left

Three concrete items still on the work list. One slide each.

---

### 1.3.1 — QtGladeLoader

- We **kept the GTK `.glade` XML** as the single source of truth for the widget tree, and we parse it ourselves at runtime into Qt widgets — instead of porting the .glade files to Qt Designer `.ui` files.
- **Why**: upstream master continues to maintain `main.ui` as a Glade file. Forking to a different XML dialect would force a manual three-way merge every time upstream touches the UI. Parsing the Glade XML lets upstream changes flow in unchanged.

```
   upstream main.ui (Glade XML)
            │
            ▼
   QtGladeLoader::loadFile()
            │
            ▼
       QMainWindow + QWidget tree
```

---

### 1.3.1 — QtGladeLoader (future state)

Once upstream migrates the source-of-truth UI file:

```
   upstream main.ui (Qt Designer XML)
            │
            ▼
       Qt UI loader (built-in)
            │
            ▼
       QMainWindow + QWidget tree
```

Once `main.ui` upstream is itself a Qt Designer file, we drop the custom GTK-XML parser entirely and use Qt's native UI loader — QtGladeLoader becomes dead code we can delete.

---

### 1.3.2 — Code-comment coverage

- The Qt port introduced new modules (RHI backend, deferred renderer, RHI canvas widget, RHI scene renderer) — public-API headers are documented, but many private helpers are not.
- Next step: pass over hot files to bring them to Doxygen-friendly comments.

---

### 1.3.3 — More automated GUI test coverage

- Cross-reference with Part 3. Today we have ~220–250 test cases across 5 layers. *(Coverage numbers and gate thresholds to be filled in from the current `make gui-coverage` run before the deck ships.)*
- Specific gaps:
  - layer-3 widget coverage of recently-added popovers / menu buttons,
  - layer-4 keyboard navigation depth,
  - layer-5 visual-regression cases — grow the golden corpus beyond the current 14 scenes.

---

---

<!-- _class: lead -->

# Part 2 — Renderers

---

## Slide 2.0 — What QRhi actually is

- Qt's **Rendering Hardware Interface**: a thin portable abstraction over GPU APIs.
- Backends QRhi can map to, per platform:
  - **Linux**: OpenGL (GLES2/3), Vulkan
  - **Windows**: Direct3D 11/12, OpenGL, Vulkan
  - **macOS / iOS**: Metal
- We write one path in `rhi_renderer.cpp`; QRhi picks the right backend at runtime.

```
        Our RHI renderer
              │
              ▼
            QRhi
   ┌─────┬───┴───┬─────┐
   ▼     ▼       ▼     ▼
OpenGL Vulkan  D3D11  Metal
```

---

## Slide 2.0.5 — One drawing API, three backends

- All three renderers inherit from **`ezgl::irenderer`** ([irenderer.hpp](libs/EXTERNAL/libezgl/include/ezgl/irenderer.hpp)). Every drawing primitive (`draw_line`, `draw_rectangle`, `fill_rectangle`, `fill_poly`, `draw_text`, `draw_arc`, `draw_surface`, …) is a pure-virtual on that base.
- VPR's draw callback sees only `ezgl::renderer*` — **identical client code regardless of `--renderer immediate|deferred|rhi`**. No `#ifdef`, no branching on backend.
- All differences in the comparison tables on the next slides are **internal** to each backend; swapping renderers is configuration, not a migration.

```
       VPR draw callback
              │
              ▼
     virtual ezgl::irenderer
       (one API surface)
              │
   ┌──────────┼──────────┐
   ▼          ▼          ▼
immediate  deferred     rhi
```

---

## Slide 2.1a — Renderers: resource cost

<style scoped>
table { font-size: 13px; }
</style>

| Aspect | Immediate | Deferred | RHI |
|---|---|---|---|
| Backend | QPainter, synchronous | QPainter, batched | GPU via QRhi |
| Source | `immediate_renderer.cpp` | `deferred_renderer.cpp` | `rhi_renderer.cpp` |
| **CPU usage** | high (one QPainter call per primitive) | medium (collect → flush batches) | <span style="color:#27ae60">low–medium overall;</span> <span style="color:#c0392b">**high\*** during scene composition</span> <span style="color:#27ae60">— steady-state and camera-only frames are very cheap</span> |
| **GPU usage** | none | none | high (intended path) |
| **RAM usage** | <span style="color:#27ae60">minimal — no per-frame storage</span> | <span style="color:#e67e22">scales with scene — style-keyed batch vectors (`QLineF` / `QRectF` / `QPolygonF`) + `std::variant` overlay command queue (`m_overlay_commands`)</span> | <span style="color:#c0392b">scales with scene — CPU-side `SceneBuffers` + QImage overlay cache. Held for the renderer's lifetime (~1.6 GB on 100 M-line scenes).</span> |
| **VRAM usage** | <span style="color:#27ae60">none</span> | <span style="color:#27ae60">none</span> | <span style="color:#c0392b">high — vertex/index buffers × N frame-in-flight slots</span> |
| **Throughput on 100 M-line stress scene** | <span style="color:#c0392b">~0.1 fps</span> | <span style="color:#e67e22">~2 fps</span> | <span style="color:#27ae60">**60+ fps**</span> |

\* RHI CPU spike is at *scene composition* (binning into render tiles, packing `SceneBuffers`); amortised by the multithreaded band workers — see next slide.

---

## Slide 2.1b — Renderers: architecture & optimizations

<style scoped>
table { font-size: 13px; }
</style>

| Aspect | Immediate | Deferred | RHI |
|---|---|---|---|
| **Batching** | none | by `(primitive, color, line-width, dash, cap)` | render-tile-binned (32×32 = 1024 render tiles) + style |
| **CPU multithread optimization** | none | none | **batching workers per band** (`std::thread::hardware_concurrency`) during scene composition |
| **Offscreen-visibility culling** | <span style="color:#e67e22">per-primitive viewport test right before each QPainter call</span> | <span style="color:#e67e22">per-primitive viewport tests (line/rect/poly/arc/text/surface) before batching</span> | <span style="color:#27ae60">render-tile-band culling: only render tiles intersecting the viewport are emitted *(exception: overlay primitives are routed through `m_overlay_deferred` and use the deferred renderer's per-primitive tests instead)*</span> |
| **Overlay (text, arcs, surfaces, screen-space lines)** | none — drawn inline by the same QPainter | none — same QPainter on the same surface (overlay queue is internal, not a separate layer) | **dedicated overlay layer** — owns a `deferred_renderer` instance (`m_overlay_deferred`) that paints into a separate `QImage`, uploaded as a texture and composited on top of the GPU frame via `m_overlay_pso` |

**Terminology:** "render tile" = ezgl's 32×32 scene-space grid — not a GPU device tile (hardware tiled-rasteriser concept). The two are unrelated.

---

## Slide 2.1.1a — Inside the RHI: the scene renderer (1/2)

- The "scene renderer" referenced on slide 2.1 is **`RhiSceneRenderer`** ([rhi_scene_renderer.hpp](libs/EXTERNAL/libezgl/include/ezgl/qt/rhi_scene_renderer.hpp), [rhi_scene_renderer.cpp](libs/EXTERNAL/libezgl/src/qt/rhi_scene_renderer.cpp)). It exists **only in the RHI path** — immediate and deferred renderers don't need an equivalent.
- It is the **GPU half** of the RHI renderer: owns every QRhi object (pipelines, SRBs, vertex/instance/uniform buffers, overlay texture+sampler, per-frame-slot resources).

**Problems it solves (1/2):**

1. **CPU↔GPU separation.** `RhiRenderer` builds POD `SceneBuffers` on the CPU; `RhiSceneRenderer` uploads them and records draw commands on the GPU.
2. **Same GPU code, two callers.** Drives both the on-screen `RhiCanvasWidget` and the offscreen `save_graphics` path — this is how visual-regression PNG export works without an X display.
3. **Frame-in-flight bookkeeping.** Owns `std::vector<FrameResources>` and a per-slot dirty bitmap so geometry re-uploads lazily.

---

## Slide 2.1.1b — Inside the RHI: the scene renderer (2/2)

**Problems it solves (2/2):**

4. **One draw call per `(primitive, style)` group.** `StyleKey` packs `(primitive_type, rgba, line_width, line_dash)` into a `uint64_t`.
5. **GPU-side culling via chunks.** Each style buffer is sliced into world-space chunks; chunks outside `visible_world` are skipped at submit time.
6. **Deterministic GPU teardown.** Explicit `release()` decouples GPU-object destruction from canvas widget lifetime.

**Why deferred doesn't have an equivalent:** its "scene" is just `std::vector<QLineF>` etc., consumed by the same QPainter the renderer was handed — no upload, no frame slots, no offscreen-vs-display split.

---

## Slide 2.2 — `redraw()` split idea (geometry vs camera-move)

- Most user interaction is **pan / zoom** — the geometry of the scene doesn't change, only the camera does.
- Today, RHI already has this split:
  - `flush()` — full path: re-records commands, re-bins into tiles, re-uploads buffers.
  - `flush_mvp_only()` — camera path: pushes a new 64-byte MVP matrix only; geometry buffers are reused, overlay is replayed from cache (`m_overlay_deferred->replay_overlay()`).
  - Selected by `rhi_backend.cpp:27-66`.
- Proposal: **mirror this split in the deferred renderer** — keep the batch cache across camera-only changes, redo only the screen-space projection.

```
   user action ──► redraw_geometry()  ──►  rebuild batches  ──┐
                                                              ▼
   pan/zoom   ──► on_camera_move() ──► update MVP only ───► present
```

---

## Slide 2.3 — How a renderer is chosen

- CLI: `--renderer <immediate|deferred|rhi>`
- Default: **`rhi`** (falls back to immediate if QRhi init fails on the host).
- Plumbed via `ezgl::renderer_type` enum in `render_backend.hpp`, propagated through `QtGladeLoader::setRendererType()` so the `GtkDrawingArea` element is materialised as either `RhiCanvasWidget` or `DrawingAreaWidget`.
- VPR-side: same flag, parsed in `vpr/src/base/read_options.cpp`.

---

## Slide 2.4a — RHI render targets: UI vs headless

<style scoped>
table { font-size: 14px; }
</style>

Two distinct render-target paths in the RHI renderer — both drive the **same** `RhiSceneRenderer` (pipelines, buffers, shaders); they differ only in where the `QRhi*` and the render target come from.

| Aspect | UI mode | Headless mode |
|---|---|---|
| Entry | `RhiCanvasWidget : QRhiWidget` | `RhiCanvasWidget::render_offscreen()` (static) |
| `QRhi` owner | Qt provides; widget owns it | standalone via `create_headless_rhi()` |
| Backend | Qt picks per-platform | explicit: D3D11 (Win), Metal (mac), else OpenGL on `QOffscreenSurface` |
| Render target | swap-chain backbuffer | `QRhiTextureRenderTarget` (RGBA8) |
| Submission | `beginFrame()` / `endFrame()` | `beginOffscreenFrame()` / `endOffscreenFrame()` |
| Output | on-screen pixels | `QImage` via `readBackTexture` |
| Frames-in-flight | 2 or 3 | 1 |
| Works under `QT_QPA_PLATFORM=offscreen`? | **No** | **Yes** |

---

## Slide 2.4b — Driving constraint, with proof

`QRhiWidget` needs a real GPU context for its swap chain. Qt's offscreen platform plugin (`QT_QPA_PLATFORM=offscreen`) gives only software surfaces and does **not** provide `QRhiWidget` a usable GPU context — confirmed by three independent sources in this tree:

1. **Source comment in the headless path** ([rhi_canvas_widget.cpp:236](libs/EXTERNAL/libezgl/src/qt/rhi_canvas_widget.cpp#L236)):
   *"No QRhiWidget is involved so this works on `QT_QPA_PLATFORM=offscreen`."*

2. **Layer-3 tests that touch the widget path skip under offscreen** ([test_save_graphics.cpp:159, 185, 229](vpr/test/gui/test_save_graphics.cpp#L159)):
   `SKIP("CI offscreen Mesa lacks a working GL context for QRhiGles2")`

3. **Layer-5 visual-regression driver acknowledges the divergence** ([run_visual_regression.sh:44](vpr/test/gui/run_visual_regression.sh#L44)):
   `SKIP: CI offscreen Mesa cannot be compared against developer-host goldens`

In short: the standalone-`QRhi`-on-`QOffscreenSurface` route is the only RHI path that runs under offscreen Qt — by design, and confirmed by our own test infrastructure.

---

## Slide 2.5a — Layer 5 caveat: what it can and can't catch

Consequence of the asymmetry on slide 2.4 (cf. Part 3, Layer 5):

- Layer 5 runs under `QT_QPA_PLATFORM=offscreen` → exercises the **headless path** (standalone QRhi → texture render target → readback).
- It does **not** exercise the **UI path** (QRhiWidget → swap chain).

**Bug classes Layer 5 cannot catch** — they live entirely on the UI side:

- Swap-chain framing (multi-slot frame-in-flight timing, presentation glitches).
- `QRhiWidget` resize handling — `releaseResources()` / `initialize()` cycle. (We hit a real instance of this in this migration.)
- Render-pass-descriptor mismatches that only show up with a swap-chain render target.
- Backend-selection asymmetries between Qt's widget-integrated `QRhi` and our explicit `create_headless_rhi()`.

---

## Slide 2.5b — Mitigation: manual UI re-run of Layer 5

Until Qt's offscreen plugin gains GPU-context support (or we add a CI rig with a real / `LLVMpipe`-virtualised GPU surface), the cheap and immediate mitigation:

1. On a developer host with a working display, rerun each Layer-5 scenario interactively — no `--disp off`, no `QT_QPA_PLATFORM=offscreen`.
2. Drive scenes from [vpr/test/gui/visual_cases.sh](vpr/test/gui/visual_cases.sh) by hand or via a script that flips `--disp on`.
3. Compare visually against headless goldens in [vpr/test/gui/golden/](vpr/test/gui/golden/); substantive differences indicate a UI-only regression to file.

This is **gap coverage, not duplication** — manual UI runs catch bugs the automated Layer 5 structurally cannot.

**Recommended cadence:** before release tags and after any change to `RhiCanvasWidget`, `rhi_backend`, or a QRhi version bump.

---

<!-- _class: lead -->

# Part 3 — GUI test framework

---

## Slide 3.1 — Five-layer architecture

<style scoped>
table { font-size: 14px; }
</style>

Layers go from cheapest/fastest (top) to slowest/highest fidelity (bottom).

| Layer | Kind | ~Cases | Where | Mechanism | Status |
|---|---|---|---|---|---|
| 1 | Headless smoke (CLI + `--graphics_commands`) | ~24 | [vpr/test/gui/run_headless_smoke.sh](vpr/test/gui/run_headless_smoke.sh) | bash + VPR CLI under `QT_QPA_PLATFORM=offscreen` | active |
| 2 | libezgl unit (Qt-only) | — | inside the libezgl submodule | Qt Test / Catch2 | **runs in the libezgl repo, not wired into VPR CTest yet** |
| 3 | Catch2 unit/integration (widget tree, draw state, renderer plumbing) | ~156 | `vpr/test/gui/test_*.cpp` (22 files) | Catch2 + fixtures (`EzglAppFixture`, `VprRunStageFixture`, `DrawStructsScope`) | active |
| 4 | Interactive (mouse / keyboard via `QTest`) | ~30–40 | same files, tagged `[layer4]` | Catch2 + `QTest::mouseMove`/`keyClick` | active (subset of L3 binary) |
| 5 | Visual regression (PNG + SSIM) | 14 cases | [vpr/test/gui/run_visual_regression.sh](vpr/test/gui/run_visual_regression.sh), goldens in [vpr/test/gui/golden/](vpr/test/gui/golden/) | bash + Python `skimage.metrics.structural_similarity` | active (14 goldens shipped; corpus to be expanded) |

---

## Slide 3.2 — Why layer 2 isn't wired yet

- Layer 2 is GUI tests that are **Qt-only** — they exercise the canvas, renderers, controls, theme loading. These have no concept of VPR data structures.
- They live in `libs/EXTERNAL/libezgl/test/` and are run from the libezgl repository's own CI.
- They are **deliberately not registered** in VPR's CTest, because:
  - they duplicate coverage already provided by VPR layer 3, and
  - the submodule build doesn't always pull the test sources.
- Plan: revisit once libezgl stabilises — either lift the most useful ones into VPR layer 3, or wire the submodule's tests through `add_test()` directly.

---

## Slide 3.3 — Coverage map

- Heatmap or numeric chart: for each VPR GUI subsystem (canvas, draw_rr, draw_noc, search, breakpoint, popovers, status bar), which layer covers it today and which doesn't.
- Layer 5 is active with 14 golden scenes shipped; mark cells green for covered subsystems and amber where coverage is shallow.

---

<!-- _class: lead -->

# Part 4 — Upstream changes

VPR delta vs `ql_main`

---

## Slide 4.1 — `wait_for_stage`

- Commit **97709a7ad** (May 2026) — *"draw: add wait_for_stage <stage>_<initial|done> barrier with stage-completion tracking"*.
- New graphics-command verb: `wait_for_stage placement_initial | placement_done | routing_initial | routing_done`.
- Semantics:
  - `*_initial` — resume on first `update_screen()` call at that stage (per-iteration state is available).
  - `*_done` — resume only after `notify_stage_complete()` fires, so the data is settled.
- Implementation: two static sets `initial_stages` / `completed_stages` in [vpr/src/draw/draw.cpp:1401-1451](vpr/src/draw/draw.cpp#L1401-L1451); `notify_stage_complete()` declared in `vpr/src/draw/draw.h`.
- **Why it matters**: lets `--graphics_commands` scripts synchronise with the P&R pipeline deterministically — no more "command fired before the data existed" race.

---

## Slide 4.2 — Duplication of render calls bug

- Commit **d62d4cdfd** (April 2026) — *"fix doubled text rendering in deffered and rhi renderers"*.
- Root cause: upstream commit `b53c8583b` ("Reapply Feature ap draw") double-invoked `draw_block_pin_util()`, `draw_place(g)`, and `draw_internal_draw_subblk(g)`.
- Invisible under the **immediate** renderer because the second pass fills over the first.
- **Visible** under deferred/RHI because batching reorders fills before overlays — both text passes survive, offset by ~2 px (14 vs 16), producing visible double labels.
- Fix: duplicated calls commented out at [vpr/src/draw/draw.cpp:222-234](vpr/src/draw/draw.cpp#L222-L234) with a note explaining the backend-dependent visibility.

---

## Slide 4.3a — Graphics-command delta: new + small changes

<style scoped>
table { font-size: 14px; }
</style>

Side-by-side: upstream `ql_main` vs our `qt_layer`. Rows with sub-lists (`set_cpd`) get their own slide next.

| Command | Upstream (`ql_main`) | qt_layer |
|---|---|---|
| `wait_for_stage <stage>_<initial\|done>` | — | <span style="color:#27ae60">**new** — barrier; resume on stage init or stage completion (97709a7ad). `stage` ∈ {`placement`, `routing`}</span> |
| `set_nets <state>` | raw cast `(e_draw_nets)atoi(cmd[1])` — **no documented values** | <span style="color:#e67e22">same parser, **documented states** (d6aa9adcc): `0`=off, `1`=flylines, `2`=routed</span> |
| `set_congestion <state>` | raw cast to `e_draw_congestion`, no doc | <span style="color:#e67e22">**documented states**: `0`=off, `1`=congested, `2`=congested+nets; **range-asserted** 0..2</span> |
| `exit <int>` | `exit(atoi(cmd[1]))` — **immediate** process termination mid-iteration | <span style="color:#e67e22">**deferred** — sets `pending_graphics_exit`; honoured at next render checkpoint (19cae2df6)</span> |

**Legend:** green = new command, orange = changed semantics.

---

## Slide 4.3b — Graphics-command delta: `set_cpd <state>`

<style scoped>
table { font-size: 14px; }
</style>

| | |
|---|---|
| **Upstream (`ql_main`)** | hard-codes `show_crit_path = true; show_crit_path_flylines = true;` and only `show_crit_path_delays` is taken from the argument. The comment claims `(bool)(bool)(bool)` but the code reads one int. |
| **qt_layer** | <span style="color:#e67e22">**redefined as bitmask** (d6aa9adcc)</span> |

**qt_layer bitmask values:**

| Value | Meaning |
|---|---|
| `0` | off |
| `1` | flylines only |
| `3` | flylines + delay labels |
| `4` | routed wires only |
| `5` | flylines + routed wires |
| `7` | all on |

---

## Slide 4.3c — Graphics-command delta: unchanged commands

These commands ship identically on `ql_main` and `qt_layer`:

| Command | What it does (both branches) |
|---|---|
| `save_graphics <filename>` | save current frame to file |
| `set_macros <state>` | sets `show_placement_macros` from raw integer |
| `set_routing_util <state>` | raw cast to `e_draw_routing_util` |
| `set_clip_routing_util <state>` | bool |
| `set_draw_block_outlines <state>` | int |
| `set_draw_block_text <state>` | int |
| `set_draw_block_internals <state>` | int |
| `set_draw_net_max_fanout <int>` | int |

---

## Slide 4.4 — Supporting infra changes

Not visible as command rows, but shipped on this branch:

- **caf2004f2** — `--graphics_commands` now scheduled **inside the Qt event loop** so `--disp on` works under `QT_QPA_PLATFORM=offscreen` (this is what made layer 1 / layer 5 tests possible).
- **55c2fade7** — new `--renderer {immediate|deferred|rhi}` CLI option, stored in `draw_state->renderer_type`.
- Several RHI/deferred stability fixes for offscreen/headless world-coord init (7a996714e, 621e88f50, 310ecf1d1, ace28de13).

---

<!-- _class: lead -->

# Closing

---

## Recap

**Solid:**
- Linux + Qt 6.9.3 + RHI as default
- 220+ test cases across 5 layers
- Graphics-command surface stable, documented, with a `wait_for_stage` barrier

**Still to do:**
- macOS CI unblock
- win32 first run (MinGW/MSYS2)
- Grow layer-5 golden corpus
- Code-comment coverage pass

---

<!-- _class: lead -->

# Questions?
