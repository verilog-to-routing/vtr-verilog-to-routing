.. _vtr_pass_requirements:

Pass Requirements
-----------------

The :ref:`parse_vtr_task` scripts allow you to compare an executed task to a *golden* reference result.
The comparison, which is performed when using the :option:`parse_vtr_task.pl -check_golden` option, which reports either ``Pass`` or ``Fail``.
The requirements that must be met to qualify as a ``Pass`` are specified in the pass requirements file.

Task Configuration
~~~~~~~~~~~~~~~~~~
Tasks can be configured to use a specific pass requirements file using the **pass_requirements_file** keyword in the :ref:`vtr_tasks` configuration file.

File Location
~~~~~~~~~~~~~
All provided pass requirements files are located here::

    $VTR_ROOT/vtr_flow/parse/pass_requirements

Users can also create their own pass requirement files.

File Format
~~~~~~~~~~~
Each line of the file indicates a single metric, data type and allowable values in the following format::

    <metric>;<requirement>

* **<metric>**: The name of the metric.

* **<requirement>**: The metric's pass requirement.

    Valid requiremnt types are:

    * ``Equal()``: The metric value must exactly match the golden reference result.
    * ``Range(<min_ratio>,<max_ratio>)``: The metric value (normalized to the golden result) must be between ``<min_ratio>`` and ``<max_ratio>``.
    * ``RangeAbs(<min_ratio>,<max_ratio>,<abs_threshold>)``: The metric value (normalized to the golden result) must be between ``<min_ratio>`` and ``<max_ratio>``, *or* the metric's absolute value must be below ``<abs_threshold>``.

Or an include directive to import metrics from a separate file::

    %include "<filepath>"

* **<filepath>**: a relative path to another pass requirements file, whose metric pass requirements will be added to the current file.

In order for a ``Pass`` to be reported, **all** requirements must be met.
For this reason, all of the specified metrics must be included in the parse results (see :ref:`vtr_parse_config`).

Comments can be specified with ``#``. Anything following a ``#`` is ignored.

Example File
~~~~~~~~~~~~

.. code-block:: none

    vpr_status;Equal()                      #Pass if precisely equal
    vpr_seconds;RangeAbs(0.80,1.40,2)       #Pass if within -20%, or +40%, or absolute value less than 2
    num_pre_packed_nets;Range(0.90,1.10)    #Pass if withing +/-10%

    %include "routing_metrics.txt"          #Import all pass requirements from the file 'routing_metrics.txt'
