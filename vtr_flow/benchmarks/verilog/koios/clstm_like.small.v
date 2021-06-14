//////////////////////////////////////////////////////////////////////////////
// Author: Andrew Boutros
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//An accelerator overlay for LSTMs based on the C-LSTM paper:
//S. Wang et al., “C-LSTM: Enabling Efficient LSTM Using Structured Compression Techniques on FPGAs,” in International Symposium on Field-Programmable Gate Arrays (FPGA), 2018.
//Some properties of the design are:
//1. 18-bit fixed point is used. 
//2. FFT based circulant convolution 
//3. On-chip weight storage (compressed weights). 
//4. Double buffering of intermediate results between stages. 
//5. Element-wise addition, multiplication and activation blocks. 
///////////////////////////////////////////////////////////////////////////////

module C_LSTM_datapath (
	input clk,
	input reset,
	input [17:0] i_X_data_0,
	input [17:0] i_X_data_1,
	input [17:0] i_X_data_2,
	input [17:0] i_X_data_3,
	input [17:0] i_X_data_4,
	input [17:0] i_X_data_5,
	input [17:0] i_X_data_6,
	input [17:0] i_X_data_7,
	input [17:0] i_X_data_8,
	input [17:0] i_X_data_9,
	input [17:0] i_X_data_10,
	input [17:0] i_X_data_11,
	input [17:0] i_X_data_12,
	input [17:0] i_X_data_13,
	input [17:0] i_X_data_14,
	input [17:0] i_X_data_15,
	input i_ready,
	input i_valid,
	input start_compute,
	output o_valid,
	output [17:0] o_Yt_0_0,
	output [17:0] o_Yt_0_1,
	output [17:0] o_Yt_0_2,
	output [17:0] o_Yt_0_3,
	output [17:0] o_Yt_0_4,
	output [17:0] o_Yt_0_5,
	output [17:0] o_Yt_0_6,
	output [17:0] o_Yt_0_7,
	output [17:0] o_Yt_0_8,
	output [17:0] o_Yt_0_9,
	output [17:0] o_Yt_0_10,
	output [17:0] o_Yt_0_11,
	output [17:0] o_Yt_0_12,
	output [17:0] o_Yt_0_13,
	output [17:0] o_Yt_0_14,
	output [17:0] o_Yt_0_15,
	output o_ready
);

// Enable whenever reciever is ready 
wire enable;
assign enable = i_ready;

// Input registers 
wire [17:0] i_data_0;
wire [17:0] i_data_1;
wire [17:0] i_data_2;
wire [17:0] i_data_3;
wire [17:0] i_data_4;
wire [17:0] i_data_5;
wire [17:0] i_data_6;
wire [17:0] i_data_7;
wire [17:0] i_data_8;
wire [17:0] i_data_9;
wire [17:0] i_data_10;
wire [17:0] i_data_11;
wire [17:0] i_data_12;
wire [17:0] i_data_13;
wire [17:0] i_data_14;
wire [17:0] i_data_15;
wire [17:0] i_data_hold_0;
wire [17:0] i_data_hold_1;
wire [17:0] i_data_hold_2;
wire [17:0] i_data_hold_3;
wire [17:0] i_data_hold_4;
wire [17:0] i_data_hold_5;
wire [17:0] i_data_hold_6;
wire [17:0] i_data_hold_7;
wire [17:0] i_data_hold_8;
wire [17:0] i_data_hold_9;
wire [17:0] i_data_hold_10;
wire [17:0] i_data_hold_11;
wire [17:0] i_data_hold_12;
wire [17:0] i_data_hold_13;
wire [17:0] i_data_hold_14;
wire [17:0] i_data_hold_15;

// Output registers 
reg reg_o_valid;
reg [17:0] reg_o_data_0_0;
reg [17:0] reg_o_data_0_1;
reg [17:0] reg_o_data_0_2;
reg [17:0] reg_o_data_0_3;
reg [17:0] reg_o_data_0_4;
reg [17:0] reg_o_data_0_5;
reg [17:0] reg_o_data_0_6;
reg [17:0] reg_o_data_0_7;
reg [17:0] reg_o_data_0_8;
reg [17:0] reg_o_data_0_9;
reg [17:0] reg_o_data_0_10;
reg [17:0] reg_o_data_0_11;
reg [17:0] reg_o_data_0_12;
reg [17:0] reg_o_data_0_13;
reg [17:0] reg_o_data_0_14;
reg [17:0] reg_o_data_0_15;

// Inter connectionswire i_valid_hold;
wire stage1_valid, stage1_ready;
wire out_X_Y_buffer_valid;

// Stage 1 connections and weight buffers
// Input gate parameters
wire [17:0] Wixr_real_0_0;
wire [17:0] Wixr_imag_0_0;
wire [17:0] Wixr_real_0_1;
wire [17:0] Wixr_imag_0_1;
wire [17:0] Wixr_real_0_2;
wire [17:0] Wixr_imag_0_2;
wire [17:0] Wixr_real_0_3;
wire [17:0] Wixr_imag_0_3;
wire [17:0] Wixr_real_0_4;
wire [17:0] Wixr_imag_0_4;
wire [17:0] Wixr_real_0_5;
wire [17:0] Wixr_imag_0_5;
wire [17:0] Wixr_real_0_6;
wire [17:0] Wixr_imag_0_6;
wire [17:0] Wixr_real_0_7;
wire [17:0] Wixr_imag_0_7;
wire [17:0] Wixr_real_0_8;
wire [17:0] Wixr_imag_0_8;

// Forget gate parameters
wire [17:0] Wfxr_real_0_0;
wire [17:0] Wfxr_imag_0_0;
wire [17:0] Wfxr_real_0_1;
wire [17:0] Wfxr_imag_0_1;
wire [17:0] Wfxr_real_0_2;
wire [17:0] Wfxr_imag_0_2;
wire [17:0] Wfxr_real_0_3;
wire [17:0] Wfxr_imag_0_3;
wire [17:0] Wfxr_real_0_4;
wire [17:0] Wfxr_imag_0_4;
wire [17:0] Wfxr_real_0_5;
wire [17:0] Wfxr_imag_0_5;
wire [17:0] Wfxr_real_0_6;
wire [17:0] Wfxr_imag_0_6;
wire [17:0] Wfxr_real_0_7;
wire [17:0] Wfxr_imag_0_7;
wire [17:0] Wfxr_real_0_8;
wire [17:0] Wfxr_imag_0_8;

// Output gate parameters
wire [17:0] Woxr_real_0_0;
wire [17:0] Woxr_imag_0_0;
wire [17:0] Woxr_real_0_1;
wire [17:0] Woxr_imag_0_1;
wire [17:0] Woxr_real_0_2;
wire [17:0] Woxr_imag_0_2;
wire [17:0] Woxr_real_0_3;
wire [17:0] Woxr_imag_0_3;
wire [17:0] Woxr_real_0_4;
wire [17:0] Woxr_imag_0_4;
wire [17:0] Woxr_real_0_5;
wire [17:0] Woxr_imag_0_5;
wire [17:0] Woxr_real_0_6;
wire [17:0] Woxr_imag_0_6;
wire [17:0] Woxr_real_0_7;
wire [17:0] Woxr_imag_0_7;
wire [17:0] Woxr_real_0_8;
wire [17:0] Woxr_imag_0_8;

// Activation gate parameters
wire [17:0] Wcxr_real_0_0;
wire [17:0] Wcxr_imag_0_0;
wire [17:0] Wcxr_real_0_1;
wire [17:0] Wcxr_imag_0_1;
wire [17:0] Wcxr_real_0_2;
wire [17:0] Wcxr_imag_0_2;
wire [17:0] Wcxr_real_0_3;
wire [17:0] Wcxr_imag_0_3;
wire [17:0] Wcxr_real_0_4;
wire [17:0] Wcxr_imag_0_4;
wire [17:0] Wcxr_real_0_5;
wire [17:0] Wcxr_imag_0_5;
wire [17:0] Wcxr_real_0_6;
wire [17:0] Wcxr_imag_0_6;
wire [17:0] Wcxr_real_0_7;
wire [17:0] Wcxr_imag_0_7;
wire [17:0] Wcxr_real_0_8;
wire [17:0] Wcxr_imag_0_8;

wire [17:0] WixrXtYt_1_0_0;
wire [17:0] WfxrXtYt_1_0_0;
wire [17:0] WoxrXtYt_1_0_0;
wire [17:0] WcxrXtYt_1_0_0;
wire [17:0] WixrXtYt_1_0_1;
wire [17:0] WfxrXtYt_1_0_1;
wire [17:0] WoxrXtYt_1_0_1;
wire [17:0] WcxrXtYt_1_0_1;
wire [17:0] WixrXtYt_1_0_2;
wire [17:0] WfxrXtYt_1_0_2;
wire [17:0] WoxrXtYt_1_0_2;
wire [17:0] WcxrXtYt_1_0_2;
wire [17:0] WixrXtYt_1_0_3;
wire [17:0] WfxrXtYt_1_0_3;
wire [17:0] WoxrXtYt_1_0_3;
wire [17:0] WcxrXtYt_1_0_3;
wire [17:0] WixrXtYt_1_0_4;
wire [17:0] WfxrXtYt_1_0_4;
wire [17:0] WoxrXtYt_1_0_4;
wire [17:0] WcxrXtYt_1_0_4;
wire [17:0] WixrXtYt_1_0_5;
wire [17:0] WfxrXtYt_1_0_5;
wire [17:0] WoxrXtYt_1_0_5;
wire [17:0] WcxrXtYt_1_0_5;
wire [17:0] WixrXtYt_1_0_6;
wire [17:0] WfxrXtYt_1_0_6;
wire [17:0] WoxrXtYt_1_0_6;
wire [17:0] WcxrXtYt_1_0_6;
wire [17:0] WixrXtYt_1_0_7;
wire [17:0] WfxrXtYt_1_0_7;
wire [17:0] WoxrXtYt_1_0_7;
wire [17:0] WcxrXtYt_1_0_7;
wire [17:0] WixrXtYt_1_0_8;
wire [17:0] WfxrXtYt_1_0_8;
wire [17:0] WoxrXtYt_1_0_8;
wire [17:0] WcxrXtYt_1_0_8;
wire [17:0] WixrXtYt_1_0_9;
wire [17:0] WfxrXtYt_1_0_9;
wire [17:0] WoxrXtYt_1_0_9;
wire [17:0] WcxrXtYt_1_0_9;
wire [17:0] WixrXtYt_1_0_10;
wire [17:0] WfxrXtYt_1_0_10;
wire [17:0] WoxrXtYt_1_0_10;
wire [17:0] WcxrXtYt_1_0_10;
wire [17:0] WixrXtYt_1_0_11;
wire [17:0] WfxrXtYt_1_0_11;
wire [17:0] WoxrXtYt_1_0_11;
wire [17:0] WcxrXtYt_1_0_11;
wire [17:0] WixrXtYt_1_0_12;
wire [17:0] WfxrXtYt_1_0_12;
wire [17:0] WoxrXtYt_1_0_12;
wire [17:0] WcxrXtYt_1_0_12;
wire [17:0] WixrXtYt_1_0_13;
wire [17:0] WfxrXtYt_1_0_13;
wire [17:0] WoxrXtYt_1_0_13;
wire [17:0] WcxrXtYt_1_0_13;
wire [17:0] WixrXtYt_1_0_14;
wire [17:0] WfxrXtYt_1_0_14;
wire [17:0] WoxrXtYt_1_0_14;
wire [17:0] WcxrXtYt_1_0_14;
wire [17:0] WixrXtYt_1_0_15;
wire [17:0] WfxrXtYt_1_0_15;
wire [17:0] WoxrXtYt_1_0_15;
wire [17:0] WcxrXtYt_1_0_15;

wire stage_buffer_incr_index;
assign stage_buffer_incr_index = out_X_Y_buffer_valid & enable;
stage1_parameter_buffer_18_1_16_42_2688 stage1_parameter_buffer_18_1_16_42_2688_inst (
	.clk(clk),
	.reset(reset),
	.Wixr_real_0_0(Wixr_real_0_0),
	.Wixr_imag_0_0(Wixr_imag_0_0),
	.Wfxr_real_0_0(Wfxr_real_0_0),
	.Wfxr_imag_0_0(Wfxr_imag_0_0),
	.Woxr_real_0_0(Woxr_real_0_0),
	.Woxr_imag_0_0(Woxr_imag_0_0),
	.Wcxr_real_0_0(Wcxr_real_0_0),
	.Wcxr_imag_0_0(Wcxr_imag_0_0),
	.Wixr_real_0_1(Wixr_real_0_1),
	.Wixr_imag_0_1(Wixr_imag_0_1),
	.Wfxr_real_0_1(Wfxr_real_0_1),
	.Wfxr_imag_0_1(Wfxr_imag_0_1),
	.Woxr_real_0_1(Woxr_real_0_1),
	.Woxr_imag_0_1(Woxr_imag_0_1),
	.Wcxr_real_0_1(Wcxr_real_0_1),
	.Wcxr_imag_0_1(Wcxr_imag_0_1),
	.Wixr_real_0_2(Wixr_real_0_2),
	.Wixr_imag_0_2(Wixr_imag_0_2),
	.Wfxr_real_0_2(Wfxr_real_0_2),
	.Wfxr_imag_0_2(Wfxr_imag_0_2),
	.Woxr_real_0_2(Woxr_real_0_2),
	.Woxr_imag_0_2(Woxr_imag_0_2),
	.Wcxr_real_0_2(Wcxr_real_0_2),
	.Wcxr_imag_0_2(Wcxr_imag_0_2),
	.Wixr_real_0_3(Wixr_real_0_3),
	.Wixr_imag_0_3(Wixr_imag_0_3),
	.Wfxr_real_0_3(Wfxr_real_0_3),
	.Wfxr_imag_0_3(Wfxr_imag_0_3),
	.Woxr_real_0_3(Woxr_real_0_3),
	.Woxr_imag_0_3(Woxr_imag_0_3),
	.Wcxr_real_0_3(Wcxr_real_0_3),
	.Wcxr_imag_0_3(Wcxr_imag_0_3),
	.Wixr_real_0_4(Wixr_real_0_4),
	.Wixr_imag_0_4(Wixr_imag_0_4),
	.Wfxr_real_0_4(Wfxr_real_0_4),
	.Wfxr_imag_0_4(Wfxr_imag_0_4),
	.Woxr_real_0_4(Woxr_real_0_4),
	.Woxr_imag_0_4(Woxr_imag_0_4),
	.Wcxr_real_0_4(Wcxr_real_0_4),
	.Wcxr_imag_0_4(Wcxr_imag_0_4),
	.Wixr_real_0_5(Wixr_real_0_5),
	.Wixr_imag_0_5(Wixr_imag_0_5),
	.Wfxr_real_0_5(Wfxr_real_0_5),
	.Wfxr_imag_0_5(Wfxr_imag_0_5),
	.Woxr_real_0_5(Woxr_real_0_5),
	.Woxr_imag_0_5(Woxr_imag_0_5),
	.Wcxr_real_0_5(Wcxr_real_0_5),
	.Wcxr_imag_0_5(Wcxr_imag_0_5),
	.Wixr_real_0_6(Wixr_real_0_6),
	.Wixr_imag_0_6(Wixr_imag_0_6),
	.Wfxr_real_0_6(Wfxr_real_0_6),
	.Wfxr_imag_0_6(Wfxr_imag_0_6),
	.Woxr_real_0_6(Woxr_real_0_6),
	.Woxr_imag_0_6(Woxr_imag_0_6),
	.Wcxr_real_0_6(Wcxr_real_0_6),
	.Wcxr_imag_0_6(Wcxr_imag_0_6),
	.Wixr_real_0_7(Wixr_real_0_7),
	.Wixr_imag_0_7(Wixr_imag_0_7),
	.Wfxr_real_0_7(Wfxr_real_0_7),
	.Wfxr_imag_0_7(Wfxr_imag_0_7),
	.Woxr_real_0_7(Woxr_real_0_7),
	.Woxr_imag_0_7(Woxr_imag_0_7),
	.Wcxr_real_0_7(Wcxr_real_0_7),
	.Wcxr_imag_0_7(Wcxr_imag_0_7),
	.Wixr_real_0_8(Wixr_real_0_8),
	.Wixr_imag_0_8(Wixr_imag_0_8),
	.Wfxr_real_0_8(Wfxr_real_0_8),
	.Wfxr_imag_0_8(Wfxr_imag_0_8),
	.Woxr_real_0_8(Woxr_real_0_8),
	.Woxr_imag_0_8(Woxr_imag_0_8),
	.Wcxr_real_0_8(Wcxr_real_0_8),
	.Wcxr_imag_0_8(Wcxr_imag_0_8),
	.incr_index(stage_buffer_incr_index)
);

// Pipeline the input data for one more cycle to match the parameter rom 
shift_register_group_18_16_3 shift_register_group_18_16_3_inst (
	.clk(clk),
	.enable(1'b1),
	.in_0(i_data_0),
	.out_0(i_data_hold_0),
	.in_1(i_data_1),
	.out_1(i_data_hold_1),
	.in_2(i_data_2),
	.out_2(i_data_hold_2),
	.in_3(i_data_3),
	.out_3(i_data_hold_3),
	.in_4(i_data_4),
	.out_4(i_data_hold_4),
	.in_5(i_data_5),
	.out_5(i_data_hold_5),
	.in_6(i_data_6),
	.out_6(i_data_hold_6),
	.in_7(i_data_7),
	.out_7(i_data_hold_7),
	.in_8(i_data_8),
	.out_8(i_data_hold_8),
	.in_9(i_data_9),
	.out_9(i_data_hold_9),
	.in_10(i_data_10),
	.out_10(i_data_hold_10),
	.in_11(i_data_11),
	.out_11(i_data_hold_11),
	.in_12(i_data_12),
	.out_12(i_data_hold_12),
	.in_13(i_data_13),
	.out_13(i_data_hold_13),
	.in_14(i_data_14),
	.out_14(i_data_hold_14),
	.in_15(i_data_15),
	.out_15(i_data_hold_15),
	.reset(reset)
);

shift_register_unit_1_3 shift_register_unit_1_3_inst (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(out_X_Y_buffer_valid),
	.out(i_valid_hold)
);

C_LSTM_stage_1_18_10_160_512_1_16_1 C_LSTM_stage_1_18_10_160_512_1_16_1_inst (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_ready(enable),
	.i_Xt_Yt_1_0(i_data_hold_0),
	.i_Xt_Yt_1_1(i_data_hold_1),
	.i_Xt_Yt_1_2(i_data_hold_2),
	.i_Xt_Yt_1_3(i_data_hold_3),
	.i_Xt_Yt_1_4(i_data_hold_4),
	.i_Xt_Yt_1_5(i_data_hold_5),
	.i_Xt_Yt_1_6(i_data_hold_6),
	.i_Xt_Yt_1_7(i_data_hold_7),
	.i_Xt_Yt_1_8(i_data_hold_8),
	.i_Xt_Yt_1_9(i_data_hold_9),
	.i_Xt_Yt_1_10(i_data_hold_10),
	.i_Xt_Yt_1_11(i_data_hold_11),
	.i_Xt_Yt_1_12(i_data_hold_12),
	.i_Xt_Yt_1_13(i_data_hold_13),
	.i_Xt_Yt_1_14(i_data_hold_14),
	.i_Xt_Yt_1_15(i_data_hold_15),
	.i_Wixr_real_0_0(Wixr_real_0_0),
	.i_Wixr_imag_0_0(Wixr_imag_0_0),
	.i_Wfxr_real_0_0(Wfxr_real_0_0),
	.i_Wfxr_imag_0_0(Wfxr_imag_0_0),
	.i_Woxr_real_0_0(Woxr_real_0_0),
	.i_Woxr_imag_0_0(Woxr_imag_0_0),
	.i_Wcxr_real_0_0(Wcxr_real_0_0),
	.i_Wcxr_imag_0_0(Wcxr_imag_0_0),
	.i_Wixr_real_0_1(Wixr_real_0_1),
	.i_Wixr_imag_0_1(Wixr_imag_0_1),
	.i_Wfxr_real_0_1(Wfxr_real_0_1),
	.i_Wfxr_imag_0_1(Wfxr_imag_0_1),
	.i_Woxr_real_0_1(Woxr_real_0_1),
	.i_Woxr_imag_0_1(Woxr_imag_0_1),
	.i_Wcxr_real_0_1(Wcxr_real_0_1),
	.i_Wcxr_imag_0_1(Wcxr_imag_0_1),
	.i_Wixr_real_0_2(Wixr_real_0_2),
	.i_Wixr_imag_0_2(Wixr_imag_0_2),
	.i_Wfxr_real_0_2(Wfxr_real_0_2),
	.i_Wfxr_imag_0_2(Wfxr_imag_0_2),
	.i_Woxr_real_0_2(Woxr_real_0_2),
	.i_Woxr_imag_0_2(Woxr_imag_0_2),
	.i_Wcxr_real_0_2(Wcxr_real_0_2),
	.i_Wcxr_imag_0_2(Wcxr_imag_0_2),
	.i_Wixr_real_0_3(Wixr_real_0_3),
	.i_Wixr_imag_0_3(Wixr_imag_0_3),
	.i_Wfxr_real_0_3(Wfxr_real_0_3),
	.i_Wfxr_imag_0_3(Wfxr_imag_0_3),
	.i_Woxr_real_0_3(Woxr_real_0_3),
	.i_Woxr_imag_0_3(Woxr_imag_0_3),
	.i_Wcxr_real_0_3(Wcxr_real_0_3),
	.i_Wcxr_imag_0_3(Wcxr_imag_0_3),
	.i_Wixr_real_0_4(Wixr_real_0_4),
	.i_Wixr_imag_0_4(Wixr_imag_0_4),
	.i_Wfxr_real_0_4(Wfxr_real_0_4),
	.i_Wfxr_imag_0_4(Wfxr_imag_0_4),
	.i_Woxr_real_0_4(Woxr_real_0_4),
	.i_Woxr_imag_0_4(Woxr_imag_0_4),
	.i_Wcxr_real_0_4(Wcxr_real_0_4),
	.i_Wcxr_imag_0_4(Wcxr_imag_0_4),
	.i_Wixr_real_0_5(Wixr_real_0_5),
	.i_Wixr_imag_0_5(Wixr_imag_0_5),
	.i_Wfxr_real_0_5(Wfxr_real_0_5),
	.i_Wfxr_imag_0_5(Wfxr_imag_0_5),
	.i_Woxr_real_0_5(Woxr_real_0_5),
	.i_Woxr_imag_0_5(Woxr_imag_0_5),
	.i_Wcxr_real_0_5(Wcxr_real_0_5),
	.i_Wcxr_imag_0_5(Wcxr_imag_0_5),
	.i_Wixr_real_0_6(Wixr_real_0_6),
	.i_Wixr_imag_0_6(Wixr_imag_0_6),
	.i_Wfxr_real_0_6(Wfxr_real_0_6),
	.i_Wfxr_imag_0_6(Wfxr_imag_0_6),
	.i_Woxr_real_0_6(Woxr_real_0_6),
	.i_Woxr_imag_0_6(Woxr_imag_0_6),
	.i_Wcxr_real_0_6(Wcxr_real_0_6),
	.i_Wcxr_imag_0_6(Wcxr_imag_0_6),
	.i_Wixr_real_0_7(Wixr_real_0_7),
	.i_Wixr_imag_0_7(Wixr_imag_0_7),
	.i_Wfxr_real_0_7(Wfxr_real_0_7),
	.i_Wfxr_imag_0_7(Wfxr_imag_0_7),
	.i_Woxr_real_0_7(Woxr_real_0_7),
	.i_Woxr_imag_0_7(Woxr_imag_0_7),
	.i_Wcxr_real_0_7(Wcxr_real_0_7),
	.i_Wcxr_imag_0_7(Wcxr_imag_0_7),
	.i_Wixr_real_0_8(Wixr_real_0_8),
	.i_Wixr_imag_0_8(Wixr_imag_0_8),
	.i_Wfxr_real_0_8(Wfxr_real_0_8),
	.i_Wfxr_imag_0_8(Wfxr_imag_0_8),
	.i_Woxr_real_0_8(Woxr_real_0_8),
	.i_Woxr_imag_0_8(Woxr_imag_0_8),
	.i_Wcxr_real_0_8(Wcxr_real_0_8),
	.i_Wcxr_imag_0_8(Wcxr_imag_0_8),
	.o_valid(stage1_valid),
	.o_ready(stage1_ready),
	.o_WixrXtYt_1_0_0(WixrXtYt_1_0_0),
	.o_WfxrXtYt_1_0_0(WfxrXtYt_1_0_0),
	.o_WoxrXtYt_1_0_0(WoxrXtYt_1_0_0),
	.o_WcxrXtYt_1_0_0(WcxrXtYt_1_0_0),
	.o_WixrXtYt_1_0_1(WixrXtYt_1_0_1),
	.o_WfxrXtYt_1_0_1(WfxrXtYt_1_0_1),
	.o_WoxrXtYt_1_0_1(WoxrXtYt_1_0_1),
	.o_WcxrXtYt_1_0_1(WcxrXtYt_1_0_1),
	.o_WixrXtYt_1_0_2(WixrXtYt_1_0_2),
	.o_WfxrXtYt_1_0_2(WfxrXtYt_1_0_2),
	.o_WoxrXtYt_1_0_2(WoxrXtYt_1_0_2),
	.o_WcxrXtYt_1_0_2(WcxrXtYt_1_0_2),
	.o_WixrXtYt_1_0_3(WixrXtYt_1_0_3),
	.o_WfxrXtYt_1_0_3(WfxrXtYt_1_0_3),
	.o_WoxrXtYt_1_0_3(WoxrXtYt_1_0_3),
	.o_WcxrXtYt_1_0_3(WcxrXtYt_1_0_3),
	.o_WixrXtYt_1_0_4(WixrXtYt_1_0_4),
	.o_WfxrXtYt_1_0_4(WfxrXtYt_1_0_4),
	.o_WoxrXtYt_1_0_4(WoxrXtYt_1_0_4),
	.o_WcxrXtYt_1_0_4(WcxrXtYt_1_0_4),
	.o_WixrXtYt_1_0_5(WixrXtYt_1_0_5),
	.o_WfxrXtYt_1_0_5(WfxrXtYt_1_0_5),
	.o_WoxrXtYt_1_0_5(WoxrXtYt_1_0_5),
	.o_WcxrXtYt_1_0_5(WcxrXtYt_1_0_5),
	.o_WixrXtYt_1_0_6(WixrXtYt_1_0_6),
	.o_WfxrXtYt_1_0_6(WfxrXtYt_1_0_6),
	.o_WoxrXtYt_1_0_6(WoxrXtYt_1_0_6),
	.o_WcxrXtYt_1_0_6(WcxrXtYt_1_0_6),
	.o_WixrXtYt_1_0_7(WixrXtYt_1_0_7),
	.o_WfxrXtYt_1_0_7(WfxrXtYt_1_0_7),
	.o_WoxrXtYt_1_0_7(WoxrXtYt_1_0_7),
	.o_WcxrXtYt_1_0_7(WcxrXtYt_1_0_7),
	.o_WixrXtYt_1_0_8(WixrXtYt_1_0_8),
	.o_WfxrXtYt_1_0_8(WfxrXtYt_1_0_8),
	.o_WoxrXtYt_1_0_8(WoxrXtYt_1_0_8),
	.o_WcxrXtYt_1_0_8(WcxrXtYt_1_0_8),
	.o_WixrXtYt_1_0_9(WixrXtYt_1_0_9),
	.o_WfxrXtYt_1_0_9(WfxrXtYt_1_0_9),
	.o_WoxrXtYt_1_0_9(WoxrXtYt_1_0_9),
	.o_WcxrXtYt_1_0_9(WcxrXtYt_1_0_9),
	.o_WixrXtYt_1_0_10(WixrXtYt_1_0_10),
	.o_WfxrXtYt_1_0_10(WfxrXtYt_1_0_10),
	.o_WoxrXtYt_1_0_10(WoxrXtYt_1_0_10),
	.o_WcxrXtYt_1_0_10(WcxrXtYt_1_0_10),
	.o_WixrXtYt_1_0_11(WixrXtYt_1_0_11),
	.o_WfxrXtYt_1_0_11(WfxrXtYt_1_0_11),
	.o_WoxrXtYt_1_0_11(WoxrXtYt_1_0_11),
	.o_WcxrXtYt_1_0_11(WcxrXtYt_1_0_11),
	.o_WixrXtYt_1_0_12(WixrXtYt_1_0_12),
	.o_WfxrXtYt_1_0_12(WfxrXtYt_1_0_12),
	.o_WoxrXtYt_1_0_12(WoxrXtYt_1_0_12),
	.o_WcxrXtYt_1_0_12(WcxrXtYt_1_0_12),
	.o_WixrXtYt_1_0_13(WixrXtYt_1_0_13),
	.o_WfxrXtYt_1_0_13(WfxrXtYt_1_0_13),
	.o_WoxrXtYt_1_0_13(WoxrXtYt_1_0_13),
	.o_WcxrXtYt_1_0_13(WcxrXtYt_1_0_13),
	.o_WixrXtYt_1_0_14(WixrXtYt_1_0_14),
	.o_WfxrXtYt_1_0_14(WfxrXtYt_1_0_14),
	.o_WoxrXtYt_1_0_14(WoxrXtYt_1_0_14),
	.o_WcxrXtYt_1_0_14(WcxrXtYt_1_0_14),
	.o_WixrXtYt_1_0_15(WixrXtYt_1_0_15),
	.o_WfxrXtYt_1_0_15(WfxrXtYt_1_0_15),
	.o_WoxrXtYt_1_0_15(WoxrXtYt_1_0_15),
	.o_WcxrXtYt_1_0_15(WcxrXtYt_1_0_15),
	.i_valid(i_valid_hold)
);

// Stage 2 connections and parameter buffer 
wire stage2_valid, stage2_ready, stage1_valid_hold;
wire [17:0] Ct_1_0;
wire [17:0] stage2_Ct_0;
wire [17:0] stage2_mt_0;
wire [17:0] WixrXtYt_1_packed_0;
wire [17:0] WfxrXtYt_1_packed_0;
wire [17:0] WoxrXtYt_1_packed_0;
wire [17:0] WcxrXtYt_1_packed_0;
wire [17:0] WixrXtYt_1_hold_0;
wire [17:0] WfxrXtYt_1_hold_0;
wire [17:0] WoxrXtYt_1_hold_0;
wire [17:0] WcxrXtYt_1_hold_0;
wire [17:0] Wic_0;
wire [17:0] bi_0;
wire [17:0] Wfc_0;
wire [17:0] bf_0;
wire [17:0] Woc_0;
wire [17:0] bo_0;
wire [17:0] bc_0;
wire [17:0] Ct_1_1;
wire [17:0] stage2_Ct_1;
wire [17:0] stage2_mt_1;
wire [17:0] WixrXtYt_1_packed_1;
wire [17:0] WfxrXtYt_1_packed_1;
wire [17:0] WoxrXtYt_1_packed_1;
wire [17:0] WcxrXtYt_1_packed_1;
wire [17:0] WixrXtYt_1_hold_1;
wire [17:0] WfxrXtYt_1_hold_1;
wire [17:0] WoxrXtYt_1_hold_1;
wire [17:0] WcxrXtYt_1_hold_1;
wire [17:0] Wic_1;
wire [17:0] bi_1;
wire [17:0] Wfc_1;
wire [17:0] bf_1;
wire [17:0] Woc_1;
wire [17:0] bo_1;
wire [17:0] bc_1;
wire [17:0] Ct_1_2;
wire [17:0] stage2_Ct_2;
wire [17:0] stage2_mt_2;
wire [17:0] WixrXtYt_1_packed_2;
wire [17:0] WfxrXtYt_1_packed_2;
wire [17:0] WoxrXtYt_1_packed_2;
wire [17:0] WcxrXtYt_1_packed_2;
wire [17:0] WixrXtYt_1_hold_2;
wire [17:0] WfxrXtYt_1_hold_2;
wire [17:0] WoxrXtYt_1_hold_2;
wire [17:0] WcxrXtYt_1_hold_2;
wire [17:0] Wic_2;
wire [17:0] bi_2;
wire [17:0] Wfc_2;
wire [17:0] bf_2;
wire [17:0] Woc_2;
wire [17:0] bo_2;
wire [17:0] bc_2;
wire [17:0] Ct_1_3;
wire [17:0] stage2_Ct_3;
wire [17:0] stage2_mt_3;
wire [17:0] WixrXtYt_1_packed_3;
wire [17:0] WfxrXtYt_1_packed_3;
wire [17:0] WoxrXtYt_1_packed_3;
wire [17:0] WcxrXtYt_1_packed_3;
wire [17:0] WixrXtYt_1_hold_3;
wire [17:0] WfxrXtYt_1_hold_3;
wire [17:0] WoxrXtYt_1_hold_3;
wire [17:0] WcxrXtYt_1_hold_3;
wire [17:0] Wic_3;
wire [17:0] bi_3;
wire [17:0] Wfc_3;
wire [17:0] bf_3;
wire [17:0] Woc_3;
wire [17:0] bo_3;
wire [17:0] bc_3;
wire [17:0] Ct_1_4;
wire [17:0] stage2_Ct_4;
wire [17:0] stage2_mt_4;
wire [17:0] WixrXtYt_1_packed_4;
wire [17:0] WfxrXtYt_1_packed_4;
wire [17:0] WoxrXtYt_1_packed_4;
wire [17:0] WcxrXtYt_1_packed_4;
wire [17:0] WixrXtYt_1_hold_4;
wire [17:0] WfxrXtYt_1_hold_4;
wire [17:0] WoxrXtYt_1_hold_4;
wire [17:0] WcxrXtYt_1_hold_4;
wire [17:0] Wic_4;
wire [17:0] bi_4;
wire [17:0] Wfc_4;
wire [17:0] bf_4;
wire [17:0] Woc_4;
wire [17:0] bo_4;
wire [17:0] bc_4;
wire [17:0] Ct_1_5;
wire [17:0] stage2_Ct_5;
wire [17:0] stage2_mt_5;
wire [17:0] WixrXtYt_1_packed_5;
wire [17:0] WfxrXtYt_1_packed_5;
wire [17:0] WoxrXtYt_1_packed_5;
wire [17:0] WcxrXtYt_1_packed_5;
wire [17:0] WixrXtYt_1_hold_5;
wire [17:0] WfxrXtYt_1_hold_5;
wire [17:0] WoxrXtYt_1_hold_5;
wire [17:0] WcxrXtYt_1_hold_5;
wire [17:0] Wic_5;
wire [17:0] bi_5;
wire [17:0] Wfc_5;
wire [17:0] bf_5;
wire [17:0] Woc_5;
wire [17:0] bo_5;
wire [17:0] bc_5;
wire [17:0] Ct_1_6;
wire [17:0] stage2_Ct_6;
wire [17:0] stage2_mt_6;
wire [17:0] WixrXtYt_1_packed_6;
wire [17:0] WfxrXtYt_1_packed_6;
wire [17:0] WoxrXtYt_1_packed_6;
wire [17:0] WcxrXtYt_1_packed_6;
wire [17:0] WixrXtYt_1_hold_6;
wire [17:0] WfxrXtYt_1_hold_6;
wire [17:0] WoxrXtYt_1_hold_6;
wire [17:0] WcxrXtYt_1_hold_6;
wire [17:0] Wic_6;
wire [17:0] bi_6;
wire [17:0] Wfc_6;
wire [17:0] bf_6;
wire [17:0] Woc_6;
wire [17:0] bo_6;
wire [17:0] bc_6;
wire [17:0] Ct_1_7;
wire [17:0] stage2_Ct_7;
wire [17:0] stage2_mt_7;
wire [17:0] WixrXtYt_1_packed_7;
wire [17:0] WfxrXtYt_1_packed_7;
wire [17:0] WoxrXtYt_1_packed_7;
wire [17:0] WcxrXtYt_1_packed_7;
wire [17:0] WixrXtYt_1_hold_7;
wire [17:0] WfxrXtYt_1_hold_7;
wire [17:0] WoxrXtYt_1_hold_7;
wire [17:0] WcxrXtYt_1_hold_7;
wire [17:0] Wic_7;
wire [17:0] bi_7;
wire [17:0] Wfc_7;
wire [17:0] bf_7;
wire [17:0] Woc_7;
wire [17:0] bo_7;
wire [17:0] bc_7;
wire [17:0] Ct_1_8;
wire [17:0] stage2_Ct_8;
wire [17:0] stage2_mt_8;
wire [17:0] WixrXtYt_1_packed_8;
wire [17:0] WfxrXtYt_1_packed_8;
wire [17:0] WoxrXtYt_1_packed_8;
wire [17:0] WcxrXtYt_1_packed_8;
wire [17:0] WixrXtYt_1_hold_8;
wire [17:0] WfxrXtYt_1_hold_8;
wire [17:0] WoxrXtYt_1_hold_8;
wire [17:0] WcxrXtYt_1_hold_8;
wire [17:0] Wic_8;
wire [17:0] bi_8;
wire [17:0] Wfc_8;
wire [17:0] bf_8;
wire [17:0] Woc_8;
wire [17:0] bo_8;
wire [17:0] bc_8;
wire [17:0] Ct_1_9;
wire [17:0] stage2_Ct_9;
wire [17:0] stage2_mt_9;
wire [17:0] WixrXtYt_1_packed_9;
wire [17:0] WfxrXtYt_1_packed_9;
wire [17:0] WoxrXtYt_1_packed_9;
wire [17:0] WcxrXtYt_1_packed_9;
wire [17:0] WixrXtYt_1_hold_9;
wire [17:0] WfxrXtYt_1_hold_9;
wire [17:0] WoxrXtYt_1_hold_9;
wire [17:0] WcxrXtYt_1_hold_9;
wire [17:0] Wic_9;
wire [17:0] bi_9;
wire [17:0] Wfc_9;
wire [17:0] bf_9;
wire [17:0] Woc_9;
wire [17:0] bo_9;
wire [17:0] bc_9;
wire [17:0] Ct_1_10;
wire [17:0] stage2_Ct_10;
wire [17:0] stage2_mt_10;
wire [17:0] WixrXtYt_1_packed_10;
wire [17:0] WfxrXtYt_1_packed_10;
wire [17:0] WoxrXtYt_1_packed_10;
wire [17:0] WcxrXtYt_1_packed_10;
wire [17:0] WixrXtYt_1_hold_10;
wire [17:0] WfxrXtYt_1_hold_10;
wire [17:0] WoxrXtYt_1_hold_10;
wire [17:0] WcxrXtYt_1_hold_10;
wire [17:0] Wic_10;
wire [17:0] bi_10;
wire [17:0] Wfc_10;
wire [17:0] bf_10;
wire [17:0] Woc_10;
wire [17:0] bo_10;
wire [17:0] bc_10;
wire [17:0] Ct_1_11;
wire [17:0] stage2_Ct_11;
wire [17:0] stage2_mt_11;
wire [17:0] WixrXtYt_1_packed_11;
wire [17:0] WfxrXtYt_1_packed_11;
wire [17:0] WoxrXtYt_1_packed_11;
wire [17:0] WcxrXtYt_1_packed_11;
wire [17:0] WixrXtYt_1_hold_11;
wire [17:0] WfxrXtYt_1_hold_11;
wire [17:0] WoxrXtYt_1_hold_11;
wire [17:0] WcxrXtYt_1_hold_11;
wire [17:0] Wic_11;
wire [17:0] bi_11;
wire [17:0] Wfc_11;
wire [17:0] bf_11;
wire [17:0] Woc_11;
wire [17:0] bo_11;
wire [17:0] bc_11;
wire [17:0] Ct_1_12;
wire [17:0] stage2_Ct_12;
wire [17:0] stage2_mt_12;
wire [17:0] WixrXtYt_1_packed_12;
wire [17:0] WfxrXtYt_1_packed_12;
wire [17:0] WoxrXtYt_1_packed_12;
wire [17:0] WcxrXtYt_1_packed_12;
wire [17:0] WixrXtYt_1_hold_12;
wire [17:0] WfxrXtYt_1_hold_12;
wire [17:0] WoxrXtYt_1_hold_12;
wire [17:0] WcxrXtYt_1_hold_12;
wire [17:0] Wic_12;
wire [17:0] bi_12;
wire [17:0] Wfc_12;
wire [17:0] bf_12;
wire [17:0] Woc_12;
wire [17:0] bo_12;
wire [17:0] bc_12;
wire [17:0] Ct_1_13;
wire [17:0] stage2_Ct_13;
wire [17:0] stage2_mt_13;
wire [17:0] WixrXtYt_1_packed_13;
wire [17:0] WfxrXtYt_1_packed_13;
wire [17:0] WoxrXtYt_1_packed_13;
wire [17:0] WcxrXtYt_1_packed_13;
wire [17:0] WixrXtYt_1_hold_13;
wire [17:0] WfxrXtYt_1_hold_13;
wire [17:0] WoxrXtYt_1_hold_13;
wire [17:0] WcxrXtYt_1_hold_13;
wire [17:0] Wic_13;
wire [17:0] bi_13;
wire [17:0] Wfc_13;
wire [17:0] bf_13;
wire [17:0] Woc_13;
wire [17:0] bo_13;
wire [17:0] bc_13;
wire [17:0] Ct_1_14;
wire [17:0] stage2_Ct_14;
wire [17:0] stage2_mt_14;
wire [17:0] WixrXtYt_1_packed_14;
wire [17:0] WfxrXtYt_1_packed_14;
wire [17:0] WoxrXtYt_1_packed_14;
wire [17:0] WcxrXtYt_1_packed_14;
wire [17:0] WixrXtYt_1_hold_14;
wire [17:0] WfxrXtYt_1_hold_14;
wire [17:0] WoxrXtYt_1_hold_14;
wire [17:0] WcxrXtYt_1_hold_14;
wire [17:0] Wic_14;
wire [17:0] bi_14;
wire [17:0] Wfc_14;
wire [17:0] bf_14;
wire [17:0] Woc_14;
wire [17:0] bo_14;
wire [17:0] bc_14;
wire [17:0] Ct_1_15;
wire [17:0] stage2_Ct_15;
wire [17:0] stage2_mt_15;
wire [17:0] WixrXtYt_1_packed_15;
wire [17:0] WfxrXtYt_1_packed_15;
wire [17:0] WoxrXtYt_1_packed_15;
wire [17:0] WcxrXtYt_1_packed_15;
wire [17:0] WixrXtYt_1_hold_15;
wire [17:0] WfxrXtYt_1_hold_15;
wire [17:0] WoxrXtYt_1_hold_15;
wire [17:0] WcxrXtYt_1_hold_15;
wire [17:0] Wic_15;
wire [17:0] bi_15;
wire [17:0] Wfc_15;
wire [17:0] bf_15;
wire [17:0] Woc_15;
wire [17:0] bo_15;
wire [17:0] bc_15;

stage2_parameter_buffer_18_1_16_64 stage2_parameter_buffer_18_1_16_64_inst (
	.clk(clk),
	.reset(reset),
	.o_Wic_0(Wic_0),
	.o_bi_0(bi_0),
	.o_Wfc_0(Wfc_0),
	.o_bf_0(bf_0),
	.o_Woc_0(Woc_0),
	.o_bo_0(bo_0),
	.o_bc_0(bc_0),
	.o_Wic_1(Wic_1),
	.o_bi_1(bi_1),
	.o_Wfc_1(Wfc_1),
	.o_bf_1(bf_1),
	.o_Woc_1(Woc_1),
	.o_bo_1(bo_1),
	.o_bc_1(bc_1),
	.o_Wic_2(Wic_2),
	.o_bi_2(bi_2),
	.o_Wfc_2(Wfc_2),
	.o_bf_2(bf_2),
	.o_Woc_2(Woc_2),
	.o_bo_2(bo_2),
	.o_bc_2(bc_2),
	.o_Wic_3(Wic_3),
	.o_bi_3(bi_3),
	.o_Wfc_3(Wfc_3),
	.o_bf_3(bf_3),
	.o_Woc_3(Woc_3),
	.o_bo_3(bo_3),
	.o_bc_3(bc_3),
	.o_Wic_4(Wic_4),
	.o_bi_4(bi_4),
	.o_Wfc_4(Wfc_4),
	.o_bf_4(bf_4),
	.o_Woc_4(Woc_4),
	.o_bo_4(bo_4),
	.o_bc_4(bc_4),
	.o_Wic_5(Wic_5),
	.o_bi_5(bi_5),
	.o_Wfc_5(Wfc_5),
	.o_bf_5(bf_5),
	.o_Woc_5(Woc_5),
	.o_bo_5(bo_5),
	.o_bc_5(bc_5),
	.o_Wic_6(Wic_6),
	.o_bi_6(bi_6),
	.o_Wfc_6(Wfc_6),
	.o_bf_6(bf_6),
	.o_Woc_6(Woc_6),
	.o_bo_6(bo_6),
	.o_bc_6(bc_6),
	.o_Wic_7(Wic_7),
	.o_bi_7(bi_7),
	.o_Wfc_7(Wfc_7),
	.o_bf_7(bf_7),
	.o_Woc_7(Woc_7),
	.o_bo_7(bo_7),
	.o_bc_7(bc_7),
	.o_Wic_8(Wic_8),
	.o_bi_8(bi_8),
	.o_Wfc_8(Wfc_8),
	.o_bf_8(bf_8),
	.o_Woc_8(Woc_8),
	.o_bo_8(bo_8),
	.o_bc_8(bc_8),
	.o_Wic_9(Wic_9),
	.o_bi_9(bi_9),
	.o_Wfc_9(Wfc_9),
	.o_bf_9(bf_9),
	.o_Woc_9(Woc_9),
	.o_bo_9(bo_9),
	.o_bc_9(bc_9),
	.o_Wic_10(Wic_10),
	.o_bi_10(bi_10),
	.o_Wfc_10(Wfc_10),
	.o_bf_10(bf_10),
	.o_Woc_10(Woc_10),
	.o_bo_10(bo_10),
	.o_bc_10(bc_10),
	.o_Wic_11(Wic_11),
	.o_bi_11(bi_11),
	.o_Wfc_11(Wfc_11),
	.o_bf_11(bf_11),
	.o_Woc_11(Woc_11),
	.o_bo_11(bo_11),
	.o_bc_11(bc_11),
	.o_Wic_12(Wic_12),
	.o_bi_12(bi_12),
	.o_Wfc_12(Wfc_12),
	.o_bf_12(bf_12),
	.o_Woc_12(Woc_12),
	.o_bo_12(bo_12),
	.o_bc_12(bc_12),
	.o_Wic_13(Wic_13),
	.o_bi_13(bi_13),
	.o_Wfc_13(Wfc_13),
	.o_bf_13(bf_13),
	.o_Woc_13(Woc_13),
	.o_bo_13(bo_13),
	.o_bc_13(bc_13),
	.o_Wic_14(Wic_14),
	.o_bi_14(bi_14),
	.o_Wfc_14(Wfc_14),
	.o_bf_14(bf_14),
	.o_Woc_14(Woc_14),
	.o_bo_14(bo_14),
	.o_bc_14(bc_14),
	.o_Wic_15(Wic_15),
	.o_bi_15(bi_15),
	.o_Wfc_15(Wfc_15),
	.o_bf_15(bf_15),
	.o_Woc_15(Woc_15),
	.o_bo_15(bo_15),
	.o_bc_15(bc_15),
	.incr_index(stage1_valid)
);

assign WixrXtYt_1_packed_0 = WixrXtYt_1_0_0;
assign WfxrXtYt_1_packed_0 = WfxrXtYt_1_0_0;
assign WoxrXtYt_1_packed_0 = WoxrXtYt_1_0_0;
assign WcxrXtYt_1_packed_0 = WcxrXtYt_1_0_0;
assign WixrXtYt_1_packed_1 = WixrXtYt_1_0_1;
assign WfxrXtYt_1_packed_1 = WfxrXtYt_1_0_1;
assign WoxrXtYt_1_packed_1 = WoxrXtYt_1_0_1;
assign WcxrXtYt_1_packed_1 = WcxrXtYt_1_0_1;
assign WixrXtYt_1_packed_2 = WixrXtYt_1_0_2;
assign WfxrXtYt_1_packed_2 = WfxrXtYt_1_0_2;
assign WoxrXtYt_1_packed_2 = WoxrXtYt_1_0_2;
assign WcxrXtYt_1_packed_2 = WcxrXtYt_1_0_2;
assign WixrXtYt_1_packed_3 = WixrXtYt_1_0_3;
assign WfxrXtYt_1_packed_3 = WfxrXtYt_1_0_3;
assign WoxrXtYt_1_packed_3 = WoxrXtYt_1_0_3;
assign WcxrXtYt_1_packed_3 = WcxrXtYt_1_0_3;
assign WixrXtYt_1_packed_4 = WixrXtYt_1_0_4;
assign WfxrXtYt_1_packed_4 = WfxrXtYt_1_0_4;
assign WoxrXtYt_1_packed_4 = WoxrXtYt_1_0_4;
assign WcxrXtYt_1_packed_4 = WcxrXtYt_1_0_4;
assign WixrXtYt_1_packed_5 = WixrXtYt_1_0_5;
assign WfxrXtYt_1_packed_5 = WfxrXtYt_1_0_5;
assign WoxrXtYt_1_packed_5 = WoxrXtYt_1_0_5;
assign WcxrXtYt_1_packed_5 = WcxrXtYt_1_0_5;
assign WixrXtYt_1_packed_6 = WixrXtYt_1_0_6;
assign WfxrXtYt_1_packed_6 = WfxrXtYt_1_0_6;
assign WoxrXtYt_1_packed_6 = WoxrXtYt_1_0_6;
assign WcxrXtYt_1_packed_6 = WcxrXtYt_1_0_6;
assign WixrXtYt_1_packed_7 = WixrXtYt_1_0_7;
assign WfxrXtYt_1_packed_7 = WfxrXtYt_1_0_7;
assign WoxrXtYt_1_packed_7 = WoxrXtYt_1_0_7;
assign WcxrXtYt_1_packed_7 = WcxrXtYt_1_0_7;
assign WixrXtYt_1_packed_8 = WixrXtYt_1_0_8;
assign WfxrXtYt_1_packed_8 = WfxrXtYt_1_0_8;
assign WoxrXtYt_1_packed_8 = WoxrXtYt_1_0_8;
assign WcxrXtYt_1_packed_8 = WcxrXtYt_1_0_8;
assign WixrXtYt_1_packed_9 = WixrXtYt_1_0_9;
assign WfxrXtYt_1_packed_9 = WfxrXtYt_1_0_9;
assign WoxrXtYt_1_packed_9 = WoxrXtYt_1_0_9;
assign WcxrXtYt_1_packed_9 = WcxrXtYt_1_0_9;
assign WixrXtYt_1_packed_10 = WixrXtYt_1_0_10;
assign WfxrXtYt_1_packed_10 = WfxrXtYt_1_0_10;
assign WoxrXtYt_1_packed_10 = WoxrXtYt_1_0_10;
assign WcxrXtYt_1_packed_10 = WcxrXtYt_1_0_10;
assign WixrXtYt_1_packed_11 = WixrXtYt_1_0_11;
assign WfxrXtYt_1_packed_11 = WfxrXtYt_1_0_11;
assign WoxrXtYt_1_packed_11 = WoxrXtYt_1_0_11;
assign WcxrXtYt_1_packed_11 = WcxrXtYt_1_0_11;
assign WixrXtYt_1_packed_12 = WixrXtYt_1_0_12;
assign WfxrXtYt_1_packed_12 = WfxrXtYt_1_0_12;
assign WoxrXtYt_1_packed_12 = WoxrXtYt_1_0_12;
assign WcxrXtYt_1_packed_12 = WcxrXtYt_1_0_12;
assign WixrXtYt_1_packed_13 = WixrXtYt_1_0_13;
assign WfxrXtYt_1_packed_13 = WfxrXtYt_1_0_13;
assign WoxrXtYt_1_packed_13 = WoxrXtYt_1_0_13;
assign WcxrXtYt_1_packed_13 = WcxrXtYt_1_0_13;
assign WixrXtYt_1_packed_14 = WixrXtYt_1_0_14;
assign WfxrXtYt_1_packed_14 = WfxrXtYt_1_0_14;
assign WoxrXtYt_1_packed_14 = WoxrXtYt_1_0_14;
assign WcxrXtYt_1_packed_14 = WcxrXtYt_1_0_14;
assign WixrXtYt_1_packed_15 = WixrXtYt_1_0_15;
assign WfxrXtYt_1_packed_15 = WfxrXtYt_1_0_15;
assign WoxrXtYt_1_packed_15 = WoxrXtYt_1_0_15;
assign WcxrXtYt_1_packed_15 = WcxrXtYt_1_0_15;

shift_register_group_18_16_3 shift_register_group_18_16_3_inst_Wi (
	.clk(clk),
	.enable(enable),
	.in_0(WixrXtYt_1_packed_0),
	.out_0(WixrXtYt_1_hold_0),
	.in_1(WixrXtYt_1_packed_1),
	.out_1(WixrXtYt_1_hold_1),
	.in_2(WixrXtYt_1_packed_2),
	.out_2(WixrXtYt_1_hold_2),
	.in_3(WixrXtYt_1_packed_3),
	.out_3(WixrXtYt_1_hold_3),
	.in_4(WixrXtYt_1_packed_4),
	.out_4(WixrXtYt_1_hold_4),
	.in_5(WixrXtYt_1_packed_5),
	.out_5(WixrXtYt_1_hold_5),
	.in_6(WixrXtYt_1_packed_6),
	.out_6(WixrXtYt_1_hold_6),
	.in_7(WixrXtYt_1_packed_7),
	.out_7(WixrXtYt_1_hold_7),
	.in_8(WixrXtYt_1_packed_8),
	.out_8(WixrXtYt_1_hold_8),
	.in_9(WixrXtYt_1_packed_9),
	.out_9(WixrXtYt_1_hold_9),
	.in_10(WixrXtYt_1_packed_10),
	.out_10(WixrXtYt_1_hold_10),
	.in_11(WixrXtYt_1_packed_11),
	.out_11(WixrXtYt_1_hold_11),
	.in_12(WixrXtYt_1_packed_12),
	.out_12(WixrXtYt_1_hold_12),
	.in_13(WixrXtYt_1_packed_13),
	.out_13(WixrXtYt_1_hold_13),
	.in_14(WixrXtYt_1_packed_14),
	.out_14(WixrXtYt_1_hold_14),
	.in_15(WixrXtYt_1_packed_15),
	.out_15(WixrXtYt_1_hold_15),
	.reset(reset)
);

shift_register_group_18_16_3 shift_register_group_18_16_3_inst_Wf (
	.clk(clk),
	.enable(enable),
	.in_0(WfxrXtYt_1_packed_0),
	.out_0(WfxrXtYt_1_hold_0),
	.in_1(WfxrXtYt_1_packed_1),
	.out_1(WfxrXtYt_1_hold_1),
	.in_2(WfxrXtYt_1_packed_2),
	.out_2(WfxrXtYt_1_hold_2),
	.in_3(WfxrXtYt_1_packed_3),
	.out_3(WfxrXtYt_1_hold_3),
	.in_4(WfxrXtYt_1_packed_4),
	.out_4(WfxrXtYt_1_hold_4),
	.in_5(WfxrXtYt_1_packed_5),
	.out_5(WfxrXtYt_1_hold_5),
	.in_6(WfxrXtYt_1_packed_6),
	.out_6(WfxrXtYt_1_hold_6),
	.in_7(WfxrXtYt_1_packed_7),
	.out_7(WfxrXtYt_1_hold_7),
	.in_8(WfxrXtYt_1_packed_8),
	.out_8(WfxrXtYt_1_hold_8),
	.in_9(WfxrXtYt_1_packed_9),
	.out_9(WfxrXtYt_1_hold_9),
	.in_10(WfxrXtYt_1_packed_10),
	.out_10(WfxrXtYt_1_hold_10),
	.in_11(WfxrXtYt_1_packed_11),
	.out_11(WfxrXtYt_1_hold_11),
	.in_12(WfxrXtYt_1_packed_12),
	.out_12(WfxrXtYt_1_hold_12),
	.in_13(WfxrXtYt_1_packed_13),
	.out_13(WfxrXtYt_1_hold_13),
	.in_14(WfxrXtYt_1_packed_14),
	.out_14(WfxrXtYt_1_hold_14),
	.in_15(WfxrXtYt_1_packed_15),
	.out_15(WfxrXtYt_1_hold_15),
	.reset(reset)
);

shift_register_group_18_16_3 shift_register_group_18_16_3_inst_Wo (
	.clk(clk),
	.enable(enable),
	.in_0(WoxrXtYt_1_packed_0),
	.out_0(WoxrXtYt_1_hold_0),
	.in_1(WoxrXtYt_1_packed_1),
	.out_1(WoxrXtYt_1_hold_1),
	.in_2(WoxrXtYt_1_packed_2),
	.out_2(WoxrXtYt_1_hold_2),
	.in_3(WoxrXtYt_1_packed_3),
	.out_3(WoxrXtYt_1_hold_3),
	.in_4(WoxrXtYt_1_packed_4),
	.out_4(WoxrXtYt_1_hold_4),
	.in_5(WoxrXtYt_1_packed_5),
	.out_5(WoxrXtYt_1_hold_5),
	.in_6(WoxrXtYt_1_packed_6),
	.out_6(WoxrXtYt_1_hold_6),
	.in_7(WoxrXtYt_1_packed_7),
	.out_7(WoxrXtYt_1_hold_7),
	.in_8(WoxrXtYt_1_packed_8),
	.out_8(WoxrXtYt_1_hold_8),
	.in_9(WoxrXtYt_1_packed_9),
	.out_9(WoxrXtYt_1_hold_9),
	.in_10(WoxrXtYt_1_packed_10),
	.out_10(WoxrXtYt_1_hold_10),
	.in_11(WoxrXtYt_1_packed_11),
	.out_11(WoxrXtYt_1_hold_11),
	.in_12(WoxrXtYt_1_packed_12),
	.out_12(WoxrXtYt_1_hold_12),
	.in_13(WoxrXtYt_1_packed_13),
	.out_13(WoxrXtYt_1_hold_13),
	.in_14(WoxrXtYt_1_packed_14),
	.out_14(WoxrXtYt_1_hold_14),
	.in_15(WoxrXtYt_1_packed_15),
	.out_15(WoxrXtYt_1_hold_15),
	.reset(reset)
);

shift_register_group_18_16_3 shift_register_group_18_16_3_inst_Wc (
	.clk(clk),
	.enable(enable),
	.in_0(WcxrXtYt_1_packed_0),
	.out_0(WcxrXtYt_1_hold_0),
	.in_1(WcxrXtYt_1_packed_1),
	.out_1(WcxrXtYt_1_hold_1),
	.in_2(WcxrXtYt_1_packed_2),
	.out_2(WcxrXtYt_1_hold_2),
	.in_3(WcxrXtYt_1_packed_3),
	.out_3(WcxrXtYt_1_hold_3),
	.in_4(WcxrXtYt_1_packed_4),
	.out_4(WcxrXtYt_1_hold_4),
	.in_5(WcxrXtYt_1_packed_5),
	.out_5(WcxrXtYt_1_hold_5),
	.in_6(WcxrXtYt_1_packed_6),
	.out_6(WcxrXtYt_1_hold_6),
	.in_7(WcxrXtYt_1_packed_7),
	.out_7(WcxrXtYt_1_hold_7),
	.in_8(WcxrXtYt_1_packed_8),
	.out_8(WcxrXtYt_1_hold_8),
	.in_9(WcxrXtYt_1_packed_9),
	.out_9(WcxrXtYt_1_hold_9),
	.in_10(WcxrXtYt_1_packed_10),
	.out_10(WcxrXtYt_1_hold_10),
	.in_11(WcxrXtYt_1_packed_11),
	.out_11(WcxrXtYt_1_hold_11),
	.in_12(WcxrXtYt_1_packed_12),
	.out_12(WcxrXtYt_1_hold_12),
	.in_13(WcxrXtYt_1_packed_13),
	.out_13(WcxrXtYt_1_hold_13),
	.in_14(WcxrXtYt_1_packed_14),
	.out_14(WcxrXtYt_1_hold_14),
	.in_15(WcxrXtYt_1_packed_15),
	.out_15(WcxrXtYt_1_hold_15),
	.reset(reset)
);

shift_register_unit_1_3 shift_register_unit_1_3_inst_s2 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(stage1_valid),
	.out(stage1_valid_hold)
);

C_LSTM_stage_2_18_10_16_1 C_LSTM_stage_2_18_10_16_1_inst (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(stage1_valid_hold),
	.Ct_1_0(Ct_1_0),
	.WixrXtYt_1_0(WixrXtYt_1_hold_0),
	.Wic_0(Wic_0),
	.bi_0(bi_0),
	.WfxrXtYt_1_0(WfxrXtYt_1_hold_0),
	.Wfc_0(Wfc_0),
	.bf_0(bf_0),
	.WoxrXtYt_1_0(WoxrXtYt_1_hold_0),
	.Woc_0(Woc_0),
	.bo_0(bo_0),
	.WcxrXtYt_1_0(WcxrXtYt_1_hold_0),
	.bc_0(bc_0),
	.out_mt_0(stage2_mt_0),
	.out_ct_0(stage2_Ct_0),
	.Ct_1_1(Ct_1_1),
	.WixrXtYt_1_1(WixrXtYt_1_hold_1),
	.Wic_1(Wic_1),
	.bi_1(bi_1),
	.WfxrXtYt_1_1(WfxrXtYt_1_hold_1),
	.Wfc_1(Wfc_1),
	.bf_1(bf_1),
	.WoxrXtYt_1_1(WoxrXtYt_1_hold_1),
	.Woc_1(Woc_1),
	.bo_1(bo_1),
	.WcxrXtYt_1_1(WcxrXtYt_1_hold_1),
	.bc_1(bc_1),
	.out_mt_1(stage2_mt_1),
	.out_ct_1(stage2_Ct_1),
	.Ct_1_2(Ct_1_2),
	.WixrXtYt_1_2(WixrXtYt_1_hold_2),
	.Wic_2(Wic_2),
	.bi_2(bi_2),
	.WfxrXtYt_1_2(WfxrXtYt_1_hold_2),
	.Wfc_2(Wfc_2),
	.bf_2(bf_2),
	.WoxrXtYt_1_2(WoxrXtYt_1_hold_2),
	.Woc_2(Woc_2),
	.bo_2(bo_2),
	.WcxrXtYt_1_2(WcxrXtYt_1_hold_2),
	.bc_2(bc_2),
	.out_mt_2(stage2_mt_2),
	.out_ct_2(stage2_Ct_2),
	.Ct_1_3(Ct_1_3),
	.WixrXtYt_1_3(WixrXtYt_1_hold_3),
	.Wic_3(Wic_3),
	.bi_3(bi_3),
	.WfxrXtYt_1_3(WfxrXtYt_1_hold_3),
	.Wfc_3(Wfc_3),
	.bf_3(bf_3),
	.WoxrXtYt_1_3(WoxrXtYt_1_hold_3),
	.Woc_3(Woc_3),
	.bo_3(bo_3),
	.WcxrXtYt_1_3(WcxrXtYt_1_hold_3),
	.bc_3(bc_3),
	.out_mt_3(stage2_mt_3),
	.out_ct_3(stage2_Ct_3),
	.Ct_1_4(Ct_1_4),
	.WixrXtYt_1_4(WixrXtYt_1_hold_4),
	.Wic_4(Wic_4),
	.bi_4(bi_4),
	.WfxrXtYt_1_4(WfxrXtYt_1_hold_4),
	.Wfc_4(Wfc_4),
	.bf_4(bf_4),
	.WoxrXtYt_1_4(WoxrXtYt_1_hold_4),
	.Woc_4(Woc_4),
	.bo_4(bo_4),
	.WcxrXtYt_1_4(WcxrXtYt_1_hold_4),
	.bc_4(bc_4),
	.out_mt_4(stage2_mt_4),
	.out_ct_4(stage2_Ct_4),
	.Ct_1_5(Ct_1_5),
	.WixrXtYt_1_5(WixrXtYt_1_hold_5),
	.Wic_5(Wic_5),
	.bi_5(bi_5),
	.WfxrXtYt_1_5(WfxrXtYt_1_hold_5),
	.Wfc_5(Wfc_5),
	.bf_5(bf_5),
	.WoxrXtYt_1_5(WoxrXtYt_1_hold_5),
	.Woc_5(Woc_5),
	.bo_5(bo_5),
	.WcxrXtYt_1_5(WcxrXtYt_1_hold_5),
	.bc_5(bc_5),
	.out_mt_5(stage2_mt_5),
	.out_ct_5(stage2_Ct_5),
	.Ct_1_6(Ct_1_6),
	.WixrXtYt_1_6(WixrXtYt_1_hold_6),
	.Wic_6(Wic_6),
	.bi_6(bi_6),
	.WfxrXtYt_1_6(WfxrXtYt_1_hold_6),
	.Wfc_6(Wfc_6),
	.bf_6(bf_6),
	.WoxrXtYt_1_6(WoxrXtYt_1_hold_6),
	.Woc_6(Woc_6),
	.bo_6(bo_6),
	.WcxrXtYt_1_6(WcxrXtYt_1_hold_6),
	.bc_6(bc_6),
	.out_mt_6(stage2_mt_6),
	.out_ct_6(stage2_Ct_6),
	.Ct_1_7(Ct_1_7),
	.WixrXtYt_1_7(WixrXtYt_1_hold_7),
	.Wic_7(Wic_7),
	.bi_7(bi_7),
	.WfxrXtYt_1_7(WfxrXtYt_1_hold_7),
	.Wfc_7(Wfc_7),
	.bf_7(bf_7),
	.WoxrXtYt_1_7(WoxrXtYt_1_hold_7),
	.Woc_7(Woc_7),
	.bo_7(bo_7),
	.WcxrXtYt_1_7(WcxrXtYt_1_hold_7),
	.bc_7(bc_7),
	.out_mt_7(stage2_mt_7),
	.out_ct_7(stage2_Ct_7),
	.Ct_1_8(Ct_1_8),
	.WixrXtYt_1_8(WixrXtYt_1_hold_8),
	.Wic_8(Wic_8),
	.bi_8(bi_8),
	.WfxrXtYt_1_8(WfxrXtYt_1_hold_8),
	.Wfc_8(Wfc_8),
	.bf_8(bf_8),
	.WoxrXtYt_1_8(WoxrXtYt_1_hold_8),
	.Woc_8(Woc_8),
	.bo_8(bo_8),
	.WcxrXtYt_1_8(WcxrXtYt_1_hold_8),
	.bc_8(bc_8),
	.out_mt_8(stage2_mt_8),
	.out_ct_8(stage2_Ct_8),
	.Ct_1_9(Ct_1_9),
	.WixrXtYt_1_9(WixrXtYt_1_hold_9),
	.Wic_9(Wic_9),
	.bi_9(bi_9),
	.WfxrXtYt_1_9(WfxrXtYt_1_hold_9),
	.Wfc_9(Wfc_9),
	.bf_9(bf_9),
	.WoxrXtYt_1_9(WoxrXtYt_1_hold_9),
	.Woc_9(Woc_9),
	.bo_9(bo_9),
	.WcxrXtYt_1_9(WcxrXtYt_1_hold_9),
	.bc_9(bc_9),
	.out_mt_9(stage2_mt_9),
	.out_ct_9(stage2_Ct_9),
	.Ct_1_10(Ct_1_10),
	.WixrXtYt_1_10(WixrXtYt_1_hold_10),
	.Wic_10(Wic_10),
	.bi_10(bi_10),
	.WfxrXtYt_1_10(WfxrXtYt_1_hold_10),
	.Wfc_10(Wfc_10),
	.bf_10(bf_10),
	.WoxrXtYt_1_10(WoxrXtYt_1_hold_10),
	.Woc_10(Woc_10),
	.bo_10(bo_10),
	.WcxrXtYt_1_10(WcxrXtYt_1_hold_10),
	.bc_10(bc_10),
	.out_mt_10(stage2_mt_10),
	.out_ct_10(stage2_Ct_10),
	.Ct_1_11(Ct_1_11),
	.WixrXtYt_1_11(WixrXtYt_1_hold_11),
	.Wic_11(Wic_11),
	.bi_11(bi_11),
	.WfxrXtYt_1_11(WfxrXtYt_1_hold_11),
	.Wfc_11(Wfc_11),
	.bf_11(bf_11),
	.WoxrXtYt_1_11(WoxrXtYt_1_hold_11),
	.Woc_11(Woc_11),
	.bo_11(bo_11),
	.WcxrXtYt_1_11(WcxrXtYt_1_hold_11),
	.bc_11(bc_11),
	.out_mt_11(stage2_mt_11),
	.out_ct_11(stage2_Ct_11),
	.Ct_1_12(Ct_1_12),
	.WixrXtYt_1_12(WixrXtYt_1_hold_12),
	.Wic_12(Wic_12),
	.bi_12(bi_12),
	.WfxrXtYt_1_12(WfxrXtYt_1_hold_12),
	.Wfc_12(Wfc_12),
	.bf_12(bf_12),
	.WoxrXtYt_1_12(WoxrXtYt_1_hold_12),
	.Woc_12(Woc_12),
	.bo_12(bo_12),
	.WcxrXtYt_1_12(WcxrXtYt_1_hold_12),
	.bc_12(bc_12),
	.out_mt_12(stage2_mt_12),
	.out_ct_12(stage2_Ct_12),
	.Ct_1_13(Ct_1_13),
	.WixrXtYt_1_13(WixrXtYt_1_hold_13),
	.Wic_13(Wic_13),
	.bi_13(bi_13),
	.WfxrXtYt_1_13(WfxrXtYt_1_hold_13),
	.Wfc_13(Wfc_13),
	.bf_13(bf_13),
	.WoxrXtYt_1_13(WoxrXtYt_1_hold_13),
	.Woc_13(Woc_13),
	.bo_13(bo_13),
	.WcxrXtYt_1_13(WcxrXtYt_1_hold_13),
	.bc_13(bc_13),
	.out_mt_13(stage2_mt_13),
	.out_ct_13(stage2_Ct_13),
	.Ct_1_14(Ct_1_14),
	.WixrXtYt_1_14(WixrXtYt_1_hold_14),
	.Wic_14(Wic_14),
	.bi_14(bi_14),
	.WfxrXtYt_1_14(WfxrXtYt_1_hold_14),
	.Wfc_14(Wfc_14),
	.bf_14(bf_14),
	.WoxrXtYt_1_14(WoxrXtYt_1_hold_14),
	.Woc_14(Woc_14),
	.bo_14(bo_14),
	.WcxrXtYt_1_14(WcxrXtYt_1_hold_14),
	.bc_14(bc_14),
	.out_mt_14(stage2_mt_14),
	.out_ct_14(stage2_Ct_14),
	.Ct_1_15(Ct_1_15),
	.WixrXtYt_1_15(WixrXtYt_1_hold_15),
	.Wic_15(Wic_15),
	.bi_15(bi_15),
	.WfxrXtYt_1_15(WfxrXtYt_1_hold_15),
	.Wfc_15(Wfc_15),
	.bf_15(bf_15),
	.WoxrXtYt_1_15(WoxrXtYt_1_hold_15),
	.Woc_15(Woc_15),
	.bo_15(bo_15),
	.WcxrXtYt_1_15(WcxrXtYt_1_hold_15),
	.bc_15(bc_15),
	.out_mt_15(stage2_mt_15),
	.out_ct_15(stage2_Ct_15),
	.o_valid(stage2_valid),
	.o_ready(stage2_ready)
);

wire [17:0] mt_0_0;
wire [17:0] Ct_0__0;
wire [17:0] mt_0_1;
wire [17:0] Ct_0__1;
wire [17:0] mt_0_2;
wire [17:0] Ct_0__2;
wire [17:0] mt_0_3;
wire [17:0] Ct_0__3;
wire [17:0] mt_0_4;
wire [17:0] Ct_0__4;
wire [17:0] mt_0_5;
wire [17:0] Ct_0__5;
wire [17:0] mt_0_6;
wire [17:0] Ct_0__6;
wire [17:0] mt_0_7;
wire [17:0] Ct_0__7;
wire [17:0] mt_0_8;
wire [17:0] Ct_0__8;
wire [17:0] mt_0_9;
wire [17:0] Ct_0__9;
wire [17:0] mt_0_10;
wire [17:0] Ct_0__10;
wire [17:0] mt_0_11;
wire [17:0] Ct_0__11;
wire [17:0] mt_0_12;
wire [17:0] Ct_0__12;
wire [17:0] mt_0_13;
wire [17:0] Ct_0__13;
wire [17:0] mt_0_14;
wire [17:0] Ct_0__14;
wire [17:0] mt_0_15;
wire [17:0] Ct_0__15;

assign Ct_0__0 = stage2_Ct_0;
assign mt_0_0 = stage2_mt_0;
assign Ct_0__1 = stage2_Ct_1;
assign mt_0_1 = stage2_mt_1;
assign Ct_0__2 = stage2_Ct_2;
assign mt_0_2 = stage2_mt_2;
assign Ct_0__3 = stage2_Ct_3;
assign mt_0_3 = stage2_mt_3;
assign Ct_0__4 = stage2_Ct_4;
assign mt_0_4 = stage2_mt_4;
assign Ct_0__5 = stage2_Ct_5;
assign mt_0_5 = stage2_mt_5;
assign Ct_0__6 = stage2_Ct_6;
assign mt_0_6 = stage2_mt_6;
assign Ct_0__7 = stage2_Ct_7;
assign mt_0_7 = stage2_mt_7;
assign Ct_0__8 = stage2_Ct_8;
assign mt_0_8 = stage2_mt_8;
assign Ct_0__9 = stage2_Ct_9;
assign mt_0_9 = stage2_mt_9;
assign Ct_0__10 = stage2_Ct_10;
assign mt_0_10 = stage2_mt_10;
assign Ct_0__11 = stage2_Ct_11;
assign mt_0_11 = stage2_mt_11;
assign Ct_0__12 = stage2_Ct_12;
assign mt_0_12 = stage2_mt_12;
assign Ct_0__13 = stage2_Ct_13;
assign mt_0_13 = stage2_mt_13;
assign Ct_0__14 = stage2_Ct_14;
assign mt_0_14 = stage2_mt_14;
assign Ct_0__15 = stage2_Ct_15;
assign mt_0_15 = stage2_mt_15;

// C-LSTM buffer between stage2 and stage3
wire buffer_out_valid, pipelined_mt_valid, pipelined_Ct_valid;
wire [17:0] mt_pipelined_0;
wire [17:0] Ct_pipelined_0;
wire [17:0] out_mt_buffer_0;
wire [17:0] mt_pipelined_1;
wire [17:0] Ct_pipelined_1;
wire [17:0] out_mt_buffer_1;
wire [17:0] mt_pipelined_2;
wire [17:0] Ct_pipelined_2;
wire [17:0] out_mt_buffer_2;
wire [17:0] mt_pipelined_3;
wire [17:0] Ct_pipelined_3;
wire [17:0] out_mt_buffer_3;
wire [17:0] mt_pipelined_4;
wire [17:0] Ct_pipelined_4;
wire [17:0] out_mt_buffer_4;
wire [17:0] mt_pipelined_5;
wire [17:0] Ct_pipelined_5;
wire [17:0] out_mt_buffer_5;
wire [17:0] mt_pipelined_6;
wire [17:0] Ct_pipelined_6;
wire [17:0] out_mt_buffer_6;
wire [17:0] mt_pipelined_7;
wire [17:0] Ct_pipelined_7;
wire [17:0] out_mt_buffer_7;
wire [17:0] mt_pipelined_8;
wire [17:0] Ct_pipelined_8;
wire [17:0] out_mt_buffer_8;
wire [17:0] mt_pipelined_9;
wire [17:0] Ct_pipelined_9;
wire [17:0] out_mt_buffer_9;
wire [17:0] mt_pipelined_10;
wire [17:0] Ct_pipelined_10;
wire [17:0] out_mt_buffer_10;
wire [17:0] mt_pipelined_11;
wire [17:0] Ct_pipelined_11;
wire [17:0] out_mt_buffer_11;
wire [17:0] mt_pipelined_12;
wire [17:0] Ct_pipelined_12;
wire [17:0] out_mt_buffer_12;
wire [17:0] mt_pipelined_13;
wire [17:0] Ct_pipelined_13;
wire [17:0] out_mt_buffer_13;
wire [17:0] mt_pipelined_14;
wire [17:0] Ct_pipelined_14;
wire [17:0] out_mt_buffer_14;
wire [17:0] mt_pipelined_15;
wire [17:0] Ct_pipelined_15;
wire [17:0] out_mt_buffer_15;

pipelined_input_18_1_16 pipelined_input_18_1_16_inst_mt (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.load_input(stage2_valid),
	.i_data_0_0(mt_0_0),
	.i_data_0_1(mt_0_1),
	.i_data_0_2(mt_0_2),
	.i_data_0_3(mt_0_3),
	.i_data_0_4(mt_0_4),
	.i_data_0_5(mt_0_5),
	.i_data_0_6(mt_0_6),
	.i_data_0_7(mt_0_7),
	.i_data_0_8(mt_0_8),
	.i_data_0_9(mt_0_9),
	.i_data_0_10(mt_0_10),
	.i_data_0_11(mt_0_11),
	.i_data_0_12(mt_0_12),
	.i_data_0_13(mt_0_13),
	.i_data_0_14(mt_0_14),
	.i_data_0_15(mt_0_15),
	.o_data_0(mt_pipelined_0),
	.o_data_1(mt_pipelined_1),
	.o_data_2(mt_pipelined_2),
	.o_data_3(mt_pipelined_3),
	.o_data_4(mt_pipelined_4),
	.o_data_5(mt_pipelined_5),
	.o_data_6(mt_pipelined_6),
	.o_data_7(mt_pipelined_7),
	.o_data_8(mt_pipelined_8),
	.o_data_9(mt_pipelined_9),
	.o_data_10(mt_pipelined_10),
	.o_data_11(mt_pipelined_11),
	.o_data_12(mt_pipelined_12),
	.o_data_13(mt_pipelined_13),
	.o_data_14(mt_pipelined_14),
	.o_data_15(mt_pipelined_15),
	.o_valid(pipelined_mt_valid)
);

pipelined_input_18_1_16 pipelined_input_18_1_16_inst_Ct (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.load_input(stage2_valid),
	.i_data_0_0(Ct_0__0),
	.i_data_0_1(Ct_0__1),
	.i_data_0_2(Ct_0__2),
	.i_data_0_3(Ct_0__3),
	.i_data_0_4(Ct_0__4),
	.i_data_0_5(Ct_0__5),
	.i_data_0_6(Ct_0__6),
	.i_data_0_7(Ct_0__7),
	.i_data_0_8(Ct_0__8),
	.i_data_0_9(Ct_0__9),
	.i_data_0_10(Ct_0__10),
	.i_data_0_11(Ct_0__11),
	.i_data_0_12(Ct_0__12),
	.i_data_0_13(Ct_0__13),
	.i_data_0_14(Ct_0__14),
	.i_data_0_15(Ct_0__15),
	.o_data_0(Ct_pipelined_0),
	.o_data_1(Ct_pipelined_1),
	.o_data_2(Ct_pipelined_2),
	.o_data_3(Ct_pipelined_3),
	.o_data_4(Ct_pipelined_4),
	.o_data_5(Ct_pipelined_5),
	.o_data_6(Ct_pipelined_6),
	.o_data_7(Ct_pipelined_7),
	.o_data_8(Ct_pipelined_8),
	.o_data_9(Ct_pipelined_9),
	.o_data_10(Ct_pipelined_10),
	.o_data_11(Ct_pipelined_11),
	.o_data_12(Ct_pipelined_12),
	.o_data_13(Ct_pipelined_13),
	.o_data_14(Ct_pipelined_14),
	.o_data_15(Ct_pipelined_15),
	.o_valid(pipelined_Ct_valid)
);

stage2_Ct_buffer_18_1_16_64 stage2_Ct_buffer_18_1_16_64_inst (
	.clk(clk),
	.reset(reset),
	.wen(pipelined_Ct_valid),
	.ren(stage1_valid),
	.i_Ct_0(Ct_pipelined_0),
	.i_Ct_1(Ct_pipelined_1),
	.i_Ct_2(Ct_pipelined_2),
	.i_Ct_3(Ct_pipelined_3),
	.i_Ct_4(Ct_pipelined_4),
	.i_Ct_5(Ct_pipelined_5),
	.i_Ct_6(Ct_pipelined_6),
	.i_Ct_7(Ct_pipelined_7),
	.i_Ct_8(Ct_pipelined_8),
	.i_Ct_9(Ct_pipelined_9),
	.i_Ct_10(Ct_pipelined_10),
	.i_Ct_11(Ct_pipelined_11),
	.i_Ct_12(Ct_pipelined_12),
	.i_Ct_13(Ct_pipelined_13),
	.i_Ct_14(Ct_pipelined_14),
	.i_Ct_15(Ct_pipelined_15),
	.o_Ct_0(Ct_1_0),
	.o_Ct_1(Ct_1_1),
	.o_Ct_2(Ct_1_2),
	.o_Ct_3(Ct_1_3),
	.o_Ct_4(Ct_1_4),
	.o_Ct_5(Ct_1_5),
	.o_Ct_6(Ct_1_6),
	.o_Ct_7(Ct_1_7),
	.o_Ct_8(Ct_1_8),
	.o_Ct_9(Ct_1_9),
	.o_Ct_10(Ct_1_10),
	.o_Ct_11(Ct_1_11),
	.o_Ct_12(Ct_1_12),
	.o_Ct_13(Ct_1_13),
	.o_Ct_14(Ct_1_14),
	.o_Ct_15(Ct_1_15),
	.o_valid()
);

stage2_mt_buffer_18_1_16_64_32 stage2_mt_buffer_18_1_16_64_32_inst (
	.clk(clk),
	.reset(reset),
	.i_valid(pipelined_mt_valid),
	.data_0(mt_pipelined_0),
	.q_0(out_mt_buffer_0),
	.data_1(mt_pipelined_1),
	.q_1(out_mt_buffer_1),
	.data_2(mt_pipelined_2),
	.q_2(out_mt_buffer_2),
	.data_3(mt_pipelined_3),
	.q_3(out_mt_buffer_3),
	.data_4(mt_pipelined_4),
	.q_4(out_mt_buffer_4),
	.data_5(mt_pipelined_5),
	.q_5(out_mt_buffer_5),
	.data_6(mt_pipelined_6),
	.q_6(out_mt_buffer_6),
	.data_7(mt_pipelined_7),
	.q_7(out_mt_buffer_7),
	.data_8(mt_pipelined_8),
	.q_8(out_mt_buffer_8),
	.data_9(mt_pipelined_9),
	.q_9(out_mt_buffer_9),
	.data_10(mt_pipelined_10),
	.q_10(out_mt_buffer_10),
	.data_11(mt_pipelined_11),
	.q_11(out_mt_buffer_11),
	.data_12(mt_pipelined_12),
	.q_12(out_mt_buffer_12),
	.data_13(mt_pipelined_13),
	.q_13(out_mt_buffer_13),
	.data_14(mt_pipelined_14),
	.q_14(out_mt_buffer_14),
	.data_15(mt_pipelined_15),
	.q_15(out_mt_buffer_15),
	.o_valid(buffer_out_valid)
);

// C-LSTM Stage 3 and inner-connections
wire stage3_valid, stage3_ready;
wire [17:0] new_Yt_0_0;
wire [17:0] new_Yt_0_1;
wire [17:0] new_Yt_0_2;
wire [17:0] new_Yt_0_3;
wire [17:0] new_Yt_0_4;
wire [17:0] new_Yt_0_5;
wire [17:0] new_Yt_0_6;
wire [17:0] new_Yt_0_7;
wire [17:0] new_Yt_0_8;
wire [17:0] new_Yt_0_9;
wire [17:0] new_Yt_0_10;
wire [17:0] new_Yt_0_11;
wire [17:0] new_Yt_0_12;
wire [17:0] new_Yt_0_13;
wire [17:0] new_Yt_0_14;
wire [17:0] new_Yt_0_15;

C_LSTM_stage_3_18_10_64_2048_1_16_1 C_LSTM_stage_3_18_10_64_2048_1_16_1_inst (
	.clk(clk),
	.reset(reset),
	.i_ready(enable),
	.i_valid(buffer_out_valid),
	.i_mt_0(out_mt_buffer_0),
	.i_mt_1(out_mt_buffer_1),
	.i_mt_2(out_mt_buffer_2),
	.i_mt_3(out_mt_buffer_3),
	.i_mt_4(out_mt_buffer_4),
	.i_mt_5(out_mt_buffer_5),
	.i_mt_6(out_mt_buffer_6),
	.i_mt_7(out_mt_buffer_7),
	.i_mt_8(out_mt_buffer_8),
	.i_mt_9(out_mt_buffer_9),
	.i_mt_10(out_mt_buffer_10),
	.i_mt_11(out_mt_buffer_11),
	.i_mt_12(out_mt_buffer_12),
	.i_mt_13(out_mt_buffer_13),
	.i_mt_14(out_mt_buffer_14),
	.i_mt_15(out_mt_buffer_15),
	.o_Yt_0_0(new_Yt_0_0),
	.o_Yt_0_1(new_Yt_0_1),
	.o_Yt_0_2(new_Yt_0_2),
	.o_Yt_0_3(new_Yt_0_3),
	.o_Yt_0_4(new_Yt_0_4),
	.o_Yt_0_5(new_Yt_0_5),
	.o_Yt_0_6(new_Yt_0_6),
	.o_Yt_0_7(new_Yt_0_7),
	.o_Yt_0_8(new_Yt_0_8),
	.o_Yt_0_9(new_Yt_0_9),
	.o_Yt_0_10(new_Yt_0_10),
	.o_Yt_0_11(new_Yt_0_11),
	.o_Yt_0_12(new_Yt_0_12),
	.o_Yt_0_13(new_Yt_0_13),
	.o_Yt_0_14(new_Yt_0_14),
	.o_Yt_0_15(new_Yt_0_15),
	.o_valid(stage3_valid),
	.o_ready(stage3_ready)
);

assign o_Yt_0_0 = new_Yt_0_0;
assign o_Yt_0_1 = new_Yt_0_1;
assign o_Yt_0_2 = new_Yt_0_2;
assign o_Yt_0_3 = new_Yt_0_3;
assign o_Yt_0_4 = new_Yt_0_4;
assign o_Yt_0_5 = new_Yt_0_5;
assign o_Yt_0_6 = new_Yt_0_6;
assign o_Yt_0_7 = new_Yt_0_7;
assign o_Yt_0_8 = new_Yt_0_8;
assign o_Yt_0_9 = new_Yt_0_9;
assign o_Yt_0_10 = new_Yt_0_10;
assign o_Yt_0_11 = new_Yt_0_11;
assign o_Yt_0_12 = new_Yt_0_12;
assign o_Yt_0_13 = new_Yt_0_13;
assign o_Yt_0_14 = new_Yt_0_14;
assign o_Yt_0_15 = new_Yt_0_15;

// Stage 3 buffer and inter-connections
wire [17:0] pipelined_Yt_0;
wire [17:0] pipelined_Yt_1;
wire [17:0] pipelined_Yt_2;
wire [17:0] pipelined_Yt_3;
wire [17:0] pipelined_Yt_4;
wire [17:0] pipelined_Yt_5;
wire [17:0] pipelined_Yt_6;
wire [17:0] pipelined_Yt_7;
wire [17:0] pipelined_Yt_8;
wire [17:0] pipelined_Yt_9;
wire [17:0] pipelined_Yt_10;
wire [17:0] pipelined_Yt_11;
wire [17:0] pipelined_Yt_12;
wire [17:0] pipelined_Yt_13;
wire [17:0] pipelined_Yt_14;
wire [17:0] pipelined_Yt_15;
wire pipelined_Yt_valid;

pipelined_input_18_1_16 pipelined_input_18_1_16_inst_Y (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.load_input(stage3_valid),
	.i_data_0_0(new_Yt_0_0),
	.i_data_0_1(new_Yt_0_1),
	.i_data_0_2(new_Yt_0_2),
	.i_data_0_3(new_Yt_0_3),
	.i_data_0_4(new_Yt_0_4),
	.i_data_0_5(new_Yt_0_5),
	.i_data_0_6(new_Yt_0_6),
	.i_data_0_7(new_Yt_0_7),
	.i_data_0_8(new_Yt_0_8),
	.i_data_0_9(new_Yt_0_9),
	.i_data_0_10(new_Yt_0_10),
	.i_data_0_11(new_Yt_0_11),
	.i_data_0_12(new_Yt_0_12),
	.i_data_0_13(new_Yt_0_13),
	.i_data_0_14(new_Yt_0_14),
	.i_data_0_15(new_Yt_0_15),
	.o_data_0(pipelined_Yt_0),
	.o_data_1(pipelined_Yt_1),
	.o_data_2(pipelined_Yt_2),
	.o_data_3(pipelined_Yt_3),
	.o_data_4(pipelined_Yt_4),
	.o_data_5(pipelined_Yt_5),
	.o_data_6(pipelined_Yt_6),
	.o_data_7(pipelined_Yt_7),
	.o_data_8(pipelined_Yt_8),
	.o_data_9(pipelined_Yt_9),
	.o_data_10(pipelined_Yt_10),
	.o_data_11(pipelined_Yt_11),
	.o_data_12(pipelined_Yt_12),
	.o_data_13(pipelined_Yt_13),
	.o_data_14(pipelined_Yt_14),
	.o_data_15(pipelined_Yt_15),
	.o_valid(pipelined_Yt_valid)
);

stage3_X_Y_buffer_18_16_1_10_32_64 stage3_X_Y_buffer_18_16_1_10_32_64_inst (
	.clk(clk),
	.reset(reset),
	.i_X_valid(i_valid),
	.i_Y_valid(pipelined_Yt_valid),
	.feed_start(start_compute),
	.i_X_data_0(i_X_data_0),
	.i_Y_data_0(pipelined_Yt_0),
	.o_data_0(i_data_0),
	.i_X_data_1(i_X_data_1),
	.i_Y_data_1(pipelined_Yt_1),
	.o_data_1(i_data_1),
	.i_X_data_2(i_X_data_2),
	.i_Y_data_2(pipelined_Yt_2),
	.o_data_2(i_data_2),
	.i_X_data_3(i_X_data_3),
	.i_Y_data_3(pipelined_Yt_3),
	.o_data_3(i_data_3),
	.i_X_data_4(i_X_data_4),
	.i_Y_data_4(pipelined_Yt_4),
	.o_data_4(i_data_4),
	.i_X_data_5(i_X_data_5),
	.i_Y_data_5(pipelined_Yt_5),
	.o_data_5(i_data_5),
	.i_X_data_6(i_X_data_6),
	.i_Y_data_6(pipelined_Yt_6),
	.o_data_6(i_data_6),
	.i_X_data_7(i_X_data_7),
	.i_Y_data_7(pipelined_Yt_7),
	.o_data_7(i_data_7),
	.i_X_data_8(i_X_data_8),
	.i_Y_data_8(pipelined_Yt_8),
	.o_data_8(i_data_8),
	.i_X_data_9(i_X_data_9),
	.i_Y_data_9(pipelined_Yt_9),
	.o_data_9(i_data_9),
	.i_X_data_10(i_X_data_10),
	.i_Y_data_10(pipelined_Yt_10),
	.o_data_10(i_data_10),
	.i_X_data_11(i_X_data_11),
	.i_Y_data_11(pipelined_Yt_11),
	.o_data_11(i_data_11),
	.i_X_data_12(i_X_data_12),
	.i_Y_data_12(pipelined_Yt_12),
	.o_data_12(i_data_12),
	.i_X_data_13(i_X_data_13),
	.i_Y_data_13(pipelined_Yt_13),
	.o_data_13(i_data_13),
	.i_X_data_14(i_X_data_14),
	.i_Y_data_14(pipelined_Yt_14),
	.o_data_14(i_data_14),
	.i_X_data_15(i_X_data_15),
	.i_Y_data_15(pipelined_Yt_15),
	.o_data_15(i_data_15),
	.o_valid(out_X_Y_buffer_valid),
	.o_ready(o_ready)
);

always @ (posedge clk) begin
	if(reset) begin
		reg_o_data_0_0 <= 0;
		reg_o_data_0_1 <= 0;
		reg_o_data_0_2 <= 0;
		reg_o_data_0_3 <= 0;
		reg_o_data_0_4 <= 0;
		reg_o_data_0_5 <= 0;
		reg_o_data_0_6 <= 0;
		reg_o_data_0_7 <= 0;
		reg_o_data_0_8 <= 0;
		reg_o_data_0_9 <= 0;
		reg_o_data_0_10 <= 0;
		reg_o_data_0_11 <= 0;
		reg_o_data_0_12 <= 0;
		reg_o_data_0_13 <= 0;
		reg_o_data_0_14 <= 0;
		reg_o_data_0_15 <= 0;
		reg_o_valid <= 1'b0;
	end
end

assign o_valid = stage3_valid;

endmodule

module stage1_parameter_buffer_18_1_16_42_2688 (
	input clk,
	input reset,
	output [17:0] Wixr_real_0_0,
	output [17:0] Wixr_imag_0_0,
	output [17:0] Wfxr_real_0_0,
	output [17:0] Wfxr_imag_0_0,
	output [17:0] Woxr_real_0_0,
	output [17:0] Woxr_imag_0_0,
	output [17:0] Wcxr_real_0_0,
	output [17:0] Wcxr_imag_0_0,
	output [17:0] Wixr_real_0_1,
	output [17:0] Wixr_imag_0_1,
	output [17:0] Wfxr_real_0_1,
	output [17:0] Wfxr_imag_0_1,
	output [17:0] Woxr_real_0_1,
	output [17:0] Woxr_imag_0_1,
	output [17:0] Wcxr_real_0_1,
	output [17:0] Wcxr_imag_0_1,
	output [17:0] Wixr_real_0_2,
	output [17:0] Wixr_imag_0_2,
	output [17:0] Wfxr_real_0_2,
	output [17:0] Wfxr_imag_0_2,
	output [17:0] Woxr_real_0_2,
	output [17:0] Woxr_imag_0_2,
	output [17:0] Wcxr_real_0_2,
	output [17:0] Wcxr_imag_0_2,
	output [17:0] Wixr_real_0_3,
	output [17:0] Wixr_imag_0_3,
	output [17:0] Wfxr_real_0_3,
	output [17:0] Wfxr_imag_0_3,
	output [17:0] Woxr_real_0_3,
	output [17:0] Woxr_imag_0_3,
	output [17:0] Wcxr_real_0_3,
	output [17:0] Wcxr_imag_0_3,
	output [17:0] Wixr_real_0_4,
	output [17:0] Wixr_imag_0_4,
	output [17:0] Wfxr_real_0_4,
	output [17:0] Wfxr_imag_0_4,
	output [17:0] Woxr_real_0_4,
	output [17:0] Woxr_imag_0_4,
	output [17:0] Wcxr_real_0_4,
	output [17:0] Wcxr_imag_0_4,
	output [17:0] Wixr_real_0_5,
	output [17:0] Wixr_imag_0_5,
	output [17:0] Wfxr_real_0_5,
	output [17:0] Wfxr_imag_0_5,
	output [17:0] Woxr_real_0_5,
	output [17:0] Woxr_imag_0_5,
	output [17:0] Wcxr_real_0_5,
	output [17:0] Wcxr_imag_0_5,
	output [17:0] Wixr_real_0_6,
	output [17:0] Wixr_imag_0_6,
	output [17:0] Wfxr_real_0_6,
	output [17:0] Wfxr_imag_0_6,
	output [17:0] Woxr_real_0_6,
	output [17:0] Woxr_imag_0_6,
	output [17:0] Wcxr_real_0_6,
	output [17:0] Wcxr_imag_0_6,
	output [17:0] Wixr_real_0_7,
	output [17:0] Wixr_imag_0_7,
	output [17:0] Wfxr_real_0_7,
	output [17:0] Wfxr_imag_0_7,
	output [17:0] Woxr_real_0_7,
	output [17:0] Woxr_imag_0_7,
	output [17:0] Wcxr_real_0_7,
	output [17:0] Wcxr_imag_0_7,
	output [17:0] Wixr_real_0_8,
	output [17:0] Wixr_imag_0_8,
	output [17:0] Wfxr_real_0_8,
	output [17:0] Wfxr_imag_0_8,
	output [17:0] Woxr_real_0_8,
	output [17:0] Woxr_imag_0_8,
	output [17:0] Wcxr_real_0_8,
	output [17:0] Wcxr_imag_0_8,
	input incr_index
);

// A counter that counts how many sub blocks we have processed
wire [13:0] input_index_counter;
counter_41_1 counter_41_1_inst (
	.clk(clk),
	.reset(reset),
	.ena(incr_index),
	.count(input_index_counter)
);

wire incr_row_index;
assign incr_row_index = (input_index_counter == 41);
wire counter_enable_row_index;
assign counter_enable_row_index = (incr_row_index & incr_index);

// A counter that records which weight portion to use
wire [13:0] weight_row_index_counter;
counter_63_1 counter_63_1_inst (
	.clk(clk),
	.reset(reset),
	.ena(counter_enable_row_index),
	.count(weight_row_index_counter)
);

reg [13:0] weight_index;
always @ (*) begin
	weight_index = weight_row_index_counter * 42 + input_index_counter;
end

// Input Gate
weight_buffer_18_9_42_1_2688Wixr_real_half_0 Wixr_real_buffer (
	.clk(clk),
	.q_0_0(Wixr_real_0_0),
	.q_0_1(Wixr_real_0_1),
	.q_0_2(Wixr_real_0_2),
	.q_0_3(Wixr_real_0_3),
	.q_0_4(Wixr_real_0_4),
	.q_0_5(Wixr_real_0_5),
	.q_0_6(Wixr_real_0_6),
	.q_0_7(Wixr_real_0_7),
	.q_0_8(Wixr_real_0_8),
	.index(weight_index)
);

weight_buffer_18_9_42_1_2688Wixr_imag_half_0 Wixr_imag_buffer (
	.clk(clk),
	.q_0_0(Wixr_imag_0_0),
	.q_0_1(Wixr_imag_0_1),
	.q_0_2(Wixr_imag_0_2),
	.q_0_3(Wixr_imag_0_3),
	.q_0_4(Wixr_imag_0_4),
	.q_0_5(Wixr_imag_0_5),
	.q_0_6(Wixr_imag_0_6),
	.q_0_7(Wixr_imag_0_7),
	.q_0_8(Wixr_imag_0_8),
	.index(weight_index)
);

// Forget Gate
weight_buffer_18_9_42_1_2688Wfxr_real_half_0 Wfxr_real_buffer (
	.clk(clk),
	.q_0_0(Wfxr_real_0_0),
	.q_0_1(Wfxr_real_0_1),
	.q_0_2(Wfxr_real_0_2),
	.q_0_3(Wfxr_real_0_3),
	.q_0_4(Wfxr_real_0_4),
	.q_0_5(Wfxr_real_0_5),
	.q_0_6(Wfxr_real_0_6),
	.q_0_7(Wfxr_real_0_7),
	.q_0_8(Wfxr_real_0_8),
	.index(weight_index)
);

weight_buffer_18_9_42_1_2688Wfxr_imag_half_0 Wfxr_imag_buffer (
	.clk(clk),
	.q_0_0(Wfxr_imag_0_0),
	.q_0_1(Wfxr_imag_0_1),
	.q_0_2(Wfxr_imag_0_2),
	.q_0_3(Wfxr_imag_0_3),
	.q_0_4(Wfxr_imag_0_4),
	.q_0_5(Wfxr_imag_0_5),
	.q_0_6(Wfxr_imag_0_6),
	.q_0_7(Wfxr_imag_0_7),
	.q_0_8(Wfxr_imag_0_8),
	.index(weight_index)
);

// Output Gate
weight_buffer_18_9_42_1_2688Woxr_real_half_0 Woxr_real_buffer (
	.clk(clk),
	.q_0_0(Woxr_real_0_0),
	.q_0_1(Woxr_real_0_1),
	.q_0_2(Woxr_real_0_2),
	.q_0_3(Woxr_real_0_3),
	.q_0_4(Woxr_real_0_4),
	.q_0_5(Woxr_real_0_5),
	.q_0_6(Woxr_real_0_6),
	.q_0_7(Woxr_real_0_7),
	.q_0_8(Woxr_real_0_8),
	.index(weight_index)
);

weight_buffer_18_9_42_1_2688Woxr_imag_half_0 Woxr_imag_buffer (
	.clk(clk),
	.q_0_0(Woxr_imag_0_0),
	.q_0_1(Woxr_imag_0_1),
	.q_0_2(Woxr_imag_0_2),
	.q_0_3(Woxr_imag_0_3),
	.q_0_4(Woxr_imag_0_4),
	.q_0_5(Woxr_imag_0_5),
	.q_0_6(Woxr_imag_0_6),
	.q_0_7(Woxr_imag_0_7),
	.q_0_8(Woxr_imag_0_8),
	.index(weight_index)
);

// Output Activation Gate
weight_buffer_18_9_42_1_2688Wcxr_real_half_0 Wcxr_real_buffer (
	.clk(clk),
	.q_0_0(Wcxr_real_0_0),
	.q_0_1(Wcxr_real_0_1),
	.q_0_2(Wcxr_real_0_2),
	.q_0_3(Wcxr_real_0_3),
	.q_0_4(Wcxr_real_0_4),
	.q_0_5(Wcxr_real_0_5),
	.q_0_6(Wcxr_real_0_6),
	.q_0_7(Wcxr_real_0_7),
	.q_0_8(Wcxr_real_0_8),
	.index(weight_index)
);

weight_buffer_18_9_42_1_2688Wcxr_imag_half_0 Wcxr_imag_buffer (
	.clk(clk),
	.q_0_0(Wcxr_imag_0_0),
	.q_0_1(Wcxr_imag_0_1),
	.q_0_2(Wcxr_imag_0_2),
	.q_0_3(Wcxr_imag_0_3),
	.q_0_4(Wcxr_imag_0_4),
	.q_0_5(Wcxr_imag_0_5),
	.q_0_6(Wcxr_imag_0_6),
	.q_0_7(Wcxr_imag_0_7),
	.q_0_8(Wcxr_imag_0_8),
	.index(weight_index)
);

endmodule


module weight_buffer_18_9_42_1_2688Woxr_imag_half_0 (
	input clk,
	output [17:0] q_0_0,
	output [17:0] q_0_1,
	output [17:0] q_0_2,
	output [17:0] q_0_3,
	output [17:0] q_0_4,
	output [17:0] q_0_5,
	output [17:0] q_0_6,
	output [17:0] q_0_7,
	output [17:0] q_0_8,
	input [11:0] index
);

wire [161:0] packed_result_0;
reg [11:0] addrs_0;
reg [11:0] addrs_base_0;

always @ (posedge clk) begin
	addrs_base_0 <= 0;
	addrs_0 <= index + addrs_base_0;
end

wire rom_we;
assign rom_we = 1'b0;

single_port_ram ram_inst_0 (
	.we(rom_we),
	.addr(addrs_0),
	.data(162'd0),
	.out(packed_result_0),
	.clk(clk)
);

// Unpack result
assign q_0_0 = packed_result_0[17:0];
assign q_0_1 = packed_result_0[35:18];
assign q_0_2 = packed_result_0[53:36];
assign q_0_3 = packed_result_0[71:54];
assign q_0_4 = packed_result_0[89:72];
assign q_0_5 = packed_result_0[107:90];
assign q_0_6 = packed_result_0[125:108];
assign q_0_7 = packed_result_0[143:126];
assign q_0_8 = packed_result_0[161:144];

endmodule

module weight_buffer_18_9_42_1_2688Wixr_real_half_0 (
	input clk,
	output [17:0] q_0_0,
	output [17:0] q_0_1,
	output [17:0] q_0_2,
	output [17:0] q_0_3,
	output [17:0] q_0_4,
	output [17:0] q_0_5,
	output [17:0] q_0_6,
	output [17:0] q_0_7,
	output [17:0] q_0_8,
	input [11:0] index
);

wire [161:0] packed_result_0;
reg [11:0] addrs_0;
reg [11:0] addrs_base_0;

always @ (posedge clk) begin
	addrs_base_0 <= 0;
	addrs_0 <= index + addrs_base_0;
end

wire rom_we;
assign rom_we = 1'b0;

single_port_ram ram_inst_0 (
	.we(rom_we),
	.addr(addrs_0),
	.data(162'd0),
	.out(packed_result_0),
	.clk(clk)
);

// Unpack result
assign q_0_0 = packed_result_0[17:0];
assign q_0_1 = packed_result_0[35:18];
assign q_0_2 = packed_result_0[53:36];
assign q_0_3 = packed_result_0[71:54];
assign q_0_4 = packed_result_0[89:72];
assign q_0_5 = packed_result_0[107:90];
assign q_0_6 = packed_result_0[125:108];
assign q_0_7 = packed_result_0[143:126];
assign q_0_8 = packed_result_0[161:144];

endmodule

module weight_buffer_18_9_42_1_2688Woxr_real_half_0 (
	input clk,
	output [17:0] q_0_0,
	output [17:0] q_0_1,
	output [17:0] q_0_2,
	output [17:0] q_0_3,
	output [17:0] q_0_4,
	output [17:0] q_0_5,
	output [17:0] q_0_6,
	output [17:0] q_0_7,
	output [17:0] q_0_8,
	input [11:0] index
);

wire [161:0] packed_result_0;
reg [11:0] addrs_0;
reg [11:0] addrs_base_0;

always @ (posedge clk) begin
	addrs_base_0 <= 0;
	addrs_0 <= index + addrs_base_0;
end

wire rom_we;
assign rom_we = 1'b0;

single_port_ram ram_inst_0 (
	.we(rom_we),
	.addr(addrs_0),
	.data(162'd0),
	.out(packed_result_0),
	.clk(clk)
);

// Unpack result
assign q_0_0 = packed_result_0[17:0];
assign q_0_1 = packed_result_0[35:18];
assign q_0_2 = packed_result_0[53:36];
assign q_0_3 = packed_result_0[71:54];
assign q_0_4 = packed_result_0[89:72];
assign q_0_5 = packed_result_0[107:90];
assign q_0_6 = packed_result_0[125:108];
assign q_0_7 = packed_result_0[143:126];
assign q_0_8 = packed_result_0[161:144];

endmodule

module weight_buffer_18_9_42_1_2688Wcxr_real_half_0 (
	input clk,
	output [17:0] q_0_0,
	output [17:0] q_0_1,
	output [17:0] q_0_2,
	output [17:0] q_0_3,
	output [17:0] q_0_4,
	output [17:0] q_0_5,
	output [17:0] q_0_6,
	output [17:0] q_0_7,
	output [17:0] q_0_8,
	input [11:0] index
);

wire [161:0] packed_result_0;
reg [11:0] addrs_0;
reg [11:0] addrs_base_0;

always @ (posedge clk) begin
	addrs_base_0 <= 0;
	addrs_0 <= index + addrs_base_0;
end

wire rom_we;
assign rom_we = 1'b0;

single_port_ram ram_inst_0 (
	.we(rom_we),
	.addr(addrs_0),
	.data(162'd0),
	.out(packed_result_0),
	.clk(clk)
);

// Unpack result
assign q_0_0 = packed_result_0[17:0];
assign q_0_1 = packed_result_0[35:18];
assign q_0_2 = packed_result_0[53:36];
assign q_0_3 = packed_result_0[71:54];
assign q_0_4 = packed_result_0[89:72];
assign q_0_5 = packed_result_0[107:90];
assign q_0_6 = packed_result_0[125:108];
assign q_0_7 = packed_result_0[143:126];
assign q_0_8 = packed_result_0[161:144];

endmodule

module counter_41_1 (
	input clk,
	input reset,
	input ena,
	output reg [13:0] count
);

always @ (posedge clk) begin 
	if (reset) begin
		count <= 0;
	end else if (ena) begin
		if((count + 1) <= 41) begin
			count <= count + 1;
		end else begin
			count <= 14'd0;
		end
	end
end

endmodule

module weight_buffer_18_9_42_1_2688Wfxr_imag_half_0 (
	input clk,
	output [17:0] q_0_0,
	output [17:0] q_0_1,
	output [17:0] q_0_2,
	output [17:0] q_0_3,
	output [17:0] q_0_4,
	output [17:0] q_0_5,
	output [17:0] q_0_6,
	output [17:0] q_0_7,
	output [17:0] q_0_8,
	input [11:0] index
);

wire [161:0] packed_result_0;
reg [11:0] addrs_0;
reg [11:0] addrs_base_0;

always @ (posedge clk) begin
	addrs_base_0 <= 0;
	addrs_0 <= index + addrs_base_0;
end

wire rom_we;
assign rom_we = 1'b0;

single_port_ram ram_inst_0 (
	.we(rom_we),
	.addr(addrs_0),
	.data(162'd0),
	.out(packed_result_0),
	.clk(clk)
);

// Unpack result
assign q_0_0 = packed_result_0[17:0];
assign q_0_1 = packed_result_0[35:18];
assign q_0_2 = packed_result_0[53:36];
assign q_0_3 = packed_result_0[71:54];
assign q_0_4 = packed_result_0[89:72];
assign q_0_5 = packed_result_0[107:90];
assign q_0_6 = packed_result_0[125:108];
assign q_0_7 = packed_result_0[143:126];
assign q_0_8 = packed_result_0[161:144];

endmodule

module weight_buffer_18_9_42_1_2688Wixr_imag_half_0 (
	input clk,
	output [17:0] q_0_0,
	output [17:0] q_0_1,
	output [17:0] q_0_2,
	output [17:0] q_0_3,
	output [17:0] q_0_4,
	output [17:0] q_0_5,
	output [17:0] q_0_6,
	output [17:0] q_0_7,
	output [17:0] q_0_8,
	input [11:0] index
);

wire [161:0] packed_result_0;
reg [11:0] addrs_0;
reg [11:0] addrs_base_0;

always @ (posedge clk) begin
	addrs_base_0 <= 0;
	addrs_0 <= index + addrs_base_0;
end

wire rom_we;
assign rom_we = 1'b0;

single_port_ram ram_inst_0 (
	.we(rom_we),
	.addr(addrs_0),
	.data(162'd0),
	.out(packed_result_0),
	.clk(clk)
);

// Unpack result
assign q_0_0 = packed_result_0[17:0];
assign q_0_1 = packed_result_0[35:18];
assign q_0_2 = packed_result_0[53:36];
assign q_0_3 = packed_result_0[71:54];
assign q_0_4 = packed_result_0[89:72];
assign q_0_5 = packed_result_0[107:90];
assign q_0_6 = packed_result_0[125:108];
assign q_0_7 = packed_result_0[143:126];
assign q_0_8 = packed_result_0[161:144];

endmodule

module counter_63_1 (
	input clk,
	input reset,
	input ena,
	output reg [13:0] count
);

always @ (posedge clk) begin 
	if (reset) begin
		count <= 0;
	end else if (ena) begin
		if((count + 1) <= 63) begin
			count <= count + 1;
		end else begin
			count <= 14'd0;
		end
	end
end

endmodule

module weight_buffer_18_9_42_1_2688Wfxr_real_half_0 (
	input clk,
	output [17:0] q_0_0,
	output [17:0] q_0_1,
	output [17:0] q_0_2,
	output [17:0] q_0_3,
	output [17:0] q_0_4,
	output [17:0] q_0_5,
	output [17:0] q_0_6,
	output [17:0] q_0_7,
	output [17:0] q_0_8,
	input [11:0] index
);

wire [161:0] packed_result_0;
reg [11:0] addrs_0;
reg [11:0] addrs_base_0;

always @ (posedge clk) begin
	addrs_base_0 <= 0;
	addrs_0 <= index + addrs_base_0;
end

wire rom_we;
assign rom_we = 1'b0;

single_port_ram ram_inst_0 (
	.we(rom_we),
	.addr(addrs_0),
	.data(162'd0),
	.out(packed_result_0),
	.clk(clk)
);

// Unpack result
assign q_0_0 = packed_result_0[17:0];
assign q_0_1 = packed_result_0[35:18];
assign q_0_2 = packed_result_0[53:36];
assign q_0_3 = packed_result_0[71:54];
assign q_0_4 = packed_result_0[89:72];
assign q_0_5 = packed_result_0[107:90];
assign q_0_6 = packed_result_0[125:108];
assign q_0_7 = packed_result_0[143:126];
assign q_0_8 = packed_result_0[161:144];

endmodule

module weight_buffer_18_9_42_1_2688Wcxr_imag_half_0 (
	input clk,
	output [17:0] q_0_0,
	output [17:0] q_0_1,
	output [17:0] q_0_2,
	output [17:0] q_0_3,
	output [17:0] q_0_4,
	output [17:0] q_0_5,
	output [17:0] q_0_6,
	output [17:0] q_0_7,
	output [17:0] q_0_8,
	input [11:0] index
);

wire [161:0] packed_result_0;
reg [11:0] addrs_0;
reg [11:0] addrs_base_0;

always @ (posedge clk) begin
	addrs_base_0 <= 0;
	addrs_0 <= index + addrs_base_0;
end

wire rom_we;
assign rom_we = 1'b0;

single_port_ram ram_inst_0 (
	.we(rom_we),
	.addr(addrs_0),
	.data(162'd0),
	.out(packed_result_0),
	.clk(clk)
);

// Unpack result
assign q_0_0 = packed_result_0[17:0];
assign q_0_1 = packed_result_0[35:18];
assign q_0_2 = packed_result_0[53:36];
assign q_0_3 = packed_result_0[71:54];
assign q_0_4 = packed_result_0[89:72];
assign q_0_5 = packed_result_0[107:90];
assign q_0_6 = packed_result_0[125:108];
assign q_0_7 = packed_result_0[143:126];
assign q_0_8 = packed_result_0[161:144];

endmodule

module stage2_Ct_buffer_18_1_16_64 (
	input clk,
	input reset,
	input wen,
	input ren,
	input [17:0] i_Ct_0,
	input [17:0] i_Ct_1,
	input [17:0] i_Ct_2,
	input [17:0] i_Ct_3,
	input [17:0] i_Ct_4,
	input [17:0] i_Ct_5,
	input [17:0] i_Ct_6,
	input [17:0] i_Ct_7,
	input [17:0] i_Ct_8,
	input [17:0] i_Ct_9,
	input [17:0] i_Ct_10,
	input [17:0] i_Ct_11,
	input [17:0] i_Ct_12,
	input [17:0] i_Ct_13,
	input [17:0] i_Ct_14,
	input [17:0] i_Ct_15,
	output [17:0] o_Ct_0,
	output [17:0] o_Ct_1,
	output [17:0] o_Ct_2,
	output [17:0] o_Ct_3,
	output [17:0] o_Ct_4,
	output [17:0] o_Ct_5,
	output [17:0] o_Ct_6,
	output [17:0] o_Ct_7,
	output [17:0] o_Ct_8,
	output [17:0] o_Ct_9,
	output [17:0] o_Ct_10,
	output [17:0] o_Ct_11,
	output [17:0] o_Ct_12,
	output [17:0] o_Ct_13,
	output [17:0] o_Ct_14,
	output [17:0] o_Ct_15,
	output o_valid
);

wire [287:0] packed_o_Ct_0;
reg [5:0] raddrs_0;
wire [287:0] packed_Ct;

wire [13:0] input_index_counter;
counter_63_1 counter_63_1_inst_in (
	.clk(clk),
	.reset(reset),
	.ena(wen),
	.count(input_index_counter)
);

wire [13:0] output_index_counter;
counter_63_1 counter_63_1_inst_out (
	.clk(clk),
	.reset(reset),
	.ena(ren),
	.count(output_index_counter)
);

always @ (posedge clk) begin
	if ((input_index_counter + 0) < 64)
		raddrs_0 <= input_index_counter + 0;
	else
		raddrs_0 <= 64;
end

assign packed_Ct[17:0] = i_Ct_0;
assign packed_Ct[35:18] = i_Ct_1;
assign packed_Ct[53:36] = i_Ct_2;
assign packed_Ct[71:54] = i_Ct_3;
assign packed_Ct[89:72] = i_Ct_4;
assign packed_Ct[107:90] = i_Ct_5;
assign packed_Ct[125:108] = i_Ct_6;
assign packed_Ct[143:126] = i_Ct_7;
assign packed_Ct[161:144] = i_Ct_8;
assign packed_Ct[179:162] = i_Ct_9;
assign packed_Ct[197:180] = i_Ct_10;
assign packed_Ct[215:198] = i_Ct_11;
assign packed_Ct[233:216] = i_Ct_12;
assign packed_Ct[251:234] = i_Ct_13;
assign packed_Ct[269:252] = i_Ct_14;
assign packed_Ct[287:270] = i_Ct_15;

ram_288_0_64 ram_288_0_64_inst_0 (
	.clk(clk),
	.waddr(input_index_counter),
	.wdata(packed_Ct),
	.wen(wen),
	.raddr(raddrs_0),
	.q(packed_o_Ct_0)
);

assign o_Ct_0 = packed_o_Ct_0[17:0];
assign o_Ct_1 = packed_o_Ct_0[35:18];
assign o_Ct_2 = packed_o_Ct_0[53:36];
assign o_Ct_3 = packed_o_Ct_0[71:54];
assign o_Ct_4 = packed_o_Ct_0[89:72];
assign o_Ct_5 = packed_o_Ct_0[107:90];
assign o_Ct_6 = packed_o_Ct_0[125:108];
assign o_Ct_7 = packed_o_Ct_0[143:126];
assign o_Ct_8 = packed_o_Ct_0[161:144];
assign o_Ct_9 = packed_o_Ct_0[179:162];
assign o_Ct_10 = packed_o_Ct_0[197:180];
assign o_Ct_11 = packed_o_Ct_0[215:198];
assign o_Ct_12 = packed_o_Ct_0[233:216];
assign o_Ct_13 = packed_o_Ct_0[251:234];
assign o_Ct_14 = packed_o_Ct_0[269:252];
assign o_Ct_15 = packed_o_Ct_0[287:270];
endmodule

module ram_288_0_64 (
	input clk,
	input [5:0] waddr,
	input [287:0] wdata,
	input wen,
	input [5:0] raddr,
	output [287:0] q
);

wire [287:0] rd_dummy_signal;
wire [287:0] wr_dummy_signal;
assign rd_dummy_signal = 0;

dual_port_ram u_dual_port_ram(
	.addr1(waddr),
	.we1(wen),
	.data1(wdata),
	.out1(wr_dummy_signal),
	.addr2(raddr),
	.we2(1'b0),
	.data2(rd_dummy_signal),
	.out2(q),
	.clk(clk)
);
endmodule

module stage2_parameter_buffer_18_1_16_64 (
	input clk,
	input reset,
	output [17:0] o_Wic_0,
	output [17:0] o_bi_0,
	output [17:0] o_Wfc_0,
	output [17:0] o_bf_0,
	output [17:0] o_Woc_0,
	output [17:0] o_bo_0,
	output [17:0] o_bc_0,
	output [17:0] o_Wic_1,
	output [17:0] o_bi_1,
	output [17:0] o_Wfc_1,
	output [17:0] o_bf_1,
	output [17:0] o_Woc_1,
	output [17:0] o_bo_1,
	output [17:0] o_bc_1,
	output [17:0] o_Wic_2,
	output [17:0] o_bi_2,
	output [17:0] o_Wfc_2,
	output [17:0] o_bf_2,
	output [17:0] o_Woc_2,
	output [17:0] o_bo_2,
	output [17:0] o_bc_2,
	output [17:0] o_Wic_3,
	output [17:0] o_bi_3,
	output [17:0] o_Wfc_3,
	output [17:0] o_bf_3,
	output [17:0] o_Woc_3,
	output [17:0] o_bo_3,
	output [17:0] o_bc_3,
	output [17:0] o_Wic_4,
	output [17:0] o_bi_4,
	output [17:0] o_Wfc_4,
	output [17:0] o_bf_4,
	output [17:0] o_Woc_4,
	output [17:0] o_bo_4,
	output [17:0] o_bc_4,
	output [17:0] o_Wic_5,
	output [17:0] o_bi_5,
	output [17:0] o_Wfc_5,
	output [17:0] o_bf_5,
	output [17:0] o_Woc_5,
	output [17:0] o_bo_5,
	output [17:0] o_bc_5,
	output [17:0] o_Wic_6,
	output [17:0] o_bi_6,
	output [17:0] o_Wfc_6,
	output [17:0] o_bf_6,
	output [17:0] o_Woc_6,
	output [17:0] o_bo_6,
	output [17:0] o_bc_6,
	output [17:0] o_Wic_7,
	output [17:0] o_bi_7,
	output [17:0] o_Wfc_7,
	output [17:0] o_bf_7,
	output [17:0] o_Woc_7,
	output [17:0] o_bo_7,
	output [17:0] o_bc_7,
	output [17:0] o_Wic_8,
	output [17:0] o_bi_8,
	output [17:0] o_Wfc_8,
	output [17:0] o_bf_8,
	output [17:0] o_Woc_8,
	output [17:0] o_bo_8,
	output [17:0] o_bc_8,
	output [17:0] o_Wic_9,
	output [17:0] o_bi_9,
	output [17:0] o_Wfc_9,
	output [17:0] o_bf_9,
	output [17:0] o_Woc_9,
	output [17:0] o_bo_9,
	output [17:0] o_bc_9,
	output [17:0] o_Wic_10,
	output [17:0] o_bi_10,
	output [17:0] o_Wfc_10,
	output [17:0] o_bf_10,
	output [17:0] o_Woc_10,
	output [17:0] o_bo_10,
	output [17:0] o_bc_10,
	output [17:0] o_Wic_11,
	output [17:0] o_bi_11,
	output [17:0] o_Wfc_11,
	output [17:0] o_bf_11,
	output [17:0] o_Woc_11,
	output [17:0] o_bo_11,
	output [17:0] o_bc_11,
	output [17:0] o_Wic_12,
	output [17:0] o_bi_12,
	output [17:0] o_Wfc_12,
	output [17:0] o_bf_12,
	output [17:0] o_Woc_12,
	output [17:0] o_bo_12,
	output [17:0] o_bc_12,
	output [17:0] o_Wic_13,
	output [17:0] o_bi_13,
	output [17:0] o_Wfc_13,
	output [17:0] o_bf_13,
	output [17:0] o_Woc_13,
	output [17:0] o_bo_13,
	output [17:0] o_bc_13,
	output [17:0] o_Wic_14,
	output [17:0] o_bi_14,
	output [17:0] o_Wfc_14,
	output [17:0] o_bf_14,
	output [17:0] o_Woc_14,
	output [17:0] o_bo_14,
	output [17:0] o_bc_14,
	output [17:0] o_Wic_15,
	output [17:0] o_bi_15,
	output [17:0] o_Wfc_15,
	output [17:0] o_bf_15,
	output [17:0] o_Woc_15,
	output [17:0] o_bo_15,
	output [17:0] o_bc_15,
	input incr_index
);

wire [17:0] Wic_0_0;
wire [17:0] bi_0_0;
wire [17:0] Wfc_0_0;
wire [17:0] bf_0_0;
wire [17:0] Woc_0_0;
wire [17:0] bo_0_0;
wire [17:0] bc_0_0;
wire [17:0] Wic_0_1;
wire [17:0] bi_0_1;
wire [17:0] Wfc_0_1;
wire [17:0] bf_0_1;
wire [17:0] Woc_0_1;
wire [17:0] bo_0_1;
wire [17:0] bc_0_1;
wire [17:0] Wic_0_2;
wire [17:0] bi_0_2;
wire [17:0] Wfc_0_2;
wire [17:0] bf_0_2;
wire [17:0] Woc_0_2;
wire [17:0] bo_0_2;
wire [17:0] bc_0_2;
wire [17:0] Wic_0_3;
wire [17:0] bi_0_3;
wire [17:0] Wfc_0_3;
wire [17:0] bf_0_3;
wire [17:0] Woc_0_3;
wire [17:0] bo_0_3;
wire [17:0] bc_0_3;
wire [17:0] Wic_0_4;
wire [17:0] bi_0_4;
wire [17:0] Wfc_0_4;
wire [17:0] bf_0_4;
wire [17:0] Woc_0_4;
wire [17:0] bo_0_4;
wire [17:0] bc_0_4;
wire [17:0] Wic_0_5;
wire [17:0] bi_0_5;
wire [17:0] Wfc_0_5;
wire [17:0] bf_0_5;
wire [17:0] Woc_0_5;
wire [17:0] bo_0_5;
wire [17:0] bc_0_5;
wire [17:0] Wic_0_6;
wire [17:0] bi_0_6;
wire [17:0] Wfc_0_6;
wire [17:0] bf_0_6;
wire [17:0] Woc_0_6;
wire [17:0] bo_0_6;
wire [17:0] bc_0_6;
wire [17:0] Wic_0_7;
wire [17:0] bi_0_7;
wire [17:0] Wfc_0_7;
wire [17:0] bf_0_7;
wire [17:0] Woc_0_7;
wire [17:0] bo_0_7;
wire [17:0] bc_0_7;
wire [17:0] Wic_0_8;
wire [17:0] bi_0_8;
wire [17:0] Wfc_0_8;
wire [17:0] bf_0_8;
wire [17:0] Woc_0_8;
wire [17:0] bo_0_8;
wire [17:0] bc_0_8;
wire [17:0] Wic_0_9;
wire [17:0] bi_0_9;
wire [17:0] Wfc_0_9;
wire [17:0] bf_0_9;
wire [17:0] Woc_0_9;
wire [17:0] bo_0_9;
wire [17:0] bc_0_9;
wire [17:0] Wic_0_10;
wire [17:0] bi_0_10;
wire [17:0] Wfc_0_10;
wire [17:0] bf_0_10;
wire [17:0] Woc_0_10;
wire [17:0] bo_0_10;
wire [17:0] bc_0_10;
wire [17:0] Wic_0_11;
wire [17:0] bi_0_11;
wire [17:0] Wfc_0_11;
wire [17:0] bf_0_11;
wire [17:0] Woc_0_11;
wire [17:0] bo_0_11;
wire [17:0] bc_0_11;
wire [17:0] Wic_0_12;
wire [17:0] bi_0_12;
wire [17:0] Wfc_0_12;
wire [17:0] bf_0_12;
wire [17:0] Woc_0_12;
wire [17:0] bo_0_12;
wire [17:0] bc_0_12;
wire [17:0] Wic_0_13;
wire [17:0] bi_0_13;
wire [17:0] Wfc_0_13;
wire [17:0] bf_0_13;
wire [17:0] Woc_0_13;
wire [17:0] bo_0_13;
wire [17:0] bc_0_13;
wire [17:0] Wic_0_14;
wire [17:0] bi_0_14;
wire [17:0] Wfc_0_14;
wire [17:0] bf_0_14;
wire [17:0] Woc_0_14;
wire [17:0] bo_0_14;
wire [17:0] bc_0_14;
wire [17:0] Wic_0_15;
wire [17:0] bi_0_15;
wire [17:0] Wfc_0_15;
wire [17:0] bf_0_15;
wire [17:0] Woc_0_15;
wire [17:0] bo_0_15;
wire [17:0] bc_0_15;

wire [13:0] input_index_counter;
counter_63_1 counter_63_1_inst (
	.clk(clk),
	.reset(reset),
	.ena(incr_index),
	.count(input_index_counter)
);

weight_buffer_18_16_1_64_Wic_0 weight_buffer_18_16_1_64_Wic_0_inst (
	.clk(clk),
	.q_0_0(Wic_0_0),
	.q_0_1(Wic_0_1),
	.q_0_2(Wic_0_2),
	.q_0_3(Wic_0_3),
	.q_0_4(Wic_0_4),
	.q_0_5(Wic_0_5),
	.q_0_6(Wic_0_6),
	.q_0_7(Wic_0_7),
	.q_0_8(Wic_0_8),
	.q_0_9(Wic_0_9),
	.q_0_10(Wic_0_10),
	.q_0_11(Wic_0_11),
	.q_0_12(Wic_0_12),
	.q_0_13(Wic_0_13),
	.q_0_14(Wic_0_14),
	.q_0_15(Wic_0_15),
	.index(input_index_counter)
);

weight_buffer_18_16_1_64_bi_0 weight_buffer_18_16_1_64_bi_0_inst (
	.clk(clk),
	.q_0_0(bi_0_0),
	.q_0_1(bi_0_1),
	.q_0_2(bi_0_2),
	.q_0_3(bi_0_3),
	.q_0_4(bi_0_4),
	.q_0_5(bi_0_5),
	.q_0_6(bi_0_6),
	.q_0_7(bi_0_7),
	.q_0_8(bi_0_8),
	.q_0_9(bi_0_9),
	.q_0_10(bi_0_10),
	.q_0_11(bi_0_11),
	.q_0_12(bi_0_12),
	.q_0_13(bi_0_13),
	.q_0_14(bi_0_14),
	.q_0_15(bi_0_15),
	.index(input_index_counter)
);

weight_buffer_18_16_1_64_Wfc_0 weight_buffer_18_16_1_64_Wfc_0_inst (
	.clk(clk),
	.q_0_0(Wfc_0_0),
	.q_0_1(Wfc_0_1),
	.q_0_2(Wfc_0_2),
	.q_0_3(Wfc_0_3),
	.q_0_4(Wfc_0_4),
	.q_0_5(Wfc_0_5),
	.q_0_6(Wfc_0_6),
	.q_0_7(Wfc_0_7),
	.q_0_8(Wfc_0_8),
	.q_0_9(Wfc_0_9),
	.q_0_10(Wfc_0_10),
	.q_0_11(Wfc_0_11),
	.q_0_12(Wfc_0_12),
	.q_0_13(Wfc_0_13),
	.q_0_14(Wfc_0_14),
	.q_0_15(Wfc_0_15),
	.index(input_index_counter)
);

weight_buffer_18_16_1_64_bf_0 weight_buffer_18_16_1_64_bf_0_inst (
	.clk(clk),
	.q_0_0(bf_0_0),
	.q_0_1(bf_0_1),
	.q_0_2(bf_0_2),
	.q_0_3(bf_0_3),
	.q_0_4(bf_0_4),
	.q_0_5(bf_0_5),
	.q_0_6(bf_0_6),
	.q_0_7(bf_0_7),
	.q_0_8(bf_0_8),
	.q_0_9(bf_0_9),
	.q_0_10(bf_0_10),
	.q_0_11(bf_0_11),
	.q_0_12(bf_0_12),
	.q_0_13(bf_0_13),
	.q_0_14(bf_0_14),
	.q_0_15(bf_0_15),
	.index(input_index_counter)
);

weight_buffer_18_16_1_64_Woc_0 weight_buffer_18_16_1_64_Woc_0_inst (
	.clk(clk),
	.q_0_0(Woc_0_0),
	.q_0_1(Woc_0_1),
	.q_0_2(Woc_0_2),
	.q_0_3(Woc_0_3),
	.q_0_4(Woc_0_4),
	.q_0_5(Woc_0_5),
	.q_0_6(Woc_0_6),
	.q_0_7(Woc_0_7),
	.q_0_8(Woc_0_8),
	.q_0_9(Woc_0_9),
	.q_0_10(Woc_0_10),
	.q_0_11(Woc_0_11),
	.q_0_12(Woc_0_12),
	.q_0_13(Woc_0_13),
	.q_0_14(Woc_0_14),
	.q_0_15(Woc_0_15),
	.index(input_index_counter)
);

weight_buffer_18_16_1_64_bo_0 weight_buffer_18_16_1_64_bo_0_inst (
	.clk(clk),
	.q_0_0(bo_0_0),
	.q_0_1(bo_0_1),
	.q_0_2(bo_0_2),
	.q_0_3(bo_0_3),
	.q_0_4(bo_0_4),
	.q_0_5(bo_0_5),
	.q_0_6(bo_0_6),
	.q_0_7(bo_0_7),
	.q_0_8(bo_0_8),
	.q_0_9(bo_0_9),
	.q_0_10(bo_0_10),
	.q_0_11(bo_0_11),
	.q_0_12(bo_0_12),
	.q_0_13(bo_0_13),
	.q_0_14(bo_0_14),
	.q_0_15(bo_0_15),
	.index(input_index_counter)
);

weight_buffer_18_16_1_64_bc_0 weight_buffer_18_16_1_64_bc_0_inst (
	.clk(clk),
	.q_0_0(bc_0_0),
	.q_0_1(bc_0_1),
	.q_0_2(bc_0_2),
	.q_0_3(bc_0_3),
	.q_0_4(bc_0_4),
	.q_0_5(bc_0_5),
	.q_0_6(bc_0_6),
	.q_0_7(bc_0_7),
	.q_0_8(bc_0_8),
	.q_0_9(bc_0_9),
	.q_0_10(bc_0_10),
	.q_0_11(bc_0_11),
	.q_0_12(bc_0_12),
	.q_0_13(bc_0_13),
	.q_0_14(bc_0_14),
	.q_0_15(bc_0_15),
	.index(input_index_counter)
);

assign o_Wic_0 = Wic_0_0;
assign o_bi_0 = bi_0_0;
assign o_Wfc_0 = Wfc_0_0;
assign o_bf_0 = bf_0_0;
assign o_Woc_0 = Woc_0_0;
assign o_bo_0 = bo_0_0;
assign o_bc_0 = bc_0_0;
assign o_Wic_1 = Wic_0_1;
assign o_bi_1 = bi_0_1;
assign o_Wfc_1 = Wfc_0_1;
assign o_bf_1 = bf_0_1;
assign o_Woc_1 = Woc_0_1;
assign o_bo_1 = bo_0_1;
assign o_bc_1 = bc_0_1;
assign o_Wic_2 = Wic_0_2;
assign o_bi_2 = bi_0_2;
assign o_Wfc_2 = Wfc_0_2;
assign o_bf_2 = bf_0_2;
assign o_Woc_2 = Woc_0_2;
assign o_bo_2 = bo_0_2;
assign o_bc_2 = bc_0_2;
assign o_Wic_3 = Wic_0_3;
assign o_bi_3 = bi_0_3;
assign o_Wfc_3 = Wfc_0_3;
assign o_bf_3 = bf_0_3;
assign o_Woc_3 = Woc_0_3;
assign o_bo_3 = bo_0_3;
assign o_bc_3 = bc_0_3;
assign o_Wic_4 = Wic_0_4;
assign o_bi_4 = bi_0_4;
assign o_Wfc_4 = Wfc_0_4;
assign o_bf_4 = bf_0_4;
assign o_Woc_4 = Woc_0_4;
assign o_bo_4 = bo_0_4;
assign o_bc_4 = bc_0_4;
assign o_Wic_5 = Wic_0_5;
assign o_bi_5 = bi_0_5;
assign o_Wfc_5 = Wfc_0_5;
assign o_bf_5 = bf_0_5;
assign o_Woc_5 = Woc_0_5;
assign o_bo_5 = bo_0_5;
assign o_bc_5 = bc_0_5;
assign o_Wic_6 = Wic_0_6;
assign o_bi_6 = bi_0_6;
assign o_Wfc_6 = Wfc_0_6;
assign o_bf_6 = bf_0_6;
assign o_Woc_6 = Woc_0_6;
assign o_bo_6 = bo_0_6;
assign o_bc_6 = bc_0_6;
assign o_Wic_7 = Wic_0_7;
assign o_bi_7 = bi_0_7;
assign o_Wfc_7 = Wfc_0_7;
assign o_bf_7 = bf_0_7;
assign o_Woc_7 = Woc_0_7;
assign o_bo_7 = bo_0_7;
assign o_bc_7 = bc_0_7;
assign o_Wic_8 = Wic_0_8;
assign o_bi_8 = bi_0_8;
assign o_Wfc_8 = Wfc_0_8;
assign o_bf_8 = bf_0_8;
assign o_Woc_8 = Woc_0_8;
assign o_bo_8 = bo_0_8;
assign o_bc_8 = bc_0_8;
assign o_Wic_9 = Wic_0_9;
assign o_bi_9 = bi_0_9;
assign o_Wfc_9 = Wfc_0_9;
assign o_bf_9 = bf_0_9;
assign o_Woc_9 = Woc_0_9;
assign o_bo_9 = bo_0_9;
assign o_bc_9 = bc_0_9;
assign o_Wic_10 = Wic_0_10;
assign o_bi_10 = bi_0_10;
assign o_Wfc_10 = Wfc_0_10;
assign o_bf_10 = bf_0_10;
assign o_Woc_10 = Woc_0_10;
assign o_bo_10 = bo_0_10;
assign o_bc_10 = bc_0_10;
assign o_Wic_11 = Wic_0_11;
assign o_bi_11 = bi_0_11;
assign o_Wfc_11 = Wfc_0_11;
assign o_bf_11 = bf_0_11;
assign o_Woc_11 = Woc_0_11;
assign o_bo_11 = bo_0_11;
assign o_bc_11 = bc_0_11;
assign o_Wic_12 = Wic_0_12;
assign o_bi_12 = bi_0_12;
assign o_Wfc_12 = Wfc_0_12;
assign o_bf_12 = bf_0_12;
assign o_Woc_12 = Woc_0_12;
assign o_bo_12 = bo_0_12;
assign o_bc_12 = bc_0_12;
assign o_Wic_13 = Wic_0_13;
assign o_bi_13 = bi_0_13;
assign o_Wfc_13 = Wfc_0_13;
assign o_bf_13 = bf_0_13;
assign o_Woc_13 = Woc_0_13;
assign o_bo_13 = bo_0_13;
assign o_bc_13 = bc_0_13;
assign o_Wic_14 = Wic_0_14;
assign o_bi_14 = bi_0_14;
assign o_Wfc_14 = Wfc_0_14;
assign o_bf_14 = bf_0_14;
assign o_Woc_14 = Woc_0_14;
assign o_bo_14 = bo_0_14;
assign o_bc_14 = bc_0_14;
assign o_Wic_15 = Wic_0_15;
assign o_bi_15 = bi_0_15;
assign o_Wfc_15 = Wfc_0_15;
assign o_bf_15 = bf_0_15;
assign o_Woc_15 = Woc_0_15;
assign o_bo_15 = bo_0_15;
assign o_bc_15 = bc_0_15;

endmodule

module weight_buffer_18_16_1_64_Wfc_0 (
	input clk,
	output [17:0] q_0_0,
	output [17:0] q_0_1,
	output [17:0] q_0_2,
	output [17:0] q_0_3,
	output [17:0] q_0_4,
	output [17:0] q_0_5,
	output [17:0] q_0_6,
	output [17:0] q_0_7,
	output [17:0] q_0_8,
	output [17:0] q_0_9,
	output [17:0] q_0_10,
	output [17:0] q_0_11,
	output [17:0] q_0_12,
	output [17:0] q_0_13,
	output [17:0] q_0_14,
	output [17:0] q_0_15,
	input [5:0] index
);

wire [287:0] packed_result_0;
reg [5:0] addrs_0;
reg [5:0] addrs_base_0;

always @ (posedge clk) begin
	addrs_base_0 <= 0;
	addrs_0 <= index + addrs_base_0;
end

wire rom_we;
assign rom_we = 1'b0;

single_port_ram ram_inst_0 (
	.we(rom_we),
	.addr(addrs_0),
	.data(288'd0),
	.out(packed_result_0),
	.clk(clk)
);

// Unpack result
assign q_0_0 = packed_result_0[17:0];
assign q_0_1 = packed_result_0[35:18];
assign q_0_2 = packed_result_0[53:36];
assign q_0_3 = packed_result_0[71:54];
assign q_0_4 = packed_result_0[89:72];
assign q_0_5 = packed_result_0[107:90];
assign q_0_6 = packed_result_0[125:108];
assign q_0_7 = packed_result_0[143:126];
assign q_0_8 = packed_result_0[161:144];
assign q_0_9 = packed_result_0[179:162];
assign q_0_10 = packed_result_0[197:180];
assign q_0_11 = packed_result_0[215:198];
assign q_0_12 = packed_result_0[233:216];
assign q_0_13 = packed_result_0[251:234];
assign q_0_14 = packed_result_0[269:252];
assign q_0_15 = packed_result_0[287:270];

endmodule

module weight_buffer_18_16_1_64_bo_0 (
	input clk,
	output [17:0] q_0_0,
	output [17:0] q_0_1,
	output [17:0] q_0_2,
	output [17:0] q_0_3,
	output [17:0] q_0_4,
	output [17:0] q_0_5,
	output [17:0] q_0_6,
	output [17:0] q_0_7,
	output [17:0] q_0_8,
	output [17:0] q_0_9,
	output [17:0] q_0_10,
	output [17:0] q_0_11,
	output [17:0] q_0_12,
	output [17:0] q_0_13,
	output [17:0] q_0_14,
	output [17:0] q_0_15,
	input [5:0] index
);

wire [287:0] packed_result_0;
reg [5:0] addrs_0;
reg [5:0] addrs_base_0;

always @ (posedge clk) begin
	addrs_base_0 <= 0;
	addrs_0 <= index + addrs_base_0;
end

wire rom_we;
assign rom_we = 1'b0;

single_port_ram ram_inst_0 (
	.we(rom_we),
	.addr(addrs_0),
	.data(288'd0),
	.out(packed_result_0),
	.clk(clk)
);

// Unpack result
assign q_0_0 = packed_result_0[17:0];
assign q_0_1 = packed_result_0[35:18];
assign q_0_2 = packed_result_0[53:36];
assign q_0_3 = packed_result_0[71:54];
assign q_0_4 = packed_result_0[89:72];
assign q_0_5 = packed_result_0[107:90];
assign q_0_6 = packed_result_0[125:108];
assign q_0_7 = packed_result_0[143:126];
assign q_0_8 = packed_result_0[161:144];
assign q_0_9 = packed_result_0[179:162];
assign q_0_10 = packed_result_0[197:180];
assign q_0_11 = packed_result_0[215:198];
assign q_0_12 = packed_result_0[233:216];
assign q_0_13 = packed_result_0[251:234];
assign q_0_14 = packed_result_0[269:252];
assign q_0_15 = packed_result_0[287:270];

endmodule

module weight_buffer_18_16_1_64_bc_0 (
	input clk,
	output [17:0] q_0_0,
	output [17:0] q_0_1,
	output [17:0] q_0_2,
	output [17:0] q_0_3,
	output [17:0] q_0_4,
	output [17:0] q_0_5,
	output [17:0] q_0_6,
	output [17:0] q_0_7,
	output [17:0] q_0_8,
	output [17:0] q_0_9,
	output [17:0] q_0_10,
	output [17:0] q_0_11,
	output [17:0] q_0_12,
	output [17:0] q_0_13,
	output [17:0] q_0_14,
	output [17:0] q_0_15,
	input [5:0] index
);

wire [287:0] packed_result_0;
reg [5:0] addrs_0;
reg [5:0] addrs_base_0;

always @ (posedge clk) begin
	addrs_base_0 <= 0;
	addrs_0 <= index + addrs_base_0;
end

wire rom_we;
assign rom_we = 1'b0;

single_port_ram ram_inst_0 (
	.we(rom_we),
	.addr(addrs_0),
	.data(288'd0),
	.out(packed_result_0),
	.clk(clk)
);

// Unpack result
assign q_0_0 = packed_result_0[17:0];
assign q_0_1 = packed_result_0[35:18];
assign q_0_2 = packed_result_0[53:36];
assign q_0_3 = packed_result_0[71:54];
assign q_0_4 = packed_result_0[89:72];
assign q_0_5 = packed_result_0[107:90];
assign q_0_6 = packed_result_0[125:108];
assign q_0_7 = packed_result_0[143:126];
assign q_0_8 = packed_result_0[161:144];
assign q_0_9 = packed_result_0[179:162];
assign q_0_10 = packed_result_0[197:180];
assign q_0_11 = packed_result_0[215:198];
assign q_0_12 = packed_result_0[233:216];
assign q_0_13 = packed_result_0[251:234];
assign q_0_14 = packed_result_0[269:252];
assign q_0_15 = packed_result_0[287:270];

endmodule

module weight_buffer_18_16_1_64_Woc_0 (
	input clk,
	output [17:0] q_0_0,
	output [17:0] q_0_1,
	output [17:0] q_0_2,
	output [17:0] q_0_3,
	output [17:0] q_0_4,
	output [17:0] q_0_5,
	output [17:0] q_0_6,
	output [17:0] q_0_7,
	output [17:0] q_0_8,
	output [17:0] q_0_9,
	output [17:0] q_0_10,
	output [17:0] q_0_11,
	output [17:0] q_0_12,
	output [17:0] q_0_13,
	output [17:0] q_0_14,
	output [17:0] q_0_15,
	input [5:0] index
);

wire [287:0] packed_result_0;
reg [5:0] addrs_0;
reg [5:0] addrs_base_0;

always @ (posedge clk) begin
	addrs_base_0 <= 0;
	addrs_0 <= index + addrs_base_0;
end

wire rom_we;
assign rom_we = 1'b0;

single_port_ram ram_inst_0 (
	.we(rom_we),
	.addr(addrs_0),
	.data(288'd0),
	.out(packed_result_0),
	.clk(clk)
);

// Unpack result
assign q_0_0 = packed_result_0[17:0];
assign q_0_1 = packed_result_0[35:18];
assign q_0_2 = packed_result_0[53:36];
assign q_0_3 = packed_result_0[71:54];
assign q_0_4 = packed_result_0[89:72];
assign q_0_5 = packed_result_0[107:90];
assign q_0_6 = packed_result_0[125:108];
assign q_0_7 = packed_result_0[143:126];
assign q_0_8 = packed_result_0[161:144];
assign q_0_9 = packed_result_0[179:162];
assign q_0_10 = packed_result_0[197:180];
assign q_0_11 = packed_result_0[215:198];
assign q_0_12 = packed_result_0[233:216];
assign q_0_13 = packed_result_0[251:234];
assign q_0_14 = packed_result_0[269:252];
assign q_0_15 = packed_result_0[287:270];

endmodule

module weight_buffer_18_16_1_64_bi_0 (
	input clk,
	output [17:0] q_0_0,
	output [17:0] q_0_1,
	output [17:0] q_0_2,
	output [17:0] q_0_3,
	output [17:0] q_0_4,
	output [17:0] q_0_5,
	output [17:0] q_0_6,
	output [17:0] q_0_7,
	output [17:0] q_0_8,
	output [17:0] q_0_9,
	output [17:0] q_0_10,
	output [17:0] q_0_11,
	output [17:0] q_0_12,
	output [17:0] q_0_13,
	output [17:0] q_0_14,
	output [17:0] q_0_15,
	input [5:0] index
);

wire [287:0] packed_result_0;
reg [5:0] addrs_0;
reg [5:0] addrs_base_0;

always @ (posedge clk) begin
	addrs_base_0 <= 0;
	addrs_0 <= index + addrs_base_0;
end

wire rom_we;
assign rom_we = 1'b0;

single_port_ram ram_inst_0 (
	.we(rom_we),
	.addr(addrs_0),
	.data(288'd0),
	.out(packed_result_0),
	.clk(clk)
);

// Unpack result
assign q_0_0 = packed_result_0[17:0];
assign q_0_1 = packed_result_0[35:18];
assign q_0_2 = packed_result_0[53:36];
assign q_0_3 = packed_result_0[71:54];
assign q_0_4 = packed_result_0[89:72];
assign q_0_5 = packed_result_0[107:90];
assign q_0_6 = packed_result_0[125:108];
assign q_0_7 = packed_result_0[143:126];
assign q_0_8 = packed_result_0[161:144];
assign q_0_9 = packed_result_0[179:162];
assign q_0_10 = packed_result_0[197:180];
assign q_0_11 = packed_result_0[215:198];
assign q_0_12 = packed_result_0[233:216];
assign q_0_13 = packed_result_0[251:234];
assign q_0_14 = packed_result_0[269:252];
assign q_0_15 = packed_result_0[287:270];

endmodule

module weight_buffer_18_16_1_64_bf_0 (
	input clk,
	output [17:0] q_0_0,
	output [17:0] q_0_1,
	output [17:0] q_0_2,
	output [17:0] q_0_3,
	output [17:0] q_0_4,
	output [17:0] q_0_5,
	output [17:0] q_0_6,
	output [17:0] q_0_7,
	output [17:0] q_0_8,
	output [17:0] q_0_9,
	output [17:0] q_0_10,
	output [17:0] q_0_11,
	output [17:0] q_0_12,
	output [17:0] q_0_13,
	output [17:0] q_0_14,
	output [17:0] q_0_15,
	input [5:0] index
);

wire [287:0] packed_result_0;
reg [5:0] addrs_0;
reg [5:0] addrs_base_0;

always @ (posedge clk) begin
	addrs_base_0 <= 0;
	addrs_0 <= index + addrs_base_0;
end

wire rom_we;
assign rom_we = 1'b0;

single_port_ram ram_inst_0 (
	.we(rom_we),
	.addr(addrs_0),
	.data(288'd0),
	.out(packed_result_0),
	.clk(clk)
);

// Unpack result
assign q_0_0 = packed_result_0[17:0];
assign q_0_1 = packed_result_0[35:18];
assign q_0_2 = packed_result_0[53:36];
assign q_0_3 = packed_result_0[71:54];
assign q_0_4 = packed_result_0[89:72];
assign q_0_5 = packed_result_0[107:90];
assign q_0_6 = packed_result_0[125:108];
assign q_0_7 = packed_result_0[143:126];
assign q_0_8 = packed_result_0[161:144];
assign q_0_9 = packed_result_0[179:162];
assign q_0_10 = packed_result_0[197:180];
assign q_0_11 = packed_result_0[215:198];
assign q_0_12 = packed_result_0[233:216];
assign q_0_13 = packed_result_0[251:234];
assign q_0_14 = packed_result_0[269:252];
assign q_0_15 = packed_result_0[287:270];

endmodule

module weight_buffer_18_16_1_64_Wic_0 (
	input clk,
	output [17:0] q_0_0,
	output [17:0] q_0_1,
	output [17:0] q_0_2,
	output [17:0] q_0_3,
	output [17:0] q_0_4,
	output [17:0] q_0_5,
	output [17:0] q_0_6,
	output [17:0] q_0_7,
	output [17:0] q_0_8,
	output [17:0] q_0_9,
	output [17:0] q_0_10,
	output [17:0] q_0_11,
	output [17:0] q_0_12,
	output [17:0] q_0_13,
	output [17:0] q_0_14,
	output [17:0] q_0_15,
	input [5:0] index
);

wire [287:0] packed_result_0;
reg [5:0] addrs_0;
reg [5:0] addrs_base_0;

always @ (posedge clk) begin
	addrs_base_0 <= 0;
	addrs_0 <= index + addrs_base_0;
end

wire rom_we;
assign rom_we = 1'b0;

single_port_ram ram_inst_0 (
	.we(rom_we),
	.addr(addrs_0),
	.data(288'd0),
	.out(packed_result_0),
	.clk(clk)
);

// Unpack result
assign q_0_0 = packed_result_0[17:0];
assign q_0_1 = packed_result_0[35:18];
assign q_0_2 = packed_result_0[53:36];
assign q_0_3 = packed_result_0[71:54];
assign q_0_4 = packed_result_0[89:72];
assign q_0_5 = packed_result_0[107:90];
assign q_0_6 = packed_result_0[125:108];
assign q_0_7 = packed_result_0[143:126];
assign q_0_8 = packed_result_0[161:144];
assign q_0_9 = packed_result_0[179:162];
assign q_0_10 = packed_result_0[197:180];
assign q_0_11 = packed_result_0[215:198];
assign q_0_12 = packed_result_0[233:216];
assign q_0_13 = packed_result_0[251:234];
assign q_0_14 = packed_result_0[269:252];
assign q_0_15 = packed_result_0[287:270];

endmodule

module C_LSTM_stage_3_18_10_64_2048_1_16_1 (
	input clk,
	input reset,
	input i_ready,
	input i_valid,
	input [17:0] i_mt_0,
	input [17:0] i_mt_1,
	input [17:0] i_mt_2,
	input [17:0] i_mt_3,
	input [17:0] i_mt_4,
	input [17:0] i_mt_5,
	input [17:0] i_mt_6,
	input [17:0] i_mt_7,
	input [17:0] i_mt_8,
	input [17:0] i_mt_9,
	input [17:0] i_mt_10,
	input [17:0] i_mt_11,
	input [17:0] i_mt_12,
	input [17:0] i_mt_13,
	input [17:0] i_mt_14,
	input [17:0] i_mt_15,
	output [17:0] o_Yt_0_0,
	output [17:0] o_Yt_0_1,
	output [17:0] o_Yt_0_2,
	output [17:0] o_Yt_0_3,
	output [17:0] o_Yt_0_4,
	output [17:0] o_Yt_0_5,
	output [17:0] o_Yt_0_6,
	output [17:0] o_Yt_0_7,
	output [17:0] o_Yt_0_8,
	output [17:0] o_Yt_0_9,
	output [17:0] o_Yt_0_10,
	output [17:0] o_Yt_0_11,
	output [17:0] o_Yt_0_12,
	output [17:0] o_Yt_0_13,
	output [17:0] o_Yt_0_14,
	output [17:0] o_Yt_0_15,
	output o_valid,
	output o_ready
);

wire enable;
assign enable = i_ready;
wire [17:0] mt_hold_0;
wire [17:0] mt_hold_1;
wire [17:0] mt_hold_2;
wire [17:0] mt_hold_3;
wire [17:0] mt_hold_4;
wire [17:0] mt_hold_5;
wire [17:0] mt_hold_6;
wire [17:0] mt_hold_7;
wire [17:0] mt_hold_8;
wire [17:0] mt_hold_9;
wire [17:0] mt_hold_10;
wire [17:0] mt_hold_11;
wire [17:0] mt_hold_12;
wire [17:0] mt_hold_13;
wire [17:0] mt_hold_14;
wire [17:0] mt_hold_15;
wire [17:0] Wym_real_0_0;
wire [17:0] Wym_imag_0_0;
wire [17:0] Wym_real_0_1;
wire [17:0] Wym_imag_0_1;
wire [17:0] Wym_real_0_2;
wire [17:0] Wym_imag_0_2;
wire [17:0] Wym_real_0_3;
wire [17:0] Wym_imag_0_3;
wire [17:0] Wym_real_0_4;
wire [17:0] Wym_imag_0_4;
wire [17:0] Wym_real_0_5;
wire [17:0] Wym_imag_0_5;
wire [17:0] Wym_real_0_6;
wire [17:0] Wym_imag_0_6;
wire [17:0] Wym_real_0_7;
wire [17:0] Wym_imag_0_7;
wire [17:0] Wym_real_0_8;
wire [17:0] Wym_imag_0_8;
wire reg_i_valid;
reg reg_i_ready;

shift_register_group_18_16_3 shift_register_group_18_16_3_mt_holder (
	.clk(clk),
	.enable(enable),
	.in_0(i_mt_0),
	.out_0(mt_hold_0),
	.in_1(i_mt_1),
	.out_1(mt_hold_1),
	.in_2(i_mt_2),
	.out_2(mt_hold_2),
	.in_3(i_mt_3),
	.out_3(mt_hold_3),
	.in_4(i_mt_4),
	.out_4(mt_hold_4),
	.in_5(i_mt_5),
	.out_5(mt_hold_5),
	.in_6(i_mt_6),
	.out_6(mt_hold_6),
	.in_7(i_mt_7),
	.out_7(mt_hold_7),
	.in_8(i_mt_8),
	.out_8(mt_hold_8),
	.in_9(i_mt_9),
	.out_9(mt_hold_9),
	.in_10(i_mt_10),
	.out_10(mt_hold_10),
	.in_11(i_mt_11),
	.out_11(mt_hold_11),
	.in_12(i_mt_12),
	.out_12(mt_hold_12),
	.in_13(i_mt_13),
	.out_13(mt_hold_13),
	.in_14(i_mt_14),
	.out_14(mt_hold_14),
	.in_15(i_mt_15),
	.out_15(mt_hold_15),
	.reset(reset)
);

shift_register_unit_18_3 shift_register_unit_18_3_valid_holder (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(i_valid),
	.out(reg_i_valid)
);

stage3_parameter_buffer_18_1_16_64_2048 stage3_parameter_buffer_18_1_16_64_2048_inst (
	.clk(clk),
	.reset(reset),
	.Wym_real_0_0(Wym_real_0_0),
	.Wym_imag_0_0(Wym_imag_0_0),
	.Wym_real_0_1(Wym_real_0_1),
	.Wym_imag_0_1(Wym_imag_0_1),
	.Wym_real_0_2(Wym_real_0_2),
	.Wym_imag_0_2(Wym_imag_0_2),
	.Wym_real_0_3(Wym_real_0_3),
	.Wym_imag_0_3(Wym_imag_0_3),
	.Wym_real_0_4(Wym_real_0_4),
	.Wym_imag_0_4(Wym_imag_0_4),
	.Wym_real_0_5(Wym_real_0_5),
	.Wym_imag_0_5(Wym_imag_0_5),
	.Wym_real_0_6(Wym_real_0_6),
	.Wym_imag_0_6(Wym_imag_0_6),
	.Wym_real_0_7(Wym_real_0_7),
	.Wym_imag_0_7(Wym_imag_0_7),
	.Wym_real_0_8(Wym_real_0_8),
	.Wym_imag_0_8(Wym_imag_0_8),
	.incr_index(i_valid)
);

multiple_c_matrix_vec_mult_and_sum_18_10_16_1_1_64 multiple_c_matrix_vec_mult_and_sum_18_10_16_1_1_64_inst (
	.clk(clk),
	.reset(reset),
	.i_ready(reg_i_ready),
	.i_valid(reg_i_valid),
	.i_X_0(mt_hold_0),
	.i_X_1(mt_hold_1),
	.i_X_2(mt_hold_2),
	.i_X_3(mt_hold_3),
	.i_X_4(mt_hold_4),
	.i_X_5(mt_hold_5),
	.i_X_6(mt_hold_6),
	.i_X_7(mt_hold_7),
	.i_X_8(mt_hold_8),
	.i_X_9(mt_hold_9),
	.i_X_10(mt_hold_10),
	.i_X_11(mt_hold_11),
	.i_X_12(mt_hold_12),
	.i_X_13(mt_hold_13),
	.i_X_14(mt_hold_14),
	.i_X_15(mt_hold_15),
	.i_W_real_0_0(Wym_real_0_0),
	.i_W_imag_0_0(Wym_imag_0_0),
	.i_W_real_0_1(Wym_real_0_1),
	.i_W_imag_0_1(Wym_imag_0_1),
	.i_W_real_0_2(Wym_real_0_2),
	.i_W_imag_0_2(Wym_imag_0_2),
	.i_W_real_0_3(Wym_real_0_3),
	.i_W_imag_0_3(Wym_imag_0_3),
	.i_W_real_0_4(Wym_real_0_4),
	.i_W_imag_0_4(Wym_imag_0_4),
	.i_W_real_0_5(Wym_real_0_5),
	.i_W_imag_0_5(Wym_imag_0_5),
	.i_W_real_0_6(Wym_real_0_6),
	.i_W_imag_0_6(Wym_imag_0_6),
	.i_W_real_0_7(Wym_real_0_7),
	.i_W_imag_0_7(Wym_imag_0_7),
	.i_W_real_0_8(Wym_real_0_8),
	.i_W_imag_0_8(Wym_imag_0_8),
	.o_Y_0_0(o_Yt_0_0),
	.o_Y_0_1(o_Yt_0_1),
	.o_Y_0_2(o_Yt_0_2),
	.o_Y_0_3(o_Yt_0_3),
	.o_Y_0_4(o_Yt_0_4),
	.o_Y_0_5(o_Yt_0_5),
	.o_Y_0_6(o_Yt_0_6),
	.o_Y_0_7(o_Yt_0_7),
	.o_Y_0_8(o_Yt_0_8),
	.o_Y_0_9(o_Yt_0_9),
	.o_Y_0_10(o_Yt_0_10),
	.o_Y_0_11(o_Yt_0_11),
	.o_Y_0_12(o_Yt_0_12),
	.o_Y_0_13(o_Yt_0_13),
	.o_Y_0_14(o_Yt_0_14),
	.o_Y_0_15(o_Yt_0_15),
	.o_valid(o_valid),
	.o_ready(o_ready)
);

always @ (posedge clk) begin
	if (reset) begin
		reg_i_ready <= 1'b0;
	end else begin
		reg_i_ready <= i_ready;
	end
end

endmodule

module multiple_c_matrix_vec_mult_and_sum_18_10_16_1_1_64 (
	input clk,
	input reset,
	input i_ready,
	input i_valid,
	input [17:0] i_X_0,
	input [17:0] i_X_1,
	input [17:0] i_X_2,
	input [17:0] i_X_3,
	input [17:0] i_X_4,
	input [17:0] i_X_5,
	input [17:0] i_X_6,
	input [17:0] i_X_7,
	input [17:0] i_X_8,
	input [17:0] i_X_9,
	input [17:0] i_X_10,
	input [17:0] i_X_11,
	input [17:0] i_X_12,
	input [17:0] i_X_13,
	input [17:0] i_X_14,
	input [17:0] i_X_15,
	input [17:0] i_W_real_0_0,
	input [17:0] i_W_imag_0_0,
	input [17:0] i_W_real_0_1,
	input [17:0] i_W_imag_0_1,
	input [17:0] i_W_real_0_2,
	input [17:0] i_W_imag_0_2,
	input [17:0] i_W_real_0_3,
	input [17:0] i_W_imag_0_3,
	input [17:0] i_W_real_0_4,
	input [17:0] i_W_imag_0_4,
	input [17:0] i_W_real_0_5,
	input [17:0] i_W_imag_0_5,
	input [17:0] i_W_real_0_6,
	input [17:0] i_W_imag_0_6,
	input [17:0] i_W_real_0_7,
	input [17:0] i_W_imag_0_7,
	input [17:0] i_W_real_0_8,
	input [17:0] i_W_imag_0_8,
	output [17:0] o_Y_0_0,
	output [17:0] o_Y_0_1,
	output [17:0] o_Y_0_2,
	output [17:0] o_Y_0_3,
	output [17:0] o_Y_0_4,
	output [17:0] o_Y_0_5,
	output [17:0] o_Y_0_6,
	output [17:0] o_Y_0_7,
	output [17:0] o_Y_0_8,
	output [17:0] o_Y_0_9,
	output [17:0] o_Y_0_10,
	output [17:0] o_Y_0_11,
	output [17:0] o_Y_0_12,
	output [17:0] o_Y_0_13,
	output [17:0] o_Y_0_14,
	output [17:0] o_Y_0_15,
	output o_valid,
	output o_ready
);

wire matrix_vec_mult_ready, matrix_vec_mult_valid;
wire accum_valid_0;
wire idft_next_out_0;
reg idft_out_valid;
wire [17:0] Y_imag_0_0;
wire [17:0] Y_real_0_0;
wire [17:0] sum_Y_real_0_0;
wire [17:0] sum_Y_imag_0_0;
wire [17:0] sum_Y_real_hold_0_0;
wire [17:0] sum_Y_imag_hold_0_0;
wire [17:0] out_Y_idft_0_0;
reg [17:0] reg_Y_0_0;
wire [17:0] Y_imag_0_1;
wire [17:0] Y_real_0_1;
wire [17:0] sum_Y_real_0_1;
wire [17:0] sum_Y_imag_0_1;
wire [17:0] sum_Y_real_hold_0_1;
wire [17:0] sum_Y_imag_hold_0_1;
wire [17:0] out_Y_idft_0_1;
reg [17:0] reg_Y_0_1;
wire [17:0] Y_imag_0_2;
wire [17:0] Y_real_0_2;
wire [17:0] sum_Y_real_0_2;
wire [17:0] sum_Y_imag_0_2;
wire [17:0] sum_Y_real_hold_0_2;
wire [17:0] sum_Y_imag_hold_0_2;
wire [17:0] out_Y_idft_0_2;
reg [17:0] reg_Y_0_2;
wire [17:0] Y_imag_0_3;
wire [17:0] Y_real_0_3;
wire [17:0] sum_Y_real_0_3;
wire [17:0] sum_Y_imag_0_3;
wire [17:0] sum_Y_real_hold_0_3;
wire [17:0] sum_Y_imag_hold_0_3;
wire [17:0] out_Y_idft_0_3;
reg [17:0] reg_Y_0_3;
wire [17:0] Y_imag_0_4;
wire [17:0] Y_real_0_4;
wire [17:0] sum_Y_real_0_4;
wire [17:0] sum_Y_imag_0_4;
wire [17:0] sum_Y_real_hold_0_4;
wire [17:0] sum_Y_imag_hold_0_4;
wire [17:0] out_Y_idft_0_4;
reg [17:0] reg_Y_0_4;
wire [17:0] Y_imag_0_5;
wire [17:0] Y_real_0_5;
wire [17:0] sum_Y_real_0_5;
wire [17:0] sum_Y_imag_0_5;
wire [17:0] sum_Y_real_hold_0_5;
wire [17:0] sum_Y_imag_hold_0_5;
wire [17:0] out_Y_idft_0_5;
reg [17:0] reg_Y_0_5;
wire [17:0] Y_imag_0_6;
wire [17:0] Y_real_0_6;
wire [17:0] sum_Y_real_0_6;
wire [17:0] sum_Y_imag_0_6;
wire [17:0] sum_Y_real_hold_0_6;
wire [17:0] sum_Y_imag_hold_0_6;
wire [17:0] out_Y_idft_0_6;
reg [17:0] reg_Y_0_6;
wire [17:0] Y_imag_0_7;
wire [17:0] Y_real_0_7;
wire [17:0] sum_Y_real_0_7;
wire [17:0] sum_Y_imag_0_7;
wire [17:0] sum_Y_real_hold_0_7;
wire [17:0] sum_Y_imag_hold_0_7;
wire [17:0] out_Y_idft_0_7;
reg [17:0] reg_Y_0_7;
wire [17:0] Y_imag_0_8;
wire [17:0] Y_real_0_8;
wire [17:0] sum_Y_real_0_8;
wire [17:0] sum_Y_imag_0_8;
wire [17:0] sum_Y_real_hold_0_8;
wire [17:0] sum_Y_imag_hold_0_8;
wire [17:0] out_Y_idft_0_8;
reg [17:0] reg_Y_0_8;
wire [17:0] Y_imag_0_9;
wire [17:0] Y_real_0_9;
wire [17:0] sum_Y_real_0_9;
wire [17:0] sum_Y_imag_0_9;
wire [17:0] sum_Y_real_hold_0_9;
wire [17:0] sum_Y_imag_hold_0_9;
wire [17:0] out_Y_idft_0_9;
reg [17:0] reg_Y_0_9;
wire [17:0] Y_imag_0_10;
wire [17:0] Y_real_0_10;
wire [17:0] sum_Y_real_0_10;
wire [17:0] sum_Y_imag_0_10;
wire [17:0] sum_Y_real_hold_0_10;
wire [17:0] sum_Y_imag_hold_0_10;
wire [17:0] out_Y_idft_0_10;
reg [17:0] reg_Y_0_10;
wire [17:0] Y_imag_0_11;
wire [17:0] Y_real_0_11;
wire [17:0] sum_Y_real_0_11;
wire [17:0] sum_Y_imag_0_11;
wire [17:0] sum_Y_real_hold_0_11;
wire [17:0] sum_Y_imag_hold_0_11;
wire [17:0] out_Y_idft_0_11;
reg [17:0] reg_Y_0_11;
wire [17:0] Y_imag_0_12;
wire [17:0] Y_real_0_12;
wire [17:0] sum_Y_real_0_12;
wire [17:0] sum_Y_imag_0_12;
wire [17:0] sum_Y_real_hold_0_12;
wire [17:0] sum_Y_imag_hold_0_12;
wire [17:0] out_Y_idft_0_12;
reg [17:0] reg_Y_0_12;
wire [17:0] Y_imag_0_13;
wire [17:0] Y_real_0_13;
wire [17:0] sum_Y_real_0_13;
wire [17:0] sum_Y_imag_0_13;
wire [17:0] sum_Y_real_hold_0_13;
wire [17:0] sum_Y_imag_hold_0_13;
wire [17:0] out_Y_idft_0_13;
reg [17:0] reg_Y_0_13;
wire [17:0] Y_imag_0_14;
wire [17:0] Y_real_0_14;
wire [17:0] sum_Y_real_0_14;
wire [17:0] sum_Y_imag_0_14;
wire [17:0] sum_Y_real_hold_0_14;
wire [17:0] sum_Y_imag_hold_0_14;
wire [17:0] out_Y_idft_0_14;
reg [17:0] reg_Y_0_14;
wire [17:0] Y_imag_0_15;
wire [17:0] Y_real_0_15;
wire [17:0] sum_Y_real_0_15;
wire [17:0] sum_Y_imag_0_15;
wire [17:0] sum_Y_real_hold_0_15;
wire [17:0] sum_Y_imag_hold_0_15;
wire [17:0] out_Y_idft_0_15;
reg [17:0] reg_Y_0_15;
reg reg_o_valid;

// Enable whenever the reciever is ready
wire enable;
assign enable = i_ready;
c_matrix_vec_mult_core_18_10_16_1_1 c_matrix_vec_mult_core_18_10_16_1_1_inst (
	.clk(clk),
	.reset(reset),
	.i_ready(i_ready),
	.i_valid(i_valid),
	.i_X_0(i_X_0),
	.i_X_1(i_X_1),
	.i_X_2(i_X_2),
	.i_X_3(i_X_3),
	.i_X_4(i_X_4),
	.i_X_5(i_X_5),
	.i_X_6(i_X_6),
	.i_X_7(i_X_7),
	.i_X_8(i_X_8),
	.i_X_9(i_X_9),
	.i_X_10(i_X_10),
	.i_X_11(i_X_11),
	.i_X_12(i_X_12),
	.i_X_13(i_X_13),
	.i_X_14(i_X_14),
	.i_X_15(i_X_15),
	.i_W_real_0_0(i_W_real_0_0),
	.i_W_imag_0_0(i_W_imag_0_0),
	.i_W_real_0_1(i_W_real_0_1),
	.i_W_imag_0_1(i_W_imag_0_1),
	.i_W_real_0_2(i_W_real_0_2),
	.i_W_imag_0_2(i_W_imag_0_2),
	.i_W_real_0_3(i_W_real_0_3),
	.i_W_imag_0_3(i_W_imag_0_3),
	.i_W_real_0_4(i_W_real_0_4),
	.i_W_imag_0_4(i_W_imag_0_4),
	.i_W_real_0_5(i_W_real_0_5),
	.i_W_imag_0_5(i_W_imag_0_5),
	.i_W_real_0_6(i_W_real_0_6),
	.i_W_imag_0_6(i_W_imag_0_6),
	.i_W_real_0_7(i_W_real_0_7),
	.i_W_imag_0_7(i_W_imag_0_7),
	.i_W_real_0_8(i_W_real_0_8),
	.i_W_imag_0_8(i_W_imag_0_8),
	.o_Y_real_0_0(Y_real_0_0),
	.o_Y_imag_0_0(Y_imag_0_0),
	.o_Y_real_0_1(Y_real_0_1),
	.o_Y_imag_0_1(Y_imag_0_1),
	.o_Y_real_0_2(Y_real_0_2),
	.o_Y_imag_0_2(Y_imag_0_2),
	.o_Y_real_0_3(Y_real_0_3),
	.o_Y_imag_0_3(Y_imag_0_3),
	.o_Y_real_0_4(Y_real_0_4),
	.o_Y_imag_0_4(Y_imag_0_4),
	.o_Y_real_0_5(Y_real_0_5),
	.o_Y_imag_0_5(Y_imag_0_5),
	.o_Y_real_0_6(Y_real_0_6),
	.o_Y_imag_0_6(Y_imag_0_6),
	.o_Y_real_0_7(Y_real_0_7),
	.o_Y_imag_0_7(Y_imag_0_7),
	.o_Y_real_0_8(Y_real_0_8),
	.o_Y_imag_0_8(Y_imag_0_8),
	.o_Y_real_0_9(Y_real_0_9),
	.o_Y_imag_0_9(Y_imag_0_9),
	.o_Y_real_0_10(Y_real_0_10),
	.o_Y_imag_0_10(Y_imag_0_10),
	.o_Y_real_0_11(Y_real_0_11),
	.o_Y_imag_0_11(Y_imag_0_11),
	.o_Y_real_0_12(Y_real_0_12),
	.o_Y_imag_0_12(Y_imag_0_12),
	.o_Y_real_0_13(Y_real_0_13),
	.o_Y_imag_0_13(Y_imag_0_13),
	.o_Y_real_0_14(Y_real_0_14),
	.o_Y_imag_0_14(Y_imag_0_14),
	.o_Y_real_0_15(Y_real_0_15),
	.o_Y_imag_0_15(Y_imag_0_15),
	.o_ready(matrix_vec_mult_ready),
	.o_valid(matrix_vec_mult_valid)
);

sum_complex_vector_unit_18_18_16_64 sum_complex_vector_unit_18_18_16_64_inst_0 (
	.clk(clk),
	.clr(reset),
	.enable(enable),
	.i_valid(matrix_vec_mult_valid),
	.i_real_0(Y_real_0_0),
	.i_imag_0(Y_imag_0_0),
	.o_real_0(sum_Y_real_0_0),
	.o_imag_0(sum_Y_imag_0_0),
	.i_real_1(Y_real_0_1),
	.i_imag_1(Y_imag_0_1),
	.o_real_1(sum_Y_real_0_1),
	.o_imag_1(sum_Y_imag_0_1),
	.i_real_2(Y_real_0_2),
	.i_imag_2(Y_imag_0_2),
	.o_real_2(sum_Y_real_0_2),
	.o_imag_2(sum_Y_imag_0_2),
	.i_real_3(Y_real_0_3),
	.i_imag_3(Y_imag_0_3),
	.o_real_3(sum_Y_real_0_3),
	.o_imag_3(sum_Y_imag_0_3),
	.i_real_4(Y_real_0_4),
	.i_imag_4(Y_imag_0_4),
	.o_real_4(sum_Y_real_0_4),
	.o_imag_4(sum_Y_imag_0_4),
	.i_real_5(Y_real_0_5),
	.i_imag_5(Y_imag_0_5),
	.o_real_5(sum_Y_real_0_5),
	.o_imag_5(sum_Y_imag_0_5),
	.i_real_6(Y_real_0_6),
	.i_imag_6(Y_imag_0_6),
	.o_real_6(sum_Y_real_0_6),
	.o_imag_6(sum_Y_imag_0_6),
	.i_real_7(Y_real_0_7),
	.i_imag_7(Y_imag_0_7),
	.o_real_7(sum_Y_real_0_7),
	.o_imag_7(sum_Y_imag_0_7),
	.i_real_8(Y_real_0_8),
	.i_imag_8(Y_imag_0_8),
	.o_real_8(sum_Y_real_0_8),
	.o_imag_8(sum_Y_imag_0_8),
	.i_real_9(Y_real_0_9),
	.i_imag_9(Y_imag_0_9),
	.o_real_9(sum_Y_real_0_9),
	.o_imag_9(sum_Y_imag_0_9),
	.i_real_10(Y_real_0_10),
	.i_imag_10(Y_imag_0_10),
	.o_real_10(sum_Y_real_0_10),
	.o_imag_10(sum_Y_imag_0_10),
	.i_real_11(Y_real_0_11),
	.i_imag_11(Y_imag_0_11),
	.o_real_11(sum_Y_real_0_11),
	.o_imag_11(sum_Y_imag_0_11),
	.i_real_12(Y_real_0_12),
	.i_imag_12(Y_imag_0_12),
	.o_real_12(sum_Y_real_0_12),
	.o_imag_12(sum_Y_imag_0_12),
	.i_real_13(Y_real_0_13),
	.i_imag_13(Y_imag_0_13),
	.o_real_13(sum_Y_real_0_13),
	.o_imag_13(sum_Y_imag_0_13),
	.i_real_14(Y_real_0_14),
	.i_imag_14(Y_imag_0_14),
	.o_real_14(sum_Y_real_0_14),
	.o_imag_14(sum_Y_imag_0_14),
	.i_real_15(Y_real_0_15),
	.i_imag_15(Y_imag_0_15),
	.o_real_15(sum_Y_real_0_15),
	.o_imag_15(sum_Y_imag_0_15),
	.o_valid(accum_valid_0)
);

shift_register_group_18_16_1 shift_register_group_18_16_1_inst_real_0 (
	.clk(clk),
	.enable(enable),
	.in_0(sum_Y_real_0_0),
	.out_0(sum_Y_real_hold_0_0),
	.in_1(sum_Y_real_0_1),
	.out_1(sum_Y_real_hold_0_1),
	.in_2(sum_Y_real_0_2),
	.out_2(sum_Y_real_hold_0_2),
	.in_3(sum_Y_real_0_3),
	.out_3(sum_Y_real_hold_0_3),
	.in_4(sum_Y_real_0_4),
	.out_4(sum_Y_real_hold_0_4),
	.in_5(sum_Y_real_0_5),
	.out_5(sum_Y_real_hold_0_5),
	.in_6(sum_Y_real_0_6),
	.out_6(sum_Y_real_hold_0_6),
	.in_7(sum_Y_real_0_7),
	.out_7(sum_Y_real_hold_0_7),
	.in_8(sum_Y_real_0_8),
	.out_8(sum_Y_real_hold_0_8),
	.in_9(sum_Y_real_0_9),
	.out_9(sum_Y_real_hold_0_9),
	.in_10(sum_Y_real_0_10),
	.out_10(sum_Y_real_hold_0_10),
	.in_11(sum_Y_real_0_11),
	.out_11(sum_Y_real_hold_0_11),
	.in_12(sum_Y_real_0_12),
	.out_12(sum_Y_real_hold_0_12),
	.in_13(sum_Y_real_0_13),
	.out_13(sum_Y_real_hold_0_13),
	.in_14(sum_Y_real_0_14),
	.out_14(sum_Y_real_hold_0_14),
	.in_15(sum_Y_real_0_15),
	.out_15(sum_Y_real_hold_0_15),
	.reset(reset)
);

shift_register_group_18_16_1 shift_register_group_18_16_1_inst_imag_0 (
	.clk(clk),
	.enable(enable),
	.in_0(sum_Y_imag_0_0),
	.out_0(sum_Y_imag_hold_0_0),
	.in_1(sum_Y_imag_0_1),
	.out_1(sum_Y_imag_hold_0_1),
	.in_2(sum_Y_imag_0_2),
	.out_2(sum_Y_imag_hold_0_2),
	.in_3(sum_Y_imag_0_3),
	.out_3(sum_Y_imag_hold_0_3),
	.in_4(sum_Y_imag_0_4),
	.out_4(sum_Y_imag_hold_0_4),
	.in_5(sum_Y_imag_0_5),
	.out_5(sum_Y_imag_hold_0_5),
	.in_6(sum_Y_imag_0_6),
	.out_6(sum_Y_imag_hold_0_6),
	.in_7(sum_Y_imag_0_7),
	.out_7(sum_Y_imag_hold_0_7),
	.in_8(sum_Y_imag_0_8),
	.out_8(sum_Y_imag_hold_0_8),
	.in_9(sum_Y_imag_0_9),
	.out_9(sum_Y_imag_hold_0_9),
	.in_10(sum_Y_imag_0_10),
	.out_10(sum_Y_imag_hold_0_10),
	.in_11(sum_Y_imag_0_11),
	.out_11(sum_Y_imag_hold_0_11),
	.in_12(sum_Y_imag_0_12),
	.out_12(sum_Y_imag_hold_0_12),
	.in_13(sum_Y_imag_0_13),
	.out_13(sum_Y_imag_hold_0_13),
	.in_14(sum_Y_imag_0_14),
	.out_14(sum_Y_imag_hold_0_14),
	.in_15(sum_Y_imag_0_15),
	.out_15(sum_Y_imag_hold_0_15),
	.reset(reset)
);

idft_16_top_18 idft_16_top_18_inst_0 (
	.clk(clk),
	.reset(reset),
	.next(accum_valid_0),
	.X0(sum_Y_real_hold_0_0),
	.Y0(out_Y_idft_0_0),
	.X1(sum_Y_imag_hold_0_0),
	.Y1(),
	.X2(sum_Y_real_hold_0_1),
	.Y2(out_Y_idft_0_1),
	.X3(sum_Y_imag_hold_0_1),
	.Y3(),
	.X4(sum_Y_real_hold_0_2),
	.Y4(out_Y_idft_0_2),
	.X5(sum_Y_imag_hold_0_2),
	.Y5(),
	.X6(sum_Y_real_hold_0_3),
	.Y6(out_Y_idft_0_3),
	.X7(sum_Y_imag_hold_0_3),
	.Y7(),
	.X8(sum_Y_real_hold_0_4),
	.Y8(out_Y_idft_0_4),
	.X9(sum_Y_imag_hold_0_4),
	.Y9(),
	.X10(sum_Y_real_hold_0_5),
	.Y10(out_Y_idft_0_5),
	.X11(sum_Y_imag_hold_0_5),
	.Y11(),
	.X12(sum_Y_real_hold_0_6),
	.Y12(out_Y_idft_0_6),
	.X13(sum_Y_imag_hold_0_6),
	.Y13(),
	.X14(sum_Y_real_hold_0_7),
	.Y14(out_Y_idft_0_7),
	.X15(sum_Y_imag_hold_0_7),
	.Y15(),
	.X16(sum_Y_real_hold_0_8),
	.Y16(out_Y_idft_0_8),
	.X17(sum_Y_imag_hold_0_8),
	.Y17(),
	.X18(sum_Y_real_hold_0_9),
	.Y18(out_Y_idft_0_9),
	.X19(sum_Y_imag_hold_0_9),
	.Y19(),
	.X20(sum_Y_real_hold_0_10),
	.Y20(out_Y_idft_0_10),
	.X21(sum_Y_imag_hold_0_10),
	.Y21(),
	.X22(sum_Y_real_hold_0_11),
	.Y22(out_Y_idft_0_11),
	.X23(sum_Y_imag_hold_0_11),
	.Y23(),
	.X24(sum_Y_real_hold_0_12),
	.Y24(out_Y_idft_0_12),
	.X25(sum_Y_imag_hold_0_12),
	.Y25(),
	.X26(sum_Y_real_hold_0_13),
	.Y26(out_Y_idft_0_13),
	.X27(sum_Y_imag_hold_0_13),
	.Y27(),
	.X28(sum_Y_real_hold_0_14),
	.Y28(out_Y_idft_0_14),
	.X29(sum_Y_imag_hold_0_14),
	.Y29(),
	.X30(sum_Y_real_hold_0_15),
	.Y30(out_Y_idft_0_15),
	.X31(sum_Y_imag_hold_0_15),
	.Y31(),
	.next_out(idft_next_out_0)
);

always @ (posedge clk) begin
	if (reset) begin
		reg_Y_0_0 <= 0;
		reg_Y_0_1 <= 0;
		reg_Y_0_2 <= 0;
		reg_Y_0_3 <= 0;
		reg_Y_0_4 <= 0;
		reg_Y_0_5 <= 0;
		reg_Y_0_6 <= 0;
		reg_Y_0_7 <= 0;
		reg_Y_0_8 <= 0;
		reg_Y_0_9 <= 0;
		reg_Y_0_10 <= 0;
		reg_Y_0_11 <= 0;
		reg_Y_0_12 <= 0;
		reg_Y_0_13 <= 0;
		reg_Y_0_14 <= 0;
		reg_Y_0_15 <= 0;
		idft_out_valid <= 1'b0;
		reg_o_valid <= 1'b0;
	end else if (enable) begin
		reg_Y_0_0 <= (out_Y_idft_0_0 >>> 4);
		reg_Y_0_1 <= (out_Y_idft_0_1 >>> 4);
		reg_Y_0_2 <= (out_Y_idft_0_2 >>> 4);
		reg_Y_0_3 <= (out_Y_idft_0_3 >>> 4);
		reg_Y_0_4 <= (out_Y_idft_0_4 >>> 4);
		reg_Y_0_5 <= (out_Y_idft_0_5 >>> 4);
		reg_Y_0_6 <= (out_Y_idft_0_6 >>> 4);
		reg_Y_0_7 <= (out_Y_idft_0_7 >>> 4);
		reg_Y_0_8 <= (out_Y_idft_0_8 >>> 4);
		reg_Y_0_9 <= (out_Y_idft_0_9 >>> 4);
		reg_Y_0_10 <= (out_Y_idft_0_10 >>> 4);
		reg_Y_0_11 <= (out_Y_idft_0_11 >>> 4);
		reg_Y_0_12 <= (out_Y_idft_0_12 >>> 4);
		reg_Y_0_13 <= (out_Y_idft_0_13 >>> 4);
		reg_Y_0_14 <= (out_Y_idft_0_14 >>> 4);
		reg_Y_0_15 <= (out_Y_idft_0_15 >>> 4);
		idft_out_valid <= idft_next_out_0;
		reg_o_valid <= idft_out_valid;
	end
end

assign o_valid = enable & reg_o_valid;
assign o_ready = matrix_vec_mult_ready;
assign o_Y_0_0 = reg_Y_0_0;
assign o_Y_0_1 = reg_Y_0_1;
assign o_Y_0_2 = reg_Y_0_2;
assign o_Y_0_3 = reg_Y_0_3;
assign o_Y_0_4 = reg_Y_0_4;
assign o_Y_0_5 = reg_Y_0_5;
assign o_Y_0_6 = reg_Y_0_6;
assign o_Y_0_7 = reg_Y_0_7;
assign o_Y_0_8 = reg_Y_0_8;
assign o_Y_0_9 = reg_Y_0_9;
assign o_Y_0_10 = reg_Y_0_10;
assign o_Y_0_11 = reg_Y_0_11;
assign o_Y_0_12 = reg_Y_0_12;
assign o_Y_0_13 = reg_Y_0_13;
assign o_Y_0_14 = reg_Y_0_14;
assign o_Y_0_15 = reg_Y_0_15;

endmodule

module idft_16_top_18 (
	input clk,
	input reset,
	input next,
	input [17:0] X0,
	output [17:0] Y0,
	input [17:0] X1,
	output [17:0] Y1,
	input [17:0] X2,
	output [17:0] Y2,
	input [17:0] X3,
	output [17:0] Y3,
	input [17:0] X4,
	output [17:0] Y4,
	input [17:0] X5,
	output [17:0] Y5,
	input [17:0] X6,
	output [17:0] Y6,
	input [17:0] X7,
	output [17:0] Y7,
	input [17:0] X8,
	output [17:0] Y8,
	input [17:0] X9,
	output [17:0] Y9,
	input [17:0] X10,
	output [17:0] Y10,
	input [17:0] X11,
	output [17:0] Y11,
	input [17:0] X12,
	output [17:0] Y12,
	input [17:0] X13,
	output [17:0] Y13,
	input [17:0] X14,
	output [17:0] Y14,
	input [17:0] X15,
	output [17:0] Y15,
	input [17:0] X16,
	output [17:0] Y16,
	input [17:0] X17,
	output [17:0] Y17,
	input [17:0] X18,
	output [17:0] Y18,
	input [17:0] X19,
	output [17:0] Y19,
	input [17:0] X20,
	output [17:0] Y20,
	input [17:0] X21,
	output [17:0] Y21,
	input [17:0] X22,
	output [17:0] Y22,
	input [17:0] X23,
	output [17:0] Y23,
	input [17:0] X24,
	output [17:0] Y24,
	input [17:0] X25,
	output [17:0] Y25,
	input [17:0] X26,
	output [17:0] Y26,
	input [17:0] X27,
	output [17:0] Y27,
	input [17:0] X28,
	output [17:0] Y28,
	input [17:0] X29,
	output [17:0] Y29,
	input [17:0] X30,
	output [17:0] Y30,
	input [17:0] X31,
	output [17:0] Y31,
	output next_out
);
wire [17:0] t0_0;
wire [17:0] t0_1;
wire [17:0] t0_2;
wire [17:0] t0_3;
wire [17:0] t0_4;
wire [17:0] t0_5;
wire [17:0] t0_6;
wire [17:0] t0_7;
wire [17:0] t0_8;
wire [17:0] t0_9;
wire [17:0] t0_10;
wire [17:0] t0_11;
wire [17:0] t0_12;
wire [17:0] t0_13;
wire [17:0] t0_14;
wire [17:0] t0_15;
wire [17:0] t0_16;
wire [17:0] t0_17;
wire [17:0] t0_18;
wire [17:0] t0_19;
wire [17:0] t0_20;
wire [17:0] t0_21;
wire [17:0] t0_22;
wire [17:0] t0_23;
wire [17:0] t0_24;
wire [17:0] t0_25;
wire [17:0] t0_26;
wire [17:0] t0_27;
wire [17:0] t0_28;
wire [17:0] t0_29;
wire [17:0] t0_30;
wire [17:0] t0_31;
wire next_0;
wire [17:0] t1_0;
wire [17:0] t1_1;
wire [17:0] t1_2;
wire [17:0] t1_3;
wire [17:0] t1_4;
wire [17:0] t1_5;
wire [17:0] t1_6;
wire [17:0] t1_7;
wire [17:0] t1_8;
wire [17:0] t1_9;
wire [17:0] t1_10;
wire [17:0] t1_11;
wire [17:0] t1_12;
wire [17:0] t1_13;
wire [17:0] t1_14;
wire [17:0] t1_15;
wire [17:0] t1_16;
wire [17:0] t1_17;
wire [17:0] t1_18;
wire [17:0] t1_19;
wire [17:0] t1_20;
wire [17:0] t1_21;
wire [17:0] t1_22;
wire [17:0] t1_23;
wire [17:0] t1_24;
wire [17:0] t1_25;
wire [17:0] t1_26;
wire [17:0] t1_27;
wire [17:0] t1_28;
wire [17:0] t1_29;
wire [17:0] t1_30;
wire [17:0] t1_31;
wire next_1;
wire [17:0] t2_0;
wire [17:0] t2_1;
wire [17:0] t2_2;
wire [17:0] t2_3;
wire [17:0] t2_4;
wire [17:0] t2_5;
wire [17:0] t2_6;
wire [17:0] t2_7;
wire [17:0] t2_8;
wire [17:0] t2_9;
wire [17:0] t2_10;
wire [17:0] t2_11;
wire [17:0] t2_12;
wire [17:0] t2_13;
wire [17:0] t2_14;
wire [17:0] t2_15;
wire [17:0] t2_16;
wire [17:0] t2_17;
wire [17:0] t2_18;
wire [17:0] t2_19;
wire [17:0] t2_20;
wire [17:0] t2_21;
wire [17:0] t2_22;
wire [17:0] t2_23;
wire [17:0] t2_24;
wire [17:0] t2_25;
wire [17:0] t2_26;
wire [17:0] t2_27;
wire [17:0] t2_28;
wire [17:0] t2_29;
wire [17:0] t2_30;
wire [17:0] t2_31;
wire next_2;

assign t0_0 = X0;
assign Y0 = t2_0;
assign t0_1 = X1;
assign Y1 = t2_1;
assign t0_2 = X2;
assign Y2 = t2_2;
assign t0_3 = X3;
assign Y3 = t2_3;
assign t0_4 = X4;
assign Y4 = t2_4;
assign t0_5 = X5;
assign Y5 = t2_5;
assign t0_6 = X6;
assign Y6 = t2_6;
assign t0_7 = X7;
assign Y7 = t2_7;
assign t0_8 = X8;
assign Y8 = t2_8;
assign t0_9 = X9;
assign Y9 = t2_9;
assign t0_10 = X10;
assign Y10 = t2_10;
assign t0_11 = X11;
assign Y11 = t2_11;
assign t0_12 = X12;
assign Y12 = t2_12;
assign t0_13 = X13;
assign Y13 = t2_13;
assign t0_14 = X14;
assign Y14 = t2_14;
assign t0_15 = X15;
assign Y15 = t2_15;
assign t0_16 = X16;
assign Y16 = t2_16;
assign t0_17 = X17;
assign Y17 = t2_17;
assign t0_18 = X18;
assign Y18 = t2_18;
assign t0_19 = X19;
assign Y19 = t2_19;
assign t0_20 = X20;
assign Y20 = t2_20;
assign t0_21 = X21;
assign Y21 = t2_21;
assign t0_22 = X22;
assign Y22 = t2_22;
assign t0_23 = X23;
assign Y23 = t2_23;
assign t0_24 = X24;
assign Y24 = t2_24;
assign t0_25 = X25;
assign Y25 = t2_25;
assign t0_26 = X26;
assign Y26 = t2_26;
assign t0_27 = X27;
assign Y27 = t2_27;
assign t0_28 = X28;
assign Y28 = t2_28;
assign t0_29 = X29;
assign Y29 = t2_29;
assign t0_30 = X30;
assign Y30 = t2_30;
assign t0_31 = X31;
assign Y31 = t2_31;
assign next_0 = next;
assign next_out = next_2;
codeBlock98050_18 codeBlock98050_18_inst (
	.clk(clk),
	.reset(reset),
	.next_in(next_0),
	.X0_in(t0_0),
	.Y0(t1_0),
	.X1_in(t0_1),
	.Y1(t1_1),
	.X2_in(t0_2),
	.Y2(t1_2),
	.X3_in(t0_3),
	.Y3(t1_3),
	.X4_in(t0_4),
	.Y4(t1_4),
	.X5_in(t0_5),
	.Y5(t1_5),
	.X6_in(t0_6),
	.Y6(t1_6),
	.X7_in(t0_7),
	.Y7(t1_7),
	.X8_in(t0_8),
	.Y8(t1_8),
	.X9_in(t0_9),
	.Y9(t1_9),
	.X10_in(t0_10),
	.Y10(t1_10),
	.X11_in(t0_11),
	.Y11(t1_11),
	.X12_in(t0_12),
	.Y12(t1_12),
	.X13_in(t0_13),
	.Y13(t1_13),
	.X14_in(t0_14),
	.Y14(t1_14),
	.X15_in(t0_15),
	.Y15(t1_15),
	.X16_in(t0_16),
	.Y16(t1_16),
	.X17_in(t0_17),
	.Y17(t1_17),
	.X18_in(t0_18),
	.Y18(t1_18),
	.X19_in(t0_19),
	.Y19(t1_19),
	.X20_in(t0_20),
	.Y20(t1_20),
	.X21_in(t0_21),
	.Y21(t1_21),
	.X22_in(t0_22),
	.Y22(t1_22),
	.X23_in(t0_23),
	.Y23(t1_23),
	.X24_in(t0_24),
	.Y24(t1_24),
	.X25_in(t0_25),
	.Y25(t1_25),
	.X26_in(t0_26),
	.Y26(t1_26),
	.X27_in(t0_27),
	.Y27(t1_27),
	.X28_in(t0_28),
	.Y28(t1_28),
	.X29_in(t0_29),
	.Y29(t1_29),
	.X30_in(t0_30),
	.Y30(t1_30),
	.X31_in(t0_31),
	.Y31(t1_31),
	.next_out(next_1)
);

codeBlock99168_18 codeBlock99168_18_inst (
	.clk(clk),
	.reset(reset),
	.next_in(next_1),
	.X0_in(t1_0),
	.Y0(t2_0),
	.X1_in(t1_1),
	.Y1(t2_1),
	.X2_in(t1_2),
	.Y2(t2_2),
	.X3_in(t1_3),
	.Y3(t2_3),
	.X4_in(t1_4),
	.Y4(t2_4),
	.X5_in(t1_5),
	.Y5(t2_5),
	.X6_in(t1_6),
	.Y6(t2_6),
	.X7_in(t1_7),
	.Y7(t2_7),
	.X8_in(t1_8),
	.Y8(t2_8),
	.X9_in(t1_9),
	.Y9(t2_9),
	.X10_in(t1_10),
	.Y10(t2_10),
	.X11_in(t1_11),
	.Y11(t2_11),
	.X12_in(t1_12),
	.Y12(t2_12),
	.X13_in(t1_13),
	.Y13(t2_13),
	.X14_in(t1_14),
	.Y14(t2_14),
	.X15_in(t1_15),
	.Y15(t2_15),
	.X16_in(t1_16),
	.Y16(t2_16),
	.X17_in(t1_17),
	.Y17(t2_17),
	.X18_in(t1_18),
	.Y18(t2_18),
	.X19_in(t1_19),
	.Y19(t2_19),
	.X20_in(t1_20),
	.Y20(t2_20),
	.X21_in(t1_21),
	.Y21(t2_21),
	.X22_in(t1_22),
	.Y22(t2_22),
	.X23_in(t1_23),
	.Y23(t2_23),
	.X24_in(t1_24),
	.Y24(t2_24),
	.X25_in(t1_25),
	.Y25(t2_25),
	.X26_in(t1_26),
	.Y26(t2_26),
	.X27_in(t1_27),
	.Y27(t2_27),
	.X28_in(t1_28),
	.Y28(t2_28),
	.X29_in(t1_29),
	.Y29(t2_29),
	.X30_in(t1_30),
	.Y30(t2_30),
	.X31_in(t1_31),
	.Y31(t2_31),
	.next_out(next_2)
);

endmodule

module codeBlock98050_18 (
	input clk,
	input reset,
	input next_in,
	input [17:0] X0_in,
	output [17:0] Y0,
	input [17:0] X1_in,
	output [17:0] Y1,
	input [17:0] X2_in,
	output [17:0] Y2,
	input [17:0] X3_in,
	output [17:0] Y3,
	input [17:0] X4_in,
	output [17:0] Y4,
	input [17:0] X5_in,
	output [17:0] Y5,
	input [17:0] X6_in,
	output [17:0] Y6,
	input [17:0] X7_in,
	output [17:0] Y7,
	input [17:0] X8_in,
	output [17:0] Y8,
	input [17:0] X9_in,
	output [17:0] Y9,
	input [17:0] X10_in,
	output [17:0] Y10,
	input [17:0] X11_in,
	output [17:0] Y11,
	input [17:0] X12_in,
	output [17:0] Y12,
	input [17:0] X13_in,
	output [17:0] Y13,
	input [17:0] X14_in,
	output [17:0] Y14,
	input [17:0] X15_in,
	output [17:0] Y15,
	input [17:0] X16_in,
	output [17:0] Y16,
	input [17:0] X17_in,
	output [17:0] Y17,
	input [17:0] X18_in,
	output [17:0] Y18,
	input [17:0] X19_in,
	output [17:0] Y19,
	input [17:0] X20_in,
	output [17:0] Y20,
	input [17:0] X21_in,
	output [17:0] Y21,
	input [17:0] X22_in,
	output [17:0] Y22,
	input [17:0] X23_in,
	output [17:0] Y23,
	input [17:0] X24_in,
	output [17:0] Y24,
	input [17:0] X25_in,
	output [17:0] Y25,
	input [17:0] X26_in,
	output [17:0] Y26,
	input [17:0] X27_in,
	output [17:0] Y27,
	input [17:0] X28_in,
	output [17:0] Y28,
	input [17:0] X29_in,
	output [17:0] Y29,
	input [17:0] X30_in,
	output [17:0] Y30,
	input [17:0] X31_in,
	output [17:0] Y31,
	output next_out
);

reg next;
reg [17:0] X0;
reg [17:0] X1;
reg [17:0] X2;
reg [17:0] X3;
reg [17:0] X4;
reg [17:0] X5;
reg [17:0] X6;
reg [17:0] X7;
reg [17:0] X8;
reg [17:0] X9;
reg [17:0] X10;
reg [17:0] X11;
reg [17:0] X12;
reg [17:0] X13;
reg [17:0] X14;
reg [17:0] X15;
reg [17:0] X16;
reg [17:0] X17;
reg [17:0] X18;
reg [17:0] X19;
reg [17:0] X20;
reg [17:0] X21;
reg [17:0] X22;
reg [17:0] X23;
reg [17:0] X24;
reg [17:0] X25;
reg [17:0] X26;
reg [17:0] X27;
reg [17:0] X28;
reg [17:0] X29;
reg [17:0] X30;
reg [17:0] X31;
shiftRegFIFO_5_1 shiftRegFIFO_5_1_inst (
	.X(next),
	.Y(next_out),
	.reset(reset),
	.clk(clk)
);
wire  [17:0] a249;
 		wire  [17:0] a250;
 		wire  [17:0] a251;
 		wire  [17:0] a252;
 		wire  [17:0] a257;
 		wire  [17:0] a258;
 		wire  [17:0] a259;
 		wire  [17:0] a260;
 		wire  [17:0] a265;
 		wire  [17:0] a266;
 		wire  [17:0] a267;
 		wire  [17:0] a268;
 		wire  [17:0] a273;
 		wire  [17:0] a274;
 		wire  [17:0] a275;
 		wire  [17:0] a276;
 		wire  [17:0] a281;
 		wire  [17:0] a282;
 		wire  [17:0] a283;
 		wire  [17:0] a284;
 		wire  [17:0] a289;
 		wire  [17:0] a290;
 		wire  [17:0] a291;
 		wire  [17:0] a292;
 		wire  [17:0] a297;
 		wire  [17:0] a298;
 		wire  [17:0] a299;
 		wire  [17:0] a300;
 		wire  [17:0] a305;
 		wire  [17:0] a306;
 		wire  [17:0] a307;
 		wire  [17:0] a308;
 		wire  [17:0] t914;
 		wire  [17:0] t915;
 		wire  [17:0] t916;
 		wire  [17:0] t917;
 		wire  [17:0] t918;
 		wire  [17:0] t919;
 		wire  [17:0] t920;
 		wire  [17:0] t921;
 		wire  [17:0] t930;
 		wire  [17:0] t931;
 		wire  [17:0] t932;
 		wire  [17:0] t933;
 		wire  [17:0] t934;
 		wire  [17:0] t935;
 		wire  [17:0] t936;
 		wire  [17:0] t937;
 		wire  [17:0] t952;
 		wire  [17:0] t953;
 		wire  [17:0] t954;
 		wire  [17:0] t955;
 		wire  [17:0] t956;
 		wire  [17:0] t957;
 		wire  [17:0] t958;
 		wire  [17:0] t959;
 		wire  [17:0] t972;
 		wire  [17:0] t973;
 		wire  [17:0] t974;
 		wire  [17:0] t975;
 		wire  [17:0] t976;
 		wire  [17:0] t977;
 		wire  [17:0] t978;
 		wire  [17:0] t979;
 		wire  [17:0] t922;
 		wire  [17:0] t923;
 		wire  [17:0] t924;
 		wire  [17:0] t925;
 		wire  [17:0] t926;
 		wire  [17:0] t927;
 		wire  [17:0] t928;
 		wire  [17:0] t929;
 		wire  [17:0] t938;
 		wire  [17:0] t939;
 		wire  [17:0] t940;
 		wire  [17:0] t941;
 		wire  [17:0] t944;
 		wire  [17:0] t945;
 		wire  [17:0] t946;
 		wire  [17:0] t947;
 		wire  [17:0] t960;
 		wire  [17:0] t961;
 		wire  [17:0] t962;
 		wire  [17:0] t963;
 		wire  [17:0] t964;
 		wire  [17:0] t965;
 		wire  [17:0] t966;
 		wire  [17:0] t967;
 		wire  [17:0] t980;
 		wire  [17:0] t981;
 		wire  [17:0] t982;
 		wire  [17:0] t983;
 		wire  [17:0] t986;
 		wire  [17:0] t987;
 		wire  [17:0] t988;
 		wire  [17:0] t989;
 		reg  [17:0] tm24;
 		reg  [17:0] tm27;
 		reg  [17:0] tm30;
 		reg  [17:0] tm33;
 		reg  [17:0] tm36;
 		reg  [17:0] tm39;
 		reg  [17:0] tm42;
 		reg  [17:0] tm45;
 		reg  [17:0] tm48;
 		reg  [17:0] tm51;
 		reg  [17:0] tm54;
 		reg  [17:0] tm57;
 		reg  [17:0] tm60;
 		reg  [17:0] tm63;
 		reg  [17:0] tm66;
 		reg  [17:0] tm69;
 		wire  [17:0] a225;
 		wire  [17:0] a226;
 		wire  [17:0] a227;
 		wire  [17:0] a228;
 		wire  [17:0] a229;
 		wire  [17:0] a230;
 		wire  [17:0] a231;
 		wire  [17:0] a232;
 		wire  [17:0] a233;
 		wire  [17:0] a234;
 		wire  [17:0] a235;
 		wire  [17:0] a236;
 		wire  [17:0] a237;
 		wire  [17:0] a238;
 		wire  [17:0] a239;
 		wire  [17:0] a240;
 		wire  [17:0] a241;
 		wire  [17:0] a242;
 		wire  [17:0] a243;
 		wire  [17:0] a244;
 		wire  [17:0] a245;
 		wire  [17:0] a246;
 		wire  [17:0] a247;
 		wire  [17:0] a248;
 		reg  [17:0] tm25;
 		reg  [17:0] tm28;
 		reg  [17:0] tm31;
 		reg  [17:0] tm34;
 		reg  [17:0] tm37;
 		reg  [17:0] tm40;
 		reg  [17:0] tm43;
 		reg  [17:0] tm46;
 		reg  [17:0] tm49;
 		reg  [17:0] tm52;
 		reg  [17:0] tm55;
 		reg  [17:0] tm58;
 		reg  [17:0] tm61;
 		reg  [17:0] tm64;
 		reg  [17:0] tm67;
 		reg  [17:0] tm70;
 		wire  [17:0] t942;
 		wire  [17:0] t943;
 		wire  [17:0] t948;
 		wire  [17:0] t949;
 		wire  [17:0] t950;
 		wire  [17:0] t951;
 		wire  [17:0] t968;
 		wire  [17:0] t969;
 		wire  [17:0] t970;
 		wire  [17:0] t971;
 		wire  [17:0] t984;
 		wire  [17:0] t985;
 		wire  [17:0] t990;
 		wire  [17:0] t991;
 		wire  [17:0] t992;
 		wire  [17:0] t993;
 		reg  [17:0] tm26;
 		reg  [17:0] tm29;
 		reg  [17:0] tm32;
 		reg  [17:0] tm35;
 		reg  [17:0] tm38;
 		reg  [17:0] tm41;
 		reg  [17:0] tm44;
 		reg  [17:0] tm47;
 		reg  [17:0] tm50;
 		reg  [17:0] tm53;
 		reg  [17:0] tm56;
 		reg  [17:0] tm59;
 		reg  [17:0] tm62;
 		reg  [17:0] tm65;
 		reg  [17:0] tm68;
 		reg  [17:0] tm71;

wire  [17:0] tm0;
assign tm0 = (18'hb505 >> (18-18));
wire  [17:0] tm2;
assign tm2 = (18'hec83 >> (18-18));
wire  [17:0] tm3;
assign tm3 = (18'h61f8 >> (18-18));

assign a249 = X0;
 		assign a250 = X16;
 		assign a251 = X1;
 		assign a252 = X17;
 		assign a257 = X8;
 		assign a258 = X24;
 		assign a259 = X9;
 		assign a260 = X25;
 		assign a265 = X2;
 		assign a266 = X18;
 		assign a267 = X3;
 		assign a268 = X19;
 		assign a273 = X10;
 		assign a274 = X26;
 		assign a275 = X11;
 		assign a276 = X27;
 		assign a281 = X4;
 		assign a282 = X20;
 		assign a283 = X5;
 		assign a284 = X21;
 		assign a289 = X12;
 		assign a290 = X28;
 		assign a291 = X13;
 		assign a292 = X29;
 		assign a297 = X6;
 		assign a298 = X22;
 		assign a299 = X7;
 		assign a300 = X23;
 		assign a305 = X14;
 		assign a306 = X30;
 		assign a307 = X15;
 		assign a308 = X31;
 		assign Y0 = tm26;
 		assign Y1 = tm29;
 		assign Y4 = tm32;
 		assign Y5 = tm35;
 		assign Y2 = tm38;
 		assign Y3 = tm41;
 		assign Y6 = tm44;
 		assign Y7 = tm47;
 		assign Y8 = tm50;
 		assign Y9 = tm53;
 		assign Y12 = t942;
 		assign Y13 = t943;
 		assign Y10 = t948;
 		assign Y11 = t949;
 		assign Y14 = t950;
 		assign Y15 = t951;
 		assign Y16 = tm56;
 		assign Y17 = tm59;
 		assign Y20 = tm62;
 		assign Y21 = tm65;
 		assign Y18 = t968;
 		assign Y19 = t969;
 		assign Y22 = (~(t970)+1);
 		assign Y23 = t971;
 		assign Y24 = tm68;
 		assign Y25 = tm71;
 		assign Y28 = (~(t984)+1);
 		assign Y29 = t985;
 		assign Y26 = t990;
 		assign Y27 = t991;
 		assign Y30 = t992;
 		assign Y31 = (~(t993)+1);

addfxp_18_1 add98062(.a(a249), .b(a250), .clk(clk), .q(t914));
 		addfxp_18_1 add98077(.a(a251), .b(a252), .clk(clk), .q(t915));
 		subfxp_18_1 sub98092(.a(a249), .b(a250), .clk(clk), .q(t916));
 		subfxp_18_1 sub98107(.a(a251), .b(a252), .clk(clk), .q(t917));
 		addfxp_18_1 add98122(.a(a257), .b(a258), .clk(clk), .q(t918));
 		addfxp_18_1 add98137(.a(a259), .b(a260), .clk(clk), .q(t919));
 		subfxp_18_1 sub98152(.a(a257), .b(a258), .clk(clk), .q(t920));
 		subfxp_18_1 sub98167(.a(a259), .b(a260), .clk(clk), .q(t921));
 		addfxp_18_1 add98270(.a(a265), .b(a266), .clk(clk), .q(t930));
 		addfxp_18_1 add98285(.a(a267), .b(a268), .clk(clk), .q(t931));
 		subfxp_18_1 sub98300(.a(a265), .b(a266), .clk(clk), .q(t932));
 		subfxp_18_1 sub98315(.a(a267), .b(a268), .clk(clk), .q(t933));
 		addfxp_18_1 add98330(.a(a273), .b(a274), .clk(clk), .q(t934));
 		addfxp_18_1 add98345(.a(a275), .b(a276), .clk(clk), .q(t935));
 		subfxp_18_1 sub98360(.a(a273), .b(a274), .clk(clk), .q(t936));
 		subfxp_18_1 sub98375(.a(a275), .b(a276), .clk(clk), .q(t937));
 		addfxp_18_1 add98590(.a(a281), .b(a282), .clk(clk), .q(t952));
 		addfxp_18_1 add98605(.a(a283), .b(a284), .clk(clk), .q(t953));
 		subfxp_18_1 sub98620(.a(a281), .b(a282), .clk(clk), .q(t954));
 		subfxp_18_1 sub98635(.a(a283), .b(a284), .clk(clk), .q(t955));
 		addfxp_18_1 add98650(.a(a289), .b(a290), .clk(clk), .q(t956));
 		addfxp_18_1 add98665(.a(a291), .b(a292), .clk(clk), .q(t957));
 		subfxp_18_1 sub98680(.a(a289), .b(a290), .clk(clk), .q(t958));
 		subfxp_18_1 sub98695(.a(a291), .b(a292), .clk(clk), .q(t959));
 		addfxp_18_1 add98856(.a(a297), .b(a298), .clk(clk), .q(t972));
 		addfxp_18_1 add98871(.a(a299), .b(a300), .clk(clk), .q(t973));
 		subfxp_18_1 sub98886(.a(a297), .b(a298), .clk(clk), .q(t974));
 		subfxp_18_1 sub98901(.a(a299), .b(a300), .clk(clk), .q(t975));
 		addfxp_18_1 add98916(.a(a305), .b(a306), .clk(clk), .q(t976));
 		addfxp_18_1 add98931(.a(a307), .b(a308), .clk(clk), .q(t977));
 		subfxp_18_1 sub98946(.a(a305), .b(a306), .clk(clk), .q(t978));
 		subfxp_18_1 sub98961(.a(a307), .b(a308), .clk(clk), .q(t979));
 		addfxp_18_1 add98174(.a(t914), .b(t918), .clk(clk), .q(t922));
 		addfxp_18_1 add98181(.a(t915), .b(t919), .clk(clk), .q(t923));
 		subfxp_18_1 sub98188(.a(t914), .b(t918), .clk(clk), .q(t924));
 		subfxp_18_1 sub98195(.a(t915), .b(t919), .clk(clk), .q(t925));
 		subfxp_18_1 sub98218(.a(t916), .b(t921), .clk(clk), .q(t926));
 		addfxp_18_1 add98225(.a(t917), .b(t920), .clk(clk), .q(t927));
 		addfxp_18_1 add98232(.a(t916), .b(t921), .clk(clk), .q(t928));
 		subfxp_18_1 sub98239(.a(t917), .b(t920), .clk(clk), .q(t929));
 		addfxp_18_1 add98382(.a(t930), .b(t934), .clk(clk), .q(t938));
 		addfxp_18_1 add98389(.a(t931), .b(t935), .clk(clk), .q(t939));
 		subfxp_18_1 sub98396(.a(t930), .b(t934), .clk(clk), .q(t940));
 		subfxp_18_1 sub98403(.a(t931), .b(t935), .clk(clk), .q(t941));
 		subfxp_18_1 sub98454(.a(t932), .b(t937), .clk(clk), .q(t944));
 		addfxp_18_1 add98461(.a(t933), .b(t936), .clk(clk), .q(t945));
 		addfxp_18_1 add98468(.a(t932), .b(t937), .clk(clk), .q(t946));
 		subfxp_18_1 sub98475(.a(t933), .b(t936), .clk(clk), .q(t947));
 		addfxp_18_1 add98702(.a(t952), .b(t956), .clk(clk), .q(t960));
 		addfxp_18_1 add98709(.a(t953), .b(t957), .clk(clk), .q(t961));
 		subfxp_18_1 sub98716(.a(t952), .b(t956), .clk(clk), .q(t962));
 		subfxp_18_1 sub98723(.a(t953), .b(t957), .clk(clk), .q(t963));
 		subfxp_18_1 sub98747(.a(t954), .b(t959), .clk(clk), .q(t964));
 		addfxp_18_1 add98754(.a(t955), .b(t958), .clk(clk), .q(t965));
 		addfxp_18_1 add98761(.a(t954), .b(t959), .clk(clk), .q(t966));
 		subfxp_18_1 sub98768(.a(t955), .b(t958), .clk(clk), .q(t967));
 		addfxp_18_1 add98968(.a(t972), .b(t976), .clk(clk), .q(t980));
 		addfxp_18_1 add98975(.a(t973), .b(t977), .clk(clk), .q(t981));
 		subfxp_18_1 sub98982(.a(t972), .b(t976), .clk(clk), .q(t982));
 		subfxp_18_1 sub98989(.a(t973), .b(t977), .clk(clk), .q(t983));
 		subfxp_18_1 sub99041(.a(t974), .b(t979), .clk(clk), .q(t986));
 		addfxp_18_1 add99048(.a(t975), .b(t978), .clk(clk), .q(t987));
 		addfxp_18_1 add99055(.a(t974), .b(t979), .clk(clk), .q(t988));
 		subfxp_18_1 sub99062(.a(t975), .b(t978), .clk(clk), .q(t989));

multfix_alt_dsp_18 m88566(.ax(tm0), .ay(t940), .bx(tm0), .by(t941), .clk(clk), .a_q_sc(a225), .a_q_unsc(), .b_q_sc(a226), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88570(.ax(tm2), .ay(t944), .bx(tm3), .by(t945), .clk(clk), .a_q_sc(a227), .a_q_unsc(), .b_q_sc(a228), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88572(.ax(tm3), .ay(t944), .bx(tm2), .by(t945), .clk(clk), .a_q_sc(a229), .a_q_unsc(), .b_q_sc(a230), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88574(.ax(tm3), .ay(t946), .bx(tm2), .by(t947), .clk(clk), .a_q_sc(a231), .a_q_unsc(), .b_q_sc(a232), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88576(.ax(tm2), .ay(t946), .bx(tm3), .by(t947), .clk(clk), .a_q_sc(a233), .a_q_unsc(), .b_q_sc(a234), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88578(.ax(tm0), .ay(t964), .bx(tm0), .by(t965), .clk(clk), .a_q_sc(a235), .a_q_unsc(), .b_q_sc(a236), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88580(.ax(tm0), .ay(t966), .bx(tm0), .by(t967), .clk(clk), .a_q_sc(a237), .a_q_unsc(), .b_q_sc(a238), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88582(.ax(tm0), .ay(t982), .bx(tm0), .by(t983), .clk(clk), .a_q_sc(a239), .a_q_unsc(), .b_q_sc(a240), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88584(.ax(tm3), .ay(t986), .bx(tm2), .by(t987), .clk(clk), .a_q_sc(a241), .a_q_unsc(), .b_q_sc(a242), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88586(.ax(tm2), .ay(t986), .bx(tm3), .by(t987), .clk(clk), .a_q_sc(a243), .a_q_unsc(), .b_q_sc(a244), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88588(.ax(tm3), .ay(t989), .bx(tm2), .by(t988), .clk(clk), .a_q_sc(a245), .a_q_unsc(), .b_q_sc(a246), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88590(.ax(tm3), .ay(t988), .bx(tm2), .by(t989), .clk(clk), .a_q_sc(a247), .a_q_unsc(), .b_q_sc(a248), .b_q_unsc(), .rst(reset));

subfxp_18_1 sub98424(.a(a225), .b(a226), .clk(clk), .q(t942));
 		addfxp_18_1 add98431(.a(a225), .b(a226), .clk(clk), .q(t943));
 		subfxp_18_1 sub98496(.a(a227), .b(a228), .clk(clk), .q(t948));
 		addfxp_18_1 add98517(.a(a229), .b(a230), .clk(clk), .q(t949));
 		subfxp_18_1 sub98538(.a(a231), .b(a232), .clk(clk), .q(t950));
 		addfxp_18_1 add98559(.a(a233), .b(a234), .clk(clk), .q(t951));
 		subfxp_18_1 sub98789(.a(a235), .b(a236), .clk(clk), .q(t968));
 		addfxp_18_1 add98796(.a(a235), .b(a236), .clk(clk), .q(t969));
 		addfxp_18_1 add98817(.a(a237), .b(a238), .clk(clk), .q(t970));
 		subfxp_18_1 sub98824(.a(a237), .b(a238), .clk(clk), .q(t971));
 		addfxp_18_1 add99010(.a(a239), .b(a240), .clk(clk), .q(t984));
 		subfxp_18_1 sub99017(.a(a239), .b(a240), .clk(clk), .q(t985));
 		subfxp_18_1 sub99083(.a(a241), .b(a242), .clk(clk), .q(t990));
 		addfxp_18_1 add99104(.a(a243), .b(a244), .clk(clk), .q(t991));
 		subfxp_18_1 sub99125(.a(a245), .b(a246), .clk(clk), .q(t992));
 		addfxp_18_1 add99146(.a(a247), .b(a248), .clk(clk), .q(t993));

always @(posedge clk) begin
	if (reset == 1) begin
		next <= 1'b0;
	end else begin
		X0 <= X0_in;
		X1 <= X1_in;
		X2 <= X2_in;
		X3 <= X3_in;
		X4 <= X4_in;
		X5 <= X5_in;
		X6 <= X6_in;
		X7 <= X7_in;
		X8 <= X8_in;
		X9 <= X9_in;
		X10 <= X10_in;
		X11 <= X11_in;
		X12 <= X12_in;
		X13 <= X13_in;
		X14 <= X14_in;
		X15 <= X15_in;
		X16 <= X16_in;
		X17 <= X17_in;
		X18 <= X18_in;
		X19 <= X19_in;
		X20 <= X20_in;
		X21 <= X21_in;
		X22 <= X22_in;
		X23 <= X23_in;
		X24 <= X24_in;
		X25 <= X25_in;
		X26 <= X26_in;
		X27 <= X27_in;
		X28 <= X28_in;
		X29 <= X29_in;
		X30 <= X30_in;
		X31 <= X31_in;
		next <= next_in;
		tm24 <= t922;
		tm27 <= t923;
		tm30 <= t924;
		tm33 <= t925;
		tm36 <= t926;
		tm39 <= t927;
		tm42 <= t928;
		tm45 <= t929;
		tm48 <= t938;
		tm51 <= t939;
		tm54 <= t960;
		tm57 <= t961;
		tm60 <= (~(t963)+1);
		tm63 <= t962;
		tm66 <= t980;
		tm69 <= t981;
		tm25 <= tm24;
		tm28 <= tm27;
		tm31 <= tm30;
		tm34 <= tm33;
		tm37 <= tm36;
		tm40 <= tm39;
		tm43 <= tm42;
		tm46 <= tm45;
		tm49 <= tm48;
		tm52 <= tm51;
		tm55 <= tm54;
		tm58 <= tm57;
		tm61 <= tm60;
		tm64 <= tm63;
		tm67 <= tm66;
		tm70 <= tm69;
		tm26 <= tm25;
		tm29 <= tm28;
		tm32 <= tm31;
		tm35 <= tm34;
		tm38 <= tm37;
		tm41 <= tm40;
		tm44 <= tm43;
		tm47 <= tm46;
		tm50 <= tm49;
		tm53 <= tm52;
		tm56 <= tm55;
		tm59 <= tm58;
		tm62 <= tm61;
		tm65 <= tm64;
		tm68 <= tm67;
		tm71 <= tm70;
	end
end

endmodule

module addfxp_18_1 (
	input [17:0] a,
	input [17:0] b,
	input clk,
	output [17:0] q
);

reg [17:0] res_0;
assign q = res_0;

always @(posedge clk) begin
	res_0 <= a + b;
end

endmodule

module multfix_alt_dsp_18 (
	input [17:0] ax,
	input [17:0] ay,
	input [17:0] bx,
	input [17:0] by,
	input clk,
	output [17:0] a_q_sc,
	output [17:0] a_q_unsc,
	output [17:0] b_q_sc,
	output [17:0] b_q_unsc,
	input rst
);

wire [35:0] a_res;
wire [35:0] b_res;

assign a_q_unsc = a_res[17:0];
assign a_q_sc = {a_res[35-1], a_res[32			:16]};
assign b_q_unsc = b_res[17:0];
assign b_q_sc = {b_res[35-1], b_res[32			:16]};

dsp_signed_mult_18x18_unit_18_36_0 dsp_signed_mult_18x18_unit_18_36_0_inst (
	.clk(clk),
	.ena(1'b1),
	.reset(rst),
	.i_valid(),
	.o_valid(),
	.ax(ax),
	.ay(ay),
	.bx(bx),
	.by(by),
	.resulta(a_res),
	.resultb(b_res)
);

endmodule

module dsp_signed_mult_18x18_unit_18_36_0 (
	input clk,
	input reset,
	input ena,
	input i_valid,
	input [17:0] ax,
	input [17:0] ay,
	input [17:0] bx,
	input [17:0] by,
	output o_valid,
	output [35:0] resulta,
	output [35:0] resultb 
);

reg [35:0] reg_resa, reg_resb;
always @(posedge clk) begin
	reg_resa <= ax * ay;
	reg_resb <= bx * by;
end
assign resulta = reg_resa;
assign resultb = reg_resb;
reg input_valid, result_valid, output_valid;
always @ (posedge clk) begin
	if (reset) begin
		output_valid <= 1'b0;
		input_valid <= 1'b0;
		result_valid <= 1'b0;
	end else if (ena) begin
		input_valid <= i_valid;
		result_valid <= input_valid;
		output_valid <= result_valid;
	end
end
assign o_valid = output_valid;

endmodule

module shiftRegFIFO_5_1 (
	input [0:0] X,
	output [0:0] Y,
	input reset,
	input clk
);

reg [0:0] mem_0;
reg [0:0] mem_1;
reg [0:0] mem_2;
reg [0:0] mem_3;
reg [0:0] mem_4;
assign Y = mem_4;

always @ (posedge clk) begin
	if (reset) begin
		mem_0 <= 0;
		mem_1 <= 0;
		mem_2 <= 0;
		mem_3 <= 0;
		mem_4 <= 0;
	end else begin
		mem_1 <= mem_0;
		mem_2 <= mem_1;
		mem_3 <= mem_2;
		mem_4 <= mem_3;
		mem_0 <= X;
	end
end

endmodule

module subfxp_18_1 (
	input [17:0] a,
	input [17:0] b,
	input clk,
	output [17:0] q
);

reg [17:0] res_0;
assign q = res_0;

always @(posedge clk) begin
	res_0 <= a + b;
end

endmodule

module codeBlock99168_18 (
	input clk,
	input reset,
	input next_in,
	input [17:0] X0_in,
	output [17:0] Y0,
	input [17:0] X1_in,
	output [17:0] Y1,
	input [17:0] X2_in,
	output [17:0] Y2,
	input [17:0] X3_in,
	output [17:0] Y3,
	input [17:0] X4_in,
	output [17:0] Y4,
	input [17:0] X5_in,
	output [17:0] Y5,
	input [17:0] X6_in,
	output [17:0] Y6,
	input [17:0] X7_in,
	output [17:0] Y7,
	input [17:0] X8_in,
	output [17:0] Y8,
	input [17:0] X9_in,
	output [17:0] Y9,
	input [17:0] X10_in,
	output [17:0] Y10,
	input [17:0] X11_in,
	output [17:0] Y11,
	input [17:0] X12_in,
	output [17:0] Y12,
	input [17:0] X13_in,
	output [17:0] Y13,
	input [17:0] X14_in,
	output [17:0] Y14,
	input [17:0] X15_in,
	output [17:0] Y15,
	input [17:0] X16_in,
	output [17:0] Y16,
	input [17:0] X17_in,
	output [17:0] Y17,
	input [17:0] X18_in,
	output [17:0] Y18,
	input [17:0] X19_in,
	output [17:0] Y19,
	input [17:0] X20_in,
	output [17:0] Y20,
	input [17:0] X21_in,
	output [17:0] Y21,
	input [17:0] X22_in,
	output [17:0] Y22,
	input [17:0] X23_in,
	output [17:0] Y23,
	input [17:0] X24_in,
	output [17:0] Y24,
	input [17:0] X25_in,
	output [17:0] Y25,
	input [17:0] X26_in,
	output [17:0] Y26,
	input [17:0] X27_in,
	output [17:0] Y27,
	input [17:0] X28_in,
	output [17:0] Y28,
	input [17:0] X29_in,
	output [17:0] Y29,
	input [17:0] X30_in,
	output [17:0] Y30,
	input [17:0] X31_in,
	output [17:0] Y31,
	output next_out
);

reg next;
reg [17:0] X0;
reg [17:0] X1;
reg [17:0] X2;
reg [17:0] X3;
reg [17:0] X4;
reg [17:0] X5;
reg [17:0] X6;
reg [17:0] X7;
reg [17:0] X8;
reg [17:0] X9;
reg [17:0] X10;
reg [17:0] X11;
reg [17:0] X12;
reg [17:0] X13;
reg [17:0] X14;
reg [17:0] X15;
reg [17:0] X16;
reg [17:0] X17;
reg [17:0] X18;
reg [17:0] X19;
reg [17:0] X20;
reg [17:0] X21;
reg [17:0] X22;
reg [17:0] X23;
reg [17:0] X24;
reg [17:0] X25;
reg [17:0] X26;
reg [17:0] X27;
reg [17:0] X28;
reg [17:0] X29;
reg [17:0] X30;
reg [17:0] X31;
shiftRegFIFO_2_1 shiftRegFIFO_2_1_inst (
	.X(next),
	.Y(next_out),
	.reset(reset),
	.clk(clk)
);
wire  [17:0] a65;
 		wire  [17:0] a66;
 		wire  [17:0] a67;
 		wire  [17:0] a68;
 		wire  [17:0] a73;
 		wire  [17:0] a74;
 		wire  [17:0] a75;
 		wire  [17:0] a76;
 		wire  [17:0] a81;
 		wire  [17:0] a82;
 		wire  [17:0] a83;
 		wire  [17:0] a84;
 		wire  [17:0] a89;
 		wire  [17:0] a90;
 		wire  [17:0] a91;
 		wire  [17:0] a92;
 		wire  [17:0] a97;
 		wire  [17:0] a98;
 		wire  [17:0] a99;
 		wire  [17:0] a100;
 		wire  [17:0] a105;
 		wire  [17:0] a106;
 		wire  [17:0] a107;
 		wire  [17:0] a108;
 		wire  [17:0] a113;
 		wire  [17:0] a114;
 		wire  [17:0] a115;
 		wire  [17:0] a116;
 		wire  [17:0] a121;
 		wire  [17:0] a122;
 		wire  [17:0] a123;
 		wire  [17:0] a124;
 		wire  [17:0] t402;
 		wire  [17:0] t403;
 		wire  [17:0] t404;
 		wire  [17:0] t405;
 		wire  [17:0] t406;
 		wire  [17:0] t407;
 		wire  [17:0] t408;
 		wire  [17:0] t409;
 		wire  [17:0] t418;
 		wire  [17:0] t419;
 		wire  [17:0] t420;
 		wire  [17:0] t421;
 		wire  [17:0] t422;
 		wire  [17:0] t423;
 		wire  [17:0] t424;
 		wire  [17:0] t425;
 		wire  [17:0] t434;
 		wire  [17:0] t435;
 		wire  [17:0] t436;
 		wire  [17:0] t437;
 		wire  [17:0] t438;
 		wire  [17:0] t439;
 		wire  [17:0] t440;
 		wire  [17:0] t441;
 		wire  [17:0] t450;
 		wire  [17:0] t451;
 		wire  [17:0] t452;
 		wire  [17:0] t453;
 		wire  [17:0] t454;
 		wire  [17:0] t455;
 		wire  [17:0] t456;
 		wire  [17:0] t457;
 		wire  [17:0] t410;
 		wire  [17:0] t411;
 		wire  [17:0] t412;
 		wire  [17:0] t413;
 		wire  [17:0] t414;
 		wire  [17:0] t415;
 		wire  [17:0] t416;
 		wire  [17:0] t417;
 		wire  [17:0] t426;
 		wire  [17:0] t427;
 		wire  [17:0] t428;
 		wire  [17:0] t429;
 		wire  [17:0] t430;
 		wire  [17:0] t431;
 		wire  [17:0] t432;
 		wire  [17:0] t433;
 		wire  [17:0] t442;
 		wire  [17:0] t443;
 		wire  [17:0] t444;
 		wire  [17:0] t445;
 		wire  [17:0] t446;
 		wire  [17:0] t447;
 		wire  [17:0] t448;
 		wire  [17:0] t449;
 		wire  [17:0] t458;
 		wire  [17:0] t459;
 		wire  [17:0] t460;
 		wire  [17:0] t461;
 		wire  [17:0] t462;
 		wire  [17:0] t463;
 		wire  [17:0] t464;
 		wire  [17:0] t465;

assign a65 = X0;
 		assign a66 = X16;
 		assign a67 = X1;
 		assign a68 = X17;
 		assign a73 = X8;
 		assign a74 = X24;
 		assign a75 = X9;
 		assign a76 = X25;
 		assign a81 = X2;
 		assign a82 = X18;
 		assign a83 = X3;
 		assign a84 = X19;
 		assign a89 = X10;
 		assign a90 = X26;
 		assign a91 = X11;
 		assign a92 = X27;
 		assign a97 = X4;
 		assign a98 = X20;
 		assign a99 = X5;
 		assign a100 = X21;
 		assign a105 = X12;
 		assign a106 = X28;
 		assign a107 = X13;
 		assign a108 = X29;
 		assign a113 = X6;
 		assign a114 = X22;
 		assign a115 = X7;
 		assign a116 = X23;
 		assign a121 = X14;
 		assign a122 = X30;
 		assign a123 = X15;
 		assign a124 = X31;
 		assign Y0 = t410;
 		assign Y1 = t411;
 		assign Y16 = t412;
 		assign Y17 = t413;
 		assign Y8 = t414;
 		assign Y9 = t415;
 		assign Y24 = t416;
 		assign Y25 = t417;
 		assign Y2 = t426;
 		assign Y3 = t427;
 		assign Y18 = t428;
 		assign Y19 = t429;
 		assign Y10 = t430;
 		assign Y11 = t431;
 		assign Y26 = t432;
 		assign Y27 = t433;
 		assign Y4 = t442;
 		assign Y5 = t443;
 		assign Y20 = t444;
 		assign Y21 = t445;
 		assign Y12 = t446;
 		assign Y13 = t447;
 		assign Y28 = t448;
 		assign Y29 = t449;
 		assign Y6 = t458;
 		assign Y7 = t459;
 		assign Y22 = t460;
 		assign Y23 = t461;
 		assign Y14 = t462;
 		assign Y15 = t463;
 		assign Y30 = t464;
 		assign Y31 = t465;

addfxp_18_1 add99180(.a(a65), .b(a66), .clk(clk), .q(t402));
 		addfxp_18_1 add99195(.a(a67), .b(a68), .clk(clk), .q(t403));
 		subfxp_18_1 sub99210(.a(a65), .b(a66), .clk(clk), .q(t404));
 		subfxp_18_1 sub99225(.a(a67), .b(a68), .clk(clk), .q(t405));
 		addfxp_18_1 add99240(.a(a73), .b(a74), .clk(clk), .q(t406));
 		addfxp_18_1 add99255(.a(a75), .b(a76), .clk(clk), .q(t407));
 		subfxp_18_1 sub99270(.a(a73), .b(a74), .clk(clk), .q(t408));
 		subfxp_18_1 sub99285(.a(a75), .b(a76), .clk(clk), .q(t409));
 		addfxp_18_1 add99388(.a(a81), .b(a82), .clk(clk), .q(t418));
 		addfxp_18_1 add99403(.a(a83), .b(a84), .clk(clk), .q(t419));
 		subfxp_18_1 sub99418(.a(a81), .b(a82), .clk(clk), .q(t420));
 		subfxp_18_1 sub99433(.a(a83), .b(a84), .clk(clk), .q(t421));
 		addfxp_18_1 add99448(.a(a89), .b(a90), .clk(clk), .q(t422));
 		addfxp_18_1 add99463(.a(a91), .b(a92), .clk(clk), .q(t423));
 		subfxp_18_1 sub99478(.a(a89), .b(a90), .clk(clk), .q(t424));
 		subfxp_18_1 sub99493(.a(a91), .b(a92), .clk(clk), .q(t425));
 		addfxp_18_1 add99596(.a(a97), .b(a98), .clk(clk), .q(t434));
 		addfxp_18_1 add99611(.a(a99), .b(a100), .clk(clk), .q(t435));
 		subfxp_18_1 sub99626(.a(a97), .b(a98), .clk(clk), .q(t436));
 		subfxp_18_1 sub99641(.a(a99), .b(a100), .clk(clk), .q(t437));
 		addfxp_18_1 add99656(.a(a105), .b(a106), .clk(clk), .q(t438));
 		addfxp_18_1 add99671(.a(a107), .b(a108), .clk(clk), .q(t439));
 		subfxp_18_1 sub99686(.a(a105), .b(a106), .clk(clk), .q(t440));
 		subfxp_18_1 sub99701(.a(a107), .b(a108), .clk(clk), .q(t441));
 		addfxp_18_1 add99804(.a(a113), .b(a114), .clk(clk), .q(t450));
 		addfxp_18_1 add99819(.a(a115), .b(a116), .clk(clk), .q(t451));
 		subfxp_18_1 sub99834(.a(a113), .b(a114), .clk(clk), .q(t452));
 		subfxp_18_1 sub99849(.a(a115), .b(a116), .clk(clk), .q(t453));
 		addfxp_18_1 add99864(.a(a121), .b(a122), .clk(clk), .q(t454));
 		addfxp_18_1 add99879(.a(a123), .b(a124), .clk(clk), .q(t455));
 		subfxp_18_1 sub99894(.a(a121), .b(a122), .clk(clk), .q(t456));
 		subfxp_18_1 sub99909(.a(a123), .b(a124), .clk(clk), .q(t457));
 		addfxp_18_1 add99292(.a(t402), .b(t406), .clk(clk), .q(t410));
 		addfxp_18_1 add99299(.a(t403), .b(t407), .clk(clk), .q(t411));
 		subfxp_18_1 sub99306(.a(t402), .b(t406), .clk(clk), .q(t412));
 		subfxp_18_1 sub99313(.a(t403), .b(t407), .clk(clk), .q(t413));
 		subfxp_18_1 sub99336(.a(t404), .b(t409), .clk(clk), .q(t414));
 		addfxp_18_1 add99343(.a(t405), .b(t408), .clk(clk), .q(t415));
 		addfxp_18_1 add99350(.a(t404), .b(t409), .clk(clk), .q(t416));
 		subfxp_18_1 sub99357(.a(t405), .b(t408), .clk(clk), .q(t417));
 		addfxp_18_1 add99500(.a(t418), .b(t422), .clk(clk), .q(t426));
 		addfxp_18_1 add99507(.a(t419), .b(t423), .clk(clk), .q(t427));
 		subfxp_18_1 sub99514(.a(t418), .b(t422), .clk(clk), .q(t428));
 		subfxp_18_1 sub99521(.a(t419), .b(t423), .clk(clk), .q(t429));
 		subfxp_18_1 sub99544(.a(t420), .b(t425), .clk(clk), .q(t430));
 		addfxp_18_1 add99551(.a(t421), .b(t424), .clk(clk), .q(t431));
 		addfxp_18_1 add99558(.a(t420), .b(t425), .clk(clk), .q(t432));
 		subfxp_18_1 sub99565(.a(t421), .b(t424), .clk(clk), .q(t433));
 		addfxp_18_1 add99708(.a(t434), .b(t438), .clk(clk), .q(t442));
 		addfxp_18_1 add99715(.a(t435), .b(t439), .clk(clk), .q(t443));
 		subfxp_18_1 sub99722(.a(t434), .b(t438), .clk(clk), .q(t444));
 		subfxp_18_1 sub99729(.a(t435), .b(t439), .clk(clk), .q(t445));
 		subfxp_18_1 sub99752(.a(t436), .b(t441), .clk(clk), .q(t446));
 		addfxp_18_1 add99759(.a(t437), .b(t440), .clk(clk), .q(t447));
 		addfxp_18_1 add99766(.a(t436), .b(t441), .clk(clk), .q(t448));
 		subfxp_18_1 sub99773(.a(t437), .b(t440), .clk(clk), .q(t449));
 		addfxp_18_1 add99916(.a(t450), .b(t454), .clk(clk), .q(t458));
 		addfxp_18_1 add99923(.a(t451), .b(t455), .clk(clk), .q(t459));
 		subfxp_18_1 sub99930(.a(t450), .b(t454), .clk(clk), .q(t460));
 		subfxp_18_1 sub99937(.a(t451), .b(t455), .clk(clk), .q(t461));
 		subfxp_18_1 sub99960(.a(t452), .b(t457), .clk(clk), .q(t462));
 		addfxp_18_1 add99967(.a(t453), .b(t456), .clk(clk), .q(t463));
 		addfxp_18_1 add99974(.a(t452), .b(t457), .clk(clk), .q(t464));
 		subfxp_18_1 sub99981(.a(t453), .b(t456), .clk(clk), .q(t465));

always @(posedge clk) begin
	if (reset == 1) begin
		next <= 1'b0;
	end else begin
		X0 <= X0_in;
		X1 <= X1_in;
		X2 <= X2_in;
		X3 <= X3_in;
		X4 <= X4_in;
		X5 <= X5_in;
		X6 <= X6_in;
		X7 <= X7_in;
		X8 <= X8_in;
		X9 <= X9_in;
		X10 <= X10_in;
		X11 <= X11_in;
		X12 <= X12_in;
		X13 <= X13_in;
		X14 <= X14_in;
		X15 <= X15_in;
		X16 <= X16_in;
		X17 <= X17_in;
		X18 <= X18_in;
		X19 <= X19_in;
		X20 <= X20_in;
		X21 <= X21_in;
		X22 <= X22_in;
		X23 <= X23_in;
		X24 <= X24_in;
		X25 <= X25_in;
		X26 <= X26_in;
		X27 <= X27_in;
		X28 <= X28_in;
		X29 <= X29_in;
		X30 <= X30_in;
		X31 <= X31_in;
		next <= next_in;
	end
end

endmodule

module shiftRegFIFO_2_1 (
	input [0:0] X,
	output [0:0] Y,
	input reset,
	input clk
);

reg [0:0] mem_0;
reg [0:0] mem_1;
assign Y = mem_1;

always @ (posedge clk) begin
	if (reset) begin
		mem_0 <= 0;
		mem_1 <= 0;
	end else begin
		mem_1 <= mem_0;
		mem_0 <= X;
	end
end

endmodule

module shift_register_group_18_16_1 (
	input clk,
	input enable,
	input [17:0] in_0,
	output [17:0] out_0,
	input [17:0] in_1,
	output [17:0] out_1,
	input [17:0] in_2,
	output [17:0] out_2,
	input [17:0] in_3,
	output [17:0] out_3,
	input [17:0] in_4,
	output [17:0] out_4,
	input [17:0] in_5,
	output [17:0] out_5,
	input [17:0] in_6,
	output [17:0] out_6,
	input [17:0] in_7,
	output [17:0] out_7,
	input [17:0] in_8,
	output [17:0] out_8,
	input [17:0] in_9,
	output [17:0] out_9,
	input [17:0] in_10,
	output [17:0] out_10,
	input [17:0] in_11,
	output [17:0] out_11,
	input [17:0] in_12,
	output [17:0] out_12,
	input [17:0] in_13,
	output [17:0] out_13,
	input [17:0] in_14,
	output [17:0] out_14,
	input [17:0] in_15,
	output [17:0] out_15,
	input reset
);

shift_register_unit_18_1 shift_register_unit_18_1_inst_0 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_0),
	.out(out_0)
);

shift_register_unit_18_1 shift_register_unit_18_1_inst_1 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_1),
	.out(out_1)
);

shift_register_unit_18_1 shift_register_unit_18_1_inst_2 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_2),
	.out(out_2)
);

shift_register_unit_18_1 shift_register_unit_18_1_inst_3 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_3),
	.out(out_3)
);

shift_register_unit_18_1 shift_register_unit_18_1_inst_4 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_4),
	.out(out_4)
);

shift_register_unit_18_1 shift_register_unit_18_1_inst_5 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_5),
	.out(out_5)
);

shift_register_unit_18_1 shift_register_unit_18_1_inst_6 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_6),
	.out(out_6)
);

shift_register_unit_18_1 shift_register_unit_18_1_inst_7 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_7),
	.out(out_7)
);

shift_register_unit_18_1 shift_register_unit_18_1_inst_8 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_8),
	.out(out_8)
);

shift_register_unit_18_1 shift_register_unit_18_1_inst_9 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_9),
	.out(out_9)
);

shift_register_unit_18_1 shift_register_unit_18_1_inst_10 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_10),
	.out(out_10)
);

shift_register_unit_18_1 shift_register_unit_18_1_inst_11 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_11),
	.out(out_11)
);

shift_register_unit_18_1 shift_register_unit_18_1_inst_12 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_12),
	.out(out_12)
);

shift_register_unit_18_1 shift_register_unit_18_1_inst_13 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_13),
	.out(out_13)
);

shift_register_unit_18_1 shift_register_unit_18_1_inst_14 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_14),
	.out(out_14)
);

shift_register_unit_18_1 shift_register_unit_18_1_inst_15 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_15),
	.out(out_15)
);

endmodule

module shift_register_unit_18_1 (
	input clk,
	input reset,
	input enable,
	input [17:0] in,
	output [17:0] out
);

reg [17:0] shift_registers_0;
always @ (posedge clk) begin
	if (reset) begin
		shift_registers_0 <= 18'd0;
	end else if (enable) begin
		shift_registers_0 <= in;
	end
end

assign out = shift_registers_0;

endmodule

module c_matrix_vec_mult_core_18_10_16_1_1 (
	input clk,
	input reset,
	input i_ready,
	input i_valid,
	input [17:0] i_X_0,
	input [17:0] i_X_1,
	input [17:0] i_X_2,
	input [17:0] i_X_3,
	input [17:0] i_X_4,
	input [17:0] i_X_5,
	input [17:0] i_X_6,
	input [17:0] i_X_7,
	input [17:0] i_X_8,
	input [17:0] i_X_9,
	input [17:0] i_X_10,
	input [17:0] i_X_11,
	input [17:0] i_X_12,
	input [17:0] i_X_13,
	input [17:0] i_X_14,
	input [17:0] i_X_15,
	input [17:0] i_W_real_0_0,
	input [17:0] i_W_imag_0_0,
	input [17:0] i_W_real_0_1,
	input [17:0] i_W_imag_0_1,
	input [17:0] i_W_real_0_2,
	input [17:0] i_W_imag_0_2,
	input [17:0] i_W_real_0_3,
	input [17:0] i_W_imag_0_3,
	input [17:0] i_W_real_0_4,
	input [17:0] i_W_imag_0_4,
	input [17:0] i_W_real_0_5,
	input [17:0] i_W_imag_0_5,
	input [17:0] i_W_real_0_6,
	input [17:0] i_W_imag_0_6,
	input [17:0] i_W_real_0_7,
	input [17:0] i_W_imag_0_7,
	input [17:0] i_W_real_0_8,
	input [17:0] i_W_imag_0_8,
	output [17:0] o_Y_real_0_0,
	output [17:0] o_Y_imag_0_0,
	output [17:0] o_Y_real_0_1,
	output [17:0] o_Y_imag_0_1,
	output [17:0] o_Y_real_0_2,
	output [17:0] o_Y_imag_0_2,
	output [17:0] o_Y_real_0_3,
	output [17:0] o_Y_imag_0_3,
	output [17:0] o_Y_real_0_4,
	output [17:0] o_Y_imag_0_4,
	output [17:0] o_Y_real_0_5,
	output [17:0] o_Y_imag_0_5,
	output [17:0] o_Y_real_0_6,
	output [17:0] o_Y_imag_0_6,
	output [17:0] o_Y_real_0_7,
	output [17:0] o_Y_imag_0_7,
	output [17:0] o_Y_real_0_8,
	output [17:0] o_Y_imag_0_8,
	output [17:0] o_Y_real_0_9,
	output [17:0] o_Y_imag_0_9,
	output [17:0] o_Y_real_0_10,
	output [17:0] o_Y_imag_0_10,
	output [17:0] o_Y_real_0_11,
	output [17:0] o_Y_imag_0_11,
	output [17:0] o_Y_real_0_12,
	output [17:0] o_Y_imag_0_12,
	output [17:0] o_Y_real_0_13,
	output [17:0] o_Y_imag_0_13,
	output [17:0] o_Y_real_0_14,
	output [17:0] o_Y_imag_0_14,
	output [17:0] o_Y_real_0_15,
	output [17:0] o_Y_imag_0_15,
	output o_ready,
	output o_valid
);

// Enable whenever reciever is ready
wire enable;
assign enable = i_ready;
// Register the inputs
reg [17:0] reg_X_0;
reg [17:0] reg_X_2_0;
reg [17:0] reg_X_1;
reg [17:0] reg_X_2_1;
reg [17:0] reg_X_2;
reg [17:0] reg_X_2_2;
reg [17:0] reg_X_3;
reg [17:0] reg_X_2_3;
reg [17:0] reg_X_4;
reg [17:0] reg_X_2_4;
reg [17:0] reg_X_5;
reg [17:0] reg_X_2_5;
reg [17:0] reg_X_6;
reg [17:0] reg_X_2_6;
reg [17:0] reg_X_7;
reg [17:0] reg_X_2_7;
reg [17:0] reg_X_8;
reg [17:0] reg_X_2_8;
reg [17:0] reg_X_9;
reg [17:0] reg_X_2_9;
reg [17:0] reg_X_10;
reg [17:0] reg_X_2_10;
reg [17:0] reg_X_11;
reg [17:0] reg_X_2_11;
reg [17:0] reg_X_12;
reg [17:0] reg_X_2_12;
reg [17:0] reg_X_13;
reg [17:0] reg_X_2_13;
reg [17:0] reg_X_14;
reg [17:0] reg_X_2_14;
reg [17:0] reg_X_15;
reg [17:0] reg_X_2_15;
reg [17:0] reg_W_real_0_0;
reg [17:0] reg_W_imag_0_0;
reg [17:0] reg_W_real_0_1;
reg [17:0] reg_W_imag_0_1;
reg [17:0] reg_W_real_0_2;
reg [17:0] reg_W_imag_0_2;
reg [17:0] reg_W_real_0_3;
reg [17:0] reg_W_imag_0_3;
reg [17:0] reg_W_real_0_4;
reg [17:0] reg_W_imag_0_4;
reg [17:0] reg_W_real_0_5;
reg [17:0] reg_W_imag_0_5;
reg [17:0] reg_W_real_0_6;
reg [17:0] reg_W_imag_0_6;
reg [17:0] reg_W_real_0_7;
reg [17:0] reg_W_imag_0_7;
reg [17:0] reg_W_real_0_8;
reg [17:0] reg_W_imag_0_8;
reg reg_i_valid;

// Register the outputs
reg [17:0] reg_Y_real_0_0;
reg [17:0] reg_Y_imag_0_0;
reg [17:0] reg_Y_real_0_1;
reg [17:0] reg_Y_imag_0_1;
reg [17:0] reg_Y_real_0_2;
reg [17:0] reg_Y_imag_0_2;
reg [17:0] reg_Y_real_0_3;
reg [17:0] reg_Y_imag_0_3;
reg [17:0] reg_Y_real_0_4;
reg [17:0] reg_Y_imag_0_4;
reg [17:0] reg_Y_real_0_5;
reg [17:0] reg_Y_imag_0_5;
reg [17:0] reg_Y_real_0_6;
reg [17:0] reg_Y_imag_0_6;
reg [17:0] reg_Y_real_0_7;
reg [17:0] reg_Y_imag_0_7;
reg [17:0] reg_Y_real_0_8;
reg [17:0] reg_Y_imag_0_8;
reg [17:0] reg_Y_real_0_9;
reg [17:0] reg_Y_imag_0_9;
reg [17:0] reg_Y_real_0_10;
reg [17:0] reg_Y_imag_0_10;
reg [17:0] reg_Y_real_0_11;
reg [17:0] reg_Y_imag_0_11;
reg [17:0] reg_Y_real_0_12;
reg [17:0] reg_Y_imag_0_12;
reg [17:0] reg_Y_real_0_13;
reg [17:0] reg_Y_imag_0_13;
reg [17:0] reg_Y_real_0_14;
reg [17:0] reg_Y_imag_0_14;
reg [17:0] reg_Y_real_0_15;
reg [17:0] reg_Y_imag_0_15;
reg reg_o_valid;

// Inter-connections
reg fft_valid;
reg reg_o_ready;
wire o_fft_next;
wire mult_X_real_W_real_valid_0;
wire mult_X_imag_W_real_valid_0;
wire mult_X_real_W_imag_valid_0;
wire mult_X_imag_W_imag_valid_0;
wire sub_y_real_valid_0;
wire add_y_imag_valid_0;

wire [17:0] W_real_wires_0_0;
wire [17:0] W_imag_wires_0_0;
wire [17:0] W_real_holder_0_0;
wire [17:0] W_imag_holder_0_0;
wire [17:0] o_mult_X_real_W_real_0_0;
wire [17:0] o_mult_X_imag_W_real_0_0;
wire [17:0] o_mult_X_real_W_imag_0_0;
wire [17:0] o_mult_X_imag_W_imag_0_0;
wire [17:0] o_sub_y_real_0_0;
wire [17:0] o_add_y_imag_0_0;
wire [17:0] W_real_wires_0_1;
wire [17:0] W_imag_wires_0_1;
wire [17:0] W_real_holder_0_1;
wire [17:0] W_imag_holder_0_1;
wire [17:0] o_mult_X_real_W_real_0_1;
wire [17:0] o_mult_X_imag_W_real_0_1;
wire [17:0] o_mult_X_real_W_imag_0_1;
wire [17:0] o_mult_X_imag_W_imag_0_1;
wire [17:0] o_sub_y_real_0_1;
wire [17:0] o_add_y_imag_0_1;
wire [17:0] W_real_wires_0_2;
wire [17:0] W_imag_wires_0_2;
wire [17:0] W_real_holder_0_2;
wire [17:0] W_imag_holder_0_2;
wire [17:0] o_mult_X_real_W_real_0_2;
wire [17:0] o_mult_X_imag_W_real_0_2;
wire [17:0] o_mult_X_real_W_imag_0_2;
wire [17:0] o_mult_X_imag_W_imag_0_2;
wire [17:0] o_sub_y_real_0_2;
wire [17:0] o_add_y_imag_0_2;
wire [17:0] W_real_wires_0_3;
wire [17:0] W_imag_wires_0_3;
wire [17:0] W_real_holder_0_3;
wire [17:0] W_imag_holder_0_3;
wire [17:0] o_mult_X_real_W_real_0_3;
wire [17:0] o_mult_X_imag_W_real_0_3;
wire [17:0] o_mult_X_real_W_imag_0_3;
wire [17:0] o_mult_X_imag_W_imag_0_3;
wire [17:0] o_sub_y_real_0_3;
wire [17:0] o_add_y_imag_0_3;
wire [17:0] W_real_wires_0_4;
wire [17:0] W_imag_wires_0_4;
wire [17:0] W_real_holder_0_4;
wire [17:0] W_imag_holder_0_4;
wire [17:0] o_mult_X_real_W_real_0_4;
wire [17:0] o_mult_X_imag_W_real_0_4;
wire [17:0] o_mult_X_real_W_imag_0_4;
wire [17:0] o_mult_X_imag_W_imag_0_4;
wire [17:0] o_sub_y_real_0_4;
wire [17:0] o_add_y_imag_0_4;
wire [17:0] W_real_wires_0_5;
wire [17:0] W_imag_wires_0_5;
wire [17:0] W_real_holder_0_5;
wire [17:0] W_imag_holder_0_5;
wire [17:0] o_mult_X_real_W_real_0_5;
wire [17:0] o_mult_X_imag_W_real_0_5;
wire [17:0] o_mult_X_real_W_imag_0_5;
wire [17:0] o_mult_X_imag_W_imag_0_5;
wire [17:0] o_sub_y_real_0_5;
wire [17:0] o_add_y_imag_0_5;
wire [17:0] W_real_wires_0_6;
wire [17:0] W_imag_wires_0_6;
wire [17:0] W_real_holder_0_6;
wire [17:0] W_imag_holder_0_6;
wire [17:0] o_mult_X_real_W_real_0_6;
wire [17:0] o_mult_X_imag_W_real_0_6;
wire [17:0] o_mult_X_real_W_imag_0_6;
wire [17:0] o_mult_X_imag_W_imag_0_6;
wire [17:0] o_sub_y_real_0_6;
wire [17:0] o_add_y_imag_0_6;
wire [17:0] W_real_wires_0_7;
wire [17:0] W_imag_wires_0_7;
wire [17:0] W_real_holder_0_7;
wire [17:0] W_imag_holder_0_7;
wire [17:0] o_mult_X_real_W_real_0_7;
wire [17:0] o_mult_X_imag_W_real_0_7;
wire [17:0] o_mult_X_real_W_imag_0_7;
wire [17:0] o_mult_X_imag_W_imag_0_7;
wire [17:0] o_sub_y_real_0_7;
wire [17:0] o_add_y_imag_0_7;
wire [17:0] W_real_wires_0_8;
wire [17:0] W_imag_wires_0_8;
wire [17:0] W_real_holder_0_8;
wire [17:0] W_imag_holder_0_8;
wire [17:0] o_mult_X_real_W_real_0_8;
wire [17:0] o_mult_X_imag_W_real_0_8;
wire [17:0] o_mult_X_real_W_imag_0_8;
wire [17:0] o_mult_X_imag_W_imag_0_8;
wire [17:0] o_sub_y_real_0_8;
wire [17:0] o_add_y_imag_0_8;
wire [17:0] o_fft_X_real_0;
wire [17:0] o_fft_X_imag_0;
wire [17:0] o_fft_X_real_1;
wire [17:0] o_fft_X_imag_1;
wire [17:0] o_fft_X_real_2;
wire [17:0] o_fft_X_imag_2;
wire [17:0] o_fft_X_real_3;
wire [17:0] o_fft_X_imag_3;
wire [17:0] o_fft_X_real_4;
wire [17:0] o_fft_X_imag_4;
wire [17:0] o_fft_X_real_5;
wire [17:0] o_fft_X_imag_5;
wire [17:0] o_fft_X_real_6;
wire [17:0] o_fft_X_imag_6;
wire [17:0] o_fft_X_real_7;
wire [17:0] o_fft_X_imag_7;
wire [17:0] o_fft_X_real_8;
wire [17:0] o_fft_X_imag_8;
wire [17:0] o_fft_X_real_9;
wire [17:0] o_fft_X_imag_9;
wire [17:0] o_fft_X_real_10;
wire [17:0] o_fft_X_imag_10;
wire [17:0] o_fft_X_real_11;
wire [17:0] o_fft_X_imag_11;
wire [17:0] o_fft_X_real_12;
wire [17:0] o_fft_X_imag_12;
wire [17:0] o_fft_X_real_13;
wire [17:0] o_fft_X_imag_13;
wire [17:0] o_fft_X_real_14;
wire [17:0] o_fft_X_imag_14;
wire [17:0] o_fft_X_real_15;
wire [17:0] o_fft_X_imag_15;

// Hold weights value until X_FFT finishes
assign W_real_wires_0_0 = reg_W_real_0_0;
assign W_imag_wires_0_0 = reg_W_imag_0_0;
assign W_real_wires_0_1 = reg_W_real_0_1;
assign W_imag_wires_0_1 = reg_W_imag_0_1;
assign W_real_wires_0_2 = reg_W_real_0_2;
assign W_imag_wires_0_2 = reg_W_imag_0_2;
assign W_real_wires_0_3 = reg_W_real_0_3;
assign W_imag_wires_0_3 = reg_W_imag_0_3;
assign W_real_wires_0_4 = reg_W_real_0_4;
assign W_imag_wires_0_4 = reg_W_imag_0_4;
assign W_real_wires_0_5 = reg_W_real_0_5;
assign W_imag_wires_0_5 = reg_W_imag_0_5;
assign W_real_wires_0_6 = reg_W_real_0_6;
assign W_imag_wires_0_6 = reg_W_imag_0_6;
assign W_real_wires_0_7 = reg_W_real_0_7;
assign W_imag_wires_0_7 = reg_W_imag_0_7;
assign W_real_wires_0_8 = reg_W_real_0_8;
assign W_imag_wires_0_8 = reg_W_imag_0_8;

shift_register_group_18_910 shift_register_group_18_910_inst_0_real (
	.clk(clk),
	.enable(enable),
	.in_0(W_real_wires_0_0),
	.out_0(W_real_holder_0_0),
	.in_1(W_real_wires_0_1),
	.out_1(W_real_holder_0_1),
	.in_2(W_real_wires_0_2),
	.out_2(W_real_holder_0_2),
	.in_3(W_real_wires_0_3),
	.out_3(W_real_holder_0_3),
	.in_4(W_real_wires_0_4),
	.out_4(W_real_holder_0_4),
	.in_5(W_real_wires_0_5),
	.out_5(W_real_holder_0_5),
	.in_6(W_real_wires_0_6),
	.out_6(W_real_holder_0_6),
	.in_7(W_real_wires_0_7),
	.out_7(W_real_holder_0_7),
	.in_8(W_real_wires_0_8),
	.out_8(W_real_holder_0_8),
	.reset(reset)
);

shift_register_group_18_910 shift_register_group_18_910_inst_0_imag (
	.clk(clk),
	.enable(enable),
	.in_0(W_imag_wires_0_0),
	.out_0(W_imag_holder_0_0),
	.in_1(W_imag_wires_0_1),
	.out_1(W_imag_holder_0_1),
	.in_2(W_imag_wires_0_2),
	.out_2(W_imag_holder_0_2),
	.in_3(W_imag_wires_0_3),
	.out_3(W_imag_holder_0_3),
	.in_4(W_imag_wires_0_4),
	.out_4(W_imag_holder_0_4),
	.in_5(W_imag_wires_0_5),
	.out_5(W_imag_holder_0_5),
	.in_6(W_imag_wires_0_6),
	.out_6(W_imag_holder_0_6),
	.in_7(W_imag_wires_0_7),
	.out_7(W_imag_holder_0_7),
	.in_8(W_imag_wires_0_8),
	.out_8(W_imag_holder_0_8),
	.reset(reset)
);

dft_16_top_18 dft_16_top_18_inst (
	.clk(clk),
	.reset(reset),
	.next(reg_i_valid),
	.X0(reg_X_2_0),
	.Y0(o_fft_X_real_0),
	.X1(0),
	.Y1(o_fft_X_imag_0),
	.X2(reg_X_2_1),
	.Y2(o_fft_X_real_1),
	.X3(0),
	.Y3(o_fft_X_imag_1),
	.X4(reg_X_2_2),
	.Y4(o_fft_X_real_2),
	.X5(0),
	.Y5(o_fft_X_imag_2),
	.X6(reg_X_2_3),
	.Y6(o_fft_X_real_3),
	.X7(0),
	.Y7(o_fft_X_imag_3),
	.X8(reg_X_2_4),
	.Y8(o_fft_X_real_4),
	.X9(0),
	.Y9(o_fft_X_imag_4),
	.X10(reg_X_2_5),
	.Y10(o_fft_X_real_5),
	.X11(0),
	.Y11(o_fft_X_imag_5),
	.X12(reg_X_2_6),
	.Y12(o_fft_X_real_6),
	.X13(0),
	.Y13(o_fft_X_imag_6),
	.X14(reg_X_2_7),
	.Y14(o_fft_X_real_7),
	.X15(0),
	.Y15(o_fft_X_imag_7),
	.X16(reg_X_2_8),
	.Y16(o_fft_X_real_8),
	.X17(0),
	.Y17(o_fft_X_imag_8),
	.X18(reg_X_2_9),
	.Y18(o_fft_X_real_9),
	.X19(0),
	.Y19(o_fft_X_imag_9),
	.X20(reg_X_2_10),
	.Y20(o_fft_X_real_10),
	.X21(0),
	.Y21(o_fft_X_imag_10),
	.X22(reg_X_2_11),
	.Y22(o_fft_X_real_11),
	.X23(0),
	.Y23(o_fft_X_imag_11),
	.X24(reg_X_2_12),
	.Y24(o_fft_X_real_12),
	.X25(0),
	.Y25(o_fft_X_imag_12),
	.X26(reg_X_2_13),
	.Y26(o_fft_X_real_13),
	.X27(0),
	.Y27(o_fft_X_imag_13),
	.X28(reg_X_2_14),
	.Y28(o_fft_X_real_14),
	.X29(0),
	.Y29(o_fft_X_imag_14),
	.X30(reg_X_2_15),
	.Y30(o_fft_X_real_15),
	.X31(0),
	.Y31(o_fft_X_imag_15),
	.next_out(o_fft_next)
);
elementwise_mult_core_18_1810_9_1 elementwise_mult_core_18_1810_9_1_inst_0_real_real (
	.clk(clk),
	.reset(reset),
	.i_valid(fft_valid),
	.i_ready(enable),
	.i_A_0(W_real_holder_0_0),
	.i_B_0(o_fft_X_real_0),
	.o_C_0(o_mult_X_real_W_real_0_0),
	.i_A_1(W_real_holder_0_1),
	.i_B_1(o_fft_X_real_1),
	.o_C_1(o_mult_X_real_W_real_0_1),
	.i_A_2(W_real_holder_0_2),
	.i_B_2(o_fft_X_real_2),
	.o_C_2(o_mult_X_real_W_real_0_2),
	.i_A_3(W_real_holder_0_3),
	.i_B_3(o_fft_X_real_3),
	.o_C_3(o_mult_X_real_W_real_0_3),
	.i_A_4(W_real_holder_0_4),
	.i_B_4(o_fft_X_real_4),
	.o_C_4(o_mult_X_real_W_real_0_4),
	.i_A_5(W_real_holder_0_5),
	.i_B_5(o_fft_X_real_5),
	.o_C_5(o_mult_X_real_W_real_0_5),
	.i_A_6(W_real_holder_0_6),
	.i_B_6(o_fft_X_real_6),
	.o_C_6(o_mult_X_real_W_real_0_6),
	.i_A_7(W_real_holder_0_7),
	.i_B_7(o_fft_X_real_7),
	.o_C_7(o_mult_X_real_W_real_0_7),
	.i_A_8(W_real_holder_0_8),
	.i_B_8(o_fft_X_real_8),
	.o_C_8(o_mult_X_real_W_real_0_8),
	.o_valid(mult_X_real_W_real_valid_0),
	.o_ready()
);

elementwise_mult_core_18_1810_9_1 elementwise_mult_core_18_1810_9_1_inst_0_real_imag (
	.clk(clk),
	.reset(reset),
	.i_valid(fft_valid),
	.i_ready(enable),
	.i_A_0(W_imag_holder_0_0),
	.i_B_0(o_fft_X_real_0),
	.o_C_0(o_mult_X_real_W_imag_0_0),
	.i_A_1(W_imag_holder_0_1),
	.i_B_1(o_fft_X_real_1),
	.o_C_1(o_mult_X_real_W_imag_0_1),
	.i_A_2(W_imag_holder_0_2),
	.i_B_2(o_fft_X_real_2),
	.o_C_2(o_mult_X_real_W_imag_0_2),
	.i_A_3(W_imag_holder_0_3),
	.i_B_3(o_fft_X_real_3),
	.o_C_3(o_mult_X_real_W_imag_0_3),
	.i_A_4(W_imag_holder_0_4),
	.i_B_4(o_fft_X_real_4),
	.o_C_4(o_mult_X_real_W_imag_0_4),
	.i_A_5(W_imag_holder_0_5),
	.i_B_5(o_fft_X_real_5),
	.o_C_5(o_mult_X_real_W_imag_0_5),
	.i_A_6(W_imag_holder_0_6),
	.i_B_6(o_fft_X_real_6),
	.o_C_6(o_mult_X_real_W_imag_0_6),
	.i_A_7(W_imag_holder_0_7),
	.i_B_7(o_fft_X_real_7),
	.o_C_7(o_mult_X_real_W_imag_0_7),
	.i_A_8(W_imag_holder_0_8),
	.i_B_8(o_fft_X_real_8),
	.o_C_8(o_mult_X_real_W_imag_0_8),
	.o_valid(mult_X_real_W_imag_valid_0),
	.o_ready()
);

elementwise_mult_core_18_1810_9_1 elementwise_mult_core_18_1810_9_1_inst_0_imag_real (
	.clk(clk),
	.reset(reset),
	.i_valid(fft_valid),
	.i_ready(enable),
	.i_A_0(o_fft_X_imag_0),
	.i_B_0(W_real_holder_0_0),
	.o_C_0(o_mult_X_imag_W_real_0_0),
	.i_A_1(o_fft_X_imag_1),
	.i_B_1(W_real_holder_0_1),
	.o_C_1(o_mult_X_imag_W_real_0_1),
	.i_A_2(o_fft_X_imag_2),
	.i_B_2(W_real_holder_0_2),
	.o_C_2(o_mult_X_imag_W_real_0_2),
	.i_A_3(o_fft_X_imag_3),
	.i_B_3(W_real_holder_0_3),
	.o_C_3(o_mult_X_imag_W_real_0_3),
	.i_A_4(o_fft_X_imag_4),
	.i_B_4(W_real_holder_0_4),
	.o_C_4(o_mult_X_imag_W_real_0_4),
	.i_A_5(o_fft_X_imag_5),
	.i_B_5(W_real_holder_0_5),
	.o_C_5(o_mult_X_imag_W_real_0_5),
	.i_A_6(o_fft_X_imag_6),
	.i_B_6(W_real_holder_0_6),
	.o_C_6(o_mult_X_imag_W_real_0_6),
	.i_A_7(o_fft_X_imag_7),
	.i_B_7(W_real_holder_0_7),
	.o_C_7(o_mult_X_imag_W_real_0_7),
	.i_A_8(o_fft_X_imag_8),
	.i_B_8(W_real_holder_0_8),
	.o_C_8(o_mult_X_imag_W_real_0_8),
	.o_valid(mult_X_imag_W_real_valid_0),
	.o_ready()
);

elementwise_mult_core_18_1810_9_1 elementwise_mult_core_18_1810_9_1_inst_0_imag_imag (
	.clk(clk),
	.reset(reset),
	.i_valid(fft_valid),
	.i_ready(enable),
	.i_A_0(o_fft_X_imag_0),
	.i_B_0(W_imag_holder_0_0),
	.o_C_0(o_mult_X_imag_W_imag_0_0),
	.i_A_1(o_fft_X_imag_1),
	.i_B_1(W_imag_holder_0_1),
	.o_C_1(o_mult_X_imag_W_imag_0_1),
	.i_A_2(o_fft_X_imag_2),
	.i_B_2(W_imag_holder_0_2),
	.o_C_2(o_mult_X_imag_W_imag_0_2),
	.i_A_3(o_fft_X_imag_3),
	.i_B_3(W_imag_holder_0_3),
	.o_C_3(o_mult_X_imag_W_imag_0_3),
	.i_A_4(o_fft_X_imag_4),
	.i_B_4(W_imag_holder_0_4),
	.o_C_4(o_mult_X_imag_W_imag_0_4),
	.i_A_5(o_fft_X_imag_5),
	.i_B_5(W_imag_holder_0_5),
	.o_C_5(o_mult_X_imag_W_imag_0_5),
	.i_A_6(o_fft_X_imag_6),
	.i_B_6(W_imag_holder_0_6),
	.o_C_6(o_mult_X_imag_W_imag_0_6),
	.i_A_7(o_fft_X_imag_7),
	.i_B_7(W_imag_holder_0_7),
	.o_C_7(o_mult_X_imag_W_imag_0_7),
	.i_A_8(o_fft_X_imag_8),
	.i_B_8(W_imag_holder_0_8),
	.o_C_8(o_mult_X_imag_W_imag_0_8),
	.o_valid(mult_X_imag_W_imag_valid_0),
	.o_ready()
);

wire sub_core_valid_0;
assign sub_core_valid_0 = mult_X_real_W_real_valid_0 & mult_X_imag_W_imag_valid_0;

elementwise_sub_core_18_18_9 elementwise_sub_core_18_18_9_inst_0 (
	.clk(clk),
	.reset(reset),
	.i_valid(sub_core_valid_0),
	.i_ready(enable),
	.i_A_0(o_mult_X_real_W_real_0_0),
	.i_B_0(o_mult_X_imag_W_imag_0_0),
	.o_C_0(o_sub_y_real_0_0),
	.i_A_1(o_mult_X_real_W_real_0_1),
	.i_B_1(o_mult_X_imag_W_imag_0_1),
	.o_C_1(o_sub_y_real_0_1),
	.i_A_2(o_mult_X_real_W_real_0_2),
	.i_B_2(o_mult_X_imag_W_imag_0_2),
	.o_C_2(o_sub_y_real_0_2),
	.i_A_3(o_mult_X_real_W_real_0_3),
	.i_B_3(o_mult_X_imag_W_imag_0_3),
	.o_C_3(o_sub_y_real_0_3),
	.i_A_4(o_mult_X_real_W_real_0_4),
	.i_B_4(o_mult_X_imag_W_imag_0_4),
	.o_C_4(o_sub_y_real_0_4),
	.i_A_5(o_mult_X_real_W_real_0_5),
	.i_B_5(o_mult_X_imag_W_imag_0_5),
	.o_C_5(o_sub_y_real_0_5),
	.i_A_6(o_mult_X_real_W_real_0_6),
	.i_B_6(o_mult_X_imag_W_imag_0_6),
	.o_C_6(o_sub_y_real_0_6),
	.i_A_7(o_mult_X_real_W_real_0_7),
	.i_B_7(o_mult_X_imag_W_imag_0_7),
	.o_C_7(o_sub_y_real_0_7),
	.i_A_8(o_mult_X_real_W_real_0_8),
	.i_B_8(o_mult_X_imag_W_imag_0_8),
	.o_C_8(o_sub_y_real_0_8),
	.o_valid(sub_y_real_valid_0),
	.o_ready()
);

wire add_core_valid_0;
assign add_core_valid_0 = mult_X_real_W_imag_valid_0 & mult_X_imag_W_real_valid_0;

elementwise_add_core_18_18_9 elementwise_add_core_18_18_9_inst_0 (
	.clk(clk),
	.reset(reset),
	.i_valid(add_core_valid_0),
	.i_ready(enable),
	.i_A_0(o_mult_X_real_W_imag_0_0),
	.i_B_0(o_mult_X_imag_W_real_0_0),
	.o_C_0(o_add_y_imag_0_0),
	.i_A_1(o_mult_X_real_W_imag_0_1),
	.i_B_1(o_mult_X_imag_W_real_0_1),
	.o_C_1(o_add_y_imag_0_1),
	.i_A_2(o_mult_X_real_W_imag_0_2),
	.i_B_2(o_mult_X_imag_W_real_0_2),
	.o_C_2(o_add_y_imag_0_2),
	.i_A_3(o_mult_X_real_W_imag_0_3),
	.i_B_3(o_mult_X_imag_W_real_0_3),
	.o_C_3(o_add_y_imag_0_3),
	.i_A_4(o_mult_X_real_W_imag_0_4),
	.i_B_4(o_mult_X_imag_W_real_0_4),
	.o_C_4(o_add_y_imag_0_4),
	.i_A_5(o_mult_X_real_W_imag_0_5),
	.i_B_5(o_mult_X_imag_W_real_0_5),
	.o_C_5(o_add_y_imag_0_5),
	.i_A_6(o_mult_X_real_W_imag_0_6),
	.i_B_6(o_mult_X_imag_W_real_0_6),
	.o_C_6(o_add_y_imag_0_6),
	.i_A_7(o_mult_X_real_W_imag_0_7),
	.i_B_7(o_mult_X_imag_W_real_0_7),
	.o_C_7(o_add_y_imag_0_7),
	.i_A_8(o_mult_X_real_W_imag_0_8),
	.i_B_8(o_mult_X_imag_W_real_0_8),
	.o_C_8(o_add_y_imag_0_8),
	.o_valid(add_y_imag_valid_0),
	.o_ready()
);

always @ (posedge clk) begin 
	if (reset) begin
		reg_i_valid <= 1'b0;
		fft_valid <= 1'b0;
		reg_o_valid <= 1'b0;
		reg_o_ready <= 1'b0;
		reg_X_0 <= 0;
		reg_X_2_0 <= 0;
		reg_X_1 <= 0;
		reg_X_2_1 <= 0;
		reg_X_2 <= 0;
		reg_X_2_2 <= 0;
		reg_X_3 <= 0;
		reg_X_2_3 <= 0;
		reg_X_4 <= 0;
		reg_X_2_4 <= 0;
		reg_X_5 <= 0;
		reg_X_2_5 <= 0;
		reg_X_6 <= 0;
		reg_X_2_6 <= 0;
		reg_X_7 <= 0;
		reg_X_2_7 <= 0;
		reg_X_8 <= 0;
		reg_X_2_8 <= 0;
		reg_X_9 <= 0;
		reg_X_2_9 <= 0;
		reg_X_10 <= 0;
		reg_X_2_10 <= 0;
		reg_X_11 <= 0;
		reg_X_2_11 <= 0;
		reg_X_12 <= 0;
		reg_X_2_12 <= 0;
		reg_X_13 <= 0;
		reg_X_2_13 <= 0;
		reg_X_14 <= 0;
		reg_X_2_14 <= 0;
		reg_X_15 <= 0;
		reg_X_2_15 <= 0;
		reg_W_real_0_0 <= 0;
		reg_W_imag_0_0 <= 0;
		reg_W_real_0_1 <= 0;
		reg_W_imag_0_1 <= 0;
		reg_W_real_0_2 <= 0;
		reg_W_imag_0_2 <= 0;
		reg_W_real_0_3 <= 0;
		reg_W_imag_0_3 <= 0;
		reg_W_real_0_4 <= 0;
		reg_W_imag_0_4 <= 0;
		reg_W_real_0_5 <= 0;
		reg_W_imag_0_5 <= 0;
		reg_W_real_0_6 <= 0;
		reg_W_imag_0_6 <= 0;
		reg_W_real_0_7 <= 0;
		reg_W_imag_0_7 <= 0;
		reg_W_real_0_8 <= 0;
		reg_W_imag_0_8 <= 0;
		reg_Y_real_0_0 <= 0;
		reg_Y_imag_0_0 <= 0;
		reg_Y_real_0_1 <= 0;
		reg_Y_imag_0_1 <= 0;
		reg_Y_real_0_2 <= 0;
		reg_Y_imag_0_2 <= 0;
		reg_Y_real_0_3 <= 0;
		reg_Y_imag_0_3 <= 0;
		reg_Y_real_0_4 <= 0;
		reg_Y_imag_0_4 <= 0;
		reg_Y_real_0_5 <= 0;
		reg_Y_imag_0_5 <= 0;
		reg_Y_real_0_6 <= 0;
		reg_Y_imag_0_6 <= 0;
		reg_Y_real_0_7 <= 0;
		reg_Y_imag_0_7 <= 0;
		reg_Y_real_0_8 <= 0;
		reg_Y_imag_0_8 <= 0;
		reg_Y_real_0_9 <= 0;
		reg_Y_imag_0_9 <= 0;
		reg_Y_real_0_10 <= 0;
		reg_Y_imag_0_10 <= 0;
		reg_Y_real_0_11 <= 0;
		reg_Y_imag_0_11 <= 0;
		reg_Y_real_0_12 <= 0;
		reg_Y_imag_0_12 <= 0;
		reg_Y_real_0_13 <= 0;
		reg_Y_imag_0_13 <= 0;
		reg_Y_real_0_14 <= 0;
		reg_Y_imag_0_14 <= 0;
		reg_Y_real_0_15 <= 0;
		reg_Y_imag_0_15 <= 0;
	end else if (enable) begin
		reg_i_valid <= i_valid;
		fft_valid <= o_fft_next;
		reg_o_valid <= add_y_imag_valid_0 & sub_y_real_valid_0;
		reg_o_ready <= ~i_valid & enable;
		reg_X_0 <= i_X_0;
		reg_X_2_0 <= reg_X_0;
		reg_X_1 <= i_X_1;
		reg_X_2_1 <= reg_X_1;
		reg_X_2 <= i_X_2;
		reg_X_2_2 <= reg_X_2;
		reg_X_3 <= i_X_3;
		reg_X_2_3 <= reg_X_3;
		reg_X_4 <= i_X_4;
		reg_X_2_4 <= reg_X_4;
		reg_X_5 <= i_X_5;
		reg_X_2_5 <= reg_X_5;
		reg_X_6 <= i_X_6;
		reg_X_2_6 <= reg_X_6;
		reg_X_7 <= i_X_7;
		reg_X_2_7 <= reg_X_7;
		reg_X_8 <= i_X_8;
		reg_X_2_8 <= reg_X_8;
		reg_X_9 <= i_X_9;
		reg_X_2_9 <= reg_X_9;
		reg_X_10 <= i_X_10;
		reg_X_2_10 <= reg_X_10;
		reg_X_11 <= i_X_11;
		reg_X_2_11 <= reg_X_11;
		reg_X_12 <= i_X_12;
		reg_X_2_12 <= reg_X_12;
		reg_X_13 <= i_X_13;
		reg_X_2_13 <= reg_X_13;
		reg_X_14 <= i_X_14;
		reg_X_2_14 <= reg_X_14;
		reg_X_15 <= i_X_15;
		reg_X_2_15 <= reg_X_15;
		reg_W_real_0_0 <= i_W_real_0_0;
		reg_W_imag_0_0 <= i_W_imag_0_0;
		reg_W_real_0_1 <= i_W_real_0_1;
		reg_W_imag_0_1 <= i_W_imag_0_1;
		reg_W_real_0_2 <= i_W_real_0_2;
		reg_W_imag_0_2 <= i_W_imag_0_2;
		reg_W_real_0_3 <= i_W_real_0_3;
		reg_W_imag_0_3 <= i_W_imag_0_3;
		reg_W_real_0_4 <= i_W_real_0_4;
		reg_W_imag_0_4 <= i_W_imag_0_4;
		reg_W_real_0_5 <= i_W_real_0_5;
		reg_W_imag_0_5 <= i_W_imag_0_5;
		reg_W_real_0_6 <= i_W_real_0_6;
		reg_W_imag_0_6 <= i_W_imag_0_6;
		reg_W_real_0_7 <= i_W_real_0_7;
		reg_W_imag_0_7 <= i_W_imag_0_7;
		reg_W_real_0_8 <= i_W_real_0_8;
		reg_W_imag_0_8 <= i_W_imag_0_8;
		reg_Y_real_0_0 <= o_sub_y_real_0_0;
		reg_Y_imag_0_0 <= o_add_y_imag_0_0;
		reg_Y_real_0_1 <= o_sub_y_real_0_1;
		reg_Y_imag_0_1 <= o_add_y_imag_0_1;
		reg_Y_real_0_2 <= o_sub_y_real_0_2;
		reg_Y_imag_0_2 <= o_add_y_imag_0_2;
		reg_Y_real_0_3 <= o_sub_y_real_0_3;
		reg_Y_imag_0_3 <= o_add_y_imag_0_3;
		reg_Y_real_0_4 <= o_sub_y_real_0_4;
		reg_Y_imag_0_4 <= o_add_y_imag_0_4;
		reg_Y_real_0_5 <= o_sub_y_real_0_5;
		reg_Y_imag_0_5 <= o_add_y_imag_0_5;
		reg_Y_real_0_6 <= o_sub_y_real_0_6;
		reg_Y_imag_0_6 <= o_add_y_imag_0_6;
		reg_Y_real_0_7 <= o_sub_y_real_0_7;
		reg_Y_imag_0_7 <= o_add_y_imag_0_7;
		reg_Y_real_0_8 <= o_sub_y_real_0_8;
		reg_Y_imag_0_8 <= o_add_y_imag_0_8;
		reg_Y_real_0_9 <= o_sub_y_real_0_7;
		reg_Y_imag_0_9 <= -o_add_y_imag_0_7;
		reg_Y_real_0_10 <= o_sub_y_real_0_6;
		reg_Y_imag_0_10 <= -o_add_y_imag_0_6;
		reg_Y_real_0_11 <= o_sub_y_real_0_5;
		reg_Y_imag_0_11 <= -o_add_y_imag_0_5;
		reg_Y_real_0_12 <= o_sub_y_real_0_4;
		reg_Y_imag_0_12 <= -o_add_y_imag_0_4;
		reg_Y_real_0_13 <= o_sub_y_real_0_3;
		reg_Y_imag_0_13 <= -o_add_y_imag_0_3;
		reg_Y_real_0_14 <= o_sub_y_real_0_2;
		reg_Y_imag_0_14 <= -o_add_y_imag_0_2;
		reg_Y_real_0_15 <= o_sub_y_real_0_1;
		reg_Y_imag_0_15 <= -o_add_y_imag_0_1;
	end
end

assign o_ready = reg_o_ready & i_ready;
assign o_valid = reg_o_valid;
assign o_Y_real_0_0 = reg_Y_real_0_0;
assign o_Y_imag_0_0 = reg_Y_imag_0_0;
assign o_Y_real_0_1 = reg_Y_real_0_1;
assign o_Y_imag_0_1 = reg_Y_imag_0_1;
assign o_Y_real_0_2 = reg_Y_real_0_2;
assign o_Y_imag_0_2 = reg_Y_imag_0_2;
assign o_Y_real_0_3 = reg_Y_real_0_3;
assign o_Y_imag_0_3 = reg_Y_imag_0_3;
assign o_Y_real_0_4 = reg_Y_real_0_4;
assign o_Y_imag_0_4 = reg_Y_imag_0_4;
assign o_Y_real_0_5 = reg_Y_real_0_5;
assign o_Y_imag_0_5 = reg_Y_imag_0_5;
assign o_Y_real_0_6 = reg_Y_real_0_6;
assign o_Y_imag_0_6 = reg_Y_imag_0_6;
assign o_Y_real_0_7 = reg_Y_real_0_7;
assign o_Y_imag_0_7 = reg_Y_imag_0_7;
assign o_Y_real_0_8 = reg_Y_real_0_8;
assign o_Y_imag_0_8 = reg_Y_imag_0_8;
assign o_Y_real_0_9 = reg_Y_real_0_9;
assign o_Y_imag_0_9 = reg_Y_imag_0_9;
assign o_Y_real_0_10 = reg_Y_real_0_10;
assign o_Y_imag_0_10 = reg_Y_imag_0_10;
assign o_Y_real_0_11 = reg_Y_real_0_11;
assign o_Y_imag_0_11 = reg_Y_imag_0_11;
assign o_Y_real_0_12 = reg_Y_real_0_12;
assign o_Y_imag_0_12 = reg_Y_imag_0_12;
assign o_Y_real_0_13 = reg_Y_real_0_13;
assign o_Y_imag_0_13 = reg_Y_imag_0_13;
assign o_Y_real_0_14 = reg_Y_real_0_14;
assign o_Y_imag_0_14 = reg_Y_imag_0_14;
assign o_Y_real_0_15 = reg_Y_real_0_15;
assign o_Y_imag_0_15 = reg_Y_imag_0_15;

endmodule

module shift_register_group_18_910 (
	input clk,
	input enable,
	input [17:0] in_0,
	output [17:0] out_0,
	input [17:0] in_1,
	output [17:0] out_1,
	input [17:0] in_2,
	output [17:0] out_2,
	input [17:0] in_3,
	output [17:0] out_3,
	input [17:0] in_4,
	output [17:0] out_4,
	input [17:0] in_5,
	output [17:0] out_5,
	input [17:0] in_6,
	output [17:0] out_6,
	input [17:0] in_7,
	output [17:0] out_7,
	input [17:0] in_8,
	output [17:0] out_8,
	input reset
);

shift_register_unit_18_10 shift_register_unit_18_10_inst_0 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_0),
	.out(out_0)
);

shift_register_unit_18_10 shift_register_unit_18_10_inst_1 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_1),
	.out(out_1)
);

shift_register_unit_18_10 shift_register_unit_18_10_inst_2 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_2),
	.out(out_2)
);

shift_register_unit_18_10 shift_register_unit_18_10_inst_3 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_3),
	.out(out_3)
);

shift_register_unit_18_10 shift_register_unit_18_10_inst_4 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_4),
	.out(out_4)
);

shift_register_unit_18_10 shift_register_unit_18_10_inst_5 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_5),
	.out(out_5)
);

shift_register_unit_18_10 shift_register_unit_18_10_inst_6 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_6),
	.out(out_6)
);

shift_register_unit_18_10 shift_register_unit_18_10_inst_7 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_7),
	.out(out_7)
);

shift_register_unit_18_10 shift_register_unit_18_10_inst_8 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_8),
	.out(out_8)
);

endmodule

module shift_register_unit_18_10 (
	input clk,
	input reset,
	input enable,
	input [17:0] in,
	output [17:0] out
);

reg [17:0] shift_registers_0;
reg [17:0] shift_registers_1;
reg [17:0] shift_registers_2;
reg [17:0] shift_registers_3;
reg [17:0] shift_registers_4;
reg [17:0] shift_registers_5;
reg [17:0] shift_registers_6;
reg [17:0] shift_registers_7;
reg [17:0] shift_registers_8;
reg [17:0] shift_registers_9;
always @ (posedge clk) begin
	if (reset) begin
		shift_registers_0 <= 18'd0;
		shift_registers_1 <= 18'd0;
		shift_registers_2 <= 18'd0;
		shift_registers_3 <= 18'd0;
		shift_registers_4 <= 18'd0;
		shift_registers_5 <= 18'd0;
		shift_registers_6 <= 18'd0;
		shift_registers_7 <= 18'd0;
		shift_registers_8 <= 18'd0;
		shift_registers_9 <= 18'd0;
	end else if (enable) begin
		shift_registers_0 <= in;
		shift_registers_1 <= shift_registers_0;
		shift_registers_2 <= shift_registers_1;
		shift_registers_3 <= shift_registers_2;
		shift_registers_4 <= shift_registers_3;
		shift_registers_5 <= shift_registers_4;
		shift_registers_6 <= shift_registers_5;
		shift_registers_7 <= shift_registers_6;
		shift_registers_8 <= shift_registers_7;
		shift_registers_9 <= shift_registers_8;
	end
end

assign out = shift_registers_9;

endmodule

module elementwise_add_core_18_18_9 (
	input clk,
	input reset,
	input i_valid,
	input i_ready,
	input [17:0] i_A_0,
	input [17:0] i_B_0,
	output [17:0] o_C_0,
	input [17:0] i_A_1,
	input [17:0] i_B_1,
	output [17:0] o_C_1,
	input [17:0] i_A_2,
	input [17:0] i_B_2,
	output [17:0] o_C_2,
	input [17:0] i_A_3,
	input [17:0] i_B_3,
	output [17:0] o_C_3,
	input [17:0] i_A_4,
	input [17:0] i_B_4,
	output [17:0] o_C_4,
	input [17:0] i_A_5,
	input [17:0] i_B_5,
	output [17:0] o_C_5,
	input [17:0] i_A_6,
	input [17:0] i_B_6,
	output [17:0] o_C_6,
	input [17:0] i_A_7,
	input [17:0] i_B_7,
	output [17:0] o_C_7,
	input [17:0] i_A_8,
	input [17:0] i_B_8,
	output [17:0] o_C_8,
	output o_valid,
	output o_ready
);

reg [17:0] reg_A_0;
reg [17:0] reg_B_0;
reg [17:0] reg_C_0;
reg [17:0] reg_A_1;
reg [17:0] reg_B_1;
reg [17:0] reg_C_1;
reg [17:0] reg_A_2;
reg [17:0] reg_B_2;
reg [17:0] reg_C_2;
reg [17:0] reg_A_3;
reg [17:0] reg_B_3;
reg [17:0] reg_C_3;
reg [17:0] reg_A_4;
reg [17:0] reg_B_4;
reg [17:0] reg_C_4;
reg [17:0] reg_A_5;
reg [17:0] reg_B_5;
reg [17:0] reg_C_5;
reg [17:0] reg_A_6;
reg [17:0] reg_B_6;
reg [17:0] reg_C_6;
reg [17:0] reg_A_7;
reg [17:0] reg_B_7;
reg [17:0] reg_C_7;
reg [17:0] reg_A_8;
reg [17:0] reg_B_8;
reg [17:0] reg_C_8;

reg valid_A_B, valid_C;
wire enable;
assign enable = i_ready;

always @ (posedge clk) begin
	if (reset) begin
		valid_A_B <= 1'b0;
		valid_C <= 1'b0;
		reg_A_0 <= 0;
		reg_B_0 <= 0;
		reg_C_0 <= 0;
		reg_A_1 <= 0;
		reg_B_1 <= 0;
		reg_C_1 <= 0;
		reg_A_2 <= 0;
		reg_B_2 <= 0;
		reg_C_2 <= 0;
		reg_A_3 <= 0;
		reg_B_3 <= 0;
		reg_C_3 <= 0;
		reg_A_4 <= 0;
		reg_B_4 <= 0;
		reg_C_4 <= 0;
		reg_A_5 <= 0;
		reg_B_5 <= 0;
		reg_C_5 <= 0;
		reg_A_6 <= 0;
		reg_B_6 <= 0;
		reg_C_6 <= 0;
		reg_A_7 <= 0;
		reg_B_7 <= 0;
		reg_C_7 <= 0;
		reg_A_8 <= 0;
		reg_B_8 <= 0;
		reg_C_8 <= 0;
	end else if (enable) begin
		reg_A_0 <= i_A_0;
		reg_B_0 <= i_B_0;
		reg_C_0 <= reg_A_0 + reg_B_0;
		reg_A_1 <= i_A_1;
		reg_B_1 <= i_B_1;
		reg_C_1 <= reg_A_1 + reg_B_1;
		reg_A_2 <= i_A_2;
		reg_B_2 <= i_B_2;
		reg_C_2 <= reg_A_2 + reg_B_2;
		reg_A_3 <= i_A_3;
		reg_B_3 <= i_B_3;
		reg_C_3 <= reg_A_3 + reg_B_3;
		reg_A_4 <= i_A_4;
		reg_B_4 <= i_B_4;
		reg_C_4 <= reg_A_4 + reg_B_4;
		reg_A_5 <= i_A_5;
		reg_B_5 <= i_B_5;
		reg_C_5 <= reg_A_5 + reg_B_5;
		reg_A_6 <= i_A_6;
		reg_B_6 <= i_B_6;
		reg_C_6 <= reg_A_6 + reg_B_6;
		reg_A_7 <= i_A_7;
		reg_B_7 <= i_B_7;
		reg_C_7 <= reg_A_7 + reg_B_7;
		reg_A_8 <= i_A_8;
		reg_B_8 <= i_B_8;
		reg_C_8 <= reg_A_8 + reg_B_8;
		valid_A_B <= i_valid;
		valid_C <= valid_A_B;
	end
end

assign o_C_0 = reg_C_0;
assign o_C_1 = reg_C_1;
assign o_C_2 = reg_C_2;
assign o_C_3 = reg_C_3;
assign o_C_4 = reg_C_4;
assign o_C_5 = reg_C_5;
assign o_C_6 = reg_C_6;
assign o_C_7 = reg_C_7;
assign o_C_8 = reg_C_8;
assign o_ready = i_ready;
assign o_valid = valid_C & i_ready;

endmodule

module elementwise_sub_core_18_18_9 (
	input clk,
	input reset,
	input i_valid,
	input i_ready,
	input [17:0] i_A_0,
	input [17:0] i_B_0,
	output [17:0] o_C_0,
	input [17:0] i_A_1,
	input [17:0] i_B_1,
	output [17:0] o_C_1,
	input [17:0] i_A_2,
	input [17:0] i_B_2,
	output [17:0] o_C_2,
	input [17:0] i_A_3,
	input [17:0] i_B_3,
	output [17:0] o_C_3,
	input [17:0] i_A_4,
	input [17:0] i_B_4,
	output [17:0] o_C_4,
	input [17:0] i_A_5,
	input [17:0] i_B_5,
	output [17:0] o_C_5,
	input [17:0] i_A_6,
	input [17:0] i_B_6,
	output [17:0] o_C_6,
	input [17:0] i_A_7,
	input [17:0] i_B_7,
	output [17:0] o_C_7,
	input [17:0] i_A_8,
	input [17:0] i_B_8,
	output [17:0] o_C_8,
	output o_valid,
	output o_ready
);

reg [17:0] reg_A_0;
reg [17:0] reg_B_0;
reg [17:0] reg_C_0;
reg [17:0] reg_A_1;
reg [17:0] reg_B_1;
reg [17:0] reg_C_1;
reg [17:0] reg_A_2;
reg [17:0] reg_B_2;
reg [17:0] reg_C_2;
reg [17:0] reg_A_3;
reg [17:0] reg_B_3;
reg [17:0] reg_C_3;
reg [17:0] reg_A_4;
reg [17:0] reg_B_4;
reg [17:0] reg_C_4;
reg [17:0] reg_A_5;
reg [17:0] reg_B_5;
reg [17:0] reg_C_5;
reg [17:0] reg_A_6;
reg [17:0] reg_B_6;
reg [17:0] reg_C_6;
reg [17:0] reg_A_7;
reg [17:0] reg_B_7;
reg [17:0] reg_C_7;
reg [17:0] reg_A_8;
reg [17:0] reg_B_8;
reg [17:0] reg_C_8;

reg valid_A_B, valid_C;
wire enable;
assign enable = i_ready;

always @ (posedge clk) begin
	if (reset) begin
		valid_A_B <= 1'b0;
		valid_C <= 1'b0;
		reg_A_0 <= 0;
		reg_B_0 <= 0;
		reg_C_0 <= 0;
		reg_A_1 <= 0;
		reg_B_1 <= 0;
		reg_C_1 <= 0;
		reg_A_2 <= 0;
		reg_B_2 <= 0;
		reg_C_2 <= 0;
		reg_A_3 <= 0;
		reg_B_3 <= 0;
		reg_C_3 <= 0;
		reg_A_4 <= 0;
		reg_B_4 <= 0;
		reg_C_4 <= 0;
		reg_A_5 <= 0;
		reg_B_5 <= 0;
		reg_C_5 <= 0;
		reg_A_6 <= 0;
		reg_B_6 <= 0;
		reg_C_6 <= 0;
		reg_A_7 <= 0;
		reg_B_7 <= 0;
		reg_C_7 <= 0;
		reg_A_8 <= 0;
		reg_B_8 <= 0;
		reg_C_8 <= 0;
	end else if (enable) begin
		reg_A_0 <= i_A_0;
		reg_B_0 <= i_B_0;
		reg_C_0 <= reg_A_0 - reg_B_0;
		reg_A_1 <= i_A_1;
		reg_B_1 <= i_B_1;
		reg_C_1 <= reg_A_1 - reg_B_1;
		reg_A_2 <= i_A_2;
		reg_B_2 <= i_B_2;
		reg_C_2 <= reg_A_2 - reg_B_2;
		reg_A_3 <= i_A_3;
		reg_B_3 <= i_B_3;
		reg_C_3 <= reg_A_3 - reg_B_3;
		reg_A_4 <= i_A_4;
		reg_B_4 <= i_B_4;
		reg_C_4 <= reg_A_4 - reg_B_4;
		reg_A_5 <= i_A_5;
		reg_B_5 <= i_B_5;
		reg_C_5 <= reg_A_5 - reg_B_5;
		reg_A_6 <= i_A_6;
		reg_B_6 <= i_B_6;
		reg_C_6 <= reg_A_6 - reg_B_6;
		reg_A_7 <= i_A_7;
		reg_B_7 <= i_B_7;
		reg_C_7 <= reg_A_7 - reg_B_7;
		reg_A_8 <= i_A_8;
		reg_B_8 <= i_B_8;
		reg_C_8 <= reg_A_8 - reg_B_8;
		valid_A_B <= i_valid;
		valid_C <= valid_A_B;
	end
end

assign o_C_0 = reg_C_0;
assign o_C_1 = reg_C_1;
assign o_C_2 = reg_C_2;
assign o_C_3 = reg_C_3;
assign o_C_4 = reg_C_4;
assign o_C_5 = reg_C_5;
assign o_C_6 = reg_C_6;
assign o_C_7 = reg_C_7;
assign o_C_8 = reg_C_8;
assign o_ready = i_ready;
assign o_valid = valid_C & i_ready;

endmodule

module dft_16_top_18 (
	input clk,
	input reset,
	input next,
	input [17:0] X0,
	output [17:0] Y0,
	input [17:0] X1,
	output [17:0] Y1,
	input [17:0] X2,
	output [17:0] Y2,
	input [17:0] X3,
	output [17:0] Y3,
	input [17:0] X4,
	output [17:0] Y4,
	input [17:0] X5,
	output [17:0] Y5,
	input [17:0] X6,
	output [17:0] Y6,
	input [17:0] X7,
	output [17:0] Y7,
	input [17:0] X8,
	output [17:0] Y8,
	input [17:0] X9,
	output [17:0] Y9,
	input [17:0] X10,
	output [17:0] Y10,
	input [17:0] X11,
	output [17:0] Y11,
	input [17:0] X12,
	output [17:0] Y12,
	input [17:0] X13,
	output [17:0] Y13,
	input [17:0] X14,
	output [17:0] Y14,
	input [17:0] X15,
	output [17:0] Y15,
	input [17:0] X16,
	output [17:0] Y16,
	input [17:0] X17,
	output [17:0] Y17,
	input [17:0] X18,
	output [17:0] Y18,
	input [17:0] X19,
	output [17:0] Y19,
	input [17:0] X20,
	output [17:0] Y20,
	input [17:0] X21,
	output [17:0] Y21,
	input [17:0] X22,
	output [17:0] Y22,
	input [17:0] X23,
	output [17:0] Y23,
	input [17:0] X24,
	output [17:0] Y24,
	input [17:0] X25,
	output [17:0] Y25,
	input [17:0] X26,
	output [17:0] Y26,
	input [17:0] X27,
	output [17:0] Y27,
	input [17:0] X28,
	output [17:0] Y28,
	input [17:0] X29,
	output [17:0] Y29,
	input [17:0] X30,
	output [17:0] Y30,
	input [17:0] X31,
	output [17:0] Y31,
	output next_out
);

wire [17:0] t0_0;
wire [17:0] t0_1;
wire [17:0] t0_2;
wire [17:0] t0_3;
wire [17:0] t0_4;
wire [17:0] t0_5;
wire [17:0] t0_6;
wire [17:0] t0_7;
wire [17:0] t0_8;
wire [17:0] t0_9;
wire [17:0] t0_10;
wire [17:0] t0_11;
wire [17:0] t0_12;
wire [17:0] t0_13;
wire [17:0] t0_14;
wire [17:0] t0_15;
wire [17:0] t0_16;
wire [17:0] t0_17;
wire [17:0] t0_18;
wire [17:0] t0_19;
wire [17:0] t0_20;
wire [17:0] t0_21;
wire [17:0] t0_22;
wire [17:0] t0_23;
wire [17:0] t0_24;
wire [17:0] t0_25;
wire [17:0] t0_26;
wire [17:0] t0_27;
wire [17:0] t0_28;
wire [17:0] t0_29;
wire [17:0] t0_30;
wire [17:0] t0_31;
wire next_0;
wire [17:0] t1_0;
wire [17:0] t1_1;
wire [17:0] t1_2;
wire [17:0] t1_3;
wire [17:0] t1_4;
wire [17:0] t1_5;
wire [17:0] t1_6;
wire [17:0] t1_7;
wire [17:0] t1_8;
wire [17:0] t1_9;
wire [17:0] t1_10;
wire [17:0] t1_11;
wire [17:0] t1_12;
wire [17:0] t1_13;
wire [17:0] t1_14;
wire [17:0] t1_15;
wire [17:0] t1_16;
wire [17:0] t1_17;
wire [17:0] t1_18;
wire [17:0] t1_19;
wire [17:0] t1_20;
wire [17:0] t1_21;
wire [17:0] t1_22;
wire [17:0] t1_23;
wire [17:0] t1_24;
wire [17:0] t1_25;
wire [17:0] t1_26;
wire [17:0] t1_27;
wire [17:0] t1_28;
wire [17:0] t1_29;
wire [17:0] t1_30;
wire [17:0] t1_31;
wire next_1;
wire [17:0] t2_0;
wire [17:0] t2_1;
wire [17:0] t2_2;
wire [17:0] t2_3;
wire [17:0] t2_4;
wire [17:0] t2_5;
wire [17:0] t2_6;
wire [17:0] t2_7;
wire [17:0] t2_8;
wire [17:0] t2_9;
wire [17:0] t2_10;
wire [17:0] t2_11;
wire [17:0] t2_12;
wire [17:0] t2_13;
wire [17:0] t2_14;
wire [17:0] t2_15;
wire [17:0] t2_16;
wire [17:0] t2_17;
wire [17:0] t2_18;
wire [17:0] t2_19;
wire [17:0] t2_20;
wire [17:0] t2_21;
wire [17:0] t2_22;
wire [17:0] t2_23;
wire [17:0] t2_24;
wire [17:0] t2_25;
wire [17:0] t2_26;
wire [17:0] t2_27;
wire [17:0] t2_28;
wire [17:0] t2_29;
wire [17:0] t2_30;
wire [17:0] t2_31;
wire next_2;

assign t0_0 = X0;
assign Y0 = t2_0;
assign t0_1 = X1;
assign Y1 = t2_1;
assign t0_2 = X2;
assign Y2 = t2_2;
assign t0_3 = X3;
assign Y3 = t2_3;
assign t0_4 = X4;
assign Y4 = t2_4;
assign t0_5 = X5;
assign Y5 = t2_5;
assign t0_6 = X6;
assign Y6 = t2_6;
assign t0_7 = X7;
assign Y7 = t2_7;
assign t0_8 = X8;
assign Y8 = t2_8;
assign t0_9 = X9;
assign Y9 = t2_9;
assign t0_10 = X10;
assign Y10 = t2_10;
assign t0_11 = X11;
assign Y11 = t2_11;
assign t0_12 = X12;
assign Y12 = t2_12;
assign t0_13 = X13;
assign Y13 = t2_13;
assign t0_14 = X14;
assign Y14 = t2_14;
assign t0_15 = X15;
assign Y15 = t2_15;
assign t0_16 = X16;
assign Y16 = t2_16;
assign t0_17 = X17;
assign Y17 = t2_17;
assign t0_18 = X18;
assign Y18 = t2_18;
assign t0_19 = X19;
assign Y19 = t2_19;
assign t0_20 = X20;
assign Y20 = t2_20;
assign t0_21 = X21;
assign Y21 = t2_21;
assign t0_22 = X22;
assign Y22 = t2_22;
assign t0_23 = X23;
assign Y23 = t2_23;
assign t0_24 = X24;
assign Y24 = t2_24;
assign t0_25 = X25;
assign Y25 = t2_25;
assign t0_26 = X26;
assign Y26 = t2_26;
assign t0_27 = X27;
assign Y27 = t2_27;
assign t0_28 = X28;
assign Y28 = t2_28;
assign t0_29 = X29;
assign Y29 = t2_29;
assign t0_30 = X30;
assign Y30 = t2_30;
assign t0_31 = X31;
assign Y31 = t2_31;
assign next_0 = next;
assign next_out = next_2;
codeBlock88206_18 codeBlock88206_18_inst (
	.clk(clk),
	.reset(reset),
	.next_in(next_0),
	.X0_in(t0_0),
	.Y0(t1_0),
	.X1_in(t0_1),
	.Y1(t1_1),
	.X2_in(t0_2),
	.Y2(t1_2),
	.X3_in(t0_3),
	.Y3(t1_3),
	.X4_in(t0_4),
	.Y4(t1_4),
	.X5_in(t0_5),
	.Y5(t1_5),
	.X6_in(t0_6),
	.Y6(t1_6),
	.X7_in(t0_7),
	.Y7(t1_7),
	.X8_in(t0_8),
	.Y8(t1_8),
	.X9_in(t0_9),
	.Y9(t1_9),
	.X10_in(t0_10),
	.Y10(t1_10),
	.X11_in(t0_11),
	.Y11(t1_11),
	.X12_in(t0_12),
	.Y12(t1_12),
	.X13_in(t0_13),
	.Y13(t1_13),
	.X14_in(t0_14),
	.Y14(t1_14),
	.X15_in(t0_15),
	.Y15(t1_15),
	.X16_in(t0_16),
	.Y16(t1_16),
	.X17_in(t0_17),
	.Y17(t1_17),
	.X18_in(t0_18),
	.Y18(t1_18),
	.X19_in(t0_19),
	.Y19(t1_19),
	.X20_in(t0_20),
	.Y20(t1_20),
	.X21_in(t0_21),
	.Y21(t1_21),
	.X22_in(t0_22),
	.Y22(t1_22),
	.X23_in(t0_23),
	.Y23(t1_23),
	.X24_in(t0_24),
	.Y24(t1_24),
	.X25_in(t0_25),
	.Y25(t1_25),
	.X26_in(t0_26),
	.Y26(t1_26),
	.X27_in(t0_27),
	.Y27(t1_27),
	.X28_in(t0_28),
	.Y28(t1_28),
	.X29_in(t0_29),
	.Y29(t1_29),
	.X30_in(t0_30),
	.Y30(t1_30),
	.X31_in(t0_31),
	.Y31(t1_31),
	.next_out(next_1)
);

codeBlock89324_18 codeBlock89324_18_inst (
	.clk(clk),
	.reset(reset),
	.next_in(next_1),
	.X0_in(t1_0),
	.Y0(t2_0),
	.X1_in(t1_1),
	.Y1(t2_1),
	.X2_in(t1_2),
	.Y2(t2_2),
	.X3_in(t1_3),
	.Y3(t2_3),
	.X4_in(t1_4),
	.Y4(t2_4),
	.X5_in(t1_5),
	.Y5(t2_5),
	.X6_in(t1_6),
	.Y6(t2_6),
	.X7_in(t1_7),
	.Y7(t2_7),
	.X8_in(t1_8),
	.Y8(t2_8),
	.X9_in(t1_9),
	.Y9(t2_9),
	.X10_in(t1_10),
	.Y10(t2_10),
	.X11_in(t1_11),
	.Y11(t2_11),
	.X12_in(t1_12),
	.Y12(t2_12),
	.X13_in(t1_13),
	.Y13(t2_13),
	.X14_in(t1_14),
	.Y14(t2_14),
	.X15_in(t1_15),
	.Y15(t2_15),
	.X16_in(t1_16),
	.Y16(t2_16),
	.X17_in(t1_17),
	.Y17(t2_17),
	.X18_in(t1_18),
	.Y18(t2_18),
	.X19_in(t1_19),
	.Y19(t2_19),
	.X20_in(t1_20),
	.Y20(t2_20),
	.X21_in(t1_21),
	.Y21(t2_21),
	.X22_in(t1_22),
	.Y22(t2_22),
	.X23_in(t1_23),
	.Y23(t2_23),
	.X24_in(t1_24),
	.Y24(t2_24),
	.X25_in(t1_25),
	.Y25(t2_25),
	.X26_in(t1_26),
	.Y26(t2_26),
	.X27_in(t1_27),
	.Y27(t2_27),
	.X28_in(t1_28),
	.Y28(t2_28),
	.X29_in(t1_29),
	.Y29(t2_29),
	.X30_in(t1_30),
	.Y30(t2_30),
	.X31_in(t1_31),
	.Y31(t2_31),
	.next_out(next_2)
);

endmodule

module codeBlock89324_18 (
	input clk,
	input reset,
	input next_in,
	input [17:0] X0_in,
	output [17:0] Y0,
	input [17:0] X1_in,
	output [17:0] Y1,
	input [17:0] X2_in,
	output [17:0] Y2,
	input [17:0] X3_in,
	output [17:0] Y3,
	input [17:0] X4_in,
	output [17:0] Y4,
	input [17:0] X5_in,
	output [17:0] Y5,
	input [17:0] X6_in,
	output [17:0] Y6,
	input [17:0] X7_in,
	output [17:0] Y7,
	input [17:0] X8_in,
	output [17:0] Y8,
	input [17:0] X9_in,
	output [17:0] Y9,
	input [17:0] X10_in,
	output [17:0] Y10,
	input [17:0] X11_in,
	output [17:0] Y11,
	input [17:0] X12_in,
	output [17:0] Y12,
	input [17:0] X13_in,
	output [17:0] Y13,
	input [17:0] X14_in,
	output [17:0] Y14,
	input [17:0] X15_in,
	output [17:0] Y15,
	input [17:0] X16_in,
	output [17:0] Y16,
	input [17:0] X17_in,
	output [17:0] Y17,
	input [17:0] X18_in,
	output [17:0] Y18,
	input [17:0] X19_in,
	output [17:0] Y19,
	input [17:0] X20_in,
	output [17:0] Y20,
	input [17:0] X21_in,
	output [17:0] Y21,
	input [17:0] X22_in,
	output [17:0] Y22,
	input [17:0] X23_in,
	output [17:0] Y23,
	input [17:0] X24_in,
	output [17:0] Y24,
	input [17:0] X25_in,
	output [17:0] Y25,
	input [17:0] X26_in,
	output [17:0] Y26,
	input [17:0] X27_in,
	output [17:0] Y27,
	input [17:0] X28_in,
	output [17:0] Y28,
	input [17:0] X29_in,
	output [17:0] Y29,
	input [17:0] X30_in,
	output [17:0] Y30,
	input [17:0] X31_in,
	output [17:0] Y31,
	output next_out
);

reg next;
reg [17:0] X0;
reg [17:0] X1;
reg [17:0] X2;
reg [17:0] X3;
reg [17:0] X4;
reg [17:0] X5;
reg [17:0] X6;
reg [17:0] X7;
reg [17:0] X8;
reg [17:0] X9;
reg [17:0] X10;
reg [17:0] X11;
reg [17:0] X12;
reg [17:0] X13;
reg [17:0] X14;
reg [17:0] X15;
reg [17:0] X16;
reg [17:0] X17;
reg [17:0] X18;
reg [17:0] X19;
reg [17:0] X20;
reg [17:0] X21;
reg [17:0] X22;
reg [17:0] X23;
reg [17:0] X24;
reg [17:0] X25;
reg [17:0] X26;
reg [17:0] X27;
reg [17:0] X28;
reg [17:0] X29;
reg [17:0] X30;
reg [17:0] X31;

shiftRegFIFO_2_1 shiftRegFIFO_2_1_inst (
	.X(next),
	.Y(next_out),
	.reset(reset),
	.clk(clk)
);

wire  [17:0] a65;
 		wire  [17:0] a66;
 		wire  [17:0] a67;
 		wire  [17:0] a68;
 		wire  [17:0] a73;
 		wire  [17:0] a74;
 		wire  [17:0] a75;
 		wire  [17:0] a76;
 		wire  [17:0] a81;
 		wire  [17:0] a82;
 		wire  [17:0] a83;
 		wire  [17:0] a84;
 		wire  [17:0] a89;
 		wire  [17:0] a90;
 		wire  [17:0] a91;
 		wire  [17:0] a92;
 		wire  [17:0] a97;
 		wire  [17:0] a98;
 		wire  [17:0] a99;
 		wire  [17:0] a100;
 		wire  [17:0] a105;
 		wire  [17:0] a106;
 		wire  [17:0] a107;
 		wire  [17:0] a108;
 		wire  [17:0] a113;
 		wire  [17:0] a114;
 		wire  [17:0] a115;
 		wire  [17:0] a116;
 		wire  [17:0] a121;
 		wire  [17:0] a122;
 		wire  [17:0] a123;
 		wire  [17:0] a124;
 		wire  [17:0] t402;
 		wire  [17:0] t403;
 		wire  [17:0] t404;
 		wire  [17:0] t405;
 		wire  [17:0] t406;
 		wire  [17:0] t407;
 		wire  [17:0] t408;
 		wire  [17:0] t409;
 		wire  [17:0] t418;
 		wire  [17:0] t419;
 		wire  [17:0] t420;
 		wire  [17:0] t421;
 		wire  [17:0] t422;
 		wire  [17:0] t423;
 		wire  [17:0] t424;
 		wire  [17:0] t425;
 		wire  [17:0] t434;
 		wire  [17:0] t435;
 		wire  [17:0] t436;
 		wire  [17:0] t437;
 		wire  [17:0] t438;
 		wire  [17:0] t439;
 		wire  [17:0] t440;
 		wire  [17:0] t441;
 		wire  [17:0] t450;
 		wire  [17:0] t451;
 		wire  [17:0] t452;
 		wire  [17:0] t453;
 		wire  [17:0] t454;
 		wire  [17:0] t455;
 		wire  [17:0] t456;
 		wire  [17:0] t457;
 		wire  [17:0] t410;
 		wire  [17:0] t411;
 		wire  [17:0] t412;
 		wire  [17:0] t413;
 		wire  [17:0] t414;
 		wire  [17:0] t415;
 		wire  [17:0] t416;
 		wire  [17:0] t417;
 		wire  [17:0] t426;
 		wire  [17:0] t427;
 		wire  [17:0] t428;
 		wire  [17:0] t429;
 		wire  [17:0] t430;
 		wire  [17:0] t431;
 		wire  [17:0] t432;
 		wire  [17:0] t433;
 		wire  [17:0] t442;
 		wire  [17:0] t443;
 		wire  [17:0] t444;
 		wire  [17:0] t445;
 		wire  [17:0] t446;
 		wire  [17:0] t447;
 		wire  [17:0] t448;
 		wire  [17:0] t449;
 		wire  [17:0] t458;
 		wire  [17:0] t459;
 		wire  [17:0] t460;
 		wire  [17:0] t461;
 		wire  [17:0] t462;
 		wire  [17:0] t463;
 		wire  [17:0] t464;
 		wire  [17:0] t465;
assign a65 = X0;
 		assign a66 = X16;
 		assign a67 = X1;
 		assign a68 = X17;
 		assign a73 = X8;
 		assign a74 = X24;
 		assign a75 = X9;
 		assign a76 = X25;
 		assign a81 = X2;
 		assign a82 = X18;
 		assign a83 = X3;
 		assign a84 = X19;
 		assign a89 = X10;
 		assign a90 = X26;
 		assign a91 = X11;
 		assign a92 = X27;
 		assign a97 = X4;
 		assign a98 = X20;
 		assign a99 = X5;
 		assign a100 = X21;
 		assign a105 = X12;
 		assign a106 = X28;
 		assign a107 = X13;
 		assign a108 = X29;
 		assign a113 = X6;
 		assign a114 = X22;
 		assign a115 = X7;
 		assign a116 = X23;
 		assign a121 = X14;
 		assign a122 = X30;
 		assign a123 = X15;
 		assign a124 = X31;
 		assign Y0 = t410;
 		assign Y1 = t411;
 		assign Y16 = t412;
 		assign Y17 = t413;
 		assign Y8 = t414;
 		assign Y9 = t415;
 		assign Y24 = t416;
 		assign Y25 = t417;
 		assign Y2 = t426;
 		assign Y3 = t427;
 		assign Y18 = t428;
 		assign Y19 = t429;
 		assign Y10 = t430;
 		assign Y11 = t431;
 		assign Y26 = t432;
 		assign Y27 = t433;
 		assign Y4 = t442;
 		assign Y5 = t443;
 		assign Y20 = t444;
 		assign Y21 = t445;
 		assign Y12 = t446;
 		assign Y13 = t447;
 		assign Y28 = t448;
 		assign Y29 = t449;
 		assign Y6 = t458;
 		assign Y7 = t459;
 		assign Y22 = t460;
 		assign Y23 = t461;
 		assign Y14 = t462;
 		assign Y15 = t463;
 		assign Y30 = t464;
 		assign Y31 = t465;

addfxp_18_1 add89336(.a(a65), .b(a66), .clk(clk), .q(t402));
 		addfxp_18_1 add89351(.a(a67), .b(a68), .clk(clk), .q(t403));
 		subfxp_18_1 sub89366(.a(a65), .b(a66), .clk(clk), .q(t404));
 		subfxp_18_1 sub89381(.a(a67), .b(a68), .clk(clk), .q(t405));
 		addfxp_18_1 add89396(.a(a73), .b(a74), .clk(clk), .q(t406));
 		addfxp_18_1 add89411(.a(a75), .b(a76), .clk(clk), .q(t407));
 		subfxp_18_1 sub89426(.a(a73), .b(a74), .clk(clk), .q(t408));
 		subfxp_18_1 sub89441(.a(a75), .b(a76), .clk(clk), .q(t409));
 		addfxp_18_1 add89544(.a(a81), .b(a82), .clk(clk), .q(t418));
 		addfxp_18_1 add89559(.a(a83), .b(a84), .clk(clk), .q(t419));
 		subfxp_18_1 sub89574(.a(a81), .b(a82), .clk(clk), .q(t420));
 		subfxp_18_1 sub89589(.a(a83), .b(a84), .clk(clk), .q(t421));
 		addfxp_18_1 add89604(.a(a89), .b(a90), .clk(clk), .q(t422));
 		addfxp_18_1 add89619(.a(a91), .b(a92), .clk(clk), .q(t423));
 		subfxp_18_1 sub89634(.a(a89), .b(a90), .clk(clk), .q(t424));
 		subfxp_18_1 sub89649(.a(a91), .b(a92), .clk(clk), .q(t425));
 		addfxp_18_1 add89752(.a(a97), .b(a98), .clk(clk), .q(t434));
 		addfxp_18_1 add89767(.a(a99), .b(a100), .clk(clk), .q(t435));
 		subfxp_18_1 sub89782(.a(a97), .b(a98), .clk(clk), .q(t436));
		subfxp_18_1 sub89797(.a(a99), .b(a100), .clk(clk), .q(t437));
 		addfxp_18_1 add89812(.a(a105), .b(a106), .clk(clk), .q(t438));
 		addfxp_18_1 add89827(.a(a107), .b(a108), .clk(clk), .q(t439));
 		subfxp_18_1 sub89842(.a(a105), .b(a106), .clk(clk), .q(t440));
 		subfxp_18_1 sub89857(.a(a107), .b(a108), .clk(clk), .q(t441));
 		addfxp_18_1 add89960(.a(a113), .b(a114), .clk(clk), .q(t450));
 		addfxp_18_1 add89975(.a(a115), .b(a116), .clk(clk), .q(t451));
 		subfxp_18_1 sub89990(.a(a113), .b(a114), .clk(clk), .q(t452));
 		subfxp_18_1 sub90005(.a(a115), .b(a116), .clk(clk), .q(t453));
 		addfxp_18_1 add90020(.a(a121), .b(a122), .clk(clk), .q(t454));
 		addfxp_18_1 add90035(.a(a123), .b(a124), .clk(clk), .q(t455));
 		subfxp_18_1 sub90050(.a(a121), .b(a122), .clk(clk), .q(t456));
 		subfxp_18_1 sub90065(.a(a123), .b(a124), .clk(clk), .q(t457));
 		addfxp_18_1 add89448(.a(t402), .b(t406), .clk(clk), .q(t410));
 		addfxp_18_1 add89455(.a(t403), .b(t407), .clk(clk), .q(t411));
 		subfxp_18_1 sub89462(.a(t402), .b(t406), .clk(clk), .q(t412));
 		subfxp_18_1 sub89469(.a(t403), .b(t407), .clk(clk), .q(t413));
 		addfxp_18_1 add89492(.a(t404), .b(t409), .clk(clk), .q(t414));
 		subfxp_18_1 sub89499(.a(t405), .b(t408), .clk(clk), .q(t415));
 		subfxp_18_1 sub89506(.a(t404), .b(t409), .clk(clk), .q(t416));
 		addfxp_18_1 add89513(.a(t405), .b(t408), .clk(clk), .q(t417));
 		addfxp_18_1 add89656(.a(t418), .b(t422), .clk(clk), .q(t426));
 		addfxp_18_1 add89663(.a(t419), .b(t423), .clk(clk), .q(t427));
 		subfxp_18_1 sub89670(.a(t418), .b(t422), .clk(clk), .q(t428));
 		subfxp_18_1 sub89677(.a(t419), .b(t423), .clk(clk), .q(t429));
 		addfxp_18_1 add89700(.a(t420), .b(t425), .clk(clk), .q(t430));
 		subfxp_18_1 sub89707(.a(t421), .b(t424), .clk(clk), .q(t431));
 		subfxp_18_1 sub89714(.a(t420), .b(t425), .clk(clk), .q(t432));
 		addfxp_18_1 add89721(.a(t421), .b(t424), .clk(clk), .q(t433));
 		addfxp_18_1 add89864(.a(t434), .b(t438), .clk(clk), .q(t442));
 		addfxp_18_1 add89871(.a(t435), .b(t439), .clk(clk), .q(t443));
 		subfxp_18_1 sub89878(.a(t434), .b(t438), .clk(clk), .q(t444));
 		subfxp_18_1 sub89885(.a(t435), .b(t439), .clk(clk), .q(t445));
 		addfxp_18_1 add89908(.a(t436), .b(t441), .clk(clk), .q(t446));
 		subfxp_18_1 sub89915(.a(t437), .b(t440), .clk(clk), .q(t447));
 		subfxp_18_1 sub89922(.a(t436), .b(t441), .clk(clk), .q(t448));
 		addfxp_18_1 add89929(.a(t437), .b(t440), .clk(clk), .q(t449));
 		addfxp_18_1 add90072(.a(t450), .b(t454), .clk(clk), .q(t458));
 		addfxp_18_1 add90079(.a(t451), .b(t455), .clk(clk), .q(t459));
 		subfxp_18_1 sub90086(.a(t450), .b(t454), .clk(clk), .q(t460));
 		subfxp_18_1 sub90093(.a(t451), .b(t455), .clk(clk), .q(t461));
 		addfxp_18_1 add90116(.a(t452), .b(t457), .clk(clk), .q(t462));
 		subfxp_18_1 sub90123(.a(t453), .b(t456), .clk(clk), .q(t463));
 		subfxp_18_1 sub90130(.a(t452), .b(t457), .clk(clk), .q(t464));
 		addfxp_18_1 add90137(.a(t453), .b(t456), .clk(clk), .q(t465));

always @(posedge clk) begin
	if (reset == 1) begin
		next <= 1'b0;
	end else begin
		X0 <= X0_in;
		X1 <= X1_in;
		X2 <= X2_in;
		X3 <= X3_in;
		X4 <= X4_in;
		X5 <= X5_in;
		X6 <= X6_in;
		X7 <= X7_in;
		X8 <= X8_in;
		X9 <= X9_in;
		X10 <= X10_in;
		X11 <= X11_in;
		X12 <= X12_in;
		X13 <= X13_in;
		X14 <= X14_in;
		X15 <= X15_in;
		X16 <= X16_in;
		X17 <= X17_in;
		X18 <= X18_in;
		X19 <= X19_in;
		X20 <= X20_in;
		X21 <= X21_in;
		X22 <= X22_in;
		X23 <= X23_in;
		X24 <= X24_in;
		X25 <= X25_in;
		X26 <= X26_in;
		X27 <= X27_in;
		X28 <= X28_in;
		X29 <= X29_in;
		X30 <= X30_in;
		X31 <= X31_in;
		next <= next_in;
	end
end

endmodule

module codeBlock88206_18 (
	input clk,
	input reset,
	input next_in,
	input [17:0] X0_in,
	output [17:0] Y0,
	input [17:0] X1_in,
	output [17:0] Y1,
	input [17:0] X2_in,
	output [17:0] Y2,
	input [17:0] X3_in,
	output [17:0] Y3,
	input [17:0] X4_in,
	output [17:0] Y4,
	input [17:0] X5_in,
	output [17:0] Y5,
	input [17:0] X6_in,
	output [17:0] Y6,
	input [17:0] X7_in,
	output [17:0] Y7,
	input [17:0] X8_in,
	output [17:0] Y8,
	input [17:0] X9_in,
	output [17:0] Y9,
	input [17:0] X10_in,
	output [17:0] Y10,
	input [17:0] X11_in,
	output [17:0] Y11,
	input [17:0] X12_in,
	output [17:0] Y12,
	input [17:0] X13_in,
	output [17:0] Y13,
	input [17:0] X14_in,
	output [17:0] Y14,
	input [17:0] X15_in,
	output [17:0] Y15,
	input [17:0] X16_in,
	output [17:0] Y16,
	input [17:0] X17_in,
	output [17:0] Y17,
	input [17:0] X18_in,
	output [17:0] Y18,
	input [17:0] X19_in,
	output [17:0] Y19,
	input [17:0] X20_in,
	output [17:0] Y20,
	input [17:0] X21_in,
	output [17:0] Y21,
	input [17:0] X22_in,
	output [17:0] Y22,
	input [17:0] X23_in,
	output [17:0] Y23,
	input [17:0] X24_in,
	output [17:0] Y24,
	input [17:0] X25_in,
	output [17:0] Y25,
	input [17:0] X26_in,
	output [17:0] Y26,
	input [17:0] X27_in,
	output [17:0] Y27,
	input [17:0] X28_in,
	output [17:0] Y28,
	input [17:0] X29_in,
	output [17:0] Y29,
	input [17:0] X30_in,
	output [17:0] Y30,
	input [17:0] X31_in,
	output [17:0] Y31,
	output next_out
);

reg next;
reg [17:0] X0;
reg [17:0] X1;
reg [17:0] X2;
reg [17:0] X3;
reg [17:0] X4;
reg [17:0] X5;
reg [17:0] X6;
reg [17:0] X7;
reg [17:0] X8;
reg [17:0] X9;
reg [17:0] X10;
reg [17:0] X11;
reg [17:0] X12;
reg [17:0] X13;
reg [17:0] X14;
reg [17:0] X15;
reg [17:0] X16;
reg [17:0] X17;
reg [17:0] X18;
reg [17:0] X19;
reg [17:0] X20;
reg [17:0] X21;
reg [17:0] X22;
reg [17:0] X23;
reg [17:0] X24;
reg [17:0] X25;
reg [17:0] X26;
reg [17:0] X27;
reg [17:0] X28;
reg [17:0] X29;
reg [17:0] X30;
reg [17:0] X31;
shiftRegFIFO_5_1 shiftRegFIFO_5_1_inst (
	.X(next),
	.Y(next_out),
	.reset(reset),
	.clk(clk)
);
wire  [17:0] a249;
 		wire  [17:0] a250;
 		wire  [17:0] a251;
 		wire  [17:0] a252;
 		wire  [17:0] a257;
 		wire  [17:0] a258;
 		wire  [17:0] a259;
 		wire  [17:0] a260;
 		wire  [17:0] a265;
 		wire  [17:0] a266;
 		wire  [17:0] a267;
 		wire  [17:0] a268;
 		wire  [17:0] a273;
 		wire  [17:0] a274;
 		wire  [17:0] a275;
 		wire  [17:0] a276;
 		wire  [17:0] a281;
 		wire  [17:0] a282;
 		wire  [17:0] a283;
 		wire  [17:0] a284;
 		wire  [17:0] a289;
 		wire  [17:0] a290;
 		wire  [17:0] a291;
 		wire  [17:0] a292;
 		wire  [17:0] a297;
 		wire  [17:0] a298;
 		wire  [17:0] a299;
 		wire  [17:0] a300;
 		wire  [17:0] a305;
 		wire  [17:0] a306;
 		wire  [17:0] a307;
 		wire  [17:0] a308;
 		wire  [17:0] t914;
 		wire  [17:0] t915;
 		wire  [17:0] t916;
 		wire  [17:0] t917;
 		wire  [17:0] t918;
 		wire  [17:0] t919;
 		wire  [17:0] t920;
 		wire  [17:0] t921;
 		wire  [17:0] t930;
 		wire  [17:0] t931;
 		wire  [17:0] t932;
 		wire  [17:0] t933;
 		wire  [17:0] t934;
 		wire  [17:0] t935;
 		wire  [17:0] t936;
 		wire  [17:0] t937;
 		wire  [17:0] t952;
 		wire  [17:0] t953;
 		wire  [17:0] t954;
 		wire  [17:0] t955;
 		wire  [17:0] t956;
 		wire  [17:0] t957;
 		wire  [17:0] t958;
 		wire  [17:0] t959;
 		wire  [17:0] t972;
 		wire  [17:0] t973;
 		wire  [17:0] t974;
 		wire  [17:0] t975;
 		wire  [17:0] t976;
 		wire  [17:0] t977;
 		wire  [17:0] t978;
 		wire  [17:0] t979;
 		wire  [17:0] t922;
 		wire  [17:0] t923;
 		wire  [17:0] t924;
 		wire  [17:0] t925;
 		wire  [17:0] t926;
 		wire  [17:0] t927;
 		wire  [17:0] t928;
 		wire  [17:0] t929;
 		wire  [17:0] t938;
 		wire  [17:0] t939;
 		wire  [17:0] t940;
 		wire  [17:0] t941;
 		wire  [17:0] t944;
 		wire  [17:0] t945;
 		wire  [17:0] t946;
 		wire  [17:0] t947;
 		wire  [17:0] t960;
 		wire  [17:0] t961;
 		wire  [17:0] t962;
 		wire  [17:0] t963;
 		wire  [17:0] t964;
 		wire  [17:0] t965;
 		wire  [17:0] t966;
 		wire  [17:0] t967;
 		wire  [17:0] t980;
 		wire  [17:0] t981;
 		wire  [17:0] t982;
 		wire  [17:0] t983;
 		wire  [17:0] t986;
 		wire  [17:0] t987;
 		wire  [17:0] t988;
 		wire  [17:0] t989;
 		reg  [17:0] tm24;
 		reg  [17:0] tm27;
 		reg  [17:0] tm30;
 		reg  [17:0] tm33;
 		reg  [17:0] tm36;
 		reg  [17:0] tm39;
 		reg  [17:0] tm42;
 		reg  [17:0] tm45;
 		reg  [17:0] tm48;
 		reg  [17:0] tm51;
 		reg  [17:0] tm54;
 		reg  [17:0] tm57;
 		reg  [17:0] tm60;
 		reg  [17:0] tm63;
 		reg  [17:0] tm66;
 		reg  [17:0] tm69;
 		wire  [17:0] a225;
 		wire  [17:0] a226;
 		wire  [17:0] a227;
 		wire  [17:0] a228;
 		wire  [17:0] a229;
 		wire  [17:0] a230;
 		wire  [17:0] a231;
 		wire  [17:0] a232;
 		wire  [17:0] a233;
 		wire  [17:0] a234;
 		wire  [17:0] a235;
 		wire  [17:0] a236;
 		wire  [17:0] a237;
 		wire  [17:0] a238;
 		wire  [17:0] a239;
 		wire  [17:0] a240;
 		wire  [17:0] a241;
 		wire  [17:0] a242;
 		wire  [17:0] a243;
 		wire  [17:0] a244;
 		wire  [17:0] a245;
 		wire  [17:0] a246;
 		wire  [17:0] a247;
 		wire  [17:0] a248;
 		reg  [17:0] tm25;
 		reg  [17:0] tm28;
 		reg  [17:0] tm31;
 		reg  [17:0] tm34;
 		reg  [17:0] tm37;
 		reg  [17:0] tm40;
 		reg  [17:0] tm43;
 		reg  [17:0] tm46;
 		reg  [17:0] tm49;
 		reg  [17:0] tm52;
 		reg  [17:0] tm55;
 		reg  [17:0] tm58;
 		reg  [17:0] tm61;
 		reg  [17:0] tm64;
 		reg  [17:0] tm67;
 		reg  [17:0] tm70;
 		wire  [17:0] t942;
 		wire  [17:0] t943;
 		wire  [17:0] t948;
 		wire  [17:0] t949;
 		wire  [17:0] t950;
 		wire  [17:0] t951;
 		wire  [17:0] t968;
 		wire  [17:0] t969;
 		wire  [17:0] t970;
 		wire  [17:0] t971;
 		wire  [17:0] t984;
 		wire  [17:0] t985;
 		wire  [17:0] t990;
 		wire  [17:0] t991;
 		wire  [17:0] t992;
 		wire  [17:0] t993;
 		reg  [17:0] tm26;
 		reg  [17:0] tm29;
 		reg  [17:0] tm32;
 		reg  [17:0] tm35;
 		reg  [17:0] tm38;
 		reg  [17:0] tm41;
 		reg  [17:0] tm44;
 		reg  [17:0] tm47;
 		reg  [17:0] tm50;
 		reg  [17:0] tm53;
 		reg  [17:0] tm56;
 		reg  [17:0] tm59;
 		reg  [17:0] tm62;
 		reg  [17:0] tm65;
 		reg  [17:0] tm68;
 		reg  [17:0] tm71;

wire [17:0] tm0;
assign tm0 = (18'hb505 >> (18-18));
wire  [17:0] tm2;
assign tm2 = (18'hec83 >> (18-18));
wire  [17:0] tm3;
assign tm3 = (18'h61f8 >> (18-18));

assign a249 = X0;
 		assign a250 = X16;
 		assign a251 = X1;
 		assign a252 = X17;
 		assign a257 = X8;
 		assign a258 = X24;
 		assign a259 = X9;
 		assign a260 = X25;
 		assign a265 = X2;
 		assign a266 = X18;
 		assign a267 = X3;
 		assign a268 = X19;
 		assign a273 = X10;
 		assign a274 = X26;
 		assign a275 = X11;
 		assign a276 = X27;
 		assign a281 = X4;
 		assign a282 = X20;
 		assign a283 = X5;
 		assign a284 = X21;
 		assign a289 = X12;
 		assign a290 = X28;
 		assign a291 = X13;
 		assign a292 = X29;
 		assign a297 = X6;
 		assign a298 = X22;
 		assign a299 = X7;
 		assign a300 = X23;
 		assign a305 = X14;
 		assign a306 = X30;
 		assign a307 = X15;
 		assign a308 = X31;
 		assign Y0 = tm26;
 		assign Y1 = tm29;
 		assign Y4 = tm32;
 		assign Y5 = tm35;
 		assign Y2 = tm38;
 		assign Y3 = tm41;
 		assign Y6 = tm44;
 		assign Y7 = tm47;
 		assign Y8 = tm50;
 		assign Y9 = tm53;
 		assign Y12 = t942;
 		assign Y13 = t943;
 		assign Y10 = t948;
 		assign Y11 = t949;
 		assign Y14 = t950;
 		assign Y15 = t951;
 		assign Y16 = tm56;
 		assign Y17 = tm59;
 		assign Y20 = tm62;
 		assign Y21 = tm65;
 		assign Y18 = t968;
 		assign Y19 = t969;
 		assign Y22 = t970;
 		assign Y23 = (~(t971)+1);
 		assign Y24 = tm68;
 		assign Y25 = tm71;
 		assign Y28 = t984;
 		assign Y29 = (~(t985)+1);
 		assign Y26 = t990;
 		assign Y27 = t991;
 		assign Y30 = (~(t992)+1);
 		assign Y31 = t993;

addfxp_18_1 add88218(.a(a249), .b(a250), .clk(clk), .q(t914));
 		addfxp_18_1 add88233(.a(a251), .b(a252), .clk(clk), .q(t915));
 		subfxp_18_1 sub88248(.a(a249), .b(a250), .clk(clk), .q(t916));
 		subfxp_18_1 sub88263(.a(a251), .b(a252), .clk(clk), .q(t917));
 		addfxp_18_1 add88278(.a(a257), .b(a258), .clk(clk), .q(t918));
 		addfxp_18_1 add88293(.a(a259), .b(a260), .clk(clk), .q(t919));
 		subfxp_18_1 sub88308(.a(a257), .b(a258), .clk(clk), .q(t920));
 		subfxp_18_1 sub88323(.a(a259), .b(a260), .clk(clk), .q(t921));
 		addfxp_18_1 add88426(.a(a265), .b(a266), .clk(clk), .q(t930));
 		addfxp_18_1 add88441(.a(a267), .b(a268), .clk(clk), .q(t931));
 		subfxp_18_1 sub88456(.a(a265), .b(a266), .clk(clk), .q(t932));
 		subfxp_18_1 sub88471(.a(a267), .b(a268), .clk(clk), .q(t933));
 		addfxp_18_1 add88486(.a(a273), .b(a274), .clk(clk), .q(t934));
 		addfxp_18_1 add88501(.a(a275), .b(a276), .clk(clk), .q(t935));
 		subfxp_18_1 sub88516(.a(a273), .b(a274), .clk(clk), .q(t936));
 		subfxp_18_1 sub88531(.a(a275), .b(a276), .clk(clk), .q(t937));
 		addfxp_18_1 add88746(.a(a281), .b(a282), .clk(clk), .q(t952));
 		addfxp_18_1 add88761(.a(a283), .b(a284), .clk(clk), .q(t953));
 		subfxp_18_1 sub88776(.a(a281), .b(a282), .clk(clk), .q(t954));
 		subfxp_18_1 sub88791(.a(a283), .b(a284), .clk(clk), .q(t955));
 		addfxp_18_1 add88806(.a(a289), .b(a290), .clk(clk), .q(t956));
 		addfxp_18_1 add88821(.a(a291), .b(a292), .clk(clk), .q(t957));
 		subfxp_18_1 sub88836(.a(a289), .b(a290), .clk(clk), .q(t958));
 		subfxp_18_1 sub88851(.a(a291), .b(a292), .clk(clk), .q(t959));
 		addfxp_18_1 add89012(.a(a297), .b(a298), .clk(clk), .q(t972));
 		addfxp_18_1 add89027(.a(a299), .b(a300), .clk(clk), .q(t973));
 		subfxp_18_1 sub89042(.a(a297), .b(a298), .clk(clk), .q(t974));
 		subfxp_18_1 sub89057(.a(a299), .b(a300), .clk(clk), .q(t975));
 		addfxp_18_1 add89072(.a(a305), .b(a306), .clk(clk), .q(t976));
 		addfxp_18_1 add89087(.a(a307), .b(a308), .clk(clk), .q(t977));
 		subfxp_18_1 sub89102(.a(a305), .b(a306), .clk(clk), .q(t978));
 		subfxp_18_1 sub89117(.a(a307), .b(a308), .clk(clk), .q(t979));
 		addfxp_18_1 add88330(.a(t914), .b(t918), .clk(clk), .q(t922));
 		addfxp_18_1 add88337(.a(t915), .b(t919), .clk(clk), .q(t923));
 		subfxp_18_1 sub88344(.a(t914), .b(t918), .clk(clk), .q(t924));
 		subfxp_18_1 sub88351(.a(t915), .b(t919), .clk(clk), .q(t925));
 		addfxp_18_1 add88374(.a(t916), .b(t921), .clk(clk), .q(t926));
 		subfxp_18_1 sub88381(.a(t917), .b(t920), .clk(clk), .q(t927));
 		subfxp_18_1 sub88388(.a(t916), .b(t921), .clk(clk), .q(t928));
 		addfxp_18_1 add88395(.a(t917), .b(t920), .clk(clk), .q(t929));
 		addfxp_18_1 add88538(.a(t930), .b(t934), .clk(clk), .q(t938));
 		addfxp_18_1 add88545(.a(t931), .b(t935), .clk(clk), .q(t939));
 		subfxp_18_1 sub88552(.a(t930), .b(t934), .clk(clk), .q(t940));
 		subfxp_18_1 sub88559(.a(t931), .b(t935), .clk(clk), .q(t941));
 		addfxp_18_1 add88610(.a(t932), .b(t937), .clk(clk), .q(t944));
 		subfxp_18_1 sub88617(.a(t933), .b(t936), .clk(clk), .q(t945));
 		subfxp_18_1 sub88624(.a(t932), .b(t937), .clk(clk), .q(t946));
 		addfxp_18_1 add88631(.a(t933), .b(t936), .clk(clk), .q(t947));
 		addfxp_18_1 add88858(.a(t952), .b(t956), .clk(clk), .q(t960));
 		addfxp_18_1 add88865(.a(t953), .b(t957), .clk(clk), .q(t961));
 		subfxp_18_1 sub88872(.a(t952), .b(t956), .clk(clk), .q(t962));
 		subfxp_18_1 sub88879(.a(t953), .b(t957), .clk(clk), .q(t963));
 		addfxp_18_1 add88903(.a(t954), .b(t959), .clk(clk), .q(t964));
 		subfxp_18_1 sub88910(.a(t955), .b(t958), .clk(clk), .q(t965));
 		subfxp_18_1 sub88917(.a(t954), .b(t959), .clk(clk), .q(t966));
 		addfxp_18_1 add88924(.a(t955), .b(t958), .clk(clk), .q(t967));
 		addfxp_18_1 add89124(.a(t972), .b(t976), .clk(clk), .q(t980));
 		addfxp_18_1 add89131(.a(t973), .b(t977), .clk(clk), .q(t981));
 		subfxp_18_1 sub89138(.a(t972), .b(t976), .clk(clk), .q(t982));
 		subfxp_18_1 sub89145(.a(t973), .b(t977), .clk(clk), .q(t983));
 		addfxp_18_1 add89197(.a(t974), .b(t979), .clk(clk), .q(t986));
 		subfxp_18_1 sub89204(.a(t975), .b(t978), .clk(clk), .q(t987));
 		subfxp_18_1 sub89211(.a(t974), .b(t979), .clk(clk), .q(t988));
 		addfxp_18_1 add89218(.a(t975), .b(t978), .clk(clk), .q(t989));

multfix_alt_dsp_18 m88566(.ax(tm0), .ay(t940), .bx(tm0), .by(t941), .clk(clk), .a_q_sc(a225), .a_q_unsc(), .b_q_sc(a226), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88570(.ax(tm2), .ay(t944), .bx(tm3), .by(t945), .clk(clk), .a_q_sc(a227), .a_q_unsc(), .b_q_sc(a228), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88572(.ax(tm2), .ay(t945), .bx(tm3), .by(t944), .clk(clk), .a_q_sc(a229), .a_q_unsc(), .b_q_sc(a230), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88574(.ax(tm3), .ay(t946), .bx(tm2), .by(t947), .clk(clk), .a_q_sc(a231), .a_q_unsc(), .b_q_sc(a232), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88576(.ax(tm3), .ay(t947), .bx(tm2), .by(t946), .clk(clk), .a_q_sc(a233), .a_q_unsc(), .b_q_sc(a234), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88578(.ax(tm0), .ay(t964), .bx(tm0), .by(t965), .clk(clk), .a_q_sc(a235), .a_q_unsc(), .b_q_sc(a236), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88580(.ax(tm0), .ay(t967), .bx(tm0), .by(t966), .clk(clk), .a_q_sc(a237), .a_q_unsc(), .b_q_sc(a238), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88582(.ax(tm0), .ay(t983), .bx(tm0), .by(t982), .clk(clk), .a_q_sc(a239), .a_q_unsc(), .b_q_sc(a240), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88584(.ax(tm3), .ay(t986), .bx(tm2), .by(t987), .clk(clk), .a_q_sc(a241), .a_q_unsc(), .b_q_sc(a242), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88586(.ax(tm3), .ay(t987), .bx(tm2), .by(t986), .clk(clk), .a_q_sc(a243), .a_q_unsc(), .b_q_sc(a244), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88588(.ax(tm2), .ay(t988), .bx(tm3), .by(t989), .clk(clk), .a_q_sc(a245), .a_q_unsc(), .b_q_sc(a246), .b_q_unsc(), .rst(reset));
 			multfix_alt_dsp_18 m88590(.ax(tm3), .ay(t988), .bx(tm2), .by(t989), .clk(clk), .a_q_sc(a247), .a_q_unsc(), .b_q_sc(a248), .b_q_unsc(), .rst(reset));

addfxp_18_1 add88580(.a(a225), .b(a226), .clk(clk), .q(t942));
 		subfxp_18_1 sub88587(.a(a226), .b(a225), .clk(clk), .q(t943));
 		addfxp_18_1 add88652(.a(a227), .b(a228), .clk(clk), .q(t948));
 		subfxp_18_1 sub88673(.a(a229), .b(a230), .clk(clk), .q(t949));
 		addfxp_18_1 add88694(.a(a231), .b(a232), .clk(clk), .q(t950));
 		subfxp_18_1 sub88715(.a(a233), .b(a234), .clk(clk), .q(t951));
 		addfxp_18_1 add88945(.a(a235), .b(a236), .clk(clk), .q(t968));
 		subfxp_18_1 sub88952(.a(a236), .b(a235), .clk(clk), .q(t969));
 		subfxp_18_1 sub88973(.a(a237), .b(a238), .clk(clk), .q(t970));
 		addfxp_18_1 add88980(.a(a238), .b(a237), .clk(clk), .q(t971));
 		subfxp_18_1 sub89166(.a(a239), .b(a240), .clk(clk), .q(t984));
 		addfxp_18_1 add89173(.a(a240), .b(a239), .clk(clk), .q(t985));
 		addfxp_18_1 add89239(.a(a241), .b(a242), .clk(clk), .q(t990));
 		subfxp_18_1 sub89260(.a(a243), .b(a244), .clk(clk), .q(t991));
 		addfxp_18_1 add89281(.a(a245), .b(a246), .clk(clk), .q(t992));
 		subfxp_18_1 sub89302(.a(a247), .b(a248), .clk(clk), .q(t993));

always @(posedge clk) begin
	if (reset == 1) begin
		next <= 1'b0;
	end else begin
		X0 <= X0_in;
		X1 <= X1_in;
		X2 <= X2_in;
		X3 <= X3_in;
		X4 <= X4_in;
		X5 <= X5_in;
		X6 <= X6_in;
		X7 <= X7_in;
		X8 <= X8_in;
		X9 <= X9_in;
		X10 <= X10_in;
		X11 <= X11_in;
		X12 <= X12_in;
		X13 <= X13_in;
		X14 <= X14_in;
		X15 <= X15_in;
		X16 <= X16_in;
		X17 <= X17_in;
		X18 <= X18_in;
		X19 <= X19_in;
		X20 <= X20_in;
		X21 <= X21_in;
		X22 <= X22_in;
		X23 <= X23_in;
		X24 <= X24_in;
		X25 <= X25_in;
		X26 <= X26_in;
		X27 <= X27_in;
		X28 <= X28_in;
		X29 <= X29_in;
		X30 <= X30_in;
		X31 <= X31_in;
		next <= next_in;
		tm24 <= t922;
		tm27 <= t923;
		tm30 <= t924;
		tm33 <= t925;
		tm36 <= t926;
		tm39 <= t927;
		tm42 <= t928;
		tm45 <= t929;
		tm48 <= t938;
		tm51 <= t939;
		tm54 <= t960;
		tm57 <= t961;
		tm60 <= t963;
		tm63 <= (~(t962)+1);
		tm66 <= t980;
		tm69 <= t981;
		tm25 <= tm24;
		tm28 <= tm27;
		tm31 <= tm30;
		tm34 <= tm33;
		tm37 <= tm36;
		tm40 <= tm39;
		tm43 <= tm42;
		tm46 <= tm45;
		tm49 <= tm48;
		tm52 <= tm51;
		tm55 <= tm54;
		tm58 <= tm57;
		tm61 <= tm60;
		tm64 <= tm63;
		tm67 <= tm66;
		tm70 <= tm69;
		tm26 <= tm25;
		tm29 <= tm28;
		tm32 <= tm31;
		tm35 <= tm34;
		tm38 <= tm37;
		tm41 <= tm40;
		tm44 <= tm43;
		tm47 <= tm46;
		tm50 <= tm49;
		tm53 <= tm52;
		tm56 <= tm55;
		tm59 <= tm58;
		tm62 <= tm61;
		tm65 <= tm64;
		tm68 <= tm67;
		tm71 <= tm70;
	end
end

endmodule

module elementwise_mult_core_18_1810_9_1 (
	input clk,
	input reset,
	input i_valid,
	input i_ready,
	input [17:0] i_A_0,
	input [17:0] i_B_0,
	output [17:0] o_C_0,
	input [17:0] i_A_1,
	input [17:0] i_B_1,
	output [17:0] o_C_1,
	input [17:0] i_A_2,
	input [17:0] i_B_2,
	output [17:0] o_C_2,
	input [17:0] i_A_3,
	input [17:0] i_B_3,
	output [17:0] o_C_3,
	input [17:0] i_A_4,
	input [17:0] i_B_4,
	output [17:0] o_C_4,
	input [17:0] i_A_5,
	input [17:0] i_B_5,
	output [17:0] o_C_5,
	input [17:0] i_A_6,
	input [17:0] i_B_6,
	output [17:0] o_C_6,
	input [17:0] i_A_7,
	input [17:0] i_B_7,
	output [17:0] o_C_7,
	input [17:0] i_A_8,
	input [17:0] i_B_8,
	output [17:0] o_C_8,
	output o_valid,
	output o_ready
);

// Store inputs and outputs in registers
reg [17:0] reg_A_0;
reg [17:0] reg_B_0;
wire [17:0] reg_C_0;
reg [17:0] reg_A_1;
reg [17:0] reg_B_1;
wire [17:0] reg_C_1;
reg [17:0] reg_A_2;
reg [17:0] reg_B_2;
wire [17:0] reg_C_2;
reg [17:0] reg_A_3;
reg [17:0] reg_B_3;
wire [17:0] reg_C_3;
reg [17:0] reg_A_4;
reg [17:0] reg_B_4;
wire [17:0] reg_C_4;
reg [17:0] reg_A_5;
reg [17:0] reg_B_5;
wire [17:0] reg_C_5;
reg [17:0] reg_A_6;
reg [17:0] reg_B_6;
wire [17:0] reg_C_6;
reg [17:0] reg_A_7;
reg [17:0] reg_B_7;
wire [17:0] reg_C_7;
reg [17:0] reg_A_8;
reg [17:0] reg_B_8;
wire [17:0] reg_C_8;

reg valid_A_B;
wire valid_C;
wire enable;
assign enable = i_ready;

wire mult_valid_0;
wire round_valid_0;
wire [36:0] mult_C_0;
wire [36:0] rounded_C_0;
wire mult_valid_1;
wire round_valid_1;
wire [36:0] mult_C_1;
wire [36:0] rounded_C_1;
wire mult_valid_2;
wire round_valid_2;
wire [36:0] mult_C_2;
wire [36:0] rounded_C_2;
wire mult_valid_3;
wire round_valid_3;
wire [36:0] mult_C_3;
wire [36:0] rounded_C_3;
wire mult_valid_4;
wire round_valid_4;
wire [36:0] mult_C_4;
wire [36:0] rounded_C_4;
wire mult_valid_5;
wire round_valid_5;
wire [36:0] mult_C_5;
wire [36:0] rounded_C_5;
wire mult_valid_6;
wire round_valid_6;
wire [36:0] mult_C_6;
wire [36:0] rounded_C_6;
wire mult_valid_7;
wire round_valid_7;
wire [36:0] mult_C_7;
wire [36:0] rounded_C_7;
wire mult_valid_8;
wire round_valid_8;
wire [36:0] mult_C_8;
wire [36:0] rounded_C_8;

dsp_signed_mult_18x18_unit_18_18_1 dsp_signed_mult_18x18_unit_18_18_1_inst0 (
	.clk(clk),
	.reset(reset),
	.ena(enable),
	.i_valid(valid_A_B),
	.ax(reg_A_0),
	.ay(reg_B_0),
	.bx(reg_A_1),
	.by(reg_B_1),
	.o_valid(mult_valid_0),
	.resulta(mult_C_0),
	.resultb(mult_C_1)
);
assign mult_valid_1 = mult_valid_0;
dsp_signed_mult_18x18_unit_18_18_1 dsp_signed_mult_18x18_unit_18_18_1_inst2 (
	.clk(clk),
	.reset(reset),
	.ena(enable),
	.i_valid(valid_A_B),
	.ax(reg_A_2),
	.ay(reg_B_2),
	.bx(reg_A_3),
	.by(reg_B_3),
	.o_valid(mult_valid_2),
	.resulta(mult_C_2),
	.resultb(mult_C_3)
);
assign mult_valid_3 = mult_valid_2;
dsp_signed_mult_18x18_unit_18_18_1 dsp_signed_mult_18x18_unit_18_18_1_inst4 (
	.clk(clk),
	.reset(reset),
	.ena(enable),
	.i_valid(valid_A_B),
	.ax(reg_A_4),
	.ay(reg_B_4),
	.bx(reg_A_5),
	.by(reg_B_5),
	.o_valid(mult_valid_4),
	.resulta(mult_C_4),
	.resultb(mult_C_5)
);
assign mult_valid_5 = mult_valid_4;
dsp_signed_mult_18x18_unit_18_18_1 dsp_signed_mult_18x18_unit_18_18_1_inst6 (
	.clk(clk),
	.reset(reset),
	.ena(enable),
	.i_valid(valid_A_B),
	.ax(reg_A_6),
	.ay(reg_B_6),
	.bx(reg_A_7),
	.by(reg_B_7),
	.o_valid(mult_valid_6),
	.resulta(mult_C_6),
	.resultb(mult_C_7)
);
assign mult_valid_7 = mult_valid_6;
dsp_signed_mult_18x18_unit_18_18_1 dsp_signed_mult_18x18_unit_18_18_1_inst8 (
	.clk(clk),
	.reset(reset),
	.ena(enable),
	.i_valid(valid_A_B),
	.ax(reg_A_8),
	.ay(reg_B_8),
	.bx(),
	.by(),
	.o_valid(mult_valid_8),
	.resulta(mult_C_8),
	.resultb()
);
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst0 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_0),
	.in(mult_C_0),
	.o_valid(round_valid_0),
	.out(rounded_C_0)
);
assign reg_C_0 = rounded_C_0;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst1 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_1),
	.in(mult_C_1),
	.o_valid(round_valid_1),
	.out(rounded_C_1)
);
assign reg_C_1 = rounded_C_1;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst2 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_2),
	.in(mult_C_2),
	.o_valid(round_valid_2),
	.out(rounded_C_2)
);
assign reg_C_2 = rounded_C_2;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst3 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_3),
	.in(mult_C_3),
	.o_valid(round_valid_3),
	.out(rounded_C_3)
);
assign reg_C_3 = rounded_C_3;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst4 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_4),
	.in(mult_C_4),
	.o_valid(round_valid_4),
	.out(rounded_C_4)
);
assign reg_C_4 = rounded_C_4;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst5 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_5),
	.in(mult_C_5),
	.o_valid(round_valid_5),
	.out(rounded_C_5)
);
assign reg_C_5 = rounded_C_5;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst6 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_6),
	.in(mult_C_6),
	.o_valid(round_valid_6),
	.out(rounded_C_6)
);
assign reg_C_6 = rounded_C_6;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst7 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_7),
	.in(mult_C_7),
	.o_valid(round_valid_7),
	.out(rounded_C_7)
);
assign reg_C_7 = rounded_C_7;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst8 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_8),
	.in(mult_C_8),
	.o_valid(round_valid_8),
	.out(rounded_C_8)
);
assign reg_C_8 = rounded_C_8;
always @ (posedge clk) begin
	if (reset) begin
		valid_A_B <= 1'b0;
		reg_A_0 <= 0;
		reg_B_0 <= 0;
		reg_A_1 <= 0;
		reg_B_1 <= 0;
		reg_A_2 <= 0;
		reg_B_2 <= 0;
		reg_A_3 <= 0;
		reg_B_3 <= 0;
		reg_A_4 <= 0;
		reg_B_4 <= 0;
		reg_A_5 <= 0;
		reg_B_5 <= 0;
		reg_A_6 <= 0;
		reg_B_6 <= 0;
		reg_A_7 <= 0;
		reg_B_7 <= 0;
		reg_A_8 <= 0;
		reg_B_8 <= 0;
	end else if (enable) begin
		reg_A_0 <= i_A_0;
		reg_B_0 <= i_B_0;
		reg_A_1 <= i_A_1;
		reg_B_1 <= i_B_1;
		reg_A_2 <= i_A_2;
		reg_B_2 <= i_B_2;
		reg_A_3 <= i_A_3;
		reg_B_3 <= i_B_3;
		reg_A_4 <= i_A_4;
		reg_B_4 <= i_B_4;
		reg_A_5 <= i_A_5;
		reg_B_5 <= i_B_5;
		reg_A_6 <= i_A_6;
		reg_B_6 <= i_B_6;
		reg_A_7 <= i_A_7;
		reg_B_7 <= i_B_7;
		reg_A_8 <= i_A_8;
		reg_B_8 <= i_B_8;
		valid_A_B <= i_valid;
	end
end

assign o_C_0 = reg_C_0;
assign o_C_1 = reg_C_1;
assign o_C_2 = reg_C_2;
assign o_C_3 = reg_C_3;
assign o_C_4 = reg_C_4;
assign o_C_5 = reg_C_5;
assign o_C_6 = reg_C_6;
assign o_C_7 = reg_C_7;
assign o_C_8 = reg_C_8;
assign valid_C = round_valid_0;
assign o_ready = i_ready;
assign o_valid = valid_C & i_ready;

endmodule

module dsp_signed_mult_18x18_unit_18_18_1 (
	input clk,
	input reset,
	input ena,
	input i_valid,
	input [17:0] ax,
	input [17:0] ay,
	input [17:0] bx,
	input [17:0] by,
	output o_valid,
	output [36:0] resulta,
	output [36:0] resultb 
);

reg [17:0] reg_ax, reg_ay, reg_bx, reg_by;
reg [36:0] reg_resa, reg_resb;
always @(posedge clk) begin
	if (reset) begin
		reg_ax <= 0;
		reg_ay <= 0;
		reg_bx <= 0;
		reg_by <= 0;
		reg_resa <= 0;
		reg_resb <= 0;
	end else if (ena) begin
		reg_ax <= ax;
		reg_ay <= ay;
		reg_bx <= bx;
		reg_by <= by;
		reg_resa <= reg_ax * reg_ay;
		reg_resb <= reg_bx * reg_by;
	end
end

assign resulta = reg_resa;
assign resultb = reg_resb;
reg input_valid, result_valid, output_valid;
always @ (posedge clk) begin
	if (reset) begin
		output_valid <= 1'b0;
		input_valid <= 1'b0;
		result_valid <= 1'b0;
	end else if (ena) begin
		input_valid <= i_valid;
		result_valid <= input_valid;
		output_valid <= result_valid;
	end
end
assign o_valid = output_valid;

endmodule

module fp_rounding_unit_1_37_10 (
	input clk,
	input reset,
	input enable,
	input i_valid,
	input [36:0] in,
	output [36:0] out,
	output o_valid
);

reg [36:0] rounded_result;
reg [36:0] floor;
reg [36:0] ceil;
reg is_ceil;
reg floor_ceil_valid;

always @ (*) begin
	if (is_ceil) begin
		rounded_result = ceil;
	end else begin
		rounded_result = floor;
	end
end

reg valid_reg;
reg [36:0] out_reg;
always @ (posedge clk) begin
	if (reset) begin
		is_ceil <= 1'b0;
		floor_ceil_valid <= 1'b0;
		valid_reg <= 1'b0;
		floor <= 0;
		ceil <= 0;
		out_reg <= 0;
	end else if (enable) begin
		is_ceil <= in[9];
		floor <= in >>> 10;
		ceil <= (in >>> 10) + 1;
		floor_ceil_valid <= i_valid;
		out_reg <= rounded_result;
		valid_reg <= floor_ceil_valid;
	end
end

assign o_valid = valid_reg;

assign out = out_reg;

endmodule

module sum_complex_vector_unit_18_18_16_64 (
	input clk,
	input clr,
	input i_valid,
	input enable,
	input [17:0] i_real_0,
	input [17:0] i_imag_0,
	output [17:0] o_real_0,
	output [17:0] o_imag_0,
	input [17:0] i_real_1,
	input [17:0] i_imag_1,
	output [17:0] o_real_1,
	output [17:0] o_imag_1,
	input [17:0] i_real_2,
	input [17:0] i_imag_2,
	output [17:0] o_real_2,
	output [17:0] o_imag_2,
	input [17:0] i_real_3,
	input [17:0] i_imag_3,
	output [17:0] o_real_3,
	output [17:0] o_imag_3,
	input [17:0] i_real_4,
	input [17:0] i_imag_4,
	output [17:0] o_real_4,
	output [17:0] o_imag_4,
	input [17:0] i_real_5,
	input [17:0] i_imag_5,
	output [17:0] o_real_5,
	output [17:0] o_imag_5,
	input [17:0] i_real_6,
	input [17:0] i_imag_6,
	output [17:0] o_real_6,
	output [17:0] o_imag_6,
	input [17:0] i_real_7,
	input [17:0] i_imag_7,
	output [17:0] o_real_7,
	output [17:0] o_imag_7,
	input [17:0] i_real_8,
	input [17:0] i_imag_8,
	output [17:0] o_real_8,
	output [17:0] o_imag_8,
	input [17:0] i_real_9,
	input [17:0] i_imag_9,
	output [17:0] o_real_9,
	output [17:0] o_imag_9,
	input [17:0] i_real_10,
	input [17:0] i_imag_10,
	output [17:0] o_real_10,
	output [17:0] o_imag_10,
	input [17:0] i_real_11,
	input [17:0] i_imag_11,
	output [17:0] o_real_11,
	output [17:0] o_imag_11,
	input [17:0] i_real_12,
	input [17:0] i_imag_12,
	output [17:0] o_real_12,
	output [17:0] o_imag_12,
	input [17:0] i_real_13,
	input [17:0] i_imag_13,
	output [17:0] o_real_13,
	output [17:0] o_imag_13,
	input [17:0] i_real_14,
	input [17:0] i_imag_14,
	output [17:0] o_real_14,
	output [17:0] o_imag_14,
	input [17:0] i_real_15,
	input [17:0] i_imag_15,
	output [17:0] o_real_15,
	output [17:0] o_imag_15,
	output o_valid
);

reg [17:0] sum_real_0;
reg [17:0] sum_imag_0;
reg [17:0] sum_real_1;
reg [17:0] sum_imag_1;
reg [17:0] sum_real_2;
reg [17:0] sum_imag_2;
reg [17:0] sum_real_3;
reg [17:0] sum_imag_3;
reg [17:0] sum_real_4;
reg [17:0] sum_imag_4;
reg [17:0] sum_real_5;
reg [17:0] sum_imag_5;
reg [17:0] sum_real_6;
reg [17:0] sum_imag_6;
reg [17:0] sum_real_7;
reg [17:0] sum_imag_7;
reg [17:0] sum_real_8;
reg [17:0] sum_imag_8;
reg [17:0] sum_real_9;
reg [17:0] sum_imag_9;
reg [17:0] sum_real_10;
reg [17:0] sum_imag_10;
reg [17:0] sum_real_11;
reg [17:0] sum_imag_11;
reg [17:0] sum_real_12;
reg [17:0] sum_imag_12;
reg [17:0] sum_real_13;
reg [17:0] sum_imag_13;
reg [17:0] sum_real_14;
reg [17:0] sum_imag_14;
reg [17:0] sum_real_15;
reg [17:0] sum_imag_15;
reg reg_i_valid;

// Count the number data in accumulation
reg [13:0] counter;
wire counter_full;
always @ (posedge clk) begin 
	if (clr) begin
		sum_real_0 <= 0;
		sum_imag_0 <= 0;
		sum_real_1 <= 0;
		sum_imag_1 <= 0;
		sum_real_2 <= 0;
		sum_imag_2 <= 0;
		sum_real_3 <= 0;
		sum_imag_3 <= 0;
		sum_real_4 <= 0;
		sum_imag_4 <= 0;
		sum_real_5 <= 0;
		sum_imag_5 <= 0;
		sum_real_6 <= 0;
		sum_imag_6 <= 0;
		sum_real_7 <= 0;
		sum_imag_7 <= 0;
		sum_real_8 <= 0;
		sum_imag_8 <= 0;
		sum_real_9 <= 0;
		sum_imag_9 <= 0;
		sum_real_10 <= 0;
		sum_imag_10 <= 0;
		sum_real_11 <= 0;
		sum_imag_11 <= 0;
		sum_real_12 <= 0;
		sum_imag_12 <= 0;
		sum_real_13 <= 0;
		sum_imag_13 <= 0;
		sum_real_14 <= 0;
		sum_imag_14 <= 0;
		sum_real_15 <= 0;
		sum_imag_15 <= 0;
		counter <= 14'd0;
		reg_i_valid <= 1'b0;
	end else if (enable) begin
		reg_i_valid <= i_valid;
		// Accumulate the number only when data is valid
		if (i_valid) begin
			if (counter == 64)
				counter <= 1;
			else
				counter <= counter + 1;

			if (counter == 64) begin
				sum_real_0 <= i_real_0;
				sum_imag_0 <= i_imag_0;
				sum_real_1 <= i_real_1;
				sum_imag_1 <= i_imag_1;
				sum_real_2 <= i_real_2;
				sum_imag_2 <= i_imag_2;
				sum_real_3 <= i_real_3;
				sum_imag_3 <= i_imag_3;
				sum_real_4 <= i_real_4;
				sum_imag_4 <= i_imag_4;
				sum_real_5 <= i_real_5;
				sum_imag_5 <= i_imag_5;
				sum_real_6 <= i_real_6;
				sum_imag_6 <= i_imag_6;
				sum_real_7 <= i_real_7;
				sum_imag_7 <= i_imag_7;
				sum_real_8 <= i_real_8;
				sum_imag_8 <= i_imag_8;
				sum_real_9 <= i_real_9;
				sum_imag_9 <= i_imag_9;
				sum_real_10 <= i_real_10;
				sum_imag_10 <= i_imag_10;
				sum_real_11 <= i_real_11;
				sum_imag_11 <= i_imag_11;
				sum_real_12 <= i_real_12;
				sum_imag_12 <= i_imag_12;
				sum_real_13 <= i_real_13;
				sum_imag_13 <= i_imag_13;
				sum_real_14 <= i_real_14;
				sum_imag_14 <= i_imag_14;
				sum_real_15 <= i_real_15;
				sum_imag_15 <= i_imag_15;
			end else begin
				sum_real_0 <= sum_real_0 + i_real_0;
				sum_imag_0 <= sum_imag_0 + i_imag_0;
				sum_real_1 <= sum_real_1 + i_real_1;
				sum_imag_1 <= sum_imag_1 + i_imag_1;
				sum_real_2 <= sum_real_2 + i_real_2;
				sum_imag_2 <= sum_imag_2 + i_imag_2;
				sum_real_3 <= sum_real_3 + i_real_3;
				sum_imag_3 <= sum_imag_3 + i_imag_3;
				sum_real_4 <= sum_real_4 + i_real_4;
				sum_imag_4 <= sum_imag_4 + i_imag_4;
				sum_real_5 <= sum_real_5 + i_real_5;
				sum_imag_5 <= sum_imag_5 + i_imag_5;
				sum_real_6 <= sum_real_6 + i_real_6;
				sum_imag_6 <= sum_imag_6 + i_imag_6;
				sum_real_7 <= sum_real_7 + i_real_7;
				sum_imag_7 <= sum_imag_7 + i_imag_7;
				sum_real_8 <= sum_real_8 + i_real_8;
				sum_imag_8 <= sum_imag_8 + i_imag_8;
				sum_real_9 <= sum_real_9 + i_real_9;
				sum_imag_9 <= sum_imag_9 + i_imag_9;
				sum_real_10 <= sum_real_10 + i_real_10;
				sum_imag_10 <= sum_imag_10 + i_imag_10;
				sum_real_11 <= sum_real_11 + i_real_11;
				sum_imag_11 <= sum_imag_11 + i_imag_11;
				sum_real_12 <= sum_real_12 + i_real_12;
				sum_imag_12 <= sum_imag_12 + i_imag_12;
				sum_real_13 <= sum_real_13 + i_real_13;
				sum_imag_13 <= sum_imag_13 + i_imag_13;
				sum_real_14 <= sum_real_14 + i_real_14;
				sum_imag_14 <= sum_imag_14 + i_imag_14;
				sum_real_15 <= sum_real_15 + i_real_15;
				sum_imag_15 <= sum_imag_15 + i_imag_15;
			end
		end
	end
end

assign counter_full = (counter == 64);
assign o_real_0 = sum_real_0;
assign o_imag_0 = sum_imag_0;
assign o_real_1 = sum_real_1;
assign o_imag_1 = sum_imag_1;
assign o_real_2 = sum_real_2;
assign o_imag_2 = sum_imag_2;
assign o_real_3 = sum_real_3;
assign o_imag_3 = sum_imag_3;
assign o_real_4 = sum_real_4;
assign o_imag_4 = sum_imag_4;
assign o_real_5 = sum_real_5;
assign o_imag_5 = sum_imag_5;
assign o_real_6 = sum_real_6;
assign o_imag_6 = sum_imag_6;
assign o_real_7 = sum_real_7;
assign o_imag_7 = sum_imag_7;
assign o_real_8 = sum_real_8;
assign o_imag_8 = sum_imag_8;
assign o_real_9 = sum_real_9;
assign o_imag_9 = sum_imag_9;
assign o_real_10 = sum_real_10;
assign o_imag_10 = sum_imag_10;
assign o_real_11 = sum_real_11;
assign o_imag_11 = sum_imag_11;
assign o_real_12 = sum_real_12;
assign o_imag_12 = sum_imag_12;
assign o_real_13 = sum_real_13;
assign o_imag_13 = sum_imag_13;
assign o_real_14 = sum_real_14;
assign o_imag_14 = sum_imag_14;
assign o_real_15 = sum_real_15;
assign o_imag_15 = sum_imag_15;
assign o_valid = counter_full & reg_i_valid;

endmodule

module shift_register_unit_18_3 (
	input clk,
	input reset,
	input enable,
	input [17:0] in,
	output [17:0] out
);

reg [17:0] shift_registers_0;
reg [17:0] shift_registers_1;
reg [17:0] shift_registers_2;
always @ (posedge clk) begin
	if (reset) begin
		shift_registers_0 <= 18'd0;
		shift_registers_1 <= 18'd0;
		shift_registers_2 <= 18'd0;
	end else if (enable) begin
		shift_registers_0 <= in;
		shift_registers_1 <= shift_registers_0;
		shift_registers_2 <= shift_registers_1;
	end
end

assign out = shift_registers_2;

endmodule

module stage3_parameter_buffer_18_1_16_64_2048 (
	input clk,
	input reset,
	output [17:0] Wym_real_0_0,
	output [17:0] Wym_imag_0_0,
	output [17:0] Wym_real_0_1,
	output [17:0] Wym_imag_0_1,
	output [17:0] Wym_real_0_2,
	output [17:0] Wym_imag_0_2,
	output [17:0] Wym_real_0_3,
	output [17:0] Wym_imag_0_3,
	output [17:0] Wym_real_0_4,
	output [17:0] Wym_imag_0_4,
	output [17:0] Wym_real_0_5,
	output [17:0] Wym_imag_0_5,
	output [17:0] Wym_real_0_6,
	output [17:0] Wym_imag_0_6,
	output [17:0] Wym_real_0_7,
	output [17:0] Wym_imag_0_7,
	output [17:0] Wym_real_0_8,
	output [17:0] Wym_imag_0_8,
	input incr_index
);

wire [13:0] input_index_counter;
counter_63_1 counter_63_1_inst_input (
	.clk(clk),
	.reset(reset),
	.ena(incr_index),
	.count(input_index_counter)
);

wire incr_row_index;
assign incr_row_index = (input_index_counter == 63 & incr_index);
wire [13:0] weight_row_index_counter;
counter_31_1 counter_31_1_inst_weight (
	.clk(clk),
	.reset(reset),
	.ena(incr_row_index),
	.count(weight_row_index_counter)
);

reg [13:0] weight_index;
always @ (*) begin
	weight_index = weight_row_index_counter * 64 + input_index_counter;
end
weight_buffer_18_9_1_64_2048_Wym_real_half_0 weight_buffer_18_9_1_64_2048_Wym_real_half_0_inst_real (
	.clk(clk),
	.q_0_0(Wym_real_0_0),
	.q_0_1(Wym_real_0_1),
	.q_0_2(Wym_real_0_2),
	.q_0_3(Wym_real_0_3),
	.q_0_4(Wym_real_0_4),
	.q_0_5(Wym_real_0_5),
	.q_0_6(Wym_real_0_6),
	.q_0_7(Wym_real_0_7),
	.q_0_8(Wym_real_0_8),
	.index(weight_index)
);

weight_buffer_18_9_1_64_2048_Wym_imag_half_0 weight_buffer_18_9_1_64_2048_Wym_imag_half_0_inst_imag (
	.clk(clk),
	.q_0_0(Wym_imag_0_0),
	.q_0_1(Wym_imag_0_1),
	.q_0_2(Wym_imag_0_2),
	.q_0_3(Wym_imag_0_3),
	.q_0_4(Wym_imag_0_4),
	.q_0_5(Wym_imag_0_5),
	.q_0_6(Wym_imag_0_6),
	.q_0_7(Wym_imag_0_7),
	.q_0_8(Wym_imag_0_8),
	.index(weight_index)
);

endmodule

module counter_31_1 (
	input clk,
	input reset,
	input ena,
	output reg [13:0] count
);

always @ (posedge clk) begin 
	if (reset) begin
		count <= 0;
	end else if (ena) begin
		if((count + 1) <= 31) begin
			count <= count + 1;
		end else begin
			count <= 14'd0;
		end
	end
end

endmodule

module weight_buffer_18_9_1_64_2048_Wym_real_half_0 (
	input clk,
	output [17:0] q_0_0,
	output [17:0] q_0_1,
	output [17:0] q_0_2,
	output [17:0] q_0_3,
	output [17:0] q_0_4,
	output [17:0] q_0_5,
	output [17:0] q_0_6,
	output [17:0] q_0_7,
	output [17:0] q_0_8,
	input [10:0] index
);

wire [161:0] packed_result_0;
reg [10:0] addrs_0;
reg [10:0] addrs_base_0;

always @ (posedge clk) begin
	addrs_base_0 <= 0;
	addrs_0 <= index + addrs_base_0;
end

wire rom_we;
assign rom_we = 1'b0;

single_port_ram ram_inst_0 (
	.we(rom_we),
	.addr(addrs_0),
	.data(162'd0),
	.out(packed_result_0),
	.clk(clk)
);

// Unpack result
assign q_0_0 = packed_result_0[17:0];
assign q_0_1 = packed_result_0[35:18];
assign q_0_2 = packed_result_0[53:36];
assign q_0_3 = packed_result_0[71:54];
assign q_0_4 = packed_result_0[89:72];
assign q_0_5 = packed_result_0[107:90];
assign q_0_6 = packed_result_0[125:108];
assign q_0_7 = packed_result_0[143:126];
assign q_0_8 = packed_result_0[161:144];

endmodule

module weight_buffer_18_9_1_64_2048_Wym_imag_half_0 (
	input clk,
	output [17:0] q_0_0,
	output [17:0] q_0_1,
	output [17:0] q_0_2,
	output [17:0] q_0_3,
	output [17:0] q_0_4,
	output [17:0] q_0_5,
	output [17:0] q_0_6,
	output [17:0] q_0_7,
	output [17:0] q_0_8,
	input [10:0] index
);

wire [161:0] packed_result_0;
reg [10:0] addrs_0;
reg [10:0] addrs_base_0;

always @ (posedge clk) begin
	addrs_base_0 <= 0;
	addrs_0 <= index + addrs_base_0;
end

wire rom_we;
assign rom_we = 1'b0;

single_port_ram ram_inst_0 (
	.we(rom_we),
	.addr(addrs_0),
	.data(162'd0),
	.out(packed_result_0),
	.clk(clk)
);

// Unpack result
assign q_0_0 = packed_result_0[17:0];
assign q_0_1 = packed_result_0[35:18];
assign q_0_2 = packed_result_0[53:36];
assign q_0_3 = packed_result_0[71:54];
assign q_0_4 = packed_result_0[89:72];
assign q_0_5 = packed_result_0[107:90];
assign q_0_6 = packed_result_0[125:108];
assign q_0_7 = packed_result_0[143:126];
assign q_0_8 = packed_result_0[161:144];

endmodule

module C_LSTM_stage_2_18_10_16_1 (
	input clk,
	input reset,
	input enable,
	input i_valid,
	input [17:0] Ct_1_0,
	input [17:0] WixrXtYt_1_0,
	input [17:0] Wic_0,
	input [17:0] bi_0,
	input [17:0] WfxrXtYt_1_0,
	input [17:0] Wfc_0,
	input [17:0] bf_0,
	input [17:0] WoxrXtYt_1_0,
	input [17:0] Woc_0,
	input [17:0] bo_0,
	input [17:0] WcxrXtYt_1_0,
	input [17:0] bc_0,
	output [17:0] out_mt_0,
	output [17:0] out_ct_0,
	input [17:0] Ct_1_1,
	input [17:0] WixrXtYt_1_1,
	input [17:0] Wic_1,
	input [17:0] bi_1,
	input [17:0] WfxrXtYt_1_1,
	input [17:0] Wfc_1,
	input [17:0] bf_1,
	input [17:0] WoxrXtYt_1_1,
	input [17:0] Woc_1,
	input [17:0] bo_1,
	input [17:0] WcxrXtYt_1_1,
	input [17:0] bc_1,
	output [17:0] out_mt_1,
	output [17:0] out_ct_1,
	input [17:0] Ct_1_2,
	input [17:0] WixrXtYt_1_2,
	input [17:0] Wic_2,
	input [17:0] bi_2,
	input [17:0] WfxrXtYt_1_2,
	input [17:0] Wfc_2,
	input [17:0] bf_2,
	input [17:0] WoxrXtYt_1_2,
	input [17:0] Woc_2,
	input [17:0] bo_2,
	input [17:0] WcxrXtYt_1_2,
	input [17:0] bc_2,
	output [17:0] out_mt_2,
	output [17:0] out_ct_2,
	input [17:0] Ct_1_3,
	input [17:0] WixrXtYt_1_3,
	input [17:0] Wic_3,
	input [17:0] bi_3,
	input [17:0] WfxrXtYt_1_3,
	input [17:0] Wfc_3,
	input [17:0] bf_3,
	input [17:0] WoxrXtYt_1_3,
	input [17:0] Woc_3,
	input [17:0] bo_3,
	input [17:0] WcxrXtYt_1_3,
	input [17:0] bc_3,
	output [17:0] out_mt_3,
	output [17:0] out_ct_3,
	input [17:0] Ct_1_4,
	input [17:0] WixrXtYt_1_4,
	input [17:0] Wic_4,
	input [17:0] bi_4,
	input [17:0] WfxrXtYt_1_4,
	input [17:0] Wfc_4,
	input [17:0] bf_4,
	input [17:0] WoxrXtYt_1_4,
	input [17:0] Woc_4,
	input [17:0] bo_4,
	input [17:0] WcxrXtYt_1_4,
	input [17:0] bc_4,
	output [17:0] out_mt_4,
	output [17:0] out_ct_4,
	input [17:0] Ct_1_5,
	input [17:0] WixrXtYt_1_5,
	input [17:0] Wic_5,
	input [17:0] bi_5,
	input [17:0] WfxrXtYt_1_5,
	input [17:0] Wfc_5,
	input [17:0] bf_5,
	input [17:0] WoxrXtYt_1_5,
	input [17:0] Woc_5,
	input [17:0] bo_5,
	input [17:0] WcxrXtYt_1_5,
	input [17:0] bc_5,
	output [17:0] out_mt_5,
	output [17:0] out_ct_5,
	input [17:0] Ct_1_6,
	input [17:0] WixrXtYt_1_6,
	input [17:0] Wic_6,
	input [17:0] bi_6,
	input [17:0] WfxrXtYt_1_6,
	input [17:0] Wfc_6,
	input [17:0] bf_6,
	input [17:0] WoxrXtYt_1_6,
	input [17:0] Woc_6,
	input [17:0] bo_6,
	input [17:0] WcxrXtYt_1_6,
	input [17:0] bc_6,
	output [17:0] out_mt_6,
	output [17:0] out_ct_6,
	input [17:0] Ct_1_7,
	input [17:0] WixrXtYt_1_7,
	input [17:0] Wic_7,
	input [17:0] bi_7,
	input [17:0] WfxrXtYt_1_7,
	input [17:0] Wfc_7,
	input [17:0] bf_7,
	input [17:0] WoxrXtYt_1_7,
	input [17:0] Woc_7,
	input [17:0] bo_7,
	input [17:0] WcxrXtYt_1_7,
	input [17:0] bc_7,
	output [17:0] out_mt_7,
	output [17:0] out_ct_7,
	input [17:0] Ct_1_8,
	input [17:0] WixrXtYt_1_8,
	input [17:0] Wic_8,
	input [17:0] bi_8,
	input [17:0] WfxrXtYt_1_8,
	input [17:0] Wfc_8,
	input [17:0] bf_8,
	input [17:0] WoxrXtYt_1_8,
	input [17:0] Woc_8,
	input [17:0] bo_8,
	input [17:0] WcxrXtYt_1_8,
	input [17:0] bc_8,
	output [17:0] out_mt_8,
	output [17:0] out_ct_8,
	input [17:0] Ct_1_9,
	input [17:0] WixrXtYt_1_9,
	input [17:0] Wic_9,
	input [17:0] bi_9,
	input [17:0] WfxrXtYt_1_9,
	input [17:0] Wfc_9,
	input [17:0] bf_9,
	input [17:0] WoxrXtYt_1_9,
	input [17:0] Woc_9,
	input [17:0] bo_9,
	input [17:0] WcxrXtYt_1_9,
	input [17:0] bc_9,
	output [17:0] out_mt_9,
	output [17:0] out_ct_9,
	input [17:0] Ct_1_10,
	input [17:0] WixrXtYt_1_10,
	input [17:0] Wic_10,
	input [17:0] bi_10,
	input [17:0] WfxrXtYt_1_10,
	input [17:0] Wfc_10,
	input [17:0] bf_10,
	input [17:0] WoxrXtYt_1_10,
	input [17:0] Woc_10,
	input [17:0] bo_10,
	input [17:0] WcxrXtYt_1_10,
	input [17:0] bc_10,
	output [17:0] out_mt_10,
	output [17:0] out_ct_10,
	input [17:0] Ct_1_11,
	input [17:0] WixrXtYt_1_11,
	input [17:0] Wic_11,
	input [17:0] bi_11,
	input [17:0] WfxrXtYt_1_11,
	input [17:0] Wfc_11,
	input [17:0] bf_11,
	input [17:0] WoxrXtYt_1_11,
	input [17:0] Woc_11,
	input [17:0] bo_11,
	input [17:0] WcxrXtYt_1_11,
	input [17:0] bc_11,
	output [17:0] out_mt_11,
	output [17:0] out_ct_11,
	input [17:0] Ct_1_12,
	input [17:0] WixrXtYt_1_12,
	input [17:0] Wic_12,
	input [17:0] bi_12,
	input [17:0] WfxrXtYt_1_12,
	input [17:0] Wfc_12,
	input [17:0] bf_12,
	input [17:0] WoxrXtYt_1_12,
	input [17:0] Woc_12,
	input [17:0] bo_12,
	input [17:0] WcxrXtYt_1_12,
	input [17:0] bc_12,
	output [17:0] out_mt_12,
	output [17:0] out_ct_12,
	input [17:0] Ct_1_13,
	input [17:0] WixrXtYt_1_13,
	input [17:0] Wic_13,
	input [17:0] bi_13,
	input [17:0] WfxrXtYt_1_13,
	input [17:0] Wfc_13,
	input [17:0] bf_13,
	input [17:0] WoxrXtYt_1_13,
	input [17:0] Woc_13,
	input [17:0] bo_13,
	input [17:0] WcxrXtYt_1_13,
	input [17:0] bc_13,
	output [17:0] out_mt_13,
	output [17:0] out_ct_13,
	input [17:0] Ct_1_14,
	input [17:0] WixrXtYt_1_14,
	input [17:0] Wic_14,
	input [17:0] bi_14,
	input [17:0] WfxrXtYt_1_14,
	input [17:0] Wfc_14,
	input [17:0] bf_14,
	input [17:0] WoxrXtYt_1_14,
	input [17:0] Woc_14,
	input [17:0] bo_14,
	input [17:0] WcxrXtYt_1_14,
	input [17:0] bc_14,
	output [17:0] out_mt_14,
	output [17:0] out_ct_14,
	input [17:0] Ct_1_15,
	input [17:0] WixrXtYt_1_15,
	input [17:0] Wic_15,
	input [17:0] bi_15,
	input [17:0] WfxrXtYt_1_15,
	input [17:0] Wfc_15,
	input [17:0] bf_15,
	input [17:0] WoxrXtYt_1_15,
	input [17:0] Woc_15,
	input [17:0] bo_15,
	input [17:0] WcxrXtYt_1_15,
	input [17:0] bc_15,
	output [17:0] out_mt_15,
	output [17:0] out_ct_15,
	output o_valid,
	output o_ready
);

reg reg_i_valid, reg_o_valid;
reg [17:0] reg_Ct_1_0;
reg [17:0] reg_WixrXtYt_1_0;
reg [17:0] reg_Wic_0;
reg [17:0] reg_bi_0;
reg [17:0] reg_WfxrXtYt_1_0;
reg [17:0] reg_Wfc_0;
reg [17:0] reg_bf_0;
reg [17:0] reg_WoxrXtYt_1_0;
reg [17:0] reg_Woc_0;
reg [17:0] reg_bo_0;
reg [17:0] reg_WcxrXtYt_1_0;
reg [17:0] reg_bc_0;
reg [17:0] reg_out_mt_0;
reg [17:0] reg_out_ct_0;
reg [17:0] reg_Ct_1_1;
reg [17:0] reg_WixrXtYt_1_1;
reg [17:0] reg_Wic_1;
reg [17:0] reg_bi_1;
reg [17:0] reg_WfxrXtYt_1_1;
reg [17:0] reg_Wfc_1;
reg [17:0] reg_bf_1;
reg [17:0] reg_WoxrXtYt_1_1;
reg [17:0] reg_Woc_1;
reg [17:0] reg_bo_1;
reg [17:0] reg_WcxrXtYt_1_1;
reg [17:0] reg_bc_1;
reg [17:0] reg_out_mt_1;
reg [17:0] reg_out_ct_1;
reg [17:0] reg_Ct_1_2;
reg [17:0] reg_WixrXtYt_1_2;
reg [17:0] reg_Wic_2;
reg [17:0] reg_bi_2;
reg [17:0] reg_WfxrXtYt_1_2;
reg [17:0] reg_Wfc_2;
reg [17:0] reg_bf_2;
reg [17:0] reg_WoxrXtYt_1_2;
reg [17:0] reg_Woc_2;
reg [17:0] reg_bo_2;
reg [17:0] reg_WcxrXtYt_1_2;
reg [17:0] reg_bc_2;
reg [17:0] reg_out_mt_2;
reg [17:0] reg_out_ct_2;
reg [17:0] reg_Ct_1_3;
reg [17:0] reg_WixrXtYt_1_3;
reg [17:0] reg_Wic_3;
reg [17:0] reg_bi_3;
reg [17:0] reg_WfxrXtYt_1_3;
reg [17:0] reg_Wfc_3;
reg [17:0] reg_bf_3;
reg [17:0] reg_WoxrXtYt_1_3;
reg [17:0] reg_Woc_3;
reg [17:0] reg_bo_3;
reg [17:0] reg_WcxrXtYt_1_3;
reg [17:0] reg_bc_3;
reg [17:0] reg_out_mt_3;
reg [17:0] reg_out_ct_3;
reg [17:0] reg_Ct_1_4;
reg [17:0] reg_WixrXtYt_1_4;
reg [17:0] reg_Wic_4;
reg [17:0] reg_bi_4;
reg [17:0] reg_WfxrXtYt_1_4;
reg [17:0] reg_Wfc_4;
reg [17:0] reg_bf_4;
reg [17:0] reg_WoxrXtYt_1_4;
reg [17:0] reg_Woc_4;
reg [17:0] reg_bo_4;
reg [17:0] reg_WcxrXtYt_1_4;
reg [17:0] reg_bc_4;
reg [17:0] reg_out_mt_4;
reg [17:0] reg_out_ct_4;
reg [17:0] reg_Ct_1_5;
reg [17:0] reg_WixrXtYt_1_5;
reg [17:0] reg_Wic_5;
reg [17:0] reg_bi_5;
reg [17:0] reg_WfxrXtYt_1_5;
reg [17:0] reg_Wfc_5;
reg [17:0] reg_bf_5;
reg [17:0] reg_WoxrXtYt_1_5;
reg [17:0] reg_Woc_5;
reg [17:0] reg_bo_5;
reg [17:0] reg_WcxrXtYt_1_5;
reg [17:0] reg_bc_5;
reg [17:0] reg_out_mt_5;
reg [17:0] reg_out_ct_5;
reg [17:0] reg_Ct_1_6;
reg [17:0] reg_WixrXtYt_1_6;
reg [17:0] reg_Wic_6;
reg [17:0] reg_bi_6;
reg [17:0] reg_WfxrXtYt_1_6;
reg [17:0] reg_Wfc_6;
reg [17:0] reg_bf_6;
reg [17:0] reg_WoxrXtYt_1_6;
reg [17:0] reg_Woc_6;
reg [17:0] reg_bo_6;
reg [17:0] reg_WcxrXtYt_1_6;
reg [17:0] reg_bc_6;
reg [17:0] reg_out_mt_6;
reg [17:0] reg_out_ct_6;
reg [17:0] reg_Ct_1_7;
reg [17:0] reg_WixrXtYt_1_7;
reg [17:0] reg_Wic_7;
reg [17:0] reg_bi_7;
reg [17:0] reg_WfxrXtYt_1_7;
reg [17:0] reg_Wfc_7;
reg [17:0] reg_bf_7;
reg [17:0] reg_WoxrXtYt_1_7;
reg [17:0] reg_Woc_7;
reg [17:0] reg_bo_7;
reg [17:0] reg_WcxrXtYt_1_7;
reg [17:0] reg_bc_7;
reg [17:0] reg_out_mt_7;
reg [17:0] reg_out_ct_7;
reg [17:0] reg_Ct_1_8;
reg [17:0] reg_WixrXtYt_1_8;
reg [17:0] reg_Wic_8;
reg [17:0] reg_bi_8;
reg [17:0] reg_WfxrXtYt_1_8;
reg [17:0] reg_Wfc_8;
reg [17:0] reg_bf_8;
reg [17:0] reg_WoxrXtYt_1_8;
reg [17:0] reg_Woc_8;
reg [17:0] reg_bo_8;
reg [17:0] reg_WcxrXtYt_1_8;
reg [17:0] reg_bc_8;
reg [17:0] reg_out_mt_8;
reg [17:0] reg_out_ct_8;
reg [17:0] reg_Ct_1_9;
reg [17:0] reg_WixrXtYt_1_9;
reg [17:0] reg_Wic_9;
reg [17:0] reg_bi_9;
reg [17:0] reg_WfxrXtYt_1_9;
reg [17:0] reg_Wfc_9;
reg [17:0] reg_bf_9;
reg [17:0] reg_WoxrXtYt_1_9;
reg [17:0] reg_Woc_9;
reg [17:0] reg_bo_9;
reg [17:0] reg_WcxrXtYt_1_9;
reg [17:0] reg_bc_9;
reg [17:0] reg_out_mt_9;
reg [17:0] reg_out_ct_9;
reg [17:0] reg_Ct_1_10;
reg [17:0] reg_WixrXtYt_1_10;
reg [17:0] reg_Wic_10;
reg [17:0] reg_bi_10;
reg [17:0] reg_WfxrXtYt_1_10;
reg [17:0] reg_Wfc_10;
reg [17:0] reg_bf_10;
reg [17:0] reg_WoxrXtYt_1_10;
reg [17:0] reg_Woc_10;
reg [17:0] reg_bo_10;
reg [17:0] reg_WcxrXtYt_1_10;
reg [17:0] reg_bc_10;
reg [17:0] reg_out_mt_10;
reg [17:0] reg_out_ct_10;
reg [17:0] reg_Ct_1_11;
reg [17:0] reg_WixrXtYt_1_11;
reg [17:0] reg_Wic_11;
reg [17:0] reg_bi_11;
reg [17:0] reg_WfxrXtYt_1_11;
reg [17:0] reg_Wfc_11;
reg [17:0] reg_bf_11;
reg [17:0] reg_WoxrXtYt_1_11;
reg [17:0] reg_Woc_11;
reg [17:0] reg_bo_11;
reg [17:0] reg_WcxrXtYt_1_11;
reg [17:0] reg_bc_11;
reg [17:0] reg_out_mt_11;
reg [17:0] reg_out_ct_11;
reg [17:0] reg_Ct_1_12;
reg [17:0] reg_WixrXtYt_1_12;
reg [17:0] reg_Wic_12;
reg [17:0] reg_bi_12;
reg [17:0] reg_WfxrXtYt_1_12;
reg [17:0] reg_Wfc_12;
reg [17:0] reg_bf_12;
reg [17:0] reg_WoxrXtYt_1_12;
reg [17:0] reg_Woc_12;
reg [17:0] reg_bo_12;
reg [17:0] reg_WcxrXtYt_1_12;
reg [17:0] reg_bc_12;
reg [17:0] reg_out_mt_12;
reg [17:0] reg_out_ct_12;
reg [17:0] reg_Ct_1_13;
reg [17:0] reg_WixrXtYt_1_13;
reg [17:0] reg_Wic_13;
reg [17:0] reg_bi_13;
reg [17:0] reg_WfxrXtYt_1_13;
reg [17:0] reg_Wfc_13;
reg [17:0] reg_bf_13;
reg [17:0] reg_WoxrXtYt_1_13;
reg [17:0] reg_Woc_13;
reg [17:0] reg_bo_13;
reg [17:0] reg_WcxrXtYt_1_13;
reg [17:0] reg_bc_13;
reg [17:0] reg_out_mt_13;
reg [17:0] reg_out_ct_13;
reg [17:0] reg_Ct_1_14;
reg [17:0] reg_WixrXtYt_1_14;
reg [17:0] reg_Wic_14;
reg [17:0] reg_bi_14;
reg [17:0] reg_WfxrXtYt_1_14;
reg [17:0] reg_Wfc_14;
reg [17:0] reg_bf_14;
reg [17:0] reg_WoxrXtYt_1_14;
reg [17:0] reg_Woc_14;
reg [17:0] reg_bo_14;
reg [17:0] reg_WcxrXtYt_1_14;
reg [17:0] reg_bc_14;
reg [17:0] reg_out_mt_14;
reg [17:0] reg_out_ct_14;
reg [17:0] reg_Ct_1_15;
reg [17:0] reg_WixrXtYt_1_15;
reg [17:0] reg_Wic_15;
reg [17:0] reg_bi_15;
reg [17:0] reg_WfxrXtYt_1_15;
reg [17:0] reg_Wfc_15;
reg [17:0] reg_bf_15;
reg [17:0] reg_WoxrXtYt_1_15;
reg [17:0] reg_Woc_15;
reg [17:0] reg_bo_15;
reg [17:0] reg_WcxrXtYt_1_15;
reg [17:0] reg_bc_15;
reg [17:0] reg_out_mt_15;
reg [17:0] reg_out_ct_15;

wire input_gate_valid, forget_gate_valid, output_gate_valid, gt_valid;
wire it_gt_mult_valid, ft_Ct_1_mult_valid;
wire ct_valid;
wire tanh_valid_0;
wire [17:0] o_Ct_1_0;
wire [17:0] Ct_1_hold_0;
wire [17:0] o_input_gate_0;
wire [17:0] o_forget_gate_0;
wire [17:0] o_output_gate_0;
wire [17:0] ot_hold_0;
wire [17:0] o_gt_0;
wire [17:0] gt_hold_0;
wire [17:0] o_it_gt_mult_0;
wire [17:0] o_ft_Ct_1_mult_0;
wire [17:0] o_ct_0;
wire [17:0] ct_hold_0;
wire [17:0] o_tanh_0;
wire [17:0] o_mt_0;
wire tanh_valid_1;
wire [17:0] o_Ct_1_1;
wire [17:0] Ct_1_hold_1;
wire [17:0] o_input_gate_1;
wire [17:0] o_forget_gate_1;
wire [17:0] o_output_gate_1;
wire [17:0] ot_hold_1;
wire [17:0] o_gt_1;
wire [17:0] gt_hold_1;
wire [17:0] o_it_gt_mult_1;
wire [17:0] o_ft_Ct_1_mult_1;
wire [17:0] o_ct_1;
wire [17:0] ct_hold_1;
wire [17:0] o_tanh_1;
wire [17:0] o_mt_1;
wire tanh_valid_2;
wire [17:0] o_Ct_1_2;
wire [17:0] Ct_1_hold_2;
wire [17:0] o_input_gate_2;
wire [17:0] o_forget_gate_2;
wire [17:0] o_output_gate_2;
wire [17:0] ot_hold_2;
wire [17:0] o_gt_2;
wire [17:0] gt_hold_2;
wire [17:0] o_it_gt_mult_2;
wire [17:0] o_ft_Ct_1_mult_2;
wire [17:0] o_ct_2;
wire [17:0] ct_hold_2;
wire [17:0] o_tanh_2;
wire [17:0] o_mt_2;
wire tanh_valid_3;
wire [17:0] o_Ct_1_3;
wire [17:0] Ct_1_hold_3;
wire [17:0] o_input_gate_3;
wire [17:0] o_forget_gate_3;
wire [17:0] o_output_gate_3;
wire [17:0] ot_hold_3;
wire [17:0] o_gt_3;
wire [17:0] gt_hold_3;
wire [17:0] o_it_gt_mult_3;
wire [17:0] o_ft_Ct_1_mult_3;
wire [17:0] o_ct_3;
wire [17:0] ct_hold_3;
wire [17:0] o_tanh_3;
wire [17:0] o_mt_3;
wire tanh_valid_4;
wire [17:0] o_Ct_1_4;
wire [17:0] Ct_1_hold_4;
wire [17:0] o_input_gate_4;
wire [17:0] o_forget_gate_4;
wire [17:0] o_output_gate_4;
wire [17:0] ot_hold_4;
wire [17:0] o_gt_4;
wire [17:0] gt_hold_4;
wire [17:0] o_it_gt_mult_4;
wire [17:0] o_ft_Ct_1_mult_4;
wire [17:0] o_ct_4;
wire [17:0] ct_hold_4;
wire [17:0] o_tanh_4;
wire [17:0] o_mt_4;
wire tanh_valid_5;
wire [17:0] o_Ct_1_5;
wire [17:0] Ct_1_hold_5;
wire [17:0] o_input_gate_5;
wire [17:0] o_forget_gate_5;
wire [17:0] o_output_gate_5;
wire [17:0] ot_hold_5;
wire [17:0] o_gt_5;
wire [17:0] gt_hold_5;
wire [17:0] o_it_gt_mult_5;
wire [17:0] o_ft_Ct_1_mult_5;
wire [17:0] o_ct_5;
wire [17:0] ct_hold_5;
wire [17:0] o_tanh_5;
wire [17:0] o_mt_5;
wire tanh_valid_6;
wire [17:0] o_Ct_1_6;
wire [17:0] Ct_1_hold_6;
wire [17:0] o_input_gate_6;
wire [17:0] o_forget_gate_6;
wire [17:0] o_output_gate_6;
wire [17:0] ot_hold_6;
wire [17:0] o_gt_6;
wire [17:0] gt_hold_6;
wire [17:0] o_it_gt_mult_6;
wire [17:0] o_ft_Ct_1_mult_6;
wire [17:0] o_ct_6;
wire [17:0] ct_hold_6;
wire [17:0] o_tanh_6;
wire [17:0] o_mt_6;
wire tanh_valid_7;
wire [17:0] o_Ct_1_7;
wire [17:0] Ct_1_hold_7;
wire [17:0] o_input_gate_7;
wire [17:0] o_forget_gate_7;
wire [17:0] o_output_gate_7;
wire [17:0] ot_hold_7;
wire [17:0] o_gt_7;
wire [17:0] gt_hold_7;
wire [17:0] o_it_gt_mult_7;
wire [17:0] o_ft_Ct_1_mult_7;
wire [17:0] o_ct_7;
wire [17:0] ct_hold_7;
wire [17:0] o_tanh_7;
wire [17:0] o_mt_7;
wire tanh_valid_8;
wire [17:0] o_Ct_1_8;
wire [17:0] Ct_1_hold_8;
wire [17:0] o_input_gate_8;
wire [17:0] o_forget_gate_8;
wire [17:0] o_output_gate_8;
wire [17:0] ot_hold_8;
wire [17:0] o_gt_8;
wire [17:0] gt_hold_8;
wire [17:0] o_it_gt_mult_8;
wire [17:0] o_ft_Ct_1_mult_8;
wire [17:0] o_ct_8;
wire [17:0] ct_hold_8;
wire [17:0] o_tanh_8;
wire [17:0] o_mt_8;
wire tanh_valid_9;
wire [17:0] o_Ct_1_9;
wire [17:0] Ct_1_hold_9;
wire [17:0] o_input_gate_9;
wire [17:0] o_forget_gate_9;
wire [17:0] o_output_gate_9;
wire [17:0] ot_hold_9;
wire [17:0] o_gt_9;
wire [17:0] gt_hold_9;
wire [17:0] o_it_gt_mult_9;
wire [17:0] o_ft_Ct_1_mult_9;
wire [17:0] o_ct_9;
wire [17:0] ct_hold_9;
wire [17:0] o_tanh_9;
wire [17:0] o_mt_9;
wire tanh_valid_10;
wire [17:0] o_Ct_1_10;
wire [17:0] Ct_1_hold_10;
wire [17:0] o_input_gate_10;
wire [17:0] o_forget_gate_10;
wire [17:0] o_output_gate_10;
wire [17:0] ot_hold_10;
wire [17:0] o_gt_10;
wire [17:0] gt_hold_10;
wire [17:0] o_it_gt_mult_10;
wire [17:0] o_ft_Ct_1_mult_10;
wire [17:0] o_ct_10;
wire [17:0] ct_hold_10;
wire [17:0] o_tanh_10;
wire [17:0] o_mt_10;
wire tanh_valid_11;
wire [17:0] o_Ct_1_11;
wire [17:0] Ct_1_hold_11;
wire [17:0] o_input_gate_11;
wire [17:0] o_forget_gate_11;
wire [17:0] o_output_gate_11;
wire [17:0] ot_hold_11;
wire [17:0] o_gt_11;
wire [17:0] gt_hold_11;
wire [17:0] o_it_gt_mult_11;
wire [17:0] o_ft_Ct_1_mult_11;
wire [17:0] o_ct_11;
wire [17:0] ct_hold_11;
wire [17:0] o_tanh_11;
wire [17:0] o_mt_11;
wire tanh_valid_12;
wire [17:0] o_Ct_1_12;
wire [17:0] Ct_1_hold_12;
wire [17:0] o_input_gate_12;
wire [17:0] o_forget_gate_12;
wire [17:0] o_output_gate_12;
wire [17:0] ot_hold_12;
wire [17:0] o_gt_12;
wire [17:0] gt_hold_12;
wire [17:0] o_it_gt_mult_12;
wire [17:0] o_ft_Ct_1_mult_12;
wire [17:0] o_ct_12;
wire [17:0] ct_hold_12;
wire [17:0] o_tanh_12;
wire [17:0] o_mt_12;
wire tanh_valid_13;
wire [17:0] o_Ct_1_13;
wire [17:0] Ct_1_hold_13;
wire [17:0] o_input_gate_13;
wire [17:0] o_forget_gate_13;
wire [17:0] o_output_gate_13;
wire [17:0] ot_hold_13;
wire [17:0] o_gt_13;
wire [17:0] gt_hold_13;
wire [17:0] o_it_gt_mult_13;
wire [17:0] o_ft_Ct_1_mult_13;
wire [17:0] o_ct_13;
wire [17:0] ct_hold_13;
wire [17:0] o_tanh_13;
wire [17:0] o_mt_13;
wire tanh_valid_14;
wire [17:0] o_Ct_1_14;
wire [17:0] Ct_1_hold_14;
wire [17:0] o_input_gate_14;
wire [17:0] o_forget_gate_14;
wire [17:0] o_output_gate_14;
wire [17:0] ot_hold_14;
wire [17:0] o_gt_14;
wire [17:0] gt_hold_14;
wire [17:0] o_it_gt_mult_14;
wire [17:0] o_ft_Ct_1_mult_14;
wire [17:0] o_ct_14;
wire [17:0] ct_hold_14;
wire [17:0] o_tanh_14;
wire [17:0] o_mt_14;
wire tanh_valid_15;
wire [17:0] o_Ct_1_15;
wire [17:0] Ct_1_hold_15;
wire [17:0] o_input_gate_15;
wire [17:0] o_forget_gate_15;
wire [17:0] o_output_gate_15;
wire [17:0] ot_hold_15;
wire [17:0] o_gt_15;
wire [17:0] gt_hold_15;
wire [17:0] o_it_gt_mult_15;
wire [17:0] o_ft_Ct_1_mult_15;
wire [17:0] o_ct_15;
wire [17:0] ct_hold_15;
wire [17:0] o_tanh_15;
wire [17:0] o_mt_15;
wire mt_valid;

lstm_gate_18_10_16_1 lstm_gate_18_10_16_1_input (
	.clk(clk),
	.reset(reset),
	.i_ready(enable),
	.i_valid(reg_i_valid),
	.stage1_result_0(reg_WixrXtYt_1_0),
	.weight_0(reg_Wic_0),
	.Ct_1_0(reg_Ct_1_0),
	.bias_0(reg_bi_0),
	.gate_output_0(o_input_gate_0),
	.stage1_result_1(reg_WixrXtYt_1_1),
	.weight_1(reg_Wic_1),
	.Ct_1_1(reg_Ct_1_1),
	.bias_1(reg_bi_1),
	.gate_output_1(o_input_gate_1),
	.stage1_result_2(reg_WixrXtYt_1_2),
	.weight_2(reg_Wic_2),
	.Ct_1_2(reg_Ct_1_2),
	.bias_2(reg_bi_2),
	.gate_output_2(o_input_gate_2),
	.stage1_result_3(reg_WixrXtYt_1_3),
	.weight_3(reg_Wic_3),
	.Ct_1_3(reg_Ct_1_3),
	.bias_3(reg_bi_3),
	.gate_output_3(o_input_gate_3),
	.stage1_result_4(reg_WixrXtYt_1_4),
	.weight_4(reg_Wic_4),
	.Ct_1_4(reg_Ct_1_4),
	.bias_4(reg_bi_4),
	.gate_output_4(o_input_gate_4),
	.stage1_result_5(reg_WixrXtYt_1_5),
	.weight_5(reg_Wic_5),
	.Ct_1_5(reg_Ct_1_5),
	.bias_5(reg_bi_5),
	.gate_output_5(o_input_gate_5),
	.stage1_result_6(reg_WixrXtYt_1_6),
	.weight_6(reg_Wic_6),
	.Ct_1_6(reg_Ct_1_6),
	.bias_6(reg_bi_6),
	.gate_output_6(o_input_gate_6),
	.stage1_result_7(reg_WixrXtYt_1_7),
	.weight_7(reg_Wic_7),
	.Ct_1_7(reg_Ct_1_7),
	.bias_7(reg_bi_7),
	.gate_output_7(o_input_gate_7),
	.stage1_result_8(reg_WixrXtYt_1_8),
	.weight_8(reg_Wic_8),
	.Ct_1_8(reg_Ct_1_8),
	.bias_8(reg_bi_8),
	.gate_output_8(o_input_gate_8),
	.stage1_result_9(reg_WixrXtYt_1_9),
	.weight_9(reg_Wic_9),
	.Ct_1_9(reg_Ct_1_9),
	.bias_9(reg_bi_9),
	.gate_output_9(o_input_gate_9),
	.stage1_result_10(reg_WixrXtYt_1_10),
	.weight_10(reg_Wic_10),
	.Ct_1_10(reg_Ct_1_10),
	.bias_10(reg_bi_10),
	.gate_output_10(o_input_gate_10),
	.stage1_result_11(reg_WixrXtYt_1_11),
	.weight_11(reg_Wic_11),
	.Ct_1_11(reg_Ct_1_11),
	.bias_11(reg_bi_11),
	.gate_output_11(o_input_gate_11),
	.stage1_result_12(reg_WixrXtYt_1_12),
	.weight_12(reg_Wic_12),
	.Ct_1_12(reg_Ct_1_12),
	.bias_12(reg_bi_12),
	.gate_output_12(o_input_gate_12),
	.stage1_result_13(reg_WixrXtYt_1_13),
	.weight_13(reg_Wic_13),
	.Ct_1_13(reg_Ct_1_13),
	.bias_13(reg_bi_13),
	.gate_output_13(o_input_gate_13),
	.stage1_result_14(reg_WixrXtYt_1_14),
	.weight_14(reg_Wic_14),
	.Ct_1_14(reg_Ct_1_14),
	.bias_14(reg_bi_14),
	.gate_output_14(o_input_gate_14),
	.stage1_result_15(reg_WixrXtYt_1_15),
	.weight_15(reg_Wic_15),
	.Ct_1_15(reg_Ct_1_15),
	.bias_15(reg_bi_15),
	.gate_output_15(o_input_gate_15),
	.o_valid(input_gate_valid),
	.o_ready()
);

lstm_gate_18_10_16_1 lstm_gate_18_10_16_1_forget (
	.clk(clk),
	.reset(reset),
	.i_ready(enable),
	.i_valid(reg_i_valid),
	.stage1_result_0(reg_WfxrXtYt_1_0),
	.weight_0(reg_Wfc_0),
	.Ct_1_0(reg_Ct_1_0),
	.bias_0(reg_bf_0),
	.gate_output_0(o_forget_gate_0),
	.stage1_result_1(reg_WfxrXtYt_1_1),
	.weight_1(reg_Wfc_1),
	.Ct_1_1(reg_Ct_1_1),
	.bias_1(reg_bf_1),
	.gate_output_1(o_forget_gate_1),
	.stage1_result_2(reg_WfxrXtYt_1_2),
	.weight_2(reg_Wfc_2),
	.Ct_1_2(reg_Ct_1_2),
	.bias_2(reg_bf_2),
	.gate_output_2(o_forget_gate_2),
	.stage1_result_3(reg_WfxrXtYt_1_3),
	.weight_3(reg_Wfc_3),
	.Ct_1_3(reg_Ct_1_3),
	.bias_3(reg_bf_3),
	.gate_output_3(o_forget_gate_3),
	.stage1_result_4(reg_WfxrXtYt_1_4),
	.weight_4(reg_Wfc_4),
	.Ct_1_4(reg_Ct_1_4),
	.bias_4(reg_bf_4),
	.gate_output_4(o_forget_gate_4),
	.stage1_result_5(reg_WfxrXtYt_1_5),
	.weight_5(reg_Wfc_5),
	.Ct_1_5(reg_Ct_1_5),
	.bias_5(reg_bf_5),
	.gate_output_5(o_forget_gate_5),
	.stage1_result_6(reg_WfxrXtYt_1_6),
	.weight_6(reg_Wfc_6),
	.Ct_1_6(reg_Ct_1_6),
	.bias_6(reg_bf_6),
	.gate_output_6(o_forget_gate_6),
	.stage1_result_7(reg_WfxrXtYt_1_7),
	.weight_7(reg_Wfc_7),
	.Ct_1_7(reg_Ct_1_7),
	.bias_7(reg_bf_7),
	.gate_output_7(o_forget_gate_7),
	.stage1_result_8(reg_WfxrXtYt_1_8),
	.weight_8(reg_Wfc_8),
	.Ct_1_8(reg_Ct_1_8),
	.bias_8(reg_bf_8),
	.gate_output_8(o_forget_gate_8),
	.stage1_result_9(reg_WfxrXtYt_1_9),
	.weight_9(reg_Wfc_9),
	.Ct_1_9(reg_Ct_1_9),
	.bias_9(reg_bf_9),
	.gate_output_9(o_forget_gate_9),
	.stage1_result_10(reg_WfxrXtYt_1_10),
	.weight_10(reg_Wfc_10),
	.Ct_1_10(reg_Ct_1_10),
	.bias_10(reg_bf_10),
	.gate_output_10(o_forget_gate_10),
	.stage1_result_11(reg_WfxrXtYt_1_11),
	.weight_11(reg_Wfc_11),
	.Ct_1_11(reg_Ct_1_11),
	.bias_11(reg_bf_11),
	.gate_output_11(o_forget_gate_11),
	.stage1_result_12(reg_WfxrXtYt_1_12),
	.weight_12(reg_Wfc_12),
	.Ct_1_12(reg_Ct_1_12),
	.bias_12(reg_bf_12),
	.gate_output_12(o_forget_gate_12),
	.stage1_result_13(reg_WfxrXtYt_1_13),
	.weight_13(reg_Wfc_13),
	.Ct_1_13(reg_Ct_1_13),
	.bias_13(reg_bf_13),
	.gate_output_13(o_forget_gate_13),
	.stage1_result_14(reg_WfxrXtYt_1_14),
	.weight_14(reg_Wfc_14),
	.Ct_1_14(reg_Ct_1_14),
	.bias_14(reg_bf_14),
	.gate_output_14(o_forget_gate_14),
	.stage1_result_15(reg_WfxrXtYt_1_15),
	.weight_15(reg_Wfc_15),
	.Ct_1_15(reg_Ct_1_15),
	.bias_15(reg_bf_15),
	.gate_output_15(o_forget_gate_15),
	.o_valid(forget_gate_valid),
	.o_ready()
);

lstm_gate_18_10_16_1 lstm_gate_18_10_16_1_output (
	.clk(clk),
	.reset(reset),
	.i_ready(enable),
	.i_valid(reg_i_valid),
	.stage1_result_0(reg_WoxrXtYt_1_0),
	.weight_0(reg_Woc_0),
	.Ct_1_0(reg_Ct_1_0),
	.bias_0(reg_bo_0),
	.gate_output_0(o_output_gate_0),
	.stage1_result_1(reg_WoxrXtYt_1_1),
	.weight_1(reg_Woc_1),
	.Ct_1_1(reg_Ct_1_1),
	.bias_1(reg_bo_1),
	.gate_output_1(o_output_gate_1),
	.stage1_result_2(reg_WoxrXtYt_1_2),
	.weight_2(reg_Woc_2),
	.Ct_1_2(reg_Ct_1_2),
	.bias_2(reg_bo_2),
	.gate_output_2(o_output_gate_2),
	.stage1_result_3(reg_WoxrXtYt_1_3),
	.weight_3(reg_Woc_3),
	.Ct_1_3(reg_Ct_1_3),
	.bias_3(reg_bo_3),
	.gate_output_3(o_output_gate_3),
	.stage1_result_4(reg_WoxrXtYt_1_4),
	.weight_4(reg_Woc_4),
	.Ct_1_4(reg_Ct_1_4),
	.bias_4(reg_bo_4),
	.gate_output_4(o_output_gate_4),
	.stage1_result_5(reg_WoxrXtYt_1_5),
	.weight_5(reg_Woc_5),
	.Ct_1_5(reg_Ct_1_5),
	.bias_5(reg_bo_5),
	.gate_output_5(o_output_gate_5),
	.stage1_result_6(reg_WoxrXtYt_1_6),
	.weight_6(reg_Woc_6),
	.Ct_1_6(reg_Ct_1_6),
	.bias_6(reg_bo_6),
	.gate_output_6(o_output_gate_6),
	.stage1_result_7(reg_WoxrXtYt_1_7),
	.weight_7(reg_Woc_7),
	.Ct_1_7(reg_Ct_1_7),
	.bias_7(reg_bo_7),
	.gate_output_7(o_output_gate_7),
	.stage1_result_8(reg_WoxrXtYt_1_8),
	.weight_8(reg_Woc_8),
	.Ct_1_8(reg_Ct_1_8),
	.bias_8(reg_bo_8),
	.gate_output_8(o_output_gate_8),
	.stage1_result_9(reg_WoxrXtYt_1_9),
	.weight_9(reg_Woc_9),
	.Ct_1_9(reg_Ct_1_9),
	.bias_9(reg_bo_9),
	.gate_output_9(o_output_gate_9),
	.stage1_result_10(reg_WoxrXtYt_1_10),
	.weight_10(reg_Woc_10),
	.Ct_1_10(reg_Ct_1_10),
	.bias_10(reg_bo_10),
	.gate_output_10(o_output_gate_10),
	.stage1_result_11(reg_WoxrXtYt_1_11),
	.weight_11(reg_Woc_11),
	.Ct_1_11(reg_Ct_1_11),
	.bias_11(reg_bo_11),
	.gate_output_11(o_output_gate_11),
	.stage1_result_12(reg_WoxrXtYt_1_12),
	.weight_12(reg_Woc_12),
	.Ct_1_12(reg_Ct_1_12),
	.bias_12(reg_bo_12),
	.gate_output_12(o_output_gate_12),
	.stage1_result_13(reg_WoxrXtYt_1_13),
	.weight_13(reg_Woc_13),
	.Ct_1_13(reg_Ct_1_13),
	.bias_13(reg_bo_13),
	.gate_output_13(o_output_gate_13),
	.stage1_result_14(reg_WoxrXtYt_1_14),
	.weight_14(reg_Woc_14),
	.Ct_1_14(reg_Ct_1_14),
	.bias_14(reg_bo_14),
	.gate_output_14(o_output_gate_14),
	.stage1_result_15(reg_WoxrXtYt_1_15),
	.weight_15(reg_Woc_15),
	.Ct_1_15(reg_Ct_1_15),
	.bias_15(reg_bo_15),
	.gate_output_15(o_output_gate_15),
	.o_valid(output_gate_valid),
	.o_ready()
);

output_activation_18_10_16_1 output_activation_18_10_16_1_inst (
	.clk(clk),
	.reset(reset),
	.i_ready(enable),
	.i_valid(reg_i_valid),
	.stage1_result_0(reg_WcxrXtYt_1_0),
	.bias_0(reg_bc_0),
	.output_value_0(o_gt_0),
	.stage1_result_1(reg_WcxrXtYt_1_1),
	.bias_1(reg_bc_1),
	.output_value_1(o_gt_1),
	.stage1_result_2(reg_WcxrXtYt_1_2),
	.bias_2(reg_bc_2),
	.output_value_2(o_gt_2),
	.stage1_result_3(reg_WcxrXtYt_1_3),
	.bias_3(reg_bc_3),
	.output_value_3(o_gt_3),
	.stage1_result_4(reg_WcxrXtYt_1_4),
	.bias_4(reg_bc_4),
	.output_value_4(o_gt_4),
	.stage1_result_5(reg_WcxrXtYt_1_5),
	.bias_5(reg_bc_5),
	.output_value_5(o_gt_5),
	.stage1_result_6(reg_WcxrXtYt_1_6),
	.bias_6(reg_bc_6),
	.output_value_6(o_gt_6),
	.stage1_result_7(reg_WcxrXtYt_1_7),
	.bias_7(reg_bc_7),
	.output_value_7(o_gt_7),
	.stage1_result_8(reg_WcxrXtYt_1_8),
	.bias_8(reg_bc_8),
	.output_value_8(o_gt_8),
	.stage1_result_9(reg_WcxrXtYt_1_9),
	.bias_9(reg_bc_9),
	.output_value_9(o_gt_9),
	.stage1_result_10(reg_WcxrXtYt_1_10),
	.bias_10(reg_bc_10),
	.output_value_10(o_gt_10),
	.stage1_result_11(reg_WcxrXtYt_1_11),
	.bias_11(reg_bc_11),
	.output_value_11(o_gt_11),
	.stage1_result_12(reg_WcxrXtYt_1_12),
	.bias_12(reg_bc_12),
	.output_value_12(o_gt_12),
	.stage1_result_13(reg_WcxrXtYt_1_13),
	.bias_13(reg_bc_13),
	.output_value_13(o_gt_13),
	.stage1_result_14(reg_WcxrXtYt_1_14),
	.bias_14(reg_bc_14),
	.output_value_14(o_gt_14),
	.stage1_result_15(reg_WcxrXtYt_1_15),
	.bias_15(reg_bc_15),
	.output_value_15(o_gt_15),
	.o_valid(gt_valid),
	.o_ready()
);

shift_register_group_18_16_6 shift_register_group_18_16_6_eltwisemult (
	.clk(clk),
	.enable(enable),
	.in_0(o_gt_0),
	.out_0(gt_hold_0),
	.in_1(o_gt_1),
	.out_1(gt_hold_1),
	.in_2(o_gt_2),
	.out_2(gt_hold_2),
	.in_3(o_gt_3),
	.out_3(gt_hold_3),
	.in_4(o_gt_4),
	.out_4(gt_hold_4),
	.in_5(o_gt_5),
	.out_5(gt_hold_5),
	.in_6(o_gt_6),
	.out_6(gt_hold_6),
	.in_7(o_gt_7),
	.out_7(gt_hold_7),
	.in_8(o_gt_8),
	.out_8(gt_hold_8),
	.in_9(o_gt_9),
	.out_9(gt_hold_9),
	.in_10(o_gt_10),
	.out_10(gt_hold_10),
	.in_11(o_gt_11),
	.out_11(gt_hold_11),
	.in_12(o_gt_12),
	.out_12(gt_hold_12),
	.in_13(o_gt_13),
	.out_13(gt_hold_13),
	.in_14(o_gt_14),
	.out_14(gt_hold_14),
	.in_15(o_gt_15),
	.out_15(gt_hold_15),
	.reset(reset)
);

elementwise_mult_core_18_18_10_16_1 elementwise_mult_core_18_18_10_16_1_it_gt_mult (
	.clk(clk),
	.reset(reset),
	.i_valid(forget_gate_valid),
	.i_ready(enable),
	.i_A_0(o_input_gate_0),
	.i_B_0(gt_hold_0),
	.o_C_0(o_it_gt_mult_0),
	.i_A_1(o_input_gate_1),
	.i_B_1(gt_hold_1),
	.o_C_1(o_it_gt_mult_1),
	.i_A_2(o_input_gate_2),
	.i_B_2(gt_hold_2),
	.o_C_2(o_it_gt_mult_2),
	.i_A_3(o_input_gate_3),
	.i_B_3(gt_hold_3),
	.o_C_3(o_it_gt_mult_3),
	.i_A_4(o_input_gate_4),
	.i_B_4(gt_hold_4),
	.o_C_4(o_it_gt_mult_4),
	.i_A_5(o_input_gate_5),
	.i_B_5(gt_hold_5),
	.o_C_5(o_it_gt_mult_5),
	.i_A_6(o_input_gate_6),
	.i_B_6(gt_hold_6),
	.o_C_6(o_it_gt_mult_6),
	.i_A_7(o_input_gate_7),
	.i_B_7(gt_hold_7),
	.o_C_7(o_it_gt_mult_7),
	.i_A_8(o_input_gate_8),
	.i_B_8(gt_hold_8),
	.o_C_8(o_it_gt_mult_8),
	.i_A_9(o_input_gate_9),
	.i_B_9(gt_hold_9),
	.o_C_9(o_it_gt_mult_9),
	.i_A_10(o_input_gate_10),
	.i_B_10(gt_hold_10),
	.o_C_10(o_it_gt_mult_10),
	.i_A_11(o_input_gate_11),
	.i_B_11(gt_hold_11),
	.o_C_11(o_it_gt_mult_11),
	.i_A_12(o_input_gate_12),
	.i_B_12(gt_hold_12),
	.o_C_12(o_it_gt_mult_12),
	.i_A_13(o_input_gate_13),
	.i_B_13(gt_hold_13),
	.o_C_13(o_it_gt_mult_13),
	.i_A_14(o_input_gate_14),
	.i_B_14(gt_hold_14),
	.o_C_14(o_it_gt_mult_14),
	.i_A_15(o_input_gate_15),
	.i_B_15(gt_hold_15),
	.o_C_15(o_it_gt_mult_15),
	.o_valid(it_gt_mult_valid),
	.o_ready()
);

assign o_Ct_1_0 = reg_Ct_1_0;
assign o_Ct_1_1 = reg_Ct_1_1;
assign o_Ct_1_2 = reg_Ct_1_2;
assign o_Ct_1_3 = reg_Ct_1_3;
assign o_Ct_1_4 = reg_Ct_1_4;
assign o_Ct_1_5 = reg_Ct_1_5;
assign o_Ct_1_6 = reg_Ct_1_6;
assign o_Ct_1_7 = reg_Ct_1_7;
assign o_Ct_1_8 = reg_Ct_1_8;
assign o_Ct_1_9 = reg_Ct_1_9;
assign o_Ct_1_10 = reg_Ct_1_10;
assign o_Ct_1_11 = reg_Ct_1_11;
assign o_Ct_1_12 = reg_Ct_1_12;
assign o_Ct_1_13 = reg_Ct_1_13;
assign o_Ct_1_14 = reg_Ct_1_14;
assign o_Ct_1_15 = reg_Ct_1_15;
shift_register_group_18_16_18 shift_register_group_18_16_18_lstm_gate (
	.clk(clk),
	.enable(enable),
	.in_0(o_Ct_1_0),
	.out_0(Ct_1_hold_0),
	.in_1(o_Ct_1_1),
	.out_1(Ct_1_hold_1),
	.in_2(o_Ct_1_2),
	.out_2(Ct_1_hold_2),
	.in_3(o_Ct_1_3),
	.out_3(Ct_1_hold_3),
	.in_4(o_Ct_1_4),
	.out_4(Ct_1_hold_4),
	.in_5(o_Ct_1_5),
	.out_5(Ct_1_hold_5),
	.in_6(o_Ct_1_6),
	.out_6(Ct_1_hold_6),
	.in_7(o_Ct_1_7),
	.out_7(Ct_1_hold_7),
	.in_8(o_Ct_1_8),
	.out_8(Ct_1_hold_8),
	.in_9(o_Ct_1_9),
	.out_9(Ct_1_hold_9),
	.in_10(o_Ct_1_10),
	.out_10(Ct_1_hold_10),
	.in_11(o_Ct_1_11),
	.out_11(Ct_1_hold_11),
	.in_12(o_Ct_1_12),
	.out_12(Ct_1_hold_12),
	.in_13(o_Ct_1_13),
	.out_13(Ct_1_hold_13),
	.in_14(o_Ct_1_14),
	.out_14(Ct_1_hold_14),
	.in_15(o_Ct_1_15),
	.out_15(Ct_1_hold_15),
	.reset(reset)
);

elementwise_mult_core_18_18_10_16_1 elementwise_mult_core_18_18_10_16_1_ft_Ct_1_mult (
	.clk(clk),
	.reset(reset),
	.i_valid(forget_gate_valid),
	.i_ready(enable),
	.i_A_0(Ct_1_hold_0),
	.i_B_0(o_forget_gate_0),
	.o_C_0(o_ft_Ct_1_mult_0),
	.i_A_1(Ct_1_hold_1),
	.i_B_1(o_forget_gate_1),
	.o_C_1(o_ft_Ct_1_mult_1),
	.i_A_2(Ct_1_hold_2),
	.i_B_2(o_forget_gate_2),
	.o_C_2(o_ft_Ct_1_mult_2),
	.i_A_3(Ct_1_hold_3),
	.i_B_3(o_forget_gate_3),
	.o_C_3(o_ft_Ct_1_mult_3),
	.i_A_4(Ct_1_hold_4),
	.i_B_4(o_forget_gate_4),
	.o_C_4(o_ft_Ct_1_mult_4),
	.i_A_5(Ct_1_hold_5),
	.i_B_5(o_forget_gate_5),
	.o_C_5(o_ft_Ct_1_mult_5),
	.i_A_6(Ct_1_hold_6),
	.i_B_6(o_forget_gate_6),
	.o_C_6(o_ft_Ct_1_mult_6),
	.i_A_7(Ct_1_hold_7),
	.i_B_7(o_forget_gate_7),
	.o_C_7(o_ft_Ct_1_mult_7),
	.i_A_8(Ct_1_hold_8),
	.i_B_8(o_forget_gate_8),
	.o_C_8(o_ft_Ct_1_mult_8),
	.i_A_9(Ct_1_hold_9),
	.i_B_9(o_forget_gate_9),
	.o_C_9(o_ft_Ct_1_mult_9),
	.i_A_10(Ct_1_hold_10),
	.i_B_10(o_forget_gate_10),
	.o_C_10(o_ft_Ct_1_mult_10),
	.i_A_11(Ct_1_hold_11),
	.i_B_11(o_forget_gate_11),
	.o_C_11(o_ft_Ct_1_mult_11),
	.i_A_12(Ct_1_hold_12),
	.i_B_12(o_forget_gate_12),
	.o_C_12(o_ft_Ct_1_mult_12),
	.i_A_13(Ct_1_hold_13),
	.i_B_13(o_forget_gate_13),
	.o_C_13(o_ft_Ct_1_mult_13),
	.i_A_14(Ct_1_hold_14),
	.i_B_14(o_forget_gate_14),
	.o_C_14(o_ft_Ct_1_mult_14),
	.i_A_15(Ct_1_hold_15),
	.i_B_15(o_forget_gate_15),
	.o_C_15(o_ft_Ct_1_mult_15),
	.o_valid(ft_Ct_1_mult_valid),
	.o_ready()
);

wire eltwise_add_core_valid;
assign eltwise_add_core_valid = ft_Ct_1_mult_valid & it_gt_mult_valid;

elementwise_add_core_18_18_16 elementwise_add_core_18_18_16_inst (
	.clk(clk),
	.reset(reset),
	.i_valid(eltwise_add_core_valid),
	.i_ready(enable),
	.i_A_0(o_it_gt_mult_0),
	.i_B_0(o_ft_Ct_1_mult_0),
	.o_C_0(o_ct_0),
	.i_A_1(o_it_gt_mult_1),
	.i_B_1(o_ft_Ct_1_mult_1),
	.o_C_1(o_ct_1),
	.i_A_2(o_it_gt_mult_2),
	.i_B_2(o_ft_Ct_1_mult_2),
	.o_C_2(o_ct_2),
	.i_A_3(o_it_gt_mult_3),
	.i_B_3(o_ft_Ct_1_mult_3),
	.o_C_3(o_ct_3),
	.i_A_4(o_it_gt_mult_4),
	.i_B_4(o_ft_Ct_1_mult_4),
	.o_C_4(o_ct_4),
	.i_A_5(o_it_gt_mult_5),
	.i_B_5(o_ft_Ct_1_mult_5),
	.o_C_5(o_ct_5),
	.i_A_6(o_it_gt_mult_6),
	.i_B_6(o_ft_Ct_1_mult_6),
	.o_C_6(o_ct_6),
	.i_A_7(o_it_gt_mult_7),
	.i_B_7(o_ft_Ct_1_mult_7),
	.o_C_7(o_ct_7),
	.i_A_8(o_it_gt_mult_8),
	.i_B_8(o_ft_Ct_1_mult_8),
	.o_C_8(o_ct_8),
	.i_A_9(o_it_gt_mult_9),
	.i_B_9(o_ft_Ct_1_mult_9),
	.o_C_9(o_ct_9),
	.i_A_10(o_it_gt_mult_10),
	.i_B_10(o_ft_Ct_1_mult_10),
	.o_C_10(o_ct_10),
	.i_A_11(o_it_gt_mult_11),
	.i_B_11(o_ft_Ct_1_mult_11),
	.o_C_11(o_ct_11),
	.i_A_12(o_it_gt_mult_12),
	.i_B_12(o_ft_Ct_1_mult_12),
	.o_C_12(o_ct_12),
	.i_A_13(o_it_gt_mult_13),
	.i_B_13(o_ft_Ct_1_mult_13),
	.o_C_13(o_ct_13),
	.i_A_14(o_it_gt_mult_14),
	.i_B_14(o_ft_Ct_1_mult_14),
	.o_C_14(o_ct_14),
	.i_A_15(o_it_gt_mult_15),
	.i_B_15(o_ft_Ct_1_mult_15),
	.o_C_15(o_ct_15),
	.o_valid(ct_valid),
	.o_ready()
);

shift_register_group_18_16_14 shift_register_group_18_16_14_Ct (
	.clk(clk),
	.enable(enable),
	.in_0(o_ct_0),
	.out_0(ct_hold_0),
	.in_1(o_ct_1),
	.out_1(ct_hold_1),
	.in_2(o_ct_2),
	.out_2(ct_hold_2),
	.in_3(o_ct_3),
	.out_3(ct_hold_3),
	.in_4(o_ct_4),
	.out_4(ct_hold_4),
	.in_5(o_ct_5),
	.out_5(ct_hold_5),
	.in_6(o_ct_6),
	.out_6(ct_hold_6),
	.in_7(o_ct_7),
	.out_7(ct_hold_7),
	.in_8(o_ct_8),
	.out_8(ct_hold_8),
	.in_9(o_ct_9),
	.out_9(ct_hold_9),
	.in_10(o_ct_10),
	.out_10(ct_hold_10),
	.in_11(o_ct_11),
	.out_11(ct_hold_11),
	.in_12(o_ct_12),
	.out_12(ct_hold_12),
	.in_13(o_ct_13),
	.out_13(ct_hold_13),
	.in_14(o_ct_14),
	.out_14(ct_hold_14),
	.in_15(o_ct_15),
	.out_15(ct_hold_15),
	.reset(reset)
);

tanh_core_18_18_10_32_1 tanh_core_18_18_10_32_1_inst_0 (
	.clk(clk),
	.reset(reset),
	.i_valid(ct_valid),
	.i_ready(enable),
	.o_ready(),
	.o_valid(tanh_valid_0),
	.i_x(o_ct_0),
	.o_y(o_tanh_0)
);

tanh_core_18_18_10_32_1 tanh_core_18_18_10_32_1_inst_1 (
	.clk(clk),
	.reset(reset),
	.i_valid(ct_valid),
	.i_ready(enable),
	.o_ready(),
	.o_valid(tanh_valid_1),
	.i_x(o_ct_1),
	.o_y(o_tanh_1)
);

tanh_core_18_18_10_32_1 tanh_core_18_18_10_32_1_inst_2 (
	.clk(clk),
	.reset(reset),
	.i_valid(ct_valid),
	.i_ready(enable),
	.o_ready(),
	.o_valid(tanh_valid_2),
	.i_x(o_ct_2),
	.o_y(o_tanh_2)
);

tanh_core_18_18_10_32_1 tanh_core_18_18_10_32_1_inst_3 (
	.clk(clk),
	.reset(reset),
	.i_valid(ct_valid),
	.i_ready(enable),
	.o_ready(),
	.o_valid(tanh_valid_3),
	.i_x(o_ct_3),
	.o_y(o_tanh_3)
);

tanh_core_18_18_10_32_1 tanh_core_18_18_10_32_1_inst_4 (
	.clk(clk),
	.reset(reset),
	.i_valid(ct_valid),
	.i_ready(enable),
	.o_ready(),
	.o_valid(tanh_valid_4),
	.i_x(o_ct_4),
	.o_y(o_tanh_4)
);

tanh_core_18_18_10_32_1 tanh_core_18_18_10_32_1_inst_5 (
	.clk(clk),
	.reset(reset),
	.i_valid(ct_valid),
	.i_ready(enable),
	.o_ready(),
	.o_valid(tanh_valid_5),
	.i_x(o_ct_5),
	.o_y(o_tanh_5)
);

tanh_core_18_18_10_32_1 tanh_core_18_18_10_32_1_inst_6 (
	.clk(clk),
	.reset(reset),
	.i_valid(ct_valid),
	.i_ready(enable),
	.o_ready(),
	.o_valid(tanh_valid_6),
	.i_x(o_ct_6),
	.o_y(o_tanh_6)
);

tanh_core_18_18_10_32_1 tanh_core_18_18_10_32_1_inst_7 (
	.clk(clk),
	.reset(reset),
	.i_valid(ct_valid),
	.i_ready(enable),
	.o_ready(),
	.o_valid(tanh_valid_7),
	.i_x(o_ct_7),
	.o_y(o_tanh_7)
);

tanh_core_18_18_10_32_1 tanh_core_18_18_10_32_1_inst_8 (
	.clk(clk),
	.reset(reset),
	.i_valid(ct_valid),
	.i_ready(enable),
	.o_ready(),
	.o_valid(tanh_valid_8),
	.i_x(o_ct_8),
	.o_y(o_tanh_8)
);

tanh_core_18_18_10_32_1 tanh_core_18_18_10_32_1_inst_9 (
	.clk(clk),
	.reset(reset),
	.i_valid(ct_valid),
	.i_ready(enable),
	.o_ready(),
	.o_valid(tanh_valid_9),
	.i_x(o_ct_9),
	.o_y(o_tanh_9)
);

tanh_core_18_18_10_32_1 tanh_core_18_18_10_32_1_inst_10 (
	.clk(clk),
	.reset(reset),
	.i_valid(ct_valid),
	.i_ready(enable),
	.o_ready(),
	.o_valid(tanh_valid_10),
	.i_x(o_ct_10),
	.o_y(o_tanh_10)
);

tanh_core_18_18_10_32_1 tanh_core_18_18_10_32_1_inst_11 (
	.clk(clk),
	.reset(reset),
	.i_valid(ct_valid),
	.i_ready(enable),
	.o_ready(),
	.o_valid(tanh_valid_11),
	.i_x(o_ct_11),
	.o_y(o_tanh_11)
);

tanh_core_18_18_10_32_1 tanh_core_18_18_10_32_1_inst_12 (
	.clk(clk),
	.reset(reset),
	.i_valid(ct_valid),
	.i_ready(enable),
	.o_ready(),
	.o_valid(tanh_valid_12),
	.i_x(o_ct_12),
	.o_y(o_tanh_12)
);

tanh_core_18_18_10_32_1 tanh_core_18_18_10_32_1_inst_13 (
	.clk(clk),
	.reset(reset),
	.i_valid(ct_valid),
	.i_ready(enable),
	.o_ready(),
	.o_valid(tanh_valid_13),
	.i_x(o_ct_13),
	.o_y(o_tanh_13)
);

tanh_core_18_18_10_32_1 tanh_core_18_18_10_32_1_inst_14 (
	.clk(clk),
	.reset(reset),
	.i_valid(ct_valid),
	.i_ready(enable),
	.o_ready(),
	.o_valid(tanh_valid_14),
	.i_x(o_ct_14),
	.o_y(o_tanh_14)
);

tanh_core_18_18_10_32_1 tanh_core_18_18_10_32_1_inst_15 (
	.clk(clk),
	.reset(reset),
	.i_valid(ct_valid),
	.i_ready(enable),
	.o_ready(),
	.o_valid(tanh_valid_15),
	.i_x(o_ct_15),
	.o_y(o_tanh_15)
);

shift_register_group_18_16_18 shift_register_group_18_16_18_Ct (
	.clk(clk),
	.enable(enable),
	.in_0(o_output_gate_0),
	.out_0(ot_hold_0),
	.in_1(o_output_gate_1),
	.out_1(ot_hold_1),
	.in_2(o_output_gate_2),
	.out_2(ot_hold_2),
	.in_3(o_output_gate_3),
	.out_3(ot_hold_3),
	.in_4(o_output_gate_4),
	.out_4(ot_hold_4),
	.in_5(o_output_gate_5),
	.out_5(ot_hold_5),
	.in_6(o_output_gate_6),
	.out_6(ot_hold_6),
	.in_7(o_output_gate_7),
	.out_7(ot_hold_7),
	.in_8(o_output_gate_8),
	.out_8(ot_hold_8),
	.in_9(o_output_gate_9),
	.out_9(ot_hold_9),
	.in_10(o_output_gate_10),
	.out_10(ot_hold_10),
	.in_11(o_output_gate_11),
	.out_11(ot_hold_11),
	.in_12(o_output_gate_12),
	.out_12(ot_hold_12),
	.in_13(o_output_gate_13),
	.out_13(ot_hold_13),
	.in_14(o_output_gate_14),
	.out_14(ot_hold_14),
	.in_15(o_output_gate_15),
	.out_15(ot_hold_15),
	.reset(reset)
);

elementwise_mult_core_18_18_10_16_1 elementwise_mult_core_18_18_10_16_1_ot_tanh_mult (
	.clk(clk),
	.reset(reset),
	.i_valid(tanh_valid_0),
	.i_ready(enable),
	.i_A_0(o_tanh_0),
	.i_B_0(ot_hold_0),
	.o_C_0(o_mt_0),
	.i_A_1(o_tanh_1),
	.i_B_1(ot_hold_1),
	.o_C_1(o_mt_1),
	.i_A_2(o_tanh_2),
	.i_B_2(ot_hold_2),
	.o_C_2(o_mt_2),
	.i_A_3(o_tanh_3),
	.i_B_3(ot_hold_3),
	.o_C_3(o_mt_3),
	.i_A_4(o_tanh_4),
	.i_B_4(ot_hold_4),
	.o_C_4(o_mt_4),
	.i_A_5(o_tanh_5),
	.i_B_5(ot_hold_5),
	.o_C_5(o_mt_5),
	.i_A_6(o_tanh_6),
	.i_B_6(ot_hold_6),
	.o_C_6(o_mt_6),
	.i_A_7(o_tanh_7),
	.i_B_7(ot_hold_7),
	.o_C_7(o_mt_7),
	.i_A_8(o_tanh_8),
	.i_B_8(ot_hold_8),
	.o_C_8(o_mt_8),
	.i_A_9(o_tanh_9),
	.i_B_9(ot_hold_9),
	.o_C_9(o_mt_9),
	.i_A_10(o_tanh_10),
	.i_B_10(ot_hold_10),
	.o_C_10(o_mt_10),
	.i_A_11(o_tanh_11),
	.i_B_11(ot_hold_11),
	.o_C_11(o_mt_11),
	.i_A_12(o_tanh_12),
	.i_B_12(ot_hold_12),
	.o_C_12(o_mt_12),
	.i_A_13(o_tanh_13),
	.i_B_13(ot_hold_13),
	.o_C_13(o_mt_13),
	.i_A_14(o_tanh_14),
	.i_B_14(ot_hold_14),
	.o_C_14(o_mt_14),
	.i_A_15(o_tanh_15),
	.i_B_15(ot_hold_15),
	.o_C_15(o_mt_15),
	.o_valid(mt_valid),
	.o_ready()
);

always @ (posedge clk) begin
	if(reset) begin
		reg_i_valid <= 1'b0;
		reg_o_valid <= 1'b0;
		reg_Ct_1_0 <= 0;
		reg_WixrXtYt_1_0 <= 0;
		reg_Wic_0 <= 0;
		reg_bi_0 <= 0;
		reg_WfxrXtYt_1_0 <= 0;
		reg_Wfc_0 <= 0;
		reg_bf_0 <= 0;
		reg_WoxrXtYt_1_0 <= 0;
		reg_Woc_0 <= 0;
		reg_bo_0 <= 0;
		reg_WcxrXtYt_1_0 <= 0;
		reg_bc_0 <= 0;
		reg_out_mt_0 <= 0;
		reg_out_ct_0 <= 0;
		reg_Ct_1_1 <= 0;
		reg_WixrXtYt_1_1 <= 0;
		reg_Wic_1 <= 0;
		reg_bi_1 <= 0;
		reg_WfxrXtYt_1_1 <= 0;
		reg_Wfc_1 <= 0;
		reg_bf_1 <= 0;
		reg_WoxrXtYt_1_1 <= 0;
		reg_Woc_1 <= 0;
		reg_bo_1 <= 0;
		reg_WcxrXtYt_1_1 <= 0;
		reg_bc_1 <= 0;
		reg_out_mt_1 <= 0;
		reg_out_ct_1 <= 0;
		reg_Ct_1_2 <= 0;
		reg_WixrXtYt_1_2 <= 0;
		reg_Wic_2 <= 0;
		reg_bi_2 <= 0;
		reg_WfxrXtYt_1_2 <= 0;
		reg_Wfc_2 <= 0;
		reg_bf_2 <= 0;
		reg_WoxrXtYt_1_2 <= 0;
		reg_Woc_2 <= 0;
		reg_bo_2 <= 0;
		reg_WcxrXtYt_1_2 <= 0;
		reg_bc_2 <= 0;
		reg_out_mt_2 <= 0;
		reg_out_ct_2 <= 0;
		reg_Ct_1_3 <= 0;
		reg_WixrXtYt_1_3 <= 0;
		reg_Wic_3 <= 0;
		reg_bi_3 <= 0;
		reg_WfxrXtYt_1_3 <= 0;
		reg_Wfc_3 <= 0;
		reg_bf_3 <= 0;
		reg_WoxrXtYt_1_3 <= 0;
		reg_Woc_3 <= 0;
		reg_bo_3 <= 0;
		reg_WcxrXtYt_1_3 <= 0;
		reg_bc_3 <= 0;
		reg_out_mt_3 <= 0;
		reg_out_ct_3 <= 0;
		reg_Ct_1_4 <= 0;
		reg_WixrXtYt_1_4 <= 0;
		reg_Wic_4 <= 0;
		reg_bi_4 <= 0;
		reg_WfxrXtYt_1_4 <= 0;
		reg_Wfc_4 <= 0;
		reg_bf_4 <= 0;
		reg_WoxrXtYt_1_4 <= 0;
		reg_Woc_4 <= 0;
		reg_bo_4 <= 0;
		reg_WcxrXtYt_1_4 <= 0;
		reg_bc_4 <= 0;
		reg_out_mt_4 <= 0;
		reg_out_ct_4 <= 0;
		reg_Ct_1_5 <= 0;
		reg_WixrXtYt_1_5 <= 0;
		reg_Wic_5 <= 0;
		reg_bi_5 <= 0;
		reg_WfxrXtYt_1_5 <= 0;
		reg_Wfc_5 <= 0;
		reg_bf_5 <= 0;
		reg_WoxrXtYt_1_5 <= 0;
		reg_Woc_5 <= 0;
		reg_bo_5 <= 0;
		reg_WcxrXtYt_1_5 <= 0;
		reg_bc_5 <= 0;
		reg_out_mt_5 <= 0;
		reg_out_ct_5 <= 0;
		reg_Ct_1_6 <= 0;
		reg_WixrXtYt_1_6 <= 0;
		reg_Wic_6 <= 0;
		reg_bi_6 <= 0;
		reg_WfxrXtYt_1_6 <= 0;
		reg_Wfc_6 <= 0;
		reg_bf_6 <= 0;
		reg_WoxrXtYt_1_6 <= 0;
		reg_Woc_6 <= 0;
		reg_bo_6 <= 0;
		reg_WcxrXtYt_1_6 <= 0;
		reg_bc_6 <= 0;
		reg_out_mt_6 <= 0;
		reg_out_ct_6 <= 0;
		reg_Ct_1_7 <= 0;
		reg_WixrXtYt_1_7 <= 0;
		reg_Wic_7 <= 0;
		reg_bi_7 <= 0;
		reg_WfxrXtYt_1_7 <= 0;
		reg_Wfc_7 <= 0;
		reg_bf_7 <= 0;
		reg_WoxrXtYt_1_7 <= 0;
		reg_Woc_7 <= 0;
		reg_bo_7 <= 0;
		reg_WcxrXtYt_1_7 <= 0;
		reg_bc_7 <= 0;
		reg_out_mt_7 <= 0;
		reg_out_ct_7 <= 0;
		reg_Ct_1_8 <= 0;
		reg_WixrXtYt_1_8 <= 0;
		reg_Wic_8 <= 0;
		reg_bi_8 <= 0;
		reg_WfxrXtYt_1_8 <= 0;
		reg_Wfc_8 <= 0;
		reg_bf_8 <= 0;
		reg_WoxrXtYt_1_8 <= 0;
		reg_Woc_8 <= 0;
		reg_bo_8 <= 0;
		reg_WcxrXtYt_1_8 <= 0;
		reg_bc_8 <= 0;
		reg_out_mt_8 <= 0;
		reg_out_ct_8 <= 0;
		reg_Ct_1_9 <= 0;
		reg_WixrXtYt_1_9 <= 0;
		reg_Wic_9 <= 0;
		reg_bi_9 <= 0;
		reg_WfxrXtYt_1_9 <= 0;
		reg_Wfc_9 <= 0;
		reg_bf_9 <= 0;
		reg_WoxrXtYt_1_9 <= 0;
		reg_Woc_9 <= 0;
		reg_bo_9 <= 0;
		reg_WcxrXtYt_1_9 <= 0;
		reg_bc_9 <= 0;
		reg_out_mt_9 <= 0;
		reg_out_ct_9 <= 0;
		reg_Ct_1_10 <= 0;
		reg_WixrXtYt_1_10 <= 0;
		reg_Wic_10 <= 0;
		reg_bi_10 <= 0;
		reg_WfxrXtYt_1_10 <= 0;
		reg_Wfc_10 <= 0;
		reg_bf_10 <= 0;
		reg_WoxrXtYt_1_10 <= 0;
		reg_Woc_10 <= 0;
		reg_bo_10 <= 0;
		reg_WcxrXtYt_1_10 <= 0;
		reg_bc_10 <= 0;
		reg_out_mt_10 <= 0;
		reg_out_ct_10 <= 0;
		reg_Ct_1_11 <= 0;
		reg_WixrXtYt_1_11 <= 0;
		reg_Wic_11 <= 0;
		reg_bi_11 <= 0;
		reg_WfxrXtYt_1_11 <= 0;
		reg_Wfc_11 <= 0;
		reg_bf_11 <= 0;
		reg_WoxrXtYt_1_11 <= 0;
		reg_Woc_11 <= 0;
		reg_bo_11 <= 0;
		reg_WcxrXtYt_1_11 <= 0;
		reg_bc_11 <= 0;
		reg_out_mt_11 <= 0;
		reg_out_ct_11 <= 0;
		reg_Ct_1_12 <= 0;
		reg_WixrXtYt_1_12 <= 0;
		reg_Wic_12 <= 0;
		reg_bi_12 <= 0;
		reg_WfxrXtYt_1_12 <= 0;
		reg_Wfc_12 <= 0;
		reg_bf_12 <= 0;
		reg_WoxrXtYt_1_12 <= 0;
		reg_Woc_12 <= 0;
		reg_bo_12 <= 0;
		reg_WcxrXtYt_1_12 <= 0;
		reg_bc_12 <= 0;
		reg_out_mt_12 <= 0;
		reg_out_ct_12 <= 0;
		reg_Ct_1_13 <= 0;
		reg_WixrXtYt_1_13 <= 0;
		reg_Wic_13 <= 0;
		reg_bi_13 <= 0;
		reg_WfxrXtYt_1_13 <= 0;
		reg_Wfc_13 <= 0;
		reg_bf_13 <= 0;
		reg_WoxrXtYt_1_13 <= 0;
		reg_Woc_13 <= 0;
		reg_bo_13 <= 0;
		reg_WcxrXtYt_1_13 <= 0;
		reg_bc_13 <= 0;
		reg_out_mt_13 <= 0;
		reg_out_ct_13 <= 0;
		reg_Ct_1_14 <= 0;
		reg_WixrXtYt_1_14 <= 0;
		reg_Wic_14 <= 0;
		reg_bi_14 <= 0;
		reg_WfxrXtYt_1_14 <= 0;
		reg_Wfc_14 <= 0;
		reg_bf_14 <= 0;
		reg_WoxrXtYt_1_14 <= 0;
		reg_Woc_14 <= 0;
		reg_bo_14 <= 0;
		reg_WcxrXtYt_1_14 <= 0;
		reg_bc_14 <= 0;
		reg_out_mt_14 <= 0;
		reg_out_ct_14 <= 0;
		reg_Ct_1_15 <= 0;
		reg_WixrXtYt_1_15 <= 0;
		reg_Wic_15 <= 0;
		reg_bi_15 <= 0;
		reg_WfxrXtYt_1_15 <= 0;
		reg_Wfc_15 <= 0;
		reg_bf_15 <= 0;
		reg_WoxrXtYt_1_15 <= 0;
		reg_Woc_15 <= 0;
		reg_bo_15 <= 0;
		reg_WcxrXtYt_1_15 <= 0;
		reg_bc_15 <= 0;
		reg_out_mt_15 <= 0;
		reg_out_ct_15 <= 0;
	end else if (enable) begin
		reg_i_valid <= i_valid;
		reg_o_valid <= mt_valid;
		reg_Ct_1_0 <= Ct_1_0;
		reg_WixrXtYt_1_0 <= WixrXtYt_1_0;
		reg_Wic_0 <= Wic_0;
		reg_bi_0 <= bi_0;
		reg_WfxrXtYt_1_0 <= WfxrXtYt_1_0;
		reg_Wfc_0 <= Wfc_0;
		reg_bf_0 <= bf_0;
		reg_WoxrXtYt_1_0 <= WoxrXtYt_1_0;
		reg_Woc_0 <= Woc_0;
		reg_bo_0 <= bo_0;
		reg_WcxrXtYt_1_0 <= WcxrXtYt_1_0;
		reg_bc_0 <= bc_0;
		reg_out_mt_0 <= o_mt_0;
		reg_out_ct_0 <= ct_hold_0;
		reg_Ct_1_1 <= Ct_1_1;
		reg_WixrXtYt_1_1 <= WixrXtYt_1_1;
		reg_Wic_1 <= Wic_1;
		reg_bi_1 <= bi_1;
		reg_WfxrXtYt_1_1 <= WfxrXtYt_1_1;
		reg_Wfc_1 <= Wfc_1;
		reg_bf_1 <= bf_1;
		reg_WoxrXtYt_1_1 <= WoxrXtYt_1_1;
		reg_Woc_1 <= Woc_1;
		reg_bo_1 <= bo_1;
		reg_WcxrXtYt_1_1 <= WcxrXtYt_1_1;
		reg_bc_1 <= bc_1;
		reg_out_mt_1 <= o_mt_1;
		reg_out_ct_1 <= ct_hold_1;
		reg_Ct_1_2 <= Ct_1_2;
		reg_WixrXtYt_1_2 <= WixrXtYt_1_2;
		reg_Wic_2 <= Wic_2;
		reg_bi_2 <= bi_2;
		reg_WfxrXtYt_1_2 <= WfxrXtYt_1_2;
		reg_Wfc_2 <= Wfc_2;
		reg_bf_2 <= bf_2;
		reg_WoxrXtYt_1_2 <= WoxrXtYt_1_2;
		reg_Woc_2 <= Woc_2;
		reg_bo_2 <= bo_2;
		reg_WcxrXtYt_1_2 <= WcxrXtYt_1_2;
		reg_bc_2 <= bc_2;
		reg_out_mt_2 <= o_mt_2;
		reg_out_ct_2 <= ct_hold_2;
		reg_Ct_1_3 <= Ct_1_3;
		reg_WixrXtYt_1_3 <= WixrXtYt_1_3;
		reg_Wic_3 <= Wic_3;
		reg_bi_3 <= bi_3;
		reg_WfxrXtYt_1_3 <= WfxrXtYt_1_3;
		reg_Wfc_3 <= Wfc_3;
		reg_bf_3 <= bf_3;
		reg_WoxrXtYt_1_3 <= WoxrXtYt_1_3;
		reg_Woc_3 <= Woc_3;
		reg_bo_3 <= bo_3;
		reg_WcxrXtYt_1_3 <= WcxrXtYt_1_3;
		reg_bc_3 <= bc_3;
		reg_out_mt_3 <= o_mt_3;
		reg_out_ct_3 <= ct_hold_3;
		reg_Ct_1_4 <= Ct_1_4;
		reg_WixrXtYt_1_4 <= WixrXtYt_1_4;
		reg_Wic_4 <= Wic_4;
		reg_bi_4 <= bi_4;
		reg_WfxrXtYt_1_4 <= WfxrXtYt_1_4;
		reg_Wfc_4 <= Wfc_4;
		reg_bf_4 <= bf_4;
		reg_WoxrXtYt_1_4 <= WoxrXtYt_1_4;
		reg_Woc_4 <= Woc_4;
		reg_bo_4 <= bo_4;
		reg_WcxrXtYt_1_4 <= WcxrXtYt_1_4;
		reg_bc_4 <= bc_4;
		reg_out_mt_4 <= o_mt_4;
		reg_out_ct_4 <= ct_hold_4;
		reg_Ct_1_5 <= Ct_1_5;
		reg_WixrXtYt_1_5 <= WixrXtYt_1_5;
		reg_Wic_5 <= Wic_5;
		reg_bi_5 <= bi_5;
		reg_WfxrXtYt_1_5 <= WfxrXtYt_1_5;
		reg_Wfc_5 <= Wfc_5;
		reg_bf_5 <= bf_5;
		reg_WoxrXtYt_1_5 <= WoxrXtYt_1_5;
		reg_Woc_5 <= Woc_5;
		reg_bo_5 <= bo_5;
		reg_WcxrXtYt_1_5 <= WcxrXtYt_1_5;
		reg_bc_5 <= bc_5;
		reg_out_mt_5 <= o_mt_5;
		reg_out_ct_5 <= ct_hold_5;
		reg_Ct_1_6 <= Ct_1_6;
		reg_WixrXtYt_1_6 <= WixrXtYt_1_6;
		reg_Wic_6 <= Wic_6;
		reg_bi_6 <= bi_6;
		reg_WfxrXtYt_1_6 <= WfxrXtYt_1_6;
		reg_Wfc_6 <= Wfc_6;
		reg_bf_6 <= bf_6;
		reg_WoxrXtYt_1_6 <= WoxrXtYt_1_6;
		reg_Woc_6 <= Woc_6;
		reg_bo_6 <= bo_6;
		reg_WcxrXtYt_1_6 <= WcxrXtYt_1_6;
		reg_bc_6 <= bc_6;
		reg_out_mt_6 <= o_mt_6;
		reg_out_ct_6 <= ct_hold_6;
		reg_Ct_1_7 <= Ct_1_7;
		reg_WixrXtYt_1_7 <= WixrXtYt_1_7;
		reg_Wic_7 <= Wic_7;
		reg_bi_7 <= bi_7;
		reg_WfxrXtYt_1_7 <= WfxrXtYt_1_7;
		reg_Wfc_7 <= Wfc_7;
		reg_bf_7 <= bf_7;
		reg_WoxrXtYt_1_7 <= WoxrXtYt_1_7;
		reg_Woc_7 <= Woc_7;
		reg_bo_7 <= bo_7;
		reg_WcxrXtYt_1_7 <= WcxrXtYt_1_7;
		reg_bc_7 <= bc_7;
		reg_out_mt_7 <= o_mt_7;
		reg_out_ct_7 <= ct_hold_7;
		reg_Ct_1_8 <= Ct_1_8;
		reg_WixrXtYt_1_8 <= WixrXtYt_1_8;
		reg_Wic_8 <= Wic_8;
		reg_bi_8 <= bi_8;
		reg_WfxrXtYt_1_8 <= WfxrXtYt_1_8;
		reg_Wfc_8 <= Wfc_8;
		reg_bf_8 <= bf_8;
		reg_WoxrXtYt_1_8 <= WoxrXtYt_1_8;
		reg_Woc_8 <= Woc_8;
		reg_bo_8 <= bo_8;
		reg_WcxrXtYt_1_8 <= WcxrXtYt_1_8;
		reg_bc_8 <= bc_8;
		reg_out_mt_8 <= o_mt_8;
		reg_out_ct_8 <= ct_hold_8;
		reg_Ct_1_9 <= Ct_1_9;
		reg_WixrXtYt_1_9 <= WixrXtYt_1_9;
		reg_Wic_9 <= Wic_9;
		reg_bi_9 <= bi_9;
		reg_WfxrXtYt_1_9 <= WfxrXtYt_1_9;
		reg_Wfc_9 <= Wfc_9;
		reg_bf_9 <= bf_9;
		reg_WoxrXtYt_1_9 <= WoxrXtYt_1_9;
		reg_Woc_9 <= Woc_9;
		reg_bo_9 <= bo_9;
		reg_WcxrXtYt_1_9 <= WcxrXtYt_1_9;
		reg_bc_9 <= bc_9;
		reg_out_mt_9 <= o_mt_9;
		reg_out_ct_9 <= ct_hold_9;
		reg_Ct_1_10 <= Ct_1_10;
		reg_WixrXtYt_1_10 <= WixrXtYt_1_10;
		reg_Wic_10 <= Wic_10;
		reg_bi_10 <= bi_10;
		reg_WfxrXtYt_1_10 <= WfxrXtYt_1_10;
		reg_Wfc_10 <= Wfc_10;
		reg_bf_10 <= bf_10;
		reg_WoxrXtYt_1_10 <= WoxrXtYt_1_10;
		reg_Woc_10 <= Woc_10;
		reg_bo_10 <= bo_10;
		reg_WcxrXtYt_1_10 <= WcxrXtYt_1_10;
		reg_bc_10 <= bc_10;
		reg_out_mt_10 <= o_mt_10;
		reg_out_ct_10 <= ct_hold_10;
		reg_Ct_1_11 <= Ct_1_11;
		reg_WixrXtYt_1_11 <= WixrXtYt_1_11;
		reg_Wic_11 <= Wic_11;
		reg_bi_11 <= bi_11;
		reg_WfxrXtYt_1_11 <= WfxrXtYt_1_11;
		reg_Wfc_11 <= Wfc_11;
		reg_bf_11 <= bf_11;
		reg_WoxrXtYt_1_11 <= WoxrXtYt_1_11;
		reg_Woc_11 <= Woc_11;
		reg_bo_11 <= bo_11;
		reg_WcxrXtYt_1_11 <= WcxrXtYt_1_11;
		reg_bc_11 <= bc_11;
		reg_out_mt_11 <= o_mt_11;
		reg_out_ct_11 <= ct_hold_11;
		reg_Ct_1_12 <= Ct_1_12;
		reg_WixrXtYt_1_12 <= WixrXtYt_1_12;
		reg_Wic_12 <= Wic_12;
		reg_bi_12 <= bi_12;
		reg_WfxrXtYt_1_12 <= WfxrXtYt_1_12;
		reg_Wfc_12 <= Wfc_12;
		reg_bf_12 <= bf_12;
		reg_WoxrXtYt_1_12 <= WoxrXtYt_1_12;
		reg_Woc_12 <= Woc_12;
		reg_bo_12 <= bo_12;
		reg_WcxrXtYt_1_12 <= WcxrXtYt_1_12;
		reg_bc_12 <= bc_12;
		reg_out_mt_12 <= o_mt_12;
		reg_out_ct_12 <= ct_hold_12;
		reg_Ct_1_13 <= Ct_1_13;
		reg_WixrXtYt_1_13 <= WixrXtYt_1_13;
		reg_Wic_13 <= Wic_13;
		reg_bi_13 <= bi_13;
		reg_WfxrXtYt_1_13 <= WfxrXtYt_1_13;
		reg_Wfc_13 <= Wfc_13;
		reg_bf_13 <= bf_13;
		reg_WoxrXtYt_1_13 <= WoxrXtYt_1_13;
		reg_Woc_13 <= Woc_13;
		reg_bo_13 <= bo_13;
		reg_WcxrXtYt_1_13 <= WcxrXtYt_1_13;
		reg_bc_13 <= bc_13;
		reg_out_mt_13 <= o_mt_13;
		reg_out_ct_13 <= ct_hold_13;
		reg_Ct_1_14 <= Ct_1_14;
		reg_WixrXtYt_1_14 <= WixrXtYt_1_14;
		reg_Wic_14 <= Wic_14;
		reg_bi_14 <= bi_14;
		reg_WfxrXtYt_1_14 <= WfxrXtYt_1_14;
		reg_Wfc_14 <= Wfc_14;
		reg_bf_14 <= bf_14;
		reg_WoxrXtYt_1_14 <= WoxrXtYt_1_14;
		reg_Woc_14 <= Woc_14;
		reg_bo_14 <= bo_14;
		reg_WcxrXtYt_1_14 <= WcxrXtYt_1_14;
		reg_bc_14 <= bc_14;
		reg_out_mt_14 <= o_mt_14;
		reg_out_ct_14 <= ct_hold_14;
		reg_Ct_1_15 <= Ct_1_15;
		reg_WixrXtYt_1_15 <= WixrXtYt_1_15;
		reg_Wic_15 <= Wic_15;
		reg_bi_15 <= bi_15;
		reg_WfxrXtYt_1_15 <= WfxrXtYt_1_15;
		reg_Wfc_15 <= Wfc_15;
		reg_bf_15 <= bf_15;
		reg_WoxrXtYt_1_15 <= WoxrXtYt_1_15;
		reg_Woc_15 <= Woc_15;
		reg_bo_15 <= bo_15;
		reg_WcxrXtYt_1_15 <= WcxrXtYt_1_15;
		reg_bc_15 <= bc_15;
		reg_out_mt_15 <= o_mt_15;
		reg_out_ct_15 <= ct_hold_15;
	end
end
assign out_mt_0 = reg_out_mt_0;
assign out_ct_0 = reg_out_ct_0;
assign out_mt_1 = reg_out_mt_1;
assign out_ct_1 = reg_out_ct_1;
assign out_mt_2 = reg_out_mt_2;
assign out_ct_2 = reg_out_ct_2;
assign out_mt_3 = reg_out_mt_3;
assign out_ct_3 = reg_out_ct_3;
assign out_mt_4 = reg_out_mt_4;
assign out_ct_4 = reg_out_ct_4;
assign out_mt_5 = reg_out_mt_5;
assign out_ct_5 = reg_out_ct_5;
assign out_mt_6 = reg_out_mt_6;
assign out_ct_6 = reg_out_ct_6;
assign out_mt_7 = reg_out_mt_7;
assign out_ct_7 = reg_out_ct_7;
assign out_mt_8 = reg_out_mt_8;
assign out_ct_8 = reg_out_ct_8;
assign out_mt_9 = reg_out_mt_9;
assign out_ct_9 = reg_out_ct_9;
assign out_mt_10 = reg_out_mt_10;
assign out_ct_10 = reg_out_ct_10;
assign out_mt_11 = reg_out_mt_11;
assign out_ct_11 = reg_out_ct_11;
assign out_mt_12 = reg_out_mt_12;
assign out_ct_12 = reg_out_ct_12;
assign out_mt_13 = reg_out_mt_13;
assign out_ct_13 = reg_out_ct_13;
assign out_mt_14 = reg_out_mt_14;
assign out_ct_14 = reg_out_ct_14;
assign out_mt_15 = reg_out_mt_15;
assign out_ct_15 = reg_out_ct_15;
assign o_valid = reg_o_valid;
assign o_ready = enable;
endmodule

module output_activation_18_10_16_1 (
	input clk,
	input reset,
	input i_ready,
	input i_valid,
	input [17:0] stage1_result_0,
	input [17:0] bias_0,
	output [17:0] output_value_0,
	input [17:0] stage1_result_1,
	input [17:0] bias_1,
	output [17:0] output_value_1,
	input [17:0] stage1_result_2,
	input [17:0] bias_2,
	output [17:0] output_value_2,
	input [17:0] stage1_result_3,
	input [17:0] bias_3,
	output [17:0] output_value_3,
	input [17:0] stage1_result_4,
	input [17:0] bias_4,
	output [17:0] output_value_4,
	input [17:0] stage1_result_5,
	input [17:0] bias_5,
	output [17:0] output_value_5,
	input [17:0] stage1_result_6,
	input [17:0] bias_6,
	output [17:0] output_value_6,
	input [17:0] stage1_result_7,
	input [17:0] bias_7,
	output [17:0] output_value_7,
	input [17:0] stage1_result_8,
	input [17:0] bias_8,
	output [17:0] output_value_8,
	input [17:0] stage1_result_9,
	input [17:0] bias_9,
	output [17:0] output_value_9,
	input [17:0] stage1_result_10,
	input [17:0] bias_10,
	output [17:0] output_value_10,
	input [17:0] stage1_result_11,
	input [17:0] bias_11,
	output [17:0] output_value_11,
	input [17:0] stage1_result_12,
	input [17:0] bias_12,
	output [17:0] output_value_12,
	input [17:0] stage1_result_13,
	input [17:0] bias_13,
	output [17:0] output_value_13,
	input [17:0] stage1_result_14,
	input [17:0] bias_14,
	output [17:0] output_value_14,
	input [17:0] stage1_result_15,
	input [17:0] bias_15,
	output [17:0] output_value_15,
	output o_valid,
	output o_ready
);

wire adder2_valid, adder2_ready;
wire sigmoid_valid_0, sigmoid_ready_0;
wire [17:0] o_add2_0;
wire [17:0] o_sigmoid_0;
wire sigmoid_valid_1, sigmoid_ready_1;
wire [17:0] o_add2_1;
wire [17:0] o_sigmoid_1;
wire sigmoid_valid_2, sigmoid_ready_2;
wire [17:0] o_add2_2;
wire [17:0] o_sigmoid_2;
wire sigmoid_valid_3, sigmoid_ready_3;
wire [17:0] o_add2_3;
wire [17:0] o_sigmoid_3;
wire sigmoid_valid_4, sigmoid_ready_4;
wire [17:0] o_add2_4;
wire [17:0] o_sigmoid_4;
wire sigmoid_valid_5, sigmoid_ready_5;
wire [17:0] o_add2_5;
wire [17:0] o_sigmoid_5;
wire sigmoid_valid_6, sigmoid_ready_6;
wire [17:0] o_add2_6;
wire [17:0] o_sigmoid_6;
wire sigmoid_valid_7, sigmoid_ready_7;
wire [17:0] o_add2_7;
wire [17:0] o_sigmoid_7;
wire sigmoid_valid_8, sigmoid_ready_8;
wire [17:0] o_add2_8;
wire [17:0] o_sigmoid_8;
wire sigmoid_valid_9, sigmoid_ready_9;
wire [17:0] o_add2_9;
wire [17:0] o_sigmoid_9;
wire sigmoid_valid_10, sigmoid_ready_10;
wire [17:0] o_add2_10;
wire [17:0] o_sigmoid_10;
wire sigmoid_valid_11, sigmoid_ready_11;
wire [17:0] o_add2_11;
wire [17:0] o_sigmoid_11;
wire sigmoid_valid_12, sigmoid_ready_12;
wire [17:0] o_add2_12;
wire [17:0] o_sigmoid_12;
wire sigmoid_valid_13, sigmoid_ready_13;
wire [17:0] o_add2_13;
wire [17:0] o_sigmoid_13;
wire sigmoid_valid_14, sigmoid_ready_14;
wire [17:0] o_add2_14;
wire [17:0] o_sigmoid_14;
wire sigmoid_valid_15, sigmoid_ready_15;
wire [17:0] o_add2_15;
wire [17:0] o_sigmoid_15;
wire enable;
assign enable = i_ready;

elementwise_add_core_18_18_16 elementwise_add_core_18_18_16_inst (
	.clk(clk),
	.reset(reset),
	.i_valid(i_valid),
	.i_ready(sigmoid_ready_0),
	.i_A_0(stage1_result_0),
	.i_B_0(bias_0),
	.o_C_0(o_add2_0),
	.i_A_1(stage1_result_1),
	.i_B_1(bias_1),
	.o_C_1(o_add2_1),
	.i_A_2(stage1_result_2),
	.i_B_2(bias_2),
	.o_C_2(o_add2_2),
	.i_A_3(stage1_result_3),
	.i_B_3(bias_3),
	.o_C_3(o_add2_3),
	.i_A_4(stage1_result_4),
	.i_B_4(bias_4),
	.o_C_4(o_add2_4),
	.i_A_5(stage1_result_5),
	.i_B_5(bias_5),
	.o_C_5(o_add2_5),
	.i_A_6(stage1_result_6),
	.i_B_6(bias_6),
	.o_C_6(o_add2_6),
	.i_A_7(stage1_result_7),
	.i_B_7(bias_7),
	.o_C_7(o_add2_7),
	.i_A_8(stage1_result_8),
	.i_B_8(bias_8),
	.o_C_8(o_add2_8),
	.i_A_9(stage1_result_9),
	.i_B_9(bias_9),
	.o_C_9(o_add2_9),
	.i_A_10(stage1_result_10),
	.i_B_10(bias_10),
	.o_C_10(o_add2_10),
	.i_A_11(stage1_result_11),
	.i_B_11(bias_11),
	.o_C_11(o_add2_11),
	.i_A_12(stage1_result_12),
	.i_B_12(bias_12),
	.o_C_12(o_add2_12),
	.i_A_13(stage1_result_13),
	.i_B_13(bias_13),
	.o_C_13(o_add2_13),
	.i_A_14(stage1_result_14),
	.i_B_14(bias_14),
	.o_C_14(o_add2_14),
	.i_A_15(stage1_result_15),
	.i_B_15(bias_15),
	.o_C_15(o_add2_15),
	.o_valid(adder2_valid),
	.o_ready(adder2_ready)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_0 (
	.clk(clk),
	.reset(reset),
	.i_valid(adder2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_0),
	.o_valid(sigmoid_valid_0),
	.i_x(o_add2_0),
	.o_y(o_sigmoid_0)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_1 (
	.clk(clk),
	.reset(reset),
	.i_valid(adder2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_1),
	.o_valid(sigmoid_valid_1),
	.i_x(o_add2_1),
	.o_y(o_sigmoid_1)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_2 (
	.clk(clk),
	.reset(reset),
	.i_valid(adder2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_2),
	.o_valid(sigmoid_valid_2),
	.i_x(o_add2_2),
	.o_y(o_sigmoid_2)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_3 (
	.clk(clk),
	.reset(reset),
	.i_valid(adder2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_3),
	.o_valid(sigmoid_valid_3),
	.i_x(o_add2_3),
	.o_y(o_sigmoid_3)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_4 (
	.clk(clk),
	.reset(reset),
	.i_valid(adder2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_4),
	.o_valid(sigmoid_valid_4),
	.i_x(o_add2_4),
	.o_y(o_sigmoid_4)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_5 (
	.clk(clk),
	.reset(reset),
	.i_valid(adder2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_5),
	.o_valid(sigmoid_valid_5),
	.i_x(o_add2_5),
	.o_y(o_sigmoid_5)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_6 (
	.clk(clk),
	.reset(reset),
	.i_valid(adder2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_6),
	.o_valid(sigmoid_valid_6),
	.i_x(o_add2_6),
	.o_y(o_sigmoid_6)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_7 (
	.clk(clk),
	.reset(reset),
	.i_valid(adder2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_7),
	.o_valid(sigmoid_valid_7),
	.i_x(o_add2_7),
	.o_y(o_sigmoid_7)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_8 (
	.clk(clk),
	.reset(reset),
	.i_valid(adder2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_8),
	.o_valid(sigmoid_valid_8),
	.i_x(o_add2_8),
	.o_y(o_sigmoid_8)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_9 (
	.clk(clk),
	.reset(reset),
	.i_valid(adder2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_9),
	.o_valid(sigmoid_valid_9),
	.i_x(o_add2_9),
	.o_y(o_sigmoid_9)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_10 (
	.clk(clk),
	.reset(reset),
	.i_valid(adder2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_10),
	.o_valid(sigmoid_valid_10),
	.i_x(o_add2_10),
	.o_y(o_sigmoid_10)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_11 (
	.clk(clk),
	.reset(reset),
	.i_valid(adder2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_11),
	.o_valid(sigmoid_valid_11),
	.i_x(o_add2_11),
	.o_y(o_sigmoid_11)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_12 (
	.clk(clk),
	.reset(reset),
	.i_valid(adder2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_12),
	.o_valid(sigmoid_valid_12),
	.i_x(o_add2_12),
	.o_y(o_sigmoid_12)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_13 (
	.clk(clk),
	.reset(reset),
	.i_valid(adder2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_13),
	.o_valid(sigmoid_valid_13),
	.i_x(o_add2_13),
	.o_y(o_sigmoid_13)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_14 (
	.clk(clk),
	.reset(reset),
	.i_valid(adder2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_14),
	.o_valid(sigmoid_valid_14),
	.i_x(o_add2_14),
	.o_y(o_sigmoid_14)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_15 (
	.clk(clk),
	.reset(reset),
	.i_valid(adder2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_15),
	.o_valid(sigmoid_valid_15),
	.i_x(o_add2_15),
	.o_y(o_sigmoid_15)
);

assign o_ready = adder2_ready;
assign o_valid = sigmoid_valid_0 & i_ready;
assign output_value_0 = o_sigmoid_0;
assign output_value_1 = o_sigmoid_1;
assign output_value_2 = o_sigmoid_2;
assign output_value_3 = o_sigmoid_3;
assign output_value_4 = o_sigmoid_4;
assign output_value_5 = o_sigmoid_5;
assign output_value_6 = o_sigmoid_6;
assign output_value_7 = o_sigmoid_7;
assign output_value_8 = o_sigmoid_8;
assign output_value_9 = o_sigmoid_9;
assign output_value_10 = o_sigmoid_10;
assign output_value_11 = o_sigmoid_11;
assign output_value_12 = o_sigmoid_12;
assign output_value_13 = o_sigmoid_13;
assign output_value_14 = o_sigmoid_14;
assign output_value_15 = o_sigmoid_15;

endmodule

module sigmoid_core_18_18_10_32_1 ( 
	input clk,
	input reset,
	input i_valid,
	input i_ready,
	output o_ready,
	output o_valid,
	input [17:0] i_x,
	output [17:0] o_y
);

reg [12:0] k_list_0;
reg [12:0] b_list_0;
reg [12:0] k_list_1;
reg [12:0] b_list_1;
reg [12:0] k_list_2;
reg [12:0] b_list_2;
reg [12:0] k_list_3;
reg [12:0] b_list_3;
reg [12:0] k_list_4;
reg [12:0] b_list_4;
reg [12:0] k_list_5;
reg [12:0] b_list_5;
reg [12:0] k_list_6;
reg [12:0] b_list_6;
reg [12:0] k_list_7;
reg [12:0] b_list_7;
reg [12:0] k_list_8;
reg [12:0] b_list_8;
reg [12:0] k_list_9;
reg [12:0] b_list_9;
reg [12:0] k_list_10;
reg [12:0] b_list_10;
reg [12:0] k_list_11;
reg [12:0] b_list_11;
reg [12:0] k_list_12;
reg [12:0] b_list_12;
reg [12:0] k_list_13;
reg [12:0] b_list_13;
reg [12:0] k_list_14;
reg [12:0] b_list_14;
reg [12:0] k_list_15;
reg [12:0] b_list_15;
reg [12:0] k_list_16;
reg [12:0] b_list_16;
reg [12:0] k_list_17;
reg [12:0] b_list_17;
reg [12:0] k_list_18;
reg [12:0] b_list_18;
reg [12:0] k_list_19;
reg [12:0] b_list_19;
reg [12:0] k_list_20;
reg [12:0] b_list_20;
reg [12:0] k_list_21;
reg [12:0] b_list_21;
reg [12:0] k_list_22;
reg [12:0] b_list_22;
reg [12:0] k_list_23;
reg [12:0] b_list_23;
reg [12:0] k_list_24;
reg [12:0] b_list_24;
reg [12:0] k_list_25;
reg [12:0] b_list_25;
reg [12:0] k_list_26;
reg [12:0] b_list_26;
reg [12:0] k_list_27;
reg [12:0] b_list_27;
reg [12:0] k_list_28;
reg [12:0] b_list_28;
reg [12:0] k_list_29;
reg [12:0] b_list_29;
reg [12:0] k_list_30;
reg [12:0] b_list_30;
reg [12:0] k_list_31;
reg [12:0] b_list_31;

always @ (posedge clk) begin
	k_list_0  <= 13'b0000111111101;
	k_list_1  <= 13'b0000111101110;
	k_list_2  <= 13'b0000111010001;
	k_list_3  <= 13'b0000110101001;
	k_list_4  <= 13'b0000101111011;
	k_list_5  <= 13'b0000101001010;
	k_list_6  <= 13'b0000100011010;
	k_list_7  <= 13'b0000011101100;
	k_list_8  <= 13'b0000011000011;
	k_list_9  <= 13'b0000010100000;
	k_list_10 <= 13'b0000010000001;
	k_list_11 <= 13'b0000001101000;
	k_list_12 <= 13'b0000001010011;
	k_list_13 <= 13'b0000001000010;
	k_list_14 <= 13'b0000000110100;
	k_list_15 <= 13'b0000000101001;
	k_list_16 <= 13'b0000000100000;
	k_list_17 <= 13'b0000000011001;
	k_list_18 <= 13'b0000000010100;
	k_list_19 <= 13'b0000000001111;
	k_list_20 <= 13'b0000000001100;
	k_list_21 <= 13'b0000000001001;
	k_list_22 <= 13'b0000000000111;
	k_list_23 <= 13'b0000000000110;
	k_list_24 <= 13'b0000000000100;
	k_list_25 <= 13'b0000000000011;
	k_list_26 <= 13'b0000000000011;
	k_list_27 <= 13'b0000000000010;
	k_list_28 <= 13'b0000000000010;
	k_list_29 <= 13'b0000000000001;
	k_list_30 <= 13'b0000000000001;
	k_list_31 <= 13'b0000000000001;
	b_list_0  <= 13'b0010000000000;
	b_list_1  <= 13'b0010000000100;
	b_list_2  <= 13'b0010000010010;
	b_list_3  <= 13'b0010000110000;
	b_list_4  <= 13'b0010001011110;
	b_list_5  <= 13'b0010010011011;
	b_list_6  <= 13'b0010011100100;
	b_list_7  <= 13'b0010100110011;
	b_list_8  <= 13'b0010110000101;
	b_list_9  <= 13'b0010111010101;
	b_list_10 <= 13'b0011000100010;
	b_list_11 <= 13'b0011001101000;
	b_list_12 <= 13'b0011010100111;
	b_list_13 <= 13'b0011011011110;
	b_list_14 <= 13'b0011100001110;
	b_list_15 <= 13'b0011100111000;
	b_list_16 <= 13'b0011101011011;
	b_list_17 <= 13'b0011101111000;
	b_list_18 <= 13'b0011110010001;
	b_list_19 <= 13'b0011110100101;
	b_list_20 <= 13'b0011110110110;
	b_list_21 <= 13'b0011111000100;
	b_list_22 <= 13'b0011111001111;
	b_list_23 <= 13'b0011111011001;
	b_list_24 <= 13'b0011111100000;
	b_list_25 <= 13'b0011111100110;
	b_list_26 <= 13'b0011111101011;
	b_list_27 <= 13'b0011111101111;
	b_list_28 <= 13'b0011111110011;
	b_list_29 <= 13'b0011111110101;
	b_list_30 <= 13'b0011111110111;
	b_list_31 <= 13'b0011111111001;
end
reg [17:0] x;
reg [17:0] y;
reg valid_x, valid_y, enable;
wire [4:0] sel_k_b;

wire abs_valid, round_valid, mult_valid, compute_valid;
reg use_boundary_value;
reg [12:0] mac_ay;
reg [22:0] mac_az;
reg is_x_negative;
wire is_x_negative_hold;
wire [17:0] abs_x;
wire [17:0] x_partial;
reg [31:0] y_compute;
wire [31:0] x_k_plus_b;
wire [31:0] y_rounded;

assign x_partial = (abs_x >> 8);
assign sel_k_b = x_partial [4:0];

reg [12:0] selected_k, selected_b;
always @ (*) begin
	if (sel_k_b == 0) begin
		selected_k <= k_list_0;
		selected_b <= b_list_0;
	end else if (sel_k_b == 1) begin
		selected_k <= k_list_1;
		selected_b <= b_list_1;
	end else if (sel_k_b == 2) begin
		selected_k <= k_list_2;
		selected_b <= b_list_2;
	end else if (sel_k_b == 3) begin
		selected_k <= k_list_3;
		selected_b <= b_list_3;
	end else if (sel_k_b == 4) begin
		selected_k <= k_list_4;
		selected_b <= b_list_4;
	end else if (sel_k_b == 5) begin
		selected_k <= k_list_5;
		selected_b <= b_list_5;
	end else if (sel_k_b == 6) begin
		selected_k <= k_list_6;
		selected_b <= b_list_6;
	end else if (sel_k_b == 7) begin
		selected_k <= k_list_7;
		selected_b <= b_list_7;
	end else if (sel_k_b == 8) begin
		selected_k <= k_list_8;
		selected_b <= b_list_8;
	end else if (sel_k_b == 9) begin
		selected_k <= k_list_9;
		selected_b <= b_list_9;
	end else if (sel_k_b == 10) begin
		selected_k <= k_list_10;
		selected_b <= b_list_10;
	end else if (sel_k_b == 11) begin
		selected_k <= k_list_11;
		selected_b <= b_list_11;
	end else if (sel_k_b == 12) begin
		selected_k <= k_list_12;
		selected_b <= b_list_12;
	end else if (sel_k_b == 13) begin
		selected_k <= k_list_13;
		selected_b <= b_list_13;
	end else if (sel_k_b == 14) begin
		selected_k <= k_list_14;
		selected_b <= b_list_14;
	end else if (sel_k_b == 15) begin
		selected_k <= k_list_15;
		selected_b <= b_list_15;
	end else if (sel_k_b == 16) begin
		selected_k <= k_list_16;
		selected_b <= b_list_16;
	end else if (sel_k_b == 17) begin
		selected_k <= k_list_17;
		selected_b <= b_list_17;
	end else if (sel_k_b == 18) begin
		selected_k <= k_list_18;
		selected_b <= b_list_18;
	end else if (sel_k_b == 19) begin
		selected_k <= k_list_19;
		selected_b <= b_list_19;
	end else if (sel_k_b == 20) begin
		selected_k <= k_list_20;
		selected_b <= b_list_20;
	end else if (sel_k_b == 21) begin
		selected_k <= k_list_21;
		selected_b <= b_list_21;
	end else if (sel_k_b == 22) begin
		selected_k <= k_list_22;
		selected_b <= b_list_22;
	end else if (sel_k_b == 23) begin
		selected_k <= k_list_23;
		selected_b <= b_list_23;
	end else if (sel_k_b == 24) begin
		selected_k <= k_list_24;
		selected_b <= b_list_24;
	end else if (sel_k_b == 25) begin
		selected_k <= k_list_25;
		selected_b <= b_list_25;
	end else if (sel_k_b == 26) begin
		selected_k <= k_list_26;
		selected_b <= b_list_26;
	end else if (sel_k_b == 27) begin
		selected_k <= k_list_27;
		selected_b <= b_list_27;
	end else if (sel_k_b == 28) begin
		selected_k <= k_list_28;
		selected_b <= b_list_28;
	end else if (sel_k_b == 29) begin
		selected_k <= k_list_29;
		selected_b <= b_list_29;
	end else if (sel_k_b == 30) begin
		selected_k <= k_list_30;
		selected_b <= b_list_30;
	end else if (sel_k_b == 31) begin
		selected_k <= k_list_31;
		selected_b <= b_list_31;
	end else begin
		selected_k <= 0;
		selected_b <= 0;
	end
end
always @ (*) begin
	if (abs_x >= 8192) begin
		use_boundary_value <= 1'b1;
		mac_ay <= 0;
		mac_az <= 2097152;
	end else begin
		use_boundary_value <= 1'b0;
		mac_ay <= selected_k;
		mac_az <= (selected_b << 10);
	end
end
dsp_signed_mac_18_13_23_32 dsp_signed_mac_18_13_23_32_inst (
	.clk(clk),
	.reset(reset),
	.ena(enable),
	.ax(abs_x),
	.ay(mac_ay),
	.az(mac_az),
	.i_valid(abs_valid),
	.o_valid(compute_valid),
	.resulta(x_k_plus_b)
);

shift_register_unit_1_3 shift_register_unit_1_3_inst (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(is_x_negative),
	.out(is_x_negative_hold)
);

abs_unit_18 abs_unit_18_inst (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(valid_x),
	.in(x),
	.o_valid(abs_valid),
	.out(abs_x)
);

fp_rounding_unit_1_32_11 fp_rounding_unit_1_32_11_inst (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(compute_valid),
	.in(y_compute),
	.o_valid(round_valid),
	.out(y_rounded)
);

always @ (*) begin
	if (is_x_negative_hold)
		y_compute = 2097152 - x_k_plus_b;
	else 
		y_compute = x_k_plus_b;
	enable = i_ready;
end
always @ (posedge clk) begin
	if (reset) begin 
		valid_x <= 1'b0;
		valid_y <= 1'b0;
		x <= 0;
		y <= 0;
	end else if (enable) begin 
		valid_x <= i_valid;
		valid_y <= round_valid;
		x <= i_x;
		if (x[17] == 1'b1)
			is_x_negative <= 1'b1;
		else
			is_x_negative <= 1'b0;
		y <= y_rounded[17:0];
	end
end

assign o_y = y;
assign o_ready = i_ready;
assign o_valid = valid_y & i_ready;

endmodule

module abs_unit_18 (
	input clk,
	input reset,
	input enable,
	input i_valid,
	input [17:0] in,
	output o_valid,
	output [17:0] out
);

reg [17:0] abs_result;

always @ (*) begin
	if (in[17] == 1'b1)
		abs_result = -in;
	else 
		abs_result = in;
end

reg valid_reg;
reg [17:0] out_reg;
always @ (posedge clk) begin
	if (reset) begin
		valid_reg <= 1'b0;
		out_reg <= 0;
	end else if (enable) begin
		valid_reg <= i_valid;
		out_reg <= abs_result;
	end
end
assign out = out_reg;
assign o_valid = valid_reg;
endmodule

module dsp_signed_mac_18_13_23_32 (
	input clk,
	input reset,
	input ena,
	input i_valid,
	input [17:0] ax,
	input [12:0] ay,
	input [22:0] az,
	output o_valid,
	output [31:0] resulta
);

reg [17:0] reg_ax;
reg [12:0] reg_ay;
reg [22:0] reg_az;
reg [31:0] reg_res;
always @ (posedge clk) begin
	if (reset) begin
		reg_ax  <= 0;
		reg_ay  <= 0;
		reg_az  <= 0;
		reg_res <= 0;
	end else begin
		reg_ax  <= ax;
		reg_ay  <= ay;
		reg_az  <= az;
		reg_res <= (reg_ax * reg_ay) + reg_az;
	end
end

assign resulta = reg_res;
reg input_valid, result_valid, output_valid;
always @ (posedge clk) begin
	if (reset) begin
		output_valid <= 1'b0;
		input_valid <= 1'b0;
		result_valid <= 1'b0;
	end else if (ena) begin
		input_valid <= i_valid;
		result_valid <= input_valid;
		output_valid <= result_valid;
	end
end
assign o_valid = output_valid;
endmodule

module fp_rounding_unit_1_32_11 (
	input clk,
	input reset,
	input enable,
	input i_valid,
	input [31:0] in,
	output [31:0] out,
	output o_valid
);

reg [31:0] rounded_result;
reg [31:0] floor;
reg [31:0] ceil;
reg is_ceil;
reg floor_ceil_valid;

always @ (*) begin
	if (is_ceil) begin
		rounded_result = ceil;
	end else begin
		rounded_result = floor;
	end
end

reg valid_reg;
reg [31:0] out_reg;
always @ (posedge clk) begin
	if (reset) begin
		is_ceil <= 1'b0;
		floor_ceil_valid <= 1'b0;
		valid_reg <= 1'b0;
		floor <= 0;
		ceil <= 0;
		out_reg <= 0;
	end else if (enable) begin
		is_ceil <= in[10];
		floor <= in >>> 11;
		ceil <= (in >>> 11) + 1;
		floor_ceil_valid <= i_valid;
		out_reg <= rounded_result;
		valid_reg <= floor_ceil_valid;
	end
end

assign o_valid = valid_reg;

assign out = out_reg;

endmodule

module tanh_core_18_18_10_32_1 ( 
	input clk,
	input reset,
	input i_valid,
	input i_ready,
	output o_ready,
	output o_valid,
	input [17:0] i_x,
	output [17:0] o_y
);

reg [12:0] k_list_0;
reg [12:0] b_list_0;
reg [12:0] k_list_1;
reg [12:0] b_list_1;
reg [12:0] k_list_2;
reg [12:0] b_list_2;
reg [12:0] k_list_3;
reg [12:0] b_list_3;
reg [12:0] k_list_4;
reg [12:0] b_list_4;
reg [12:0] k_list_5;
reg [12:0] b_list_5;
reg [12:0] k_list_6;
reg [12:0] b_list_6;
reg [12:0] k_list_7;
reg [12:0] b_list_7;
reg [12:0] k_list_8;
reg [12:0] b_list_8;
reg [12:0] k_list_9;
reg [12:0] b_list_9;
reg [12:0] k_list_10;
reg [12:0] b_list_10;
reg [12:0] k_list_11;
reg [12:0] b_list_11;
reg [12:0] k_list_12;
reg [12:0] b_list_12;
reg [12:0] k_list_13;
reg [12:0] b_list_13;
reg [12:0] k_list_14;
reg [12:0] b_list_14;
reg [12:0] k_list_15;
reg [12:0] b_list_15;
reg [12:0] k_list_16;
reg [12:0] b_list_16;
reg [12:0] k_list_17;
reg [12:0] b_list_17;
reg [12:0] k_list_18;
reg [12:0] b_list_18;
reg [12:0] k_list_19;
reg [12:0] b_list_19;
reg [12:0] k_list_20;
reg [12:0] b_list_20;
reg [12:0] k_list_21;
reg [12:0] b_list_21;
reg [12:0] k_list_22;
reg [12:0] b_list_22;
reg [12:0] k_list_23;
reg [12:0] b_list_23;
reg [12:0] k_list_24;
reg [12:0] b_list_24;
reg [12:0] k_list_25;
reg [12:0] b_list_25;
reg [12:0] k_list_26;
reg [12:0] b_list_26;
reg [12:0] k_list_27;
reg [12:0] b_list_27;
reg [12:0] k_list_28;
reg [12:0] b_list_28;
reg [12:0] k_list_29;
reg [12:0] b_list_29;
reg [12:0] k_list_30;
reg [12:0] b_list_30;
reg [12:0] k_list_31;
reg [12:0] b_list_31;

always @ (posedge clk) begin
	k_list_0  <= 13'b0011111110101;
	k_list_1  <= 13'b0011110110111;
	k_list_2  <= 13'b0011101000011;
	k_list_3  <= 13'b0011010100100;
	k_list_4  <= 13'b0010111101011;
	k_list_5  <= 13'b0010100101000;
	k_list_6  <= 13'b0010001100111;
	k_list_7  <= 13'b0001110110001;
	k_list_8  <= 13'b0001100001110;
	k_list_9  <= 13'b0001001111111;
	k_list_10 <= 13'b0001000000101;
	k_list_11 <= 13'b0000110011111;
	k_list_12 <= 13'b0000101001011;
	k_list_13 <= 13'b0000100000111;
	k_list_14 <= 13'b0000011010000;
	k_list_15 <= 13'b0000010100100;
	k_list_16 <= 13'b0000010000001;
	k_list_17 <= 13'b0000001100101;
	k_list_18 <= 13'b0000001001111;
	k_list_19 <= 13'b0000000111110;
	k_list_20 <= 13'b0000000110000;
	k_list_21 <= 13'b0000000100110;
	k_list_22 <= 13'b0000000011101;
	k_list_23 <= 13'b0000000010111;
	k_list_24 <= 13'b0000000010010;
	k_list_25 <= 13'b0000000001110;
	k_list_26 <= 13'b0000000001011;
	k_list_27 <= 13'b0000000001000;
	k_list_28 <= 13'b0000000000111;
	k_list_29 <= 13'b0000000000101;
	k_list_30 <= 13'b0000000000100;
	k_list_31 <= 13'b0000000000011;
	b_list_0  <= 13'b0000000000000;
	b_list_1  <= 13'b0000000001000;
	b_list_2  <= 13'b0000000100101;
	b_list_3  <= 13'b0000001100000;
	b_list_4  <= 13'b0000010111101;
	b_list_5  <= 13'b0000100110111;
	b_list_6  <= 13'b0000111001000;
	b_list_7  <= 13'b0001001100111;
	b_list_8  <= 13'b0001100001010;
	b_list_9  <= 13'b0001110101011;
	b_list_10 <= 13'b0010001000011;
	b_list_11 <= 13'b0010011001111;
	b_list_12 <= 13'b0010101001101;
	b_list_13 <= 13'b0010110111100;
	b_list_14 <= 13'b0011000011101;
	b_list_15 <= 13'b0011001101111;
	b_list_16 <= 13'b0011010110101;
	b_list_17 <= 13'b0011011110000;
	b_list_18 <= 13'b0011100100001;
	b_list_19 <= 13'b0011101001010;
	b_list_20 <= 13'b0011101101100;
	b_list_21 <= 13'b0011110001000;
	b_list_22 <= 13'b0011110011110;
	b_list_23 <= 13'b0011110110001;
	b_list_24 <= 13'b0011111000000;
	b_list_25 <= 13'b0011111001101;
	b_list_26 <= 13'b0011111010111;
	b_list_27 <= 13'b0011111011111;
	b_list_28 <= 13'b0011111100101;
	b_list_29 <= 13'b0011111101010;
	b_list_30 <= 13'b0011111101111;
	b_list_31 <= 13'b0011111110010;
end
reg [17:0] x;
reg [17:0] y;
reg valid_x, valid_y, enable;
wire [4:0] sel_k_b;

wire abs_valid, round_valid, mult_valid, compute_valid;
reg use_boundary_value;
reg [12:0] mac_ay;
reg [22:0] mac_az;
reg is_x_negative;
wire is_x_negative_hold;
wire [17:0] abs_x;
wire [17:0] x_partial;
reg [31:0] y_compute;
wire [31:0] x_k_plus_b;
wire [31:0] y_rounded;

assign x_partial = (abs_x >> 7);
assign sel_k_b = x_partial [4:0];

reg [12:0] selected_k, selected_b;
always @ (*) begin
	if (sel_k_b == 0) begin
		selected_k <= k_list_0;
		selected_b <= b_list_0;
	end else if (sel_k_b == 1) begin
		selected_k <= k_list_1;
		selected_b <= b_list_1;
	end else if (sel_k_b == 2) begin
		selected_k <= k_list_2;
		selected_b <= b_list_2;
	end else if (sel_k_b == 3) begin
		selected_k <= k_list_3;
		selected_b <= b_list_3;
	end else if (sel_k_b == 4) begin
		selected_k <= k_list_4;
		selected_b <= b_list_4;
	end else if (sel_k_b == 5) begin
		selected_k <= k_list_5;
		selected_b <= b_list_5;
	end else if (sel_k_b == 6) begin
		selected_k <= k_list_6;
		selected_b <= b_list_6;
	end else if (sel_k_b == 7) begin
		selected_k <= k_list_7;
		selected_b <= b_list_7;
	end else if (sel_k_b == 8) begin
		selected_k <= k_list_8;
		selected_b <= b_list_8;
	end else if (sel_k_b == 9) begin
		selected_k <= k_list_9;
		selected_b <= b_list_9;
	end else if (sel_k_b == 10) begin
		selected_k <= k_list_10;
		selected_b <= b_list_10;
	end else if (sel_k_b == 11) begin
		selected_k <= k_list_11;
		selected_b <= b_list_11;
	end else if (sel_k_b == 12) begin
		selected_k <= k_list_12;
		selected_b <= b_list_12;
	end else if (sel_k_b == 13) begin
		selected_k <= k_list_13;
		selected_b <= b_list_13;
	end else if (sel_k_b == 14) begin
		selected_k <= k_list_14;
		selected_b <= b_list_14;
	end else if (sel_k_b == 15) begin
		selected_k <= k_list_15;
		selected_b <= b_list_15;
	end else if (sel_k_b == 16) begin
		selected_k <= k_list_16;
		selected_b <= b_list_16;
	end else if (sel_k_b == 17) begin
		selected_k <= k_list_17;
		selected_b <= b_list_17;
	end else if (sel_k_b == 18) begin
		selected_k <= k_list_18;
		selected_b <= b_list_18;
	end else if (sel_k_b == 19) begin
		selected_k <= k_list_19;
		selected_b <= b_list_19;
	end else if (sel_k_b == 20) begin
		selected_k <= k_list_20;
		selected_b <= b_list_20;
	end else if (sel_k_b == 21) begin
		selected_k <= k_list_21;
		selected_b <= b_list_21;
	end else if (sel_k_b == 22) begin
		selected_k <= k_list_22;
		selected_b <= b_list_22;
	end else if (sel_k_b == 23) begin
		selected_k <= k_list_23;
		selected_b <= b_list_23;
	end else if (sel_k_b == 24) begin
		selected_k <= k_list_24;
		selected_b <= b_list_24;
	end else if (sel_k_b == 25) begin
		selected_k <= k_list_25;
		selected_b <= b_list_25;
	end else if (sel_k_b == 26) begin
		selected_k <= k_list_26;
		selected_b <= b_list_26;
	end else if (sel_k_b == 27) begin
		selected_k <= k_list_27;
		selected_b <= b_list_27;
	end else if (sel_k_b == 28) begin
		selected_k <= k_list_28;
		selected_b <= b_list_28;
	end else if (sel_k_b == 29) begin
		selected_k <= k_list_29;
		selected_b <= b_list_29;
	end else if (sel_k_b == 30) begin
		selected_k <= k_list_30;
		selected_b <= b_list_30;
	end else if (sel_k_b == 31) begin
		selected_k <= k_list_31;
		selected_b <= b_list_31;
	end else begin
		selected_k <= 0;
		selected_b <= 0;
	end
end
always @ (*) begin
	if (abs_x >= 4096) begin
		use_boundary_value <= 1'b1;
		mac_ay <= 0;
		mac_az <= 2097152;
	end else begin
		use_boundary_value <= 1'b0;
		mac_ay <= selected_k;
		mac_az <= (selected_b << 10);
	end
end
dsp_signed_mac_18_13_23_32 dsp_signed_mac_18_13_23_32_inst (
	.clk(clk),
	.reset(reset),
	.ena(enable),
	.ax(abs_x),
	.ay(mac_ay),
	.az(mac_az),
	.i_valid(abs_valid),
	.o_valid(compute_valid),
	.resulta(x_k_plus_b)
);

shift_register_unit_1_3 shift_register_unit_1_3_inst (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(is_x_negative),
	.out(is_x_negative_hold)
);

abs_unit_18 abs_unit_18_inst (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(valid_x),
	.in(x),
	.o_valid(abs_valid),
	.out(abs_x)
);

fp_rounding_unit_1_32_11 fp_rounding_unit_1_32_11_inst (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(compute_valid),
	.in(y_compute),
	.o_valid(round_valid),
	.out(y_rounded)
);

always @ (*) begin
	if (is_x_negative_hold)
		y_compute = 2097152 - x_k_plus_b;
	else 
		y_compute = x_k_plus_b;
	enable = i_ready;
end
always @ (posedge clk) begin
	if (reset) begin 
		valid_x <= 1'b0;
		valid_y <= 1'b0;
		x <= 0;
		y <= 0;
	end else if (enable) begin 
		valid_x <= i_valid;
		valid_y <= round_valid;
		x <= i_x;
		if (x[17] == 1'b1)
			is_x_negative <= 1'b1;
		else
			is_x_negative <= 1'b0;
		y <= y_rounded[17:0];
	end
end

assign o_y = y;
assign o_ready = i_ready;
assign o_valid = valid_y & i_ready;

endmodule

module lstm_gate_18_10_16_1 (
	input clk,
	input reset,
	input i_ready,
	input i_valid,
	input [17:0] stage1_result_0,
	input [17:0] weight_0,
	input [17:0] Ct_1_0,
	input [17:0] bias_0,
	output [17:0] gate_output_0,
	input [17:0] stage1_result_1,
	input [17:0] weight_1,
	input [17:0] Ct_1_1,
	input [17:0] bias_1,
	output [17:0] gate_output_1,
	input [17:0] stage1_result_2,
	input [17:0] weight_2,
	input [17:0] Ct_1_2,
	input [17:0] bias_2,
	output [17:0] gate_output_2,
	input [17:0] stage1_result_3,
	input [17:0] weight_3,
	input [17:0] Ct_1_3,
	input [17:0] bias_3,
	output [17:0] gate_output_3,
	input [17:0] stage1_result_4,
	input [17:0] weight_4,
	input [17:0] Ct_1_4,
	input [17:0] bias_4,
	output [17:0] gate_output_4,
	input [17:0] stage1_result_5,
	input [17:0] weight_5,
	input [17:0] Ct_1_5,
	input [17:0] bias_5,
	output [17:0] gate_output_5,
	input [17:0] stage1_result_6,
	input [17:0] weight_6,
	input [17:0] Ct_1_6,
	input [17:0] bias_6,
	output [17:0] gate_output_6,
	input [17:0] stage1_result_7,
	input [17:0] weight_7,
	input [17:0] Ct_1_7,
	input [17:0] bias_7,
	output [17:0] gate_output_7,
	input [17:0] stage1_result_8,
	input [17:0] weight_8,
	input [17:0] Ct_1_8,
	input [17:0] bias_8,
	output [17:0] gate_output_8,
	input [17:0] stage1_result_9,
	input [17:0] weight_9,
	input [17:0] Ct_1_9,
	input [17:0] bias_9,
	output [17:0] gate_output_9,
	input [17:0] stage1_result_10,
	input [17:0] weight_10,
	input [17:0] Ct_1_10,
	input [17:0] bias_10,
	output [17:0] gate_output_10,
	input [17:0] stage1_result_11,
	input [17:0] weight_11,
	input [17:0] Ct_1_11,
	input [17:0] bias_11,
	output [17:0] gate_output_11,
	input [17:0] stage1_result_12,
	input [17:0] weight_12,
	input [17:0] Ct_1_12,
	input [17:0] bias_12,
	output [17:0] gate_output_12,
	input [17:0] stage1_result_13,
	input [17:0] weight_13,
	input [17:0] Ct_1_13,
	input [17:0] bias_13,
	output [17:0] gate_output_13,
	input [17:0] stage1_result_14,
	input [17:0] weight_14,
	input [17:0] Ct_1_14,
	input [17:0] bias_14,
	output [17:0] gate_output_14,
	input [17:0] stage1_result_15,
	input [17:0] weight_15,
	input [17:0] Ct_1_15,
	input [17:0] bias_15,
	output [17:0] gate_output_15,
	output o_valid,
	output o_ready
);

wire mult_valid, add0_valid, add1_valid, add2_valid;
wire mult_ready, add0_ready, add1_ready, add2_ready;
wire sigmoid_valid_0, sigmoid_ready_0;
wire [17:0] o_mult_0;
wire [17:0] o_add0_0;
wire [17:0] o_add1_0;
wire [17:0] add1_hold_0;
wire [17:0] o_add2_0;
wire [17:0] o_sigmoid_0;
wire sigmoid_valid_1, sigmoid_ready_1;
wire [17:0] o_mult_1;
wire [17:0] o_add0_1;
wire [17:0] o_add1_1;
wire [17:0] add1_hold_1;
wire [17:0] o_add2_1;
wire [17:0] o_sigmoid_1;
wire sigmoid_valid_2, sigmoid_ready_2;
wire [17:0] o_mult_2;
wire [17:0] o_add0_2;
wire [17:0] o_add1_2;
wire [17:0] add1_hold_2;
wire [17:0] o_add2_2;
wire [17:0] o_sigmoid_2;
wire sigmoid_valid_3, sigmoid_ready_3;
wire [17:0] o_mult_3;
wire [17:0] o_add0_3;
wire [17:0] o_add1_3;
wire [17:0] add1_hold_3;
wire [17:0] o_add2_3;
wire [17:0] o_sigmoid_3;
wire sigmoid_valid_4, sigmoid_ready_4;
wire [17:0] o_mult_4;
wire [17:0] o_add0_4;
wire [17:0] o_add1_4;
wire [17:0] add1_hold_4;
wire [17:0] o_add2_4;
wire [17:0] o_sigmoid_4;
wire sigmoid_valid_5, sigmoid_ready_5;
wire [17:0] o_mult_5;
wire [17:0] o_add0_5;
wire [17:0] o_add1_5;
wire [17:0] add1_hold_5;
wire [17:0] o_add2_5;
wire [17:0] o_sigmoid_5;
wire sigmoid_valid_6, sigmoid_ready_6;
wire [17:0] o_mult_6;
wire [17:0] o_add0_6;
wire [17:0] o_add1_6;
wire [17:0] add1_hold_6;
wire [17:0] o_add2_6;
wire [17:0] o_sigmoid_6;
wire sigmoid_valid_7, sigmoid_ready_7;
wire [17:0] o_mult_7;
wire [17:0] o_add0_7;
wire [17:0] o_add1_7;
wire [17:0] add1_hold_7;
wire [17:0] o_add2_7;
wire [17:0] o_sigmoid_7;
wire sigmoid_valid_8, sigmoid_ready_8;
wire [17:0] o_mult_8;
wire [17:0] o_add0_8;
wire [17:0] o_add1_8;
wire [17:0] add1_hold_8;
wire [17:0] o_add2_8;
wire [17:0] o_sigmoid_8;
wire sigmoid_valid_9, sigmoid_ready_9;
wire [17:0] o_mult_9;
wire [17:0] o_add0_9;
wire [17:0] o_add1_9;
wire [17:0] add1_hold_9;
wire [17:0] o_add2_9;
wire [17:0] o_sigmoid_9;
wire sigmoid_valid_10, sigmoid_ready_10;
wire [17:0] o_mult_10;
wire [17:0] o_add0_10;
wire [17:0] o_add1_10;
wire [17:0] add1_hold_10;
wire [17:0] o_add2_10;
wire [17:0] o_sigmoid_10;
wire sigmoid_valid_11, sigmoid_ready_11;
wire [17:0] o_mult_11;
wire [17:0] o_add0_11;
wire [17:0] o_add1_11;
wire [17:0] add1_hold_11;
wire [17:0] o_add2_11;
wire [17:0] o_sigmoid_11;
wire sigmoid_valid_12, sigmoid_ready_12;
wire [17:0] o_mult_12;
wire [17:0] o_add0_12;
wire [17:0] o_add1_12;
wire [17:0] add1_hold_12;
wire [17:0] o_add2_12;
wire [17:0] o_sigmoid_12;
wire sigmoid_valid_13, sigmoid_ready_13;
wire [17:0] o_mult_13;
wire [17:0] o_add0_13;
wire [17:0] o_add1_13;
wire [17:0] add1_hold_13;
wire [17:0] o_add2_13;
wire [17:0] o_sigmoid_13;
wire sigmoid_valid_14, sigmoid_ready_14;
wire [17:0] o_mult_14;
wire [17:0] o_add0_14;
wire [17:0] o_add1_14;
wire [17:0] add1_hold_14;
wire [17:0] o_add2_14;
wire [17:0] o_sigmoid_14;
wire sigmoid_valid_15, sigmoid_ready_15;
wire [17:0] o_mult_15;
wire [17:0] o_add0_15;
wire [17:0] o_add1_15;
wire [17:0] add1_hold_15;
wire [17:0] o_add2_15;
wire [17:0] o_sigmoid_15;
wire enable;
assign enable = i_ready;

elementwise_mult_core_18_18_10_16_1 elementwise_mult_core_18_18_10_16_1_mult (
	.clk(clk),
	.reset(reset),
	.i_valid(i_valid),
	.i_ready(add2_ready),
	.i_A_0(weight_0),
	.i_B_0(Ct_1_0),
	.o_C_0(o_mult_0),
	.i_A_1(weight_1),
	.i_B_1(Ct_1_1),
	.o_C_1(o_mult_1),
	.i_A_2(weight_2),
	.i_B_2(Ct_1_2),
	.o_C_2(o_mult_2),
	.i_A_3(weight_3),
	.i_B_3(Ct_1_3),
	.o_C_3(o_mult_3),
	.i_A_4(weight_4),
	.i_B_4(Ct_1_4),
	.o_C_4(o_mult_4),
	.i_A_5(weight_5),
	.i_B_5(Ct_1_5),
	.o_C_5(o_mult_5),
	.i_A_6(weight_6),
	.i_B_6(Ct_1_6),
	.o_C_6(o_mult_6),
	.i_A_7(weight_7),
	.i_B_7(Ct_1_7),
	.o_C_7(o_mult_7),
	.i_A_8(weight_8),
	.i_B_8(Ct_1_8),
	.o_C_8(o_mult_8),
	.i_A_9(weight_9),
	.i_B_9(Ct_1_9),
	.o_C_9(o_mult_9),
	.i_A_10(weight_10),
	.i_B_10(Ct_1_10),
	.o_C_10(o_mult_10),
	.i_A_11(weight_11),
	.i_B_11(Ct_1_11),
	.o_C_11(o_mult_11),
	.i_A_12(weight_12),
	.i_B_12(Ct_1_12),
	.o_C_12(o_mult_12),
	.i_A_13(weight_13),
	.i_B_13(Ct_1_13),
	.o_C_13(o_mult_13),
	.i_A_14(weight_14),
	.i_B_14(Ct_1_14),
	.o_C_14(o_mult_14),
	.i_A_15(weight_15),
	.i_B_15(Ct_1_15),
	.o_C_15(o_mult_15),
	.o_valid(mult_valid),
	.o_ready(mult_ready)
);

elementwise_add_core_18_18_16 elementwise_add_core_18_18_16_add_1 (
	.clk(clk),
	.reset(reset),
	.i_valid(i_valid),
	.i_ready(add2_ready),
	.i_A_0(stage1_result_0),
	.i_B_0(bias_0),
	.o_C_0(o_add1_0),
	.i_A_1(stage1_result_1),
	.i_B_1(bias_1),
	.o_C_1(o_add1_1),
	.i_A_2(stage1_result_2),
	.i_B_2(bias_2),
	.o_C_2(o_add1_2),
	.i_A_3(stage1_result_3),
	.i_B_3(bias_3),
	.o_C_3(o_add1_3),
	.i_A_4(stage1_result_4),
	.i_B_4(bias_4),
	.o_C_4(o_add1_4),
	.i_A_5(stage1_result_5),
	.i_B_5(bias_5),
	.o_C_5(o_add1_5),
	.i_A_6(stage1_result_6),
	.i_B_6(bias_6),
	.o_C_6(o_add1_6),
	.i_A_7(stage1_result_7),
	.i_B_7(bias_7),
	.o_C_7(o_add1_7),
	.i_A_8(stage1_result_8),
	.i_B_8(bias_8),
	.o_C_8(o_add1_8),
	.i_A_9(stage1_result_9),
	.i_B_9(bias_9),
	.o_C_9(o_add1_9),
	.i_A_10(stage1_result_10),
	.i_B_10(bias_10),
	.o_C_10(o_add1_10),
	.i_A_11(stage1_result_11),
	.i_B_11(bias_11),
	.o_C_11(o_add1_11),
	.i_A_12(stage1_result_12),
	.i_B_12(bias_12),
	.o_C_12(o_add1_12),
	.i_A_13(stage1_result_13),
	.i_B_13(bias_13),
	.o_C_13(o_add1_13),
	.i_A_14(stage1_result_14),
	.i_B_14(bias_14),
	.o_C_14(o_add1_14),
	.i_A_15(stage1_result_15),
	.i_B_15(bias_15),
	.o_C_15(o_add1_15),
	.o_valid(add1_valid),
	.o_ready(add1_ready)
);

shift_register_group_18_16_10 shift_register_group_18_16_10_Ct (
	.clk(clk),
	.enable(enable),
	.in_0(o_add1_0),
	.out_0(add1_hold_0),
	.in_1(o_add1_1),
	.out_1(add1_hold_1),
	.in_2(o_add1_2),
	.out_2(add1_hold_2),
	.in_3(o_add1_3),
	.out_3(add1_hold_3),
	.in_4(o_add1_4),
	.out_4(add1_hold_4),
	.in_5(o_add1_5),
	.out_5(add1_hold_5),
	.in_6(o_add1_6),
	.out_6(add1_hold_6),
	.in_7(o_add1_7),
	.out_7(add1_hold_7),
	.in_8(o_add1_8),
	.out_8(add1_hold_8),
	.in_9(o_add1_9),
	.out_9(add1_hold_9),
	.in_10(o_add1_10),
	.out_10(add1_hold_10),
	.in_11(o_add1_11),
	.out_11(add1_hold_11),
	.in_12(o_add1_12),
	.out_12(add1_hold_12),
	.in_13(o_add1_13),
	.out_13(add1_hold_13),
	.in_14(o_add1_14),
	.out_14(add1_hold_14),
	.in_15(o_add1_15),
	.out_15(add1_hold_15),
	.reset(reset)
);

elementwise_add_core_18_18_16 elementwise_add_core_18_18_16_add_2 (
	.clk(clk),
	.reset(reset),
	.i_valid(mult_valid),
	.i_ready(sigmoid_ready_0),
	.i_A_0(add1_hold_0),
	.i_B_0(o_mult_0),
	.o_C_0(o_add2_0),
	.i_A_1(add1_hold_1),
	.i_B_1(o_mult_1),
	.o_C_1(o_add2_1),
	.i_A_2(add1_hold_2),
	.i_B_2(o_mult_2),
	.o_C_2(o_add2_2),
	.i_A_3(add1_hold_3),
	.i_B_3(o_mult_3),
	.o_C_3(o_add2_3),
	.i_A_4(add1_hold_4),
	.i_B_4(o_mult_4),
	.o_C_4(o_add2_4),
	.i_A_5(add1_hold_5),
	.i_B_5(o_mult_5),
	.o_C_5(o_add2_5),
	.i_A_6(add1_hold_6),
	.i_B_6(o_mult_6),
	.o_C_6(o_add2_6),
	.i_A_7(add1_hold_7),
	.i_B_7(o_mult_7),
	.o_C_7(o_add2_7),
	.i_A_8(add1_hold_8),
	.i_B_8(o_mult_8),
	.o_C_8(o_add2_8),
	.i_A_9(add1_hold_9),
	.i_B_9(o_mult_9),
	.o_C_9(o_add2_9),
	.i_A_10(add1_hold_10),
	.i_B_10(o_mult_10),
	.o_C_10(o_add2_10),
	.i_A_11(add1_hold_11),
	.i_B_11(o_mult_11),
	.o_C_11(o_add2_11),
	.i_A_12(add1_hold_12),
	.i_B_12(o_mult_12),
	.o_C_12(o_add2_12),
	.i_A_13(add1_hold_13),
	.i_B_13(o_mult_13),
	.o_C_13(o_add2_13),
	.i_A_14(add1_hold_14),
	.i_B_14(o_mult_14),
	.o_C_14(o_add2_14),
	.i_A_15(add1_hold_15),
	.i_B_15(o_mult_15),
	.o_C_15(o_add2_15),
	.o_valid(add2_valid),
	.o_ready(add2_ready)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_0 (
	.clk(clk),
	.reset(reset),
	.i_valid(add2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_0),
	.o_valid(sigmoid_valid_0),
	.i_x(o_add2_0),
	.o_y(o_sigmoid_0)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_1 (
	.clk(clk),
	.reset(reset),
	.i_valid(add2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_1),
	.o_valid(sigmoid_valid_1),
	.i_x(o_add2_1),
	.o_y(o_sigmoid_1)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_2 (
	.clk(clk),
	.reset(reset),
	.i_valid(add2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_2),
	.o_valid(sigmoid_valid_2),
	.i_x(o_add2_2),
	.o_y(o_sigmoid_2)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_3 (
	.clk(clk),
	.reset(reset),
	.i_valid(add2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_3),
	.o_valid(sigmoid_valid_3),
	.i_x(o_add2_3),
	.o_y(o_sigmoid_3)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_4 (
	.clk(clk),
	.reset(reset),
	.i_valid(add2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_4),
	.o_valid(sigmoid_valid_4),
	.i_x(o_add2_4),
	.o_y(o_sigmoid_4)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_5 (
	.clk(clk),
	.reset(reset),
	.i_valid(add2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_5),
	.o_valid(sigmoid_valid_5),
	.i_x(o_add2_5),
	.o_y(o_sigmoid_5)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_6 (
	.clk(clk),
	.reset(reset),
	.i_valid(add2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_6),
	.o_valid(sigmoid_valid_6),
	.i_x(o_add2_6),
	.o_y(o_sigmoid_6)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_7 (
	.clk(clk),
	.reset(reset),
	.i_valid(add2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_7),
	.o_valid(sigmoid_valid_7),
	.i_x(o_add2_7),
	.o_y(o_sigmoid_7)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_8 (
	.clk(clk),
	.reset(reset),
	.i_valid(add2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_8),
	.o_valid(sigmoid_valid_8),
	.i_x(o_add2_8),
	.o_y(o_sigmoid_8)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_9 (
	.clk(clk),
	.reset(reset),
	.i_valid(add2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_9),
	.o_valid(sigmoid_valid_9),
	.i_x(o_add2_9),
	.o_y(o_sigmoid_9)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_10 (
	.clk(clk),
	.reset(reset),
	.i_valid(add2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_10),
	.o_valid(sigmoid_valid_10),
	.i_x(o_add2_10),
	.o_y(o_sigmoid_10)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_11 (
	.clk(clk),
	.reset(reset),
	.i_valid(add2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_11),
	.o_valid(sigmoid_valid_11),
	.i_x(o_add2_11),
	.o_y(o_sigmoid_11)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_12 (
	.clk(clk),
	.reset(reset),
	.i_valid(add2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_12),
	.o_valid(sigmoid_valid_12),
	.i_x(o_add2_12),
	.o_y(o_sigmoid_12)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_13 (
	.clk(clk),
	.reset(reset),
	.i_valid(add2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_13),
	.o_valid(sigmoid_valid_13),
	.i_x(o_add2_13),
	.o_y(o_sigmoid_13)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_14 (
	.clk(clk),
	.reset(reset),
	.i_valid(add2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_14),
	.o_valid(sigmoid_valid_14),
	.i_x(o_add2_14),
	.o_y(o_sigmoid_14)
);

sigmoid_core_18_18_10_32_1 sigmoid_core_18_18_10_32_1_inst_15 (
	.clk(clk),
	.reset(reset),
	.i_valid(add2_valid),
	.i_ready(i_ready),
	.o_ready(sigmoid_ready_15),
	.o_valid(sigmoid_valid_15),
	.i_x(o_add2_15),
	.o_y(o_sigmoid_15)
);

assign o_ready = mult_ready;
assign o_valid = sigmoid_valid_0 & i_ready;
assign gate_output_0 = o_sigmoid_0;
assign gate_output_1 = o_sigmoid_1;
assign gate_output_2 = o_sigmoid_2;
assign gate_output_3 = o_sigmoid_3;
assign gate_output_4 = o_sigmoid_4;
assign gate_output_5 = o_sigmoid_5;
assign gate_output_6 = o_sigmoid_6;
assign gate_output_7 = o_sigmoid_7;
assign gate_output_8 = o_sigmoid_8;
assign gate_output_9 = o_sigmoid_9;
assign gate_output_10 = o_sigmoid_10;
assign gate_output_11 = o_sigmoid_11;
assign gate_output_12 = o_sigmoid_12;
assign gate_output_13 = o_sigmoid_13;
assign gate_output_14 = o_sigmoid_14;
assign gate_output_15 = o_sigmoid_15;

endmodule

module shift_register_group_18_16_10 (
	input clk,
	input enable,
	input [17:0] in_0,
	output [17:0] out_0,
	input [17:0] in_1,
	output [17:0] out_1,
	input [17:0] in_2,
	output [17:0] out_2,
	input [17:0] in_3,
	output [17:0] out_3,
	input [17:0] in_4,
	output [17:0] out_4,
	input [17:0] in_5,
	output [17:0] out_5,
	input [17:0] in_6,
	output [17:0] out_6,
	input [17:0] in_7,
	output [17:0] out_7,
	input [17:0] in_8,
	output [17:0] out_8,
	input [17:0] in_9,
	output [17:0] out_9,
	input [17:0] in_10,
	output [17:0] out_10,
	input [17:0] in_11,
	output [17:0] out_11,
	input [17:0] in_12,
	output [17:0] out_12,
	input [17:0] in_13,
	output [17:0] out_13,
	input [17:0] in_14,
	output [17:0] out_14,
	input [17:0] in_15,
	output [17:0] out_15,
	input reset
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_0 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_0),
	.out(out_0)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_1 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_1),
	.out(out_1)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_2 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_2),
	.out(out_2)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_3 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_3),
	.out(out_3)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_4 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_4),
	.out(out_4)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_5 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_5),
	.out(out_5)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_6 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_6),
	.out(out_6)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_7 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_7),
	.out(out_7)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_8 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_8),
	.out(out_8)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_9 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_9),
	.out(out_9)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_10 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_10),
	.out(out_10)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_11 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_11),
	.out(out_11)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_12 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_12),
	.out(out_12)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_13 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_13),
	.out(out_13)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_14 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_14),
	.out(out_14)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_15 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_15),
	.out(out_15)
);

endmodule

module shift_register_unit_18_18 (
	input clk,
	input reset,
	input enable,
	input [17:0] in,
	output [17:0] out
);

reg [17:0] shift_registers_0;
reg [17:0] shift_registers_1;
reg [17:0] shift_registers_2;
reg [17:0] shift_registers_3;
reg [17:0] shift_registers_4;
reg [17:0] shift_registers_5;
reg [17:0] shift_registers_6;
reg [17:0] shift_registers_7;
reg [17:0] shift_registers_8;
reg [17:0] shift_registers_9;
reg [17:0] shift_registers_10;
reg [17:0] shift_registers_11;
reg [17:0] shift_registers_12;
reg [17:0] shift_registers_13;
reg [17:0] shift_registers_14;
reg [17:0] shift_registers_15;
reg [17:0] shift_registers_16;
reg [17:0] shift_registers_17;
always @ (posedge clk) begin
	if (reset) begin
		shift_registers_0 <= 18'd0;
		shift_registers_1 <= 18'd0;
		shift_registers_2 <= 18'd0;
		shift_registers_3 <= 18'd0;
		shift_registers_4 <= 18'd0;
		shift_registers_5 <= 18'd0;
		shift_registers_6 <= 18'd0;
		shift_registers_7 <= 18'd0;
		shift_registers_8 <= 18'd0;
		shift_registers_9 <= 18'd0;
		shift_registers_10 <= 18'd0;
		shift_registers_11 <= 18'd0;
		shift_registers_12 <= 18'd0;
		shift_registers_13 <= 18'd0;
		shift_registers_14 <= 18'd0;
		shift_registers_15 <= 18'd0;
		shift_registers_16 <= 18'd0;
		shift_registers_17 <= 18'd0;
	end else if (enable) begin
		shift_registers_0 <= in;
		shift_registers_1 <= shift_registers_0;
		shift_registers_2 <= shift_registers_1;
		shift_registers_3 <= shift_registers_2;
		shift_registers_4 <= shift_registers_3;
		shift_registers_5 <= shift_registers_4;
		shift_registers_6 <= shift_registers_5;
		shift_registers_7 <= shift_registers_6;
		shift_registers_8 <= shift_registers_7;
		shift_registers_9 <= shift_registers_8;
		shift_registers_10 <= shift_registers_9;
		shift_registers_11 <= shift_registers_10;
		shift_registers_12 <= shift_registers_11;
		shift_registers_13 <= shift_registers_12;
		shift_registers_14 <= shift_registers_13;
		shift_registers_15 <= shift_registers_14;
		shift_registers_16 <= shift_registers_15;
		shift_registers_17 <= shift_registers_16;
	end
end

assign out = shift_registers_17;

endmodule

module shift_register_group_18_16_18 (
	input clk,
	input enable,
	input [17:0] in_0,
	output [17:0] out_0,
	input [17:0] in_1,
	output [17:0] out_1,
	input [17:0] in_2,
	output [17:0] out_2,
	input [17:0] in_3,
	output [17:0] out_3,
	input [17:0] in_4,
	output [17:0] out_4,
	input [17:0] in_5,
	output [17:0] out_5,
	input [17:0] in_6,
	output [17:0] out_6,
	input [17:0] in_7,
	output [17:0] out_7,
	input [17:0] in_8,
	output [17:0] out_8,
	input [17:0] in_9,
	output [17:0] out_9,
	input [17:0] in_10,
	output [17:0] out_10,
	input [17:0] in_11,
	output [17:0] out_11,
	input [17:0] in_12,
	output [17:0] out_12,
	input [17:0] in_13,
	output [17:0] out_13,
	input [17:0] in_14,
	output [17:0] out_14,
	input [17:0] in_15,
	output [17:0] out_15,
	input reset
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_0 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_0),
	.out(out_0)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_1 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_1),
	.out(out_1)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_2 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_2),
	.out(out_2)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_3 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_3),
	.out(out_3)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_4 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_4),
	.out(out_4)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_5 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_5),
	.out(out_5)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_6 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_6),
	.out(out_6)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_7 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_7),
	.out(out_7)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_8 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_8),
	.out(out_8)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_9 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_9),
	.out(out_9)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_10 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_10),
	.out(out_10)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_11 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_11),
	.out(out_11)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_12 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_12),
	.out(out_12)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_13 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_13),
	.out(out_13)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_14 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_14),
	.out(out_14)
);

shift_register_unit_18_18 shift_register_unit_18_18_inst_15 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_15),
	.out(out_15)
);

endmodule

module elementwise_mult_core_18_18_10_16_1 (
	input clk,
	input reset,
	input i_valid,
	input i_ready,
	input [17:0] i_A_0,
	input [17:0] i_B_0,
	output [17:0] o_C_0,
	input [17:0] i_A_1,
	input [17:0] i_B_1,
	output [17:0] o_C_1,
	input [17:0] i_A_2,
	input [17:0] i_B_2,
	output [17:0] o_C_2,
	input [17:0] i_A_3,
	input [17:0] i_B_3,
	output [17:0] o_C_3,
	input [17:0] i_A_4,
	input [17:0] i_B_4,
	output [17:0] o_C_4,
	input [17:0] i_A_5,
	input [17:0] i_B_5,
	output [17:0] o_C_5,
	input [17:0] i_A_6,
	input [17:0] i_B_6,
	output [17:0] o_C_6,
	input [17:0] i_A_7,
	input [17:0] i_B_7,
	output [17:0] o_C_7,
	input [17:0] i_A_8,
	input [17:0] i_B_8,
	output [17:0] o_C_8,
	input [17:0] i_A_9,
	input [17:0] i_B_9,
	output [17:0] o_C_9,
	input [17:0] i_A_10,
	input [17:0] i_B_10,
	output [17:0] o_C_10,
	input [17:0] i_A_11,
	input [17:0] i_B_11,
	output [17:0] o_C_11,
	input [17:0] i_A_12,
	input [17:0] i_B_12,
	output [17:0] o_C_12,
	input [17:0] i_A_13,
	input [17:0] i_B_13,
	output [17:0] o_C_13,
	input [17:0] i_A_14,
	input [17:0] i_B_14,
	output [17:0] o_C_14,
	input [17:0] i_A_15,
	input [17:0] i_B_15,
	output [17:0] o_C_15,
	output o_valid,
	output o_ready
);

// Store inputs and outputs in registers
reg [17:0] reg_A_0;
reg [17:0] reg_B_0;
wire [17:0] reg_C_0;
reg [17:0] reg_A_1;
reg [17:0] reg_B_1;
wire [17:0] reg_C_1;
reg [17:0] reg_A_2;
reg [17:0] reg_B_2;
wire [17:0] reg_C_2;
reg [17:0] reg_A_3;
reg [17:0] reg_B_3;
wire [17:0] reg_C_3;
reg [17:0] reg_A_4;
reg [17:0] reg_B_4;
wire [17:0] reg_C_4;
reg [17:0] reg_A_5;
reg [17:0] reg_B_5;
wire [17:0] reg_C_5;
reg [17:0] reg_A_6;
reg [17:0] reg_B_6;
wire [17:0] reg_C_6;
reg [17:0] reg_A_7;
reg [17:0] reg_B_7;
wire [17:0] reg_C_7;
reg [17:0] reg_A_8;
reg [17:0] reg_B_8;
wire [17:0] reg_C_8;
reg [17:0] reg_A_9;
reg [17:0] reg_B_9;
wire [17:0] reg_C_9;
reg [17:0] reg_A_10;
reg [17:0] reg_B_10;
wire [17:0] reg_C_10;
reg [17:0] reg_A_11;
reg [17:0] reg_B_11;
wire [17:0] reg_C_11;
reg [17:0] reg_A_12;
reg [17:0] reg_B_12;
wire [17:0] reg_C_12;
reg [17:0] reg_A_13;
reg [17:0] reg_B_13;
wire [17:0] reg_C_13;
reg [17:0] reg_A_14;
reg [17:0] reg_B_14;
wire [17:0] reg_C_14;
reg [17:0] reg_A_15;
reg [17:0] reg_B_15;
wire [17:0] reg_C_15;

reg valid_A_B;
wire valid_C;
wire enable;
assign enable = i_ready;

wire mult_valid_0;
wire round_valid_0;
wire [36:0] mult_C_0;
wire [36:0] rounded_C_0;
wire mult_valid_1;
wire round_valid_1;
wire [36:0] mult_C_1;
wire [36:0] rounded_C_1;
wire mult_valid_2;
wire round_valid_2;
wire [36:0] mult_C_2;
wire [36:0] rounded_C_2;
wire mult_valid_3;
wire round_valid_3;
wire [36:0] mult_C_3;
wire [36:0] rounded_C_3;
wire mult_valid_4;
wire round_valid_4;
wire [36:0] mult_C_4;
wire [36:0] rounded_C_4;
wire mult_valid_5;
wire round_valid_5;
wire [36:0] mult_C_5;
wire [36:0] rounded_C_5;
wire mult_valid_6;
wire round_valid_6;
wire [36:0] mult_C_6;
wire [36:0] rounded_C_6;
wire mult_valid_7;
wire round_valid_7;
wire [36:0] mult_C_7;
wire [36:0] rounded_C_7;
wire mult_valid_8;
wire round_valid_8;
wire [36:0] mult_C_8;
wire [36:0] rounded_C_8;
wire mult_valid_9;
wire round_valid_9;
wire [36:0] mult_C_9;
wire [36:0] rounded_C_9;
wire mult_valid_10;
wire round_valid_10;
wire [36:0] mult_C_10;
wire [36:0] rounded_C_10;
wire mult_valid_11;
wire round_valid_11;
wire [36:0] mult_C_11;
wire [36:0] rounded_C_11;
wire mult_valid_12;
wire round_valid_12;
wire [36:0] mult_C_12;
wire [36:0] rounded_C_12;
wire mult_valid_13;
wire round_valid_13;
wire [36:0] mult_C_13;
wire [36:0] rounded_C_13;
wire mult_valid_14;
wire round_valid_14;
wire [36:0] mult_C_14;
wire [36:0] rounded_C_14;
wire mult_valid_15;
wire round_valid_15;
wire [36:0] mult_C_15;
wire [36:0] rounded_C_15;

dsp_signed_mult_18x18_unit_18_18_1 dsp_signed_mult_18x18_unit_18_18_1_inst0 (
	.clk(clk),
	.reset(reset),
	.ena(enable),
	.i_valid(valid_A_B),
	.ax(reg_A_0),
	.ay(reg_B_0),
	.bx(reg_A_1),
	.by(reg_B_1),
	.o_valid(mult_valid_0),
	.resulta(mult_C_0),
	.resultb(mult_C_1)
);
assign mult_valid_1 = mult_valid_0;
dsp_signed_mult_18x18_unit_18_18_1 dsp_signed_mult_18x18_unit_18_18_1_inst2 (
	.clk(clk),
	.reset(reset),
	.ena(enable),
	.i_valid(valid_A_B),
	.ax(reg_A_2),
	.ay(reg_B_2),
	.bx(reg_A_3),
	.by(reg_B_3),
	.o_valid(mult_valid_2),
	.resulta(mult_C_2),
	.resultb(mult_C_3)
);
assign mult_valid_3 = mult_valid_2;
dsp_signed_mult_18x18_unit_18_18_1 dsp_signed_mult_18x18_unit_18_18_1_inst4 (
	.clk(clk),
	.reset(reset),
	.ena(enable),
	.i_valid(valid_A_B),
	.ax(reg_A_4),
	.ay(reg_B_4),
	.bx(reg_A_5),
	.by(reg_B_5),
	.o_valid(mult_valid_4),
	.resulta(mult_C_4),
	.resultb(mult_C_5)
);
assign mult_valid_5 = mult_valid_4;
dsp_signed_mult_18x18_unit_18_18_1 dsp_signed_mult_18x18_unit_18_18_1_inst6 (
	.clk(clk),
	.reset(reset),
	.ena(enable),
	.i_valid(valid_A_B),
	.ax(reg_A_6),
	.ay(reg_B_6),
	.bx(reg_A_7),
	.by(reg_B_7),
	.o_valid(mult_valid_6),
	.resulta(mult_C_6),
	.resultb(mult_C_7)
);
assign mult_valid_7 = mult_valid_6;
dsp_signed_mult_18x18_unit_18_18_1 dsp_signed_mult_18x18_unit_18_18_1_inst8 (
	.clk(clk),
	.reset(reset),
	.ena(enable),
	.i_valid(valid_A_B),
	.ax(reg_A_8),
	.ay(reg_B_8),
	.bx(reg_A_9),
	.by(reg_B_9),
	.o_valid(mult_valid_8),
	.resulta(mult_C_8),
	.resultb(mult_C_9)
);
assign mult_valid_9 = mult_valid_8;
dsp_signed_mult_18x18_unit_18_18_1 dsp_signed_mult_18x18_unit_18_18_1_inst10 (
	.clk(clk),
	.reset(reset),
	.ena(enable),
	.i_valid(valid_A_B),
	.ax(reg_A_10),
	.ay(reg_B_10),
	.bx(reg_A_11),
	.by(reg_B_11),
	.o_valid(mult_valid_10),
	.resulta(mult_C_10),
	.resultb(mult_C_11)
);
assign mult_valid_11 = mult_valid_10;
dsp_signed_mult_18x18_unit_18_18_1 dsp_signed_mult_18x18_unit_18_18_1_inst12 (
	.clk(clk),
	.reset(reset),
	.ena(enable),
	.i_valid(valid_A_B),
	.ax(reg_A_12),
	.ay(reg_B_12),
	.bx(reg_A_13),
	.by(reg_B_13),
	.o_valid(mult_valid_12),
	.resulta(mult_C_12),
	.resultb(mult_C_13)
);
assign mult_valid_13 = mult_valid_12;
dsp_signed_mult_18x18_unit_18_18_1 dsp_signed_mult_18x18_unit_18_18_1_inst14 (
	.clk(clk),
	.reset(reset),
	.ena(enable),
	.i_valid(valid_A_B),
	.ax(reg_A_14),
	.ay(reg_B_14),
	.bx(reg_A_15),
	.by(reg_B_15),
	.o_valid(mult_valid_14),
	.resulta(mult_C_14),
	.resultb(mult_C_15)
);
assign mult_valid_15 = mult_valid_14;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst0 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_0),
	.in(mult_C_0),
	.o_valid(round_valid_0),
	.out(rounded_C_0)
);
assign reg_C_0 = rounded_C_0;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst1 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_1),
	.in(mult_C_1),
	.o_valid(round_valid_1),
	.out(rounded_C_1)
);
assign reg_C_1 = rounded_C_1;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst2 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_2),
	.in(mult_C_2),
	.o_valid(round_valid_2),
	.out(rounded_C_2)
);
assign reg_C_2 = rounded_C_2;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst3 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_3),
	.in(mult_C_3),
	.o_valid(round_valid_3),
	.out(rounded_C_3)
);
assign reg_C_3 = rounded_C_3;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst4 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_4),
	.in(mult_C_4),
	.o_valid(round_valid_4),
	.out(rounded_C_4)
);
assign reg_C_4 = rounded_C_4;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst5 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_5),
	.in(mult_C_5),
	.o_valid(round_valid_5),
	.out(rounded_C_5)
);
assign reg_C_5 = rounded_C_5;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst6 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_6),
	.in(mult_C_6),
	.o_valid(round_valid_6),
	.out(rounded_C_6)
);
assign reg_C_6 = rounded_C_6;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst7 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_7),
	.in(mult_C_7),
	.o_valid(round_valid_7),
	.out(rounded_C_7)
);
assign reg_C_7 = rounded_C_7;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst8 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_8),
	.in(mult_C_8),
	.o_valid(round_valid_8),
	.out(rounded_C_8)
);
assign reg_C_8 = rounded_C_8;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst9 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_9),
	.in(mult_C_9),
	.o_valid(round_valid_9),
	.out(rounded_C_9)
);
assign reg_C_9 = rounded_C_9;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst10 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_10),
	.in(mult_C_10),
	.o_valid(round_valid_10),
	.out(rounded_C_10)
);
assign reg_C_10 = rounded_C_10;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst11 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_11),
	.in(mult_C_11),
	.o_valid(round_valid_11),
	.out(rounded_C_11)
);
assign reg_C_11 = rounded_C_11;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst12 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_12),
	.in(mult_C_12),
	.o_valid(round_valid_12),
	.out(rounded_C_12)
);
assign reg_C_12 = rounded_C_12;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst13 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_13),
	.in(mult_C_13),
	.o_valid(round_valid_13),
	.out(rounded_C_13)
);
assign reg_C_13 = rounded_C_13;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst14 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_14),
	.in(mult_C_14),
	.o_valid(round_valid_14),
	.out(rounded_C_14)
);
assign reg_C_14 = rounded_C_14;
fp_rounding_unit_1_37_10 fp_rounding_unit_1_37_10_inst15 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_valid(mult_valid_15),
	.in(mult_C_15),
	.o_valid(round_valid_15),
	.out(rounded_C_15)
);
assign reg_C_15 = rounded_C_15;
always @ (posedge clk) begin
	if (reset) begin
		valid_A_B <= 1'b0;
		reg_A_0 <= 0;
		reg_B_0 <= 0;
		reg_A_1 <= 0;
		reg_B_1 <= 0;
		reg_A_2 <= 0;
		reg_B_2 <= 0;
		reg_A_3 <= 0;
		reg_B_3 <= 0;
		reg_A_4 <= 0;
		reg_B_4 <= 0;
		reg_A_5 <= 0;
		reg_B_5 <= 0;
		reg_A_6 <= 0;
		reg_B_6 <= 0;
		reg_A_7 <= 0;
		reg_B_7 <= 0;
		reg_A_8 <= 0;
		reg_B_8 <= 0;
		reg_A_9 <= 0;
		reg_B_9 <= 0;
		reg_A_10 <= 0;
		reg_B_10 <= 0;
		reg_A_11 <= 0;
		reg_B_11 <= 0;
		reg_A_12 <= 0;
		reg_B_12 <= 0;
		reg_A_13 <= 0;
		reg_B_13 <= 0;
		reg_A_14 <= 0;
		reg_B_14 <= 0;
		reg_A_15 <= 0;
		reg_B_15 <= 0;
	end else if (enable) begin
		reg_A_0 <= i_A_0;
		reg_B_0 <= i_B_0;
		reg_A_1 <= i_A_1;
		reg_B_1 <= i_B_1;
		reg_A_2 <= i_A_2;
		reg_B_2 <= i_B_2;
		reg_A_3 <= i_A_3;
		reg_B_3 <= i_B_3;
		reg_A_4 <= i_A_4;
		reg_B_4 <= i_B_4;
		reg_A_5 <= i_A_5;
		reg_B_5 <= i_B_5;
		reg_A_6 <= i_A_6;
		reg_B_6 <= i_B_6;
		reg_A_7 <= i_A_7;
		reg_B_7 <= i_B_7;
		reg_A_8 <= i_A_8;
		reg_B_8 <= i_B_8;
		reg_A_9 <= i_A_9;
		reg_B_9 <= i_B_9;
		reg_A_10 <= i_A_10;
		reg_B_10 <= i_B_10;
		reg_A_11 <= i_A_11;
		reg_B_11 <= i_B_11;
		reg_A_12 <= i_A_12;
		reg_B_12 <= i_B_12;
		reg_A_13 <= i_A_13;
		reg_B_13 <= i_B_13;
		reg_A_14 <= i_A_14;
		reg_B_14 <= i_B_14;
		reg_A_15 <= i_A_15;
		reg_B_15 <= i_B_15;
		valid_A_B <= i_valid;
	end
end

assign o_C_0 = reg_C_0;
assign o_C_1 = reg_C_1;
assign o_C_2 = reg_C_2;
assign o_C_3 = reg_C_3;
assign o_C_4 = reg_C_4;
assign o_C_5 = reg_C_5;
assign o_C_6 = reg_C_6;
assign o_C_7 = reg_C_7;
assign o_C_8 = reg_C_8;
assign o_C_9 = reg_C_9;
assign o_C_10 = reg_C_10;
assign o_C_11 = reg_C_11;
assign o_C_12 = reg_C_12;
assign o_C_13 = reg_C_13;
assign o_C_14 = reg_C_14;
assign o_C_15 = reg_C_15;
assign valid_C = round_valid_0;
assign o_ready = i_ready;
assign o_valid = valid_C & i_ready;

endmodule

module elementwise_add_core_18_18_16 (
	input clk,
	input reset,
	input i_valid,
	input i_ready,
	input [17:0] i_A_0,
	input [17:0] i_B_0,
	output [17:0] o_C_0,
	input [17:0] i_A_1,
	input [17:0] i_B_1,
	output [17:0] o_C_1,
	input [17:0] i_A_2,
	input [17:0] i_B_2,
	output [17:0] o_C_2,
	input [17:0] i_A_3,
	input [17:0] i_B_3,
	output [17:0] o_C_3,
	input [17:0] i_A_4,
	input [17:0] i_B_4,
	output [17:0] o_C_4,
	input [17:0] i_A_5,
	input [17:0] i_B_5,
	output [17:0] o_C_5,
	input [17:0] i_A_6,
	input [17:0] i_B_6,
	output [17:0] o_C_6,
	input [17:0] i_A_7,
	input [17:0] i_B_7,
	output [17:0] o_C_7,
	input [17:0] i_A_8,
	input [17:0] i_B_8,
	output [17:0] o_C_8,
	input [17:0] i_A_9,
	input [17:0] i_B_9,
	output [17:0] o_C_9,
	input [17:0] i_A_10,
	input [17:0] i_B_10,
	output [17:0] o_C_10,
	input [17:0] i_A_11,
	input [17:0] i_B_11,
	output [17:0] o_C_11,
	input [17:0] i_A_12,
	input [17:0] i_B_12,
	output [17:0] o_C_12,
	input [17:0] i_A_13,
	input [17:0] i_B_13,
	output [17:0] o_C_13,
	input [17:0] i_A_14,
	input [17:0] i_B_14,
	output [17:0] o_C_14,
	input [17:0] i_A_15,
	input [17:0] i_B_15,
	output [17:0] o_C_15,
	output o_valid,
	output o_ready
);

reg [17:0] reg_A_0;
reg [17:0] reg_B_0;
reg [17:0] reg_C_0;
reg [17:0] reg_A_1;
reg [17:0] reg_B_1;
reg [17:0] reg_C_1;
reg [17:0] reg_A_2;
reg [17:0] reg_B_2;
reg [17:0] reg_C_2;
reg [17:0] reg_A_3;
reg [17:0] reg_B_3;
reg [17:0] reg_C_3;
reg [17:0] reg_A_4;
reg [17:0] reg_B_4;
reg [17:0] reg_C_4;
reg [17:0] reg_A_5;
reg [17:0] reg_B_5;
reg [17:0] reg_C_5;
reg [17:0] reg_A_6;
reg [17:0] reg_B_6;
reg [17:0] reg_C_6;
reg [17:0] reg_A_7;
reg [17:0] reg_B_7;
reg [17:0] reg_C_7;
reg [17:0] reg_A_8;
reg [17:0] reg_B_8;
reg [17:0] reg_C_8;
reg [17:0] reg_A_9;
reg [17:0] reg_B_9;
reg [17:0] reg_C_9;
reg [17:0] reg_A_10;
reg [17:0] reg_B_10;
reg [17:0] reg_C_10;
reg [17:0] reg_A_11;
reg [17:0] reg_B_11;
reg [17:0] reg_C_11;
reg [17:0] reg_A_12;
reg [17:0] reg_B_12;
reg [17:0] reg_C_12;
reg [17:0] reg_A_13;
reg [17:0] reg_B_13;
reg [17:0] reg_C_13;
reg [17:0] reg_A_14;
reg [17:0] reg_B_14;
reg [17:0] reg_C_14;
reg [17:0] reg_A_15;
reg [17:0] reg_B_15;
reg [17:0] reg_C_15;

reg valid_A_B, valid_C;
wire enable;
assign enable = i_ready;

always @ (posedge clk) begin
	if (reset) begin
		valid_A_B <= 1'b0;
		valid_C <= 1'b0;
		reg_A_0 <= 0;
		reg_B_0 <= 0;
		reg_C_0 <= 0;
		reg_A_1 <= 0;
		reg_B_1 <= 0;
		reg_C_1 <= 0;
		reg_A_2 <= 0;
		reg_B_2 <= 0;
		reg_C_2 <= 0;
		reg_A_3 <= 0;
		reg_B_3 <= 0;
		reg_C_3 <= 0;
		reg_A_4 <= 0;
		reg_B_4 <= 0;
		reg_C_4 <= 0;
		reg_A_5 <= 0;
		reg_B_5 <= 0;
		reg_C_5 <= 0;
		reg_A_6 <= 0;
		reg_B_6 <= 0;
		reg_C_6 <= 0;
		reg_A_7 <= 0;
		reg_B_7 <= 0;
		reg_C_7 <= 0;
		reg_A_8 <= 0;
		reg_B_8 <= 0;
		reg_C_8 <= 0;
		reg_A_9 <= 0;
		reg_B_9 <= 0;
		reg_C_9 <= 0;
		reg_A_10 <= 0;
		reg_B_10 <= 0;
		reg_C_10 <= 0;
		reg_A_11 <= 0;
		reg_B_11 <= 0;
		reg_C_11 <= 0;
		reg_A_12 <= 0;
		reg_B_12 <= 0;
		reg_C_12 <= 0;
		reg_A_13 <= 0;
		reg_B_13 <= 0;
		reg_C_13 <= 0;
		reg_A_14 <= 0;
		reg_B_14 <= 0;
		reg_C_14 <= 0;
		reg_A_15 <= 0;
		reg_B_15 <= 0;
		reg_C_15 <= 0;
	end else if (enable) begin
		reg_A_0 <= i_A_0;
		reg_B_0 <= i_B_0;
		reg_C_0 <= reg_A_0 + reg_B_0;
		reg_A_1 <= i_A_1;
		reg_B_1 <= i_B_1;
		reg_C_1 <= reg_A_1 + reg_B_1;
		reg_A_2 <= i_A_2;
		reg_B_2 <= i_B_2;
		reg_C_2 <= reg_A_2 + reg_B_2;
		reg_A_3 <= i_A_3;
		reg_B_3 <= i_B_3;
		reg_C_3 <= reg_A_3 + reg_B_3;
		reg_A_4 <= i_A_4;
		reg_B_4 <= i_B_4;
		reg_C_4 <= reg_A_4 + reg_B_4;
		reg_A_5 <= i_A_5;
		reg_B_5 <= i_B_5;
		reg_C_5 <= reg_A_5 + reg_B_5;
		reg_A_6 <= i_A_6;
		reg_B_6 <= i_B_6;
		reg_C_6 <= reg_A_6 + reg_B_6;
		reg_A_7 <= i_A_7;
		reg_B_7 <= i_B_7;
		reg_C_7 <= reg_A_7 + reg_B_7;
		reg_A_8 <= i_A_8;
		reg_B_8 <= i_B_8;
		reg_C_8 <= reg_A_8 + reg_B_8;
		reg_A_9 <= i_A_9;
		reg_B_9 <= i_B_9;
		reg_C_9 <= reg_A_9 + reg_B_9;
		reg_A_10 <= i_A_10;
		reg_B_10 <= i_B_10;
		reg_C_10 <= reg_A_10 + reg_B_10;
		reg_A_11 <= i_A_11;
		reg_B_11 <= i_B_11;
		reg_C_11 <= reg_A_11 + reg_B_11;
		reg_A_12 <= i_A_12;
		reg_B_12 <= i_B_12;
		reg_C_12 <= reg_A_12 + reg_B_12;
		reg_A_13 <= i_A_13;
		reg_B_13 <= i_B_13;
		reg_C_13 <= reg_A_13 + reg_B_13;
		reg_A_14 <= i_A_14;
		reg_B_14 <= i_B_14;
		reg_C_14 <= reg_A_14 + reg_B_14;
		reg_A_15 <= i_A_15;
		reg_B_15 <= i_B_15;
		reg_C_15 <= reg_A_15 + reg_B_15;
		valid_A_B <= i_valid;
		valid_C <= valid_A_B;
	end
end

assign o_C_0 = reg_C_0;
assign o_C_1 = reg_C_1;
assign o_C_2 = reg_C_2;
assign o_C_3 = reg_C_3;
assign o_C_4 = reg_C_4;
assign o_C_5 = reg_C_5;
assign o_C_6 = reg_C_6;
assign o_C_7 = reg_C_7;
assign o_C_8 = reg_C_8;
assign o_C_9 = reg_C_9;
assign o_C_10 = reg_C_10;
assign o_C_11 = reg_C_11;
assign o_C_12 = reg_C_12;
assign o_C_13 = reg_C_13;
assign o_C_14 = reg_C_14;
assign o_C_15 = reg_C_15;
assign o_ready = i_ready;
assign o_valid = valid_C & i_ready;

endmodule

module shift_register_group_18_16_14 (
	input clk,
	input enable,
	input [17:0] in_0,
	output [17:0] out_0,
	input [17:0] in_1,
	output [17:0] out_1,
	input [17:0] in_2,
	output [17:0] out_2,
	input [17:0] in_3,
	output [17:0] out_3,
	input [17:0] in_4,
	output [17:0] out_4,
	input [17:0] in_5,
	output [17:0] out_5,
	input [17:0] in_6,
	output [17:0] out_6,
	input [17:0] in_7,
	output [17:0] out_7,
	input [17:0] in_8,
	output [17:0] out_8,
	input [17:0] in_9,
	output [17:0] out_9,
	input [17:0] in_10,
	output [17:0] out_10,
	input [17:0] in_11,
	output [17:0] out_11,
	input [17:0] in_12,
	output [17:0] out_12,
	input [17:0] in_13,
	output [17:0] out_13,
	input [17:0] in_14,
	output [17:0] out_14,
	input [17:0] in_15,
	output [17:0] out_15,
	input reset
);

shift_register_unit_18_14 shift_register_unit_18_14_inst_0 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_0),
	.out(out_0)
);

shift_register_unit_18_14 shift_register_unit_18_14_inst_1 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_1),
	.out(out_1)
);

shift_register_unit_18_14 shift_register_unit_18_14_inst_2 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_2),
	.out(out_2)
);

shift_register_unit_18_14 shift_register_unit_18_14_inst_3 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_3),
	.out(out_3)
);

shift_register_unit_18_14 shift_register_unit_18_14_inst_4 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_4),
	.out(out_4)
);

shift_register_unit_18_14 shift_register_unit_18_14_inst_5 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_5),
	.out(out_5)
);

shift_register_unit_18_14 shift_register_unit_18_14_inst_6 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_6),
	.out(out_6)
);

shift_register_unit_18_14 shift_register_unit_18_14_inst_7 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_7),
	.out(out_7)
);

shift_register_unit_18_14 shift_register_unit_18_14_inst_8 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_8),
	.out(out_8)
);

shift_register_unit_18_14 shift_register_unit_18_14_inst_9 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_9),
	.out(out_9)
);

shift_register_unit_18_14 shift_register_unit_18_14_inst_10 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_10),
	.out(out_10)
);

shift_register_unit_18_14 shift_register_unit_18_14_inst_11 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_11),
	.out(out_11)
);

shift_register_unit_18_14 shift_register_unit_18_14_inst_12 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_12),
	.out(out_12)
);

shift_register_unit_18_14 shift_register_unit_18_14_inst_13 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_13),
	.out(out_13)
);

shift_register_unit_18_14 shift_register_unit_18_14_inst_14 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_14),
	.out(out_14)
);

shift_register_unit_18_14 shift_register_unit_18_14_inst_15 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_15),
	.out(out_15)
);

endmodule

module shift_register_unit_18_14 (
	input clk,
	input reset,
	input enable,
	input [17:0] in,
	output [17:0] out
);

reg [17:0] shift_registers_0;
reg [17:0] shift_registers_1;
reg [17:0] shift_registers_2;
reg [17:0] shift_registers_3;
reg [17:0] shift_registers_4;
reg [17:0] shift_registers_5;
reg [17:0] shift_registers_6;
reg [17:0] shift_registers_7;
reg [17:0] shift_registers_8;
reg [17:0] shift_registers_9;
reg [17:0] shift_registers_10;
reg [17:0] shift_registers_11;
reg [17:0] shift_registers_12;
reg [17:0] shift_registers_13;
always @ (posedge clk) begin
	if (reset) begin
		shift_registers_0 <= 18'd0;
		shift_registers_1 <= 18'd0;
		shift_registers_2 <= 18'd0;
		shift_registers_3 <= 18'd0;
		shift_registers_4 <= 18'd0;
		shift_registers_5 <= 18'd0;
		shift_registers_6 <= 18'd0;
		shift_registers_7 <= 18'd0;
		shift_registers_8 <= 18'd0;
		shift_registers_9 <= 18'd0;
		shift_registers_10 <= 18'd0;
		shift_registers_11 <= 18'd0;
		shift_registers_12 <= 18'd0;
		shift_registers_13 <= 18'd0;
	end else if (enable) begin
		shift_registers_0 <= in;
		shift_registers_1 <= shift_registers_0;
		shift_registers_2 <= shift_registers_1;
		shift_registers_3 <= shift_registers_2;
		shift_registers_4 <= shift_registers_3;
		shift_registers_5 <= shift_registers_4;
		shift_registers_6 <= shift_registers_5;
		shift_registers_7 <= shift_registers_6;
		shift_registers_8 <= shift_registers_7;
		shift_registers_9 <= shift_registers_8;
		shift_registers_10 <= shift_registers_9;
		shift_registers_11 <= shift_registers_10;
		shift_registers_12 <= shift_registers_11;
		shift_registers_13 <= shift_registers_12;
	end
end

assign out = shift_registers_13;

endmodule

module shift_register_group_18_16_6 (
	input clk,
	input enable,
	input [17:0] in_0,
	output [17:0] out_0,
	input [17:0] in_1,
	output [17:0] out_1,
	input [17:0] in_2,
	output [17:0] out_2,
	input [17:0] in_3,
	output [17:0] out_3,
	input [17:0] in_4,
	output [17:0] out_4,
	input [17:0] in_5,
	output [17:0] out_5,
	input [17:0] in_6,
	output [17:0] out_6,
	input [17:0] in_7,
	output [17:0] out_7,
	input [17:0] in_8,
	output [17:0] out_8,
	input [17:0] in_9,
	output [17:0] out_9,
	input [17:0] in_10,
	output [17:0] out_10,
	input [17:0] in_11,
	output [17:0] out_11,
	input [17:0] in_12,
	output [17:0] out_12,
	input [17:0] in_13,
	output [17:0] out_13,
	input [17:0] in_14,
	output [17:0] out_14,
	input [17:0] in_15,
	output [17:0] out_15,
	input reset
);

shift_register_unit_18_6 shift_register_unit_18_6_inst_0 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_0),
	.out(out_0)
);

shift_register_unit_18_6 shift_register_unit_18_6_inst_1 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_1),
	.out(out_1)
);

shift_register_unit_18_6 shift_register_unit_18_6_inst_2 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_2),
	.out(out_2)
);

shift_register_unit_18_6 shift_register_unit_18_6_inst_3 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_3),
	.out(out_3)
);

shift_register_unit_18_6 shift_register_unit_18_6_inst_4 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_4),
	.out(out_4)
);

shift_register_unit_18_6 shift_register_unit_18_6_inst_5 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_5),
	.out(out_5)
);

shift_register_unit_18_6 shift_register_unit_18_6_inst_6 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_6),
	.out(out_6)
);

shift_register_unit_18_6 shift_register_unit_18_6_inst_7 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_7),
	.out(out_7)
);

shift_register_unit_18_6 shift_register_unit_18_6_inst_8 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_8),
	.out(out_8)
);

shift_register_unit_18_6 shift_register_unit_18_6_inst_9 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_9),
	.out(out_9)
);

shift_register_unit_18_6 shift_register_unit_18_6_inst_10 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_10),
	.out(out_10)
);

shift_register_unit_18_6 shift_register_unit_18_6_inst_11 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_11),
	.out(out_11)
);

shift_register_unit_18_6 shift_register_unit_18_6_inst_12 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_12),
	.out(out_12)
);

shift_register_unit_18_6 shift_register_unit_18_6_inst_13 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_13),
	.out(out_13)
);

shift_register_unit_18_6 shift_register_unit_18_6_inst_14 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_14),
	.out(out_14)
);

shift_register_unit_18_6 shift_register_unit_18_6_inst_15 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_15),
	.out(out_15)
);

endmodule

module shift_register_unit_18_6 (
	input clk,
	input reset,
	input enable,
	input [17:0] in,
	output [17:0] out
);

reg [17:0] shift_registers_0;
reg [17:0] shift_registers_1;
reg [17:0] shift_registers_2;
reg [17:0] shift_registers_3;
reg [17:0] shift_registers_4;
reg [17:0] shift_registers_5;
always @ (posedge clk) begin
	if (reset) begin
		shift_registers_0 <= 18'd0;
		shift_registers_1 <= 18'd0;
		shift_registers_2 <= 18'd0;
		shift_registers_3 <= 18'd0;
		shift_registers_4 <= 18'd0;
		shift_registers_5 <= 18'd0;
	end else if (enable) begin
		shift_registers_0 <= in;
		shift_registers_1 <= shift_registers_0;
		shift_registers_2 <= shift_registers_1;
		shift_registers_3 <= shift_registers_2;
		shift_registers_4 <= shift_registers_3;
		shift_registers_5 <= shift_registers_4;
	end
end

assign out = shift_registers_5;

endmodule

module stage3_X_Y_buffer_18_16_1_10_32_64 (
	input clk,
	input reset,
	input i_X_valid,
	input i_Y_valid,
	input feed_start,
	input [17:0] i_X_data_0,
	input [17:0] i_Y_data_0,
	output [17:0] o_data_0,
	input [17:0] i_X_data_1,
	input [17:0] i_Y_data_1,
	output [17:0] o_data_1,
	input [17:0] i_X_data_2,
	input [17:0] i_Y_data_2,
	output [17:0] o_data_2,
	input [17:0] i_X_data_3,
	input [17:0] i_Y_data_3,
	output [17:0] o_data_3,
	input [17:0] i_X_data_4,
	input [17:0] i_Y_data_4,
	output [17:0] o_data_4,
	input [17:0] i_X_data_5,
	input [17:0] i_Y_data_5,
	output [17:0] o_data_5,
	input [17:0] i_X_data_6,
	input [17:0] i_Y_data_6,
	output [17:0] o_data_6,
	input [17:0] i_X_data_7,
	input [17:0] i_Y_data_7,
	output [17:0] o_data_7,
	input [17:0] i_X_data_8,
	input [17:0] i_Y_data_8,
	output [17:0] o_data_8,
	input [17:0] i_X_data_9,
	input [17:0] i_Y_data_9,
	output [17:0] o_data_9,
	input [17:0] i_X_data_10,
	input [17:0] i_Y_data_10,
	output [17:0] o_data_10,
	input [17:0] i_X_data_11,
	input [17:0] i_Y_data_11,
	output [17:0] o_data_11,
	input [17:0] i_X_data_12,
	input [17:0] i_Y_data_12,
	output [17:0] o_data_12,
	input [17:0] i_X_data_13,
	input [17:0] i_Y_data_13,
	output [17:0] o_data_13,
	input [17:0] i_X_data_14,
	input [17:0] i_Y_data_14,
	output [17:0] o_data_14,
	input [17:0] i_X_data_15,
	input [17:0] i_Y_data_15,
	output [17:0] o_data_15,
	output o_valid,
	output o_ready
);

reg reg_feed_start;
reg [17:0] i_data_0;
reg [17:0] i_data_1;
reg [17:0] i_data_2;
reg [17:0] i_data_3;
reg [17:0] i_data_4;
reg [17:0] i_data_5;
reg [17:0] i_data_6;
reg [17:0] i_data_7;
reg [17:0] i_data_8;
reg [17:0] i_data_9;
reg [17:0] i_data_10;
reg [17:0] i_data_11;
reg [17:0] i_data_12;
reg [17:0] i_data_13;
reg [17:0] i_data_14;
reg [17:0] i_data_15;
wire [287:0] packed_o_data;
wire [287:0] packed_data;
reg wen;
wire ready_to_accept_new_X;
wire [13:0] input_index_counter;
assign ready_to_accept_new_X = (input_index_counter >= 32);
assign o_ready = ready_to_accept_new_X;
always @ (*) begin
	if(ready_to_accept_new_X) begin
		wen <= i_X_valid;
		i_data_0 <= i_X_data_0;
		i_data_1 <= i_X_data_1;
		i_data_2 <= i_X_data_2;
		i_data_3 <= i_X_data_3;
		i_data_4 <= i_X_data_4;
		i_data_5 <= i_X_data_5;
		i_data_6 <= i_X_data_6;
		i_data_7 <= i_X_data_7;
		i_data_8 <= i_X_data_8;
		i_data_9 <= i_X_data_9;
		i_data_10 <= i_X_data_10;
		i_data_11 <= i_X_data_11;
		i_data_12 <= i_X_data_12;
		i_data_13 <= i_X_data_13;
		i_data_14 <= i_X_data_14;
		i_data_15 <= i_X_data_15;
	end else begin
		wen <= i_Y_valid;
		i_data_0 <= i_Y_data_0;
		i_data_1 <= i_Y_data_1;
		i_data_2 <= i_Y_data_2;
		i_data_3 <= i_Y_data_3;
		i_data_4 <= i_Y_data_4;
		i_data_5 <= i_Y_data_5;
		i_data_6 <= i_Y_data_6;
		i_data_7 <= i_Y_data_7;
		i_data_8 <= i_Y_data_8;
		i_data_9 <= i_Y_data_9;
		i_data_10 <= i_Y_data_10;
		i_data_11 <= i_Y_data_11;
		i_data_12 <= i_Y_data_12;
		i_data_13 <= i_Y_data_13;
		i_data_14 <= i_Y_data_14;
		i_data_15 <= i_Y_data_15;
	end
end

assign o_data_0 = packed_o_data[17:0];
assign packed_data[17:0] = i_data_0;
assign o_data_1 = packed_o_data[35:18];
assign packed_data[35:18] = i_data_1;
assign o_data_2 = packed_o_data[53:36];
assign packed_data[53:36] = i_data_2;
assign o_data_3 = packed_o_data[71:54];
assign packed_data[71:54] = i_data_3;
assign o_data_4 = packed_o_data[89:72];
assign packed_data[89:72] = i_data_4;
assign o_data_5 = packed_o_data[107:90];
assign packed_data[107:90] = i_data_5;
assign o_data_6 = packed_o_data[125:108];
assign packed_data[125:108] = i_data_6;
assign o_data_7 = packed_o_data[143:126];
assign packed_data[143:126] = i_data_7;
assign o_data_8 = packed_o_data[161:144];
assign packed_data[161:144] = i_data_8;
assign o_data_9 = packed_o_data[179:162];
assign packed_data[179:162] = i_data_9;
assign o_data_10 = packed_o_data[197:180];
assign packed_data[197:180] = i_data_10;
assign o_data_11 = packed_o_data[215:198];
assign packed_data[215:198] = i_data_11;
assign o_data_12 = packed_o_data[233:216];
assign packed_data[233:216] = i_data_12;
assign o_data_13 = packed_o_data[251:234];
assign packed_data[251:234] = i_data_13;
assign o_data_14 = packed_o_data[269:252];
assign packed_data[269:252] = i_data_14;
assign o_data_15 = packed_o_data[287:270];
assign packed_data[287:270] = i_data_15;
counter_41_1_32 counter_41_1_32_inst (
	.clk(clk),
	.reset(reset),
	.ena(wen),
	.count(input_index_counter)
);

wire [13:0] output_index_counter;
reg en_output_counter;
counter_41_1 counter_41_1_inst (
	.clk(clk),
	.reset(reset),
	.ena(en_output_counter),
	.count(output_index_counter)
);

wire incr_loop_index;
assign incr_loop_index = (output_index_counter == 41 && en_output_counter);
reg output_finish;
wire [13:0] loop_counter;
counter_63_1 counter_63_1_inst_loop (
	.clk(clk),
	.reset(reset),
	.ena(incr_loop_index),
	.count(loop_counter)
);

ram_288_0_42 ram_288_0_42_inst (
	.clk(clk),
	.waddr(input_index_counter),
	.wdata(packed_data),
	.wen(wen),
	.raddr(output_index_counter),
	.q(packed_o_data)
);

shift_register_unit_1_2 shift_register_unit_1_2_inst (
	.clk(clk),
	.reset(reset),
	.enable(1'b1),
	.in(en_output_counter),
	.out(o_valid)
);

always @ (posedge clk) begin
	if (reset) begin
		en_output_counter <= 1'b0;
		output_finish <= 1'b0;
		reg_feed_start <= 1'b0;
	end else begin
		en_output_counter <= (reg_feed_start && ~en_output_counter && ~output_finish);
		if(feed_start)
			reg_feed_start <= 1'b1;
		else if (output_finish)
			reg_feed_start <= 1'b0;
		if ((loop_counter == 63)
			&&(output_index_counter == 41)
			&& en_output_counter)
			output_finish <= 1'b1;
		else if (loop_counter == 0 && wen)
			output_finish <= 1'b0;
	end
end

endmodule

module counter_41_1_32 (
	input clk,
	input reset,
	input ena,
	output reg [13:0] count
);

always @ (posedge clk) begin 
	if (reset) begin
		count <= 32;
	end else if (ena) begin
		if((count + 1) <= 41) begin
			count <= count + 1;
		end else begin
			count <= 14'd0;
		end
	end
end

endmodule

module shift_register_unit_1_2 (
	input clk,
	input reset,
	input enable,
	input [0:0] in,
	output [0:0] out
);

reg [0:0] shift_registers_0;
reg [0:0] shift_registers_1;
always @ (posedge clk) begin
	if (reset) begin
		shift_registers_0 <= 1'd0;
		shift_registers_1 <= 1'd0;
	end else if (enable) begin
		shift_registers_0 <= in;
		shift_registers_1 <= shift_registers_0;
	end
end

assign out = shift_registers_1;

endmodule

module ram_288_0_42 (
	input clk,
	input [5:0] waddr,
	input [287:0] wdata,
	input wen,
	input [5:0] raddr,
	output [287:0] q
);

wire [287:0] rd_dummy_signal;
wire [287:0] wr_dummy_signal;
assign rd_dummy_signal = 0;

dual_port_ram u_dual_port_ram(
	.addr1(waddr),
	.we1(wen),
	.data1(wdata),
	.out1(wr_dummy_signal),
	.addr2(raddr),
	.we2(1'b0),
	.data2(rd_dummy_signal),
	.out2(q),
	.clk(clk)
);
endmodule

module shift_register_unit_1_3 (
	input clk,
	input reset,
	input enable,
	input [0:0] in,
	output [0:0] out
);

reg [0:0] shift_registers_0;
reg [0:0] shift_registers_1;
reg [0:0] shift_registers_2;
always @ (posedge clk) begin
	if (reset) begin
		shift_registers_0 <= 1'd0;
		shift_registers_1 <= 1'd0;
		shift_registers_2 <= 1'd0;
	end else if (enable) begin
		shift_registers_0 <= in;
		shift_registers_1 <= shift_registers_0;
		shift_registers_2 <= shift_registers_1;
	end
end

assign out = shift_registers_2;

endmodule

module shift_register_group_18_16_3 (
	input clk,
	input enable,
	input [17:0] in_0,
	output [17:0] out_0,
	input [17:0] in_1,
	output [17:0] out_1,
	input [17:0] in_2,
	output [17:0] out_2,
	input [17:0] in_3,
	output [17:0] out_3,
	input [17:0] in_4,
	output [17:0] out_4,
	input [17:0] in_5,
	output [17:0] out_5,
	input [17:0] in_6,
	output [17:0] out_6,
	input [17:0] in_7,
	output [17:0] out_7,
	input [17:0] in_8,
	output [17:0] out_8,
	input [17:0] in_9,
	output [17:0] out_9,
	input [17:0] in_10,
	output [17:0] out_10,
	input [17:0] in_11,
	output [17:0] out_11,
	input [17:0] in_12,
	output [17:0] out_12,
	input [17:0] in_13,
	output [17:0] out_13,
	input [17:0] in_14,
	output [17:0] out_14,
	input [17:0] in_15,
	output [17:0] out_15,
	input reset
);

shift_register_unit_18_3 shift_register_unit_18_3_inst_0 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_0),
	.out(out_0)
);

shift_register_unit_18_3 shift_register_unit_18_3_inst_1 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_1),
	.out(out_1)
);

shift_register_unit_18_3 shift_register_unit_18_3_inst_2 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_2),
	.out(out_2)
);

shift_register_unit_18_3 shift_register_unit_18_3_inst_3 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_3),
	.out(out_3)
);

shift_register_unit_18_3 shift_register_unit_18_3_inst_4 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_4),
	.out(out_4)
);

shift_register_unit_18_3 shift_register_unit_18_3_inst_5 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_5),
	.out(out_5)
);

shift_register_unit_18_3 shift_register_unit_18_3_inst_6 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_6),
	.out(out_6)
);

shift_register_unit_18_3 shift_register_unit_18_3_inst_7 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_7),
	.out(out_7)
);

shift_register_unit_18_3 shift_register_unit_18_3_inst_8 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_8),
	.out(out_8)
);

shift_register_unit_18_3 shift_register_unit_18_3_inst_9 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_9),
	.out(out_9)
);

shift_register_unit_18_3 shift_register_unit_18_3_inst_10 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_10),
	.out(out_10)
);

shift_register_unit_18_3 shift_register_unit_18_3_inst_11 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_11),
	.out(out_11)
);

shift_register_unit_18_3 shift_register_unit_18_3_inst_12 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_12),
	.out(out_12)
);

shift_register_unit_18_3 shift_register_unit_18_3_inst_13 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_13),
	.out(out_13)
);

shift_register_unit_18_3 shift_register_unit_18_3_inst_14 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_14),
	.out(out_14)
);

shift_register_unit_18_3 shift_register_unit_18_3_inst_15 (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.in(in_15),
	.out(out_15)
);

endmodule

module pipelined_input_18_1_16 (
	input clk,
	input reset,
	input enable,
	input load_input,
	input [17:0] i_data_0_0,
	input [17:0] i_data_0_1,
	input [17:0] i_data_0_2,
	input [17:0] i_data_0_3,
	input [17:0] i_data_0_4,
	input [17:0] i_data_0_5,
	input [17:0] i_data_0_6,
	input [17:0] i_data_0_7,
	input [17:0] i_data_0_8,
	input [17:0] i_data_0_9,
	input [17:0] i_data_0_10,
	input [17:0] i_data_0_11,
	input [17:0] i_data_0_12,
	input [17:0] i_data_0_13,
	input [17:0] i_data_0_14,
	input [17:0] i_data_0_15,
	output [17:0] o_data_0,
	output [17:0] o_data_1,
	output [17:0] o_data_2,
	output [17:0] o_data_3,
	output [17:0] o_data_4,
	output [17:0] o_data_5,
	output [17:0] o_data_6,
	output [17:0] o_data_7,
	output [17:0] o_data_8,
	output [17:0] o_data_9,
	output [17:0] o_data_10,
	output [17:0] o_data_11,
	output [17:0] o_data_12,
	output [17:0] o_data_13,
	output [17:0] o_data_14,
	output [17:0] o_data_15,
	output o_valid
);

reg [17:0] pipeline_0_0;
reg [17:0] pipeline_0_1;
reg [17:0] pipeline_0_2;
reg [17:0] pipeline_0_3;
reg [17:0] pipeline_0_4;
reg [17:0] pipeline_0_5;
reg [17:0] pipeline_0_6;
reg [17:0] pipeline_0_7;
reg [17:0] pipeline_0_8;
reg [17:0] pipeline_0_9;
reg [17:0] pipeline_0_10;
reg [17:0] pipeline_0_11;
reg [17:0] pipeline_0_12;
reg [17:0] pipeline_0_13;
reg [17:0] pipeline_0_14;
reg [17:0] pipeline_0_15;
reg pipeline_valid_0;

always @ (posedge clk) begin
	if (reset) begin
		pipeline_0_0 <= 0;
		pipeline_0_1 <= 0;
		pipeline_0_2 <= 0;
		pipeline_0_3 <= 0;
		pipeline_0_4 <= 0;
		pipeline_0_5 <= 0;
		pipeline_0_6 <= 0;
		pipeline_0_7 <= 0;
		pipeline_0_8 <= 0;
		pipeline_0_9 <= 0;
		pipeline_0_10 <= 0;
		pipeline_0_11 <= 0;
		pipeline_0_12 <= 0;
		pipeline_0_13 <= 0;
		pipeline_0_14 <= 0;
		pipeline_0_15 <= 0;
		pipeline_valid_0 <= 0;
	end else if (enable) begin
		if (load_input) begin
			pipeline_0_0 <= i_data_0_0;
			pipeline_0_1 <= i_data_0_1;
			pipeline_0_2 <= i_data_0_2;
			pipeline_0_3 <= i_data_0_3;
			pipeline_0_4 <= i_data_0_4;
			pipeline_0_5 <= i_data_0_5;
			pipeline_0_6 <= i_data_0_6;
			pipeline_0_7 <= i_data_0_7;
			pipeline_0_8 <= i_data_0_8;
			pipeline_0_9 <= i_data_0_9;
			pipeline_0_10 <= i_data_0_10;
			pipeline_0_11 <= i_data_0_11;
			pipeline_0_12 <= i_data_0_12;
			pipeline_0_13 <= i_data_0_13;
			pipeline_0_14 <= i_data_0_14;
			pipeline_0_15 <= i_data_0_15;
			pipeline_valid_0 <= 1'b1;
		end else begin
			pipeline_0_0 <= 0;
			pipeline_0_1 <= 0;
			pipeline_0_2 <= 0;
			pipeline_0_3 <= 0;
			pipeline_0_4 <= 0;
			pipeline_0_5 <= 0;
			pipeline_0_6 <= 0;
			pipeline_0_7 <= 0;
			pipeline_0_8 <= 0;
			pipeline_0_9 <= 0;
			pipeline_0_10 <= 0;
			pipeline_0_11 <= 0;
			pipeline_0_12 <= 0;
			pipeline_0_13 <= 0;
			pipeline_0_14 <= 0;
			pipeline_0_15 <= 0;
			pipeline_valid_0 <= 1'b0;
		end
	end
end

assign o_data_0 = pipeline_0_0;
assign o_data_1 = pipeline_0_1;
assign o_data_2 = pipeline_0_2;
assign o_data_3 = pipeline_0_3;
assign o_data_4 = pipeline_0_4;
assign o_data_5 = pipeline_0_5;
assign o_data_6 = pipeline_0_6;
assign o_data_7 = pipeline_0_7;
assign o_data_8 = pipeline_0_8;
assign o_data_9 = pipeline_0_9;
assign o_data_10 = pipeline_0_10;
assign o_data_11 = pipeline_0_11;
assign o_data_12 = pipeline_0_12;
assign o_data_13 = pipeline_0_13;
assign o_data_14 = pipeline_0_14;
assign o_data_15 = pipeline_0_15;
assign o_valid = pipeline_valid_0;

endmodule

module C_LSTM_stage_1_18_10_160_512_1_16_1 (
	input clk,
	input reset,
	input enable,
	input i_ready,
	input [17:0] i_Xt_Yt_1_0,
	input [17:0] i_Xt_Yt_1_1,
	input [17:0] i_Xt_Yt_1_2,
	input [17:0] i_Xt_Yt_1_3,
	input [17:0] i_Xt_Yt_1_4,
	input [17:0] i_Xt_Yt_1_5,
	input [17:0] i_Xt_Yt_1_6,
	input [17:0] i_Xt_Yt_1_7,
	input [17:0] i_Xt_Yt_1_8,
	input [17:0] i_Xt_Yt_1_9,
	input [17:0] i_Xt_Yt_1_10,
	input [17:0] i_Xt_Yt_1_11,
	input [17:0] i_Xt_Yt_1_12,
	input [17:0] i_Xt_Yt_1_13,
	input [17:0] i_Xt_Yt_1_14,
	input [17:0] i_Xt_Yt_1_15,
	input [17:0] i_Wixr_real_0_0,
	input [17:0] i_Wixr_imag_0_0,
	input [17:0] i_Wfxr_real_0_0,
	input [17:0] i_Wfxr_imag_0_0,
	input [17:0] i_Woxr_real_0_0,
	input [17:0] i_Woxr_imag_0_0,
	input [17:0] i_Wcxr_real_0_0,
	input [17:0] i_Wcxr_imag_0_0,
	input [17:0] i_Wixr_real_0_1,
	input [17:0] i_Wixr_imag_0_1,
	input [17:0] i_Wfxr_real_0_1,
	input [17:0] i_Wfxr_imag_0_1,
	input [17:0] i_Woxr_real_0_1,
	input [17:0] i_Woxr_imag_0_1,
	input [17:0] i_Wcxr_real_0_1,
	input [17:0] i_Wcxr_imag_0_1,
	input [17:0] i_Wixr_real_0_2,
	input [17:0] i_Wixr_imag_0_2,
	input [17:0] i_Wfxr_real_0_2,
	input [17:0] i_Wfxr_imag_0_2,
	input [17:0] i_Woxr_real_0_2,
	input [17:0] i_Woxr_imag_0_2,
	input [17:0] i_Wcxr_real_0_2,
	input [17:0] i_Wcxr_imag_0_2,
	input [17:0] i_Wixr_real_0_3,
	input [17:0] i_Wixr_imag_0_3,
	input [17:0] i_Wfxr_real_0_3,
	input [17:0] i_Wfxr_imag_0_3,
	input [17:0] i_Woxr_real_0_3,
	input [17:0] i_Woxr_imag_0_3,
	input [17:0] i_Wcxr_real_0_3,
	input [17:0] i_Wcxr_imag_0_3,
	input [17:0] i_Wixr_real_0_4,
	input [17:0] i_Wixr_imag_0_4,
	input [17:0] i_Wfxr_real_0_4,
	input [17:0] i_Wfxr_imag_0_4,
	input [17:0] i_Woxr_real_0_4,
	input [17:0] i_Woxr_imag_0_4,
	input [17:0] i_Wcxr_real_0_4,
	input [17:0] i_Wcxr_imag_0_4,
	input [17:0] i_Wixr_real_0_5,
	input [17:0] i_Wixr_imag_0_5,
	input [17:0] i_Wfxr_real_0_5,
	input [17:0] i_Wfxr_imag_0_5,
	input [17:0] i_Woxr_real_0_5,
	input [17:0] i_Woxr_imag_0_5,
	input [17:0] i_Wcxr_real_0_5,
	input [17:0] i_Wcxr_imag_0_5,
	input [17:0] i_Wixr_real_0_6,
	input [17:0] i_Wixr_imag_0_6,
	input [17:0] i_Wfxr_real_0_6,
	input [17:0] i_Wfxr_imag_0_6,
	input [17:0] i_Woxr_real_0_6,
	input [17:0] i_Woxr_imag_0_6,
	input [17:0] i_Wcxr_real_0_6,
	input [17:0] i_Wcxr_imag_0_6,
	input [17:0] i_Wixr_real_0_7,
	input [17:0] i_Wixr_imag_0_7,
	input [17:0] i_Wfxr_real_0_7,
	input [17:0] i_Wfxr_imag_0_7,
	input [17:0] i_Woxr_real_0_7,
	input [17:0] i_Woxr_imag_0_7,
	input [17:0] i_Wcxr_real_0_7,
	input [17:0] i_Wcxr_imag_0_7,
	input [17:0] i_Wixr_real_0_8,
	input [17:0] i_Wixr_imag_0_8,
	input [17:0] i_Wfxr_real_0_8,
	input [17:0] i_Wfxr_imag_0_8,
	input [17:0] i_Woxr_real_0_8,
	input [17:0] i_Woxr_imag_0_8,
	input [17:0] i_Wcxr_real_0_8,
	input [17:0] i_Wcxr_imag_0_8,
	output o_valid,
	output o_ready,
	output [17:0] o_WixrXtYt_1_0_0,
	output [17:0] o_WfxrXtYt_1_0_0,
	output [17:0] o_WoxrXtYt_1_0_0,
	output [17:0] o_WcxrXtYt_1_0_0,
	output [17:0] o_WixrXtYt_1_0_1,
	output [17:0] o_WfxrXtYt_1_0_1,
	output [17:0] o_WoxrXtYt_1_0_1,
	output [17:0] o_WcxrXtYt_1_0_1,
	output [17:0] o_WixrXtYt_1_0_2,
	output [17:0] o_WfxrXtYt_1_0_2,
	output [17:0] o_WoxrXtYt_1_0_2,
	output [17:0] o_WcxrXtYt_1_0_2,
	output [17:0] o_WixrXtYt_1_0_3,
	output [17:0] o_WfxrXtYt_1_0_3,
	output [17:0] o_WoxrXtYt_1_0_3,
	output [17:0] o_WcxrXtYt_1_0_3,
	output [17:0] o_WixrXtYt_1_0_4,
	output [17:0] o_WfxrXtYt_1_0_4,
	output [17:0] o_WoxrXtYt_1_0_4,
	output [17:0] o_WcxrXtYt_1_0_4,
	output [17:0] o_WixrXtYt_1_0_5,
	output [17:0] o_WfxrXtYt_1_0_5,
	output [17:0] o_WoxrXtYt_1_0_5,
	output [17:0] o_WcxrXtYt_1_0_5,
	output [17:0] o_WixrXtYt_1_0_6,
	output [17:0] o_WfxrXtYt_1_0_6,
	output [17:0] o_WoxrXtYt_1_0_6,
	output [17:0] o_WcxrXtYt_1_0_6,
	output [17:0] o_WixrXtYt_1_0_7,
	output [17:0] o_WfxrXtYt_1_0_7,
	output [17:0] o_WoxrXtYt_1_0_7,
	output [17:0] o_WcxrXtYt_1_0_7,
	output [17:0] o_WixrXtYt_1_0_8,
	output [17:0] o_WfxrXtYt_1_0_8,
	output [17:0] o_WoxrXtYt_1_0_8,
	output [17:0] o_WcxrXtYt_1_0_8,
	output [17:0] o_WixrXtYt_1_0_9,
	output [17:0] o_WfxrXtYt_1_0_9,
	output [17:0] o_WoxrXtYt_1_0_9,
	output [17:0] o_WcxrXtYt_1_0_9,
	output [17:0] o_WixrXtYt_1_0_10,
	output [17:0] o_WfxrXtYt_1_0_10,
	output [17:0] o_WoxrXtYt_1_0_10,
	output [17:0] o_WcxrXtYt_1_0_10,
	output [17:0] o_WixrXtYt_1_0_11,
	output [17:0] o_WfxrXtYt_1_0_11,
	output [17:0] o_WoxrXtYt_1_0_11,
	output [17:0] o_WcxrXtYt_1_0_11,
	output [17:0] o_WixrXtYt_1_0_12,
	output [17:0] o_WfxrXtYt_1_0_12,
	output [17:0] o_WoxrXtYt_1_0_12,
	output [17:0] o_WcxrXtYt_1_0_12,
	output [17:0] o_WixrXtYt_1_0_13,
	output [17:0] o_WfxrXtYt_1_0_13,
	output [17:0] o_WoxrXtYt_1_0_13,
	output [17:0] o_WcxrXtYt_1_0_13,
	output [17:0] o_WixrXtYt_1_0_14,
	output [17:0] o_WfxrXtYt_1_0_14,
	output [17:0] o_WoxrXtYt_1_0_14,
	output [17:0] o_WcxrXtYt_1_0_14,
	output [17:0] o_WixrXtYt_1_0_15,
	output [17:0] o_WfxrXtYt_1_0_15,
	output [17:0] o_WoxrXtYt_1_0_15,
	output [17:0] o_WcxrXtYt_1_0_15,
	input i_valid
);

wire input_gate_mult_valid, forget_gate_mult_valid, output_gate_mult_valid, output_act_mult_valid;
wire input_gate_mult_ready, forget_gate_mult_ready, output_gate_mult_ready, output_act_mult_ready;

// Input Gate Multiplication 
matrix_times_two_vectors_18_10_1_672_16_1 input_gate_mult (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_ready(i_ready),
	.i_valid(i_valid),
	.i_Xt_Yt_1_0(i_Xt_Yt_1_0),
	.i_Xt_Yt_1_1(i_Xt_Yt_1_1),
	.i_Xt_Yt_1_2(i_Xt_Yt_1_2),
	.i_Xt_Yt_1_3(i_Xt_Yt_1_3),
	.i_Xt_Yt_1_4(i_Xt_Yt_1_4),
	.i_Xt_Yt_1_5(i_Xt_Yt_1_5),
	.i_Xt_Yt_1_6(i_Xt_Yt_1_6),
	.i_Xt_Yt_1_7(i_Xt_Yt_1_7),
	.i_Xt_Yt_1_8(i_Xt_Yt_1_8),
	.i_Xt_Yt_1_9(i_Xt_Yt_1_9),
	.i_Xt_Yt_1_10(i_Xt_Yt_1_10),
	.i_Xt_Yt_1_11(i_Xt_Yt_1_11),
	.i_Xt_Yt_1_12(i_Xt_Yt_1_12),
	.i_Xt_Yt_1_13(i_Xt_Yt_1_13),
	.i_Xt_Yt_1_14(i_Xt_Yt_1_14),
	.i_Xt_Yt_1_15(i_Xt_Yt_1_15),
	.i_Wxr_real_0_0(i_Wixr_real_0_0),
	.i_Wxr_imag_0_0(i_Wixr_imag_0_0),
	.i_Wxr_real_0_1(i_Wixr_real_0_1),
	.i_Wxr_imag_0_1(i_Wixr_imag_0_1),
	.i_Wxr_real_0_2(i_Wixr_real_0_2),
	.i_Wxr_imag_0_2(i_Wixr_imag_0_2),
	.i_Wxr_real_0_3(i_Wixr_real_0_3),
	.i_Wxr_imag_0_3(i_Wixr_imag_0_3),
	.i_Wxr_real_0_4(i_Wixr_real_0_4),
	.i_Wxr_imag_0_4(i_Wixr_imag_0_4),
	.i_Wxr_real_0_5(i_Wixr_real_0_5),
	.i_Wxr_imag_0_5(i_Wixr_imag_0_5),
	.i_Wxr_real_0_6(i_Wixr_real_0_6),
	.i_Wxr_imag_0_6(i_Wixr_imag_0_6),
	.i_Wxr_real_0_7(i_Wixr_real_0_7),
	.i_Wxr_imag_0_7(i_Wixr_imag_0_7),
	.i_Wxr_real_0_8(i_Wixr_real_0_8),
	.i_Wxr_imag_0_8(i_Wixr_imag_0_8),
	.o_W_times_X_Y_0_0(o_WixrXtYt_1_0_0),
	.o_W_times_X_Y_0_1(o_WixrXtYt_1_0_1),
	.o_W_times_X_Y_0_2(o_WixrXtYt_1_0_2),
	.o_W_times_X_Y_0_3(o_WixrXtYt_1_0_3),
	.o_W_times_X_Y_0_4(o_WixrXtYt_1_0_4),
	.o_W_times_X_Y_0_5(o_WixrXtYt_1_0_5),
	.o_W_times_X_Y_0_6(o_WixrXtYt_1_0_6),
	.o_W_times_X_Y_0_7(o_WixrXtYt_1_0_7),
	.o_W_times_X_Y_0_8(o_WixrXtYt_1_0_8),
	.o_W_times_X_Y_0_9(o_WixrXtYt_1_0_9),
	.o_W_times_X_Y_0_10(o_WixrXtYt_1_0_10),
	.o_W_times_X_Y_0_11(o_WixrXtYt_1_0_11),
	.o_W_times_X_Y_0_12(o_WixrXtYt_1_0_12),
	.o_W_times_X_Y_0_13(o_WixrXtYt_1_0_13),
	.o_W_times_X_Y_0_14(o_WixrXtYt_1_0_14),
	.o_W_times_X_Y_0_15(o_WixrXtYt_1_0_15),
	.o_valid(input_gate_mult_valid),
	.o_ready(input_gate_mult_ready)
);

// Forget Gate Multiplication 
matrix_times_two_vectors_18_10_1_672_16_1 forget_gate_mult (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_ready(i_ready),
	.i_valid(i_valid),
	.i_Xt_Yt_1_0(i_Xt_Yt_1_0),
	.i_Xt_Yt_1_1(i_Xt_Yt_1_1),
	.i_Xt_Yt_1_2(i_Xt_Yt_1_2),
	.i_Xt_Yt_1_3(i_Xt_Yt_1_3),
	.i_Xt_Yt_1_4(i_Xt_Yt_1_4),
	.i_Xt_Yt_1_5(i_Xt_Yt_1_5),
	.i_Xt_Yt_1_6(i_Xt_Yt_1_6),
	.i_Xt_Yt_1_7(i_Xt_Yt_1_7),
	.i_Xt_Yt_1_8(i_Xt_Yt_1_8),
	.i_Xt_Yt_1_9(i_Xt_Yt_1_9),
	.i_Xt_Yt_1_10(i_Xt_Yt_1_10),
	.i_Xt_Yt_1_11(i_Xt_Yt_1_11),
	.i_Xt_Yt_1_12(i_Xt_Yt_1_12),
	.i_Xt_Yt_1_13(i_Xt_Yt_1_13),
	.i_Xt_Yt_1_14(i_Xt_Yt_1_14),
	.i_Xt_Yt_1_15(i_Xt_Yt_1_15),
	.i_Wxr_real_0_0(i_Wfxr_real_0_0),
	.i_Wxr_imag_0_0(i_Wfxr_imag_0_0),
	.i_Wxr_real_0_1(i_Wfxr_real_0_1),
	.i_Wxr_imag_0_1(i_Wfxr_imag_0_1),
	.i_Wxr_real_0_2(i_Wfxr_real_0_2),
	.i_Wxr_imag_0_2(i_Wfxr_imag_0_2),
	.i_Wxr_real_0_3(i_Wfxr_real_0_3),
	.i_Wxr_imag_0_3(i_Wfxr_imag_0_3),
	.i_Wxr_real_0_4(i_Wfxr_real_0_4),
	.i_Wxr_imag_0_4(i_Wfxr_imag_0_4),
	.i_Wxr_real_0_5(i_Wfxr_real_0_5),
	.i_Wxr_imag_0_5(i_Wfxr_imag_0_5),
	.i_Wxr_real_0_6(i_Wfxr_real_0_6),
	.i_Wxr_imag_0_6(i_Wfxr_imag_0_6),
	.i_Wxr_real_0_7(i_Wfxr_real_0_7),
	.i_Wxr_imag_0_7(i_Wfxr_imag_0_7),
	.i_Wxr_real_0_8(i_Wfxr_real_0_8),
	.i_Wxr_imag_0_8(i_Wfxr_imag_0_8),
	.o_W_times_X_Y_0_0(o_WfxrXtYt_1_0_0),
	.o_W_times_X_Y_0_1(o_WfxrXtYt_1_0_1),
	.o_W_times_X_Y_0_2(o_WfxrXtYt_1_0_2),
	.o_W_times_X_Y_0_3(o_WfxrXtYt_1_0_3),
	.o_W_times_X_Y_0_4(o_WfxrXtYt_1_0_4),
	.o_W_times_X_Y_0_5(o_WfxrXtYt_1_0_5),
	.o_W_times_X_Y_0_6(o_WfxrXtYt_1_0_6),
	.o_W_times_X_Y_0_7(o_WfxrXtYt_1_0_7),
	.o_W_times_X_Y_0_8(o_WfxrXtYt_1_0_8),
	.o_W_times_X_Y_0_9(o_WfxrXtYt_1_0_9),
	.o_W_times_X_Y_0_10(o_WfxrXtYt_1_0_10),
	.o_W_times_X_Y_0_11(o_WfxrXtYt_1_0_11),
	.o_W_times_X_Y_0_12(o_WfxrXtYt_1_0_12),
	.o_W_times_X_Y_0_13(o_WfxrXtYt_1_0_13),
	.o_W_times_X_Y_0_14(o_WfxrXtYt_1_0_14),
	.o_W_times_X_Y_0_15(o_WfxrXtYt_1_0_15),
	.o_valid(forget_gate_mult_valid),
	.o_ready(forget_gate_mult_ready)
);

// Output Gate Multiplication 
matrix_times_two_vectors_18_10_1_672_16_1 output_gate_mult (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_ready(i_ready),
	.i_valid(i_valid),
	.i_Xt_Yt_1_0(i_Xt_Yt_1_0),
	.i_Xt_Yt_1_1(i_Xt_Yt_1_1),
	.i_Xt_Yt_1_2(i_Xt_Yt_1_2),
	.i_Xt_Yt_1_3(i_Xt_Yt_1_3),
	.i_Xt_Yt_1_4(i_Xt_Yt_1_4),
	.i_Xt_Yt_1_5(i_Xt_Yt_1_5),
	.i_Xt_Yt_1_6(i_Xt_Yt_1_6),
	.i_Xt_Yt_1_7(i_Xt_Yt_1_7),
	.i_Xt_Yt_1_8(i_Xt_Yt_1_8),
	.i_Xt_Yt_1_9(i_Xt_Yt_1_9),
	.i_Xt_Yt_1_10(i_Xt_Yt_1_10),
	.i_Xt_Yt_1_11(i_Xt_Yt_1_11),
	.i_Xt_Yt_1_12(i_Xt_Yt_1_12),
	.i_Xt_Yt_1_13(i_Xt_Yt_1_13),
	.i_Xt_Yt_1_14(i_Xt_Yt_1_14),
	.i_Xt_Yt_1_15(i_Xt_Yt_1_15),
	.i_Wxr_real_0_0(i_Woxr_real_0_0),
	.i_Wxr_imag_0_0(i_Woxr_imag_0_0),
	.i_Wxr_real_0_1(i_Woxr_real_0_1),
	.i_Wxr_imag_0_1(i_Woxr_imag_0_1),
	.i_Wxr_real_0_2(i_Woxr_real_0_2),
	.i_Wxr_imag_0_2(i_Woxr_imag_0_2),
	.i_Wxr_real_0_3(i_Woxr_real_0_3),
	.i_Wxr_imag_0_3(i_Woxr_imag_0_3),
	.i_Wxr_real_0_4(i_Woxr_real_0_4),
	.i_Wxr_imag_0_4(i_Woxr_imag_0_4),
	.i_Wxr_real_0_5(i_Woxr_real_0_5),
	.i_Wxr_imag_0_5(i_Woxr_imag_0_5),
	.i_Wxr_real_0_6(i_Woxr_real_0_6),
	.i_Wxr_imag_0_6(i_Woxr_imag_0_6),
	.i_Wxr_real_0_7(i_Woxr_real_0_7),
	.i_Wxr_imag_0_7(i_Woxr_imag_0_7),
	.i_Wxr_real_0_8(i_Woxr_real_0_8),
	.i_Wxr_imag_0_8(i_Woxr_imag_0_8),
	.o_W_times_X_Y_0_0(o_WoxrXtYt_1_0_0),
	.o_W_times_X_Y_0_1(o_WoxrXtYt_1_0_1),
	.o_W_times_X_Y_0_2(o_WoxrXtYt_1_0_2),
	.o_W_times_X_Y_0_3(o_WoxrXtYt_1_0_3),
	.o_W_times_X_Y_0_4(o_WoxrXtYt_1_0_4),
	.o_W_times_X_Y_0_5(o_WoxrXtYt_1_0_5),
	.o_W_times_X_Y_0_6(o_WoxrXtYt_1_0_6),
	.o_W_times_X_Y_0_7(o_WoxrXtYt_1_0_7),
	.o_W_times_X_Y_0_8(o_WoxrXtYt_1_0_8),
	.o_W_times_X_Y_0_9(o_WoxrXtYt_1_0_9),
	.o_W_times_X_Y_0_10(o_WoxrXtYt_1_0_10),
	.o_W_times_X_Y_0_11(o_WoxrXtYt_1_0_11),
	.o_W_times_X_Y_0_12(o_WoxrXtYt_1_0_12),
	.o_W_times_X_Y_0_13(o_WoxrXtYt_1_0_13),
	.o_W_times_X_Y_0_14(o_WoxrXtYt_1_0_14),
	.o_W_times_X_Y_0_15(o_WoxrXtYt_1_0_15),
	.o_valid(output_gate_mult_valid),
	.o_ready(output_gate_mult_ready)
);

// Output Activation Multiplication 
matrix_times_two_vectors_18_10_1_672_16_1 output_act_gate_mult (
	.clk(clk),
	.reset(reset),
	.enable(enable),
	.i_ready(i_ready),
	.i_valid(i_valid),
	.i_Xt_Yt_1_0(i_Xt_Yt_1_0),
	.i_Xt_Yt_1_1(i_Xt_Yt_1_1),
	.i_Xt_Yt_1_2(i_Xt_Yt_1_2),
	.i_Xt_Yt_1_3(i_Xt_Yt_1_3),
	.i_Xt_Yt_1_4(i_Xt_Yt_1_4),
	.i_Xt_Yt_1_5(i_Xt_Yt_1_5),
	.i_Xt_Yt_1_6(i_Xt_Yt_1_6),
	.i_Xt_Yt_1_7(i_Xt_Yt_1_7),
	.i_Xt_Yt_1_8(i_Xt_Yt_1_8),
	.i_Xt_Yt_1_9(i_Xt_Yt_1_9),
	.i_Xt_Yt_1_10(i_Xt_Yt_1_10),
	.i_Xt_Yt_1_11(i_Xt_Yt_1_11),
	.i_Xt_Yt_1_12(i_Xt_Yt_1_12),
	.i_Xt_Yt_1_13(i_Xt_Yt_1_13),
	.i_Xt_Yt_1_14(i_Xt_Yt_1_14),
	.i_Xt_Yt_1_15(i_Xt_Yt_1_15),
	.i_Wxr_real_0_0(i_Wcxr_real_0_0),
	.i_Wxr_imag_0_0(i_Wcxr_imag_0_0),
	.i_Wxr_real_0_1(i_Wcxr_real_0_1),
	.i_Wxr_imag_0_1(i_Wcxr_imag_0_1),
	.i_Wxr_real_0_2(i_Wcxr_real_0_2),
	.i_Wxr_imag_0_2(i_Wcxr_imag_0_2),
	.i_Wxr_real_0_3(i_Wcxr_real_0_3),
	.i_Wxr_imag_0_3(i_Wcxr_imag_0_3),
	.i_Wxr_real_0_4(i_Wcxr_real_0_4),
	.i_Wxr_imag_0_4(i_Wcxr_imag_0_4),
	.i_Wxr_real_0_5(i_Wcxr_real_0_5),
	.i_Wxr_imag_0_5(i_Wcxr_imag_0_5),
	.i_Wxr_real_0_6(i_Wcxr_real_0_6),
	.i_Wxr_imag_0_6(i_Wcxr_imag_0_6),
	.i_Wxr_real_0_7(i_Wcxr_real_0_7),
	.i_Wxr_imag_0_7(i_Wcxr_imag_0_7),
	.i_Wxr_real_0_8(i_Wcxr_real_0_8),
	.i_Wxr_imag_0_8(i_Wcxr_imag_0_8),
	.o_W_times_X_Y_0_0(o_WcxrXtYt_1_0_0),
	.o_W_times_X_Y_0_1(o_WcxrXtYt_1_0_1),
	.o_W_times_X_Y_0_2(o_WcxrXtYt_1_0_2),
	.o_W_times_X_Y_0_3(o_WcxrXtYt_1_0_3),
	.o_W_times_X_Y_0_4(o_WcxrXtYt_1_0_4),
	.o_W_times_X_Y_0_5(o_WcxrXtYt_1_0_5),
	.o_W_times_X_Y_0_6(o_WcxrXtYt_1_0_6),
	.o_W_times_X_Y_0_7(o_WcxrXtYt_1_0_7),
	.o_W_times_X_Y_0_8(o_WcxrXtYt_1_0_8),
	.o_W_times_X_Y_0_9(o_WcxrXtYt_1_0_9),
	.o_W_times_X_Y_0_10(o_WcxrXtYt_1_0_10),
	.o_W_times_X_Y_0_11(o_WcxrXtYt_1_0_11),
	.o_W_times_X_Y_0_12(o_WcxrXtYt_1_0_12),
	.o_W_times_X_Y_0_13(o_WcxrXtYt_1_0_13),
	.o_W_times_X_Y_0_14(o_WcxrXtYt_1_0_14),
	.o_W_times_X_Y_0_15(o_WcxrXtYt_1_0_15),
	.o_valid(output_act_mult_valid),
	.o_ready(output_act_mult_ready)
);

assign o_valid = input_gate_mult_valid & forget_gate_mult_valid & output_gate_mult_valid & output_act_mult_valid;
assign o_ready = input_gate_mult_ready & forget_gate_mult_ready & output_gate_mult_ready & output_act_mult_ready;

endmodule

module matrix_times_two_vectors_18_10_1_672_16_1 (
	input clk,
	input reset,
	input enable,
	input i_ready,
	input i_valid,
	input [17:0] i_Xt_Yt_1_0,
	input [17:0] i_Xt_Yt_1_1,
	input [17:0] i_Xt_Yt_1_2,
	input [17:0] i_Xt_Yt_1_3,
	input [17:0] i_Xt_Yt_1_4,
	input [17:0] i_Xt_Yt_1_5,
	input [17:0] i_Xt_Yt_1_6,
	input [17:0] i_Xt_Yt_1_7,
	input [17:0] i_Xt_Yt_1_8,
	input [17:0] i_Xt_Yt_1_9,
	input [17:0] i_Xt_Yt_1_10,
	input [17:0] i_Xt_Yt_1_11,
	input [17:0] i_Xt_Yt_1_12,
	input [17:0] i_Xt_Yt_1_13,
	input [17:0] i_Xt_Yt_1_14,
	input [17:0] i_Xt_Yt_1_15,
	input [17:0] i_Wxr_real_0_0,
	input [17:0] i_Wxr_imag_0_0,
	input [17:0] i_Wxr_real_0_1,
	input [17:0] i_Wxr_imag_0_1,
	input [17:0] i_Wxr_real_0_2,
	input [17:0] i_Wxr_imag_0_2,
	input [17:0] i_Wxr_real_0_3,
	input [17:0] i_Wxr_imag_0_3,
	input [17:0] i_Wxr_real_0_4,
	input [17:0] i_Wxr_imag_0_4,
	input [17:0] i_Wxr_real_0_5,
	input [17:0] i_Wxr_imag_0_5,
	input [17:0] i_Wxr_real_0_6,
	input [17:0] i_Wxr_imag_0_6,
	input [17:0] i_Wxr_real_0_7,
	input [17:0] i_Wxr_imag_0_7,
	input [17:0] i_Wxr_real_0_8,
	input [17:0] i_Wxr_imag_0_8,
	output [17:0] o_W_times_X_Y_0_0,
	output [17:0] o_W_times_X_Y_0_1,
	output [17:0] o_W_times_X_Y_0_2,
	output [17:0] o_W_times_X_Y_0_3,
	output [17:0] o_W_times_X_Y_0_4,
	output [17:0] o_W_times_X_Y_0_5,
	output [17:0] o_W_times_X_Y_0_6,
	output [17:0] o_W_times_X_Y_0_7,
	output [17:0] o_W_times_X_Y_0_8,
	output [17:0] o_W_times_X_Y_0_9,
	output [17:0] o_W_times_X_Y_0_10,
	output [17:0] o_W_times_X_Y_0_11,
	output [17:0] o_W_times_X_Y_0_12,
	output [17:0] o_W_times_X_Y_0_13,
	output [17:0] o_W_times_X_Y_0_14,
	output [17:0] o_W_times_X_Y_0_15,
	output o_valid,
	output o_ready
);

multiple_c_matrix_vec_mult_and_sum_18_10_16_1_1_42 multiple_c_matrix_vec_mult_and_sum_18_10_16_1_1_42_inst (
	.clk(clk),
	.reset(reset),
	.i_ready(i_ready),
	.i_valid(i_valid),
	.i_X_0(i_Xt_Yt_1_0),
	.i_X_1(i_Xt_Yt_1_1),
	.i_X_2(i_Xt_Yt_1_2),
	.i_X_3(i_Xt_Yt_1_3),
	.i_X_4(i_Xt_Yt_1_4),
	.i_X_5(i_Xt_Yt_1_5),
	.i_X_6(i_Xt_Yt_1_6),
	.i_X_7(i_Xt_Yt_1_7),
	.i_X_8(i_Xt_Yt_1_8),
	.i_X_9(i_Xt_Yt_1_9),
	.i_X_10(i_Xt_Yt_1_10),
	.i_X_11(i_Xt_Yt_1_11),
	.i_X_12(i_Xt_Yt_1_12),
	.i_X_13(i_Xt_Yt_1_13),
	.i_X_14(i_Xt_Yt_1_14),
	.i_X_15(i_Xt_Yt_1_15),
	.i_W_real_0_0(i_Wxr_real_0_0),
	.i_W_imag_0_0(i_Wxr_imag_0_0),
	.i_W_real_0_1(i_Wxr_real_0_1),
	.i_W_imag_0_1(i_Wxr_imag_0_1),
	.i_W_real_0_2(i_Wxr_real_0_2),
	.i_W_imag_0_2(i_Wxr_imag_0_2),
	.i_W_real_0_3(i_Wxr_real_0_3),
	.i_W_imag_0_3(i_Wxr_imag_0_3),
	.i_W_real_0_4(i_Wxr_real_0_4),
	.i_W_imag_0_4(i_Wxr_imag_0_4),
	.i_W_real_0_5(i_Wxr_real_0_5),
	.i_W_imag_0_5(i_Wxr_imag_0_5),
	.i_W_real_0_6(i_Wxr_real_0_6),
	.i_W_imag_0_6(i_Wxr_imag_0_6),
	.i_W_real_0_7(i_Wxr_real_0_7),
	.i_W_imag_0_7(i_Wxr_imag_0_7),
	.i_W_real_0_8(i_Wxr_real_0_8),
	.i_W_imag_0_8(i_Wxr_imag_0_8),
	.o_Y_0_0(o_W_times_X_Y_0_0),
	.o_Y_0_1(o_W_times_X_Y_0_1),
	.o_Y_0_2(o_W_times_X_Y_0_2),
	.o_Y_0_3(o_W_times_X_Y_0_3),
	.o_Y_0_4(o_W_times_X_Y_0_4),
	.o_Y_0_5(o_W_times_X_Y_0_5),
	.o_Y_0_6(o_W_times_X_Y_0_6),
	.o_Y_0_7(o_W_times_X_Y_0_7),
	.o_Y_0_8(o_W_times_X_Y_0_8),
	.o_Y_0_9(o_W_times_X_Y_0_9),
	.o_Y_0_10(o_W_times_X_Y_0_10),
	.o_Y_0_11(o_W_times_X_Y_0_11),
	.o_Y_0_12(o_W_times_X_Y_0_12),
	.o_Y_0_13(o_W_times_X_Y_0_13),
	.o_Y_0_14(o_W_times_X_Y_0_14),
	.o_Y_0_15(o_W_times_X_Y_0_15),
	.o_valid(o_valid),
	.o_ready(o_ready)
);

endmodule

module multiple_c_matrix_vec_mult_and_sum_18_10_16_1_1_42 (
	input clk,
	input reset,
	input i_ready,
	input i_valid,
	input [17:0] i_X_0,
	input [17:0] i_X_1,
	input [17:0] i_X_2,
	input [17:0] i_X_3,
	input [17:0] i_X_4,
	input [17:0] i_X_5,
	input [17:0] i_X_6,
	input [17:0] i_X_7,
	input [17:0] i_X_8,
	input [17:0] i_X_9,
	input [17:0] i_X_10,
	input [17:0] i_X_11,
	input [17:0] i_X_12,
	input [17:0] i_X_13,
	input [17:0] i_X_14,
	input [17:0] i_X_15,
	input [17:0] i_W_real_0_0,
	input [17:0] i_W_imag_0_0,
	input [17:0] i_W_real_0_1,
	input [17:0] i_W_imag_0_1,
	input [17:0] i_W_real_0_2,
	input [17:0] i_W_imag_0_2,
	input [17:0] i_W_real_0_3,
	input [17:0] i_W_imag_0_3,
	input [17:0] i_W_real_0_4,
	input [17:0] i_W_imag_0_4,
	input [17:0] i_W_real_0_5,
	input [17:0] i_W_imag_0_5,
	input [17:0] i_W_real_0_6,
	input [17:0] i_W_imag_0_6,
	input [17:0] i_W_real_0_7,
	input [17:0] i_W_imag_0_7,
	input [17:0] i_W_real_0_8,
	input [17:0] i_W_imag_0_8,
	output [17:0] o_Y_0_0,
	output [17:0] o_Y_0_1,
	output [17:0] o_Y_0_2,
	output [17:0] o_Y_0_3,
	output [17:0] o_Y_0_4,
	output [17:0] o_Y_0_5,
	output [17:0] o_Y_0_6,
	output [17:0] o_Y_0_7,
	output [17:0] o_Y_0_8,
	output [17:0] o_Y_0_9,
	output [17:0] o_Y_0_10,
	output [17:0] o_Y_0_11,
	output [17:0] o_Y_0_12,
	output [17:0] o_Y_0_13,
	output [17:0] o_Y_0_14,
	output [17:0] o_Y_0_15,
	output o_valid,
	output o_ready
);

wire matrix_vec_mult_ready, matrix_vec_mult_valid;
wire accum_valid_0;
wire idft_next_out_0;
reg idft_out_valid;
wire [17:0] Y_imag_0_0;
wire [17:0] Y_real_0_0;
wire [17:0] sum_Y_real_0_0;
wire [17:0] sum_Y_imag_0_0;
wire [17:0] sum_Y_real_hold_0_0;
wire [17:0] sum_Y_imag_hold_0_0;
wire [17:0] out_Y_idft_0_0;
reg [17:0] reg_Y_0_0;
wire [17:0] Y_imag_0_1;
wire [17:0] Y_real_0_1;
wire [17:0] sum_Y_real_0_1;
wire [17:0] sum_Y_imag_0_1;
wire [17:0] sum_Y_real_hold_0_1;
wire [17:0] sum_Y_imag_hold_0_1;
wire [17:0] out_Y_idft_0_1;
reg [17:0] reg_Y_0_1;
wire [17:0] Y_imag_0_2;
wire [17:0] Y_real_0_2;
wire [17:0] sum_Y_real_0_2;
wire [17:0] sum_Y_imag_0_2;
wire [17:0] sum_Y_real_hold_0_2;
wire [17:0] sum_Y_imag_hold_0_2;
wire [17:0] out_Y_idft_0_2;
reg [17:0] reg_Y_0_2;
wire [17:0] Y_imag_0_3;
wire [17:0] Y_real_0_3;
wire [17:0] sum_Y_real_0_3;
wire [17:0] sum_Y_imag_0_3;
wire [17:0] sum_Y_real_hold_0_3;
wire [17:0] sum_Y_imag_hold_0_3;
wire [17:0] out_Y_idft_0_3;
reg [17:0] reg_Y_0_3;
wire [17:0] Y_imag_0_4;
wire [17:0] Y_real_0_4;
wire [17:0] sum_Y_real_0_4;
wire [17:0] sum_Y_imag_0_4;
wire [17:0] sum_Y_real_hold_0_4;
wire [17:0] sum_Y_imag_hold_0_4;
wire [17:0] out_Y_idft_0_4;
reg [17:0] reg_Y_0_4;
wire [17:0] Y_imag_0_5;
wire [17:0] Y_real_0_5;
wire [17:0] sum_Y_real_0_5;
wire [17:0] sum_Y_imag_0_5;
wire [17:0] sum_Y_real_hold_0_5;
wire [17:0] sum_Y_imag_hold_0_5;
wire [17:0] out_Y_idft_0_5;
reg [17:0] reg_Y_0_5;
wire [17:0] Y_imag_0_6;
wire [17:0] Y_real_0_6;
wire [17:0] sum_Y_real_0_6;
wire [17:0] sum_Y_imag_0_6;
wire [17:0] sum_Y_real_hold_0_6;
wire [17:0] sum_Y_imag_hold_0_6;
wire [17:0] out_Y_idft_0_6;
reg [17:0] reg_Y_0_6;
wire [17:0] Y_imag_0_7;
wire [17:0] Y_real_0_7;
wire [17:0] sum_Y_real_0_7;
wire [17:0] sum_Y_imag_0_7;
wire [17:0] sum_Y_real_hold_0_7;
wire [17:0] sum_Y_imag_hold_0_7;
wire [17:0] out_Y_idft_0_7;
reg [17:0] reg_Y_0_7;
wire [17:0] Y_imag_0_8;
wire [17:0] Y_real_0_8;
wire [17:0] sum_Y_real_0_8;
wire [17:0] sum_Y_imag_0_8;
wire [17:0] sum_Y_real_hold_0_8;
wire [17:0] sum_Y_imag_hold_0_8;
wire [17:0] out_Y_idft_0_8;
reg [17:0] reg_Y_0_8;
wire [17:0] Y_imag_0_9;
wire [17:0] Y_real_0_9;
wire [17:0] sum_Y_real_0_9;
wire [17:0] sum_Y_imag_0_9;
wire [17:0] sum_Y_real_hold_0_9;
wire [17:0] sum_Y_imag_hold_0_9;
wire [17:0] out_Y_idft_0_9;
reg [17:0] reg_Y_0_9;
wire [17:0] Y_imag_0_10;
wire [17:0] Y_real_0_10;
wire [17:0] sum_Y_real_0_10;
wire [17:0] sum_Y_imag_0_10;
wire [17:0] sum_Y_real_hold_0_10;
wire [17:0] sum_Y_imag_hold_0_10;
wire [17:0] out_Y_idft_0_10;
reg [17:0] reg_Y_0_10;
wire [17:0] Y_imag_0_11;
wire [17:0] Y_real_0_11;
wire [17:0] sum_Y_real_0_11;
wire [17:0] sum_Y_imag_0_11;
wire [17:0] sum_Y_real_hold_0_11;
wire [17:0] sum_Y_imag_hold_0_11;
wire [17:0] out_Y_idft_0_11;
reg [17:0] reg_Y_0_11;
wire [17:0] Y_imag_0_12;
wire [17:0] Y_real_0_12;
wire [17:0] sum_Y_real_0_12;
wire [17:0] sum_Y_imag_0_12;
wire [17:0] sum_Y_real_hold_0_12;
wire [17:0] sum_Y_imag_hold_0_12;
wire [17:0] out_Y_idft_0_12;
reg [17:0] reg_Y_0_12;
wire [17:0] Y_imag_0_13;
wire [17:0] Y_real_0_13;
wire [17:0] sum_Y_real_0_13;
wire [17:0] sum_Y_imag_0_13;
wire [17:0] sum_Y_real_hold_0_13;
wire [17:0] sum_Y_imag_hold_0_13;
wire [17:0] out_Y_idft_0_13;
reg [17:0] reg_Y_0_13;
wire [17:0] Y_imag_0_14;
wire [17:0] Y_real_0_14;
wire [17:0] sum_Y_real_0_14;
wire [17:0] sum_Y_imag_0_14;
wire [17:0] sum_Y_real_hold_0_14;
wire [17:0] sum_Y_imag_hold_0_14;
wire [17:0] out_Y_idft_0_14;
reg [17:0] reg_Y_0_14;
wire [17:0] Y_imag_0_15;
wire [17:0] Y_real_0_15;
wire [17:0] sum_Y_real_0_15;
wire [17:0] sum_Y_imag_0_15;
wire [17:0] sum_Y_real_hold_0_15;
wire [17:0] sum_Y_imag_hold_0_15;
wire [17:0] out_Y_idft_0_15;
reg [17:0] reg_Y_0_15;
reg reg_o_valid;

// Enable whenever the reciever is ready
wire enable;
assign enable = i_ready;
c_matrix_vec_mult_core_18_10_16_1_1 c_matrix_vec_mult_core_18_10_16_1_1_inst (
	.clk(clk),
	.reset(reset),
	.i_ready(i_ready),
	.i_valid(i_valid),
	.i_X_0(i_X_0),
	.i_X_1(i_X_1),
	.i_X_2(i_X_2),
	.i_X_3(i_X_3),
	.i_X_4(i_X_4),
	.i_X_5(i_X_5),
	.i_X_6(i_X_6),
	.i_X_7(i_X_7),
	.i_X_8(i_X_8),
	.i_X_9(i_X_9),
	.i_X_10(i_X_10),
	.i_X_11(i_X_11),
	.i_X_12(i_X_12),
	.i_X_13(i_X_13),
	.i_X_14(i_X_14),
	.i_X_15(i_X_15),
	.i_W_real_0_0(i_W_real_0_0),
	.i_W_imag_0_0(i_W_imag_0_0),
	.i_W_real_0_1(i_W_real_0_1),
	.i_W_imag_0_1(i_W_imag_0_1),
	.i_W_real_0_2(i_W_real_0_2),
	.i_W_imag_0_2(i_W_imag_0_2),
	.i_W_real_0_3(i_W_real_0_3),
	.i_W_imag_0_3(i_W_imag_0_3),
	.i_W_real_0_4(i_W_real_0_4),
	.i_W_imag_0_4(i_W_imag_0_4),
	.i_W_real_0_5(i_W_real_0_5),
	.i_W_imag_0_5(i_W_imag_0_5),
	.i_W_real_0_6(i_W_real_0_6),
	.i_W_imag_0_6(i_W_imag_0_6),
	.i_W_real_0_7(i_W_real_0_7),
	.i_W_imag_0_7(i_W_imag_0_7),
	.i_W_real_0_8(i_W_real_0_8),
	.i_W_imag_0_8(i_W_imag_0_8),
	.o_Y_real_0_0(Y_real_0_0),
	.o_Y_imag_0_0(Y_imag_0_0),
	.o_Y_real_0_1(Y_real_0_1),
	.o_Y_imag_0_1(Y_imag_0_1),
	.o_Y_real_0_2(Y_real_0_2),
	.o_Y_imag_0_2(Y_imag_0_2),
	.o_Y_real_0_3(Y_real_0_3),
	.o_Y_imag_0_3(Y_imag_0_3),
	.o_Y_real_0_4(Y_real_0_4),
	.o_Y_imag_0_4(Y_imag_0_4),
	.o_Y_real_0_5(Y_real_0_5),
	.o_Y_imag_0_5(Y_imag_0_5),
	.o_Y_real_0_6(Y_real_0_6),
	.o_Y_imag_0_6(Y_imag_0_6),
	.o_Y_real_0_7(Y_real_0_7),
	.o_Y_imag_0_7(Y_imag_0_7),
	.o_Y_real_0_8(Y_real_0_8),
	.o_Y_imag_0_8(Y_imag_0_8),
	.o_Y_real_0_9(Y_real_0_9),
	.o_Y_imag_0_9(Y_imag_0_9),
	.o_Y_real_0_10(Y_real_0_10),
	.o_Y_imag_0_10(Y_imag_0_10),
	.o_Y_real_0_11(Y_real_0_11),
	.o_Y_imag_0_11(Y_imag_0_11),
	.o_Y_real_0_12(Y_real_0_12),
	.o_Y_imag_0_12(Y_imag_0_12),
	.o_Y_real_0_13(Y_real_0_13),
	.o_Y_imag_0_13(Y_imag_0_13),
	.o_Y_real_0_14(Y_real_0_14),
	.o_Y_imag_0_14(Y_imag_0_14),
	.o_Y_real_0_15(Y_real_0_15),
	.o_Y_imag_0_15(Y_imag_0_15),
	.o_ready(matrix_vec_mult_ready),
	.o_valid(matrix_vec_mult_valid)
);

sum_complex_vector_unit_18_18_16_42 sum_complex_vector_unit_18_18_16_42_inst_0 (
	.clk(clk),
	.clr(reset),
	.enable(enable),
	.i_valid(matrix_vec_mult_valid),
	.i_real_0(Y_real_0_0),
	.i_imag_0(Y_imag_0_0),
	.o_real_0(sum_Y_real_0_0),
	.o_imag_0(sum_Y_imag_0_0),
	.i_real_1(Y_real_0_1),
	.i_imag_1(Y_imag_0_1),
	.o_real_1(sum_Y_real_0_1),
	.o_imag_1(sum_Y_imag_0_1),
	.i_real_2(Y_real_0_2),
	.i_imag_2(Y_imag_0_2),
	.o_real_2(sum_Y_real_0_2),
	.o_imag_2(sum_Y_imag_0_2),
	.i_real_3(Y_real_0_3),
	.i_imag_3(Y_imag_0_3),
	.o_real_3(sum_Y_real_0_3),
	.o_imag_3(sum_Y_imag_0_3),
	.i_real_4(Y_real_0_4),
	.i_imag_4(Y_imag_0_4),
	.o_real_4(sum_Y_real_0_4),
	.o_imag_4(sum_Y_imag_0_4),
	.i_real_5(Y_real_0_5),
	.i_imag_5(Y_imag_0_5),
	.o_real_5(sum_Y_real_0_5),
	.o_imag_5(sum_Y_imag_0_5),
	.i_real_6(Y_real_0_6),
	.i_imag_6(Y_imag_0_6),
	.o_real_6(sum_Y_real_0_6),
	.o_imag_6(sum_Y_imag_0_6),
	.i_real_7(Y_real_0_7),
	.i_imag_7(Y_imag_0_7),
	.o_real_7(sum_Y_real_0_7),
	.o_imag_7(sum_Y_imag_0_7),
	.i_real_8(Y_real_0_8),
	.i_imag_8(Y_imag_0_8),
	.o_real_8(sum_Y_real_0_8),
	.o_imag_8(sum_Y_imag_0_8),
	.i_real_9(Y_real_0_9),
	.i_imag_9(Y_imag_0_9),
	.o_real_9(sum_Y_real_0_9),
	.o_imag_9(sum_Y_imag_0_9),
	.i_real_10(Y_real_0_10),
	.i_imag_10(Y_imag_0_10),
	.o_real_10(sum_Y_real_0_10),
	.o_imag_10(sum_Y_imag_0_10),
	.i_real_11(Y_real_0_11),
	.i_imag_11(Y_imag_0_11),
	.o_real_11(sum_Y_real_0_11),
	.o_imag_11(sum_Y_imag_0_11),
	.i_real_12(Y_real_0_12),
	.i_imag_12(Y_imag_0_12),
	.o_real_12(sum_Y_real_0_12),
	.o_imag_12(sum_Y_imag_0_12),
	.i_real_13(Y_real_0_13),
	.i_imag_13(Y_imag_0_13),
	.o_real_13(sum_Y_real_0_13),
	.o_imag_13(sum_Y_imag_0_13),
	.i_real_14(Y_real_0_14),
	.i_imag_14(Y_imag_0_14),
	.o_real_14(sum_Y_real_0_14),
	.o_imag_14(sum_Y_imag_0_14),
	.i_real_15(Y_real_0_15),
	.i_imag_15(Y_imag_0_15),
	.o_real_15(sum_Y_real_0_15),
	.o_imag_15(sum_Y_imag_0_15),
	.o_valid(accum_valid_0)
);

shift_register_group_18_16_1 shift_register_group_18_16_1_inst_real_0 (
	.clk(clk),
	.enable(enable),
	.in_0(sum_Y_real_0_0),
	.out_0(sum_Y_real_hold_0_0),
	.in_1(sum_Y_real_0_1),
	.out_1(sum_Y_real_hold_0_1),
	.in_2(sum_Y_real_0_2),
	.out_2(sum_Y_real_hold_0_2),
	.in_3(sum_Y_real_0_3),
	.out_3(sum_Y_real_hold_0_3),
	.in_4(sum_Y_real_0_4),
	.out_4(sum_Y_real_hold_0_4),
	.in_5(sum_Y_real_0_5),
	.out_5(sum_Y_real_hold_0_5),
	.in_6(sum_Y_real_0_6),
	.out_6(sum_Y_real_hold_0_6),
	.in_7(sum_Y_real_0_7),
	.out_7(sum_Y_real_hold_0_7),
	.in_8(sum_Y_real_0_8),
	.out_8(sum_Y_real_hold_0_8),
	.in_9(sum_Y_real_0_9),
	.out_9(sum_Y_real_hold_0_9),
	.in_10(sum_Y_real_0_10),
	.out_10(sum_Y_real_hold_0_10),
	.in_11(sum_Y_real_0_11),
	.out_11(sum_Y_real_hold_0_11),
	.in_12(sum_Y_real_0_12),
	.out_12(sum_Y_real_hold_0_12),
	.in_13(sum_Y_real_0_13),
	.out_13(sum_Y_real_hold_0_13),
	.in_14(sum_Y_real_0_14),
	.out_14(sum_Y_real_hold_0_14),
	.in_15(sum_Y_real_0_15),
	.out_15(sum_Y_real_hold_0_15),
	.reset(reset)
);

shift_register_group_18_16_1 shift_register_group_18_16_1_inst_imag_0 (
	.clk(clk),
	.enable(enable),
	.in_0(sum_Y_imag_0_0),
	.out_0(sum_Y_imag_hold_0_0),
	.in_1(sum_Y_imag_0_1),
	.out_1(sum_Y_imag_hold_0_1),
	.in_2(sum_Y_imag_0_2),
	.out_2(sum_Y_imag_hold_0_2),
	.in_3(sum_Y_imag_0_3),
	.out_3(sum_Y_imag_hold_0_3),
	.in_4(sum_Y_imag_0_4),
	.out_4(sum_Y_imag_hold_0_4),
	.in_5(sum_Y_imag_0_5),
	.out_5(sum_Y_imag_hold_0_5),
	.in_6(sum_Y_imag_0_6),
	.out_6(sum_Y_imag_hold_0_6),
	.in_7(sum_Y_imag_0_7),
	.out_7(sum_Y_imag_hold_0_7),
	.in_8(sum_Y_imag_0_8),
	.out_8(sum_Y_imag_hold_0_8),
	.in_9(sum_Y_imag_0_9),
	.out_9(sum_Y_imag_hold_0_9),
	.in_10(sum_Y_imag_0_10),
	.out_10(sum_Y_imag_hold_0_10),
	.in_11(sum_Y_imag_0_11),
	.out_11(sum_Y_imag_hold_0_11),
	.in_12(sum_Y_imag_0_12),
	.out_12(sum_Y_imag_hold_0_12),
	.in_13(sum_Y_imag_0_13),
	.out_13(sum_Y_imag_hold_0_13),
	.in_14(sum_Y_imag_0_14),
	.out_14(sum_Y_imag_hold_0_14),
	.in_15(sum_Y_imag_0_15),
	.out_15(sum_Y_imag_hold_0_15),
	.reset(reset)
);

idft_16_top_18 idft_16_top_18_inst_0 (
	.clk(clk),
	.reset(reset),
	.next(accum_valid_0),
	.X0(sum_Y_real_hold_0_0),
	.Y0(out_Y_idft_0_0),
	.X1(sum_Y_imag_hold_0_0),
	.Y1(),
	.X2(sum_Y_real_hold_0_1),
	.Y2(out_Y_idft_0_1),
	.X3(sum_Y_imag_hold_0_1),
	.Y3(),
	.X4(sum_Y_real_hold_0_2),
	.Y4(out_Y_idft_0_2),
	.X5(sum_Y_imag_hold_0_2),
	.Y5(),
	.X6(sum_Y_real_hold_0_3),
	.Y6(out_Y_idft_0_3),
	.X7(sum_Y_imag_hold_0_3),
	.Y7(),
	.X8(sum_Y_real_hold_0_4),
	.Y8(out_Y_idft_0_4),
	.X9(sum_Y_imag_hold_0_4),
	.Y9(),
	.X10(sum_Y_real_hold_0_5),
	.Y10(out_Y_idft_0_5),
	.X11(sum_Y_imag_hold_0_5),
	.Y11(),
	.X12(sum_Y_real_hold_0_6),
	.Y12(out_Y_idft_0_6),
	.X13(sum_Y_imag_hold_0_6),
	.Y13(),
	.X14(sum_Y_real_hold_0_7),
	.Y14(out_Y_idft_0_7),
	.X15(sum_Y_imag_hold_0_7),
	.Y15(),
	.X16(sum_Y_real_hold_0_8),
	.Y16(out_Y_idft_0_8),
	.X17(sum_Y_imag_hold_0_8),
	.Y17(),
	.X18(sum_Y_real_hold_0_9),
	.Y18(out_Y_idft_0_9),
	.X19(sum_Y_imag_hold_0_9),
	.Y19(),
	.X20(sum_Y_real_hold_0_10),
	.Y20(out_Y_idft_0_10),
	.X21(sum_Y_imag_hold_0_10),
	.Y21(),
	.X22(sum_Y_real_hold_0_11),
	.Y22(out_Y_idft_0_11),
	.X23(sum_Y_imag_hold_0_11),
	.Y23(),
	.X24(sum_Y_real_hold_0_12),
	.Y24(out_Y_idft_0_12),
	.X25(sum_Y_imag_hold_0_12),
	.Y25(),
	.X26(sum_Y_real_hold_0_13),
	.Y26(out_Y_idft_0_13),
	.X27(sum_Y_imag_hold_0_13),
	.Y27(),
	.X28(sum_Y_real_hold_0_14),
	.Y28(out_Y_idft_0_14),
	.X29(sum_Y_imag_hold_0_14),
	.Y29(),
	.X30(sum_Y_real_hold_0_15),
	.Y30(out_Y_idft_0_15),
	.X31(sum_Y_imag_hold_0_15),
	.Y31(),
	.next_out(idft_next_out_0)
);

always @ (posedge clk) begin
	if (reset) begin
		reg_Y_0_0 <= 0;
		reg_Y_0_1 <= 0;
		reg_Y_0_2 <= 0;
		reg_Y_0_3 <= 0;
		reg_Y_0_4 <= 0;
		reg_Y_0_5 <= 0;
		reg_Y_0_6 <= 0;
		reg_Y_0_7 <= 0;
		reg_Y_0_8 <= 0;
		reg_Y_0_9 <= 0;
		reg_Y_0_10 <= 0;
		reg_Y_0_11 <= 0;
		reg_Y_0_12 <= 0;
		reg_Y_0_13 <= 0;
		reg_Y_0_14 <= 0;
		reg_Y_0_15 <= 0;
		idft_out_valid <= 1'b0;
		reg_o_valid <= 1'b0;
	end else if (enable) begin
		reg_Y_0_0 <= (out_Y_idft_0_0 >>> 4);
		reg_Y_0_1 <= (out_Y_idft_0_1 >>> 4);
		reg_Y_0_2 <= (out_Y_idft_0_2 >>> 4);
		reg_Y_0_3 <= (out_Y_idft_0_3 >>> 4);
		reg_Y_0_4 <= (out_Y_idft_0_4 >>> 4);
		reg_Y_0_5 <= (out_Y_idft_0_5 >>> 4);
		reg_Y_0_6 <= (out_Y_idft_0_6 >>> 4);
		reg_Y_0_7 <= (out_Y_idft_0_7 >>> 4);
		reg_Y_0_8 <= (out_Y_idft_0_8 >>> 4);
		reg_Y_0_9 <= (out_Y_idft_0_9 >>> 4);
		reg_Y_0_10 <= (out_Y_idft_0_10 >>> 4);
		reg_Y_0_11 <= (out_Y_idft_0_11 >>> 4);
		reg_Y_0_12 <= (out_Y_idft_0_12 >>> 4);
		reg_Y_0_13 <= (out_Y_idft_0_13 >>> 4);
		reg_Y_0_14 <= (out_Y_idft_0_14 >>> 4);
		reg_Y_0_15 <= (out_Y_idft_0_15 >>> 4);
		idft_out_valid <= idft_next_out_0;
		reg_o_valid <= idft_out_valid;
	end
end

assign o_valid = enable & reg_o_valid;
assign o_ready = matrix_vec_mult_ready;
assign o_Y_0_0 = reg_Y_0_0;
assign o_Y_0_1 = reg_Y_0_1;
assign o_Y_0_2 = reg_Y_0_2;
assign o_Y_0_3 = reg_Y_0_3;
assign o_Y_0_4 = reg_Y_0_4;
assign o_Y_0_5 = reg_Y_0_5;
assign o_Y_0_6 = reg_Y_0_6;
assign o_Y_0_7 = reg_Y_0_7;
assign o_Y_0_8 = reg_Y_0_8;
assign o_Y_0_9 = reg_Y_0_9;
assign o_Y_0_10 = reg_Y_0_10;
assign o_Y_0_11 = reg_Y_0_11;
assign o_Y_0_12 = reg_Y_0_12;
assign o_Y_0_13 = reg_Y_0_13;
assign o_Y_0_14 = reg_Y_0_14;
assign o_Y_0_15 = reg_Y_0_15;

endmodule

module sum_complex_vector_unit_18_18_16_42 (
	input clk,
	input clr,
	input i_valid,
	input enable,
	input [17:0] i_real_0,
	input [17:0] i_imag_0,
	output [17:0] o_real_0,
	output [17:0] o_imag_0,
	input [17:0] i_real_1,
	input [17:0] i_imag_1,
	output [17:0] o_real_1,
	output [17:0] o_imag_1,
	input [17:0] i_real_2,
	input [17:0] i_imag_2,
	output [17:0] o_real_2,
	output [17:0] o_imag_2,
	input [17:0] i_real_3,
	input [17:0] i_imag_3,
	output [17:0] o_real_3,
	output [17:0] o_imag_3,
	input [17:0] i_real_4,
	input [17:0] i_imag_4,
	output [17:0] o_real_4,
	output [17:0] o_imag_4,
	input [17:0] i_real_5,
	input [17:0] i_imag_5,
	output [17:0] o_real_5,
	output [17:0] o_imag_5,
	input [17:0] i_real_6,
	input [17:0] i_imag_6,
	output [17:0] o_real_6,
	output [17:0] o_imag_6,
	input [17:0] i_real_7,
	input [17:0] i_imag_7,
	output [17:0] o_real_7,
	output [17:0] o_imag_7,
	input [17:0] i_real_8,
	input [17:0] i_imag_8,
	output [17:0] o_real_8,
	output [17:0] o_imag_8,
	input [17:0] i_real_9,
	input [17:0] i_imag_9,
	output [17:0] o_real_9,
	output [17:0] o_imag_9,
	input [17:0] i_real_10,
	input [17:0] i_imag_10,
	output [17:0] o_real_10,
	output [17:0] o_imag_10,
	input [17:0] i_real_11,
	input [17:0] i_imag_11,
	output [17:0] o_real_11,
	output [17:0] o_imag_11,
	input [17:0] i_real_12,
	input [17:0] i_imag_12,
	output [17:0] o_real_12,
	output [17:0] o_imag_12,
	input [17:0] i_real_13,
	input [17:0] i_imag_13,
	output [17:0] o_real_13,
	output [17:0] o_imag_13,
	input [17:0] i_real_14,
	input [17:0] i_imag_14,
	output [17:0] o_real_14,
	output [17:0] o_imag_14,
	input [17:0] i_real_15,
	input [17:0] i_imag_15,
	output [17:0] o_real_15,
	output [17:0] o_imag_15,
	output o_valid
);

reg [17:0] sum_real_0;
reg [17:0] sum_imag_0;
reg [17:0] sum_real_1;
reg [17:0] sum_imag_1;
reg [17:0] sum_real_2;
reg [17:0] sum_imag_2;
reg [17:0] sum_real_3;
reg [17:0] sum_imag_3;
reg [17:0] sum_real_4;
reg [17:0] sum_imag_4;
reg [17:0] sum_real_5;
reg [17:0] sum_imag_5;
reg [17:0] sum_real_6;
reg [17:0] sum_imag_6;
reg [17:0] sum_real_7;
reg [17:0] sum_imag_7;
reg [17:0] sum_real_8;
reg [17:0] sum_imag_8;
reg [17:0] sum_real_9;
reg [17:0] sum_imag_9;
reg [17:0] sum_real_10;
reg [17:0] sum_imag_10;
reg [17:0] sum_real_11;
reg [17:0] sum_imag_11;
reg [17:0] sum_real_12;
reg [17:0] sum_imag_12;
reg [17:0] sum_real_13;
reg [17:0] sum_imag_13;
reg [17:0] sum_real_14;
reg [17:0] sum_imag_14;
reg [17:0] sum_real_15;
reg [17:0] sum_imag_15;
reg reg_i_valid;

// Count the number data in accumulation
reg [13:0] counter;
wire counter_full;
always @ (posedge clk) begin 
	if (clr) begin
		sum_real_0 <= 0;
		sum_imag_0 <= 0;
		sum_real_1 <= 0;
		sum_imag_1 <= 0;
		sum_real_2 <= 0;
		sum_imag_2 <= 0;
		sum_real_3 <= 0;
		sum_imag_3 <= 0;
		sum_real_4 <= 0;
		sum_imag_4 <= 0;
		sum_real_5 <= 0;
		sum_imag_5 <= 0;
		sum_real_6 <= 0;
		sum_imag_6 <= 0;
		sum_real_7 <= 0;
		sum_imag_7 <= 0;
		sum_real_8 <= 0;
		sum_imag_8 <= 0;
		sum_real_9 <= 0;
		sum_imag_9 <= 0;
		sum_real_10 <= 0;
		sum_imag_10 <= 0;
		sum_real_11 <= 0;
		sum_imag_11 <= 0;
		sum_real_12 <= 0;
		sum_imag_12 <= 0;
		sum_real_13 <= 0;
		sum_imag_13 <= 0;
		sum_real_14 <= 0;
		sum_imag_14 <= 0;
		sum_real_15 <= 0;
		sum_imag_15 <= 0;
		counter <= 14'd0;
		reg_i_valid <= 1'b0;
	end else if (enable) begin
		reg_i_valid <= i_valid;
		// Accumulate the number only when data is valid
		if (i_valid) begin
			if (counter == 42)
				counter <= 1;
			else
				counter <= counter + 1;

			if (counter == 42) begin
				sum_real_0 <= i_real_0;
				sum_imag_0 <= i_imag_0;
				sum_real_1 <= i_real_1;
				sum_imag_1 <= i_imag_1;
				sum_real_2 <= i_real_2;
				sum_imag_2 <= i_imag_2;
				sum_real_3 <= i_real_3;
				sum_imag_3 <= i_imag_3;
				sum_real_4 <= i_real_4;
				sum_imag_4 <= i_imag_4;
				sum_real_5 <= i_real_5;
				sum_imag_5 <= i_imag_5;
				sum_real_6 <= i_real_6;
				sum_imag_6 <= i_imag_6;
				sum_real_7 <= i_real_7;
				sum_imag_7 <= i_imag_7;
				sum_real_8 <= i_real_8;
				sum_imag_8 <= i_imag_8;
				sum_real_9 <= i_real_9;
				sum_imag_9 <= i_imag_9;
				sum_real_10 <= i_real_10;
				sum_imag_10 <= i_imag_10;
				sum_real_11 <= i_real_11;
				sum_imag_11 <= i_imag_11;
				sum_real_12 <= i_real_12;
				sum_imag_12 <= i_imag_12;
				sum_real_13 <= i_real_13;
				sum_imag_13 <= i_imag_13;
				sum_real_14 <= i_real_14;
				sum_imag_14 <= i_imag_14;
				sum_real_15 <= i_real_15;
				sum_imag_15 <= i_imag_15;
			end else begin
				sum_real_0 <= sum_real_0 + i_real_0;
				sum_imag_0 <= sum_imag_0 + i_imag_0;
				sum_real_1 <= sum_real_1 + i_real_1;
				sum_imag_1 <= sum_imag_1 + i_imag_1;
				sum_real_2 <= sum_real_2 + i_real_2;
				sum_imag_2 <= sum_imag_2 + i_imag_2;
				sum_real_3 <= sum_real_3 + i_real_3;
				sum_imag_3 <= sum_imag_3 + i_imag_3;
				sum_real_4 <= sum_real_4 + i_real_4;
				sum_imag_4 <= sum_imag_4 + i_imag_4;
				sum_real_5 <= sum_real_5 + i_real_5;
				sum_imag_5 <= sum_imag_5 + i_imag_5;
				sum_real_6 <= sum_real_6 + i_real_6;
				sum_imag_6 <= sum_imag_6 + i_imag_6;
				sum_real_7 <= sum_real_7 + i_real_7;
				sum_imag_7 <= sum_imag_7 + i_imag_7;
				sum_real_8 <= sum_real_8 + i_real_8;
				sum_imag_8 <= sum_imag_8 + i_imag_8;
				sum_real_9 <= sum_real_9 + i_real_9;
				sum_imag_9 <= sum_imag_9 + i_imag_9;
				sum_real_10 <= sum_real_10 + i_real_10;
				sum_imag_10 <= sum_imag_10 + i_imag_10;
				sum_real_11 <= sum_real_11 + i_real_11;
				sum_imag_11 <= sum_imag_11 + i_imag_11;
				sum_real_12 <= sum_real_12 + i_real_12;
				sum_imag_12 <= sum_imag_12 + i_imag_12;
				sum_real_13 <= sum_real_13 + i_real_13;
				sum_imag_13 <= sum_imag_13 + i_imag_13;
				sum_real_14 <= sum_real_14 + i_real_14;
				sum_imag_14 <= sum_imag_14 + i_imag_14;
				sum_real_15 <= sum_real_15 + i_real_15;
				sum_imag_15 <= sum_imag_15 + i_imag_15;
			end
		end
	end
end

assign counter_full = (counter == 42);
assign o_real_0 = sum_real_0;
assign o_imag_0 = sum_imag_0;
assign o_real_1 = sum_real_1;
assign o_imag_1 = sum_imag_1;
assign o_real_2 = sum_real_2;
assign o_imag_2 = sum_imag_2;
assign o_real_3 = sum_real_3;
assign o_imag_3 = sum_imag_3;
assign o_real_4 = sum_real_4;
assign o_imag_4 = sum_imag_4;
assign o_real_5 = sum_real_5;
assign o_imag_5 = sum_imag_5;
assign o_real_6 = sum_real_6;
assign o_imag_6 = sum_imag_6;
assign o_real_7 = sum_real_7;
assign o_imag_7 = sum_imag_7;
assign o_real_8 = sum_real_8;
assign o_imag_8 = sum_imag_8;
assign o_real_9 = sum_real_9;
assign o_imag_9 = sum_imag_9;
assign o_real_10 = sum_real_10;
assign o_imag_10 = sum_imag_10;
assign o_real_11 = sum_real_11;
assign o_imag_11 = sum_imag_11;
assign o_real_12 = sum_real_12;
assign o_imag_12 = sum_imag_12;
assign o_real_13 = sum_real_13;
assign o_imag_13 = sum_imag_13;
assign o_real_14 = sum_real_14;
assign o_imag_14 = sum_imag_14;
assign o_real_15 = sum_real_15;
assign o_imag_15 = sum_imag_15;
assign o_valid = counter_full & reg_i_valid;

endmodule

module stage2_mt_buffer_18_1_16_64_32 (
	input clk,
	input reset,
	input i_valid,
	input [17:0] data_0,
	output [17:0] q_0,
	input [17:0] data_1,
	output [17:0] q_1,
	input [17:0] data_2,
	output [17:0] q_2,
	input [17:0] data_3,
	output [17:0] q_3,
	input [17:0] data_4,
	output [17:0] q_4,
	input [17:0] data_5,
	output [17:0] q_5,
	input [17:0] data_6,
	output [17:0] q_6,
	input [17:0] data_7,
	output [17:0] q_7,
	input [17:0] data_8,
	output [17:0] q_8,
	input [17:0] data_9,
	output [17:0] q_9,
	input [17:0] data_10,
	output [17:0] q_10,
	input [17:0] data_11,
	output [17:0] q_11,
	input [17:0] data_12,
	output [17:0] q_12,
	input [17:0] data_13,
	output [17:0] q_13,
	input [17:0] data_14,
	output [17:0] q_14,
	input [17:0] data_15,
	output [17:0] q_15,
	output o_valid
);

wire [287:0] packed_result;
wire [287:0] packed_data;

wire [13:0] input_index_counter;
reg is_buffer_full;
counter_63_1 counter_63_1_inst_in (
	.clk(clk),
	.reset(reset),
	.ena(i_valid),
	.count(input_index_counter)
);

reg en_output_counter;
wire [13:0] output_index_counter;
counter_63_1 counter_63_1_inst_out_count (
	.clk(clk),
	.reset(reset),
	.ena(en_output_counter),
	.count(output_index_counter)
);

reg [5:0] raddr;
always @ (*) begin
	if (is_buffer_full)
		raddr <= output_index_counter;
	else
		raddr <= input_index_counter;
end

wire incr_loop_index;
assign incr_loop_index = (output_index_counter ==  (63) && en_output_counter);

reg is_output_enough;
wire [13:0] loop_counter;
counter_30_1 counter_30_1_inst_out_enough (
	.clk(clk),
	.reset(reset),
	.ena(incr_loop_index),
	.count(loop_counter)
);

ram_288_0_64 ram_288_0_64_inst (
	.clk(clk),
	.waddr(input_index_counter),
	.wdata(packed_data),
	.wen(i_valid),
	.raddr(raddr),
	.q(packed_result)
);

assign q_0 = packed_result[17:0];
assign packed_data[17:0] = data_0;
assign q_1 = packed_result[35:18];
assign packed_data[35:18] = data_1;
assign q_2 = packed_result[53:36];
assign packed_data[53:36] = data_2;
assign q_3 = packed_result[71:54];
assign packed_data[71:54] = data_3;
assign q_4 = packed_result[89:72];
assign packed_data[89:72] = data_4;
assign q_5 = packed_result[107:90];
assign packed_data[107:90] = data_5;
assign q_6 = packed_result[125:108];
assign packed_data[125:108] = data_6;
assign q_7 = packed_result[143:126];
assign packed_data[143:126] = data_7;
assign q_8 = packed_result[161:144];
assign packed_data[161:144] = data_8;
assign q_9 = packed_result[179:162];
assign packed_data[179:162] = data_9;
assign q_10 = packed_result[197:180];
assign packed_data[197:180] = data_10;
assign q_11 = packed_result[215:198];
assign packed_data[215:198] = data_11;
assign q_12 = packed_result[233:216];
assign packed_data[233:216] = data_12;
assign q_13 = packed_result[251:234];
assign packed_data[251:234] = data_13;
assign q_14 = packed_result[269:252];
assign packed_data[269:252] = data_14;
assign q_15 = packed_result[287:270];
assign packed_data[287:270] = data_15;

always @ (posedge clk) begin
	if (reset) begin
		en_output_counter <= 1'b0;
		is_buffer_full <= 1'b0;
		is_output_enough <= 1'b0;
	end else begin
		en_output_counter <= (is_buffer_full && ~en_output_counter && ~is_output_enough);
		if (input_index_counter == 63 && i_valid)
			is_buffer_full <= 1'b1;
		else if (input_index_counter == 0 && output_index_counter == 0 && is_output_enough)
			is_buffer_full <= 1'b0;
		if ((loop_counter == (30))
			&&(output_index_counter == 63)
			&& en_output_counter)
			is_output_enough <= 1'b1;
		else if (loop_counter == 0 && i_valid)
			is_output_enough <= 1'b0;
	end
end

wire valid_1, valid_2, is_buffer_full_hold;
shift_register_unit_12 shift_register_unit_12_inst_is_buffer_full (
	.clk(clk),
	.reset(reset),
	.enable(1'b1),
	.in(is_buffer_full),
	.out(is_buffer_full_hold)
);

shift_register_unit_12 shift_register_unit_12_inst_valid1 (
	.clk(clk),
	.reset(reset),
	.enable(1'b1),
	.in(i_valid),
	.out(valid_1)
);

shift_register_unit_12 shift_register_unit_12_inst_valid2 (
	.clk(clk),
	.reset(reset),
	.enable(1'b1),
	.in(en_output_counter),
	.out(valid_2)
);

reg output_valid;
always @ (*) begin
	if (is_buffer_full_hold)
		output_valid <= valid_2;
	else
		output_valid <= valid_1;
end
assign o_valid = output_valid;

endmodule

module counter_30_1 (
	input clk,
	input reset,
	input ena,
	output reg [13:0] count
);

always @ (posedge clk) begin 
	if (reset) begin
		count <= 0;
	end else if (ena) begin
		if((count + 1) <= 30) begin
			count <= count + 1;
		end else begin
			count <= 14'd0;
		end
	end
end

endmodule

module shift_register_unit_12 (
	input clk,
	input reset,
	input enable,
	input [0:0] in,
	output [0:0] out
);

reg [0:0] shift_registers_0;
reg [0:0] shift_registers_1;
always @ (posedge clk) begin
	if (reset) begin
		shift_registers_0 <= 1'd0;
		shift_registers_1 <= 1'd0;
	end else if (enable) begin
		shift_registers_0 <= in;
		shift_registers_1 <= shift_registers_0;
	end
end

assign out = shift_registers_1;

endmodule


