`timescale 1ns / 1ps
/*
 * File         : IFID_Stage.v
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
 *   The Pipeline Register to bridge the Instruction Fetch
 *   and Instruction Decode stages.
 */
module IFID_Stage(
    input  clock,
    input  reset,
    input  IF_Flush,
    input  IF_Stall,
    input  ID_Stall,
    // Control Signals
    input  [31:0] IF_Instruction,
    // Data Signals
    input  [31:0] IF_PCAdd4,
    input  [31:0] IF_PC,
    input  IF_IsBDS,
    // ------------------
    output reg [31:0] ID_Instruction,
    output reg [31:0] ID_PCAdd4,
    output reg [31:0] ID_RestartPC,
    output reg ID_IsBDS,
    output reg ID_IsFlushed
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
    ***/


    /***
     The signal 'ID_IsFlushed' is needed because of interrupts. Normally, a flushed instruction
     is a NOP which will never cause an exception and thus its restart PC will never be needed
     or used. However, interrupts are detected in ID and may occur when any instruction, flushed
     or not, is in the ID stage. It is an error to save the restart PC of a flushed instruction
     since it was never supposed to execute (such as the "delay slot" after ERET or the branch
     delay slot after a canceled Branch Likely instruction). A simple way to prevent this is to
     pass a signal to ID indicating that its instruction was flushed. Interrupt detection is then
     masked when this signal is high, and the interrupt will trigger on the next instruction load to ID.
    ***/

    always @(posedge clock) begin
        ID_Instruction <= (reset) ? 32'b0 : ((ID_Stall) ? ID_Instruction : ((IF_Stall | IF_Flush) ? 32'b0 : IF_Instruction));
        ID_PCAdd4      <= (reset) ? 32'b0 : ((ID_Stall) ? ID_PCAdd4                                       : IF_PCAdd4);
        ID_IsBDS       <= (reset) ? 1'b0  : ((ID_Stall) ? ID_IsBDS                                        : IF_IsBDS);
        ID_RestartPC   <= (reset) ? 32'b0 : ((ID_Stall | IF_IsBDS) ? ID_RestartPC                         : IF_PC);
        ID_IsFlushed   <= (reset) ? 1'b0  : ((ID_Stall) ? ID_IsFlushed                                    : IF_Flush);
    end

endmodule

