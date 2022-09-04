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
    ./run_vtr_flow <path/to/Verilog/File> <path/to/arch/file> -elaborator yosys -fflegalize

    # Using the Yosys-SystemVerilog plugin if installed, otherwise the Yosys conventional Verilog parser
    ./run_vtr_flow <path/to/SystemVerilog/File> <path/to/arch/file> -elaborator yosys -fflegalize
    
    # Using the Surelog plugin if installed, otherwise failure on the unsupported file type
    ./run_vtr_flow <path/to/UHDM/File> <path/to/arch/file> -elaborator yosys -fflegalize

Passes a Verilog/SystemVerilog/UHDM file to Yosys to perform elaboration. 
The BLIF elaboration and partial mapping phases will be executed on the generated netlist by Odin-II, and all latches in the Yosys+Odin-II output file will be rising edge.
Then ABC and VPR perform the default behaviour for the VTR flow, respectively.

.. code-block:: bash

    # Using the Yosys conventional Verilog parser
    ./run_vtr_flow <path/to/Verilog/File> <path/to/arch/file> -start yosys

    # Using the Yosys-SystemVerilog plugin if installed, otherwise the Yosys conventional Verilog parser
    ./run_vtr_flow <path/to/SystemVerilog/File> <path/to/arch/file> -start yosys

Running the VTR flow with the default configuration using the Yosys standalone front-end.
The parser for these runs is considered the Yosys conventional Verilog/SystemVerilog parser (i.e., ``read_verilog -sv``), as the parser is not explicitly specified.

.. code-block:: bash

    # Using the Yosys-SystemVerilog plugin if installed, otherwise the Yosys conventional Verilog parser
    ./run_vtr_flow <path/to/SystemVerilog/File> <path/to/arch/file> -start yosys -parser yosys-plugin

    # Using the Surelog plugin if installed, otherwise failure on the unsupported file type
    ./run_vtr_flow <path/to/UHDM/File> <path/to/arch/file> -start yosys -parser surelog

Running the default VTR flow using the Yosys standalone front-end.
The Yosys HDL parser is considered as Yosys-SystemVerilog plugin (i.e., ``read_systemverilog``) and Yosys UHDM plugin (i.e., ``read_uhdm``), respectively.
It is worth mentioning that utilizing Yosys plugins requires passing the ``-DYOSYS_SV_UHDM_PLUGIN=ON`` compile flag to build and install the plugins for the Yosys front-end. 

Detailed Command-line Options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. note:: Any options not recognized by this script is forwarded to VPR.

.. option:: -starting_stage <stage>

    Start the VTR flow at the specified stage.

    Accepted values:

      * ``odin``
      * ``yosys``
      * ``abc``
      * ``scripts``
      * ``vpr``

    **Default:** ``odin``

.. option:: -ending_stage <stage>

    End the VTR flow at the specified stage.


    Accepted values:

      * ``odin``
      * ``yosys``
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

    Tells ODIN II the minimum multiplier size that should be implemented
    using hard multiplier (if available). Smaller multipliers will be
    implemented using soft logic.

    **Default:** 3

.. option:: -min_hard_adder_size <int>

    Tells ODIN II the minimum adder size that should be implemented
    using hard adders (if available). Smaller adders will be
    implemented using soft logic.

    **Default:** 1

.. option:: -adder_cin_global

    Tells ODIN II to connect the first cin in an adder/subtractor chain
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
    
    Tells ODIN II the minimum multiplier size (in bits) to be implemented using hard multiplier.
    
    **Default:** 3

.. option:: -min_hard_adder_size <MIN_HARD_ADDER_SIZE>
    
    Tells ODIN II the minimum adder size (in bits) that should be implemented using hard adder.
    
    **Default:** 1

.. option:: -elaborator <ELABORATOR>
    
    Specifies the elaborator of the synthesis flow for ODIN II [odin, yosys]

    **Default:** odin

.. option:: -top_module <TOP_MODULE>
    
    Specifies the name of the module in the design that should be considered as top

.. option:: -coarsen
    
    Notifies ODIN II if the input BLIF is coarse-grained.

    **Default:** False

.. note::

    A coarse-grained BLIF file is defined as a BLIF file inclduing unmapped cells with the Yosys internal cell (listed `here <https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/b913727959e22ae7a535ac8b907d0aaa9a3eda3d/ODIN_II/SRC/enum_str.cpp#L402-L494>`_) format which are represented by the ``.subckt`` tag in coarse-grained BLIF.
    
.. option:: -fflegalize
    
    Makes flip-flops rising edge for coarse-grained input BLIFs in the partial technology mapping phase (ODIN II synthesis flow generates rising edge FFs by default, should be used for Yosys+Odin-II)
    
    **Default:** False

.. option:: -encode_names
    
    Enables ODIN II utilization of operation-type-encoded naming style for Yosys coarse-grained RTLIL nodes.
    
    .. code-block::

        # example of a DFF subcircuit in the Yosys coarse-grained BLIF
        .subckt $dff CLK=clk D=a Q=inst1.inst2.temp
        .param CLK_POLARITY 1

        .names inst1.inst2.temp o
        1 1

        # fine-grained BLIF file with enabled encode_names option for Odin-II partial mapper
        .latch test^a test^inst1.inst2.temp^FF~0 re test^clk 3

        .names test^inst1.inst2.temp^FF~0 test^o
        1 1

        # fine-grained BLIF file with disabled encode_names option for Odin-II partial mapper
        .latch test^a test^$dff^FF~0 re test^clk 3

        .names test^$dff^FF~0 test^o
        1 1

    **Default:** False

.. option:: -yosys_script <YOSYS_SCRIPT>
    
    Supplies Yosys with a .ys script file (similar to Tcl script), including the synthesis steps.
    
    **Default:** None

.. option:: -parser <PARSER>

    Specify a parser for the Yosys synthesizer [yosys (Verilog-2005), surelog (UHDM), yosys-plugin (SystemVerilog)].
    The script uses the Yosys conventional Verilog parser if this argument is not used.
    
    **Default:** yosys

.. note::

    Universal Hardware Data Model (UHDM) is a complete modeling of the IEEE SystemVerilog Object Model with VPI Interface, Elaborator, Serialization, Visitor and Listener.
    UHDM is used as a compiled interchange format in between SystemVerilog tools. Typical inputs to the UHDM flow are files with ``.v`` or ``.sv`` extensions.
    The ``yosys-plugins`` parser, which represents the ``read_systemverilog`` command, reads SystemVerilog files directly in Yosys.
    It executes Surelog with provided filenames and converts them (in memory) into UHDM file. Then, this UHDM file is converted into Yosys AST. `[Yosys-SystemVerilog] <https://github.com/antmicro/yosys-systemverilog#usage>`_
    On the other hand, the ``surelog`` parser, which uses the ``read_uhdm`` Yosys command, walks the design tree and converts its nodes into Yosys AST nodes using Surelog. `[UHDM-Yosys <https://github.com/chipsalliance/UHDM-integration-tests#uhdm-yosys>`_, `Surelog] <https://github.com/chipsalliance/Surelog#surelog>`_