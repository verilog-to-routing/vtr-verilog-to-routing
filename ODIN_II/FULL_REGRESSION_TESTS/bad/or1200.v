`define OR1200_DCFGR_NDP		3'h0	// Zero DVR/DCR pairs
`define OR1200_DCFGR_WPCI		1'b0	// WP counters not impl.

`define OR1200_DCFGR_RES1		28'h0000000




`define OR1200_M2R_BYTE0 4'b0000
`define OR1200_M2R_BYTE1 4'b0001
`define OR1200_M2R_BYTE2 4'b0010
`define OR1200_M2R_BYTE3 4'b0011
`define OR1200_M2R_EXTB0 4'b0100
`define OR1200_M2R_EXTB1 4'b0101
`define OR1200_M2R_EXTB2 4'b0110
`define OR1200_M2R_EXTB3 4'b0111
`define OR1200_M2R_ZERO  4'b0000


`define OR1200_ICCFGR_NCW		3'h0	// 1 cache way
`define OR1200_ICCFGR_NCS 9	// Num cache sets
`define OR1200_ICCFGR_CBS 9	// 16 byte cache block
`define OR1200_ICCFGR_CWS		1'b0	// Irrelevant
`define OR1200_ICCFGR_CCRI		1'b1	// Cache control reg impl.
`define OR1200_ICCFGR_CBIRI		1'b1	// Cache block inv reg impl.
`define OR1200_ICCFGR_CBPRI		1'b0	// Cache block prefetch reg not impl.
`define OR1200_ICCFGR_CBLRI		1'b0	// Cache block lock reg not impl.
`define OR1200_ICCFGR_CBFRI		1'b1	// Cache block flush reg impl.
`define OR1200_ICCFGR_CBWBRI		1'b0	// Irrelevant
`define OR1200_ICCFGR_RES1		17'h00000

//`define OR1200_ICCFGR_NCW_BITS		2:0
//`define OR1200_ICCFGR_NCS_BITS		6:3
`define OR1200_ICCFGR_CBS_BITS		7
`define OR1200_ICCFGR_CWS_BITS		8
`define OR1200_ICCFGR_CCRI_BITS		9
`define OR1200_ICCFGR_CBIRI_BITS	10
`define OR1200_ICCFGR_CBPRI_BITS	11
`define OR1200_ICCFGR_CBLRI_BITS	12
`define OR1200_ICCFGR_CBFRI_BITS	13
`define OR1200_ICCFGR_CBWBRI_BITS	14
//`define OR1200_ICCFGR_RES1_BITS	31:15


`define OR1200_DCCFGR_NCW		3'h0	// 1 cache way
`define OR1200_DCCFGR_NCS 9	// Num cache sets
`define OR1200_DCCFGR_CBS 9	// 16 byte cache block
`define OR1200_DCCFGR_CWS		1'b0	// Write-through strategy
`define OR1200_DCCFGR_CCRI		1'b1	// Cache control reg impl.
`define OR1200_DCCFGR_CBIRI		1'b1	// Cache block inv reg impl.
`define OR1200_DCCFGR_CBPRI		1'b0	// Cache block prefetch reg not impl.
`define OR1200_DCCFGR_CBLRI		1'b0	// Cache block lock reg not impl.
`define OR1200_DCCFGR_CBFRI		1'b1	// Cache block flush reg impl.
`define OR1200_DCCFGR_CBWBRI		1'b0	// Cache block WB reg not impl.
`define OR1200_DCCFGR_RES1		17'h00000



//`define OR1200_DCCFGR_NCW_BITS		2:0
//`define OR1200_DCCFGR_NCS_BITS		6:3
`define OR1200_DCCFGR_CBS_BITS		7
`define OR1200_DCCFGR_CWS_BITS		8
`define OR1200_DCCFGR_CCRI_BITS		9
`define OR1200_DCCFGR_CBIRI_BITS	10
`define OR1200_DCCFGR_CBPRI_BITS	11
`define OR1200_DCCFGR_CBLRI_BITS	12
`define OR1200_DCCFGR_CBFRI_BITS	13
`define OR1200_DCCFGR_CBWBRI_BITS	14
//`define OR1200_DCCFGR_RES1_BITS	31:15


`define OR1200_IMMUCFGR_NTW		2'h0	// 1 TLB way
`define OR1200_IMMUCFGR_NTS 3'b101	// Num TLB sets
`define OR1200_IMMUCFGR_NAE		3'h0	// No ATB entry
`define OR1200_IMMUCFGR_CRI		1'b0	// No control reg
`define OR1200_IMMUCFGR_PRI		1'b0	// No protection reg
`define OR1200_IMMUCFGR_TEIRI		1'b1	// TLB entry inv reg impl
`define OR1200_IMMUCFGR_HTR		1'b0	// No HW TLB reload
`define OR1200_IMMUCFGR_RES1		20'h00000

// CPUCFGR fields
//`define OR1200_CPUCFGR_NSGF_BITS	3:0
`define OR1200_CPUCFGR_HGF_BITS	4
`define OR1200_CPUCFGR_OB32S_BITS	5
`define OR1200_CPUCFGR_OB64S_BITS	6
`define OR1200_CPUCFGR_OF32S_BITS	7
`define OR1200_CPUCFGR_OF64S_BITS	8
`define OR1200_CPUCFGR_OV64S_BITS	9
//`define OR1200_CPUCFGR_RES1_BITS	31:10

// CPUCFGR values
`define OR1200_CPUCFGR_NSGF		4'h0
`define OR1200_CPUCFGR_HGF		1'b0
`define OR1200_CPUCFGR_OB32S		1'b1
`define OR1200_CPUCFGR_OB64S		1'b0
`define OR1200_CPUCFGR_OF32S		1'b0
`define OR1200_CPUCFGR_OF64S		1'b0
`define OR1200_CPUCFGR_OV64S		1'b0
`define OR1200_CPUCFGR_RES1		22'h000000

// DMMUCFGR fields
/*
`define OR1200_DMMUCFGR_NTW_BITS	1:0
`define OR1200_DMMUCFGR_NTS_BITS	4:2
`define OR1200_DMMUCFGR_NAE_BITS	7:5
*/
`define OR1200_DMMUCFGR_CRI_BITS	8
`define OR1200_DMMUCFGR_PRI_BITS	9
`define OR1200_DMMUCFGR_TEIRI_BITS	10
`define OR1200_DMMUCFGR_HTR_BITS	11
//`define OR1200_DMMUCFGR_RES1_BITS	31:12

// DMMUCFGR values

`define OR1200_DMMUCFGR_NTW		2'h0	// 1 TLB way
`define OR1200_DMMUCFGR_NTS 3'b110	// Num TLB sets
`define OR1200_DMMUCFGR_NAE		3'h0	// No ATB entries
`define OR1200_DMMUCFGR_CRI		1'b0	// No control register
`define OR1200_DMMUCFGR_PRI		1'b0	// No protection reg
`define OR1200_DMMUCFGR_TEIRI		1'b1	// TLB entry inv reg impl.
`define OR1200_DMMUCFGR_HTR		1'b0	// No HW TLB reload
`define OR1200_DMMUCFGR_RES1		20'h00000


// IMMUCFGR fields
/*
`define OR1200_IMMUCFGR_NTW_BITS	1:0
`define OR1200_IMMUCFGR_NTS_BITS	4:2
`define OR1200_IMMUCFGR_NAE_BITS	7:5
*/
`define OR1200_IMMUCFGR_CRI_BITS	8
`define OR1200_IMMUCFGR_PRI_BITS	9
`define OR1200_IMMUCFGR_TEIRI_BITS	10
`define OR1200_IMMUCFGR_HTR_BITS	11

//`define OR1200_IMMUCFGR_RES1_BITS	31:12



`define OR1200_SPRGRP_SYS_VR		4'h0
`define OR1200_SPRGRP_SYS_UPR		4'h1
`define OR1200_SPRGRP_SYS_CPUCFGR	4'h2
`define OR1200_SPRGRP_SYS_DMMUCFGR	4'h3
`define OR1200_SPRGRP_SYS_IMMUCFGR	4'h4
`define OR1200_SPRGRP_SYS_DCCFGR	4'h5
`define OR1200_SPRGRP_SYS_ICCFGR	4'h6
`define OR1200_SPRGRP_SYS_DCFGR	4'h7

// VR fields
/*
`define OR1200_VR_REV_BITS		5:0
`define OR1200_VR_RES1_BITS		15:6
`define OR1200_VR_CFG_BITS		23:16
`define OR1200_VR_VER_BITS		31:24
*/
// VR values
`define OR1200_VR_REV			6'h01
`define OR1200_VR_RES1			10'h000
`define OR1200_VR_CFG			8'h00
`define OR1200_VR_VER			8'h12

`define OR1200_UPR_UP_BITS		0
`define OR1200_UPR_DCP_BITS		1
`define OR1200_UPR_ICP_BITS		2
`define OR1200_UPR_DMP_BITS		3
`define OR1200_UPR_IMP_BITS		4
`define OR1200_UPR_MP_BITS		5
`define OR1200_UPR_DUP_BITS		6
`define OR1200_UPR_PCUP_BITS		7
`define OR1200_UPR_PMP_BITS		8
`define OR1200_UPR_PICP_BITS		9
`define OR1200_UPR_TTP_BITS		10
/*
`define OR1200_UPR_RES1_BITS		23:11
`define OR1200_UPR_CUP_BITS		31:24
*/

`define OR1200_UPR_RES1			13'h0000
`define OR1200_UPR_CUP			8'h00


`define OR1200_DU_DSR_WIDTH 14


`define OR1200_EXCEPT_UNUSED		3'hf
`define OR1200_EXCEPT_TRAP		3'he
`define OR1200_EXCEPT_BREAK		3'hd
`define OR1200_EXCEPT_SYSCALL		3'hc
`define OR1200_EXCEPT_RANGE		3'hb
`define OR1200_EXCEPT_ITLBMISS		3'ha
`define OR1200_EXCEPT_DTLBMISS		3'h9
`define OR1200_EXCEPT_INT		3'h8
`define OR1200_EXCEPT_ILLEGAL		3'h7
`define OR1200_EXCEPT_ALIGN		3'h6
`define OR1200_EXCEPT_TICK		3'h5
`define OR1200_EXCEPT_IPF		3'h4
`define OR1200_EXCEPT_DPF		3'h3
`define OR1200_EXCEPT_BUSERR		3'h2
`define OR1200_EXCEPT_RESET		3'h1
`define OR1200_EXCEPT_NONE		3'h0

`define OR1200_OPERAND_WIDTH		32
`define OR1200_REGFILE_ADDR_WIDTH	5
`define OR1200_ALUOP_WIDTH	4
`define OR1200_ALUOP_NOP	4'b000

`define OR1200_ALUOP_ADD	4'b0000
`define OR1200_ALUOP_ADDC	4'b0001
`define OR1200_ALUOP_SUB	4'b0010
`define OR1200_ALUOP_AND	4'b0011
`define OR1200_ALUOP_OR		4'b0100
`define OR1200_ALUOP_XOR	4'b0101
`define OR1200_ALUOP_MUL	4'b0110
`define OR1200_ALUOP_CUST5	4'b0111
`define OR1200_ALUOP_SHROT	4'b1000
`define OR1200_ALUOP_DIV	4'b1001
`define OR1200_ALUOP_DIVU	4'b1010

`define OR1200_ALUOP_IMM	4'b1011
`define OR1200_ALUOP_MOVHI	4'b1100
`define OR1200_ALUOP_COMP	4'b1101
`define OR1200_ALUOP_MTSR	4'b1110
`define OR1200_ALUOP_MFSR	4'b1111
`define OR1200_ALUOP_CMOV 4'b1110
`define OR1200_ALUOP_FF1  4'b1111

`define OR1200_MACOP_WIDTH	2
`define OR1200_MACOP_NOP	2'b00
`define OR1200_MACOP_MAC	2'b01
`define OR1200_MACOP_MSB	2'b10


`define OR1200_SHROTOP_WIDTH	2
`define OR1200_SHROTOP_NOP	2'b00
`define OR1200_SHROTOP_SLL	2'b00
`define OR1200_SHROTOP_SRL	2'b01
`define OR1200_SHROTOP_SRA	2'b10
`define OR1200_SHROTOP_ROR	2'b11

// Execution cycles per instruction
`define OR1200_MULTICYCLE_WIDTH	2
`define OR1200_ONE_CYCLE		2'b00
`define OR1200_TWO_CYCLES		2'b01

// Operand MUX selects
`define OR1200_SEL_WIDTH		2
`define OR1200_SEL_RF			2'b00
`define OR1200_SEL_IMM			2'b01
`define OR1200_SEL_EX_FORW		2'b10
`define OR1200_SEL_WB_FORW		2'b11

//
// BRANCHOPs
//
`define OR1200_BRANCHOP_WIDTH		3
`define OR1200_BRANCHOP_NOP		3'b000
`define OR1200_BRANCHOP_J		3'b001
`define OR1200_BRANCHOP_JR		3'b010
`define OR1200_BRANCHOP_BAL		3'b011
`define OR1200_BRANCHOP_BF		3'b100
`define OR1200_BRANCHOP_BNF		3'b101
`define OR1200_BRANCHOP_RFE		3'b110

//
// LSUOPs
//
// Bit 0: sign extend
// Bits 1-2: 00 doubleword, 01 byte, 10 halfword, 11 singleword
// Bit 3: 0 load, 1 store
`define OR1200_LSUOP_WIDTH		4
`define OR1200_LSUOP_NOP		4'b0000
`define OR1200_LSUOP_LBZ		4'b0010
`define OR1200_LSUOP_LBS		4'b0011
`define OR1200_LSUOP_LHZ		4'b0100
`define OR1200_LSUOP_LHS		4'b0101
`define OR1200_LSUOP_LWZ		4'b0110
`define OR1200_LSUOP_LWS		4'b0111
`define OR1200_LSUOP_LD		4'b0001
`define OR1200_LSUOP_SD		4'b1000
`define OR1200_LSUOP_SB		4'b1010
`define OR1200_LSUOP_SH		4'b1100
`define OR1200_LSUOP_SW		4'b1110

// FETCHOPs
`define OR1200_FETCHOP_WIDTH		1
`define OR1200_FETCHOP_NOP		1'b0
`define OR1200_FETCHOP_LW		1'b1

//
// Register File Write-Back OPs
//
// Bit 0: register file write enable
// Bits 2-1: write-back mux selects
`define OR1200_RFWBOP_WIDTH		3
`define OR1200_RFWBOP_NOP		3'b000
`define OR1200_RFWBOP_ALU		3'b001
`define OR1200_RFWBOP_LSU		3'b011
`define OR1200_RFWBOP_SPRS		3'b101
`define OR1200_RFWBOP_LR		3'b111

// Compare instructions
`define OR1200_COP_SFEQ       3'b000
`define OR1200_COP_SFNE       3'b001
`define OR1200_COP_SFGT       3'b010
`define OR1200_COP_SFGE       3'b011
`define OR1200_COP_SFLT       3'b100
`define OR1200_COP_SFLE       3'b101
`define OR1200_COP_X          3'b111
`define OR1200_SIGNED_COMPARE 3'b011
`define OR1200_COMPOP_WIDTH	4

//
// TAGs for instruction bus
//
`define OR1200_ITAG_IDLE	4'h0	// idle bus
`define	OR1200_ITAG_NI		4'h1	// normal insn
`define OR1200_ITAG_BE		4'hb	// Bus error exception
`define OR1200_ITAG_PE		4'hc	// Page fault exception
`define OR1200_ITAG_TE		4'hd	// TLB miss exception

//
// TAGs for data bus
//
`define OR1200_DTAG_IDLE	4'h0	// idle bus
`define	OR1200_DTAG_ND		4'h1	// normal data
`define OR1200_DTAG_AE		4'ha	// Alignment exception
`define OR1200_DTAG_BE		4'hb	// Bus error exception
`define OR1200_DTAG_PE		4'hc	// Page fault exception
`define OR1200_DTAG_TE		4'hd	// TLB miss exception



`define OR1200_DU_DSR_RSTE	0
`define OR1200_DU_DSR_BUSEE	1
`define OR1200_DU_DSR_DPFE	2
`define OR1200_DU_DSR_IPFE	3
`define OR1200_DU_DSR_TTE	4
`define OR1200_DU_DSR_AE	5
`define OR1200_DU_DSR_IIE	6
`define OR1200_DU_DSR_IE	7
`define OR1200_DU_DSR_DME	8
`define OR1200_DU_DSR_IME	9
`define OR1200_DU_DSR_RE	10
`define OR1200_DU_DSR_SCE	11
`define OR1200_DU_DSR_BE	12
`define OR1200_DU_DSR_TE	13
//////////////////////////////////////////////
//
// ORBIS32 ISA specifics
//

// SHROT_OP position in machine word
//`define OR1200_SHROTOP_POS		7:6

// ALU instructions multicycle field in machine word
//`define OR1200_ALUMCYC_POS		9:8

//
// Instruction opcode groups (basic)
//
`define OR1200_OR32_J                 6'b000000
`define OR1200_OR32_JAL               6'b000001
`define OR1200_OR32_BNF               6'b000011
`define OR1200_OR32_BF                6'b000100
`define OR1200_OR32_NOP               6'b000101
`define OR1200_OR32_MOVHI             6'b000110
`define OR1200_OR32_XSYNC             6'b001000
`define OR1200_OR32_RFE               6'b001001
/* */
`define OR1200_OR32_JR                6'b010001
`define OR1200_OR32_JALR              6'b010010
`define OR1200_OR32_MACI              6'b010011
/* */
`define OR1200_OR32_LWZ               6'b100001
`define OR1200_OR32_LBZ               6'b100011
`define OR1200_OR32_LBS               6'b100100
`define OR1200_OR32_LHZ               6'b100101
`define OR1200_OR32_LHS               6'b100110
`define OR1200_OR32_ADDI              6'b100111
`define OR1200_OR32_ADDIC             6'b101000
`define OR1200_OR32_ANDI              6'b101001
`define OR1200_OR32_ORI               6'b101010
`define OR1200_OR32_XORI              6'b101011
`define OR1200_OR32_MULI              6'b101100
`define OR1200_OR32_MFSPR             6'b101101
`define OR1200_OR32_SH_ROTI 	      6'b101110
`define OR1200_OR32_SFXXI             6'b101111
/* */
`define OR1200_OR32_MTSPR             6'b110000
`define OR1200_OR32_MACMSB            6'b110001
/* */
`define OR1200_OR32_SW                6'b110101
`define OR1200_OR32_SB                6'b110110
`define OR1200_OR32_SH                6'b110111
`define OR1200_OR32_ALU               6'b111000
`define OR1200_OR32_SFXX              6'b111001
`define OR1200_OR32_CUST5             6'b111100


/////////////////////////////////////////////////////
//
// Exceptions
//

//
// Exception vectors per OR1K architecture:
// 0xPPPPP100 - reset
// 0xPPPPP200 - bus error
// ... etc
// where P represents exception prefix.
//
// Exception vectors can be customized as per
// the following formula:
// 0xPPPPPNVV - exception N
//
// P represents exception prefix
// N represents exception N
// VV represents length of the individual vector space,
//   usually it is 8 bits wide and starts with all bits zero
//

//
// PPPPP and VV parts
//
// Sum of these two defines needs to be 28
//
`define OR1200_EXCEPT_EPH0_P 20'h00000
`define OR1200_EXCEPT_EPH1_P 20'hF0000
`define OR1200_EXCEPT_V		   8'h00

//
// N part width
//
`define OR1200_EXCEPT_WIDTH 4

`define OR1200_SPR_GROUP_SYS	5'b00000
`define OR1200_SPR_GROUP_DMMU	5'b00001
`define OR1200_SPR_GROUP_IMMU	5'b00010
`define OR1200_SPR_GROUP_DC	5'b00011
`define OR1200_SPR_GROUP_IC	5'b00100
`define OR1200_SPR_GROUP_MAC	5'b00101
`define OR1200_SPR_GROUP_DU	5'b00110
`define OR1200_SPR_GROUP_PM	5'b01000
`define OR1200_SPR_GROUP_PIC	5'b01001
`define OR1200_SPR_GROUP_TT	5'b01010


/////////////////////////////////////////////////////
//
// System group
//

//
// System registers
//
`define OR1200_SPR_CFGR		7'b0000000
`define OR1200_SPR_RF		6'b100000	// 1024 >> 5
`define OR1200_SPR_NPC		11'b00000010000
`define OR1200_SPR_SR		11'b00000010001
`define OR1200_SPR_PPC		11'b00000010010
`define OR1200_SPR_EPCR		11'b00000100000
`define OR1200_SPR_EEAR		11'b00000110000
`define OR1200_SPR_ESR		11'b00001000000

//
// SR bits
//
`define OR1200_SR_WIDTH 16
`define OR1200_SR_SM   0
`define OR1200_SR_TEE  1
`define OR1200_SR_IEE  2
`define OR1200_SR_DCE  3
`define OR1200_SR_ICE  4
`define OR1200_SR_DME  5
`define OR1200_SR_IME  6
`define OR1200_SR_LEE  7
`define OR1200_SR_CE   8
`define OR1200_SR_F    9
`define OR1200_SR_CY   10	// Unused
`define OR1200_SR_OV   11	// Unused
`define OR1200_SR_OVE  12	// Unused
`define OR1200_SR_DSX  13	// Unused
`define OR1200_SR_EPH  14
`define OR1200_SR_FO   15


//
// Bits that define offset inside the group

//
// Default Exception Prefix
//
// 1'b0 - OR1200_EXCEPT_EPH0_P (0x0000_0000)
// 1'b1 - OR1200_EXCEPT_EPH1_P (0xF000_0000)
//
`define OR1200_SR_EPH_DEF	1'b0

/////////////////////////////////////////////////////
//
// Power Management (PM)
//
// Bit positions inside PMR (don't change)

`define OR1200_PM_PMR_DME 4
`define OR1200_PM_PMR_SME 5
`define OR1200_PM_PMR_DCGE 6


// PMR offset inside PM group of registers
`define OR1200_PM_OFS_PMR 11'b0

// PM group
`define OR1200_SPRGRP_PM 5'b01000



// Define it if you want PIC implemented


// Define number of interrupt inputs (2-31)
`define OR1200_PIC_INTS 20

// Address offsets of PIC registers inside PIC group
`define OR1200_PIC_OFS_PICMR 2'b00
`define OR1200_PIC_OFS_PICSR 2'b10

// Position of offset bits inside SPR address


// Address offsets of TT registers inside TT group
`define OR1200_TT_OFS_TTMR 1'b0
`define OR1200_TT_OFS_TTCR 1'b1

// Position of offset bits inside SPR group
`define OR1200_TTOFS_BITS 0

// TTMR bits
`define OR1200_TT_TTMR_IP 28
`define OR1200_TT_TTMR_IE 29



//////////////////////////////////////////////
//
// MAC
//
`define OR1200_MAC_ADDR		0	// MACLO 0xxxxxxxx1, MACHI 0xxxxxxxx0
//
// Shift {MACHI,MACLO} into destination register when executing l.macrc
//
// According to architecture manual there is no shift, so default value is 0.
//
// However the implementation has deviated in this from the arch manual and had hard coded shift by 28 bits which
// is a useful optimization for MP3 decoding (if using libmad fixed point library). Shifts are no longer
// default setup, but if you need to remain backward compatible, define your shift bits, which were normally
// dest_GPR = {MACHI,MACLO}[59:28]
`define OR1200_MAC_SHIFTBY	0	// 0 = According to arch manual, 28 = obsolete backward compatibility


//////////////////////////////////////////////
//
// Data MMU (DMMU)
//

//
// Address that selects between TLB TR and MR
//
`define OR1200_DTLB_TM_ADDR	7

//
// DTLBMR fields
//
`define	OR1200_DTLBMR_V_BITS	0
// DTLBTR fields
//
`define	OR1200_DTLBTR_CC_BITS	0
`define	OR1200_DTLBTR_CI_BITS	1
`define	OR1200_DTLBTR_WBC_BITS	2
`define	OR1200_DTLBTR_WOM_BITS	3
`define	OR1200_DTLBTR_A_BITS	4
`define	OR1200_DTLBTR_D_BITS	5
`define	OR1200_DTLBTR_URE_BITS	6
`define	OR1200_DTLBTR_UWE_BITS	7
`define	OR1200_DTLBTR_SRE_BITS	8
`define	OR1200_DTLBTR_SWE_BITS	9
//
// DTLB configuration
//
`define	OR1200_DMMU_PS		13					// 13 for 8KB page size
`define	OR1200_DTLB_INDXW	6							// +5 because of protection bits and CI

//
// Cache inhibit while DMMU is not enabled/implemented
//
// cache inhibited 0GB-4GB		1'b1
// cache inhibited 0GB-2GB		!dcpu_adr_i[31]
// cache inhibited 0GB-1GB 2GB-3GB	!dcpu_adr_i[30]
// cache inhibited 1GB-2GB 3GB-4GB	dcpu_adr_i[30]
// cache inhibited 2GB-4GB (default)	dcpu_adr_i[31]
// cached 0GB-4GB			1'b0
//


//////////////////////////////////////////////
//
// Insn MMU (IMMU)
//

//
// Address that selects between TLB TR and MR
//
`define OR1200_ITLB_TM_ADDR	7

//
// ITLBMR fields
//
`define	OR1200_ITLBMR_V_BITS	0
//
// ITLBTR fields
//
`define	OR1200_ITLBTR_CC_BITS	0
`define	OR1200_ITLBTR_CI_BITS	1
`define	OR1200_ITLBTR_WBC_BITS	2
`define	OR1200_ITLBTR_WOM_BITS	3
`define	OR1200_ITLBTR_A_BITS	4
`define	OR1200_ITLBTR_D_BITS	5
`define	OR1200_ITLBTR_SXE_BITS	6
`define	OR1200_ITLBTR_UXE_BITS	7
//
// ITLB configuration
//
`define	OR1200_IMMU_PS 13					
`define	OR1200_ITLB_INDXW	6			

//
// Cache inhibit while IMMU is not enabled/implemented
// Note: all combinations that use icpu_adr_i cause async loop
//
// cache inhibited 0GB-4GB		1'b1
// cache inhibited 0GB-2GB		!icpu_adr_i[31]
// cache inhibited 0GB-1GB 2GB-3GB	!icpu_adr_i[30]
// cache inhibited 1GB-2GB 3GB-4GB	icpu_adr_i[30]
// cache inhibited 2GB-4GB (default)	icpu_adr_i[31]
// cached 0GB-4GB			1'b0
//
`define OR1200_IMMU_CI			1'b0


/////////////////////////////////////////////////
//
// Insn cache (IC)
//

// 3 for 8 bytes, 4 for 16 bytes etc
`define OR1200_ICLS		4

/////////////////////////////////////////////////
//
// Data cache (DC)
//

// 3 for 8 bytes, 4 for 16 bytes etc
`define OR1200_DCLS		4

// Define to perform store refill (potential performance penalty)
// `define OR1200_DC_STORE_REFILL

//
// DC configurations
`define OR1200_DCSIZE			12			// 4096

`define	OR1200_DCTAG_W			21



/////////////////////////////////////////////////
//
// Store buffer (SB)
//

//
// Store buffer
//
// It will improve performance by "caching" CPU stores
// using store buffer. This is most important for function
// prologues because DC can only work in write though mode
// and all stores would have to complete external WB writes
// to memory.
// Store buffer is between DC and data BIU.
// All stores will be stored into store buffer and immediately
// completed by the CPU, even though actual external writes
// will be performed later. As a consequence store buffer masks
// all data bus errors related to stores (data bus errors
// related to loads are delivered normally).
// All pending CPU loads will wait until store buffer is empty to
// ensure strict memory model. Right now this is necessary because
// we don't make destinction between cached and cache inhibited
// address space, so we simply empty store buffer until loads
// can begin.
//
// It makes design a bit bigger, depending what is the number of
// entries in SB FIFO. Number of entries can be changed further
// down.
//
//`define OR1200_SB_IMPLEMENTED

//
// Number of store buffer entries
//
// Verified number of entries are 4 and 8 entries
// (2 and 3 for OR1200_SB_LOG). OR1200_SB_ENTRIES must
// always match 2**OR1200_SB_LOG.
// To disable store buffer, undefine
// OR1200_SB_IMPLEMENTED.
//
`define OR1200_SB_LOG		2	// 2 or 3
`define OR1200_SB_ENTRIES	4	// 4 or 8


/////////////////////////////////////////////////
//
// Quick Embedded Memory (QMEM)
//

//
// Quick Embedded Memory
//
// Instantiation of dedicated insn/data memory (RAM or ROM).
// Insn fetch has effective throughput 1insn / clock cycle.
// Data load takes two clock cycles / access, data store
// takes 1 clock cycle / access (if there is no insn fetch)).
// Memory instantiation is shared between insn and data,
// meaning if insn fetch are performed, data load/store
// performance will be lower.
//
// Main reason for QMEM is to put some time critical functions
// into this memory and to have predictable and fast access
// to these functions. (soft fpu, context switch, exception
// handlers, stack, etc)
//
// It makes design a bit bigger and slower. QMEM sits behind
// IMMU/DMMU so all addresses are physical (so the MMUs can be
// used with QMEM and QMEM is seen by the CPU just like any other
// memory in the system). IC/DC are sitting behind QMEM so the
// whole design timing might be worse with QMEM implemented.
//
//
// Base address defines first address of QMEM. Mask defines
// QMEM range in address space. Actual size of QMEM is however
// determined with instantiated RAM/ROM. However bigger
// mask will reserve more address space for QMEM, but also
// make design faster, while more tight mask will take
// less address space but also make design slower. If
// instantiated RAM/ROM is smaller than space reserved with
// the mask, instatiated RAM/ROM will also be shadowed
// at higher addresses in reserved space.
//
`define OR1200_QMEM_IADDR	32'h00800000
`define OR1200_QMEM_IMASK	32'hfff00000	// Max QMEM size 1MB
`define OR1200_QMEM_DADDR  32'h00800000
`define OR1200_QMEM_DMASK  32'hfff00000 // Max QMEM size 1MB

//
// QMEM interface byte-select capability
//
// To enable qmem_sel* ports, define this macro.
//
//`define OR1200_QMEM_BSEL

//
// QMEM interface acknowledge
//
// To enable qmem_ack port, define this macro.
//
//`define OR1200_QMEM_ACK

/////////////////////////////////////////////////////
//
// VR, UPR and Configuration Registers
//
//
// VR, UPR and configuration registers are optional. If 
// implemented, operating system can automatically figure
// out how to use the processor because it knows 
// what units are available in the processor and how they
// are configured.
//
// This section must be last in or1200_defines.v file so
// that all units are already configured and thus
// configuration registers are properly set.

// Offsets of VR, UPR and CFGR registers
`define OR1200_SPRGRP_SYS_VR		4'h0
`define OR1200_SPRGRP_SYS_UPR		4'h1
`define OR1200_SPRGRP_SYS_CPUCFGR	4'h2
`define OR1200_SPRGRP_SYS_DMMUCFGR	4'h3
`define OR1200_SPRGRP_SYS_IMMUCFGR	4'h4
`define OR1200_SPRGRP_SYS_DCCFGR	4'h5
`define OR1200_SPRGRP_SYS_ICCFGR	4'h6
`define OR1200_SPRGRP_SYS_DCFGR	4'h7

// VR fields
// VR values
`define OR1200_VR_REV			6'h01
`define OR1200_VR_RES1			10'h000
`define OR1200_VR_CFG			8'h00
`define OR1200_VR_VER			8'h12


// UPR values
`define OR1200_UPR_UP			1'b1
`define OR1200_UPR_DCP			1'b1

`define OR1200_UPR_ICP			1'b1

`define OR1200_UPR_DMP			1'b1

`define OR1200_UPR_IMP			1'b1

`define OR1200_UPR_MP			1'b1	// MAC always present

`define OR1200_UPR_DUP			1'b1


`define OR1200_UPR_PCUP			1'b0	// Performance counters not present

`define OR1200_UPR_PMP			1'b1

`define OR1200_UPR_PICP			1'b1

`define OR1200_UPR_TTP			1'b1

`define OR1200_UPR_RES1			13'h0000
`define OR1200_UPR_CUP			8'h00


`define OR1200_CPUCFGR_HGF_BITS	4
`define OR1200_CPUCFGR_OB32S_BITS	5
`define OR1200_CPUCFGR_OB64S_BITS	6
`define OR1200_CPUCFGR_OF32S_BITS	7
`define OR1200_CPUCFGR_OF64S_BITS	8
`define OR1200_CPUCFGR_OV64S_BITS	9

// CPUCFGR values
`define OR1200_CPUCFGR_NSGF		4'h0
`define OR1200_CPUCFGR_HGF		1'b0
`define OR1200_CPUCFGR_OB32S		1'b1
`define OR1200_CPUCFGR_OB64S		1'b0
`define OR1200_CPUCFGR_OF32S		1'b0
`define OR1200_CPUCFGR_OF64S		1'b0
`define OR1200_CPUCFGR_OV64S		1'b0
`define OR1200_CPUCFGR_RES1		22'h000000

// DMMUCFGR fields
`define OR1200_DMMUCFGR_CRI_BITS	8
`define OR1200_DMMUCFGR_PRI_BITS	9
`define OR1200_DMMUCFGR_TEIRI_BITS	10
`define OR1200_DMMUCFGR_HTR_BITS	11

// DMMUCFGR values
`define OR1200_DMMUCFGR_NTW		2'h0	// 1 TLB way
`define OR1200_DMMUCFGR_NAE		3'h0	// No ATB entries
`define OR1200_DMMUCFGR_CRI		1'b0	// No control register
`define OR1200_DMMUCFGR_PRI		1'b0	// No protection reg
`define OR1200_DMMUCFGR_TEIRI		1'b1	// TLB entry inv reg impl.
`define OR1200_DMMUCFGR_HTR		1'b0	// No HW TLB reload
`define OR1200_DMMUCFGR_RES1		20'h00000


// IMMUCFGR fields
`define OR1200_IMMUCFGR_CRI_BITS	8
`define OR1200_IMMUCFGR_PRI_BITS	9
`define OR1200_IMMUCFGR_TEIRI_BITS	10
`define OR1200_IMMUCFGR_HTR_BITS	11
// IMMUCFGR values
`define OR1200_IMMUCFGR_NTW		2'h0	// 1 TLB way
`define OR1200_IMMUCFGR_NAE		3'h0	// No ATB entry
`define OR1200_IMMUCFGR_CRI		1'b0	// No control reg
`define OR1200_IMMUCFGR_PRI		1'b0	// No protection reg
`define OR1200_IMMUCFGR_TEIRI		1'b1	// TLB entry inv reg impl
`define OR1200_IMMUCFGR_HTR		1'b0	// No HW TLB reload
`define OR1200_IMMUCFGR_RES1		20'h00000


`define OR1200_DCCFGR_CBS_BITS		7
`define OR1200_DCCFGR_CWS_BITS		8
`define OR1200_DCCFGR_CCRI_BITS		9
`define OR1200_DCCFGR_CBIRI_BITS	10
`define OR1200_DCCFGR_CBPRI_BITS	11
`define OR1200_DCCFGR_CBLRI_BITS	12
`define OR1200_DCCFGR_CBFRI_BITS	13
`define OR1200_DCCFGR_CBWBRI_BITS	14

// DCCFGR values
`define OR1200_DCCFGR_NCW		3'h0	// 1 cache way
`define OR1200_DCCFGR_CWS		1'b0	// Write-through strategy
`define OR1200_DCCFGR_CCRI		1'b1	// Cache control reg impl.
`define OR1200_DCCFGR_CBIRI		1'b1	// Cache block inv reg impl.
`define OR1200_DCCFGR_CBPRI		1'b0	// Cache block prefetch reg not impl.
`define OR1200_DCCFGR_CBLRI		1'b0	// Cache block lock reg not impl.
`define OR1200_DCCFGR_CBFRI		1'b1	// Cache block flush reg impl.
`define OR1200_DCCFGR_CBWBRI		1'b0	// Cache block WB reg not impl.
`define OR1200_DCCFGR_RES1		17'h00000


// ICCFGR fields
`define OR1200_ICCFGR_CBS_BITS		7
`define OR1200_ICCFGR_CWS_BITS		8
`define OR1200_ICCFGR_CCRI_BITS		9
`define OR1200_ICCFGR_CBIRI_BITS	10
`define OR1200_ICCFGR_CBPRI_BITS	11
`define OR1200_ICCFGR_CBLRI_BITS	12
`define OR1200_ICCFGR_CBFRI_BITS	13
`define OR1200_ICCFGR_CBWBRI_BITS	14

`define OR1200_ICCFGR_NCW		3'h0	// 1 cache way
`define OR1200_ICCFGR_CWS		1'b0	// Irrelevant
`define OR1200_ICCFGR_CCRI		1'b1	// Cache control reg impl.
`define OR1200_ICCFGR_CBIRI		1'b1	// Cache block inv reg impl.
`define OR1200_ICCFGR_CBPRI		1'b0	// Cache block prefetch reg not impl.
`define OR1200_ICCFGR_CBLRI		1'b0	// Cache block lock reg not impl.
`define OR1200_ICCFGR_CBFRI		1'b1	// Cache block flush reg impl.
`define OR1200_ICCFGR_CBWBRI		1'b0	// Irrelevant
`define OR1200_ICCFGR_RES1		17'h00000


// DCFGR fields
`define OR1200_DCFGR_WPCI_BITS		3

// DCFGR values

`define OR1200_DCFGR_NDP		3'h0	// Zero DVR/DCR pairs
`define OR1200_DCFGR_WPCI		1'b0	// WP counters not impl.

`define OR1200_DCFGR_RES1		28'h0000000
 

module or1200_flat( // or1200_cpu
	// Clk & Rst
	clk, rst,

	// Insn interface
	ic_en,
	icpu_adr_o, icpu_cycstb_o, icpu_sel_o, icpu_tag_o,
	icpu_dat_i, icpu_ack_i, icpu_rty_i, icpu_err_i, icpu_adr_i, icpu_tag_i,
	immu_en,

	// Debug unit
	ex_insn, ex_freeze, id_pc, branch_op,
	spr_dat_npc, rf_dataw,
	du_stall, du_addr, du_dat_du, du_read, du_write, du_dsr, du_hwbkpt,
	du_except, du_dat_cpu,
	
	// Data interface
	dc_en,
	dcpu_adr_o, dcpu_cycstb_o, dcpu_we_o, dcpu_sel_o, dcpu_tag_o, dcpu_dat_o,
	dcpu_dat_i, dcpu_ack_i, dcpu_rty_i, dcpu_err_i, dcpu_tag_i,
	dmmu_en,

	// Interrupt & tick exceptions
	sig_int, sig_tick,

	// SPR interface
	supv, spr_addr, spr_dat_cpu, spr_dat_pic, spr_dat_tt, spr_dat_pm,
	spr_dat_dmmu, spr_dat_immu, spr_dat_du, spr_cs, spr_we
);

//parameter dw = `OR1200_OPERAND_WIDTH;
//parameter aw = `OR1200_REGFILE_ADDR_WIDTH;

//
// I/O ports
//

//
// Clk & Rst
//
input 				clk;
input 				rst;

//
// Insn (IC) interface
//
output				ic_en;
output	[31:0]			icpu_adr_o;
output				icpu_cycstb_o;
output	[3:0]			icpu_sel_o;
output	[3:0]			icpu_tag_o;
input	[31:0]			icpu_dat_i;
input				icpu_ack_i;
input				icpu_rty_i;
input				icpu_err_i;
input	[31:0]			icpu_adr_i;
input	[3:0]			icpu_tag_i;

//
// Insn (IMMU) interface
//
output				immu_en;

//
// Debug interface
//
output	[31:0]			ex_insn;
output				ex_freeze;
output	[31:0]			id_pc;
output	[`OR1200_BRANCHOP_WIDTH-1:0]	branch_op;

input				du_stall;
input	[`OR1200_OPERAND_WIDTH-1:0]		du_addr;
input	[`OR1200_OPERAND_WIDTH-1:0]		du_dat_du;
input				du_read;
input				du_write;
input	[`OR1200_DU_DSR_WIDTH-1:0]	du_dsr;
input				du_hwbkpt;
output	[12:0]			du_except;
output	[`OR1200_OPERAND_WIDTH-1:0]		du_dat_cpu;
output	[`OR1200_OPERAND_WIDTH-1:0]		rf_dataw;

//
// Data (DC) interface
//
output	[31:0]			dcpu_adr_o;
output				dcpu_cycstb_o;
output				dcpu_we_o;
output	[3:0]			dcpu_sel_o;
output	[3:0]			dcpu_tag_o;
output	[31:0]			dcpu_dat_o;
input	[31:0]			dcpu_dat_i;
input				dcpu_ack_i;
input				dcpu_rty_i;
input				dcpu_err_i;
input	[3:0]			dcpu_tag_i;
output				dc_en;

//
// Data (DMMU) interface
//
output				dmmu_en;

//
// SPR interface
//
output				supv;
input	[`OR1200_OPERAND_WIDTH-1:0]		spr_dat_pic;
input	[`OR1200_OPERAND_WIDTH-1:0]		spr_dat_tt;
input	[`OR1200_OPERAND_WIDTH-1:0]		spr_dat_pm;
input	[`OR1200_OPERAND_WIDTH-1:0]		spr_dat_dmmu;
input	[`OR1200_OPERAND_WIDTH-1:0]		spr_dat_immu;
input	[`OR1200_OPERAND_WIDTH-1:0]		spr_dat_du;
output	[`OR1200_OPERAND_WIDTH-1:0]		spr_addr;
output	[`OR1200_OPERAND_WIDTH-1:0]		spr_dat_cpu;
output	[`OR1200_OPERAND_WIDTH-1:0]		spr_dat_npc;
output	[31:0]			spr_cs;
output				spr_we;

//
// Interrupt exceptions
//
input				sig_int;
input				sig_tick;

//
// Internal wires
//
wire	[31:0]			if_insn;
wire	[31:0]			if_pc;
wire	[31:2]			lr_sav;
wire	[`OR1200_REGFILE_ADDR_WIDTH-1:0]		rf_addrw;
wire	[`OR1200_REGFILE_ADDR_WIDTH-1:0] 		rf_addra;
wire	[`OR1200_REGFILE_ADDR_WIDTH-1:0] 		rf_addrb;
wire				rf_rda;
wire				rf_rdb;
wire	[`OR1200_OPERAND_WIDTH-1:0]		simm;
wire	[`OR1200_OPERAND_WIDTH-1:2]		branch_addrofs;
wire	[`OR1200_ALUOP_WIDTH-1:0]	alu_op;
wire	[`OR1200_SHROTOP_WIDTH-1:0]	shrot_op;
wire	[`OR1200_COMPOP_WIDTH-1:0]	comp_op;
wire	[`OR1200_BRANCHOP_WIDTH-1:0]	branch_op;
wire	[`OR1200_LSUOP_WIDTH-1:0]	lsu_op;
wire				genpc_freeze;
wire				if_freeze;
wire				id_freeze;
wire				ex_freeze;
wire				wb_freeze;
wire	[`OR1200_SEL_WIDTH-1:0]	sel_a;
wire	[`OR1200_SEL_WIDTH-1:0]	sel_b;
wire	[`OR1200_RFWBOP_WIDTH-1:0]	rfwb_op;
wire	[`OR1200_OPERAND_WIDTH-1:0]		rf_dataw;
wire	[`OR1200_OPERAND_WIDTH-1:0]		rf_dataa;
wire	[`OR1200_OPERAND_WIDTH-1:0]		rf_datab;
wire	[`OR1200_OPERAND_WIDTH-1:0]		muxed_b;
wire	[`OR1200_OPERAND_WIDTH-1:0]		wb_forw;
wire				wbforw_valid;
wire	[`OR1200_OPERAND_WIDTH-1:0]		operand_a;
wire	[`OR1200_OPERAND_WIDTH-1:0]		operand_b;
wire	[`OR1200_OPERAND_WIDTH-1:0]		alu_dataout;
wire	[`OR1200_OPERAND_WIDTH-1:0]		lsu_dataout;
wire	[`OR1200_OPERAND_WIDTH-1:0]		sprs_dataout;
wire	[31:0]			lsu_addrofs;
wire	[`OR1200_MULTICYCLE_WIDTH-1:0]	multicycle;
wire	[`OR1200_EXCEPT_WIDTH-1:0]	except_type;
wire	[4:0]			cust5_op;
wire	[5:0]			cust5_limm;
wire				flushpipe;
wire				extend_flush;
wire				branch_taken;
wire				flag;
wire				flagforw;
wire				flag_we;
wire				k_carry;
wire				cyforw;
wire				cy_we;
wire				lsu_stall;
wire				epcr_we;
wire				eear_we;
wire				esr_we;
wire				pc_we;
wire	[31:0]			epcr;
wire	[31:0]			eear;
wire	[`OR1200_SR_WIDTH-1:0]	esr;
wire				sr_we;
wire	[`OR1200_SR_WIDTH-1:0]	to_sr;
wire	[`OR1200_SR_WIDTH-1:0]	sr;
wire				except_start;
wire				except_started;
wire	[31:0]			wb_insn;
wire	[15:0]			spr_addrimm;
wire				sig_syscall;
wire				sig_trap;
wire	[31:0]			spr_dat_cfgr;
wire	[31:0]			spr_dat_rf;
wire    [31:0]                  spr_dat_npc;
wire	[31:0]			spr_dat_ppc;
wire	[31:0]			spr_dat_mac;
wire				force_dslot_fetch;
wire				no_more_dslot;
wire				ex_void;
wire				if_stall;
wire				id_macrc_op;
wire				ex_macrc_op;
wire	[`OR1200_MACOP_WIDTH-1:0] mac_op;
wire	[31:0]			mult_mac_result;
wire				mac_stall;
wire	[12:0]			except_stop;
wire				genpc_refetch;
wire				rfe;
wire				lsu_unstall;
wire				except_align;
wire				except_dtlbmiss;
wire				except_dmmufault;
wire				except_illegal;
wire				except_itlbmiss;
wire				except_immufault;
wire				except_ibuserr;
wire				except_dbuserr;
wire				abort_ex;

//
// Send exceptions to Debug Unit
//
assign du_except = except_stop;

//
// Data cache enable
//
assign dc_en = sr[`OR1200_SR_DCE];

//
// Instruction cache enable
//
assign ic_en = sr[`OR1200_SR_ICE];

//
// DMMU enable
//
assign dmmu_en = sr[`OR1200_SR_DME];

//
// IMMU enable
//
assign immu_en = sr[`OR1200_SR_IME];

//
// SUPV bit
//
assign supv = sr[`OR1200_SR_SM];

//
// Instantiation of instruction fetch block
//
or1200_genpc or1200_genpc(
	.clk(clk),
	.rst(rst),
	.icpu_adr_o(icpu_adr_o),
	.icpu_cycstb_o(icpu_cycstb_o),
	.icpu_sel_o(icpu_sel_o),
	.icpu_tag_o(icpu_tag_o),
	.icpu_rty_i(icpu_rty_i),
	.icpu_adr_i(icpu_adr_i),

	.branch_op(branch_op),
	.except_type(except_type),
	.except_start(except_start),
	.except_prefix(sr[`OR1200_SR_EPH]),
	.branch_addrofs(branch_addrofs),
	.lr_restor(operand_b),
	.flag(flag),
	.taken(branch_taken),
	.binsn_addr(lr_sav),
	.epcr(epcr),
	.spr_dat_i(spr_dat_cpu),
	.spr_pc_we(pc_we),
	.genpc_refetch(genpc_refetch),
	.genpc_freeze(genpc_freeze),
  .genpc_stop_prefetch(1'b0),
	.no_more_dslot(no_more_dslot)
);

//
// Instantiation of instruction fetch block
//
or1200_if or1200_if(
	.clk(clk),
	.rst(rst),
	.icpu_dat_i(icpu_dat_i),
	.icpu_ack_i(icpu_ack_i),
	.icpu_err_i(icpu_err_i),
	.icpu_adr_i(icpu_adr_i),
	.icpu_tag_i(icpu_tag_i),

	.if_freeze(if_freeze),
	.if_insn(if_insn),
	.if_pc(if_pc),
	.flushpipe(flushpipe),
	.if_stall(if_stall),
	.no_more_dslot(no_more_dslot),
	.genpc_refetch(genpc_refetch),
	.rfe(rfe),
	.except_itlbmiss(except_itlbmiss),
	.except_immufault(except_immufault),
	.except_ibuserr(except_ibuserr)
);

//
// Instantiation of instruction decode/control logic
//
or1200_ctrl or1200_ctrl(
	.clk(clk),
	.rst(rst),
	.id_freeze(id_freeze),
	.ex_freeze(ex_freeze),
	.wb_freeze(wb_freeze),
	.flushpipe(flushpipe),
	.if_insn(if_insn),
	.ex_insn(ex_insn),
	.branch_op(branch_op),
	.branch_taken(branch_taken),
	.rf_addra(rf_addra),
	.rf_addrb(rf_addrb),
	.rf_rda(rf_rda),
	.rf_rdb(rf_rdb),
	.alu_op(alu_op),
	.mac_op(mac_op),
	.shrot_op(shrot_op),
	.comp_op(comp_op),
	.rf_addrw(rf_addrw),
	.rfwb_op(rfwb_op),
	.wb_insn(wb_insn),
	.simm(simm),
	.branch_addrofs(branch_addrofs),
	.lsu_addrofs(lsu_addrofs),
	.sel_a(sel_a),
	.sel_b(sel_b),
	.lsu_op(lsu_op),
	.cust5_op(cust5_op),
	.cust5_limm(cust5_limm),
	.multicycle(multicycle),
	.spr_addrimm(spr_addrimm),
	.wbforw_valid(wbforw_valid),
	.sig_syscall(sig_syscall),
	.sig_trap(sig_trap),
	.force_dslot_fetch(force_dslot_fetch),
	.no_more_dslot(no_more_dslot),
	.ex_void(ex_void),
	.id_macrc_op(id_macrc_op),
	.ex_macrc_op(ex_macrc_op),
	.rfe(rfe),
	.du_hwbkpt(du_hwbkpt),
	.except_illegal(except_illegal)
);

//
// Instantiation of register file
//
or1200_rf or1200_rf(
	.clk(clk),
	.rst(rst),
	.supv(sr[`OR1200_SR_SM]),
	.wb_freeze(wb_freeze),
	.addrw(rf_addrw),
	.dataw(rf_dataw),
	.id_freeze(id_freeze),
	.we(rfwb_op[0]),
	.flushpipe(flushpipe),
	.addra(rf_addra),
	.rda(rf_rda),
	.dataa(rf_dataa),
	.addrb(rf_addrb),
	.rdb(rf_rdb),
	.datab(rf_datab),
	.spr_cs(spr_cs[`OR1200_SPR_GROUP_SYS]),
	.spr_write(spr_we),
	.spr_addr(spr_addr),
	.spr_dat_i(spr_dat_cpu),
	.spr_dat_o(spr_dat_rf)
);

//
// Instantiation of operand muxes
//
or1200_operandmuxes or1200_operandmuxes(
	.clk(clk),
	.rst(rst),
	.id_freeze(id_freeze),
	.ex_freeze(ex_freeze),
	.rf_dataa(rf_dataa),
	.rf_datab(rf_datab),
	.ex_forw(rf_dataw),
	.wb_forw(wb_forw),
	.simm(simm),
	.sel_a(sel_a),
	.sel_b(sel_b),
	.operand_a(operand_a),
	.operand_b(operand_b),
	.muxed_b(muxed_b)
);

//
// Instantiation of CPU's ALU
//
or1200_alu or1200_alu(
	.a(operand_a),
	.b(operand_b),
	.mult_mac_result(mult_mac_result),
	.macrc_op(ex_macrc_op),
	.alu_op(alu_op),
	.shrot_op(shrot_op),
	.comp_op(comp_op),
	.cust5_op(cust5_op),
	.cust5_limm(cust5_limm),
	.result(alu_dataout),
	.flagforw(flagforw),
	.flag_we(flag_we),
	.cyforw(cyforw),
	.cy_we(cy_we),
  .flag(flag),
	.k_carry(k_carry)
);

//
// Instantiation of CPU's ALU
//
or1200_mult_mac or1200_mult_mac(
	.clk(clk),
	.rst(rst),
	.ex_freeze(ex_freeze),
	.id_macrc_op(id_macrc_op),
	.macrc_op(ex_macrc_op),
	.a(operand_a),
	.b(operand_b),
	.mac_op(mac_op),
	.alu_op(alu_op),
	.result(mult_mac_result),
	.mac_stall_r(mac_stall),
	.spr_cs(spr_cs[`OR1200_SPR_GROUP_MAC]),
	.spr_write(spr_we),
	.spr_addr(spr_addr),
	.spr_dat_i(spr_dat_cpu),
	.spr_dat_o(spr_dat_mac)
);

//
// Instantiation of CPU's SPRS block
//
or1200_sprs or1200_sprs(
	.clk(clk),
	.rst(rst),
	.addrbase(operand_a),
	.addrofs(spr_addrimm),
	.dat_i(operand_b),
	.alu_op(alu_op),
	.flagforw(flagforw),
	.flag_we(flag_we),
	.flag(flag),
	.cyforw(cyforw),
	.cy_we(cy_we),
	.carry(k_carry),
	.to_wbmux(sprs_dataout),

	.du_addr(du_addr),
	.du_dat_du(du_dat_du),
	.du_read(du_read),
	.du_write(du_write),
	.du_dat_cpu(du_dat_cpu),

	.spr_addr(spr_addr),
	.spr_dat_pic(spr_dat_pic),
	.spr_dat_tt(spr_dat_tt),
	.spr_dat_pm(spr_dat_pm),
	.spr_dat_cfgr(spr_dat_cfgr),
	.spr_dat_rf(spr_dat_rf),
	.spr_dat_npc(spr_dat_npc),
        .spr_dat_ppc(spr_dat_ppc),
	.spr_dat_mac(spr_dat_mac),
	.spr_dat_dmmu(spr_dat_dmmu),
	.spr_dat_immu(spr_dat_immu),
	.spr_dat_du(spr_dat_du),
	.spr_dat_o(spr_dat_cpu),
	.spr_cs(spr_cs),
	.spr_we(spr_we),

	.epcr_we(epcr_we),
	.eear_we(eear_we),
	.esr_we(esr_we),
	.pc_we(pc_we),
	.epcr(epcr),
	.eear(eear),
	.esr(esr),
	.except_started(except_started),

	.sr_we(sr_we),
	.to_sr(to_sr),
	.sr(sr),
	.branch_op(branch_op)
);

//
// Instantiation of load/store unit
//
or1200_lsu or1200_lsu(
	.addrbase(operand_a),
	.addrofs(lsu_addrofs),
	.lsu_op(lsu_op),
	.lsu_datain(operand_b),
	.lsu_dataout(lsu_dataout),
	.lsu_stall(lsu_stall),
	.lsu_unstall(lsu_unstall),
        .du_stall(du_stall),
	.except_align(except_align),
	.except_dtlbmiss(except_dtlbmiss),
	.except_dmmufault(except_dmmufault),
	.except_dbuserr(except_dbuserr),

	.dcpu_adr_o(dcpu_adr_o),
	.dcpu_cycstb_o(dcpu_cycstb_o),
	.dcpu_we_o(dcpu_we_o),
	.dcpu_sel_o(dcpu_sel_o),
	.dcpu_tag_o(dcpu_tag_o),
	.dcpu_dat_o(dcpu_dat_o),
	.dcpu_dat_i(dcpu_dat_i),
	.dcpu_ack_i(dcpu_ack_i),
	.dcpu_rty_i(dcpu_rty_i),
	.dcpu_err_i(dcpu_err_i),
	.dcpu_tag_i(dcpu_tag_i)
);

//
// Instantiation of write-back muxes
//
or1200_wbmux or1200_wbmux(
	.clk(clk),
	.rst(rst),
	.wb_freeze(wb_freeze),
	.rfwb_op(rfwb_op),
	.muxin_a(alu_dataout),
	.muxin_b(lsu_dataout),
	.muxin_c(sprs_dataout),
	.muxin_d({lr_sav, 2'b0}),
	.muxout(rf_dataw),
	.muxreg(wb_forw),
	.muxreg_valid(wbforw_valid)
);

//
// Instantiation of freeze logic
//
or1200_freeze or1200_freeze(
	.clk(clk),
	.rst(rst),
	.multicycle(multicycle),
	.flushpipe(flushpipe),
	.extend_flush(extend_flush),
	.lsu_stall(lsu_stall),
	.if_stall(if_stall),
	.lsu_unstall(lsu_unstall),
	.force_dslot_fetch(force_dslot_fetch),
	.abort_ex(abort_ex),
	.du_stall(du_stall),
	.mac_stall(mac_stall),
	.genpc_freeze(genpc_freeze),
	.if_freeze(if_freeze),
	.id_freeze(id_freeze),
	.ex_freeze(ex_freeze),
	.wb_freeze(wb_freeze),
	.icpu_ack_i(icpu_ack_i),
	.icpu_err_i(icpu_err_i)
);

//
// Instantiation of exception block
//
or1200_except or1200_except(
	.clk(clk),
	.rst(rst),
	.sig_ibuserr(except_ibuserr),
	.sig_dbuserr(except_dbuserr),
	.sig_illegal(except_illegal),
	.sig_align(except_align),
	.sig_range(1'b0),
	.sig_dtlbmiss(except_dtlbmiss),
	.sig_dmmufault(except_dmmufault),
	.sig_int(sig_int),
	.sig_syscall(sig_syscall),
	.sig_trap(sig_trap),
	.sig_itlbmiss(except_itlbmiss),
	.sig_immufault(except_immufault),
	.sig_tick(sig_tick),
	.branch_taken(branch_taken),
	.icpu_ack_i(icpu_ack_i),
	.icpu_err_i(icpu_err_i),
	.dcpu_ack_i(dcpu_ack_i),
	.dcpu_err_i(dcpu_err_i),
	.genpc_freeze(genpc_freeze),
        .id_freeze(id_freeze),
        .ex_freeze(ex_freeze),
        .wb_freeze(wb_freeze),
	.if_stall(if_stall),
	.if_pc(if_pc),
	.id_pc(id_pc),
	.lr_sav(lr_sav),
	.flushpipe(flushpipe),
	.extend_flush(extend_flush),
	.except_type(except_type),
	.except_start(except_start),
	.except_started(except_started),
	.except_stop(except_stop),
	.ex_void(ex_void),
	.spr_dat_ppc(spr_dat_ppc),
	.spr_dat_npc(spr_dat_npc),

	.datain(operand_b),
	.du_dsr(du_dsr),
	.epcr_we(epcr_we),
	.eear_we(eear_we),
	.esr_we(esr_we),
	.pc_we(pc_we),
        .epcr(epcr),
	.eear(eear),
	.esr(esr),

	.lsu_addr(dcpu_adr_o),
	.sr_we(sr_we),
	.to_sr(to_sr),
	.sr(sr),
	.abort_ex(abort_ex)
);

//
// Instantiation of configuration registers
//
or1200_cfgr or1200_cfgr(
	.spr_addr(spr_addr),
	.spr_dat_o(spr_dat_cfgr)
);

endmodule



`define OR1200_ITAG_IDLE	4'h0	// idle bus
`define	OR1200_ITAG_NI		4'h1	// normal insn
`define OR1200_ITAG_BE		4'hb	// Bus error exception
`define OR1200_ITAG_PE		4'hc	// Page fault exception
`define OR1200_ITAG_TE		4'hd	// TLB miss exception
`define OR1200_BRANCHOP_WIDTH		3
`define OR1200_BRANCHOP_NOP		3'b000
`define OR1200_BRANCHOP_J		3'b001
`define OR1200_BRANCHOP_JR		3'b010
`define OR1200_BRANCHOP_BAL		3'b011
`define OR1200_BRANCHOP_BF		3'b100
`define OR1200_BRANCHOP_BNF		3'b101
`define OR1200_BRANCHOP_RFE		3'b110
`define OR1200_EXCEPT_WIDTH 4
`define OR1200_EXCEPT_EPH0_P 20'h00000
`define OR1200_EXCEPT_EPH1_P 20'hF0000
`define OR1200_EXCEPT_V		   8'h00

module or1200_genpc(
	// Clock and reset
	clk, rst,

	// External i/f to IC
	icpu_adr_o, icpu_cycstb_o, icpu_sel_o, icpu_tag_o,
	icpu_rty_i, icpu_adr_i,

	// Internal i/f
	branch_op, except_type,except_start, except_prefix, 
	branch_addrofs, lr_restor, flag, taken, 
	binsn_addr, epcr, spr_dat_i, spr_pc_we, genpc_refetch,
	genpc_freeze, genpc_stop_prefetch, no_more_dslot
);

//
// I/O
//

//
// Clock and reset
//
input				clk;
input				rst;

//
// External i/f to IC
//
output	[31:0]			icpu_adr_o;
output				icpu_cycstb_o;
output	[3:0]			icpu_sel_o;
output	[3:0]			icpu_tag_o;
input				icpu_rty_i;
input	[31:0]			icpu_adr_i;

//
// Internal i/f
//
input	[`OR1200_BRANCHOP_WIDTH-1:0]	branch_op;
input	[`OR1200_EXCEPT_WIDTH-1:0]	except_type;
input				except_start;
input					except_prefix;
input	[31:2]			branch_addrofs;
input	[31:0]			lr_restor;
input				flag;
output				taken;

input	[31:2]			binsn_addr;
input	[31:0]			epcr;
input	[31:0]			spr_dat_i;
input				spr_pc_we;
input				genpc_refetch;
input				genpc_freeze;
input				genpc_stop_prefetch;
input				no_more_dslot;

//
// Internal wires and regs
//
reg	[31:2]			pcreg;
reg	[31:0]			pc;
reg				taken;	/* Set to in case of jump or taken branch */
reg				genpc_refetch_r;

//
// Address of insn to be fecthed
//
assign icpu_adr_o = !no_more_dslot & !except_start & !spr_pc_we & (icpu_rty_i | genpc_refetch) ? icpu_adr_i : pc;
// assign icpu_adr_o = !except_start & !spr_pc_we & (icpu_rty_i | genpc_refetch) ? icpu_adr_i : pc;

//
// Control access to IC subsystem
//
// assign icpu_cycstb_o = !genpc_freeze & !no_more_dslot;
assign icpu_cycstb_o = !genpc_freeze; // works, except remaining raised cycstb during long load/store
//assign icpu_cycstb_o = !(genpc_freeze | genpc_refetch & genpc_refetch_r);
//assign icpu_cycstb_o = !(genpc_freeze | genpc_stop_prefetch);
assign icpu_sel_o = 4'b1111;
assign icpu_tag_o = `OR1200_ITAG_NI;

//
// genpc_freeze_r
//
always @(posedge clk )
	if (rst)
		genpc_refetch_r <= 1'b0;
	else if (genpc_refetch)
		genpc_refetch_r <=  1'b1;
	else
		genpc_refetch_r <=  1'b0;

//
// Async calculation of new PC value. This value is used for addressing the IC.
//
always @(pcreg or branch_addrofs or binsn_addr or flag or branch_op or except_type
	or except_start or lr_restor or epcr or spr_pc_we or spr_dat_i or except_prefix) begin
	case ({spr_pc_we, except_start, branch_op})	// synopsys parallel_case
		{2'b00, `OR1200_BRANCHOP_NOP}: begin
			pc = {pcreg + 30'b000000000000000000000000000001, 2'b0};
			taken = 1'b0;
		end
		{2'b00, `OR1200_BRANCHOP_J}: begin

			pc = {branch_addrofs, 2'b0};
			taken = 1'b1;
		end
		{2'b00, `OR1200_BRANCHOP_JR}: begin

			pc = lr_restor;
			taken = 1'b1;
		end
		{2'b00, `OR1200_BRANCHOP_BAL}: begin
	pc = {binsn_addr + branch_addrofs, 2'b0};
			taken = 1'b1;
		end
		{2'b00, `OR1200_BRANCHOP_BF}:
			if (flag) begin

				pc = {binsn_addr + branch_addrofs, 2'b0};
				taken = 1'b1;
			end
			else begin

				pc = {pcreg + 30'b000000000000000000000000000001, 2'b0};
				taken = 1'b0;
			end
		{2'b00, `OR1200_BRANCHOP_BNF}:
			if (flag) begin
				pc = {pcreg + 30'b000000000000000000000000000001, 2'b0};

				taken = 1'b0;
			end
			else begin				pc = {binsn_addr + branch_addrofs, 2'b0};
				taken = 1'b1;
			end
		{2'b00, `OR1200_BRANCHOP_RFE}: begin

			pc = epcr;
			taken = 1'b1;
		end
		{2'b01, 3'b000}: begin
			pc = {(except_prefix ? `OR1200_EXCEPT_EPH1_P : `OR1200_EXCEPT_EPH0_P), except_type, `OR1200_EXCEPT_V};
			taken = 1'b1;
		end
		{2'b01, 3'b001}: begin

                        pc = {(except_prefix ? `OR1200_EXCEPT_EPH1_P : `OR1200_EXCEPT_EPH0_P), except_type, `OR1200_EXCEPT_V};
                        taken = 1'b1;
                end
		{2'b01, 3'b010}: begin

                        pc = {(except_prefix ? `OR1200_EXCEPT_EPH1_P : `OR1200_EXCEPT_EPH0_P), except_type, `OR1200_EXCEPT_V};
                        taken = 1'b1;
                end
		{2'b01, 3'b011}: begin

                        pc = {(except_prefix ? `OR1200_EXCEPT_EPH1_P : `OR1200_EXCEPT_EPH0_P), except_type, `OR1200_EXCEPT_V};
                        taken = 1'b1;
                end
		{2'b01, 3'b100}: begin

                        pc = {(except_prefix ? `OR1200_EXCEPT_EPH1_P : `OR1200_EXCEPT_EPH0_P), except_type, `OR1200_EXCEPT_V};
                        taken = 1'b1;
                end
		{2'b01, 3'b101}: begin

                        pc = {(except_prefix ? `OR1200_EXCEPT_EPH1_P : `OR1200_EXCEPT_EPH0_P), except_type, `OR1200_EXCEPT_V};
                        taken = 1'b1;
                end
		{2'b01, 3'b110}: begin

                        pc = {(except_prefix ? `OR1200_EXCEPT_EPH1_P : `OR1200_EXCEPT_EPH0_P), except_type, `OR1200_EXCEPT_V};
                        taken = 1'b1;
                end
		{2'b01, 3'b111}: begin

                        pc = {(except_prefix ? `OR1200_EXCEPT_EPH1_P : `OR1200_EXCEPT_EPH0_P), except_type, `OR1200_EXCEPT_V};
                        taken = 1'b1;
                end
		default: begin
		
			pc = spr_dat_i;
			taken = 1'b0;
		end
	endcase
end

//
// PC register
//
always @(posedge clk )
	if (rst)
//		pcreg <=  30'd63;
		pcreg <=  ({(except_prefix ? `OR1200_EXCEPT_EPH1_P : `OR1200_EXCEPT_EPH0_P),8'b11111111, `OR1200_EXCEPT_V} - 1) >> 2;
	else if (spr_pc_we)
		pcreg <=  spr_dat_i[31:2];
	else if (no_more_dslot | except_start | !genpc_freeze & !icpu_rty_i & !genpc_refetch)
//	else if (except_start | !genpc_freeze & !icpu_rty_i & !genpc_refetch)
		pcreg <=  pc[31:2];

		wire unused;
		assign unused = |except_prefix & | binsn_addr | genpc_stop_prefetch ;
endmodule

`define OR1200_ITAG_IDLE	4'h0	// idle bus
`define	OR1200_ITAG_NI		4'h1	// normal insn
`define OR1200_ITAG_BE		4'hb	// Bus error exception
`define OR1200_ITAG_PE		4'hc	// Page fault exception
`define OR1200_ITAG_TE		4'hd	// TLB miss exception

`define OR1200_OR32_J                 6'b000000
`define OR1200_OR32_JAL               6'b000001
`define OR1200_OR32_BNF               6'b000011
`define OR1200_OR32_BF                6'b000100
`define OR1200_OR32_NOP               6'b000101
`define OR1200_OR32_MOVHI             6'b000110
`define OR1200_OR32_XSYNC             6'b001000
`define OR1200_OR32_RFE               6'b001001
/* */
`define OR1200_OR32_JR                6'b010001
`define OR1200_OR32_JALR              6'b010010
`define OR1200_OR32_MACI              6'b010011
/* */
`define OR1200_OR32_LWZ               6'b100001
`define OR1200_OR32_LBZ               6'b100011
`define OR1200_OR32_LBS               6'b100100
`define OR1200_OR32_LHZ               6'b100101
`define OR1200_OR32_LHS               6'b100110
`define OR1200_OR32_ADDI              6'b100111
`define OR1200_OR32_ADDIC             6'b101000
`define OR1200_OR32_ANDI              6'b101001
`define OR1200_OR32_ORI               6'b101010
`define OR1200_OR32_XORI              6'b101011
`define OR1200_OR32_MULI              6'b101100
`define OR1200_OR32_MFSPR             6'b101101
`define OR1200_OR32_SH_ROTI 	      6'b101110
`define OR1200_OR32_SFXXI             6'b101111
/* */
`define OR1200_OR32_MTSPR             6'b110000
`define OR1200_OR32_MACMSB            6'b110001
/* */
`define OR1200_OR32_SW                6'b110101
`define OR1200_OR32_SB                6'b110110
`define OR1200_OR32_SH                6'b110111
`define OR1200_OR32_ALU               6'b111000
`define OR1200_OR32_SFXX              6'b111001
//`define OR1200_OR32_CUST5             6'b111100


module or1200_if(
	// Clock and reset
	clk, rst,

	// External i/f to IC
	icpu_dat_i, icpu_ack_i, icpu_err_i, icpu_adr_i, icpu_tag_i,

	// Internal i/f
	if_freeze, if_insn, if_pc, flushpipe,
	if_stall, no_more_dslot, genpc_refetch, rfe,
	except_itlbmiss, except_immufault, except_ibuserr
);

//
// I/O
//

//
// Clock and reset
//
input				clk;
input				rst;

//
// External i/f to IC
//
input	[31:0]			icpu_dat_i;
input				icpu_ack_i;
input				icpu_err_i;
input	[31:0]			icpu_adr_i;
input	[3:0]			icpu_tag_i;

//
// Internal i/f
//
input				if_freeze;
output	[31:0]			if_insn;
output	[31:0]			if_pc;
input				flushpipe;
output				if_stall;
input				no_more_dslot;
output				genpc_refetch;
input				rfe;
output				except_itlbmiss;
output				except_immufault;
output				except_ibuserr;

//
// Internal wires and regs
//
reg	[31:0]			insn_saved;
reg	[31:0]			addr_saved;
reg				saved;

//
// IF stage insn
//
assign if_insn = icpu_err_i | no_more_dslot | rfe ? {`OR1200_OR32_NOP, 26'h0410000} : saved ? insn_saved : icpu_ack_i ? icpu_dat_i : {`OR1200_OR32_NOP, 26'h0610000};
assign if_pc = saved ? addr_saved : icpu_adr_i;
// assign if_stall = !icpu_err_i & !icpu_ack_i & !saved & !no_more_dslot;
assign if_stall = !icpu_err_i & !icpu_ack_i & !saved;
assign genpc_refetch = saved & icpu_ack_i;
assign except_itlbmiss = icpu_err_i & (icpu_tag_i == `OR1200_ITAG_TE) & !no_more_dslot;
assign except_immufault = icpu_err_i & (icpu_tag_i == `OR1200_ITAG_PE) & !no_more_dslot;
assign except_ibuserr = icpu_err_i & (icpu_tag_i == `OR1200_ITAG_BE) & !no_more_dslot;

//
// Flag for saved insn/address
//
always @(posedge clk )
	if (rst)
		saved <=  1'b0;
	else if (flushpipe)
		saved <=  1'b0;
	else if (icpu_ack_i & if_freeze & !saved)
		saved <=  1'b1;
	else if (!if_freeze)
		saved <=  1'b0;

//
// Store fetched instruction
//
always @(posedge clk )
	if (rst)
		insn_saved <=  {`OR1200_OR32_NOP, 26'h0410000};
	else if (flushpipe)
		insn_saved <=  {`OR1200_OR32_NOP, 26'h0410000};
	else if (icpu_ack_i & if_freeze & !saved)
		insn_saved <=  icpu_dat_i;
	else if (!if_freeze)
		insn_saved <=  {`OR1200_OR32_NOP, 26'h0410000};

//
// Store fetched instruction's address
//
always @(posedge clk )
	if (rst)
		addr_saved <=  32'h00000000;
	else if (flushpipe)
		addr_saved <=  32'h00000000;
	else if (icpu_ack_i & if_freeze & !saved)
		addr_saved <=  icpu_adr_i;
	else if (!if_freeze)
		addr_saved <=  icpu_adr_i;

endmodule

//////////////////////////////////////////////////////////////////////
////                                                              ////
////  OR1200's Instruction decode                                 ////
////                                                              ////
////  This file is part of the OpenRISC 1200 project              ////
////  http://www.opencores.org/cores/or1k/                        ////
////                                                              ////
////  Description                                                 ////
////  Majority of instruction decoding is performed here.         ////
////                                                              ////
////  To Do:                                                      ////
////   - make it smaller and faster                               ////
////                                                              ////
////  Author(s):                                                  ////
////      - Damjan Lampret, lampret@opencores.org                 ////
////                                                              ////
//////////////////////////////////////////////////////////////////////
////                                                              ////
//// Copyright (C) 2000 Authors and OPENCORES.ORG                 ////
////                                                              ////
//// This source file may be used and distributed without         ////
//// restriction provided that this copyright statement is not    ////
//// removed from the file and that any derivative work contains  ////
//// the original copyright notice and the associated disclaimer. ////
////                                                              ////
//// This source file is free software; you can redistribute it   ////
//// and/or modify it under the terms of the GNU Lesser General   ////
//// Public License as published by the Free Software Foundation; ////
//// either version 2.1 of the License, or (at your option) any   ////
//// later version.                                               ////
////                                                              ////
//// This source is distributed in the hope that it will be       ////
//// useful, but WITHOUT ANY WARRANTY; without even the implied   ////
//// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      ////
//// PURPOSE.  See the GNU Lesser General Public License for more ////
//// details.                                                     ////
////                                                              ////
//// You should have received a copy of the GNU Lesser General    ////
//// Public License along with this source; if not, download it   ////
//// from http://www.opencores.org/lgpl.shtml                     ////
////                                                              ////
//////////////////////////////////////////////////////////////////////
//
// CVS Revision History
//
// $Log: not supported by cvs2svn $
// Revision 1.12  2005/01/07 09:31:07  andreje
// sign/zero extension for l.sfxxi instructions corrected
//
// Revision 1.11  2004/06/08 18:17:36  lampret
// Non-functional changes. Coding style fixes.
//
// Revision 1.10  2004/05/09 19:49:04  lampret
// Added some l.cust5 custom instructions as example
//
// Revision 1.9  2004/04/05 08:29:57  lampret
// Merged branch_qmem into main tree.
//
// Revision 1.8.4.1  2004/02/11 01:40:11  lampret
// preliminary HW breakpoints support in debug unit (by default disabled). To enable define OR1200_DU_HWBKPTS.
//
// Revision 1.8  2003/04/24 00:16:07  lampret
// No functional changes. Added defines to disable implementation of multiplier/MAC
//
// Revision 1.7  2002/09/07 05:42:02  lampret
// Added optional SR[CY]. Added define to enable additional (compare) flag modifiers. Defines are OR1200_IMPL_ADDC and OR1200_ADDITIONAL_FLAG_MODIFIERS.
//
// Revision 1.6  2002/03/29 15:16:54  lampret
// Some of the warnings fixed.
//
// Revision 1.5  2002/02/01 19:56:54  lampret
// Fixed combinational loops.
//
// Revision 1.4  2002/01/28 01:15:59  lampret
// Changed 'void' nop-ops instead of insn[0] to use insn[16]. Debug unit stalls the tick timer. Prepared new flag generation for add and and insns. Blocked DC/IC while they are turned off. Fixed I/D MMU SPRs layout except WAYs. TODO: smart IC invalidate, l.j 2 and TLB ways.
//
// Revision 1.3  2002/01/18 14:21:43  lampret
// Fixed 'the NPC single-step fix'.
//
// Revision 1.2  2002/01/14 06:18:22  lampret
// Fixed mem2reg bug in FAST implementation. Updated debug unit to work with new genpc/if.
//
// Revision 1.1  2002/01/03 08:16:15  lampret
// New prefixes for RTL files, prefixed module names. Updated cache controllers and MMUs.
//
// Revision 1.14  2001/11/30 18:59:17  simons
// force_dslot_fetch does not work -  allways zero.
//
// Revision 1.13  2001/11/20 18:46:15  simons
// Break point bug fixed
//
// Revision 1.12  2001/11/18 08:36:28  lampret
// For GDB changed single stepping and disabled trap exception.
//
// Revision 1.11  2001/11/13 10:02:21  lampret
// Added 'setpc'. Renamed some signals (except_flushpipe into flushpipe etc)
//
// Revision 1.10  2001/11/12 01:45:40  lampret
// Moved flag bit into SR. Changed RF enable from constant enable to dynamic enable for read ports.
//
// Revision 1.9  2001/11/10 03:43:57  lampret
// Fixed exceptions.
//
// Revision 1.8  2001/10/21 17:57:16  lampret
// Removed params from generic_XX.v. Added translate_off/on in sprs.v and id.v. Removed spr_addr from dc.v and ic.v. Fixed CR+LF.
//
// Revision 1.7  2001/10/14 13:12:09  lampret
// MP3 version.
//
// Revision 1.1.1.1  2001/10/06 10:18:36  igorm
// no message
//
// Revision 1.2  2001/08/13 03:36:20  lampret
// Added cfg regs. Moved all defines into one defines.v file. More cleanup.
//
// Revision 1.1  2001/08/09 13:39:33  lampret
// Major clean-up.
//
//


module or1200_ctrl(
	// Clock and reset
	clk, rst,

	// Internal i/f
	id_freeze, ex_freeze, wb_freeze, flushpipe, if_insn, ex_insn, branch_op, branch_taken,
	rf_addra, rf_addrb, rf_rda, rf_rdb, alu_op, mac_op, shrot_op, comp_op, rf_addrw, rfwb_op,
	wb_insn, simm, branch_addrofs, lsu_addrofs, sel_a, sel_b, lsu_op,
	cust5_op, cust5_limm,
	multicycle, spr_addrimm, wbforw_valid, sig_syscall, sig_trap,
	force_dslot_fetch, no_more_dslot, ex_void, id_macrc_op, ex_macrc_op, rfe,du_hwbkpt, except_illegal
);

//
// I/O
//
input					clk;
input					rst;
input					id_freeze;
input					ex_freeze;
input					wb_freeze;
input					flushpipe;
input	[31:0]				if_insn;
output	[31:0]				ex_insn;
output	[`OR1200_BRANCHOP_WIDTH-1:0]		branch_op;
input						branch_taken;
output	[`OR1200_REGFILE_ADDR_WIDTH-1:0]	rf_addrw;
output	[`OR1200_REGFILE_ADDR_WIDTH-1:0]	rf_addra;
output	[`OR1200_REGFILE_ADDR_WIDTH-1:0]	rf_addrb;
output					rf_rda;
output					rf_rdb;
output	[`OR1200_ALUOP_WIDTH-1:0]		alu_op;
output	[`OR1200_MACOP_WIDTH-1:0]		mac_op;
output	[`OR1200_SHROTOP_WIDTH-1:0]		shrot_op;
output	[`OR1200_RFWBOP_WIDTH-1:0]		rfwb_op;
output	[31:0]				wb_insn;
output	[31:0]				simm;
output	[31:2]				branch_addrofs;
output	[31:0]				lsu_addrofs;
output	[`OR1200_SEL_WIDTH-1:0]		sel_a;
output	[`OR1200_SEL_WIDTH-1:0]		sel_b;
output	[`OR1200_LSUOP_WIDTH-1:0]		lsu_op;
output	[`OR1200_COMPOP_WIDTH-1:0]		comp_op;
output	[`OR1200_MULTICYCLE_WIDTH-1:0]		multicycle;
output	[4:0]				cust5_op;
output	[5:0]				cust5_limm;
output	[15:0]				spr_addrimm;
input					wbforw_valid;
input					du_hwbkpt;
output					sig_syscall;
output					sig_trap;
output					force_dslot_fetch;
output					no_more_dslot;
output					ex_void;
output					id_macrc_op;
output					ex_macrc_op;
output					rfe;
output					except_illegal;

//
// Internal wires and regs
//
reg	[`OR1200_BRANCHOP_WIDTH-1:0]		pre_branch_op;
reg	[`OR1200_BRANCHOP_WIDTH-1:0]		branch_op;
reg	[`OR1200_ALUOP_WIDTH-1:0]		alu_op;

reg	[`OR1200_MACOP_WIDTH-1:0]		mac_op;
reg					ex_macrc_op;

reg	[`OR1200_SHROTOP_WIDTH-1:0]		shrot_op;
reg	[31:0]				id_insn;
reg	[31:0]				ex_insn;
reg	[31:0]				wb_insn;
reg	[`OR1200_REGFILE_ADDR_WIDTH-1:0]	rf_addrw;
reg	[`OR1200_REGFILE_ADDR_WIDTH-1:0]	wb_rfaddrw;
reg	[`OR1200_RFWBOP_WIDTH-1:0]		rfwb_op;
reg	[31:0]				lsu_addrofs;
reg	[`OR1200_SEL_WIDTH-1:0]		sel_a;
reg	[`OR1200_SEL_WIDTH-1:0]		sel_b;
reg					sel_imm;
reg	[`OR1200_LSUOP_WIDTH-1:0]		lsu_op;
reg	[`OR1200_COMPOP_WIDTH-1:0]		comp_op;
reg	[`OR1200_MULTICYCLE_WIDTH-1:0]		multicycle;
reg					imm_signextend;
reg	[15:0]				spr_addrimm;
reg					sig_syscall;
reg					sig_trap;
reg					except_illegal;
wire					id_void;

//
// Register file read addresses
//
assign rf_addra = if_insn[20:16];
assign rf_addrb = if_insn[15:11];
assign rf_rda = if_insn[31];
assign rf_rdb = if_insn[30];

//
// Force fetch of delay slot instruction when jump/branch is preceeded by load/store
// instructions
//
// SIMON
// assign force_dslot_fetch = ((|pre_branch_op) & (|lsu_op));
assign force_dslot_fetch = 1'b0;
assign no_more_dslot = |branch_op & !id_void & branch_taken | (branch_op == `OR1200_BRANCHOP_RFE);
assign id_void = (id_insn[31:26] == `OR1200_OR32_NOP) & id_insn[16];
assign ex_void = (ex_insn[31:26] == `OR1200_OR32_NOP) & ex_insn[16];

//
// Sign/Zero extension of immediates
//
assign simm = (imm_signextend == 1'b1) ? {{id_insn[15]},{id_insn[15]},{id_insn[15]},{id_insn[15]},{id_insn[15]},{id_insn[15]},{id_insn[15]},{id_insn[15]},{id_insn[15]},{id_insn[15]},{id_insn[15]},{id_insn[15]},{id_insn[15]},{id_insn[15]},{id_insn[15]},{id_insn[15]}, id_insn[15:0]} : {{16'b0}, id_insn[15:0]};

//
// Sign extension of branch offset
//
assign branch_addrofs = {{ex_insn[25]},{ex_insn[25]},{ex_insn[25]},{ex_insn[25]},{ex_insn[25]}, ex_insn[25:0]};

//
// l.macrc in ID stage
//

assign id_macrc_op = (id_insn[31:26] == `OR1200_OR32_MOVHI) & id_insn[16];

//
// cust5_op, cust5_limm (L immediate)
//
assign cust5_op = ex_insn[4:0];
assign cust5_limm = ex_insn[10:5];

//
//
//
assign rfe = (pre_branch_op == `OR1200_BRANCHOP_RFE) | (branch_op == `OR1200_BRANCHOP_RFE);

//
// Generation of sel_a
//
always @(rf_addrw or id_insn or rfwb_op or wbforw_valid or wb_rfaddrw)
	if ((id_insn[20:16] == rf_addrw) && rfwb_op[0])
		sel_a = `OR1200_SEL_EX_FORW;
	else if ((id_insn[20:16] == wb_rfaddrw) && wbforw_valid)
		sel_a = `OR1200_SEL_WB_FORW;
	else
		sel_a = `OR1200_SEL_RF;

//
// Generation of sel_b
//
always @(rf_addrw or sel_imm or id_insn or rfwb_op or wbforw_valid or wb_rfaddrw)
	if (sel_imm)
		sel_b = `OR1200_SEL_IMM;
	else if ((id_insn[15:11] == rf_addrw) && rfwb_op[0])
		sel_b = `OR1200_SEL_EX_FORW;
	else if ((id_insn[15:11] == wb_rfaddrw) && wbforw_valid)
		sel_b = `OR1200_SEL_WB_FORW;
	else
		sel_b = `OR1200_SEL_RF;

//
// l.macrc in EX stage
//

always @(posedge clk ) begin
	if (rst)
		ex_macrc_op <=  1'b0;
	else if (!ex_freeze & id_freeze | flushpipe)
		ex_macrc_op <=  1'b0;
	else if (!ex_freeze)
		ex_macrc_op <=  id_macrc_op;
end
//
// Decode of spr_addrimm
//
always @(posedge clk ) begin
	if (rst)
		spr_addrimm <=  16'h0000;
	else if (!ex_freeze & id_freeze | flushpipe)
		spr_addrimm <=  16'h0000;
	else if (!ex_freeze) begin
		case (id_insn[31:26])	// synopsys parallel_case
			// l.mfspr
			`OR1200_OR32_MFSPR: 
				spr_addrimm <=  id_insn[15:0];
			// l.mtspr
			default:
				spr_addrimm <=  {id_insn[25:21], id_insn[10:0]};
		endcase
	end
end

//
// Decode of multicycle
//
always @(id_insn) begin
  case (id_insn[31:26])		// synopsys parallel_case

    // l.lwz
    `OR1200_OR32_LWZ:
      multicycle = `OR1200_TWO_CYCLES;
    
    // l.lbz
    `OR1200_OR32_LBZ:
      multicycle = `OR1200_TWO_CYCLES;
    
    // l.lbs
    `OR1200_OR32_LBS:
      multicycle = `OR1200_TWO_CYCLES;
    
    // l.lhz
    `OR1200_OR32_LHZ:
      multicycle = `OR1200_TWO_CYCLES;
    
    // l.lhs
    `OR1200_OR32_LHS:
      multicycle = `OR1200_TWO_CYCLES;
    
    // l.sw
    `OR1200_OR32_SW:
      multicycle = `OR1200_TWO_CYCLES;
    
    // l.sb
    `OR1200_OR32_SB:
      multicycle = `OR1200_TWO_CYCLES;
    
    // l.sh
    `OR1200_OR32_SH:
      multicycle = `OR1200_TWO_CYCLES;

    // ALU instructions except the one with immediate
    `OR1200_OR32_ALU:
      multicycle = id_insn[9:8];
    
    // Single cycle instructions
    default: begin
      multicycle = `OR1200_ONE_CYCLE;
    end
    
  endcase
  
end

//
// Decode of imm_signextend
//
always @(id_insn) begin
  case (id_insn[31:26])		// synopsys parallel_case

	// l.addi
	`OR1200_OR32_ADDI:
		imm_signextend = 1'b1;

	// l.addic
	`OR1200_OR32_ADDIC:
		imm_signextend = 1'b1;

	// l.xori
	`OR1200_OR32_XORI:
		imm_signextend = 1'b1;

	// l.muli

	`OR1200_OR32_MULI:
		imm_signextend = 1'b1;


	// l.maci

	`OR1200_OR32_MACI:
		imm_signextend = 1'b1;


	// SFXX insns with immediate
	`OR1200_OR32_SFXXI:
		imm_signextend = 1'b1;

	// Instructions with no or zero extended immediate
	default: begin
		imm_signextend = 1'b0;
	end

endcase

end

//
// LSU addr offset
//
always @(lsu_op or ex_insn) begin
	lsu_addrofs[10:0] = ex_insn[10:0];
	case(lsu_op)	// synopsys parallel_case
		`OR1200_LSUOP_SB : 
			lsu_addrofs[31:11] = {{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}}, ex_insn[25:21]};
			`OR1200_LSUOP_SH : 
			lsu_addrofs[31:11] = {{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}}, ex_insn[25:21]};
		
		`OR1200_LSUOP_SW : 
			lsu_addrofs[31:11] = {{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}},{{ex_insn[25]}}, ex_insn[25:21]};
		
		default : 
			lsu_addrofs[31:11] = {{{ex_insn[15]}},{{ex_insn[15]}},{{ex_insn[15]}},{{ex_insn[15]}},{{ex_insn[15]}},{{ex_insn[15]}},{{ex_insn[15]}},{{ex_insn[15]}},{{ex_insn[15]}},{{ex_insn[15]}},{{ex_insn[15]}},{{ex_insn[15]}},{{ex_insn[15]}},{{ex_insn[15]}},{{ex_insn[15]}},{{ex_insn[15]}}, ex_insn[15:11]};
	endcase
end

//
// Register file write address
//
always @(posedge clk) begin
	if (rst)
		rf_addrw <=  5'b00000;
	else if (!ex_freeze & id_freeze)
		rf_addrw <=  5'b00000;
	else if (!ex_freeze)
		case (pre_branch_op)	// synopsys parallel_case
`OR1200_BRANCHOP_BAL:
				rf_addrw <=  5'b01001;	// link register r9
				`OR1200_BRANCHOP_JR:
				rf_addrw <=  5'b01001;
			default:
				rf_addrw <=  id_insn[25:21];
		endcase
end

//
// rf_addrw in wb stage (used in forwarding logic)
//
always @(posedge clk ) begin
	if (rst)
		wb_rfaddrw <=  5'b00000;
	else if (!wb_freeze)
		wb_rfaddrw <=  rf_addrw;
end

//
// Instruction latch in id_insn
//
always @(posedge clk ) begin
	if (rst)
		id_insn <=  {`OR1200_OR32_NOP, 26'h0410000};
        else if (flushpipe)
                id_insn <=  {`OR1200_OR32_NOP, 26'h0410000};        // id_insn[16] must be 1
	else if (!id_freeze) begin
		id_insn <=  if_insn;

	end
end

//
// Instruction latch in ex_insn
//
always @(posedge clk ) begin
	if (rst)
		ex_insn <=  {`OR1200_OR32_NOP, 26'h0410000};
	else if (!ex_freeze & id_freeze | flushpipe)
		ex_insn <=  {`OR1200_OR32_NOP, 26'h0410000};	// ex_insn[16] must be 1
	else if (!ex_freeze) begin
		ex_insn <=  id_insn;
	end
end

//
// Instruction latch in wb_insn
//
always @(posedge clk ) begin
	if (rst)
		wb_insn <=  {`OR1200_OR32_NOP, 26'h0410000};
	else if (flushpipe)
		wb_insn <=  {`OR1200_OR32_NOP, 26'h0410000};	// wb_insn[16] must be 1
	else if (!wb_freeze) begin
		wb_insn <=  ex_insn;
	end
end

//
// Decode of sel_imm
//
always @(posedge clk ) begin
	if (rst)
		sel_imm <=  1'b0;
	else if (!id_freeze) begin
	  case (if_insn[31:26])		// synopsys parallel_case

	    // j.jalr
	    `OR1200_OR32_JALR:
	      sel_imm <=  1'b0;
	    
	    // l.jr
	    `OR1200_OR32_JR:
	      sel_imm <=  1'b0;
	    
	    // l.rfe
	    `OR1200_OR32_RFE:
	      sel_imm <=  1'b0;
	    
	    // l.mfspr
	    `OR1200_OR32_MFSPR:
	      sel_imm <=  1'b0;
	    
	    // l.mtspr
	    `OR1200_OR32_MTSPR:
	      sel_imm <=  1'b0;
	    
	    // l.sys, l.brk and all three sync insns
	    `OR1200_OR32_XSYNC:
	      sel_imm <=  1'b0;
	    
	    // l.mac/l.msb

	    `OR1200_OR32_MACMSB:
	      sel_imm <=  1'b0;

	    // l.sw
	    `OR1200_OR32_SW:
	      sel_imm <=  1'b0;
	    
	    // l.sb
	    `OR1200_OR32_SB:
	      sel_imm <=  1'b0;
	    
	    // l.sh
	    `OR1200_OR32_SH:
	      sel_imm <=  1'b0;
	    
	    // ALU instructions except the one with immediate
	    `OR1200_OR32_ALU:
	      sel_imm <=  1'b0;
	    
	    // SFXX instructions
	    `OR1200_OR32_SFXX:
	      sel_imm <=  1'b0;


	    // l.cust5 instructions
	    `OR1200_OR32_CUST5:
	      sel_imm <=  1'b0;

	    
	    // l.nop
	    `OR1200_OR32_NOP:
	      sel_imm <=  1'b0;

	    // All instructions with immediates
	    default: begin
	      sel_imm <=  1'b1;
	    end
	    
	  endcase
	  
	end
end

//
// Decode of except_illegal
//
always @(posedge clk ) begin
	if (rst)
		except_illegal <=  1'b0;
	else if (!ex_freeze & id_freeze | flushpipe)
		except_illegal <=  1'b0;
	else if (!ex_freeze) begin
	      except_illegal <=  1'b1;
	end
end

//
// Decode of alu_op
//
always @(posedge clk ) begin
	if (rst)
		alu_op <=  `OR1200_ALUOP_NOP;
	else if (!ex_freeze & id_freeze | flushpipe)
		alu_op <=  `OR1200_ALUOP_NOP;
	else if (!ex_freeze) begin
	  case (id_insn[31:26])		// synopsys parallel_case
	    
	    // l.j
	    `OR1200_OR32_J:
	      alu_op <=  `OR1200_ALUOP_IMM;
	    
	    // j.jal
	    `OR1200_OR32_JAL:
	      alu_op <=  `OR1200_ALUOP_IMM;
	    
	    // l.bnf
	    `OR1200_OR32_BNF:
	      alu_op <=  `OR1200_ALUOP_NOP;
	    
	    // l.bf
	    `OR1200_OR32_BF:
	      alu_op <=  `OR1200_ALUOP_NOP;
	    
	    // l.movhi
	    `OR1200_OR32_MOVHI:
	      alu_op <=  `OR1200_ALUOP_MOVHI;
	    
	    // l.mfspr
	    `OR1200_OR32_MFSPR:
	      alu_op <=  `OR1200_ALUOP_MFSR;
	    
	    // l.mtspr
	    `OR1200_OR32_MTSPR:
	      alu_op <=  `OR1200_ALUOP_MTSR;
	    
	    // l.addi
	    `OR1200_OR32_ADDI:
	      alu_op <=  `OR1200_ALUOP_ADD;
	    
	    // l.addic
	    `OR1200_OR32_ADDIC:
	      alu_op <=  `OR1200_ALUOP_ADDC;
	    
	    // l.andi
	    `OR1200_OR32_ANDI:
	      alu_op <=  `OR1200_ALUOP_AND;
	    
	    // l.ori
	    `OR1200_OR32_ORI:
	      alu_op <=  `OR1200_ALUOP_OR;
	    
	    // l.xori
	    `OR1200_OR32_XORI:
	      alu_op <=  `OR1200_ALUOP_XOR;
	    
	    // l.muli

	    `OR1200_OR32_MULI:
	      alu_op <=  `OR1200_ALUOP_MUL;

	    
	    // Shift and rotate insns with immediate
	    `OR1200_OR32_SH_ROTI:
	      alu_op <=  `OR1200_ALUOP_SHROT;
	    
	    // SFXX insns with immediate
	    `OR1200_OR32_SFXXI:
	      alu_op <=  `OR1200_ALUOP_COMP;
	    
	    // ALU instructions except the one with immediate
	    `OR1200_OR32_ALU:
	      alu_op <=  id_insn[3:0];
	    
	    // SFXX instructions
	    `OR1200_OR32_SFXX:
	      alu_op <=  `OR1200_ALUOP_COMP;


	    // l.cust5 instructions
	    `OR1200_OR32_CUST5:
	      alu_op <=  `OR1200_ALUOP_CUST5;	    
	    // Default
	    default: begin
	      alu_op <=  `OR1200_ALUOP_NOP;
	    end
	      
	  endcase
	  
	end
end

//
// Decode of mac_op

always @(posedge clk ) begin
	if (rst)
		mac_op <=  `OR1200_MACOP_NOP;
	else if (!ex_freeze & id_freeze | flushpipe)
		mac_op <=  `OR1200_MACOP_NOP;
	else if (!ex_freeze)
	  case (id_insn[31:26])		// synopsys parallel_case

	    // l.maci
	    `OR1200_OR32_MACI:
	      mac_op <=  `OR1200_MACOP_MAC;

	    // l.nop
	    `OR1200_OR32_MACMSB:
	      mac_op <=  id_insn[1:0];

	    // Illegal and OR1200 unsupported instructions
	    default: begin
	      mac_op <=  `OR1200_MACOP_NOP;
	    end	      

	  endcase
	else
		mac_op <=  `OR1200_MACOP_NOP;
end

//
// Decode of shrot_op
//
always @(posedge clk ) begin
	if (rst)
		shrot_op <=  `OR1200_SHROTOP_NOP;
	else if (!ex_freeze & id_freeze | flushpipe)
		shrot_op <=  `OR1200_SHROTOP_NOP;
	else if (!ex_freeze) begin
		shrot_op <=  id_insn[7:6];
	end
end

//
// Decode of rfwb_op
//
always @(posedge clk ) begin
	if (rst)
		rfwb_op <=  `OR1200_RFWBOP_NOP;
	else  if (!ex_freeze & id_freeze | flushpipe)
		rfwb_op <=  `OR1200_RFWBOP_NOP;
	else  if (!ex_freeze) begin
		case (id_insn[31:26])		// synopsys parallel_case

		  // j.jal
		  `OR1200_OR32_JAL:
		    rfwb_op <=  `OR1200_RFWBOP_LR;
		  
		  // j.jalr
		  `OR1200_OR32_JALR:
		    rfwb_op <=  `OR1200_RFWBOP_LR;
		  
		  // l.movhi
		  `OR1200_OR32_MOVHI:
		    rfwb_op <=  `OR1200_RFWBOP_ALU;
		  
		  // l.mfspr
		  `OR1200_OR32_MFSPR:
		    rfwb_op <=  `OR1200_RFWBOP_SPRS;
		  
		  // l.lwz
		  `OR1200_OR32_LWZ:
		    rfwb_op <=  `OR1200_RFWBOP_LSU;
		  
		  // l.lbz
		  `OR1200_OR32_LBZ:
		    rfwb_op <=  `OR1200_RFWBOP_LSU;
		  
		  // l.lbs
		  `OR1200_OR32_LBS:
		    rfwb_op <=  `OR1200_RFWBOP_LSU;
		  
		  // l.lhz
		  `OR1200_OR32_LHZ:
		    rfwb_op <=  `OR1200_RFWBOP_LSU;
		  
		  // l.lhs
		  `OR1200_OR32_LHS:
		    rfwb_op <=  `OR1200_RFWBOP_LSU;
		  
		  // l.addi
		  `OR1200_OR32_ADDI:
		    rfwb_op <=  `OR1200_RFWBOP_ALU;
		  
		  // l.addic
		  `OR1200_OR32_ADDIC:
		    rfwb_op <=  `OR1200_RFWBOP_ALU;
		  
		  // l.andi
		  `OR1200_OR32_ANDI:
		    rfwb_op <=  `OR1200_RFWBOP_ALU;
		  
		  // l.ori
		  `OR1200_OR32_ORI:
		    rfwb_op <=  `OR1200_RFWBOP_ALU;
		  
		  // l.xori
		  `OR1200_OR32_XORI:
		    rfwb_op <=  `OR1200_RFWBOP_ALU;
		  
		  // l.muli

		  `OR1200_OR32_MULI:
		    rfwb_op <=  `OR1200_RFWBOP_ALU;

		  
		  // Shift and rotate insns with immediate
		  `OR1200_OR32_SH_ROTI:
		    rfwb_op <=  `OR1200_RFWBOP_ALU;
		  
		  // ALU instructions except the one with immediate
		  `OR1200_OR32_ALU:
		    rfwb_op <=  `OR1200_RFWBOP_ALU;


		  // l.cust5 instructions
		  `OR1200_OR32_CUST5:
		    rfwb_op <=  `OR1200_RFWBOP_ALU;


		  // Instructions w/o register-file write-back
		  default: begin
		    rfwb_op <=  `OR1200_RFWBOP_NOP;
		  end

		endcase
	end
end

//
// Decode of pre_branch_op
//
always @(posedge clk ) begin
	if (rst)
		pre_branch_op <=  `OR1200_BRANCHOP_NOP;
	else if (flushpipe)
		pre_branch_op <=  `OR1200_BRANCHOP_NOP;
	else if (!id_freeze) begin
		case (if_insn[31:26])		// synopsys parallel_case
		  
		  // l.j
		  `OR1200_OR32_J:
		    pre_branch_op <=  `OR1200_BRANCHOP_BAL;
		  
		  // j.jal
		  `OR1200_OR32_JAL:
		    pre_branch_op <=  `OR1200_BRANCHOP_BAL;
		  
		  // j.jalr
		  `OR1200_OR32_JALR:
		    pre_branch_op <=  `OR1200_BRANCHOP_JR;
		  
		  // l.jr
		  `OR1200_OR32_JR:
		    pre_branch_op <=  `OR1200_BRANCHOP_JR;
		  
		  // l.bnf
		  `OR1200_OR32_BNF:
		    pre_branch_op <=  `OR1200_BRANCHOP_BNF;
		  
		  // l.bf
		  `OR1200_OR32_BF:
		    pre_branch_op <=  `OR1200_BRANCHOP_BF;
		  
		  // l.rfe
		  `OR1200_OR32_RFE:
		    pre_branch_op <=  `OR1200_BRANCHOP_RFE;
		  
		  // Non branch instructions
		  default: begin
		    pre_branch_op <=  `OR1200_BRANCHOP_NOP;
		  end
		endcase
	end
end

//
// Generation of branch_op
//
always @(posedge clk )
	if (rst)
		branch_op <=  `OR1200_BRANCHOP_NOP;
	else if (!ex_freeze & id_freeze | flushpipe)
		branch_op <=  `OR1200_BRANCHOP_NOP;		
	else if (!ex_freeze)
		branch_op <=  pre_branch_op;

//
// Decode of lsu_op
//
always @(posedge clk ) begin
	if (rst)
		lsu_op <=  `OR1200_LSUOP_NOP;
	else if (!ex_freeze & id_freeze | flushpipe)
		lsu_op <=  `OR1200_LSUOP_NOP;
	else if (!ex_freeze)  begin
	  case (id_insn[31:26])		// synopsys parallel_case
	    
	    // l.lwz
	    `OR1200_OR32_LWZ:
	      lsu_op <=  `OR1200_LSUOP_LWZ;
	    
	    // l.lbz
	    `OR1200_OR32_LBZ:
	      lsu_op <=  `OR1200_LSUOP_LBZ;
	    
	    // l.lbs
	    `OR1200_OR32_LBS:
	      lsu_op <=  `OR1200_LSUOP_LBS;
	    
	    // l.lhz
	    `OR1200_OR32_LHZ:
	      lsu_op <=  `OR1200_LSUOP_LHZ;
	    
	    // l.lhs
	    `OR1200_OR32_LHS:
	      lsu_op <=  `OR1200_LSUOP_LHS;
	    
	    // l.sw
	    `OR1200_OR32_SW:
	      lsu_op <=  `OR1200_LSUOP_SW;
	    
	    // l.sb
	    `OR1200_OR32_SB:
	      lsu_op <=  `OR1200_LSUOP_SB;
	    
	    // l.sh
	    `OR1200_OR32_SH:
	      lsu_op <=  `OR1200_LSUOP_SH;
	    
	    // Non load/store instructions
	    default: begin
	      lsu_op <=  `OR1200_LSUOP_NOP;
	    end
	  endcase
	end
end

//
// Decode of comp_op
//
always @(posedge clk ) begin
	if (rst) begin
		comp_op <=  4'b0000;
	end else if (!ex_freeze & id_freeze | flushpipe)
		comp_op <=  4'b0000;
	else if (!ex_freeze)
		comp_op <=  id_insn[24:21];
end

//
// Decode of l.sys
//
always @(posedge clk ) begin
	if (rst)
		sig_syscall <=  1'b0;
	else if (!ex_freeze & id_freeze | flushpipe)
		sig_syscall <=  1'b0;
	else if (!ex_freeze) begin

		sig_syscall <=  (id_insn[31:23] == {`OR1200_OR32_XSYNC, 3'b000});
	end
end

//
// Decode of l.trap
//
always @(posedge clk ) begin
	if (rst)
		sig_trap <=  1'b0;
	else if (!ex_freeze & id_freeze | flushpipe)
		sig_trap <=  1'b0;
	else if (!ex_freeze) begin

		sig_trap <=  (id_insn[31:23] == {`OR1200_OR32_XSYNC, 3'b010})
			| du_hwbkpt;
	end
end

endmodule


module or1200_rf(
	// Clock and reset
	clk, rst,

	// Write i/f
	supv, wb_freeze, addrw, dataw,id_freeze, we, flushpipe,

	// Read i/f
 addra, rda, dataa,  addrb,rdb, datab, 
	// Debug
	spr_cs, spr_write, spr_addr, spr_dat_i, spr_dat_o
);

//parameter aw = `OR1200_OPERAND_WIDTH;
//parameter dw= `OR1200_REGFILE_ADDR_WIDTH;

//
// I/O
//

//
// Clock and reset
//
input				clk;
input				rst;

//
// Write i/f
//
input				supv;
input				wb_freeze;
input	[`OR1200_REGFILE_ADDR_WIDTH-1:0]		addrw;
input	[`OR1200_OPERAND_WIDTH-1:0]		dataw;
input				we;
input				flushpipe;

//
// Read i/f
//
input				id_freeze;
input	[`OR1200_REGFILE_ADDR_WIDTH-1:0]		addra;
input	[`OR1200_REGFILE_ADDR_WIDTH-1:0]		addrb;
output	[`OR1200_OPERAND_WIDTH-1:0]		dataa;
output	[`OR1200_OPERAND_WIDTH-1:0]		datab;
input				rda;
input				rdb;

//
// SPR access for debugging purposes
//
input				spr_cs;
input				spr_write;
input	[31:0]			spr_addr;
input	[31:0]			spr_dat_i;
output	[31:0]			spr_dat_o;

//
// Internal wires and regs
//
wire	[`OR1200_OPERAND_WIDTH-1:0]		from_rfa;
wire	[`OR1200_OPERAND_WIDTH-1:0]		from_rfb;
reg	[`OR1200_OPERAND_WIDTH:0]			dataa_saved;
reg	[`OR1200_OPERAND_WIDTH:0]			datab_saved;
wire	[`OR1200_REGFILE_ADDR_WIDTH-1:0]		rf_addra;
wire	[`OR1200_REGFILE_ADDR_WIDTH-1:0]		rf_addrw;
wire	[`OR1200_OPERAND_WIDTH-1:0]		rf_dataw;
wire				rf_we;
wire				spr_valid;
wire				rf_ena;
wire				rf_enb;
reg				rf_we_allow;

//
// SPR access is valid when spr_cs is asserted and
// SPR address matches GPR addresses
//
assign spr_valid = spr_cs & (spr_addr[10:5] == `OR1200_SPR_RF);

//
// SPR data output is always from RF A
//
assign spr_dat_o = from_rfa;

//
// Operand A comes from RF or from saved A register
//
assign dataa = (dataa_saved[32]) ? dataa_saved[31:0] : from_rfa;

//
// Operand B comes from RF or from saved B register
//
assign datab = (datab_saved[32]) ? datab_saved[31:0] : from_rfb;

//
// RF A read address is either from SPRS or normal from CPU control
//
assign rf_addra = (spr_valid & !spr_write) ? spr_addr[4:0] : addra;

//
// RF write address is either from SPRS or normal from CPU control
//
assign rf_addrw = (spr_valid & spr_write) ? spr_addr[4:0] : addrw;

//
// RF write data is either from SPRS or normal from CPU datapath
//
assign rf_dataw = (spr_valid & spr_write) ? spr_dat_i : dataw;

//
// RF write enable is either from SPRS or normal from CPU control
//
always @(posedge clk)
	if (rst)
		rf_we_allow <=  1'b1;
	else if (~wb_freeze)
		rf_we_allow <= ~flushpipe;

assign rf_we = ((spr_valid & spr_write) | (we & ~wb_freeze)) & rf_we_allow & (supv | (|rf_addrw));

//
// CS RF A asserted when instruction reads operand A and ID stage
// is not stalled
//
assign rf_ena = rda & ~id_freeze | spr_valid;	// probably works with fixed binutils
// assign rf_ena = 1'b1;			// does not work with single-stepping
//assign rf_ena = ~id_freeze | spr_valid;	// works with broken binutils 

//
// CS RF B asserted when instruction reads operand B and ID stage
// is not stalled
//
assign rf_enb = rdb & ~id_freeze | spr_valid;
// assign rf_enb = 1'b1;
//assign rf_enb = ~id_freeze | spr_valid;	// works with broken binutils 

//
// Stores operand from RF_A into temp reg when pipeline is frozen
//
always @(posedge clk )
	if (rst) begin
		dataa_saved <=33'b000000000000000000000000000000000;
	end
	else if (id_freeze & !dataa_saved[32]) begin
		dataa_saved <= {1'b1, from_rfa};
	end
	else if (!id_freeze)
		dataa_saved <=33'b000000000000000000000000000000000;

//
// Stores operand from RF_B into temp reg when pipeline is frozen
//
always @(posedge clk)
	if (rst) begin
		datab_saved <=  33'b000000000000000000000000000000000;
	end
	else if (id_freeze & !datab_saved[32]) begin
		datab_saved <=  {1'b1, from_rfb};
	end
	else if (!id_freeze)
		datab_saved <=  33'b000000000000000000000000000000000;

		
		
		
// Black-box memory instead of the or1200 memory implementation

wire const_one;
wire const_zero;
assign const_one = 1'b1;
assign const_zero = 1'b0;
wire [31:0] const_zero_data;
assign const_zero_data = 32'b00000000000000000000000000000000;
wire [31:0] dont_care_out;
wire [31:0] dont_care_out2;

dual_port_ram rf_a(	

  .clk (clk),
  .we1(const_zero),
  .we2(rf_we),
  .data1(const_zero_data),
  .data2(rf_dataw),
  .out1(from_rfa),
  .out2 (dont_care_out),
  .addr1(rf_addra),
  .addr2(rf_addrw));
//
// Instantiation of register file two-port RAM A
//
/*
or1200_tpram_32x32 rf_a(
	// Port A
	.clk_a(clk),
	.rst_a(rst),
	.ce_a(rf_ena),
	.we_a(1'b0),
	.oe_a(1'b1),
	.addr_a(rf_addra),
	.di_a(32'h00000000),
	.do_a(from_rfa),

	// Port B
	.clk_b(clk),
	.rst_b(rst),
	.ce_b(rf_we),
	.we_b(rf_we),
	.oe_b(1'b0),
	.addr_b(rf_addrw),
	.di_b(rf_dataw),
	.do_b()
);
*/
//
// Instantiation of register file two-port RAM B
//

dual_port_ram rf_b(	
  .clk (clk),
  .we1(const_zero),
  .we2(rf_we),
  .data1(const_zero_data),
  .data2(rf_dataw),
  .out1(from_rfb),
  .out2 (dont_care_out2),
  .addr1(addrb),
  .addr2(rf_addrw));
/*
or1200_tpram_32x32 rf_b(
	// Port A
	.clk_a(clk),
	.rst_a(rst),
	.ce_a(rf_enb),
	.we_a(1'b0),
	.oe_a(1'b1),
	.addr_a(addrb),
	.di_a(32'h00000000),
	.do_a(from_rfb),

	// Port B
	.clk_b(clk),
	.rst_b(rst),
	.ce_b(rf_we),
	.we_b(rf_we),
	.oe_b(1'b0),
	.addr_b(rf_addrw),
	.di_b(rf_dataw),
	.do_b()
);
*/
wire unused;
assign unused = |spr_addr;
endmodule



module or1200_operandmuxes(
	// Clock and reset
	clk, rst,

	// Internal i/f
	id_freeze, ex_freeze, rf_dataa, rf_datab, ex_forw, wb_forw,
	simm, sel_a, sel_b, operand_a, operand_b, muxed_b
);

//parameter width = `OR1200_OPERAND_WIDTH;

//
// I/O
//
input				clk;
input				rst;
input				id_freeze;
input				ex_freeze;
input	[`OR1200_OPERAND_WIDTH-1:0]		rf_dataa;
input	[`OR1200_OPERAND_WIDTH-1:0]		rf_datab;
input	[`OR1200_OPERAND_WIDTH-1:0]		ex_forw;
input	[`OR1200_OPERAND_WIDTH-1:0]		wb_forw;
input	[`OR1200_OPERAND_WIDTH-1:0]		simm;
input	[`OR1200_SEL_WIDTH-1:0]	sel_a;
input	[`OR1200_SEL_WIDTH-1:0]	sel_b;
output	[`OR1200_OPERAND_WIDTH-1:0]		operand_a;
output	[`OR1200_OPERAND_WIDTH-1:0]		operand_b;
output	[`OR1200_OPERAND_WIDTH-1:0]		muxed_b;

//
// Internal wires and regs
//
reg	[`OR1200_OPERAND_WIDTH-1:0]		operand_a;
reg	[`OR1200_OPERAND_WIDTH-1:0]		operand_b;
reg	[`OR1200_OPERAND_WIDTH-1:0]		muxed_a;
reg	[`OR1200_OPERAND_WIDTH-1:0]		muxed_b;
reg				saved_a;
reg				saved_b;

//
// Operand A register
//
always @(posedge clk ) begin
	if (rst) begin
		operand_a <=  32'b0000000000000000000000000000;
		saved_a <=  1'b0;
	end else if (!ex_freeze && id_freeze && !saved_a) begin
		operand_a <=  muxed_a;
		saved_a <=  1'b1;
	end else if (!ex_freeze && !saved_a) begin
		operand_a <=  muxed_a;
	end else if (!ex_freeze && !id_freeze)
		saved_a <=  1'b0;
end

//
// Operand B register
//
always @(posedge clk ) begin
	if (rst) begin
		operand_b <=  32'b0000000000000000000000000000;
		saved_b <=  1'b0;
	end else if (!ex_freeze && id_freeze && !saved_b) begin
		operand_b <=  muxed_b;
		saved_b <=  1'b1;
	end else if (!ex_freeze && !saved_b) begin
		operand_b <=  muxed_b;
	end else if (!ex_freeze && !id_freeze)
		saved_b <=  1'b0;
end

//
// Forwarding logic for operand A register
//
always @(ex_forw or wb_forw or rf_dataa or sel_a) begin

	case (sel_a)	
		`OR1200_SEL_EX_FORW:
			muxed_a = ex_forw;
		`OR1200_SEL_WB_FORW:
			muxed_a = wb_forw;
		default:
			muxed_a = rf_dataa;
	endcase
end

//
// Forwarding logic for operand B register
//
always @(simm or ex_forw or wb_forw or rf_datab or sel_b) begin

	case (sel_b)	// synopsys parallel_case

		`OR1200_SEL_IMM:
			muxed_b = simm;
		`OR1200_SEL_EX_FORW:
			muxed_b = ex_forw;
		`OR1200_SEL_WB_FORW:
			muxed_b = wb_forw;
		default:
			muxed_b = rf_datab;
	endcase
end

endmodule





module or1200_alu(
	a, b, mult_mac_result, macrc_op,
	alu_op, shrot_op, comp_op,
	cust5_op, cust5_limm,
	result, flagforw, flag_we,
	cyforw, cy_we, flag,k_carry
);

//
// I/O
//
input	[32-1:0]		a;
input	[32-1:0]		b;
input	[32-1:0]		mult_mac_result;
input				macrc_op;
input	[`OR1200_ALUOP_WIDTH-1:0]	alu_op;
input	[2-1:0]	shrot_op;
input	[4-1:0]	comp_op;
input	[4:0]			cust5_op;
input	[5:0]			cust5_limm;
output	[32-1:0]		result;
output				flagforw;
output				flag_we;
output				cyforw;
output				cy_we;
input				k_carry;
input         flag;

//
// Internal wires and regs
//
reg	[32-1:0]		result;
reg	[32-1:0]		shifted_rotated;
reg	[32-1:0]		result_cust5;
reg				flagforw;
reg				flagcomp;
reg				flag_we;
reg				cy_we;
wire	[32-1:0]		comp_a;
wire	[32-1:0]		comp_b;

wire				a_eq_b;
wire				a_lt_b;

wire	[32-1:0]		result_sum;

wire	[32-1:0]		result_csum;
wire				cy_csum;

wire	[32-1:0]		result_and;
wire				cy_sum;
reg				cyforw;

//
// Combinatorial logic
//
assign comp_a [31:3]= a[31] ^ comp_op[3];
assign comp_a [2:0] = a[30:0];

assign comp_b [31:3]  = b[31] ^ comp_op[3] ;
assign comp_b [2:0] =  b[32-2:0];

assign a_eq_b = (comp_a == comp_b);
assign a_lt_b = (comp_a < comp_b);

assign cy_sum= a + b;
assign result_sum = a+b;
assign cy_csum =a + b + {32'b00000000000000000000000000000000, k_carry};
assign result_csum = a + b + {32'b00000000000000000000000000000000, k_carry};

assign result_and = a & b;



// Central part of the ALU
//
always @(alu_op or a or b or result_sum or result_and or macrc_op or shifted_rotated or mult_mac_result) 
begin

	case (alu_op)		// synopsys parallel_case

    4'b1111: begin
        result = a[0] ? 1 : a[1] ? 2 : a[2] ? 3 : a[3] ? 4 : a[4] ? 5 : a[5] ? 6 : a[6] ? 7 : a[7] ? 8 : a[8] ? 9 : a[9] ? 10 : a[10] ? 11 : a[11] ? 12 : a[12] ? 13 : a[13] ? 14 : a[14] ? 15 : a[15] ? 16 : a[16] ? 17 : a[17] ? 18 : a[18] ? 19 : a[19] ? 20 : a[20] ? 21 : a[21] ? 22 : a[22] ? 23 : a[23] ? 24 : a[24] ? 25 : a[25] ? 26 : a[26] ? 27 : a[27] ? 28 : a[28] ? 29 : a[29] ? 30 : a[30] ? 31 : a[31] ? 32 : 0;
    end
		`OR1200_ALUOP_CUST5 : begin 
				result = result_cust5;
		end
		`OR1200_ALUOP_SHROT : begin 
				result = shifted_rotated;
		end
		`OR1200_ALUOP_ADD : begin
				result = result_sum;
		end

		`OR1200_ALUOP_ADDC : begin
				result = result_csum;
		end

		`OR1200_ALUOP_SUB : begin
				result = a - b;
		end
		`OR1200_ALUOP_XOR : begin
				result = a ^ b;
		end
		`OR1200_ALUOP_OR  : begin
				result = a | b;
		end
		`OR1200_ALUOP_IMM : begin
				result = b;
		end
		`OR1200_ALUOP_MOVHI : begin
				if (macrc_op) begin
					result = mult_mac_result;
				end
				else begin
					result = b << 16;
				end
		end

		`OR1200_ALUOP_MUL : begin
				result = mult_mac_result;
		end

    4'b1110: begin
        result = flag ? a : b;
    end

    default: 
    begin
      result=result_and;
    end 
	endcase
end

//
// l.cust5 custom instructions
//
// Examples for move byte, set bit and clear bit
//
always @(cust5_op or cust5_limm or a or b) begin
	case (cust5_op)		// synopsys parallel_case
		5'h1 : begin 
			case (cust5_limm[1:0])
				2'h0: result_cust5 = {a[31:8], b[7:0]};
				2'h1: result_cust5 = {a[31:16], b[7:0], a[7:0]};
				2'h2: result_cust5 = {a[31:24], b[7:0], a[15:0]};
				2'h3: result_cust5 = {b[7:0], a[23:0]};
			endcase
		end
		5'h2 :
			result_cust5 = a | (1 << 4);
		5'h3 :
			result_cust5 = a & (32'b11111111111111111111111111111111^ (cust5_limm));
//
// *** Put here new l.cust5 custom instructions ***
//
		default: begin
			result_cust5 = a;
		end
	endcase
end

//
// Generate flag and flag write enable
//
always @(alu_op or result_sum or result_and or flagcomp) begin
	case (alu_op)		// synopsys parallel_case

		`OR1200_ALUOP_ADD : begin
			flagforw = (result_sum == 32'b00000000000000000000000000000000);
			flag_we = 1'b1;
		end

		`OR1200_ALUOP_ADDC : begin
			flagforw = (result_csum == 32'b00000000000000000000000000000000);
			flag_we = 1'b1;
		end

		`OR1200_ALUOP_AND: begin
			flagforw = (result_and == 32'b00000000000000000000000000000000);
			flag_we = 1'b1;
		end

		`OR1200_ALUOP_COMP: begin
			flagforw = flagcomp;
			flag_we = 1'b1;
		end
		default: begin
			flagforw = 1'b0;
			flag_we = 1'b0;
		end
	endcase
end

//
// Generate SR[CY] write enable
//
always @(alu_op or cy_sum


	) begin
	case (alu_op)		// synopsys parallel_case

		`OR1200_ALUOP_ADD : begin
			cyforw = cy_sum;
			cy_we = 1'b1;
		end

		`OR1200_ALUOP_ADDC: begin
			cyforw = cy_csum;
			cy_we = 1'b1;
		end

		default: begin
			cyforw = 1'b0;
			cy_we = 1'b0;
		end
	endcase
end

//
// Shifts and rotation
//
always @(shrot_op or a or b) begin
	case (shrot_op)		// synopsys parallel_case
	2'b00 :
				shifted_rotated = (a << 2);
		`OR1200_SHROTOP_SRL :
				shifted_rotated = (a >> 2);


		`OR1200_SHROTOP_ROR :
				shifted_rotated = (a << 1'b1);
		default:
				shifted_rotated = (a << 1);
	endcase
end

//
// First type of compare implementation
//
always @(comp_op or a_eq_b or a_lt_b) begin
	case(comp_op[2:0])	// synopsys parallel_case
		`OR1200_COP_SFEQ:
			flagcomp = a_eq_b;
		`OR1200_COP_SFNE:
			flagcomp = ~a_eq_b;
		`OR1200_COP_SFGT:
			flagcomp = ~(a_eq_b | a_lt_b);
		`OR1200_COP_SFGE:
			flagcomp = ~a_lt_b;
		`OR1200_COP_SFLT:
			flagcomp = a_lt_b;
		`OR1200_COP_SFLE:
			flagcomp = a_eq_b | a_lt_b;
		default:
			flagcomp = 1'b0;
	endcase
end

//

endmodule




module or1200_mult_mac(
	// Clock and reset
	clk, rst,

	// Multiplier/MAC interface
	ex_freeze, id_macrc_op, macrc_op, a, b, mac_op, alu_op, result, mac_stall_r,

	// SPR interface
	spr_cs, spr_write, spr_addr, spr_dat_i, spr_dat_o
);

//parameter width = `OR1200_OPERAND_WIDTH;

//
// I/O
//

//
// Clock and reset
//
input				clk;
input				rst;

//
// Multiplier/MAC interface
//
input				ex_freeze;
input				id_macrc_op;
input				macrc_op;
input	[`OR1200_OPERAND_WIDTH-1:0]		a;
input	[`OR1200_OPERAND_WIDTH-1:0]		b;
input	[`OR1200_MACOP_WIDTH-1:0]	mac_op;
input	[`OR1200_ALUOP_WIDTH-1:0]	alu_op;
output	[`OR1200_OPERAND_WIDTH-1:0]		result;
output				mac_stall_r;

//
// SPR interface
//
input				spr_cs;
input				spr_write;
input	[31:0]			spr_addr;
input	[31:0]			spr_dat_i;
output	[31:0]			spr_dat_o;

//
// Internal wires and regs
//

reg	[`OR1200_OPERAND_WIDTH-1:0]		result;
reg	[2*`OR1200_OPERAND_WIDTH-1:0]		mul_prod_r;

wire	[2*`OR1200_OPERAND_WIDTH-1:0]		mul_prod;
wire	[`OR1200_MACOP_WIDTH-1:0]	mac_op;

reg	[`OR1200_MACOP_WIDTH-1:0]	mac_op_r1;
reg	[`OR1200_MACOP_WIDTH-1:0]	mac_op_r2;
reg	[`OR1200_MACOP_WIDTH-1:0]	mac_op_r3;
reg				mac_stall_r;
reg	[2*`OR1200_OPERAND_WIDTH-1:0]		mac_r;

wire	[`OR1200_OPERAND_WIDTH-1:0]		x;
wire	[`OR1200_OPERAND_WIDTH-1:0]		y;
wire				spr_maclo_we;
wire				spr_machi_we;
wire				alu_op_div_divu;
wire				alu_op_div;
reg				div_free;

wire	[`OR1200_OPERAND_WIDTH-1:0]		div_tmp;
reg	[5:0]			div_cntr;


//
// Combinatorial logic
//

assign spr_maclo_we = spr_cs & spr_write & spr_addr[`OR1200_MAC_ADDR];
assign spr_machi_we = spr_cs & spr_write & !spr_addr[`OR1200_MAC_ADDR];
assign spr_dat_o = spr_addr[`OR1200_MAC_ADDR] ? mac_r[31:0] : mac_r[63:32];

assign x = (alu_op_div & a[31]) ? ~a + 1'b1 : alu_op_div_divu | (alu_op == `OR1200_ALUOP_MUL) | (|mac_op) ? a : 32'h00000000;
assign y = (alu_op_div & b[31]) ? ~b + 1'b1 : alu_op_div_divu | (alu_op == `OR1200_ALUOP_MUL) | (|mac_op) ? b : 32'h00000000;
assign alu_op_div = (alu_op == `OR1200_ALUOP_DIV);
assign alu_op_div_divu = alu_op_div | (alu_op == `OR1200_ALUOP_DIVU);
assign div_tmp = mul_prod_r[63:32] - y;




//
// Select result of current ALU operation to be forwarded
// to next instruction and to WB stage
//
always @(alu_op or mul_prod_r or mac_r or a or b)
	case(alu_op)	

		`OR1200_ALUOP_DIV:
			result = a[31] ^ b[31] ? ~mul_prod_r[31:0] + 1'b1 : mul_prod_r[31:0];
		`OR1200_ALUOP_DIVU:
		begin
			result = mul_prod_r[31:0];
		end

		`OR1200_ALUOP_MUL: begin
			result = mul_prod_r[31:0];
		end
		default:

		result = mac_r[31:0];

	endcase

//
// Instantiation of the multiplier
//

// doing an implicit multiply here
assign mul_prod = x * y;
/*
or1200_gmultp2_32x32 or1200_gmultp2_32x32(
	.X(x),
	.Y(y),
	.RST(rst),
	.CLK(clk),
	.P(mul_prod)
);
*/


//
// Registered output from the multiplier and
// an optional divider
//
always @(posedge clk)
	if (rst) begin
		mul_prod_r <=  64'h0000000000000000;
		div_free <=  1'b1;

		div_cntr <=  6'b000000;

	end

	else if (|div_cntr) begin
		if (div_tmp[31])
			mul_prod_r <=  {mul_prod_r[62:0], 1'b0};
		else
			mul_prod_r <=  {div_tmp[30:0], mul_prod_r[31:0], 1'b1};
		div_cntr <=  div_cntr - 1'b1;
	end
	else if (alu_op_div_divu && div_free) begin
		mul_prod_r <=  {31'b0000000000000000000000000000000, x[31:0], 1'b0};
		div_cntr <=  6'b100000;
		div_free <=  1'b0;
	end

	else if (div_free | !ex_freeze) begin
		mul_prod_r <=  mul_prod[63:0];
		div_free <=  1'b1;
	end


// Propagation of l.mac opcode
//
always @(posedge clk)
	if (rst)
		mac_op_r1 <=  2'b00;
	else
		mac_op_r1 <=  mac_op;

//
// Propagation of l.mac opcode
//
always @(posedge clk)
	if (rst)
		mac_op_r2 <=  2'b00;
	else
		mac_op_r2 <=  mac_op_r1;

//
// Propagation of l.mac opcode
//
always @(posedge clk )
	if (rst)
		mac_op_r3 <=  2'b00;
	else
		mac_op_r3 <=  mac_op_r2;

//
// Implementation of MAC
//
always @(posedge clk)
	if (rst)
		mac_r <=  64'h0000000000000000;

	else if (spr_maclo_we)
		mac_r[31:0] <=  spr_dat_i;
	else if (spr_machi_we)
		mac_r[63:32] <=  spr_dat_i;

	else if (mac_op_r3 == `OR1200_MACOP_MAC)
		mac_r <=  mac_r + mul_prod_r;
	else if (mac_op_r3 == `OR1200_MACOP_MSB)
		mac_r <=  mac_r - mul_prod_r;
	else if (macrc_op & !ex_freeze)
		mac_r <=  64'h0000000000000000;

//
// Stall CPU if l.macrc is in ID and MAC still has to process l.mac instructions
// in EX stage (e.g. inside multiplier)
// This stall signal is also used by the divider.
//

wire unused;
assign unused = |spr_addr;
always @( posedge clk)
	if (rst)
		mac_stall_r <=  1'b0;
	else
		mac_stall_r <=  (|mac_op | (|mac_op_r1) | (|mac_op_r2)) & id_macrc_op

				| (|div_cntr)

				;

				
				

endmodule




module or1200_sprs(
		// Clk & Rst
		clk, rst,

		// Internal CPU interface
			addrbase, addrofs, dat_i, alu_op, 
		flagforw, flag_we, flag, cyforw, cy_we, carry,	to_wbmux,
		
		du_addr, du_dat_du, du_read,
		du_write, du_dat_cpu,
		
		spr_addr,spr_dat_pic, spr_dat_tt, spr_dat_pm,
		spr_dat_cfgr, spr_dat_rf, spr_dat_npc, spr_dat_ppc, spr_dat_mac,
		spr_dat_dmmu, spr_dat_immu, spr_dat_du, spr_dat_o, spr_cs, spr_we,
		
		 epcr_we, eear_we,esr_we, pc_we,epcr, eear, esr, except_started,
		
		sr_we, to_sr, sr,branch_op

);

//parameter width = `OR1200_OPERAND_WIDTH;

//
// I/O Ports
//

//
// Internal CPU interface
//
input				clk; 		// Clock
input 				rst;		// Reset
input 				flagforw;	// From ALU
input 				flag_we;	// From ALU
output 				flag;		// SR[F]
input 				cyforw;		// From ALU
input 				cy_we;		// From ALU
output 				carry;		// SR[CY]
input	[`OR1200_OPERAND_WIDTH-1:0] 		addrbase;	// SPR base address
input	[15:0] 			addrofs;	// SPR offset
input	[`OR1200_OPERAND_WIDTH-1:0]		dat_i;		// SPR write data
input	[`OR1200_ALUOP_WIDTH-1:0]	alu_op;		// ALU operation
input	[`OR1200_BRANCHOP_WIDTH-1:0]	branch_op;	// Branch operation
input	[`OR1200_OPERAND_WIDTH-1:0] 		epcr;		// EPCR0
input	[`OR1200_OPERAND_WIDTH-1:0] 		eear;		// EEAR0
input	[`OR1200_SR_WIDTH-1:0] 	esr;		// ESR0
input 				except_started; // Exception was started
output	[`OR1200_OPERAND_WIDTH-1:0]		to_wbmux;	// For l.mfspr
output				epcr_we;	// EPCR0 write enable
output				eear_we;	// EEAR0 write enable
output				esr_we;		// ESR0 write enable
output				pc_we;		// PC write enable
output 				sr_we;		// Write enable SR
output	[`OR1200_SR_WIDTH-1:0]	to_sr;		// Data to SR
output	[`OR1200_SR_WIDTH-1:0]	sr;		// SR
input	[31:0]			spr_dat_cfgr;	// Data from CFGR
input	[31:0]			spr_dat_rf;	// Data from RF
input	[31:0]			spr_dat_npc;	// Data from NPC
input	[31:0]			spr_dat_ppc;	// Data from PPC   
input	[31:0]			spr_dat_mac;	// Data from MAC

//
// To/from other RISC units
//
input	[31:0]			spr_dat_pic;	// Data from PIC
input	[31:0]			spr_dat_tt;	// Data from TT
input	[31:0]			spr_dat_pm;	// Data from PM
input	[31:0]			spr_dat_dmmu;	// Data from DMMU
input	[31:0]			spr_dat_immu;	// Data from IMMU
input	[31:0]			spr_dat_du;	// Data from DU
output	[31:0]			spr_addr;	// SPR Address
output	[31:0]			spr_dat_o;	// Data to unit
output	[31:0]			spr_cs;		// Unit select
output				spr_we;		// SPR write enable

//
// To/from Debug Unit
//
input	[`OR1200_OPERAND_WIDTH-1:0]		du_addr;	// Address
input	[`OR1200_OPERAND_WIDTH-1:0]		du_dat_du;	// Data from DU to SPRS
input				du_read;	// Read qualifier
input				du_write;	// Write qualifier
output	[`OR1200_OPERAND_WIDTH-1:0]		du_dat_cpu;	// Data from SPRS to DU

//
// Internal regs & wires
//
reg	[`OR1200_SR_WIDTH-1:0]		sr;		// SR
reg				write_spr;	// Write SPR
reg				read_spr;	// Read SPR
reg	[`OR1200_OPERAND_WIDTH-1:0]		to_wbmux;	// For l.mfspr
wire				cfgr_sel;	// Select for cfg regs
wire				rf_sel;		// Select for RF
wire				npc_sel;	// Select for NPC
wire				ppc_sel;	// Select for PPC
wire 				sr_sel;		// Select for SR	
wire 				epcr_sel;	// Select for EPCR0
wire 				eear_sel;	// Select for EEAR0
wire 				esr_sel;	// Select for ESR0
wire	[31:0]			sys_data;	// Read data from system SPRs
wire				du_access;	// Debug unit access
wire	[`OR1200_ALUOP_WIDTH-1:0]	sprs_op;	// ALU operation
reg	[31:0]			unqualified_cs;	// Unqualified chip selects

//
// Decide if it is debug unit access
//
assign du_access = du_read | du_write;

//
// Generate sprs opcode
//
assign sprs_op = du_write ? `OR1200_ALUOP_MTSR : du_read ? `OR1200_ALUOP_MFSR : alu_op;

//
// Generate SPR address from base address and offset
// OR from debug unit address
//
assign spr_addr = du_access ? du_addr : addrbase | {16'h0000, addrofs};

//
// SPR is written by debug unit or by l.mtspr
//
assign spr_dat_o = du_write ? du_dat_du : dat_i;

//
// debug unit data input:
//  - write into debug unit SPRs by debug unit itself
//  - read of SPRS by debug unit
//  - write into debug unit SPRs by l.mtspr
//
assign du_dat_cpu = du_write ? du_dat_du : du_read ? to_wbmux : dat_i;

//
// Write into SPRs when l.mtspr
//
assign spr_we = du_write | write_spr;

//
// Qualify chip selects
//
assign spr_cs = unqualified_cs & {{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr},{read_spr | write_spr}};

//
// Decoding of groups
//
always @(spr_addr)
	case (spr_addr[15:11])	
		5'b00000: unqualified_cs = 32'b00000000000000000000000000000001;
		5'b00001: unqualified_cs = 32'b00000000000000000000000000000010;
		5'b00010: unqualified_cs = 32'b00000000000000000000000000000100;
		5'b00011: unqualified_cs = 32'b00000000000000000000000000001000;
		5'b00100: unqualified_cs = 32'b00000000000000000000000000010000;
		5'b00101: unqualified_cs = 32'b00000000000000000000000000100000;
		5'b00110: unqualified_cs = 32'b00000000000000000000000001000000;
		5'b00111: unqualified_cs = 32'b00000000000000000000000010000000;
		5'b01000: unqualified_cs = 32'b00000000000000000000000100000000;
		5'b01001: unqualified_cs = 32'b00000000000000000000001000000000;
		5'b01010: unqualified_cs = 32'b00000000000000000000010000000000;
		5'b01011: unqualified_cs = 32'b00000000000000000000100000000000;
		5'b01100: unqualified_cs = 32'b00000000000000000001000000000000;
		5'b01101: unqualified_cs = 32'b00000000000000000010000000000000;
		5'b01110: unqualified_cs = 32'b00000000000000000100000000000000;
		5'b01111: unqualified_cs = 32'b00000000000000001000000000000000;
		5'b10000: unqualified_cs = 32'b00000000000000010000000000000000;
		5'b10001: unqualified_cs = 32'b00000000000000100000000000000000;
		5'b10010: unqualified_cs = 32'b00000000000001000000000000000000;
		5'b10011: unqualified_cs = 32'b00000000000010000000000000000000;
		5'b10100: unqualified_cs = 32'b00000000000100000000000000000000;
		5'b10101: unqualified_cs = 32'b00000000001000000000000000000000;
		5'b10110: unqualified_cs = 32'b00000000010000000000000000000000;
		5'b10111: unqualified_cs = 32'b00000000100000000000000000000000;
		5'b11000: unqualified_cs = 32'b00000001000000000000000000000000;
		5'b11001: unqualified_cs = 32'b00000010000000000000000000000000;
		5'b11010: unqualified_cs = 32'b00000100000000000000000000000000;
		5'b11011: unqualified_cs = 32'b00001000000000000000000000000000;
		5'b11100: unqualified_cs = 32'b00010000000000000000000000000000;
		5'b11101: unqualified_cs = 32'b00100000000000000000000000000000;
		5'b11110: unqualified_cs = 32'b01000000000000000000000000000000;
		5'b11111: unqualified_cs = 32'b10000000000000000000000000000000;
	endcase

//
// SPRs System Group
//

//
// What to write into SR
//
assign to_sr[`OR1200_SR_FO:`OR1200_SR_OV] =
		(branch_op == `OR1200_BRANCHOP_RFE) ? esr[`OR1200_SR_FO:`OR1200_SR_OV] :
		(write_spr && sr_sel) ? {1'b1, spr_dat_o[`OR1200_SR_FO-1:`OR1200_SR_OV]}:
		sr[`OR1200_SR_FO:`OR1200_SR_OV];
assign to_sr[`OR1200_SR_CY] =
		(branch_op == `OR1200_BRANCHOP_RFE) ? esr[`OR1200_SR_CY] :
		cy_we ? cyforw :
		(write_spr && sr_sel) ? spr_dat_o[`OR1200_SR_CY] :
		sr[`OR1200_SR_CY];
assign to_sr[`OR1200_SR_F] =
		(branch_op == `OR1200_BRANCHOP_RFE) ? esr[`OR1200_SR_F] :
		flag_we ? flagforw :
		(write_spr && sr_sel) ? spr_dat_o[`OR1200_SR_F] :
		sr[`OR1200_SR_F];
assign to_sr[`OR1200_SR_CE:`OR1200_SR_SM] =
		(branch_op == `OR1200_BRANCHOP_RFE) ? esr[`OR1200_SR_CE:`OR1200_SR_SM] :
		(write_spr && sr_sel) ? spr_dat_o[`OR1200_SR_CE:`OR1200_SR_SM]:
		sr[`OR1200_SR_CE:`OR1200_SR_SM];

//
// Selects for system SPRs
//
assign cfgr_sel = (spr_cs[`OR1200_SPR_GROUP_SYS] && (spr_addr[10:4] == `OR1200_SPR_CFGR));
assign rf_sel = (spr_cs[`OR1200_SPR_GROUP_SYS] && (spr_addr[10:5] == `OR1200_SPR_RF));
assign npc_sel = (spr_cs[`OR1200_SPR_GROUP_SYS] && (spr_addr[10:0] == `OR1200_SPR_NPC));
assign ppc_sel = (spr_cs[`OR1200_SPR_GROUP_SYS] && (spr_addr[10:0] == `OR1200_SPR_PPC));
assign sr_sel = (spr_cs[`OR1200_SPR_GROUP_SYS] && (spr_addr[10:0] == `OR1200_SPR_SR));
assign epcr_sel = (spr_cs[`OR1200_SPR_GROUP_SYS] && (spr_addr[10:0] == `OR1200_SPR_EPCR));
assign eear_sel = (spr_cs[`OR1200_SPR_GROUP_SYS] && (spr_addr[10:0] == `OR1200_SPR_EEAR));
assign esr_sel = (spr_cs[`OR1200_SPR_GROUP_SYS] && (spr_addr[10:0] == `OR1200_SPR_ESR));

//
// Write enables for system SPRs
//
assign sr_we = (write_spr && sr_sel) | (branch_op == `OR1200_BRANCHOP_RFE) | flag_we | cy_we;
assign pc_we = (write_spr && (npc_sel | ppc_sel));
assign epcr_we = (write_spr && epcr_sel);
assign eear_we = (write_spr && eear_sel);
assign esr_we = (write_spr && esr_sel);

//
// Output from system SPRs
//
assign sys_data = (spr_dat_cfgr & {{read_spr & cfgr_sel}}) |
		  (spr_dat_rf & {{read_spr & rf_sel}}) |
		  (spr_dat_npc & {{read_spr & npc_sel}}) |
		  (spr_dat_ppc & {{read_spr & ppc_sel}}) |
		  ({{{16'b0000000000000000}},sr} & {{read_spr & sr_sel}}) |
		  (epcr & {{read_spr & epcr_sel}}) |
		  (eear & {{read_spr & eear_sel}}) |
		  ({{{16'b0000000000000000}},esr} & {{read_spr & esr_sel}});

//
// Flag alias
//
assign flag = sr[`OR1200_SR_F];

//
// Carry alias
//
assign carry = sr[`OR1200_SR_CY];

//
// Supervision register
//
always @(posedge clk)
	if (rst)
		sr <=  {1'b1, `OR1200_SR_EPH_DEF, {{13'b0000000000000}}, 1'b1};
	else if (except_started) begin
		sr[`OR1200_SR_SM]  <=  1'b1;
		sr[`OR1200_SR_TEE] <=  1'b0;
		sr[`OR1200_SR_IEE] <=  1'b0;
		sr[`OR1200_SR_DME] <=  1'b0;
		sr[`OR1200_SR_IME] <=  1'b0;
	end
	else if (sr_we)
		sr <=  to_sr[`OR1200_SR_WIDTH-1:0];

//
// MTSPR/MFSPR interface
//
always @(sprs_op or spr_addr or sys_data or spr_dat_mac or spr_dat_pic or spr_dat_pm or
	spr_dat_dmmu or spr_dat_immu or spr_dat_du or spr_dat_tt) begin
	case (sprs_op)	// synopsys parallel_case
		`OR1200_ALUOP_MTSR : begin
			write_spr = 1'b1;
			read_spr = 1'b0;
			to_wbmux = 32'b00000000000000000000000000000000;
		end
		`OR1200_ALUOP_MFSR : begin
			case (spr_addr[15:11]) // synopsys parallel_case
				`OR1200_SPR_GROUP_TT:
					to_wbmux = spr_dat_tt;
				`OR1200_SPR_GROUP_PIC:
					to_wbmux = spr_dat_pic;
				`OR1200_SPR_GROUP_PM:
					to_wbmux = spr_dat_pm;
				`OR1200_SPR_GROUP_DMMU:
					to_wbmux = spr_dat_dmmu;
				`OR1200_SPR_GROUP_IMMU:
					to_wbmux = spr_dat_immu;
				`OR1200_SPR_GROUP_MAC:
					to_wbmux = spr_dat_mac;
				`OR1200_SPR_GROUP_DU:
					to_wbmux = spr_dat_du;
				`OR1200_SPR_GROUP_SYS:
					to_wbmux = sys_data;
				default:
					to_wbmux = 32'b00000000000000000000000000000000;
			endcase
			write_spr = 1'b0;
			read_spr = 1'b1;
		end
		default : begin
			write_spr = 1'b0;
			read_spr = 1'b0;
			to_wbmux = 32'b00000000000000000000000000000000;
		end
	endcase
end

endmodule





`define OR1200_NO_FREEZE	3'b000
`define OR1200_FREEZE_BYDC	3'b001
`define OR1200_FREEZE_BYMULTICYCLE	3'b010
`define OR1200_WAIT_LSU_TO_FINISH	3'b011
`define OR1200_WAIT_IC			3'b100

//
// Freeze logic (stalls CPU pipeline, ifetcher etc.)
//
module or1200_freeze(
	// Clock and reset
	clk, rst,

	// Internal i/f
	multicycle, flushpipe, extend_flush, lsu_stall, if_stall,
	lsu_unstall,  
	force_dslot_fetch, abort_ex, du_stall,  mac_stall,
	genpc_freeze, if_freeze, id_freeze, ex_freeze, wb_freeze,
	icpu_ack_i, icpu_err_i
);

//
// I/O
//
input				clk;
input				rst;
input	[`OR1200_MULTICYCLE_WIDTH-1:0]	multicycle;
input				flushpipe;
input				extend_flush;
input				lsu_stall;
input				if_stall;
input				lsu_unstall;
input				force_dslot_fetch;
input				abort_ex;
input				du_stall;
input				mac_stall;
output				genpc_freeze;
output				if_freeze;
output				id_freeze;
output				ex_freeze;
output				wb_freeze;
input				icpu_ack_i;
input				icpu_err_i;

//
// Internal wires and regs
//
wire				multicycle_freeze;
reg	[`OR1200_MULTICYCLE_WIDTH-1:0]	multicycle_cnt;
reg				flushpipe_r;

//
// Pipeline freeze
//
// Rules how to create freeze signals:
// 1. Not overwriting pipeline stages:
// Freze signals at the beginning of pipeline (such as if_freeze) can be asserted more
// often than freeze signals at the of pipeline (such as wb_freeze). In other words, wb_freeze must never
// be asserted when ex_freeze is not. ex_freeze must never be asserted when id_freeze is not etc.
//
// 2. Inserting NOPs in the middle of pipeline only if supported:
// At this time, only ex_freeze (and wb_freeze) can be deassrted when id_freeze (and if_freeze) are asserted.
// This way NOP is asserted from stage ID into EX stage.
//
//assign genpc_freeze = du_stall | flushpipe_r | lsu_stall;
assign genpc_freeze = du_stall | flushpipe_r;
assign if_freeze = id_freeze | extend_flush;
//assign id_freeze = (lsu_stall | (~lsu_unstall & if_stall) | multicycle_freeze | force_dslot_fetch) & ~flushpipe | du_stall;
assign id_freeze = (lsu_stall | (~lsu_unstall & if_stall) | multicycle_freeze | force_dslot_fetch) | du_stall | mac_stall;
assign ex_freeze = wb_freeze;
//assign wb_freeze = (lsu_stall | (~lsu_unstall & if_stall) | multicycle_freeze) & ~flushpipe | du_stall | mac_stall;
assign wb_freeze = (lsu_stall | (~lsu_unstall & if_stall) | multicycle_freeze) | du_stall | mac_stall | abort_ex;

//
// registered flushpipe
//
always @(posedge clk )
	if (rst)
		flushpipe_r <=  1'b0;
	else if (icpu_ack_i | icpu_err_i)
//	else if (!if_stall)
		flushpipe_r <=  flushpipe;
	else if (!flushpipe)
		flushpipe_r <=  1'b0;
		
//
// Multicycle freeze
//
assign multicycle_freeze = |multicycle_cnt;

//
// Multicycle counter
//
always @(posedge clk )
	if (rst)
		multicycle_cnt <=  2'b00;
	else if (|multicycle_cnt)
		multicycle_cnt <=  multicycle_cnt - 2'b01;
	else if (|multicycle & !ex_freeze)
		multicycle_cnt <=  multicycle;

endmodule




`define OR1200_EXCEPTFSM_WIDTH 3

`define OR1200_EXCEPTFSM_IDLE	3'b000
`define OR1200_EXCEPTFSM_FLU1 	3'b001
`define OR1200_EXCEPTFSM_FLU2 	3'b010
`define OR1200_EXCEPTFSM_FLU3 	3'b011
`define OR1200_EXCEPTFSM_FLU5 	3'b101
`define OR1200_EXCEPTFSM_FLU4 	3'b100

//
// Exception recognition and sequencing
//

module or1200_except(
	// Clock and reset
	clk, rst, 

	// Internal i/f
	sig_ibuserr, sig_dbuserr, sig_illegal, sig_align, sig_range, sig_dtlbmiss, sig_dmmufault,
	sig_int, sig_syscall, sig_trap, sig_itlbmiss, sig_immufault, sig_tick,
	branch_taken,icpu_ack_i, icpu_err_i, dcpu_ack_i, dcpu_err_i,
	genpc_freeze, id_freeze, ex_freeze, wb_freeze, if_stall,
	if_pc, id_pc, lr_sav, flushpipe, extend_flush, except_type, except_start,
	except_started, except_stop, ex_void,
	spr_dat_ppc, spr_dat_npc, datain, du_dsr, epcr_we, eear_we, esr_we, pc_we, epcr, eear,
	esr, lsu_addr, sr_we, to_sr, sr, abort_ex
);

//
// I/O
//
input				clk;
input				rst;
input				sig_ibuserr;
input				sig_dbuserr;
input				sig_illegal;
input				sig_align;
input				sig_range;
input				sig_dtlbmiss;
input				sig_dmmufault;
input				sig_int;
input				sig_syscall;
input				sig_trap;
input				sig_itlbmiss;
input				sig_immufault;
input				sig_tick;
input				branch_taken;
input				genpc_freeze;
input				id_freeze;
input				ex_freeze;
input				wb_freeze;
input				if_stall;
input	[31:0]			if_pc;
output	[31:0]			id_pc;
output	[31:2]			lr_sav;
input	[31:0]			datain;
input   [`OR1200_DU_DSR_WIDTH-1:0]     du_dsr;
input				epcr_we;
input				eear_we;
input				esr_we;
input				pc_we;
output	[31:0]			epcr;
output	[31:0]			eear;
output	[`OR1200_SR_WIDTH-1:0]	esr;
input	[`OR1200_SR_WIDTH-1:0]	to_sr;
input				sr_we;
input	[`OR1200_SR_WIDTH-1:0]	sr;
input	[31:0]			lsu_addr;
output				flushpipe;
output				extend_flush;
output	[`OR1200_EXCEPT_WIDTH-1:0]	except_type;
output				except_start;
output				except_started;
output	[12:0]			except_stop;
input				ex_void;
output	[31:0]			spr_dat_ppc;
output	[31:0]			spr_dat_npc;
output				abort_ex;
input				icpu_ack_i;
input				icpu_err_i;
input				dcpu_ack_i;
input				dcpu_err_i;

//
// Internal regs and wires
//
reg	[`OR1200_EXCEPT_WIDTH-1:0]	except_type;
reg	[31:0]			id_pc;
reg	[31:0]			ex_pc;
reg	[31:0]			wb_pc;
reg	[31:0]			epcr;
reg	[31:0]			eear;
reg	[`OR1200_SR_WIDTH-1:0]		esr;
reg	[2:0]			id_exceptflags;
reg	[2:0]			ex_exceptflags;
reg	[`OR1200_EXCEPTFSM_WIDTH-1:0]	state;
reg				extend_flush;
reg				extend_flush_last;
reg				ex_dslot;
reg				delayed1_ex_dslot;
reg				delayed2_ex_dslot;
wire				except_started;
wire	[12:0]			except_trig;
wire				except_flushpipe;
reg	[2:0]			delayed_iee;
reg	[2:0]			delayed_tee;
wire				int_pending;
wire				tick_pending;

//
// Simple combinatorial logic
//
assign except_started = extend_flush & except_start;
assign lr_sav = ex_pc[31:2];
assign spr_dat_ppc = wb_pc;
assign spr_dat_npc = ex_void ? id_pc : ex_pc;
assign except_start = (except_type != 4'b0000) & extend_flush;
assign int_pending = sig_int & sr[`OR1200_SR_IEE] & delayed_iee[2] & ~ex_freeze & ~branch_taken & ~ex_dslot & ~sr_we;
assign tick_pending = sig_tick & sr[`OR1200_SR_TEE] & ~ex_freeze & ~branch_taken & ~ex_dslot & ~sr_we;
assign abort_ex = sig_dbuserr | sig_dmmufault | sig_dtlbmiss | sig_align | sig_illegal;		// Abort write into RF by load & other instructions

//
// Order defines exception detection priority
//
assign except_trig = {
			tick_pending		& ~du_dsr[`OR1200_DU_DSR_TTE],
			int_pending 		& ~du_dsr[`OR1200_DU_DSR_IE],
			ex_exceptflags[1]	& ~du_dsr[`OR1200_DU_DSR_IME],
			ex_exceptflags[0]	& ~du_dsr[`OR1200_DU_DSR_IPFE],
			ex_exceptflags[2]	& ~du_dsr[`OR1200_DU_DSR_BUSEE],
			sig_illegal		& ~du_dsr[`OR1200_DU_DSR_IIE],
			sig_align		& ~du_dsr[`OR1200_DU_DSR_AE],
			sig_dtlbmiss		& ~du_dsr[`OR1200_DU_DSR_DME],
			sig_dmmufault		& ~du_dsr[`OR1200_DU_DSR_DPFE],
			sig_dbuserr		& ~du_dsr[`OR1200_DU_DSR_BUSEE],
			sig_range		& ~du_dsr[`OR1200_DU_DSR_RE],
			sig_trap		& ~du_dsr[`OR1200_DU_DSR_TE] & ~ex_freeze,
			sig_syscall		& ~du_dsr[`OR1200_DU_DSR_SCE] & ~ex_freeze
		};
assign except_stop = {
			tick_pending		& du_dsr[`OR1200_DU_DSR_TTE],
			int_pending 		& du_dsr[`OR1200_DU_DSR_IE],
			ex_exceptflags[1]	& du_dsr[`OR1200_DU_DSR_IME],
			ex_exceptflags[0]	& du_dsr[`OR1200_DU_DSR_IPFE],
			ex_exceptflags[2]	& du_dsr[`OR1200_DU_DSR_BUSEE],
			sig_illegal		& du_dsr[`OR1200_DU_DSR_IIE],
			sig_align		& du_dsr[`OR1200_DU_DSR_AE],
			sig_dtlbmiss		& du_dsr[`OR1200_DU_DSR_DME],
			sig_dmmufault		& du_dsr[`OR1200_DU_DSR_DPFE],
			sig_dbuserr		& du_dsr[`OR1200_DU_DSR_BUSEE],
			sig_range		& du_dsr[`OR1200_DU_DSR_RE],
			sig_trap		& du_dsr[`OR1200_DU_DSR_TE] & ~ex_freeze,
			sig_syscall		& du_dsr[`OR1200_DU_DSR_SCE] & ~ex_freeze
		};

//
// PC and Exception flags pipelines
//
always @(posedge clk ) begin
	if (rst) begin
		id_pc <=  32'b00000000000000000000000000000000;
		id_exceptflags <=  3'b000;
	end
	else if (flushpipe) begin
		id_pc <=  32'h00000000;
		id_exceptflags <=  3'b000;
	end
	else if (!id_freeze) begin
		id_pc <=  if_pc;
		id_exceptflags <=  { sig_ibuserr, sig_itlbmiss, sig_immufault };
	end
end

//
// delayed_iee
//
// SR[IEE] should not enable interrupts right away
// when it is restored with l.rfe. Instead delayed_iee
// together with SR[IEE] enables interrupts once
// pipeline is again ready.
//
always @(posedge clk)
	if (rst)
		delayed_iee <=  3'b000;
	else if (!sr[`OR1200_SR_IEE])
		delayed_iee <=  3'b000;
	else
		delayed_iee <=  {delayed_iee[1:0], 1'b1};

//
// delayed_tee
//
// SR[TEE] should not enable tick exceptions right away
// when it is restored with l.rfe. Instead delayed_tee
// together with SR[TEE] enables tick exceptions once
// pipeline is again ready.
//
always @( posedge clk)
	if (rst)
		delayed_tee <=  3'b000;
	else if (!sr[`OR1200_SR_TEE])
		delayed_tee <=  3'b000;
	else
		delayed_tee <=  {delayed_tee[1:0], 1'b1};

//
// PC and Exception flags pipelines
//
always @(posedge clk ) begin
	if (rst) begin
		ex_dslot <=  1'b0;
		ex_pc <=  32'd0;
		ex_exceptflags <=  3'b000;
		delayed1_ex_dslot <=  1'b0;
		delayed2_ex_dslot <=  1'b0;
	end
	else if (flushpipe) begin
		ex_dslot <=  1'b0;
		ex_pc <=  32'h00000000;
		ex_exceptflags <=  3'b000;
		delayed1_ex_dslot <=  1'b0;
		delayed2_ex_dslot <=  1'b0;
	end
	else if (!ex_freeze & id_freeze) begin
		ex_dslot <=  1'b0;
		ex_pc <=  id_pc;
		ex_exceptflags <=  3'b000;
		delayed1_ex_dslot <=  ex_dslot;
		delayed2_ex_dslot <=  delayed1_ex_dslot;
	end
	else if (!ex_freeze) begin
		ex_dslot <=  branch_taken;
		ex_pc <=  id_pc;
		ex_exceptflags <=  id_exceptflags;
		delayed1_ex_dslot <=  ex_dslot;
		delayed2_ex_dslot <=  delayed1_ex_dslot;
	end
end

//
// PC and Exception flags pipelines
//
always @(posedge clk ) begin
	if (rst) begin
		wb_pc <=  32'b00000000000000000000000000000000;
	end
	else if (!wb_freeze) begin
		wb_pc <=  ex_pc;
	end
end

//
// Flush pipeline
//
assign flushpipe = except_flushpipe | pc_we | extend_flush;

//
// We have started execution of exception handler:
//  1. Asserted for 3 clock cycles
//  2. Don't execute any instruction that is still in pipeline and is not part of exception handler
//
assign except_flushpipe = |except_trig & ~|state;

//
// Exception FSM that sequences execution of exception handler
//
// except_type signals which exception handler we start fetching in:
//  1. Asserted in next clock cycle after exception is recognized
//
always @(posedge clk ) begin
	if (rst) begin
		state <=  `OR1200_EXCEPTFSM_IDLE;
		except_type <=  4'b0000;
		extend_flush <=  1'b0;
		epcr <=  32'b00000000000000000000000000000000;
		eear <=  32'b00000000000000000000000000000000;
		esr <=  {{1'b1, 1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0},{1'b0}, {1'b1}};
		extend_flush_last <=  1'b0;
	end
	else begin

		case (state)
			`OR1200_EXCEPTFSM_IDLE:
				if (except_flushpipe) begin
					state <=  `OR1200_EXCEPTFSM_FLU1;
					extend_flush <=  1'b1;
					esr <=  sr_we ? to_sr : sr;
					
					if (except_trig[12] == 1)
					begin
						except_type <=  `OR1200_EXCEPT_TICK;
					   epcr <=  ex_dslot ? wb_pc : delayed1_ex_dslot ? id_pc : delayed2_ex_dslot ? id_pc : id_pc;
					end
					else if (except_trig[12] == 0 && except_trig[11] == 0)
					begin
						except_type <=  `OR1200_EXCEPT_INT;
						epcr <=  ex_dslot ? wb_pc : delayed1_ex_dslot ? id_pc : delayed2_ex_dslot ? id_pc : id_pc;
					end
					else if (except_trig[12] == 0 && except_trig[11] == 0 && except_trig[10] == 1)
					begin
						except_type <=  `OR1200_EXCEPT_ITLBMISS;
//
// itlb miss exception and active ex_dslot caused wb_pc to put into eear instead of +4 address of ex_pc (or id_pc since it was equal to ex_pc?)
//							eear <=  ex_dslot ? wb_pc : delayed1_ex_dslot ? id_pc : delayed2_ex_dslot ? id_pc : id_pc;
//	mmu-icdc-O2 ex_pc only OK when no ex_dslot	eear <=  ex_dslot ? ex_pc : delayed1_ex_dslot ? id_pc : delayed2_ex_dslot ? id_pc : id_pc;
//	mmu-icdc-O2 ex_pc only OK when no ex_dslot	epcr <=  ex_dslot ? wb_pc : delayed1_ex_dslot ? id_pc : delayed2_ex_dslot ? id_pc : id_pc;
						eear <=  ex_dslot ? ex_pc : ex_pc;
						epcr <=  ex_dslot ? wb_pc : ex_pc;
//					 	eear <=  ex_dslot ? ex_pc : delayed1_ex_dslot ? id_pc : delayed2_ex_dslot ? id_pc : id_pc;
//							epcr <=  ex_dslot ? wb_pc : delayed1_ex_dslot ? id_pc : delayed2_ex_dslot ? id_pc : id_pc;
					end
					else
					begin  
						except_type <=  4'b0000;
					end					
				end
				else if (pc_we) begin
					state <=  `OR1200_EXCEPTFSM_FLU1;
					extend_flush <=  1'b1;
				end
				else begin
					if (epcr_we)
						epcr <=  datain;
					if (eear_we)
						eear <=  datain;
					if (esr_we)
						esr <=  {1'b1, datain[`OR1200_SR_WIDTH-2:0]};
				end
			`OR1200_EXCEPTFSM_FLU1:
				if (icpu_ack_i | icpu_err_i | genpc_freeze)
					state <=  `OR1200_EXCEPTFSM_FLU2;
			`OR1200_EXCEPTFSM_FLU2:
					state <=  `OR1200_EXCEPTFSM_FLU3;
			`OR1200_EXCEPTFSM_FLU3:
					begin
						state <=  `OR1200_EXCEPTFSM_FLU4;
					end
			`OR1200_EXCEPTFSM_FLU4: begin
					state <=  `OR1200_EXCEPTFSM_FLU5;
					extend_flush <=  1'b0;
					extend_flush_last <=  1'b0; // damjan
				end

			default: begin
				if (!if_stall && !id_freeze) begin
					state <=  `OR1200_EXCEPTFSM_IDLE;
					except_type <=  4'b0000;
					extend_flush_last <=  1'b0;
				end
			end
		endcase
	end
end

wire unused;
assign unused = sig_range | sig_syscall | sig_trap | dcpu_ack_i| dcpu_err_i | du_dsr | lsu_addr;
endmodule





module or1200_cfgr(
	// RISC Internal Interface
	spr_addr, spr_dat_o
);

//
// RISC Internal Interface
//
input	[31:0]	spr_addr;	// SPR Address
output	[31:0]	spr_dat_o;	// SPR Read Data

//
// Internal wires & registers
//
reg	[31:0]	spr_dat_o;	// SPR Read Data



//
// Implementation of VR, UPR and configuration registers
//
always @(spr_addr)
	if (~|spr_addr[31:4])

		case(spr_addr[3:0])		// synopsys parallel_case
			`OR1200_SPRGRP_SYS_VR: begin
				spr_dat_o[5:0] = `OR1200_VR_REV;
				spr_dat_o[16:6] = `OR1200_VR_RES1;
				spr_dat_o[23:17] = `OR1200_VR_CFG;
				spr_dat_o[31:24] = `OR1200_VR_VER;
			end
			`OR1200_SPRGRP_SYS_UPR: begin
				spr_dat_o[`OR1200_UPR_UP_BITS] = `OR1200_UPR_UP;
				spr_dat_o[`OR1200_UPR_DCP_BITS] = `OR1200_UPR_DCP;
				spr_dat_o[`OR1200_UPR_ICP_BITS] = `OR1200_UPR_ICP;
				spr_dat_o[`OR1200_UPR_DMP_BITS] = `OR1200_UPR_DMP;
				spr_dat_o[`OR1200_UPR_IMP_BITS] = `OR1200_UPR_IMP;
				spr_dat_o[`OR1200_UPR_MP_BITS] = `OR1200_UPR_MP;
				spr_dat_o[`OR1200_UPR_DUP_BITS] = `OR1200_UPR_DUP;
				spr_dat_o[`OR1200_UPR_PCUP_BITS] = `OR1200_UPR_PCUP;
				spr_dat_o[`OR1200_UPR_PMP_BITS] = `OR1200_UPR_PMP;
				spr_dat_o[`OR1200_UPR_PICP_BITS] = `OR1200_UPR_PICP;
				spr_dat_o[`OR1200_UPR_TTP_BITS] = `OR1200_UPR_TTP;
				spr_dat_o[23:11] = `OR1200_UPR_RES1;
				spr_dat_o[31:24] = `OR1200_UPR_CUP;
			end
			`OR1200_SPRGRP_SYS_CPUCFGR: begin
				spr_dat_o[3:0] = `OR1200_CPUCFGR_NSGF;
				spr_dat_o[`OR1200_CPUCFGR_HGF_BITS] = `OR1200_CPUCFGR_HGF;
				spr_dat_o[`OR1200_CPUCFGR_OB32S_BITS] = `OR1200_CPUCFGR_OB32S;
				spr_dat_o[`OR1200_CPUCFGR_OB64S_BITS] = `OR1200_CPUCFGR_OB64S;
				spr_dat_o[`OR1200_CPUCFGR_OF32S_BITS] = `OR1200_CPUCFGR_OF32S;
				spr_dat_o[`OR1200_CPUCFGR_OF64S_BITS] = `OR1200_CPUCFGR_OF64S;
				spr_dat_o[`OR1200_CPUCFGR_OV64S_BITS] = `OR1200_CPUCFGR_OV64S;
				spr_dat_o[31:10] = `OR1200_CPUCFGR_RES1;
			end
			`OR1200_SPRGRP_SYS_DMMUCFGR: begin
				spr_dat_o[1:0] = `OR1200_DMMUCFGR_NTW;
				spr_dat_o[4:2] = `OR1200_DMMUCFGR_NTS;
				spr_dat_o[7:5] = `OR1200_DMMUCFGR_NAE;
				spr_dat_o[`OR1200_DMMUCFGR_CRI_BITS] = `OR1200_DMMUCFGR_CRI;
				spr_dat_o[`OR1200_DMMUCFGR_PRI_BITS] = `OR1200_DMMUCFGR_PRI;
				spr_dat_o[`OR1200_DMMUCFGR_TEIRI_BITS] = `OR1200_DMMUCFGR_TEIRI;
				spr_dat_o[`OR1200_DMMUCFGR_HTR_BITS] = `OR1200_DMMUCFGR_HTR;
				spr_dat_o[31:12] = `OR1200_DMMUCFGR_RES1;
			end
			`OR1200_SPRGRP_SYS_IMMUCFGR: begin
				spr_dat_o[1:0] = `OR1200_IMMUCFGR_NTW;
				spr_dat_o[4:2] = `OR1200_IMMUCFGR_NTS;
				spr_dat_o[7:5] = `OR1200_IMMUCFGR_NAE;
				spr_dat_o[`OR1200_IMMUCFGR_CRI_BITS] = `OR1200_IMMUCFGR_CRI;
				spr_dat_o[`OR1200_IMMUCFGR_PRI_BITS] = `OR1200_IMMUCFGR_PRI;
				spr_dat_o[`OR1200_IMMUCFGR_TEIRI_BITS] = `OR1200_IMMUCFGR_TEIRI;
				spr_dat_o[`OR1200_IMMUCFGR_HTR_BITS] = `OR1200_IMMUCFGR_HTR;
				spr_dat_o[31:12] = `OR1200_IMMUCFGR_RES1;
			end
			`OR1200_SPRGRP_SYS_DCCFGR: begin
				spr_dat_o[2:0] = `OR1200_DCCFGR_NCW;
				spr_dat_o[6:3] = `OR1200_DCCFGR_NCS;
				spr_dat_o[`OR1200_DCCFGR_CBS_BITS] = `OR1200_DCCFGR_CBS;
				spr_dat_o[`OR1200_DCCFGR_CWS_BITS] = `OR1200_DCCFGR_CWS;
				spr_dat_o[`OR1200_DCCFGR_CCRI_BITS] = `OR1200_DCCFGR_CCRI;
				spr_dat_o[`OR1200_DCCFGR_CBIRI_BITS] = `OR1200_DCCFGR_CBIRI;
				spr_dat_o[`OR1200_DCCFGR_CBPRI_BITS] = `OR1200_DCCFGR_CBPRI;
				spr_dat_o[`OR1200_DCCFGR_CBLRI_BITS] = `OR1200_DCCFGR_CBLRI;
				spr_dat_o[`OR1200_DCCFGR_CBFRI_BITS] = `OR1200_DCCFGR_CBFRI;
				spr_dat_o[`OR1200_DCCFGR_CBWBRI_BITS] = `OR1200_DCCFGR_CBWBRI;
				spr_dat_o[31:15] = `OR1200_DCCFGR_RES1;
			end
			`OR1200_SPRGRP_SYS_ICCFGR: begin
				spr_dat_o[2:0] = `OR1200_ICCFGR_NCW;
				spr_dat_o[6:3] = `OR1200_ICCFGR_NCS;
				spr_dat_o[`OR1200_ICCFGR_CBS_BITS] = `OR1200_ICCFGR_CBS;
				spr_dat_o[`OR1200_ICCFGR_CWS_BITS] = `OR1200_ICCFGR_CWS;
				spr_dat_o[`OR1200_ICCFGR_CCRI_BITS] = `OR1200_ICCFGR_CCRI;
				spr_dat_o[`OR1200_ICCFGR_CBIRI_BITS] = `OR1200_ICCFGR_CBIRI;
				spr_dat_o[`OR1200_ICCFGR_CBPRI_BITS] = `OR1200_ICCFGR_CBPRI;
				spr_dat_o[`OR1200_ICCFGR_CBLRI_BITS] = `OR1200_ICCFGR_CBLRI;
				spr_dat_o[`OR1200_ICCFGR_CBFRI_BITS] = `OR1200_ICCFGR_CBFRI;
				spr_dat_o[`OR1200_ICCFGR_CBWBRI_BITS] = `OR1200_ICCFGR_CBWBRI;
				spr_dat_o[31:15] = `OR1200_ICCFGR_RES1;
			end
			`OR1200_SPRGRP_SYS_DCFGR: begin
				spr_dat_o[2:0] = `OR1200_DCFGR_NDP;
				spr_dat_o[3] = `OR1200_DCFGR_WPCI;
				spr_dat_o[31:4] = `OR1200_DCFGR_RES1;
			end
			default: spr_dat_o = 32'h00000000;
		endcase
		


//

endmodule

module or1200_wbmux(
	// Clock and reset
	clk, rst,

	// Internal i/f
	wb_freeze, rfwb_op,
	muxin_a, muxin_b, muxin_c, muxin_d,
	muxout, muxreg, muxreg_valid
);

//parameter width = `OR1200_OPERAND_WIDTH;

//
// I/O
//

//
// Clock and reset
//
input				clk;
input				rst;

//
// Internal i/f
//
input				wb_freeze;
input	[`OR1200_RFWBOP_WIDTH-1:0]	rfwb_op;
input	[32-1:0]		muxin_a;
input	[32-1:0]		muxin_b;
input	[32-1:0]		muxin_c;
input	[32-1:0]		muxin_d;
output	[32-1:0]		muxout;
output	[32-1:0]		muxreg;
output				muxreg_valid;

//
// Internal wires and regs
//
reg	[32-1:0]		muxout;
reg	[32-1:0]		muxreg;
reg				muxreg_valid;

//
// Registered output from the write-back multiplexer
//
always @(posedge clk) begin
	if (rst) begin
		muxreg <=  32'b00000000000000000000000000000000;
		muxreg_valid <=  1'b0;
	end
	else if (!wb_freeze) begin
		muxreg <=  muxout;
		muxreg_valid <=  rfwb_op[0];
	end
end

//
// Write-back multiplexer
//
always @(muxin_a or muxin_b or muxin_c or muxin_d or rfwb_op) begin
	case(rfwb_op[`OR1200_RFWBOP_WIDTH-1:1]) 
		2'b00: muxout = muxin_a;
		2'b01: begin
			muxout = muxin_b;
		end
		2'b10: begin
			muxout = muxin_c;
		end
		2'b11: begin
			muxout = muxin_d + 32'b00000000000000000000000000001000;
		end
	endcase
end

endmodule



module or1200_lsu(

	// Internal i/f
	addrbase, addrofs, lsu_op, lsu_datain, lsu_dataout, lsu_stall, lsu_unstall,
        du_stall, except_align, except_dtlbmiss, except_dmmufault, except_dbuserr,

	// External i/f to DC
	dcpu_adr_o, dcpu_cycstb_o, dcpu_we_o, dcpu_sel_o, dcpu_tag_o, dcpu_dat_o,
	dcpu_dat_i, dcpu_ack_i, dcpu_rty_i, dcpu_err_i, dcpu_tag_i
);

//parameter dw = `OR1200_OPERAND_WIDTH;
//parameter aw = `OR1200_REGFILE_ADDR_WIDTH;

//
// I/O
//

//
// Internal i/f
//
input	[31:0]			addrbase;
input	[31:0]			addrofs;
input	[`OR1200_LSUOP_WIDTH-1:0]	lsu_op;
input	[`OR1200_OPERAND_WIDTH-1:0]		lsu_datain;
output	[`OR1200_OPERAND_WIDTH-1:0]		lsu_dataout;
output				lsu_stall;
output				lsu_unstall;
input                           du_stall;
output				except_align;
output				except_dtlbmiss;
output				except_dmmufault;
output				except_dbuserr;

//
// External i/f to DC
//
output	[31:0]			dcpu_adr_o;
output				dcpu_cycstb_o;
output				dcpu_we_o;
output	[3:0]			dcpu_sel_o;
output	[3:0]			dcpu_tag_o;
output	[31:0]			dcpu_dat_o;
input	[31:0]			dcpu_dat_i;
input				dcpu_ack_i;
input				dcpu_rty_i;
input				dcpu_err_i;
input	[3:0]			dcpu_tag_i;

//
// Internal wires/regs
//
reg	[3:0]			dcpu_sel_o;

//
// Internal I/F assignments
//
assign lsu_stall = dcpu_rty_i & dcpu_cycstb_o;
assign lsu_unstall = dcpu_ack_i;
assign except_align = ((lsu_op == `OR1200_LSUOP_SH) | (lsu_op == `OR1200_LSUOP_LHZ) | (lsu_op == `OR1200_LSUOP_LHS)) & dcpu_adr_o[0]
		|  ((lsu_op == `OR1200_LSUOP_SW) | (lsu_op == `OR1200_LSUOP_LWZ) | (lsu_op == `OR1200_LSUOP_LWS)) & |dcpu_adr_o[1:0];
assign except_dtlbmiss = dcpu_err_i & (dcpu_tag_i == `OR1200_DTAG_TE);
assign except_dmmufault = dcpu_err_i & (dcpu_tag_i == `OR1200_DTAG_PE);
assign except_dbuserr = dcpu_err_i & (dcpu_tag_i == `OR1200_DTAG_BE);

//
// External I/F assignments
//
assign dcpu_adr_o = addrbase + addrofs;
assign dcpu_cycstb_o = du_stall | lsu_unstall | except_align ? 1'b0 : |lsu_op;
assign dcpu_we_o = lsu_op[3];
assign dcpu_tag_o = dcpu_cycstb_o ? `OR1200_DTAG_ND : `OR1200_DTAG_IDLE;
always @(lsu_op or dcpu_adr_o)
	case({lsu_op, dcpu_adr_o[1:0]})
		{`OR1200_LSUOP_SB, 2'b00} : dcpu_sel_o = 4'b1000;
		{`OR1200_LSUOP_SB, 2'b01} : dcpu_sel_o = 4'b0100;
		{`OR1200_LSUOP_SB, 2'b10} : dcpu_sel_o = 4'b0010;
		{`OR1200_LSUOP_SB, 2'b11} : dcpu_sel_o = 4'b0001;
		{`OR1200_LSUOP_SH, 2'b00} : dcpu_sel_o = 4'b1100;
		{`OR1200_LSUOP_SH, 2'b10} : dcpu_sel_o = 4'b0011;
		{`OR1200_LSUOP_SW, 2'b00} : dcpu_sel_o = 4'b1111;
		{`OR1200_LSUOP_LBZ, 2'b00} : dcpu_sel_o = 4'b1000;
		{`OR1200_LSUOP_LBS, 2'b00} : dcpu_sel_o = 4'b1000;
		{`OR1200_LSUOP_LBZ, 2'b01}: dcpu_sel_o = 4'b0100;
		{`OR1200_LSUOP_LBS, 2'b01} : dcpu_sel_o = 4'b0100;
		{`OR1200_LSUOP_LBZ, 2'b10}: dcpu_sel_o = 4'b0010;
		{`OR1200_LSUOP_LBS, 2'b10} : dcpu_sel_o = 4'b0010;
		{`OR1200_LSUOP_LBZ, 2'b11}: dcpu_sel_o = 4'b0001;
		{`OR1200_LSUOP_LBS, 2'b11} : dcpu_sel_o = 4'b0001;
		{`OR1200_LSUOP_LHZ, 2'b00}: dcpu_sel_o = 4'b1100;
		{`OR1200_LSUOP_LHS, 2'b00} : dcpu_sel_o = 4'b1100;
		{`OR1200_LSUOP_LHZ, 2'b10}: dcpu_sel_o = 4'b0011;
		{`OR1200_LSUOP_LHS, 2'b10} : dcpu_sel_o = 4'b0011;
		{`OR1200_LSUOP_LWZ, 2'b00}: dcpu_sel_o = 4'b1111;
		{4'b1111, 2'b00} : dcpu_sel_o = 4'b1111;
		default : dcpu_sel_o = 4'b0000;
	endcase

//
// Instantiation of Memory-to-regfile aligner
//
or1200_mem2reg or1200_mem2reg(
	.addr(dcpu_adr_o[1:0]),
	.lsu_op(lsu_op),
	.memdata(dcpu_dat_i),
	.regdata(lsu_dataout)
);

//
// Instantiation of Regfile-to-memory aligner
//
or1200_reg2mem or1200_reg2mem(
        .addr(dcpu_adr_o[1:0]),
        .lsu_op(lsu_op),
        .regdata(lsu_datain),
        .memdata(dcpu_dat_o)
);

endmodule




module or1200_reg2mem(addr, lsu_op, regdata, memdata);

//parameter width = `OR1200_OPERAND_WIDTH;

//
// I/O
//
input	[1:0]			addr;
input	[`OR1200_LSUOP_WIDTH-1:0]	lsu_op;
input	[32-1:0]		regdata;
output	[32-1:0]		memdata;

//
// Internal regs and wires
//
reg	[7:0]			memdata_hh;
reg	[7:0]			memdata_hl;
reg	[7:0]			memdata_lh;
reg	[7:0]			memdata_ll;

assign memdata = {memdata_hh, memdata_hl, memdata_lh, memdata_ll};

//
// Mux to memdata[31:24]
//
always @(lsu_op or addr or regdata) begin
	case({lsu_op, addr[1:0]})	// synopsys parallel_case
		{`OR1200_LSUOP_SB, 2'b00} : memdata_hh = regdata[7:0];
		{`OR1200_LSUOP_SH, 2'b00} : memdata_hh = regdata[15:8];
		default : memdata_hh = regdata[31:24];
	endcase
end

//
// Mux to memdata[23:16]
//
always @(lsu_op or addr or regdata) begin
	case({lsu_op, addr[1:0]})	// synopsys parallel_case
		{`OR1200_LSUOP_SW, 2'b00} : memdata_hl = regdata[23:16];
		default : memdata_hl = regdata[7:0];
	endcase
end

//
// Mux to memdata[15:8]
//
always @(lsu_op or addr or regdata) begin
	case({lsu_op, addr[1:0]})	// synopsys parallel_case
		{`OR1200_LSUOP_SB, 2'b10} : memdata_lh = regdata[7:0];
		default : memdata_lh = regdata[15:8];
	endcase
end

//
// Mux to memdata[7:0]
//
always @(regdata)
	memdata_ll = regdata[7:0];

endmodule



module or1200_mem2reg(addr, lsu_op, memdata, regdata);

//parameter width = `OR1200_OPERAND_WIDTH;

//
// I/O
//
input	[1:0]			addr;
input	[`OR1200_LSUOP_WIDTH-1:0]	lsu_op;
input	[32-1:0]		memdata;
output	[32-1:0]		regdata;
wire	[32-1:0]		regdata;

//
// In the past faster implementation of mem2reg (today probably slower)
//
reg	[7:0]			regdata_hh;
reg	[7:0]			regdata_hl;
reg	[7:0]			regdata_lh;
reg	[7:0]			regdata_ll;
reg	[32-1:0]		aligned;
reg	[3:0]			sel_byte0, sel_byte1,
				sel_byte2, sel_byte3;

assign regdata = {regdata_hh, regdata_hl, regdata_lh, regdata_ll};

//
// Byte select 0
//
always @(addr or lsu_op) begin
	case({lsu_op[2:0], addr})	// synopsys parallel_case
		{3'b011, 2'b00}:			// lbz/lbs 0
			sel_byte0 = `OR1200_M2R_BYTE3;	// take byte 3
		{3'b011, 2'b01}:	
sel_byte0 = `OR1200_M2R_BYTE2;		
		{3'b101, 2'b00}:			// lhz/lhs 0
			sel_byte0 = `OR1200_M2R_BYTE2;	// take byte 2
		{3'b011, 2'b10}:			// lbz/lbs 2
			sel_byte0 = `OR1200_M2R_BYTE1;	// take byte 1
		default:				// all other cases
			sel_byte0 = `OR1200_M2R_BYTE0;	// take byte 0
	endcase
end

//
// Byte select 1
//
always @(addr or lsu_op) begin
	case({lsu_op[2:0], addr})	// synopsys parallel_case
		{3'b010, 2'b00}:			// lbz
			sel_byte1 = `OR1200_M2R_ZERO;	// zero extend
		{3'b011, 2'b00}:			// lbs 0
			sel_byte1 = `OR1200_M2R_EXTB3;	// sign extend from byte 3
		{3'b011, 2'b01}:			// lbs 1
			sel_byte1 = `OR1200_M2R_EXTB2;	// sign extend from byte 2
		{3'b011, 2'b10}:			// lbs 2
			sel_byte1 = `OR1200_M2R_EXTB1;	// sign extend from byte 1
		{3'b011, 2'b11}:			// lbs 3
			sel_byte1 = `OR1200_M2R_EXTB0;	// sign extend from byte 0
		{3'b100, 2'b00}:			// lhz/lhs 0
			sel_byte1 = `OR1200_M2R_BYTE3;	// take byte 3
		default:				// all other cases
			sel_byte1 = `OR1200_M2R_BYTE1;	// take byte 1
	endcase
end

//
// Byte select 2
//
always @(addr or lsu_op) begin
	case({lsu_op[2:0], addr})	// synopsys parallel_case
		{3'b010, 2'b00}:	
sel_byte2 = `OR1200_M2R_ZERO;			// lbz
		{3'b100, 2'b00}:			// lhz
			sel_byte2 = `OR1200_M2R_ZERO;	// zero extend
		{3'b011, 2'b00}:	
			sel_byte2 = `OR1200_M2R_EXTB3;	// sign extend from byte 3	
		{3'b101, 2'b00}:			// lhs 0
			sel_byte2 = `OR1200_M2R_EXTB3;	// sign extend from byte 3
		{3'b011, 2'b01}:			// lbs 1
			sel_byte2 = `OR1200_M2R_EXTB2;	// sign extend from byte 2
		{3'b011, 2'b10}:	
	sel_byte2 = `OR1200_M2R_EXTB1;	
		{3'b101, 2'b10}:			// lhs 0
			sel_byte2 = `OR1200_M2R_EXTB1;	// sign extend from byte 1
		{3'b011, 2'b11}:			// lbs 3
			sel_byte2 = `OR1200_M2R_EXTB0;	// sign extend from byte 0
		default:				// all other cases
			sel_byte2 = `OR1200_M2R_BYTE2;	// take byte 2
	endcase
end

//
// Byte select 3
//
always @(addr or lsu_op) begin
	case({lsu_op[2:0], addr}) // synopsys parallel_case
		{3'b010, 2'b00}:
			sel_byte3 = `OR1200_M2R_ZERO;	// zero extend		// lbz
		{3'b100, 2'b00}:			// lhz
			sel_byte3 = `OR1200_M2R_ZERO;	// zero extend
		{3'b011, 2'b00}:
sel_byte3 = `OR1200_M2R_EXTB3;	
		{3'b101, 2'b00}:			// lhs 0
			sel_byte3 = `OR1200_M2R_EXTB3;	// sign extend from byte 3
		{3'b011, 2'b01}:			// lbs 1
			sel_byte3 = `OR1200_M2R_EXTB2;	// sign extend from byte 2
		{3'b011, 2'b10}:
			sel_byte3 = `OR1200_M2R_EXTB1;	
		{3'b101, 2'b10}:			// lhs 0
			sel_byte3 = `OR1200_M2R_EXTB1;	// sign extend from byte 1
		{3'b011, 2'b11}:			// lbs 3
			sel_byte3 = `OR1200_M2R_EXTB0;	// sign extend from byte 0
		default:				// all other cases
			sel_byte3 = `OR1200_M2R_BYTE3;	// take byte 3
	endcase
end

//
// Byte 0
//
always @(sel_byte0 or memdata)
 begin
		case(sel_byte0)
		`OR1200_M2R_BYTE0: begin
				regdata_ll = memdata[7:0];
			end
		`OR1200_M2R_BYTE1: begin
				regdata_ll = memdata[15:8];
			end
		`OR1200_M2R_BYTE2: begin
				regdata_ll = memdata[23:16];
			end

		default: begin
		
				regdata_ll = memdata[31:24];
			end
	endcase
end

//
// Byte 1
//
always @(sel_byte1 or memdata) begin

	case(sel_byte1) 

		`OR1200_M2R_ZERO: begin
				regdata_lh = 8'h00;
			end
		`OR1200_M2R_BYTE1: begin
				regdata_lh = memdata[15:8];
			end
		`OR1200_M2R_BYTE3: begin
				regdata_lh = memdata[31:24];
			end
		`OR1200_M2R_EXTB0: begin
				regdata_lh = {{memdata[7]},{memdata[7]},{memdata[7]},{memdata[7]},{memdata[7]},{memdata[7]},{memdata[7]},{memdata[7]}};
			end
		`OR1200_M2R_EXTB1: begin
				regdata_lh = {{memdata[15]},{memdata[15]},{memdata[15]},{memdata[15]},{memdata[15]},{memdata[15]},{memdata[15]},{memdata[15]}};
			end
		`OR1200_M2R_EXTB2: begin
				regdata_lh = {{memdata[23]},{memdata[23]},{memdata[23]},{memdata[23]},{memdata[23]},{memdata[23]},{memdata[23]},{memdata[23]}};
			end
		default: begin

				regdata_lh = {{memdata[31]},{memdata[31]},{memdata[31]},{memdata[31]},{memdata[31]},{memdata[31]},{memdata[31]},{memdata[31]}};
			end
	endcase
end

//
// Byte 2
//
always @(sel_byte2 or memdata) begin


	case(sel_byte2) 

		`OR1200_M2R_ZERO: begin
				regdata_hl = 8'h00;
			end
		`OR1200_M2R_BYTE2: begin
				regdata_hl = memdata[23:16];
			end
		`OR1200_M2R_EXTB0: begin
				regdata_hl = {{memdata[7]},{memdata[7]},{memdata[7]},{memdata[7]},{memdata[7]},{memdata[7]},{memdata[7]},{memdata[7]}};
			end
		`OR1200_M2R_EXTB1: begin
				regdata_hl =  {{memdata[15]},{memdata[15]},{memdata[15]},{memdata[15]},{memdata[15]},{memdata[15]},{memdata[15]},{memdata[15]}};
			end
		`OR1200_M2R_EXTB2: begin
				regdata_hl = {{memdata[23]},{memdata[23]},{memdata[23]},{memdata[23]},{memdata[23]},{memdata[23]},{memdata[23]},{memdata[23]}};
			end
		default: begin
				regdata_hl = {{memdata[31]},{memdata[31]},{memdata[31]},{memdata[31]},{memdata[31]},{memdata[31]},{memdata[31]},{memdata[31]}};
			end
	endcase
end

//
// Byte 3
//
always @(sel_byte3 or memdata) begin

	case(sel_byte3) 
		`OR1200_M2R_ZERO: begin
				regdata_hh = 8'h00;
			end
		`OR1200_M2R_BYTE3: begin
				regdata_hh = memdata[31:24];
			end
		`OR1200_M2R_EXTB0: begin
				regdata_hh = {{memdata[7]},{memdata[7]},{memdata[7]},{memdata[7]},{memdata[7]},{memdata[7]},{memdata[7]},{memdata[7]}};
			end
		`OR1200_M2R_EXTB1: begin
				regdata_hh = {{memdata[15]},{memdata[15]},{memdata[15]},{memdata[15]},{memdata[15]},{memdata[15]},{memdata[15]},{memdata[15]}};
			end
		`OR1200_M2R_EXTB2: begin
				regdata_hh = {{memdata[23]},{memdata[23]},{memdata[23]},{memdata[23]},{memdata[23]},{memdata[23]},{memdata[23]},{memdata[23]}};
			end
		`OR1200_M2R_EXTB3: begin
				regdata_hh =  {{memdata[31]},{memdata[31]},{memdata[31]},{memdata[31]},{memdata[31]},{memdata[31]},{memdata[31]},{memdata[31]}};
			end
	endcase
end

//
// Straightforward implementation of mem2reg
//

// reg	[32-1:0]		regdata;

//
// Alignment
//
always @(addr or memdata) begin
	case(addr) 
		2'b00:
			aligned = memdata;
		2'b01:
			aligned = {memdata[23:0], 8'b00000000};
		2'b10:
			aligned = {memdata[15:0], 16'b0000000000000000};
		2'b11:
			aligned = {memdata[7:0], 24'b000000000000000000000000};
	endcase
end

//
// Bytes
//
/*
always @(lsu_op or aligned) begin
	case(lsu_op) 
		`OR1200_LSUOP_LBZ: begin
				regdata[7:0] = aligned[31:24];
				regdata[31:8] = 24'b000000000000000000000000;
			end
		`OR1200_LSUOP_LBS: begin
				regdata[7:0] = aligned[31:24];
				regdata[31:8] = {24'b000000000000000000000000};
			end
		`OR1200_LSUOP_LHZ: begin
				regdata[15:0] = aligned[31:16];
				regdata[31:16] = 16'b0000000000000000;
			end
		`OR1200_LSUOP_LHS: begin
				regdata[15:0] = aligned[31:16];
				regdata[31:16] = {16'b0000000000000000};
			end
		default:
				regdata = aligned;
	endcase
end
*/
wire[8:0] unused_signal;
assign unused_signal = lsu_op;
endmodule
