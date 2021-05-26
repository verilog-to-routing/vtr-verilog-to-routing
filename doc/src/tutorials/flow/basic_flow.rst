.. _basic_design_flow_tutorial:

Basic Design Flow Tutorial
==========================

The following steps show you to run the VTR design flow to map a sample circuit to an FPGA architecture containing embedded memories and multipliers:

#.  From the :term:`$VTR_ROOT`  , move to the ``vtr_flow/tasks/regression_tests/vtr_reg_basic`` directory, and run:

    .. code-block:: shell
    
        ../../../scripts/run_vtr_task.py basic_no_timing
        
    or:
 
     .. code-block:: shell

        $VTR_ROOT/vtr_flow/scripts/run_vtr_task.py basic_no_timing 
        
    The subdirectory ``regression_tests/vtr_reg_basic`` contains tests that are to be run before each commit. They check for basic functionallity to make sure nothing was           extremely out of order. This command runs the VTR flow on a set of circuits and a single architecture. 
    The files generated from the run are stored in ``basic_no_timing/run[#]`` where ``[#]`` is the number of runs you have done.
    If this is your first time running the flow, the results will be stored in basic_no_timing/run001.
    When the script completes, enter the following command:

    .. code-block:: shell

         ../../../scripts/python_libs/vtr/parse_vtr_task.py basic_no_timing/

    This parses out the information of the VTR run and outputs the results in a text file called ``run[#]/parse_results.txt``.

    More info on how to run the flow on multiple circuits and architectures along with different options later.
    Before that, we need to ensure that the run that you have done works.

#.  The basic_no_timing comes with golden results that you can use to check for correctness.
    To do this check, enter the following command:

    .. code-block:: shell

        ../../../scripts/python_libs/vtr/parse_vtr_task.py -check_golden basic_no_timing

    It should return: ``basic_no_timing...[Pass]``

    .. note::

        Due to the nature of the algorithms employed, the measurements that you get may not match exactly with the golden measurements.
        We included margins in our scripts to account for that noise during the check.
        We also included runtime estimates based on our machine.
        The actual runtimes that you get may differ dramatically from these values.

#.  To see precisely which see circuits, architecture, and CAD flow was employed by the run, look at ``vtr_flow/tasks/regression_tests/vtr_reg_basic/config.txt``.
    Inside this directory, the ``config.txt`` file contains the circuits and architecture file employed in the run.

    Some also contain a ``golden_results.txt`` file that is used by the scripts to check for correctness.

    The ``$VTR_ROOT/vtr_flow/scripts/run_vtr_flow.py`` script describes the CAD flow employed in the test.
    You can modify the flow by editing this script.

    At this point, feel free to run any of the tasks with the prefix ``vtr_reg`` 
    These are regression tests included with the flow that test various combinations of flows, architectures, and benchmarks.         Refer to the ``README`` for a description what each task aims to test. 

#.  For more information on how the vtr_flow infrastructure works (and how to add the tests that you want to do to this infrastructure) see :ref:`vtr_tasks`.
