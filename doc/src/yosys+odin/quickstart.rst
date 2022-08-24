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

To build, you may use the Makefile wrapper in the `$VTR_ROOT/ODIN_II` directory, via the ``make build ELABORATOR=yosys`` command.
As Yosys is added as an external library to the VTR project, the debug option is only available for Odin-II.
To build with debug mode, you may need to pass the debug flag to the CMake parameters using the previous Makefile wrapper, i.e., ``CMAKE_PARAMS="-DODIN_DEBUG=ON"``.
To ease this process, you can build Yosys+Odin-II with Odin-II in debug mode using the ``make debug ELABORATOR=yosys`` command.

The second approach to build the VTR flow with the Yosys+Odin-II front-end is to use the main VTR Makefile. i.e., calling ``make`` in the `$VTR_ROOT` directory.
In this approach, the compile flag ``-DODIN_USE_YOSYS=ON`` should be passed to the CMake parameters as follows: ``make CMAKE_PARAMS="-DODIN_USE_YOSYS=ON"``.

.. note::

    Compiling the VTR flow with the ``-DYOSYS_SV_UHDM_PLUGIN=ON`` flag is required to build and install Yosys SystemVerilog and UHDM plugins.
    Using this compile flag, the `Yosys-F4PGA-Plugins <https://github.com/chipsalliance/yosys-f4pga-plugins>`_ and `Surelog <https://github.com/chipsalliance/Surelog>`_ repositories are cloned in the ``$VTR_ROOT/libs/EXTERNAL`` directory and then will be compiled and added as external plugins to the Yosys front-end.

.. note::

	Yosys uses Makefile as its build system. Since CMake provides portable, cross-platform build systems with many useful features, we provide a CMake wrapper to successfully embeds the Yosys library into the VTR flow.
	The Makefile wrapper in the `$VTR_ROOT/ODIN_II` provides the build support for Unix-like systems but, in fact, calls CMake behind the scenes.

.. warning::

	Once you build Yosys+Odin-II, you would run ``make test ELABORATOR=yosys`` from the `$VTR_ROOT/ODIN_II` to simulate and verify the light and heavy benchmark suites to ensure that Yosys+Odin-II is working correctly on your system.

Basic Usage
-----------

.. code-block:: bash

    ./odin_II --elaborator yosys [arguments]

*Requires one and only one of `-c`, `-v`, `-s`, `-u` or `-b`*

.. table::

    ====  ==========================  ===================================================================================================
    Arg   Following Argument          Description
    ====  ==========================  ===================================================================================================
    `-c`  XML Configuration File      an XML configuration file dictating the runtime parameters of odin
    `-v`  Verilog HDL File            You may specify multiple space-separated Verilog HDL files                        
    `-s`  System Verilog HDL File     You may specify multiple space-separated System Verilog files                        
    `-u`  UHDM File                   You may specify multiple space-separated UHDM files (require compiling with the ``-DYOSYS_SV_UHDM_PLUGIN=ON`` flag)                        
    `-b`  BLIF File                   You may specify multiple space-separated BLIF files (require compiling with the ``-DYOSYS_SV_UHDM_PLUGIN=ON`` flag)                               
    `-o`  BLIF Output File            full output path and file name for the BLIF output file           
    `-a`  Architecture File           VPR XML architecture file. You may not specify the architecture file, which results in pure soft logic synthesis           
    `-h`                              Print help   
    ====  ==========================  ===================================================================================================


Example Usage
-------------

The following are simple command-line arguments and a description of what they do. 
It is assumed that they are being performed in the Odin-II directory.

The following commands pass a Verilog/SystemVerilog/UHDM HDL file to Yosys for elaboration, then Odin-II performs the partial mapping and optimization into pure soft logic. 
Warnings and errors may appear regarding the HDL code by Yosys.

.. code-block:: bash

    # Elaborate the input file using Yosys conventional Verilog parser and then partial map the coarse-grained netlist using Odin-II
    ./odin_II --elaborator yosys -v <path/to/Verilog/File>

    # Elaborate the input file using the Yosys-SystemVerilog plugin if installed, otherwise the Yosys conventional Verilog parser 
    # and then partial map the coarse-grained netlist using Odin-II
    ./odin_II --elaborator yosys -s <path/to/SystemVerilog/File>
    
    # Elaborate the input file using the Surelog plugin if installed, otherwise failure on the unsupported type. 
    # If succeed, then Odin-II performs the partial mapping on the coarse-grained netlist
    ./odin_II --elaborator yosys -u <path/to/UHDM/File>


.. note::

    The entire log file of the Yosys elaboration for each run is outputted into a file called ``elaboration.yosys.log`` located in the same directory of the final output BLIF file.

The following command passes a Verilog HDL file and architecture to Yosys+Odin-II, where it is synthesized.
Yosys will use the HDL files to perform elaboration.
Then, Odin-II will use the architecture to do partial technology mapping, and will output the BLIF in the current directory at ``./output.blif``.
If the output BLIF file is not specified, ``default_out.blif`` is considered the output file name, again located in the current directory.

.. code-block:: bash

   ./odin_II --elaborator yosys -v <path/to/Verilog/File> -a <path/to/arch/file> -o output.blif

.. note::
	
	Once the elaboration is fully executed, Yosys generates a coarse-grained BLIF file that the Odin-II BLIF reader will read to create a netlist. This file is named ``coarsen_netlist.yosys.blif`` located in the current directory.

The following command passes a Tcl script file, including commands for the elaboration by Yosys, along with the architecture file.

.. code-block:: bash

   ./odin_II -S <path/to/Tcl/File> -a <path/to/arch/file> -o myModel.blif

.. note::

	The Tcl script file should follow the same generic synthesis flow, brought as an example in the `$VTR_ROOT/ODIN_II/regression_test/tools/synth.tcl`.
	Also, the input HDL file should be specified in the Tcl script while using this approach.