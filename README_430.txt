This archive contains VPR, an FPGA placement and routing tool, and T-VPack,
a program to pack LUTs and flip flops into coarser grained logic blocks and
convert a netlist from blif format to VPR's .net format.  There is a detailed
manual covering everything from how to compile the programs
to how to use them in manual_430.ps (and manual_430.pdf if you prefer .pdf
format).

To see VPR in action on the (small) sample circuit (e64 from the MCNC 
benchmark suite) and the sample FPGA architecture files type:

cd vpr (if not already there)
vpr e64-4x4lut.net 4x4lut_sanitized.arch e64.p e64.r -route_chan_width 40 -inner_num 3

(Add -nodisp to the command line above if you're not running on an X-Windows
graphics capable computer.)

The logic block for this architecture contains 4, 4-input look-up tables and
4 flip flops along with local routing (a cluster-based logic block of size
4).  The routing wire segments all span 4 logic blocks in this architecture.


I've also included a simpler architecture in which a logic block is only a
4 LUT plus a flip flop, and in which all routing wires span only one logic
block.  To run this case, try:

cd vpr (if not already there)
vpr e64-4lut.net 4lut_sanitized.arch e64.p e64.r -route_chan_width 10 -inner_num 3


The two netlists (e64-4x4lut.net and e64-4lut.net) were created using T-VPack
on a technology-mapped netlist in .blif format, with the appropriate options
in each case:

cd t-vpack  (if not already there)
t-vpack e64.blif e64-4x4lut.net -cluster_size 4 -inputs_per_cluster 10

and

cd t-vpack  (if not already there)
t-vpack e64.blif e64-4lut.net -no_clustering



There are a lot of different ways to use VPR and T-VPack -- see manual_430.ps
or manual_430.pdf for details.

-- Vaughn Betz, March 25, 2000


==============================================================================
Contents of the archive:

README_430.txt:  This file.
manual_430.ps:  Postscript manual of VPR and T-VPack.
manual_430.pdf:  PDF version of the manual.

./vpr

  *.c, *.h:  Source code for VPR.

  makefile:  Makefile for VPR.  Currently set for Solaris and gcc; you may
             have to modify the library paths and compiler options on your
             machine.

  descript.txt:  A revision history of VPR.
  
  e64-4x4lut.net:  A sample netlist file from the MCNC bencmark set.  The logic
            block contains 4 4-input LUTs and 4 FFs.

  e64-4lut.net:  A sample netlist file from the MCNC benchmark set.  The logic
            block contains 1 4-LUT and 1 FF.

  4x4lut_sanitized.arch:  A sample FPGA architecture file, with a logic block
            containing 4 4-input LUTs and 4 FFs.

  4lut_sanitized.arch:  A sample FPGA architecture file, with a logic block
            containing 1 4-LUT and 1 FF.


./vpack

  *.c, *.h:  Source code for T-VPack.

  makefile:  Makefile for T-VPack.  May have to be modified for non Solaris
             machines or for compilers other than gcc.

  descript.txt:  Revision history of VPack / T-VPack.

  e64.blif:  MCNC benchmark circuit e64 technology-mapped by Flowmap to 
             4-input LUTs.  This is a combinational circuit.

  s1423.blif:  A sequential MCNC benchmark circuit.  It has been technology-
               mapped to 4-input LUTs and flip flops by Flowmap.


==============================================================================

Major changes from VPR Version 4.22 to this Version (4.30),
and from VPack Version 2.09 to T-VPack Version 4.30 are:

1) VPack has been made timing-driven, and it's name has been changed
   to T-VPack.  It can still be run in the old, non-timing driven mode
   (specify -timing_driven off), but the timing-driven algorithm gets
   better timing and routability than the old algorithm.  The timing-driven
   packing algorithm is used by default.  Sandy Marquardt wrote the new,
   timing-driven algorithm in T-VPack.

2) The VPR placer algorithm has been made timing-driven, again by Sandy
   Marquardt.  The timing-driven mode is the default.  Running the placer
   in timing-driven mode speeds up the typical circuit by about 25%
   while only costing about 5% more routing.
