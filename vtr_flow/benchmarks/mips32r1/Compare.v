`timescale 1ns / 1ps
/*
 * File         : Compare.v
 * Project      : University of Utah, XUM Project MIPS32 core
 * Creator(s)   : Grant Ayers (ayers@cs.utah.edu)
 *
 * Modification History:
 *   Rev   Date         Initials  Description of Change
 *   1.0   15-Jun-2011  GEA       Initial design.
 *
 * Standards/Formatting:
 *   Verilog 2001, 4 soft tab, wide column.
 *
 * Description:
 *   Compares two 32-bit values and outputs the following information about them:
 *      EQ  : A and B are equal
 *      GZ  : A is greater than zero
 *      LZ  : A is less than zero
 *      GEZ : A is greater than or equal to zero
 *      LEZ : A is less than or equal to zero
 */
module Compare(
    input  [31:0] A,
    input  [31:0] B,
    output EQ,
    output GZ,
    output LZ,
    output GEZ,
    output LEZ
    );

    wire   ZeroA = (A == 32'b0);

    assign EQ  = ( A == B);
    assign GZ  = (~A[31] & ~ZeroA);
    assign LZ  =   A[31];
    assign GEZ =  ~A[31];
    assign LEZ = ( A[31] |  ZeroA);

endmodule

