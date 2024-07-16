

`timescale 1ns/10ps
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 cores Definitions              		          ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////  8051 definitions.                                           ////
////                                                              ////
////  To Do:                                                      ////
////   Nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
////      - Jaka Simsic, jakas@opencores.org                      ////
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
// ver: 1
//

//
// oc8051 pherypherals
//
`define OC8051_UART
`define OC8051_TC01
`define OC8051_TC2
`define OC8051_PORTS  //ports global enable
`define OC8051_PORT0
`define OC8051_PORT1
`define OC8051_PORT2
`define OC8051_PORT3


//
// oc8051 ITERNAL ROM
//
//`define OC8051_ROM


//
// oc8051 memory
//
//`define OC8051_CACHE
//`define OC8051_WB

//`define OC8051_RAM_XILINX
//`define OC8051_RAM_VIRTUALSILICON
`define OC8051_RAM_GENERIC


`define OC8051_XILINX_ROM

//
// oc8051 simulation defines
//
//`define OC8051_SIMULATION
//`define OC8051_SERIAL

//
// oc8051 bist
//
//`define OC8051_BIST


//
// operation codes for alu
//


`define OC8051_ALU_NOP 4'b0000
`define OC8051_ALU_ADD 4'b0001
`define OC8051_ALU_SUB 4'b0010
`define OC8051_ALU_MUL 4'b0011
`define OC8051_ALU_DIV 4'b0100
`define OC8051_ALU_DA 4'b0101
`define OC8051_ALU_NOT 4'b0110
`define OC8051_ALU_AND 4'b0111
`define OC8051_ALU_XOR 4'b1000
`define OC8051_ALU_OR 4'b1001
`define OC8051_ALU_RL 4'b1010
`define OC8051_ALU_RLC 4'b1011
`define OC8051_ALU_RR 4'b1100
`define OC8051_ALU_RRC 4'b1101
`define OC8051_ALU_INC 4'b1110
`define OC8051_ALU_XCH 4'b1111

//
// sfr addresses
//

`define OC8051_SFR_ACC 8'he0 //accumulator
`define OC8051_SFR_B 8'hf0 //b register
`define OC8051_SFR_PSW 8'hd0 //program status word
`define OC8051_SFR_P0 8'h80 //port 0
`define OC8051_SFR_P1 8'h90 //port 1
`define OC8051_SFR_P2 8'ha0 //port 2
`define OC8051_SFR_P3 8'hb0 //port 3
`define OC8051_SFR_DPTR_LO 8'h82 // data pointer high bits
`define OC8051_SFR_DPTR_HI 8'h83 // data pointer low bits
`define OC8051_SFR_IP0 8'hb8 // interrupt priority
`define OC8051_SFR_IEN0 8'ha8 // interrupt enable 0
`define OC8051_SFR_TMOD 8'h89 // timer/counter mode
`define OC8051_SFR_TCON 8'h88 // timer/counter control
`define OC8051_SFR_TH0 8'h8c // timer/counter 0 high bits
`define OC8051_SFR_TL0 8'h8a // timer/counter 0 low bits
`define OC8051_SFR_TH1 8'h8d // timer/counter 1 high bits
`define OC8051_SFR_TL1 8'h8b // timer/counter 1 low bits

`define OC8051_SFR_SCON 8'h98 // serial control 0
`define OC8051_SFR_SBUF 8'h99 // serial data buffer 0
`define OC8051_SFR_SADDR 8'ha9 // serila address register 0
`define OC8051_SFR_SADEN 8'hb9 // serila address enable 0

`define OC8051_SFR_PCON 8'h87 // power control
`define OC8051_SFR_SP 8'h81 // stack pointer



`define OC8051_SFR_IE 8'ha8 // interrupt enable
`define OC8051_SFR_IP 8'hb7 // interrupt priority

`define OC8051_SFR_RCAP2H 8'hcb // timer 2 capture high
`define OC8051_SFR_RCAP2L 8'hca // timer 2 capture low

`define OC8051_SFR_T2CON 8'hc8 // timer 2 control register
`define OC8051_SFR_TH2 8'hcd // timer 2 high
`define OC8051_SFR_TL2 8'hcc // timer 2 low



//
// sfr bit addresses
//
`define OC8051_SFR_B_ACC 5'b11100 //accumulator
`define OC8051_SFR_B_PSW 5'b11010 //program status word
`define OC8051_SFR_B_P0  5'b10000 //port 0
`define OC8051_SFR_B_P1  5'b10010 //port 1
`define OC8051_SFR_B_P2  5'b10100 //port 2
`define OC8051_SFR_B_P3  5'b10110 //port 3
`define OC8051_SFR_B_B   5'b11110 // b register
`define OC8051_SFR_B_IP  5'b10111 // interrupt priority control 0
`define OC8051_SFR_B_IE  5'b10101 // interrupt enable control 0
`define OC8051_SFR_B_SCON 5'b10011 // serial control
`define OC8051_SFR_B_TCON  5'b10001 // timer/counter control
`define OC8051_SFR_B_T2CON 5'b11001 // timer/counter2 control


//
//carry input in alu
//
`define OC8051_CY_0 2'b00 // 1'b0;
`define OC8051_CY_PSW 2'b01 // carry from psw
`define OC8051_CY_RAM 2'b10 // carry from ram
`define OC8051_CY_1 2'b11 // 1'b1;
`define OC8051_CY_DC 2'b00 // carry from psw

//
// instruction set
//

//op_code [4:0]
`define OC8051_ACALL 8'bxxx1_0001 // absolute call
`define OC8051_AJMP 8'bxxx0_0001 // absolute jump

//op_code [7:3]
`define OC8051_ADD_R 8'b0010_1xxx // add A=A+Rx
`define OC8051_ADDC_R 8'b0011_1xxx // add A=A+Rx+c
`define OC8051_ANL_R 8'b0101_1xxx // and A=A^Rx
`define OC8051_CJNE_R 8'b1011_1xxx // compare and jump if not equal; Rx<>constant
`define OC8051_DEC_R 8'b0001_1xxx // decrement reg Rn=Rn-1
`define OC8051_DJNZ_R 8'b1101_1xxx // decrement and jump if not zero
`define OC8051_INC_R 8'b0000_1xxx // increment Rn
`define OC8051_MOV_R 8'b1110_1xxx // move A=Rn
`define OC8051_MOV_AR 8'b1111_1xxx // move Rn=A
`define OC8051_MOV_DR 8'b1010_1xxx // move Rn=(direct)
`define OC8051_MOV_CR 8'b0111_1xxx // move Rn=constant
`define OC8051_MOV_RD 8'b1000_1xxx // move (direct)=Rn
`define OC8051_ORL_R 8'b0100_1xxx // or A=A or Rn
`define OC8051_SUBB_R 8'b1001_1xxx // substract with borrow  A=A-c-Rn
`define OC8051_XCH_R 8'b1100_1xxx // exchange A<->Rn
`define OC8051_XRL_R 8'b0110_1xxx // XOR A=A XOR Rn

//op_code [7:1]
`define OC8051_ADD_I 8'b0010_011x // add A=A+@Ri
`define OC8051_ADDC_I 8'b0011_011x // add A=A+@Ri+c
`define OC8051_ANL_I 8'b0101_011x // and A=A^@Ri
`define OC8051_CJNE_I 8'b1011_011x // compare and jump if not equal; @Ri<>constant
`define OC8051_DEC_I 8'b0001_011x // decrement indirect @Ri=@Ri-1
`define OC8051_INC_I 8'b0000_011x // increment @Ri
`define OC8051_MOV_I 8'b1110_011x // move A=@Ri
`define OC8051_MOV_ID 8'b1000_011x // move (direct)=@Ri
`define OC8051_MOV_AI 8'b1111_011x // move @Ri=A
`define OC8051_MOV_DI 8'b1010_011x // move @Ri=(direct)
`define OC8051_MOV_CI 8'b0111_011x // move @Ri=constant
`define OC8051_MOVX_IA 8'b1110_001x // move A=(@Ri)
`define OC8051_MOVX_AI 8'b1111_001x // move (@Ri)=A
`define OC8051_ORL_I 8'b0100_011x // or A=A or @Ri
`define OC8051_SUBB_I 8'b1001_011x // substract with borrow  A=A-c-@Ri
`define OC8051_XCH_I 8'b1100_011x // exchange A<->@Ri
`define OC8051_XCHD 8'b1101_011x // exchange digit A<->Ri
`define OC8051_XRL_I 8'b0110_011x // XOR A=A XOR @Ri

//op_code [7:0]
`define OC8051_ADD_D 8'b0010_0101 // add A=A+(direct)
`define OC8051_ADD_C 8'b0010_0100 // add A=A+constant
`define OC8051_ADDC_D 8'b0011_0101 // add A=A+(direct)+c
`define OC8051_ADDC_C 8'b0011_0100 // add A=A+constant+c
`define OC8051_ANL_D 8'b0101_0101 // and A=A^(direct)
`define OC8051_ANL_C 8'b0101_0100 // and A=A^constant
`define OC8051_ANL_DD 8'b0101_0010 // and (direct)=(direct)^A
`define OC8051_ANL_DC 8'b0101_0011 // and (direct)=(direct)^constant
`define OC8051_ANL_B 8'b1000_0010 // and c=c^bit
`define OC8051_ANL_NB 8'b1011_0000 // and c=c^!bit
`define OC8051_CJNE_D 8'b1011_0101 // compare and jump if not equal; a<>(direct)
`define OC8051_CJNE_C 8'b1011_0100 // compare and jump if not equal; a<>constant
`define OC8051_CLR_A 8'b1110_0100 // clear accumulator
`define OC8051_CLR_C 8'b1100_0011 // clear carry
`define OC8051_CLR_B 8'b1100_0010 // clear bit
`define OC8051_CPL_A 8'b1111_0100 // complement accumulator
`define OC8051_CPL_C 8'b1011_0011 // complement carry
`define OC8051_CPL_B 8'b1011_0010 // complement bit
`define OC8051_DA 8'b1101_0100 // decimal adjust (A)
`define OC8051_DEC_A 8'b0001_0100 // decrement accumulator a=a-1
`define OC8051_DEC_D 8'b0001_0101 // decrement direct (direct)=(direct)-1
`define OC8051_DIV 8'b1000_0100 // divide
`define OC8051_DJNZ_D 8'b1101_0101 // decrement and jump if not zero (direct)
`define OC8051_INC_A 8'b0000_0100 // increment accumulator
`define OC8051_INC_D 8'b0000_0101 // increment (direct)
`define OC8051_INC_DP 8'b1010_0011 // increment data pointer
`define OC8051_JB 8'b0010_0000 // jump if bit set
`define OC8051_JBC 8'b0001_0000 // jump if bit set and clear bit
`define OC8051_JC 8'b0100_0000 // jump if carry is set
`define OC8051_JMP_D 8'b0111_0011 // jump indirect
`define OC8051_JNB 8'b0011_0000 // jump if bit not set
`define OC8051_JNC 8'b0101_0000 // jump if carry not set
`define OC8051_JNZ 8'b0111_0000 // jump if accumulator not zero
`define OC8051_JZ 8'b0110_0000 // jump if accumulator zero
`define OC8051_LCALL 8'b0001_0010 // long call
`define OC8051_LJMP 8'b0000_0010 // long jump
`define OC8051_MOV_D 8'b1110_0101 // move A=(direct)
`define OC8051_MOV_C 8'b0111_0100 // move A=constant
`define OC8051_MOV_DA 8'b1111_0101 // move (direct)=A
`define OC8051_MOV_DD 8'b1000_0101 // move (direct)=(direct)
`define OC8051_MOV_CD 8'b0111_0101 // move (direct)=constant
`define OC8051_MOV_BC 8'b1010_0010 // move c=bit
`define OC8051_MOV_CB 8'b1001_0010 // move bit=c
`define OC8051_MOV_DP 8'b1001_0000 // move dptr=constant(16 bit)
`define OC8051_MOVC_DP 8'b1001_0011 // move A=dptr+A
`define OC8051_MOVC_PC 8'b1000_0011 // move A=pc+A
`define OC8051_MOVX_PA 8'b1110_0000 // move A=(dptr)
`define OC8051_MOVX_AP 8'b1111_0000 // move (dptr)=A
`define OC8051_MUL 8'b1010_0100 // multiply a*b
`define OC8051_NOP 8'b0000_0000 // no operation
`define OC8051_ORL_D 8'b0100_0101 // or A=A or (direct)
`define OC8051_ORL_C 8'b0100_0100 // or A=A or constant
`define OC8051_ORL_AD 8'b0100_0010 // or (direct)=(direct) or A
`define OC8051_ORL_CD 8'b0100_0011 // or (direct)=(direct) or constant
`define OC8051_ORL_B 8'b0111_0010 // or c = c or bit
`define OC8051_ORL_NB 8'b1010_0000 // or c = c or !bit
`define OC8051_POP 8'b1101_0000 // stack pop
`define OC8051_PUSH 8'b1100_0000 // stack push
`define OC8051_RET 8'b0010_0010 // return from subrutine
`define OC8051_RETI 8'b0011_0010 // return from interrupt
`define OC8051_RL 8'b0010_0011 // rotate left
`define OC8051_RLC 8'b0011_0011 // rotate left thrugh carry
`define OC8051_RR 8'b0000_0011 // rotate right
`define OC8051_RRC 8'b0001_0011 // rotate right thrugh carry
`define OC8051_SETB_C 8'b1101_0011 // set carry
`define OC8051_SETB_B 8'b1101_0010 // set bit
`define OC8051_SJMP 8'b1000_0000 // short jump
`define OC8051_SUBB_D 8'b1001_0101 // substract with borrow  A=A-c-(direct)
`define OC8051_SUBB_C 8'b1001_0100 // substract with borrow  A=A-c-constant
`define OC8051_SWAP 8'b1100_0100 // swap A(0-3) <-> A(4-7)
`define OC8051_XCH_D 8'b1100_0101 // exchange A<->(direct)
`define OC8051_XRL_D 8'b0110_0101 // XOR A=A XOR (direct)
`define OC8051_XRL_C 8'b0110_0100 // XOR A=A XOR constant
`define OC8051_XRL_AD 8'b0110_0010 // XOR (direct)=(direct) XOR A
`define OC8051_XRL_CD 8'b0110_0011 // XOR (direct)=(direct) XOR constant


//
// default values (used after reset)
//
`define OC8051_RST_PC 23'h0 // program counter
`define OC8051_RST_ACC 8'h00 // accumulator
`define OC8051_RST_B 8'h00 // b register
`define OC8051_RST_PSW 8'h00 // program status word
`define OC8051_RST_SP 8'b0000_0111 // stack pointer
`define OC8051_RST_DPH 8'h00 // data pointer (high)
`define OC8051_RST_DPL 8'h00 // data pointer (low)
`define OC8051_RST_P0 8'b1111_1111 // port 0
`define OC8051_RST_P1 8'b1111_1111 // port 1
`define OC8051_RST_P2 8'b1111_1111 // port 2
`define OC8051_RST_P3 8'b1111_1111 // port 3
`define OC8051_RST_IP 8'b0000_0000 // interrupt priority
`define OC8051_RST_IE 8'b0000_0000 // interrupt enable
`define OC8051_RST_TMOD 8'b0000_0000 // timer/counter mode control
`define OC8051_RST_TCON 8'b0000_0000 // timer/counter control
`define OC8051_RST_TH0 8'b0000_0000 // timer/counter 0 high bits
`define OC8051_RST_TL0 8'b0000_0000 // timer/counter 0 low bits
`define OC8051_RST_TH1 8'b0000_0000 // timer/counter 1 high bits
`define OC8051_RST_TL1 8'b0000_0000 // timer/counter 1 low bits
`define OC8051_RST_SCON 8'b0000_0000 // serial control
`define OC8051_RST_SBUF 8'b0000_0000 // serial data buffer
`define OC8051_RST_PCON 8'b0000_0000 // power control register



`define OC8051_RST_RCAP2H 8'h00 // timer 2 capture high
`define OC8051_RST_RCAP2L 8'h00 // timer 2 capture low

`define OC8051_RST_T2CON 8'h00 // timer 2 control register
`define OC8051_RST_T2MOD 8'h00 // timer 2 mode control
`define OC8051_RST_TH2 8'h00 // timer 2 high
`define OC8051_RST_TL2 8'h00 // timer 2 low


//
// alu source 1 select
//
`define OC8051_AS1_RAM  3'b000 // RAM
`define OC8051_AS1_OP1  3'b111 //
`define OC8051_AS1_OP2  3'b001 //
`define OC8051_AS1_OP3  3'b010 //
`define OC8051_AS1_ACC  3'b011 // accumulator
`define OC8051_AS1_PCH  3'b100 //
`define OC8051_AS1_PCL  3'b101 //
`define OC8051_AS1_DC   3'b000 //

//
// alu source 2 select
//
`define OC8051_AS2_RAM   3'b00 // RAM
`define OC8051_AS2_ACC   3'b01 // accumulator
`define OC8051_AS2_ZERO  3'b10 // 8'h00
`define OC8051_AS2_OP2   3'b11 //

`define OC8051_AS2_DC    3'b00 //

//
// alu source 3 select
//
`define OC8051_AS3_DP   1'b0 // data pointer
`define OC8051_AS3_PC   1'b1 // program clunter
//`define OC8051_AS3_PCU  3'b101 // program clunter not registered
`define OC8051_AS3_DC   1'b0  //


//
//write sfr
//
`define OC8051_WRS_N    2'b00  //no
`define OC8051_WRS_ACC1 2'b01  // acc destination 1
`define OC8051_WRS_ACC2 2'b10  // acc destination 2
`define OC8051_WRS_DPTR 2'b11  // data pointer


//
// ram read select
//

`define OC8051_RRS_RN   3'b000 // registers
`define OC8051_RRS_I    3'b001 // indirect addressing (op2)
`define OC8051_RRS_D    3'b010 // direct addressing
`define OC8051_RRS_SP   3'b011 // stack pointer

`define OC8051_RRS_B    3'b100 // b register
`define OC8051_RRS_DPTR 3'b101 // data pointer
`define OC8051_RRS_PSW  3'b110 // program status word
`define OC8051_RRS_ACC  3'b111 // acc

`define OC8051_RRS_DC 3'b000 // don't c

//
// ram write select
//

`define OC8051_RWS_RN 3'b000 // registers
`define OC8051_RWS_D  3'b001 // direct addressing
`define OC8051_RWS_I  3'b010 // indirect addressing
`define OC8051_RWS_SP 3'b011 // stack pointer
`define OC8051_RWS_D3 3'b101 // direct address (op3)
`define OC8051_RWS_D1 3'b110 // direct address (op1)
`define OC8051_RWS_B  3'b111 // b register
`define OC8051_RWS_DC 3'b000 //

//
// pc in select
//
`define OC8051_PIS_DC  3'b000 // dont c
`define OC8051_PIS_AL  3'b000 // alu low
`define OC8051_PIS_AH  3'b001 // alu high
`define OC8051_PIS_SO1 3'b010 // relative address, op1
`define OC8051_PIS_SO2 3'b011 // relative address, op2
`define OC8051_PIS_I11 3'b100 // 11 bit immediate
`define OC8051_PIS_I16 3'b101 // 16 bit immediate
`define OC8051_PIS_ALU 3'b110 // alu destination {des2, des1}

//
// compare source select
//
`define OC8051_CSS_AZ  2'b00 // eq = accumulator == zero
`define OC8051_CSS_DES 2'b01 // eq = destination == zero
`define OC8051_CSS_CY  2'b10 // eq = cy
`define OC8051_CSS_BIT 2'b11 // eq = b_in
`define OC8051_CSS_DC  2'b01 // don't care


//
// pc Write
//
`define OC8051_PCW_N 1'b0 // not
`define OC8051_PCW_Y 1'b1 // yes

//
//psw set
//
`define OC8051_PS_NOT 2'b00 // DONT
`define OC8051_PS_CY 2'b01 // only carry
`define OC8051_PS_OV 2'b10 // carry and overflov
`define OC8051_PS_AC 2'b11 // carry, overflov an ac...

//
// rom address select
//
`define OC8051_RAS_PC 1'b0 // program counter
`define OC8051_RAS_DES 1'b1 // alu destination

////
//// write accumulator
////
//`define OC8051_WA_N 1'b0 // not
//`define OC8051_WA_Y 1'b1 // yes


//
//memory action select
//
`define OC8051_MAS_DPTR_R 3'b000 // read from external rom: acc=(dptr)
`define OC8051_MAS_DPTR_W 3'b001 // write to external rom: (dptr)=acc
`define OC8051_MAS_RI_R   3'b010 // read from external rom: acc=(Ri)
`define OC8051_MAS_RI_W   3'b011 // write to external rom: (Ri)=acc
`define OC8051_MAS_CODE   3'b100 // read from program memory
`define OC8051_MAS_NO     3'b111 // no action


////////////////////////////////////////////////////

//
// Timer/Counter modes
//

`define OC8051_MODE0 2'b00  // mode 0
`define OC8051_MODE1 2'b01  // mode 0
`define OC8051_MODE2 2'b10  // mode 0
`define OC8051_MODE3 2'b11  // mode 0


//
// Interrupt numbers (vectors)
//

`define OC8051_INT_X0   8'h03  // external interrupt 0
`define OC8051_INT_T0   8'h0b  // T/C 0 owerflow interrupt
`define OC8051_INT_X1   8'h13  // external interrupt 1
`define OC8051_INT_T1   8'h1b  // T/C 1 owerflow interrupt
`define OC8051_INT_UART 8'h23  // uart interrupt
`define OC8051_INT_T2   8'h2b  // T/C 2 owerflow interrupt


//
// interrupt levels
//

`define OC8051_ILEV_L0 1'b0  // interrupt on level 0
`define OC8051_ILEV_L1 1'b1  // interrupt on level 1

//
// interrupt sources
//
`define OC8051_ISRC_NO   3'b000  // no interrupts
`define OC8051_ISRC_IE0  3'b001  // EXTERNAL INTERRUPT 0
`define OC8051_ISRC_TF0  3'b010  // t/c owerflov 0
`define OC8051_ISRC_IE1  3'b011  // EXTERNAL INTERRUPT 1
`define OC8051_ISRC_TF1  3'b100  // t/c owerflov 1
`define OC8051_ISRC_UART 3'b101  // UART  Interrupt
`define OC8051_ISRC_T2   3'b110  // t/c owerflov 2



//
// miscellaneus
//

`define OC8051_RW0 1'b1
`define OC8051_RW1 1'b0


//
// read modify write instruction
//

`define OC8051_RMW_Y 1'b1  // yes
`define OC8051_RMW_N 1'b0  // no

//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 cores acccumulator                                     ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   accumulaor register for 8051 core                          ////
////                                                              ////
////  To Do:                                                      ////
////   Nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.13  2003/06/03 17:16:16  simont
// `ifdef added.
//
// Revision 1.12  2003/04/09 16:24:03  simont
// change wr_sft to 2 bit wire.
//
// Revision 1.11  2003/04/09 15:49:42  simont
// Register oc8051_sfr dato output, add signal wait_data.
//
// Revision 1.10  2003/04/07 14:58:02  simont
// change sfr's interface.
//
// Revision 1.9  2003/01/13 14:14:40  simont
// replace some modules
//
// Revision 1.8  2002/11/05 17:23:54  simont
// add module oc8051_sfr, 256 bytes internal ram
//
// Revision 1.7  2002/09/30 17:33:59  simont
// prepared header
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"


module oc8051_acc (clk, rst, 
                 bit_in, data_in, data2_in, 
		 data_out,
		 wr, wr_bit, wr_addr,
		 p, wr_sfr);


input clk, rst, wr, wr_bit, bit_in;
input [1:0] wr_sfr;
input [7:0] wr_addr, data_in, data2_in;

output p;
output [7:0] data_out;

reg [7:0] data_out;
reg [7:0] acc;

wire wr_acc, wr2_acc, wr_bit_acc;
//
//calculates parity
assign p = ^acc;

assign wr_acc     = (wr_sfr==`OC8051_WRS_ACC1) | (wr & !wr_bit & (wr_addr==`OC8051_SFR_ACC));
assign wr2_acc    = (wr_sfr==`OC8051_WRS_ACC2);
assign wr_bit_acc = (wr & wr_bit & (wr_addr[7:3]==`OC8051_SFR_B_ACC));
//
//writing to acc
always @(wr_sfr or data2_in or wr2_acc or wr_acc or wr_bit_acc or wr_addr[2:0] or data_in or bit_in or data_out)
begin
  if (wr2_acc)
    acc = data2_in;
  else if (wr_acc)
    acc = data_in;
  else if (wr_bit_acc)
    case (wr_addr[2:0]) /* synopsys full_case parallel_case */
      3'b000: acc = {data_out[7:1], bit_in};
      3'b001: acc = {data_out[7:2], bit_in, data_out[0]};
      3'b010: acc = {data_out[7:3], bit_in, data_out[1:0]};
      3'b011: acc = {data_out[7:4], bit_in, data_out[2:0]};
      3'b100: acc = {data_out[7:5], bit_in, data_out[3:0]};
      3'b101: acc = {data_out[7:6], bit_in, data_out[4:0]};
      3'b110: acc = {data_out[7],   bit_in, data_out[5:0]};
      3'b111: acc = {bit_in, data_out[6:0]};
    endcase
  else
    acc = data_out;
end

always @(posedge clk or posedge rst)
begin
  if (rst)
    data_out <= #1 `OC8051_RST_ACC;
  else
    data_out <= #1 acc;
end


`ifdef OC8051_SIMULATION

always @(data_out)
  if (data_out===8'hxx) begin
    $display("time ",$time, "   faulire: invalid write to ACC (oc8051_acc)");
#22
    $finish;

  end


`endif


endmodule

//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 alu source select module                               ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   Multiplexer wiht whitch we select data on alu sources      ////
////                                                              ////
////  To Do:                                                      ////
////   nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.3  2003/06/03 17:13:57  simont
// remove pc_r register.
//
// Revision 1.2  2003/05/06 09:41:35  simont
// remove define OC8051_AS2_PCL, chane signal src_sel2 to 2 bit wide.
//
// Revision 1.1  2003/01/13 14:13:12  simont
// initial import
//
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"


module oc8051_alu_src_sel (clk, rst, rd, sel1, sel2, sel3,
                     acc, ram, pc, dptr,

                     op1, op2, op3,

                     src1, src2, src3);


input clk, rst, rd, sel3;
input [1:0] sel2;
input [2:0] sel1;
input [7:0] acc, ram;
input [15:0] dptr;
input [15:0] pc;


input [7:0] op1, op2, op3;

output [7:0] src1, src2, src3;

reg [7:0] src1, src2, src3;

reg [7:0] op1_r, op2_r, op3_r;

///////
//
// src1
//
///////
always @(sel1 or op1_r or op2_r or op3_r or pc or acc or ram)
begin
  case (sel1) /* synopsys full_case parallel_case */
    `OC8051_AS1_RAM: src1 = ram;
    `OC8051_AS1_ACC: src1 = acc;
    `OC8051_AS1_OP1: src1 = op1_r;
    `OC8051_AS1_OP2: src1 = op2_r;
    `OC8051_AS1_OP3: src1 = op3_r;
    `OC8051_AS1_PCH: src1 = pc[15:8];
    `OC8051_AS1_PCL: src1 = pc[7:0];
//    default: src1 = 8'h00;
  endcase
end

///////
//
// src2
//
///////
always @(sel2 or op2_r or acc or ram or op1_r)
begin
  case (sel2) /* synopsys full_case parallel_case */
    `OC8051_AS2_ACC: src2= acc;
    `OC8051_AS2_ZERO: src2= 8'h00;
    `OC8051_AS2_RAM: src2= ram;
    `OC8051_AS2_OP2: src2= op2_r;
//    default: src2= 8'h00;
  endcase
end

///////
//
// src3
//
///////

always @(sel3 or pc[15:8] or dptr[15:8] or op1_r)
begin
  case (sel3) /* synopsys full_case parallel_case */
    `OC8051_AS3_DP:   src3= dptr[15:8];
    `OC8051_AS3_PC:   src3= pc[15:8];
//    default: src3= 16'h0;
  endcase
end


always @(posedge clk or posedge rst)
  if (rst) begin
    op1_r <= #1 8'h00;
    op2_r <= #1 8'h00;
    op3_r <= #1 8'h00;
  end else begin
    op1_r <= #1 op1;
    op2_r <= #1 op2;
    op3_r <= #1 op3;
  end

endmodule
//////////////////////////////////////////////////////////////////////
//// 								  ////
//// alu for 8051 Core 						  ////
//// 								  ////
//// This file is part of the 8051 cores project 		  ////
//// http://www.opencores.org/cores/8051/ 			  ////
//// 								  ////
//// Description 						  ////
//// Implementation of aritmetic unit  according to 		  ////
//// 8051 IP core specification document. Uses divide.v and 	  ////
//// multiply.v							  ////
//// 								  ////
//// To Do: 							  ////
////  pc signed add                                               ////
//// 								  ////
//// Author(s): 						  ////
//// - Simon Teran, simont@opencores.org 			  ////
//// 								  ////
//////////////////////////////////////////////////////////////////////
//// 								  ////
//// Copyright (C) 2001 Authors and OPENCORES.ORG 		  ////
//// 								  ////
//// This source file may be used and distributed without 	  ////
//// restriction provided that this copyright statement is not 	  ////
//// removed from the file and that any derivative work contains  ////
//// the original copyright notice and the associated disclaimer. ////
//// 								  ////
//// This source file is free software; you can redistribute it   ////
//// and/or modify it under the terms of the GNU Lesser General   ////
//// Public License as published by the Free Software Foundation; ////
//// either version 2.1 of the License, or (at your option) any   ////
//// later version. 						  ////
//// 								  ////
//// This source is distributed in the hope that it will be 	  ////
//// useful, but WITHOUT ANY WARRANTY; without even the implied   ////
//// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 	  ////
//// PURPOSE. See the GNU Lesser General Public License for more  ////
//// details. 							  ////
//// 								  ////
//// You should have received a copy of the GNU Lesser General 	  ////
//// Public License along with this source; if not, download it   ////
//// from http://www.opencores.org/lgpl.shtml 			  ////
//// 								  ////
//////////////////////////////////////////////////////////////////////
//
// CVS Revision History
//
// $Log: not supported by cvs2svn $
// Revision 1.18  2003/07/01 18:51:11  simont
// x replaced with 0.
//
// Revision 1.17  2003/06/09 16:51:16  simont
// fix bug in DA operation.
//
// Revision 1.16  2003/06/03 17:15:06  simont
// sub_result output added.
//
// Revision 1.15  2003/05/07 12:31:53  simont
// add wire sub_result, conect it to des_acc and des1.
//
// Revision 1.14  2003/05/05 15:46:36  simont
// add aditional alu destination to solve critical path.
//
// Revision 1.13  2003/04/29 08:35:12  simont
// fix bug in substraction.
//
// Revision 1.12  2003/04/25 17:15:51  simont
// change branch instruction execution (reduse needed clock periods).
//
// Revision 1.11  2003/04/14 14:29:42  simont
// fiz bug iv pcs operation.
//
// Revision 1.10  2003/01/13 14:14:40  simont
// replace some modules
//
// Revision 1.9  2002/09/30 17:33:59  simont
// prepared header
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"



module oc8051_alu (clk, rst, op_code, src1, src2, src3, srcCy, srcAc, bit_in, 
                  des1, des2, des_acc, desCy, desAc, desOv, sub_result);
//
// op_code      (in)  operation code [oc8051_decoder.alu_op -r]
// src1         (in)  first operand [oc8051_alu_src1_sel.des]
// src2         (in)  second operand [oc8051_alu_src2_sel.des]
// src3         (in)  third operand [oc8051_alu_src3_sel.des]
// srcCy        (in)  carry input [oc8051_cy_select.data_out]
// srcAc        (in)  auxiliary carry input [oc8051_psw.data_out[6] ]
// bit_in       (in)  bit input, used for logic operatins on bits [oc8051_ram_sel.bit_out]
// des1         (out)
// des2         (out)
// desCy        (out) carry output [oc8051_ram_top.bit_data_in, oc8051_acc.bit_in, oc8051_b_register.bit_in, oc8051_psw.cy_in, oc8051_ports.bit_in]
// desAc        (out) auxiliary carry output [oc8051_psw.ac_in]
// desOv        (out) Overflow output [oc8051_psw.ov_in]
//

input        srcCy, srcAc, bit_in, clk, rst;
input  [3:0] op_code;
input  [7:0] src1, src2, src3;
output       desCy, desAc, desOv;
output [7:0] des1, des2, des_acc, sub_result;

reg desCy, desAc, desOv;
reg [7:0] des1, des2, des_acc;


//
//add
//
wire [4:0] add1, add2, add3, add4;
wire [3:0] add5, add6, add7, add8;
wire [1:0] add9, adda, addb, addc;

//
//sub
//
wire [4:0] sub1, sub2, sub3, sub4;
wire [3:0] sub5, sub6, sub7, sub8;
wire [1:0] sub9, suba, subb, subc;
wire [7:0] sub_result;

//
//mul
//
  wire [7:0] mulsrc1, mulsrc2;
  wire mulOv;
  reg enable_mul;

//
//div
//
wire [7:0] divsrc1,divsrc2;
wire divOv;
reg enable_div;

//
//da
//
reg da_tmp, da_tmp1;
//reg [8:0] da1;

//
// inc
//
wire [15:0] inc, dec;

oc8051_multiply oc8051_mul1(.clk(clk), .rst(rst), .enable(enable_mul), .src1(src1), .src2(src2), .des1(mulsrc1), .des2(mulsrc2), .desOv(mulOv));
oc8051_divide oc8051_div1(.clk(clk), .rst(rst), .enable(enable_div), .src1(src1), .src2(src2), .des1(divsrc1), .des2(divsrc2), .desOv(divOv));

/* Add */
assign add1 = {1'b0,src1[3:0]};
assign add2 = {1'b0,src2[3:0]};
assign add3 = {3'b000,srcCy};
assign add4 = add1+add2+add3;

assign add5 = {1'b0,src1[6:4]};
assign add6 = {1'b0,src2[6:4]};
assign add7 = {1'b0,1'b0,1'b0,add4[4]};
assign add8 = add5+add6+add7;

assign add9 = {1'b0,src1[7]};
assign adda = {1'b0,src2[7]};
assign addb = {1'b0,add8[3]};
assign addc = add9+adda+addb;

/* Sub */
assign sub1 = {1'b1,src1[3:0]};
assign sub2 = {1'b0,src2[3:0]};
assign sub3 = {1'b0,1'b0,1'b0,srcCy};
assign sub4 = sub1-sub2-sub3;

assign sub5 = {1'b1,src1[6:4]};
assign sub6 = {1'b0,src2[6:4]};
assign sub7 = {1'b0,1'b0,1'b0, !sub4[4]};
assign sub8 = sub5-sub6-sub7;

assign sub9 = {1'b1,src1[7]};
assign suba = {1'b0,src2[7]};
assign subb = {1'b0,!sub8[3]};
assign subc = sub9-suba-subb;

assign sub_result = {subc[0],sub8[2:0],sub4[3:0]};

/* inc */
assign inc = {src2, src1} + {15'h0, 1'b1};
assign dec = {src2, src1} - {15'h0, 1'b1};

always @(op_code or src1 or src2 or srcCy or srcAc or bit_in or src3 or mulsrc1
      or mulsrc2 or mulOv or divsrc1 or divsrc2 or divOv or addc or add8 or add4
      or sub4 or sub8 or subc or da_tmp or inc or dec or sub_result)
begin

  case (op_code) /* synopsys full_case parallel_case */
//operation add
    `OC8051_ALU_ADD: begin
      des_acc = {addc[0],add8[2:0],add4[3:0]};
      des1 = src1;
      des2 = src3+ {7'b0, addc[1]};
      desCy = addc[1];
      desAc = add4[4];
      desOv = addc[1] ^ add8[3];

      enable_mul = 1'b0;
      enable_div = 1'b0;
    end
//operation subtract
    `OC8051_ALU_SUB: begin
      des_acc = sub_result;
//      des1 = sub_result;
      des1 = 8'h00;
      des2 = 8'h00;
      desCy = !subc[1];
      desAc = !sub4[4];
      desOv = !subc[1] ^ !sub8[3];

      enable_mul = 1'b0;
      enable_div = 1'b0;
    end
//operation multiply
    `OC8051_ALU_MUL: begin
      des_acc = mulsrc1;
      des1 = src1;
      des2 = mulsrc2;
      desOv = mulOv;
      desCy = 1'b0;
      desAc = 1'b0;
      enable_mul = 1'b1;
      enable_div = 1'b0;
    end
//operation divide
    `OC8051_ALU_DIV: begin
      des_acc = divsrc1;
      des1 = src1;
      des2 = divsrc2;
      desOv = divOv;
      desAc = 1'b0;
      desCy = 1'b0;
      enable_mul = 1'b0;
      enable_div = 1'b1;
    end
//operation decimal adjustment
    `OC8051_ALU_DA: begin

      if (srcAc==1'b1 | src1[3:0]>4'b1001) {da_tmp, des_acc[3:0]} = {1'b0, src1[3:0]}+ 5'b00110;
      else {da_tmp, des_acc[3:0]} = {1'b0, src1[3:0]};

      if (srcCy | da_tmp | src1[7:4]>4'b1001)
        {da_tmp1, des_acc[7:4]} = {srcCy, src1[7:4]}+ 5'b00110 + {4'b0, da_tmp};
      else {da_tmp1, des_acc[7:4]} = {srcCy, src1[7:4]} + {4'b0, da_tmp};

      desCy = da_tmp | da_tmp1;
      des1 = src1;
      des2 = 8'h00;
      desAc = 1'b0;
      desOv = 1'b0;
      enable_mul = 1'b0;
      enable_div = 1'b0;
    end
//operation not
// bit operation not
    `OC8051_ALU_NOT: begin
      des_acc = ~src1;
      des1 = ~src1;
      des2 = 8'h00;
      desCy = !srcCy;
      desAc = 1'b0;
      desOv = 1'b0;
      enable_mul = 1'b0;
      enable_div = 1'b0;
    end
//operation and
//bit operation and
    `OC8051_ALU_AND: begin
      des_acc = src1 & src2;
      des1 = src1 & src2;
      des2 = 8'h00;
      desCy = srcCy & bit_in;
      desAc = 1'b0;
      desOv = 1'b0;
      enable_mul = 1'b0;
      enable_div = 1'b0;
    end
//operation xor
// bit operation xor
    `OC8051_ALU_XOR: begin
      des_acc = src1 ^ src2;
      des1 = src1 ^ src2;
      des2 = 8'h00;
      desCy = srcCy ^ bit_in;
      desAc = 1'b0;
      desOv = 1'b0;
      enable_mul = 1'b0;
      enable_div = 1'b0;
    end
//operation or
// bit operation or
    `OC8051_ALU_OR: begin
      des_acc = src1 | src2;
      des1 = src1 | src2;
      des2 = 8'h00;
      desCy = srcCy | bit_in;
      desAc = 1'b0;
      desOv = 1'b0;
      enable_mul = 1'b0;
      enable_div = 1'b0;
    end
//operation rotate left
// bit operation cy= cy or (not ram)
    `OC8051_ALU_RL: begin
      des_acc = {src1[6:0], src1[7]};
      des1 = src1 ;
      des2 = 8'h00;
      desCy = srcCy | !bit_in;
      desAc = 1'b0;
      desOv = 1'b0;
      enable_mul = 1'b0;
      enable_div = 1'b0;
    end
//operation rotate left with carry and swap nibbles
    `OC8051_ALU_RLC: begin
      des_acc = {src1[6:0], srcCy};
      des1 = src1 ;
      des2 = {src1[3:0], src1[7:4]};
      desCy = src1[7];
      desAc = 1'b0;
      desOv = 1'b0;
      enable_mul = 1'b0;
      enable_div = 1'b0;
    end
//operation rotate right
    `OC8051_ALU_RR: begin
      des_acc = {src1[0], src1[7:1]};
      des1 = src1 ;
      des2 = 8'h00;
      desCy = srcCy & !bit_in;
      desAc = 1'b0;
      desOv = 1'b0;
      enable_mul = 1'b0;
      enable_div = 1'b0;
    end
//operation rotate right with carry
    `OC8051_ALU_RRC: begin
      des_acc = {srcCy, src1[7:1]};
      des1 = src1 ;
      des2 = 8'h00;
      desCy = src1[0];
      desAc = 1'b0;
      desOv = 1'b0;
      enable_mul = 1'b0;
      enable_div = 1'b0;
    end
//operation pcs Add
    `OC8051_ALU_INC: begin
      if (srcCy) begin
        des_acc = dec[7:0];
	des1 = dec[7:0];
        des2 = dec[15:8];
      end else begin
        des_acc = inc[7:0];
	des1 = inc[7:0];
        des2 = inc[15:8];
      end
      desCy = 1'b0;
      desAc = 1'b0;
      desOv = 1'b0;
      enable_mul = 1'b0;
      enable_div = 1'b0;
    end
//operation exchange
//if carry = 0 exchange low order digit
    `OC8051_ALU_XCH: begin
      if (srcCy)
      begin
        des_acc = src2;
        des1 = src2;
        des2 = src1;
      end else begin
        des_acc = {src1[7:4],src2[3:0]};
        des1 = {src1[7:4],src2[3:0]};
        des2 = {src2[7:4],src1[3:0]};
      end
      desCy = 1'b0;
      desAc = 1'b0;
      desOv = 1'b0;
      enable_mul = 1'b0;
      enable_div = 1'b0;
    end
    `OC8051_ALU_NOP: begin
      des_acc = src1;
      des1 = src1;
      des2 = src2;
      desCy = srcCy;
      desAc = srcAc;
      desOv = 1'b0;
      enable_mul = 1'b0;
      enable_div = 1'b0;
    end
  endcase
end

endmodule
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 cores b register                                       ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   b register for 8051 core                                   ////
////                                                              ////
////  To Do:                                                      ////
////   Nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.8  2003/04/07 14:58:02  simont
// change sfr's interface.
//
// Revision 1.7  2003/01/13 14:14:40  simont
// replace some modules
//
// Revision 1.6  2002/09/30 17:33:59  simont
// prepared header
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"


module oc8051_b_register (clk, rst, bit_in, data_in, wr, wr_bit,
              wr_addr, data_out);


input clk, rst, wr, wr_bit, bit_in;
input [7:0] wr_addr, data_in;

output [7:0] data_out;

reg [7:0] data_out;

//
//writing to b
//must check if write high and correct address
always @(posedge clk or posedge rst)
begin
  if (rst)
    data_out <= #1 `OC8051_RST_B;
  else if (wr) begin
    if (!wr_bit) begin
      if (wr_addr==`OC8051_SFR_B)
        data_out <= #1 data_in;
    end else begin
      if (wr_addr[7:3]==`OC8051_SFR_B_B)
        data_out[wr_addr[2:0]] <= #1 bit_in;
    end
  end
end

endmodule
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 cache ram                                              ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   64x32 dual port ram for instruction cache                  ////
////                                                              ////
////  To Do:                                                      ////
////   nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.2  2002/10/24 13:37:43  simont
// add parameters
//
// Revision 1.1  2002/10/23 16:58:21  simont
// initial import
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on


module oc8051_cache_ram (clk, rst, addr0, data0, addr1, data1_i, data1_o, wr1);
//
// this module is part of oc8051_icache
// it's tehnology dependent
//
// clk          (in)  clock
// addr0        (in)  addres port 0
// data0        (out) data output port 0
// addr1        (in)  address port 1
// data1_i      (in)  data input port 1
// data1_o      (out) data output port 1
// wr1          (in)  write port 1
//

parameter ADR_WIDTH = 7; // cache address wihth
parameter CACHE_RAM = 128; // cache ram x 32 (2^ADR_WIDTH)

input clk, wr1, rst;
input [ADR_WIDTH-1:0] addr0, addr1;
input [31:0] data1_i;
output [31:0] data0, data1_o;

`ifdef OC8051_XILINX_RAM

  RAMB4_S8_S8 ram1(.DOA(data0[7:0]), .DOB(data1_o[7:0]), .ADDRA({2'b0, addr0}), .DIA(8'h00), .ENA(1'b1), .CLKA(clk), .WEA(1'b0),
                .RSTA(rst), .ADDRB({2'b0, addr1}), .DIB(data1_i[7:0]), .ENB(1'b1), .CLKB(clk), .WEB(wr1), .RSTB(rst));

  RAMB4_S8_S8 ram2(.DOA(data0[15:8]), .DOB(data1_o[15:8]), .ADDRA({2'b0, addr0}), .DIA(8'h00), .ENA(1'b1), .CLKA(clk), .WEA(1'b0),
                .RSTA(rst), .ADDRB({2'b0, addr1}), .DIB(data1_i[15:8]), .ENB(1'b1), .CLKB(clk), .WEB(wr1), .RSTB(rst));

  RAMB4_S8_S8 ram3(.DOA(data0[23:16]), .DOB(data1_o[23:16]), .ADDRA({2'b0, addr0}), .DIA(8'h00), .ENA(1'b1), .CLKA(clk), .WEA(1'b0),
                .RSTA(rst), .ADDRB({2'b0, addr1}), .DIB(data1_i[23:16]), .ENB(1'b1), .CLKB(clk), .WEB(wr1), .RSTB(rst));

  RAMB4_S8_S8 ram4(.DOA(data0[31:24]), .DOB(data1_o[31:24]), .ADDRA({2'b0, addr0}), .DIA(8'h00), .ENA(1'b1), .CLKA(clk), .WEA(1'b0),
                .RSTA(rst), .ADDRB({2'b0, addr1}), .DIB(data1_i[31:24]), .ENB(1'b1), .CLKB(clk), .WEB(wr1), .RSTB(rst));

`else

reg [31:0] data0, data1_o;

//
// buffer
reg [31:0] buff [0:CACHE_RAM];

//
// port 1
//
always @(posedge clk or posedge rst)
begin
  if (rst)
    data1_o <= #1 32'h0;
  else if (wr1) begin
    buff[addr1] <= #1 data1_i;
    data1_o <= #1 data1_i;
  end else
    data1_o <= #1 buff[addr1];
end

//
// port 0
//
always @(posedge clk or posedge rst)
begin
  if (rst)
    data0 <= #1 32'h0;
  else if ((addr0==addr1) & wr1)
    data0 <= #1 data1_i;
  else
    data0 <= #1 buff[addr0];
end

`endif


endmodule
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 compare                                                ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   compares selected inputs and set eq to 1 if they are equal ////
////   Is used for conditional jumps.                             ////
////                                                              ////
////  To Do:                                                      ////
////   replace CSS_AZ with CSS_DES                                ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.7  2003/04/25 17:15:51  simont
// change branch instruction execution (reduse needed clock periods).
//
// Revision 1.6  2003/04/02 11:26:21  simont
// updating...
//
// Revision 1.5  2002/09/30 17:33:59  simont
// prepared header
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"


module oc8051_comp (sel, b_in, cy, acc, des, /*comp_wait, */eq);
//
// sel          (in)  select whithc sourses to compare (look defines.v) [oc8051_decoder.comp_sel]
// b_in         (in)  bit in - output from bit addressable memory space [oc8051_ram_sel.bit_out]
// cy           (in)  carry flag [oc8051_psw.data_out[7] ]
// acc          (in)  accumulator [oc8051_acc.data_out]
// ram          (in)  input from ram [oc8051_ram_sel.out_data]
// op2          (in)  immediate data [oc8051_op_select.op2_out -r]
// des          (in)  destination from alu [oc8051_alu.des1 -r]
// eq           (out) if (src1 == src2) eq = 1  [oc8051_decoder.eq]
//


input [1:0] sel;
input b_in, cy/*, comp_wait*/;
input [7:0] acc, des;

output eq;

reg eq_r;

assign eq = eq_r;// & comp_wait;


always @(sel or b_in or cy or acc or des)
begin
  case (sel) /* synopsys full_case parallel_case */
    `OC8051_CSS_AZ  : eq_r = (acc == 8'h00);
    `OC8051_CSS_DES : eq_r = (des == 8'h00);
    `OC8051_CSS_CY  : eq_r = cy;
    `OC8051_CSS_BIT : eq_r = b_in;
  endcase
end

endmodule
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 alu carry select module                                ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   Multiplexer wiht whitch we select carry in alu             ////
////                                                              ////
////  To Do:                                                      ////
////   nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.3  2003/04/02 11:26:21  simont
// updating...
//
// Revision 1.2  2002/09/30 17:33:59  simont
// prepared header
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"


module oc8051_cy_select (cy_sel, cy_in, data_in, data_out);
//
// cy_sel       (in)  carry select, from decoder (see defines.v) [oc8051_decoder.cy_sel -r]
// cy_in        (in)  carry input [oc8051_psw.data_out[7] ]
// data_in      (in)  ram data input [oc8051_ram_sel.bit_out]
// data_out     (out) data output [oc8051_alu.srcCy]
//

input [1:0] cy_sel;
input cy_in, data_in;

output data_out;
reg data_out;

always @(cy_sel or cy_in or data_in)
begin
  case (cy_sel) /* synopsys full_case parallel_case */
    `OC8051_CY_0: data_out = 1'b0;
    `OC8051_CY_PSW: data_out = cy_in;
    `OC8051_CY_RAM: data_out = data_in;
    `OC8051_CY_1: data_out = 1'b1;
  endcase
end

endmodule
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 core decoder                                           ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   Main 8051 core module. decodes instruction and creates     ////
////   control sigals.                                            ////
////                                                              ////
////  To Do:                                                      ////
////   optimize state machine, especially IDS ASS and AS3         ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.21  2003/06/03 17:09:57  simont
// pipelined acces to axternal instruction interface added.
//
// Revision 1.20  2003/05/06 11:10:38  simont
// optimize state machine.
//
// Revision 1.19  2003/05/06 09:41:35  simont
// remove define OC8051_AS2_PCL, chane signal src_sel2 to 2 bit wide.
//
// Revision 1.18  2003/05/05 15:46:36  simont
// add aditional alu destination to solve critical path.
//
// Revision 1.17  2003/04/25 17:15:51  simont
// change branch instruction execution (reduse needed clock periods).
//
// Revision 1.16  2003/04/09 16:24:03  simont
// change wr_sft to 2 bit wire.
//
// Revision 1.15  2003/04/09 15:49:42  simont
// Register oc8051_sfr dato output, add signal wait_data.
//
// Revision 1.14  2003/01/13 14:14:40  simont
// replace some modules
//
// Revision 1.13  2002/10/23 16:53:39  simont
// fix bugs in instruction interface
//
// Revision 1.12  2002/10/17 18:50:00  simont
// cahnge interface to instruction rom
//
// Revision 1.11  2002/09/30 17:33:59  simont
// prepared header
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"


module oc8051_decoder (clk, rst, op_in, op1_c,
  ram_rd_sel_o, ram_wr_sel_o,
  bit_addr, wr_o, wr_sfr_o,
  src_sel1, src_sel2, src_sel3,
  alu_op_o, psw_set, eq, cy_sel, comp_sel,
  pc_wr, pc_sel, rd, rmw, istb, mem_act, mem_wait,
  wait_data);

//
// clk          (in)  clock
// rst          (in)  reset
// op_in        (in)  operation code [oc8051_op_select.op1]
// eq           (in)  compare result [oc8051_comp.eq]
// ram_rd_sel   (out) select, whitch address will be send to ram for read [oc8051_ram_rd_sel.sel, oc8051_sp.ram_rd_sel]
// ram_wr_sel   (out) select, whitch address will be send to ram for write [oc8051_ram_wr_sel.sel -r, oc8051_sp.ram_wr_sel -r]
// wr           (out) write - if 1 then we will write to ram [oc8051_ram_top.wr -r, oc8051_acc.wr -r, oc8051_b_register.wr -r, oc8051_sp.wr-r, oc8051_dptr.wr -r, oc8051_psw.wr -r, oc8051_indi_addr.wr -r, oc8051_ports.wr -r]
// src_sel1     (out) select alu source 1 [oc8051_alu_src1_sel.sel -r]
// src_sel2     (out) select alu source 2 [oc8051_alu_src2_sel.sel -r]
// src_sel3     (out) select alu source 3 [oc8051_alu_src3_sel.sel -r]
// alu_op       (out) alu operation [oc8051_alu.op_code -r]
// psw_set      (out) will we remember cy, ac, ov from alu [oc8051_psw.set -r]
// cy_sel       (out) carry in alu select [oc8051_cy_select.cy_sel -r]
// comp_sel     (out) compare source select [oc8051_comp.sel]
// bit_addr     (out) if instruction is bit addresable [oc8051_ram_top.bit_addr -r, oc8051_acc.wr_bit -r, oc8051_b_register.wr_bit-r, oc8051_sp.wr_bit -r, oc8051_dptr.wr_bit -r, oc8051_psw.wr_bit -r, oc8051_indi_addr.wr_bit -r, oc8051_ports.wr_bit -r]
// pc_wr        (out) pc write [oc8051_pc.wr]
// pc_sel       (out) pc select [oc8051_pc.pc_wr_sel]
// rd           (out) read from rom [oc8051_pc.rd, oc8051_op_select.rd]
// reti         (out) return from interrupt [pin]
// rmw          (out) read modify write feature [oc8051_ports.rmw]
// pc_wait      (out)
//

input clk, rst, eq, mem_wait, wait_data;
input [7:0] op_in;

output wr_o, bit_addr, pc_wr, rmw, istb, src_sel3;
output [1:0] psw_set, cy_sel, wr_sfr_o, src_sel2, comp_sel;
output [2:0] mem_act, src_sel1, ram_rd_sel_o, ram_wr_sel_o, pc_sel, op1_c;
output [3:0] alu_op_o;
output rd;

reg rmw;
reg src_sel3, wr,  bit_addr, pc_wr;
reg [3:0] alu_op;
reg [1:0] src_sel2, comp_sel, psw_set, cy_sel, wr_sfr;
reg [2:0] mem_act, src_sel1, ram_wr_sel, ram_rd_sel, pc_sel;

//
// state        if 2'b00 then normal execution, sle instructin that need more than one clock
// op           instruction buffer
reg  [1:0] state;
wire [1:0] state_dec;
reg  [7:0] op;
wire [7:0] op_cur;
reg  [2:0] ram_rd_sel_r;

reg stb_i;

assign rd = !state[0] && !state[1] && !wait_data;// && !stb_o;

assign istb = (!state[1]) && stb_i;

assign state_dec = wait_data ? 2'b00 : state;

assign op_cur = mem_wait ? 8'h00
                : (state[0] || state[1] || mem_wait || wait_data) ? op : op_in;
//assign op_cur = (state[0] || state[1] || mem_wait || wait_data) ? op : op_in;

assign op1_c = op_cur[2:0];

assign alu_op_o     = wait_data ? `OC8051_ALU_NOP : alu_op;
assign wr_sfr_o     = wait_data ? `OC8051_WRS_N   : wr_sfr;
assign ram_rd_sel_o = wait_data ? ram_rd_sel_r    : ram_rd_sel;
assign ram_wr_sel_o = wait_data ? `OC8051_RWS_DC  : ram_wr_sel;
assign wr_o         = wait_data ? 1'b0            : wr;

//
// main block
// unregisterd outputs
always @(op_cur or eq or state_dec or mem_wait)
begin
    case (state_dec) /* synopsys full_case parallel_case */
      2'b01: begin
        casex (op_cur) /* synopsys parallel_case */
          `OC8051_DIV : begin
              ram_rd_sel = `OC8051_RRS_B;
            end
          `OC8051_MUL : begin
              ram_rd_sel = `OC8051_RRS_B;
            end
          default begin
              ram_rd_sel = `OC8051_RRS_DC;
          end
        endcase
        stb_i = 1'b1;
        bit_addr = 1'b0;
        pc_wr = `OC8051_PCW_N;
        pc_sel = `OC8051_PIS_DC;
        comp_sel =  `OC8051_CSS_DC;
        rmw = `OC8051_RMW_N;
      end
      2'b10: begin
        casex (op_cur) /* synopsys parallel_case */
          `OC8051_SJMP : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_Y;
              pc_sel = `OC8051_PIS_SO1;
              comp_sel =  `OC8051_CSS_DC;
              bit_addr = 1'b0;
            end
          `OC8051_JC : begin
              ram_rd_sel = `OC8051_RRS_PSW;
              pc_wr = eq;
              pc_sel = `OC8051_PIS_SO1;
              comp_sel =  `OC8051_CSS_CY;
              bit_addr = 1'b0;
            end
          `OC8051_JNC : begin
              ram_rd_sel = `OC8051_RRS_PSW;
              pc_wr = !eq;
              pc_sel = `OC8051_PIS_SO1;
              comp_sel =  `OC8051_CSS_CY;
              bit_addr = 1'b0;
            end
          `OC8051_JNZ : begin
              ram_rd_sel = `OC8051_RRS_ACC;
              pc_wr = !eq;
              pc_sel = `OC8051_PIS_SO1;
              comp_sel =  `OC8051_CSS_AZ;
              bit_addr = 1'b0;
            end
          `OC8051_JZ : begin
              ram_rd_sel = `OC8051_RRS_ACC;
              pc_wr = eq;
              pc_sel = `OC8051_PIS_SO1;
              comp_sel =  `OC8051_CSS_AZ;
              bit_addr = 1'b0;
            end

          `OC8051_RET : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_Y;
              pc_sel = `OC8051_PIS_AL;
              comp_sel =  `OC8051_CSS_DC;
              bit_addr = 1'b0;
            end
          `OC8051_RETI : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_Y;
              pc_sel = `OC8051_PIS_AL;
              comp_sel =  `OC8051_CSS_DC;
              bit_addr = 1'b0;
            end
          `OC8051_CJNE_R : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = !eq;
              pc_sel = `OC8051_PIS_SO2;
              comp_sel =  `OC8051_CSS_DES;
              bit_addr = 1'b0;
            end
          `OC8051_CJNE_I : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = !eq;
              pc_sel = `OC8051_PIS_SO2;
              comp_sel =  `OC8051_CSS_DES;
              bit_addr = 1'b0;
            end
          `OC8051_CJNE_D : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = !eq;
              pc_sel = `OC8051_PIS_SO2;
              comp_sel =  `OC8051_CSS_DES;
              bit_addr = 1'b0;
            end
          `OC8051_CJNE_C : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = !eq;
              pc_sel = `OC8051_PIS_SO2;
              comp_sel =  `OC8051_CSS_DES;
              bit_addr = 1'b0;
            end
          `OC8051_DJNZ_R : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = !eq;
              pc_sel = `OC8051_PIS_SO1;
              comp_sel =  `OC8051_CSS_DES;
              bit_addr = 1'b0;
            end
          `OC8051_DJNZ_D : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = !eq;
              pc_sel = `OC8051_PIS_SO2;
              comp_sel =  `OC8051_CSS_DES;
              bit_addr = 1'b0;
            end
          `OC8051_JB : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = eq;
              pc_sel = `OC8051_PIS_SO2;
              comp_sel =  `OC8051_CSS_BIT;
              bit_addr = 1'b0;
            end
          `OC8051_JBC : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = eq;
              pc_sel = `OC8051_PIS_SO2;
              comp_sel =  `OC8051_CSS_BIT;
              bit_addr = 1'b1;
            end
          `OC8051_JMP_D : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_Y;
              pc_sel = `OC8051_PIS_ALU;
              comp_sel =  `OC8051_CSS_DC;
              bit_addr = 1'b0;
            end
          `OC8051_JNB : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = !eq;
              pc_sel = `OC8051_PIS_SO2;
              comp_sel =  `OC8051_CSS_BIT;
              bit_addr = 1'b1;
            end
          `OC8051_DIV : begin
              ram_rd_sel = `OC8051_RRS_B;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              bit_addr = 1'b0;
            end
          `OC8051_MUL : begin
              ram_rd_sel = `OC8051_RRS_B;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              bit_addr = 1'b0;
            end
          default begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              bit_addr = 1'b0;
          end
        endcase
        rmw = `OC8051_RMW_N;
        stb_i = 1'b1;
      end
      2'b11: begin
        casex (op_cur) /* synopsys parallel_case */
          `OC8051_CJNE_R : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
            end
          `OC8051_CJNE_I : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
            end
          `OC8051_CJNE_D : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
            end
          `OC8051_CJNE_C : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
            end
          `OC8051_DJNZ_R : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
            end
          `OC8051_DJNZ_D : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
            end
          `OC8051_RET : begin
              ram_rd_sel = `OC8051_RRS_SP;
              pc_wr = `OC8051_PCW_Y;
              pc_sel = `OC8051_PIS_AH;
            end
          `OC8051_RETI : begin
              ram_rd_sel = `OC8051_RRS_SP;
              pc_wr = `OC8051_PCW_Y;
              pc_sel = `OC8051_PIS_AH;
            end
          `OC8051_DIV : begin
              ram_rd_sel = `OC8051_RRS_B;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
            end
          `OC8051_MUL : begin
              ram_rd_sel = `OC8051_RRS_B;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
            end
         default begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
          end
        endcase
        comp_sel =  `OC8051_CSS_DC;
        rmw = `OC8051_RMW_N;
        stb_i = 1'b1;
        bit_addr = 1'b0;
      end
      2'b00: begin
        casex (op_cur) /* synopsys parallel_case */
          `OC8051_ACALL :begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_Y;
              pc_sel = `OC8051_PIS_I11;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_AJMP : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_Y;
              pc_sel = `OC8051_PIS_I11;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_ADD_R : begin
              ram_rd_sel = `OC8051_RRS_RN;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_ADDC_R : begin
             ram_rd_sel = `OC8051_RRS_RN;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_ANL_R : begin
              ram_rd_sel = `OC8051_RRS_RN;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_CJNE_R : begin
              ram_rd_sel = `OC8051_RRS_RN;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_DEC_R : begin
              ram_rd_sel = `OC8051_RRS_RN;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_DJNZ_R : begin
              ram_rd_sel = `OC8051_RRS_RN;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_INC_R : begin
              ram_rd_sel = `OC8051_RRS_RN;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_MOV_R : begin
              ram_rd_sel = `OC8051_RRS_RN;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_MOV_DR : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_MOV_RD : begin
              ram_rd_sel = `OC8051_RRS_RN;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_ORL_R : begin
              ram_rd_sel = `OC8051_RRS_RN;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_SUBB_R : begin
              ram_rd_sel = `OC8051_RRS_RN;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_XCH_R : begin
              ram_rd_sel = `OC8051_RRS_RN;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_XRL_R : begin
              ram_rd_sel = `OC8051_RRS_RN;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
    
    //op_code [7:1]
          `OC8051_ADD_I : begin
              ram_rd_sel = `OC8051_RRS_I;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_ADDC_I : begin
              ram_rd_sel = `OC8051_RRS_I;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_ANL_I : begin
              ram_rd_sel = `OC8051_RRS_I;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_CJNE_I : begin
              ram_rd_sel = `OC8051_RRS_I;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_DEC_I : begin
              ram_rd_sel = `OC8051_RRS_I;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_INC_I : begin
              ram_rd_sel = `OC8051_RRS_I;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_MOV_I : begin
              ram_rd_sel = `OC8051_RRS_I;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_MOV_ID : begin
              ram_rd_sel = `OC8051_RRS_I;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_MOV_DI : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_MOVX_IA : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_MOVX_AI :begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_ORL_I : begin
              ram_rd_sel = `OC8051_RRS_I;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_SUBB_I : begin
              ram_rd_sel = `OC8051_RRS_I;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_XCH_I : begin
              ram_rd_sel = `OC8051_RRS_I;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_XCHD :begin
              ram_rd_sel = `OC8051_RRS_I;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_XRL_I : begin
              ram_rd_sel = `OC8051_RRS_I;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
    
    //op_code [7:0]
          `OC8051_ADD_D : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_ADDC_D : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_ANL_D : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_ANL_C : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_ANL_DD : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_ANL_DC : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_ANL_B : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b1;
            end
          `OC8051_ANL_NB : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b1;
            end
          `OC8051_CJNE_D : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_CJNE_C : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_CLR_B : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b1;
            end
          `OC8051_CPL_B : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b1;
            end
          `OC8051_DEC_D : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_DIV : begin
              ram_rd_sel = `OC8051_RRS_B;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_DJNZ_D : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_INC_D : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_INC_DP : begin
              ram_rd_sel = `OC8051_RRS_DPTR;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_JB : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_SO2;
              comp_sel =  `OC8051_CSS_BIT;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b1;
            end
          `OC8051_JBC : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_BIT;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b1;
            end
/*          `OC8051_JC : begin
              ram_rd_sel = `OC8051_RRS_PSW;
              pc_wr = eq;
              pc_sel = `OC8051_PIS_SO1;
              comp_sel =  `OC8051_CSS_CY;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end*/
          `OC8051_JMP_D : begin
              ram_rd_sel = `OC8051_RRS_DPTR;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
    
          `OC8051_JNB : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_SO2;
              comp_sel =  `OC8051_CSS_BIT;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b1;
            end
/*          `OC8051_JNC : begin
              ram_rd_sel = `OC8051_RRS_PSW;
              pc_wr = !eq;
              pc_sel = `OC8051_PIS_SO1;
              comp_sel =  `OC8051_CSS_CY;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_JNZ : begin
              ram_rd_sel = `OC8051_RRS_ACC;
              pc_wr = !eq;
              pc_sel = `OC8051_PIS_SO1;
              comp_sel =  `OC8051_CSS_AZ;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_JZ : begin
              ram_rd_sel = `OC8051_RRS_ACC;
              pc_wr = eq;
              pc_sel = `OC8051_PIS_SO1;
              comp_sel =  `OC8051_CSS_AZ;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end*/
          `OC8051_LCALL :begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_Y;
              pc_sel = `OC8051_PIS_I16;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_LJMP : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_Y;
              pc_sel = `OC8051_PIS_I16;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_MOV_D : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_MOV_DD : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_MOV_BC : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b1;
            end
          `OC8051_MOV_CB : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b1;
            end
          `OC8051_MOVC_DP :begin
              ram_rd_sel = `OC8051_RRS_DPTR;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_MOVC_PC : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_MOVX_PA : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_MOVX_AP : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_MUL : begin
              ram_rd_sel = `OC8051_RRS_B;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_ORL_D : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_ORL_AD : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_ORL_CD : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_ORL_B : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b1;
            end
          `OC8051_ORL_NB : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b1;
            end
          `OC8051_POP : begin
              ram_rd_sel = `OC8051_RRS_SP;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_PUSH : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_RET : begin
              ram_rd_sel = `OC8051_RRS_SP;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_RETI : begin
              ram_rd_sel = `OC8051_RRS_SP;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end
          `OC8051_SETB_B : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b1;
            end
/*          `OC8051_SJMP : begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_Y;
              pc_sel = `OC8051_PIS_SO1;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b0;
              bit_addr = 1'b0;
            end*/
          `OC8051_SUBB_D : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_XCH_D : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_XRL_D : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_XRL_AD : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          `OC8051_XRL_CD : begin
              ram_rd_sel = `OC8051_RRS_D;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_Y;
              stb_i = 1'b1;
              bit_addr = 1'b0;
            end
          default: begin
              ram_rd_sel = `OC8051_RRS_DC;
              pc_wr = `OC8051_PCW_N;
              pc_sel = `OC8051_PIS_DC;
              comp_sel =  `OC8051_CSS_DC;
              rmw = `OC8051_RMW_N;
              stb_i = 1'b1;
              bit_addr = 1'b0;
           end
        endcase
      end
    endcase
end










//
//
// registerd outputs

always @(posedge clk or posedge rst)
begin
  if (rst) begin
    ram_wr_sel <= #1 `OC8051_RWS_DC;
    src_sel1 <= #1 `OC8051_AS1_DC;
    src_sel2 <= #1 `OC8051_AS2_DC;
    alu_op <= #1 `OC8051_ALU_NOP;
    wr <= #1 1'b0;
    psw_set <= #1 `OC8051_PS_NOT;
    cy_sel <= #1 `OC8051_CY_0;
    src_sel3 <= #1 `OC8051_AS3_DC;
    wr_sfr <= #1 `OC8051_WRS_N;
  end else if (!wait_data) begin
    case (state_dec) /* synopsys parallel_case */
      2'b01: begin
        casex (op_cur) /* synopsys parallel_case */
          `OC8051_MOVC_DP :begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_OP1;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_MOVC_PC :begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_OP1;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_MOVX_PA : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_OP1;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_MOVX_IA : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_OP1;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
/*          `OC8051_ACALL :begin
              ram_wr_sel <= #1 `OC8051_RWS_SP;
              src_sel1 <= #1 `OC8051_AS1_PCH;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_AJMP : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_LCALL :begin
              ram_wr_sel <= #1 `OC8051_RWS_SP;
              src_sel1 <= #1 `OC8051_AS1_PCH;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              wr_sfr <= #1 `OC8051_WRS_N;
            end*/
          `OC8051_DIV : begin
              ram_wr_sel <= #1 `OC8051_RWS_B;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_DIV;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_OV;
              wr_sfr <= #1 `OC8051_WRS_ACC2;
            end
          `OC8051_MUL : begin
              ram_wr_sel <= #1 `OC8051_RWS_B;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_MUL;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_OV;
              wr_sfr <= #1 `OC8051_WRS_ACC2;
            end
          default begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              wr_sfr <= #1 `OC8051_WRS_N;
          end
        endcase
        cy_sel <= #1 `OC8051_CY_0;
        src_sel3 <= #1 `OC8051_AS3_DC;
      end
      2'b10: begin
        casex (op_cur) /* synopsys parallel_case */
          `OC8051_ACALL :begin
              ram_wr_sel <= #1 `OC8051_RWS_SP;
              src_sel1 <= #1 `OC8051_AS1_PCH;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
            end
          `OC8051_LCALL :begin
              ram_wr_sel <= #1 `OC8051_RWS_SP;
              src_sel1 <= #1 `OC8051_AS1_PCH;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
            end
          `OC8051_JBC : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
            end
          `OC8051_DIV : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_DIV;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_OV;
            end
          `OC8051_MUL : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_MUL;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_OV;
            end
          default begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
          end
        endcase
        cy_sel <= #1 `OC8051_CY_0;
        src_sel3 <= #1 `OC8051_AS3_DC;
        wr_sfr <= #1 `OC8051_WRS_N;
      end

      2'b11: begin
        casex (op_cur) /* synopsys parallel_case */
          `OC8051_RET : begin
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              psw_set <= #1 `OC8051_PS_NOT;
            end
          `OC8051_RETI : begin
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              psw_set <= #1 `OC8051_PS_NOT;
            end
          `OC8051_DIV : begin
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_DIV;
              psw_set <= #1 `OC8051_PS_OV;
            end
          `OC8051_MUL : begin
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_MUL;
              psw_set <= #1 `OC8051_PS_OV;
            end
         default begin
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              psw_set <= #1 `OC8051_PS_NOT;
          end
        endcase
        ram_wr_sel <= #1 `OC8051_RWS_DC;
        wr <= #1 1'b0;
        cy_sel <= #1 `OC8051_CY_0;
        src_sel3 <= #1 `OC8051_AS3_DC;
        wr_sfr <= #1 `OC8051_WRS_N;
      end
      default: begin
        casex (op_cur) /* synopsys parallel_case */
          `OC8051_ACALL :begin
              ram_wr_sel <= #1 `OC8051_RWS_SP;
              src_sel1 <= #1 `OC8051_AS1_PCL;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_AJMP : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_ADD_R : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_ADD;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_AC;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_ADDC_R : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_ADD;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_AC;
              cy_sel <= #1 `OC8051_CY_PSW;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_ANL_R : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_AND;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_CJNE_R : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_OP2;
              alu_op <= #1 `OC8051_ALU_SUB;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_CY;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_DEC_R : begin
              ram_wr_sel <= #1 `OC8051_RWS_RN;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ZERO;
              alu_op <= #1 `OC8051_ALU_INC;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_1;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_DJNZ_R : begin
              ram_wr_sel <= #1 `OC8051_RWS_RN;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ZERO;
              alu_op <= #1 `OC8051_ALU_INC;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_1;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_INC_R : begin
              ram_wr_sel <= #1 `OC8051_RWS_RN;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ZERO;
              alu_op <= #1 `OC8051_ALU_INC;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOV_R : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_MOV_AR : begin
              ram_wr_sel <= #1 `OC8051_RWS_RN;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOV_DR : begin
              ram_wr_sel <= #1 `OC8051_RWS_RN;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOV_CR : begin
              ram_wr_sel <= #1 `OC8051_RWS_RN;
              src_sel1 <= #1 `OC8051_AS1_OP2;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOV_RD : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_ORL_R : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_OR;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_SUBB_R : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_SUB;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_AC;
              cy_sel <= #1 `OC8051_CY_PSW;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_XCH_R : begin
              ram_wr_sel <= #1 `OC8051_RWS_RN;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_XCH;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_1;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC2;
            end
          `OC8051_XRL_R : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_XOR;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
    
    //op_code [7:1]
          `OC8051_ADD_I : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_ADD;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_AC;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_ADDC_I : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_ADD;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_AC;
              cy_sel <= #1 `OC8051_CY_PSW;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_ANL_I : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_AND;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_CJNE_I : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_OP2;
              alu_op <= #1 `OC8051_ALU_SUB;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_CY;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_DEC_I : begin
              ram_wr_sel <= #1 `OC8051_RWS_I;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ZERO;
              alu_op <= #1 `OC8051_ALU_INC;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_1;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_INC_I : begin
              ram_wr_sel <= #1 `OC8051_RWS_I;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ZERO;
              alu_op <= #1 `OC8051_ALU_INC;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOV_I : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_MOV_ID : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOV_AI : begin
              ram_wr_sel <= #1 `OC8051_RWS_I;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOV_DI : begin
              ram_wr_sel <= #1 `OC8051_RWS_I;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOV_CI : begin
              ram_wr_sel <= #1 `OC8051_RWS_I;
              src_sel1 <= #1 `OC8051_AS1_OP2;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOVX_IA : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOVX_AI :begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_ORL_I : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_OR;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_SUBB_I : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_SUB;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_AC;
              cy_sel <= #1 `OC8051_CY_PSW;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_XCH_I : begin
              ram_wr_sel <= #1 `OC8051_RWS_I;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_XCH;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_1;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC2;
            end
          `OC8051_XCHD :begin
              ram_wr_sel <= #1 `OC8051_RWS_I;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_XCH;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC2;
            end
          `OC8051_XRL_I : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_XOR;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
    
    //op_code [7:0]
          `OC8051_ADD_D : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_ADD;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_AC;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_ADD_C : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_OP2;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_ADD;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_AC;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_ADDC_D : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_ADD;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_AC;
              cy_sel <= #1 `OC8051_CY_PSW;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_ADDC_C : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_OP2;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_ADD;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_AC;
              cy_sel <= #1 `OC8051_CY_PSW;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_ANL_D : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_AND;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_ANL_C : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_OP2;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_AND;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_ANL_DD : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_AND;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_ANL_DC : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_OP3;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_AND;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_ANL_B : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_AND;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_CY;
              cy_sel <= #1 `OC8051_CY_PSW;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_ANL_NB : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_RR;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_CY;
              cy_sel <= #1 `OC8051_CY_PSW;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_CJNE_D : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_SUB;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_CY;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_CJNE_C : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_OP2;
              alu_op <= #1 `OC8051_ALU_SUB;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_CY;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_CLR_A : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_SUB;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_PC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_CLR_C : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_CY;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_PC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_CLR_B : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_PC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_CPL_A : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOT;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_CPL_C : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOT;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_CY;
              cy_sel <= #1 `OC8051_CY_PSW;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_CPL_B : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOT;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_RAM;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_DA : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_DA;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_CY;
              cy_sel <= #1 `OC8051_CY_PSW;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_DEC_A : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_ZERO;
              alu_op <= #1 `OC8051_ALU_INC;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_1;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_DEC_D : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ZERO;
              alu_op <= #1 `OC8051_ALU_INC;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_1;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_DIV : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_DIV;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_OV;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_DJNZ_D : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ZERO;
              alu_op <= #1 `OC8051_ALU_INC;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_1;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_INC_A : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_ZERO;
              alu_op <= #1 `OC8051_ALU_INC;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_INC_D : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ZERO;
              alu_op <= #1 `OC8051_ALU_INC;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_INC_DP : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ZERO;
              alu_op <= #1 `OC8051_ALU_ADD;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_1;
              src_sel3 <= #1 `OC8051_AS3_DP;
              wr_sfr <= #1 `OC8051_WRS_DPTR;
            end
          `OC8051_JB : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_PC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_JBC :begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_PC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_JC : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_PC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_JMP_D : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_ADD;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DP;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_JNB : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_PC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_JNC : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_PC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_JNZ :begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_PC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_JZ : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_PC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_LCALL :begin
              ram_wr_sel <= #1 `OC8051_RWS_SP;
              src_sel1 <= #1 `OC8051_AS1_PCL;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_LJMP : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOV_D : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_MOV_C : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_OP2;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_MOV_DA : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOV_DD : begin
              ram_wr_sel <= #1 `OC8051_RWS_D3;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOV_CD : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_OP3;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOV_BC : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_CY;
              cy_sel <= #1 `OC8051_CY_RAM;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOV_CB : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_PSW;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOV_DP : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_OP3;
              src_sel2 <= #1 `OC8051_AS2_OP2;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_DPTR;
            end
          `OC8051_MOVC_DP :begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_ADD;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DP;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOVC_PC : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_PCL;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_ADD;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_PC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOVX_PA : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MOVX_AP : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_MUL : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_MUL;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_OV;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_ORL_D : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_OR;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_ORL_C : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_OP2;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_OR;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_ORL_AD : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_OR;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_ORL_CD : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_OP3;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_OR;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_ORL_B : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_OR;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_CY;
              cy_sel <= #1 `OC8051_CY_PSW;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_ORL_NB : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_RL;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_CY;
              cy_sel <= #1 `OC8051_CY_PSW;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_POP : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_PUSH : begin
              ram_wr_sel <= #1 `OC8051_RWS_SP;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_RET : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_RETI : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_RL : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_RL;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_RLC : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_RLC;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_CY;
              cy_sel <= #1 `OC8051_CY_PSW;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_RR : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_RR;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_RRC : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_RRC;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_CY;
              cy_sel <= #1 `OC8051_CY_PSW;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_SETB_C : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_CY;
              cy_sel <= #1 `OC8051_CY_1;
              src_sel3 <= #1 `OC8051_AS3_PC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_SETB_B : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_1;
              src_sel3 <= #1 `OC8051_AS3_PC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_SJMP : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_PC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_SUBB_D : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_SUB;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_AC;
              cy_sel <= #1 `OC8051_CY_PSW;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_SUBB_C : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_OP2;
              alu_op <= #1 `OC8051_ALU_SUB;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_AC;
              cy_sel <= #1 `OC8051_CY_PSW;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_SWAP : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_ACC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_RLC;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC2;
            end
          `OC8051_XCH_D : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_XCH;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_1;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC2;
            end
          `OC8051_XRL_D : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_XOR;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_XRL_C : begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_OP2;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_XOR;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_ACC1;
            end
          `OC8051_XRL_AD : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_RAM;
              src_sel2 <= #1 `OC8051_AS2_ACC;
              alu_op <= #1 `OC8051_ALU_XOR;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          `OC8051_XRL_CD : begin
              ram_wr_sel <= #1 `OC8051_RWS_D;
              src_sel1 <= #1 `OC8051_AS1_OP3;
              src_sel2 <= #1 `OC8051_AS2_RAM;
              alu_op <= #1 `OC8051_ALU_XOR;
              wr <= #1 1'b1;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
            end
          default: begin
              ram_wr_sel <= #1 `OC8051_RWS_DC;
              src_sel1 <= #1 `OC8051_AS1_DC;
              src_sel2 <= #1 `OC8051_AS2_DC;
              alu_op <= #1 `OC8051_ALU_NOP;
              wr <= #1 1'b0;
              psw_set <= #1 `OC8051_PS_NOT;
              cy_sel <= #1 `OC8051_CY_0;
              src_sel3 <= #1 `OC8051_AS3_DC;
              wr_sfr <= #1 `OC8051_WRS_N;
           end
        endcase
      end
      endcase
  end
end


//
// remember current instruction
always @(posedge clk or posedge rst)
  if (rst) op <= #1 2'b00;
  else if (state==2'b00) op <= #1 op_in;

//
// in case of instructions that needs more than one clock set state
always @(posedge clk or posedge rst)
begin
  if (rst)
    state <= #1 2'b11;
  else if  (!mem_wait & !wait_data) begin
    case (state) /* synopsys parallel_case */
      2'b10: state <= #1 2'b01;
      2'b11: state <= #1 2'b10;
      2'b00:
          casex (op_in) /* synopsys full_case parallel_case */
            `OC8051_ACALL   : state <= #1 2'b10;
            `OC8051_AJMP    : state <= #1 2'b10;
            `OC8051_CJNE_R  : state <= #1 2'b10;
            `OC8051_CJNE_I  : state <= #1 2'b10;
            `OC8051_CJNE_D  : state <= #1 2'b10;
            `OC8051_CJNE_C  : state <= #1 2'b10;
            `OC8051_LJMP    : state <= #1 2'b10;
            `OC8051_DJNZ_R  : state <= #1 2'b10;
            `OC8051_DJNZ_D  : state <= #1 2'b10;
            `OC8051_LCALL   : state <= #1 2'b10;
            `OC8051_MOVC_DP : state <= #1 2'b11;
            `OC8051_MOVC_PC : state <= #1 2'b11;
            `OC8051_MOVX_IA : state <= #1 2'b10;
            `OC8051_MOVX_AI : state <= #1 2'b10;
            `OC8051_MOVX_PA : state <= #1 2'b10;
            `OC8051_MOVX_AP : state <= #1 2'b10;
            `OC8051_RET     : state <= #1 2'b11;
            `OC8051_RETI    : state <= #1 2'b11;
            `OC8051_SJMP    : state <= #1 2'b10;
            `OC8051_JB      : state <= #1 2'b10;
            `OC8051_JBC     : state <= #1 2'b10;
            `OC8051_JC      : state <= #1 2'b10;
            `OC8051_JMP_D   : state <= #1 2'b10;
            `OC8051_JNC     : state <= #1 2'b10;
            `OC8051_JNB     : state <= #1 2'b10;
            `OC8051_JNZ     : state <= #1 2'b10;
            `OC8051_JZ      : state <= #1 2'b10;
            `OC8051_DIV     : state <= #1 2'b11;
            `OC8051_MUL     : state <= #1 2'b11;
//            default         : state <= #1 2'b00;
          endcase
      default: state <= #1 2'b00;
    endcase
  end
end


//
//in case of writing to external ram
always @(posedge clk or posedge rst)
begin
  if (rst) begin
    mem_act <= #1 `OC8051_MAS_NO;
  end else if (!rd) begin
    mem_act <= #1 `OC8051_MAS_NO;
  end else
    casex (op_cur) /* synopsys parallel_case */
      `OC8051_MOVX_AI : mem_act <= #1 `OC8051_MAS_RI_W;
      `OC8051_MOVX_AP : mem_act <= #1 `OC8051_MAS_DPTR_W;
      `OC8051_MOVX_IA : mem_act <= #1 `OC8051_MAS_RI_R;
      `OC8051_MOVX_PA : mem_act <= #1 `OC8051_MAS_DPTR_R;
      `OC8051_MOVC_DP : mem_act <= #1 `OC8051_MAS_CODE;
      `OC8051_MOVC_PC : mem_act <= #1 `OC8051_MAS_CODE;
      default : mem_act <= #1 `OC8051_MAS_NO;
    endcase
end

always @(posedge clk or posedge rst)
begin
  if (rst) begin
    ram_rd_sel_r <= #1 3'h0;
  end else begin
    ram_rd_sel_r <= #1 ram_rd_sel;
  end
end



`ifdef OC8051_SIMULATION

always @(op_cur)
  if (op_cur===8'hxx) begin
    $display("time ",$time, "   faulire: invalid instruction (oc8051_decoder)");
#22
    $finish;

  end


`endif




endmodule


//////////////////////////////////////////////////////////////////////
//// 								  ////
//// divide for 8051 Core 				  	  ////
//// 								  ////
//// This file is part of the 8051 cores project 		  ////
//// http://www.opencores.org/cores/8051/ 			  ////
//// 								  ////
//// Description 						  ////
//// Four cycle implementation of division used in alu.v	  ////
//// 								  ////
//// To Do: 							  ////
////  check if compiler does proper optimizations of the code     ////
//// 								  ////
//// Author(s): 						  ////
//// - Simon Teran, simont@opencores.org 			  ////
//// - Marko Mlinar, markom@opencores.org 			  ////
//// 								  ////
//////////////////////////////////////////////////////////////////////
//// 								  ////
//// Copyright (C) 2001 Authors and OPENCORES.ORG 		  ////
//// 								  ////
//// This source file may be used and distributed without 	  ////
//// restriction provided that this copyright statement is not 	  ////
//// removed from the file and that any derivative work contains  ////
//// the original copyright notice and the associated disclaimer. ////
//// 								  ////
//// This source file is free software; you can redistribute it   ////
//// and/or modify it under the terms of the GNU Lesser General   ////
//// Public License as published by the Free Software Foundation; ////
//// either version 2.1 of the License, or (at your option) any   ////
//// later version. 						  ////
//// 								  ////
//// This source is distributed in the hope that it will be 	  ////
//// useful, but WITHOUT ANY WARRANTY; without even the implied   ////
//// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 	  ////
//// PURPOSE. See the GNU Lesser General Public License for more  ////
//// details. 							  ////
//// 								  ////
//// You should have received a copy of the GNU Lesser General 	  ////
//// Public License along with this source; if not, download it   ////
//// from http://www.opencores.org/lgpl.shtml 			  ////
//// 								  ////
//////////////////////////////////////////////////////////////////////
//
// CVS Revision History
//
// $Log: not supported by cvs2svn $
// Revision 1.8  2002/09/30 17:15:31  simont
// prepared header
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

module oc8051_divide (clk, rst, enable, src1, src2, des1, des2, desOv);
//
// this module is part of alu
// clk          (in)
// rst          (in)
// enable       (in)  starts divison
// src1         (in)  first operand
// src2         (in)  second operand
// des1         (out) first result
// des2         (out) second result
// desOv        (out) Overflow output
//

input clk, rst, enable;
input [7:0] src1, src2;
output desOv;
output [7:0] des1, des2;

// wires
wire desOv;
wire div0, div1;
wire [7:0] rem0, rem1, rem2;
wire [8:0] sub0, sub1;
wire [15:0] cmp0, cmp1;
wire [7:0] div_out, rem_out;

// real registers
reg [1:0] cycle;
reg [5:0] tmp_div;
reg [7:0] tmp_rem;

// The main logic
assign cmp1 = src2 << ({2'h3 - cycle, 1'b0} + 3'h1);
assign cmp0 = src2 << ({2'h3 - cycle, 1'b0} + 3'h0);

assign rem2 = cycle != 0 ? tmp_rem : src1;

assign sub1 = {1'b0, rem2} - {1'b0, cmp1[7:0]};
assign div1 = |cmp1[15:8] ? 1'b0 : !sub1[8];
assign rem1 = div1 ? sub1[7:0] : rem2[7:0];

assign sub0 = {1'b0, rem1} - {1'b0, cmp0[7:0]};
assign div0 = |cmp0[15:8] ? 1'b0 : !sub0[8];
assign rem0 = div0 ? sub0[7:0] : rem1[7:0];

//
// in clock cycle 0 we first calculate two MSB bits, ...
// till finally in clock cycle 3 we calculate two LSB bits
assign div_out = {tmp_div, div1, div0};
assign rem_out = rem0;
assign desOv = src2 == 8'h0;

//
// divider works in four clock cycles -- 0, 1, 2 and 3
always @(posedge clk or posedge rst)
begin
  if (rst) begin
    cycle <= #1 2'b0;
    tmp_div <= #1 6'h0;
    tmp_rem <= #1 8'h0;
  end else begin
    if (enable) cycle <= #1 cycle + 2'b1;
    tmp_div <= #1 div_out[5:0];
    tmp_rem <= #1 rem_out;
  end
end

//
// assign outputs
assign des1 = rem_out;
assign des2 = div_out;

endmodule
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 data pointer                                           ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   8051 special function register: data pointer               ////
////                                                              ////
////  To Do:                                                      ////
////   nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.3  2003/01/13 14:14:40  simont
// replace some modules
//
// Revision 1.2  2002/09/30 17:33:59  simont
// prepared header
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"


module oc8051_dptr(clk, rst, addr, data_in, data2_in, wr, wr_sfr, wr_bit, data_hi, data_lo);
//
// clk          (in)  clock
// rst          (in)  reset
// addr         (in)  write address input [oc8051_ram_wr_sel.out]
// data_in      (in)  destination 1 from alu [oc8051_alu.des1]
// data2_in     (in)  destination 2 from alu [oc8051_alu.des2]
// wr           (in)  write to ram [oc8051_decoder.wr -r]
// wd2          (in)  write from destination 2 [oc8051_decoder.ram_wr_sel -r]
// wr_bit       (in)  write bit addresable [oc8051_decoder.bit_addr -r]
// data_hi      (out) output (high bits) [oc8051_alu_src3_sel.dptr, oc8051_ext_addr_sel.dptr_hi, oc8051_ram_sel.dptr_hi]
// data_lo      (out) output (low bits) [oc8051_ext_addr_sel.dptr_lo]
//


input clk, rst, wr, wr_bit;
input [1:0] wr_sfr;
input [7:0] addr, data_in, data2_in;

output [7:0] data_hi, data_lo;

reg [7:0] data_hi, data_lo;

always @(posedge clk or posedge rst)
begin
  if (rst) begin
    data_hi <= #1 `OC8051_RST_DPH;
    data_lo <= #1 `OC8051_RST_DPL;
  end else if (wr_sfr==`OC8051_WRS_DPTR) begin
//
//write from destination 2 and 1
    data_hi <= #1 data2_in;
    data_lo <= #1 data_in;
  end else if ((addr==`OC8051_SFR_DPTR_HI) & (wr) & !(wr_bit))
//
//case of writing to dptr
    data_hi <= #1 data_in;
  else if ((addr==`OC8051_SFR_DPTR_LO) & (wr) & !(wr_bit))
    data_lo <= #1 data_in;
end

endmodule

//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 instruction cache                                      ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////  8051 instruction cache                                      ////
////                                                              ////
////  To Do:                                                      ////
////    nothing                                                   ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.8  2003/06/20 13:36:37  simont
// ram modules added.
//
// Revision 1.7  2003/05/05 10:35:35  simont
// change to fit xrom.
//
// Revision 1.6  2003/04/03 19:15:37  simont
// fix some bugs, use oc8051_cache_ram.
//
// Revision 1.5  2003/04/02 11:22:15  simont
// fix bug.
//
// Revision 1.4  2003/01/21 14:08:18  simont
// fix bugs
//
// Revision 1.3  2003/01/13 14:14:41  simont
// replace some modules
//
// Revision 1.2  2002/10/24 13:34:02  simont
// add parameters for instruction cache
//
// Revision 1.1  2002/10/23 16:55:36  simont
// fix bugs in instruction interface
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"

module oc8051_icache (rst, clk, 
             adr_i, dat_o, stb_i, ack_o, cyc_i,
             adr_o, dat_i, stb_o, ack_i, cyc_o
`ifdef OC8051_BIST
         ,
         scanb_rst,
         scanb_clk,
         scanb_si,
         scanb_so,
         scanb_en
`endif
	     );
//
// rst           (in)  reset - pin
// clk           (in)  clock - pini
input rst, clk;

//
// interface to oc8051 cpu
//
// adr_i    (in)  address
// dat_o    (out) data output
// stb_i    (in)  strobe
// ack_o    (out) acknowledge
// cyc_i    (in)  cycle
input         stb_i,
              cyc_i;
input  [15:0] adr_i;
output        ack_o;
output [31:0] dat_o;
reg    [31:0] dat_o;

//
// interface to instruction rom
//
// adr_o    (out) address
// dat_i    (in)  data input
// stb_o    (out) strobe
// ack_i    (in) acknowledge
// cyc_o    (out)  cycle
input         ack_i;
input  [31:0] dat_i;
output        stb_o, 
              cyc_o;
output [15:0] adr_o;
reg           stb_o,
              cyc_o;

`ifdef OC8051_BIST
input   scanb_rst;
input   scanb_clk;
input   scanb_si;
output  scanb_so;
input   scanb_en;
`endif

parameter ADR_WIDTH = 6; // cache address wihth
parameter LINE_WIDTH = 2; // line address width (2 => 4x32)
parameter BL_WIDTH = ADR_WIDTH - LINE_WIDTH; // block address width
parameter BL_NUM = 15; // number of blocks (2^BL_WIDTH-1)
parameter CACHE_RAM = 64; // cache ram x 32 (2^ADR_WIDTH)

//
// internal buffers adn wires
//
// con_buf control buffer, contains upper addresses [15:ADDR_WIDTH1] in cache
reg [13-ADR_WIDTH:0] con_buf [BL_NUM:0];
// valid[x]=1 if block x is valid;
reg [BL_NUM:0] valid;
// con0, con2 contain temporal control information of current address and corrent address+2
// part of con_buf memory
reg [13-ADR_WIDTH:0] con0, con2;
//current upper address,
reg [13-ADR_WIDTH:0] cadr0, cadr2;
reg stb_b;
// byte_select in 32 bit line (adr_i[1:0])
reg [1:0] byte_sel;
// read cycle
reg [LINE_WIDTH-1:0] cyc;
// data input from cache ram
reg [31:0] data1_i;
// temporaly data from ram
reg [15:0] tmp_data1;
reg wr1, wr1_t, stb_it;

////////////////

reg vaild_h, vaild_l;


wire [31:0] data0, data1_o;
wire cy, cy1;
wire [BL_WIDTH-1:0] adr_i2;
wire hit, hit_l, hit_h;
wire [ADR_WIDTH-1:0] adr_r, addr1;
reg [ADR_WIDTH-1:0] adr_w;
reg [15:0] mis_adr;
wire [15:0] data1;
wire [LINE_WIDTH-1:0] adr_r1;


assign cy = &adr_i[LINE_WIDTH+1:1];
assign {cy1, adr_i2} = {1'b0, adr_i[ADR_WIDTH+1:LINE_WIDTH+2]}+cy;
assign hit_l = (con0==cadr0) & vaild_l;
assign hit_h = (con2==cadr2) & vaild_h;
assign hit = hit_l && hit_h;

assign adr_r = adr_i[ADR_WIDTH+1:2] + adr_i[1];
assign addr1 = wr1 ? adr_w : adr_r;
assign adr_r1 = adr_r[LINE_WIDTH-1:0] + 2'b01;
assign ack_o = hit && stb_it;

assign data1 = wr1_t ? tmp_data1 : data1_o[15:0];

assign adr_o = {mis_adr[15:LINE_WIDTH+2], cyc, 2'b00};


oc8051_ram_64x32_dual_bist oc8051_cache_ram(
                           .clk     ( clk        ),
                           .rst     ( rst        ),
			   .adr0    ( adr_i[ADR_WIDTH+1:2] ),
			   .dat0_o  ( data0      ),
			   .en0     ( 1'b1       ),
			   .adr1    ( addr1      ),
			   .dat1_o  ( data1_o    ),
			   .dat1_i  ( data1_i    ),
			   .en1     ( 1'b1       ),
			   .wr1     ( wr1        )
`ifdef OC8051_BIST
         ,
         .scanb_rst(scanb_rst),
         .scanb_clk(scanb_clk),
         .scanb_si(scanb_soi),
         .scanb_so(scanb_so),
         .scanb_en(scanb_en)
`endif
			   );

defparam oc8051_cache_ram.ADR_WIDTH = ADR_WIDTH;


always @(stb_b or data0 or data1 or byte_sel)
begin
  if (stb_b) begin
    case (byte_sel) /* synopsys full_case parallel_case */
      2'b00  : dat_o = data0;
      2'b01  : dat_o = {data1[7:0],   data0[31:8]};
      2'b10  : dat_o = {data1[15:0],  data0[31:16]};
      2'b11  : dat_o = {8'h00, data1, data0[31:24]};
    endcase
  end else begin
    dat_o = 32'h0;
  end
end

always @(posedge clk or posedge rst)
begin
  if (rst)
    begin
        con0 <= #1 9'h0;
        con2 <= #1 9'h0;
        vaild_h <= #1 1'b0;
	vaild_l <= #1 1'b0;
    end
  else
    begin
        con0 <= #1 {con_buf[adr_i[ADR_WIDTH+1:LINE_WIDTH+2]]};
        con2 <= #1 {con_buf[adr_i2]};
	vaild_l <= #1 valid[adr_i[ADR_WIDTH+1:LINE_WIDTH+2]];
	vaild_h <= #1 valid[adr_i2];
    end
end

always @(posedge clk or posedge rst)
begin
  if (rst) begin
    cadr0 <= #1 8'h00;
    cadr2 <= #1 8'h00;
  end else begin
    cadr0 <= #1 adr_i[15:ADR_WIDTH+2];
    cadr2 <= #1 adr_i[15:ADR_WIDTH+2]+ cy1;
  end
end

always @(posedge clk or posedge rst)
begin
  if (rst) begin
    stb_b <= #1 1'b0;
    byte_sel <= #1 2'b00;
  end else begin
    stb_b <= #1 stb_i;
    byte_sel <= #1 adr_i[1:0];
  end
end

always @(posedge clk or posedge rst)
begin
  if (rst)
    begin
        cyc    <= #1 2'b00;
        cyc_o  <= #1 1'b0;
        stb_o  <= #1 1'b0;
        data1_i<= #1 32'h0;
        wr1    <= #1 1'b0;
        adr_w  <= #1 6'h0;
        valid  <= #1 16'h0;
    end
  else if (stb_b && !hit && !stb_o && !wr1)
    begin
        cyc     <= #1 2'b00;
        cyc_o   <= #1 1'b1;
        stb_o   <= #1 1'b1;
        data1_i <= #1 32'h0;
        wr1     <= #1 1'b0;
    end
  else if (stb_o && ack_i)
    begin
        data1_i<= #1 dat_i; ///??
        wr1    <= #1 1'b1;
        adr_w  <= #1 adr_o[ADR_WIDTH+1:2];

        if (&cyc)
          begin
              cyc   <= #1 2'b00;
              cyc_o <= #1 1'b0;
              stb_o <= #1 1'b0;
              valid[mis_adr[ADR_WIDTH+1:LINE_WIDTH+2]] <= #1 1'b1;
          end
        else
          begin
              cyc   <= #1 cyc + 1'b1;
              cyc_o <= #1 1'b1;
              stb_o <= #1 1'b1;
              valid[mis_adr[ADR_WIDTH+1:LINE_WIDTH+2]] <= #1 1'b0;
          end
    end
  else
    wr1 <= #1 1'b0;
end

//rih
always @(posedge clk)
  if ( ~(stb_b && !hit && !stb_o && !wr1) & (stb_o && ack_i && cyc) )
    con_buf[mis_adr[ADR_WIDTH+1:LINE_WIDTH+2]] <= #1 mis_adr[15:ADR_WIDTH+2];


always @(posedge clk or posedge rst)
begin
  if (rst)
    mis_adr <= #1 1'b0;
  else if (!hit_l)
    mis_adr <= #1 adr_i;
  else if (!hit_h)
    mis_adr <= #1 adr_i+'d2;
end

always @(posedge clk or posedge rst)
begin
  if (rst)
    tmp_data1 <= #1 1'b0;
  else if (!hit_h && wr1 && (cyc==adr_r1))
//    tmp_data1 <= #1 dat_i[31:16]; //???
    tmp_data1 <= #1 dat_i[15:0]; //???
  else if (!hit_l && hit_h && wr1)
//    tmp_data1 <= #1 data1_o[31:16];
    tmp_data1 <= #1 data1_o[15:0]; //??
end

always @(posedge clk or posedge rst)
begin
  if (rst) begin
    wr1_t <= #1 1'b0;
    stb_it <= #1 1'b0;
  end else begin
    wr1_t <= #1 wr1;
    stb_it <= #1 stb_i;
  end
end

endmodule

//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 indirect address                                       ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   Contains ragister 0 and register 1. used for indirrect     ////
////   addressing.                                                ////
////                                                              ////
////  To Do:                                                      ////
////   nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.6  2003/05/05 15:46:37  simont
// add aditional alu destination to solve critical path.
//
// Revision 1.5  2003/01/13 14:14:41  simont
// replace some modules
//
// Revision 1.4  2002/09/30 17:33:59  simont
// prepared header
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on


module oc8051_indi_addr (clk, rst, wr_addr, data_in, wr, wr_bit, ri_out, sel, bank);
//


input        clk,	// clock
             rst,	// reset
	     wr,	// write
             sel,	// select register
	     wr_bit;	// write bit addressable
input  [1:0] bank;	// select register bank
input  [7:0] data_in;	// data input
input  [7:0] wr_addr;	// write address

output [7:0] ri_out;

//reg [7:0] buff [31:0];
reg wr_bit_r;


reg [7:0] buff [0:7];

//
//write to buffer
always @(posedge clk or posedge rst)
begin
  if (rst) begin
    buff[3'b000] <= #1 8'h00;
    buff[3'b001] <= #1 8'h00;
    buff[3'b010] <= #1 8'h00;
    buff[3'b011] <= #1 8'h00;
    buff[3'b100] <= #1 8'h00;
    buff[3'b101] <= #1 8'h00;
    buff[3'b110] <= #1 8'h00;
    buff[3'b111] <= #1 8'h00;
  end else begin
    if ((wr) & !(wr_bit_r)) begin
      case (wr_addr) /* synopsys full_case parallel_case */
        8'h00: buff[3'b000] <= #1 data_in;
        8'h01: buff[3'b001] <= #1 data_in;
        8'h08: buff[3'b010] <= #1 data_in;
        8'h09: buff[3'b011] <= #1 data_in;
        8'h10: buff[3'b100] <= #1 data_in;
        8'h11: buff[3'b101] <= #1 data_in;
        8'h18: buff[3'b110] <= #1 data_in;
        8'h19: buff[3'b111] <= #1 data_in;
      endcase
    end
  end
end

//
//read from buffer

assign ri_out = (({3'b000, bank, 2'b00, sel}==wr_addr) & (wr) & !wr_bit_r) ?
                 data_in : buff[{bank, sel}];



always @(posedge clk or posedge rst)
  if (rst) begin
    wr_bit_r <= #1 1'b0;
  end else begin
    wr_bit_r <= #1 wr_bit;
  end

endmodule
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 cores interrupt control module                         ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   contains sfr's: tcon, ip, ie;                              ////
////   interrupt handling                                         ////
////                                                              ////
////  To Do:                                                      ////
////   Nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
////      - Jaka Simsic, jakas@opencores.org                      ////
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
// Revision 1.9  2003/06/03 17:12:05  simont
// fix some bugs.
//
// Revision 1.8  2003/04/07 14:58:02  simont
// change sfr's interface.
//
// Revision 1.7  2003/03/28 17:45:57  simont
// change module name.
//
// Revision 1.6  2003/01/13 14:14:41  simont
// replace some modules
//
// Revision 1.5  2002/09/30 17:33:59  simont
// prepared header
//
//


// `include "oc8051_defines.v"

//synopsys translate_off
// `include "oc8051_timescale.v"
//synopsys translate_on



module oc8051_int (clk, rst, 
        wr_addr,  
	data_in, bit_in, 
	wr, wr_bit,
//timer interrupts
        tf0, tf1, t2_int,
	tr0, tr1,
//external interrupts
        ie0, ie1,
//uart interrupts
        uart_int,
//to cpu
        intr, reti, int_vec, ack,
//registers
	ie, tcon, ip);

input [7:0] wr_addr, data_in;
input wr, tf0, tf1, t2_int, ie0, ie1, clk, rst, reti, wr_bit, bit_in, ack, uart_int;

output tr0, tr1, intr;
output [7:0] int_vec,
             ie,
	     tcon,
	     ip;

reg [7:0] ip, ie, int_vec;

reg [3:0] tcon_s;
reg tcon_tf1, tcon_tf0, tcon_ie1, tcon_ie0;

//
// isrc		processing interrupt sources
// int_dept
wire [2:0] isrc_cur;
reg [2:0] isrc [1:0];
reg [1:0] int_dept;
wire [1:0] int_dept_1;
reg int_proc;
reg [1:0] int_lev [1:0];
wire cur_lev;

assign isrc_cur = int_proc ? isrc[int_dept_1] : 2'h0;
assign int_dept_1 = int_dept - 2'b01;
assign cur_lev = int_lev[int_dept_1];

//
// contains witch level of interrupts is running
//reg [1:0] int_levl, int_levl_w;

//
// int_ln	waiting interrupts on level n
// ip_ln	interrupts on level n
// int_src	interrupt sources
wire [5:0] int_l0, int_l1;
wire [5:0] ip_l0, ip_l1;
wire [5:0] int_src;
wire il0, il1;


reg tf0_buff, tf1_buff, ie0_buff, ie1_buff;

//
//interrupt priority
assign ip_l0 = ~ip[5:0];
assign ip_l1 = ip[5:0];

assign int_src = {t2_int, uart_int, tcon_tf1, tcon_ie1, tcon_tf0, tcon_ie0};

//
// waiting interrupts
assign int_l0 = ip_l0 & {ie[5:0]} & int_src;
assign int_l1 = ip_l1 & {ie[5:0]} & int_src;
assign il0 = |int_l0;
assign il1 = |int_l1;

//
// TCON
assign tcon = {tcon_tf1, tcon_s[3], tcon_tf0, tcon_s[2], tcon_ie1, tcon_s[1], tcon_ie0, tcon_s[0]};
assign tr0 = tcon_s[2];
assign tr1 = tcon_s[3];
assign intr = |int_vec;


//
// IP
always @(posedge clk or posedge rst)
begin
 if (rst) begin
   ip <=#1 `OC8051_RST_IP;
 end else if ((wr) & !(wr_bit) & (wr_addr==`OC8051_SFR_IP)) begin
   ip <= #1 data_in;
 end else if ((wr) & (wr_bit) & (wr_addr[7:3]==`OC8051_SFR_B_IP))
   ip[wr_addr[2:0]] <= #1 bit_in;
end

//
// IE
always @(posedge clk or posedge rst)
begin
 if (rst) begin
   ie <=#1 `OC8051_RST_IE;
 end else if ((wr) & !(wr_bit) & (wr_addr==`OC8051_SFR_IE)) begin
   ie <= #1 data_in;
 end else if ((wr) & (wr_bit) & (wr_addr[7:3]==`OC8051_SFR_B_IE))
   ie[wr_addr[2:0]] <= #1 bit_in;
end

//
// tcon_s
//
always @(posedge clk or posedge rst)
begin
 if (rst) begin
   tcon_s <=#1 4'b0000;
 end else if ((wr) & !(wr_bit) & (wr_addr==`OC8051_SFR_TCON)) begin
   tcon_s <= #1 {data_in[6], data_in[4], data_in[2], data_in[0]};
 end else if ((wr) & (wr_bit) & (wr_addr[7:3]==`OC8051_SFR_B_TCON)) begin
   case (wr_addr[2:0]) /* synopsys full_case parallel_case */
     3'b000: tcon_s[0] <= #1 bit_in;
     3'b010: tcon_s[1] <= #1 bit_in;
     3'b100: tcon_s[2] <= #1 bit_in;
     3'b110: tcon_s[3] <= #1 bit_in;
   endcase
 end
end

//
// tf1 (tmod.7)
//
always @(posedge clk or posedge rst)
begin
 if (rst) begin
   tcon_tf1 <=#1 1'b0;
 end else if ((wr) & !(wr_bit) & (wr_addr==`OC8051_SFR_TCON)) begin
   tcon_tf1 <= #1 data_in[7];
 end else if ((wr) & (wr_bit) & (wr_addr=={`OC8051_SFR_B_TCON, 3'b111})) begin
   tcon_tf1 <= #1 bit_in;
 end else if (!(tf1_buff) & (tf1)) begin
   tcon_tf1 <= #1 1'b1;
 end else if (ack & (isrc_cur==`OC8051_ISRC_TF1)) begin
   tcon_tf1 <= #1 1'b0;
 end
end

//
// tf0 (tmod.5)
//
always @(posedge clk or posedge rst)
begin
 if (rst) begin
   tcon_tf0 <=#1 1'b0;
 end else if ((wr) & !(wr_bit) & (wr_addr==`OC8051_SFR_TCON)) begin
   tcon_tf0 <= #1 data_in[5];
 end else if ((wr) & (wr_bit) & (wr_addr=={`OC8051_SFR_B_TCON, 3'b101})) begin
   tcon_tf0 <= #1 bit_in;
 end else if (!(tf0_buff) & (tf0)) begin
   tcon_tf0 <= #1 1'b1;
 end else if (ack & (isrc_cur==`OC8051_ISRC_TF0)) begin
   tcon_tf0 <= #1 1'b0;
 end
end


//
// ie0 (tmod.1)
//
always @(posedge clk or posedge rst)
begin
 if (rst) begin
   tcon_ie0 <=#1 1'b0;
 end else if ((wr) & !(wr_bit) & (wr_addr==`OC8051_SFR_TCON)) begin
   tcon_ie0 <= #1 data_in[1];
 end else if ((wr) & (wr_bit) & (wr_addr=={`OC8051_SFR_B_TCON, 3'b001})) begin
   tcon_ie0 <= #1 bit_in;
 end else if (((tcon_s[0]) & (ie0_buff) & !(ie0)) | (!(tcon_s[0]) & !(ie0))) begin
   tcon_ie0 <= #1 1'b1;
 end else if (ack & (isrc_cur==`OC8051_ISRC_IE0) & (tcon_s[0])) begin
   tcon_ie0 <= #1 1'b0;
 end else if (!(tcon_s[0]) & (ie0)) begin
   tcon_ie0 <= #1 1'b0;
 end
end


//
// ie1 (tmod.3)
//
always @(posedge clk or posedge rst)
begin
 if (rst) begin
   tcon_ie1 <=#1 1'b0;
 end else if ((wr) & !(wr_bit) & (wr_addr==`OC8051_SFR_TCON)) begin
   tcon_ie1 <= #1 data_in[3];
 end else if ((wr) & (wr_bit) & (wr_addr=={`OC8051_SFR_B_TCON, 3'b011})) begin
   tcon_ie1 <= #1 bit_in;
 end else if (((tcon_s[1]) & (ie1_buff) & !(ie1)) | (!(tcon_s[1]) & !(ie1))) begin
   tcon_ie1 <= #1 1'b1;
 end else if (ack & (isrc_cur==`OC8051_ISRC_IE1) & (tcon_s[1])) begin
   tcon_ie1 <= #1 1'b0;
 end else if (!(tcon_s[1]) & (ie1)) begin
   tcon_ie1 <= #1 1'b0;
 end
end

//
// interrupt processing
always @(posedge clk or posedge rst)
begin
  if (rst) begin
    int_vec <= #1 8'h00;
    int_dept <= #1 2'b0;
    isrc[0] <= #1 3'h0;
    isrc[1] <= #1 3'h0;
    int_proc <= #1 1'b0;
    int_lev[0] <= #1 1'b0;
    int_lev[1] <= #1 1'b0;
  end else if (reti & int_proc) begin  // return from interrupt
   if (int_dept==2'b01)
     int_proc <= #1 1'b0;
   int_dept <= #1 int_dept - 2'b01;
  end else if (((ie[7]) & (!cur_lev) || !int_proc) & il1) begin  // interrupt on level 1
   int_proc <= #1 1'b1;
   int_lev[int_dept] <= #1 `OC8051_ILEV_L1;
   int_dept <= #1 int_dept + 2'b01;
   if (int_l1[0]) begin
     int_vec <= #1 `OC8051_INT_X0;
     isrc[int_dept] <= #1 `OC8051_ISRC_IE0;
   end else if (int_l1[1]) begin
     int_vec <= #1 `OC8051_INT_T0;
     isrc[int_dept] <= #1 `OC8051_ISRC_TF0;
   end else if (int_l1[2]) begin
     int_vec <= #1 `OC8051_INT_X1;
     isrc[int_dept] <= #1 `OC8051_ISRC_IE1;
   end else if (int_l1[3]) begin
     int_vec <= #1 `OC8051_INT_T1;
     isrc[int_dept] <= #1 `OC8051_ISRC_TF1;
   end else if (int_l1[4]) begin
     int_vec <= #1 `OC8051_INT_UART;
     isrc[int_dept] <= #1 `OC8051_ISRC_UART;
   end else if (int_l1[5]) begin
     int_vec <= #1 `OC8051_INT_T2;
     isrc[int_dept] <= #1 `OC8051_ISRC_T2;
   end

 end else if ((ie[7]) & !int_proc & il0) begin  // interrupt on level 0
   int_proc <= #1 1'b1;
   int_lev[int_dept] <= #1 `OC8051_ILEV_L0;
   int_dept <= #1 2'b01;
   if (int_l0[0]) begin
     int_vec <= #1 `OC8051_INT_X0;
     isrc[int_dept] <= #1 `OC8051_ISRC_IE0;
   end else if (int_l0[1]) begin
     int_vec <= #1 `OC8051_INT_T0;
     isrc[int_dept] <= #1 `OC8051_ISRC_TF0;
   end else if (int_l0[2]) begin
     int_vec <= #1 `OC8051_INT_X1;
     isrc[int_dept] <= #1 `OC8051_ISRC_IE1;
   end else if (int_l0[3]) begin
     int_vec <= #1 `OC8051_INT_T1;
     isrc[int_dept] <= #1 `OC8051_ISRC_TF1;
   end else if (int_l0[4]) begin
     int_vec <= #1 `OC8051_INT_UART;
     isrc[int_dept] <= #1 `OC8051_ISRC_UART;
   end else if (int_l0[5]) begin
     int_vec <= #1 `OC8051_INT_T2;
     isrc[int_dept] <= #1 `OC8051_ISRC_T2;
   end
 end else begin
   int_vec <= #1 8'h00;
 end
end


always @(posedge clk or posedge rst)
  if (rst) begin
    tf0_buff <= #1 1'b0;
    tf1_buff <= #1 1'b0;
    ie0_buff <= #1 1'b0;
    ie1_buff <= #1 1'b0;
  end else begin
    tf0_buff <= #1 tf0;
    tf1_buff <= #1 tf1;
    ie0_buff <= #1 ie0;
    ie1_buff <= #1 ie1;
  end

endmodule
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 memory interface                                       ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   comunication betwen cpu and memory                         ////
////                                                              ////
////  To Do:                                                      ////
////   nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.12  2003/07/01 20:47:39  simont
// add /* synopsys xx_case */ to case statments.
//
// Revision 1.11  2003/06/20 13:35:10  simont
// simualtion `ifdef added
//
// Revision 1.10  2003/06/05 11:15:02  simont
// fix bug.
//
// Revision 1.9  2003/06/03 17:09:57  simont
// pipelined acces to axternal instruction interface added.
//
// Revision 1.8  2003/05/12 16:27:40  simont
// fix bug in movc intruction.
//
// Revision 1.7  2003/05/06 09:39:34  simont
// cahnge assigment to pc_wait (remove istb_o)
//
// Revision 1.6  2003/05/05 15:46:37  simont
// add aditional alu destination to solve critical path.
//
// Revision 1.5  2003/04/25 17:15:51  simont
// change branch instruction execution (reduse needed clock periods).
//
// Revision 1.4  2003/04/16 10:04:09  simont
// chance idat_ir to 24 bit wide
//
// Revision 1.3  2003/04/11 10:05:08  simont
// Change pc add value from 23'h to 16'h
//
// Revision 1.2  2003/04/09 16:24:03  simont
// change wr_sft to 2 bit wire.
//
// Revision 1.1  2003/01/13 14:13:12  simont
// initial import
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"


module oc8051_memory_interface (clk, rst,

//decoder
     wr_i,
     wr_bit_i,
     rd_sel,
     wr_sel,
     pc_wr_sel,
     pc_wr,
     pc,
     rd,
     mem_wait,
     mem_act,
     istb,

//internal ram
     wr_o, 
     wr_bit_o, 
     rd_addr, 
     wr_addr, 
     rd_ind, 
     wr_ind, 
     wr_dat,

     bit_in, 
     in_ram, 
     sfr, 
     sfr_bit, 
     bit_out, 
     iram_out,

//program rom
     iadr_o, 
     ea, 
     ea_int,
     op1_out, 
     op2_out, 
     op3_out,

//internal
     idat_onchip,

//external
     iack_i, 
     istb_o, 
     idat_i,

//external data ram
     dadr_o, 
     dwe_o, 
     dstb_o, 
     dack_i,
     ddat_i, 
     ddat_o,

//interrupt interface
     intr, 
     int_v, 
     int_ack,

//alu
     des_acc, 
     des1, 
     des2,

//sfr's
     dptr, 
     ri, 
     sp,  
     sp_w, 
     rn, 
     acc, 
     reti
   );


input         clk,
              rst,
	      wr_i,
	      wr_bit_i;

input         bit_in,
              sfr_bit,
	      dack_i;
input [2:0]   mem_act;
input [7:0]   in_ram,
              sfr,
	      acc,
	      sp_w;
input [31:0]  idat_i;

output        bit_out,
              mem_wait,
	      reti;
output [7:0]  iram_out,
              wr_dat;

reg           bit_out,
              reti;
reg [7:0]     iram_out,
              sp_r;
reg           rd_addr_r;
output        wr_o,
              wr_bit_o;

//????
reg           dack_ir;
reg [7:0]     ddat_ir;
reg [23:0]    idat_ir;

/////////////////////////////
//
//  rom_addr_sel
//
/////////////////////////////
input         iack_i;
input [7:0]   des_acc,
              des1,
	      des2;
output [15:0] iadr_o;

wire          ea_rom_sel;

/////////////////////////////
//
// ext_addr_sel
//
/////////////////////////////
input [7:0]   ri,
              ddat_i;
input [15:0]  dptr;

output        dstb_o,
              dwe_o;
output [7:0]  ddat_o;
output [15:0] dadr_o;

/////////////////////////////
//
// ram_adr_sel
//
/////////////////////////////

input [2:0]   rd_sel,
              wr_sel;
input [4:0]   rn;
input [7:0]   sp;

output        rd_ind,
              wr_ind;
output [7:0]  wr_addr,
              rd_addr;
reg           rd_ind,
              wr_ind;
reg [7:0]     wr_addr,
              rd_addr;

reg [4:0]     rn_r;
reg [7:0]     ri_r,
              imm_r,
	      imm2_r,
	      op1_r;
wire [7:0]    imm,
              imm2;

/////////////////////////////
//
// op_select
//
/////////////////////////////

input         intr,
              rd,
	      ea, 
	      ea_int, 
	      istb;

input  [7:0]  int_v;

input  [31:0] idat_onchip;

output        int_ack,
              istb_o;

output  [7:0] op1_out,
              op3_out,
	      op2_out;

reg           int_ack_t,
              int_ack,
	      int_ack_buff;

reg [7:0]     int_vec_buff;
reg [7:0]     op1_out,
              op2_buff,
	      op3_buff;
reg [7:0]     op1_o,
              op2_o,
	      op3_o;

reg [7:0]     op1_xt, 
              op2_xt, 
	      op3_xt;

reg [7:0]     op1,
              op2,
	      op3;
wire [7:0]    op2_direct;

input [2:0]   pc_wr_sel;

input         pc_wr;
output [15:0] pc;

reg [15:0]    pc;

//
//pc            program counter register, save current value
reg [15:0]    pc_buf;
wire [15:0]   alu;


reg           int_buff,
              int_buff1; // interrupt buffer: used to prevent interrupting in the middle of executin instructions


//
//
////////////////////////////
reg           istb_t,
              imem_wait,
	      dstb_o,
	      dwe_o;

reg [7:0]     ddat_o;
reg [15:0]    iadr_t,
              dadr_ot;
reg           dmem_wait;
wire          pc_wait;
wire [1:0]    bank;
wire [7:0]    isr_call;

reg [1:0]     op_length;
reg [2:0]     op_pos;
wire          inc_pc;

reg           pc_wr_r;

wire [15:0]   pc_out;

reg [31:0]    idat_cur,
              idat_old;

reg           inc_pc_r,
              pc_wr_r2;

reg [7:0]     cdata;
reg           cdone;


assign bank       = rn[4:3];
assign imm        = op2_out;
assign imm2       = op3_out;
assign alu        = {des2, des_acc};
assign ea_rom_sel = ea && ea_int;
assign wr_o       = wr_i;
assign wr_bit_o   = wr_bit_i;

//assign mem_wait   = dmem_wait || imem_wait || pc_wr_r;
assign mem_wait   = dmem_wait || imem_wait || pc_wr_r2;
//assign mem_wait   = dmem_wait || imem_wait;
assign istb_o     = (istb || (istb_t & !iack_i)) && !dstb_o && !ea_rom_sel;

assign pc_wait    = rd && (ea_rom_sel || (!istb_t && iack_i));

assign wr_dat     = des1;


`ifdef OC8051_SIMULATION
  always @(negedge rst) begin
    #5
    if (ea_rom_sel)
      $display("   progran execution from external rom");
    else
      $display("   progran execution from internal rom");
  end

`endif


/////////////////////////////
//
//  ram_select
//
/////////////////////////////
always @(rd_addr_r or in_ram or sfr or bit_in or sfr_bit or rd_ind)
begin
  if (rd_addr_r && !rd_ind) begin
    iram_out = sfr;
    bit_out = sfr_bit;
  end else begin
    iram_out = in_ram;
    bit_out = bit_in;
  end
end

/////////////////////////////
//
// ram_adr_sel
//
/////////////////////////////

always @(rd_sel or sp or ri or rn or imm or dadr_o[15:0] or bank)
begin
  case (rd_sel) /* synopsys full_case parallel_case */
    `OC8051_RRS_RN   : rd_addr = {3'h0, rn};
    `OC8051_RRS_I    : rd_addr = ri;
    `OC8051_RRS_D    : rd_addr = imm;
    `OC8051_RRS_SP   : rd_addr = sp;

    `OC8051_RRS_B    : rd_addr = `OC8051_SFR_B;
    `OC8051_RRS_DPTR : rd_addr = `OC8051_SFR_DPTR_LO;
    `OC8051_RRS_PSW  : rd_addr = `OC8051_SFR_PSW;
    `OC8051_RRS_ACC  : rd_addr = `OC8051_SFR_ACC;
//    default          : rd_addr = 2'bxx;
  endcase

end


//
//
always @(wr_sel or sp_w or rn_r or imm_r or ri_r or imm2_r or op1_r or dadr_o[15:0])
begin
  case (wr_sel) /* synopsys full_case parallel_case */
    `OC8051_RWS_RN : wr_addr = {3'h0, rn_r};
    `OC8051_RWS_I  : wr_addr = ri_r;
    `OC8051_RWS_D  : wr_addr = imm_r;
    `OC8051_RWS_SP : wr_addr = sp_w;
    `OC8051_RWS_D3 : wr_addr = imm2_r;
    `OC8051_RWS_B  : wr_addr = `OC8051_SFR_B;
//    default        : wr_addr = 2'bxx;
  endcase
end

always @(posedge clk or posedge rst)
  if (rst)
    rd_ind <= #1 1'b0;
  else if ((rd_sel==`OC8051_RRS_I) || (rd_sel==`OC8051_RRS_SP))
    rd_ind <= #1 1'b1;
  else
    rd_ind <= #1 1'b0;

always @(wr_sel)
  if ((wr_sel==`OC8051_RWS_I) || (wr_sel==`OC8051_RWS_SP))
    wr_ind = 1'b1;
  else
    wr_ind = 1'b0;


/////////////////////////////
//
//  rom_addr_sel
//
/////////////////////////////
//
// output address is alu destination
// (instructions MOVC)

//assign iadr_o = (istb_t & !iack_i) ? iadr_t : pc_out;
assign iadr_o = (istb_t) ? iadr_t : pc_out;


always @(posedge clk or posedge rst)
begin
  if (rst) begin
    iadr_t <= #1 23'h0;
    istb_t <= #1 1'b0;
    imem_wait <= #1 1'b0;
    idat_ir <= #1 24'h0;
  end else if (mem_act==`OC8051_MAS_CODE) begin
    iadr_t <= #1 alu;
    istb_t <= #1 1'b1;
    imem_wait <= #1 1'b1;
  end else if (ea_rom_sel && imem_wait) begin
    imem_wait <= #1 1'b0;
  end else if (!imem_wait && istb_t) begin
    istb_t <= #1 1'b0;
  end else if (iack_i) begin
    imem_wait <= #1 1'b0;
    idat_ir <= #1 idat_i [23:0];
  end
end

/////////////////////////////
//
// ext_addr_sel
//
/////////////////////////////

assign dadr_o = dadr_ot;

always @(posedge clk or posedge rst)
begin
  if (rst) begin
    dwe_o <= #1 1'b0;
    dmem_wait <= #1 1'b0;
    dstb_o <= #1 1'b0;
    ddat_o <= #1 8'h00;
    dadr_ot <= #1 23'h0;
  end else if (dack_i) begin
    dwe_o <= #1 1'b0;
    dstb_o <= #1 1'b0;
    dmem_wait <= #1 1'b0;
  end else begin
    case (mem_act) /* synopsys full_case parallel_case */
      `OC8051_MAS_DPTR_R: begin  // read from external rom: acc=(dptr)
        dwe_o <= #1 1'b0;
        dstb_o <= #1 1'b1;
        ddat_o <= #1 8'h00;
        dadr_ot <= #1 {7'h0, dptr};
        dmem_wait <= #1 1'b1;
      end
      `OC8051_MAS_DPTR_W: begin  // write to external rom: (dptr)=acc
        dwe_o <= #1 1'b1;
        dstb_o <= #1 1'b1;
        ddat_o <= #1 acc;
        dadr_ot <= #1 {7'h0, dptr};
        dmem_wait <= #1 1'b1;
      end
      `OC8051_MAS_RI_R:   begin  // read from external rom: acc=(Ri)
        dwe_o <= #1 1'b0;
        dstb_o <= #1 1'b1;
        ddat_o <= #1 8'h00;
        dadr_ot <= #1 {15'h0, ri};
        dmem_wait <= #1 1'b1;
      end
      `OC8051_MAS_RI_W: begin    // write to external rom: (Ri)=acc
        dwe_o <= #1 1'b1;
        dstb_o <= #1 1'b1;
        ddat_o <= #1 acc;
        dadr_ot <= #1 {15'h0, ri};
        dmem_wait <= #1 1'b1;
      end
    endcase
  end
end

/////////////////////////////
//
// op_select
//
/////////////////////////////



always @(posedge clk or posedge rst)
begin
  if (rst) begin
    idat_cur <= #1 32'h0;
    idat_old <= #1 32'h0;
  end else if ((iack_i | ea_rom_sel) & (inc_pc | pc_wr_r2)) begin
    idat_cur <= #1 ea_rom_sel ? idat_onchip : idat_i;
    idat_old <= #1 idat_cur;
  end

end

always @(posedge clk or posedge rst)
begin
  if (rst) begin
    cdata <= #1 8'h00;
    cdone <= #1 1'b0;
  end else if (istb_t) begin
    cdata <= #1 ea_rom_sel ? idat_onchip[7:0] : idat_i[7:0];
    cdone <= #1 1'b1;
  end else begin
    cdone <= #1 1'b0;
  end
end

always @(op_pos or idat_cur or idat_old)
begin
  case (op_pos)  /* synopsys parallel_case */
    3'b000: begin
       op1 = idat_old[7:0]  ;
       op2 = idat_old[15:8] ;
       op3 = idat_old[23:16];
      end
    3'b001: begin
       op1 = idat_old[15:8] ;
       op2 = idat_old[23:16];
       op3 = idat_old[31:24];
      end
    3'b010: begin
       op1 = idat_old[23:16];
       op2 = idat_old[31:24];
       op3 = idat_cur[7:0]  ;
      end
    3'b011: begin
       op1 = idat_old[31:24];
       op2 = idat_cur[7:0]  ;
       op3 = idat_cur[15:8] ;
      end
    3'b100: begin
       op1 = idat_cur[7:0]  ;
       op2 = idat_cur[15:8] ;
       op3 = idat_cur[23:16];
      end
    default: begin
       op1 = idat_cur[15:8] ;
       op2 = idat_cur[23:16];
       op3 = idat_cur[31:24];
      end
  endcase
end

/*assign op1 = ea_rom_sel ? idat_onchip[7:0]   : op1_xt;
assign op2 = ea_rom_sel ? idat_onchip[15:8]  : op2_xt;
assign op3 = ea_rom_sel ? idat_onchip[23:16] : op3_xt;*/


always @(dack_ir or ddat_ir or op1_o or iram_out or cdone or cdata)
  if (dack_ir)
    op1_out = ddat_ir;
  else if (cdone)
    op1_out = cdata;
  else
    op1_out = op1_o;

assign op3_out = (rd) ? op3_o : op3_buff;
assign op2_out = (rd) ? op2_o : op2_buff;

always @(idat_i or iack_i or idat_ir or rd)
begin
  if (iack_i) begin
    op1_xt = idat_i[7:0];
    op2_xt = idat_i[15:8];
    op3_xt = idat_i[23:16];
  end else if (!rd) begin
    op1_xt = idat_ir[7:0];
    op2_xt = idat_ir[15:8];
    op3_xt = idat_ir[23:16];
  end else begin
    op1_xt = 8'h00;
    op2_xt = 8'h00;
    op3_xt = 8'h00;
  end
end


//
// in case of interrupts
always @(op1 or op2 or op3 or int_ack_t or int_vec_buff or iack_i or ea_rom_sel)
begin
  if (int_ack_t && (iack_i || ea_rom_sel)) begin
    op1_o = `OC8051_LCALL;
    op2_o = 8'h00;
    op3_o = int_vec_buff;
  end else begin
    op1_o = op1;
    op2_o = op2;
    op3_o = op3;
  end
end

//
//in case of reti
always @(posedge clk or posedge rst)
  if (rst) reti <= #1 1'b0;
  else if ((op1_o==`OC8051_RETI) & rd & !mem_wait) reti <= #1 1'b1;
  else reti <= #1 1'b0;

//
// remember inputs
always @(posedge clk or posedge rst)
begin
  if (rst) begin
    op2_buff <= #1 8'h0;
    op3_buff <= #1 8'h0;
  end else if (rd) begin
    op2_buff <= #1 op2_o;
    op3_buff <= #1 op3_o;
  end
end

/////////////////////////////
//
//  pc
//
/////////////////////////////

always @(op1_out)
begin
        casex (op1_out) /* synopsys parallel_case */
          `OC8051_ACALL :  op_length = 2'h2;
          `OC8051_AJMP :   op_length = 2'h2;

        //op_code [7:3]
          `OC8051_CJNE_R : op_length = 2'h3;
          `OC8051_DJNZ_R : op_length = 2'h2;
          `OC8051_MOV_DR : op_length = 2'h2;
          `OC8051_MOV_CR : op_length = 2'h2;
          `OC8051_MOV_RD : op_length = 2'h2;

        //op_code [7:1]
          `OC8051_CJNE_I : op_length = 2'h3;
          `OC8051_MOV_ID : op_length = 2'h2;
          `OC8051_MOV_DI : op_length = 2'h2;
          `OC8051_MOV_CI : op_length = 2'h2;

        //op_code [7:0]
          `OC8051_ADD_D :  op_length = 2'h2;
          `OC8051_ADD_C :  op_length = 2'h2;
          `OC8051_ADDC_D : op_length = 2'h2;
          `OC8051_ADDC_C : op_length = 2'h2;
          `OC8051_ANL_D :  op_length = 2'h2;
          `OC8051_ANL_C :  op_length = 2'h2;
          `OC8051_ANL_DD : op_length = 2'h2;
          `OC8051_ANL_DC : op_length = 2'h3;
          `OC8051_ANL_B :  op_length = 2'h2;
          `OC8051_ANL_NB : op_length = 2'h2;
          `OC8051_CJNE_D : op_length = 2'h3;
          `OC8051_CJNE_C : op_length = 2'h3;
          `OC8051_CLR_B :  op_length = 2'h2;
          `OC8051_CPL_B :  op_length = 2'h2;
          `OC8051_DEC_D :  op_length = 2'h2;
          `OC8051_DJNZ_D : op_length = 2'h3;
          `OC8051_INC_D :  op_length = 2'h2;
          `OC8051_JB :     op_length = 2'h3;
          `OC8051_JBC :    op_length = 2'h3;
          `OC8051_JC :     op_length = 2'h2;
          `OC8051_JNB :    op_length = 2'h3;
          `OC8051_JNC :    op_length = 2'h2;
          `OC8051_JNZ :    op_length = 2'h2;
          `OC8051_JZ :     op_length = 2'h2;
          `OC8051_LCALL :  op_length = 2'h3;
          `OC8051_LJMP :   op_length = 2'h3;
          `OC8051_MOV_D :  op_length = 2'h2;
          `OC8051_MOV_C :  op_length = 2'h2;
          `OC8051_MOV_DA : op_length = 2'h2;
          `OC8051_MOV_DD : op_length = 2'h3;
          `OC8051_MOV_CD : op_length = 2'h3;
          `OC8051_MOV_BC : op_length = 2'h2;
          `OC8051_MOV_CB : op_length = 2'h2;
          `OC8051_MOV_DP : op_length = 2'h3;
          `OC8051_ORL_D :  op_length = 2'h2;
          `OC8051_ORL_C :  op_length = 2'h2;
          `OC8051_ORL_AD : op_length = 2'h2;
          `OC8051_ORL_CD : op_length = 2'h3;
          `OC8051_ORL_B :  op_length = 2'h2;
          `OC8051_ORL_NB : op_length = 2'h2;
          `OC8051_POP :    op_length = 2'h2;
          `OC8051_PUSH :   op_length = 2'h2;
          `OC8051_SETB_B : op_length = 2'h2;
          `OC8051_SJMP :   op_length = 2'h2;
          `OC8051_SUBB_D : op_length = 2'h2;
          `OC8051_SUBB_C : op_length = 2'h2;
          `OC8051_XCH_D :  op_length = 2'h2;
          `OC8051_XRL_D :  op_length = 2'h2;
          `OC8051_XRL_C :  op_length = 2'h2;
          `OC8051_XRL_AD : op_length = 2'h2;
          `OC8051_XRL_CD : op_length = 2'h3;
          default:         op_length = 2'h1;
        endcase
end

/*
always @(posedge clk or posedge rst)
begin
    if (rst) begin
      op_length = 2'h2;
//    end else if (pc_wait) begin
    end else begin
        casex (op1_out)
          `OC8051_ACALL :  op_length <= #1 2'h2;
          `OC8051_AJMP :   op_length <= #1 2'h2;

        //op_code [7:3]
          `OC8051_CJNE_R : op_length <= #1 2'h3;
          `OC8051_DJNZ_R : op_length <= #1 2'h2;
          `OC8051_MOV_DR : op_length <= #1 2'h2;
          `OC8051_MOV_CR : op_length <= #1 2'h2;
          `OC8051_MOV_RD : op_length <= #1 2'h2;

        //op_code [7:1]
          `OC8051_CJNE_I : op_length <= #1 2'h3;
          `OC8051_MOV_ID : op_length <= #1 2'h2;
          `OC8051_MOV_DI : op_length <= #1 2'h2;
          `OC8051_MOV_CI : op_length <= #1 2'h2;

        //op_code [7:0]
          `OC8051_ADD_D :  op_length <= #1 2'h2;
          `OC8051_ADD_C :  op_length <= #1 2'h2;
          `OC8051_ADDC_D : op_length <= #1 2'h2;
          `OC8051_ADDC_C : op_length <= #1 2'h2;
          `OC8051_ANL_D :  op_length <= #1 2'h2;
          `OC8051_ANL_C :  op_length <= #1 2'h2;
          `OC8051_ANL_DD : op_length <= #1 2'h2;
          `OC8051_ANL_DC : op_length <= #1 2'h3;
          `OC8051_ANL_B :  op_length <= #1 2'h2;
          `OC8051_ANL_NB : op_length <= #1 2'h2;
          `OC8051_CJNE_D : op_length <= #1 2'h3;
          `OC8051_CJNE_C : op_length <= #1 2'h3;
          `OC8051_CLR_B :  op_length <= #1 2'h2;
          `OC8051_CPL_B :  op_length <= #1 2'h2;
          `OC8051_DEC_D :  op_length <= #1 2'h2;
          `OC8051_DJNZ_D : op_length <= #1 2'h3;
          `OC8051_INC_D :  op_length <= #1 2'h2;
          `OC8051_JB :     op_length <= #1 2'h3;
          `OC8051_JBC :    op_length <= #1 2'h3;
          `OC8051_JC :     op_length <= #1 2'h2;
          `OC8051_JNB :    op_length <= #1 2'h3;
          `OC8051_JNC :    op_length <= #1 2'h2;
          `OC8051_JNZ :    op_length <= #1 2'h2;
          `OC8051_JZ :     op_length <= #1 2'h2;
          `OC8051_LCALL :  op_length <= #1 2'h3;
          `OC8051_LJMP :   op_length <= #1 2'h3;
          `OC8051_MOV_D :  op_length <= #1 2'h2;
          `OC8051_MOV_C :  op_length <= #1 2'h2;
          `OC8051_MOV_DA : op_length <= #1 2'h2;
          `OC8051_MOV_DD : op_length <= #1 2'h3;
          `OC8051_MOV_CD : op_length <= #1 2'h3;
          `OC8051_MOV_BC : op_length <= #1 2'h2;
          `OC8051_MOV_CB : op_length <= #1 2'h2;
          `OC8051_MOV_DP : op_length <= #1 2'h3;
          `OC8051_ORL_D :  op_length <= #1 2'h2;
          `OC8051_ORL_C :  op_length <= #1 2'h2;
          `OC8051_ORL_AD : op_length <= #1 2'h2;
          `OC8051_ORL_CD : op_length <= #1 2'h3;
          `OC8051_ORL_B :  op_length <= #1 2'h2;
          `OC8051_ORL_NB : op_length <= #1 2'h2;
          `OC8051_POP :    op_length <= #1 2'h2;
          `OC8051_PUSH :   op_length <= #1 2'h2;
          `OC8051_SETB_B : op_length <= #1 2'h2;
          `OC8051_SJMP :   op_length <= #1 2'h2;
          `OC8051_SUBB_D : op_length <= #1 2'h2;
          `OC8051_SUBB_C : op_length <= #1 2'h2;
          `OC8051_XCH_D :  op_length <= #1 2'h2;
          `OC8051_XRL_D :  op_length <= #1 2'h2;
          `OC8051_XRL_C :  op_length <= #1 2'h2;
          `OC8051_XRL_AD : op_length <= #1 2'h2;
          `OC8051_XRL_CD : op_length <= #1 2'h3;
          default:         op_length <= #1 2'h1;
        endcase
//
//in case of instructions that use more than one clock hold current pc
//    end else begin
//      pc= pc_buf;
   end
end
*/

assign inc_pc = ((op_pos[2] | (&op_pos[1:0])) & rd) | pc_wr_r2;

always @(posedge rst or posedge clk)
begin
  if (rst) begin
    op_pos <= #1 3'h0;
  end else if (pc_wr_r2) begin
    op_pos <= #1 3'h4;// - op_length;////****??????????
/*  end else if (inc_pc & rd) begin
    op_pos[2]   <= #1 op_pos[2] & !op_pos[1] & op_pos[0] & (&op_length);
    op_pos[1:0] <= #1 op_pos[1:0] + op_length;
//    op_pos   <= #1 {1'b0, op_pos[1:0]} + {1'b0, op_length};
  end else if (rd) begin
    op_pos <= #1 op_pos + {1'b0, op_length};
  end*/
  end else if (inc_pc & rd) begin
    op_pos[2]   <= #1 op_pos[2] & !op_pos[1] & op_pos[0] & (&op_length);
    op_pos[1:0] <= #1 op_pos[1:0] + op_length;
//    op_pos   <= #1 {1'b0, op_pos[1:0]} + {1'b0, op_length};
//  end else if (istb & rd) begin
  end else if (rd) begin
    op_pos <= #1 op_pos + {1'b0, op_length};
  end
end

//
// remember interrupt
// we don't want to interrupt instruction in the middle of execution
always @(posedge clk or posedge rst)
 if (rst) begin
   int_ack_t <= #1 1'b0;
   int_vec_buff <= #1 8'h00;
 end else if (intr) begin
   int_ack_t <= #1 1'b1;
   int_vec_buff <= #1 int_v;
 end else if (rd && (ea_rom_sel || iack_i) && !pc_wr_r2) int_ack_t <= #1 1'b0;

always @(posedge clk or posedge rst)
  if (rst) int_ack_buff <= #1 1'b0;
  else int_ack_buff <= #1 int_ack_t;

always @(posedge clk or posedge rst)
  if (rst) int_ack <= #1 1'b0;
  else begin
    if ((int_ack_buff) & !(int_ack_t))
      int_ack <= #1 1'b1;
    else int_ack <= #1 1'b0;
  end


//
//interrupt buffer
always @(posedge clk or posedge rst)
  if (rst) begin
    int_buff1 <= #1 1'b0;
  end else begin
    int_buff1 <= #1 int_buff;
  end

always @(posedge clk or posedge rst)
  if (rst) begin
    int_buff <= #1 1'b0;
  end else if (intr) begin
    int_buff <= #1 1'b1;
  end else if (pc_wait)
    int_buff <= #1 1'b0;

wire [7:0]  pcs_source;
reg  [15:0] pcs_result;
reg         pcs_cy;

assign pcs_source = pc_wr_sel[0] ? op3_out : op2_out;

always @(pcs_source or pc or pcs_cy)
begin
  if (pcs_source[7]) begin
    {pcs_cy, pcs_result[7:0]} = {1'b0, pc[7:0]} + {1'b0, pcs_source};
    pcs_result[15:8] = pc[15:8] - {7'h0, !pcs_cy};
  end else pcs_result = pc + {8'h00, pcs_source};
end

//assign pc = pc_buf - {13'h0, op_pos[2] | inc_pc_r, op_pos[1:0]}; ////******???
//assign pc = pc_buf - 16'h8 + {13'h0, op_pos}; ////******???
//assign pc = pc_buf - 16'h8 + {13'h0, op_pos} + {14'h0, op_length};

always @(posedge clk or posedge rst)
begin
  if (rst)
    pc <= #1 16'h0;
  else if (pc_wr_r2)
    pc <= #1 pc_buf;
  else if (rd & !int_ack_t)
    pc <= #1 pc_buf - 16'h8 + {13'h0, op_pos} + {14'h0, op_length};
end


always @(posedge clk or posedge rst)
begin
  if (rst) begin
    pc_buf <= #1 `OC8051_RST_PC;
  end else if (pc_wr) begin
//
//case of writing new value to pc (jupms)
      case (pc_wr_sel) /* synopsys full_case parallel_case */
        `OC8051_PIS_ALU: pc_buf        <= #1 alu;
        `OC8051_PIS_AL:  pc_buf[7:0]   <= #1 alu[7:0];
        `OC8051_PIS_AH:  pc_buf[15:8]  <= #1 alu[7:0];
        `OC8051_PIS_I11: pc_buf[10:0]  <= #1 {op1_out[7:5], op2_out};
        `OC8051_PIS_I16: pc_buf        <= #1 {op2_out, op3_out};
        `OC8051_PIS_SO1: pc_buf        <= #1 pcs_result;
        `OC8051_PIS_SO2: pc_buf        <= #1 pcs_result;
      endcase
//  end else if (inc_pc) begin
  end else begin
//
//or just remember current
      pc_buf <= #1 pc_out;
  end
end


assign pc_out = inc_pc ? pc_buf + 16'h4
                       : pc_buf ;





always @(posedge clk or posedge rst)
  if (rst)
    ddat_ir <= #1 8'h00;
  else if (dack_i)
    ddat_ir <= #1 ddat_i;

/*

always @(pc_buf or op1_out or pc_wait or int_buff or int_buff1 or ea_rom_sel or iack_i)
begin
    if (int_buff || int_buff1) begin
//
//in case of interrupt hold valut, to be written to stack
      pc= pc_buf;
//    end else if (pis_l) begin
//      pc = {pc_buf[22:8], alu[7:0]};
    end else if (pc_wait) begin
        casex (op1_out)
          `OC8051_ACALL :  pc= pc_buf + 16'h2;
          `OC8051_AJMP :   pc= pc_buf + 16'h2;

        //op_code [7:3]
          `OC8051_CJNE_R : pc= pc_buf + 16'h3;
          `OC8051_DJNZ_R : pc= pc_buf + 16'h2;
          `OC8051_MOV_DR : pc= pc_buf + 16'h2;
          `OC8051_MOV_CR : pc= pc_buf + 16'h2;
          `OC8051_MOV_RD : pc= pc_buf + 16'h2;

        //op_code [7:1]
          `OC8051_CJNE_I : pc= pc_buf + 16'h3;
          `OC8051_MOV_ID : pc= pc_buf + 16'h2;
          `OC8051_MOV_DI : pc= pc_buf + 16'h2;
          `OC8051_MOV_CI : pc= pc_buf + 16'h2;

        //op_code [7:0]
          `OC8051_ADD_D :  pc= pc_buf + 16'h2;
          `OC8051_ADD_C :  pc= pc_buf + 16'h2;
          `OC8051_ADDC_D : pc= pc_buf + 16'h2;
          `OC8051_ADDC_C : pc= pc_buf + 16'h2;
          `OC8051_ANL_D :  pc= pc_buf + 16'h2;
          `OC8051_ANL_C :  pc= pc_buf + 16'h2;
          `OC8051_ANL_DD : pc= pc_buf + 16'h2;
          `OC8051_ANL_DC : pc= pc_buf + 16'h3;
          `OC8051_ANL_B :  pc= pc_buf + 16'h2;
          `OC8051_ANL_NB : pc= pc_buf + 16'h2;
          `OC8051_CJNE_D : pc= pc_buf + 16'h3;
          `OC8051_CJNE_C : pc= pc_buf + 16'h3;
          `OC8051_CLR_B :  pc= pc_buf + 16'h2;
          `OC8051_CPL_B :  pc= pc_buf + 16'h2;
          `OC8051_DEC_D :  pc= pc_buf + 16'h2;
          `OC8051_DJNZ_D : pc= pc_buf + 16'h3;
          `OC8051_INC_D :  pc= pc_buf + 16'h2;
          `OC8051_JB :     pc= pc_buf + 16'h3;
          `OC8051_JBC :    pc= pc_buf + 16'h3;
          `OC8051_JC :     pc= pc_buf + 16'h2;
          `OC8051_JNB :    pc= pc_buf + 16'h3;
          `OC8051_JNC :    pc= pc_buf + 16'h2;
          `OC8051_JNZ :    pc= pc_buf + 16'h2;
          `OC8051_JZ :     pc= pc_buf + 16'h2;
          `OC8051_LCALL :  pc= pc_buf + 16'h3;
          `OC8051_LJMP :   pc= pc_buf + 16'h3;
          `OC8051_MOV_D :  pc= pc_buf + 16'h2;
          `OC8051_MOV_C :  pc= pc_buf + 16'h2;
          `OC8051_MOV_DA : pc= pc_buf + 16'h2;
          `OC8051_MOV_DD : pc= pc_buf + 16'h3;
          `OC8051_MOV_CD : pc= pc_buf + 16'h3;
          `OC8051_MOV_BC : pc= pc_buf + 16'h2;
          `OC8051_MOV_CB : pc= pc_buf + 16'h2;
          `OC8051_MOV_DP : pc= pc_buf + 16'h3;
          `OC8051_ORL_D :  pc= pc_buf + 16'h2;
          `OC8051_ORL_C :  pc= pc_buf + 16'h2;
          `OC8051_ORL_AD : pc= pc_buf + 16'h2;
          `OC8051_ORL_CD : pc= pc_buf + 16'h3;
          `OC8051_ORL_B :  pc= pc_buf + 16'h2;
          `OC8051_ORL_NB : pc= pc_buf + 16'h2;
          `OC8051_POP :    pc= pc_buf + 16'h2;
          `OC8051_PUSH :   pc= pc_buf + 16'h2;
          `OC8051_SETB_B : pc= pc_buf + 16'h2;
          `OC8051_SJMP :   pc= pc_buf + 16'h2;
          `OC8051_SUBB_D : pc= pc_buf + 16'h2;
          `OC8051_SUBB_C : pc= pc_buf + 16'h2;
          `OC8051_XCH_D :  pc= pc_buf + 16'h2;
          `OC8051_XRL_D :  pc= pc_buf + 16'h2;
          `OC8051_XRL_C :  pc= pc_buf + 16'h2;
          `OC8051_XRL_AD : pc= pc_buf + 16'h2;
          `OC8051_XRL_CD : pc= pc_buf + 16'h3;
          default:         pc= pc_buf + 16'h1;
        endcase
//
//in case of instructions that use more than one clock hold current pc
    end else begin
      pc= pc_buf;
   end
end


//
//interrupt buffer
always @(posedge clk or posedge rst)
  if (rst) begin
    int_buff1 <= #1 1'b0;
  end else begin
    int_buff1 <= #1 int_buff;
  end

always @(posedge clk or posedge rst)
  if (rst) begin
    int_buff <= #1 1'b0;
  end else if (intr) begin
    int_buff <= #1 1'b1;
  end else if (pc_wait)
    int_buff <= #1 1'b0;

wire [7:0]  pcs_source;
reg  [15:0] pcs_result;
reg         pcs_cy;

assign pcs_source = pc_wr_sel[0] ? op3_out : op2_out;

always @(pcs_source or pc or pcs_cy)
begin
  if (pcs_source[7]) begin
    {pcs_cy, pcs_result[7:0]} = {1'b0, pc[7:0]} + {1'b0, pcs_source};
    pcs_result[15:8] = pc[15:8] - {7'h0, !pcs_cy};
  end else pcs_result = pc + {8'h00, pcs_source};
end


always @(posedge clk or posedge rst)
begin
  if (rst) begin
    pc_buf <= #1 `OC8051_RST_PC;
  end else begin
    if (pc_wr) begin
//
//case of writing new value to pc (jupms)
      case (pc_wr_sel)
        `OC8051_PIS_ALU: pc_buf        <= #1 alu;
        `OC8051_PIS_AL:  pc_buf[7:0]   <= #1 alu[7:0];
        `OC8051_PIS_AH:  pc_buf[15:8]  <= #1 alu[7:0];
        `OC8051_PIS_I11: pc_buf[10:0]  <= #1 {op1_out[7:5], op2_out};
        `OC8051_PIS_I16: pc_buf        <= #1 {op2_out, op3_out};
        `OC8051_PIS_SO1: pc_buf        <= #1 pcs_result;
        `OC8051_PIS_SO2: pc_buf        <= #1 pcs_result;
      endcase
    end else
//
//or just remember current
      pc_buf <= #1 pc;
  end
end


always @(posedge clk or posedge rst)
  if (rst)
    ddat_ir <= #1 8'h00;
  else if (dack_i)
    ddat_ir <= #1 ddat_i;
*/

////////////////////////
always @(posedge clk or posedge rst)
  if (rst) begin
    rn_r      <= #1 5'd0;
    ri_r      <= #1 8'h00;
    imm_r     <= #1 8'h00;
    imm2_r    <= #1 8'h00;
    rd_addr_r <= #1 1'b0;
    op1_r     <= #1 8'h0;
    dack_ir   <= #1 1'b0;
    sp_r      <= #1 1'b0;
    pc_wr_r   <= #1 1'b0;
    pc_wr_r2  <= #1 1'b0;
  end else begin
    rn_r      <= #1 rn;
    ri_r      <= #1 ri;
    imm_r     <= #1 imm;
    imm2_r    <= #1 imm2;
    rd_addr_r <= #1 rd_addr[7];
    op1_r     <= #1 op1_out;
    dack_ir   <= #1 dack_i;
    sp_r      <= #1 sp;
    pc_wr_r   <= #1 pc_wr && (pc_wr_sel != `OC8051_PIS_AH);
    pc_wr_r2  <= #1 pc_wr_r;
  end

always @(posedge clk or posedge rst)
  if (rst) begin
    inc_pc_r  <= #1 1'b1;
  end else if (istb) begin
    inc_pc_r  <= #1 inc_pc;
  end

`ifdef OC8051_SIMULATION

initial
begin
  wait (!rst)
  if (ea_rom_sel) begin
    $display(" ");
    $display("\t Program running from internal rom !");
    $display(" ");
  end else begin
    $display(" ");
    $display("\t Program running from external rom !");
    $display(" ");
  end
end
`endif
  

endmodule
//////////////////////////////////////////////////////////////////////
//// 								  ////
//// multiply for 8051 Core 				  	  ////
//// 								  ////
//// This file is part of the 8051 cores project 		  ////
//// http://www.opencores.org/cores/8051/ 			  ////
//// 								  ////
//// Description 						  ////
//// Implementation of multipication used in alu.v 		  ////
//// 								  ////
//// To Do: 							  ////
////  Nothing							  ////
//// 								  ////
//// Author(s): 						  ////
//// - Simon Teran, simont@opencores.org 			  ////
//// - Marko Mlinar, markom@opencores.org 			  ////
//// 								  ////
//////////////////////////////////////////////////////////////////////
//// 								  ////
//// Copyright (C) 2001 Authors and OPENCORES.ORG 		  ////
//// 								  ////
//// This source file may be used and distributed without 	  ////
//// restriction provided that this copyright statement is not 	  ////
//// removed from the file and that any derivative work contains  ////
//// the original copyright notice and the associated disclaimer. ////
//// 								  ////
//// This source file is free software; you can redistribute it   ////
//// and/or modify it under the terms of the GNU Lesser General   ////
//// Public License as published by the Free Software Foundation; ////
//// either version 2.1 of the License, or (at your option) any   ////
//// later version. 						  ////
//// 								  ////
//// This source is distributed in the hope that it will be 	  ////
//// useful, but WITHOUT ANY WARRANTY; without even the implied   ////
//// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 	  ////
//// PURPOSE. See the GNU Lesser General Public License for more  ////
//// details. 							  ////
//// 								  ////
//// You should have received a copy of the GNU Lesser General 	  ////
//// Public License along with this source; if not, download it   ////
//// from http://www.opencores.org/lgpl.shtml 			  ////
//// 								  ////
//////////////////////////////////////////////////////////////////////
//
// CVS Revision History
//
// $Log: not supported by cvs2svn $
// Revision 1.8  2002/09/30 17:33:59  simont
// prepared header
//
//
// ver: 2 markom
// changed to two cycle multiplication, to save resources and
// increase speed
//
// ver: 3 markom
// changed to four cycle multiplication, to save resources and
// increase speed

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on


module oc8051_multiply (clk, rst, enable, src1, src2, des1, des2, desOv);
//
// this module is part of alu
// clk          (in)
// rst          (in)
// enable       (in)
// src1         (in)  first operand
// src2         (in)  second operand
// des1         (out) first result
// des2         (out) second result
// desOv        (out) Overflow output
//

input clk, rst, enable;
input [7:0] src1, src2;
output desOv;
output [7:0] des1, des2;

// wires
wire [15:0] mul_result1, mul_result, shifted;

// real registers
reg [1:0] cycle;
reg [15:0] tmp_mul;

assign mul_result1 = src1 * (cycle == 2'h0 ? src2[7:6] 
                           : cycle == 2'h1 ? src2[5:4]
                           : cycle == 2'h2 ? src2[3:2]
                           : src2[1:0]);

assign shifted = (cycle == 2'h0 ? 16'h0 : {tmp_mul[13:0], 2'b00});
assign mul_result = mul_result1 + shifted;
assign des1 = mul_result[15:8];
assign des2 = mul_result[7:0];
assign desOv = | des1;

always @(posedge clk or posedge rst)
begin
  if (rst) begin
    cycle <= #1 2'b0;
    tmp_mul <= #1 16'b0;
  end else begin
    if (enable) cycle <= #1 cycle + 2'b1;
    tmp_mul <= #1 mul_result;
  end
end

endmodule
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 port output                                            ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   8051 special function registers: port 0:3 - output         ////
////                                                              ////
////  To Do:                                                      ////
////   nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.9  2003/04/10 12:43:19  simont
// defines for pherypherals added
//
// Revision 1.8  2003/04/07 14:58:02  simont
// change sfr's interface.
//
// Revision 1.7  2003/01/13 14:14:41  simont
// replace some modules
//
// Revision 1.6  2002/09/30 17:33:59  simont
// prepared header
//
//


// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"


module oc8051_ports (clk, 
                    rst,
                    bit_in, 
		    data_in,
		    wr, 
		    wr_bit,
		    wr_addr, 

	`ifdef OC8051_PORT0
		    p0_out,
                    p0_in,
		    p0_data,
	`endif

	`ifdef OC8051_PORT1
		    p1_out,
		    p1_in,
		    p1_data,

	`endif

	`ifdef OC8051_PORT2
		    p2_out,
		    p2_in,
		    p2_data,
	`endif

	`ifdef OC8051_PORT3
		    p3_out,
		    p3_in,
		    p3_data,
	`endif

		    rmw);

input        clk,	//clock
             rst,	//reset
	     wr,	//write [oc8051_decoder.wr -r]
	     wr_bit,	//write bit addresable [oc8051_decoder.bit_addr -r]
	     bit_in,	//bit input [oc8051_alu.desCy]
	     rmw;	//read modify write feature [oc8051_decoder.rmw]
input [7:0]  wr_addr,	//write address [oc8051_ram_wr_sel.out]
             data_in; 	//data input (from alu destiantion 1) [oc8051_alu.des1]

`ifdef OC8051_PORT0
  input  [7:0] p0_in;
  output [7:0] p0_out,
               p0_data;
  reg    [7:0] p0_out;

  assign p0_data = rmw ? p0_out : p0_in;
`endif


`ifdef OC8051_PORT1
  input  [7:0] p1_in;
  output [7:0] p1_out,
               p1_data;
  reg    [7:0] p1_out;

  assign p1_data = rmw ? p1_out : p1_in;
`endif


`ifdef OC8051_PORT2
  input  [7:0] p2_in;
  output [7:0] p2_out,
	       p2_data;
  reg    [7:0] p2_out;

  assign p2_data = rmw ? p2_out : p2_in;
`endif


`ifdef OC8051_PORT3
  input  [7:0] p3_in;
  output [7:0] p3_out,
	       p3_data;
  reg    [7:0] p3_out;

  assign p3_data = rmw ? p3_out : p3_in;
`endif

//
// case of writing to port
always @(posedge clk or posedge rst)
begin
  if (rst) begin
`ifdef OC8051_PORT0
    p0_out <= #1 `OC8051_RST_P0;
`endif

`ifdef OC8051_PORT1
    p1_out <= #1 `OC8051_RST_P1;
`endif

`ifdef OC8051_PORT2
    p2_out <= #1 `OC8051_RST_P2;
`endif

`ifdef OC8051_PORT3
    p3_out <= #1 `OC8051_RST_P3;
`endif
  end else if (wr) begin
    if (!wr_bit) begin
      case (wr_addr) /* synopsys full_case parallel_case */
//
// bytaddresable
`ifdef OC8051_PORT0
        `OC8051_SFR_P0: p0_out <= #1 data_in;
`endif

`ifdef OC8051_PORT1
        `OC8051_SFR_P1: p1_out <= #1 data_in;
`endif

`ifdef OC8051_PORT2
        `OC8051_SFR_P2: p2_out <= #1 data_in;
`endif

`ifdef OC8051_PORT3
        `OC8051_SFR_P3: p3_out <= #1 data_in;
`endif
      endcase
    end else begin
      case (wr_addr[7:3]) /* synopsys full_case parallel_case */

//
// bit addressable
`ifdef OC8051_PORT0
        `OC8051_SFR_B_P0: p0_out[wr_addr[2:0]] <= #1 bit_in;
`endif

`ifdef OC8051_PORT1
        `OC8051_SFR_B_P1: p1_out[wr_addr[2:0]] <= #1 bit_in;
`endif

`ifdef OC8051_PORT2
        `OC8051_SFR_B_P2: p2_out[wr_addr[2:0]] <= #1 bit_in;
`endif

`ifdef OC8051_PORT3
        `OC8051_SFR_B_P3: p3_out[wr_addr[2:0]] <= #1 bit_in;
`endif
      endcase
    end
  end
end


endmodule

//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 program status word                                    ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   program status word                                        ////
////                                                              ////
////  To Do:                                                      ////
////   nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.11  2003/04/09 15:49:42  simont
// Register oc8051_sfr dato output, add signal wait_data.
//
// Revision 1.10  2003/04/07 14:58:02  simont
// change sfr's interface.
//
// Revision 1.9  2003/01/13 14:14:41  simont
// replace some modules
//
// Revision 1.8  2002/11/05 17:23:54  simont
// add module oc8051_sfr, 256 bytes internal ram
//
// Revision 1.7  2002/09/30 17:33:59  simont
// prepared header
//
//


// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"


module oc8051_psw (clk, rst, wr_addr, data_in, wr, wr_bit, data_out, p,
                cy_in, ac_in, ov_in, set, bank_sel);
//
// clk          (in)  clock
// rst          (in)  reset
// addr         (in)  write address [oc8051_ram_wr_sel.out]
// data_in      (in)  data input [oc8051_alu.des1]
// wr           (in)  write [oc8051_decoder.wr -r]
// wr_bit       (in)  write bit addresable [oc8051_decoder.bit_addr -r]
// p            (in)  parity [oc8051_acc.p]
// cy_in        (in)  input bit data [oc8051_alu.desCy]
// ac_in        (in)  auxiliary carry input [oc8051_alu.desAc]
// ov_in        (in)  overflov input [oc8051_alu.desOv]
// set          (in)  set psw (write to caryy, carry and overflov or carry, owerflov and ac) [oc8051_decoder.psw_set -r]
//


input clk, rst, wr, p, cy_in, ac_in, ov_in, wr_bit;
input [1:0] set;
input [7:0] wr_addr, data_in;

output [1:0] bank_sel;
output [7:0] data_out;

reg [7:1] data;
wire wr_psw;

assign wr_psw = (wr & (wr_addr==`OC8051_SFR_PSW) && !wr_bit);

assign bank_sel = wr_psw ? data_in[4:3]:data[4:3];
assign data_out = {data[7:1], p};

//
//case writing to psw
always @(posedge clk or posedge rst)
begin
  if (rst)
    data <= #1 `OC8051_RST_PSW;

//
// write to psw (byte addressable)
  else begin
    if (wr & (wr_bit==1'b0) & (wr_addr==`OC8051_SFR_PSW))
      data[7:1] <= #1 data_in[7:1];
//
// write to psw (bit addressable)
    else if (wr & wr_bit & (wr_addr[7:3]==`OC8051_SFR_B_PSW))
      data[wr_addr[2:0]] <= #1 cy_in;
    else begin
      case (set) /* synopsys full_case parallel_case */
        `OC8051_PS_CY: begin
//
//write carry
          data[7] <= #1 cy_in;
        end
        `OC8051_PS_OV: begin
//
//write carry and overflov
          data[7] <= #1 cy_in;
          data[2] <= #1 ov_in;
        end
        `OC8051_PS_AC:begin
//
//write carry, overflov and ac
          data[7] <= #1 cy_in;
          data[6] <= #1 ac_in;
          data[2] <= #1 ov_in;

        end
      endcase
    end
  end
end

endmodule

//////////////////////////////////////////////////////////////////////
////                                                              ////
////  Generic Dual-Port Synchronous RAM                           ////
////                                                              ////
////  This file is part of memory library available from          ////
////  http://www.opencores.org/cvsweb.shtml/generic_memories/     ////
////                                                              ////
////  Description                                                 ////
////  This block is a wrapper with common dual-port               ////
////  synchronous memory interface for different                  ////
////  types of ASIC and FPGA RAMs. Beside universal memory        ////
////  interface it also provides behavioral model of generic      ////
////  dual-port synchronous RAM.                                  ////
////  It also contains a fully synthesizeable model for FPGAs.    ////
////  It should be used in all OPENCORES designs that want to be  ////
////  portable accross different target technologies and          ////
////  independent of target memory.                               ////
////                                                              ////
////  Supported ASIC RAMs are:                                    ////
////  - Artisan Dual-Port Sync RAM                                ////
////  - Avant! Two-Port Sync RAM (*)                              ////
////  - Virage 2-port Sync RAM                                    ////
////                                                              ////
////  Supported FPGA RAMs are:                                    ////
////  - Generic FPGA (VENDOR_FPGA)                                ////
////    Tested RAMs: Altera, Xilinx                               ////
////    Synthesis tools: LeonardoSpectrum, Synplicity             ////
////  - Xilinx (VENDOR_XILINX)                                    ////
////  - Altera (VENDOR_ALTERA)                                    ////
////                                                              ////
////  To Do:                                                      ////
////   - fix Avant!                                               ////
////   - add additional RAMs (VS etc)                             ////
////                                                              ////
////  Author(s):                                                  ////
////      - Richard Herveille, richard@asics.ws                   ////
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
// Revision 1.3  2001/11/09 00:34:18  samg
// minor changes: unified with all common rams
//
// Revision 1.2  2001/11/08 19:11:31  samg
// added valid checks to behvioral model
//
// Revision 1.1.1.1  2001/09/14 09:57:10  rherveille
// Major cleanup.
// Files are now compliant to Altera & Xilinx memories.
// Memories are now compatible, i.e. drop-in replacements.
// Added synthesizeable generic FPGA description.
// Created "generic_memories" cvs entry.
//
// Revision 1.1.1.2  2001/08/21 13:09:27  damjan
// *** empty log message ***
//
// Revision 1.1  2001/08/20 18:23:20  damjan
// Initial revision
//
// Revision 1.1  2001/08/09 13:39:33  lampret
// Major clean-up.
//
// Revision 1.2  2001/07/30 05:38:02  lampret
// Adding empty directories required by HDL coding guidelines
//
//

//`include "timescale.v"

//`define VENDOR_FPGA
//`define VENDOR_XILINX
//`define VENDOR_ALTERA

module generic_dpram(
	// Generic synchronous dual-port RAM interface
	rclk, rrst, rce, oe, raddr, do,
	wclk, wrst, wce, we, waddr, di
);

	//
	// Default address and data buses width
	//
	parameter aw = 5;  // number of bits in address-bus
	parameter dw = 16; // number of bits in data-bus

	//
	// Generic synchronous double-port RAM interface
	//
	// read port
	input           rclk;  // read clock, rising edge trigger
	input           rrst;  // read port reset, active high
	input           rce;   // read port chip enable, active high
	input           oe;	   // output enable, active high
	input  [aw-1:0] raddr; // read address
	output [dw-1:0] do;    // data output

	// write port
	input          wclk;  // write clock, rising edge trigger
	input          wrst;  // write port reset, active high
	input          wce;   // write port chip enable, active high
	input          we;    // write enable, active high
	input [aw-1:0] waddr; // write address
	input [dw-1:0] di;    // data input

	//
	// Module body
	//

`ifdef VENDOR_FPGA
	//
	// Instantiation synthesizeable FPGA memory
	//
	// This code has been tested using LeonardoSpectrum and Synplicity.
	// The code correctly instantiates Altera EABs and Xilinx BlockRAMs.
	//
	reg [dw-1:0] mem [(1<<aw) -1:0]; // instantiate memory
	reg [aw-1:0] ra;                 // register read address

	// read operation
	always @(posedge rclk)
	  if (rce)
	    ra <= #1 raddr;

	assign do = mem[ra];

	// write operation
	always@(posedge wclk)
		if (we && wce)
			mem[waddr] <= #1 di;

`else

`ifdef VENDOR_XILINX
	//
	// Instantiation of FPGA memory:
	//
	// Virtex/Spartan2 BlockRAMs
	//
	xilinx_ram_dp xilinx_ram(
		// read port
		.CLKA(rclk),
		.RSTA(rrst),
		.ENA(rce),
		.ADDRA(raddr),
		.DIA( {dw{1'b0}} ),
		.WEA(1'b0),
		.DOA(do),

		// write port
		.CLKB(wclk),
		.RSTB(wrst),
		.ENB(wce),
		.ADDRB(waddr),
		.DIB(di),
		.WEB(we),
		.DOB()
	);

	defparam
		xilinx_ram.dwidth = dw,
		xilinx_ram.awidth = aw;

`else

`ifdef VENDOR_ALTERA
	//
	// Instantiation of FPGA memory:
	//
	// Altera FLEX/APEX EABs
	//
	altera_ram_dp altera_ram(
		// read port
		.rdclock(rclk),
		.rdclocken(rce),
		.rdaddress(raddr),
		.q(do),

		// write port
		.wrclock(wclk),
		.wrclocken(wce),
		.wren(we),
		.wraddress(waddr),
		.data(di)
	);

	defparam
		altera_ram.dwidth = dw,
		altera_ram.awidth = aw;

`else

`ifdef VENDOR_ARTISAN

	//
	// Instantiation of ASIC memory:
	//
	// Artisan Synchronous Double-Port RAM (ra2sh)
	//
	art_hsdp #(dw, 1<<aw, aw) artisan_sdp(
		// read port
		.qa(do),
		.clka(rclk),
		.cena(~rce),
		.wena(1'b1),
		.aa(raddr),
		.da( {dw{1'b0}} ),
		.oena(~oe),

		// write port
		.qb(),
		.clkb(wclk),
		.cenb(~wce),
		.wenb(~we),
		.ab(waddr),
		.db(di),
		.oenb(1'b1)
	);

`else

`ifdef VENDOR_AVANT

	//
	// Instantiation of ASIC memory:
	//
	// Avant! Asynchronous Two-Port RAM
	//
	avant_atp avant_atp(
		.web(~we),
		.reb(),
		.oeb(~oe),
		.rcsb(),
		.wcsb(),
		.ra(raddr),
		.wa(waddr),
		.di(di),
		.do(do)
	);

`else

`ifdef VENDOR_VIRAGE

	//
	// Instantiation of ASIC memory:
	//
	// Virage Synchronous 2-port R/W RAM
	//
	virage_stp virage_stp(
		// read port
		.CLKA(rclk),
		.MEA(rce_a),
		.ADRA(raddr),
		.DA( {dw{1'b0}} ),
		.WEA(1'b0),
		.OEA(oe),
		.QA(do),

		// write port
		.CLKB(wclk),
		.MEB(wce),
		.ADRB(waddr),
		.DB(di),
		.WEB(we),
		.OEB(1'b1),
		.QB()
	);

`else

	//
	// Generic dual-port synchronous RAM model
	//

	//
	// Generic RAM's registers and wires
	//
	reg	[dw-1:0]	mem [(1<<aw)-1:0]; // RAM content
	reg	[dw-1:0]	do_reg;            // RAM data output register

	//
	// Data output drivers
	//
	assign do = (oe & rce) ? do_reg : {dw{1'bz}};

	// read operation
	always @(posedge rclk)
		if (rce)
          		do_reg <= #1 (we && (waddr==raddr)) ? {dw{1'b x}} : mem[raddr];

	// write operation
	always @(posedge wclk)
		if (wce && we)
			mem[waddr] <= #1 di;


	// Task prints range of memory
	// *** Remember that tasks are non reentrant, don't call this task in parallel for multiple instantiations.
	task print_ram;
	input [aw-1:0] start;
	input [aw-1:0] finish;
	integer rnum;
  	begin
    		for (rnum=start;rnum<=finish;rnum=rnum+1)
      			$display("Addr %h = %h",rnum,mem[rnum]);
  	end
	endtask

`endif // !VENDOR_VIRAGE
`endif // !VENDOR_AVANT
`endif // !VENDOR_ARTISAN
`endif // !VENDOR_ALTERA
`endif // !VENDOR_XILINX
`endif // !VENDOR_FPGA

endmodule

//
// Black-box modules
//

`ifdef VENDOR_ALTERA
	module altera_ram_dp(
		data,
		wraddress,
		rdaddress,
		wren,
		wrclock,
		wrclocken,
		rdclock,
		rdclocken,
		q) /* synthesis black_box */;

		parameter awidth = 7;
		parameter dwidth = 8;

		input [dwidth -1:0] data;
		input [awidth -1:0] wraddress;
		input [awidth -1:0] rdaddress;
		input               wren;
		input               wrclock;
		input               wrclocken;
		input               rdclock;
		input               rdclocken;
		output [dwidth -1:0] q;

		// synopsis translate_off
		// exemplar translate_off

		syn_dpram_rowr #(
			"UNUSED",
			dwidth,
			awidth,
			1 << awidth
		)
		altera_dpram_model (
			// read port
			.RdClock(rdclock),
			.RdClken(rdclocken),
			.RdAddress(rdaddress),
			.RdEn(1'b1),
			.Q(q),

			// write port
			.WrClock(wrclock),
			.WrClken(wrclocken),
			.WrAddress(wraddress),
			.WrEn(wren),
			.Data(data)
		);

		// exemplar translate_on
		// synopsis translate_on

	endmodule
`endif // VENDOR_ALTERA

`ifdef VENDOR_XILINX
	module xilinx_ram_dp (
		ADDRA,
		CLKA,
		ADDRB,
		CLKB,
		DIA,
		WEA,
		DIB,
		WEB,
		ENA,
		ENB,
		RSTA,
		RSTB,
		DOA,
		DOB) /* synthesis black_box */ ;

	parameter awidth = 7;
	parameter dwidth = 8;

	// port_a
	input               CLKA;
	input               RSTA;
	input               ENA;
	input [awidth-1:0]  ADDRA;
	input [dwidth-1:0]  DIA;
	input               WEA;
	output [dwidth-1:0] DOA;

	// port_b
	input               CLKB;
	input               RSTB;
	input               ENB;
	input [awidth-1:0]  ADDRB;
	input [dwidth-1:0]  DIB;
	input               WEB;
	output [dwidth-1:0] DOB;

	// insert simulation model


	// synopsys translate_off
	// exemplar translate_off

	C_MEM_DP_BLOCK_V1_0 #(
		awidth,
		awidth,
		1,
		1,
		"0",
		1 << awidth,
		1 << awidth,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		"",
		16,
		0,
		0,
		1,
		1,
		1,
		1,
		dwidth,
		dwidth)
	xilinx_dpram_model (
		.ADDRA(ADDRA),
		.CLKA(CLKA),
		.ADDRB(ADDRB),
		.CLKB(CLKB),
		.DIA(DIA),
		.WEA(WEA),
		.DIB(DIB),
		.WEB(WEB),
		.ENA(ENA),
		.ENB(ENB),
		.RSTA(RSTA),
		.RSTB(RSTB),
		.DOA(DOA),
		.DOB(DOB));

		// exemplar translate_on
		// synopsys translate_on

	endmodule
`endif // VENDOR_XILINX
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 internal ram                                           ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   256 bytes two port ram                                     ////
////                                                              ////
////  To Do:                                                      ////
////   nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"

//
// two port ram
//
module oc8051_ram_256x8_two_bist (
                     clk,
                     rst,
		     rd_addr,
		     rd_data,
		     rd_en,
		     wr_addr,
		     wr_data,
		     wr_en,
		     wr
`ifdef OC8051_BIST
	 ,
         scanb_rst,
         scanb_clk,
         scanb_si,
         scanb_so,
         scanb_en
`endif
		     );


input         clk, 
              wr, 
	      rst,
	      rd_en,
	      wr_en;
input  [7:0]  wr_data;
input  [7:0]  rd_addr,
              wr_addr;
output [7:0]  rd_data;

`ifdef OC8051_BIST
input   scanb_rst;
input   scanb_clk;
input   scanb_si;
output  scanb_so;
input   scanb_en;
`endif


`ifdef OC8051_RAM_XILINX
  xilinx_ram_dp xilinx_ram(
  	// read port
  	.CLKA(clk),
  	.RSTA(rst),
  	.ENA(rd_en),
  	.ADDRA(rd_addr),
  	.DIA(8'h00),
  	.WEA(1'b0),
  	.DOA(rd_data),
  
  	// write port
  	.CLKB(clk),
  	.RSTB(rst),
  	.ENB(wr_en),
  	.ADDRB(wr_addr),
  	.DIB(wr_data),
  	.WEB(wr),
  	.DOB()
  );
  
  defparam
  	xilinx_ram.dwidth = 8,
  	xilinx_ram.awidth = 8;

`else

  `ifdef OC8051_RAM_VIRTUALSILICON

  `else

    `ifdef OC8051_RAM_GENERIC
    
      generic_dpram #(8, 8) oc8051_ram1(
      	.rclk  ( clk            ),
      	.rrst  ( rst            ),
      	.rce   ( rd_en          ),
      	.oe    ( 1'b1           ),
      	.raddr ( rd_addr        ),
      	.do    ( rd_data        ),
      
      	.wclk  ( clk            ),
      	.wrst  ( rst            ),
      	.wce   ( wr_en          ),
      	.we    ( wr             ),
      	.waddr ( wr_addr        ),
      	.di    ( wr_data        )
      );
    
    `else

      reg    [7:0]  rd_data;
      //
      // buffer
      reg    [7:0]  buff [0:256];
      
      
      //
      // writing to ram
      always @(posedge clk)
      begin
       if (wr)
          buff[wr_addr] <= #1 wr_data;
      end
      
      //
      // reading from ram
      always @(posedge clk or posedge rst)
      begin
        if (rst)
          rd_data <= #1 8'h0;
        else if ((wr_addr==rd_addr) & wr & rd_en)
          rd_data <= #1 wr_data;
        else if (rd_en)
          rd_data <= #1 buff[rd_addr];
      end
    `endif  //OC8051_RAM_GENERIC
  `endif    //OC8051_RAM_VIRTUALSILICON  
`endif      //OC8051_RAM_XILINX

endmodule
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 cache ram                                              ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   64x31 dual port ram                                        ////
////                                                              ////
////  To Do:                                                      ////
////   nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"

//
// duble port ram
//
module oc8051_ram_64x32_dual_bist (
                     clk,
                     rst,
                     
		     adr0,
		     dat0_o,
		     en0,
		     
		     adr1,
		     dat1_i,
		     dat1_o,
		     en1,
		     wr1
`ifdef OC8051_BIST
	 ,
         scanb_rst,
         scanb_clk,
         scanb_si,
         scanb_so,
         scanb_en
`endif
		     );

parameter ADR_WIDTH = 6;

input         clk, 
              wr1, 
	      rst,
	      en0,
	      en1;
input  [7:0]  dat1_i;
input  [ADR_WIDTH-1:0]  adr0,
                        adr1;
output [7:0]  dat0_o,
              dat1_o;

reg    [7:0]  rd_data;


`ifdef OC8051_BIST
input   scanb_rst;
input   scanb_clk;
input   scanb_si;
output  scanb_so;
input   scanb_en;
`endif


`ifdef OC8051_RAM_XILINX
  xilinx_ram_dp xilinx_ram(
  	// read port
  	.CLKA(clk),
  	.RSTA(rst),
  	.ENA(en0),
  	.ADDRA(adr0),
  	.DIA(32'h00),
  	.WEA(1'b0),
  	.DOA(dat0_o),
  
  	// write port
  	.CLKB(clk),
  	.RSTB(rst),
  	.ENB(en1),
  	.ADDRB(adr1),
  	.DIB(dat1_i),
  	.WEB(wr1),
  	.DOB(dat1_o)
  );
  
  defparam
  	xilinx_ram.dwidth = 32,
  	xilinx_ram.awidth = ADR_WIDTH;

`else

  `ifdef OC8051_RAM_VIRTUALSILICON

  `else

    `ifdef OC8051_RAM_GENERIC
    
      generic_dpram #(ADR_WIDTH, 32) oc8051_ram1(
      	.rclk  ( clk            ),
      	.rrst  ( rst            ),
      	.rce   ( en0            ),
      	.oe    ( 1'b1           ),
      	.raddr ( adr0           ),
      	.do    ( dat0_o         ),
      
      	.wclk  ( clk            ),
      	.wrst  ( rst            ),
      	.wce   ( en1            ),
      	.we    ( wr1            ),
      	.waddr ( adr1           ),
      	.di    ( dat1_i         )
      );
    
    `else

      reg [31:0] dat1_o, 
                 dat0_o;  
      //
      // buffer
      reg    [31:0]  buff [0:(1<<ADR_WIDTH) -1];

      always @(posedge clk or posedge rst)
      begin
        if (rst)
          dat1_o     <= #1 32'h0;
        else if (wr1) begin
          buff[adr1] <= #1 dat1_i;
          dat1_o    <= #1 dat1_i;
        end else
          dat1_o <= #1 buff[adr1];
      end
      
      always @(posedge clk or posedge rst)
      begin
        if (rst)
          dat0_o <= #1 32'h0;
        else if ((adr0==adr1) & wr1)
          dat0_o <= #1 dat1_i;
        else
          dat0_o <= #1 buff[adr0];
      end
            
    `endif  //OC8051_RAM_GENERIC
  `endif    //OC8051_RAM_VIRTUALSILICON  
`endif      //OC8051_RAM_XILINX

endmodule
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 data ram                                               ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   data ram                                                   ////
////                                                              ////
////  To Do:                                                      ////
////   nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.10  2003/06/20 13:36:37  simont
// ram modules added.
//
// Revision 1.9  2003/06/17 14:17:22  simont
// BIST signals added.
//
// Revision 1.8  2003/04/02 16:12:04  simont
// generic_dpram used
//
// Revision 1.7  2003/04/02 11:26:21  simont
// updating...
//
// Revision 1.6  2003/01/26 14:19:22  rherveille
// Replaced oc8051_ram by generic_dpram.
//
// Revision 1.5  2003/01/13 14:14:41  simont
// replace some modules
//
// Revision 1.4  2002/09/30 17:33:59  simont
// prepared header
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"


module oc8051_ram_top (clk, 
                       rst, 
		       rd_addr, 
		       rd_data, 
		       wr_addr, 
		       bit_addr, 
		       wr_data, 
		       wr, 
		       bit_data_in, 
		       bit_data_out
`ifdef OC8051_BIST
         ,
         scanb_rst,
         scanb_clk,
         scanb_si,
         scanb_so,
         scanb_en
`endif
		       );

// on-chip ram-size (2**ram_aw bytes)
parameter ram_aw = 8; // default 256 bytes


//
// clk          (in)  clock
// rd_addr      (in)  read addres [oc8051_ram_rd_sel.out]
// rd_data      (out) read data [oc8051_ram_sel.in_ram]
// wr_addr      (in)  write addres [oc8051_ram_wr_sel.out]
// bit_addr     (in)  bit addresable instruction [oc8051_decoder.bit_addr -r]
// wr_data      (in)  write data [oc8051_alu.des1]
// wr           (in)  write [oc8051_decoder.wr -r]
// bit_data_in  (in)  bit data input [oc8051_alu.desCy]
// bit_data_out (out)  bit data output [oc8051_ram_sel.bit_in]
//

input clk, wr, bit_addr, bit_data_in, rst;
input [7:0] wr_data;
input [7:0] rd_addr, wr_addr;
output bit_data_out;
output [7:0] rd_data;

`ifdef OC8051_BIST
input   scanb_rst;
input   scanb_clk;
input   scanb_si;
output  scanb_so;
input   scanb_en;
`endif

// rd_addr_m    read address modified
// wr_addr_m    write address modified
// wr_data_m    write data modified
reg [7:0] wr_data_m;
reg [7:0] rd_addr_m, wr_addr_m;


wire       rd_en;
reg        bit_addr_r,
           rd_en_r;
reg  [7:0] wr_data_r;
wire [7:0] rd_data_m;
reg  [2:0] bit_select;

assign bit_data_out = rd_data[bit_select];


assign rd_data = rd_en_r ? wr_data_r: rd_data_m;
assign rd_en   = (rd_addr_m == wr_addr_m) & wr;

oc8051_ram_256x8_two_bist oc8051_idata(
                           .clk     ( clk        ),
                           .rst     ( rst        ),
			   .rd_addr ( rd_addr_m  ),
			   .rd_data ( rd_data_m  ),
			   .rd_en   ( !rd_en     ),
			   .wr_addr ( wr_addr_m  ),
			   .wr_data ( wr_data_m  ),
			   .wr_en   ( 1'b1       ),
			   .wr      ( wr         )
`ifdef OC8051_BIST
         ,
         .scanb_rst(scanb_rst),
         .scanb_clk(scanb_clk),
         .scanb_si(scanb_si),
         .scanb_so(scanb_so),
         .scanb_en(scanb_en)
`endif
			   );

always @(posedge clk or posedge rst)
  if (rst) begin
    bit_addr_r <= #1 1'b0;
    bit_select <= #1 3'b0;
  end else begin
    bit_addr_r <= #1 bit_addr;
    bit_select <= #1 rd_addr[2:0];
  end


always @(posedge clk or posedge rst)
  if (rst) begin
    rd_en_r    <= #1 1'b0;
    wr_data_r  <= #1 8'h0;
  end else begin
    rd_en_r    <= #1 rd_en;
    wr_data_r  <= #1 wr_data_m;
  end


always @(rd_addr or bit_addr)
  casex ( {bit_addr, rd_addr[7]} ) // synopsys full_case parallel_case
      2'b0?: rd_addr_m = rd_addr;
      2'b10: rd_addr_m = {4'b0010, rd_addr[6:3]};
      2'b11: rd_addr_m = {1'b1, rd_addr[6:3], 3'b000};
  endcase


always @(wr_addr or bit_addr_r)
  casex ( {bit_addr_r, wr_addr[7]} ) // synopsys full_case parallel_case
      2'b0?: wr_addr_m = wr_addr;
      2'b10: wr_addr_m = {8'h00, 4'b0010, wr_addr[6:3]};
      2'b11: wr_addr_m = {8'h00, 1'b1, wr_addr[6:3], 3'b000};
  endcase


always @(rd_data or bit_select or bit_data_in or wr_data or bit_addr_r)
  casex ( {bit_addr_r, bit_select} ) // synopsys full_case parallel_case
      4'b0_???: wr_data_m = wr_data;
      4'b1_000: wr_data_m = {rd_data[7:1], bit_data_in};
      4'b1_001: wr_data_m = {rd_data[7:2], bit_data_in, rd_data[0]};
      4'b1_010: wr_data_m = {rd_data[7:3], bit_data_in, rd_data[1:0]};
      4'b1_011: wr_data_m = {rd_data[7:4], bit_data_in, rd_data[2:0]};
      4'b1_100: wr_data_m = {rd_data[7:5], bit_data_in, rd_data[3:0]};
      4'b1_101: wr_data_m = {rd_data[7:6], bit_data_in, rd_data[4:0]};
      4'b1_110: wr_data_m = {rd_data[7], bit_data_in, rd_data[5:0]};
      4'b1_111: wr_data_m = {bit_data_in, rd_data[6:0]};
  endcase


endmodule
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 internal program rom                                   ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   internal program rom for 8051 core                         ////
////                                                              ////
////  To Do:                                                      ////
////   Nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.3  2003/06/03 17:09:57  simont
// pipelined acces to axternal instruction interface added.
//
// Revision 1.2  2003/04/03 19:17:19  simont
// add // `include "oc8051_defines.v"
//
// Revision 1.1  2003/04/02 11:16:22  simont
// initial inport
//
// Revision 1.4  2002/10/23 17:00:18  simont
// signal es_int=1'b0
//
// Revision 1.3  2002/09/30 17:34:01  simont
// prepared header
//
//
// `include "oc8051_defines.v"

module oc8051_rom (rst, clk, addr, ea_int, data_o);

//parameter INT_ROM_WID= 15;

input rst, clk;
input [15:0] addr;
//input [22:0] addr;
output ea_int;
output [31:0] data_o;

reg [31:0] data_o;
wire ea;

reg ea_int;


`ifdef OC8051_XILINX_ROM

parameter INT_ROM_WID= 12;

reg [4:0] addr01;


wire [15:0] addr_rst;
wire [7:0] int_data0, int_data1, int_data2, int_data3, int_data4, int_data5, int_data6, int_data7, int_data8, int_data9, int_data10, int_data11, int_data12, int_data13, int_data14, int_data15, int_data16, int_data17, int_data18, int_data19, int_data20, int_data21, int_data22, int_data23, int_data24, int_data25, int_data26, int_data27, int_data28, int_data29, int_data30, int_data31, int_data32, int_data33, int_data34, int_data35, int_data36, int_data37, int_data38, int_data39, int_data40, int_data41, int_data42, int_data43, int_data44, int_data45, int_data46, int_data47, int_data48, int_data49, int_data50, int_data51, int_data52, int_data53, int_data54, int_data55, int_data56, int_data57, int_data58, int_data59, int_data60, int_data61, int_data62, int_data63, int_data64, int_data65, int_data66, int_data67, int_data68, int_data69, int_data70, int_data71, int_data72, int_data73, int_data74, int_data75, int_data76, int_data77, int_data78, int_data79, int_data80, int_data81, int_data82, int_data83, int_data84, int_data85, int_data86, int_data87, int_data88, int_data89, int_data90, int_data91, int_data92, int_data93, int_data94, int_data95, int_data96, int_data97, int_data98, int_data99, int_data100, int_data101, int_data102, int_data103, int_data104, int_data105, int_data106, int_data107, int_data108, int_data109, int_data110, int_data111, int_data112, int_data113, int_data114, int_data115, int_data116, int_data117, int_data118, int_data119, int_data120, int_data121, int_data122, int_data123, int_data124, int_data125, int_data126, int_data127;

assign ea = | addr[15:INT_ROM_WID];

assign addr_rst = rst ? 16'h0000 : addr;

  rom0 rom_0 (.a(addr01), .o(int_data0));
  rom1 rom_1 (.a(addr01), .o(int_data1));
  rom2 rom_2 (.a(addr_rst[11:7]), .o(int_data2));
  rom3 rom_3 (.a(addr_rst[11:7]), .o(int_data3));
  rom4 rom_4 (.a(addr_rst[11:7]), .o(int_data4));
  rom5 rom_5 (.a(addr_rst[11:7]), .o(int_data5));
  rom6 rom_6 (.a(addr_rst[11:7]), .o(int_data6));
  rom7 rom_7 (.a(addr_rst[11:7]), .o(int_data7));
  rom8 rom_8 (.a(addr_rst[11:7]), .o(int_data8));
  rom9 rom_9 (.a(addr_rst[11:7]), .o(int_data9));
  rom10 rom_10 (.a(addr_rst[11:7]), .o(int_data10));
  rom11 rom_11 (.a(addr_rst[11:7]), .o(int_data11));
  rom12 rom_12 (.a(addr_rst[11:7]), .o(int_data12));
  rom13 rom_13 (.a(addr_rst[11:7]), .o(int_data13));
  rom14 rom_14 (.a(addr_rst[11:7]), .o(int_data14));
  rom15 rom_15 (.a(addr_rst[11:7]), .o(int_data15));
  rom16 rom_16 (.a(addr_rst[11:7]), .o(int_data16));
  rom17 rom_17 (.a(addr_rst[11:7]), .o(int_data17));
  rom18 rom_18 (.a(addr_rst[11:7]), .o(int_data18));
  rom19 rom_19 (.a(addr_rst[11:7]), .o(int_data19));
  rom20 rom_20 (.a(addr_rst[11:7]), .o(int_data20));
  rom21 rom_21 (.a(addr_rst[11:7]), .o(int_data21));
  rom22 rom_22 (.a(addr_rst[11:7]), .o(int_data22));
  rom23 rom_23 (.a(addr_rst[11:7]), .o(int_data23));
  rom24 rom_24 (.a(addr_rst[11:7]), .o(int_data24));
  rom25 rom_25 (.a(addr_rst[11:7]), .o(int_data25));
  rom26 rom_26 (.a(addr_rst[11:7]), .o(int_data26));
  rom27 rom_27 (.a(addr_rst[11:7]), .o(int_data27));
  rom28 rom_28 (.a(addr_rst[11:7]), .o(int_data28));
  rom29 rom_29 (.a(addr_rst[11:7]), .o(int_data29));
  rom30 rom_30 (.a(addr_rst[11:7]), .o(int_data30));
  rom31 rom_31 (.a(addr_rst[11:7]), .o(int_data31));
  rom32 rom_32 (.a(addr_rst[11:7]), .o(int_data32));
  rom33 rom_33 (.a(addr_rst[11:7]), .o(int_data33));
  rom34 rom_34 (.a(addr_rst[11:7]), .o(int_data34));
  rom35 rom_35 (.a(addr_rst[11:7]), .o(int_data35));
  rom36 rom_36 (.a(addr_rst[11:7]), .o(int_data36));
  rom37 rom_37 (.a(addr_rst[11:7]), .o(int_data37));
  rom38 rom_38 (.a(addr_rst[11:7]), .o(int_data38));
  rom39 rom_39 (.a(addr_rst[11:7]), .o(int_data39));
  rom40 rom_40 (.a(addr_rst[11:7]), .o(int_data40));
  rom41 rom_41 (.a(addr_rst[11:7]), .o(int_data41));
  rom42 rom_42 (.a(addr_rst[11:7]), .o(int_data42));
  rom43 rom_43 (.a(addr_rst[11:7]), .o(int_data43));
  rom44 rom_44 (.a(addr_rst[11:7]), .o(int_data44));
  rom45 rom_45 (.a(addr_rst[11:7]), .o(int_data45));
  rom46 rom_46 (.a(addr_rst[11:7]), .o(int_data46));
  rom47 rom_47 (.a(addr_rst[11:7]), .o(int_data47));
  rom48 rom_48 (.a(addr_rst[11:7]), .o(int_data48));
  rom49 rom_49 (.a(addr_rst[11:7]), .o(int_data49));
  rom50 rom_50 (.a(addr_rst[11:7]), .o(int_data50));
  rom51 rom_51 (.a(addr_rst[11:7]), .o(int_data51));
  rom52 rom_52 (.a(addr_rst[11:7]), .o(int_data52));
  rom53 rom_53 (.a(addr_rst[11:7]), .o(int_data53));
  rom54 rom_54 (.a(addr_rst[11:7]), .o(int_data54));
  rom55 rom_55 (.a(addr_rst[11:7]), .o(int_data55));
  rom56 rom_56 (.a(addr_rst[11:7]), .o(int_data56));
  rom57 rom_57 (.a(addr_rst[11:7]), .o(int_data57));
  rom58 rom_58 (.a(addr_rst[11:7]), .o(int_data58));
  rom59 rom_59 (.a(addr_rst[11:7]), .o(int_data59));
  rom60 rom_60 (.a(addr_rst[11:7]), .o(int_data60));
  rom61 rom_61 (.a(addr_rst[11:7]), .o(int_data61));
  rom62 rom_62 (.a(addr_rst[11:7]), .o(int_data62));
  rom63 rom_63 (.a(addr_rst[11:7]), .o(int_data63));
  rom64 rom_64 (.a(addr_rst[11:7]), .o(int_data64));
  rom65 rom_65 (.a(addr_rst[11:7]), .o(int_data65));
  rom66 rom_66 (.a(addr_rst[11:7]), .o(int_data66));
  rom67 rom_67 (.a(addr_rst[11:7]), .o(int_data67));
  rom68 rom_68 (.a(addr_rst[11:7]), .o(int_data68));
  rom69 rom_69 (.a(addr_rst[11:7]), .o(int_data69));
  rom70 rom_70 (.a(addr_rst[11:7]), .o(int_data70));
  rom71 rom_71 (.a(addr_rst[11:7]), .o(int_data71));
  rom72 rom_72 (.a(addr_rst[11:7]), .o(int_data72));
  rom73 rom_73 (.a(addr_rst[11:7]), .o(int_data73));
  rom74 rom_74 (.a(addr_rst[11:7]), .o(int_data74));
  rom75 rom_75 (.a(addr_rst[11:7]), .o(int_data75));
  rom76 rom_76 (.a(addr_rst[11:7]), .o(int_data76));
  rom77 rom_77 (.a(addr_rst[11:7]), .o(int_data77));
  rom78 rom_78 (.a(addr_rst[11:7]), .o(int_data78));
  rom79 rom_79 (.a(addr_rst[11:7]), .o(int_data79));
  rom80 rom_80 (.a(addr_rst[11:7]), .o(int_data80));
  rom81 rom_81 (.a(addr_rst[11:7]), .o(int_data81));
  rom82 rom_82 (.a(addr_rst[11:7]), .o(int_data82));
  rom83 rom_83 (.a(addr_rst[11:7]), .o(int_data83));
  rom84 rom_84 (.a(addr_rst[11:7]), .o(int_data84));
  rom85 rom_85 (.a(addr_rst[11:7]), .o(int_data85));
  rom86 rom_86 (.a(addr_rst[11:7]), .o(int_data86));
  rom87 rom_87 (.a(addr_rst[11:7]), .o(int_data87));
  rom88 rom_88 (.a(addr_rst[11:7]), .o(int_data88));
  rom89 rom_89 (.a(addr_rst[11:7]), .o(int_data89));
  rom90 rom_90 (.a(addr_rst[11:7]), .o(int_data90));
  rom91 rom_91 (.a(addr_rst[11:7]), .o(int_data91));
  rom92 rom_92 (.a(addr_rst[11:7]), .o(int_data92));
  rom93 rom_93 (.a(addr_rst[11:7]), .o(int_data93));
  rom94 rom_94 (.a(addr_rst[11:7]), .o(int_data94));
  rom95 rom_95 (.a(addr_rst[11:7]), .o(int_data95));
  rom96 rom_96 (.a(addr_rst[11:7]), .o(int_data96));
  rom97 rom_97 (.a(addr_rst[11:7]), .o(int_data97));
  rom98 rom_98 (.a(addr_rst[11:7]), .o(int_data98));
  rom99 rom_99 (.a(addr_rst[11:7]), .o(int_data99));
  rom100 rom_100 (.a(addr_rst[11:7]), .o(int_data100));
  rom101 rom_101 (.a(addr_rst[11:7]), .o(int_data101));
  rom102 rom_102 (.a(addr_rst[11:7]), .o(int_data102));
  rom103 rom_103 (.a(addr_rst[11:7]), .o(int_data103));
  rom104 rom_104 (.a(addr_rst[11:7]), .o(int_data104));
  rom105 rom_105 (.a(addr_rst[11:7]), .o(int_data105));
  rom106 rom_106 (.a(addr_rst[11:7]), .o(int_data106));
  rom107 rom_107 (.a(addr_rst[11:7]), .o(int_data107));
  rom108 rom_108 (.a(addr_rst[11:7]), .o(int_data108));
  rom109 rom_109 (.a(addr_rst[11:7]), .o(int_data109));
  rom110 rom_110 (.a(addr_rst[11:7]), .o(int_data110));
  rom111 rom_111 (.a(addr_rst[11:7]), .o(int_data111));
  rom112 rom_112 (.a(addr_rst[11:7]), .o(int_data112));
  rom113 rom_113 (.a(addr_rst[11:7]), .o(int_data113));
  rom114 rom_114 (.a(addr_rst[11:7]), .o(int_data114));
  rom115 rom_115 (.a(addr_rst[11:7]), .o(int_data115));
  rom116 rom_116 (.a(addr_rst[11:7]), .o(int_data116));
  rom117 rom_117 (.a(addr_rst[11:7]), .o(int_data117));
  rom118 rom_118 (.a(addr_rst[11:7]), .o(int_data118));
  rom119 rom_119 (.a(addr_rst[11:7]), .o(int_data119));
  rom120 rom_120 (.a(addr_rst[11:7]), .o(int_data120));
  rom121 rom_121 (.a(addr_rst[11:7]), .o(int_data121));
  rom122 rom_122 (.a(addr_rst[11:7]), .o(int_data122));
  rom123 rom_123 (.a(addr_rst[11:7]), .o(int_data123));
  rom124 rom_124 (.a(addr_rst[11:7]), .o(int_data124));
  rom125 rom_125 (.a(addr_rst[11:7]), .o(int_data125));
  rom126 rom_126 (.a(addr_rst[11:7]), .o(int_data126));
  rom127 rom_127 (.a(addr_rst[11:7]), .o(int_data127));

always @(addr_rst)
begin
  if (addr_rst[1])
    addr01= addr_rst[11:7]+ 5'h1;
  else
    addr01= addr_rst[11:7];
end

//
// always read tree bits in row
always @(posedge clk)
begin
  case(addr[6:0]) /* synopsys parallel_case */
    7'd0: begin
      data1 <= #1 int_data0;
      data2 <= #1 int_data1;
      data3 <= #1 int_data2;
	end
    7'd1: begin
      data1 <= #1 int_data1;
      data2 <= #1 int_data2;
      data3 <= #1 int_data3;
	end
    7'd2: begin
      data1 <= #1 int_data2;
      data2 <= #1 int_data3;
      data3 <= #1 int_data4;
	end
    7'd3: begin
      data1 <= #1 int_data3;
      data2 <= #1 int_data4;
      data3 <= #1 int_data5;
	end
    7'd4: begin
      data1 <= #1 int_data4;
      data2 <= #1 int_data5;
      data3 <= #1 int_data6;
	end
    7'd5: begin
      data1 <= #1 int_data5;
      data2 <= #1 int_data6;
      data3 <= #1 int_data7;
	end
    7'd6: begin
      data1 <= #1 int_data6;
      data2 <= #1 int_data7;
      data3 <= #1 int_data8;
	end
    7'd7: begin
      data1 <= #1 int_data7;
      data2 <= #1 int_data8;
      data3 <= #1 int_data9;
	end
    7'd8: begin
      data1 <= #1 int_data8;
      data2 <= #1 int_data9;
      data3 <= #1 int_data10;
	end
    7'd9: begin
      data1 <= #1 int_data9;
      data2 <= #1 int_data10;
      data3 <= #1 int_data11;
	end
    7'd10: begin
      data1 <= #1 int_data10;
      data2 <= #1 int_data11;
      data3 <= #1 int_data12;
	end
    7'd11: begin
      data1 <= #1 int_data11;
      data2 <= #1 int_data12;
      data3 <= #1 int_data13;
	end
    7'd12: begin
      data1 <= #1 int_data12;
      data2 <= #1 int_data13;
      data3 <= #1 int_data14;
	end
    7'd13: begin
      data1 <= #1 int_data13;
      data2 <= #1 int_data14;
      data3 <= #1 int_data15;
	end
    7'd14: begin
      data1 <= #1 int_data14;
      data2 <= #1 int_data15;
      data3 <= #1 int_data16;
	end
    7'd15: begin
      data1 <= #1 int_data15;
      data2 <= #1 int_data16;
      data3 <= #1 int_data17;
	end
    7'd16: begin
      data1 <= #1 int_data16;
      data2 <= #1 int_data17;
      data3 <= #1 int_data18;
	end
    7'd17: begin
      data1 <= #1 int_data17;
      data2 <= #1 int_data18;
      data3 <= #1 int_data19;
	end
    7'd18: begin
      data1 <= #1 int_data18;
      data2 <= #1 int_data19;
      data3 <= #1 int_data20;
	end
    7'd19: begin
      data1 <= #1 int_data19;
      data2 <= #1 int_data20;
      data3 <= #1 int_data21;
	end
    7'd20: begin
      data1 <= #1 int_data20;
      data2 <= #1 int_data21;
      data3 <= #1 int_data22;
	end
    7'd21: begin
      data1 <= #1 int_data21;
      data2 <= #1 int_data22;
      data3 <= #1 int_data23;
	end
    7'd22: begin
      data1 <= #1 int_data22;
      data2 <= #1 int_data23;
      data3 <= #1 int_data24;
	end
    7'd23: begin
      data1 <= #1 int_data23;
      data2 <= #1 int_data24;
      data3 <= #1 int_data25;
	end
    7'd24: begin
      data1 <= #1 int_data24;
      data2 <= #1 int_data25;
      data3 <= #1 int_data26;
	end
    7'd25: begin
      data1 <= #1 int_data25;
      data2 <= #1 int_data26;
      data3 <= #1 int_data27;
	end
    7'd26: begin
      data1 <= #1 int_data26;
      data2 <= #1 int_data27;
      data3 <= #1 int_data28;
	end
    7'd27: begin
      data1 <= #1 int_data27;
      data2 <= #1 int_data28;
      data3 <= #1 int_data29;
	end
    7'd28: begin
      data1 <= #1 int_data28;
      data2 <= #1 int_data29;
      data3 <= #1 int_data30;
	end
    7'd29: begin
      data1 <= #1 int_data29;
      data2 <= #1 int_data30;
      data3 <= #1 int_data31;
	end
    7'd30: begin
      data1 <= #1 int_data30;
      data2 <= #1 int_data31;
      data3 <= #1 int_data32;
	end
    7'd31: begin
      data1 <= #1 int_data31;
      data2 <= #1 int_data32;
      data3 <= #1 int_data33;
	end
    7'd32: begin
      data1 <= #1 int_data32;
      data2 <= #1 int_data33;
      data3 <= #1 int_data34;
	end
    7'd33: begin
      data1 <= #1 int_data33;
      data2 <= #1 int_data34;
      data3 <= #1 int_data35;
	end
    7'd34: begin
      data1 <= #1 int_data34;
      data2 <= #1 int_data35;
      data3 <= #1 int_data36;
	end
    7'd35: begin
      data1 <= #1 int_data35;
      data2 <= #1 int_data36;
      data3 <= #1 int_data37;
	end
    7'd36: begin
      data1 <= #1 int_data36;
      data2 <= #1 int_data37;
      data3 <= #1 int_data38;
	end
    7'd37: begin
      data1 <= #1 int_data37;
      data2 <= #1 int_data38;
      data3 <= #1 int_data39;
	end
    7'd38: begin
      data1 <= #1 int_data38;
      data2 <= #1 int_data39;
      data3 <= #1 int_data40;
	end
    7'd39: begin
      data1 <= #1 int_data39;
      data2 <= #1 int_data40;
      data3 <= #1 int_data41;
	end
    7'd40: begin
      data1 <= #1 int_data40;
      data2 <= #1 int_data41;
      data3 <= #1 int_data42;
	end
    7'd41: begin
      data1 <= #1 int_data41;
      data2 <= #1 int_data42;
      data3 <= #1 int_data43;
	end
    7'd42: begin
      data1 <= #1 int_data42;
      data2 <= #1 int_data43;
      data3 <= #1 int_data44;
	end
    7'd43: begin
      data1 <= #1 int_data43;
      data2 <= #1 int_data44;
      data3 <= #1 int_data45;
	end
    7'd44: begin
      data1 <= #1 int_data44;
      data2 <= #1 int_data45;
      data3 <= #1 int_data46;
	end
    7'd45: begin
      data1 <= #1 int_data45;
      data2 <= #1 int_data46;
      data3 <= #1 int_data47;
	end
    7'd46: begin
      data1 <= #1 int_data46;
      data2 <= #1 int_data47;
      data3 <= #1 int_data48;
	end
    7'd47: begin
      data1 <= #1 int_data47;
      data2 <= #1 int_data48;
      data3 <= #1 int_data49;
	end
    7'd48: begin
      data1 <= #1 int_data48;
      data2 <= #1 int_data49;
      data3 <= #1 int_data50;
	end
    7'd49: begin
      data1 <= #1 int_data49;
      data2 <= #1 int_data50;
      data3 <= #1 int_data51;
	end
    7'd50: begin
      data1 <= #1 int_data50;
      data2 <= #1 int_data51;
      data3 <= #1 int_data52;
	end
    7'd51: begin
      data1 <= #1 int_data51;
      data2 <= #1 int_data52;
      data3 <= #1 int_data53;
	end
    7'd52: begin
      data1 <= #1 int_data52;
      data2 <= #1 int_data53;
      data3 <= #1 int_data54;
	end
    7'd53: begin
      data1 <= #1 int_data53;
      data2 <= #1 int_data54;
      data3 <= #1 int_data55;
	end
    7'd54: begin
      data1 <= #1 int_data54;
      data2 <= #1 int_data55;
      data3 <= #1 int_data56;
	end
    7'd55: begin
      data1 <= #1 int_data55;
      data2 <= #1 int_data56;
      data3 <= #1 int_data57;
	end
    7'd56: begin
      data1 <= #1 int_data56;
      data2 <= #1 int_data57;
      data3 <= #1 int_data58;
	end
    7'd57: begin
      data1 <= #1 int_data57;
      data2 <= #1 int_data58;
      data3 <= #1 int_data59;
	end
    7'd58: begin
      data1 <= #1 int_data58;
      data2 <= #1 int_data59;
      data3 <= #1 int_data60;
	end
    7'd59: begin
      data1 <= #1 int_data59;
      data2 <= #1 int_data60;
      data3 <= #1 int_data61;
	end
    7'd60: begin
      data1 <= #1 int_data60;
      data2 <= #1 int_data61;
      data3 <= #1 int_data62;
	end
    7'd61: begin
      data1 <= #1 int_data61;
      data2 <= #1 int_data62;
      data3 <= #1 int_data63;
	end
    7'd62: begin
      data1 <= #1 int_data62;
      data2 <= #1 int_data63;
      data3 <= #1 int_data64;
	end
    7'd63: begin
      data1 <= #1 int_data63;
      data2 <= #1 int_data64;
      data3 <= #1 int_data65;
	end
    7'd64: begin
      data1 <= #1 int_data64;
      data2 <= #1 int_data65;
      data3 <= #1 int_data66;
	end
    7'd65: begin
      data1 <= #1 int_data65;
      data2 <= #1 int_data66;
      data3 <= #1 int_data67;
	end
    7'd66: begin
      data1 <= #1 int_data66;
      data2 <= #1 int_data67;
      data3 <= #1 int_data68;
	end
    7'd67: begin
      data1 <= #1 int_data67;
      data2 <= #1 int_data68;
      data3 <= #1 int_data69;
	end
    7'd68: begin
      data1 <= #1 int_data68;
      data2 <= #1 int_data69;
      data3 <= #1 int_data70;
	end
    7'd69: begin
      data1 <= #1 int_data69;
      data2 <= #1 int_data70;
      data3 <= #1 int_data71;
	end
    7'd70: begin
      data1 <= #1 int_data70;
      data2 <= #1 int_data71;
      data3 <= #1 int_data72;
	end
    7'd71: begin
      data1 <= #1 int_data71;
      data2 <= #1 int_data72;
      data3 <= #1 int_data73;
	end
    7'd72: begin
      data1 <= #1 int_data72;
      data2 <= #1 int_data73;
      data3 <= #1 int_data74;
	end
    7'd73: begin
      data1 <= #1 int_data73;
      data2 <= #1 int_data74;
      data3 <= #1 int_data75;
	end
    7'd74: begin
      data1 <= #1 int_data74;
      data2 <= #1 int_data75;
      data3 <= #1 int_data76;
	end
    7'd75: begin
      data1 <= #1 int_data75;
      data2 <= #1 int_data76;
      data3 <= #1 int_data77;
	end
    7'd76: begin
      data1 <= #1 int_data76;
      data2 <= #1 int_data77;
      data3 <= #1 int_data78;
	end
    7'd77: begin
      data1 <= #1 int_data77;
      data2 <= #1 int_data78;
      data3 <= #1 int_data79;
	end
    7'd78: begin
      data1 <= #1 int_data78;
      data2 <= #1 int_data79;
      data3 <= #1 int_data80;
	end
    7'd79: begin
      data1 <= #1 int_data79;
      data2 <= #1 int_data80;
      data3 <= #1 int_data81;
	end
    7'd80: begin
      data1 <= #1 int_data80;
      data2 <= #1 int_data81;
      data3 <= #1 int_data82;
	end
    7'd81: begin
      data1 <= #1 int_data81;
      data2 <= #1 int_data82;
      data3 <= #1 int_data83;
	end
    7'd82: begin
      data1 <= #1 int_data82;
      data2 <= #1 int_data83;
      data3 <= #1 int_data84;
	end
    7'd83: begin
      data1 <= #1 int_data83;
      data2 <= #1 int_data84;
      data3 <= #1 int_data85;
	end
    7'd84: begin
      data1 <= #1 int_data84;
      data2 <= #1 int_data85;
      data3 <= #1 int_data86;
	end
    7'd85: begin
      data1 <= #1 int_data85;
      data2 <= #1 int_data86;
      data3 <= #1 int_data87;
	end
    7'd86: begin
      data1 <= #1 int_data86;
      data2 <= #1 int_data87;
      data3 <= #1 int_data88;
	end
    7'd87: begin
      data1 <= #1 int_data87;
      data2 <= #1 int_data88;
      data3 <= #1 int_data89;
	end
    7'd88: begin
      data1 <= #1 int_data88;
      data2 <= #1 int_data89;
      data3 <= #1 int_data90;
	end
    7'd89: begin
      data1 <= #1 int_data89;
      data2 <= #1 int_data90;
      data3 <= #1 int_data91;
	end
    7'd90: begin
      data1 <= #1 int_data90;
      data2 <= #1 int_data91;
      data3 <= #1 int_data92;
	end
    7'd91: begin
      data1 <= #1 int_data91;
      data2 <= #1 int_data92;
      data3 <= #1 int_data93;
	end
    7'd92: begin
      data1 <= #1 int_data92;
      data2 <= #1 int_data93;
      data3 <= #1 int_data94;
	end
    7'd93: begin
      data1 <= #1 int_data93;
      data2 <= #1 int_data94;
      data3 <= #1 int_data95;
	end
    7'd94: begin
      data1 <= #1 int_data94;
      data2 <= #1 int_data95;
      data3 <= #1 int_data96;
	end
    7'd95: begin
      data1 <= #1 int_data95;
      data2 <= #1 int_data96;
      data3 <= #1 int_data97;
	end
    7'd96: begin
      data1 <= #1 int_data96;
      data2 <= #1 int_data97;
      data3 <= #1 int_data98;
	end
    7'd97: begin
      data1 <= #1 int_data97;
      data2 <= #1 int_data98;
      data3 <= #1 int_data99;
	end
    7'd98: begin
      data1 <= #1 int_data98;
      data2 <= #1 int_data99;
      data3 <= #1 int_data100;
	end
    7'd99: begin
      data1 <= #1 int_data99;
      data2 <= #1 int_data100;
      data3 <= #1 int_data101;
	end
    7'd100: begin
      data1 <= #1 int_data100;
      data2 <= #1 int_data101;
      data3 <= #1 int_data102;
	end
    7'd101: begin
      data1 <= #1 int_data101;
      data2 <= #1 int_data102;
      data3 <= #1 int_data103;
	end
    7'd102: begin
      data1 <= #1 int_data102;
      data2 <= #1 int_data103;
      data3 <= #1 int_data104;
	end
    7'd103: begin
      data1 <= #1 int_data103;
      data2 <= #1 int_data104;
      data3 <= #1 int_data105;
	end
    7'd104: begin
      data1 <= #1 int_data104;
      data2 <= #1 int_data105;
      data3 <= #1 int_data106;
	end
    7'd105: begin
      data1 <= #1 int_data105;
      data2 <= #1 int_data106;
      data3 <= #1 int_data107;
	end
    7'd106: begin
      data1 <= #1 int_data106;
      data2 <= #1 int_data107;
      data3 <= #1 int_data108;
	end
    7'd107: begin
      data1 <= #1 int_data107;
      data2 <= #1 int_data108;
      data3 <= #1 int_data109;
	end
    7'd108: begin
      data1 <= #1 int_data108;
      data2 <= #1 int_data109;
      data3 <= #1 int_data110;
	end
    7'd109: begin
      data1 <= #1 int_data109;
      data2 <= #1 int_data110;
      data3 <= #1 int_data111;
	end
    7'd110: begin
      data1 <= #1 int_data110;
      data2 <= #1 int_data111;
      data3 <= #1 int_data112;
	end
    7'd111: begin
      data1 <= #1 int_data111;
      data2 <= #1 int_data112;
      data3 <= #1 int_data113;
	end
    7'd112: begin
      data1 <= #1 int_data112;
      data2 <= #1 int_data113;
      data3 <= #1 int_data114;
	end
    7'd113: begin
      data1 <= #1 int_data113;
      data2 <= #1 int_data114;
      data3 <= #1 int_data115;
	end
    7'd114: begin
      data1 <= #1 int_data114;
      data2 <= #1 int_data115;
      data3 <= #1 int_data116;
	end
    7'd115: begin
      data1 <= #1 int_data115;
      data2 <= #1 int_data116;
      data3 <= #1 int_data117;
	end
    7'd116: begin
      data1 <= #1 int_data116;
      data2 <= #1 int_data117;
      data3 <= #1 int_data118;
	end
    7'd117: begin
      data1 <= #1 int_data117;
      data2 <= #1 int_data118;
      data3 <= #1 int_data119;
	end
    7'd118: begin
      data1 <= #1 int_data118;
      data2 <= #1 int_data119;
      data3 <= #1 int_data120;
	end
    7'd119: begin
      data1 <= #1 int_data119;
      data2 <= #1 int_data120;
      data3 <= #1 int_data121;
	end
    7'd120: begin
      data1 <= #1 int_data120;
      data2 <= #1 int_data121;
      data3 <= #1 int_data122;
	end
    7'd121: begin
      data1 <= #1 int_data121;
      data2 <= #1 int_data122;
      data3 <= #1 int_data123;
	end
    7'd122: begin
      data1 <= #1 int_data122;
      data2 <= #1 int_data123;
      data3 <= #1 int_data124;
	end
    7'd123: begin
      data1 <= #1 int_data123;
      data2 <= #1 int_data124;
      data3 <= #1 int_data125;
	end
    7'd124: begin
      data1 <= #1 int_data124;
      data2 <= #1 int_data125;
      data3 <= #1 int_data126;
	end
    7'd125: begin
      data1 <= #1 int_data125;
      data2 <= #1 int_data126;
      data3 <= #1 int_data127;
	end
    7'd126: begin
      data1 <= #1 int_data126;
      data2 <= #1 int_data127;
      data3 <= #1 int_data0;
	end
    7'd127: begin
      data1 <= #1 int_data127;
      data2 <= #1 int_data0;
      data3 <= #1 int_data1;
	end
    default: begin
      data1 <= #1 8'h00;
      data2 <= #1 8'h00;
      data3 <= #1 8'h00;
	end
  endcase
end

always @(posedge clk or posedge rst)
 if (rst)
   ea_int <= #1 1'b1;
  else ea_int <= #1 !ea;

`else


reg [7:0] buff [0:65535]; //64kb

assign ea = 1'b0;

initial
begin
  $readmemh("../../../bench/in/oc8051_rom.in", buff);
end

always @(posedge clk or posedge rst)
 if (rst)
   ea_int <= #1 1'b1;
  else ea_int <= #1 !ea;

always @(posedge clk)
begin
  data_o <= #1 {buff[addr+3], buff[addr+2], buff[addr+1], buff[addr]};
end


`endif


endmodule


`ifdef OC8051_XILINX_ROM

//rom0
module rom0 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=004760c0" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00475754" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=032c2000" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00300087" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0228b085" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01002085" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=015378ed" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=024bd0c4" */;
endmodule

//rom1
module rom1 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=022c68a6" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0008e892" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=032feb64" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0010a020" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=015b2028" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00bca44c" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02bc34c2" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=016700c3" */;
endmodule

//rom2
module rom2 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00701310" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01a79000" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00a40cee" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01371711" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02340c6b" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=022c00c4" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=022c50c4" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00000aaa" */;
endmodule

//rom3
module rom3 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00f46b58" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=027c0c49" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0280bb9b" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00088848" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00807bbd" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00c30bf9" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01b72fbd" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00343c40" */;
endmodule

//rom4
module rom4 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0340f400" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01036080" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03677000" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0110b0c4" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00ecabfa" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0248ecaa" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0258ecea" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02a4c790" */;
endmodule

//rom5
module rom5 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02638826" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02cca816" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03c36298" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02e40029" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02e3d7a0" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03431409" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02d394c5" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0020c005" */;
endmodule

//rom6
module rom6 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=010070c0" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01885b92" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0318302c" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=004044c0" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0223a46d" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=012c20a9" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=002cb029" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03030084" */;
endmodule

//rom7
module rom7 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00289808" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000d8429" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02bf1b1c" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00820339" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=033b6429" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=010b5069" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=034854e9" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00503421" */;
endmodule

//rom8
module rom8 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=019c8ff3" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01438c62" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=002c6c4b" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01904b92" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00bc93d5" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02acb31d" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02bc1335" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00c09b32" */;
endmodule

//rom9
module rom9 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01004a06" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0300642c" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02835972" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00230208" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000cdb73" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00904fb2" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01f0cb93" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01cc1441" */;
endmodule

//rom10
module rom10 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=026f5480" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00d8c4d4" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0338e680" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00036050" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03a358a2" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03234240" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03af4228" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00c34942" */;
endmodule

//rom11
module rom11 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00b2e334" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01ac492a" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00b38899" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01202a00" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0232faf1" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00b3b259" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00f3f551" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02c16380" */;
endmodule

//rom12
module rom12 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02637465" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00434de2" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01ce621d" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=026e0080" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01f3a04d" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02422290" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02433231" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=019181a8" */;
endmodule

//rom13
module rom13 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=006ca309" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0232a510" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0151b922" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=001eaa5a" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000d72e1" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=014db1c4" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=017d71c5" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00ec0129" */;
endmodule

//rom14
module rom14 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00002cb2" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0033c0d5" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02085465" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02450843" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=010c18cb" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=008c184a" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01ae0b1a" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00021853" */;
endmodule

//rom15
module rom15 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02247616" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00a64f08" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01f6f6d7" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02368d00" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=026f3ef6" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=022658fe" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=022f78e6" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02406000" */;
endmodule

//rom16
module rom16 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00d9a200" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00442ca0" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=038f21fa" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0010aaa0" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=029cf3cc" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03def39a" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02bef75b" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00129484" */;
endmodule

//rom17
module rom17 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0223de06" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01237e04" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=021014e3" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01209000" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03da88a2" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=025882a2" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=015b8ab2" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=008b0908" */;
endmodule

//rom18
module rom18 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00822340" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00804d64" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00e7108b" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02c00a20" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02b10be3" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=030c062a" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=038421f2" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=003f3761" */;
endmodule

//rom19
module rom19 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0256e421" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0387452b" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=015ac695" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0023004e" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00160871" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=005aa200" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02769a61" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=024ca130" */;
endmodule

//rom20
module rom20 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01507386" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=001643a0" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01962932" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02d25004" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=013678b4" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03373167" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=033f25cd" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00000184" */;
endmodule

//rom21
module rom21 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00b0cc00" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00251ac1" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0269408f" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=009c8448" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03ca61bc" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=008a5016" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=008249b6" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03c03800" */;
endmodule

//rom22
module rom22 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00730101" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02101d00" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03212219" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00020834" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=021dd058" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=032da894" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=033d9cc2" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02b05c09" */;
endmodule

//rom23
module rom23 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00348623" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01a827a8" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00b08e12" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=002c40cc" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=006b0816" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02684216" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02721036" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00010a28" */;
endmodule

//rom24
module rom24 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00b31840" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=027b4450" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03d3b126" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01102810" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=009592ab" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0099940c" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00b482cc" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00280463" */;
endmodule

//rom25
module rom25 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0346a616" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000d0433" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02665666" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0100628a" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03b8385e" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03422906" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=035a2816" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01ea3050" */;
endmodule

//rom26
module rom26 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0134c390" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01522880" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0007195d" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0063da01" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=023ac37a" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0086433e" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01ae47ae" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03044080" */;
endmodule

//rom27
module rom27 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0017662b" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=029e6c24" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=014c0298" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02052805" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=014d99cb" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=032d9a5b" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0337da5b" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00534189" */;
endmodule

//rom28
module rom28 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=010c3346" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03248460" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00bba08b" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00c00204" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00137387" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0109320f" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000d0017" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=001e4200" */;
endmodule

//rom29
module rom29 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00b09c14" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00034969" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0324b616" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02930009" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00bcc090" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00e8b296" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01e8ba86" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=019c3800" */;
endmodule

//rom30
module rom30 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=028846e6" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00cc4ac2" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00406536" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02054903" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02b1d817" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02135427" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=039b500f" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02a8d030" */;
endmodule

//rom31
module rom31 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01404535" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=013262c8" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03da0477" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01100100" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02e8953b" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02c4f567" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02e41d27" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00084100" */;
endmodule

//rom32
module rom32 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=020350c1" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=022d8082" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0196734c" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00001803" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=015b0579" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=009a7155" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02936155" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03416428" */;
endmodule

//rom33
module rom33 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0244461a" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01404a0d" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0264b28e" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00b18ca9" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03cf3458" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=038d3a9e" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02dd3808" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00034bc8" */;
endmodule

//rom34
module rom34 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=023a8140" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02c82561" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=021a4892" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02aaa041" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0337c899" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=033a801b" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0331831f" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0024c980" */;
endmodule

//rom35
module rom35 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0311fa85" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01176acd" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02791cbd" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01d08cad" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02b85985" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02389991" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=003c3893" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0080a980" */;
endmodule

//rom36
module rom36 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02788531" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000c0311" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=030b214c" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=005a0463" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01701c2d" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03495dab" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03c95dbb" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00611534" */;
endmodule

//rom37
module rom37 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=033a1850" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0310588c" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=022cd6b2" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02071048" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02ea847e" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=022b847c" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=037ba568" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0153004b" */;
endmodule

//rom38
module rom38 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=017767c3" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0121ac20" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02b276f9" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00242623" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=032e45c5" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00ae46dd" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=013f44d5" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=034d1602" */;
endmodule

//rom39
module rom39 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00ba8a65" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=029a006c" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=039f2f41" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0032804c" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=025fe7c7" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02960743" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=035e1773" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03c164a1" */;
endmodule

//rom40
module rom40 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00247801" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=002328d1" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=004818a7" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000368d1" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01a0066d" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02c8dc77" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03f0844f" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00280b40" */;
endmodule

//rom41
module rom41 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01648f42" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=022455e1" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03d08a32" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0082042d" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00022b13" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00433807" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=006f2943" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00441151" */;
endmodule

//rom42
module rom42 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03aa200c" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01af321c" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=027bc4d7" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00226090" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02329d01" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=026e8557" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03e2947b" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01941948" */;
endmodule

//rom43
module rom43 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00114d80" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000a0901" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01c12f48" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00104a81" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03f16cd8" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=008d61d0" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=009775f4" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=036e418a" */;
endmodule

//rom44
module rom44 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0240f2c0" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02ca2854" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=010264be" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0382a241" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=012f26a6" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=035276d3" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=023e6782" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00096660" */;
endmodule

//rom45
module rom45 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000426c0" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02306582" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00160af4" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=008109a2" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=008932fc" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01e39298" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01cab6d9" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00026404" */;
endmodule

//rom46
module rom46 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=003e1a4a" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=016f90c9" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03d0b756" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02001ac5" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00161fe1" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00984bf2" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00b64bf1" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00260c84" */;
endmodule

//rom47
module rom47 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03489a90" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000e4698" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0163d823" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02810420" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03f0a835" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03449e26" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0348bad6" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02be1591" */;
endmodule

//rom48
module rom48 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00356b59" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00120945" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=021a6ab1" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00130841" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03ebdf75" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02075eb1" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02975fa3" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=016d9180" */;
endmodule

//rom49
module rom49 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03098402" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00500e22" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03c112b4" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00080a22" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=026ff387" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=032ba3d5" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=016722df" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00085108" */;
endmodule

//rom50
module rom50 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=002379e0" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0145bca5" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00b51303" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=023350c4" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02290863" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02894388" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02090b39" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000009a2" */;
endmodule

//rom51
module rom51 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02340e14" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02896092" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=036c9e79" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03d9801e" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=029e1bf5" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02341225" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02b61385" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00300042" */;
endmodule

//rom52
module rom52 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=036a8fa8" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00222fe5" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=023a6abc" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0161cee1" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03758e9a" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03f98ece" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03f98a8e" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=014c8050" */;
endmodule

//rom53
module rom53 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02265616" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02a40113" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03c2b616" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02006011" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0358fc4e" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=034af655" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=036edef6" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=003a6008" */;
endmodule

//rom54
module rom54 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02d1a2ea" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03070084" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00aae1ee" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01120a80" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00bbfef2" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00927236" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0293f217" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02691400" */;
endmodule

//rom55
module rom55 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=007a0ad1" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02528d1c" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=021c03e2" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01350714" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01b0e775" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=035903c6" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03120ac7" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00a328a4" */;
endmodule

//rom56
module rom56 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01952bc6" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=001186e4" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0253bbff" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000500e8" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=027f03f3" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01bd6bd6" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01bd09d2" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02461a03" */;
endmodule

//rom57
module rom57 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00544809" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02e6e850" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02208e42" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=011a8c59" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=022d897e" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02004afa" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03055abe" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0304c159" */;
endmodule

//rom58
module rom58 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=001056b8" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00155392" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01a9a848" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00802486" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=013c1de9" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=026a0469" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=027a0459" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01140c10" */;
endmodule

//rom59
module rom59 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01b82c17" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=025a1704" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=025200f3" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00a92801" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=008024f8" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01857032" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01b5a465" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0085348c" */;
endmodule

//rom60
module rom60 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02ef2408" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=008534a0" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=026ac37f" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000021a0" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03fe3c05" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=036e6cb3" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03fb7833" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00855484" */;
endmodule

//rom61
module rom61 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01300960" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00206069" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01a5c204" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00202101" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02ff9bd0" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00a80180" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00a817f2" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=024f8360" */;
endmodule

//rom62
module rom62 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=003cfcc1" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0053b483" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0282ddfa" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0363100b" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=006acbf4" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03824925" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03d0eb46" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01102000" */;
endmodule

//rom63
module rom63 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00a02cdc" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02622c54" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01981be2" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00002848" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01905aea" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=008d99c8" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00a59dd9" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0138451a" */;
endmodule

//rom64
module rom64 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00b00782" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01a5d486" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02c7819a" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03d25283" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0090079a" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=008057ca" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00a823eb" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00101412" */;
endmodule

//rom65
module rom65 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0267b804" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00a04ae8" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00259391" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02e202e8" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=027f3683" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=037713a2" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03771f02" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02428881" */;
endmodule

//rom66
module rom66 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00c0ae04" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=014a2603" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03904ba0" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0030600f" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02c51af8" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02083398" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=026231de" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00cf3321" */;
endmodule

//rom67
module rom67 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03834916" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=008a0c38" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03d62b96" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00834858" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03dbe987" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03d34b07" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01fe4a87" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0008e120" */;
endmodule

//rom68
module rom68 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00602648" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0025a248" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02025793" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00302200" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01004034" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02024132" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=029a497b" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0142104c" */;
endmodule

//rom69
module rom69 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02520980" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03d200b2" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=013d8465" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000dcd1a" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00f221db" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0150203b" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03123693" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=022200d2" */;
endmodule

//rom70
module rom70 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=003f9e10" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01081095" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02c23e62" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0035a201" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0257c4f2" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00878c22" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=002f8dba" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=024d8810" */;
endmodule

//rom71
module rom71 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00924b99" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02d2d9aa" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=038a29b5" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01032aaf" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=022f2fd8" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02a37af9" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02336ae0" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=029c5400" */;
endmodule

//rom72
module rom72 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0138a6e6" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=003ef660" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0030eb9a" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=010a2604" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01cc3f2c" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03c82396" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03d033b5" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=010838ac" */;
endmodule

//rom73
module rom73 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00206b81" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02802fc1" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03a54aaf" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00c00881" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=010efabd" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=012f4ba7" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=013bf996" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0021ab00" */;
endmodule

//rom74
module rom74 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02ef9140" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01e91044" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=023ca721" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01222414" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0221c92b" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02248b88" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02e58a8a" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00c54101" */;
endmodule

//rom75
module rom75 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=012c4e1b" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00245ad9" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01d8010d" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00014481" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03d3a449" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01c9450d" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01eb2573" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=023c8411" */;
endmodule

//rom76
module rom76 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03528166" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0183fb22" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0326347c" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03910020" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=013c80fd" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0308844d" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=031804cf" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=006c0412" */;
endmodule

//rom77
module rom77 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01e20811" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03448509" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00827a32" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00206089" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00ed1463" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01da2001" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01bb3404" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00022462" */;
endmodule

//rom78
module rom78 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=018a5cac" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=009b00ac" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03d39f43" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0282c040" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00c67a19" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00a67415" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01a67e9d" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01404ea8" */;
endmodule

//rom79
module rom79 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0352a910" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=012e2551" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01b40984" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03018109" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03109dae" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0391cc2a" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0393cb22" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02039484" */;
endmodule

//rom80
module rom80 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00ad0625" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00017235" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03a644c7" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00088067" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03b289a6" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02649407" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0265850c" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01970c00" */;
endmodule

//rom81
module rom81 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03804148" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01c6b210" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02c16122" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=008a8140" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02b944ef" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03804cee" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=019844e7" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=002d0040" */;
endmodule

//rom82
module rom82 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02682496" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=003108a5" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=024c930f" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00013474" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03ec617f" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=034a8163" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03eaa176" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0004e028" */;
endmodule

//rom83
module rom83 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0200f17b" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0322c6e1" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0396295e" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02105a40" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=024cb25e" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02057156" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0201b777" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00489721" */;
endmodule

//rom84
module rom84 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03520fc0" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=024140c4" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01ab0723" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00789804" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00523b39" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01969f2a" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=035e9e4a" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0310ad51" */;
endmodule

//rom85
module rom85 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01210038" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03cc3432" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=011561dd" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01a058aa" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01b38e80" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03312d4c" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03b189c0" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=002206c0" */;
endmodule

//rom86
module rom86 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0206cdc6" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02022b14" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=036e8ece" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=024a5804" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03375aed" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02a64ccf" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02a45ced" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01115400" */;
endmodule

//rom87
module rom87 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0078c041" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0193db40" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0280387a" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01492050" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=005d08e1" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02490c4b" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0258cc4e" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=025cc080" */;
endmodule

//rom88
module rom88 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=028434fc" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00210ce8" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02de9381" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00022ad4" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00e8b015" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03e83051" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03fc703f" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0200a078" */;
endmodule

//rom89
module rom89 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=027a6b03" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0304cd82" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03092175" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02138308" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=028a3b7f" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0222225b" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=022a2b51" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00e21a24" */;
endmodule

//rom90
module rom90 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01497425" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00c87850" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03062ea2" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000520d5" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=031b6a42" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0183f241" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01bba348" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02094046" */;
endmodule

//rom91
module rom91 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0004018a" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0282b74a" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0241824a" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00262092" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03550af6" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=020504be" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02054ef7" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03440080" */;
endmodule

//rom92
module rom92 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0110a4a4" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01316120" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0184debd" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00831021" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=002a8c8e" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03268693" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=037ea6d6" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00542208" */;
endmodule

//rom93
module rom93 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00827892" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=030b08ca" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=021a61bf" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=008a10ca" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0096d6f5" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00e252be" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00e35abc" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00b59c01" */;
endmodule

//rom94
module rom94 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02952101" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00c32aa5" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=034bad13" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=011536a5" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02bbf4ab" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0203b9fb" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02a339fb" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=009848a0" */;
endmodule

//rom95
module rom95 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=014101ba" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000584ba" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=006c4b06" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01844052" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03d90b63" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01d4430f" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01dd23a3" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=030108a0" */;
endmodule

//rom96
module rom96 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00172812" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02b64210" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0381a6ea" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02909c01" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01ef336a" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03a900ab" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03ab129a" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=004711d4" */;
endmodule

//rom97
module rom97 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0021a5a0" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=030b11f0" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00d7721b" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0149c460" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00d78d91" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00b3ae5b" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00f3ae8f" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0026a640" */;
endmodule

//rom98
module rom98 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0000f08d" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0044298f" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=028cbc85" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03106a87" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=010bf6e9" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=018cf6c1" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01bff6fd" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00004208" */;
endmodule

//rom99
module rom99 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03188093" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01b88890" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01e0e946" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=034b83b9" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=038cf85c" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=038cca9b" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=039cca00" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02103684" */;
endmodule

//rom100
module rom100 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=036c3160" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=008cb024" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03334452" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00c4a104" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03600350" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03631742" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=037b03eb" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=004b0134" */;
endmodule

//rom101
module rom101 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03530ed9" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03c01a41" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=038781f8" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=031004c8" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03f465fc" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0307a145" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0347a946" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00e3cc20" */;
endmodule

//rom102
module rom102 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0307e084" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0103ea66" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02637deb" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00033044" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02638c59" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=034301d2" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01fb8594" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000084c9" */;
endmodule

//rom103
module rom103 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0247f099" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03670098" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0118a7de" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00005089" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=008435f6" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0200f1fc" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0303f8ec" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01079503" */;
endmodule

//rom104
module rom104 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0240e222" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0240e224" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03bfa350" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0220be06" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=035ff459" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0280f799" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0200f7d9" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=011f0202" */;
endmodule

//rom105
module rom105 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03bc7389" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=038f2831" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00c42a45" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=003b2199" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=007ca38b" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01ac6e06" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=033cee02" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02e06800" */;
endmodule

//rom106
module rom106 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0200e054" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00002472" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02208c8d" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00001000" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01a07add" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03bf400f" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03bf715d" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02403ad4" */;
endmodule

//rom107
module rom107 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0220ef00" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=031f9bf8" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=039b2f52" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02840888" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02400f7a" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02404cba" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0240cc6e" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0000c8a9" */;
endmodule

//rom108
module rom108 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01bb702d" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00a02208" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=031ba124" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00402001" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=021feda0" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=011f69e1" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01bf79fc" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03a0940a" */;
endmodule

//rom109
module rom109 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01404d92" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03448940" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03801686" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01008648" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=039b34bc" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03a0511a" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03c07101" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=027b4024" */;
endmodule

//rom110
module rom110 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=001972a4" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00856c05" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=014c7196" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00890225" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=015153dd" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0265d386" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=027ed2ce" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01200314" */;
endmodule

//rom111
module rom111 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00446a88" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03df82b8" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0284ad91" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=010004c0" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0084129d" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00846184" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00a46386" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00007201" */;
endmodule

//rom112
module rom112 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0264ff4c" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00403bc9" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=029fa8a2" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=003b24a3" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0244ec92" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0384fc90" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0384ee5c" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00000ec9" */;
endmodule

//rom113
module rom113 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=001b48b0" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01801180" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01c04c5e" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=001b0030" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02dfecef" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=003b6c35" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=001b6ea5" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02ff80ea" */;
endmodule

//rom114
module rom114 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0322a0a0" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=020426a7" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=031e9359" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02011406" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=019bc8f1" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=033b8010" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03bf00b0" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00204822" */;
endmodule

//rom115
module rom115 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00025fa0" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0207e780" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00820095" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00035080" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=013a14cb" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=004258df" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=006216fb" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01181700" */;
endmodule

//rom116
module rom116 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=003010f1" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0043687c" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02f4b489" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=030e802e" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=001815a7" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01981025" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=011913b5" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00010035" */;
endmodule

//rom117
module rom117 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=027d93a1" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01b11250" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=005179ba" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=029ff901" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02f7b43e" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0257b0bb" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02f7b4ae" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0224a080" */;
endmodule

//rom118
module rom118 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=021968c6" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000b16cc" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03062182" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0112d88c" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=024fedae" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02a6e98e" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=028fe99b" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0069482a" */;
endmodule

//rom119
module rom119 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0154955f" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0092843c" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00c7db12" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=014c8020" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0352f809" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0123c992" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=017bdd8c" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0331344c" */;
endmodule

//rom120
module rom120 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0284ba00" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02543ae0" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0380b757" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=02011a88" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0388df5d" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0380bb0a" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03b89a42" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00050f15" */;
endmodule

//rom121
module rom121 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00198e11" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0031b011" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01779b35" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00518431" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=009dca89" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0013cb51" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01138bdb" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00860001" */;
endmodule

//rom122
module rom122 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00ea1120" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=008e8142" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00247040" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0140a0a8" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01731e37" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01e55c35" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01ff5c35" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=004a4a20" */;
endmodule

//rom123
module rom123 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=015124f9" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01c249bc" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=010a8447" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01012601" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=012c3a46" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0116a054" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0144e0e8" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=026af0aa" */;
endmodule

//rom124
module rom124 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=017ea002" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00568202" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03a21d9f" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=028e2002" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=036be4df" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=03027417" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0373a437" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=025888ca" */;
endmodule

//rom125
module rom125 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0181416c" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0121b121" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0155e2fc" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01802241" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01de20a7" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01cce037" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01cee837" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0090c121" */;
endmodule

//rom126
module rom126 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01c72e11" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00846c52" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=012d7806" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=002a1a23" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0190a2b8" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=000122ad" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0181afb8" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00910610" */;
endmodule

//rom127
module rom127 (o,a);
input [4:0] a;
output [7:0] o;
ROM32X1 u0 (o[0],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=0025d188" */;
ROM32X1 u1 (o[1],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=010529bc" */;
ROM32X1 u2 (o[2],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01c9c652" */;
ROM32X1 u3 (o[3],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00213064" */;
ROM32X1 u4 (o[4],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01fb5633" */;
ROM32X1 u5 (o[5],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=00bb5a32" */;
ROM32X1 u6 (o[6],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01bb5223" */;
ROM32X1 u7 (o[7],a[0],a[1],a[2],a[3],a[4]) /* synthesis xc_props="INIT=01641820" */;
endmodule


`endif
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 cores sfr top level module                             ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   special function registers for oc8051                      ////
////                                                              ////
////  To Do:                                                      ////
////    nothing                                                   ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.14  2003/05/07 12:39:20  simont
// fix bug in case of sequence of inc dptr instrucitons.
//
// Revision 1.13  2003/05/05 15:46:37  simont
// add aditional alu destination to solve critical path.
//
// Revision 1.12  2003/04/29 11:24:31  simont
// fix bug in case execution of two data dependent instructions.
//
// Revision 1.11  2003/04/25 17:15:51  simont
// change branch instruction execution (reduse needed clock periods).
//
// Revision 1.10  2003/04/10 12:43:19  simont
// defines for pherypherals added
//
// Revision 1.9  2003/04/09 16:24:03  simont
// change wr_sft to 2 bit wire.
//
// Revision 1.8  2003/04/09 15:49:42  simont
// Register oc8051_sfr dato output, add signal wait_data.
//
// Revision 1.7  2003/04/07 14:58:02  simont
// change sfr's interface.
//
// Revision 1.6  2003/04/07 13:29:16  simont
// change uart to meet timing.
//
// Revision 1.5  2003/04/04 10:35:07  simont
// signal prsc_ow added.
//
// Revision 1.4  2003/03/28 17:45:57  simont
// change module name.
//
// Revision 1.3  2003/01/21 13:51:30  simont
// add include oc8051_defines.v
//
// Revision 1.2  2003/01/13 14:14:41  simont
// replace some modules
//
// Revision 1.1  2002/11/05 17:22:27  simont
// initial import
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"


module oc8051_sfr (rst, clk,
       adr0, adr1, dat0, 
       dat1, dat2, bit_in,
       des_acc,
       we, wr_bit,
       bit_out,
       wr_sfr, acc, 
       ram_wr_sel, ram_rd_sel, 
       sp, sp_w, 
       bank_sel, 
       desAc, desOv,
       srcAc, cy,
       psw_set, rmw,
       comp_sel,
       comp_wait,

`ifdef OC8051_PORTS

  `ifdef OC8051_PORT0
       p0_out,
       p0_in,
  `endif

  `ifdef OC8051_PORT1
       p1_out,
       p1_in,
  `endif

  `ifdef OC8051_PORT2
       p2_out,
       p2_in,
  `endif

  `ifdef OC8051_PORT3
       p3_out,
       p3_in,
  `endif

`endif


  `ifdef OC8051_UART
       rxd, txd,
  `endif

       int_ack, intr,
       int0, int1,
       int_src,
       reti,

  `ifdef OC8051_TC01
       t0, t1,
  `endif

  `ifdef OC8051_TC2
       t2, t2ex,
  `endif

       dptr_hi, dptr_lo,
       wait_data);


input       rst,	// reset - pin
	    clk,	// clock - pin
            we,		// write enable
	    bit_in,
	    desAc,
	    desOv,
	    rmw;
input       int_ack,
            int0,
	    int1,
            reti,
	    wr_bit;
input [1:0] psw_set,
            wr_sfr,
	    comp_sel;
input [2:0] ram_rd_sel,
            ram_wr_sel;
input [7:0] adr0, 	//address 0 input
            adr1, 	//address 1 input
	    des_acc,
	    dat1,	//data 1 input (des1)
            dat2;	//data 2 input (des2)

output       bit_out,
             intr,
             srcAc,
	     cy,
	     wait_data,
	     comp_wait;
output [1:0] bank_sel;
output [7:0] dat0,	//data output
	     int_src,
	     dptr_hi,
	     dptr_lo,
	     acc;
output [7:0] sp,
             sp_w;

// ports
`ifdef OC8051_PORTS

`ifdef OC8051_PORT0
input  [7:0] p0_in;
output [7:0] p0_out;
wire   [7:0] p0_data;
`endif

`ifdef OC8051_PORT1
input  [7:0] p1_in;
output [7:0] p1_out;
wire   [7:0] p1_data;
`endif

`ifdef OC8051_PORT2
input  [7:0] p2_in;
output [7:0] p2_out;
wire   [7:0] p2_data;
`endif

`ifdef OC8051_PORT3
input  [7:0] p3_in;
output [7:0] p3_out;
wire   [7:0] p3_data;
`endif

`endif


// serial interface
`ifdef OC8051_UART
input        rxd;
output       txd;
`endif

// timer/counter 0,1
`ifdef OC8051_TC01
input	     t0, t1;
`endif

// timer/counter 2
`ifdef OC8051_TC2
input	     t2, t2ex;
`endif

reg        bit_out, 
           wait_data;
reg [7:0]  dat0,
           adr0_r;

reg        wr_bit_r;
reg [2:0]  ram_wr_sel_r;


wire       p,
           uart_int,
	   tf0,
	   tf1,
	   tr0,
	   tr1,
           rclk,
           tclk,
	   brate2,
	   tc2_int;


wire [7:0] b_reg, 
           psw,

`ifdef OC8051_TC2
  // t/c 2
	   t2con, 
	   tl2, 
	   th2, 
	   rcap2l, 
	   rcap2h,
`endif

`ifdef OC8051_TC01
  // t/c 0,1
	   tmod, 
	   tl0, 
	   th0, 
	   tl1,
	   th1,
`endif

  // serial interface
`ifdef OC8051_UART
           scon, 
	   pcon, 
	   sbuf,
`endif

  //interrupt control
	   ie, 
	   tcon, 
	   ip;


reg        pres_ow;
reg [3:0]  prescaler;


assign cy = psw[7];
assign srcAc = psw [6];



//
// accumulator
// ACC
oc8051_acc oc8051_acc1(.clk(clk), 
                       .rst(rst), 
		       .bit_in(bit_in), 
		       .data_in(des_acc),
		       .data2_in(dat2),
		       .wr(we),
		       .wr_bit(wr_bit_r),
		       .wr_sfr(wr_sfr),
		       .wr_addr(adr1),
		       .data_out(acc),
		       .p(p));


//
// b register
// B
oc8051_b_register oc8051_b_register (.clk(clk),
                                     .rst(rst),
				     .bit_in(bit_in),
				     .data_in(des_acc),
				     .wr(we), 
				     .wr_bit(wr_bit_r), 
				     .wr_addr(adr1),
				     .data_out(b_reg));

//
//stack pointer
// SP
oc8051_sp oc8051_sp1(.clk(clk), 
                     .rst(rst), 
		     .ram_rd_sel(ram_rd_sel), 
		     .ram_wr_sel(ram_wr_sel), 
		     .wr_addr(adr1), 
		     .wr(we), 
		     .wr_bit(wr_bit_r), 
		     .data_in(dat1), 
		     .sp_out(sp), 
		     .sp_w(sp_w));

//
//data pointer
// DPTR, DPH, DPL
oc8051_dptr oc8051_dptr1(.clk(clk), 
                         .rst(rst), 
			 .addr(adr1), 
			 .data_in(des_acc),
			 .data2_in(dat2), 
			 .wr(we), 
			 .wr_bit(wr_bit_r),
			 .data_hi(dptr_hi),
			 .data_lo(dptr_lo), 
			 .wr_sfr(wr_sfr));


//
//program status word
// PSW
oc8051_psw oc8051_psw1 (.clk(clk), 
                        .rst(rst), 
			.wr_addr(adr1), 
			.data_in(dat1),
			.wr(we), 
			.wr_bit(wr_bit_r), 
			.data_out(psw), 
			.p(p), 
			.cy_in(bit_in),
			.ac_in(desAc), 
			.ov_in(desOv), 
			.set(psw_set), 
			.bank_sel(bank_sel));

//
// ports
// P0, P1, P2, P3
`ifdef OC8051_PORTS
  oc8051_ports oc8051_ports1(.clk(clk),
                           .rst(rst),
			   .bit_in(bit_in),
			   .data_in(dat1),
			   .wr(we),
			   .wr_bit(wr_bit_r),
			   .wr_addr(adr1),

		`ifdef OC8051_PORT0
			   .p0_out(p0_out),
			   .p0_in(p0_in),
			   .p0_data(p0_data),
		`endif

		`ifdef OC8051_PORT1
			   .p1_out(p1_out),
			   .p1_in(p1_in),
			   .p1_data(p1_data),
		`endif

		`ifdef OC8051_PORT2
			   .p2_out(p2_out),
			   .p2_in(p2_in),
			   .p2_data(p2_data),
		`endif

		`ifdef OC8051_PORT3
			   .p3_out(p3_out),
			   .p3_in(p3_in),
			   .p3_data(p3_data),
		`endif

			   .rmw(rmw));
`endif

//
// serial interface
// SCON, SBUF
`ifdef OC8051_UART
  oc8051_uart oc8051_uatr1 (.clk(clk), 
                            .rst(rst), 
			    .bit_in(bit_in),
			    .data_in(dat1), 
			    .wr(we), 
			    .wr_bit(wr_bit_r), 
			    .wr_addr(adr1),
			    .rxd(rxd), 
			    .txd(txd), 
		// interrupt
			    .intr(uart_int),
		// baud rate sources
			    .brate2(brate2),
			    .t1_ow(tf1),
			    .pres_ow(pres_ow),
			    .rclk(rclk),
			    .tclk(tclk),
		//registers
			    .scon(scon),
			    .pcon(pcon),
			    .sbuf(sbuf));
`else
  assign uart_int = 1'b0;
`endif

//
// interrupt control
// IP, IE, TCON
oc8051_int oc8051_int1 (.clk(clk), 
                        .rst(rst), 
			.wr_addr(adr1), 
			.bit_in(bit_in),
			.ack(int_ack), 
			.data_in(dat1),
			.wr(we), 
			.wr_bit(wr_bit_r),
			.tf0(tf0), 
			.tf1(tf1), 
			.t2_int(tc2_int), 
			.tr0(tr0), 
			.tr1(tr1),
			.ie0(int0), 
			.ie1(int1),
			.uart_int(uart_int),
			.reti(reti),
			.intr(intr),
			.int_vec(int_src),
			.ie(ie),
			.tcon(tcon), 
			.ip(ip));


//
// timer/counter control
// TH0, TH1, TL0, TH1, TMOD
`ifdef OC8051_TC01
  oc8051_tc oc8051_tc1(.clk(clk), 
                       .rst(rst), 
		       .wr_addr(adr1),
		       .data_in(dat1), 
		       .wr(we), 
		       .wr_bit(wr_bit_r), 
		       .ie0(int0), 
		       .ie1(int1), 
		       .tr0(tr0),
		       .tr1(tr1), 
		       .t0(t0), 
		       .t1(t1), 
		       .tf0(tf0), 
		       .tf1(tf1), 
		       .pres_ow(pres_ow),
		       .tmod(tmod), 
		       .tl0(tl0), 
		       .th0(th0), 
		       .tl1(tl1), 
		       .th1(th1));
`else
  assign tf0 = 1'b0;
  assign tf1 = 1'b0;
`endif

//
// timer/counter 2
// TH2, TL2, RCAPL2L, RCAPL2H, T2CON
`ifdef OC8051_TC2
  oc8051_tc2 oc8051_tc21(.clk(clk), 
                         .rst(rst), 
			 .wr_addr(adr1),
			 .data_in(dat1), 
			 .wr(we),
			 .wr_bit(wr_bit_r), 
			 .bit_in(bit_in), 
			 .t2(t2), 
			 .t2ex(t2ex),
			 .rclk(rclk), 
			 .tclk(tclk), 
			 .brate2(brate2), 
			 .tc2_int(tc2_int), 
			 .pres_ow(pres_ow),
			 .t2con(t2con), 
			 .tl2(tl2), 
			 .th2(th2), 
			 .rcap2l(rcap2l), 
			 .rcap2h(rcap2h));
`else
  assign tc2_int = 1'b0;
  assign rclk    = 1'b0;
  assign tclk    = 1'b0;
  assign brate2  = 1'b0;
`endif



always @(posedge clk or posedge rst)
  if (rst) begin
    adr0_r <= #1 8'h00;
    ram_wr_sel_r <= #1 3'b000;
    wr_bit_r <= #1 1'b0;
//    wait_data <= #1 1'b0;
  end else begin
    adr0_r <= #1 adr0;
    ram_wr_sel_r <= #1 ram_wr_sel;
    wr_bit_r <= #1 wr_bit;
  end

assign comp_wait = !(
                    ((comp_sel==`OC8051_CSS_AZ) &
		       ((wr_sfr==`OC8051_WRS_ACC1) |
		        (wr_sfr==`OC8051_WRS_ACC2) |
			((adr1==`OC8051_SFR_ACC) & we & !wr_bit_r) |
			((adr1[7:3]==`OC8051_SFR_B_ACC) & we & wr_bit_r))) |
		    ((comp_sel==`OC8051_CSS_CY) &
		       ((|psw_set) |
			((adr1==`OC8051_SFR_PSW) & we & !wr_bit_r) |
			((adr1[7:3]==`OC8051_SFR_B_PSW) & we & wr_bit_r))) |
		    ((comp_sel==`OC8051_CSS_BIT) &
		       ((adr1[7:3]==adr0[7:3]) & (~&adr1[2:0]) &  we & !wr_bit_r) |
		       ((adr1==adr0) & adr1[7] & we & !wr_bit_r)));



//
//set output in case of address (byte)
always @(posedge clk or posedge rst)
begin
  if (rst) begin
    dat0 <= #1 8'h00;
    wait_data <= #1 1'b0;
  end else if ((wr_sfr==`OC8051_WRS_DPTR) & (adr0==`OC8051_SFR_DPTR_LO)) begin				//write and read same address
    dat0 <= #1 des_acc;
    wait_data <= #1 1'b0;
  end else if (
      (
        ((wr_sfr==`OC8051_WRS_ACC1) & (adr0==`OC8051_SFR_ACC)) | 	//write to acc
//        ((wr_sfr==`OC8051_WRS_DPTR) & (adr0==`OC8051_SFR_DPTR_LO)) |	//write to dpl
        (adr1[7] & (adr1==adr0) & we & !wr_bit_r) |			//write and read same address
        (adr1[7] & (adr1[7:3]==adr0[7:3]) & (~&adr0[2:0]) &  we & wr_bit_r) //write bit addressable to read address
      ) & !wait_data) begin
    wait_data <= #1 1'b1;

  end else if ((
      ((|psw_set) & (adr0==`OC8051_SFR_PSW)) |
      ((wr_sfr==`OC8051_WRS_ACC2) & (adr0==`OC8051_SFR_ACC)) | 	//write to acc
      ((wr_sfr==`OC8051_WRS_DPTR) & (adr0==`OC8051_SFR_DPTR_HI))	//write to dph
      ) & !wait_data) begin
    wait_data <= #1 1'b1;

  end else begin
    case (adr0) /* synopsys full_case parallel_case */
      `OC8051_SFR_ACC: 		dat0 <= #1 acc;
      `OC8051_SFR_PSW: 		dat0 <= #1 psw;

`ifdef OC8051_PORTS
  `ifdef OC8051_PORT0
      `OC8051_SFR_P0: 		dat0 <= #1 p0_data;
  `endif

  `ifdef OC8051_PORT1
      `OC8051_SFR_P1: 		dat0 <= #1 p1_data;
  `endif

  `ifdef OC8051_PORT2
      `OC8051_SFR_P2: 		dat0 <= #1 p2_data;
  `endif

  `ifdef OC8051_PORT3
      `OC8051_SFR_P3: 		dat0 <= #1 p3_data;
  `endif
`endif

      `OC8051_SFR_SP: 		dat0 <= #1 sp;
      `OC8051_SFR_B: 		dat0 <= #1 b_reg;
      `OC8051_SFR_DPTR_HI: 	dat0 <= #1 dptr_hi;
      `OC8051_SFR_DPTR_LO: 	dat0 <= #1 dptr_lo;

`ifdef OC8051_UART
      `OC8051_SFR_SCON: 	dat0 <= #1 scon;
      `OC8051_SFR_SBUF: 	dat0 <= #1 sbuf;
      `OC8051_SFR_PCON: 	dat0 <= #1 pcon;
`endif

`ifdef OC8051_TC01
      `OC8051_SFR_TH0: 		dat0 <= #1 th0;
      `OC8051_SFR_TH1: 		dat0 <= #1 th1;
      `OC8051_SFR_TL0: 		dat0 <= #1 tl0;
      `OC8051_SFR_TL1: 		dat0 <= #1 tl1;
      `OC8051_SFR_TMOD: 	dat0 <= #1 tmod;
`endif

      `OC8051_SFR_IP: 		dat0 <= #1 ip;
      `OC8051_SFR_IE: 		dat0 <= #1 ie;
      `OC8051_SFR_TCON: 	dat0 <= #1 tcon;

`ifdef OC8051_TC2
      `OC8051_SFR_RCAP2H: 	dat0 <= #1 rcap2h;
      `OC8051_SFR_RCAP2L: 	dat0 <= #1 rcap2l;
      `OC8051_SFR_TH2:    	dat0 <= #1 th2;
      `OC8051_SFR_TL2:    	dat0 <= #1 tl2;
      `OC8051_SFR_T2CON:  	dat0 <= #1 t2con;
`endif

//      default: 			dat0 <= #1 8'h00;
    endcase
    wait_data <= #1 1'b0;
  end
end


//
//set output in case of address (bit)

always @(posedge clk or posedge rst)
begin
  if (rst)
    bit_out <= #1 1'h0;
  else if (
          ((adr1[7:3]==adr0[7:3]) & (~&adr1[2:0]) &  we & !wr_bit_r) |
          ((wr_sfr==`OC8051_WRS_ACC1) & (adr0[7:3]==`OC8051_SFR_B_ACC)) 	//write to acc
	  )

    bit_out <= #1 dat1[adr0[2:0]];
  else if ((adr1==adr0) & we & wr_bit_r)
    bit_out <= #1 bit_in;
  else
    case (adr0[7:3]) /* synopsys full_case parallel_case */
      `OC8051_SFR_B_ACC:   bit_out <= #1 acc[adr0[2:0]];
      `OC8051_SFR_B_PSW:   bit_out <= #1 psw[adr0[2:0]];

`ifdef OC8051_PORTS
  `ifdef OC8051_PORT0
      `OC8051_SFR_B_P0:    bit_out <= #1 p0_data[adr0[2:0]];
  `endif

  `ifdef OC8051_PORT1
      `OC8051_SFR_B_P1:    bit_out <= #1 p1_data[adr0[2:0]];
  `endif

  `ifdef OC8051_PORT2
      `OC8051_SFR_B_P2:    bit_out <= #1 p2_data[adr0[2:0]];
  `endif

  `ifdef OC8051_PORT3
      `OC8051_SFR_B_P3:    bit_out <= #1 p3_data[adr0[2:0]];
  `endif
`endif

      `OC8051_SFR_B_B:     bit_out <= #1 b_reg[adr0[2:0]];
      `OC8051_SFR_B_IP:    bit_out <= #1 ip[adr0[2:0]];
      `OC8051_SFR_B_IE:    bit_out <= #1 ie[adr0[2:0]];
      `OC8051_SFR_B_TCON:  bit_out <= #1 tcon[adr0[2:0]];

`ifdef OC8051_UART
      `OC8051_SFR_B_SCON:  bit_out <= #1 scon[adr0[2:0]];
`endif

`ifdef OC8051_TC2
      `OC8051_SFR_B_T2CON: bit_out <= #1 t2con[adr0[2:0]];
`endif

//      default:             bit_out <= #1 1'b0;
    endcase
end

always @(posedge clk or posedge rst)
begin
  if (rst) begin
    prescaler <= #1 4'h0;
    pres_ow <= #1 1'b0;
  end else if (prescaler==4'b1011) begin
    prescaler <= #1 4'h0;
    pres_ow <= #1 1'b1;
  end else begin
    prescaler <= #1 prescaler + 4'h1;
    pres_ow <= #1 1'b0;
  end
end

endmodule
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 stack pointer                                          ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   8051 special function register: stack pointer.             ////
////                                                              ////
////  To Do:                                                      ////
////   nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.5  2003/01/13 14:14:41  simont
// replace some modules
//
// Revision 1.4  2002/11/05 17:23:54  simont
// add module oc8051_sfr, 256 bytes internal ram
//
// Revision 1.3  2002/09/30 17:33:59  simont
// prepared header
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"



module oc8051_sp (clk, rst, ram_rd_sel, ram_wr_sel, wr_addr, wr, wr_bit, data_in, sp_out, sp_w);


input clk, rst, wr, wr_bit;
input [2:0] ram_rd_sel, ram_wr_sel;
input [7:0] data_in, wr_addr;
output [7:0] sp_out, sp_w;

reg [7:0] sp_out, sp_w;
reg pop;
wire write;
wire [7:0] sp_t;

reg [7:0] sp;


assign write = ((wr_addr==`OC8051_SFR_SP) & (wr) & !(wr_bit));

assign sp_t= write ? data_in : sp;


always @(posedge clk or posedge rst)
begin
  if (rst)
    sp <= #1 `OC8051_RST_SP;
  else if (write)
    sp <= #1 data_in;
  else
    sp <= #1 sp_out;
end


always @(sp or ram_wr_sel)
begin
//
// push
  if (ram_wr_sel==`OC8051_RWS_SP) sp_w = sp + 8'h01;
  else sp_w = sp;

end


always @(sp_t or ram_wr_sel or pop or write)
begin
//
// push
  if (write) sp_out = sp_t;
  else if (ram_wr_sel==`OC8051_RWS_SP) sp_out = sp_t + 8'h01;
  else sp_out = sp_t - {7'b0, pop};

end


always @(posedge clk or posedge rst)
begin
  if (rst)
    pop <= #1 1'b0;
  else if (ram_rd_sel==`OC8051_RRS_SP) pop <= #1 1'b1;
  else pop <= #1 1'b0;
end

endmodule
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 cores timer/counter2 control                           ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   timers and counters 2 8051 core                            ////
////                                                              ////
////  To Do:                                                      ////
////   Nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.2  2003/04/04 10:34:13  simont
// change timers to meet timing specifications (add divider with 12)
//
// Revision 1.1  2003/01/13 14:13:12  simont
// initial import
//
//
//

// `include "oc8051_defines.v"

//synopsys translate_off
// `include "oc8051_timescale.v"
//synopsys translate_on



module oc8051_tc2 (clk, rst,
            wr_addr,
	    data_in, bit_in,
	    wr, wr_bit,
	    t2, t2ex,
            rclk, tclk,
	    brate2, tc2_int,
	    pres_ow,
//registers
	    t2con, tl2, th2, rcap2l, rcap2h);

input [7:0]  wr_addr,
             data_in;
input        clk,
             rst,
	     wr,
	     wr_bit,
	     t2,
	     t2ex,
	     bit_in,
	     pres_ow;	//prescalre owerflov
output [7:0] t2con,
             tl2,
	     th2,
	     rcap2l,
	     rcap2h;
output       tc2_int,
	     rclk,
	     tclk,
	     brate2;


reg brate2;
reg [7:0] t2con, tl2, th2, rcap2l, rcap2h;

reg neg_trans, t2ex_r, t2_r, tc2_event, tf2_set;

wire run;

//
// t2con
wire tf2, exf2, exen2, tr2, ct2, cprl2;

assign tc2_int = tf2 | exf2;
assign tf2   = t2con[7];
assign exf2  = t2con[6];
assign rclk  = t2con[5];
assign tclk  = t2con[4];
assign exen2 = t2con[3];
assign tr2   = t2con[2];
assign ct2   = t2con[1];
assign cprl2 = t2con[0];

always @(posedge clk or posedge rst)
begin
  if (rst) begin
    t2con <= #1 `OC8051_RST_T2CON;
  end else if ((wr) & !(wr_bit) & (wr_addr==`OC8051_SFR_T2CON)) begin
    t2con <= #1 data_in;
  end else if ((wr) & (wr_bit) & (wr_addr[7:3]==`OC8051_SFR_B_T2CON)) begin
    t2con[wr_addr[2:0]] <= #1 bit_in;
  end else if (tf2_set) begin
    t2con[7] <= #1 1'b1;
  end else if (exen2 & neg_trans) begin
    t2con[6] <= #1 1'b1;
  end
end


//
//th2, tl2
assign run = tr2 & ((!ct2 & pres_ow) | (ct2 & tc2_event));

always @(posedge clk or posedge rst)
begin
  if (rst) begin
//
// reset
//
    tl2 <= #1 `OC8051_RST_TL2;
    th2 <= #1 `OC8051_RST_TH2;
    brate2 <= #1 1'b0;
    tf2_set <= #1 1'b0;
  end else if ((wr) & !(wr_bit) & (wr_addr==`OC8051_SFR_TH2)) begin
//
// write to timer 2 high
//
    th2 <= #1 data_in;
  end else if ((wr) & !(wr_bit) & (wr_addr==`OC8051_SFR_TL2)) begin
//
// write to timer 2 low
//
    tl2 <= #1 data_in;
  end else if (!(rclk | tclk) & !cprl2 & exen2 & neg_trans) begin
//
// avto reload mode, exen2=1, 0-1 transition on t2ex pin
//
    th2 <= #1 rcap2h;
    tl2 <= #1 rcap2l;
    tf2_set <= #1 1'b0;
  end else if (run) begin
    if (rclk | tclk) begin
//
// boud rate generator mode
//
      if (&{th2, tl2}) begin
        th2 <= #1 rcap2h;
        tl2 <= #1 rcap2l;
        brate2 <= #1 1'b1;
      end else begin
        {brate2, th2, tl2}  <= #1 {1'b0, th2, tl2} + 17'h1;
      end
      tf2_set <= #1 1'b0;
    end else if (cprl2) begin
//
// capture mode
//
      {tf2_set, th2, tl2}  <= #1 {1'b0, th2, tl2} + 17'h1;
    end else begin
//
// auto reload mode
//
      if (&{th2, tl2}) begin
        th2 <= #1 rcap2h;
        tl2 <= #1 rcap2l;
        tf2_set <= #1 1'b1;
      end else begin
        {tf2_set, th2, tl2} <= #1 {1'b0, th2, tl2} + 17'h1;
      end
    end
  end else tf2_set <= #1 1'b0;
end


//
// rcap2l, rcap2h
always @(posedge clk or posedge rst)
begin
  if (rst) begin
    rcap2l <= #1 `OC8051_RST_RCAP2L;
    rcap2h <= #1 `OC8051_RST_RCAP2H;
  end else if ((wr) & !(wr_bit) & (wr_addr==`OC8051_SFR_RCAP2H)) begin
    rcap2h <= #1 data_in;
  end else if ((wr) & !(wr_bit) & (wr_addr==`OC8051_SFR_RCAP2L)) begin
    rcap2l <= #1 data_in;
  end else if (!(rclk | tclk) & exen2 & cprl2 & neg_trans) begin
    rcap2l <= #1 tl2;
    rcap2h <= #1 th2;
  end
end


//
//
always @(posedge clk or posedge rst)
begin
  if (rst) begin
    neg_trans <= #1 1'b0;
    t2ex_r <= #1 1'b0;
  end else if (t2ex) begin
    neg_trans <= #1 1'b0;
    t2ex_r <= #1 1'b1;
  end else if (t2ex_r) begin
    neg_trans <= #1 1'b1;
    t2ex_r <= #1 1'b0;
  end else begin
    neg_trans <= #1 1'b0;
    t2ex_r <= #1 t2ex_r;
  end
end

//
//
always @(posedge clk or posedge rst)
begin
  if (rst) begin
    tc2_event <= #1 1'b0;
    t2_r <= #1 1'b0;
  end else if (t2) begin
    tc2_event <= #1 1'b0;
    t2_r <= #1 1'b1;
  end else if (!t2 & t2_r) begin
    tc2_event <= #1 1'b1;
    t2_r <= #1 1'b0;
  end else begin
    tc2_event <= #1 1'b0;
  end
end

endmodule
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 cores timer/counter control                            ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   timers and counters handling for 8051 core                 ////
////                                                              ////
////  To Do:                                                      ////
////   Nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.8  2003/04/10 12:43:19  simont
// defines for pherypherals added
//
// Revision 1.7  2003/04/07 14:58:02  simont
// change sfr's interface.
//
// Revision 1.6  2003/04/04 10:34:13  simont
// change timers to meet timing specifications (add divider with 12)
//
// Revision 1.5  2003/01/13 14:14:41  simont
// replace some modules
//
// Revision 1.4  2002/09/30 17:33:59  simont
// prepared header
//
//

// `include "oc8051_defines.v"

//synopsys translate_off
// `include "oc8051_timescale.v"
//synopsys translate_on



module oc8051_tc (clk, rst, 
            data_in,
            wr_addr,
	    wr, wr_bit,
	    ie0, ie1,
	    tr0, tr1,
	    t0, t1,
            tf0, tf1,
	    pres_ow,
//registers
	    tmod, tl0, th0, tl1, th1);

input [7:0]  wr_addr,
             data_in;
input        clk,
             rst,
	     wr,
	     wr_bit,
	     ie0,
	     ie1,
	     tr0,
	     tr1,
	     t0,
	     t1,
	     pres_ow;
output [7:0] tmod,
             tl0,
	     th0,
	     tl1,
	     th1;
output       tf0,
             tf1;


reg [7:0] tmod, tl0, th0, tl1, th1;
reg tf0, tf1_0, tf1_1, t0_buff, t1_buff;

wire tc0_add, tc1_add;

assign tc0_add = (tr0 & (!tmod[3] | !ie0) & ((!tmod[2] & pres_ow) | (tmod[2] & !t0 & t0_buff)));
assign tc1_add = (tr1 & (!tmod[7] | !ie1) & ((!tmod[6] & pres_ow) | (tmod[6] & !t1 & t1_buff)));
assign tf1= tf1_0 | tf1_1;

//
// read or write from one of the addresses in tmod
//
always @(posedge clk or posedge rst)
begin
 if (rst) begin
   tmod <=#1 `OC8051_RST_TMOD;
 end else if ((wr) & !(wr_bit) & (wr_addr==`OC8051_SFR_TMOD))
    tmod <= #1 data_in;
end

//
// TIMER COUNTER 0
//
always @(posedge clk or posedge rst)
begin
 if (rst) begin
   tl0 <=#1 `OC8051_RST_TL0;
   th0 <=#1 `OC8051_RST_TH0;
   tf0 <= #1 1'b0;
   tf1_0 <= #1 1'b0;
 end else if ((wr) & !(wr_bit) & (wr_addr==`OC8051_SFR_TL0)) begin
   tl0 <= #1 data_in;
   tf0 <= #1 1'b0;
   tf1_0 <= #1 1'b0;
 end else if ((wr) & !(wr_bit) & (wr_addr==`OC8051_SFR_TH0)) begin
   th0 <= #1 data_in;
   tf0 <= #1 1'b0;
   tf1_0 <= #1 1'b0;
 end else begin
     case (tmod[1:0]) /* synopsys full_case parallel_case */
      `OC8051_MODE0: begin                       // mode 0
        tf1_0 <= #1 1'b0;
        if (tc0_add)
          {tf0, th0,tl0[4:0]} <= #1 {1'b0, th0, tl0[4:0]}+ 1'b1;
      end
      `OC8051_MODE1: begin                       // mode 1
        tf1_0 <= #1 1'b0;
        if (tc0_add)
          {tf0, th0,tl0} <= #1 {1'b0, th0, tl0}+ 1'b1;
      end

      `OC8051_MODE2: begin                       // mode 2
        tf1_0 <= #1 1'b0;
        if (tc0_add) begin
	  if (tl0 == 8'b1111_1111) begin
            tf0 <=#1 1'b1;
            tl0 <=#1 th0;
           end
          else begin
            tl0 <=#1 tl0 + 8'h1;
            tf0 <= #1 1'b0;
          end
	end
      end
      `OC8051_MODE3: begin                       // mode 3

	 if (tc0_add)
	   {tf0, tl0} <= #1 {1'b0, tl0} +1'b1;

         if (tr1 & pres_ow)
	   {tf1_0, th0} <= #1 {1'b0, th0} +1'b1;

      end
/*      default:begin
        tf0 <= #1 1'b0;
        tf1_0 <= #1 1'b0;
      end*/
    endcase
 end
end

//
// TIMER COUNTER 1
//
always @(posedge clk or posedge rst)
begin
 if (rst) begin
   tl1 <=#1 `OC8051_RST_TL1;
   th1 <=#1 `OC8051_RST_TH1;
   tf1_1 <= #1 1'b0;
 end else if ((wr) & !(wr_bit) & (wr_addr==`OC8051_SFR_TL1)) begin
   tl1 <= #1 data_in;
   tf1_1 <= #1 1'b0;
 end else if ((wr) & !(wr_bit) & (wr_addr==`OC8051_SFR_TH1)) begin
   th1 <= #1 data_in;
   tf1_1 <= #1 1'b0;
 end else begin
     case (tmod[5:4]) /* synopsys full_case parallel_case */
      `OC8051_MODE0: begin                       // mode 0
        if (tc1_add)
          {tf1_1, th1,tl1[4:0]} <= #1 {1'b0, th1, tl1[4:0]}+ 1'b1;
      end
      `OC8051_MODE1: begin                       // mode 1
        if (tc1_add)
          {tf1_1, th1,tl1} <= #1 {1'b0, th1, tl1}+ 1'b1;
      end

      `OC8051_MODE2: begin                       // mode 2
        if (tc1_add) begin
	  if (tl1 == 8'b1111_1111) begin
            tf1_1 <=#1 1'b1;
            tl1 <=#1 th1;
           end
          else begin
            tl1 <=#1 tl1 + 8'h1;
            tf1_1 <= #1 1'b0;
          end
	end
      end
/*      default:begin
        tf1_1 <= #1 1'b0;
      end*/
    endcase
 end
end


always @(posedge clk or posedge rst)
  if (rst) begin
    t0_buff <= #1 1'b0;
    t1_buff <= #1 1'b0;
  end else begin
    t0_buff <= #1 t0;
    t1_buff <= #1 t1;
  end
endmodule
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 cores serial interface                                 ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////   uart for 8051 core                                         ////
////                                                              ////
////  To Do:                                                      ////
////   Nothing                                                    ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.14  2003/04/29 11:25:42  simont
// prepared start of receiving if ren is not active.
//
// Revision 1.13  2003/04/10 08:57:16  simont
// remove signal sbuf_txd [12:11]
//
// Revision 1.12  2003/04/07 14:58:02  simont
// change sfr's interface.
//
// Revision 1.11  2003/04/07 13:29:16  simont
// change uart to meet timing.
//
// Revision 1.10  2003/01/13 14:14:41  simont
// replace some modules
//
// Revision 1.9  2002/09/30 17:33:59  simont
// prepared header
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"

module oc8051_uart (rst, clk, 
             bit_in, data_in,
	     wr_addr,
	     wr, wr_bit,
             rxd, txd,
	     intr,
             brate2, t1_ow, pres_ow,
	     rclk, tclk,
//registers
	     scon, pcon, sbuf);

input        rst,
             clk,
	     bit_in,
	     wr,
	     rxd,
	     wr_bit,
	     t1_ow,
	     brate2,
	     pres_ow,
	     rclk,
	     tclk;
input [7:0]  data_in,
	     wr_addr;

output       txd,
             intr;
output [7:0] scon,
             pcon,
	     sbuf;


reg t1_ow_buf;
//
reg [7:0] scon, pcon;


reg        txd,
           trans,
           receive,
           tx_done,
	   rx_done,
	   rxd_r,
	   shift_tr,
	   shift_re;
reg [1:0]  rx_sam;
reg [3:0]  tr_count,
           re_count;
reg [7:0]  sbuf_rxd;
reg [11:0] sbuf_rxd_tmp;
reg [10:0] sbuf_txd;

assign sbuf = sbuf_rxd;
assign intr = scon[1] | scon [0];

//
//serial port control register
//
wire ren, tb8, rb8, ri;
assign ren = scon[4];
assign tb8 = scon[3];
assign rb8 = scon[2];
assign ri  = scon[0];

always @(posedge clk or posedge rst)
begin
  if (rst)
    scon <= #1 `OC8051_RST_SCON;
  else if ((wr) & !(wr_bit) & (wr_addr==`OC8051_SFR_SCON))
    scon <= #1 data_in;
  else if ((wr) & (wr_bit) & (wr_addr[7:3]==`OC8051_SFR_B_SCON))
    scon[wr_addr[2:0]] <= #1 bit_in;
  else if (tx_done)
    scon[1] <= #1 1'b1;
  else if (!rx_done) begin
    if (scon[7:6]==2'b00) begin
      scon[0] <= #1 1'b1;
    end else if ((sbuf_rxd_tmp[11]) | !(scon[5])) begin
      scon[0] <= #1 1'b1;
      scon[2] <= #1 sbuf_rxd_tmp[11];
    end else
      scon[2] <= #1 sbuf_rxd_tmp[11];
  end
end

//
//power control register
//
wire smod;
assign smod = pcon[7];
always @(posedge clk or posedge rst)
begin
  if (rst)
  begin
    pcon <= #1 `OC8051_RST_PCON;
  end else if ((wr_addr==`OC8051_SFR_PCON) & (wr) & !(wr_bit))
    pcon <= #1 data_in;
end


//
//serial port buffer (transmit)
//

wire wr_sbuf;
assign wr_sbuf = (wr_addr==`OC8051_SFR_SBUF) & (wr) & !(wr_bit);

always @(posedge clk or posedge rst)
begin
  if (rst) begin
    txd      <= #1 1'b1;
    tr_count <= #1 4'd0;
    trans    <= #1 1'b0;
    sbuf_txd <= #1 11'h00;
    tx_done  <= #1 1'b0;
//
// start transmiting
//
  end else if (wr_sbuf) begin
    case (scon[7:6]) /* synopsys parallel_case */
      2'b00: begin  // mode 0
        sbuf_txd <= #1 {3'b001, data_in};
      end
      2'b01: begin // mode 1
        sbuf_txd <= #1 {2'b01, data_in, 1'b0};
      end
      default: begin  // mode 2 and mode 3
        sbuf_txd <= #1 {1'b1, tb8, data_in, 1'b0};
      end
    endcase
    trans    <= #1 1'b1;
    tr_count <= #1 4'd0;
    tx_done  <= #1 1'b0;
//
// transmiting
//
  end else if (trans & (scon[7:6] == 2'b00) & pres_ow) // mode 0
  begin
    if (~|sbuf_txd[10:1]) begin
      trans   <= #1 1'b0;
      tx_done <= #1 1'b1;
    end else begin
      {sbuf_txd, txd} <= #1 {1'b0, sbuf_txd};
      tx_done         <= #1 1'b0;
    end
  end else if (trans & (scon[7:6] != 2'b00) & shift_tr) begin // mode 1, 2, 3
    tr_count <= #1 tr_count + 4'd1;
    if (~|tr_count) begin
      if (~|sbuf_txd[10:0]) begin
        trans   <= #1 1'b0;
        tx_done <= #1 1'b1;
        txd <= #1 1'b1;
      end else begin
        {sbuf_txd, txd} <= #1 {1'b0, sbuf_txd};
        tx_done         <= #1 1'b0;
      end
    end
  end else if (!trans) begin
    txd     <= #1 1'b1;
    tx_done <= #1 1'b0;
  end
end

//
//
reg sc_clk_tr, smod_clk_tr;
always @(brate2 or t1_ow or t1_ow_buf or scon[7:6] or tclk)
begin
  if (scon[7:6]==8'b10) begin //mode 2
    sc_clk_tr = 1'b1;
  end else if (tclk) begin //
    sc_clk_tr = brate2;
  end else begin //
    sc_clk_tr = !t1_ow_buf & t1_ow;
  end
end

always @(posedge clk or posedge rst)
begin
  if (rst) begin
    smod_clk_tr <= #1 1'b0;
    shift_tr    <= #1 1'b0;
  end else if (sc_clk_tr) begin
    if (smod) begin
      shift_tr <= #1 1'b1;
    end else begin
      shift_tr    <= #1  smod_clk_tr;
      smod_clk_tr <= #1 !smod_clk_tr;
    end
  end else begin
    shift_tr <= #1 1'b0;
  end
end


//
//serial port buffer (receive)
//
always @(posedge clk or posedge rst)
begin
  if (rst) begin
    re_count     <= #1 4'd0;
    receive      <= #1 1'b0;
    sbuf_rxd     <= #1 8'h00;
    sbuf_rxd_tmp <= #1 12'd0;
    rx_done      <= #1 1'b1;
    rxd_r        <= #1 1'b1;
    rx_sam       <= #1 2'b00;
  end else if (!rx_done) begin
    receive <= #1 1'b0;
    rx_done <= #1 1'b1;
    sbuf_rxd <= #1 sbuf_rxd_tmp[10:3];
  end else if (receive & (scon[7:6]==2'b00) & pres_ow) begin //mode 0
    {sbuf_rxd_tmp, rx_done} <= #1 {rxd, sbuf_rxd_tmp};
  end else if (receive & (scon[7:6]!=2'b00) & shift_re) begin //mode 1, 2, 3
    re_count <= #1 re_count + 4'd1;
    case (re_count) /* synopsys full_case parallel_case */
      4'h7: rx_sam[0] <= #1 rxd;
      4'h8: rx_sam[1] <= #1 rxd;
      4'h9: begin
        {sbuf_rxd_tmp, rx_done} <= #1 {(rxd==rx_sam[0] ? rxd : rx_sam[1]), sbuf_rxd_tmp};
      end
    endcase
//
//start receiving
//
  end else if (scon[7:6]==2'b00) begin //start mode 0
    rx_done <= #1 1'b1;
    if (ren && !ri && !receive) begin
      receive      <= #1 1'b1;
      sbuf_rxd_tmp <= #1 10'h0ff;
    end
  end else if (ren & shift_re) begin
    rxd_r <= #1 rxd;
    rx_done <= #1 1'b1;
    re_count <= #1 4'h0;
    receive <= #1 (rxd_r & !rxd);
    sbuf_rxd_tmp <= #1 10'h1ff;
  end else if (!ren) begin
    rxd_r <= #1 rxd;
  end else
    rx_done <= #1 1'b1;
end

//
//
reg sc_clk_re, smod_clk_re;
always @(brate2 or t1_ow or t1_ow_buf or scon[7:6] or rclk)
begin
  if (scon[7:6]==8'b10) begin //mode 2
    sc_clk_re = 1'b1;
  end else if (rclk) begin //
    sc_clk_re = brate2;
  end else begin //
    sc_clk_re = !t1_ow_buf & t1_ow;
  end
end

always @(posedge clk or posedge rst)
begin
  if (rst) begin
    smod_clk_re <= #1 1'b0;
    shift_re    <= #1 1'b0;
  end else if (sc_clk_re) begin
    if (smod) begin
      shift_re <= #1 1'b1;
    end else begin
      shift_re    <= #1  smod_clk_re;
      smod_clk_re <= #1 !smod_clk_re;
    end
  end else begin
    shift_re <= #1 1'b0;
  end
end



//
//
//

always @(posedge clk or posedge rst)
begin
  if (rst) begin
    t1_ow_buf <= #1 1'b0;
  end else begin
    t1_ow_buf <= #1 t1_ow;
  end
end



endmodule

//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 wishbone interface to instruction rom                  ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////                                                              ////
////                                                              ////
////  To Do:                                                      ////
////    nothing                                                   ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.5  2003/05/05 10:34:27  simont
// registering outputs.
//
// Revision 1.4  2003/04/16 10:02:45  simont
// fix bug (cyc_o and stb_o)
//
// Revision 1.3  2003/04/03 19:19:02  simont
// change adr_i and adr_o length.
//
// Revision 1.2  2003/01/13 14:14:41  simont
// replace some modules
//
// Revision 1.1  2002/10/28 16:42:08  simont
// initial import
//
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on


module oc8051_wb_iinterface(rst, clk, 
                  adr_i, dat_o, cyc_i, stb_i, ack_o,
		  adr_o, dat_i, cyc_o, stb_o, ack_i
		  );
//
// rst           (in)  reset - pin
// clk           (in)  clock - pini
input rst, clk;

//
// interface to oc8051 cpu
//
// adr_i    (in)  address
// dat_o    (out) data output
// stb_i    (in)  strobe
// ack_o    (out) acknowledge
// cyc_i    (in)  cycle
input         stb_i,
              cyc_i;
input  [15:0] adr_i;
output        ack_o;
output [31:0] dat_o;

//
// interface to instruction rom
//
// adr_o    (out) address
// dat_i    (in)  data input
// stb_o    (out) strobe
// ack_i    (in) acknowledge
// cyc_o    (out)  cycle
input         ack_i;
input  [31:0] dat_i;
output        stb_o,
              cyc_o;
output [15:0] adr_o;

//
// internal bufers and wires
//
reg [15:0] adr_o;
reg        stb_o;

assign ack_o = ack_i;
assign dat_o = dat_i;
//assign stb_o = stb_i || ack_i;
assign cyc_o = stb_o;
//assign adr_o = ack_i ? adr : adr_i;

always @(posedge clk or posedge rst)
  if (rst) begin
    stb_o <= #1 1'b0;
    adr_o <= #1 16'h0000;
  end else if (ack_i) begin
    stb_o <= #1 stb_i;
    adr_o <= #1 adr_i;
  end else if (!stb_o & stb_i) begin
    stb_o <= #1 1'b1;
    adr_o <= #1 adr_i;
  end

endmodule
//////////////////////////////////////////////////////////////////////
////                                                              ////
////  8051 cores top level module                                 ////
////                                                              ////
////  This file is part of the 8051 cores project                 ////
////  http://www.opencores.org/cores/8051/                        ////
////                                                              ////
////  Description                                                 ////
////  8051 definitions.                                           ////
////                                                              ////
////  To Do:                                                      ////
////    nothing                                                   ////
////                                                              ////
////  Author(s):                                                  ////
////      - Simon Teran, simont@opencores.org                     ////
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
// Revision 1.32  2003/06/20 13:36:37  simont
// ram modules added.
//
// Revision 1.31  2003/06/17 14:17:22  simont
// BIST signals added.
//
// Revision 1.30  2003/06/03 16:51:24  simont
// include "8051_defines" added.
//
// Revision 1.29  2003/05/07 12:36:03  simont
// chsnge comp.des to des1
//
// Revision 1.28  2003/05/06 09:41:35  simont
// remove define OC8051_AS2_PCL, chane signal src_sel2 to 2 bit wide.
//
// Revision 1.27  2003/05/05 15:46:37  simont
// add aditional alu destination to solve critical path.
//
// Revision 1.26  2003/04/29 11:24:31  simont
// fix bug in case execution of two data dependent instructions.
//
// Revision 1.25  2003/04/25 17:15:51  simont
// change branch instruction execution (reduse needed clock periods).
//
// Revision 1.24  2003/04/11 10:05:59  simont
// deifne OC8051_ROM added
//
// Revision 1.23  2003/04/10 12:43:19  simont
// defines for pherypherals added
//
// Revision 1.22  2003/04/09 16:24:04  simont
// change wr_sft to 2 bit wire.
//
// Revision 1.21  2003/04/09 15:49:42  simont
// Register oc8051_sfr dato output, add signal wait_data.
//
// Revision 1.20  2003/04/03 19:13:28  simont
// Include instruction cache.
//
// Revision 1.19  2003/04/02 15:08:30  simont
// raname signals.
//
// Revision 1.18  2003/01/13 14:14:41  simont
// replace some modules
//
// Revision 1.17  2002/11/05 17:23:54  simont
// add module oc8051_sfr, 256 bytes internal ram
//
// Revision 1.16  2002/10/28 14:55:00  simont
// fix bug in interface to external data ram
//
// Revision 1.15  2002/10/23 16:53:39  simont
// fix bugs in instruction interface
//
// Revision 1.14  2002/10/17 18:50:00  simont
// cahnge interface to instruction rom
//
// Revision 1.13  2002/09/30 17:33:59  simont
// prepared header
//
//

// synopsys translate_off
// `include "oc8051_timescale.v"
// synopsys translate_on

// `include "oc8051_defines.v"

module oc8051_top (wb_rst_i, wb_clk_i,
//interface to instruction rom
		wbi_adr_o, 
		wbi_dat_i, 
		wbi_stb_o, 
		wbi_ack_i, 
		wbi_cyc_o, 
		wbi_err_i,

//interface to data ram
		wbd_dat_i, 
		wbd_dat_o,
		wbd_adr_o, 
		wbd_we_o, 
		wbd_ack_i,
		wbd_stb_o, 
		wbd_cyc_o, 
		wbd_err_i,

// interrupt interface
		int0_i, 
		int1_i,


// port interface
  `ifdef OC8051_PORTS
	`ifdef OC8051_PORT0
		p0_i,
		p0_o,
	`endif

	`ifdef OC8051_PORT1
		p1_i,
		p1_o,
	`endif

	`ifdef OC8051_PORT2
		p2_i,
		p2_o,
	`endif

	`ifdef OC8051_PORT3
		p3_i,
		p3_o,
	`endif
  `endif

// serial interface
	`ifdef OC8051_UART
		rxd_i, txd_o,
	`endif

// counter interface
	`ifdef OC8051_TC01
		t0_i, t1_i,
	`endif

	`ifdef OC8051_TC2
		t2_i, t2ex_i,
	`endif

// BIST
`ifdef OC8051_BIST
         scanb_rst,
         scanb_clk,
         scanb_si,
         scanb_so,
         scanb_en,
`endif
// external access (active low)
		ea_in
		);



input         wb_rst_i,		// reset input
              wb_clk_i,		// clock input
              int0_i,		// interrupt 0
              int1_i,		// interrupt 1
              ea_in,		// external access
              wbd_ack_i,	// data acknowalge
              wbi_ack_i,	// instruction acknowlage
              wbd_err_i,	// data error
              wbi_err_i;	// instruction error

input [7:0]   wbd_dat_i;	// ram data input
input [31:0]  wbi_dat_i;	// rom data input

output        wbd_we_o,		// data write enable
	      wbd_stb_o,	// data strobe
	      wbd_cyc_o,	// data cycle
	      wbi_stb_o,	// instruction strobe
	      wbi_cyc_o;	// instruction cycle

output [7:0]  wbd_dat_o;	// data output

output [15:0] wbd_adr_o,	// data address
              wbi_adr_o;	// instruction address

`ifdef OC8051_PORTS

`ifdef OC8051_PORT0
input  [7:0]  p0_i;		// port 0 input
output [7:0]  p0_o;		// port 0 output
`endif

`ifdef OC8051_PORT1
input  [7:0]  p1_i;		// port 1 input
output [7:0]  p1_o;		// port 1 output
`endif

`ifdef OC8051_PORT2
input  [7:0]  p2_i;		// port 2 input
output [7:0]  p2_o;		// port 2 output
`endif

`ifdef OC8051_PORT3
input  [7:0]  p3_i;		// port 3 input
output [7:0]  p3_o;		// port 3 output
`endif

`endif






`ifdef OC8051_UART
input         rxd_i;		// receive
output        txd_o;		// transnmit
`endif

`ifdef OC8051_TC01
input         t0_i,		// counter 0 input
              t1_i;		// counter 1 input
`endif

`ifdef OC8051_TC2
input         t2_i,		// counter 2 input
              t2ex_i;		//
`endif

`ifdef OC8051_BIST
input   scanb_rst;
input   scanb_clk;
input   scanb_si;
output  scanb_so;
input   scanb_en;
wire    scanb_soi;
`endif

wire [7:0]  dptr_hi,
	    dptr_lo, 
	    ri, 
	    data_out,
            op1,
            op2,
	    op3,
            acc,
            p0_out,
	    p1_out,
	    p2_out,
	    p3_out,
            sp,
            sp_w;

wire [31:0] idat_onchip;

wire [15:0] pc;

assign wbd_cyc_o = wbd_stb_o;

wire        src_sel3;
wire [1:0]  wr_sfr,
            src_sel2;
wire [2:0]  ram_rd_sel,	// ram read
            ram_wr_sel,	// ram write
            src_sel1;

wire [7:0]  ram_data,
            ram_out,	//data from ram
	    sfr_out,
	    wr_dat,
            wr_addr,	//ram write addres
            rd_addr;	//data ram read addres
wire        sfr_bit;

wire [1:0]  cy_sel,	//carry select; from decoder to cy_selct1
            bank_sel;
wire        rom_addr_sel,	//rom addres select; alu or pc
            rmw,
	    ea_int;

wire        reti,
            intr,
	    int_ack,
	    istb;
wire [7:0]  int_src;

wire        mem_wait;
wire [2:0]  mem_act;
wire [3:0]  alu_op;	//alu operation (from decoder)
wire [1:0]  psw_set;    //write to psw or not; from decoder to psw (through register)

wire [7:0]  src1,	//alu sources 1
            src2,	//alu sources 2
            src3,	//alu sources 3
	    des_acc,
	    des1,	//alu destination 1
	    des2;	//alu destinations 2
wire        desCy,	//carry out
            desAc,
	    desOv,	//overflow
	    alu_cy,
	    wr,		//write to data ram
	    wr_o;

wire        rd,		//read program rom
            pc_wr;
wire [2:0]  pc_wr_sel;	//program counter write select (from decoder to pc)

wire [7:0]  op1_n, //from memory_interface to decoder
            op2_n,
	    op3_n;

wire [1:0]  comp_sel;	//select source1 and source2 to compare
wire        eq,		//result (from comp1 to decoder)
            srcAc,
	    cy,
	    rd_ind,
	    wr_ind,
	    comp_wait;
wire [2:0]  op1_cur;

wire        bit_addr,	//bit addresable instruction
            bit_data,	//bit data from ram to ram_select
	    bit_out,	//bit data from ram_select to alu and cy_select
	    bit_addr_o,
	    wait_data;

//
// cpu to cache/wb_interface
wire        iack_i,
            istb_o,
	    icyc_o;
wire [31:0] idat_i;
wire [15:0] iadr_o;


//
// decoder
oc8051_decoder oc8051_decoder1(.clk(wb_clk_i), 
                               .rst(wb_rst_i), 
			       .op_in(op1_n), 
			       .op1_c(op1_cur),
			       .ram_rd_sel_o(ram_rd_sel), 
			       .ram_wr_sel_o(ram_wr_sel), 
			       .bit_addr(bit_addr),

			       .src_sel1(src_sel1),
			       .src_sel2(src_sel2),
			       .src_sel3(src_sel3),

			       .alu_op_o(alu_op),
			       .psw_set(psw_set),
			       .cy_sel(cy_sel),
			       .wr_o(wr),
			       .pc_wr(pc_wr),
			       .pc_sel(pc_wr_sel),
			       .comp_sel(comp_sel),
			       .eq(eq),
			       .wr_sfr_o(wr_sfr),
			       .rd(rd),
			       .rmw(rmw),
			       .istb(istb),
			       .mem_act(mem_act),
			       .mem_wait(mem_wait),
			       .wait_data(wait_data));


wire [7:0] sub_result;
//
//alu
oc8051_alu oc8051_alu1(.rst(wb_rst_i),
                       .clk(wb_clk_i),
		       .op_code(alu_op),
		       .src1(src1),
		       .src2(src2),
		       .src3(src3),
		       .srcCy(alu_cy),
		       .srcAc(srcAc),
		       .des_acc(des_acc),
		       .sub_result(sub_result),
		       .des1(des1),
		       .des2(des2),
		       .desCy(desCy),
		       .desAc(desAc),
		       .desOv(desOv),
		       .bit_in(bit_out));

//
//data ram
oc8051_ram_top oc8051_ram_top1(.clk(wb_clk_i),
                               .rst(wb_rst_i),
			       .rd_addr(rd_addr),
			       .rd_data(ram_data),
			       .wr_addr(wr_addr),
			       .bit_addr(bit_addr_o),
			       .wr_data(wr_dat),
			       .wr(wr_o && (!wr_addr[7] || wr_ind)),
			       .bit_data_in(desCy),
			       .bit_data_out(bit_data)
`ifdef OC8051_BIST
         ,
         .scanb_rst(scanb_rst),
         .scanb_clk(scanb_clk),
         .scanb_si(scanb_soi),
         .scanb_so(scanb_so),
         .scanb_en(scanb_en)
`endif
			       );

//

oc8051_alu_src_sel oc8051_alu_src_sel1(.clk(wb_clk_i),
                                       .rst(wb_rst_i),
				       .rd(rd),

				       .sel1(src_sel1),
				       .sel2(src_sel2),
				       .sel3(src_sel3),

				       .acc(acc),
				       .ram(ram_out),
				       .pc(pc),
				       .dptr({dptr_hi, dptr_lo}),
				       .op1(op1_n),
				       .op2(op2_n),
				       .op3(op3_n),

				       .src1(src1),
				       .src2(src2),
				       .src3(src3));


//
//
oc8051_comp oc8051_comp1(.sel(comp_sel),
                         .eq(eq),
			 .b_in(bit_out),
			 .cy(cy),
			 .acc(acc),
			 .des(sub_result)
			 );


//
//program rom
`ifdef OC8051_ROM
  oc8051_rom oc8051_rom1(.rst(wb_rst_i),
                       .clk(wb_clk_i),
		       .ea_int(ea_int),
		       .addr(iadr_o),
		       .data_o(idat_onchip)
		       );
`else
  assign ea_int = 1'b0;
  assign idat_onchip = 32'h0;
  
  `ifdef OC8051_SIMULATION

    initial
    begin
      $display("\t * ");
      $display("\t * Internal rom disabled!!!");
      $display("\t * ");
    end

  `endif

`endif

//
//
oc8051_cy_select oc8051_cy_select1(.cy_sel(cy_sel), 
                                   .cy_in(cy), 
				   .data_in(bit_out),
				   .data_out(alu_cy));
//
//
oc8051_indi_addr oc8051_indi_addr1 (.clk(wb_clk_i), 
                                    .rst(wb_rst_i), 
				    .wr_addr(wr_addr),
				    .data_in(wr_dat),
				    .wr(wr_o),
				    .wr_bit(bit_addr_o), 
				    .ri_out(ri),
				    .sel(op1_cur[0]),
				    .bank(bank_sel));



assign icyc_o = istb_o;
//
//
oc8051_memory_interface oc8051_memory_interface1(.clk(wb_clk_i), 
                       .rst(wb_rst_i),
// internal ram
                       .wr_i(wr), 
		       .wr_o(wr_o), 
		       .wr_bit_i(bit_addr), 
		       .wr_bit_o(bit_addr_o), 
		       .wr_dat(wr_dat),
		       .des_acc(des_acc),
		       .des1(des1),
		       .des2(des2),
		       .rd_addr(rd_addr),
		       .wr_addr(wr_addr),
		       .wr_ind(wr_ind),
		       .bit_in(bit_data),
		       .in_ram(ram_data),
		       .sfr(sfr_out),
		       .sfr_bit(sfr_bit),
		       .bit_out(bit_out),
		       .iram_out(ram_out),

// external instrauction rom
                       .iack_i(iack_i),
                       .iadr_o(iadr_o),
                       .idat_i(idat_i),
                       .istb_o(istb_o),

// internal instruction rom
		       .idat_onchip(idat_onchip),

// data memory
                       .dadr_o(wbd_adr_o),
		       .ddat_o(wbd_dat_o),
		       .dwe_o(wbd_we_o),
		       .dstb_o(wbd_stb_o),
		       .ddat_i(wbd_dat_i),
		       .dack_i(wbd_ack_i),

// from decoder
                       .rd_sel(ram_rd_sel),
		       .wr_sel(ram_wr_sel),
		       .rn({bank_sel, op1_cur}),
		       .rd_ind(rd_ind),
		       .rd(rd),
		       .mem_act(mem_act),
		       .mem_wait(mem_wait),

// external access
                       .ea(ea_in),
		       .ea_int(ea_int),

// instructions outputs to cpu
                       .op1_out(op1_n),
		       .op2_out(op2_n),
		       .op3_out(op3_n),

// interrupt interface
                       .intr(intr), 
		       .int_v(int_src), 
		       .int_ack(int_ack), 
		       .istb(istb),
		       .reti(reti),

//pc
                       .pc_wr_sel(pc_wr_sel), 
		       .pc_wr(pc_wr & comp_wait),
		       .pc(pc),

// sfr's
                       .sp_w(sp_w), 
		       .dptr({dptr_hi, dptr_lo}),
		       .ri(ri), 
		       .acc(acc),
		       .sp(sp)
		       );


//
//

oc8051_sfr oc8051_sfr1(.rst(wb_rst_i), 
                       .clk(wb_clk_i), 
		       .adr0(rd_addr[7:0]), 
		       .adr1(wr_addr[7:0]),
		       .dat0(sfr_out),
		       .dat1(wr_dat),
		       .dat2(des2),
		       .des_acc(des_acc),
		       .we(wr_o && !wr_ind),
		       .bit_in(desCy),
		       .bit_out(sfr_bit), 
		       .wr_bit(bit_addr_o),
		       .ram_rd_sel(ram_rd_sel),
		       .ram_wr_sel(ram_wr_sel),
		       .wr_sfr(wr_sfr),
		       .comp_sel(comp_sel),
		       .comp_wait(comp_wait),
// acc
		       .acc(acc),
// sp
		       .sp(sp), 
		       .sp_w(sp_w),
// psw
		       .bank_sel(bank_sel), 
		       .desAc(desAc), 
		       .desOv(desOv), 
		       .psw_set(psw_set),
		       .srcAc(srcAc), 
		       .cy(cy),
// ports
		       .rmw(rmw),

  `ifdef OC8051_PORTS
	`ifdef OC8051_PORT0
		       .p0_out(p0_o),
		       .p0_in(p0_i),
	`endif

	`ifdef OC8051_PORT1
		       .p1_out(p1_o),
		       .p1_in(p1_i),
	`endif

	`ifdef OC8051_PORT2
		       .p2_out(p2_o),
		       .p2_in(p2_i),
	`endif

	`ifdef OC8051_PORT3
		       .p3_out(p3_o),
		       .p3_in(p3_i),
	`endif
  `endif

// uart
	`ifdef OC8051_UART
		       .rxd(rxd_i), .txd(txd_o),
	`endif

// int
		       .int_ack(int_ack),
		       .intr(intr),
		       .int0(int0_i),
		       .int1(int1_i),
		       .reti(reti),
		       .int_src(int_src),

// t/c 0,1
	`ifdef OC8051_TC01
		       .t0(t0_i),
		       .t1(t1_i),
	`endif

// t/c 2
	`ifdef OC8051_TC2
		       .t2(t2_i),
		       .t2ex(t2ex_i),
	`endif

// dptr
		       .dptr_hi(dptr_hi),
		       .dptr_lo(dptr_lo),
		       .wait_data(wait_data)
		       );




`ifdef OC8051_CACHE


  oc8051_icache oc8051_icache1(.rst(wb_rst_i), .clk(wb_clk_i),
  // cpu
        .adr_i(iadr_o),
	.dat_o(idat_i),
	.stb_i(istb_o),
	.ack_o(iack_i),
        .cyc_i(icyc_o),
  // pins
        .dat_i(wbi_dat_i),
	.stb_o(wbi_stb_o),
	.adr_o(wbi_adr_o),
	.ack_i(wbi_ack_i),
        .cyc_o(wbi_cyc_o)
`ifdef OC8051_BIST
         ,
         .scanb_rst(scanb_rst),
         .scanb_clk(scanb_clk),
         .scanb_si(scanb_si),
         .scanb_so(scanb_soi),
         .scanb_en(scanb_en)
`endif
	);

  defparam oc8051_icache1.ADR_WIDTH = 6;  // cache address wihth
  defparam oc8051_icache1.LINE_WIDTH = 2; // line address width (2 => 4x32)
  defparam oc8051_icache1.BL_NUM = 15; // number of blocks (2^BL_WIDTH-1); BL_WIDTH = ADR_WIDTH - LINE_WIDTH
  defparam oc8051_icache1.CACHE_RAM = 64; // cache ram x 32 (2^ADR_WIDTH)

  

  `ifdef OC8051_SIMULATION

    initial
    begin
      #1
      $display("\t * ");
      $display("\t * External rom interface: cache");
      $display("\t * ");
    end

  `endif



//
//    no cache
//
`else

  `ifdef OC8051_BIST
       assign scanb_soi=scanb_si;
  `endif

  `ifdef OC8051_WB

    oc8051_wb_iinterface oc8051_wb_iinterface(.rst(wb_rst_i), .clk(wb_clk_i),
    // cpu
        .adr_i(iadr_o),
	.dat_o(idat_i),
	.stb_i(istb_o),
	.ack_o(iack_i),
        .cyc_i(icyc_o),
    // external rom
        .dat_i(wbi_dat_i),
	.stb_o(wbi_stb_o),
	.adr_o(wbi_adr_o),
	.ack_i(wbi_ack_i),
        .cyc_o(wbi_cyc_o));

  `ifdef OC8051_SIMULATION

    initial
    begin
      #1
      $display("\t * ");
      $display("\t * External rom interface: WB interface");
      $display("\t * ");
    end

  `endif

  `else

    assign wbi_adr_o = iadr_o    ;
    assign idat_i    = wbi_dat_i ;
    assign wbi_stb_o = 1'b1      ;
    assign iack_i    = wbi_ack_i ;
    assign wbi_cyc_o = 1'b1      ;

  `ifdef OC8051_SIMULATION

    initial
    begin
      #1
      $display("\t * ");
      $display("\t * External rom interface: Pipelined interface");
      $display("\t * ");
    end

  `endif


  `endif

`endif



endmodule
