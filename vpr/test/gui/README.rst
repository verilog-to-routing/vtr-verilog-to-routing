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

Rendered PNGs and per-case diff triptychs from the visual-regression
runner are written into the cmake build tree and are **never wiped** —
new runs overwrite by filename, stale PNGs from removed cases linger
until cleaned by hand.

.. list-table::
   :widths: 35 65
   :header-rows: 1

   * - Path (under ``build/vpr/test/gui/``)
     - Contents
   * - ``pass1/<case>.png``
     - Rendered PNGs from VPR pass 1 (placement_done + routing_done
       overlays). One file per case in ``VISUAL_CASE_NAMES``.
   * - ``pass1/vpr.log``
     - Full VPR stdout/stderr for pass 1.
   * - ``pass2/<case>.png``
     - Rendered PNGs from VPR pass 2 (routing_initial congestion).
   * - ``pass2/vpr.log``
     - Full VPR stdout/stderr for pass 2.
   * - ``diff/<case>.png``
     - ``[golden | current | amplified-diff]`` triptych. The diff
       channel is ``|golden − current|`` amplified 8× and clipped, so
       sub-pixel drift is actually visible at the SSIM thresholds we
       care about (~0.98+).

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

Known Issues
------------

See ``doc/src/dev/vpr_gui_defect_log.rst`` for the full defect log.

- **DEF-001** (Minor): ``QtGladeLoader`` ignores ``sensitive`` property
- **DEF-002** (Major): ``QtGladeLoader`` calls ``std::exit(1)`` on file-open failure
- **DEF-003** (Minor): ``g_return_if_fail``/``g_return_val_if_fail`` call ``std::exit(1)``
- **DEF-004** (Critical): ``save_graphics``/``print_png`` crashes in Qt offscreen mode — blocks Layer 1 save tests and all Layer 5 tests

Related Documents
-----------------

- Testing strategy: ``doc/src/dev/vpr_gui_testing_strategy.rst``
- Defect log: ``doc/src/dev/vpr_gui_defect_log.rst``
