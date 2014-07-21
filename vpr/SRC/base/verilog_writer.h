/*
verilog_writer.c defines the main functions used to:

1) identify the primitives in a design
2) find the connectivity between primitives
3) Write the verilog code representing the post-synthesized (packed,placed,routed) design consisting of LUTs, IOs, Flip Flops, Multiplier blocks, and RAM blocks
4) Write the Standard Delay Format(SDF) file corresponding to the verilog code mentioned in (3). The SDF file contains all the timing information of the design which
allows the user to perform timing simulation

Original Author Miad Nasr
Edited by Jason Anderson and Jason Luu

*/

#ifndef VERILOG_WRITER_H
#define VERILOG_WRITER_H

/*The verilog_writer function is the main function that will generate and write to the verilog and SDF files
  Al the functions declared bellow are called directly or indirectly by verilog_writer.c
  net_delay is a float 2D array containing the inter clb delay information.*/
void verilog_writer(void);

#endif
