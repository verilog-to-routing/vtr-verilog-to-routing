`timescale 1ns / 1ps
/*
 * File         : Add.v
 * Project      : University of Utah, XUM Project MIPS32 core
 * Creator(s)   : Grant Ayers (ayers@cs.utah.edu)
 *
 * Modification History:
 *   Rev   Date         Initials  Description of Change
 *   1.0   7-Jun-2011   GEA       Initial design.
 *
 * Standards/Formatting:
 *   Verilog 2001, 4 soft tab, wide column.
 *
 * Description:
 *   A simple 32-bit 2-input adder.
 */
module Add(
    input  [31:0] A,
    input  [31:0] B,
    output [31:0] C
    );

    assign C = (A + B);

endmodule

