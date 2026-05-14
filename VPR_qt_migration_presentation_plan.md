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

- The Qt port introduced new modules (RHI backend, deferred renderer, RHI canvas widget, RHI scene renderer) — public-API headers are documented, but many private helpers are not.
- Next step: pass over hot files to bring them to Doxygen-friendly comments.

#### 1.3.3 — More automated GUI test coverage

- Cross-reference with section 3. Today we have ~220–250 test cases across 5 layers. *(Coverage numbers and gate thresholds to be filled in from the current `make gui-coverage` run before the deck ships — do not pull from the test-plan doc, it is stale.)*
- Specific gaps to call out on the slide:
  - layer-3 widget coverage of recently-added popovers / menu buttons,
  - layer-4 keyboard navigation depth,
  - layer-5 visual-regression cases — grow the golden corpus beyond the current 14 scenes.

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

### Slide 2.0.5 — One drawing API, three backends

- All three renderers inherit from a single abstract base, **`ezgl::irenderer`** ([include/ezgl/irenderer.hpp](libs/EXTERNAL/libezgl/include/ezgl/irenderer.hpp)). Every drawing primitive — `draw_line`, `draw_rectangle`, `fill_rectangle`, `fill_poly`, `fill_triangle`, `draw_arc` / `fill_arc`, `draw_text`, `draw_surface`, `fill_arrow_pointer_triangle`, etc. — is a pure-virtual on that base.
- Application code (the VPR draw callback in `vpr/src/draw/`) sees only `ezgl::renderer*` and calls those primitives. **The code is identical regardless of `--renderer immediate|deferred|rhi`** — no `#ifdef`, no branching on backend, no compile-time variation in user code.
- The choice of renderer is a runtime decision made once during canvas setup (`QtGladeLoader::setRendererType()` in slide 2.3); from the application's point of view it's invisible.
- **Implication for the comparison table on the next slide**: every difference between the three columns is *internal* (CPU/GPU paths, batching, multithreading, culling strategy). None of them require client-code changes. Swapping renderers is configuration, not migration.

```
        VPR draw callback
                │
                ▼
       virtual ezgl::irenderer
       (one API surface)
                │
   ┌────────────┼────────────┐
   ▼            ▼            ▼
immediate    deferred       rhi
```

### Slide 2.1 — Three renderers, side by side

This is the core comparison slide. Render as one table on the slide.

| Aspect | Immediate | Deferred | RHI |
|---|---|---|---|
| Backend | QPainter, synchronous | QPainter, batched | GPU via QRhi |
| Source | [immediate_renderer.cpp](libs/EXTERNAL/libezgl/src/qt/immediate_renderer.cpp) | [deferred_renderer.cpp](libs/EXTERNAL/libezgl/src/qt/deferred_renderer.cpp) | [rhi_renderer.cpp](libs/EXTERNAL/libezgl/src/qt/rhi_renderer.cpp) |
| **CPU usage** | high (one QPainter call per primitive) | medium (collect → flush batches) | <span style="color:#27ae60">low–medium overall,</span> <span style="color:#c0392b">**high\*** during scene composition (binning into render tiles, building `SceneBuffers`);</span> <span style="color:#27ae60">steady-state and camera-only frames are very cheap</span> |
| **GPU usage** | none | none | high (intended path) |
| **RAM usage** | <span style="color:#27ae60">minimal (no per-frame storage; just QPainter state)</span> | <span style="color:#e67e22">scales with scene — two structures held until flush: (1) style-keyed batch vectors of `QLineF` / `QRectF` / `QPolygonF` for batchable primitives, (2) a `std::variant` command queue (`m_overlay_commands`) for text, arcs, polys, surfaces, screen-space arrows</span> | <span style="color:#c0392b">scales with scene — CPU-side `SceneBuffers` (POD geometry per style, chunked) + QImage overlay cache. Held for the renderer's lifetime (~1.6 GB on 100 M-line scenes — same size as one GPU slot's buffers; VRAM holds N copies, so VRAM total = N × this).</span> |
| **VRAM usage** | <span style="color:#27ae60">none</span> | <span style="color:#27ae60">none</span> | <span style="color:#c0392b">high (vertex/index buffers per render tile)</span> |
| **Batching** | none | by `(primitive, color, line-width, dash, cap)` | render-tile-binned (32×32 = 1024 render tiles) + style |
| **CPU multithread optimization** | none | none | **batching workers per band** (`std::thread::hardware_concurrency`) during scene composition |
| **Offscreen-visibility culling** | <span style="color:#e67e22">per-primitive viewport test right before each QPainter call</span> | <span style="color:#e67e22">per-primitive viewport tests (line/rect/poly/arc/text/surface) before batching</span> | <span style="color:#27ae60">render-tile-band culling: only render tiles intersecting the viewport are emitted *(exception: overlay primitives — text, arcs, surfaces, screen-space lines — are routed through `m_overlay_deferred`, so they use the deferred renderer's per-primitive viewport tests instead)*</span> |
| **Overlay** | none | none | **dedicated overlay layer** for text, arcs, surfaces, screen-space lines — owns a `deferred_renderer` instance (`m_overlay_deferred`) that paints these primitives into a separate `QImage`, which is uploaded as a texture and composited on top of the GPU frame via `m_overlay_pso` |
| **Approx. throughput on 100 M-line stress scene** | <span style="color:#c0392b">~0.1 fps</span> | <span style="color:#e67e22">~2 fps</span> | <span style="color:#27ae60">**60+ fps**</span> |

Notes for the speaker:
- **Terminology note**: throughout this slide, "render tile" refers to ezgl's own scene-space partitioning (a 32×32 grid over the visible world) — *not* a GPU device tile (tiled-rasteriser hardware concept). The two are unrelated.
- **"flush" footnote** (deferred/RHI): `flush()` = `replay()` + `reset()`. `replay()` walks the batches and overlay command queue and issues the actual QPainter / GPU draw calls — *this is the moment pixels are written*. `reset()` empties the accumulators so the next frame starts clean. Reset is automatic at end of flush; it is **not** triggered by draw-state changes — those just cause a redraw, and reset is part of that redraw's flush.
- **\* on RHI CPU usage**: the spike is at *scene composition* — when the user (or the P&R pipeline) hands us a new scene, the CPU bins all primitives by style/render tile and packs them into `SceneBuffers`. Once that's done, redraws (and especially camera-only frames via `flush_mvp_only()`) reuse the cached buffers and the CPU goes back to near-idle. This is what the multithreaded band workers were added to amortise.
- **Why RHI has an overlay layer at all**: text rasterisation (font hinting, shaping, antialiasing) and complex curves are exactly the things GPUs are *bad* at compared to QPainter. Instead of porting all that to shaders, RHI builds a separate transparent `QImage` once per camera using QPainter (via an internally-owned `deferred_renderer` for batching efficiency) and composites that image on top of the GPU frame. Deferred has no equivalent because it already runs entirely on QPainter — there's nothing to "lift" onto a different surface.

**Suggested visual on this slide:** small bar chart of fps from the stress-bench results, with one bar per renderer.

### Slide 2.1.1 — Inside the RHI: the scene renderer

- The "scene renderer" referenced on slide 2.1 is **`RhiSceneRenderer`** ([rhi_scene_renderer.hpp](libs/EXTERNAL/libezgl/include/ezgl/qt/rhi_scene_renderer.hpp), [rhi_scene_renderer.cpp](libs/EXTERNAL/libezgl/src/qt/rhi_scene_renderer.cpp)). It exists **only in the RHI path** — immediate and deferred renderers don't need an equivalent.
- It is the **GPU half** of the RHI renderer: owns every QRhi object (pipelines, SRBs, vertex/instance/uniform buffers, overlay texture+sampler, per-frame-slot resources).

**Problems it solves, in one line each:**

1. **CPU↔GPU separation.** `RhiRenderer` builds POD `SceneBuffers` on the CPU; `RhiSceneRenderer` uploads them and records draw commands on the GPU. Keeping these in two classes means each side can change without re-validating the other.
2. **Same GPU code, two callers.** Drives both the on-screen `RhiCanvasWidget` and the offscreen `save_graphics` path (own QRhi on a `QOffscreenSurface`) — this is how visual-regression PNG export works without an X display.
3. **Frame-in-flight bookkeeping.** Owns a `std::vector<FrameResources>` and a per-slot dirty bitmap (`m_frame_slot_geom_valid`) so geometry re-uploads lazily, one slot at a time, instead of stalling the pipeline on every scene change.
4. **One draw call per `(primitive, style)` group.** The `StyleKey` packs `(primitive_type, rgba, line_width, line_dash)` into a `uint64_t`; all geometry sharing a key lands in one vertex buffer and one draw call — a few hundred draws cover millions of primitives.
5. **GPU-side culling via chunks.** Each style buffer is sliced into world-space chunks with bounding rectangles; chunks outside `visible_world` are skipped at submit time — same data uploaded once, only the visible slice drawn.
6. **Deterministic GPU teardown.** Explicit `release()` decouples GPU-object destruction from canvas widget lifetime — important under QRhi where order-of-destruction relative to the `QRhi*` matters.

**Why deferred doesn't have an equivalent:** its "scene" is just `std::vector<QLineF>` etc., consumed by the same QPainter that the renderer was handed — no upload, no frame slots, no offscreen-vs-display split. Splitting it into two classes would buy nothing.

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

### Slide 2.4 — RHI render targets: UI vs headless

The RHI renderer has **two distinct render-target paths**, governed by where the `QRhi` instance and its surface come from:

| Aspect | UI mode | Headless mode |
|---|---|---|
| Entry point | `RhiCanvasWidget` (extends `QRhiWidget`) | `RhiCanvasWidget::render_offscreen()` (static) |
| `QRhi` instance | provided by Qt; widget owns it | constructed standalone via `create_headless_rhi()` ([rhi_canvas_widget.cpp:164-211](libs/EXTERNAL/libezgl/src/qt/rhi_canvas_widget.cpp#L164-L211)) |
| Backend selection | Qt picks per-platform | explicit per-platform: D3D11 (Win), Metal (mac/iOS), else OpenGL on a `QOffscreenSurface` |
| Render target | widget's **swap-chain backbuffer** | **`QRhiTextureRenderTarget`** over an RGBA8 `QRhiTexture` ([rhi_canvas_widget.cpp:244-254](libs/EXTERNAL/libezgl/src/qt/rhi_canvas_widget.cpp#L244-L254)) |
| Frame submission | `beginFrame()` / `endFrame()` driven by Qt paint loop | `beginOffscreenFrame()` / `endOffscreenFrame()` |
| Output | on-screen pixels | `QImage` via `readBackTexture` ([rhi_canvas_widget.cpp:268-272](libs/EXTERNAL/libezgl/src/qt/rhi_canvas_widget.cpp#L268-L272)) |
| Frames-in-flight | 2 or 3 (per backend) | 1 |
| Qt platform plugin requirement | needs a GUI plugin (xcb/wayland/cocoa/windows) with **a working GPU context** | works under any plugin, including `QT_QPA_PLATFORM=offscreen` |

Both paths drive the **same** `RhiSceneRenderer` (the pipeline/buffer/shader code) — the only difference is who owns the `QRhi*` and the render target.

#### The driving constraint, with proof

`QRhiWidget` requires a real GPU context for its swap chain. Qt's **offscreen platform plugin** (`QT_QPA_PLATFORM=offscreen`) provides only software surfaces and **does not give `QRhiWidget` a usable GPU context** under our CI environment. Three independent sources of evidence in this very tree:

1. **Source comment in the headless path** ([rhi_canvas_widget.cpp:236](libs/EXTERNAL/libezgl/src/qt/rhi_canvas_widget.cpp#L236)):
   > *"No QRhiWidget is involved so this works on `QT_QPA_PLATFORM=offscreen`."*
2. **Layer-3 tests that touch the widget path skip under offscreen** ([test_save_graphics.cpp:159, 185, 229](vpr/test/gui/test_save_graphics.cpp#L159)):
   > `SKIP("CI offscreen Mesa lacks a working GL context for QRhiGles2")`
3. **Layer-5 visual regression driver acknowledges the divergence** ([run_visual_regression.sh:44](vpr/test/gui/run_visual_regression.sh#L44)):
   > `SKIP: CI offscreen Mesa cannot be compared against developer-host goldens`

In short: the standalone-`QRhi`-on-`QOffscreenSurface` route is the only RHI path that runs under offscreen Qt — by design, and confirmed by our own test infrastructure.

### Slide 2.5 — Layer 5 caveat and manual-GUI mitigation

Concrete consequence of the asymmetry on slide 2.4 for the test framework (cf. Slide 3.1, Layer 5):

- Layer 5 visual regression **runs under `QT_QPA_PLATFORM=offscreen`** and goes through `flush_capture()` → `RhiCanvasWidget::render_offscreen()` → headless path.
- It therefore exercises the standalone-`QRhi` + texture-render-target pipeline.
- It does **not** exercise the `QRhiWidget` swap-chain pipeline.

Bug classes Layer 5 cannot catch, because they live entirely on the UI path:

- Swap-chain framing issues (multi-slot frame-in-flight timing, presentation glitches, vsync interactions).
- `QRhiWidget` resize handling — the `releaseResources()` / `initialize()` cycle Qt fires on surface re-create. (We hit a concrete instance of this earlier in the migration.)
- Render-pass-descriptor mismatches that only appear with a swap-chain render target.
- Backend-selection asymmetries in Qt's widget-integrated `QRhi` choice vs our explicit `create_headless_rhi()` selection.

#### Suggested mitigation: manual UI re-run of Layer 5

Until either (a) Qt's offscreen plugin gains usable GPU-context support, or (b) we set up a CI rig with a real or virtualised (e.g. `LLVMpipe`) GPU surface, the gap is real but bounded. The cheap and immediate mitigation:

1. On a developer host with a working display, **rerun each Layer-5 scenario interactively** — no `--disp off`, no `QT_QPA_PLATFORM=offscreen`.
2. Drive the scenes from [vpr/test/gui/visual_cases.sh](vpr/test/gui/visual_cases.sh) by hand (or via a small script that toggles `--disp on`).
3. Compare visually against the headless goldens in [vpr/test/gui/golden/](vpr/test/gui/golden/). Substantive differences indicate a UI-only regression to investigate and file.

This is **gap coverage, not duplication** — these manual runs catch a class of bugs the automated Layer 5 structurally cannot. Worth doing before each release tag and after any change to `RhiCanvasWidget`, `rhi_backend`, or QRhi-version bumps.

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
| 5 | Visual regression (PNG + SSIM) | 14 cases | [vpr/test/gui/run_visual_regression.sh](vpr/test/gui/run_visual_regression.sh), goldens in [vpr/test/gui/golden/](vpr/test/gui/golden/) | bash + Python `skimage.metrics.structural_similarity` | active (14 goldens shipped; corpus to be expanded) |

### Slide 3.2 — Why layer 2 isn't wired yet

- Layer 2 is GUI tests that are **Qt-only** — they exercise the canvas, renderers, controls, theme loading. These have no concept of VPR data structures.
- They live in `libs/EXTERNAL/libezgl/test/` and are run from the libezgl repository's own CI.
- They are **deliberately not registered** in VPR's CTest, because (a) they duplicate coverage already provided by VPR layer 3, and (b) the submodule build doesn't always pull the test sources.
- Plan: revisit once libezgl stabilises — either lift the most useful ones into VPR layer 3, or wire the submodule's tests through `add_test()` directly.

### Slide 3.3 — Coverage map

- Show a heatmap or numeric chart: for each VPR GUI subsystem (canvas, draw_rr, draw_noc, search, breakpoint, popovers, status bar), which layer covers it today and which doesn't.
- Layer 5 is active with 14 golden scenes shipped; mark cells green for covered subsystems and amber where coverage is shallow.

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

### Slide 4.3 — Graphics-command surface, full delta

Side-by-side: upstream `ql_main` vs our `qt_layer`. Rows that changed are flagged in the right column.

| Command | Upstream (`ql_main`) | qt_layer |
|---|---|---|
| `wait_for_stage <stage>_<initial\|done>` | — | <span style="color:#27ae60">**new** — barrier; resume on stage init or stage completion (97709a7ad)<br/><br/>where `stage` ∈ {`placement`, `routing`}</span> |
| `save_graphics <filename>` | save current frame to file | unchanged |
| `set_macros <state>` | sets `show_placement_macros` from raw integer | unchanged |
| `set_nets <state>` | raw cast `(e_draw_nets)atoi(cmd[1])` — **no documented values**, error message just says "Expect net draw state" | <span style="color:#e67e22">same parser, but **documented states**; error message lists them (d6aa9adcc)<br/><br/>`0`=off<br/>`1`=flylines<br/>`2`=routed</span> |
| `set_cpd <state>` | hard-codes `show_crit_path = true; show_crit_path_flylines = true;` and only `show_crit_path_delays` is taken from the argument — comment claims "(bool)(bool)(bool)" but code reads one int | <span style="color:#e67e22">**redefined as bitmask** (d6aa9adcc)<br/><br/>`0`=off<br/>`1`=flylines only<br/>`3`=flylines + delay labels<br/>`4`=routed wires only (routing stage)<br/>`5`=flylines + routed wires<br/>`7`=flylines + delay labels + routed wires (all on)</span> |
| `set_routing_util <state>` | raw cast to `e_draw_routing_util` | unchanged |
| `set_clip_routing_util <state>` | bool | unchanged |
| `set_congestion <state>` | raw cast to `e_draw_congestion`, no doc | <span style="color:#e67e22">**documented states**: 0=off, 1=congested, 2=congested+nets; **range-asserted** 0..2</span> |
| `set_draw_block_outlines <state>` | int | unchanged |
| `set_draw_block_text <state>` | int | unchanged |
| `set_draw_block_internals <state>` | int | unchanged |
| `set_draw_net_max_fanout <int>` | int | unchanged |
| `exit <int>` | `exit(atoi(cmd[1]))` — **immediate** process termination mid-iteration | <span style="color:#e67e22">**deferred** — sets `pending_graphics_exit` flag; honoured at next render checkpoint so the current frame finishes cleanly (19cae2df6)</span> |

**Color legend:** green = new command, orange = changed semantics.

Supporting infra changes that aren't visible as command rows (cover in speaker notes or as a separate slide):

- **caf2004f2** — `--graphics_commands` now scheduled **inside the Qt event loop** so `--disp on` works under `QT_QPA_PLATFORM=offscreen` (this is what made layer 1 / layer 5 tests possible).
- **55c2fade7** — new `--renderer {immediate|deferred|rhi}` CLI option, stored in `draw_state->renderer_type`.
- Several RHI/deferred stability fixes for offscreen/headless world-coord init (7a996714e, 621e88f50, 310ecf1d1, ace28de13).

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
5. **Current coverage numbers** for slide 1.3.3 — what does `make gui-coverage` report today on Linux and macOS? (The test-plan RST is stale; please paste fresh numbers from a recent run.)
