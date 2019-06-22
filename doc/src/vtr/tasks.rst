.. _vtr_tasks:

Tasks
-----

Tasks provide a framework for running the VTR flow on multiple benchmarks, architectures and with multiple CAD tool parameters.

A task specifies a set of benchmark circuits, architectures and CAD tool parameters to be used.
By default, tasks execute the :ref:`run_vtr_flow` script for every circuit/architecture/CAD parameter combination.

Example Tasks
~~~~~~~~~~~~~
* ``basic_flow``: Runs the VTR flow mapping a simple Verilog circuit to an FPGA architecture.

* ``timing``: Runs the flagship VTR benchmarks on a comprehensive, realistic architecture file.

* ``timing_chain``: Same as ``timing`` but with carry chains.

* ``regression_mcnc``: Runs VTR on the historical MCNC benchmarks on a legacy architecture file. (Note: This is only useful for comparing to the past, it is not realistic in the modern world)

* ``regression_titan/titan_small``: Runs a small subset of the Titan benchmarks targetting a simplified Altera Stratix IV (commercial FPGA) architecture capture

* ``regression_fpu_hard_block_arch``: Custom hard FPU logic block architecture

Directory Layout
~~~~~~~~~~~~~~~~

All of VTR's included tasks are located here::

    $VTR_ROOT/vtr_flow/tasks

If users wishes to create their own task, they must do so in this location.

All tasks must contain a configuration file located here::

    $VTR_ROOT/vtr_flow/tasks/<task_name>/config/config.txt


:numref:`fig_vtr_tasks_file_layout` illustrates the directory layout for a VTR task.
Every time the task is run a new ``run<#>`` directory is created to store the output files, where ``<#>`` is the smallest integer to make the run directory name unique.

The symbolic link ``latest`` will point to the most recent ``run<#>`` directory.

.. _fig_vtr_tasks_file_layout:

.. figure:: vtr_task_fig.*
    :width: 75%

    Task directory layout.

Creating a New Task
~~~~~~~~~~~~~~~~~~~

#. Create the folder ``$VTR_ROOT/vtr_flow/tasks/<task_name>``
#. Create the folder ``$VTR_ROOT/vtr_flow/tasks/<task_name>/config``
#. Create and configure the file ``$VTR_ROOT/vtr_flow/tasks/<task_name>/config/config.txt``


Task Configuration File
~~~~~~~~~~~~~~~~~~~~~~~
The task configuration file contains key/value pairs separated by the ``=`` character.
Comment line are indicted using the ``#`` symbol.

Example configuration file:

.. code-block:: none

    # Path to directory of circuits to use
    circuits_dir=benchmarks/verilog

    # Path to directory of architectures to use
    archs_dir=arch/timing

    # Add circuits to list to sweep
    circuit_list_add=ch_intrinsics.v
    circuit_list_add=diffeq1.v

    # Add architectures to list to sweep
    arch_list_add=k6_N10_memSize16384_memData64_stratix4_based_timing_sparse.xml

    # Parse info and how to parse
    parse_file=vpr_standard.txt

.. note::

    :ref:`run_vtr_task` will invoke the script (default :ref`run_vtr_flow`) for the cartesian product of circuits, architectures and script parameters specified in the config file.

Required Fields
~~~~~~~~~~~~~~~

* **circuit_dir**: Directory path of the benchmark circuits.

    Absolute path or relative to ``$VTR_ROOT/vtr_flow/``.

* **arch_dir**: Directory path of the architecture XML files.

    Absolute path or relative to ``$VTR_ROOT/vtr_flow/``.

* **circuit_list_add**: Name of a benchmark circuit file.

    Use multiple lines to add multiple circuits.

* **arch_list_add**: Name of an architecture XML file.

    Use multiple lines to add multiple architectures.

* **parse_file**: :ref:`vtr_parse_config` file used for parsing and extracting the statistics.

    Absolute path or relative to ``$VTR_ROOT/vtr_flow/parse/parse_config``.

Optional Fields
~~~~~~~~~~~~~~~

* **script_path**: Script to run for each architecture/circuit combination.

    Absolute path or relative to ``$VTR_ROOT/vtr_flow/scripts/`` or ``$VTR_ROOT/vtr_flow/tasks/<task_name>/config/``)

    **Default:** :ref:`run_vtr_flow`

    Users can set this option to use their own script instead of the default.
    The circuit path will be provided as the first argument, and architecture path as the second argument to the user script.

* **script_params_common**: Common parameters to be passed to all script invocations.

    This can be used, for example, to run partial VTR flows.

    **Default:** none

* **script_params**: Alias for `script_params_common`

* **script_params_list_add**: Adds a set of command-line arguments

    Multiple `script_params_list_add` can be provided which are addded to the cartesian product of configurations to be evaluated.

* **pass_requirements_file**: :ref:`vtr_pass_requirements` file.

    Absolute path or relative to ``$VTR_ROOT/vtr_flow/parse/pass_requirements/`` or ``$VTR_ROOT/vtr_flow/tasks/<task_name>/config/``

    **Default:** none
