.. _odin_II:

###########
Odin II
###########

Odin II is used for logic synthesis and elaboration, converting a subset of the Verilog Hardware Description Language (HDL) into a BLIF netlist.

.. note::
    Odin-II has been deprecated and will be removed in a future version. Now VTR uses Parmys as the default frontend which utilizes Yosys as elaborator with partial mapping features enabled.

    To build the VTR flow with the Odin-II front-end you may use the VTR Makefile wrapper, by calling the ``make CMAKE_PARAMS="-DWITH_ODIN=ON"`` command in the `$VTR_ROOT` directory.

.. toctree::
   :glob:
   :maxdepth: 2
   :Caption: Quickstart

   quickstart


.. toctree::
   :glob:
   :maxdepth: 2
   :Caption: User Guide

   user_guide

.. toctree::
   :glob:
   :maxdepth: 2
   :Caption: verilog Support

   verilog_support

.. toctree::
   :glob:
   :maxdepth: 2
   :Caption: Developer Guide

   dev_guide/contributing
   dev_guide/regression_test
   dev_guide/verify_script
   dev_guide/testing
