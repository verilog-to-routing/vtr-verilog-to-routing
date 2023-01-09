`timescale 1ns / 1ps
/*
 * File         : TrapDetect.v
 * Project      : University of Utah, XUM Project MIPS32 core
 * Creator(s)   : Grant Ayers (ayers@cs.utah.edu)
 *
 * Modification History:
 *   Rev   Date         Initials  Description of Change
 *   1.0   15-May-2012  GEA       Initial design.
 *
 * Standards/Formatting:
 *   Verilog 2001, 4 soft tab, wide column.
 *
 * Description:
 *   Detects a Trap Exception in the pipeline.
 */
module TrapDetect(
    input  Trap,
    input  TrapCond,
    input  [31:0] ALUResult,
    output EXC_Tr
    );

    wire ALUZero = (ALUResult == 32'h00000000);
    assign EXC_Tr = Trap & (TrapCond ^ ALUZero);

endmodule

