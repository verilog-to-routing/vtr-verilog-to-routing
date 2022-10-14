.. _quickstart:

Quickstart
==========

Prerequisites
-------------

* ctags
* bison
* flex
* gcc 5.x
* cmake 3.9 (minimum version)
* time
* cairo
* gawk
* xdot
* tcl-dev
* graphviz
* pkg-config
* python3
* libffi-dev
* libreadline-dev
* libboost-system-dev
* libboost-python-dev
* libboost-filesystem-dev
* zlib1g-dev

Building
--------

To build the VTR flow with the Yosys front-end you may use the VTR Makefile wrapper, by calling the ``make CMAKE_PARAMS="-DWITH_YOSYS=ON"`` command in the `$VTR_ROOT` directory.
The compile flag ``-DWITH_YOSYS=ON`` should be passed to the CMake parameters to enable the Yosys compilation process.

.. note::

    Compiling the VTR flow with the ``-DYOSYS_SV_UHDM_PLUGIN=ON`` flag is required to build and install Yosys SystemVerilog and UHDM plugins.
    Using this compile flag, the `Yosys-F4PGA-Plugins <https://github.com/chipsalliance/yosys-f4pga-plugins>`_ and `Surelog <https://github.com/chipsalliance/Surelog>`_ repositories are cloned in the ``$VTR_ROOT/libs/EXTERNAL`` directory and then will be compiled and added as external plugins to the Yosys front-end.

 
.. note::

	Yosys uses Makefile as its build system. Since CMake provides portable, cross-platform build systems with many useful features, we provide a CMake wrapper to successfully embeds the Yosys library into the VTR flow.


Basic Usage
-----------

To run the VTR flow with the Yosys front-end, you would need to run the `run_vtr_flow.py <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/scripts/run_vtr_flow.py>`_ script with the start stage specified as `Yosys`.

.. code-block:: bash

    ./run_vtr_flow `PATH_TO_VERILOG_FILE.v` `PATH_TO_ARCH_FILE.xml` -start yosys

.. note::

    Please see `Run VTR Flow <https://docs.verilogtorouting.org/en/latest/vtr/run_vtr_flow/#advanced-usage>`_ for advanced usage of the Yosys front-end with external plugins.
