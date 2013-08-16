+--------------------------+
|  Toro - a VPR front-end  |
+--------------------------+

This directory contains Toro source code.

Toro is a front-end to VPR that provides a number of functional
enhancements. Features include:

 * Fabric Definition - Lets you define and/or override a FPGA fabric. 

   For example...

   ­ Non-uniform physical blocks: You can assign physical blocks types
     to specific positions within the FPGA grid. Furthermore, blocks may
     span multiple FPGA grid rows and columns
   ­ Non-uniform IO blocks: You can assign IO block types anywhere 
     within the FPGA grid.
   ­ Non-default switch boxes: You can override side mapping for any
     switch box.
   ­ Non-default connection blocks: You can override connection block
     patterns for any block.
   ­ Non-default channel widths: You can define variable channel widths 
     across a FPGA.

 * Circuit Description - Lets you override a FPGA physical design.

   For example...

   ­ Region-based placement: You can limit physical or IO block
     placement to specific regions.
   ­ Relative placement macros: You can define placement macro
     constraints (such as a carry-chain) based on relative positions
     between blocks in a macro.
   ­ Pre-placed blocks: You can pre-place physical and/or IO blocks.
   ­ Pre-routed nets: You can pre-route nets.

 * Program Options - Lets you define and/or override run-time options.

   For example...

   ­ Input and output files: You can override specific input or output
     file names.
   ­ Message control: You can override message warning vs. error types. 
     In addition, you have the ability to accept or reject messages
     based on type and/or content.
   ­ Program execution: You can limit execution based on number of
     acceptable warnings or errors.
   ­ Trace files: You can select and name specific VPR trace echo files.

For more information on Toro, refer to the "Toro User Reference Guide"
(toro_uguide_*.docx).

To build on Linux RedHat...

   1. "setenv ISP linux_x86_64"
   2. "cd toro"
   3. "make Optimized"
      -or-
    . "make Debug"
   4. "cd .."

   Note: Building Toro on Linux assumes the following libraries exist:
         ../libcommon_c++/pcre/libpcre.a
         ../libcommon_c/libcommon_c++.a
         ../libpccts_133MR7/linux_x86_64/libh.a
         ../libpccts_133MR7/linux_x86_64/libpcctsGeneric.a
         ../vpr/linux_x86_64/libvpr.a

To build on Linux ubuntu...

   1. "export ISP=linux_i686"
   2. "cd toro"
   3. "make"
   4. "cd .."

   Note: Building Toro on Linux assumes the following libraries exist
         ../libcommon_c++/pcre/libpcre.a
         ../libcommon_c/libcommon_c++.a
         ../libpccts_133MR7/linux_i686/libh.a
         ../libpccts_133MR7/linux_i686/libpcctsGeneric.a
         ../vpr/linux_i686/libvpr.a

To build on Windows VisualC++...

   1. run VisualC++ (2010 or 2012)
   2. open "toro/toro.sln"
   3. build
