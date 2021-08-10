//////////////////////////////////////////////////////////////////////////////
// Author: Andrew Boutros
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//A CNN accelerator overlay called DLA from Intel based on the paper:
//U. Aydonat et al., “An OpenCL Deep Learning Accelerator on Arria10,” in International Symposium on Field-Programmable Gate Arrays (FPGA), 2017.
//This design was also used in this paper: 
//A. Boutros et al., “You Cannot Improve What You Do Not Measure: FPGA vs. ASIC Efficiency Gaps for Convolutional Neural Network Inference,” ACM Transactions on Reconfigurable Technology Systems (TRETS), vol. 11, no. 3, 2018
//
//Some properties of the design are:
//1. 16-bit fixed point for activations, 8-bit fixed point for weights 
//2. Winograd Transform based convolution. 
//3. 2D mac array. Centralized weight buffer for processing elements. 
//4. Double-buffering after each layer. 
///////////////////////////////////////////////////////////////////////////////


module DLA (
	input clk,
	input i_reset,
	input [15:0] i_ddr_0_0,
	output o_dummy_out_0_0,
	input [15:0] i_ddr_0_1,
	output o_dummy_out_0_1,
	input [15:0] i_ddr_1_0,
	output o_dummy_out_1_0,
	input [15:0] i_ddr_1_1,
	output o_dummy_out_1_1,
	input [15:0] i_ddr_2_0,
	output o_dummy_out_2_0,
	input [15:0] i_ddr_2_1,
	output o_dummy_out_2_1,
	input [15:0] i_ddr_3_0,
	output o_dummy_out_3_0,
	input [15:0] i_ddr_3_1,
	output o_dummy_out_3_1,
	input [15:0] i_ddr_4_0,
	output o_dummy_out_4_0,
	input [15:0] i_ddr_4_1,
	output o_dummy_out_4_1,
	input [15:0] i_ddr_5_0,
	output o_dummy_out_5_0,
	input [15:0] i_ddr_5_1,
	output o_dummy_out_5_1,
	output o_valid
);

wire [15:0] f_buffer_pe_0_0;
wire valid_buff_0_0;
wire [15:0] f_buffer_pe_0_1;
wire valid_buff_0_1;
wire [15:0] f_buffer_pe_1_0;
wire valid_buff_1_0;
wire [15:0] f_buffer_pe_1_1;
wire valid_buff_1_1;
wire [15:0] f_buffer_pe_2_0;
wire valid_buff_2_0;
wire [15:0] f_buffer_pe_2_1;
wire valid_buff_2_1;
wire [15:0] f_buffer_pe_3_0;
wire valid_buff_3_0;
wire [15:0] f_buffer_pe_3_1;
wire valid_buff_3_1;
wire [15:0] f_buffer_pe_4_0;
wire valid_buff_4_0;
wire [15:0] f_buffer_pe_4_1;
wire valid_buff_4_1;
wire [15:0] f_buffer_pe_5_0;
wire valid_buff_5_0;
wire [15:0] f_buffer_pe_5_1;
wire valid_buff_5_1;
wire ready;
wire [15:0] f_winograd_0_0;
wire [15:0] f_winograd_0_1;
wire [15:0] f_winograd_0_2;
wire [15:0] f_winograd_0_3;
wire [15:0] f_winograd_0_4;
wire [15:0] f_winograd_0_5;
wire [15:0] f_winograd_1_0;
wire [15:0] f_winograd_1_1;
wire [15:0] f_winograd_1_2;
wire [15:0] f_winograd_1_3;
wire [15:0] f_winograd_1_4;
wire [15:0] f_winograd_1_5;
wire winograd_valid_0;
wire winograd_valid_1;

// PE Wires
wire [15:0] daisy_chain_0_0_0;
wire [15:0] daisy_chain_0_0_1;
wire [15:0] daisy_chain_0_0_2;
wire [15:0] daisy_chain_0_0_3;
wire [15:0] daisy_chain_0_0_4;
wire [15:0] daisy_chain_0_0_5;
wire [15:0] daisy_chain_0_1_0;
wire [15:0] daisy_chain_0_1_1;
wire [15:0] daisy_chain_0_1_2;
wire [15:0] daisy_chain_0_1_3;
wire [15:0] daisy_chain_0_1_4;
wire [15:0] daisy_chain_0_1_5;
wire [15:0] daisy_chain_1_0_0;
wire [15:0] daisy_chain_1_0_1;
wire [15:0] daisy_chain_1_0_2;
wire [15:0] daisy_chain_1_0_3;
wire [15:0] daisy_chain_1_0_4;
wire [15:0] daisy_chain_1_0_5;
wire [15:0] daisy_chain_1_1_0;
wire [15:0] daisy_chain_1_1_1;
wire [15:0] daisy_chain_1_1_2;
wire [15:0] daisy_chain_1_1_3;
wire [15:0] daisy_chain_1_1_4;
wire [15:0] daisy_chain_1_1_5;
wire [15:0] daisy_chain_2_0_0;
wire [15:0] daisy_chain_2_0_1;
wire [15:0] daisy_chain_2_0_2;
wire [15:0] daisy_chain_2_0_3;
wire [15:0] daisy_chain_2_0_4;
wire [15:0] daisy_chain_2_0_5;
wire [15:0] daisy_chain_2_1_0;
wire [15:0] daisy_chain_2_1_1;
wire [15:0] daisy_chain_2_1_2;
wire [15:0] daisy_chain_2_1_3;
wire [15:0] daisy_chain_2_1_4;
wire [15:0] daisy_chain_2_1_5;
wire [15:0] daisy_chain_3_0_0;
wire [15:0] daisy_chain_3_0_1;
wire [15:0] daisy_chain_3_0_2;
wire [15:0] daisy_chain_3_0_3;
wire [15:0] daisy_chain_3_0_4;
wire [15:0] daisy_chain_3_0_5;
wire [15:0] daisy_chain_3_1_0;
wire [15:0] daisy_chain_3_1_1;
wire [15:0] daisy_chain_3_1_2;
wire [15:0] daisy_chain_3_1_3;
wire [15:0] daisy_chain_3_1_4;
wire [15:0] daisy_chain_3_1_5;
wire [15:0] daisy_chain_4_0_0;
wire [15:0] daisy_chain_4_0_1;
wire [15:0] daisy_chain_4_0_2;
wire [15:0] daisy_chain_4_0_3;
wire [15:0] daisy_chain_4_0_4;
wire [15:0] daisy_chain_4_0_5;
wire [15:0] daisy_chain_4_1_0;
wire [15:0] daisy_chain_4_1_1;
wire [15:0] daisy_chain_4_1_2;
wire [15:0] daisy_chain_4_1_3;
wire [15:0] daisy_chain_4_1_4;
wire [15:0] daisy_chain_4_1_5;
wire [15:0] daisy_chain_5_0_0;
wire [15:0] daisy_chain_5_0_1;
wire [15:0] daisy_chain_5_0_2;
wire [15:0] daisy_chain_5_0_3;
wire [15:0] daisy_chain_5_0_4;
wire [15:0] daisy_chain_5_0_5;
wire [15:0] daisy_chain_5_1_0;
wire [15:0] daisy_chain_5_1_1;
wire [15:0] daisy_chain_5_1_2;
wire [15:0] daisy_chain_5_1_3;
wire [15:0] daisy_chain_5_1_4;
wire [15:0] daisy_chain_5_1_5;
wire [15:0] daisy_chain_6_0_0;
wire [15:0] daisy_chain_6_0_1;
wire [15:0] daisy_chain_6_0_2;
wire [15:0] daisy_chain_6_0_3;
wire [15:0] daisy_chain_6_0_4;
wire [15:0] daisy_chain_6_0_5;
wire [15:0] daisy_chain_6_1_0;
wire [15:0] daisy_chain_6_1_1;
wire [15:0] daisy_chain_6_1_2;
wire [15:0] daisy_chain_6_1_3;
wire [15:0] daisy_chain_6_1_4;
wire [15:0] daisy_chain_6_1_5;
wire [15:0] daisy_chain_7_0_0;
wire [15:0] daisy_chain_7_0_1;
wire [15:0] daisy_chain_7_0_2;
wire [15:0] daisy_chain_7_0_3;
wire [15:0] daisy_chain_7_0_4;
wire [15:0] daisy_chain_7_0_5;
wire [15:0] daisy_chain_7_1_0;
wire [15:0] daisy_chain_7_1_1;
wire [15:0] daisy_chain_7_1_2;
wire [15:0] daisy_chain_7_1_3;
wire [15:0] daisy_chain_7_1_4;
wire [15:0] daisy_chain_7_1_5;
wire [15:0] daisy_chain_8_0_0;
wire [15:0] daisy_chain_8_0_1;
wire [15:0] daisy_chain_8_0_2;
wire [15:0] daisy_chain_8_0_3;
wire [15:0] daisy_chain_8_0_4;
wire [15:0] daisy_chain_8_0_5;
wire [15:0] daisy_chain_8_1_0;
wire [15:0] daisy_chain_8_1_1;
wire [15:0] daisy_chain_8_1_2;
wire [15:0] daisy_chain_8_1_3;
wire [15:0] daisy_chain_8_1_4;
wire [15:0] daisy_chain_8_1_5;
wire [15:0] daisy_chain_9_0_0;
wire [15:0] daisy_chain_9_0_1;
wire [15:0] daisy_chain_9_0_2;
wire [15:0] daisy_chain_9_0_3;
wire [15:0] daisy_chain_9_0_4;
wire [15:0] daisy_chain_9_0_5;
wire [15:0] daisy_chain_9_1_0;
wire [15:0] daisy_chain_9_1_1;
wire [15:0] daisy_chain_9_1_2;
wire [15:0] daisy_chain_9_1_3;
wire [15:0] daisy_chain_9_1_4;
wire [15:0] daisy_chain_9_1_5;
wire [15:0] daisy_chain_10_0_0;
wire [15:0] daisy_chain_10_0_1;
wire [15:0] daisy_chain_10_0_2;
wire [15:0] daisy_chain_10_0_3;
wire [15:0] daisy_chain_10_0_4;
wire [15:0] daisy_chain_10_0_5;
wire [15:0] daisy_chain_10_1_0;
wire [15:0] daisy_chain_10_1_1;
wire [15:0] daisy_chain_10_1_2;
wire [15:0] daisy_chain_10_1_3;
wire [15:0] daisy_chain_10_1_4;
wire [15:0] daisy_chain_10_1_5;
wire [15:0] daisy_chain_11_0_0;
wire [15:0] daisy_chain_11_0_1;
wire [15:0] daisy_chain_11_0_2;
wire [15:0] daisy_chain_11_0_3;
wire [15:0] daisy_chain_11_0_4;
wire [15:0] daisy_chain_11_0_5;
wire [15:0] daisy_chain_11_1_0;
wire [15:0] daisy_chain_11_1_1;
wire [15:0] daisy_chain_11_1_2;
wire [15:0] daisy_chain_11_1_3;
wire [15:0] daisy_chain_11_1_4;
wire [15:0] daisy_chain_11_1_5;
wire [29:0] PE_output_0_0;
wire [29:0] PE_output_0_1;
wire [29:0] PE_output_0_2;
wire [29:0] PE_output_0_3;
wire [29:0] PE_output_0_4;
wire [29:0] PE_output_0_5;
wire [29:0] PE_output_1_0;
wire [29:0] PE_output_1_1;
wire [29:0] PE_output_1_2;
wire [29:0] PE_output_1_3;
wire [29:0] PE_output_1_4;
wire [29:0] PE_output_1_5;
wire [29:0] PE_output_2_0;
wire [29:0] PE_output_2_1;
wire [29:0] PE_output_2_2;
wire [29:0] PE_output_2_3;
wire [29:0] PE_output_2_4;
wire [29:0] PE_output_2_5;
wire [29:0] PE_output_3_0;
wire [29:0] PE_output_3_1;
wire [29:0] PE_output_3_2;
wire [29:0] PE_output_3_3;
wire [29:0] PE_output_3_4;
wire [29:0] PE_output_3_5;
wire [29:0] PE_output_4_0;
wire [29:0] PE_output_4_1;
wire [29:0] PE_output_4_2;
wire [29:0] PE_output_4_3;
wire [29:0] PE_output_4_4;
wire [29:0] PE_output_4_5;
wire [29:0] PE_output_5_0;
wire [29:0] PE_output_5_1;
wire [29:0] PE_output_5_2;
wire [29:0] PE_output_5_3;
wire [29:0] PE_output_5_4;
wire [29:0] PE_output_5_5;
wire [29:0] PE_output_6_0;
wire [29:0] PE_output_6_1;
wire [29:0] PE_output_6_2;
wire [29:0] PE_output_6_3;
wire [29:0] PE_output_6_4;
wire [29:0] PE_output_6_5;
wire [29:0] PE_output_7_0;
wire [29:0] PE_output_7_1;
wire [29:0] PE_output_7_2;
wire [29:0] PE_output_7_3;
wire [29:0] PE_output_7_4;
wire [29:0] PE_output_7_5;
wire [29:0] PE_output_8_0;
wire [29:0] PE_output_8_1;
wire [29:0] PE_output_8_2;
wire [29:0] PE_output_8_3;
wire [29:0] PE_output_8_4;
wire [29:0] PE_output_8_5;
wire [29:0] PE_output_9_0;
wire [29:0] PE_output_9_1;
wire [29:0] PE_output_9_2;
wire [29:0] PE_output_9_3;
wire [29:0] PE_output_9_4;
wire [29:0] PE_output_9_5;
wire [29:0] PE_output_10_0;
wire [29:0] PE_output_10_1;
wire [29:0] PE_output_10_2;
wire [29:0] PE_output_10_3;
wire [29:0] PE_output_10_4;
wire [29:0] PE_output_10_5;
wire [29:0] PE_output_11_0;
wire [29:0] PE_output_11_1;
wire [29:0] PE_output_11_2;
wire [29:0] PE_output_11_3;
wire [29:0] PE_output_11_4;
wire [29:0] PE_output_11_5;
wire PE_valid_0;
wire PE_next_reset_0;
wire PE_next_valid_0;
wire PE_valid_1;
wire PE_next_reset_1;
wire PE_next_valid_1;
wire PE_valid_2;
wire PE_next_reset_2;
wire PE_next_valid_2;
wire PE_valid_3;
wire PE_next_reset_3;
wire PE_next_valid_3;
wire PE_valid_4;
wire PE_next_reset_4;
wire PE_next_valid_4;
wire PE_valid_5;
wire PE_next_reset_5;
wire PE_next_valid_5;
wire PE_valid_6;
wire PE_next_reset_6;
wire PE_next_valid_6;
wire PE_valid_7;
wire PE_next_reset_7;
wire PE_next_valid_7;
wire PE_valid_8;
wire PE_next_reset_8;
wire PE_next_valid_8;
wire PE_valid_9;
wire PE_next_reset_9;
wire PE_next_valid_9;
wire PE_valid_10;
wire PE_next_reset_10;
wire PE_next_valid_10;
wire PE_valid_11;
wire PE_next_reset_11;
wire PE_next_valid_11;

// Inverse Winograd Wires
wire [15:0] INV_output_0_0;
wire [15:0] INV_output_0_1;
wire [15:0] INV_output_0_2;
wire [15:0] INV_output_0_3;
wire INV_valid_0;
wire [15:0] INV_output_1_0;
wire [15:0] INV_output_1_1;
wire [15:0] INV_output_1_2;
wire [15:0] INV_output_1_3;
wire INV_valid_1;
wire [15:0] INV_output_2_0;
wire [15:0] INV_output_2_1;
wire [15:0] INV_output_2_2;
wire [15:0] INV_output_2_3;
wire INV_valid_2;
wire [15:0] INV_output_3_0;
wire [15:0] INV_output_3_1;
wire [15:0] INV_output_3_2;
wire [15:0] INV_output_3_3;
wire INV_valid_3;
wire [15:0] INV_output_4_0;
wire [15:0] INV_output_4_1;
wire [15:0] INV_output_4_2;
wire [15:0] INV_output_4_3;
wire INV_valid_4;
wire [15:0] INV_output_5_0;
wire [15:0] INV_output_5_1;
wire [15:0] INV_output_5_2;
wire [15:0] INV_output_5_3;
wire INV_valid_5;
wire [15:0] INV_output_6_0;
wire [15:0] INV_output_6_1;
wire [15:0] INV_output_6_2;
wire [15:0] INV_output_6_3;
wire INV_valid_6;
wire [15:0] INV_output_7_0;
wire [15:0] INV_output_7_1;
wire [15:0] INV_output_7_2;
wire [15:0] INV_output_7_3;
wire INV_valid_7;
wire [15:0] INV_output_8_0;
wire [15:0] INV_output_8_1;
wire [15:0] INV_output_8_2;
wire [15:0] INV_output_8_3;
wire INV_valid_8;
wire [15:0] INV_output_9_0;
wire [15:0] INV_output_9_1;
wire [15:0] INV_output_9_2;
wire [15:0] INV_output_9_3;
wire INV_valid_9;
wire [15:0] INV_output_10_0;
wire [15:0] INV_output_10_1;
wire [15:0] INV_output_10_2;
wire [15:0] INV_output_10_3;
wire INV_valid_10;
wire [15:0] INV_output_11_0;
wire [15:0] INV_output_11_1;
wire [15:0] INV_output_11_2;
wire [15:0] INV_output_11_3;
wire INV_valid_11;

// Pooling Wires
wire [15:0] POOL_output_0;
wire POOL_valid_0;
wire [15:0] POOL_output_1;
wire POOL_valid_1;
wire [15:0] POOL_output_2;
wire POOL_valid_2;
wire [15:0] POOL_output_3;
wire POOL_valid_3;
wire [15:0] POOL_output_4;
wire POOL_valid_4;
wire [15:0] POOL_output_5;
wire POOL_valid_5;
wire [15:0] POOL_output_6;
wire POOL_valid_6;
wire [15:0] POOL_output_7;
wire POOL_valid_7;
wire [15:0] POOL_output_8;
wire POOL_valid_8;
wire [15:0] POOL_output_9;
wire POOL_valid_9;
wire [15:0] POOL_output_10;
wire POOL_valid_10;
wire [15:0] POOL_output_11;
wire POOL_valid_11;

// Store Output Wires
wire [15:0] STORE_output_0_0;
wire [15:0] STORE_output_0_1;
wire [15:0] STORE_output_1_0;
wire [15:0] STORE_output_1_1;
wire [15:0] STORE_output_2_0;
wire [15:0] STORE_output_2_1;
wire [15:0] STORE_output_3_0;
wire [15:0] STORE_output_3_1;
wire [15:0] STORE_output_4_0;
wire [15:0] STORE_output_4_1;
wire [15:0] STORE_output_5_0;
wire [15:0] STORE_output_5_1;
wire [14:0] STORE_addr;
wire STORE_wen_0;
wire STORE_wen_1;
wire STORE_wen_2;
wire STORE_wen_3;
wire STORE_wen_4;
wire STORE_wen_5;

// Eltwise Wires
wire [15:0] f_buffer_el_0_0;
wire [15:0] f_buffer_el_0_1;
wire [15:0] f_buffer_el_1_0;
wire [15:0] f_buffer_el_1_1;
wire [15:0] f_buffer_el_2_0;
wire [15:0] f_buffer_el_2_1;
wire [15:0] f_buffer_el_3_0;
wire [15:0] f_buffer_el_3_1;
wire [15:0] f_buffer_el_4_0;
wire [15:0] f_buffer_el_4_1;
wire [15:0] f_buffer_el_5_0;
wire [15:0] f_buffer_el_5_1;

// Output Wires
wire [15:0] dummy_out_0_0;
wire [15:0] dummy_out_0_1;
wire [15:0] dummy_out_1_0;
wire [15:0] dummy_out_1_1;
wire [15:0] dummy_out_2_0;
wire [15:0] dummy_out_2_1;
wire [15:0] dummy_out_3_0;
wire [15:0] dummy_out_3_1;
wire [15:0] dummy_out_4_0;
wire [15:0] dummy_out_4_1;
wire [15:0] dummy_out_5_0;
wire [15:0] dummy_out_5_1;

stream_buffer_0_0 stream_buffer_0_0_inst (
	.clk(clk),
	.i_reset(i_reset),
	.i_wen0(1'b0),
	.i_wen1(STORE_wen_0),
	.i_ddr(i_ddr_0_0),
	.i_pool(STORE_output_0_0),
	.i_eltwise_sel(1'b0),
	.i_eltwise(0),
	.i_waddr(STORE_addr),
	.o_feature_0(f_buffer_pe_0_0),
	.o_feature_1(f_buffer_el_0_0),
	.o_done(valid_buff_0_0)
);
assign dummy_out_0_0 = f_buffer_el_0_0;

stream_buffer_0_1 stream_buffer_0_1_inst (
	.clk(clk),
	.i_reset(i_reset),
	.i_wen0(1'b0),
	.i_wen1(STORE_wen_0),
	.i_ddr(i_ddr_0_1),
	.i_pool(STORE_output_0_1),
	.i_eltwise_sel(1'b0),
	.i_eltwise(0),
	.i_waddr(STORE_addr),
	.o_feature_0(f_buffer_pe_0_1),
	.o_feature_1(f_buffer_el_0_1),
	.o_done(valid_buff_0_1)
);
assign dummy_out_0_1 = f_buffer_el_0_1;

stream_buffer_1_0 stream_buffer_1_0_inst (
	.clk(clk),
	.i_reset(i_reset),
	.i_wen0(1'b0),
	.i_wen1(STORE_wen_1),
	.i_ddr(i_ddr_1_0),
	.i_pool(STORE_output_1_0),
	.i_eltwise_sel(1'b0),
	.i_eltwise(0),
	.i_waddr(STORE_addr),
	.o_feature_0(f_buffer_pe_1_0),
	.o_feature_1(f_buffer_el_1_0),
	.o_done(valid_buff_1_0)
);
assign dummy_out_1_0 = f_buffer_el_1_0;

stream_buffer_1_1 stream_buffer_1_1_inst (
	.clk(clk),
	.i_reset(i_reset),
	.i_wen0(1'b0),
	.i_wen1(STORE_wen_1),
	.i_ddr(i_ddr_1_1),
	.i_pool(STORE_output_1_1),
	.i_eltwise_sel(1'b0),
	.i_eltwise(0),
	.i_waddr(STORE_addr),
	.o_feature_0(f_buffer_pe_1_1),
	.o_feature_1(f_buffer_el_1_1),
	.o_done(valid_buff_1_1)
);
assign dummy_out_1_1 = f_buffer_el_1_1;

stream_buffer_2_0 stream_buffer_2_0_inst (
	.clk(clk),
	.i_reset(i_reset),
	.i_wen0(1'b0),
	.i_wen1(STORE_wen_2),
	.i_ddr(i_ddr_2_0),
	.i_pool(STORE_output_2_0),
	.i_eltwise_sel(1'b0),
	.i_eltwise(0),
	.i_waddr(STORE_addr),
	.o_feature_0(f_buffer_pe_2_0),
	.o_feature_1(f_buffer_el_2_0),
	.o_done(valid_buff_2_0)
);
assign dummy_out_2_0 = f_buffer_el_2_0;

stream_buffer_2_1 stream_buffer_2_1_inst (
	.clk(clk),
	.i_reset(i_reset),
	.i_wen0(1'b0),
	.i_wen1(STORE_wen_2),
	.i_ddr(i_ddr_2_1),
	.i_pool(STORE_output_2_1),
	.i_eltwise_sel(1'b0),
	.i_eltwise(0),
	.i_waddr(STORE_addr),
	.o_feature_0(f_buffer_pe_2_1),
	.o_feature_1(f_buffer_el_2_1),
	.o_done(valid_buff_2_1)
);
assign dummy_out_2_1 = f_buffer_el_2_1;

stream_buffer_3_0 stream_buffer_3_0_inst (
	.clk(clk),
	.i_reset(i_reset),
	.i_wen0(1'b0),
	.i_wen1(STORE_wen_3),
	.i_ddr(i_ddr_3_0),
	.i_pool(STORE_output_3_0),
	.i_eltwise_sel(1'b0),
	.i_eltwise(0),
	.i_waddr(STORE_addr),
	.o_feature_0(f_buffer_pe_3_0),
	.o_feature_1(f_buffer_el_3_0),
	.o_done(valid_buff_3_0)
);
assign dummy_out_3_0 = f_buffer_el_3_0;

stream_buffer_3_1 stream_buffer_3_1_inst (
	.clk(clk),
	.i_reset(i_reset),
	.i_wen0(1'b0),
	.i_wen1(STORE_wen_3),
	.i_ddr(i_ddr_3_1),
	.i_pool(STORE_output_3_1),
	.i_eltwise_sel(1'b0),
	.i_eltwise(0),
	.i_waddr(STORE_addr),
	.o_feature_0(f_buffer_pe_3_1),
	.o_feature_1(f_buffer_el_3_1),
	.o_done(valid_buff_3_1)
);
assign dummy_out_3_1 = f_buffer_el_3_1;

stream_buffer_4_0 stream_buffer_4_0_inst (
	.clk(clk),
	.i_reset(i_reset),
	.i_wen0(1'b0),
	.i_wen1(STORE_wen_4),
	.i_ddr(i_ddr_4_0),
	.i_pool(STORE_output_4_0),
	.i_eltwise_sel(1'b0),
	.i_eltwise(0),
	.i_waddr(STORE_addr),
	.o_feature_0(f_buffer_pe_4_0),
	.o_feature_1(f_buffer_el_4_0),
	.o_done(valid_buff_4_0)
);
assign dummy_out_4_0 = f_buffer_el_4_0;

stream_buffer_4_1 stream_buffer_4_1_inst (
	.clk(clk),
	.i_reset(i_reset),
	.i_wen0(1'b0),
	.i_wen1(STORE_wen_4),
	.i_ddr(i_ddr_4_1),
	.i_pool(STORE_output_4_1),
	.i_eltwise_sel(1'b0),
	.i_eltwise(0),
	.i_waddr(STORE_addr),
	.o_feature_0(f_buffer_pe_4_1),
	.o_feature_1(f_buffer_el_4_1),
	.o_done(valid_buff_4_1)
);
assign dummy_out_4_1 = f_buffer_el_4_1;

stream_buffer_5_0 stream_buffer_5_0_inst (
	.clk(clk),
	.i_reset(i_reset),
	.i_wen0(1'b0),
	.i_wen1(STORE_wen_5),
	.i_ddr(i_ddr_5_0),
	.i_pool(STORE_output_5_0),
	.i_eltwise_sel(1'b0),
	.i_eltwise(0),
	.i_waddr(STORE_addr),
	.o_feature_0(f_buffer_pe_5_0),
	.o_feature_1(f_buffer_el_5_0),
	.o_done(valid_buff_5_0)
);
assign dummy_out_5_0 = f_buffer_el_5_0;

stream_buffer_5_1 stream_buffer_5_1_inst (
	.clk(clk),
	.i_reset(i_reset),
	.i_wen0(1'b0),
	.i_wen1(STORE_wen_5),
	.i_ddr(i_ddr_5_1),
	.i_pool(STORE_output_5_1),
	.i_eltwise_sel(1'b0),
	.i_eltwise(0),
	.i_waddr(STORE_addr),
	.o_feature_0(f_buffer_pe_5_1),
	.o_feature_1(f_buffer_el_5_1),
	.o_done(valid_buff_5_1)
);
assign dummy_out_5_1 = f_buffer_el_5_1;

winograd_transform_0 winograd_transform_0_inst (
	.clk(clk),
	.i_valid(valid_buff_0_0),
	.i_result_0_0(f_buffer_pe_0_0),
	.i_result_0_1(f_buffer_pe_0_1),
	.i_result_1_0(f_buffer_pe_1_0),
	.i_result_1_1(f_buffer_pe_1_1),
	.i_result_2_0(f_buffer_pe_2_0),
	.i_result_2_1(f_buffer_pe_2_1),
	.i_result_3_0(f_buffer_pe_3_0),
	.i_result_3_1(f_buffer_pe_3_1),
	.i_result_4_0(f_buffer_pe_4_0),
	.i_result_4_1(f_buffer_pe_4_1),
	.i_result_5_0(f_buffer_pe_5_0),
	.i_result_5_1(f_buffer_pe_5_1),
	.o_feature_0(f_winograd_0_0),
	.o_feature_1(f_winograd_0_1),
	.o_feature_2(f_winograd_0_2),
	.o_feature_3(f_winograd_0_3),
	.o_feature_4(f_winograd_0_4),
	.o_feature_5(f_winograd_0_5),
	.o_valid(winograd_valid_0)
);

winograd_transform_1 winograd_transform_1_inst (
	.clk(clk),
	.i_valid(valid_buff_0_0),
	.i_result_0_0(f_buffer_pe_0_0),
	.i_result_0_1(f_buffer_pe_0_1),
	.i_result_1_0(f_buffer_pe_1_0),
	.i_result_1_1(f_buffer_pe_1_1),
	.i_result_2_0(f_buffer_pe_2_0),
	.i_result_2_1(f_buffer_pe_2_1),
	.i_result_3_0(f_buffer_pe_3_0),
	.i_result_3_1(f_buffer_pe_3_1),
	.i_result_4_0(f_buffer_pe_4_0),
	.i_result_4_1(f_buffer_pe_4_1),
	.i_result_5_0(f_buffer_pe_5_0),
	.i_result_5_1(f_buffer_pe_5_1),
	.o_feature_0(f_winograd_1_0),
	.o_feature_1(f_winograd_1_1),
	.o_feature_2(f_winograd_1_2),
	.o_feature_3(f_winograd_1_3),
	.o_feature_4(f_winograd_1_4),
	.o_feature_5(f_winograd_1_5),
	.o_valid(winograd_valid_1)
);

processing_element processing_element_inst_0 (
	.clk(clk),
	.i_reset(i_reset),
	.i_valid(winograd_valid_0),
	.i_features_0_0(f_winograd_0_0),
	.o_features_0_0(daisy_chain_0_0_0),
	.i_features_0_1(f_winograd_0_1),
	.o_features_0_1(daisy_chain_0_0_1),
	.i_features_0_2(f_winograd_0_2),
	.o_features_0_2(daisy_chain_0_0_2),
	.i_features_0_3(f_winograd_0_3),
	.o_features_0_3(daisy_chain_0_0_3),
	.i_features_0_4(f_winograd_0_4),
	.o_features_0_4(daisy_chain_0_0_4),
	.i_features_0_5(f_winograd_0_5),
	.o_features_0_5(daisy_chain_0_0_5),
	.i_features_1_0(f_winograd_1_0),
	.o_features_1_0(daisy_chain_0_1_0),
	.i_features_1_1(f_winograd_1_1),
	.o_features_1_1(daisy_chain_0_1_1),
	.i_features_1_2(f_winograd_1_2),
	.o_features_1_2(daisy_chain_0_1_2),
	.i_features_1_3(f_winograd_1_3),
	.o_features_1_3(daisy_chain_0_1_3),
	.i_features_1_4(f_winograd_1_4),
	.o_features_1_4(daisy_chain_0_1_4),
	.i_features_1_5(f_winograd_1_5),
	.o_features_1_5(daisy_chain_0_1_5),
	.o_result_0(PE_output_0_0),
	.o_result_1(PE_output_0_1),
	.o_result_2(PE_output_0_2),
	.o_result_3(PE_output_0_3),
	.o_result_4(PE_output_0_4),
	.o_result_5(PE_output_0_5),
	.o_valid(PE_valid_0),
	.o_next_reset(PE_next_reset_0),
	.o_next_valid(PE_next_valid_0)
);

processing_element processing_element_inst_1 (
	.clk(clk),
	.i_reset(PE_next_reset_0),
	.i_valid(PE_next_valid_0),
	.i_features_0_0(daisy_chain_0_0_0),
	.o_features_0_0(daisy_chain_1_0_0),
	.i_features_0_1(daisy_chain_0_0_1),
	.o_features_0_1(daisy_chain_1_0_1),
	.i_features_0_2(daisy_chain_0_0_2),
	.o_features_0_2(daisy_chain_1_0_2),
	.i_features_0_3(daisy_chain_0_0_3),
	.o_features_0_3(daisy_chain_1_0_3),
	.i_features_0_4(daisy_chain_0_0_4),
	.o_features_0_4(daisy_chain_1_0_4),
	.i_features_0_5(daisy_chain_0_0_5),
	.o_features_0_5(daisy_chain_1_0_5),
	.i_features_1_0(daisy_chain_0_1_0),
	.o_features_1_0(daisy_chain_1_1_0),
	.i_features_1_1(daisy_chain_0_1_1),
	.o_features_1_1(daisy_chain_1_1_1),
	.i_features_1_2(daisy_chain_0_1_2),
	.o_features_1_2(daisy_chain_1_1_2),
	.i_features_1_3(daisy_chain_0_1_3),
	.o_features_1_3(daisy_chain_1_1_3),
	.i_features_1_4(daisy_chain_0_1_4),
	.o_features_1_4(daisy_chain_1_1_4),
	.i_features_1_5(daisy_chain_0_1_5),
	.o_features_1_5(daisy_chain_1_1_5),
	.o_result_0(PE_output_1_0),
	.o_result_1(PE_output_1_1),
	.o_result_2(PE_output_1_2),
	.o_result_3(PE_output_1_3),
	.o_result_4(PE_output_1_4),
	.o_result_5(PE_output_1_5),
	.o_valid(PE_valid_1),
	.o_next_reset(PE_next_reset_1),
	.o_next_valid(PE_next_valid_1)
);

processing_element processing_element_inst_2 (
	.clk(clk),
	.i_reset(PE_next_reset_1),
	.i_valid(PE_next_valid_1),
	.i_features_0_0(daisy_chain_1_0_0),
	.o_features_0_0(daisy_chain_2_0_0),
	.i_features_0_1(daisy_chain_1_0_1),
	.o_features_0_1(daisy_chain_2_0_1),
	.i_features_0_2(daisy_chain_1_0_2),
	.o_features_0_2(daisy_chain_2_0_2),
	.i_features_0_3(daisy_chain_1_0_3),
	.o_features_0_3(daisy_chain_2_0_3),
	.i_features_0_4(daisy_chain_1_0_4),
	.o_features_0_4(daisy_chain_2_0_4),
	.i_features_0_5(daisy_chain_1_0_5),
	.o_features_0_5(daisy_chain_2_0_5),
	.i_features_1_0(daisy_chain_1_1_0),
	.o_features_1_0(daisy_chain_2_1_0),
	.i_features_1_1(daisy_chain_1_1_1),
	.o_features_1_1(daisy_chain_2_1_1),
	.i_features_1_2(daisy_chain_1_1_2),
	.o_features_1_2(daisy_chain_2_1_2),
	.i_features_1_3(daisy_chain_1_1_3),
	.o_features_1_3(daisy_chain_2_1_3),
	.i_features_1_4(daisy_chain_1_1_4),
	.o_features_1_4(daisy_chain_2_1_4),
	.i_features_1_5(daisy_chain_1_1_5),
	.o_features_1_5(daisy_chain_2_1_5),
	.o_result_0(PE_output_2_0),
	.o_result_1(PE_output_2_1),
	.o_result_2(PE_output_2_2),
	.o_result_3(PE_output_2_3),
	.o_result_4(PE_output_2_4),
	.o_result_5(PE_output_2_5),
	.o_valid(PE_valid_2),
	.o_next_reset(PE_next_reset_2),
	.o_next_valid(PE_next_valid_2)
);

processing_element processing_element_inst_3 (
	.clk(clk),
	.i_reset(PE_next_reset_2),
	.i_valid(PE_next_valid_2),
	.i_features_0_0(daisy_chain_2_0_0),
	.o_features_0_0(daisy_chain_3_0_0),
	.i_features_0_1(daisy_chain_2_0_1),
	.o_features_0_1(daisy_chain_3_0_1),
	.i_features_0_2(daisy_chain_2_0_2),
	.o_features_0_2(daisy_chain_3_0_2),
	.i_features_0_3(daisy_chain_2_0_3),
	.o_features_0_3(daisy_chain_3_0_3),
	.i_features_0_4(daisy_chain_2_0_4),
	.o_features_0_4(daisy_chain_3_0_4),
	.i_features_0_5(daisy_chain_2_0_5),
	.o_features_0_5(daisy_chain_3_0_5),
	.i_features_1_0(daisy_chain_2_1_0),
	.o_features_1_0(daisy_chain_3_1_0),
	.i_features_1_1(daisy_chain_2_1_1),
	.o_features_1_1(daisy_chain_3_1_1),
	.i_features_1_2(daisy_chain_2_1_2),
	.o_features_1_2(daisy_chain_3_1_2),
	.i_features_1_3(daisy_chain_2_1_3),
	.o_features_1_3(daisy_chain_3_1_3),
	.i_features_1_4(daisy_chain_2_1_4),
	.o_features_1_4(daisy_chain_3_1_4),
	.i_features_1_5(daisy_chain_2_1_5),
	.o_features_1_5(daisy_chain_3_1_5),
	.o_result_0(PE_output_3_0),
	.o_result_1(PE_output_3_1),
	.o_result_2(PE_output_3_2),
	.o_result_3(PE_output_3_3),
	.o_result_4(PE_output_3_4),
	.o_result_5(PE_output_3_5),
	.o_valid(PE_valid_3),
	.o_next_reset(PE_next_reset_3),
	.o_next_valid(PE_next_valid_3)
);

processing_element processing_element_inst_4 (
	.clk(clk),
	.i_reset(PE_next_reset_3),
	.i_valid(PE_next_valid_3),
	.i_features_0_0(daisy_chain_3_0_0),
	.o_features_0_0(daisy_chain_4_0_0),
	.i_features_0_1(daisy_chain_3_0_1),
	.o_features_0_1(daisy_chain_4_0_1),
	.i_features_0_2(daisy_chain_3_0_2),
	.o_features_0_2(daisy_chain_4_0_2),
	.i_features_0_3(daisy_chain_3_0_3),
	.o_features_0_3(daisy_chain_4_0_3),
	.i_features_0_4(daisy_chain_3_0_4),
	.o_features_0_4(daisy_chain_4_0_4),
	.i_features_0_5(daisy_chain_3_0_5),
	.o_features_0_5(daisy_chain_4_0_5),
	.i_features_1_0(daisy_chain_3_1_0),
	.o_features_1_0(daisy_chain_4_1_0),
	.i_features_1_1(daisy_chain_3_1_1),
	.o_features_1_1(daisy_chain_4_1_1),
	.i_features_1_2(daisy_chain_3_1_2),
	.o_features_1_2(daisy_chain_4_1_2),
	.i_features_1_3(daisy_chain_3_1_3),
	.o_features_1_3(daisy_chain_4_1_3),
	.i_features_1_4(daisy_chain_3_1_4),
	.o_features_1_4(daisy_chain_4_1_4),
	.i_features_1_5(daisy_chain_3_1_5),
	.o_features_1_5(daisy_chain_4_1_5),
	.o_result_0(PE_output_4_0),
	.o_result_1(PE_output_4_1),
	.o_result_2(PE_output_4_2),
	.o_result_3(PE_output_4_3),
	.o_result_4(PE_output_4_4),
	.o_result_5(PE_output_4_5),
	.o_valid(PE_valid_4),
	.o_next_reset(PE_next_reset_4),
	.o_next_valid(PE_next_valid_4)
);

processing_element processing_element_inst_5 (
	.clk(clk),
	.i_reset(PE_next_reset_4),
	.i_valid(PE_next_valid_4),
	.i_features_0_0(daisy_chain_4_0_0),
	.o_features_0_0(daisy_chain_5_0_0),
	.i_features_0_1(daisy_chain_4_0_1),
	.o_features_0_1(daisy_chain_5_0_1),
	.i_features_0_2(daisy_chain_4_0_2),
	.o_features_0_2(daisy_chain_5_0_2),
	.i_features_0_3(daisy_chain_4_0_3),
	.o_features_0_3(daisy_chain_5_0_3),
	.i_features_0_4(daisy_chain_4_0_4),
	.o_features_0_4(daisy_chain_5_0_4),
	.i_features_0_5(daisy_chain_4_0_5),
	.o_features_0_5(daisy_chain_5_0_5),
	.i_features_1_0(daisy_chain_4_1_0),
	.o_features_1_0(daisy_chain_5_1_0),
	.i_features_1_1(daisy_chain_4_1_1),
	.o_features_1_1(daisy_chain_5_1_1),
	.i_features_1_2(daisy_chain_4_1_2),
	.o_features_1_2(daisy_chain_5_1_2),
	.i_features_1_3(daisy_chain_4_1_3),
	.o_features_1_3(daisy_chain_5_1_3),
	.i_features_1_4(daisy_chain_4_1_4),
	.o_features_1_4(daisy_chain_5_1_4),
	.i_features_1_5(daisy_chain_4_1_5),
	.o_features_1_5(daisy_chain_5_1_5),
	.o_result_0(PE_output_5_0),
	.o_result_1(PE_output_5_1),
	.o_result_2(PE_output_5_2),
	.o_result_3(PE_output_5_3),
	.o_result_4(PE_output_5_4),
	.o_result_5(PE_output_5_5),
	.o_valid(PE_valid_5),
	.o_next_reset(PE_next_reset_5),
	.o_next_valid(PE_next_valid_5)
);

processing_element processing_element_inst_6 (
	.clk(clk),
	.i_reset(PE_next_reset_5),
	.i_valid(PE_next_valid_5),
	.i_features_0_0(daisy_chain_5_0_0),
	.o_features_0_0(daisy_chain_6_0_0),
	.i_features_0_1(daisy_chain_5_0_1),
	.o_features_0_1(daisy_chain_6_0_1),
	.i_features_0_2(daisy_chain_5_0_2),
	.o_features_0_2(daisy_chain_6_0_2),
	.i_features_0_3(daisy_chain_5_0_3),
	.o_features_0_3(daisy_chain_6_0_3),
	.i_features_0_4(daisy_chain_5_0_4),
	.o_features_0_4(daisy_chain_6_0_4),
	.i_features_0_5(daisy_chain_5_0_5),
	.o_features_0_5(daisy_chain_6_0_5),
	.i_features_1_0(daisy_chain_5_1_0),
	.o_features_1_0(daisy_chain_6_1_0),
	.i_features_1_1(daisy_chain_5_1_1),
	.o_features_1_1(daisy_chain_6_1_1),
	.i_features_1_2(daisy_chain_5_1_2),
	.o_features_1_2(daisy_chain_6_1_2),
	.i_features_1_3(daisy_chain_5_1_3),
	.o_features_1_3(daisy_chain_6_1_3),
	.i_features_1_4(daisy_chain_5_1_4),
	.o_features_1_4(daisy_chain_6_1_4),
	.i_features_1_5(daisy_chain_5_1_5),
	.o_features_1_5(daisy_chain_6_1_5),
	.o_result_0(PE_output_6_0),
	.o_result_1(PE_output_6_1),
	.o_result_2(PE_output_6_2),
	.o_result_3(PE_output_6_3),
	.o_result_4(PE_output_6_4),
	.o_result_5(PE_output_6_5),
	.o_valid(PE_valid_6),
	.o_next_reset(PE_next_reset_6),
	.o_next_valid(PE_next_valid_6)
);

processing_element processing_element_inst_7 (
	.clk(clk),
	.i_reset(PE_next_reset_6),
	.i_valid(PE_next_valid_6),
	.i_features_0_0(daisy_chain_6_0_0),
	.o_features_0_0(daisy_chain_7_0_0),
	.i_features_0_1(daisy_chain_6_0_1),
	.o_features_0_1(daisy_chain_7_0_1),
	.i_features_0_2(daisy_chain_6_0_2),
	.o_features_0_2(daisy_chain_7_0_2),
	.i_features_0_3(daisy_chain_6_0_3),
	.o_features_0_3(daisy_chain_7_0_3),
	.i_features_0_4(daisy_chain_6_0_4),
	.o_features_0_4(daisy_chain_7_0_4),
	.i_features_0_5(daisy_chain_6_0_5),
	.o_features_0_5(daisy_chain_7_0_5),
	.i_features_1_0(daisy_chain_6_1_0),
	.o_features_1_0(daisy_chain_7_1_0),
	.i_features_1_1(daisy_chain_6_1_1),
	.o_features_1_1(daisy_chain_7_1_1),
	.i_features_1_2(daisy_chain_6_1_2),
	.o_features_1_2(daisy_chain_7_1_2),
	.i_features_1_3(daisy_chain_6_1_3),
	.o_features_1_3(daisy_chain_7_1_3),
	.i_features_1_4(daisy_chain_6_1_4),
	.o_features_1_4(daisy_chain_7_1_4),
	.i_features_1_5(daisy_chain_6_1_5),
	.o_features_1_5(daisy_chain_7_1_5),
	.o_result_0(PE_output_7_0),
	.o_result_1(PE_output_7_1),
	.o_result_2(PE_output_7_2),
	.o_result_3(PE_output_7_3),
	.o_result_4(PE_output_7_4),
	.o_result_5(PE_output_7_5),
	.o_valid(PE_valid_7),
	.o_next_reset(PE_next_reset_7),
	.o_next_valid(PE_next_valid_7)
);

processing_element processing_element_inst_8 (
	.clk(clk),
	.i_reset(PE_next_reset_7),
	.i_valid(PE_next_valid_7),
	.i_features_0_0(daisy_chain_7_0_0),
	.o_features_0_0(daisy_chain_8_0_0),
	.i_features_0_1(daisy_chain_7_0_1),
	.o_features_0_1(daisy_chain_8_0_1),
	.i_features_0_2(daisy_chain_7_0_2),
	.o_features_0_2(daisy_chain_8_0_2),
	.i_features_0_3(daisy_chain_7_0_3),
	.o_features_0_3(daisy_chain_8_0_3),
	.i_features_0_4(daisy_chain_7_0_4),
	.o_features_0_4(daisy_chain_8_0_4),
	.i_features_0_5(daisy_chain_7_0_5),
	.o_features_0_5(daisy_chain_8_0_5),
	.i_features_1_0(daisy_chain_7_1_0),
	.o_features_1_0(daisy_chain_8_1_0),
	.i_features_1_1(daisy_chain_7_1_1),
	.o_features_1_1(daisy_chain_8_1_1),
	.i_features_1_2(daisy_chain_7_1_2),
	.o_features_1_2(daisy_chain_8_1_2),
	.i_features_1_3(daisy_chain_7_1_3),
	.o_features_1_3(daisy_chain_8_1_3),
	.i_features_1_4(daisy_chain_7_1_4),
	.o_features_1_4(daisy_chain_8_1_4),
	.i_features_1_5(daisy_chain_7_1_5),
	.o_features_1_5(daisy_chain_8_1_5),
	.o_result_0(PE_output_8_0),
	.o_result_1(PE_output_8_1),
	.o_result_2(PE_output_8_2),
	.o_result_3(PE_output_8_3),
	.o_result_4(PE_output_8_4),
	.o_result_5(PE_output_8_5),
	.o_valid(PE_valid_8),
	.o_next_reset(PE_next_reset_8),
	.o_next_valid(PE_next_valid_8)
);

processing_element processing_element_inst_9 (
	.clk(clk),
	.i_reset(PE_next_reset_8),
	.i_valid(PE_next_valid_8),
	.i_features_0_0(daisy_chain_8_0_0),
	.o_features_0_0(daisy_chain_9_0_0),
	.i_features_0_1(daisy_chain_8_0_1),
	.o_features_0_1(daisy_chain_9_0_1),
	.i_features_0_2(daisy_chain_8_0_2),
	.o_features_0_2(daisy_chain_9_0_2),
	.i_features_0_3(daisy_chain_8_0_3),
	.o_features_0_3(daisy_chain_9_0_3),
	.i_features_0_4(daisy_chain_8_0_4),
	.o_features_0_4(daisy_chain_9_0_4),
	.i_features_0_5(daisy_chain_8_0_5),
	.o_features_0_5(daisy_chain_9_0_5),
	.i_features_1_0(daisy_chain_8_1_0),
	.o_features_1_0(daisy_chain_9_1_0),
	.i_features_1_1(daisy_chain_8_1_1),
	.o_features_1_1(daisy_chain_9_1_1),
	.i_features_1_2(daisy_chain_8_1_2),
	.o_features_1_2(daisy_chain_9_1_2),
	.i_features_1_3(daisy_chain_8_1_3),
	.o_features_1_3(daisy_chain_9_1_3),
	.i_features_1_4(daisy_chain_8_1_4),
	.o_features_1_4(daisy_chain_9_1_4),
	.i_features_1_5(daisy_chain_8_1_5),
	.o_features_1_5(daisy_chain_9_1_5),
	.o_result_0(PE_output_9_0),
	.o_result_1(PE_output_9_1),
	.o_result_2(PE_output_9_2),
	.o_result_3(PE_output_9_3),
	.o_result_4(PE_output_9_4),
	.o_result_5(PE_output_9_5),
	.o_valid(PE_valid_9),
	.o_next_reset(PE_next_reset_9),
	.o_next_valid(PE_next_valid_9)
);

processing_element processing_element_inst_10 (
	.clk(clk),
	.i_reset(PE_next_reset_9),
	.i_valid(PE_next_valid_9),
	.i_features_0_0(daisy_chain_9_0_0),
	.o_features_0_0(daisy_chain_10_0_0),
	.i_features_0_1(daisy_chain_9_0_1),
	.o_features_0_1(daisy_chain_10_0_1),
	.i_features_0_2(daisy_chain_9_0_2),
	.o_features_0_2(daisy_chain_10_0_2),
	.i_features_0_3(daisy_chain_9_0_3),
	.o_features_0_3(daisy_chain_10_0_3),
	.i_features_0_4(daisy_chain_9_0_4),
	.o_features_0_4(daisy_chain_10_0_4),
	.i_features_0_5(daisy_chain_9_0_5),
	.o_features_0_5(daisy_chain_10_0_5),
	.i_features_1_0(daisy_chain_9_1_0),
	.o_features_1_0(daisy_chain_10_1_0),
	.i_features_1_1(daisy_chain_9_1_1),
	.o_features_1_1(daisy_chain_10_1_1),
	.i_features_1_2(daisy_chain_9_1_2),
	.o_features_1_2(daisy_chain_10_1_2),
	.i_features_1_3(daisy_chain_9_1_3),
	.o_features_1_3(daisy_chain_10_1_3),
	.i_features_1_4(daisy_chain_9_1_4),
	.o_features_1_4(daisy_chain_10_1_4),
	.i_features_1_5(daisy_chain_9_1_5),
	.o_features_1_5(daisy_chain_10_1_5),
	.o_result_0(PE_output_10_0),
	.o_result_1(PE_output_10_1),
	.o_result_2(PE_output_10_2),
	.o_result_3(PE_output_10_3),
	.o_result_4(PE_output_10_4),
	.o_result_5(PE_output_10_5),
	.o_valid(PE_valid_10),
	.o_next_reset(PE_next_reset_10),
	.o_next_valid(PE_next_valid_10)
);

processing_element processing_element_inst_11 (
	.clk(clk),
	.i_reset(PE_next_reset_10),
	.i_valid(PE_next_valid_10),
	.i_features_0_0(daisy_chain_10_0_0),
	.o_features_0_0(daisy_chain_11_0_0),
	.i_features_0_1(daisy_chain_10_0_1),
	.o_features_0_1(daisy_chain_11_0_1),
	.i_features_0_2(daisy_chain_10_0_2),
	.o_features_0_2(daisy_chain_11_0_2),
	.i_features_0_3(daisy_chain_10_0_3),
	.o_features_0_3(daisy_chain_11_0_3),
	.i_features_0_4(daisy_chain_10_0_4),
	.o_features_0_4(daisy_chain_11_0_4),
	.i_features_0_5(daisy_chain_10_0_5),
	.o_features_0_5(daisy_chain_11_0_5),
	.i_features_1_0(daisy_chain_10_1_0),
	.o_features_1_0(daisy_chain_11_1_0),
	.i_features_1_1(daisy_chain_10_1_1),
	.o_features_1_1(daisy_chain_11_1_1),
	.i_features_1_2(daisy_chain_10_1_2),
	.o_features_1_2(daisy_chain_11_1_2),
	.i_features_1_3(daisy_chain_10_1_3),
	.o_features_1_3(daisy_chain_11_1_3),
	.i_features_1_4(daisy_chain_10_1_4),
	.o_features_1_4(daisy_chain_11_1_4),
	.i_features_1_5(daisy_chain_10_1_5),
	.o_features_1_5(daisy_chain_11_1_5),
	.o_result_0(PE_output_11_0),
	.o_result_1(PE_output_11_1),
	.o_result_2(PE_output_11_2),
	.o_result_3(PE_output_11_3),
	.o_result_4(PE_output_11_4),
	.o_result_5(PE_output_11_5),
	.o_valid(PE_valid_11),
	.o_next_reset(PE_next_reset_11),
	.o_next_valid(PE_next_valid_11)
);

inverse_winograd_0 inverse_winograd_0_inst (
	.clk(clk),
	.i_valid(PE_valid_0),
	.i_result_0(PE_output_0_0),
	.i_result_1(PE_output_0_1),
	.i_result_2(PE_output_0_2),
	.i_result_3(PE_output_0_3),
	.i_result_4(PE_output_0_4),
	.i_result_5(PE_output_0_5),
	.o_result_0(INV_output_0_0),
	.o_result_1(INV_output_0_1),
	.o_result_2(INV_output_0_2),
	.o_result_3(INV_output_0_3),
	.o_valid(INV_valid_0)
);

inverse_winograd_1 inverse_winograd_1_inst (
	.clk(clk),
	.i_valid(PE_valid_1),
	.i_result_0(PE_output_1_0),
	.i_result_1(PE_output_1_1),
	.i_result_2(PE_output_1_2),
	.i_result_3(PE_output_1_3),
	.i_result_4(PE_output_1_4),
	.i_result_5(PE_output_1_5),
	.o_result_0(INV_output_1_0),
	.o_result_1(INV_output_1_1),
	.o_result_2(INV_output_1_2),
	.o_result_3(INV_output_1_3),
	.o_valid(INV_valid_1)
);

inverse_winograd_2 inverse_winograd_2_inst (
	.clk(clk),
	.i_valid(PE_valid_2),
	.i_result_0(PE_output_2_0),
	.i_result_1(PE_output_2_1),
	.i_result_2(PE_output_2_2),
	.i_result_3(PE_output_2_3),
	.i_result_4(PE_output_2_4),
	.i_result_5(PE_output_2_5),
	.o_result_0(INV_output_2_0),
	.o_result_1(INV_output_2_1),
	.o_result_2(INV_output_2_2),
	.o_result_3(INV_output_2_3),
	.o_valid(INV_valid_2)
);

inverse_winograd_3 inverse_winograd_3_inst (
	.clk(clk),
	.i_valid(PE_valid_3),
	.i_result_0(PE_output_3_0),
	.i_result_1(PE_output_3_1),
	.i_result_2(PE_output_3_2),
	.i_result_3(PE_output_3_3),
	.i_result_4(PE_output_3_4),
	.i_result_5(PE_output_3_5),
	.o_result_0(INV_output_3_0),
	.o_result_1(INV_output_3_1),
	.o_result_2(INV_output_3_2),
	.o_result_3(INV_output_3_3),
	.o_valid(INV_valid_3)
);

inverse_winograd_4 inverse_winograd_4_inst (
	.clk(clk),
	.i_valid(PE_valid_4),
	.i_result_0(PE_output_4_0),
	.i_result_1(PE_output_4_1),
	.i_result_2(PE_output_4_2),
	.i_result_3(PE_output_4_3),
	.i_result_4(PE_output_4_4),
	.i_result_5(PE_output_4_5),
	.o_result_0(INV_output_4_0),
	.o_result_1(INV_output_4_1),
	.o_result_2(INV_output_4_2),
	.o_result_3(INV_output_4_3),
	.o_valid(INV_valid_4)
);

inverse_winograd_5 inverse_winograd_5_inst (
	.clk(clk),
	.i_valid(PE_valid_5),
	.i_result_0(PE_output_5_0),
	.i_result_1(PE_output_5_1),
	.i_result_2(PE_output_5_2),
	.i_result_3(PE_output_5_3),
	.i_result_4(PE_output_5_4),
	.i_result_5(PE_output_5_5),
	.o_result_0(INV_output_5_0),
	.o_result_1(INV_output_5_1),
	.o_result_2(INV_output_5_2),
	.o_result_3(INV_output_5_3),
	.o_valid(INV_valid_5)
);

inverse_winograd_6 inverse_winograd_6_inst (
	.clk(clk),
	.i_valid(PE_valid_6),
	.i_result_0(PE_output_6_0),
	.i_result_1(PE_output_6_1),
	.i_result_2(PE_output_6_2),
	.i_result_3(PE_output_6_3),
	.i_result_4(PE_output_6_4),
	.i_result_5(PE_output_6_5),
	.o_result_0(INV_output_6_0),
	.o_result_1(INV_output_6_1),
	.o_result_2(INV_output_6_2),
	.o_result_3(INV_output_6_3),
	.o_valid(INV_valid_6)
);

inverse_winograd_7 inverse_winograd_7_inst (
	.clk(clk),
	.i_valid(PE_valid_7),
	.i_result_0(PE_output_7_0),
	.i_result_1(PE_output_7_1),
	.i_result_2(PE_output_7_2),
	.i_result_3(PE_output_7_3),
	.i_result_4(PE_output_7_4),
	.i_result_5(PE_output_7_5),
	.o_result_0(INV_output_7_0),
	.o_result_1(INV_output_7_1),
	.o_result_2(INV_output_7_2),
	.o_result_3(INV_output_7_3),
	.o_valid(INV_valid_7)
);

inverse_winograd_8 inverse_winograd_8_inst (
	.clk(clk),
	.i_valid(PE_valid_8),
	.i_result_0(PE_output_8_0),
	.i_result_1(PE_output_8_1),
	.i_result_2(PE_output_8_2),
	.i_result_3(PE_output_8_3),
	.i_result_4(PE_output_8_4),
	.i_result_5(PE_output_8_5),
	.o_result_0(INV_output_8_0),
	.o_result_1(INV_output_8_1),
	.o_result_2(INV_output_8_2),
	.o_result_3(INV_output_8_3),
	.o_valid(INV_valid_8)
);

inverse_winograd_9 inverse_winograd_9_inst (
	.clk(clk),
	.i_valid(PE_valid_9),
	.i_result_0(PE_output_9_0),
	.i_result_1(PE_output_9_1),
	.i_result_2(PE_output_9_2),
	.i_result_3(PE_output_9_3),
	.i_result_4(PE_output_9_4),
	.i_result_5(PE_output_9_5),
	.o_result_0(INV_output_9_0),
	.o_result_1(INV_output_9_1),
	.o_result_2(INV_output_9_2),
	.o_result_3(INV_output_9_3),
	.o_valid(INV_valid_9)
);

inverse_winograd_10 inverse_winograd_10_inst (
	.clk(clk),
	.i_valid(PE_valid_10),
	.i_result_0(PE_output_10_0),
	.i_result_1(PE_output_10_1),
	.i_result_2(PE_output_10_2),
	.i_result_3(PE_output_10_3),
	.i_result_4(PE_output_10_4),
	.i_result_5(PE_output_10_5),
	.o_result_0(INV_output_10_0),
	.o_result_1(INV_output_10_1),
	.o_result_2(INV_output_10_2),
	.o_result_3(INV_output_10_3),
	.o_valid(INV_valid_10)
);

inverse_winograd_11 inverse_winograd_11_inst (
	.clk(clk),
	.i_valid(PE_valid_11),
	.i_result_0(PE_output_11_0),
	.i_result_1(PE_output_11_1),
	.i_result_2(PE_output_11_2),
	.i_result_3(PE_output_11_3),
	.i_result_4(PE_output_11_4),
	.i_result_5(PE_output_11_5),
	.o_result_0(INV_output_11_0),
	.o_result_1(INV_output_11_1),
	.o_result_2(INV_output_11_2),
	.o_result_3(INV_output_11_3),
	.o_valid(INV_valid_11)
);

pooling pooling_inst_0 (
	.clk(clk),
	.i_reset(i_reset),
	.i_valid(INV_valid_0),
	.i_result_0(INV_output_0_0),
	.i_result_1(INV_output_0_1),
	.i_result_2(INV_output_0_2),
	.i_result_3(INV_output_0_3),
	.o_result(POOL_output_0),
	.o_valid(POOL_valid_0)
);

pooling pooling_inst_1 (
	.clk(clk),
	.i_reset(i_reset),
	.i_valid(INV_valid_1),
	.i_result_0(INV_output_1_0),
	.i_result_1(INV_output_1_1),
	.i_result_2(INV_output_1_2),
	.i_result_3(INV_output_1_3),
	.o_result(POOL_output_1),
	.o_valid(POOL_valid_1)
);

pooling pooling_inst_2 (
	.clk(clk),
	.i_reset(i_reset),
	.i_valid(INV_valid_2),
	.i_result_0(INV_output_2_0),
	.i_result_1(INV_output_2_1),
	.i_result_2(INV_output_2_2),
	.i_result_3(INV_output_2_3),
	.o_result(POOL_output_2),
	.o_valid(POOL_valid_2)
);

pooling pooling_inst_3 (
	.clk(clk),
	.i_reset(i_reset),
	.i_valid(INV_valid_3),
	.i_result_0(INV_output_3_0),
	.i_result_1(INV_output_3_1),
	.i_result_2(INV_output_3_2),
	.i_result_3(INV_output_3_3),
	.o_result(POOL_output_3),
	.o_valid(POOL_valid_3)
);

pooling pooling_inst_4 (
	.clk(clk),
	.i_reset(i_reset),
	.i_valid(INV_valid_4),
	.i_result_0(INV_output_4_0),
	.i_result_1(INV_output_4_1),
	.i_result_2(INV_output_4_2),
	.i_result_3(INV_output_4_3),
	.o_result(POOL_output_4),
	.o_valid(POOL_valid_4)
);

pooling pooling_inst_5 (
	.clk(clk),
	.i_reset(i_reset),
	.i_valid(INV_valid_5),
	.i_result_0(INV_output_5_0),
	.i_result_1(INV_output_5_1),
	.i_result_2(INV_output_5_2),
	.i_result_3(INV_output_5_3),
	.o_result(POOL_output_5),
	.o_valid(POOL_valid_5)
);

pooling pooling_inst_6 (
	.clk(clk),
	.i_reset(i_reset),
	.i_valid(INV_valid_6),
	.i_result_0(INV_output_6_0),
	.i_result_1(INV_output_6_1),
	.i_result_2(INV_output_6_2),
	.i_result_3(INV_output_6_3),
	.o_result(POOL_output_6),
	.o_valid(POOL_valid_6)
);

pooling pooling_inst_7 (
	.clk(clk),
	.i_reset(i_reset),
	.i_valid(INV_valid_7),
	.i_result_0(INV_output_7_0),
	.i_result_1(INV_output_7_1),
	.i_result_2(INV_output_7_2),
	.i_result_3(INV_output_7_3),
	.o_result(POOL_output_7),
	.o_valid(POOL_valid_7)
);

pooling pooling_inst_8 (
	.clk(clk),
	.i_reset(i_reset),
	.i_valid(INV_valid_8),
	.i_result_0(INV_output_8_0),
	.i_result_1(INV_output_8_1),
	.i_result_2(INV_output_8_2),
	.i_result_3(INV_output_8_3),
	.o_result(POOL_output_8),
	.o_valid(POOL_valid_8)
);

pooling pooling_inst_9 (
	.clk(clk),
	.i_reset(i_reset),
	.i_valid(INV_valid_9),
	.i_result_0(INV_output_9_0),
	.i_result_1(INV_output_9_1),
	.i_result_2(INV_output_9_2),
	.i_result_3(INV_output_9_3),
	.o_result(POOL_output_9),
	.o_valid(POOL_valid_9)
);

pooling pooling_inst_10 (
	.clk(clk),
	.i_reset(i_reset),
	.i_valid(INV_valid_10),
	.i_result_0(INV_output_10_0),
	.i_result_1(INV_output_10_1),
	.i_result_2(INV_output_10_2),
	.i_result_3(INV_output_10_3),
	.o_result(POOL_output_10),
	.o_valid(POOL_valid_10)
);

pooling pooling_inst_11 (
	.clk(clk),
	.i_reset(i_reset),
	.i_valid(INV_valid_11),
	.i_result_0(INV_output_11_0),
	.i_result_1(INV_output_11_1),
	.i_result_2(INV_output_11_2),
	.i_result_3(INV_output_11_3),
	.o_result(POOL_output_11),
	.o_valid(POOL_valid_11)
);

store_output store_output_inst (
	.clk(clk),
	.i_valid(POOL_valid_0),
	.i_reset(i_reset),
	.i_result_0(POOL_output_0),
	.i_result_1(POOL_output_1),
	.i_result_2(POOL_output_2),
	.i_result_3(POOL_output_3),
	.i_result_4(POOL_output_4),
	.i_result_5(POOL_output_5),
	.i_result_6(POOL_output_6),
	.i_result_7(POOL_output_7),
	.i_result_8(POOL_output_8),
	.i_result_9(POOL_output_9),
	.i_result_10(POOL_output_10),
	.i_result_11(POOL_output_11),
	.o_store_0_0(STORE_output_0_0),
	.o_store_0_1(STORE_output_0_1),
	.o_store_1_0(STORE_output_1_0),
	.o_store_1_1(STORE_output_1_1),
	.o_store_2_0(STORE_output_2_0),
	.o_store_2_1(STORE_output_2_1),
	.o_store_3_0(STORE_output_3_0),
	.o_store_3_1(STORE_output_3_1),
	.o_store_4_0(STORE_output_4_0),
	.o_store_4_1(STORE_output_4_1),
	.o_store_5_0(STORE_output_5_0),
	.o_store_5_1(STORE_output_5_1),
	.o_wen_0(STORE_wen_0),
	.o_wen_1(STORE_wen_1),
	.o_wen_2(STORE_wen_2),
	.o_wen_3(STORE_wen_3),
	.o_wen_4(STORE_wen_4),
	.o_wen_5(STORE_wen_5),
	.o_addr(STORE_addr)
);

signal_width_reducer signal_width_reducer_inst (
	.clk(clk),
	.signals_0_0(dummy_out_0_0),
	.reduced_signals_0_0(o_dummy_out_0_0),
	.signals_0_1(dummy_out_0_1),
	.reduced_signals_0_1(o_dummy_out_0_1),
	.signals_1_0(dummy_out_1_0),
	.reduced_signals_1_0(o_dummy_out_1_0),
	.signals_1_1(dummy_out_1_1),
	.reduced_signals_1_1(o_dummy_out_1_1),
	.signals_2_0(dummy_out_2_0),
	.reduced_signals_2_0(o_dummy_out_2_0),
	.signals_2_1(dummy_out_2_1),
	.reduced_signals_2_1(o_dummy_out_2_1),
	.signals_3_0(dummy_out_3_0),
	.reduced_signals_3_0(o_dummy_out_3_0),
	.signals_3_1(dummy_out_3_1),
	.reduced_signals_3_1(o_dummy_out_3_1),
	.signals_4_0(dummy_out_4_0),
	.reduced_signals_4_0(o_dummy_out_4_0),
	.signals_4_1(dummy_out_4_1),
	.reduced_signals_4_1(o_dummy_out_4_1),
	.signals_5_0(dummy_out_5_0),
	.reduced_signals_5_0(o_dummy_out_5_0),
	.signals_5_1(dummy_out_5_1),
	.reduced_signals_5_1(o_dummy_out_5_1),
	.reset(i_reset)
);

assign o_valid = POOL_valid_0;

endmodule

module inverse_winograd_1 (
	input clk,
	input i_valid,
	input [29:0] i_result_0,
	input [29:0] i_result_1,
	input [29:0] i_result_2,
	input [29:0] i_result_3,
	input [29:0] i_result_4,
	input [29:0] i_result_5,
	output [15:0] o_result_0,
	output [15:0] o_result_1,
	output [15:0] o_result_2,
	output [15:0] o_result_3,
	output o_valid
);

reg [2:0] state, next_state;
wire [29:0] adders_res_0_0;
reg [29:0] serialize_reg_0_0;
reg [29:0] adders_res_0_1;
reg [29:0] serialize_reg_0_1;
reg [29:0] adders_res_0_2;
reg [29:0] serialize_reg_0_2;
reg [29:0] adders_res_0_3;
reg [29:0] serialize_reg_0_3;
reg [29:0] adders_res_0_4;
reg [29:0] serialize_reg_0_4;
reg [29:0] adders_res_0_5;
reg [29:0] serialize_reg_0_5;
wire [29:0] adders_res_1_0;
reg [29:0] serialize_reg_1_0;
reg [29:0] adders_res_1_1;
reg [29:0] serialize_reg_1_1;
reg [29:0] adders_res_1_2;
reg [29:0] serialize_reg_1_2;
reg [29:0] adders_res_1_3;
reg [29:0] serialize_reg_1_3;
reg [29:0] adders_res_1_4;
reg [29:0] serialize_reg_1_4;
reg [29:0] adders_res_1_5;
reg [29:0] serialize_reg_1_5;
wire [29:0] adders_res_2_0;
reg [29:0] serialize_reg_2_0;
reg [29:0] adders_res_2_1;
reg [29:0] serialize_reg_2_1;
reg [29:0] adders_res_2_2;
reg [29:0] serialize_reg_2_2;
reg [29:0] adders_res_2_3;
reg [29:0] serialize_reg_2_3;
reg [29:0] adders_res_2_4;
reg [29:0] serialize_reg_2_4;
reg [29:0] adders_res_2_5;
reg [29:0] serialize_reg_2_5;
wire [29:0] adders_res_3_0;
reg [29:0] serialize_reg_3_0;
reg [29:0] adders_res_3_1;
reg [29:0] serialize_reg_3_1;
reg [29:0] adders_res_3_2;
reg [29:0] serialize_reg_3_2;
reg [29:0] adders_res_3_3;
reg [29:0] serialize_reg_3_3;
reg [29:0] adders_res_3_4;
reg [29:0] serialize_reg_3_4;
reg [29:0] adders_res_3_5;
reg [29:0] serialize_reg_3_5;

wire [29:0] result_reg_0_0;
wire [29:0] result_reg_0_1;
wire [29:0] result_reg_0_2;
wire [29:0] result_reg_0_3;
reg [29:0] result_reg_1_0;
reg [29:0] result_reg_1_1;
reg [29:0] result_reg_1_2;
reg [29:0] result_reg_1_3;
reg [29:0] result_reg_2_0;
reg [29:0] result_reg_2_1;
reg [29:0] result_reg_2_2;
reg [29:0] result_reg_2_3;
reg [29:0] result_reg_3_0;
reg [29:0] result_reg_3_1;
reg [29:0] result_reg_3_2;
reg [29:0] result_reg_3_3;
reg [29:0] result_reg_4_0;
reg [29:0] result_reg_4_1;
reg [29:0] result_reg_4_2;
reg [29:0] result_reg_4_3;
reg [29:0] result_reg_5_0;
reg [29:0] result_reg_5_1;
reg [29:0] result_reg_5_2;
reg [29:0] result_reg_5_3;
reg [29:0] result_reg_6_0;
reg [29:0] result_reg_6_1;
reg [29:0] result_reg_6_2;
reg [29:0] result_reg_6_3;
reg [29:0] result_reg_7_0;
reg [29:0] result_reg_7_1;
reg [29:0] result_reg_7_2;
reg [29:0] result_reg_7_3;
reg [29:0] result_reg_8_0;
reg [29:0] result_reg_8_1;
reg [29:0] result_reg_8_2;
reg [29:0] result_reg_8_3;
reg [29:0] result_reg_9_0;
reg [29:0] result_reg_9_1;
reg [29:0] result_reg_9_2;
reg [29:0] result_reg_9_3;
reg [29:0] result_reg_10_0;
reg [29:0] result_reg_10_1;
reg [29:0] result_reg_10_2;
reg [29:0] result_reg_10_3;
reg [29:0] result_reg_11_0;
reg [29:0] result_reg_11_1;
reg [29:0] result_reg_11_2;
reg [29:0] result_reg_11_3;
reg [29:0] result_reg_12_0;
reg [29:0] result_reg_12_1;
reg [29:0] result_reg_12_2;
reg [29:0] result_reg_12_3;
reg [29:0] result_reg_13_0;
reg [29:0] result_reg_13_1;
reg [29:0] result_reg_13_2;
reg [29:0] result_reg_13_3;
reg [29:0] result_reg_14_0;
reg [29:0] result_reg_14_1;
reg [29:0] result_reg_14_2;
reg [29:0] result_reg_14_3;
reg [29:0] result_reg_15_0;
reg [29:0] result_reg_15_1;
reg [29:0] result_reg_15_2;
reg [29:0] result_reg_15_3;

reg out_valid_0;
reg out_valid_1;
reg out_valid_2;
reg out_valid_3;
reg out_valid_4;
reg out_valid_5;
reg out_valid_6;
reg out_valid_7;
reg out_valid_8;
reg out_valid_9;
reg out_valid_10;
reg out_valid_11;
reg out_valid_12;
reg out_valid_13;
reg out_valid_14;
reg out_valid_15;
reg [1:0] serialize_count;

always @ (posedge clk) begin
	out_valid_1 <= out_valid_0;
	result_reg_1_0 <= result_reg_0_0;
	result_reg_1_1 <= result_reg_0_1;
	result_reg_1_2 <= result_reg_0_2;
	result_reg_1_3 <= result_reg_0_3;
	out_valid_2 <= out_valid_1;
	result_reg_2_0 <= result_reg_1_0;
	result_reg_2_1 <= result_reg_1_1;
	result_reg_2_2 <= result_reg_1_2;
	result_reg_2_3 <= result_reg_1_3;
	out_valid_3 <= out_valid_2;
	result_reg_3_0 <= result_reg_2_0;
	result_reg_3_1 <= result_reg_2_1;
	result_reg_3_2 <= result_reg_2_2;
	result_reg_3_3 <= result_reg_2_3;
	out_valid_4 <= out_valid_3;
	result_reg_4_0 <= result_reg_3_0;
	result_reg_4_1 <= result_reg_3_1;
	result_reg_4_2 <= result_reg_3_2;
	result_reg_4_3 <= result_reg_3_3;
	out_valid_5 <= out_valid_4;
	result_reg_5_0 <= result_reg_4_0;
	result_reg_5_1 <= result_reg_4_1;
	result_reg_5_2 <= result_reg_4_2;
	result_reg_5_3 <= result_reg_4_3;
	out_valid_6 <= out_valid_5;
	result_reg_6_0 <= result_reg_5_0;
	result_reg_6_1 <= result_reg_5_1;
	result_reg_6_2 <= result_reg_5_2;
	result_reg_6_3 <= result_reg_5_3;
	out_valid_7 <= out_valid_6;
	result_reg_7_0 <= result_reg_6_0;
	result_reg_7_1 <= result_reg_6_1;
	result_reg_7_2 <= result_reg_6_2;
	result_reg_7_3 <= result_reg_6_3;
	out_valid_8 <= out_valid_7;
	result_reg_8_0 <= result_reg_7_0;
	result_reg_8_1 <= result_reg_7_1;
	result_reg_8_2 <= result_reg_7_2;
	result_reg_8_3 <= result_reg_7_3;
	out_valid_9 <= out_valid_8;
	result_reg_9_0 <= result_reg_8_0;
	result_reg_9_1 <= result_reg_8_1;
	result_reg_9_2 <= result_reg_8_2;
	result_reg_9_3 <= result_reg_8_3;
	out_valid_10 <= out_valid_9;
	result_reg_10_0 <= result_reg_9_0;
	result_reg_10_1 <= result_reg_9_1;
	result_reg_10_2 <= result_reg_9_2;
	result_reg_10_3 <= result_reg_9_3;
	out_valid_11 <= out_valid_10;
	result_reg_11_0 <= result_reg_10_0;
	result_reg_11_1 <= result_reg_10_1;
	result_reg_11_2 <= result_reg_10_2;
	result_reg_11_3 <= result_reg_10_3;
	out_valid_12 <= out_valid_11;
	result_reg_12_0 <= result_reg_11_0;
	result_reg_12_1 <= result_reg_11_1;
	result_reg_12_2 <= result_reg_11_2;
	result_reg_12_3 <= result_reg_11_3;
	out_valid_13 <= out_valid_12;
	result_reg_13_0 <= result_reg_12_0;
	result_reg_13_1 <= result_reg_12_1;
	result_reg_13_2 <= result_reg_12_2;
	result_reg_13_3 <= result_reg_12_3;
	out_valid_14 <= out_valid_13;
	result_reg_14_0 <= result_reg_13_0;
	result_reg_14_1 <= result_reg_13_1;
	result_reg_14_2 <= result_reg_13_2;
	result_reg_14_3 <= result_reg_13_3;
	out_valid_15 <= out_valid_14;
	result_reg_15_0 <= result_reg_14_0;
	result_reg_15_1 <= result_reg_14_1;
	result_reg_15_2 <= result_reg_14_2;
	result_reg_15_3 <= result_reg_14_3;
end

// FSM Combinational Logic
always @ (state or i_valid or serialize_count) begin
	if(state == 3'b000) begin
		if(i_valid == 1'b1)begin
			next_state = 3'b010;
		end else begin
			next_state = 3'b000;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b0) begin
			next_state = 3'b100;
		end else begin
			next_state = 3'b010;
		end
	end else if (state == 3'b100) begin
		if(serialize_count == 3)begin
			next_state = 3'b000;
		end else begin
			next_state = 3'b100;
		end
	end else begin
		next_state = 3'b000;
	end
end

// FSM Sequential Logic
always @ (posedge clk) begin
	if (state == 3'b000) begin
		serialize_count <= 0;
		out_valid_0 <= 0;
		if (i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end else begin
			serialize_reg_0_0 <= adders_res_0_0;
			serialize_reg_1_0 <= adders_res_1_0;
			serialize_reg_2_0 <= adders_res_2_0;
			serialize_reg_3_0 <= adders_res_3_0;
			serialize_reg_0_1 <= adders_res_0_1;
			serialize_reg_1_1 <= adders_res_1_1;
			serialize_reg_2_1 <= adders_res_2_1;
			serialize_reg_3_1 <= adders_res_3_1;
			serialize_reg_0_2 <= adders_res_0_2;
			serialize_reg_1_2 <= adders_res_1_2;
			serialize_reg_2_2 <= adders_res_2_2;
			serialize_reg_3_2 <= adders_res_3_2;
			serialize_reg_0_3 <= adders_res_0_3;
			serialize_reg_1_3 <= adders_res_1_3;
			serialize_reg_2_3 <= adders_res_2_3;
			serialize_reg_3_3 <= adders_res_3_3;
			serialize_reg_0_4 <= adders_res_0_4;
			serialize_reg_1_4 <= adders_res_1_4;
			serialize_reg_2_4 <= adders_res_2_4;
			serialize_reg_3_4 <= adders_res_3_4;
			serialize_reg_0_5 <= adders_res_0_5;
			serialize_reg_1_5 <= adders_res_1_5;
			serialize_reg_2_5 <= adders_res_2_5;
			serialize_reg_3_5 <= adders_res_3_5;
		end
	end else if (state == 3'b100) begin
		if (serialize_count < 3) begin
			serialize_reg_0_0 <= serialize_reg_1_0;
			serialize_reg_0_1 <= serialize_reg_1_1;
			serialize_reg_0_2 <= serialize_reg_1_2;
			serialize_reg_0_3 <= serialize_reg_1_3;
			serialize_reg_0_4 <= serialize_reg_1_4;
			serialize_reg_0_5 <= serialize_reg_1_5;
			serialize_reg_1_0 <= serialize_reg_2_0;
			serialize_reg_1_1 <= serialize_reg_2_1;
			serialize_reg_1_2 <= serialize_reg_2_2;
			serialize_reg_1_3 <= serialize_reg_2_3;
			serialize_reg_1_4 <= serialize_reg_2_4;
			serialize_reg_1_5 <= serialize_reg_2_5;
			serialize_reg_2_0 <= serialize_reg_3_0;
			serialize_reg_2_1 <= serialize_reg_3_1;
			serialize_reg_2_2 <= serialize_reg_3_2;
			serialize_reg_2_3 <= serialize_reg_3_3;
			serialize_reg_2_4 <= serialize_reg_3_4;
			serialize_reg_2_5 <= serialize_reg_3_5;
			serialize_count <= serialize_count + 1'b1;
			out_valid_0 <= 1;
		end
	end
	state <=  next_state;
end

// AT Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA0 (
	.data0x(i_result_0),
	.data1x(i_result_1),
	.data2x(i_result_2),
	.data3x(i_result_3),
	.data4x(i_result_4),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(adders_res_0_0)
);
wire [29:0] f1, f2;
assign f1 = -i_result_2;
assign f2 = -{i_result_4[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA1 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f1),
	.data3x({i_result_3[28:0], 1'b0}),
	.data4x(f2),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_1_0)
);
wire [29:0] f3, f4;
assign f3 = -i_result_2;
assign f4 = -{i_result_4[27:0], 2'b00};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA2 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f3),
	.data3x({i_result_3[27:0], 2'b00}),
	.data4x(f4),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_2_0)
);
wire [29:0] f5, f6;
assign f5 = -i_result_2;
assign f6 = -{i_result_4[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA3 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f5),
	.data3x({i_result_3[26:0], 3'b000}),
	.data4x(f6),
	.data5x(i_result_5),
	.clock(clk),
	.result(adders_res_3_0)
);

// A Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT0 (
	.data0x(serialize_reg_0_5),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x(serialize_reg_0_2),
	.data4x(serialize_reg_0_1),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(result_reg_0_0)
);
wire [29:0] f7, f8;
assign f7 = -serialize_reg_0_3;
assign f8 = -{serialize_reg_0_1[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT1 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f7),
	.data3x({serialize_reg_0_2[28:0], 1'b0}),
	.data4x(f8),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_1)
);
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT2 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x({serialize_reg_0_2[27:0], 2'b00}),
	.data4x({serialize_reg_0_1[27:0], 2'b00}),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_2)
);
wire [29:0] f9, f10;
assign f9 = -serialize_reg_0_3;
assign f10 = -{serialize_reg_0_1[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT3 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f9),
	.data3x({serialize_reg_0_2[26:0], 3'b000}),
	.data4x(f10),
	.data5x(serialize_reg_0_0),
	.clock(clk),
	.result(result_reg_0_3)
);

reg [15:0] result_wire_0;
reg [15:0] result_wire_1;
reg [15:0] result_wire_2;
reg [15:0] result_wire_3;

always @ (*) begin
	if(result_reg_15_0[29] == 1'b0) begin
		result_wire_0 <= result_reg_15_0[23:8];
	end else begin
		result_wire_0 <= { (16){1'b0} };
	end
	if(result_reg_15_1[29] == 1'b0) begin
		result_wire_1 <= result_reg_15_1[23:8];
	end else begin
		result_wire_1 <= { (16){1'b0} };
	end
	if(result_reg_15_2[29] == 1'b0) begin
		result_wire_2 <= result_reg_15_2[23:8];
	end else begin
		result_wire_2 <= { (16){1'b0} };
	end
	if(result_reg_15_3[29] == 1'b0) begin
		result_wire_3 <= result_reg_15_3[23:8];
	end else begin
		result_wire_3 <= { (16){1'b0} };
	end
end

assign o_valid = out_valid_15;
assign o_result_0 = result_wire_0;
assign o_result_1 = result_wire_1;
assign o_result_2 = result_wire_2;
assign o_result_3 = result_wire_3;

endmodule

module inverse_winograd_adder_30_3 (
	input clock,
	input [29:0] data0x,
	input [29:0] data1x,
	input [29:0] data2x,
	input [29:0] data3x,
	input [29:0] data4x,
	input [29:0] data5x,
	output [29:0] result
);

reg [32:0] pipeline_0_0;
reg [32:0] pipeline_0_1;
reg [32:0] pipeline_0_2;
reg [32:0] pipeline_1_0;
reg [32:0] pipeline_1_1;
reg [32:0] pipeline_2_0;

always @ (posedge clock) begin
	pipeline_0_0 <= data0x + data1x;
	pipeline_0_1 <= data2x + data3x;
	pipeline_0_2 <= data4x + data5x;
	pipeline_1_0 <= pipeline_0_0 + pipeline_0_1;
	pipeline_1_1 <= pipeline_0_2;
	pipeline_2_0 <= pipeline_1_0 + pipeline_1_1;
end

assign result = pipeline_2_0;

endmodule

module inverse_winograd_0 (
	input clk,
	input i_valid,
	input [29:0] i_result_0,
	input [29:0] i_result_1,
	input [29:0] i_result_2,
	input [29:0] i_result_3,
	input [29:0] i_result_4,
	input [29:0] i_result_5,
	output [15:0] o_result_0,
	output [15:0] o_result_1,
	output [15:0] o_result_2,
	output [15:0] o_result_3,
	output o_valid
);

reg [2:0] state, next_state;
wire [29:0] adders_res_0_0;
reg [29:0] serialize_reg_0_0;
reg [29:0] adders_res_0_1;
reg [29:0] serialize_reg_0_1;
reg [29:0] adders_res_0_2;
reg [29:0] serialize_reg_0_2;
reg [29:0] adders_res_0_3;
reg [29:0] serialize_reg_0_3;
reg [29:0] adders_res_0_4;
reg [29:0] serialize_reg_0_4;
reg [29:0] adders_res_0_5;
reg [29:0] serialize_reg_0_5;
wire [29:0] adders_res_1_0;
reg [29:0] serialize_reg_1_0;
reg [29:0] adders_res_1_1;
reg [29:0] serialize_reg_1_1;
reg [29:0] adders_res_1_2;
reg [29:0] serialize_reg_1_2;
reg [29:0] adders_res_1_3;
reg [29:0] serialize_reg_1_3;
reg [29:0] adders_res_1_4;
reg [29:0] serialize_reg_1_4;
reg [29:0] adders_res_1_5;
reg [29:0] serialize_reg_1_5;
wire [29:0] adders_res_2_0;
reg [29:0] serialize_reg_2_0;
reg [29:0] adders_res_2_1;
reg [29:0] serialize_reg_2_1;
reg [29:0] adders_res_2_2;
reg [29:0] serialize_reg_2_2;
reg [29:0] adders_res_2_3;
reg [29:0] serialize_reg_2_3;
reg [29:0] adders_res_2_4;
reg [29:0] serialize_reg_2_4;
reg [29:0] adders_res_2_5;
reg [29:0] serialize_reg_2_5;
wire [29:0] adders_res_3_0;
reg [29:0] serialize_reg_3_0;
reg [29:0] adders_res_3_1;
reg [29:0] serialize_reg_3_1;
reg [29:0] adders_res_3_2;
reg [29:0] serialize_reg_3_2;
reg [29:0] adders_res_3_3;
reg [29:0] serialize_reg_3_3;
reg [29:0] adders_res_3_4;
reg [29:0] serialize_reg_3_4;
reg [29:0] adders_res_3_5;
reg [29:0] serialize_reg_3_5;

wire [29:0] result_reg_0_0;
wire [29:0] result_reg_0_1;
wire [29:0] result_reg_0_2;
wire [29:0] result_reg_0_3;
reg [29:0] result_reg_1_0;
reg [29:0] result_reg_1_1;
reg [29:0] result_reg_1_2;
reg [29:0] result_reg_1_3;
reg [29:0] result_reg_2_0;
reg [29:0] result_reg_2_1;
reg [29:0] result_reg_2_2;
reg [29:0] result_reg_2_3;
reg [29:0] result_reg_3_0;
reg [29:0] result_reg_3_1;
reg [29:0] result_reg_3_2;
reg [29:0] result_reg_3_3;
reg [29:0] result_reg_4_0;
reg [29:0] result_reg_4_1;
reg [29:0] result_reg_4_2;
reg [29:0] result_reg_4_3;
reg [29:0] result_reg_5_0;
reg [29:0] result_reg_5_1;
reg [29:0] result_reg_5_2;
reg [29:0] result_reg_5_3;
reg [29:0] result_reg_6_0;
reg [29:0] result_reg_6_1;
reg [29:0] result_reg_6_2;
reg [29:0] result_reg_6_3;
reg [29:0] result_reg_7_0;
reg [29:0] result_reg_7_1;
reg [29:0] result_reg_7_2;
reg [29:0] result_reg_7_3;
reg [29:0] result_reg_8_0;
reg [29:0] result_reg_8_1;
reg [29:0] result_reg_8_2;
reg [29:0] result_reg_8_3;
reg [29:0] result_reg_9_0;
reg [29:0] result_reg_9_1;
reg [29:0] result_reg_9_2;
reg [29:0] result_reg_9_3;
reg [29:0] result_reg_10_0;
reg [29:0] result_reg_10_1;
reg [29:0] result_reg_10_2;
reg [29:0] result_reg_10_3;
reg [29:0] result_reg_11_0;
reg [29:0] result_reg_11_1;
reg [29:0] result_reg_11_2;
reg [29:0] result_reg_11_3;
reg [29:0] result_reg_12_0;
reg [29:0] result_reg_12_1;
reg [29:0] result_reg_12_2;
reg [29:0] result_reg_12_3;
reg [29:0] result_reg_13_0;
reg [29:0] result_reg_13_1;
reg [29:0] result_reg_13_2;
reg [29:0] result_reg_13_3;
reg [29:0] result_reg_14_0;
reg [29:0] result_reg_14_1;
reg [29:0] result_reg_14_2;
reg [29:0] result_reg_14_3;
reg [29:0] result_reg_15_0;
reg [29:0] result_reg_15_1;
reg [29:0] result_reg_15_2;
reg [29:0] result_reg_15_3;
reg [29:0] result_reg_16_0;
reg [29:0] result_reg_16_1;
reg [29:0] result_reg_16_2;
reg [29:0] result_reg_16_3;

reg out_valid_0;
reg out_valid_1;
reg out_valid_2;
reg out_valid_3;
reg out_valid_4;
reg out_valid_5;
reg out_valid_6;
reg out_valid_7;
reg out_valid_8;
reg out_valid_9;
reg out_valid_10;
reg out_valid_11;
reg out_valid_12;
reg out_valid_13;
reg out_valid_14;
reg out_valid_15;
reg out_valid_16;
reg [1:0] serialize_count;

always @ (posedge clk) begin
	out_valid_1 <= out_valid_0;
	result_reg_1_0 <= result_reg_0_0;
	result_reg_1_1 <= result_reg_0_1;
	result_reg_1_2 <= result_reg_0_2;
	result_reg_1_3 <= result_reg_0_3;
	out_valid_2 <= out_valid_1;
	result_reg_2_0 <= result_reg_1_0;
	result_reg_2_1 <= result_reg_1_1;
	result_reg_2_2 <= result_reg_1_2;
	result_reg_2_3 <= result_reg_1_3;
	out_valid_3 <= out_valid_2;
	result_reg_3_0 <= result_reg_2_0;
	result_reg_3_1 <= result_reg_2_1;
	result_reg_3_2 <= result_reg_2_2;
	result_reg_3_3 <= result_reg_2_3;
	out_valid_4 <= out_valid_3;
	result_reg_4_0 <= result_reg_3_0;
	result_reg_4_1 <= result_reg_3_1;
	result_reg_4_2 <= result_reg_3_2;
	result_reg_4_3 <= result_reg_3_3;
	out_valid_5 <= out_valid_4;
	result_reg_5_0 <= result_reg_4_0;
	result_reg_5_1 <= result_reg_4_1;
	result_reg_5_2 <= result_reg_4_2;
	result_reg_5_3 <= result_reg_4_3;
	out_valid_6 <= out_valid_5;
	result_reg_6_0 <= result_reg_5_0;
	result_reg_6_1 <= result_reg_5_1;
	result_reg_6_2 <= result_reg_5_2;
	result_reg_6_3 <= result_reg_5_3;
	out_valid_7 <= out_valid_6;
	result_reg_7_0 <= result_reg_6_0;
	result_reg_7_1 <= result_reg_6_1;
	result_reg_7_2 <= result_reg_6_2;
	result_reg_7_3 <= result_reg_6_3;
	out_valid_8 <= out_valid_7;
	result_reg_8_0 <= result_reg_7_0;
	result_reg_8_1 <= result_reg_7_1;
	result_reg_8_2 <= result_reg_7_2;
	result_reg_8_3 <= result_reg_7_3;
	out_valid_9 <= out_valid_8;
	result_reg_9_0 <= result_reg_8_0;
	result_reg_9_1 <= result_reg_8_1;
	result_reg_9_2 <= result_reg_8_2;
	result_reg_9_3 <= result_reg_8_3;
	out_valid_10 <= out_valid_9;
	result_reg_10_0 <= result_reg_9_0;
	result_reg_10_1 <= result_reg_9_1;
	result_reg_10_2 <= result_reg_9_2;
	result_reg_10_3 <= result_reg_9_3;
	out_valid_11 <= out_valid_10;
	result_reg_11_0 <= result_reg_10_0;
	result_reg_11_1 <= result_reg_10_1;
	result_reg_11_2 <= result_reg_10_2;
	result_reg_11_3 <= result_reg_10_3;
	out_valid_12 <= out_valid_11;
	result_reg_12_0 <= result_reg_11_0;
	result_reg_12_1 <= result_reg_11_1;
	result_reg_12_2 <= result_reg_11_2;
	result_reg_12_3 <= result_reg_11_3;
	out_valid_13 <= out_valid_12;
	result_reg_13_0 <= result_reg_12_0;
	result_reg_13_1 <= result_reg_12_1;
	result_reg_13_2 <= result_reg_12_2;
	result_reg_13_3 <= result_reg_12_3;
	out_valid_14 <= out_valid_13;
	result_reg_14_0 <= result_reg_13_0;
	result_reg_14_1 <= result_reg_13_1;
	result_reg_14_2 <= result_reg_13_2;
	result_reg_14_3 <= result_reg_13_3;
	out_valid_15 <= out_valid_14;
	result_reg_15_0 <= result_reg_14_0;
	result_reg_15_1 <= result_reg_14_1;
	result_reg_15_2 <= result_reg_14_2;
	result_reg_15_3 <= result_reg_14_3;
	out_valid_16 <= out_valid_15;
	result_reg_16_0 <= result_reg_15_0;
	result_reg_16_1 <= result_reg_15_1;
	result_reg_16_2 <= result_reg_15_2;
	result_reg_16_3 <= result_reg_15_3;
end

// FSM Combinational Logic
always @ (state or i_valid or serialize_count) begin
	if(state == 3'b000) begin
		if(i_valid == 1'b1)begin
			next_state = 3'b010;
		end else begin
			next_state = 3'b000;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b0) begin
			next_state = 3'b100;
		end else begin
			next_state = 3'b010;
		end
	end else if (state == 3'b100) begin
		if(serialize_count == 3)begin
			next_state = 3'b000;
		end else begin
			next_state = 3'b100;
		end
	end else begin
		next_state = 3'b000;
	end
end

// FSM Sequential Logic
always @ (posedge clk) begin
	if (state == 3'b000) begin
		serialize_count <= 0;
		out_valid_0 <= 0;
		if (i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end else begin
			serialize_reg_0_0 <= adders_res_0_0;
			serialize_reg_1_0 <= adders_res_1_0;
			serialize_reg_2_0 <= adders_res_2_0;
			serialize_reg_3_0 <= adders_res_3_0;
			serialize_reg_0_1 <= adders_res_0_1;
			serialize_reg_1_1 <= adders_res_1_1;
			serialize_reg_2_1 <= adders_res_2_1;
			serialize_reg_3_1 <= adders_res_3_1;
			serialize_reg_0_2 <= adders_res_0_2;
			serialize_reg_1_2 <= adders_res_1_2;
			serialize_reg_2_2 <= adders_res_2_2;
			serialize_reg_3_2 <= adders_res_3_2;
			serialize_reg_0_3 <= adders_res_0_3;
			serialize_reg_1_3 <= adders_res_1_3;
			serialize_reg_2_3 <= adders_res_2_3;
			serialize_reg_3_3 <= adders_res_3_3;
			serialize_reg_0_4 <= adders_res_0_4;
			serialize_reg_1_4 <= adders_res_1_4;
			serialize_reg_2_4 <= adders_res_2_4;
			serialize_reg_3_4 <= adders_res_3_4;
			serialize_reg_0_5 <= adders_res_0_5;
			serialize_reg_1_5 <= adders_res_1_5;
			serialize_reg_2_5 <= adders_res_2_5;
			serialize_reg_3_5 <= adders_res_3_5;
		end
	end else if (state == 3'b100) begin
		if (serialize_count < 3) begin
			serialize_reg_0_0 <= serialize_reg_1_0;
			serialize_reg_0_1 <= serialize_reg_1_1;
			serialize_reg_0_2 <= serialize_reg_1_2;
			serialize_reg_0_3 <= serialize_reg_1_3;
			serialize_reg_0_4 <= serialize_reg_1_4;
			serialize_reg_0_5 <= serialize_reg_1_5;
			serialize_reg_1_0 <= serialize_reg_2_0;
			serialize_reg_1_1 <= serialize_reg_2_1;
			serialize_reg_1_2 <= serialize_reg_2_2;
			serialize_reg_1_3 <= serialize_reg_2_3;
			serialize_reg_1_4 <= serialize_reg_2_4;
			serialize_reg_1_5 <= serialize_reg_2_5;
			serialize_reg_2_0 <= serialize_reg_3_0;
			serialize_reg_2_1 <= serialize_reg_3_1;
			serialize_reg_2_2 <= serialize_reg_3_2;
			serialize_reg_2_3 <= serialize_reg_3_3;
			serialize_reg_2_4 <= serialize_reg_3_4;
			serialize_reg_2_5 <= serialize_reg_3_5;
			serialize_count <= serialize_count + 1'b1;
			out_valid_0 <= 1;
		end
	end
	state <=  next_state;
end

// AT Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA0 (
	.data0x(i_result_0),
	.data1x(i_result_1),
	.data2x(i_result_2),
	.data3x(i_result_3),
	.data4x(i_result_4),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(adders_res_0_0)
);
wire [29:0] f1, f2;
assign f1 = -i_result_2;
assign f2 = -{i_result_4[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA1 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f1),
	.data3x({i_result_3[28:0], 1'b0}),
	.data4x(f2),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_1_0)
);
wire [29:0] f3, f4;
assign f3 = -i_result_2;
assign f4 = -{i_result_4[27:0], 2'b00};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA2 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f3),
	.data3x({i_result_3[27:0], 2'b00}),
	.data4x(f4),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_2_0)
);
wire [29:0] f5, f6;
assign f5 = -i_result_2;
assign f6 = -{i_result_4[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA3 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f5),
	.data3x({i_result_3[26:0], 3'b000}),
	.data4x(f6),
	.data5x(i_result_5),
	.clock(clk),
	.result(adders_res_3_0)
);

// A Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT0 (
	.data0x(serialize_reg_0_5),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x(serialize_reg_0_2),
	.data4x(serialize_reg_0_1),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(result_reg_0_0)
);
wire [29:0] f7, f8;
assign f7 = -serialize_reg_0_3;
assign f8 = -{serialize_reg_0_1[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT1 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f7),
	.data3x({serialize_reg_0_2[28:0], 1'b0}),
	.data4x(f8),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_1)
);
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT2 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x({serialize_reg_0_2[27:0], 2'b00}),
	.data4x({serialize_reg_0_1[27:0], 2'b00}),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_2)
);
wire [29:0] f9, f10;
assign f9 = -serialize_reg_0_3;
assign f10 = -{serialize_reg_0_1[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT3 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f9),
	.data3x({serialize_reg_0_2[26:0], 3'b000}),
	.data4x(f10),
	.data5x(serialize_reg_0_0),
	.clock(clk),
	.result(result_reg_0_3)
);

reg [15:0] result_wire_0;
reg [15:0] result_wire_1;
reg [15:0] result_wire_2;
reg [15:0] result_wire_3;

always @ (*) begin
	if(result_reg_16_0[29] == 1'b0) begin
		result_wire_0 <= result_reg_16_0[23:8];
	end else begin
		result_wire_0 <= { (16){1'b0} };
	end
	if(result_reg_16_1[29] == 1'b0) begin
		result_wire_1 <= result_reg_16_1[23:8];
	end else begin
		result_wire_1 <= { (16){1'b0} };
	end
	if(result_reg_16_2[29] == 1'b0) begin
		result_wire_2 <= result_reg_16_2[23:8];
	end else begin
		result_wire_2 <= { (16){1'b0} };
	end
	if(result_reg_16_3[29] == 1'b0) begin
		result_wire_3 <= result_reg_16_3[23:8];
	end else begin
		result_wire_3 <= { (16){1'b0} };
	end
end

assign o_valid = out_valid_16;
assign o_result_0 = result_wire_0;
assign o_result_1 = result_wire_1;
assign o_result_2 = result_wire_2;
assign o_result_3 = result_wire_3;

endmodule

module inverse_winograd_3 (
	input clk,
	input i_valid,
	input [29:0] i_result_0,
	input [29:0] i_result_1,
	input [29:0] i_result_2,
	input [29:0] i_result_3,
	input [29:0] i_result_4,
	input [29:0] i_result_5,
	output [15:0] o_result_0,
	output [15:0] o_result_1,
	output [15:0] o_result_2,
	output [15:0] o_result_3,
	output o_valid
);

reg [2:0] state, next_state;
wire [29:0] adders_res_0_0;
reg [29:0] serialize_reg_0_0;
reg [29:0] adders_res_0_1;
reg [29:0] serialize_reg_0_1;
reg [29:0] adders_res_0_2;
reg [29:0] serialize_reg_0_2;
reg [29:0] adders_res_0_3;
reg [29:0] serialize_reg_0_3;
reg [29:0] adders_res_0_4;
reg [29:0] serialize_reg_0_4;
reg [29:0] adders_res_0_5;
reg [29:0] serialize_reg_0_5;
wire [29:0] adders_res_1_0;
reg [29:0] serialize_reg_1_0;
reg [29:0] adders_res_1_1;
reg [29:0] serialize_reg_1_1;
reg [29:0] adders_res_1_2;
reg [29:0] serialize_reg_1_2;
reg [29:0] adders_res_1_3;
reg [29:0] serialize_reg_1_3;
reg [29:0] adders_res_1_4;
reg [29:0] serialize_reg_1_4;
reg [29:0] adders_res_1_5;
reg [29:0] serialize_reg_1_5;
wire [29:0] adders_res_2_0;
reg [29:0] serialize_reg_2_0;
reg [29:0] adders_res_2_1;
reg [29:0] serialize_reg_2_1;
reg [29:0] adders_res_2_2;
reg [29:0] serialize_reg_2_2;
reg [29:0] adders_res_2_3;
reg [29:0] serialize_reg_2_3;
reg [29:0] adders_res_2_4;
reg [29:0] serialize_reg_2_4;
reg [29:0] adders_res_2_5;
reg [29:0] serialize_reg_2_5;
wire [29:0] adders_res_3_0;
reg [29:0] serialize_reg_3_0;
reg [29:0] adders_res_3_1;
reg [29:0] serialize_reg_3_1;
reg [29:0] adders_res_3_2;
reg [29:0] serialize_reg_3_2;
reg [29:0] adders_res_3_3;
reg [29:0] serialize_reg_3_3;
reg [29:0] adders_res_3_4;
reg [29:0] serialize_reg_3_4;
reg [29:0] adders_res_3_5;
reg [29:0] serialize_reg_3_5;

wire [29:0] result_reg_0_0;
wire [29:0] result_reg_0_1;
wire [29:0] result_reg_0_2;
wire [29:0] result_reg_0_3;
reg [29:0] result_reg_1_0;
reg [29:0] result_reg_1_1;
reg [29:0] result_reg_1_2;
reg [29:0] result_reg_1_3;
reg [29:0] result_reg_2_0;
reg [29:0] result_reg_2_1;
reg [29:0] result_reg_2_2;
reg [29:0] result_reg_2_3;
reg [29:0] result_reg_3_0;
reg [29:0] result_reg_3_1;
reg [29:0] result_reg_3_2;
reg [29:0] result_reg_3_3;
reg [29:0] result_reg_4_0;
reg [29:0] result_reg_4_1;
reg [29:0] result_reg_4_2;
reg [29:0] result_reg_4_3;
reg [29:0] result_reg_5_0;
reg [29:0] result_reg_5_1;
reg [29:0] result_reg_5_2;
reg [29:0] result_reg_5_3;
reg [29:0] result_reg_6_0;
reg [29:0] result_reg_6_1;
reg [29:0] result_reg_6_2;
reg [29:0] result_reg_6_3;
reg [29:0] result_reg_7_0;
reg [29:0] result_reg_7_1;
reg [29:0] result_reg_7_2;
reg [29:0] result_reg_7_3;
reg [29:0] result_reg_8_0;
reg [29:0] result_reg_8_1;
reg [29:0] result_reg_8_2;
reg [29:0] result_reg_8_3;
reg [29:0] result_reg_9_0;
reg [29:0] result_reg_9_1;
reg [29:0] result_reg_9_2;
reg [29:0] result_reg_9_3;
reg [29:0] result_reg_10_0;
reg [29:0] result_reg_10_1;
reg [29:0] result_reg_10_2;
reg [29:0] result_reg_10_3;
reg [29:0] result_reg_11_0;
reg [29:0] result_reg_11_1;
reg [29:0] result_reg_11_2;
reg [29:0] result_reg_11_3;
reg [29:0] result_reg_12_0;
reg [29:0] result_reg_12_1;
reg [29:0] result_reg_12_2;
reg [29:0] result_reg_12_3;
reg [29:0] result_reg_13_0;
reg [29:0] result_reg_13_1;
reg [29:0] result_reg_13_2;
reg [29:0] result_reg_13_3;

reg out_valid_0;
reg out_valid_1;
reg out_valid_2;
reg out_valid_3;
reg out_valid_4;
reg out_valid_5;
reg out_valid_6;
reg out_valid_7;
reg out_valid_8;
reg out_valid_9;
reg out_valid_10;
reg out_valid_11;
reg out_valid_12;
reg out_valid_13;
reg [1:0] serialize_count;

always @ (posedge clk) begin
	out_valid_1 <= out_valid_0;
	result_reg_1_0 <= result_reg_0_0;
	result_reg_1_1 <= result_reg_0_1;
	result_reg_1_2 <= result_reg_0_2;
	result_reg_1_3 <= result_reg_0_3;
	out_valid_2 <= out_valid_1;
	result_reg_2_0 <= result_reg_1_0;
	result_reg_2_1 <= result_reg_1_1;
	result_reg_2_2 <= result_reg_1_2;
	result_reg_2_3 <= result_reg_1_3;
	out_valid_3 <= out_valid_2;
	result_reg_3_0 <= result_reg_2_0;
	result_reg_3_1 <= result_reg_2_1;
	result_reg_3_2 <= result_reg_2_2;
	result_reg_3_3 <= result_reg_2_3;
	out_valid_4 <= out_valid_3;
	result_reg_4_0 <= result_reg_3_0;
	result_reg_4_1 <= result_reg_3_1;
	result_reg_4_2 <= result_reg_3_2;
	result_reg_4_3 <= result_reg_3_3;
	out_valid_5 <= out_valid_4;
	result_reg_5_0 <= result_reg_4_0;
	result_reg_5_1 <= result_reg_4_1;
	result_reg_5_2 <= result_reg_4_2;
	result_reg_5_3 <= result_reg_4_3;
	out_valid_6 <= out_valid_5;
	result_reg_6_0 <= result_reg_5_0;
	result_reg_6_1 <= result_reg_5_1;
	result_reg_6_2 <= result_reg_5_2;
	result_reg_6_3 <= result_reg_5_3;
	out_valid_7 <= out_valid_6;
	result_reg_7_0 <= result_reg_6_0;
	result_reg_7_1 <= result_reg_6_1;
	result_reg_7_2 <= result_reg_6_2;
	result_reg_7_3 <= result_reg_6_3;
	out_valid_8 <= out_valid_7;
	result_reg_8_0 <= result_reg_7_0;
	result_reg_8_1 <= result_reg_7_1;
	result_reg_8_2 <= result_reg_7_2;
	result_reg_8_3 <= result_reg_7_3;
	out_valid_9 <= out_valid_8;
	result_reg_9_0 <= result_reg_8_0;
	result_reg_9_1 <= result_reg_8_1;
	result_reg_9_2 <= result_reg_8_2;
	result_reg_9_3 <= result_reg_8_3;
	out_valid_10 <= out_valid_9;
	result_reg_10_0 <= result_reg_9_0;
	result_reg_10_1 <= result_reg_9_1;
	result_reg_10_2 <= result_reg_9_2;
	result_reg_10_3 <= result_reg_9_3;
	out_valid_11 <= out_valid_10;
	result_reg_11_0 <= result_reg_10_0;
	result_reg_11_1 <= result_reg_10_1;
	result_reg_11_2 <= result_reg_10_2;
	result_reg_11_3 <= result_reg_10_3;
	out_valid_12 <= out_valid_11;
	result_reg_12_0 <= result_reg_11_0;
	result_reg_12_1 <= result_reg_11_1;
	result_reg_12_2 <= result_reg_11_2;
	result_reg_12_3 <= result_reg_11_3;
	out_valid_13 <= out_valid_12;
	result_reg_13_0 <= result_reg_12_0;
	result_reg_13_1 <= result_reg_12_1;
	result_reg_13_2 <= result_reg_12_2;
	result_reg_13_3 <= result_reg_12_3;
end

// FSM Combinational Logic
always @ (state or i_valid or serialize_count) begin
	if(state == 3'b000) begin
		if(i_valid == 1'b1)begin
			next_state = 3'b010;
		end else begin
			next_state = 3'b000;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b0) begin
			next_state = 3'b100;
		end else begin
			next_state = 3'b010;
		end
	end else if (state == 3'b100) begin
		if(serialize_count == 3)begin
			next_state = 3'b000;
		end else begin
			next_state = 3'b100;
		end
	end else begin
		next_state = 3'b000;
	end
end

// FSM Sequential Logic
always @ (posedge clk) begin
	if (state == 3'b000) begin
		serialize_count <= 0;
		out_valid_0 <= 0;
		if (i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end else begin
			serialize_reg_0_0 <= adders_res_0_0;
			serialize_reg_1_0 <= adders_res_1_0;
			serialize_reg_2_0 <= adders_res_2_0;
			serialize_reg_3_0 <= adders_res_3_0;
			serialize_reg_0_1 <= adders_res_0_1;
			serialize_reg_1_1 <= adders_res_1_1;
			serialize_reg_2_1 <= adders_res_2_1;
			serialize_reg_3_1 <= adders_res_3_1;
			serialize_reg_0_2 <= adders_res_0_2;
			serialize_reg_1_2 <= adders_res_1_2;
			serialize_reg_2_2 <= adders_res_2_2;
			serialize_reg_3_2 <= adders_res_3_2;
			serialize_reg_0_3 <= adders_res_0_3;
			serialize_reg_1_3 <= adders_res_1_3;
			serialize_reg_2_3 <= adders_res_2_3;
			serialize_reg_3_3 <= adders_res_3_3;
			serialize_reg_0_4 <= adders_res_0_4;
			serialize_reg_1_4 <= adders_res_1_4;
			serialize_reg_2_4 <= adders_res_2_4;
			serialize_reg_3_4 <= adders_res_3_4;
			serialize_reg_0_5 <= adders_res_0_5;
			serialize_reg_1_5 <= adders_res_1_5;
			serialize_reg_2_5 <= adders_res_2_5;
			serialize_reg_3_5 <= adders_res_3_5;
		end
	end else if (state == 3'b100) begin
		if (serialize_count < 3) begin
			serialize_reg_0_0 <= serialize_reg_1_0;
			serialize_reg_0_1 <= serialize_reg_1_1;
			serialize_reg_0_2 <= serialize_reg_1_2;
			serialize_reg_0_3 <= serialize_reg_1_3;
			serialize_reg_0_4 <= serialize_reg_1_4;
			serialize_reg_0_5 <= serialize_reg_1_5;
			serialize_reg_1_0 <= serialize_reg_2_0;
			serialize_reg_1_1 <= serialize_reg_2_1;
			serialize_reg_1_2 <= serialize_reg_2_2;
			serialize_reg_1_3 <= serialize_reg_2_3;
			serialize_reg_1_4 <= serialize_reg_2_4;
			serialize_reg_1_5 <= serialize_reg_2_5;
			serialize_reg_2_0 <= serialize_reg_3_0;
			serialize_reg_2_1 <= serialize_reg_3_1;
			serialize_reg_2_2 <= serialize_reg_3_2;
			serialize_reg_2_3 <= serialize_reg_3_3;
			serialize_reg_2_4 <= serialize_reg_3_4;
			serialize_reg_2_5 <= serialize_reg_3_5;
			serialize_count <= serialize_count + 1'b1;
			out_valid_0 <= 1;
		end
	end
	state <=  next_state;
end

// AT Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA0 (
	.data0x(i_result_0),
	.data1x(i_result_1),
	.data2x(i_result_2),
	.data3x(i_result_3),
	.data4x(i_result_4),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(adders_res_0_0)
);
wire [29:0] f1, f2;
assign f1 = -i_result_2;
assign f2 = -{i_result_4[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA1 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f1),
	.data3x({i_result_3[28:0], 1'b0}),
	.data4x(f2),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_1_0)
);
wire [29:0] f3, f4;
assign f3 = -i_result_2;
assign f4 = -{i_result_4[27:0], 2'b00};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA2 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f3),
	.data3x({i_result_3[27:0], 2'b00}),
	.data4x(f4),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_2_0)
);
wire [29:0] f5, f6;
assign f5 = -i_result_2;
assign f6 = -{i_result_4[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA3 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f5),
	.data3x({i_result_3[26:0], 3'b000}),
	.data4x(f6),
	.data5x(i_result_5),
	.clock(clk),
	.result(adders_res_3_0)
);

// A Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT0 (
	.data0x(serialize_reg_0_5),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x(serialize_reg_0_2),
	.data4x(serialize_reg_0_1),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(result_reg_0_0)
);
wire [29:0] f7, f8;
assign f7 = -serialize_reg_0_3;
assign f8 = -{serialize_reg_0_1[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT1 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f7),
	.data3x({serialize_reg_0_2[28:0], 1'b0}),
	.data4x(f8),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_1)
);
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT2 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x({serialize_reg_0_2[27:0], 2'b00}),
	.data4x({serialize_reg_0_1[27:0], 2'b00}),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_2)
);
wire [29:0] f9, f10;
assign f9 = -serialize_reg_0_3;
assign f10 = -{serialize_reg_0_1[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT3 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f9),
	.data3x({serialize_reg_0_2[26:0], 3'b000}),
	.data4x(f10),
	.data5x(serialize_reg_0_0),
	.clock(clk),
	.result(result_reg_0_3)
);

reg [15:0] result_wire_0;
reg [15:0] result_wire_1;
reg [15:0] result_wire_2;
reg [15:0] result_wire_3;

always @ (*) begin
	if(result_reg_13_0[29] == 1'b0) begin
		result_wire_0 <= result_reg_13_0[23:8];
	end else begin
		result_wire_0 <= { (16){1'b0} };
	end
	if(result_reg_13_1[29] == 1'b0) begin
		result_wire_1 <= result_reg_13_1[23:8];
	end else begin
		result_wire_1 <= { (16){1'b0} };
	end
	if(result_reg_13_2[29] == 1'b0) begin
		result_wire_2 <= result_reg_13_2[23:8];
	end else begin
		result_wire_2 <= { (16){1'b0} };
	end
	if(result_reg_13_3[29] == 1'b0) begin
		result_wire_3 <= result_reg_13_3[23:8];
	end else begin
		result_wire_3 <= { (16){1'b0} };
	end
end

assign o_valid = out_valid_13;
assign o_result_0 = result_wire_0;
assign o_result_1 = result_wire_1;
assign o_result_2 = result_wire_2;
assign o_result_3 = result_wire_3;

endmodule

module inverse_winograd_2 (
	input clk,
	input i_valid,
	input [29:0] i_result_0,
	input [29:0] i_result_1,
	input [29:0] i_result_2,
	input [29:0] i_result_3,
	input [29:0] i_result_4,
	input [29:0] i_result_5,
	output [15:0] o_result_0,
	output [15:0] o_result_1,
	output [15:0] o_result_2,
	output [15:0] o_result_3,
	output o_valid
);

reg [2:0] state, next_state;
wire [29:0] adders_res_0_0;
reg [29:0] serialize_reg_0_0;
reg [29:0] adders_res_0_1;
reg [29:0] serialize_reg_0_1;
reg [29:0] adders_res_0_2;
reg [29:0] serialize_reg_0_2;
reg [29:0] adders_res_0_3;
reg [29:0] serialize_reg_0_3;
reg [29:0] adders_res_0_4;
reg [29:0] serialize_reg_0_4;
reg [29:0] adders_res_0_5;
reg [29:0] serialize_reg_0_5;
wire [29:0] adders_res_1_0;
reg [29:0] serialize_reg_1_0;
reg [29:0] adders_res_1_1;
reg [29:0] serialize_reg_1_1;
reg [29:0] adders_res_1_2;
reg [29:0] serialize_reg_1_2;
reg [29:0] adders_res_1_3;
reg [29:0] serialize_reg_1_3;
reg [29:0] adders_res_1_4;
reg [29:0] serialize_reg_1_4;
reg [29:0] adders_res_1_5;
reg [29:0] serialize_reg_1_5;
wire [29:0] adders_res_2_0;
reg [29:0] serialize_reg_2_0;
reg [29:0] adders_res_2_1;
reg [29:0] serialize_reg_2_1;
reg [29:0] adders_res_2_2;
reg [29:0] serialize_reg_2_2;
reg [29:0] adders_res_2_3;
reg [29:0] serialize_reg_2_3;
reg [29:0] adders_res_2_4;
reg [29:0] serialize_reg_2_4;
reg [29:0] adders_res_2_5;
reg [29:0] serialize_reg_2_5;
wire [29:0] adders_res_3_0;
reg [29:0] serialize_reg_3_0;
reg [29:0] adders_res_3_1;
reg [29:0] serialize_reg_3_1;
reg [29:0] adders_res_3_2;
reg [29:0] serialize_reg_3_2;
reg [29:0] adders_res_3_3;
reg [29:0] serialize_reg_3_3;
reg [29:0] adders_res_3_4;
reg [29:0] serialize_reg_3_4;
reg [29:0] adders_res_3_5;
reg [29:0] serialize_reg_3_5;

wire [29:0] result_reg_0_0;
wire [29:0] result_reg_0_1;
wire [29:0] result_reg_0_2;
wire [29:0] result_reg_0_3;
reg [29:0] result_reg_1_0;
reg [29:0] result_reg_1_1;
reg [29:0] result_reg_1_2;
reg [29:0] result_reg_1_3;
reg [29:0] result_reg_2_0;
reg [29:0] result_reg_2_1;
reg [29:0] result_reg_2_2;
reg [29:0] result_reg_2_3;
reg [29:0] result_reg_3_0;
reg [29:0] result_reg_3_1;
reg [29:0] result_reg_3_2;
reg [29:0] result_reg_3_3;
reg [29:0] result_reg_4_0;
reg [29:0] result_reg_4_1;
reg [29:0] result_reg_4_2;
reg [29:0] result_reg_4_3;
reg [29:0] result_reg_5_0;
reg [29:0] result_reg_5_1;
reg [29:0] result_reg_5_2;
reg [29:0] result_reg_5_3;
reg [29:0] result_reg_6_0;
reg [29:0] result_reg_6_1;
reg [29:0] result_reg_6_2;
reg [29:0] result_reg_6_3;
reg [29:0] result_reg_7_0;
reg [29:0] result_reg_7_1;
reg [29:0] result_reg_7_2;
reg [29:0] result_reg_7_3;
reg [29:0] result_reg_8_0;
reg [29:0] result_reg_8_1;
reg [29:0] result_reg_8_2;
reg [29:0] result_reg_8_3;
reg [29:0] result_reg_9_0;
reg [29:0] result_reg_9_1;
reg [29:0] result_reg_9_2;
reg [29:0] result_reg_9_3;
reg [29:0] result_reg_10_0;
reg [29:0] result_reg_10_1;
reg [29:0] result_reg_10_2;
reg [29:0] result_reg_10_3;
reg [29:0] result_reg_11_0;
reg [29:0] result_reg_11_1;
reg [29:0] result_reg_11_2;
reg [29:0] result_reg_11_3;
reg [29:0] result_reg_12_0;
reg [29:0] result_reg_12_1;
reg [29:0] result_reg_12_2;
reg [29:0] result_reg_12_3;
reg [29:0] result_reg_13_0;
reg [29:0] result_reg_13_1;
reg [29:0] result_reg_13_2;
reg [29:0] result_reg_13_3;
reg [29:0] result_reg_14_0;
reg [29:0] result_reg_14_1;
reg [29:0] result_reg_14_2;
reg [29:0] result_reg_14_3;

reg out_valid_0;
reg out_valid_1;
reg out_valid_2;
reg out_valid_3;
reg out_valid_4;
reg out_valid_5;
reg out_valid_6;
reg out_valid_7;
reg out_valid_8;
reg out_valid_9;
reg out_valid_10;
reg out_valid_11;
reg out_valid_12;
reg out_valid_13;
reg out_valid_14;
reg [1:0] serialize_count;

always @ (posedge clk) begin
	out_valid_1 <= out_valid_0;
	result_reg_1_0 <= result_reg_0_0;
	result_reg_1_1 <= result_reg_0_1;
	result_reg_1_2 <= result_reg_0_2;
	result_reg_1_3 <= result_reg_0_3;
	out_valid_2 <= out_valid_1;
	result_reg_2_0 <= result_reg_1_0;
	result_reg_2_1 <= result_reg_1_1;
	result_reg_2_2 <= result_reg_1_2;
	result_reg_2_3 <= result_reg_1_3;
	out_valid_3 <= out_valid_2;
	result_reg_3_0 <= result_reg_2_0;
	result_reg_3_1 <= result_reg_2_1;
	result_reg_3_2 <= result_reg_2_2;
	result_reg_3_3 <= result_reg_2_3;
	out_valid_4 <= out_valid_3;
	result_reg_4_0 <= result_reg_3_0;
	result_reg_4_1 <= result_reg_3_1;
	result_reg_4_2 <= result_reg_3_2;
	result_reg_4_3 <= result_reg_3_3;
	out_valid_5 <= out_valid_4;
	result_reg_5_0 <= result_reg_4_0;
	result_reg_5_1 <= result_reg_4_1;
	result_reg_5_2 <= result_reg_4_2;
	result_reg_5_3 <= result_reg_4_3;
	out_valid_6 <= out_valid_5;
	result_reg_6_0 <= result_reg_5_0;
	result_reg_6_1 <= result_reg_5_1;
	result_reg_6_2 <= result_reg_5_2;
	result_reg_6_3 <= result_reg_5_3;
	out_valid_7 <= out_valid_6;
	result_reg_7_0 <= result_reg_6_0;
	result_reg_7_1 <= result_reg_6_1;
	result_reg_7_2 <= result_reg_6_2;
	result_reg_7_3 <= result_reg_6_3;
	out_valid_8 <= out_valid_7;
	result_reg_8_0 <= result_reg_7_0;
	result_reg_8_1 <= result_reg_7_1;
	result_reg_8_2 <= result_reg_7_2;
	result_reg_8_3 <= result_reg_7_3;
	out_valid_9 <= out_valid_8;
	result_reg_9_0 <= result_reg_8_0;
	result_reg_9_1 <= result_reg_8_1;
	result_reg_9_2 <= result_reg_8_2;
	result_reg_9_3 <= result_reg_8_3;
	out_valid_10 <= out_valid_9;
	result_reg_10_0 <= result_reg_9_0;
	result_reg_10_1 <= result_reg_9_1;
	result_reg_10_2 <= result_reg_9_2;
	result_reg_10_3 <= result_reg_9_3;
	out_valid_11 <= out_valid_10;
	result_reg_11_0 <= result_reg_10_0;
	result_reg_11_1 <= result_reg_10_1;
	result_reg_11_2 <= result_reg_10_2;
	result_reg_11_3 <= result_reg_10_3;
	out_valid_12 <= out_valid_11;
	result_reg_12_0 <= result_reg_11_0;
	result_reg_12_1 <= result_reg_11_1;
	result_reg_12_2 <= result_reg_11_2;
	result_reg_12_3 <= result_reg_11_3;
	out_valid_13 <= out_valid_12;
	result_reg_13_0 <= result_reg_12_0;
	result_reg_13_1 <= result_reg_12_1;
	result_reg_13_2 <= result_reg_12_2;
	result_reg_13_3 <= result_reg_12_3;
	out_valid_14 <= out_valid_13;
	result_reg_14_0 <= result_reg_13_0;
	result_reg_14_1 <= result_reg_13_1;
	result_reg_14_2 <= result_reg_13_2;
	result_reg_14_3 <= result_reg_13_3;
end

// FSM Combinational Logic
always @ (state or i_valid or serialize_count) begin
	if(state == 3'b000) begin
		if(i_valid == 1'b1)begin
			next_state = 3'b010;
		end else begin
			next_state = 3'b000;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b0) begin
			next_state = 3'b100;
		end else begin
			next_state = 3'b010;
		end
	end else if (state == 3'b100) begin
		if(serialize_count == 3)begin
			next_state = 3'b000;
		end else begin
			next_state = 3'b100;
		end
	end else begin
		next_state = 3'b000;
	end
end

// FSM Sequential Logic
always @ (posedge clk) begin
	if (state == 3'b000) begin
		serialize_count <= 0;
		out_valid_0 <= 0;
		if (i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end else begin
			serialize_reg_0_0 <= adders_res_0_0;
			serialize_reg_1_0 <= adders_res_1_0;
			serialize_reg_2_0 <= adders_res_2_0;
			serialize_reg_3_0 <= adders_res_3_0;
			serialize_reg_0_1 <= adders_res_0_1;
			serialize_reg_1_1 <= adders_res_1_1;
			serialize_reg_2_1 <= adders_res_2_1;
			serialize_reg_3_1 <= adders_res_3_1;
			serialize_reg_0_2 <= adders_res_0_2;
			serialize_reg_1_2 <= adders_res_1_2;
			serialize_reg_2_2 <= adders_res_2_2;
			serialize_reg_3_2 <= adders_res_3_2;
			serialize_reg_0_3 <= adders_res_0_3;
			serialize_reg_1_3 <= adders_res_1_3;
			serialize_reg_2_3 <= adders_res_2_3;
			serialize_reg_3_3 <= adders_res_3_3;
			serialize_reg_0_4 <= adders_res_0_4;
			serialize_reg_1_4 <= adders_res_1_4;
			serialize_reg_2_4 <= adders_res_2_4;
			serialize_reg_3_4 <= adders_res_3_4;
			serialize_reg_0_5 <= adders_res_0_5;
			serialize_reg_1_5 <= adders_res_1_5;
			serialize_reg_2_5 <= adders_res_2_5;
			serialize_reg_3_5 <= adders_res_3_5;
		end
	end else if (state == 3'b100) begin
		if (serialize_count < 3) begin
			serialize_reg_0_0 <= serialize_reg_1_0;
			serialize_reg_0_1 <= serialize_reg_1_1;
			serialize_reg_0_2 <= serialize_reg_1_2;
			serialize_reg_0_3 <= serialize_reg_1_3;
			serialize_reg_0_4 <= serialize_reg_1_4;
			serialize_reg_0_5 <= serialize_reg_1_5;
			serialize_reg_1_0 <= serialize_reg_2_0;
			serialize_reg_1_1 <= serialize_reg_2_1;
			serialize_reg_1_2 <= serialize_reg_2_2;
			serialize_reg_1_3 <= serialize_reg_2_3;
			serialize_reg_1_4 <= serialize_reg_2_4;
			serialize_reg_1_5 <= serialize_reg_2_5;
			serialize_reg_2_0 <= serialize_reg_3_0;
			serialize_reg_2_1 <= serialize_reg_3_1;
			serialize_reg_2_2 <= serialize_reg_3_2;
			serialize_reg_2_3 <= serialize_reg_3_3;
			serialize_reg_2_4 <= serialize_reg_3_4;
			serialize_reg_2_5 <= serialize_reg_3_5;
			serialize_count <= serialize_count + 1'b1;
			out_valid_0 <= 1;
		end
	end
	state <=  next_state;
end

// AT Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA0 (
	.data0x(i_result_0),
	.data1x(i_result_1),
	.data2x(i_result_2),
	.data3x(i_result_3),
	.data4x(i_result_4),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(adders_res_0_0)
);
wire [29:0] f1, f2;
assign f1 = -i_result_2;
assign f2 = -{i_result_4[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA1 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f1),
	.data3x({i_result_3[28:0], 1'b0}),
	.data4x(f2),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_1_0)
);
wire [29:0] f3, f4;
assign f3 = -i_result_2;
assign f4 = -{i_result_4[27:0], 2'b00};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA2 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f3),
	.data3x({i_result_3[27:0], 2'b00}),
	.data4x(f4),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_2_0)
);
wire [29:0] f5, f6;
assign f5 = -i_result_2;
assign f6 = -{i_result_4[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA3 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f5),
	.data3x({i_result_3[26:0], 3'b000}),
	.data4x(f6),
	.data5x(i_result_5),
	.clock(clk),
	.result(adders_res_3_0)
);

// A Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT0 (
	.data0x(serialize_reg_0_5),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x(serialize_reg_0_2),
	.data4x(serialize_reg_0_1),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(result_reg_0_0)
);
wire [29:0] f7, f8;
assign f7 = -serialize_reg_0_3;
assign f8 = -{serialize_reg_0_1[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT1 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f7),
	.data3x({serialize_reg_0_2[28:0], 1'b0}),
	.data4x(f8),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_1)
);
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT2 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x({serialize_reg_0_2[27:0], 2'b00}),
	.data4x({serialize_reg_0_1[27:0], 2'b00}),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_2)
);
wire [29:0] f9, f10;
assign f9 = -serialize_reg_0_3;
assign f10 = -{serialize_reg_0_1[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT3 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f9),
	.data3x({serialize_reg_0_2[26:0], 3'b000}),
	.data4x(f10),
	.data5x(serialize_reg_0_0),
	.clock(clk),
	.result(result_reg_0_3)
);

reg [15:0] result_wire_0;
reg [15:0] result_wire_1;
reg [15:0] result_wire_2;
reg [15:0] result_wire_3;

always @ (*) begin
	if(result_reg_14_0[29] == 1'b0) begin
		result_wire_0 <= result_reg_14_0[23:8];
	end else begin
		result_wire_0 <= { (16){1'b0} };
	end
	if(result_reg_14_1[29] == 1'b0) begin
		result_wire_1 <= result_reg_14_1[23:8];
	end else begin
		result_wire_1 <= { (16){1'b0} };
	end
	if(result_reg_14_2[29] == 1'b0) begin
		result_wire_2 <= result_reg_14_2[23:8];
	end else begin
		result_wire_2 <= { (16){1'b0} };
	end
	if(result_reg_14_3[29] == 1'b0) begin
		result_wire_3 <= result_reg_14_3[23:8];
	end else begin
		result_wire_3 <= { (16){1'b0} };
	end
end

assign o_valid = out_valid_14;
assign o_result_0 = result_wire_0;
assign o_result_1 = result_wire_1;
assign o_result_2 = result_wire_2;
assign o_result_3 = result_wire_3;

endmodule

module inverse_winograd_5 (
	input clk,
	input i_valid,
	input [29:0] i_result_0,
	input [29:0] i_result_1,
	input [29:0] i_result_2,
	input [29:0] i_result_3,
	input [29:0] i_result_4,
	input [29:0] i_result_5,
	output [15:0] o_result_0,
	output [15:0] o_result_1,
	output [15:0] o_result_2,
	output [15:0] o_result_3,
	output o_valid
);

reg [2:0] state, next_state;
wire [29:0] adders_res_0_0;
reg [29:0] serialize_reg_0_0;
reg [29:0] adders_res_0_1;
reg [29:0] serialize_reg_0_1;
reg [29:0] adders_res_0_2;
reg [29:0] serialize_reg_0_2;
reg [29:0] adders_res_0_3;
reg [29:0] serialize_reg_0_3;
reg [29:0] adders_res_0_4;
reg [29:0] serialize_reg_0_4;
reg [29:0] adders_res_0_5;
reg [29:0] serialize_reg_0_5;
wire [29:0] adders_res_1_0;
reg [29:0] serialize_reg_1_0;
reg [29:0] adders_res_1_1;
reg [29:0] serialize_reg_1_1;
reg [29:0] adders_res_1_2;
reg [29:0] serialize_reg_1_2;
reg [29:0] adders_res_1_3;
reg [29:0] serialize_reg_1_3;
reg [29:0] adders_res_1_4;
reg [29:0] serialize_reg_1_4;
reg [29:0] adders_res_1_5;
reg [29:0] serialize_reg_1_5;
wire [29:0] adders_res_2_0;
reg [29:0] serialize_reg_2_0;
reg [29:0] adders_res_2_1;
reg [29:0] serialize_reg_2_1;
reg [29:0] adders_res_2_2;
reg [29:0] serialize_reg_2_2;
reg [29:0] adders_res_2_3;
reg [29:0] serialize_reg_2_3;
reg [29:0] adders_res_2_4;
reg [29:0] serialize_reg_2_4;
reg [29:0] adders_res_2_5;
reg [29:0] serialize_reg_2_5;
wire [29:0] adders_res_3_0;
reg [29:0] serialize_reg_3_0;
reg [29:0] adders_res_3_1;
reg [29:0] serialize_reg_3_1;
reg [29:0] adders_res_3_2;
reg [29:0] serialize_reg_3_2;
reg [29:0] adders_res_3_3;
reg [29:0] serialize_reg_3_3;
reg [29:0] adders_res_3_4;
reg [29:0] serialize_reg_3_4;
reg [29:0] adders_res_3_5;
reg [29:0] serialize_reg_3_5;

wire [29:0] result_reg_0_0;
wire [29:0] result_reg_0_1;
wire [29:0] result_reg_0_2;
wire [29:0] result_reg_0_3;
reg [29:0] result_reg_1_0;
reg [29:0] result_reg_1_1;
reg [29:0] result_reg_1_2;
reg [29:0] result_reg_1_3;
reg [29:0] result_reg_2_0;
reg [29:0] result_reg_2_1;
reg [29:0] result_reg_2_2;
reg [29:0] result_reg_2_3;
reg [29:0] result_reg_3_0;
reg [29:0] result_reg_3_1;
reg [29:0] result_reg_3_2;
reg [29:0] result_reg_3_3;
reg [29:0] result_reg_4_0;
reg [29:0] result_reg_4_1;
reg [29:0] result_reg_4_2;
reg [29:0] result_reg_4_3;
reg [29:0] result_reg_5_0;
reg [29:0] result_reg_5_1;
reg [29:0] result_reg_5_2;
reg [29:0] result_reg_5_3;
reg [29:0] result_reg_6_0;
reg [29:0] result_reg_6_1;
reg [29:0] result_reg_6_2;
reg [29:0] result_reg_6_3;
reg [29:0] result_reg_7_0;
reg [29:0] result_reg_7_1;
reg [29:0] result_reg_7_2;
reg [29:0] result_reg_7_3;
reg [29:0] result_reg_8_0;
reg [29:0] result_reg_8_1;
reg [29:0] result_reg_8_2;
reg [29:0] result_reg_8_3;
reg [29:0] result_reg_9_0;
reg [29:0] result_reg_9_1;
reg [29:0] result_reg_9_2;
reg [29:0] result_reg_9_3;
reg [29:0] result_reg_10_0;
reg [29:0] result_reg_10_1;
reg [29:0] result_reg_10_2;
reg [29:0] result_reg_10_3;
reg [29:0] result_reg_11_0;
reg [29:0] result_reg_11_1;
reg [29:0] result_reg_11_2;
reg [29:0] result_reg_11_3;

reg out_valid_0;
reg out_valid_1;
reg out_valid_2;
reg out_valid_3;
reg out_valid_4;
reg out_valid_5;
reg out_valid_6;
reg out_valid_7;
reg out_valid_8;
reg out_valid_9;
reg out_valid_10;
reg out_valid_11;
reg [1:0] serialize_count;

always @ (posedge clk) begin
	out_valid_1 <= out_valid_0;
	result_reg_1_0 <= result_reg_0_0;
	result_reg_1_1 <= result_reg_0_1;
	result_reg_1_2 <= result_reg_0_2;
	result_reg_1_3 <= result_reg_0_3;
	out_valid_2 <= out_valid_1;
	result_reg_2_0 <= result_reg_1_0;
	result_reg_2_1 <= result_reg_1_1;
	result_reg_2_2 <= result_reg_1_2;
	result_reg_2_3 <= result_reg_1_3;
	out_valid_3 <= out_valid_2;
	result_reg_3_0 <= result_reg_2_0;
	result_reg_3_1 <= result_reg_2_1;
	result_reg_3_2 <= result_reg_2_2;
	result_reg_3_3 <= result_reg_2_3;
	out_valid_4 <= out_valid_3;
	result_reg_4_0 <= result_reg_3_0;
	result_reg_4_1 <= result_reg_3_1;
	result_reg_4_2 <= result_reg_3_2;
	result_reg_4_3 <= result_reg_3_3;
	out_valid_5 <= out_valid_4;
	result_reg_5_0 <= result_reg_4_0;
	result_reg_5_1 <= result_reg_4_1;
	result_reg_5_2 <= result_reg_4_2;
	result_reg_5_3 <= result_reg_4_3;
	out_valid_6 <= out_valid_5;
	result_reg_6_0 <= result_reg_5_0;
	result_reg_6_1 <= result_reg_5_1;
	result_reg_6_2 <= result_reg_5_2;
	result_reg_6_3 <= result_reg_5_3;
	out_valid_7 <= out_valid_6;
	result_reg_7_0 <= result_reg_6_0;
	result_reg_7_1 <= result_reg_6_1;
	result_reg_7_2 <= result_reg_6_2;
	result_reg_7_3 <= result_reg_6_3;
	out_valid_8 <= out_valid_7;
	result_reg_8_0 <= result_reg_7_0;
	result_reg_8_1 <= result_reg_7_1;
	result_reg_8_2 <= result_reg_7_2;
	result_reg_8_3 <= result_reg_7_3;
	out_valid_9 <= out_valid_8;
	result_reg_9_0 <= result_reg_8_0;
	result_reg_9_1 <= result_reg_8_1;
	result_reg_9_2 <= result_reg_8_2;
	result_reg_9_3 <= result_reg_8_3;
	out_valid_10 <= out_valid_9;
	result_reg_10_0 <= result_reg_9_0;
	result_reg_10_1 <= result_reg_9_1;
	result_reg_10_2 <= result_reg_9_2;
	result_reg_10_3 <= result_reg_9_3;
	out_valid_11 <= out_valid_10;
	result_reg_11_0 <= result_reg_10_0;
	result_reg_11_1 <= result_reg_10_1;
	result_reg_11_2 <= result_reg_10_2;
	result_reg_11_3 <= result_reg_10_3;
end

// FSM Combinational Logic
always @ (state or i_valid or serialize_count) begin
	if(state == 3'b000) begin
		if(i_valid == 1'b1)begin
			next_state = 3'b010;
		end else begin
			next_state = 3'b000;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b0) begin
			next_state = 3'b100;
		end else begin
			next_state = 3'b010;
		end
	end else if (state == 3'b100) begin
		if(serialize_count == 3)begin
			next_state = 3'b000;
		end else begin
			next_state = 3'b100;
		end
	end else begin
		next_state = 3'b000;
	end
end

// FSM Sequential Logic
always @ (posedge clk) begin
	if (state == 3'b000) begin
		serialize_count <= 0;
		out_valid_0 <= 0;
		if (i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end else begin
			serialize_reg_0_0 <= adders_res_0_0;
			serialize_reg_1_0 <= adders_res_1_0;
			serialize_reg_2_0 <= adders_res_2_0;
			serialize_reg_3_0 <= adders_res_3_0;
			serialize_reg_0_1 <= adders_res_0_1;
			serialize_reg_1_1 <= adders_res_1_1;
			serialize_reg_2_1 <= adders_res_2_1;
			serialize_reg_3_1 <= adders_res_3_1;
			serialize_reg_0_2 <= adders_res_0_2;
			serialize_reg_1_2 <= adders_res_1_2;
			serialize_reg_2_2 <= adders_res_2_2;
			serialize_reg_3_2 <= adders_res_3_2;
			serialize_reg_0_3 <= adders_res_0_3;
			serialize_reg_1_3 <= adders_res_1_3;
			serialize_reg_2_3 <= adders_res_2_3;
			serialize_reg_3_3 <= adders_res_3_3;
			serialize_reg_0_4 <= adders_res_0_4;
			serialize_reg_1_4 <= adders_res_1_4;
			serialize_reg_2_4 <= adders_res_2_4;
			serialize_reg_3_4 <= adders_res_3_4;
			serialize_reg_0_5 <= adders_res_0_5;
			serialize_reg_1_5 <= adders_res_1_5;
			serialize_reg_2_5 <= adders_res_2_5;
			serialize_reg_3_5 <= adders_res_3_5;
		end
	end else if (state == 3'b100) begin
		if (serialize_count < 3) begin
			serialize_reg_0_0 <= serialize_reg_1_0;
			serialize_reg_0_1 <= serialize_reg_1_1;
			serialize_reg_0_2 <= serialize_reg_1_2;
			serialize_reg_0_3 <= serialize_reg_1_3;
			serialize_reg_0_4 <= serialize_reg_1_4;
			serialize_reg_0_5 <= serialize_reg_1_5;
			serialize_reg_1_0 <= serialize_reg_2_0;
			serialize_reg_1_1 <= serialize_reg_2_1;
			serialize_reg_1_2 <= serialize_reg_2_2;
			serialize_reg_1_3 <= serialize_reg_2_3;
			serialize_reg_1_4 <= serialize_reg_2_4;
			serialize_reg_1_5 <= serialize_reg_2_5;
			serialize_reg_2_0 <= serialize_reg_3_0;
			serialize_reg_2_1 <= serialize_reg_3_1;
			serialize_reg_2_2 <= serialize_reg_3_2;
			serialize_reg_2_3 <= serialize_reg_3_3;
			serialize_reg_2_4 <= serialize_reg_3_4;
			serialize_reg_2_5 <= serialize_reg_3_5;
			serialize_count <= serialize_count + 1'b1;
			out_valid_0 <= 1;
		end
	end
	state <=  next_state;
end

// AT Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA0 (
	.data0x(i_result_0),
	.data1x(i_result_1),
	.data2x(i_result_2),
	.data3x(i_result_3),
	.data4x(i_result_4),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(adders_res_0_0)
);
wire [29:0] f1, f2;
assign f1 = -i_result_2;
assign f2 = -{i_result_4[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA1 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f1),
	.data3x({i_result_3[28:0], 1'b0}),
	.data4x(f2),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_1_0)
);
wire [29:0] f3, f4;
assign f3 = -i_result_2;
assign f4 = -{i_result_4[27:0], 2'b00};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA2 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f3),
	.data3x({i_result_3[27:0], 2'b00}),
	.data4x(f4),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_2_0)
);
wire [29:0] f5, f6;
assign f5 = -i_result_2;
assign f6 = -{i_result_4[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA3 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f5),
	.data3x({i_result_3[26:0], 3'b000}),
	.data4x(f6),
	.data5x(i_result_5),
	.clock(clk),
	.result(adders_res_3_0)
);

// A Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT0 (
	.data0x(serialize_reg_0_5),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x(serialize_reg_0_2),
	.data4x(serialize_reg_0_1),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(result_reg_0_0)
);
wire [29:0] f7, f8;
assign f7 = -serialize_reg_0_3;
assign f8 = -{serialize_reg_0_1[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT1 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f7),
	.data3x({serialize_reg_0_2[28:0], 1'b0}),
	.data4x(f8),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_1)
);
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT2 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x({serialize_reg_0_2[27:0], 2'b00}),
	.data4x({serialize_reg_0_1[27:0], 2'b00}),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_2)
);
wire [29:0] f9, f10;
assign f9 = -serialize_reg_0_3;
assign f10 = -{serialize_reg_0_1[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT3 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f9),
	.data3x({serialize_reg_0_2[26:0], 3'b000}),
	.data4x(f10),
	.data5x(serialize_reg_0_0),
	.clock(clk),
	.result(result_reg_0_3)
);

reg [15:0] result_wire_0;
reg [15:0] result_wire_1;
reg [15:0] result_wire_2;
reg [15:0] result_wire_3;

always @ (*) begin
	if(result_reg_11_0[29] == 1'b0) begin
		result_wire_0 <= result_reg_11_0[23:8];
	end else begin
		result_wire_0 <= { (16){1'b0} };
	end
	if(result_reg_11_1[29] == 1'b0) begin
		result_wire_1 <= result_reg_11_1[23:8];
	end else begin
		result_wire_1 <= { (16){1'b0} };
	end
	if(result_reg_11_2[29] == 1'b0) begin
		result_wire_2 <= result_reg_11_2[23:8];
	end else begin
		result_wire_2 <= { (16){1'b0} };
	end
	if(result_reg_11_3[29] == 1'b0) begin
		result_wire_3 <= result_reg_11_3[23:8];
	end else begin
		result_wire_3 <= { (16){1'b0} };
	end
end

assign o_valid = out_valid_11;
assign o_result_0 = result_wire_0;
assign o_result_1 = result_wire_1;
assign o_result_2 = result_wire_2;
assign o_result_3 = result_wire_3;

endmodule

module inverse_winograd_4 (
	input clk,
	input i_valid,
	input [29:0] i_result_0,
	input [29:0] i_result_1,
	input [29:0] i_result_2,
	input [29:0] i_result_3,
	input [29:0] i_result_4,
	input [29:0] i_result_5,
	output [15:0] o_result_0,
	output [15:0] o_result_1,
	output [15:0] o_result_2,
	output [15:0] o_result_3,
	output o_valid
);

reg [2:0] state, next_state;
wire [29:0] adders_res_0_0;
reg [29:0] serialize_reg_0_0;
reg [29:0] adders_res_0_1;
reg [29:0] serialize_reg_0_1;
reg [29:0] adders_res_0_2;
reg [29:0] serialize_reg_0_2;
reg [29:0] adders_res_0_3;
reg [29:0] serialize_reg_0_3;
reg [29:0] adders_res_0_4;
reg [29:0] serialize_reg_0_4;
reg [29:0] adders_res_0_5;
reg [29:0] serialize_reg_0_5;
wire [29:0] adders_res_1_0;
reg [29:0] serialize_reg_1_0;
reg [29:0] adders_res_1_1;
reg [29:0] serialize_reg_1_1;
reg [29:0] adders_res_1_2;
reg [29:0] serialize_reg_1_2;
reg [29:0] adders_res_1_3;
reg [29:0] serialize_reg_1_3;
reg [29:0] adders_res_1_4;
reg [29:0] serialize_reg_1_4;
reg [29:0] adders_res_1_5;
reg [29:0] serialize_reg_1_5;
wire [29:0] adders_res_2_0;
reg [29:0] serialize_reg_2_0;
reg [29:0] adders_res_2_1;
reg [29:0] serialize_reg_2_1;
reg [29:0] adders_res_2_2;
reg [29:0] serialize_reg_2_2;
reg [29:0] adders_res_2_3;
reg [29:0] serialize_reg_2_3;
reg [29:0] adders_res_2_4;
reg [29:0] serialize_reg_2_4;
reg [29:0] adders_res_2_5;
reg [29:0] serialize_reg_2_5;
wire [29:0] adders_res_3_0;
reg [29:0] serialize_reg_3_0;
reg [29:0] adders_res_3_1;
reg [29:0] serialize_reg_3_1;
reg [29:0] adders_res_3_2;
reg [29:0] serialize_reg_3_2;
reg [29:0] adders_res_3_3;
reg [29:0] serialize_reg_3_3;
reg [29:0] adders_res_3_4;
reg [29:0] serialize_reg_3_4;
reg [29:0] adders_res_3_5;
reg [29:0] serialize_reg_3_5;

wire [29:0] result_reg_0_0;
wire [29:0] result_reg_0_1;
wire [29:0] result_reg_0_2;
wire [29:0] result_reg_0_3;
reg [29:0] result_reg_1_0;
reg [29:0] result_reg_1_1;
reg [29:0] result_reg_1_2;
reg [29:0] result_reg_1_3;
reg [29:0] result_reg_2_0;
reg [29:0] result_reg_2_1;
reg [29:0] result_reg_2_2;
reg [29:0] result_reg_2_3;
reg [29:0] result_reg_3_0;
reg [29:0] result_reg_3_1;
reg [29:0] result_reg_3_2;
reg [29:0] result_reg_3_3;
reg [29:0] result_reg_4_0;
reg [29:0] result_reg_4_1;
reg [29:0] result_reg_4_2;
reg [29:0] result_reg_4_3;
reg [29:0] result_reg_5_0;
reg [29:0] result_reg_5_1;
reg [29:0] result_reg_5_2;
reg [29:0] result_reg_5_3;
reg [29:0] result_reg_6_0;
reg [29:0] result_reg_6_1;
reg [29:0] result_reg_6_2;
reg [29:0] result_reg_6_3;
reg [29:0] result_reg_7_0;
reg [29:0] result_reg_7_1;
reg [29:0] result_reg_7_2;
reg [29:0] result_reg_7_3;
reg [29:0] result_reg_8_0;
reg [29:0] result_reg_8_1;
reg [29:0] result_reg_8_2;
reg [29:0] result_reg_8_3;
reg [29:0] result_reg_9_0;
reg [29:0] result_reg_9_1;
reg [29:0] result_reg_9_2;
reg [29:0] result_reg_9_3;
reg [29:0] result_reg_10_0;
reg [29:0] result_reg_10_1;
reg [29:0] result_reg_10_2;
reg [29:0] result_reg_10_3;
reg [29:0] result_reg_11_0;
reg [29:0] result_reg_11_1;
reg [29:0] result_reg_11_2;
reg [29:0] result_reg_11_3;
reg [29:0] result_reg_12_0;
reg [29:0] result_reg_12_1;
reg [29:0] result_reg_12_2;
reg [29:0] result_reg_12_3;

reg out_valid_0;
reg out_valid_1;
reg out_valid_2;
reg out_valid_3;
reg out_valid_4;
reg out_valid_5;
reg out_valid_6;
reg out_valid_7;
reg out_valid_8;
reg out_valid_9;
reg out_valid_10;
reg out_valid_11;
reg out_valid_12;
reg [1:0] serialize_count;

always @ (posedge clk) begin
	out_valid_1 <= out_valid_0;
	result_reg_1_0 <= result_reg_0_0;
	result_reg_1_1 <= result_reg_0_1;
	result_reg_1_2 <= result_reg_0_2;
	result_reg_1_3 <= result_reg_0_3;
	out_valid_2 <= out_valid_1;
	result_reg_2_0 <= result_reg_1_0;
	result_reg_2_1 <= result_reg_1_1;
	result_reg_2_2 <= result_reg_1_2;
	result_reg_2_3 <= result_reg_1_3;
	out_valid_3 <= out_valid_2;
	result_reg_3_0 <= result_reg_2_0;
	result_reg_3_1 <= result_reg_2_1;
	result_reg_3_2 <= result_reg_2_2;
	result_reg_3_3 <= result_reg_2_3;
	out_valid_4 <= out_valid_3;
	result_reg_4_0 <= result_reg_3_0;
	result_reg_4_1 <= result_reg_3_1;
	result_reg_4_2 <= result_reg_3_2;
	result_reg_4_3 <= result_reg_3_3;
	out_valid_5 <= out_valid_4;
	result_reg_5_0 <= result_reg_4_0;
	result_reg_5_1 <= result_reg_4_1;
	result_reg_5_2 <= result_reg_4_2;
	result_reg_5_3 <= result_reg_4_3;
	out_valid_6 <= out_valid_5;
	result_reg_6_0 <= result_reg_5_0;
	result_reg_6_1 <= result_reg_5_1;
	result_reg_6_2 <= result_reg_5_2;
	result_reg_6_3 <= result_reg_5_3;
	out_valid_7 <= out_valid_6;
	result_reg_7_0 <= result_reg_6_0;
	result_reg_7_1 <= result_reg_6_1;
	result_reg_7_2 <= result_reg_6_2;
	result_reg_7_3 <= result_reg_6_3;
	out_valid_8 <= out_valid_7;
	result_reg_8_0 <= result_reg_7_0;
	result_reg_8_1 <= result_reg_7_1;
	result_reg_8_2 <= result_reg_7_2;
	result_reg_8_3 <= result_reg_7_3;
	out_valid_9 <= out_valid_8;
	result_reg_9_0 <= result_reg_8_0;
	result_reg_9_1 <= result_reg_8_1;
	result_reg_9_2 <= result_reg_8_2;
	result_reg_9_3 <= result_reg_8_3;
	out_valid_10 <= out_valid_9;
	result_reg_10_0 <= result_reg_9_0;
	result_reg_10_1 <= result_reg_9_1;
	result_reg_10_2 <= result_reg_9_2;
	result_reg_10_3 <= result_reg_9_3;
	out_valid_11 <= out_valid_10;
	result_reg_11_0 <= result_reg_10_0;
	result_reg_11_1 <= result_reg_10_1;
	result_reg_11_2 <= result_reg_10_2;
	result_reg_11_3 <= result_reg_10_3;
	out_valid_12 <= out_valid_11;
	result_reg_12_0 <= result_reg_11_0;
	result_reg_12_1 <= result_reg_11_1;
	result_reg_12_2 <= result_reg_11_2;
	result_reg_12_3 <= result_reg_11_3;
end

// FSM Combinational Logic
always @ (state or i_valid or serialize_count) begin
	if(state == 3'b000) begin
		if(i_valid == 1'b1)begin
			next_state = 3'b010;
		end else begin
			next_state = 3'b000;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b0) begin
			next_state = 3'b100;
		end else begin
			next_state = 3'b010;
		end
	end else if (state == 3'b100) begin
		if(serialize_count == 3)begin
			next_state = 3'b000;
		end else begin
			next_state = 3'b100;
		end
	end else begin
		next_state = 3'b000;
	end
end

// FSM Sequential Logic
always @ (posedge clk) begin
	if (state == 3'b000) begin
		serialize_count <= 0;
		out_valid_0 <= 0;
		if (i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end else begin
			serialize_reg_0_0 <= adders_res_0_0;
			serialize_reg_1_0 <= adders_res_1_0;
			serialize_reg_2_0 <= adders_res_2_0;
			serialize_reg_3_0 <= adders_res_3_0;
			serialize_reg_0_1 <= adders_res_0_1;
			serialize_reg_1_1 <= adders_res_1_1;
			serialize_reg_2_1 <= adders_res_2_1;
			serialize_reg_3_1 <= adders_res_3_1;
			serialize_reg_0_2 <= adders_res_0_2;
			serialize_reg_1_2 <= adders_res_1_2;
			serialize_reg_2_2 <= adders_res_2_2;
			serialize_reg_3_2 <= adders_res_3_2;
			serialize_reg_0_3 <= adders_res_0_3;
			serialize_reg_1_3 <= adders_res_1_3;
			serialize_reg_2_3 <= adders_res_2_3;
			serialize_reg_3_3 <= adders_res_3_3;
			serialize_reg_0_4 <= adders_res_0_4;
			serialize_reg_1_4 <= adders_res_1_4;
			serialize_reg_2_4 <= adders_res_2_4;
			serialize_reg_3_4 <= adders_res_3_4;
			serialize_reg_0_5 <= adders_res_0_5;
			serialize_reg_1_5 <= adders_res_1_5;
			serialize_reg_2_5 <= adders_res_2_5;
			serialize_reg_3_5 <= adders_res_3_5;
		end
	end else if (state == 3'b100) begin
		if (serialize_count < 3) begin
			serialize_reg_0_0 <= serialize_reg_1_0;
			serialize_reg_0_1 <= serialize_reg_1_1;
			serialize_reg_0_2 <= serialize_reg_1_2;
			serialize_reg_0_3 <= serialize_reg_1_3;
			serialize_reg_0_4 <= serialize_reg_1_4;
			serialize_reg_0_5 <= serialize_reg_1_5;
			serialize_reg_1_0 <= serialize_reg_2_0;
			serialize_reg_1_1 <= serialize_reg_2_1;
			serialize_reg_1_2 <= serialize_reg_2_2;
			serialize_reg_1_3 <= serialize_reg_2_3;
			serialize_reg_1_4 <= serialize_reg_2_4;
			serialize_reg_1_5 <= serialize_reg_2_5;
			serialize_reg_2_0 <= serialize_reg_3_0;
			serialize_reg_2_1 <= serialize_reg_3_1;
			serialize_reg_2_2 <= serialize_reg_3_2;
			serialize_reg_2_3 <= serialize_reg_3_3;
			serialize_reg_2_4 <= serialize_reg_3_4;
			serialize_reg_2_5 <= serialize_reg_3_5;
			serialize_count <= serialize_count + 1'b1;
			out_valid_0 <= 1;
		end
	end
	state <=  next_state;
end

// AT Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA0 (
	.data0x(i_result_0),
	.data1x(i_result_1),
	.data2x(i_result_2),
	.data3x(i_result_3),
	.data4x(i_result_4),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(adders_res_0_0)
);
wire [29:0] f1, f2;
assign f1 = -i_result_2;
assign f2 = -{i_result_4[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA1 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f1),
	.data3x({i_result_3[28:0], 1'b0}),
	.data4x(f2),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_1_0)
);
wire [29:0] f3, f4;
assign f3 = -i_result_2;
assign f4 = -{i_result_4[27:0], 2'b00};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA2 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f3),
	.data3x({i_result_3[27:0], 2'b00}),
	.data4x(f4),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_2_0)
);
wire [29:0] f5, f6;
assign f5 = -i_result_2;
assign f6 = -{i_result_4[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA3 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f5),
	.data3x({i_result_3[26:0], 3'b000}),
	.data4x(f6),
	.data5x(i_result_5),
	.clock(clk),
	.result(adders_res_3_0)
);

// A Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT0 (
	.data0x(serialize_reg_0_5),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x(serialize_reg_0_2),
	.data4x(serialize_reg_0_1),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(result_reg_0_0)
);
wire [29:0] f7, f8;
assign f7 = -serialize_reg_0_3;
assign f8 = -{serialize_reg_0_1[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT1 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f7),
	.data3x({serialize_reg_0_2[28:0], 1'b0}),
	.data4x(f8),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_1)
);
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT2 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x({serialize_reg_0_2[27:0], 2'b00}),
	.data4x({serialize_reg_0_1[27:0], 2'b00}),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_2)
);
wire [29:0] f9, f10;
assign f9 = -serialize_reg_0_3;
assign f10 = -{serialize_reg_0_1[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT3 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f9),
	.data3x({serialize_reg_0_2[26:0], 3'b000}),
	.data4x(f10),
	.data5x(serialize_reg_0_0),
	.clock(clk),
	.result(result_reg_0_3)
);

reg [15:0] result_wire_0;
reg [15:0] result_wire_1;
reg [15:0] result_wire_2;
reg [15:0] result_wire_3;

always @ (*) begin
	if(result_reg_12_0[29] == 1'b0) begin
		result_wire_0 <= result_reg_12_0[23:8];
	end else begin
		result_wire_0 <= { (16){1'b0} };
	end
	if(result_reg_12_1[29] == 1'b0) begin
		result_wire_1 <= result_reg_12_1[23:8];
	end else begin
		result_wire_1 <= { (16){1'b0} };
	end
	if(result_reg_12_2[29] == 1'b0) begin
		result_wire_2 <= result_reg_12_2[23:8];
	end else begin
		result_wire_2 <= { (16){1'b0} };
	end
	if(result_reg_12_3[29] == 1'b0) begin
		result_wire_3 <= result_reg_12_3[23:8];
	end else begin
		result_wire_3 <= { (16){1'b0} };
	end
end

assign o_valid = out_valid_12;
assign o_result_0 = result_wire_0;
assign o_result_1 = result_wire_1;
assign o_result_2 = result_wire_2;
assign o_result_3 = result_wire_3;

endmodule

module inverse_winograd_7 (
	input clk,
	input i_valid,
	input [29:0] i_result_0,
	input [29:0] i_result_1,
	input [29:0] i_result_2,
	input [29:0] i_result_3,
	input [29:0] i_result_4,
	input [29:0] i_result_5,
	output [15:0] o_result_0,
	output [15:0] o_result_1,
	output [15:0] o_result_2,
	output [15:0] o_result_3,
	output o_valid
);

reg [2:0] state, next_state;
wire [29:0] adders_res_0_0;
reg [29:0] serialize_reg_0_0;
reg [29:0] adders_res_0_1;
reg [29:0] serialize_reg_0_1;
reg [29:0] adders_res_0_2;
reg [29:0] serialize_reg_0_2;
reg [29:0] adders_res_0_3;
reg [29:0] serialize_reg_0_3;
reg [29:0] adders_res_0_4;
reg [29:0] serialize_reg_0_4;
reg [29:0] adders_res_0_5;
reg [29:0] serialize_reg_0_5;
wire [29:0] adders_res_1_0;
reg [29:0] serialize_reg_1_0;
reg [29:0] adders_res_1_1;
reg [29:0] serialize_reg_1_1;
reg [29:0] adders_res_1_2;
reg [29:0] serialize_reg_1_2;
reg [29:0] adders_res_1_3;
reg [29:0] serialize_reg_1_3;
reg [29:0] adders_res_1_4;
reg [29:0] serialize_reg_1_4;
reg [29:0] adders_res_1_5;
reg [29:0] serialize_reg_1_5;
wire [29:0] adders_res_2_0;
reg [29:0] serialize_reg_2_0;
reg [29:0] adders_res_2_1;
reg [29:0] serialize_reg_2_1;
reg [29:0] adders_res_2_2;
reg [29:0] serialize_reg_2_2;
reg [29:0] adders_res_2_3;
reg [29:0] serialize_reg_2_3;
reg [29:0] adders_res_2_4;
reg [29:0] serialize_reg_2_4;
reg [29:0] adders_res_2_5;
reg [29:0] serialize_reg_2_5;
wire [29:0] adders_res_3_0;
reg [29:0] serialize_reg_3_0;
reg [29:0] adders_res_3_1;
reg [29:0] serialize_reg_3_1;
reg [29:0] adders_res_3_2;
reg [29:0] serialize_reg_3_2;
reg [29:0] adders_res_3_3;
reg [29:0] serialize_reg_3_3;
reg [29:0] adders_res_3_4;
reg [29:0] serialize_reg_3_4;
reg [29:0] adders_res_3_5;
reg [29:0] serialize_reg_3_5;

wire [29:0] result_reg_0_0;
wire [29:0] result_reg_0_1;
wire [29:0] result_reg_0_2;
wire [29:0] result_reg_0_3;
reg [29:0] result_reg_1_0;
reg [29:0] result_reg_1_1;
reg [29:0] result_reg_1_2;
reg [29:0] result_reg_1_3;
reg [29:0] result_reg_2_0;
reg [29:0] result_reg_2_1;
reg [29:0] result_reg_2_2;
reg [29:0] result_reg_2_3;
reg [29:0] result_reg_3_0;
reg [29:0] result_reg_3_1;
reg [29:0] result_reg_3_2;
reg [29:0] result_reg_3_3;
reg [29:0] result_reg_4_0;
reg [29:0] result_reg_4_1;
reg [29:0] result_reg_4_2;
reg [29:0] result_reg_4_3;
reg [29:0] result_reg_5_0;
reg [29:0] result_reg_5_1;
reg [29:0] result_reg_5_2;
reg [29:0] result_reg_5_3;
reg [29:0] result_reg_6_0;
reg [29:0] result_reg_6_1;
reg [29:0] result_reg_6_2;
reg [29:0] result_reg_6_3;
reg [29:0] result_reg_7_0;
reg [29:0] result_reg_7_1;
reg [29:0] result_reg_7_2;
reg [29:0] result_reg_7_3;
reg [29:0] result_reg_8_0;
reg [29:0] result_reg_8_1;
reg [29:0] result_reg_8_2;
reg [29:0] result_reg_8_3;
reg [29:0] result_reg_9_0;
reg [29:0] result_reg_9_1;
reg [29:0] result_reg_9_2;
reg [29:0] result_reg_9_3;

reg out_valid_0;
reg out_valid_1;
reg out_valid_2;
reg out_valid_3;
reg out_valid_4;
reg out_valid_5;
reg out_valid_6;
reg out_valid_7;
reg out_valid_8;
reg out_valid_9;
reg [1:0] serialize_count;

always @ (posedge clk) begin
	out_valid_1 <= out_valid_0;
	result_reg_1_0 <= result_reg_0_0;
	result_reg_1_1 <= result_reg_0_1;
	result_reg_1_2 <= result_reg_0_2;
	result_reg_1_3 <= result_reg_0_3;
	out_valid_2 <= out_valid_1;
	result_reg_2_0 <= result_reg_1_0;
	result_reg_2_1 <= result_reg_1_1;
	result_reg_2_2 <= result_reg_1_2;
	result_reg_2_3 <= result_reg_1_3;
	out_valid_3 <= out_valid_2;
	result_reg_3_0 <= result_reg_2_0;
	result_reg_3_1 <= result_reg_2_1;
	result_reg_3_2 <= result_reg_2_2;
	result_reg_3_3 <= result_reg_2_3;
	out_valid_4 <= out_valid_3;
	result_reg_4_0 <= result_reg_3_0;
	result_reg_4_1 <= result_reg_3_1;
	result_reg_4_2 <= result_reg_3_2;
	result_reg_4_3 <= result_reg_3_3;
	out_valid_5 <= out_valid_4;
	result_reg_5_0 <= result_reg_4_0;
	result_reg_5_1 <= result_reg_4_1;
	result_reg_5_2 <= result_reg_4_2;
	result_reg_5_3 <= result_reg_4_3;
	out_valid_6 <= out_valid_5;
	result_reg_6_0 <= result_reg_5_0;
	result_reg_6_1 <= result_reg_5_1;
	result_reg_6_2 <= result_reg_5_2;
	result_reg_6_3 <= result_reg_5_3;
	out_valid_7 <= out_valid_6;
	result_reg_7_0 <= result_reg_6_0;
	result_reg_7_1 <= result_reg_6_1;
	result_reg_7_2 <= result_reg_6_2;
	result_reg_7_3 <= result_reg_6_3;
	out_valid_8 <= out_valid_7;
	result_reg_8_0 <= result_reg_7_0;
	result_reg_8_1 <= result_reg_7_1;
	result_reg_8_2 <= result_reg_7_2;
	result_reg_8_3 <= result_reg_7_3;
	out_valid_9 <= out_valid_8;
	result_reg_9_0 <= result_reg_8_0;
	result_reg_9_1 <= result_reg_8_1;
	result_reg_9_2 <= result_reg_8_2;
	result_reg_9_3 <= result_reg_8_3;
end

// FSM Combinational Logic
always @ (state or i_valid or serialize_count) begin
	if(state == 3'b000) begin
		if(i_valid == 1'b1)begin
			next_state = 3'b010;
		end else begin
			next_state = 3'b000;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b0) begin
			next_state = 3'b100;
		end else begin
			next_state = 3'b010;
		end
	end else if (state == 3'b100) begin
		if(serialize_count == 3)begin
			next_state = 3'b000;
		end else begin
			next_state = 3'b100;
		end
	end else begin
		next_state = 3'b000;
	end
end

// FSM Sequential Logic
always @ (posedge clk) begin
	if (state == 3'b000) begin
		serialize_count <= 0;
		out_valid_0 <= 0;
		if (i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end else begin
			serialize_reg_0_0 <= adders_res_0_0;
			serialize_reg_1_0 <= adders_res_1_0;
			serialize_reg_2_0 <= adders_res_2_0;
			serialize_reg_3_0 <= adders_res_3_0;
			serialize_reg_0_1 <= adders_res_0_1;
			serialize_reg_1_1 <= adders_res_1_1;
			serialize_reg_2_1 <= adders_res_2_1;
			serialize_reg_3_1 <= adders_res_3_1;
			serialize_reg_0_2 <= adders_res_0_2;
			serialize_reg_1_2 <= adders_res_1_2;
			serialize_reg_2_2 <= adders_res_2_2;
			serialize_reg_3_2 <= adders_res_3_2;
			serialize_reg_0_3 <= adders_res_0_3;
			serialize_reg_1_3 <= adders_res_1_3;
			serialize_reg_2_3 <= adders_res_2_3;
			serialize_reg_3_3 <= adders_res_3_3;
			serialize_reg_0_4 <= adders_res_0_4;
			serialize_reg_1_4 <= adders_res_1_4;
			serialize_reg_2_4 <= adders_res_2_4;
			serialize_reg_3_4 <= adders_res_3_4;
			serialize_reg_0_5 <= adders_res_0_5;
			serialize_reg_1_5 <= adders_res_1_5;
			serialize_reg_2_5 <= adders_res_2_5;
			serialize_reg_3_5 <= adders_res_3_5;
		end
	end else if (state == 3'b100) begin
		if (serialize_count < 3) begin
			serialize_reg_0_0 <= serialize_reg_1_0;
			serialize_reg_0_1 <= serialize_reg_1_1;
			serialize_reg_0_2 <= serialize_reg_1_2;
			serialize_reg_0_3 <= serialize_reg_1_3;
			serialize_reg_0_4 <= serialize_reg_1_4;
			serialize_reg_0_5 <= serialize_reg_1_5;
			serialize_reg_1_0 <= serialize_reg_2_0;
			serialize_reg_1_1 <= serialize_reg_2_1;
			serialize_reg_1_2 <= serialize_reg_2_2;
			serialize_reg_1_3 <= serialize_reg_2_3;
			serialize_reg_1_4 <= serialize_reg_2_4;
			serialize_reg_1_5 <= serialize_reg_2_5;
			serialize_reg_2_0 <= serialize_reg_3_0;
			serialize_reg_2_1 <= serialize_reg_3_1;
			serialize_reg_2_2 <= serialize_reg_3_2;
			serialize_reg_2_3 <= serialize_reg_3_3;
			serialize_reg_2_4 <= serialize_reg_3_4;
			serialize_reg_2_5 <= serialize_reg_3_5;
			serialize_count <= serialize_count + 1'b1;
			out_valid_0 <= 1;
		end
	end
	state <=  next_state;
end

// AT Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA0 (
	.data0x(i_result_0),
	.data1x(i_result_1),
	.data2x(i_result_2),
	.data3x(i_result_3),
	.data4x(i_result_4),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(adders_res_0_0)
);
wire [29:0] f1, f2;
assign f1 = -i_result_2;
assign f2 = -{i_result_4[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA1 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f1),
	.data3x({i_result_3[28:0], 1'b0}),
	.data4x(f2),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_1_0)
);
wire [29:0] f3, f4;
assign f3 = -i_result_2;
assign f4 = -{i_result_4[27:0], 2'b00};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA2 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f3),
	.data3x({i_result_3[27:0], 2'b00}),
	.data4x(f4),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_2_0)
);
wire [29:0] f5, f6;
assign f5 = -i_result_2;
assign f6 = -{i_result_4[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA3 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f5),
	.data3x({i_result_3[26:0], 3'b000}),
	.data4x(f6),
	.data5x(i_result_5),
	.clock(clk),
	.result(adders_res_3_0)
);

// A Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT0 (
	.data0x(serialize_reg_0_5),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x(serialize_reg_0_2),
	.data4x(serialize_reg_0_1),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(result_reg_0_0)
);
wire [29:0] f7, f8;
assign f7 = -serialize_reg_0_3;
assign f8 = -{serialize_reg_0_1[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT1 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f7),
	.data3x({serialize_reg_0_2[28:0], 1'b0}),
	.data4x(f8),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_1)
);
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT2 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x({serialize_reg_0_2[27:0], 2'b00}),
	.data4x({serialize_reg_0_1[27:0], 2'b00}),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_2)
);
wire [29:0] f9, f10;
assign f9 = -serialize_reg_0_3;
assign f10 = -{serialize_reg_0_1[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT3 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f9),
	.data3x({serialize_reg_0_2[26:0], 3'b000}),
	.data4x(f10),
	.data5x(serialize_reg_0_0),
	.clock(clk),
	.result(result_reg_0_3)
);

reg [15:0] result_wire_0;
reg [15:0] result_wire_1;
reg [15:0] result_wire_2;
reg [15:0] result_wire_3;

always @ (*) begin
	if(result_reg_9_0[29] == 1'b0) begin
		result_wire_0 <= result_reg_9_0[23:8];
	end else begin
		result_wire_0 <= { (16){1'b0} };
	end
	if(result_reg_9_1[29] == 1'b0) begin
		result_wire_1 <= result_reg_9_1[23:8];
	end else begin
		result_wire_1 <= { (16){1'b0} };
	end
	if(result_reg_9_2[29] == 1'b0) begin
		result_wire_2 <= result_reg_9_2[23:8];
	end else begin
		result_wire_2 <= { (16){1'b0} };
	end
	if(result_reg_9_3[29] == 1'b0) begin
		result_wire_3 <= result_reg_9_3[23:8];
	end else begin
		result_wire_3 <= { (16){1'b0} };
	end
end

assign o_valid = out_valid_9;
assign o_result_0 = result_wire_0;
assign o_result_1 = result_wire_1;
assign o_result_2 = result_wire_2;
assign o_result_3 = result_wire_3;

endmodule

module inverse_winograd_6 (
	input clk,
	input i_valid,
	input [29:0] i_result_0,
	input [29:0] i_result_1,
	input [29:0] i_result_2,
	input [29:0] i_result_3,
	input [29:0] i_result_4,
	input [29:0] i_result_5,
	output [15:0] o_result_0,
	output [15:0] o_result_1,
	output [15:0] o_result_2,
	output [15:0] o_result_3,
	output o_valid
);

reg [2:0] state, next_state;
wire [29:0] adders_res_0_0;
reg [29:0] serialize_reg_0_0;
reg [29:0] adders_res_0_1;
reg [29:0] serialize_reg_0_1;
reg [29:0] adders_res_0_2;
reg [29:0] serialize_reg_0_2;
reg [29:0] adders_res_0_3;
reg [29:0] serialize_reg_0_3;
reg [29:0] adders_res_0_4;
reg [29:0] serialize_reg_0_4;
reg [29:0] adders_res_0_5;
reg [29:0] serialize_reg_0_5;
wire [29:0] adders_res_1_0;
reg [29:0] serialize_reg_1_0;
reg [29:0] adders_res_1_1;
reg [29:0] serialize_reg_1_1;
reg [29:0] adders_res_1_2;
reg [29:0] serialize_reg_1_2;
reg [29:0] adders_res_1_3;
reg [29:0] serialize_reg_1_3;
reg [29:0] adders_res_1_4;
reg [29:0] serialize_reg_1_4;
reg [29:0] adders_res_1_5;
reg [29:0] serialize_reg_1_5;
wire [29:0] adders_res_2_0;
reg [29:0] serialize_reg_2_0;
reg [29:0] adders_res_2_1;
reg [29:0] serialize_reg_2_1;
reg [29:0] adders_res_2_2;
reg [29:0] serialize_reg_2_2;
reg [29:0] adders_res_2_3;
reg [29:0] serialize_reg_2_3;
reg [29:0] adders_res_2_4;
reg [29:0] serialize_reg_2_4;
reg [29:0] adders_res_2_5;
reg [29:0] serialize_reg_2_5;
wire [29:0] adders_res_3_0;
reg [29:0] serialize_reg_3_0;
reg [29:0] adders_res_3_1;
reg [29:0] serialize_reg_3_1;
reg [29:0] adders_res_3_2;
reg [29:0] serialize_reg_3_2;
reg [29:0] adders_res_3_3;
reg [29:0] serialize_reg_3_3;
reg [29:0] adders_res_3_4;
reg [29:0] serialize_reg_3_4;
reg [29:0] adders_res_3_5;
reg [29:0] serialize_reg_3_5;

wire [29:0] result_reg_0_0;
wire [29:0] result_reg_0_1;
wire [29:0] result_reg_0_2;
wire [29:0] result_reg_0_3;
reg [29:0] result_reg_1_0;
reg [29:0] result_reg_1_1;
reg [29:0] result_reg_1_2;
reg [29:0] result_reg_1_3;
reg [29:0] result_reg_2_0;
reg [29:0] result_reg_2_1;
reg [29:0] result_reg_2_2;
reg [29:0] result_reg_2_3;
reg [29:0] result_reg_3_0;
reg [29:0] result_reg_3_1;
reg [29:0] result_reg_3_2;
reg [29:0] result_reg_3_3;
reg [29:0] result_reg_4_0;
reg [29:0] result_reg_4_1;
reg [29:0] result_reg_4_2;
reg [29:0] result_reg_4_3;
reg [29:0] result_reg_5_0;
reg [29:0] result_reg_5_1;
reg [29:0] result_reg_5_2;
reg [29:0] result_reg_5_3;
reg [29:0] result_reg_6_0;
reg [29:0] result_reg_6_1;
reg [29:0] result_reg_6_2;
reg [29:0] result_reg_6_3;
reg [29:0] result_reg_7_0;
reg [29:0] result_reg_7_1;
reg [29:0] result_reg_7_2;
reg [29:0] result_reg_7_3;
reg [29:0] result_reg_8_0;
reg [29:0] result_reg_8_1;
reg [29:0] result_reg_8_2;
reg [29:0] result_reg_8_3;
reg [29:0] result_reg_9_0;
reg [29:0] result_reg_9_1;
reg [29:0] result_reg_9_2;
reg [29:0] result_reg_9_3;
reg [29:0] result_reg_10_0;
reg [29:0] result_reg_10_1;
reg [29:0] result_reg_10_2;
reg [29:0] result_reg_10_3;

reg out_valid_0;
reg out_valid_1;
reg out_valid_2;
reg out_valid_3;
reg out_valid_4;
reg out_valid_5;
reg out_valid_6;
reg out_valid_7;
reg out_valid_8;
reg out_valid_9;
reg out_valid_10;
reg [1:0] serialize_count;

always @ (posedge clk) begin
	out_valid_1 <= out_valid_0;
	result_reg_1_0 <= result_reg_0_0;
	result_reg_1_1 <= result_reg_0_1;
	result_reg_1_2 <= result_reg_0_2;
	result_reg_1_3 <= result_reg_0_3;
	out_valid_2 <= out_valid_1;
	result_reg_2_0 <= result_reg_1_0;
	result_reg_2_1 <= result_reg_1_1;
	result_reg_2_2 <= result_reg_1_2;
	result_reg_2_3 <= result_reg_1_3;
	out_valid_3 <= out_valid_2;
	result_reg_3_0 <= result_reg_2_0;
	result_reg_3_1 <= result_reg_2_1;
	result_reg_3_2 <= result_reg_2_2;
	result_reg_3_3 <= result_reg_2_3;
	out_valid_4 <= out_valid_3;
	result_reg_4_0 <= result_reg_3_0;
	result_reg_4_1 <= result_reg_3_1;
	result_reg_4_2 <= result_reg_3_2;
	result_reg_4_3 <= result_reg_3_3;
	out_valid_5 <= out_valid_4;
	result_reg_5_0 <= result_reg_4_0;
	result_reg_5_1 <= result_reg_4_1;
	result_reg_5_2 <= result_reg_4_2;
	result_reg_5_3 <= result_reg_4_3;
	out_valid_6 <= out_valid_5;
	result_reg_6_0 <= result_reg_5_0;
	result_reg_6_1 <= result_reg_5_1;
	result_reg_6_2 <= result_reg_5_2;
	result_reg_6_3 <= result_reg_5_3;
	out_valid_7 <= out_valid_6;
	result_reg_7_0 <= result_reg_6_0;
	result_reg_7_1 <= result_reg_6_1;
	result_reg_7_2 <= result_reg_6_2;
	result_reg_7_3 <= result_reg_6_3;
	out_valid_8 <= out_valid_7;
	result_reg_8_0 <= result_reg_7_0;
	result_reg_8_1 <= result_reg_7_1;
	result_reg_8_2 <= result_reg_7_2;
	result_reg_8_3 <= result_reg_7_3;
	out_valid_9 <= out_valid_8;
	result_reg_9_0 <= result_reg_8_0;
	result_reg_9_1 <= result_reg_8_1;
	result_reg_9_2 <= result_reg_8_2;
	result_reg_9_3 <= result_reg_8_3;
	out_valid_10 <= out_valid_9;
	result_reg_10_0 <= result_reg_9_0;
	result_reg_10_1 <= result_reg_9_1;
	result_reg_10_2 <= result_reg_9_2;
	result_reg_10_3 <= result_reg_9_3;
end

// FSM Combinational Logic
always @ (state or i_valid or serialize_count) begin
	if(state == 3'b000) begin
		if(i_valid == 1'b1)begin
			next_state = 3'b010;
		end else begin
			next_state = 3'b000;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b0) begin
			next_state = 3'b100;
		end else begin
			next_state = 3'b010;
		end
	end else if (state == 3'b100) begin
		if(serialize_count == 3)begin
			next_state = 3'b000;
		end else begin
			next_state = 3'b100;
		end
	end else begin
		next_state = 3'b000;
	end
end

// FSM Sequential Logic
always @ (posedge clk) begin
	if (state == 3'b000) begin
		serialize_count <= 0;
		out_valid_0 <= 0;
		if (i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end else begin
			serialize_reg_0_0 <= adders_res_0_0;
			serialize_reg_1_0 <= adders_res_1_0;
			serialize_reg_2_0 <= adders_res_2_0;
			serialize_reg_3_0 <= adders_res_3_0;
			serialize_reg_0_1 <= adders_res_0_1;
			serialize_reg_1_1 <= adders_res_1_1;
			serialize_reg_2_1 <= adders_res_2_1;
			serialize_reg_3_1 <= adders_res_3_1;
			serialize_reg_0_2 <= adders_res_0_2;
			serialize_reg_1_2 <= adders_res_1_2;
			serialize_reg_2_2 <= adders_res_2_2;
			serialize_reg_3_2 <= adders_res_3_2;
			serialize_reg_0_3 <= adders_res_0_3;
			serialize_reg_1_3 <= adders_res_1_3;
			serialize_reg_2_3 <= adders_res_2_3;
			serialize_reg_3_3 <= adders_res_3_3;
			serialize_reg_0_4 <= adders_res_0_4;
			serialize_reg_1_4 <= adders_res_1_4;
			serialize_reg_2_4 <= adders_res_2_4;
			serialize_reg_3_4 <= adders_res_3_4;
			serialize_reg_0_5 <= adders_res_0_5;
			serialize_reg_1_5 <= adders_res_1_5;
			serialize_reg_2_5 <= adders_res_2_5;
			serialize_reg_3_5 <= adders_res_3_5;
		end
	end else if (state == 3'b100) begin
		if (serialize_count < 3) begin
			serialize_reg_0_0 <= serialize_reg_1_0;
			serialize_reg_0_1 <= serialize_reg_1_1;
			serialize_reg_0_2 <= serialize_reg_1_2;
			serialize_reg_0_3 <= serialize_reg_1_3;
			serialize_reg_0_4 <= serialize_reg_1_4;
			serialize_reg_0_5 <= serialize_reg_1_5;
			serialize_reg_1_0 <= serialize_reg_2_0;
			serialize_reg_1_1 <= serialize_reg_2_1;
			serialize_reg_1_2 <= serialize_reg_2_2;
			serialize_reg_1_3 <= serialize_reg_2_3;
			serialize_reg_1_4 <= serialize_reg_2_4;
			serialize_reg_1_5 <= serialize_reg_2_5;
			serialize_reg_2_0 <= serialize_reg_3_0;
			serialize_reg_2_1 <= serialize_reg_3_1;
			serialize_reg_2_2 <= serialize_reg_3_2;
			serialize_reg_2_3 <= serialize_reg_3_3;
			serialize_reg_2_4 <= serialize_reg_3_4;
			serialize_reg_2_5 <= serialize_reg_3_5;
			serialize_count <= serialize_count + 1'b1;
			out_valid_0 <= 1;
		end
	end
	state <=  next_state;
end

// AT Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA0 (
	.data0x(i_result_0),
	.data1x(i_result_1),
	.data2x(i_result_2),
	.data3x(i_result_3),
	.data4x(i_result_4),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(adders_res_0_0)
);
wire [29:0] f1, f2;
assign f1 = -i_result_2;
assign f2 = -{i_result_4[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA1 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f1),
	.data3x({i_result_3[28:0], 1'b0}),
	.data4x(f2),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_1_0)
);
wire [29:0] f3, f4;
assign f3 = -i_result_2;
assign f4 = -{i_result_4[27:0], 2'b00};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA2 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f3),
	.data3x({i_result_3[27:0], 2'b00}),
	.data4x(f4),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_2_0)
);
wire [29:0] f5, f6;
assign f5 = -i_result_2;
assign f6 = -{i_result_4[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA3 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f5),
	.data3x({i_result_3[26:0], 3'b000}),
	.data4x(f6),
	.data5x(i_result_5),
	.clock(clk),
	.result(adders_res_3_0)
);

// A Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT0 (
	.data0x(serialize_reg_0_5),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x(serialize_reg_0_2),
	.data4x(serialize_reg_0_1),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(result_reg_0_0)
);
wire [29:0] f7, f8;
assign f7 = -serialize_reg_0_3;
assign f8 = -{serialize_reg_0_1[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT1 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f7),
	.data3x({serialize_reg_0_2[28:0], 1'b0}),
	.data4x(f8),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_1)
);
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT2 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x({serialize_reg_0_2[27:0], 2'b00}),
	.data4x({serialize_reg_0_1[27:0], 2'b00}),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_2)
);
wire [29:0] f9, f10;
assign f9 = -serialize_reg_0_3;
assign f10 = -{serialize_reg_0_1[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT3 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f9),
	.data3x({serialize_reg_0_2[26:0], 3'b000}),
	.data4x(f10),
	.data5x(serialize_reg_0_0),
	.clock(clk),
	.result(result_reg_0_3)
);

reg [15:0] result_wire_0;
reg [15:0] result_wire_1;
reg [15:0] result_wire_2;
reg [15:0] result_wire_3;

always @ (*) begin
	if(result_reg_10_0[29] == 1'b0) begin
		result_wire_0 <= result_reg_10_0[23:8];
	end else begin
		result_wire_0 <= { (16){1'b0} };
	end
	if(result_reg_10_1[29] == 1'b0) begin
		result_wire_1 <= result_reg_10_1[23:8];
	end else begin
		result_wire_1 <= { (16){1'b0} };
	end
	if(result_reg_10_2[29] == 1'b0) begin
		result_wire_2 <= result_reg_10_2[23:8];
	end else begin
		result_wire_2 <= { (16){1'b0} };
	end
	if(result_reg_10_3[29] == 1'b0) begin
		result_wire_3 <= result_reg_10_3[23:8];
	end else begin
		result_wire_3 <= { (16){1'b0} };
	end
end

assign o_valid = out_valid_10;
assign o_result_0 = result_wire_0;
assign o_result_1 = result_wire_1;
assign o_result_2 = result_wire_2;
assign o_result_3 = result_wire_3;

endmodule

module inverse_winograd_9 (
	input clk,
	input i_valid,
	input [29:0] i_result_0,
	input [29:0] i_result_1,
	input [29:0] i_result_2,
	input [29:0] i_result_3,
	input [29:0] i_result_4,
	input [29:0] i_result_5,
	output [15:0] o_result_0,
	output [15:0] o_result_1,
	output [15:0] o_result_2,
	output [15:0] o_result_3,
	output o_valid
);

reg [2:0] state, next_state;
wire [29:0] adders_res_0_0;
reg [29:0] serialize_reg_0_0;
reg [29:0] adders_res_0_1;
reg [29:0] serialize_reg_0_1;
reg [29:0] adders_res_0_2;
reg [29:0] serialize_reg_0_2;
reg [29:0] adders_res_0_3;
reg [29:0] serialize_reg_0_3;
reg [29:0] adders_res_0_4;
reg [29:0] serialize_reg_0_4;
reg [29:0] adders_res_0_5;
reg [29:0] serialize_reg_0_5;
wire [29:0] adders_res_1_0;
reg [29:0] serialize_reg_1_0;
reg [29:0] adders_res_1_1;
reg [29:0] serialize_reg_1_1;
reg [29:0] adders_res_1_2;
reg [29:0] serialize_reg_1_2;
reg [29:0] adders_res_1_3;
reg [29:0] serialize_reg_1_3;
reg [29:0] adders_res_1_4;
reg [29:0] serialize_reg_1_4;
reg [29:0] adders_res_1_5;
reg [29:0] serialize_reg_1_5;
wire [29:0] adders_res_2_0;
reg [29:0] serialize_reg_2_0;
reg [29:0] adders_res_2_1;
reg [29:0] serialize_reg_2_1;
reg [29:0] adders_res_2_2;
reg [29:0] serialize_reg_2_2;
reg [29:0] adders_res_2_3;
reg [29:0] serialize_reg_2_3;
reg [29:0] adders_res_2_4;
reg [29:0] serialize_reg_2_4;
reg [29:0] adders_res_2_5;
reg [29:0] serialize_reg_2_5;
wire [29:0] adders_res_3_0;
reg [29:0] serialize_reg_3_0;
reg [29:0] adders_res_3_1;
reg [29:0] serialize_reg_3_1;
reg [29:0] adders_res_3_2;
reg [29:0] serialize_reg_3_2;
reg [29:0] adders_res_3_3;
reg [29:0] serialize_reg_3_3;
reg [29:0] adders_res_3_4;
reg [29:0] serialize_reg_3_4;
reg [29:0] adders_res_3_5;
reg [29:0] serialize_reg_3_5;

wire [29:0] result_reg_0_0;
wire [29:0] result_reg_0_1;
wire [29:0] result_reg_0_2;
wire [29:0] result_reg_0_3;
reg [29:0] result_reg_1_0;
reg [29:0] result_reg_1_1;
reg [29:0] result_reg_1_2;
reg [29:0] result_reg_1_3;
reg [29:0] result_reg_2_0;
reg [29:0] result_reg_2_1;
reg [29:0] result_reg_2_2;
reg [29:0] result_reg_2_3;
reg [29:0] result_reg_3_0;
reg [29:0] result_reg_3_1;
reg [29:0] result_reg_3_2;
reg [29:0] result_reg_3_3;
reg [29:0] result_reg_4_0;
reg [29:0] result_reg_4_1;
reg [29:0] result_reg_4_2;
reg [29:0] result_reg_4_3;
reg [29:0] result_reg_5_0;
reg [29:0] result_reg_5_1;
reg [29:0] result_reg_5_2;
reg [29:0] result_reg_5_3;
reg [29:0] result_reg_6_0;
reg [29:0] result_reg_6_1;
reg [29:0] result_reg_6_2;
reg [29:0] result_reg_6_3;
reg [29:0] result_reg_7_0;
reg [29:0] result_reg_7_1;
reg [29:0] result_reg_7_2;
reg [29:0] result_reg_7_3;

reg out_valid_0;
reg out_valid_1;
reg out_valid_2;
reg out_valid_3;
reg out_valid_4;
reg out_valid_5;
reg out_valid_6;
reg out_valid_7;
reg [1:0] serialize_count;

always @ (posedge clk) begin
	out_valid_1 <= out_valid_0;
	result_reg_1_0 <= result_reg_0_0;
	result_reg_1_1 <= result_reg_0_1;
	result_reg_1_2 <= result_reg_0_2;
	result_reg_1_3 <= result_reg_0_3;
	out_valid_2 <= out_valid_1;
	result_reg_2_0 <= result_reg_1_0;
	result_reg_2_1 <= result_reg_1_1;
	result_reg_2_2 <= result_reg_1_2;
	result_reg_2_3 <= result_reg_1_3;
	out_valid_3 <= out_valid_2;
	result_reg_3_0 <= result_reg_2_0;
	result_reg_3_1 <= result_reg_2_1;
	result_reg_3_2 <= result_reg_2_2;
	result_reg_3_3 <= result_reg_2_3;
	out_valid_4 <= out_valid_3;
	result_reg_4_0 <= result_reg_3_0;
	result_reg_4_1 <= result_reg_3_1;
	result_reg_4_2 <= result_reg_3_2;
	result_reg_4_3 <= result_reg_3_3;
	out_valid_5 <= out_valid_4;
	result_reg_5_0 <= result_reg_4_0;
	result_reg_5_1 <= result_reg_4_1;
	result_reg_5_2 <= result_reg_4_2;
	result_reg_5_3 <= result_reg_4_3;
	out_valid_6 <= out_valid_5;
	result_reg_6_0 <= result_reg_5_0;
	result_reg_6_1 <= result_reg_5_1;
	result_reg_6_2 <= result_reg_5_2;
	result_reg_6_3 <= result_reg_5_3;
	out_valid_7 <= out_valid_6;
	result_reg_7_0 <= result_reg_6_0;
	result_reg_7_1 <= result_reg_6_1;
	result_reg_7_2 <= result_reg_6_2;
	result_reg_7_3 <= result_reg_6_3;
end

// FSM Combinational Logic
always @ (state or i_valid or serialize_count) begin
	if(state == 3'b000) begin
		if(i_valid == 1'b1)begin
			next_state = 3'b010;
		end else begin
			next_state = 3'b000;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b0) begin
			next_state = 3'b100;
		end else begin
			next_state = 3'b010;
		end
	end else if (state == 3'b100) begin
		if(serialize_count == 3)begin
			next_state = 3'b000;
		end else begin
			next_state = 3'b100;
		end
	end else begin
		next_state = 3'b000;
	end
end

// FSM Sequential Logic
always @ (posedge clk) begin
	if (state == 3'b000) begin
		serialize_count <= 0;
		out_valid_0 <= 0;
		if (i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end else begin
			serialize_reg_0_0 <= adders_res_0_0;
			serialize_reg_1_0 <= adders_res_1_0;
			serialize_reg_2_0 <= adders_res_2_0;
			serialize_reg_3_0 <= adders_res_3_0;
			serialize_reg_0_1 <= adders_res_0_1;
			serialize_reg_1_1 <= adders_res_1_1;
			serialize_reg_2_1 <= adders_res_2_1;
			serialize_reg_3_1 <= adders_res_3_1;
			serialize_reg_0_2 <= adders_res_0_2;
			serialize_reg_1_2 <= adders_res_1_2;
			serialize_reg_2_2 <= adders_res_2_2;
			serialize_reg_3_2 <= adders_res_3_2;
			serialize_reg_0_3 <= adders_res_0_3;
			serialize_reg_1_3 <= adders_res_1_3;
			serialize_reg_2_3 <= adders_res_2_3;
			serialize_reg_3_3 <= adders_res_3_3;
			serialize_reg_0_4 <= adders_res_0_4;
			serialize_reg_1_4 <= adders_res_1_4;
			serialize_reg_2_4 <= adders_res_2_4;
			serialize_reg_3_4 <= adders_res_3_4;
			serialize_reg_0_5 <= adders_res_0_5;
			serialize_reg_1_5 <= adders_res_1_5;
			serialize_reg_2_5 <= adders_res_2_5;
			serialize_reg_3_5 <= adders_res_3_5;
		end
	end else if (state == 3'b100) begin
		if (serialize_count < 3) begin
			serialize_reg_0_0 <= serialize_reg_1_0;
			serialize_reg_0_1 <= serialize_reg_1_1;
			serialize_reg_0_2 <= serialize_reg_1_2;
			serialize_reg_0_3 <= serialize_reg_1_3;
			serialize_reg_0_4 <= serialize_reg_1_4;
			serialize_reg_0_5 <= serialize_reg_1_5;
			serialize_reg_1_0 <= serialize_reg_2_0;
			serialize_reg_1_1 <= serialize_reg_2_1;
			serialize_reg_1_2 <= serialize_reg_2_2;
			serialize_reg_1_3 <= serialize_reg_2_3;
			serialize_reg_1_4 <= serialize_reg_2_4;
			serialize_reg_1_5 <= serialize_reg_2_5;
			serialize_reg_2_0 <= serialize_reg_3_0;
			serialize_reg_2_1 <= serialize_reg_3_1;
			serialize_reg_2_2 <= serialize_reg_3_2;
			serialize_reg_2_3 <= serialize_reg_3_3;
			serialize_reg_2_4 <= serialize_reg_3_4;
			serialize_reg_2_5 <= serialize_reg_3_5;
			serialize_count <= serialize_count + 1'b1;
			out_valid_0 <= 1;
		end
	end
	state <=  next_state;
end

// AT Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA0 (
	.data0x(i_result_0),
	.data1x(i_result_1),
	.data2x(i_result_2),
	.data3x(i_result_3),
	.data4x(i_result_4),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(adders_res_0_0)
);
wire [29:0] f1, f2;
assign f1 = -i_result_2;
assign f2 = -{i_result_4[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA1 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f1),
	.data3x({i_result_3[28:0], 1'b0}),
	.data4x(f2),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_1_0)
);
wire [29:0] f3, f4;
assign f3 = -i_result_2;
assign f4 = -{i_result_4[27:0], 2'b00};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA2 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f3),
	.data3x({i_result_3[27:0], 2'b00}),
	.data4x(f4),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_2_0)
);
wire [29:0] f5, f6;
assign f5 = -i_result_2;
assign f6 = -{i_result_4[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA3 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f5),
	.data3x({i_result_3[26:0], 3'b000}),
	.data4x(f6),
	.data5x(i_result_5),
	.clock(clk),
	.result(adders_res_3_0)
);

// A Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT0 (
	.data0x(serialize_reg_0_5),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x(serialize_reg_0_2),
	.data4x(serialize_reg_0_1),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(result_reg_0_0)
);
wire [29:0] f7, f8;
assign f7 = -serialize_reg_0_3;
assign f8 = -{serialize_reg_0_1[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT1 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f7),
	.data3x({serialize_reg_0_2[28:0], 1'b0}),
	.data4x(f8),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_1)
);
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT2 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x({serialize_reg_0_2[27:0], 2'b00}),
	.data4x({serialize_reg_0_1[27:0], 2'b00}),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_2)
);
wire [29:0] f9, f10;
assign f9 = -serialize_reg_0_3;
assign f10 = -{serialize_reg_0_1[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT3 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f9),
	.data3x({serialize_reg_0_2[26:0], 3'b000}),
	.data4x(f10),
	.data5x(serialize_reg_0_0),
	.clock(clk),
	.result(result_reg_0_3)
);

reg [15:0] result_wire_0;
reg [15:0] result_wire_1;
reg [15:0] result_wire_2;
reg [15:0] result_wire_3;

always @ (*) begin
	if(result_reg_7_0[29] == 1'b0) begin
		result_wire_0 <= result_reg_7_0[23:8];
	end else begin
		result_wire_0 <= { (16){1'b0} };
	end
	if(result_reg_7_1[29] == 1'b0) begin
		result_wire_1 <= result_reg_7_1[23:8];
	end else begin
		result_wire_1 <= { (16){1'b0} };
	end
	if(result_reg_7_2[29] == 1'b0) begin
		result_wire_2 <= result_reg_7_2[23:8];
	end else begin
		result_wire_2 <= { (16){1'b0} };
	end
	if(result_reg_7_3[29] == 1'b0) begin
		result_wire_3 <= result_reg_7_3[23:8];
	end else begin
		result_wire_3 <= { (16){1'b0} };
	end
end

assign o_valid = out_valid_7;
assign o_result_0 = result_wire_0;
assign o_result_1 = result_wire_1;
assign o_result_2 = result_wire_2;
assign o_result_3 = result_wire_3;

endmodule

module inverse_winograd_8 (
	input clk,
	input i_valid,
	input [29:0] i_result_0,
	input [29:0] i_result_1,
	input [29:0] i_result_2,
	input [29:0] i_result_3,
	input [29:0] i_result_4,
	input [29:0] i_result_5,
	output [15:0] o_result_0,
	output [15:0] o_result_1,
	output [15:0] o_result_2,
	output [15:0] o_result_3,
	output o_valid
);

reg [2:0] state, next_state;
wire [29:0] adders_res_0_0;
reg [29:0] serialize_reg_0_0;
reg [29:0] adders_res_0_1;
reg [29:0] serialize_reg_0_1;
reg [29:0] adders_res_0_2;
reg [29:0] serialize_reg_0_2;
reg [29:0] adders_res_0_3;
reg [29:0] serialize_reg_0_3;
reg [29:0] adders_res_0_4;
reg [29:0] serialize_reg_0_4;
reg [29:0] adders_res_0_5;
reg [29:0] serialize_reg_0_5;
wire [29:0] adders_res_1_0;
reg [29:0] serialize_reg_1_0;
reg [29:0] adders_res_1_1;
reg [29:0] serialize_reg_1_1;
reg [29:0] adders_res_1_2;
reg [29:0] serialize_reg_1_2;
reg [29:0] adders_res_1_3;
reg [29:0] serialize_reg_1_3;
reg [29:0] adders_res_1_4;
reg [29:0] serialize_reg_1_4;
reg [29:0] adders_res_1_5;
reg [29:0] serialize_reg_1_5;
wire [29:0] adders_res_2_0;
reg [29:0] serialize_reg_2_0;
reg [29:0] adders_res_2_1;
reg [29:0] serialize_reg_2_1;
reg [29:0] adders_res_2_2;
reg [29:0] serialize_reg_2_2;
reg [29:0] adders_res_2_3;
reg [29:0] serialize_reg_2_3;
reg [29:0] adders_res_2_4;
reg [29:0] serialize_reg_2_4;
reg [29:0] adders_res_2_5;
reg [29:0] serialize_reg_2_5;
wire [29:0] adders_res_3_0;
reg [29:0] serialize_reg_3_0;
reg [29:0] adders_res_3_1;
reg [29:0] serialize_reg_3_1;
reg [29:0] adders_res_3_2;
reg [29:0] serialize_reg_3_2;
reg [29:0] adders_res_3_3;
reg [29:0] serialize_reg_3_3;
reg [29:0] adders_res_3_4;
reg [29:0] serialize_reg_3_4;
reg [29:0] adders_res_3_5;
reg [29:0] serialize_reg_3_5;

wire [29:0] result_reg_0_0;
wire [29:0] result_reg_0_1;
wire [29:0] result_reg_0_2;
wire [29:0] result_reg_0_3;
reg [29:0] result_reg_1_0;
reg [29:0] result_reg_1_1;
reg [29:0] result_reg_1_2;
reg [29:0] result_reg_1_3;
reg [29:0] result_reg_2_0;
reg [29:0] result_reg_2_1;
reg [29:0] result_reg_2_2;
reg [29:0] result_reg_2_3;
reg [29:0] result_reg_3_0;
reg [29:0] result_reg_3_1;
reg [29:0] result_reg_3_2;
reg [29:0] result_reg_3_3;
reg [29:0] result_reg_4_0;
reg [29:0] result_reg_4_1;
reg [29:0] result_reg_4_2;
reg [29:0] result_reg_4_3;
reg [29:0] result_reg_5_0;
reg [29:0] result_reg_5_1;
reg [29:0] result_reg_5_2;
reg [29:0] result_reg_5_3;
reg [29:0] result_reg_6_0;
reg [29:0] result_reg_6_1;
reg [29:0] result_reg_6_2;
reg [29:0] result_reg_6_3;
reg [29:0] result_reg_7_0;
reg [29:0] result_reg_7_1;
reg [29:0] result_reg_7_2;
reg [29:0] result_reg_7_3;
reg [29:0] result_reg_8_0;
reg [29:0] result_reg_8_1;
reg [29:0] result_reg_8_2;
reg [29:0] result_reg_8_3;

reg out_valid_0;
reg out_valid_1;
reg out_valid_2;
reg out_valid_3;
reg out_valid_4;
reg out_valid_5;
reg out_valid_6;
reg out_valid_7;
reg out_valid_8;
reg [1:0] serialize_count;

always @ (posedge clk) begin
	out_valid_1 <= out_valid_0;
	result_reg_1_0 <= result_reg_0_0;
	result_reg_1_1 <= result_reg_0_1;
	result_reg_1_2 <= result_reg_0_2;
	result_reg_1_3 <= result_reg_0_3;
	out_valid_2 <= out_valid_1;
	result_reg_2_0 <= result_reg_1_0;
	result_reg_2_1 <= result_reg_1_1;
	result_reg_2_2 <= result_reg_1_2;
	result_reg_2_3 <= result_reg_1_3;
	out_valid_3 <= out_valid_2;
	result_reg_3_0 <= result_reg_2_0;
	result_reg_3_1 <= result_reg_2_1;
	result_reg_3_2 <= result_reg_2_2;
	result_reg_3_3 <= result_reg_2_3;
	out_valid_4 <= out_valid_3;
	result_reg_4_0 <= result_reg_3_0;
	result_reg_4_1 <= result_reg_3_1;
	result_reg_4_2 <= result_reg_3_2;
	result_reg_4_3 <= result_reg_3_3;
	out_valid_5 <= out_valid_4;
	result_reg_5_0 <= result_reg_4_0;
	result_reg_5_1 <= result_reg_4_1;
	result_reg_5_2 <= result_reg_4_2;
	result_reg_5_3 <= result_reg_4_3;
	out_valid_6 <= out_valid_5;
	result_reg_6_0 <= result_reg_5_0;
	result_reg_6_1 <= result_reg_5_1;
	result_reg_6_2 <= result_reg_5_2;
	result_reg_6_3 <= result_reg_5_3;
	out_valid_7 <= out_valid_6;
	result_reg_7_0 <= result_reg_6_0;
	result_reg_7_1 <= result_reg_6_1;
	result_reg_7_2 <= result_reg_6_2;
	result_reg_7_3 <= result_reg_6_3;
	out_valid_8 <= out_valid_7;
	result_reg_8_0 <= result_reg_7_0;
	result_reg_8_1 <= result_reg_7_1;
	result_reg_8_2 <= result_reg_7_2;
	result_reg_8_3 <= result_reg_7_3;
end

// FSM Combinational Logic
always @ (state or i_valid or serialize_count) begin
	if(state == 3'b000) begin
		if(i_valid == 1'b1)begin
			next_state = 3'b010;
		end else begin
			next_state = 3'b000;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b0) begin
			next_state = 3'b100;
		end else begin
			next_state = 3'b010;
		end
	end else if (state == 3'b100) begin
		if(serialize_count == 3)begin
			next_state = 3'b000;
		end else begin
			next_state = 3'b100;
		end
	end else begin
		next_state = 3'b000;
	end
end

// FSM Sequential Logic
always @ (posedge clk) begin
	if (state == 3'b000) begin
		serialize_count <= 0;
		out_valid_0 <= 0;
		if (i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end else begin
			serialize_reg_0_0 <= adders_res_0_0;
			serialize_reg_1_0 <= adders_res_1_0;
			serialize_reg_2_0 <= adders_res_2_0;
			serialize_reg_3_0 <= adders_res_3_0;
			serialize_reg_0_1 <= adders_res_0_1;
			serialize_reg_1_1 <= adders_res_1_1;
			serialize_reg_2_1 <= adders_res_2_1;
			serialize_reg_3_1 <= adders_res_3_1;
			serialize_reg_0_2 <= adders_res_0_2;
			serialize_reg_1_2 <= adders_res_1_2;
			serialize_reg_2_2 <= adders_res_2_2;
			serialize_reg_3_2 <= adders_res_3_2;
			serialize_reg_0_3 <= adders_res_0_3;
			serialize_reg_1_3 <= adders_res_1_3;
			serialize_reg_2_3 <= adders_res_2_3;
			serialize_reg_3_3 <= adders_res_3_3;
			serialize_reg_0_4 <= adders_res_0_4;
			serialize_reg_1_4 <= adders_res_1_4;
			serialize_reg_2_4 <= adders_res_2_4;
			serialize_reg_3_4 <= adders_res_3_4;
			serialize_reg_0_5 <= adders_res_0_5;
			serialize_reg_1_5 <= adders_res_1_5;
			serialize_reg_2_5 <= adders_res_2_5;
			serialize_reg_3_5 <= adders_res_3_5;
		end
	end else if (state == 3'b100) begin
		if (serialize_count < 3) begin
			serialize_reg_0_0 <= serialize_reg_1_0;
			serialize_reg_0_1 <= serialize_reg_1_1;
			serialize_reg_0_2 <= serialize_reg_1_2;
			serialize_reg_0_3 <= serialize_reg_1_3;
			serialize_reg_0_4 <= serialize_reg_1_4;
			serialize_reg_0_5 <= serialize_reg_1_5;
			serialize_reg_1_0 <= serialize_reg_2_0;
			serialize_reg_1_1 <= serialize_reg_2_1;
			serialize_reg_1_2 <= serialize_reg_2_2;
			serialize_reg_1_3 <= serialize_reg_2_3;
			serialize_reg_1_4 <= serialize_reg_2_4;
			serialize_reg_1_5 <= serialize_reg_2_5;
			serialize_reg_2_0 <= serialize_reg_3_0;
			serialize_reg_2_1 <= serialize_reg_3_1;
			serialize_reg_2_2 <= serialize_reg_3_2;
			serialize_reg_2_3 <= serialize_reg_3_3;
			serialize_reg_2_4 <= serialize_reg_3_4;
			serialize_reg_2_5 <= serialize_reg_3_5;
			serialize_count <= serialize_count + 1'b1;
			out_valid_0 <= 1;
		end
	end
	state <=  next_state;
end

// AT Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA0 (
	.data0x(i_result_0),
	.data1x(i_result_1),
	.data2x(i_result_2),
	.data3x(i_result_3),
	.data4x(i_result_4),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(adders_res_0_0)
);
wire [29:0] f1, f2;
assign f1 = -i_result_2;
assign f2 = -{i_result_4[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA1 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f1),
	.data3x({i_result_3[28:0], 1'b0}),
	.data4x(f2),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_1_0)
);
wire [29:0] f3, f4;
assign f3 = -i_result_2;
assign f4 = -{i_result_4[27:0], 2'b00};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA2 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f3),
	.data3x({i_result_3[27:0], 2'b00}),
	.data4x(f4),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_2_0)
);
wire [29:0] f5, f6;
assign f5 = -i_result_2;
assign f6 = -{i_result_4[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA3 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f5),
	.data3x({i_result_3[26:0], 3'b000}),
	.data4x(f6),
	.data5x(i_result_5),
	.clock(clk),
	.result(adders_res_3_0)
);

// A Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT0 (
	.data0x(serialize_reg_0_5),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x(serialize_reg_0_2),
	.data4x(serialize_reg_0_1),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(result_reg_0_0)
);
wire [29:0] f7, f8;
assign f7 = -serialize_reg_0_3;
assign f8 = -{serialize_reg_0_1[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT1 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f7),
	.data3x({serialize_reg_0_2[28:0], 1'b0}),
	.data4x(f8),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_1)
);
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT2 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x({serialize_reg_0_2[27:0], 2'b00}),
	.data4x({serialize_reg_0_1[27:0], 2'b00}),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_2)
);
wire [29:0] f9, f10;
assign f9 = -serialize_reg_0_3;
assign f10 = -{serialize_reg_0_1[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT3 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f9),
	.data3x({serialize_reg_0_2[26:0], 3'b000}),
	.data4x(f10),
	.data5x(serialize_reg_0_0),
	.clock(clk),
	.result(result_reg_0_3)
);

reg [15:0] result_wire_0;
reg [15:0] result_wire_1;
reg [15:0] result_wire_2;
reg [15:0] result_wire_3;

always @ (*) begin
	if(result_reg_8_0[29] == 1'b0) begin
		result_wire_0 <= result_reg_8_0[23:8];
	end else begin
		result_wire_0 <= { (16){1'b0} };
	end
	if(result_reg_8_1[29] == 1'b0) begin
		result_wire_1 <= result_reg_8_1[23:8];
	end else begin
		result_wire_1 <= { (16){1'b0} };
	end
	if(result_reg_8_2[29] == 1'b0) begin
		result_wire_2 <= result_reg_8_2[23:8];
	end else begin
		result_wire_2 <= { (16){1'b0} };
	end
	if(result_reg_8_3[29] == 1'b0) begin
		result_wire_3 <= result_reg_8_3[23:8];
	end else begin
		result_wire_3 <= { (16){1'b0} };
	end
end

assign o_valid = out_valid_8;
assign o_result_0 = result_wire_0;
assign o_result_1 = result_wire_1;
assign o_result_2 = result_wire_2;
assign o_result_3 = result_wire_3;

endmodule

module inverse_winograd_11 (
	input clk,
	input i_valid,
	input [29:0] i_result_0,
	input [29:0] i_result_1,
	input [29:0] i_result_2,
	input [29:0] i_result_3,
	input [29:0] i_result_4,
	input [29:0] i_result_5,
	output [15:0] o_result_0,
	output [15:0] o_result_1,
	output [15:0] o_result_2,
	output [15:0] o_result_3,
	output o_valid
);

reg [2:0] state, next_state;
wire [29:0] adders_res_0_0;
reg [29:0] serialize_reg_0_0;
reg [29:0] adders_res_0_1;
reg [29:0] serialize_reg_0_1;
reg [29:0] adders_res_0_2;
reg [29:0] serialize_reg_0_2;
reg [29:0] adders_res_0_3;
reg [29:0] serialize_reg_0_3;
reg [29:0] adders_res_0_4;
reg [29:0] serialize_reg_0_4;
reg [29:0] adders_res_0_5;
reg [29:0] serialize_reg_0_5;
wire [29:0] adders_res_1_0;
reg [29:0] serialize_reg_1_0;
reg [29:0] adders_res_1_1;
reg [29:0] serialize_reg_1_1;
reg [29:0] adders_res_1_2;
reg [29:0] serialize_reg_1_2;
reg [29:0] adders_res_1_3;
reg [29:0] serialize_reg_1_3;
reg [29:0] adders_res_1_4;
reg [29:0] serialize_reg_1_4;
reg [29:0] adders_res_1_5;
reg [29:0] serialize_reg_1_5;
wire [29:0] adders_res_2_0;
reg [29:0] serialize_reg_2_0;
reg [29:0] adders_res_2_1;
reg [29:0] serialize_reg_2_1;
reg [29:0] adders_res_2_2;
reg [29:0] serialize_reg_2_2;
reg [29:0] adders_res_2_3;
reg [29:0] serialize_reg_2_3;
reg [29:0] adders_res_2_4;
reg [29:0] serialize_reg_2_4;
reg [29:0] adders_res_2_5;
reg [29:0] serialize_reg_2_5;
wire [29:0] adders_res_3_0;
reg [29:0] serialize_reg_3_0;
reg [29:0] adders_res_3_1;
reg [29:0] serialize_reg_3_1;
reg [29:0] adders_res_3_2;
reg [29:0] serialize_reg_3_2;
reg [29:0] adders_res_3_3;
reg [29:0] serialize_reg_3_3;
reg [29:0] adders_res_3_4;
reg [29:0] serialize_reg_3_4;
reg [29:0] adders_res_3_5;
reg [29:0] serialize_reg_3_5;

wire [29:0] result_reg_0_0;
wire [29:0] result_reg_0_1;
wire [29:0] result_reg_0_2;
wire [29:0] result_reg_0_3;
reg [29:0] result_reg_1_0;
reg [29:0] result_reg_1_1;
reg [29:0] result_reg_1_2;
reg [29:0] result_reg_1_3;
reg [29:0] result_reg_2_0;
reg [29:0] result_reg_2_1;
reg [29:0] result_reg_2_2;
reg [29:0] result_reg_2_3;
reg [29:0] result_reg_3_0;
reg [29:0] result_reg_3_1;
reg [29:0] result_reg_3_2;
reg [29:0] result_reg_3_3;
reg [29:0] result_reg_4_0;
reg [29:0] result_reg_4_1;
reg [29:0] result_reg_4_2;
reg [29:0] result_reg_4_3;
reg [29:0] result_reg_5_0;
reg [29:0] result_reg_5_1;
reg [29:0] result_reg_5_2;
reg [29:0] result_reg_5_3;

reg out_valid_0;
reg out_valid_1;
reg out_valid_2;
reg out_valid_3;
reg out_valid_4;
reg out_valid_5;
reg [1:0] serialize_count;

always @ (posedge clk) begin
	out_valid_1 <= out_valid_0;
	result_reg_1_0 <= result_reg_0_0;
	result_reg_1_1 <= result_reg_0_1;
	result_reg_1_2 <= result_reg_0_2;
	result_reg_1_3 <= result_reg_0_3;
	out_valid_2 <= out_valid_1;
	result_reg_2_0 <= result_reg_1_0;
	result_reg_2_1 <= result_reg_1_1;
	result_reg_2_2 <= result_reg_1_2;
	result_reg_2_3 <= result_reg_1_3;
	out_valid_3 <= out_valid_2;
	result_reg_3_0 <= result_reg_2_0;
	result_reg_3_1 <= result_reg_2_1;
	result_reg_3_2 <= result_reg_2_2;
	result_reg_3_3 <= result_reg_2_3;
	out_valid_4 <= out_valid_3;
	result_reg_4_0 <= result_reg_3_0;
	result_reg_4_1 <= result_reg_3_1;
	result_reg_4_2 <= result_reg_3_2;
	result_reg_4_3 <= result_reg_3_3;
	out_valid_5 <= out_valid_4;
	result_reg_5_0 <= result_reg_4_0;
	result_reg_5_1 <= result_reg_4_1;
	result_reg_5_2 <= result_reg_4_2;
	result_reg_5_3 <= result_reg_4_3;
end

// FSM Combinational Logic
always @ (state or i_valid or serialize_count) begin
	if(state == 3'b000) begin
		if(i_valid == 1'b1)begin
			next_state = 3'b010;
		end else begin
			next_state = 3'b000;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b0) begin
			next_state = 3'b100;
		end else begin
			next_state = 3'b010;
		end
	end else if (state == 3'b100) begin
		if(serialize_count == 3)begin
			next_state = 3'b000;
		end else begin
			next_state = 3'b100;
		end
	end else begin
		next_state = 3'b000;
	end
end

// FSM Sequential Logic
always @ (posedge clk) begin
	if (state == 3'b000) begin
		serialize_count <= 0;
		out_valid_0 <= 0;
		if (i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end else begin
			serialize_reg_0_0 <= adders_res_0_0;
			serialize_reg_1_0 <= adders_res_1_0;
			serialize_reg_2_0 <= adders_res_2_0;
			serialize_reg_3_0 <= adders_res_3_0;
			serialize_reg_0_1 <= adders_res_0_1;
			serialize_reg_1_1 <= adders_res_1_1;
			serialize_reg_2_1 <= adders_res_2_1;
			serialize_reg_3_1 <= adders_res_3_1;
			serialize_reg_0_2 <= adders_res_0_2;
			serialize_reg_1_2 <= adders_res_1_2;
			serialize_reg_2_2 <= adders_res_2_2;
			serialize_reg_3_2 <= adders_res_3_2;
			serialize_reg_0_3 <= adders_res_0_3;
			serialize_reg_1_3 <= adders_res_1_3;
			serialize_reg_2_3 <= adders_res_2_3;
			serialize_reg_3_3 <= adders_res_3_3;
			serialize_reg_0_4 <= adders_res_0_4;
			serialize_reg_1_4 <= adders_res_1_4;
			serialize_reg_2_4 <= adders_res_2_4;
			serialize_reg_3_4 <= adders_res_3_4;
			serialize_reg_0_5 <= adders_res_0_5;
			serialize_reg_1_5 <= adders_res_1_5;
			serialize_reg_2_5 <= adders_res_2_5;
			serialize_reg_3_5 <= adders_res_3_5;
		end
	end else if (state == 3'b100) begin
		if (serialize_count < 3) begin
			serialize_reg_0_0 <= serialize_reg_1_0;
			serialize_reg_0_1 <= serialize_reg_1_1;
			serialize_reg_0_2 <= serialize_reg_1_2;
			serialize_reg_0_3 <= serialize_reg_1_3;
			serialize_reg_0_4 <= serialize_reg_1_4;
			serialize_reg_0_5 <= serialize_reg_1_5;
			serialize_reg_1_0 <= serialize_reg_2_0;
			serialize_reg_1_1 <= serialize_reg_2_1;
			serialize_reg_1_2 <= serialize_reg_2_2;
			serialize_reg_1_3 <= serialize_reg_2_3;
			serialize_reg_1_4 <= serialize_reg_2_4;
			serialize_reg_1_5 <= serialize_reg_2_5;
			serialize_reg_2_0 <= serialize_reg_3_0;
			serialize_reg_2_1 <= serialize_reg_3_1;
			serialize_reg_2_2 <= serialize_reg_3_2;
			serialize_reg_2_3 <= serialize_reg_3_3;
			serialize_reg_2_4 <= serialize_reg_3_4;
			serialize_reg_2_5 <= serialize_reg_3_5;
			serialize_count <= serialize_count + 1'b1;
			out_valid_0 <= 1;
		end
	end
	state <=  next_state;
end

// AT Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA0 (
	.data0x(i_result_0),
	.data1x(i_result_1),
	.data2x(i_result_2),
	.data3x(i_result_3),
	.data4x(i_result_4),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(adders_res_0_0)
);
wire [29:0] f1, f2;
assign f1 = -i_result_2;
assign f2 = -{i_result_4[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA1 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f1),
	.data3x({i_result_3[28:0], 1'b0}),
	.data4x(f2),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_1_0)
);
wire [29:0] f3, f4;
assign f3 = -i_result_2;
assign f4 = -{i_result_4[27:0], 2'b00};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA2 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f3),
	.data3x({i_result_3[27:0], 2'b00}),
	.data4x(f4),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_2_0)
);
wire [29:0] f5, f6;
assign f5 = -i_result_2;
assign f6 = -{i_result_4[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA3 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f5),
	.data3x({i_result_3[26:0], 3'b000}),
	.data4x(f6),
	.data5x(i_result_5),
	.clock(clk),
	.result(adders_res_3_0)
);

// A Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT0 (
	.data0x(serialize_reg_0_5),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x(serialize_reg_0_2),
	.data4x(serialize_reg_0_1),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(result_reg_0_0)
);
wire [29:0] f7, f8;
assign f7 = -serialize_reg_0_3;
assign f8 = -{serialize_reg_0_1[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT1 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f7),
	.data3x({serialize_reg_0_2[28:0], 1'b0}),
	.data4x(f8),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_1)
);
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT2 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x({serialize_reg_0_2[27:0], 2'b00}),
	.data4x({serialize_reg_0_1[27:0], 2'b00}),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_2)
);
wire [29:0] f9, f10;
assign f9 = -serialize_reg_0_3;
assign f10 = -{serialize_reg_0_1[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT3 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f9),
	.data3x({serialize_reg_0_2[26:0], 3'b000}),
	.data4x(f10),
	.data5x(serialize_reg_0_0),
	.clock(clk),
	.result(result_reg_0_3)
);

reg [15:0] result_wire_0;
reg [15:0] result_wire_1;
reg [15:0] result_wire_2;
reg [15:0] result_wire_3;

always @ (*) begin
	if(result_reg_5_0[29] == 1'b0) begin
		result_wire_0 <= result_reg_5_0[23:8];
	end else begin
		result_wire_0 <= { (16){1'b0} };
	end
	if(result_reg_5_1[29] == 1'b0) begin
		result_wire_1 <= result_reg_5_1[23:8];
	end else begin
		result_wire_1 <= { (16){1'b0} };
	end
	if(result_reg_5_2[29] == 1'b0) begin
		result_wire_2 <= result_reg_5_2[23:8];
	end else begin
		result_wire_2 <= { (16){1'b0} };
	end
	if(result_reg_5_3[29] == 1'b0) begin
		result_wire_3 <= result_reg_5_3[23:8];
	end else begin
		result_wire_3 <= { (16){1'b0} };
	end
end

assign o_valid = out_valid_5;
assign o_result_0 = result_wire_0;
assign o_result_1 = result_wire_1;
assign o_result_2 = result_wire_2;
assign o_result_3 = result_wire_3;

endmodule

module inverse_winograd_10 (
	input clk,
	input i_valid,
	input [29:0] i_result_0,
	input [29:0] i_result_1,
	input [29:0] i_result_2,
	input [29:0] i_result_3,
	input [29:0] i_result_4,
	input [29:0] i_result_5,
	output [15:0] o_result_0,
	output [15:0] o_result_1,
	output [15:0] o_result_2,
	output [15:0] o_result_3,
	output o_valid
);

reg [2:0] state, next_state;
wire [29:0] adders_res_0_0;
reg [29:0] serialize_reg_0_0;
reg [29:0] adders_res_0_1;
reg [29:0] serialize_reg_0_1;
reg [29:0] adders_res_0_2;
reg [29:0] serialize_reg_0_2;
reg [29:0] adders_res_0_3;
reg [29:0] serialize_reg_0_3;
reg [29:0] adders_res_0_4;
reg [29:0] serialize_reg_0_4;
reg [29:0] adders_res_0_5;
reg [29:0] serialize_reg_0_5;
wire [29:0] adders_res_1_0;
reg [29:0] serialize_reg_1_0;
reg [29:0] adders_res_1_1;
reg [29:0] serialize_reg_1_1;
reg [29:0] adders_res_1_2;
reg [29:0] serialize_reg_1_2;
reg [29:0] adders_res_1_3;
reg [29:0] serialize_reg_1_3;
reg [29:0] adders_res_1_4;
reg [29:0] serialize_reg_1_4;
reg [29:0] adders_res_1_5;
reg [29:0] serialize_reg_1_5;
wire [29:0] adders_res_2_0;
reg [29:0] serialize_reg_2_0;
reg [29:0] adders_res_2_1;
reg [29:0] serialize_reg_2_1;
reg [29:0] adders_res_2_2;
reg [29:0] serialize_reg_2_2;
reg [29:0] adders_res_2_3;
reg [29:0] serialize_reg_2_3;
reg [29:0] adders_res_2_4;
reg [29:0] serialize_reg_2_4;
reg [29:0] adders_res_2_5;
reg [29:0] serialize_reg_2_5;
wire [29:0] adders_res_3_0;
reg [29:0] serialize_reg_3_0;
reg [29:0] adders_res_3_1;
reg [29:0] serialize_reg_3_1;
reg [29:0] adders_res_3_2;
reg [29:0] serialize_reg_3_2;
reg [29:0] adders_res_3_3;
reg [29:0] serialize_reg_3_3;
reg [29:0] adders_res_3_4;
reg [29:0] serialize_reg_3_4;
reg [29:0] adders_res_3_5;
reg [29:0] serialize_reg_3_5;

wire [29:0] result_reg_0_0;
wire [29:0] result_reg_0_1;
wire [29:0] result_reg_0_2;
wire [29:0] result_reg_0_3;
reg [29:0] result_reg_1_0;
reg [29:0] result_reg_1_1;
reg [29:0] result_reg_1_2;
reg [29:0] result_reg_1_3;
reg [29:0] result_reg_2_0;
reg [29:0] result_reg_2_1;
reg [29:0] result_reg_2_2;
reg [29:0] result_reg_2_3;
reg [29:0] result_reg_3_0;
reg [29:0] result_reg_3_1;
reg [29:0] result_reg_3_2;
reg [29:0] result_reg_3_3;
reg [29:0] result_reg_4_0;
reg [29:0] result_reg_4_1;
reg [29:0] result_reg_4_2;
reg [29:0] result_reg_4_3;
reg [29:0] result_reg_5_0;
reg [29:0] result_reg_5_1;
reg [29:0] result_reg_5_2;
reg [29:0] result_reg_5_3;
reg [29:0] result_reg_6_0;
reg [29:0] result_reg_6_1;
reg [29:0] result_reg_6_2;
reg [29:0] result_reg_6_3;

reg out_valid_0;
reg out_valid_1;
reg out_valid_2;
reg out_valid_3;
reg out_valid_4;
reg out_valid_5;
reg out_valid_6;
reg [1:0] serialize_count;

always @ (posedge clk) begin
	out_valid_1 <= out_valid_0;
	result_reg_1_0 <= result_reg_0_0;
	result_reg_1_1 <= result_reg_0_1;
	result_reg_1_2 <= result_reg_0_2;
	result_reg_1_3 <= result_reg_0_3;
	out_valid_2 <= out_valid_1;
	result_reg_2_0 <= result_reg_1_0;
	result_reg_2_1 <= result_reg_1_1;
	result_reg_2_2 <= result_reg_1_2;
	result_reg_2_3 <= result_reg_1_3;
	out_valid_3 <= out_valid_2;
	result_reg_3_0 <= result_reg_2_0;
	result_reg_3_1 <= result_reg_2_1;
	result_reg_3_2 <= result_reg_2_2;
	result_reg_3_3 <= result_reg_2_3;
	out_valid_4 <= out_valid_3;
	result_reg_4_0 <= result_reg_3_0;
	result_reg_4_1 <= result_reg_3_1;
	result_reg_4_2 <= result_reg_3_2;
	result_reg_4_3 <= result_reg_3_3;
	out_valid_5 <= out_valid_4;
	result_reg_5_0 <= result_reg_4_0;
	result_reg_5_1 <= result_reg_4_1;
	result_reg_5_2 <= result_reg_4_2;
	result_reg_5_3 <= result_reg_4_3;
	out_valid_6 <= out_valid_5;
	result_reg_6_0 <= result_reg_5_0;
	result_reg_6_1 <= result_reg_5_1;
	result_reg_6_2 <= result_reg_5_2;
	result_reg_6_3 <= result_reg_5_3;
end

// FSM Combinational Logic
always @ (state or i_valid or serialize_count) begin
	if(state == 3'b000) begin
		if(i_valid == 1'b1)begin
			next_state = 3'b010;
		end else begin
			next_state = 3'b000;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b0) begin
			next_state = 3'b100;
		end else begin
			next_state = 3'b010;
		end
	end else if (state == 3'b100) begin
		if(serialize_count == 3)begin
			next_state = 3'b000;
		end else begin
			next_state = 3'b100;
		end
	end else begin
		next_state = 3'b000;
	end
end

// FSM Sequential Logic
always @ (posedge clk) begin
	if (state == 3'b000) begin
		serialize_count <= 0;
		out_valid_0 <= 0;
		if (i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end
	end else if (state == 3'b010) begin
		if(i_valid == 1'b1) begin
			adders_res_0_1 <= adders_res_0_0;
			adders_res_1_1 <= adders_res_1_0;
			adders_res_2_1 <= adders_res_2_0;
			adders_res_3_1 <= adders_res_3_0;
			adders_res_0_2 <= adders_res_0_1;
			adders_res_1_2 <= adders_res_1_1;
			adders_res_2_2 <= adders_res_2_1;
			adders_res_3_2 <= adders_res_3_1;
			adders_res_0_3 <= adders_res_0_2;
			adders_res_1_3 <= adders_res_1_2;
			adders_res_2_3 <= adders_res_2_2;
			adders_res_3_3 <= adders_res_3_2;
			adders_res_0_4 <= adders_res_0_3;
			adders_res_1_4 <= adders_res_1_3;
			adders_res_2_4 <= adders_res_2_3;
			adders_res_3_4 <= adders_res_3_3;
			adders_res_0_5 <= adders_res_0_4;
			adders_res_1_5 <= adders_res_1_4;
			adders_res_2_5 <= adders_res_2_4;
			adders_res_3_5 <= adders_res_3_4;
		end else begin
			serialize_reg_0_0 <= adders_res_0_0;
			serialize_reg_1_0 <= adders_res_1_0;
			serialize_reg_2_0 <= adders_res_2_0;
			serialize_reg_3_0 <= adders_res_3_0;
			serialize_reg_0_1 <= adders_res_0_1;
			serialize_reg_1_1 <= adders_res_1_1;
			serialize_reg_2_1 <= adders_res_2_1;
			serialize_reg_3_1 <= adders_res_3_1;
			serialize_reg_0_2 <= adders_res_0_2;
			serialize_reg_1_2 <= adders_res_1_2;
			serialize_reg_2_2 <= adders_res_2_2;
			serialize_reg_3_2 <= adders_res_3_2;
			serialize_reg_0_3 <= adders_res_0_3;
			serialize_reg_1_3 <= adders_res_1_3;
			serialize_reg_2_3 <= adders_res_2_3;
			serialize_reg_3_3 <= adders_res_3_3;
			serialize_reg_0_4 <= adders_res_0_4;
			serialize_reg_1_4 <= adders_res_1_4;
			serialize_reg_2_4 <= adders_res_2_4;
			serialize_reg_3_4 <= adders_res_3_4;
			serialize_reg_0_5 <= adders_res_0_5;
			serialize_reg_1_5 <= adders_res_1_5;
			serialize_reg_2_5 <= adders_res_2_5;
			serialize_reg_3_5 <= adders_res_3_5;
		end
	end else if (state == 3'b100) begin
		if (serialize_count < 3) begin
			serialize_reg_0_0 <= serialize_reg_1_0;
			serialize_reg_0_1 <= serialize_reg_1_1;
			serialize_reg_0_2 <= serialize_reg_1_2;
			serialize_reg_0_3 <= serialize_reg_1_3;
			serialize_reg_0_4 <= serialize_reg_1_4;
			serialize_reg_0_5 <= serialize_reg_1_5;
			serialize_reg_1_0 <= serialize_reg_2_0;
			serialize_reg_1_1 <= serialize_reg_2_1;
			serialize_reg_1_2 <= serialize_reg_2_2;
			serialize_reg_1_3 <= serialize_reg_2_3;
			serialize_reg_1_4 <= serialize_reg_2_4;
			serialize_reg_1_5 <= serialize_reg_2_5;
			serialize_reg_2_0 <= serialize_reg_3_0;
			serialize_reg_2_1 <= serialize_reg_3_1;
			serialize_reg_2_2 <= serialize_reg_3_2;
			serialize_reg_2_3 <= serialize_reg_3_3;
			serialize_reg_2_4 <= serialize_reg_3_4;
			serialize_reg_2_5 <= serialize_reg_3_5;
			serialize_count <= serialize_count + 1'b1;
			out_valid_0 <= 1;
		end
	end
	state <=  next_state;
end

// AT Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA0 (
	.data0x(i_result_0),
	.data1x(i_result_1),
	.data2x(i_result_2),
	.data3x(i_result_3),
	.data4x(i_result_4),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(adders_res_0_0)
);
wire [29:0] f1, f2;
assign f1 = -i_result_2;
assign f2 = -{i_result_4[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA1 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f1),
	.data3x({i_result_3[28:0], 1'b0}),
	.data4x(f2),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_1_0)
);
wire [29:0] f3, f4;
assign f3 = -i_result_2;
assign f4 = -{i_result_4[27:0], 2'b00};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA2 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f3),
	.data3x({i_result_3[27:0], 2'b00}),
	.data4x(f4),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(adders_res_2_0)
);
wire [29:0] f5, f6;
assign f5 = -i_result_2;
assign f6 = -{i_result_4[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWA3 (
	.data0x({ (30){1'b0} }),
	.data1x(i_result_1),
	.data2x(f5),
	.data3x({i_result_3[26:0], 3'b000}),
	.data4x(f6),
	.data5x(i_result_5),
	.clock(clk),
	.result(adders_res_3_0)
);

// A Adders
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT0 (
	.data0x(serialize_reg_0_5),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x(serialize_reg_0_2),
	.data4x(serialize_reg_0_1),
	.data5x({ (30) {1'b0} }),
	.clock(clk),
	.result(result_reg_0_0)
);
wire [29:0] f7, f8;
assign f7 = -serialize_reg_0_3;
assign f8 = -{serialize_reg_0_1[28:0], 1'b0};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT1 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f7),
	.data3x({serialize_reg_0_2[28:0], 1'b0}),
	.data4x(f8),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_1)
);
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT2 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(serialize_reg_0_3),
	.data3x({serialize_reg_0_2[27:0], 2'b00}),
	.data4x({serialize_reg_0_1[27:0], 2'b00}),
	.data5x({ (30){1'b0} }),
	.clock(clk),
	.result(result_reg_0_2)
);
wire [29:0] f9, f10;
assign f9 = -serialize_reg_0_3;
assign f10 = -{serialize_reg_0_1[26:0], 3'b000};
inverse_winograd_adder_30_3 inverse_winograd_adder_30_3_inst_IWAT3 (
	.data0x({ (30){1'b0} }),
	.data1x(serialize_reg_0_4),
	.data2x(f9),
	.data3x({serialize_reg_0_2[26:0], 3'b000}),
	.data4x(f10),
	.data5x(serialize_reg_0_0),
	.clock(clk),
	.result(result_reg_0_3)
);

reg [15:0] result_wire_0;
reg [15:0] result_wire_1;
reg [15:0] result_wire_2;
reg [15:0] result_wire_3;

always @ (*) begin
	if(result_reg_6_0[29] == 1'b0) begin
		result_wire_0 <= result_reg_6_0[23:8];
	end else begin
		result_wire_0 <= { (16){1'b0} };
	end
	if(result_reg_6_1[29] == 1'b0) begin
		result_wire_1 <= result_reg_6_1[23:8];
	end else begin
		result_wire_1 <= { (16){1'b0} };
	end
	if(result_reg_6_2[29] == 1'b0) begin
		result_wire_2 <= result_reg_6_2[23:8];
	end else begin
		result_wire_2 <= { (16){1'b0} };
	end
	if(result_reg_6_3[29] == 1'b0) begin
		result_wire_3 <= result_reg_6_3[23:8];
	end else begin
		result_wire_3 <= { (16){1'b0} };
	end
end

assign o_valid = out_valid_6;
assign o_result_0 = result_wire_0;
assign o_result_1 = result_wire_1;
assign o_result_2 = result_wire_2;
assign o_result_3 = result_wire_3;

endmodule

module winograd_transform_1 (
	input clk,
	input i_valid,
	input [15:0] i_result_0_0,
	input [15:0] i_result_0_1,
	input [15:0] i_result_1_0,
	input [15:0] i_result_1_1,
	input [15:0] i_result_2_0,
	input [15:0] i_result_2_1,
	input [15:0] i_result_3_0,
	input [15:0] i_result_3_1,
	input [15:0] i_result_4_0,
	input [15:0] i_result_4_1,
	input [15:0] i_result_5_0,
	input [15:0] i_result_5_1,
	output [15:0] o_feature_0,
	output [15:0] o_feature_1,
	output [15:0] o_feature_2,
	output [15:0] o_feature_3,
	output [15:0] o_feature_4,
	output [15:0] o_feature_5,
	output o_valid
);

reg [15:0] input_buffer_0_0;
reg [19:0] output_buffer_0_0;
wire [19:0] rslt_buffer_0_0;
reg [15:0] input_buffer_0_1;
reg [19:0] output_buffer_0_1;
wire [19:0] rslt_buffer_0_1;
reg [15:0] input_buffer_0_2;
reg [19:0] output_buffer_0_2;
wire [19:0] rslt_buffer_0_2;
reg [15:0] input_buffer_0_3;
reg [19:0] output_buffer_0_3;
wire [19:0] rslt_buffer_0_3;
reg [15:0] input_buffer_0_4;
reg [19:0] output_buffer_0_4;
wire [19:0] rslt_buffer_0_4;
reg [15:0] input_buffer_0_5;
reg [19:0] output_buffer_0_5;
wire [19:0] rslt_buffer_0_5;
reg [15:0] input_buffer_1_0;
reg [19:0] output_buffer_1_0;
wire [19:0] rslt_buffer_1_0;
reg [15:0] input_buffer_1_1;
reg [19:0] output_buffer_1_1;
wire [19:0] rslt_buffer_1_1;
reg [15:0] input_buffer_1_2;
reg [19:0] output_buffer_1_2;
wire [19:0] rslt_buffer_1_2;
reg [15:0] input_buffer_1_3;
reg [19:0] output_buffer_1_3;
wire [19:0] rslt_buffer_1_3;
reg [15:0] input_buffer_1_4;
reg [19:0] output_buffer_1_4;
wire [19:0] rslt_buffer_1_4;
reg [15:0] input_buffer_1_5;
reg [19:0] output_buffer_1_5;
wire [19:0] rslt_buffer_1_5;
reg [15:0] input_buffer_2_0;
reg [19:0] output_buffer_2_0;
wire [19:0] rslt_buffer_2_0;
reg [15:0] input_buffer_2_1;
reg [19:0] output_buffer_2_1;
wire [19:0] rslt_buffer_2_1;
reg [15:0] input_buffer_2_2;
reg [19:0] output_buffer_2_2;
wire [19:0] rslt_buffer_2_2;
reg [15:0] input_buffer_2_3;
reg [19:0] output_buffer_2_3;
wire [19:0] rslt_buffer_2_3;
reg [15:0] input_buffer_2_4;
reg [19:0] output_buffer_2_4;
wire [19:0] rslt_buffer_2_4;
reg [15:0] input_buffer_2_5;
reg [19:0] output_buffer_2_5;
wire [19:0] rslt_buffer_2_5;
reg [15:0] input_buffer_3_0;
reg [19:0] output_buffer_3_0;
wire [19:0] rslt_buffer_3_0;
reg [15:0] input_buffer_3_1;
reg [19:0] output_buffer_3_1;
wire [19:0] rslt_buffer_3_1;
reg [15:0] input_buffer_3_2;
reg [19:0] output_buffer_3_2;
wire [19:0] rslt_buffer_3_2;
reg [15:0] input_buffer_3_3;
reg [19:0] output_buffer_3_3;
wire [19:0] rslt_buffer_3_3;
reg [15:0] input_buffer_3_4;
reg [19:0] output_buffer_3_4;
wire [19:0] rslt_buffer_3_4;
reg [15:0] input_buffer_3_5;
reg [19:0] output_buffer_3_5;
wire [19:0] rslt_buffer_3_5;
reg [15:0] input_buffer_4_0;
reg [19:0] output_buffer_4_0;
wire [19:0] rslt_buffer_4_0;
reg [15:0] input_buffer_4_1;
reg [19:0] output_buffer_4_1;
wire [19:0] rslt_buffer_4_1;
reg [15:0] input_buffer_4_2;
reg [19:0] output_buffer_4_2;
wire [19:0] rslt_buffer_4_2;
reg [15:0] input_buffer_4_3;
reg [19:0] output_buffer_4_3;
wire [19:0] rslt_buffer_4_3;
reg [15:0] input_buffer_4_4;
reg [19:0] output_buffer_4_4;
wire [19:0] rslt_buffer_4_4;
reg [15:0] input_buffer_4_5;
reg [19:0] output_buffer_4_5;
wire [19:0] rslt_buffer_4_5;
reg [15:0] input_buffer_5_0;
reg [19:0] output_buffer_5_0;
wire [19:0] rslt_buffer_5_0;
reg [15:0] input_buffer_5_1;
reg [19:0] output_buffer_5_1;
wire [19:0] rslt_buffer_5_1;
reg [15:0] input_buffer_5_2;
reg [19:0] output_buffer_5_2;
wire [19:0] rslt_buffer_5_2;
reg [15:0] input_buffer_5_3;
reg [19:0] output_buffer_5_3;
wire [19:0] rslt_buffer_5_3;
reg [15:0] input_buffer_5_4;
reg [19:0] output_buffer_5_4;
wire [19:0] rslt_buffer_5_4;
reg [15:0] input_buffer_5_5;
reg [19:0] output_buffer_5_5;
wire [19:0] rslt_buffer_5_5;
reg calculate, calculate_1, calculate_2, calculate_3;
reg valid_0;
reg valid_1;
reg valid_2;
reg valid_3;
reg valid_4;
reg valid_5;
reg valid_6;
reg valid_7;
reg valid_8;
reg valid_9;
reg valid_10;
reg valid_11;
reg valid_12;
reg [2:0] input_buffer_count;
wire [15:0] dsp_out_1_0;
wire [15:0] dsp_out_1_1;
wire [15:0] dsp_out_1_2;
wire [15:0] dsp_out_1_3;
wire [15:0] dsp_out_1_4;
wire [15:0] dsp_out_1_5;
wire [15:0] dsp_out_1_6;
wire [15:0] dsp_out_1_7;
wire [15:0] dsp_out_1_8;
wire [15:0] dsp_out_1_9;
wire [15:0] dsp_out_1_10;
wire [15:0] dsp_out_1_11;
wire [15:0] dsp_out_2_0;
wire [15:0] dsp_out_2_1;
wire [15:0] dsp_out_2_2;
wire [15:0] dsp_out_2_3;
wire [15:0] dsp_out_2_4;
wire [15:0] dsp_out_2_5;
wire [15:0] dsp_out_2_6;
wire [15:0] dsp_out_2_7;
wire [15:0] dsp_out_3_0;
wire [15:0] dsp_out_3_1;
wire [15:0] dsp_out_3_2;
wire [15:0] dsp_out_3_3;
wire [15:0] dsp_out_3_4;
wire [15:0] dsp_out_3_5;
wire [15:0] dsp_out_3_6;
wire [15:0] dsp_out_3_7;
wire [15:0] dsp_out_4_0;
wire [15:0] dsp_out_4_1;
wire [15:0] dsp_out_4_2;
wire [15:0] dsp_out_4_3;
wire [15:0] dsp_out_4_4;
wire [15:0] dsp_out_4_5;
wire [15:0] dsp_out_4_6;
wire [15:0] dsp_out_4_7;
wire [15:0] dsp_out_5_0;
wire [15:0] dsp_out_5_1;
wire [15:0] dsp_out_5_2;
wire [15:0] dsp_out_5_3;
wire [15:0] dsp_out_5_4;
wire [15:0] dsp_out_5_5;
wire [15:0] dsp_out_5_6;
wire [15:0] dsp_out_5_7;
wire [15:0] dsp_out_6_0;
wire [15:0] dsp_out_6_1;
wire [15:0] dsp_out_6_2;
wire [15:0] dsp_out_6_3;
wire [15:0] dsp_out_6_4;
wire [15:0] dsp_out_6_5;
wire [15:0] dsp_out_6_6;
wire [15:0] dsp_out_6_7;
wire [15:0] dsp_out_6_8;
wire [15:0] dsp_out_6_9;
wire [15:0] dsp_out_6_10;
wire [15:0] dsp_out_6_11;
reg [15:0] feature_reg_0_0_0;
reg [15:0] feature_reg_0_0_1;
reg [15:0] feature_reg_0_1_0;
reg [15:0] feature_reg_0_1_1;
reg [15:0] feature_reg_0_2_0;
reg [15:0] feature_reg_0_2_1;
reg [15:0] feature_reg_0_3_0;
reg [15:0] feature_reg_0_3_1;
reg [15:0] feature_reg_0_4_0;
reg [15:0] feature_reg_0_4_1;
reg [15:0] feature_reg_0_5_0;
reg [15:0] feature_reg_0_5_1;
reg [15:0] feature_reg_1_0_0;
reg [15:0] feature_reg_1_0_1;
reg [15:0] feature_reg_1_1_0;
reg [15:0] feature_reg_1_1_1;
reg [15:0] feature_reg_1_2_0;
reg [15:0] feature_reg_1_2_1;
reg [15:0] feature_reg_1_3_0;
reg [15:0] feature_reg_1_3_1;
reg [15:0] feature_reg_1_4_0;
reg [15:0] feature_reg_1_4_1;
reg [15:0] feature_reg_1_5_0;
reg [15:0] feature_reg_1_5_1;
reg [15:0] feature_reg_2_0_0;
reg [15:0] feature_reg_2_0_1;
reg [15:0] feature_reg_2_1_0;
reg [15:0] feature_reg_2_1_1;
reg [15:0] feature_reg_2_2_0;
reg [15:0] feature_reg_2_2_1;
reg [15:0] feature_reg_2_3_0;
reg [15:0] feature_reg_2_3_1;
reg [15:0] feature_reg_2_4_0;
reg [15:0] feature_reg_2_4_1;
reg [15:0] feature_reg_2_5_0;
reg [15:0] feature_reg_2_5_1;
reg [15:0] feature_reg_3_0_0;
reg [15:0] feature_reg_3_0_1;
reg [15:0] feature_reg_3_1_0;
reg [15:0] feature_reg_3_1_1;
reg [15:0] feature_reg_3_2_0;
reg [15:0] feature_reg_3_2_1;
reg [15:0] feature_reg_3_3_0;
reg [15:0] feature_reg_3_3_1;
reg [15:0] feature_reg_3_4_0;
reg [15:0] feature_reg_3_4_1;
reg [15:0] feature_reg_3_5_0;
reg [15:0] feature_reg_3_5_1;
reg [15:0] feature_reg_4_0_0;
reg [15:0] feature_reg_4_0_1;
reg [15:0] feature_reg_4_1_0;
reg [15:0] feature_reg_4_1_1;
reg [15:0] feature_reg_4_2_0;
reg [15:0] feature_reg_4_2_1;
reg [15:0] feature_reg_4_3_0;
reg [15:0] feature_reg_4_3_1;
reg [15:0] feature_reg_4_4_0;
reg [15:0] feature_reg_4_4_1;
reg [15:0] feature_reg_4_5_0;
reg [15:0] feature_reg_4_5_1;
reg [15:0] feature_reg_5_0_0;
reg [15:0] feature_reg_5_0_1;
reg [15:0] feature_reg_5_1_0;
reg [15:0] feature_reg_5_1_1;
reg [15:0] feature_reg_5_2_0;
reg [15:0] feature_reg_5_2_1;
reg [15:0] feature_reg_5_3_0;
reg [15:0] feature_reg_5_3_1;
reg [15:0] feature_reg_5_4_0;
reg [15:0] feature_reg_5_4_1;
reg [15:0] feature_reg_5_5_0;
reg [15:0] feature_reg_5_5_1;

always @ (posedge clk) begin
	calculate_1 <= calculate;
	calculate_2 <= calculate_1;
	calculate_3 <= calculate_2;
	//Valid pipeline
	valid_0 <= i_valid;
	valid_1 <= valid_0;
	valid_2 <= valid_1;
	valid_3 <= valid_2;
	valid_4 <= valid_3;
	valid_5 <= valid_4;
	valid_6 <= valid_5;
	valid_7 <= valid_6;
	valid_8 <= valid_7;
	valid_9 <= valid_8;
	valid_10 <= valid_9;
	valid_11 <= valid_10;
	valid_12 <= valid_11;
	if (i_valid) begin
		input_buffer_count <= 0;
		calculate <= 0;
	end else begin
		//Input buffering logic
		if (input_buffer_count == 5) begin
			calculate <= 1;
			input_buffer_count <= 0;
		end else begin
			calculate <= 0;
			input_buffer_count <= input_buffer_count + 1'b1;
		end
		input_buffer_5_0 <= i_result_0_1;
		input_buffer_5_1 <= i_result_1_1;
		input_buffer_5_2 <= i_result_2_1;
		input_buffer_5_3 <= i_result_3_1;
		input_buffer_5_4 <= i_result_4_1;
		input_buffer_5_5 <= i_result_5_1;
	end
	input_buffer_0_0 <= input_buffer_1_0;
	input_buffer_0_1 <= input_buffer_1_1;
	input_buffer_0_2 <= input_buffer_1_2;
	input_buffer_0_3 <= input_buffer_1_3;
	input_buffer_0_4 <= input_buffer_1_4;
	input_buffer_0_5 <= input_buffer_1_5;
	input_buffer_1_0 <= input_buffer_2_0;
	input_buffer_1_1 <= input_buffer_2_1;
	input_buffer_1_2 <= input_buffer_2_2;
	input_buffer_1_3 <= input_buffer_2_3;
	input_buffer_1_4 <= input_buffer_2_4;
	input_buffer_1_5 <= input_buffer_2_5;
	input_buffer_2_0 <= input_buffer_3_0;
	input_buffer_2_1 <= input_buffer_3_1;
	input_buffer_2_2 <= input_buffer_3_2;
	input_buffer_2_3 <= input_buffer_3_3;
	input_buffer_2_4 <= input_buffer_3_4;
	input_buffer_2_5 <= input_buffer_3_5;
	input_buffer_3_0 <= input_buffer_4_0;
	input_buffer_3_1 <= input_buffer_4_1;
	input_buffer_3_2 <= input_buffer_4_2;
	input_buffer_3_3 <= input_buffer_4_3;
	input_buffer_3_4 <= input_buffer_4_4;
	input_buffer_3_5 <= input_buffer_4_5;
	input_buffer_4_0 <= input_buffer_5_0;
	input_buffer_4_1 <= input_buffer_5_1;
	input_buffer_4_2 <= input_buffer_5_2;
	input_buffer_4_3 <= input_buffer_5_3;
	input_buffer_4_4 <= input_buffer_5_4;
	input_buffer_4_5 <= input_buffer_5_5;
	//Pipelining to synchronize DSPs and non-DSPs
	feature_reg_0_0_0 <= input_buffer_0_0;
	feature_reg_0_1_0 <= input_buffer_0_1;
	feature_reg_0_2_0 <= input_buffer_0_2;
	feature_reg_0_3_0 <= input_buffer_0_3;
	feature_reg_0_4_0 <= input_buffer_0_4;
	feature_reg_0_5_0 <= input_buffer_0_5;
	feature_reg_1_0_0 <= input_buffer_1_0;
	feature_reg_1_1_0 <= input_buffer_1_1;
	feature_reg_1_2_0 <= input_buffer_1_2;
	feature_reg_1_3_0 <= input_buffer_1_3;
	feature_reg_1_4_0 <= input_buffer_1_4;
	feature_reg_1_5_0 <= input_buffer_1_5;
	feature_reg_2_0_0 <= input_buffer_2_0;
	feature_reg_2_1_0 <= input_buffer_2_1;
	feature_reg_2_2_0 <= input_buffer_2_2;
	feature_reg_2_3_0 <= input_buffer_2_3;
	feature_reg_2_4_0 <= input_buffer_2_4;
	feature_reg_2_5_0 <= input_buffer_2_5;
	feature_reg_3_0_0 <= input_buffer_3_0;
	feature_reg_3_1_0 <= input_buffer_3_1;
	feature_reg_3_2_0 <= input_buffer_3_2;
	feature_reg_3_3_0 <= input_buffer_3_3;
	feature_reg_3_4_0 <= input_buffer_3_4;
	feature_reg_3_5_0 <= input_buffer_3_5;
	feature_reg_4_0_0 <= input_buffer_4_0;
	feature_reg_4_1_0 <= input_buffer_4_1;
	feature_reg_4_2_0 <= input_buffer_4_2;
	feature_reg_4_3_0 <= input_buffer_4_3;
	feature_reg_4_4_0 <= input_buffer_4_4;
	feature_reg_4_5_0 <= input_buffer_4_5;
	feature_reg_5_0_0 <= input_buffer_5_0;
	feature_reg_5_1_0 <= input_buffer_5_1;
	feature_reg_5_2_0 <= input_buffer_5_2;
	feature_reg_5_3_0 <= input_buffer_5_3;
	feature_reg_5_4_0 <= input_buffer_5_4;
	feature_reg_5_5_0 <= input_buffer_5_5;
	feature_reg_0_0_1 <= feature_reg_0_0_0;
	feature_reg_0_1_1 <= feature_reg_0_1_0;
	feature_reg_0_2_1 <= feature_reg_0_2_0;
	feature_reg_0_3_1 <= feature_reg_0_3_0;
	feature_reg_0_4_1 <= feature_reg_0_4_0;
	feature_reg_0_5_1 <= feature_reg_0_5_0;
	feature_reg_1_0_1 <= feature_reg_1_0_0;
	feature_reg_1_1_1 <= feature_reg_1_1_0;
	feature_reg_1_2_1 <= feature_reg_1_2_0;
	feature_reg_1_3_1 <= feature_reg_1_3_0;
	feature_reg_1_4_1 <= feature_reg_1_4_0;
	feature_reg_1_5_1 <= feature_reg_1_5_0;
	feature_reg_2_0_1 <= feature_reg_2_0_0;
	feature_reg_2_1_1 <= feature_reg_2_1_0;
	feature_reg_2_2_1 <= feature_reg_2_2_0;
	feature_reg_2_3_1 <= feature_reg_2_3_0;
	feature_reg_2_4_1 <= feature_reg_2_4_0;
	feature_reg_2_5_1 <= feature_reg_2_5_0;
	feature_reg_3_0_1 <= feature_reg_3_0_0;
	feature_reg_3_1_1 <= feature_reg_3_1_0;
	feature_reg_3_2_1 <= feature_reg_3_2_0;
	feature_reg_3_3_1 <= feature_reg_3_3_0;
	feature_reg_3_4_1 <= feature_reg_3_4_0;
	feature_reg_3_5_1 <= feature_reg_3_5_0;
	feature_reg_4_0_1 <= feature_reg_4_0_0;
	feature_reg_4_1_1 <= feature_reg_4_1_0;
	feature_reg_4_2_1 <= feature_reg_4_2_0;
	feature_reg_4_3_1 <= feature_reg_4_3_0;
	feature_reg_4_4_1 <= feature_reg_4_4_0;
	feature_reg_4_5_1 <= feature_reg_4_5_0;
	feature_reg_5_0_1 <= feature_reg_5_0_0;
	feature_reg_5_1_1 <= feature_reg_5_1_0;
	feature_reg_5_2_1 <= feature_reg_5_2_0;
	feature_reg_5_3_1 <= feature_reg_5_3_0;
	feature_reg_5_4_1 <= feature_reg_5_4_0;
	feature_reg_5_5_1 <= feature_reg_5_5_0;
	//Output Serializing logic
	if (calculate_3) begin
		output_buffer_0_0 <= rslt_buffer_0_0;
		output_buffer_1_0 <= rslt_buffer_0_1;
		output_buffer_2_0 <= rslt_buffer_0_2;
		output_buffer_3_0 <= rslt_buffer_0_3;
		output_buffer_4_0 <= rslt_buffer_0_4;
		output_buffer_5_0 <= rslt_buffer_0_5;
		output_buffer_0_1 <= rslt_buffer_1_0;
		output_buffer_1_1 <= rslt_buffer_1_1;
		output_buffer_2_1 <= rslt_buffer_1_2;
		output_buffer_3_1 <= rslt_buffer_1_3;
		output_buffer_4_1 <= rslt_buffer_1_4;
		output_buffer_5_1 <= rslt_buffer_1_5;
		output_buffer_0_2 <= rslt_buffer_2_0;
		output_buffer_1_2 <= rslt_buffer_2_1;
		output_buffer_2_2 <= rslt_buffer_2_2;
		output_buffer_3_2 <= rslt_buffer_2_3;
		output_buffer_4_2 <= rslt_buffer_2_4;
		output_buffer_5_2 <= rslt_buffer_2_5;
		output_buffer_0_3 <= rslt_buffer_3_0;
		output_buffer_1_3 <= rslt_buffer_3_1;
		output_buffer_2_3 <= rslt_buffer_3_2;
		output_buffer_3_3 <= rslt_buffer_3_3;
		output_buffer_4_3 <= rslt_buffer_3_4;
		output_buffer_5_3 <= rslt_buffer_3_5;
		output_buffer_0_4 <= rslt_buffer_4_0;
		output_buffer_1_4 <= rslt_buffer_4_1;
		output_buffer_2_4 <= rslt_buffer_4_2;
		output_buffer_3_4 <= rslt_buffer_4_3;
		output_buffer_4_4 <= rslt_buffer_4_4;
		output_buffer_5_4 <= rslt_buffer_4_5;
		output_buffer_0_5 <= rslt_buffer_5_0;
		output_buffer_1_5 <= rslt_buffer_5_1;
		output_buffer_2_5 <= rslt_buffer_5_2;
		output_buffer_3_5 <= rslt_buffer_5_3;
		output_buffer_4_5 <= rslt_buffer_5_4;
		output_buffer_5_5 <= rslt_buffer_5_5;
	end else begin
		output_buffer_0_0 <= output_buffer_0_1;
		output_buffer_0_1 <= output_buffer_0_2;
		output_buffer_0_2 <= output_buffer_0_3;
		output_buffer_0_3 <= output_buffer_0_4;
		output_buffer_0_4 <= output_buffer_0_5;
		output_buffer_1_0 <= output_buffer_1_1;
		output_buffer_1_1 <= output_buffer_1_2;
		output_buffer_1_2 <= output_buffer_1_3;
		output_buffer_1_3 <= output_buffer_1_4;
		output_buffer_1_4 <= output_buffer_1_5;
		output_buffer_2_0 <= output_buffer_2_1;
		output_buffer_2_1 <= output_buffer_2_2;
		output_buffer_2_2 <= output_buffer_2_3;
		output_buffer_2_3 <= output_buffer_2_4;
		output_buffer_2_4 <= output_buffer_2_5;
		output_buffer_3_0 <= output_buffer_3_1;
		output_buffer_3_1 <= output_buffer_3_2;
		output_buffer_3_2 <= output_buffer_3_3;
		output_buffer_3_3 <= output_buffer_3_4;
		output_buffer_3_4 <= output_buffer_3_5;
		output_buffer_4_0 <= output_buffer_4_1;
		output_buffer_4_1 <= output_buffer_4_2;
		output_buffer_4_2 <= output_buffer_4_3;
		output_buffer_4_3 <= output_buffer_4_4;
		output_buffer_4_4 <= output_buffer_4_5;
		output_buffer_5_0 <= output_buffer_5_1;
		output_buffer_5_1 <= output_buffer_5_2;
		output_buffer_5_2 <= output_buffer_5_3;
		output_buffer_5_3 <= output_buffer_5_4;
		output_buffer_5_4 <= output_buffer_5_5;
	end
end

////// FIRST COLUMN //////
winograd_dsp_16 winograd_dsp_16_WD00 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_0_2),
	.by(input_buffer_2_0),
	.coefsela(3'b101),
	.coefselb(3'b101),
	.resulta(dsp_out_1_0),
	.resultb(dsp_out_1_1)
);

winograd_dsp_16 winograd_dsp_16_WD10 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_2),
	.by(input_buffer_2_4),
	.coefsela(3'b110),
	.coefselb(3'b001),
	.resulta(dsp_out_1_2),
	.resultb(dsp_out_1_3)
);

winograd_dsp_16 winograd_dsp_16_WD20 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_4_2),
	.by(input_buffer_1_0),
	.coefsela(3'b001),
	.coefselb(3'b100),
	.resulta(dsp_out_1_4),
	.resultb(dsp_out_1_5)
);

winograd_dsp_16 winograd_dsp_16_WD30 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_2),
	.by(input_buffer_3_2),
	.coefsela(3'b100),
	.coefselb(3'b001),
	.resulta(dsp_out_1_6),
	.resultb(dsp_out_1_7)
);

winograd_dsp_16 winograd_dsp_16_WD40 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_0),
	.by(input_buffer_3_2),
	.coefsela(3'b101),
	.coefselb(3'b110),
	.resulta(dsp_out_1_8),
	.resultb(dsp_out_1_9)
);

winograd_dsp_16 winograd_dsp_16_WD50 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_4),
	.by(input_buffer_5_2),
	.coefsela(3'b001),
	.coefselb(3'b001),
	.resulta(dsp_out_1_10),
	.resultb(dsp_out_1_11)
);

////// SECOND COLUMN //////
winograd_dsp_16 winograd_dsp_16_WD01 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_1),
	.by(input_buffer_2_2),
	.coefsela(3'b100),
	.coefselb(3'b100),
	.resulta(dsp_out_2_0),
	.resultb(dsp_out_2_1)
);

winograd_dsp_16 winograd_dsp_16_WD11 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_3),
	.by(input_buffer_2_4),
	.coefsela(3'b001),
	.coefselb(3'b001),
	.resulta(dsp_out_2_2),
	.resultb(dsp_out_2_3)
);

winograd_dsp_16 winograd_dsp_16_WD21 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_1),
	.by(input_buffer_3_2),
	.coefsela(3'b100),
	.coefselb(3'b100),
	.resulta(dsp_out_2_4),
	.resultb(dsp_out_2_5)
);

winograd_dsp_16 winograd_dsp_16_WD31 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_3),
	.by(input_buffer_3_4),
	.coefsela(3'b001),
	.coefselb(3'b001),
	.resulta(dsp_out_2_6),
	.resultb(dsp_out_2_7)
);

////// THIRD COLUMN //////
winograd_dsp_16 winograd_dsp_16_WD02 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_1),
	.by(input_buffer_2_2),
	.coefsela(3'b101),
	.coefselb(3'b100),
	.resulta(dsp_out_3_0),
	.resultb(dsp_out_3_1)
);

winograd_dsp_16 winograd_dsp_16_WD12 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_3),
	.by(input_buffer_2_4),
	.coefsela(3'b000),
	.coefselb(3'b001),
	.resulta(dsp_out_3_2),
	.resultb(dsp_out_3_3)
);

winograd_dsp_16 winograd_dsp_16_WD22 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_1),
	.by(input_buffer_3_2),
	.coefsela(3'b101),
	.coefselb(3'b100),
	.resulta(dsp_out_3_4),
	.resultb(dsp_out_3_5)
);

winograd_dsp_16 winograd_dsp_16_WD32 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_3),
	.by(input_buffer_3_4),
	.coefsela(3'b000),
	.coefselb(3'b001),
	.resulta(dsp_out_3_6),
	.resultb(dsp_out_3_7)
);

////// FOURTH COLUMN //////
winograd_dsp_16 winograd_dsp_16_WD03 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_1),
	.by(input_buffer_2_2),
	.coefsela(3'b010),
	.coefselb(3'b000),
	.resulta(dsp_out_4_0),
	.resultb(dsp_out_4_1)
);

winograd_dsp_16 winograd_dsp_16_WD13 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_3),
	.by(input_buffer_2_4),
	.coefsela(3'b011),
	.coefselb(3'b001),
	.resulta(dsp_out_4_2),
	.resultb(dsp_out_4_3)
);

winograd_dsp_16 winograd_dsp_16_WD23 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_1),
	.by(input_buffer_3_2),
	.coefsela(3'b010),
	.coefselb(3'b000),
	.resulta(dsp_out_4_4),
	.resultb(dsp_out_4_5)
);

winograd_dsp_16 winograd_dsp_16_WD33 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_3),
	.by(input_buffer_3_4),
	.coefsela(3'b011),
	.coefselb(3'b001),
	.resulta(dsp_out_4_6),
	.resultb(dsp_out_4_7)
);

////// FIFTH COLUMN //////
winograd_dsp_16 winograd_dsp_16_WD04 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_1),
	.by(input_buffer_2_2),
	.coefsela(3'b011),
	.coefselb(3'b000),
	.resulta(dsp_out_5_0),
	.resultb(dsp_out_5_1)
);

winograd_dsp_16 winograd_dsp_16_WD14 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_3),
	.by(input_buffer_2_4),
	.coefsela(3'b010),
	.coefselb(3'b001),
	.resulta(dsp_out_5_2),
	.resultb(dsp_out_5_3)
);

winograd_dsp_16 winograd_dsp_16_WD24 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_1),
	.by(input_buffer_3_2),
	.coefsela(3'b011),
	.coefselb(3'b000),
	.resulta(dsp_out_5_4),
	.resultb(dsp_out_5_5)
);

winograd_dsp_16 winograd_dsp_16_WD34 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_3),
	.by(input_buffer_3_4),
	.coefsela(3'b010),
	.coefselb(3'b001),
	.resulta(dsp_out_5_6),
	.resultb(dsp_out_5_7)
);

////// SIXTH COLUMN //////
winograd_dsp_16 winograd_dsp_16_WD05 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_0_3),
	.by(input_buffer_2_1),
	.coefsela(3'b101),
	.coefselb(3'b101),
	.resulta(dsp_out_6_0),
	.resultb(dsp_out_6_1)
);

winograd_dsp_16 winograd_dsp_16_WD15 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_3),
	.by(input_buffer_2_5),
	.coefsela(3'b110),
	.coefselb(3'b001),
	.resulta(dsp_out_6_2),
	.resultb(dsp_out_6_3)
);

winograd_dsp_16 winograd_dsp_16_WD25 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_4_3),
	.by(input_buffer_1_3),
	.coefsela(3'b001),
	.coefselb(3'b100),
	.resulta(dsp_out_6_4),
	.resultb(dsp_out_6_5)
);

winograd_dsp_16 winograd_dsp_16_WD35 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_3),
	.by(input_buffer_3_3),
	.coefsela(3'b100),
	.coefselb(3'b001),
	.resulta(dsp_out_6_6),
	.resultb(dsp_out_6_7)
);

winograd_dsp_16 winograd_dsp_16_WD45 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_1),
	.by(input_buffer_3_3),
	.coefsela(3'b101),
	.coefselb(3'b110),
	.resulta(dsp_out_6_8),
	.resultb(dsp_out_6_9)
);

winograd_dsp_16 winograd_dsp_16_WD55 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_5),
	.by(input_buffer_5_3),
	.coefsela(3'b001),
	.coefselb(3'b001),
	.resulta(dsp_out_6_10),
	.resultb(dsp_out_6_11)
);

winograd_adder_16_20_4 winograd_adder_16_20_4_WA00 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_0_0_1[11:0], 4'b0000}),
	.data1x(dsp_out_1_0),
	.data2x({feature_reg_0_4_1[13:0], 2'b00}),
	.data3x(dsp_out_1_1),
	.data4x(dsp_out_1_2),
	.data5x(dsp_out_1_3),
	.data6x({feature_reg_4_0_1[13:0], 2'b00}),
	.data7x(dsp_out_1_4),
	.data8x(feature_reg_4_4_1),
	.data9x(0),
	.data10x(0),
	.data11x(0),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_0_0)
);

wire [15:0] f1, f2, f3, f4;
assign f1 = -{feature_reg_1_0_1[11:0], 4'b0000};
assign f2 = -{feature_reg_1_4_1[13:0], 2'b00};
assign f3 = -{feature_reg_2_0_1[11:0], 4'b0000};
assign f4 = -{feature_reg_2_4_1[13:0], 2'b00};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA10 (
	.clock(clk),
	.clken(calculate_2),
	.data0x(dsp_out_1_5),
	.data1x(f1),
	.data2x(f2),
	.data3x(f3),
	.data4x(dsp_out_1_6),
	.data5x(f4),
	.data6x({feature_reg_3_0_1[13:0], 2'b00}),
	.data7x(dsp_out_1_7),
	.data8x(feature_reg_3_4_1),
	.data9x({feature_reg_4_0_1[13:0], 2'b00}),
	.data10x(dsp_out_1_4),
	.data11x(feature_reg_4_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_1_0)
);

wire [15:0] f5, f6, f7, f8, f9, f10;
assign f5 = -dsp_out_1_5;
assign f6 = -{feature_reg_2_0_1[11:0], 4'b0000};
assign f7 = -{feature_reg_2_4_1[13:0], 2'b00};
assign f8 = -{feature_reg_3_0_1[13:0], 2'b00};
assign f9 = -dsp_out_1_7;
assign f10 = -feature_reg_3_4_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA20 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_0_1[11:0], 4'b0000}),
	.data1x(f5),
	.data2x({feature_reg_1_4_1[13:0], 2'b00}),
	.data3x(f6),
	.data4x(dsp_out_1_6),
	.data5x(f7),
	.data6x(f8),
	.data7x(f9),
	.data8x(f10),
	.data9x({feature_reg_4_0_1[13:0], 2'b00}),
	.data10x(dsp_out_1_4),
	.data11x(feature_reg_4_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_2_0)
);

wire [15:0] f11, f12, f13, f14, f15, f16, f17;
assign f11 = -{feature_reg_1_0_1[12:0], 3'b000};
assign f12 = -{feature_reg_1_4_1[14:0], 1'b0};
assign f13 = -{feature_reg_2_0_1[13:0], 2'b00};
assign f14 = -feature_reg_2_4_1;
assign f15 = dsp_out_1_5 >>> 1;
assign f16 = dsp_out_1_6 >>> 2;
assign f17 = dsp_out_1_7 <<< 1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA30 (
	.clock(clk),
	.clken(calculate_2),
	.data0x(f15),
	.data1x(f11),
	.data2x(f12),
	.data3x(f13),
	.data4x(f16),
	.data5x(f14),
	.data6x({feature_reg_3_0_1[12:0], 3'b000}),
	.data7x(f17),
	.data8x({feature_reg_3_4_1[14:0], 1'b0}),
	.data9x({feature_reg_4_0_1[13:0], 2'b00}),
	.data10x(dsp_out_1_4),
	.data11x(feature_reg_4_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_3_0)
);

wire [15:0] f18, f19, f20, f21, f22, f23, f23b;
assign f18 = -(dsp_out_1_5 >>> 1);
assign f19 = -{feature_reg_2_0_1[13:0], 2'b00};
assign f20 = -feature_reg_2_4_1;
assign f21 = -{feature_reg_3_0_1[12:0], 3'b000};
assign f22 = -(dsp_out_1_7 <<< 1);
assign f23 = -{feature_reg_3_4_1[14:0], 1'b0};
assign f23b = dsp_out_1_6 >>> 2;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA40 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_0_1[12:0], 3'b000}),
	.data1x(f18),
	.data2x({feature_reg_1_4_1[14:0], 1'b0}),
	.data3x(f19),
	.data4x(f23b),
	.data5x(f20),
	.data6x(f21),
	.data7x(f22),
	.data8x(f23),
	.data9x({feature_reg_4_0_1[13:0], 2'b00}),
	.data10x(dsp_out_1_4),
	.data11x(feature_reg_4_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_4_0)
);

wire [15:0] f24;
assign f24 = -dsp_out_1_5;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA50 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_0_1[11:0], 4'b0000}),
	.data1x(f24),
	.data2x({feature_reg_1_4_1[13:0], 2'b00}),
	.data3x(dsp_out_1_8),
	.data4x(dsp_out_1_9),
	.data5x(dsp_out_1_10),
	.data6x({feature_reg_5_0_1[13:0], 2'b00}),
	.data7x(dsp_out_1_11),
	.data8x(feature_reg_5_4_1),
	.data9x(0),
	.data10x(0),
	.data11x(0),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_5_0)
);

wire [15:0] f25, f26, f27, f28;
assign f25 = -{feature_reg_0_2_1[11:0], 4'b0000};
assign f26 = -{feature_reg_0_1_1[11:0], 4'b0000};
assign f27 = -{feature_reg_4_1_1[13:0], 2'b00};
assign f28 = -{feature_reg_4_2_1[13:0], 2'b00};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA01 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_0_3_1[13:0], 2'b00}),
	.data1x(f25),
	.data2x(f26),
	.data3x({feature_reg_0_4_1[13:0], 2'b00}),
	.data4x(dsp_out_2_0),
	.data5x(dsp_out_2_1),
	.data6x(dsp_out_2_2),
	.data7x(dsp_out_2_3),
	.data8x(f27),
	.data9x(f28),
	.data10x(feature_reg_4_3_1),
	.data11x(feature_reg_4_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_0_1)
);

wire [15:0] f29, f30, f31, f32, f33, f34, f35, f36;
assign f29 = -{feature_reg_1_3_1[13:0], 2'b00};
assign f30 = -{feature_reg_1_4_1[13:0], 2'b00};
assign f31 = -{feature_reg_2_3_1[13:0], 2'b00};
assign f32 = -{feature_reg_2_4_1[13:0], 2'b00};
assign f33 = -{feature_reg_3_1_1[13:0], 2'b00};
assign f34 = -{feature_reg_3_2_1[13:0], 2'b00};
assign f35 = -{feature_reg_4_1_1[13:0], 2'b00};
assign f36 = -{feature_reg_4_2_1[13:0], 2'b00};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA11 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[11:0], 4'b0000}),
	.data1x({feature_reg_1_2_1[11:0], 4'b0000}),
	.data2x(f29),
	.data3x(f30),
	.data4x({feature_reg_2_1_1[11:0], 4'b0000}),
	.data5x({feature_reg_2_2_1[11:0], 4'b0000}),
	.data6x(f31),
	.data7x(f32),
	.data8x(f33),
	.data9x(f34),
	.data10x(feature_reg_3_3_1),
	.data11x(feature_reg_3_4_1),
	.data12x(f35),
	.data13x(f36),
	.data14x(feature_reg_4_3_1),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_1_1)
);

wire [15:0] f37, f38, f39, f40, f41, f42, f43, f44;
assign f37 = -{feature_reg_1_2_1[11:0],4'b0000};
assign f38 = -{feature_reg_1_1_1[11:0],4'b0000};
assign f39 = -{feature_reg_2_3_1[13:0],2'b00};
assign f40 = -{feature_reg_2_4_1[13:0],2'b00};
assign f41 = -feature_reg_3_3_1;
assign f42 = -feature_reg_3_4_1;
assign f43 = -{feature_reg_4_1_1[13:0],2'b00};
assign f44 = -{feature_reg_4_2_1[13:0],2'b00};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA21 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_3_1[13:0],2'b00}),
	.data1x(f37),
	.data2x(f38),
	.data3x({feature_reg_1_4_1[13:0],2'b00}),
	.data4x({feature_reg_2_1_1[11:0],4'b0000}),
	.data5x({feature_reg_2_2_1[11:0],4'b0000}),
	.data6x(f39),
	.data7x(f40),
	.data8x({feature_reg_3_1_1[13:0],2'b00}),
	.data9x({feature_reg_3_2_1[13:0],2'b00}),
	.data10x(f41),
	.data11x(f42),
	.data12x(f43),
	.data13x(f44),
	.data14x(feature_reg_4_3_1),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_2_1)
);

wire [15:0] f45, f46, f47, f48, f49, f50, f51, f52;
assign f45 = -{feature_reg_1_3_1[14:0],1'b0};
assign f46 = -{feature_reg_1_4_1[14:0],1'b0};
assign f47 = -feature_reg_2_3_1;
assign f48 = -feature_reg_2_4_1;
assign f49 = -{feature_reg_3_1_1[12:0],3'b000};
assign f50 = -{feature_reg_3_2_1[12:0],3'b000};
assign f51 = -{feature_reg_4_1_1[13:0],2'b00};
assign f52 = -{feature_reg_4_2_1[13:0],2'b00};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA31 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[12:0],3'b000}),
	.data1x({feature_reg_1_2_1[12:0],3'b000}),
	.data2x(f45),
	.data3x(f46),
	.data4x({feature_reg_2_1_1[13:0],2'b00}),
	.data5x({feature_reg_2_2_1[13:0],2'b00}),
	.data6x(f47),
	.data7x(f48),
	.data8x(f49),
	.data9x(f50),
	.data10x({feature_reg_3_3_1[14:0],1'b0}),
	.data11x({feature_reg_3_4_1[14:0],1'b0}),
	.data12x(f51),
	.data13x(f52),
	.data14x(feature_reg_4_3_1),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_3_1)
);

wire [15:0] f53, f54, f55, f56, f57, f58, f59, f60;
assign f53 = -{feature_reg_1_2_1[12:0],3'b000};
assign f54 = -{feature_reg_1_1_1[12:0],3'b000};
assign f55 = -feature_reg_2_3_1;
assign f56 = -feature_reg_2_4_1;
assign f57 = -{feature_reg_3_3_1[14:0],1'b0};
assign f58 = -{feature_reg_3_4_1[14:0],1'b0};
assign f59 = -{feature_reg_4_1_1[13:0],2'b00};
assign f60 = -{feature_reg_4_2_1[13:0],2'b00};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA41 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_3_1[14:0],1'b0}),
	.data1x(f53),
	.data2x(f54),
	.data3x({feature_reg_1_4_1[14:0],1'b0}),
	.data4x({feature_reg_2_1_1[13:0],2'b00}),
	.data5x({feature_reg_2_2_1[13:0],2'b00}),
	.data6x(f55),
	.data7x(f56),
	.data8x({feature_reg_3_1_1[12:0],3'b000}),
	.data9x({feature_reg_3_2_1[12:0],3'b000}),
	.data10x(f57),
	.data11x(f58),
	.data12x(f59),
	.data13x(f60),
	.data14x(feature_reg_4_3_1),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_4_1)
);

wire [15:0] f61, f62, f63, f64;
assign f61 = -{feature_reg_1_2_1[11:0],4'b0000};
assign f62 = -{feature_reg_1_1_1[11:0],4'b0000};
assign f63 = -{feature_reg_5_1_1[13:0],2'b00};
assign f64 = -{feature_reg_5_2_1[13:0],2'b00};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA51 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_3_1[13:0],2'b00}),
	.data1x(f61),
	.data2x(f62),
	.data3x({feature_reg_1_4_1[13:0],2'b00}),
	.data4x(dsp_out_2_4),
	.data5x(dsp_out_2_5),
	.data6x(dsp_out_2_6),
	.data7x(dsp_out_2_7),
	.data8x(f63),
	.data9x(f64),
	.data10x(feature_reg_5_3_1),
	.data11x(feature_reg_5_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_5_1)
);

wire [15:0] f65, f66, f67, f68;
assign f65 = -{feature_reg_0_2_1[11:0],4'b0000};
assign f66 = -{feature_reg_0_3_1[13:0],2'b00};
assign f67 = -{feature_reg_4_2_1[13:0],2'b00};
assign f68 = -feature_reg_4_3_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA02 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_0_1_1[11:0],4'b0000}),
	.data1x(f65),
	.data2x(f66),
	.data3x({feature_reg_0_4_1[13:0],2'b00}),
	.data4x(dsp_out_3_0),
	.data5x(dsp_out_3_1),
	.data6x(dsp_out_3_2),
	.data7x(dsp_out_3_3),
	.data8x({feature_reg_4_1_1[13:0],2'b00}),
	.data9x(f67),
	.data10x(f68),
	.data11x(feature_reg_4_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_0_2)
);

wire [15:0] f69, f70, f71, f72, f73, f74, f75, f76;
assign f69 = -{feature_reg_1_1_1[11:0],4'b0000};
assign f70 = -{feature_reg_1_4_1[13:0],2'b00};
assign f71 = -{feature_reg_2_1_1[11:0],4'b0000};
assign f72 = -{feature_reg_2_4_1[13:0],2'b00};
assign f73 = -{feature_reg_3_2_1[13:0],2'b00};
assign f74 = -feature_reg_3_3_1;
assign f75 = -{feature_reg_4_2_1[13:0],2'b00};
assign f76 = -feature_reg_4_3_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA12 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_2_1[11:0],4'b0000}),
	.data1x(f69),
	.data2x({feature_reg_1_3_1[13:0],2'b00}),
	.data3x(f70),
	.data4x(f71),
	.data5x({feature_reg_2_2_1[11:0],4'b0000}),
	.data6x({feature_reg_2_3_1[13:0],2'b00}),
	.data7x(f72),
	.data8x({feature_reg_3_1_1[13:0],2'b00}),
	.data9x(f73),
	.data10x(f74),
	.data11x(feature_reg_3_4_1),
	.data12x({feature_reg_4_1_1[13:0],2'b00}),
	.data13x(f75),
	.data14x(f76),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_1_2)
);

wire [15:0] f77, f78, f79, f80, f81, f82, f83, f84;
assign f77 = -{feature_reg_1_2_1[11:0],4'b0000};
assign f78 = -{feature_reg_1_3_1[13:0],2'b00};
assign f79 = -{feature_reg_2_1_1[11:0],4'b0000};
assign f80 = -{feature_reg_2_4_1[13:0],2'b00};
assign f81 = -{feature_reg_3_1_1[13:0],2'b00};
assign f82 = -feature_reg_3_4_1;
assign f83 = -{feature_reg_4_2_1[13:0],2'b00};
assign f84 = -feature_reg_4_3_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA22 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[11:0],4'b0000}),
	.data1x(f77),
	.data2x(f78),
	.data3x({feature_reg_1_4_1[13:0],2'b00}),
	.data4x(f79),
	.data5x({feature_reg_2_2_1[11:0],4'b0000}),
	.data6x({feature_reg_2_3_1[13:0],2'b00}),
	.data7x(f80),
	.data8x(f81),
	.data9x({feature_reg_3_2_1[13:0],2'b00}),
	.data10x(feature_reg_3_3_1),
	.data11x(f82),
	.data12x({feature_reg_4_1_1[13:0],2'b00}),
	.data13x(f83),
	.data14x(f84),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_2_2)
);

wire [15:0] f85, f86, f87, f88, f89, f90, f91, f92;
assign f85 = -{feature_reg_1_1_1[12:0],3'b000};
assign f86 = -{feature_reg_1_4_1[14:0],1'b0};
assign f87 = -{feature_reg_2_1_1[13:0],2'b00};
assign f88 = -feature_reg_2_4_1;
assign f89 = -{feature_reg_3_2_1[12:0],3'b000};
assign f90 = -{feature_reg_3_3_1[14:0],1'b0};
assign f91 = -{feature_reg_4_2_1[13:0],2'b00};
assign f92 = -feature_reg_4_3_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA32 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_2_1[12:0],3'b000}),
	.data1x(f85),
	.data2x({feature_reg_1_3_1[14:0],1'b0}),
	.data3x(f86),
	.data4x(f87),
	.data5x({feature_reg_2_2_1[13:0],2'b00}),
	.data6x(feature_reg_2_3_1),
	.data7x(f88),
	.data8x({feature_reg_3_1_1[12:0],3'b000}),
	.data9x(f89),
	.data10x(f90),
	.data11x({feature_reg_3_4_1[14:0],1'b0}),
	.data12x({feature_reg_4_1_1[13:0],2'b00}),
	.data13x(f91),
	.data14x(f92),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_3_2)
);

wire [15:0] f93, f94, f95, f96, f97, f98, f99, f100;
assign f93 = -{feature_reg_1_2_1[12:0],3'b000};
assign f94 = -{feature_reg_1_3_1[14:0],1'b0};
assign f95 = -{feature_reg_2_1_1[13:0],2'b00};
assign f96 = -feature_reg_2_4_1;
assign f97 = -{feature_reg_3_1_1[12:0],3'b000};
assign f98 = -{feature_reg_3_4_1[14:0],1'b0};
assign f99 = -{feature_reg_4_2_1[13:0],2'b00};
assign f100 = -feature_reg_4_3_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA42 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[12:0],3'b000}),
	.data1x(f93),
	.data2x(f94),
	.data3x({feature_reg_1_4_1[14:0],1'b0}),
	.data4x(f95),
	.data5x({feature_reg_2_2_1[13:0],2'b00}),
	.data6x(feature_reg_2_3_1),
	.data7x(f96),
	.data8x(f97),
	.data9x({feature_reg_3_2_1[12:0],3'b000}),
	.data10x({feature_reg_3_3_1[14:0],1'b0}),
	.data11x(f98),
	.data12x({feature_reg_4_1_1[13:0],2'b00}),
	.data13x(f99),
	.data14x(f100),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_4_2)
);

wire [15:0] f101, f102, f103, f104;
assign f101 = -{feature_reg_1_2_1[11:0],4'b0000};
assign f102 = -{feature_reg_1_3_1[13:0],2'b00};
assign f103 = -{feature_reg_5_2_1[13:0],2'b00};
assign f104 = -feature_reg_5_3_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA52 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[11:0],4'b0000}),
	.data1x(f101),
	.data2x(f102),
	.data3x({feature_reg_1_4_1[13:0],2'b00}),
	.data4x(dsp_out_3_4),
	.data5x(dsp_out_3_5),
	.data6x(dsp_out_3_6),
	.data7x(dsp_out_3_7),
	.data8x({feature_reg_5_1_1[13:0],2'b00}),
	.data9x(f103),
	.data10x(f104),
	.data11x(feature_reg_5_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_5_2)
);

wire [15:0] f105, f106, f107, f108;
assign f105 = -{feature_reg_0_2_1[13:0],2'b00};
assign f106 = -{feature_reg_0_1_1[12:0],3'b000};
assign f107 = -{feature_reg_4_1_1[14:0],1'b0};
assign f108 = -feature_reg_4_2_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA03 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_0_3_1[12:0],3'b000}),
	.data1x(f105),
	.data2x(f106),
	.data3x({feature_reg_0_4_1[13:0],2'b00}),
	.data4x(dsp_out_4_0),
	.data5x(dsp_out_4_1),
	.data6x(dsp_out_4_2),
	.data7x(dsp_out_4_3),
	.data8x(f107),
	.data9x(f108),
	.data10x({feature_reg_4_3_1[14:0],1'b0}),
	.data11x(feature_reg_4_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_0_3)
);

wire [15:0] f109, f110, f111, f112, f113, f114, f115, f116;
assign f109 = -{feature_reg_1_3_1[12:0],3'b000};
assign f110 = -{feature_reg_1_4_1[13:0],2'b00};
assign f111 = -{feature_reg_2_3_1[12:0],3'b000};
assign f112 = -{feature_reg_2_4_1[13:0],2'b00};
assign f113 = -{feature_reg_3_1_1[14:0],1'b0};
assign f114 = -feature_reg_3_2_1;
assign f115 = -{feature_reg_4_1_1[14:0],1'b0};
assign f116 = -feature_reg_4_2_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA13 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[12:0],3'b000}),
	.data1x({feature_reg_1_2_1[13:0],2'b00}),
	.data2x(f109),
	.data3x(f110),
	.data4x({feature_reg_2_1_1[12:0],3'b000}),
	.data5x({feature_reg_2_2_1[13:0],2'b00}),
	.data6x(f111),
	.data7x(f112),
	.data8x(f113),
	.data9x(f114),
	.data10x({feature_reg_3_3_1[14:0],1'b0}),
	.data11x(feature_reg_3_4_1),
	.data12x(f115),
	.data13x(f116),
	.data14x({feature_reg_4_3_1[14:0],1'b0}),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_1_3)
);

wire [15:0] f117, f118, f119, f120, f121, f122, f123, f124;
assign f117 = -{feature_reg_1_2_1[13:0],2'b00};
assign f118 = -{feature_reg_1_1_1[12:0],3'b000};
assign f119 = -{feature_reg_2_3_1[12:0],3'b000};
assign f120 = -{feature_reg_2_4_1[13:0],2'b00};
assign f121 = -{feature_reg_3_3_1[14:0],1'b0};
assign f122 = -feature_reg_3_4_1;
assign f123 = -{feature_reg_4_1_1[14:0],1'b0};
assign f124 = -feature_reg_4_2_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA23 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_3_1[12:0],3'b000}), 
	.data1x(f117),
	.data2x(f118),
	.data3x({feature_reg_1_4_1[13:0],2'b00}),
	.data4x({feature_reg_2_1_1[12:0],3'b000}),
	.data5x({feature_reg_2_2_1[13:0],2'b00}),
	.data6x(f119),
	.data7x(f120),
	.data8x({feature_reg_3_1_1[14:0],1'b0}),
	.data9x(feature_reg_3_2_1),
	.data10x(f121),
	.data11x(f122),
	.data12x(f123),
	.data13x(f124),
	.data14x({feature_reg_4_3_1[14:0],1'b0}),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_2_3)
);

wire [15:0] f125, f126, f127, f128, f129, f130, f131, f132;
assign f125 = -{feature_reg_1_3_1[13:0],2'b00};
assign f126 = -{feature_reg_1_4_1[14:0],1'b0};
assign f127 = -{feature_reg_2_3_1[14:0],1'b0};
assign f128 = -feature_reg_2_4_1;
assign f129 = -{feature_reg_3_1_1[13:0],2'b00};
assign f130 = -{feature_reg_3_2_1[14:0],1'b0};
assign f131 = -{feature_reg_4_1_1[14:0],1'b0};
assign f132 = -feature_reg_4_2_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA33 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[13:0],2'b00}),
	.data1x({feature_reg_1_2_1[14:0],1'b0}),
	.data2x(f125),
	.data3x(f126),
	.data4x({feature_reg_2_1_1[14:0],1'b0}),
	.data5x(feature_reg_2_2_1),
	.data6x(f127),
	.data7x(f128),
	.data8x(f129),
	.data9x(f130),
	.data10x({feature_reg_3_3_1[13:0],2'b00}),
	.data11x({feature_reg_3_4_1[14:0],1'b0}),
	.data12x(f131),
	.data13x(f132),
	.data14x({feature_reg_4_3_1[14:0],1'b0}),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_3_3)
);

wire [15:0] f133, f134, f135, f136, f137, f138, f139, f140;
assign f133 = -{feature_reg_1_2_1[14:0],1'b0};
assign f134 = -{feature_reg_1_1_1[13:0],2'b00};
assign f135 = -{feature_reg_2_3_1[14:0],1'b0};
assign f136 = -feature_reg_2_4_1;
assign f137 = -{feature_reg_3_3_1[13:0],2'b00};
assign f138 = -{feature_reg_3_4_1[14:0],1'b0};
assign f139 = -{feature_reg_4_1_1[14:0],1'b0};
assign f140 = -feature_reg_4_2_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA43 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_3_1[13:0],2'b00}),
	.data1x(f133),
	.data2x(f134),
	.data3x({feature_reg_1_4_1[14:0],1'b0}),
	.data4x({feature_reg_2_1_1[14:0],1'b0}),
	.data5x(feature_reg_2_2_1),
	.data6x(f135),
	.data7x(f136),
	.data8x({feature_reg_3_1_1[13:0],2'b00}),
	.data9x({feature_reg_3_2_1[14:0],1'b0}),
	.data10x(f137),
	.data11x(f138),
	.data12x(f139),
	.data13x(f140),
	.data14x({feature_reg_4_3_1[14:0],1'b0}),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_4_3)
);

wire [15:0] f141, f142, f143, f144;
assign f141 = -{feature_reg_1_2_1[13:0],2'b00};
assign f142 = -{feature_reg_1_1_1[12:0],3'b000};
assign f143 = -{feature_reg_5_1_1[14:0],1'b0};
assign f144 = -feature_reg_5_2_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA53 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_3_1[12:0],3'b000}),
	.data1x(f141),
	.data2x(f142),
	.data3x({feature_reg_1_4_1[13:0],2'b00}),
	.data4x(dsp_out_4_4),
	.data5x(dsp_out_4_5),
	.data6x(dsp_out_4_6),
	.data7x(dsp_out_4_7),
	.data8x(f143),
	.data9x(f144),
	.data10x({feature_reg_5_3_1[14:0],1'b0}),
	.data11x(feature_reg_5_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_5_3)
);

wire [15:0] f145, f146, f147, f148;
assign f145 = -{feature_reg_0_2_1[13:0],2'b00};
assign f146 = -{feature_reg_0_3_1[12:0],3'b000};
assign f147 = -feature_reg_4_2_1;
assign f148 = -{feature_reg_4_3_1[14:0],1'b0};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA04 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_0_1_1[12:0],3'b000}),
	.data1x(f145),
	.data2x(f146),
	.data3x({feature_reg_0_4_1[13:0],2'b00}),
	.data4x(dsp_out_5_0),
	.data5x(dsp_out_5_1),
	.data6x(dsp_out_5_2),
	.data7x(dsp_out_5_3),
	.data8x({feature_reg_4_1_1[14:0],1'b0}),
	.data9x(f147),
	.data10x(f148),
	.data11x(feature_reg_4_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_0_4)
);

wire [15:0] f149, f150, f151, f152, f153, f154, f155, f156;
assign f149 = -{feature_reg_1_1_1[12:0],3'b000};
assign f150 = -{feature_reg_1_4_1[13:0],2'b00};
assign f151 = -{feature_reg_2_1_1[12:0],3'b000};
assign f152 = -{feature_reg_2_4_1[13:0],2'b00};
assign f153 = -feature_reg_3_2_1;
assign f154 = -{feature_reg_3_3_1[14:0],1'b0};
assign f155 = -feature_reg_4_2_1;
assign f156 = -{feature_reg_4_3_1[14:0],1'b0};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA14 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_2_1[13:0],2'b00}),
	.data1x(f149),
	.data2x({feature_reg_1_3_1[12:0],3'b000}),
	.data3x(f150),
	.data4x(f151),
	.data5x({feature_reg_2_2_1[13:0],2'b00}),
	.data6x({feature_reg_2_3_1[12:0],3'b000}),
	.data7x(f152),
	.data8x({feature_reg_3_1_1[14:0],1'b0}),
	.data9x(f153),
	.data10x(f154),
	.data11x(feature_reg_3_4_1),
	.data12x({feature_reg_4_1_1[14:0],1'b0}),
	.data13x(f155),
	.data14x(f156),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_1_4)
);

wire [15:0] f157, f158, f159, f160, f161, f162, f163, f164;
assign f157 = -{feature_reg_1_2_1[13:0],2'b00};
assign f158 = -{feature_reg_1_3_1[12:0],3'b000};
assign f159 = -{feature_reg_2_1_1[12:0],3'b000};
assign f160 = -{feature_reg_2_4_1[13:0],2'b00};
assign f161 = -{feature_reg_3_1_1[14:0],1'b0};
assign f162 = -feature_reg_3_4_1;
assign f163 = -feature_reg_4_2_1;
assign f164 = -{feature_reg_4_3_1[14:0],1'b0};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA24 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[12:0],3'b000}),
	.data1x(f157),
	.data2x(f158),
	.data3x({feature_reg_1_4_1[13:0],2'b00}),
	.data4x(f159),
	.data5x({feature_reg_2_2_1[13:0],2'b00}),
	.data6x({feature_reg_2_3_1[12:0],3'b000}),
	.data7x(f160),
	.data8x(f161),
	.data9x(feature_reg_3_2_1),
	.data10x({feature_reg_3_3_1[14:0],1'b0}),
	.data11x(f162),
	.data12x({feature_reg_4_1_1[14:0],1'b0}),
	.data13x(f163),
	.data14x(f164),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_2_4)
);

wire [15:0] f165, f166, f167, f168, f169, f170, f171, f172;
assign f165 = -{feature_reg_1_1_1[13:0],2'b00};
assign f166 = -{feature_reg_1_4_1[14:0],1'b0};
assign f167 = -{feature_reg_2_1_1[14:0],1'b0};
assign f168 = -feature_reg_2_4_1;
assign f169 = -{feature_reg_3_2_1[14:0],1'b0};
assign f170 = -{feature_reg_3_3_1[13:0],2'b00};
assign f171 = -feature_reg_4_2_1;
assign f172 = -{feature_reg_4_1_1[14:0],1'b0};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA34 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_2_1[14:0],1'b0}),
	.data1x(f165),
	.data2x({feature_reg_1_3_1[13:0],2'b00}),
	.data3x(f166),
	.data4x(f167),
	.data5x(feature_reg_2_2_1),
	.data6x({feature_reg_2_3_1[14:0],1'b0}),
	.data7x(f168),
	.data8x({feature_reg_3_1_1[13:0],2'b00}),
	.data9x(f169),
	.data10x(f170),
	.data11x({feature_reg_3_4_1[14:0],1'b0}),
	.data12x({feature_reg_4_1_1[14:0],1'b0}),
	.data13x(f171),
	.data14x(f172),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_3_4)
);

wire [15:0] f173, f174, f175, f176, f177, f178, f179, f180;
assign f173 = -{feature_reg_1_2_1[14:0],1'b0};
assign f174 = -{feature_reg_1_3_1[13:0],2'b00};
assign f175 = -{feature_reg_2_1_1[14:0],1'b0};
assign f176 = -feature_reg_2_4_1;
assign f177 = -{feature_reg_3_1_1[13:0],2'b00};
assign f178 = -{feature_reg_3_4_1[14:0],1'b0};
assign f179 = -feature_reg_4_2_1;
assign f180 = -{feature_reg_4_3_1[14:0],1'b0};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA44 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[13:0],2'b00}),
	.data1x(f173),
	.data2x(f174),
	.data3x({feature_reg_1_4_1[14:0],1'b0}),
	.data4x(f175),
	.data5x(feature_reg_2_2_1),
	.data6x({feature_reg_2_3_1[14:0],1'b0}),
	.data7x(f176),
	.data8x(f177),
	.data9x({feature_reg_3_2_1[14:0],1'b0}),
	.data10x({feature_reg_3_3_1[13:0],2'b00}),
	.data11x(f178),
	.data12x({feature_reg_4_1_1[14:0],1'b0}),
	.data13x(f179),
	.data14x(f180),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_4_4)
);

wire [15:0] f181, f182, f183, f184;
assign f181 = -{feature_reg_1_2_1[13:0],2'b00};
assign f182 = -{feature_reg_1_3_1[12:0],3'b000};
assign f183 = -feature_reg_5_2_1;
assign f184 = -{feature_reg_5_3_1[14:0],1'b0};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA54 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[12:0],3'b000}),
	.data1x(f181),
	.data2x(f182),
	.data3x({feature_reg_1_4_1[13:0],2'b00}),
	.data4x(dsp_out_5_4),
	.data5x(dsp_out_5_5),
	.data6x(dsp_out_5_6),
	.data7x(dsp_out_5_7),
	.data8x({feature_reg_5_1_1[14:0],1'b0}),
	.data9x(f183),
	.data10x(f184),
	.data11x(feature_reg_5_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_5_4)
);

winograd_adder_16_20_4 winograd_adder_16_20_4_WA05 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_0_1_1[11:0],4'b0000}),
	.data1x(dsp_out_6_0),
	.data2x({feature_reg_0_5_1[13:0],2'b00}),
	.data3x(dsp_out_6_1),
	.data4x(dsp_out_6_2),
	.data5x(dsp_out_6_3),
	.data6x({feature_reg_4_1_1[13:0],2'b00}),
	.data7x(dsp_out_6_4),
	.data8x(feature_reg_4_5_1),
	.data9x(0),
	.data10x(0),
	.data11x(0),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_0_5)
);

wire [15:0] f185, f186, f187, f188;
assign f185 = -{feature_reg_1_1_1[11:0],4'b0000};
assign f186 = -{feature_reg_1_5_1[13:0],2'b00};
assign f187 = -{feature_reg_2_1_1[11:0],4'b0000};
assign f188 = -{feature_reg_2_5_1[13:0],2'b00};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA15 (
	.clock(clk),
	.clken(calculate_2),
	.data0x(dsp_out_6_5),
	.data1x(f185),
	.data2x(f186),
	.data3x(f187),
	.data4x(dsp_out_6_6),
	.data5x(f188),
	.data6x({feature_reg_3_1_1[13:0],2'b00}),
	.data7x(dsp_out_6_7),
	.data8x(feature_reg_3_5_1),
	.data9x({feature_reg_4_1_1[13:0],2'b00}),
	.data10x(dsp_out_6_4),
	.data11x(feature_reg_4_5_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_1_5)
);

wire [15:0] f189, f190, f191, f192, f193, f194;
assign f189 = -dsp_out_6_5;
assign f190 = -{feature_reg_2_1_1[11:0],4'b0000};
assign f191 = -{feature_reg_2_5_1[13:0],2'b00};
assign f192 = -{feature_reg_3_1_1[13:0],2'b00};
assign f193 = -dsp_out_6_7;
assign f194 = -feature_reg_3_5_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA25 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[11:0],4'b0000}),
	.data1x(f189),
	.data2x({feature_reg_1_5_1[13:0],2'b00}),
	.data3x(f190),
	.data4x(dsp_out_6_6),
	.data5x(f191),
	.data6x(f192),
	.data7x(f193),
	.data8x(f194),
	.data9x({feature_reg_4_1_1[13:0],2'b00}),
	.data10x(dsp_out_6_4),
	.data11x(feature_reg_4_5_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_2_5)
);

wire [15:0] f195, f196, f197, f198, f199, f200, f201;
assign f195 = dsp_out_6_5 >>> 1;
assign f196 = -{feature_reg_1_1_1[12:0],3'b000};
assign f197 = -{feature_reg_1_5_1[14:0],1'b0};
assign f198 = -{feature_reg_2_1_1[13:0],2'b00};
assign f199 = dsp_out_6_6 >>> 2;
assign f200 = -feature_reg_2_5_1;
assign f201 = dsp_out_6_7 <<< 1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA35 (
	.clock(clk),
	.clken(calculate_2),
	.data0x(f195),
	.data1x(f196),
	.data2x(f197),
	.data3x(f198),
	.data4x(f199),
	.data5x(f200),
	.data6x({feature_reg_3_1_1[12:0],3'b000}),
	.data7x(f201),
	.data8x({feature_reg_3_5_1[14:0],1'b0}),
	.data9x({feature_reg_4_1_1[13:0],2'b00}),
	.data10x(dsp_out_6_4),
	.data11x(feature_reg_4_5_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_3_5)
);

wire [15:0] f202, f203, f204, f205, f206, f207, f208;
assign f202 = -(dsp_out_6_5 >>> 1);
assign f203 = -{feature_reg_2_1_1[13:0],2'b00};
assign f204 = dsp_out_6_6 >>> 2;
assign f205 = -feature_reg_2_5_1;
assign f206 = -{feature_reg_3_1_1[12:0],3'b000};
assign f207 = -(dsp_out_6_7 <<< 1);
assign f208 = -{feature_reg_3_5_1[14:0],1'b0};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA45 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[12:0],3'b000}), 
	.data1x(f202),
	.data2x({feature_reg_1_5_1[14:0],1'b0}),
	.data3x(f203),
	.data4x(f204),
	.data5x(f205),
	.data6x(f206),
	.data7x(f207),
	.data8x(f208),
	.data9x({feature_reg_4_1_1[13:0],2'b00}),
	.data10x(dsp_out_6_4),
	.data11x(feature_reg_4_5_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_4_5)
);

wire [15:0] f209;
assign f209 = -dsp_out_6_5;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA55 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[11:0],4'b0000}),
	.data1x(f209),
	.data2x({feature_reg_1_5_1[13:0],2'b00}),
	.data3x(dsp_out_6_8),
	.data4x(dsp_out_6_9),
	.data5x(dsp_out_6_10),
	.data6x({feature_reg_5_1_1[13:0],2'b00}),
	.data7x(dsp_out_6_11),
	.data8x(feature_reg_5_5_1),
	.data9x(0),
	.data10x(0),
	.data11x(0),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_5_5)
);

assign o_feature_0 = output_buffer_0_0[15:0];
assign o_feature_1 = output_buffer_1_0[15:0];
assign o_feature_2 = output_buffer_2_0[15:0];
assign o_feature_3 = output_buffer_3_0[15:0];
assign o_feature_4 = output_buffer_4_0[15:0];
assign o_feature_5 = output_buffer_5_0[15:0];
assign o_valid = valid_9;

endmodule

module winograd_adder_16_20_4 (
	input clken,
	input clock,
	input [15:0] data0x,
	input [15:0] data1x,
	input [15:0] data2x,
	input [15:0] data3x,
	input [15:0] data4x,
	input [15:0] data5x,
	input [15:0] data6x,
	input [15:0] data7x,
	input [15:0] data8x,
	input [15:0] data9x,
	input [15:0] data10x,
	input [15:0] data11x,
	input [15:0] data12x,
	input [15:0] data13x,
	input [15:0] data14x,
	input [15:0] data15x,
	output [19:0] result
);

reg [19:0] pipeline_0_0;
reg [19:0] pipeline_0_1;
reg [19:0] pipeline_0_2;
reg [19:0] pipeline_0_3;
reg [19:0] pipeline_0_4;
reg [19:0] pipeline_0_5;
reg [19:0] pipeline_0_6;
reg [19:0] pipeline_0_7;
reg [19:0] pipeline_1_0;
reg [19:0] pipeline_1_1;
reg [19:0] pipeline_1_2;
reg [19:0] pipeline_1_3;
reg [19:0] pipeline_2_0;
reg [19:0] pipeline_2_1;
reg [19:0] pipeline_3_0;

always @ (posedge clock) begin
	pipeline_0_0 <= data0x + data1x;
	pipeline_0_1 <= data2x + data3x;
	pipeline_0_2 <= data4x + data5x;
	pipeline_0_3 <= data6x + data7x;
	pipeline_0_4 <= data8x + data9x;
	pipeline_0_5 <= data10x + data11x;
	pipeline_0_6 <= data12x + data13x;
	pipeline_0_7 <= data14x + data15x;
	pipeline_1_0 <= pipeline_0_0 + pipeline_0_1;
	pipeline_1_1 <= pipeline_0_2 + pipeline_0_3;
	pipeline_1_2 <= pipeline_0_4 + pipeline_0_5;
	pipeline_1_3 <= pipeline_0_6 + pipeline_0_7;
	pipeline_2_0 <= pipeline_1_0 + pipeline_1_1;
	pipeline_2_1 <= pipeline_1_2 + pipeline_1_3;
	pipeline_3_0 <= pipeline_2_0 + pipeline_2_1;
end

assign result = pipeline_3_0;

endmodule

module winograd_dsp_16 (
	input clk,
	input ena,
	input aclr,
	input [15:0] ay,
	input [15:0] by,
	input [2:0] coefsela,
	input [2:0] coefselb,
	output [15:0] resulta,
	output [15:0] resultb
);

reg [15:0] coefa, coefb, ay_reg, by_reg, resa_reg, resb_reg;
assign resulta = resa_reg;
assign resultb = resb_reg;

always @ (posedge clk) begin
	if (aclr) begin
		coefa <= 0;
		coefb <= 0;
		ay_reg <= 0;
		by_reg <= 0;
		resa_reg <= 0;
		resb_reg <= 0;
	end else begin
		ay_reg <= ay;
		by_reg <= by;
		if (coefsela == 0) begin
			coefa <= 5;
		end else if (coefsela == 1) begin
			coefa <= -5;
		end else if (coefsela == 2) begin
			coefa <= 10;
		end else if (coefsela == 3) begin
			coefa <= -10;
		end else if (coefsela == 4) begin
			coefa <= 20;
		end else if (coefsela == 5) begin
			coefa <= -20;
		end else if (coefsela == 6) begin
			coefa <= 25;
		end else if (coefsela == 7) begin
			coefa <= -25;
		end else begin
			coefa <= 0;
		end
		if (coefselb == 0) begin
			coefb <= 5;
		end else if (coefselb == 1) begin
			coefb <= -5;
		end else if (coefselb == 2) begin
			coefb <= 10;
		end else if (coefselb == 3) begin
			coefb <= -10;
		end else if (coefselb == 4) begin
			coefb <= 20;
		end else if (coefselb == 5) begin
			coefb <= -20;
		end else if (coefselb == 6) begin
			coefb <= 25;
		end else if (coefselb == 7) begin
			coefb <= -25;
		end else begin
			coefb <= 0;
		end
		resa_reg <= ay_reg * coefa;
		resb_reg <= by_reg * coefb;
	end
end

endmodule

module winograd_transform_0 (
	input clk,
	input i_valid,
	input [15:0] i_result_0_0,
	input [15:0] i_result_0_1,
	input [15:0] i_result_1_0,
	input [15:0] i_result_1_1,
	input [15:0] i_result_2_0,
	input [15:0] i_result_2_1,
	input [15:0] i_result_3_0,
	input [15:0] i_result_3_1,
	input [15:0] i_result_4_0,
	input [15:0] i_result_4_1,
	input [15:0] i_result_5_0,
	input [15:0] i_result_5_1,
	output [15:0] o_feature_0,
	output [15:0] o_feature_1,
	output [15:0] o_feature_2,
	output [15:0] o_feature_3,
	output [15:0] o_feature_4,
	output [15:0] o_feature_5,
	output o_valid
);

reg [15:0] input_buffer_0_0;
reg [19:0] output_buffer_0_0;
wire [19:0] rslt_buffer_0_0;
reg [15:0] input_buffer_0_1;
reg [19:0] output_buffer_0_1;
wire [19:0] rslt_buffer_0_1;
reg [15:0] input_buffer_0_2;
reg [19:0] output_buffer_0_2;
wire [19:0] rslt_buffer_0_2;
reg [15:0] input_buffer_0_3;
reg [19:0] output_buffer_0_3;
wire [19:0] rslt_buffer_0_3;
reg [15:0] input_buffer_0_4;
reg [19:0] output_buffer_0_4;
wire [19:0] rslt_buffer_0_4;
reg [15:0] input_buffer_0_5;
reg [19:0] output_buffer_0_5;
wire [19:0] rslt_buffer_0_5;
reg [15:0] input_buffer_1_0;
reg [19:0] output_buffer_1_0;
wire [19:0] rslt_buffer_1_0;
reg [15:0] input_buffer_1_1;
reg [19:0] output_buffer_1_1;
wire [19:0] rslt_buffer_1_1;
reg [15:0] input_buffer_1_2;
reg [19:0] output_buffer_1_2;
wire [19:0] rslt_buffer_1_2;
reg [15:0] input_buffer_1_3;
reg [19:0] output_buffer_1_3;
wire [19:0] rslt_buffer_1_3;
reg [15:0] input_buffer_1_4;
reg [19:0] output_buffer_1_4;
wire [19:0] rslt_buffer_1_4;
reg [15:0] input_buffer_1_5;
reg [19:0] output_buffer_1_5;
wire [19:0] rslt_buffer_1_5;
reg [15:0] input_buffer_2_0;
reg [19:0] output_buffer_2_0;
wire [19:0] rslt_buffer_2_0;
reg [15:0] input_buffer_2_1;
reg [19:0] output_buffer_2_1;
wire [19:0] rslt_buffer_2_1;
reg [15:0] input_buffer_2_2;
reg [19:0] output_buffer_2_2;
wire [19:0] rslt_buffer_2_2;
reg [15:0] input_buffer_2_3;
reg [19:0] output_buffer_2_3;
wire [19:0] rslt_buffer_2_3;
reg [15:0] input_buffer_2_4;
reg [19:0] output_buffer_2_4;
wire [19:0] rslt_buffer_2_4;
reg [15:0] input_buffer_2_5;
reg [19:0] output_buffer_2_5;
wire [19:0] rslt_buffer_2_5;
reg [15:0] input_buffer_3_0;
reg [19:0] output_buffer_3_0;
wire [19:0] rslt_buffer_3_0;
reg [15:0] input_buffer_3_1;
reg [19:0] output_buffer_3_1;
wire [19:0] rslt_buffer_3_1;
reg [15:0] input_buffer_3_2;
reg [19:0] output_buffer_3_2;
wire [19:0] rslt_buffer_3_2;
reg [15:0] input_buffer_3_3;
reg [19:0] output_buffer_3_3;
wire [19:0] rslt_buffer_3_3;
reg [15:0] input_buffer_3_4;
reg [19:0] output_buffer_3_4;
wire [19:0] rslt_buffer_3_4;
reg [15:0] input_buffer_3_5;
reg [19:0] output_buffer_3_5;
wire [19:0] rslt_buffer_3_5;
reg [15:0] input_buffer_4_0;
reg [19:0] output_buffer_4_0;
wire [19:0] rslt_buffer_4_0;
reg [15:0] input_buffer_4_1;
reg [19:0] output_buffer_4_1;
wire [19:0] rslt_buffer_4_1;
reg [15:0] input_buffer_4_2;
reg [19:0] output_buffer_4_2;
wire [19:0] rslt_buffer_4_2;
reg [15:0] input_buffer_4_3;
reg [19:0] output_buffer_4_3;
wire [19:0] rslt_buffer_4_3;
reg [15:0] input_buffer_4_4;
reg [19:0] output_buffer_4_4;
wire [19:0] rslt_buffer_4_4;
reg [15:0] input_buffer_4_5;
reg [19:0] output_buffer_4_5;
wire [19:0] rslt_buffer_4_5;
reg [15:0] input_buffer_5_0;
reg [19:0] output_buffer_5_0;
wire [19:0] rslt_buffer_5_0;
reg [15:0] input_buffer_5_1;
reg [19:0] output_buffer_5_1;
wire [19:0] rslt_buffer_5_1;
reg [15:0] input_buffer_5_2;
reg [19:0] output_buffer_5_2;
wire [19:0] rslt_buffer_5_2;
reg [15:0] input_buffer_5_3;
reg [19:0] output_buffer_5_3;
wire [19:0] rslt_buffer_5_3;
reg [15:0] input_buffer_5_4;
reg [19:0] output_buffer_5_4;
wire [19:0] rslt_buffer_5_4;
reg [15:0] input_buffer_5_5;
reg [19:0] output_buffer_5_5;
wire [19:0] rslt_buffer_5_5;
reg calculate, calculate_1, calculate_2, calculate_3;
reg valid_0;
reg valid_1;
reg valid_2;
reg valid_3;
reg valid_4;
reg valid_5;
reg valid_6;
reg valid_7;
reg valid_8;
reg valid_9;
reg valid_10;
reg valid_11;
reg valid_12;
reg [2:0] input_buffer_count;
wire [15:0] dsp_out_1_0;
wire [15:0] dsp_out_1_1;
wire [15:0] dsp_out_1_2;
wire [15:0] dsp_out_1_3;
wire [15:0] dsp_out_1_4;
wire [15:0] dsp_out_1_5;
wire [15:0] dsp_out_1_6;
wire [15:0] dsp_out_1_7;
wire [15:0] dsp_out_1_8;
wire [15:0] dsp_out_1_9;
wire [15:0] dsp_out_1_10;
wire [15:0] dsp_out_1_11;
wire [15:0] dsp_out_2_0;
wire [15:0] dsp_out_2_1;
wire [15:0] dsp_out_2_2;
wire [15:0] dsp_out_2_3;
wire [15:0] dsp_out_2_4;
wire [15:0] dsp_out_2_5;
wire [15:0] dsp_out_2_6;
wire [15:0] dsp_out_2_7;
wire [15:0] dsp_out_3_0;
wire [15:0] dsp_out_3_1;
wire [15:0] dsp_out_3_2;
wire [15:0] dsp_out_3_3;
wire [15:0] dsp_out_3_4;
wire [15:0] dsp_out_3_5;
wire [15:0] dsp_out_3_6;
wire [15:0] dsp_out_3_7;
wire [15:0] dsp_out_4_0;
wire [15:0] dsp_out_4_1;
wire [15:0] dsp_out_4_2;
wire [15:0] dsp_out_4_3;
wire [15:0] dsp_out_4_4;
wire [15:0] dsp_out_4_5;
wire [15:0] dsp_out_4_6;
wire [15:0] dsp_out_4_7;
wire [15:0] dsp_out_5_0;
wire [15:0] dsp_out_5_1;
wire [15:0] dsp_out_5_2;
wire [15:0] dsp_out_5_3;
wire [15:0] dsp_out_5_4;
wire [15:0] dsp_out_5_5;
wire [15:0] dsp_out_5_6;
wire [15:0] dsp_out_5_7;
wire [15:0] dsp_out_6_0;
wire [15:0] dsp_out_6_1;
wire [15:0] dsp_out_6_2;
wire [15:0] dsp_out_6_3;
wire [15:0] dsp_out_6_4;
wire [15:0] dsp_out_6_5;
wire [15:0] dsp_out_6_6;
wire [15:0] dsp_out_6_7;
wire [15:0] dsp_out_6_8;
wire [15:0] dsp_out_6_9;
wire [15:0] dsp_out_6_10;
wire [15:0] dsp_out_6_11;
reg [15:0] feature_reg_0_0_0;
reg [15:0] feature_reg_0_0_1;
reg [15:0] feature_reg_0_1_0;
reg [15:0] feature_reg_0_1_1;
reg [15:0] feature_reg_0_2_0;
reg [15:0] feature_reg_0_2_1;
reg [15:0] feature_reg_0_3_0;
reg [15:0] feature_reg_0_3_1;
reg [15:0] feature_reg_0_4_0;
reg [15:0] feature_reg_0_4_1;
reg [15:0] feature_reg_0_5_0;
reg [15:0] feature_reg_0_5_1;
reg [15:0] feature_reg_1_0_0;
reg [15:0] feature_reg_1_0_1;
reg [15:0] feature_reg_1_1_0;
reg [15:0] feature_reg_1_1_1;
reg [15:0] feature_reg_1_2_0;
reg [15:0] feature_reg_1_2_1;
reg [15:0] feature_reg_1_3_0;
reg [15:0] feature_reg_1_3_1;
reg [15:0] feature_reg_1_4_0;
reg [15:0] feature_reg_1_4_1;
reg [15:0] feature_reg_1_5_0;
reg [15:0] feature_reg_1_5_1;
reg [15:0] feature_reg_2_0_0;
reg [15:0] feature_reg_2_0_1;
reg [15:0] feature_reg_2_1_0;
reg [15:0] feature_reg_2_1_1;
reg [15:0] feature_reg_2_2_0;
reg [15:0] feature_reg_2_2_1;
reg [15:0] feature_reg_2_3_0;
reg [15:0] feature_reg_2_3_1;
reg [15:0] feature_reg_2_4_0;
reg [15:0] feature_reg_2_4_1;
reg [15:0] feature_reg_2_5_0;
reg [15:0] feature_reg_2_5_1;
reg [15:0] feature_reg_3_0_0;
reg [15:0] feature_reg_3_0_1;
reg [15:0] feature_reg_3_1_0;
reg [15:0] feature_reg_3_1_1;
reg [15:0] feature_reg_3_2_0;
reg [15:0] feature_reg_3_2_1;
reg [15:0] feature_reg_3_3_0;
reg [15:0] feature_reg_3_3_1;
reg [15:0] feature_reg_3_4_0;
reg [15:0] feature_reg_3_4_1;
reg [15:0] feature_reg_3_5_0;
reg [15:0] feature_reg_3_5_1;
reg [15:0] feature_reg_4_0_0;
reg [15:0] feature_reg_4_0_1;
reg [15:0] feature_reg_4_1_0;
reg [15:0] feature_reg_4_1_1;
reg [15:0] feature_reg_4_2_0;
reg [15:0] feature_reg_4_2_1;
reg [15:0] feature_reg_4_3_0;
reg [15:0] feature_reg_4_3_1;
reg [15:0] feature_reg_4_4_0;
reg [15:0] feature_reg_4_4_1;
reg [15:0] feature_reg_4_5_0;
reg [15:0] feature_reg_4_5_1;
reg [15:0] feature_reg_5_0_0;
reg [15:0] feature_reg_5_0_1;
reg [15:0] feature_reg_5_1_0;
reg [15:0] feature_reg_5_1_1;
reg [15:0] feature_reg_5_2_0;
reg [15:0] feature_reg_5_2_1;
reg [15:0] feature_reg_5_3_0;
reg [15:0] feature_reg_5_3_1;
reg [15:0] feature_reg_5_4_0;
reg [15:0] feature_reg_5_4_1;
reg [15:0] feature_reg_5_5_0;
reg [15:0] feature_reg_5_5_1;

always @ (posedge clk) begin
	calculate_1 <= calculate;
	calculate_2 <= calculate_1;
	calculate_3 <= calculate_2;
	//Valid pipeline
	valid_0 <= i_valid;
	valid_1 <= valid_0;
	valid_2 <= valid_1;
	valid_3 <= valid_2;
	valid_4 <= valid_3;
	valid_5 <= valid_4;
	valid_6 <= valid_5;
	valid_7 <= valid_6;
	valid_8 <= valid_7;
	valid_9 <= valid_8;
	valid_10 <= valid_9;
	valid_11 <= valid_10;
	valid_12 <= valid_11;
	if (i_valid) begin
		input_buffer_count <= 0;
		calculate <= 0;
	end else begin
		//Input buffering logic
		if (input_buffer_count == 5) begin
			calculate <= 1;
			input_buffer_count <= 0;
		end else begin
			calculate <= 0;
			input_buffer_count <= input_buffer_count + 1'b1;
		end
		input_buffer_5_0 <= i_result_0_0;
		input_buffer_5_1 <= i_result_1_0;
		input_buffer_5_2 <= i_result_2_0;
		input_buffer_5_3 <= i_result_3_0;
		input_buffer_5_4 <= i_result_4_0;
		input_buffer_5_5 <= i_result_5_0;
	end
	input_buffer_0_0 <= input_buffer_1_0;
	input_buffer_0_1 <= input_buffer_1_1;
	input_buffer_0_2 <= input_buffer_1_2;
	input_buffer_0_3 <= input_buffer_1_3;
	input_buffer_0_4 <= input_buffer_1_4;
	input_buffer_0_5 <= input_buffer_1_5;
	input_buffer_1_0 <= input_buffer_2_0;
	input_buffer_1_1 <= input_buffer_2_1;
	input_buffer_1_2 <= input_buffer_2_2;
	input_buffer_1_3 <= input_buffer_2_3;
	input_buffer_1_4 <= input_buffer_2_4;
	input_buffer_1_5 <= input_buffer_2_5;
	input_buffer_2_0 <= input_buffer_3_0;
	input_buffer_2_1 <= input_buffer_3_1;
	input_buffer_2_2 <= input_buffer_3_2;
	input_buffer_2_3 <= input_buffer_3_3;
	input_buffer_2_4 <= input_buffer_3_4;
	input_buffer_2_5 <= input_buffer_3_5;
	input_buffer_3_0 <= input_buffer_4_0;
	input_buffer_3_1 <= input_buffer_4_1;
	input_buffer_3_2 <= input_buffer_4_2;
	input_buffer_3_3 <= input_buffer_4_3;
	input_buffer_3_4 <= input_buffer_4_4;
	input_buffer_3_5 <= input_buffer_4_5;
	input_buffer_4_0 <= input_buffer_5_0;
	input_buffer_4_1 <= input_buffer_5_1;
	input_buffer_4_2 <= input_buffer_5_2;
	input_buffer_4_3 <= input_buffer_5_3;
	input_buffer_4_4 <= input_buffer_5_4;
	input_buffer_4_5 <= input_buffer_5_5;
	//Pipelining to synchronize DSPs and non-DSPs
	feature_reg_0_0_0 <= input_buffer_0_0;
	feature_reg_0_1_0 <= input_buffer_0_1;
	feature_reg_0_2_0 <= input_buffer_0_2;
	feature_reg_0_3_0 <= input_buffer_0_3;
	feature_reg_0_4_0 <= input_buffer_0_4;
	feature_reg_0_5_0 <= input_buffer_0_5;
	feature_reg_1_0_0 <= input_buffer_1_0;
	feature_reg_1_1_0 <= input_buffer_1_1;
	feature_reg_1_2_0 <= input_buffer_1_2;
	feature_reg_1_3_0 <= input_buffer_1_3;
	feature_reg_1_4_0 <= input_buffer_1_4;
	feature_reg_1_5_0 <= input_buffer_1_5;
	feature_reg_2_0_0 <= input_buffer_2_0;
	feature_reg_2_1_0 <= input_buffer_2_1;
	feature_reg_2_2_0 <= input_buffer_2_2;
	feature_reg_2_3_0 <= input_buffer_2_3;
	feature_reg_2_4_0 <= input_buffer_2_4;
	feature_reg_2_5_0 <= input_buffer_2_5;
	feature_reg_3_0_0 <= input_buffer_3_0;
	feature_reg_3_1_0 <= input_buffer_3_1;
	feature_reg_3_2_0 <= input_buffer_3_2;
	feature_reg_3_3_0 <= input_buffer_3_3;
	feature_reg_3_4_0 <= input_buffer_3_4;
	feature_reg_3_5_0 <= input_buffer_3_5;
	feature_reg_4_0_0 <= input_buffer_4_0;
	feature_reg_4_1_0 <= input_buffer_4_1;
	feature_reg_4_2_0 <= input_buffer_4_2;
	feature_reg_4_3_0 <= input_buffer_4_3;
	feature_reg_4_4_0 <= input_buffer_4_4;
	feature_reg_4_5_0 <= input_buffer_4_5;
	feature_reg_5_0_0 <= input_buffer_5_0;
	feature_reg_5_1_0 <= input_buffer_5_1;
	feature_reg_5_2_0 <= input_buffer_5_2;
	feature_reg_5_3_0 <= input_buffer_5_3;
	feature_reg_5_4_0 <= input_buffer_5_4;
	feature_reg_5_5_0 <= input_buffer_5_5;
	feature_reg_0_0_1 <= feature_reg_0_0_0;
	feature_reg_0_1_1 <= feature_reg_0_1_0;
	feature_reg_0_2_1 <= feature_reg_0_2_0;
	feature_reg_0_3_1 <= feature_reg_0_3_0;
	feature_reg_0_4_1 <= feature_reg_0_4_0;
	feature_reg_0_5_1 <= feature_reg_0_5_0;
	feature_reg_1_0_1 <= feature_reg_1_0_0;
	feature_reg_1_1_1 <= feature_reg_1_1_0;
	feature_reg_1_2_1 <= feature_reg_1_2_0;
	feature_reg_1_3_1 <= feature_reg_1_3_0;
	feature_reg_1_4_1 <= feature_reg_1_4_0;
	feature_reg_1_5_1 <= feature_reg_1_5_0;
	feature_reg_2_0_1 <= feature_reg_2_0_0;
	feature_reg_2_1_1 <= feature_reg_2_1_0;
	feature_reg_2_2_1 <= feature_reg_2_2_0;
	feature_reg_2_3_1 <= feature_reg_2_3_0;
	feature_reg_2_4_1 <= feature_reg_2_4_0;
	feature_reg_2_5_1 <= feature_reg_2_5_0;
	feature_reg_3_0_1 <= feature_reg_3_0_0;
	feature_reg_3_1_1 <= feature_reg_3_1_0;
	feature_reg_3_2_1 <= feature_reg_3_2_0;
	feature_reg_3_3_1 <= feature_reg_3_3_0;
	feature_reg_3_4_1 <= feature_reg_3_4_0;
	feature_reg_3_5_1 <= feature_reg_3_5_0;
	feature_reg_4_0_1 <= feature_reg_4_0_0;
	feature_reg_4_1_1 <= feature_reg_4_1_0;
	feature_reg_4_2_1 <= feature_reg_4_2_0;
	feature_reg_4_3_1 <= feature_reg_4_3_0;
	feature_reg_4_4_1 <= feature_reg_4_4_0;
	feature_reg_4_5_1 <= feature_reg_4_5_0;
	feature_reg_5_0_1 <= feature_reg_5_0_0;
	feature_reg_5_1_1 <= feature_reg_5_1_0;
	feature_reg_5_2_1 <= feature_reg_5_2_0;
	feature_reg_5_3_1 <= feature_reg_5_3_0;
	feature_reg_5_4_1 <= feature_reg_5_4_0;
	feature_reg_5_5_1 <= feature_reg_5_5_0;
	//Output Serializing logic
	if (calculate_3) begin
		output_buffer_0_0 <= rslt_buffer_0_0;
		output_buffer_1_0 <= rslt_buffer_0_1;
		output_buffer_2_0 <= rslt_buffer_0_2;
		output_buffer_3_0 <= rslt_buffer_0_3;
		output_buffer_4_0 <= rslt_buffer_0_4;
		output_buffer_5_0 <= rslt_buffer_0_5;
		output_buffer_0_1 <= rslt_buffer_1_0;
		output_buffer_1_1 <= rslt_buffer_1_1;
		output_buffer_2_1 <= rslt_buffer_1_2;
		output_buffer_3_1 <= rslt_buffer_1_3;
		output_buffer_4_1 <= rslt_buffer_1_4;
		output_buffer_5_1 <= rslt_buffer_1_5;
		output_buffer_0_2 <= rslt_buffer_2_0;
		output_buffer_1_2 <= rslt_buffer_2_1;
		output_buffer_2_2 <= rslt_buffer_2_2;
		output_buffer_3_2 <= rslt_buffer_2_3;
		output_buffer_4_2 <= rslt_buffer_2_4;
		output_buffer_5_2 <= rslt_buffer_2_5;
		output_buffer_0_3 <= rslt_buffer_3_0;
		output_buffer_1_3 <= rslt_buffer_3_1;
		output_buffer_2_3 <= rslt_buffer_3_2;
		output_buffer_3_3 <= rslt_buffer_3_3;
		output_buffer_4_3 <= rslt_buffer_3_4;
		output_buffer_5_3 <= rslt_buffer_3_5;
		output_buffer_0_4 <= rslt_buffer_4_0;
		output_buffer_1_4 <= rslt_buffer_4_1;
		output_buffer_2_4 <= rslt_buffer_4_2;
		output_buffer_3_4 <= rslt_buffer_4_3;
		output_buffer_4_4 <= rslt_buffer_4_4;
		output_buffer_5_4 <= rslt_buffer_4_5;
		output_buffer_0_5 <= rslt_buffer_5_0;
		output_buffer_1_5 <= rslt_buffer_5_1;
		output_buffer_2_5 <= rslt_buffer_5_2;
		output_buffer_3_5 <= rslt_buffer_5_3;
		output_buffer_4_5 <= rslt_buffer_5_4;
		output_buffer_5_5 <= rslt_buffer_5_5;
	end else begin
		output_buffer_0_0 <= output_buffer_0_1;
		output_buffer_0_1 <= output_buffer_0_2;
		output_buffer_0_2 <= output_buffer_0_3;
		output_buffer_0_3 <= output_buffer_0_4;
		output_buffer_0_4 <= output_buffer_0_5;
		output_buffer_1_0 <= output_buffer_1_1;
		output_buffer_1_1 <= output_buffer_1_2;
		output_buffer_1_2 <= output_buffer_1_3;
		output_buffer_1_3 <= output_buffer_1_4;
		output_buffer_1_4 <= output_buffer_1_5;
		output_buffer_2_0 <= output_buffer_2_1;
		output_buffer_2_1 <= output_buffer_2_2;
		output_buffer_2_2 <= output_buffer_2_3;
		output_buffer_2_3 <= output_buffer_2_4;
		output_buffer_2_4 <= output_buffer_2_5;
		output_buffer_3_0 <= output_buffer_3_1;
		output_buffer_3_1 <= output_buffer_3_2;
		output_buffer_3_2 <= output_buffer_3_3;
		output_buffer_3_3 <= output_buffer_3_4;
		output_buffer_3_4 <= output_buffer_3_5;
		output_buffer_4_0 <= output_buffer_4_1;
		output_buffer_4_1 <= output_buffer_4_2;
		output_buffer_4_2 <= output_buffer_4_3;
		output_buffer_4_3 <= output_buffer_4_4;
		output_buffer_4_4 <= output_buffer_4_5;
		output_buffer_5_0 <= output_buffer_5_1;
		output_buffer_5_1 <= output_buffer_5_2;
		output_buffer_5_2 <= output_buffer_5_3;
		output_buffer_5_3 <= output_buffer_5_4;
		output_buffer_5_4 <= output_buffer_5_5;
	end
end

////// FIRST COLUMN //////
winograd_dsp_16 winograd_dsp_16_WD00 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_0_2),
	.by(input_buffer_2_0),
	.coefsela(3'b101),
	.coefselb(3'b101),
	.resulta(dsp_out_1_0),
	.resultb(dsp_out_1_1)
);

winograd_dsp_16 winograd_dsp_16_WD10 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_2),
	.by(input_buffer_2_4),
	.coefsela(3'b110),
	.coefselb(3'b001),
	.resulta(dsp_out_1_2),
	.resultb(dsp_out_1_3)
);

winograd_dsp_16 winograd_dsp_16_WD20 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_4_2),
	.by(input_buffer_1_0),
	.coefsela(3'b001),
	.coefselb(3'b100),
	.resulta(dsp_out_1_4),
	.resultb(dsp_out_1_5)
);

winograd_dsp_16 winograd_dsp_16_WD30 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_2),
	.by(input_buffer_3_2),
	.coefsela(3'b100),
	.coefselb(3'b001),
	.resulta(dsp_out_1_6),
	.resultb(dsp_out_1_7)
);

winograd_dsp_16 winograd_dsp_16_WD40 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_0),
	.by(input_buffer_3_2),
	.coefsela(3'b101),
	.coefselb(3'b110),
	.resulta(dsp_out_1_8),
	.resultb(dsp_out_1_9)
);

winograd_dsp_16 winograd_dsp_16_WD50 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_4),
	.by(input_buffer_5_2),
	.coefsela(3'b001),
	.coefselb(3'b001),
	.resulta(dsp_out_1_10),
	.resultb(dsp_out_1_11)
);

////// SECOND COLUMN //////
winograd_dsp_16 winograd_dsp_16_WD01 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_1),
	.by(input_buffer_2_2),
	.coefsela(3'b100),
	.coefselb(3'b100),
	.resulta(dsp_out_2_0),
	.resultb(dsp_out_2_1)
);

winograd_dsp_16 winograd_dsp_16_WD11 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_3),
	.by(input_buffer_2_4),
	.coefsela(3'b001),
	.coefselb(3'b001),
	.resulta(dsp_out_2_2),
	.resultb(dsp_out_2_3)
);

winograd_dsp_16 winograd_dsp_16_WD21 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_1),
	.by(input_buffer_3_2),
	.coefsela(3'b100),
	.coefselb(3'b100),
	.resulta(dsp_out_2_4),
	.resultb(dsp_out_2_5)
);

winograd_dsp_16 winograd_dsp_16_WD31 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_3),
	.by(input_buffer_3_4),
	.coefsela(3'b001),
	.coefselb(3'b001),
	.resulta(dsp_out_2_6),
	.resultb(dsp_out_2_7)
);

////// THIRD COLUMN //////
winograd_dsp_16 winograd_dsp_16_WD02 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_1),
	.by(input_buffer_2_2),
	.coefsela(3'b101),
	.coefselb(3'b100),
	.resulta(dsp_out_3_0),
	.resultb(dsp_out_3_1)
);

winograd_dsp_16 winograd_dsp_16_WD12 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_3),
	.by(input_buffer_2_4),
	.coefsela(3'b000),
	.coefselb(3'b001),
	.resulta(dsp_out_3_2),
	.resultb(dsp_out_3_3)
);

winograd_dsp_16 winograd_dsp_16_WD22 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_1),
	.by(input_buffer_3_2),
	.coefsela(3'b101),
	.coefselb(3'b100),
	.resulta(dsp_out_3_4),
	.resultb(dsp_out_3_5)
);

winograd_dsp_16 winograd_dsp_16_WD32 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_3),
	.by(input_buffer_3_4),
	.coefsela(3'b000),
	.coefselb(3'b001),
	.resulta(dsp_out_3_6),
	.resultb(dsp_out_3_7)
);

////// FOURTH COLUMN //////
winograd_dsp_16 winograd_dsp_16_WD03 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_1),
	.by(input_buffer_2_2),
	.coefsela(3'b010),
	.coefselb(3'b000),
	.resulta(dsp_out_4_0),
	.resultb(dsp_out_4_1)
);

winograd_dsp_16 winograd_dsp_16_WD13 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_3),
	.by(input_buffer_2_4),
	.coefsela(3'b011),
	.coefselb(3'b001),
	.resulta(dsp_out_4_2),
	.resultb(dsp_out_4_3)
);

winograd_dsp_16 winograd_dsp_16_WD23 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_1),
	.by(input_buffer_3_2),
	.coefsela(3'b010),
	.coefselb(3'b000),
	.resulta(dsp_out_4_4),
	.resultb(dsp_out_4_5)
);

winograd_dsp_16 winograd_dsp_16_WD33 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_3),
	.by(input_buffer_3_4),
	.coefsela(3'b011),
	.coefselb(3'b001),
	.resulta(dsp_out_4_6),
	.resultb(dsp_out_4_7)
);

////// FIFTH COLUMN //////
winograd_dsp_16 winograd_dsp_16_WD04 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_1),
	.by(input_buffer_2_2),
	.coefsela(3'b011),
	.coefselb(3'b000),
	.resulta(dsp_out_5_0),
	.resultb(dsp_out_5_1)
);

winograd_dsp_16 winograd_dsp_16_WD14 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_3),
	.by(input_buffer_2_4),
	.coefsela(3'b010),
	.coefselb(3'b001),
	.resulta(dsp_out_5_2),
	.resultb(dsp_out_5_3)
);

winograd_dsp_16 winograd_dsp_16_WD24 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_1),
	.by(input_buffer_3_2),
	.coefsela(3'b011),
	.coefselb(3'b000),
	.resulta(dsp_out_5_4),
	.resultb(dsp_out_5_5)
);

winograd_dsp_16 winograd_dsp_16_WD34 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_3),
	.by(input_buffer_3_4),
	.coefsela(3'b010),
	.coefselb(3'b001),
	.resulta(dsp_out_5_6),
	.resultb(dsp_out_5_7)
);

////// SIXTH COLUMN //////
winograd_dsp_16 winograd_dsp_16_WD05 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_0_3),
	.by(input_buffer_2_1),
	.coefsela(3'b101),
	.coefselb(3'b101),
	.resulta(dsp_out_6_0),
	.resultb(dsp_out_6_1)
);

winograd_dsp_16 winograd_dsp_16_WD15 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_3),
	.by(input_buffer_2_5),
	.coefsela(3'b110),
	.coefselb(3'b001),
	.resulta(dsp_out_6_2),
	.resultb(dsp_out_6_3)
);

winograd_dsp_16 winograd_dsp_16_WD25 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_4_3),
	.by(input_buffer_1_3),
	.coefsela(3'b001),
	.coefselb(3'b100),
	.resulta(dsp_out_6_4),
	.resultb(dsp_out_6_5)
);

winograd_dsp_16 winograd_dsp_16_WD35 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_2_3),
	.by(input_buffer_3_3),
	.coefsela(3'b100),
	.coefselb(3'b001),
	.resulta(dsp_out_6_6),
	.resultb(dsp_out_6_7)
);

winograd_dsp_16 winograd_dsp_16_WD45 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_1),
	.by(input_buffer_3_3),
	.coefsela(3'b101),
	.coefselb(3'b110),
	.resulta(dsp_out_6_8),
	.resultb(dsp_out_6_9)
);

winograd_dsp_16 winograd_dsp_16_WD55 (
	.clk(clk),
	.ena(1'b1),
	.aclr(1'b0),
	.ay(input_buffer_3_5),
	.by(input_buffer_5_3),
	.coefsela(3'b001),
	.coefselb(3'b001),
	.resulta(dsp_out_6_10),
	.resultb(dsp_out_6_11)
);

winograd_adder_16_20_4 winograd_adder_16_20_4_WA00 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_0_0_1[11:0], 4'b0000}),
	.data1x(dsp_out_1_0),
	.data2x({feature_reg_0_4_1[13:0], 2'b00}),
	.data3x(dsp_out_1_1),
	.data4x(dsp_out_1_2),
	.data5x(dsp_out_1_3),
	.data6x({feature_reg_4_0_1[13:0], 2'b00}),
	.data7x(dsp_out_1_4),
	.data8x(feature_reg_4_4_1),
	.data9x(0),
	.data10x(0),
	.data11x(0),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_0_0)
);

wire [15:0] f1, f2, f3, f4;
assign f1 = -{feature_reg_1_0_1[11:0], 4'b0000};
assign f2 = -{feature_reg_1_4_1[13:0], 2'b00};
assign f3 = -{feature_reg_2_0_1[11:0], 4'b0000};
assign f4 = -{feature_reg_2_4_1[13:0], 2'b00};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA10 (
	.clock(clk),
	.clken(calculate_2),
	.data0x(dsp_out_1_5),
	.data1x(f1),
	.data2x(f2),
	.data3x(f3),
	.data4x(dsp_out_1_6),
	.data5x(f4),
	.data6x({feature_reg_3_0_1[13:0], 2'b00}),
	.data7x(dsp_out_1_7),
	.data8x(feature_reg_3_4_1),
	.data9x({feature_reg_4_0_1[13:0], 2'b00}),
	.data10x(dsp_out_1_4),
	.data11x(feature_reg_4_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_1_0)
);

wire [15:0] f5, f6, f7, f8, f9, f10;
assign f5 = -dsp_out_1_5;
assign f6 = -{feature_reg_2_0_1[11:0], 4'b0000};
assign f7 = -{feature_reg_2_4_1[13:0], 2'b00};
assign f8 = -{feature_reg_3_0_1[13:0], 2'b00};
assign f9 = -dsp_out_1_7;
assign f10 = -feature_reg_3_4_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA20 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_0_1[11:0], 4'b0000}),
	.data1x(f5),
	.data2x({feature_reg_1_4_1[13:0], 2'b00}),
	.data3x(f6),
	.data4x(dsp_out_1_6),
	.data5x(f7),
	.data6x(f8),
	.data7x(f9),
	.data8x(f10),
	.data9x({feature_reg_4_0_1[13:0], 2'b00}),
	.data10x(dsp_out_1_4),
	.data11x(feature_reg_4_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_2_0)
);

wire [15:0] f11, f12, f13, f14, f15, f16, f17;
assign f11 = -{feature_reg_1_0_1[12:0], 3'b000};
assign f12 = -{feature_reg_1_4_1[14:0], 1'b0};
assign f13 = -{feature_reg_2_0_1[13:0], 2'b00};
assign f14 = -feature_reg_2_4_1;
assign f15 = dsp_out_1_5 >>> 1;
assign f16 = dsp_out_1_6 >>> 2;
assign f17 = dsp_out_1_7 <<< 1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA30 (
	.clock(clk),
	.clken(calculate_2),
	.data0x(f15),
	.data1x(f11),
	.data2x(f12),
	.data3x(f13),
	.data4x(f16),
	.data5x(f14),
	.data6x({feature_reg_3_0_1[12:0], 3'b000}),
	.data7x(f17),
	.data8x({feature_reg_3_4_1[14:0], 1'b0}),
	.data9x({feature_reg_4_0_1[13:0], 2'b00}),
	.data10x(dsp_out_1_4),
	.data11x(feature_reg_4_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_3_0)
);

wire [15:0] f18, f19, f20, f21, f22, f23, f23b;
assign f18 = -(dsp_out_1_5 >>> 1);
assign f19 = -{feature_reg_2_0_1[13:0], 2'b00};
assign f20 = -feature_reg_2_4_1;
assign f21 = -{feature_reg_3_0_1[12:0], 3'b000};
assign f22 = -(dsp_out_1_7 <<< 1);
assign f23 = -{feature_reg_3_4_1[14:0], 1'b0};
assign f23b = dsp_out_1_6 >>> 2;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA40 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_0_1[12:0], 3'b000}),
	.data1x(f18),
	.data2x({feature_reg_1_4_1[14:0], 1'b0}),
	.data3x(f19),
	.data4x(f23b),
	.data5x(f20),
	.data6x(f21),
	.data7x(f22),
	.data8x(f23),
	.data9x({feature_reg_4_0_1[13:0], 2'b00}),
	.data10x(dsp_out_1_4),
	.data11x(feature_reg_4_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_4_0)
);

wire [15:0] f24;
assign f24 = -dsp_out_1_5;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA50 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_0_1[11:0], 4'b0000}),
	.data1x(f24),
	.data2x({feature_reg_1_4_1[13:0], 2'b00}),
	.data3x(dsp_out_1_8),
	.data4x(dsp_out_1_9),
	.data5x(dsp_out_1_10),
	.data6x({feature_reg_5_0_1[13:0], 2'b00}),
	.data7x(dsp_out_1_11),
	.data8x(feature_reg_5_4_1),
	.data9x(0),
	.data10x(0),
	.data11x(0),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_5_0)
);

wire [15:0] f25, f26, f27, f28;
assign f25 = -{feature_reg_0_2_1[11:0], 4'b0000};
assign f26 = -{feature_reg_0_1_1[11:0], 4'b0000};
assign f27 = -{feature_reg_4_1_1[13:0], 2'b00};
assign f28 = -{feature_reg_4_2_1[13:0], 2'b00};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA01 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_0_3_1[13:0], 2'b00}),
	.data1x(f25),
	.data2x(f26),
	.data3x({feature_reg_0_4_1[13:0], 2'b00}),
	.data4x(dsp_out_2_0),
	.data5x(dsp_out_2_1),
	.data6x(dsp_out_2_2),
	.data7x(dsp_out_2_3),
	.data8x(f27),
	.data9x(f28),
	.data10x(feature_reg_4_3_1),
	.data11x(feature_reg_4_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_0_1)
);

wire [15:0] f29, f30, f31, f32, f33, f34, f35, f36;
assign f29 = -{feature_reg_1_3_1[13:0], 2'b00};
assign f30 = -{feature_reg_1_4_1[13:0], 2'b00};
assign f31 = -{feature_reg_2_3_1[13:0], 2'b00};
assign f32 = -{feature_reg_2_4_1[13:0], 2'b00};
assign f33 = -{feature_reg_3_1_1[13:0], 2'b00};
assign f34 = -{feature_reg_3_2_1[13:0], 2'b00};
assign f35 = -{feature_reg_4_1_1[13:0], 2'b00};
assign f36 = -{feature_reg_4_2_1[13:0], 2'b00};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA11 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[11:0], 4'b0000}),
	.data1x({feature_reg_1_2_1[11:0], 4'b0000}),
	.data2x(f29),
	.data3x(f30),
	.data4x({feature_reg_2_1_1[11:0], 4'b0000}),
	.data5x({feature_reg_2_2_1[11:0], 4'b0000}),
	.data6x(f31),
	.data7x(f32),
	.data8x(f33),
	.data9x(f34),
	.data10x(feature_reg_3_3_1),
	.data11x(feature_reg_3_4_1),
	.data12x(f35),
	.data13x(f36),
	.data14x(feature_reg_4_3_1),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_1_1)
);

wire [15:0] f37, f38, f39, f40, f41, f42, f43, f44;
assign f37 = -{feature_reg_1_2_1[11:0],4'b0000};
assign f38 = -{feature_reg_1_1_1[11:0],4'b0000};
assign f39 = -{feature_reg_2_3_1[13:0],2'b00};
assign f40 = -{feature_reg_2_4_1[13:0],2'b00};
assign f41 = -feature_reg_3_3_1;
assign f42 = -feature_reg_3_4_1;
assign f43 = -{feature_reg_4_1_1[13:0],2'b00};
assign f44 = -{feature_reg_4_2_1[13:0],2'b00};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA21 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_3_1[13:0],2'b00}),
	.data1x(f37),
	.data2x(f38),
	.data3x({feature_reg_1_4_1[13:0],2'b00}),
	.data4x({feature_reg_2_1_1[11:0],4'b0000}),
	.data5x({feature_reg_2_2_1[11:0],4'b0000}),
	.data6x(f39),
	.data7x(f40),
	.data8x({feature_reg_3_1_1[13:0],2'b00}),
	.data9x({feature_reg_3_2_1[13:0],2'b00}),
	.data10x(f41),
	.data11x(f42),
	.data12x(f43),
	.data13x(f44),
	.data14x(feature_reg_4_3_1),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_2_1)
);

wire [15:0] f45, f46, f47, f48, f49, f50, f51, f52;
assign f45 = -{feature_reg_1_3_1[14:0],1'b0};
assign f46 = -{feature_reg_1_4_1[14:0],1'b0};
assign f47 = -feature_reg_2_3_1;
assign f48 = -feature_reg_2_4_1;
assign f49 = -{feature_reg_3_1_1[12:0],3'b000};
assign f50 = -{feature_reg_3_2_1[12:0],3'b000};
assign f51 = -{feature_reg_4_1_1[13:0],2'b00};
assign f52 = -{feature_reg_4_2_1[13:0],2'b00};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA31 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[12:0],3'b000}),
	.data1x({feature_reg_1_2_1[12:0],3'b000}),
	.data2x(f45),
	.data3x(f46),
	.data4x({feature_reg_2_1_1[13:0],2'b00}),
	.data5x({feature_reg_2_2_1[13:0],2'b00}),
	.data6x(f47),
	.data7x(f48),
	.data8x(f49),
	.data9x(f50),
	.data10x({feature_reg_3_3_1[14:0],1'b0}),
	.data11x({feature_reg_3_4_1[14:0],1'b0}),
	.data12x(f51),
	.data13x(f52),
	.data14x(feature_reg_4_3_1),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_3_1)
);

wire [15:0] f53, f54, f55, f56, f57, f58, f59, f60;
assign f53 = -{feature_reg_1_2_1[12:0],3'b000};
assign f54 = -{feature_reg_1_1_1[12:0],3'b000};
assign f55 = -feature_reg_2_3_1;
assign f56 = -feature_reg_2_4_1;
assign f57 = -{feature_reg_3_3_1[14:0],1'b0};
assign f58 = -{feature_reg_3_4_1[14:0],1'b0};
assign f59 = -{feature_reg_4_1_1[13:0],2'b00};
assign f60 = -{feature_reg_4_2_1[13:0],2'b00};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA41 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_3_1[14:0],1'b0}),
	.data1x(f53),
	.data2x(f54),
	.data3x({feature_reg_1_4_1[14:0],1'b0}),
	.data4x({feature_reg_2_1_1[13:0],2'b00}),
	.data5x({feature_reg_2_2_1[13:0],2'b00}),
	.data6x(f55),
	.data7x(f56),
	.data8x({feature_reg_3_1_1[12:0],3'b000}),
	.data9x({feature_reg_3_2_1[12:0],3'b000}),
	.data10x(f57),
	.data11x(f58),
	.data12x(f59),
	.data13x(f60),
	.data14x(feature_reg_4_3_1),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_4_1)
);

wire [15:0] f61, f62, f63, f64;
assign f61 = -{feature_reg_1_2_1[11:0],4'b0000};
assign f62 = -{feature_reg_1_1_1[11:0],4'b0000};
assign f63 = -{feature_reg_5_1_1[13:0],2'b00};
assign f64 = -{feature_reg_5_2_1[13:0],2'b00};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA51 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_3_1[13:0],2'b00}),
	.data1x(f61),
	.data2x(f62),
	.data3x({feature_reg_1_4_1[13:0],2'b00}),
	.data4x(dsp_out_2_4),
	.data5x(dsp_out_2_5),
	.data6x(dsp_out_2_6),
	.data7x(dsp_out_2_7),
	.data8x(f63),
	.data9x(f64),
	.data10x(feature_reg_5_3_1),
	.data11x(feature_reg_5_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_5_1)
);

wire [15:0] f65, f66, f67, f68;
assign f65 = -{feature_reg_0_2_1[11:0],4'b0000};
assign f66 = -{feature_reg_0_3_1[13:0],2'b00};
assign f67 = -{feature_reg_4_2_1[13:0],2'b00};
assign f68 = -feature_reg_4_3_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA02 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_0_1_1[11:0],4'b0000}),
	.data1x(f65),
	.data2x(f66),
	.data3x({feature_reg_0_4_1[13:0],2'b00}),
	.data4x(dsp_out_3_0),
	.data5x(dsp_out_3_1),
	.data6x(dsp_out_3_2),
	.data7x(dsp_out_3_3),
	.data8x({feature_reg_4_1_1[13:0],2'b00}),
	.data9x(f67),
	.data10x(f68),
	.data11x(feature_reg_4_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_0_2)
);

wire [15:0] f69, f70, f71, f72, f73, f74, f75, f76;
assign f69 = -{feature_reg_1_1_1[11:0],4'b0000};
assign f70 = -{feature_reg_1_4_1[13:0],2'b00};
assign f71 = -{feature_reg_2_1_1[11:0],4'b0000};
assign f72 = -{feature_reg_2_4_1[13:0],2'b00};
assign f73 = -{feature_reg_3_2_1[13:0],2'b00};
assign f74 = -feature_reg_3_3_1;
assign f75 = -{feature_reg_4_2_1[13:0],2'b00};
assign f76 = -feature_reg_4_3_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA12 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_2_1[11:0],4'b0000}),
	.data1x(f69),
	.data2x({feature_reg_1_3_1[13:0],2'b00}),
	.data3x(f70),
	.data4x(f71),
	.data5x({feature_reg_2_2_1[11:0],4'b0000}),
	.data6x({feature_reg_2_3_1[13:0],2'b00}),
	.data7x(f72),
	.data8x({feature_reg_3_1_1[13:0],2'b00}),
	.data9x(f73),
	.data10x(f74),
	.data11x(feature_reg_3_4_1),
	.data12x({feature_reg_4_1_1[13:0],2'b00}),
	.data13x(f75),
	.data14x(f76),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_1_2)
);

wire [15:0] f77, f78, f79, f80, f81, f82, f83, f84;
assign f77 = -{feature_reg_1_2_1[11:0],4'b0000};
assign f78 = -{feature_reg_1_3_1[13:0],2'b00};
assign f79 = -{feature_reg_2_1_1[11:0],4'b0000};
assign f80 = -{feature_reg_2_4_1[13:0],2'b00};
assign f81 = -{feature_reg_3_1_1[13:0],2'b00};
assign f82 = -feature_reg_3_4_1;
assign f83 = -{feature_reg_4_2_1[13:0],2'b00};
assign f84 = -feature_reg_4_3_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA22 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[11:0],4'b0000}),
	.data1x(f77),
	.data2x(f78),
	.data3x({feature_reg_1_4_1[13:0],2'b00}),
	.data4x(f79),
	.data5x({feature_reg_2_2_1[11:0],4'b0000}),
	.data6x({feature_reg_2_3_1[13:0],2'b00}),
	.data7x(f80),
	.data8x(f81),
	.data9x({feature_reg_3_2_1[13:0],2'b00}),
	.data10x(feature_reg_3_3_1),
	.data11x(f82),
	.data12x({feature_reg_4_1_1[13:0],2'b00}),
	.data13x(f83),
	.data14x(f84),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_2_2)
);

wire [15:0] f85, f86, f87, f88, f89, f90, f91, f92;
assign f85 = -{feature_reg_1_1_1[12:0],3'b000};
assign f86 = -{feature_reg_1_4_1[14:0],1'b0};
assign f87 = -{feature_reg_2_1_1[13:0],2'b00};
assign f88 = -feature_reg_2_4_1;
assign f89 = -{feature_reg_3_2_1[12:0],3'b000};
assign f90 = -{feature_reg_3_3_1[14:0],1'b0};
assign f91 = -{feature_reg_4_2_1[13:0],2'b00};
assign f92 = -feature_reg_4_3_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA32 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_2_1[12:0],3'b000}),
	.data1x(f85),
	.data2x({feature_reg_1_3_1[14:0],1'b0}),
	.data3x(f86),
	.data4x(f87),
	.data5x({feature_reg_2_2_1[13:0],2'b00}),
	.data6x(feature_reg_2_3_1),
	.data7x(f88),
	.data8x({feature_reg_3_1_1[12:0],3'b000}),
	.data9x(f89),
	.data10x(f90),
	.data11x({feature_reg_3_4_1[14:0],1'b0}),
	.data12x({feature_reg_4_1_1[13:0],2'b00}),
	.data13x(f91),
	.data14x(f92),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_3_2)
);

wire [15:0] f93, f94, f95, f96, f97, f98, f99, f100;
assign f93 = -{feature_reg_1_2_1[12:0],3'b000};
assign f94 = -{feature_reg_1_3_1[14:0],1'b0};
assign f95 = -{feature_reg_2_1_1[13:0],2'b00};
assign f96 = -feature_reg_2_4_1;
assign f97 = -{feature_reg_3_1_1[12:0],3'b000};
assign f98 = -{feature_reg_3_4_1[14:0],1'b0};
assign f99 = -{feature_reg_4_2_1[13:0],2'b00};
assign f100 = -feature_reg_4_3_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA42 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[12:0],3'b000}),
	.data1x(f93),
	.data2x(f94),
	.data3x({feature_reg_1_4_1[14:0],1'b0}),
	.data4x(f95),
	.data5x({feature_reg_2_2_1[13:0],2'b00}),
	.data6x(feature_reg_2_3_1),
	.data7x(f96),
	.data8x(f97),
	.data9x({feature_reg_3_2_1[12:0],3'b000}),
	.data10x({feature_reg_3_3_1[14:0],1'b0}),
	.data11x(f98),
	.data12x({feature_reg_4_1_1[13:0],2'b00}),
	.data13x(f99),
	.data14x(f100),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_4_2)
);

wire [15:0] f101, f102, f103, f104;
assign f101 = -{feature_reg_1_2_1[11:0],4'b0000};
assign f102 = -{feature_reg_1_3_1[13:0],2'b00};
assign f103 = -{feature_reg_5_2_1[13:0],2'b00};
assign f104 = -feature_reg_5_3_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA52 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[11:0],4'b0000}),
	.data1x(f101),
	.data2x(f102),
	.data3x({feature_reg_1_4_1[13:0],2'b00}),
	.data4x(dsp_out_3_4),
	.data5x(dsp_out_3_5),
	.data6x(dsp_out_3_6),
	.data7x(dsp_out_3_7),
	.data8x({feature_reg_5_1_1[13:0],2'b00}),
	.data9x(f103),
	.data10x(f104),
	.data11x(feature_reg_5_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_5_2)
);

wire [15:0] f105, f106, f107, f108;
assign f105 = -{feature_reg_0_2_1[13:0],2'b00};
assign f106 = -{feature_reg_0_1_1[12:0],3'b000};
assign f107 = -{feature_reg_4_1_1[14:0],1'b0};
assign f108 = -feature_reg_4_2_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA03 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_0_3_1[12:0],3'b000}),
	.data1x(f105),
	.data2x(f106),
	.data3x({feature_reg_0_4_1[13:0],2'b00}),
	.data4x(dsp_out_4_0),
	.data5x(dsp_out_4_1),
	.data6x(dsp_out_4_2),
	.data7x(dsp_out_4_3),
	.data8x(f107),
	.data9x(f108),
	.data10x({feature_reg_4_3_1[14:0],1'b0}),
	.data11x(feature_reg_4_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_0_3)
);

wire [15:0] f109, f110, f111, f112, f113, f114, f115, f116;
assign f109 = -{feature_reg_1_3_1[12:0],3'b000};
assign f110 = -{feature_reg_1_4_1[13:0],2'b00};
assign f111 = -{feature_reg_2_3_1[12:0],3'b000};
assign f112 = -{feature_reg_2_4_1[13:0],2'b00};
assign f113 = -{feature_reg_3_1_1[14:0],1'b0};
assign f114 = -feature_reg_3_2_1;
assign f115 = -{feature_reg_4_1_1[14:0],1'b0};
assign f116 = -feature_reg_4_2_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA13 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[12:0],3'b000}),
	.data1x({feature_reg_1_2_1[13:0],2'b00}),
	.data2x(f109),
	.data3x(f110),
	.data4x({feature_reg_2_1_1[12:0],3'b000}),
	.data5x({feature_reg_2_2_1[13:0],2'b00}),
	.data6x(f111),
	.data7x(f112),
	.data8x(f113),
	.data9x(f114),
	.data10x({feature_reg_3_3_1[14:0],1'b0}),
	.data11x(feature_reg_3_4_1),
	.data12x(f115),
	.data13x(f116),
	.data14x({feature_reg_4_3_1[14:0],1'b0}),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_1_3)
);

wire [15:0] f117, f118, f119, f120, f121, f122, f123, f124;
assign f117 = -{feature_reg_1_2_1[13:0],2'b00};
assign f118 = -{feature_reg_1_1_1[12:0],3'b000};
assign f119 = -{feature_reg_2_3_1[12:0],3'b000};
assign f120 = -{feature_reg_2_4_1[13:0],2'b00};
assign f121 = -{feature_reg_3_3_1[14:0],1'b0};
assign f122 = -feature_reg_3_4_1;
assign f123 = -{feature_reg_4_1_1[14:0],1'b0};
assign f124 = -feature_reg_4_2_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA23 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_3_1[12:0],3'b000}), 
	.data1x(f117),
	.data2x(f118),
	.data3x({feature_reg_1_4_1[13:0],2'b00}),
	.data4x({feature_reg_2_1_1[12:0],3'b000}),
	.data5x({feature_reg_2_2_1[13:0],2'b00}),
	.data6x(f119),
	.data7x(f120),
	.data8x({feature_reg_3_1_1[14:0],1'b0}),
	.data9x(feature_reg_3_2_1),
	.data10x(f121),
	.data11x(f122),
	.data12x(f123),
	.data13x(f124),
	.data14x({feature_reg_4_3_1[14:0],1'b0}),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_2_3)
);

wire [15:0] f125, f126, f127, f128, f129, f130, f131, f132;
assign f125 = -{feature_reg_1_3_1[13:0],2'b00};
assign f126 = -{feature_reg_1_4_1[14:0],1'b0};
assign f127 = -{feature_reg_2_3_1[14:0],1'b0};
assign f128 = -feature_reg_2_4_1;
assign f129 = -{feature_reg_3_1_1[13:0],2'b00};
assign f130 = -{feature_reg_3_2_1[14:0],1'b0};
assign f131 = -{feature_reg_4_1_1[14:0],1'b0};
assign f132 = -feature_reg_4_2_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA33 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[13:0],2'b00}),
	.data1x({feature_reg_1_2_1[14:0],1'b0}),
	.data2x(f125),
	.data3x(f126),
	.data4x({feature_reg_2_1_1[14:0],1'b0}),
	.data5x(feature_reg_2_2_1),
	.data6x(f127),
	.data7x(f128),
	.data8x(f129),
	.data9x(f130),
	.data10x({feature_reg_3_3_1[13:0],2'b00}),
	.data11x({feature_reg_3_4_1[14:0],1'b0}),
	.data12x(f131),
	.data13x(f132),
	.data14x({feature_reg_4_3_1[14:0],1'b0}),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_3_3)
);

wire [15:0] f133, f134, f135, f136, f137, f138, f139, f140;
assign f133 = -{feature_reg_1_2_1[14:0],1'b0};
assign f134 = -{feature_reg_1_1_1[13:0],2'b00};
assign f135 = -{feature_reg_2_3_1[14:0],1'b0};
assign f136 = -feature_reg_2_4_1;
assign f137 = -{feature_reg_3_3_1[13:0],2'b00};
assign f138 = -{feature_reg_3_4_1[14:0],1'b0};
assign f139 = -{feature_reg_4_1_1[14:0],1'b0};
assign f140 = -feature_reg_4_2_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA43 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_3_1[13:0],2'b00}),
	.data1x(f133),
	.data2x(f134),
	.data3x({feature_reg_1_4_1[14:0],1'b0}),
	.data4x({feature_reg_2_1_1[14:0],1'b0}),
	.data5x(feature_reg_2_2_1),
	.data6x(f135),
	.data7x(f136),
	.data8x({feature_reg_3_1_1[13:0],2'b00}),
	.data9x({feature_reg_3_2_1[14:0],1'b0}),
	.data10x(f137),
	.data11x(f138),
	.data12x(f139),
	.data13x(f140),
	.data14x({feature_reg_4_3_1[14:0],1'b0}),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_4_3)
);

wire [15:0] f141, f142, f143, f144;
assign f141 = -{feature_reg_1_2_1[13:0],2'b00};
assign f142 = -{feature_reg_1_1_1[12:0],3'b000};
assign f143 = -{feature_reg_5_1_1[14:0],1'b0};
assign f144 = -feature_reg_5_2_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA53 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_3_1[12:0],3'b000}),
	.data1x(f141),
	.data2x(f142),
	.data3x({feature_reg_1_4_1[13:0],2'b00}),
	.data4x(dsp_out_4_4),
	.data5x(dsp_out_4_5),
	.data6x(dsp_out_4_6),
	.data7x(dsp_out_4_7),
	.data8x(f143),
	.data9x(f144),
	.data10x({feature_reg_5_3_1[14:0],1'b0}),
	.data11x(feature_reg_5_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_5_3)
);

wire [15:0] f145, f146, f147, f148;
assign f145 = -{feature_reg_0_2_1[13:0],2'b00};
assign f146 = -{feature_reg_0_3_1[12:0],3'b000};
assign f147 = -feature_reg_4_2_1;
assign f148 = -{feature_reg_4_3_1[14:0],1'b0};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA04 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_0_1_1[12:0],3'b000}),
	.data1x(f145),
	.data2x(f146),
	.data3x({feature_reg_0_4_1[13:0],2'b00}),
	.data4x(dsp_out_5_0),
	.data5x(dsp_out_5_1),
	.data6x(dsp_out_5_2),
	.data7x(dsp_out_5_3),
	.data8x({feature_reg_4_1_1[14:0],1'b0}),
	.data9x(f147),
	.data10x(f148),
	.data11x(feature_reg_4_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_0_4)
);

wire [15:0] f149, f150, f151, f152, f153, f154, f155, f156;
assign f149 = -{feature_reg_1_1_1[12:0],3'b000};
assign f150 = -{feature_reg_1_4_1[13:0],2'b00};
assign f151 = -{feature_reg_2_1_1[12:0],3'b000};
assign f152 = -{feature_reg_2_4_1[13:0],2'b00};
assign f153 = -feature_reg_3_2_1;
assign f154 = -{feature_reg_3_3_1[14:0],1'b0};
assign f155 = -feature_reg_4_2_1;
assign f156 = -{feature_reg_4_3_1[14:0],1'b0};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA14 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_2_1[13:0],2'b00}),
	.data1x(f149),
	.data2x({feature_reg_1_3_1[12:0],3'b000}),
	.data3x(f150),
	.data4x(f151),
	.data5x({feature_reg_2_2_1[13:0],2'b00}),
	.data6x({feature_reg_2_3_1[12:0],3'b000}),
	.data7x(f152),
	.data8x({feature_reg_3_1_1[14:0],1'b0}),
	.data9x(f153),
	.data10x(f154),
	.data11x(feature_reg_3_4_1),
	.data12x({feature_reg_4_1_1[14:0],1'b0}),
	.data13x(f155),
	.data14x(f156),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_1_4)
);

wire [15:0] f157, f158, f159, f160, f161, f162, f163, f164;
assign f157 = -{feature_reg_1_2_1[13:0],2'b00};
assign f158 = -{feature_reg_1_3_1[12:0],3'b000};
assign f159 = -{feature_reg_2_1_1[12:0],3'b000};
assign f160 = -{feature_reg_2_4_1[13:0],2'b00};
assign f161 = -{feature_reg_3_1_1[14:0],1'b0};
assign f162 = -feature_reg_3_4_1;
assign f163 = -feature_reg_4_2_1;
assign f164 = -{feature_reg_4_3_1[14:0],1'b0};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA24 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[12:0],3'b000}),
	.data1x(f157),
	.data2x(f158),
	.data3x({feature_reg_1_4_1[13:0],2'b00}),
	.data4x(f159),
	.data5x({feature_reg_2_2_1[13:0],2'b00}),
	.data6x({feature_reg_2_3_1[12:0],3'b000}),
	.data7x(f160),
	.data8x(f161),
	.data9x(feature_reg_3_2_1),
	.data10x({feature_reg_3_3_1[14:0],1'b0}),
	.data11x(f162),
	.data12x({feature_reg_4_1_1[14:0],1'b0}),
	.data13x(f163),
	.data14x(f164),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_2_4)
);

wire [15:0] f165, f166, f167, f168, f169, f170, f171, f172;
assign f165 = -{feature_reg_1_1_1[13:0],2'b00};
assign f166 = -{feature_reg_1_4_1[14:0],1'b0};
assign f167 = -{feature_reg_2_1_1[14:0],1'b0};
assign f168 = -feature_reg_2_4_1;
assign f169 = -{feature_reg_3_2_1[14:0],1'b0};
assign f170 = -{feature_reg_3_3_1[13:0],2'b00};
assign f171 = -feature_reg_4_2_1;
assign f172 = -{feature_reg_4_1_1[14:0],1'b0};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA34 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_2_1[14:0],1'b0}),
	.data1x(f165),
	.data2x({feature_reg_1_3_1[13:0],2'b00}),
	.data3x(f166),
	.data4x(f167),
	.data5x(feature_reg_2_2_1),
	.data6x({feature_reg_2_3_1[14:0],1'b0}),
	.data7x(f168),
	.data8x({feature_reg_3_1_1[13:0],2'b00}),
	.data9x(f169),
	.data10x(f170),
	.data11x({feature_reg_3_4_1[14:0],1'b0}),
	.data12x({feature_reg_4_1_1[14:0],1'b0}),
	.data13x(f171),
	.data14x(f172),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_3_4)
);

wire [15:0] f173, f174, f175, f176, f177, f178, f179, f180;
assign f173 = -{feature_reg_1_2_1[14:0],1'b0};
assign f174 = -{feature_reg_1_3_1[13:0],2'b00};
assign f175 = -{feature_reg_2_1_1[14:0],1'b0};
assign f176 = -feature_reg_2_4_1;
assign f177 = -{feature_reg_3_1_1[13:0],2'b00};
assign f178 = -{feature_reg_3_4_1[14:0],1'b0};
assign f179 = -feature_reg_4_2_1;
assign f180 = -{feature_reg_4_3_1[14:0],1'b0};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA44 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[13:0],2'b00}),
	.data1x(f173),
	.data2x(f174),
	.data3x({feature_reg_1_4_1[14:0],1'b0}),
	.data4x(f175),
	.data5x(feature_reg_2_2_1),
	.data6x({feature_reg_2_3_1[14:0],1'b0}),
	.data7x(f176),
	.data8x(f177),
	.data9x({feature_reg_3_2_1[14:0],1'b0}),
	.data10x({feature_reg_3_3_1[13:0],2'b00}),
	.data11x(f178),
	.data12x({feature_reg_4_1_1[14:0],1'b0}),
	.data13x(f179),
	.data14x(f180),
	.data15x(feature_reg_4_4_1),
	.result(rslt_buffer_4_4)
);

wire [15:0] f181, f182, f183, f184;
assign f181 = -{feature_reg_1_2_1[13:0],2'b00};
assign f182 = -{feature_reg_1_3_1[12:0],3'b000};
assign f183 = -feature_reg_5_2_1;
assign f184 = -{feature_reg_5_3_1[14:0],1'b0};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA54 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[12:0],3'b000}),
	.data1x(f181),
	.data2x(f182),
	.data3x({feature_reg_1_4_1[13:0],2'b00}),
	.data4x(dsp_out_5_4),
	.data5x(dsp_out_5_5),
	.data6x(dsp_out_5_6),
	.data7x(dsp_out_5_7),
	.data8x({feature_reg_5_1_1[14:0],1'b0}),
	.data9x(f183),
	.data10x(f184),
	.data11x(feature_reg_5_4_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_5_4)
);

winograd_adder_16_20_4 winograd_adder_16_20_4_WA05 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_0_1_1[11:0],4'b0000}),
	.data1x(dsp_out_6_0),
	.data2x({feature_reg_0_5_1[13:0],2'b00}),
	.data3x(dsp_out_6_1),
	.data4x(dsp_out_6_2),
	.data5x(dsp_out_6_3),
	.data6x({feature_reg_4_1_1[13:0],2'b00}),
	.data7x(dsp_out_6_4),
	.data8x(feature_reg_4_5_1),
	.data9x(0),
	.data10x(0),
	.data11x(0),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_0_5)
);

wire [15:0] f185, f186, f187, f188;
assign f185 = -{feature_reg_1_1_1[11:0],4'b0000};
assign f186 = -{feature_reg_1_5_1[13:0],2'b00};
assign f187 = -{feature_reg_2_1_1[11:0],4'b0000};
assign f188 = -{feature_reg_2_5_1[13:0],2'b00};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA15 (
	.clock(clk),
	.clken(calculate_2),
	.data0x(dsp_out_6_5),
	.data1x(f185),
	.data2x(f186),
	.data3x(f187),
	.data4x(dsp_out_6_6),
	.data5x(f188),
	.data6x({feature_reg_3_1_1[13:0],2'b00}),
	.data7x(dsp_out_6_7),
	.data8x(feature_reg_3_5_1),
	.data9x({feature_reg_4_1_1[13:0],2'b00}),
	.data10x(dsp_out_6_4),
	.data11x(feature_reg_4_5_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_1_5)
);

wire [15:0] f189, f190, f191, f192, f193, f194;
assign f189 = -dsp_out_6_5;
assign f190 = -{feature_reg_2_1_1[11:0],4'b0000};
assign f191 = -{feature_reg_2_5_1[13:0],2'b00};
assign f192 = -{feature_reg_3_1_1[13:0],2'b00};
assign f193 = -dsp_out_6_7;
assign f194 = -feature_reg_3_5_1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA25 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[11:0],4'b0000}),
	.data1x(f189),
	.data2x({feature_reg_1_5_1[13:0],2'b00}),
	.data3x(f190),
	.data4x(dsp_out_6_6),
	.data5x(f191),
	.data6x(f192),
	.data7x(f193),
	.data8x(f194),
	.data9x({feature_reg_4_1_1[13:0],2'b00}),
	.data10x(dsp_out_6_4),
	.data11x(feature_reg_4_5_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_2_5)
);

wire [15:0] f195, f196, f197, f198, f199, f200, f201;
assign f195 = dsp_out_6_5 >>> 1;
assign f196 = -{feature_reg_1_1_1[12:0],3'b000};
assign f197 = -{feature_reg_1_5_1[14:0],1'b0};
assign f198 = -{feature_reg_2_1_1[13:0],2'b00};
assign f199 = dsp_out_6_6 >>> 2;
assign f200 = -feature_reg_2_5_1;
assign f201 = dsp_out_6_7 <<< 1;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA35 (
	.clock(clk),
	.clken(calculate_2),
	.data0x(f195),
	.data1x(f196),
	.data2x(f197),
	.data3x(f198),
	.data4x(f199),
	.data5x(f200),
	.data6x({feature_reg_3_1_1[12:0],3'b000}),
	.data7x(f201),
	.data8x({feature_reg_3_5_1[14:0],1'b0}),
	.data9x({feature_reg_4_1_1[13:0],2'b00}),
	.data10x(dsp_out_6_4),
	.data11x(feature_reg_4_5_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_3_5)
);

wire [15:0] f202, f203, f204, f205, f206, f207, f208;
assign f202 = -(dsp_out_6_5 >>> 1);
assign f203 = -{feature_reg_2_1_1[13:0],2'b00};
assign f204 = dsp_out_6_6 >>> 2;
assign f205 = -feature_reg_2_5_1;
assign f206 = -{feature_reg_3_1_1[12:0],3'b000};
assign f207 = -(dsp_out_6_7 <<< 1);
assign f208 = -{feature_reg_3_5_1[14:0],1'b0};
winograd_adder_16_20_4 winograd_adder_16_20_4_WA45 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[12:0],3'b000}), 
	.data1x(f202),
	.data2x({feature_reg_1_5_1[14:0],1'b0}),
	.data3x(f203),
	.data4x(f204),
	.data5x(f205),
	.data6x(f206),
	.data7x(f207),
	.data8x(f208),
	.data9x({feature_reg_4_1_1[13:0],2'b00}),
	.data10x(dsp_out_6_4),
	.data11x(feature_reg_4_5_1),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_4_5)
);

wire [15:0] f209;
assign f209 = -dsp_out_6_5;
winograd_adder_16_20_4 winograd_adder_16_20_4_WA55 (
	.clock(clk),
	.clken(calculate_2),
	.data0x({feature_reg_1_1_1[11:0],4'b0000}),
	.data1x(f209),
	.data2x({feature_reg_1_5_1[13:0],2'b00}),
	.data3x(dsp_out_6_8),
	.data4x(dsp_out_6_9),
	.data5x(dsp_out_6_10),
	.data6x({feature_reg_5_1_1[13:0],2'b00}),
	.data7x(dsp_out_6_11),
	.data8x(feature_reg_5_5_1),
	.data9x(0),
	.data10x(0),
	.data11x(0),
	.data12x(0),
	.data13x(0),
	.data14x(0),
	.data15x(0),
	.result(rslt_buffer_5_5)
);

assign o_feature_0 = output_buffer_0_0[15:0];
assign o_feature_1 = output_buffer_1_0[15:0];
assign o_feature_2 = output_buffer_2_0[15:0];
assign o_feature_3 = output_buffer_3_0[15:0];
assign o_feature_4 = output_buffer_4_0[15:0];
assign o_feature_5 = output_buffer_5_0[15:0];
assign o_valid = valid_9;

endmodule

module stream_buffer_5_1 (
	input clk,
	input i_reset,
	input i_wen0,
	input i_wen1,
	input [15:0] i_ddr,
	input [15:0] i_pool,
	input i_eltwise_sel,
	input [15:0] i_eltwise,
	input [14:0] i_waddr,
	output [15:0] o_feature_0,
	output [15:0] o_feature_1,
	output o_done
);

reg [14:0] base_addr;
reg [14:0] offset;
reg [14:0] base_addr_b1;
reg [14:0] offset_b1;
reg [1:0] L_counter;
reg [1:0] C_counter;
reg [1:0] W_counter;
reg [1:0] L_counter_b1;
reg [1:0] C_counter_b1;
reg [1:0] W_counter_b1;
reg done, done_1, done_2, done_3;
reg valid, valid_1, valid_2;
wire [15:0] feature_out_b0;
wire [15:0] feature_out_b1;

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr <= 0;
		offset <= 0;
		C_counter <= 0;
		L_counter <= 0;
		W_counter <= 0;
	end else if (done == 0) begin
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			base_addr <= 0;
			C_counter <= 0;
			L_counter <= 0;
			W_counter <= 0;
			offset <= 0;
		end else if((C_counter == 1) && (L_counter == 2)) begin
			base_addr <= base_addr + 5;
			W_counter <= W_counter + 1'b1;
			C_counter <= 0;
			L_counter <= 0;
			offset <= 0;
		end else if(L_counter == 2) begin
			base_addr <= base_addr + 1'b1;
			C_counter <= C_counter + 1'b1;
			L_counter <= 0;
			offset <= 0;
		end else begin
			offset <= offset + 2;
			L_counter <= L_counter + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr_b1 <= 0;
		offset_b1 <= 0;
		C_counter_b1 <= 0;
		L_counter_b1 <= 0;
		W_counter_b1 <= 0;
	end else if (done == 0) begin
		if((W_counter_b1 == 1443) && (C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= 0;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			W_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if((C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= base_addr_b1 + 5;
			W_counter_b1 <= W_counter_b1 + 1'b1;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if(L_counter_b1 == 2) begin
			base_addr_b1 <= base_addr_b1 + 1'b1;
			C_counter_b1 <= C_counter_b1 + 1'b1;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else begin
			offset_b1 <= offset_b1 + 2;
			L_counter_b1 <= L_counter_b1 + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if(i_reset == 1'b1)begin
		done <= 0;
		done_1 <= 0;
		done_2 <= 0;
		done_3 <= 0;
		valid <= 0;
		valid_1 <= 0;
		valid_2 <= 0;
	end else begin
		valid <= 1;
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			done <= 1;
		end
		done_1 <= done;
		done_2 <= done_1;
		done_3 <= done_2;
		valid_1 <= valid;
		valid_2 <= valid_1;
	end
end

reg [14:0] b0_waddr, b0_raddr, b1_raddr;
always @ (*) begin
	b0_waddr <= base_addr+offset;
	b0_raddr <= base_addr+offset;
	b1_raddr <= base_addr_b1+offset_b1;
end
buffer_16_24200_buffer_init_15 buffer_16_24200_buffer_init_15_B0 (
	.clk(clk),
	.wen(i_wen0),
	.waddr(b0_waddr),
	.wdata(i_ddr),
	.raddr(b0_raddr),
	.rdata(feature_out_b0)
);

reg [15:0] B1_wdata;
always @ (*) begin
	if (i_eltwise_sel) begin
		B1_wdata <= i_eltwise;
	end else begin
		B1_wdata <= i_pool;
	end
end

buffer_16_24200_buffer_init_15 buffer_16_24200_buffer_init_15_B1 (
	.clk(clk),
	.wen(i_wen1),
	.waddr(i_waddr),
	.wdata(B1_wdata),
	.raddr(b1_raddr),
	.rdata(feature_out_b1)
);

assign o_done = valid_2 && (~done_3);
assign o_feature_0 = feature_out_b0;
assign o_feature_1 = feature_out_b1;

endmodule

module buffer_16_24200_buffer_init_15 (
	input clk,
	input wen,
	input [14:0] waddr,
	input [15:0] wdata,
	input [14:0] raddr,
	output [15:0] rdata
);

reg [14:0] raddr_reg;
reg [15:0] rdata_reg;
reg [15:0] pipeline_reg_0;
wire [15:0] rd_dummy_signal;
wire [15:0] wr_dummy_signal;
wire [15:0] rdata_wire;
assign rd_dummy_signal = 0;

always @(posedge clk) begin
	rdata_reg <= rdata_wire;
	raddr_reg <= raddr;
	pipeline_reg_0 <= rdata_reg;
end

dual_port_ram u_dual_port_ram(
	.addr1(waddr),
	.we1(wen),
	.data1(wdata),
	.out1(wr_dummy_signal),
	.addr2(raddr_reg),
	.we2(1'b0),
	.data2(rd_dummy_signal),
	.out2(rdata_wire),
	.clk(clk)
);
assign rdata = pipeline_reg_0;

endmodule

module stream_buffer_5_0 (
	input clk,
	input i_reset,
	input i_wen0,
	input i_wen1,
	input [15:0] i_ddr,
	input [15:0] i_pool,
	input i_eltwise_sel,
	input [15:0] i_eltwise,
	input [14:0] i_waddr,
	output [15:0] o_feature_0,
	output [15:0] o_feature_1,
	output o_done
);

reg [14:0] base_addr;
reg [14:0] offset;
reg [14:0] base_addr_b1;
reg [14:0] offset_b1;
reg [1:0] L_counter;
reg [1:0] C_counter;
reg [1:0] W_counter;
reg [1:0] L_counter_b1;
reg [1:0] C_counter_b1;
reg [1:0] W_counter_b1;
reg done, done_1, done_2, done_3;
reg valid, valid_1, valid_2;
wire [15:0] feature_out_b0;
wire [15:0] feature_out_b1;

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr <= 0;
		offset <= 0;
		C_counter <= 0;
		L_counter <= 0;
		W_counter <= 0;
	end else if (done == 0) begin
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			base_addr <= 0;
			C_counter <= 0;
			L_counter <= 0;
			W_counter <= 0;
			offset <= 0;
		end else if((C_counter == 1) && (L_counter == 2)) begin
			base_addr <= base_addr + 5;
			W_counter <= W_counter + 1'b1;
			C_counter <= 0;
			L_counter <= 0;
			offset <= 0;
		end else if(L_counter == 2) begin
			base_addr <= base_addr + 1'b1;
			C_counter <= C_counter + 1'b1;
			L_counter <= 0;
			offset <= 0;
		end else begin
			offset <= offset + 2;
			L_counter <= L_counter + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr_b1 <= 0;
		offset_b1 <= 0;
		C_counter_b1 <= 0;
		L_counter_b1 <= 0;
		W_counter_b1 <= 0;
	end else if (done == 0) begin
		if((W_counter_b1 == 1443) && (C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= 0;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			W_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if((C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= base_addr_b1 + 5;
			W_counter_b1 <= W_counter_b1 + 1'b1;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if(L_counter_b1 == 2) begin
			base_addr_b1 <= base_addr_b1 + 1'b1;
			C_counter_b1 <= C_counter_b1 + 1'b1;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else begin
			offset_b1 <= offset_b1 + 2;
			L_counter_b1 <= L_counter_b1 + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if(i_reset == 1'b1)begin
		done <= 0;
		done_1 <= 0;
		done_2 <= 0;
		done_3 <= 0;
		valid <= 0;
		valid_1 <= 0;
		valid_2 <= 0;
	end else begin
		valid <= 1;
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			done <= 1;
		end
		done_1 <= done;
		done_2 <= done_1;
		done_3 <= done_2;
		valid_1 <= valid;
		valid_2 <= valid_1;
	end
end

reg [14:0] b0_waddr, b0_raddr, b1_raddr;
always @ (*) begin
	b0_waddr <= base_addr+offset;
	b0_raddr <= base_addr+offset;
	b1_raddr <= base_addr_b1+offset_b1;
end
buffer_16_24200_buffer_init_05 buffer_16_24200_buffer_init_05_B0 (
	.clk(clk),
	.wen(i_wen0),
	.waddr(b0_waddr),
	.wdata(i_ddr),
	.raddr(b0_raddr),
	.rdata(feature_out_b0)
);

reg [15:0] B1_wdata;
always @ (*) begin
	if (i_eltwise_sel) begin
		B1_wdata <= i_eltwise;
	end else begin
		B1_wdata <= i_pool;
	end
end

buffer_16_24200_buffer_init_05 buffer_16_24200_buffer_init_05_B1 (
	.clk(clk),
	.wen(i_wen1),
	.waddr(i_waddr),
	.wdata(B1_wdata),
	.raddr(b1_raddr),
	.rdata(feature_out_b1)
);

assign o_done = valid_2 && (~done_3);
assign o_feature_0 = feature_out_b0;
assign o_feature_1 = feature_out_b1;

endmodule

module buffer_16_24200_buffer_init_05 (
	input clk,
	input wen,
	input [14:0] waddr,
	input [15:0] wdata,
	input [14:0] raddr,
	output [15:0] rdata
);

reg [14:0] raddr_reg;
reg [15:0] rdata_reg;
reg [15:0] pipeline_reg_0;
wire [15:0] rd_dummy_signal;
wire [15:0] wr_dummy_signal;
wire [15:0] rdata_wire;
assign rd_dummy_signal = 0;

always @(posedge clk) begin
	rdata_reg <= rdata_wire;
	raddr_reg <= raddr;
	pipeline_reg_0 <= rdata_reg;
end

dual_port_ram u_dual_port_ram(
	.addr1(waddr),
	.we1(wen),
	.data1(wdata),
	.out1(wr_dummy_signal),
	.addr2(raddr_reg),
	.we2(1'b0),
	.data2(rd_dummy_signal),
	.out2(rdata_wire),
	.clk(clk)
);
assign rdata = pipeline_reg_0;

endmodule

module processing_element (
	input clk,
	input i_reset,
	input i_valid,
	input [15:0] i_features_0_0,
	output [15:0] o_features_0_0,
	input [15:0] i_features_0_1,
	output [15:0] o_features_0_1,
	input [15:0] i_features_0_2,
	output [15:0] o_features_0_2,
	input [15:0] i_features_0_3,
	output [15:0] o_features_0_3,
	input [15:0] i_features_0_4,
	output [15:0] o_features_0_4,
	input [15:0] i_features_0_5,
	output [15:0] o_features_0_5,
	input [15:0] i_features_1_0,
	output [15:0] o_features_1_0,
	input [15:0] i_features_1_1,
	output [15:0] o_features_1_1,
	input [15:0] i_features_1_2,
	output [15:0] o_features_1_2,
	input [15:0] i_features_1_3,
	output [15:0] o_features_1_3,
	input [15:0] i_features_1_4,
	output [15:0] o_features_1_4,
	input [15:0] i_features_1_5,
	output [15:0] o_features_1_5,
	output [29:0] o_result_0,
	output [29:0] o_result_1,
	output [29:0] o_result_2,
	output [29:0] o_result_3,
	output [29:0] o_result_4,
	output [29:0] o_result_5,
	output o_valid,
	output o_next_reset,
	output o_next_valid
);

wire [23:0] DP_res_0;
reg [15:0] if_reg_0_0;
wire [7:0] weights_0_0;
reg [15:0] if_reg_0_1;
wire [7:0] weights_0_1;
wire [23:0] DP_res_1;
reg [15:0] if_reg_1_0;
wire [7:0] weights_1_0;
reg [15:0] if_reg_1_1;
wire [7:0] weights_1_1;
wire [23:0] DP_res_2;
reg [15:0] if_reg_2_0;
wire [7:0] weights_2_0;
reg [15:0] if_reg_2_1;
wire [7:0] weights_2_1;
wire [23:0] DP_res_3;
reg [15:0] if_reg_3_0;
wire [7:0] weights_3_0;
reg [15:0] if_reg_3_1;
wire [7:0] weights_3_1;
wire [23:0] DP_res_4;
reg [15:0] if_reg_4_0;
wire [7:0] weights_4_0;
reg [15:0] if_reg_4_1;
wire [7:0] weights_4_1;
wire [23:0] DP_res_5;
reg [15:0] if_reg_5_0;
wire [7:0] weights_5_0;
reg [15:0] if_reg_5_1;
wire [7:0] weights_5_1;

reg [10:0] base_addr;
reg [10:0] offset;
reg [1:0] L_counter;
reg [1:0] C_counter;
reg [15:0] T_counter;

reg reset_1, reset_2, reset_3, next_reset, next_reset_2;
reg done, done_1, done_2, done_3, done_4, done_5, done_6;

wire [15:0] features_0_0;
wire [15:0] features_0_1;
wire [15:0] features_1_0;
wire [15:0] features_1_1;
wire [15:0] features_2_0;
wire [15:0] features_2_1;
wire [15:0] features_3_0;
wire [15:0] features_3_1;
wire [15:0] features_4_0;
wire [15:0] features_4_1;
wire [15:0] features_5_0;
wire [15:0] features_5_1;
reg valid_0;
reg valid_1;
reg valid_2;
reg valid_3;
reg valid_4;
reg valid_5;
reg valid_6;
reg valid_7;
reg valid_8;
reg valid_9;
reg valid_10;
reg valid_11;
reg next_valid;

always @ (posedge clk) begin
	reset_1 <= ~(i_valid || valid_11);
	reset_2 <= reset_1;
	reset_3 <= reset_2;
	next_reset <= i_reset;
	next_reset_2 <= next_reset;
	if (i_reset == 1'b0) begin
		if_reg_0_0 <= features_0_0;
		if_reg_0_1 <= features_0_1;
		if_reg_1_0 <= features_1_0;
		if_reg_1_1 <= features_1_1;
		if_reg_2_0 <= features_2_0;
		if_reg_2_1 <= features_2_1;
		if_reg_3_0 <= features_3_0;
		if_reg_3_1 <= features_3_1;
		if_reg_4_0 <= features_4_0;
		if_reg_4_1 <= features_4_1;
		if_reg_5_0 <= features_5_0;
		if_reg_5_1 <= features_5_1;
	end
end

always @ (posedge clk) begin
	next_valid <= i_valid;
	if (i_reset) begin
		valid_0 <= 0;
		valid_1 <= 0;
		valid_2 <= 0;
		valid_3 <= 0;
		valid_4 <= 0;
		valid_5 <= 0;
		valid_6 <= 0;
		valid_7 <= 0;
		valid_8 <= 0;
		valid_9 <= 0;
		valid_10 <= 0;
		valid_11 <= 0;
	end else if ((i_valid == 1'b0) && (valid_11 == 1'b0)) begin
		base_addr <= 0;
		offset <= 0;
		C_counter <= 0;
		L_counter <= 0;
		T_counter <= 0;
		done <= 0;
		done_1 <= 0;
		done_2 <= 0;
		done_3 <= 0;
		done_4 <= 0;
		done_5 <= 0;
		done_6 <= 0;
	end else if (i_valid || valid_11) begin
		valid_0 <= i_valid;
		valid_1 <= valid_0;
		valid_2 <= valid_1;
		valid_3 <= valid_2;
		valid_4 <= valid_3;
		valid_5 <= valid_4;
		valid_6 <= valid_5;
		valid_7 <= valid_6;
		valid_8 <= valid_7;
		valid_9 <= valid_8;
		valid_10 <= valid_9;
		valid_11 <= valid_10;
		if (T_counter <= 1809025) begin
			done_1 <= done;
			done_2 <= done_1;
			done_3 <= done_2;
			done_4 <= done_3;
			done_5 <= done_4;
			done_6 <= done_5;
			if((C_counter == 0) && (L_counter == 2)) begin
				base_addr <= 0;
				C_counter <= 0;
				L_counter <= 0;
				offset <= 0;
				done <= 1;
				T_counter <= T_counter + 1'b1;
			end else if(L_counter == 2) begin
				base_addr <= base_addr + 1'b1;
				C_counter <= C_counter + 1'b1;
				L_counter <= 0;
				offset <= 0;
				done <= 0;
			end else begin
				offset <= offset + 1;
				L_counter <= L_counter + 1'b1;
			end
		end
	end
end

dot_product_16_8_30_2 dot_product_16_8_30_2_inst_0 (
	.clk(clk),
	.i_reset(reset_3),
	.i_features_0(features_0_0),
	.i_weights_0(weights_0_0),
	.i_features_1(features_0_1),
	.i_weights_1(weights_0_1),
	.o_result(DP_res_0)
);

dot_product_16_8_30_2 dot_product_16_8_30_2_inst_1 (
	.clk(clk),
	.i_reset(reset_3),
	.i_features_0(features_1_0),
	.i_weights_0(weights_1_0),
	.i_features_1(features_1_1),
	.i_weights_1(weights_1_1),
	.o_result(DP_res_1)
);

dot_product_16_8_30_2 dot_product_16_8_30_2_inst_2 (
	.clk(clk),
	.i_reset(reset_3),
	.i_features_0(features_2_0),
	.i_weights_0(weights_2_0),
	.i_features_1(features_2_1),
	.i_weights_1(weights_2_1),
	.o_result(DP_res_2)
);

dot_product_16_8_30_2 dot_product_16_8_30_2_inst_3 (
	.clk(clk),
	.i_reset(reset_3),
	.i_features_0(features_3_0),
	.i_weights_0(weights_3_0),
	.i_features_1(features_3_1),
	.i_weights_1(weights_3_1),
	.o_result(DP_res_3)
);

dot_product_16_8_30_2 dot_product_16_8_30_2_inst_4 (
	.clk(clk),
	.i_reset(reset_3),
	.i_features_0(features_4_0),
	.i_weights_0(weights_4_0),
	.i_features_1(features_4_1),
	.i_weights_1(weights_4_1),
	.o_result(DP_res_4)
);

dot_product_16_8_30_2 dot_product_16_8_30_2_inst_5 (
	.clk(clk),
	.i_reset(reset_3),
	.i_features_0(features_5_0),
	.i_weights_0(weights_5_0),
	.i_features_1(features_5_1),
	.i_weights_1(weights_5_1),
	.o_result(DP_res_5)
);

accumulator_24_30_3 accumulator_24_30_3_inst_0 (
	.clk(clk),
	.i_reset(reset_3),
	.i_result(DP_res_0),
	.i_dp_done(done_5),
	.o_accum(o_result_0)
);

accumulator_24_30_3 accumulator_24_30_3_inst_1 (
	.clk(clk),
	.i_reset(reset_3),
	.i_result(DP_res_1),
	.i_dp_done(done_5),
	.o_accum(o_result_1)
);

accumulator_24_30_3 accumulator_24_30_3_inst_2 (
	.clk(clk),
	.i_reset(reset_3),
	.i_result(DP_res_2),
	.i_dp_done(done_5),
	.o_accum(o_result_2)
);

accumulator_24_30_3 accumulator_24_30_3_inst_3 (
	.clk(clk),
	.i_reset(reset_3),
	.i_result(DP_res_3),
	.i_dp_done(done_5),
	.o_accum(o_result_3)
);

accumulator_24_30_3 accumulator_24_30_3_inst_4 (
	.clk(clk),
	.i_reset(reset_3),
	.i_result(DP_res_4),
	.i_dp_done(done_5),
	.o_accum(o_result_4)
);

accumulator_24_30_3 accumulator_24_30_3_inst_5 (
	.clk(clk),
	.i_reset(reset_3),
	.i_result(DP_res_5),
	.i_dp_done(done_5),
	.o_accum(o_result_5)
);

reg [10:0] weight_cache_addr;
always @ (*) begin
	weight_cache_addr <= base_addr+offset;
end
weight_cache_2048_8_0_weight_init_00 weight_cache_2048_8_0_weight_init_00_inst_0_0 (
	.clk(clk),
	.wen0(1'b0),
	.wen1(1'b0),
	.addr0(weight_cache_addr),
	.wdata0(0),
	.data0(weights_0_0),
	.addr1(weight_cache_addr),
	.wdata1(0),
	.data1(weights_0_1)
);

weight_cache_2048_8_0_weight_init_01 weight_cache_2048_8_0_weight_init_01_inst_1_0 (
	.clk(clk),
	.wen0(1'b0),
	.wen1(1'b0),
	.addr0(weight_cache_addr),
	.wdata0(0),
	.data0(weights_1_0),
	.addr1(weight_cache_addr),
	.wdata1(0),
	.data1(weights_1_1)
);

weight_cache_2048_8_0_weight_init_02 weight_cache_2048_8_0_weight_init_02_inst_2_0 (
	.clk(clk),
	.wen0(1'b0),
	.wen1(1'b0),
	.addr0(weight_cache_addr),
	.wdata0(0),
	.data0(weights_2_0),
	.addr1(weight_cache_addr),
	.wdata1(0),
	.data1(weights_2_1)
);

weight_cache_2048_8_0_weight_init_03 weight_cache_2048_8_0_weight_init_03_inst_3_0 (
	.clk(clk),
	.wen0(1'b0),
	.wen1(1'b0),
	.addr0(weight_cache_addr),
	.wdata0(0),
	.data0(weights_3_0),
	.addr1(weight_cache_addr),
	.wdata1(0),
	.data1(weights_3_1)
);

weight_cache_2048_8_0_weight_init_04 weight_cache_2048_8_0_weight_init_04_inst_4_0 (
	.clk(clk),
	.wen0(1'b0),
	.wen1(1'b0),
	.addr0(weight_cache_addr),
	.wdata0(0),
	.data0(weights_4_0),
	.addr1(weight_cache_addr),
	.wdata1(0),
	.data1(weights_4_1)
);

weight_cache_2048_8_0_weight_init_05 weight_cache_2048_8_0_weight_init_05_inst_5_0 (
	.clk(clk),
	.wen0(1'b0),
	.wen1(1'b0),
	.addr0(weight_cache_addr),
	.wdata0(0),
	.data0(weights_5_0),
	.addr1(weight_cache_addr),
	.wdata1(0),
	.data1(weights_5_1)
);

assign o_features_0_0 = if_reg_0_0;
assign features_0_0 = i_features_0_0;
assign o_features_1_0 = if_reg_0_1;
assign features_0_1 = i_features_1_0;
assign o_features_0_1 = if_reg_1_0;
assign features_1_0 = i_features_0_1;
assign o_features_1_1 = if_reg_1_1;
assign features_1_1 = i_features_1_1;
assign o_features_0_2 = if_reg_2_0;
assign features_2_0 = i_features_0_2;
assign o_features_1_2 = if_reg_2_1;
assign features_2_1 = i_features_1_2;
assign o_features_0_3 = if_reg_3_0;
assign features_3_0 = i_features_0_3;
assign o_features_1_3 = if_reg_3_1;
assign features_3_1 = i_features_1_3;
assign o_features_0_4 = if_reg_4_0;
assign features_4_0 = i_features_0_4;
assign o_features_1_4 = if_reg_4_1;
assign features_4_1 = i_features_1_4;
assign o_features_0_5 = if_reg_5_0;
assign features_5_0 = i_features_0_5;
assign o_features_1_5 = if_reg_5_1;
assign features_5_1 = i_features_1_5;

assign o_valid = done_6;
assign o_next_reset = next_reset;
assign o_next_valid = next_valid;

endmodule

module dot_product_16_8_30_2 (
	input clk,
	input i_reset,
	input [15:0] i_features_0,
	input [7:0] i_weights_0,
	input [15:0] i_features_1,
	input [7:0] i_weights_1,
	output [23:0] o_result
);

wire [63:0] chains_0;
wire [23:0] res;
reg [15:0] f_pipeline_0_0;
reg [7:0] w_pipeline_0_0;
reg [15:0] f_pipeline_1_0;
reg [7:0] w_pipeline_1_0;
reg r_pipeline_0;

always @ (posedge clk) begin
	r_pipeline_0 <= i_reset;
	if(i_reset == 1'b1) begin
		f_pipeline_0_0 <= 0;
		w_pipeline_0_0 <= 0;
		f_pipeline_1_0 <= 0;
		w_pipeline_1_0 <= 0;
	end else begin
		f_pipeline_0_0 <= i_features_0;
		w_pipeline_0_0 <= i_weights_0;
		f_pipeline_1_0 <= i_features_1;
		w_pipeline_1_0 <= i_weights_1;
	end
end

dsp_block_16_8_false dsp_block_16_8_false_inst_0 (
	.clk(clk),
	.ena(1'b1),
	.aclr(r_pipeline_0),
	.ax(f_pipeline_0_0),
	.ay(w_pipeline_0_0),
	.bx(f_pipeline_1_0),
	.by(w_pipeline_1_0),
	.chainin(64'd0),
	.chainout(chains_0),
	.resulta(res)
);

assign o_result = res;

endmodule

module dsp_block_16_8_false (
	input clk,
	input ena,
	input aclr,
	input [15:0] ax,
	input [7:0] ay,
	input [15:0] bx,
	input [7:0] by,
	input [63:0] chainin,
	output [63:0] chainout,
	output [23:0] resulta
);

wire [10:0] mode;
assign mode = 11'b1010_1010_011;

`ifdef complex_dsp
int_sop_2 mac_component (
	.mode_sigs(mode),
	.clk(clk),
	.reset(aclr),
	.ax(ax),
	.ay(ay),
	.bx(bx),
	.by(by),
	.chainin(chainin),
	.resulta(resulta),
	.chainout(chainout)
);

`else
reg [15:0] ax_reg;
reg [7:0] ay_reg;
reg [15:0] bx_reg;
reg [7:0] by_reg;
reg [23:0] resulta;
always @(posedge clk) begin
  if(aclr) begin
    resulta <= 0;
    ax_reg <= 0;
    ay_reg <= 0;
    bx_reg <= 0;
    by_reg <= 0;
  end
  else begin
    ax_reg <= ax;
    ay_reg <= ay;
    bx_reg <= bx;
    by_reg <= by;
    resulta <= ax_reg * ay_reg + bx_reg * by_reg + chainin;
  end
end
assign chainout = {40'b0, resulta};
`endif

endmodule

module weight_cache_2048_8_0_weight_init_05 (
	input clk,
	input wen0,
	input wen1,
	input [10:0] addr0,
	input [7:0] wdata0,
	output [7:0] data0,
	input [10:0] addr1,
	input [7:0] wdata1,
	output [7:0] data1
);

reg [10:0] addr0_reg;
reg [7:0] data0_reg;
reg [10:0] addr1_reg;
reg [7:0] data1_reg;
reg [7:0] pipeline0_reg_0;
reg [7:0] pipeline1_reg_0;
always @(posedge clk) begin
	addr0_reg <= addr0;
	addr1_reg <= addr1;
	pipeline0_reg_0 <= data0_reg;
	pipeline1_reg_0 <= data1_reg;
end

dual_port_ram u_dual_port_ram(
	.addr1(addr0_reg),
	.we1(wen0),
	.data1(wdata0),
	.out1(data0_reg),
	.addr2(addr1_reg),
	.we2(wen1),
	.data2(wdata1),
	.out2(data1_reg),
	.clk(clk)
);

assign data0 = pipeline0_reg_0;
assign data1 = pipeline1_reg_0;

endmodule

module weight_cache_2048_8_0_weight_init_03 (
	input clk,
	input wen0,
	input wen1,
	input [10:0] addr0,
	input [7:0] wdata0,
	output [7:0] data0,
	input [10:0] addr1,
	input [7:0] wdata1,
	output [7:0] data1
);

reg [10:0] addr0_reg;
reg [7:0] data0_reg;
reg [10:0] addr1_reg;
reg [7:0] data1_reg;
reg [7:0] pipeline0_reg_0;
reg [7:0] pipeline1_reg_0;
always @(posedge clk) begin
	addr0_reg <= addr0;
	addr1_reg <= addr1;
	pipeline0_reg_0 <= data0_reg;
	pipeline1_reg_0 <= data1_reg;
end

dual_port_ram u_dual_port_ram(
	.addr1(addr0_reg),
	.we1(wen0),
	.data1(wdata0),
	.out1(data0_reg),
	.addr2(addr1_reg),
	.we2(wen1),
	.data2(wdata1),
	.out2(data1_reg),
	.clk(clk)
);

assign data0 = pipeline0_reg_0;
assign data1 = pipeline1_reg_0;

endmodule

module weight_cache_2048_8_0_weight_init_02 (
	input clk,
	input wen0,
	input wen1,
	input [10:0] addr0,
	input [7:0] wdata0,
	output [7:0] data0,
	input [10:0] addr1,
	input [7:0] wdata1,
	output [7:0] data1
);

reg [10:0] addr0_reg;
reg [7:0] data0_reg;
reg [10:0] addr1_reg;
reg [7:0] data1_reg;
reg [7:0] pipeline0_reg_0;
reg [7:0] pipeline1_reg_0;
always @(posedge clk) begin
	addr0_reg <= addr0;
	addr1_reg <= addr1;
	pipeline0_reg_0 <= data0_reg;
	pipeline1_reg_0 <= data1_reg;
end

dual_port_ram u_dual_port_ram(
	.addr1(addr0_reg),
	.we1(wen0),
	.data1(wdata0),
	.out1(data0_reg),
	.addr2(addr1_reg),
	.we2(wen1),
	.data2(wdata1),
	.out2(data1_reg),
	.clk(clk)
);

assign data0 = pipeline0_reg_0;
assign data1 = pipeline1_reg_0;

endmodule

module weight_cache_2048_8_0_weight_init_01 (
	input clk,
	input wen0,
	input wen1,
	input [10:0] addr0,
	input [7:0] wdata0,
	output [7:0] data0,
	input [10:0] addr1,
	input [7:0] wdata1,
	output [7:0] data1
);

reg [10:0] addr0_reg;
reg [7:0] data0_reg;
reg [10:0] addr1_reg;
reg [7:0] data1_reg;
reg [7:0] pipeline0_reg_0;
reg [7:0] pipeline1_reg_0;
always @(posedge clk) begin
	addr0_reg <= addr0;
	addr1_reg <= addr1;
	pipeline0_reg_0 <= data0_reg;
	pipeline1_reg_0 <= data1_reg;
end

dual_port_ram u_dual_port_ram(
	.addr1(addr0_reg),
	.we1(wen0),
	.data1(wdata0),
	.out1(data0_reg),
	.addr2(addr1_reg),
	.we2(wen1),
	.data2(wdata1),
	.out2(data1_reg),
	.clk(clk)
);

assign data0 = pipeline0_reg_0;
assign data1 = pipeline1_reg_0;

endmodule

module weight_cache_2048_8_0_weight_init_00 (
	input clk,
	input wen0,
	input wen1,
	input [10:0] addr0,
	input [7:0] wdata0,
	output [7:0] data0,
	input [10:0] addr1,
	input [7:0] wdata1,
	output [7:0] data1
);

reg [10:0] addr0_reg;
reg [7:0] data0_reg;
reg [10:0] addr1_reg;
reg [7:0] data1_reg;
reg [7:0] pipeline0_reg_0;
reg [7:0] pipeline1_reg_0;
always @(posedge clk) begin
	addr0_reg <= addr0;
	addr1_reg <= addr1;
	pipeline0_reg_0 <= data0_reg;
	pipeline1_reg_0 <= data1_reg;
end

dual_port_ram u_dual_port_ram(
	.addr1(addr0_reg),
	.we1(wen0),
	.data1(wdata0),
	.out1(data0_reg),
	.addr2(addr1_reg),
	.we2(wen1),
	.data2(wdata1),
	.out2(data1_reg),
	.clk(clk)
);

assign data0 = pipeline0_reg_0;
assign data1 = pipeline1_reg_0;

endmodule

module accumulator_24_30_3 (
	input clk,
	input i_reset,
	input [23:0] i_result,
	input i_dp_done,
	output [29:0] o_accum
);

reg [29:0] cir_shift_reg_0;
reg [29:0] cir_shift_reg_1;
reg [29:0] cir_shift_reg_2;
reg [29:0] out_reg;
reg [29:0] in_reg;

always @ (posedge clk) begin
	if(i_reset == 1'b1) begin
		cir_shift_reg_0 <= 0;
		cir_shift_reg_1 <= 0;
		cir_shift_reg_2 <= 0;
		out_reg <= 0;
		in_reg <= 0;
	end else begin
		if (i_result[23] == 1'b0) begin
			in_reg <= {6'b000000, i_result};
		end else begin
			in_reg <= {6'b111111, i_result};
		end
		if(i_dp_done == 1'b1) begin
			out_reg <= (cir_shift_reg_0 + in_reg);
			cir_shift_reg_2 <= 0;
		end else begin
			cir_shift_reg_2 <= (cir_shift_reg_0 + in_reg);
		end
		cir_shift_reg_0 <= cir_shift_reg_1;
		cir_shift_reg_1 <= cir_shift_reg_2;
	end
end

assign o_accum = out_reg;

endmodule

module weight_cache_2048_8_0_weight_init_04 (
	input clk,
	input wen0,
	input wen1,
	input [10:0] addr0,
	input [7:0] wdata0,
	output [7:0] data0,
	input [10:0] addr1,
	input [7:0] wdata1,
	output [7:0] data1
);

reg [10:0] addr0_reg;
reg [7:0] data0_reg;
reg [10:0] addr1_reg;
reg [7:0] data1_reg;
reg [7:0] pipeline0_reg_0;
reg [7:0] pipeline1_reg_0;
always @(posedge clk) begin
	addr0_reg <= addr0;
	addr1_reg <= addr1;
	pipeline0_reg_0 <= data0_reg;
	pipeline1_reg_0 <= data1_reg;
end

dual_port_ram u_dual_port_ram(
	.addr1(addr0_reg),
	.we1(wen0),
	.data1(wdata0),
	.out1(data0_reg),
	.addr2(addr1_reg),
	.we2(wen1),
	.data2(wdata1),
	.out2(data1_reg),
	.clk(clk)
);

assign data0 = pipeline0_reg_0;
assign data1 = pipeline1_reg_0;

endmodule

module stream_buffer_4_0 (
	input clk,
	input i_reset,
	input i_wen0,
	input i_wen1,
	input [15:0] i_ddr,
	input [15:0] i_pool,
	input i_eltwise_sel,
	input [15:0] i_eltwise,
	input [14:0] i_waddr,
	output [15:0] o_feature_0,
	output [15:0] o_feature_1,
	output o_done
);

reg [14:0] base_addr;
reg [14:0] offset;
reg [14:0] base_addr_b1;
reg [14:0] offset_b1;
reg [1:0] L_counter;
reg [1:0] C_counter;
reg [1:0] W_counter;
reg [1:0] L_counter_b1;
reg [1:0] C_counter_b1;
reg [1:0] W_counter_b1;
reg done, done_1, done_2, done_3;
reg valid, valid_1, valid_2;
wire [15:0] feature_out_b0;
wire [15:0] feature_out_b1;

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr <= 0;
		offset <= 0;
		C_counter <= 0;
		L_counter <= 0;
		W_counter <= 0;
	end else if (done == 0) begin
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			base_addr <= 0;
			C_counter <= 0;
			L_counter <= 0;
			W_counter <= 0;
			offset <= 0;
		end else if((C_counter == 1) && (L_counter == 2)) begin
			base_addr <= base_addr + 5;
			W_counter <= W_counter + 1'b1;
			C_counter <= 0;
			L_counter <= 0;
			offset <= 0;
		end else if(L_counter == 2) begin
			base_addr <= base_addr + 1'b1;
			C_counter <= C_counter + 1'b1;
			L_counter <= 0;
			offset <= 0;
		end else begin
			offset <= offset + 2;
			L_counter <= L_counter + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr_b1 <= 0;
		offset_b1 <= 0;
		C_counter_b1 <= 0;
		L_counter_b1 <= 0;
		W_counter_b1 <= 0;
	end else if (done == 0) begin
		if((W_counter_b1 == 1443) && (C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= 0;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			W_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if((C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= base_addr_b1 + 5;
			W_counter_b1 <= W_counter_b1 + 1'b1;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if(L_counter_b1 == 2) begin
			base_addr_b1 <= base_addr_b1 + 1'b1;
			C_counter_b1 <= C_counter_b1 + 1'b1;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else begin
			offset_b1 <= offset_b1 + 2;
			L_counter_b1 <= L_counter_b1 + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if(i_reset == 1'b1)begin
		done <= 0;
		done_1 <= 0;
		done_2 <= 0;
		done_3 <= 0;
		valid <= 0;
		valid_1 <= 0;
		valid_2 <= 0;
	end else begin
		valid <= 1;
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			done <= 1;
		end
		done_1 <= done;
		done_2 <= done_1;
		done_3 <= done_2;
		valid_1 <= valid;
		valid_2 <= valid_1;
	end
end

reg [14:0] b0_waddr, b0_raddr, b1_raddr;
always @ (*) begin
	b0_waddr <= base_addr+offset;
	b0_raddr <= base_addr+offset;
	b1_raddr <= base_addr_b1+offset_b1;
end
buffer_16_24200_buffer_init_04 buffer_16_24200_buffer_init_04_B0 (
	.clk(clk),
	.wen(i_wen0),
	.waddr(b0_waddr),
	.wdata(i_ddr),
	.raddr(b0_raddr),
	.rdata(feature_out_b0)
);

reg [15:0] B1_wdata;
always @ (*) begin
	if (i_eltwise_sel) begin
		B1_wdata <= i_eltwise;
	end else begin
		B1_wdata <= i_pool;
	end
end

buffer_16_24200_buffer_init_04 buffer_16_24200_buffer_init_04_B1 (
	.clk(clk),
	.wen(i_wen1),
	.waddr(i_waddr),
	.wdata(B1_wdata),
	.raddr(b1_raddr),
	.rdata(feature_out_b1)
);

assign o_done = valid_2 && (~done_3);
assign o_feature_0 = feature_out_b0;
assign o_feature_1 = feature_out_b1;

endmodule

module buffer_16_24200_buffer_init_04 (
	input clk,
	input wen,
	input [14:0] waddr,
	input [15:0] wdata,
	input [14:0] raddr,
	output [15:0] rdata
);

reg [14:0] raddr_reg;
reg [15:0] rdata_reg;
reg [15:0] pipeline_reg_0;
wire [15:0] rd_dummy_signal;
wire [15:0] wr_dummy_signal;
wire [15:0] rdata_wire;
assign rd_dummy_signal = 0;

always @(posedge clk) begin
	rdata_reg <= rdata_wire;
	raddr_reg <= raddr;
	pipeline_reg_0 <= rdata_reg;
end

dual_port_ram u_dual_port_ram(
	.addr1(waddr),
	.we1(wen),
	.data1(wdata),
	.out1(wr_dummy_signal),
	.addr2(raddr_reg),
	.we2(1'b0),
	.data2(rd_dummy_signal),
	.out2(rdata_wire),
	.clk(clk)
);
assign rdata = pipeline_reg_0;

endmodule

module stream_buffer_4_1 (
	input clk,
	input i_reset,
	input i_wen0,
	input i_wen1,
	input [15:0] i_ddr,
	input [15:0] i_pool,
	input i_eltwise_sel,
	input [15:0] i_eltwise,
	input [14:0] i_waddr,
	output [15:0] o_feature_0,
	output [15:0] o_feature_1,
	output o_done
);

reg [14:0] base_addr;
reg [14:0] offset;
reg [14:0] base_addr_b1;
reg [14:0] offset_b1;
reg [1:0] L_counter;
reg [1:0] C_counter;
reg [1:0] W_counter;
reg [1:0] L_counter_b1;
reg [1:0] C_counter_b1;
reg [1:0] W_counter_b1;
reg done, done_1, done_2, done_3;
reg valid, valid_1, valid_2;
wire [15:0] feature_out_b0;
wire [15:0] feature_out_b1;

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr <= 0;
		offset <= 0;
		C_counter <= 0;
		L_counter <= 0;
		W_counter <= 0;
	end else if (done == 0) begin
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			base_addr <= 0;
			C_counter <= 0;
			L_counter <= 0;
			W_counter <= 0;
			offset <= 0;
		end else if((C_counter == 1) && (L_counter == 2)) begin
			base_addr <= base_addr + 5;
			W_counter <= W_counter + 1'b1;
			C_counter <= 0;
			L_counter <= 0;
			offset <= 0;
		end else if(L_counter == 2) begin
			base_addr <= base_addr + 1'b1;
			C_counter <= C_counter + 1'b1;
			L_counter <= 0;
			offset <= 0;
		end else begin
			offset <= offset + 2;
			L_counter <= L_counter + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr_b1 <= 0;
		offset_b1 <= 0;
		C_counter_b1 <= 0;
		L_counter_b1 <= 0;
		W_counter_b1 <= 0;
	end else if (done == 0) begin
		if((W_counter_b1 == 1443) && (C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= 0;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			W_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if((C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= base_addr_b1 + 5;
			W_counter_b1 <= W_counter_b1 + 1'b1;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if(L_counter_b1 == 2) begin
			base_addr_b1 <= base_addr_b1 + 1'b1;
			C_counter_b1 <= C_counter_b1 + 1'b1;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else begin
			offset_b1 <= offset_b1 + 2;
			L_counter_b1 <= L_counter_b1 + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if(i_reset == 1'b1)begin
		done <= 0;
		done_1 <= 0;
		done_2 <= 0;
		done_3 <= 0;
		valid <= 0;
		valid_1 <= 0;
		valid_2 <= 0;
	end else begin
		valid <= 1;
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			done <= 1;
		end
		done_1 <= done;
		done_2 <= done_1;
		done_3 <= done_2;
		valid_1 <= valid;
		valid_2 <= valid_1;
	end
end

reg [14:0] b0_waddr, b0_raddr, b1_raddr;
always @ (*) begin
	b0_waddr <= base_addr+offset;
	b0_raddr <= base_addr+offset;
	b1_raddr <= base_addr_b1+offset_b1;
end
buffer_16_24200_buffer_init_14 buffer_16_24200_buffer_init_14_B0 (
	.clk(clk),
	.wen(i_wen0),
	.waddr(b0_waddr),
	.wdata(i_ddr),
	.raddr(b0_raddr),
	.rdata(feature_out_b0)
);

reg [15:0] B1_wdata;
always @ (*) begin
	if (i_eltwise_sel) begin
		B1_wdata <= i_eltwise;
	end else begin
		B1_wdata <= i_pool;
	end
end

buffer_16_24200_buffer_init_14 buffer_16_24200_buffer_init_14_B1 (
	.clk(clk),
	.wen(i_wen1),
	.waddr(i_waddr),
	.wdata(B1_wdata),
	.raddr(b1_raddr),
	.rdata(feature_out_b1)
);

assign o_done = valid_2 && (~done_3);
assign o_feature_0 = feature_out_b0;
assign o_feature_1 = feature_out_b1;

endmodule

module buffer_16_24200_buffer_init_14 (
	input clk,
	input wen,
	input [14:0] waddr,
	input [15:0] wdata,
	input [14:0] raddr,
	output [15:0] rdata
);

reg [14:0] raddr_reg;
reg [15:0] rdata_reg;
reg [15:0] pipeline_reg_0;
wire [15:0] rd_dummy_signal;
wire [15:0] wr_dummy_signal;
wire [15:0] rdata_wire;
assign rd_dummy_signal = 0;

always @(posedge clk) begin
	rdata_reg <= rdata_wire;
	raddr_reg <= raddr;
	pipeline_reg_0 <= rdata_reg;
end

dual_port_ram u_dual_port_ram(
	.addr1(waddr),
	.we1(wen),
	.data1(wdata),
	.out1(wr_dummy_signal),
	.addr2(raddr_reg),
	.we2(1'b0),
	.data2(rd_dummy_signal),
	.out2(rdata_wire),
	.clk(clk)
);
assign rdata = pipeline_reg_0;

endmodule

module stream_buffer_2_0 (
	input clk,
	input i_reset,
	input i_wen0,
	input i_wen1,
	input [15:0] i_ddr,
	input [15:0] i_pool,
	input i_eltwise_sel,
	input [15:0] i_eltwise,
	input [14:0] i_waddr,
	output [15:0] o_feature_0,
	output [15:0] o_feature_1,
	output o_done
);

reg [14:0] base_addr;
reg [14:0] offset;
reg [14:0] base_addr_b1;
reg [14:0] offset_b1;
reg [1:0] L_counter;
reg [1:0] C_counter;
reg [1:0] W_counter;
reg [1:0] L_counter_b1;
reg [1:0] C_counter_b1;
reg [1:0] W_counter_b1;
reg done, done_1, done_2, done_3;
reg valid, valid_1, valid_2;
wire [15:0] feature_out_b0;
wire [15:0] feature_out_b1;

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr <= 0;
		offset <= 0;
		C_counter <= 0;
		L_counter <= 0;
		W_counter <= 0;
	end else if (done == 0) begin
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			base_addr <= 0;
			C_counter <= 0;
			L_counter <= 0;
			W_counter <= 0;
			offset <= 0;
		end else if((C_counter == 1) && (L_counter == 2)) begin
			base_addr <= base_addr + 5;
			W_counter <= W_counter + 1'b1;
			C_counter <= 0;
			L_counter <= 0;
			offset <= 0;
		end else if(L_counter == 2) begin
			base_addr <= base_addr + 1'b1;
			C_counter <= C_counter + 1'b1;
			L_counter <= 0;
			offset <= 0;
		end else begin
			offset <= offset + 2;
			L_counter <= L_counter + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr_b1 <= 0;
		offset_b1 <= 0;
		C_counter_b1 <= 0;
		L_counter_b1 <= 0;
		W_counter_b1 <= 0;
	end else if (done == 0) begin
		if((W_counter_b1 == 1443) && (C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= 0;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			W_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if((C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= base_addr_b1 + 5;
			W_counter_b1 <= W_counter_b1 + 1'b1;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if(L_counter_b1 == 2) begin
			base_addr_b1 <= base_addr_b1 + 1'b1;
			C_counter_b1 <= C_counter_b1 + 1'b1;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else begin
			offset_b1 <= offset_b1 + 2;
			L_counter_b1 <= L_counter_b1 + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if(i_reset == 1'b1)begin
		done <= 0;
		done_1 <= 0;
		done_2 <= 0;
		done_3 <= 0;
		valid <= 0;
		valid_1 <= 0;
		valid_2 <= 0;
	end else begin
		valid <= 1;
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			done <= 1;
		end
		done_1 <= done;
		done_2 <= done_1;
		done_3 <= done_2;
		valid_1 <= valid;
		valid_2 <= valid_1;
	end
end

reg [14:0] b0_waddr, b0_raddr, b1_raddr;
always @ (*) begin
	b0_waddr <= base_addr+offset;
	b0_raddr <= base_addr+offset;
	b1_raddr <= base_addr_b1+offset_b1;
end
buffer_16_24200_buffer_init_02 buffer_16_24200_buffer_init_02_B0 (
	.clk(clk),
	.wen(i_wen0),
	.waddr(b0_waddr),
	.wdata(i_ddr),
	.raddr(b0_raddr),
	.rdata(feature_out_b0)
);

reg [15:0] B1_wdata;
always @ (*) begin
	if (i_eltwise_sel) begin
		B1_wdata <= i_eltwise;
	end else begin
		B1_wdata <= i_pool;
	end
end

buffer_16_24200_buffer_init_02 buffer_16_24200_buffer_init_02_B1 (
	.clk(clk),
	.wen(i_wen1),
	.waddr(i_waddr),
	.wdata(B1_wdata),
	.raddr(b1_raddr),
	.rdata(feature_out_b1)
);

assign o_done = valid_2 && (~done_3);
assign o_feature_0 = feature_out_b0;
assign o_feature_1 = feature_out_b1;

endmodule

module buffer_16_24200_buffer_init_02 (
	input clk,
	input wen,
	input [14:0] waddr,
	input [15:0] wdata,
	input [14:0] raddr,
	output [15:0] rdata
);

reg [14:0] raddr_reg;
reg [15:0] rdata_reg;
reg [15:0] pipeline_reg_0;
wire [15:0] rd_dummy_signal;
wire [15:0] wr_dummy_signal;
wire [15:0] rdata_wire;
assign rd_dummy_signal = 0;

always @(posedge clk) begin
	rdata_reg <= rdata_wire;
	raddr_reg <= raddr;
	pipeline_reg_0 <= rdata_reg;
end

dual_port_ram u_dual_port_ram(
	.addr1(waddr),
	.we1(wen),
	.data1(wdata),
	.out1(wr_dummy_signal),
	.addr2(raddr_reg),
	.we2(1'b0),
	.data2(rd_dummy_signal),
	.out2(rdata_wire),
	.clk(clk)
);
assign rdata = pipeline_reg_0;

endmodule

module stream_buffer_2_1 (
	input clk,
	input i_reset,
	input i_wen0,
	input i_wen1,
	input [15:0] i_ddr,
	input [15:0] i_pool,
	input i_eltwise_sel,
	input [15:0] i_eltwise,
	input [14:0] i_waddr,
	output [15:0] o_feature_0,
	output [15:0] o_feature_1,
	output o_done
);

reg [14:0] base_addr;
reg [14:0] offset;
reg [14:0] base_addr_b1;
reg [14:0] offset_b1;
reg [1:0] L_counter;
reg [1:0] C_counter;
reg [1:0] W_counter;
reg [1:0] L_counter_b1;
reg [1:0] C_counter_b1;
reg [1:0] W_counter_b1;
reg done, done_1, done_2, done_3;
reg valid, valid_1, valid_2;
wire [15:0] feature_out_b0;
wire [15:0] feature_out_b1;

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr <= 0;
		offset <= 0;
		C_counter <= 0;
		L_counter <= 0;
		W_counter <= 0;
	end else if (done == 0) begin
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			base_addr <= 0;
			C_counter <= 0;
			L_counter <= 0;
			W_counter <= 0;
			offset <= 0;
		end else if((C_counter == 1) && (L_counter == 2)) begin
			base_addr <= base_addr + 5;
			W_counter <= W_counter + 1'b1;
			C_counter <= 0;
			L_counter <= 0;
			offset <= 0;
		end else if(L_counter == 2) begin
			base_addr <= base_addr + 1'b1;
			C_counter <= C_counter + 1'b1;
			L_counter <= 0;
			offset <= 0;
		end else begin
			offset <= offset + 2;
			L_counter <= L_counter + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr_b1 <= 0;
		offset_b1 <= 0;
		C_counter_b1 <= 0;
		L_counter_b1 <= 0;
		W_counter_b1 <= 0;
	end else if (done == 0) begin
		if((W_counter_b1 == 1443) && (C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= 0;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			W_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if((C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= base_addr_b1 + 5;
			W_counter_b1 <= W_counter_b1 + 1'b1;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if(L_counter_b1 == 2) begin
			base_addr_b1 <= base_addr_b1 + 1'b1;
			C_counter_b1 <= C_counter_b1 + 1'b1;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else begin
			offset_b1 <= offset_b1 + 2;
			L_counter_b1 <= L_counter_b1 + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if(i_reset == 1'b1)begin
		done <= 0;
		done_1 <= 0;
		done_2 <= 0;
		done_3 <= 0;
		valid <= 0;
		valid_1 <= 0;
		valid_2 <= 0;
	end else begin
		valid <= 1;
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			done <= 1;
		end
		done_1 <= done;
		done_2 <= done_1;
		done_3 <= done_2;
		valid_1 <= valid;
		valid_2 <= valid_1;
	end
end

reg [14:0] b0_waddr, b0_raddr, b1_raddr;
always @ (*) begin
	b0_waddr <= base_addr+offset;
	b0_raddr <= base_addr+offset;
	b1_raddr <= base_addr_b1+offset_b1;
end
buffer_16_24200_buffer_init_12 buffer_16_24200_buffer_init_12_B0 (
	.clk(clk),
	.wen(i_wen0),
	.waddr(b0_waddr),
	.wdata(i_ddr),
	.raddr(b0_raddr),
	.rdata(feature_out_b0)
);

reg [15:0] B1_wdata;
always @ (*) begin
	if (i_eltwise_sel) begin
		B1_wdata <= i_eltwise;
	end else begin
		B1_wdata <= i_pool;
	end
end

buffer_16_24200_buffer_init_12 buffer_16_24200_buffer_init_12_B1 (
	.clk(clk),
	.wen(i_wen1),
	.waddr(i_waddr),
	.wdata(B1_wdata),
	.raddr(b1_raddr),
	.rdata(feature_out_b1)
);

assign o_done = valid_2 && (~done_3);
assign o_feature_0 = feature_out_b0;
assign o_feature_1 = feature_out_b1;

endmodule

module buffer_16_24200_buffer_init_12 (
	input clk,
	input wen,
	input [14:0] waddr,
	input [15:0] wdata,
	input [14:0] raddr,
	output [15:0] rdata
);

reg [14:0] raddr_reg;
reg [15:0] rdata_reg;
reg [15:0] pipeline_reg_0;
wire [15:0] rd_dummy_signal;
wire [15:0] wr_dummy_signal;
wire [15:0] rdata_wire;
assign rd_dummy_signal = 0;

always @(posedge clk) begin
	rdata_reg <= rdata_wire;
	raddr_reg <= raddr;
	pipeline_reg_0 <= rdata_reg;
end

dual_port_ram u_dual_port_ram(
	.addr1(waddr),
	.we1(wen),
	.data1(wdata),
	.out1(wr_dummy_signal),
	.addr2(raddr_reg),
	.we2(1'b0),
	.data2(rd_dummy_signal),
	.out2(rdata_wire),
	.clk(clk)
);
assign rdata = pipeline_reg_0;

endmodule

module stream_buffer_1_1 (
	input clk,
	input i_reset,
	input i_wen0,
	input i_wen1,
	input [15:0] i_ddr,
	input [15:0] i_pool,
	input i_eltwise_sel,
	input [15:0] i_eltwise,
	input [14:0] i_waddr,
	output [15:0] o_feature_0,
	output [15:0] o_feature_1,
	output o_done
);

reg [14:0] base_addr;
reg [14:0] offset;
reg [14:0] base_addr_b1;
reg [14:0] offset_b1;
reg [1:0] L_counter;
reg [1:0] C_counter;
reg [1:0] W_counter;
reg [1:0] L_counter_b1;
reg [1:0] C_counter_b1;
reg [1:0] W_counter_b1;
reg done, done_1, done_2, done_3;
reg valid, valid_1, valid_2;
wire [15:0] feature_out_b0;
wire [15:0] feature_out_b1;

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr <= 0;
		offset <= 0;
		C_counter <= 0;
		L_counter <= 0;
		W_counter <= 0;
	end else if (done == 0) begin
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			base_addr <= 0;
			C_counter <= 0;
			L_counter <= 0;
			W_counter <= 0;
			offset <= 0;
		end else if((C_counter == 1) && (L_counter == 2)) begin
			base_addr <= base_addr + 5;
			W_counter <= W_counter + 1'b1;
			C_counter <= 0;
			L_counter <= 0;
			offset <= 0;
		end else if(L_counter == 2) begin
			base_addr <= base_addr + 1'b1;
			C_counter <= C_counter + 1'b1;
			L_counter <= 0;
			offset <= 0;
		end else begin
			offset <= offset + 2;
			L_counter <= L_counter + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr_b1 <= 0;
		offset_b1 <= 0;
		C_counter_b1 <= 0;
		L_counter_b1 <= 0;
		W_counter_b1 <= 0;
	end else if (done == 0) begin
		if((W_counter_b1 == 1443) && (C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= 0;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			W_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if((C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= base_addr_b1 + 5;
			W_counter_b1 <= W_counter_b1 + 1'b1;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if(L_counter_b1 == 2) begin
			base_addr_b1 <= base_addr_b1 + 1'b1;
			C_counter_b1 <= C_counter_b1 + 1'b1;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else begin
			offset_b1 <= offset_b1 + 2;
			L_counter_b1 <= L_counter_b1 + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if(i_reset == 1'b1)begin
		done <= 0;
		done_1 <= 0;
		done_2 <= 0;
		done_3 <= 0;
		valid <= 0;
		valid_1 <= 0;
		valid_2 <= 0;
	end else begin
		valid <= 1;
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			done <= 1;
		end
		done_1 <= done;
		done_2 <= done_1;
		done_3 <= done_2;
		valid_1 <= valid;
		valid_2 <= valid_1;
	end
end

reg [14:0] b0_waddr, b0_raddr, b1_raddr;
always @ (*) begin
	b0_waddr <= base_addr+offset;
	b0_raddr <= base_addr+offset;
	b1_raddr <= base_addr_b1+offset_b1;
end
buffer_16_24200_buffer_init_11 buffer_16_24200_buffer_init_11_B0 (
	.clk(clk),
	.wen(i_wen0),
	.waddr(b0_waddr),
	.wdata(i_ddr),
	.raddr(b0_raddr),
	.rdata(feature_out_b0)
);

reg [15:0] B1_wdata;
always @ (*) begin
	if (i_eltwise_sel) begin
		B1_wdata <= i_eltwise;
	end else begin
		B1_wdata <= i_pool;
	end
end

buffer_16_24200_buffer_init_11 buffer_16_24200_buffer_init_11_B1 (
	.clk(clk),
	.wen(i_wen1),
	.waddr(i_waddr),
	.wdata(B1_wdata),
	.raddr(b1_raddr),
	.rdata(feature_out_b1)
);

assign o_done = valid_2 && (~done_3);
assign o_feature_0 = feature_out_b0;
assign o_feature_1 = feature_out_b1;

endmodule

module buffer_16_24200_buffer_init_11 (
	input clk,
	input wen,
	input [14:0] waddr,
	input [15:0] wdata,
	input [14:0] raddr,
	output [15:0] rdata
);

reg [14:0] raddr_reg;
reg [15:0] rdata_reg;
reg [15:0] pipeline_reg_0;
wire [15:0] rd_dummy_signal;
wire [15:0] wr_dummy_signal;
wire [15:0] rdata_wire;
assign rd_dummy_signal = 0;

always @(posedge clk) begin
	rdata_reg <= rdata_wire;
	raddr_reg <= raddr;
	pipeline_reg_0 <= rdata_reg;
end

dual_port_ram u_dual_port_ram(
	.addr1(waddr),
	.we1(wen),
	.data1(wdata),
	.out1(wr_dummy_signal),
	.addr2(raddr_reg),
	.we2(1'b0),
	.data2(rd_dummy_signal),
	.out2(rdata_wire),
	.clk(clk)
);
assign rdata = pipeline_reg_0;

endmodule

module stream_buffer_1_0 (
	input clk,
	input i_reset,
	input i_wen0,
	input i_wen1,
	input [15:0] i_ddr,
	input [15:0] i_pool,
	input i_eltwise_sel,
	input [15:0] i_eltwise,
	input [14:0] i_waddr,
	output [15:0] o_feature_0,
	output [15:0] o_feature_1,
	output o_done
);

reg [14:0] base_addr;
reg [14:0] offset;
reg [14:0] base_addr_b1;
reg [14:0] offset_b1;
reg [1:0] L_counter;
reg [1:0] C_counter;
reg [1:0] W_counter;
reg [1:0] L_counter_b1;
reg [1:0] C_counter_b1;
reg [1:0] W_counter_b1;
reg done, done_1, done_2, done_3;
reg valid, valid_1, valid_2;
wire [15:0] feature_out_b0;
wire [15:0] feature_out_b1;

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr <= 0;
		offset <= 0;
		C_counter <= 0;
		L_counter <= 0;
		W_counter <= 0;
	end else if (done == 0) begin
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			base_addr <= 0;
			C_counter <= 0;
			L_counter <= 0;
			W_counter <= 0;
			offset <= 0;
		end else if((C_counter == 1) && (L_counter == 2)) begin
			base_addr <= base_addr + 5;
			W_counter <= W_counter + 1'b1;
			C_counter <= 0;
			L_counter <= 0;
			offset <= 0;
		end else if(L_counter == 2) begin
			base_addr <= base_addr + 1'b1;
			C_counter <= C_counter + 1'b1;
			L_counter <= 0;
			offset <= 0;
		end else begin
			offset <= offset + 2;
			L_counter <= L_counter + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr_b1 <= 0;
		offset_b1 <= 0;
		C_counter_b1 <= 0;
		L_counter_b1 <= 0;
		W_counter_b1 <= 0;
	end else if (done == 0) begin
		if((W_counter_b1 == 1443) && (C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= 0;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			W_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if((C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= base_addr_b1 + 5;
			W_counter_b1 <= W_counter_b1 + 1'b1;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if(L_counter_b1 == 2) begin
			base_addr_b1 <= base_addr_b1 + 1'b1;
			C_counter_b1 <= C_counter_b1 + 1'b1;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else begin
			offset_b1 <= offset_b1 + 2;
			L_counter_b1 <= L_counter_b1 + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if(i_reset == 1'b1)begin
		done <= 0;
		done_1 <= 0;
		done_2 <= 0;
		done_3 <= 0;
		valid <= 0;
		valid_1 <= 0;
		valid_2 <= 0;
	end else begin
		valid <= 1;
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			done <= 1;
		end
		done_1 <= done;
		done_2 <= done_1;
		done_3 <= done_2;
		valid_1 <= valid;
		valid_2 <= valid_1;
	end
end

reg [14:0] b0_waddr, b0_raddr, b1_raddr;
always @ (*) begin
	b0_waddr <= base_addr+offset;
	b0_raddr <= base_addr+offset;
	b1_raddr <= base_addr_b1+offset_b1;
end
buffer_16_24200_buffer_init_01 buffer_16_24200_buffer_init_01_B0 (
	.clk(clk),
	.wen(i_wen0),
	.waddr(b0_waddr),
	.wdata(i_ddr),
	.raddr(b0_raddr),
	.rdata(feature_out_b0)
);

reg [15:0] B1_wdata;
always @ (*) begin
	if (i_eltwise_sel) begin
		B1_wdata <= i_eltwise;
	end else begin
		B1_wdata <= i_pool;
	end
end

buffer_16_24200_buffer_init_01 buffer_16_24200_buffer_init_01_B1 (
	.clk(clk),
	.wen(i_wen1),
	.waddr(i_waddr),
	.wdata(B1_wdata),
	.raddr(b1_raddr),
	.rdata(feature_out_b1)
);

assign o_done = valid_2 && (~done_3);
assign o_feature_0 = feature_out_b0;
assign o_feature_1 = feature_out_b1;

endmodule

module buffer_16_24200_buffer_init_01 (
	input clk,
	input wen,
	input [14:0] waddr,
	input [15:0] wdata,
	input [14:0] raddr,
	output [15:0] rdata
);

reg [14:0] raddr_reg;
reg [15:0] rdata_reg;
reg [15:0] pipeline_reg_0;
wire [15:0] rd_dummy_signal;
wire [15:0] wr_dummy_signal;
wire [15:0] rdata_wire;
assign rd_dummy_signal = 0;

always @(posedge clk) begin
	rdata_reg <= rdata_wire;
	raddr_reg <= raddr;
	pipeline_reg_0 <= rdata_reg;
end

dual_port_ram u_dual_port_ram(
	.addr1(waddr),
	.we1(wen),
	.data1(wdata),
	.out1(wr_dummy_signal),
	.addr2(raddr_reg),
	.we2(1'b0),
	.data2(rd_dummy_signal),
	.out2(rdata_wire),
	.clk(clk)
);
assign rdata = pipeline_reg_0;

endmodule

module stream_buffer_0_0 (
	input clk,
	input i_reset,
	input i_wen0,
	input i_wen1,
	input [15:0] i_ddr,
	input [15:0] i_pool,
	input i_eltwise_sel,
	input [15:0] i_eltwise,
	input [14:0] i_waddr,
	output [15:0] o_feature_0,
	output [15:0] o_feature_1,
	output o_done
);

reg [14:0] base_addr;
reg [14:0] offset;
reg [14:0] base_addr_b1;
reg [14:0] offset_b1;
reg [1:0] L_counter;
reg [1:0] C_counter;
reg [1:0] W_counter;
reg [1:0] L_counter_b1;
reg [1:0] C_counter_b1;
reg [1:0] W_counter_b1;
reg done, done_1, done_2, done_3;
reg valid, valid_1, valid_2;
wire [15:0] feature_out_b0;
wire [15:0] feature_out_b1;

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr <= 0;
		offset <= 0;
		C_counter <= 0;
		L_counter <= 0;
		W_counter <= 0;
	end else if (done == 0) begin
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			base_addr <= 0;
			C_counter <= 0;
			L_counter <= 0;
			W_counter <= 0;
			offset <= 0;
		end else if((C_counter == 1) && (L_counter == 2)) begin
			base_addr <= base_addr + 5;
			W_counter <= W_counter + 1'b1;
			C_counter <= 0;
			L_counter <= 0;
			offset <= 0;
		end else if(L_counter == 2) begin
			base_addr <= base_addr + 1'b1;
			C_counter <= C_counter + 1'b1;
			L_counter <= 0;
			offset <= 0;
		end else begin
			offset <= offset + 2;
			L_counter <= L_counter + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr_b1 <= 0;
		offset_b1 <= 0;
		C_counter_b1 <= 0;
		L_counter_b1 <= 0;
		W_counter_b1 <= 0;
	end else if (done == 0) begin
		if((W_counter_b1 == 1443) && (C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= 0;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			W_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if((C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= base_addr_b1 + 5;
			W_counter_b1 <= W_counter_b1 + 1'b1;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if(L_counter_b1 == 2) begin
			base_addr_b1 <= base_addr_b1 + 1'b1;
			C_counter_b1 <= C_counter_b1 + 1'b1;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else begin
			offset_b1 <= offset_b1 + 2;
			L_counter_b1 <= L_counter_b1 + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if(i_reset == 1'b1)begin
		done <= 0;
		done_1 <= 0;
		done_2 <= 0;
		done_3 <= 0;
		valid <= 0;
		valid_1 <= 0;
		valid_2 <= 0;
	end else begin
		valid <= 1;
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			done <= 1;
		end
		done_1 <= done;
		done_2 <= done_1;
		done_3 <= done_2;
		valid_1 <= valid;
		valid_2 <= valid_1;
	end
end

reg [14:0] b0_waddr, b0_raddr, b1_raddr;
always @ (*) begin
	b0_waddr <= base_addr+offset;
	b0_raddr <= base_addr+offset;
	b1_raddr <= base_addr_b1+offset_b1;
end
buffer_16_24200_buffer_init_00 buffer_16_24200_buffer_init_00_B0 (
	.clk(clk),
	.wen(i_wen0),
	.waddr(b0_waddr),
	.wdata(i_ddr),
	.raddr(b0_raddr),
	.rdata(feature_out_b0)
);

reg [15:0] B1_wdata;
always @ (*) begin
	if (i_eltwise_sel) begin
		B1_wdata <= i_eltwise;
	end else begin
		B1_wdata <= i_pool;
	end
end

buffer_16_24200_buffer_init_00 buffer_16_24200_buffer_init_00_B1 (
	.clk(clk),
	.wen(i_wen1),
	.waddr(i_waddr),
	.wdata(B1_wdata),
	.raddr(b1_raddr),
	.rdata(feature_out_b1)
);

assign o_done = valid_2 && (~done_3);
assign o_feature_0 = feature_out_b0;
assign o_feature_1 = feature_out_b1;

endmodule

module buffer_16_24200_buffer_init_00 (
	input clk,
	input wen,
	input [14:0] waddr,
	input [15:0] wdata,
	input [14:0] raddr,
	output [15:0] rdata
);

reg [14:0] raddr_reg;
reg [15:0] rdata_reg;
reg [15:0] pipeline_reg_0;
wire [15:0] rd_dummy_signal;
wire [15:0] wr_dummy_signal;
wire [15:0] rdata_wire;
assign rd_dummy_signal = 0;

always @(posedge clk) begin
	rdata_reg <= rdata_wire;
	raddr_reg <= raddr;
	pipeline_reg_0 <= rdata_reg;
end

dual_port_ram u_dual_port_ram(
	.addr1(waddr),
	.we1(wen),
	.data1(wdata),
	.out1(wr_dummy_signal),
	.addr2(raddr_reg),
	.we2(1'b0),
	.data2(rd_dummy_signal),
	.out2(rdata_wire),
	.clk(clk)
);
assign rdata = pipeline_reg_0;

endmodule

module stream_buffer_0_1 (
	input clk,
	input i_reset,
	input i_wen0,
	input i_wen1,
	input [15:0] i_ddr,
	input [15:0] i_pool,
	input i_eltwise_sel,
	input [15:0] i_eltwise,
	input [14:0] i_waddr,
	output [15:0] o_feature_0,
	output [15:0] o_feature_1,
	output o_done
);

reg [14:0] base_addr;
reg [14:0] offset;
reg [14:0] base_addr_b1;
reg [14:0] offset_b1;
reg [1:0] L_counter;
reg [1:0] C_counter;
reg [1:0] W_counter;
reg [1:0] L_counter_b1;
reg [1:0] C_counter_b1;
reg [1:0] W_counter_b1;
reg done, done_1, done_2, done_3;
reg valid, valid_1, valid_2;
wire [15:0] feature_out_b0;
wire [15:0] feature_out_b1;

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr <= 0;
		offset <= 0;
		C_counter <= 0;
		L_counter <= 0;
		W_counter <= 0;
	end else if (done == 0) begin
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			base_addr <= 0;
			C_counter <= 0;
			L_counter <= 0;
			W_counter <= 0;
			offset <= 0;
		end else if((C_counter == 1) && (L_counter == 2)) begin
			base_addr <= base_addr + 5;
			W_counter <= W_counter + 1'b1;
			C_counter <= 0;
			L_counter <= 0;
			offset <= 0;
		end else if(L_counter == 2) begin
			base_addr <= base_addr + 1'b1;
			C_counter <= C_counter + 1'b1;
			L_counter <= 0;
			offset <= 0;
		end else begin
			offset <= offset + 2;
			L_counter <= L_counter + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr_b1 <= 0;
		offset_b1 <= 0;
		C_counter_b1 <= 0;
		L_counter_b1 <= 0;
		W_counter_b1 <= 0;
	end else if (done == 0) begin
		if((W_counter_b1 == 1443) && (C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= 0;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			W_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if((C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= base_addr_b1 + 5;
			W_counter_b1 <= W_counter_b1 + 1'b1;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if(L_counter_b1 == 2) begin
			base_addr_b1 <= base_addr_b1 + 1'b1;
			C_counter_b1 <= C_counter_b1 + 1'b1;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else begin
			offset_b1 <= offset_b1 + 2;
			L_counter_b1 <= L_counter_b1 + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if(i_reset == 1'b1)begin
		done <= 0;
		done_1 <= 0;
		done_2 <= 0;
		done_3 <= 0;
		valid <= 0;
		valid_1 <= 0;
		valid_2 <= 0;
	end else begin
		valid <= 1;
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			done <= 1;
		end
		done_1 <= done;
		done_2 <= done_1;
		done_3 <= done_2;
		valid_1 <= valid;
		valid_2 <= valid_1;
	end
end

reg [14:0] b0_waddr, b0_raddr, b1_raddr;
always @ (*) begin
	b0_waddr <= base_addr+offset;
	b0_raddr <= base_addr+offset;
	b1_raddr <= base_addr_b1+offset_b1;
end
buffer_16_24200_buffer_init_10 buffer_16_24200_buffer_init_10_B0 (
	.clk(clk),
	.wen(i_wen0),
	.waddr(b0_waddr),
	.wdata(i_ddr),
	.raddr(b0_raddr),
	.rdata(feature_out_b0)
);

reg [15:0] B1_wdata;
always @ (*) begin
	if (i_eltwise_sel) begin
		B1_wdata <= i_eltwise;
	end else begin
		B1_wdata <= i_pool;
	end
end

buffer_16_24200_buffer_init_10 buffer_16_24200_buffer_init_10_B1 (
	.clk(clk),
	.wen(i_wen1),
	.waddr(i_waddr),
	.wdata(B1_wdata),
	.raddr(b1_raddr),
	.rdata(feature_out_b1)
);

assign o_done = valid_2 && (~done_3);
assign o_feature_0 = feature_out_b0;
assign o_feature_1 = feature_out_b1;

endmodule

module buffer_16_24200_buffer_init_10 (
	input clk,
	input wen,
	input [14:0] waddr,
	input [15:0] wdata,
	input [14:0] raddr,
	output [15:0] rdata
);

reg [14:0] raddr_reg;
reg [15:0] rdata_reg;
reg [15:0] pipeline_reg_0;
wire [15:0] rd_dummy_signal;
wire [15:0] wr_dummy_signal;
wire [15:0] rdata_wire;
assign rd_dummy_signal = 0;

always @(posedge clk) begin
	rdata_reg <= rdata_wire;
	raddr_reg <= raddr;
	pipeline_reg_0 <= rdata_reg;
end

dual_port_ram u_dual_port_ram(
	.addr1(waddr),
	.we1(wen),
	.data1(wdata),
	.out1(wr_dummy_signal),
	.addr2(raddr_reg),
	.we2(1'b0),
	.data2(rd_dummy_signal),
	.out2(rdata_wire),
	.clk(clk)
);
assign rdata = pipeline_reg_0;

endmodule

module signal_width_reducer (
	input clk,
	input [15:0] signals_0_0,
	output reduced_signals_0_0,
	input [15:0] signals_0_1,
	output reduced_signals_0_1,
	input [15:0] signals_1_0,
	output reduced_signals_1_0,
	input [15:0] signals_1_1,
	output reduced_signals_1_1,
	input [15:0] signals_2_0,
	output reduced_signals_2_0,
	input [15:0] signals_2_1,
	output reduced_signals_2_1,
	input [15:0] signals_3_0,
	output reduced_signals_3_0,
	input [15:0] signals_3_1,
	output reduced_signals_3_1,
	input [15:0] signals_4_0,
	output reduced_signals_4_0,
	input [15:0] signals_4_1,
	output reduced_signals_4_1,
	input [15:0] signals_5_0,
	output reduced_signals_5_0,
	input [15:0] signals_5_1,
	output reduced_signals_5_1,
	input reset
);

pipelined_xor_tree_16 pipelined_xor_tree_16_inst_0_0 (
	.clk(clk),
	.reset(reset),
	.signal(signals_0_0),
	.reduced_signal(reduced_signals_0_0)
);

pipelined_xor_tree_16 pipelined_xor_tree_16_inst_0_1 (
	.clk(clk),
	.reset(reset),
	.signal(signals_0_1),
	.reduced_signal(reduced_signals_0_1)
);

pipelined_xor_tree_16 pipelined_xor_tree_16_inst_1_0 (
	.clk(clk),
	.reset(reset),
	.signal(signals_1_0),
	.reduced_signal(reduced_signals_1_0)
);

pipelined_xor_tree_16 pipelined_xor_tree_16_inst_1_1 (
	.clk(clk),
	.reset(reset),
	.signal(signals_1_1),
	.reduced_signal(reduced_signals_1_1)
);

pipelined_xor_tree_16 pipelined_xor_tree_16_inst_2_0 (
	.clk(clk),
	.reset(reset),
	.signal(signals_2_0),
	.reduced_signal(reduced_signals_2_0)
);

pipelined_xor_tree_16 pipelined_xor_tree_16_inst_2_1 (
	.clk(clk),
	.reset(reset),
	.signal(signals_2_1),
	.reduced_signal(reduced_signals_2_1)
);

pipelined_xor_tree_16 pipelined_xor_tree_16_inst_3_0 (
	.clk(clk),
	.reset(reset),
	.signal(signals_3_0),
	.reduced_signal(reduced_signals_3_0)
);

pipelined_xor_tree_16 pipelined_xor_tree_16_inst_3_1 (
	.clk(clk),
	.reset(reset),
	.signal(signals_3_1),
	.reduced_signal(reduced_signals_3_1)
);

pipelined_xor_tree_16 pipelined_xor_tree_16_inst_4_0 (
	.clk(clk),
	.reset(reset),
	.signal(signals_4_0),
	.reduced_signal(reduced_signals_4_0)
);

pipelined_xor_tree_16 pipelined_xor_tree_16_inst_4_1 (
	.clk(clk),
	.reset(reset),
	.signal(signals_4_1),
	.reduced_signal(reduced_signals_4_1)
);

pipelined_xor_tree_16 pipelined_xor_tree_16_inst_5_0 (
	.clk(clk),
	.reset(reset),
	.signal(signals_5_0),
	.reduced_signal(reduced_signals_5_0)
);

pipelined_xor_tree_16 pipelined_xor_tree_16_inst_5_1 (
	.clk(clk),
	.reset(reset),
	.signal(signals_5_1),
	.reduced_signal(reduced_signals_5_1)
);

endmodule

module pipelined_xor_tree_16 (
	input clk,
	input reset,
	input [15:0] signal,
	output reduced_signal
);

reg pipeline_0_0;
reg pipeline_0_1;
reg pipeline_0_2;
reg pipeline_1_0;
reg pipeline_1_1;
reg pipeline_1_2;
reg pipeline_2_0;
reg pipeline_2_1;
reg pipeline_2_2;
reg pipeline_3_0;
reg pipeline_3_1;
reg pipeline_3_2;
reg pipeline_4_0;
reg pipeline_4_1;
reg pipeline_4_2;
reg pipeline_5_0;
reg pipeline_5_1;
reg pipeline_5_2;
reg pipeline_6_0;
reg pipeline_6_1;
reg pipeline_6_2;
reg pipeline_7_0;
reg pipeline_7_1;
reg pipeline_7_2;
reg pipeline_8_0;
reg pipeline_8_1;
reg pipeline_8_2;
reg pipeline_9_0;
reg pipeline_9_1;
reg pipeline_9_2;
reg pipeline_10_0;
reg pipeline_10_1;
reg pipeline_10_2;
reg pipeline_11_0;
reg pipeline_11_1;
reg pipeline_11_2;
reg pipeline_12_0;
reg pipeline_12_1;
reg pipeline_12_2;
reg pipeline_13_0;
reg pipeline_13_1;
reg pipeline_13_2;
reg pipeline_14_0;
reg pipeline_14_1;
reg pipeline_14_2;
reg pipeline_15_0;
reg pipeline_15_1;
reg pipeline_15_2;

always @ (posedge clk) begin
	if (reset) begin
		pipeline_0_0 <= 0;
		pipeline_0_1 <= 0;
		pipeline_0_2 <= 0;
		pipeline_1_0 <= 0;
		pipeline_1_1 <= 0;
		pipeline_1_2 <= 0;
		pipeline_2_0 <= 0;
		pipeline_2_1 <= 0;
		pipeline_2_2 <= 0;
		pipeline_3_0 <= 0;
		pipeline_3_1 <= 0;
		pipeline_3_2 <= 0;
		pipeline_4_0 <= 0;
		pipeline_4_1 <= 0;
		pipeline_4_2 <= 0;
		pipeline_5_0 <= 0;
		pipeline_5_1 <= 0;
		pipeline_5_2 <= 0;
		pipeline_6_0 <= 0;
		pipeline_6_1 <= 0;
		pipeline_6_2 <= 0;
		pipeline_7_0 <= 0;
		pipeline_7_1 <= 0;
		pipeline_7_2 <= 0;
		pipeline_8_0 <= 0;
		pipeline_8_1 <= 0;
		pipeline_8_2 <= 0;
		pipeline_9_0 <= 0;
		pipeline_9_1 <= 0;
		pipeline_9_2 <= 0;
		pipeline_10_0 <= 0;
		pipeline_10_1 <= 0;
		pipeline_10_2 <= 0;
		pipeline_11_0 <= 0;
		pipeline_11_1 <= 0;
		pipeline_11_2 <= 0;
		pipeline_12_0 <= 0;
		pipeline_12_1 <= 0;
		pipeline_12_2 <= 0;
		pipeline_13_0 <= 0;
		pipeline_13_1 <= 0;
		pipeline_13_2 <= 0;
		pipeline_14_0 <= 0;
		pipeline_14_1 <= 0;
		pipeline_14_2 <= 0;
		pipeline_15_0 <= 0;
		pipeline_15_1 <= 0;
		pipeline_15_2 <= 0;
	end else begin
		pipeline_0_0 <= signal[15];
		pipeline_1_0 <= signal[14];
		pipeline_2_0 <= signal[13];
		pipeline_3_0 <= signal[12];
		pipeline_4_0 <= signal[11];
		pipeline_5_0 <= signal[10];
		pipeline_6_0 <= signal[9];
		pipeline_7_0 <= signal[8];
		pipeline_8_0 <= signal[7];
		pipeline_9_0 <= signal[6];
		pipeline_10_0 <= signal[5];
		pipeline_11_0 <= signal[4];
		pipeline_12_0 <= signal[3];
		pipeline_13_0 <= signal[2];
		pipeline_14_0 <= signal[1];
		pipeline_15_0 <= signal[0];
		pipeline_0_1 <= pipeline_0_0 ^ pipeline_1_0^ pipeline_2_0 ^ pipeline_3_0;
		pipeline_4_1 <= pipeline_4_0 ^ pipeline_5_0^ pipeline_6_0 ^ pipeline_7_0;
		pipeline_8_1 <= pipeline_8_0 ^ pipeline_9_0^ pipeline_10_0 ^ pipeline_11_0;
		pipeline_12_1 <= pipeline_12_0 ^ pipeline_13_0^ pipeline_14_0 ^ pipeline_15_0;
		pipeline_0_2 <= pipeline_0_1 ^ pipeline_4_1^ pipeline_8_1 ^ pipeline_12_1;
	end
end

assign reduced_signal = pipeline_0_2;

endmodule

module pooling (
	input clk,
	input i_valid,
	input i_reset,
	input [15:0] i_result_0,
	input [15:0] i_result_1,
	input [15:0] i_result_2,
	input [15:0] i_result_3,
	output [15:0] o_result,
	output o_valid
);

reg [15:0] buffer_0_0;
reg [15:0] buffer_0_1;
reg [15:0] buffer_1_0;
reg [15:0] buffer_1_1;
reg [1:0] count;
reg [0:0] s_count;
reg [15:0] result_0;
reg [15:0] result_1;
reg valid_1, valid_2, valid_3;

always @(posedge clk) begin
	buffer_0_1 <= buffer_0_0;
	buffer_1_1 <= buffer_1_0;
	valid_1 <= i_valid;
	valid_2 <= valid_1;
	valid_3 <= valid_2;
	if (i_valid) begin
		count <= 0;
	end else begin
		if(count == 3) begin
			count <= 0;
		end else begin
			count <= count + 1'b1;
		end
		if(i_result_0 > i_result_1) begin
			buffer_0_0 <= i_result_0;
		end else begin
			buffer_0_0 <= i_result_1;
		end
		if(i_result_2 > i_result_3) begin
			buffer_1_0 <= i_result_2;
		end else begin
			buffer_1_0 <= i_result_3;
		end
	end
end

always @(posedge clk) begin
	if (i_reset) begin
		s_count <= 0;
	end else if (valid_1 || valid_2) begin
		if (s_count == 1) begin
			if (buffer_0_0 > buffer_0_1) begin
				result_0 <= buffer_0_0;
			end else begin
				result_0 <= buffer_0_1;
			end
			if (buffer_1_0 > buffer_1_1) begin
				result_1 <= buffer_1_0;
			end else begin
				result_1 <= buffer_1_1;
			end
			s_count <= 0;
		end else begin
			result_0 <= result_1;
			s_count <= s_count + 1'b1;
		end
	end else begin
		s_count <= 0;
	end
end

assign o_result = result_0;
assign o_valid  = valid_3;

endmodule

module store_output (
	input clk,
	input i_valid,
	input i_reset,
	input [15:0] i_result_0,
	input [15:0] i_result_1,
	input [15:0] i_result_2,
	input [15:0] i_result_3,
	input [15:0] i_result_4,
	input [15:0] i_result_5,
	input [15:0] i_result_6,
	input [15:0] i_result_7,
	input [15:0] i_result_8,
	input [15:0] i_result_9,
	input [15:0] i_result_10,
	input [15:0] i_result_11,
	output [15:0] o_store_0_0,
	output [15:0] o_store_0_1,
	output [15:0] o_store_1_0,
	output [15:0] o_store_1_1,
	output [15:0] o_store_2_0,
	output [15:0] o_store_2_1,
	output [15:0] o_store_3_0,
	output [15:0] o_store_3_1,
	output [15:0] o_store_4_0,
	output [15:0] o_store_4_1,
	output [15:0] o_store_5_0,
	output [15:0] o_store_5_1,
	output o_wen_0,
	output o_wen_1,
	output o_wen_2,
	output o_wen_3,
	output o_wen_4,
	output o_wen_5,
	output [14:0] o_addr
);

reg wen_0;
reg wen_1;
reg wen_2;
reg wen_3;
reg wen_4;
reg wen_5;
reg [14:0] base_addr;
reg [14:0] offset;
reg [5:0] count;
reg [5:0] count_to_wvec;
reg [5:0] count_x;
reg [5:0] count_y;
reg valid;
reg [15:0] buffer_reg_0;
reg [15:0] buffer_reg_1;
reg [15:0] buffer_reg_2;
reg [15:0] buffer_reg_3;
reg [15:0] buffer_reg_4;
reg [15:0] buffer_reg_5;
reg [15:0] buffer_reg_6;
reg [15:0] buffer_reg_7;
reg [15:0] buffer_reg_8;
reg [15:0] buffer_reg_9;
reg [15:0] buffer_reg_10;
reg [15:0] buffer_reg_11;
reg [14:0] addr_reg;

wire [5:0] count_div_two;
assign count_div_two = count >> 1;
always @ (posedge clk) begin
	valid <= i_valid;
	buffer_reg_0 <= i_result_0;
	buffer_reg_1 <= i_result_1;
	buffer_reg_2 <= i_result_2;
	buffer_reg_3 <= i_result_3;
	buffer_reg_4 <= i_result_4;
	buffer_reg_5 <= i_result_5;
	buffer_reg_6 <= i_result_6;
	buffer_reg_7 <= i_result_7;
	buffer_reg_8 <= i_result_8;
	buffer_reg_9 <= i_result_9;
	buffer_reg_10 <= i_result_10;
	buffer_reg_11 <= i_result_11;
	addr_reg <= base_addr + offset;
	if (i_reset) begin
		count <= 0;
		count_to_wvec <= 0;
		base_addr <= 0;
		offset <= 0;
		count_x <= 0;
		count_y <= 0;
		wen_0 <= 1'b0;
		wen_1 <= 1'b0;
		wen_2 <= 1'b0;
		wen_3 <= 1'b0;
		wen_4 <= 1'b0;
		wen_5 <= 1'b0;
	end else if (i_valid) begin
		if (count_x == 5) begin
			if(count_y == 6)begin
				base_addr <= base_addr + 16;
				count_y <= 0;
				count_x <= 0;
				offset <= 0;
			end else begin
				if(count[0] == 1'b0) begin
					offset <= 8;
					count <= count + 1'b1;
					if(count_to_wvec == 5) begin
						count_to_wvec <= 0;
					end else begin
						count_to_wvec <= count_to_wvec + 1'b1;
					end
				end else if (count[1] == 1'b1) begin
					offset <= 0;
					base_addr <= base_addr + 16;
					count_x <= 0;
					count_y <= count_y + 2;
					count <= count + 1'b1;
					if(count_to_wvec == 5) begin
						count_to_wvec <= 0;
					end else begin
						count_to_wvec <= count_to_wvec + 1'b1;
					end
				end
			end
		end else if(count[0] == 1'b0) begin
			offset <= 8;
			count <= count + 1'b1;
			if(count_to_wvec == 5) begin
				count_to_wvec <= 0;
			end else begin
				count_to_wvec <= count_to_wvec + 1'b1;
			end
		end else if (count[0] == 1'b1) begin
			offset <= 0;
			count <= count + 1'b1;
			if(count_to_wvec == 5) begin
				count_to_wvec <= 0;
			end else begin
				count_to_wvec <= count_to_wvec + 1'b1;
			end
			count_x <= count_x + 1'b1;
		end
		if ((i_valid || valid) == 1'b1) begin
			if ((count_to_wvec == 0) && i_valid == 1) begin
				wen_0 <= 1'b1;
			end else begin
				wen_0 <= 1'b0;
			end
			if ((count_to_wvec == 1) && i_valid == 1) begin
				wen_1 <= 1'b1;
			end else begin
				wen_1 <= 1'b0;
			end
			if ((count_to_wvec == 2) && i_valid == 1) begin
				wen_2 <= 1'b1;
			end else begin
				wen_2 <= 1'b0;
			end
			if ((count_to_wvec == 3) && i_valid == 1) begin
				wen_3 <= 1'b1;
			end else begin
				wen_3 <= 1'b0;
			end
			if ((count_to_wvec == 4) && i_valid == 1) begin
				wen_4 <= 1'b1;
			end else begin
				wen_4 <= 1'b0;
			end
			if ((count_to_wvec == 5) && i_valid == 1) begin
				wen_5 <= 1'b1;
			end else begin
				wen_5 <= 1'b0;
			end
		end
	end
end

assign o_addr = addr_reg;
assign o_store_0_0 = buffer_reg_0;
assign o_store_1_0 = buffer_reg_2;
assign o_store_2_0 = buffer_reg_4;
assign o_store_3_0 = buffer_reg_6;
assign o_store_4_0 = buffer_reg_8;
assign o_store_5_0 = buffer_reg_10;
assign o_store_0_1 = buffer_reg_1;
assign o_store_1_1 = buffer_reg_3;
assign o_store_2_1 = buffer_reg_5;
assign o_store_3_1 = buffer_reg_7;
assign o_store_4_1 = buffer_reg_9;
assign o_store_5_1 = buffer_reg_11;

assign o_wen_0 = wen_0;
assign o_wen_1 = wen_1;
assign o_wen_2 = wen_2;
assign o_wen_3 = wen_3;
assign o_wen_4 = wen_4;
assign o_wen_5 = wen_5;

endmodule

module stream_buffer_3_1 (
	input clk,
	input i_reset,
	input i_wen0,
	input i_wen1,
	input [15:0] i_ddr,
	input [15:0] i_pool,
	input i_eltwise_sel,
	input [15:0] i_eltwise,
	input [14:0] i_waddr,
	output [15:0] o_feature_0,
	output [15:0] o_feature_1,
	output o_done
);

reg [14:0] base_addr;
reg [14:0] offset;
reg [14:0] base_addr_b1;
reg [14:0] offset_b1;
reg [1:0] L_counter;
reg [1:0] C_counter;
reg [1:0] W_counter;
reg [1:0] L_counter_b1;
reg [1:0] C_counter_b1;
reg [1:0] W_counter_b1;
reg done, done_1, done_2, done_3;
reg valid, valid_1, valid_2;
wire [15:0] feature_out_b0;
wire [15:0] feature_out_b1;

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr <= 0;
		offset <= 0;
		C_counter <= 0;
		L_counter <= 0;
		W_counter <= 0;
	end else if (done == 0) begin
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			base_addr <= 0;
			C_counter <= 0;
			L_counter <= 0;
			W_counter <= 0;
			offset <= 0;
		end else if((C_counter == 1) && (L_counter == 2)) begin
			base_addr <= base_addr + 5;
			W_counter <= W_counter + 1'b1;
			C_counter <= 0;
			L_counter <= 0;
			offset <= 0;
		end else if(L_counter == 2) begin
			base_addr <= base_addr + 1'b1;
			C_counter <= C_counter + 1'b1;
			L_counter <= 0;
			offset <= 0;
		end else begin
			offset <= offset + 2;
			L_counter <= L_counter + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr_b1 <= 0;
		offset_b1 <= 0;
		C_counter_b1 <= 0;
		L_counter_b1 <= 0;
		W_counter_b1 <= 0;
	end else if (done == 0) begin
		if((W_counter_b1 == 1443) && (C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= 0;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			W_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if((C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= base_addr_b1 + 5;
			W_counter_b1 <= W_counter_b1 + 1'b1;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if(L_counter_b1 == 2) begin
			base_addr_b1 <= base_addr_b1 + 1'b1;
			C_counter_b1 <= C_counter_b1 + 1'b1;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else begin
			offset_b1 <= offset_b1 + 2;
			L_counter_b1 <= L_counter_b1 + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if(i_reset == 1'b1)begin
		done <= 0;
		done_1 <= 0;
		done_2 <= 0;
		done_3 <= 0;
		valid <= 0;
		valid_1 <= 0;
		valid_2 <= 0;
	end else begin
		valid <= 1;
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			done <= 1;
		end
		done_1 <= done;
		done_2 <= done_1;
		done_3 <= done_2;
		valid_1 <= valid;
		valid_2 <= valid_1;
	end
end

reg [14:0] b0_waddr, b0_raddr, b1_raddr;
always @ (*) begin
	b0_waddr <= base_addr+offset;
	b0_raddr <= base_addr+offset;
	b1_raddr <= base_addr_b1+offset_b1;
end
buffer_16_24200_buffer_init_13 buffer_16_24200_buffer_init_13_B0 (
	.clk(clk),
	.wen(i_wen0),
	.waddr(b0_waddr),
	.wdata(i_ddr),
	.raddr(b0_raddr),
	.rdata(feature_out_b0)
);

reg [15:0] B1_wdata;
always @ (*) begin
	if (i_eltwise_sel) begin
		B1_wdata <= i_eltwise;
	end else begin
		B1_wdata <= i_pool;
	end
end

buffer_16_24200_buffer_init_13 buffer_16_24200_buffer_init_13_B1 (
	.clk(clk),
	.wen(i_wen1),
	.waddr(i_waddr),
	.wdata(B1_wdata),
	.raddr(b1_raddr),
	.rdata(feature_out_b1)
);

assign o_done = valid_2 && (~done_3);
assign o_feature_0 = feature_out_b0;
assign o_feature_1 = feature_out_b1;

endmodule

module buffer_16_24200_buffer_init_13 (
	input clk,
	input wen,
	input [14:0] waddr,
	input [15:0] wdata,
	input [14:0] raddr,
	output [15:0] rdata
);

reg [14:0] raddr_reg;
reg [15:0] rdata_reg;
reg [15:0] pipeline_reg_0;
wire [15:0] rd_dummy_signal;
wire [15:0] wr_dummy_signal;
wire [15:0] rdata_wire;
assign rd_dummy_signal = 0;

always @(posedge clk) begin
	rdata_reg <= rdata_wire;
	raddr_reg <= raddr;
	pipeline_reg_0 <= rdata_reg;
end

dual_port_ram u_dual_port_ram(
	.addr1(waddr),
	.we1(wen),
	.data1(wdata),
	.out1(wr_dummy_signal),
	.addr2(raddr_reg),
	.we2(1'b0),
	.data2(rd_dummy_signal),
	.out2(rdata_wire),
	.clk(clk)
);
assign rdata = pipeline_reg_0;

endmodule

module stream_buffer_3_0 (
	input clk,
	input i_reset,
	input i_wen0,
	input i_wen1,
	input [15:0] i_ddr,
	input [15:0] i_pool,
	input i_eltwise_sel,
	input [15:0] i_eltwise,
	input [14:0] i_waddr,
	output [15:0] o_feature_0,
	output [15:0] o_feature_1,
	output o_done
);

reg [14:0] base_addr;
reg [14:0] offset;
reg [14:0] base_addr_b1;
reg [14:0] offset_b1;
reg [1:0] L_counter;
reg [1:0] C_counter;
reg [1:0] W_counter;
reg [1:0] L_counter_b1;
reg [1:0] C_counter_b1;
reg [1:0] W_counter_b1;
reg done, done_1, done_2, done_3;
reg valid, valid_1, valid_2;
wire [15:0] feature_out_b0;
wire [15:0] feature_out_b1;

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr <= 0;
		offset <= 0;
		C_counter <= 0;
		L_counter <= 0;
		W_counter <= 0;
	end else if (done == 0) begin
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			base_addr <= 0;
			C_counter <= 0;
			L_counter <= 0;
			W_counter <= 0;
			offset <= 0;
		end else if((C_counter == 1) && (L_counter == 2)) begin
			base_addr <= base_addr + 5;
			W_counter <= W_counter + 1'b1;
			C_counter <= 0;
			L_counter <= 0;
			offset <= 0;
		end else if(L_counter == 2) begin
			base_addr <= base_addr + 1'b1;
			C_counter <= C_counter + 1'b1;
			L_counter <= 0;
			offset <= 0;
		end else begin
			offset <= offset + 2;
			L_counter <= L_counter + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if (i_reset) begin
		base_addr_b1 <= 0;
		offset_b1 <= 0;
		C_counter_b1 <= 0;
		L_counter_b1 <= 0;
		W_counter_b1 <= 0;
	end else if (done == 0) begin
		if((W_counter_b1 == 1443) && (C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= 0;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			W_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if((C_counter_b1 == 1) && (L_counter_b1 == 2)) begin
			base_addr_b1 <= base_addr_b1 + 5;
			W_counter_b1 <= W_counter_b1 + 1'b1;
			C_counter_b1 <= 0;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else if(L_counter_b1 == 2) begin
			base_addr_b1 <= base_addr_b1 + 1'b1;
			C_counter_b1 <= C_counter_b1 + 1'b1;
			L_counter_b1 <= 0;
			offset_b1 <= 0;
		end else begin
			offset_b1 <= offset_b1 + 2;
			L_counter_b1 <= L_counter_b1 + 1'b1;
		end
	end
end

always @ (posedge clk) begin
	if(i_reset == 1'b1)begin
		done <= 0;
		done_1 <= 0;
		done_2 <= 0;
		done_3 <= 0;
		valid <= 0;
		valid_1 <= 0;
		valid_2 <= 0;
	end else begin
		valid <= 1;
		if((W_counter == 1443) && (C_counter == 1) && (L_counter == 2)) begin
			done <= 1;
		end
		done_1 <= done;
		done_2 <= done_1;
		done_3 <= done_2;
		valid_1 <= valid;
		valid_2 <= valid_1;
	end
end

reg [14:0] b0_waddr, b0_raddr, b1_raddr;
always @ (*) begin
	b0_waddr <= base_addr+offset;
	b0_raddr <= base_addr+offset;
	b1_raddr <= base_addr_b1+offset_b1;
end
buffer_16_24200_buffer_init_03 buffer_16_24200_buffer_init_03_B0 (
	.clk(clk),
	.wen(i_wen0),
	.waddr(b0_waddr),
	.wdata(i_ddr),
	.raddr(b0_raddr),
	.rdata(feature_out_b0)
);

reg [15:0] B1_wdata;
always @ (*) begin
	if (i_eltwise_sel) begin
		B1_wdata <= i_eltwise;
	end else begin
		B1_wdata <= i_pool;
	end
end

buffer_16_24200_buffer_init_03 buffer_16_24200_buffer_init_03_B1 (
	.clk(clk),
	.wen(i_wen1),
	.waddr(i_waddr),
	.wdata(B1_wdata),
	.raddr(b1_raddr),
	.rdata(feature_out_b1)
);

assign o_done = valid_2 && (~done_3);
assign o_feature_0 = feature_out_b0;
assign o_feature_1 = feature_out_b1;

endmodule

module buffer_16_24200_buffer_init_03 (
	input clk,
	input wen,
	input [14:0] waddr,
	input [15:0] wdata,
	input [14:0] raddr,
	output [15:0] rdata
);

reg [14:0] raddr_reg;
reg [15:0] rdata_reg;
reg [15:0] pipeline_reg_0;
wire [15:0] rd_dummy_signal;
wire [15:0] wr_dummy_signal;
wire [15:0] rdata_wire;
assign rd_dummy_signal = 0;

always @(posedge clk) begin
	rdata_reg <= rdata_wire;
	raddr_reg <= raddr;
	pipeline_reg_0 <= rdata_reg;
end

dual_port_ram u_dual_port_ram(
	.addr1(waddr),
	.we1(wen),
	.data1(wdata),
	.out1(wr_dummy_signal),
	.addr2(raddr_reg),
	.we2(1'b0),
	.data2(rd_dummy_signal),
	.out2(rdata_wire),
	.clk(clk)
);
assign rdata = pipeline_reg_0;

endmodule


