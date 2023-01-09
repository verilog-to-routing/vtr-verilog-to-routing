`timescale 1ns / 1ps
/*
 * File         : Register.v
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
 *   A variable-width register (d flip-flop) with configurable initial
 *   value. Default is 32-bit width and 0s for initial value.
 */
module Register #(parameter WIDTH = 32, INIT = 0)(
    input  clock,
    input  reset,
    input  enable,
    input  [(WIDTH-1):0] D,
    output reg [(WIDTH-1):0] Q
    );

    initial
        Q = INIT;

    always @(posedge clock) begin
        Q <= (reset) ? INIT : ((enable) ? D : Q);
    end

endmodule

