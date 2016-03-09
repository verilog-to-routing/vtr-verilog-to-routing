.. _run_vtr_flow:

run_vtr_flow
---------------

This script runs the VTR flow for a single benchmark circuit and architecture file. 

The script is located at::

    <vtr>/vtr_flow/scripts/run_vtr_flow.pl

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

.. option:: -specific_vpr_stage <vpr_stage>

    Perform only this stage of :ref:`vpr`.

    To have any time saving effect, previous result files must be kept, as the most recent necessary ones will be moved to the current run directory (use inside tasks only).

    Accepted values:

      * ``pack``
      * ``place``
      * ``route``

    **Default:** empty (run all vpr stages)

    .. note:: Specifying the routing stage requires a channel width to also be specified.

.. option:: -power
    
    Enables power estimation.

    See :ref:`power_estimation`

.. option:: -cmos_tech <file>
    
    CMOS technology XML file.

    See :ref:`power_technology_properties`

.. option:: -keep_intermediate_files

    Do not delete intermediate files.

.. option:: -keep_result_files

    Do not delete the result files (i.e. VPR's ``.net``, ``.place``, ``.route`` outputs)

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
