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


DEF-013 — ``search_and_highlight`` segfaults on a null ``ezgl::application``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity:  Minor
:Component: ``vpr/src/draw`` search subsystem
:File(s):   ``vpr/src/draw/search_bar.cpp`` — ``search_and_highlight``
            (lines 50–58, the ``find_line_edit`` lookup and the next
            ``text_entry->text()`` deref).
:Found by:  ``test_search_null_guard.cpp`` (GUI-T-019) —
            *"search_and_highlight(nullptr) returns cleanly
            [symmetric-guard contract — DEF-013]"*.
:Symptom:   ``search_and_highlight`` looks up
            ``text_entry = app->find_line_edit("TextInput")`` and then
            unconditionally calls ``text_entry->text().toStdString()``
            on the next line. When ``app`` is null (or the lookup
            misses, e.g. because main.ui is not loaded) ``text_entry``
            is ``nullptr`` and the deref crashes the process with
            SIGSEGV. The same code path is reached on the regular
            click-to-search wiring; it has not been seen in production
            because the search button is connected through
            ``ui_setup.cpp::basic_button_setup`` only after main.ui is
            loaded, but the asymmetry with DEF-005 / DEF-012 means the
            crash will surface the moment the dispatcher mis-fires
            once.
:Expected:  ``search_and_highlight`` returns cleanly when ``app`` is
            null or the ``TextInput`` lookup misses, mirroring the
            DEF-005 guard pattern. The two sibling entry points
            (``enable_autocomplete``, ``search_type_changed``) already
            satisfy the contract via their existing
            ``if (!searchBar) return;`` lines; this entry only covers
            the asymmetry in ``search_and_highlight``.
:GitHub issue: `vtr-verilog-to-routing-QL#40
   <https://github.com/QL-Proprietary/vtr-verilog-to-routing-QL/issues/40>`_
:Status:    Open — failing case is tagged ``[!shouldfail]`` with an
            inline DEF-013 citation per §9 of
            :ref:`vpr_gui_test_plan`. The annotation is removed in the
            same PR that lands the symmetric guard.


DEF-014 — ``QtGladeLoader`` leaks GtkPopover top-level ``QFrame``\ s into ``QApplication::allWidgets()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity:  Minor (test-infrastructure only — **not a blocker**;
            unobservable in production where VPR constructs a single
            ``ezgl::application`` and never destroys + re-loads
            ``main.ui`` in the same process).
:Component: ``libs/EXTERNAL/libezgl`` Glade-to-Qt UI loader.
:File(s):   ``libs/EXTERNAL/libezgl/include/ezgl/qt/qtgladeloader.hpp``,
            ``libs/EXTERNAL/libezgl/src/qt/qtgladeloader.cpp``.
            Consumer that surfaces the leak:
            ``vpr/test/gui/test_gui_helpers.hpp::findWidgetByName<T>``
            (mirrors ``ezgl::application::find_widget`` which
            iterates ``QApplication::allWidgets()``).
:Found by:  Wave 2 GUI-T-008 implementation of
            ``vpr/test/gui/test_stage_gating.cpp``. Initial
            implementation flaked the *unrelated*
            ``Flow: Net popover has expected controls`` case in
            ``test_gui_flow.cpp`` after the new T-008 cases mutated
            ``ToggleNetType`` (``netTypeCombo->count() == 1``,
            expected 2). Failure manifested only when T-008 cases
            ran before the flow case; ``QApplication::allWidgets()``
            inspection confirmed multiple ``ToggleNetType`` widgets
            present after one ``loadFile`` + ``QMainWindow`` destroy.
:Symptom:   ``QtGladeLoader::loadFile`` creates GtkPopover-converted
            widgets as orphan top-level ``Qt::Popup`` ``QFrame``\ s
            **not parented** to the loaded ``QMainWindow``. When the
            ``QMainWindow`` is destroyed those popovers (and their
            child ``QComboBox`` / ``QLineEdit`` / ``QSpinBox``
            widgets) persist in ``QApplication::allWidgets()`` and
            leak mutated state into anything that subsequently
            iterates the global widget set —
            ``ezgl::application::find_widget`` and our test helper
            ``findWidgetByName<T>`` are exactly such consumers.
:Expected:  ``QtGladeLoader::loadFile`` returns a widget tree whose
            destruction releases every widget the loader created.
            Concretely: any orphan top-level ``Qt::Popup``
            ``QFrame``\ s the loader produces should be reparented
            to the returned root before return, so that ``QObject``
            ownership is transitive and ``delete root`` is
            sufficient teardown.
:Workaround in tree: Per-test RAII guards in
            ``vpr/test/gui/test_stage_gating.cpp``:
            ``ComboItemsGuard`` (snapshot/restore items + index) and
            ``WidgetEnabledGuard`` (snapshot/restore enabled bit).
            An aggressive sweep (delete-all-new-top-level widgets in
            ``EzglAppFixture::~EzglAppFixture``) was tried and
            **abandoned** — Qt has deferred-deletion semantics for
            some ``Qt::Popup`` frames and the sweep crashed mid-suite.
:GitHub issue: `vtr-verilog-to-routing-QL#41
   <https://github.com/QL-Proprietary/vtr-verilog-to-routing-QL/issues/41>`_
:Status:    Open — **not a blocker** for Wave 2. No ``[!shouldfail]``
            Catch2 case (the symptom is non-deterministic and a
            deterministic reproducer would require its own
            infrastructure ticket; per §4 of
            :ref:`vpr_gui_test_plan` we do not add placeholder
            coverage). Future Layer-4 tickets composing multiple
            ``QtGladeLoader::loadFile`` lifetimes should either
            reuse the in-tree RAII guards or wait for the libezgl
            fix.



DEF-015 — ``highlight_cluster_block`` probes the world origin instead of the requested block
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity:  Medium (functional — toolbar **Search** silently selects
            the wrong block / sub-pb whenever the requested cluster
            block is not the one whose pb-tree happens to cover the
            world-coordinate origin).
:Component: ``vpr/src/draw/`` Search bar / block-highlight path.
:File(s):   ``vpr/src/draw/search_bar.cpp:312-340`` —
            ``highlight_cluster_block(ClusterBlockId)``.
:Found by:  Wave 2 GUI-T-001 implementation of
            ``vpr/test/gui/test_search_dispatch.cpp``. The first
            draft of the *"Search: Block Name with a real cluster
            name selects that block"* case asserted
            ``draw_state->block_color(target) == SELECTED_COLOR``
            and failed: ``block_color(target)`` was the default
            colour while ``selected_sub_block_info`` reported a
            sub-pb of an *unrelated* cluster block as selected.
            Inspection of ``highlight_cluster_block`` exposed the
            cause below.
:Symptom:   ``highlight_cluster_block`` declares ``ezgl::rectangle
            clb_bbox;`` (zero-sized rect at the origin), **never
            assigns it**, and immediately probes
            ``point_in_clb = clb_bbox.bottom_left();`` — i.e.
            ``(0, 0)`` in world coordinates — and passes that into
            ``highlight_sub_block(point_in_clb, clb_index, ...)``.
            On any device whose grid origin contains a placed block
            with a sub-pb tree, ``highlight_sub_block`` succeeds at
            ``(0,0)`` and sets ``selected_sub_block_info``; the
            ``has_selection()`` short-circuit then skips
            ``draw_highlight_blocks_color(... clb_index)`` and the
            user-requested block is **never coloured**
            ``SELECTED_COLOR``. The status-bar message reports
            *"sub-block X selected"* for the wrong containing block.
:Expected:  ``clb_bbox`` is filled with the absolute bounding box of
            the requested ``clb_index`` *before* its corner is used
            as the sub-block probe point — e.g.::

               clb_bbox = draw_coords->get_absolute_clb_bbox(
                   clb_index, cluster_ctx.clb_nlist.block_type(clb_index));

            so the sub-block probe runs at a point inside the
            requested block, and the fall-through to
            ``draw_highlight_blocks_color(... clb_index)`` colours
            the right cluster block when no sub-pb is hit.
:Reproducer: Open VPR with ``--disp on`` on
            ``and_latch.blif`` / ``k6_N10_40nm.xml`` post-place,
            select ``Block ID`` in the search combo, type any block
            id whose location is not ``(0, 0)`` (e.g. the ``clb`` in
            this benchmark), press *Search*: status bar reads
            *"sub-block <io-pin> selected"* and the canvas
            highlights an io block, not the searched clb.
            In-tree assertion that pins the bug:
            ``vpr/test/gui/test_search_dispatch.cpp::"Search:
            Block Name with a real cluster name selects a cluster
            block"`` and ``"Search: Block ID with a valid id selects
            that cluster block"`` accept *either* ``block_color ==
            SELECTED_COLOR`` *or* ``selected_sub_block_info``
            containing the requested cluster block — both branches
            are valid resolutions of the search; the broadened
            observable will tighten to the ``block_color`` branch
            in the same PR that fixes ``highlight_cluster_block``.
:Workaround in tree: None required — the tests accept both legal
            resolutions of the search dispatcher today and a
            future tightening tracks the fix.
:GitHub issue: `vtr-verilog-to-routing-QL#42
   <https://github.com/QL-Proprietary/vtr-verilog-to-routing-QL/issues/42>`_
:Status:    Open — filed during Wave 2; fix is a one-liner in
            ``highlight_cluster_block``; tightening of the T-001
            ``Block ID`` / ``Block Name`` assertions to the
            ``block_color`` branch lands in the same PR.

DEF-016 — ``draw_noc`` dereferences ``router_list.begin()`` before checking emptiness
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity:  Major
:Component: ``vpr/src/draw`` — NoC overlay rendering
:File(s):   ``vpr/src/draw/draw_noc.cpp`` — ``draw_noc()``,
            lines 39 — 45 (the block immediately after the
            ``DRAW_NO_NOC`` early-return).
:Found by:  Wave 3 — ``test_visual_render_smoke.cpp``
            *"Visual: draw_noc with DRAW_NO_NOC is a no-op
            (baseline-equivalent)"* and accompanying inspection
            while writing the NoC-overlay coverage. The test
            deliberately *does not* enable ``draw_noc`` on the
            NoC-less ``k6_N10_40nm.xml`` benchmark for the reason
            documented in the case body — the implementation crashes
            if it does, which is exactly this defect.
:Symptom:   On any architecture without a NoC (i.e. every benchmark
            in ``vtr_flow/arch/timing``), if the user toggles
            *"Draw NoC Links"* or *"Draw NoC Link Usage"* in the
            toolbar, ``draw_noc`` runs past the ``DRAW_NO_NOC``
            early-return and immediately calls

            .. code-block:: c++

               const auto& type = device_ctx.grid.get_physical_type(
                   {router_list.begin()->get_router_grid_position_x(),
                    router_list.begin()->get_router_grid_position_y(),
                    router_list.begin()->get_router_layer_position()});

            with ``router_list`` (a
            ``vtr::vector<NocRouterId, NocRouter>``) empty.
            ``router_list.begin()`` returns ``end()``, dereferencing
            it is undefined behaviour and in practice produces a
            segfault inside ``ezgl``'s draw callback during the very
            next ``refresh_drawing()``.
:Expected:  ``draw_noc`` should treat an empty NoC router list as
            "no NoC, nothing to draw" and early-return symmetrically
            with the ``DRAW_NO_NOC`` and ``num_subtiles == 0``
            checks already in place. A two-line fix:

            .. code-block:: c++

               if (router_list.empty()) {
                   return;
               }

            inserted between the existing ``DRAW_NO_NOC`` early-return
            and the ``router_list.begin()->...`` deref closes the gap.
            The toolbar toggle then becomes a no-op on NoC-less
            archs (the user's intent on such an arch is undefined,
            but silently ignoring is safer than crashing).
:Reproducer:Open VPR with ``--disp on`` on
            ``and_latch.blif`` / ``k6_N10_40nm.xml``, advance to
            Placement, click *"Draw NoC Links"* in the toolbar →
            crash on the next canvas refresh. The same crash is
            reachable from ``--graphics_commands "draw_noc 1"`` on
            a NoC-less arch.
:Workaround in tree: The Layer-5 NoC visual case
            ``"Visual: draw_noc with DRAW_NO_NOC is a no-op
            (baseline-equivalent)"`` deliberately leaves
            ``draw_state->draw_noc`` at ``DRAW_NO_NOC`` and asserts
            the no-op gate contract; a NoC-arch fixture (future
            work) will lock the enabled-overlay path symmetrically
            once this defect is fixed.
:GitHub issue: `vtr-verilog-to-routing-QL#43
   <https://github.com/QL-Proprietary/vtr-verilog-to-routing-QL/issues/43>`_
:Status:    Open — Wave 3 finding; fix is a two-line guard in
            ``draw_noc.cpp``; the Layer-5 NoC visual coverage
            tightens to the enabled path in the same PR.
