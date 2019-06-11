/****************************************************************************
		  ISA definition file

  - The MIPS I ISA has a 6 bit opcode in the upper 6 bits.  
  - The opcode can also specify a "class".  There are two classes:
			1.  SPECIAL - look in lowest 6 bits to find operation
			2.  REGIMM - look in [20:16] to find type of branch

****************************************************************************/

/****** OPCODES - bits 31...26 *******/

`define VAL 31

`define WIDTH 32
`define NUMREGS 32
`define LOG2NUMREGS 5
`define PC_WIDTH 30
`define I_DATAWIDTH 32
`define I_ADDRESSWIDTH 14
`define I_SIZE 16384

`define D_ADDRESSWIDTH 32
`define DM_DATAWIDTH 32
`define DM_BYTEENAWIDTH 4
`define DM_ADDRESSWIDTH 10
`define DM_SIZE 16384



`define     OP_SPECIAL       6'b000000
`define     OP_REGIMM        6'b000001
`define     OP_J             6'b000010
`define     OP_JAL           6'b000011
`define     OP_BEQ           6'b000100
`define     OP_BNE           6'b000101
`define     OP_BLEZ          6'b000110
`define     OP_BGTZ          6'b000111

`define     OP_ADDI          6'b001000
`define     OP_ADDIU         6'b001001
`define     OP_SLTI          6'b001010
`define     OP_SLTIU         6'b001011
`define     OP_ANDI          6'b001100
`define     OP_ORI           6'b001101
`define     OP_XORI          6'b001110
`define     OP_LUI           6'b001111

`define     OP_LB            6'b100000
`define     OP_LH            6'b100001
`define     OP_LWL           6'b100010
`define     OP_LW            6'b100011
`define     OP_LBU           6'b100100
`define     OP_LHU           6'b100101
`define     OP_LWR           6'b100110

`define     OP_SB            6'b101100
`define     OP_SH            6'b101101
`define     OP_SWL           6'b101010
`define     OP_SW            6'b101111
`define     OP_SWR           6'b101110

/****** FUNCTION CLASS - bits 5...0 *******/
`define     FUNC_SLL         6'b000000
`define     FUNC_SRL         6'b000010
`define     FUNC_SRA         6'b000011
`define     FUNC_SLLV        6'b000100
`define     FUNC_SRLV        6'b000110
`define     FUNC_SRAV        6'b000111

`define     FUNC_JR          6'b001110
`define     FUNC_JALR        6'b001111

`define     FUNC_MFHI        6'b110100
`define     FUNC_MTHI        6'b110101
`define     FUNC_MFLO        6'b110110
`define     FUNC_MTLO        6'b110111

`define     FUNC_MULT        6'b111100
`define     FUNC_MULTU       6'b111101
`define     FUNC_DIV         6'b111110
`define     FUNC_DIVU        6'b111111

`define     FUNC_ADD         6'b100000
`define     FUNC_ADDU        6'b100001
`define     FUNC_SUB         6'b100010
`define     FUNC_SUBU        6'b100011
`define     FUNC_AND         6'b100100
`define     FUNC_OR          6'b100101
`define     FUNC_XOR         6'b100110
`define     FUNC_NOR         6'b100111

`define     FUNC_SLT         6'b101010
`define     FUNC_SLTU        6'b101011

/****** REGIMM Class - bits 20...16 *******/
`define     FUNC_BLTZ        1'b0
`define     FUNC_BGEZ        1'b1

`define     OP_COP2        6'b010010
`define     COP2_FUNC_CFC2      6'b111000
`define     COP2_FUNC_CTC2      6'b111010
`define     COP2_FUNC_MTC2      6'b111011

//`define     FUNC_BLTZAL      5'b10000
//`define     FUNC_BGEZAL      5'b10001

/****** 
 * Original REGIMM class, compressed above to save decode logic
`define     FUNC_BLTZ        5'b00000
`define     FUNC_BGEZ        5'b00001
`define     FUNC_BLTZAL      5'b10000
`define     FUNC_BGEZAL      5'b10001
*/


module system ( 
	clk,
	resetn,
	boot_iaddr,
	boot_idata,
	boot_iwe,
	boot_daddr,
	boot_ddata,
	boot_dwe,
	nop7_q
);

/************************* IO Declarations *********************/
input clk;
input resetn;
input [31:0] boot_iaddr;
input [31:0] boot_idata;
input boot_iwe;
input [31:0] boot_daddr;
input [31:0] boot_ddata;
input boot_dwe;
output [31:0] nop7_q;


/*********************** Signal Declarations *******************/
wire	branch_mispred;
wire	stall_2nd_delayslot;
wire	has_delayslot;
wire	haz_zeroer0_q_pipereg5_q;
wire	haz_zeroer_q_pipereg5_q;
		// Datapath signals declarations
wire	addersub_result_slt;
wire	[ 31 : 0 ]	addersub_result;
wire	[ 31 : 0 ]	reg_file_b_readdataout;
wire	[ 31 : 0 ]	reg_file_a_readdataout;
wire	[ 31 : 0 ]	mul_shift_result;
wire	[ 31 : 0 ]	mul_lo;
wire	[ 31 : 0 ]	mul_hi;
wire	ctrl_mul_stalled;
wire	[ 31 : 0 ]	ifetch_pc_out;
wire	[ 31 : 0 ]	ifetch_instr;
wire	[ 5 : 0 ]	ifetch_opcode;
wire	[ 5 : 0 ]	ifetch_func;
wire	[4 : 0 ]	ifetch_rs;
wire	[ 4 : 0 ]	ifetch_rt;
wire	[ 4 : 0 ]	ifetch_rd;
wire	[ 25 : 0 ]	ifetch_instr_index;
wire	[ 15 : 0 ]	ifetch_offset;
wire	[ 4 : 0 ]	ifetch_sa;
wire	[ 31 : 0 ]	ifetch_next_pc;
wire	[ 31 : 0 ]	data_mem_d_loadresult;
wire	ctrl_data_mem_stalled;
wire	[ 31 : 0 ]	logic_unit_result;
wire	[ 31 : 0 ]	pcadder_result;
wire	[ 31 : 0 ]	signext16_out;
wire	[ 31 : 0 ]	merge26lo_out;
wire	[ 31 : 0 ]	hi_reg_q;
wire	branchresolve_eqz;
wire	branchresolve_gez;
wire	branchresolve_gtz;
wire	branchresolve_lez;
wire	branchresolve_ltz;
wire	branchresolve_ne;
wire	branchresolve_eq;
wire	[ 31 : 0 ]	lo_reg_q;
wire	[ 31 : 0 ]	const8_out;
wire	[ 31 : 0 ]	const9_out;
wire	[ 31 : 0 ]	const_out;
wire	[ 31 : 0 ]	pipereg_q;
wire	[ 25 : 0 ]	pipereg1_q;
wire	[ 4 : 0 ]	pipereg2_q;
wire	[ 31 : 0 ]	pipereg5_q;
wire	[ 31 : 0 ]	pipereg14_q;
wire	[ 31 : 0 ]	pipereg3_q;
wire	[ 31 : 0 ]	nop7_q;
wire	[ 31 : 0 ]	nop_q;
wire	[ 31 : 0 ]	nop10_q;
wire	[ 31 : 0 ]	nop6_q;
wire	[ 31 : 0 ]	zeroer_q;
wire	[ 31 : 0 ]	zeroer0_q;
wire	[ 31 : 0 ]	zeroer4_q;
wire	[ 31 : 0 ]	fakedelay_q;
wire	[ 31 : 0 ]	mux3to1_ifetch_load_data_out;
wire	[ 31 : 0 ]	mux2to1_mul_opA_out;
wire	mux6to1_ifetch_load_out;
wire	[ 4 : 0 ]	mux3to1_mul_sa_out;
wire	[ 31 : 0 ]	mux2to1_addersub_opA_out;
wire	[ 31 : 0 ]	mux7to1_nop10_d_out;
wire	[ 31 : 0 ]	mux2to1_pipereg_d_out;
wire	[ 31 : 0 ]	mux3to1_nop6_d_out;
wire	[ 31 : 0 ]	mux3to1_zeroer4_d_out;
wire	[ 5 : 0 ]	pipereg11_q;
wire	[ 31 : 0 ]	mux2to1_nop_d_out;
wire	pipereg16_q;
wire	pipereg15_q;
wire	[ 31 : 0 ]	mux2to1_nop7_d_out;
wire	[ 5 : 0 ]	pipereg12_q;
wire	[ 4 : 0 ]	pipereg13_q;
/***************** Control Signals ***************/
		//Decoded Opcode signal declarations
reg	ctrl_mux2to1_pipereg_d_sel;
reg	[ 2 : 0 ]	ctrl_mux7to1_nop10_d_sel;
reg	ctrl_mux2to1_addersub_opA_sel;
reg	[ 2 : 0 ]	ctrl_mux6to1_ifetch_load_sel;
reg	[ 1 : 0 ]	ctrl_mux3to1_nop6_d_sel;
reg	ctrl_mux2to1_mul_opA_sel;
reg	[ 1 : 0 ]	ctrl_mux3to1_mul_sa_sel;
reg	[ 1 : 0 ]	ctrl_mux3to1_ifetch_load_data_sel;
reg	[ 1 : 0 ]	ctrl_mux3to1_zeroer4_d_sel;
reg	ctrl_zeroer4_en;
reg	ctrl_zeroer0_en;
reg	ctrl_zeroer_en;
reg	[ 3 : 0 ]	ctrl_data_mem_op;
reg	[ 2 : 0 ]	ctrl_addersub_op;
reg	ctrl_ifetch_op;
reg	[ 2 : 0 ]	ctrl_mul_op;
reg	[ 1 : 0 ]	ctrl_logic_unit_op;
		//Enable signal declarations
reg	ctrl_hi_reg_en;
reg	ctrl_lo_reg_en;
reg	ctrl_branchresolve_en;
reg	ctrl_reg_file_c_we;
reg	ctrl_reg_file_b_en;
reg	ctrl_reg_file_a_en;
reg	ctrl_data_mem_en;
reg	ctrl_ifetch_we;
reg	ctrl_ifetch_en;
reg	ctrl_mul_start;
		//Other Signals
wire	squash_stage2;
wire	stall_out_stage2;
wire	squash_stage1;
wire	stall_out_stage1;
wire	ctrl_pipereg16_squashn;
wire	ctrl_pipereg15_squashn;
wire	ctrl_pipereg14_squashn;
wire	ctrl_pipereg_squashn;
wire	ctrl_pipereg5_squashn;
wire	ctrl_pipereg2_squashn;
wire	ctrl_pipereg3_squashn;
wire	ctrl_pipereg1_squashn;
wire	ctrl_pipereg11_squashn;
wire	ctrl_pipereg12_squashn;
wire	ctrl_pipereg13_squashn;
wire	ctrl_pipereg16_resetn;
wire	ctrl_pipereg15_resetn;
wire	ctrl_pipereg14_resetn;
wire	ctrl_pipereg_resetn;
wire	ctrl_pipereg5_resetn;
wire	ctrl_pipereg2_resetn;
wire	ctrl_pipereg3_resetn;
wire	ctrl_pipereg1_resetn;
wire	ctrl_pipereg11_resetn;
wire	ctrl_pipereg12_resetn;
wire	ctrl_pipereg13_resetn;
wire	ctrl_pipereg16_en;
wire	ctrl_pipereg15_en;
wire	ctrl_pipereg14_en;
wire	ctrl_pipereg_en;
wire	ctrl_pipereg5_en;
wire	ctrl_pipereg2_en;
wire	ctrl_pipereg3_en;
wire	ctrl_pipereg1_en;
wire	ctrl_pipereg11_en;
wire	ctrl_pipereg12_en;
wire	ctrl_pipereg13_en;
wire crtl_ifetch_squashn;

/****************************** Control **************************/
		//Decode Logic for Opcode and Multiplex Select signals
always@(posedge clk)
begin
		// Initialize control opcodes to zero
	
	case (ifetch_opcode)
		`OP_ADDI:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_mux3to1_zeroer4_d_sel <= 2'b10;
			ctrl_zeroer4_en <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
		`OP_ADDIU:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_mux3to1_zeroer4_d_sel <= 2'b10;
			ctrl_zeroer4_en <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
		`OP_ANDI:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_mux3to1_zeroer4_d_sel <= 2'b10;
			ctrl_zeroer4_en <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
		`OP_BEQ:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_zeroer0_en <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
		`OP_BGTZ:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
		`OP_BLEZ:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
		`OP_BNE:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_zeroer0_en <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
		`OP_JAL:
		begin
			ctrl_mux3to1_zeroer4_d_sel <= 2'b10;
			ctrl_zeroer4_en <= 1'b1;
		end
		`OP_LB:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_mux3to1_zeroer4_d_sel <= 2'b10;
			ctrl_zeroer4_en <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
		`OP_LBU:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_mux3to1_zeroer4_d_sel <= 2'b10;
			ctrl_zeroer4_en <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
		`OP_LH:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_mux3to1_zeroer4_d_sel <= 2'b10;
			ctrl_zeroer4_en <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
		`OP_LHU:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_mux3to1_zeroer4_d_sel <= 2'b10;
			ctrl_zeroer4_en <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
		`OP_LUI:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_mux3to1_zeroer4_d_sel <= 2'b10;
			ctrl_zeroer4_en <= 1'b1;
		end
		`OP_LW:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_mux3to1_zeroer4_d_sel <= 2'b10;
			ctrl_zeroer4_en <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
		`OP_ORI:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_mux3to1_zeroer4_d_sel <= 2'b10;
			ctrl_zeroer4_en <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
		`OP_REGIMM:
		case (ifetch_rt[0])
			`FUNC_BGEZ:
			begin
				ctrl_mux2to1_pipereg_d_sel <= 1'b1;
				ctrl_zeroer_en <= 1'b1;
			end
			`FUNC_BLTZ:
			begin
				ctrl_mux2to1_pipereg_d_sel <= 1'b1;
				ctrl_zeroer_en <= 1'b1;
			end
		endcase
		`OP_SB:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_zeroer0_en <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
		`OP_SH:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_zeroer0_en <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
		`OP_SLTI:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_mux3to1_zeroer4_d_sel <= 2'b10;
			ctrl_zeroer4_en <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
		`OP_SLTIU:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_mux3to1_zeroer4_d_sel <= 2'b10;
			ctrl_zeroer4_en <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
		`OP_SPECIAL:
		case (ifetch_func)
			`FUNC_ADD:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
				ctrl_zeroer0_en <= 1'b1;
				ctrl_zeroer_en <= 1'b1;
			end
			`FUNC_ADDU:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
				ctrl_zeroer0_en <= 1'b1;
				ctrl_zeroer_en <= 1'b1;
			end
			`FUNC_AND:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
				ctrl_zeroer0_en <= 1'b1;
				ctrl_zeroer_en <= 1'b1;
			end
			`FUNC_JALR:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
				ctrl_zeroer_en <= 1'b1;
			end
			`FUNC_JR:
				ctrl_zeroer_en <= 1'b1;
			`FUNC_MFHI:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
			end
			`FUNC_MFLO:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
			end
			`FUNC_MULT:
			begin
				ctrl_zeroer0_en <= 1'b1;
				ctrl_zeroer_en <= 1'b1;
			end
			`FUNC_MULTU:
			begin
				ctrl_zeroer0_en <= 1'b1;
				ctrl_zeroer_en <= 1'b1;
			end
			`FUNC_NOR:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
				ctrl_zeroer0_en <= 1'b1;
				ctrl_zeroer_en <= 1'b1;
			end
			`FUNC_OR:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
				ctrl_zeroer0_en <= 1'b1;
				ctrl_zeroer_en <= 1'b1;
			end
			`FUNC_SLL:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
				ctrl_zeroer0_en <= 1'b1;
			end
			`FUNC_SLLV:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
				ctrl_zeroer0_en <= 1'b1;
				ctrl_zeroer_en <= 1'b1;
			end
			`FUNC_SLT:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
				ctrl_zeroer0_en <= 1'b1;
				ctrl_zeroer_en <= 1'b1;
			end
			`FUNC_SLTU:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
				ctrl_zeroer0_en <= 1'b1;
				ctrl_zeroer_en <= 1'b1;
			end
			`FUNC_SRA:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
				ctrl_zeroer0_en <= 1'b1;
			end
			`FUNC_SRAV:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
				ctrl_zeroer0_en <= 1'b1;
				ctrl_zeroer_en <= 1'b1;
			end
			`FUNC_SRL:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
				ctrl_zeroer0_en <= 1'b1;
			end
			`FUNC_SRLV:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
				ctrl_zeroer0_en <= 1'b1;
				ctrl_zeroer_en <= 1'b1;
			end
			`FUNC_SUB:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
				ctrl_zeroer0_en <= 1'b1;
				ctrl_zeroer_en <= 1'b1;
			end
			`FUNC_SUBU:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
				ctrl_zeroer0_en <= 1'b1;
				ctrl_zeroer_en <= 1'b1;
			end
			`FUNC_XOR:
			begin
				ctrl_mux3to1_zeroer4_d_sel <= 2'b01;
				ctrl_zeroer4_en <= 1'b1;
				ctrl_zeroer0_en <= 1'b1;
				ctrl_zeroer_en <= 1'b1;
			end
		endcase
		`OP_SW:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_zeroer0_en <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
		`OP_XORI:
		begin
			ctrl_mux2to1_pipereg_d_sel <= 1'b1;
			ctrl_mux3to1_zeroer4_d_sel <= 2'b10;
			ctrl_zeroer4_en <= 1'b1;
			ctrl_zeroer_en <= 1'b1;
		end
	endcase

		//Logic for enable signals in Pipe Stage 1
	ctrl_reg_file_b_en <= ~stall_out_stage2;
	ctrl_reg_file_a_en <= ~stall_out_stage2;
	ctrl_ifetch_en <= ~stall_out_stage2;

		//Decode Logic for Opcode and Multiplex Select signals


		// Initialize control opcodes to zero

	
	case (pipereg11_q)
		`OP_ADDI:
		begin
			ctrl_mux7to1_nop10_d_sel <= 3'b110;
			ctrl_mux2to1_addersub_opA_sel <= 1'b0;
			ctrl_mux3to1_nop6_d_sel <= 2'b10;
			ctrl_addersub_op <= 3'b011;
		end
		`OP_ADDIU:
		begin
			ctrl_mux7to1_nop10_d_sel <= 3'b110;
			ctrl_mux2to1_addersub_opA_sel <= 1'b0;
			ctrl_mux3to1_nop6_d_sel <= 2'b10;
			ctrl_addersub_op <= 3'b001;
		end
		`OP_ANDI:
		begin
			ctrl_mux7to1_nop10_d_sel <= 3'b100;
			ctrl_mux3to1_nop6_d_sel <= 2'b10;
			ctrl_logic_unit_op <= 2'b00;
		end
		`OP_BEQ:
		begin
			ctrl_mux6to1_ifetch_load_sel <= 3'b101;
			ctrl_mux3to1_ifetch_load_data_sel<= 2'b10;
			ctrl_ifetch_op <= 1'b0;
		end
		`OP_BGTZ:
		begin
			ctrl_mux6to1_ifetch_load_sel <= 3'b000;
			ctrl_mux3to1_ifetch_load_data_sel<= 2'b10;
			ctrl_ifetch_op <= 1'b0;
		end
		`OP_BLEZ:
		begin
			ctrl_mux6to1_ifetch_load_sel <= 3'b011;
			ctrl_mux3to1_ifetch_load_data_sel<= 2'b10;
			ctrl_ifetch_op <= 1'b0;
		end
		`OP_BNE:
		begin
			ctrl_mux6to1_ifetch_load_sel <= 3'b100;
			ctrl_mux3to1_ifetch_load_data_sel<= 2'b10;
			ctrl_ifetch_op <= 1'b0;
		end
		`OP_J:
		begin
			ctrl_mux3to1_ifetch_load_data_sel<= 2'b01;
			ctrl_ifetch_op <= 1'b1;
		end
		`OP_JAL:
		begin
			ctrl_mux7to1_nop10_d_sel <= 3'b110;
			ctrl_mux2to1_addersub_opA_sel <= 1'b1;
			ctrl_mux3to1_ifetch_load_data_sel<= 2'b01;
			ctrl_addersub_op <= 3'b001;
			ctrl_ifetch_op <= 1'b1;
		end
		`OP_LB:
		begin
			ctrl_mux7to1_nop10_d_sel <= 3'b010;
			ctrl_mux2to1_addersub_opA_sel <= 1'b0;
			ctrl_mux3to1_nop6_d_sel <= 2'b10;
			ctrl_data_mem_op <= 4'b0111;
			ctrl_addersub_op <= 3'b011;
		end
		`OP_LBU:
		begin
			ctrl_mux7to1_nop10_d_sel <= 3'b010;
			ctrl_mux2to1_addersub_opA_sel <= 1'b0;
			ctrl_mux3to1_nop6_d_sel <= 2'b10;
			ctrl_data_mem_op <= 4'b0011;
			ctrl_addersub_op <= 3'b011;
		end
		`OP_LH:
		begin
			ctrl_mux7to1_nop10_d_sel <= 3'b010;
			ctrl_mux2to1_addersub_opA_sel <= 1'b0;
			ctrl_mux3to1_nop6_d_sel <= 2'b10;
			ctrl_data_mem_op <= 4'b0101;
			ctrl_addersub_op <= 3'b011;
		end
		`OP_LHU:
		begin
			ctrl_mux7to1_nop10_d_sel <= 3'b010;
			ctrl_mux2to1_addersub_opA_sel <= 1'b0;
			ctrl_mux3to1_nop6_d_sel <= 2'b10;
			ctrl_data_mem_op <= 4'b0001;
			ctrl_addersub_op <= 3'b011;
		end
		`OP_LUI:
		begin
			ctrl_mux7to1_nop10_d_sel <= 3'b011;
			ctrl_mux3to1_nop6_d_sel <= 2'b10;
			ctrl_mux2to1_mul_opA_sel <= 1'b0;
			ctrl_mux3to1_mul_sa_sel <= 2'b01;
			ctrl_mul_op <= 3'b000;
		end
		`OP_LW:
		begin
			ctrl_mux7to1_nop10_d_sel <= 3'b010;
			ctrl_mux2to1_addersub_opA_sel <= 1'b0;
			ctrl_mux3to1_nop6_d_sel <= 2'b10;
			ctrl_data_mem_op <= 4'b0000;
			ctrl_addersub_op <= 3'b011;
		end
		`OP_ORI:
		begin
			ctrl_mux7to1_nop10_d_sel <= 3'b100;
			ctrl_mux3to1_nop6_d_sel <= 2'b10;
			ctrl_logic_unit_op <= 2'b01;
		end
		`OP_REGIMM:
		case (pipereg13_q[0])
			`FUNC_BGEZ:
			begin
				ctrl_mux6to1_ifetch_load_sel <= 3'b001;
				ctrl_mux3to1_ifetch_load_data_sel<= 2'b10;
				ctrl_ifetch_op <= 1'b0;
			end
			`FUNC_BLTZ:
			begin
				ctrl_mux6to1_ifetch_load_sel <= 3'b010;
				ctrl_mux3to1_ifetch_load_data_sel<= 2'b10;
				ctrl_ifetch_op <= 1'b0;
			end
		endcase
		`OP_SB:
		begin
			ctrl_mux2to1_addersub_opA_sel <= 1'b0;
			ctrl_mux3to1_nop6_d_sel <= 2'b10;
			ctrl_data_mem_op <= 4'b0011;
			ctrl_addersub_op <= 3'b011;
		end
		`OP_SH:
		begin
			ctrl_mux2to1_addersub_opA_sel <= 1'b0;
			ctrl_mux3to1_nop6_d_sel <= 2'b10;
			ctrl_data_mem_op <= 4'b1001;
			ctrl_addersub_op <= 3'b011;
		end
		`OP_SLTI:
		begin
			ctrl_mux7to1_nop10_d_sel <= 3'b101;
			ctrl_mux2to1_addersub_opA_sel <= 1'b0;
			ctrl_mux3to1_nop6_d_sel <= 2'b10;
			ctrl_addersub_op <= 3'b101;
		end
		`OP_SLTIU:
		begin
			ctrl_mux7to1_nop10_d_sel <= 3'b101;
			ctrl_mux2to1_addersub_opA_sel <= 1'b0;
			ctrl_mux3to1_nop6_d_sel <= 2'b10;
			ctrl_addersub_op <= 3'b100;
		end
		`OP_SPECIAL:
		case (pipereg12_q)
			`FUNC_ADD:
			begin
				ctrl_mux7to1_nop10_d_sel <= 3'b110;
				ctrl_mux2to1_addersub_opA_sel <= 1'b0;
				ctrl_mux3to1_nop6_d_sel <= 2'b01;
				ctrl_addersub_op <= 3'b011;
			end
			`FUNC_ADDU:
			begin
				ctrl_mux7to1_nop10_d_sel <= 3'b110;
				ctrl_mux2to1_addersub_opA_sel <= 1'b0;
				ctrl_mux3to1_nop6_d_sel <= 2'b01;
				ctrl_addersub_op <= 3'b001;
			end
			`FUNC_AND:
			begin
				ctrl_mux7to1_nop10_d_sel <= 3'b100;
				ctrl_mux3to1_nop6_d_sel <= 2'b01;
				ctrl_logic_unit_op <= 2'b00;
			end
			`FUNC_JALR:
			begin
				ctrl_mux7to1_nop10_d_sel <= 3'b110;
				ctrl_mux2to1_addersub_opA_sel <= 1'b1;
				ctrl_mux3to1_ifetch_load_data_sel<= 2'b00;
				ctrl_addersub_op <= 3'b001;
				ctrl_ifetch_op <= 1'b1;
			end
			`FUNC_JR:
			begin
				ctrl_mux3to1_ifetch_load_data_sel<= 2'b00;
				ctrl_ifetch_op <= 1'b1;
			end
			`FUNC_MFHI:
				ctrl_mux7to1_nop10_d_sel <= 3'b001;
			`FUNC_MFLO:
				ctrl_mux7to1_nop10_d_sel <= 3'b000;
			`FUNC_MULT:
			begin
				ctrl_mux2to1_mul_opA_sel <= 1'b1;
				ctrl_mul_op <= 3'b110;
			end
			`FUNC_MULTU:
			begin
				ctrl_mux2to1_mul_opA_sel <= 1'b1;
				ctrl_mul_op <= 3'b100;
			end
			`FUNC_NOR:
			begin
				ctrl_mux7to1_nop10_d_sel <= 3'b100;
				ctrl_mux3to1_nop6_d_sel <= 2'b01;
				ctrl_logic_unit_op <= 2'b11;
			end
			`FUNC_OR:
			begin
				ctrl_mux7to1_nop10_d_sel <= 3'b100;
				ctrl_mux3to1_nop6_d_sel <= 2'b01;
				ctrl_logic_unit_op <= 2'b01;
			end
			`FUNC_SLL:
			begin
				ctrl_mux7to1_nop10_d_sel <= 3'b011;
				ctrl_mux3to1_nop6_d_sel <= 2'b01;
				ctrl_mux2to1_mul_opA_sel <= 1'b0;
				ctrl_mux3to1_mul_sa_sel <= 2'b00;
				ctrl_mul_op <= 3'b000;
			end
			`FUNC_SLLV:
			begin
				ctrl_mux7to1_nop10_d_sel <= 3'b011;
				ctrl_mux3to1_nop6_d_sel <= 2'b01;
				ctrl_mux2to1_mul_opA_sel <= 1'b0;
				ctrl_mux3to1_mul_sa_sel <= 2'b10;
				ctrl_mul_op <= 3'b000;
			end
			`FUNC_SLT:
			begin
				ctrl_mux7to1_nop10_d_sel <= 3'b101;
				ctrl_mux2to1_addersub_opA_sel <= 1'b0;
				ctrl_mux3to1_nop6_d_sel <= 2'b01;
				ctrl_addersub_op <= 3'b110;
			end
			`FUNC_SLTU:
			begin
				ctrl_mux7to1_nop10_d_sel <= 3'b101;
				ctrl_mux2to1_addersub_opA_sel <= 1'b0;
				ctrl_mux3to1_nop6_d_sel <= 2'b01;
				ctrl_addersub_op <= 3'b100;
			end
			`FUNC_SRA:
			begin
				ctrl_mux7to1_nop10_d_sel <= 3'b011;
				ctrl_mux3to1_nop6_d_sel <= 2'b01;
				ctrl_mux2to1_mul_opA_sel <= 1'b0;
				ctrl_mux3to1_mul_sa_sel <= 2'b00;
				ctrl_mul_op <= 3'b011;
			end
			`FUNC_SRAV:
			begin
				ctrl_mux7to1_nop10_d_sel <= 3'b011;
				ctrl_mux3to1_nop6_d_sel <= 2'b01;
				ctrl_mux2to1_mul_opA_sel <= 1'b0;
				ctrl_mux3to1_mul_sa_sel <= 2'b10;
				ctrl_mul_op <= 3'b011;
			end
			`FUNC_SRL:
			begin
				ctrl_mux7to1_nop10_d_sel <= 3'b011;
				ctrl_mux3to1_nop6_d_sel <= 2'b01;
				ctrl_mux2to1_mul_opA_sel <= 1'b0;
				ctrl_mux3to1_mul_sa_sel <= 2'b00;
				ctrl_mul_op <= 3'b001;
			end
			`FUNC_SRLV:
			begin
				ctrl_mux7to1_nop10_d_sel <= 3'b011;
				ctrl_mux3to1_nop6_d_sel <= 2'b01;
				ctrl_mux2to1_mul_opA_sel <= 1'b0;
				ctrl_mux3to1_mul_sa_sel <= 2'b10;
				ctrl_mul_op <= 3'b001;
			end
			`FUNC_SUB:
			begin
				ctrl_mux7to1_nop10_d_sel <= 3'b110;
				ctrl_mux2to1_addersub_opA_sel <= 1'b0;
				ctrl_mux3to1_nop6_d_sel <= 2'b01;
				ctrl_addersub_op <= 3'b000;
			end
			`FUNC_SUBU:
			begin
				ctrl_mux7to1_nop10_d_sel <= 3'b110;
				ctrl_mux2to1_addersub_opA_sel <= 1'b0;
				ctrl_mux3to1_nop6_d_sel <= 2'b01;
				ctrl_addersub_op <= 3'b010;
			end
			`FUNC_XOR:
			begin
				ctrl_mux7to1_nop10_d_sel <= 3'b100;
				ctrl_mux3to1_nop6_d_sel <= 2'b01;
				ctrl_logic_unit_op <= 2'b10;
			end
		endcase
		`OP_SW:
		begin
			ctrl_mux2to1_addersub_opA_sel <= 1'b0;
			ctrl_mux3to1_nop6_d_sel <= 2'b10;
			ctrl_data_mem_op <= 4'b1000;
			ctrl_addersub_op <= 3'b011;
		end
		`OP_XORI:
		begin
			ctrl_mux7to1_nop10_d_sel <= 3'b100;
			ctrl_mux3to1_nop6_d_sel <= 2'b10;
			ctrl_logic_unit_op <= 2'b10;
		end
	endcase


	
		//Logic for enable signals in Pipe Stage 2



	case (pipereg11_q)
		`OP_ADDI:
			ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
		`OP_ADDIU:
			ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
		`OP_ANDI:
			ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
		`OP_BEQ:
		begin
			ctrl_branchresolve_en <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			ctrl_ifetch_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
		end
		`OP_BGTZ:
		begin
			ctrl_branchresolve_en <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			ctrl_ifetch_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
		end
		`OP_BLEZ:
		begin
			ctrl_branchresolve_en <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			ctrl_ifetch_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
		end
		`OP_BNE:
		begin
			ctrl_branchresolve_en <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			ctrl_ifetch_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
		end
		`OP_J:
			ctrl_ifetch_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
		`OP_JAL:
		begin
			ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			ctrl_ifetch_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
		end
		`OP_LB:
		begin
			ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			ctrl_data_mem_en <=1'b1;
		end
		`OP_LBU:
		begin
			ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			ctrl_data_mem_en <=1'b1;
		end
		`OP_LH:
		begin
			ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			ctrl_data_mem_en <=1'b1;
		end
		`OP_LHU:
		begin
			ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			ctrl_data_mem_en <=1'b1;
		end
		`OP_LUI:
		begin
			ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			ctrl_mul_start <=1'b1;
		end
		`OP_LW:
		begin
			ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			ctrl_data_mem_en <=1'b1;
		end
		`OP_ORI:
			ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
		`OP_REGIMM:
		case (pipereg13_q[0])
			`FUNC_BGEZ:
			begin
				ctrl_branchresolve_en <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
				ctrl_ifetch_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			end
			`FUNC_BLTZ:
			begin
				ctrl_branchresolve_en <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
				ctrl_ifetch_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			end
		endcase
		`OP_SB:
			ctrl_data_mem_en <=1'b1;
		`OP_SH:
			ctrl_data_mem_en <=1'b1;
		`OP_SLTI:
			ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
		`OP_SLTIU:
			ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
		`OP_SPECIAL:
		case (pipereg12_q)
			`FUNC_ADD:
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			`FUNC_ADDU:
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			`FUNC_AND:
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			`FUNC_JALR:
			begin
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
				ctrl_ifetch_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			end
			`FUNC_JR:
				ctrl_ifetch_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			`FUNC_MFHI:
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			`FUNC_MFLO:
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			`FUNC_MULT:
			begin
				ctrl_hi_reg_en <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
				ctrl_lo_reg_en <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
				ctrl_mul_start <=1'b1;
			end
			`FUNC_MULTU:
			begin
				ctrl_hi_reg_en <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
				ctrl_lo_reg_en <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
				ctrl_mul_start <=1'b1;
			end
			`FUNC_NOR:
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			`FUNC_OR:
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			`FUNC_SLL:
			begin
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
				ctrl_mul_start <=1'b1;
			end
			`FUNC_SLLV:
			begin
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
				ctrl_mul_start <=1'b1;
			end
			`FUNC_SLT:
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			`FUNC_SLTU:
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			`FUNC_SRA:
			begin
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
				ctrl_mul_start <=1'b1;
			end
			`FUNC_SRAV:
			begin
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
				ctrl_mul_start <=1'b1;
			end
			`FUNC_SRL:
			begin
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
				ctrl_mul_start <=1'b1;
			end
			`FUNC_SRLV:
			begin
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
				ctrl_mul_start <=1'b1;
			end
			`FUNC_SUB:
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			`FUNC_SUBU:
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
			`FUNC_XOR:
				ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
		endcase
		`OP_SW:
			ctrl_data_mem_en <=1'b1;
		`OP_XORI:
			ctrl_reg_file_c_we <=~ctrl_data_mem_stalled&~ctrl_mul_stalled;
	endcase
end

/********* Stall Network & PipeReg Control ********/
assign stall_out_stage1 = stall_out_stage2;
assign ctrl_pipereg13_en = ~stall_out_stage1;
assign ctrl_pipereg12_en = ~stall_out_stage1;
assign ctrl_pipereg11_en = ~stall_out_stage1;
assign ctrl_pipereg1_en = ~stall_out_stage1;
assign ctrl_pipereg3_en = ~stall_out_stage1;
assign ctrl_pipereg2_en = ~stall_out_stage1;
assign ctrl_pipereg5_en = ~stall_out_stage1;
assign ctrl_pipereg_en = ~stall_out_stage1;
assign ctrl_pipereg14_en = ~stall_out_stage1;
assign ctrl_pipereg15_en = ~stall_out_stage1;
assign ctrl_pipereg16_en = ~stall_out_stage1;
assign stall_out_stage2 = ctrl_data_mem_stalled|ctrl_mul_stalled;
assign branch_mispred = (((ctrl_ifetch_op==1) || (ctrl_ifetch_op==0 && mux6to1_ifetch_load_out)) & ctrl_ifetch_we);
assign stall_2nd_delayslot = &has_delayslot;
assign has_delayslot = 0;
assign squash_stage1 = ((stall_out_stage1&~stall_out_stage2))|~resetn;
assign ctrl_pipereg13_resetn = ~squash_stage1;
assign ctrl_pipereg12_resetn = ~squash_stage1;
assign ctrl_pipereg11_resetn = ~squash_stage1;
assign ctrl_pipereg1_resetn = ~squash_stage1;
assign ctrl_pipereg3_resetn = ~squash_stage1;
assign ctrl_pipereg2_resetn = ~squash_stage1;
assign ctrl_pipereg5_resetn = ~squash_stage1;
assign ctrl_pipereg_resetn = ~squash_stage1;
assign ctrl_pipereg14_resetn = ~squash_stage1;
assign ctrl_pipereg15_resetn = ~squash_stage1;
assign ctrl_pipereg16_resetn = ~squash_stage1;
assign ctrl_pipereg16_squashn = 1'b1;
assign ctrl_pipereg15_squashn = 1'b1;
assign ctrl_pipereg14_squashn = 1'b1;
assign ctrl_pipereg_squashn = 1'b1;
assign ctrl_pipereg5_squashn = 1'b1;
assign ctrl_pipereg2_squashn = 1'b1;
assign ctrl_pipereg3_squashn = 1'b1;
assign ctrl_pipereg1_squashn = 1'b1;
assign ctrl_pipereg11_squashn = 1'b1;
assign ctrl_pipereg12_squashn = 1'b1;
assign ctrl_pipereg13_squashn = 1'b1;
assign ctrl_ifetch_squashn = 1'b1;
wire ctrl_ifetch_squashn;

assign squash_stage2 = ((stall_out_stage2))|~resetn;

/****************************** Datapath **************************/
/******************** Hazard Detection Logic ***********************/
assign haz_zeroer0_q_pipereg5_q = (zeroer0_q==pipereg5_q) && (|zeroer0_q);
assign haz_zeroer_q_pipereg5_q = (zeroer_q==pipereg5_q) && (|zeroer_q);
assign const8_out = 32'b00000000000000000000000000000000;
assign const9_out = 32'b00000000000000000000000000010000;
assign const_out =  32'b00000000000000000000000000011111;




/*************** DATAPATH COMPONENTS **************/
addersub addersub (
	.opB(nop6_q),
	.opA(mux2to1_addersub_opA_out),
	.op(ctrl_addersub_op),
	.result_slt(addersub_result_slt),
	.result(addersub_result));
//	defparam
	//	addersub.WIDTH=32;

reg_file reg_file (
	.clk(clk),
	.resetn(resetn),
	.c_writedatain(nop10_q),
	.c_reg(pipereg5_q),
	.b_reg(zeroer0_q),
	.a_reg(zeroer_q),
	.c_we(ctrl_reg_file_c_we),
	.b_en(ctrl_reg_file_b_en),
	.a_en(ctrl_reg_file_a_en),
	.b_readdataout(reg_file_b_readdataout),
	.a_readdataout(reg_file_a_readdataout));

mul mul (
	.clk(clk),
	.resetn(resetn),
	.sa(mux3to1_mul_sa_out),
	.dst(pipereg5_q),
	.opB(nop7_q),
	.opA(mux2to1_mul_opA_out),
	.op(ctrl_mul_op),
	.start(ctrl_mul_start),
	.stalled(ctrl_mul_stalled),
	.shift_result(mul_shift_result),
	.lo(mul_lo),
	.hi(mul_hi));
//	defparam
	//	mul.WIDTH=32;

ifetch ifetch (
	.clk(clk),
	.resetn(resetn),
	.boot_iaddr(boot_iaddr),
	.boot_idata(boot_idata),
	.boot_iwe(boot_iwe),
	.load(mux6to1_ifetch_load_out),
	.load_data(mux3to1_ifetch_load_data_out),
	.op(ctrl_ifetch_op),
	.we(ctrl_ifetch_we),
	.squashn(ctrl_ifetch_squashn),
	.en(ctrl_ifetch_en),
	.pc_out(ifetch_pc_out),
	.instr(ifetch_instr),
	.opcode(ifetch_opcode),
	.func(ifetch_func),
	.rs(ifetch_rs),
	.rt(ifetch_rt),
	.rd(ifetch_rd),
	.instr_index(ifetch_instr_index),
	.offset(ifetch_offset),
	.sa(ifetch_sa),
	.next_pc(ifetch_next_pc));
	
data_mem data_mem (
	.clk(clk),
	.resetn(resetn),
	.stalled(ctrl_data_mem_stalled),
	.d_writedata(nop7_q),
	.d_address(addersub_result),
	.op(ctrl_data_mem_op),
	.d_loadresult(data_mem_d_loadresult));

logic_unit logic_unit (
	.opB(nop6_q),
	.opA(nop_q),
	.op(ctrl_logic_unit_op),
	.result(logic_unit_result));
//	defparam
	//	logic_unit.WIDTH=32;

pcadder pcadder (
	.offset(pipereg_q),
	.pc(pipereg3_q),
	.result(pcadder_result));

signext16 signext16 (
	.in(ifetch_offset),
	.out(signext16_out));

merge26lo merge26lo (
	.in2(pipereg1_q),
	.in1(pipereg3_q),
	.out(merge26lo_out));

hi_reg hi_reg (
	.clk(clk),
	.resetn(resetn),
	.d(mul_hi),
	.en(ctrl_hi_reg_en),
	.q(hi_reg_q));
//	defparam
//		hi_reg.WIDTH=32;

branchresolve branchresolve (
	.rt(nop7_q),
	.rs(nop_q),
	.en(ctrl_branchresolve_en),
	.eqz(branchresolve_eqz),
	.gez(branchresolve_gez),
	.gtz(branchresolve_gtz),
	.lez(branchresolve_lez),
	.ltz(branchresolve_ltz),
	.ne(branchresolve_ne),
	.eq(branchresolve_eq));
//	defparam
//		branchresolve.WIDTH=32;

lo_reg lo_reg (
	.clk(clk),
	.resetn(resetn),
	.d(mul_lo),
	.en(ctrl_lo_reg_en),
	.q(lo_reg_q));
//	defparam
//		lo_reg.WIDTH=32;

/*
const const8 (
	.out(const8_out));
//	defparam
	//	const8.WIDTH=32,
		//const8.VAL=0;

const const9 (
	.out(const9_out));
//	defparam
	//	const9.WIDTH=32,
		//const9.VAL=16;

const const (
	.out(const_out));
//	defparam
	//	const.WIDTH=32,
		//const.VAL=31;
		*/

pipereg_w32 pipereg (
	.clk(clk),
	.resetn(ctrl_pipereg_resetn),
	.d(mux2to1_pipereg_d_out),
	.squashn(ctrl_pipereg_squashn),
	.en(ctrl_pipereg_en),
	.q(pipereg_q));
//	defparam
	//	pipereg.WIDTH=32;

pipereg_w26 pipereg1 (
	.clk(clk),
	.resetn(ctrl_pipereg1_resetn),
	.d(ifetch_instr_index),
	.squashn(ctrl_pipereg1_squashn),
	.en(ctrl_pipereg1_en),
	.q(pipereg1_q));
//	defparam
	//	pipereg1.WIDTH=26;

pipereg_w5 pipereg2 (
	.clk(clk),
	.resetn(ctrl_pipereg2_resetn),
	.d(ifetch_sa),
	.squashn(ctrl_pipereg2_squashn),
	.en(ctrl_pipereg2_en),
	.q(pipereg2_q));
//	defparam
	//	pipereg2.WIDTH=5;

pipereg_w5 pipereg5 (
	.clk(clk),
	.resetn(ctrl_pipereg5_resetn),
	.d(zeroer4_q),
	.squashn(ctrl_pipereg5_squashn),
	.en(ctrl_pipereg5_en),
	.q(pipereg5_q));
	//defparam
		//pipereg5.WIDTH=5;

pipereg_w32 pipereg14 (
	.clk(clk),
	.resetn(ctrl_pipereg14_resetn),
	.d(nop10_q),
	.squashn(ctrl_pipereg14_squashn),
	.en(ctrl_pipereg14_en),
	.q(pipereg14_q));
	//defparam
	//	pipereg14.WIDTH=32;

pipereg_w32 pipereg3 (
	.clk(clk),
	.resetn(ctrl_pipereg3_resetn),
	.d(ifetch_pc_out),
	.squashn(ctrl_pipereg3_squashn),
	.en(ctrl_pipereg3_en),
	.q(pipereg3_q));
//	defparam
//		pipereg3.WIDTH=32;

nop nop7 (
	.d(mux2to1_nop7_d_out),
	.q(nop7_q));
	//defparam
//		nop7.WIDTH=32;

nop nop (
	.d(mux2to1_nop_d_out),
	.q(nop_q));
	//defparam
	//	nop.WIDTH=32;

nop nop10 (
	.d(mux7to1_nop10_d_out),
	.q(nop10_q));
	//defparam
	//	nop10.WIDTH=32;

nop nop6 (
	.d(mux3to1_nop6_d_out),
	.q(nop6_q));
	//defparam
	//	nop6.WIDTH=32;

zeroer zeroer (
	.d(ifetch_rs),
	.en(ctrl_zeroer_en),
	.q(zeroer_q));
	//defparam
	//	zeroer.WIDTH=5;

zeroer zeroer0 (
	.d(ifetch_rt),
	.en(ctrl_zeroer0_en),
	.q(zeroer0_q));
	//defparam
	//	zeroer0.WIDTH=5;

zeroer zeroer4 (
	.d(mux3to1_zeroer4_d_out),
	.en(ctrl_zeroer4_en),
	.q(zeroer4_q));
	//defparam
	//	zeroer4.WIDTH=5;

fakedelay fakedelay (
	.clk(clk),
	.d(ifetch_pc_out),
	.q(fakedelay_q));
	//defparam
	//	fakedelay.WIDTH=32;

		// Multiplexor mux3to1_ifetch_load_data instantiation
assign mux3to1_ifetch_load_data_out = 
	(ctrl_mux3to1_ifetch_load_data_sel==2) ? pcadder_result :
	(ctrl_mux3to1_ifetch_load_data_sel==1) ? merge26lo_out :
	nop_q;

		// Multiplexor mux2to1_mul_opA instantiation
assign mux2to1_mul_opA_out = 
	(ctrl_mux2to1_mul_opA_sel==1) ? nop_q :
	nop6_q;

		// Multiplexor mux6to1_ifetch_load instantiation
assign mux6to1_ifetch_load_out = 
	(ctrl_mux6to1_ifetch_load_sel==3'd5) ? branchresolve_eq :
	(ctrl_mux6to1_ifetch_load_sel==3'd4) ? branchresolve_ne :
	(ctrl_mux6to1_ifetch_load_sel==3'd3) ? branchresolve_lez :
	(ctrl_mux6to1_ifetch_load_sel==3'd2) ? branchresolve_ltz :
	(ctrl_mux6to1_ifetch_load_sel==3'd1) ? branchresolve_gez :
	branchresolve_gtz;

		// Multiplexor mux3to1_mul_sa instantiation
assign mux3to1_mul_sa_out = 
	(ctrl_mux3to1_mul_sa_sel==2) ? nop_q :
	(ctrl_mux3to1_mul_sa_sel==1) ? const9_out :
	pipereg2_q;

		// Multiplexor mux2to1_addersub_opA instantiation
assign mux2to1_addersub_opA_out = 
	(ctrl_mux2to1_addersub_opA_sel==1) ? fakedelay_q :
	nop_q;

		// Multiplexor mux7to1_nop10_d instantiation
assign mux7to1_nop10_d_out = 
	(ctrl_mux7to1_nop10_d_sel==3'd6) ? addersub_result :
	(ctrl_mux7to1_nop10_d_sel==3'd5) ? addersub_result_slt :
	(ctrl_mux7to1_nop10_d_sel==3'd4) ? logic_unit_result :
	(ctrl_mux7to1_nop10_d_sel==3'd3) ? mul_shift_result :
	(ctrl_mux7to1_nop10_d_sel==3'd2) ? data_mem_d_loadresult :
	(ctrl_mux7to1_nop10_d_sel==3'd1) ? hi_reg_q :
	lo_reg_q;

		// Multiplexor mux2to1_pipereg_d instantiation
assign mux2to1_pipereg_d_out = 
	(ctrl_mux2to1_pipereg_d_sel==1) ? ifetch_offset :
	signext16_out;

		// Multiplexor mux3to1_nop6_d instantiation
assign mux3to1_nop6_d_out = 
	(ctrl_mux3to1_nop6_d_sel==2) ? pipereg_q :
	(ctrl_mux3to1_nop6_d_sel==1) ? nop7_q :
	const8_out;

		// Multiplexor mux3to1_zeroer4_d instantiation
assign mux3to1_zeroer4_d_out = 
	(ctrl_mux3to1_zeroer4_d_sel==2) ? ifetch_rt :
	(ctrl_mux3to1_zeroer4_d_sel==1) ? ifetch_rd :
	const_out;

pipereg_w6 pipereg11 (
	.clk(clk),
	.resetn(ctrl_pipereg11_resetn),
	.d(ifetch_opcode),
	.squashn(ctrl_pipereg11_squashn),
	.en(ctrl_pipereg11_en),
	.q(pipereg11_q));
	//defparam
	//	pipereg11.WIDTH=6;

		// Multiplexor mux2to1_nop_d instantiation
assign mux2to1_nop_d_out = 
	(pipereg15_q==1) ? pipereg14_q :
	reg_file_a_readdataout;

pipereg_w1 pipereg16 (
	.clk(clk),
	.resetn(ctrl_pipereg16_resetn),
	.d(haz_zeroer0_q_pipereg5_q),
	.squashn(ctrl_pipereg16_squashn),
	.en(ctrl_pipereg16_en),
	.q(pipereg16_q));
	//defparam
	//	pipereg16.WIDTH=1;

pipereg_w1 pipereg15 (
	.clk(clk),
	.resetn(ctrl_pipereg15_resetn),
	.d(haz_zeroer_q_pipereg5_q),
	.squashn(ctrl_pipereg15_squashn),
	.en(ctrl_pipereg15_en),
	.q(pipereg15_q));
	//defparam
	//	pipereg15.WIDTH=1;

		// Multiplexor mux2to1_nop7_d instantiation
assign mux2to1_nop7_d_out = 
	(pipereg16_q==1) ? pipereg14_q :
	reg_file_b_readdataout;

pipereg_w6 pipereg12 (
	.clk(clk),
	.resetn(ctrl_pipereg12_resetn),
	.d(ifetch_func),
	.squashn(ctrl_pipereg12_squashn),
	.en(ctrl_pipereg12_en),
	.q(pipereg12_q));
	//defparam
	//	pipereg12.WIDTH=6;

pipereg_w5 pipereg13 (
	.clk(clk),
	.resetn(ctrl_pipereg13_resetn),
	.d(ifetch_rt),
	.squashn(ctrl_pipereg13_squashn),
	.en(ctrl_pipereg13_en),
	.q(pipereg13_q));
	//defparam
	//	pipereg13.WIDTH=5;



endmodule

/****************************************************************************
		  AddSub unit
- Should perform ADD, ADDU, SUBU, SUB, SLT, SLTU

  is_slt signext addsub
	op[2] op[1] op[0]  |  Operation
0     0     0     0         SUBU
2     0     1     0         SUB
1     0     0     1         ADDU
3     0     1     1         ADD
4     1     0     0         SLTU
6     1     1     0         SLT

****************************************************************************/
module addersub (
	opB, 
	opA,
	op, 
	result_slt,
	result 
);

//parameter WIDTH=32;
//`DEFINE WIDTH 32


input [31:0] opA;
input [31:0] opB;
//input carry_in;
input [2:0] op;
output result_slt;
output [31:0] result;



wire [32:0] sum;


wire addsub;
wire useless;
assign useless = op[1] & op[2];


assign addsub=op[0];
wire not_addsub;
assign not_addsub = ~addsub;

assign result=sum[31:0];

assign result_slt=sum[32];

dummy_add_sub adder32bit (opA,opB,not_addsub,sum);


// This is an LPM from Altera, replacing with a dummy one for now
/*
lpm_add_sub adder_inst(
	.dataa({signext&opA[WIDTH-1],opA}),
	.datab({signext&opB[WIDTH-1],opB}),
	.cin(~addsub),
	.add_sub(addsub),
	.result(sum)
		// synopsys translate_off
		,
		.cout (),
		.clken (),
		.clock (),
		.overflow (),
		.aclr ()
		// synopsys translate_on
	);
//defparam 
//    adder_inst.lpm_width=WIDTH+1,
//   adder_inst.lpm_representation="SIGNED";
*/

endmodule





module dummy_add_sub (
	dataa,
	datab,
	cin,
	result
);

//this is goign to be UUUUGGGGGGLLLYYYYY
//probably going to do some serious timing violations
// but i'm sure it will be interesting for the packing problem
input [31:0] dataa;
input [31:0] datab;
input cin;
output [32:0] result;
//
wire [31:0] carry_from;
wire [31:0] sum;


full_adder bit0 (cin,dataa[0],datab[0],sum[0],carry_from [0]);
full_adder bit1 (carry_from [0],dataa[1],datab[1],sum[1],carry_from [1]);
full_adder bit2 (carry_from [1],dataa[2],datab[2],sum[2],carry_from [2]);
full_adder bit3 (carry_from [2],dataa[3],datab[3],sum[3],carry_from [3]);
full_adder bit4 (carry_from [3],dataa[4],datab[4],sum[4],carry_from [4]);
full_adder bit5 (carry_from [4],dataa[5],datab[5],sum[5],carry_from [5]);
full_adder bit6 (carry_from [5],dataa[6],datab[6],sum[6],carry_from [6]);
full_adder bit7 (carry_from [6],dataa[7],datab[7],sum[7],carry_from [7]);

full_adder bit8 (carry_from [7],dataa[8],datab[8],sum[8],carry_from [8]);
full_adder bit9 (carry_from [8],dataa[9],datab[9],sum[9],carry_from [9]);
full_adder bit10 (carry_from [9],dataa[10],datab[10],sum[10],carry_from [10]);
full_adder bit11 (carry_from [10],dataa[11],datab[11],sum[11],carry_from [11]);
full_adder bit12 (carry_from [11],dataa[12],datab[12],sum[12],carry_from [12]);
full_adder bit13 (carry_from [12],dataa[13],datab[13],sum[13],carry_from [13]);
full_adder bit14 (carry_from [13],dataa[14],datab[14],sum[14],carry_from [14]);
full_adder bit15 (carry_from [14],dataa[15],datab[15],sum[15],carry_from [15]);

full_adder bit16 (carry_from [15],dataa[16],datab[16],sum[16],carry_from [16]);
full_adder bit17 (carry_from [16],dataa[17],datab[17],sum[17],carry_from [17]);
full_adder bit18 (carry_from [17],dataa[18],datab[18],sum[18],carry_from [18]);
full_adder bit19 (carry_from [18],dataa[19],datab[19],sum[19],carry_from [19]);
full_adder bit20 (carry_from [19],dataa[20],datab[20],sum[20],carry_from [20]);
full_adder bit21 (carry_from [20],dataa[21],datab[21],sum[21],carry_from [21]);
full_adder bit22 (carry_from [21],dataa[22],datab[22],sum[22],carry_from [22]);
full_adder bit23 (carry_from [22],dataa[23],datab[23],sum[23],carry_from [23]);

full_adder bit24 (carry_from [23],dataa[24],datab[24],sum[24],carry_from [24]);
full_adder bit25 (carry_from [24],dataa[25],datab[25],sum[25],carry_from [25]);
full_adder bit26 (carry_from [25],dataa[26],datab[26],sum[26],carry_from [26]);
full_adder bit27 (carry_from [26],dataa[27],datab[27],sum[27],carry_from [27]);
full_adder bit28 (carry_from [27],dataa[28],datab[28],sum[28],carry_from [28]);
full_adder bit29 (carry_from [28],dataa[29],datab[29],sum[29],carry_from [29]);
full_adder bit30 (carry_from [29],dataa[30],datab[30],sum[30],carry_from [30]);
full_adder bit31 (carry_from [30],dataa[31],datab[31],sum[31],carry_from [31]);

assign result [31:0] = sum;
assign result [32] = carry_from [31];

endmodule


module full_adder (cin,x,y,s,cout);
input cin;
input x;
input y;
output s;
output cout;
assign s = x^y^cin;
assign cout = (x&y) | (x & cin) | (y&cin);
endmodule

/****************************************************************************
		  Register File

   - Has two read ports (a and b) and one write port (c)
   - sel chooses the register to be read/written
****************************************************************************/

module reg_file(
	clk, 
	resetn, 
	c_writedatain,
	c_reg,
	b_reg,
	a_reg,
	c_we, 
	b_en, 
	a_en,
	b_readdataout, 
	a_readdataout
);
//parameter WIDTH=32;
//parameter NUMREGS=32;
//parameter LOG2NUMREGS=5;
input clk;
input resetn;

input a_en;
input b_en;
input [31:0] c_writedatain;
input c_we;
input [31:0] a_reg;
input [31:0] b_reg;
input [31:0] c_reg;
output [31:0] a_readdataout;
output [31:0] b_readdataout;
reg [31:0] a_readdataout;
reg [31:0] b_readdataout;


wire [31:0] a_readdataout_temp;
wire [31:0] b_readdataout_temp;


assign b_readdataout = b_readdataout_temp;
assign a_readdataout = a_readdataout_temp;

wire wren1;
assign wren1 = (c_we & (|c_reg));
single_port_ram regfile1_replace (
	.clk (clk),
	.we(wren1),
	.data(c_writedatain),
	.out(a_readdataout_temp),
	.addr(c_reg[4:0])
);

//Reg file duplicated to avoid contention 
//between 2 read and 1 write
//MORE MEMORY

single_port_ram regfile2_replace(
	.clk (clk),
	.we(wren1),
	.data(c_writedatain),
	.out(b_readdataout_temp),
	.addr(c_reg[4:0])
);		

//Odin II does not recognize that address 
//registers are being used to read and 
//write data, so they are assigned to an 
//unused wire which is later dropped by the
//optimizer.
wire useless_inputs;
//`a_reg` and `b_reg` were not used correctly in last version 
//of `spree.v` according to the comment above this module.
//Investigate whether the comment or the code is wrong
assign useless_inputs = resetn & b_en & a_en & ( | a_reg ) & ( | b_reg );
endmodule

/****************************************************************************
		  MUL/DIV unit

Operation table

   op sign dir
4  1   0    x    |  MULTU
6  1   1    x    |  MULT
0  0   0    0    |  ShiftLeft
1  0   0    1    |  ShiftRightLogic
3  0   1    1    |  ShiftRightArith
****************************************************************************/
module mul(
	clk,
	resetn,
	sa,
	dst, 
	opB,
	opA,
	op, 
	start, 
	stalled,
	shift_result,
	lo,
	hi
);

input clk;
input resetn;

input start;
output stalled;

input [4:0] dst;

input [31:0] opA;
input [31:0] opB;
input [4:0] sa;
input [2:0] op;

output [31:0] shift_result;
output [31:0] hi;
output [31:0] lo;

/********* Control Signals *********/
wire is_signed;
wire dir;
wire is_mul;
assign is_mul=op[2];      // selects between opB and the computed shift amount
assign is_signed=op[1];
assign dir=op[0];         // selects between 2^sa and 2^(32-sa) for right shift

/********* Circuit Body *********/
wire dum;
wire dum2; 
wire dum3;
wire [32:0] opB_mux_out;
wire [4:0] left_sa;     // Amount of left shift required for both left/right
reg [32:0] decoded_sa;
wire [31:0] result;
//assign opB_mux_out= (is_mul) ? {is_signed&opB[31],opB} : decoded_sa;
assign opB_mux_out = opB;



dummy_mult fake_mult_one (opA,opB_mux_out, clk, resetn, result);
assign hi = result [15:8];
assign lo = result [7:0];
// Cannot support this now
/*
lpm_mult  lpm_mult_component (
  .dataa ({is_signed&opA[31],opA}),
  .datab (opB_mux_out),
  .sum(),
  .clock(clk),
  .clken(),
  .aclr(~resetn),
  .result ({dum2,dum,hi,lo}));
defparam
  lpm_mult_component.lpm_32a = 32+1,
  lpm_mult_component.lpm_32b = 32+1,
  lpm_mult_component.lpm_32p = 2*32+2,
  lpm_mult_component.lpm_32s = 1,
  lpm_mult_component.lpm_pipeline = 1,
  lpm_mult_component.lpm_type = "LPM_MULT",
  lpm_mult_component.lpm_representation = "SIGNED",
  lpm_mult_component.lpm_hint = "MAXIMIZE_SPEED=6";
*/
assign shift_result= (dir & |sa) ? hi : lo;


// 1 cycle stall state machine
wire or_dst;
wire start_and_ismul;
wire request;

assign or_dst = |dst;
assign start_and_ismul = start & is_mul; 
assign request = (or_dst & start & ~is_mul) | (start_and_ismul);
onecyclestall staller(request,clk,resetn,stalled);


endmodule

module dummy_mult  (
	opA,
	opB_mux_out, 
	clk, 
	resetn, 
	result
);

input [31:0] opA;
input [31:0] opB_mux_out;
input  clk;
input  resetn;
output[31:0] result;
reg [31:0] result;


always @ (posedge clk)
begin
	if (resetn)
		result <= 32'b00000000000000000000000000000000;
	else
		//multiplier by star symbol
		//though this is probably supposed to be signed
		result <= opA * opB_mux_out;
end
endmodule
		

/****************************************************************************
			Fetch Unit
op
  0  Conditional PC write
  1  UnConditional PC write

****************************************************************************/

module ifetch(
	clk,
	resetn,
	boot_iaddr, 
	boot_idata, 
	boot_iwe,
	load,
	load_data,
	op,
	we,
	squashn,
	en,
	pc_out,
	instr,
	opcode,
	func,
	rs,
	rt,
	rd,
	instr_index,
	offset,
	sa,
	next_pc
);

//parameter PC_WIDTH=30;
//parameter I_DATAWIDTH=32;
//parameter I_ADDRESSWIDTH=14;
//parameter I_SIZE=16384;

input [31:0] boot_iaddr;
input [31:0] boot_idata;
input boot_iwe;

input clk;
input resetn;
input en;     // PC increment enable
input we;     // PC write enable
input squashn;// squash fetch
input op;     // determines if conditional or unconditional branch
input load;
input [`I_DATAWIDTH-1:0] load_data;
output [`I_DATAWIDTH-1:0] pc_out;   // output pc + 1 shifted left 2 bits
output [`PC_WIDTH-1:0] next_pc;
output [31:26] opcode;
output [25:21] rs;
output [20:16] rt;
output [15:11] rd;
output [10:6] sa;
output [15:0] offset;
output [25:0] instr_index;
output [5:0] func;
output [`I_DATAWIDTH-1:0] instr;


wire [`PC_WIDTH-1:0] pc_plus_1;
wire [`PC_WIDTH-1:0] pc;
assign pc_plus_1 = pc;
wire ctrl_load;
wire out_of_sync;

assign ctrl_load=(load&~op|op);
wire notresetn;
assign notresetn = ~resetn;
wire count_en;
assign count_en = (~ctrl_load)&~out_of_sync;
wire counter_en;
assign counter_en = en | we;
wire [32:2] reg_load_data;

assign reg_load_data = load_data [31:2];

wire reg_d;
wire reg_en;
assign reg_d = (we&(~en)&(squashn));
assign reg_en = en|we;


register_1bit sync_pcs_up( reg_d, clk, resetn,reg_en, out_of_sync);

wire wren1;
assign wren1 = 1'b0;
wire [9:0] next_pc_wire;
assign next_pc_wire = next_pc [9:0];

wire [31:0]dummyout2;

dual_port_ram imem_replace(
	.clk (clk),
	.we1(wren1),
	.we2(boot_iwe),
	.data1(load_data),
	.data2(boot_idata),
	.out1(instr),
	.out2(dummyout2),
	.addr1(next_pc_wire),
	.addr2(boot_iaddr[9:0])
);

wire [31:0] dummyin1;
assign dummyin1 = 32'b00000000000000000000000000000000;

dummy_counter pc_reg ((reg_load_data),(clk),(counter_en),(count_en),(notresetn),(ctrl_load),(pc));
assign pc_out [31:2] = pc_plus_1;
assign pc_out [1:0] = 2'b00;

assign next_pc = ctrl_load ? load_data[31:2] : pc_plus_1;
assign opcode=instr[31:26];
assign rs=instr[25:21];
assign rt=instr[20:16];
assign rd=instr[15:11];
assign sa=instr[10:6];
assign offset=instr[15:0]; 
assign instr_index=instr[25:0];
assign func=instr[5:0];

//Odin II does not recognize that boot_iaddr 
//is being used to write data when system 
//is given 1'b1 on the boot_iwe wire so is
//is assigned to an unused wire which is 
//later dropped by the optimizer.
wire NoUse;
assign NoUse = ( |boot_iaddr );

endmodule


module dummy_counter (
	data,
	clock,
	clk_en,
	cnt_en,
	aset,
	sload,
	q
);

input [31:2] data;
input clock;
input clk_en;
input cnt_en;
input aset;
input sload;
output [`PC_WIDTH-1:0] q;
reg [`PC_WIDTH-1:0] q;

wire [2:0] sload_cnten_aset;
assign sload_cnten_aset [0] = sload;
assign sload_cnten_aset [1] = cnt_en;
assign sload_cnten_aset [2] = aset;

always @ (posedge clock)

//if (cnt_en == 1)
//q <= q+1;
begin

case (sload_cnten_aset)
	3'b000:
		q <= q;
	3'b011:
		q <= q;
	3'b110:
		q <= q;
	3'b111:
		q <= q;
	3'b101:
		q <= q;
	3'b100: 
		q <= data;
	3'b010:
	begin
		if (clk_en) 
			q <= q+1;
		else
			q <= q;
		end
	3'b001:
		q <= 29'b00000000000000000000000000000;
	default:
		q <= q;
endcase
end
endmodule




module data_mem(
	clk, 
	resetn, 
	stalled,
	d_writedata,
	d_address,
	op,
	d_loadresult
);

input clk;
input resetn;
output stalled;

input [`D_ADDRESSWIDTH-1:0] d_address;
input [3:0] op;
input [31:0] d_writedata;
output [`DM_DATAWIDTH-1:0] d_loadresult;

wire [`DM_BYTEENAWIDTH-1:0] d_byteena;
wire [`DM_DATAWIDTH-1:0] d_readdatain;
wire [`DM_DATAWIDTH-1:0] d_writedatamem;

wire d_write;
wire [1:0] d_address_latched;

assign d_write=op[3];

wire [1:0] d_small_adr;
assign d_small_adr = d_address[1:0];

wire one;
assign one = 1'b1;
				

wire [1:0] d_adr_one_zero;
assign d_adr_one_zero = d_address [1:0];

wire [1:0] opsize;
assign opsize = op[1:0];
wire opext;
assign opext = op[2];


store_data_translator sdtrans_inst(
	.write_data(d_writedata),
	.d_address(d_adr_one_zero),
	.store_size(op[1:0]),
	.d_byteena(d_byteena),
	.d_writedataout(d_writedatamem)
);



load_data_translator ldtrans_inst(
	.d_readdatain(d_readdatain),  
	.d_loadresult(d_loadresult)
);


wire  dnot_address;
assign dnot_address = ~d_address[31];
wire will_be_wren1;
assign will_be_wren1 = d_write&(dnot_address);

wire [9:0] memaddr_wrd;


assign memaddr_wrd = d_address[`DM_ADDRESSWIDTH:2];
single_port_ram dmem_replace(
	.clk (clk),
	.we(will_be_wren1),
	.data(d_writedatamem),
	.out(d_readdatain),
	.addr(memaddr_wrd)
);
// 1 cycle stall state machine

wire en_and_not_d_write;
assign en_and_not_d_write = ~d_write;
onecyclestall staller(en_and_not_d_write,clk,resetn,stalled);

wire useless_inputs;
assign useless_inputs = |d_address;

endmodule

//temp in here




/****************************************************************************
		  Store data translator
		  - moves store data to appropriate byte/halfword 
		  - interfaces with altera blockrams
****************************************************************************/
module store_data_translator(
	write_data,    // data in least significant position
	d_address,
	store_size,
	d_byteena,
	d_writedataout // shifted data to coincide with address
);

//parameter WIDTH=32;

input [31:0] write_data;
input [1:0] d_address;
input [1:0] store_size;
output [3:0] d_byteena;
output [31:0] d_writedataout;

reg [3:0] d_byteena;
reg [31:0] d_writedataout;

always @(write_data or d_address or store_size)
begin
	case (store_size)
		2'b11:
			case(d_address[1:0])
				2'b00: 
				begin 
					d_byteena=4'b1000; 
					d_writedataout={write_data[7:0],24'b0}; 
				end
				2'b01: 
				begin 
					d_byteena=4'b0100; 
					d_writedataout={8'b0,write_data[7:0],16'b0}; 
				end
				2'b10: 
				begin 
					d_byteena=4'b0010; 
					d_writedataout={16'b0,write_data[7:0],8'b0}; 
				end
				default: 
				begin 
					d_byteena=4'b0001; 
					d_writedataout={24'b0,write_data[7:0]}; 
				end
			endcase
		2'b01:
			case(d_address[1])
				1'b0: 
				begin 
					d_byteena=4'b1100; 
					d_writedataout={write_data[15:0],16'b0}; 
				end
				default: 
				begin 
					d_byteena=4'b0011; 
					d_writedataout={16'b0,write_data[15:0]}; 
				end
			endcase
		default:
		begin
			d_byteena=4'b1111;
			d_writedataout=write_data;
		end
	endcase
end

endmodule

/****************************************************************************
		  Load data translator
		  - moves read data to appropriate byte/halfword and zero/sign extends
****************************************************************************/

module load_data_translator(
	d_readdatain,
	d_loadresult
);

//parameter WIDTH=32;

input [31:0] d_readdatain;

output [31:0] d_loadresult;

wire d_adr_one;
assign d_adr_one = d_address [1];
reg [31:0] d_loadresult;
reg sign;
wire [1:0] d_address;
assign d_address [1:0] =d_readdatain [25:24];


//assume always full-word-access
always @(d_readdatain or d_address )
begin
	d_loadresult[31:0]=d_readdatain[31:0];
end
/*
Odin II REFUSES TO ACKNOWLEDGE THAT SIGN EXTENDING IS NOT A COMBINATIONAL LOOP
always @(d_readdatain or d_address or load_size or load_sign_ext)
begin
	case (load_size)
		2'b11:
		begin
			case (d_address)
				2'b00:
					begin
					d_loadresult[7:0]=d_readdatain[31:24];
					sign = d_readdatain[31];
					end
				2'b01:
						begin
						d_loadresult[7:0]=d_readdatain[23:16];
						sign = d_readdatain[23];
						end
				2'b10: 
					begin
					d_loadresult[7:0]=d_readdatain[15:8];
					sign = d_readdatain[15];
					end
				default: 
					begin
					d_loadresult[7:0]=d_readdatain[7:0];
					sign = d_readdatain[7];
					end
			endcase
			// peter milankov note: do this by hand
			// odin II does not support multiple concatenation
			//d_loadresult[31:8]={24{load_sign_ext&d_loadresult[7]}};
			d_loadresult[31]= load_sign_ext&sign;
			d_loadresult[30]= load_sign_ext&sign;
			d_loadresult[29]= load_sign_ext&sign;
			d_loadresult[28]= load_sign_ext&sign;
			d_loadresult[27]= load_sign_ext&sign;
			d_loadresult[26]= load_sign_ext&sign;
			d_loadresult[25]= load_sign_ext&sign;
			d_loadresult[24]= load_sign_ext&sign;
			d_loadresult[23]= load_sign_ext&sign;
			d_loadresult[22]= load_sign_ext&sign;
			d_loadresult[21]= load_sign_ext&sign;
			d_loadresult[20]= load_sign_ext&sign;
			d_loadresult[19]= load_sign_ext&sign;
			d_loadresult[18]= load_sign_ext&sign;
			d_loadresult[17]= load_sign_ext&sign;
			d_loadresult[16]= load_sign_ext&sign;
			d_loadresult[15]= load_sign_ext&sign;
			d_loadresult[14]= load_sign_ext&sign;
			d_loadresult[13]= load_sign_ext&sign;
			d_loadresult[12]= load_sign_ext&sign;
			d_loadresult[11]= load_sign_ext&sign;
			d_loadresult[10]= load_sign_ext&sign;
			d_loadresult[9]= load_sign_ext&sign;
			d_loadresult[8]= load_sign_ext&sign;
		end
		2'b01:
		begin
			case (d_adr_one)
				1'b0:
					begin
						d_loadresult[15:0]=d_readdatain[31:16];
						sign = d_readdatain[31];
					end
				default:
					begin
						d_loadresult[15:0]=d_readdatain[15:0];
						sign = d_readdatain[15];
					end
			endcase
// peter milankov note sign extend is concat, do by hand
			//d_loadresult[31:16]={16{load_sign_ext&d_loadresult[15]}};
			d_loadresult[31]= load_sign_ext&sign;
			d_loadresult[30]= load_sign_ext&sign;
			d_loadresult[29]= load_sign_ext&sign;
			d_loadresult[28]= load_sign_ext&sign;
			d_loadresult[27]= load_sign_ext&sign;
			d_loadresult[26]= load_sign_ext&sign;
			d_loadresult[25]= load_sign_ext&sign;
			d_loadresult[24]= load_sign_ext&sign;
			d_loadresult[23]= load_sign_ext&sign;
			d_loadresult[22]= load_sign_ext&sign;
			d_loadresult[21]= load_sign_ext&sign;
			d_loadresult[20]= load_sign_ext&sign;
			d_loadresult[19]= load_sign_ext&sign;
			d_loadresult[18]= load_sign_ext&sign;
			d_loadresult[17]= load_sign_ext&sign;
			d_loadresult[16]= load_sign_ext&sign;
		end
		default:
			d_loadresult[31:0]=d_readdatain[31:0];
	endcase
end
*/
endmodule

/****************************************************************************
		  logic unit
- note ALU must be able to increment PC for JAL type instructions

Operation Table
  op
  0     AND
  1     OR
  2     XOR
  3     NOR
****************************************************************************/
module logic_unit (
	opB, 
	opA,
	op,
	result
);

//parameter WIDTH=32;

input [31:0] opA;
input [31:0] opB;
input [1:0] op;
output [31:0] result;

reg [31:0] logic_result;

always@(opA or opB or op )
	case(op)
		2'b00:
			logic_result=opA&opB;
		2'b01:
			logic_result=opA|opB;
		2'b10:
			logic_result=opA^opB;
		2'b11:
			logic_result=~(opA|opB);
	endcase

assign result=logic_result;


endmodule

module pcadder(
	offset,
	pc, 
	result
);

//parameter PC_WIDTH=32;


input [31:0] pc;
input [31:0] offset;
output [31:0] result;

wire dum;
wire useless_inputs;
assign useless_inputs = |offset;
assign {dum,result} = pc + {offset[31:0],2'b0};

endmodule


module signext16 (in, out);

input [15:0] in;
output [31:0] out;


assign out [30]= in[15];
assign out [31]= in[15];
assign out [29]= in[15];
assign out [28]= in[15];
assign out [27]= in[15];
assign out [26]= in[15];
assign out [25]= in[15];
assign out [24]= in[15];
assign out [23]= in[15];
assign out [22]= in[15];
assign out [21]= in[15];
assign out [20]= in[15];
assign out [19]= in[15];
assign out [18]= in[15];
assign out [17]= in[15];
assign out [16]= in[15];
assign out [15:0] = in [15:0];

endmodule


module merge26lo(in2, in1, out);
input [31:0] in1;
input [25:0] in2;
output [31:0] out;

//assign out[31:0]={in1[31:28],in2[25:0],2'b0};

assign out [31:28] = in1 [31:28];
assign out [27:2] = in2 [25:0];
assign out [1:0] = 2'b00;


wire useless_inputs;
assign useless_inputs = |in1 & |in2;
endmodule

/****************************************************************************
		  Generic Register
****************************************************************************/
module lo_reg(
	clk,
	resetn,
	d,
	en,
	q
);

//parameter WIDTH=32;


input clk;
input resetn;
input en;
input [31:0] d;
output [31:0] q;
reg [31:0] q;

always @(posedge clk )		
begin
	if (resetn==0)
		q<=0;
	else if (en==1)
		q<=d;
end

endmodule

/****************************************************************************
		  Generic Register
****************************************************************************/
module hi_reg(
	clk,
	resetn,
	d,
	en,
	q
);

//parameter WIDTH=32;


input clk;
input resetn;
input en;
input [31:0] d;
output [31:0] q;
reg [31:0] q;

always @(posedge clk )		//used to be asynchronous reset
begin
	if (resetn==0)
		q<=0;
	else if (en==1)
		q<=d;
end

endmodule

/****************************************************************************
		  Generic Register
****************************************************************************/

//`define WIDTH 32
/*
module register(d,clk,resetn,en,q);
//parameter WIDTH=32;






input clk;
input resetn;
input en;
input [31:0] d;
output [31:0] q;
reg [31:0] q;

always @(posedge clk )		
begin
	if (resetn==0)
		q<=0;
	else if (en==1)
		q<=d;
end

endmodule
*/
module register_1bit(
	d,
	clk,
	resetn,
	en,
	q
);

//parameter WIDTH=32;

input clk;
input resetn;
input en;
input  d;
output q;
reg q;

always @(posedge clk )		
begin
	if (resetn==0)
		q<=0;
	else if (en==1)
		q<=d;
end

endmodule


/****************************************************************************
		  Generic Register - synchronous reset
****************************************************************************/
/*
module register_sync(d,clk,resetn,en,q);
//parameter WIDTH=32;

input clk;
input resetn;
input en;
input [31:0] d;
output [31:0] q;
reg [31:0] q;

always @(posedge clk)		//synchronous reset
begin
	if (resetn==0)
		q<=0;
	else if (en==1)
		q<=d;
end

endmodule
*/
/****************************************************************************
		  Generic Pipelined Register

		  - Special component, components starting with "pipereg" have
		  their enables treated independently of instructrions that use them.
		  - They are enabled whenever the stage is active and not stalled
****************************************************************************/
/*
module pipereg(clk,resetn,d,squashn,en,q);
//parameter WIDTH=32;
//`define WIDTH 32

input clk;
input resetn;
input en;
input squashn;
input [31:0] d;
output [31:0] q;
reg [31:0] q;

always @(posedge clk)   //synchronous reset
begin
  if (resetn==0 || squashn==0)
	q<=0;
  else if (en==1)
	q<=d;
end

endmodule
*/
module pipereg_w32(
	clk,
	resetn,
	d,
	squashn,
	en,
	q
);

//parameter WIDTH=32;
//`define WIDTH 32

input clk;
input resetn;
input en;
input squashn;
input [31:0] d;
output [31:0] q;
reg [31:0] q;

always @(posedge clk)   //synchronous reset
begin
  if (resetn==0 || squashn==0)
	q<=0;
  else if (en==1)
	q<=d;
end

endmodule


module pipereg_w26(
	clk,
	resetn,
	d,
	squashn,
	en,
	q
);

//parameter WIDTH=32;
//`define WIDTH 32

input clk;
input resetn;
input en;
input squashn;
input [25:0] d;
output [25:0] q;
reg [25:0] q;

always @(posedge clk)   //synchronous reset
begin
  if (resetn==0 || squashn==0)
	q<=0;
  else if (en==1)
	q<=d;
end

endmodule


module pipereg_w6(
	clk,
	resetn,
	d,
	squashn,
	en,
	q
);

//parameter WIDTH=32;
//`define WIDTH 32

input clk;
input resetn;
input en;
input squashn;
input [31:0] d;
output [31:0] q;
reg [31:0] q;

always @(posedge clk)   //synchronous reset
begin
  if (resetn==0 || squashn==0)
	q<=0;
  else if (en==1)
  begin
	q[5:0]<=d;
	q[31:6] <= 0;
	
	end
end

endmodule


module pipereg_w5(
	clk,
	resetn,
	d,
	squashn,
	en,
	q
);

//parameter WIDTH=32;
//`define WIDTH 32

input clk;
input resetn;
input en;
input squashn;
input [31:0] d;
output [31:0] q;
reg [31:0] q;

always @(posedge clk)   //synchronous reset
begin
  if (resetn==0 || squashn==0)
	q<=0;
  else if (en==1)
   begin
	q[4:0]<=d;
	q[31:5] <= 0;
	
	end
end

endmodule

module pipereg_w1(
	clk,
	resetn,
	d,
	squashn,
	en,
	q
);

//parameter WIDTH=32;
//`define WIDTH 32

input clk;
input resetn;
input en;
input squashn;
input  d;
output  q;
reg q;

always @(posedge clk)   //synchronous reset
begin
  if (resetn==0 || squashn==0)
	q<=0;
  else if (en==1)
	q<=d;
end

endmodule

/****************************************************************************
		  Generic Pipelined Register 2 -OLD: If not enabled, queues squash

		  - This piperegister stalls the reset signal as well
*/
/*
module pipereg_full(d,clk,resetn,squashn,en,q);
//parameter WIDTH=32;

input clk;
input resetn;
input en;
input squashn;
input [31:0] d;
output [31:0] q;
reg [31:0] q;
reg squash_save;

  always @(posedge clk)   //synchronous reset
  begin
	if (resetn==0 || (squashn==0 && en==1) || (squash_save&en))
	  q<=0;
	else if (en==1)
	  q<=d;
  end

  always @(posedge clk)
  begin
	if (resetn==1 && squashn==0 && en==0)
	  squash_save<=1;
	else
	  squash_save<=0;
  end
endmodule
*/
/****************************************************************************/

/****************************************************************************
		  One cycle Stall circuit
****************************************************************************/
module onecyclestall(
	request,
	clk,
	resetn,
	stalled
);

input request;
input clk;
input resetn;
output stalled;

  reg T,Tnext;

  // State machine for Stalling 1 cycle
  always@(request or T)
  begin
	case(T) 
	  1'b0: Tnext=request;
	  1'b1: Tnext=0;
	endcase 
  end       
  always@(posedge clk)
	if (~resetn)
	  T<=0; 
	else    
	  T<=Tnext;
  assign stalled=(request&~T);
endmodule

/****************************************************************************

		  Multi cycle Stall circuit - with wait signal

		  - One FF plus one 2:1 mux to stall 1st cycle on request, then wait
		  - this makes wait don't care for the first cycle
****************************************************************************/
/*
module multicyclestall(request, devwait,clk,resetn,stalled);
input request;
input devwait;
input clk;
input resetn;
output stalled;

  reg T;

  always@(posedge clk)
	if (~resetn)
	  T<=0;
	else
	  T<=stalled;

  assign stalled=(T) ? devwait : request;
endmodule
*/
/****************************************************************************
		  One cycle - Pipeline delay register
****************************************************************************/
/*
module pipedelayreg(d,en,clk,resetn,squashn,dst,stalled,q);
//`define WIDTH 32
//parameter WIDTH=32;
input [31:0] d;
input [4:0] dst;
input en;
input clk;
input resetn;
input squashn;
output stalled;
output [31:0] q;

  reg [31:0] q;
  reg T,Tnext;

  // State machine for Stalling 1 cycle
  always@(en or T or dst)
  begin
	case(T) 
	  0: Tnext=en&(|dst);
	  1: Tnext=0;
	endcase 
  end       
  always@(posedge clk)
	if (~resetn)
	  T<=0; 
	else    
	  T<=Tnext;

  always @(posedge clk)   //synchronous reset
  begin
	if (resetn==0 || squashn==0)
	  q<=0;
	else if (en==1)
	  q<=d;
  end

  assign stalled=(en&~T&(|dst));
endmodule
*/

/****************************************************************************
		  Fake Delay
****************************************************************************/
module fakedelay(clk,d,q);
//`define WIDTH 32
//parameter WIDTH=32;
input [31:0] d;
input clk;
output [31:0] q;

wire unused;
assign unused = clk;
assign q=d;

endmodule

/****************************************************************************
		  Zeroer
****************************************************************************/
module zeroer(d,en,q);
//parameter WIDTH=32;
//`define WIDTH 32

input en;
input [4:0] d;
output [31:0] q;
assign q[4:0]= (en) ? d : 0;
assign q [31:05] = 0;

endmodule

/****************************************************************************
		  NOP - used to hack position of multiplexors
****************************************************************************/
module nop(d,q);
//parameter WIDTH=32;
//`define WIDTH 32

input [31:0] d;
output [31:0] q;

  assign q=d;

endmodule

/****************************************************************************

		  Const
****************************************************************************/

/****************************************************************************
		  Branch detector
****************************************************************************/
/*
module branch_detector(opcode, func, is_branch);


input [5:0] opcode;
input [5:0] func;
output is_branch;

wire is_special;

assign is_special=!(|opcode);
assign is_branch=((!(|opcode[5:3])) && !is_special) || 
				  ((is_special)&&(func[5:3]==3'b001));

endmodule
*/
//`define WIDTH 32

module branchresolve ( 
	rt, 
	rs,
	en, 
	eqz,
	gez,
	gtz,
	lez,
	ltz,
	ne, 
	eq
);

//parameter WIDTH=32;

input en;
input [31:0] rs;
input [31:0] rt;
output eq;
output ne;
output ltz;
output lez;
output gtz;
output gez;
output eqz;

assign eq=(en)&(rs==rt);
assign ne=(en)&~eq;
assign eqz=(en)&~(|rs);
assign ltz=(en)&rs[31];
assign lez=(en)&rs[31] | eqz;
assign gtz=(en)&(~rs[31]) & ~eqz;
assign gez=(en)&(~rs[31]);

endmodule


