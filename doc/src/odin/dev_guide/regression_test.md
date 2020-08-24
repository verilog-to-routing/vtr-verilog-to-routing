# Regression Tests

Regression tests are tests that are repeatedly executed to assess functionality.
Each regression test targets a specific function of Odin II.
There are two main components of a regression test; benchmarks and a configuration file.
The benchmarks are comprised of verilog files, input vector files and output vector files.
The configuration file calls upon each benchmark and synthesizes them with different architectures.
The current regression tests of Odin II can be found in regression_test/benchmarks.

## Benchmarks

Benchmarks are used to test the functionality of Odin II and ensure that it runs properly.
Benchmarks of Odin II can be found in regression_test/benchmarks/verilog/any_folder.
Each benchmark is comprised of a verilog file, an input vector file, and an output vector file.
They are called upon during regression tests and synthesized with different architectures to be compared against the expected results.
These tests are usefull for developers to test the functionality of Odin II after implementing changes.
The command `make test` runs through all these tests, comparing the results to previously generated results, and should be run through when first installing.

### Unit Benchmarks

Unit Benchmarks are the simplest of benchmarks. They are meant to isolate different functions of Odin II.
The goal is that if it does not function properly, the error can be traced back to the function being tested.
This cannot always be achieved as different functions depend on others to work properly.
It is ideal that these benchmarks test bit size capacity, errorenous cases, as well as standards set by the IEEE Standard for Verilog® Hardware Description Language - 2005.

### Micro Benchmarks

Micro benchmark's are precise, like unit benchmarks, however are more syntactic.
They are meant to isolate the behaviour of different functions.
They trace the behaviour of functions to ensure they adhere to the IEEE Standard for Verilog® Hardware Description Language - 2005.
Like micro benchmarks, they should check errorenous cases and behavioural standards et by the IEEE Standard for Verilog® Hardware Description Language - 2005.

### Macro Benchmarks

Macro benchmarks are more realistic tests that incorporate multiple functions of Odin II.
They are intended to simulate real-user behaviour to ensure that functions work together properly.
These tests are designed to test things like syntax and more complicated standards set by the IEEE Standard for Verilog® Hardware Description Language - 2005.

### External Benchmarks

External Benchmarks are benchmarks created by outside users to the project.
It is possible to pull an outside directory and build them on the fly thus creating a benchmark for Odin II.

## Creating Regression tests

### New Regression Test Checklist

* Create benchmarks [here](#creating-benchmarks)
* Create configuration file [here](#creating-a-configuration-file)
* Create a folder in the task directory for the configuration file [here](#creating-a-task)
* Generate the results [here](#regenerating-results)
* Add the task to a suite (large suite if generating the results takes longer than 3 minutes, otherwise put in light suite) [here](#creating-a-suite)
* Update the documentation by providing a summary in Regression Test Summary section and updating the Directory tree [here](#regression-test-summaries)

### New Benchmarks added to Regression Test Checklist

* Create benchmarks and add them to the correct regression test folder found in the benchmark/verilog directory [here](#creating-benchmarks)  (There is a description of each regression test [here](#regression-test-summaries))
* Regenerate the results [here](#regenerating-results)

### Include

* verilog file
* input vector file
* expected ouptut vector file
* configuration file (conditional)
* architecture file (optional)

### Creating Benchmarks

If only a few benchmarks are needed for a PR, simply add the benchmarks to the appropriate set of regression tests.
[The Regression Test Summary](#regression-test-summaries) summarizes the target of each regression test which may be helpful.

The standard of naming the benchmarks are as follows:

* verilog file: meaningful_title.v
* input vector file: meaningful_title_input
* output vector file: meaningful_title_output

If the tests needed do not fit in an already existing set of regression tests or need certain architecture(s), create a seperate folder in the verilog directory and label appropriately.
Store the benchmarks in that folder.
Add the architecture (if it isn't one that already exists) to ../vtr_flow/arch.

> **NOTE**
>
> If a benchmark fails and should pass, include a $display statement in the verilog file in the following format:
>
> `$display("Expect::FUNCTION < message >);`
>  
> The function should be in all caps and state what is causing the issue. For instance, if else if was behaving incorrectly it should read ELSE_IF. The message
> should illustrate what should happen and perhaps a suggestion in where things are going wrong.

### Creating a Configuration File

A configuration file is only necessary if the benchmarks added are placed in a new folder.
The configuration file is where architectures and commands are specified for the synthesis of the benchmarks.
**The configuration file must be named task.conf.**
The following is an example of a standard task.conf (configuration) file:  

```bash
########################
# <title> benchmarks config
########################

# commands
regression_params=--include_default_arch
script_synthesis_params=--time_limit 3600s 
script_simulation_params=--time_limit 3600s
simulation_params= -L reset rst -H we

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
```

The following key = value are available for configuration files:

|  key                     |following argument                                  |
|--------------------------|----------------------------------------------------|
|circuits_dir              |< path/to/circuit/dir >                             |
|circuit_list_add          |< circuit file path relative to [circuits_dir] >    |
|archs_dir                 |< path/to/arch/dir >                                |
|arch_list_add             |< architecture file path relative to [archs_dir] >  |
|synthesis_parse_file      |< path/to/parse/file >                              |
|simulation_parse_file     |< path/to/parse/file >                              |
|script_synthesis_params   | [see exec_wrapper.sh options]                      |
|script_simulation_params  |[see exec_wrapper.sh options]                       |
|synthesis_params          |[see Odin options]                                  |
|simulation_params         |[see Odin options]                                  |
|regression_params         |[see Regression Parameters bellow]

Regression Parameters:

* `--verbose`                display error logs after batch of tests
* `--concat_circuit_list`    concatenate the circuit list and pass it straight through to odin
* `--generate_bench`         generate input and output vectors from scratch
* `--disable_simulation`     disable the simulation for this task
* `--disable_parallel_jobs`  disable running circuit/task pairs in parralel
* `--randomize`             performs a dry run randomly to check the validity of the task and flow                        |
* `--regenerate_expectation`regenerates expectation and overrides thee expected value only if there's a mismatch          |
* `--generate_expectation`   generate the expectation and overrides the expectation file                                  |

### Creating a task

The following diagram illustrates the structure of regression tests.
Each regression test needs a corresponding folder in the task directory containing the configuration file.
The \<task display name\> should have the same name as the verilog file group in the verilog directory.
This folder is where the synthesis results and simulation results will be stored.
The task diplay name and the verilog file group should share the same title.

```bash
└── ODIN_II
      └── regression_test
              └── benchmark
                    ├── task
                    │     └── < task display name >
                    │             ├── [ simulation_result.json ]
                    │             ├── [ synthesis_result.json ]
                    │             └── task.conf
                    └── verilog
                          └── < verilog file group >
                                  ├── *.v
                                  └── *.vh
```

#### Creating a Complicated Task

There are times where multiple configuration files are needed in a regression test due to different commands wanted or architectures.
The task cmd_line_args is an example of this.
If that is the case, each configuration file will still need its own folder, however these folder's should be placed in a parent folder.

```bash
└── ODIN_II
      └── regression_test
              └── benchmark
                    ├── task
                    │     └──  < parent task display name >
                    |              ├── < task display name >
                    │              │             ├── [ simulation_result.json ]
                    │              │             ├── [ synthesis_result.json ]
                    |              │             └── task.conf
                    │              └── < task display name >
                    │              .             ├── [ simulation_result.json ]
                    │              .             ├── [ synthesis_result.json ]
                    |              .             └── task.conf
                    └── verilog
                          └── < verilog file group >
                                   ├── *.v
                                   └── *.vh
```

#### Creating a Suite

Suites are used to call multiple tasks at once. This is handy for regenerating results for multiple tasks.
In the diagram below you can see the structure of the suite.
The suite contains a configuration file that calls upon the different tasks **named task_list.conf**.  

```bash
└── ODIN_II
      └── regression_test
              └── benchmark
                    ├── suite
                    │     └──  < suite display name >
                    |              └── task_list.conf
                    ├── task
                    │     ├── < parent task display name >
                    |     │        ├── < task display name >
                    │     │        │             ├── [ simulation_result.json ]
                    │     │        │             ├── [ synthesis_result.json ]
                    |     │        │             └── task.conf
                    │     │        └── < task display name >
                    │     │                      ├── [ simulation_result.json ]
                    │     │                      ├── [ synthesis_result.json ]
                    |     │                      └── task.conf
                    │     └── < task display name >
                    │              ├── [ simulation_result.json ]
                    │              ├── [ synthesis_result.json ]
                    |              └── task.conf  
                    └── verilog
                          └── < verilog file group >
                                   ├── *.v
                                   └── *.vh
```

In the configuration file all that is required is to list the tasks to be included in the suite with the path.
For example, if the wanted suite was to call the binary task and the operators task, the configuration file would be as follows:

```bash
regression_test/benchmark/task/operators
regression_test/benchmark/task/binary
```

For more examples of task_list.conf configuration files look at the already existing configuration files in the suites.

### Regenerating Results

> **WARNING**
>
> **BEFORE** regenerating the result, run `make test` to ensure any changes in the code don't affect the results of benchmarks beside your own. If they do, the failing benchmarks will be listed.

Regenerating results is necessary if any regression test is changed (added benchmarks), if a regression test is added, or if a bug fix was implemented that changes the results of a regression test.
For all cases, it is necessary to regenerate the results of the task corresponding to said change.
The following commands illustrate how to do so:

```shell
make sanitize
```

then: where N is the number of processors in the computer, and the path following -t ends with the same name as the folder you placed

```shell
./verify_odin.sh -j N --regenerate_expectation -t regression_test/benchmark/task/<task_display_name>
```

> **NOTE**
>
> **DO NOT** run the `make sanitize` if regenerating the large test. It is probable that the computer will not have a enough ram to do so and it will take a long time. Instead run `make build`

For more on regenerating results, refer to the [Verify Script](./verify_script.md) section.

## Regression Test Summaries

### c_functions

This regression test targets c functions supported by Verilog such as clog_2.

### cmd_line_args

This is a more complicated regression test that incorporates multiple child tasks.
It targets different commands available in Odin II.
Although it doesn't have a dedicated set of benchmarks in the verilog folder, the configuration files call on different preexisting benchmarks.

### FIR

FIR is an acronym for "Finite Impulse Response".
These benchmarks were sourced from [Layout Aware Optimization of High Speed Fixed Coefficient FIR Filters for FPGAs](http://kastner.ucsd.edu/fir-benchmarks/?fbclid=IwAR0sLk_qaBXfeCeDuzD2EWBrCJ_qGQd7rNISYPemU6u98F6CeFjWOMAM2NM).
They test a method of implementing high speed FIR filters on FPGAs discussed in the paper.

### full

The full regression test is designed to test real user behaviour.  
It does this by simulating flip flop, muxes and other common uses of Verilog.  

### large

This regression test targets cases that require a lot of ram and time.  

### micro

The micro regression tests targets hards blocks and pieces that can be easily instantiated in architectures.

### mixing_optimization

The mixing optimization regression tests targets mixing implementations for operations implementable in hard blocks and their soft logic counterparts that can be can be easily instantiated in architectures.

### operators

This regression test targets the functionality of different opertators. It checks bit size capacity and behaviour.  

### syntax

The syntax regression tests targets syntactic behaviour. It checks that functions work cohesively together and adhere to the verilog standard.

### keywords

This regression test targets the function of keywords. It has a folder or child for each keyword containing their respective benchmarks. Some folders have benchmarks for two keywords like task_endtask because they both are required together to function properly.

### preprocessor

This set of regression test includes benchmarks targetting compiler directives available in Verilog.

### Regression Tests Directory Tree

```bash
benchmark
      ├── suite
      │     ├── complex_synthesis_suite
      │     │   └── task_list.conf
      │     ├── full_suite
      │     │   └── task_list.conf
      │     ├── heavy_suite
      │     │   └── task_list.conf
      │     └── light_suite
      │         └── task_list.conf
      ├── task
      │     ├── arch_sweep
      │     │   ├── synthesis_result.json
      │     │   └── task.conf
      │     ├── binary
      │     │   ├── simulation_result.json
      │     │   ├── synthesis_result.json
      │     │   └── task.conf
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
      │     ├── full
      │     │   ├── simulation_result.json
      │     │   ├── synthesis_result.json
      │     │   └── task.conf
      │     ├── keywords
      |     |   ├── always
      │     │   ├── and
      |     |   ├── assign
      |     |   ├── at_parenthathese
      |     |   ├── automatic
      |     |   ├── begin_end
      |     |   ├── buf
      |     |   ├── case_endcase
      |     |   ├── default
      |     |   ├── defparam
      |     |   ├── else
      |     |   ├── for
      |     |   ├── function_endfunction
      |     |   ├── generate
      |     |   ├── genvar
      |     |   ├── if
      |     |   ├── initial
      |     |   ├── inout
      |     |   ├── input_output
      |     |   ├── integer
      |     |   ├── localparam
      |     |   ├── macromodule
      |     |   ├── nand
      |     |   ├── negedge
      |     |   ├── nor
      |     |   ├── not
      |     |   ├── or
      |     |   ├── parameter
      |     |   ├── posedge
      |     |   ├── reg
      |     |   ├── signed_unsigned
      |     |   ├── specify_endspecify
      |     |   ├── specparam
      |     |   ├── star
      |     |   ├── task_endtask
      |     |   ├── while
      |     |   ├── wire
      |     |   ├── xnor
      │     │   └── xor
      │     ├── large
      │     │   ├── synthesis_result.json
      │     │   └── task.conf
      │     ├── micro
      │     │   ├── simulation_result.json
      │     │   ├── synthesis_result.json
      │     │   └── task.conf
      │     ├── mixing_optimization
      |     |   ├── mults_auto_full
      │     │   |   ├── simulation_result.json
      │     │   |   |── synthesis_result.json
      │     │   |   └── task.conf
      |     |   ├── mults_auto_half
      │     │   |   ├── simulation_result.json
      │     │   |   |── synthesis_result.json
      │     │   |   └── task.conf
      |     |   ├── mults_auto_none
      │     │   |   ├── simulation_result.json
      │     │   |   |── synthesis_result.json
      │     │   |   └── task.conf
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
      │     └── vtr
      │       └── task.conf
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
```
