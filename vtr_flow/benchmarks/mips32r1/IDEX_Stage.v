`timescale 1ns / 1ps
/*
 * File         : IDEX_Stage.v
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
 *   The Pipeline Register to bridge the Instruction Decode
 *   and Execute stages.
 */
module IDEX_Stage(
    input  clock,
    input  reset,
    input  ID_Flush,
    input  ID_Stall,
    input  EX_Stall,
    // Control Signals
    input  ID_Link,
    input  ID_RegDst,
    input  ID_ALUSrcImm,
    input  [4:0] ID_ALUOp,
    input  ID_Movn,
    input  ID_Movz,
    input  ID_LLSC,
    input  ID_MemRead,
    input  ID_MemWrite,
    input  ID_MemByte,
    input  ID_MemHalf,
    input  ID_MemSignExtend,
    input  ID_Left,
    input  ID_Right,
    input  ID_RegWrite,
    input  ID_MemtoReg,
    input  ID_ReverseEndian,
    // Hazard & Forwarding
    input  [4:0] ID_Rs,
    input  [4:0] ID_Rt,
    input  ID_WantRsByEX,
    input  ID_NeedRsByEX,
    input  ID_WantRtByEX,
    input  ID_NeedRtByEX,
    // Exception Control/Info
    input  ID_KernelMode,
    input  [31:0] ID_RestartPC,
    input  ID_IsBDS,
    input  ID_Trap,
    input  ID_TrapCond,
    input  ID_EX_CanErr,
    input  ID_M_CanErr,
    // Data Signals
    input  [31:0] ID_ReadData1,
    input  [31:0] ID_ReadData2,
    input  [16:0] ID_SignExtImm, // ID_Rd, ID_Shamt included here
    // ----------------
    output reg EX_Link,
    output [1:0] EX_LinkRegDst,
    output reg EX_ALUSrcImm,
    output reg [4:0] EX_ALUOp,
    output reg EX_Movn,
    output reg EX_Movz,
    output reg EX_LLSC,
    output reg EX_MemRead,
    output reg EX_MemWrite,
    output reg EX_MemByte,
    output reg EX_MemHalf,
    output reg EX_MemSignExtend,
    output reg EX_Left,
    output reg EX_Right,
    output reg EX_RegWrite,
    output reg EX_MemtoReg,
    output reg EX_ReverseEndian,
    output reg [4:0]  EX_Rs,
    output reg [4:0]  EX_Rt,
    output reg EX_WantRsByEX,
    output reg EX_NeedRsByEX,
    output reg EX_WantRtByEX,
    output reg EX_NeedRtByEX,
    output reg EX_KernelMode,
    output reg [31:0] EX_RestartPC,
    output reg EX_IsBDS,
    output reg EX_Trap,
    output reg EX_TrapCond,
    output reg EX_EX_CanErr,
    output reg EX_M_CanErr,
    output reg [31:0] EX_ReadData1,
    output reg [31:0] EX_ReadData2,
    output [31:0] EX_SignExtImm,
    output [4:0]      EX_Rd,
    output [4:0]      EX_Shamt
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

    reg [16:0] EX_SignExtImm_pre;
    reg EX_RegDst;
    assign EX_LinkRegDst = (EX_Link) ? 2'b10 : ((EX_RegDst) ? 2'b01 : 2'b00);
    assign EX_Rd = EX_SignExtImm[15:11];
    assign EX_Shamt = EX_SignExtImm[10:6];
    assign EX_SignExtImm = (EX_SignExtImm_pre[16]) ? {15'h7fff, EX_SignExtImm_pre[16:0]} : {15'h0000, EX_SignExtImm_pre[16:0]};

    always @(posedge clock) begin
        EX_Link           <= (reset) ? 1'b0  : ((EX_Stall) ? EX_Link                                          : ID_Link);
        EX_RegDst         <= (reset) ? 1'b0  : ((EX_Stall) ? EX_RegDst                                        : ID_RegDst);
        EX_ALUSrcImm      <= (reset) ? 1'b0  : ((EX_Stall) ? EX_ALUSrcImm                                     : ID_ALUSrcImm);
        EX_ALUOp          <= (reset) ? 5'b0  : ((EX_Stall) ? EX_ALUOp         : ((ID_Stall | ID_Flush) ? 5'b0 : ID_ALUOp));
        EX_Movn           <= (reset) ? 1'b0  : ((EX_Stall) ? EX_Movn                                          : ID_Movn);
        EX_Movz           <= (reset) ? 1'b0  : ((EX_Stall) ? EX_Movz                                          : ID_Movz);
        EX_LLSC           <= (reset) ? 1'b0  : ((EX_Stall) ? EX_LLSC                                          : ID_LLSC);
        EX_MemRead        <= (reset) ? 1'b0  : ((EX_Stall) ? EX_MemRead       : ((ID_Stall | ID_Flush) ? 1'b0 : ID_MemRead));
        EX_MemWrite       <= (reset) ? 1'b0  : ((EX_Stall) ? EX_MemWrite      : ((ID_Stall | ID_Flush) ? 1'b0 : ID_MemWrite));
        EX_MemByte        <= (reset) ? 1'b0  : ((EX_Stall) ? EX_MemByte                                       : ID_MemByte);
        EX_MemHalf        <= (reset) ? 1'b0  : ((EX_Stall) ? EX_MemHalf                                       : ID_MemHalf);
        EX_MemSignExtend  <= (reset) ? 1'b0  : ((EX_Stall) ? EX_MemSignExtend                                 : ID_MemSignExtend);
        EX_Left           <= (reset) ? 1'b0  : ((EX_Stall) ? EX_Left                                          : ID_Left);
        EX_Right          <= (reset) ? 1'b0  : ((EX_Stall) ? EX_Right                                         : ID_Right);
        EX_RegWrite       <= (reset) ? 1'b0  : ((EX_Stall) ? EX_RegWrite      : ((ID_Stall | ID_Flush) ? 1'b0 : ID_RegWrite));
        EX_MemtoReg       <= (reset) ? 1'b0  : ((EX_Stall) ? EX_MemtoReg                                      : ID_MemtoReg);
        EX_ReverseEndian  <= (reset) ? 1'b0  : ((EX_Stall) ? EX_ReverseEndian                                 : ID_ReverseEndian);
        EX_RestartPC      <= (reset) ? 32'b0 : ((EX_Stall) ? EX_RestartPC                                     : ID_RestartPC);
        EX_IsBDS          <= (reset) ? 1'b0  : ((EX_Stall) ? EX_IsBDS                                         : ID_IsBDS);
        EX_Trap           <= (reset) ? 1'b0  : ((EX_Stall) ? EX_Trap          : ((ID_Stall | ID_Flush) ? 1'b0 : ID_Trap));
        EX_TrapCond       <= (reset) ? 1'b0  : ((EX_Stall) ? EX_TrapCond                                      : ID_TrapCond);
        EX_EX_CanErr      <= (reset) ? 1'b0  : ((EX_Stall) ? EX_EX_CanErr     : ((ID_Stall | ID_Flush) ? 1'b0 : ID_EX_CanErr));
        EX_M_CanErr       <= (reset) ? 1'b0  : ((EX_Stall) ? EX_M_CanErr      : ((ID_Stall | ID_Flush) ? 1'b0 : ID_M_CanErr));
        EX_ReadData1      <= (reset) ? 32'b0 : ((EX_Stall) ? EX_ReadData1                                     : ID_ReadData1);
        EX_ReadData2      <= (reset) ? 32'b0 : ((EX_Stall) ? EX_ReadData2                                     : ID_ReadData2);
        EX_SignExtImm_pre <= (reset) ? 17'b0 : ((EX_Stall) ? EX_SignExtImm_pre                                : ID_SignExtImm);
        EX_Rs             <= (reset) ? 5'b0  : ((EX_Stall) ? EX_Rs                                            : ID_Rs);
        EX_Rt             <= (reset) ? 5'b0  : ((EX_Stall) ? EX_Rt                                            : ID_Rt);
        EX_WantRsByEX     <= (reset) ? 1'b0  : ((EX_Stall) ? EX_WantRsByEX    : ((ID_Stall | ID_Flush) ? 1'b0 : ID_WantRsByEX));
        EX_NeedRsByEX     <= (reset) ? 1'b0  : ((EX_Stall) ? EX_NeedRsByEX    : ((ID_Stall | ID_Flush) ? 1'b0 : ID_NeedRsByEX));
        EX_WantRtByEX     <= (reset) ? 1'b0  : ((EX_Stall) ? EX_WantRtByEX    : ((ID_Stall | ID_Flush) ? 1'b0 : ID_WantRtByEX));
        EX_NeedRtByEX     <= (reset) ? 1'b0  : ((EX_Stall) ? EX_NeedRtByEX    : ((ID_Stall | ID_Flush) ? 1'b0 : ID_NeedRtByEX));
        EX_KernelMode     <= (reset) ? 1'b0  : ((EX_Stall) ? EX_KernelMode                                    : ID_KernelMode);
    end

endmodule
