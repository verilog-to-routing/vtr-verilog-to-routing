.. _vpr_gui_defect_log:

VPR GUI Qt Migration — Defect Log
==================================

This document tracks real issues discovered during test implementation
for the VPR GUI Qt migration (``qt_layer`` branch).  When a test exposes
a bug or quality concern in the production EZGL / VPR GUI code, the issue
is logged here **instead of modifying the test to work around it**.

The objective is to ensure the Qt implementation is robust, correct, and
production-quality.

.. contents:: Table of Contents
   :depth: 2
   :local:

How to use this log
-------------------

Each entry records:

* **ID** — sequential (DEF-001, DEF-002, …)
* **Severity** — Critical / Major / Minor / Cosmetic
* **Component** — e.g. ``ezgl::renderer``, ``deferred_renderer``, ``camera``, VPR widget
* **File(s)** — source file(s) where the defect lives
* **Found by** — test file and test-case name
* **Description** — what the test revealed
* **Expected behaviour** — what should happen
* **Actual behaviour** — what happens instead
* **Status** — Open / Fixed (with commit ref) / Won't-Fix (with rationale)

Severity definitions
^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 15 85

   * - Severity
     - Criteria
   * - Critical
     - Crash, data corruption, or security issue; blocks release.
   * - Major
     - Incorrect visual output or broken user interaction; must fix before production.
   * - Minor
     - Deviation from specification that does not block functionality.
   * - Cosmetic
     - Visual polish, naming inconsistency, or documentation gap.

Defects
-------

DEF-001 — QtGladeLoader ignores ``sensitive`` property
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity: Minor
:Component: ``QtGladeLoader``
:File(s): ``libs/EXTERNAL/libezgl/src/qt/qtgladeloader.cpp`` — ``applyCommonProperties()``
:Found by: ``test_qtgladeloader.cpp`` — "QtGladeLoader: sensitive=False disables widget"
:Status: Open

**Description:**
The ``applyCommonProperties()`` method in QtGladeLoader handles ``visible`` and ``can-focus`` but does not handle the ``sensitive`` Glade property.  16 widgets in ``main.ui`` have ``<property name="sensitive">False</property>`` but remain enabled in the Qt widget tree.

**Expected:**
Widgets with ``sensitive=False`` should have ``setEnabled(false)`` called, so ``isEnabled()`` returns ``false``.

**Actual:**
All widgets are enabled regardless of the ``sensitive`` property in the Glade XML.

DEF-002 — QtGladeLoader calls ``std::exit(1)`` on file-open failure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity: Major
:Component: ``QtGladeLoader``
:File(s): ``libs/EXTERNAL/libezgl/src/qt/qtgladeloader.cpp`` — ``loadFile()``
:Found by: ``test_qtgladeloader.cpp`` — "QtGladeLoader: loadFile returns null for nonexistent file"
:Status: Open

**Description:**
When ``loadFile()`` cannot open the file (nonexistent path, empty string), it calls ``std::exit(1)`` instead of returning ``nullptr``.  This makes error handling impossible for callers and prevents testing of error paths.

**Expected:**
``loadFile()`` should return ``nullptr`` on file-open failure, allowing the caller to handle the error gracefully.

**Actual:**
The process terminates immediately via ``std::exit(1)``.

DEF-003 — ``g_return_if_fail`` / ``g_return_val_if_fail`` call ``std::exit(1)``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity: Minor
:Component: ``ezgl_qtcompat``
:File(s): ``libs/EXTERNAL/libezgl/include/ezgl/qt/ezgl_qtcompat.hpp``
:Found by: EZGL Layer 2 ``test_ezgl_qtcompat.cpp`` — "ezgl_qtcompat: g_return_if_fail returns on false condition"
:Status: Open

**Description:**
The GTK compatibility macros ``g_return_if_fail`` and ``g_return_val_if_fail`` call ``std::exit(1)`` before their ``return`` statement when the condition fails.  The original GTK macros only print a warning and return — they do not terminate the process.

**Expected:**
On condition failure: print a warning/critical message and ``return`` (or ``return val``), matching GTK behaviour.

**Actual:**
On condition failure: prints a message, calls ``std::exit(1)``, then has an unreachable ``return`` statement.

DEF-004 — ``save_graphics`` / ``print_png`` crashes in Qt offscreen mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Severity: Critical
:Component: Canvas / ``save_graphics``
:File(s): ``vpr/src/draw/save_graphics.cpp``, EZGL canvas ``print_png``
:Found by: ``run_headless_smoke.sh`` — smoke_save_png
:Status: Open

**Description:**
Running VPR with ``QT_QPA_PLATFORM=offscreen --graphics_commands "save_graphics output.png; exit 0"`` causes a ``QPainter::setFont: Painter not active`` cascade followed by a ``VTR_ASSERT`` failure (``result == true``) in ``save_graphics()``.  The deferred renderer reports only 4 text primitives and zero geometry (``lines=0 fill_rects=0 fill_polys=0``).  The canvas ``print_png`` returns ``false`` because the ``QPainter`` device is not properly initialised in offscreen mode.

**Expected:**
``save_graphics`` should produce a valid 2048-px wide PNG file and return ``true`` when running under ``QT_QPA_PLATFORM=offscreen``.

**Actual:**
``QPainter`` operations fail with "Painter not active" warnings; ``print_png`` returns ``false``; ``VTR_ASSERT`` fires, aborting the process (exit 134 / SIGABRT).

.. Template for new entries:
..
.. DEF-NNN — <short title>
.. ^^^^^^^^^^^^^^^^^^^^^^^^
..
.. :Severity: Major
.. :Component: ``ezgl::renderer``
.. :File(s): ``src/graphics.cpp``
.. :Found by: ``test_renderer.cpp`` — "renderer draw_line clips correctly"
.. :Status: Open
..
.. **Description:**
.. <What the test revealed.>
..
.. **Expected:**
.. <What should happen.>
..
.. **Actual:**
.. <What happens instead.>
