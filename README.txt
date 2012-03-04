This is the development trunk for the Verilog-to-Routing project.  Unlike the nicely packaged release that we create, you are working with code in a constant state of flux.  You should expect that the tools are not always stable and that more work is needed to get the flow to run.

The VTR framework consists of 5 major parts: Odin II, ABC, libvpr, vpr, and vtr_flow.  When we release VTR, these projects are packaged together with scripts that make these parts talk to each other.  You download VPR and vtr_flow on this site.  You can download the most recent version of Odin II and libvpr here: (http://code.google.com/p/odin-ii/).  The most recent version of abc here: (https://bitbucket.org/alanmi/abc/src)

You need to organize the directory structure such that the Odin II and ABC folders are in the same directory as vpr and vtr_flow as follows:

odin-ii/ODIN_II/ODIN_II goes to ODIN_II
odin-ii/ODIN_II/libvpr_6 goes to libvpr
abc goes to abc_with_bb_support

So at the end, you should see:

abc_with_bb_support
ODIN_II
libvpr
vtr_flow
vpr

in the same directory as the Makefile

You don't need to use the latest and greatest versions of each tool.  At the end of the day, what's important is that the directory structure must match the release directory structure.


 