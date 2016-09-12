.. _running_vtr:

Running the VTR Flow
--------------------

VTR is a collection of tools that perform the full FPGA CAD flow from Verilog to routing.

The design flow consists of:

* :ref:`odin_ii` (Logic Synthesis)
* :ref:`abc` (Logic Optimization & Technology Mapping)
* :ref:`vpr` (Pack, Place & Route)

There is no single executable for the entire flow.

Instead, scripts are provided to allow the user to easily run the entire tool flow.
The following provides instructions on using these scripts to run VTR.

Running a Single Benchmark
~~~~~~~~~~~~~~~~~~~~~~~~~~
The :ref:`run_vtr_flow` script is provided to execute the VTR flow for a single benchmark and architecture.

.. note:: In the following :term:`$VTR_ROOT` means the root directory of the VTR source code tree.

.. code-block:: none

    $VTR_ROOT/vtr_flow/scripts/run_vtr_flow.pl <circuit_file> <architecture_file>

It requires two arguments:

 * ``<circuit_file>`` A benchmark circuit, and
 * ``<architecture_file>`` an FPGA architecture file

Circuits can be found under::

    $VTR_ROOT/vtr_flow/benchmarks/

Architecture files can be found under::

    $VTR_ROOT/vtr_flow/arch/

The script can also be used to run parts of the VTR flow.

.. seealso:: :ref:`run_vtr_flow` for the detailed command line options of ``run_vtr_flow.pl``.


Running Multiple Benchmarks & Architectures with Tasks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
VTR also supports *tasks*, which manage the execution of the VTR flow for multiple benchmarks and architectures.
By default, tasks execute the :ref:`run_vtr_flow` for every circuit/architecture combination.

VTR provides a variety of standard tasks which can be found under::

    $VTR_ROOT/vtr_flow/tasks


Tasks can be executed using :ref:`run_vtr_task`::

    $VTR_ROOT/vtr_flow/scripts/run_vtr_task.pl <task_name>

.. seealso:: :ref:`run_vtr_task` for the detailed command line options of ``run_vtr_task.pl``.

.. seealso:: :ref:`vtr_tasks` for more information on creating, modifying and running tasks.


Extracting Information & Statistics
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
VTR can also extract useful information and statistics from executions of the flow such as area, speed tool execution time etc.

For single benchmarks :ref:`parse_vtr_flow` extrastics statistics from a single execution of the flow.

For a :ref:`Task <vtr_tasks>`, :ref:`parse_vtr_task` can be used to parse and assemble statistics for the entire task (i.e. multiple circuits and architectures).

For regression testing purposes these results can also be verified against a set of *golden* reference results.
See :ref:`parse_vtr_task` for details.
