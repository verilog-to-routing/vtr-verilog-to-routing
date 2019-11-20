################################################
# Readme: VPR Quick Start                      #
################################################

VPR is a CAD tool that is used to pack, place, and route a technology mapped 
user circuit to an FPGA architecture.

# Build VPR

VPR has support for both Unix/Linux/MAC and Windows environments.

For Unix/Linux Users:

Enter "make" in the vpr directory to build VPR by itself.  Alternatively, if you 
are working in the VTR framework, enter make in the parent directory to build 
all tools used in the framework (including VPR).

For Windows Users:

Open the Microsoft Visual Studio 2010 project called VPR.sln.  Build in that IDE and run.

# How to use VPR

Enter "./vpr sample_arch.xml or1200.latren.blif" in this directory to run VPR on a sample circuit and architecture file.

Enter "./vpr" by itself for a comprehensive list of options that can be used with VPR.

Read the VPR User Manual for detailed information on how to use VPR and what the 
different options do.

If you want more information about research publications, helpful 
infrastructure, news, etc on VPR, please refer to 
http://www.eecg.utoronto.ca/vpr/.

# Final Notes

If you would like to submit a bug report, please do so on the VTR issue tracker website: 
http://code.google.com/p/vtr-verilog-to-routing/issues/list

If you have questions or comments, please e-mail us at vpr@eecg.utoronto.ca

