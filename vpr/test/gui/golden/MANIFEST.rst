Golden Images — Visual Regression Baseline
===========================================

This directory stores the reference (golden) PNG images for Layer 5 visual
regression tests.  Each image is 2048 px wide and produced by VPR's
``save_graphics`` command under ``QT_QPA_PLATFORM=offscreen``.

**Status:** Empty — blocked by DEF-004 (``save_graphics`` / ``print_png``
crashes in Qt offscreen mode).  See ``doc/src/dev/vpr_gui_defect_log.rst``.

Expected Images
---------------

Once DEF-004 is resolved, run ``generate_goldens.sh`` to populate this
directory.  The following 6 images will be generated:

.. list-table::
   :widths: 30 20 50
   :header-rows: 1

   * - Filename
     - Circuit
     - Description
   * - ``placement_default.png``
     - ``mult_4x4.blif``
     - Default placement view after pack + place
   * - ``placement_nets.png``
     - ``mult_4x4.blif``
     - Placement with net overlays (``set_nets 1``)
   * - ``placement_congestion.png``
     - ``mult_4x4.blif``
     - Placement congestion heatmap (``set_congestion 1``)
   * - ``routing_default.png``
     - ``mult_4x4.blif``
     - Default routing view after pack + place + route
   * - ``routing_timing.png``
     - ``mult_4x4.blif``
     - Routing with critical-path display (``set_cpd 1``)
   * - ``placement_block_internals.png``
     - ``mult_4x4.blif``
     - Placement with block-internal detail level 2

Regeneration Command
--------------------

.. code-block:: bash

   ./vpr/test/gui/generate_goldens.sh \
       ./build/vpr/vpr \
       ./vtr_flow/arch/timing \
       ./vtr_flow/benchmarks/microbenchmarks

Architecture: ``k6_N10_40nm.xml``, seed: ``1``, display: ``--disp off``.

.. warning::

   Regenerating goldens invalidates all previous SSIM comparisons.
   Only regenerate when an intentional visual change is made (e.g., colour
   scheme update, layout algorithm change) and commit the new PNGs.
