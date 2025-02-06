.. _run_vtr_flow:

run_vtr_flow
---------------

This script runs the VTR flow for a single benchmark circuit and architecture file.

The script is located at::

    $VTR_ROOT/vtr_flow/scripts/run_vtr_flow.py

.. program:: run_vtr_flow.py

Basic Usage
~~~~~~~~~~~

At a minimum ``run_vtr_flow.py`` requires two command-line arguments::

    run_vtr_flow.py <circuit_file> <architecture_file>

where:

  * ``<circuit_file>`` is the circuit to be processed
  * ``<architecture_file>`` is the target :ref:`FPGA architecture <fpga_architecture_description>`


.. note::
    The script will create a ``./temp`` directory, unless otherwise specified with the :option:`-temp_dir` option.
    The circuit file and architecture file will be copied to the temporary directory.
    All stages of the flow will be run within this directory.
    Several intermediate files will be generated and deleted upon completion.
    **Users should ensure that no important files are kept in this directory as they may be deleted.**

Output
~~~~~~
The standard out of the script will produce a single line with the format::

    <architecture>/<circuit_name>...<status>

If execution completed successfully the status will be 'OK'. Otherwise, the status will indicate which stage of execution failed.

The script will also produce an output files (\*.out) for each stage, containing the standout output of the executable(s).

Advanced Usage
~~~~~~~~~~~~~~

Additional *optional* command arguments can also be passed to ``run_vtr_flow.py``::

    run_vtr_flow.py <circuit_file> <architecture_file> [<options>] [<vpr_options>]

where:

  * ``<options>`` are additional arguments passed to ``run_vtr_flow.py`` (described below),
  * ``<vpr_options>`` are any arguments not recognized by ``run_vtr_flow.py``. These will be forwarded to VPR.

For example::

   run_vtr_flow.py my_circuit.v my_arch.xml -track_memory_usage --pack --place

will run the VTR flow to map the circuit ``my_circuit.v`` onto the architecture ``my_arch.xml``; the arguments ``--pack`` and ``--place`` will be passed to VPR (since they are unrecognized arguments to ``run_vtr_flow.py``).
They will cause VPR to perform only :ref:`packing and placement <general_options>`.

.. code-block:: bash

    # Using the Yosys conventional Verilog parser
    ./run_vtr_flow <path/to/Verilog/File> <path/to/arch/file>

    # Using the Yosys-SystemVerilog plugin if installed, otherwise the Yosys conventional Verilog parser
    ./run_vtr_flow <path/to/SystemVerilog/File> <path/to/arch/file> -parser system-verilog

Running the VTR flow with the default configuration using the Yosys standalone front-end.
The parser for these runs is considered the Yosys conventional Verilog/SystemVerilog parser (i.e., ``read_verilog -sv``), as the parser is not explicitly specified.

.. code-block:: bash

    # Using the Synlig System_Verilog tool if installed, otherwise the Yosys conventional Verilog parser
    ./run_vtr_flow <path/to/SystemVerilog/File> <path/to/arch/file> -parser system-verilog

    # Using the Surelog plugin if installed, otherwise failure on the unsupported file type
    ./run_vtr_flow <path/to/UHDM/File> <path/to/arch/file> -parser surelog

Running the default VTR flow using the Parmys standalone front-end.
The Synlig HDL parser supports the (i.e., ``read_systemverilog``) and (i.e., ``read_uhdm``) commands. It utilizes Surelog for SystemVerilog 2017 processing and Yosys for synthesis.
Enable Synlig tool with the ``-DSYNLIG_SYSTEMVERILOG=ON`` compile flag for the Parmys front-end.

.. code-block:: bash

    # Using the Parmys (Partial Mapper for Yosys) plugin as partial mapper
    ./run_vtr_flow <path/to/Verilog/File> <path/to/arch/file>

Will run the VTR flow (default configuration) with Yosys frontend using Parmys plugin as partial mapper. To utilize the Parmys plugin, the ``-DYOSYS_PARMYS_PLUGIN=ON`` compile flag should be passed while building the VTR project with Yosys as a frontend.

Detailed Command-line Options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. note:: Any options not recognized by this script is forwarded to VPR.

.. option:: -starting_stage <stage>

    Start the VTR flow at the specified stage.

    Accepted values:

      * ``odin``
      * ``parmys``
      * ``abc``
      * ``scripts``
      * ``vpr``

    **Default:** ``parmys``

.. option:: -ending_stage <stage>

    End the VTR flow at the specified stage.


    Accepted values:

      * ``odin``
      * ``parmys``
      * ``abc``
      * ``scripts``
      * ``vpr``

    **Default:** ``vpr``

.. option:: -power

    Enables power estimation.

    See :ref:`power_estimation`

.. option:: -cmos_tech <file>

    CMOS technology XML file.

    See :ref:`power_technology_properties`

.. option:: -delete_intermediate_files

    Delete intermediate files (i.e. ``.dot``, ``.xml``, ``.rc``, etc)

.. option:: -delete_result_files

    Delete result files (i.e. VPR's ``.net``, ``.place``, ``.route`` outputs)

.. option:: -track_memory_usage

    Record peak memory usage and additional statistics for each stage.

    .. note::
        Requires ``/usr/bin/time -v`` command.
        Some operating systems do not report peak memory.

    **Default:** off

.. option:: -limit_memory_usage

    Kill benchmark if it is taking up too much memory to avoid slow disk swaps.

    .. note:: Requires ``ulimit -Sv`` command.

    **Default:** off
.. option:: -timeout <float>

    Maximum amount of time to spend on a single stage of a task in seconds.

    **Default:** 14 days

.. option:: -temp_dir <path>

    Temporary directory used for execution and intermediate files.
    The script will automatically create this directory if necessary.

    **Default:** ``./temp``

.. option:: -valgrind

    Run the flow with valgrind while using the following valgrind
    options:

        * --leak-check=full
        * --errors-for-leak-kinds=none
        * --error-exitcode=1
        * --track-origins=yes

.. option:: -min_hard_mult_size <int>

    Tells Parmys/ODIN II the minimum multiplier size that should be implemented
    using hard multiplier (if available). Smaller multipliers will be
    implemented using soft logic.

    **Default:** 3

.. option:: -min_hard_adder_size <int>

    Tells Parmys/ODIN II the minimum adder size that should be implemented
    using hard adders (if available). Smaller adders will be
    implemented using soft logic.

    **Default:** 1

.. option:: -adder_cin_global

    Tells Parmys/ODIN II to connect the first cin in an adder/subtractor chain
    to a global gnd/vdd net. Instead of creating a dummy adder to generate
    the input signal of the first cin port of the chain.

.. option:: -odin_xml <path_to_custom_xml>

    Tells VTR flow to use a custom ODIN II configuration value. The default
    behavior is to use the vtr_flow/misc/basic_odin_config_split.xml. 
    Instead, an alternative config file might be supplied; compare the 
    default and vtr_flow/misc/custom_odin_config_no_mults.xml for usage 
    scenarios. This option is needed for running the entire VTR flow with 
    additional parameters for ODIN II that are provided from within the 
    .xml file.

.. option:: -use_odin_simulation 
    
    Tells ODIN II to run simulation.

.. option:: -min_hard_mult_size <min_hard_mult_size>
    
    Tells Parmys/ODIN II the minimum multiplier size (in bits) to be implemented using hard multiplier.
    
    **Default:** 3

.. option:: -min_hard_adder_size <MIN_HARD_ADDER_SIZE>
    
    Tells Parmys/ODIN II the minimum adder size (in bits) that should be implemented using hard adder.
    
    **Default:** 1

.. option:: -top_module <TOP_MODULE>
    
    Specifies the name of the module in the design that should be considered as top

.. option:: -yosys_script <YOSYS_SCRIPT>
    
    Supplies Parmys(Yosys) with a .ys script file (similar to Tcl script), including the synthesis steps.
    
    **Default:** None

.. option:: -parser <PARSER>

    Specify a parser for the Yosys synthesizer [default (Verilog-2005), surelog (UHDM), system-verilog].
    The script uses the default conventional Verilog parser if this argument is not used.
    
    **Default:** default

.. note::

    Universal Hardware Data Model (UHDM) is a complete modeling of the IEEE SystemVerilog Object Model with VPI Interface, Elaborator, Serialization, Visitor and Listener.
    UHDM is used as a compiled interchange format in between SystemVerilog tools. Typical inputs to the UHDM flow are files with ``.v`` or ``.sv`` extensions.
    The ``system-verilog`` parser, which represents the ``read_systemverilog`` command, reads SystemVerilog files directly in Yosys.
    It executes Surelog with provided filenames and converts them (in memory) into UHDM file. Then, this UHDM file is converted into Yosys AST. `[Yosys-SystemVerilog] <https://github.com/antmicro/yosys-systemverilog#usage>`_
    On the other hand, the ``surelog`` parser, which uses the ``read_uhdm`` Yosys command, walks the design tree and converts its nodes into Yosys AST nodes using Surelog. `[UHDM-Yosys <https://github.com/chipsalliance/UHDM-integration-tests#uhdm-yosys>`_, `Surelog] <https://github.com/chipsalliance/Surelog#surelog>`_

.. note::

    Parmys is a Yosys plugin which provides intelligent partial mapping features (inference, binding, and hard/soft logic trade-offs) from Odin-II for Yosys. For more information on available paramters see the `Parmys <https://github.com/CAS-Atlantic/parmys-plugin.git>`_ plugin page.
