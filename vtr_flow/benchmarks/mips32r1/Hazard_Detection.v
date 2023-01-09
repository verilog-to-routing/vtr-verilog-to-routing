`timescale 1ns / 1ps
/*
 * File         : Hazard_Detection.v
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
 *   Hazard Detection and Forward Control. This is the glue that allows a
 *   pipelined processor to operate efficiently and correctly in the presence
 *   of data, structural, and control hazards. For each pipeline stage, it
 *   detects whether that stage requires data that is still in the pipeline,
 *   and whether that data may be forwarded or if the pipeline must be stalled.
 *
 *   This module is heavily commented. Read below for more information.
 */
module Hazard_Detection(
    input  [7:0] DP_Hazards,
    input  [4:0] ID_Rs,
    input  [4:0] ID_Rt,
    input  [4:0] EX_Rs,
    input  [4:0] EX_Rt,
    input  [4:0] EX_RtRd,
    input  [4:0] MEM_RtRd,
    input  [4:0] WB_RtRd,
    input  EX_Link,
    input  EX_RegWrite,
    input  MEM_RegWrite,
    input  WB_RegWrite,
    input  MEM_MemRead,
    input  MEM_MemWrite,        // Needed for Store Conditional which writes to a register
    input  InstMem_Read,
    input  InstMem_Ack,
    input  Mfc0,                // Using fwd mux; not part of haz/fwd.
    input  IF_Exception_Stall,
    input  ID_Exception_Stall,
    input  EX_Exception_Stall,
    input  EX_ALU_Stall,
    input  M_Stall_Controller,  // Determined by data memory controller
    output IF_Stall,
    output ID_Stall,
    output EX_Stall,
    output M_Stall,
    output WB_Stall,
    output [1:0] ID_RsFwdSel,
    output [1:0] ID_RtFwdSel,
    output [1:0] EX_RsFwdSel,
    output [1:0] EX_RtFwdSel,
    output M_WriteDataFwdSel
    );

    /* Hazard and Forward Detection
     *
     * Most instructions read from one or more registers. Normally this occurs in
     * the ID stage. However, frequently the register file in the ID stage is stale
     * when one or more forward stages in the pipeline (EX, MEM, or WB) contains
     * an instruction which will eventually update it but has not yet done so.
     *
     * A hazard condition is created when a forward pipeline stage is set to write
     * the same register that a current pipeline stage (e.g. in ID) needs to read.
     * The solution is to stall the current stage (and effectively all stages behind
     * it) or bypass (forward) the data from forward stages. Fortunately forwarding
     * works for most combinations of instructions.
     *
     * Hazard and Forward conditions are handled based on two simple rules:
     * "Wants" and "Needs." If an instruction "wants" data in a certain pipeline
     * stage, and that data is available further along in the pipeline, it will
     * be forwarded. If it "needs" data and the data is not yet available for forwarding,
     * the pipeline stage stalls. If it does not want or need data in a certain
     * stage, forwarding is disabled and a stall will not occur. This is important
     * for instructions which insert custom data, such as jal or movz.
     *
     * Currently, "Want" and "Need" conditions are defined for both Rs data and Rt
     * data (the two read registers in MIPS), and these conditions exist in the
     * ID and EX pipeline stages. This is a total of eight condition bits.
     *
     * A unique exception exists with Store instructions, which don't need the
     * "Rt" data until the MEM stage. Because data doesn't change in WB, and WB
     * is the only stage following MEM, forwarding is *always* possible from
     * WB to Mem. This unit handles this situation, and a condition bit is not
     * needed.
     *
     * When data is needed from the MEM stage by a previous stage (ID or EX), the
     * decision to forward or stall is based on whether MEM is accessing memory
     * (stall) or not (forward). Normally store instructions don't write to registers
     * and thus are never needed for a data dependence, so the signal 'MEM_MemRead'
     * is sufficient to determine. Because of the Store Conditional instruction,
     * however, 'MEM_MemWrite' must also be considered because it writes to a register.
     *
     */

    wire WantRsByID, NeedRsByID, WantRtByID, NeedRtByID, WantRsByEX, NeedRsByEX, WantRtByEX, NeedRtByEX;
    assign WantRsByID = DP_Hazards[7];
    assign NeedRsByID = DP_Hazards[6];
    assign WantRtByID = DP_Hazards[5];
    assign NeedRtByID = DP_Hazards[4];
    assign WantRsByEX = DP_Hazards[3];
    assign NeedRsByEX = DP_Hazards[2];
    assign WantRtByEX = DP_Hazards[1];
    assign NeedRtByEX = DP_Hazards[0];

    // Trick allowed by RegDst = 0 which gives Rt. MEM_Rt is only used on
    // Data Memory write operations (stores), and RegWrite is always 0 in this case.
    wire [4:0] MEM_Rt = MEM_RtRd;

    // Forwarding should not happen when the src/dst register is $zero
    wire EX_RtRd_NZ  = (EX_RtRd  != 5'b00000);
    wire MEM_RtRd_NZ = (MEM_RtRd != 5'b00000);
    wire WB_RtRd_NZ  = (WB_RtRd  != 5'b00000);

    // ID Dependencies
    wire Rs_IDEX_Match  = (ID_Rs == EX_RtRd)  & EX_RtRd_NZ  & (WantRsByID | NeedRsByID) & EX_RegWrite;
    wire Rt_IDEX_Match  = (ID_Rt == EX_RtRd)  & EX_RtRd_NZ  & (WantRtByID | NeedRtByID) & EX_RegWrite;
    wire Rs_IDMEM_Match = (ID_Rs == MEM_RtRd) & MEM_RtRd_NZ & (WantRsByID | NeedRsByID) & MEM_RegWrite;
    wire Rt_IDMEM_Match = (ID_Rt == MEM_RtRd) & MEM_RtRd_NZ & (WantRtByID | NeedRtByID) & MEM_RegWrite;
    wire Rs_IDWB_Match  = (ID_Rs == WB_RtRd)  & WB_RtRd_NZ  & (WantRsByID | NeedRsByID) & WB_RegWrite;
    wire Rt_IDWB_Match  = (ID_Rt == WB_RtRd)  & WB_RtRd_NZ  & (WantRtByID | NeedRtByID) & WB_RegWrite;
    // EX Dependencies
    wire Rs_EXMEM_Match = (EX_Rs == MEM_RtRd) & MEM_RtRd_NZ & (WantRsByEX | NeedRsByEX) & MEM_RegWrite;
    wire Rt_EXMEM_Match = (EX_Rt == MEM_RtRd) & MEM_RtRd_NZ & (WantRtByEX | NeedRtByEX) & MEM_RegWrite;
    wire Rs_EXWB_Match  = (EX_Rs == WB_RtRd)  & WB_RtRd_NZ  & (WantRsByEX | NeedRsByEX) & WB_RegWrite;
    wire Rt_EXWB_Match  = (EX_Rt == WB_RtRd)  & WB_RtRd_NZ  & (WantRtByEX | NeedRtByEX) & WB_RegWrite;
    // MEM Dependencies
    wire Rt_MEMWB_Match = (MEM_Rt == WB_RtRd) & WB_RtRd_NZ  & WB_RegWrite;


    // ID needs data from EX  : Stall
    wire ID_Stall_1 = (Rs_IDEX_Match  &  NeedRsByID);
    wire ID_Stall_2 = (Rt_IDEX_Match  &  NeedRtByID);
    // ID needs data from MEM : Stall if mem access
    wire ID_Stall_3 = (Rs_IDMEM_Match &  (MEM_MemRead | MEM_MemWrite) & NeedRsByID);
    wire ID_Stall_4 = (Rt_IDMEM_Match &  (MEM_MemRead | MEM_MemWrite) & NeedRtByID);
    // ID wants data from MEM : Forward if not mem access
    wire ID_Fwd_1   = (Rs_IDMEM_Match & ~(MEM_MemRead | MEM_MemWrite));
    wire ID_Fwd_2   = (Rt_IDMEM_Match & ~(MEM_MemRead | MEM_MemWrite));
    // ID wants/needs data from WB  : Forward
    wire ID_Fwd_3   = (Rs_IDWB_Match);
    wire ID_Fwd_4   = (Rt_IDWB_Match);
    // EX needs data from MEM : Stall if mem access
    wire EX_Stall_1 = (Rs_EXMEM_Match &  (MEM_MemRead | MEM_MemWrite) & NeedRsByEX);
    wire EX_Stall_2 = (Rt_EXMEM_Match &  (MEM_MemRead | MEM_MemWrite) & NeedRtByEX);
    // EX wants data from MEM : Forward if not mem access
    wire EX_Fwd_1   = (Rs_EXMEM_Match & ~(MEM_MemRead | MEM_MemWrite));
    wire EX_Fwd_2   = (Rt_EXMEM_Match & ~(MEM_MemRead | MEM_MemWrite));
    // EX wants/needs data from WB  : Forward
    wire EX_Fwd_3   = (Rs_EXWB_Match);
    wire EX_Fwd_4   = (Rt_EXWB_Match);
    // MEM needs data from WB : Forward
    wire MEM_Fwd_1  = (Rt_MEMWB_Match);


    // Stalls and Control Flow Final Assignments
    assign WB_Stall = M_Stall;
    assign  M_Stall = IF_Stall | M_Stall_Controller;
    assign EX_Stall = (EX_Stall_1 | EX_Stall_2 | EX_Exception_Stall) | EX_ALU_Stall | M_Stall;
    assign ID_Stall = (ID_Stall_1 | ID_Stall_2 | ID_Stall_3 | ID_Stall_4 | ID_Exception_Stall) | EX_Stall;
    assign IF_Stall = InstMem_Read | InstMem_Ack | IF_Exception_Stall;

    // Forwarding Control Final Assignments
    assign ID_RsFwdSel = (ID_Fwd_1) ? 2'b01 : ((ID_Fwd_3) ? 2'b10 : 2'b00);
    assign ID_RtFwdSel = (Mfc0) ? 2'b11 : ((ID_Fwd_2) ? 2'b01 : ((ID_Fwd_4) ? 2'b10 : 2'b00));
    assign EX_RsFwdSel = (EX_Link) ? 2'b11 : ((EX_Fwd_1) ? 2'b01 : ((EX_Fwd_3) ? 2'b10 : 2'b00));
    assign EX_RtFwdSel = (EX_Link)  ? 2'b11 : ((EX_Fwd_2) ? 2'b01 : ((EX_Fwd_4) ? 2'b10 : 2'b00));
    assign M_WriteDataFwdSel = MEM_Fwd_1;

endmodule

