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
The compile flag ``-DWITH_YOSYS=ON`` should be passed to the CMake parameters to enable Yosys compilation process.
 
.. note::

	Yosys uses Makefile as its build system. Since CMake provides portable, cross-platform build systems with many useful features, we provide a CMake wrapper to successfully embeds the Yosys library into the VTR flow.


Basic Usage
-----------

To run the VTR flow with the Yosys front-end, you would need to run the `run_vtr_flow.py <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/scripts/run_vtr_flow.py>`_ script with the start stage specified as `Yosys`.

.. code-block:: bash

    ./run_vtr_flow `PATH_TO_VERILOG_FILE.v` `PATH_TO_ARCH_FILE.xml` -start yosys