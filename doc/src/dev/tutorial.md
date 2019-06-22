# New Developer Tutorial #

## Overview ##

Welcome to the Verilog-to-Routing (VTR) Project. This project is an open-source, international, collaboration towards a comprehensive FPGA architecture exploration system that includes CAD tools, benchmarks, transistor-optimized architecture files, and documentation, along with support to make this all fit together and work. The purpose of this tutorial is to equip you, the new developer, with the tools and understanding that you need to begin making a useful contribution to this project.

While you are going through this tutorial, please record down things that should be changed. Whether it is the tutorial itself, documentation, or other parts of the VTR project. Your thoughts are valuable and welcome because fresh eyes help evaluate whether or not our work is clearly presented.


## Environment Setup ##

Log into your workstation/personal computer. Check your account for general features such as internet, printing, git, etc. If there are problems at this stage, talk to your advisor to get this setup.

If you are not familiar with development on Linux, this is the time to get up to speed. Look up online tutorials on general commands, basic development using Makefiles, etc.

## Background Reading ##

Read the first two chapters of "Architecture and CAD for deep-submicron FPGAs" by Vaughn Betz, et al. This is a great introduction to the topic of FPGA CAD and architecture. Note though that this book is old so it only covers a small core of what the VTR project is currently capable of.

Read chapters 1 to 5 of "FPGA Architecture: Survey and Challenges" by Ian Kuon et al.

Review material learned with fellow colleagues.

## Setup VTR ##

Use git to clone a copy of VTR from the [GitHub repository](https://github.com/verilog-to-routing/vtr-verilog-to-routing)

Build the project by running the `make` command

Run `./run_quick_test.pl` to check that the build worked

Follow the Basic Design Flow Tutorial found in the Tutorials section of the [Welcome to Verilog-to-Routing's documentation!](https://vtr.readthedocs.io/en/latest). This tutorial will allow you to run a circuit through the entire flow and read the statistics gathered from that run.

## Use VTR ##

Create your own custom Verilog file. Create your own custom architecture file using one of the existing architecture files as a template. Use VTR to map that circuit that you created to that architecture that you created. The VTR documentation, to be found at the [Welcome to Verilog-to-Routing's documentation!](https://vtr.readthedocs.io/en/latest/) will prove useful. You may also wish to look at the following links for descriptions of the language used inside the architecture files:
  * [Architecture Description and Packing](http://www.eecg.utoronto.ca/~jluu/publications/luu_vpr_fpga2011.pdf)
  * [Classical Soft Logic Block Example](http://www.eecg.utoronto.ca/vpr/utfal_ex1.html)

Perform a simple architecture experiment. Run an experiment that varies Fc_in from 0.01 to 1.00 on the benchmarks ch_intrinsics, or1200, and sha.  Use `tasks/timing` as your template.  Graph the geometric average of minimum channel width and critical path delay for these three benchmarks across your different values of Fc_in. Review the results with your colleagues and/or advisor.

## Open the Black Box ##

At this stage, you have gotten a taste of how an FPGA architect would go about using VTR.  As a developer though, you need a much deeper understanding of how this tool works.  The purpose of this section is to have you to learn the details of the VTR CAD flow by having you manually do what the scripts do.

Using the custom Verilog circuit and architecture created in the previous step, directly run Odin II on it to generate a blif netlist.  You may need to skim the `ODIN_II/README.rst` and the `vtr_flow/scripts/run_vtr_flow.pl`.


Using the output netlist of Odin II, run ABC to generate a technology-mapped blif file.  You may need to skim the [ABC homepage](http://www.eecs.berkeley.edu/~alanmi/abc/).
```shell
# Run the ABC program from regular terminal (bash shell)
$VTR_ROOT/abc abc

# Using the ABC shell to read and write blif file
abc 01> read_blif Odin_II_output.blif
abc 01> write_blif abc_output.blif
```

Using the output of ABC and your architecture file, run VPR to complete the mapping of a user circuit to a target architecture.  You may need to consult the VPR User Manual.

```shell 
# Run the VPR program 
$VTR_ROOT/vpr vpr architecture.xml abc_output.blif
```

Read the VPR section of the online documentation.

## Submitting Changes and Regression Testing ##

Read `README.developers.md` in the base directory of VTR. Code changes rapidly so please help keep this up to date if you see something that is out of date.

Make your first change to git by modifying `README.txt` and pushing it.  I recommend adding your name to the list of contributors.  If you have nothing to modify, just add/remove a line of whitespace at the bottom of the file.


Now that you have completed the tutorial, you should have a general sense of what the VTR project is about and how the different parts work together.  It's time to talk to your advisor to get your first assignment.






