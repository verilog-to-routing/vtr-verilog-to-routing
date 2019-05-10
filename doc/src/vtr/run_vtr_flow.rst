.. _run_vtr_flow:

run_vtr_flow
---------------

This script runs the VTR flow for a single benchmark circuit and architecture file.

The script is located at::

    $VTR_ROOT/vtr_flow/scripts/run_vtr_flow.pl

.. program:: run_vtr_flow.pl

Basic Usage
~~~~~~~~~~~

At a minimum ``run_vtr_flow.pl`` requires two command-line arguments::

    run_vtr_flow.pl <circuit_file> <architecture_file>

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

Additional *optional* command arguments can also be passed to ``run_vtr_flow.pl``::

    run_vtr_flow.pl <circuit_file> <architecture_file> [<options>] [<vpr_options>]

where:

  * ``<options>`` are additional arguments passed to ``run_vtr_flow.pl`` (described below),
  * ``<vpr_options>`` are any arguments not recognized by ``run_vtr_flow.pl``. These will be forwarded to VPR.

For example::

   run_vtr_flow.pl my_circuit.v my_arch.xml -track_memory_usage --pack --place

will run the VTR flow to map the circuit ``my_circuit.v`` onto the architecture ``my_arch.xml``; the arguments ``--pack`` and ``--place`` will be passed to VPR (since they are unrecognized arguments to ``run_vtr_flow.pl``).
They will cause VPR to perform only :ref:`packing and placement <general_options>`.

Detailed Command-line Options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. note:: Any options not recognized by this script is forwarded to VPR.

.. option:: -starting_stage <stage>

    Start the VTR flow at the specified stage.

    Accepted values:

      * ``odin``
      * ``abc``
      * ``scripts``
      * ``vpr``

    **Default:** ``odin``

.. option:: -ending_stage <stage>

    End the VTR flow at the specified stage.


    Accepted values:

      * ``odin``
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
