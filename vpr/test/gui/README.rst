VPR GUI Test Suite
==================

Automated tests for the VPR Qt 6 GUI migration (EZGL/GTK3 → Qt 6).

Test Layers
-----------

.. list-table::
   :widths: 10 20 15 55
   :header-rows: 1

   * - Layer
     - Name
     - Type
     - Description
   * - **1**
     - Headless Smoke
     - Bash
     - Runs VPR with ``--disp off`` and ``--graphics_commands`` in offscreen mode.
       Validates no crashes during pack/place/route and draw-state changes.
   * - **3**
     - Integration Tests
     - Catch2 (C++)
     - Tests UI loading (``QtGladeLoader``) and widget tree verification
       against ``main.ui``.
   * - **5**
     - Visual Regression
     - Bash + Python
     - Compares VPR-rendered PNGs against golden images using SSIM.
       Currently blocked by DEF-004.

Quick Start
-----------

**Prerequisites:**

.. code-block:: bash

   # Build VPR and tests
   cmake -B build \
       -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qtbase" \
       -DQt6Svg_DIR=/opt/homebrew/opt/qtsvg/lib/cmake/Qt6Svg
   cmake --build build --target vpr test_vpr_gui -j$(sysctl -n hw.ncpu)

   # For visual regression (Layer 5) — install Python deps
   pip install scikit-image Pillow numpy

**Run all layers:**

.. code-block:: bash

   ./vpr/test/gui/run_all_tests.sh \
       ./build/vpr/vpr \
       ./vtr_flow/arch/timing \
       ./vtr_flow/benchmarks/microbenchmarks

**Run individual layers:**

.. code-block:: bash

   # Layer 3 — Catch2 integration tests
   QT_QPA_PLATFORM=offscreen ./build/vpr/test/gui/test_vpr_gui --colour-mode ansi

   # Layer 1 — Headless smoke tests
   ./vpr/test/gui/run_headless_smoke.sh \
       ./build/vpr/vpr \
       ./vtr_flow/arch/timing \
       ./vtr_flow/benchmarks/microbenchmarks

   # Layer 5 — Visual regression (requires golden images)
   ./vpr/test/gui/run_visual_regression.sh \
       ./build/vpr/vpr \
       ./vtr_flow/arch/timing \
       ./vtr_flow/benchmarks/microbenchmarks

**Debug mode (write a diff PNG for every visual case):**

.. code-block:: bash

   ./vpr/test/gui/run_all_tests.sh --debug \
       ./build/vpr/vpr \
       ./vtr_flow/arch/timing \
       ./vtr_flow/benchmarks/microbenchmarks

Layer 5 Output and Diff Triptychs
---------------------------------

Each visual-regression run sweeps the matrix of
``${#VISUAL_CASE_NAMES[@]} cases × {rhi, immediate, deferred}``
renderers — VPR is invoked twice per renderer with the shared
graphics_commands sequences from ``visual_cases.sh``, passing
``--renderer <renderer>`` explicitly. Outputs land in
``build/vpr/test/gui/artifacts/<renderer>/`` and are compared against
**per-renderer** goldens at
``vpr/test/gui/golden/<renderer>/<case>.png``. Each renderer has its
own baseline so legitimate cross-renderer differences (dash phase,
0.5px stroke shift, etc.) don't show up as regressions. Missing
goldens **FAIL** the case (strict mode). The whole ``artifacts/`` dir
is wiped at the **start** of every run so stale PNGs from a previous
session can't be mistaken for the current one. The parent
``build/vpr/test/gui/`` dir is left alone — it also hosts the cmake
build state (``test_vpr_gui`` binary, ``CMakeFiles/``,
``CTestTestfile.cmake``).

.. list-table::
   :widths: 45 55
   :header-rows: 1

   * - Path (under ``build/vpr/test/gui/artifacts/``)
     - Contents
   * - ``<renderer>/<case>.png``
     - One rendered PNG per (renderer, case) pair. The renderer
       subdir groups all outputs from one ``--renderer`` value
       together — mirrors ``vpr/test/gui/golden/<renderer>/`` so the
       compare loop just diffs same-name files in matching dirs.
   * - ``<renderer>/vpr_<phase>.log``
     - Full stdout/stderr of one VPR invocation, named after the
       phase it covers: ``vpr_placement_routing.log``
       (placement_done + routing_done overlays) and
       ``vpr_routing_initial.log`` (routing_initial congestion). See
       ``visual_cases.sh`` for the case-to-phase mapping.
   * - ``<renderer>/diff/<case>.png``
     - ``[golden | current | amplified-diff]`` triptych. The diff
       channel is ``|golden − current|`` amplified 8× and clipped, so
       sub-pixel drift is actually visible at the SSIM thresholds we
       care about (~0.98+).

Promoting a current render to a per-renderer golden (e.g. when adding
goldens for ``immediate`` / ``deferred`` for the first time, or after
an intentional visual change)::

    cp build/vpr/test/gui/artifacts/<renderer>/<case>.png \
       vpr/test/gui/golden/<renderer>/<case>.png

To update **all** goldens at once — the usual case after an intentional
change to how a stage is drawn (placement, congestion, etc.) — regenerate
the whole matrix (every case × every renderer) with ``generate_goldens.sh``
rather than copying files by hand::

    ./vpr/test/gui/generate_goldens.sh        # defaults to build/vpr/vpr

This overwrites every ``vpr/test/gui/golden/<renderer>/<case>.png``, so
afterwards review the change (``git diff --stat`` and the cross-renderer
triptychs under ``golden/tmp/``) and commit only the intended updates.
Only regenerate when the visual change is intentional — it invalidates all
prior comparisons.

Diff-write policy:

* **default** — the triptych is written only when SSIM falls below
  the threshold (i.e. a test case fails). Passing cases skip the
  triptych-write cost so the runner stays cheap.
* **--debug** — the triptych is written for **every** case, passing
  or failing, so a developer can eyeball even passing renders for
  sub-threshold drift.

The triptych behaviour is implemented by ``compare_images.py``'s
``--diff-out`` (path) and ``--diff-on-fail-only`` (toggle) flags;
``run_visual_regression.sh`` always passes ``--diff-out`` and adds
``--diff-on-fail-only`` unless ``VPR_GUI_DEBUG=1`` is set by
``run_all_tests.sh --debug``.

**Via CTest (from build subdirectory):**

.. code-block:: bash

   cd build/vpr/test/gui
   ctest --output-on-failure              # all 3 targets
   ctest -R test_vpr_gui$                 # Layer 3 only
   ctest -R test_vpr_gui_smoke            # Layer 1 only
   ctest -R test_vpr_gui_visual           # Layer 5 only
   ctest -L gui                           # by label

.. note::

   CTest targets are registered in ``vpr/test/gui/CMakeLists.txt`` with a local
   ``enable_testing()`` call.  Run ``ctest`` from ``build/vpr/test/gui/``, not
   from the top-level ``build/`` directory (top-level ``enable_testing()`` is
   disabled during migration — ``QT_MIGRATION_TMP``).

Files
-----

.. list-table::
   :widths: 35 65
   :header-rows: 1

   * - File
     - Purpose
   * - ``CMakeLists.txt``
     - Build target ``test_vpr_gui`` + CTest entries for all layers
   * - ``test_gui_main.cpp``
     - Custom Catch2 main with ``QApplication`` initialisation
   * - ``test_gui_helpers.hpp``
     - ``findWidgetByName<T>()`` — top-level widget lookup via ``QApplication::allWidgets()``
   * - ``test_gui_flow.cpp``
     - Layer 3: widget tree verification against ``main.ui``
   * - ``test_qtgladeloader.cpp``
     - Layer 3: Glade XML → Qt widget loader tests
   * - ``run_headless_smoke.sh``
     - Layer 1: 8 headless smoke tests (6 pass + 2 xfail DEF-004)
   * - ``run_visual_regression.sh``
     - Layer 5: SSIM comparison against golden images
   * - ``generate_goldens.sh``
     - Layer 5: generate reference golden PNGs
   * - ``compare_images.py``
     - Layer 5: SSIM image comparison tool
   * - ``golden/``
     - Layer 5: golden image storage (see ``golden/MANIFEST.rst``)
   * - ``run_all_tests.sh``
     - Unified runner — orchestrates all layers
