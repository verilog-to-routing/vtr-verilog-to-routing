`timescale 1ns / 1ps
/*
 * File         : Control.v
 * Project      : University of Utah, XUM Project MIPS32 core
 * Creator(s)   : Grant Ayers (ayers@cs.utah.edu)
 *
 * Modification History:
 *   Rev   Date         Initials  Description of Change
 *   1.0    7-Jun-2011  GEA       Initial design.
 *   2.0   26-May-2012  GEA       Release version with CP0.
 *
 * Standards/Formatting:
 *   Verilog 2001, 4 soft tab, wide column.
 *
 * Description:
 *   The Datapath Controller. This module sets the datapath control
 *   bits for an incoming instruction. These control bits follow the
 *   instruction through each pipeline stage as needed, and constitute
 *   the effective operation of the processor through each pipeline stage.
 */
module Control(
    input  ID_Stall,
    input  [5:0] OpCode,
    input  [5:0] Funct,
    input  [4:0] Rs,        // used to differentiate mfc0 and mtc0
    input  [4:0] Rt,        // used to differentiate bgez,bgezal,bltz,bltzal,teqi,tgei,tgeiu,tlti,tltiu,tnei
    input  Cmp_EQ,
    input  Cmp_GZ,
    input  Cmp_GEZ,
    input  Cmp_LZ,
    input  Cmp_LEZ,
    //------------
    output IF_Flush,
    output reg [7:0] DP_Hazards,
    output [1:0] PCSrc,
    output SignExtend,
    output Link,
    output Movn,
    output Movz,
    output Mfc0,
    output Mtc0,
    output CP1,
    output CP2,
    output CP3,
    output Eret,
    output Trap,
    output TrapCond,
    output EXC_Sys,
    output EXC_Bp,
    output EXC_RI,
    output ID_CanErr,
    output EX_CanErr,
    output M_CanErr,
    output NextIsDelay,
    output RegDst,
    output ALUSrcImm,
    output reg [4:0] ALUOp,
    output LLSC,
    output MemWrite,
    output MemRead,
    output MemByte,
    output MemHalf,
    output MemSignExtend,
    output Left,
    output Right,
    output RegWrite,
    output MemtoReg
    );

    `include "MIPS_Parameters.v"

    wire Movc;
    wire Branch, Branch_EQ, Branch_GTZ, Branch_LEZ, Branch_NEQ, Branch_GEZ, Branch_LTZ;
    wire Unaligned_Mem;

    reg [15:0] Datapath;
    assign PCSrc[0]      = Datapath[14];
    assign Link          = Datapath[13];
    assign ALUSrcImm     = Datapath[12];
    assign Movc          = Datapath[11];
    assign Trap          = Datapath[10];
    assign TrapCond      = Datapath[9];
    assign RegDst        = Datapath[8];
    assign LLSC          = Datapath[7];
    assign MemRead       = Datapath[6];
    assign MemWrite      = Datapath[5];
    assign MemHalf       = Datapath[4];
    assign MemByte       = Datapath[3];
    assign MemSignExtend = Datapath[2];
    assign RegWrite      = Datapath[1];
    assign MemtoReg      = Datapath[0];

    reg [2:0] DP_Exceptions;
    assign ID_CanErr = DP_Exceptions[2];
    assign EX_CanErr = DP_Exceptions[1];
    assign  M_CanErr = DP_Exceptions[0];

    // Set the main datapath control signals based on the Op Code
    always @(*) begin
        if (ID_Stall)
            Datapath <= `DP_None;
        else begin
            case (OpCode)
                // R-Type
                `Op_Type_R  :
                    begin
                        case (Funct)
                            `Funct_Add     : Datapath <= `DP_Add;
                            `Funct_Addu    : Datapath <= `DP_Addu;
                            `Funct_And     : Datapath <= `DP_And;
                            `Funct_Break   : Datapath <= `DP_Break;
                            `Funct_Div     : Datapath <= `DP_Div;
                            `Funct_Divu    : Datapath <= `DP_Divu;
                            `Funct_Jalr    : Datapath <= `DP_Jalr;
                            `Funct_Jr      : Datapath <= `DP_Jr;
                            `Funct_Mfhi    : Datapath <= `DP_Mfhi;
                            `Funct_Mflo    : Datapath <= `DP_Mflo;
                            `Funct_Movn    : Datapath <= `DP_Movn;
                            `Funct_Movz    : Datapath <= `DP_Movz;
                            `Funct_Mthi    : Datapath <= `DP_Mthi;
                            `Funct_Mtlo    : Datapath <= `DP_Mtlo;
                            `Funct_Mult    : Datapath <= `DP_Mult;
                            `Funct_Multu   : Datapath <= `DP_Multu;
                            `Funct_Nor     : Datapath <= `DP_Nor;
                            `Funct_Or      : Datapath <= `DP_Or;
                            `Funct_Sll     : Datapath <= `DP_Sll;
                            `Funct_Sllv    : Datapath <= `DP_Sllv;
                            `Funct_Slt     : Datapath <= `DP_Slt;
                            `Funct_Sltu    : Datapath <= `DP_Sltu;
                            `Funct_Sra     : Datapath <= `DP_Sra;
                            `Funct_Srav    : Datapath <= `DP_Srav;
                            `Funct_Srl     : Datapath <= `DP_Srl;
                            `Funct_Srlv    : Datapath <= `DP_Srlv;
                            `Funct_Sub     : Datapath <= `DP_Sub;
                            `Funct_Subu    : Datapath <= `DP_Subu;
                            `Funct_Syscall : Datapath <= `DP_Syscall;
                            `Funct_Teq     : Datapath <= `DP_Teq;
                            `Funct_Tge     : Datapath <= `DP_Tge;
                            `Funct_Tgeu    : Datapath <= `DP_Tgeu;
                            `Funct_Tlt     : Datapath <= `DP_Tlt;
                            `Funct_Tltu    : Datapath <= `DP_Tltu;
                            `Funct_Tne     : Datapath <= `DP_Tne;
                            `Funct_Xor     : Datapath <= `DP_Xor;
                            default        : Datapath <= `DP_None;
                        endcase
                    end
                // R2-Type
                `Op_Type_R2 :
                    begin
                        case (Funct)
                            `Funct_Clo   : Datapath <= `DP_Clo;
                            `Funct_Clz   : Datapath <= `DP_Clz;
                            `Funct_Madd  : Datapath <= `DP_Madd;
                            `Funct_Maddu : Datapath <= `DP_Maddu;
                            `Funct_Msub  : Datapath <= `DP_Msub;
                            `Funct_Msubu : Datapath <= `DP_Msubu;
                            `Funct_Mul   : Datapath <= `DP_Mul;
                            default      : Datapath <= `DP_None;
                        endcase
                    end
                // I-Type
                `Op_Addi    : Datapath <= `DP_Addi;
                `Op_Addiu   : Datapath <= `DP_Addiu;
                `Op_Andi    : Datapath <= `DP_Andi;
                `Op_Ori     : Datapath <= `DP_Ori;
                `Op_Pref    : Datapath <= `DP_Pref;
                `Op_Slti    : Datapath <= `DP_Slti;
                `Op_Sltiu   : Datapath <= `DP_Sltiu;
                `Op_Xori    : Datapath <= `DP_Xori;
                // Jumps (using immediates)
                `Op_J       : Datapath <= `DP_J;
                `Op_Jal     : Datapath <= `DP_Jal;
                // Branches and Traps
                `Op_Type_BI :
                    begin
                        case (Rt)
                            `OpRt_Bgez   : Datapath <= `DP_Bgez;
                            `OpRt_Bgezal : Datapath <= `DP_Bgezal;
                            `OpRt_Bltz   : Datapath <= `DP_Bltz;
                            `OpRt_Bltzal : Datapath <= `DP_Bltzal;
                            `OpRt_Teqi   : Datapath <= `DP_Teqi;
                            `OpRt_Tgei   : Datapath <= `DP_Tgei;
                            `OpRt_Tgeiu  : Datapath <= `DP_Tgeiu;
                            `OpRt_Tlti   : Datapath <= `DP_Tlti;
                            `OpRt_Tltiu  : Datapath <= `DP_Tltiu;
                            `OpRt_Tnei   : Datapath <= `DP_Tnei;
                            default      : Datapath <= `DP_None;
                        endcase
                    end
                `Op_Beq     : Datapath <= `DP_Beq;
                `Op_Bgtz    : Datapath <= `DP_Bgtz;
                `Op_Blez    : Datapath <= `DP_Blez;
                `Op_Bne     : Datapath <= `DP_Bne;
                // Coprocessor 0
                `Op_Type_CP0 :
                    begin
                        case (Rs)
                            `OpRs_MF   : Datapath <= `DP_Mfc0;
                            `OpRs_MT   : Datapath <= `DP_Mtc0;
                            `OpRs_ERET : Datapath <= (Funct == `Funct_ERET) ? `DP_Eret : `DP_None;
                            default    : Datapath <= `DP_None;
                        endcase
                    end
                // Memory
                `Op_Lb   : Datapath <= `DP_Lb;
                `Op_Lbu  : Datapath <= `DP_Lbu;
                `Op_Lh   : Datapath <= `DP_Lh;
                `Op_Lhu  : Datapath <= `DP_Lhu;
                `Op_Ll   : Datapath <= `DP_Ll;
                `Op_Lui  : Datapath <= `DP_Lui;
                `Op_Lw   : Datapath <= `DP_Lw;
                `Op_Lwl  : Datapath <= `DP_Lwl;
                `Op_Lwr  : Datapath <= `DP_Lwr;
                `Op_Sb   : Datapath <= `DP_Sb;
                `Op_Sc   : Datapath <= `DP_Sc;
                `Op_Sh   : Datapath <= `DP_Sh;
                `Op_Sw   : Datapath <= `DP_Sw;
                `Op_Swl  : Datapath <= `DP_Swl;
                `Op_Swr  : Datapath <= `DP_Swr;
                default  : Datapath <= `DP_None;
            endcase
        end
    end

    // Set the Hazard Control Signals and Exception Indicators based on the Op Code
    always @(*) begin
        case (OpCode)
            // R-Type
            `Op_Type_R  :
                begin
                    case (Funct)
                        `Funct_Add     : begin DP_Hazards <= `HAZ_Add;     DP_Exceptions <= `EXC_Add;     end
                        `Funct_Addu    : begin DP_Hazards <= `HAZ_Addu;    DP_Exceptions <= `EXC_Addu;    end
                        `Funct_And     : begin DP_Hazards <= `HAZ_And;     DP_Exceptions <= `EXC_And;     end
                        `Funct_Break   : begin DP_Hazards <= `HAZ_Break;   DP_Exceptions <= `EXC_Break;   end
                        `Funct_Div     : begin DP_Hazards <= `HAZ_Div;     DP_Exceptions <= `EXC_Div;     end
                        `Funct_Divu    : begin DP_Hazards <= `HAZ_Divu;    DP_Exceptions <= `EXC_Divu;    end
                        `Funct_Jalr    : begin DP_Hazards <= `HAZ_Jalr;    DP_Exceptions <= `EXC_Jalr;    end
                        `Funct_Jr      : begin DP_Hazards <= `HAZ_Jr;      DP_Exceptions <= `EXC_Jr;      end
                        `Funct_Mfhi    : begin DP_Hazards <= `HAZ_Mfhi;    DP_Exceptions <= `EXC_Mfhi;    end
                        `Funct_Mflo    : begin DP_Hazards <= `HAZ_Mflo;    DP_Exceptions <= `EXC_Mflo;    end
                        `Funct_Movn    : begin DP_Hazards <= `HAZ_Movn;    DP_Exceptions <= `EXC_Movn;    end
                        `Funct_Movz    : begin DP_Hazards <= `HAZ_Movz;    DP_Exceptions <= `EXC_Movz;    end
                        `Funct_Mthi    : begin DP_Hazards <= `HAZ_Mthi;    DP_Exceptions <= `EXC_Mthi;    end
                        `Funct_Mtlo    : begin DP_Hazards <= `HAZ_Mtlo;    DP_Exceptions <= `EXC_Mtlo;    end
                        `Funct_Mult    : begin DP_Hazards <= `HAZ_Mult;    DP_Exceptions <= `EXC_Mult;    end
                        `Funct_Multu   : begin DP_Hazards <= `HAZ_Multu;   DP_Exceptions <= `EXC_Multu;   end
                        `Funct_Nor     : begin DP_Hazards <= `HAZ_Nor;     DP_Exceptions <= `EXC_Nor;     end
                        `Funct_Or      : begin DP_Hazards <= `HAZ_Or;      DP_Exceptions <= `EXC_Or;      end
                        `Funct_Sll     : begin DP_Hazards <= `HAZ_Sll;     DP_Exceptions <= `EXC_Sll;     end
                        `Funct_Sllv    : begin DP_Hazards <= `HAZ_Sllv;    DP_Exceptions <= `EXC_Sllv;    end
                        `Funct_Slt     : begin DP_Hazards <= `HAZ_Slt;     DP_Exceptions <= `EXC_Slt;     end
                        `Funct_Sltu    : begin DP_Hazards <= `HAZ_Sltu;    DP_Exceptions <= `EXC_Sltu;    end
                        `Funct_Sra     : begin DP_Hazards <= `HAZ_Sra;     DP_Exceptions <= `EXC_Sra;     end
                        `Funct_Srav    : begin DP_Hazards <= `HAZ_Srav;    DP_Exceptions <= `EXC_Srav;    end
                        `Funct_Srl     : begin DP_Hazards <= `HAZ_Srl;     DP_Exceptions <= `EXC_Srl;     end
                        `Funct_Srlv    : begin DP_Hazards <= `HAZ_Srlv;    DP_Exceptions <= `EXC_Srlv;    end
                        `Funct_Sub     : begin DP_Hazards <= `HAZ_Sub;     DP_Exceptions <= `EXC_Sub;     end
                        `Funct_Subu    : begin DP_Hazards <= `HAZ_Subu;    DP_Exceptions <= `EXC_Subu;    end
                        `Funct_Syscall : begin DP_Hazards <= `HAZ_Syscall; DP_Exceptions <= `EXC_Syscall; end
                        `Funct_Teq     : begin DP_Hazards <= `HAZ_Teq;     DP_Exceptions <= `EXC_Teq;     end
                        `Funct_Tge     : begin DP_Hazards <= `HAZ_Tge;     DP_Exceptions <= `EXC_Tge;     end
                        `Funct_Tgeu    : begin DP_Hazards <= `HAZ_Tgeu;    DP_Exceptions <= `EXC_Tgeu;    end
                        `Funct_Tlt     : begin DP_Hazards <= `HAZ_Tlt;     DP_Exceptions <= `EXC_Tlt;     end
                        `Funct_Tltu    : begin DP_Hazards <= `HAZ_Tltu;    DP_Exceptions <= `EXC_Tltu;    end
                        `Funct_Tne     : begin DP_Hazards <= `HAZ_Tne;     DP_Exceptions <= `EXC_Tne;     end
                        `Funct_Xor     : begin DP_Hazards <= `HAZ_Xor;     DP_Exceptions <= `EXC_Xor;     end
                        default        : begin DP_Hazards <= 8'hxx;        DP_Exceptions <= 3'bxxx;       end
                    endcase
                end
            // R2-Type
            `Op_Type_R2 :
                begin
                    case (Funct)
                        `Funct_Clo   : begin DP_Hazards <= `HAZ_Clo;   DP_Exceptions <= `EXC_Clo;   end
                        `Funct_Clz   : begin DP_Hazards <= `HAZ_Clz;   DP_Exceptions <= `EXC_Clz;   end
                        `Funct_Madd  : begin DP_Hazards <= `HAZ_Madd;  DP_Exceptions <= `EXC_Madd;  end
                        `Funct_Maddu : begin DP_Hazards <= `HAZ_Maddu; DP_Exceptions <= `EXC_Maddu; end
                        `Funct_Msub  : begin DP_Hazards <= `HAZ_Msub;  DP_Exceptions <= `EXC_Msub;  end
                        `Funct_Msubu : begin DP_Hazards <= `HAZ_Msubu; DP_Exceptions <= `EXC_Msubu; end
                        `Funct_Mul   : begin DP_Hazards <= `HAZ_Mul;   DP_Exceptions <= `EXC_Mul;   end
                        default      : begin DP_Hazards <= 8'hxx;      DP_Exceptions <= 3'bxxx;     end
                    endcase
                end
            // I-Type
            `Op_Addi    : begin DP_Hazards <= `HAZ_Addi;  DP_Exceptions <= `EXC_Addi;  end
            `Op_Addiu   : begin DP_Hazards <= `HAZ_Addiu; DP_Exceptions <= `EXC_Addiu; end
            `Op_Andi    : begin DP_Hazards <= `HAZ_Andi;  DP_Exceptions <= `EXC_Andi;  end
            `Op_Ori     : begin DP_Hazards <= `HAZ_Ori;   DP_Exceptions <= `EXC_Ori;   end
            `Op_Pref    : begin DP_Hazards <= `HAZ_Pref;  DP_Exceptions <= `EXC_Pref;  end
            `Op_Slti    : begin DP_Hazards <= `HAZ_Slti;  DP_Exceptions <= `EXC_Slti;  end
            `Op_Sltiu   : begin DP_Hazards <= `HAZ_Sltiu; DP_Exceptions <= `EXC_Sltiu; end
            `Op_Xori    : begin DP_Hazards <= `HAZ_Xori;  DP_Exceptions <= `EXC_Xori;  end
            // Jumps
            `Op_J       : begin DP_Hazards <= `HAZ_J;     DP_Exceptions <= `EXC_J;     end
            `Op_Jal     : begin DP_Hazards <= `HAZ_Jal;   DP_Exceptions <= `EXC_Jal;   end
            // Branches and Traps
            `Op_Type_BI :
                begin
                    case (Rt)
                        `OpRt_Bgez   : begin DP_Hazards <= `HAZ_Bgez;   DP_Exceptions <= `EXC_Bgez;   end
                        `OpRt_Bgezal : begin DP_Hazards <= `HAZ_Bgezal; DP_Exceptions <= `EXC_Bgezal; end
                        `OpRt_Bltz   : begin DP_Hazards <= `HAZ_Bltz;   DP_Exceptions <= `EXC_Bltz;   end
                        `OpRt_Bltzal : begin DP_Hazards <= `HAZ_Bltzal; DP_Exceptions <= `EXC_Bltzal; end
                        `OpRt_Teqi   : begin DP_Hazards <= `HAZ_Teqi;   DP_Exceptions <= `EXC_Teqi;   end
                        `OpRt_Tgei   : begin DP_Hazards <= `HAZ_Tgei;   DP_Exceptions <= `EXC_Tgei;   end
                        `OpRt_Tgeiu  : begin DP_Hazards <= `HAZ_Tgeiu;  DP_Exceptions <= `EXC_Tgeiu;  end
                        `OpRt_Tlti   : begin DP_Hazards <= `HAZ_Tlti;   DP_Exceptions <= `EXC_Tlti;   end
                        `OpRt_Tltiu  : begin DP_Hazards <= `HAZ_Tltiu;  DP_Exceptions <= `EXC_Tltiu;  end
                        `OpRt_Tnei   : begin DP_Hazards <= `HAZ_Tnei;   DP_Exceptions <= `EXC_Tnei;   end
                        default      : begin DP_Hazards <= 8'hxx;       DP_Exceptions <= 3'bxxx;      end
                    endcase
                end
            `Op_Beq     : begin DP_Hazards <= `HAZ_Beq;  DP_Exceptions <= `EXC_Beq;  end
            `Op_Bgtz    : begin DP_Hazards <= `HAZ_Bgtz; DP_Exceptions <= `EXC_Bgtz; end
            `Op_Blez    : begin DP_Hazards <= `HAZ_Blez; DP_Exceptions <= `EXC_Blez; end
            `Op_Bne     : begin DP_Hazards <= `HAZ_Bne;  DP_Exceptions <= `EXC_Bne;  end
            // Coprocessor 0
            `Op_Type_CP0 :
                begin
                    case (Rs)
                        `OpRs_MF   : begin DP_Hazards <= `HAZ_Mfc0; DP_Exceptions <= `EXC_Mfc0; end
                        `OpRs_MT   : begin DP_Hazards <= `HAZ_Mtc0; DP_Exceptions <= `EXC_Mtc0; end
                        `OpRs_ERET : begin DP_Hazards <= (Funct == `Funct_ERET) ? `HAZ_Eret : 8'hxx; DP_Exceptions <= `EXC_Eret; end
                        default    : begin DP_Hazards <= 8'hxx;     DP_Exceptions <= 3'bxxx;    end
                    endcase
                end
            // Memory
            `Op_Lb   : begin DP_Hazards <= `HAZ_Lb;  DP_Exceptions <= `EXC_Lb;  end
            `Op_Lbu  : begin DP_Hazards <= `HAZ_Lbu; DP_Exceptions <= `EXC_Lbu; end
            `Op_Lh   : begin DP_Hazards <= `HAZ_Lh;  DP_Exceptions <= `EXC_Lh;  end
            `Op_Lhu  : begin DP_Hazards <= `HAZ_Lhu; DP_Exceptions <= `EXC_Lhu; end
            `Op_Ll   : begin DP_Hazards <= `HAZ_Ll;  DP_Exceptions <= `EXC_Ll;  end
            `Op_Lui  : begin DP_Hazards <= `HAZ_Lui; DP_Exceptions <= `EXC_Lui; end
            `Op_Lw   : begin DP_Hazards <= `HAZ_Lw;  DP_Exceptions <= `EXC_Lw;  end
            `Op_Lwl  : begin DP_Hazards <= `HAZ_Lwl; DP_Exceptions <= `EXC_Lwl; end
            `Op_Lwr  : begin DP_Hazards <= `HAZ_Lwr; DP_Exceptions <= `EXC_Lwr; end
            `Op_Sb   : begin DP_Hazards <= `HAZ_Sb;  DP_Exceptions <= `EXC_Sb;  end
            `Op_Sc   : begin DP_Hazards <= `HAZ_Sc;  DP_Exceptions <= `EXC_Sc;  end
            `Op_Sh   : begin DP_Hazards <= `HAZ_Sh;  DP_Exceptions <= `EXC_Sh;  end
            `Op_Sw   : begin DP_Hazards <= `HAZ_Sw;  DP_Exceptions <= `EXC_Sw;  end
            `Op_Swl  : begin DP_Hazards <= `HAZ_Swl; DP_Exceptions <= `EXC_Swl; end
            `Op_Swr  : begin DP_Hazards <= `HAZ_Swr; DP_Exceptions <= `EXC_Swr; end
            default  : begin DP_Hazards <= 8'hxx;    DP_Exceptions <= 3'bxxx;   end
        endcase
    end

    // ALU Assignment
    always @(*) begin
        if (ID_Stall)
            ALUOp <= `AluOp_Addu;  // Any Op that doesn't write HILO or cause exceptions
        else begin
            case (OpCode)
                `Op_Type_R  :
                    begin
                        case (Funct)
                            `Funct_Add     : ALUOp <= `AluOp_Add;
                            `Funct_Addu    : ALUOp <= `AluOp_Addu;
                            `Funct_And     : ALUOp <= `AluOp_And;
                            `Funct_Div     : ALUOp <= `AluOp_Div;
                            `Funct_Divu    : ALUOp <= `AluOp_Divu;
                            `Funct_Jalr    : ALUOp <= `AluOp_Addu;
                            `Funct_Mfhi    : ALUOp <= `AluOp_Mfhi;
                            `Funct_Mflo    : ALUOp <= `AluOp_Mflo;
                            `Funct_Movn    : ALUOp <= `AluOp_Addu;
                            `Funct_Movz    : ALUOp <= `AluOp_Addu;
                            `Funct_Mthi    : ALUOp <= `AluOp_Mthi;
                            `Funct_Mtlo    : ALUOp <= `AluOp_Mtlo;
                            `Funct_Mult    : ALUOp <= `AluOp_Mult;
                            `Funct_Multu   : ALUOp <= `AluOp_Multu;
                            `Funct_Nor     : ALUOp <= `AluOp_Nor;
                            `Funct_Or      : ALUOp <= `AluOp_Or;
                            `Funct_Sll     : ALUOp <= `AluOp_Sll;
                            `Funct_Sllv    : ALUOp <= `AluOp_Sllv;
                            `Funct_Slt     : ALUOp <= `AluOp_Slt;
                            `Funct_Sltu    : ALUOp <= `AluOp_Sltu;
                            `Funct_Sra     : ALUOp <= `AluOp_Sra;
                            `Funct_Srav    : ALUOp <= `AluOp_Srav;
                            `Funct_Srl     : ALUOp <= `AluOp_Srl;
                            `Funct_Srlv    : ALUOp <= `AluOp_Srlv;
                            `Funct_Sub     : ALUOp <= `AluOp_Sub;
                            `Funct_Subu    : ALUOp <= `AluOp_Subu;
                            `Funct_Syscall : ALUOp <= `AluOp_Addu;
                            `Funct_Teq     : ALUOp <= `AluOp_Subu;
                            `Funct_Tge     : ALUOp <= `AluOp_Slt;
                            `Funct_Tgeu    : ALUOp <= `AluOp_Sltu;
                            `Funct_Tlt     : ALUOp <= `AluOp_Slt;
                            `Funct_Tltu    : ALUOp <= `AluOp_Sltu;
                            `Funct_Tne     : ALUOp <= `AluOp_Subu;
                            `Funct_Xor     : ALUOp <= `AluOp_Xor;
                            default        : ALUOp <= `AluOp_Addu;
                        endcase
                    end
                `Op_Type_R2 :
                    begin
                        case (Funct)
                            `Funct_Clo   : ALUOp <= `AluOp_Clo;
                            `Funct_Clz   : ALUOp <= `AluOp_Clz;
                            `Funct_Madd  : ALUOp <= `AluOp_Madd;
                            `Funct_Maddu : ALUOp <= `AluOp_Maddu;
                            `Funct_Msub  : ALUOp <= `AluOp_Msub;
                            `Funct_Msubu : ALUOp <= `AluOp_Msubu;
                            `Funct_Mul   : ALUOp <= `AluOp_Mul;
                            default      : ALUOp <= `AluOp_Addu;
                        endcase
                    end
                `Op_Type_BI  :
                    begin
                        case (Rt)
                            `OpRt_Teqi   : ALUOp <= `AluOp_Subu;
                            `OpRt_Tgei   : ALUOp <= `AluOp_Slt;
                            `OpRt_Tgeiu  : ALUOp <= `AluOp_Sltu;
                            `OpRt_Tlti   : ALUOp <= `AluOp_Slt;
                            `OpRt_Tltiu  : ALUOp <= `AluOp_Sltu;
                            `OpRt_Tnei   : ALUOp <= `AluOp_Subu;
                            default      : ALUOp <= `AluOp_Addu;  // Branches don't matter.
                        endcase
                    end
                `Op_Type_CP0 : ALUOp <= `AluOp_Addu;
                `Op_Addi     : ALUOp <= `AluOp_Add;
                `Op_Addiu    : ALUOp <= `AluOp_Addu;
                `Op_Andi     : ALUOp <= `AluOp_And;
                `Op_Jal      : ALUOp <= `AluOp_Addu;
                `Op_Lb       : ALUOp <= `AluOp_Addu;
                `Op_Lbu      : ALUOp <= `AluOp_Addu;
                `Op_Lh       : ALUOp <= `AluOp_Addu;
                `Op_Lhu      : ALUOp <= `AluOp_Addu;
                `Op_Ll       : ALUOp <= `AluOp_Addu;
                `Op_Lui      : ALUOp <= `AluOp_Sllc;
                `Op_Lw       : ALUOp <= `AluOp_Addu;
                `Op_Lwl      : ALUOp <= `AluOp_Addu;
                `Op_Lwr      : ALUOp <= `AluOp_Addu;
                `Op_Ori      : ALUOp <= `AluOp_Or;
                `Op_Sb       : ALUOp <= `AluOp_Addu;
                `Op_Sc       : ALUOp <= `AluOp_Addu;  // XXX Needs HW implement
                `Op_Sh       : ALUOp <= `AluOp_Addu;
                `Op_Slti     : ALUOp <= `AluOp_Slt;
                `Op_Sltiu    : ALUOp <= `AluOp_Sltu;
                `Op_Sw       : ALUOp <= `AluOp_Addu;
                `Op_Swl      : ALUOp <= `AluOp_Addu;
                `Op_Swr      : ALUOp <= `AluOp_Addu;
                `Op_Xori     : ALUOp <= `AluOp_Xor;
                default      : ALUOp <= `AluOp_Addu;
            endcase
        end
    end

    /***
     These remaining options cover portions of the datapath that are not
     controlled directly by the datapath bits. Note that some refer to bits of
     the opcode or other fields, which breaks the otherwise fully-abstracted view
     of instruction encodings. Make sure when adding custom instructions that
     no false positives/negatives are generated here.
     ***/

    // Branch Detection: Options are mutually exclusive.
    assign Branch_EQ  =  OpCode[2] & ~OpCode[1] & ~OpCode[0] &  Cmp_EQ;
    assign Branch_GTZ =  OpCode[2] &  OpCode[1] &  OpCode[0] &  Cmp_GZ;
    assign Branch_LEZ =  OpCode[2] &  OpCode[1] & ~OpCode[0] &  Cmp_LEZ;
    assign Branch_NEQ =  OpCode[2] & ~OpCode[1] &  OpCode[0] & ~Cmp_EQ;
    assign Branch_GEZ = ~OpCode[2] &  Rt[0] & Cmp_GEZ;
    assign Branch_LTZ = ~OpCode[2] & ~Rt[0] & Cmp_LZ;

    assign Branch = Branch_EQ | Branch_GTZ | Branch_LEZ | Branch_NEQ | Branch_GEZ | Branch_LTZ;
    assign PCSrc[1] = (Datapath[15] & ~Datapath[14]) ? Branch : Datapath[15];

    /* In MIPS32, all Branch and Jump operations execute the Branch Delay Slot,
     * or next instruction, regardless if the branch is taken or not. The exception
     * is the "Branch Likely" instruction group. These are deprecated, however, and not
     * implemented here. "IF_Flush" is defined to allow for the cancelation of a
     * Branch Delay Slot should these be implemented later.
     */
    assign IF_Flush = 0;

    // Indicator that next instruction is a Branch Delay Slot.
    assign NextIsDelay = Datapath[15] | Datapath[14];

    // Sign- or Zero-Extension Control. The only ops that require zero-extension are
    // Andi, Ori, and Xori. The following also zero-extends 'lui', however it does not alter the effect of lui.
    assign SignExtend = (OpCode[5:2] != 4'b0011);

    // Move Conditional
    assign Movn = Movc &  Funct[0];
    assign Movz = Movc & ~Funct[0];

    // Coprocessor 0 (Mfc0, Mtc0) control signals.
    assign Mfc0 = ((OpCode == `Op_Type_CP0) && (Rs == `OpRs_MF));
    assign Mtc0 = ((OpCode == `Op_Type_CP0) && (Rs == `OpRs_MT));
    assign Eret = ((OpCode == `Op_Type_CP0) && (Rs == `OpRs_ERET) && (Funct == `Funct_ERET));

    // Coprocessor 1,2,3 accesses (not implemented)
    assign CP1 = (OpCode == `Op_Type_CP1);
    assign CP2 = (OpCode == `Op_Type_CP2);
    assign CP3 = (OpCode == `Op_Type_CP3);

    // Exceptions found in ID
    assign EXC_Sys = ((OpCode == `Op_Type_R) && (Funct == `Funct_Syscall));
    assign EXC_Bp  = ((OpCode == `Op_Type_R) && (Funct == `Funct_Break));

    // Unaligned Memory Accesses (lwl, lwr, swl, swr)
    assign Unaligned_Mem = OpCode[5] & ~OpCode[4] & OpCode[1] & ~OpCode[0];
    assign Left  = Unaligned_Mem & ~OpCode[2];
    assign Right = Unaligned_Mem &  OpCode[2];

    // TODO: Reserved Instruction Exception must still be implemented
    assign EXC_RI  = 0;

endmodule

