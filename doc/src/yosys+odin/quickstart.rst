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

	Yosys uses Makefile as its build system. Since CMake provides portable, cross-platform build systems with many useful features, we provide a CMake wrapper to successfully embeds the Yosys library into the VTR flow.
	The Makefile wrapper in the `$VTR_ROOT/ODIN_II` provides the build support for Unix-like systems but, in fact, calls CMake behind the scenes.

.. warning::

	Once you build Yosys+Odin-II, you would run ``make test ELABORATOR=yosys`` from the `$VTR_ROOT/ODIN_II` to simulate and verify the light and heavy benchmark suites to ensure that Yosys+Odin-II is working correctly on your system.

Basic Usage
-----------

.. code-block:: bash

    ./odin_II --elaborator yosys [arguments]

*Requires one and only one of `-c`, `-V` or `-b`*

.. table::

    ====  ==========================  =======================================================================
    Arg   Following Argument          Description
    ====  ==========================  =======================================================================
    `-c`  XML Configuration File      an XML configuration file dictating the runtime parameters of odin
    `-V`  Verilog HDL FIle            You may specify multiple verilog HDL files                        
    `-b`  BLIF File                   You may specify multiple blif files                               
    `-o`  BLIF output file            full output path and file name for the blif output file           
    `-a`  architecture file           You may specify multiple verilog HDL files for synthesis          
    `-h`                              Print help   
    ====  ==========================  =======================================================================


Example Usage
-------------

The following are simple command-line arguments and a description of what they do. 
It is assumed that they are being performed in the Odin-II directory.

.. code-block:: bash

   ./odin_II --elaborator yosys -V <path/to/Verilog/File>


Passes a Verilog HDL file to Yosys for elaboration, then Odin-II performs the partial mapping and optimization. 
Warnings and errors may appear regarding the HDL code by Yosys.

.. note::

    The entire log file of the Yosys elaboration for each run is outputted into a file called ``elaboration.yosys.log`` located in the same directory of the final output BLIF file.

.. code-block:: bash

   ./odin_II --elaborator yosys -V <path/to/Verilog/File> -a <path/to/arch/file> -o output.blif

Passes a Verilog HDL file and architecture to Yosys+Odin-II, where it is synthesized.
Yosys will use the HDL files to perform elaboration.
Then, Odin-II will use the architecture to do partial technology mapping, and will output the BLIF in the current directory at ``./output.blif``.
If the output BLIF file is not specified, ``default_out.blif`` is considered the output file name, again located in the current directory.

.. note::
	
	Once the elaboration is fully executed, Yosys generates a coarse-grained BLIF file that the Odin-II BLIF reader will read to create a netlist. This file is named ``coarsen_netlist.yosys.blif`` located in the current directory.


.. code-block:: bash

   ./odin_II -S <path/to/Tcl/File> -a <path/to/arch/file> -o myModel.blif

Passes a Tcl script file, including commands for the elaboration by Yosys, along with the architecture file.

.. note::

	The Tcl script file should follow the same generic synthesis flow, brought as an example in the `$VTR_ROOT/ODIN_II/regression_test/tools/synth.tcl`.
	Also, the input HDL file should be specified in the Tcl script while using this approach.