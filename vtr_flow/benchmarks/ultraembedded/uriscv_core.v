//-----------------------------------------------------------------
//                          uRISC-V CPU
//                            V0.5.0
//               github.com/ultraembedded/core_uriscv
//                     Copyright 2015-2021
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2015-2021 github.com/ultraembedded
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//-----------------------------------------------------------------
module uriscv_branch
(
     input  [31:0]  pc_i
    ,input  [31:0]  opcode_i
    ,input  [31:0]  rs1_val_i
    ,input  [31:0]  rs2_val_i
    ,output         branch_o
    ,output [31:0]  branch_target_o
);

//-----------------------------------------------------------------
// Includes
//-----------------------------------------------------------------
//`include "uriscv_defs.v"

//-----------------------------------------------------------------
// less_than_signed: Less than operator (signed)
// Inputs: x = left operand, y = right operand
// Return: (int)x < (int)y
//-----------------------------------------------------------------
function [0:0] less_than_signed;
    input  [31:0] x;
    input  [31:0] y;
    reg [31:0] v;
begin
    v = (x - y);
    if (x[31] != y[31])
        less_than_signed = x[31];
    else
        less_than_signed = v[31];
end
endfunction

//-----------------------------------------------------------------
// greater_than_signed: Greater than operator (signed)
// Inputs: x = left operand, y = right operand
// Return: (int)x > (int)y
//-----------------------------------------------------------------
function [0:0] greater_than_signed;
    input  [31:0] x;
    input  [31:0] y;
    reg [31:0] v;
begin
    v = (y - x);
    if (x[31] != y[31])
        greater_than_signed = y[31];
    else
        greater_than_signed = v[31];
end
endfunction

//-----------------------------------------------------------------
// Branch Decode
//-----------------------------------------------------------------
wire type_branch_w  = (opcode_i[6:2] == 5'b11000);
wire type_jalr_w    = (opcode_i[6:2] == 5'b11001);
wire type_jal_w     = (opcode_i[6:2] == 5'b11011);

wire [2:0] func3_w  = opcode_i[14:12]; // R, I, S
wire [6:0] func7_w  = opcode_i[31:25]; // R

wire branch_beq_w   = (func3_w == 3'b000);
wire branch_bne_w   = (func3_w == 3'b001);
wire branch_blt_w   = (func3_w == 3'b100);
wire branch_bge_w   = (func3_w == 3'b101);
wire branch_bltu_w  = (func3_w == 3'b110);
wire branch_bgeu_w  = (func3_w == 3'b111);

reg         branch_r;
reg [31:0]  branch_target_r;
reg [31:0]  imm12_r;
reg [31:0]  bimm_r;
reg [31:0]  jimm20_r;

always @ *
begin
    branch_r        = 1'b0;
    branch_target_r = 32'b0;

    // Opcode decode
    imm12_r         = {{20{opcode_i[31]}}, opcode_i[31:20]};
    bimm_r          = {{19{opcode_i[31]}}, opcode_i[31],    opcode_i[7],  opcode_i[30:25], opcode_i[11:8],  1'b0};
    jimm20_r        = {{12{opcode_i[31]}}, opcode_i[19:12], opcode_i[20], opcode_i[30:25], opcode_i[24:21], 1'b0};

    // Default branch target is relative to current PC
    branch_target_r = (pc_i + bimm_r);    

    if (type_jal_w)
    begin
        branch_r        = 1'b1;
        branch_target_r = pc_i + jimm20_r;
    end
    else if (type_jalr_w)
    begin
        branch_r            = 1'b1;
        branch_target_r     = rs1_val_i + imm12_r;
        branch_target_r[0]  = 1'b0;
    end
    else if (type_branch_w)
    begin
        case (1'b1)
        branch_beq_w: // beq
            branch_r      = (rs1_val_i == rs2_val_i);

        branch_bne_w: // bne
            branch_r      = (rs1_val_i != rs2_val_i);

        branch_blt_w: // blt
            branch_r      = less_than_signed(rs1_val_i, rs2_val_i);

        branch_bge_w: // bge
            branch_r      = greater_than_signed(rs1_val_i, rs2_val_i) | (rs1_val_i == rs2_val_i);

        branch_bltu_w: // bltu
            branch_r      = (rs1_val_i < rs2_val_i);

        branch_bgeu_w: // bgeu
            branch_r      = (rs1_val_i >= rs2_val_i);

        default:
            ;
        endcase
    end
end

assign branch_o        = branch_r;
assign branch_target_o = branch_target_r;

endmodule
//-----------------------------------------------------------------
//                          uRISC-V CPU
//                            V0.5.0
//               github.com/ultraembedded/core_uriscv
//                     Copyright 2015-2021
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2015-2021 github.com/ultraembedded
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//-----------------------------------------------------------------
//--------------------------------------------------------------------
// ALU Operations
//--------------------------------------------------------------------

`define RV_ALU_NONE                                4'b0000
`define RV_ALU_SHIFTL                              4'b0001
`define RV_ALU_SHIFTR                              4'b0010
`define RV_ALU_SHIFTR_ARITH                        4'b0011
`define RV_ALU_ADD                                 4'b0100
`define RV_ALU_SUB                                 4'b0110
`define RV_ALU_AND                                 4'b0111
`define RV_ALU_OR                                  4'b1000
`define RV_ALU_XOR                                 4'b1001
`define RV_ALU_LESS_THAN                           4'b1010
`define RV_ALU_LESS_THAN_SIGNED                    4'b1011

//-----------------------------------------------------------------
// Privilege levels
//-----------------------------------------------------------------
`define PRIV_USER                       0
`define PRIV_SUPER                      1
`define PRIV_MACHINE                    3

//-----------------------------------------------------------------
// Status Register
//-----------------------------------------------------------------
`define SR_UIE         (1 << 0)
`define SR_UIE_R       0
`define SR_SIE         (1 << 1)
`define SR_SIE_R       1
`define SR_MIE         (1 << 3)
`define SR_MIE_R       3
`define SR_UPIE        (1 << 4)
`define SR_UPIE_R      4
`define SR_SPIE        (1 << 5)
`define SR_SPIE_R      5
`define SR_MPIE        (1 << 7)
`define SR_MPIE_R      7
`define SR_SPP         (1 << 8)
`define SR_SPP_R       8

`define SR_MPP_SHIFT   11
`define SR_MPP_MASK    2'h3
`define SR_MPP_R       12:11
`define SR_MPP_U       `PRIV_USER
`define SR_MPP_S       `PRIV_SUPER
`define SR_MPP_M       `PRIV_MACHINE

`define SR_SUM          (1 << 18)
`define SR_SUM_R        18

//-----------------------------------------------------------------
// IRQ Numbers
//-----------------------------------------------------------------
`define IRQ_S_SOFT   1
`define IRQ_M_SOFT   3
`define IRQ_S_TIMER  5
`define IRQ_M_TIMER  7
`define IRQ_S_EXT    9
`define IRQ_M_EXT    11
`define IRQ_MIN      (`IRQ_S_SOFT)
`define IRQ_MAX      (`IRQ_M_EXT + 1)
`define IRQ_MASK     ((1 << `IRQ_M_EXT)   |                       (1 << `IRQ_M_TIMER) |                       (1 << `IRQ_M_SOFT))

`define SR_IP_MSIP_R      `IRQ_M_SOFT
`define SR_IP_MTIP_R      `IRQ_M_TIMER
`define SR_IP_MEIP_R      `IRQ_M_EXT
`define SR_IP_SSIP_R      `IRQ_S_SOFT
`define SR_IP_STIP_R      `IRQ_S_TIMER
`define SR_IP_SEIP_R      `IRQ_S_EXT

//-----------------------------------------------------------------
// CSR Registers - Machine
//-----------------------------------------------------------------
`define CSR_MSTATUS       12'h300
`define CSR_MSTATUS_MASK  32'hFFFFFFFF
`define CSR_MISA          12'h301
`define CSR_MISA_MASK     32'hFFFFFFFF
    `define MISA_RV32     32'h40000000
    `define MISA_RVI      32'h00000100
    `define MISA_RVE      32'h00000010
    `define MISA_RVM      32'h00001000
    `define MISA_RVA      32'h00000001
    `define MISA_RVF      32'h00000020
    `define MISA_RVD      32'h00000008
    `define MISA_RVC      32'h00000004
    `define MISA_RVS      32'h00040000
    `define MISA_RVU      32'h00100000
`define CSR_MEDELEG       12'h302
`define CSR_MEDELEG_MASK  32'h0000FFFF
`define CSR_MIDELEG       12'h303
`define CSR_MIDELEG_MASK  32'h0000FFFF
`define CSR_MIE           12'h304
`define CSR_MIE_MASK      `IRQ_MASK
`define CSR_MTVEC         12'h305
`define CSR_MTVEC_MASK    32'hFFFFFFFF
`define CSR_MSCRATCH      12'h340
`define CSR_MSCRATCH_MASK 32'hFFFFFFFF
`define CSR_MEPC          12'h341
`define CSR_MEPC_MASK     32'hFFFFFFFF
`define CSR_MCAUSE        12'h342
`define CSR_MCAUSE_MASK   32'h8000000F
`define CSR_MTVAL         12'h343
`define CSR_MTVAL_MASK    32'hFFFFFFFF
`define CSR_MIP           12'h344
`define CSR_MIP_MASK      `IRQ_MASK
`define CSR_MCYCLE        12'hc00
`define CSR_MCYCLE_MASK   32'hFFFFFFFF
`define CSR_MTIME         12'hc01
`define CSR_MTIME_MASK    32'hFFFFFFFF
`define CSR_MTIMEH        12'hc81
`define CSR_MTIMEH_MASK   32'hFFFFFFFF
`define CSR_MHARTID       12'hF14
`define CSR_MHARTID_MASK  32'hFFFFFFFF

// Non-std
`define CSR_MTIMECMP        12'h7c0
`define CSR_MTIMECMP_MASK   32'hFFFFFFFF

//-----------------------------------------------------------------
// CSR Registers - Simulation control
//-----------------------------------------------------------------
`define CSR_DSCRATCH       12'h7b2
`define CSR_DSCRATCH_MASK  32'hFFFFFFFF
`define CSR_SIM_CTRL       12'h8b2
`define CSR_SIM_CTRL_MASK  32'hFFFFFFFF
    `define CSR_SIM_CTRL_EXIT (0 << 24)
    `define CSR_SIM_CTRL_PUTC (1 << 24)

//-----------------------------------------------------------------
// Exception Causes
//-----------------------------------------------------------------
`define MCAUSE_INT                      31
`define MCAUSE_MISALIGNED_FETCH         ((0 << `MCAUSE_INT) | 0)
`define MCAUSE_FAULT_FETCH              ((0 << `MCAUSE_INT) | 1)
`define MCAUSE_ILLEGAL_INSTRUCTION      ((0 << `MCAUSE_INT) | 2)
`define MCAUSE_BREAKPOINT               ((0 << `MCAUSE_INT) | 3)
`define MCAUSE_MISALIGNED_LOAD          ((0 << `MCAUSE_INT) | 4)
`define MCAUSE_FAULT_LOAD               ((0 << `MCAUSE_INT) | 5)
`define MCAUSE_MISALIGNED_STORE         ((0 << `MCAUSE_INT) | 6)
`define MCAUSE_FAULT_STORE              ((0 << `MCAUSE_INT) | 7)
`define MCAUSE_ECALL_U                  ((0 << `MCAUSE_INT) | 8)
`define MCAUSE_ECALL_S                  ((0 << `MCAUSE_INT) | 9)
`define MCAUSE_ECALL_H                  ((0 << `MCAUSE_INT) | 10)
`define MCAUSE_ECALL_M                  ((0 << `MCAUSE_INT) | 11)
`define MCAUSE_PAGE_FAULT_INST          ((0 << `MCAUSE_INT) | 12)
`define MCAUSE_PAGE_FAULT_LOAD          ((0 << `MCAUSE_INT) | 13)
`define MCAUSE_PAGE_FAULT_STORE         ((0 << `MCAUSE_INT) | 15)
`define MCAUSE_INTERRUPT                (1 << `MCAUSE_INT)

//-----------------------------------------------------------------
// Debug defines for exception types
//-----------------------------------------------------------------
`define RV_EXCPN_W                        6
`define RV_EXCPN_MISALIGNED_FETCH         6'h10
`define RV_EXCPN_FAULT_FETCH              6'h11
`define RV_EXCPN_ILLEGAL_INSTRUCTION      6'h12
`define RV_EXCPN_BREAKPOINT               6'h13
`define RV_EXCPN_MISALIGNED_LOAD          6'h14
`define RV_EXCPN_FAULT_LOAD               6'h15
`define RV_EXCPN_MISALIGNED_STORE         6'h16
`define RV_EXCPN_FAULT_STORE              6'h17
`define RV_EXCPN_ECALL                    6'h18
`define RV_EXCPN_ECALL_U                  6'h18
`define RV_EXCPN_ECALL_S                  6'h19
`define RV_EXCPN_ECALL_H                  6'h1a
`define RV_EXCPN_ECALL_M                  6'h1b
`define RV_EXCPN_PAGE_FAULT_INST          6'h1c
`define RV_EXCPN_PAGE_FAULT_LOAD          6'h1d
`define RV_EXCPN_PAGE_FAULT_STORE         6'h1f
`define RV_EXCPN_EXCEPTION                6'h10
`define RV_EXCPN_INTERRUPT                6'h20
`define RV_EXCPN_ERET                     6'h30
`define RV_EXCPN_FENCE                    6'h31
`define RV_EXCPN_TYPE_MASK                6'h30
`define RV_EXCPN_SUBTYPE_R                3:0
//-----------------------------------------------------------------
//                          uRISC-V CPU
//                            V0.5.0
//               github.com/ultraembedded/core_uriscv
//                     Copyright 2015-2021
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2015-2021 github.com/ultraembedded
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//-----------------------------------------------------------------
module uriscv_muldiv
(
    input           clk_i,
    input           rst_i,

    // Operation select
    input           valid_i,
    input           inst_mul_i,
    input           inst_mulh_i,
    input           inst_mulhsu_i,
    input           inst_mulhu_i,
    input           inst_div_i,
    input           inst_divu_i,
    input           inst_rem_i,
    input           inst_remu_i,

    // Operands
    input [31:0]    operand_ra_i,
    input [31:0]    operand_rb_i,

    // Result
    output          stall_o,
    output          ready_o,
    output [31:0]   result_o
);

//-----------------------------------------------------------------
// Includes
//-----------------------------------------------------------------
//`include "uriscv_defs.v"

//-------------------------------------------------------------
// Multiplier
//-------------------------------------------------------------
reg [32:0]   mul_operand_a_q;
reg [32:0]   mul_operand_b_q;
reg          mulhi_sel_q;

//-------------------------------------------------------------
// Multiplier
//-------------------------------------------------------------
wire [64:0]  mult_result_w;
reg  [32:0]  operand_b_r;
reg  [32:0]  operand_a_r;
reg  [31:0]  mul_result_r;

wire mult_inst_w    = inst_mul_i     |
                      inst_mulh_i    |
                      inst_mulhsu_i  |
                      inst_mulhu_i;


always @ *
begin
    if (inst_mulhsu_i)
        operand_a_r = {operand_ra_i[31], operand_ra_i[31:0]};
    else if (inst_mulh_i)
        operand_a_r = {operand_ra_i[31], operand_ra_i[31:0]};
    else // MULHU || MUL
        operand_a_r = {1'b0, operand_ra_i[31:0]};
end

always @ *
begin
    if (inst_mulhsu_i)
        operand_b_r = {1'b0, operand_rb_i[31:0]};
    else if (inst_mulh_i)
        operand_b_r = {operand_rb_i[31], operand_rb_i[31:0]};
    else // MULHU || MUL
        operand_b_r = {1'b0, operand_rb_i[31:0]};
end

// Pipeline flops for multiplier
always @(posedge clk_i )
if (rst_i)
begin
    mul_operand_a_q <= 33'b0;
    mul_operand_b_q <= 33'b0;
    mulhi_sel_q     <= 1'b0;
end
else if (valid_i && mult_inst_w)
begin
    mul_operand_a_q <= operand_a_r;
    mul_operand_b_q <= operand_b_r;
    mulhi_sel_q     <= ~inst_mul_i;
end
else
begin
    mul_operand_a_q <= 33'b0;
    mul_operand_b_q <= 33'b0;
    mulhi_sel_q     <= 1'b0;
end

assign mult_result_w = {{ 32 {mul_operand_a_q[32]}}, mul_operand_a_q}*{{ 32 {mul_operand_b_q[32]}}, mul_operand_b_q};

always @ *
begin
    mul_result_r = mulhi_sel_q ? mult_result_w[63:32] : mult_result_w[31:0];
end

reg mul_busy_q;

always @(posedge clk_i )
if (rst_i)
    mul_busy_q <= 1'b0;
else
    mul_busy_q <= valid_i & mult_inst_w;

//-------------------------------------------------------------
// Divider
//-------------------------------------------------------------
wire div_rem_inst_w     = inst_div_i  || 
                          inst_divu_i ||
                          inst_rem_i  ||
                          inst_remu_i;

wire signed_operation_w = inst_div_i || inst_rem_i;
wire div_operation_w    = inst_div_i || inst_divu_i;

reg [31:0] dividend_q;
reg [62:0] divisor_q;
reg [31:0] quotient_q;
reg [31:0] q_mask_q;
reg        div_inst_q;
reg        div_busy_q;
reg        invert_res_q;

wire div_start_w    = valid_i & div_rem_inst_w & !stall_o;
wire div_complete_w = !(|q_mask_q) & div_busy_q;

always @ (posedge clk_i )
if (rst_i)
begin
    div_busy_q     <= 1'b0;
    dividend_q     <= 32'b0;
    divisor_q      <= 63'b0;
    invert_res_q   <= 1'b0;
    quotient_q     <= 32'b0;
    q_mask_q       <= 32'b0;
    div_inst_q     <= 1'b0;
end 
else if (div_start_w)
begin
    div_busy_q     <= 1'b1;
    div_inst_q     <= div_operation_w;

    if (signed_operation_w && operand_ra_i[31])
        dividend_q <= -operand_ra_i;
    else
        dividend_q <= operand_ra_i;

    if (signed_operation_w && operand_rb_i[31])
        divisor_q <= {-operand_rb_i, 31'b0};
    else
        divisor_q <= {operand_rb_i, 31'b0};

    invert_res_q  <= (inst_div_i && (operand_ra_i[31] != operand_rb_i[31]) && |operand_rb_i) || 
                     (inst_rem_i && operand_ra_i[31]);

    quotient_q     <= 32'b0;
    q_mask_q       <= 32'h80000000;
end
else if (div_complete_w)
begin
    div_busy_q <= 1'b0;
end
else if (div_busy_q)
begin
    if (divisor_q <= {31'b0, dividend_q})
    begin
        dividend_q <= dividend_q - divisor_q[31:0];
        quotient_q <= quotient_q | q_mask_q;
    end

    divisor_q <= {1'b0, divisor_q[62:1]};
    q_mask_q  <= {1'b0, q_mask_q[31:1]};
end

reg [31:0] div_result_r;
always @ *
begin
    div_result_r = 32'b0;

    if (div_inst_q)
        div_result_r = invert_res_q ? -quotient_q : quotient_q;
    else
        div_result_r = invert_res_q ? -dividend_q : dividend_q;
end

//-------------------------------------------------------------
// Shared logic
//-------------------------------------------------------------

// Stall if divider logic is busy and new multiplier or divider op
assign stall_o = (div_busy_q  & (mult_inst_w | div_rem_inst_w)) ||
                 (mul_busy_q & div_rem_inst_w);

reg  [31:0]  result_q;
reg          ready_q;

always @ (posedge clk_i )
if (rst_i)
    ready_q <= 1'b0;
else if (mul_busy_q)
    ready_q <= 1'b1;
else if (div_complete_w)
    ready_q <= 1'b1;
else
    ready_q <= 1'b0;

always @ (posedge clk_i )
if (rst_i)
    result_q <= 32'b0;
else if (div_complete_w)
    result_q <= div_result_r;
else if (mul_busy_q)
    result_q <= mul_result_r;

assign result_o  = result_q;
assign ready_o   = ready_q;

endmodule
//-----------------------------------------------------------------
//                          uRISC-V CPU
//                            V0.5.0
//               github.com/ultraembedded/core_uriscv
//                     Copyright 2015-2021
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2015-2021 github.com/ultraembedded
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//-----------------------------------------------------------------
module uriscv_alu
(
    // ALU operation select
    input [3:0]     op_i,

    // Operands
    input [31:0]    a_i,
    input [31:0]    b_i,

    // Result
    output [31:0]   p_o
);

//-----------------------------------------------------------------
// Includes
//-----------------------------------------------------------------
//`include "uriscv_defs.v"

//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------
reg [31:0]      result_r;

reg [31:16]     shift_right_fill_r;
reg [31:0]      shift_right_1_r;
reg [31:0]      shift_right_2_r;
reg [31:0]      shift_right_4_r;
reg [31:0]      shift_right_8_r;

reg [31:0]      shift_left_1_r;
reg [31:0]      shift_left_2_r;
reg [31:0]      shift_left_4_r;
reg [31:0]      shift_left_8_r;

wire [31:0]     sub_res_w = a_i - b_i;

//-----------------------------------------------------------------
// ALU
//-----------------------------------------------------------------
always @ *
begin
   case (op_i)
       //----------------------------------------------
       // Shift Left
       //----------------------------------------------   
       `RV_ALU_SHIFTL :
       begin
            if (b_i[0] == 1'b1)
                shift_left_1_r = {a_i[30:0],1'b0};
            else
                shift_left_1_r = a_i;

            if (b_i[1] == 1'b1)
                shift_left_2_r = {shift_left_1_r[29:0],2'b00};
            else
                shift_left_2_r = shift_left_1_r;

            if (b_i[2] == 1'b1)
                shift_left_4_r = {shift_left_2_r[27:0],4'b0000};
            else
                shift_left_4_r = shift_left_2_r;

            if (b_i[3] == 1'b1)
                shift_left_8_r = {shift_left_4_r[23:0],8'b00000000};
            else
                shift_left_8_r = shift_left_4_r;

            if (b_i[4] == 1'b1)
                result_r = {shift_left_8_r[15:0],16'b0000000000000000};
            else
                result_r = shift_left_8_r;
       end
       //----------------------------------------------
       // Shift Right
       //----------------------------------------------
       `RV_ALU_SHIFTR, `RV_ALU_SHIFTR_ARITH:
       begin
            // Arithmetic shift? Fill with 1's if MSB set
            if (a_i[31] == 1'b1 && op_i == `RV_ALU_SHIFTR_ARITH)
                shift_right_fill_r = 16'b1111111111111111;
            else
                shift_right_fill_r = 16'b0000000000000000;

            if (b_i[0] == 1'b1)
                shift_right_1_r = {shift_right_fill_r[31], a_i[31:1]};
            else
                shift_right_1_r = a_i;

            if (b_i[1] == 1'b1)
                shift_right_2_r = {shift_right_fill_r[31:30], shift_right_1_r[31:2]};
            else
                shift_right_2_r = shift_right_1_r;

            if (b_i[2] == 1'b1)
                shift_right_4_r = {shift_right_fill_r[31:28], shift_right_2_r[31:4]};
            else
                shift_right_4_r = shift_right_2_r;

            if (b_i[3] == 1'b1)
                shift_right_8_r = {shift_right_fill_r[31:24], shift_right_4_r[31:8]};
            else
                shift_right_8_r = shift_right_4_r;

            if (b_i[4] == 1'b1)
                result_r = {shift_right_fill_r[31:16], shift_right_8_r[31:16]};
            else
                result_r = shift_right_8_r;
       end       
       //----------------------------------------------
       // Arithmetic
       //----------------------------------------------
       `RV_ALU_ADD : 
       begin
            result_r      = (a_i + b_i);
       end
       `RV_ALU_SUB : 
       begin
            result_r      = sub_res_w;
       end
       //----------------------------------------------
       // Logical
       //----------------------------------------------       
       `RV_ALU_AND : 
       begin
            result_r      = (a_i & b_i);
       end
       `RV_ALU_OR  : 
       begin
            result_r      = (a_i | b_i);
       end
       `RV_ALU_XOR : 
       begin
            result_r      = (a_i ^ b_i);
       end
       //----------------------------------------------
       // Comparision
       //----------------------------------------------
       `RV_ALU_LESS_THAN : 
       begin
            result_r      = (a_i < b_i) ? 32'h1 : 32'h0;
       end
       `RV_ALU_LESS_THAN_SIGNED : 
       begin
            if (a_i[31] != b_i[31])
                result_r  = a_i[31] ? 32'h1 : 32'h0;
            else
                result_r  = sub_res_w[31] ? 32'h1 : 32'h0;            
       end       
       default  : 
       begin
            result_r      = a_i;
       end
   endcase
end

assign p_o    = result_r;

endmodule
//-----------------------------------------------------------------
//                          uRISC-V CPU
//                            V0.5.0
//               github.com/ultraembedded/core_uriscv
//                     Copyright 2015-2021
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2015-2021 github.com/ultraembedded
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//-----------------------------------------------------------------
module uriscv_csr
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter SUPPORT_CSR = 1
    ,parameter SUPPORT_MCYCLE = 1
    ,parameter SUPPORT_MTIMECMP = 1
    ,parameter SUPPORT_MSCRATCH = 1
    ,parameter SUPPORT_MIP_MIE = 1
    ,parameter SUPPORT_MTVEC = 1
    ,parameter SUPPORT_MTVAL = 1
    ,parameter SUPPORT_MULDIV = 1
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
     input          clk_i
    ,input          rst_i

    ,input          intr_i
    ,input  [31:0]  isr_vector_i

    ,input  [31:0]  cpu_id_i

    ,input          valid_i
    ,input  [31:0]  pc_i
    ,input  [31:0]  opcode_i
    ,input  [31:0]  rs1_val_i
    ,input  [31:0]  rs2_val_i
    ,output [31:0]  csr_rdata_o

    ,input          excpn_invalid_inst_i
    ,input          excpn_lsu_align_i

    ,input  [31:0]  mem_addr_i

    ,output [31:0]  csr_mepc_o

    ,output         exception_o
    ,output [5:0]   exception_type_o
    ,output [31:0]  exception_pc_o
);


//-----------------------------------------------------------------
// Includes
//-----------------------------------------------------------------
// `include "uriscv_defs.v"

wire take_interrupt_w;
wire exception_w;

//-----------------------------------------------------------------
// Instruction Decode
//-----------------------------------------------------------------
wire [2:0] func3_w     = opcode_i[14:12]; // R, I, S
wire [4:0] rs1_w       = opcode_i[19:15];
wire [4:0] rs2_w       = opcode_i[24:20];
wire [4:0] rd_w        = opcode_i[11:7];

wire type_system_w     = (opcode_i[6:2] == 5'b11100);
wire type_store_w      = (opcode_i[6:2] == 5'b01000);

wire inst_csr_w        = SUPPORT_CSR && type_system_w && (func3_w != 3'b000 && func3_w != 3'b100);
wire inst_csrrw_w      = inst_csr_w  && (func3_w == 3'b001);
wire inst_csrrs_w      = inst_csr_w  && (func3_w == 3'b010);
wire inst_csrrc_w      = inst_csr_w  && (func3_w == 3'b011);
wire inst_csrrwi_w     = inst_csr_w  && (func3_w == 3'b101);
wire inst_csrrsi_w     = inst_csr_w  && (func3_w == 3'b110);
wire inst_csrrci_w     = inst_csr_w  && (func3_w == 3'b111);

wire inst_ecall_w      = SUPPORT_CSR && type_system_w && (opcode_i[31:7] == 25'h000000);
wire inst_ebreak_w     = SUPPORT_CSR && type_system_w && (opcode_i[31:7] == 25'h002000);
wire inst_mret_w       = SUPPORT_CSR && type_system_w && (opcode_i[31:7] == 25'h604000);

wire [11:0] csr_addr_w = valid_i ? opcode_i[31:20] : 12'b0;
wire [31:0] csr_data_w = (inst_csrrwi_w || inst_csrrsi_w || inst_csrrci_w) ? {27'b0, rs1_w} : rs1_val_i;
wire        csr_set_w  = (valid_i && !exception_w) ? (inst_csrrw_w || inst_csrrs_w || inst_csrrwi_w || inst_csrrsi_w): 1'b0;
wire        csr_clr_w  = (valid_i && !exception_w) ? (inst_csrrw_w || inst_csrrc_w || inst_csrrwi_w || inst_csrrci_w): 1'b0;

//-----------------------------------------------------------------
// Execute: CSR Access
//-----------------------------------------------------------------
reg [31:0] csr_mepc_q;
reg [31:0] csr_mepc_r;
reg [31:0] csr_mcause_q;
reg [31:0] csr_mcause_r;
reg [31:0] csr_sr_q;
reg [31:0] csr_sr_r;
reg [31:0] csr_mcycle_q;
reg [31:0] csr_mcycle_r;
reg [31:0] csr_mtimecmp_q;
reg [31:0] csr_mtimecmp_r;
reg [31:0] csr_mscratch_q;
reg [31:0] csr_mscratch_r;
reg [31:0] csr_mip_q;
reg [31:0] csr_mip_r;
reg [31:0] csr_mie_q;
reg [31:0] csr_mie_r;
reg [31:0] csr_mtvec_q;
reg [31:0] csr_mtvec_r;
reg [31:0] csr_mtval_q;
reg [31:0] csr_mtval_r;

always @ *
begin
    csr_mepc_r      = csr_mepc_q;
    csr_mcause_r    = csr_mcause_q;
    csr_sr_r        = csr_sr_q;

    csr_mcycle_r    = csr_mcycle_q + 32'd1;
    csr_mtimecmp_r  = csr_mtimecmp_q;
    csr_mscratch_r  = csr_mscratch_q;
    csr_mip_r       = csr_mip_q;
    csr_mie_r       = csr_mie_q;
    csr_mtvec_r     = csr_mtvec_q;
    csr_mtval_r     = csr_mtval_q;

    // External interrupt
    if (intr_i)
        csr_mip_r[`IRQ_M_EXT] = 1'b1;

    // Timer match - generate IRQ
    if (SUPPORT_MTIMECMP && csr_mcycle_r == csr_mtimecmp_r)
        csr_mip_r[`SR_IP_MTIP_R] = 1'b1;

    // Execute instruction / exception
    if (valid_i)
    begin
        // Exception / break / ecall
        if (exception_w || inst_ebreak_w || inst_ecall_w)
        begin
            // Save interrupt / supervisor state
            csr_sr_r[`SR_MPIE_R] = csr_sr_q[`SR_MIE_R];
            csr_sr_r[`SR_MPP_R]  = `PRIV_MACHINE;

            // Disable interrupts and enter supervisor mode
            csr_sr_r[`SR_MIE_R]  = 1'b0;

            // Save PC of next instruction (not yet executed)
            csr_mepc_r           = pc_i;

            // Extra info (badaddr / fault opcode)
            csr_mtval_r          = 32'b0;

            // Exception source
            if (excpn_invalid_inst_i)
            begin
                csr_mcause_r   = `MCAUSE_ILLEGAL_INSTRUCTION;
                csr_mtval_r    = opcode_i;
            end
            else if (inst_ebreak_w)
                csr_mcause_r   = `MCAUSE_BREAKPOINT;
            else if (inst_ecall_w)
                csr_mcause_r   = `MCAUSE_ECALL_M;
            else if (excpn_lsu_align_i)
            begin
                csr_mcause_r   = type_store_w ? `MCAUSE_MISALIGNED_STORE : `MCAUSE_MISALIGNED_LOAD;
                csr_mtval_r    = mem_addr_i;
            end
            else if (take_interrupt_w)
                csr_mcause_r   = `MCAUSE_INTERRUPT;
        end
        // MRET
        else if (inst_mret_w) 
        begin
            // Interrupt enable pop
            csr_sr_r[`SR_MIE_R]  = csr_sr_r[`SR_MPIE_R];
            csr_sr_r[`SR_MPIE_R] = 1'b1;

            // This CPU only supports machine mode
            csr_sr_r[`SR_MPP_R] = `PRIV_MACHINE;
        end
        else
        begin
            case (csr_addr_w)
            `CSR_MEPC:
            begin
                if (csr_set_w && csr_clr_w)
                    csr_mepc_r = csr_data_w;
                else if (csr_set_w)
                    csr_mepc_r = csr_mepc_r | csr_data_w;
                else if (csr_clr_w)
                    csr_mepc_r = csr_mepc_r & ~csr_data_w;
            end
            `CSR_MCAUSE:
            begin
                if (csr_set_w && csr_clr_w)
                    csr_mcause_r = csr_data_w;
                else if (csr_set_w)
                    csr_mcause_r = csr_mcause_r | csr_data_w;
                else if (csr_clr_w)
                    csr_mcause_r = csr_mcause_r & ~csr_data_w;
            end
            `CSR_MSTATUS:
            begin
                if (csr_set_w && csr_clr_w)
                    csr_sr_r = csr_data_w;
                else if (csr_set_w)
                    csr_sr_r = csr_sr_r | csr_data_w;
                else if (csr_clr_w)
                    csr_sr_r = csr_sr_r & ~csr_data_w;
            end
            `CSR_MTIMECMP:
            begin
                if (SUPPORT_MTIMECMP && csr_set_w && csr_data_w != 32'b0)
                begin
                    csr_mtimecmp_r = csr_data_w;

                    // Clear interrupt pending
                    csr_mip_r[`SR_IP_MTIP_R] = 1'b0;
                end
            end
            `CSR_MSCRATCH:
            begin
                if (csr_set_w && csr_clr_w)
                    csr_mscratch_r = csr_data_w;
                else if (csr_set_w)
                    csr_mscratch_r = csr_mscratch_r | csr_data_w;
                else if (csr_clr_w)
                    csr_mscratch_r = csr_mscratch_r & ~csr_data_w;
            end
            `CSR_MIP:
            begin
                if (csr_set_w && csr_clr_w)
                    csr_mip_r = csr_data_w;
                else if (csr_set_w)
                    csr_mip_r = csr_mip_r | csr_data_w;
                else if (csr_clr_w)
                    csr_mip_r = csr_mip_r & ~csr_data_w;
            end
            `CSR_MIE:
            begin
                if (csr_set_w && csr_clr_w)
                    csr_mie_r = csr_data_w;
                else if (csr_set_w)
                    csr_mie_r = csr_mie_r | csr_data_w;
                else if (csr_clr_w)
                    csr_mie_r = csr_mie_r & ~csr_data_w;
            end
            `CSR_MTVEC:
            begin
                if (csr_set_w && csr_clr_w)
                    csr_mtvec_r = csr_data_w;
                else if (csr_set_w)
                    csr_mtvec_r = csr_mtvec_r | csr_data_w;
                else if (csr_clr_w)
                    csr_mtvec_r = csr_mtvec_r & ~csr_data_w;
            end
            `CSR_MTVAL:
            begin
                if (csr_set_w && csr_clr_w)
                    csr_mtval_r = csr_data_w;
                else if (csr_set_w)
                    csr_mtval_r = csr_mtval_r | csr_data_w;
                else if (csr_clr_w)
                    csr_mtval_r = csr_mtval_r & ~csr_data_w;
            end
            default:
              ;
            endcase
        end
    end
end

`ifdef verilator
`define HAS_SIM_CTRL
`endif
`ifdef verilog_sim
`define HAS_SIM_CTRL
`endif

always @ (posedge clk_i )
if (rst_i)
begin
    csr_mepc_q       <= 32'b0;
    csr_mcause_q     <= 32'b0;
    csr_sr_q         <= 32'b0;
    csr_mcycle_q     <= 32'b0;
    csr_mtimecmp_q   <= 32'b0;
    csr_mscratch_q   <= 32'b0;
    csr_mie_q        <= 32'b0;
    csr_mip_q        <= 32'b0;
    csr_mtvec_q      <= 32'b0;
    csr_mtval_q      <= 32'b0;
end
else
begin
    csr_mepc_q       <= csr_mepc_r;
    csr_mcause_q     <= csr_mcause_r;
    csr_sr_q         <= csr_sr_r;
    csr_mcycle_q     <= SUPPORT_MCYCLE   ? csr_mcycle_r   : 32'b0;
    csr_mtimecmp_q   <= SUPPORT_MTIMECMP ? csr_mtimecmp_r : 32'b0;
    csr_mscratch_q   <= SUPPORT_MSCRATCH ? csr_mscratch_r : 32'b0;
    csr_mie_q        <= SUPPORT_MIP_MIE  ? csr_mie_r      : 32'b0;
    csr_mip_q        <= SUPPORT_MIP_MIE  ? csr_mip_r      : 32'b0;
    csr_mtvec_q      <= SUPPORT_MTVEC    ? csr_mtvec_r    : 32'b0;
    csr_mtval_q      <= SUPPORT_MTVAL    ? csr_mtval_r    : 32'b0;

`ifdef HAS_SIM_CTRL
    if (valid_i && (csr_addr_w == `CSR_DSCRATCH || csr_addr_w == `CSR_SIM_CTRL) && inst_csr_w)
    begin
        case (csr_data_w & 32'hFF000000)
        `CSR_SIM_CTRL_EXIT:
        begin
            $finish;
            $finish;
        end
        `CSR_SIM_CTRL_PUTC:
        begin
            $write("%c", csr_data_w[7:0]);
        end
        endcase
    end
`endif           
end

//-----------------------------------------------------------------
// CSR Read Data MUX
//-----------------------------------------------------------------
reg [31:0] csr_data_r;

always @ *
begin
    csr_data_r = 32'b0;

    case (csr_addr_w)
    `CSR_MEPC:      csr_data_r = csr_mepc_q & `CSR_MEPC_MASK;
    `CSR_MCAUSE:    csr_data_r = csr_mcause_q & `CSR_MCAUSE_MASK;
    `CSR_MSTATUS:   csr_data_r = csr_sr_q & `CSR_MSTATUS_MASK;
    `CSR_MTVEC:     csr_data_r = csr_mtvec_q & `CSR_MTVEC_MASK;
    `CSR_MTVAL:     csr_data_r = csr_mtval_q & `CSR_MTVAL_MASK;
    `CSR_MTIME,
    `CSR_MCYCLE:    csr_data_r = csr_mcycle_q & `CSR_MTIME_MASK;
    `CSR_MTIMECMP:  csr_data_r = csr_mtimecmp_q & `CSR_MTIMECMP_MASK;
    `CSR_MSCRATCH:  csr_data_r = csr_mscratch_q & `CSR_MSCRATCH_MASK;
    `CSR_MIP:       csr_data_r = csr_mip_q & `CSR_MIP_MASK;
    `CSR_MIE:       csr_data_r = csr_mie_q & `CSR_MIE_MASK;
    `CSR_MISA:      csr_data_r = (SUPPORT_MULDIV ? `MISA_RVM : 32'b0) |
                                 `MISA_RV32 | `MISA_RVI;
    `CSR_MHARTID:   csr_data_r = cpu_id_i;
    default:        csr_data_r = 32'b0;
    endcase
end

assign csr_rdata_o       = csr_data_r;

// Interrupt request and interrupt enabled
assign take_interrupt_w  = SUPPORT_MIP_MIE ? ((|(csr_mip_q & csr_mie_q)) & csr_sr_q[`SR_MIE_R]) : (intr_i & csr_sr_q[`SR_MIE_R]);
assign exception_w       = valid_i && (take_interrupt_w || excpn_invalid_inst_i || (SUPPORT_CSR && excpn_lsu_align_i));

assign exception_o       = exception_w;
assign exception_pc_o    = SUPPORT_MTVEC ? csr_mtvec_q  : 
                           SUPPORT_CSR   ? isr_vector_i :
                           pc_i + 32'd4;
assign csr_mepc_o        = csr_mepc_q;

//-----------------------------------------------------------------
// Debug - exception type (checker use only)
//-----------------------------------------------------------------
reg [5:0] v_etype_r;

always @ *
begin
    v_etype_r = 6'b0;

    if (csr_mcause_r[`MCAUSE_INT])
        v_etype_r = `RV_EXCPN_INTERRUPT;
    else case (csr_mcause_r)
    `MCAUSE_MISALIGNED_FETCH   : v_etype_r = `RV_EXCPN_MISALIGNED_FETCH;
    `MCAUSE_FAULT_FETCH        : v_etype_r = `RV_EXCPN_FAULT_FETCH;
    `MCAUSE_ILLEGAL_INSTRUCTION: v_etype_r = `RV_EXCPN_ILLEGAL_INSTRUCTION;
    `MCAUSE_BREAKPOINT         : v_etype_r = `RV_EXCPN_BREAKPOINT;
    `MCAUSE_MISALIGNED_LOAD    : v_etype_r = `RV_EXCPN_MISALIGNED_LOAD;
    `MCAUSE_FAULT_LOAD         : v_etype_r = `RV_EXCPN_FAULT_LOAD;
    `MCAUSE_MISALIGNED_STORE   : v_etype_r = `RV_EXCPN_MISALIGNED_STORE;
    `MCAUSE_FAULT_STORE        : v_etype_r = `RV_EXCPN_FAULT_STORE;
    `MCAUSE_ECALL_U            : v_etype_r = `RV_EXCPN_ECALL_U;
    `MCAUSE_ECALL_S            : v_etype_r = `RV_EXCPN_ECALL_S;
    `MCAUSE_ECALL_H            : v_etype_r = `RV_EXCPN_ECALL_H;
    `MCAUSE_ECALL_M            : v_etype_r = `RV_EXCPN_ECALL_M;
    `MCAUSE_PAGE_FAULT_INST    : v_etype_r = `RV_EXCPN_PAGE_FAULT_INST;
    `MCAUSE_PAGE_FAULT_LOAD    : v_etype_r = `RV_EXCPN_PAGE_FAULT_LOAD;
    `MCAUSE_PAGE_FAULT_STORE   : v_etype_r = `RV_EXCPN_PAGE_FAULT_STORE;
    endcase
end

assign exception_type_o = v_etype_r;

endmodule
//-----------------------------------------------------------------
//                          uRISC-V CPU
//                            V0.5.0
//               github.com/ultraembedded/core_uriscv
//                     Copyright 2015-2021
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2015-2021 github.com/ultraembedded
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//-----------------------------------------------------------------
module uriscv_lsu
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter SUPPORT_TRAP_LSU_ALIGN = 1
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
     input  [31:0]  opcode_i
    ,input  [31:0]  rs1_val_i
    ,input  [31:0]  rs2_val_i

    ,output         mem_rd_o
    ,output [3:0]   mem_wr_o
    ,output [31:0]  mem_addr_o
    ,output [31:0]  mem_data_o
    ,output         mem_misaligned_o
);

//-----------------------------------------------------------------
// Includes
//-----------------------------------------------------------------
// `include "uriscv_defs.v"

//-----------------------------------------------------------------
// Instruction Decode
//-----------------------------------------------------------------
wire type_load_w     = (opcode_i[6:2] == 5'b00000);
wire type_store_w    = (opcode_i[6:2] == 5'b01000);

wire [2:0] func3_w   = opcode_i[14:12]; // R, I, S

wire inst_lb_w       = type_load_w  && (func3_w == 3'b000);
wire inst_lh_w       = type_load_w  && (func3_w == 3'b001);
wire inst_lw_w       = type_load_w  && (func3_w == 3'b010);
wire inst_lbu_w      = type_load_w  && (func3_w == 3'b100);
wire inst_lhu_w      = type_load_w  && (func3_w == 3'b101);
wire inst_sb_w       = type_store_w && (func3_w == 3'b000);
wire inst_sh_w       = type_store_w && (func3_w == 3'b001);
wire inst_sw_w       = type_store_w && (func3_w == 3'b010);

//-----------------------------------------------------------------
// Decode LSU operation
//-----------------------------------------------------------------
reg [31:0]  imm12_r;
reg [31:0]  storeimm_r;

reg [31:0]  mem_addr_r;
reg [31:0]  mem_data_r;
reg [3:0]   mem_wr_r;
reg         mem_rd_r;
reg         mem_misaligned_r;

always @ *
begin
    imm12_r     = {{20{opcode_i[31]}}, opcode_i[31:20]};
    storeimm_r  = {{20{opcode_i[31]}}, opcode_i[31:25], opcode_i[11:7]};

    // Memory address
    mem_addr_r  = rs1_val_i + (type_store_w ? storeimm_r : imm12_r);

    if (SUPPORT_TRAP_LSU_ALIGN)
        mem_misaligned_r = (inst_lh_w | inst_lhu_w | inst_sh_w) ? mem_addr_r[0]:
                           (inst_lw_w | inst_sw_w)              ? (|mem_addr_r[1:0]):
                           1'b0;
    else
        mem_misaligned_r = 1'b0;

    mem_data_r = 32'h00000000;
    mem_wr_r   = 4'b0000;
    mem_rd_r   = 1'b0;

    case (1'b1)

    type_load_w:
        mem_rd_r   = 1'b1;

    inst_sb_w:
    begin
        case (mem_addr_r[1:0])
        2'h3 :
        begin
            mem_data_r      = {rs2_val_i[7:0], 24'h000000};
            mem_wr_r        = 4'b1000;
            mem_rd_r        = 1'b0;
        end
        2'h2 :
        begin
            mem_data_r      = {8'h00,rs2_val_i[7:0],16'h0000};
            mem_wr_r        = 4'b0100;
            mem_rd_r        = 1'b0;
        end
        2'h1 :
        begin
            mem_data_r      = {16'h0000,rs2_val_i[7:0],8'h00};
            mem_wr_r        = 4'b0010;
            mem_rd_r        = 1'b0;
        end
        2'h0 :
        begin
            mem_data_r      = {24'h000000,rs2_val_i[7:0]};
            mem_wr_r        = 4'b0001;
            mem_rd_r        = 1'b0;
        end
        default : ;
        endcase
    end

    inst_sh_w:
    begin
        case (mem_addr_r[1:0])
        2'h2 :
        begin
            mem_data_r      = {rs2_val_i[15:0],16'h0000};
            mem_wr_r        = 4'b1100;
            mem_rd_r        = 1'b0;
        end
        default :
        begin
            mem_data_r      = {16'h0000,rs2_val_i[15:0]};
            mem_wr_r        = 4'b0011;
            mem_rd_r        = 1'b0;
        end
        endcase
    end

    inst_sw_w:
    begin
        mem_data_r          = rs2_val_i;
        mem_wr_r            = 4'b1111;
        mem_rd_r            = 1'b0;
    end

    // Non load / store
    default:
        ;
    endcase
end

assign mem_rd_o         = mem_rd_r;
assign mem_wr_o         = mem_wr_r;
assign mem_addr_o       = mem_addr_r;
assign mem_data_o       = mem_data_r;
assign mem_misaligned_o = mem_misaligned_r;

endmodule
//-----------------------------------------------------------------
//                          uRISC-V CPU
//                            V0.5.0
//               github.com/ultraembedded/core_uriscv
//                     Copyright 2015-2021
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2015-2021 github.com/ultraembedded
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//-----------------------------------------------------------------

module riscv_core
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter SUPPORT_MUL      = 1
    ,parameter SUPPORT_DIV      = 1
    ,parameter SUPPORT_CSR      = 1
    ,parameter SUPPORT_TRAP_LSU_ALIGN = 1
    ,parameter SUPPORT_MTVEC    = 0
    ,parameter SUPPORT_MTVAL    = 0
    ,parameter SUPPORT_MIP_MIE  = 0
    ,parameter SUPPORT_MSCRATCH = 0
    ,parameter SUPPORT_MCYCLE   = 1
    ,parameter SUPPORT_MTIMECMP = 0
    ,parameter SUPPORT_TRAP_INVALID_OPC = 1
    ,parameter SUPPORT_BRAM_REGFILE = 0
    ,parameter ISR_VECTOR       = 32'h00000010
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Clock
     input           clk_i

    // Reset (active high)
    ,input           rst_i

    // External interrupt (M_EXT)
    ,input           intr_i

    // Initial boot address
    ,input  [ 31:0]  reset_vector_i

    // MHARTID value
    ,input  [ 31:0]  cpu_id_i

    // Instruction Fetch
    ,output          mem_i_rd_o
    ,output [ 31:0]  mem_i_pc_o
    ,input           mem_i_accept_i
    ,input           mem_i_valid_i
    ,input  [ 31:0]  mem_i_inst_i

    // Instruction fetch: Unused on this core
    ,output          mem_i_flush_o
    ,output          mem_i_invalidate_o

    // Instruction fetch: Unused (tie low)
    ,input           mem_i_error_i

    // Data Access
    ,output [ 31:0]  mem_d_addr_o
    ,output [ 31:0]  mem_d_data_wr_o
    ,output          mem_d_rd_o
    ,output [  3:0]  mem_d_wr_o
    ,input  [ 31:0]  mem_d_data_rd_i
    ,input           mem_d_accept_i
    ,input           mem_d_ack_i

    // Instruction fetch: Unused on this core
    ,output          mem_d_cacheable_o
    ,output [ 10:0]  mem_d_req_tag_o
    ,output          mem_d_invalidate_o
    ,output          mem_d_writeback_o
    ,output          mem_d_flush_o

    // Data Access: Unused (tie low)
    ,input           mem_d_error_i
    ,input  [ 10:0]  mem_d_resp_tag_i
);



// `include "uriscv_defs.v"

//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
`define PC_W        32
`define ADDR_W      32

localparam           PC_W                = `PC_W;
localparam           PC_PAD_W            = 0;
localparam           PC_EXT_W            = 0;

localparam           ADDR_W              = `ADDR_W;
localparam           ADDR_PAD_W          = 0;

// Current state
localparam           STATE_W           = 3;
localparam           STATE_RESET       = 0;
localparam           STATE_FETCH_WB    = 1;
localparam           STATE_EXEC        = 2;
localparam           STATE_MEM         = 3;
localparam           STATE_DECODE      = 4; // Only if SUPPORT_BRAM_REGFILE = 1

//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------

// Current state
reg [STATE_W-1:0] state_q;

// Executing PC
reg [PC_W-1:0]  pc_q;

// Destination register
reg [4:0]       rd_q;

// Destination writeback enable
reg             rd_wr_en_q;

// ALU inputs
reg [31:0]      alu_a_q;
reg [31:0]      alu_b_q;

// ALU operation selection
reg [3:0]       alu_func_q;

// CSR read data
wire [31:0]     csr_data_w;

// Instruction decode fault
reg             invalid_inst_r;

// Register indexes
wire [4:0]      rd_w;
wire [4:0]      rs1_w;
wire [4:0]      rs2_w;

// Operand values
wire [31:0]     rs1_val_w;
wire [31:0]     rs2_val_w;

// Opcode (memory bus)
wire [31:0]     opcode_w;

wire            opcode_valid_w;
wire            opcode_fetch_w = mem_i_rd_o & mem_i_accept_i;

// Execute exception (or interrupt)
wire            exception_w;
wire [5:0]      exception_type_w;
wire [31:0]     exception_target_w;

wire [31:0]     csr_mepc_w;

// Load result (formatted based on load type)
reg [31:0]      load_result_r;

// Writeback enable / value
wire            rd_writeen_w;
wire [31:0]     rd_val_w;

// Memory interface
wire             mem_misaligned_w;
reg [ADDR_W-1:0] mem_addr_q;
reg [31:0]       mem_data_q;
reg [3:0]        mem_wr_q;
reg              mem_rd_q;

// Load type / byte / half index
reg [1:0]       load_offset_q;
reg             load_signed_q;
reg             load_byte_q;
reg             load_half_q;

wire            enable_w = 1'b1;

wire [31:0]     muldiv_result_w;
wire            muldiv_ready_w;
wire            muldiv_inst_w;

//-----------------------------------------------------------------
// ALU
//-----------------------------------------------------------------
uriscv_alu alu
(
    // ALU operation select
    .op_i(alu_func_q),

    // Operands
    .a_i(alu_a_q),
    .b_i(alu_b_q),

    // Result
    .p_o(rd_val_w)
);

//-----------------------------------------------------------------
// Register file
//-----------------------------------------------------------------
reg [31:0] reg_file[0:31];

always @ (posedge clk_i)
if (rd_writeen_w)
    reg_file[rd_q] <= rd_val_w;

wire [31:0] rs1_val_gpr_w = reg_file[mem_i_inst_i[19:15]];
wire [31:0] rs2_val_gpr_w = reg_file[mem_i_inst_i[24:20]];

reg [31:0] rs1_val_gpr_q;
reg [31:0] rs2_val_gpr_q;

always @ (posedge clk_i)
begin
    rs1_val_gpr_q <= rs1_val_gpr_w;
    rs2_val_gpr_q <= rs2_val_gpr_w;
end

assign rs1_val_w = SUPPORT_BRAM_REGFILE ? rs1_val_gpr_q : rs1_val_gpr_w;
assign rs2_val_w = SUPPORT_BRAM_REGFILE ? rs2_val_gpr_q : rs2_val_gpr_w;

// Writeback enable
assign rd_writeen_w  = rd_wr_en_q & (state_q == STATE_FETCH_WB);


`ifdef verilator
`define HAS_REGFILE_WIRES
`endif
`ifdef verilog_sim
`define HAS_REGFILE_WIRES
`endif

// Simulation friendly names
`ifdef HAS_REGFILE_WIRES
wire [31:0] x0_zero_w = reg_file[0];
wire [31:0] x1_ra_w   = reg_file[1];
wire [31:0] x2_sp_w   = reg_file[2];
wire [31:0] x3_gp_w   = reg_file[3];
wire [31:0] x4_tp_w   = reg_file[4];
wire [31:0] x5_t0_w   = reg_file[5];
wire [31:0] x6_t1_w   = reg_file[6];
wire [31:0] x7_t2_w   = reg_file[7];
wire [31:0] x8_s0_w   = reg_file[8];
wire [31:0] x9_s1_w   = reg_file[9];
wire [31:0] x10_a0_w  = reg_file[10];
wire [31:0] x11_a1_w  = reg_file[11];
wire [31:0] x12_a2_w  = reg_file[12];
wire [31:0] x13_a3_w  = reg_file[13];
wire [31:0] x14_a4_w  = reg_file[14];
wire [31:0] x15_a5_w  = reg_file[15];
wire [31:0] x16_a6_w  = reg_file[16];
wire [31:0] x17_a7_w  = reg_file[17];
wire [31:0] x18_s2_w  = reg_file[18];
wire [31:0] x19_s3_w  = reg_file[19];
wire [31:0] x20_s4_w  = reg_file[20];
wire [31:0] x21_s5_w  = reg_file[21];
wire [31:0] x22_s6_w  = reg_file[22];
wire [31:0] x23_s7_w  = reg_file[23];
wire [31:0] x24_s8_w  = reg_file[24];
wire [31:0] x25_s9_w  = reg_file[25];
wire [31:0] x26_s10_w = reg_file[26];
wire [31:0] x27_s11_w = reg_file[27];
wire [31:0] x28_t3_w  = reg_file[28];
wire [31:0] x29_t4_w  = reg_file[29];
wire [31:0] x30_t5_w  = reg_file[30];
wire [31:0] x31_t6_w  = reg_file[31];
`endif

//-----------------------------------------------------------------
// Next State Logic
//-----------------------------------------------------------------
reg [STATE_W-1:0] next_state_r;
always @ *
begin
    next_state_r = state_q;

    case (state_q)
    // RESET - First cycle after reset
    STATE_RESET:
    begin
        next_state_r = STATE_FETCH_WB;
    end
    // FETCH_WB - Writeback / Fetch next isn
    STATE_FETCH_WB :
    begin
        if (opcode_fetch_w)
            next_state_r    = SUPPORT_BRAM_REGFILE ? STATE_DECODE : STATE_EXEC;
    end
    // DECODE - Used to access register file if SUPPORT_BRAM_REGFILE=1
    STATE_DECODE:
    begin
        if (mem_i_valid_i)
            next_state_r = STATE_EXEC;
    end
    // EXEC - Execute instruction (when ready)
    STATE_EXEC :
    begin
        // Instruction ready
        if (opcode_valid_w)
        begin
            if (exception_w)
                next_state_r    = STATE_FETCH_WB;
            else if (type_load_w || type_store_w)
                next_state_r    = STATE_MEM;
            // Multiplication / division - stay in exec state until result ready
            else if (muldiv_inst_w)
                ;
            else
                next_state_r    = STATE_FETCH_WB;
        end
        else if (muldiv_ready_w)
            next_state_r    = STATE_FETCH_WB;
    end
    // MEM - Perform load or store
    STATE_MEM :
    begin
        // Memory access complete
        if (mem_d_ack_i)
            next_state_r = STATE_FETCH_WB;
    end
    default:
        ;
    endcase

    if (!enable_w)
        next_state_r = STATE_RESET;
end

// Update state
always @ (posedge clk_i )
if (rst_i)
    state_q   <= STATE_RESET;
else
    state_q   <= next_state_r;

//-----------------------------------------------------------------
// Instruction Decode
//-----------------------------------------------------------------
reg [31:0] opcode_q;

always @ (posedge clk_i )
if (rst_i)
    opcode_q <= 32'b0;
else if (state_q == STATE_DECODE)
    opcode_q <= mem_i_inst_i;

reg opcode_valid_q;

always @ (posedge clk_i )
if (rst_i)
    opcode_valid_q <= 1'b0;
else if (state_q == STATE_DECODE)
    opcode_valid_q <= mem_i_valid_i;
else
    opcode_valid_q <= 1'b0;

assign opcode_w       = SUPPORT_BRAM_REGFILE ? opcode_q : mem_i_inst_i;
assign opcode_valid_w = SUPPORT_BRAM_REGFILE ? opcode_valid_q : mem_i_valid_i;

assign rs1_w        = opcode_w[19:15];
assign rs2_w        = opcode_w[24:20];
assign rd_w         = opcode_w[11:7];

wire type_rvc_w     = (opcode_w[1:0] != 2'b11);

wire type_load_w    = (opcode_w[6:2] == 5'b00000);
wire type_opimm_w   = (opcode_w[6:2] == 5'b00100);
wire type_auipc_w   = (opcode_w[6:2] == 5'b00101);
wire type_store_w   = (opcode_w[6:2] == 5'b01000);
wire type_op_w      = (opcode_w[6:2] == 5'b01100);
wire type_lui_w     = (opcode_w[6:2] == 5'b01101);
wire type_branch_w  = (opcode_w[6:2] == 5'b11000);
wire type_jalr_w    = (opcode_w[6:2] == 5'b11001);
wire type_jal_w     = (opcode_w[6:2] == 5'b11011);
wire type_system_w  = (opcode_w[6:2] == 5'b11100);
wire type_miscm_w   = (opcode_w[6:2] == 5'b00011);

wire [2:0] func3_w  = opcode_w[14:12]; // R, I, S
wire [6:0] func7_w  = opcode_w[31:25]; // R

// ALU operations excluding mul/div
wire type_alu_op_w  = (type_op_w && (func7_w == 7'b0000000)) ||
                      (type_op_w && (func7_w == 7'b0100000));

// Loose decoding - gate with type_load_w on use
wire inst_lb_w       = (func3_w == 3'b000);
wire inst_lh_w       = (func3_w == 3'b001);
wire inst_lbu_w      = (func3_w == 3'b100);
wire inst_lhu_w      = (func3_w == 3'b101);

wire inst_ecall_w    = SUPPORT_CSR && type_system_w && (opcode_w[31:7] == 25'h000000);
wire inst_ebreak_w   = SUPPORT_CSR && type_system_w && (opcode_w[31:7] == 25'h002000);
wire inst_mret_w     = SUPPORT_CSR && type_system_w && (opcode_w[31:7] == 25'h604000);

wire inst_csr_w      = SUPPORT_CSR && type_system_w && (func3_w != 3'b000 && func3_w != 3'b100);

wire mul_inst_w      = SUPPORT_MUL && type_op_w && (func7_w == 7'b0000001) && ~func3_w[2];
wire div_inst_w      = SUPPORT_DIV && type_op_w && (func7_w == 7'b0000001) &&  func3_w[2];
wire inst_mul_w      = mul_inst_w && (func3_w == 3'b000);
wire inst_mulh_w     = mul_inst_w && (func3_w == 3'b001);
wire inst_mulhsu_w   = mul_inst_w && (func3_w == 3'b010);
wire inst_mulhu_w    = mul_inst_w && (func3_w == 3'b011);
wire inst_div_w      = div_inst_w && (func3_w == 3'b100);
wire inst_divu_w     = div_inst_w && (func3_w == 3'b101);
wire inst_rem_w      = div_inst_w && (func3_w == 3'b110);
wire inst_remu_w     = div_inst_w && (func3_w == 3'b111);
wire inst_nop_w      = (type_miscm_w && (func3_w == 3'b000)) | // fence
                       (type_miscm_w && (func3_w == 3'b001));  // fence.i

assign muldiv_inst_w = mul_inst_w | div_inst_w;

reg [31:0]  imm20_r;
reg [31:0]  imm12_r;

always @ *
begin
    imm20_r     = {opcode_w[31:12], 12'b0};
    imm12_r     = {{20{opcode_w[31]}}, opcode_w[31:20]};
end

//-----------------------------------------------------------------
// ALU inputs
//-----------------------------------------------------------------
// ALU operation selection
reg [3:0]  alu_func_r;

// ALU operands
reg [31:0] alu_input_a_r;
reg [31:0] alu_input_b_r;
reg        write_rd_r;

always @ *
begin
    alu_func_r     = `RV_ALU_NONE;
    alu_input_a_r  = rs1_val_w;
    alu_input_b_r  = rs2_val_w;
    write_rd_r     = 1'b0;

    case (1'b1)
    type_alu_op_w:
    begin
        alu_input_a_r  = rs1_val_w;
        alu_input_b_r  = rs2_val_w;
    end
    type_opimm_w:
    begin
        alu_input_a_r  = rs1_val_w;
        alu_input_b_r  = imm12_r;
    end
    type_lui_w:
    begin
        alu_input_a_r  = 32'b0;
        alu_input_b_r  = imm20_r;
    end
    type_auipc_w:
    begin
        alu_input_a_r[PC_W-1:0]  = pc_q;
        alu_input_b_r  = imm20_r;
    end
    type_jal_w,
    type_jalr_w:
    begin
        alu_input_a_r[PC_W-1:0]  = pc_q;
        alu_input_b_r  = 32'd4;
    end
    default : ;
    endcase

    if (muldiv_inst_w)
        write_rd_r     = 1'b1;
    else if (type_opimm_w || type_alu_op_w)
    begin
        case (func3_w)
        3'b000:  alu_func_r =  (type_op_w & opcode_w[30]) ? 
                              `RV_ALU_SUB:              // SUB
                              `RV_ALU_ADD;              // ADD  / ADDI
        3'b001:  alu_func_r = `RV_ALU_SHIFTL;           // SLL  / SLLI
        3'b010:  alu_func_r = `RV_ALU_LESS_THAN_SIGNED; // SLT  / SLTI
        3'b011:  alu_func_r = `RV_ALU_LESS_THAN;        // SLTU / SLTIU
        3'b100:  alu_func_r = `RV_ALU_XOR;              // XOR  / XORI
        3'b101:  alu_func_r = opcode_w[30] ? 
                              `RV_ALU_SHIFTR_ARITH:     // SRA  / SRAI
                              `RV_ALU_SHIFTR;           // SRL  / SRLI
        3'b110:  alu_func_r = `RV_ALU_OR;               // OR   / ORI
        3'b111:  alu_func_r = `RV_ALU_AND;              // AND  / ANDI
        endcase

        write_rd_r = 1'b1;
    end
    else if (inst_csr_w)
    begin
        alu_func_r     = `RV_ALU_ADD;
        alu_input_a_r  = 32'b0;
        alu_input_b_r  = csr_data_w;
        write_rd_r     = 1'b1;
    end
    else if (type_auipc_w || type_lui_w || type_jalr_w || type_jal_w)
    begin
        write_rd_r     = 1'b1;
        alu_func_r     = `RV_ALU_ADD;
    end
    else if (type_load_w)
        write_rd_r     = 1'b1;
end

//-------------------------------------------------------------------
// Load result resolve
//-------------------------------------------------------------------
always @ *
begin
    load_result_r = 32'b0;

    if (load_byte_q)
    begin
        case (load_offset_q[1:0])
            2'h3:
                load_result_r = {24'b0, mem_d_data_rd_i[31:24]};
            2'h2:
                load_result_r = {24'b0, mem_d_data_rd_i[23:16]};
            2'h1:
                load_result_r = {24'b0, mem_d_data_rd_i[15:8]};
            2'h0:
                load_result_r = {24'b0, mem_d_data_rd_i[7:0]};
        endcase

        if (load_signed_q && load_result_r[7])
            load_result_r = {24'hFFFFFF, load_result_r[7:0]};
    end
    else if (load_half_q)
    begin
        if (load_offset_q[1])
            load_result_r = {16'b0, mem_d_data_rd_i[31:16]};
        else
            load_result_r = {16'b0, mem_d_data_rd_i[15:0]};

        if (load_signed_q && load_result_r[15])
            load_result_r = {16'hFFFF, load_result_r[15:0]};
    end
    else
        load_result_r = mem_d_data_rd_i;
end

//-----------------------------------------------------------------
// Branches
//-----------------------------------------------------------------
wire        branch_w;
wire [31:0] branch_target_w;
wire [31:0] pc_ext_w = {{PC_EXT_W{1'b0}}, pc_q};

uriscv_branch
u_branch
(
     .pc_i(pc_ext_w)
    ,.opcode_i(opcode_w)
    ,.rs1_val_i(rs1_val_w)
    ,.rs2_val_i(rs2_val_w)
    ,.branch_o(branch_w)
    ,.branch_target_o(branch_target_w)
);

//-----------------------------------------------------------------
// Invalid instruction
//-----------------------------------------------------------------
always @ *
begin
    invalid_inst_r = SUPPORT_TRAP_INVALID_OPC;

    if (   type_load_w
         | type_opimm_w
         | type_auipc_w
         | type_store_w
         | type_alu_op_w
         | type_lui_w
         | type_branch_w
         | type_jalr_w
         | type_jal_w
         | inst_ecall_w 
         | inst_ebreak_w 
         | inst_mret_w 
         | inst_csr_w
         | inst_nop_w
         | muldiv_inst_w)
        invalid_inst_r = SUPPORT_TRAP_INVALID_OPC && type_rvc_w;
end

//-----------------------------------------------------------------
// Execute: ALU control
//-----------------------------------------------------------------
always @ (posedge clk_i )
if (rst_i)
begin      
    alu_func_q   <= `RV_ALU_NONE;
    alu_a_q      <= 32'h00000000;
    alu_b_q      <= 32'h00000000;
    rd_q         <= 5'b00000;

    // Reset x0 in-case of RAM
    rd_wr_en_q   <= 1'b1;
end
// Load result ready
else if ((state_q == STATE_MEM) && mem_d_ack_i)
begin
    // Update ALU input with load result
    alu_func_q   <= `RV_ALU_NONE;
    alu_a_q      <= load_result_r;
    alu_b_q      <= 32'b0;
end
// Multiplier / Divider result
else if (muldiv_ready_w)
begin
    // Update ALU input with load result
    alu_func_q   <= `RV_ALU_NONE;
    alu_a_q      <= muldiv_result_w;
    alu_b_q      <= 32'b0;
end
// Execute instruction
else if (opcode_valid_w)
begin
    // Update ALU input flops
    alu_func_q   <= alu_func_r;
    alu_a_q      <= alu_input_a_r;
    alu_b_q      <= alu_input_b_r;

    // Take exception
    if (exception_w)
    begin
        // No register writeback
        rd_q         <= 5'b0;
        rd_wr_en_q   <= 1'b0;
    end   
    // Valid instruction
    else
    begin
        // Instruction with register writeback
        rd_q         <= rd_w;
        rd_wr_en_q   <= write_rd_r & (rd_w != 5'b0);
    end
end
else if (state_q == STATE_FETCH_WB)
   rd_wr_en_q   <= 1'b0;

//-----------------------------------------------------------------
// Execute: Branch / exceptions
//-----------------------------------------------------------------
wire [31:0] boot_vector_w = reset_vector_i;

always @ (posedge clk_i )
if (rst_i)
    pc_q        <= boot_vector_w[PC_W-1:0];
else if (state_q == STATE_RESET)
    pc_q        <= boot_vector_w[PC_W-1:0];
else if (opcode_valid_w)
begin
    // Exception / Break / ecall (branch to ISR)
    if (exception_w || inst_ebreak_w || inst_ecall_w) 
        pc_q    <= exception_target_w[PC_W-1:0];
    // MRET (branch to EPC)
    else if (inst_mret_w) 
        pc_q    <= csr_mepc_w;
    // Branch
    else if (branch_w)
        pc_q    <= branch_target_w[PC_W-1:0];
    else
        pc_q    <= pc_q + `PC_W'd4;
end


//-----------------------------------------------------------------
// Writeback/Fetch: Instruction Fetch
//-----------------------------------------------------------------
assign mem_i_rd_o = (state_q == STATE_FETCH_WB);
assign mem_i_pc_o = pc_ext_w;

//-----------------------------------------------------------------
// Execute: Memory operations
//-----------------------------------------------------------------
wire         mem_rd_w;
wire [3:0]   mem_wr_w;
wire [31:0]  mem_addr_w;
wire [31:0]  mem_data_w;

uriscv_lsu
#( .SUPPORT_TRAP_LSU_ALIGN(SUPPORT_TRAP_LSU_ALIGN) )
u_lsu
(
     .opcode_i(opcode_w)
    ,.rs1_val_i(rs1_val_w)
    ,.rs2_val_i(rs2_val_w)

    ,.mem_rd_o(mem_rd_w)
    ,.mem_wr_o(mem_wr_w)
    ,.mem_addr_o(mem_addr_w)
    ,.mem_data_o(mem_data_w)
    ,.mem_misaligned_o(mem_misaligned_w)
);

always @ (posedge clk_i )
if (rst_i)
begin
    mem_addr_q  <= {ADDR_W{1'b0}};
    mem_data_q  <= 32'h00000000;
    mem_wr_q    <= 4'b0000;
    mem_rd_q    <= 1'b0;
end
// Valid instruction to execute
else if (opcode_valid_w && !exception_w)
begin
    mem_addr_q  <= {mem_addr_w[ADDR_W-1:2], 2'b0};
    mem_data_q  <= mem_data_w;
    mem_wr_q    <= mem_wr_w;
    mem_rd_q    <= mem_rd_w;
end
// No instruction, clear memory request
else if (mem_d_accept_i)
begin
    mem_wr_q    <= 4'b0000;
    mem_rd_q    <= 1'b0;
end

always @ (posedge clk_i )
if (rst_i)
begin
    load_signed_q  <= 1'b0;
    load_byte_q    <= 1'b0;
    load_half_q    <= 1'b0;
    load_offset_q  <= 2'b0;
end
// Valid instruction to execute
else if (opcode_valid_w)
begin
    load_signed_q  <= inst_lh_w | inst_lb_w;
    load_byte_q    <= inst_lb_w | inst_lbu_w;
    load_half_q    <= inst_lh_w | inst_lhu_w;
    load_offset_q  <= mem_addr_w[1:0];
end

assign mem_d_addr_o    = {{ADDR_PAD_W{1'b0}}, mem_addr_q};
assign mem_d_data_wr_o = mem_data_q;
assign mem_d_wr_o      = mem_wr_q;
assign mem_d_rd_o      = mem_rd_q;

//-----------------------------------------------------------------
// Execute: CSR Access
//-----------------------------------------------------------------
uriscv_csr
#(
     .SUPPORT_CSR(SUPPORT_CSR)
    ,.SUPPORT_MCYCLE(SUPPORT_MCYCLE)
    ,.SUPPORT_MTIMECMP(SUPPORT_MTIMECMP)
    ,.SUPPORT_MSCRATCH(SUPPORT_MSCRATCH)
    ,.SUPPORT_MIP_MIE(SUPPORT_MIP_MIE)
    ,.SUPPORT_MTVEC(SUPPORT_MTVEC)
    ,.SUPPORT_MTVAL(SUPPORT_MTVAL)
    ,.SUPPORT_MULDIV(SUPPORT_MUL || SUPPORT_DIV)
)
u_csr
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    // Reset vector (only used if SUPPORT_MTVEC=0)
    ,.isr_vector_i(reset_vector_i + ISR_VECTOR)

    // HartID
    ,.cpu_id_i(cpu_id_i)

    // External interrupt
    ,.intr_i(intr_i)

    // Executing instruction
    ,.valid_i(opcode_valid_w)
    ,.opcode_i(opcode_w)
    ,.pc_i(pc_q)
    ,.rs1_val_i(rs1_val_w)
    ,.rs2_val_i(rs2_val_w)

    // CSR read result
    ,.csr_rdata_o(csr_data_w)

    // Exception sources
    ,.excpn_invalid_inst_i(invalid_inst_r)
    ,.excpn_lsu_align_i(mem_misaligned_w)

    // Used on memory alignment errors
    ,.mem_addr_i(mem_addr_w)

    // CSR registers
    ,.csr_mepc_o(csr_mepc_w)

    // Exception entry
    ,.exception_o(exception_w)
    ,.exception_type_o(exception_type_w)
    ,.exception_pc_o(exception_target_w)
);

//-----------------------------------------------------------------
// Multiplier / Divider
//-----------------------------------------------------------------
generate
if (SUPPORT_MUL != 0 || SUPPORT_DIV != 0)
begin
    uriscv_muldiv
    u_muldiv
    (
        .clk_i(clk_i),
        .rst_i(rst_i),

        // Operation select
        .valid_i(opcode_valid_w & ~exception_w),
        .inst_mul_i(inst_mul_w),
        .inst_mulh_i(inst_mulh_w),
        .inst_mulhsu_i(inst_mulhsu_w),
        .inst_mulhu_i(inst_mulhu_w),
        .inst_div_i(inst_div_w),
        .inst_divu_i(inst_divu_w),
        .inst_rem_i(inst_rem_w),
        .inst_remu_i(inst_remu_w),

        // Operands
        .operand_ra_i(rs1_val_w),
        .operand_rb_i(rs2_val_w),

        // Result
        .stall_o(),
        .ready_o(muldiv_ready_w),
        .result_o(muldiv_result_w)
    );
end
else
begin
    assign muldiv_ready_w  = 1'b0;
    assign muldiv_result_w = 32'b0;
end
endgenerate

//-----------------------------------------------------------------
// Unused
//-----------------------------------------------------------------
assign mem_i_flush_o      = 1'b0;
assign mem_i_invalidate_o = 1'b0;

assign mem_d_flush_o      = 1'b0;
assign mem_d_cacheable_o  = 1'b0;
assign mem_d_req_tag_o    = 11'b0;
assign mem_d_invalidate_o = 1'b0;
assign mem_d_writeback_o  = 1'b0;

//-------------------------------------------------------------------
// Hooks for debug
//-------------------------------------------------------------------
`ifdef verilator
reg        v_dbg_valid_q;
reg [31:0] v_dbg_pc_q;

always @ (posedge clk_i )
if (rst_i)
begin
    v_dbg_valid_q  <= 1'b0;
    v_dbg_pc_q     <= 32'b0;
end
else
begin
    v_dbg_valid_q  <= opcode_valid_w;
    v_dbg_pc_q     <= pc_ext_w;
end

//-------------------------------------------------------------------
// get_valid: Instruction valid
//-------------------------------------------------------------------
function [0:0] get_valid; /*verilator public*/
begin
    get_valid = v_dbg_valid_q;
end
endfunction
//-------------------------------------------------------------------
// get_pc: Get executed instruction PC
//-------------------------------------------------------------------
function [31:0] get_pc; /*verilator public*/
begin
    get_pc = v_dbg_pc_q;
end
endfunction
//-------------------------------------------------------------------
// get_reg_valid: Register contents valid
//-------------------------------------------------------------------
function [0:0] get_reg_valid; /*verilator public*/
    input [4:0] r;
begin    
    get_reg_valid = opcode_valid_w;
end
endfunction
//-------------------------------------------------------------------
// get_register: Read register file
//-------------------------------------------------------------------
function [31:0] get_register; /*verilator public*/
    input [4:0] r;
begin
    get_register = reg_file[r];
end
endfunction
`endif



endmodule
