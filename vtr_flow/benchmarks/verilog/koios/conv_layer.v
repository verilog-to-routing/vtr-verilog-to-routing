//////////////////////////////////////////////////////////////////////////////
// Convolution layer accelerator design. 
// 1. 16-bit integer precision is used
// 2. Convolution lowered into matrix multiplication. Uses tiling and elementwise addition of partial products.
// 3. An 8x8 fp16 input image with 3 channels, padding=1, stride=1, filter size = 3x3 and batch size=2. 
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Author: Aman Arora
//////////////////////////////////////////////////////////////////////////////

`timescale 1ns/1ns
//`define ARCH_SPECIFIC_IMP 1
`define DWIDTH 16
`define AWIDTH 10
`define MEM_SIZE 1024
`define DESIGN_SIZE 8
`define MAT_MUL_SIZE 4
`define MASK_WIDTH 4
`define LOG2_MAT_MUL_SIZE 2
`define NUM_CYCLES_IN_MAC 3
`define MEM_ACCESS_LATENCY 1
`define REG_DATAWIDTH 32
`define REG_ADDRWIDTH 8
`define ADDR_STRIDE_WIDTH 16
`define REG_STDN_TPU_ADDR 32'h4
`define REG_MATRIX_A_ADDR 32'he
`define REG_MATRIX_B_ADDR 32'h12
`define REG_MATRIX_C_ADDR 32'h16
`define REG_VALID_MASK_A_ROWS_ADDR 32'h20
`define REG_VALID_MASK_A_COLS_ADDR 32'h54
`define REG_VALID_MASK_B_ROWS_ADDR 32'h5c
`define REG_VALID_MASK_B_COLS_ADDR 32'h58
`define REG_MATRIX_A_STRIDE_ADDR 32'h28
`define REG_MATRIX_B_STRIDE_ADDR 32'h32
`define REG_MATRIX_C_STRIDE_ADDR 32'h36
`define ADDRESS_BASE_A 10'd0
`define ADDRESS_BASE_B 10'd0
`define ADDRESS_BASE_C 10'd0

module conv(
  input clk,
  input clk_mem,
  input resetn,
  input pe_resetn,
  input start,
  output reg done,
  input  [7:0] bram_select,
  input  [`AWIDTH-1:0] bram_addr_ext,
  output reg [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_ext,
  input  [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_ext,
  input  [`MAT_MUL_SIZE-1:0] bram_we_ext
);


  wire PCLK;
  assign PCLK = clk;
  //Dummy register to sync all other invalid/unimplemented addresses
  reg [`REG_DATAWIDTH-1:0] reg_dummy;

wire reset;
assign reset = ~resetn;
wire pe_reset;
assign pe_reset = ~pe_resetn;


  reg pe_reset_0;	
  reg start_mat_mul_0;
  wire done_mat_mul_0;
  reg [`AWIDTH-1:0] address_mat_a_0;
  reg [`AWIDTH-1:0] address_mat_b_0;
  reg [`AWIDTH-1:0] address_mat_c_0;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_a_0;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_b_0;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_c_0;
  wire [3:0] flags_NC_0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_in_0_NC;
  assign a_data_in_0_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_in_0_NC;
  assign b_data_in_0_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_in_0_NC;
  assign c_data_in_0_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_out_0_NC;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_out_0_NC;
  wire [`AWIDTH-1:0] a_addr_0;
  wire [`AWIDTH-1:0] b_addr_0;
  wire [`AWIDTH-1:0] c_addr_0;
  wire c_data_0_available;
  reg [3:0] validity_mask_a_0_rows;
  reg [3:0] validity_mask_a_0_cols;
  reg [3:0] validity_mask_b_0_rows;
  reg [3:0] validity_mask_b_0_cols;
  
  

  reg pe_reset_1;	
  reg start_mat_mul_1;
  wire done_mat_mul_1;
  reg [`AWIDTH-1:0] address_mat_a_1;
  reg [`AWIDTH-1:0] address_mat_b_1;
  reg [`AWIDTH-1:0] address_mat_c_1;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_a_1;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_b_1;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_c_1;
  wire [3:0] flags_NC_1;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_1;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_1;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_1;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_in_1_NC;
  assign a_data_in_1_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_in_1_NC;
  assign b_data_in_1_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_in_1_NC;
  assign c_data_in_1_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_out_1_NC;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_out_1_NC;
  wire [`AWIDTH-1:0] a_addr_1;
  wire [`AWIDTH-1:0] b_addr_1;
  wire [`AWIDTH-1:0] c_addr_1;
  wire c_data_1_available;
  reg [3:0] validity_mask_a_1_rows;
  reg [3:0] validity_mask_a_1_cols;
  reg [3:0] validity_mask_b_1_rows;
  reg [3:0] validity_mask_b_1_cols;
  
  

  reg pe_reset_2;	
  reg start_mat_mul_2;
  wire done_mat_mul_2;
  reg [`AWIDTH-1:0] address_mat_a_2;
  reg [`AWIDTH-1:0] address_mat_b_2;
  reg [`AWIDTH-1:0] address_mat_c_2;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_a_2;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_b_2;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_c_2;
  wire [3:0] flags_NC_2;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_2;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_2;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_2;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_in_2_NC;
  assign a_data_in_2_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_in_2_NC;
  assign b_data_in_2_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_in_2_NC;
  assign c_data_in_2_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_out_2_NC;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_out_2_NC;
  wire [`AWIDTH-1:0] a_addr_2;
  wire [`AWIDTH-1:0] b_addr_2;
  wire [`AWIDTH-1:0] c_addr_2;
  wire c_data_2_available;
  reg [3:0] validity_mask_a_2_rows;
  reg [3:0] validity_mask_a_2_cols;
  reg [3:0] validity_mask_b_2_rows;
  reg [3:0] validity_mask_b_2_cols;
  
  

  reg pe_reset_3;	
  reg start_mat_mul_3;
  wire done_mat_mul_3;
  reg [`AWIDTH-1:0] address_mat_a_3;
  reg [`AWIDTH-1:0] address_mat_b_3;
  reg [`AWIDTH-1:0] address_mat_c_3;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_a_3;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_b_3;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_c_3;
  wire [3:0] flags_NC_3;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_3;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_3;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_3;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_in_3_NC;
  assign a_data_in_3_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_in_3_NC;
  assign b_data_in_3_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_in_3_NC;
  assign c_data_in_3_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_out_3_NC;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_out_3_NC;
  wire [`AWIDTH-1:0] a_addr_3;
  wire [`AWIDTH-1:0] b_addr_3;
  wire [`AWIDTH-1:0] c_addr_3;
  wire c_data_3_available;
  reg [3:0] validity_mask_a_3_rows;
  reg [3:0] validity_mask_a_3_cols;
  reg [3:0] validity_mask_b_3_rows;
  reg [3:0] validity_mask_b_3_cols;
  
  

  reg pe_reset_4;	
  reg start_mat_mul_4;
  wire done_mat_mul_4;
  reg [`AWIDTH-1:0] address_mat_a_4;
  reg [`AWIDTH-1:0] address_mat_b_4;
  reg [`AWIDTH-1:0] address_mat_c_4;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_a_4;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_b_4;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_c_4;
  wire [3:0] flags_NC_4;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_4;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_4;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_4;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_in_4_NC;
  assign a_data_in_4_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_in_4_NC;
  assign b_data_in_4_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_in_4_NC;
  assign c_data_in_4_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_out_4_NC;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_out_4_NC;
  wire [`AWIDTH-1:0] a_addr_4;
  wire [`AWIDTH-1:0] b_addr_4;
  wire [`AWIDTH-1:0] c_addr_4;
  wire c_data_4_available;
  reg [3:0] validity_mask_a_4_rows;
  reg [3:0] validity_mask_a_4_cols;
  reg [3:0] validity_mask_b_4_rows;
  reg [3:0] validity_mask_b_4_cols;
  
  

  reg pe_reset_5;	
  reg start_mat_mul_5;
  wire done_mat_mul_5;
  reg [`AWIDTH-1:0] address_mat_a_5;
  reg [`AWIDTH-1:0] address_mat_b_5;
  reg [`AWIDTH-1:0] address_mat_c_5;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_a_5;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_b_5;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_c_5;
  wire [3:0] flags_NC_5;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_5;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_5;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_5;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_in_5_NC;
  assign a_data_in_5_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_in_5_NC;
  assign b_data_in_5_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_in_5_NC;
  assign c_data_in_5_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_out_5_NC;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_out_5_NC;
  wire [`AWIDTH-1:0] a_addr_5;
  wire [`AWIDTH-1:0] b_addr_5;
  wire [`AWIDTH-1:0] c_addr_5;
  wire c_data_5_available;
  reg [3:0] validity_mask_a_5_rows;
  reg [3:0] validity_mask_a_5_cols;
  reg [3:0] validity_mask_b_5_rows;
  reg [3:0] validity_mask_b_5_cols;
  
  

  reg pe_reset_6;	
  reg start_mat_mul_6;
  wire done_mat_mul_6;
  reg [`AWIDTH-1:0] address_mat_a_6;
  reg [`AWIDTH-1:0] address_mat_b_6;
  reg [`AWIDTH-1:0] address_mat_c_6;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_a_6;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_b_6;
  reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_c_6;
  wire [3:0] flags_NC_6;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_6;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_6;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_6;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_in_6_NC;
  assign a_data_in_6_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_in_6_NC;
  assign b_data_in_6_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_in_6_NC;
  assign c_data_in_6_NC = 0;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_out_6_NC;
  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_out_6_NC;
  wire [`AWIDTH-1:0] a_addr_6;
  wire [`AWIDTH-1:0] b_addr_6;
  wire [`AWIDTH-1:0] c_addr_6;
  wire c_data_6_available;
  reg [3:0] validity_mask_a_6_rows;
  reg [3:0] validity_mask_a_6_cols;
  reg [3:0] validity_mask_b_6_rows;
  reg [3:0] validity_mask_b_6_cols;
  
  

    reg [`AWIDTH-1:0] bram_addr_a_0_ext;
    wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_a_0_ext;
    reg [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_a_0_ext;
    reg [`MASK_WIDTH-1:0] bram_we_a_0_ext;
    
	  wire [`AWIDTH-1:0] bram_addr_a_0;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_a_0;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_a_0;
	  wire [`MASK_WIDTH-1:0] bram_we_a_0;
	  wire bram_en_a_0;

    reg [`AWIDTH-1:0] bram_addr_b_0_ext;
    wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_b_0_ext;
    reg [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_b_0_ext;
    reg [`MASK_WIDTH-1:0] bram_we_b_0_ext;
    
	  wire [`AWIDTH-1:0] bram_addr_b_0;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_b_0;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_b_0;
	  wire [`MASK_WIDTH-1:0] bram_we_b_0;
	  wire bram_en_b_0;

    

    reg [`AWIDTH-1:0] bram_addr_a_1_ext;
    wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_a_1_ext;
    reg [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_a_1_ext;
    reg [`MASK_WIDTH-1:0] bram_we_a_1_ext;
    
	  wire [`AWIDTH-1:0] bram_addr_a_1;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_a_1;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_a_1;
	  wire [`MASK_WIDTH-1:0] bram_we_a_1;
	  wire bram_en_a_1;

    reg [`AWIDTH-1:0] bram_addr_b_1_ext;
    wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_b_1_ext;
    reg [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_b_1_ext;
    reg [`MASK_WIDTH-1:0] bram_we_b_1_ext;
    
	  wire [`AWIDTH-1:0] bram_addr_b_1;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_b_1;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_b_1;
	  wire [`MASK_WIDTH-1:0] bram_we_b_1;
	  wire bram_en_b_1;

    

    reg [`AWIDTH-1:0] bram_addr_a_2_ext;
    wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_a_2_ext;
    reg [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_a_2_ext;
    reg [`MASK_WIDTH-1:0] bram_we_a_2_ext;
    
	  wire [`AWIDTH-1:0] bram_addr_a_2;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_a_2;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_a_2;
	  wire [`MASK_WIDTH-1:0] bram_we_a_2;
	  wire bram_en_a_2;

    reg [`AWIDTH-1:0] bram_addr_b_2_ext;
    wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_b_2_ext;
    reg [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_b_2_ext;
    reg [`MASK_WIDTH-1:0] bram_we_b_2_ext;
    
	  wire [`AWIDTH-1:0] bram_addr_b_2;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_b_2;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_b_2;
	  wire [`MASK_WIDTH-1:0] bram_we_b_2;
	  wire bram_en_b_2;

    

    reg [`AWIDTH-1:0] bram_addr_a_3_ext;
    wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_a_3_ext;
    reg [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_a_3_ext;
    reg [`MASK_WIDTH-1:0] bram_we_a_3_ext;
    
	  wire [`AWIDTH-1:0] bram_addr_a_3;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_a_3;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_a_3;
	  wire [`MASK_WIDTH-1:0] bram_we_a_3;
	  wire bram_en_a_3;

    reg [`AWIDTH-1:0] bram_addr_b_3_ext;
    wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_b_3_ext;
    reg [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_b_3_ext;
    reg [`MASK_WIDTH-1:0] bram_we_b_3_ext;
    
	  wire [`AWIDTH-1:0] bram_addr_b_3;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_b_3;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_b_3;
	  wire [`MASK_WIDTH-1:0] bram_we_b_3;
	  wire bram_en_b_3;

    

    reg [`AWIDTH-1:0] bram_addr_a_4_ext;
    wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_a_4_ext;
    reg [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_a_4_ext;
    reg [`MASK_WIDTH-1:0] bram_we_a_4_ext;
    
	  wire [`AWIDTH-1:0] bram_addr_a_4;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_a_4;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_a_4;
	  wire [`MASK_WIDTH-1:0] bram_we_a_4;
	  wire bram_en_a_4;

    reg [`AWIDTH-1:0] bram_addr_b_4_ext;
    wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_b_4_ext;
    reg [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_b_4_ext;
    reg [`MASK_WIDTH-1:0] bram_we_b_4_ext;
    
	  wire [`AWIDTH-1:0] bram_addr_b_4;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_b_4;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_b_4;
	  wire [`MASK_WIDTH-1:0] bram_we_b_4;
	  wire bram_en_b_4;

    

    reg [`AWIDTH-1:0] bram_addr_a_5_ext;
    wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_a_5_ext;
    reg [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_a_5_ext;
    reg [`MASK_WIDTH-1:0] bram_we_a_5_ext;
    
	  wire [`AWIDTH-1:0] bram_addr_a_5;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_a_5;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_a_5;
	  wire [`MASK_WIDTH-1:0] bram_we_a_5;
	  wire bram_en_a_5;

    reg [`AWIDTH-1:0] bram_addr_b_5_ext;
    wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_b_5_ext;
    reg [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_b_5_ext;
    reg [`MASK_WIDTH-1:0] bram_we_b_5_ext;
    
	  wire [`AWIDTH-1:0] bram_addr_b_5;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_b_5;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_b_5;
	  wire [`MASK_WIDTH-1:0] bram_we_b_5;
	  wire bram_en_b_5;

    

    reg [`AWIDTH-1:0] bram_addr_a_6_ext;
    wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_a_6_ext;
    reg [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_a_6_ext;
    reg [`MASK_WIDTH-1:0] bram_we_a_6_ext;
    
	  wire [`AWIDTH-1:0] bram_addr_a_6;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_a_6;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_a_6;
	  wire [`MASK_WIDTH-1:0] bram_we_a_6;
	  wire bram_en_a_6;

    reg [`AWIDTH-1:0] bram_addr_b_6_ext;
    wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_b_6_ext;
    reg [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_b_6_ext;
    reg [`MASK_WIDTH-1:0] bram_we_b_6_ext;
    
	  wire [`AWIDTH-1:0] bram_addr_b_6;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_rdata_b_6;
	  wire [`MAT_MUL_SIZE*`DWIDTH-1:0] bram_wdata_b_6;
	  wire [`MASK_WIDTH-1:0] bram_we_b_6;
	  wire bram_en_b_6;

    

  always @* begin
    case (bram_select)


      0: begin
      bram_addr_a_0_ext = bram_addr_ext;
      bram_wdata_a_0_ext = bram_wdata_ext;
      bram_we_a_0_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_a_0_ext;
      end
    

      1: begin
      bram_addr_b_0_ext = bram_addr_ext;
      bram_wdata_b_0_ext = bram_wdata_ext;
      bram_we_b_0_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_b_0_ext;
      end
    

      2: begin
      bram_addr_a_1_ext = bram_addr_ext;
      bram_wdata_a_1_ext = bram_wdata_ext;
      bram_we_a_1_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_a_1_ext;
      end
    

      3: begin
      bram_addr_b_1_ext = bram_addr_ext;
      bram_wdata_b_1_ext = bram_wdata_ext;
      bram_we_b_1_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_b_1_ext;
      end
    

      4: begin
      bram_addr_a_2_ext = bram_addr_ext;
      bram_wdata_a_2_ext = bram_wdata_ext;
      bram_we_a_2_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_a_2_ext;
      end
    

      5: begin
      bram_addr_b_2_ext = bram_addr_ext;
      bram_wdata_b_2_ext = bram_wdata_ext;
      bram_we_b_2_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_b_2_ext;
      end
    

      6: begin
      bram_addr_a_3_ext = bram_addr_ext;
      bram_wdata_a_3_ext = bram_wdata_ext;
      bram_we_a_3_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_a_3_ext;
      end
    

      7: begin
      bram_addr_b_3_ext = bram_addr_ext;
      bram_wdata_b_3_ext = bram_wdata_ext;
      bram_we_b_3_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_b_3_ext;
      end
    

      8: begin
      bram_addr_a_4_ext = bram_addr_ext;
      bram_wdata_a_4_ext = bram_wdata_ext;
      bram_we_a_4_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_a_4_ext;
      end
    

      9: begin
      bram_addr_b_4_ext = bram_addr_ext;
      bram_wdata_b_4_ext = bram_wdata_ext;
      bram_we_b_4_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_b_4_ext;
      end
    

      10: begin
      bram_addr_a_5_ext = bram_addr_ext;
      bram_wdata_a_5_ext = bram_wdata_ext;
      bram_we_a_5_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_a_5_ext;
      end
    

      11: begin
      bram_addr_b_5_ext = bram_addr_ext;
      bram_wdata_b_5_ext = bram_wdata_ext;
      bram_we_b_5_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_b_5_ext;
      end
    

      12: begin
      bram_addr_a_6_ext = bram_addr_ext;
      bram_wdata_a_6_ext = bram_wdata_ext;
      bram_we_a_6_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_a_6_ext;
      end
    

      13: begin
      bram_addr_b_6_ext = bram_addr_ext;
      bram_wdata_b_6_ext = bram_wdata_ext;
      bram_we_b_6_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_b_6_ext;
      end
    

      default: begin
      bram_rdata_ext = 0;
      end
    endcase 
  end
   

  ram matrix_A_0(
    .addr0(bram_addr_a_0),
    .d0(bram_wdata_a_0), 
    .we0(bram_we_a_0), 
    .q0(a_data_0), 
    .addr1(bram_addr_a_0_ext),
    .d1(bram_wdata_a_0_ext), 
    .we1(bram_we_a_0_ext), 
    .q1(bram_rdata_a_0_ext), 
    .clk(clk_mem));

  ram matrix_B_0(
    .addr0(b_addr_0),
    .d0(bram_wdata_b_0), 
    .we0(bram_we_b_0), 
    .q0(b_data_0), 
    .addr1(bram_addr_b_0_ext),
    .d1(bram_wdata_b_0_ext), 
    .we1(bram_we_b_0_ext), 
    .q1(bram_rdata_b_0_ext), 
    .clk(clk_mem));

    

  ram matrix_A_1(
    .addr0(bram_addr_a_1),
    .d0(bram_wdata_a_1), 
    .we0(bram_we_a_1), 
    .q0(a_data_1), 
    .addr1(bram_addr_a_1_ext),
    .d1(bram_wdata_a_1_ext), 
    .we1(bram_we_a_1_ext), 
    .q1(bram_rdata_a_1_ext), 
    .clk(clk_mem));

  ram matrix_B_1(
    .addr0(b_addr_1),
    .d0(bram_wdata_b_1), 
    .we0(bram_we_b_1), 
    .q0(b_data_1), 
    .addr1(bram_addr_b_1_ext),
    .d1(bram_wdata_b_1_ext), 
    .we1(bram_we_b_1_ext), 
    .q1(bram_rdata_b_1_ext), 
    .clk(clk_mem));

    

  ram matrix_A_2(
    .addr0(bram_addr_a_2),
    .d0(bram_wdata_a_2), 
    .we0(bram_we_a_2), 
    .q0(a_data_2), 
    .addr1(bram_addr_a_2_ext),
    .d1(bram_wdata_a_2_ext), 
    .we1(bram_we_a_2_ext), 
    .q1(bram_rdata_a_2_ext), 
    .clk(clk_mem));

  ram matrix_B_2(
    .addr0(b_addr_2),
    .d0(bram_wdata_b_2), 
    .we0(bram_we_b_2), 
    .q0(b_data_2), 
    .addr1(bram_addr_b_2_ext),
    .d1(bram_wdata_b_2_ext), 
    .we1(bram_we_b_2_ext), 
    .q1(bram_rdata_b_2_ext), 
    .clk(clk_mem));

    

  ram matrix_A_3(
    .addr0(bram_addr_a_3),
    .d0(bram_wdata_a_3), 
    .we0(bram_we_a_3), 
    .q0(a_data_3), 
    .addr1(bram_addr_a_3_ext),
    .d1(bram_wdata_a_3_ext), 
    .we1(bram_we_a_3_ext), 
    .q1(bram_rdata_a_3_ext), 
    .clk(clk_mem));

  ram matrix_B_3(
    .addr0(b_addr_3),
    .d0(bram_wdata_b_3), 
    .we0(bram_we_b_3), 
    .q0(b_data_3), 
    .addr1(bram_addr_b_3_ext),
    .d1(bram_wdata_b_3_ext), 
    .we1(bram_we_b_3_ext), 
    .q1(bram_rdata_b_3_ext), 
    .clk(clk_mem));

    

  ram matrix_A_4(
    .addr0(bram_addr_a_4),
    .d0(bram_wdata_a_4), 
    .we0(bram_we_a_4), 
    .q0(a_data_4), 
    .addr1(bram_addr_a_4_ext),
    .d1(bram_wdata_a_4_ext), 
    .we1(bram_we_a_4_ext), 
    .q1(bram_rdata_a_4_ext), 
    .clk(clk_mem));

  ram matrix_B_4(
    .addr0(b_addr_4),
    .d0(bram_wdata_b_4), 
    .we0(bram_we_b_4), 
    .q0(b_data_4), 
    .addr1(bram_addr_b_4_ext),
    .d1(bram_wdata_b_4_ext), 
    .we1(bram_we_b_4_ext), 
    .q1(bram_rdata_b_4_ext), 
    .clk(clk_mem));

    

  ram matrix_A_5(
    .addr0(bram_addr_a_5),
    .d0(bram_wdata_a_5), 
    .we0(bram_we_a_5), 
    .q0(a_data_5), 
    .addr1(bram_addr_a_5_ext),
    .d1(bram_wdata_a_5_ext), 
    .we1(bram_we_a_5_ext), 
    .q1(bram_rdata_a_5_ext), 
    .clk(clk_mem));

  ram matrix_B_5(
    .addr0(b_addr_5),
    .d0(bram_wdata_b_5), 
    .we0(bram_we_b_5), 
    .q0(b_data_5), 
    .addr1(bram_addr_b_5_ext),
    .d1(bram_wdata_b_5_ext), 
    .we1(bram_we_b_5_ext), 
    .q1(bram_rdata_b_5_ext), 
    .clk(clk_mem));

    

  ram matrix_A_6(
    .addr0(bram_addr_a_6),
    .d0(bram_wdata_a_6), 
    .we0(bram_we_a_6), 
    .q0(a_data_6), 
    .addr1(bram_addr_a_6_ext),
    .d1(bram_wdata_a_6_ext), 
    .we1(bram_we_a_6_ext), 
    .q1(bram_rdata_a_6_ext), 
    .clk(clk_mem));

  ram matrix_B_6(
    .addr0(b_addr_6),
    .d0(bram_wdata_b_6), 
    .we0(bram_we_b_6), 
    .q0(b_data_6), 
    .addr1(bram_addr_b_6_ext),
    .d1(bram_wdata_b_6_ext), 
    .we1(bram_we_b_6_ext), 
    .q1(bram_rdata_b_6_ext), 
    .clk(clk_mem));

    


  assign bram_wdata_a_0 = c_data_0;
  assign bram_en_a_0 = 1'b1;
  assign bram_we_a_0 = (c_data_0_available) ? {`MASK_WIDTH{1'b1}} : {`MASK_WIDTH{1'b0}};  
  assign bram_addr_a_0 = (c_data_0_available) ? c_addr_0 : a_addr_0;

  assign bram_wdata_b_0 = {`MAT_MUL_SIZE*`DWIDTH{1'b0}};
  assign bram_en_b_0 = 1'b1;
  assign bram_we_b_0 = {`MASK_WIDTH{1'b0}};
  


  assign bram_wdata_a_1 = c_data_1;
  assign bram_en_a_1 = 1'b1;
  assign bram_we_a_1 = (c_data_1_available) ? {`MASK_WIDTH{1'b1}} : {`MASK_WIDTH{1'b0}};  
  assign bram_addr_a_1 = (c_data_1_available) ? c_addr_1 : a_addr_1;

  assign bram_wdata_b_1 = {`MAT_MUL_SIZE*`DWIDTH{1'b0}};
  assign bram_en_b_1 = 1'b1;
  assign bram_we_b_1 = {`MASK_WIDTH{1'b0}};
  


  assign bram_wdata_a_2 = c_data_2;
  assign bram_en_a_2 = 1'b1;
  assign bram_we_a_2 = (c_data_2_available) ? {`MASK_WIDTH{1'b1}} : {`MASK_WIDTH{1'b0}};  
  assign bram_addr_a_2 = (c_data_2_available) ? c_addr_2 : a_addr_2;

  assign bram_wdata_b_2 = {`MAT_MUL_SIZE*`DWIDTH{1'b0}};
  assign bram_en_b_2 = 1'b1;
  assign bram_we_b_2 = {`MASK_WIDTH{1'b0}};
  


  assign bram_wdata_a_3 = c_data_3;
  assign bram_en_a_3 = 1'b1;
  assign bram_we_a_3 = (c_data_3_available) ? {`MASK_WIDTH{1'b1}} : {`MASK_WIDTH{1'b0}};  
  assign bram_addr_a_3 = (c_data_3_available) ? c_addr_3 : a_addr_3;

  assign bram_wdata_b_3 = {`MAT_MUL_SIZE*`DWIDTH{1'b0}};
  assign bram_en_b_3 = 1'b1;
  assign bram_we_b_3 = {`MASK_WIDTH{1'b0}};
  


  assign bram_wdata_a_4 = c_data_4;
  assign bram_en_a_4 = 1'b1;
  assign bram_we_a_4 = (c_data_4_available) ? {`MASK_WIDTH{1'b1}} : {`MASK_WIDTH{1'b0}};  
  assign bram_addr_a_4 = (c_data_4_available) ? c_addr_4 : a_addr_4;

  assign bram_wdata_b_4 = {`MAT_MUL_SIZE*`DWIDTH{1'b0}};
  assign bram_en_b_4 = 1'b1;
  assign bram_we_b_4 = {`MASK_WIDTH{1'b0}};
  


  assign bram_wdata_a_5 = c_data_5;
  assign bram_en_a_5 = 1'b1;
  assign bram_we_a_5 = (c_data_5_available) ? {`MASK_WIDTH{1'b1}} : {`MASK_WIDTH{1'b0}};  
  assign bram_addr_a_5 = (c_data_5_available) ? c_addr_5 : a_addr_5;

  assign bram_wdata_b_5 = {`MAT_MUL_SIZE*`DWIDTH{1'b0}};
  assign bram_en_b_5 = 1'b1;
  assign bram_we_b_5 = {`MASK_WIDTH{1'b0}};
  


  assign bram_wdata_a_6 = c_data_6;
  assign bram_en_a_6 = 1'b1;
  assign bram_we_a_6 = (c_data_6_available) ? {`MASK_WIDTH{1'b1}} : {`MASK_WIDTH{1'b0}};  
  assign bram_addr_a_6 = (c_data_6_available) ? c_addr_6 : a_addr_6;

  assign bram_wdata_b_6 = {`MAT_MUL_SIZE*`DWIDTH{1'b0}};
  assign bram_en_b_6 = 1'b1;
  assign bram_we_b_6 = {`MASK_WIDTH{1'b0}};
  

wire done_mat_mul;
assign done_mat_mul = 
done_mat_mul_0 &
done_mat_mul_1 &
done_mat_mul_2 &
done_mat_mul_3 &
done_mat_mul_4 &
done_mat_mul_5 &
done_mat_mul_6;

wire done_eltwise_add_phase_1;
wire done_eltwise_add_phase_2;
wire done_eltwise_add_phase_3;
assign done_eltwise_add_phase_1 = done_mat_mul_4 & done_mat_mul_5 & done_mat_mul_6;
assign done_eltwise_add_phase_2 = done_mat_mul_5 & done_mat_mul_6;
assign done_eltwise_add_phase_3 = done_mat_mul_6;



reg [1:0] slice_0_op;
    

reg [1:0] slice_1_op;
    

reg [1:0] slice_2_op;
    

reg [1:0] slice_3_op;
    

reg [1:0] slice_4_op;
    

reg [1:0] slice_5_op;
    

reg [1:0] slice_6_op;
    

reg [3:0] count;
reg [4:0] state;
reg [4:0] vertical_count;

	always @( posedge clk) begin
      if (resetn == 1'b0) begin
        state <= 5'd0;
        done <= 0;
        

      slice_0_op <= 0;
      start_mat_mul_0 <= 0;
      address_mat_a_0 <= 0;
      address_mat_b_0 <= 0;
      address_mat_c_0 <= 0;
      address_stride_a_0 <= 0;
      address_stride_b_0 <= 0;
      address_stride_c_0 <= 0;
      validity_mask_a_0_rows <= 0;
      validity_mask_a_0_cols <= 0;
      validity_mask_b_0_rows <= 0;
      validity_mask_b_0_cols <= 0;
    

      slice_1_op <= 0;
      start_mat_mul_1 <= 0;
      address_mat_a_1 <= 0;
      address_mat_b_1 <= 0;
      address_mat_c_1 <= 0;
      address_stride_a_1 <= 0;
      address_stride_b_1 <= 0;
      address_stride_c_1 <= 0;
      validity_mask_a_1_rows <= 0;
      validity_mask_a_1_cols <= 0;
      validity_mask_b_1_rows <= 0;
      validity_mask_b_1_cols <= 0;
    

      slice_2_op <= 0;
      start_mat_mul_2 <= 0;
      address_mat_a_2 <= 0;
      address_mat_b_2 <= 0;
      address_mat_c_2 <= 0;
      address_stride_a_2 <= 0;
      address_stride_b_2 <= 0;
      address_stride_c_2 <= 0;
      validity_mask_a_2_rows <= 0;
      validity_mask_a_2_cols <= 0;
      validity_mask_b_2_rows <= 0;
      validity_mask_b_2_cols <= 0;
    

      slice_3_op <= 0;
      start_mat_mul_3 <= 0;
      address_mat_a_3 <= 0;
      address_mat_b_3 <= 0;
      address_mat_c_3 <= 0;
      address_stride_a_3 <= 0;
      address_stride_b_3 <= 0;
      address_stride_c_3 <= 0;
      validity_mask_a_3_rows <= 0;
      validity_mask_a_3_cols <= 0;
      validity_mask_b_3_rows <= 0;
      validity_mask_b_3_cols <= 0;
    

      slice_4_op <= 0;
      start_mat_mul_4 <= 0;
      address_mat_a_4 <= 0;
      address_mat_b_4 <= 0;
      address_mat_c_4 <= 0;
      address_stride_a_4 <= 0;
      address_stride_b_4 <= 0;
      address_stride_c_4 <= 0;
      validity_mask_a_4_rows <= 0;
      validity_mask_a_4_cols <= 0;
      validity_mask_b_4_rows <= 0;
      validity_mask_b_4_cols <= 0;
    

      slice_5_op <= 0;
      start_mat_mul_5 <= 0;
      address_mat_a_5 <= 0;
      address_mat_b_5 <= 0;
      address_mat_c_5 <= 0;
      address_stride_a_5 <= 0;
      address_stride_b_5 <= 0;
      address_stride_c_5 <= 0;
      validity_mask_a_5_rows <= 0;
      validity_mask_a_5_cols <= 0;
      validity_mask_b_5_rows <= 0;
      validity_mask_b_5_cols <= 0;
    

      slice_6_op <= 0;
      start_mat_mul_6 <= 0;
      address_mat_a_6 <= 0;
      address_mat_b_6 <= 0;
      address_mat_c_6 <= 0;
      address_stride_a_6 <= 0;
      address_stride_b_6 <= 0;
      address_stride_c_6 <= 0;
      validity_mask_a_6_rows <= 0;
      validity_mask_a_6_cols <= 0;
      validity_mask_b_6_rows <= 0;
      validity_mask_b_6_cols <= 0;
    

        count <= 0;
        vertical_count <= 0;
      end 
      else begin
        case (state)
        5'd0: begin
        
start_mat_mul_0 <= 1'b0;
start_mat_mul_1 <= 1'b0;
start_mat_mul_2 <= 1'b0;
start_mat_mul_3 <= 1'b0;
start_mat_mul_4 <= 1'b0;
start_mat_mul_5 <= 1'b0;
start_mat_mul_6 <= 1'b0;

          if (start== 1'b1) begin
            count <= 4'd1;
            vertical_count <= 5'd1;
            state <= 5'd1;
            done <= 0;
          end 
        end


    5'd1: begin


      slice_0_op <= 2'b00;
      start_mat_mul_0 <= 1'b1;
      address_mat_a_0 <=  vertical_count + `ADDRESS_BASE_A +10'd0; //will change horizontally
      address_mat_b_0 <=  vertical_count + `ADDRESS_BASE_B +10'd0; //will change horizontally
      address_mat_c_0 <=  vertical_count + `ADDRESS_BASE_A +10'd192; //will stay constant horizontally
      //if (count==4'd4) begin
      // address_stride_a_0 <= 16'd8;
      //end else begin
        address_stride_a_0 <= 16'd1; 
      //end
      address_stride_b_0 <= 16'd3; //constant horiz
      address_stride_c_0 <= 16'd64; //constant horiz
      validity_mask_a_0_rows <= 4'b1111; //constant
      validity_mask_a_0_cols <= 4'b1111; //constant
      validity_mask_b_0_rows <= 4'b1111; //constant
      validity_mask_b_0_cols <= 4'b0111; //constant
      

      slice_1_op <= 2'b00;
      start_mat_mul_1 <= 1'b1;
      address_mat_a_1 <=  vertical_count + `ADDRESS_BASE_A +10'd11; //will change horizontally
      address_mat_b_1 <=  vertical_count + `ADDRESS_BASE_B +10'd12; //will change horizontally
      address_mat_c_1 <=  vertical_count + `ADDRESS_BASE_A +10'd192; //will stay constant horizontally
      //if (count==4'd4) begin
      // address_stride_a_1 <= 16'd8;
      //end else begin
        address_stride_a_1 <= 16'd1; 
      //end
      address_stride_b_1 <= 16'd3; //constant horiz
      address_stride_c_1 <= 16'd64; //constant horiz
      validity_mask_a_1_rows <= 4'b1111; //constant
      validity_mask_a_1_cols <= 4'b1111; //constant
      validity_mask_b_1_rows <= 4'b1111; //constant
      validity_mask_b_1_cols <= 4'b0111; //constant
      

      slice_2_op <= 2'b00;
      start_mat_mul_2 <= 1'b1;
      address_mat_a_2 <=  vertical_count + `ADDRESS_BASE_A +10'd22; //will change horizontally
      address_mat_b_2 <=  vertical_count + `ADDRESS_BASE_B +10'd24; //will change horizontally
      address_mat_c_2 <=  vertical_count + `ADDRESS_BASE_A +10'd192; //will stay constant horizontally
      //if (count==4'd4) begin
      // address_stride_a_2 <= 16'd8;
      //end else begin
        address_stride_a_2 <= 16'd1; 
      //end
      address_stride_b_2 <= 16'd3; //constant horiz
      address_stride_c_2 <= 16'd64; //constant horiz
      validity_mask_a_2_rows <= 4'b1111; //constant
      validity_mask_a_2_cols <= 4'b1111; //constant
      validity_mask_b_2_rows <= 4'b1111; //constant
      validity_mask_b_2_cols <= 4'b0111; //constant
      

      slice_3_op <= 2'b00;
      start_mat_mul_3 <= 1'b1;
      address_mat_a_3 <=  vertical_count + `ADDRESS_BASE_A +10'd33; //will change horizontally
      address_mat_b_3 <=  vertical_count + `ADDRESS_BASE_B +10'd36; //will change horizontally
      address_mat_c_3 <=  vertical_count + `ADDRESS_BASE_A +10'd192; //will stay constant horizontally
      //if (count==4'd4) begin
      // address_stride_a_3 <= 16'd8;
      //end else begin
        address_stride_a_3 <= 16'd1; 
      //end
      address_stride_b_3 <= 16'd3; //constant horiz
      address_stride_c_3 <= 16'd64; //constant horiz
      validity_mask_a_3_rows <= 4'b1111; //constant
      validity_mask_a_3_cols <= 4'b1111; //constant
      validity_mask_b_3_rows <= 4'b1111; //constant
      validity_mask_b_3_cols <= 4'b0111; //constant
      

      slice_4_op <= 2'b00;
      start_mat_mul_4 <= 1'b1;
      address_mat_a_4 <=  vertical_count + `ADDRESS_BASE_A +10'd44; //will change horizontally
      address_mat_b_4 <=  vertical_count + `ADDRESS_BASE_B +10'd48; //will change horizontally
      address_mat_c_4 <=  vertical_count + `ADDRESS_BASE_A +10'd192; //will stay constant horizontally
      //if (count==4'd4) begin
      // address_stride_a_4 <= 16'd8;
      //end else begin
        address_stride_a_4 <= 16'd1; 
      //end
      address_stride_b_4 <= 16'd3; //constant horiz
      address_stride_c_4 <= 16'd64; //constant horiz
      validity_mask_a_4_rows <= 4'b1111; //constant
      validity_mask_a_4_cols <= 4'b1111; //constant
      validity_mask_b_4_rows <= 4'b1111; //constant
      validity_mask_b_4_cols <= 4'b0111; //constant
      

      slice_5_op <= 2'b00;
      start_mat_mul_5 <= 1'b1;
      address_mat_a_5 <=  vertical_count + `ADDRESS_BASE_A +10'd55; //will change horizontally
      address_mat_b_5 <=  vertical_count + `ADDRESS_BASE_B +10'd60; //will change horizontally
      address_mat_c_5 <=  vertical_count + `ADDRESS_BASE_A +10'd192; //will stay constant horizontally
      //if (count==4'd4) begin
      // address_stride_a_5 <= 16'd8;
      //end else begin
        address_stride_a_5 <= 16'd1; 
      //end
      address_stride_b_5 <= 16'd3; //constant horiz
      address_stride_c_5 <= 16'd64; //constant horiz
      validity_mask_a_5_rows <= 4'b1111; //constant
      validity_mask_a_5_cols <= 4'b1111; //constant
      validity_mask_b_5_rows <= 4'b1111; //constant
      validity_mask_b_5_cols <= 4'b0111; //constant
      

      slice_6_op <= 2'b00;
      start_mat_mul_6 <= 1'b1;
      address_mat_a_6 <=  vertical_count + `ADDRESS_BASE_A +10'd66; //will change horizontally
      address_mat_b_6 <=  vertical_count + `ADDRESS_BASE_B +10'd72; //will change horizontally
      address_mat_c_6 <=  vertical_count + `ADDRESS_BASE_A +10'd192; //will stay constant horizontally
      //if (count==4'd4) begin
      // address_stride_a_6 <= 16'd8;
      //end else begin
        address_stride_a_6 <= 16'd1; 
      //end
      address_stride_b_6 <= 16'd3; //constant horiz
      address_stride_c_6 <= 16'd64; //constant horiz
      validity_mask_a_6_rows <= 4'b1111; //constant
      validity_mask_a_6_cols <= 4'b1111; //constant
      validity_mask_b_6_rows <= 4'b1111; //constant
      validity_mask_b_6_cols <= 4'b0111; //constant
      

  count <= count + 1;

  if (done_mat_mul == 1'b1) begin
    state <= 5'd2;


  end
end


    5'd2: begin
    count <= 4'b0;


    start_mat_mul_0 <= 1'b0;

    start_mat_mul_1 <= 1'b0;

    start_mat_mul_2 <= 1'b0;

    start_mat_mul_3 <= 1'b0;

    start_mat_mul_4 <= 1'b0;

    start_mat_mul_5 <= 1'b0;

    start_mat_mul_6 <= 1'b0;

    state <= 5'd3;
  end


    5'd3: begin


      slice_4_op <= 2'b10;
      start_mat_mul_4 <= 1'b1;
      address_mat_a_4 <= vertical_count + `ADDRESS_BASE_A + 10'd192; //will stay constant horizontally
      address_mat_b_4 <= vertical_count + `ADDRESS_BASE_A +  10'd192; //will stay constant horizontally
      address_mat_c_4 <= vertical_count + `ADDRESS_BASE_A + 10'd512; //will stay constant horizontally
      address_stride_a_4 <= 16'd64;
      address_stride_b_4 <= 16'd64;
      address_stride_c_4 <= 16'd64; 
      validity_mask_a_4_rows <= 4'b1111; //constant
      validity_mask_a_4_cols <= 4'b1111; //constant
      validity_mask_b_4_rows <= 4'b1111; //constant
      validity_mask_b_4_cols <= 4'b0111; //constant
    

      slice_5_op <= 2'b10;
      start_mat_mul_5 <= 1'b1;
      address_mat_a_5 <= vertical_count + `ADDRESS_BASE_A + 10'd192; //will stay constant horizontally
      address_mat_b_5 <= vertical_count + `ADDRESS_BASE_A +  10'd192; //will stay constant horizontally
      address_mat_c_5 <= vertical_count + `ADDRESS_BASE_A + 10'd512; //will stay constant horizontally
      address_stride_a_5 <= 16'd64;
      address_stride_b_5 <= 16'd64;
      address_stride_c_5 <= 16'd64; 
      validity_mask_a_5_rows <= 4'b1111; //constant
      validity_mask_a_5_cols <= 4'b1111; //constant
      validity_mask_b_5_rows <= 4'b1111; //constant
      validity_mask_b_5_cols <= 4'b0111; //constant
    

      slice_6_op <= 2'b10;
      start_mat_mul_6 <= 1'b1;
      address_mat_a_6 <= vertical_count + `ADDRESS_BASE_A + 10'd192; //will stay constant horizontally
      address_mat_b_6 <= vertical_count + `ADDRESS_BASE_A +  10'd192; //will stay constant horizontally
      address_mat_c_6 <= vertical_count + `ADDRESS_BASE_A + 10'd512; //will stay constant horizontally
      address_stride_a_6 <= 16'd64;
      address_stride_b_6 <= 16'd64;
      address_stride_c_6 <= 16'd64; 
      validity_mask_a_6_rows <= 4'b1111; //constant
      validity_mask_a_6_cols <= 4'b1111; //constant
      validity_mask_b_6_rows <= 4'b1111; //constant
      validity_mask_b_6_cols <= 4'b0111; //constant
    

      if (done_eltwise_add_phase_1 == 1'b1) begin
        state <= 5'd4;
      end


end
    5'd4: begin
        start_mat_mul_4 <= 1'b0;
        start_mat_mul_5 <= 1'b0;
        start_mat_mul_6 <= 1'b0;
        state <= 5'd5;


end
    5'd5: begin


      slice_5_op <= 2'b10;
      start_mat_mul_5 <= 1'b1;
      address_mat_a_5 <= vertical_count + `ADDRESS_BASE_A + 10'd512; //will stay constant horizontally
      address_mat_b_5 <= vertical_count + `ADDRESS_BASE_A +  10'd512; //will stay constant horizontally
      address_mat_c_5 <= vertical_count + `ADDRESS_BASE_A + 10'd768; //will stay constant horizontally
      address_stride_a_5 <= 16'd64;
      address_stride_b_5 <= 16'd64;
      address_stride_c_5 <= 16'd64; 
      validity_mask_a_5_rows <= 4'b1111; //constant
      validity_mask_a_5_cols <= 4'b1111; //constant
      validity_mask_b_5_rows <= 4'b1111; //constant
      validity_mask_b_5_cols <= 4'b0111; //constant
    

      slice_6_op <= 2'b10;
      start_mat_mul_6 <= 1'b1;
      address_mat_a_6 <= vertical_count + `ADDRESS_BASE_A + 10'd512; //will stay constant horizontally
      address_mat_b_6 <= vertical_count + `ADDRESS_BASE_A +  10'd512; //will stay constant horizontally
      address_mat_c_6 <= vertical_count + `ADDRESS_BASE_A + 10'd768; //will stay constant horizontally
      address_stride_a_6 <= 16'd64;
      address_stride_b_6 <= 16'd64;
      address_stride_c_6 <= 16'd64; 
      validity_mask_a_6_rows <= 4'b1111; //constant
      validity_mask_a_6_cols <= 4'b1111; //constant
      validity_mask_b_6_rows <= 4'b1111; //constant
      validity_mask_b_6_cols <= 4'b0111; //constant
    

      if (done_eltwise_add_phase_2 == 1'b1) begin
        state <= 5'd6;
      end


end
    5'd6: begin
        state <= 5'd7;
        start_mat_mul_5 <= 1'b0;
        start_mat_mul_6 <= 1'b0;


end
    5'd7: begin


      slice_6_op <= 2'b10;
      start_mat_mul_6 <= 1'b1;
      address_mat_a_6 <= vertical_count + `ADDRESS_BASE_A + 10'd768; //will stay constant horizontally
      address_mat_b_6 <= vertical_count + `ADDRESS_BASE_A +  10'd768; //will stay constant horizontally
      address_mat_c_6 <= vertical_count + `ADDRESS_BASE_A + 10'd900; //will stay constant horizontally
      address_stride_a_6 <= 16'd64;
      address_stride_b_6 <= 16'd64;
      address_stride_c_6 <= 16'd64; 
      validity_mask_a_6_rows <= 4'b1111; //constant
      validity_mask_a_6_cols <= 4'b1111; //constant
      validity_mask_b_6_rows <= 4'b1111; //constant
      validity_mask_b_6_cols <= 4'b0111; //constant
    

      if (done_eltwise_add_phase_3 == 1'b1) begin
        state <= 5'd8;
      end
end


    5'd8: begin
        state <= 5'd9;
        start_mat_mul_6 <= 1'b0;
end


    5'd9: begin
    if (vertical_count == 5'd16) begin
      done <= 1'b1;
      state <= 5'd0;
    end 
    else begin
      vertical_count <= vertical_count + 1;
      state <= 5'd1;
    end
    end
  
endcase
end
end


    matmul_4x4_systolic u_matmul_4x4_systolic_0(
      .clk(clk),
      .reset(reset),
      .pe_reset(pe_reset),
      .start_mat_mul(start_mat_mul_0),
      .done_mat_mul(done_mat_mul_0),
      .address_mat_a(address_mat_a_0),
      .address_mat_b(address_mat_b_0),
      .address_mat_c(address_mat_c_0),
      .address_stride_a(address_stride_a_0),
      .address_stride_b(address_stride_b_0),
      .address_stride_c(address_stride_c_0),
      .a_data(a_data_0),
      .b_data(b_data_0),
      .a_data_in(a_data_in_0_NC),
      .b_data_in(b_data_in_0_NC),
      .c_data_in(c_data_in_0_NC),
      .c_data_out(c_data_0),
      .a_data_out(a_data_out_0_NC),
      .b_data_out(b_data_out_0_NC),
      .a_addr(a_addr_0),
      .b_addr(b_addr_0),
      .c_addr(c_addr_0),
      .c_data_available(c_data_0_available),
      .flags(flags_NC_0),
      .validity_mask_a_rows(validity_mask_a_0_rows),
      .validity_mask_a_cols(validity_mask_a_0_cols),
      .validity_mask_b_rows(validity_mask_b_0_rows),
      .validity_mask_b_cols(validity_mask_b_0_cols),
      .slice_mode(1'b0), //0 is SLICE_MODE_MATMUL
      .slice_dtype(1'b0),
      .op(slice_0_op), 
      .preload(1'b0),
      .final_mat_mul_size(8'd4),
      .a_loc(8'd0),
      .b_loc(8'd0)
    );
    
    

    matmul_4x4_systolic u_matmul_4x4_systolic_1(
      .clk(clk),
      .reset(reset),
      .pe_reset(pe_reset),
      .start_mat_mul(start_mat_mul_1),
      .done_mat_mul(done_mat_mul_1),
      .address_mat_a(address_mat_a_1),
      .address_mat_b(address_mat_b_1),
      .address_mat_c(address_mat_c_1),
      .address_stride_a(address_stride_a_1),
      .address_stride_b(address_stride_b_1),
      .address_stride_c(address_stride_c_1),
      .a_data(a_data_1),
      .b_data(b_data_1),
      .a_data_in(a_data_in_1_NC),
      .b_data_in(b_data_in_1_NC),
      .c_data_in(c_data_in_1_NC),
      .c_data_out(c_data_1),
      .a_data_out(a_data_out_1_NC),
      .b_data_out(b_data_out_1_NC),
      .a_addr(a_addr_1),
      .b_addr(b_addr_1),
      .c_addr(c_addr_1),
      .c_data_available(c_data_1_available),
      .flags(flags_NC_1),
      .validity_mask_a_rows(validity_mask_a_1_rows),
      .validity_mask_a_cols(validity_mask_a_1_cols),
      .validity_mask_b_rows(validity_mask_b_1_rows),
      .validity_mask_b_cols(validity_mask_b_1_cols),
      .slice_mode(1'b0), //0 is SLICE_MODE_MATMUL
      .slice_dtype(1'b0),
      .op(slice_1_op), 
      .preload(1'b0),
      .final_mat_mul_size(8'd4),
      .a_loc(8'd0),
      .b_loc(8'd0)
    );
    
    

    matmul_4x4_systolic u_matmul_4x4_systolic_2(
      .clk(clk),
      .reset(reset),
      .pe_reset(pe_reset),
      .start_mat_mul(start_mat_mul_2),
      .done_mat_mul(done_mat_mul_2),
      .address_mat_a(address_mat_a_2),
      .address_mat_b(address_mat_b_2),
      .address_mat_c(address_mat_c_2),
      .address_stride_a(address_stride_a_2),
      .address_stride_b(address_stride_b_2),
      .address_stride_c(address_stride_c_2),
      .a_data(a_data_2),
      .b_data(b_data_2),
      .a_data_in(a_data_in_2_NC),
      .b_data_in(b_data_in_2_NC),
      .c_data_in(c_data_in_2_NC),
      .c_data_out(c_data_2),
      .a_data_out(a_data_out_2_NC),
      .b_data_out(b_data_out_2_NC),
      .a_addr(a_addr_2),
      .b_addr(b_addr_2),
      .c_addr(c_addr_2),
      .c_data_available(c_data_2_available),
      .flags(flags_NC_2),
      .validity_mask_a_rows(validity_mask_a_2_rows),
      .validity_mask_a_cols(validity_mask_a_2_cols),
      .validity_mask_b_rows(validity_mask_b_2_rows),
      .validity_mask_b_cols(validity_mask_b_2_cols),
      .slice_mode(1'b0), //0 is SLICE_MODE_MATMUL
      .slice_dtype(1'b0),
      .op(slice_2_op), 
      .preload(1'b0),
      .final_mat_mul_size(8'd4),
      .a_loc(8'd0),
      .b_loc(8'd0)
    );
    
    

    matmul_4x4_systolic u_matmul_4x4_systolic_3(
      .clk(clk),
      .reset(reset),
      .pe_reset(pe_reset),
      .start_mat_mul(start_mat_mul_3),
      .done_mat_mul(done_mat_mul_3),
      .address_mat_a(address_mat_a_3),
      .address_mat_b(address_mat_b_3),
      .address_mat_c(address_mat_c_3),
      .address_stride_a(address_stride_a_3),
      .address_stride_b(address_stride_b_3),
      .address_stride_c(address_stride_c_3),
      .a_data(a_data_3),
      .b_data(b_data_3),
      .a_data_in(a_data_in_3_NC),
      .b_data_in(b_data_in_3_NC),
      .c_data_in(c_data_in_3_NC),
      .c_data_out(c_data_3),
      .a_data_out(a_data_out_3_NC),
      .b_data_out(b_data_out_3_NC),
      .a_addr(a_addr_3),
      .b_addr(b_addr_3),
      .c_addr(c_addr_3),
      .c_data_available(c_data_3_available),
      .flags(flags_NC_3),
      .validity_mask_a_rows(validity_mask_a_3_rows),
      .validity_mask_a_cols(validity_mask_a_3_cols),
      .validity_mask_b_rows(validity_mask_b_3_rows),
      .validity_mask_b_cols(validity_mask_b_3_cols),
      .slice_mode(1'b0), //0 is SLICE_MODE_MATMUL
      .slice_dtype(1'b0),
      .op(slice_3_op), 
      .preload(1'b0),
      .final_mat_mul_size(8'd4),
      .a_loc(8'd0),
      .b_loc(8'd0)
    );
    
    

    matmul_4x4_systolic u_matmul_4x4_systolic_4(
      .clk(clk),
      .reset(reset),
      .pe_reset(pe_reset),
      .start_mat_mul(start_mat_mul_4),
      .done_mat_mul(done_mat_mul_4),
      .address_mat_a(address_mat_a_4),
      .address_mat_b(address_mat_b_4),
      .address_mat_c(address_mat_c_4),
      .address_stride_a(address_stride_a_4),
      .address_stride_b(address_stride_b_4),
      .address_stride_c(address_stride_c_4),
      .a_data(a_data_4),
      .b_data(b_data_4),
      .a_data_in(a_data_in_4_NC),
      .b_data_in(b_data_in_4_NC),
      .c_data_in(c_data_in_4_NC),
      .c_data_out(c_data_4),
      .a_data_out(a_data_out_4_NC),
      .b_data_out(b_data_out_4_NC),
      .a_addr(a_addr_4),
      .b_addr(b_addr_4),
      .c_addr(c_addr_4),
      .c_data_available(c_data_4_available),
      .flags(flags_NC_4),
      .validity_mask_a_rows(validity_mask_a_4_rows),
      .validity_mask_a_cols(validity_mask_a_4_cols),
      .validity_mask_b_rows(validity_mask_b_4_rows),
      .validity_mask_b_cols(validity_mask_b_4_cols),
      .slice_mode(1'b0), //0 is SLICE_MODE_MATMUL
      .slice_dtype(1'b0),
      .op(slice_4_op), 
      .preload(1'b0),
      .final_mat_mul_size(8'd4),
      .a_loc(8'd0),
      .b_loc(8'd0)
    );
    
    

    matmul_4x4_systolic u_matmul_4x4_systolic_5(
      .clk(clk),
      .reset(reset),
      .pe_reset(pe_reset),
      .start_mat_mul(start_mat_mul_5),
      .done_mat_mul(done_mat_mul_5),
      .address_mat_a(address_mat_a_5),
      .address_mat_b(address_mat_b_5),
      .address_mat_c(address_mat_c_5),
      .address_stride_a(address_stride_a_5),
      .address_stride_b(address_stride_b_5),
      .address_stride_c(address_stride_c_5),
      .a_data(a_data_5),
      .b_data(b_data_5),
      .a_data_in(a_data_in_5_NC),
      .b_data_in(b_data_in_5_NC),
      .c_data_in(c_data_in_5_NC),
      .c_data_out(c_data_5),
      .a_data_out(a_data_out_5_NC),
      .b_data_out(b_data_out_5_NC),
      .a_addr(a_addr_5),
      .b_addr(b_addr_5),
      .c_addr(c_addr_5),
      .c_data_available(c_data_5_available),
      .flags(flags_NC_5),
      .validity_mask_a_rows(validity_mask_a_5_rows),
      .validity_mask_a_cols(validity_mask_a_5_cols),
      .validity_mask_b_rows(validity_mask_b_5_rows),
      .validity_mask_b_cols(validity_mask_b_5_cols),
      .slice_mode(1'b0), //0 is SLICE_MODE_MATMUL
      .slice_dtype(1'b0),
      .op(slice_5_op), 
      .preload(1'b0),
      .final_mat_mul_size(8'd4),
      .a_loc(8'd0),
      .b_loc(8'd0)
    );
    
    

    matmul_4x4_systolic u_matmul_4x4_systolic_6(
      .clk(clk),
      .reset(reset),
      .pe_reset(pe_reset),
      .start_mat_mul(start_mat_mul_6),
      .done_mat_mul(done_mat_mul_6),
      .address_mat_a(address_mat_a_6),
      .address_mat_b(address_mat_b_6),
      .address_mat_c(address_mat_c_6),
      .address_stride_a(address_stride_a_6),
      .address_stride_b(address_stride_b_6),
      .address_stride_c(address_stride_c_6),
      .a_data(a_data_6),
      .b_data(b_data_6),
      .a_data_in(a_data_in_6_NC),
      .b_data_in(b_data_in_6_NC),
      .c_data_in(c_data_in_6_NC),
      .c_data_out(c_data_6),
      .a_data_out(a_data_out_6_NC),
      .b_data_out(b_data_out_6_NC),
      .a_addr(a_addr_6),
      .b_addr(b_addr_6),
      .c_addr(c_addr_6),
      .c_data_available(c_data_6_available),
      .flags(flags_NC_6),
      .validity_mask_a_rows(validity_mask_a_6_rows),
      .validity_mask_a_cols(validity_mask_a_6_cols),
      .validity_mask_b_rows(validity_mask_b_6_rows),
      .validity_mask_b_cols(validity_mask_b_6_cols),
      .slice_mode(1'b0), //0 is SLICE_MODE_MATMUL
      .slice_dtype(1'b0),
      .op(slice_6_op), 
      .preload(1'b0),
      .final_mat_mul_size(8'd4),
      .a_loc(8'd0),
      .b_loc(8'd0)
    );
    
    


endmodule


//////////////////////////////////
//Dual port RAM
//////////////////////////////////
module ram (
        addr0, 
        d0, 
        we0, 
        q0,  
        addr1,
        d1,
        we1,
        q1,
        clk);

input [`AWIDTH-1:0] addr0;
input [`AWIDTH-1:0] addr1;
input [`MAT_MUL_SIZE*`DWIDTH-1:0] d0;
input [`MAT_MUL_SIZE*`DWIDTH-1:0] d1;
input [`MAT_MUL_SIZE-1:0] we0;
input [`MAT_MUL_SIZE-1:0] we1;
output [`MAT_MUL_SIZE*`DWIDTH-1:0] q0;
output [`MAT_MUL_SIZE*`DWIDTH-1:0] q1;
input clk;

`ifdef VCS
reg [`MAT_MUL_SIZE*`DWIDTH-1:0] q0;
reg [`MAT_MUL_SIZE*`DWIDTH-1:0] q1;
reg [`DWIDTH-1:0] ram[((1<<`AWIDTH)-1):0];
reg [31:0] i;

always @(posedge clk)  
begin 
    for (i = 0; i < `MAT_MUL_SIZE; i=i+1) begin
        if (we0[i]) ram[addr0+i] <= d0[i*`DWIDTH +: `DWIDTH]; 
    end    
    for (i = 0; i < `MAT_MUL_SIZE; i=i+1) begin
        q0[i*`DWIDTH +: `DWIDTH] <= ram[addr0+i];
    end    
end

always @(posedge clk)  
begin 
    for (i = 0; i < `MAT_MUL_SIZE; i=i+1) begin
        if (we1[i]) ram[addr0+i] <= d1[i*`DWIDTH +: `DWIDTH]; 
    end    
    for (i = 0; i < `MAT_MUL_SIZE; i=i+1) begin
        q1[i*`DWIDTH +: `DWIDTH] <= ram[addr1+i];
    end    
end

`else
//BRAMs available in VTR FPGA architectures have one bit write-enables.
//So let's combine multiple bits into 1. We don't have a usecase of
//writing/not-writing only parts of the word anyway.
wire we0_coalesced;
assign we0_coalesced = |we0;
wire we1_coalesced;
assign we1_coalesced = |we1;

dual_port_ram u_dual_port_ram(
.addr1(addr0),
.we1(we0_coalesced),
.data1(d0),
.out1(q0),
.addr2(addr1),
.we2(we1_coalesced),
.data2(d1),
.out2(q1),
.clk(clk)
);

`endif


endmodule

  
module matmul_4x4_systolic(
 clk,
 reset,
 pe_reset,
 start_mat_mul,
 done_mat_mul,
 address_mat_a,
 address_mat_b,
 address_mat_c,
 address_stride_a,
 address_stride_b,
 address_stride_c,
 a_data,
 b_data,
 a_data_in, //Data values coming in from previous matmul - systolic connections
 b_data_in,
 c_data_in, //Data values coming in from previous matmul - systolic shifting
 c_data_out, //Data values going out to next matmul - systolic shifting
 a_data_out,
 b_data_out,
 a_addr,
 b_addr,
 c_addr,
 c_data_available,
 flags,
 validity_mask_a_rows,
 validity_mask_a_cols,
 validity_mask_b_rows,
 validity_mask_b_cols,
 slice_mode,
 slice_dtype,
 op,
 preload,
 final_mat_mul_size,
 a_loc,
 b_loc
);

 input clk;
 input reset;
 input pe_reset;
 input start_mat_mul;
 output done_mat_mul;
 input [`AWIDTH-1:0] address_mat_a;
 input [`AWIDTH-1:0] address_mat_b;
 input [`AWIDTH-1:0] address_mat_c;
 input [`ADDR_STRIDE_WIDTH-1:0] address_stride_a;
 input [`ADDR_STRIDE_WIDTH-1:0] address_stride_b;
 input [`ADDR_STRIDE_WIDTH-1:0] address_stride_c;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_in;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_in;
 input [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_in;
 output [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_out;
 output [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_out;
 output [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_out;
 output [`AWIDTH-1:0] a_addr;
 output [`AWIDTH-1:0] b_addr;
 output [`AWIDTH-1:0] c_addr;
 output c_data_available;
 output [3:0] flags;
 input [`MASK_WIDTH-1:0] validity_mask_a_rows;
 input [`MASK_WIDTH-1:0] validity_mask_a_cols;
 input [`MASK_WIDTH-1:0] validity_mask_b_rows;
 input [`MASK_WIDTH-1:0] validity_mask_b_cols;
//7:0 is okay here. We aren't going to make a matmul larger than 128x128
//In fact, these will get optimized out by the synthesis tool, because
//we hardcode them at the instantiation level.
 input [7:0] final_mat_mul_size;
 input [7:0] a_loc;
 input [7:0] b_loc;
 input slice_dtype;
 input slice_mode;
 input [1:0] op;
 input preload;


 wire slice_dtype_NC;
 wire slice_mode_NC;
 wire preload_NC;

 assign slice_dtype_NC = slice_dtype;
 assign slice_mode_NC = slice_mode;
 assign preload_NC = preload;

//op[1]  op[0]
//   0      0   -> mat mul
//   0      1   -> elt mul
//   1      0   -> elt add
//   1      1   -> elt sub
wire eltwise_mode;
wire eltwise_mul;
wire eltwise_add;
wire eltwise_sub;
assign eltwise_mode =  ( op[1]  |   op[0]);
assign eltwise_mul  =  (~op[1]  &   op[0]);
assign eltwise_add  =  ( op[1]  &  ~op[0]);
assign eltwise_sub  =  ( op[1]  &   op[0]);

//////////////////////////////////////////////////////////////////////////
// Logic for clock counting and when to assert done
//////////////////////////////////////////////////////////////////////////

reg done_mat_mul;
//This is 7 bits because the expectation is that clock count will be pretty
//small. For large matmuls, this will need to increased to have more bits.
//In general, a systolic multiplier takes 4*N-2+P cycles, where N is the size 
//of the matmul and P is the number of pipleine stages in the MAC block.
reg [7:0] clk_cnt;

//Finding out number of cycles to assert matmul done.
//When we have to save the outputs to accumulators, then we don't need to
//shift out data. So, we can assert done_mat_mul early.
//In the normal case, we have to include the time to shift out the results. 
//Note: the count expression used to contain "4*final_mat_mul_size", but 
//to avoid multiplication, we now use "final_mat_mul_size<<2"
wire [7:0] clk_cnt_for_done;
assign clk_cnt_for_done = 
                          (eltwise_mode) ? ((final_mat_mul_size<<1) + 1):
                          ((final_mat_mul_size<<2) - 2 + `NUM_CYCLES_IN_MAC) ;  

always @(posedge clk) begin
  if (reset || ~start_mat_mul) begin
    clk_cnt <= 0;
    done_mat_mul <= 0;
  end
  else if (clk_cnt == clk_cnt_for_done) begin
    done_mat_mul <= 1;
    clk_cnt <= clk_cnt + 1;
  end
  else if (done_mat_mul == 0) begin
    clk_cnt <= clk_cnt + 1;
  end    
  else begin
    done_mat_mul <= 0;
    clk_cnt <= clk_cnt + 1;
  end
end


wire [`DWIDTH-1:0] a0_data;
wire [`DWIDTH-1:0] a1_data;
wire [`DWIDTH-1:0] a2_data;
wire [`DWIDTH-1:0] a3_data;
wire [`DWIDTH-1:0] b0_data;
wire [`DWIDTH-1:0] b1_data;
wire [`DWIDTH-1:0] b2_data;
wire [`DWIDTH-1:0] b3_data;
wire [`DWIDTH-1:0] a1_data_delayed_1;
wire [`DWIDTH-1:0] a2_data_delayed_1;
wire [`DWIDTH-1:0] a2_data_delayed_2;
wire [`DWIDTH-1:0] a3_data_delayed_1;
wire [`DWIDTH-1:0] a3_data_delayed_2;
wire [`DWIDTH-1:0] a3_data_delayed_3;
wire [`DWIDTH-1:0] b1_data_delayed_1;
wire [`DWIDTH-1:0] b2_data_delayed_1;
wire [`DWIDTH-1:0] b2_data_delayed_2;
wire [`DWIDTH-1:0] b3_data_delayed_1;
wire [`DWIDTH-1:0] b3_data_delayed_2;
wire [`DWIDTH-1:0] b3_data_delayed_3;

//////////////////////////////////////////////////////////////////////////
// Instantiation of systolic data setup
//////////////////////////////////////////////////////////////////////////
systolic_data_setup u_systolic_data_setup(
.clk(clk),
.reset(reset),
.start_mat_mul(start_mat_mul),
.eltwise_mode(eltwise_mode),
.a_addr(a_addr),
.b_addr(b_addr),
.address_mat_a(address_mat_a),
.address_mat_b(address_mat_b),
.address_stride_a(address_stride_a),
.address_stride_b(address_stride_b),
.a_data(a_data),
.b_data(b_data),
.clk_cnt(clk_cnt),
.a0_data(a0_data),
.a1_data_delayed_1(a1_data_delayed_1),
.a2_data_delayed_2(a2_data_delayed_2),
.a3_data_delayed_3(a3_data_delayed_3),
.b0_data(b0_data),
.b1_data_delayed_1(b1_data_delayed_1),
.b2_data_delayed_2(b2_data_delayed_2),
.b3_data_delayed_3(b3_data_delayed_3),
.validity_mask_a_rows(validity_mask_a_rows),
.validity_mask_a_cols(validity_mask_a_cols),
.validity_mask_b_rows(validity_mask_b_rows),
.validity_mask_b_cols(validity_mask_b_cols),
.final_mat_mul_size(final_mat_mul_size),
.a_loc(a_loc),
.b_loc(b_loc)
);


//////////////////////////////////////////////////////////////////////////
// Logic to mux data_in coming from neighboring matmuls
//////////////////////////////////////////////////////////////////////////
wire [`DWIDTH-1:0] a0;
wire [`DWIDTH-1:0] a1;
wire [`DWIDTH-1:0] a2;
wire [`DWIDTH-1:0] a3;
wire [`DWIDTH-1:0] b0;
wire [`DWIDTH-1:0] b1;
wire [`DWIDTH-1:0] b2;
wire [`DWIDTH-1:0] b3;

wire [`DWIDTH-1:0] a0_data_in;
wire [`DWIDTH-1:0] a1_data_in;
wire [`DWIDTH-1:0] a2_data_in;
wire [`DWIDTH-1:0] a3_data_in;
assign a0_data_in = a_data_in[`DWIDTH-1:0];
assign a1_data_in = a_data_in[2*`DWIDTH-1:`DWIDTH];
assign a2_data_in = a_data_in[3*`DWIDTH-1:2*`DWIDTH];
assign a3_data_in = a_data_in[4*`DWIDTH-1:3*`DWIDTH];

wire [`DWIDTH-1:0] b0_data_in;
wire [`DWIDTH-1:0] b1_data_in;
wire [`DWIDTH-1:0] b2_data_in;
wire [`DWIDTH-1:0] b3_data_in;
assign b0_data_in = b_data_in[`DWIDTH-1:0];
assign b1_data_in = b_data_in[2*`DWIDTH-1:`DWIDTH];
assign b2_data_in = b_data_in[3*`DWIDTH-1:2*`DWIDTH];
assign b3_data_in = b_data_in[4*`DWIDTH-1:3*`DWIDTH];

//If b_loc is 0, that means this matmul block is on the top-row of the
//final large matmul. In that case, b will take inputs from mem.
//If b_loc != 0, that means this matmul block is not on the top-row of the
//final large matmul. In that case, b will take inputs from the matmul on top
//of this one.
assign a0 = (b_loc==0) ? a0_data           : a0_data_in;
assign a1 = (b_loc==0) ? a1_data_delayed_1 : a1_data_in;
assign a2 = (b_loc==0) ? a2_data_delayed_2 : a2_data_in;
assign a3 = (b_loc==0) ? a3_data_delayed_3 : a3_data_in;

//If a_loc is 0, that means this matmul block is on the left-col of the
//final large matmul. In that case, a will take inputs from mem.
//If a_loc != 0, that means this matmul block is not on the left-col of the
//final large matmul. In that case, a will take inputs from the matmul on left
//of this one.
assign b0 = (a_loc==0) ? b0_data           : b0_data_in;
assign b1 = (a_loc==0) ? b1_data_delayed_1 : b1_data_in;
assign b2 = (a_loc==0) ? b2_data_delayed_2 : b2_data_in;
assign b3 = (a_loc==0) ? b3_data_delayed_3 : b3_data_in;


wire [`DWIDTH-1:0] matrixC00;
wire [`DWIDTH-1:0] matrixC01;
wire [`DWIDTH-1:0] matrixC02;
wire [`DWIDTH-1:0] matrixC03;
wire [`DWIDTH-1:0] matrixC10;
wire [`DWIDTH-1:0] matrixC11;
wire [`DWIDTH-1:0] matrixC12;
wire [`DWIDTH-1:0] matrixC13;
wire [`DWIDTH-1:0] matrixC20;
wire [`DWIDTH-1:0] matrixC21;
wire [`DWIDTH-1:0] matrixC22;
wire [`DWIDTH-1:0] matrixC23;
wire [`DWIDTH-1:0] matrixC30;
wire [`DWIDTH-1:0] matrixC31;
wire [`DWIDTH-1:0] matrixC32;
wire [`DWIDTH-1:0] matrixC33;


//////////////////////////////////////////////////////////////////////////
// Instantiation of the output logic
//////////////////////////////////////////////////////////////////////////
output_logic u_output_logic(
.clk(clk),
.reset(reset),
.start_mat_mul(start_mat_mul),
.done_mat_mul(done_mat_mul),
.eltwise_mode(eltwise_mode),
.address_mat_c(address_mat_c),
.address_stride_c(address_stride_c),
.c_data_out(c_data_out),
.c_data_in(c_data_in),
.c_addr(c_addr),
.c_data_available(c_data_available),
.clk_cnt(clk_cnt),
.row_latch_en(row_latch_en),
.final_mat_mul_size(final_mat_mul_size),
.matrixC00(matrixC00),
.matrixC01(matrixC01),
.matrixC02(matrixC02),
.matrixC03(matrixC03),
.matrixC10(matrixC10),
.matrixC11(matrixC11),
.matrixC12(matrixC12),
.matrixC13(matrixC13),
.matrixC20(matrixC20),
.matrixC21(matrixC21),
.matrixC22(matrixC22),
.matrixC23(matrixC23),
.matrixC30(matrixC30),
.matrixC31(matrixC31),
.matrixC32(matrixC32),
.matrixC33(matrixC33)
);

wire ready_for_eltwise_op;
assign ready_for_eltwise_op = (clk_cnt > final_mat_mul_size);

//////////////////////////////////////////////////////////////////////////
// Instantiations of the actual PEs
//////////////////////////////////////////////////////////////////////////
systolic_pe_matrix u_systolic_pe_matrix(
.reset(reset),
.clk(clk),
.pe_reset(pe_reset),
.start_mat_mul(start_mat_mul),
.ready_for_eltwise_op(ready_for_eltwise_op),
.op(op),
.a0(a0), 
.a1(a1), 
.a2(a2), 
.a3(a3),
.b0(b0), 
.b1(b1), 
.b2(b2), 
.b3(b3),
.matrixC00(matrixC00),
.matrixC01(matrixC01),
.matrixC02(matrixC02),
.matrixC03(matrixC03),
.matrixC10(matrixC10),
.matrixC11(matrixC11),
.matrixC12(matrixC12),
.matrixC13(matrixC13),
.matrixC20(matrixC20),
.matrixC21(matrixC21),
.matrixC22(matrixC22),
.matrixC23(matrixC23),
.matrixC30(matrixC30),
.matrixC31(matrixC31),
.matrixC32(matrixC32),
.matrixC33(matrixC33),
.a_data_out(a_data_out),
.b_data_out(b_data_out),
.flags(flags)
);

endmodule

//////////////////////////////////////////////////////////////////////////
// Output logic
//////////////////////////////////////////////////////////////////////////
module output_logic(
clk,
reset,
start_mat_mul,
done_mat_mul,
eltwise_mode,
address_mat_c,
address_stride_c,
c_data_in,
c_data_out, //Data values going out to next matmul - systolic shifting
c_addr,
c_data_available,
clk_cnt,
row_latch_en,
final_mat_mul_size,
matrixC00,
matrixC01,
matrixC02,
matrixC03,
matrixC10,
matrixC11,
matrixC12,
matrixC13,
matrixC20,
matrixC21,
matrixC22,
matrixC23,
matrixC30,
matrixC31,
matrixC32,
matrixC33
);

input clk;
input reset;
input start_mat_mul;
input done_mat_mul;
input eltwise_mode;
input [`AWIDTH-1:0] address_mat_c;
input [`ADDR_STRIDE_WIDTH-1:0] address_stride_c;
input [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_in;
output [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_out;
output [`AWIDTH-1:0] c_addr;
output c_data_available;
input [7:0] clk_cnt;
output row_latch_en;
input [7:0] final_mat_mul_size;
input [`DWIDTH-1:0] matrixC00;
input [`DWIDTH-1:0] matrixC01;
input [`DWIDTH-1:0] matrixC02;
input [`DWIDTH-1:0] matrixC03;
input [`DWIDTH-1:0] matrixC10;
input [`DWIDTH-1:0] matrixC11;
input [`DWIDTH-1:0] matrixC12;
input [`DWIDTH-1:0] matrixC13;
input [`DWIDTH-1:0] matrixC20;
input [`DWIDTH-1:0] matrixC21;
input [`DWIDTH-1:0] matrixC22;
input [`DWIDTH-1:0] matrixC23;
input [`DWIDTH-1:0] matrixC30;
input [`DWIDTH-1:0] matrixC31;
input [`DWIDTH-1:0] matrixC32;
input [`DWIDTH-1:0] matrixC33;

wire row_latch_en;

//////////////////////////////////////////////////////////////////////////
// Logic to capture matrix C data from the PEs and shift it out
//////////////////////////////////////////////////////////////////////////

//assign row_latch_en = (clk_cnt==(`MAT_MUL_SIZE + (a_loc+b_loc) * `BB_MAT_MUL_SIZE + 10 +  `NUM_CYCLES_IN_MAC - 1));
//Writing the line above to avoid multiplication:
//assign row_latch_en = (clk_cnt==(`MAT_MUL_SIZE + ((a_loc+b_loc) << `LOG2_MAT_MUL_SIZE) + 10 +  `NUM_CYCLES_IN_MAC - 1));
//Fixing bug. The line above is inaccurate. Using the line below. 
//TODO: This line needs to be fixed to include a_loc and b_loc ie. when final_mat_mul_size is different from `MAT_MUL_SIZE
assign row_latch_en =  
                       //((clk_cnt == ((`MAT_MUL_SIZE<<2) - `MAT_MUL_SIZE -2 +`NUM_CYCLES_IN_MAC)));
                       (eltwise_mode) ? ((clk_cnt == (final_mat_mul_size + 1 ))) :
                       ((clk_cnt == ((final_mat_mul_size<<2) - final_mat_mul_size -1 +`NUM_CYCLES_IN_MAC)));

reg c_data_available;
reg [`AWIDTH-1:0] c_addr;
reg start_capturing_c_data;
reg [31:0] counter;
reg [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_out;
reg [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_out_1;
reg [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_out_2;
reg [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_out_3;

wire [`MAT_MUL_SIZE*`DWIDTH-1:0] col0;
wire [`MAT_MUL_SIZE*`DWIDTH-1:0] col1;
wire [`MAT_MUL_SIZE*`DWIDTH-1:0] col2;
wire [`MAT_MUL_SIZE*`DWIDTH-1:0] col3;
assign col0 = {matrixC30, matrixC20, matrixC10, matrixC00};
assign col1 = {matrixC31, matrixC21, matrixC11, matrixC01};
assign col2 = {matrixC32, matrixC22, matrixC12, matrixC02};
assign col3 = {matrixC33, matrixC23, matrixC13, matrixC03};

//If save_output_to_accum is asserted, that means we are not intending to shift
//out the outputs, because the outputs are still partial sums. 
wire condition_to_start_shifting_output;
assign condition_to_start_shifting_output = 
                          row_latch_en ;  

//For larger matmuls, this logic will have more entries in the case statement
always @(posedge clk) begin
  if (reset | ~start_mat_mul) begin
    start_capturing_c_data <= 1'b0;
    c_data_available <= 1'b0;
    c_addr <= address_mat_c+address_stride_c;
    c_data_out <= 0;
    counter <= 0;
    c_data_out_1 <= 0; 
    c_data_out_2 <= 0; 
    c_data_out_3 <= 0; 
  end
  else if (condition_to_start_shifting_output) begin
    start_capturing_c_data <= 1'b1;
    c_data_available <= 1'b1;
    c_addr <= c_addr - address_stride_c;
    c_data_out <= col0; 
    c_data_out_1 <= col1; 
    c_data_out_2 <= col2; 
    c_data_out_3 <= col3; 
    counter <= counter + 1;
  end 
  else if (done_mat_mul) begin
    start_capturing_c_data <= 1'b0;
    c_data_available <= 1'b0;
    c_addr <= address_mat_c+address_stride_c;
    c_data_out <= 0;
    c_data_out_1 <= 0;
    c_data_out_2 <= 0;
    c_data_out_3 <= 0;
  end 
  else if (counter >= `MAT_MUL_SIZE) begin
    c_addr <= c_addr - address_stride_c;
    c_data_out <= c_data_out_1;
    c_data_out_1 <= c_data_out_2;
    c_data_out_2 <= c_data_out_3;
    c_data_out_3 <= c_data_in;
  end
  else if (start_capturing_c_data) begin
    c_data_available <= 1'b1;
    c_addr <= c_addr - address_stride_c;
    counter <= counter + 1;
    c_data_out <= c_data_out_1;
    c_data_out_1 <= c_data_out_2;
    c_data_out_2 <= c_data_out_3;
    c_data_out_3 <= c_data_in;
  end
end

endmodule

//////////////////////////////////////////////////////////////////////////
// Systolic data setup
//////////////////////////////////////////////////////////////////////////
module systolic_data_setup(
clk,
reset,
start_mat_mul,
eltwise_mode,
a_addr,
b_addr,
address_mat_a,
address_mat_b,
address_stride_a,
address_stride_b,
a_data,
b_data,
clk_cnt,
a0_data,
a1_data_delayed_1,
a2_data_delayed_2,
a3_data_delayed_3,
b0_data,
b1_data_delayed_1,
b2_data_delayed_2,
b3_data_delayed_3,
validity_mask_a_rows,
validity_mask_a_cols,
validity_mask_b_rows,
validity_mask_b_cols,
final_mat_mul_size,
a_loc,
b_loc
);

input clk;
input reset;
input start_mat_mul;
input eltwise_mode;
output [`AWIDTH-1:0] a_addr;
output [`AWIDTH-1:0] b_addr;
input [`AWIDTH-1:0] address_mat_a;
input [`AWIDTH-1:0] address_mat_b;
input [`ADDR_STRIDE_WIDTH-1:0] address_stride_a;
input [`ADDR_STRIDE_WIDTH-1:0] address_stride_b;
input [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data;
input [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data;
input [7:0] clk_cnt;
output [`DWIDTH-1:0] a0_data;
output [`DWIDTH-1:0] a1_data_delayed_1;
output [`DWIDTH-1:0] a2_data_delayed_2;
output [`DWIDTH-1:0] a3_data_delayed_3;
output [`DWIDTH-1:0] b0_data;
output [`DWIDTH-1:0] b1_data_delayed_1;
output [`DWIDTH-1:0] b2_data_delayed_2;
output [`DWIDTH-1:0] b3_data_delayed_3;
input [`MASK_WIDTH-1:0] validity_mask_a_rows;
input [`MASK_WIDTH-1:0] validity_mask_a_cols;
input [`MASK_WIDTH-1:0] validity_mask_b_rows;
input [`MASK_WIDTH-1:0] validity_mask_b_cols;
input [7:0] final_mat_mul_size;
input [7:0] a_loc;
input [7:0] b_loc;

wire [`DWIDTH-1:0] a0_data;
wire [`DWIDTH-1:0] a1_data;
wire [`DWIDTH-1:0] a2_data;
wire [`DWIDTH-1:0] a3_data;
wire [`DWIDTH-1:0] b0_data;
wire [`DWIDTH-1:0] b1_data;
wire [`DWIDTH-1:0] b2_data;
wire [`DWIDTH-1:0] b3_data;

//////////////////////////////////////////////////////////////////////////
// Logic to generate addresses to BRAM A
//////////////////////////////////////////////////////////////////////////
reg [`AWIDTH-1:0] a_addr;
reg a_mem_access; //flag that tells whether the matmul is trying to access memory or not

always @(posedge clk) begin
  //else if (clk_cnt >= a_loc*`MAT_MUL_SIZE+final_mat_mul_size) begin
  //Writing the line above to avoid multiplication:
  if ((reset || ~start_mat_mul) || (clk_cnt >= (a_loc<<`LOG2_MAT_MUL_SIZE)+final_mat_mul_size)) begin
      a_addr <= address_mat_a-address_stride_a;
    a_mem_access <= 0;
  end

  //else if ((clk_cnt >= a_loc*`MAT_MUL_SIZE) && (clk_cnt < a_loc*`MAT_MUL_SIZE+final_mat_mul_size)) begin
  //Writing the line above to avoid multiplication:
  else if ((clk_cnt >= (a_loc<<`LOG2_MAT_MUL_SIZE)) && (clk_cnt < (a_loc<<`LOG2_MAT_MUL_SIZE)+final_mat_mul_size)) begin
      a_addr <= a_addr + address_stride_a;
    a_mem_access <= 1;
  end
end  

//////////////////////////////////////////////////////////////////////////
// Logic to generate valid signals for data coming from BRAM A
//////////////////////////////////////////////////////////////////////////
reg [7:0] a_mem_access_counter;
always @(posedge clk) begin
  if (reset || ~start_mat_mul) begin
    a_mem_access_counter <= 0;
  end
  else if (a_mem_access == 1) begin
    a_mem_access_counter <= a_mem_access_counter + 1;  

  end
  else begin
    a_mem_access_counter <= 0;
  end
end

wire a_data_valid; //flag that tells whether the data from memory is valid
assign a_data_valid = 
       ((validity_mask_a_cols[0]==1'b0 && a_mem_access_counter==1) ||
        (validity_mask_a_cols[1]==1'b0 && a_mem_access_counter==2) ||
        (validity_mask_a_cols[2]==1'b0 && a_mem_access_counter==3) ||
        (validity_mask_a_cols[3]==1'b0 && a_mem_access_counter==4)) ?
        1'b0 : (a_mem_access_counter >= `MEM_ACCESS_LATENCY);

//////////////////////////////////////////////////////////////////////////
// Logic to delay certain parts of the data received from BRAM A (systolic data setup)
//////////////////////////////////////////////////////////////////////////
//Slice data into chunks and qualify it with whether it is valid or not
assign a0_data = a_data[`DWIDTH-1:0] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[0]}};
assign a1_data = a_data[2*`DWIDTH-1:`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[1]}};
assign a2_data = a_data[3*`DWIDTH-1:2*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[2]}};
assign a3_data = a_data[4*`DWIDTH-1:3*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[3]}};

//For larger matmuls, more such delaying flops will be needed
reg [`DWIDTH-1:0] a1_data_delayed_1_temp;
reg [`DWIDTH-1:0] a2_data_delayed_1_temp;
reg [`DWIDTH-1:0] a2_data_delayed_2_temp;
reg [`DWIDTH-1:0] a3_data_delayed_1_temp;
reg [`DWIDTH-1:0] a3_data_delayed_2_temp;
reg [`DWIDTH-1:0] a3_data_delayed_3_temp;
always @(posedge clk) begin
  if (reset || ~start_mat_mul || clk_cnt==0) begin
    a1_data_delayed_1_temp <= 0;
    a2_data_delayed_1_temp <= 0;
    a2_data_delayed_2_temp <= 0;
    a3_data_delayed_1_temp <= 0;
    a3_data_delayed_2_temp <= 0;
    a3_data_delayed_3_temp <= 0;
  end
  else begin
    a1_data_delayed_1_temp <= a1_data;
    a2_data_delayed_1_temp <= a2_data;
    a2_data_delayed_2_temp <= a2_data_delayed_1_temp;
    a3_data_delayed_1_temp <= a3_data;
    a3_data_delayed_2_temp <= a3_data_delayed_1_temp;
    a3_data_delayed_3_temp <= a3_data_delayed_2_temp;
  end
end

assign a1_data_delayed_1 = eltwise_mode ? a1_data : a1_data_delayed_1_temp;
assign a2_data_delayed_2 = eltwise_mode ? a2_data : a2_data_delayed_2_temp;
assign a3_data_delayed_3 = eltwise_mode ? a3_data : a3_data_delayed_3_temp;

//////////////////////////////////////////////////////////////////////////
// Logic to generate addresses to BRAM B
//////////////////////////////////////////////////////////////////////////
reg [`AWIDTH-1:0] b_addr;
reg b_mem_access; //flag that tells whether the matmul is trying to access memory or not

always @(posedge clk) begin
  //else if (clk_cnt >= b_loc*`MAT_MUL_SIZE+final_mat_mul_size) begin
  //Writing the line above to avoid multiplication:
  if ((reset || ~start_mat_mul) || (clk_cnt >= (b_loc<<`LOG2_MAT_MUL_SIZE)+final_mat_mul_size)) begin
      b_addr <= address_mat_b - address_stride_b;
    b_mem_access <= 0;
  end
  //else if ((clk_cnt >= b_loc*`MAT_MUL_SIZE) && (clk_cnt < b_loc*`MAT_MUL_SIZE+final_mat_mul_size)) begin
  //Writing the line above to avoid multiplication:
  else if ((clk_cnt >= (b_loc<<`LOG2_MAT_MUL_SIZE)) && (clk_cnt < (b_loc<<`LOG2_MAT_MUL_SIZE)+final_mat_mul_size)) begin
      b_addr <= b_addr + address_stride_b;
    b_mem_access <= 1;
  end
end  

//////////////////////////////////////////////////////////////////////////
// Logic to generate valid signals for data coming from BRAM B
//////////////////////////////////////////////////////////////////////////
reg [7:0] b_mem_access_counter;
always @(posedge clk) begin
  if (reset || ~start_mat_mul) begin
    b_mem_access_counter <= 0;
  end
  else if (b_mem_access == 1) begin
    b_mem_access_counter <= b_mem_access_counter + 1;  
  end
  else begin
    b_mem_access_counter <= 0;
  end
end

wire b_data_valid; //flag that tells whether the data from memory is valid
assign b_data_valid = 
       ((validity_mask_b_rows[0]==1'b0 && b_mem_access_counter==1) ||
        (validity_mask_b_rows[1]==1'b0 && b_mem_access_counter==2) ||
        (validity_mask_b_rows[2]==1'b0 && b_mem_access_counter==3) ||
        (validity_mask_b_rows[3]==1'b0 && b_mem_access_counter==4)) ?
        1'b0 : (b_mem_access_counter >= `MEM_ACCESS_LATENCY);


//////////////////////////////////////////////////////////////////////////
// Logic to delay certain parts of the data received from BRAM B (systolic data setup)
//////////////////////////////////////////////////////////////////////////
//Slice data into chunks and qualify it with whether it is valid or not
assign b0_data = b_data[`DWIDTH-1:0] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[0]}};
assign b1_data = b_data[2*`DWIDTH-1:`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[1]}};
assign b2_data = b_data[3*`DWIDTH-1:2*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[2]}};
assign b3_data = b_data[4*`DWIDTH-1:3*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[3]}};

//For larger matmuls, more such delaying flops will be needed
reg [`DWIDTH-1:0] b1_data_delayed_1_temp;
reg [`DWIDTH-1:0] b2_data_delayed_1_temp;
reg [`DWIDTH-1:0] b2_data_delayed_2_temp;
reg [`DWIDTH-1:0] b3_data_delayed_1_temp;
reg [`DWIDTH-1:0] b3_data_delayed_2_temp;
reg [`DWIDTH-1:0] b3_data_delayed_3_temp;
always @(posedge clk) begin
  if (reset || ~start_mat_mul || clk_cnt==0) begin
    b1_data_delayed_1_temp <= 0;
    b2_data_delayed_1_temp <= 0;
    b2_data_delayed_2_temp <= 0;
    b3_data_delayed_1_temp <= 0;
    b3_data_delayed_2_temp <= 0;
    b3_data_delayed_3_temp <= 0;
  end
  else begin
    b1_data_delayed_1_temp <= b1_data;
    b2_data_delayed_1_temp <= b2_data;
    b2_data_delayed_2_temp <= b2_data_delayed_1_temp;
    b3_data_delayed_1_temp <= b3_data;
    b3_data_delayed_2_temp <= b3_data_delayed_1_temp;
    b3_data_delayed_3_temp <= b3_data_delayed_2_temp;
  end
end


assign b1_data_delayed_1 = eltwise_mode ? b1_data : b1_data_delayed_1_temp;
assign b2_data_delayed_2 = eltwise_mode ? b2_data : b2_data_delayed_2_temp;
assign b3_data_delayed_3 = eltwise_mode ? b3_data : b3_data_delayed_3_temp;

endmodule



//////////////////////////////////////////////////////////////////////////
// Systolically connected PEs
//////////////////////////////////////////////////////////////////////////
module systolic_pe_matrix(
reset,
clk,
pe_reset,
start_mat_mul,
ready_for_eltwise_op,
op,
a0, a1, a2, a3,
b0, b1, b2, b3,
matrixC00,
matrixC01,
matrixC02,
matrixC03,
matrixC10,
matrixC11,
matrixC12,
matrixC13,
matrixC20,
matrixC21,
matrixC22,
matrixC23,
matrixC30,
matrixC31,
matrixC32,
matrixC33,
a_data_out,
b_data_out,
flags
);

input clk;
input reset;
input pe_reset;
input start_mat_mul;
input ready_for_eltwise_op;
input [1:0] op;
input [`DWIDTH-1:0] a0;
input [`DWIDTH-1:0] a1;
input [`DWIDTH-1:0] a2;
input [`DWIDTH-1:0] a3;
input [`DWIDTH-1:0] b0;
input [`DWIDTH-1:0] b1;
input [`DWIDTH-1:0] b2;
input [`DWIDTH-1:0] b3;
output [`DWIDTH-1:0] matrixC00;
output [`DWIDTH-1:0] matrixC01;
output [`DWIDTH-1:0] matrixC02;
output [`DWIDTH-1:0] matrixC03;
output [`DWIDTH-1:0] matrixC10;
output [`DWIDTH-1:0] matrixC11;
output [`DWIDTH-1:0] matrixC12;
output [`DWIDTH-1:0] matrixC13;
output [`DWIDTH-1:0] matrixC20;
output [`DWIDTH-1:0] matrixC21;
output [`DWIDTH-1:0] matrixC22;
output [`DWIDTH-1:0] matrixC23;
output [`DWIDTH-1:0] matrixC30;
output [`DWIDTH-1:0] matrixC31;
output [`DWIDTH-1:0] matrixC32;
output [`DWIDTH-1:0] matrixC33;
output [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_out;
output [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_out;
output [3:0] flags;

wire [`DWIDTH-1:0] a00to01, a01to02, a02to03, a03to04;
wire [`DWIDTH-1:0] a10to11, a11to12, a12to13, a13to14;
wire [`DWIDTH-1:0] a20to21, a21to22, a22to23, a23to24;
wire [`DWIDTH-1:0] a30to31, a31to32, a32to33, a33to34;

wire [`DWIDTH-1:0] b00to10, b10to20, b20to30, b30to40; 
wire [`DWIDTH-1:0] b01to11, b11to21, b21to31, b31to41;
wire [`DWIDTH-1:0] b02to12, b12to22, b22to32, b32to42;
wire [`DWIDTH-1:0] b03to13, b13to23, b23to33, b33to43;

wire effective_rst;
assign effective_rst = reset | pe_reset;

wire [3:0] flags_pe00;
wire [3:0] flags_pe01;
wire [3:0] flags_pe02;
wire [3:0] flags_pe03;
wire [3:0] flags_pe10;
wire [3:0] flags_pe11;
wire [3:0] flags_pe12;
wire [3:0] flags_pe13;
wire [3:0] flags_pe20;
wire [3:0] flags_pe21;
wire [3:0] flags_pe22;
wire [3:0] flags_pe23;
wire [3:0] flags_pe30;
wire [3:0] flags_pe31;
wire [3:0] flags_pe32;
wire [3:0] flags_pe33;

assign flags = 
flags_pe00 |
flags_pe01 |
flags_pe02 |
flags_pe03 |
flags_pe10 |
flags_pe11 |
flags_pe12 |
flags_pe13 |
flags_pe20 |
flags_pe21 |
flags_pe22 |
flags_pe23 |
flags_pe30 |
flags_pe31 |
flags_pe32 |
flags_pe33;

assign flags_pe03 = 4'b0;
assign flags_pe13 = 4'b0;
assign flags_pe23 = 4'b0;
assign flags_pe33 = 4'b0;

processing_element pe00(.reset(effective_rst), .clk(clk),  .flags(flags_pe00), .op(op), .ready_for_eltwise_op(ready_for_eltwise_op), .in_a(a0),      .in_b(b0),  .out_a(a00to01), .out_b(b00to10), .out_c(matrixC00));
processing_element pe01(.reset(effective_rst), .clk(clk),  .flags(flags_pe01), .op(op), .ready_for_eltwise_op(ready_for_eltwise_op), .in_a(a00to01), .in_b(b1),  .out_a(a01to02), .out_b(b01to11), .out_c(matrixC01));
processing_element pe02(.reset(effective_rst), .clk(clk),  .flags(flags_pe02), .op(op), .ready_for_eltwise_op(ready_for_eltwise_op), .in_a(a01to02), .in_b(b2),  .out_a(a02to03), .out_b(b02to12), .out_c(matrixC02));
//processing_element pe03(.reset(effective_rst), .clk(clk),  .flags(flags_pe03), .op(op), .ready_for_eltwise_op(ready_for_eltwise_op), .in_a(a02to03), .in_b(b3),  .out_a(a03to04), .out_b(b03to13), .out_c(matrixC03));

processing_element pe10(.reset(effective_rst), .clk(clk),  .flags(flags_pe10), .op(op), .ready_for_eltwise_op(ready_for_eltwise_op), .in_a(a1),      .in_b(b00to10), .out_a(a10to11), .out_b(b10to20), .out_c(matrixC10));
processing_element pe11(.reset(effective_rst), .clk(clk),  .flags(flags_pe11), .op(op), .ready_for_eltwise_op(ready_for_eltwise_op), .in_a(a10to11), .in_b(b01to11), .out_a(a11to12), .out_b(b11to21), .out_c(matrixC11));
processing_element pe12(.reset(effective_rst), .clk(clk),  .flags(flags_pe12), .op(op), .ready_for_eltwise_op(ready_for_eltwise_op), .in_a(a11to12), .in_b(b02to12), .out_a(a12to13), .out_b(b12to22), .out_c(matrixC12));
//processing_element pe13(.reset(effective_rst), .clk(clk),  .flags(flags_pe13), .op(op), .ready_for_eltwise_op(ready_for_eltwise_op), .in_a(a12to13), .in_b(b03to13), .out_a(a13to14), .out_b(b13to23), .out_c(matrixC13));

processing_element pe20(.reset(effective_rst), .clk(clk),  .flags(flags_pe20), .op(op), .ready_for_eltwise_op(ready_for_eltwise_op), .in_a(a2),      .in_b(b10to20), .out_a(a20to21), .out_b(b20to30), .out_c(matrixC20));
processing_element pe21(.reset(effective_rst), .clk(clk),  .flags(flags_pe21), .op(op), .ready_for_eltwise_op(ready_for_eltwise_op), .in_a(a20to21), .in_b(b11to21), .out_a(a21to22), .out_b(b21to31), .out_c(matrixC21));
processing_element pe22(.reset(effective_rst), .clk(clk),  .flags(flags_pe22), .op(op), .ready_for_eltwise_op(ready_for_eltwise_op), .in_a(a21to22), .in_b(b12to22), .out_a(a22to23), .out_b(b22to32), .out_c(matrixC22));
//processing_element pe23(.reset(effective_rst), .clk(clk),  .flags(flags_pe23), .op(op), .ready_for_eltwise_op(ready_for_eltwise_op), .in_a(a22to23), .in_b(b13to23), .out_a(a23to24), .out_b(b23to33), .out_c(matrixC23));

processing_element pe30(.reset(effective_rst), .clk(clk),  .flags(flags_pe30), .op(op), .ready_for_eltwise_op(ready_for_eltwise_op), .in_a(a3),      .in_b(b20to30), .out_a(a30to31), .out_b(b30to40), .out_c(matrixC30));
processing_element pe31(.reset(effective_rst), .clk(clk),  .flags(flags_pe31), .op(op), .ready_for_eltwise_op(ready_for_eltwise_op), .in_a(a30to31), .in_b(b21to31), .out_a(a31to32), .out_b(b31to41), .out_c(matrixC31));
processing_element pe32(.reset(effective_rst), .clk(clk),  .flags(flags_pe32), .op(op), .ready_for_eltwise_op(ready_for_eltwise_op), .in_a(a31to32), .in_b(b22to32), .out_a(a32to33), .out_b(b32to42), .out_c(matrixC32));
//processing_element pe33(.reset(effective_rst), .clk(clk),  .flags(flags_pe33), .op(op), .ready_for_eltwise_op(ready_for_eltwise_op), .in_a(a32to33), .in_b(b23to33), .out_a(a33to34), .out_b(b33to43), .out_c(matrixC33));

assign a_data_out = {a33to34,a23to24,a13to14,a03to04};
assign b_data_out = {b33to43,b32to42,b31to41,b30to40};

endmodule


//////////////////////////////////////////////////////////////////////////
// Processing element (PE)
//////////////////////////////////////////////////////////////////////////
module processing_element(
 reset, 
 clk, 
 op,
 ready_for_eltwise_op,
 in_a,
 in_b, 
 out_a, 
 out_b, 
 out_c,
 flags
 );

 input reset;
 input clk;
 input [1:0] op;
 input ready_for_eltwise_op;
 input  [`DWIDTH-1:0] in_a;
 input  [`DWIDTH-1:0] in_b;
 output [`DWIDTH-1:0] out_a;
 output [`DWIDTH-1:0] out_b;
 output [`DWIDTH-1:0] out_c;  //reduced precision
 output [3:0] flags;

 reg [`DWIDTH-1:0] out_a;
 reg [`DWIDTH-1:0] out_b;
 wire [`DWIDTH-1:0] out_c;

 wire [`DWIDTH-1:0] out_mac;

//op[1]  op[0]
//   0      0   -> mat mul
//   0      1   -> elt mul
//   1      0   -> elt add
//   1      1   -> elt sub
wire eltwise_mode;
wire eltwise_mul;
wire eltwise_add;
wire eltwise_sub;
assign eltwise_mode =  ( op[1]  |   op[0]);
assign eltwise_mul  =  (~op[1]  &   op[0]);
assign eltwise_add  =  ( op[1]  &  ~op[0]);
assign eltwise_sub  =  ( op[1]  &   op[0]);

 assign out_c = out_mac;

 //Keep the mac in reset if we're adding 
 wire mac_reset;
 assign mac_reset = reset || (eltwise_mode && (~ready_for_eltwise_op));
 seq_mac u_mac(.a(in_a), .b(in_b), .out(out_mac), .eltwise_mode(eltwise_mode), .eltwise_add(eltwise_add), .reset(mac_reset), .clk(clk));

 always @(posedge clk)begin
    if(reset) begin
      out_a<=0;
      out_b<=0;
    end
    //stop shifting when you're in eltwise mode and ready to perform the op
    else if ((~eltwise_mode) || (eltwise_mode & ~ready_for_eltwise_op)) begin  
      out_a<=in_a;
      out_b<=in_b;
    end
 end
 
endmodule

//////////////////////////////////////////////////////////////////////////
// Multiply-and-accumulate (MAC) block
//////////////////////////////////////////////////////////////////////////

`ifdef ARCH_SPECIFIC_IMP

module seq_mac(a, b, out, eltwise_mode, eltwise_add, reset, clk);
input [`DWIDTH-1:0] a;
input [`DWIDTH-1:0] b;
output [`DWIDTH-1:0] out;
input eltwise_mode;
input eltwise_add;
input reset;
input clk;

reg [`DWIDTH-1:0] out;
wire [`DWIDTH-1:0] add_out;
wire [2*`DWIDTH-1:0] add_out_temp;
wire [2*`DWIDTH-1:0] mac_out;
reg [2*`DWIDTH-1:0] add_out_reg;

reg [`DWIDTH-1:0] a_flopped;
reg [`DWIDTH-1:0] b_flopped;

always @(posedge clk) begin
  if (reset) begin
    a_flopped <= 0;
    b_flopped <= 0;
  end else begin
    a_flopped <= a;
    b_flopped <= b;
  end
end

wire [17:0] a_in;
wire [18:0] b_in;
wire [36:0] c_out;
assign a_in = {1'b0,a_flopped};
assign b_in = {1'b0,1'b0,b_flopped};
mac_int u_mac (.clk(clk), .reset(reset), .a(a_flopped), .b(b_flopped), .out(c_out));
assign mac_out = c_out[2*`DWIDTH-1:0];


wire [2*`DWIDTH-1:0] add_in1;
wire [2*`DWIDTH-1:0] add_in2;
assign add_in1 = {{`DWIDTH{1'b0}}, a_flopped};
assign add_in2 = {{`DWIDTH{1'b0}}, b_flopped};
qadd add_u1(.a(add_in1), .b(add_in2), .c(add_out_temp));

always @(posedge clk) begin
  if (reset) begin
    add_out_reg <= 0;
  end
  else if(eltwise_mode & eltwise_add) begin
    add_out_reg <= add_out_temp;
  end
  else begin
    add_out_reg <= mac_out;
  end
end

//down cast the result
assign add_out = 
    (add_out_reg[2*`DWIDTH-1] == 0) ?  //positive number
        (
           (|(add_out_reg[2*`DWIDTH-2 : `DWIDTH-1])) ?  //is any bit from 14:7 is 1, that means overlfow
             {add_out_reg[2*`DWIDTH-1] , {(`DWIDTH-1){1'b1}}} : //sign bit and then all 1s
             {add_out_reg[2*`DWIDTH-1] , add_out_reg[`DWIDTH-2:0]} 
        )
        : //negative number
        (
           (|(add_out_reg[2*`DWIDTH-2 : `DWIDTH-1])) ?  //is any bit from 14:7 is 0, that means overlfow
             {add_out_reg[2*`DWIDTH-1] , add_out_reg[`DWIDTH-2:0]} :
             {add_out_reg[2*`DWIDTH-1] , {(`DWIDTH-1){1'b0}}} //sign bit and then all 0s
        );


always @(posedge clk) begin
  if (reset) begin
    out <= 0;
  end 
  else begin
    out <= add_out;
  end
end

endmodule


module qadd(a,b,c);
input [2*`DWIDTH-1:0] a;
input [2*`DWIDTH-1:0] b;
output [2*`DWIDTH-1:0] c;

assign c = a + b;
//DW01_add #(`DWIDTH) u_add(.A(a), .B(b), .CI(1'b0), .SUM(c), .CO());
endmodule



`else
module seq_mac(a, b, out, eltwise_mode, eltwise_add, reset, clk);
input [`DWIDTH-1:0] a;
input [`DWIDTH-1:0] b;
output [`DWIDTH-1:0] out;
input eltwise_mode;
input eltwise_add;
input reset;
input clk;

reg [`DWIDTH-1:0] out;
wire [2*`DWIDTH-1:0] add_out;

reg [`DWIDTH-1:0] a_flopped;
reg [`DWIDTH-1:0] b_flopped;

wire [2*`DWIDTH-1:0] mul_out;
reg [2*`DWIDTH-1:0] mul_out_reg;

always @(posedge clk) begin
  if (reset) begin
    a_flopped <= 0;
    b_flopped <= 0;
  end else begin
    a_flopped <= a;
    b_flopped <= b;
  end
end

qmult mult_u1(.i_multiplicand(a_flopped), .i_multiplier(b_flopped), .o_result(mul_out));

always @(posedge clk) begin
  if (reset) begin
    mul_out_reg <= 0;
  end else begin
    mul_out_reg <= mul_out;
  end
end


wire [2*`DWIDTH-1:0] add_in1;
wire [2*`DWIDTH-1:0] add_in2;
reg [2*`DWIDTH-1:0] add_out_reg;
assign add_in1 = (eltwise_mode & eltwise_add) ? {{`DWIDTH{1'b0}}, a_flopped} : add_out_reg;
assign add_in2 = (eltwise_mode & eltwise_add) ? {{`DWIDTH{1'b0}}, b_flopped} : mul_out_reg;

qadd add_u1(.a(add_in1), .b(add_in2), .c(add_out));

always @(posedge clk) begin
  if (reset) begin
    add_out_reg <= 0;
  end else begin
    add_out_reg <= add_out;
  end
end

wire [`DWIDTH-1:0] mac_out;

//down cast the result
assign mac_out = 
    (add_out_reg[2*`DWIDTH-1] == 0) ?  //positive number
        (
           (|(add_out_reg[2*`DWIDTH-2 : `DWIDTH-1])) ?  //is any bit from 14:7 is 1, that means overlfow
             {add_out_reg[2*`DWIDTH-1] , {(`DWIDTH-1){1'b1}}} : //sign bit and then all 1s
             {add_out_reg[2*`DWIDTH-1] , add_out_reg[`DWIDTH-2:0]} 
        )
        : //negative number
        (
           (|(add_out_reg[2*`DWIDTH-2 : `DWIDTH-1])) ?  //is any bit from 14:7 is 0, that means overlfow
             {add_out_reg[2*`DWIDTH-1] , add_out_reg[`DWIDTH-2:0]} :
             {add_out_reg[2*`DWIDTH-1] , {(`DWIDTH-1){1'b0}}} //sign bit and then all 0s
        );


always @(posedge clk) begin
  if (reset) begin
    out <= 0;
  end else begin
    out <= mac_out;
  end
end

endmodule


module qmult(i_multiplicand,i_multiplier,o_result);
input [`DWIDTH-1:0] i_multiplicand;
input [`DWIDTH-1:0] i_multiplier;
output [2*`DWIDTH-1:0] o_result;

assign o_result = i_multiplicand * i_multiplier;
//DW02_mult #(`DWIDTH,`DWIDTH) u_mult(.A(i_multiplicand), .B(i_multiplier), .TC(1'b1), .PRODUCT(o_result));

endmodule

module qadd(a,b,c);
input [2*`DWIDTH-1:0] a;
input [2*`DWIDTH-1:0] b;
output [2*`DWIDTH-1:0] c;

assign c = a + b;
//DW01_add #(`DWIDTH) u_add(.A(a), .B(b), .CI(1'b0), .SUM(c), .CO());
endmodule



`endif


