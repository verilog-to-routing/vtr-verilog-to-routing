`timescale 1ns / 1ps
/*
 * File         : MEMWB_Stage.v
 * Project      : University of Utah, XUM Project MIPS32 core
 * Creator(s)   : Grant Ayers (ayers@cs.utah.edu)
 *
 * Modification History:
 *   Rev   Date         Initials  Description of Change
 *   1.0   9-Jun-2011   GEA       Initial design.
 *   2.0   26-Jul-2012  GEA       Many updates have been made.
 *
 * Standards/Formatting:
 *   Verilog 2001, 4 soft tab, wide column.
 *
 * Description:
 *   The Pipeline Register to bridge the Memory and Writeback stages.
 */
module MEMWB_Stage(
    input  clock,
    input  reset,
    input  M_Flush,
    input  M_Stall,
    input  WB_Stall,
    // Control Signals
    input  M_RegWrite,
    input  M_MemtoReg,
    // Data Signals
    input  [31:0] M_ReadData,
    input  [31:0] M_ALU_Result,
    input  [4:0]  M_RtRd,
    // ----------------
    output reg WB_RegWrite,
    output reg WB_MemtoReg,
    output reg [31:0] WB_ReadData,
    output reg [31:0] WB_ALU_Result,
    output reg [4:0]  WB_RtRd
    );


    /***
     The purpose of a pipeline register is to capture data from one pipeline stage
     and provide it to the next pipeline stage. This creates at least one clock cycle
     of delay, but reduces the combinatorial path length of signals which allows for
     higher clock speeds.

     All pipeline registers update unless the forward stage is stalled. When this occurs
     or when the current stage is being flushed, the forward stage will receive data that
     is effectively a NOP and causes nothing to happen throughout the remaining pipeline
     traversal. In other words:

     A stall masks all control signals to forward stages. A flush permanently clears
     control signals to forward stages (but not certain data for exception purposes).

     Since WB is the final stage in the pipeline, it would normally never stall.
     However, because the MEM stage may be using data forwarded from WB, WB must stall
     when MEM is stalled. If it didn't, the forward data would not be preserved. If
     the processor didn't forward any data, a stall would not be needed.

     In practice, the only time WB stalls is when forwarding for a Lw->Sw sequence, since
     MEM doesn't need the data until its stage, but it does not latch the forwarded data.
     This means WB_Stall is probably identical to M_Stall. There is no speed difference by
     allowing WB to stall.
    ***/

    always @(posedge clock) begin
        WB_RegWrite   <= (reset) ? 1'b0  : ((WB_Stall) ? WB_RegWrite   : ((M_Stall | M_Flush) ? 1'b0 : M_RegWrite));
        WB_MemtoReg   <= (reset) ? 1'b0  : ((WB_Stall) ? WB_MemtoReg                                 : M_MemtoReg);
        WB_ReadData   <= (reset) ? 32'b0 : ((WB_Stall) ? WB_ReadData                                 : M_ReadData);
        WB_ALU_Result <= (reset) ? 32'b0 : ((WB_Stall) ? WB_ALU_Result                               : M_ALU_Result);
        WB_RtRd       <= (reset) ? 5'b0  : ((WB_Stall) ? WB_RtRd                                     : M_RtRd);
    end

endmodule

