`timescale 1ns / 1ps
/*
 * File         : EXMEM_Stage.v
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
 *   The Pipeline Register to bridge the Execute and Memory stages.
 */
module EXMEM_Stage(
    input  clock,
    input  reset,
    input  EX_Flush,
    input  EX_Stall,
    input  M_Stall,
    // Control Signals
    input  EX_Movn,
    input  EX_Movz,
    input  EX_BZero,
    input  EX_RegWrite,  // Future Control to WB
    input  EX_MemtoReg,  // Future Control to WB
    input  EX_ReverseEndian,
    input  EX_LLSC,
    input  EX_MemRead,
    input  EX_MemWrite,
    input  EX_MemByte,
    input  EX_MemHalf,
    input  EX_MemSignExtend,
    input  EX_Left,
    input  EX_Right,
    // Exception Control/Info
    input  EX_KernelMode,
    input  [31:0] EX_RestartPC,
    input  EX_IsBDS,
    input  EX_Trap,
    input  EX_TrapCond,
    input  EX_M_CanErr,
    // Data Signals
    input  [31:0] EX_ALU_Result,
    input  [31:0] EX_ReadData2,
    input  [4:0]  EX_RtRd,
    // ------------------
    output reg M_RegWrite,
    output reg M_MemtoReg,
    output reg M_ReverseEndian,
    output reg M_LLSC,
    output reg M_MemRead,
    output reg M_MemWrite,
    output reg M_MemByte,
    output reg M_MemHalf,
    output reg M_MemSignExtend,
    output reg M_Left,
    output reg M_Right,
    output reg M_KernelMode,
    output reg [31:0] M_RestartPC,
    output reg M_IsBDS,
    output reg M_Trap,
    output reg M_TrapCond,
    output reg M_M_CanErr,
    output reg [31:0] M_ALU_Result,
    output reg [31:0] M_ReadData2,
    output reg [4:0]  M_RtRd
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

    // Mask of RegWrite if a Move Conditional failed.
    wire MovcRegWrite = (EX_Movn & ~EX_BZero) | (EX_Movz & EX_BZero);

    always @(posedge clock) begin
        M_RegWrite      <= (reset) ? 1'b0  : ((M_Stall) ? M_RegWrite      : ((EX_Stall | EX_Flush) ? 1'b0 : ((EX_Movn | EX_Movz) ? MovcRegWrite : EX_RegWrite)));
        M_MemtoReg      <= (reset) ? 1'b0  : ((M_Stall) ? M_MemtoReg                                      : EX_MemtoReg);
        M_ReverseEndian <= (reset) ? 1'b0  : ((M_Stall) ? M_ReverseEndian                                 : EX_ReverseEndian);
        M_LLSC          <= (reset) ? 1'b0  : ((M_Stall) ? M_LLSC                                          : EX_LLSC);
        M_MemRead       <= (reset) ? 1'b0  : ((M_Stall) ? M_MemRead       : ((EX_Stall | EX_Flush) ? 1'b0 : EX_MemRead));
        M_MemWrite      <= (reset) ? 1'b0  : ((M_Stall) ? M_MemWrite      : ((EX_Stall | EX_Flush) ? 1'b0 : EX_MemWrite));
        M_MemByte       <= (reset) ? 1'b0  : ((M_Stall) ? M_MemByte                                       : EX_MemByte);
        M_MemHalf       <= (reset) ? 1'b0  : ((M_Stall) ? M_MemHalf                                       : EX_MemHalf);
        M_MemSignExtend <= (reset) ? 1'b0  : ((M_Stall) ? M_MemSignExtend                                 : EX_MemSignExtend);
        M_Left          <= (reset) ? 1'b0  : ((M_Stall) ? M_Left                                          : EX_Left);
        M_Right         <= (reset) ? 1'b0  : ((M_Stall) ? M_Right                                         : EX_Right);
        M_KernelMode    <= (reset) ? 1'b0  : ((M_Stall) ? M_KernelMode                                    : EX_KernelMode);
        M_RestartPC     <= (reset) ? 32'b0 : ((M_Stall) ? M_RestartPC                                     : EX_RestartPC);
        M_IsBDS         <= (reset) ? 1'b0  : ((M_Stall) ? M_IsBDS                                         : EX_IsBDS);
        M_Trap          <= (reset) ? 1'b0  : ((M_Stall) ? M_Trap          : ((EX_Stall | EX_Flush) ? 1'b0 : EX_Trap));
        M_TrapCond      <= (reset) ? 1'b0  : ((M_Stall) ? M_TrapCond                                      : EX_TrapCond);
        M_M_CanErr      <= (reset) ? 1'b0  : ((M_Stall) ? M_M_CanErr      : ((EX_Stall | EX_Flush) ? 1'b0 : EX_M_CanErr));
        M_ALU_Result    <= (reset) ? 32'b0 : ((M_Stall) ? M_ALU_Result                                    : EX_ALU_Result);
        M_ReadData2     <= (reset) ? 32'b0 : ((M_Stall) ? M_ReadData2                                     : EX_ReadData2);
        M_RtRd          <= (reset) ? 5'b0  : ((M_Stall) ? M_RtRd                                          : EX_RtRd);
    end

endmodule

