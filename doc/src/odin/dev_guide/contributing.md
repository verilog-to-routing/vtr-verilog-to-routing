# Contributing

The Odin II team welcomes outside help from anyone interested.
To fix issues or add a new feature submit a PR or WIP PR following the provided guidelines.  

## Creating a Pull Request (PR)

**Important** Before creating a Pull Request (PR), if it is a bug you have happened upon and intend to fix make sure you create an issue beforhand.

Pull requests are intended to correct bugs and improve Odin's performance.
To create a pull request, clone the vtr-verilog-rooting repository and branch from the master.
Make changes to the branch that improve Odin II and correct the bug.
**Important** In addition to correcting the bug, it is required that test cases (benchmarks) are created that reproduce the issue and are included in the regression tests.
An example of a good test case could be the benchmark found in the "Issue" being addressed.
The results of these new tests need to be regenerate. See [regression tests](./regression_tests) for further instruction.
Push these changes to the cloned repository and create the pull request.
Add a description of the changes made and reference the "issue" that it corrects. There is a template provided on GitHub.

### Creating a "Work in progress" (WIP) PR

**Important** Before creating a WIP PR, if it is a bug you have happened upon and intend to fix make sure you create an issue beforhand.

A "work in progress" PR is a pull request that isn't complete or ready to be merged.
It is intended to demonstrate that an Issue is being addressed and indicates to other developpers that they don't need to fix it.
Creating a WIP PR is similar to a regular PR with a few adjustements.
First, clone the vtr-verilog-rooting repository and branch from the master.
Make changes to that branch.
Then, create a pull request with that branch and **include WIP in the title.**
This will automatically indicate that this PR is not ready to be merged.
Continue to work on the branch, pushing the commits regularly.
Like a PR, test cases are also needed to be included through the use of benchmarks.
See [regression tests](./regression_tests) for further instruction.

### Formating

Odin II shares the same contributing philosophy as [VPR](https://docs.verilogtorouting.org/en/latest/dev/contributing/contributing/).
Most importantly PRs will be rejected if they do not respect the coding standard: see [VPRs coding standard](https://docs.verilogtorouting.org/en/latest/dev/developing/#code-formatting)

## Odin II's Flow

Odin II functions by systematically executing a set of steps determined by the files and arguments passed in.
The figure below illustrates the flow of Odin II if a Verilog File is passed, with an optional FPGA Architecture Specification File.
The simulator is only activated if an Input Vector file is passed in which creates the Output Vector File.

.. graphviz ::
digraph G {
    0 [label="Verilog HDL File",shape=plaintext];
    2 [label="Input Vector File",shape=plaintext];
    3 [label="Output Vector File",shape=diamond];
    4 [label="FPGA Architecture Specification File",shape=plaintext];
    5 [label="Build Abstract Syntax Tree",shape=box];
    6 [label="Elaborate AST",shape=box];
    7 [label="Build Netlist",shape=box];
    8 [label="Partial Mapping",shape=box];
    10 [label="Simulator",shape=box];
    11 [label="Output Blif",shape=diamond];

    0 -> 5 -> 6 -> 7 -> 8
    7->10 [color=purple]
    4->8  [style=dotted] [color=purple]
    8->11
    4->10 [style=dotted] [color=purple]
    2->10 [color=purple]
    10->3 [color=purple]
}

Currently, BLIF files being passed in are only used for simulation; no partial mapping takes place.
The flow is depicted in the figure below.

.. graphviz ::
digraph G {
    0 [label="Input Blif File",shape=plaintext];
    1 [label="Read Blif",shape=box];
    3 [label="Build Netlist",shape=box];
    4 [label="Output Blif",shape=diamond];
    5 [label="Simulator",shape=box];
    6 [label="FPGA Architecture Specification File",shape=box];
    7 [label="Input Vector File",shape=plaintext];
    8 [label="Output Vector File",shape=diamond];

    0->1->3
    3->5 [color=purple]
    3->4
    5->8 [color=purple]
    7->5 [color=purple]
    6->5 [style=dotted] [color=purple]
}

### Building the Abstract Syntax Tree (AST)

Odin II uses Bison and Flex to parse a passed Verilog file and produce an Abstract Syntax Tree for each module found in the Verilog File.
The AST is considered the "front-end" of Odin II.
Most of the code for this can be found in verilog_bison.y, verilog_flex.l and parse_making_ast.cpp located in the ODIN_II/SRC directory.

### AST Elaboration

In this step, Odin II parses through the ASTs and elaborates specific parts like for loops, function instances, etc.
It also will simplify the tree and rid itself of useless parts, such as an unused if statement.
It then builds one large AST, incorporating each module.
The code for this can mostly be found in ast_elaborate.cpp located in the ODIN_II/SRC directory.

> **NOTE**
>
> These ASTs can be viewed via graphviz using the command -A. The file(s) will appear in the main directory.

### Building the Netlist

Once again, Odin II parses through the AST assembling a Netlist.
During the Netlist creation, pins are assigned and connected.
The code for this can be found in netlist_create_from_ast.cpp located in the ODIN_II/SRC directory.

> **NOTE**
>
> The Netlist can be viewed via graphviz using the command -G. The file will appear in the main directory under net.dot.

### Partial Mapping

During partial mapping, Odin II maps the logic using an architecture.
If no architecture is passed in, Odin II will create the soft logic and use LUTs for mapping.
However, if an architecture is passed, Odin II will map accordingly to the available hard blocks and LUTs.
It uses a combination of soft logic and hard logic.

### Simulator

The simulator of Odin II takes an Input Vector file and creates an Output Vector file determined by the behaviour described in the Verilog file or BLIF file.

## Useful tools of Odin II for Developers

When making improvements to Odin II, there are some features the developer should be aware of to make their job easier.
For instance, Odin II has a -A and -G command that prints the ASTs and Netlist viewable with GraphViz.
These files can be found in the ODIN_II directory.
This is very helpful to visualize what is being created and how everything is related to each other in the Netlist and AST.

Another feature to be aware of is ``make test``.
This build runs through all the regression tests and will list all the benchmarks that fail.
It is important to run this after every major change implemented to ensure the change only affects benchmarks it was intended to effect (if any).
It sheds insight on what needs to be fixed and how close it is to being merged with the master.
