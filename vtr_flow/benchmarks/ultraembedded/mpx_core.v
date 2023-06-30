//-----------------------------------------------------------------
//                          MPX Core
//                            V0.1
//                   github.com/ultraembedded
//                       Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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
`define ALU_NONE            4'd0
`define ALU_SHIFTL          4'd1
`define ALU_SHIFTR          4'd2
`define ALU_SHIFTR_ARITH    4'd3
`define ALU_ADD             4'd4
`define ALU_SUB             4'd5
`define ALU_AND             4'd6
`define ALU_OR              4'd7
`define ALU_XOR             4'd8
`define ALU_NOR             4'd9
`define ALU_SLT             4'd10
`define ALU_SLTE            4'd11
`define ALU_SLTU            4'd12

//--------------------------------------------------------------------
// Instruction Encoding
//--------------------------------------------------------------------  
`define OPCODE_INST_R       31:26
`define OPCODE_RS_R         25:21
`define OPCODE_RT_R         20:16
`define OPCODE_RD_R         15:11
`define OPCODE_RE_R         10:6
`define OPCODE_FUNC_R       5:0
`define OPCODE_IMM_R        15:0
`define OPCODE_ADDR_R       25:0

//--------------------------------------------------------------------
// R Type
//--------------------------------------------------------------------
`define INSTR_R_SPECIAL     6'h00
`define INSTR_R_SLL         6'h00
`define INSTR_R_SRL         6'h02
`define INSTR_R_SRA         6'h03
`define INSTR_R_SLLV        6'h04
`define INSTR_R_SRLV        6'h06
`define INSTR_R_SRAV        6'h07
`define INSTR_R_JR          6'h08
`define INSTR_R_JALR        6'h09
`define INSTR_R_SYSCALL     6'h0c
`define INSTR_R_BREAK       6'h0d
`define INSTR_R_MFHI        6'h10
`define INSTR_R_MTHI        6'h11
`define INSTR_R_MFLO        6'h12
`define INSTR_R_MTLO        6'h13
`define INSTR_R_MULT        6'h18
`define INSTR_R_MULTU       6'h19
`define INSTR_R_DIV         6'h1a
`define INSTR_R_DIVU        6'h1b
`define INSTR_R_ADD         6'h20
`define INSTR_R_ADDU        6'h21
`define INSTR_R_SUB         6'h22
`define INSTR_R_SUBU        6'h23
`define INSTR_R_AND         6'h24
`define INSTR_R_OR          6'h25
`define INSTR_R_XOR         6'h26
`define INSTR_R_NOR         6'h27
`define INSTR_R_SLT         6'h2a
`define INSTR_R_SLTU        6'h2b

`define INSTR_COP0          6'h10
`define INSTR_COP1          6'h11
`define INSTR_COP2          6'h12
`define INSTR_COP3          6'h13

// NOP (duplicate - actually SLL)
`define INSTR_NOP           32'b0

//--------------------------------------------------------------------
// J Type
//--------------------------------------------------------------------
`define INSTR_J_JAL         6'h03
`define INSTR_J_J           6'h02
`define INSTR_J_BEQ         6'h04
`define INSTR_J_BNE         6'h05
`define INSTR_J_BLEZ        6'h06
`define INSTR_J_BGTZ        6'h07

//--------------------------------------------------------------------
// I Type
//--------------------------------------------------------------------
`define INSTR_I_ADDI        6'h08
`define INSTR_I_ADDIU       6'h09
`define INSTR_I_SLTI        6'h0a
`define INSTR_I_SLTIU       6'h0b
`define INSTR_I_ANDI        6'h0c
`define INSTR_I_ORI         6'h0d
`define INSTR_I_XORI        6'h0e
`define INSTR_I_LUI         6'h0f
`define INSTR_I_LB          6'h20
`define INSTR_I_LH          6'h21
`define INSTR_I_LW          6'h23
`define INSTR_I_LBU         6'h24
`define INSTR_I_LHU         6'h25
`define INSTR_I_SB          6'h28
`define INSTR_I_SH          6'h29
`define INSTR_I_SW          6'h2b

`define INSTR_I_BRCOND      6'h01
    `define INSTR_I_BRCOND_GTE_R 0:0
    `define INSTR_I_BRCOND_RA_MASK  5'h1e
    `define INSTR_I_BRCOND_RA_WR    5'h10

`define INSTR_I_LWL         6'h22
`define INSTR_I_LWR         6'h26
`define INSTR_I_SWL         6'h2a
`define INSTR_I_SWR         6'h2e

`define INSTR_I_LWC0        6'h30
`define INSTR_I_LWC1        6'h31
`define INSTR_I_LWC2        6'h32
`define INSTR_I_LWC3        6'h33
`define INSTR_I_SWC0        6'h38
`define INSTR_I_SWC1        6'h39
`define INSTR_I_SWC2        6'h3a
`define INSTR_I_SWC3        6'h3b

//--------------------------------------------------------------------
// COP0
//--------------------------------------------------------------------
`define COP0_RFE            5'h10
`define COP0_MFC0           5'h00
`define COP0_MTC0           5'h04

`define COP0_STATUS         5'd12 // Processor status and control
    `define COP0_SR_IEC         0 // Interrupt enable (current)
    `define COP0_SR_KUC         1 // User mode (current)
    `define COP0_SR_IEP         2 // Interrupt enable (previous)
    `define COP0_SR_KUP         3 // User mode (previous)
    `define COP0_SR_IEO         4 // Interrupt enable (old)
    `define COP0_SR_KUO         5 // User mode (old)
    `define COP0_SR_IM0         8 // Interrupt mask
    `define COP0_SR_IM_MASK     8'hFF
    `define COP0_SR_IM_R        15:8
    `define COP0_SR_CU0         28 // User mode enable to COPx
    `define COP0_SR_CU_MASK     4'hF
`define COP0_CAUSE          5'd13 // Cause of last general exception
    `define COP0_CAUSE_EXC      2
    `define COP0_CAUSE_EXC_MASK 5'h1F
    `define COP0_CAUSE_EXC_R    5:2
    `define COP0_CAUSE_IP0      8
    `define COP0_CAUSE_IP_MASK  8'hFF
    `define COP0_CAUSE_IP1_0_R  9:8
    `define COP0_CAUSE_IP_R     15:8
    `define COP0_CAUSE_IV       23
    `define COP0_CAUSE_CE       28
    `define COP0_CAUSE_CE_MASK  3'h7
    `define COP0_CAUSE_BD       31
`define COP0_EPC            5'd14 // Program counter at last exception
`define COP0_BADADDR        5'd8  // Bad address value
`define COP0_PRID           5'd15 // Processor identification and revision
`define COP0_COUNT          5'd9  // Processor cycle count (Non-std)

//--------------------------------------------------------------------
// Exception Causes
//--------------------------------------------------------------------
`define EXCEPTION_W         6
`define EXCEPTION_INT       6'h20  // Interrupt
`define EXCEPTION_ADEL      6'h14  // Address error exception (load or instruction fetch)
`define EXCEPTION_ADES      6'h15  // Address error exception (store)
`define EXCEPTION_IBE       6'h16  // Bus error exception (instruction fetch)
`define EXCEPTION_DBE       6'h17  // Bus error exception (data reference: load or store)
`define EXCEPTION_SYS       6'h18  // Syscall exception
`define EXCEPTION_BP        6'h19  // Breakpoint exception
`define EXCEPTION_RI        6'h1a  // Reserved instruction exception
`define EXCEPTION_CPU       6'h1b  // Coprocessor Unusable exception 
`define EXCEPTION_OV        6'h1c  // Arithmetic Overflow exception
`define EXCEPTION_MASK      6'h10
//-----------------------------------------------------------------
//                          MPX Core
//                            V0.1
//                   github.com/ultraembedded
//                       Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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
module mpx_alu
(
    // Inputs
     input  [  3:0]  alu_op_i
    ,input  [ 31:0]  alu_a_i
    ,input  [ 31:0]  alu_b_i

    // Outputs
    ,output [ 31:0]  alu_p_o
);

//-----------------------------------------------------------------
// Includes
//-----------------------------------------------------------------
// `include "mpx_defs.v"

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

wire [31:0]     sub_res_w = alu_a_i - alu_b_i;

//-----------------------------------------------------------------
// ALU
//-----------------------------------------------------------------
always @ (alu_op_i or alu_a_i or alu_b_i or sub_res_w)
begin
    shift_right_fill_r = 16'b0;
    shift_right_1_r = 32'b0;
    shift_right_2_r = 32'b0;
    shift_right_4_r = 32'b0;
    shift_right_8_r = 32'b0;

    shift_left_1_r = 32'b0;
    shift_left_2_r = 32'b0;
    shift_left_4_r = 32'b0;
    shift_left_8_r = 32'b0;

    case (alu_op_i)
       //----------------------------------------------
       // Shift Left
       //----------------------------------------------   
       `ALU_SHIFTL :
       begin
            if (alu_b_i[0] == 1'b1)
                shift_left_1_r = {alu_a_i[30:0],1'b0};
            else
                shift_left_1_r = alu_a_i;

            if (alu_b_i[1] == 1'b1)
                shift_left_2_r = {shift_left_1_r[29:0],2'b00};
            else
                shift_left_2_r = shift_left_1_r;

            if (alu_b_i[2] == 1'b1)
                shift_left_4_r = {shift_left_2_r[27:0],4'b0000};
            else
                shift_left_4_r = shift_left_2_r;

            if (alu_b_i[3] == 1'b1)
                shift_left_8_r = {shift_left_4_r[23:0],8'b00000000};
            else
                shift_left_8_r = shift_left_4_r;

            if (alu_b_i[4] == 1'b1)
                result_r = {shift_left_8_r[15:0],16'b0000000000000000};
            else
                result_r = shift_left_8_r;
       end
       //----------------------------------------------
       // Shift Right
       //----------------------------------------------
       `ALU_SHIFTR, `ALU_SHIFTR_ARITH:
       begin
            // Arithmetic shift? Fill with 1's if MSB set
            if (alu_a_i[31] == 1'b1 && alu_op_i == `ALU_SHIFTR_ARITH)
                shift_right_fill_r = 16'b1111111111111111;
            else
                shift_right_fill_r = 16'b0000000000000000;

            if (alu_b_i[0] == 1'b1)
                shift_right_1_r = {shift_right_fill_r[31], alu_a_i[31:1]};
            else
                shift_right_1_r = alu_a_i;

            if (alu_b_i[1] == 1'b1)
                shift_right_2_r = {shift_right_fill_r[31:30], shift_right_1_r[31:2]};
            else
                shift_right_2_r = shift_right_1_r;

            if (alu_b_i[2] == 1'b1)
                shift_right_4_r = {shift_right_fill_r[31:28], shift_right_2_r[31:4]};
            else
                shift_right_4_r = shift_right_2_r;

            if (alu_b_i[3] == 1'b1)
                shift_right_8_r = {shift_right_fill_r[31:24], shift_right_4_r[31:8]};
            else
                shift_right_8_r = shift_right_4_r;

            if (alu_b_i[4] == 1'b1)
                result_r = {shift_right_fill_r[31:16], shift_right_8_r[31:16]};
            else
                result_r = shift_right_8_r;
       end       
       //----------------------------------------------
       // Arithmetic
       //----------------------------------------------
       `ALU_ADD : 
       begin
            result_r      = (alu_a_i + alu_b_i);
       end
       `ALU_SUB : 
       begin
            result_r      = sub_res_w;
       end
       //----------------------------------------------
       // Logical
       //----------------------------------------------       
       `ALU_AND : 
       begin
            result_r      = (alu_a_i & alu_b_i);
       end
       `ALU_OR  : 
       begin
            result_r      = (alu_a_i | alu_b_i);
       end
       `ALU_XOR : 
       begin
            result_r      = (alu_a_i ^ alu_b_i);
       end
       `ALU_NOR  :
       begin
            result_r      = ~(alu_a_i | alu_b_i);
       end
       //----------------------------------------------
       // Comparision
       //----------------------------------------------
       `ALU_SLTU : 
       begin
            result_r      = (alu_a_i < alu_b_i) ? 32'h1 : 32'h0;
       end
       `ALU_SLT :
       begin
            if (alu_a_i[31] != alu_b_i[31])
                result_r  = alu_a_i[31] ? 32'h1 : 32'h0;
            else
                result_r  = sub_res_w[31] ? 32'h1 : 32'h0;
       end
       `ALU_SLTE :
       begin
            if (alu_a_i == alu_b_i)
                result_r = 32'h1;
            else if (alu_a_i[31] != alu_b_i[31])
                result_r  = alu_a_i[31] ? 32'h1 : 32'h0;
            else
                result_r  = sub_res_w[31] ? 32'h1 : 32'h0;
       end
       default  : 
       begin
            result_r      = alu_a_i;
       end
    endcase
end

assign alu_p_o    = result_r;

endmodule
//-----------------------------------------------------------------
//                          MPX Core
//                            V0.1
//                   github.com/ultraembedded
//                       Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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
module mpx_cop0_regfile
(
     input           clk_i
    ,input           rst_i

    ,input [5:0]     ext_intr_i

    ,input [31:0]    exception_vector_i

    ,input [31:0]    cop0_prid_i

    ,input [5:0]     exception_i
    ,input [31:0]    exception_pc_i
    ,input [31:0]    exception_addr_i
    ,input           exception_delay_slot_i

    // COP0 read port
    ,input           cop0_ren_i
    ,input  [5:0]    cop0_raddr_i
    ,output [31:0]   cop0_rdata_o

    // COP0 write port
    ,input  [5:0]    cop0_waddr_i
    ,input  [31:0]   cop0_wdata_i

    // Mul/Div results
    ,input           muldiv_i
    ,input [31:0]    muldiv_hi_i
    ,input [31:0]    muldiv_lo_i

    ,output          cop0_branch_o
    ,output [31:0]   cop0_target_o

    // COP0 registers
    ,output          priv_o
    ,output [31:0]   status_o

    // Masked interrupt output
    ,output          interrupt_o
);

//-----------------------------------------------------------------
// Includes
//-----------------------------------------------------------------
// `include "mpx_defs.v"

//-----------------------------------------------------------------
// Registers / Wires
//-----------------------------------------------------------------
reg [31:0]  cop0_epc_q;
reg [31:0]  cop0_cause_q;
reg [31:0]  cop0_status_q;
reg [31:0]  cop0_bad_addr_q;
reg [31:0]  hi_q;
reg [31:0]  lo_q;
reg [31:0]  cop0_cycles_q;

//-----------------------------------------------------------------
// Masked Interrupts
//-----------------------------------------------------------------
reg [7:0]  irq_pending_r;
reg [7:0]  irq_masked_r;

always @ *
begin
    irq_masked_r    = 8'b0;
    irq_pending_r   = {ext_intr_i, cop0_cause_q[`COP0_CAUSE_IP1_0_R]};

    if (cop0_status_q[`COP0_SR_IEC])
        irq_masked_r    = irq_pending_r & cop0_status_q[`COP0_SR_IM_R];
    else
        irq_masked_r    = 8'b0;
end

assign interrupt_o = (|irq_masked_r);

//-----------------------------------------------------------------
// COP0 Read Port
//-----------------------------------------------------------------
reg [31:0] rdata_r;
always @ *
begin
    rdata_r = 32'b0;

    case (cop0_raddr_i)
    // HI/LO
    {1'b1, 5'd0}:          rdata_r = lo_q;
    {1'b1, 5'd1}:          rdata_r = hi_q;
    // COP0
    {1'b0, `COP0_STATUS}:  rdata_r = cop0_status_q;
    {1'b0, `COP0_CAUSE}:   rdata_r = cop0_cause_q;
    {1'b0, `COP0_EPC}:     rdata_r = cop0_epc_q;
    {1'b0, `COP0_BADADDR}: rdata_r = cop0_bad_addr_q;
    {1'b0, `COP0_COUNT}:   rdata_r = cop0_cycles_q; // TODO: Non-std
    {1'b0, `COP0_PRID}:    rdata_r = cop0_prid_i;
    default:               rdata_r = 32'b0;
    endcase
end

assign cop0_rdata_o = rdata_r;
assign priv_o      = cop0_status_q[`COP0_SR_KUC];
assign status_o    = cop0_status_q;

//-----------------------------------------------------------------
// COP0 register next state
//-----------------------------------------------------------------
reg [31:0]  cop0_epc_r;
reg [31:0]  cop0_cause_r;
reg [31:0]  cop0_bad_addr_r;
reg [31:0]  cop0_status_r;

wire is_exception_w = |(exception_i & `EXCEPTION_MASK);
wire is_interrupt_w =  (exception_i == `EXCEPTION_INT);

always @ *
begin
    cop0_epc_r      = cop0_epc_q;
    cop0_status_r   = cop0_status_q;
    cop0_cause_r    = cop0_cause_q;
    cop0_bad_addr_r = cop0_bad_addr_q;

    // Interrupts
    if (is_interrupt_w)
    begin
        // STATUS: Interrupt enable stack push
        cop0_status_r[`COP0_SR_IEO] = cop0_status_r[`COP0_SR_IEP];
        cop0_status_r[`COP0_SR_IEP] = cop0_status_r[`COP0_SR_IEC];
        cop0_status_r[`COP0_SR_IEC] = 1'b0;

        // STATUS: User mode stack push
        cop0_status_r[`COP0_SR_KUO] = cop0_status_r[`COP0_SR_KUP];
        cop0_status_r[`COP0_SR_KUP] = cop0_status_r[`COP0_SR_KUC];
        cop0_status_r[`COP0_SR_KUC] = 1'b0;

        // CAUSE: Set exception cause
        cop0_cause_r[`COP0_CAUSE_EXC_R] = exception_i[3:0]; // TODO: ???

        // CAUSE: Record if this exception was in a branch delay slot
        cop0_cause_r[`COP0_CAUSE_BD]    = exception_delay_slot_i;

        // Record fault source PC
        // TODO: Use pc_m?
        cop0_epc_r      = exception_delay_slot_i ? (exception_pc_i - 32'd4) : exception_pc_i;

        // Bad address / PC
        cop0_bad_addr_r = exception_addr_i;
    end
    // Exception - handled in machine mode
    else if (is_exception_w)
    begin
        // STATUS: Interrupt enable stack push
        cop0_status_r[`COP0_SR_IEO] = cop0_status_r[`COP0_SR_IEP];
        cop0_status_r[`COP0_SR_IEP] = cop0_status_r[`COP0_SR_IEC];
        cop0_status_r[`COP0_SR_IEC] = 1'b0;

        // STATUS: User mode stack push
        cop0_status_r[`COP0_SR_KUO] = cop0_status_r[`COP0_SR_KUP];
        cop0_status_r[`COP0_SR_KUP] = cop0_status_r[`COP0_SR_KUC];
        cop0_status_r[`COP0_SR_KUC] = 1'b0;

        // CAUSE: Set exception cause
        cop0_cause_r[`COP0_CAUSE_EXC_R] = exception_i[3:0]; // TODO: ???

        // CAUSE: Record if this exception was in a branch delay slot
        cop0_cause_r[`COP0_CAUSE_BD]    = exception_delay_slot_i;

        // Record fault source PC
        // TODO: Use pc_m?
        cop0_epc_r      = exception_delay_slot_i ? (exception_pc_i - 32'd4) : exception_pc_i;

        // Bad address / PC
        cop0_bad_addr_r = exception_addr_i;
    end
    else
    begin
        case (cop0_waddr_i)
        // COP0
        {1'b0, `COP0_STATUS}:  cop0_status_r   = cop0_wdata_i;
        {1'b0, `COP0_CAUSE}:   cop0_cause_r    = cop0_wdata_i;
        {1'b0, `COP0_EPC}:     cop0_epc_r      = cop0_wdata_i;
        {1'b0, `COP0_BADADDR}: cop0_bad_addr_r = cop0_wdata_i;
        // RFE - pop interrupt / mode stacks
        {1'b1, `COP0_STATUS}:
        begin
            // STATUS: Interrupt enable stack pop
            cop0_status_r[`COP0_SR_IEC] = cop0_status_r[`COP0_SR_IEP];
            cop0_status_r[`COP0_SR_IEP] = cop0_status_r[`COP0_SR_IEO];

            // STATUS: User mode stack pop
            cop0_status_r[`COP0_SR_KUC] = cop0_status_r[`COP0_SR_KUP];
            cop0_status_r[`COP0_SR_KUP] = cop0_status_r[`COP0_SR_KUO];
        end
        default:
            ;
        endcase
    end

    // External interrupt pending state override
    cop0_cause_r[`COP0_CAUSE_IP_R] = {ext_intr_i, cop0_cause_r[`COP0_CAUSE_IP1_0_R]};
end

//-----------------------------------------------------------------
// Sequential
//-----------------------------------------------------------------
always @ (posedge clk_i )
if (rst_i)
begin
    cop0_epc_q         <= 32'b0;
    cop0_status_q      <= 32'b0;
    cop0_cause_q       <= 32'b0;
    cop0_bad_addr_q    <= 32'b0;
    cop0_cycles_q      <= 32'b0;
end
else
begin
    cop0_epc_q         <= cop0_epc_r;
    cop0_status_q      <= cop0_status_r;
    cop0_cause_q       <= cop0_cause_r;
    cop0_bad_addr_q    <= cop0_bad_addr_r;
    cop0_cycles_q      <= cop0_cycles_q + 32'd1;
end

//-----------------------------------------------------------------
// HI/LO registers
//-----------------------------------------------------------------
always @ (posedge clk_i )
if (rst_i)
begin
    hi_q    <= 32'b0;
    lo_q    <= 32'b0;
end
else if (muldiv_i && !is_exception_w)
begin
    hi_q    <= muldiv_hi_i;
    lo_q    <= muldiv_lo_i;
end
else if (!is_exception_w && !is_interrupt_w)
begin
    if (cop0_waddr_i == {1'b1, 5'd0})
        lo_q    <= cop0_wdata_i;
    if (cop0_waddr_i == {1'b1, 5'd1})
        hi_q    <= cop0_wdata_i;
end

//-----------------------------------------------------------------
// COP0 branch (exceptions, interrupts, syscall, break)
//-----------------------------------------------------------------
reg        branch_r;
reg [31:0] branch_target_r;

always @ *
begin
    branch_r        = 1'b0;
    branch_target_r = 32'b0;

    // Interrupts
    if (is_interrupt_w)
    begin
        branch_r        = 1'b1;
        branch_target_r = exception_vector_i;
    end
    // Exception - handled in machine mode
    else if (is_exception_w)
    begin
        branch_r        = 1'b1;
        branch_target_r = exception_vector_i;
    end
end

assign cop0_branch_o = branch_r;
assign cop0_target_o = branch_target_r;

endmodule
//-----------------------------------------------------------------
//                          MPX Core
//                            V0.1
//                   github.com/ultraembedded
//                       Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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

module mpx_cop0
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter SUPPORT_MULDIV   = 1
    ,parameter COP0_PRID        = 2
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input  [  5:0]  intr_i
    ,input           opcode_valid_i
    ,input  [ 31:0]  opcode_opcode_i
    ,input  [ 31:0]  opcode_pc_i
    ,input           opcode_invalid_i
    ,input           opcode_delay_slot_i
    ,input  [  4:0]  opcode_rd_idx_i
    ,input  [  4:0]  opcode_rs_idx_i
    ,input  [  4:0]  opcode_rt_idx_i
    ,input  [ 31:0]  opcode_rs_operand_i
    ,input  [ 31:0]  opcode_rt_operand_i
    ,input           cop0_rd_ren_i
    ,input  [  5:0]  cop0_rd_raddr_i
    ,input           cop0_writeback_write_i
    ,input  [  5:0]  cop0_writeback_waddr_i
    ,input  [ 31:0]  cop0_writeback_wdata_i
    ,input  [  5:0]  cop0_writeback_exception_i
    ,input  [ 31:0]  cop0_writeback_exception_pc_i
    ,input  [ 31:0]  cop0_writeback_exception_addr_i
    ,input           cop0_writeback_delay_slot_i
    ,input           mul_result_m_valid_i
    ,input  [ 31:0]  mul_result_m_hi_i
    ,input  [ 31:0]  mul_result_m_lo_i
    ,input           div_result_valid_i
    ,input  [ 31:0]  div_result_hi_i
    ,input  [ 31:0]  div_result_lo_i
    ,input           squash_muldiv_i
    ,input  [ 31:0]  reset_vector_i
    ,input  [ 31:0]  exception_vector_i
    ,input           interrupt_inhibit_i

    // Outputs
    ,output [ 31:0]  cop0_rd_rdata_o
    ,output [  5:0]  cop0_result_x_exception_o
    ,output          branch_cop0_request_o
    ,output          branch_cop0_exception_o
    ,output [ 31:0]  branch_cop0_pc_o
    ,output          branch_cop0_priv_o
    ,output          take_interrupt_o
    ,output          iflush_o
    ,output [ 31:0]  status_o
);



//-----------------------------------------------------------------
// Includes
//-----------------------------------------------------------------
// `include "mpx_defs.v"

//-----------------------------------------------------------------
// COP0 Faults...
//-----------------------------------------------------------------
reg         cop0_fault_r;
always @ *
begin
    // TODO: Detect access fault on COP access
    cop0_fault_r     = 1'b0;
end

//-----------------------------------------------------------------
// COP0 register file
//-----------------------------------------------------------------
wire [31:0] csr_rdata_w;

wire        csr_branch_w;
wire [31:0] csr_target_w;

wire        interrupt_w;

mpx_cop0_regfile
u_regfile
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.cop0_prid_i(COP0_PRID)

    ,.ext_intr_i(intr_i)

    // Exception handler address
    ,.exception_vector_i(exception_vector_i)

    // Issue
    ,.cop0_ren_i(cop0_rd_ren_i)
    ,.cop0_raddr_i(cop0_rd_raddr_i)
    ,.cop0_rdata_o(cop0_rd_rdata_o)

    // Exception (WB)
    ,.exception_i(cop0_writeback_exception_i)
    ,.exception_pc_i(cop0_writeback_exception_pc_i)
    ,.exception_addr_i(cop0_writeback_exception_addr_i)
    ,.exception_delay_slot_i(cop0_writeback_delay_slot_i)

    // COP0 register writes (WB)
    ,.cop0_waddr_i(cop0_writeback_write_i ? cop0_writeback_waddr_i : 6'b0) // TODO
    ,.cop0_wdata_i(cop0_writeback_wdata_i)

    // Multiply / Divide result
    ,.muldiv_i((mul_result_m_valid_i | div_result_valid_i) & ~ squash_muldiv_i)
    ,.muldiv_hi_i(div_result_valid_i ? div_result_hi_i : mul_result_m_hi_i)
    ,.muldiv_lo_i(div_result_valid_i ? div_result_lo_i : mul_result_m_lo_i)

    // COP0 branches
    ,.cop0_branch_o(csr_branch_w)
    ,.cop0_target_o(csr_target_w)

    // Various COP0 registers
    ,.priv_o()
    ,.status_o(status_o)

    // Masked interrupt output
    ,.interrupt_o(interrupt_w)
);

//-----------------------------------------------------------------
// COP0 early exceptions (X)
//-----------------------------------------------------------------
reg [`EXCEPTION_W-1:0]  exception_x_q;

always @ (posedge clk_i )
if (rst_i)
begin
    exception_x_q  <= `EXCEPTION_W'b0;
end
else if (opcode_valid_i)
begin
    // (Exceptions from X) COP0 exceptions
    if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SYSCALL)
        exception_x_q  <= `EXCEPTION_SYS;
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_BREAK)
        exception_x_q  <= `EXCEPTION_BP;
    else if (opcode_invalid_i || cop0_fault_r)
        exception_x_q  <= `EXCEPTION_RI;
    else
        exception_x_q  <= `EXCEPTION_W'b0;
end
else
begin
    exception_x_q  <= `EXCEPTION_W'b0;
end

assign cop0_result_x_exception_o = exception_x_q;

//-----------------------------------------------------------------
// Interrupt launch enable
//-----------------------------------------------------------------
reg take_interrupt_q;

always @ (posedge clk_i )
if (rst_i)
    take_interrupt_q    <= 1'b0;
else
    take_interrupt_q    <= interrupt_w & ~interrupt_inhibit_i;

assign take_interrupt_o = take_interrupt_q;

//-----------------------------------------------------------------
// Instruction cache flush
//-----------------------------------------------------------------
// NOTE: Detect transition on COP0R12.16 and use this as a hook to flush the cache
wire cop0_r12b16_wr_w = cop0_writeback_write_i && cop0_writeback_waddr_i ==  {1'b0, `COP0_STATUS};
wire cop0_r12b16_w    = cop0_writeback_wdata_i[16];

reg  cop0_r12b16_q;

always @ (posedge clk_i )
if (rst_i)
    cop0_r12b16_q <= 1'b0;
else if (cop0_r12b16_wr_w)
    cop0_r12b16_q <= cop0_r12b16_w;

reg iflush_q;

always @ (posedge clk_i )
if (rst_i)
    iflush_q    <= 1'b0;
else if (cop0_r12b16_wr_w)
    iflush_q    <= cop0_r12b16_w ^ cop0_r12b16_q;
else
    iflush_q    <= 1'b0;

assign iflush_o = iflush_q;

//-----------------------------------------------------------------
// Execute - Branch operations
//-----------------------------------------------------------------
reg        branch_q;
reg [31:0] branch_target_q;
reg        reset_q;

always @ (posedge clk_i )
if (rst_i)
begin
    branch_target_q <= 32'b0;
    branch_q        <= 1'b0;
    reset_q         <= 1'b1;
end
else if (reset_q)
begin
    branch_target_q <= reset_vector_i;
    branch_q        <= 1'b1;
    reset_q         <= 1'b0;
end
else
begin
    branch_q        <= csr_branch_w;
    branch_target_q <= csr_target_w;
end

assign branch_cop0_request_o = branch_q;
assign branch_cop0_pc_o      = branch_target_q;
assign branch_cop0_priv_o    = 1'b0; // TODO: USER/KERN


endmodule
//-----------------------------------------------------------------
//                          MPX Core
//                            V0.1
//                   github.com/ultraembedded
//                       Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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

module mpx_decode
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter SUPPORT_MULDIV   = 1
    ,parameter SUPPORTED_COP    = 5
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           fetch_in_valid_i
    ,input  [ 31:0]  fetch_in_instr_i
    ,input  [ 31:0]  fetch_in_pc_i
    ,input           fetch_in_delay_slot_i
    ,input           fetch_in_fault_fetch_i
    ,input           fetch_out_accept_i

    // Outputs
    ,output          fetch_in_accept_o
    ,output          fetch_out_valid_o
    ,output [ 31:0]  fetch_out_instr_o
    ,output [ 31:0]  fetch_out_pc_o
    ,output          fetch_out_delay_slot_o
    ,output          fetch_out_fault_fetch_o
    ,output          fetch_out_instr_exec_o
    ,output          fetch_out_instr_lsu_o
    ,output          fetch_out_instr_branch_o
    ,output          fetch_out_instr_mul_o
    ,output          fetch_out_instr_div_o
    ,output          fetch_out_instr_cop0_o
    ,output          fetch_out_instr_cop0_wr_o
    ,output          fetch_out_instr_cop1_o
    ,output          fetch_out_instr_cop1_wr_o
    ,output          fetch_out_instr_cop2_o
    ,output          fetch_out_instr_cop2_wr_o
    ,output          fetch_out_instr_cop3_o
    ,output          fetch_out_instr_cop3_wr_o
    ,output          fetch_out_instr_rd_valid_o
    ,output          fetch_out_instr_rt_is_rd_o
    ,output          fetch_out_instr_lr_is_rd_o
    ,output          fetch_out_instr_invalid_o
);



// `include "mpx_defs.v"

wire        valid_w         = fetch_in_valid_i;
wire        fetch_fault_w   = fetch_in_fault_fetch_i;
wire [31:0] opcode_w        = fetch_out_instr_o;
wire        enable_muldiv_w = SUPPORT_MULDIV;
wire [3:0]  enable_cop_w    = SUPPORTED_COP[3:0];

// Move to and from HI/LO registers
wire inst_mfhi_w    = (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_MFHI) && enable_muldiv_w;
wire inst_mflo_w    = (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_MFLO) && enable_muldiv_w;
wire inst_mthi_w    = (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_MTHI) && enable_muldiv_w;
wire inst_mtlo_w    = (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_MTLO) && enable_muldiv_w;

// COPx + 24-bits of operand
wire inst_cop0_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP0 && opcode_w[25]) && enable_cop_w[0];
wire inst_cop1_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP1 && opcode_w[25]) && enable_cop_w[1];
wire inst_cop2_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP2 && opcode_w[25]) && enable_cop_w[2];
wire inst_cop3_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP3 && opcode_w[25]) && enable_cop_w[3];

// MFC, MTC, CFC, CTC (COP <-> GPR register move)
wire inst_mfc0_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP0) && (opcode_w[`OPCODE_RS_R] == 5'b00000) && enable_cop_w[0];
wire inst_cfc0_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP0) && (opcode_w[`OPCODE_RS_R] == 5'b00010) && enable_cop_w[0];
wire inst_mtc0_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP0) && (opcode_w[`OPCODE_RS_R] == 5'b00100) && enable_cop_w[0];
wire inst_ctc0_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP0) && (opcode_w[`OPCODE_RS_R] == 5'b00110) && enable_cop_w[0];
wire inst_mfc1_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP1) && (opcode_w[`OPCODE_RS_R] == 5'b00000) && enable_cop_w[1];
wire inst_cfc1_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP1) && (opcode_w[`OPCODE_RS_R] == 5'b00010) && enable_cop_w[1];
wire inst_mtc1_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP1) && (opcode_w[`OPCODE_RS_R] == 5'b00100) && enable_cop_w[1];
wire inst_ctc1_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP1) && (opcode_w[`OPCODE_RS_R] == 5'b00110) && enable_cop_w[1];
wire inst_mfc2_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP2) && (opcode_w[`OPCODE_RS_R] == 5'b00000) && enable_cop_w[2];
wire inst_cfc2_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP2) && (opcode_w[`OPCODE_RS_R] == 5'b00010) && enable_cop_w[2];
wire inst_mtc2_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP2) && (opcode_w[`OPCODE_RS_R] == 5'b00100) && enable_cop_w[2];
wire inst_ctc2_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP2) && (opcode_w[`OPCODE_RS_R] == 5'b00110) && enable_cop_w[2];
wire inst_mfc3_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP3) && (opcode_w[`OPCODE_RS_R] == 5'b00000) && enable_cop_w[3];
wire inst_cfc3_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP3) && (opcode_w[`OPCODE_RS_R] == 5'b00010) && enable_cop_w[3];
wire inst_mtc3_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP3) && (opcode_w[`OPCODE_RS_R] == 5'b00100) && enable_cop_w[3];
wire inst_ctc3_w = (opcode_w[`OPCODE_INST_R] == `INSTR_COP3) && (opcode_w[`OPCODE_RS_R] == 5'b00110) && enable_cop_w[3];

// Return from exception
wire inst_rfe_w     = inst_cop0_w && (opcode_w[`OPCODE_FUNC_R] == 6'b010000);

// Invalid instruction
wire invalid_w =    valid_w && 
                   1'b0; // TODO:

assign fetch_out_instr_invalid_o = invalid_w;

assign fetch_out_instr_rt_is_rd_o =
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_ADDI)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_ADDIU) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_SLTI)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_SLTIU) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_ANDI)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_ORI)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_XORI)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LUI)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LB)    ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LH)    ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LW)    ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LBU)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LHU)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LWL)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LWR)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_COP0)    ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_COP1)    ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_COP2)    ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_COP3);

assign fetch_out_instr_rd_valid_o =
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_ADDI)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_ADDIU) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_SLTI)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_SLTIU) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_ANDI)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_ORI)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_XORI)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LUI)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SLL)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SRL)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SRA)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SLLV)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SRLV)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SRAV)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_ADD)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_ADDU)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SUB)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SUBU)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_AND)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_OR)    ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_XOR)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_NOR)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SLT)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SLTU)  ||
                    inst_mfhi_w  ||
                    inst_mflo_w  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LB)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LH)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LW)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LBU)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LHU)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LWL)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LWR)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_J_JAL)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_JALR)     ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_BRCOND && ((opcode_w[`OPCODE_RT_R] & `INSTR_I_BRCOND_RA_MASK) == `INSTR_I_BRCOND_RA_WR)) || 
                    inst_mfc0_w ||
                    inst_mfc1_w ||
                    inst_mfc2_w ||
                    inst_mfc3_w ||
                    inst_cfc0_w ||
                    inst_cfc1_w ||
                    inst_cfc2_w ||
                    inst_cfc3_w;

// NOTE: Not currently used
assign fetch_out_instr_exec_o =
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_ADDI)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_ADDIU) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_SLTI)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_SLTIU) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_ANDI)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_ORI)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_XORI)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LUI)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SLL)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SRL)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SRA)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SLLV)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SRLV)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SRAV)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_ADD)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_ADDU)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SUB)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SUBU)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_AND)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_OR)    ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_XOR)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_NOR)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SLT)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SLTU);

assign fetch_out_instr_lsu_o =
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LB)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LH)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LW)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LBU)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LHU)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LWL)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LWR)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_SB)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_SH)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_SW)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_SWL)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_SWR)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LWC0 && enable_cop_w[0]) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LWC1 && enable_cop_w[1]) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LWC2 && enable_cop_w[2]) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LWC3 && enable_cop_w[3]) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_SWC0 && enable_cop_w[0]) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_SWC1 && enable_cop_w[1]) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_SWC2 && enable_cop_w[2]) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_SWC3 && enable_cop_w[3]);

// NOTE: Not currently used
assign fetch_out_instr_branch_o =
                    (opcode_w[`OPCODE_INST_R] == `INSTR_J_JAL)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_J_J)    ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_J_BEQ)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_J_BNE)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_J_BLEZ) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_J_BGTZ) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_JR)       ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_JALR)     ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_BRCOND);

assign fetch_out_instr_lr_is_rd_o =
                    (opcode_w[`OPCODE_INST_R] == `INSTR_J_JAL)  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_BRCOND && ((opcode_w[`OPCODE_RT_R] & `INSTR_I_BRCOND_RA_MASK) == `INSTR_I_BRCOND_RA_WR));

assign fetch_out_instr_mul_o =
                    enable_muldiv_w &&
                    ((opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_MULT) ||
                     (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_MULTU));

assign fetch_out_instr_div_o =
                    enable_muldiv_w &&
                    ((opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_DIV) ||
                     (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_DIVU));

assign fetch_out_instr_cop0_o =
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_SYSCALL) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_w[`OPCODE_FUNC_R] == `INSTR_R_BREAK)   ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_COP0) ||
                    invalid_w   ||
                    fetch_fault_w ||
                    inst_mfhi_w ||
                    inst_mflo_w ||
                    inst_mthi_w ||
                    inst_mtlo_w;

assign fetch_out_instr_cop0_wr_o =
                    inst_mtc0_w ||
                    inst_mthi_w ||
                    inst_mtlo_w ||
                    inst_rfe_w  ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LWC0 && enable_cop_w[0]);

assign fetch_out_instr_cop1_o =
                   ((opcode_w[`OPCODE_INST_R] == `INSTR_COP1) ||
                    inst_mfc1_w ||
                    inst_cfc1_w ||
                    inst_mtc1_w ||
                    inst_ctc1_w ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LWC1) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_SWC1)) && enable_cop_w[1];

assign fetch_out_instr_cop1_wr_o =
                    inst_mtc1_w ||
                    inst_ctc1_w ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LWC1 && enable_cop_w[1]);

assign fetch_out_instr_cop2_o =
                   ((opcode_w[`OPCODE_INST_R] == `INSTR_COP2) ||
                    inst_mfc2_w ||
                    inst_cfc2_w ||
                    inst_mtc2_w ||
                    inst_ctc2_w ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LWC2) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_SWC2)) && enable_cop_w[2];

assign fetch_out_instr_cop2_wr_o =
                    inst_mtc2_w ||
                    inst_ctc2_w ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LWC2 && enable_cop_w[2]);

assign fetch_out_instr_cop3_o =
                   ((opcode_w[`OPCODE_INST_R] == `INSTR_COP3) ||
                    inst_mfc3_w ||
                    inst_cfc3_w ||
                    inst_mtc3_w ||
                    inst_ctc3_w ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LWC3) ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_SWC3)) && enable_cop_w[3];

assign fetch_out_instr_cop3_wr_o =
                    inst_mtc3_w ||
                    inst_ctc3_w ||
                    (opcode_w[`OPCODE_INST_R] == `INSTR_I_LWC3 && enable_cop_w[3]);


// Outputs
assign fetch_out_valid_o        = fetch_in_valid_i;
assign fetch_out_pc_o           = fetch_in_pc_i;
assign fetch_out_instr_o        = fetch_in_instr_i;
assign fetch_out_fault_fetch_o  = fetch_in_fault_fetch_i;
assign fetch_out_delay_slot_o   = fetch_in_delay_slot_i;

assign fetch_in_accept_o        = fetch_out_accept_i;




endmodule
//-----------------------------------------------------------------
//                          MPX Core
//                            V0.1
//                   github.com/ultraembedded
//                       Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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

module mpx_divider
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           opcode_valid_i
    ,input  [ 31:0]  opcode_opcode_i
    ,input  [ 31:0]  opcode_pc_i
    ,input           opcode_invalid_i
    ,input           opcode_delay_slot_i
    ,input  [  4:0]  opcode_rd_idx_i
    ,input  [  4:0]  opcode_rs_idx_i
    ,input  [  4:0]  opcode_rt_idx_i
    ,input  [ 31:0]  opcode_rs_operand_i
    ,input  [ 31:0]  opcode_rt_operand_i

    // Outputs
    ,output          writeback_valid_o
    ,output [ 31:0]  writeback_hi_o
    ,output [ 31:0]  writeback_lo_o
);



//-------------------------------------------------------------
// Includes
//-------------------------------------------------------------
// `include "mpx_defs.v"

//-------------------------------------------------------------
// Registers / Wires
//-------------------------------------------------------------
reg          valid_q;
reg  [31:0]  wb_hi_q;
reg  [31:0]  wb_lo_q;

//-------------------------------------------------------------
// Divider
//-------------------------------------------------------------
wire inst_div_w         = opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_DIV;
wire inst_divu_w        = opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_DIVU;

wire div_rem_inst_w     = inst_div_w | inst_divu_w;

wire signed_operation_w = inst_div_w;

reg [31:0] dividend_q;
reg [62:0] divisor_q;
reg [31:0] quotient_q;
reg [31:0] q_mask_q;
reg        div_busy_q;
reg        invert_div_q;
reg        invert_mod_q;

wire div_start_w    = opcode_valid_i & div_rem_inst_w;
wire div_complete_w = !(|q_mask_q) & div_busy_q;

always @(posedge clk_i )
if (rst_i)
begin
    div_busy_q     <= 1'b0;
    dividend_q     <= 32'b0;
    divisor_q      <= 63'b0;
    invert_div_q   <= 1'b0;
    invert_mod_q   <= 1'b0;
    quotient_q     <= 32'b0;
    q_mask_q       <= 32'b0;
end
else if (div_start_w)
begin
    div_busy_q     <= 1'b1;

    if (signed_operation_w && opcode_rs_operand_i[31])
        dividend_q <= -opcode_rs_operand_i;
    else
        dividend_q <= opcode_rs_operand_i;

    if (signed_operation_w && opcode_rt_operand_i[31])
        divisor_q <= {-opcode_rt_operand_i, 31'b0};
    else
        divisor_q <= {opcode_rt_operand_i, 31'b0};

    invert_div_q   <= signed_operation_w && (opcode_rs_operand_i[31] != opcode_rt_operand_i[31]);
    invert_mod_q   <= signed_operation_w && opcode_rs_operand_i[31];
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
reg [31:0] mod_result_r;
always @ *
begin
    div_result_r = invert_div_q ? -quotient_q : quotient_q;
    mod_result_r = invert_mod_q ? -dividend_q : dividend_q;
end

always @(posedge clk_i )
if (rst_i)
    valid_q <= 1'b0;
else
    valid_q <= div_complete_w;

always @(posedge clk_i )
if (rst_i)
    wb_lo_q <= 32'b0;
else if (div_complete_w)
    wb_lo_q <= div_result_r;

always @(posedge clk_i )
if (rst_i)
    wb_hi_q <= 32'b0;
else if (div_complete_w)
    wb_hi_q <= mod_result_r;

assign writeback_valid_o = valid_q;
assign writeback_lo_o    = wb_lo_q;
assign writeback_hi_o    = wb_hi_q;



endmodule
//-----------------------------------------------------------------
//                          MPX Core
//                            V0.1
//                   github.com/ultraembedded
//                       Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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

module mpx_exec
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           opcode_valid_i
    ,input  [ 31:0]  opcode_opcode_i
    ,input  [ 31:0]  opcode_pc_i
    ,input           opcode_invalid_i
    ,input           opcode_delay_slot_i
    ,input  [  4:0]  opcode_rd_idx_i
    ,input  [  4:0]  opcode_rs_idx_i
    ,input  [  4:0]  opcode_rt_idx_i
    ,input  [ 31:0]  opcode_rs_operand_i
    ,input  [ 31:0]  opcode_rt_operand_i
    ,input           hold_i

    // Outputs
    ,output          branch_d_request_o
    ,output          branch_d_exception_o
    ,output [ 31:0]  branch_d_pc_o
    ,output          branch_d_priv_o
    ,output [ 31:0]  writeback_value_o
);



//-------------------------------------------------------------
// Includes
//-------------------------------------------------------------
// `include "mpx_defs.v"

//-------------------------------------------------------------
// Execute - ALU operations
//-------------------------------------------------------------
reg [3:0]  alu_func_r;
reg [31:0] alu_input_a_r;
reg [31:0] alu_input_b_r;

wire [4:0] rs_idx_w  = opcode_opcode_i[`OPCODE_RS_R];
wire [4:0] rt_idx_w  = opcode_opcode_i[`OPCODE_RT_R];
wire [4:0] rd_idx_w  = opcode_opcode_i[`OPCODE_RD_R];
wire [4:0] re_w      = opcode_opcode_i[`OPCODE_RE_R];

wire [15:0] imm_w    = opcode_opcode_i[`OPCODE_IMM_R];
wire [31:0] u_imm_w  = {16'b0, imm_w};
wire [31:0] s_imm_w  = {{16{imm_w[15]}}, imm_w};

wire [31:0] target_w = {4'b0, opcode_opcode_i[`OPCODE_ADDR_R], 2'b0};
wire [31:0] br_off_w = {s_imm_w[29:0], 2'b0};

always @ *
begin
    alu_func_r     = `ALU_NONE;
    alu_input_a_r  = 32'b0;
    alu_input_b_r  = 32'b0;

    // R Type
    if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SLL)
    begin
        // result = reg_rt << re
        alu_func_r     = `ALU_SHIFTL;
        alu_input_a_r  = opcode_rt_operand_i;
        alu_input_b_r  = {27'b0, re_w};
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SRL)
    begin
        // result = reg_rt >> re
        alu_func_r     = `ALU_SHIFTR;
        alu_input_a_r  = opcode_rt_operand_i;
        alu_input_b_r  = {27'b0, re_w};
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SRA)
    begin
        // result = (int)reg_rt >> re;
        alu_func_r     = `ALU_SHIFTR_ARITH;
        alu_input_a_r  = opcode_rt_operand_i;
        alu_input_b_r  = {27'b0, re_w};
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SLLV)
    begin
        // result = reg_rt << reg_rs
        alu_func_r     = `ALU_SHIFTL;
        alu_input_a_r  = opcode_rt_operand_i;
        alu_input_b_r  = opcode_rs_operand_i;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SRLV)
    begin
        // result = reg_rt >> reg_rs
        alu_func_r     = `ALU_SHIFTR;
        alu_input_a_r  = opcode_rt_operand_i;
        alu_input_b_r  = opcode_rs_operand_i;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SRAV)
    begin
        // result = (int)reg_rt >> reg_rs
        alu_func_r     = `ALU_SHIFTR_ARITH;
        alu_input_a_r  = opcode_rt_operand_i;
        alu_input_b_r  = opcode_rs_operand_i;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_JALR)
    begin
        // result   = pc_next
        alu_func_r     = `ALU_ADD;
        alu_input_a_r  = opcode_pc_i;
        alu_input_b_r  = 32'd8; // TODO: CHECK...
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_MFHI)
    begin
        // TODO:
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_MTHI)
    begin
        // TODO:
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_MFLO)
    begin
        // TODO:
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_MTLO)
    begin
        // TODO:
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_ADD)
    begin
        // result = reg_rs + reg_rt
        // TODO: Arithmetic overflow exception...
        alu_func_r     = `ALU_ADD;
        alu_input_a_r  = opcode_rs_operand_i;
        alu_input_b_r  = opcode_rt_operand_i;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_ADDU)
    begin
        // result = reg_rs + reg_rt
        alu_func_r     = `ALU_ADD;
        alu_input_a_r  = opcode_rs_operand_i;
        alu_input_b_r  = opcode_rt_operand_i;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SUB)
    begin
        // result = reg_rs - reg_rt
        // TODO: Arithmetic overflow exception...
        alu_func_r     = `ALU_SUB;
        alu_input_a_r  = opcode_rs_operand_i;
        alu_input_b_r  = opcode_rt_operand_i;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SUBU)
    begin
        // result = reg_rs - reg_rt
        alu_func_r     = `ALU_SUB;
        alu_input_a_r  = opcode_rs_operand_i;
        alu_input_b_r  = opcode_rt_operand_i;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_AND)
    begin
        // result = reg_rs & reg_rt
        alu_func_r     = `ALU_AND;
        alu_input_a_r  = opcode_rs_operand_i;
        alu_input_b_r  = opcode_rt_operand_i;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_OR)
    begin
        // result = reg_rs | reg_rt
        alu_func_r     = `ALU_OR;
        alu_input_a_r  = opcode_rs_operand_i;
        alu_input_b_r  = opcode_rt_operand_i;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_XOR)
    begin
        // result = reg_rs ^ reg_rt
        alu_func_r     = `ALU_XOR;
        alu_input_a_r  = opcode_rs_operand_i;
        alu_input_b_r  = opcode_rt_operand_i;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_NOR)
    begin
        // result = ~(reg_rs | reg_rt)
        alu_func_r     = `ALU_NOR;
        alu_input_a_r  = opcode_rs_operand_i;
        alu_input_b_r  = opcode_rt_operand_i;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SLT)
    begin
        // result = (int)reg_rs < (int)reg_rt
        alu_func_r     = `ALU_SLT;
        alu_input_a_r  = opcode_rs_operand_i;
        alu_input_b_r  = opcode_rt_operand_i;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SLTU)
    begin
        // result = reg_rs < reg_rt
        alu_func_r     = `ALU_SLTU;
        alu_input_a_r  = opcode_rs_operand_i;
        alu_input_b_r  = opcode_rt_operand_i;
    end
    // I Type
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_ADDI)
    begin
        // result = reg_rs + (signed short)imm;
        // TODO: Arithmetic overflow exception...
        alu_func_r     = `ALU_ADD;
        alu_input_a_r  = opcode_rs_operand_i;
        alu_input_b_r  = s_imm_w;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_ADDIU)
    begin
        // result = reg_rs + (signed short)imm
        alu_func_r     = `ALU_ADD;
        alu_input_a_r  = opcode_rs_operand_i;
        alu_input_b_r  = s_imm_w;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_SLTI)
    begin
        // result = (int)reg_rs < (signed short)imm
        alu_func_r     = `ALU_SLT;
        alu_input_a_r  = opcode_rs_operand_i;
        alu_input_b_r  = s_imm_w;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_SLTIU)
    begin
        // result = reg_rs < (unsigned int)(signed short)imm
        alu_func_r     = `ALU_SLTU;
        alu_input_a_r  = opcode_rs_operand_i;
        alu_input_b_r  = s_imm_w;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_ANDI)
    begin
        // result = reg_rs & imm
        alu_func_r     = `ALU_AND;
        alu_input_a_r  = opcode_rs_operand_i;
        alu_input_b_r  = u_imm_w;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_ORI)
    begin
        // result = reg_rs | imm
        alu_func_r     = `ALU_OR;
        alu_input_a_r  = opcode_rs_operand_i;
        alu_input_b_r  = u_imm_w;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_XORI)
    begin
        // result = reg_rs ^ imm
        alu_func_r     = `ALU_XOR;
        alu_input_a_r  = opcode_rs_operand_i;
        alu_input_b_r  = u_imm_w;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_LUI)
    begin
        // result = (imm << 16)
        alu_input_a_r  = {imm_w, 16'b0};
    end
    // Branch cond - write $RA
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_BRCOND && rt_idx_w[4:1] == 4'h8)
    begin
        // result = pc_next
        alu_func_r     = `ALU_ADD;
        alu_input_a_r  = opcode_pc_i;
        alu_input_b_r  = 32'd8; // TODO: CHECK...
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_J_JAL)
    begin
        // result = pc_next
        alu_func_r     = `ALU_ADD;
        alu_input_a_r  = opcode_pc_i;
        alu_input_b_r  = 32'd8; // TODO: CHECK...
    end
end


//-------------------------------------------------------------
// ALU
//-------------------------------------------------------------
wire [31:0]  alu_p_w;
mpx_alu
u_alu
(
    .alu_op_i(alu_func_r),
    .alu_a_i(alu_input_a_r),
    .alu_b_i(alu_input_b_r),
    .alu_p_o(alu_p_w)
);

//-------------------------------------------------------------
// Flop ALU output
//-------------------------------------------------------------
reg [31:0] result_q;
always @ (posedge clk_i )
if (rst_i)
    result_q  <= 32'b0;
else if (~hold_i)
    result_q <= alu_p_w;

assign writeback_value_o  = result_q;

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

//-------------------------------------------------------------
// Branch operations (decode stage)
//-------------------------------------------------------------
reg        branch_r;
reg        branch_taken_r;
reg [31:0] branch_target_r;

always @ *
begin
    branch_r        = 1'b0;
    branch_taken_r  = 1'b0;
    branch_target_r = 32'b0;

    // branch_target_r = pc_next + offset
    branch_target_r = opcode_pc_i + 32'd4 + br_off_w;

    if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_J_JAL)
    begin
        branch_r        = 1'b1;
        branch_taken_r  = 1'b1;
        branch_target_r = (opcode_pc_i & 32'hf0000000) | target_w;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_J_J)
    begin
        branch_r        = 1'b1;
        branch_taken_r  = 1'b1;
        branch_target_r = (opcode_pc_i & 32'hf0000000) | target_w;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_J_BEQ)
    begin
        // take_branch = (reg_rs == reg_rt)
        branch_r        = 1'b1;
        branch_taken_r  = (opcode_rs_operand_i == opcode_rt_operand_i);
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_J_BNE)
    begin
        // take_branch = (reg_rs != reg_rt)
        branch_r        = 1'b1;
        branch_taken_r  = (opcode_rs_operand_i != opcode_rt_operand_i);
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_J_BLEZ)
    begin
        // take_branch = ((int)reg_rs <= 0)
        branch_r        = 1'b1;
        branch_taken_r  = less_than_signed(opcode_rs_operand_i, 32'h00000001);
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_J_BGTZ)
    begin
        // take_branch = ((int)reg_rs > 0)
        branch_r        = 1'b1;
        branch_taken_r  = greater_than_signed(opcode_rs_operand_i, 32'h00000000);
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_JR)
    begin
        // pc_next  = reg_rs
        branch_r        = 1'b1;
        branch_taken_r  = 1'b1;
        branch_target_r = opcode_rs_operand_i;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_JALR)
    begin
        // pc_next  = reg_rs
        branch_r        = 1'b1;
        branch_taken_r  = 1'b1;
        branch_target_r = opcode_rs_operand_i;
    end
    else if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_BRCOND)
    begin
        // >= 0
        if (rt_idx_w[0])
        begin
            // take_branch = ((int)reg_rs >= 0)
            branch_r        = 1'b1;
            branch_taken_r  = greater_than_signed(opcode_rs_operand_i, 32'h00000000) |
                              (opcode_rs_operand_i == 32'b0);        
        end
        else
        begin
            // take_branch = ((int)reg_rs < 0)
            branch_r        = 1'b1;
            branch_taken_r  = less_than_signed(opcode_rs_operand_i, 32'h00000000);        
        end
    end
end

assign branch_d_request_o = (branch_r && opcode_valid_i && branch_taken_r);
assign branch_d_pc_o      = branch_target_r;
assign branch_d_priv_o    = 1'b0; // don't care



endmodule
//-----------------------------------------------------------------
//                          MPX Core
//                            V0.1
//                   github.com/ultraembedded
//                       Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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

module mpx_fetch
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           fetch_accept_i
    ,input           icache_accept_i
    ,input           icache_valid_i
    ,input           icache_error_i
    ,input  [ 31:0]  icache_inst_i
    ,input           fetch_invalidate_i
    ,input           branch_request_i
    ,input           branch_exception_i
    ,input  [ 31:0]  branch_pc_i
    ,input           branch_priv_i

    // Outputs
    ,output          fetch_valid_o
    ,output [ 31:0]  fetch_instr_o
    ,output [ 31:0]  fetch_pc_o
    ,output          fetch_delay_slot_o
    ,output          fetch_fault_fetch_o
    ,output          icache_rd_o
    ,output          icache_flush_o
    ,output          icache_invalidate_o
    ,output [ 31:0]  icache_pc_o
    ,output          icache_priv_o
);



//-------------------------------------------------------------
// Includes
//-------------------------------------------------------------
// `include "mpx_defs.v"

reg         active_q;
wire        fetch_valid_w;
wire        icache_busy_w;
wire        stall_w       = !fetch_accept_i || icache_busy_w || !icache_accept_i;

//-------------------------------------------------------------
// Buffered branch (required for busy I$)
//-------------------------------------------------------------
reg         branch_q;
reg         branch_excpn_q;
reg [31:0]  branch_pc_q;
reg         branch_priv_q;

always @ (posedge clk_i )
if (rst_i)
begin
    branch_q       <= 1'b0;
    branch_excpn_q <= 1'b0;
    branch_pc_q    <= 32'b0;
    branch_priv_q  <= 1'b0;
end
// Branch request
else if (branch_request_i)
begin
    branch_q       <= stall_w && active_q;
    branch_excpn_q <= stall_w && active_q && branch_exception_i;
    branch_pc_q    <= branch_pc_i;
    branch_priv_q  <= branch_priv_i;
end
else if (icache_rd_o && icache_accept_i)
begin
    branch_q       <= 1'b0;
    branch_excpn_q <= 1'b0;
    branch_pc_q    <= 32'b0;
end

//-------------------------------------------------------------
// Active flag
//-------------------------------------------------------------
always @ (posedge clk_i )
if (rst_i)
    active_q    <= 1'b0;
else if (branch_request_i)
    active_q    <= 1'b1;

//-------------------------------------------------------------
// Request tracking
//-------------------------------------------------------------
reg icache_fetch_q;
reg icache_invalidate_q;

// ICACHE fetch tracking
always @ (posedge clk_i )
if (rst_i)
    icache_fetch_q <= 1'b0;
else if (icache_rd_o && icache_accept_i)
    icache_fetch_q <= 1'b1;
else if (icache_valid_i)
    icache_fetch_q <= 1'b0;

always @ (posedge clk_i )
if (rst_i)
    icache_invalidate_q <= 1'b0;
else if (icache_invalidate_o && !icache_accept_i)
    icache_invalidate_q <= 1'b1;
else
    icache_invalidate_q <= 1'b0;

//-------------------------------------------------------------
// PC
//-------------------------------------------------------------
reg [31:0]  pc_f_q;
reg [31:0]  pc_d_q;

wire [31:0] icache_pc_w;
wire        icache_priv_w;

always @ (posedge clk_i )
if (rst_i)
    pc_f_q  <= 32'b0;
else if (branch_request_i && (!active_q || !stall_w))
    pc_f_q  <= branch_pc_i;
// Delayed branch due to stall
else if (branch_q && ~stall_w)
    pc_f_q  <= branch_pc_q;
// NPC
else if (!stall_w)
    pc_f_q  <= {icache_pc_w[31:2],2'b0} + 32'd4;

assign icache_pc_w       = pc_f_q;
assign icache_priv_w     = 1'b0;

// Last fetch address
always @ (posedge clk_i )
if (rst_i)
    pc_d_q <= 32'b0;
else if (icache_rd_o && icache_accept_i)
    pc_d_q <= icache_pc_w;

//-------------------------------------------------------------
// Branch Delay Slot
//-------------------------------------------------------------
reg delay_slot_q;

always @ (posedge clk_i )
if (rst_i)
    delay_slot_q <= 1'b0;
else if (branch_request_i && !(stall_w && active_q))
    delay_slot_q <= ~branch_exception_i;
else if (branch_q && ~stall_w)
    delay_slot_q <= ~branch_excpn_q;
else if (fetch_valid_o && fetch_accept_i)
    delay_slot_q <= 1'b0;

//-------------------------------------------------------------
// Exception Delay Slot (squashed)
//-------------------------------------------------------------
reg excpn_slot_q;

always @ (posedge clk_i )
if (rst_i)
    excpn_slot_q <= 1'b0;
else if (branch_request_i && branch_exception_i && icache_busy_w)
    excpn_slot_q <= 1'b1;
else if (branch_request_i && !stall_w)
    excpn_slot_q <= branch_exception_i & active_q;
else if (branch_q && ~stall_w)
    excpn_slot_q <= branch_excpn_q;
else if (fetch_valid_w)
    excpn_slot_q <= 1'b0;

//-------------------------------------------------------------
// Outputs
//-------------------------------------------------------------
assign icache_rd_o         = active_q & fetch_accept_i & !icache_busy_w;
assign icache_pc_o         = {icache_pc_w[31:2],2'b0};
assign icache_priv_o       = icache_priv_w;
assign icache_flush_o      = fetch_invalidate_i | icache_invalidate_q;
assign icache_invalidate_o = 1'b0;

assign icache_busy_w       =  icache_fetch_q && !icache_valid_i;

//-------------------------------------------------------------
// Response Buffer (for back-pressure from the decoder)
//-------------------------------------------------------------
reg [64:0]  skid_buffer_q;
reg         skid_valid_q;

always @ (posedge clk_i )
if (rst_i)
begin
    skid_buffer_q  <= 65'b0;
    skid_valid_q   <= 1'b0;
end
// Core is back-pressuring current response, but exception comes along
else if (fetch_valid_o && !fetch_accept_i && branch_exception_i)
begin
    skid_buffer_q  <= 65'b0;
    skid_valid_q   <= 1'b0;
end
// Instruction output back-pressured - hold in skid buffer
else if (fetch_valid_o && !fetch_accept_i)
begin
    skid_valid_q  <= 1'b1;
    skid_buffer_q <= {fetch_fault_fetch_o, fetch_pc_o, fetch_instr_o};
end
else
begin
    skid_valid_q  <= 1'b0;
    skid_buffer_q <= 65'b0;
end

assign fetch_valid_w       = (icache_valid_i || skid_valid_q);

assign fetch_valid_o       = fetch_valid_w & ~excpn_slot_q;
assign fetch_pc_o          = skid_valid_q ? skid_buffer_q[63:32] : {pc_d_q[31:2],2'b0};
assign fetch_instr_o       = (skid_valid_q ? skid_buffer_q[31:0]  : icache_inst_i) & {32{~fetch_fault_fetch_o}};
assign fetch_delay_slot_o  = delay_slot_q;

// Faults (clamp instruction to NOP to avoid odd pipeline behaviour)
assign fetch_fault_fetch_o = skid_valid_q ? skid_buffer_q[64] : icache_error_i;



endmodule
//-----------------------------------------------------------------
//                          MPX Core
//                            V0.1
//                   github.com/ultraembedded
//                       Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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

module mpx_issue
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter SUPPORT_MULDIV   = 1
    ,parameter SUPPORT_LOAD_BYPASS = 1
    ,parameter SUPPORT_MUL_BYPASS = 1
    ,parameter SUPPORT_REGFILE_XILINX = 0
    ,parameter SUPPORTED_COP    = 5
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           fetch_valid_i
    ,input  [ 31:0]  fetch_instr_i
    ,input  [ 31:0]  fetch_pc_i
    ,input           fetch_delay_slot_i
    ,input           fetch_fault_fetch_i
    ,input           fetch_instr_exec_i
    ,input           fetch_instr_lsu_i
    ,input           fetch_instr_branch_i
    ,input           fetch_instr_mul_i
    ,input           fetch_instr_div_i
    ,input           fetch_instr_cop0_i
    ,input           fetch_instr_cop0_wr_i
    ,input           fetch_instr_cop1_i
    ,input           fetch_instr_cop1_wr_i
    ,input           fetch_instr_cop2_i
    ,input           fetch_instr_cop2_wr_i
    ,input           fetch_instr_cop3_i
    ,input           fetch_instr_cop3_wr_i
    ,input           fetch_instr_rd_valid_i
    ,input           fetch_instr_rt_is_rd_i
    ,input           fetch_instr_lr_is_rd_i
    ,input           fetch_instr_invalid_i
    ,input  [ 31:0]  cop0_rd_rdata_i
    ,input           branch_d_exec_request_i
    ,input           branch_d_exec_exception_i
    ,input  [ 31:0]  branch_d_exec_pc_i
    ,input           branch_d_exec_priv_i
    ,input           branch_cop0_request_i
    ,input           branch_cop0_exception_i
    ,input  [ 31:0]  branch_cop0_pc_i
    ,input           branch_cop0_priv_i
    ,input  [ 31:0]  writeback_exec_value_i
    ,input           writeback_mem_valid_i
    ,input  [ 31:0]  writeback_mem_value_i
    ,input  [  5:0]  writeback_mem_exception_i
    ,input           writeback_mul_valid_i
    ,input  [ 31:0]  writeback_mul_hi_i
    ,input  [ 31:0]  writeback_mul_lo_i
    ,input           writeback_div_valid_i
    ,input  [ 31:0]  writeback_div_hi_i
    ,input  [ 31:0]  writeback_div_lo_i
    ,input  [  5:0]  cop0_result_x_exception_i
    ,input           lsu_stall_i
    ,input           take_interrupt_i
    ,input           cop1_accept_i
    ,input  [ 31:0]  cop1_reg_rdata_i
    ,input           cop2_accept_i
    ,input  [ 31:0]  cop2_reg_rdata_i
    ,input           cop3_accept_i
    ,input  [ 31:0]  cop3_reg_rdata_i

    // Outputs
    ,output          fetch_accept_o
    ,output          branch_request_o
    ,output          branch_exception_o
    ,output [ 31:0]  branch_pc_o
    ,output          branch_priv_o
    ,output          exec_opcode_valid_o
    ,output          lsu_opcode_valid_o
    ,output          cop0_opcode_valid_o
    ,output          mul_opcode_valid_o
    ,output          div_opcode_valid_o
    ,output [ 31:0]  opcode_opcode_o
    ,output [ 31:0]  opcode_pc_o
    ,output          opcode_invalid_o
    ,output          opcode_delay_slot_o
    ,output [  4:0]  opcode_rd_idx_o
    ,output [  4:0]  opcode_rs_idx_o
    ,output [  4:0]  opcode_rt_idx_o
    ,output [ 31:0]  opcode_rs_operand_o
    ,output [ 31:0]  opcode_rt_operand_o
    ,output [ 31:0]  lsu_opcode_opcode_o
    ,output [ 31:0]  lsu_opcode_pc_o
    ,output          lsu_opcode_invalid_o
    ,output          lsu_opcode_delay_slot_o
    ,output [  4:0]  lsu_opcode_rd_idx_o
    ,output [  4:0]  lsu_opcode_rs_idx_o
    ,output [  4:0]  lsu_opcode_rt_idx_o
    ,output [ 31:0]  lsu_opcode_rs_operand_o
    ,output [ 31:0]  lsu_opcode_rt_operand_o
    ,output [ 31:0]  lsu_opcode_rt_operand_m_o
    ,output [ 31:0]  mul_opcode_opcode_o
    ,output [ 31:0]  mul_opcode_pc_o
    ,output          mul_opcode_invalid_o
    ,output          mul_opcode_delay_slot_o
    ,output [  4:0]  mul_opcode_rd_idx_o
    ,output [  4:0]  mul_opcode_rs_idx_o
    ,output [  4:0]  mul_opcode_rt_idx_o
    ,output [ 31:0]  mul_opcode_rs_operand_o
    ,output [ 31:0]  mul_opcode_rt_operand_o
    ,output [ 31:0]  cop0_opcode_opcode_o
    ,output [ 31:0]  cop0_opcode_pc_o
    ,output          cop0_opcode_invalid_o
    ,output          cop0_opcode_delay_slot_o
    ,output [  4:0]  cop0_opcode_rd_idx_o
    ,output [  4:0]  cop0_opcode_rs_idx_o
    ,output [  4:0]  cop0_opcode_rt_idx_o
    ,output [ 31:0]  cop0_opcode_rs_operand_o
    ,output [ 31:0]  cop0_opcode_rt_operand_o
    ,output          cop0_rd_ren_o
    ,output [  5:0]  cop0_rd_raddr_o
    ,output          cop0_writeback_write_o
    ,output [  5:0]  cop0_writeback_waddr_o
    ,output [ 31:0]  cop0_writeback_wdata_o
    ,output [  5:0]  cop0_writeback_exception_o
    ,output [ 31:0]  cop0_writeback_exception_pc_o
    ,output [ 31:0]  cop0_writeback_exception_addr_o
    ,output          cop0_writeback_delay_slot_o
    ,output          exec_hold_o
    ,output          mul_hold_o
    ,output          squash_muldiv_o
    ,output          interrupt_inhibit_o
    ,output          cop1_valid_o
    ,output [ 31:0]  cop1_opcode_o
    ,output          cop1_reg_write_o
    ,output [  5:0]  cop1_reg_waddr_o
    ,output [ 31:0]  cop1_reg_wdata_o
    ,output [  5:0]  cop1_reg_raddr_o
    ,output          cop2_valid_o
    ,output [ 31:0]  cop2_opcode_o
    ,output          cop2_reg_write_o
    ,output [  5:0]  cop2_reg_waddr_o
    ,output [ 31:0]  cop2_reg_wdata_o
    ,output [  5:0]  cop2_reg_raddr_o
    ,output          cop3_valid_o
    ,output [ 31:0]  cop3_opcode_o
    ,output          cop3_reg_write_o
    ,output [  5:0]  cop3_reg_waddr_o
    ,output [ 31:0]  cop3_reg_wdata_o
    ,output [  5:0]  cop3_reg_raddr_o
);



//-------------------------------------------------------------
// Includes
//-------------------------------------------------------------
// `include "mpx_defs.v"

wire        enable_muldiv_w = SUPPORT_MULDIV;
wire [3:0]  enable_cop_w    = SUPPORTED_COP[3:0];

wire stall_w;
wire squash_w;

wire [3:0] cop_stall_w;

assign cop_stall_w[0] = 1'b0;
assign cop_stall_w[1] = enable_cop_w[1] & (fetch_instr_cop1_i & ~cop1_accept_i);
assign cop_stall_w[2] = enable_cop_w[2] & (fetch_instr_cop2_i & ~cop2_accept_i);
assign cop_stall_w[3] = enable_cop_w[3] & (fetch_instr_cop3_i & ~cop3_accept_i);

//-------------------------------------------------------------
// Priv level
//-------------------------------------------------------------
reg priv_x_q;

always @ (posedge clk_i )
if (rst_i)
    priv_x_q <= 1'b0;
else if (branch_cop0_request_i)
    priv_x_q <= branch_cop0_priv_i;

//-------------------------------------------------------------
// Branch request (exception or branch instruction)
//-------------------------------------------------------------
assign branch_request_o     = branch_cop0_request_i | branch_d_exec_request_i;
assign branch_pc_o          = branch_cop0_request_i ? branch_cop0_pc_i   : branch_d_exec_pc_i;
assign branch_priv_o        = branch_cop0_request_i ? branch_cop0_priv_i : priv_x_q;
assign branch_exception_o   = branch_cop0_request_i;

//-------------------------------------------------------------
// Instruction Input
//-------------------------------------------------------------
wire       opcode_valid_w   = fetch_valid_i & ~squash_w & ~branch_cop0_request_i;
wire [4:0] issue_rs_idx_w   = fetch_instr_i[`OPCODE_RS_R];
wire [4:0] issue_rt_idx_w   = fetch_instr_i[`OPCODE_RT_R];
wire [4:0] issue_rd_idx_w   = fetch_instr_lr_is_rd_i ? 5'd31: // $LR 
                              fetch_instr_rt_is_rd_i ? fetch_instr_i[`OPCODE_RT_R] : 
                                                       fetch_instr_i[`OPCODE_RD_R];
wire       issue_sb_alloc_w = fetch_instr_rd_valid_i;
wire       issue_lsu_w      = fetch_instr_lsu_i;
wire       issue_branch_w   = fetch_instr_branch_i;
wire       issue_mul_w      = fetch_instr_mul_i;
wire       issue_div_w      = fetch_instr_div_i;
wire       issue_cop0_w     = fetch_instr_cop0_i;
wire       issue_cop1_w     = fetch_instr_cop1_i;
wire       issue_cop2_w     = fetch_instr_cop2_i;
wire       issue_cop3_w     = fetch_instr_cop3_i;
wire       issue_invalid_w  = fetch_instr_invalid_i;

//-------------------------------------------------------------
// Pipeline status tracking
//------------------------------------------------------------- 
wire        pipe_squash_x_m_w;

reg         opcode_issue_r;
reg         opcode_accept_r;
wire        pipe_stall_raw_w;

wire        pipe_load_x_w;
wire        pipe_store_x_w;
wire        pipe_mul_x_w;
wire        pipe_branch_x_w;
wire [4:0]  pipe_rd_x_w;

wire [31:0] pipe_pc_x_w;
wire [31:0] pipe_opcode_x_w;
wire [31:0] pipe_operand_rs_x_w;
wire [31:0] pipe_operand_rt_x_w;

wire        pipe_load_m_w;
wire        pipe_mul_m_w;
wire [4:0]  pipe_rd_m_w;
wire [31:0] pipe_result_m_w;

wire        pipe_valid_wb_w;
wire        pipe_cop0_wb_w;
wire        pipe_cop1_wb_w;
wire        pipe_cop2_wb_w;
wire        pipe_cop3_wb_w;
wire [4:0]  pipe_rd_wb_w;
wire [31:0] pipe_result_wb_w;
wire [31:0] pipe_pc_wb_w;
wire [31:0] pipe_opc_wb_w;
wire [31:0] pipe_rs_val_wb_w;
wire [31:0] pipe_rt_val_wb_w;
wire        pipe_del_slot_wb_w;
wire [`EXCEPTION_W-1:0] pipe_exception_x_w;
wire [`EXCEPTION_W-1:0] pipe_exception_wb_w;

wire [`EXCEPTION_W-1:0] issue_fault_w = fetch_fault_fetch_i ? `EXCEPTION_IBE:
                                                              `EXCEPTION_W'b0;

mpx_pipe_ctrl
#( 
     .SUPPORT_LOAD_BYPASS(SUPPORT_LOAD_BYPASS)
    ,.SUPPORT_MUL_BYPASS(SUPPORT_MUL_BYPASS)
)
u_pipe_ctrl
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)    

    // Issue
    ,.issue_valid_i(opcode_issue_r)
    ,.issue_accept_i(opcode_accept_r)
    ,.issue_stall_i(stall_w)
    ,.issue_lsu_i(issue_lsu_w)
    ,.issue_cop0_i(issue_cop0_w)
    ,.issue_cop0_wr_i(fetch_instr_cop0_wr_i)
    ,.issue_cop1_i(issue_cop1_w)
    ,.issue_cop1_wr_i(fetch_instr_cop1_wr_i)
    ,.issue_cop2_i(issue_cop2_w)
    ,.issue_cop2_wr_i(fetch_instr_cop2_wr_i)
    ,.issue_cop3_i(issue_cop3_w)
    ,.issue_cop3_wr_i(fetch_instr_cop3_wr_i)
    ,.issue_div_i(issue_div_w)
    ,.issue_mul_i(issue_mul_w)
    ,.issue_branch_i(issue_branch_w)
    ,.issue_rd_valid_i(issue_sb_alloc_w)
    ,.issue_rt_is_rd_i(fetch_instr_rt_is_rd_i)
    ,.issue_rd_i(issue_rd_idx_w)
    ,.issue_exception_i(issue_fault_w)
    ,.issue_pc_i(opcode_pc_o)
    ,.issue_opcode_i(opcode_opcode_o)
    ,.issue_operand_rs_i(opcode_rs_operand_o)
    ,.issue_operand_rt_i(opcode_rt_operand_o)
    ,.issue_branch_taken_i(branch_d_exec_request_i)
    ,.issue_branch_target_i(branch_d_exec_pc_i)
    ,.issue_delay_slot_i(fetch_delay_slot_i)
    ,.take_interrupt_i(take_interrupt_i)

    // Issue: 0 cycle latency read results (COPx)
    ,.cop0_result_rdata_i(cop0_rd_rdata_i)
    ,.cop1_result_rdata_i(cop1_reg_rdata_i)
    ,.cop2_result_rdata_i(cop2_reg_rdata_i)
    ,.cop3_result_rdata_i(cop3_reg_rdata_i)

    // Execution stage 1: ALU result
    ,.alu_result_x_i(writeback_exec_value_i)
    ,.cop0_result_exception_x_i(cop0_result_x_exception_i)

    // Execution stage 1
    ,.load_x_o(pipe_load_x_w)
    ,.store_x_o(pipe_store_x_w)
    ,.mul_x_o(pipe_mul_x_w)
    ,.branch_x_o(pipe_branch_x_w)
    ,.rd_x_o(pipe_rd_x_w)
    ,.pc_x_o(pipe_pc_x_w)
    ,.opcode_x_o(pipe_opcode_x_w)
    ,.operand_rs_x_o(pipe_operand_rs_x_w)
    ,.operand_rt_x_o(pipe_operand_rt_x_w)
    ,.exception_x_o(pipe_exception_x_w)

    // Execution stage 2: Other results
    ,.mem_complete_i(writeback_mem_valid_i)
    ,.mem_result_m_i(writeback_mem_value_i)
    ,.mem_exception_m_i(writeback_mem_exception_i)

    // Execution stage 2
    ,.load_m_o(pipe_load_m_w)
    ,.mul_m_o(pipe_mul_m_w)
    ,.rd_m_o(pipe_rd_m_w)
    ,.result_m_o(pipe_result_m_w)
    ,.operand_rt_m_o(lsu_opcode_rt_operand_m_o) // (for LWL, LWR)

    ,.stall_o(pipe_stall_raw_w)
    ,.squash_x_m_o(pipe_squash_x_m_w)

    // Out of pipe: Divide Result
    ,.div_complete_i(writeback_div_valid_i)

    // Commit
    ,.valid_wb_o(pipe_valid_wb_w)
    ,.cop0_wb_o(pipe_cop0_wb_w)
    ,.cop1_wb_o(pipe_cop1_wb_w)
    ,.cop2_wb_o(pipe_cop2_wb_w)
    ,.cop3_wb_o(pipe_cop3_wb_w)
    ,.rd_wb_o(pipe_rd_wb_w)
    ,.result_wb_o(pipe_result_wb_w)
    ,.pc_wb_o(pipe_pc_wb_w)
    ,.opcode_wb_o(pipe_opc_wb_w)
    ,.operand_rs_wb_o(pipe_rs_val_wb_w)
    ,.operand_rt_wb_o(pipe_rt_val_wb_w)
    ,.exception_wb_o(pipe_exception_wb_w)
    ,.delay_slot_wb_o(pipe_del_slot_wb_w)
    
    ,.cop0_write_wb_o(cop0_writeback_write_o)
    ,.cop1_write_wb_o(cop1_reg_write_o)
    ,.cop2_write_wb_o(cop2_reg_write_o)
    ,.cop3_write_wb_o(cop3_reg_write_o)

    ,.cop0_waddr_wb_o(cop0_writeback_waddr_o)
    ,.cop0_wdata_wb_o(cop0_writeback_wdata_o)   
);

assign exec_hold_o = stall_w;
assign mul_hold_o  = stall_w;

//-------------------------------------------------------------
// Status tracking
//-------------------------------------------------------------
assign cop0_writeback_exception_o      = pipe_exception_wb_w;
assign cop0_writeback_exception_pc_o   = pipe_pc_wb_w;
assign cop0_writeback_exception_addr_o = pipe_result_wb_w;
assign cop0_writeback_delay_slot_o     = pipe_del_slot_wb_w;

//-------------------------------------------------------------
// Blocking events (division, COP0 unit access)
//-------------------------------------------------------------
reg mul_pending_q;
reg div_pending_q;
reg cop0_pending_q;
reg cop1_pending_q;
reg cop2_pending_q;
reg cop3_pending_q;

// TODO: Multiplies should be non-blocking...
always @ (posedge clk_i )
if (rst_i)
    mul_pending_q <= 1'b0;
else if (pipe_squash_x_m_w)
    mul_pending_q <= 1'b0;
else if (mul_opcode_valid_o && issue_mul_w)
    mul_pending_q <= 1'b1;
else if (writeback_mul_valid_i)
    mul_pending_q <= 1'b0;

// Division operations take 2 - 34 cycles and stall
// the pipeline (complete out-of-pipe) until completed.
always @ (posedge clk_i )
if (rst_i)
    div_pending_q <= 1'b0;
else if (pipe_squash_x_m_w)
    div_pending_q <= 1'b0;
else if (div_opcode_valid_o && issue_div_w)
    div_pending_q <= 1'b1;
else if (writeback_div_valid_i)
    div_pending_q <= 1'b0;

// COP0 operations are infrequent - avoid any complications of pipelining them.
// These only take a 2-3 cycles anyway and may result in a pipe flush (e.g. syscall, break..).
always @ (posedge clk_i )
if (rst_i)
    cop0_pending_q <= 1'b0;
else if (pipe_squash_x_m_w)
    cop0_pending_q <= 1'b0;
else if (cop0_opcode_valid_o && issue_cop0_w)
    cop0_pending_q <= 1'b1;
else if (pipe_cop0_wb_w)
    cop0_pending_q <= 1'b0;

// TODO: This is all wrong
always @ (posedge clk_i )
if (rst_i)
    cop1_pending_q <= 1'b0;
else if (pipe_squash_x_m_w)
    cop1_pending_q <= 1'b0;
else if (cop1_valid_o && issue_cop1_w)
    cop1_pending_q <= 1'b1;
else if (pipe_cop1_wb_w)
    cop1_pending_q <= 1'b0;
always @ (posedge clk_i )
if (rst_i)
    cop2_pending_q <= 1'b0;
else if (pipe_squash_x_m_w)
    cop2_pending_q <= 1'b0;
else if (cop2_valid_o && issue_cop2_w)
    cop2_pending_q <= 1'b1;
else if (pipe_cop2_wb_w)
    cop2_pending_q <= 1'b0;
always @ (posedge clk_i )
if (rst_i)
    cop3_pending_q <= 1'b0;
else if (pipe_squash_x_m_w)
    cop3_pending_q <= 1'b0;
else if (cop3_valid_o && issue_cop3_w)
    cop3_pending_q <= 1'b1;
else if (pipe_cop3_wb_w)
    cop3_pending_q <= 1'b0;

assign squash_w = pipe_squash_x_m_w;
assign squash_muldiv_o = squash_w;

//-------------------------------------------------------------
// Issue / scheduling logic
//-------------------------------------------------------------
reg [31:0] scoreboard_r;

always @ *
begin
    opcode_issue_r     = 1'b0;
    opcode_accept_r    = 1'b0;
    scoreboard_r       = 32'b0;

    // Execution units with >= 2 cycle latency
    if (SUPPORT_LOAD_BYPASS == 0)
    begin
        if (pipe_load_m_w)
            scoreboard_r[pipe_rd_m_w] = 1'b1;
    end
    if (SUPPORT_MUL_BYPASS == 0)
    begin
        if (pipe_mul_m_w)
            scoreboard_r[pipe_rd_m_w] = 1'b1;
    end

    // Execution units with >= 1 cycle latency (loads / multiply)
    if (pipe_load_x_w || pipe_mul_x_w)
        scoreboard_r[pipe_rd_x_w] = 1'b1;

    // Do not start multiply, division or COP operation in the cycle after a load (leaving only ALU operations and branches)
    if ((pipe_load_x_w || pipe_store_x_w) && (issue_mul_w || issue_div_w || issue_cop0_w || issue_cop1_w || issue_cop2_w || issue_cop3_w))
        scoreboard_r = 32'hFFFFFFFF;

    // Stall - no issues...
    if (lsu_stall_i || stall_w || div_pending_q || mul_pending_q || 
        cop0_pending_q || cop1_pending_q || cop2_pending_q || cop3_pending_q || (|cop_stall_w))
        ;
    // Handling exception
    else if (|pipe_exception_x_w)
        ;
    // Valid opcode - no hazards
    else if (opcode_valid_w &&
        !(scoreboard_r[issue_rs_idx_w] || 
          scoreboard_r[issue_rt_idx_w] ||
          scoreboard_r[issue_rd_idx_w]))
    begin
        opcode_issue_r  = 1'b1;
        opcode_accept_r = 1'b1;

        if (opcode_accept_r && issue_sb_alloc_w && (|issue_rd_idx_w))
            scoreboard_r[issue_rd_idx_w] = 1'b1;
    end 
end

assign lsu_opcode_valid_o   = opcode_issue_r & ~take_interrupt_i;
assign exec_opcode_valid_o  = opcode_issue_r;
assign mul_opcode_valid_o   = enable_muldiv_w & opcode_issue_r;
assign div_opcode_valid_o   = enable_muldiv_w & opcode_issue_r;
assign interrupt_inhibit_o  = cop0_pending_q || cop1_pending_q || cop2_pending_q || cop3_pending_q || 
                              issue_cop0_w   || issue_cop1_w   || issue_cop2_w   || issue_cop3_w;

assign fetch_accept_o       = opcode_valid_w ? (opcode_accept_r & ~take_interrupt_i) : 1'b1;

assign stall_w              = pipe_stall_raw_w;

//-------------------------------------------------------------
// Register File
//------------------------------------------------------------- 
wire [31:0] issue_rs_value_w;
wire [31:0] issue_rt_value_w;

// Register file: 1W2R
mpx_regfile
#(
     .SUPPORT_REGFILE_XILINX(SUPPORT_REGFILE_XILINX)
)
u_regfile
(
    .clk_i(clk_i),
    .rst_i(rst_i),

    // Write ports
    .rd0_i(pipe_rd_wb_w),
    .rd0_value_i(pipe_result_wb_w),

    // Read ports
    .ra0_i(issue_rs_idx_w),
    .rb0_i(issue_rt_idx_w),
    .ra0_value_o(issue_rs_value_w),
    .rb0_value_o(issue_rt_value_w)
);

//-------------------------------------------------------------
// Operand resolution (bypass logic)
//------------------------------------------------------------- 
assign opcode_opcode_o = fetch_instr_i;
assign opcode_pc_o     = fetch_pc_i;
assign opcode_rd_idx_o = issue_rd_idx_w;
assign opcode_rs_idx_o = issue_rs_idx_w;
assign opcode_rt_idx_o = issue_rt_idx_w;
assign opcode_invalid_o= 1'b0; 

reg [31:0] issue_rs_value_r;
reg [31:0] issue_rt_value_r;

always @ *
begin
    // NOTE: Newest version of operand takes priority
    issue_rs_value_r = issue_rs_value_w;
    issue_rt_value_r = issue_rt_value_w;

    // Bypass - WB
    if (pipe_rd_wb_w == issue_rs_idx_w)
        issue_rs_value_r = pipe_result_wb_w;
    if (pipe_rd_wb_w == issue_rt_idx_w)
        issue_rt_value_r = pipe_result_wb_w;

    // Bypass - M
    if (pipe_rd_m_w == issue_rs_idx_w)
        issue_rs_value_r = pipe_result_m_w;
    if (pipe_rd_m_w == issue_rt_idx_w)
        issue_rt_value_r = pipe_result_m_w;

    // Bypass - X
    if (pipe_rd_x_w == issue_rs_idx_w)
        issue_rs_value_r = writeback_exec_value_i;
    if (pipe_rd_x_w == issue_rt_idx_w)
        issue_rt_value_r = writeback_exec_value_i;

    // Reg 0 source
    if (issue_rs_idx_w == 5'b0)
        issue_rs_value_r = 32'b0;
    if (issue_rt_idx_w == 5'b0)
        issue_rt_value_r = 32'b0;
end

assign opcode_rs_operand_o = issue_rs_value_r;
assign opcode_rt_operand_o = issue_rt_value_r;
assign opcode_delay_slot_o = fetch_delay_slot_i;

//-------------------------------------------------------------
// Load store unit
//-------------------------------------------------------------
assign lsu_opcode_opcode_o      = opcode_opcode_o;
assign lsu_opcode_pc_o          = opcode_pc_o;
assign lsu_opcode_rd_idx_o      = opcode_rd_idx_o;
assign lsu_opcode_rs_idx_o      = opcode_rs_idx_o;
assign lsu_opcode_rt_idx_o      = opcode_rt_idx_o;
assign lsu_opcode_rs_operand_o  = opcode_rs_operand_o;
assign lsu_opcode_rt_operand_o  = (opcode_opcode_o[`OPCODE_INST_R] == `INSTR_I_SWC0) ? cop0_rd_rdata_i : opcode_rt_operand_o; // TODO: Horrible long paths...
assign lsu_opcode_invalid_o     = 1'b0;
assign lsu_opcode_delay_slot_o  = fetch_delay_slot_i;

//-------------------------------------------------------------
// Multiply
//-------------------------------------------------------------
assign mul_opcode_opcode_o      = opcode_opcode_o;
assign mul_opcode_pc_o          = opcode_pc_o;
assign mul_opcode_rd_idx_o      = opcode_rd_idx_o;
assign mul_opcode_rs_idx_o      = opcode_rs_idx_o;
assign mul_opcode_rt_idx_o      = opcode_rt_idx_o;
assign mul_opcode_rs_operand_o  = opcode_rs_operand_o;
assign mul_opcode_rt_operand_o  = opcode_rt_operand_o;
assign mul_opcode_invalid_o     = 1'b0;
assign mul_opcode_delay_slot_o  = fetch_delay_slot_i;

//-------------------------------------------------------------
// COP0 unit
//-------------------------------------------------------------
assign cop0_opcode_valid_o       = opcode_issue_r & ~take_interrupt_i;
assign cop0_opcode_opcode_o      = opcode_opcode_o;
assign cop0_opcode_pc_o          = opcode_pc_o;
assign cop0_opcode_rd_idx_o      = opcode_rd_idx_o;
assign cop0_opcode_rs_idx_o      = opcode_rs_idx_o;
assign cop0_opcode_rt_idx_o      = opcode_rt_idx_o;
assign cop0_opcode_rs_operand_o  = opcode_rs_operand_o;
assign cop0_opcode_rt_operand_o  = opcode_rt_operand_o;
assign cop0_opcode_invalid_o     = opcode_issue_r && issue_invalid_w;
assign cop0_opcode_delay_slot_o  = fetch_delay_slot_i;

reg [5:0]   cop_raddr_r;

always @ *
begin
    cop_raddr_r = 6'b0;

    // COP0 only (HI/LO access)
    if (opcode_opcode_o[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_o[`OPCODE_FUNC_R] == `INSTR_R_MFHI)
        cop_raddr_r = {1'b1, 5'd1};
    else if (opcode_opcode_o[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_o[`OPCODE_FUNC_R] == `INSTR_R_MTHI)
        cop_raddr_r = {1'b1, 5'd1};
    else if (opcode_opcode_o[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_o[`OPCODE_FUNC_R] == `INSTR_R_MFLO)
        cop_raddr_r = {1'b1, 5'd0};
    else if (opcode_opcode_o[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_o[`OPCODE_FUNC_R] == `INSTR_R_MTLO)
        cop_raddr_r = {1'b1, 5'd0};
    // MFCx: r0 - r31
    else if (opcode_opcode_o[31:28] == 4'b0100 && opcode_opcode_o[`OPCODE_RS_R] == 5'b00000)
        cop_raddr_r = {1'b0, opcode_opcode_o[`OPCODE_RD_R]};
    // CFCx: r32 - r63
    else if (opcode_opcode_o[31:28] == 4'b0100 && opcode_opcode_o[`OPCODE_RS_R] == 5'b00010)
        cop_raddr_r = {1'b1, opcode_opcode_o[`OPCODE_RD_R]};
    else if (opcode_opcode_o[`OPCODE_INST_R] == `INSTR_I_SWC0 || 
             opcode_opcode_o[`OPCODE_INST_R] == `INSTR_I_SWC1 ||
             opcode_opcode_o[`OPCODE_INST_R] == `INSTR_I_SWC2 ||
             opcode_opcode_o[`OPCODE_INST_R] == `INSTR_I_SWC3)
        cop_raddr_r = {1'b0, opcode_opcode_o[`OPCODE_RT_R]};
    else
        cop_raddr_r = {1'b0, opcode_opcode_o[`OPCODE_RD_R]};
end

assign cop0_rd_ren_o   = cop0_opcode_valid_o;
assign cop0_rd_raddr_o = cop_raddr_r;

//-------------------------------------------------------------
// COPx unit
//-------------------------------------------------------------
assign cop1_valid_o     = fetch_instr_cop1_i & opcode_issue_r & ~take_interrupt_i;
assign cop1_opcode_o    = opcode_opcode_o;
assign cop1_reg_raddr_o = cop0_rd_raddr_o;
assign cop1_reg_waddr_o = cop0_writeback_waddr_o;
assign cop1_reg_wdata_o = cop0_writeback_wdata_o;
assign cop2_valid_o     = fetch_instr_cop2_i & opcode_issue_r & ~take_interrupt_i;
assign cop2_opcode_o    = opcode_opcode_o;
assign cop2_reg_raddr_o = cop0_rd_raddr_o;
assign cop2_reg_waddr_o = cop0_writeback_waddr_o;
assign cop2_reg_wdata_o = cop0_writeback_wdata_o;
assign cop3_valid_o     = fetch_instr_cop3_i & opcode_issue_r & ~take_interrupt_i;
assign cop3_opcode_o    = opcode_opcode_o;
assign cop3_reg_raddr_o = cop0_rd_raddr_o;
assign cop3_reg_waddr_o = cop0_writeback_waddr_o;
assign cop3_reg_wdata_o = cop0_writeback_wdata_o;

//-------------------------------------------------------------
// Checker Interface
//-------------------------------------------------------------
`ifdef verilator
mpx_trace_sim
u_pipe_dec0_verif
(
     .valid_i(pipe_valid_wb_w)
    ,.pc_i(pipe_pc_wb_w)
    ,.opcode_i(pipe_opc_wb_w)
);

wire [4:0] v_pipe_rs_w = pipe_opc_wb_w[`OPCODE_RS_R];
wire [4:0] v_pipe_rt_w = pipe_opc_wb_w[`OPCODE_RT_R];

function [0:0] complete_valid; /*verilator public*/
begin
    complete_valid = pipe_valid_wb_w;
end
endfunction
function [31:0] complete_pc; /*verilator public*/
begin
    complete_pc = pipe_pc_wb_w;
end
endfunction
function [31:0] complete_opcode; /*verilator public*/
begin
    complete_opcode = pipe_opc_wb_w;
end
endfunction
function [4:0] complete_rs; /*verilator public*/
begin
    complete_rs = v_pipe_rs_w;
end
endfunction
function [4:0] complete_rt; /*verilator public*/
begin
    complete_rt = v_pipe_rt_w;
end
endfunction
function [4:0] complete_rd; /*verilator public*/
begin
    complete_rd = pipe_rd_wb_w;
end
endfunction
function [31:0] complete_rs_val; /*verilator public*/
begin
    complete_rs_val = pipe_rs_val_wb_w;
end
endfunction
function [31:0] complete_rt_val; /*verilator public*/
begin
    complete_rt_val = pipe_rt_val_wb_w;
end
endfunction
function [31:0] complete_rd_val; /*verilator public*/
begin
    if (|pipe_rd_wb_w)
        complete_rd_val = pipe_result_wb_w;
    else
        complete_rd_val = 32'b0;
end
endfunction
function [5:0] complete_exception; /*verilator public*/
begin
    complete_exception = pipe_exception_wb_w;
end
endfunction
`endif


endmodule
//-----------------------------------------------------------------
//                          MPX Core
//                            V0.1
//                   github.com/ultraembedded
//                       Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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

module mpx_lsu
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter MEM_CACHE_ADDR_MIN = 32'h80000000
    ,parameter MEM_CACHE_ADDR_MAX = 32'h8fffffff
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           opcode_valid_i
    ,input  [ 31:0]  opcode_opcode_i
    ,input  [ 31:0]  opcode_pc_i
    ,input           opcode_invalid_i
    ,input           opcode_delay_slot_i
    ,input  [  4:0]  opcode_rd_idx_i
    ,input  [  4:0]  opcode_rs_idx_i
    ,input  [  4:0]  opcode_rt_idx_i
    ,input  [ 31:0]  opcode_rs_operand_i
    ,input  [ 31:0]  opcode_rt_operand_i
    ,input  [ 31:0]  opcode_rt_operand_m_i
    ,input  [ 31:0]  mem_data_rd_i
    ,input           mem_accept_i
    ,input           mem_ack_i
    ,input           mem_error_i
    ,input  [ 10:0]  mem_resp_tag_i

    // Outputs
    ,output [ 31:0]  mem_addr_o
    ,output [ 31:0]  mem_data_wr_o
    ,output          mem_rd_o
    ,output [  3:0]  mem_wr_o
    ,output          mem_cacheable_o
    ,output [ 10:0]  mem_req_tag_o
    ,output          mem_invalidate_o
    ,output          mem_writeback_o
    ,output          mem_flush_o
    ,output          writeback_valid_o
    ,output [ 31:0]  writeback_value_o
    ,output [  5:0]  writeback_exception_o
    ,output          stall_o
);



//-----------------------------------------------------------------
// Includes
//-----------------------------------------------------------------
// `include "mpx_defs.v"

//-----------------------------------------------------------------
// Registers / Wires
//-----------------------------------------------------------------
reg [ 31:0]  mem_addr_q;
reg [ 31:0]  mem_data_wr_q;
reg          mem_rd_q;
reg [  3:0]  mem_wr_q;
reg          mem_cacheable_q;
reg          mem_invalidate_q;
reg          mem_writeback_q;
reg          mem_flush_q;
reg          mem_unaligned_x_q;
reg          mem_unaligned_m_q;

reg          mem_load_q;
reg          mem_xb_q;
reg          mem_xh_q;
reg          mem_ls_q;
reg          mem_xwl_q;
reg          mem_xwr_q;

//-----------------------------------------------------------------
// Outstanding Access Tracking
//-----------------------------------------------------------------
reg pending_lsu_m_q;

wire issue_lsu_x_w    = (mem_rd_o || (|mem_wr_o) || mem_writeback_o || mem_invalidate_o || mem_flush_o) && mem_accept_i;
wire complete_ok_m_w  = mem_ack_i & ~mem_error_i;
wire complete_err_m_w = mem_ack_i & mem_error_i;

always @ (posedge clk_i )
if (rst_i)
    pending_lsu_m_q <= 1'b0;
else if (issue_lsu_x_w)
    pending_lsu_m_q <= 1'b1;
else if (complete_ok_m_w || complete_err_m_w)
    pending_lsu_m_q <= 1'b0;

// Delay next instruction if outstanding response is late
wire delay_lsu_m_w = pending_lsu_m_q && !complete_ok_m_w;

//-----------------------------------------------------------------
// Dummy Ack (unaligned access / M - stage)
//-----------------------------------------------------------------
always @ (posedge clk_i )
if (rst_i)
    mem_unaligned_m_q <= 1'b0;
else
    mem_unaligned_m_q <= mem_unaligned_x_q & ~delay_lsu_m_w;

//-----------------------------------------------------------------
// Opcode decode
//-----------------------------------------------------------------
wire inst_lb_w  = (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_LB);
wire inst_lh_w  = (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_LH);
wire inst_lw_w  = (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_LW   || 
                   opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_LWC0 ||
                   opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_LWC1 ||
                   opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_LWC2 ||
                   opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_LWC3);
wire inst_lbu_w = (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_LBU);
wire inst_lhu_w = (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_LHU);
wire inst_lwl_w = (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_LWL);
wire inst_lwr_w = (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_LWR);

wire inst_sb_w  = (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_SB);
wire inst_sh_w  = (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_SH);
wire inst_sw_w  = (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_SW   || 
                   opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_SWC0 ||
                   opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_SWC1 ||
                   opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_SWC2 ||
                   opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_SWC3);
wire inst_swl_w = (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_SWL);
wire inst_swr_w = (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_I_SWR);

wire load_inst_w =  inst_lb_w  ||
                    inst_lh_w  ||
                    inst_lw_w  ||
                    inst_lbu_w ||
                    inst_lhu_w ||
                    inst_lwl_w ||
                    inst_lwr_w;

wire load_signed_inst_w = inst_lb_w  ||
                          inst_lh_w  ||
                          inst_lw_w;

wire store_inst_w = inst_sb_w  ||
                    inst_sh_w  ||
                    inst_sw_w  ||
                    inst_swl_w ||
                    inst_swr_w;

wire req_lb_w = inst_lb_w || inst_lbu_w;
wire req_lh_w = inst_lh_w || inst_lhu_w;
wire req_sb_w = inst_sb_w;
wire req_sh_w = inst_sh_w;
wire req_sw_w = inst_sw_w;

wire req_sw_lw_w = inst_lw_w | req_sw_w;
wire req_sh_lh_w = req_lh_w | req_sh_w;

wire [15:0] imm_w    = opcode_opcode_i[`OPCODE_IMM_R];
wire [31:0] s_imm_w  = {{16{imm_w[15]}}, imm_w};

reg [31:0]  mem_addr_r;
reg         mem_unaligned_r;
reg [31:0]  mem_data_r;
reg         mem_rd_r;
reg [3:0]   mem_wr_r;

always @ *
begin
    mem_addr_r      = opcode_rs_operand_i + s_imm_w;
    mem_data_r      = 32'b0;
    mem_unaligned_r = 1'b0;
    mem_wr_r        = 4'b0;
    mem_rd_r        = 1'b0;

    if (opcode_valid_i && (inst_lwl_w || inst_lwr_w))
        mem_unaligned_r = 1'b0;
    else if (opcode_valid_i && (inst_swl_w || inst_swr_w))
        mem_unaligned_r = 1'b0;
    else if (opcode_valid_i && req_sw_lw_w)
        mem_unaligned_r = (mem_addr_r[1:0] != 2'b0);
    else if (opcode_valid_i && req_sh_lh_w)
        mem_unaligned_r = mem_addr_r[0];

    mem_rd_r = (opcode_valid_i && load_inst_w && !mem_unaligned_r);

    // SWL
    if (opcode_valid_i && inst_swl_w)
    begin
        case (mem_addr_r[1:0])
        2'h0 :
        begin
            mem_data_r  = {24'b0, opcode_rt_operand_i[31:24]};
            mem_wr_r    = 4'b0001;
        end
        2'h1 :
        begin
            mem_data_r  = {16'b0, opcode_rt_operand_i[31:16]};
            mem_wr_r    = 4'b0011;
        end
        2'h2 :
        begin
            mem_data_r  = {8'b0, opcode_rt_operand_i[31:8]};
            mem_wr_r    = 4'b0111;
        end
        default :
        begin
            mem_data_r  = opcode_rt_operand_i;
            mem_wr_r    = 4'b1111;
        end
        endcase
    end
    // SWR
    else if (opcode_valid_i && inst_swr_w)
    begin
        case (mem_addr_r[1:0])
        2'h0 :
        begin
            mem_data_r  = opcode_rt_operand_i;
            mem_wr_r    = 4'b1111;
        end
        2'h1 :
        begin
            mem_data_r  = {opcode_rt_operand_i[23:0],8'b0};
            mem_wr_r    = 4'b1110;
        end
        2'h2 :
        begin
            mem_data_r  = {opcode_rt_operand_i[15:0],16'b0};
            mem_wr_r    = 4'b1100;
        end
        default :
        begin
            mem_data_r  = {opcode_rt_operand_i[7:0],24'b0};
            mem_wr_r    = 4'b1000;
        end
        endcase
    end
    // SW
    else if (opcode_valid_i && req_sw_w && !mem_unaligned_r)
    begin
        mem_data_r  = opcode_rt_operand_i;
        mem_wr_r    = 4'hF;
    end
    // SH
    else if (opcode_valid_i && req_sh_w && !mem_unaligned_r)
    begin
        case (mem_addr_r[1:0])
        2'h2 :
        begin
            mem_data_r  = {opcode_rt_operand_i[15:0],16'h0000};
            mem_wr_r    = 4'b1100;
        end
        default :
        begin
            mem_data_r  = {16'h0000,opcode_rt_operand_i[15:0]};
            mem_wr_r    = 4'b0011;
        end
        endcase
    end
    // SB
    else if (opcode_valid_i && req_sb_w)
    begin
        case (mem_addr_r[1:0])
        2'h3 :
        begin
            mem_data_r  = {opcode_rt_operand_i[7:0],24'h000000};
            mem_wr_r    = 4'b1000;
        end
        2'h2 :
        begin
            mem_data_r  = {{8'h00,opcode_rt_operand_i[7:0]},16'h0000};
            mem_wr_r    = 4'b0100;
        end
        2'h1 :
        begin
            mem_data_r  = {{16'h0000,opcode_rt_operand_i[7:0]},8'h00};
            mem_wr_r    = 4'b0010;
        end
        2'h0 :
        begin
            mem_data_r  = {24'h000000,opcode_rt_operand_i[7:0]};
            mem_wr_r    = 4'b0001;
        end
        default :
            ;
        endcase
    end
    else
        mem_wr_r    = 4'b0;
end

//-----------------------------------------------------------------
// Sequential
//-----------------------------------------------------------------
always @ (posedge clk_i )
if (rst_i)
begin
    mem_addr_q         <= 32'b0;
    mem_data_wr_q      <= 32'b0;
    mem_rd_q           <= 1'b0;
    mem_wr_q           <= 4'b0;
    mem_cacheable_q    <= 1'b0;
    mem_invalidate_q   <= 1'b0;
    mem_writeback_q    <= 1'b0;
    mem_flush_q        <= 1'b0;
    mem_unaligned_x_q <= 1'b0;
    mem_load_q         <= 1'b0;
    mem_xb_q           <= 1'b0;
    mem_xh_q           <= 1'b0;
    mem_ls_q           <= 1'b0;
    mem_xwl_q          <= 1'b0;
    mem_xwr_q          <= 1'b0;
end
// Memory access fault - squash next operation (exception coming...)
else if (complete_err_m_w || mem_unaligned_m_q)
begin
    mem_addr_q         <= 32'b0;
    mem_data_wr_q      <= 32'b0;
    mem_rd_q           <= 1'b0;
    mem_wr_q           <= 4'b0;
    mem_cacheable_q    <= 1'b0;
    mem_invalidate_q   <= 1'b0;
    mem_writeback_q    <= 1'b0;
    mem_flush_q        <= 1'b0;
    mem_unaligned_x_q <= 1'b0;
    mem_load_q         <= 1'b0;
    mem_xb_q           <= 1'b0;
    mem_xh_q           <= 1'b0;
    mem_ls_q           <= 1'b0;
    mem_xwl_q          <= 1'b0;
    mem_xwr_q          <= 1'b0;    
end
else if ((mem_rd_q || (|mem_wr_q) || mem_unaligned_x_q) && delay_lsu_m_w)
    ;
else if (!((mem_writeback_o || mem_invalidate_o || mem_flush_o || mem_rd_o || mem_wr_o != 4'b0) && !mem_accept_i))
begin
    mem_addr_q         <= 32'b0;
    mem_data_wr_q      <= mem_data_r;
    mem_rd_q           <= mem_rd_r;
    mem_wr_q           <= mem_wr_r;
    mem_cacheable_q    <= 1'b0;
    mem_invalidate_q   <= 1'b0;
    mem_writeback_q    <= 1'b0;
    mem_flush_q        <= 1'b0;
    mem_unaligned_x_q <= mem_unaligned_r;
    mem_load_q         <= opcode_valid_i && load_inst_w;
    mem_xb_q           <= req_lb_w | req_sb_w;
    mem_xh_q           <= req_lh_w | req_sh_w;
    mem_ls_q           <= load_signed_inst_w;
    mem_xwl_q          <= inst_lwl_w | inst_swl_w;
    mem_xwr_q          <= inst_lwr_w | inst_swr_w;

/* verilator lint_off UNSIGNED */
/* verilator lint_off CMPCONST */
    mem_cacheable_q  <= (mem_addr_r >= MEM_CACHE_ADDR_MIN && mem_addr_r <= MEM_CACHE_ADDR_MAX);
/* verilator lint_on CMPCONST */
/* verilator lint_on UNSIGNED */

    mem_invalidate_q <= 1'b0;
    mem_writeback_q  <= 1'b0;
    mem_flush_q      <= 1'b0;
    mem_addr_q       <= mem_addr_r;
end

assign mem_addr_o       = {mem_addr_q[31:2], 2'b0};
assign mem_data_wr_o    = mem_data_wr_q;
assign mem_rd_o         = mem_rd_q & ~delay_lsu_m_w;
assign mem_wr_o         = mem_wr_q & ~{4{delay_lsu_m_w}};
assign mem_cacheable_o  = mem_cacheable_q;
assign mem_req_tag_o    = 11'b0;
assign mem_invalidate_o = mem_invalidate_q;
assign mem_writeback_o  = mem_writeback_q;
assign mem_flush_o      = mem_flush_q;

// Stall upstream if cache is busy
assign stall_o          = ((mem_writeback_o || mem_invalidate_o || mem_flush_o || mem_rd_o || mem_wr_o != 4'b0) && !mem_accept_i) || delay_lsu_m_w || mem_unaligned_x_q;

wire        resp_load_w;
wire [31:0] resp_addr_w;
wire        resp_byte_w;
wire        resp_half_w;
wire        resp_signed_w;
wire        resp_xwl_w;
wire        resp_xwr_w;

mpx_lsu_fifo
#(
     .WIDTH(38)
    ,.DEPTH(2)
    ,.ADDR_W(1)
)
u_lsu_request
(
     .clk_i(clk_i)
    ,.rst_i(rst_i)

    ,.push_i(((mem_rd_o || (|mem_wr_o) || mem_writeback_o || mem_invalidate_o || mem_flush_o) && mem_accept_i) || (mem_unaligned_x_q && ~delay_lsu_m_w))
    ,.data_in_i({mem_addr_q, mem_xwl_q, mem_xwr_q, mem_ls_q, mem_xh_q, mem_xb_q, mem_load_q})
    ,.accept_o()

    ,.valid_o()
    ,.data_out_o({resp_addr_w, resp_xwl_w, resp_xwr_w, resp_signed_w, resp_half_w, resp_byte_w, resp_load_w})
    ,.pop_i(mem_ack_i || mem_unaligned_m_q)
);

//-----------------------------------------------------------------
// Load response
//-----------------------------------------------------------------
reg [1:0]  addr_lsb_r;
reg        load_byte_r;
reg        load_half_r;
reg        load_signed_r;
reg [31:0] wb_result_r;

always @ *
begin
    wb_result_r   = 32'b0;

    // Tag associated with load
    addr_lsb_r    = resp_addr_w[1:0];
    load_byte_r   = resp_byte_w;
    load_half_r   = resp_half_w;
    load_signed_r = resp_signed_w;

    // Access fault - pass badaddr on writeback result bus
    if ((mem_ack_i && mem_error_i) || mem_unaligned_m_q)
        wb_result_r = resp_addr_w;
    // Handle responses
    else if (mem_ack_i && resp_load_w)
    begin
        // LWL
        if (resp_xwl_w)
        begin
            case (addr_lsb_r[1:0])
            2'h0: wb_result_r = {mem_data_rd_i[7:0],  opcode_rt_operand_m_i[23:0]};
            2'h1: wb_result_r = {mem_data_rd_i[15:0], opcode_rt_operand_m_i[15:0]};
            2'h2: wb_result_r = {mem_data_rd_i[23:0], opcode_rt_operand_m_i[7:0]};
            2'h3: wb_result_r = mem_data_rd_i;
            endcase
        end
        // LWR
        else if (resp_xwr_w)
        begin
            case (addr_lsb_r[1:0])
            2'h0: wb_result_r = mem_data_rd_i;
            2'h1: wb_result_r = {opcode_rt_operand_m_i[31:24], mem_data_rd_i[31:8]};
            2'h2: wb_result_r = {opcode_rt_operand_m_i[31:16], mem_data_rd_i[31:16]};
            2'h3: wb_result_r = {opcode_rt_operand_m_i[31:8],  mem_data_rd_i[31:24]};
            endcase
        end
        else if (load_byte_r)
        begin
            case (addr_lsb_r[1:0])
            2'h3: wb_result_r = {24'b0, mem_data_rd_i[31:24]};
            2'h2: wb_result_r = {24'b0, mem_data_rd_i[23:16]};
            2'h1: wb_result_r = {24'b0, mem_data_rd_i[15:8]};
            2'h0: wb_result_r = {24'b0, mem_data_rd_i[7:0]};
            endcase

            if (load_signed_r && wb_result_r[7])
                wb_result_r = {24'hFFFFFF, wb_result_r[7:0]};
        end
        else if (load_half_r)
        begin
            if (addr_lsb_r[1])
                wb_result_r = {16'b0, mem_data_rd_i[31:16]};
            else
                wb_result_r = {16'b0, mem_data_rd_i[15:0]};

            if (load_signed_r && wb_result_r[15])
                wb_result_r = {16'hFFFF, wb_result_r[15:0]};
        end
        else
            wb_result_r = mem_data_rd_i;
    end
end

assign writeback_valid_o    = mem_ack_i | mem_unaligned_m_q;
assign writeback_value_o    = wb_result_r;

wire fault_load_align_w     = mem_unaligned_m_q & resp_load_w;
wire fault_store_align_w    = mem_unaligned_m_q & ~resp_load_w;

assign writeback_exception_o         = fault_load_align_w  ? `EXCEPTION_ADEL:
                                       fault_store_align_w ? `EXCEPTION_ADES:
                                       mem_error_i         ? `EXCEPTION_DBE:
                                                             `EXCEPTION_W'b0;

endmodule 

module mpx_lsu_fifo
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
    parameter WIDTH   = 8,
    parameter DEPTH   = 4,
    parameter ADDR_W  = 2
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input               clk_i
    ,input               rst_i
    ,input  [WIDTH-1:0]  data_in_i
    ,input               push_i
    ,input               pop_i

    // Outputs
    ,output [WIDTH-1:0]  data_out_o
    ,output              accept_o
    ,output              valid_o
);

//-----------------------------------------------------------------
// Local Params
//-----------------------------------------------------------------
localparam COUNT_W = ADDR_W + 1;

//-----------------------------------------------------------------
// Registers
//-----------------------------------------------------------------
reg [WIDTH-1:0]   ram_q[DEPTH-1:0];
reg [ADDR_W-1:0]  rd_ptr_q;
reg [ADDR_W-1:0]  wr_ptr_q;
reg [COUNT_W-1:0] count_q;

integer i;

//-----------------------------------------------------------------
// Sequential
//-----------------------------------------------------------------
always @ (posedge clk_i )
if (rst_i)
begin
    count_q   <= {(COUNT_W) {1'b0}};
    rd_ptr_q  <= {(ADDR_W) {1'b0}};
    wr_ptr_q  <= {(ADDR_W) {1'b0}};

    for (i=0;i<DEPTH;i=i+1)
    begin
        ram_q[i] <= {(WIDTH) {1'b0}};
    end
end
else
begin
    // Push
    if (push_i & accept_o)
    begin
        ram_q[wr_ptr_q] <= data_in_i;
        wr_ptr_q        <= wr_ptr_q + 1;
    end

    // Pop
    if (pop_i & valid_o)
        rd_ptr_q      <= rd_ptr_q + 1;

    // Count up
    if ((push_i & accept_o) & ~(pop_i & valid_o))
        count_q <= count_q + 1;
    // Count down
    else if (~(push_i & accept_o) & (pop_i & valid_o))
        count_q <= count_q - 1;
end

//-------------------------------------------------------------------
// Combinatorial
//-------------------------------------------------------------------
/* verilator lint_off WIDTH */
assign valid_o       = (count_q != 0);
assign accept_o      = (count_q != DEPTH);
/* verilator lint_on WIDTH */

assign data_out_o    = ram_q[rd_ptr_q];



endmodule
//-----------------------------------------------------------------
//                          MPX Core
//                            V0.1
//                   github.com/ultraembedded
//                       Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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

module mpx_multiplier
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input           opcode_valid_i
    ,input  [ 31:0]  opcode_opcode_i
    ,input  [ 31:0]  opcode_pc_i
    ,input           opcode_invalid_i
    ,input           opcode_delay_slot_i
    ,input  [  4:0]  opcode_rd_idx_i
    ,input  [  4:0]  opcode_rs_idx_i
    ,input  [  4:0]  opcode_rt_idx_i
    ,input  [ 31:0]  opcode_rs_operand_i
    ,input  [ 31:0]  opcode_rt_operand_i
    ,input           hold_i

    // Outputs
    ,output          writeback_valid_o
    ,output [ 31:0]  writeback_hi_o
    ,output [ 31:0]  writeback_lo_o
);



//-------------------------------------------------------------
// Includes
//-------------------------------------------------------------
// `include "mpx_defs.v"

localparam MULT_STAGES = 2; // 2 or 3

//-------------------------------------------------------------
// Registers / Wires
//-------------------------------------------------------------
reg          valid_x_q;
reg          valid_m_q;
reg          valid_e3_q;

reg [63:0]   result_m_q;
reg [63:0]   result_e3_q;

reg [32:0]   operand_a_x_q;
reg [32:0]   operand_b_x_q;

//-------------------------------------------------------------
// Multiplier
//-------------------------------------------------------------
wire [64:0]  mult_result_w;
reg  [32:0]  operand_b_r;
reg  [32:0]  operand_a_r;

wire mult_inst_w    = (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_MULT) || 
                      (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_MULTU);

always @ *
begin
    if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_MULT)
        operand_a_r = {opcode_rs_operand_i[31], opcode_rs_operand_i[31:0]};
    else // MULTU
        operand_a_r = {1'b0, opcode_rs_operand_i[31:0]};
end

always @ *
begin
    if (opcode_opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_opcode_i[`OPCODE_FUNC_R] == `INSTR_R_MULT)
        operand_b_r = {opcode_rt_operand_i[31], opcode_rt_operand_i[31:0]};
    else // MULTU
        operand_b_r = {1'b0, opcode_rt_operand_i[31:0]};
end

// Pipeline flops for multiplier
always @(posedge clk_i )
if (rst_i)
begin
    valid_x_q     <= 1'b0;
    operand_a_x_q <= 33'b0;
    operand_b_x_q <= 33'b0;
end
else if (hold_i)
    ;
else if (opcode_valid_i && mult_inst_w)
begin
    valid_x_q     <= 1'b1;
    operand_a_x_q <= operand_a_r;
    operand_b_x_q <= operand_b_r;
end
else
begin
    valid_x_q     <= 1'b0;
    operand_a_x_q <= 33'b0;
    operand_b_x_q <= 33'b0;
end

assign mult_result_w = {{ 32 {operand_a_x_q[32]}}, operand_a_x_q}*{{ 32 {operand_b_x_q[32]}}, operand_b_x_q};

always @(posedge clk_i )
if (rst_i)
    valid_m_q <= 1'b0;
else if (~hold_i)
    valid_m_q <= valid_x_q;

always @(posedge clk_i )
if (rst_i)
    valid_e3_q <= 1'b0;
else if (~hold_i)
    valid_e3_q <= valid_m_q;

always @(posedge clk_i )
if (rst_i)
    result_m_q <= 64'b0;
else if (~hold_i)
    result_m_q <= mult_result_w[63:0];

always @(posedge clk_i )
if (rst_i)
    result_e3_q <= 64'b0;
else if (~hold_i)
    result_e3_q <= result_m_q;

assign writeback_valid_o = (MULT_STAGES == 3) ? valid_e3_q : valid_m_q;
assign writeback_lo_o    = (MULT_STAGES == 3) ? result_e3_q[31:0] : result_m_q[31:0];
assign writeback_hi_o    = (MULT_STAGES == 3) ? result_e3_q[63:32] : result_m_q[63:32];


endmodule
//-----------------------------------------------------------------
//                          MPX Core
//                            V0.1
//                   github.com/ultraembedded
//                       Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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
module mpx_pipe_ctrl
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter SUPPORT_LOAD_BYPASS = 1
    ,parameter SUPPORT_MUL_BYPASS  = 1
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
     input           clk_i
    ,input           rst_i

    // Issue
    ,input           issue_valid_i
    ,input           issue_accept_i
    ,input           issue_stall_i
    ,input           issue_lsu_i
    ,input           issue_cop0_i
    ,input           issue_cop0_wr_i
    ,input           issue_cop1_i
    ,input           issue_cop1_wr_i
    ,input           issue_cop2_i
    ,input           issue_cop2_wr_i
    ,input           issue_cop3_i
    ,input           issue_cop3_wr_i
    ,input           issue_div_i
    ,input           issue_mul_i
    ,input           issue_branch_i
    ,input           issue_rd_valid_i
    ,input           issue_rt_is_rd_i
    ,input  [4:0]    issue_rd_i
    ,input  [5:0]    issue_exception_i
    ,input           take_interrupt_i
    ,input           issue_branch_taken_i
    ,input [31:0]    issue_branch_target_i
    ,input [31:0]    issue_pc_i
    ,input [31:0]    issue_opcode_i
    ,input [31:0]    issue_operand_rs_i
    ,input [31:0]    issue_operand_rt_i
    ,input           issue_delay_slot_i

    // Issue (combinatorial reads)
    ,input [31:0]    cop0_result_rdata_i
    ,input [31:0]    cop1_result_rdata_i
    ,input [31:0]    cop2_result_rdata_i
    ,input [31:0]    cop3_result_rdata_i

    // Execution stage (X): ALU result
    ,input [31:0]    alu_result_x_i

    // Execution stage (X): COP0 early exceptions
    ,input [  5:0]   cop0_result_exception_x_i

    // Execution stage (X)
    ,output          load_x_o
    ,output          store_x_o
    ,output          mul_x_o
    ,output          branch_x_o
    ,output [  4:0]  rd_x_o
    ,output [31:0]   pc_x_o
    ,output [31:0]   opcode_x_o
    ,output [31:0]   operand_rs_x_o
    ,output [31:0]   operand_rt_x_o
    ,output [ 5:0]   exception_x_o

    // Memory stage (M): Other results
    ,input           mem_complete_i
    ,input [31:0]    mem_result_m_i
    ,input  [5:0]    mem_exception_m_i

    // Memory stage (M)
    ,output          load_m_o
    ,output          mul_m_o
    ,output [  4:0]  rd_m_o
    ,output [31:0]   result_m_o
    ,output [31:0]   operand_rt_m_o

    // Out of pipe: Divide Result
    ,input           div_complete_i

    // Writeback stage (W)
    ,output          valid_wb_o
    ,output          cop0_wb_o
    ,output          cop1_wb_o
    ,output          cop2_wb_o
    ,output          cop3_wb_o
    ,output [  4:0]  rd_wb_o
    ,output [31:0]   result_wb_o
    ,output [31:0]   pc_wb_o
    ,output [31:0]   opcode_wb_o
    ,output [31:0]   operand_rs_wb_o
    ,output [31:0]   operand_rt_wb_o
    ,output [5:0]    exception_wb_o
    ,output          delay_slot_wb_o

    ,output          cop0_write_wb_o
    ,output          cop1_write_wb_o
    ,output          cop2_write_wb_o
    ,output          cop3_write_wb_o
    ,output [5:0]    cop0_waddr_wb_o
    ,output [31:0]   cop0_wdata_wb_o

    ,output          stall_o
    ,output          squash_x_m_o
);

//-------------------------------------------------------------
// Includes
//-------------------------------------------------------------
// `include "mpx_defs.v"

wire squash_x_m_w;
wire branch_misaligned_w = (issue_branch_taken_i && issue_branch_target_i[1:0] != 2'b0);

//-------------------------------------------------------------
// X-stage
//------------------------------------------------------------- 
`define PCINFO_W     23
`define PCINFO_ALU       0
`define PCINFO_LOAD      1
`define PCINFO_STORE     2
`define PCINFO_DEL_SLOT  3
`define PCINFO_DIV       4
`define PCINFO_MUL       5
`define PCINFO_BRANCH    6
`define PCINFO_RD_VALID  7
`define PCINFO_RD        12:8
`define PCINFO_INTR      13
`define PCINFO_COMPLETE  14
`define PCINFO_COP0      15
`define PCINFO_COP0_WR   16
`define PCINFO_COP1      17
`define PCINFO_COP1_WR   18
`define PCINFO_COP2      19
`define PCINFO_COP2_WR   20
`define PCINFO_COP3      21
`define PCINFO_COP3_WR   22

reg                     valid_x_q;
reg [`PCINFO_W-1:0]     ctrl_x_q;
reg [31:0]              pc_x_q;
reg [31:0]              npc_x_q;
reg [31:0]              opcode_x_q;
reg [31:0]              operand_rs_x_q;
reg [31:0]              operand_rt_x_q;
reg [31:0]              result_x_q;
reg [`EXCEPTION_W-1:0]  exception_x_q;

always @ (posedge clk_i )
if (rst_i)
begin
    valid_x_q      <= 1'b0;
    ctrl_x_q       <= `PCINFO_W'b0;
    pc_x_q         <= 32'b0;
    npc_x_q        <= 32'b0;
    opcode_x_q     <= 32'b0;
    operand_rs_x_q <= 32'b0;
    operand_rt_x_q <= 32'b0;
    result_x_q     <= 32'b0;
    exception_x_q  <= `EXCEPTION_W'b0;
end
// Stall - no change in X state
else if (issue_stall_i)
    ;
else if ((issue_valid_i && issue_accept_i) && ~(squash_x_m_o))
begin
    valid_x_q                  <= 1'b1;
    ctrl_x_q[`PCINFO_ALU]      <= ~(issue_lsu_i | issue_cop0_i | issue_div_i | issue_mul_i);
    ctrl_x_q[`PCINFO_LOAD]     <= issue_lsu_i      &  issue_rd_valid_i & ~take_interrupt_i; // TODO: Check
    ctrl_x_q[`PCINFO_STORE]    <= issue_lsu_i      & ~issue_rd_valid_i & ~take_interrupt_i;
    ctrl_x_q[`PCINFO_COP0]     <= issue_cop0_i     & ~take_interrupt_i;
    ctrl_x_q[`PCINFO_COP0_WR]  <= issue_cop0_wr_i  & ~take_interrupt_i;
    ctrl_x_q[`PCINFO_COP1]     <= issue_cop1_i     & ~take_interrupt_i;
    ctrl_x_q[`PCINFO_COP1_WR]  <= issue_cop1_wr_i  & ~take_interrupt_i;
    ctrl_x_q[`PCINFO_COP2]     <= issue_cop2_i     & ~take_interrupt_i;
    ctrl_x_q[`PCINFO_COP2_WR]  <= issue_cop2_wr_i  & ~take_interrupt_i;
    ctrl_x_q[`PCINFO_COP3]     <= issue_cop3_i     & ~take_interrupt_i;
    ctrl_x_q[`PCINFO_COP3_WR]  <= issue_cop3_wr_i  & ~take_interrupt_i;
    ctrl_x_q[`PCINFO_DIV]      <= issue_div_i      & ~take_interrupt_i;
    ctrl_x_q[`PCINFO_MUL]      <= issue_mul_i      & ~take_interrupt_i;
    ctrl_x_q[`PCINFO_BRANCH]   <= issue_branch_i   & ~take_interrupt_i;
    ctrl_x_q[`PCINFO_RD_VALID] <= issue_rd_valid_i & ~take_interrupt_i;
    ctrl_x_q[`PCINFO_RD]       <= issue_rd_i;
    ctrl_x_q[`PCINFO_DEL_SLOT] <= issue_delay_slot_i;
    ctrl_x_q[`PCINFO_INTR]     <= take_interrupt_i;
    ctrl_x_q[`PCINFO_COMPLETE] <= 1'b1;

    pc_x_q         <= issue_pc_i;
    npc_x_q        <= issue_branch_taken_i ? issue_branch_target_i : issue_pc_i + 32'd4;
    opcode_x_q     <= issue_opcode_i;
    operand_rs_x_q <= issue_operand_rs_i;
    operand_rt_x_q <= issue_operand_rt_i;
    result_x_q     <= (issue_cop0_wr_i | issue_cop1_wr_i | issue_cop2_wr_i | issue_cop3_wr_i) ? 
                       ((issue_opcode_i[31:28] == 4'b0100 /* COPx */) ? issue_operand_rt_i : issue_operand_rs_i) : issue_cop3_i ? cop3_result_rdata_i :
                       issue_cop2_i ? cop2_result_rdata_i :
                       issue_cop1_i ? cop1_result_rdata_i :
                                      cop0_result_rdata_i;
    exception_x_q  <= (|issue_exception_i) ? issue_exception_i : 
                       branch_misaligned_w  ? `EXCEPTION_ADEL : `EXCEPTION_W'b0;

    // Record bad address target
    if (!(|issue_exception_i) && branch_misaligned_w)
        result_x_q     <= issue_branch_target_i; 
end
// No valid instruction (or pipeline flush event)
else
begin
    valid_x_q      <= 1'b0;
    ctrl_x_q       <= `PCINFO_W'b0;
    pc_x_q         <= 32'b0;
    npc_x_q        <= 32'b0;
    opcode_x_q     <= 32'b0;
    operand_rs_x_q <= 32'b0;
    operand_rt_x_q <= 32'b0;
    result_x_q     <= 32'b0;
    exception_x_q  <= `EXCEPTION_W'b0;
end

wire   alu_x_w        = ctrl_x_q[`PCINFO_ALU];
assign load_x_o       = ctrl_x_q[`PCINFO_LOAD];
assign store_x_o      = ctrl_x_q[`PCINFO_STORE];
assign mul_x_o        = ctrl_x_q[`PCINFO_MUL];
assign branch_x_o     = ctrl_x_q[`PCINFO_BRANCH];
assign rd_x_o         = {5{ctrl_x_q[`PCINFO_RD_VALID]}} & ctrl_x_q[`PCINFO_RD];
assign pc_x_o         = pc_x_q;
assign opcode_x_o     = opcode_x_q;
assign operand_rs_x_o = operand_rs_x_q;
assign operand_rt_x_o = operand_rt_x_q;
assign exception_x_o  = exception_x_q;

//-------------------------------------------------------------
// M-stage (Mem result)
//------------------------------------------------------------- 
reg                     valid_m_q;
reg [`PCINFO_W-1:0]     ctrl_m_q;
reg [31:0]              result_m_q;
reg [31:0]              pc_m_q;
reg [31:0]              npc_m_q;
reg [31:0]              opcode_m_q;
reg [31:0]              operand_rs_m_q;
reg [31:0]              operand_rt_m_q;
reg [`EXCEPTION_W-1:0]  exception_m_q;

always @ (posedge clk_i )
if (rst_i)
begin
    valid_m_q      <= 1'b0;
    ctrl_m_q       <= `PCINFO_W'b0;
    pc_m_q         <= 32'b0;
    npc_m_q        <= 32'b0;
    opcode_m_q     <= 32'b0;
    operand_rs_m_q <= 32'b0;
    operand_rt_m_q <= 32'b0;
    result_m_q     <= 32'b0;
    exception_m_q  <= `EXCEPTION_W'b0;
end
// Stall - no change in M state
else if (issue_stall_i)
    ;
// Pipeline flush
else if (squash_x_m_o)
begin
    valid_m_q      <= 1'b0;
    ctrl_m_q       <= `PCINFO_W'b0;
    pc_m_q         <= 32'b0;
    npc_m_q        <= 32'b0;
    opcode_m_q     <= 32'b0;
    operand_rs_m_q <= 32'b0;
    operand_rt_m_q <= 32'b0;
    result_m_q     <= 32'b0;
    exception_m_q  <= `EXCEPTION_W'b0;
end
// Normal pipeline advance
else
begin
    valid_m_q      <= valid_x_q;
    ctrl_m_q       <= ctrl_x_q;
    pc_m_q         <= pc_x_q;
    npc_m_q        <= npc_x_q;
    opcode_m_q     <= opcode_x_q;
    operand_rs_m_q <= operand_rs_x_q;
    operand_rt_m_q <= operand_rt_x_q;

    // Launch interrupt
    if (ctrl_x_q[`PCINFO_INTR])
        exception_m_q  <= `EXCEPTION_INT;
    // If frontend reports bad instruction, ignore later CSR errors...
    else if (|exception_x_q)
    begin
        valid_m_q      <= 1'b0;
        exception_m_q  <= exception_x_q;
    end
    else
        exception_m_q  <= cop0_result_exception_x_i;

    if (ctrl_x_q[`PCINFO_COP0] | ctrl_x_q[`PCINFO_COP1] | ctrl_x_q[`PCINFO_COP2] | ctrl_x_q[`PCINFO_COP3])
        result_m_q <= result_x_q;
    else if (exception_x_q  != `EXCEPTION_W'b0)
        result_m_q <= result_x_q;
    else
        result_m_q <= alu_result_x_i;
end

reg [31:0] result_m_r;

wire valid_m_w      = valid_m_q & ~issue_stall_i;

always @ *
begin
    // Default: ALU result
    result_m_r = result_m_q;

    if (SUPPORT_LOAD_BYPASS && valid_m_w && (ctrl_m_q[`PCINFO_LOAD] || ctrl_m_q[`PCINFO_STORE]))
        result_m_r = mem_result_m_i;
end

wire   load_store_m_w = ctrl_m_q[`PCINFO_LOAD] | ctrl_m_q[`PCINFO_STORE];
assign load_m_o       = ctrl_m_q[`PCINFO_LOAD];
assign mul_m_o        = ctrl_m_q[`PCINFO_MUL];
assign rd_m_o         = {5{(valid_m_w && ctrl_m_q[`PCINFO_RD_VALID] && ~stall_o)}} & ctrl_m_q[`PCINFO_RD];
assign operand_rt_m_o = operand_rt_m_q;
assign result_m_o     = result_m_r;

// Load store result not ready when reaching M
assign stall_o         = (ctrl_x_q[`PCINFO_DIV] && ~div_complete_i) || ((ctrl_m_q[`PCINFO_LOAD] | ctrl_m_q[`PCINFO_STORE]) & ~mem_complete_i);

reg [`EXCEPTION_W-1:0] exception_m_r;
always @ *
begin
    if (valid_m_q && (ctrl_m_q[`PCINFO_LOAD] || ctrl_m_q[`PCINFO_STORE]) && mem_complete_i)
        exception_m_r = mem_exception_m_i;
    else
        exception_m_r = exception_m_q;
end

assign squash_x_m_w = |exception_m_r;

reg squash_x_m_q;

always @ (posedge clk_i )
if (rst_i)
    squash_x_m_q <= 1'b0;
else if (~issue_stall_i)
    squash_x_m_q <= squash_x_m_w;

assign squash_x_m_o = squash_x_m_w | squash_x_m_q;

//-------------------------------------------------------------
// Writeback / Commit
//------------------------------------------------------------- 
reg                     valid_wb_q;
reg [`PCINFO_W-1:0]     ctrl_wb_q;
reg [31:0]              result_wb_q;
reg [31:0]              pc_wb_q;
reg [31:0]              npc_wb_q;
reg [31:0]              opcode_wb_q;
reg [31:0]              operand_rs_wb_q;
reg [31:0]              operand_rt_wb_q;
reg [`EXCEPTION_W-1:0]  exception_wb_q;

always @ (posedge clk_i )
if (rst_i)
begin
    valid_wb_q      <= 1'b0;
    ctrl_wb_q       <= `PCINFO_W'b0;
    pc_wb_q         <= 32'b0;
    npc_wb_q        <= 32'b0;
    opcode_wb_q     <= 32'b0;
    operand_rs_wb_q <= 32'b0;
    operand_rt_wb_q <= 32'b0;
    result_wb_q     <= 32'b0;
    exception_wb_q  <= `EXCEPTION_W'b0;
end
// Stall - no change in WB state
else if (issue_stall_i)
    ;
else
begin
    // Squash instruction valid on memory faults
    case (exception_m_r)
    `EXCEPTION_ADEL,
    `EXCEPTION_ADES,
    `EXCEPTION_IBE,
    `EXCEPTION_DBE: // TODO: Other exception..
        valid_wb_q      <= 1'b0;
    default:
        valid_wb_q      <= valid_m_q;
    endcase

    // Exception - squash writeback
    if (|exception_m_r)
        ctrl_wb_q       <= ctrl_m_q & ~(1 << `PCINFO_RD_VALID);
    else
        ctrl_wb_q       <= ctrl_m_q;

    pc_wb_q         <= pc_m_q;
    npc_wb_q        <= npc_m_q;
    opcode_wb_q     <= opcode_m_q;
    operand_rs_wb_q <= operand_rs_m_q;
    operand_rt_wb_q <= operand_rt_m_q;
    exception_wb_q  <= exception_m_r;

    if (valid_m_w && (ctrl_m_q[`PCINFO_LOAD] || ctrl_m_q[`PCINFO_STORE]))
        result_wb_q <= mem_result_m_i;
    else
        result_wb_q <= result_m_q;
end

// Instruction completion (for debug)
wire complete_wb_w     = ctrl_wb_q[`PCINFO_COMPLETE] & ~issue_stall_i;

assign valid_wb_o      = valid_wb_q & ~issue_stall_i;
assign rd_wb_o         = {5{(valid_wb_o && ctrl_wb_q[`PCINFO_RD_VALID] && ~stall_o)}} & ctrl_wb_q[`PCINFO_RD];
assign result_wb_o     = result_wb_q;
assign pc_wb_o         = pc_wb_q;
assign opcode_wb_o     = opcode_wb_q;
assign operand_rs_wb_o = operand_rs_wb_q;
assign operand_rt_wb_o = operand_rt_wb_q;

assign exception_wb_o  = exception_wb_q;
assign delay_slot_wb_o = ctrl_wb_q[`PCINFO_DEL_SLOT];

assign cop0_wb_o       = ctrl_wb_q[`PCINFO_COP0] & ~issue_stall_i; // TODO: Fault disable???
assign cop1_wb_o       = ctrl_wb_q[`PCINFO_COP1] & ~issue_stall_i; // TODO: Fault disable???
assign cop2_wb_o       = ctrl_wb_q[`PCINFO_COP2] & ~issue_stall_i; // TODO: Fault disable???
assign cop3_wb_o       = ctrl_wb_q[`PCINFO_COP3] & ~issue_stall_i; // TODO: Fault disable???
assign cop0_write_wb_o = valid_wb_o && ctrl_wb_q[`PCINFO_COP0_WR] && ~stall_o;
assign cop1_write_wb_o = valid_wb_o && ctrl_wb_q[`PCINFO_COP1_WR] && ~stall_o;
assign cop2_write_wb_o = valid_wb_o && ctrl_wb_q[`PCINFO_COP2_WR] && ~stall_o;
assign cop3_write_wb_o = valid_wb_o && ctrl_wb_q[`PCINFO_COP3_WR] && ~stall_o;
assign cop0_wdata_wb_o = result_wb_q;

reg [5:0] cop_waddr_r;
always @ *
begin
    cop_waddr_r = 6'b0;

    if (opcode_wb_q[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_wb_q[`OPCODE_FUNC_R] == `INSTR_R_MFHI)
        cop_waddr_r = {1'b1, 5'd1};
    else if (opcode_wb_q[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_wb_q[`OPCODE_FUNC_R] == `INSTR_R_MTHI)
        cop_waddr_r = {1'b1, 5'd1};
    else if (opcode_wb_q[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_wb_q[`OPCODE_FUNC_R] == `INSTR_R_MFLO)
        cop_waddr_r = {1'b1, 5'd0};
    else if (opcode_wb_q[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_wb_q[`OPCODE_FUNC_R] == `INSTR_R_MTLO)
        cop_waddr_r = {1'b1, 5'd0};
    else if (opcode_wb_q[`OPCODE_INST_R] == `INSTR_I_LWC0 || 
             opcode_wb_q[`OPCODE_INST_R] == `INSTR_I_LWC1 ||
             opcode_wb_q[`OPCODE_INST_R] == `INSTR_I_LWC2 || 
             opcode_wb_q[`OPCODE_INST_R] == `INSTR_I_LWC3)
        cop_waddr_r = {1'b0, opcode_wb_q[`OPCODE_RT_R]};
    // COP0 - RFE
    else if (opcode_wb_q[`OPCODE_INST_R] == `INSTR_COP0 && opcode_wb_q[`OPCODE_FUNC_R] == 6'b010000)
        cop_waddr_r  = {1'b1, `COP0_STATUS};
    else if (opcode_wb_q[31:28] == 4'b0100 /* COPx */ && opcode_wb_q[`OPCODE_RS_R] == 5'b00110)
        cop_waddr_r  = {1'b1, opcode_wb_q[`OPCODE_RD_R]};
    else
        cop_waddr_r  = {1'b0, opcode_wb_q[`OPCODE_RD_R]};
end
assign cop0_waddr_wb_o  = cop_waddr_r;

`ifdef verilator
mpx_trace_sim
u_trace_d
(
     .valid_i(issue_valid_i)
    ,.pc_i(issue_pc_i)
    ,.opcode_i(issue_opcode_i)
);

mpx_trace_sim
u_trace_wb
(
     .valid_i(valid_wb_o)
    ,.pc_i(pc_wb_o)
    ,.opcode_i(opcode_wb_o)
);
`endif

endmodule//-----------------------------------------------------------------
//                          MPX Core
//                            V0.1
//                   github.com/ultraembedded
//                       Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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
module mpx_regfile
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter SUPPORT_REGFILE_XILINX = 0
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input  [  4:0]  rd0_i
    ,input  [ 31:0]  rd0_value_i
    ,input  [  4:0]  ra0_i
    ,input  [  4:0]  rb0_i

    // Outputs
    ,output [ 31:0]  ra0_value_o
    ,output [ 31:0]  rb0_value_o
);

//-----------------------------------------------------------------
// Xilinx specific register file
//-----------------------------------------------------------------
generate
if (SUPPORT_REGFILE_XILINX)
begin: REGFILE_XILINX_SINGLE

    mpx_xilinx_2r1w
    u_reg
    (
        // Inputs
         .clk_i(clk_i)
        ,.rst_i(rst_i)
        ,.rd0_i(rd0_i)
        ,.rd0_value_i(rd0_value_i)
        ,.ra_i(ra0_i)
        ,.rb_i(rb0_i)

        // Outputs
        ,.ra_value_o(ra0_value_o)
        ,.rb_value_o(rb0_value_o)
    );
end
//-----------------------------------------------------------------
// Flop based register file
//-----------------------------------------------------------------
else
begin: REGFILE
    reg [31:0] reg_r1_q;
    reg [31:0] reg_r2_q;
    reg [31:0] reg_r3_q;
    reg [31:0] reg_r4_q;
    reg [31:0] reg_r5_q;
    reg [31:0] reg_r6_q;
    reg [31:0] reg_r7_q;
    reg [31:0] reg_r8_q;
    reg [31:0] reg_r9_q;
    reg [31:0] reg_r10_q;
    reg [31:0] reg_r11_q;
    reg [31:0] reg_r12_q;
    reg [31:0] reg_r13_q;
    reg [31:0] reg_r14_q;
    reg [31:0] reg_r15_q;
    reg [31:0] reg_r16_q;
    reg [31:0] reg_r17_q;
    reg [31:0] reg_r18_q;
    reg [31:0] reg_r19_q;
    reg [31:0] reg_r20_q;
    reg [31:0] reg_r21_q;
    reg [31:0] reg_r22_q;
    reg [31:0] reg_r23_q;
    reg [31:0] reg_r24_q;
    reg [31:0] reg_r25_q;
    reg [31:0] reg_r26_q;
    reg [31:0] reg_r27_q;
    reg [31:0] reg_r28_q;
    reg [31:0] reg_r29_q;
    reg [31:0] reg_r30_q;
    reg [31:0] reg_r31_q;

    // Simulation friendly names
    wire [31:0] x0_zero_w = 32'b0;
    wire [31:0] x1_at_w   = reg_r1_q;
    wire [31:0] x2_v0_w   = reg_r2_q;
    wire [31:0] x3_v1_w   = reg_r3_q;
    wire [31:0] x4_a0_w   = reg_r4_q;
    wire [31:0] x5_a1_w   = reg_r5_q;
    wire [31:0] x6_a2_w   = reg_r6_q;
    wire [31:0] x7_a3_w   = reg_r7_q;
    wire [31:0] x8_t0_w   = reg_r8_q;
    wire [31:0] x9_t1_w   = reg_r9_q;
    wire [31:0] x10_t2_w  = reg_r10_q;
    wire [31:0] x11_t3_w  = reg_r11_q;
    wire [31:0] x12_t4_w  = reg_r12_q;
    wire [31:0] x13_t5_w  = reg_r13_q;
    wire [31:0] x14_t6_w  = reg_r14_q;
    wire [31:0] x15_t7_w  = reg_r15_q;
    wire [31:0] x16_s0_w  = reg_r16_q;
    wire [31:0] x17_s1_w  = reg_r17_q;
    wire [31:0] x18_s2_w  = reg_r18_q;
    wire [31:0] x19_s3_w  = reg_r19_q;
    wire [31:0] x20_s4_w  = reg_r20_q;
    wire [31:0] x21_s5_w  = reg_r21_q;
    wire [31:0] x22_s6_w  = reg_r22_q;
    wire [31:0] x23_s7_w  = reg_r23_q;
    wire [31:0] x24_t8_w  = reg_r24_q;
    wire [31:0] x25_t9_w  = reg_r25_q;
    wire [31:0] x26_k0_w  = reg_r26_q;
    wire [31:0] x27_k1_w  = reg_r27_q;
    wire [31:0] x28_gp_w  = reg_r28_q;
    wire [31:0] x29_sp_w  = reg_r29_q;
    wire [31:0] x30_fp_w  = reg_r30_q;
    wire [31:0] x31_ra_w  = reg_r31_q;

    //-----------------------------------------------------------------
    // Flop based register File (for simulation)
    //-----------------------------------------------------------------

    // Synchronous register write back
    always @ (posedge clk_i )
    if (rst_i)
    begin
        reg_r1_q       <= 32'h00000000;
        reg_r2_q       <= 32'h00000000;
        reg_r3_q       <= 32'h00000000;
        reg_r4_q       <= 32'h00000000;
        reg_r5_q       <= 32'h00000000;
        reg_r6_q       <= 32'h00000000;
        reg_r7_q       <= 32'h00000000;
        reg_r8_q       <= 32'h00000000;
        reg_r9_q       <= 32'h00000000;
        reg_r10_q      <= 32'h00000000;
        reg_r11_q      <= 32'h00000000;
        reg_r12_q      <= 32'h00000000;
        reg_r13_q      <= 32'h00000000;
        reg_r14_q      <= 32'h00000000;
        reg_r15_q      <= 32'h00000000;
        reg_r16_q      <= 32'h00000000;
        reg_r17_q      <= 32'h00000000;
        reg_r18_q      <= 32'h00000000;
        reg_r19_q      <= 32'h00000000;
        reg_r20_q      <= 32'h00000000;
        reg_r21_q      <= 32'h00000000;
        reg_r22_q      <= 32'h00000000;
        reg_r23_q      <= 32'h00000000;
        reg_r24_q      <= 32'h00000000;
        reg_r25_q      <= 32'h00000000;
        reg_r26_q      <= 32'h00000000;
        reg_r27_q      <= 32'h00000000;
        reg_r28_q      <= 32'h00000000;
        reg_r29_q      <= 32'h00000000;
        reg_r30_q      <= 32'h00000000;
        reg_r31_q      <= 32'h00000000;
    end
    else
    begin
        if      (rd0_i == 5'd1) reg_r1_q <= rd0_value_i;
        if      (rd0_i == 5'd2) reg_r2_q <= rd0_value_i;
        if      (rd0_i == 5'd3) reg_r3_q <= rd0_value_i;
        if      (rd0_i == 5'd4) reg_r4_q <= rd0_value_i;
        if      (rd0_i == 5'd5) reg_r5_q <= rd0_value_i;
        if      (rd0_i == 5'd6) reg_r6_q <= rd0_value_i;
        if      (rd0_i == 5'd7) reg_r7_q <= rd0_value_i;
        if      (rd0_i == 5'd8) reg_r8_q <= rd0_value_i;
        if      (rd0_i == 5'd9) reg_r9_q <= rd0_value_i;
        if      (rd0_i == 5'd10) reg_r10_q <= rd0_value_i;
        if      (rd0_i == 5'd11) reg_r11_q <= rd0_value_i;
        if      (rd0_i == 5'd12) reg_r12_q <= rd0_value_i;
        if      (rd0_i == 5'd13) reg_r13_q <= rd0_value_i;
        if      (rd0_i == 5'd14) reg_r14_q <= rd0_value_i;
        if      (rd0_i == 5'd15) reg_r15_q <= rd0_value_i;
        if      (rd0_i == 5'd16) reg_r16_q <= rd0_value_i;
        if      (rd0_i == 5'd17) reg_r17_q <= rd0_value_i;
        if      (rd0_i == 5'd18) reg_r18_q <= rd0_value_i;
        if      (rd0_i == 5'd19) reg_r19_q <= rd0_value_i;
        if      (rd0_i == 5'd20) reg_r20_q <= rd0_value_i;
        if      (rd0_i == 5'd21) reg_r21_q <= rd0_value_i;
        if      (rd0_i == 5'd22) reg_r22_q <= rd0_value_i;
        if      (rd0_i == 5'd23) reg_r23_q <= rd0_value_i;
        if      (rd0_i == 5'd24) reg_r24_q <= rd0_value_i;
        if      (rd0_i == 5'd25) reg_r25_q <= rd0_value_i;
        if      (rd0_i == 5'd26) reg_r26_q <= rd0_value_i;
        if      (rd0_i == 5'd27) reg_r27_q <= rd0_value_i;
        if      (rd0_i == 5'd28) reg_r28_q <= rd0_value_i;
        if      (rd0_i == 5'd29) reg_r29_q <= rd0_value_i;
        if      (rd0_i == 5'd30) reg_r30_q <= rd0_value_i;
        if      (rd0_i == 5'd31) reg_r31_q <= rd0_value_i;
    end

    //-----------------------------------------------------------------
    // Asynchronous read
    //-----------------------------------------------------------------
    reg [31:0] ra0_value_r;
    reg [31:0] rb0_value_r;
    always @ *
    begin
        case (ra0_i)
        5'd1: ra0_value_r = reg_r1_q;
        5'd2: ra0_value_r = reg_r2_q;
        5'd3: ra0_value_r = reg_r3_q;
        5'd4: ra0_value_r = reg_r4_q;
        5'd5: ra0_value_r = reg_r5_q;
        5'd6: ra0_value_r = reg_r6_q;
        5'd7: ra0_value_r = reg_r7_q;
        5'd8: ra0_value_r = reg_r8_q;
        5'd9: ra0_value_r = reg_r9_q;
        5'd10: ra0_value_r = reg_r10_q;
        5'd11: ra0_value_r = reg_r11_q;
        5'd12: ra0_value_r = reg_r12_q;
        5'd13: ra0_value_r = reg_r13_q;
        5'd14: ra0_value_r = reg_r14_q;
        5'd15: ra0_value_r = reg_r15_q;
        5'd16: ra0_value_r = reg_r16_q;
        5'd17: ra0_value_r = reg_r17_q;
        5'd18: ra0_value_r = reg_r18_q;
        5'd19: ra0_value_r = reg_r19_q;
        5'd20: ra0_value_r = reg_r20_q;
        5'd21: ra0_value_r = reg_r21_q;
        5'd22: ra0_value_r = reg_r22_q;
        5'd23: ra0_value_r = reg_r23_q;
        5'd24: ra0_value_r = reg_r24_q;
        5'd25: ra0_value_r = reg_r25_q;
        5'd26: ra0_value_r = reg_r26_q;
        5'd27: ra0_value_r = reg_r27_q;
        5'd28: ra0_value_r = reg_r28_q;
        5'd29: ra0_value_r = reg_r29_q;
        5'd30: ra0_value_r = reg_r30_q;
        5'd31: ra0_value_r = reg_r31_q;
        default : ra0_value_r = 32'h00000000;
        endcase

        case (rb0_i)
        5'd1: rb0_value_r = reg_r1_q;
        5'd2: rb0_value_r = reg_r2_q;
        5'd3: rb0_value_r = reg_r3_q;
        5'd4: rb0_value_r = reg_r4_q;
        5'd5: rb0_value_r = reg_r5_q;
        5'd6: rb0_value_r = reg_r6_q;
        5'd7: rb0_value_r = reg_r7_q;
        5'd8: rb0_value_r = reg_r8_q;
        5'd9: rb0_value_r = reg_r9_q;
        5'd10: rb0_value_r = reg_r10_q;
        5'd11: rb0_value_r = reg_r11_q;
        5'd12: rb0_value_r = reg_r12_q;
        5'd13: rb0_value_r = reg_r13_q;
        5'd14: rb0_value_r = reg_r14_q;
        5'd15: rb0_value_r = reg_r15_q;
        5'd16: rb0_value_r = reg_r16_q;
        5'd17: rb0_value_r = reg_r17_q;
        5'd18: rb0_value_r = reg_r18_q;
        5'd19: rb0_value_r = reg_r19_q;
        5'd20: rb0_value_r = reg_r20_q;
        5'd21: rb0_value_r = reg_r21_q;
        5'd22: rb0_value_r = reg_r22_q;
        5'd23: rb0_value_r = reg_r23_q;
        5'd24: rb0_value_r = reg_r24_q;
        5'd25: rb0_value_r = reg_r25_q;
        5'd26: rb0_value_r = reg_r26_q;
        5'd27: rb0_value_r = reg_r27_q;
        5'd28: rb0_value_r = reg_r28_q;
        5'd29: rb0_value_r = reg_r29_q;
        5'd30: rb0_value_r = reg_r30_q;
        5'd31: rb0_value_r = reg_r31_q;
        default : rb0_value_r = 32'h00000000;
        endcase
    end

    assign ra0_value_o = ra0_value_r;
    assign rb0_value_o = rb0_value_r;

    //-------------------------------------------------------------
    // get_register: Read register file
    //-------------------------------------------------------------
    `ifdef verilator
    function [31:0] get_register; /*verilator public*/
        input [4:0] r;
    begin
        case (r)
        5'd1: get_register = reg_r1_q;
        5'd2: get_register = reg_r2_q;
        5'd3: get_register = reg_r3_q;
        5'd4: get_register = reg_r4_q;
        5'd5: get_register = reg_r5_q;
        5'd6: get_register = reg_r6_q;
        5'd7: get_register = reg_r7_q;
        5'd8: get_register = reg_r8_q;
        5'd9: get_register = reg_r9_q;
        5'd10: get_register = reg_r10_q;
        5'd11: get_register = reg_r11_q;
        5'd12: get_register = reg_r12_q;
        5'd13: get_register = reg_r13_q;
        5'd14: get_register = reg_r14_q;
        5'd15: get_register = reg_r15_q;
        5'd16: get_register = reg_r16_q;
        5'd17: get_register = reg_r17_q;
        5'd18: get_register = reg_r18_q;
        5'd19: get_register = reg_r19_q;
        5'd20: get_register = reg_r20_q;
        5'd21: get_register = reg_r21_q;
        5'd22: get_register = reg_r22_q;
        5'd23: get_register = reg_r23_q;
        5'd24: get_register = reg_r24_q;
        5'd25: get_register = reg_r25_q;
        5'd26: get_register = reg_r26_q;
        5'd27: get_register = reg_r27_q;
        5'd28: get_register = reg_r28_q;
        5'd29: get_register = reg_r29_q;
        5'd30: get_register = reg_r30_q;
        5'd31: get_register = reg_r31_q;
        default : get_register = 32'h00000000;
        endcase
    end
    endfunction
    //-------------------------------------------------------------
    // set_register: Write register file
    //-------------------------------------------------------------
    function set_register; /*verilator public*/
        input [4:0] r;
        input [31:0] value;
    begin
        //case (r)
        //5'd1:  reg_r1_q  <= value;
        //5'd2:  reg_r2_q  <= value;
        //5'd3:  reg_r3_q  <= value;
        //5'd4:  reg_r4_q  <= value;
        //5'd5:  reg_r5_q  <= value;
        //5'd6:  reg_r6_q  <= value;
        //5'd7:  reg_r7_q  <= value;
        //5'd8:  reg_r8_q  <= value;
        //5'd9:  reg_r9_q  <= value;
        //5'd10: reg_r10_q <= value;
        //5'd11: reg_r11_q <= value;
        //5'd12: reg_r12_q <= value;
        //5'd13: reg_r13_q <= value;
        //5'd14: reg_r14_q <= value;
        //5'd15: reg_r15_q <= value;
        //5'd16: reg_r16_q <= value;
        //5'd17: reg_r17_q <= value;
        //5'd18: reg_r18_q <= value;
        //5'd19: reg_r19_q <= value;
        //5'd20: reg_r20_q <= value;
        //5'd21: reg_r21_q <= value;
        //5'd22: reg_r22_q <= value;
        //5'd23: reg_r23_q <= value;
        //5'd24: reg_r24_q <= value;
        //5'd25: reg_r25_q <= value;
        //5'd26: reg_r26_q <= value;
        //5'd27: reg_r27_q <= value;
        //5'd28: reg_r28_q <= value;
        //5'd29: reg_r29_q <= value;
        //5'd30: reg_r30_q <= value;
        //5'd31: reg_r31_q <= value;
        //default :
        //    ;
        //endcase
    end
    endfunction
    `endif

end
endgenerate

endmodule
//-----------------------------------------------------------------
//                          MPX Core
//                            V0.1
//                   github.com/ultraembedded
//                       Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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
// `include "mpx_defs.v"

module mpx_trace_sim
(
     input                        valid_i
    ,input  [31:0]                pc_i
    ,input  [31:0]                opcode_i
);

//-----------------------------------------------------------------
// get_regname_str: Convert register number to string
//-----------------------------------------------------------------
`ifdef verilator
function [79:0] get_regname_str;
    input  [4:0] regnum;
begin
    case (regnum)
    5'd0:  get_regname_str = "zero";
    5'd1:  get_regname_str = "at";
    5'd2:  get_regname_str = "v0";
    5'd3:  get_regname_str = "v1";
    5'd4:  get_regname_str = "a0";
    5'd5:  get_regname_str = "a1";
    5'd6:  get_regname_str = "a2";
    5'd7:  get_regname_str = "a3";
    5'd8:  get_regname_str = "t0";
    5'd9:  get_regname_str = "t1";
    5'd10:  get_regname_str = "t2";
    5'd11:  get_regname_str = "t3";
    5'd12:  get_regname_str = "t4";
    5'd13:  get_regname_str = "t5";
    5'd14:  get_regname_str = "t6";
    5'd15:  get_regname_str = "t7";
    5'd16:  get_regname_str = "s0";
    5'd17:  get_regname_str = "s1";
    5'd18:  get_regname_str = "s2";
    5'd19:  get_regname_str = "s3";
    5'd20:  get_regname_str = "s4";
    5'd21:  get_regname_str = "s5";
    5'd22:  get_regname_str = "s6";
    5'd23:  get_regname_str = "s7";
    5'd24:  get_regname_str = "t8";
    5'd25:  get_regname_str = "t9";
    5'd26:  get_regname_str = "k0";
    5'd27:  get_regname_str = "k1";
    5'd28:  get_regname_str = "gp";
    5'd29:  get_regname_str = "sp";
    5'd30:  get_regname_str = "fp";
    5'd31:  get_regname_str = "ra";
    endcase
end
endfunction

//-------------------------------------------------------------------
// Debug strings
//-------------------------------------------------------------------
reg [79:0] dbg_inst_str;
reg [79:0] dbg_inst_rs;
reg [79:0] dbg_inst_rt;
reg [79:0] dbg_inst_rd;
reg [31:0] dbg_inst_imm;
reg [31:0] dbg_inst_pc;

wire [4:0] rs_idx_w = opcode_i[`OPCODE_RS_R];
wire [4:0] rt_idx_w = opcode_i[`OPCODE_RT_R];
wire [4:0] rd_idx_w = opcode_i[`OPCODE_RD_R];
wire [4:0] re_idx_w = opcode_i[`OPCODE_RE_R];

always @ *
begin
    dbg_inst_str = "-";
    dbg_inst_rs  = "-";
    dbg_inst_rt  = "-";
    dbg_inst_rd  = "-";
    dbg_inst_pc  = 32'bx;

    if (valid_i)
    begin
        dbg_inst_pc  = pc_i;
        dbg_inst_rs  = get_regname_str(rs_idx_w);
        dbg_inst_rt  = get_regname_str(rt_idx_w);
        dbg_inst_rd  = get_regname_str(rd_idx_w);

        case (1'b1)
        (opcode_i[`OPCODE_INST_R] == `INSTR_J_JAL)   : dbg_inst_str = "JAL";
        (opcode_i[`OPCODE_INST_R] == `INSTR_J_J)     : dbg_inst_str = "J";
        (opcode_i[`OPCODE_INST_R] == `INSTR_J_BEQ)   : dbg_inst_str = "BEQ";
        (opcode_i[`OPCODE_INST_R] == `INSTR_J_BNE)   : dbg_inst_str = "BNE";
        (opcode_i[`OPCODE_INST_R] == `INSTR_J_BLEZ)  : dbg_inst_str = "BLEZ";
        (opcode_i[`OPCODE_INST_R] == `INSTR_J_BGTZ)  : dbg_inst_str = "BGTZ";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_ADDI)  : dbg_inst_str = "ADDI";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_ADDIU) : dbg_inst_str = "ADDIU";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_SLTI)  : dbg_inst_str = "SLTI";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_SLTIU) : dbg_inst_str = "SLTIU";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_ANDI)  : dbg_inst_str = "ANDI";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_ORI)   : dbg_inst_str = "ORI";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_XORI)  : dbg_inst_str = "XORI";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_LUI)   : dbg_inst_str = "LUI";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_LB)    : dbg_inst_str = "LB";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_LH)    : dbg_inst_str = "LH";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_LW)    : dbg_inst_str = "LW";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_LBU)   : dbg_inst_str = "LBU";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_LHU)   : dbg_inst_str = "LHU";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_LWL)   : dbg_inst_str = "LWL";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_LWR)   : dbg_inst_str = "LWR";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_SB)    : dbg_inst_str = "SB";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_SH)    : dbg_inst_str = "SH";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_SW)    : dbg_inst_str = "SW";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_SWL)   : dbg_inst_str = "SWL";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_SWR)   : dbg_inst_str = "SWR";
        (opcode_i[`OPCODE_INST_R] == `INSTR_COP0)    : dbg_inst_str = "COP0";
        (opcode_i[`OPCODE_INST_R] == `INSTR_COP1)    : dbg_inst_str = "COP1";
        (opcode_i[`OPCODE_INST_R] == `INSTR_COP2)    : dbg_inst_str = "COP2";
        (opcode_i[`OPCODE_INST_R] == `INSTR_COP3)    : dbg_inst_str = "COP3";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_LWC0)  : dbg_inst_str = "LWC0";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_LWC1)  : dbg_inst_str = "LWC1";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_LWC2)  : dbg_inst_str = "LWC2";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_LWC3)  : dbg_inst_str = "LWC3";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_SWC0)  : dbg_inst_str = "SWC0";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_SWC1)  : dbg_inst_str = "SWC1";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_SWC2)  : dbg_inst_str = "SWC2";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_SWC3)  : dbg_inst_str = "SWC3";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SLL)     : dbg_inst_str = "SLL";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SRL)     : dbg_inst_str = "SRL";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SRA)     : dbg_inst_str = "SRA";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SLLV)    : dbg_inst_str = "SLLV";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SRLV)    : dbg_inst_str = "SRLV";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SRAV)    : dbg_inst_str = "SRAV";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_JR)      : dbg_inst_str = "JR";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_JALR)    : dbg_inst_str = "JALR";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SYSCALL) : dbg_inst_str = "SYSCALL";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_BREAK)   : dbg_inst_str = "BREAK";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_MFHI)    : dbg_inst_str = "MFHI";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_MTHI)    : dbg_inst_str = "MTHI";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_MFLO)    : dbg_inst_str = "MFLO";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_MTLO)    : dbg_inst_str = "MTLO";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_MULT)    : dbg_inst_str = "MULT";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_MULTU)   : dbg_inst_str = "MULTU";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_DIV)     : dbg_inst_str = "DIV";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_DIVU)    : dbg_inst_str = "DIVU";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_ADD)     : dbg_inst_str = "ADD";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_ADDU)    : dbg_inst_str = "ADDU";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SUB)     : dbg_inst_str = "SUB";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SUBU)    : dbg_inst_str = "SUBU";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_AND)     : dbg_inst_str = "AND";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_OR)      : dbg_inst_str = "OR";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_XOR)     : dbg_inst_str = "XOR";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_NOR)     : dbg_inst_str = "NOR";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SLT)     : dbg_inst_str = "SLT";
        (opcode_i[`OPCODE_INST_R] == `INSTR_R_SPECIAL && opcode_i[`OPCODE_FUNC_R] == `INSTR_R_SLTU)    : dbg_inst_str = "SLTU";
        (opcode_i[`OPCODE_INST_R] == `INSTR_I_BRCOND) :
        begin
            case ({rt_idx_w[0],(rt_idx_w[4:1] == 4'h8)})
            2'b00:   dbg_inst_str = "BLTZ";
            2'b01:   dbg_inst_str = "BLTZAL";
            2'b10:   dbg_inst_str = "BGEZ";
            default: dbg_inst_str = "BGEZAL";
            endcase
        end
        endcase

        case (1'b1)
        (opcode_i[`OPCODE_INST_R] == `INSTR_J_JAL),
        (opcode_i[`OPCODE_INST_R] == `INSTR_J_J) :
        begin
            dbg_inst_rs  = "-";
            dbg_inst_rt  = "-";
            dbg_inst_rd  = "-";
            dbg_inst_imm = {4'b0, opcode_i[`OPCODE_ADDR_R], 2'b0};
        end
        (opcode_i[`OPCODE_INST_R] == `INSTR_COP2):
        begin
            if (opcode_i[25])
            begin
                dbg_inst_str = "COP2";
                dbg_inst_imm = {6'b0, opcode_i[25:0]};
            end
            else if (opcode_i[`OPCODE_RS_R] == 5'b00000)
            begin
                dbg_inst_str = "MFC2";
                dbg_inst_imm = 32'h0;
            end
            else if (opcode_i[`OPCODE_RS_R] == 5'b00010)
            begin
                dbg_inst_str = "CFC2";
                dbg_inst_imm = 32'h0;
            end
            else if (opcode_i[`OPCODE_RS_R] == 5'b00100)
            begin
                dbg_inst_str = "MTC2";
                dbg_inst_imm = 32'h0;
            end
            else if (opcode_i[`OPCODE_RS_R] == 5'b00110)
            begin
                dbg_inst_str = "CTC2";
                dbg_inst_imm = 32'h0;
            end
        end
        default:
        begin
            dbg_inst_imm = {{16{opcode_i[15]}}, opcode_i[`OPCODE_IMM_R]};
        end
        endcase        
    end
end
`endif

endmodule
//-----------------------------------------------------------------
//                          MPX Core
//                            V0.1
//                   github.com/ultraembedded
//                       Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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
//-----------------------------------------------------------------
// Module - Xilinx register file (2 async read, 1 write port)
//-----------------------------------------------------------------
module mpx_xilinx_2r1w
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input  [  4:0]  rd0_i
    ,input  [ 31:0]  rd0_value_i
    ,input  [  4:0]  ra_i
    ,input  [  4:0]  rb_i

    // Outputs
    ,output [ 31:0]  ra_value_o
    ,output [ 31:0]  rb_value_o
);


//-----------------------------------------------------------------
// Registers / Wires
//-----------------------------------------------------------------
wire [31:0]     reg_rs1_w;
wire [31:0]     reg_rs2_w;
wire [31:0]     rs1_0_15_w;
wire [31:0]     rs1_16_31_w;
wire [31:0]     rs2_0_15_w;
wire [31:0]     rs2_16_31_w;
wire            write_enable_w;
wire            write_banka_w;
wire            write_bankb_w;

//-----------------------------------------------------------------
// Register File (using RAM16X1D )
//-----------------------------------------------------------------
genvar i;

// Registers 0 - 15
generate
for (i=0;i<32;i=i+1)
begin : reg_loop1
    RAM16X1D reg_bit1a(.WCLK(clk_i), .WE(write_banka_w), .A0(rd0_i[0]), .A1(rd0_i[1]), .A2(rd0_i[2]), .A3(rd0_i[3]), .D(rd0_value_i[i]), .DPRA0(ra_i[0]), .DPRA1(ra_i[1]), .DPRA2(ra_i[2]), .DPRA3(ra_i[3]), .DPO(rs1_0_15_w[i]), .SPO(/* open */));
    RAM16X1D reg_bit2a(.WCLK(clk_i), .WE(write_banka_w), .A0(rd0_i[0]), .A1(rd0_i[1]), .A2(rd0_i[2]), .A3(rd0_i[3]), .D(rd0_value_i[i]), .DPRA0(rb_i[0]), .DPRA1(rb_i[1]), .DPRA2(rb_i[2]), .DPRA3(rb_i[3]), .DPO(rs2_0_15_w[i]), .SPO(/* open */));
end
endgenerate

// Registers 16 - 31
generate
for (i=0;i<32;i=i+1)
begin : reg_loop2
    RAM16X1D reg_bit1b(.WCLK(clk_i), .WE(write_bankb_w), .A0(rd0_i[0]), .A1(rd0_i[1]), .A2(rd0_i[2]), .A3(rd0_i[3]), .D(rd0_value_i[i]), .DPRA0(ra_i[0]), .DPRA1(ra_i[1]), .DPRA2(ra_i[2]), .DPRA3(ra_i[3]), .DPO(rs1_16_31_w[i]), .SPO(/* open */));
    RAM16X1D reg_bit2b(.WCLK(clk_i), .WE(write_bankb_w), .A0(rd0_i[0]), .A1(rd0_i[1]), .A2(rd0_i[2]), .A3(rd0_i[3]), .D(rd0_value_i[i]), .DPRA0(rb_i[0]), .DPRA1(rb_i[1]), .DPRA2(rb_i[2]), .DPRA3(rb_i[3]), .DPO(rs2_16_31_w[i]), .SPO(/* open */));
end
endgenerate

//-----------------------------------------------------------------
// Combinatorial Assignments
//-----------------------------------------------------------------
assign reg_rs1_w       = (ra_i[4] == 1'b0) ? rs1_0_15_w : rs1_16_31_w;
assign reg_rs2_w       = (rb_i[4] == 1'b0) ? rs2_0_15_w : rs2_16_31_w;

assign write_enable_w = (rd0_i != 5'b00000);

assign write_banka_w  = (write_enable_w & (~rd0_i[4]));
assign write_bankb_w  = (write_enable_w & rd0_i[4]);

reg [31:0] ra_value_r;
reg [31:0] rb_value_r;

// Register read ports
always @ *
begin
    if (ra_i == 5'b00000)
        ra_value_r = 32'h00000000;
    else
        ra_value_r = reg_rs1_w;

    if (rb_i == 5'b00000)
        rb_value_r = 32'h00000000;
    else
        rb_value_r = reg_rs2_w;
end

assign ra_value_o = ra_value_r;
assign rb_value_o = rb_value_r;

endmodule

//-------------------------------------------------------------
// RAM16X1D: Verilator target RAM16X1D model
//-------------------------------------------------------------
`ifdef verilator
module RAM16X1D (DPO, SPO, A0, A1, A2, A3, D, DPRA0, DPRA1, DPRA2, DPRA3, WCLK, WE);

    parameter INIT = 16'h0000;

    output DPO, SPO;

    input  A0, A1, A2, A3, D, DPRA0, DPRA1, DPRA2, DPRA3, WCLK, WE;

    reg  [15:0] mem;
    wire [3:0] adr;

    assign adr = {A3, A2, A1, A0};
    assign SPO = mem[adr];
    assign DPO = mem[{DPRA3, DPRA2, DPRA1, DPRA0}];

    initial 
        mem = INIT;

    always @(posedge WCLK) 
        if (WE == 1'b1)
            mem[adr] <= D;

endmodule
`endif
//-----------------------------------------------------------------
//                          MPX Core
//                            V0.1
//                   github.com/ultraembedded
//                       Copyright 2020
//
//                   admin@ultra-embedded.com
//
//                     License: Apache 2.0
//-----------------------------------------------------------------
// Copyright 2020 Ultra-Embedded.com
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

module mpx_core
//-----------------------------------------------------------------
// Params
//-----------------------------------------------------------------
#(
     parameter SUPPORT_MULDIV   = 1
    ,parameter SUPPORT_REGFILE_XILINX = 0
    ,parameter MEM_CACHE_ADDR_MIN = 32'h80000000
    ,parameter MEM_CACHE_ADDR_MAX = 32'h8fffffff
    ,parameter SUPPORTED_COP    = 5
    ,parameter COP0_PRID        = 2
)
//-----------------------------------------------------------------
// Ports
//-----------------------------------------------------------------
(
    // Inputs
     input           clk_i
    ,input           rst_i
    ,input  [ 31:0]  mem_d_data_rd_i
    ,input           mem_d_accept_i
    ,input           mem_d_ack_i
    ,input           mem_d_error_i
    ,input  [ 10:0]  mem_d_resp_tag_i
    ,input           mem_i_accept_i
    ,input           mem_i_valid_i
    ,input           mem_i_error_i
    ,input  [ 31:0]  mem_i_inst_i
    ,input  [  5:0]  intr_i
    ,input  [ 31:0]  reset_vector_i
    ,input  [ 31:0]  exception_vector_i
    ,input           cop2_accept_i
    ,input  [ 31:0]  cop2_reg_rdata_i

    // Outputs
    ,output [ 31:0]  mem_d_addr_o
    ,output [ 31:0]  mem_d_data_wr_o
    ,output          mem_d_rd_o
    ,output [  3:0]  mem_d_wr_o
    ,output          mem_d_cacheable_o
    ,output [ 10:0]  mem_d_req_tag_o
    ,output          mem_d_invalidate_o
    ,output          mem_d_writeback_o
    ,output          mem_d_flush_o
    ,output          mem_i_rd_o
    ,output          mem_i_flush_o
    ,output          mem_i_invalidate_o
    ,output [ 31:0]  mem_i_pc_o
    ,output [ 31:0]  cop0_status_o
    ,output          cop2_valid_o
    ,output [ 31:0]  cop2_opcode_o
    ,output          cop2_reg_write_o
    ,output [  5:0]  cop2_reg_waddr_o
    ,output [ 31:0]  cop2_reg_wdata_o
    ,output [  5:0]  cop2_reg_raddr_o
);

wire  [  4:0]  mul_opcode_rd_idx_w;
wire           fetch_dec_delay_slot_w;
wire  [ 31:0]  lsu_opcode_pc_w;
wire           fetch_accept_w;
wire  [ 31:0]  opcode_rt_operand_m_w;
wire  [ 31:0]  lsu_opcode_rt_operand_w;
wire  [ 31:0]  lsu_opcode_rs_operand_w;
wire  [ 31:0]  opcode_pc_w;
wire           cop0_opcode_delay_slot_w;
wire           mul_opcode_valid_w;
wire  [ 31:0]  writeback_div_hi_w;
wire  [ 31:0]  cop0_opcode_pc_w;
wire           cop0_opcode_invalid_w;
wire           branch_d_exec_priv_w;
wire           fetch_instr_mul_w;
wire           fetch_instr_rt_is_rd_w;
wire           branch_request_w;
wire           writeback_mem_valid_w;
wire  [ 31:0]  opcode_rt_operand_w;
wire           cop0_writeback_write_w;
wire           opcode_delay_slot_w;
wire           iflush_w;
wire  [  4:0]  mul_opcode_rt_idx_w;
wire           branch_d_exec_exception_w;
wire  [  5:0]  cop0_writeback_waddr_w;
wire  [ 31:0]  cop0_writeback_exception_addr_w;
wire  [ 31:0]  mul_opcode_opcode_w;
wire           exec_hold_w;
wire  [  4:0]  cop0_opcode_rs_idx_w;
wire           fetch_instr_invalid_w;
wire  [  4:0]  cop0_opcode_rd_idx_w;
wire  [ 31:0]  branch_pc_w;
wire           lsu_stall_w;
wire           writeback_mul_valid_w;
wire  [ 31:0]  writeback_mul_lo_w;
wire  [ 31:0]  mul_opcode_rs_operand_w;
wire  [ 31:0]  opcode_opcode_w;
wire  [ 31:0]  mul_opcode_pc_w;
wire           branch_d_exec_request_w;
wire           fetch_instr_cop3_wr_w;
wire           branch_cop0_request_w;
wire  [  4:0]  cop0_opcode_rt_idx_w;
wire  [ 31:0]  opcode_rs_operand_w;
wire  [ 31:0]  cop0_opcode_rs_operand_w;
wire           squash_muldiv_w;
wire           fetch_dec_fault_fetch_w;
wire           fetch_dec_valid_w;
wire  [  5:0]  cop0_result_x_exception_w;
wire  [  4:0]  mul_opcode_rs_idx_w;
wire           fetch_fault_fetch_w;
wire           lsu_opcode_invalid_w;
wire           mul_hold_w;
wire  [ 31:0]  fetch_pc_w;
wire  [  5:0]  cop0_rd_raddr_w;
wire           branch_exception_w;
wire  [ 31:0]  lsu_opcode_opcode_w;
wire           branch_priv_w;
wire           div_opcode_valid_w;
wire  [ 31:0]  fetch_dec_pc_w;
wire           interrupt_inhibit_w;
wire  [ 31:0]  cop0_writeback_wdata_w;
wire           lsu_opcode_delay_slot_w;
wire  [  5:0]  writeback_mem_exception_w;
wire  [ 31:0]  writeback_div_lo_w;
wire           fetch_instr_lsu_w;
wire           fetch_instr_cop0_wr_w;
wire  [ 31:0]  cop0_rd_rdata_w;
wire  [ 31:0]  writeback_mem_value_w;
wire           cop0_opcode_valid_w;
wire           writeback_div_valid_w;
wire           fetch_instr_branch_w;
wire           fetch_instr_lr_is_rd_w;
wire  [  4:0]  lsu_opcode_rd_idx_w;
wire           branch_cop0_priv_w;
wire           fetch_delay_slot_w;
wire  [  4:0]  opcode_rt_idx_w;
wire           fetch_dec_accept_w;
wire  [  4:0]  opcode_rs_idx_w;
wire           branch_cop0_exception_w;
wire           fetch_instr_exec_w;
wire  [  4:0]  opcode_rd_idx_w;
wire           opcode_invalid_w;
wire           cop0_rd_ren_w;
wire           take_interrupt_w;
wire  [  4:0]  lsu_opcode_rs_idx_w;
wire  [ 31:0]  branch_d_exec_pc_w;
wire  [ 31:0]  mul_opcode_rt_operand_w;
wire  [  4:0]  lsu_opcode_rt_idx_w;
wire           fetch_valid_w;
wire           mul_opcode_invalid_w;
wire  [ 31:0]  branch_cop0_pc_w;
wire  [ 31:0]  cop0_opcode_rt_operand_w;
wire  [ 31:0]  cop0_writeback_exception_pc_w;
wire           fetch_instr_cop1_wr_w;
wire           lsu_opcode_valid_w;
wire           cop0_writeback_delay_slot_w;
wire  [ 31:0]  fetch_dec_instr_w;
wire  [ 31:0]  cop0_opcode_opcode_w;
wire           fetch_instr_div_w;
wire  [ 31:0]  fetch_instr_w;
wire           fetch_instr_rd_valid_w;
wire           exec_opcode_valid_w;
wire           fetch_instr_cop2_wr_w;
wire           fetch_instr_cop0_w;
wire           fetch_instr_cop1_w;
wire           fetch_instr_cop2_w;
wire           fetch_instr_cop3_w;
wire           mul_opcode_delay_slot_w;
wire  [ 31:0]  writeback_mul_hi_w;
wire  [ 31:0]  writeback_exec_value_w;
wire  [  5:0]  cop0_writeback_exception_w;


mpx_exec
u_exec
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.opcode_valid_i(exec_opcode_valid_w)
    ,.opcode_opcode_i(opcode_opcode_w)
    ,.opcode_pc_i(opcode_pc_w)
    ,.opcode_invalid_i(opcode_invalid_w)
    ,.opcode_delay_slot_i(opcode_delay_slot_w)
    ,.opcode_rd_idx_i(opcode_rd_idx_w)
    ,.opcode_rs_idx_i(opcode_rs_idx_w)
    ,.opcode_rt_idx_i(opcode_rt_idx_w)
    ,.opcode_rs_operand_i(opcode_rs_operand_w)
    ,.opcode_rt_operand_i(opcode_rt_operand_w)
    ,.hold_i(exec_hold_w)

    // Outputs
    ,.branch_d_request_o(branch_d_exec_request_w)
    ,.branch_d_exception_o(branch_d_exec_exception_w)
    ,.branch_d_pc_o(branch_d_exec_pc_w)
    ,.branch_d_priv_o(branch_d_exec_priv_w)
    ,.writeback_value_o(writeback_exec_value_w)
);


mpx_decode
#(
     .SUPPORT_MULDIV(SUPPORT_MULDIV)
    ,.SUPPORTED_COP(SUPPORTED_COP)
)
u_decode
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.fetch_in_valid_i(fetch_dec_valid_w)
    ,.fetch_in_instr_i(fetch_dec_instr_w)
    ,.fetch_in_pc_i(fetch_dec_pc_w)
    ,.fetch_in_delay_slot_i(fetch_dec_delay_slot_w)
    ,.fetch_in_fault_fetch_i(fetch_dec_fault_fetch_w)
    ,.fetch_out_accept_i(fetch_accept_w)

    // Outputs
    ,.fetch_in_accept_o(fetch_dec_accept_w)
    ,.fetch_out_valid_o(fetch_valid_w)
    ,.fetch_out_instr_o(fetch_instr_w)
    ,.fetch_out_pc_o(fetch_pc_w)
    ,.fetch_out_delay_slot_o(fetch_delay_slot_w)
    ,.fetch_out_fault_fetch_o(fetch_fault_fetch_w)
    ,.fetch_out_instr_exec_o(fetch_instr_exec_w)
    ,.fetch_out_instr_lsu_o(fetch_instr_lsu_w)
    ,.fetch_out_instr_branch_o(fetch_instr_branch_w)
    ,.fetch_out_instr_mul_o(fetch_instr_mul_w)
    ,.fetch_out_instr_div_o(fetch_instr_div_w)
    ,.fetch_out_instr_cop0_o(fetch_instr_cop0_w)
    ,.fetch_out_instr_cop0_wr_o(fetch_instr_cop0_wr_w)
    ,.fetch_out_instr_cop1_o(fetch_instr_cop1_w)
    ,.fetch_out_instr_cop1_wr_o(fetch_instr_cop1_wr_w)
    ,.fetch_out_instr_cop2_o(fetch_instr_cop2_w)
    ,.fetch_out_instr_cop2_wr_o(fetch_instr_cop2_wr_w)
    ,.fetch_out_instr_cop3_o(fetch_instr_cop3_w)
    ,.fetch_out_instr_cop3_wr_o(fetch_instr_cop3_wr_w)
    ,.fetch_out_instr_rd_valid_o(fetch_instr_rd_valid_w)
    ,.fetch_out_instr_rt_is_rd_o(fetch_instr_rt_is_rd_w)
    ,.fetch_out_instr_lr_is_rd_o(fetch_instr_lr_is_rd_w)
    ,.fetch_out_instr_invalid_o(fetch_instr_invalid_w)
);


mpx_cop0
#(
     .SUPPORT_MULDIV(SUPPORT_MULDIV)
    ,.COP0_PRID(COP0_PRID)
)
u_cop0
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.intr_i(intr_i)
    ,.opcode_valid_i(cop0_opcode_valid_w)
    ,.opcode_opcode_i(cop0_opcode_opcode_w)
    ,.opcode_pc_i(cop0_opcode_pc_w)
    ,.opcode_invalid_i(cop0_opcode_invalid_w)
    ,.opcode_delay_slot_i(cop0_opcode_delay_slot_w)
    ,.opcode_rd_idx_i(cop0_opcode_rd_idx_w)
    ,.opcode_rs_idx_i(cop0_opcode_rs_idx_w)
    ,.opcode_rt_idx_i(cop0_opcode_rt_idx_w)
    ,.opcode_rs_operand_i(cop0_opcode_rs_operand_w)
    ,.opcode_rt_operand_i(cop0_opcode_rt_operand_w)
    ,.cop0_rd_ren_i(cop0_rd_ren_w)
    ,.cop0_rd_raddr_i(cop0_rd_raddr_w)
    ,.cop0_writeback_write_i(cop0_writeback_write_w)
    ,.cop0_writeback_waddr_i(cop0_writeback_waddr_w)
    ,.cop0_writeback_wdata_i(cop0_writeback_wdata_w)
    ,.cop0_writeback_exception_i(cop0_writeback_exception_w)
    ,.cop0_writeback_exception_pc_i(cop0_writeback_exception_pc_w)
    ,.cop0_writeback_exception_addr_i(cop0_writeback_exception_addr_w)
    ,.cop0_writeback_delay_slot_i(cop0_writeback_delay_slot_w)
    ,.mul_result_m_valid_i(writeback_mul_valid_w)
    ,.mul_result_m_hi_i(writeback_mul_hi_w)
    ,.mul_result_m_lo_i(writeback_mul_lo_w)
    ,.div_result_valid_i(writeback_div_valid_w)
    ,.div_result_hi_i(writeback_div_hi_w)
    ,.div_result_lo_i(writeback_div_lo_w)
    ,.squash_muldiv_i(squash_muldiv_w)
    ,.reset_vector_i(reset_vector_i)
    ,.exception_vector_i(exception_vector_i)
    ,.interrupt_inhibit_i(interrupt_inhibit_w)

    // Outputs
    ,.cop0_rd_rdata_o(cop0_rd_rdata_w)
    ,.cop0_result_x_exception_o(cop0_result_x_exception_w)
    ,.branch_cop0_request_o(branch_cop0_request_w)
    ,.branch_cop0_exception_o(branch_cop0_exception_w)
    ,.branch_cop0_pc_o(branch_cop0_pc_w)
    ,.branch_cop0_priv_o(branch_cop0_priv_w)
    ,.take_interrupt_o(take_interrupt_w)
    ,.iflush_o(iflush_w)
    ,.status_o(cop0_status_o)
);


mpx_lsu
#(
     .MEM_CACHE_ADDR_MAX(MEM_CACHE_ADDR_MAX)
    ,.MEM_CACHE_ADDR_MIN(MEM_CACHE_ADDR_MIN)
)
u_lsu
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.opcode_valid_i(lsu_opcode_valid_w)
    ,.opcode_opcode_i(lsu_opcode_opcode_w)
    ,.opcode_pc_i(lsu_opcode_pc_w)
    ,.opcode_invalid_i(lsu_opcode_invalid_w)
    ,.opcode_delay_slot_i(lsu_opcode_delay_slot_w)
    ,.opcode_rd_idx_i(lsu_opcode_rd_idx_w)
    ,.opcode_rs_idx_i(lsu_opcode_rs_idx_w)
    ,.opcode_rt_idx_i(lsu_opcode_rt_idx_w)
    ,.opcode_rs_operand_i(lsu_opcode_rs_operand_w)
    ,.opcode_rt_operand_i(lsu_opcode_rt_operand_w)
    ,.opcode_rt_operand_m_i(opcode_rt_operand_m_w)
    ,.mem_data_rd_i(mem_d_data_rd_i)
    ,.mem_accept_i(mem_d_accept_i)
    ,.mem_ack_i(mem_d_ack_i)
    ,.mem_error_i(mem_d_error_i)
    ,.mem_resp_tag_i(mem_d_resp_tag_i)

    // Outputs
    ,.mem_addr_o(mem_d_addr_o)
    ,.mem_data_wr_o(mem_d_data_wr_o)
    ,.mem_rd_o(mem_d_rd_o)
    ,.mem_wr_o(mem_d_wr_o)
    ,.mem_cacheable_o(mem_d_cacheable_o)
    ,.mem_req_tag_o(mem_d_req_tag_o)
    ,.mem_invalidate_o(mem_d_invalidate_o)
    ,.mem_writeback_o(mem_d_writeback_o)
    ,.mem_flush_o(mem_d_flush_o)
    ,.writeback_valid_o(writeback_mem_valid_w)
    ,.writeback_value_o(writeback_mem_value_w)
    ,.writeback_exception_o(writeback_mem_exception_w)
    ,.stall_o(lsu_stall_w)
);


mpx_multiplier
u_mul
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.opcode_valid_i(mul_opcode_valid_w)
    ,.opcode_opcode_i(mul_opcode_opcode_w)
    ,.opcode_pc_i(mul_opcode_pc_w)
    ,.opcode_invalid_i(mul_opcode_invalid_w)
    ,.opcode_delay_slot_i(mul_opcode_delay_slot_w)
    ,.opcode_rd_idx_i(mul_opcode_rd_idx_w)
    ,.opcode_rs_idx_i(mul_opcode_rs_idx_w)
    ,.opcode_rt_idx_i(mul_opcode_rt_idx_w)
    ,.opcode_rs_operand_i(mul_opcode_rs_operand_w)
    ,.opcode_rt_operand_i(mul_opcode_rt_operand_w)
    ,.hold_i(mul_hold_w)

    // Outputs
    ,.writeback_valid_o(writeback_mul_valid_w)
    ,.writeback_hi_o(writeback_mul_hi_w)
    ,.writeback_lo_o(writeback_mul_lo_w)
);


mpx_divider
u_div
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.opcode_valid_i(div_opcode_valid_w)
    ,.opcode_opcode_i(opcode_opcode_w)
    ,.opcode_pc_i(opcode_pc_w)
    ,.opcode_invalid_i(opcode_invalid_w)
    ,.opcode_delay_slot_i(opcode_delay_slot_w)
    ,.opcode_rd_idx_i(opcode_rd_idx_w)
    ,.opcode_rs_idx_i(opcode_rs_idx_w)
    ,.opcode_rt_idx_i(opcode_rt_idx_w)
    ,.opcode_rs_operand_i(opcode_rs_operand_w)
    ,.opcode_rt_operand_i(opcode_rt_operand_w)

    // Outputs
    ,.writeback_valid_o(writeback_div_valid_w)
    ,.writeback_hi_o(writeback_div_hi_w)
    ,.writeback_lo_o(writeback_div_lo_w)
);


mpx_issue
#(
     .SUPPORT_REGFILE_XILINX(SUPPORT_REGFILE_XILINX)
    ,.SUPPORT_LOAD_BYPASS(1)
    ,.SUPPORT_MULDIV(SUPPORT_MULDIV)
    ,.SUPPORTED_COP(SUPPORTED_COP)
    ,.SUPPORT_MUL_BYPASS(1)
)
u_issue
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.fetch_valid_i(fetch_valid_w)
    ,.fetch_instr_i(fetch_instr_w)
    ,.fetch_pc_i(fetch_pc_w)
    ,.fetch_delay_slot_i(fetch_delay_slot_w)
    ,.fetch_fault_fetch_i(fetch_fault_fetch_w)
    ,.fetch_instr_exec_i(fetch_instr_exec_w)
    ,.fetch_instr_lsu_i(fetch_instr_lsu_w)
    ,.fetch_instr_branch_i(fetch_instr_branch_w)
    ,.fetch_instr_mul_i(fetch_instr_mul_w)
    ,.fetch_instr_div_i(fetch_instr_div_w)
    ,.fetch_instr_cop0_i(fetch_instr_cop0_w)
    ,.fetch_instr_cop0_wr_i(fetch_instr_cop0_wr_w)
    ,.fetch_instr_cop1_i(fetch_instr_cop1_w)
    ,.fetch_instr_cop1_wr_i(fetch_instr_cop1_wr_w)
    ,.fetch_instr_cop2_i(fetch_instr_cop2_w)
    ,.fetch_instr_cop2_wr_i(fetch_instr_cop2_wr_w)
    ,.fetch_instr_cop3_i(fetch_instr_cop3_w)
    ,.fetch_instr_cop3_wr_i(fetch_instr_cop3_wr_w)
    ,.fetch_instr_rd_valid_i(fetch_instr_rd_valid_w)
    ,.fetch_instr_rt_is_rd_i(fetch_instr_rt_is_rd_w)
    ,.fetch_instr_lr_is_rd_i(fetch_instr_lr_is_rd_w)
    ,.fetch_instr_invalid_i(fetch_instr_invalid_w)
    ,.cop0_rd_rdata_i(cop0_rd_rdata_w)
    ,.branch_d_exec_request_i(branch_d_exec_request_w)
    ,.branch_d_exec_exception_i(branch_d_exec_exception_w)
    ,.branch_d_exec_pc_i(branch_d_exec_pc_w)
    ,.branch_d_exec_priv_i(branch_d_exec_priv_w)
    ,.branch_cop0_request_i(branch_cop0_request_w)
    ,.branch_cop0_exception_i(branch_cop0_exception_w)
    ,.branch_cop0_pc_i(branch_cop0_pc_w)
    ,.branch_cop0_priv_i(branch_cop0_priv_w)
    ,.writeback_exec_value_i(writeback_exec_value_w)
    ,.writeback_mem_valid_i(writeback_mem_valid_w)
    ,.writeback_mem_value_i(writeback_mem_value_w)
    ,.writeback_mem_exception_i(writeback_mem_exception_w)
    ,.writeback_mul_valid_i(writeback_mul_valid_w)
    ,.writeback_mul_hi_i(writeback_mul_hi_w)
    ,.writeback_mul_lo_i(writeback_mul_lo_w)
    ,.writeback_div_valid_i(writeback_div_valid_w)
    ,.writeback_div_hi_i(writeback_div_hi_w)
    ,.writeback_div_lo_i(writeback_div_lo_w)
    ,.cop0_result_x_exception_i(cop0_result_x_exception_w)
    ,.lsu_stall_i(lsu_stall_w)
    ,.take_interrupt_i(take_interrupt_w)
    ,.cop1_accept_i(1'b0)
    ,.cop1_reg_rdata_i(32'b0)
    ,.cop2_accept_i(cop2_accept_i)
    ,.cop2_reg_rdata_i(cop2_reg_rdata_i)
    ,.cop3_accept_i(1'b0)
    ,.cop3_reg_rdata_i(32'b0)

    // Outputs
    ,.fetch_accept_o(fetch_accept_w)
    ,.branch_request_o(branch_request_w)
    ,.branch_exception_o(branch_exception_w)
    ,.branch_pc_o(branch_pc_w)
    ,.branch_priv_o(branch_priv_w)
    ,.exec_opcode_valid_o(exec_opcode_valid_w)
    ,.lsu_opcode_valid_o(lsu_opcode_valid_w)
    ,.cop0_opcode_valid_o(cop0_opcode_valid_w)
    ,.mul_opcode_valid_o(mul_opcode_valid_w)
    ,.div_opcode_valid_o(div_opcode_valid_w)
    ,.opcode_opcode_o(opcode_opcode_w)
    ,.opcode_pc_o(opcode_pc_w)
    ,.opcode_invalid_o(opcode_invalid_w)
    ,.opcode_delay_slot_o(opcode_delay_slot_w)
    ,.opcode_rd_idx_o(opcode_rd_idx_w)
    ,.opcode_rs_idx_o(opcode_rs_idx_w)
    ,.opcode_rt_idx_o(opcode_rt_idx_w)
    ,.opcode_rs_operand_o(opcode_rs_operand_w)
    ,.opcode_rt_operand_o(opcode_rt_operand_w)
    ,.lsu_opcode_opcode_o(lsu_opcode_opcode_w)
    ,.lsu_opcode_pc_o(lsu_opcode_pc_w)
    ,.lsu_opcode_invalid_o(lsu_opcode_invalid_w)
    ,.lsu_opcode_delay_slot_o(lsu_opcode_delay_slot_w)
    ,.lsu_opcode_rd_idx_o(lsu_opcode_rd_idx_w)
    ,.lsu_opcode_rs_idx_o(lsu_opcode_rs_idx_w)
    ,.lsu_opcode_rt_idx_o(lsu_opcode_rt_idx_w)
    ,.lsu_opcode_rs_operand_o(lsu_opcode_rs_operand_w)
    ,.lsu_opcode_rt_operand_o(lsu_opcode_rt_operand_w)
    ,.lsu_opcode_rt_operand_m_o(opcode_rt_operand_m_w)
    ,.mul_opcode_opcode_o(mul_opcode_opcode_w)
    ,.mul_opcode_pc_o(mul_opcode_pc_w)
    ,.mul_opcode_invalid_o(mul_opcode_invalid_w)
    ,.mul_opcode_delay_slot_o(mul_opcode_delay_slot_w)
    ,.mul_opcode_rd_idx_o(mul_opcode_rd_idx_w)
    ,.mul_opcode_rs_idx_o(mul_opcode_rs_idx_w)
    ,.mul_opcode_rt_idx_o(mul_opcode_rt_idx_w)
    ,.mul_opcode_rs_operand_o(mul_opcode_rs_operand_w)
    ,.mul_opcode_rt_operand_o(mul_opcode_rt_operand_w)
    ,.cop0_opcode_opcode_o(cop0_opcode_opcode_w)
    ,.cop0_opcode_pc_o(cop0_opcode_pc_w)
    ,.cop0_opcode_invalid_o(cop0_opcode_invalid_w)
    ,.cop0_opcode_delay_slot_o(cop0_opcode_delay_slot_w)
    ,.cop0_opcode_rd_idx_o(cop0_opcode_rd_idx_w)
    ,.cop0_opcode_rs_idx_o(cop0_opcode_rs_idx_w)
    ,.cop0_opcode_rt_idx_o(cop0_opcode_rt_idx_w)
    ,.cop0_opcode_rs_operand_o(cop0_opcode_rs_operand_w)
    ,.cop0_opcode_rt_operand_o(cop0_opcode_rt_operand_w)
    ,.cop0_rd_ren_o(cop0_rd_ren_w)
    ,.cop0_rd_raddr_o(cop0_rd_raddr_w)
    ,.cop0_writeback_write_o(cop0_writeback_write_w)
    ,.cop0_writeback_waddr_o(cop0_writeback_waddr_w)
    ,.cop0_writeback_wdata_o(cop0_writeback_wdata_w)
    ,.cop0_writeback_exception_o(cop0_writeback_exception_w)
    ,.cop0_writeback_exception_pc_o(cop0_writeback_exception_pc_w)
    ,.cop0_writeback_exception_addr_o(cop0_writeback_exception_addr_w)
    ,.cop0_writeback_delay_slot_o(cop0_writeback_delay_slot_w)
    ,.exec_hold_o(exec_hold_w)
    ,.mul_hold_o(mul_hold_w)
    ,.squash_muldiv_o(squash_muldiv_w)
    ,.interrupt_inhibit_o(interrupt_inhibit_w)
    ,.cop1_valid_o()
    ,.cop1_opcode_o()
    ,.cop1_reg_write_o()
    ,.cop1_reg_waddr_o()
    ,.cop1_reg_wdata_o()
    ,.cop1_reg_raddr_o()
    ,.cop2_valid_o(cop2_valid_o)
    ,.cop2_opcode_o(cop2_opcode_o)
    ,.cop2_reg_write_o(cop2_reg_write_o)
    ,.cop2_reg_waddr_o(cop2_reg_waddr_o)
    ,.cop2_reg_wdata_o(cop2_reg_wdata_o)
    ,.cop2_reg_raddr_o(cop2_reg_raddr_o)
    ,.cop3_valid_o()
    ,.cop3_opcode_o()
    ,.cop3_reg_write_o()
    ,.cop3_reg_waddr_o()
    ,.cop3_reg_wdata_o()
    ,.cop3_reg_raddr_o()
);


mpx_fetch
u_fetch
(
    // Inputs
     .clk_i(clk_i)
    ,.rst_i(rst_i)
    ,.fetch_accept_i(fetch_dec_accept_w)
    ,.icache_accept_i(mem_i_accept_i)
    ,.icache_valid_i(mem_i_valid_i)
    ,.icache_error_i(mem_i_error_i)
    ,.icache_inst_i(mem_i_inst_i)
    ,.fetch_invalidate_i(iflush_w)
    ,.branch_request_i(branch_request_w)
    ,.branch_exception_i(branch_exception_w)
    ,.branch_pc_i(branch_pc_w)
    ,.branch_priv_i(branch_priv_w)

    // Outputs
    ,.fetch_valid_o(fetch_dec_valid_w)
    ,.fetch_instr_o(fetch_dec_instr_w)
    ,.fetch_pc_o(fetch_dec_pc_w)
    ,.fetch_delay_slot_o(fetch_dec_delay_slot_w)
    ,.fetch_fault_fetch_o(fetch_dec_fault_fetch_w)
    ,.icache_rd_o(mem_i_rd_o)
    ,.icache_flush_o(mem_i_flush_o)
    ,.icache_invalidate_o(mem_i_invalidate_o)
    ,.icache_pc_o(mem_i_pc_o)
    ,.icache_priv_o()
);



endmodule
