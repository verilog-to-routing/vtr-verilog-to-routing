`timescale 1ns / 1ps
/*
 * File         : Processor.v
 * Project      : University of Utah, XUM Project MIPS32 core
 * Creator(s)   : Grant Ayers (ayers@cs.utah.edu)
 *
 * Modification History:
 *   Rev   Date         Initials  Description of Change
 *   1.0   23-Jul-2011  GEA       Initial design.
 *   2.0   26-May-2012  GEA       Release version with CP0.
 *   2.01   1-Nov-2012  GEA       Fixed issue with Jal.
 *   ***   ***********  ***       [Further changes tracked in source control]
 *
 * Standards/Formatting:
 *   Verilog 2001, 4 soft tab, wide column.
 *
 * Description:
 *   The top-level MIPS32 Processor. This file is mostly the instantiation
 *   and wiring of the building blocks of the processor according to the
 *   hardware design diagram. It contains very little logic itself.
 */
module Processor(
    input  clock,
    input  reset,
    input  [4:0] Interrupts,            // 5 general-purpose hardware interrupts
    input  NMI,                         // Non-maskable interrupt
    // Data Memory Interface
    input  [31:0] DataMem_In,
    input  DataMem_Ack,
    output DataMem_Read,
    output [3:0]  DataMem_Write,        // 4-bit Write, one for each byte in word.
    output [29:0] DataMem_Address,      // Addresses are words, not bytes.
    output [31:0] DataMem_Out,
    // Instruction Memory Interface
    input  [31:0] InstMem_In,
    output [29:0] InstMem_Address,      // Addresses are words, not bytes.
    input  InstMem_Ack,
    output InstMem_Read,
    output [7:0] IP                     // Pending interrupts (diagnostic)
    );

    `include "MIPS_Parameters.v"


    /*** MIPS Instruction and Components (ID Stage) ***/
    wire [31:0] Instruction;
    wire [5:0]  OpCode = Instruction[31:26];
    wire [4:0]  Rs = Instruction[25:21];
    wire [4:0]  Rt = Instruction[20:16];
    wire [4:0]  Rd = Instruction[15:11];
    wire [5:0]  Funct = Instruction[5:0];
    wire [15:0] Immediate = Instruction[15:0];
    wire [25:0] JumpAddress = Instruction[25:0];
    wire [2:0]  Cp0_Sel = Instruction[2:0];

    /*** IF (Instruction Fetch) Signals ***/
    wire IF_Stall, IF_Flush;
    wire IF_EXC_AdIF;
    wire IF_Exception_Stall;
    wire IF_Exception_Flush;
    wire IF_IsBDS;
    wire [31:0] IF_PCAdd4, IF_PC_PreExc, IF_PCIn, IF_PCOut, IF_Instruction;

    /*** ID (Instruction Decode) Signals ***/
    wire ID_Stall;
    wire [1:0] ID_PCSrc;
    wire [1:0] ID_RsFwdSel, ID_RtFwdSel;
    wire ID_Link, ID_Movn, ID_Movz;
    wire ID_SignExtend;
    wire ID_LLSC;
    wire ID_RegDst, ID_ALUSrcImm, ID_MemWrite, ID_MemRead, ID_MemByte, ID_MemHalf, ID_MemSignExtend, ID_RegWrite, ID_MemtoReg;
    wire [4:0] ID_ALUOp;
    wire ID_Mfc0, ID_Mtc0, ID_Eret;
    wire ID_NextIsDelay;
    wire ID_CanErr, ID_ID_CanErr, ID_EX_CanErr, ID_M_CanErr;
    wire ID_KernelMode;
    wire ID_ReverseEndian;
    wire ID_Trap, ID_TrapCond;
    wire ID_EXC_Sys, ID_EXC_Bp, ID_EXC_RI;
    wire ID_Exception_Stall;
    wire ID_Exception_Flush;
    wire ID_PCSrc_Exc;
    wire [31:0] ID_ExceptionPC;
    wire ID_CP1, ID_CP2, ID_CP3;
    wire [31:0] ID_PCAdd4;
    wire [31:0] ID_ReadData1_RF, ID_ReadData1_End;
    wire [31:0] ID_ReadData2_RF, ID_ReadData2_End;
    wire [31:0] CP0_RegOut;
    wire ID_CmpEQ, ID_CmpGZ, ID_CmpLZ, ID_CmpGEZ, ID_CmpLEZ;
    wire [29:0] ID_SignExtImm = (ID_SignExtend & Immediate[15]) ? {14'h3FFF, Immediate} : {14'h0000, Immediate};
    wire [31:0] ID_ImmLeftShift2 = {ID_SignExtImm[29:0], 2'b00};
    wire [31:0] ID_JumpAddress = {ID_PCAdd4[31:28], JumpAddress[25:0], 2'b00};
    wire [31:0] ID_BranchAddress;
    wire [31:0] ID_RestartPC;
    wire ID_IsBDS;
    wire ID_Left, ID_Right;
    wire ID_IsFlushed;

    /*** EX (Execute) Signals ***/
    wire EX_ALU_Stall, EX_Stall;
    wire [1:0] EX_RsFwdSel, EX_RtFwdSel;
    wire EX_Link;
    wire [1:0] EX_LinkRegDst;
    wire EX_ALUSrcImm;
    wire [4:0] EX_ALUOp;
    wire EX_Movn, EX_Movz;
    wire EX_LLSC;
    wire EX_MemRead, EX_MemWrite, EX_MemByte, EX_MemHalf, EX_MemSignExtend, EX_RegWrite, EX_MemtoReg;
    wire [4:0] EX_Rs, EX_Rt;
    wire EX_WantRsByEX, EX_NeedRsByEX, EX_WantRtByEX, EX_NeedRtByEX;
    wire EX_Trap, EX_TrapCond;
    wire EX_CanErr, EX_EX_CanErr, EX_M_CanErr;
    wire EX_KernelMode;
    wire EX_ReverseEndian;
    wire EX_Exception_Stall;
    wire EX_Exception_Flush;
    wire [31:0] EX_ReadData1_PR, EX_ReadData1_Fwd, EX_ReadData2_PR, EX_ReadData2_Fwd, EX_ReadData2_Imm;
    wire [31:0] EX_SignExtImm;
    wire [4:0] EX_Rd, EX_RtRd, EX_Shamt;
    wire [31:0] EX_ALUResult;
    wire EX_BZero;
    wire EX_EXC_Ov;
    wire [31:0] EX_RestartPC;
    wire EX_IsBDS;
    wire EX_Left, EX_Right;

    /*** MEM (Memory) Signals ***/
    wire M_Stall, M_Stall_Controller;
    wire M_LLSC;
    wire M_MemRead, M_MemWrite, M_MemByte, M_MemHalf, M_MemSignExtend;
    wire M_RegWrite, M_MemtoReg;
    wire M_WriteDataFwdSel;
    wire M_EXC_AdEL, M_EXC_AdES;
    wire M_M_CanErr;
    wire M_KernelMode;
    wire M_ReverseEndian;
    wire M_Trap, M_TrapCond;
    wire M_EXC_Tr;
    wire M_Exception_Flush;
    wire [31:0] M_ALUResult, M_ReadData2_PR;
    wire [4:0] M_RtRd;
    wire [31:0] M_MemReadData;
    wire [31:0] M_RestartPC;
    wire M_IsBDS;
    wire [31:0] M_WriteData_Pre;
    wire M_Left, M_Right;
    wire M_Exception_Stall;

    /*** WB (Writeback) Signals ***/
    wire WB_Stall, WB_RegWrite, WB_MemtoReg;
    wire [31:0] WB_ReadData, WB_ALUResult;
    wire [4:0]  WB_RtRd;
    wire [31:0] WB_WriteData;

    /*** Other Signals ***/
    wire [7:0] ID_DP_Hazards, HAZ_DP_Hazards;

    /*** Assignments ***/
    assign IF_Instruction = (IF_Stall) ? 32'h00000000 : InstMem_In;
    assign IF_IsBDS = ID_NextIsDelay;
    assign HAZ_DP_Hazards = {ID_DP_Hazards[7:4], EX_WantRsByEX, EX_NeedRsByEX, EX_WantRtByEX, EX_NeedRtByEX};
    assign IF_EXC_AdIF = IF_PCOut[1] | IF_PCOut[0];
    assign ID_CanErr = ID_ID_CanErr | ID_EX_CanErr | ID_M_CanErr;
    assign EX_CanErr = EX_EX_CanErr | EX_M_CanErr;

    // External Memory Interface
    reg IRead, IReadMask;
    assign InstMem_Address = IF_PCOut[31:2];
    assign DataMem_Address = M_ALUResult[31:2];
    always @(posedge clock) begin
        IRead <= (reset) ? 1'b1 : ~InstMem_Ack;
        IReadMask <= (reset) ? 1'b0 : ((IRead & InstMem_Ack) ? 1'b1 : ((~IF_Stall) ? 1'b0 : IReadMask));
    end
    assign InstMem_Read = IRead & ~IReadMask;


    /*** Datapath Controller ***/
    Control Controller (
        .ID_Stall       (ID_Stall),
        .OpCode         (OpCode),
        .Funct          (Funct),
        .Rs             (Rs),
        .Rt             (Rt),
        .Cmp_EQ         (ID_CmpEQ),
        .Cmp_GZ         (ID_CmpGZ),
        .Cmp_GEZ        (ID_CmpGEZ),
        .Cmp_LZ         (ID_CmpLZ),
        .Cmp_LEZ        (ID_CmpLEZ),
        .IF_Flush       (IF_Flush),
        .DP_Hazards     (ID_DP_Hazards),
        .PCSrc          (ID_PCSrc),
        .SignExtend     (ID_SignExtend),
        .Link           (ID_Link),
        .Movn           (ID_Movn),
        .Movz           (ID_Movz),
        .Mfc0           (ID_Mfc0),
        .Mtc0           (ID_Mtc0),
        .CP1            (ID_CP1),
        .CP2            (ID_CP2),
        .CP3            (ID_CP3),
        .Eret           (ID_Eret),
        .Trap           (ID_Trap),
        .TrapCond       (ID_TrapCond),
        .EXC_Sys        (ID_EXC_Sys),
        .EXC_Bp         (ID_EXC_Bp),
        .EXC_RI         (ID_EXC_RI),
        .ID_CanErr      (ID_ID_CanErr),
        .EX_CanErr      (ID_EX_CanErr),
        .M_CanErr       (ID_M_CanErr),
        .NextIsDelay    (ID_NextIsDelay),
        .RegDst         (ID_RegDst),
        .ALUSrcImm      (ID_ALUSrcImm),
        .ALUOp          (ID_ALUOp),
        .LLSC           (ID_LLSC),
        .MemWrite       (ID_MemWrite),
        .MemRead        (ID_MemRead),
        .MemByte        (ID_MemByte),
        .MemHalf        (ID_MemHalf),
        .MemSignExtend  (ID_MemSignExtend),
        .Left           (ID_Left),
        .Right          (ID_Right),
        .RegWrite       (ID_RegWrite),
        .MemtoReg       (ID_MemtoReg)
    );

    /*** Hazard and Forward Control Unit ***/
    Hazard_Detection HazardControl (
        .DP_Hazards          (HAZ_DP_Hazards),
        .ID_Rs               (Rs),
        .ID_Rt               (Rt),
        .EX_Rs               (EX_Rs),
        .EX_Rt               (EX_Rt),
        .EX_RtRd             (EX_RtRd),
        .MEM_RtRd            (M_RtRd),
        .WB_RtRd             (WB_RtRd),
        .EX_Link             (EX_Link),
        .EX_RegWrite         (EX_RegWrite),
        .MEM_RegWrite        (M_RegWrite),
        .WB_RegWrite         (WB_RegWrite),
        .MEM_MemRead         (M_MemRead),
        .MEM_MemWrite        (M_MemWrite),
        .InstMem_Read        (InstMem_Read),
        .InstMem_Ack         (InstMem_Ack),
        .Mfc0                (ID_Mfc0),
        .IF_Exception_Stall  (IF_Exception_Stall),
        .ID_Exception_Stall  (ID_Exception_Stall),
        .EX_Exception_Stall  (EX_Exception_Stall),
        .EX_ALU_Stall        (EX_ALU_Stall),
        .M_Stall_Controller  (M_Stall_Controller),
        .IF_Stall            (IF_Stall),
        .ID_Stall            (ID_Stall),
        .EX_Stall            (EX_Stall),
        .M_Stall             (M_Stall),
        .WB_Stall            (WB_Stall),
        .ID_RsFwdSel         (ID_RsFwdSel),
        .ID_RtFwdSel         (ID_RtFwdSel),
        .EX_RsFwdSel         (EX_RsFwdSel),
        .EX_RtFwdSel         (EX_RtFwdSel),
        .M_WriteDataFwdSel   (M_WriteDataFwdSel)
    );

    /*** Coprocessor 0: Exceptions and Interrupts ***/
    CPZero CP0 (
        .clock               (clock),
        .Mfc0                (ID_Mfc0),
        .Mtc0                (ID_Mtc0),
        .IF_Stall            (IF_Stall),
        .ID_Stall            (ID_Stall),
        .COP1                (ID_CP1),
        .COP2                (ID_CP2),
        .COP3                (ID_CP3),
        .ERET                (ID_Eret),
        .Rd                  (Rd),
        .Sel                 (Cp0_Sel),
        .Reg_In              (ID_ReadData2_End),
        .Reg_Out             (CP0_RegOut),
        .KernelMode          (ID_KernelMode),
        .ReverseEndian       (ID_ReverseEndian),
        .Int                 (Interrupts),
        .reset               (reset),
        .EXC_NMI             (NMI),
        .EXC_AdIF            (IF_EXC_AdIF),
        .EXC_AdEL            (M_EXC_AdEL),
        .EXC_AdES            (M_EXC_AdES),
        .EXC_Ov              (EX_EXC_Ov),
        .EXC_Tr              (M_EXC_Tr),
        .EXC_Sys             (ID_EXC_Sys),
        .EXC_Bp              (ID_EXC_Bp),
        .EXC_RI              (ID_EXC_RI),
        .ID_RestartPC        (ID_RestartPC),
        .EX_RestartPC        (EX_RestartPC),
        .M_RestartPC         (M_RestartPC),
        .ID_IsFlushed        (ID_IsFlushed),
        .IF_IsBD             (IF_IsBDS),
        .ID_IsBD             (ID_IsBDS),
        .EX_IsBD             (EX_IsBDS),
        .M_IsBD              (M_IsBDS),
        .BadAddr_M           (M_ALUResult),
        .BadAddr_IF          (IF_PCOut),
        .ID_CanErr           (ID_CanErr),
        .EX_CanErr           (EX_CanErr),
        .M_CanErr            (M_M_CanErr),
        .IF_Exception_Stall  (IF_Exception_Stall),
        .ID_Exception_Stall  (ID_Exception_Stall),
        .EX_Exception_Stall  (EX_Exception_Stall),
        .M_Exception_Stall   (M_Exception_Stall),
        .IF_Exception_Flush  (IF_Exception_Flush),
        .ID_Exception_Flush  (ID_Exception_Flush),
        .EX_Exception_Flush  (EX_Exception_Flush),
        .M_Exception_Flush   (M_Exception_Flush),
        .Exc_PC_Sel          (ID_PCSrc_Exc),
        .Exc_PC_Out          (ID_ExceptionPC),
        .IP                  (IP)
    );

    /*** PC Source Non-Exception Mux ***/
    Mux4 #(.WIDTH(32)) PCSrcStd_Mux (
        .sel  (ID_PCSrc),
        .in0  (IF_PCAdd4),
        .in1  (ID_JumpAddress),
        .in2  (ID_BranchAddress),
        .in3  (ID_ReadData1_End),
        .out  (IF_PC_PreExc)
    );

    /*** PC Source Exception Mux ***/
    Mux2 #(.WIDTH(32)) PCSrcExc_Mux (
        .sel  (ID_PCSrc_Exc),
        .in0  (IF_PC_PreExc),
        .in1  (ID_ExceptionPC),
        .out  (IF_PCIn)
    );

    /*** Program Counter (MIPS spec is 0xBFC00000 starting address) ***/
    Register #(.WIDTH(32), .INIT(`EXC_Vector_Base_Reset)) PC (
        .clock   (clock),
        .reset   (reset),
        //.enable  (~IF_Stall),   // XXX verify. HERE. Was 1 but on stall latches PC+4, ad nauseum.
        .enable (~(IF_Stall | ID_Stall)),
        .D       (IF_PCIn),
        .Q       (IF_PCOut)
    );

    /*** PC +4 Adder ***/
    Add PC_Add4 (
        .A  (IF_PCOut),
        .B  (32'h00000004),
        .C  (IF_PCAdd4)
    );

    /*** Instruction Fetch -> Instruction Decode Stage Register ***/
    IFID_Stage IFID (
        .clock           (clock),
        .reset           (reset),
        .IF_Flush        (IF_Exception_Flush | IF_Flush),
        .IF_Stall        (IF_Stall),
        .ID_Stall        (ID_Stall),
        .IF_Instruction  (IF_Instruction),
        .IF_PCAdd4       (IF_PCAdd4),
        .IF_PC           (IF_PCOut),
        .IF_IsBDS        (IF_IsBDS),
        .ID_Instruction  (Instruction),
        .ID_PCAdd4       (ID_PCAdd4),
        .ID_RestartPC    (ID_RestartPC),
        .ID_IsBDS        (ID_IsBDS),
        .ID_IsFlushed    (ID_IsFlushed)
    );

    /*** Register File ***/
    RegisterFile RegisterFile (
        .clock      (clock),
        .reset      (reset),
        .ReadReg1   (Rs),
        .ReadReg2   (Rt),
        .WriteReg   (WB_RtRd),
        .WriteData  (WB_WriteData),
        .RegWrite   (WB_RegWrite),
        .ReadData1  (ID_ReadData1_RF),
        .ReadData2  (ID_ReadData2_RF)
    );

    /*** ID Rs Forwarding/Link Mux ***/
    Mux4 #(.WIDTH(32)) IDRsFwd_Mux (
        .sel  (ID_RsFwdSel),
        .in0  (ID_ReadData1_RF),
        .in1  (M_ALUResult),
        .in2  (WB_WriteData),
        .in3  (32'hxxxxxxxx),
        .out  (ID_ReadData1_End)
    );

    /*** ID Rt Forwarding/CP0 Mfc0 Mux ***/
    Mux4 #(.WIDTH(32)) IDRtFwd_Mux (
        .sel  (ID_RtFwdSel),
        .in0  (ID_ReadData2_RF),
        .in1  (M_ALUResult),
        .in2  (WB_WriteData),
        .in3  (CP0_RegOut),
        .out  (ID_ReadData2_End)
    );

    /*** Condition Compare Unit ***/
    Compare Compare (
        .A    (ID_ReadData1_End),
        .B    (ID_ReadData2_End),
        .EQ   (ID_CmpEQ),
        .GZ   (ID_CmpGZ),
        .LZ   (ID_CmpLZ),
        .GEZ  (ID_CmpGEZ),
        .LEZ  (ID_CmpLEZ)
    );

    /*** Branch Address Adder ***/
    Add BranchAddress_Add (
        .A  (ID_PCAdd4),
        .B  (ID_ImmLeftShift2),
        .C  (ID_BranchAddress)
    );

    /*** Instruction Decode -> Execute Pipeline Stage ***/
    IDEX_Stage IDEX (
        .clock             (clock),
        .reset             (reset),
        .ID_Flush          (ID_Exception_Flush),
        .ID_Stall          (ID_Stall),
        .EX_Stall          (EX_Stall),
        .ID_Link           (ID_Link),
        .ID_RegDst         (ID_RegDst),
        .ID_ALUSrcImm      (ID_ALUSrcImm),
        .ID_ALUOp          (ID_ALUOp),
        .ID_Movn           (ID_Movn),
        .ID_Movz           (ID_Movz),
        .ID_LLSC           (ID_LLSC),
        .ID_MemRead        (ID_MemRead),
        .ID_MemWrite       (ID_MemWrite),
        .ID_MemByte        (ID_MemByte),
        .ID_MemHalf        (ID_MemHalf),
        .ID_MemSignExtend  (ID_MemSignExtend),
        .ID_Left           (ID_Left),
        .ID_Right          (ID_Right),
        .ID_RegWrite       (ID_RegWrite),
        .ID_MemtoReg       (ID_MemtoReg),
        .ID_ReverseEndian  (ID_ReverseEndian),
        .ID_Rs             (Rs),
        .ID_Rt             (Rt),
        .ID_WantRsByEX     (ID_DP_Hazards[3]),
        .ID_NeedRsByEX     (ID_DP_Hazards[2]),
        .ID_WantRtByEX     (ID_DP_Hazards[1]),
        .ID_NeedRtByEX     (ID_DP_Hazards[0]),
        .ID_KernelMode     (ID_KernelMode),
        .ID_RestartPC      (ID_RestartPC),
        .ID_IsBDS          (ID_IsBDS),
        .ID_Trap           (ID_Trap),
        .ID_TrapCond       (ID_TrapCond),
        .ID_EX_CanErr      (ID_EX_CanErr),
        .ID_M_CanErr       (ID_M_CanErr),
        .ID_ReadData1      (ID_ReadData1_End),
        .ID_ReadData2      (ID_ReadData2_End),
        .ID_SignExtImm     (ID_SignExtImm[16:0]),
        .EX_Link           (EX_Link),
        .EX_LinkRegDst     (EX_LinkRegDst),
        .EX_ALUSrcImm      (EX_ALUSrcImm),
        .EX_ALUOp          (EX_ALUOp),
        .EX_Movn           (EX_Movn),
        .EX_Movz           (EX_Movz),
        .EX_LLSC           (EX_LLSC),
        .EX_MemRead        (EX_MemRead),
        .EX_MemWrite       (EX_MemWrite),
        .EX_MemByte        (EX_MemByte),
        .EX_MemHalf        (EX_MemHalf),
        .EX_MemSignExtend  (EX_MemSignExtend),
        .EX_Left           (EX_Left),
        .EX_Right          (EX_Right),
        .EX_RegWrite       (EX_RegWrite),
        .EX_MemtoReg       (EX_MemtoReg),
        .EX_ReverseEndian  (EX_ReverseEndian),
        .EX_Rs             (EX_Rs),
        .EX_Rt             (EX_Rt),
        .EX_WantRsByEX     (EX_WantRsByEX),
        .EX_NeedRsByEX     (EX_NeedRsByEX),
        .EX_WantRtByEX     (EX_WantRtByEX),
        .EX_NeedRtByEX     (EX_NeedRtByEX),
        .EX_KernelMode     (EX_KernelMode),
        .EX_RestartPC      (EX_RestartPC),
        .EX_IsBDS          (EX_IsBDS),
        .EX_Trap           (EX_Trap),
        .EX_TrapCond       (EX_TrapCond),
        .EX_EX_CanErr      (EX_EX_CanErr),
        .EX_M_CanErr       (EX_M_CanErr),
        .EX_ReadData1      (EX_ReadData1_PR),
        .EX_ReadData2      (EX_ReadData2_PR),
        .EX_SignExtImm     (EX_SignExtImm),
        .EX_Rd             (EX_Rd),
        .EX_Shamt          (EX_Shamt)
    );

    /*** EX Rs Forwarding Mux ***/
    Mux4 #(.WIDTH(32)) EXRsFwd_Mux (
        .sel  (EX_RsFwdSel),
        .in0  (EX_ReadData1_PR),
        .in1  (M_ALUResult),
        .in2  (WB_WriteData),
        .in3  (EX_RestartPC),
        .out  (EX_ReadData1_Fwd)
    );

    /*** EX Rt Forwarding / Link Mux ***/
    Mux4 #(.WIDTH(32)) EXRtFwdLnk_Mux (
        .sel  (EX_RtFwdSel),
        .in0  (EX_ReadData2_PR),
        .in1  (M_ALUResult),
        .in2  (WB_WriteData),
        .in3  (32'h00000008),
        .out  (EX_ReadData2_Fwd)
    );

    /*** EX ALU Immediate Mux ***/
    Mux2 #(.WIDTH(32)) EXALUImm_Mux (
        .sel  (EX_ALUSrcImm),
        .in0  (EX_ReadData2_Fwd),
        .in1  (EX_SignExtImm),
        .out  (EX_ReadData2_Imm)
    );

    /*** EX RtRd / Link Mux ***/
    Mux4 #(.WIDTH(5)) EXRtRdLnk_Mux (
        .sel  (EX_LinkRegDst),
        .in0  (EX_Rt),
        .in1  (EX_Rd),
        .in2  (5'b11111),
        .in3  (5'bxxxxx),
        .out  (EX_RtRd)
    );

    /*** Arithmetic Logic Unit ***/
    ALU ALU (
        .clock      (clock),
        .reset      (reset),
        .EX_Stall   (EX_Stall),
        .EX_Flush   (EX_Exception_Flush),
        .A          (EX_ReadData1_Fwd),
        .B          (EX_ReadData2_Imm),
        .Operation  (EX_ALUOp),
        .Shamt      (EX_Shamt),
        .Result     (EX_ALUResult),
        .BZero      (EX_BZero),
        .EXC_Ov     (EX_EXC_Ov),
        .ALU_Stall  (EX_ALU_Stall)
    );

    /*** Execute -> Memory Pipeline Stage ***/
    EXMEM_Stage EXMEM (
        .clock             (clock),
        .reset             (reset),
        .EX_Flush          (EX_Exception_Flush),
        .EX_Stall          (EX_Stall),
        .M_Stall           (M_Stall),
        .EX_Movn           (EX_Movn),
        .EX_Movz           (EX_Movz),
        .EX_BZero          (EX_BZero),
        .EX_RegWrite       (EX_RegWrite),
        .EX_MemtoReg       (EX_MemtoReg),
        .EX_ReverseEndian  (EX_ReverseEndian),
        .EX_LLSC           (EX_LLSC),
        .EX_MemRead        (EX_MemRead),
        .EX_MemWrite       (EX_MemWrite),
        .EX_MemByte        (EX_MemByte),
        .EX_MemHalf        (EX_MemHalf),
        .EX_MemSignExtend  (EX_MemSignExtend),
        .EX_Left           (EX_Left),
        .EX_Right          (EX_Right),
        .EX_KernelMode     (EX_KernelMode),
        .EX_RestartPC      (EX_RestartPC),
        .EX_IsBDS          (EX_IsBDS),
        .EX_Trap           (EX_Trap),
        .EX_TrapCond       (EX_TrapCond),
        .EX_M_CanErr       (EX_M_CanErr),
        .EX_ALU_Result     (EX_ALUResult),
        .EX_ReadData2      (EX_ReadData2_Fwd),
        .EX_RtRd           (EX_RtRd),
        .M_RegWrite        (M_RegWrite),
        .M_MemtoReg        (M_MemtoReg),
        .M_ReverseEndian   (M_ReverseEndian),
        .M_LLSC            (M_LLSC),
        .M_MemRead         (M_MemRead),
        .M_MemWrite        (M_MemWrite),
        .M_MemByte         (M_MemByte),
        .M_MemHalf         (M_MemHalf),
        .M_MemSignExtend   (M_MemSignExtend),
        .M_Left            (M_Left),
        .M_Right           (M_Right),
        .M_KernelMode      (M_KernelMode),
        .M_RestartPC       (M_RestartPC),
        .M_IsBDS           (M_IsBDS),
        .M_Trap            (M_Trap),
        .M_TrapCond        (M_TrapCond),
        .M_M_CanErr        (M_M_CanErr),
        .M_ALU_Result      (M_ALUResult),
        .M_ReadData2       (M_ReadData2_PR),
        .M_RtRd            (M_RtRd)
    );

    /*** Trap Detection Unit ***/
    TrapDetect TrapDetect (
        .Trap       (M_Trap),
        .TrapCond   (M_TrapCond),
        .ALUResult  (M_ALUResult),
        .EXC_Tr     (M_EXC_Tr)
    );

    /*** MEM Write Data Mux ***/
    Mux2 #(.WIDTH(32)) MWriteData_Mux (
        .sel  (M_WriteDataFwdSel),
        .in0  (M_ReadData2_PR),
        .in1  (WB_WriteData),
        .out  (M_WriteData_Pre)
    );

    /*** Data Memory Controller ***/
    MemControl DataMem_Controller (
        .clock         (clock),
        .reset         (reset),
        .DataIn        (M_WriteData_Pre),
        .Address       (M_ALUResult),
        .MReadData     (DataMem_In),
        .MemRead       (M_MemRead),
        .MemWrite      (M_MemWrite),
        .DataMem_Ack   (DataMem_Ack),
        .Byte          (M_MemByte),
        .Half          (M_MemHalf),
        .SignExtend    (M_MemSignExtend),
        .KernelMode    (M_KernelMode),
        .ReverseEndian (M_ReverseEndian),
        .LLSC          (M_LLSC),
        .ERET          (ID_Eret),
        .Left          (M_Left),
        .Right         (M_Right),
        .M_Exception_Stall (M_Exception_Stall),
        .IF_Stall      (IF_Stall),
        .DataOut       (M_MemReadData),
        .MWriteData    (DataMem_Out),
        .WriteEnable   (DataMem_Write),
        .ReadEnable    (DataMem_Read),
        .M_Stall       (M_Stall_Controller),
        .EXC_AdEL      (M_EXC_AdEL),
        .EXC_AdES      (M_EXC_AdES)
    );

    /*** Memory -> Writeback Pipeline Stage ***/
    MEMWB_Stage MEMWB (
        .clock          (clock),
        .reset          (reset),
        .M_Flush        (M_Exception_Flush),
        .M_Stall        (M_Stall),
        .WB_Stall       (WB_Stall),
        .M_RegWrite     (M_RegWrite),
        .M_MemtoReg     (M_MemtoReg),
        .M_ReadData     (M_MemReadData),
        .M_ALU_Result   (M_ALUResult),
        .M_RtRd         (M_RtRd),
        .WB_RegWrite    (WB_RegWrite),
        .WB_MemtoReg    (WB_MemtoReg),
        .WB_ReadData    (WB_ReadData),
        .WB_ALU_Result  (WB_ALUResult),
        .WB_RtRd        (WB_RtRd)
    );

    /*** WB MemtoReg Mux ***/
    Mux2 #(.WIDTH(32)) WBMemtoReg_Mux (
        .sel  (WB_MemtoReg),
        .in0  (WB_ALUResult),
        .in1  (WB_ReadData),
        .out  (WB_WriteData)
    );

endmodule

