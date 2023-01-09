`timescale 1ns / 1ps
/*
 * File         : Mux4.v
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
 *   A 4-input Mux of variable width, defaulting to 32-bit width.
 */
module Mux4 #(parameter WIDTH = 32)(
    input  [1:0] sel,
    input  [(WIDTH-1):0] in0, in1, in2, in3,
    output reg [(WIDTH-1):0] out
    );

    always @(*) begin
        case (sel)
            2'b00 : out <= in0;
            2'b01 : out <= in1;
            2'b10 : out <= in2;
            2'b11 : out <= in3;
        endcase
    end

endmodule

