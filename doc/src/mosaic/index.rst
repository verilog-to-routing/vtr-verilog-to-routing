.. _mosaic:

######
Mosaic
######

.. figure:: mosaic.png
    :align: center
    :width: 90%
    :alt: Mosaic logo

Mosaic is a Yosys-based synthesis frontend for VTR. It elaborates Verilog, maps inferred operators onto architecture hardblocks, and performs LUT mapping inside Yosys. The name refers to template-backed technology mapping: shared mapping templates are filled from the architecture XML at run time, and optional per-architecture overlays replace only the pieces that need to differ.

Select Mosaic with ``-start mosaic`` on :ref:`run_vtr_flow`. Parmys remains the default frontend. Mosaic writes a LUT-mapped BLIF (``<circuit>.mosaic.blif``), so the flow skips the external ABC stage and continues into VPR.

The default VTR build includes Mosaic (``WITH_MOSAIC``, default ``ON``). The plugin is installed at ``build/share/yosys/plugins/mosaic.so``.

.. toctree::
   :maxdepth: 2

   users
   developers
