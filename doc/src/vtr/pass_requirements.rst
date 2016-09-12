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

    <vtr>/vtr_flow/parse/pass_requirements

Users can also create their own pass requirement files.

File Format
~~~~~~~~~~~
Each line of the file indicates a single metric, data type and allowable values in the following format::

    <metric_name>;<type>;<min_value>;<max_value>

* **<metric_name>**: The identifier for the metric.

* **<type>**: The type of metric.

    Valid types are:

    * ``range``: numerical values with the permissible range set by ``<min_value>`` and ``<max_value>``.
    * ``string``: requires an exact match

* **<min_value>**: Minimum allowed metric value (normalized to golden result).

* **<max_value>**: Maximum allowed metric value (normalized to golden result).

In order for a ``Pass`` to be reported, **all** requirements must be met.
For this reason, all of the specified metrics must be included in the parse results (see :ref:`vtr_parse_config`).

Example File
~~~~~~~~~~~~

.. code-block:: none

    vpr_status;string
    vpr_seconds;range;0.80;1.40
    width;range;0.80;1.40
    pack_time;range;0.80;1.40
    place_time;range;0.80;1.40
    route_time;range;0.80;1.40
    num_pre_packed_nets;range;0.80;1.40
    num_pre_packed_blocks;range;0.80;1.40
    num_post_packed_nets;range;0.80;1.40
    num_clb;range;0.80;1.40
    num_outputs;range;0.80;1.40
    error;string
