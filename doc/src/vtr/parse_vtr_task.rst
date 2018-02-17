.. _parse_vtr_task:

parse_vtr_task
--------------

This script is used to parse the output of one or more :ref:`vtr_tasks`.
The values that will be parsed are specified using a :ref:`vtr_parse_config` file, which is specified in the task configuration.

The script will always parse the results of the latest execution of the task.

The script is located at::

    $VTR_ROOT/vtr_flow/scripts/parse_vtr_task.pl

.. program:: parse_vtr_task.pl

Usage
~~~~~

Typical usage is::

    parse_vtr_task.pl <task_name1> <task_name2> ...

.. note:: At least one task must be specified, either directly as a parameter or through the :option:`-l` option.

Output
~~~~~~

By default this script produces no standard output.
A tab delimited file containing the parse results will be produced for each task.
The file will be located here::

    $VTR_ROOT/vtr_flow/tasks/<task_name>/run<#>/parse_results.txt

If the :option:`-check_golden` is used, the script will output one line for each task in the format::

    <task_name>...<status>

where ``<status>`` will be ``[Pass]``, ``[Fail]``, or ``[Error]``.

Detailed Command-line Options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


.. option:: -l <task_list_file>

    A file containing a list of tasks to parse.
    Each task name should be on a separate line.

.. option:: -create_golden

    The results will be stored as golden results.
    If previous golden results exist they will be overwritten.

    The golden results are located here::

        $VTR_ROOT/vtr_flow/tasks/<task_name>/config/golden_results.txt

.. option:: -check_golden

    The results will be compared to the golden results using the :ref:`vtr_pass_requirements` file specified in the task configuration.
    A ``Pass`` or ``Fail`` will be output for each task (see below).
    In order to compare against the golden results, they must already exist, and have the same architectures, circuits and parse fields, otherwise the script will report ``Error``.

    If the golden results are missing, or need to be updated, use the :option:`-create_golden` option.
