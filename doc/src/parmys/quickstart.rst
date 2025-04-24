.. _quickstart:

Quickstart
==========

Prerequisites
-------------

* ctags
* bison
* flex
* g++ 9.x
* cmake 3.16 (minimum version)
* time
* cairo
* build-essential
* libreadline-dev
* gawk tcl-dev
* libffi-dev 
* git
* graphviz
* xdot
* pkg-config
* python3-dev
* libboost-system-dev
* libboost-python-dev
* libboost-filesystem-dev 
* zlib1g-dev

Building
--------

To build the VTR flow with the Parmys front-end you may use the VTR Makefile wrapper, by calling the ``make CMAKE_PARAMS="-DWITH_PARMYS=ON"`` command in the `$VTR_ROOT` directory.

.. note::
    Our CI testing is on Ubuntu 22.04, so that is the best tested platform and recommended for development.

.. note::

    Compiling the VTR flow with the ``-DSynlig_SystemVerilog=ON`` flag is required to build and install Synlig SystemVerilog and UHDM plugins.
    Using this compile flag, the `Synlig_SystemVerilog <https://github.com/chipsalliance/synlig>`_ repository is cloned and installed in the ``$VTR_ROOT/build/bin`` directory and then will be compiled and added as external tool to the Parmys front-end.

Basic Usage
-----------

To run the VTR flow with the Parmys front-end, you would need to run the `run_vtr_flow.py <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/scripts/run_vtr_flow.py>`_ script with the start stage specified as `parmys`.

.. code-block:: bash

    ./run_vtr_flow `PATH_TO_VERILOG_FILE.v` `PATH_TO_ARCH_FILE.xml` -start parmys

.. note::

    Please see `Run VTR Flow <https://docs.verilogtorouting.org/en/latest/vtr/run_vtr_flow/#advanced-usage>`_ for advanced usage of the Parmys front-end with external plugins.

.. note::

    Parmys is the default frontend in VTR flow which means it is no more necessary to pass build flags to cmake or explicitly define the start stage of vtr flow as `parmys`.
