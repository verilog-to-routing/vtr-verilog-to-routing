.. _run_vtr_task:

run_vtr_task
---------------
This script is used to execute one or more *tasks* (i.e. collections of benchmarks and architectures).

.. seealso:: See :ref:`vtr_tasks` for creation and configuration of tasks.

This script runs the VTR flow for a single benchmark circuit and architecture file. 

The script is located at::

    <vtr>/vtr_flow/scripts/run_vtr_task.pl

.. program:: run_vtr_task.pl

Basic Usage
~~~~~~~~~~~

Typical usage is::

    run_vtr_task.pl <task_name1> <task_name2> ...

.. note:: At least one task must be specified, either directly as a parameter or via the :option:`-l` options.

Output
~~~~~~
Each task will execute the script specified in the configuration file for every benchmark/circuit combination.
The standard output of the underlying script will be forwarded to the output of this script.

If golden results exist (see :ref:`parse_vtr_task`), they will be inspected for runtime values.
Any entries in the golden results with with the field names ``pack_time``, ``place_time``, ``route_time``, ``min_chan_width_route_time``, or ``crit_path_route_time`` will be summed to determine an estimated runtime for the benchmark.
This information will be output in the following format before each circuit/benchmark combination::

    Current time: Jan-01 01:00 AM.  Expected runtime of next benchmark: 3 minutes

Depending on the estimated runtime the units will automatically change between seconds, minutes and hours.
This will not be output if the golden results file cannot be found, or if the :option:`-hide_runtime` option is used, or if the underlying script is changed from the default :ref:`run_vtr_flow`.

Detailed Command-line Options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


.. option:: -s <script_param> ...

    Treat the remaining command line options as parameters to forward to the underlying script (e.g. :ref:`run_vtr_flow`).

.. option:: -p <N>

    Perform parallel execution using ``N`` threads.

    .. warning::
        Large benchmarks will use very large amounts of memory (several to 10s of gigabytes).
        Because of this, parallel execution often saturates the physical memory, requiring the use of swap memory, which significantly slows execution.
        Be sure you have allocated a sufficiently large swap memory or errors may result.

.. option:: -l <task_list_file>
    
    A file containing a list of tasks to execute.
    
    Each task name should be on a separate line, e.g.::
        
        <task_name1>
        <task_name2>
        <task_name3>
        ...

.. option:: -hide_runtime

    Do not show runtime estimates.
