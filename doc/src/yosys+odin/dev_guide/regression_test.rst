.. _regression_test:

Regression Tests
================

Regression tests are tests that are repeatedly executed to assess functionality.
Each regression test targets a specific function of Odin-II and Yosys+Odin-II.
There are two main components of a regression test; benchmarks and a configuration file.
The benchmarks are comprised of Verilog files, input vector files and output vector files.
Due to different approaches taken by Yosys+Odin-II and Odin-II synthesizers and some unsupported Verilog features by Odin-II, each synthesizer has its output vector files, named using ``XXX_odin_input/output`` and ``XXX_yosys_input/output`` name convention.
In some cases, both output vectors are identical while they differ for the remainder of them.
The configuration file calls upon each benchmark and synthesizes them with different architectures.
The current regression tests of Odin-II and Yosys+Odin-II can be found in `regression_test/benchmark`.

Benchmarks
----------

Benchmarks are used to test the functionality of Odin-II and Yosys+Odin-II and ensure that they run properly.
Except for the common benchmarks, located in `regression_test/benchmark/verilog/common`, which are Yosys basic benchmarks to verify its functionality, Yosys+Odin-II mainly utilizes Odin-II's benchmarks that can be found in `regression_test/benchmark/verilog/any_folder`.
Moreover, Yosys+Odin-II has its own tasks and suites which are located in `regression_test/benchmark/task/yosys+odin` and `regression_test/benchmark/suite/yosys+odin` directories.
Each benchmark is comprised of a Verilog file, an input vector file, and an output vector file.
They are called upon during regression tests and synthesized with different architectures to be compared against the expected results.
These tests are useful for developers to test the functionality of Yosys+Odin-II after implementing changes.
The command ``make test_yosys+odin`` runs through all these tests, comparing the results to previously generated results, and should be run through when first installing.

Unit Benchmarks
~~~~~~~~~~~~~~~

Unit benchmarks are the simplest of benchmarks. They are meant to isolate different functions of Yosys+Odin-II.
The goal is that if it does not function properly, the error can be traced back to the function being tested.
This cannot always be achieved as different functions depend on others to work properly.
It is ideal that these benchmarks test bit size capacity, erroneous cases, as well as Yosys basic sub-ciruits as coarse-grained netlist building blocks.

Micro Benchmarks
~~~~~~~~~~~~~~~~

Micro benchmarks are precise, like unit benchmarks, however are more syntactic.
They are meant to isolate the behaviour of different functions.
They trace the behaviour of functions to ensure they adhere to the IEEE Standard for Verilog® Hardware Description Language - 2005.
Like unit benchmarks, they should check erroneous cases and behavioural standards set by the IEEE Standard for Verilog® Hardware Description Language - 2005.

Macro Benchmarks
~~~~~~~~~~~~~~~~

Macro benchmarks are more realistic tests that incorporate multiple functions of Odin-II and Yosys+Odin-II.
They are intended to simulate real-user behaviour to ensure that functions work together properly.
These tests are designed to test things like syntax and more complicated standards set by the IEEE Standard for Verilog® Hardware Description Language - 2005.

External Benchmarks
~~~~~~~~~~~~~~~~~~~

External benchmarks are benchmarks created by outside users to the project.
It is possible to pull an outside directory and build them on the fly thus creating a benchmark for Odin-II and Yosys+Odin-II.

Creating Regression Tests
-------------------------

New Regression Test Checklist
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* :ref:`Creating benchmarks <creating_benchmarks>`
* :ref:`Creating configuration file <creating_configuration_file>`
* :ref:`Creating a folder in the task directory for the configuration file <creating_task>`
* :ref:`Generating the results <regenerating_results>`
* :ref:`Adding the task to a suite <creating_suite>` (large suite if generating the results takes longer than 3 minutes, otherwise put in light suite)
* :ref:`Updating the documentation by providing a summary in Regression Test Summary section and updating the Directory Tree <regression_test_summeries>`

New Benchmarks added to Regression Test Checklist
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* :ref:`Creating benchmarks and add them to the correct regression test folder found in the benchmark/verilog directory <creating_benchmarks>`  (There is a description of each regression test :ref:`here<regression_test_summeries>`)
* :ref:`Regenerating the results <regenerating_results>`

**Include**

* Verilog file
* Input vector file (`be careful about the elaborator-based naming style of simulation vector files`)
* Expected output vector file (`be careful about the elaborator-based naming style of simulation vector files`)
* Configuration file (`conditional`)
* Architecture file (`optional`)

.. _creating_benchmarks:

Creating Benchmarks
~~~~~~~~~~~~~~~~~~~

If only a few benchmarks are needed for a PR, simply add the benchmarks to the appropriate set of regression tests.
The [Regression Test Summary](#regression-test-summaries) summarizes the target of each regression test which may be helpful.


.. note::

	In the following, assumed Yosys+Odin-II performs the synthesis, i.e., ``--elaborator yosys`` is added to the Odin-II execution command arguments.

The standard of naming the benchmarks are as follows:

* verilog file: meaningful_title.v
* input vector file: meaningful_title_yosys_input
* output vector file: meaningful_title_yosys_output

If the tests needed do not fit in an already existing set of regression tests or need certain architecture(s), create a separate folder in the _verilog_ directory and label appropriately.
Store the benchmarks in that folder.
Add the architecture (if it isn't one that already exists) to `$VTR_ROOT/vtr_flow/arch`.

.. note::

	If a benchmark fails and should pass, include a $display statement in the Verilog file in the following format:

	`$display("Expect::FUNCTION < message >");`

	The function should be in all caps and state what is causing the issue. For instance, a sub-circuit _add_ was instantiated incorrectly. The message 
	should illustrate what should happen and perhaps a suggestion in where things are going wrong. This would cause Yosys elaboration failure, so that developers can easily figure out which benchmark results in the buggy feature.


.. _creating_configuration_file:

Creating a Configuration File
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A configuration file is only necessary if the benchmarks added are placed in a new folder.
The configuration file is where architectures and commands are specified for the synthesis of the benchmarks.
**The configuration file must be named task.conf.**
The following is an example of a standard task.conf (configuration) file:  

.. code-block::

	
	###########################
	# <title> benchmarks config
	###########################

	# commands
	regression_params=--include_default_arch
	synthesis_params=--elaborator yosys --fflegalize --show_yosys_log
	simulation_params= -L reset rst -H we
	script_synthesis_params=--time_limit 3600s 
	script_simulation_params=--time_limit 3600s

	# setup the architecture (standard architectures already available)
	archs_dir=../vtr_flow/arch/timing

	arch_list_add=k6_N10_40nm.xml
	arch_list_add=k6_N10_mem32K_40nm.xml
	arch_list_add=k6_frac_N10_frac_chain_mem32K_40nm.xml

	# setup the circuits
	circuits_dir=regression_test/benchmark/verilog/

	circuit_list_add=<verilog file group>/*.vh
	circuit_list_add=<verilog file group>/*.v


	synthesis_parse_file=regression_test/parse_result/conf/synth.toml
	simulation_parse_file=regression_test/parse_result/conf/sim.toml


The following key = value are available for configuration files:

.. table::

    ===========================    =======================================================================
             Key                                              Following Argument
    ===========================    =======================================================================
     circuits_dir                   < path/to/circuit/dir >                            
     circuit_list_add               < circuit file path relative to [circuits_dir] >   
     archs_dir                      < path/to/arch/dir >                               
     arch_list_add                  < architecture file path relative to [archs_dir] > 
     synthesis_parse_file           < path/to/parse/file >                             
     simulation_parse_file          < path/to/parse/file >                             
     script_synthesis_params        [see exec_wrapper.sh options]                     
     script_simulation_params       [see exec_wrapper.sh options]                      
     synthesis_params               [see Odin-II options]                              
     simulation_params              [see Odin-II options]                              
     regression_params              - ``--verbose``                display error logs after batch of tests				
                                    - ``--concat_circuit_list``    concatenate the circuit list and pass it straight through to Odin-II
                                    - ``--generate_bench``         generate input and output vectors from scratch
                                    - ``--disable_simulation``     disable the simulation for this task
                                    - ``--disable_parallel_jobs``  disable running circuit/task pairs in parallel
                                    - ``--randomize``              perform a dry run randomly to check the validity of the task and flow                       
                                    - ``--regenerate_expectation`` regenerate expectation and override the expected value only if there's a mismatch          
                                    - ``--generate_expectation``   generate the expectation and override the expectation file       
                                    - ``--enable_techmap``         enable the technology mapping for this task (only coarsen blif files should pass as `circuit_list_add`)                   
    ===========================    =======================================================================

.. _creating_task:

Creating a Task
~~~~~~~~~~~~~~~

The following diagram illustrates the structure of regression tests.
Each regression test needs a corresponding folder in the task directory containing the configuration file.
The \<task display name\> should have the same name as the Verilog file group in the `verilog` directory.
This folder is where the synthesis results and simulation results will be stored.
The task display name and the Verilog file group should share the same title.

.. code-block:: bash

	└── ODIN_II
		└── regression_test
			└── benchmark
				├── task
				│    └── yosys+odin
				│        └── < task display name >
				│            ├── [ simulation_result.json ]
				│            ├── [ synthesis/techmap_result.json ]
				│            └── task.conf
				├── verilog
				│    └── < Verilog file group >
				│        ├── *.v
				│        ├── *.vh
				│        ├── *_yosys_input
				│        └── *_yosys_output
				└── blif
					└── < BLIF file group >
						├── *.blif
						├── *_yosys_input
						└── *_yosys_output


.. _creating_suite:

Creating a Suite
~~~~~~~~~~~~~~~~

Suites are used to call multiple tasks at once. This is handy for regenerating results for multiple tasks.
In the diagram below you can see the structure of the suite.
The suite contains a configuration file that calls upon the different tasks named **task_list.conf**.

.. code-block::

	└── ODIN_II
		└── regression_test
			└── benchmark
				├── suite
				│    └── yosys+odin
				│        └── < suite display name >
				│            └── task_list.conf
				├── task
				│    └── yosys+odin
				│        └── < task display name >
				│            ├── [ simulation_result.json ]
				│            ├── [ synthesis/techmap_result.json ]
				│            └── task.conf
				├── verilog
				│    └── < verilog file group >
				│        ├── *.v
				│        ├── *.vh
				│        ├── *_yosys_input
				│        └── *_yosys_output
				└── blif
					└── < blif file group >
						├── *.blif
						├── *_yosys_input
						└── *_yosys_output


.. note::

	To generate only coarse-grained BLIF files using Yosys elaborator, a new script named ``run_yosys.sh``, is created in the `regression_test/tools` directory. The behaviour of this script is similar to the ``verfiy_script.sh``. Indeed, it can perform the Yosys elaboration on a task or suite and generate corresponding coarse-grained BLIF files.

In the configuration file all that is required is to list the tasks to be included in the suite with the path.
For example, if the wanted suite was to call the FIR task and the operators task, the configuration file would be as follows:

.. code-block::

	regression_test/benchmark/task/yosys+odin/operators
	regression_test/benchmark/task/yosys+odin/FIR


For more examples of `task_list.conf` configuration files look at the already existing configuration files in the suites.

.. _regenerating_results:

Regenerating Results
~~~~~~~~~~~~~~~~~~~~

.. note::

	**BEFORE** regenerating the result, run ``make test_yosys+odin`` to ensure any changes in the code don't affect the results of benchmarks beside your own. If they do, the failing benchmarks will be listed.

Regenerating results is necessary if any regression test is changed (added benchmarks), if a regression test is added, or if a bug fix was implemented that changes the results of a regression test.
For all cases, it is necessary to regenerate the results of the task corresponding to said change.
The following commands illustrate how to do so:

.. code-block:: bash

	make sanitize CMAKE_PARAMS="-DODIN_USE_YOSYS=ON"


Then, where N is the number of processors in the computer, and the path following ``-t`` ends with the same name as the folder you placed

.. code-block:: bash

	./verify_odin.sh -j N --regenerate_expectation -t regression_test/benchmark/task/yosys+odin/<task_display_name>


.. note::

	**DO NOT** run the ``make sanitize`` if regenerating the large test. It is probable that the computer will not have enough RAM to do so and it will take a long time. Instead run ``make yosys+odin``

For more on regenerating results, refer to the `Verify Script <https://docs.verilogtorouting.org/en/latest/odin/dev_guide/verify_script/>`_  section in the Odin-II developers guide.

.. _regression_test_summeries:

Regression Test Summaries
-------------------------

c_functions
~~~~~~~~~~~

This regression test targets C functions supported by Verilog such as `clog_2`.

FIR
~~~

FIR is an acronym for "Finite Impulse Response".
These benchmarks were sourced from `Layout Aware Optimization of High Speed Fixed Coefficient FIR Filters for FPGAs <http://kastner.ucsd.edu/fir-benchmarks/?fbclid=IwAR0sLk_qaBXfeCeDuzD2EWBrCJ_qGQd7rNISYPemU6u98F6CeFjWOMAM2NM>`_.
They test a method of implementing high speed FIR filters on FPGAs discussed in the paper.

full
~~~~

The full regression test is designed to test real user behaviour.  
It does this by simulating flip flop, muxes and other common uses of Verilog.  

large
~~~~~

This regression test targets cases that require a lot of ram and time.  

micro
~~~~~

The micro regression test targets hards blocks and pieces that can be easily instantiated in architectures.

mixing_optimization
~~~~~~~~~~~~~~~~~~~

The mixing optimization regression test targets mixing implementations for operations implementable in hard blocks and their soft logic counterparts that can be can be easily instantiated in architectures. The tests support extensive command line coverage, as well as provide infrastructure to enable the optimization from an .xml configuration file, require for using the optimization as a part of VTR synthesis flow.

operators
~~~~~~~~~

This regression test targets the functionality of different operators. It checks bit size capacity and behaviour.

syntax
~~~~~~

The syntax regression test targets syntactic behaviour. It checks that functions work cohesively together and adhere to the verilog standard.

keywords
~~~~~~~~

This regression test targets the function of keywords. It has a folder or child for each keyword containing their respective benchmarks. Some folders have benchmarks for two keywords like task_endtask because they both are required together to function properly.

preprocessor
~~~~~~~~~~~~

This set of regression test includes benchmarks targetting compiler directives available in Verilog.

Regression Tests Directory Tree
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block::

	benchmark
		├── suite
		│     ├── complex_synthesis_suite
		│     │   └── task_list.conf
		│     ├── full_suite
		│     │   └── task_list.conf
		│     ├── heavy_suite
		│     │   └── task_list.conf
		│     ├── light_suite
		│     │    └── task_list.conf
		│     └── yosys+odin
		│          ├── techmap_heavysuite	
		│          ├── techmap_keywords_suite	
		│          └── techmap_lightsuite	
		├── task
		│     ├── arch_sweep
		│     │   ├── synthesis_result.json
		│     │   └── task.conf
		│     ├── c_functions
		│     │   └── clog2
		│     │       ├── simulation_result.json
		│     │       ├── synthesis_result.json
		│     │       └── task.conf
		│     ├── cmd_line_args
		│     │   ├── batch_simulation
		│     │   │   ├── simulation_result.json
		│     │   │   ├── synthesis_result.json
		│     │   │   └── task.conf
		│     │   ├── best_coverage
		│     │   │   ├── simulation_result.json
		│     │   │   ├── synthesis_result.json
		│     │   │   └── task.conf
		│     │   ├── coverage
		│     │   │   ├── simulation_result.json
		│     │   │   ├── synthesis_result.json
		│     │   │   └── task.conf
		│     │   ├── graphviz_ast
		│     │   │   ├── synthesis_result.json
		│     │   │   └── task.conf
		│     │   ├── graphviz_netlist
		│     │   │   ├── synthesis_result.json
		│     │   │   └── task.conf
		│     │   └── parallel_simulation
		│     │       ├── simulation_result.json
		│     │       ├── synthesis_result.json
		│     │       └── task.conf
		│     ├── FIR
		│     │   ├── simulation_result.json
		│     │   ├── synthesis_result.json
		│     │   └── task.conf
		│     ├── fpu
		│     │   └── hardlogic
		│     │       ├── simulation_result.json
		│     │       ├── synthesis_result.json
		│     │       └── task.conf
		│     ├── full
		│     │   ├── simulation_result.json
		│     │   ├── synthesis_result.json
		│     │   └── task.conf
		│     ├── keywords
		│     │   ├── always
		│     │   ├── and
		│     │   ├── assign
		│     │   ├── at_parenthathese
		│     │   ├── automatic
		│     │   ├── begin_end
		│     │   ├── buf
		│     │   ├── case_endcase
		│     │   ├── default
		│     │   ├── defparam
		│     │   ├── else
		│     │   ├── for
		│     │   ├── function_endfunction
		│     │   ├── generate
		│     │   ├── genvar
		│     │   ├── if
		│     │   ├── initial
		│     │   ├── inout
		│     │   ├── input_output
		│     │   ├── integer
		│     │   ├── localparam
		│     │   ├── macromodule
		│     │   ├── nand
		│     │   ├── negedge
		│     │   ├── nor
		│     │   ├── not
		│     │   ├── or
		│     │   ├── parameter
		│     │   ├── posedge
		│     │   ├── reg
		│     │   ├── signed_unsigned
		│     │   ├── specify_endspecify
		│     │   ├── specparam
		│     │   ├── star
		│     │   ├── task_endtask
		│     │   ├── while
		│     │   ├── wire
		│     │   ├── xnor
		│     │   └── xor
		│     ├── koios
		│     │   ├── synthesis_result.json
		│     │   └── task.conf
		│     ├── large
		│     │   ├── synthesis_result.json
		│     │   └── task.conf
		│     ├── micro
		│     │   ├── simulation_result.json
		│     │   ├── synthesis_result.json
		│     │   └── task.conf
		│     ├── mixing_optimization
		│     │   ├── mults_auto_full
		│     │   │   ├── simulation_result.json
		│     │   │   │── synthesis_result.json
		│     │   │   └── task.conf
		│     │   ├── mults_auto_half
		│     │   │   ├── simulation_result.json
		│     │   │   │── synthesis_result.json
		│     │   │   └── task.conf
		│     │   ├── mults_auto_none
		│     │   │   ├── simulation_result.json
		│     │   │   │── synthesis_result.json
		│     │   │   └── task.conf
		│     │   ├── config_file_half
		│     │   │   ├── config_file_half.xml
		│     │   │   ├── simulation_result.json
		│     │   │   │── synthesis_result.json
		│     │   │   └── task.conf
		│     ├── operators
		│     │   ├── simulation_result.json
		│     │   ├── synthesis_result.json
		│     │   └── task.conf
		│     ├── preprocessor
		│     │   ├── simulation_result.json
		│     │   ├── synthesis_result.json
		│     │   └── task.conf
		│     ├── syntax
		│     │   ├── simulation_result.json
		│     │   ├── synthesis_result.json
		│     │   └── task.conf
		│     ├── vtr
		│     │   ├── synthesis_result.json
		│     │   └── task.conf
		│     └── yosys+odin
		│         ├── arch_sweep
		│         │   ├── synthesis_result.json
		│         │   └── task.conf
		│         ├── c_functions
		│         │	└── clog2
		│         │   	├── simulation_result.json
		│         │   	├── synthesis_result.json
		│         │   	└── task.conf
		│         ├── common
		│         │   ├── simulation_result.json
		│         │   ├── synthesis_result.json
		│         │   └── task.conf
		│         ├── FIR
		│         │   ├── simulation_result.json
		│         │   ├── synthesis_result.json
		│         │   └── task.conf
		│         ├── fpu
		│	        │   ├── hardlogic
		│         │   │   ├── synthesis_result.json
		│         │   │   └── task.conf
		│         │   └── softlogic
		│         │       ├── simulation_result.json
		│         │       ├── synthesis_result.json
		│         │       └── task.conf
		│         ├── full
		│         │   ├── simulation_result.json
		│         │   ├── synthesis_result.json
		│         │   └── task.conf
		│         ├── keywords
		│         │   ├── always
		│         │   ├── and
		│         │   ├── assign
		│         │   ├── at_parenthathese
		│         │   ├── automatic
		│         │   ├── begin_end
		│         │   ├── buf
		│         │   ├── case_endcase
		│         │   ├── default
		│         │   ├── defparam
		│         │   ├── else
		│         │   ├── for
		│         │   ├── function_endfunction
		│         │   ├── generate
		│         │   ├── genvar
		│         │   ├── if
		│         │   ├── initial
		│         │   ├── inout
		│         │   ├── input_output
		│         │   ├── integer
		│         │   ├── localparam
		│         │   ├── macromodule
		│         │   ├── nand
		│         │   ├── negedge
		│         │   ├── nor
		│         │   ├── not
		│         │   ├── or
		│         │   ├── parameter
		│         │   ├── posedge
		│         │   ├── reg
		│         │   ├── signed_unsigned
		│         │   ├── specify_endspecify
		│         │   ├── specparam
		│         │   ├── star
		│         │   ├── task_endtask
		│         │   ├── while
		│         │   ├── wire
		│         │   ├── xnor
		│         │   └── xor
		│         ├── koios
		│         │   ├── synthesis_result.json
		│         │   └── task.conf
		│         ├── large
		│         │   ├── synthesis_result.json
		│         │   └── task.conf
		│         ├── micro
		│         │   ├── simulation_result.json
		│         │   ├── synthesis_result.json
		│         │   └── task.conf
		│         ├── mixing_optimization
		│         │   ├── config_file_half
		│         │   │   ├── config_file_half.xml
		│         │   │   ├── simulation_result.json
		│         │   │   │── synthesis_result.json
		│         │   │   └── task.conf
		│         │   ├── mults_auto_full
		│         │   │   ├── simulation_result.json
		│         │   │   │── synthesis_result.json
		│         │   │   └── task.conf
		│         │   ├── mults_auto_half
		│         │   │   ├── simulation_result.json
		│         │   │   │── synthesis_result.json
		│         │   │   └── task.conf
		│         │   └── mults_auto_none
		│         │       ├── simulation_result.json
		│         │       │── synthesis_result.json
		│         │       └── task.conf
		│         ├── operators
		│         │   ├── simulation_result.json
		│         │   ├── synthesis_result.json
		│         │   └── task.conf
		│         ├── preprocessor
		│         │   ├── simulation_result.json
		│         │   ├── synthesis_result.json
		│         │   └── task.conf
		│         ├── syntax
		│         │   ├── simulation_result.json
		│         │   ├── synthesis_result.json
		│         │   └── task.conf
		│         └── vtr
		│             ├── synthesis_result.json
		│             └── task.conf
		├── third_party
		│     └── SymbiFlow
		│         ├── build.sh
		│         └── task.mk
		└── verilog
				├── binary
				├── ex1BT16_fir_20_input
				├── FIR
				├── full
				├── keywords
				├── large
				├── micro
				├── operators
				├── preprocessor
				└── syntax