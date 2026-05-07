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
     - GUI logic without a window: ``QtGladeLoader``, draw-state
       transitions, renderer plumbing, ``ezgl_qtcompat`` macros.
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
     - Install scripts provision ``gtime`` and a paired
       ``-no_track_memory_usage`` flag exists; tracked for upstream
       removal.
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
  ``QtGladeLoader`` own the resulting ``QMainWindow`` through a
  ``std::unique_ptr<QMainWindow>``, so a failed ``REQUIRE`` cannot
  leak windows or leave widgets registered with the global
  ``QApplication``.
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
