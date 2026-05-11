.. _vpr_gui_defect_issues_log:

VPR GUI Qt Migration — Defect & Issues Log
==========================================

.. contents:: Table of Contents
   :local:
   :depth: 2

:Document owner: QuickLogic — EDA Tools Team
:Companion:      :ref:`vpr_gui_test_plan`

This is the canonical log of real defects exposed by the VPR Qt GUI test
suite. Every entry is paired with a GitHub issue in the repository that
owns the fix.

The policy that governs this log — failing tests are not modified to
pass, every suppression cites a ``DEF-NNN``, goldens are evidence not
authority — is defined in :ref:`vpr_gui_test_plan` §4 and §9.


Severity legend
---------------

.. list-table::
   :widths: 15 85
   :header-rows: 1

   * - Severity
     - Criteria
   * - Critical
     - Crash, data corruption, or test-suite blocker on a primary
       platform.
   * - Major
     - Wrong output on a supported configuration; primary user path
       broken.
   * - Minor
     - Reproducible deviation from specification with a known
       workaround.
   * - Cosmetic
     - Visual polish, naming, or documentation only.


Entry conventions
-----------------

Each entry records:

* **Severity** and **Component**.
* **File(s)** where the defect lives.
* **Found by** — the test name or script that exposes it.
* **Symptom** — observed behaviour.
* **Expected** — what should happen.
* **GitHub issue** — link to the canonical ticket.
* **Status** — *Open*, *Fixed (commit ref)*, or *Won't-Fix (rationale)*.


Defects
-------

DEF-001 — ``QtGladeLoader`` ignores the ``sensitive`` Glade property
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity:  Minor
:Component: ``ezgl::QtGladeLoader``
:File(s):   ``libs/EXTERNAL/libezgl/src/qt/qtgladeloader.cpp`` —
            ``applyCommonProperties()``
:Found by:  ``test_qtgladeloader.cpp`` —
            *"sensitive=False disables widget"*
:Symptom:   16 widgets in ``main.ui`` carry
            ``<property name="sensitive">False</property>`` but remain
            enabled in the Qt widget tree.
:Expected:  Widgets with ``sensitive=False`` should call
            ``setEnabled(false)`` so ``isEnabled()`` returns ``false``.
:GitHub issue: (not filed — internal triage; subset of the
                ``QtGladeLoader`` cleanup tracked under
                vtr-verilog-to-routing-QL#23)
:Status:    Open


DEF-002 — ``QtGladeLoader::loadFile`` calls ``std::exit(1)`` on failure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity:  Major
:Component: ``ezgl::QtGladeLoader``
:File(s):   ``libs/EXTERNAL/libezgl/src/qt/qtgladeloader.cpp`` —
            ``loadFile()``
:Found by:  ``test_qtgladeloader.cpp`` —
            *"loadFile returns null for nonexistent file"*
:Symptom:   On a missing or empty path, ``loadFile()`` calls
            ``std::exit(1)`` and tears down the host process; callers
            cannot handle the error and the test binary cannot continue.
:Expected:  ``loadFile()`` returns ``nullptr`` on file-open failure so
            the caller can react.
:GitHub issue: `ezgl-QL#10
   <https://github.com/QL-Proprietary/ezgl-QL/issues/10>`_
:Status:    Open


DEF-003 — ``g_return_if_fail`` macros call ``std::exit(1)``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity:  Minor
:Component: ``ezgl_qtcompat``
:File(s):   ``libs/EXTERNAL/libezgl/include/ezgl/qt/ezgl_qtcompat.hpp``
:Found by:  ``test_ezgl_qtcompat.cpp`` —
            *"g_return_if_fail returns on false condition"*
:Symptom:   The GTK-compatibility macros ``g_return_if_fail`` and
            ``g_return_val_if_fail`` call ``std::exit(1)`` before the
            ``return`` statement when the condition fails — terminating
            the host process instead of warning and returning as the
            real GTK macros do.
:Expected:  Print a warning and ``return`` (or ``return val``).
:GitHub issue: `ezgl-QL#11
   <https://github.com/QL-Proprietary/ezgl-QL/issues/11>`_
:Status:    Open


DEF-004 — ``save_graphics`` aborts under ``QT_QPA_PLATFORM=offscreen``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity:  Critical
:Component: Canvas / ``save_graphics``
:File(s):   ``vpr/src/draw/save_graphics.cpp``;
            ``libs/EXTERNAL/libezgl/src/qt/canvas.cpp`` — ``print_png``
:Found by:  ``run_headless_smoke.sh`` — ``smoke_save_png``
:Symptom:   Running ``vpr ... --graphics_commands "save_graphics out.png;
            exit 0"`` under ``QT_QPA_PLATFORM=offscreen`` triggers a
            ``QPainter::setFont: Painter not active`` cascade,
            ``print_png`` returns ``false``, and a ``VTR_ASSERT``
            aborts the process.
:Expected:  ``save_graphics`` produces a valid PNG and returns ``true``.
:GitHub issue: `ezgl-QL#9
   <https://github.com/QL-Proprietary/ezgl-QL/issues/9>`_
   (tracker: `vtr-verilog-to-routing-QL#27
   <https://github.com/QL-Proprietary/vtr-verilog-to-routing-QL/issues/27>`_)
:Status:    Open — Layer 5 scenes that exercise ``save_graphics`` are
            allowed-failure pending the upstream fix.


DEF-005 — ``act_on_mouse_move`` segfaults on a null ``ezgl::application``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity:  Critical
:Component: ``vpr/src/draw`` callback dispatch
:File(s):   ``vpr/src/draw/draw.cpp`` — ``act_on_mouse_move``
:Found by:  ``test_mouse_events.cpp`` —
            *"act_on_mouse_move with null application returns cleanly"*;
            also reproducible interactively on macOS / Qt 6.11
            (`vtr-verilog-to-routing-QL#26
            <https://github.com/QL-Proprietary/vtr-verilog-to-routing-QL/issues/26>`_).
:Symptom:   Under the offscreen Qt platform the ezgl event loop
            dispatches mouse-move callbacks before the
            ``ezgl::application`` pointer is attached; the production
            callback dereferences ``app`` unconditionally and segfaults.
:Expected:  The callback is a safe no-op when the application is not
            yet wired up.
:GitHub issue: `vtr-verilog-to-routing-QL#28
   <https://github.com/QL-Proprietary/vtr-verilog-to-routing-QL/issues/28>`_
   (root-cause: `ezgl-QL#13
   <https://github.com/QL-Proprietary/ezgl-QL/issues/13>`_)
:Status:    Symptom-guarded in ``draw.cpp``; root-cause tracked in
            ezgl-QL#13.


DEF-006 — ``run_vtr_flow.py`` fails on macOS — hard-coded ``time -v``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity:  Major
:Component: VTR flow scripts
:File(s):   ``vtr_flow/scripts/python_libs/vtr/util.py``
:Found by:  Any ``run_vtr_flow.py`` invocation on macOS.
:Symptom:   ``util.py`` hard-codes ``["/usr/bin/env", "time", "-v"]``;
            BSD ``time`` does not support ``-v`` and rejects every
            invocation, so ``vpr`` is never started.
:Expected:  Probe for GNU ``time`` (``gtime`` or ``time -v``) at runtime
            and disable memory tracking gracefully when neither works.
:GitHub issue: `vtr-verilog-to-routing-QL#29
   <https://github.com/QL-Proprietary/vtr-verilog-to-routing-QL/issues/29>`_
:Status:    Mitigated locally (install scripts provision ``gtime``;
            ``util.py`` probes at runtime). Tracked for upstream merge.


DEF-007 — ``-track_memory_usage`` cannot be disabled from the CLI
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity:  Minor
:Component: VTR flow scripts
:File(s):   ``vtr_flow/scripts/run_vtr_flow.py``
:Found by:  Same reproducer as DEF-006 plus
            ``-track_memory_usage off`` (rejected as an unknown
            positional argument).
:Symptom:   ``-track_memory_usage`` is declared as ``store_true`` with
            no companion ``-no_track_memory_usage``; users have no CLI
            escape hatch on platforms where GNU ``time`` is absent.
:Expected:  A paired ``-no_track_memory_usage`` flag (or
            ``BooleanOptionalAction``).
:GitHub issue: `vtr-verilog-to-routing-QL#30
   <https://github.com/QL-Proprietary/vtr-verilog-to-routing-QL/issues/30>`_
:Status:    Mitigated locally (paired flag exists in this branch);
            tracked for upstream merge.


DEF-008 — ``--disp on`` under offscreen Qt hangs and ignores ``--graphics_commands``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity:  Major
:Component: ezgl event loop / VPR graphics-commands dispatcher
:File(s):   ``vpr/src/draw/`` and
            ``libs/EXTERNAL/libezgl/src/qt/application.cpp``
:Found by:  Layer 1 smoke probe for stateful rendering under
            ``QT_QPA_PLATFORM=offscreen``.
:Symptom:   With ``--disp on`` and the offscreen Qt platform, ``vpr``
            blocks indefinitely after placement; the scripted
            ``--graphics_commands`` is never dispatched. The same
            command with ``--disp off`` exits cleanly.
:Expected:  ``--graphics_commands`` is dispatched once the event loop
            is entered, regardless of whether external windowing
            events arrive.
:GitHub issue: `vtr-verilog-to-routing-QL#31
   <https://github.com/QL-Proprietary/vtr-verilog-to-routing-QL/issues/31>`_
   (root-cause: `ezgl-QL#13
   <https://github.com/QL-Proprietary/ezgl-QL/issues/13>`_)
:Status:    Open — gates fully-rendered Layer 5 scenes; affected
            tasks pin ``--disp off`` until fixed.


DEF-009 — ``set_nets`` / ``set_congestion`` / ``set_cpd`` are no-ops under ``--disp off``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity:  Major
:Component: GUI state setters / offscreen renderer
:File(s):   ``vpr/src/draw/`` state setters;
            ``libs/EXTERNAL/libezgl/src/qt/`` renderer
:Found by:  Layer 5 visual regression — sequential ``save_graphics``
            after each state setter produces byte-identical PNGs.
:Symptom:   The state-setter calls are accepted by the parser and the
            ``Saving to *.png`` log line is printed, but the saved
            images do not change. Four of six committed goldens are
            byte-identical for the same reason.
:Expected:  Each state change marks the canvas dirty, propagates
            through ``vpr_qtcompat``, and influences the offscreen
            render pass.
:GitHub issue: `vtr-verilog-to-routing-QL#32
   <https://github.com/QL-Proprietary/vtr-verilog-to-routing-QL/issues/32>`_
   (root-cause: `ezgl-QL#14
   <https://github.com/QL-Proprietary/ezgl-QL/issues/14>`_)
:Status:    Open — invalidates four of the six Layer 5 scenes; goldens
            are *not* regenerated until the dispatch path is fixed.


DEF-010 — Layer 5 goldens stale after renderer fixes (SSIM 0.946 — 0.991)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity:  Minor
:Component: Visual-regression test asset
:File(s):   ``vpr/test/gui/golden/*.png``;
            ``vpr/test/gui/run_visual_regression.sh``
:Found by:  ``make gui-tests-visual``.
:Symptom:   The originally-committed goldens fell 0.008 — 0.054 below
            the 0.999 SSIM threshold across all six scenes. Renderer
            commits landed after the goldens were captured (notably
            ``e4ae7fe73`` *save_graphics: use current camera world
            aspect for output dimensions*, ``50bad1b70`` arrow-helper
            API split, and ``e09f37322`` ``DEFAULT_ARROW_SIZE`` in
            pixel units) intentionally changed ``save_graphics`` output;
            the diff is structural (shifted I/O-pad rectangles, label
            offsets), not platform anti-aliasing drift.
:Expected:  Goldens regenerated against the current renderer; SSIM 1.0
            self-comparison; threshold remains 0.999.
:GitHub issue: `vtr-verilog-to-routing-QL#33
   <https://github.com/QL-Proprietary/vtr-verilog-to-routing-QL/issues/33>`_
:Status:    Fixed — goldens regenerated; ``make gui-tests-visual``
            passes 6/6 at SSIM 1.000 on macOS 14.
:Notes:     Linux cross-platform delta to be measured next; if non-zero,
            either commit per-platform goldens or relax the threshold
            to the measured floor (plan §10 R3).


DEF-011 — ``run_vtr_flow.py`` reports failure when ``exit 0`` lands during channel-width search
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity:  Minor
:Component: VTR flow scripts (end-of-flow checker)
:File(s):   ``vtr_flow/scripts/run_vtr_flow.py``
:Found by:  ``vtr_reg_gui`` tasks that use
            ``--graphics_commands "...; exit 0"`` without a fixed
            ``--route_chan_width``.
:Symptom:   ``vpr`` exits 0 (graphics commands ran cleanly) but the
            harness parses the log, sees no minimum-channel-width
            convergence message, and reports the run as failed.
:Expected:  The harness recognises a user-issued ``exit 0`` from
            ``--graphics_commands`` as a clean termination and skips
            the convergence assertion.
:GitHub issue: `vtr-verilog-to-routing-QL#34
   <https://github.com/QL-Proprietary/vtr-verilog-to-routing-QL/issues/34>`_
:Status:    Mitigated in ``vtr_reg_gui`` task configs (each pins
            ``--route_chan_width``); tracked for upstream fix.


DEF-012 — ``act_on_key_press`` / ``act_on_mouse_press`` lack the DEF-005 symmetric guard
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity:  Minor
:Component: ``vpr/src/draw`` callback dispatch
:File(s):   ``vpr/src/draw/draw.cpp`` — ``act_on_key_press``,
            ``act_on_mouse_press``
:Found by:  ``test_mouse_events.cpp`` (Layer 4 — direct dispatch case
            documented as a placeholder pending the symmetric guard).
:Symptom:   Both sibling callbacks dereference ``app`` unconditionally;
            if ezgl ever mis-dispatches them outside an active VPR
            draw lifecycle (the same root cause as DEF-005), the
            process will segfault rather than early-return.
:Expected:  The same defensive null-check pattern that DEF-005 applies
            to ``act_on_mouse_move``.
:GitHub issue: `vtr-verilog-to-routing-QL#35
   <https://github.com/QL-Proprietary/vtr-verilog-to-routing-QL/issues/35>`_
:Status:    Open — to be fixed alongside the DEF-005 root cause.
