.. _regression_test:

Regression Tests
================
Yosys+Odin-II utilizes the same set of benchmarks that Odin-II uses to assess it's functionality.
The benchmarks are comprised of Verilog files, input vector files and output vector files.
Due to different approaches taken by Yosys+Odin-II and Odin-II synthesizers and some unsupported Verilog features by Odin-II, each synthesizer has its output vector files, named using ``XXX_odin_input/output`` and ``XXX_yosys_input/output`` name convention.
In some cases, both output vectors are identical while they differ for the remainder of them.
The configuration file calls upon each benchmark and synthesizes them with different architectures.
The current regression tests' Verilog files can be found in `regression_test/benchmark/verilog`.

Benchmarks
----------

Except for the common benchmarks, located in `regression_test/benchmark/verilog/common`, which are Yosys basic benchmarks to verify its functionality, Yosys+Odin-II mainly utilizes Odin-II's benchmarks that can be found in `regression_test/benchmark/verilog/any_folder`.
Yosys+Odin-II has its own tasks and suites which are located in `regression_test/benchmark/task/yosys+odin` and `regression_test/benchmark/suite/yosys+odin` directories.
Each benchmark is comprised of a Verilog file, an input vector file, and an output vector file.
They are called upon during regression tests and synthesized with different architectures to be compared against the expected results.
These tests are useful for developers to test the functionality of Yosys+Odin-II after implementing changes.
The command ``make test ELABORATOR=yosys`` runs through all these tests, comparing the results to previously generated results.

Creating Regression Tests
-------------------------

The regression test creating process for Yosys+Odin-II is similar to that of Odin-II.
For more information please see the Odin-II `Regression Tests <https://docs.verilogtorouting.org/en/latest/odin/dev_guide/regression_test/#>`_.

.. _example_yosys_odin_configuration_file:

Example of a Yosys+Odin-II Configuration File
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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


.. _yosys_odin_task:

Yosys+Odin-II Task Structure
----------------------------

The Yosys+Odin-II tasks follow the Odin-II task structure, which can be found at `Creating a Task <https://docs.verilogtorouting.org/en/latest/odin/dev_guide/regression_test/#creating-a-task>`_. 
The following diagram illustrates the structure of Yosys+Odin-II regression tests.

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


.. _yosys_odin_suite:

Yosys+Odin-II Suite Structure
-----------------------------

The process of creating a suite for Yosys+Odin-II is completely identical to Odin-II. 
For more information on how Odin-II suites are created please see `Creating a Suite <https://docs.verilogtorouting.org/en/latest/odin/dev_guide/regression_test/#creating-a-suite>`_. 
In the diagram below you can see the structure of the suites allocated for the Yosys+Odin-II tasks.

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

.. _regenerating_results:

Regenerating Results
--------------------

.. note::

	**BEFORE** regenerating the result, run ``make test ELABORATOR=yosys`` to ensure any changes in the code don't affect the results of benchmarks beside your own. If they do, the failing benchmarks will be listed.

Regenerating results is necessary if any regression test is changed (added benchmarks), if a regression test is added, or if a bug fix was implemented that changes the results of a regression test.
The process of regenerating the expectation results for Yosys+Odin-II benchmarks using ``verify_script.sh`` is similar to Odin-II.
Please visit `Regenerating Results <https://docs.verilogtorouting.org/en/latest/odin/dev_guide/regression_test/#regenerating-results>`_ and `Verify Script <https://docs.verilogtorouting.org/en/latest/odin/dev_guide/verify_script/>`_ sections in the Odin-II developers guide to see how to regenerate expectation results using the verification script.

If the developer changes require regenerating all regression tests' expectation results, you can use the makefile option created specifically for this purpose.
In the following, you can see an example of how to regenerate the expectation results of all regression tests for a specific synthesizer.
It is assumed the command is run in the Odin-II root directory.

.. code-block:: bash

	make regenerate_expectation ELABORATOR=yosys

The default value of the ``ELABORATOR`` is considered ``odin``, which means if you run the command mentioned above without specifying the elaborator, it will regenerate Odin-II expectation results.


Yosys+Odin-II Regression Tests Directory Tree
---------------------------------------------

.. code-block::

	benchmark
		├── suite
		│     ├── complex_synthesis_suite
		│     ├── full_suite
		│     ├── heavy_suite
		│     ├── light_suite
		│     └── yosys+odin
		│          ├── techmap_heavysuite	
		│          ├── techmap_keywords_suite	
		│          └── techmap_lightsuite	
		├── task
		│     ├── arch_sweep
		│     ├── c_functions
		│     ├── FIR
		│     ├── fpu
		│     ├── full
		│     ├── keywords
		│     ├── koios
		│     ├── large
		│     ├── micro
		│     ├── mixing_optimization
		│     ├── operators
		│     ├── preprocessor
		│     ├── syntax
		│     ├── vtr
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
		│         │   ├── hardlogic
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
		│   
		└── verilog