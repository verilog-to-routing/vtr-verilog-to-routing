##############################################################################
# Verilog-to-Routing
#   FPGA Architecture and CAD Exploration Framework
##############################################################################

This archive contains a free academic framework, called Verilog-to-Routing 
(VTR), that you can use to explore future FPGAs and CAD algorithms for FPGAs.  
This framework includes benchmark circuits (in Verilog), example FPGA 
architectures, and a flow that maps a set of benchmark circuits to the FPGA 
architecture of your choosing.

To build all the CAD tools used in this framework, enter "make" in the same 
directory as this readme.  To run a quick pass/fail test on each tool, enter 
"run_quick_test.pl".

The following steps show you to run a simple VTR flow on a sample circuit to
an FPGA architecture that contains embedded memories and multipliers:

1.  In your "vtr_release/vtr_flow/tasks" directory, enter the following command:

	../scripts/run_vtr_task.pl basic_flow/

    This command will run the VTR flow on a single circuit and a single 
    architecture.  The files generated from the run are stored in 
    basic_flow/run[#] where [#] is the number of runs you have done.  
    Since this is your first time running the flow, the results will be stored 
    in basic_flow/run1.  When the script completes, enter the following command:

	../scripts/parse_vtr_task.pl basic_flow/

    This parses out the information of the VTR run and outputs the results in 
    a text file called run[#]/parse_results.txt.

    More info on how to run the flow on multiple circuits and architectures along 
    with different options later.  Before that, we need to ensure that the 
    run that you have done works.

2.  The basic_flow comes with golden results that you can use to check for
    correctness.  To do this check, enter the following command:

	../scripts/parse_vtr_task.pl -check_golden basic_flow/

    It should return: basic_flow/...[Pass]

    Note: Due to the nature of the algorithms employed, the measurements that 
    you get may not match exactly with the golden measurements.  We included 
    margins in our scripts to account for that noise during the check.  We also
    included runtime estimates based on our machine (Intel Xeon 5160 3 GHz
    processor with 8 GB RAM).  The actual runtimes that you get may differ 
    dramatically from these values.

3.  To see precisely which circuits, architecture, and CAD flow was employed by 
    the run, go to the directory 
    "vtr_release/vtr_flow/tasks/basic_flow/config/config.txt".  Inside this 
    directory, the "config.txt" file contains the circuits and architecture file 
    employed in the run.  Some also contain a golden results file that is used 
    by the scripts to check for correctness.  
    
    The "vtr_release/vtr_flow/scripts/run_vtr_flow.pl" 
    script describes the CAD flow employed in the test.  You can modify the flow
    by editing this script.

    At this point, feel free to run any of the tasks pre-pended with "regression".
    These are regression tests included with the flow that test various combinations of
    flows, architectures, and benchmarks.

4.  For more information on how the vtr_flow infrastructure works (and how to 
    add the tests that you want to do to this infrastructure), please read the 
    vtr_flow documentation in the vtr wiki 
    (https://vtr.readthedocs.io/en/latest/).


==============================================================================
Contents of the archive:

Tools: 

	ODIN II: An open-source Verilog elaborator

	ABC: An open-source logic synthesis tool modified to: 1) use the new 
	FPGA wiremap algorithm 2) not use the readline libraries

	VPR: An academic FPGA packing, placement, and routing tool

Libraries:

	libarchfpga: Library used by ODIN II and VPR.  This library reads an FPGA 
	architecture description file

Other:

	vtr_flow: Run the vtr flow on a set of Verilog benchmarks to a set of 
	FPGA architectures. Measures quality metrics from the output of those 
	experiments. [Optional] For regression tests, also compares with golden 
	results (with an error tolerance for algorithmic noise).

	run_quick_test.pl: Script that checks if the tools in this archive were 
	properly built.

	quick_test: Used by run_quick_test.pl to check build

===============================================================================

Important Notes:

- VPR graphics turned off by default to help with compilation.  You can turn it 
back on again when building by removing the -DNO_GRAPHICS option in the Makefile.

Bug Reporting:

Although we have made a determined effort to fix and prevent bugs, there will 
inevitably be bugs in an undertaking of this magnitude.  Please report bugs/issues to our 
issue tracker on http://code.google.com/p/vtr-verilog-to-routing/issues/list.

Build Help:

- This release has been regularly tested on a 64-bit Linux (Debian) machine.  
Our testers have gotten this to work on various other systems including various versions of Windows and Unix.

- If you are compiling in cygwin, you may need to add "#define WIN32" to libarchfpga/include/ezxml.h

- Check for library dependencies.  ODIN II requires bison.  VPR requires X11 libraries if you want graphics.  

Other Notes:

Please support us by registering as a user of VTR here: http://www.eecg.utoronto.ca/vtr/register.html

##############################################################################
# License
##############################################################################

Copyright (c) 2013
Jason Luu, Jeffrey Goeders, Chi Wai Yu, Opal Densmore, Andrew Somerville, 
Kenneth Kent, Peter Jamieson, Jason Anderson, Ian Kuon, Alexander Marquardt, 
Andy Ye, Ted Campbell, Wei Mark Fang, Vaughn Betz, Jonathan Rose

Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of 
the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS 
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER 
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

##############################################################################
# Contributors
##############################################################################

Professors: Kenneth Kent, Peter Jamieson, Jason Anderson, Vaughn Betz, Jonathan Rose

Graduate Students: Jason Luu, Jeffrey Goeders, Chi Wai Yu, Andrew Somerville, 
Ian Kuon, Alexander Marquardt, Andy Ye, Wei Mark Fang, Tim Liu

Summer Students: Opal Densmore, Ted Campbell, Cong Wang, Peter Milankov, Scott Whitty, Michael Wainberg,
Suya Liu, Miad Nasr, Nooruddin Ahmed, Thien Yu

Companies: Altera Corporation, Texas Instruments

