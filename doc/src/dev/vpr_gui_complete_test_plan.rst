.. _vpr_gui_test_plan:

VPR GUI Qt Migration — Test Plan
=================================

.. contents:: Table of Contents
   :local:
   :depth: 2

:Document owner: QuickLogic — EDA Tools Team
:Applies to:     VPR Qt GUI subsystem (``vpr/src/draw/``,
                 ``libs/EXTERNAL/libezgl``)
:Companion:      :ref:`vpr_gui_defect_issues_log`


1. Purpose and Scope
--------------------

This document describes the test plan for the VPR Qt GUI (the migration
of the legacy GTK3/Cairo graphics subsystem to Qt 6). It defines:

* what the GUI test suite covers,
* how the suite is structured and selected,
* the developer-facing entry points (``make`` / ``ctest`` /
  ``run_reg_test.py``),
* the environment and prerequisites,
* the coverage targets and quality gates,
* the entry/exit criteria for releases and pull requests, and
* the defect-tracking policy that governs every test failure.

In scope:

* All code under ``vpr/src/draw/`` and ``vpr/src/base/read_options.cpp``
  (graphics-related options).
* The Qt-only paths in ``libs/EXTERNAL/libezgl`` (canvas, renderers,
  ``qtgladeloader``, ``ezgl_qtcompat``).
* The ``--graphics_commands`` and ``--disp`` CLI surfaces.
* Visual artefacts produced by ``save_graphics``.

Out of scope:

* Non-graphics VPR functionality (covered by ``test_vpr``,
  ``vtr_reg_basic``, ``vtr_reg_strong``).
* Upstream Qt or platform-toolkit defects.


2. References
-------------

* :ref:`vpr_gui_defect_issues_log` — canonical defect log.
* ``vpr/test/gui/README.rst`` — operator instructions for the test suite.
* ``vpr/test/gui/CMakeLists.txt`` — authoritative test registration.
* ``CONTRIBUTING.md`` — repository contribution rules.


3. Test Environment and Prerequisites
-------------------------------------

3.1 Supported platforms
~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :widths: 30 70
   :header-rows: 1

   * - Platform
     - Status
   * - Ubuntu 24.04 (x86_64)
     - Primary; runs in CI on every PR.
   * - macOS 14 (arm64)
     - Primary; runs in CI on every PR (Layer 5 currently
       allowed-failure — see DEF-010).

3.2 Required toolchain
~~~~~~~~~~~~~~~~~~~~~~

* Qt 6 (``Core``, ``Gui``, ``Widgets``, ``Xml``, ``Svg``, ``Test``,
  ``ShaderTools``).
* CMake ≥ 3.16, a C++20 compiler.
* Catch2 (vendored under ``libs/EXTERNAL``).
* ``gcovr`` (coverage reporting); ``llvm`` for ``llvm-cov gcov`` on
  macOS.
* GNU ``time`` (``gtime`` from the ``gnu-time`` Homebrew package on
  macOS; ``time`` in coreutils on Linux).
* Python 3.9+ with the project ``requirements.txt`` plus
  ``scikit-image`` and ``Pillow`` (Layer 5 SSIM compare).

3.3 Single-command provisioning
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Both helper scripts leave a developer machine ready to build and run the
full GUI suite without any further manual installation:

* Linux: ``./install_apt_packages.sh``
* macOS: ``./install_brew_packages.sh``

Each script also creates a project-local virtualenv at ``${repo}/.venv``
containing every Python dependency the test suite requires. Activate it
with ``source .venv/bin/activate`` before invoking the scripted flows.

3.4 Linux without sudo (shared lab machines)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

On hosts where the developer does not have ``sudo`` (typical for shared
lab machines), the same Linux script is parameterised so the operator
can install Qt under their home directory instead of ``/opt/qt6``:

.. code-block:: console

   # The system packages listed in install_apt_packages.sh must already
   # be present (ask an admin once, or verify they were installed by the
   # base image). Then:
   VTR_SKIP_APT=1 VTR_QT_PREFIX="$HOME/software-pkgs/qt6" \
       ./install_apt_packages.sh

This installs Qt 6.9.3 to ``$HOME/software-pkgs/qt6/6.9.3/gcc_64`` and
creates the same project-local ``.venv``. The script is idempotent —
re-running it on a populated tree is a no-op.

The Qt tree is relocatable. When an admin later moves it to a shared
location (for example ``/opt/qt6`` or ``/opt/vtr-tools/qt6``), simply
set ``VTR_QT_PREFIX`` to the new root in any new shell — no rebuild of
Qt is required.

After provisioning, every new shell needs the toolchain on ``PATH``
before invoking ``cmake`` / ``make`` / ``ctest``:

.. code-block:: bash

   export QT6_HOME="$HOME/software-pkgs/qt6/6.9.3/gcc_64"   # or /opt/qt6/...
   export PATH="$QT6_HOME/bin:$PATH"
   export LD_LIBRARY_PATH="$QT6_HOME/lib:${LD_LIBRARY_PATH:-}"
   export CMAKE_PREFIX_PATH="$QT6_HOME:${CMAKE_PREFIX_PATH:-}"
   export QT_PLUGIN_PATH="$QT6_HOME/plugins"
   source <repo>/.venv/bin/activate

Save these lines in a personal helper (e.g. ``$HOME/software-pkgs/env.sh``)
and ``source`` it at the start of each session.


4. Test Philosophy
------------------

The suite is governed by a strict bug-discovery discipline. The tests
are the customer of the product code, not the other way around.

#. **A failing test is evidence of a defect, not a test problem.** Every
   failing case is investigated and recorded as a ``DEF-NNN`` entry in
   the companion defect log before any code change.
#. **No silent skips, ``xfail``\ s, or ``WILL_FAIL`` markers.** Every
   suppressed test must cite a tracked ``DEF-NNN`` (and the
   corresponding GitHub issue) inline.
#. **No "fix the test" commits.** A change that makes a previously
   failing test pass is only acceptable when paired with a product-code
   commit and a closed defect entry.
#. **Goldens are evidence, not authority.** When a Layer 5 SSIM compare
   fails, the first hypothesis is "the renderer regressed". Goldens are
   regenerated only after a defect is opened, triaged, and explicitly
   closed as *expected visual change*.


5. Test Architecture
--------------------

The suite is organised into four labelled layers. Each test carries the
CTest label set ``gui;<kind>;<layerN>`` so future tests join the
aggregate automatically.

.. list-table::
   :widths: 8 18 14 60
   :header-rows: 1

   * - Layer
     - Kind
     - Mechanism
     - What it verifies
   * - 1
     - Headless smoke
     - Bash + ``vpr`` under ``QT_QPA_PLATFORM=offscreen``
     - End-to-end ``--graphics_commands`` parsing, CLI surface,
       crash prevention, and basic ``save_graphics`` invocation.
   * - 3
     - Catch2 integration
     - ``test_vpr_gui`` binary
     - GUI logic without a window: ``ezgl::MainWindow`` widget-tree
       loading, draw-state transitions, renderer plumbing.
   * - 4
     - Interactive
     - ``test_vpr_gui [layer4]`` (``QTest::mouseMove`` /
       ``QTest::keyClick``)
     - Mouse and keyboard event dispatch through ``act_on_mouse_*``
       and ``act_on_key_*``.
   * - 5
     - Visual regression
     - Bash driver + Python SSIM compare against pinned golden PNGs
     - That rendered output for representative scenes matches the
       committed corpus within an SSIM threshold.

Layer 2 (a hypothetical Qt-only ezgl unit layer) lives in the
``libs/EXTERNAL/libezgl`` repository and is run there; it is referenced
here only for traceability.


6. Test Execution Interface
---------------------------

All test commands are launched through the top-level Make wrapper. The
``CMAKE_PARAMS`` variable is required to enable EZGL.

.. code-block:: console

   # Build VPR + the GUI test binary
   make CMAKE_PARAMS="-DVPR_USE_EZGL=on" -j$(nproc) vpr test_vpr_gui

   # Full GUI suite — selected by CTest label "gui"
   make CMAKE_PARAMS="-DVPR_USE_EZGL=on" gui-tests

   # Per-layer aggregates (label-based selection)
   make CMAKE_PARAMS="-DVPR_USE_EZGL=on" gui-tests-smoke        # layer1
   make CMAKE_PARAMS="-DVPR_USE_EZGL=on" gui-tests-unit         # layer3
   make CMAKE_PARAMS="-DVPR_USE_EZGL=on" gui-tests-interactive  # layer4
   make CMAKE_PARAMS="-DVPR_USE_EZGL=on" gui-tests-visual       # layer5

   # GUI-scoped coverage report (HTML + XML + TXT under build/)
   make BUILD_TYPE=debug \
        CMAKE_PARAMS="-DVPR_USE_EZGL=on -DVTR_ENABLE_COVERAGE=on" \
        gui-coverage

The standard regression runner is also wired:

.. code-block:: console

   ./run_reg_test.py vtr_reg_gui


7. Coverage Targets
-------------------

GUI-scoped coverage is measured by ``gcovr`` over:

* ``vpr/src/draw/``
* ``vpr/src/base/read_options.cpp``
* ``libs/EXTERNAL/libezgl/src``

Generated artefacts under ``build/`` (or ``build_cov/``):

* ``coverage_gui.html`` — per-file drill-down
* ``coverage_gui.xml``  — Cobertura, consumed by CI
* ``coverage_gui.txt``  — summary

Quality gate enforced by the ``gui-coverage`` target:

.. list-table::
   :widths: 30 30 40
   :header-rows: 1

   * - Metric
     - Threshold
     - Rationale
   * - Line
     - ≥ 35 %
     - Regression-protection floor that tracks the measured baseline.
       The absolute target remains 80 % once a Qt event-pump fixture
       lifts Layer 4 / 5 reachability (see DEF-008, DEF-009).
   * - Branch
     - ≥ 22 %
     - Same rationale — structural ceiling on the current GUI corpus.

The gate fires during ``make gui-coverage`` (``--fail-under-line`` /
``--fail-under-branch`` in ``vpr/test/gui/CMakeLists.txt``) so a PR that
removes coverage is rejected before merge.


8. Entry and Exit Criteria
--------------------------

8.1 Entry criteria (the suite may be run)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* The platform install script has been run successfully.
* ``.venv`` is present and activated.
* ``make CMAKE_PARAMS="-DVPR_USE_EZGL=on" -j$(nproc) vpr test_vpr_gui``
  builds without warnings escalated to errors.

8.2 Exit criteria (a PR may merge)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A pull request that touches GUI-scoped paths is accepted only when *all*
of the following hold:

* ``make gui-tests`` is green on Ubuntu 24.04 and macOS 14 in CI.
* ``./run_reg_test.py vtr_reg_gui`` is green.
* ``make gui-coverage`` passes the line / branch thresholds.
* Every newly skipped, ``xfail``\ ed, or ``WILL_FAIL`` case cites a
  tracked ``DEF-NNN`` (and its GitHub issue) in source.
* No previously-passing case has regressed to fail or skip.


9. Defect Tracking Policy
-------------------------

* Real defects discovered by the suite are recorded in
  :ref:`vpr_gui_defect_issues_log` with a stable ``DEF-NNN`` identifier.
* Each entry is filed as a GitHub issue in the appropriate repository
  (``vtr-verilog-to-routing-QL`` for VPR / flow scripts; ``ezgl-QL`` for
  ezgl) and the URL is linked from the entry.
* Severity levels are *Critical / Major / Minor / Cosmetic* (legend in
  the log).
* An entry closes only when (a) a fixing commit is referenced and
  (b) the previously-failing test now passes without modification.


10. Risks and Mitigations
-------------------------

.. list-table::
   :widths: 4 36 60
   :header-rows: 1

   * - #
     - Risk
     - Mitigation
   * - R1
     - ``save_graphics`` blocked under offscreen Qt (DEF-004 /
       ezgl-QL#9) keeps part of Layer 5 unreachable.
     - Affected scenes ship as allowed-failure; lifted when the upstream
       fix lands.
   * - R2
     - macOS-only ``run_vtr_flow.py`` failures (DEF-006, DEF-007).
     - Install scripts provision ``gtime`` and ``-track_memory_usage``
       accepts an explicit value (e.g. ``-track_memory_usage off``) as
       the CLI escape hatch; tracked for upstream merge.
   * - R3
     - Visual goldens drift between Qt minor versions (DEF-010).
     - SSIM threshold defaults to 0.98 and the runner auto-selects a
       per-platform golden directory (``golden/<os>-<arch>/``) when
       present; goldens are committed only after a triaged defect
       closes as *expected visual change*.
   * - R4
     - Coverage of Layer 4 / 5 paths is structurally limited until
       ``--disp on`` works under offscreen Qt (DEF-008).
     - Coverage gate set at the measured baseline; raised when the
       blocker closes.


11. Test Resilience Conventions
-------------------------------

The suite is hardened against common fragility patterns. New tests must
follow the same conventions:

* **Up-front input validation.** The Layer 1 / 5 driver scripts probe
  for the VPR binary, architecture file, and required benchmarks before
  running anything; a missing input fails with a single actionable
  error line, not a buried log dive.
* **RAII for Qt windows.** Tests that load ``main.ui`` via
  ``ezgl::MainWindow`` own the resulting ``QMainWindow`` (either
  through ``MainWindow``'s own destructor or by transferring with
  ``MainWindow::release()`` into a ``std::unique_ptr<QMainWindow>``),
  so a failed ``REQUIRE`` cannot leak windows or leave widgets
  registered with the global ``QApplication``.
* **Platform-aware visual goldens.** The Layer 5 runner first looks
  for ``vpr/test/gui/golden/<os>-<arch>/`` (e.g.
  ``golden/darwin-arm64/``) and falls back to the shared
  ``vpr/test/gui/golden/`` directory. Drop a per-platform corpus in
  the subdirectory only when the cross-platform delta exceeds the
  default 0.98 SSIM threshold.
* **Configurable SSIM threshold.** The default is 0.98. The threshold
  can be raised (e.g. for a same-host self-comparison) via the
  ``VPR_GUI_SSIM_THRESHOLD`` environment variable or the optional 5th
  positional argument to ``run_visual_regression.sh``. Lower it only
  with a paired defect entry — never to silence a regression.
* **Stable label scheme.** Every test carries
  ``LABELS "gui;<kind>;<layerN>"``; ``ctest -L`` is the only supported
  selection mechanism. Filenames or test-name regexes are not part of
  the contract.
* **No platform-conditional skips in source.** A test that does not
  apply to a platform must be ``ctest -L``-excluded by the per-target
  ``set_tests_properties`` call, with a comment citing the relevant
  ``DEF-NNN``.


12. Coverage Gap Analysis and Proposed Test Backlog
---------------------------------------------------

This section records the assessment of what the current four-layer
suite *does not* cover, and the prioritised backlog of new test cases
that close the largest gaps. The list is the single source of truth
for follow-up GUI test work; tests added in future PRs cite the
``GUI-T-NNN`` identifier from §12.3 in the commit message and in the
test source.

12.1 What the current suite covers well
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* Layer 1 — every ``--graphics_commands`` token used by Layer 5 and
  ``vtr_reg_gui`` (``set_nets``, ``set_draw_block_internals``,
  ``set_draw_block_text``, ``set_draw_block_outlines``, ``set_macros``,
  ``set_congestion``, ``set_routing_util``, ``set_cpd``,
  ``set_draw_net_max_fanout``, ``save_graphics``, ``exit``); the
  GUI-linked binary's pack-only and full P&R paths.
* Layer 3 — structural UI loading: ``ezgl::MainWindow`` (which today
  delegates to ``QtGladeLoader``) produces every widget type used in
  ``main.ui``; presence and item counts for every named widget;
  ``t_draw_state``/``t_draw_coords`` POD round-trip; ezgl primitive
  value types (``color``, ``point2d``, ``rectangle``).
* Layer 4 — null-pointer guard for ``act_on_mouse_move`` (DEF-005).
* Layer 5 — fourteen ``save_graphics`` scenes covering placement,
  routing, and mid-routing baselines plus every overlay variant
  (``set_nets`` ∈ {1, 2}, ``set_cpd`` ∈ {1, 2, 3, 4, 5, 7},
  ``set_draw_block_internals 2``, ``set_congestion`` ∈ {1, 2}). The
  case list is the sourced ``vpr/test/gui/visual_cases.sh``; saves
  are batched into two VPR invocations at the ``placement_done``,
  ``routing_done``, and ``routing_initial`` stage barriers.
* ``vtr_reg_gui`` — four flow-level scenarios driving the GUI-linked
  binary end-to-end with `--disp off` and a single setter.

12.2 What the current suite does not cover
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The audit identified the following whole-feature gaps. Each line names
the production file(s) that have *zero* assertions exercising their
behaviour from any layer, and the user-visible feature that depends on
that code path.

* **Interactive semantics (Layer 4 is placeholder).** Widget click /
  toggle handlers wired in ``vpr/src/draw/ui_setup.cpp`` and the
  default callback set in ``vpr/src/draw/draw.cpp`` (``Proceed``,
  ``ZoomFit``, ``Pause``, ``Debug``, block-outline and block-text
  checkboxes, ``ClipRoutingUtil``, ``DrawPartitions``) are wired but no
  test asserts that a click / toggle reaches the corresponding
  ``t_draw_state`` field.
* **Search subsystem (entirely untested).** ``vpr/src/draw/search_bar.cpp``:
  ``search_and_highlight`` for Block ID / Block Name / Net ID / Net Name
  / RR Node ID; ``search_type_changed`` and ``enable_autocomplete``;
  ``auto_zoom_rr_node`` camera transform; the invalid-search warning
  dialog.
* **Debugger / breakpoint UI (entirely untested).**
  ``vpr/src/draw/draw_debug.cpp`` debugger window and breakpoint list
  management; ``vpr/src/draw/breakpoint.cpp`` expression parsing,
  activation, deletion; ``breakpoint_info_window`` and
  ``invalid_breakpoint_entry_window`` dialogs; the move / temp /
  router / net / expression breakpoint kinds.
* **Manual moves (entirely untested).** ``vpr/src/draw/manual_moves.cpp``
  form validation, legal / illegal destination checks, accept / reject
  summary dialog flow.
* **RR-graph rendering (only via image diff).**
  ``vpr/src/draw/draw_rr.cpp`` — ``draw_rr_edges``, ``draw_rr_node``,
  ``draw_rr_chan``, ``draw_cluster_pin``, ``draw_rr_costs``; click-to-
  highlight RR fan-in / fan-out edges; no Layer 5 golden currently
  exercises any RR-overlay command.
* **NoC overlay (entirely untested).** ``vpr/src/draw/draw_noc.cpp``
  link-usage colour mapping; parallel-link offset logic; marker
  rendering. No Layer 1 or Layer 5 scenario enables NoC display.
* **Save-graphics dialog path.** Only the command-line
  ``save_graphics`` token is tested. The ``SaveGraphics`` button →
  ``save_graphics_from_button`` → ``save_graphics_dialog_box`` flow in
  ``vpr/src/draw/save_graphics.cpp``, format selection (PNG / PDF /
  SVG), and the invalid-extension warning path are untested.
* **Graphics-commands parser — happy paths only.**
  ``set_clip_routing_util`` is implemented but no scenario invokes it.
  No negative test covers an unknown / malformed ``--graphics_commands``
  token.
* **Runtime-generated 3D-view widgets.** ``view_button_setup`` in
  ``vpr/src/draw/ui_setup.cpp`` builds Layer-N checkboxes,
  per-layer transparency spinboxes, and cross-layer-connection
  controls dynamically based on the architecture; their effect on
  ``t_draw_state`` is unverified.
* **Stage-aware enable / disable gating.** ``ToggleCritPathRouting``
  should be enabled only in routing stage with timing info;
  ``ToggleCongestion`` only after route, etc. The gating logic in
  ``ui_setup.cpp`` is not asserted.
* **libezgl public API surface.** ``camera`` world / screen
  transforms; ``application::create_button`` / ``create_combo`` /
  ``create_dialog`` / ``create_popup_message`` / ``destroy_widget``;
  ``control::zoom_in`` / ``zoom_out`` / ``zoom_fit`` / ``translate``;
  ``canvas::print_pdf`` / ``print_svg`` / ``draw_offscreen`` are only
  type-checked at compile time, not behaviourally tested.

The coverage gap measured at the gate (Linux: 28.2 % line / 19.1 %
branch) is consistent with this inventory: the current suite touches
the *loading* surface, not the *behaviour* surface.

12.3 Proposed test backlog
~~~~~~~~~~~~~~~~~~~~~~~~~~

Each entry below is a ``GUI-T-NNN`` ticket. Tests added in future PRs
cite the ticket id in (a) the commit message, (b) the Catch2
``[GUI-T-NNN]`` tag, and (c) the bash scenario name where applicable.
Priority and **wave** are set against expected user impact and
prerequisite ordering, not implementation cost. The wave column
governs scheduling: a Wave-2 ticket is not started until every Wave-1
ticket it depends on is closed, so the suite never adds visual /
golden coverage on top of an unverified behaviour path.

.. list-table::
   :widths: 7 5 24 9 7 48
   :header-rows: 1

   * - Id
     - Layer
     - Title
     - Priority
     - Wave
     - Coverage closed (production symbol → user behaviour)
   * - GUI-T-001
     - 4
     - Search bar — all five lookup branches plus autocomplete and
       invalid-search dialog
     - High
     - 2
     - **Wave 2 PR — partial landing (6 cases / 39 assertions in
       test_search_dispatch.cpp):** ``search_and_highlight``
       positive paths for Block ID, Block Name, and post-route
       Net ID; negative paths for Block ID (out-of-range id) and
       Block Name (unknown name) routing through
       ``warning_dialog_box`` and ``deselect_all``; pre-route Net ID
       no-op contract (``highlight_nets(ClusterNetId)`` early-returns
       on empty ``route_trees``). The dispatcher's atom-first lookup
       resolves to either ``block_color == SELECTED_COLOR`` or a
       sub-pb selection inside the expected cluster — both
       observables are checked. **Coverage gaps tracked separately:**
       Net Name positive path, RR Node ID +
       ``auto_zoom_rr_node``, ``search_type_changed`` autocomplete
       branches, and the empty-search-type warning ride on a later
       wave. **Prerequisites met:** GUI-T-017 Step 1+2 (run-stage
       fixture + ``EzglAppFixture``) supply ``g_vpr_ctx`` and a
       loaded ``main.ui``; the null-guard symmetry portion is closed
       by GUI-T-019.
   * - GUI-T-004
     - 1
     - graphics_commands parser — sequencing, ``{i}`` substitution,
       state restore, plus negatives and ``set_clip_routing_util``
     - High
     - 1
     - Beyond single-token dispatch: a multi-token command string is
       executed in order; the ``{i}`` placeholder is substituted into
       ``save_graphics`` filenames across iterations; ``run_graphics_commands``
       saves and restores ``t_draw_state`` around the command list so a
       subsequent ``--disp on`` session is not contaminated; an
       unknown / malformed token surfaces an error rather than a
       silent no-op; ``set_clip_routing_util`` (the only implemented
       setter no scenario currently drives) is exercised.
   * - GUI-T-008
     - 4
     - Stage-aware control gating across placement / routing /
       routing-with-timing
     - High
     - 2
     - **Step 2 landed (Wave 2 PR):** ``test_stage_gating.cpp`` (4
       cases, 65 assertions) locks the contract of the two public
       gating functions in ``vpr/src/draw/ui_setup.cpp``:

         * ``hide_crit_path_routing(app)`` — exhaustive 10-row truth
           table over ``(pic_on_screen × setup_timing_info ×
           show_crit_path)`` covering ROUTING (all 4 combinations of
           the two booleans), PLACEMENT (all 4), and the two boundary
           stages NO_PICTURE / ANALYTICAL_PLACEMENT. Each row
           pre-flips the widget to the *opposite* of the expected
           state so the test can prove the function actually drives
           ``setEnabled``.
         * ``hide_draw_routing(app)`` — PLACEMENT removes the
           ``"Routing"`` item; non-PLACEMENT stages re-add it when
           absent and preserve a single instance on repeated calls
           (the production redraw loop calls it on every refresh, so
           idempotency is a hard contract); cross-stage transition
           PLACEMENT → ROUTING → PLACEMENT toggles presence
           consistently.

       Built atop the new Layer-4 substrate landed in this same PR
       (see GUI-T-017 Step 2): ``EzglAppFixture`` loads ``main.ui``
       via ``ezgl::MainWindow`` and exposes the process-wide
       ``ezgl::application*`` constructed in ``test_gui_main.cpp``;
       the fixture snapshots / restores ``t_draw_state`` per case so
       stage mutations do not bleed. Local ``ComboItemsGuard`` and
       ``WidgetEnabledGuard`` RAII helpers restore widget state
       inside each case to immunise downstream tests against the
       known top-level-popover leak in main.ui's GtkPopover entries.
       **Coverage gaps tracked separately:** ``ToggleCongestion``
       and ``ToggleRoutingUtil`` post-route enablement, and
       ``ToggleNocBox`` NoC-arch gating, ride on later VPR run-stage
       work; net / RR menu insertion paths land alongside GUI-T-013
       / GUI-T-014 in Step 3+. **Prerequisite met:** GUI-T-017 Step
       2 (``EzglAppFixture``) is the substrate; the static widget
       contract is separately covered by GUI-T-018.
   * - GUI-T-013
     - 4
     - Canvas zoom-select drag (rubber-band)
     - High
     - 2
     - **Wave 2 PR landed (7 cases / 26 assertions in
       test_zoom_select.cpp):** ``toggle_window_mode`` flips the
       state machine into rubber-band mode; first left-press
       collects ``point_1`` and seeds the preview cursor; mouse-move
       updates only the cursor (anchor unchanged); non-left clicks
       are ignored while armed; the second left-click commits a new
       camera world rect for both square and tall aspect ratios;
       ``zoom_fit`` recovers the original view. ``ZoomSelectStateGuard``
       restores the four file-scope globals (``window_mode``,
       ``window_point_1_collected``, ``point_1``,
       ``window_preview_cursor``) per case. Real-dispatch path —
       not just the DEF-005 null-guard. **Prerequisites met:**
       GUI-T-017 Step 2 (``EzglAppFixture`` + canvas registration).
   * - GUI-T-014
     - 4
     - Block hit-test on canvas click
     - High
     - 2
     - **Wave 2 PR landed (4 cases / 29 assertions in
       test_block_hit_test.cpp):** ``get_cluster_block_id_from_xy_loc``
       returns the expected ``ClusterBlockId`` for clicks at every
       placed block's bbox center (loops the full clustering),
       returns ``ClusterBlockId::INVALID()`` for clicks far outside
       the grid (no spurious selection), honours per-layer
       visibility (flipping ``draw_layer_display[*].visible`` to
       false makes a known-good click miss; restoring it makes the
       click hit again), and resolves to the same block at all four
       interior bbox corners (ε-from-corner sampling). Each case
       composes ``VprRunStageFixture::run_to(PostPlace)`` with a
       local ``DrawStructsScope`` that flips ``show_graphics`` and
       runs ``alloc_draw_structs`` + ``init_draw_coords`` so the
       drawing globals are populated under ``--disp off``. Baseline
       interaction every user performs. **Prerequisite met:**
       GUI-T-017 Step 1 (``VprRunStageFixture`` PostPlace stage).
   * - GUI-T-017
     - 3/4
     - VPR run-stage Catch2 fixture (post-init / post-pack / post-
       place / post-route ``g_vpr_ctx`` snapshots)
     - High
     - 2
     - **Step 1 landed (Wave 2 PR):** ``VprRunStageFixture`` (header
       in ``vpr/test/gui/test_runstage_fixture.hpp``, impl in
       ``test_runstage_fixture.cpp``) runs VPR in-process through
       ``vpr_init`` → ``vpr_pack_flow`` → ``vpr_create_device`` →
       ``vpr_place_flow`` → ``vpr_route_flow`` on
       ``and_latch.blif`` over ``k6_N10_40nm.xml`` and exposes
       ``g_vpr_ctx`` plus the in-flight ``t_vpr_setup`` / ``t_arch``
       to the test process. Each fixture instance creates its own
       ``/tmp/vpr_gui_t017_<pid>_<n>`` work-dir, ``chdir``s into it
       at construction, and tears down + ``vpr_free_all``s in the
       destructor. ``vpr_initialize_logging`` is one-shot per
       process (atomic-guarded). 5 sentinel cases tagged
       ``[GUI-T-017]`` lock the contract (PostInit logical-block
       count ≥ 2, PostPack ``clb_nlist.blocks() ≥ 3``, PostPlace
       valid block coordinates, PostRoute ``rr_graph.num_nodes() >
       0``, double-call throws ``std::logic_error``). **Step 2 landed
       (Wave 2 PR):** ``test_gui_main.cpp`` switched from constructing
       a bare ``QApplication`` to constructing a process-global
       ``ezgl::application`` (singleton accessor in
       ``test_app_singleton.hpp``); ``EzglAppFixture``
       (``test_ezgl_app_fixture.hpp/.cpp``) loads ``main.ui`` via
       ``ezgl::MainWindow`` and snapshots / restores ``t_draw_state``,
       providing the Layer-4 substrate that GUI-T-008 (and later
       GUI-T-013 / GUI-T-014 / GUI-T-001) require. **Step 3+ open:**
       per-test widget restoration helpers may be promoted into the
       fixture once two or more Wave-2 tests need the same guard
       pattern. **Unblocks** GUI-T-001, GUI-T-008, GUI-T-013,
       GUI-T-014, GUI-T-005, GUI-T-007.
   * - GUI-T-018
     - 3
     - main.ui ↔ ui_setup.cpp widget contract
     - High
     - 1
     - For every widget that ``ui_setup.cpp``, ``draw.cpp``,
       ``search_bar.cpp``, and ``draw_toggle_functions.cpp`` look up
       at runtime (``find_push_button`` / ``find_check_box`` /
       ``find_combo_box`` / ``find_spin_box`` / ``find_switch_button``
       / ``find_line_edit``), the loaded ``main.ui`` widget tree
       contains a top-level widget with the matching ``objectName``
       and the correct Qt subclass, plus contracted item count and
       order on every menu combo. Catches silent renames / retypes
       in ``main.ui`` that would otherwise misroute dispatch at
       runtime.
   * - GUI-T-019
     - 4
     - Search subsystem null-``app`` guard symmetry
     - High
     - 1
     - Closes the symmetric-guard gap named alongside DEF-005 /
       DEF-012 for the search entry points. Asserts that
       ``search_and_highlight``, ``enable_autocomplete``, and
       ``search_type_changed`` return cleanly when invoked with a
       null ``ezgl::application``; runs each in a forked child so a
       crash isolates rather than tearing down ``test_vpr_gui``. The
       ``search_and_highlight`` case is currently failing; filed as
       **DEF-013** in :ref:`vpr_gui_defect_issues_log`.
   * - GUI-T-002
     - 4
     - Debugger — add, enable, delete a breakpoint
     - High
     - 2
     - **Wave 3 PR landed (12 cases / 65 assertions in
       test_breakpoint_debug.cpp):** locks the contract of the
       breakpoint subsystem in ``vpr/src/draw/breakpoint.{h,cpp}``
       and the helper functions in ``vpr/src/draw/draw_debug.cpp``
       — ``Breakpoint`` constructors (default sentinel,
       typed-int per ``bp_type``, expression), ``operator==``,
       ``activate_breakpoint_by_index`` / ``delete_breakpoint_by_index``
       positional mutation; ``check_for_breakpoints`` empty-list /
       in_placer-gate / active-flag-gate / matching-expression
       short-circuit (uses ``BreakpointStateGlobals`` to prime
       ``move_num`` and verify ``bp_description`` round-trip);
       ``valid_expression`` COMP-BOOL alternation pattern (empty,
       leading bool, trailing bool, two-comp-in-row, balanced,
       longer balanced); ``close_debug_window`` /
       ``close_advanced_window`` idempotent state-flag reset;
       ``get_bp_state_globals`` singleton-pointer stability with
       cross-call mutation persistence (the path
       ``annealer.cpp`` and ``route_utils.cpp`` use to
       drive the placer / router pause). The window-construction
       paths (``draw_debug_window``, ``advanced_button_callback``)
       are deliberately NOT exercised — their
       ``QIcon("src/draw/trash.png")`` and dynamic ``QScrollArea``
       layouts are not stable under the offscreen Qt platform
       plugin; the state-flag contract those windows wire to
       ``QObject::destroyed`` is locked symmetrically through the
       close-window helpers. **Prerequisites met:** none beyond
       Wave 2 substrate (process-static draw_state).
   * - GUI-T-003
     - 4
     - SaveGraphics — button-driven dialog workflow
     - High
     - 2
     - **Wave 3 PR landed (6 cases / 28 assertions in
       test_save_graphics.cpp):** ``save_graphics`` writes a
       non-empty file for each of png / pdf / svg under a
       per-case ``TmpDir`` (RAII); leading-dot extension is
       stripped (``".png"`` and ``"png"`` produce the same file);
       invalid extension produces no file (warning_dialog branch);
       ``save_graphics_from_button`` reads the
       ``file_name_text_entry`` / ``file_name_combo_box`` named
       widgets and dispatches accordingly; missing widgets are a
       no-op (defensive guard); ``save_graphics_dialog_box``
       constructs a Save / Cancel dialog parented to the main
       window with the contracted shape — title ``"Save
       Graphics Contents"``, default text ``"vpr_graphics"``,
       combo items ``pdf`` / ``png`` / ``svg`` at indices 0 / 1 /
       2 (the three formats the ``save_graphics`` writer supports).
       Each case uses ``EzglAppFixture`` + ``DrawApplicationScope``
       + ``ensure_main_canvas`` (no-op draw callback) so
       ``application->get_canvas`` succeeds.
   * - GUI-T-005
     - 4
     - RR click → highlight fan-in / fan-out edges
     - Medium
     - 2
     - **Wave 3 PR landed (5 cases / 23 assertions in
       test_rr_click_highlight.cpp):** the
       ``highlight_rr_nodes(RRNodeId)`` path toggles the picked
       node's colour MAGENTA ↔ WHITE on alternate calls and
       maintains ``draw_state->hit_nodes`` set membership in
       lockstep; ``RRNodeId::INVALID()`` returns false without
       mutating ``hit_nodes``; the ``(float, float)``
       coordinate overload misses for far-out-of-grid coords (no
       spurious selection) and respects the per-layer visibility
       gate (a 5×5 world-coord sweep with all
       ``draw_layer_display[*].visible`` flipped to false produces
       no hit; restoring visibility makes hits return); the
       non-configurable expansion closure ensures every node in
       ``hit_nodes`` is MAGENTA + ``node_highlighted=true`` after
       a click (the colouring is closed under the production
       expansion, not just the picked node). Each case composes
       ``EzglAppFixture`` + ``VprRunStageFixture::run_to(PostRoute)``
       + ``DrawStructsScope`` so the rr-graph and ``draw_rr_node``
       table are populated. ``pick_inter_cluster_node()`` skips
       ``SOURCE`` / ``SINK`` (the production hit-test does the
       same, so the test mirrors the click landing behaviour).
       **Prerequisite met:** GUI-T-013 + GUI-T-014 canvas hit-test
       substrate.
   * - GUI-T-007
     - 4
     - Manual moves — validation and accept / reject
     - Medium
     - 2
     - **Wave 3 PR landed (8 cases / 39 assertions in
       test_manual_moves.cpp):** ``ManualMovesInfo`` /
       ``ManualMovesState`` zero-init defaults match the documented
       contract (``blockID = -1``, ``valid_input = true``, all
       window flags false, ``placer_move_outcome == ABORTED``);
       ``string_is_a_number`` recognises pure-digit ids and
       rejects names / signs / whitespace (and is vacuously true
       for the empty string — the production callback's
       observable behaviour); ``close_manual_moves_window`` flips
       ``manual_move_window_is_open`` to false and is idempotent;
       ``manual_move_is_selected`` reads the ``manualMove``
       checkbox loaded from ``main.ui`` and writes the result back
       into ``manual_moves_state.manual_move_enabled`` (round-trip
       under both checked and unchecked states);
       ``is_manual_move_legal`` rejects an invalid block id, an
       out-of-grid (negative + huge) destination, and a
       same-location no-op move — each rejection synchronously
       opens an ``invalid_breakpoint_entry_window`` whose
       resulting QWidget is reaped by ``EzglAppFixture``. The
       ``manual_move_cost_summary_dialog`` / ``pl_do_manual_move``
       paths that drive ``QDialog::exec()`` are deliberately NOT
       exercised — the modal event loop would either hang under
       the offscreen Qt platform plugin or require a brittle
       ``QTimer`` injection. **Prerequisite met:** GUI-T-017 Step 1
       (``VprRunStageFixture`` PostPlace).
   * - GUI-T-015
     - 4
     - Keyboard navigation — Enter / Tab / Escape / arrow keys
     - Medium
     - 2
     - **Wave 3 PR landed (4 cases / 39 assertions in
       test_keyboard_nav.cpp):** ``act_on_key_press`` (the
       ezgl key-press dispatcher in ``vpr/src/draw/draw.cpp``,
       non-static but with no header) drives ``deselect_all`` on
       ``"Escape"`` (a SELECTED block restores its default colour);
       the ``draw_state->justEnabled`` gate survives one
       non-Return / Tab keystroke (the autocomplete that was just
       enabled stays open) and clears the completer on the
       following keystroke; arrow / Home / End / Page_Up /
       Page_Down / character keys are dispatched without throwing;
       ``enable_autocomplete`` installs the BlockNames completer
       when SearchType is ``Block Name`` and is a no-op on
       ``Block ID`` (the early-return path leaves the sentinel
       completer untouched). ``CompleterStateGuard`` saves and
       restores ``QLineEdit::completer`` and
       ``draw_state->justEnabled`` per case. The keystroke tests
       drive the production code paths directly; they do not
       require focus delivery (offscreen Qt is unreliable about
       focus events, but ``act_on_key_press`` reads from
       ``draw_state->justEnabled`` and the search-bar widget's
       focus state, both of which the test prepares explicitly).
   * - GUI-T-006
     - 5
     - Visual goldens for RR and NoC overlays
     - Low
     - 3
     - **Wave 3 PR landed (5 cases / 17 assertions in
       test_visual_render_smoke.cpp — first visual coverage,
       gate-only):** ``draw_rr(nullptr)`` is null-renderer-safe
       when ``show_rr=false`` (gate is the very first statement;
       a refactor moving any ``g->...`` call above the gate
       would crash the test, catching a real GUI-repaint
       regression); ``draw_noc(nullptr)`` is null-renderer-safe
       when ``draw_noc == DRAW_NO_NOC`` (the production default,
       and a hard requirement on this NoC-less arch since
       ``draw_noc.cpp`` dereferences
       ``noc_model.get_noc_routers().begin()`` unconditionally
       after the gate); ``t_draw_state`` defaults verify every
       overlay starts gated OFF (``show_rr``, ``draw_noc``,
       ``show_nets``, ``show_crit_path``, ``draw_partitions``,
       ``draw_channel_nodes``, ``draw_intra_cluster_nodes``);
       ``show_rr`` toggle is observable through
       ``get_draw_state_vars()``; ``draw_rr_node`` table is
       sized to ``rr_graph.num_nodes()`` and round-trips colour /
       highlight writes (the storage that ``draw_rr`` reads on
       every repaint). The full PNG / SVG render path through
       ``save_graphics`` is exercised separately by GUI-T-003 with
       a no-op draw callback; driving the *real* ``draw_rr``
       through ``print_png`` under the offscreen Qt platform
       plugin's RHI fallback (the QRhiGles2 backend tries to
       negotiate a software GLES context that is not available
       on this CI host) produced ``munmap_chunk()`` heap
       corruption during teardown when this file was first
       prototyped — that path is reserved for a follow-up
       Layer-5 ticket once a stable software rasterizer is
       wired up. **Prerequisite met:** GUI-T-005 (RR rendering
       state model).
   * - GUI-T-009
     - 4
     - 3D view — runtime layer / transparency / cross-layer widgets
     - Low
     - 3
     - ``view_button_setup`` runtime widget creation; per-layer
       checkbox / spinbox effect on ``t_draw_state`` layer-visibility
       and transparency vectors.
   * - GUI-T-010
     - 4
     - Default callback wiring — Proceed, ZoomFit, Pause, Debug,
       checkbox set
     - Low
     - 3
     - End-to-end click → callback → ``t_draw_state`` mutation for
       every widget the default callback set in ``draw.cpp`` wires
       up. Catches "wired to the wrong slot" regressions.
   * - GUI-T-011
     - 3
     - libezgl ``camera`` transforms — world ↔ screen ↔ widget
     - Low
     - 3
     - Behavioural test of ``camera.hpp`` transform round-trip;
       complements the existing ``rectangle`` and ``point2d`` value
       tests.
   * - GUI-T-012
     - 3
     - libezgl ``control`` helpers — zoom_in / out / fit / translate
     - Low
     - 3
     - ``control.hpp`` against a constructed canvas + camera.

**Wave summary.**

* **Wave 1 — fixture-free, fully-testable now** (T-004, T-018,
  T-019). Closes the parser-semantics gap, locks the static widget
  contract that every dispatch path depends on, and starts the
  search-subsystem null-guard symmetry work. Per §4, every Wave-1
  case asserts a real behavioural contract — no placeholders.
* **Wave 2 — unblocked by the run-stage fixture** (T-017 first, then
  T-001, T-002, T-003, T-005, T-007, T-008, T-013, T-014, T-015).
  T-017 provides the ``g_vpr_ctx`` snapshot that turns each of these
  from a placeholder into a real assertion path; landing it first is
  the gate for the rest of Wave 2.
* **Wave 3 — visual, runtime, library** (T-006, T-009, T-010, T-011,
  T-012). Visual goldens are last on purpose: they only protect what
  has already been proven correct by behavioural tests.

12.4 Wiring new tests into ``make gui-tests``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When a backlog item is implemented, the test must:

#. Carry the ``GUI-T-NNN`` identifier in its Catch2 tag (Layer 3 / 4)
   or scenario function name (Layer 1 / 5).
#. Be registered with the standard label set
   ``LABELS "gui;<kind>;<layerN>"`` per §11 so it is selected by the
   existing ``ctest -L gui`` aggregate. **No new** ``add_test`` /
   ``register_test`` macros are introduced.
#. Append its scenario function to the existing driver script for
   Layer 1 / Layer 5 work, or add a new ``.cpp`` to the
   ``test_vpr_gui`` Catch2 binary's source list in
   ``vpr/test/gui/CMakeLists.txt`` for Layer 3 / 4 work. The CMake
   file already covers compile-time discovery; no new CTest entries
   are required.
#. Cite the closing ticket id (``GUI-T-NNN``) in the commit message
   and, when the test exposes a defect, open a paired ``DEF-NNN``
   entry in :ref:`vpr_gui_defect_issues_log` per §9.

12.5 Coverage-gate revision
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``gui-coverage`` thresholds in §7 (currently 35 % line / 22 %
branch on the macOS baseline; not yet met on Linux — see issue #37)
are deliberately set at the *measured baseline*, not at an aspirational
target. Each backlog item that lands raises the floor monotonically:
the gate is bumped in the same PR that closes the ticket so the new
floor is what protects the next change. A monotonic ratchet is
preferred over a cliff-edge upgrade to 80 %.

