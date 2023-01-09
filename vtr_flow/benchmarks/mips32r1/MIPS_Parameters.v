/*
 * File         : MIPS_Parameters.v
 * Project      : University of Utah, XUM Project MIPS32 core
 * Creator(s)   : Grant Ayers (ayers@cs.utah.edu)
 *
 * Standards/Formatting:
 *   Verilog 2001, 4 soft tab, wide column.
 *
 * Description:
 *   Provides a language abstraction for the MIPS32-specific op-codes and
 *   the processor-specific datapath, hazard, and exception bits which
 *   control the processor. These definitions are used extensively
 *   throughout the processor HDL modules.
 */


/*** Exception Vector Locations ***

     When the CPU powers up or is reset, it will begin execution at 'EXC_Vector_Base_Reset'.
     All other exceptions are the sum of a base address and offset:
      - The base address is either a bootstrap or normal value. It is controlled by
        the 'BEV' bit in the CP0 'Status' register. Both base addresses can be mapped to
        the same location.
      - The offset address is either a standard offset (which is always used for
        non-interrupt general exceptions in this processor because it lacks TLB Refill
        and Cache errors), or a special interrupt-only offset for interrupts, which is
        enabled with the 'IV' bit in the CP0 'Cause' register.

     Current Setup:
        General exceptions go to 0x0. Interrupts go to 0x8. Booting starts at 0x10.
*/
`define EXC_Vector_Base_Reset          32'h0000_0010    // MIPS Standard is 0xBFC0_0000
`define EXC_Vector_Base_Other_NoBoot   32'h0000_0000    // MIPS Standard is 0x8000_0000
`define EXC_Vector_Base_Other_Boot     32'h0000_0000    // MIPS Standard is 0xBFC0_0200
`define EXC_Vector_Offset_General      32'h0000_0000    // MIPS Standard is 0x0000_0180
`define EXC_Vector_Offset_Special      32'h0000_0008    // MIPS Standard is 0x0000_0200



/*** Kernel/User Memory Areas ***

     Kernel memory starts at address 0x0. User memory starts at 'UMem_Lower' and extends to
     the end of the address space.

     A distinction is made to protect against accesses to kernel memory while the processor
     is in user mode. Lacking MMU hardware, these addresses are physical, not virtual.
     This simple two-part division of the address space can be extended almost arbitrarily
     in the Data Memory Controller. Note that there is currently no user/kernel space check
     for the Instruction Memory, because it is assumed that instructions are in the kernel space.
*/
`define UMem_Lower 32'h08000000



/*** Processor Endianness ***

     The MIPS Configuration Register (CP0 Register 16 Select 0) specifies the processor's
     endianness. A processor in user mode may switch to reverse endianness, which will be
     the opposite of this `define.
*/
`define Big_Endian 1'b1



/*** Encodings for MIPS32 Release 1 Architecture ***/


/* Op Code Categories */
`define Op_Type_R   6'b00_0000  // Standard R-Type instructions
`define Op_Type_R2  6'b01_1100  // Extended R-Like instructions
`define Op_Type_BI  6'b00_0001  // Branch/Trap extended instructions
`define Op_Type_CP0 6'b01_0000  // Coprocessor 0 instructions
`define Op_Type_CP1 6'b01_0001  // Coprocessor 1 instructions (not implemented)
`define Op_Type_CP2 6'b01_0010  // Coprocessor 2 instructions (not implemented)
`define Op_Type_CP3 6'b01_0011  // Coprocessor 3 instructions (not implemented)
// --------------------------------------
`define Op_Add      `Op_Type_R
`define Op_Addi     6'b00_1000
`define Op_Addiu    6'b00_1001
`define Op_Addu     `Op_Type_R
`define Op_And      `Op_Type_R
`define Op_Andi     6'b00_1100
`define Op_Beq      6'b00_0100
`define Op_Bgez     `Op_Type_BI
`define Op_Bgezal   `Op_Type_BI
`define Op_Bgtz     6'b00_0111
`define Op_Blez     6'b00_0110
`define Op_Bltz     `Op_Type_BI
`define Op_Bltzal   `Op_Type_BI
`define Op_Bne      6'b00_0101
`define Op_Break    `Op_Type_R
`define Op_Clo      `Op_Type_R2
`define Op_Clz      `Op_Type_R2
`define Op_Div      `Op_Type_R
`define Op_Divu     `Op_Type_R
`define Op_Eret     `Op_Type_CP0
`define Op_J        6'b00_0010
`define Op_Jal      6'b00_0011
`define Op_Jalr     `Op_Type_R
`define Op_Jr       `Op_Type_R
`define Op_Lb       6'b10_0000
`define Op_Lbu      6'b10_0100
`define Op_Lh       6'b10_0001
`define Op_Lhu      6'b10_0101
`define Op_Ll       6'b11_0000
`define Op_Lui      6'b00_1111
`define Op_Lw       6'b10_0011
`define Op_Lwl      6'b10_0010
`define Op_Lwr      6'b10_0110
`define Op_Madd     `Op_Type_R2
`define Op_Maddu    `Op_Type_R2
`define Op_Mfc0     `Op_Type_CP0
`define Op_Mfhi     `Op_Type_R
`define Op_Mflo     `Op_Type_R
`define Op_Movn     `Op_Type_R
`define Op_Movz     `Op_Type_R
`define Op_Msub     `Op_Type_R2
`define Op_Msubu    `Op_Type_R2
`define Op_Mtc0     `Op_Type_CP0
`define Op_Mthi     `Op_Type_R
`define Op_Mtlo     `Op_Type_R
`define Op_Mul      `Op_Type_R2
`define Op_Mult     `Op_Type_R
`define Op_Multu    `Op_Type_R
`define Op_Nor      `Op_Type_R
`define Op_Or       `Op_Type_R
`define Op_Ori      6'b00_1101
`define Op_Pref     6'b11_0011  // Prefetch does nothing in this implementation.
`define Op_Sb       6'b10_1000
`define Op_Sc       6'b11_1000
`define Op_Sh       6'b10_1001
`define Op_Sll      `Op_Type_R
`define Op_Sllv     `Op_Type_R
`define Op_Slt      `Op_Type_R
`define Op_Slti     6'b00_1010
`define Op_Sltiu    6'b00_1011
`define Op_Sltu     `Op_Type_R
`define Op_Sra      `Op_Type_R
`define Op_Srav     `Op_Type_R
`define Op_Srl      `Op_Type_R
`define Op_Srlv     `Op_Type_R
`define Op_Sub      `Op_Type_R
`define Op_Subu     `Op_Type_R
`define Op_Sw       6'b10_1011
`define Op_Swl      6'b10_1010
`define Op_Swr      6'b10_1110
`define Op_Syscall  `Op_Type_R
`define Op_Teq      `Op_Type_R
`define Op_Teqi     `Op_Type_BI
`define Op_Tge      `Op_Type_R
`define Op_Tgei     `Op_Type_BI
`define Op_Tgeiu    `Op_Type_BI
`define Op_Tgeu     `Op_Type_R
`define Op_Tlt      `Op_Type_R
`define Op_Tlti     `Op_Type_BI
`define Op_Tltiu    `Op_Type_BI
`define Op_Tltu     `Op_Type_R
`define Op_Tne      `Op_Type_R
`define Op_Tnei     `Op_Type_BI
`define Op_Xor      `Op_Type_R
`define Op_Xori     6'b00_1110

/* Op Code Rt fields for Branches & Traps */
`define OpRt_Bgez   5'b00001
`define OpRt_Bgezal 5'b10001
`define OpRt_Bltz   5'b00000
`define OpRt_Bltzal 5'b10000
`define OpRt_Teqi   5'b01100
`define OpRt_Tgei   5'b01000
`define OpRt_Tgeiu  5'b01001
`define OpRt_Tlti   5'b01010
`define OpRt_Tltiu  5'b01011
`define OpRt_Tnei   5'b01110

/* Op Code Rs fields for Coprocessors */
`define OpRs_MF     5'b00000
`define OpRs_MT     5'b00100

/* Special handling for ERET */
`define OpRs_ERET   5'b10000
`define Funct_ERET  6'b011000

/* Function Codes for R-Type Op Codes */
`define Funct_Add     6'b10_0000
`define Funct_Addu    6'b10_0001
`define Funct_And     6'b10_0100
`define Funct_Break   6'b00_1101
`define Funct_Clo     6'b10_0001    // same as Addu
`define Funct_Clz     6'b10_0000    // same as Add
`define Funct_Div     6'b01_1010
`define Funct_Divu    6'b01_1011
`define Funct_Jr      6'b00_1000
`define Funct_Jalr    6'b00_1001
`define Funct_Madd    6'b00_0000
`define Funct_Maddu   6'b00_0001
`define Funct_Mfhi    6'b01_0000
`define Funct_Mflo    6'b01_0010
`define Funct_Movn    6'b00_1011
`define Funct_Movz    6'b00_1010
`define Funct_Msub    6'b00_0100    // same as Sllv
`define Funct_Msubu   6'b00_0101
`define Funct_Mthi    6'b01_0001
`define Funct_Mtlo    6'b01_0011
`define Funct_Mul     6'b00_0010    // same as Srl
`define Funct_Mult    6'b01_1000
`define Funct_Multu   6'b01_1001
`define Funct_Nor     6'b10_0111
`define Funct_Or      6'b10_0101
`define Funct_Sll     6'b00_0000
`define Funct_Sllv    6'b00_0100
`define Funct_Slt     6'b10_1010
`define Funct_Sltu    6'b10_1011
`define Funct_Sra     6'b00_0011
`define Funct_Srav    6'b00_0111
`define Funct_Srl     6'b00_0010
`define Funct_Srlv    6'b00_0110
`define Funct_Sub     6'b10_0010
`define Funct_Subu    6'b10_0011
`define Funct_Syscall 6'b00_1100
`define Funct_Teq     6'b11_0100
`define Funct_Tge     6'b11_0000
`define Funct_Tgeu    6'b11_0001
`define Funct_Tlt     6'b11_0010
`define Funct_Tltu    6'b11_0011
`define Funct_Tne     6'b11_0110
`define Funct_Xor     6'b10_0110

/* ALU Operations (Implementation) */
`define AluOp_Add    5'd1
`define AluOp_Addu   5'd0
`define AluOp_And    5'd2
`define AluOp_Clo    5'd3
`define AluOp_Clz    5'd4
`define AluOp_Div    5'd5
`define AluOp_Divu   5'd6
`define AluOp_Madd   5'd7
`define AluOp_Maddu  5'd8
`define AluOp_Mfhi   5'd9
`define AluOp_Mflo   5'd10
`define AluOp_Msub   5'd13
`define AluOp_Msubu  5'd14
`define AluOp_Mthi   5'd11
`define AluOp_Mtlo   5'd12
`define AluOp_Mul    5'd15
`define AluOp_Mult   5'd16
`define AluOp_Multu  5'd17
`define AluOp_Nor    5'd18
`define AluOp_Or     5'd19
`define AluOp_Sll    5'd20
`define AluOp_Sllc   5'd21  // Move this if another AluOp is needed
`define AluOp_Sllv   5'd22
`define AluOp_Slt    5'd23
`define AluOp_Sltu   5'd24
`define AluOp_Sra    5'd25
`define AluOp_Srav   5'd26
`define AluOp_Srl    5'd27
`define AluOp_Srlv   5'd28
`define AluOp_Sub    5'd29
`define AluOp_Subu   5'd30
`define AluOp_Xor    5'd31


// Movc:10->11, Trap:9->10, TrapCond:8->9, RegDst:7->8

/*** Datapath ***

     All Signals are Active High. Branching and Jump signals (determined by "PCSrc"),
     as well as ALU operation signals ("ALUOp") are handled by the controller and are not found here.

     Bit  Name          Description
     ------------------------------
     15:  PCSrc         (Instruction Type)
     14:                   11: Instruction is Jump to Register
                           10: Instruction is Branch
                           01: Instruction is Jump to Immediate
                           00: Instruction does not branch nor jump
     13:  Link          (Link on Branch/Jump)
     ------------------------------
     12:  ALUSrc        (ALU Source) [0=ALU input B is 2nd register file output; 1=Immediate value]
     11:  Movc          (Conditional Move)
     10:  Trap          (Trap Instruction)
     9 :  TrapCond      (Trap Condition) [0=ALU result is 0; 1=ALU result is not 0]
     8 :  RegDst        (Register File Target) [0=Rt field; 1=Rd field]
     ------------------------------
     7 :  LLSC          (Load Linked or Store Conditional)
     6 :  MemRead       (Data Memory Read)
     5 :  MemWrite      (Data Memory Write)
     4 :  MemHalf       (Half Word Memory Access)
     3 :  MemByte       (Byte size Memory Access)
     2 :  MemSignExtend (Sign Extend Read Memory) [0=Zero Extend; 1=Sign Extend]
     ------------------------------
     1 :  RegWrite      (Register File Write)
     0 :  MemtoReg      (Memory to Register) [0=Register File write data is ALU output; 1=Is Data Memory]
     ------------------------------
*/
`define DP_None        16'b000_00000_000000_00  // Instructions which require nothing of the main datapath.
`define DP_RType       16'b000_00001_000000_10  // Standard R-Type
`define DP_IType       16'b000_10000_000000_10  // Standard I-Type
`define DP_Branch      16'b100_00000_000000_00  // Standard Branch
`define DP_BranchLink  16'b101_00000_000000_10  // Branch and Link
`define DP_HiLoWr      16'b000_00000_000000_00  // Write to Hi/Lo ALU register (Div,Divu,Mult,Multu,Mthi,Mtlo). Currently 'DP_None'.
`define DP_Jump        16'b010_00000_000000_00  // Standard Jump
`define DP_JumpLink    16'b011_00000_000000_10  // Jump and Link
`define DP_JumpLinkReg 16'b111_00000_000000_10  // Jump and Link Register
`define DP_JumpReg     16'b110_00000_000000_00  // Jump Register
`define DP_LoadByteS   16'b000_10000_010011_11  // Load Byte Signed
`define DP_LoadByteU   16'b000_10000_010010_11  // Load Byte Unsigned
`define DP_LoadHalfS   16'b000_10000_010101_11  // Load Half Signed
`define DP_LoadHalfU   16'b000_10000_010100_11  // Load Half Unsigned
`define DP_LoadWord    16'b000_10000_010000_11  // Load Word
`define DP_ExtWrRt     16'b000_00000_000000_10  // A DP-external write to Rt
`define DP_ExtWrRd     16'b000_00001_000000_10  // A DP-external write to Rd
`define DP_Movc        16'b000_01001_000000_10  // Conditional Move
`define DP_LoadLinked  16'b000_10000_110000_11  // Load Linked
`define DP_StoreCond   16'b000_10000_101000_11  // Store Conditional
`define DP_StoreByte   16'b000_10000_001010_00  // Store Byte
`define DP_StoreHalf   16'b000_10000_001100_00  // Store Half
`define DP_StoreWord   16'b000_10000_001000_00  // Store Word
`define DP_TrapRegCNZ  16'b000_00110_000000_00  // Trap using Rs and Rt,  non-zero ALU (Tlt,  Tltu,  Tne)
`define DP_TrapRegCZ   16'b000_00100_000000_00  // Trap using RS and Rt,  zero ALU     (Teq,  Tge,   Tgeu)
`define DP_TrapImmCNZ  16'b000_10110_000000_00  // Trap using Rs and Imm, non-zero ALU (Tlti, Tltiu, Tnei)
`define DP_TrapImmCZ   16'b000_10100_000000_00  // Trap using Rs and Imm, zero ALU     (Teqi, Tgei,  Tgeiu)
//--------------------------------------------------------
`define DP_Add     `DP_RType
`define DP_Addi    `DP_IType
`define DP_Addiu   `DP_IType
`define DP_Addu    `DP_RType
`define DP_And     `DP_RType
`define DP_Andi    `DP_IType
`define DP_Beq     `DP_Branch
`define DP_Bgez    `DP_Branch
`define DP_Bgezal  `DP_BranchLink
`define DP_Bgtz    `DP_Branch
`define DP_Blez    `DP_Branch
`define DP_Bltz    `DP_Branch
`define DP_Bltzal  `DP_BranchLink
`define DP_Bne     `DP_Branch
`define DP_Break   `DP_None
`define DP_Clo     `DP_RType
`define DP_Clz     `DP_RType
`define DP_Div     `DP_HiLoWr
`define DP_Divu    `DP_HiLoWr
`define DP_Eret    `DP_None
`define DP_J       `DP_Jump
`define DP_Jal     `DP_JumpLink
`define DP_Jalr    `DP_JumpLinkReg
`define DP_Jr      `DP_JumpReg
`define DP_Lb      `DP_LoadByteS
`define DP_Lbu     `DP_LoadByteU
`define DP_Lh      `DP_LoadHalfS
`define DP_Lhu     `DP_LoadHalfU
`define DP_Ll      `DP_LoadLinked
`define DP_Lui     `DP_IType
`define DP_Lw      `DP_LoadWord
`define DP_Lwl     `DP_LoadWord
`define DP_Lwr     `DP_LoadWord
`define DP_Madd    `DP_HiLoWr
`define DP_Maddu   `DP_HiLoWr
`define DP_Mfc0    `DP_ExtWrRt
`define DP_Mfhi    `DP_ExtWrRd
`define DP_Mflo    `DP_ExtWrRd
`define DP_Movn    `DP_Movc
`define DP_Movz    `DP_Movc
`define DP_Msub    `DP_HiLoWr
`define DP_Msubu   `DP_HiLoWr
`define DP_Mtc0    `DP_None
`define DP_Mthi    `DP_HiLoWr
`define DP_Mtlo    `DP_HiLoWr
`define DP_Mul     `DP_RType
`define DP_Mult    `DP_HiLoWr
`define DP_Multu   `DP_HiLoWr
`define DP_Nor     `DP_RType
`define DP_Or      `DP_RType
`define DP_Ori     `DP_IType
`define DP_Pref    `DP_None         // Not Implemented
`define DP_Sb      `DP_StoreByte
`define DP_Sc      `DP_StoreCond
`define DP_Sh      `DP_StoreHalf
`define DP_Sll     `DP_RType
`define DP_Sllv    `DP_RType
`define DP_Slt     `DP_RType
`define DP_Slti    `DP_IType
`define DP_Sltiu   `DP_IType
`define DP_Sltu    `DP_RType
`define DP_Sra     `DP_RType
`define DP_Srav    `DP_RType
`define DP_Srl     `DP_RType
`define DP_Srlv    `DP_RType
`define DP_Sub     `DP_RType
`define DP_Subu    `DP_RType
`define DP_Sw      `DP_StoreWord
`define DP_Swl     `DP_StoreWord
`define DP_Swr     `DP_StoreWord
`define DP_Syscall `DP_None
`define DP_Teq     `DP_TrapRegCZ
`define DP_Teqi    `DP_TrapImmCZ
`define DP_Tge     `DP_TrapRegCZ
`define DP_Tgei    `DP_TrapImmCZ
`define DP_Tgeiu   `DP_TrapImmCZ
`define DP_Tgeu    `DP_TrapRegCZ
`define DP_Tlt     `DP_TrapRegCNZ
`define DP_Tlti    `DP_TrapImmCNZ
`define DP_Tltiu   `DP_TrapImmCNZ
`define DP_Tltu    `DP_TrapRegCNZ
`define DP_Tne     `DP_TrapRegCNZ
`define DP_Tnei    `DP_TrapImmCNZ
`define DP_Xor     `DP_RType
`define DP_Xori    `DP_IType




/*** Exception Information ***

     All signals are Active High.

     Bit  Meaning
     ------------
     2:   Instruction can cause exceptions in ID
     1:   Instruction can cause exceptions in EX
     0:   Instruction can cause exceptions in MEM
*/
`define EXC_None 3'b000
`define EXC_ID   3'b100
`define EXC_EX   3'b010
`define EXC_MEM  3'b001
//--------------------------------
`define EXC_Add     `EXC_EX
`define EXC_Addi    `EXC_EX
`define EXC_Addiu   `EXC_None
`define EXC_Addu    `EXC_None
`define EXC_And     `EXC_None
`define EXC_Andi    `EXC_None
`define EXC_Beq     `EXC_None
`define EXC_Bgez    `EXC_None
`define EXC_Bgezal  `EXC_None
`define EXC_Bgtz    `EXC_None
`define EXC_Blez    `EXC_None
`define EXC_Bltz    `EXC_None
`define EXC_Bltzal  `EXC_None
`define EXC_Bne     `EXC_None
`define EXC_Break   `EXC_ID
`define EXC_Clo     `EXC_None
`define EXC_Clz     `EXC_None
`define EXC_Div     `EXC_None
`define EXC_Divu    `EXC_None
`define EXC_Eret    `EXC_ID
`define EXC_J       `EXC_None
`define EXC_Jal     `EXC_None
`define EXC_Jalr    `EXC_None
`define EXC_Jr      `EXC_None
`define EXC_Lb      `EXC_MEM
`define EXC_Lbu     `EXC_MEM
`define EXC_Lh      `EXC_MEM
`define EXC_Lhu     `EXC_MEM
`define EXC_Ll      `EXC_MEM
`define EXC_Lui     `EXC_None
`define EXC_Lw      `EXC_MEM
`define EXC_Lwl     `EXC_MEM
`define EXC_Lwr     `EXC_MEM
`define EXC_Madd    `EXC_None
`define EXC_Maddu   `EXC_None
`define EXC_Mfc0    `EXC_ID
`define EXC_Mfhi    `EXC_None
`define EXC_Mflo    `EXC_None
`define EXC_Movn    `EXC_None
`define EXC_Movz    `EXC_None
`define EXC_Msub    `EXC_None
`define EXC_Msubu   `EXC_None
`define EXC_Mtc0    `EXC_ID
`define EXC_Mthi    `EXC_None
`define EXC_Mtlo    `EXC_None
`define EXC_Mul     `EXC_None
`define EXC_Mult    `EXC_None
`define EXC_Multu   `EXC_None
`define EXC_Nor     `EXC_None
`define EXC_Or      `EXC_None
`define EXC_Ori     `EXC_None
`define EXC_Pref    `EXC_None   // XXX
`define EXC_Sb      `EXC_MEM
`define EXC_Sc      `EXC_MEM
`define EXC_Sh      `EXC_MEM
`define EXC_Sll     `EXC_None
`define EXC_Sllv    `EXC_None
`define EXC_Slt     `EXC_None
`define EXC_Slti    `EXC_None
`define EXC_Sltiu   `EXC_None
`define EXC_Sltu    `EXC_None
`define EXC_Sra     `EXC_None
`define EXC_Srav    `EXC_None
`define EXC_Srl     `EXC_None
`define EXC_Srlv    `EXC_None
`define EXC_Sub     `EXC_EX
`define EXC_Subu    `EXC_None
`define EXC_Sw      `EXC_MEM
`define EXC_Swl     `EXC_MEM
`define EXC_Swr     `EXC_MEM
`define EXC_Syscall `EXC_ID
`define EXC_Teq     `EXC_MEM
`define EXC_Teqi    `EXC_MEM
`define EXC_Tge     `EXC_MEM
`define EXC_Tgei    `EXC_MEM
`define EXC_Tgeiu   `EXC_MEM
`define EXC_Tgeu    `EXC_MEM
`define EXC_Tlt     `EXC_MEM
`define EXC_Tlti    `EXC_MEM
`define EXC_Tltiu   `EXC_MEM
`define EXC_Tltu    `EXC_MEM
`define EXC_Tne     `EXC_MEM
`define EXC_Tnei    `EXC_MEM
`define EXC_Xor     `EXC_None
`define EXC_Xori    `EXC_None




/*** Hazard & Forwarding Datapath ***

     All signals are Active High.

     Bit  Meaning
     ------------
     7:   Wants Rs by ID
     6:   Needs Rs by ID
     5:   Wants Rt by ID
     4:   Needs Rt by ID
     3:   Wants Rs by EX
     2:   Needs Rs by EX
     1:   Wants Rt by EX
     0:   Needs Rt by EX
*/
`define HAZ_Nothing  8'b00000000    // Jumps, Lui, Mfhi/lo, special, etc.
`define HAZ_IDRsIDRt 8'b11110000    // Beq, Bne, Traps
`define HAZ_IDRs     8'b11000000    // Most branches, Jumps to registers
`define HAZ_IDRt     8'b00110000    // Mtc0
`define HAZ_IDRtEXRs 8'b10111100    // Movn, Movz
`define HAZ_EXRsEXRt 8'b10101111    // Many R-Type ops
`define HAZ_EXRs     8'b10001100    // Immediates: Loads, Clo/z, Mthi/lo, etc.
`define HAZ_EXRsWRt  8'b10101110    // Stores
`define HAZ_EXRt     8'b00100011    // Shifts using Shamt field
//-----------------------------------------
`define HAZ_Add     `HAZ_EXRsEXRt
`define HAZ_Addi    `HAZ_EXRs
`define HAZ_Addiu   `HAZ_EXRs
`define HAZ_Addu    `HAZ_EXRsEXRt
`define HAZ_And     `HAZ_EXRsEXRt
`define HAZ_Andi    `HAZ_EXRs
`define HAZ_Beq     `HAZ_IDRsIDRt
`define HAZ_Bgez    `HAZ_IDRs
`define HAZ_Bgezal  `HAZ_IDRs
`define HAZ_Bgtz    `HAZ_IDRs
`define HAZ_Blez    `HAZ_IDRs
`define HAZ_Bltz    `HAZ_IDRs
`define HAZ_Bltzal  `HAZ_IDRs
`define HAZ_Bne     `HAZ_IDRsIDRt
`define HAZ_Break   `HAZ_Nothing
`define HAZ_Clo     `HAZ_EXRs
`define HAZ_Clz     `HAZ_EXRs
`define HAZ_Div     `HAZ_EXRsEXRt
`define HAZ_Divu    `HAZ_EXRsEXRt
`define HAZ_Eret    `HAZ_Nothing
`define HAZ_J       `HAZ_Nothing
`define HAZ_Jal     `HAZ_Nothing
`define HAZ_Jalr    `HAZ_IDRs
`define HAZ_Jr      `HAZ_IDRs
`define HAZ_Lb      `HAZ_EXRs
`define HAZ_Lbu     `HAZ_EXRs
`define HAZ_Lh      `HAZ_EXRs
`define HAZ_Lhu     `HAZ_EXRs
`define HAZ_Ll      `HAZ_EXRs
`define HAZ_Lui     `HAZ_Nothing
`define HAZ_Lw      `HAZ_EXRs
`define HAZ_Lwl     `HAZ_EXRsEXRt
`define HAZ_Lwr     `HAZ_EXRsEXRt
`define HAZ_Madd    `HAZ_EXRsEXRt
`define HAZ_Maddu   `HAZ_EXRsEXRt
`define HAZ_Mfc0    `HAZ_Nothing
`define HAZ_Mfhi    `HAZ_Nothing
`define HAZ_Mflo    `HAZ_Nothing
`define HAZ_Movn    `HAZ_IDRtEXRs
`define HAZ_Movz    `HAZ_IDRtEXRs
`define HAZ_Msub    `HAZ_EXRsEXRt
`define HAZ_Msubu   `HAZ_EXRsEXRt
`define HAZ_Mtc0    `HAZ_IDRt
`define HAZ_Mthi    `HAZ_EXRs
`define HAZ_Mtlo    `HAZ_EXRs
`define HAZ_Mul     `HAZ_EXRsEXRt
`define HAZ_Mult    `HAZ_EXRsEXRt
`define HAZ_Multu   `HAZ_EXRsEXRt
`define HAZ_Nor     `HAZ_EXRsEXRt
`define HAZ_Or      `HAZ_EXRsEXRt
`define HAZ_Ori     `HAZ_EXRs
`define HAZ_Pref    `HAZ_Nothing // XXX
`define HAZ_Sb      `HAZ_EXRsWRt
`define HAZ_Sc      `HAZ_EXRsWRt
`define HAZ_Sh      `HAZ_EXRsWRt
`define HAZ_Sll     `HAZ_EXRt
`define HAZ_Sllv    `HAZ_EXRsEXRt
`define HAZ_Slt     `HAZ_EXRsEXRt
`define HAZ_Slti    `HAZ_EXRs
`define HAZ_Sltiu   `HAZ_EXRs
`define HAZ_Sltu    `HAZ_EXRsEXRt
`define HAZ_Sra     `HAZ_EXRt
`define HAZ_Srav    `HAZ_EXRsEXRt
`define HAZ_Srl     `HAZ_EXRt
`define HAZ_Srlv    `HAZ_EXRsEXRt
`define HAZ_Sub     `HAZ_EXRsEXRt
`define HAZ_Subu    `HAZ_EXRsEXRt
`define HAZ_Sw      `HAZ_EXRsWRt
`define HAZ_Swl     `HAZ_EXRsWRt
`define HAZ_Swr     `HAZ_EXRsWRt
`define HAZ_Syscall `HAZ_Nothing
`define HAZ_Teq     `HAZ_EXRsEXRt
`define HAZ_Teqi    `HAZ_EXRs
`define HAZ_Tge     `HAZ_EXRsEXRt
`define HAZ_Tgei    `HAZ_EXRs
`define HAZ_Tgeiu   `HAZ_EXRs
`define HAZ_Tgeu    `HAZ_EXRsEXRt
`define HAZ_Tlt     `HAZ_EXRsEXRt
`define HAZ_Tlti    `HAZ_EXRs
`define HAZ_Tltiu   `HAZ_EXRs
`define HAZ_Tltu    `HAZ_EXRsEXRt
`define HAZ_Tne     `HAZ_EXRsEXRt
`define HAZ_Tnei    `HAZ_EXRs
`define HAZ_Xor     `HAZ_EXRsEXRt
`define HAZ_Xori    `HAZ_EXRs

