`timescale 1ns/1ns
`define VCS
`define MATMUL_SIZE_16 
`define MORE_TESTS
`define DESIGN_SIZE_16
`define SIMULATION
`define layer_test

`define DWIDTH 8
`define AWIDTH 11
`define MEM_SIZE 2048

`ifdef MATMUL_SIZE_4
`define MAT_MUL_SIZE 4
`define MASK_WIDTH 4
`define LOG2_MAT_MUL_SIZE 2
`endif

`ifdef MATMUL_SIZE_8
`define MAT_MUL_SIZE 8
`define MASK_WIDTH 8
`define LOG2_MAT_MUL_SIZE 3
`endif

`ifdef MATMUL_SIZE_16
`define MAT_MUL_SIZE 16
`define MASK_WIDTH 16
`define LOG2_MAT_MUL_SIZE 4
`endif

`ifdef MATMUL_SIZE_32
`define MAT_MUL_SIZE 32
`define MASK_WIDTH 32
`define LOG2_MAT_MUL_SIZE 5
`endif

`ifdef DESIGN_SIZE_4
`define DESIGN_SIZE 4
`define LOG2_DESIGN_SIZE 2
`endif

`ifdef DESIGN_SIZE_8
`define DESIGN_SIZE 8
`define LOG2_DESIGN_SIZE 3
`endif

`ifdef DESIGN_SIZE_16
`define DESIGN_SIZE 16
`define LOG2_DESIGN_SIZE 4
`endif

`ifdef DESIGN_SIZE_32
`define DESIGN_SIZE 32
`define LOG2_DESIGN_SIZE 5
`endif

`define BB_MAT_MUL_SIZE `MAT_MUL_SIZE
`define NUM_CYCLES_IN_MAC 3
`define MEM_ACCESS_LATENCY 1
`define REG_DATAWIDTH 32
`define REG_ADDRWIDTH 8
`define ADDR_STRIDE_WIDTH 8
`define MAX_BITS_POOL 3

/////////////////////////////////////////////////
//How to use fully-connected mode?
/////////////////////////////////////////////////
//TODO: See layer test and accum test and write documentation

/////////////////////////////////////////////////
//How to use convolution mode?
/////////////////////////////////////////////////

//Matrix A (input activation matrix)
//----------------------------------
//* This matrix is the non-expanded matrix (ie. this contains 
//  the same number of elements as the input activation tensor).
//  It doesn't contain the expanded GEMM M matrix corresponding
//  to this convolution.
//* This matrix is expected to have been padded though. That is,
//  if there are any padding rows/columns to be added, the software
//  should do that and store the padded matrix in the BRAM. 
//* Initial address of matrix A is to be programmed once in the
//  beginning of calculation of each output tile. We don't have 
//  to reprogram the address of A every time during accumulation.
//* The register containing stride of the matrix A is not used 
//  in convolution mode. Address strides for each read are determined
//  on the basis of C,R,S values internally in the RTL. This is because
//  strides are not fixed. They vary for every read.
//* This matrix is laid out in NCHW format. 

//Matrix B (weight matrix)
//----------------------------------
//* This matrix is the non-expanded matrix (ie. this contains 
//  the same number of elements as the weight tensor).
//  It doesn't contain the expanded GEMM N matrix corresponding
//  to this convolution.
//* There is no concept of padding for this matrix.
//* Initial address of matrix B is to be programmed once in the
//  beginning of calculation of each output tile. We don't have 
//  to reprogram the address of B every time during accumulation.
//* The register containing stride of the matrix B is not used
//  in the RTL. Address strides for each read are determined
//  on the basis of C,R,S values internally in the RTL. 
//* This matrix is laid out in NCHW format, but it is transposed.
//  So technically, the format is WHCN. 

//Matrix C (output activation matrix)
//----------------------------------
//* This matrix is the non-expanded matrix (ie. this contains 
//  the same number of elements as the output activation tensor).
//  It contains the GEMM matrix corresponding
//  to this convolution.
//* There is no concept of padding for this matrix.
//* Initial address of matrix C is to be programmed in the
//  beginning of calculation of each output tile. 
//  There is no concept of programming the address of C for 
//  accumulation. We write the matrix C only after all accumulations
//  have finished.
//* The register containing stride of the matrix C is not used
//  in the RTL. That is because the stride is known and is equal to
//  out_img_width * out_img_height, and RTL just uses that directly.
//* This matrix is laid out in NCHW format.

/////////////////////////////////////////////////
//Register specification
/////////////////////////////////////////////////
//---------------------------------------
//Addr 0 : Register with enables for various blocks. 
//Includes mode of operation (convolution or fully_connected)
//---------------------------------------
`define REG_ENABLES_ADDR 32'h0
//Bit 0: enable_matmul
//Bit 1: enable_norm
//Bit 2: enable_pool
//Bit 3: enable_activation
//Bit 31: enable_conv_mode

//---------------------------------------
//Addr 4: Register that triggers the whole TPU
//---------------------------------------
`define REG_STDN_TPU_ADDR 32'h4
//Bit 0: start_tpu
//Bit 31: done_tpu

//---------------------------------------
//Addr 8: Register that stores the mean of the values
//---------------------------------------
`define REG_MEAN_ADDR 32'h8
//Bit 7:0: mean

//---------------------------------------
//Addr A: Register that stores the inverse variance of the values
//---------------------------------------
`define REG_INV_VAR_ADDR 32'hA
//Bit 7:0: inv_var

//---------------------------------------
//Addr E: Register that stores the starting address of matrix A in BRAM A.
//In fully-connected mode, this register should be programmed with the
//address of the matrix being currently multiplied. That is, the 
//address of the matrix of the matmul. So, this register will be
//programmed every time the matmul is kicked off during accumulation stages.
//Use the STRIDE registers to tell the matmul to increment addresses.
//In convolution mode, this register should be programmed with the 
//address of the input activation matrix. No need to configure
//this every time the matmul is kicked off for accmulation. Just program it 
//once it the beginning. Address increments are handled automatically .
//---------------------------------------
`define REG_MATRIX_A_ADDR 32'he
//Bit `AWIDTH-1:0 address_mat_a

//---------------------------------------
//Addr 12: Register that stores the starting address of matrix B in BRAM B.
//See detailed note on the usage of this register in REG_MATRIX_A_ADDR.
//---------------------------------------
`define REG_MATRIX_B_ADDR 32'h12
//Bit `AWIDTH-1:0 address_mat_b

//---------------------------------------
//Addr 16: Register that stores the starting address of matrix C in BRAM C.
//See detailed note on the usage of this register in REG_MATRIX_A_ADDR.
//---------------------------------------
`define REG_MATRIX_C_ADDR 32'h16
//Bit `AWIDTH-1:0 address_mat_c



//---------------------------------------
//Addr 24: Register that controls the accumulation logic
//---------------------------------------
`define REG_ACCUM_ACTIONS_ADDR 32'h24
//Bit 0 save_output_to_accumulator
//Bit 1 add_accumulator_to_output

//---------------------------------------
//(Only applicable in fully-connected mode)
//Addr 28: Register that stores the stride that should be taken to address
//elements in matrix A, after every MAT_MUL_SIZE worth of data has been fetched.
//See the diagram in "Meeting-16" notes in the EE382V project Onenote notebook.
//This stride is applied when incrementing addresses for matrix A in the vertical
//direction.
//---------------------------------------
`define REG_MATRIX_A_STRIDE_ADDR 32'h28
//Bit `ADDR_STRIDE_WIDTH-1:0 address_stride_a

//---------------------------------------
//(Only applicable in fully-connected mode)
//Addr 32: Register that stores the stride that should be taken to address
//elements in matrix B, after every MAT_MUL_SIZE worth of data has been fetched.
//See the diagram in "Meeting-16" notes in the EE382V project Onenote notebook.
//This stride is applied when incrementing addresses for matrix B in the horizontal
//direction.
//---------------------------------------
`define REG_MATRIX_B_STRIDE_ADDR 32'h32
//Bit `ADDR_STRIDE_WIDTH-1:0 address_stride_b

//---------------------------------------
//(Only applicable in fully-connected mode)
//Addr 36: Register that stores the stride that should be taken to address
//elements in matrix C, after every MAT_MUL_SIZE worth of data has been fetched.
//See the diagram in "Meeting-16" notes in the EE382V project Onenote notebook.
//This stride is applied when incrementing addresses for matrix C in the vertical
//direction (this is generally same as address_stride_a).
//---------------------------------------
`define REG_MATRIX_C_STRIDE_ADDR 32'h36
//Bit `ADDR_STRIDE_WIDTH-1:0 address_stride_c

//---------------------------------------
//Addr 3A: Register that controls the activation block. Currently, the available 
//settings are the selector of activation function that will be used. There are
//two options: ReLU and TanH. To use ReLU, clear the LSB of this register. To
//use TanH, set the LSB of this register.
//---------------------------------------
`define REG_ACTIVATION_CSR_ADDR 32'h3A

//---------------------------------------
//Addr 3E: Register defining pooling window size
//---------------------------------------
`define REG_POOL_WINDOW_ADDR 32'h3E
//Bit `MAX_BITS_POOL-1:0 pool window size

//---------------------------------------
//Addr 40: Register defining convolution parameters - 1
//----------------------------------------
`define REG_CONV_PARAMS_1_ADDR 32'h40
//Bits filter_height (R) 3:0
//Bits filter width (S)  7:4
//Bits stride_horizontal 11:8
//Bits stride_vertical 15:12
//Bits pad_left 19:16
//Bits pad_right 23:20
//Bits pad_top 27:24
//Bits pad_bottom 31:28

//---------------------------------------
//Addr 44: Register defining convolution parameters - 2
//----------------------------------------
`define REG_CONV_PARAMS_2_ADDR 32'h44
//Bits num_channels_input (C) 15:0
//Bits num_channels_output (K) 31:16

//---------------------------------------
//Addr 48: Register defining convolution parameters - 3
//----------------------------------------
`define REG_CONV_PARAMS_3_ADDR 32'h48
//Bits input_image_height (H) 15:0
//Bits input_image_width (W) 31:16

//---------------------------------------
//Addr 4C: Register defining convolution parameters - 4
//----------------------------------------
`define REG_CONV_PARAMS_4_ADDR 32'h4C
//Bits output_image_height (P) 15:0
//Bits output_image_width (Q) 31:16

//---------------------------------------
//Addr 50: Register defining batch size
//----------------------------------------
`define REG_BATCH_SIZE_ADDR 32'h50
//Bits 31:0 batch_size (number of images, N)

//---------------------------------------
//Addresses 54,58,5C: Registers that stores the mask of which parts of the matrices are valid.
//
//Some examples where this is useful:
//1. Input matrix is smaller than the matmul. 
//   Say we want to multiply a 6x6 using an 8x8 matmul.
//   The matmul still operates on the whole 8x8 part, so we need
//   to ensure that there are 0s in the BRAMs in the invalid parts.
//   But the mask is used by the blocks other than matmul. For ex,
//   norm block will use the mask to avoid applying mean and variance 
//   to invalid parts (so tha they stay 0). 
//2. When we start with large matrices, the size of the matrices can
//   reduce to something less than the matmul size because of pooling.
//   In that case for the next layer, we need to tell blocks like norm,
//   what is valid and what is not.
//
//Note: This masks is applied to both x and y directions and also
//applied to both input matrices - A and B.
//---------------------------------------
`define REG_VALID_MASK_A_ROWS_ADDR 32'h20
`define REG_VALID_MASK_A_COLS_B_ROWS_ADDR 32'h54
`define REG_VALID_MASK_B_COLS_ADDR 32'h58
//Bit `MASK_WIDTH-1:0 validity_mask

//---------------------------------------
//Addr 60-64: Register defining number of design sized matrices 
//that the input matrices can be divided into.
//----------------------------------------
`define REG_NUM_MATRICES_A_ADDR 32'h60
`define REG_NUM_MATRICES_B_ADDR 32'h64

//---------------------------------------
//Addr 68: Register defining the pooling constants
//----------------------------------------
`define REG_POOLING_ACCUM_ADDR 32'h68

////////////////////////////////////////////////////////////////////////////////
// THIS FILE WAS AUTOMATICALLY GENERATED FROM generate_matmul.v.mako
// DO NOT EDIT
////////////////////////////////////////////////////////////////////////////////

module matmul_16x16_systolic(
    clk,
    reset,
    pe_reset,
    start_mat_mul,
    done_mat_mul,
    num_matrices_A,
    num_matrices_B,
    address_mat_a,
    address_mat_b,
    address_stride_a,
    address_stride_b,
    a_data,
    b_data,
    a_data_in,  // Data values coming in from previous matmul - systolic connections
    b_data_in,  // Data values coming in from previous matmul - weight matrix 
    c_data_in,  // Data values coming in from previous matmul - systolic shifting
    c_data_out, // Data values going out to next matmul - systolic shifting
    a_data_out,
    b_data_out,
    a_addr,
    b_addr,
    c_addr,
    c_data_available,
    matrixC15_0,
    matrixC15_1,
    matrixC15_2,
    matrixC15_3,
    matrixC15_4,
    matrixC15_5,
    matrixC15_6,
    matrixC15_7,
    matrixC15_8,
    matrixC15_9,
    matrixC15_10,
    matrixC15_11,
    matrixC15_12,
    matrixC15_13,
    matrixC15_14,
    matrixC15_15,
    validity_mask_a_rows,
    validity_mask_a_cols_b_rows,
    validity_mask_b_cols,
    a_loc,
    b_loc
);

input clk;
input reset;
input pe_reset;
input start_mat_mul;
output done_mat_mul;
input [31:0] num_matrices_A; // Number of 16x16 matrices the input matrix can be divided into
input [31:0] num_matrices_B; // Number of 16x16 matrices the weight matrix can be divided into
input [`AWIDTH-1:0] address_mat_a;
input [`AWIDTH-1:0] address_mat_b;
input [`ADDR_STRIDE_WIDTH-1:0] address_stride_a;
input [`ADDR_STRIDE_WIDTH-1:0] address_stride_b;
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
output [`DWIDTH-1:0] matrixC15_0;
output [`DWIDTH-1:0] matrixC15_1;
output [`DWIDTH-1:0] matrixC15_2;
output [`DWIDTH-1:0] matrixC15_3;
output [`DWIDTH-1:0] matrixC15_4;
output [`DWIDTH-1:0] matrixC15_5;
output [`DWIDTH-1:0] matrixC15_6;
output [`DWIDTH-1:0] matrixC15_7;
output [`DWIDTH-1:0] matrixC15_8;
output [`DWIDTH-1:0] matrixC15_9;
output [`DWIDTH-1:0] matrixC15_10;
output [`DWIDTH-1:0] matrixC15_11;
output [`DWIDTH-1:0] matrixC15_12;
output [`DWIDTH-1:0] matrixC15_13;
output [`DWIDTH-1:0] matrixC15_14;
output [`DWIDTH-1:0] matrixC15_15;
input [`MASK_WIDTH-1:0] validity_mask_a_rows;
input [`MASK_WIDTH-1:0] validity_mask_a_cols_b_rows;
input [`MASK_WIDTH-1:0] validity_mask_b_cols;
input [31:0] a_loc;
input [31:0] b_loc;

//////////////////////////////////////////////////////////////////////////
// Logic for clock counting and when to assert done
//////////////////////////////////////////////////////////////////////////

reg done_mat_mul;
// This is set to 31 bits in accordance with the previous simulations.
// In general, a systolic multiplier takes 4*N-2+P cycles, where N is the size 
// of the matmul and P is the number of pipeline stages in the MAC block.
reg [31:0] clk_cnt;

// Finding out number of cycles to assert matmul done.
// When we have to save the outputs to accumulators, then we don't need to
// shift out data. So, we can assert done_mat_mul early.
// Note: the count expression used to contain "num_matrices_16x16*8", but 
// to avoid multiplication, we now use "num_matrices_16x16 << 3"
wire [31:0] clk_cnt_for_done;
assign clk_cnt_for_done = 
((num_matrices_A << (2*`LOG2_MAT_MUL_SIZE -1)) + 1  + `NUM_CYCLES_IN_MAC) ;  

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
wire [`DWIDTH-1:0] a4_data;
wire [`DWIDTH-1:0] a5_data;
wire [`DWIDTH-1:0] a6_data;
wire [`DWIDTH-1:0] a7_data;
wire [`DWIDTH-1:0] a8_data;
wire [`DWIDTH-1:0] a9_data;
wire [`DWIDTH-1:0] a10_data;
wire [`DWIDTH-1:0] a11_data;
wire [`DWIDTH-1:0] a12_data;
wire [`DWIDTH-1:0] a13_data;
wire [`DWIDTH-1:0] a14_data;
wire [`DWIDTH-1:0] a15_data;
wire [`DWIDTH-1:0] b0_data;
wire [`DWIDTH-1:0] b1_data;
wire [`DWIDTH-1:0] b2_data;
wire [`DWIDTH-1:0] b3_data;
wire [`DWIDTH-1:0] b4_data;
wire [`DWIDTH-1:0] b5_data;
wire [`DWIDTH-1:0] b6_data;
wire [`DWIDTH-1:0] b7_data;
wire [`DWIDTH-1:0] b8_data;
wire [`DWIDTH-1:0] b9_data;
wire [`DWIDTH-1:0] b10_data;
wire [`DWIDTH-1:0] b11_data;
wire [`DWIDTH-1:0] b12_data;
wire [`DWIDTH-1:0] b13_data;
wire [`DWIDTH-1:0] b14_data;
wire [`DWIDTH-1:0] b15_data;
wire [`DWIDTH-1:0] a1_data_delayed_1;
wire [`DWIDTH-1:0] a2_data_delayed_1;
wire [`DWIDTH-1:0] a2_data_delayed_2;
wire [`DWIDTH-1:0] a3_data_delayed_1;
wire [`DWIDTH-1:0] a3_data_delayed_2;
wire [`DWIDTH-1:0] a3_data_delayed_3;
wire [`DWIDTH-1:0] a4_data_delayed_1;
wire [`DWIDTH-1:0] a4_data_delayed_2;
wire [`DWIDTH-1:0] a4_data_delayed_3;
wire [`DWIDTH-1:0] a4_data_delayed_4;
wire [`DWIDTH-1:0] a5_data_delayed_1;
wire [`DWIDTH-1:0] a5_data_delayed_2;
wire [`DWIDTH-1:0] a5_data_delayed_3;
wire [`DWIDTH-1:0] a5_data_delayed_4;
wire [`DWIDTH-1:0] a5_data_delayed_5;
wire [`DWIDTH-1:0] a6_data_delayed_1;
wire [`DWIDTH-1:0] a6_data_delayed_2;
wire [`DWIDTH-1:0] a6_data_delayed_3;
wire [`DWIDTH-1:0] a6_data_delayed_4;
wire [`DWIDTH-1:0] a6_data_delayed_5;
wire [`DWIDTH-1:0] a6_data_delayed_6;
wire [`DWIDTH-1:0] a7_data_delayed_1;
wire [`DWIDTH-1:0] a7_data_delayed_2;
wire [`DWIDTH-1:0] a7_data_delayed_3;
wire [`DWIDTH-1:0] a7_data_delayed_4;
wire [`DWIDTH-1:0] a7_data_delayed_5;
wire [`DWIDTH-1:0] a7_data_delayed_6;
wire [`DWIDTH-1:0] a7_data_delayed_7;
wire [`DWIDTH-1:0] a8_data_delayed_1;
wire [`DWIDTH-1:0] a8_data_delayed_2;
wire [`DWIDTH-1:0] a8_data_delayed_3;
wire [`DWIDTH-1:0] a8_data_delayed_4;
wire [`DWIDTH-1:0] a8_data_delayed_5;
wire [`DWIDTH-1:0] a8_data_delayed_6;
wire [`DWIDTH-1:0] a8_data_delayed_7;
wire [`DWIDTH-1:0] a8_data_delayed_8;
wire [`DWIDTH-1:0] a9_data_delayed_1;
wire [`DWIDTH-1:0] a9_data_delayed_2;
wire [`DWIDTH-1:0] a9_data_delayed_3;
wire [`DWIDTH-1:0] a9_data_delayed_4;
wire [`DWIDTH-1:0] a9_data_delayed_5;
wire [`DWIDTH-1:0] a9_data_delayed_6;
wire [`DWIDTH-1:0] a9_data_delayed_7;
wire [`DWIDTH-1:0] a9_data_delayed_8;
wire [`DWIDTH-1:0] a9_data_delayed_9;
wire [`DWIDTH-1:0] a10_data_delayed_1;
wire [`DWIDTH-1:0] a10_data_delayed_2;
wire [`DWIDTH-1:0] a10_data_delayed_3;
wire [`DWIDTH-1:0] a10_data_delayed_4;
wire [`DWIDTH-1:0] a10_data_delayed_5;
wire [`DWIDTH-1:0] a10_data_delayed_6;
wire [`DWIDTH-1:0] a10_data_delayed_7;
wire [`DWIDTH-1:0] a10_data_delayed_8;
wire [`DWIDTH-1:0] a10_data_delayed_9;
wire [`DWIDTH-1:0] a10_data_delayed_10;
wire [`DWIDTH-1:0] a11_data_delayed_1;
wire [`DWIDTH-1:0] a11_data_delayed_2;
wire [`DWIDTH-1:0] a11_data_delayed_3;
wire [`DWIDTH-1:0] a11_data_delayed_4;
wire [`DWIDTH-1:0] a11_data_delayed_5;
wire [`DWIDTH-1:0] a11_data_delayed_6;
wire [`DWIDTH-1:0] a11_data_delayed_7;
wire [`DWIDTH-1:0] a11_data_delayed_8;
wire [`DWIDTH-1:0] a11_data_delayed_9;
wire [`DWIDTH-1:0] a11_data_delayed_10;
wire [`DWIDTH-1:0] a11_data_delayed_11;
wire [`DWIDTH-1:0] a12_data_delayed_1;
wire [`DWIDTH-1:0] a12_data_delayed_2;
wire [`DWIDTH-1:0] a12_data_delayed_3;
wire [`DWIDTH-1:0] a12_data_delayed_4;
wire [`DWIDTH-1:0] a12_data_delayed_5;
wire [`DWIDTH-1:0] a12_data_delayed_6;
wire [`DWIDTH-1:0] a12_data_delayed_7;
wire [`DWIDTH-1:0] a12_data_delayed_8;
wire [`DWIDTH-1:0] a12_data_delayed_9;
wire [`DWIDTH-1:0] a12_data_delayed_10;
wire [`DWIDTH-1:0] a12_data_delayed_11;
wire [`DWIDTH-1:0] a12_data_delayed_12;
wire [`DWIDTH-1:0] a13_data_delayed_1;
wire [`DWIDTH-1:0] a13_data_delayed_2;
wire [`DWIDTH-1:0] a13_data_delayed_3;
wire [`DWIDTH-1:0] a13_data_delayed_4;
wire [`DWIDTH-1:0] a13_data_delayed_5;
wire [`DWIDTH-1:0] a13_data_delayed_6;
wire [`DWIDTH-1:0] a13_data_delayed_7;
wire [`DWIDTH-1:0] a13_data_delayed_8;
wire [`DWIDTH-1:0] a13_data_delayed_9;
wire [`DWIDTH-1:0] a13_data_delayed_10;
wire [`DWIDTH-1:0] a13_data_delayed_11;
wire [`DWIDTH-1:0] a13_data_delayed_12;
wire [`DWIDTH-1:0] a13_data_delayed_13;
wire [`DWIDTH-1:0] a14_data_delayed_1;
wire [`DWIDTH-1:0] a14_data_delayed_2;
wire [`DWIDTH-1:0] a14_data_delayed_3;
wire [`DWIDTH-1:0] a14_data_delayed_4;
wire [`DWIDTH-1:0] a14_data_delayed_5;
wire [`DWIDTH-1:0] a14_data_delayed_6;
wire [`DWIDTH-1:0] a14_data_delayed_7;
wire [`DWIDTH-1:0] a14_data_delayed_8;
wire [`DWIDTH-1:0] a14_data_delayed_9;
wire [`DWIDTH-1:0] a14_data_delayed_10;
wire [`DWIDTH-1:0] a14_data_delayed_11;
wire [`DWIDTH-1:0] a14_data_delayed_12;
wire [`DWIDTH-1:0] a14_data_delayed_13;
wire [`DWIDTH-1:0] a14_data_delayed_14;
wire [`DWIDTH-1:0] a15_data_delayed_1;
wire [`DWIDTH-1:0] a15_data_delayed_2;
wire [`DWIDTH-1:0] a15_data_delayed_3;
wire [`DWIDTH-1:0] a15_data_delayed_4;
wire [`DWIDTH-1:0] a15_data_delayed_5;
wire [`DWIDTH-1:0] a15_data_delayed_6;
wire [`DWIDTH-1:0] a15_data_delayed_7;
wire [`DWIDTH-1:0] a15_data_delayed_8;
wire [`DWIDTH-1:0] a15_data_delayed_9;
wire [`DWIDTH-1:0] a15_data_delayed_10;
wire [`DWIDTH-1:0] a15_data_delayed_11;
wire [`DWIDTH-1:0] a15_data_delayed_12;
wire [`DWIDTH-1:0] a15_data_delayed_13;
wire [`DWIDTH-1:0] a15_data_delayed_14;
wire [`DWIDTH-1:0] a15_data_delayed_15;
wire [`DWIDTH-1:0] b1_data_delayed_1;
wire [`DWIDTH-1:0] b2_data_delayed_1;
wire [`DWIDTH-1:0] b2_data_delayed_2;
wire [`DWIDTH-1:0] b3_data_delayed_1;
wire [`DWIDTH-1:0] b3_data_delayed_2;
wire [`DWIDTH-1:0] b3_data_delayed_3;
wire [`DWIDTH-1:0] b4_data_delayed_1;
wire [`DWIDTH-1:0] b4_data_delayed_2;
wire [`DWIDTH-1:0] b4_data_delayed_3;
wire [`DWIDTH-1:0] b4_data_delayed_4;
wire [`DWIDTH-1:0] b5_data_delayed_1;
wire [`DWIDTH-1:0] b5_data_delayed_2;
wire [`DWIDTH-1:0] b5_data_delayed_3;
wire [`DWIDTH-1:0] b5_data_delayed_4;
wire [`DWIDTH-1:0] b5_data_delayed_5;
wire [`DWIDTH-1:0] b6_data_delayed_1;
wire [`DWIDTH-1:0] b6_data_delayed_2;
wire [`DWIDTH-1:0] b6_data_delayed_3;
wire [`DWIDTH-1:0] b6_data_delayed_4;
wire [`DWIDTH-1:0] b6_data_delayed_5;
wire [`DWIDTH-1:0] b6_data_delayed_6;
wire [`DWIDTH-1:0] b7_data_delayed_1;
wire [`DWIDTH-1:0] b7_data_delayed_2;
wire [`DWIDTH-1:0] b7_data_delayed_3;
wire [`DWIDTH-1:0] b7_data_delayed_4;
wire [`DWIDTH-1:0] b7_data_delayed_5;
wire [`DWIDTH-1:0] b7_data_delayed_6;
wire [`DWIDTH-1:0] b7_data_delayed_7;
wire [`DWIDTH-1:0] b8_data_delayed_1;
wire [`DWIDTH-1:0] b8_data_delayed_2;
wire [`DWIDTH-1:0] b8_data_delayed_3;
wire [`DWIDTH-1:0] b8_data_delayed_4;
wire [`DWIDTH-1:0] b8_data_delayed_5;
wire [`DWIDTH-1:0] b8_data_delayed_6;
wire [`DWIDTH-1:0] b8_data_delayed_7;
wire [`DWIDTH-1:0] b8_data_delayed_8;
wire [`DWIDTH-1:0] b9_data_delayed_1;
wire [`DWIDTH-1:0] b9_data_delayed_2;
wire [`DWIDTH-1:0] b9_data_delayed_3;
wire [`DWIDTH-1:0] b9_data_delayed_4;
wire [`DWIDTH-1:0] b9_data_delayed_5;
wire [`DWIDTH-1:0] b9_data_delayed_6;
wire [`DWIDTH-1:0] b9_data_delayed_7;
wire [`DWIDTH-1:0] b9_data_delayed_8;
wire [`DWIDTH-1:0] b9_data_delayed_9;
wire [`DWIDTH-1:0] b10_data_delayed_1;
wire [`DWIDTH-1:0] b10_data_delayed_2;
wire [`DWIDTH-1:0] b10_data_delayed_3;
wire [`DWIDTH-1:0] b10_data_delayed_4;
wire [`DWIDTH-1:0] b10_data_delayed_5;
wire [`DWIDTH-1:0] b10_data_delayed_6;
wire [`DWIDTH-1:0] b10_data_delayed_7;
wire [`DWIDTH-1:0] b10_data_delayed_8;
wire [`DWIDTH-1:0] b10_data_delayed_9;
wire [`DWIDTH-1:0] b10_data_delayed_10;
wire [`DWIDTH-1:0] b11_data_delayed_1;
wire [`DWIDTH-1:0] b11_data_delayed_2;
wire [`DWIDTH-1:0] b11_data_delayed_3;
wire [`DWIDTH-1:0] b11_data_delayed_4;
wire [`DWIDTH-1:0] b11_data_delayed_5;
wire [`DWIDTH-1:0] b11_data_delayed_6;
wire [`DWIDTH-1:0] b11_data_delayed_7;
wire [`DWIDTH-1:0] b11_data_delayed_8;
wire [`DWIDTH-1:0] b11_data_delayed_9;
wire [`DWIDTH-1:0] b11_data_delayed_10;
wire [`DWIDTH-1:0] b11_data_delayed_11;
wire [`DWIDTH-1:0] b12_data_delayed_1;
wire [`DWIDTH-1:0] b12_data_delayed_2;
wire [`DWIDTH-1:0] b12_data_delayed_3;
wire [`DWIDTH-1:0] b12_data_delayed_4;
wire [`DWIDTH-1:0] b12_data_delayed_5;
wire [`DWIDTH-1:0] b12_data_delayed_6;
wire [`DWIDTH-1:0] b12_data_delayed_7;
wire [`DWIDTH-1:0] b12_data_delayed_8;
wire [`DWIDTH-1:0] b12_data_delayed_9;
wire [`DWIDTH-1:0] b12_data_delayed_10;
wire [`DWIDTH-1:0] b12_data_delayed_11;
wire [`DWIDTH-1:0] b12_data_delayed_12;
wire [`DWIDTH-1:0] b13_data_delayed_1;
wire [`DWIDTH-1:0] b13_data_delayed_2;
wire [`DWIDTH-1:0] b13_data_delayed_3;
wire [`DWIDTH-1:0] b13_data_delayed_4;
wire [`DWIDTH-1:0] b13_data_delayed_5;
wire [`DWIDTH-1:0] b13_data_delayed_6;
wire [`DWIDTH-1:0] b13_data_delayed_7;
wire [`DWIDTH-1:0] b13_data_delayed_8;
wire [`DWIDTH-1:0] b13_data_delayed_9;
wire [`DWIDTH-1:0] b13_data_delayed_10;
wire [`DWIDTH-1:0] b13_data_delayed_11;
wire [`DWIDTH-1:0] b13_data_delayed_12;
wire [`DWIDTH-1:0] b13_data_delayed_13;
wire [`DWIDTH-1:0] b14_data_delayed_1;
wire [`DWIDTH-1:0] b14_data_delayed_2;
wire [`DWIDTH-1:0] b14_data_delayed_3;
wire [`DWIDTH-1:0] b14_data_delayed_4;
wire [`DWIDTH-1:0] b14_data_delayed_5;
wire [`DWIDTH-1:0] b14_data_delayed_6;
wire [`DWIDTH-1:0] b14_data_delayed_7;
wire [`DWIDTH-1:0] b14_data_delayed_8;
wire [`DWIDTH-1:0] b14_data_delayed_9;
wire [`DWIDTH-1:0] b14_data_delayed_10;
wire [`DWIDTH-1:0] b14_data_delayed_11;
wire [`DWIDTH-1:0] b14_data_delayed_12;
wire [`DWIDTH-1:0] b14_data_delayed_13;
wire [`DWIDTH-1:0] b14_data_delayed_14;
wire [`DWIDTH-1:0] b15_data_delayed_1;
wire [`DWIDTH-1:0] b15_data_delayed_2;
wire [`DWIDTH-1:0] b15_data_delayed_3;
wire [`DWIDTH-1:0] b15_data_delayed_4;
wire [`DWIDTH-1:0] b15_data_delayed_5;
wire [`DWIDTH-1:0] b15_data_delayed_6;
wire [`DWIDTH-1:0] b15_data_delayed_7;
wire [`DWIDTH-1:0] b15_data_delayed_8;
wire [`DWIDTH-1:0] b15_data_delayed_9;
wire [`DWIDTH-1:0] b15_data_delayed_10;
wire [`DWIDTH-1:0] b15_data_delayed_11;
wire [`DWIDTH-1:0] b15_data_delayed_12;
wire [`DWIDTH-1:0] b15_data_delayed_13;
wire [`DWIDTH-1:0] b15_data_delayed_14;
wire [`DWIDTH-1:0] b15_data_delayed_15;

reg b_data_sel; // MUX select for Ping-Pong buffers containing the weights in the matmul
reg b_data_valid_ping;
reg b_data_valid_pong;

always @ (posedge clk) begin
	if ((clk_cnt >= 16'd1 && clk_cnt <= 16'd8)||(clk_cnt >= 16'd17 && clk_cnt <= 16'd24))
		b_data_valid_pong <= 1'b1;
	else 
		b_data_valid_pong <= 1'b0;
end

always @ (posedge clk) begin
	if ((clk_cnt >= 16'd9 && clk_cnt <= 16'd16))
		b_data_valid_ping <= 1'b1;
	else 
		b_data_valid_ping <= 1'b0;
end

always @ (posedge clk) begin
	if ((clk_cnt >= 16'd10 && clk_cnt <= 16'd17)||(clk_cnt >= 16'd26 && clk_cnt <= 16'd33))
		b_data_sel <= 1'b1;
	else 
		b_data_sel <= 1'b0;
end

//////////////////////////////////////////////////////////////////////////
// Instantiation of systolic data setup
//////////////////////////////////////////////////////////////////////////
systolic_data_setup u_systolic_data_setup(
    .clk(clk),
    .reset(reset),
    .start_mat_mul(start_mat_mul),
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
    .a4_data_delayed_4(a4_data_delayed_4),
    .a5_data_delayed_5(a5_data_delayed_5),
    .a6_data_delayed_6(a6_data_delayed_6),
    .a7_data_delayed_7(a7_data_delayed_7),
    .a8_data_delayed_8(a8_data_delayed_8),
    .a9_data_delayed_9(a9_data_delayed_9),
    .a10_data_delayed_10(a10_data_delayed_10),
    .a11_data_delayed_11(a11_data_delayed_11),
    .a12_data_delayed_12(a12_data_delayed_12),
    .a13_data_delayed_13(a13_data_delayed_13),
    .a14_data_delayed_14(a14_data_delayed_14),
    .a15_data_delayed_15(a15_data_delayed_15),
    .b0_data(b0_data),
    .b1_data_delayed_1(b1_data_delayed_1),
    .b2_data_delayed_2(b2_data_delayed_2),
    .b3_data_delayed_3(b3_data_delayed_3),
    .b4_data_delayed_4(b4_data_delayed_4),
    .b5_data_delayed_5(b5_data_delayed_5),
    .b6_data_delayed_6(b6_data_delayed_6),
    .b7_data_delayed_7(b7_data_delayed_7),
    .b8_data_delayed_8(b8_data_delayed_8),
    .b9_data_delayed_9(b9_data_delayed_9),
    .b10_data_delayed_10(b10_data_delayed_10),
    .b11_data_delayed_11(b11_data_delayed_11),
    .b12_data_delayed_12(b12_data_delayed_12),
    .b13_data_delayed_13(b13_data_delayed_13),
    .b14_data_delayed_14(b14_data_delayed_14),
    .b15_data_delayed_15(b15_data_delayed_15),
    .validity_mask_a_rows(validity_mask_a_rows),
    .validity_mask_a_cols_b_rows(validity_mask_a_cols_b_rows),
    .validity_mask_b_cols(validity_mask_b_cols),
    .num_matrices_A(num_matrices_A),
    .num_matrices_B(num_matrices_B),
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
wire [`DWIDTH-1:0] a4;
wire [`DWIDTH-1:0] a5;
wire [`DWIDTH-1:0] a6;
wire [`DWIDTH-1:0] a7;
wire [`DWIDTH-1:0] a8;
wire [`DWIDTH-1:0] a9;
wire [`DWIDTH-1:0] a10;
wire [`DWIDTH-1:0] a11;
wire [`DWIDTH-1:0] a12;
wire [`DWIDTH-1:0] a13;
wire [`DWIDTH-1:0] a14;
wire [`DWIDTH-1:0] a15;
wire [`DWIDTH-1:0] b0;
wire [`DWIDTH-1:0] b1;
wire [`DWIDTH-1:0] b2;
wire [`DWIDTH-1:0] b3;
wire [`DWIDTH-1:0] b4;
wire [`DWIDTH-1:0] b5;
wire [`DWIDTH-1:0] b6;
wire [`DWIDTH-1:0] b7;
wire [`DWIDTH-1:0] b8;
wire [`DWIDTH-1:0] b9;
wire [`DWIDTH-1:0] b10;
wire [`DWIDTH-1:0] b11;
wire [`DWIDTH-1:0] b12;
wire [`DWIDTH-1:0] b13;
wire [`DWIDTH-1:0] b14;
wire [`DWIDTH-1:0] b15;
wire [`DWIDTH-1:0] c0;
wire [`DWIDTH-1:0] c1;
wire [`DWIDTH-1:0] c2;
wire [`DWIDTH-1:0] c3;
wire [`DWIDTH-1:0] c4;
wire [`DWIDTH-1:0] c5;
wire [`DWIDTH-1:0] c6;
wire [`DWIDTH-1:0] c7;
wire [`DWIDTH-1:0] c8;
wire [`DWIDTH-1:0] c9;
wire [`DWIDTH-1:0] c10;
wire [`DWIDTH-1:0] c11;
wire [`DWIDTH-1:0] c12;
wire [`DWIDTH-1:0] c13;
wire [`DWIDTH-1:0] c14;
wire [`DWIDTH-1:0] c15;

wire [`DWIDTH-1:0] a0_data_in;
wire [`DWIDTH-1:0] a1_data_in;
wire [`DWIDTH-1:0] a2_data_in;
wire [`DWIDTH-1:0] a3_data_in;
wire [`DWIDTH-1:0] a4_data_in;
wire [`DWIDTH-1:0] a5_data_in;
wire [`DWIDTH-1:0] a6_data_in;
wire [`DWIDTH-1:0] a7_data_in;
wire [`DWIDTH-1:0] a8_data_in;
wire [`DWIDTH-1:0] a9_data_in;
wire [`DWIDTH-1:0] a10_data_in;
wire [`DWIDTH-1:0] a11_data_in;
wire [`DWIDTH-1:0] a12_data_in;
wire [`DWIDTH-1:0] a13_data_in;
wire [`DWIDTH-1:0] a14_data_in;
wire [`DWIDTH-1:0] a15_data_in;
assign a0_data_in = a_data_in[`DWIDTH-1:0];
assign a1_data_in = a_data_in[2*`DWIDTH-1:1*`DWIDTH];
assign a2_data_in = a_data_in[3*`DWIDTH-1:2*`DWIDTH];
assign a3_data_in = a_data_in[4*`DWIDTH-1:3*`DWIDTH];
assign a4_data_in = a_data_in[5*`DWIDTH-1:4*`DWIDTH];
assign a5_data_in = a_data_in[6*`DWIDTH-1:5*`DWIDTH];
assign a6_data_in = a_data_in[7*`DWIDTH-1:6*`DWIDTH];
assign a7_data_in = a_data_in[8*`DWIDTH-1:7*`DWIDTH];
assign a8_data_in = a_data_in[9*`DWIDTH-1:8*`DWIDTH];
assign a9_data_in = a_data_in[10*`DWIDTH-1:9*`DWIDTH];
assign a10_data_in = a_data_in[11*`DWIDTH-1:10*`DWIDTH];
assign a11_data_in = a_data_in[12*`DWIDTH-1:11*`DWIDTH];
assign a12_data_in = a_data_in[13*`DWIDTH-1:12*`DWIDTH];
assign a13_data_in = a_data_in[14*`DWIDTH-1:13*`DWIDTH];
assign a14_data_in = a_data_in[15*`DWIDTH-1:14*`DWIDTH];
assign a15_data_in = a_data_in[16*`DWIDTH-1:15*`DWIDTH];

wire [`DWIDTH-1:0] b0_data_in;
wire [`DWIDTH-1:0] b1_data_in;
wire [`DWIDTH-1:0] b2_data_in;
wire [`DWIDTH-1:0] b3_data_in;
wire [`DWIDTH-1:0] b4_data_in;
wire [`DWIDTH-1:0] b5_data_in;
wire [`DWIDTH-1:0] b6_data_in;
wire [`DWIDTH-1:0] b7_data_in;
wire [`DWIDTH-1:0] b8_data_in;
wire [`DWIDTH-1:0] b9_data_in;
wire [`DWIDTH-1:0] b10_data_in;
wire [`DWIDTH-1:0] b11_data_in;
wire [`DWIDTH-1:0] b12_data_in;
wire [`DWIDTH-1:0] b13_data_in;
wire [`DWIDTH-1:0] b14_data_in;
wire [`DWIDTH-1:0] b15_data_in;
assign b0_data_in = b_data_in[`DWIDTH-1:0];
assign b1_data_in = b_data_in[2*`DWIDTH-1:1*`DWIDTH];
assign b2_data_in = b_data_in[3*`DWIDTH-1:2*`DWIDTH];
assign b3_data_in = b_data_in[4*`DWIDTH-1:3*`DWIDTH];
assign b4_data_in = b_data_in[5*`DWIDTH-1:4*`DWIDTH];
assign b5_data_in = b_data_in[6*`DWIDTH-1:5*`DWIDTH];
assign b6_data_in = b_data_in[7*`DWIDTH-1:6*`DWIDTH];
assign b7_data_in = b_data_in[8*`DWIDTH-1:7*`DWIDTH];
assign b8_data_in = b_data_in[9*`DWIDTH-1:8*`DWIDTH];
assign b9_data_in = b_data_in[10*`DWIDTH-1:9*`DWIDTH];
assign b10_data_in = b_data_in[11*`DWIDTH-1:10*`DWIDTH];
assign b11_data_in = b_data_in[12*`DWIDTH-1:11*`DWIDTH];
assign b12_data_in = b_data_in[13*`DWIDTH-1:12*`DWIDTH];
assign b13_data_in = b_data_in[14*`DWIDTH-1:13*`DWIDTH];
assign b14_data_in = b_data_in[15*`DWIDTH-1:14*`DWIDTH];
assign b15_data_in = b_data_in[16*`DWIDTH-1:15*`DWIDTH];

// If b_loc is 0, that means this matmul block is on the top-row of the
// final large matmul. In that case, b will take inputs from mem.
// If b_loc != 0, that means this matmul block is not on the top-row of the
// final large matmul. In that case, b will take inputs from the matmul on top
// of this one.
assign a0 = (b_loc==0) ? a0_data           : a0_data_in;
assign a1 = (b_loc==0) ? a1_data_delayed_1 : a1_data_in;
assign a2 = (b_loc==0) ? a2_data_delayed_2 : a2_data_in;
assign a3 = (b_loc==0) ? a3_data_delayed_3 : a3_data_in;
assign a4 = (b_loc==0) ? a4_data_delayed_4 : a4_data_in;
assign a5 = (b_loc==0) ? a5_data_delayed_5 : a5_data_in;
assign a6 = (b_loc==0) ? a6_data_delayed_6 : a6_data_in;
assign a7 = (b_loc==0) ? a7_data_delayed_7 : a7_data_in;
assign a8 = (b_loc==0) ? a8_data_delayed_8 : a8_data_in;
assign a9 = (b_loc==0) ? a9_data_delayed_9 : a9_data_in;
assign a10 = (b_loc==0) ? a10_data_delayed_10 : a10_data_in;
assign a11 = (b_loc==0) ? a11_data_delayed_11 : a11_data_in;
assign a12 = (b_loc==0) ? a12_data_delayed_12 : a12_data_in;
assign a13 = (b_loc==0) ? a13_data_delayed_13 : a13_data_in;
assign a14 = (b_loc==0) ? a14_data_delayed_14 : a14_data_in;
assign a15 = (b_loc==0) ? a15_data_delayed_15 : a15_data_in;

/// If a_loc is 0, that means this matmul block is on the left-col of the
// final large matmul. In that case, a will take inputs from mem.
// If a_loc != 0, that means this matmul block is not on the left-col of the
// final large matmul. In that case, a will take inputs from the matmul on left
// of this one.
assign b0 = (a_loc==0) ? b0_data           : b0_data_in;
assign b1 = (a_loc==0) ? b1_data_delayed_1 : b1_data_in;
assign b2 = (a_loc==0) ? b2_data_delayed_2 : b2_data_in;
assign b3 = (a_loc==0) ? b3_data_delayed_3 : b3_data_in;
assign b4 = (a_loc==0) ? b4_data_delayed_4 : b4_data_in;
assign b5 = (a_loc==0) ? b5_data_delayed_5 : b5_data_in;
assign b6 = (a_loc==0) ? b6_data_delayed_6 : b6_data_in;
assign b7 = (a_loc==0) ? b7_data_delayed_7 : b7_data_in;
assign b8 = (a_loc==0) ? b8_data_delayed_8 : b8_data_in;
assign b9 = (a_loc==0) ? b9_data_delayed_9 : b9_data_in;
assign b10 = (a_loc==0) ? b10_data_delayed_10 : b10_data_in;
assign b11 = (a_loc==0) ? b11_data_delayed_11 : b11_data_in;
assign b12 = (a_loc==0) ? b12_data_delayed_12 : b12_data_in;
assign b13 = (a_loc==0) ? b13_data_delayed_13 : b13_data_in;
assign b14 = (a_loc==0) ? b14_data_delayed_14 : b14_data_in;
assign b15 = (a_loc==0) ? b15_data_delayed_15 : b15_data_in;

assign c0 = c_data_in[`DWIDTH-1:0];
assign c1 = c_data_in[2*`DWIDTH-1:1*`DWIDTH];
assign c2 = c_data_in[3*`DWIDTH-1:2*`DWIDTH];
assign c3 = c_data_in[4*`DWIDTH-1:3*`DWIDTH];
assign c4 = c_data_in[5*`DWIDTH-1:4*`DWIDTH];
assign c5 = c_data_in[6*`DWIDTH-1:5*`DWIDTH];
assign c6 = c_data_in[7*`DWIDTH-1:6*`DWIDTH];
assign c7 = c_data_in[8*`DWIDTH-1:7*`DWIDTH];
assign c8 = c_data_in[9*`DWIDTH-1:8*`DWIDTH];
assign c9 = c_data_in[10*`DWIDTH-1:9*`DWIDTH];
assign c10 = c_data_in[11*`DWIDTH-1:10*`DWIDTH];
assign c11 = c_data_in[12*`DWIDTH-1:11*`DWIDTH];
assign c12 = c_data_in[13*`DWIDTH-1:12*`DWIDTH];
assign c13 = c_data_in[14*`DWIDTH-1:13*`DWIDTH];
assign c14 = c_data_in[15*`DWIDTH-1:14*`DWIDTH];
assign c15 = c_data_in[16*`DWIDTH-1:15*`DWIDTH];

wire [`DWIDTH-1:0] matrixC0_0;
wire [`DWIDTH-1:0] matrixC0_1;
wire [`DWIDTH-1:0] matrixC0_2;
wire [`DWIDTH-1:0] matrixC0_3;
wire [`DWIDTH-1:0] matrixC0_4;
wire [`DWIDTH-1:0] matrixC0_5;
wire [`DWIDTH-1:0] matrixC0_6;
wire [`DWIDTH-1:0] matrixC0_7;
wire [`DWIDTH-1:0] matrixC0_8;
wire [`DWIDTH-1:0] matrixC0_9;
wire [`DWIDTH-1:0] matrixC0_10;
wire [`DWIDTH-1:0] matrixC0_11;
wire [`DWIDTH-1:0] matrixC0_12;
wire [`DWIDTH-1:0] matrixC0_13;
wire [`DWIDTH-1:0] matrixC0_14;
wire [`DWIDTH-1:0] matrixC0_15;
wire [`DWIDTH-1:0] matrixC1_0;
wire [`DWIDTH-1:0] matrixC1_1;
wire [`DWIDTH-1:0] matrixC1_2;
wire [`DWIDTH-1:0] matrixC1_3;
wire [`DWIDTH-1:0] matrixC1_4;
wire [`DWIDTH-1:0] matrixC1_5;
wire [`DWIDTH-1:0] matrixC1_6;
wire [`DWIDTH-1:0] matrixC1_7;
wire [`DWIDTH-1:0] matrixC1_8;
wire [`DWIDTH-1:0] matrixC1_9;
wire [`DWIDTH-1:0] matrixC1_10;
wire [`DWIDTH-1:0] matrixC1_11;
wire [`DWIDTH-1:0] matrixC1_12;
wire [`DWIDTH-1:0] matrixC1_13;
wire [`DWIDTH-1:0] matrixC1_14;
wire [`DWIDTH-1:0] matrixC1_15;
wire [`DWIDTH-1:0] matrixC2_0;
wire [`DWIDTH-1:0] matrixC2_1;
wire [`DWIDTH-1:0] matrixC2_2;
wire [`DWIDTH-1:0] matrixC2_3;
wire [`DWIDTH-1:0] matrixC2_4;
wire [`DWIDTH-1:0] matrixC2_5;
wire [`DWIDTH-1:0] matrixC2_6;
wire [`DWIDTH-1:0] matrixC2_7;
wire [`DWIDTH-1:0] matrixC2_8;
wire [`DWIDTH-1:0] matrixC2_9;
wire [`DWIDTH-1:0] matrixC2_10;
wire [`DWIDTH-1:0] matrixC2_11;
wire [`DWIDTH-1:0] matrixC2_12;
wire [`DWIDTH-1:0] matrixC2_13;
wire [`DWIDTH-1:0] matrixC2_14;
wire [`DWIDTH-1:0] matrixC2_15;
wire [`DWIDTH-1:0] matrixC3_0;
wire [`DWIDTH-1:0] matrixC3_1;
wire [`DWIDTH-1:0] matrixC3_2;
wire [`DWIDTH-1:0] matrixC3_3;
wire [`DWIDTH-1:0] matrixC3_4;
wire [`DWIDTH-1:0] matrixC3_5;
wire [`DWIDTH-1:0] matrixC3_6;
wire [`DWIDTH-1:0] matrixC3_7;
wire [`DWIDTH-1:0] matrixC3_8;
wire [`DWIDTH-1:0] matrixC3_9;
wire [`DWIDTH-1:0] matrixC3_10;
wire [`DWIDTH-1:0] matrixC3_11;
wire [`DWIDTH-1:0] matrixC3_12;
wire [`DWIDTH-1:0] matrixC3_13;
wire [`DWIDTH-1:0] matrixC3_14;
wire [`DWIDTH-1:0] matrixC3_15;
wire [`DWIDTH-1:0] matrixC4_0;
wire [`DWIDTH-1:0] matrixC4_1;
wire [`DWIDTH-1:0] matrixC4_2;
wire [`DWIDTH-1:0] matrixC4_3;
wire [`DWIDTH-1:0] matrixC4_4;
wire [`DWIDTH-1:0] matrixC4_5;
wire [`DWIDTH-1:0] matrixC4_6;
wire [`DWIDTH-1:0] matrixC4_7;
wire [`DWIDTH-1:0] matrixC4_8;
wire [`DWIDTH-1:0] matrixC4_9;
wire [`DWIDTH-1:0] matrixC4_10;
wire [`DWIDTH-1:0] matrixC4_11;
wire [`DWIDTH-1:0] matrixC4_12;
wire [`DWIDTH-1:0] matrixC4_13;
wire [`DWIDTH-1:0] matrixC4_14;
wire [`DWIDTH-1:0] matrixC4_15;
wire [`DWIDTH-1:0] matrixC5_0;
wire [`DWIDTH-1:0] matrixC5_1;
wire [`DWIDTH-1:0] matrixC5_2;
wire [`DWIDTH-1:0] matrixC5_3;
wire [`DWIDTH-1:0] matrixC5_4;
wire [`DWIDTH-1:0] matrixC5_5;
wire [`DWIDTH-1:0] matrixC5_6;
wire [`DWIDTH-1:0] matrixC5_7;
wire [`DWIDTH-1:0] matrixC5_8;
wire [`DWIDTH-1:0] matrixC5_9;
wire [`DWIDTH-1:0] matrixC5_10;
wire [`DWIDTH-1:0] matrixC5_11;
wire [`DWIDTH-1:0] matrixC5_12;
wire [`DWIDTH-1:0] matrixC5_13;
wire [`DWIDTH-1:0] matrixC5_14;
wire [`DWIDTH-1:0] matrixC5_15;
wire [`DWIDTH-1:0] matrixC6_0;
wire [`DWIDTH-1:0] matrixC6_1;
wire [`DWIDTH-1:0] matrixC6_2;
wire [`DWIDTH-1:0] matrixC6_3;
wire [`DWIDTH-1:0] matrixC6_4;
wire [`DWIDTH-1:0] matrixC6_5;
wire [`DWIDTH-1:0] matrixC6_6;
wire [`DWIDTH-1:0] matrixC6_7;
wire [`DWIDTH-1:0] matrixC6_8;
wire [`DWIDTH-1:0] matrixC6_9;
wire [`DWIDTH-1:0] matrixC6_10;
wire [`DWIDTH-1:0] matrixC6_11;
wire [`DWIDTH-1:0] matrixC6_12;
wire [`DWIDTH-1:0] matrixC6_13;
wire [`DWIDTH-1:0] matrixC6_14;
wire [`DWIDTH-1:0] matrixC6_15;
wire [`DWIDTH-1:0] matrixC7_0;
wire [`DWIDTH-1:0] matrixC7_1;
wire [`DWIDTH-1:0] matrixC7_2;
wire [`DWIDTH-1:0] matrixC7_3;
wire [`DWIDTH-1:0] matrixC7_4;
wire [`DWIDTH-1:0] matrixC7_5;
wire [`DWIDTH-1:0] matrixC7_6;
wire [`DWIDTH-1:0] matrixC7_7;
wire [`DWIDTH-1:0] matrixC7_8;
wire [`DWIDTH-1:0] matrixC7_9;
wire [`DWIDTH-1:0] matrixC7_10;
wire [`DWIDTH-1:0] matrixC7_11;
wire [`DWIDTH-1:0] matrixC7_12;
wire [`DWIDTH-1:0] matrixC7_13;
wire [`DWIDTH-1:0] matrixC7_14;
wire [`DWIDTH-1:0] matrixC7_15;
wire [`DWIDTH-1:0] matrixC8_0;
wire [`DWIDTH-1:0] matrixC8_1;
wire [`DWIDTH-1:0] matrixC8_2;
wire [`DWIDTH-1:0] matrixC8_3;
wire [`DWIDTH-1:0] matrixC8_4;
wire [`DWIDTH-1:0] matrixC8_5;
wire [`DWIDTH-1:0] matrixC8_6;
wire [`DWIDTH-1:0] matrixC8_7;
wire [`DWIDTH-1:0] matrixC8_8;
wire [`DWIDTH-1:0] matrixC8_9;
wire [`DWIDTH-1:0] matrixC8_10;
wire [`DWIDTH-1:0] matrixC8_11;
wire [`DWIDTH-1:0] matrixC8_12;
wire [`DWIDTH-1:0] matrixC8_13;
wire [`DWIDTH-1:0] matrixC8_14;
wire [`DWIDTH-1:0] matrixC8_15;
wire [`DWIDTH-1:0] matrixC9_0;
wire [`DWIDTH-1:0] matrixC9_1;
wire [`DWIDTH-1:0] matrixC9_2;
wire [`DWIDTH-1:0] matrixC9_3;
wire [`DWIDTH-1:0] matrixC9_4;
wire [`DWIDTH-1:0] matrixC9_5;
wire [`DWIDTH-1:0] matrixC9_6;
wire [`DWIDTH-1:0] matrixC9_7;
wire [`DWIDTH-1:0] matrixC9_8;
wire [`DWIDTH-1:0] matrixC9_9;
wire [`DWIDTH-1:0] matrixC9_10;
wire [`DWIDTH-1:0] matrixC9_11;
wire [`DWIDTH-1:0] matrixC9_12;
wire [`DWIDTH-1:0] matrixC9_13;
wire [`DWIDTH-1:0] matrixC9_14;
wire [`DWIDTH-1:0] matrixC9_15;
wire [`DWIDTH-1:0] matrixC10_0;
wire [`DWIDTH-1:0] matrixC10_1;
wire [`DWIDTH-1:0] matrixC10_2;
wire [`DWIDTH-1:0] matrixC10_3;
wire [`DWIDTH-1:0] matrixC10_4;
wire [`DWIDTH-1:0] matrixC10_5;
wire [`DWIDTH-1:0] matrixC10_6;
wire [`DWIDTH-1:0] matrixC10_7;
wire [`DWIDTH-1:0] matrixC10_8;
wire [`DWIDTH-1:0] matrixC10_9;
wire [`DWIDTH-1:0] matrixC10_10;
wire [`DWIDTH-1:0] matrixC10_11;
wire [`DWIDTH-1:0] matrixC10_12;
wire [`DWIDTH-1:0] matrixC10_13;
wire [`DWIDTH-1:0] matrixC10_14;
wire [`DWIDTH-1:0] matrixC10_15;
wire [`DWIDTH-1:0] matrixC11_0;
wire [`DWIDTH-1:0] matrixC11_1;
wire [`DWIDTH-1:0] matrixC11_2;
wire [`DWIDTH-1:0] matrixC11_3;
wire [`DWIDTH-1:0] matrixC11_4;
wire [`DWIDTH-1:0] matrixC11_5;
wire [`DWIDTH-1:0] matrixC11_6;
wire [`DWIDTH-1:0] matrixC11_7;
wire [`DWIDTH-1:0] matrixC11_8;
wire [`DWIDTH-1:0] matrixC11_9;
wire [`DWIDTH-1:0] matrixC11_10;
wire [`DWIDTH-1:0] matrixC11_11;
wire [`DWIDTH-1:0] matrixC11_12;
wire [`DWIDTH-1:0] matrixC11_13;
wire [`DWIDTH-1:0] matrixC11_14;
wire [`DWIDTH-1:0] matrixC11_15;
wire [`DWIDTH-1:0] matrixC12_0;
wire [`DWIDTH-1:0] matrixC12_1;
wire [`DWIDTH-1:0] matrixC12_2;
wire [`DWIDTH-1:0] matrixC12_3;
wire [`DWIDTH-1:0] matrixC12_4;
wire [`DWIDTH-1:0] matrixC12_5;
wire [`DWIDTH-1:0] matrixC12_6;
wire [`DWIDTH-1:0] matrixC12_7;
wire [`DWIDTH-1:0] matrixC12_8;
wire [`DWIDTH-1:0] matrixC12_9;
wire [`DWIDTH-1:0] matrixC12_10;
wire [`DWIDTH-1:0] matrixC12_11;
wire [`DWIDTH-1:0] matrixC12_12;
wire [`DWIDTH-1:0] matrixC12_13;
wire [`DWIDTH-1:0] matrixC12_14;
wire [`DWIDTH-1:0] matrixC12_15;
wire [`DWIDTH-1:0] matrixC13_0;
wire [`DWIDTH-1:0] matrixC13_1;
wire [`DWIDTH-1:0] matrixC13_2;
wire [`DWIDTH-1:0] matrixC13_3;
wire [`DWIDTH-1:0] matrixC13_4;
wire [`DWIDTH-1:0] matrixC13_5;
wire [`DWIDTH-1:0] matrixC13_6;
wire [`DWIDTH-1:0] matrixC13_7;
wire [`DWIDTH-1:0] matrixC13_8;
wire [`DWIDTH-1:0] matrixC13_9;
wire [`DWIDTH-1:0] matrixC13_10;
wire [`DWIDTH-1:0] matrixC13_11;
wire [`DWIDTH-1:0] matrixC13_12;
wire [`DWIDTH-1:0] matrixC13_13;
wire [`DWIDTH-1:0] matrixC13_14;
wire [`DWIDTH-1:0] matrixC13_15;
wire [`DWIDTH-1:0] matrixC14_0;
wire [`DWIDTH-1:0] matrixC14_1;
wire [`DWIDTH-1:0] matrixC14_2;
wire [`DWIDTH-1:0] matrixC14_3;
wire [`DWIDTH-1:0] matrixC14_4;
wire [`DWIDTH-1:0] matrixC14_5;
wire [`DWIDTH-1:0] matrixC14_6;
wire [`DWIDTH-1:0] matrixC14_7;
wire [`DWIDTH-1:0] matrixC14_8;
wire [`DWIDTH-1:0] matrixC14_9;
wire [`DWIDTH-1:0] matrixC14_10;
wire [`DWIDTH-1:0] matrixC14_11;
wire [`DWIDTH-1:0] matrixC14_12;
wire [`DWIDTH-1:0] matrixC14_13;
wire [`DWIDTH-1:0] matrixC14_14;
wire [`DWIDTH-1:0] matrixC14_15;
wire [`DWIDTH-1:0] matrixC15_0;
wire [`DWIDTH-1:0] matrixC15_1;
wire [`DWIDTH-1:0] matrixC15_2;
wire [`DWIDTH-1:0] matrixC15_3;
wire [`DWIDTH-1:0] matrixC15_4;
wire [`DWIDTH-1:0] matrixC15_5;
wire [`DWIDTH-1:0] matrixC15_6;
wire [`DWIDTH-1:0] matrixC15_7;
wire [`DWIDTH-1:0] matrixC15_8;
wire [`DWIDTH-1:0] matrixC15_9;
wire [`DWIDTH-1:0] matrixC15_10;
wire [`DWIDTH-1:0] matrixC15_11;
wire [`DWIDTH-1:0] matrixC15_12;
wire [`DWIDTH-1:0] matrixC15_13;
wire [`DWIDTH-1:0] matrixC15_14;
wire [`DWIDTH-1:0] matrixC15_15;

//////////////////////////////////////////////////////////////////////////
// Instantiations of the actual PEs
//////////////////////////////////////////////////////////////////////////
systolic_pe_matrix u_systolic_pe_matrix(
    .reset(reset),
    .clk(clk),
    .pe_reset(pe_reset),
    .b_data_sel(b_data_sel),
    .b_data_valid_ping(b_data_valid_ping), 
    .b_data_valid_pong(b_data_valid_pong),
    .a0(a0),
    .a1(a1),
    .a2(a2),
    .a3(a3),
    .a4(a4),
    .a5(a5),
    .a6(a6),
    .a7(a7),
    .a8(a8),
    .a9(a9),
    .a10(a10),
    .a11(a11),
    .a12(a12),
    .a13(a13),
    .a14(a14),
    .a15(a15),
    .b0(b0),
    .b1(b1),
    .b2(b2),
    .b3(b3),
    .b4(b4),
    .b5(b5),
    .b6(b6),
    .b7(b7),
    .b8(b8),
    .b9(b9),
    .b10(b10),
    .b11(b11),
    .b12(b12),
    .b13(b13),
    .b14(b14),
    .b15(b15),
    .c0(c0),
    .c1(c1),
    .c2(c2),
    .c3(c3),
    .c4(c4),
    .c5(c5),
    .c6(c6),
    .c7(c7),
    .c8(c8),
    .c9(c9),
    .c10(c10),
    .c11(c11),
    .c12(c12),
    .c13(c13),
    .c14(c14),
    .c15(c15),
    .matrixC0_0(matrixC0_0),
    .matrixC0_1(matrixC0_1),
    .matrixC0_2(matrixC0_2),
    .matrixC0_3(matrixC0_3),
    .matrixC0_4(matrixC0_4),
    .matrixC0_5(matrixC0_5),
    .matrixC0_6(matrixC0_6),
    .matrixC0_7(matrixC0_7),
    .matrixC0_8(matrixC0_8),
    .matrixC0_9(matrixC0_9),
    .matrixC0_10(matrixC0_10),
    .matrixC0_11(matrixC0_11),
    .matrixC0_12(matrixC0_12),
    .matrixC0_13(matrixC0_13),
    .matrixC0_14(matrixC0_14),
    .matrixC0_15(matrixC0_15),
    .matrixC1_0(matrixC1_0),
    .matrixC1_1(matrixC1_1),
    .matrixC1_2(matrixC1_2),
    .matrixC1_3(matrixC1_3),
    .matrixC1_4(matrixC1_4),
    .matrixC1_5(matrixC1_5),
    .matrixC1_6(matrixC1_6),
    .matrixC1_7(matrixC1_7),
    .matrixC1_8(matrixC1_8),
    .matrixC1_9(matrixC1_9),
    .matrixC1_10(matrixC1_10),
    .matrixC1_11(matrixC1_11),
    .matrixC1_12(matrixC1_12),
    .matrixC1_13(matrixC1_13),
    .matrixC1_14(matrixC1_14),
    .matrixC1_15(matrixC1_15),
    .matrixC2_0(matrixC2_0),
    .matrixC2_1(matrixC2_1),
    .matrixC2_2(matrixC2_2),
    .matrixC2_3(matrixC2_3),
    .matrixC2_4(matrixC2_4),
    .matrixC2_5(matrixC2_5),
    .matrixC2_6(matrixC2_6),
    .matrixC2_7(matrixC2_7),
    .matrixC2_8(matrixC2_8),
    .matrixC2_9(matrixC2_9),
    .matrixC2_10(matrixC2_10),
    .matrixC2_11(matrixC2_11),
    .matrixC2_12(matrixC2_12),
    .matrixC2_13(matrixC2_13),
    .matrixC2_14(matrixC2_14),
    .matrixC2_15(matrixC2_15),
    .matrixC3_0(matrixC3_0),
    .matrixC3_1(matrixC3_1),
    .matrixC3_2(matrixC3_2),
    .matrixC3_3(matrixC3_3),
    .matrixC3_4(matrixC3_4),
    .matrixC3_5(matrixC3_5),
    .matrixC3_6(matrixC3_6),
    .matrixC3_7(matrixC3_7),
    .matrixC3_8(matrixC3_8),
    .matrixC3_9(matrixC3_9),
    .matrixC3_10(matrixC3_10),
    .matrixC3_11(matrixC3_11),
    .matrixC3_12(matrixC3_12),
    .matrixC3_13(matrixC3_13),
    .matrixC3_14(matrixC3_14),
    .matrixC3_15(matrixC3_15),
    .matrixC4_0(matrixC4_0),
    .matrixC4_1(matrixC4_1),
    .matrixC4_2(matrixC4_2),
    .matrixC4_3(matrixC4_3),
    .matrixC4_4(matrixC4_4),
    .matrixC4_5(matrixC4_5),
    .matrixC4_6(matrixC4_6),
    .matrixC4_7(matrixC4_7),
    .matrixC4_8(matrixC4_8),
    .matrixC4_9(matrixC4_9),
    .matrixC4_10(matrixC4_10),
    .matrixC4_11(matrixC4_11),
    .matrixC4_12(matrixC4_12),
    .matrixC4_13(matrixC4_13),
    .matrixC4_14(matrixC4_14),
    .matrixC4_15(matrixC4_15),
    .matrixC5_0(matrixC5_0),
    .matrixC5_1(matrixC5_1),
    .matrixC5_2(matrixC5_2),
    .matrixC5_3(matrixC5_3),
    .matrixC5_4(matrixC5_4),
    .matrixC5_5(matrixC5_5),
    .matrixC5_6(matrixC5_6),
    .matrixC5_7(matrixC5_7),
    .matrixC5_8(matrixC5_8),
    .matrixC5_9(matrixC5_9),
    .matrixC5_10(matrixC5_10),
    .matrixC5_11(matrixC5_11),
    .matrixC5_12(matrixC5_12),
    .matrixC5_13(matrixC5_13),
    .matrixC5_14(matrixC5_14),
    .matrixC5_15(matrixC5_15),
    .matrixC6_0(matrixC6_0),
    .matrixC6_1(matrixC6_1),
    .matrixC6_2(matrixC6_2),
    .matrixC6_3(matrixC6_3),
    .matrixC6_4(matrixC6_4),
    .matrixC6_5(matrixC6_5),
    .matrixC6_6(matrixC6_6),
    .matrixC6_7(matrixC6_7),
    .matrixC6_8(matrixC6_8),
    .matrixC6_9(matrixC6_9),
    .matrixC6_10(matrixC6_10),
    .matrixC6_11(matrixC6_11),
    .matrixC6_12(matrixC6_12),
    .matrixC6_13(matrixC6_13),
    .matrixC6_14(matrixC6_14),
    .matrixC6_15(matrixC6_15),
    .matrixC7_0(matrixC7_0),
    .matrixC7_1(matrixC7_1),
    .matrixC7_2(matrixC7_2),
    .matrixC7_3(matrixC7_3),
    .matrixC7_4(matrixC7_4),
    .matrixC7_5(matrixC7_5),
    .matrixC7_6(matrixC7_6),
    .matrixC7_7(matrixC7_7),
    .matrixC7_8(matrixC7_8),
    .matrixC7_9(matrixC7_9),
    .matrixC7_10(matrixC7_10),
    .matrixC7_11(matrixC7_11),
    .matrixC7_12(matrixC7_12),
    .matrixC7_13(matrixC7_13),
    .matrixC7_14(matrixC7_14),
    .matrixC7_15(matrixC7_15),
    .matrixC8_0(matrixC8_0),
    .matrixC8_1(matrixC8_1),
    .matrixC8_2(matrixC8_2),
    .matrixC8_3(matrixC8_3),
    .matrixC8_4(matrixC8_4),
    .matrixC8_5(matrixC8_5),
    .matrixC8_6(matrixC8_6),
    .matrixC8_7(matrixC8_7),
    .matrixC8_8(matrixC8_8),
    .matrixC8_9(matrixC8_9),
    .matrixC8_10(matrixC8_10),
    .matrixC8_11(matrixC8_11),
    .matrixC8_12(matrixC8_12),
    .matrixC8_13(matrixC8_13),
    .matrixC8_14(matrixC8_14),
    .matrixC8_15(matrixC8_15),
    .matrixC9_0(matrixC9_0),
    .matrixC9_1(matrixC9_1),
    .matrixC9_2(matrixC9_2),
    .matrixC9_3(matrixC9_3),
    .matrixC9_4(matrixC9_4),
    .matrixC9_5(matrixC9_5),
    .matrixC9_6(matrixC9_6),
    .matrixC9_7(matrixC9_7),
    .matrixC9_8(matrixC9_8),
    .matrixC9_9(matrixC9_9),
    .matrixC9_10(matrixC9_10),
    .matrixC9_11(matrixC9_11),
    .matrixC9_12(matrixC9_12),
    .matrixC9_13(matrixC9_13),
    .matrixC9_14(matrixC9_14),
    .matrixC9_15(matrixC9_15),
    .matrixC10_0(matrixC10_0),
    .matrixC10_1(matrixC10_1),
    .matrixC10_2(matrixC10_2),
    .matrixC10_3(matrixC10_3),
    .matrixC10_4(matrixC10_4),
    .matrixC10_5(matrixC10_5),
    .matrixC10_6(matrixC10_6),
    .matrixC10_7(matrixC10_7),
    .matrixC10_8(matrixC10_8),
    .matrixC10_9(matrixC10_9),
    .matrixC10_10(matrixC10_10),
    .matrixC10_11(matrixC10_11),
    .matrixC10_12(matrixC10_12),
    .matrixC10_13(matrixC10_13),
    .matrixC10_14(matrixC10_14),
    .matrixC10_15(matrixC10_15),
    .matrixC11_0(matrixC11_0),
    .matrixC11_1(matrixC11_1),
    .matrixC11_2(matrixC11_2),
    .matrixC11_3(matrixC11_3),
    .matrixC11_4(matrixC11_4),
    .matrixC11_5(matrixC11_5),
    .matrixC11_6(matrixC11_6),
    .matrixC11_7(matrixC11_7),
    .matrixC11_8(matrixC11_8),
    .matrixC11_9(matrixC11_9),
    .matrixC11_10(matrixC11_10),
    .matrixC11_11(matrixC11_11),
    .matrixC11_12(matrixC11_12),
    .matrixC11_13(matrixC11_13),
    .matrixC11_14(matrixC11_14),
    .matrixC11_15(matrixC11_15),
    .matrixC12_0(matrixC12_0),
    .matrixC12_1(matrixC12_1),
    .matrixC12_2(matrixC12_2),
    .matrixC12_3(matrixC12_3),
    .matrixC12_4(matrixC12_4),
    .matrixC12_5(matrixC12_5),
    .matrixC12_6(matrixC12_6),
    .matrixC12_7(matrixC12_7),
    .matrixC12_8(matrixC12_8),
    .matrixC12_9(matrixC12_9),
    .matrixC12_10(matrixC12_10),
    .matrixC12_11(matrixC12_11),
    .matrixC12_12(matrixC12_12),
    .matrixC12_13(matrixC12_13),
    .matrixC12_14(matrixC12_14),
    .matrixC12_15(matrixC12_15),
    .matrixC13_0(matrixC13_0),
    .matrixC13_1(matrixC13_1),
    .matrixC13_2(matrixC13_2),
    .matrixC13_3(matrixC13_3),
    .matrixC13_4(matrixC13_4),
    .matrixC13_5(matrixC13_5),
    .matrixC13_6(matrixC13_6),
    .matrixC13_7(matrixC13_7),
    .matrixC13_8(matrixC13_8),
    .matrixC13_9(matrixC13_9),
    .matrixC13_10(matrixC13_10),
    .matrixC13_11(matrixC13_11),
    .matrixC13_12(matrixC13_12),
    .matrixC13_13(matrixC13_13),
    .matrixC13_14(matrixC13_14),
    .matrixC13_15(matrixC13_15),
    .matrixC14_0(matrixC14_0),
    .matrixC14_1(matrixC14_1),
    .matrixC14_2(matrixC14_2),
    .matrixC14_3(matrixC14_3),
    .matrixC14_4(matrixC14_4),
    .matrixC14_5(matrixC14_5),
    .matrixC14_6(matrixC14_6),
    .matrixC14_7(matrixC14_7),
    .matrixC14_8(matrixC14_8),
    .matrixC14_9(matrixC14_9),
    .matrixC14_10(matrixC14_10),
    .matrixC14_11(matrixC14_11),
    .matrixC14_12(matrixC14_12),
    .matrixC14_13(matrixC14_13),
    .matrixC14_14(matrixC14_14),
    .matrixC14_15(matrixC14_15),
    .matrixC15_0(matrixC15_0),
    .matrixC15_1(matrixC15_1),
    .matrixC15_2(matrixC15_2),
    .matrixC15_3(matrixC15_3),
    .matrixC15_4(matrixC15_4),
    .matrixC15_5(matrixC15_5),
    .matrixC15_6(matrixC15_6),
    .matrixC15_7(matrixC15_7),
    .matrixC15_8(matrixC15_8),
    .matrixC15_9(matrixC15_9),
    .matrixC15_10(matrixC15_10),
    .matrixC15_11(matrixC15_11),
    .matrixC15_12(matrixC15_12),
    .matrixC15_13(matrixC15_13),
    .matrixC15_14(matrixC15_14),
    .matrixC15_15(matrixC15_15),
    .a_data_out(a_data_out),
    .b_data_out(b_data_out)
);
  
wire c_data_available;
  
assign c_data_available = (clk_cnt > (`LOG2_MAT_MUL_SIZE-1+(`MAT_MUL_SIZE << 1)) & clk_cnt <= ((`LOG2_MAT_MUL_SIZE+(`MAT_MUL_SIZE << 1)) + (num_matrices_A << `LOG2_MAT_MUL_SIZE)-1));

endmodule

//////////////////////////////////////////////////////////////////////////
// Systolic data setup
//////////////////////////////////////////////////////////////////////////
module systolic_data_setup(
    clk,
    reset,
    start_mat_mul,
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
    a4_data_delayed_4,
    a5_data_delayed_5,
    a6_data_delayed_6,
    a7_data_delayed_7,
    a8_data_delayed_8,
    a9_data_delayed_9,
    a10_data_delayed_10,
    a11_data_delayed_11,
    a12_data_delayed_12,
    a13_data_delayed_13,
    a14_data_delayed_14,
    a15_data_delayed_15,
    b0_data,
    b1_data_delayed_1,
    b2_data_delayed_2,
    b3_data_delayed_3,
    b4_data_delayed_4,
    b5_data_delayed_5,
    b6_data_delayed_6,
    b7_data_delayed_7,
    b8_data_delayed_8,
    b9_data_delayed_9,
    b10_data_delayed_10,
    b11_data_delayed_11,
    b12_data_delayed_12,
    b13_data_delayed_13,
    b14_data_delayed_14,
    b15_data_delayed_15,
    validity_mask_a_rows,
    validity_mask_a_cols_b_rows,
    validity_mask_b_cols,
    num_matrices_A,
    num_matrices_B,
    a_loc,
    b_loc 
);

input clk;
input reset;
input start_mat_mul;
output [`AWIDTH-1:0] a_addr;
output [`AWIDTH-1:0] b_addr;
input [`AWIDTH-1:0] address_mat_a;
input [`AWIDTH-1:0] address_mat_b;
input [`ADDR_STRIDE_WIDTH-1:0] address_stride_a;
input [`ADDR_STRIDE_WIDTH-1:0] address_stride_b;
input [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data;
input [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data;
input [31:0] clk_cnt;
output [`DWIDTH-1:0] a0_data;
output [`DWIDTH-1:0] a1_data_delayed_1;
output [`DWIDTH-1:0] a2_data_delayed_2;
output [`DWIDTH-1:0] a3_data_delayed_3;
output [`DWIDTH-1:0] a4_data_delayed_4;
output [`DWIDTH-1:0] a5_data_delayed_5;
output [`DWIDTH-1:0] a6_data_delayed_6;
output [`DWIDTH-1:0] a7_data_delayed_7;
output [`DWIDTH-1:0] a8_data_delayed_8;
output [`DWIDTH-1:0] a9_data_delayed_9;
output [`DWIDTH-1:0] a10_data_delayed_10;
output [`DWIDTH-1:0] a11_data_delayed_11;
output [`DWIDTH-1:0] a12_data_delayed_12;
output [`DWIDTH-1:0] a13_data_delayed_13;
output [`DWIDTH-1:0] a14_data_delayed_14;
output [`DWIDTH-1:0] a15_data_delayed_15;
output [`DWIDTH-1:0] b0_data;
output [`DWIDTH-1:0] b1_data_delayed_1;
output [`DWIDTH-1:0] b2_data_delayed_2;
output [`DWIDTH-1:0] b3_data_delayed_3;
output [`DWIDTH-1:0] b4_data_delayed_4;
output [`DWIDTH-1:0] b5_data_delayed_5;
output [`DWIDTH-1:0] b6_data_delayed_6;
output [`DWIDTH-1:0] b7_data_delayed_7;
output [`DWIDTH-1:0] b8_data_delayed_8;
output [`DWIDTH-1:0] b9_data_delayed_9;
output [`DWIDTH-1:0] b10_data_delayed_10;
output [`DWIDTH-1:0] b11_data_delayed_11;
output [`DWIDTH-1:0] b12_data_delayed_12;
output [`DWIDTH-1:0] b13_data_delayed_13;
output [`DWIDTH-1:0] b14_data_delayed_14;
output [`DWIDTH-1:0] b15_data_delayed_15;
input [`MASK_WIDTH-1:0] validity_mask_a_rows;
input [`MASK_WIDTH-1:0] validity_mask_a_cols_b_rows;
input [`MASK_WIDTH-1:0] validity_mask_b_cols;
input [31:0] num_matrices_A;
input [31:0] num_matrices_B;
input [31:0] a_loc;
input [31:0] b_loc;

wire [`DWIDTH-1:0] a0_data;
wire [`DWIDTH-1:0] a1_data;
wire [`DWIDTH-1:0] a2_data;
wire [`DWIDTH-1:0] a3_data;
wire [`DWIDTH-1:0] a4_data;
wire [`DWIDTH-1:0] a5_data;
wire [`DWIDTH-1:0] a6_data;
wire [`DWIDTH-1:0] a7_data;
wire [`DWIDTH-1:0] a8_data;
wire [`DWIDTH-1:0] a9_data;
wire [`DWIDTH-1:0] a10_data;
wire [`DWIDTH-1:0] a11_data;
wire [`DWIDTH-1:0] a12_data;
wire [`DWIDTH-1:0] a13_data;
wire [`DWIDTH-1:0] a14_data;
wire [`DWIDTH-1:0] a15_data;
wire [`DWIDTH-1:0] b0_data;
wire [`DWIDTH-1:0] b1_data;
wire [`DWIDTH-1:0] b2_data;
wire [`DWIDTH-1:0] b3_data;
wire [`DWIDTH-1:0] b4_data;
wire [`DWIDTH-1:0] b5_data;
wire [`DWIDTH-1:0] b6_data;
wire [`DWIDTH-1:0] b7_data;
wire [`DWIDTH-1:0] b8_data;
wire [`DWIDTH-1:0] b9_data;
wire [`DWIDTH-1:0] b10_data;
wire [`DWIDTH-1:0] b11_data;
wire [`DWIDTH-1:0] b12_data;
wire [`DWIDTH-1:0] b13_data;
wire [`DWIDTH-1:0] b14_data;
wire [`DWIDTH-1:0] b15_data;

wire a_data_valid; // flag that tells whether the data from memory is valid
wire b_data_valid; // flag that tells whether the data from memory is valid

//////////////////////////////////////////////////////////////////////////
// Logic to generate addresses to BRAM A
//////////////////////////////////////////////////////////////////////////

reg [`AWIDTH-1:0] a_addr;
reg a_mem_access; // flag that tells whether the matmul is trying to access memory or not
  
always @(posedge clk) begin     
if ((reset || ~start_mat_mul) || (clk_cnt >= (a_loc<<`LOG2_MAT_MUL_SIZE)+`MAT_MUL_SIZE+(num_matrices_A << `LOG2_MAT_MUL_SIZE))) begin
        a_addr <= address_mat_a-address_stride_a;
        a_mem_access <= 0; 
end
else if ((clk_cnt >= (a_loc<<`LOG2_MAT_MUL_SIZE)+`MAT_MUL_SIZE) && (clk_cnt < (a_loc<<`LOG2_MAT_MUL_SIZE)+`MAT_MUL_SIZE+(num_matrices_A << `LOG2_MAT_MUL_SIZE))) begin
        a_addr <= a_addr + address_stride_a;
        a_mem_access <= 1;
end
end


//////////////////////////////////////////////////////////////////////////
// Logic to generate valid signals for data coming from BRAM A
//////////////////////////////////////////////////////////////////////////

reg [31:0] a_mem_access_counter;

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
  
assign a_data_valid = 
       ((validity_mask_a_cols_b_rows[0]==1'b0 && a_mem_access_counter==1) ||
        (validity_mask_a_cols_b_rows[1]==1'b0 && a_mem_access_counter==2) ||
        (validity_mask_a_cols_b_rows[2]==1'b0 && a_mem_access_counter==3) ||
        (validity_mask_a_cols_b_rows[3]==1'b0 && a_mem_access_counter==4) ||
        (validity_mask_a_cols_b_rows[4]==1'b0 && a_mem_access_counter==5) ||
        (validity_mask_a_cols_b_rows[5]==1'b0 && a_mem_access_counter==6) ||
        (validity_mask_a_cols_b_rows[6]==1'b0 && a_mem_access_counter==7) ||
        (validity_mask_a_cols_b_rows[7]==1'b0 && a_mem_access_counter==8) ||
        (validity_mask_a_cols_b_rows[8]==1'b0 && a_mem_access_counter==9) ||
        (validity_mask_a_cols_b_rows[9]==1'b0 && a_mem_access_counter==10) ||
        (validity_mask_a_cols_b_rows[10]==1'b0 && a_mem_access_counter==11) ||
        (validity_mask_a_cols_b_rows[11]==1'b0 && a_mem_access_counter==12) ||
        (validity_mask_a_cols_b_rows[12]==1'b0 && a_mem_access_counter==13) ||
        (validity_mask_a_cols_b_rows[13]==1'b0 && a_mem_access_counter==14) ||
        (validity_mask_a_cols_b_rows[14]==1'b0 && a_mem_access_counter==15) ||
        (validity_mask_a_cols_b_rows[15]==1'b0 && a_mem_access_counter==16)) ?
        1'b0 : (a_mem_access_counter >= `MEM_ACCESS_LATENCY);

//////////////////////////////////////////////////////////////////////////
// Logic to delay certain parts of the data received from BRAM A (systolic data setup)
//////////////////////////////////////////////////////////////////////////

// Slice data into chunks and qualify it with whether it is valid or not
assign a0_data = a_data[`DWIDTH-1:0] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[0]}};
assign a1_data = a_data[2*`DWIDTH-1:1*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[1]}};
assign a2_data = a_data[3*`DWIDTH-1:2*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[2]}};
assign a3_data = a_data[4*`DWIDTH-1:3*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[3]}};
assign a4_data = a_data[5*`DWIDTH-1:4*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[4]}};
assign a5_data = a_data[6*`DWIDTH-1:5*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[5]}};
assign a6_data = a_data[7*`DWIDTH-1:6*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[6]}};
assign a7_data = a_data[8*`DWIDTH-1:7*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[7]}};
assign a8_data = a_data[9*`DWIDTH-1:8*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[8]}};
assign a9_data = a_data[10*`DWIDTH-1:9*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[9]}};
assign a10_data = a_data[11*`DWIDTH-1:10*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[10]}};
assign a11_data = a_data[12*`DWIDTH-1:11*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[11]}};
assign a12_data = a_data[13*`DWIDTH-1:12*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[12]}};
assign a13_data = a_data[14*`DWIDTH-1:13*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[13]}};
assign a14_data = a_data[15*`DWIDTH-1:14*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[14]}};
assign a15_data = a_data[16*`DWIDTH-1:15*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[15]}};

// For larger matmuls, more such delaying flops will be needed
reg [`DWIDTH-1:0] a1_data_delayed_1;
reg [`DWIDTH-1:0] a2_data_delayed_1;
reg [`DWIDTH-1:0] a2_data_delayed_2;
reg [`DWIDTH-1:0] a3_data_delayed_1;
reg [`DWIDTH-1:0] a3_data_delayed_2;
reg [`DWIDTH-1:0] a3_data_delayed_3;
reg [`DWIDTH-1:0] a4_data_delayed_1;
reg [`DWIDTH-1:0] a4_data_delayed_2;
reg [`DWIDTH-1:0] a4_data_delayed_3;
reg [`DWIDTH-1:0] a4_data_delayed_4;
reg [`DWIDTH-1:0] a5_data_delayed_1;
reg [`DWIDTH-1:0] a5_data_delayed_2;
reg [`DWIDTH-1:0] a5_data_delayed_3;
reg [`DWIDTH-1:0] a5_data_delayed_4;
reg [`DWIDTH-1:0] a5_data_delayed_5;
reg [`DWIDTH-1:0] a6_data_delayed_1;
reg [`DWIDTH-1:0] a6_data_delayed_2;
reg [`DWIDTH-1:0] a6_data_delayed_3;
reg [`DWIDTH-1:0] a6_data_delayed_4;
reg [`DWIDTH-1:0] a6_data_delayed_5;
reg [`DWIDTH-1:0] a6_data_delayed_6;
reg [`DWIDTH-1:0] a7_data_delayed_1;
reg [`DWIDTH-1:0] a7_data_delayed_2;
reg [`DWIDTH-1:0] a7_data_delayed_3;
reg [`DWIDTH-1:0] a7_data_delayed_4;
reg [`DWIDTH-1:0] a7_data_delayed_5;
reg [`DWIDTH-1:0] a7_data_delayed_6;
reg [`DWIDTH-1:0] a7_data_delayed_7;
reg [`DWIDTH-1:0] a8_data_delayed_1;
reg [`DWIDTH-1:0] a8_data_delayed_2;
reg [`DWIDTH-1:0] a8_data_delayed_3;
reg [`DWIDTH-1:0] a8_data_delayed_4;
reg [`DWIDTH-1:0] a8_data_delayed_5;
reg [`DWIDTH-1:0] a8_data_delayed_6;
reg [`DWIDTH-1:0] a8_data_delayed_7;
reg [`DWIDTH-1:0] a8_data_delayed_8;
reg [`DWIDTH-1:0] a9_data_delayed_1;
reg [`DWIDTH-1:0] a9_data_delayed_2;
reg [`DWIDTH-1:0] a9_data_delayed_3;
reg [`DWIDTH-1:0] a9_data_delayed_4;
reg [`DWIDTH-1:0] a9_data_delayed_5;
reg [`DWIDTH-1:0] a9_data_delayed_6;
reg [`DWIDTH-1:0] a9_data_delayed_7;
reg [`DWIDTH-1:0] a9_data_delayed_8;
reg [`DWIDTH-1:0] a9_data_delayed_9;
reg [`DWIDTH-1:0] a10_data_delayed_1;
reg [`DWIDTH-1:0] a10_data_delayed_2;
reg [`DWIDTH-1:0] a10_data_delayed_3;
reg [`DWIDTH-1:0] a10_data_delayed_4;
reg [`DWIDTH-1:0] a10_data_delayed_5;
reg [`DWIDTH-1:0] a10_data_delayed_6;
reg [`DWIDTH-1:0] a10_data_delayed_7;
reg [`DWIDTH-1:0] a10_data_delayed_8;
reg [`DWIDTH-1:0] a10_data_delayed_9;
reg [`DWIDTH-1:0] a10_data_delayed_10;
reg [`DWIDTH-1:0] a11_data_delayed_1;
reg [`DWIDTH-1:0] a11_data_delayed_2;
reg [`DWIDTH-1:0] a11_data_delayed_3;
reg [`DWIDTH-1:0] a11_data_delayed_4;
reg [`DWIDTH-1:0] a11_data_delayed_5;
reg [`DWIDTH-1:0] a11_data_delayed_6;
reg [`DWIDTH-1:0] a11_data_delayed_7;
reg [`DWIDTH-1:0] a11_data_delayed_8;
reg [`DWIDTH-1:0] a11_data_delayed_9;
reg [`DWIDTH-1:0] a11_data_delayed_10;
reg [`DWIDTH-1:0] a11_data_delayed_11;
reg [`DWIDTH-1:0] a12_data_delayed_1;
reg [`DWIDTH-1:0] a12_data_delayed_2;
reg [`DWIDTH-1:0] a12_data_delayed_3;
reg [`DWIDTH-1:0] a12_data_delayed_4;
reg [`DWIDTH-1:0] a12_data_delayed_5;
reg [`DWIDTH-1:0] a12_data_delayed_6;
reg [`DWIDTH-1:0] a12_data_delayed_7;
reg [`DWIDTH-1:0] a12_data_delayed_8;
reg [`DWIDTH-1:0] a12_data_delayed_9;
reg [`DWIDTH-1:0] a12_data_delayed_10;
reg [`DWIDTH-1:0] a12_data_delayed_11;
reg [`DWIDTH-1:0] a12_data_delayed_12;
reg [`DWIDTH-1:0] a13_data_delayed_1;
reg [`DWIDTH-1:0] a13_data_delayed_2;
reg [`DWIDTH-1:0] a13_data_delayed_3;
reg [`DWIDTH-1:0] a13_data_delayed_4;
reg [`DWIDTH-1:0] a13_data_delayed_5;
reg [`DWIDTH-1:0] a13_data_delayed_6;
reg [`DWIDTH-1:0] a13_data_delayed_7;
reg [`DWIDTH-1:0] a13_data_delayed_8;
reg [`DWIDTH-1:0] a13_data_delayed_9;
reg [`DWIDTH-1:0] a13_data_delayed_10;
reg [`DWIDTH-1:0] a13_data_delayed_11;
reg [`DWIDTH-1:0] a13_data_delayed_12;
reg [`DWIDTH-1:0] a13_data_delayed_13;
reg [`DWIDTH-1:0] a14_data_delayed_1;
reg [`DWIDTH-1:0] a14_data_delayed_2;
reg [`DWIDTH-1:0] a14_data_delayed_3;
reg [`DWIDTH-1:0] a14_data_delayed_4;
reg [`DWIDTH-1:0] a14_data_delayed_5;
reg [`DWIDTH-1:0] a14_data_delayed_6;
reg [`DWIDTH-1:0] a14_data_delayed_7;
reg [`DWIDTH-1:0] a14_data_delayed_8;
reg [`DWIDTH-1:0] a14_data_delayed_9;
reg [`DWIDTH-1:0] a14_data_delayed_10;
reg [`DWIDTH-1:0] a14_data_delayed_11;
reg [`DWIDTH-1:0] a14_data_delayed_12;
reg [`DWIDTH-1:0] a14_data_delayed_13;
reg [`DWIDTH-1:0] a14_data_delayed_14;
reg [`DWIDTH-1:0] a15_data_delayed_1;
reg [`DWIDTH-1:0] a15_data_delayed_2;
reg [`DWIDTH-1:0] a15_data_delayed_3;
reg [`DWIDTH-1:0] a15_data_delayed_4;
reg [`DWIDTH-1:0] a15_data_delayed_5;
reg [`DWIDTH-1:0] a15_data_delayed_6;
reg [`DWIDTH-1:0] a15_data_delayed_7;
reg [`DWIDTH-1:0] a15_data_delayed_8;
reg [`DWIDTH-1:0] a15_data_delayed_9;
reg [`DWIDTH-1:0] a15_data_delayed_10;
reg [`DWIDTH-1:0] a15_data_delayed_11;
reg [`DWIDTH-1:0] a15_data_delayed_12;
reg [`DWIDTH-1:0] a15_data_delayed_13;
reg [`DWIDTH-1:0] a15_data_delayed_14;
reg [`DWIDTH-1:0] a15_data_delayed_15;

always @(posedge clk) begin
  if (reset || ~start_mat_mul || clk_cnt==0) begin
    a1_data_delayed_1 <= 0;
    a2_data_delayed_1 <= 0;
    a2_data_delayed_2 <= 0;
    a3_data_delayed_1 <= 0;
    a3_data_delayed_2 <= 0;
    a3_data_delayed_3 <= 0;
    a4_data_delayed_1 <= 0;
    a4_data_delayed_2 <= 0;
    a4_data_delayed_3 <= 0;
    a4_data_delayed_4 <= 0;
    a5_data_delayed_1 <= 0;
    a5_data_delayed_2 <= 0;
    a5_data_delayed_3 <= 0;
    a5_data_delayed_4 <= 0;
    a5_data_delayed_5 <= 0;
    a6_data_delayed_1 <= 0;
    a6_data_delayed_2 <= 0;
    a6_data_delayed_3 <= 0;
    a6_data_delayed_4 <= 0;
    a6_data_delayed_5 <= 0;
    a6_data_delayed_6 <= 0;
    a7_data_delayed_1 <= 0;
    a7_data_delayed_2 <= 0;
    a7_data_delayed_3 <= 0;
    a7_data_delayed_4 <= 0;
    a7_data_delayed_5 <= 0;
    a7_data_delayed_6 <= 0;
    a7_data_delayed_7 <= 0;
    a8_data_delayed_1 <= 0;
    a8_data_delayed_2 <= 0;
    a8_data_delayed_3 <= 0;
    a8_data_delayed_4 <= 0;
    a8_data_delayed_5 <= 0;
    a8_data_delayed_6 <= 0;
    a8_data_delayed_7 <= 0;
    a8_data_delayed_8 <= 0;
    a9_data_delayed_1 <= 0;
    a9_data_delayed_2 <= 0;
    a9_data_delayed_3 <= 0;
    a9_data_delayed_4 <= 0;
    a9_data_delayed_5 <= 0;
    a9_data_delayed_6 <= 0;
    a9_data_delayed_7 <= 0;
    a9_data_delayed_8 <= 0;
    a9_data_delayed_9 <= 0;
    a10_data_delayed_1 <= 0;
    a10_data_delayed_2 <= 0;
    a10_data_delayed_3 <= 0;
    a10_data_delayed_4 <= 0;
    a10_data_delayed_5 <= 0;
    a10_data_delayed_6 <= 0;
    a10_data_delayed_7 <= 0;
    a10_data_delayed_8 <= 0;
    a10_data_delayed_9 <= 0;
    a10_data_delayed_10 <= 0;
    a11_data_delayed_1 <= 0;
    a11_data_delayed_2 <= 0;
    a11_data_delayed_3 <= 0;
    a11_data_delayed_4 <= 0;
    a11_data_delayed_5 <= 0;
    a11_data_delayed_6 <= 0;
    a11_data_delayed_7 <= 0;
    a11_data_delayed_8 <= 0;
    a11_data_delayed_9 <= 0;
    a11_data_delayed_10 <= 0;
    a11_data_delayed_11 <= 0;
    a12_data_delayed_1 <= 0;
    a12_data_delayed_2 <= 0;
    a12_data_delayed_3 <= 0;
    a12_data_delayed_4 <= 0;
    a12_data_delayed_5 <= 0;
    a12_data_delayed_6 <= 0;
    a12_data_delayed_7 <= 0;
    a12_data_delayed_8 <= 0;
    a12_data_delayed_9 <= 0;
    a12_data_delayed_10 <= 0;
    a12_data_delayed_11 <= 0;
    a12_data_delayed_12 <= 0;
    a13_data_delayed_1 <= 0;
    a13_data_delayed_2 <= 0;
    a13_data_delayed_3 <= 0;
    a13_data_delayed_4 <= 0;
    a13_data_delayed_5 <= 0;
    a13_data_delayed_6 <= 0;
    a13_data_delayed_7 <= 0;
    a13_data_delayed_8 <= 0;
    a13_data_delayed_9 <= 0;
    a13_data_delayed_10 <= 0;
    a13_data_delayed_11 <= 0;
    a13_data_delayed_12 <= 0;
    a13_data_delayed_13 <= 0;
    a14_data_delayed_1 <= 0;
    a14_data_delayed_2 <= 0;
    a14_data_delayed_3 <= 0;
    a14_data_delayed_4 <= 0;
    a14_data_delayed_5 <= 0;
    a14_data_delayed_6 <= 0;
    a14_data_delayed_7 <= 0;
    a14_data_delayed_8 <= 0;
    a14_data_delayed_9 <= 0;
    a14_data_delayed_10 <= 0;
    a14_data_delayed_11 <= 0;
    a14_data_delayed_12 <= 0;
    a14_data_delayed_13 <= 0;
    a14_data_delayed_14 <= 0;
    a15_data_delayed_1 <= 0;
    a15_data_delayed_2 <= 0;
    a15_data_delayed_3 <= 0;
    a15_data_delayed_4 <= 0;
    a15_data_delayed_5 <= 0;
    a15_data_delayed_6 <= 0;
    a15_data_delayed_7 <= 0;
    a15_data_delayed_8 <= 0;
    a15_data_delayed_9 <= 0;
    a15_data_delayed_10 <= 0;
    a15_data_delayed_11 <= 0;
    a15_data_delayed_12 <= 0;
    a15_data_delayed_13 <= 0;
    a15_data_delayed_14 <= 0;
    a15_data_delayed_15 <= 0;
  end
  else begin
    a1_data_delayed_1 <= a1_data;
    a2_data_delayed_1 <= a2_data;
    a2_data_delayed_2 <= a2_data_delayed_1;
    a3_data_delayed_1 <= a3_data;
    a3_data_delayed_2 <= a3_data_delayed_1;
    a3_data_delayed_3 <= a3_data_delayed_2;
    a4_data_delayed_1 <= a4_data;
    a4_data_delayed_2 <= a4_data_delayed_1;
    a4_data_delayed_3 <= a4_data_delayed_2;
    a4_data_delayed_4 <= a4_data_delayed_3;
    a5_data_delayed_1 <= a5_data;
    a5_data_delayed_2 <= a5_data_delayed_1;
    a5_data_delayed_3 <= a5_data_delayed_2;
    a5_data_delayed_4 <= a5_data_delayed_3;
    a5_data_delayed_5 <= a5_data_delayed_4;
    a6_data_delayed_1 <= a6_data;
    a6_data_delayed_2 <= a6_data_delayed_1;
    a6_data_delayed_3 <= a6_data_delayed_2;
    a6_data_delayed_4 <= a6_data_delayed_3;
    a6_data_delayed_5 <= a6_data_delayed_4;
    a6_data_delayed_6 <= a6_data_delayed_5;
    a7_data_delayed_1 <= a7_data;
    a7_data_delayed_2 <= a7_data_delayed_1;
    a7_data_delayed_3 <= a7_data_delayed_2;
    a7_data_delayed_4 <= a7_data_delayed_3;
    a7_data_delayed_5 <= a7_data_delayed_4;
    a7_data_delayed_6 <= a7_data_delayed_5;
    a7_data_delayed_7 <= a7_data_delayed_6;
    a8_data_delayed_1 <= a8_data;
    a8_data_delayed_2 <= a8_data_delayed_1;
    a8_data_delayed_3 <= a8_data_delayed_2;
    a8_data_delayed_4 <= a8_data_delayed_3;
    a8_data_delayed_5 <= a8_data_delayed_4;
    a8_data_delayed_6 <= a8_data_delayed_5;
    a8_data_delayed_7 <= a8_data_delayed_6;
    a8_data_delayed_8 <= a8_data_delayed_7;
    a9_data_delayed_1 <= a9_data;
    a9_data_delayed_2 <= a9_data_delayed_1;
    a9_data_delayed_3 <= a9_data_delayed_2;
    a9_data_delayed_4 <= a9_data_delayed_3;
    a9_data_delayed_5 <= a9_data_delayed_4;
    a9_data_delayed_6 <= a9_data_delayed_5;
    a9_data_delayed_7 <= a9_data_delayed_6;
    a9_data_delayed_8 <= a9_data_delayed_7;
    a9_data_delayed_9 <= a9_data_delayed_8;
    a10_data_delayed_1 <= a10_data;
    a10_data_delayed_2 <= a10_data_delayed_1;
    a10_data_delayed_3 <= a10_data_delayed_2;
    a10_data_delayed_4 <= a10_data_delayed_3;
    a10_data_delayed_5 <= a10_data_delayed_4;
    a10_data_delayed_6 <= a10_data_delayed_5;
    a10_data_delayed_7 <= a10_data_delayed_6;
    a10_data_delayed_8 <= a10_data_delayed_7;
    a10_data_delayed_9 <= a10_data_delayed_8;
    a10_data_delayed_10 <= a10_data_delayed_9;
    a11_data_delayed_1 <= a11_data;
    a11_data_delayed_2 <= a11_data_delayed_1;
    a11_data_delayed_3 <= a11_data_delayed_2;
    a11_data_delayed_4 <= a11_data_delayed_3;
    a11_data_delayed_5 <= a11_data_delayed_4;
    a11_data_delayed_6 <= a11_data_delayed_5;
    a11_data_delayed_7 <= a11_data_delayed_6;
    a11_data_delayed_8 <= a11_data_delayed_7;
    a11_data_delayed_9 <= a11_data_delayed_8;
    a11_data_delayed_10 <= a11_data_delayed_9;
    a11_data_delayed_11 <= a11_data_delayed_10;
    a12_data_delayed_1 <= a12_data;
    a12_data_delayed_2 <= a12_data_delayed_1;
    a12_data_delayed_3 <= a12_data_delayed_2;
    a12_data_delayed_4 <= a12_data_delayed_3;
    a12_data_delayed_5 <= a12_data_delayed_4;
    a12_data_delayed_6 <= a12_data_delayed_5;
    a12_data_delayed_7 <= a12_data_delayed_6;
    a12_data_delayed_8 <= a12_data_delayed_7;
    a12_data_delayed_9 <= a12_data_delayed_8;
    a12_data_delayed_10 <= a12_data_delayed_9;
    a12_data_delayed_11 <= a12_data_delayed_10;
    a12_data_delayed_12 <= a12_data_delayed_11;
    a13_data_delayed_1 <= a13_data;
    a13_data_delayed_2 <= a13_data_delayed_1;
    a13_data_delayed_3 <= a13_data_delayed_2;
    a13_data_delayed_4 <= a13_data_delayed_3;
    a13_data_delayed_5 <= a13_data_delayed_4;
    a13_data_delayed_6 <= a13_data_delayed_5;
    a13_data_delayed_7 <= a13_data_delayed_6;
    a13_data_delayed_8 <= a13_data_delayed_7;
    a13_data_delayed_9 <= a13_data_delayed_8;
    a13_data_delayed_10 <= a13_data_delayed_9;
    a13_data_delayed_11 <= a13_data_delayed_10;
    a13_data_delayed_12 <= a13_data_delayed_11;
    a13_data_delayed_13 <= a13_data_delayed_12;
    a14_data_delayed_1 <= a14_data;
    a14_data_delayed_2 <= a14_data_delayed_1;
    a14_data_delayed_3 <= a14_data_delayed_2;
    a14_data_delayed_4 <= a14_data_delayed_3;
    a14_data_delayed_5 <= a14_data_delayed_4;
    a14_data_delayed_6 <= a14_data_delayed_5;
    a14_data_delayed_7 <= a14_data_delayed_6;
    a14_data_delayed_8 <= a14_data_delayed_7;
    a14_data_delayed_9 <= a14_data_delayed_8;
    a14_data_delayed_10 <= a14_data_delayed_9;
    a14_data_delayed_11 <= a14_data_delayed_10;
    a14_data_delayed_12 <= a14_data_delayed_11;
    a14_data_delayed_13 <= a14_data_delayed_12;
    a14_data_delayed_14 <= a14_data_delayed_13;
    a15_data_delayed_1 <= a15_data;
    a15_data_delayed_2 <= a15_data_delayed_1;
    a15_data_delayed_3 <= a15_data_delayed_2;
    a15_data_delayed_4 <= a15_data_delayed_3;
    a15_data_delayed_5 <= a15_data_delayed_4;
    a15_data_delayed_6 <= a15_data_delayed_5;
    a15_data_delayed_7 <= a15_data_delayed_6;
    a15_data_delayed_8 <= a15_data_delayed_7;
    a15_data_delayed_9 <= a15_data_delayed_8;
    a15_data_delayed_10 <= a15_data_delayed_9;
    a15_data_delayed_11 <= a15_data_delayed_10;
    a15_data_delayed_12 <= a15_data_delayed_11;
    a15_data_delayed_13 <= a15_data_delayed_12;
    a15_data_delayed_14 <= a15_data_delayed_13;
    a15_data_delayed_15 <= a15_data_delayed_14;
  end
end

//////////////////////////////////////////////////////////////////////////
// Logic to generate addresses to BRAM B
//////////////////////////////////////////////////////////////////////////

reg [`AWIDTH-1:0] b_addr;
reg b_mem_access; // flag that tells whether the matmul is trying to access memory or not
 
always @(posedge clk) begin  
    if ((reset || ~start_mat_mul) || (clk_cnt >= (b_loc<<`LOG2_MAT_MUL_SIZE)+num_matrices_B << `LOG2_MAT_MUL_SIZE)) begin
        b_addr <= address_mat_b - address_stride_b;
        b_mem_access <= 0;
    end 
    else if ((clk_cnt >= (b_loc<<`LOG2_MAT_MUL_SIZE)) && (clk_cnt < (b_loc<<`LOG2_MAT_MUL_SIZE)+num_matrices_B << `LOG2_MAT_MUL_SIZE)) begin
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

assign b_data_valid = 
       ((validity_mask_a_cols_b_rows[0]==1'b0 && b_mem_access_counter==1) ||
        (validity_mask_a_cols_b_rows[1]==1'b0 && b_mem_access_counter==2) ||
        (validity_mask_a_cols_b_rows[2]==1'b0 && b_mem_access_counter==3) ||
        (validity_mask_a_cols_b_rows[3]==1'b0 && b_mem_access_counter==4) ||
        (validity_mask_a_cols_b_rows[4]==1'b0 && b_mem_access_counter==5) ||
        (validity_mask_a_cols_b_rows[5]==1'b0 && b_mem_access_counter==6) ||
        (validity_mask_a_cols_b_rows[6]==1'b0 && b_mem_access_counter==7) ||
        (validity_mask_a_cols_b_rows[7]==1'b0 && b_mem_access_counter==8) ||
        (validity_mask_a_cols_b_rows[8]==1'b0 && b_mem_access_counter==9) ||
        (validity_mask_a_cols_b_rows[9]==1'b0 && b_mem_access_counter==10) ||
        (validity_mask_a_cols_b_rows[10]==1'b0 && b_mem_access_counter==11) ||
        (validity_mask_a_cols_b_rows[11]==1'b0 && b_mem_access_counter==12) ||
        (validity_mask_a_cols_b_rows[12]==1'b0 && b_mem_access_counter==13) ||
        (validity_mask_a_cols_b_rows[13]==1'b0 && b_mem_access_counter==14) ||
        (validity_mask_a_cols_b_rows[14]==1'b0 && b_mem_access_counter==15) ||
        (validity_mask_a_cols_b_rows[15]==1'b0 && b_mem_access_counter==16)) ?
        1'b0 : (b_mem_access_counter >= `MEM_ACCESS_LATENCY);   

//////////////////////////////////////////////////////////////////////////
// Logic to delay certain parts of the data received from BRAM B (systolic data setup)
//////////////////////////////////////////////////////////////////////////

// Slice data into chunks and qualify it with whether it is valid or not
assign b0_data = b_data[`DWIDTH-1:0] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[0]}};
assign b1_data = b_data[2*`DWIDTH-1:1*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[1]}};
assign b2_data = b_data[3*`DWIDTH-1:2*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[2]}};
assign b3_data = b_data[4*`DWIDTH-1:3*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[3]}};
assign b4_data = b_data[5*`DWIDTH-1:4*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[4]}};
assign b5_data = b_data[6*`DWIDTH-1:5*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[5]}};
assign b6_data = b_data[7*`DWIDTH-1:6*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[6]}};
assign b7_data = b_data[8*`DWIDTH-1:7*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[7]}};
assign b8_data = b_data[9*`DWIDTH-1:8*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[8]}};
assign b9_data = b_data[10*`DWIDTH-1:9*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[9]}};
assign b10_data = b_data[11*`DWIDTH-1:10*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[10]}};
assign b11_data = b_data[12*`DWIDTH-1:11*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[11]}};
assign b12_data = b_data[13*`DWIDTH-1:12*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[12]}};
assign b13_data = b_data[14*`DWIDTH-1:13*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[13]}};
assign b14_data = b_data[15*`DWIDTH-1:14*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[14]}};
assign b15_data = b_data[16*`DWIDTH-1:15*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[15]}};

// For larger matmuls, more such delaying flops will be needed
reg [`DWIDTH-1:0] b1_data_delayed_1;
reg [`DWIDTH-1:0] b2_data_delayed_1;
reg [`DWIDTH-1:0] b2_data_delayed_2;
reg [`DWIDTH-1:0] b3_data_delayed_1;
reg [`DWIDTH-1:0] b3_data_delayed_2;
reg [`DWIDTH-1:0] b3_data_delayed_3;
reg [`DWIDTH-1:0] b4_data_delayed_1;
reg [`DWIDTH-1:0] b4_data_delayed_2;
reg [`DWIDTH-1:0] b4_data_delayed_3;
reg [`DWIDTH-1:0] b4_data_delayed_4;
reg [`DWIDTH-1:0] b5_data_delayed_1;
reg [`DWIDTH-1:0] b5_data_delayed_2;
reg [`DWIDTH-1:0] b5_data_delayed_3;
reg [`DWIDTH-1:0] b5_data_delayed_4;
reg [`DWIDTH-1:0] b5_data_delayed_5;
reg [`DWIDTH-1:0] b6_data_delayed_1;
reg [`DWIDTH-1:0] b6_data_delayed_2;
reg [`DWIDTH-1:0] b6_data_delayed_3;
reg [`DWIDTH-1:0] b6_data_delayed_4;
reg [`DWIDTH-1:0] b6_data_delayed_5;
reg [`DWIDTH-1:0] b6_data_delayed_6;
reg [`DWIDTH-1:0] b7_data_delayed_1;
reg [`DWIDTH-1:0] b7_data_delayed_2;
reg [`DWIDTH-1:0] b7_data_delayed_3;
reg [`DWIDTH-1:0] b7_data_delayed_4;
reg [`DWIDTH-1:0] b7_data_delayed_5;
reg [`DWIDTH-1:0] b7_data_delayed_6;
reg [`DWIDTH-1:0] b7_data_delayed_7;
reg [`DWIDTH-1:0] b8_data_delayed_1;
reg [`DWIDTH-1:0] b8_data_delayed_2;
reg [`DWIDTH-1:0] b8_data_delayed_3;
reg [`DWIDTH-1:0] b8_data_delayed_4;
reg [`DWIDTH-1:0] b8_data_delayed_5;
reg [`DWIDTH-1:0] b8_data_delayed_6;
reg [`DWIDTH-1:0] b8_data_delayed_7;
reg [`DWIDTH-1:0] b8_data_delayed_8;
reg [`DWIDTH-1:0] b9_data_delayed_1;
reg [`DWIDTH-1:0] b9_data_delayed_2;
reg [`DWIDTH-1:0] b9_data_delayed_3;
reg [`DWIDTH-1:0] b9_data_delayed_4;
reg [`DWIDTH-1:0] b9_data_delayed_5;
reg [`DWIDTH-1:0] b9_data_delayed_6;
reg [`DWIDTH-1:0] b9_data_delayed_7;
reg [`DWIDTH-1:0] b9_data_delayed_8;
reg [`DWIDTH-1:0] b9_data_delayed_9;
reg [`DWIDTH-1:0] b10_data_delayed_1;
reg [`DWIDTH-1:0] b10_data_delayed_2;
reg [`DWIDTH-1:0] b10_data_delayed_3;
reg [`DWIDTH-1:0] b10_data_delayed_4;
reg [`DWIDTH-1:0] b10_data_delayed_5;
reg [`DWIDTH-1:0] b10_data_delayed_6;
reg [`DWIDTH-1:0] b10_data_delayed_7;
reg [`DWIDTH-1:0] b10_data_delayed_8;
reg [`DWIDTH-1:0] b10_data_delayed_9;
reg [`DWIDTH-1:0] b10_data_delayed_10;
reg [`DWIDTH-1:0] b11_data_delayed_1;
reg [`DWIDTH-1:0] b11_data_delayed_2;
reg [`DWIDTH-1:0] b11_data_delayed_3;
reg [`DWIDTH-1:0] b11_data_delayed_4;
reg [`DWIDTH-1:0] b11_data_delayed_5;
reg [`DWIDTH-1:0] b11_data_delayed_6;
reg [`DWIDTH-1:0] b11_data_delayed_7;
reg [`DWIDTH-1:0] b11_data_delayed_8;
reg [`DWIDTH-1:0] b11_data_delayed_9;
reg [`DWIDTH-1:0] b11_data_delayed_10;
reg [`DWIDTH-1:0] b11_data_delayed_11;
reg [`DWIDTH-1:0] b12_data_delayed_1;
reg [`DWIDTH-1:0] b12_data_delayed_2;
reg [`DWIDTH-1:0] b12_data_delayed_3;
reg [`DWIDTH-1:0] b12_data_delayed_4;
reg [`DWIDTH-1:0] b12_data_delayed_5;
reg [`DWIDTH-1:0] b12_data_delayed_6;
reg [`DWIDTH-1:0] b12_data_delayed_7;
reg [`DWIDTH-1:0] b12_data_delayed_8;
reg [`DWIDTH-1:0] b12_data_delayed_9;
reg [`DWIDTH-1:0] b12_data_delayed_10;
reg [`DWIDTH-1:0] b12_data_delayed_11;
reg [`DWIDTH-1:0] b12_data_delayed_12;
reg [`DWIDTH-1:0] b13_data_delayed_1;
reg [`DWIDTH-1:0] b13_data_delayed_2;
reg [`DWIDTH-1:0] b13_data_delayed_3;
reg [`DWIDTH-1:0] b13_data_delayed_4;
reg [`DWIDTH-1:0] b13_data_delayed_5;
reg [`DWIDTH-1:0] b13_data_delayed_6;
reg [`DWIDTH-1:0] b13_data_delayed_7;
reg [`DWIDTH-1:0] b13_data_delayed_8;
reg [`DWIDTH-1:0] b13_data_delayed_9;
reg [`DWIDTH-1:0] b13_data_delayed_10;
reg [`DWIDTH-1:0] b13_data_delayed_11;
reg [`DWIDTH-1:0] b13_data_delayed_12;
reg [`DWIDTH-1:0] b13_data_delayed_13;
reg [`DWIDTH-1:0] b14_data_delayed_1;
reg [`DWIDTH-1:0] b14_data_delayed_2;
reg [`DWIDTH-1:0] b14_data_delayed_3;
reg [`DWIDTH-1:0] b14_data_delayed_4;
reg [`DWIDTH-1:0] b14_data_delayed_5;
reg [`DWIDTH-1:0] b14_data_delayed_6;
reg [`DWIDTH-1:0] b14_data_delayed_7;
reg [`DWIDTH-1:0] b14_data_delayed_8;
reg [`DWIDTH-1:0] b14_data_delayed_9;
reg [`DWIDTH-1:0] b14_data_delayed_10;
reg [`DWIDTH-1:0] b14_data_delayed_11;
reg [`DWIDTH-1:0] b14_data_delayed_12;
reg [`DWIDTH-1:0] b14_data_delayed_13;
reg [`DWIDTH-1:0] b14_data_delayed_14;
reg [`DWIDTH-1:0] b15_data_delayed_1;
reg [`DWIDTH-1:0] b15_data_delayed_2;
reg [`DWIDTH-1:0] b15_data_delayed_3;
reg [`DWIDTH-1:0] b15_data_delayed_4;
reg [`DWIDTH-1:0] b15_data_delayed_5;
reg [`DWIDTH-1:0] b15_data_delayed_6;
reg [`DWIDTH-1:0] b15_data_delayed_7;
reg [`DWIDTH-1:0] b15_data_delayed_8;
reg [`DWIDTH-1:0] b15_data_delayed_9;
reg [`DWIDTH-1:0] b15_data_delayed_10;
reg [`DWIDTH-1:0] b15_data_delayed_11;
reg [`DWIDTH-1:0] b15_data_delayed_12;
reg [`DWIDTH-1:0] b15_data_delayed_13;
reg [`DWIDTH-1:0] b15_data_delayed_14;
reg [`DWIDTH-1:0] b15_data_delayed_15;

always @(posedge clk) begin
  if (reset || ~start_mat_mul || clk_cnt==0) begin
    b1_data_delayed_1 <= 0;
    b2_data_delayed_1 <= 0;
    b2_data_delayed_2 <= 0;
    b3_data_delayed_1 <= 0;
    b3_data_delayed_2 <= 0;
    b3_data_delayed_3 <= 0;
    b4_data_delayed_1 <= 0;
    b4_data_delayed_2 <= 0;
    b4_data_delayed_3 <= 0;
    b4_data_delayed_4 <= 0;
    b5_data_delayed_1 <= 0;
    b5_data_delayed_2 <= 0;
    b5_data_delayed_3 <= 0;
    b5_data_delayed_4 <= 0;
    b5_data_delayed_5 <= 0;
    b6_data_delayed_1 <= 0;
    b6_data_delayed_2 <= 0;
    b6_data_delayed_3 <= 0;
    b6_data_delayed_4 <= 0;
    b6_data_delayed_5 <= 0;
    b6_data_delayed_6 <= 0;
    b7_data_delayed_1 <= 0;
    b7_data_delayed_2 <= 0;
    b7_data_delayed_3 <= 0;
    b7_data_delayed_4 <= 0;
    b7_data_delayed_5 <= 0;
    b7_data_delayed_6 <= 0;
    b7_data_delayed_7 <= 0;
    b8_data_delayed_1 <= 0;
    b8_data_delayed_2 <= 0;
    b8_data_delayed_3 <= 0;
    b8_data_delayed_4 <= 0;
    b8_data_delayed_5 <= 0;
    b8_data_delayed_6 <= 0;
    b8_data_delayed_7 <= 0;
    b8_data_delayed_8 <= 0;
    b9_data_delayed_1 <= 0;
    b9_data_delayed_2 <= 0;
    b9_data_delayed_3 <= 0;
    b9_data_delayed_4 <= 0;
    b9_data_delayed_5 <= 0;
    b9_data_delayed_6 <= 0;
    b9_data_delayed_7 <= 0;
    b9_data_delayed_8 <= 0;
    b9_data_delayed_9 <= 0;
    b10_data_delayed_1 <= 0;
    b10_data_delayed_2 <= 0;
    b10_data_delayed_3 <= 0;
    b10_data_delayed_4 <= 0;
    b10_data_delayed_5 <= 0;
    b10_data_delayed_6 <= 0;
    b10_data_delayed_7 <= 0;
    b10_data_delayed_8 <= 0;
    b10_data_delayed_9 <= 0;
    b10_data_delayed_10 <= 0;
    b11_data_delayed_1 <= 0;
    b11_data_delayed_2 <= 0;
    b11_data_delayed_3 <= 0;
    b11_data_delayed_4 <= 0;
    b11_data_delayed_5 <= 0;
    b11_data_delayed_6 <= 0;
    b11_data_delayed_7 <= 0;
    b11_data_delayed_8 <= 0;
    b11_data_delayed_9 <= 0;
    b11_data_delayed_10 <= 0;
    b11_data_delayed_11 <= 0;
    b12_data_delayed_1 <= 0;
    b12_data_delayed_2 <= 0;
    b12_data_delayed_3 <= 0;
    b12_data_delayed_4 <= 0;
    b12_data_delayed_5 <= 0;
    b12_data_delayed_6 <= 0;
    b12_data_delayed_7 <= 0;
    b12_data_delayed_8 <= 0;
    b12_data_delayed_9 <= 0;
    b12_data_delayed_10 <= 0;
    b12_data_delayed_11 <= 0;
    b12_data_delayed_12 <= 0;
    b13_data_delayed_1 <= 0;
    b13_data_delayed_2 <= 0;
    b13_data_delayed_3 <= 0;
    b13_data_delayed_4 <= 0;
    b13_data_delayed_5 <= 0;
    b13_data_delayed_6 <= 0;
    b13_data_delayed_7 <= 0;
    b13_data_delayed_8 <= 0;
    b13_data_delayed_9 <= 0;
    b13_data_delayed_10 <= 0;
    b13_data_delayed_11 <= 0;
    b13_data_delayed_12 <= 0;
    b13_data_delayed_13 <= 0;
    b14_data_delayed_1 <= 0;
    b14_data_delayed_2 <= 0;
    b14_data_delayed_3 <= 0;
    b14_data_delayed_4 <= 0;
    b14_data_delayed_5 <= 0;
    b14_data_delayed_6 <= 0;
    b14_data_delayed_7 <= 0;
    b14_data_delayed_8 <= 0;
    b14_data_delayed_9 <= 0;
    b14_data_delayed_10 <= 0;
    b14_data_delayed_11 <= 0;
    b14_data_delayed_12 <= 0;
    b14_data_delayed_13 <= 0;
    b14_data_delayed_14 <= 0;
    b15_data_delayed_1 <= 0;
    b15_data_delayed_2 <= 0;
    b15_data_delayed_3 <= 0;
    b15_data_delayed_4 <= 0;
    b15_data_delayed_5 <= 0;
    b15_data_delayed_6 <= 0;
    b15_data_delayed_7 <= 0;
    b15_data_delayed_8 <= 0;
    b15_data_delayed_9 <= 0;
    b15_data_delayed_10 <= 0;
    b15_data_delayed_11 <= 0;
    b15_data_delayed_12 <= 0;
    b15_data_delayed_13 <= 0;
    b15_data_delayed_14 <= 0;
    b15_data_delayed_15 <= 0;
  end
  else begin
    b1_data_delayed_1 <= b1_data;
    b2_data_delayed_1 <= b2_data;
    b2_data_delayed_2 <= b2_data_delayed_1;
    b3_data_delayed_1 <= b3_data;
    b3_data_delayed_2 <= b3_data_delayed_1;
    b3_data_delayed_3 <= b3_data_delayed_2;
    b4_data_delayed_1 <= b4_data;
    b4_data_delayed_2 <= b4_data_delayed_1;
    b4_data_delayed_3 <= b4_data_delayed_2;
    b4_data_delayed_4 <= b4_data_delayed_3;
    b5_data_delayed_1 <= b5_data;
    b5_data_delayed_2 <= b5_data_delayed_1;
    b5_data_delayed_3 <= b5_data_delayed_2;
    b5_data_delayed_4 <= b5_data_delayed_3;
    b5_data_delayed_5 <= b5_data_delayed_4;
    b6_data_delayed_1 <= b6_data;
    b6_data_delayed_2 <= b6_data_delayed_1;
    b6_data_delayed_3 <= b6_data_delayed_2;
    b6_data_delayed_4 <= b6_data_delayed_3;
    b6_data_delayed_5 <= b6_data_delayed_4;
    b6_data_delayed_6 <= b6_data_delayed_5;
    b7_data_delayed_1 <= b7_data;
    b7_data_delayed_2 <= b7_data_delayed_1;
    b7_data_delayed_3 <= b7_data_delayed_2;
    b7_data_delayed_4 <= b7_data_delayed_3;
    b7_data_delayed_5 <= b7_data_delayed_4;
    b7_data_delayed_6 <= b7_data_delayed_5;
    b7_data_delayed_7 <= b7_data_delayed_6;
    b8_data_delayed_1 <= b8_data;
    b8_data_delayed_2 <= b8_data_delayed_1;
    b8_data_delayed_3 <= b8_data_delayed_2;
    b8_data_delayed_4 <= b8_data_delayed_3;
    b8_data_delayed_5 <= b8_data_delayed_4;
    b8_data_delayed_6 <= b8_data_delayed_5;
    b8_data_delayed_7 <= b8_data_delayed_6;
    b8_data_delayed_8 <= b8_data_delayed_7;
    b9_data_delayed_1 <= b9_data;
    b9_data_delayed_2 <= b9_data_delayed_1;
    b9_data_delayed_3 <= b9_data_delayed_2;
    b9_data_delayed_4 <= b9_data_delayed_3;
    b9_data_delayed_5 <= b9_data_delayed_4;
    b9_data_delayed_6 <= b9_data_delayed_5;
    b9_data_delayed_7 <= b9_data_delayed_6;
    b9_data_delayed_8 <= b9_data_delayed_7;
    b9_data_delayed_9 <= b9_data_delayed_8;
    b10_data_delayed_1 <= b10_data;
    b10_data_delayed_2 <= b10_data_delayed_1;
    b10_data_delayed_3 <= b10_data_delayed_2;
    b10_data_delayed_4 <= b10_data_delayed_3;
    b10_data_delayed_5 <= b10_data_delayed_4;
    b10_data_delayed_6 <= b10_data_delayed_5;
    b10_data_delayed_7 <= b10_data_delayed_6;
    b10_data_delayed_8 <= b10_data_delayed_7;
    b10_data_delayed_9 <= b10_data_delayed_8;
    b10_data_delayed_10 <= b10_data_delayed_9;
    b11_data_delayed_1 <= b11_data;
    b11_data_delayed_2 <= b11_data_delayed_1;
    b11_data_delayed_3 <= b11_data_delayed_2;
    b11_data_delayed_4 <= b11_data_delayed_3;
    b11_data_delayed_5 <= b11_data_delayed_4;
    b11_data_delayed_6 <= b11_data_delayed_5;
    b11_data_delayed_7 <= b11_data_delayed_6;
    b11_data_delayed_8 <= b11_data_delayed_7;
    b11_data_delayed_9 <= b11_data_delayed_8;
    b11_data_delayed_10 <= b11_data_delayed_9;
    b11_data_delayed_11 <= b11_data_delayed_10;
    b12_data_delayed_1 <= b12_data;
    b12_data_delayed_2 <= b12_data_delayed_1;
    b12_data_delayed_3 <= b12_data_delayed_2;
    b12_data_delayed_4 <= b12_data_delayed_3;
    b12_data_delayed_5 <= b12_data_delayed_4;
    b12_data_delayed_6 <= b12_data_delayed_5;
    b12_data_delayed_7 <= b12_data_delayed_6;
    b12_data_delayed_8 <= b12_data_delayed_7;
    b12_data_delayed_9 <= b12_data_delayed_8;
    b12_data_delayed_10 <= b12_data_delayed_9;
    b12_data_delayed_11 <= b12_data_delayed_10;
    b12_data_delayed_12 <= b12_data_delayed_11;
    b13_data_delayed_1 <= b13_data;
    b13_data_delayed_2 <= b13_data_delayed_1;
    b13_data_delayed_3 <= b13_data_delayed_2;
    b13_data_delayed_4 <= b13_data_delayed_3;
    b13_data_delayed_5 <= b13_data_delayed_4;
    b13_data_delayed_6 <= b13_data_delayed_5;
    b13_data_delayed_7 <= b13_data_delayed_6;
    b13_data_delayed_8 <= b13_data_delayed_7;
    b13_data_delayed_9 <= b13_data_delayed_8;
    b13_data_delayed_10 <= b13_data_delayed_9;
    b13_data_delayed_11 <= b13_data_delayed_10;
    b13_data_delayed_12 <= b13_data_delayed_11;
    b13_data_delayed_13 <= b13_data_delayed_12;
    b14_data_delayed_1 <= b14_data;
    b14_data_delayed_2 <= b14_data_delayed_1;
    b14_data_delayed_3 <= b14_data_delayed_2;
    b14_data_delayed_4 <= b14_data_delayed_3;
    b14_data_delayed_5 <= b14_data_delayed_4;
    b14_data_delayed_6 <= b14_data_delayed_5;
    b14_data_delayed_7 <= b14_data_delayed_6;
    b14_data_delayed_8 <= b14_data_delayed_7;
    b14_data_delayed_9 <= b14_data_delayed_8;
    b14_data_delayed_10 <= b14_data_delayed_9;
    b14_data_delayed_11 <= b14_data_delayed_10;
    b14_data_delayed_12 <= b14_data_delayed_11;
    b14_data_delayed_13 <= b14_data_delayed_12;
    b14_data_delayed_14 <= b14_data_delayed_13;
    b15_data_delayed_1 <= b15_data;
    b15_data_delayed_2 <= b15_data_delayed_1;
    b15_data_delayed_3 <= b15_data_delayed_2;
    b15_data_delayed_4 <= b15_data_delayed_3;
    b15_data_delayed_5 <= b15_data_delayed_4;
    b15_data_delayed_6 <= b15_data_delayed_5;
    b15_data_delayed_7 <= b15_data_delayed_6;
    b15_data_delayed_8 <= b15_data_delayed_7;
    b15_data_delayed_9 <= b15_data_delayed_8;
    b15_data_delayed_10 <= b15_data_delayed_9;
    b15_data_delayed_11 <= b15_data_delayed_10;
    b15_data_delayed_12 <= b15_data_delayed_11;
    b15_data_delayed_13 <= b15_data_delayed_12;
    b15_data_delayed_14 <= b15_data_delayed_13;
    b15_data_delayed_15 <= b15_data_delayed_14;
  end
end
  
endmodule

//////////////////////////////////////////////////////////////////////////
// Systolically connected PEs
//////////////////////////////////////////////////////////////////////////

module systolic_pe_matrix(
    reset,
    clk,
    pe_reset,
    b_data_sel,
    a0,    a1,    a2,    a3,    a4,    a5,    a6,    a7,    a8,    a9,    a10,    a11,    a12,    a13,    a14,    a15,
    b0,    b1,    b2,    b3,    b4,    b5,    b6,    b7,    b8,    b9,    b10,    b11,    b12,    b13,    b14,    b15,
    c0,    c1,    c2,    c3,    c4,    c5,    c6,    c7,    c8,    c9,    c10,    c11,    c12,    c13,    c14,    c15,
    matrixC0_0,
    matrixC0_1,
    matrixC0_2,
    matrixC0_3,
    matrixC0_4,
    matrixC0_5,
    matrixC0_6,
    matrixC0_7,
    matrixC0_8,
    matrixC0_9,
    matrixC0_10,
    matrixC0_11,
    matrixC0_12,
    matrixC0_13,
    matrixC0_14,
    matrixC0_15,
    matrixC1_0,
    matrixC1_1,
    matrixC1_2,
    matrixC1_3,
    matrixC1_4,
    matrixC1_5,
    matrixC1_6,
    matrixC1_7,
    matrixC1_8,
    matrixC1_9,
    matrixC1_10,
    matrixC1_11,
    matrixC1_12,
    matrixC1_13,
    matrixC1_14,
    matrixC1_15,
    matrixC2_0,
    matrixC2_1,
    matrixC2_2,
    matrixC2_3,
    matrixC2_4,
    matrixC2_5,
    matrixC2_6,
    matrixC2_7,
    matrixC2_8,
    matrixC2_9,
    matrixC2_10,
    matrixC2_11,
    matrixC2_12,
    matrixC2_13,
    matrixC2_14,
    matrixC2_15,
    matrixC3_0,
    matrixC3_1,
    matrixC3_2,
    matrixC3_3,
    matrixC3_4,
    matrixC3_5,
    matrixC3_6,
    matrixC3_7,
    matrixC3_8,
    matrixC3_9,
    matrixC3_10,
    matrixC3_11,
    matrixC3_12,
    matrixC3_13,
    matrixC3_14,
    matrixC3_15,
    matrixC4_0,
    matrixC4_1,
    matrixC4_2,
    matrixC4_3,
    matrixC4_4,
    matrixC4_5,
    matrixC4_6,
    matrixC4_7,
    matrixC4_8,
    matrixC4_9,
    matrixC4_10,
    matrixC4_11,
    matrixC4_12,
    matrixC4_13,
    matrixC4_14,
    matrixC4_15,
    matrixC5_0,
    matrixC5_1,
    matrixC5_2,
    matrixC5_3,
    matrixC5_4,
    matrixC5_5,
    matrixC5_6,
    matrixC5_7,
    matrixC5_8,
    matrixC5_9,
    matrixC5_10,
    matrixC5_11,
    matrixC5_12,
    matrixC5_13,
    matrixC5_14,
    matrixC5_15,
    matrixC6_0,
    matrixC6_1,
    matrixC6_2,
    matrixC6_3,
    matrixC6_4,
    matrixC6_5,
    matrixC6_6,
    matrixC6_7,
    matrixC6_8,
    matrixC6_9,
    matrixC6_10,
    matrixC6_11,
    matrixC6_12,
    matrixC6_13,
    matrixC6_14,
    matrixC6_15,
    matrixC7_0,
    matrixC7_1,
    matrixC7_2,
    matrixC7_3,
    matrixC7_4,
    matrixC7_5,
    matrixC7_6,
    matrixC7_7,
    matrixC7_8,
    matrixC7_9,
    matrixC7_10,
    matrixC7_11,
    matrixC7_12,
    matrixC7_13,
    matrixC7_14,
    matrixC7_15,
    matrixC8_0,
    matrixC8_1,
    matrixC8_2,
    matrixC8_3,
    matrixC8_4,
    matrixC8_5,
    matrixC8_6,
    matrixC8_7,
    matrixC8_8,
    matrixC8_9,
    matrixC8_10,
    matrixC8_11,
    matrixC8_12,
    matrixC8_13,
    matrixC8_14,
    matrixC8_15,
    matrixC9_0,
    matrixC9_1,
    matrixC9_2,
    matrixC9_3,
    matrixC9_4,
    matrixC9_5,
    matrixC9_6,
    matrixC9_7,
    matrixC9_8,
    matrixC9_9,
    matrixC9_10,
    matrixC9_11,
    matrixC9_12,
    matrixC9_13,
    matrixC9_14,
    matrixC9_15,
    matrixC10_0,
    matrixC10_1,
    matrixC10_2,
    matrixC10_3,
    matrixC10_4,
    matrixC10_5,
    matrixC10_6,
    matrixC10_7,
    matrixC10_8,
    matrixC10_9,
    matrixC10_10,
    matrixC10_11,
    matrixC10_12,
    matrixC10_13,
    matrixC10_14,
    matrixC10_15,
    matrixC11_0,
    matrixC11_1,
    matrixC11_2,
    matrixC11_3,
    matrixC11_4,
    matrixC11_5,
    matrixC11_6,
    matrixC11_7,
    matrixC11_8,
    matrixC11_9,
    matrixC11_10,
    matrixC11_11,
    matrixC11_12,
    matrixC11_13,
    matrixC11_14,
    matrixC11_15,
    matrixC12_0,
    matrixC12_1,
    matrixC12_2,
    matrixC12_3,
    matrixC12_4,
    matrixC12_5,
    matrixC12_6,
    matrixC12_7,
    matrixC12_8,
    matrixC12_9,
    matrixC12_10,
    matrixC12_11,
    matrixC12_12,
    matrixC12_13,
    matrixC12_14,
    matrixC12_15,
    matrixC13_0,
    matrixC13_1,
    matrixC13_2,
    matrixC13_3,
    matrixC13_4,
    matrixC13_5,
    matrixC13_6,
    matrixC13_7,
    matrixC13_8,
    matrixC13_9,
    matrixC13_10,
    matrixC13_11,
    matrixC13_12,
    matrixC13_13,
    matrixC13_14,
    matrixC13_15,
    matrixC14_0,
    matrixC14_1,
    matrixC14_2,
    matrixC14_3,
    matrixC14_4,
    matrixC14_5,
    matrixC14_6,
    matrixC14_7,
    matrixC14_8,
    matrixC14_9,
    matrixC14_10,
    matrixC14_11,
    matrixC14_12,
    matrixC14_13,
    matrixC14_14,
    matrixC14_15,
    matrixC15_0,
    matrixC15_1,
    matrixC15_2,
    matrixC15_3,
    matrixC15_4,
    matrixC15_5,
    matrixC15_6,
    matrixC15_7,
    matrixC15_8,
    matrixC15_9,
    matrixC15_10,
    matrixC15_11,
    matrixC15_12,
    matrixC15_13,
    matrixC15_14,
    matrixC15_15,
    a_data_out,
    b_data_out,
    b_data_valid_ping,
    b_data_valid_pong
);

input clk;
input reset;
input pe_reset;
input b_data_sel;
input b_data_valid_ping;
input b_data_valid_pong;
input [`DWIDTH-1:0] a0;
input [`DWIDTH-1:0] a1;
input [`DWIDTH-1:0] a2;
input [`DWIDTH-1:0] a3;
input [`DWIDTH-1:0] a4;
input [`DWIDTH-1:0] a5;
input [`DWIDTH-1:0] a6;
input [`DWIDTH-1:0] a7;
input [`DWIDTH-1:0] a8;
input [`DWIDTH-1:0] a9;
input [`DWIDTH-1:0] a10;
input [`DWIDTH-1:0] a11;
input [`DWIDTH-1:0] a12;
input [`DWIDTH-1:0] a13;
input [`DWIDTH-1:0] a14;
input [`DWIDTH-1:0] a15;
input [`DWIDTH-1:0] b0;
input [`DWIDTH-1:0] b1;
input [`DWIDTH-1:0] b2;
input [`DWIDTH-1:0] b3;
input [`DWIDTH-1:0] b4;
input [`DWIDTH-1:0] b5;
input [`DWIDTH-1:0] b6;
input [`DWIDTH-1:0] b7;
input [`DWIDTH-1:0] b8;
input [`DWIDTH-1:0] b9;
input [`DWIDTH-1:0] b10;
input [`DWIDTH-1:0] b11;
input [`DWIDTH-1:0] b12;
input [`DWIDTH-1:0] b13;
input [`DWIDTH-1:0] b14;
input [`DWIDTH-1:0] b15;
input [`DWIDTH-1:0] c0;
input [`DWIDTH-1:0] c1;
input [`DWIDTH-1:0] c2;
input [`DWIDTH-1:0] c3;
input [`DWIDTH-1:0] c4;
input [`DWIDTH-1:0] c5;
input [`DWIDTH-1:0] c6;
input [`DWIDTH-1:0] c7;
input [`DWIDTH-1:0] c8;
input [`DWIDTH-1:0] c9;
input [`DWIDTH-1:0] c10;
input [`DWIDTH-1:0] c11;
input [`DWIDTH-1:0] c12;
input [`DWIDTH-1:0] c13;
input [`DWIDTH-1:0] c14;
input [`DWIDTH-1:0] c15;
output [`DWIDTH-1:0] matrixC0_0;
output [`DWIDTH-1:0] matrixC0_1;
output [`DWIDTH-1:0] matrixC0_2;
output [`DWIDTH-1:0] matrixC0_3;
output [`DWIDTH-1:0] matrixC0_4;
output [`DWIDTH-1:0] matrixC0_5;
output [`DWIDTH-1:0] matrixC0_6;
output [`DWIDTH-1:0] matrixC0_7;
output [`DWIDTH-1:0] matrixC0_8;
output [`DWIDTH-1:0] matrixC0_9;
output [`DWIDTH-1:0] matrixC0_10;
output [`DWIDTH-1:0] matrixC0_11;
output [`DWIDTH-1:0] matrixC0_12;
output [`DWIDTH-1:0] matrixC0_13;
output [`DWIDTH-1:0] matrixC0_14;
output [`DWIDTH-1:0] matrixC0_15;
output [`DWIDTH-1:0] matrixC1_0;
output [`DWIDTH-1:0] matrixC1_1;
output [`DWIDTH-1:0] matrixC1_2;
output [`DWIDTH-1:0] matrixC1_3;
output [`DWIDTH-1:0] matrixC1_4;
output [`DWIDTH-1:0] matrixC1_5;
output [`DWIDTH-1:0] matrixC1_6;
output [`DWIDTH-1:0] matrixC1_7;
output [`DWIDTH-1:0] matrixC1_8;
output [`DWIDTH-1:0] matrixC1_9;
output [`DWIDTH-1:0] matrixC1_10;
output [`DWIDTH-1:0] matrixC1_11;
output [`DWIDTH-1:0] matrixC1_12;
output [`DWIDTH-1:0] matrixC1_13;
output [`DWIDTH-1:0] matrixC1_14;
output [`DWIDTH-1:0] matrixC1_15;
output [`DWIDTH-1:0] matrixC2_0;
output [`DWIDTH-1:0] matrixC2_1;
output [`DWIDTH-1:0] matrixC2_2;
output [`DWIDTH-1:0] matrixC2_3;
output [`DWIDTH-1:0] matrixC2_4;
output [`DWIDTH-1:0] matrixC2_5;
output [`DWIDTH-1:0] matrixC2_6;
output [`DWIDTH-1:0] matrixC2_7;
output [`DWIDTH-1:0] matrixC2_8;
output [`DWIDTH-1:0] matrixC2_9;
output [`DWIDTH-1:0] matrixC2_10;
output [`DWIDTH-1:0] matrixC2_11;
output [`DWIDTH-1:0] matrixC2_12;
output [`DWIDTH-1:0] matrixC2_13;
output [`DWIDTH-1:0] matrixC2_14;
output [`DWIDTH-1:0] matrixC2_15;
output [`DWIDTH-1:0] matrixC3_0;
output [`DWIDTH-1:0] matrixC3_1;
output [`DWIDTH-1:0] matrixC3_2;
output [`DWIDTH-1:0] matrixC3_3;
output [`DWIDTH-1:0] matrixC3_4;
output [`DWIDTH-1:0] matrixC3_5;
output [`DWIDTH-1:0] matrixC3_6;
output [`DWIDTH-1:0] matrixC3_7;
output [`DWIDTH-1:0] matrixC3_8;
output [`DWIDTH-1:0] matrixC3_9;
output [`DWIDTH-1:0] matrixC3_10;
output [`DWIDTH-1:0] matrixC3_11;
output [`DWIDTH-1:0] matrixC3_12;
output [`DWIDTH-1:0] matrixC3_13;
output [`DWIDTH-1:0] matrixC3_14;
output [`DWIDTH-1:0] matrixC3_15;
output [`DWIDTH-1:0] matrixC4_0;
output [`DWIDTH-1:0] matrixC4_1;
output [`DWIDTH-1:0] matrixC4_2;
output [`DWIDTH-1:0] matrixC4_3;
output [`DWIDTH-1:0] matrixC4_4;
output [`DWIDTH-1:0] matrixC4_5;
output [`DWIDTH-1:0] matrixC4_6;
output [`DWIDTH-1:0] matrixC4_7;
output [`DWIDTH-1:0] matrixC4_8;
output [`DWIDTH-1:0] matrixC4_9;
output [`DWIDTH-1:0] matrixC4_10;
output [`DWIDTH-1:0] matrixC4_11;
output [`DWIDTH-1:0] matrixC4_12;
output [`DWIDTH-1:0] matrixC4_13;
output [`DWIDTH-1:0] matrixC4_14;
output [`DWIDTH-1:0] matrixC4_15;
output [`DWIDTH-1:0] matrixC5_0;
output [`DWIDTH-1:0] matrixC5_1;
output [`DWIDTH-1:0] matrixC5_2;
output [`DWIDTH-1:0] matrixC5_3;
output [`DWIDTH-1:0] matrixC5_4;
output [`DWIDTH-1:0] matrixC5_5;
output [`DWIDTH-1:0] matrixC5_6;
output [`DWIDTH-1:0] matrixC5_7;
output [`DWIDTH-1:0] matrixC5_8;
output [`DWIDTH-1:0] matrixC5_9;
output [`DWIDTH-1:0] matrixC5_10;
output [`DWIDTH-1:0] matrixC5_11;
output [`DWIDTH-1:0] matrixC5_12;
output [`DWIDTH-1:0] matrixC5_13;
output [`DWIDTH-1:0] matrixC5_14;
output [`DWIDTH-1:0] matrixC5_15;
output [`DWIDTH-1:0] matrixC6_0;
output [`DWIDTH-1:0] matrixC6_1;
output [`DWIDTH-1:0] matrixC6_2;
output [`DWIDTH-1:0] matrixC6_3;
output [`DWIDTH-1:0] matrixC6_4;
output [`DWIDTH-1:0] matrixC6_5;
output [`DWIDTH-1:0] matrixC6_6;
output [`DWIDTH-1:0] matrixC6_7;
output [`DWIDTH-1:0] matrixC6_8;
output [`DWIDTH-1:0] matrixC6_9;
output [`DWIDTH-1:0] matrixC6_10;
output [`DWIDTH-1:0] matrixC6_11;
output [`DWIDTH-1:0] matrixC6_12;
output [`DWIDTH-1:0] matrixC6_13;
output [`DWIDTH-1:0] matrixC6_14;
output [`DWIDTH-1:0] matrixC6_15;
output [`DWIDTH-1:0] matrixC7_0;
output [`DWIDTH-1:0] matrixC7_1;
output [`DWIDTH-1:0] matrixC7_2;
output [`DWIDTH-1:0] matrixC7_3;
output [`DWIDTH-1:0] matrixC7_4;
output [`DWIDTH-1:0] matrixC7_5;
output [`DWIDTH-1:0] matrixC7_6;
output [`DWIDTH-1:0] matrixC7_7;
output [`DWIDTH-1:0] matrixC7_8;
output [`DWIDTH-1:0] matrixC7_9;
output [`DWIDTH-1:0] matrixC7_10;
output [`DWIDTH-1:0] matrixC7_11;
output [`DWIDTH-1:0] matrixC7_12;
output [`DWIDTH-1:0] matrixC7_13;
output [`DWIDTH-1:0] matrixC7_14;
output [`DWIDTH-1:0] matrixC7_15;
output [`DWIDTH-1:0] matrixC8_0;
output [`DWIDTH-1:0] matrixC8_1;
output [`DWIDTH-1:0] matrixC8_2;
output [`DWIDTH-1:0] matrixC8_3;
output [`DWIDTH-1:0] matrixC8_4;
output [`DWIDTH-1:0] matrixC8_5;
output [`DWIDTH-1:0] matrixC8_6;
output [`DWIDTH-1:0] matrixC8_7;
output [`DWIDTH-1:0] matrixC8_8;
output [`DWIDTH-1:0] matrixC8_9;
output [`DWIDTH-1:0] matrixC8_10;
output [`DWIDTH-1:0] matrixC8_11;
output [`DWIDTH-1:0] matrixC8_12;
output [`DWIDTH-1:0] matrixC8_13;
output [`DWIDTH-1:0] matrixC8_14;
output [`DWIDTH-1:0] matrixC8_15;
output [`DWIDTH-1:0] matrixC9_0;
output [`DWIDTH-1:0] matrixC9_1;
output [`DWIDTH-1:0] matrixC9_2;
output [`DWIDTH-1:0] matrixC9_3;
output [`DWIDTH-1:0] matrixC9_4;
output [`DWIDTH-1:0] matrixC9_5;
output [`DWIDTH-1:0] matrixC9_6;
output [`DWIDTH-1:0] matrixC9_7;
output [`DWIDTH-1:0] matrixC9_8;
output [`DWIDTH-1:0] matrixC9_9;
output [`DWIDTH-1:0] matrixC9_10;
output [`DWIDTH-1:0] matrixC9_11;
output [`DWIDTH-1:0] matrixC9_12;
output [`DWIDTH-1:0] matrixC9_13;
output [`DWIDTH-1:0] matrixC9_14;
output [`DWIDTH-1:0] matrixC9_15;
output [`DWIDTH-1:0] matrixC10_0;
output [`DWIDTH-1:0] matrixC10_1;
output [`DWIDTH-1:0] matrixC10_2;
output [`DWIDTH-1:0] matrixC10_3;
output [`DWIDTH-1:0] matrixC10_4;
output [`DWIDTH-1:0] matrixC10_5;
output [`DWIDTH-1:0] matrixC10_6;
output [`DWIDTH-1:0] matrixC10_7;
output [`DWIDTH-1:0] matrixC10_8;
output [`DWIDTH-1:0] matrixC10_9;
output [`DWIDTH-1:0] matrixC10_10;
output [`DWIDTH-1:0] matrixC10_11;
output [`DWIDTH-1:0] matrixC10_12;
output [`DWIDTH-1:0] matrixC10_13;
output [`DWIDTH-1:0] matrixC10_14;
output [`DWIDTH-1:0] matrixC10_15;
output [`DWIDTH-1:0] matrixC11_0;
output [`DWIDTH-1:0] matrixC11_1;
output [`DWIDTH-1:0] matrixC11_2;
output [`DWIDTH-1:0] matrixC11_3;
output [`DWIDTH-1:0] matrixC11_4;
output [`DWIDTH-1:0] matrixC11_5;
output [`DWIDTH-1:0] matrixC11_6;
output [`DWIDTH-1:0] matrixC11_7;
output [`DWIDTH-1:0] matrixC11_8;
output [`DWIDTH-1:0] matrixC11_9;
output [`DWIDTH-1:0] matrixC11_10;
output [`DWIDTH-1:0] matrixC11_11;
output [`DWIDTH-1:0] matrixC11_12;
output [`DWIDTH-1:0] matrixC11_13;
output [`DWIDTH-1:0] matrixC11_14;
output [`DWIDTH-1:0] matrixC11_15;
output [`DWIDTH-1:0] matrixC12_0;
output [`DWIDTH-1:0] matrixC12_1;
output [`DWIDTH-1:0] matrixC12_2;
output [`DWIDTH-1:0] matrixC12_3;
output [`DWIDTH-1:0] matrixC12_4;
output [`DWIDTH-1:0] matrixC12_5;
output [`DWIDTH-1:0] matrixC12_6;
output [`DWIDTH-1:0] matrixC12_7;
output [`DWIDTH-1:0] matrixC12_8;
output [`DWIDTH-1:0] matrixC12_9;
output [`DWIDTH-1:0] matrixC12_10;
output [`DWIDTH-1:0] matrixC12_11;
output [`DWIDTH-1:0] matrixC12_12;
output [`DWIDTH-1:0] matrixC12_13;
output [`DWIDTH-1:0] matrixC12_14;
output [`DWIDTH-1:0] matrixC12_15;
output [`DWIDTH-1:0] matrixC13_0;
output [`DWIDTH-1:0] matrixC13_1;
output [`DWIDTH-1:0] matrixC13_2;
output [`DWIDTH-1:0] matrixC13_3;
output [`DWIDTH-1:0] matrixC13_4;
output [`DWIDTH-1:0] matrixC13_5;
output [`DWIDTH-1:0] matrixC13_6;
output [`DWIDTH-1:0] matrixC13_7;
output [`DWIDTH-1:0] matrixC13_8;
output [`DWIDTH-1:0] matrixC13_9;
output [`DWIDTH-1:0] matrixC13_10;
output [`DWIDTH-1:0] matrixC13_11;
output [`DWIDTH-1:0] matrixC13_12;
output [`DWIDTH-1:0] matrixC13_13;
output [`DWIDTH-1:0] matrixC13_14;
output [`DWIDTH-1:0] matrixC13_15;
output [`DWIDTH-1:0] matrixC14_0;
output [`DWIDTH-1:0] matrixC14_1;
output [`DWIDTH-1:0] matrixC14_2;
output [`DWIDTH-1:0] matrixC14_3;
output [`DWIDTH-1:0] matrixC14_4;
output [`DWIDTH-1:0] matrixC14_5;
output [`DWIDTH-1:0] matrixC14_6;
output [`DWIDTH-1:0] matrixC14_7;
output [`DWIDTH-1:0] matrixC14_8;
output [`DWIDTH-1:0] matrixC14_9;
output [`DWIDTH-1:0] matrixC14_10;
output [`DWIDTH-1:0] matrixC14_11;
output [`DWIDTH-1:0] matrixC14_12;
output [`DWIDTH-1:0] matrixC14_13;
output [`DWIDTH-1:0] matrixC14_14;
output [`DWIDTH-1:0] matrixC14_15;
output [`DWIDTH-1:0] matrixC15_0;
output [`DWIDTH-1:0] matrixC15_1;
output [`DWIDTH-1:0] matrixC15_2;
output [`DWIDTH-1:0] matrixC15_3;
output [`DWIDTH-1:0] matrixC15_4;
output [`DWIDTH-1:0] matrixC15_5;
output [`DWIDTH-1:0] matrixC15_6;
output [`DWIDTH-1:0] matrixC15_7;
output [`DWIDTH-1:0] matrixC15_8;
output [`DWIDTH-1:0] matrixC15_9;
output [`DWIDTH-1:0] matrixC15_10;
output [`DWIDTH-1:0] matrixC15_11;
output [`DWIDTH-1:0] matrixC15_12;
output [`DWIDTH-1:0] matrixC15_13;
output [`DWIDTH-1:0] matrixC15_14;
output [`DWIDTH-1:0] matrixC15_15;
output [`MAT_MUL_SIZE*`DWIDTH-1:0] a_data_out;
output [`MAT_MUL_SIZE*`DWIDTH-1:0] b_data_out;
  

wire [`DWIDTH-1:0] a0_0to0_1, a0_1to0_2, a0_2to0_3, a0_3to0_4, a0_4to0_5, a0_5to0_6, a0_6to0_7, a0_7to0_8, a0_8to0_9, a0_9to0_10, a0_10to0_11, a0_11to0_12, a0_12to0_13, a0_13to0_14, a0_14to0_15, a0_15to0_16;
wire [`DWIDTH-1:0] a1_0to1_1, a1_1to1_2, a1_2to1_3, a1_3to1_4, a1_4to1_5, a1_5to1_6, a1_6to1_7, a1_7to1_8, a1_8to1_9, a1_9to1_10, a1_10to1_11, a1_11to1_12, a1_12to1_13, a1_13to1_14, a1_14to1_15, a1_15to1_16;
wire [`DWIDTH-1:0] a2_0to2_1, a2_1to2_2, a2_2to2_3, a2_3to2_4, a2_4to2_5, a2_5to2_6, a2_6to2_7, a2_7to2_8, a2_8to2_9, a2_9to2_10, a2_10to2_11, a2_11to2_12, a2_12to2_13, a2_13to2_14, a2_14to2_15, a2_15to2_16;
wire [`DWIDTH-1:0] a3_0to3_1, a3_1to3_2, a3_2to3_3, a3_3to3_4, a3_4to3_5, a3_5to3_6, a3_6to3_7, a3_7to3_8, a3_8to3_9, a3_9to3_10, a3_10to3_11, a3_11to3_12, a3_12to3_13, a3_13to3_14, a3_14to3_15, a3_15to3_16;
wire [`DWIDTH-1:0] a4_0to4_1, a4_1to4_2, a4_2to4_3, a4_3to4_4, a4_4to4_5, a4_5to4_6, a4_6to4_7, a4_7to4_8, a4_8to4_9, a4_9to4_10, a4_10to4_11, a4_11to4_12, a4_12to4_13, a4_13to4_14, a4_14to4_15, a4_15to4_16;
wire [`DWIDTH-1:0] a5_0to5_1, a5_1to5_2, a5_2to5_3, a5_3to5_4, a5_4to5_5, a5_5to5_6, a5_6to5_7, a5_7to5_8, a5_8to5_9, a5_9to5_10, a5_10to5_11, a5_11to5_12, a5_12to5_13, a5_13to5_14, a5_14to5_15, a5_15to5_16;
wire [`DWIDTH-1:0] a6_0to6_1, a6_1to6_2, a6_2to6_3, a6_3to6_4, a6_4to6_5, a6_5to6_6, a6_6to6_7, a6_7to6_8, a6_8to6_9, a6_9to6_10, a6_10to6_11, a6_11to6_12, a6_12to6_13, a6_13to6_14, a6_14to6_15, a6_15to6_16;
wire [`DWIDTH-1:0] a7_0to7_1, a7_1to7_2, a7_2to7_3, a7_3to7_4, a7_4to7_5, a7_5to7_6, a7_6to7_7, a7_7to7_8, a7_8to7_9, a7_9to7_10, a7_10to7_11, a7_11to7_12, a7_12to7_13, a7_13to7_14, a7_14to7_15, a7_15to7_16;
wire [`DWIDTH-1:0] a8_0to8_1, a8_1to8_2, a8_2to8_3, a8_3to8_4, a8_4to8_5, a8_5to8_6, a8_6to8_7, a8_7to8_8, a8_8to8_9, a8_9to8_10, a8_10to8_11, a8_11to8_12, a8_12to8_13, a8_13to8_14, a8_14to8_15, a8_15to8_16;
wire [`DWIDTH-1:0] a9_0to9_1, a9_1to9_2, a9_2to9_3, a9_3to9_4, a9_4to9_5, a9_5to9_6, a9_6to9_7, a9_7to9_8, a9_8to9_9, a9_9to9_10, a9_10to9_11, a9_11to9_12, a9_12to9_13, a9_13to9_14, a9_14to9_15, a9_15to9_16;
wire [`DWIDTH-1:0] a10_0to10_1, a10_1to10_2, a10_2to10_3, a10_3to10_4, a10_4to10_5, a10_5to10_6, a10_6to10_7, a10_7to10_8, a10_8to10_9, a10_9to10_10, a10_10to10_11, a10_11to10_12, a10_12to10_13, a10_13to10_14, a10_14to10_15, a10_15to10_16;
wire [`DWIDTH-1:0] a11_0to11_1, a11_1to11_2, a11_2to11_3, a11_3to11_4, a11_4to11_5, a11_5to11_6, a11_6to11_7, a11_7to11_8, a11_8to11_9, a11_9to11_10, a11_10to11_11, a11_11to11_12, a11_12to11_13, a11_13to11_14, a11_14to11_15, a11_15to11_16;
wire [`DWIDTH-1:0] a12_0to12_1, a12_1to12_2, a12_2to12_3, a12_3to12_4, a12_4to12_5, a12_5to12_6, a12_6to12_7, a12_7to12_8, a12_8to12_9, a12_9to12_10, a12_10to12_11, a12_11to12_12, a12_12to12_13, a12_13to12_14, a12_14to12_15, a12_15to12_16;
wire [`DWIDTH-1:0] a13_0to13_1, a13_1to13_2, a13_2to13_3, a13_3to13_4, a13_4to13_5, a13_5to13_6, a13_6to13_7, a13_7to13_8, a13_8to13_9, a13_9to13_10, a13_10to13_11, a13_11to13_12, a13_12to13_13, a13_13to13_14, a13_14to13_15, a13_15to13_16;
wire [`DWIDTH-1:0] a14_0to14_1, a14_1to14_2, a14_2to14_3, a14_3to14_4, a14_4to14_5, a14_5to14_6, a14_6to14_7, a14_7to14_8, a14_8to14_9, a14_9to14_10, a14_10to14_11, a14_11to14_12, a14_12to14_13, a14_13to14_14, a14_14to14_15, a14_15to14_16;
wire [`DWIDTH-1:0] a15_0to15_1, a15_1to15_2, a15_2to15_3, a15_3to15_4, a15_4to15_5, a15_5to15_6, a15_6to15_7, a15_7to15_8, a15_8to15_9, a15_9to15_10, a15_10to15_11, a15_11to15_12, a15_12to15_13, a15_13to15_14, a15_14to15_15, a15_15to15_16;

wire [`DWIDTH-1:0] b0_0to1_0, b1_0to2_0, b2_0to3_0, b3_0to4_0, b4_0to5_0, b5_0to6_0, b6_0to7_0, b7_0to8_0, b8_0to9_0, b9_0to10_0, b10_0to11_0, b11_0to12_0, b12_0to13_0, b13_0to14_0, b14_0to15_0, b15_0to16_0;
wire [`DWIDTH-1:0] b0_1to1_1, b1_1to2_1, b2_1to3_1, b3_1to4_1, b4_1to5_1, b5_1to6_1, b6_1to7_1, b7_1to8_1, b8_1to9_1, b9_1to10_1, b10_1to11_1, b11_1to12_1, b12_1to13_1, b13_1to14_1, b14_1to15_1, b15_1to16_1;
wire [`DWIDTH-1:0] b0_2to1_2, b1_2to2_2, b2_2to3_2, b3_2to4_2, b4_2to5_2, b5_2to6_2, b6_2to7_2, b7_2to8_2, b8_2to9_2, b9_2to10_2, b10_2to11_2, b11_2to12_2, b12_2to13_2, b13_2to14_2, b14_2to15_2, b15_2to16_2;
wire [`DWIDTH-1:0] b0_3to1_3, b1_3to2_3, b2_3to3_3, b3_3to4_3, b4_3to5_3, b5_3to6_3, b6_3to7_3, b7_3to8_3, b8_3to9_3, b9_3to10_3, b10_3to11_3, b11_3to12_3, b12_3to13_3, b13_3to14_3, b14_3to15_3, b15_3to16_3;
wire [`DWIDTH-1:0] b0_4to1_4, b1_4to2_4, b2_4to3_4, b3_4to4_4, b4_4to5_4, b5_4to6_4, b6_4to7_4, b7_4to8_4, b8_4to9_4, b9_4to10_4, b10_4to11_4, b11_4to12_4, b12_4to13_4, b13_4to14_4, b14_4to15_4, b15_4to16_4;
wire [`DWIDTH-1:0] b0_5to1_5, b1_5to2_5, b2_5to3_5, b3_5to4_5, b4_5to5_5, b5_5to6_5, b6_5to7_5, b7_5to8_5, b8_5to9_5, b9_5to10_5, b10_5to11_5, b11_5to12_5, b12_5to13_5, b13_5to14_5, b14_5to15_5, b15_5to16_5;
wire [`DWIDTH-1:0] b0_6to1_6, b1_6to2_6, b2_6to3_6, b3_6to4_6, b4_6to5_6, b5_6to6_6, b6_6to7_6, b7_6to8_6, b8_6to9_6, b9_6to10_6, b10_6to11_6, b11_6to12_6, b12_6to13_6, b13_6to14_6, b14_6to15_6, b15_6to16_6;
wire [`DWIDTH-1:0] b0_7to1_7, b1_7to2_7, b2_7to3_7, b3_7to4_7, b4_7to5_7, b5_7to6_7, b6_7to7_7, b7_7to8_7, b8_7to9_7, b9_7to10_7, b10_7to11_7, b11_7to12_7, b12_7to13_7, b13_7to14_7, b14_7to15_7, b15_7to16_7;
wire [`DWIDTH-1:0] b0_8to1_8, b1_8to2_8, b2_8to3_8, b3_8to4_8, b4_8to5_8, b5_8to6_8, b6_8to7_8, b7_8to8_8, b8_8to9_8, b9_8to10_8, b10_8to11_8, b11_8to12_8, b12_8to13_8, b13_8to14_8, b14_8to15_8, b15_8to16_8;
wire [`DWIDTH-1:0] b0_9to1_9, b1_9to2_9, b2_9to3_9, b3_9to4_9, b4_9to5_9, b5_9to6_9, b6_9to7_9, b7_9to8_9, b8_9to9_9, b9_9to10_9, b10_9to11_9, b11_9to12_9, b12_9to13_9, b13_9to14_9, b14_9to15_9, b15_9to16_9;
wire [`DWIDTH-1:0] b0_10to1_10, b1_10to2_10, b2_10to3_10, b3_10to4_10, b4_10to5_10, b5_10to6_10, b6_10to7_10, b7_10to8_10, b8_10to9_10, b9_10to10_10, b10_10to11_10, b11_10to12_10, b12_10to13_10, b13_10to14_10, b14_10to15_10, b15_10to16_10;
wire [`DWIDTH-1:0] b0_11to1_11, b1_11to2_11, b2_11to3_11, b3_11to4_11, b4_11to5_11, b5_11to6_11, b6_11to7_11, b7_11to8_11, b8_11to9_11, b9_11to10_11, b10_11to11_11, b11_11to12_11, b12_11to13_11, b13_11to14_11, b14_11to15_11, b15_11to16_11;
wire [`DWIDTH-1:0] b0_12to1_12, b1_12to2_12, b2_12to3_12, b3_12to4_12, b4_12to5_12, b5_12to6_12, b6_12to7_12, b7_12to8_12, b8_12to9_12, b9_12to10_12, b10_12to11_12, b11_12to12_12, b12_12to13_12, b13_12to14_12, b14_12to15_12, b15_12to16_12;
wire [`DWIDTH-1:0] b0_13to1_13, b1_13to2_13, b2_13to3_13, b3_13to4_13, b4_13to5_13, b5_13to6_13, b6_13to7_13, b7_13to8_13, b8_13to9_13, b9_13to10_13, b10_13to11_13, b11_13to12_13, b12_13to13_13, b13_13to14_13, b14_13to15_13, b15_13to16_13;
wire [`DWIDTH-1:0] b0_14to1_14, b1_14to2_14, b2_14to3_14, b3_14to4_14, b4_14to5_14, b5_14to6_14, b6_14to7_14, b7_14to8_14, b8_14to9_14, b9_14to10_14, b10_14to11_14, b11_14to12_14, b12_14to13_14, b13_14to14_14, b14_14to15_14, b15_14to16_14;
wire [`DWIDTH-1:0] b0_15to1_15, b1_15to2_15, b2_15to3_15, b3_15to4_15, b4_15to5_15, b5_15to6_15, b6_15to7_15, b7_15to8_15, b8_15to9_15, b9_15to10_15, b10_15to11_15, b11_15to12_15, b12_15to13_15, b13_15to14_15, b14_15to15_15, b15_15to16_15;

wire [`DWIDTH-1:0] b0_0to1_0_ping, b1_0to2_0_ping, b2_0to3_0_ping, b3_0to4_0_ping, b4_0to5_0_ping, b5_0to6_0_ping, b6_0to7_0_ping, b7_0to8_0_ping, b8_0to9_0_ping, b9_0to10_0_ping, b10_0to11_0_ping, b11_0to12_0_ping, b12_0to13_0_ping, b13_0to14_0_ping, b14_0to15_0_ping, b15_0to16_0_ping;
wire [`DWIDTH-1:0] b0_1to1_1_ping, b1_1to2_1_ping, b2_1to3_1_ping, b3_1to4_1_ping, b4_1to5_1_ping, b5_1to6_1_ping, b6_1to7_1_ping, b7_1to8_1_ping, b8_1to9_1_ping, b9_1to10_1_ping, b10_1to11_1_ping, b11_1to12_1_ping, b12_1to13_1_ping, b13_1to14_1_ping, b14_1to15_1_ping, b15_1to16_1_ping;
wire [`DWIDTH-1:0] b0_2to1_2_ping, b1_2to2_2_ping, b2_2to3_2_ping, b3_2to4_2_ping, b4_2to5_2_ping, b5_2to6_2_ping, b6_2to7_2_ping, b7_2to8_2_ping, b8_2to9_2_ping, b9_2to10_2_ping, b10_2to11_2_ping, b11_2to12_2_ping, b12_2to13_2_ping, b13_2to14_2_ping, b14_2to15_2_ping, b15_2to16_2_ping;
wire [`DWIDTH-1:0] b0_3to1_3_ping, b1_3to2_3_ping, b2_3to3_3_ping, b3_3to4_3_ping, b4_3to5_3_ping, b5_3to6_3_ping, b6_3to7_3_ping, b7_3to8_3_ping, b8_3to9_3_ping, b9_3to10_3_ping, b10_3to11_3_ping, b11_3to12_3_ping, b12_3to13_3_ping, b13_3to14_3_ping, b14_3to15_3_ping, b15_3to16_3_ping;
wire [`DWIDTH-1:0] b0_4to1_4_ping, b1_4to2_4_ping, b2_4to3_4_ping, b3_4to4_4_ping, b4_4to5_4_ping, b5_4to6_4_ping, b6_4to7_4_ping, b7_4to8_4_ping, b8_4to9_4_ping, b9_4to10_4_ping, b10_4to11_4_ping, b11_4to12_4_ping, b12_4to13_4_ping, b13_4to14_4_ping, b14_4to15_4_ping, b15_4to16_4_ping;
wire [`DWIDTH-1:0] b0_5to1_5_ping, b1_5to2_5_ping, b2_5to3_5_ping, b3_5to4_5_ping, b4_5to5_5_ping, b5_5to6_5_ping, b6_5to7_5_ping, b7_5to8_5_ping, b8_5to9_5_ping, b9_5to10_5_ping, b10_5to11_5_ping, b11_5to12_5_ping, b12_5to13_5_ping, b13_5to14_5_ping, b14_5to15_5_ping, b15_5to16_5_ping;
wire [`DWIDTH-1:0] b0_6to1_6_ping, b1_6to2_6_ping, b2_6to3_6_ping, b3_6to4_6_ping, b4_6to5_6_ping, b5_6to6_6_ping, b6_6to7_6_ping, b7_6to8_6_ping, b8_6to9_6_ping, b9_6to10_6_ping, b10_6to11_6_ping, b11_6to12_6_ping, b12_6to13_6_ping, b13_6to14_6_ping, b14_6to15_6_ping, b15_6to16_6_ping;
wire [`DWIDTH-1:0] b0_7to1_7_ping, b1_7to2_7_ping, b2_7to3_7_ping, b3_7to4_7_ping, b4_7to5_7_ping, b5_7to6_7_ping, b6_7to7_7_ping, b7_7to8_7_ping, b8_7to9_7_ping, b9_7to10_7_ping, b10_7to11_7_ping, b11_7to12_7_ping, b12_7to13_7_ping, b13_7to14_7_ping, b14_7to15_7_ping, b15_7to16_7_ping;
wire [`DWIDTH-1:0] b0_8to1_8_ping, b1_8to2_8_ping, b2_8to3_8_ping, b3_8to4_8_ping, b4_8to5_8_ping, b5_8to6_8_ping, b6_8to7_8_ping, b7_8to8_8_ping, b8_8to9_8_ping, b9_8to10_8_ping, b10_8to11_8_ping, b11_8to12_8_ping, b12_8to13_8_ping, b13_8to14_8_ping, b14_8to15_8_ping, b15_8to16_8_ping;
wire [`DWIDTH-1:0] b0_9to1_9_ping, b1_9to2_9_ping, b2_9to3_9_ping, b3_9to4_9_ping, b4_9to5_9_ping, b5_9to6_9_ping, b6_9to7_9_ping, b7_9to8_9_ping, b8_9to9_9_ping, b9_9to10_9_ping, b10_9to11_9_ping, b11_9to12_9_ping, b12_9to13_9_ping, b13_9to14_9_ping, b14_9to15_9_ping, b15_9to16_9_ping;
wire [`DWIDTH-1:0] b0_10to1_10_ping, b1_10to2_10_ping, b2_10to3_10_ping, b3_10to4_10_ping, b4_10to5_10_ping, b5_10to6_10_ping, b6_10to7_10_ping, b7_10to8_10_ping, b8_10to9_10_ping, b9_10to10_10_ping, b10_10to11_10_ping, b11_10to12_10_ping, b12_10to13_10_ping, b13_10to14_10_ping, b14_10to15_10_ping, b15_10to16_10_ping;
wire [`DWIDTH-1:0] b0_11to1_11_ping, b1_11to2_11_ping, b2_11to3_11_ping, b3_11to4_11_ping, b4_11to5_11_ping, b5_11to6_11_ping, b6_11to7_11_ping, b7_11to8_11_ping, b8_11to9_11_ping, b9_11to10_11_ping, b10_11to11_11_ping, b11_11to12_11_ping, b12_11to13_11_ping, b13_11to14_11_ping, b14_11to15_11_ping, b15_11to16_11_ping;
wire [`DWIDTH-1:0] b0_12to1_12_ping, b1_12to2_12_ping, b2_12to3_12_ping, b3_12to4_12_ping, b4_12to5_12_ping, b5_12to6_12_ping, b6_12to7_12_ping, b7_12to8_12_ping, b8_12to9_12_ping, b9_12to10_12_ping, b10_12to11_12_ping, b11_12to12_12_ping, b12_12to13_12_ping, b13_12to14_12_ping, b14_12to15_12_ping, b15_12to16_12_ping;
wire [`DWIDTH-1:0] b0_13to1_13_ping, b1_13to2_13_ping, b2_13to3_13_ping, b3_13to4_13_ping, b4_13to5_13_ping, b5_13to6_13_ping, b6_13to7_13_ping, b7_13to8_13_ping, b8_13to9_13_ping, b9_13to10_13_ping, b10_13to11_13_ping, b11_13to12_13_ping, b12_13to13_13_ping, b13_13to14_13_ping, b14_13to15_13_ping, b15_13to16_13_ping;
wire [`DWIDTH-1:0] b0_14to1_14_ping, b1_14to2_14_ping, b2_14to3_14_ping, b3_14to4_14_ping, b4_14to5_14_ping, b5_14to6_14_ping, b6_14to7_14_ping, b7_14to8_14_ping, b8_14to9_14_ping, b9_14to10_14_ping, b10_14to11_14_ping, b11_14to12_14_ping, b12_14to13_14_ping, b13_14to14_14_ping, b14_14to15_14_ping, b15_14to16_14_ping;
wire [`DWIDTH-1:0] b0_15to1_15_ping, b1_15to2_15_ping, b2_15to3_15_ping, b3_15to4_15_ping, b4_15to5_15_ping, b5_15to6_15_ping, b6_15to7_15_ping, b7_15to8_15_ping, b8_15to9_15_ping, b9_15to10_15_ping, b10_15to11_15_ping, b11_15to12_15_ping, b12_15to13_15_ping, b13_15to14_15_ping, b14_15to15_15_ping, b15_15to16_15_ping;

wire [`DWIDTH-1:0] b0_0to1_0_pong, b1_0to2_0_pong, b2_0to3_0_pong, b3_0to4_0_pong, b4_0to5_0_pong, b5_0to6_0_pong, b6_0to7_0_pong, b7_0to8_0_pong, b8_0to9_0_pong, b9_0to10_0_pong, b10_0to11_0_pong, b11_0to12_0_pong, b12_0to13_0_pong, b13_0to14_0_pong, b14_0to15_0_pong, b15_0to16_0_pong;
wire [`DWIDTH-1:0] b0_1to1_1_pong, b1_1to2_1_pong, b2_1to3_1_pong, b3_1to4_1_pong, b4_1to5_1_pong, b5_1to6_1_pong, b6_1to7_1_pong, b7_1to8_1_pong, b8_1to9_1_pong, b9_1to10_1_pong, b10_1to11_1_pong, b11_1to12_1_pong, b12_1to13_1_pong, b13_1to14_1_pong, b14_1to15_1_pong, b15_1to16_1_pong;
wire [`DWIDTH-1:0] b0_2to1_2_pong, b1_2to2_2_pong, b2_2to3_2_pong, b3_2to4_2_pong, b4_2to5_2_pong, b5_2to6_2_pong, b6_2to7_2_pong, b7_2to8_2_pong, b8_2to9_2_pong, b9_2to10_2_pong, b10_2to11_2_pong, b11_2to12_2_pong, b12_2to13_2_pong, b13_2to14_2_pong, b14_2to15_2_pong, b15_2to16_2_pong;
wire [`DWIDTH-1:0] b0_3to1_3_pong, b1_3to2_3_pong, b2_3to3_3_pong, b3_3to4_3_pong, b4_3to5_3_pong, b5_3to6_3_pong, b6_3to7_3_pong, b7_3to8_3_pong, b8_3to9_3_pong, b9_3to10_3_pong, b10_3to11_3_pong, b11_3to12_3_pong, b12_3to13_3_pong, b13_3to14_3_pong, b14_3to15_3_pong, b15_3to16_3_pong;
wire [`DWIDTH-1:0] b0_4to1_4_pong, b1_4to2_4_pong, b2_4to3_4_pong, b3_4to4_4_pong, b4_4to5_4_pong, b5_4to6_4_pong, b6_4to7_4_pong, b7_4to8_4_pong, b8_4to9_4_pong, b9_4to10_4_pong, b10_4to11_4_pong, b11_4to12_4_pong, b12_4to13_4_pong, b13_4to14_4_pong, b14_4to15_4_pong, b15_4to16_4_pong;
wire [`DWIDTH-1:0] b0_5to1_5_pong, b1_5to2_5_pong, b2_5to3_5_pong, b3_5to4_5_pong, b4_5to5_5_pong, b5_5to6_5_pong, b6_5to7_5_pong, b7_5to8_5_pong, b8_5to9_5_pong, b9_5to10_5_pong, b10_5to11_5_pong, b11_5to12_5_pong, b12_5to13_5_pong, b13_5to14_5_pong, b14_5to15_5_pong, b15_5to16_5_pong;
wire [`DWIDTH-1:0] b0_6to1_6_pong, b1_6to2_6_pong, b2_6to3_6_pong, b3_6to4_6_pong, b4_6to5_6_pong, b5_6to6_6_pong, b6_6to7_6_pong, b7_6to8_6_pong, b8_6to9_6_pong, b9_6to10_6_pong, b10_6to11_6_pong, b11_6to12_6_pong, b12_6to13_6_pong, b13_6to14_6_pong, b14_6to15_6_pong, b15_6to16_6_pong;
wire [`DWIDTH-1:0] b0_7to1_7_pong, b1_7to2_7_pong, b2_7to3_7_pong, b3_7to4_7_pong, b4_7to5_7_pong, b5_7to6_7_pong, b6_7to7_7_pong, b7_7to8_7_pong, b8_7to9_7_pong, b9_7to10_7_pong, b10_7to11_7_pong, b11_7to12_7_pong, b12_7to13_7_pong, b13_7to14_7_pong, b14_7to15_7_pong, b15_7to16_7_pong;
wire [`DWIDTH-1:0] b0_8to1_8_pong, b1_8to2_8_pong, b2_8to3_8_pong, b3_8to4_8_pong, b4_8to5_8_pong, b5_8to6_8_pong, b6_8to7_8_pong, b7_8to8_8_pong, b8_8to9_8_pong, b9_8to10_8_pong, b10_8to11_8_pong, b11_8to12_8_pong, b12_8to13_8_pong, b13_8to14_8_pong, b14_8to15_8_pong, b15_8to16_8_pong;
wire [`DWIDTH-1:0] b0_9to1_9_pong, b1_9to2_9_pong, b2_9to3_9_pong, b3_9to4_9_pong, b4_9to5_9_pong, b5_9to6_9_pong, b6_9to7_9_pong, b7_9to8_9_pong, b8_9to9_9_pong, b9_9to10_9_pong, b10_9to11_9_pong, b11_9to12_9_pong, b12_9to13_9_pong, b13_9to14_9_pong, b14_9to15_9_pong, b15_9to16_9_pong;
wire [`DWIDTH-1:0] b0_10to1_10_pong, b1_10to2_10_pong, b2_10to3_10_pong, b3_10to4_10_pong, b4_10to5_10_pong, b5_10to6_10_pong, b6_10to7_10_pong, b7_10to8_10_pong, b8_10to9_10_pong, b9_10to10_10_pong, b10_10to11_10_pong, b11_10to12_10_pong, b12_10to13_10_pong, b13_10to14_10_pong, b14_10to15_10_pong, b15_10to16_10_pong;
wire [`DWIDTH-1:0] b0_11to1_11_pong, b1_11to2_11_pong, b2_11to3_11_pong, b3_11to4_11_pong, b4_11to5_11_pong, b5_11to6_11_pong, b6_11to7_11_pong, b7_11to8_11_pong, b8_11to9_11_pong, b9_11to10_11_pong, b10_11to11_11_pong, b11_11to12_11_pong, b12_11to13_11_pong, b13_11to14_11_pong, b14_11to15_11_pong, b15_11to16_11_pong;
wire [`DWIDTH-1:0] b0_12to1_12_pong, b1_12to2_12_pong, b2_12to3_12_pong, b3_12to4_12_pong, b4_12to5_12_pong, b5_12to6_12_pong, b6_12to7_12_pong, b7_12to8_12_pong, b8_12to9_12_pong, b9_12to10_12_pong, b10_12to11_12_pong, b11_12to12_12_pong, b12_12to13_12_pong, b13_12to14_12_pong, b14_12to15_12_pong, b15_12to16_12_pong;
wire [`DWIDTH-1:0] b0_13to1_13_pong, b1_13to2_13_pong, b2_13to3_13_pong, b3_13to4_13_pong, b4_13to5_13_pong, b5_13to6_13_pong, b6_13to7_13_pong, b7_13to8_13_pong, b8_13to9_13_pong, b9_13to10_13_pong, b10_13to11_13_pong, b11_13to12_13_pong, b12_13to13_13_pong, b13_13to14_13_pong, b14_13to15_13_pong, b15_13to16_13_pong;
wire [`DWIDTH-1:0] b0_14to1_14_pong, b1_14to2_14_pong, b2_14to3_14_pong, b3_14to4_14_pong, b4_14to5_14_pong, b5_14to6_14_pong, b6_14to7_14_pong, b7_14to8_14_pong, b8_14to9_14_pong, b9_14to10_14_pong, b10_14to11_14_pong, b11_14to12_14_pong, b12_14to13_14_pong, b13_14to14_14_pong, b14_14to15_14_pong, b15_14to16_14_pong;
wire [`DWIDTH-1:0] b0_15to1_15_pong, b1_15to2_15_pong, b2_15to3_15_pong, b3_15to4_15_pong, b4_15to5_15_pong, b5_15to6_15_pong, b6_15to7_15_pong, b7_15to8_15_pong, b8_15to9_15_pong, b9_15to10_15_pong, b10_15to11_15_pong, b11_15to12_15_pong, b12_15to13_15_pong, b13_15to14_15_pong, b14_15to15_15_pong, b15_15to16_15_pong;

reg [`DWIDTH-1:0] b0_data, b1_data, b2_data, b3_data, b4_data, b5_data, b6_data, b7_data, b8_data, b9_data, b10_data, b11_data, b12_data, b13_data, b14_data, b15_data; 

wire effective_rst;
assign effective_rst = reset | pe_reset;

reg b_data_sel_delay1;
reg b_data_sel_delay2;
reg b_data_sel_delay3;
reg b_data_sel_delay4;
reg b_data_sel_delay5;
reg b_data_sel_delay6;
reg b_data_sel_delay7;
reg b_data_sel_delay8;
reg b_data_sel_delay9;
reg b_data_sel_delay10;
reg b_data_sel_delay11;
reg b_data_sel_delay12;
reg b_data_sel_delay13;
reg b_data_sel_delay14;
reg b_data_sel_delay15;
reg b_data_sel_delay16;
reg b_data_sel_delay17;
reg b_data_sel_delay18;
reg b_data_sel_delay19;
reg b_data_sel_delay20;
reg b_data_sel_delay21;
reg b_data_sel_delay22;
reg b_data_sel_delay23;
reg b_data_sel_delay24;
reg b_data_sel_delay25;
reg b_data_sel_delay26;
reg b_data_sel_delay27;
reg b_data_sel_delay28;
reg b_data_sel_delay29;
reg b_data_sel_delay30;

always @ (posedge clk) begin
    if (reset) begin
        b_data_sel_delay1 <= 0;
        b_data_sel_delay2 <= 0;
        b_data_sel_delay3 <= 0;
        b_data_sel_delay4 <= 0;
        b_data_sel_delay5 <= 0;
        b_data_sel_delay6 <= 0;
        b_data_sel_delay7 <= 0;
        b_data_sel_delay8 <= 0;
        b_data_sel_delay9 <= 0;
        b_data_sel_delay10 <= 0;
        b_data_sel_delay11 <= 0;
        b_data_sel_delay12 <= 0;
        b_data_sel_delay13 <= 0;
        b_data_sel_delay14 <= 0;
        b_data_sel_delay15 <= 0;
        b_data_sel_delay16 <= 0;
        b_data_sel_delay17 <= 0;
        b_data_sel_delay18 <= 0;
        b_data_sel_delay19 <= 0;
        b_data_sel_delay20 <= 0;
        b_data_sel_delay21 <= 0;
        b_data_sel_delay22 <= 0;
        b_data_sel_delay23 <= 0;
        b_data_sel_delay24 <= 0;
        b_data_sel_delay25 <= 0;
        b_data_sel_delay26 <= 0;
        b_data_sel_delay27 <= 0;
        b_data_sel_delay28 <= 0;
        b_data_sel_delay29 <= 0;
        b_data_sel_delay30 <= 0;
    end
    else begin
        b_data_sel_delay1 <= b_data_sel;
        b_data_sel_delay2 <= b_data_sel_delay1;
        b_data_sel_delay3 <= b_data_sel_delay2;
        b_data_sel_delay4 <= b_data_sel_delay3;
        b_data_sel_delay5 <= b_data_sel_delay4;
        b_data_sel_delay6 <= b_data_sel_delay5;
        b_data_sel_delay7 <= b_data_sel_delay6;
        b_data_sel_delay8 <= b_data_sel_delay7;
        b_data_sel_delay9 <= b_data_sel_delay8;
        b_data_sel_delay10 <= b_data_sel_delay9;
        b_data_sel_delay11 <= b_data_sel_delay10;
        b_data_sel_delay12 <= b_data_sel_delay11;
        b_data_sel_delay13 <= b_data_sel_delay12;
        b_data_sel_delay14 <= b_data_sel_delay13;
        b_data_sel_delay15 <= b_data_sel_delay14;
        b_data_sel_delay16 <= b_data_sel_delay15;
        b_data_sel_delay17 <= b_data_sel_delay16;
        b_data_sel_delay18 <= b_data_sel_delay17;
        b_data_sel_delay19 <= b_data_sel_delay18;
        b_data_sel_delay20 <= b_data_sel_delay19;
        b_data_sel_delay21 <= b_data_sel_delay20;
        b_data_sel_delay22 <= b_data_sel_delay21;
        b_data_sel_delay23 <= b_data_sel_delay22;
        b_data_sel_delay24 <= b_data_sel_delay23;
        b_data_sel_delay25 <= b_data_sel_delay24;
        b_data_sel_delay26 <= b_data_sel_delay25;
        b_data_sel_delay27 <= b_data_sel_delay26;
        b_data_sel_delay28 <= b_data_sel_delay27;
        b_data_sel_delay29 <= b_data_sel_delay28;
        b_data_sel_delay30 <= b_data_sel_delay29;
  	end
end

// Signals for Each PONG buffer

reg b_data_valid_pong_delay0_1;
reg b_data_valid_pong_delay0_2;
reg b_data_valid_pong_delay0_3;
reg b_data_valid_pong_delay0_4;
reg b_data_valid_pong_delay0_5;
reg b_data_valid_pong_delay0_6;
reg b_data_valid_pong_delay0_7;
reg b_data_valid_pong_delay0_8;
reg b_data_valid_pong_delay0_9;
reg b_data_valid_pong_delay0_10;
reg b_data_valid_pong_delay0_11;
reg b_data_valid_pong_delay0_12;
reg b_data_valid_pong_delay0_13;
reg b_data_valid_pong_delay0_14;
reg b_data_valid_pong_delay0_15;
reg b_data_valid_pong_delay0_16;
reg b_data_valid_pong_delay0_17;
reg b_data_valid_pong_delay0_18;
reg b_data_valid_pong_delay0_19;
reg b_data_valid_pong_delay0_20;
reg b_data_valid_pong_delay0_21;
reg b_data_valid_pong_delay0_22;
reg b_data_valid_pong_delay0_23;
reg b_data_valid_pong_delay0_24;
reg b_data_valid_pong_delay0_25;
reg b_data_valid_pong_delay0_26;
reg b_data_valid_pong_delay0_27;
reg b_data_valid_pong_delay0_28;
reg b_data_valid_pong_delay0_29;
reg b_data_valid_pong_delay0_30;
wire b_data_valid_pong_delay1_0;
wire b_data_valid_pong_delay2_0;
wire b_data_valid_pong_delay3_0;
wire b_data_valid_pong_delay4_0;
wire b_data_valid_pong_delay5_0;
wire b_data_valid_pong_delay6_0;
wire b_data_valid_pong_delay7_0;
wire b_data_valid_pong_delay8_0;
wire b_data_valid_pong_delay9_0;
wire b_data_valid_pong_delay10_0;
wire b_data_valid_pong_delay11_0;
wire b_data_valid_pong_delay12_0;
wire b_data_valid_pong_delay13_0;
wire b_data_valid_pong_delay14_0;
wire b_data_valid_pong_delay15_0;
wire b_data_valid_pong_delay1_1;
wire b_data_valid_pong_delay2_1;
wire b_data_valid_pong_delay3_1;
wire b_data_valid_pong_delay4_1;
wire b_data_valid_pong_delay5_1;
wire b_data_valid_pong_delay6_1;
wire b_data_valid_pong_delay7_1;
wire b_data_valid_pong_delay8_1;
wire b_data_valid_pong_delay9_1;
wire b_data_valid_pong_delay10_1;
wire b_data_valid_pong_delay11_1;
wire b_data_valid_pong_delay12_1;
wire b_data_valid_pong_delay13_1;
wire b_data_valid_pong_delay14_1;
wire b_data_valid_pong_delay15_1;
wire b_data_valid_pong_delay1_2;
wire b_data_valid_pong_delay2_2;
wire b_data_valid_pong_delay3_2;
wire b_data_valid_pong_delay4_2;
wire b_data_valid_pong_delay5_2;
wire b_data_valid_pong_delay6_2;
wire b_data_valid_pong_delay7_2;
wire b_data_valid_pong_delay8_2;
wire b_data_valid_pong_delay9_2;
wire b_data_valid_pong_delay10_2;
wire b_data_valid_pong_delay11_2;
wire b_data_valid_pong_delay12_2;
wire b_data_valid_pong_delay13_2;
wire b_data_valid_pong_delay14_2;
wire b_data_valid_pong_delay15_2;
wire b_data_valid_pong_delay1_3;
wire b_data_valid_pong_delay2_3;
wire b_data_valid_pong_delay3_3;
wire b_data_valid_pong_delay4_3;
wire b_data_valid_pong_delay5_3;
wire b_data_valid_pong_delay6_3;
wire b_data_valid_pong_delay7_3;
wire b_data_valid_pong_delay8_3;
wire b_data_valid_pong_delay9_3;
wire b_data_valid_pong_delay10_3;
wire b_data_valid_pong_delay11_3;
wire b_data_valid_pong_delay12_3;
wire b_data_valid_pong_delay13_3;
wire b_data_valid_pong_delay14_3;
wire b_data_valid_pong_delay15_3;
wire b_data_valid_pong_delay1_4;
wire b_data_valid_pong_delay2_4;
wire b_data_valid_pong_delay3_4;
wire b_data_valid_pong_delay4_4;
wire b_data_valid_pong_delay5_4;
wire b_data_valid_pong_delay6_4;
wire b_data_valid_pong_delay7_4;
wire b_data_valid_pong_delay8_4;
wire b_data_valid_pong_delay9_4;
wire b_data_valid_pong_delay10_4;
wire b_data_valid_pong_delay11_4;
wire b_data_valid_pong_delay12_4;
wire b_data_valid_pong_delay13_4;
wire b_data_valid_pong_delay14_4;
wire b_data_valid_pong_delay15_4;
wire b_data_valid_pong_delay1_5;
wire b_data_valid_pong_delay2_5;
wire b_data_valid_pong_delay3_5;
wire b_data_valid_pong_delay4_5;
wire b_data_valid_pong_delay5_5;
wire b_data_valid_pong_delay6_5;
wire b_data_valid_pong_delay7_5;
wire b_data_valid_pong_delay8_5;
wire b_data_valid_pong_delay9_5;
wire b_data_valid_pong_delay10_5;
wire b_data_valid_pong_delay11_5;
wire b_data_valid_pong_delay12_5;
wire b_data_valid_pong_delay13_5;
wire b_data_valid_pong_delay14_5;
wire b_data_valid_pong_delay15_5;
wire b_data_valid_pong_delay1_6;
wire b_data_valid_pong_delay2_6;
wire b_data_valid_pong_delay3_6;
wire b_data_valid_pong_delay4_6;
wire b_data_valid_pong_delay5_6;
wire b_data_valid_pong_delay6_6;
wire b_data_valid_pong_delay7_6;
wire b_data_valid_pong_delay8_6;
wire b_data_valid_pong_delay9_6;
wire b_data_valid_pong_delay10_6;
wire b_data_valid_pong_delay11_6;
wire b_data_valid_pong_delay12_6;
wire b_data_valid_pong_delay13_6;
wire b_data_valid_pong_delay14_6;
wire b_data_valid_pong_delay15_6;
wire b_data_valid_pong_delay1_7;
wire b_data_valid_pong_delay2_7;
wire b_data_valid_pong_delay3_7;
wire b_data_valid_pong_delay4_7;
wire b_data_valid_pong_delay5_7;
wire b_data_valid_pong_delay6_7;
wire b_data_valid_pong_delay7_7;
wire b_data_valid_pong_delay8_7;
wire b_data_valid_pong_delay9_7;
wire b_data_valid_pong_delay10_7;
wire b_data_valid_pong_delay11_7;
wire b_data_valid_pong_delay12_7;
wire b_data_valid_pong_delay13_7;
wire b_data_valid_pong_delay14_7;
wire b_data_valid_pong_delay15_7;
wire b_data_valid_pong_delay1_8;
wire b_data_valid_pong_delay2_8;
wire b_data_valid_pong_delay3_8;
wire b_data_valid_pong_delay4_8;
wire b_data_valid_pong_delay5_8;
wire b_data_valid_pong_delay6_8;
wire b_data_valid_pong_delay7_8;
wire b_data_valid_pong_delay8_8;
wire b_data_valid_pong_delay9_8;
wire b_data_valid_pong_delay10_8;
wire b_data_valid_pong_delay11_8;
wire b_data_valid_pong_delay12_8;
wire b_data_valid_pong_delay13_8;
wire b_data_valid_pong_delay14_8;
wire b_data_valid_pong_delay15_8;
wire b_data_valid_pong_delay1_9;
wire b_data_valid_pong_delay2_9;
wire b_data_valid_pong_delay3_9;
wire b_data_valid_pong_delay4_9;
wire b_data_valid_pong_delay5_9;
wire b_data_valid_pong_delay6_9;
wire b_data_valid_pong_delay7_9;
wire b_data_valid_pong_delay8_9;
wire b_data_valid_pong_delay9_9;
wire b_data_valid_pong_delay10_9;
wire b_data_valid_pong_delay11_9;
wire b_data_valid_pong_delay12_9;
wire b_data_valid_pong_delay13_9;
wire b_data_valid_pong_delay14_9;
wire b_data_valid_pong_delay15_9;
wire b_data_valid_pong_delay1_10;
wire b_data_valid_pong_delay2_10;
wire b_data_valid_pong_delay3_10;
wire b_data_valid_pong_delay4_10;
wire b_data_valid_pong_delay5_10;
wire b_data_valid_pong_delay6_10;
wire b_data_valid_pong_delay7_10;
wire b_data_valid_pong_delay8_10;
wire b_data_valid_pong_delay9_10;
wire b_data_valid_pong_delay10_10;
wire b_data_valid_pong_delay11_10;
wire b_data_valid_pong_delay12_10;
wire b_data_valid_pong_delay13_10;
wire b_data_valid_pong_delay14_10;
wire b_data_valid_pong_delay15_10;
wire b_data_valid_pong_delay1_11;
wire b_data_valid_pong_delay2_11;
wire b_data_valid_pong_delay3_11;
wire b_data_valid_pong_delay4_11;
wire b_data_valid_pong_delay5_11;
wire b_data_valid_pong_delay6_11;
wire b_data_valid_pong_delay7_11;
wire b_data_valid_pong_delay8_11;
wire b_data_valid_pong_delay9_11;
wire b_data_valid_pong_delay10_11;
wire b_data_valid_pong_delay11_11;
wire b_data_valid_pong_delay12_11;
wire b_data_valid_pong_delay13_11;
wire b_data_valid_pong_delay14_11;
wire b_data_valid_pong_delay15_11;
wire b_data_valid_pong_delay1_12;
wire b_data_valid_pong_delay2_12;
wire b_data_valid_pong_delay3_12;
wire b_data_valid_pong_delay4_12;
wire b_data_valid_pong_delay5_12;
wire b_data_valid_pong_delay6_12;
wire b_data_valid_pong_delay7_12;
wire b_data_valid_pong_delay8_12;
wire b_data_valid_pong_delay9_12;
wire b_data_valid_pong_delay10_12;
wire b_data_valid_pong_delay11_12;
wire b_data_valid_pong_delay12_12;
wire b_data_valid_pong_delay13_12;
wire b_data_valid_pong_delay14_12;
wire b_data_valid_pong_delay15_12;
wire b_data_valid_pong_delay1_13;
wire b_data_valid_pong_delay2_13;
wire b_data_valid_pong_delay3_13;
wire b_data_valid_pong_delay4_13;
wire b_data_valid_pong_delay5_13;
wire b_data_valid_pong_delay6_13;
wire b_data_valid_pong_delay7_13;
wire b_data_valid_pong_delay8_13;
wire b_data_valid_pong_delay9_13;
wire b_data_valid_pong_delay10_13;
wire b_data_valid_pong_delay11_13;
wire b_data_valid_pong_delay12_13;
wire b_data_valid_pong_delay13_13;
wire b_data_valid_pong_delay14_13;
wire b_data_valid_pong_delay15_13;
wire b_data_valid_pong_delay1_14;
wire b_data_valid_pong_delay2_14;
wire b_data_valid_pong_delay3_14;
wire b_data_valid_pong_delay4_14;
wire b_data_valid_pong_delay5_14;
wire b_data_valid_pong_delay6_14;
wire b_data_valid_pong_delay7_14;
wire b_data_valid_pong_delay8_14;
wire b_data_valid_pong_delay9_14;
wire b_data_valid_pong_delay10_14;
wire b_data_valid_pong_delay11_14;
wire b_data_valid_pong_delay12_14;
wire b_data_valid_pong_delay13_14;
wire b_data_valid_pong_delay14_14;
wire b_data_valid_pong_delay15_14;
wire b_data_valid_pong_delay1_15;
wire b_data_valid_pong_delay2_15;
wire b_data_valid_pong_delay3_15;
wire b_data_valid_pong_delay4_15;
wire b_data_valid_pong_delay5_15;
wire b_data_valid_pong_delay6_15;
wire b_data_valid_pong_delay7_15;
wire b_data_valid_pong_delay8_15;
wire b_data_valid_pong_delay9_15;
wire b_data_valid_pong_delay10_15;
wire b_data_valid_pong_delay11_15;
wire b_data_valid_pong_delay12_15;
wire b_data_valid_pong_delay13_15;
wire b_data_valid_pong_delay14_15;
wire b_data_valid_pong_delay15_15;
  
always @ (posedge clk) begin
    b_data_valid_pong_delay0_1 <= b_data_valid_pong;
    b_data_valid_pong_delay0_2 <= b_data_valid_pong_delay0_1;
    b_data_valid_pong_delay0_3 <= b_data_valid_pong_delay0_2;
    b_data_valid_pong_delay0_4 <= b_data_valid_pong_delay0_3;
    b_data_valid_pong_delay0_5 <= b_data_valid_pong_delay0_4;
    b_data_valid_pong_delay0_6 <= b_data_valid_pong_delay0_5;
    b_data_valid_pong_delay0_7 <= b_data_valid_pong_delay0_6;
    b_data_valid_pong_delay0_8 <= b_data_valid_pong_delay0_7;
    b_data_valid_pong_delay0_9 <= b_data_valid_pong_delay0_8;
    b_data_valid_pong_delay0_10 <= b_data_valid_pong_delay0_9;
    b_data_valid_pong_delay0_11 <= b_data_valid_pong_delay0_10;
    b_data_valid_pong_delay0_12 <= b_data_valid_pong_delay0_11;
    b_data_valid_pong_delay0_13 <= b_data_valid_pong_delay0_12;
    b_data_valid_pong_delay0_14 <= b_data_valid_pong_delay0_13;
    b_data_valid_pong_delay0_15 <= b_data_valid_pong_delay0_14;
    b_data_valid_pong_delay0_16 <= b_data_valid_pong_delay0_15;
    b_data_valid_pong_delay0_17 <= b_data_valid_pong_delay0_16;
    b_data_valid_pong_delay0_18 <= b_data_valid_pong_delay0_17;
    b_data_valid_pong_delay0_19 <= b_data_valid_pong_delay0_18;
    b_data_valid_pong_delay0_20 <= b_data_valid_pong_delay0_19;
    b_data_valid_pong_delay0_21 <= b_data_valid_pong_delay0_20;
    b_data_valid_pong_delay0_22 <= b_data_valid_pong_delay0_21;
    b_data_valid_pong_delay0_23 <= b_data_valid_pong_delay0_22;
    b_data_valid_pong_delay0_24 <= b_data_valid_pong_delay0_23;
    b_data_valid_pong_delay0_25 <= b_data_valid_pong_delay0_24;
    b_data_valid_pong_delay0_26 <= b_data_valid_pong_delay0_25;
    b_data_valid_pong_delay0_27 <= b_data_valid_pong_delay0_26;
    b_data_valid_pong_delay0_28 <= b_data_valid_pong_delay0_27;
    b_data_valid_pong_delay0_29 <= b_data_valid_pong_delay0_28;
    b_data_valid_pong_delay0_30 <= b_data_valid_pong_delay0_29;
end

assign b_data_valid_pong_delay1_0 = b_data_valid_pong & b_data_valid_pong_delay0_1;
assign b_data_valid_pong_delay2_0 = b_data_valid_pong & b_data_valid_pong_delay0_2;
assign b_data_valid_pong_delay3_0 = b_data_valid_pong & b_data_valid_pong_delay0_3;
assign b_data_valid_pong_delay4_0 = b_data_valid_pong & b_data_valid_pong_delay0_4;
assign b_data_valid_pong_delay5_0 = b_data_valid_pong & b_data_valid_pong_delay0_5;
assign b_data_valid_pong_delay6_0 = b_data_valid_pong & b_data_valid_pong_delay0_6;
assign b_data_valid_pong_delay7_0 = b_data_valid_pong & b_data_valid_pong_delay0_7;
assign b_data_valid_pong_delay8_0 = b_data_valid_pong & b_data_valid_pong_delay0_8;
assign b_data_valid_pong_delay9_0 = b_data_valid_pong & b_data_valid_pong_delay0_9;
assign b_data_valid_pong_delay10_0 = b_data_valid_pong & b_data_valid_pong_delay0_10;
assign b_data_valid_pong_delay11_0 = b_data_valid_pong & b_data_valid_pong_delay0_11;
assign b_data_valid_pong_delay12_0 = b_data_valid_pong & b_data_valid_pong_delay0_12;
assign b_data_valid_pong_delay13_0 = b_data_valid_pong & b_data_valid_pong_delay0_13;
assign b_data_valid_pong_delay14_0 = b_data_valid_pong & b_data_valid_pong_delay0_14;
assign b_data_valid_pong_delay15_0 = b_data_valid_pong & b_data_valid_pong_delay0_15;
assign b_data_valid_pong_delay1_1 = b_data_valid_pong_delay0_1 & b_data_valid_pong_delay0_2;
assign b_data_valid_pong_delay2_1 = b_data_valid_pong_delay0_1 & b_data_valid_pong_delay0_3;
assign b_data_valid_pong_delay3_1 = b_data_valid_pong_delay0_1 & b_data_valid_pong_delay0_4;
assign b_data_valid_pong_delay4_1 = b_data_valid_pong_delay0_1 & b_data_valid_pong_delay0_5;
assign b_data_valid_pong_delay5_1 = b_data_valid_pong_delay0_1 & b_data_valid_pong_delay0_6;
assign b_data_valid_pong_delay6_1 = b_data_valid_pong_delay0_1 & b_data_valid_pong_delay0_7;
assign b_data_valid_pong_delay7_1 = b_data_valid_pong_delay0_1 & b_data_valid_pong_delay0_8;
assign b_data_valid_pong_delay8_1 = b_data_valid_pong_delay0_1 & b_data_valid_pong_delay0_9;
assign b_data_valid_pong_delay9_1 = b_data_valid_pong_delay0_1 & b_data_valid_pong_delay0_10;
assign b_data_valid_pong_delay10_1 = b_data_valid_pong_delay0_1 & b_data_valid_pong_delay0_11;
assign b_data_valid_pong_delay11_1 = b_data_valid_pong_delay0_1 & b_data_valid_pong_delay0_12;
assign b_data_valid_pong_delay12_1 = b_data_valid_pong_delay0_1 & b_data_valid_pong_delay0_13;
assign b_data_valid_pong_delay13_1 = b_data_valid_pong_delay0_1 & b_data_valid_pong_delay0_14;
assign b_data_valid_pong_delay14_1 = b_data_valid_pong_delay0_1 & b_data_valid_pong_delay0_15;
assign b_data_valid_pong_delay15_1 = b_data_valid_pong_delay0_1 & b_data_valid_pong_delay0_16;
assign b_data_valid_pong_delay1_2 = b_data_valid_pong_delay0_2 & b_data_valid_pong_delay0_3;
assign b_data_valid_pong_delay2_2 = b_data_valid_pong_delay0_2 & b_data_valid_pong_delay0_4;
assign b_data_valid_pong_delay3_2 = b_data_valid_pong_delay0_2 & b_data_valid_pong_delay0_5;
assign b_data_valid_pong_delay4_2 = b_data_valid_pong_delay0_2 & b_data_valid_pong_delay0_6;
assign b_data_valid_pong_delay5_2 = b_data_valid_pong_delay0_2 & b_data_valid_pong_delay0_7;
assign b_data_valid_pong_delay6_2 = b_data_valid_pong_delay0_2 & b_data_valid_pong_delay0_8;
assign b_data_valid_pong_delay7_2 = b_data_valid_pong_delay0_2 & b_data_valid_pong_delay0_9;
assign b_data_valid_pong_delay8_2 = b_data_valid_pong_delay0_2 & b_data_valid_pong_delay0_10;
assign b_data_valid_pong_delay9_2 = b_data_valid_pong_delay0_2 & b_data_valid_pong_delay0_11;
assign b_data_valid_pong_delay10_2 = b_data_valid_pong_delay0_2 & b_data_valid_pong_delay0_12;
assign b_data_valid_pong_delay11_2 = b_data_valid_pong_delay0_2 & b_data_valid_pong_delay0_13;
assign b_data_valid_pong_delay12_2 = b_data_valid_pong_delay0_2 & b_data_valid_pong_delay0_14;
assign b_data_valid_pong_delay13_2 = b_data_valid_pong_delay0_2 & b_data_valid_pong_delay0_15;
assign b_data_valid_pong_delay14_2 = b_data_valid_pong_delay0_2 & b_data_valid_pong_delay0_16;
assign b_data_valid_pong_delay15_2 = b_data_valid_pong_delay0_2 & b_data_valid_pong_delay0_17;
assign b_data_valid_pong_delay1_3 = b_data_valid_pong_delay0_3 & b_data_valid_pong_delay0_4;
assign b_data_valid_pong_delay2_3 = b_data_valid_pong_delay0_3 & b_data_valid_pong_delay0_5;
assign b_data_valid_pong_delay3_3 = b_data_valid_pong_delay0_3 & b_data_valid_pong_delay0_6;
assign b_data_valid_pong_delay4_3 = b_data_valid_pong_delay0_3 & b_data_valid_pong_delay0_7;
assign b_data_valid_pong_delay5_3 = b_data_valid_pong_delay0_3 & b_data_valid_pong_delay0_8;
assign b_data_valid_pong_delay6_3 = b_data_valid_pong_delay0_3 & b_data_valid_pong_delay0_9;
assign b_data_valid_pong_delay7_3 = b_data_valid_pong_delay0_3 & b_data_valid_pong_delay0_10;
assign b_data_valid_pong_delay8_3 = b_data_valid_pong_delay0_3 & b_data_valid_pong_delay0_11;
assign b_data_valid_pong_delay9_3 = b_data_valid_pong_delay0_3 & b_data_valid_pong_delay0_12;
assign b_data_valid_pong_delay10_3 = b_data_valid_pong_delay0_3 & b_data_valid_pong_delay0_13;
assign b_data_valid_pong_delay11_3 = b_data_valid_pong_delay0_3 & b_data_valid_pong_delay0_14;
assign b_data_valid_pong_delay12_3 = b_data_valid_pong_delay0_3 & b_data_valid_pong_delay0_15;
assign b_data_valid_pong_delay13_3 = b_data_valid_pong_delay0_3 & b_data_valid_pong_delay0_16;
assign b_data_valid_pong_delay14_3 = b_data_valid_pong_delay0_3 & b_data_valid_pong_delay0_17;
assign b_data_valid_pong_delay15_3 = b_data_valid_pong_delay0_3 & b_data_valid_pong_delay0_18;
assign b_data_valid_pong_delay1_4 = b_data_valid_pong_delay0_4 & b_data_valid_pong_delay0_5;
assign b_data_valid_pong_delay2_4 = b_data_valid_pong_delay0_4 & b_data_valid_pong_delay0_6;
assign b_data_valid_pong_delay3_4 = b_data_valid_pong_delay0_4 & b_data_valid_pong_delay0_7;
assign b_data_valid_pong_delay4_4 = b_data_valid_pong_delay0_4 & b_data_valid_pong_delay0_8;
assign b_data_valid_pong_delay5_4 = b_data_valid_pong_delay0_4 & b_data_valid_pong_delay0_9;
assign b_data_valid_pong_delay6_4 = b_data_valid_pong_delay0_4 & b_data_valid_pong_delay0_10;
assign b_data_valid_pong_delay7_4 = b_data_valid_pong_delay0_4 & b_data_valid_pong_delay0_11;
assign b_data_valid_pong_delay8_4 = b_data_valid_pong_delay0_4 & b_data_valid_pong_delay0_12;
assign b_data_valid_pong_delay9_4 = b_data_valid_pong_delay0_4 & b_data_valid_pong_delay0_13;
assign b_data_valid_pong_delay10_4 = b_data_valid_pong_delay0_4 & b_data_valid_pong_delay0_14;
assign b_data_valid_pong_delay11_4 = b_data_valid_pong_delay0_4 & b_data_valid_pong_delay0_15;
assign b_data_valid_pong_delay12_4 = b_data_valid_pong_delay0_4 & b_data_valid_pong_delay0_16;
assign b_data_valid_pong_delay13_4 = b_data_valid_pong_delay0_4 & b_data_valid_pong_delay0_17;
assign b_data_valid_pong_delay14_4 = b_data_valid_pong_delay0_4 & b_data_valid_pong_delay0_18;
assign b_data_valid_pong_delay15_4 = b_data_valid_pong_delay0_4 & b_data_valid_pong_delay0_19;
assign b_data_valid_pong_delay1_5 = b_data_valid_pong_delay0_5 & b_data_valid_pong_delay0_6;
assign b_data_valid_pong_delay2_5 = b_data_valid_pong_delay0_5 & b_data_valid_pong_delay0_7;
assign b_data_valid_pong_delay3_5 = b_data_valid_pong_delay0_5 & b_data_valid_pong_delay0_8;
assign b_data_valid_pong_delay4_5 = b_data_valid_pong_delay0_5 & b_data_valid_pong_delay0_9;
assign b_data_valid_pong_delay5_5 = b_data_valid_pong_delay0_5 & b_data_valid_pong_delay0_10;
assign b_data_valid_pong_delay6_5 = b_data_valid_pong_delay0_5 & b_data_valid_pong_delay0_11;
assign b_data_valid_pong_delay7_5 = b_data_valid_pong_delay0_5 & b_data_valid_pong_delay0_12;
assign b_data_valid_pong_delay8_5 = b_data_valid_pong_delay0_5 & b_data_valid_pong_delay0_13;
assign b_data_valid_pong_delay9_5 = b_data_valid_pong_delay0_5 & b_data_valid_pong_delay0_14;
assign b_data_valid_pong_delay10_5 = b_data_valid_pong_delay0_5 & b_data_valid_pong_delay0_15;
assign b_data_valid_pong_delay11_5 = b_data_valid_pong_delay0_5 & b_data_valid_pong_delay0_16;
assign b_data_valid_pong_delay12_5 = b_data_valid_pong_delay0_5 & b_data_valid_pong_delay0_17;
assign b_data_valid_pong_delay13_5 = b_data_valid_pong_delay0_5 & b_data_valid_pong_delay0_18;
assign b_data_valid_pong_delay14_5 = b_data_valid_pong_delay0_5 & b_data_valid_pong_delay0_19;
assign b_data_valid_pong_delay15_5 = b_data_valid_pong_delay0_5 & b_data_valid_pong_delay0_20;
assign b_data_valid_pong_delay1_6 = b_data_valid_pong_delay0_6 & b_data_valid_pong_delay0_7;
assign b_data_valid_pong_delay2_6 = b_data_valid_pong_delay0_6 & b_data_valid_pong_delay0_8;
assign b_data_valid_pong_delay3_6 = b_data_valid_pong_delay0_6 & b_data_valid_pong_delay0_9;
assign b_data_valid_pong_delay4_6 = b_data_valid_pong_delay0_6 & b_data_valid_pong_delay0_10;
assign b_data_valid_pong_delay5_6 = b_data_valid_pong_delay0_6 & b_data_valid_pong_delay0_11;
assign b_data_valid_pong_delay6_6 = b_data_valid_pong_delay0_6 & b_data_valid_pong_delay0_12;
assign b_data_valid_pong_delay7_6 = b_data_valid_pong_delay0_6 & b_data_valid_pong_delay0_13;
assign b_data_valid_pong_delay8_6 = b_data_valid_pong_delay0_6 & b_data_valid_pong_delay0_14;
assign b_data_valid_pong_delay9_6 = b_data_valid_pong_delay0_6 & b_data_valid_pong_delay0_15;
assign b_data_valid_pong_delay10_6 = b_data_valid_pong_delay0_6 & b_data_valid_pong_delay0_16;
assign b_data_valid_pong_delay11_6 = b_data_valid_pong_delay0_6 & b_data_valid_pong_delay0_17;
assign b_data_valid_pong_delay12_6 = b_data_valid_pong_delay0_6 & b_data_valid_pong_delay0_18;
assign b_data_valid_pong_delay13_6 = b_data_valid_pong_delay0_6 & b_data_valid_pong_delay0_19;
assign b_data_valid_pong_delay14_6 = b_data_valid_pong_delay0_6 & b_data_valid_pong_delay0_20;
assign b_data_valid_pong_delay15_6 = b_data_valid_pong_delay0_6 & b_data_valid_pong_delay0_21;
assign b_data_valid_pong_delay1_7 = b_data_valid_pong_delay0_7 & b_data_valid_pong_delay0_8;
assign b_data_valid_pong_delay2_7 = b_data_valid_pong_delay0_7 & b_data_valid_pong_delay0_9;
assign b_data_valid_pong_delay3_7 = b_data_valid_pong_delay0_7 & b_data_valid_pong_delay0_10;
assign b_data_valid_pong_delay4_7 = b_data_valid_pong_delay0_7 & b_data_valid_pong_delay0_11;
assign b_data_valid_pong_delay5_7 = b_data_valid_pong_delay0_7 & b_data_valid_pong_delay0_12;
assign b_data_valid_pong_delay6_7 = b_data_valid_pong_delay0_7 & b_data_valid_pong_delay0_13;
assign b_data_valid_pong_delay7_7 = b_data_valid_pong_delay0_7 & b_data_valid_pong_delay0_14;
assign b_data_valid_pong_delay8_7 = b_data_valid_pong_delay0_7 & b_data_valid_pong_delay0_15;
assign b_data_valid_pong_delay9_7 = b_data_valid_pong_delay0_7 & b_data_valid_pong_delay0_16;
assign b_data_valid_pong_delay10_7 = b_data_valid_pong_delay0_7 & b_data_valid_pong_delay0_17;
assign b_data_valid_pong_delay11_7 = b_data_valid_pong_delay0_7 & b_data_valid_pong_delay0_18;
assign b_data_valid_pong_delay12_7 = b_data_valid_pong_delay0_7 & b_data_valid_pong_delay0_19;
assign b_data_valid_pong_delay13_7 = b_data_valid_pong_delay0_7 & b_data_valid_pong_delay0_20;
assign b_data_valid_pong_delay14_7 = b_data_valid_pong_delay0_7 & b_data_valid_pong_delay0_21;
assign b_data_valid_pong_delay15_7 = b_data_valid_pong_delay0_7 & b_data_valid_pong_delay0_22;
assign b_data_valid_pong_delay1_8 = b_data_valid_pong_delay0_8 & b_data_valid_pong_delay0_9;
assign b_data_valid_pong_delay2_8 = b_data_valid_pong_delay0_8 & b_data_valid_pong_delay0_10;
assign b_data_valid_pong_delay3_8 = b_data_valid_pong_delay0_8 & b_data_valid_pong_delay0_11;
assign b_data_valid_pong_delay4_8 = b_data_valid_pong_delay0_8 & b_data_valid_pong_delay0_12;
assign b_data_valid_pong_delay5_8 = b_data_valid_pong_delay0_8 & b_data_valid_pong_delay0_13;
assign b_data_valid_pong_delay6_8 = b_data_valid_pong_delay0_8 & b_data_valid_pong_delay0_14;
assign b_data_valid_pong_delay7_8 = b_data_valid_pong_delay0_8 & b_data_valid_pong_delay0_15;
assign b_data_valid_pong_delay8_8 = b_data_valid_pong_delay0_8 & b_data_valid_pong_delay0_16;
assign b_data_valid_pong_delay9_8 = b_data_valid_pong_delay0_8 & b_data_valid_pong_delay0_17;
assign b_data_valid_pong_delay10_8 = b_data_valid_pong_delay0_8 & b_data_valid_pong_delay0_18;
assign b_data_valid_pong_delay11_8 = b_data_valid_pong_delay0_8 & b_data_valid_pong_delay0_19;
assign b_data_valid_pong_delay12_8 = b_data_valid_pong_delay0_8 & b_data_valid_pong_delay0_20;
assign b_data_valid_pong_delay13_8 = b_data_valid_pong_delay0_8 & b_data_valid_pong_delay0_21;
assign b_data_valid_pong_delay14_8 = b_data_valid_pong_delay0_8 & b_data_valid_pong_delay0_22;
assign b_data_valid_pong_delay15_8 = b_data_valid_pong_delay0_8 & b_data_valid_pong_delay0_23;
assign b_data_valid_pong_delay1_9 = b_data_valid_pong_delay0_9 & b_data_valid_pong_delay0_10;
assign b_data_valid_pong_delay2_9 = b_data_valid_pong_delay0_9 & b_data_valid_pong_delay0_11;
assign b_data_valid_pong_delay3_9 = b_data_valid_pong_delay0_9 & b_data_valid_pong_delay0_12;
assign b_data_valid_pong_delay4_9 = b_data_valid_pong_delay0_9 & b_data_valid_pong_delay0_13;
assign b_data_valid_pong_delay5_9 = b_data_valid_pong_delay0_9 & b_data_valid_pong_delay0_14;
assign b_data_valid_pong_delay6_9 = b_data_valid_pong_delay0_9 & b_data_valid_pong_delay0_15;
assign b_data_valid_pong_delay7_9 = b_data_valid_pong_delay0_9 & b_data_valid_pong_delay0_16;
assign b_data_valid_pong_delay8_9 = b_data_valid_pong_delay0_9 & b_data_valid_pong_delay0_17;
assign b_data_valid_pong_delay9_9 = b_data_valid_pong_delay0_9 & b_data_valid_pong_delay0_18;
assign b_data_valid_pong_delay10_9 = b_data_valid_pong_delay0_9 & b_data_valid_pong_delay0_19;
assign b_data_valid_pong_delay11_9 = b_data_valid_pong_delay0_9 & b_data_valid_pong_delay0_20;
assign b_data_valid_pong_delay12_9 = b_data_valid_pong_delay0_9 & b_data_valid_pong_delay0_21;
assign b_data_valid_pong_delay13_9 = b_data_valid_pong_delay0_9 & b_data_valid_pong_delay0_22;
assign b_data_valid_pong_delay14_9 = b_data_valid_pong_delay0_9 & b_data_valid_pong_delay0_23;
assign b_data_valid_pong_delay15_9 = b_data_valid_pong_delay0_9 & b_data_valid_pong_delay0_24;
assign b_data_valid_pong_delay1_10 = b_data_valid_pong_delay0_10 & b_data_valid_pong_delay0_11;
assign b_data_valid_pong_delay2_10 = b_data_valid_pong_delay0_10 & b_data_valid_pong_delay0_12;
assign b_data_valid_pong_delay3_10 = b_data_valid_pong_delay0_10 & b_data_valid_pong_delay0_13;
assign b_data_valid_pong_delay4_10 = b_data_valid_pong_delay0_10 & b_data_valid_pong_delay0_14;
assign b_data_valid_pong_delay5_10 = b_data_valid_pong_delay0_10 & b_data_valid_pong_delay0_15;
assign b_data_valid_pong_delay6_10 = b_data_valid_pong_delay0_10 & b_data_valid_pong_delay0_16;
assign b_data_valid_pong_delay7_10 = b_data_valid_pong_delay0_10 & b_data_valid_pong_delay0_17;
assign b_data_valid_pong_delay8_10 = b_data_valid_pong_delay0_10 & b_data_valid_pong_delay0_18;
assign b_data_valid_pong_delay9_10 = b_data_valid_pong_delay0_10 & b_data_valid_pong_delay0_19;
assign b_data_valid_pong_delay10_10 = b_data_valid_pong_delay0_10 & b_data_valid_pong_delay0_20;
assign b_data_valid_pong_delay11_10 = b_data_valid_pong_delay0_10 & b_data_valid_pong_delay0_21;
assign b_data_valid_pong_delay12_10 = b_data_valid_pong_delay0_10 & b_data_valid_pong_delay0_22;
assign b_data_valid_pong_delay13_10 = b_data_valid_pong_delay0_10 & b_data_valid_pong_delay0_23;
assign b_data_valid_pong_delay14_10 = b_data_valid_pong_delay0_10 & b_data_valid_pong_delay0_24;
assign b_data_valid_pong_delay15_10 = b_data_valid_pong_delay0_10 & b_data_valid_pong_delay0_25;
assign b_data_valid_pong_delay1_11 = b_data_valid_pong_delay0_11 & b_data_valid_pong_delay0_12;
assign b_data_valid_pong_delay2_11 = b_data_valid_pong_delay0_11 & b_data_valid_pong_delay0_13;
assign b_data_valid_pong_delay3_11 = b_data_valid_pong_delay0_11 & b_data_valid_pong_delay0_14;
assign b_data_valid_pong_delay4_11 = b_data_valid_pong_delay0_11 & b_data_valid_pong_delay0_15;
assign b_data_valid_pong_delay5_11 = b_data_valid_pong_delay0_11 & b_data_valid_pong_delay0_16;
assign b_data_valid_pong_delay6_11 = b_data_valid_pong_delay0_11 & b_data_valid_pong_delay0_17;
assign b_data_valid_pong_delay7_11 = b_data_valid_pong_delay0_11 & b_data_valid_pong_delay0_18;
assign b_data_valid_pong_delay8_11 = b_data_valid_pong_delay0_11 & b_data_valid_pong_delay0_19;
assign b_data_valid_pong_delay9_11 = b_data_valid_pong_delay0_11 & b_data_valid_pong_delay0_20;
assign b_data_valid_pong_delay10_11 = b_data_valid_pong_delay0_11 & b_data_valid_pong_delay0_21;
assign b_data_valid_pong_delay11_11 = b_data_valid_pong_delay0_11 & b_data_valid_pong_delay0_22;
assign b_data_valid_pong_delay12_11 = b_data_valid_pong_delay0_11 & b_data_valid_pong_delay0_23;
assign b_data_valid_pong_delay13_11 = b_data_valid_pong_delay0_11 & b_data_valid_pong_delay0_24;
assign b_data_valid_pong_delay14_11 = b_data_valid_pong_delay0_11 & b_data_valid_pong_delay0_25;
assign b_data_valid_pong_delay15_11 = b_data_valid_pong_delay0_11 & b_data_valid_pong_delay0_26;
assign b_data_valid_pong_delay1_12 = b_data_valid_pong_delay0_12 & b_data_valid_pong_delay0_13;
assign b_data_valid_pong_delay2_12 = b_data_valid_pong_delay0_12 & b_data_valid_pong_delay0_14;
assign b_data_valid_pong_delay3_12 = b_data_valid_pong_delay0_12 & b_data_valid_pong_delay0_15;
assign b_data_valid_pong_delay4_12 = b_data_valid_pong_delay0_12 & b_data_valid_pong_delay0_16;
assign b_data_valid_pong_delay5_12 = b_data_valid_pong_delay0_12 & b_data_valid_pong_delay0_17;
assign b_data_valid_pong_delay6_12 = b_data_valid_pong_delay0_12 & b_data_valid_pong_delay0_18;
assign b_data_valid_pong_delay7_12 = b_data_valid_pong_delay0_12 & b_data_valid_pong_delay0_19;
assign b_data_valid_pong_delay8_12 = b_data_valid_pong_delay0_12 & b_data_valid_pong_delay0_20;
assign b_data_valid_pong_delay9_12 = b_data_valid_pong_delay0_12 & b_data_valid_pong_delay0_21;
assign b_data_valid_pong_delay10_12 = b_data_valid_pong_delay0_12 & b_data_valid_pong_delay0_22;
assign b_data_valid_pong_delay11_12 = b_data_valid_pong_delay0_12 & b_data_valid_pong_delay0_23;
assign b_data_valid_pong_delay12_12 = b_data_valid_pong_delay0_12 & b_data_valid_pong_delay0_24;
assign b_data_valid_pong_delay13_12 = b_data_valid_pong_delay0_12 & b_data_valid_pong_delay0_25;
assign b_data_valid_pong_delay14_12 = b_data_valid_pong_delay0_12 & b_data_valid_pong_delay0_26;
assign b_data_valid_pong_delay15_12 = b_data_valid_pong_delay0_12 & b_data_valid_pong_delay0_27;
assign b_data_valid_pong_delay1_13 = b_data_valid_pong_delay0_13 & b_data_valid_pong_delay0_14;
assign b_data_valid_pong_delay2_13 = b_data_valid_pong_delay0_13 & b_data_valid_pong_delay0_15;
assign b_data_valid_pong_delay3_13 = b_data_valid_pong_delay0_13 & b_data_valid_pong_delay0_16;
assign b_data_valid_pong_delay4_13 = b_data_valid_pong_delay0_13 & b_data_valid_pong_delay0_17;
assign b_data_valid_pong_delay5_13 = b_data_valid_pong_delay0_13 & b_data_valid_pong_delay0_18;
assign b_data_valid_pong_delay6_13 = b_data_valid_pong_delay0_13 & b_data_valid_pong_delay0_19;
assign b_data_valid_pong_delay7_13 = b_data_valid_pong_delay0_13 & b_data_valid_pong_delay0_20;
assign b_data_valid_pong_delay8_13 = b_data_valid_pong_delay0_13 & b_data_valid_pong_delay0_21;
assign b_data_valid_pong_delay9_13 = b_data_valid_pong_delay0_13 & b_data_valid_pong_delay0_22;
assign b_data_valid_pong_delay10_13 = b_data_valid_pong_delay0_13 & b_data_valid_pong_delay0_23;
assign b_data_valid_pong_delay11_13 = b_data_valid_pong_delay0_13 & b_data_valid_pong_delay0_24;
assign b_data_valid_pong_delay12_13 = b_data_valid_pong_delay0_13 & b_data_valid_pong_delay0_25;
assign b_data_valid_pong_delay13_13 = b_data_valid_pong_delay0_13 & b_data_valid_pong_delay0_26;
assign b_data_valid_pong_delay14_13 = b_data_valid_pong_delay0_13 & b_data_valid_pong_delay0_27;
assign b_data_valid_pong_delay15_13 = b_data_valid_pong_delay0_13 & b_data_valid_pong_delay0_28;
assign b_data_valid_pong_delay1_14 = b_data_valid_pong_delay0_14 & b_data_valid_pong_delay0_15;
assign b_data_valid_pong_delay2_14 = b_data_valid_pong_delay0_14 & b_data_valid_pong_delay0_16;
assign b_data_valid_pong_delay3_14 = b_data_valid_pong_delay0_14 & b_data_valid_pong_delay0_17;
assign b_data_valid_pong_delay4_14 = b_data_valid_pong_delay0_14 & b_data_valid_pong_delay0_18;
assign b_data_valid_pong_delay5_14 = b_data_valid_pong_delay0_14 & b_data_valid_pong_delay0_19;
assign b_data_valid_pong_delay6_14 = b_data_valid_pong_delay0_14 & b_data_valid_pong_delay0_20;
assign b_data_valid_pong_delay7_14 = b_data_valid_pong_delay0_14 & b_data_valid_pong_delay0_21;
assign b_data_valid_pong_delay8_14 = b_data_valid_pong_delay0_14 & b_data_valid_pong_delay0_22;
assign b_data_valid_pong_delay9_14 = b_data_valid_pong_delay0_14 & b_data_valid_pong_delay0_23;
assign b_data_valid_pong_delay10_14 = b_data_valid_pong_delay0_14 & b_data_valid_pong_delay0_24;
assign b_data_valid_pong_delay11_14 = b_data_valid_pong_delay0_14 & b_data_valid_pong_delay0_25;
assign b_data_valid_pong_delay12_14 = b_data_valid_pong_delay0_14 & b_data_valid_pong_delay0_26;
assign b_data_valid_pong_delay13_14 = b_data_valid_pong_delay0_14 & b_data_valid_pong_delay0_27;
assign b_data_valid_pong_delay14_14 = b_data_valid_pong_delay0_14 & b_data_valid_pong_delay0_28;
assign b_data_valid_pong_delay15_14 = b_data_valid_pong_delay0_14 & b_data_valid_pong_delay0_29;
assign b_data_valid_pong_delay1_15 = b_data_valid_pong_delay0_15 & b_data_valid_pong_delay0_16;
assign b_data_valid_pong_delay2_15 = b_data_valid_pong_delay0_15 & b_data_valid_pong_delay0_17;
assign b_data_valid_pong_delay3_15 = b_data_valid_pong_delay0_15 & b_data_valid_pong_delay0_18;
assign b_data_valid_pong_delay4_15 = b_data_valid_pong_delay0_15 & b_data_valid_pong_delay0_19;
assign b_data_valid_pong_delay5_15 = b_data_valid_pong_delay0_15 & b_data_valid_pong_delay0_20;
assign b_data_valid_pong_delay6_15 = b_data_valid_pong_delay0_15 & b_data_valid_pong_delay0_21;
assign b_data_valid_pong_delay7_15 = b_data_valid_pong_delay0_15 & b_data_valid_pong_delay0_22;
assign b_data_valid_pong_delay8_15 = b_data_valid_pong_delay0_15 & b_data_valid_pong_delay0_23;
assign b_data_valid_pong_delay9_15 = b_data_valid_pong_delay0_15 & b_data_valid_pong_delay0_24;
assign b_data_valid_pong_delay10_15 = b_data_valid_pong_delay0_15 & b_data_valid_pong_delay0_25;
assign b_data_valid_pong_delay11_15 = b_data_valid_pong_delay0_15 & b_data_valid_pong_delay0_26;
assign b_data_valid_pong_delay12_15 = b_data_valid_pong_delay0_15 & b_data_valid_pong_delay0_27;
assign b_data_valid_pong_delay13_15 = b_data_valid_pong_delay0_15 & b_data_valid_pong_delay0_28;
assign b_data_valid_pong_delay14_15 = b_data_valid_pong_delay0_15 & b_data_valid_pong_delay0_29;
assign b_data_valid_pong_delay15_15 = b_data_valid_pong_delay0_15 & b_data_valid_pong_delay0_30;

// Signals for Each PING buffer

reg b_data_valid_ping_delay0_1;
reg b_data_valid_ping_delay0_2;
reg b_data_valid_ping_delay0_3;
reg b_data_valid_ping_delay0_4;
reg b_data_valid_ping_delay0_5;
reg b_data_valid_ping_delay0_6;
reg b_data_valid_ping_delay0_7;
reg b_data_valid_ping_delay0_8;
reg b_data_valid_ping_delay0_9;
reg b_data_valid_ping_delay0_10;
reg b_data_valid_ping_delay0_11;
reg b_data_valid_ping_delay0_12;
reg b_data_valid_ping_delay0_13;
reg b_data_valid_ping_delay0_14;
reg b_data_valid_ping_delay0_15;
reg b_data_valid_ping_delay0_16;
reg b_data_valid_ping_delay0_17;
reg b_data_valid_ping_delay0_18;
reg b_data_valid_ping_delay0_19;
reg b_data_valid_ping_delay0_20;
reg b_data_valid_ping_delay0_21;
reg b_data_valid_ping_delay0_22;
reg b_data_valid_ping_delay0_23;
reg b_data_valid_ping_delay0_24;
reg b_data_valid_ping_delay0_25;
reg b_data_valid_ping_delay0_26;
reg b_data_valid_ping_delay0_27;
reg b_data_valid_ping_delay0_28;
reg b_data_valid_ping_delay0_29;
reg b_data_valid_ping_delay0_30;
wire b_data_valid_ping_delay1_0;
wire b_data_valid_ping_delay2_0;
wire b_data_valid_ping_delay3_0;
wire b_data_valid_ping_delay4_0;
wire b_data_valid_ping_delay5_0;
wire b_data_valid_ping_delay6_0;
wire b_data_valid_ping_delay7_0;
wire b_data_valid_ping_delay8_0;
wire b_data_valid_ping_delay9_0;
wire b_data_valid_ping_delay10_0;
wire b_data_valid_ping_delay11_0;
wire b_data_valid_ping_delay12_0;
wire b_data_valid_ping_delay13_0;
wire b_data_valid_ping_delay14_0;
wire b_data_valid_ping_delay15_0;
wire b_data_valid_ping_delay1_1;
wire b_data_valid_ping_delay2_1;
wire b_data_valid_ping_delay3_1;
wire b_data_valid_ping_delay4_1;
wire b_data_valid_ping_delay5_1;
wire b_data_valid_ping_delay6_1;
wire b_data_valid_ping_delay7_1;
wire b_data_valid_ping_delay8_1;
wire b_data_valid_ping_delay9_1;
wire b_data_valid_ping_delay10_1;
wire b_data_valid_ping_delay11_1;
wire b_data_valid_ping_delay12_1;
wire b_data_valid_ping_delay13_1;
wire b_data_valid_ping_delay14_1;
wire b_data_valid_ping_delay15_1;
wire b_data_valid_ping_delay1_2;
wire b_data_valid_ping_delay2_2;
wire b_data_valid_ping_delay3_2;
wire b_data_valid_ping_delay4_2;
wire b_data_valid_ping_delay5_2;
wire b_data_valid_ping_delay6_2;
wire b_data_valid_ping_delay7_2;
wire b_data_valid_ping_delay8_2;
wire b_data_valid_ping_delay9_2;
wire b_data_valid_ping_delay10_2;
wire b_data_valid_ping_delay11_2;
wire b_data_valid_ping_delay12_2;
wire b_data_valid_ping_delay13_2;
wire b_data_valid_ping_delay14_2;
wire b_data_valid_ping_delay15_2;
wire b_data_valid_ping_delay1_3;
wire b_data_valid_ping_delay2_3;
wire b_data_valid_ping_delay3_3;
wire b_data_valid_ping_delay4_3;
wire b_data_valid_ping_delay5_3;
wire b_data_valid_ping_delay6_3;
wire b_data_valid_ping_delay7_3;
wire b_data_valid_ping_delay8_3;
wire b_data_valid_ping_delay9_3;
wire b_data_valid_ping_delay10_3;
wire b_data_valid_ping_delay11_3;
wire b_data_valid_ping_delay12_3;
wire b_data_valid_ping_delay13_3;
wire b_data_valid_ping_delay14_3;
wire b_data_valid_ping_delay15_3;
wire b_data_valid_ping_delay1_4;
wire b_data_valid_ping_delay2_4;
wire b_data_valid_ping_delay3_4;
wire b_data_valid_ping_delay4_4;
wire b_data_valid_ping_delay5_4;
wire b_data_valid_ping_delay6_4;
wire b_data_valid_ping_delay7_4;
wire b_data_valid_ping_delay8_4;
wire b_data_valid_ping_delay9_4;
wire b_data_valid_ping_delay10_4;
wire b_data_valid_ping_delay11_4;
wire b_data_valid_ping_delay12_4;
wire b_data_valid_ping_delay13_4;
wire b_data_valid_ping_delay14_4;
wire b_data_valid_ping_delay15_4;
wire b_data_valid_ping_delay1_5;
wire b_data_valid_ping_delay2_5;
wire b_data_valid_ping_delay3_5;
wire b_data_valid_ping_delay4_5;
wire b_data_valid_ping_delay5_5;
wire b_data_valid_ping_delay6_5;
wire b_data_valid_ping_delay7_5;
wire b_data_valid_ping_delay8_5;
wire b_data_valid_ping_delay9_5;
wire b_data_valid_ping_delay10_5;
wire b_data_valid_ping_delay11_5;
wire b_data_valid_ping_delay12_5;
wire b_data_valid_ping_delay13_5;
wire b_data_valid_ping_delay14_5;
wire b_data_valid_ping_delay15_5;
wire b_data_valid_ping_delay1_6;
wire b_data_valid_ping_delay2_6;
wire b_data_valid_ping_delay3_6;
wire b_data_valid_ping_delay4_6;
wire b_data_valid_ping_delay5_6;
wire b_data_valid_ping_delay6_6;
wire b_data_valid_ping_delay7_6;
wire b_data_valid_ping_delay8_6;
wire b_data_valid_ping_delay9_6;
wire b_data_valid_ping_delay10_6;
wire b_data_valid_ping_delay11_6;
wire b_data_valid_ping_delay12_6;
wire b_data_valid_ping_delay13_6;
wire b_data_valid_ping_delay14_6;
wire b_data_valid_ping_delay15_6;
wire b_data_valid_ping_delay1_7;
wire b_data_valid_ping_delay2_7;
wire b_data_valid_ping_delay3_7;
wire b_data_valid_ping_delay4_7;
wire b_data_valid_ping_delay5_7;
wire b_data_valid_ping_delay6_7;
wire b_data_valid_ping_delay7_7;
wire b_data_valid_ping_delay8_7;
wire b_data_valid_ping_delay9_7;
wire b_data_valid_ping_delay10_7;
wire b_data_valid_ping_delay11_7;
wire b_data_valid_ping_delay12_7;
wire b_data_valid_ping_delay13_7;
wire b_data_valid_ping_delay14_7;
wire b_data_valid_ping_delay15_7;
wire b_data_valid_ping_delay1_8;
wire b_data_valid_ping_delay2_8;
wire b_data_valid_ping_delay3_8;
wire b_data_valid_ping_delay4_8;
wire b_data_valid_ping_delay5_8;
wire b_data_valid_ping_delay6_8;
wire b_data_valid_ping_delay7_8;
wire b_data_valid_ping_delay8_8;
wire b_data_valid_ping_delay9_8;
wire b_data_valid_ping_delay10_8;
wire b_data_valid_ping_delay11_8;
wire b_data_valid_ping_delay12_8;
wire b_data_valid_ping_delay13_8;
wire b_data_valid_ping_delay14_8;
wire b_data_valid_ping_delay15_8;
wire b_data_valid_ping_delay1_9;
wire b_data_valid_ping_delay2_9;
wire b_data_valid_ping_delay3_9;
wire b_data_valid_ping_delay4_9;
wire b_data_valid_ping_delay5_9;
wire b_data_valid_ping_delay6_9;
wire b_data_valid_ping_delay7_9;
wire b_data_valid_ping_delay8_9;
wire b_data_valid_ping_delay9_9;
wire b_data_valid_ping_delay10_9;
wire b_data_valid_ping_delay11_9;
wire b_data_valid_ping_delay12_9;
wire b_data_valid_ping_delay13_9;
wire b_data_valid_ping_delay14_9;
wire b_data_valid_ping_delay15_9;
wire b_data_valid_ping_delay1_10;
wire b_data_valid_ping_delay2_10;
wire b_data_valid_ping_delay3_10;
wire b_data_valid_ping_delay4_10;
wire b_data_valid_ping_delay5_10;
wire b_data_valid_ping_delay6_10;
wire b_data_valid_ping_delay7_10;
wire b_data_valid_ping_delay8_10;
wire b_data_valid_ping_delay9_10;
wire b_data_valid_ping_delay10_10;
wire b_data_valid_ping_delay11_10;
wire b_data_valid_ping_delay12_10;
wire b_data_valid_ping_delay13_10;
wire b_data_valid_ping_delay14_10;
wire b_data_valid_ping_delay15_10;
wire b_data_valid_ping_delay1_11;
wire b_data_valid_ping_delay2_11;
wire b_data_valid_ping_delay3_11;
wire b_data_valid_ping_delay4_11;
wire b_data_valid_ping_delay5_11;
wire b_data_valid_ping_delay6_11;
wire b_data_valid_ping_delay7_11;
wire b_data_valid_ping_delay8_11;
wire b_data_valid_ping_delay9_11;
wire b_data_valid_ping_delay10_11;
wire b_data_valid_ping_delay11_11;
wire b_data_valid_ping_delay12_11;
wire b_data_valid_ping_delay13_11;
wire b_data_valid_ping_delay14_11;
wire b_data_valid_ping_delay15_11;
wire b_data_valid_ping_delay1_12;
wire b_data_valid_ping_delay2_12;
wire b_data_valid_ping_delay3_12;
wire b_data_valid_ping_delay4_12;
wire b_data_valid_ping_delay5_12;
wire b_data_valid_ping_delay6_12;
wire b_data_valid_ping_delay7_12;
wire b_data_valid_ping_delay8_12;
wire b_data_valid_ping_delay9_12;
wire b_data_valid_ping_delay10_12;
wire b_data_valid_ping_delay11_12;
wire b_data_valid_ping_delay12_12;
wire b_data_valid_ping_delay13_12;
wire b_data_valid_ping_delay14_12;
wire b_data_valid_ping_delay15_12;
wire b_data_valid_ping_delay1_13;
wire b_data_valid_ping_delay2_13;
wire b_data_valid_ping_delay3_13;
wire b_data_valid_ping_delay4_13;
wire b_data_valid_ping_delay5_13;
wire b_data_valid_ping_delay6_13;
wire b_data_valid_ping_delay7_13;
wire b_data_valid_ping_delay8_13;
wire b_data_valid_ping_delay9_13;
wire b_data_valid_ping_delay10_13;
wire b_data_valid_ping_delay11_13;
wire b_data_valid_ping_delay12_13;
wire b_data_valid_ping_delay13_13;
wire b_data_valid_ping_delay14_13;
wire b_data_valid_ping_delay15_13;
wire b_data_valid_ping_delay1_14;
wire b_data_valid_ping_delay2_14;
wire b_data_valid_ping_delay3_14;
wire b_data_valid_ping_delay4_14;
wire b_data_valid_ping_delay5_14;
wire b_data_valid_ping_delay6_14;
wire b_data_valid_ping_delay7_14;
wire b_data_valid_ping_delay8_14;
wire b_data_valid_ping_delay9_14;
wire b_data_valid_ping_delay10_14;
wire b_data_valid_ping_delay11_14;
wire b_data_valid_ping_delay12_14;
wire b_data_valid_ping_delay13_14;
wire b_data_valid_ping_delay14_14;
wire b_data_valid_ping_delay15_14;
wire b_data_valid_ping_delay1_15;
wire b_data_valid_ping_delay2_15;
wire b_data_valid_ping_delay3_15;
wire b_data_valid_ping_delay4_15;
wire b_data_valid_ping_delay5_15;
wire b_data_valid_ping_delay6_15;
wire b_data_valid_ping_delay7_15;
wire b_data_valid_ping_delay8_15;
wire b_data_valid_ping_delay9_15;
wire b_data_valid_ping_delay10_15;
wire b_data_valid_ping_delay11_15;
wire b_data_valid_ping_delay12_15;
wire b_data_valid_ping_delay13_15;
wire b_data_valid_ping_delay14_15;
wire b_data_valid_ping_delay15_15;
  
always @ (posedge clk) begin
    b_data_valid_ping_delay0_1 <= b_data_valid_ping;
    b_data_valid_ping_delay0_2 <= b_data_valid_ping_delay0_1;
    b_data_valid_ping_delay0_3 <= b_data_valid_ping_delay0_2;
    b_data_valid_ping_delay0_4 <= b_data_valid_ping_delay0_3;
    b_data_valid_ping_delay0_5 <= b_data_valid_ping_delay0_4;
    b_data_valid_ping_delay0_6 <= b_data_valid_ping_delay0_5;
    b_data_valid_ping_delay0_7 <= b_data_valid_ping_delay0_6;
    b_data_valid_ping_delay0_8 <= b_data_valid_ping_delay0_7;
    b_data_valid_ping_delay0_9 <= b_data_valid_ping_delay0_8;
    b_data_valid_ping_delay0_10 <= b_data_valid_ping_delay0_9;
    b_data_valid_ping_delay0_11 <= b_data_valid_ping_delay0_10;
    b_data_valid_ping_delay0_12 <= b_data_valid_ping_delay0_11;
    b_data_valid_ping_delay0_13 <= b_data_valid_ping_delay0_12;
    b_data_valid_ping_delay0_14 <= b_data_valid_ping_delay0_13;
    b_data_valid_ping_delay0_15 <= b_data_valid_ping_delay0_14;
    b_data_valid_ping_delay0_16 <= b_data_valid_ping_delay0_15;
    b_data_valid_ping_delay0_17 <= b_data_valid_ping_delay0_16;
    b_data_valid_ping_delay0_18 <= b_data_valid_ping_delay0_17;
    b_data_valid_ping_delay0_19 <= b_data_valid_ping_delay0_18;
    b_data_valid_ping_delay0_20 <= b_data_valid_ping_delay0_19;
    b_data_valid_ping_delay0_21 <= b_data_valid_ping_delay0_20;
    b_data_valid_ping_delay0_22 <= b_data_valid_ping_delay0_21;
    b_data_valid_ping_delay0_23 <= b_data_valid_ping_delay0_22;
    b_data_valid_ping_delay0_24 <= b_data_valid_ping_delay0_23;
    b_data_valid_ping_delay0_25 <= b_data_valid_ping_delay0_24;
    b_data_valid_ping_delay0_26 <= b_data_valid_ping_delay0_25;
    b_data_valid_ping_delay0_27 <= b_data_valid_ping_delay0_26;
    b_data_valid_ping_delay0_28 <= b_data_valid_ping_delay0_27;
    b_data_valid_ping_delay0_29 <= b_data_valid_ping_delay0_28;
    b_data_valid_ping_delay0_30 <= b_data_valid_ping_delay0_29;
end

assign b_data_valid_ping_delay1_0 = b_data_valid_ping & b_data_valid_ping_delay0_1;
assign b_data_valid_ping_delay2_0 = b_data_valid_ping & b_data_valid_ping_delay0_2;
assign b_data_valid_ping_delay3_0 = b_data_valid_ping & b_data_valid_ping_delay0_3;
assign b_data_valid_ping_delay4_0 = b_data_valid_ping & b_data_valid_ping_delay0_4;
assign b_data_valid_ping_delay5_0 = b_data_valid_ping & b_data_valid_ping_delay0_5;
assign b_data_valid_ping_delay6_0 = b_data_valid_ping & b_data_valid_ping_delay0_6;
assign b_data_valid_ping_delay7_0 = b_data_valid_ping & b_data_valid_ping_delay0_7;
assign b_data_valid_ping_delay8_0 = b_data_valid_ping & b_data_valid_ping_delay0_8;
assign b_data_valid_ping_delay9_0 = b_data_valid_ping & b_data_valid_ping_delay0_9;
assign b_data_valid_ping_delay10_0 = b_data_valid_ping & b_data_valid_ping_delay0_10;
assign b_data_valid_ping_delay11_0 = b_data_valid_ping & b_data_valid_ping_delay0_11;
assign b_data_valid_ping_delay12_0 = b_data_valid_ping & b_data_valid_ping_delay0_12;
assign b_data_valid_ping_delay13_0 = b_data_valid_ping & b_data_valid_ping_delay0_13;
assign b_data_valid_ping_delay14_0 = b_data_valid_ping & b_data_valid_ping_delay0_14;
assign b_data_valid_ping_delay15_0 = b_data_valid_ping & b_data_valid_ping_delay0_15;
assign b_data_valid_ping_delay1_1 = b_data_valid_ping_delay0_1 & b_data_valid_ping_delay0_2;
assign b_data_valid_ping_delay2_1 = b_data_valid_ping_delay0_1 & b_data_valid_ping_delay0_3;
assign b_data_valid_ping_delay3_1 = b_data_valid_ping_delay0_1 & b_data_valid_ping_delay0_4;
assign b_data_valid_ping_delay4_1 = b_data_valid_ping_delay0_1 & b_data_valid_ping_delay0_5;
assign b_data_valid_ping_delay5_1 = b_data_valid_ping_delay0_1 & b_data_valid_ping_delay0_6;
assign b_data_valid_ping_delay6_1 = b_data_valid_ping_delay0_1 & b_data_valid_ping_delay0_7;
assign b_data_valid_ping_delay7_1 = b_data_valid_ping_delay0_1 & b_data_valid_ping_delay0_8;
assign b_data_valid_ping_delay8_1 = b_data_valid_ping_delay0_1 & b_data_valid_ping_delay0_9;
assign b_data_valid_ping_delay9_1 = b_data_valid_ping_delay0_1 & b_data_valid_ping_delay0_10;
assign b_data_valid_ping_delay10_1 = b_data_valid_ping_delay0_1 & b_data_valid_ping_delay0_11;
assign b_data_valid_ping_delay11_1 = b_data_valid_ping_delay0_1 & b_data_valid_ping_delay0_12;
assign b_data_valid_ping_delay12_1 = b_data_valid_ping_delay0_1 & b_data_valid_ping_delay0_13;
assign b_data_valid_ping_delay13_1 = b_data_valid_ping_delay0_1 & b_data_valid_ping_delay0_14;
assign b_data_valid_ping_delay14_1 = b_data_valid_ping_delay0_1 & b_data_valid_ping_delay0_15;
assign b_data_valid_ping_delay15_1 = b_data_valid_ping_delay0_1 & b_data_valid_ping_delay0_16;
assign b_data_valid_ping_delay1_2 = b_data_valid_ping_delay0_2 & b_data_valid_ping_delay0_3;
assign b_data_valid_ping_delay2_2 = b_data_valid_ping_delay0_2 & b_data_valid_ping_delay0_4;
assign b_data_valid_ping_delay3_2 = b_data_valid_ping_delay0_2 & b_data_valid_ping_delay0_5;
assign b_data_valid_ping_delay4_2 = b_data_valid_ping_delay0_2 & b_data_valid_ping_delay0_6;
assign b_data_valid_ping_delay5_2 = b_data_valid_ping_delay0_2 & b_data_valid_ping_delay0_7;
assign b_data_valid_ping_delay6_2 = b_data_valid_ping_delay0_2 & b_data_valid_ping_delay0_8;
assign b_data_valid_ping_delay7_2 = b_data_valid_ping_delay0_2 & b_data_valid_ping_delay0_9;
assign b_data_valid_ping_delay8_2 = b_data_valid_ping_delay0_2 & b_data_valid_ping_delay0_10;
assign b_data_valid_ping_delay9_2 = b_data_valid_ping_delay0_2 & b_data_valid_ping_delay0_11;
assign b_data_valid_ping_delay10_2 = b_data_valid_ping_delay0_2 & b_data_valid_ping_delay0_12;
assign b_data_valid_ping_delay11_2 = b_data_valid_ping_delay0_2 & b_data_valid_ping_delay0_13;
assign b_data_valid_ping_delay12_2 = b_data_valid_ping_delay0_2 & b_data_valid_ping_delay0_14;
assign b_data_valid_ping_delay13_2 = b_data_valid_ping_delay0_2 & b_data_valid_ping_delay0_15;
assign b_data_valid_ping_delay14_2 = b_data_valid_ping_delay0_2 & b_data_valid_ping_delay0_16;
assign b_data_valid_ping_delay15_2 = b_data_valid_ping_delay0_2 & b_data_valid_ping_delay0_17;
assign b_data_valid_ping_delay1_3 = b_data_valid_ping_delay0_3 & b_data_valid_ping_delay0_4;
assign b_data_valid_ping_delay2_3 = b_data_valid_ping_delay0_3 & b_data_valid_ping_delay0_5;
assign b_data_valid_ping_delay3_3 = b_data_valid_ping_delay0_3 & b_data_valid_ping_delay0_6;
assign b_data_valid_ping_delay4_3 = b_data_valid_ping_delay0_3 & b_data_valid_ping_delay0_7;
assign b_data_valid_ping_delay5_3 = b_data_valid_ping_delay0_3 & b_data_valid_ping_delay0_8;
assign b_data_valid_ping_delay6_3 = b_data_valid_ping_delay0_3 & b_data_valid_ping_delay0_9;
assign b_data_valid_ping_delay7_3 = b_data_valid_ping_delay0_3 & b_data_valid_ping_delay0_10;
assign b_data_valid_ping_delay8_3 = b_data_valid_ping_delay0_3 & b_data_valid_ping_delay0_11;
assign b_data_valid_ping_delay9_3 = b_data_valid_ping_delay0_3 & b_data_valid_ping_delay0_12;
assign b_data_valid_ping_delay10_3 = b_data_valid_ping_delay0_3 & b_data_valid_ping_delay0_13;
assign b_data_valid_ping_delay11_3 = b_data_valid_ping_delay0_3 & b_data_valid_ping_delay0_14;
assign b_data_valid_ping_delay12_3 = b_data_valid_ping_delay0_3 & b_data_valid_ping_delay0_15;
assign b_data_valid_ping_delay13_3 = b_data_valid_ping_delay0_3 & b_data_valid_ping_delay0_16;
assign b_data_valid_ping_delay14_3 = b_data_valid_ping_delay0_3 & b_data_valid_ping_delay0_17;
assign b_data_valid_ping_delay15_3 = b_data_valid_ping_delay0_3 & b_data_valid_ping_delay0_18;
assign b_data_valid_ping_delay1_4 = b_data_valid_ping_delay0_4 & b_data_valid_ping_delay0_5;
assign b_data_valid_ping_delay2_4 = b_data_valid_ping_delay0_4 & b_data_valid_ping_delay0_6;
assign b_data_valid_ping_delay3_4 = b_data_valid_ping_delay0_4 & b_data_valid_ping_delay0_7;
assign b_data_valid_ping_delay4_4 = b_data_valid_ping_delay0_4 & b_data_valid_ping_delay0_8;
assign b_data_valid_ping_delay5_4 = b_data_valid_ping_delay0_4 & b_data_valid_ping_delay0_9;
assign b_data_valid_ping_delay6_4 = b_data_valid_ping_delay0_4 & b_data_valid_ping_delay0_10;
assign b_data_valid_ping_delay7_4 = b_data_valid_ping_delay0_4 & b_data_valid_ping_delay0_11;
assign b_data_valid_ping_delay8_4 = b_data_valid_ping_delay0_4 & b_data_valid_ping_delay0_12;
assign b_data_valid_ping_delay9_4 = b_data_valid_ping_delay0_4 & b_data_valid_ping_delay0_13;
assign b_data_valid_ping_delay10_4 = b_data_valid_ping_delay0_4 & b_data_valid_ping_delay0_14;
assign b_data_valid_ping_delay11_4 = b_data_valid_ping_delay0_4 & b_data_valid_ping_delay0_15;
assign b_data_valid_ping_delay12_4 = b_data_valid_ping_delay0_4 & b_data_valid_ping_delay0_16;
assign b_data_valid_ping_delay13_4 = b_data_valid_ping_delay0_4 & b_data_valid_ping_delay0_17;
assign b_data_valid_ping_delay14_4 = b_data_valid_ping_delay0_4 & b_data_valid_ping_delay0_18;
assign b_data_valid_ping_delay15_4 = b_data_valid_ping_delay0_4 & b_data_valid_ping_delay0_19;
assign b_data_valid_ping_delay1_5 = b_data_valid_ping_delay0_5 & b_data_valid_ping_delay0_6;
assign b_data_valid_ping_delay2_5 = b_data_valid_ping_delay0_5 & b_data_valid_ping_delay0_7;
assign b_data_valid_ping_delay3_5 = b_data_valid_ping_delay0_5 & b_data_valid_ping_delay0_8;
assign b_data_valid_ping_delay4_5 = b_data_valid_ping_delay0_5 & b_data_valid_ping_delay0_9;
assign b_data_valid_ping_delay5_5 = b_data_valid_ping_delay0_5 & b_data_valid_ping_delay0_10;
assign b_data_valid_ping_delay6_5 = b_data_valid_ping_delay0_5 & b_data_valid_ping_delay0_11;
assign b_data_valid_ping_delay7_5 = b_data_valid_ping_delay0_5 & b_data_valid_ping_delay0_12;
assign b_data_valid_ping_delay8_5 = b_data_valid_ping_delay0_5 & b_data_valid_ping_delay0_13;
assign b_data_valid_ping_delay9_5 = b_data_valid_ping_delay0_5 & b_data_valid_ping_delay0_14;
assign b_data_valid_ping_delay10_5 = b_data_valid_ping_delay0_5 & b_data_valid_ping_delay0_15;
assign b_data_valid_ping_delay11_5 = b_data_valid_ping_delay0_5 & b_data_valid_ping_delay0_16;
assign b_data_valid_ping_delay12_5 = b_data_valid_ping_delay0_5 & b_data_valid_ping_delay0_17;
assign b_data_valid_ping_delay13_5 = b_data_valid_ping_delay0_5 & b_data_valid_ping_delay0_18;
assign b_data_valid_ping_delay14_5 = b_data_valid_ping_delay0_5 & b_data_valid_ping_delay0_19;
assign b_data_valid_ping_delay15_5 = b_data_valid_ping_delay0_5 & b_data_valid_ping_delay0_20;
assign b_data_valid_ping_delay1_6 = b_data_valid_ping_delay0_6 & b_data_valid_ping_delay0_7;
assign b_data_valid_ping_delay2_6 = b_data_valid_ping_delay0_6 & b_data_valid_ping_delay0_8;
assign b_data_valid_ping_delay3_6 = b_data_valid_ping_delay0_6 & b_data_valid_ping_delay0_9;
assign b_data_valid_ping_delay4_6 = b_data_valid_ping_delay0_6 & b_data_valid_ping_delay0_10;
assign b_data_valid_ping_delay5_6 = b_data_valid_ping_delay0_6 & b_data_valid_ping_delay0_11;
assign b_data_valid_ping_delay6_6 = b_data_valid_ping_delay0_6 & b_data_valid_ping_delay0_12;
assign b_data_valid_ping_delay7_6 = b_data_valid_ping_delay0_6 & b_data_valid_ping_delay0_13;
assign b_data_valid_ping_delay8_6 = b_data_valid_ping_delay0_6 & b_data_valid_ping_delay0_14;
assign b_data_valid_ping_delay9_6 = b_data_valid_ping_delay0_6 & b_data_valid_ping_delay0_15;
assign b_data_valid_ping_delay10_6 = b_data_valid_ping_delay0_6 & b_data_valid_ping_delay0_16;
assign b_data_valid_ping_delay11_6 = b_data_valid_ping_delay0_6 & b_data_valid_ping_delay0_17;
assign b_data_valid_ping_delay12_6 = b_data_valid_ping_delay0_6 & b_data_valid_ping_delay0_18;
assign b_data_valid_ping_delay13_6 = b_data_valid_ping_delay0_6 & b_data_valid_ping_delay0_19;
assign b_data_valid_ping_delay14_6 = b_data_valid_ping_delay0_6 & b_data_valid_ping_delay0_20;
assign b_data_valid_ping_delay15_6 = b_data_valid_ping_delay0_6 & b_data_valid_ping_delay0_21;
assign b_data_valid_ping_delay1_7 = b_data_valid_ping_delay0_7 & b_data_valid_ping_delay0_8;
assign b_data_valid_ping_delay2_7 = b_data_valid_ping_delay0_7 & b_data_valid_ping_delay0_9;
assign b_data_valid_ping_delay3_7 = b_data_valid_ping_delay0_7 & b_data_valid_ping_delay0_10;
assign b_data_valid_ping_delay4_7 = b_data_valid_ping_delay0_7 & b_data_valid_ping_delay0_11;
assign b_data_valid_ping_delay5_7 = b_data_valid_ping_delay0_7 & b_data_valid_ping_delay0_12;
assign b_data_valid_ping_delay6_7 = b_data_valid_ping_delay0_7 & b_data_valid_ping_delay0_13;
assign b_data_valid_ping_delay7_7 = b_data_valid_ping_delay0_7 & b_data_valid_ping_delay0_14;
assign b_data_valid_ping_delay8_7 = b_data_valid_ping_delay0_7 & b_data_valid_ping_delay0_15;
assign b_data_valid_ping_delay9_7 = b_data_valid_ping_delay0_7 & b_data_valid_ping_delay0_16;
assign b_data_valid_ping_delay10_7 = b_data_valid_ping_delay0_7 & b_data_valid_ping_delay0_17;
assign b_data_valid_ping_delay11_7 = b_data_valid_ping_delay0_7 & b_data_valid_ping_delay0_18;
assign b_data_valid_ping_delay12_7 = b_data_valid_ping_delay0_7 & b_data_valid_ping_delay0_19;
assign b_data_valid_ping_delay13_7 = b_data_valid_ping_delay0_7 & b_data_valid_ping_delay0_20;
assign b_data_valid_ping_delay14_7 = b_data_valid_ping_delay0_7 & b_data_valid_ping_delay0_21;
assign b_data_valid_ping_delay15_7 = b_data_valid_ping_delay0_7 & b_data_valid_ping_delay0_22;
assign b_data_valid_ping_delay1_8 = b_data_valid_ping_delay0_8 & b_data_valid_ping_delay0_9;
assign b_data_valid_ping_delay2_8 = b_data_valid_ping_delay0_8 & b_data_valid_ping_delay0_10;
assign b_data_valid_ping_delay3_8 = b_data_valid_ping_delay0_8 & b_data_valid_ping_delay0_11;
assign b_data_valid_ping_delay4_8 = b_data_valid_ping_delay0_8 & b_data_valid_ping_delay0_12;
assign b_data_valid_ping_delay5_8 = b_data_valid_ping_delay0_8 & b_data_valid_ping_delay0_13;
assign b_data_valid_ping_delay6_8 = b_data_valid_ping_delay0_8 & b_data_valid_ping_delay0_14;
assign b_data_valid_ping_delay7_8 = b_data_valid_ping_delay0_8 & b_data_valid_ping_delay0_15;
assign b_data_valid_ping_delay8_8 = b_data_valid_ping_delay0_8 & b_data_valid_ping_delay0_16;
assign b_data_valid_ping_delay9_8 = b_data_valid_ping_delay0_8 & b_data_valid_ping_delay0_17;
assign b_data_valid_ping_delay10_8 = b_data_valid_ping_delay0_8 & b_data_valid_ping_delay0_18;
assign b_data_valid_ping_delay11_8 = b_data_valid_ping_delay0_8 & b_data_valid_ping_delay0_19;
assign b_data_valid_ping_delay12_8 = b_data_valid_ping_delay0_8 & b_data_valid_ping_delay0_20;
assign b_data_valid_ping_delay13_8 = b_data_valid_ping_delay0_8 & b_data_valid_ping_delay0_21;
assign b_data_valid_ping_delay14_8 = b_data_valid_ping_delay0_8 & b_data_valid_ping_delay0_22;
assign b_data_valid_ping_delay15_8 = b_data_valid_ping_delay0_8 & b_data_valid_ping_delay0_23;
assign b_data_valid_ping_delay1_9 = b_data_valid_ping_delay0_9 & b_data_valid_ping_delay0_10;
assign b_data_valid_ping_delay2_9 = b_data_valid_ping_delay0_9 & b_data_valid_ping_delay0_11;
assign b_data_valid_ping_delay3_9 = b_data_valid_ping_delay0_9 & b_data_valid_ping_delay0_12;
assign b_data_valid_ping_delay4_9 = b_data_valid_ping_delay0_9 & b_data_valid_ping_delay0_13;
assign b_data_valid_ping_delay5_9 = b_data_valid_ping_delay0_9 & b_data_valid_ping_delay0_14;
assign b_data_valid_ping_delay6_9 = b_data_valid_ping_delay0_9 & b_data_valid_ping_delay0_15;
assign b_data_valid_ping_delay7_9 = b_data_valid_ping_delay0_9 & b_data_valid_ping_delay0_16;
assign b_data_valid_ping_delay8_9 = b_data_valid_ping_delay0_9 & b_data_valid_ping_delay0_17;
assign b_data_valid_ping_delay9_9 = b_data_valid_ping_delay0_9 & b_data_valid_ping_delay0_18;
assign b_data_valid_ping_delay10_9 = b_data_valid_ping_delay0_9 & b_data_valid_ping_delay0_19;
assign b_data_valid_ping_delay11_9 = b_data_valid_ping_delay0_9 & b_data_valid_ping_delay0_20;
assign b_data_valid_ping_delay12_9 = b_data_valid_ping_delay0_9 & b_data_valid_ping_delay0_21;
assign b_data_valid_ping_delay13_9 = b_data_valid_ping_delay0_9 & b_data_valid_ping_delay0_22;
assign b_data_valid_ping_delay14_9 = b_data_valid_ping_delay0_9 & b_data_valid_ping_delay0_23;
assign b_data_valid_ping_delay15_9 = b_data_valid_ping_delay0_9 & b_data_valid_ping_delay0_24;
assign b_data_valid_ping_delay1_10 = b_data_valid_ping_delay0_10 & b_data_valid_ping_delay0_11;
assign b_data_valid_ping_delay2_10 = b_data_valid_ping_delay0_10 & b_data_valid_ping_delay0_12;
assign b_data_valid_ping_delay3_10 = b_data_valid_ping_delay0_10 & b_data_valid_ping_delay0_13;
assign b_data_valid_ping_delay4_10 = b_data_valid_ping_delay0_10 & b_data_valid_ping_delay0_14;
assign b_data_valid_ping_delay5_10 = b_data_valid_ping_delay0_10 & b_data_valid_ping_delay0_15;
assign b_data_valid_ping_delay6_10 = b_data_valid_ping_delay0_10 & b_data_valid_ping_delay0_16;
assign b_data_valid_ping_delay7_10 = b_data_valid_ping_delay0_10 & b_data_valid_ping_delay0_17;
assign b_data_valid_ping_delay8_10 = b_data_valid_ping_delay0_10 & b_data_valid_ping_delay0_18;
assign b_data_valid_ping_delay9_10 = b_data_valid_ping_delay0_10 & b_data_valid_ping_delay0_19;
assign b_data_valid_ping_delay10_10 = b_data_valid_ping_delay0_10 & b_data_valid_ping_delay0_20;
assign b_data_valid_ping_delay11_10 = b_data_valid_ping_delay0_10 & b_data_valid_ping_delay0_21;
assign b_data_valid_ping_delay12_10 = b_data_valid_ping_delay0_10 & b_data_valid_ping_delay0_22;
assign b_data_valid_ping_delay13_10 = b_data_valid_ping_delay0_10 & b_data_valid_ping_delay0_23;
assign b_data_valid_ping_delay14_10 = b_data_valid_ping_delay0_10 & b_data_valid_ping_delay0_24;
assign b_data_valid_ping_delay15_10 = b_data_valid_ping_delay0_10 & b_data_valid_ping_delay0_25;
assign b_data_valid_ping_delay1_11 = b_data_valid_ping_delay0_11 & b_data_valid_ping_delay0_12;
assign b_data_valid_ping_delay2_11 = b_data_valid_ping_delay0_11 & b_data_valid_ping_delay0_13;
assign b_data_valid_ping_delay3_11 = b_data_valid_ping_delay0_11 & b_data_valid_ping_delay0_14;
assign b_data_valid_ping_delay4_11 = b_data_valid_ping_delay0_11 & b_data_valid_ping_delay0_15;
assign b_data_valid_ping_delay5_11 = b_data_valid_ping_delay0_11 & b_data_valid_ping_delay0_16;
assign b_data_valid_ping_delay6_11 = b_data_valid_ping_delay0_11 & b_data_valid_ping_delay0_17;
assign b_data_valid_ping_delay7_11 = b_data_valid_ping_delay0_11 & b_data_valid_ping_delay0_18;
assign b_data_valid_ping_delay8_11 = b_data_valid_ping_delay0_11 & b_data_valid_ping_delay0_19;
assign b_data_valid_ping_delay9_11 = b_data_valid_ping_delay0_11 & b_data_valid_ping_delay0_20;
assign b_data_valid_ping_delay10_11 = b_data_valid_ping_delay0_11 & b_data_valid_ping_delay0_21;
assign b_data_valid_ping_delay11_11 = b_data_valid_ping_delay0_11 & b_data_valid_ping_delay0_22;
assign b_data_valid_ping_delay12_11 = b_data_valid_ping_delay0_11 & b_data_valid_ping_delay0_23;
assign b_data_valid_ping_delay13_11 = b_data_valid_ping_delay0_11 & b_data_valid_ping_delay0_24;
assign b_data_valid_ping_delay14_11 = b_data_valid_ping_delay0_11 & b_data_valid_ping_delay0_25;
assign b_data_valid_ping_delay15_11 = b_data_valid_ping_delay0_11 & b_data_valid_ping_delay0_26;
assign b_data_valid_ping_delay1_12 = b_data_valid_ping_delay0_12 & b_data_valid_ping_delay0_13;
assign b_data_valid_ping_delay2_12 = b_data_valid_ping_delay0_12 & b_data_valid_ping_delay0_14;
assign b_data_valid_ping_delay3_12 = b_data_valid_ping_delay0_12 & b_data_valid_ping_delay0_15;
assign b_data_valid_ping_delay4_12 = b_data_valid_ping_delay0_12 & b_data_valid_ping_delay0_16;
assign b_data_valid_ping_delay5_12 = b_data_valid_ping_delay0_12 & b_data_valid_ping_delay0_17;
assign b_data_valid_ping_delay6_12 = b_data_valid_ping_delay0_12 & b_data_valid_ping_delay0_18;
assign b_data_valid_ping_delay7_12 = b_data_valid_ping_delay0_12 & b_data_valid_ping_delay0_19;
assign b_data_valid_ping_delay8_12 = b_data_valid_ping_delay0_12 & b_data_valid_ping_delay0_20;
assign b_data_valid_ping_delay9_12 = b_data_valid_ping_delay0_12 & b_data_valid_ping_delay0_21;
assign b_data_valid_ping_delay10_12 = b_data_valid_ping_delay0_12 & b_data_valid_ping_delay0_22;
assign b_data_valid_ping_delay11_12 = b_data_valid_ping_delay0_12 & b_data_valid_ping_delay0_23;
assign b_data_valid_ping_delay12_12 = b_data_valid_ping_delay0_12 & b_data_valid_ping_delay0_24;
assign b_data_valid_ping_delay13_12 = b_data_valid_ping_delay0_12 & b_data_valid_ping_delay0_25;
assign b_data_valid_ping_delay14_12 = b_data_valid_ping_delay0_12 & b_data_valid_ping_delay0_26;
assign b_data_valid_ping_delay15_12 = b_data_valid_ping_delay0_12 & b_data_valid_ping_delay0_27;
assign b_data_valid_ping_delay1_13 = b_data_valid_ping_delay0_13 & b_data_valid_ping_delay0_14;
assign b_data_valid_ping_delay2_13 = b_data_valid_ping_delay0_13 & b_data_valid_ping_delay0_15;
assign b_data_valid_ping_delay3_13 = b_data_valid_ping_delay0_13 & b_data_valid_ping_delay0_16;
assign b_data_valid_ping_delay4_13 = b_data_valid_ping_delay0_13 & b_data_valid_ping_delay0_17;
assign b_data_valid_ping_delay5_13 = b_data_valid_ping_delay0_13 & b_data_valid_ping_delay0_18;
assign b_data_valid_ping_delay6_13 = b_data_valid_ping_delay0_13 & b_data_valid_ping_delay0_19;
assign b_data_valid_ping_delay7_13 = b_data_valid_ping_delay0_13 & b_data_valid_ping_delay0_20;
assign b_data_valid_ping_delay8_13 = b_data_valid_ping_delay0_13 & b_data_valid_ping_delay0_21;
assign b_data_valid_ping_delay9_13 = b_data_valid_ping_delay0_13 & b_data_valid_ping_delay0_22;
assign b_data_valid_ping_delay10_13 = b_data_valid_ping_delay0_13 & b_data_valid_ping_delay0_23;
assign b_data_valid_ping_delay11_13 = b_data_valid_ping_delay0_13 & b_data_valid_ping_delay0_24;
assign b_data_valid_ping_delay12_13 = b_data_valid_ping_delay0_13 & b_data_valid_ping_delay0_25;
assign b_data_valid_ping_delay13_13 = b_data_valid_ping_delay0_13 & b_data_valid_ping_delay0_26;
assign b_data_valid_ping_delay14_13 = b_data_valid_ping_delay0_13 & b_data_valid_ping_delay0_27;
assign b_data_valid_ping_delay15_13 = b_data_valid_ping_delay0_13 & b_data_valid_ping_delay0_28;
assign b_data_valid_ping_delay1_14 = b_data_valid_ping_delay0_14 & b_data_valid_ping_delay0_15;
assign b_data_valid_ping_delay2_14 = b_data_valid_ping_delay0_14 & b_data_valid_ping_delay0_16;
assign b_data_valid_ping_delay3_14 = b_data_valid_ping_delay0_14 & b_data_valid_ping_delay0_17;
assign b_data_valid_ping_delay4_14 = b_data_valid_ping_delay0_14 & b_data_valid_ping_delay0_18;
assign b_data_valid_ping_delay5_14 = b_data_valid_ping_delay0_14 & b_data_valid_ping_delay0_19;
assign b_data_valid_ping_delay6_14 = b_data_valid_ping_delay0_14 & b_data_valid_ping_delay0_20;
assign b_data_valid_ping_delay7_14 = b_data_valid_ping_delay0_14 & b_data_valid_ping_delay0_21;
assign b_data_valid_ping_delay8_14 = b_data_valid_ping_delay0_14 & b_data_valid_ping_delay0_22;
assign b_data_valid_ping_delay9_14 = b_data_valid_ping_delay0_14 & b_data_valid_ping_delay0_23;
assign b_data_valid_ping_delay10_14 = b_data_valid_ping_delay0_14 & b_data_valid_ping_delay0_24;
assign b_data_valid_ping_delay11_14 = b_data_valid_ping_delay0_14 & b_data_valid_ping_delay0_25;
assign b_data_valid_ping_delay12_14 = b_data_valid_ping_delay0_14 & b_data_valid_ping_delay0_26;
assign b_data_valid_ping_delay13_14 = b_data_valid_ping_delay0_14 & b_data_valid_ping_delay0_27;
assign b_data_valid_ping_delay14_14 = b_data_valid_ping_delay0_14 & b_data_valid_ping_delay0_28;
assign b_data_valid_ping_delay15_14 = b_data_valid_ping_delay0_14 & b_data_valid_ping_delay0_29;
assign b_data_valid_ping_delay1_15 = b_data_valid_ping_delay0_15 & b_data_valid_ping_delay0_16;
assign b_data_valid_ping_delay2_15 = b_data_valid_ping_delay0_15 & b_data_valid_ping_delay0_17;
assign b_data_valid_ping_delay3_15 = b_data_valid_ping_delay0_15 & b_data_valid_ping_delay0_18;
assign b_data_valid_ping_delay4_15 = b_data_valid_ping_delay0_15 & b_data_valid_ping_delay0_19;
assign b_data_valid_ping_delay5_15 = b_data_valid_ping_delay0_15 & b_data_valid_ping_delay0_20;
assign b_data_valid_ping_delay6_15 = b_data_valid_ping_delay0_15 & b_data_valid_ping_delay0_21;
assign b_data_valid_ping_delay7_15 = b_data_valid_ping_delay0_15 & b_data_valid_ping_delay0_22;
assign b_data_valid_ping_delay8_15 = b_data_valid_ping_delay0_15 & b_data_valid_ping_delay0_23;
assign b_data_valid_ping_delay9_15 = b_data_valid_ping_delay0_15 & b_data_valid_ping_delay0_24;
assign b_data_valid_ping_delay10_15 = b_data_valid_ping_delay0_15 & b_data_valid_ping_delay0_25;
assign b_data_valid_ping_delay11_15 = b_data_valid_ping_delay0_15 & b_data_valid_ping_delay0_26;
assign b_data_valid_ping_delay12_15 = b_data_valid_ping_delay0_15 & b_data_valid_ping_delay0_27;
assign b_data_valid_ping_delay13_15 = b_data_valid_ping_delay0_15 & b_data_valid_ping_delay0_28;
assign b_data_valid_ping_delay14_15 = b_data_valid_ping_delay0_15 & b_data_valid_ping_delay0_29;
assign b_data_valid_ping_delay15_15 = b_data_valid_ping_delay0_15 & b_data_valid_ping_delay0_30;

wire [`DWIDTH-1:0] in_a_0_0_NC, in_a_0_1_NC, in_a_0_2_NC, in_a_0_3_NC, in_a_0_4_NC, in_a_0_5_NC, in_a_0_6_NC, in_a_0_7_NC, in_a_0_8_NC, in_a_0_9_NC, in_a_0_10_NC, in_a_0_11_NC, in_a_0_12_NC, in_a_0_13_NC, in_a_0_14_NC, in_a_0_15_NC, in_a_1_0_NC, in_a_1_1_NC, in_a_1_2_NC, in_a_1_3_NC, in_a_1_4_NC, in_a_1_5_NC, in_a_1_6_NC, in_a_1_7_NC, in_a_1_8_NC, in_a_1_9_NC, in_a_1_10_NC, in_a_1_11_NC, in_a_1_12_NC, in_a_1_13_NC, in_a_1_14_NC, in_a_1_15_NC, in_a_2_0_NC, in_a_2_1_NC, in_a_2_2_NC, in_a_2_3_NC, in_a_2_4_NC, in_a_2_5_NC, in_a_2_6_NC, in_a_2_7_NC, in_a_2_8_NC, in_a_2_9_NC, in_a_2_10_NC, in_a_2_11_NC, in_a_2_12_NC, in_a_2_13_NC, in_a_2_14_NC, in_a_2_15_NC, in_a_3_0_NC, in_a_3_1_NC, in_a_3_2_NC, in_a_3_3_NC, in_a_3_4_NC, in_a_3_5_NC, in_a_3_6_NC, in_a_3_7_NC, in_a_3_8_NC, in_a_3_9_NC, in_a_3_10_NC, in_a_3_11_NC, in_a_3_12_NC, in_a_3_13_NC, in_a_3_14_NC, in_a_3_15_NC, in_a_4_0_NC, in_a_4_1_NC, in_a_4_2_NC, in_a_4_3_NC, in_a_4_4_NC, in_a_4_5_NC, in_a_4_6_NC, in_a_4_7_NC, in_a_4_8_NC, in_a_4_9_NC, in_a_4_10_NC, in_a_4_11_NC, in_a_4_12_NC, in_a_4_13_NC, in_a_4_14_NC, in_a_4_15_NC, in_a_5_0_NC, in_a_5_1_NC, in_a_5_2_NC, in_a_5_3_NC, in_a_5_4_NC, in_a_5_5_NC, in_a_5_6_NC, in_a_5_7_NC, in_a_5_8_NC, in_a_5_9_NC, in_a_5_10_NC, in_a_5_11_NC, in_a_5_12_NC, in_a_5_13_NC, in_a_5_14_NC, in_a_5_15_NC, in_a_6_0_NC, in_a_6_1_NC, in_a_6_2_NC, in_a_6_3_NC, in_a_6_4_NC, in_a_6_5_NC, in_a_6_6_NC, in_a_6_7_NC, in_a_6_8_NC, in_a_6_9_NC, in_a_6_10_NC, in_a_6_11_NC, in_a_6_12_NC, in_a_6_13_NC, in_a_6_14_NC, in_a_6_15_NC, in_a_7_0_NC, in_a_7_1_NC, in_a_7_2_NC, in_a_7_3_NC, in_a_7_4_NC, in_a_7_5_NC, in_a_7_6_NC, in_a_7_7_NC, in_a_7_8_NC, in_a_7_9_NC, in_a_7_10_NC, in_a_7_11_NC, in_a_7_12_NC, in_a_7_13_NC, in_a_7_14_NC, in_a_7_15_NC, in_a_8_0_NC, in_a_8_1_NC, in_a_8_2_NC, in_a_8_3_NC, in_a_8_4_NC, in_a_8_5_NC, in_a_8_6_NC, in_a_8_7_NC, in_a_8_8_NC, in_a_8_9_NC, in_a_8_10_NC, in_a_8_11_NC, in_a_8_12_NC, in_a_8_13_NC, in_a_8_14_NC, in_a_8_15_NC, in_a_9_0_NC, in_a_9_1_NC, in_a_9_2_NC, in_a_9_3_NC, in_a_9_4_NC, in_a_9_5_NC, in_a_9_6_NC, in_a_9_7_NC, in_a_9_8_NC, in_a_9_9_NC, in_a_9_10_NC, in_a_9_11_NC, in_a_9_12_NC, in_a_9_13_NC, in_a_9_14_NC, in_a_9_15_NC, in_a_10_0_NC, in_a_10_1_NC, in_a_10_2_NC, in_a_10_3_NC, in_a_10_4_NC, in_a_10_5_NC, in_a_10_6_NC, in_a_10_7_NC, in_a_10_8_NC, in_a_10_9_NC, in_a_10_10_NC, in_a_10_11_NC, in_a_10_12_NC, in_a_10_13_NC, in_a_10_14_NC, in_a_10_15_NC, in_a_11_0_NC, in_a_11_1_NC, in_a_11_2_NC, in_a_11_3_NC, in_a_11_4_NC, in_a_11_5_NC, in_a_11_6_NC, in_a_11_7_NC, in_a_11_8_NC, in_a_11_9_NC, in_a_11_10_NC, in_a_11_11_NC, in_a_11_12_NC, in_a_11_13_NC, in_a_11_14_NC, in_a_11_15_NC, in_a_12_0_NC, in_a_12_1_NC, in_a_12_2_NC, in_a_12_3_NC, in_a_12_4_NC, in_a_12_5_NC, in_a_12_6_NC, in_a_12_7_NC, in_a_12_8_NC, in_a_12_9_NC, in_a_12_10_NC, in_a_12_11_NC, in_a_12_12_NC, in_a_12_13_NC, in_a_12_14_NC, in_a_12_15_NC, in_a_13_0_NC, in_a_13_1_NC, in_a_13_2_NC, in_a_13_3_NC, in_a_13_4_NC, in_a_13_5_NC, in_a_13_6_NC, in_a_13_7_NC, in_a_13_8_NC, in_a_13_9_NC, in_a_13_10_NC, in_a_13_11_NC, in_a_13_12_NC, in_a_13_13_NC, in_a_13_14_NC, in_a_13_15_NC, in_a_14_0_NC, in_a_14_1_NC, in_a_14_2_NC, in_a_14_3_NC, in_a_14_4_NC, in_a_14_5_NC, in_a_14_6_NC, in_a_14_7_NC, in_a_14_8_NC, in_a_14_9_NC, in_a_14_10_NC, in_a_14_11_NC, in_a_14_12_NC, in_a_14_13_NC, in_a_14_14_NC, in_a_14_15_NC, in_a_15_0_NC, in_a_15_1_NC, in_a_15_2_NC, in_a_15_3_NC, in_a_15_4_NC, in_a_15_5_NC, in_a_15_6_NC, in_a_15_7_NC, in_a_15_8_NC, in_a_15_9_NC, in_a_15_10_NC, in_a_15_11_NC, in_a_15_12_NC, in_a_15_13_NC, in_a_15_14_NC, in_a_15_15_NC;

wire [`DWIDTH-1:0] in_a_chain_0_0_NC, in_a_chain_0_1_NC, in_a_chain_0_2_NC, in_a_chain_0_3_NC, in_a_chain_0_4_NC, in_a_chain_0_5_NC, in_a_chain_0_6_NC, in_a_chain_0_7_NC, in_a_chain_0_8_NC, in_a_chain_0_9_NC, in_a_chain_0_10_NC, in_a_chain_0_11_NC, in_a_chain_0_12_NC, in_a_chain_0_13_NC, in_a_chain_0_14_NC, in_a_chain_0_15_NC, in_a_chain_1_0_NC, in_a_chain_1_1_NC, in_a_chain_1_2_NC, in_a_chain_1_3_NC, in_a_chain_1_4_NC, in_a_chain_1_5_NC, in_a_chain_1_6_NC, in_a_chain_1_7_NC, in_a_chain_1_8_NC, in_a_chain_1_9_NC, in_a_chain_1_10_NC, in_a_chain_1_11_NC, in_a_chain_1_12_NC, in_a_chain_1_13_NC, in_a_chain_1_14_NC, in_a_chain_1_15_NC, in_a_chain_2_0_NC, in_a_chain_2_1_NC, in_a_chain_2_2_NC, in_a_chain_2_3_NC, in_a_chain_2_4_NC, in_a_chain_2_5_NC, in_a_chain_2_6_NC, in_a_chain_2_7_NC, in_a_chain_2_8_NC, in_a_chain_2_9_NC, in_a_chain_2_10_NC, in_a_chain_2_11_NC, in_a_chain_2_12_NC, in_a_chain_2_13_NC, in_a_chain_2_14_NC, in_a_chain_2_15_NC, in_a_chain_3_0_NC, in_a_chain_3_1_NC, in_a_chain_3_2_NC, in_a_chain_3_3_NC, in_a_chain_3_4_NC, in_a_chain_3_5_NC, in_a_chain_3_6_NC, in_a_chain_3_7_NC, in_a_chain_3_8_NC, in_a_chain_3_9_NC, in_a_chain_3_10_NC, in_a_chain_3_11_NC, in_a_chain_3_12_NC, in_a_chain_3_13_NC, in_a_chain_3_14_NC, in_a_chain_3_15_NC, in_a_chain_4_0_NC, in_a_chain_4_1_NC, in_a_chain_4_2_NC, in_a_chain_4_3_NC, in_a_chain_4_4_NC, in_a_chain_4_5_NC, in_a_chain_4_6_NC, in_a_chain_4_7_NC, in_a_chain_4_8_NC, in_a_chain_4_9_NC, in_a_chain_4_10_NC, in_a_chain_4_11_NC, in_a_chain_4_12_NC, in_a_chain_4_13_NC, in_a_chain_4_14_NC, in_a_chain_4_15_NC, in_a_chain_5_0_NC, in_a_chain_5_1_NC, in_a_chain_5_2_NC, in_a_chain_5_3_NC, in_a_chain_5_4_NC, in_a_chain_5_5_NC, in_a_chain_5_6_NC, in_a_chain_5_7_NC, in_a_chain_5_8_NC, in_a_chain_5_9_NC, in_a_chain_5_10_NC, in_a_chain_5_11_NC, in_a_chain_5_12_NC, in_a_chain_5_13_NC, in_a_chain_5_14_NC, in_a_chain_5_15_NC, in_a_chain_6_0_NC, in_a_chain_6_1_NC, in_a_chain_6_2_NC, in_a_chain_6_3_NC, in_a_chain_6_4_NC, in_a_chain_6_5_NC, in_a_chain_6_6_NC, in_a_chain_6_7_NC, in_a_chain_6_8_NC, in_a_chain_6_9_NC, in_a_chain_6_10_NC, in_a_chain_6_11_NC, in_a_chain_6_12_NC, in_a_chain_6_13_NC, in_a_chain_6_14_NC, in_a_chain_6_15_NC, in_a_chain_7_0_NC, in_a_chain_7_1_NC, in_a_chain_7_2_NC, in_a_chain_7_3_NC, in_a_chain_7_4_NC, in_a_chain_7_5_NC, in_a_chain_7_6_NC, in_a_chain_7_7_NC, in_a_chain_7_8_NC, in_a_chain_7_9_NC, in_a_chain_7_10_NC, in_a_chain_7_11_NC, in_a_chain_7_12_NC, in_a_chain_7_13_NC, in_a_chain_7_14_NC, in_a_chain_7_15_NC, in_a_chain_8_0_NC, in_a_chain_8_1_NC, in_a_chain_8_2_NC, in_a_chain_8_3_NC, in_a_chain_8_4_NC, in_a_chain_8_5_NC, in_a_chain_8_6_NC, in_a_chain_8_7_NC, in_a_chain_8_8_NC, in_a_chain_8_9_NC, in_a_chain_8_10_NC, in_a_chain_8_11_NC, in_a_chain_8_12_NC, in_a_chain_8_13_NC, in_a_chain_8_14_NC, in_a_chain_8_15_NC, in_a_chain_9_0_NC, in_a_chain_9_1_NC, in_a_chain_9_2_NC, in_a_chain_9_3_NC, in_a_chain_9_4_NC, in_a_chain_9_5_NC, in_a_chain_9_6_NC, in_a_chain_9_7_NC, in_a_chain_9_8_NC, in_a_chain_9_9_NC, in_a_chain_9_10_NC, in_a_chain_9_11_NC, in_a_chain_9_12_NC, in_a_chain_9_13_NC, in_a_chain_9_14_NC, in_a_chain_9_15_NC, in_a_chain_10_0_NC, in_a_chain_10_1_NC, in_a_chain_10_2_NC, in_a_chain_10_3_NC, in_a_chain_10_4_NC, in_a_chain_10_5_NC, in_a_chain_10_6_NC, in_a_chain_10_7_NC, in_a_chain_10_8_NC, in_a_chain_10_9_NC, in_a_chain_10_10_NC, in_a_chain_10_11_NC, in_a_chain_10_12_NC, in_a_chain_10_13_NC, in_a_chain_10_14_NC, in_a_chain_10_15_NC, in_a_chain_11_0_NC, in_a_chain_11_1_NC, in_a_chain_11_2_NC, in_a_chain_11_3_NC, in_a_chain_11_4_NC, in_a_chain_11_5_NC, in_a_chain_11_6_NC, in_a_chain_11_7_NC, in_a_chain_11_8_NC, in_a_chain_11_9_NC, in_a_chain_11_10_NC, in_a_chain_11_11_NC, in_a_chain_11_12_NC, in_a_chain_11_13_NC, in_a_chain_11_14_NC, in_a_chain_11_15_NC, in_a_chain_12_0_NC, in_a_chain_12_1_NC, in_a_chain_12_2_NC, in_a_chain_12_3_NC, in_a_chain_12_4_NC, in_a_chain_12_5_NC, in_a_chain_12_6_NC, in_a_chain_12_7_NC, in_a_chain_12_8_NC, in_a_chain_12_9_NC, in_a_chain_12_10_NC, in_a_chain_12_11_NC, in_a_chain_12_12_NC, in_a_chain_12_13_NC, in_a_chain_12_14_NC, in_a_chain_12_15_NC, in_a_chain_13_0_NC, in_a_chain_13_1_NC, in_a_chain_13_2_NC, in_a_chain_13_3_NC, in_a_chain_13_4_NC, in_a_chain_13_5_NC, in_a_chain_13_6_NC, in_a_chain_13_7_NC, in_a_chain_13_8_NC, in_a_chain_13_9_NC, in_a_chain_13_10_NC, in_a_chain_13_11_NC, in_a_chain_13_12_NC, in_a_chain_13_13_NC, in_a_chain_13_14_NC, in_a_chain_13_15_NC, in_a_chain_14_0_NC, in_a_chain_14_1_NC, in_a_chain_14_2_NC, in_a_chain_14_3_NC, in_a_chain_14_4_NC, in_a_chain_14_5_NC, in_a_chain_14_6_NC, in_a_chain_14_7_NC, in_a_chain_14_8_NC, in_a_chain_14_9_NC, in_a_chain_14_10_NC, in_a_chain_14_11_NC, in_a_chain_14_12_NC, in_a_chain_14_13_NC, in_a_chain_14_14_NC, in_a_chain_14_15_NC, in_a_chain_15_0_NC, in_a_chain_15_1_NC, in_a_chain_15_2_NC, in_a_chain_15_3_NC, in_a_chain_15_4_NC, in_a_chain_15_5_NC, in_a_chain_15_6_NC, in_a_chain_15_7_NC, in_a_chain_15_8_NC, in_a_chain_15_9_NC, in_a_chain_15_10_NC, in_a_chain_15_11_NC, in_a_chain_15_12_NC, in_a_chain_15_13_NC, in_a_chain_15_14_NC, in_a_chain_15_15_NC;

wire [`DWIDTH-1:0] out_a_0_0_NC, out_a_0_1_NC, out_a_0_2_NC, out_a_0_3_NC, out_a_0_4_NC, out_a_0_5_NC, out_a_0_6_NC, out_a_0_7_NC, out_a_0_8_NC, out_a_0_9_NC, out_a_0_10_NC, out_a_0_11_NC, out_a_0_12_NC, out_a_0_13_NC, out_a_0_14_NC, out_a_0_15_NC, out_a_1_0_NC, out_a_1_1_NC, out_a_1_2_NC, out_a_1_3_NC, out_a_1_4_NC, out_a_1_5_NC, out_a_1_6_NC, out_a_1_7_NC, out_a_1_8_NC, out_a_1_9_NC, out_a_1_10_NC, out_a_1_11_NC, out_a_1_12_NC, out_a_1_13_NC, out_a_1_14_NC, out_a_1_15_NC, out_a_2_0_NC, out_a_2_1_NC, out_a_2_2_NC, out_a_2_3_NC, out_a_2_4_NC, out_a_2_5_NC, out_a_2_6_NC, out_a_2_7_NC, out_a_2_8_NC, out_a_2_9_NC, out_a_2_10_NC, out_a_2_11_NC, out_a_2_12_NC, out_a_2_13_NC, out_a_2_14_NC, out_a_2_15_NC, out_a_3_0_NC, out_a_3_1_NC, out_a_3_2_NC, out_a_3_3_NC, out_a_3_4_NC, out_a_3_5_NC, out_a_3_6_NC, out_a_3_7_NC, out_a_3_8_NC, out_a_3_9_NC, out_a_3_10_NC, out_a_3_11_NC, out_a_3_12_NC, out_a_3_13_NC, out_a_3_14_NC, out_a_3_15_NC, out_a_4_0_NC, out_a_4_1_NC, out_a_4_2_NC, out_a_4_3_NC, out_a_4_4_NC, out_a_4_5_NC, out_a_4_6_NC, out_a_4_7_NC, out_a_4_8_NC, out_a_4_9_NC, out_a_4_10_NC, out_a_4_11_NC, out_a_4_12_NC, out_a_4_13_NC, out_a_4_14_NC, out_a_4_15_NC, out_a_5_0_NC, out_a_5_1_NC, out_a_5_2_NC, out_a_5_3_NC, out_a_5_4_NC, out_a_5_5_NC, out_a_5_6_NC, out_a_5_7_NC, out_a_5_8_NC, out_a_5_9_NC, out_a_5_10_NC, out_a_5_11_NC, out_a_5_12_NC, out_a_5_13_NC, out_a_5_14_NC, out_a_5_15_NC, out_a_6_0_NC, out_a_6_1_NC, out_a_6_2_NC, out_a_6_3_NC, out_a_6_4_NC, out_a_6_5_NC, out_a_6_6_NC, out_a_6_7_NC, out_a_6_8_NC, out_a_6_9_NC, out_a_6_10_NC, out_a_6_11_NC, out_a_6_12_NC, out_a_6_13_NC, out_a_6_14_NC, out_a_6_15_NC, out_a_7_0_NC, out_a_7_1_NC, out_a_7_2_NC, out_a_7_3_NC, out_a_7_4_NC, out_a_7_5_NC, out_a_7_6_NC, out_a_7_7_NC, out_a_7_8_NC, out_a_7_9_NC, out_a_7_10_NC, out_a_7_11_NC, out_a_7_12_NC, out_a_7_13_NC, out_a_7_14_NC, out_a_7_15_NC, out_a_8_0_NC, out_a_8_1_NC, out_a_8_2_NC, out_a_8_3_NC, out_a_8_4_NC, out_a_8_5_NC, out_a_8_6_NC, out_a_8_7_NC, out_a_8_8_NC, out_a_8_9_NC, out_a_8_10_NC, out_a_8_11_NC, out_a_8_12_NC, out_a_8_13_NC, out_a_8_14_NC, out_a_8_15_NC, out_a_9_0_NC, out_a_9_1_NC, out_a_9_2_NC, out_a_9_3_NC, out_a_9_4_NC, out_a_9_5_NC, out_a_9_6_NC, out_a_9_7_NC, out_a_9_8_NC, out_a_9_9_NC, out_a_9_10_NC, out_a_9_11_NC, out_a_9_12_NC, out_a_9_13_NC, out_a_9_14_NC, out_a_9_15_NC, out_a_10_0_NC, out_a_10_1_NC, out_a_10_2_NC, out_a_10_3_NC, out_a_10_4_NC, out_a_10_5_NC, out_a_10_6_NC, out_a_10_7_NC, out_a_10_8_NC, out_a_10_9_NC, out_a_10_10_NC, out_a_10_11_NC, out_a_10_12_NC, out_a_10_13_NC, out_a_10_14_NC, out_a_10_15_NC, out_a_11_0_NC, out_a_11_1_NC, out_a_11_2_NC, out_a_11_3_NC, out_a_11_4_NC, out_a_11_5_NC, out_a_11_6_NC, out_a_11_7_NC, out_a_11_8_NC, out_a_11_9_NC, out_a_11_10_NC, out_a_11_11_NC, out_a_11_12_NC, out_a_11_13_NC, out_a_11_14_NC, out_a_11_15_NC, out_a_12_0_NC, out_a_12_1_NC, out_a_12_2_NC, out_a_12_3_NC, out_a_12_4_NC, out_a_12_5_NC, out_a_12_6_NC, out_a_12_7_NC, out_a_12_8_NC, out_a_12_9_NC, out_a_12_10_NC, out_a_12_11_NC, out_a_12_12_NC, out_a_12_13_NC, out_a_12_14_NC, out_a_12_15_NC, out_a_13_0_NC, out_a_13_1_NC, out_a_13_2_NC, out_a_13_3_NC, out_a_13_4_NC, out_a_13_5_NC, out_a_13_6_NC, out_a_13_7_NC, out_a_13_8_NC, out_a_13_9_NC, out_a_13_10_NC, out_a_13_11_NC, out_a_13_12_NC, out_a_13_13_NC, out_a_13_14_NC, out_a_13_15_NC, out_a_14_0_NC, out_a_14_1_NC, out_a_14_2_NC, out_a_14_3_NC, out_a_14_4_NC, out_a_14_5_NC, out_a_14_6_NC, out_a_14_7_NC, out_a_14_8_NC, out_a_14_9_NC, out_a_14_10_NC, out_a_14_11_NC, out_a_14_12_NC, out_a_14_13_NC, out_a_14_14_NC, out_a_14_15_NC, out_a_15_0_NC, out_a_15_1_NC, out_a_15_2_NC, out_a_15_3_NC, out_a_15_4_NC, out_a_15_5_NC, out_a_15_6_NC, out_a_15_7_NC, out_a_15_8_NC, out_a_15_9_NC, out_a_15_10_NC, out_a_15_11_NC, out_a_15_12_NC, out_a_15_13_NC, out_a_15_14_NC, out_a_15_15_NC;

processing_element pe0_0(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel),             .in_a(a0),    .in_a_chain(in_a_chain_0_0_NC),  .in_b(b0),      .in_c(c0),        .out_a(out_a_0_0_NC), .out_a_chain(a0_0to0_1), .out_b(b0_0to1_0), .out_b0(b0_0to1_0_ping), .out_b1(b0_0to1_0_pong), .out_c(matrixC0_0), .b_data_valid_ping(b_data_valid_ping),         .b_data_valid_pong(b_data_valid_pong        ), .mode(1'b1));
processing_element pe0_1(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay1), .in_a(in_a_0_1_NC), .in_a_chain(a0_0to0_1), .in_b(b1),      .in_c(c1),        .out_a(out_a_0_1_NC), .out_a_chain(a0_1to0_2), .out_b(b0_1to1_1), .out_b0(b0_1to1_1_ping), .out_b1(b0_1to1_1_pong), .out_c(matrixC0_1), .b_data_valid_ping(b_data_valid_ping_delay0_1), .b_data_valid_pong(b_data_valid_pong_delay0_1), .mode(1'b0));
processing_element pe0_2(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay2), .in_a(in_a_0_2_NC), .in_a_chain(a0_1to0_2), .in_b(b2),      .in_c(c2),        .out_a(out_a_0_2_NC), .out_a_chain(a0_2to0_3), .out_b(b0_2to1_2), .out_b0(b0_2to1_2_ping), .out_b1(b0_2to1_2_pong), .out_c(matrixC0_2), .b_data_valid_ping(b_data_valid_ping_delay0_2), .b_data_valid_pong(b_data_valid_pong_delay0_2), .mode(1'b0));
processing_element pe0_3(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay3), .in_a(in_a_0_3_NC), .in_a_chain(a0_2to0_3), .in_b(b3),      .in_c(c3),        .out_a(out_a_0_3_NC), .out_a_chain(a0_3to0_4), .out_b(b0_3to1_3), .out_b0(b0_3to1_3_ping), .out_b1(b0_3to1_3_pong), .out_c(matrixC0_3), .b_data_valid_ping(b_data_valid_ping_delay0_3), .b_data_valid_pong(b_data_valid_pong_delay0_3), .mode(1'b0));
processing_element pe0_4(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay4), .in_a(in_a_0_4_NC), .in_a_chain(a0_3to0_4), .in_b(b4),      .in_c(c4),        .out_a(out_a_0_4_NC), .out_a_chain(a0_4to0_5), .out_b(b0_4to1_4), .out_b0(b0_4to1_4_ping), .out_b1(b0_4to1_4_pong), .out_c(matrixC0_4), .b_data_valid_ping(b_data_valid_ping_delay0_4), .b_data_valid_pong(b_data_valid_pong_delay0_4), .mode(1'b0));
processing_element pe0_5(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay5), .in_a(in_a_0_5_NC), .in_a_chain(a0_4to0_5), .in_b(b5),      .in_c(c5),        .out_a(out_a_0_5_NC), .out_a_chain(a0_5to0_6), .out_b(b0_5to1_5), .out_b0(b0_5to1_5_ping), .out_b1(b0_5to1_5_pong), .out_c(matrixC0_5), .b_data_valid_ping(b_data_valid_ping_delay0_5), .b_data_valid_pong(b_data_valid_pong_delay0_5), .mode(1'b0));
processing_element pe0_6(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay6), .in_a(in_a_0_6_NC), .in_a_chain(a0_5to0_6), .in_b(b6),      .in_c(c6),        .out_a(out_a_0_6_NC), .out_a_chain(a0_6to0_7), .out_b(b0_6to1_6), .out_b0(b0_6to1_6_ping), .out_b1(b0_6to1_6_pong), .out_c(matrixC0_6), .b_data_valid_ping(b_data_valid_ping_delay0_6), .b_data_valid_pong(b_data_valid_pong_delay0_6), .mode(1'b0));
processing_element pe0_7(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay7), .in_a(in_a_0_7_NC), .in_a_chain(a0_6to0_7), .in_b(b7),      .in_c(c7),        .out_a(out_a_0_7_NC), .out_a_chain(a0_7to0_8), .out_b(b0_7to1_7), .out_b0(b0_7to1_7_ping), .out_b1(b0_7to1_7_pong), .out_c(matrixC0_7), .b_data_valid_ping(b_data_valid_ping_delay0_7), .b_data_valid_pong(b_data_valid_pong_delay0_7), .mode(1'b0));
processing_element pe0_8(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay8), .in_a(in_a_0_8_NC), .in_a_chain(a0_7to0_8), .in_b(b8),      .in_c(c8),        .out_a(out_a_0_8_NC), .out_a_chain(a0_8to0_9), .out_b(b0_8to1_8), .out_b0(b0_8to1_8_ping), .out_b1(b0_8to1_8_pong), .out_c(matrixC0_8), .b_data_valid_ping(b_data_valid_ping_delay0_8), .b_data_valid_pong(b_data_valid_pong_delay0_8), .mode(1'b0));
processing_element pe0_9(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay9), .in_a(in_a_0_9_NC), .in_a_chain(a0_8to0_9), .in_b(b9),      .in_c(c9),        .out_a(out_a_0_9_NC), .out_a_chain(a0_9to0_10), .out_b(b0_9to1_9), .out_b0(b0_9to1_9_ping), .out_b1(b0_9to1_9_pong), .out_c(matrixC0_9), .b_data_valid_ping(b_data_valid_ping_delay0_9), .b_data_valid_pong(b_data_valid_pong_delay0_9), .mode(1'b0));
processing_element pe0_10(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay10), .in_a(in_a_0_10_NC), .in_a_chain(a0_9to0_10), .in_b(b10),      .in_c(c10),        .out_a(out_a_0_10_NC), .out_a_chain(a0_10to0_11), .out_b(b0_10to1_10), .out_b0(b0_10to1_10_ping), .out_b1(b0_10to1_10_pong), .out_c(matrixC0_10), .b_data_valid_ping(b_data_valid_ping_delay0_10), .b_data_valid_pong(b_data_valid_pong_delay0_10), .mode(1'b0));
processing_element pe0_11(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay11), .in_a(in_a_0_11_NC), .in_a_chain(a0_10to0_11), .in_b(b11),      .in_c(c11),        .out_a(out_a_0_11_NC), .out_a_chain(a0_11to0_12), .out_b(b0_11to1_11), .out_b0(b0_11to1_11_ping), .out_b1(b0_11to1_11_pong), .out_c(matrixC0_11), .b_data_valid_ping(b_data_valid_ping_delay0_11), .b_data_valid_pong(b_data_valid_pong_delay0_11), .mode(1'b0));
processing_element pe0_12(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay12), .in_a(in_a_0_12_NC), .in_a_chain(a0_11to0_12), .in_b(b12),      .in_c(c12),        .out_a(out_a_0_12_NC), .out_a_chain(a0_12to0_13), .out_b(b0_12to1_12), .out_b0(b0_12to1_12_ping), .out_b1(b0_12to1_12_pong), .out_c(matrixC0_12), .b_data_valid_ping(b_data_valid_ping_delay0_12), .b_data_valid_pong(b_data_valid_pong_delay0_12), .mode(1'b0));
processing_element pe0_13(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay13), .in_a(in_a_0_13_NC), .in_a_chain(a0_12to0_13), .in_b(b13),      .in_c(c13),        .out_a(out_a_0_13_NC), .out_a_chain(a0_13to0_14), .out_b(b0_13to1_13), .out_b0(b0_13to1_13_ping), .out_b1(b0_13to1_13_pong), .out_c(matrixC0_13), .b_data_valid_ping(b_data_valid_ping_delay0_13), .b_data_valid_pong(b_data_valid_pong_delay0_13), .mode(1'b0));
processing_element pe0_14(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay14), .in_a(in_a_0_14_NC), .in_a_chain(a0_13to0_14), .in_b(b14),      .in_c(c14),        .out_a(out_a_0_14_NC), .out_a_chain(a0_14to0_15), .out_b(b0_14to1_14), .out_b0(b0_14to1_14_ping), .out_b1(b0_14to1_14_pong), .out_c(matrixC0_14), .b_data_valid_ping(b_data_valid_ping_delay0_14), .b_data_valid_pong(b_data_valid_pong_delay0_14), .mode(1'b0));
processing_element pe0_15(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay15), .in_a(in_a_0_15_NC), .in_a_chain(a0_14to0_15), .in_b(b15),      .in_c(c15),        .out_a(out_a_0_15_NC), .out_a_chain(a0_15to0_16), .out_b(b0_15to1_15), .out_b0(b0_15to1_15_ping), .out_b1(b0_15to1_15_pong), .out_c(matrixC0_15), .b_data_valid_ping(b_data_valid_ping_delay0_15), .b_data_valid_pong(b_data_valid_pong_delay0_15), .mode(1'b0));
processing_element pe1_0(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay1), .in_a(a1),   .in_a_chain(in_a_chain_1_0_NC),   .in_b(b0_0to1_0), .in_c(matrixC0_0), .out_a(out_a_1_0_NC), .out_a_chain(a1_0to1_1), .out_b(b1_0to2_0), .out_b0(b1_0to2_0_ping), .out_b1(b1_0to2_0_pong), .out_c(matrixC1_0), .b_data_valid_ping(b_data_valid_ping_delay1_0), .b_data_valid_pong(b_data_valid_pong_delay1_0), .mode(1'b1));
processing_element pe1_1(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay2), .in_a(in_a_1_1_NC),  .in_a_chain(a1_0to1_1), .in_b(b0_1to1_1), .in_c(matrixC0_1), .out_a(out_a_1_1_NC), .out_a_chain(a1_1to1_2), .out_b(b1_1to2_1), .out_b0(b1_1to2_1_ping), .out_b1(b1_1to2_1_pong), .out_c(matrixC1_1), .b_data_valid_ping(b_data_valid_ping_delay1_1), .b_data_valid_pong(b_data_valid_pong_delay1_1), .mode(1'b0));
processing_element pe1_2(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay3), .in_a(in_a_1_2_NC),  .in_a_chain(a1_1to1_2), .in_b(b0_2to1_2), .in_c(matrixC0_2), .out_a(out_a_1_2_NC), .out_a_chain(a1_2to1_3), .out_b(b1_2to2_2), .out_b0(b1_2to2_2_ping), .out_b1(b1_2to2_2_pong), .out_c(matrixC1_2), .b_data_valid_ping(b_data_valid_ping_delay1_2), .b_data_valid_pong(b_data_valid_pong_delay1_2), .mode(1'b0));
processing_element pe1_3(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay4), .in_a(in_a_1_3_NC),  .in_a_chain(a1_2to1_3), .in_b(b0_3to1_3), .in_c(matrixC0_3), .out_a(out_a_1_3_NC), .out_a_chain(a1_3to1_4), .out_b(b1_3to2_3), .out_b0(b1_3to2_3_ping), .out_b1(b1_3to2_3_pong), .out_c(matrixC1_3), .b_data_valid_ping(b_data_valid_ping_delay1_3), .b_data_valid_pong(b_data_valid_pong_delay1_3), .mode(1'b0));
processing_element pe1_4(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay5), .in_a(in_a_1_4_NC),  .in_a_chain(a1_3to1_4), .in_b(b0_4to1_4), .in_c(matrixC0_4), .out_a(out_a_1_4_NC), .out_a_chain(a1_4to1_5), .out_b(b1_4to2_4), .out_b0(b1_4to2_4_ping), .out_b1(b1_4to2_4_pong), .out_c(matrixC1_4), .b_data_valid_ping(b_data_valid_ping_delay1_4), .b_data_valid_pong(b_data_valid_pong_delay1_4), .mode(1'b0));
processing_element pe1_5(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay6), .in_a(in_a_1_5_NC),  .in_a_chain(a1_4to1_5), .in_b(b0_5to1_5), .in_c(matrixC0_5), .out_a(out_a_1_5_NC), .out_a_chain(a1_5to1_6), .out_b(b1_5to2_5), .out_b0(b1_5to2_5_ping), .out_b1(b1_5to2_5_pong), .out_c(matrixC1_5), .b_data_valid_ping(b_data_valid_ping_delay1_5), .b_data_valid_pong(b_data_valid_pong_delay1_5), .mode(1'b0));
processing_element pe1_6(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay7), .in_a(in_a_1_6_NC),  .in_a_chain(a1_5to1_6), .in_b(b0_6to1_6), .in_c(matrixC0_6), .out_a(out_a_1_6_NC), .out_a_chain(a1_6to1_7), .out_b(b1_6to2_6), .out_b0(b1_6to2_6_ping), .out_b1(b1_6to2_6_pong), .out_c(matrixC1_6), .b_data_valid_ping(b_data_valid_ping_delay1_6), .b_data_valid_pong(b_data_valid_pong_delay1_6), .mode(1'b0));
processing_element pe1_7(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay8), .in_a(in_a_1_7_NC),  .in_a_chain(a1_6to1_7), .in_b(b0_7to1_7), .in_c(matrixC0_7), .out_a(out_a_1_7_NC), .out_a_chain(a1_7to1_8), .out_b(b1_7to2_7), .out_b0(b1_7to2_7_ping), .out_b1(b1_7to2_7_pong), .out_c(matrixC1_7), .b_data_valid_ping(b_data_valid_ping_delay1_7), .b_data_valid_pong(b_data_valid_pong_delay1_7), .mode(1'b0));
processing_element pe1_8(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay9), .in_a(in_a_1_8_NC),  .in_a_chain(a1_7to1_8), .in_b(b0_8to1_8), .in_c(matrixC0_8), .out_a(out_a_1_8_NC), .out_a_chain(a1_8to1_9), .out_b(b1_8to2_8), .out_b0(b1_8to2_8_ping), .out_b1(b1_8to2_8_pong), .out_c(matrixC1_8), .b_data_valid_ping(b_data_valid_ping_delay1_8), .b_data_valid_pong(b_data_valid_pong_delay1_8), .mode(1'b0));
processing_element pe1_9(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay10), .in_a(in_a_1_9_NC),  .in_a_chain(a1_8to1_9), .in_b(b0_9to1_9), .in_c(matrixC0_9), .out_a(out_a_1_9_NC), .out_a_chain(a1_9to1_10), .out_b(b1_9to2_9), .out_b0(b1_9to2_9_ping), .out_b1(b1_9to2_9_pong), .out_c(matrixC1_9), .b_data_valid_ping(b_data_valid_ping_delay1_9), .b_data_valid_pong(b_data_valid_pong_delay1_9), .mode(1'b0));
processing_element pe1_10(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay11), .in_a(in_a_1_10_NC),  .in_a_chain(a1_9to1_10), .in_b(b0_10to1_10), .in_c(matrixC0_10), .out_a(out_a_1_10_NC), .out_a_chain(a1_10to1_11), .out_b(b1_10to2_10), .out_b0(b1_10to2_10_ping), .out_b1(b1_10to2_10_pong), .out_c(matrixC1_10), .b_data_valid_ping(b_data_valid_ping_delay1_10), .b_data_valid_pong(b_data_valid_pong_delay1_10), .mode(1'b0));
processing_element pe1_11(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay12), .in_a(in_a_1_11_NC),  .in_a_chain(a1_10to1_11), .in_b(b0_11to1_11), .in_c(matrixC0_11), .out_a(out_a_1_11_NC), .out_a_chain(a1_11to1_12), .out_b(b1_11to2_11), .out_b0(b1_11to2_11_ping), .out_b1(b1_11to2_11_pong), .out_c(matrixC1_11), .b_data_valid_ping(b_data_valid_ping_delay1_11), .b_data_valid_pong(b_data_valid_pong_delay1_11), .mode(1'b0));
processing_element pe1_12(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay13), .in_a(in_a_1_12_NC),  .in_a_chain(a1_11to1_12), .in_b(b0_12to1_12), .in_c(matrixC0_12), .out_a(out_a_1_12_NC), .out_a_chain(a1_12to1_13), .out_b(b1_12to2_12), .out_b0(b1_12to2_12_ping), .out_b1(b1_12to2_12_pong), .out_c(matrixC1_12), .b_data_valid_ping(b_data_valid_ping_delay1_12), .b_data_valid_pong(b_data_valid_pong_delay1_12), .mode(1'b0));
processing_element pe1_13(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay14), .in_a(in_a_1_13_NC),  .in_a_chain(a1_12to1_13), .in_b(b0_13to1_13), .in_c(matrixC0_13), .out_a(out_a_1_13_NC), .out_a_chain(a1_13to1_14), .out_b(b1_13to2_13), .out_b0(b1_13to2_13_ping), .out_b1(b1_13to2_13_pong), .out_c(matrixC1_13), .b_data_valid_ping(b_data_valid_ping_delay1_13), .b_data_valid_pong(b_data_valid_pong_delay1_13), .mode(1'b0));
processing_element pe1_14(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay15), .in_a(in_a_1_14_NC),  .in_a_chain(a1_13to1_14), .in_b(b0_14to1_14), .in_c(matrixC0_14), .out_a(out_a_1_14_NC), .out_a_chain(a1_14to1_15), .out_b(b1_14to2_14), .out_b0(b1_14to2_14_ping), .out_b1(b1_14to2_14_pong), .out_c(matrixC1_14), .b_data_valid_ping(b_data_valid_ping_delay1_14), .b_data_valid_pong(b_data_valid_pong_delay1_14), .mode(1'b0));
processing_element pe1_15(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay16), .in_a(in_a_1_15_NC),  .in_a_chain(a1_14to1_15), .in_b(b0_15to1_15), .in_c(matrixC0_15), .out_a(out_a_1_15_NC), .out_a_chain(a1_15to1_16), .out_b(b1_15to2_15), .out_b0(b1_15to2_15_ping), .out_b1(b1_15to2_15_pong), .out_c(matrixC1_15), .b_data_valid_ping(b_data_valid_ping_delay1_15), .b_data_valid_pong(b_data_valid_pong_delay1_15), .mode(1'b0));
processing_element pe2_0(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay2), .in_a(a2),   .in_a_chain(in_a_chain_2_0_NC),   .in_b(b1_0to2_0), .in_c(matrixC1_0), .out_a(out_a_2_0_NC), .out_a_chain(a2_0to2_1), .out_b(b2_0to3_0), .out_b0(b2_0to3_0_ping), .out_b1(b2_0to3_0_pong), .out_c(matrixC2_0), .b_data_valid_ping(b_data_valid_ping_delay2_0), .b_data_valid_pong(b_data_valid_pong_delay2_0), .mode(1'b1));
processing_element pe2_1(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay3), .in_a(in_a_2_1_NC),  .in_a_chain(a2_0to2_1), .in_b(b1_1to2_1), .in_c(matrixC1_1), .out_a(out_a_2_1_NC), .out_a_chain(a2_1to2_2), .out_b(b2_1to3_1), .out_b0(b2_1to3_1_ping), .out_b1(b2_1to3_1_pong), .out_c(matrixC2_1), .b_data_valid_ping(b_data_valid_ping_delay2_1), .b_data_valid_pong(b_data_valid_pong_delay2_1), .mode(1'b0));
processing_element pe2_2(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay4), .in_a(in_a_2_2_NC),  .in_a_chain(a2_1to2_2), .in_b(b1_2to2_2), .in_c(matrixC1_2), .out_a(out_a_2_2_NC), .out_a_chain(a2_2to2_3), .out_b(b2_2to3_2), .out_b0(b2_2to3_2_ping), .out_b1(b2_2to3_2_pong), .out_c(matrixC2_2), .b_data_valid_ping(b_data_valid_ping_delay2_2), .b_data_valid_pong(b_data_valid_pong_delay2_2), .mode(1'b0));
processing_element pe2_3(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay5), .in_a(in_a_2_3_NC),  .in_a_chain(a2_2to2_3), .in_b(b1_3to2_3), .in_c(matrixC1_3), .out_a(out_a_2_3_NC), .out_a_chain(a2_3to2_4), .out_b(b2_3to3_3), .out_b0(b2_3to3_3_ping), .out_b1(b2_3to3_3_pong), .out_c(matrixC2_3), .b_data_valid_ping(b_data_valid_ping_delay2_3), .b_data_valid_pong(b_data_valid_pong_delay2_3), .mode(1'b0));
processing_element pe2_4(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay6), .in_a(in_a_2_4_NC),  .in_a_chain(a2_3to2_4), .in_b(b1_4to2_4), .in_c(matrixC1_4), .out_a(out_a_2_4_NC), .out_a_chain(a2_4to2_5), .out_b(b2_4to3_4), .out_b0(b2_4to3_4_ping), .out_b1(b2_4to3_4_pong), .out_c(matrixC2_4), .b_data_valid_ping(b_data_valid_ping_delay2_4), .b_data_valid_pong(b_data_valid_pong_delay2_4), .mode(1'b0));
processing_element pe2_5(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay7), .in_a(in_a_2_5_NC),  .in_a_chain(a2_4to2_5), .in_b(b1_5to2_5), .in_c(matrixC1_5), .out_a(out_a_2_5_NC), .out_a_chain(a2_5to2_6), .out_b(b2_5to3_5), .out_b0(b2_5to3_5_ping), .out_b1(b2_5to3_5_pong), .out_c(matrixC2_5), .b_data_valid_ping(b_data_valid_ping_delay2_5), .b_data_valid_pong(b_data_valid_pong_delay2_5), .mode(1'b0));
processing_element pe2_6(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay8), .in_a(in_a_2_6_NC),  .in_a_chain(a2_5to2_6), .in_b(b1_6to2_6), .in_c(matrixC1_6), .out_a(out_a_2_6_NC), .out_a_chain(a2_6to2_7), .out_b(b2_6to3_6), .out_b0(b2_6to3_6_ping), .out_b1(b2_6to3_6_pong), .out_c(matrixC2_6), .b_data_valid_ping(b_data_valid_ping_delay2_6), .b_data_valid_pong(b_data_valid_pong_delay2_6), .mode(1'b0));
processing_element pe2_7(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay9), .in_a(in_a_2_7_NC),  .in_a_chain(a2_6to2_7), .in_b(b1_7to2_7), .in_c(matrixC1_7), .out_a(out_a_2_7_NC), .out_a_chain(a2_7to2_8), .out_b(b2_7to3_7), .out_b0(b2_7to3_7_ping), .out_b1(b2_7to3_7_pong), .out_c(matrixC2_7), .b_data_valid_ping(b_data_valid_ping_delay2_7), .b_data_valid_pong(b_data_valid_pong_delay2_7), .mode(1'b0));
processing_element pe2_8(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay10), .in_a(in_a_2_8_NC),  .in_a_chain(a2_7to2_8), .in_b(b1_8to2_8), .in_c(matrixC1_8), .out_a(out_a_2_8_NC), .out_a_chain(a2_8to2_9), .out_b(b2_8to3_8), .out_b0(b2_8to3_8_ping), .out_b1(b2_8to3_8_pong), .out_c(matrixC2_8), .b_data_valid_ping(b_data_valid_ping_delay2_8), .b_data_valid_pong(b_data_valid_pong_delay2_8), .mode(1'b0));
processing_element pe2_9(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay11), .in_a(in_a_2_9_NC),  .in_a_chain(a2_8to2_9), .in_b(b1_9to2_9), .in_c(matrixC1_9), .out_a(out_a_2_9_NC), .out_a_chain(a2_9to2_10), .out_b(b2_9to3_9), .out_b0(b2_9to3_9_ping), .out_b1(b2_9to3_9_pong), .out_c(matrixC2_9), .b_data_valid_ping(b_data_valid_ping_delay2_9), .b_data_valid_pong(b_data_valid_pong_delay2_9), .mode(1'b0));
processing_element pe2_10(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay12), .in_a(in_a_2_10_NC),  .in_a_chain(a2_9to2_10), .in_b(b1_10to2_10), .in_c(matrixC1_10), .out_a(out_a_2_10_NC), .out_a_chain(a2_10to2_11), .out_b(b2_10to3_10), .out_b0(b2_10to3_10_ping), .out_b1(b2_10to3_10_pong), .out_c(matrixC2_10), .b_data_valid_ping(b_data_valid_ping_delay2_10), .b_data_valid_pong(b_data_valid_pong_delay2_10), .mode(1'b0));
processing_element pe2_11(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay13), .in_a(in_a_2_11_NC),  .in_a_chain(a2_10to2_11), .in_b(b1_11to2_11), .in_c(matrixC1_11), .out_a(out_a_2_11_NC), .out_a_chain(a2_11to2_12), .out_b(b2_11to3_11), .out_b0(b2_11to3_11_ping), .out_b1(b2_11to3_11_pong), .out_c(matrixC2_11), .b_data_valid_ping(b_data_valid_ping_delay2_11), .b_data_valid_pong(b_data_valid_pong_delay2_11), .mode(1'b0));
processing_element pe2_12(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay14), .in_a(in_a_2_12_NC),  .in_a_chain(a2_11to2_12), .in_b(b1_12to2_12), .in_c(matrixC1_12), .out_a(out_a_2_12_NC), .out_a_chain(a2_12to2_13), .out_b(b2_12to3_12), .out_b0(b2_12to3_12_ping), .out_b1(b2_12to3_12_pong), .out_c(matrixC2_12), .b_data_valid_ping(b_data_valid_ping_delay2_12), .b_data_valid_pong(b_data_valid_pong_delay2_12), .mode(1'b0));
processing_element pe2_13(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay15), .in_a(in_a_2_13_NC),  .in_a_chain(a2_12to2_13), .in_b(b1_13to2_13), .in_c(matrixC1_13), .out_a(out_a_2_13_NC), .out_a_chain(a2_13to2_14), .out_b(b2_13to3_13), .out_b0(b2_13to3_13_ping), .out_b1(b2_13to3_13_pong), .out_c(matrixC2_13), .b_data_valid_ping(b_data_valid_ping_delay2_13), .b_data_valid_pong(b_data_valid_pong_delay2_13), .mode(1'b0));
processing_element pe2_14(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay16), .in_a(in_a_2_14_NC),  .in_a_chain(a2_13to2_14), .in_b(b1_14to2_14), .in_c(matrixC1_14), .out_a(out_a_2_14_NC), .out_a_chain(a2_14to2_15), .out_b(b2_14to3_14), .out_b0(b2_14to3_14_ping), .out_b1(b2_14to3_14_pong), .out_c(matrixC2_14), .b_data_valid_ping(b_data_valid_ping_delay2_14), .b_data_valid_pong(b_data_valid_pong_delay2_14), .mode(1'b0));
processing_element pe2_15(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay17), .in_a(in_a_2_15_NC),  .in_a_chain(a2_14to2_15), .in_b(b1_15to2_15), .in_c(matrixC1_15), .out_a(out_a_2_15_NC), .out_a_chain(a2_15to2_16), .out_b(b2_15to3_15), .out_b0(b2_15to3_15_ping), .out_b1(b2_15to3_15_pong), .out_c(matrixC2_15), .b_data_valid_ping(b_data_valid_ping_delay2_15), .b_data_valid_pong(b_data_valid_pong_delay2_15), .mode(1'b0));
processing_element pe3_0(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay3), .in_a(a3),   .in_a_chain(in_a_chain_3_0_NC),   .in_b(b2_0to3_0), .in_c(matrixC2_0), .out_a(out_a_3_0_NC), .out_a_chain(a3_0to3_1), .out_b(b3_0to4_0), .out_b0(b3_0to4_0_ping), .out_b1(b3_0to4_0_pong), .out_c(matrixC3_0), .b_data_valid_ping(b_data_valid_ping_delay3_0), .b_data_valid_pong(b_data_valid_pong_delay3_0), .mode(1'b1));
processing_element pe3_1(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay4), .in_a(in_a_3_1_NC),  .in_a_chain(a3_0to3_1), .in_b(b2_1to3_1), .in_c(matrixC2_1), .out_a(out_a_3_1_NC), .out_a_chain(a3_1to3_2), .out_b(b3_1to4_1), .out_b0(b3_1to4_1_ping), .out_b1(b3_1to4_1_pong), .out_c(matrixC3_1), .b_data_valid_ping(b_data_valid_ping_delay3_1), .b_data_valid_pong(b_data_valid_pong_delay3_1), .mode(1'b0));
processing_element pe3_2(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay5), .in_a(in_a_3_2_NC),  .in_a_chain(a3_1to3_2), .in_b(b2_2to3_2), .in_c(matrixC2_2), .out_a(out_a_3_2_NC), .out_a_chain(a3_2to3_3), .out_b(b3_2to4_2), .out_b0(b3_2to4_2_ping), .out_b1(b3_2to4_2_pong), .out_c(matrixC3_2), .b_data_valid_ping(b_data_valid_ping_delay3_2), .b_data_valid_pong(b_data_valid_pong_delay3_2), .mode(1'b0));
processing_element pe3_3(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay6), .in_a(in_a_3_3_NC),  .in_a_chain(a3_2to3_3), .in_b(b2_3to3_3), .in_c(matrixC2_3), .out_a(out_a_3_3_NC), .out_a_chain(a3_3to3_4), .out_b(b3_3to4_3), .out_b0(b3_3to4_3_ping), .out_b1(b3_3to4_3_pong), .out_c(matrixC3_3), .b_data_valid_ping(b_data_valid_ping_delay3_3), .b_data_valid_pong(b_data_valid_pong_delay3_3), .mode(1'b0));
processing_element pe3_4(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay7), .in_a(in_a_3_4_NC),  .in_a_chain(a3_3to3_4), .in_b(b2_4to3_4), .in_c(matrixC2_4), .out_a(out_a_3_4_NC), .out_a_chain(a3_4to3_5), .out_b(b3_4to4_4), .out_b0(b3_4to4_4_ping), .out_b1(b3_4to4_4_pong), .out_c(matrixC3_4), .b_data_valid_ping(b_data_valid_ping_delay3_4), .b_data_valid_pong(b_data_valid_pong_delay3_4), .mode(1'b0));
processing_element pe3_5(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay8), .in_a(in_a_3_5_NC),  .in_a_chain(a3_4to3_5), .in_b(b2_5to3_5), .in_c(matrixC2_5), .out_a(out_a_3_5_NC), .out_a_chain(a3_5to3_6), .out_b(b3_5to4_5), .out_b0(b3_5to4_5_ping), .out_b1(b3_5to4_5_pong), .out_c(matrixC3_5), .b_data_valid_ping(b_data_valid_ping_delay3_5), .b_data_valid_pong(b_data_valid_pong_delay3_5), .mode(1'b0));
processing_element pe3_6(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay9), .in_a(in_a_3_6_NC),  .in_a_chain(a3_5to3_6), .in_b(b2_6to3_6), .in_c(matrixC2_6), .out_a(out_a_3_6_NC), .out_a_chain(a3_6to3_7), .out_b(b3_6to4_6), .out_b0(b3_6to4_6_ping), .out_b1(b3_6to4_6_pong), .out_c(matrixC3_6), .b_data_valid_ping(b_data_valid_ping_delay3_6), .b_data_valid_pong(b_data_valid_pong_delay3_6), .mode(1'b0));
processing_element pe3_7(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay10), .in_a(in_a_3_7_NC),  .in_a_chain(a3_6to3_7), .in_b(b2_7to3_7), .in_c(matrixC2_7), .out_a(out_a_3_7_NC), .out_a_chain(a3_7to3_8), .out_b(b3_7to4_7), .out_b0(b3_7to4_7_ping), .out_b1(b3_7to4_7_pong), .out_c(matrixC3_7), .b_data_valid_ping(b_data_valid_ping_delay3_7), .b_data_valid_pong(b_data_valid_pong_delay3_7), .mode(1'b0));
processing_element pe3_8(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay11), .in_a(in_a_3_8_NC),  .in_a_chain(a3_7to3_8), .in_b(b2_8to3_8), .in_c(matrixC2_8), .out_a(out_a_3_8_NC), .out_a_chain(a3_8to3_9), .out_b(b3_8to4_8), .out_b0(b3_8to4_8_ping), .out_b1(b3_8to4_8_pong), .out_c(matrixC3_8), .b_data_valid_ping(b_data_valid_ping_delay3_8), .b_data_valid_pong(b_data_valid_pong_delay3_8), .mode(1'b0));
processing_element pe3_9(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay12), .in_a(in_a_3_9_NC),  .in_a_chain(a3_8to3_9), .in_b(b2_9to3_9), .in_c(matrixC2_9), .out_a(out_a_3_9_NC), .out_a_chain(a3_9to3_10), .out_b(b3_9to4_9), .out_b0(b3_9to4_9_ping), .out_b1(b3_9to4_9_pong), .out_c(matrixC3_9), .b_data_valid_ping(b_data_valid_ping_delay3_9), .b_data_valid_pong(b_data_valid_pong_delay3_9), .mode(1'b0));
processing_element pe3_10(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay13), .in_a(in_a_3_10_NC),  .in_a_chain(a3_9to3_10), .in_b(b2_10to3_10), .in_c(matrixC2_10), .out_a(out_a_3_10_NC), .out_a_chain(a3_10to3_11), .out_b(b3_10to4_10), .out_b0(b3_10to4_10_ping), .out_b1(b3_10to4_10_pong), .out_c(matrixC3_10), .b_data_valid_ping(b_data_valid_ping_delay3_10), .b_data_valid_pong(b_data_valid_pong_delay3_10), .mode(1'b0));
processing_element pe3_11(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay14), .in_a(in_a_3_11_NC),  .in_a_chain(a3_10to3_11), .in_b(b2_11to3_11), .in_c(matrixC2_11), .out_a(out_a_3_11_NC), .out_a_chain(a3_11to3_12), .out_b(b3_11to4_11), .out_b0(b3_11to4_11_ping), .out_b1(b3_11to4_11_pong), .out_c(matrixC3_11), .b_data_valid_ping(b_data_valid_ping_delay3_11), .b_data_valid_pong(b_data_valid_pong_delay3_11), .mode(1'b0));
processing_element pe3_12(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay15), .in_a(in_a_3_12_NC),  .in_a_chain(a3_11to3_12), .in_b(b2_12to3_12), .in_c(matrixC2_12), .out_a(out_a_3_12_NC), .out_a_chain(a3_12to3_13), .out_b(b3_12to4_12), .out_b0(b3_12to4_12_ping), .out_b1(b3_12to4_12_pong), .out_c(matrixC3_12), .b_data_valid_ping(b_data_valid_ping_delay3_12), .b_data_valid_pong(b_data_valid_pong_delay3_12), .mode(1'b0));
processing_element pe3_13(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay16), .in_a(in_a_3_13_NC),  .in_a_chain(a3_12to3_13), .in_b(b2_13to3_13), .in_c(matrixC2_13), .out_a(out_a_3_13_NC), .out_a_chain(a3_13to3_14), .out_b(b3_13to4_13), .out_b0(b3_13to4_13_ping), .out_b1(b3_13to4_13_pong), .out_c(matrixC3_13), .b_data_valid_ping(b_data_valid_ping_delay3_13), .b_data_valid_pong(b_data_valid_pong_delay3_13), .mode(1'b0));
processing_element pe3_14(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay17), .in_a(in_a_3_14_NC),  .in_a_chain(a3_13to3_14), .in_b(b2_14to3_14), .in_c(matrixC2_14), .out_a(out_a_3_14_NC), .out_a_chain(a3_14to3_15), .out_b(b3_14to4_14), .out_b0(b3_14to4_14_ping), .out_b1(b3_14to4_14_pong), .out_c(matrixC3_14), .b_data_valid_ping(b_data_valid_ping_delay3_14), .b_data_valid_pong(b_data_valid_pong_delay3_14), .mode(1'b0));
processing_element pe3_15(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay18), .in_a(in_a_3_15_NC),  .in_a_chain(a3_14to3_15), .in_b(b2_15to3_15), .in_c(matrixC2_15), .out_a(out_a_3_15_NC), .out_a_chain(a3_15to3_16), .out_b(b3_15to4_15), .out_b0(b3_15to4_15_ping), .out_b1(b3_15to4_15_pong), .out_c(matrixC3_15), .b_data_valid_ping(b_data_valid_ping_delay3_15), .b_data_valid_pong(b_data_valid_pong_delay3_15), .mode(1'b0));
processing_element pe4_0(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay4), .in_a(a4),   .in_a_chain(in_a_chain_4_0_NC),   .in_b(b3_0to4_0), .in_c(matrixC3_0), .out_a(out_a_4_0_NC), .out_a_chain(a4_0to4_1), .out_b(b4_0to5_0), .out_b0(b4_0to5_0_ping), .out_b1(b4_0to5_0_pong), .out_c(matrixC4_0), .b_data_valid_ping(b_data_valid_ping_delay4_0), .b_data_valid_pong(b_data_valid_pong_delay4_0), .mode(1'b1));
processing_element pe4_1(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay5), .in_a(in_a_4_1_NC),  .in_a_chain(a4_0to4_1), .in_b(b3_1to4_1), .in_c(matrixC3_1), .out_a(out_a_4_1_NC), .out_a_chain(a4_1to4_2), .out_b(b4_1to5_1), .out_b0(b4_1to5_1_ping), .out_b1(b4_1to5_1_pong), .out_c(matrixC4_1), .b_data_valid_ping(b_data_valid_ping_delay4_1), .b_data_valid_pong(b_data_valid_pong_delay4_1), .mode(1'b0));
processing_element pe4_2(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay6), .in_a(in_a_4_2_NC),  .in_a_chain(a4_1to4_2), .in_b(b3_2to4_2), .in_c(matrixC3_2), .out_a(out_a_4_2_NC), .out_a_chain(a4_2to4_3), .out_b(b4_2to5_2), .out_b0(b4_2to5_2_ping), .out_b1(b4_2to5_2_pong), .out_c(matrixC4_2), .b_data_valid_ping(b_data_valid_ping_delay4_2), .b_data_valid_pong(b_data_valid_pong_delay4_2), .mode(1'b0));
processing_element pe4_3(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay7), .in_a(in_a_4_3_NC),  .in_a_chain(a4_2to4_3), .in_b(b3_3to4_3), .in_c(matrixC3_3), .out_a(out_a_4_3_NC), .out_a_chain(a4_3to4_4), .out_b(b4_3to5_3), .out_b0(b4_3to5_3_ping), .out_b1(b4_3to5_3_pong), .out_c(matrixC4_3), .b_data_valid_ping(b_data_valid_ping_delay4_3), .b_data_valid_pong(b_data_valid_pong_delay4_3), .mode(1'b0));
processing_element pe4_4(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay8), .in_a(in_a_4_4_NC),  .in_a_chain(a4_3to4_4), .in_b(b3_4to4_4), .in_c(matrixC3_4), .out_a(out_a_4_4_NC), .out_a_chain(a4_4to4_5), .out_b(b4_4to5_4), .out_b0(b4_4to5_4_ping), .out_b1(b4_4to5_4_pong), .out_c(matrixC4_4), .b_data_valid_ping(b_data_valid_ping_delay4_4), .b_data_valid_pong(b_data_valid_pong_delay4_4), .mode(1'b0));
processing_element pe4_5(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay9), .in_a(in_a_4_5_NC),  .in_a_chain(a4_4to4_5), .in_b(b3_5to4_5), .in_c(matrixC3_5), .out_a(out_a_4_5_NC), .out_a_chain(a4_5to4_6), .out_b(b4_5to5_5), .out_b0(b4_5to5_5_ping), .out_b1(b4_5to5_5_pong), .out_c(matrixC4_5), .b_data_valid_ping(b_data_valid_ping_delay4_5), .b_data_valid_pong(b_data_valid_pong_delay4_5), .mode(1'b0));
processing_element pe4_6(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay10), .in_a(in_a_4_6_NC),  .in_a_chain(a4_5to4_6), .in_b(b3_6to4_6), .in_c(matrixC3_6), .out_a(out_a_4_6_NC), .out_a_chain(a4_6to4_7), .out_b(b4_6to5_6), .out_b0(b4_6to5_6_ping), .out_b1(b4_6to5_6_pong), .out_c(matrixC4_6), .b_data_valid_ping(b_data_valid_ping_delay4_6), .b_data_valid_pong(b_data_valid_pong_delay4_6), .mode(1'b0));
processing_element pe4_7(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay11), .in_a(in_a_4_7_NC),  .in_a_chain(a4_6to4_7), .in_b(b3_7to4_7), .in_c(matrixC3_7), .out_a(out_a_4_7_NC), .out_a_chain(a4_7to4_8), .out_b(b4_7to5_7), .out_b0(b4_7to5_7_ping), .out_b1(b4_7to5_7_pong), .out_c(matrixC4_7), .b_data_valid_ping(b_data_valid_ping_delay4_7), .b_data_valid_pong(b_data_valid_pong_delay4_7), .mode(1'b0));
processing_element pe4_8(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay12), .in_a(in_a_4_8_NC),  .in_a_chain(a4_7to4_8), .in_b(b3_8to4_8), .in_c(matrixC3_8), .out_a(out_a_4_8_NC), .out_a_chain(a4_8to4_9), .out_b(b4_8to5_8), .out_b0(b4_8to5_8_ping), .out_b1(b4_8to5_8_pong), .out_c(matrixC4_8), .b_data_valid_ping(b_data_valid_ping_delay4_8), .b_data_valid_pong(b_data_valid_pong_delay4_8), .mode(1'b0));
processing_element pe4_9(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay13), .in_a(in_a_4_9_NC),  .in_a_chain(a4_8to4_9), .in_b(b3_9to4_9), .in_c(matrixC3_9), .out_a(out_a_4_9_NC), .out_a_chain(a4_9to4_10), .out_b(b4_9to5_9), .out_b0(b4_9to5_9_ping), .out_b1(b4_9to5_9_pong), .out_c(matrixC4_9), .b_data_valid_ping(b_data_valid_ping_delay4_9), .b_data_valid_pong(b_data_valid_pong_delay4_9), .mode(1'b0));
processing_element pe4_10(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay14), .in_a(in_a_4_10_NC),  .in_a_chain(a4_9to4_10), .in_b(b3_10to4_10), .in_c(matrixC3_10), .out_a(out_a_4_10_NC), .out_a_chain(a4_10to4_11), .out_b(b4_10to5_10), .out_b0(b4_10to5_10_ping), .out_b1(b4_10to5_10_pong), .out_c(matrixC4_10), .b_data_valid_ping(b_data_valid_ping_delay4_10), .b_data_valid_pong(b_data_valid_pong_delay4_10), .mode(1'b0));
processing_element pe4_11(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay15), .in_a(in_a_4_11_NC),  .in_a_chain(a4_10to4_11), .in_b(b3_11to4_11), .in_c(matrixC3_11), .out_a(out_a_4_11_NC), .out_a_chain(a4_11to4_12), .out_b(b4_11to5_11), .out_b0(b4_11to5_11_ping), .out_b1(b4_11to5_11_pong), .out_c(matrixC4_11), .b_data_valid_ping(b_data_valid_ping_delay4_11), .b_data_valid_pong(b_data_valid_pong_delay4_11), .mode(1'b0));
processing_element pe4_12(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay16), .in_a(in_a_4_12_NC),  .in_a_chain(a4_11to4_12), .in_b(b3_12to4_12), .in_c(matrixC3_12), .out_a(out_a_4_12_NC), .out_a_chain(a4_12to4_13), .out_b(b4_12to5_12), .out_b0(b4_12to5_12_ping), .out_b1(b4_12to5_12_pong), .out_c(matrixC4_12), .b_data_valid_ping(b_data_valid_ping_delay4_12), .b_data_valid_pong(b_data_valid_pong_delay4_12), .mode(1'b0));
processing_element pe4_13(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay17), .in_a(in_a_4_13_NC),  .in_a_chain(a4_12to4_13), .in_b(b3_13to4_13), .in_c(matrixC3_13), .out_a(out_a_4_13_NC), .out_a_chain(a4_13to4_14), .out_b(b4_13to5_13), .out_b0(b4_13to5_13_ping), .out_b1(b4_13to5_13_pong), .out_c(matrixC4_13), .b_data_valid_ping(b_data_valid_ping_delay4_13), .b_data_valid_pong(b_data_valid_pong_delay4_13), .mode(1'b0));
processing_element pe4_14(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay18), .in_a(in_a_4_14_NC),  .in_a_chain(a4_13to4_14), .in_b(b3_14to4_14), .in_c(matrixC3_14), .out_a(out_a_4_14_NC), .out_a_chain(a4_14to4_15), .out_b(b4_14to5_14), .out_b0(b4_14to5_14_ping), .out_b1(b4_14to5_14_pong), .out_c(matrixC4_14), .b_data_valid_ping(b_data_valid_ping_delay4_14), .b_data_valid_pong(b_data_valid_pong_delay4_14), .mode(1'b0));
processing_element pe4_15(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay19), .in_a(in_a_4_15_NC),  .in_a_chain(a4_14to4_15), .in_b(b3_15to4_15), .in_c(matrixC3_15), .out_a(out_a_4_15_NC), .out_a_chain(a4_15to4_16), .out_b(b4_15to5_15), .out_b0(b4_15to5_15_ping), .out_b1(b4_15to5_15_pong), .out_c(matrixC4_15), .b_data_valid_ping(b_data_valid_ping_delay4_15), .b_data_valid_pong(b_data_valid_pong_delay4_15), .mode(1'b0));
processing_element pe5_0(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay5), .in_a(a5),   .in_a_chain(in_a_chain_5_0_NC),   .in_b(b4_0to5_0), .in_c(matrixC4_0), .out_a(out_a_5_0_NC), .out_a_chain(a5_0to5_1), .out_b(b5_0to6_0), .out_b0(b5_0to6_0_ping), .out_b1(b5_0to6_0_pong), .out_c(matrixC5_0), .b_data_valid_ping(b_data_valid_ping_delay5_0), .b_data_valid_pong(b_data_valid_pong_delay5_0), .mode(1'b1));
processing_element pe5_1(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay6), .in_a(in_a_5_1_NC),  .in_a_chain(a5_0to5_1), .in_b(b4_1to5_1), .in_c(matrixC4_1), .out_a(out_a_5_1_NC), .out_a_chain(a5_1to5_2), .out_b(b5_1to6_1), .out_b0(b5_1to6_1_ping), .out_b1(b5_1to6_1_pong), .out_c(matrixC5_1), .b_data_valid_ping(b_data_valid_ping_delay5_1), .b_data_valid_pong(b_data_valid_pong_delay5_1), .mode(1'b0));
processing_element pe5_2(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay7), .in_a(in_a_5_2_NC),  .in_a_chain(a5_1to5_2), .in_b(b4_2to5_2), .in_c(matrixC4_2), .out_a(out_a_5_2_NC), .out_a_chain(a5_2to5_3), .out_b(b5_2to6_2), .out_b0(b5_2to6_2_ping), .out_b1(b5_2to6_2_pong), .out_c(matrixC5_2), .b_data_valid_ping(b_data_valid_ping_delay5_2), .b_data_valid_pong(b_data_valid_pong_delay5_2), .mode(1'b0));
processing_element pe5_3(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay8), .in_a(in_a_5_3_NC),  .in_a_chain(a5_2to5_3), .in_b(b4_3to5_3), .in_c(matrixC4_3), .out_a(out_a_5_3_NC), .out_a_chain(a5_3to5_4), .out_b(b5_3to6_3), .out_b0(b5_3to6_3_ping), .out_b1(b5_3to6_3_pong), .out_c(matrixC5_3), .b_data_valid_ping(b_data_valid_ping_delay5_3), .b_data_valid_pong(b_data_valid_pong_delay5_3), .mode(1'b0));
processing_element pe5_4(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay9), .in_a(in_a_5_4_NC),  .in_a_chain(a5_3to5_4), .in_b(b4_4to5_4), .in_c(matrixC4_4), .out_a(out_a_5_4_NC), .out_a_chain(a5_4to5_5), .out_b(b5_4to6_4), .out_b0(b5_4to6_4_ping), .out_b1(b5_4to6_4_pong), .out_c(matrixC5_4), .b_data_valid_ping(b_data_valid_ping_delay5_4), .b_data_valid_pong(b_data_valid_pong_delay5_4), .mode(1'b0));
processing_element pe5_5(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay10), .in_a(in_a_5_5_NC),  .in_a_chain(a5_4to5_5), .in_b(b4_5to5_5), .in_c(matrixC4_5), .out_a(out_a_5_5_NC), .out_a_chain(a5_5to5_6), .out_b(b5_5to6_5), .out_b0(b5_5to6_5_ping), .out_b1(b5_5to6_5_pong), .out_c(matrixC5_5), .b_data_valid_ping(b_data_valid_ping_delay5_5), .b_data_valid_pong(b_data_valid_pong_delay5_5), .mode(1'b0));
processing_element pe5_6(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay11), .in_a(in_a_5_6_NC),  .in_a_chain(a5_5to5_6), .in_b(b4_6to5_6), .in_c(matrixC4_6), .out_a(out_a_5_6_NC), .out_a_chain(a5_6to5_7), .out_b(b5_6to6_6), .out_b0(b5_6to6_6_ping), .out_b1(b5_6to6_6_pong), .out_c(matrixC5_6), .b_data_valid_ping(b_data_valid_ping_delay5_6), .b_data_valid_pong(b_data_valid_pong_delay5_6), .mode(1'b0));
processing_element pe5_7(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay12), .in_a(in_a_5_7_NC),  .in_a_chain(a5_6to5_7), .in_b(b4_7to5_7), .in_c(matrixC4_7), .out_a(out_a_5_7_NC), .out_a_chain(a5_7to5_8), .out_b(b5_7to6_7), .out_b0(b5_7to6_7_ping), .out_b1(b5_7to6_7_pong), .out_c(matrixC5_7), .b_data_valid_ping(b_data_valid_ping_delay5_7), .b_data_valid_pong(b_data_valid_pong_delay5_7), .mode(1'b0));
processing_element pe5_8(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay13), .in_a(in_a_5_8_NC),  .in_a_chain(a5_7to5_8), .in_b(b4_8to5_8), .in_c(matrixC4_8), .out_a(out_a_5_8_NC), .out_a_chain(a5_8to5_9), .out_b(b5_8to6_8), .out_b0(b5_8to6_8_ping), .out_b1(b5_8to6_8_pong), .out_c(matrixC5_8), .b_data_valid_ping(b_data_valid_ping_delay5_8), .b_data_valid_pong(b_data_valid_pong_delay5_8), .mode(1'b0));
processing_element pe5_9(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay14), .in_a(in_a_5_9_NC),  .in_a_chain(a5_8to5_9), .in_b(b4_9to5_9), .in_c(matrixC4_9), .out_a(out_a_5_9_NC), .out_a_chain(a5_9to5_10), .out_b(b5_9to6_9), .out_b0(b5_9to6_9_ping), .out_b1(b5_9to6_9_pong), .out_c(matrixC5_9), .b_data_valid_ping(b_data_valid_ping_delay5_9), .b_data_valid_pong(b_data_valid_pong_delay5_9), .mode(1'b0));
processing_element pe5_10(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay15), .in_a(in_a_5_10_NC),  .in_a_chain(a5_9to5_10), .in_b(b4_10to5_10), .in_c(matrixC4_10), .out_a(out_a_5_10_NC), .out_a_chain(a5_10to5_11), .out_b(b5_10to6_10), .out_b0(b5_10to6_10_ping), .out_b1(b5_10to6_10_pong), .out_c(matrixC5_10), .b_data_valid_ping(b_data_valid_ping_delay5_10), .b_data_valid_pong(b_data_valid_pong_delay5_10), .mode(1'b0));
processing_element pe5_11(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay16), .in_a(in_a_5_11_NC),  .in_a_chain(a5_10to5_11), .in_b(b4_11to5_11), .in_c(matrixC4_11), .out_a(out_a_5_11_NC), .out_a_chain(a5_11to5_12), .out_b(b5_11to6_11), .out_b0(b5_11to6_11_ping), .out_b1(b5_11to6_11_pong), .out_c(matrixC5_11), .b_data_valid_ping(b_data_valid_ping_delay5_11), .b_data_valid_pong(b_data_valid_pong_delay5_11), .mode(1'b0));
processing_element pe5_12(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay17), .in_a(in_a_5_12_NC),  .in_a_chain(a5_11to5_12), .in_b(b4_12to5_12), .in_c(matrixC4_12), .out_a(out_a_5_12_NC), .out_a_chain(a5_12to5_13), .out_b(b5_12to6_12), .out_b0(b5_12to6_12_ping), .out_b1(b5_12to6_12_pong), .out_c(matrixC5_12), .b_data_valid_ping(b_data_valid_ping_delay5_12), .b_data_valid_pong(b_data_valid_pong_delay5_12), .mode(1'b0));
processing_element pe5_13(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay18), .in_a(in_a_5_13_NC),  .in_a_chain(a5_12to5_13), .in_b(b4_13to5_13), .in_c(matrixC4_13), .out_a(out_a_5_13_NC), .out_a_chain(a5_13to5_14), .out_b(b5_13to6_13), .out_b0(b5_13to6_13_ping), .out_b1(b5_13to6_13_pong), .out_c(matrixC5_13), .b_data_valid_ping(b_data_valid_ping_delay5_13), .b_data_valid_pong(b_data_valid_pong_delay5_13), .mode(1'b0));
processing_element pe5_14(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay19), .in_a(in_a_5_14_NC),  .in_a_chain(a5_13to5_14), .in_b(b4_14to5_14), .in_c(matrixC4_14), .out_a(out_a_5_14_NC), .out_a_chain(a5_14to5_15), .out_b(b5_14to6_14), .out_b0(b5_14to6_14_ping), .out_b1(b5_14to6_14_pong), .out_c(matrixC5_14), .b_data_valid_ping(b_data_valid_ping_delay5_14), .b_data_valid_pong(b_data_valid_pong_delay5_14), .mode(1'b0));
processing_element pe5_15(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay20), .in_a(in_a_5_15_NC),  .in_a_chain(a5_14to5_15), .in_b(b4_15to5_15), .in_c(matrixC4_15), .out_a(out_a_5_15_NC), .out_a_chain(a5_15to5_16), .out_b(b5_15to6_15), .out_b0(b5_15to6_15_ping), .out_b1(b5_15to6_15_pong), .out_c(matrixC5_15), .b_data_valid_ping(b_data_valid_ping_delay5_15), .b_data_valid_pong(b_data_valid_pong_delay5_15), .mode(1'b0));
processing_element pe6_0(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay6), .in_a(a6),   .in_a_chain(in_a_chain_6_0_NC),   .in_b(b5_0to6_0), .in_c(matrixC5_0), .out_a(out_a_6_0_NC), .out_a_chain(a6_0to6_1), .out_b(b6_0to7_0), .out_b0(b6_0to7_0_ping), .out_b1(b6_0to7_0_pong), .out_c(matrixC6_0), .b_data_valid_ping(b_data_valid_ping_delay6_0), .b_data_valid_pong(b_data_valid_pong_delay6_0), .mode(1'b1));
processing_element pe6_1(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay7), .in_a(in_a_6_1_NC),  .in_a_chain(a6_0to6_1), .in_b(b5_1to6_1), .in_c(matrixC5_1), .out_a(out_a_6_1_NC), .out_a_chain(a6_1to6_2), .out_b(b6_1to7_1), .out_b0(b6_1to7_1_ping), .out_b1(b6_1to7_1_pong), .out_c(matrixC6_1), .b_data_valid_ping(b_data_valid_ping_delay6_1), .b_data_valid_pong(b_data_valid_pong_delay6_1), .mode(1'b0));
processing_element pe6_2(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay8), .in_a(in_a_6_2_NC),  .in_a_chain(a6_1to6_2), .in_b(b5_2to6_2), .in_c(matrixC5_2), .out_a(out_a_6_2_NC), .out_a_chain(a6_2to6_3), .out_b(b6_2to7_2), .out_b0(b6_2to7_2_ping), .out_b1(b6_2to7_2_pong), .out_c(matrixC6_2), .b_data_valid_ping(b_data_valid_ping_delay6_2), .b_data_valid_pong(b_data_valid_pong_delay6_2), .mode(1'b0));
processing_element pe6_3(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay9), .in_a(in_a_6_3_NC),  .in_a_chain(a6_2to6_3), .in_b(b5_3to6_3), .in_c(matrixC5_3), .out_a(out_a_6_3_NC), .out_a_chain(a6_3to6_4), .out_b(b6_3to7_3), .out_b0(b6_3to7_3_ping), .out_b1(b6_3to7_3_pong), .out_c(matrixC6_3), .b_data_valid_ping(b_data_valid_ping_delay6_3), .b_data_valid_pong(b_data_valid_pong_delay6_3), .mode(1'b0));
processing_element pe6_4(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay10), .in_a(in_a_6_4_NC),  .in_a_chain(a6_3to6_4), .in_b(b5_4to6_4), .in_c(matrixC5_4), .out_a(out_a_6_4_NC), .out_a_chain(a6_4to6_5), .out_b(b6_4to7_4), .out_b0(b6_4to7_4_ping), .out_b1(b6_4to7_4_pong), .out_c(matrixC6_4), .b_data_valid_ping(b_data_valid_ping_delay6_4), .b_data_valid_pong(b_data_valid_pong_delay6_4), .mode(1'b0));
processing_element pe6_5(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay11), .in_a(in_a_6_5_NC),  .in_a_chain(a6_4to6_5), .in_b(b5_5to6_5), .in_c(matrixC5_5), .out_a(out_a_6_5_NC), .out_a_chain(a6_5to6_6), .out_b(b6_5to7_5), .out_b0(b6_5to7_5_ping), .out_b1(b6_5to7_5_pong), .out_c(matrixC6_5), .b_data_valid_ping(b_data_valid_ping_delay6_5), .b_data_valid_pong(b_data_valid_pong_delay6_5), .mode(1'b0));
processing_element pe6_6(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay12), .in_a(in_a_6_6_NC),  .in_a_chain(a6_5to6_6), .in_b(b5_6to6_6), .in_c(matrixC5_6), .out_a(out_a_6_6_NC), .out_a_chain(a6_6to6_7), .out_b(b6_6to7_6), .out_b0(b6_6to7_6_ping), .out_b1(b6_6to7_6_pong), .out_c(matrixC6_6), .b_data_valid_ping(b_data_valid_ping_delay6_6), .b_data_valid_pong(b_data_valid_pong_delay6_6), .mode(1'b0));
processing_element pe6_7(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay13), .in_a(in_a_6_7_NC),  .in_a_chain(a6_6to6_7), .in_b(b5_7to6_7), .in_c(matrixC5_7), .out_a(out_a_6_7_NC), .out_a_chain(a6_7to6_8), .out_b(b6_7to7_7), .out_b0(b6_7to7_7_ping), .out_b1(b6_7to7_7_pong), .out_c(matrixC6_7), .b_data_valid_ping(b_data_valid_ping_delay6_7), .b_data_valid_pong(b_data_valid_pong_delay6_7), .mode(1'b0));
processing_element pe6_8(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay14), .in_a(in_a_6_8_NC),  .in_a_chain(a6_7to6_8), .in_b(b5_8to6_8), .in_c(matrixC5_8), .out_a(out_a_6_8_NC), .out_a_chain(a6_8to6_9), .out_b(b6_8to7_8), .out_b0(b6_8to7_8_ping), .out_b1(b6_8to7_8_pong), .out_c(matrixC6_8), .b_data_valid_ping(b_data_valid_ping_delay6_8), .b_data_valid_pong(b_data_valid_pong_delay6_8), .mode(1'b0));
processing_element pe6_9(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay15), .in_a(in_a_6_9_NC),  .in_a_chain(a6_8to6_9), .in_b(b5_9to6_9), .in_c(matrixC5_9), .out_a(out_a_6_9_NC), .out_a_chain(a6_9to6_10), .out_b(b6_9to7_9), .out_b0(b6_9to7_9_ping), .out_b1(b6_9to7_9_pong), .out_c(matrixC6_9), .b_data_valid_ping(b_data_valid_ping_delay6_9), .b_data_valid_pong(b_data_valid_pong_delay6_9), .mode(1'b0));
processing_element pe6_10(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay16), .in_a(in_a_6_10_NC),  .in_a_chain(a6_9to6_10), .in_b(b5_10to6_10), .in_c(matrixC5_10), .out_a(out_a_6_10_NC), .out_a_chain(a6_10to6_11), .out_b(b6_10to7_10), .out_b0(b6_10to7_10_ping), .out_b1(b6_10to7_10_pong), .out_c(matrixC6_10), .b_data_valid_ping(b_data_valid_ping_delay6_10), .b_data_valid_pong(b_data_valid_pong_delay6_10), .mode(1'b0));
processing_element pe6_11(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay17), .in_a(in_a_6_11_NC),  .in_a_chain(a6_10to6_11), .in_b(b5_11to6_11), .in_c(matrixC5_11), .out_a(out_a_6_11_NC), .out_a_chain(a6_11to6_12), .out_b(b6_11to7_11), .out_b0(b6_11to7_11_ping), .out_b1(b6_11to7_11_pong), .out_c(matrixC6_11), .b_data_valid_ping(b_data_valid_ping_delay6_11), .b_data_valid_pong(b_data_valid_pong_delay6_11), .mode(1'b0));
processing_element pe6_12(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay18), .in_a(in_a_6_12_NC),  .in_a_chain(a6_11to6_12), .in_b(b5_12to6_12), .in_c(matrixC5_12), .out_a(out_a_6_12_NC), .out_a_chain(a6_12to6_13), .out_b(b6_12to7_12), .out_b0(b6_12to7_12_ping), .out_b1(b6_12to7_12_pong), .out_c(matrixC6_12), .b_data_valid_ping(b_data_valid_ping_delay6_12), .b_data_valid_pong(b_data_valid_pong_delay6_12), .mode(1'b0));
processing_element pe6_13(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay19), .in_a(in_a_6_13_NC),  .in_a_chain(a6_12to6_13), .in_b(b5_13to6_13), .in_c(matrixC5_13), .out_a(out_a_6_13_NC), .out_a_chain(a6_13to6_14), .out_b(b6_13to7_13), .out_b0(b6_13to7_13_ping), .out_b1(b6_13to7_13_pong), .out_c(matrixC6_13), .b_data_valid_ping(b_data_valid_ping_delay6_13), .b_data_valid_pong(b_data_valid_pong_delay6_13), .mode(1'b0));
processing_element pe6_14(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay20), .in_a(in_a_6_14_NC),  .in_a_chain(a6_13to6_14), .in_b(b5_14to6_14), .in_c(matrixC5_14), .out_a(out_a_6_14_NC), .out_a_chain(a6_14to6_15), .out_b(b6_14to7_14), .out_b0(b6_14to7_14_ping), .out_b1(b6_14to7_14_pong), .out_c(matrixC6_14), .b_data_valid_ping(b_data_valid_ping_delay6_14), .b_data_valid_pong(b_data_valid_pong_delay6_14), .mode(1'b0));
processing_element pe6_15(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay21), .in_a(in_a_6_15_NC),  .in_a_chain(a6_14to6_15), .in_b(b5_15to6_15), .in_c(matrixC5_15), .out_a(out_a_6_15_NC), .out_a_chain(a6_15to6_16), .out_b(b6_15to7_15), .out_b0(b6_15to7_15_ping), .out_b1(b6_15to7_15_pong), .out_c(matrixC6_15), .b_data_valid_ping(b_data_valid_ping_delay6_15), .b_data_valid_pong(b_data_valid_pong_delay6_15), .mode(1'b0));
processing_element pe7_0(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay7), .in_a(a7),   .in_a_chain(in_a_chain_7_0_NC),   .in_b(b6_0to7_0), .in_c(matrixC6_0), .out_a(out_a_7_0_NC), .out_a_chain(a7_0to7_1), .out_b(b7_0to8_0), .out_b0(b7_0to8_0_ping), .out_b1(b7_0to8_0_pong), .out_c(matrixC7_0), .b_data_valid_ping(b_data_valid_ping_delay7_0), .b_data_valid_pong(b_data_valid_pong_delay7_0), .mode(1'b1));
processing_element pe7_1(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay8), .in_a(in_a_7_1_NC),  .in_a_chain(a7_0to7_1), .in_b(b6_1to7_1), .in_c(matrixC6_1), .out_a(out_a_7_1_NC), .out_a_chain(a7_1to7_2), .out_b(b7_1to8_1), .out_b0(b7_1to8_1_ping), .out_b1(b7_1to8_1_pong), .out_c(matrixC7_1), .b_data_valid_ping(b_data_valid_ping_delay7_1), .b_data_valid_pong(b_data_valid_pong_delay7_1), .mode(1'b0));
processing_element pe7_2(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay9), .in_a(in_a_7_2_NC),  .in_a_chain(a7_1to7_2), .in_b(b6_2to7_2), .in_c(matrixC6_2), .out_a(out_a_7_2_NC), .out_a_chain(a7_2to7_3), .out_b(b7_2to8_2), .out_b0(b7_2to8_2_ping), .out_b1(b7_2to8_2_pong), .out_c(matrixC7_2), .b_data_valid_ping(b_data_valid_ping_delay7_2), .b_data_valid_pong(b_data_valid_pong_delay7_2), .mode(1'b0));
processing_element pe7_3(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay10), .in_a(in_a_7_3_NC),  .in_a_chain(a7_2to7_3), .in_b(b6_3to7_3), .in_c(matrixC6_3), .out_a(out_a_7_3_NC), .out_a_chain(a7_3to7_4), .out_b(b7_3to8_3), .out_b0(b7_3to8_3_ping), .out_b1(b7_3to8_3_pong), .out_c(matrixC7_3), .b_data_valid_ping(b_data_valid_ping_delay7_3), .b_data_valid_pong(b_data_valid_pong_delay7_3), .mode(1'b0));
processing_element pe7_4(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay11), .in_a(in_a_7_4_NC),  .in_a_chain(a7_3to7_4), .in_b(b6_4to7_4), .in_c(matrixC6_4), .out_a(out_a_7_4_NC), .out_a_chain(a7_4to7_5), .out_b(b7_4to8_4), .out_b0(b7_4to8_4_ping), .out_b1(b7_4to8_4_pong), .out_c(matrixC7_4), .b_data_valid_ping(b_data_valid_ping_delay7_4), .b_data_valid_pong(b_data_valid_pong_delay7_4), .mode(1'b0));
processing_element pe7_5(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay12), .in_a(in_a_7_5_NC),  .in_a_chain(a7_4to7_5), .in_b(b6_5to7_5), .in_c(matrixC6_5), .out_a(out_a_7_5_NC), .out_a_chain(a7_5to7_6), .out_b(b7_5to8_5), .out_b0(b7_5to8_5_ping), .out_b1(b7_5to8_5_pong), .out_c(matrixC7_5), .b_data_valid_ping(b_data_valid_ping_delay7_5), .b_data_valid_pong(b_data_valid_pong_delay7_5), .mode(1'b0));
processing_element pe7_6(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay13), .in_a(in_a_7_6_NC),  .in_a_chain(a7_5to7_6), .in_b(b6_6to7_6), .in_c(matrixC6_6), .out_a(out_a_7_6_NC), .out_a_chain(a7_6to7_7), .out_b(b7_6to8_6), .out_b0(b7_6to8_6_ping), .out_b1(b7_6to8_6_pong), .out_c(matrixC7_6), .b_data_valid_ping(b_data_valid_ping_delay7_6), .b_data_valid_pong(b_data_valid_pong_delay7_6), .mode(1'b0));
processing_element pe7_7(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay14), .in_a(in_a_7_7_NC),  .in_a_chain(a7_6to7_7), .in_b(b6_7to7_7), .in_c(matrixC6_7), .out_a(out_a_7_7_NC), .out_a_chain(a7_7to7_8), .out_b(b7_7to8_7), .out_b0(b7_7to8_7_ping), .out_b1(b7_7to8_7_pong), .out_c(matrixC7_7), .b_data_valid_ping(b_data_valid_ping_delay7_7), .b_data_valid_pong(b_data_valid_pong_delay7_7), .mode(1'b0));
processing_element pe7_8(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay15), .in_a(in_a_7_8_NC),  .in_a_chain(a7_7to7_8), .in_b(b6_8to7_8), .in_c(matrixC6_8), .out_a(out_a_7_8_NC), .out_a_chain(a7_8to7_9), .out_b(b7_8to8_8), .out_b0(b7_8to8_8_ping), .out_b1(b7_8to8_8_pong), .out_c(matrixC7_8), .b_data_valid_ping(b_data_valid_ping_delay7_8), .b_data_valid_pong(b_data_valid_pong_delay7_8), .mode(1'b0));
processing_element pe7_9(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay16), .in_a(in_a_7_9_NC),  .in_a_chain(a7_8to7_9), .in_b(b6_9to7_9), .in_c(matrixC6_9), .out_a(out_a_7_9_NC), .out_a_chain(a7_9to7_10), .out_b(b7_9to8_9), .out_b0(b7_9to8_9_ping), .out_b1(b7_9to8_9_pong), .out_c(matrixC7_9), .b_data_valid_ping(b_data_valid_ping_delay7_9), .b_data_valid_pong(b_data_valid_pong_delay7_9), .mode(1'b0));
processing_element pe7_10(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay17), .in_a(in_a_7_10_NC),  .in_a_chain(a7_9to7_10), .in_b(b6_10to7_10), .in_c(matrixC6_10), .out_a(out_a_7_10_NC), .out_a_chain(a7_10to7_11), .out_b(b7_10to8_10), .out_b0(b7_10to8_10_ping), .out_b1(b7_10to8_10_pong), .out_c(matrixC7_10), .b_data_valid_ping(b_data_valid_ping_delay7_10), .b_data_valid_pong(b_data_valid_pong_delay7_10), .mode(1'b0));
processing_element pe7_11(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay18), .in_a(in_a_7_11_NC),  .in_a_chain(a7_10to7_11), .in_b(b6_11to7_11), .in_c(matrixC6_11), .out_a(out_a_7_11_NC), .out_a_chain(a7_11to7_12), .out_b(b7_11to8_11), .out_b0(b7_11to8_11_ping), .out_b1(b7_11to8_11_pong), .out_c(matrixC7_11), .b_data_valid_ping(b_data_valid_ping_delay7_11), .b_data_valid_pong(b_data_valid_pong_delay7_11), .mode(1'b0));
processing_element pe7_12(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay19), .in_a(in_a_7_12_NC),  .in_a_chain(a7_11to7_12), .in_b(b6_12to7_12), .in_c(matrixC6_12), .out_a(out_a_7_12_NC), .out_a_chain(a7_12to7_13), .out_b(b7_12to8_12), .out_b0(b7_12to8_12_ping), .out_b1(b7_12to8_12_pong), .out_c(matrixC7_12), .b_data_valid_ping(b_data_valid_ping_delay7_12), .b_data_valid_pong(b_data_valid_pong_delay7_12), .mode(1'b0));
processing_element pe7_13(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay20), .in_a(in_a_7_13_NC),  .in_a_chain(a7_12to7_13), .in_b(b6_13to7_13), .in_c(matrixC6_13), .out_a(out_a_7_13_NC), .out_a_chain(a7_13to7_14), .out_b(b7_13to8_13), .out_b0(b7_13to8_13_ping), .out_b1(b7_13to8_13_pong), .out_c(matrixC7_13), .b_data_valid_ping(b_data_valid_ping_delay7_13), .b_data_valid_pong(b_data_valid_pong_delay7_13), .mode(1'b0));
processing_element pe7_14(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay21), .in_a(in_a_7_14_NC),  .in_a_chain(a7_13to7_14), .in_b(b6_14to7_14), .in_c(matrixC6_14), .out_a(out_a_7_14_NC), .out_a_chain(a7_14to7_15), .out_b(b7_14to8_14), .out_b0(b7_14to8_14_ping), .out_b1(b7_14to8_14_pong), .out_c(matrixC7_14), .b_data_valid_ping(b_data_valid_ping_delay7_14), .b_data_valid_pong(b_data_valid_pong_delay7_14), .mode(1'b0));
processing_element pe7_15(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay22), .in_a(in_a_7_15_NC),  .in_a_chain(a7_14to7_15), .in_b(b6_15to7_15), .in_c(matrixC6_15), .out_a(out_a_7_15_NC), .out_a_chain(a7_15to7_16), .out_b(b7_15to8_15), .out_b0(b7_15to8_15_ping), .out_b1(b7_15to8_15_pong), .out_c(matrixC7_15), .b_data_valid_ping(b_data_valid_ping_delay7_15), .b_data_valid_pong(b_data_valid_pong_delay7_15), .mode(1'b0));
processing_element pe8_0(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay8), .in_a(a8),   .in_a_chain(in_a_chain_8_0_NC),   .in_b(b7_0to8_0), .in_c(matrixC7_0), .out_a(out_a_8_0_NC), .out_a_chain(a8_0to8_1), .out_b(b8_0to9_0), .out_b0(b8_0to9_0_ping), .out_b1(b8_0to9_0_pong), .out_c(matrixC8_0), .b_data_valid_ping(b_data_valid_ping_delay8_0), .b_data_valid_pong(b_data_valid_pong_delay8_0), .mode(1'b1));
processing_element pe8_1(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay9), .in_a(in_a_8_1_NC),  .in_a_chain(a8_0to8_1), .in_b(b7_1to8_1), .in_c(matrixC7_1), .out_a(out_a_8_1_NC), .out_a_chain(a8_1to8_2), .out_b(b8_1to9_1), .out_b0(b8_1to9_1_ping), .out_b1(b8_1to9_1_pong), .out_c(matrixC8_1), .b_data_valid_ping(b_data_valid_ping_delay8_1), .b_data_valid_pong(b_data_valid_pong_delay8_1), .mode(1'b0));
processing_element pe8_2(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay10), .in_a(in_a_8_2_NC),  .in_a_chain(a8_1to8_2), .in_b(b7_2to8_2), .in_c(matrixC7_2), .out_a(out_a_8_2_NC), .out_a_chain(a8_2to8_3), .out_b(b8_2to9_2), .out_b0(b8_2to9_2_ping), .out_b1(b8_2to9_2_pong), .out_c(matrixC8_2), .b_data_valid_ping(b_data_valid_ping_delay8_2), .b_data_valid_pong(b_data_valid_pong_delay8_2), .mode(1'b0));
processing_element pe8_3(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay11), .in_a(in_a_8_3_NC),  .in_a_chain(a8_2to8_3), .in_b(b7_3to8_3), .in_c(matrixC7_3), .out_a(out_a_8_3_NC), .out_a_chain(a8_3to8_4), .out_b(b8_3to9_3), .out_b0(b8_3to9_3_ping), .out_b1(b8_3to9_3_pong), .out_c(matrixC8_3), .b_data_valid_ping(b_data_valid_ping_delay8_3), .b_data_valid_pong(b_data_valid_pong_delay8_3), .mode(1'b0));
processing_element pe8_4(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay12), .in_a(in_a_8_4_NC),  .in_a_chain(a8_3to8_4), .in_b(b7_4to8_4), .in_c(matrixC7_4), .out_a(out_a_8_4_NC), .out_a_chain(a8_4to8_5), .out_b(b8_4to9_4), .out_b0(b8_4to9_4_ping), .out_b1(b8_4to9_4_pong), .out_c(matrixC8_4), .b_data_valid_ping(b_data_valid_ping_delay8_4), .b_data_valid_pong(b_data_valid_pong_delay8_4), .mode(1'b0));
processing_element pe8_5(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay13), .in_a(in_a_8_5_NC),  .in_a_chain(a8_4to8_5), .in_b(b7_5to8_5), .in_c(matrixC7_5), .out_a(out_a_8_5_NC), .out_a_chain(a8_5to8_6), .out_b(b8_5to9_5), .out_b0(b8_5to9_5_ping), .out_b1(b8_5to9_5_pong), .out_c(matrixC8_5), .b_data_valid_ping(b_data_valid_ping_delay8_5), .b_data_valid_pong(b_data_valid_pong_delay8_5), .mode(1'b0));
processing_element pe8_6(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay14), .in_a(in_a_8_6_NC),  .in_a_chain(a8_5to8_6), .in_b(b7_6to8_6), .in_c(matrixC7_6), .out_a(out_a_8_6_NC), .out_a_chain(a8_6to8_7), .out_b(b8_6to9_6), .out_b0(b8_6to9_6_ping), .out_b1(b8_6to9_6_pong), .out_c(matrixC8_6), .b_data_valid_ping(b_data_valid_ping_delay8_6), .b_data_valid_pong(b_data_valid_pong_delay8_6), .mode(1'b0));
processing_element pe8_7(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay15), .in_a(in_a_8_7_NC),  .in_a_chain(a8_6to8_7), .in_b(b7_7to8_7), .in_c(matrixC7_7), .out_a(out_a_8_7_NC), .out_a_chain(a8_7to8_8), .out_b(b8_7to9_7), .out_b0(b8_7to9_7_ping), .out_b1(b8_7to9_7_pong), .out_c(matrixC8_7), .b_data_valid_ping(b_data_valid_ping_delay8_7), .b_data_valid_pong(b_data_valid_pong_delay8_7), .mode(1'b0));
processing_element pe8_8(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay16), .in_a(in_a_8_8_NC),  .in_a_chain(a8_7to8_8), .in_b(b7_8to8_8), .in_c(matrixC7_8), .out_a(out_a_8_8_NC), .out_a_chain(a8_8to8_9), .out_b(b8_8to9_8), .out_b0(b8_8to9_8_ping), .out_b1(b8_8to9_8_pong), .out_c(matrixC8_8), .b_data_valid_ping(b_data_valid_ping_delay8_8), .b_data_valid_pong(b_data_valid_pong_delay8_8), .mode(1'b0));
processing_element pe8_9(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay17), .in_a(in_a_8_9_NC),  .in_a_chain(a8_8to8_9), .in_b(b7_9to8_9), .in_c(matrixC7_9), .out_a(out_a_8_9_NC), .out_a_chain(a8_9to8_10), .out_b(b8_9to9_9), .out_b0(b8_9to9_9_ping), .out_b1(b8_9to9_9_pong), .out_c(matrixC8_9), .b_data_valid_ping(b_data_valid_ping_delay8_9), .b_data_valid_pong(b_data_valid_pong_delay8_9), .mode(1'b0));
processing_element pe8_10(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay18), .in_a(in_a_8_10_NC),  .in_a_chain(a8_9to8_10), .in_b(b7_10to8_10), .in_c(matrixC7_10), .out_a(out_a_8_10_NC), .out_a_chain(a8_10to8_11), .out_b(b8_10to9_10), .out_b0(b8_10to9_10_ping), .out_b1(b8_10to9_10_pong), .out_c(matrixC8_10), .b_data_valid_ping(b_data_valid_ping_delay8_10), .b_data_valid_pong(b_data_valid_pong_delay8_10), .mode(1'b0));
processing_element pe8_11(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay19), .in_a(in_a_8_11_NC),  .in_a_chain(a8_10to8_11), .in_b(b7_11to8_11), .in_c(matrixC7_11), .out_a(out_a_8_11_NC), .out_a_chain(a8_11to8_12), .out_b(b8_11to9_11), .out_b0(b8_11to9_11_ping), .out_b1(b8_11to9_11_pong), .out_c(matrixC8_11), .b_data_valid_ping(b_data_valid_ping_delay8_11), .b_data_valid_pong(b_data_valid_pong_delay8_11), .mode(1'b0));
processing_element pe8_12(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay20), .in_a(in_a_8_12_NC),  .in_a_chain(a8_11to8_12), .in_b(b7_12to8_12), .in_c(matrixC7_12), .out_a(out_a_8_12_NC), .out_a_chain(a8_12to8_13), .out_b(b8_12to9_12), .out_b0(b8_12to9_12_ping), .out_b1(b8_12to9_12_pong), .out_c(matrixC8_12), .b_data_valid_ping(b_data_valid_ping_delay8_12), .b_data_valid_pong(b_data_valid_pong_delay8_12), .mode(1'b0));
processing_element pe8_13(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay21), .in_a(in_a_8_13_NC),  .in_a_chain(a8_12to8_13), .in_b(b7_13to8_13), .in_c(matrixC7_13), .out_a(out_a_8_13_NC), .out_a_chain(a8_13to8_14), .out_b(b8_13to9_13), .out_b0(b8_13to9_13_ping), .out_b1(b8_13to9_13_pong), .out_c(matrixC8_13), .b_data_valid_ping(b_data_valid_ping_delay8_13), .b_data_valid_pong(b_data_valid_pong_delay8_13), .mode(1'b0));
processing_element pe8_14(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay22), .in_a(in_a_8_14_NC),  .in_a_chain(a8_13to8_14), .in_b(b7_14to8_14), .in_c(matrixC7_14), .out_a(out_a_8_14_NC), .out_a_chain(a8_14to8_15), .out_b(b8_14to9_14), .out_b0(b8_14to9_14_ping), .out_b1(b8_14to9_14_pong), .out_c(matrixC8_14), .b_data_valid_ping(b_data_valid_ping_delay8_14), .b_data_valid_pong(b_data_valid_pong_delay8_14), .mode(1'b0));
processing_element pe8_15(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay23), .in_a(in_a_8_15_NC),  .in_a_chain(a8_14to8_15), .in_b(b7_15to8_15), .in_c(matrixC7_15), .out_a(out_a_8_15_NC), .out_a_chain(a8_15to8_16), .out_b(b8_15to9_15), .out_b0(b8_15to9_15_ping), .out_b1(b8_15to9_15_pong), .out_c(matrixC8_15), .b_data_valid_ping(b_data_valid_ping_delay8_15), .b_data_valid_pong(b_data_valid_pong_delay8_15), .mode(1'b0));
processing_element pe9_0(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay9), .in_a(a9),   .in_a_chain(in_a_chain_9_0_NC),   .in_b(b8_0to9_0), .in_c(matrixC8_0), .out_a(out_a_9_0_NC), .out_a_chain(a9_0to9_1), .out_b(b9_0to10_0), .out_b0(b9_0to10_0_ping), .out_b1(b9_0to10_0_pong), .out_c(matrixC9_0), .b_data_valid_ping(b_data_valid_ping_delay9_0), .b_data_valid_pong(b_data_valid_pong_delay9_0), .mode(1'b1));
processing_element pe9_1(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay10), .in_a(in_a_9_1_NC),  .in_a_chain(a9_0to9_1), .in_b(b8_1to9_1), .in_c(matrixC8_1), .out_a(out_a_9_1_NC), .out_a_chain(a9_1to9_2), .out_b(b9_1to10_1), .out_b0(b9_1to10_1_ping), .out_b1(b9_1to10_1_pong), .out_c(matrixC9_1), .b_data_valid_ping(b_data_valid_ping_delay9_1), .b_data_valid_pong(b_data_valid_pong_delay9_1), .mode(1'b0));
processing_element pe9_2(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay11), .in_a(in_a_9_2_NC),  .in_a_chain(a9_1to9_2), .in_b(b8_2to9_2), .in_c(matrixC8_2), .out_a(out_a_9_2_NC), .out_a_chain(a9_2to9_3), .out_b(b9_2to10_2), .out_b0(b9_2to10_2_ping), .out_b1(b9_2to10_2_pong), .out_c(matrixC9_2), .b_data_valid_ping(b_data_valid_ping_delay9_2), .b_data_valid_pong(b_data_valid_pong_delay9_2), .mode(1'b0));
processing_element pe9_3(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay12), .in_a(in_a_9_3_NC),  .in_a_chain(a9_2to9_3), .in_b(b8_3to9_3), .in_c(matrixC8_3), .out_a(out_a_9_3_NC), .out_a_chain(a9_3to9_4), .out_b(b9_3to10_3), .out_b0(b9_3to10_3_ping), .out_b1(b9_3to10_3_pong), .out_c(matrixC9_3), .b_data_valid_ping(b_data_valid_ping_delay9_3), .b_data_valid_pong(b_data_valid_pong_delay9_3), .mode(1'b0));
processing_element pe9_4(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay13), .in_a(in_a_9_4_NC),  .in_a_chain(a9_3to9_4), .in_b(b8_4to9_4), .in_c(matrixC8_4), .out_a(out_a_9_4_NC), .out_a_chain(a9_4to9_5), .out_b(b9_4to10_4), .out_b0(b9_4to10_4_ping), .out_b1(b9_4to10_4_pong), .out_c(matrixC9_4), .b_data_valid_ping(b_data_valid_ping_delay9_4), .b_data_valid_pong(b_data_valid_pong_delay9_4), .mode(1'b0));
processing_element pe9_5(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay14), .in_a(in_a_9_5_NC),  .in_a_chain(a9_4to9_5), .in_b(b8_5to9_5), .in_c(matrixC8_5), .out_a(out_a_9_5_NC), .out_a_chain(a9_5to9_6), .out_b(b9_5to10_5), .out_b0(b9_5to10_5_ping), .out_b1(b9_5to10_5_pong), .out_c(matrixC9_5), .b_data_valid_ping(b_data_valid_ping_delay9_5), .b_data_valid_pong(b_data_valid_pong_delay9_5), .mode(1'b0));
processing_element pe9_6(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay15), .in_a(in_a_9_6_NC),  .in_a_chain(a9_5to9_6), .in_b(b8_6to9_6), .in_c(matrixC8_6), .out_a(out_a_9_6_NC), .out_a_chain(a9_6to9_7), .out_b(b9_6to10_6), .out_b0(b9_6to10_6_ping), .out_b1(b9_6to10_6_pong), .out_c(matrixC9_6), .b_data_valid_ping(b_data_valid_ping_delay9_6), .b_data_valid_pong(b_data_valid_pong_delay9_6), .mode(1'b0));
processing_element pe9_7(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay16), .in_a(in_a_9_7_NC),  .in_a_chain(a9_6to9_7), .in_b(b8_7to9_7), .in_c(matrixC8_7), .out_a(out_a_9_7_NC), .out_a_chain(a9_7to9_8), .out_b(b9_7to10_7), .out_b0(b9_7to10_7_ping), .out_b1(b9_7to10_7_pong), .out_c(matrixC9_7), .b_data_valid_ping(b_data_valid_ping_delay9_7), .b_data_valid_pong(b_data_valid_pong_delay9_7), .mode(1'b0));
processing_element pe9_8(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay17), .in_a(in_a_9_8_NC),  .in_a_chain(a9_7to9_8), .in_b(b8_8to9_8), .in_c(matrixC8_8), .out_a(out_a_9_8_NC), .out_a_chain(a9_8to9_9), .out_b(b9_8to10_8), .out_b0(b9_8to10_8_ping), .out_b1(b9_8to10_8_pong), .out_c(matrixC9_8), .b_data_valid_ping(b_data_valid_ping_delay9_8), .b_data_valid_pong(b_data_valid_pong_delay9_8), .mode(1'b0));
processing_element pe9_9(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay18), .in_a(in_a_9_9_NC),  .in_a_chain(a9_8to9_9), .in_b(b8_9to9_9), .in_c(matrixC8_9), .out_a(out_a_9_9_NC), .out_a_chain(a9_9to9_10), .out_b(b9_9to10_9), .out_b0(b9_9to10_9_ping), .out_b1(b9_9to10_9_pong), .out_c(matrixC9_9), .b_data_valid_ping(b_data_valid_ping_delay9_9), .b_data_valid_pong(b_data_valid_pong_delay9_9), .mode(1'b0));
processing_element pe9_10(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay19), .in_a(in_a_9_10_NC),  .in_a_chain(a9_9to9_10), .in_b(b8_10to9_10), .in_c(matrixC8_10), .out_a(out_a_9_10_NC), .out_a_chain(a9_10to9_11), .out_b(b9_10to10_10), .out_b0(b9_10to10_10_ping), .out_b1(b9_10to10_10_pong), .out_c(matrixC9_10), .b_data_valid_ping(b_data_valid_ping_delay9_10), .b_data_valid_pong(b_data_valid_pong_delay9_10), .mode(1'b0));
processing_element pe9_11(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay20), .in_a(in_a_9_11_NC),  .in_a_chain(a9_10to9_11), .in_b(b8_11to9_11), .in_c(matrixC8_11), .out_a(out_a_9_11_NC), .out_a_chain(a9_11to9_12), .out_b(b9_11to10_11), .out_b0(b9_11to10_11_ping), .out_b1(b9_11to10_11_pong), .out_c(matrixC9_11), .b_data_valid_ping(b_data_valid_ping_delay9_11), .b_data_valid_pong(b_data_valid_pong_delay9_11), .mode(1'b0));
processing_element pe9_12(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay21), .in_a(in_a_9_12_NC),  .in_a_chain(a9_11to9_12), .in_b(b8_12to9_12), .in_c(matrixC8_12), .out_a(out_a_9_12_NC), .out_a_chain(a9_12to9_13), .out_b(b9_12to10_12), .out_b0(b9_12to10_12_ping), .out_b1(b9_12to10_12_pong), .out_c(matrixC9_12), .b_data_valid_ping(b_data_valid_ping_delay9_12), .b_data_valid_pong(b_data_valid_pong_delay9_12), .mode(1'b0));
processing_element pe9_13(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay22), .in_a(in_a_9_13_NC),  .in_a_chain(a9_12to9_13), .in_b(b8_13to9_13), .in_c(matrixC8_13), .out_a(out_a_9_13_NC), .out_a_chain(a9_13to9_14), .out_b(b9_13to10_13), .out_b0(b9_13to10_13_ping), .out_b1(b9_13to10_13_pong), .out_c(matrixC9_13), .b_data_valid_ping(b_data_valid_ping_delay9_13), .b_data_valid_pong(b_data_valid_pong_delay9_13), .mode(1'b0));
processing_element pe9_14(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay23), .in_a(in_a_9_14_NC),  .in_a_chain(a9_13to9_14), .in_b(b8_14to9_14), .in_c(matrixC8_14), .out_a(out_a_9_14_NC), .out_a_chain(a9_14to9_15), .out_b(b9_14to10_14), .out_b0(b9_14to10_14_ping), .out_b1(b9_14to10_14_pong), .out_c(matrixC9_14), .b_data_valid_ping(b_data_valid_ping_delay9_14), .b_data_valid_pong(b_data_valid_pong_delay9_14), .mode(1'b0));
processing_element pe9_15(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay24), .in_a(in_a_9_15_NC),  .in_a_chain(a9_14to9_15), .in_b(b8_15to9_15), .in_c(matrixC8_15), .out_a(out_a_9_15_NC), .out_a_chain(a9_15to9_16), .out_b(b9_15to10_15), .out_b0(b9_15to10_15_ping), .out_b1(b9_15to10_15_pong), .out_c(matrixC9_15), .b_data_valid_ping(b_data_valid_ping_delay9_15), .b_data_valid_pong(b_data_valid_pong_delay9_15), .mode(1'b0));
processing_element pe10_0(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay10), .in_a(a10),   .in_a_chain(in_a_chain_10_0_NC),   .in_b(b9_0to10_0), .in_c(matrixC9_0), .out_a(out_a_10_0_NC), .out_a_chain(a10_0to10_1), .out_b(b10_0to11_0), .out_b0(b10_0to11_0_ping), .out_b1(b10_0to11_0_pong), .out_c(matrixC10_0), .b_data_valid_ping(b_data_valid_ping_delay10_0), .b_data_valid_pong(b_data_valid_pong_delay10_0), .mode(1'b1));
processing_element pe10_1(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay11), .in_a(in_a_10_1_NC),  .in_a_chain(a10_0to10_1), .in_b(b9_1to10_1), .in_c(matrixC9_1), .out_a(out_a_10_1_NC), .out_a_chain(a10_1to10_2), .out_b(b10_1to11_1), .out_b0(b10_1to11_1_ping), .out_b1(b10_1to11_1_pong), .out_c(matrixC10_1), .b_data_valid_ping(b_data_valid_ping_delay10_1), .b_data_valid_pong(b_data_valid_pong_delay10_1), .mode(1'b0));
processing_element pe10_2(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay12), .in_a(in_a_10_2_NC),  .in_a_chain(a10_1to10_2), .in_b(b9_2to10_2), .in_c(matrixC9_2), .out_a(out_a_10_2_NC), .out_a_chain(a10_2to10_3), .out_b(b10_2to11_2), .out_b0(b10_2to11_2_ping), .out_b1(b10_2to11_2_pong), .out_c(matrixC10_2), .b_data_valid_ping(b_data_valid_ping_delay10_2), .b_data_valid_pong(b_data_valid_pong_delay10_2), .mode(1'b0));
processing_element pe10_3(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay13), .in_a(in_a_10_3_NC),  .in_a_chain(a10_2to10_3), .in_b(b9_3to10_3), .in_c(matrixC9_3), .out_a(out_a_10_3_NC), .out_a_chain(a10_3to10_4), .out_b(b10_3to11_3), .out_b0(b10_3to11_3_ping), .out_b1(b10_3to11_3_pong), .out_c(matrixC10_3), .b_data_valid_ping(b_data_valid_ping_delay10_3), .b_data_valid_pong(b_data_valid_pong_delay10_3), .mode(1'b0));
processing_element pe10_4(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay14), .in_a(in_a_10_4_NC),  .in_a_chain(a10_3to10_4), .in_b(b9_4to10_4), .in_c(matrixC9_4), .out_a(out_a_10_4_NC), .out_a_chain(a10_4to10_5), .out_b(b10_4to11_4), .out_b0(b10_4to11_4_ping), .out_b1(b10_4to11_4_pong), .out_c(matrixC10_4), .b_data_valid_ping(b_data_valid_ping_delay10_4), .b_data_valid_pong(b_data_valid_pong_delay10_4), .mode(1'b0));
processing_element pe10_5(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay15), .in_a(in_a_10_5_NC),  .in_a_chain(a10_4to10_5), .in_b(b9_5to10_5), .in_c(matrixC9_5), .out_a(out_a_10_5_NC), .out_a_chain(a10_5to10_6), .out_b(b10_5to11_5), .out_b0(b10_5to11_5_ping), .out_b1(b10_5to11_5_pong), .out_c(matrixC10_5), .b_data_valid_ping(b_data_valid_ping_delay10_5), .b_data_valid_pong(b_data_valid_pong_delay10_5), .mode(1'b0));
processing_element pe10_6(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay16), .in_a(in_a_10_6_NC),  .in_a_chain(a10_5to10_6), .in_b(b9_6to10_6), .in_c(matrixC9_6), .out_a(out_a_10_6_NC), .out_a_chain(a10_6to10_7), .out_b(b10_6to11_6), .out_b0(b10_6to11_6_ping), .out_b1(b10_6to11_6_pong), .out_c(matrixC10_6), .b_data_valid_ping(b_data_valid_ping_delay10_6), .b_data_valid_pong(b_data_valid_pong_delay10_6), .mode(1'b0));
processing_element pe10_7(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay17), .in_a(in_a_10_7_NC),  .in_a_chain(a10_6to10_7), .in_b(b9_7to10_7), .in_c(matrixC9_7), .out_a(out_a_10_7_NC), .out_a_chain(a10_7to10_8), .out_b(b10_7to11_7), .out_b0(b10_7to11_7_ping), .out_b1(b10_7to11_7_pong), .out_c(matrixC10_7), .b_data_valid_ping(b_data_valid_ping_delay10_7), .b_data_valid_pong(b_data_valid_pong_delay10_7), .mode(1'b0));
processing_element pe10_8(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay18), .in_a(in_a_10_8_NC),  .in_a_chain(a10_7to10_8), .in_b(b9_8to10_8), .in_c(matrixC9_8), .out_a(out_a_10_8_NC), .out_a_chain(a10_8to10_9), .out_b(b10_8to11_8), .out_b0(b10_8to11_8_ping), .out_b1(b10_8to11_8_pong), .out_c(matrixC10_8), .b_data_valid_ping(b_data_valid_ping_delay10_8), .b_data_valid_pong(b_data_valid_pong_delay10_8), .mode(1'b0));
processing_element pe10_9(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay19), .in_a(in_a_10_9_NC),  .in_a_chain(a10_8to10_9), .in_b(b9_9to10_9), .in_c(matrixC9_9), .out_a(out_a_10_9_NC), .out_a_chain(a10_9to10_10), .out_b(b10_9to11_9), .out_b0(b10_9to11_9_ping), .out_b1(b10_9to11_9_pong), .out_c(matrixC10_9), .b_data_valid_ping(b_data_valid_ping_delay10_9), .b_data_valid_pong(b_data_valid_pong_delay10_9), .mode(1'b0));
processing_element pe10_10(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay20), .in_a(in_a_10_10_NC),  .in_a_chain(a10_9to10_10), .in_b(b9_10to10_10), .in_c(matrixC9_10), .out_a(out_a_10_10_NC), .out_a_chain(a10_10to10_11), .out_b(b10_10to11_10), .out_b0(b10_10to11_10_ping), .out_b1(b10_10to11_10_pong), .out_c(matrixC10_10), .b_data_valid_ping(b_data_valid_ping_delay10_10), .b_data_valid_pong(b_data_valid_pong_delay10_10), .mode(1'b0));
processing_element pe10_11(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay21), .in_a(in_a_10_11_NC),  .in_a_chain(a10_10to10_11), .in_b(b9_11to10_11), .in_c(matrixC9_11), .out_a(out_a_10_11_NC), .out_a_chain(a10_11to10_12), .out_b(b10_11to11_11), .out_b0(b10_11to11_11_ping), .out_b1(b10_11to11_11_pong), .out_c(matrixC10_11), .b_data_valid_ping(b_data_valid_ping_delay10_11), .b_data_valid_pong(b_data_valid_pong_delay10_11), .mode(1'b0));
processing_element pe10_12(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay22), .in_a(in_a_10_12_NC),  .in_a_chain(a10_11to10_12), .in_b(b9_12to10_12), .in_c(matrixC9_12), .out_a(out_a_10_12_NC), .out_a_chain(a10_12to10_13), .out_b(b10_12to11_12), .out_b0(b10_12to11_12_ping), .out_b1(b10_12to11_12_pong), .out_c(matrixC10_12), .b_data_valid_ping(b_data_valid_ping_delay10_12), .b_data_valid_pong(b_data_valid_pong_delay10_12), .mode(1'b0));
processing_element pe10_13(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay23), .in_a(in_a_10_13_NC),  .in_a_chain(a10_12to10_13), .in_b(b9_13to10_13), .in_c(matrixC9_13), .out_a(out_a_10_13_NC), .out_a_chain(a10_13to10_14), .out_b(b10_13to11_13), .out_b0(b10_13to11_13_ping), .out_b1(b10_13to11_13_pong), .out_c(matrixC10_13), .b_data_valid_ping(b_data_valid_ping_delay10_13), .b_data_valid_pong(b_data_valid_pong_delay10_13), .mode(1'b0));
processing_element pe10_14(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay24), .in_a(in_a_10_14_NC),  .in_a_chain(a10_13to10_14), .in_b(b9_14to10_14), .in_c(matrixC9_14), .out_a(out_a_10_14_NC), .out_a_chain(a10_14to10_15), .out_b(b10_14to11_14), .out_b0(b10_14to11_14_ping), .out_b1(b10_14to11_14_pong), .out_c(matrixC10_14), .b_data_valid_ping(b_data_valid_ping_delay10_14), .b_data_valid_pong(b_data_valid_pong_delay10_14), .mode(1'b0));
processing_element pe10_15(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay25), .in_a(in_a_10_15_NC),  .in_a_chain(a10_14to10_15), .in_b(b9_15to10_15), .in_c(matrixC9_15), .out_a(out_a_10_15_NC), .out_a_chain(a10_15to10_16), .out_b(b10_15to11_15), .out_b0(b10_15to11_15_ping), .out_b1(b10_15to11_15_pong), .out_c(matrixC10_15), .b_data_valid_ping(b_data_valid_ping_delay10_15), .b_data_valid_pong(b_data_valid_pong_delay10_15), .mode(1'b0));
processing_element pe11_0(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay11), .in_a(a11),   .in_a_chain(in_a_chain_11_0_NC),   .in_b(b10_0to11_0), .in_c(matrixC10_0), .out_a(out_a_11_0_NC), .out_a_chain(a11_0to11_1), .out_b(b11_0to12_0), .out_b0(b11_0to12_0_ping), .out_b1(b11_0to12_0_pong), .out_c(matrixC11_0), .b_data_valid_ping(b_data_valid_ping_delay11_0), .b_data_valid_pong(b_data_valid_pong_delay11_0), .mode(1'b1));
processing_element pe11_1(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay12), .in_a(in_a_11_1_NC),  .in_a_chain(a11_0to11_1), .in_b(b10_1to11_1), .in_c(matrixC10_1), .out_a(out_a_11_1_NC), .out_a_chain(a11_1to11_2), .out_b(b11_1to12_1), .out_b0(b11_1to12_1_ping), .out_b1(b11_1to12_1_pong), .out_c(matrixC11_1), .b_data_valid_ping(b_data_valid_ping_delay11_1), .b_data_valid_pong(b_data_valid_pong_delay11_1), .mode(1'b0));
processing_element pe11_2(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay13), .in_a(in_a_11_2_NC),  .in_a_chain(a11_1to11_2), .in_b(b10_2to11_2), .in_c(matrixC10_2), .out_a(out_a_11_2_NC), .out_a_chain(a11_2to11_3), .out_b(b11_2to12_2), .out_b0(b11_2to12_2_ping), .out_b1(b11_2to12_2_pong), .out_c(matrixC11_2), .b_data_valid_ping(b_data_valid_ping_delay11_2), .b_data_valid_pong(b_data_valid_pong_delay11_2), .mode(1'b0));
processing_element pe11_3(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay14), .in_a(in_a_11_3_NC),  .in_a_chain(a11_2to11_3), .in_b(b10_3to11_3), .in_c(matrixC10_3), .out_a(out_a_11_3_NC), .out_a_chain(a11_3to11_4), .out_b(b11_3to12_3), .out_b0(b11_3to12_3_ping), .out_b1(b11_3to12_3_pong), .out_c(matrixC11_3), .b_data_valid_ping(b_data_valid_ping_delay11_3), .b_data_valid_pong(b_data_valid_pong_delay11_3), .mode(1'b0));
processing_element pe11_4(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay15), .in_a(in_a_11_4_NC),  .in_a_chain(a11_3to11_4), .in_b(b10_4to11_4), .in_c(matrixC10_4), .out_a(out_a_11_4_NC), .out_a_chain(a11_4to11_5), .out_b(b11_4to12_4), .out_b0(b11_4to12_4_ping), .out_b1(b11_4to12_4_pong), .out_c(matrixC11_4), .b_data_valid_ping(b_data_valid_ping_delay11_4), .b_data_valid_pong(b_data_valid_pong_delay11_4), .mode(1'b0));
processing_element pe11_5(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay16), .in_a(in_a_11_5_NC),  .in_a_chain(a11_4to11_5), .in_b(b10_5to11_5), .in_c(matrixC10_5), .out_a(out_a_11_5_NC), .out_a_chain(a11_5to11_6), .out_b(b11_5to12_5), .out_b0(b11_5to12_5_ping), .out_b1(b11_5to12_5_pong), .out_c(matrixC11_5), .b_data_valid_ping(b_data_valid_ping_delay11_5), .b_data_valid_pong(b_data_valid_pong_delay11_5), .mode(1'b0));
processing_element pe11_6(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay17), .in_a(in_a_11_6_NC),  .in_a_chain(a11_5to11_6), .in_b(b10_6to11_6), .in_c(matrixC10_6), .out_a(out_a_11_6_NC), .out_a_chain(a11_6to11_7), .out_b(b11_6to12_6), .out_b0(b11_6to12_6_ping), .out_b1(b11_6to12_6_pong), .out_c(matrixC11_6), .b_data_valid_ping(b_data_valid_ping_delay11_6), .b_data_valid_pong(b_data_valid_pong_delay11_6), .mode(1'b0));
processing_element pe11_7(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay18), .in_a(in_a_11_7_NC),  .in_a_chain(a11_6to11_7), .in_b(b10_7to11_7), .in_c(matrixC10_7), .out_a(out_a_11_7_NC), .out_a_chain(a11_7to11_8), .out_b(b11_7to12_7), .out_b0(b11_7to12_7_ping), .out_b1(b11_7to12_7_pong), .out_c(matrixC11_7), .b_data_valid_ping(b_data_valid_ping_delay11_7), .b_data_valid_pong(b_data_valid_pong_delay11_7), .mode(1'b0));
processing_element pe11_8(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay19), .in_a(in_a_11_8_NC),  .in_a_chain(a11_7to11_8), .in_b(b10_8to11_8), .in_c(matrixC10_8), .out_a(out_a_11_8_NC), .out_a_chain(a11_8to11_9), .out_b(b11_8to12_8), .out_b0(b11_8to12_8_ping), .out_b1(b11_8to12_8_pong), .out_c(matrixC11_8), .b_data_valid_ping(b_data_valid_ping_delay11_8), .b_data_valid_pong(b_data_valid_pong_delay11_8), .mode(1'b0));
processing_element pe11_9(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay20), .in_a(in_a_11_9_NC),  .in_a_chain(a11_8to11_9), .in_b(b10_9to11_9), .in_c(matrixC10_9), .out_a(out_a_11_9_NC), .out_a_chain(a11_9to11_10), .out_b(b11_9to12_9), .out_b0(b11_9to12_9_ping), .out_b1(b11_9to12_9_pong), .out_c(matrixC11_9), .b_data_valid_ping(b_data_valid_ping_delay11_9), .b_data_valid_pong(b_data_valid_pong_delay11_9), .mode(1'b0));
processing_element pe11_10(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay21), .in_a(in_a_11_10_NC),  .in_a_chain(a11_9to11_10), .in_b(b10_10to11_10), .in_c(matrixC10_10), .out_a(out_a_11_10_NC), .out_a_chain(a11_10to11_11), .out_b(b11_10to12_10), .out_b0(b11_10to12_10_ping), .out_b1(b11_10to12_10_pong), .out_c(matrixC11_10), .b_data_valid_ping(b_data_valid_ping_delay11_10), .b_data_valid_pong(b_data_valid_pong_delay11_10), .mode(1'b0));
processing_element pe11_11(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay22), .in_a(in_a_11_11_NC),  .in_a_chain(a11_10to11_11), .in_b(b10_11to11_11), .in_c(matrixC10_11), .out_a(out_a_11_11_NC), .out_a_chain(a11_11to11_12), .out_b(b11_11to12_11), .out_b0(b11_11to12_11_ping), .out_b1(b11_11to12_11_pong), .out_c(matrixC11_11), .b_data_valid_ping(b_data_valid_ping_delay11_11), .b_data_valid_pong(b_data_valid_pong_delay11_11), .mode(1'b0));
processing_element pe11_12(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay23), .in_a(in_a_11_12_NC),  .in_a_chain(a11_11to11_12), .in_b(b10_12to11_12), .in_c(matrixC10_12), .out_a(out_a_11_12_NC), .out_a_chain(a11_12to11_13), .out_b(b11_12to12_12), .out_b0(b11_12to12_12_ping), .out_b1(b11_12to12_12_pong), .out_c(matrixC11_12), .b_data_valid_ping(b_data_valid_ping_delay11_12), .b_data_valid_pong(b_data_valid_pong_delay11_12), .mode(1'b0));
processing_element pe11_13(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay24), .in_a(in_a_11_13_NC),  .in_a_chain(a11_12to11_13), .in_b(b10_13to11_13), .in_c(matrixC10_13), .out_a(out_a_11_13_NC), .out_a_chain(a11_13to11_14), .out_b(b11_13to12_13), .out_b0(b11_13to12_13_ping), .out_b1(b11_13to12_13_pong), .out_c(matrixC11_13), .b_data_valid_ping(b_data_valid_ping_delay11_13), .b_data_valid_pong(b_data_valid_pong_delay11_13), .mode(1'b0));
processing_element pe11_14(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay25), .in_a(in_a_11_14_NC),  .in_a_chain(a11_13to11_14), .in_b(b10_14to11_14), .in_c(matrixC10_14), .out_a(out_a_11_14_NC), .out_a_chain(a11_14to11_15), .out_b(b11_14to12_14), .out_b0(b11_14to12_14_ping), .out_b1(b11_14to12_14_pong), .out_c(matrixC11_14), .b_data_valid_ping(b_data_valid_ping_delay11_14), .b_data_valid_pong(b_data_valid_pong_delay11_14), .mode(1'b0));
processing_element pe11_15(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay26), .in_a(in_a_11_15_NC),  .in_a_chain(a11_14to11_15), .in_b(b10_15to11_15), .in_c(matrixC10_15), .out_a(out_a_11_15_NC), .out_a_chain(a11_15to11_16), .out_b(b11_15to12_15), .out_b0(b11_15to12_15_ping), .out_b1(b11_15to12_15_pong), .out_c(matrixC11_15), .b_data_valid_ping(b_data_valid_ping_delay11_15), .b_data_valid_pong(b_data_valid_pong_delay11_15), .mode(1'b0));
processing_element pe12_0(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay12), .in_a(a12),   .in_a_chain(in_a_chain_12_0_NC),   .in_b(b11_0to12_0), .in_c(matrixC11_0), .out_a(out_a_12_0_NC), .out_a_chain(a12_0to12_1), .out_b(b12_0to13_0), .out_b0(b12_0to13_0_ping), .out_b1(b12_0to13_0_pong), .out_c(matrixC12_0), .b_data_valid_ping(b_data_valid_ping_delay12_0), .b_data_valid_pong(b_data_valid_pong_delay12_0), .mode(1'b1));
processing_element pe12_1(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay13), .in_a(in_a_12_1_NC),  .in_a_chain(a12_0to12_1), .in_b(b11_1to12_1), .in_c(matrixC11_1), .out_a(out_a_12_1_NC), .out_a_chain(a12_1to12_2), .out_b(b12_1to13_1), .out_b0(b12_1to13_1_ping), .out_b1(b12_1to13_1_pong), .out_c(matrixC12_1), .b_data_valid_ping(b_data_valid_ping_delay12_1), .b_data_valid_pong(b_data_valid_pong_delay12_1), .mode(1'b0));
processing_element pe12_2(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay14), .in_a(in_a_12_2_NC),  .in_a_chain(a12_1to12_2), .in_b(b11_2to12_2), .in_c(matrixC11_2), .out_a(out_a_12_2_NC), .out_a_chain(a12_2to12_3), .out_b(b12_2to13_2), .out_b0(b12_2to13_2_ping), .out_b1(b12_2to13_2_pong), .out_c(matrixC12_2), .b_data_valid_ping(b_data_valid_ping_delay12_2), .b_data_valid_pong(b_data_valid_pong_delay12_2), .mode(1'b0));
processing_element pe12_3(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay15), .in_a(in_a_12_3_NC),  .in_a_chain(a12_2to12_3), .in_b(b11_3to12_3), .in_c(matrixC11_3), .out_a(out_a_12_3_NC), .out_a_chain(a12_3to12_4), .out_b(b12_3to13_3), .out_b0(b12_3to13_3_ping), .out_b1(b12_3to13_3_pong), .out_c(matrixC12_3), .b_data_valid_ping(b_data_valid_ping_delay12_3), .b_data_valid_pong(b_data_valid_pong_delay12_3), .mode(1'b0));
processing_element pe12_4(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay16), .in_a(in_a_12_4_NC),  .in_a_chain(a12_3to12_4), .in_b(b11_4to12_4), .in_c(matrixC11_4), .out_a(out_a_12_4_NC), .out_a_chain(a12_4to12_5), .out_b(b12_4to13_4), .out_b0(b12_4to13_4_ping), .out_b1(b12_4to13_4_pong), .out_c(matrixC12_4), .b_data_valid_ping(b_data_valid_ping_delay12_4), .b_data_valid_pong(b_data_valid_pong_delay12_4), .mode(1'b0));
processing_element pe12_5(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay17), .in_a(in_a_12_5_NC),  .in_a_chain(a12_4to12_5), .in_b(b11_5to12_5), .in_c(matrixC11_5), .out_a(out_a_12_5_NC), .out_a_chain(a12_5to12_6), .out_b(b12_5to13_5), .out_b0(b12_5to13_5_ping), .out_b1(b12_5to13_5_pong), .out_c(matrixC12_5), .b_data_valid_ping(b_data_valid_ping_delay12_5), .b_data_valid_pong(b_data_valid_pong_delay12_5), .mode(1'b0));
processing_element pe12_6(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay18), .in_a(in_a_12_6_NC),  .in_a_chain(a12_5to12_6), .in_b(b11_6to12_6), .in_c(matrixC11_6), .out_a(out_a_12_6_NC), .out_a_chain(a12_6to12_7), .out_b(b12_6to13_6), .out_b0(b12_6to13_6_ping), .out_b1(b12_6to13_6_pong), .out_c(matrixC12_6), .b_data_valid_ping(b_data_valid_ping_delay12_6), .b_data_valid_pong(b_data_valid_pong_delay12_6), .mode(1'b0));
processing_element pe12_7(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay19), .in_a(in_a_12_7_NC),  .in_a_chain(a12_6to12_7), .in_b(b11_7to12_7), .in_c(matrixC11_7), .out_a(out_a_12_7_NC), .out_a_chain(a12_7to12_8), .out_b(b12_7to13_7), .out_b0(b12_7to13_7_ping), .out_b1(b12_7to13_7_pong), .out_c(matrixC12_7), .b_data_valid_ping(b_data_valid_ping_delay12_7), .b_data_valid_pong(b_data_valid_pong_delay12_7), .mode(1'b0));
processing_element pe12_8(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay20), .in_a(in_a_12_8_NC),  .in_a_chain(a12_7to12_8), .in_b(b11_8to12_8), .in_c(matrixC11_8), .out_a(out_a_12_8_NC), .out_a_chain(a12_8to12_9), .out_b(b12_8to13_8), .out_b0(b12_8to13_8_ping), .out_b1(b12_8to13_8_pong), .out_c(matrixC12_8), .b_data_valid_ping(b_data_valid_ping_delay12_8), .b_data_valid_pong(b_data_valid_pong_delay12_8), .mode(1'b0));
processing_element pe12_9(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay21), .in_a(in_a_12_9_NC),  .in_a_chain(a12_8to12_9), .in_b(b11_9to12_9), .in_c(matrixC11_9), .out_a(out_a_12_9_NC), .out_a_chain(a12_9to12_10), .out_b(b12_9to13_9), .out_b0(b12_9to13_9_ping), .out_b1(b12_9to13_9_pong), .out_c(matrixC12_9), .b_data_valid_ping(b_data_valid_ping_delay12_9), .b_data_valid_pong(b_data_valid_pong_delay12_9), .mode(1'b0));
processing_element pe12_10(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay22), .in_a(in_a_12_10_NC),  .in_a_chain(a12_9to12_10), .in_b(b11_10to12_10), .in_c(matrixC11_10), .out_a(out_a_12_10_NC), .out_a_chain(a12_10to12_11), .out_b(b12_10to13_10), .out_b0(b12_10to13_10_ping), .out_b1(b12_10to13_10_pong), .out_c(matrixC12_10), .b_data_valid_ping(b_data_valid_ping_delay12_10), .b_data_valid_pong(b_data_valid_pong_delay12_10), .mode(1'b0));
processing_element pe12_11(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay23), .in_a(in_a_12_11_NC),  .in_a_chain(a12_10to12_11), .in_b(b11_11to12_11), .in_c(matrixC11_11), .out_a(out_a_12_11_NC), .out_a_chain(a12_11to12_12), .out_b(b12_11to13_11), .out_b0(b12_11to13_11_ping), .out_b1(b12_11to13_11_pong), .out_c(matrixC12_11), .b_data_valid_ping(b_data_valid_ping_delay12_11), .b_data_valid_pong(b_data_valid_pong_delay12_11), .mode(1'b0));
processing_element pe12_12(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay24), .in_a(in_a_12_12_NC),  .in_a_chain(a12_11to12_12), .in_b(b11_12to12_12), .in_c(matrixC11_12), .out_a(out_a_12_12_NC), .out_a_chain(a12_12to12_13), .out_b(b12_12to13_12), .out_b0(b12_12to13_12_ping), .out_b1(b12_12to13_12_pong), .out_c(matrixC12_12), .b_data_valid_ping(b_data_valid_ping_delay12_12), .b_data_valid_pong(b_data_valid_pong_delay12_12), .mode(1'b0));
processing_element pe12_13(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay25), .in_a(in_a_12_13_NC),  .in_a_chain(a12_12to12_13), .in_b(b11_13to12_13), .in_c(matrixC11_13), .out_a(out_a_12_13_NC), .out_a_chain(a12_13to12_14), .out_b(b12_13to13_13), .out_b0(b12_13to13_13_ping), .out_b1(b12_13to13_13_pong), .out_c(matrixC12_13), .b_data_valid_ping(b_data_valid_ping_delay12_13), .b_data_valid_pong(b_data_valid_pong_delay12_13), .mode(1'b0));
processing_element pe12_14(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay26), .in_a(in_a_12_14_NC),  .in_a_chain(a12_13to12_14), .in_b(b11_14to12_14), .in_c(matrixC11_14), .out_a(out_a_12_14_NC), .out_a_chain(a12_14to12_15), .out_b(b12_14to13_14), .out_b0(b12_14to13_14_ping), .out_b1(b12_14to13_14_pong), .out_c(matrixC12_14), .b_data_valid_ping(b_data_valid_ping_delay12_14), .b_data_valid_pong(b_data_valid_pong_delay12_14), .mode(1'b0));
processing_element pe12_15(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay27), .in_a(in_a_12_15_NC),  .in_a_chain(a12_14to12_15), .in_b(b11_15to12_15), .in_c(matrixC11_15), .out_a(out_a_12_15_NC), .out_a_chain(a12_15to12_16), .out_b(b12_15to13_15), .out_b0(b12_15to13_15_ping), .out_b1(b12_15to13_15_pong), .out_c(matrixC12_15), .b_data_valid_ping(b_data_valid_ping_delay12_15), .b_data_valid_pong(b_data_valid_pong_delay12_15), .mode(1'b0));
processing_element pe13_0(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay13), .in_a(a13),   .in_a_chain(in_a_chain_13_0_NC),   .in_b(b12_0to13_0), .in_c(matrixC12_0), .out_a(out_a_13_0_NC), .out_a_chain(a13_0to13_1), .out_b(b13_0to14_0), .out_b0(b13_0to14_0_ping), .out_b1(b13_0to14_0_pong), .out_c(matrixC13_0), .b_data_valid_ping(b_data_valid_ping_delay13_0), .b_data_valid_pong(b_data_valid_pong_delay13_0), .mode(1'b1));
processing_element pe13_1(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay14), .in_a(in_a_13_1_NC),  .in_a_chain(a13_0to13_1), .in_b(b12_1to13_1), .in_c(matrixC12_1), .out_a(out_a_13_1_NC), .out_a_chain(a13_1to13_2), .out_b(b13_1to14_1), .out_b0(b13_1to14_1_ping), .out_b1(b13_1to14_1_pong), .out_c(matrixC13_1), .b_data_valid_ping(b_data_valid_ping_delay13_1), .b_data_valid_pong(b_data_valid_pong_delay13_1), .mode(1'b0));
processing_element pe13_2(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay15), .in_a(in_a_13_2_NC),  .in_a_chain(a13_1to13_2), .in_b(b12_2to13_2), .in_c(matrixC12_2), .out_a(out_a_13_2_NC), .out_a_chain(a13_2to13_3), .out_b(b13_2to14_2), .out_b0(b13_2to14_2_ping), .out_b1(b13_2to14_2_pong), .out_c(matrixC13_2), .b_data_valid_ping(b_data_valid_ping_delay13_2), .b_data_valid_pong(b_data_valid_pong_delay13_2), .mode(1'b0));
processing_element pe13_3(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay16), .in_a(in_a_13_3_NC),  .in_a_chain(a13_2to13_3), .in_b(b12_3to13_3), .in_c(matrixC12_3), .out_a(out_a_13_3_NC), .out_a_chain(a13_3to13_4), .out_b(b13_3to14_3), .out_b0(b13_3to14_3_ping), .out_b1(b13_3to14_3_pong), .out_c(matrixC13_3), .b_data_valid_ping(b_data_valid_ping_delay13_3), .b_data_valid_pong(b_data_valid_pong_delay13_3), .mode(1'b0));
processing_element pe13_4(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay17), .in_a(in_a_13_4_NC),  .in_a_chain(a13_3to13_4), .in_b(b12_4to13_4), .in_c(matrixC12_4), .out_a(out_a_13_4_NC), .out_a_chain(a13_4to13_5), .out_b(b13_4to14_4), .out_b0(b13_4to14_4_ping), .out_b1(b13_4to14_4_pong), .out_c(matrixC13_4), .b_data_valid_ping(b_data_valid_ping_delay13_4), .b_data_valid_pong(b_data_valid_pong_delay13_4), .mode(1'b0));
processing_element pe13_5(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay18), .in_a(in_a_13_5_NC),  .in_a_chain(a13_4to13_5), .in_b(b12_5to13_5), .in_c(matrixC12_5), .out_a(out_a_13_5_NC), .out_a_chain(a13_5to13_6), .out_b(b13_5to14_5), .out_b0(b13_5to14_5_ping), .out_b1(b13_5to14_5_pong), .out_c(matrixC13_5), .b_data_valid_ping(b_data_valid_ping_delay13_5), .b_data_valid_pong(b_data_valid_pong_delay13_5), .mode(1'b0));
processing_element pe13_6(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay19), .in_a(in_a_13_6_NC),  .in_a_chain(a13_5to13_6), .in_b(b12_6to13_6), .in_c(matrixC12_6), .out_a(out_a_13_6_NC), .out_a_chain(a13_6to13_7), .out_b(b13_6to14_6), .out_b0(b13_6to14_6_ping), .out_b1(b13_6to14_6_pong), .out_c(matrixC13_6), .b_data_valid_ping(b_data_valid_ping_delay13_6), .b_data_valid_pong(b_data_valid_pong_delay13_6), .mode(1'b0));
processing_element pe13_7(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay20), .in_a(in_a_13_7_NC),  .in_a_chain(a13_6to13_7), .in_b(b12_7to13_7), .in_c(matrixC12_7), .out_a(out_a_13_7_NC), .out_a_chain(a13_7to13_8), .out_b(b13_7to14_7), .out_b0(b13_7to14_7_ping), .out_b1(b13_7to14_7_pong), .out_c(matrixC13_7), .b_data_valid_ping(b_data_valid_ping_delay13_7), .b_data_valid_pong(b_data_valid_pong_delay13_7), .mode(1'b0));
processing_element pe13_8(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay21), .in_a(in_a_13_8_NC),  .in_a_chain(a13_7to13_8), .in_b(b12_8to13_8), .in_c(matrixC12_8), .out_a(out_a_13_8_NC), .out_a_chain(a13_8to13_9), .out_b(b13_8to14_8), .out_b0(b13_8to14_8_ping), .out_b1(b13_8to14_8_pong), .out_c(matrixC13_8), .b_data_valid_ping(b_data_valid_ping_delay13_8), .b_data_valid_pong(b_data_valid_pong_delay13_8), .mode(1'b0));
processing_element pe13_9(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay22), .in_a(in_a_13_9_NC),  .in_a_chain(a13_8to13_9), .in_b(b12_9to13_9), .in_c(matrixC12_9), .out_a(out_a_13_9_NC), .out_a_chain(a13_9to13_10), .out_b(b13_9to14_9), .out_b0(b13_9to14_9_ping), .out_b1(b13_9to14_9_pong), .out_c(matrixC13_9), .b_data_valid_ping(b_data_valid_ping_delay13_9), .b_data_valid_pong(b_data_valid_pong_delay13_9), .mode(1'b0));
processing_element pe13_10(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay23), .in_a(in_a_13_10_NC),  .in_a_chain(a13_9to13_10), .in_b(b12_10to13_10), .in_c(matrixC12_10), .out_a(out_a_13_10_NC), .out_a_chain(a13_10to13_11), .out_b(b13_10to14_10), .out_b0(b13_10to14_10_ping), .out_b1(b13_10to14_10_pong), .out_c(matrixC13_10), .b_data_valid_ping(b_data_valid_ping_delay13_10), .b_data_valid_pong(b_data_valid_pong_delay13_10), .mode(1'b0));
processing_element pe13_11(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay24), .in_a(in_a_13_11_NC),  .in_a_chain(a13_10to13_11), .in_b(b12_11to13_11), .in_c(matrixC12_11), .out_a(out_a_13_11_NC), .out_a_chain(a13_11to13_12), .out_b(b13_11to14_11), .out_b0(b13_11to14_11_ping), .out_b1(b13_11to14_11_pong), .out_c(matrixC13_11), .b_data_valid_ping(b_data_valid_ping_delay13_11), .b_data_valid_pong(b_data_valid_pong_delay13_11), .mode(1'b0));
processing_element pe13_12(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay25), .in_a(in_a_13_12_NC),  .in_a_chain(a13_11to13_12), .in_b(b12_12to13_12), .in_c(matrixC12_12), .out_a(out_a_13_12_NC), .out_a_chain(a13_12to13_13), .out_b(b13_12to14_12), .out_b0(b13_12to14_12_ping), .out_b1(b13_12to14_12_pong), .out_c(matrixC13_12), .b_data_valid_ping(b_data_valid_ping_delay13_12), .b_data_valid_pong(b_data_valid_pong_delay13_12), .mode(1'b0));
processing_element pe13_13(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay26), .in_a(in_a_13_13_NC),  .in_a_chain(a13_12to13_13), .in_b(b12_13to13_13), .in_c(matrixC12_13), .out_a(out_a_13_13_NC), .out_a_chain(a13_13to13_14), .out_b(b13_13to14_13), .out_b0(b13_13to14_13_ping), .out_b1(b13_13to14_13_pong), .out_c(matrixC13_13), .b_data_valid_ping(b_data_valid_ping_delay13_13), .b_data_valid_pong(b_data_valid_pong_delay13_13), .mode(1'b0));
processing_element pe13_14(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay27), .in_a(in_a_13_14_NC),  .in_a_chain(a13_13to13_14), .in_b(b12_14to13_14), .in_c(matrixC12_14), .out_a(out_a_13_14_NC), .out_a_chain(a13_14to13_15), .out_b(b13_14to14_14), .out_b0(b13_14to14_14_ping), .out_b1(b13_14to14_14_pong), .out_c(matrixC13_14), .b_data_valid_ping(b_data_valid_ping_delay13_14), .b_data_valid_pong(b_data_valid_pong_delay13_14), .mode(1'b0));
processing_element pe13_15(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay28), .in_a(in_a_13_15_NC),  .in_a_chain(a13_14to13_15), .in_b(b12_15to13_15), .in_c(matrixC12_15), .out_a(out_a_13_15_NC), .out_a_chain(a13_15to13_16), .out_b(b13_15to14_15), .out_b0(b13_15to14_15_ping), .out_b1(b13_15to14_15_pong), .out_c(matrixC13_15), .b_data_valid_ping(b_data_valid_ping_delay13_15), .b_data_valid_pong(b_data_valid_pong_delay13_15), .mode(1'b0));
processing_element pe14_0(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay14), .in_a(a14),   .in_a_chain(in_a_chain_14_0_NC),   .in_b(b13_0to14_0), .in_c(matrixC13_0), .out_a(out_a_14_0_NC), .out_a_chain(a14_0to14_1), .out_b(b14_0to15_0), .out_b0(b14_0to15_0_ping), .out_b1(b14_0to15_0_pong), .out_c(matrixC14_0), .b_data_valid_ping(b_data_valid_ping_delay14_0), .b_data_valid_pong(b_data_valid_pong_delay14_0), .mode(1'b1));
processing_element pe14_1(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay15), .in_a(in_a_14_1_NC),  .in_a_chain(a14_0to14_1), .in_b(b13_1to14_1), .in_c(matrixC13_1), .out_a(out_a_14_1_NC), .out_a_chain(a14_1to14_2), .out_b(b14_1to15_1), .out_b0(b14_1to15_1_ping), .out_b1(b14_1to15_1_pong), .out_c(matrixC14_1), .b_data_valid_ping(b_data_valid_ping_delay14_1), .b_data_valid_pong(b_data_valid_pong_delay14_1), .mode(1'b0));
processing_element pe14_2(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay16), .in_a(in_a_14_2_NC),  .in_a_chain(a14_1to14_2), .in_b(b13_2to14_2), .in_c(matrixC13_2), .out_a(out_a_14_2_NC), .out_a_chain(a14_2to14_3), .out_b(b14_2to15_2), .out_b0(b14_2to15_2_ping), .out_b1(b14_2to15_2_pong), .out_c(matrixC14_2), .b_data_valid_ping(b_data_valid_ping_delay14_2), .b_data_valid_pong(b_data_valid_pong_delay14_2), .mode(1'b0));
processing_element pe14_3(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay17), .in_a(in_a_14_3_NC),  .in_a_chain(a14_2to14_3), .in_b(b13_3to14_3), .in_c(matrixC13_3), .out_a(out_a_14_3_NC), .out_a_chain(a14_3to14_4), .out_b(b14_3to15_3), .out_b0(b14_3to15_3_ping), .out_b1(b14_3to15_3_pong), .out_c(matrixC14_3), .b_data_valid_ping(b_data_valid_ping_delay14_3), .b_data_valid_pong(b_data_valid_pong_delay14_3), .mode(1'b0));
processing_element pe14_4(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay18), .in_a(in_a_14_4_NC),  .in_a_chain(a14_3to14_4), .in_b(b13_4to14_4), .in_c(matrixC13_4), .out_a(out_a_14_4_NC), .out_a_chain(a14_4to14_5), .out_b(b14_4to15_4), .out_b0(b14_4to15_4_ping), .out_b1(b14_4to15_4_pong), .out_c(matrixC14_4), .b_data_valid_ping(b_data_valid_ping_delay14_4), .b_data_valid_pong(b_data_valid_pong_delay14_4), .mode(1'b0));
processing_element pe14_5(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay19), .in_a(in_a_14_5_NC),  .in_a_chain(a14_4to14_5), .in_b(b13_5to14_5), .in_c(matrixC13_5), .out_a(out_a_14_5_NC), .out_a_chain(a14_5to14_6), .out_b(b14_5to15_5), .out_b0(b14_5to15_5_ping), .out_b1(b14_5to15_5_pong), .out_c(matrixC14_5), .b_data_valid_ping(b_data_valid_ping_delay14_5), .b_data_valid_pong(b_data_valid_pong_delay14_5), .mode(1'b0));
processing_element pe14_6(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay20), .in_a(in_a_14_6_NC),  .in_a_chain(a14_5to14_6), .in_b(b13_6to14_6), .in_c(matrixC13_6), .out_a(out_a_14_6_NC), .out_a_chain(a14_6to14_7), .out_b(b14_6to15_6), .out_b0(b14_6to15_6_ping), .out_b1(b14_6to15_6_pong), .out_c(matrixC14_6), .b_data_valid_ping(b_data_valid_ping_delay14_6), .b_data_valid_pong(b_data_valid_pong_delay14_6), .mode(1'b0));
processing_element pe14_7(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay21), .in_a(in_a_14_7_NC),  .in_a_chain(a14_6to14_7), .in_b(b13_7to14_7), .in_c(matrixC13_7), .out_a(out_a_14_7_NC), .out_a_chain(a14_7to14_8), .out_b(b14_7to15_7), .out_b0(b14_7to15_7_ping), .out_b1(b14_7to15_7_pong), .out_c(matrixC14_7), .b_data_valid_ping(b_data_valid_ping_delay14_7), .b_data_valid_pong(b_data_valid_pong_delay14_7), .mode(1'b0));
processing_element pe14_8(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay22), .in_a(in_a_14_8_NC),  .in_a_chain(a14_7to14_8), .in_b(b13_8to14_8), .in_c(matrixC13_8), .out_a(out_a_14_8_NC), .out_a_chain(a14_8to14_9), .out_b(b14_8to15_8), .out_b0(b14_8to15_8_ping), .out_b1(b14_8to15_8_pong), .out_c(matrixC14_8), .b_data_valid_ping(b_data_valid_ping_delay14_8), .b_data_valid_pong(b_data_valid_pong_delay14_8), .mode(1'b0));
processing_element pe14_9(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay23), .in_a(in_a_14_9_NC),  .in_a_chain(a14_8to14_9), .in_b(b13_9to14_9), .in_c(matrixC13_9), .out_a(out_a_14_9_NC), .out_a_chain(a14_9to14_10), .out_b(b14_9to15_9), .out_b0(b14_9to15_9_ping), .out_b1(b14_9to15_9_pong), .out_c(matrixC14_9), .b_data_valid_ping(b_data_valid_ping_delay14_9), .b_data_valid_pong(b_data_valid_pong_delay14_9), .mode(1'b0));
processing_element pe14_10(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay24), .in_a(in_a_14_10_NC),  .in_a_chain(a14_9to14_10), .in_b(b13_10to14_10), .in_c(matrixC13_10), .out_a(out_a_14_10_NC), .out_a_chain(a14_10to14_11), .out_b(b14_10to15_10), .out_b0(b14_10to15_10_ping), .out_b1(b14_10to15_10_pong), .out_c(matrixC14_10), .b_data_valid_ping(b_data_valid_ping_delay14_10), .b_data_valid_pong(b_data_valid_pong_delay14_10), .mode(1'b0));
processing_element pe14_11(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay25), .in_a(in_a_14_11_NC),  .in_a_chain(a14_10to14_11), .in_b(b13_11to14_11), .in_c(matrixC13_11), .out_a(out_a_14_11_NC), .out_a_chain(a14_11to14_12), .out_b(b14_11to15_11), .out_b0(b14_11to15_11_ping), .out_b1(b14_11to15_11_pong), .out_c(matrixC14_11), .b_data_valid_ping(b_data_valid_ping_delay14_11), .b_data_valid_pong(b_data_valid_pong_delay14_11), .mode(1'b0));
processing_element pe14_12(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay26), .in_a(in_a_14_12_NC),  .in_a_chain(a14_11to14_12), .in_b(b13_12to14_12), .in_c(matrixC13_12), .out_a(out_a_14_12_NC), .out_a_chain(a14_12to14_13), .out_b(b14_12to15_12), .out_b0(b14_12to15_12_ping), .out_b1(b14_12to15_12_pong), .out_c(matrixC14_12), .b_data_valid_ping(b_data_valid_ping_delay14_12), .b_data_valid_pong(b_data_valid_pong_delay14_12), .mode(1'b0));
processing_element pe14_13(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay27), .in_a(in_a_14_13_NC),  .in_a_chain(a14_12to14_13), .in_b(b13_13to14_13), .in_c(matrixC13_13), .out_a(out_a_14_13_NC), .out_a_chain(a14_13to14_14), .out_b(b14_13to15_13), .out_b0(b14_13to15_13_ping), .out_b1(b14_13to15_13_pong), .out_c(matrixC14_13), .b_data_valid_ping(b_data_valid_ping_delay14_13), .b_data_valid_pong(b_data_valid_pong_delay14_13), .mode(1'b0));
processing_element pe14_14(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay28), .in_a(in_a_14_14_NC),  .in_a_chain(a14_13to14_14), .in_b(b13_14to14_14), .in_c(matrixC13_14), .out_a(out_a_14_14_NC), .out_a_chain(a14_14to14_15), .out_b(b14_14to15_14), .out_b0(b14_14to15_14_ping), .out_b1(b14_14to15_14_pong), .out_c(matrixC14_14), .b_data_valid_ping(b_data_valid_ping_delay14_14), .b_data_valid_pong(b_data_valid_pong_delay14_14), .mode(1'b0));
processing_element pe14_15(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay29), .in_a(in_a_14_15_NC),  .in_a_chain(a14_14to14_15), .in_b(b13_15to14_15), .in_c(matrixC13_15), .out_a(out_a_14_15_NC), .out_a_chain(a14_15to14_16), .out_b(b14_15to15_15), .out_b0(b14_15to15_15_ping), .out_b1(b14_15to15_15_pong), .out_c(matrixC14_15), .b_data_valid_ping(b_data_valid_ping_delay14_15), .b_data_valid_pong(b_data_valid_pong_delay14_15), .mode(1'b0));
processing_element pe15_0(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay15), .in_a(a15),   .in_a_chain(in_a_chain_15_0_NC),   .in_b(b14_0to15_0), .in_c(matrixC14_0), .out_a(out_a_15_0_NC), .out_a_chain(a15_0to15_1), .out_b(b15_0to16_0), .out_b0(b15_0to16_0_ping), .out_b1(b15_0to16_0_pong), .out_c(matrixC15_0), .b_data_valid_ping(b_data_valid_ping_delay15_0), .b_data_valid_pong(b_data_valid_pong_delay15_0), .mode(1'b1));
processing_element pe15_1(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay16), .in_a(in_a_15_1_NC),  .in_a_chain(a15_0to15_1), .in_b(b14_1to15_1), .in_c(matrixC14_1), .out_a(out_a_15_1_NC), .out_a_chain(a15_1to15_2), .out_b(b15_1to16_1), .out_b0(b15_1to16_1_ping), .out_b1(b15_1to16_1_pong), .out_c(matrixC15_1), .b_data_valid_ping(b_data_valid_ping_delay15_1), .b_data_valid_pong(b_data_valid_pong_delay15_1), .mode(1'b0));
processing_element pe15_2(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay17), .in_a(in_a_15_2_NC),  .in_a_chain(a15_1to15_2), .in_b(b14_2to15_2), .in_c(matrixC14_2), .out_a(out_a_15_2_NC), .out_a_chain(a15_2to15_3), .out_b(b15_2to16_2), .out_b0(b15_2to16_2_ping), .out_b1(b15_2to16_2_pong), .out_c(matrixC15_2), .b_data_valid_ping(b_data_valid_ping_delay15_2), .b_data_valid_pong(b_data_valid_pong_delay15_2), .mode(1'b0));
processing_element pe15_3(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay18), .in_a(in_a_15_3_NC),  .in_a_chain(a15_2to15_3), .in_b(b14_3to15_3), .in_c(matrixC14_3), .out_a(out_a_15_3_NC), .out_a_chain(a15_3to15_4), .out_b(b15_3to16_3), .out_b0(b15_3to16_3_ping), .out_b1(b15_3to16_3_pong), .out_c(matrixC15_3), .b_data_valid_ping(b_data_valid_ping_delay15_3), .b_data_valid_pong(b_data_valid_pong_delay15_3), .mode(1'b0));
processing_element pe15_4(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay19), .in_a(in_a_15_4_NC),  .in_a_chain(a15_3to15_4), .in_b(b14_4to15_4), .in_c(matrixC14_4), .out_a(out_a_15_4_NC), .out_a_chain(a15_4to15_5), .out_b(b15_4to16_4), .out_b0(b15_4to16_4_ping), .out_b1(b15_4to16_4_pong), .out_c(matrixC15_4), .b_data_valid_ping(b_data_valid_ping_delay15_4), .b_data_valid_pong(b_data_valid_pong_delay15_4), .mode(1'b0));
processing_element pe15_5(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay20), .in_a(in_a_15_5_NC),  .in_a_chain(a15_4to15_5), .in_b(b14_5to15_5), .in_c(matrixC14_5), .out_a(out_a_15_5_NC), .out_a_chain(a15_5to15_6), .out_b(b15_5to16_5), .out_b0(b15_5to16_5_ping), .out_b1(b15_5to16_5_pong), .out_c(matrixC15_5), .b_data_valid_ping(b_data_valid_ping_delay15_5), .b_data_valid_pong(b_data_valid_pong_delay15_5), .mode(1'b0));
processing_element pe15_6(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay21), .in_a(in_a_15_6_NC),  .in_a_chain(a15_5to15_6), .in_b(b14_6to15_6), .in_c(matrixC14_6), .out_a(out_a_15_6_NC), .out_a_chain(a15_6to15_7), .out_b(b15_6to16_6), .out_b0(b15_6to16_6_ping), .out_b1(b15_6to16_6_pong), .out_c(matrixC15_6), .b_data_valid_ping(b_data_valid_ping_delay15_6), .b_data_valid_pong(b_data_valid_pong_delay15_6), .mode(1'b0));
processing_element pe15_7(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay22), .in_a(in_a_15_7_NC),  .in_a_chain(a15_6to15_7), .in_b(b14_7to15_7), .in_c(matrixC14_7), .out_a(out_a_15_7_NC), .out_a_chain(a15_7to15_8), .out_b(b15_7to16_7), .out_b0(b15_7to16_7_ping), .out_b1(b15_7to16_7_pong), .out_c(matrixC15_7), .b_data_valid_ping(b_data_valid_ping_delay15_7), .b_data_valid_pong(b_data_valid_pong_delay15_7), .mode(1'b0));
processing_element pe15_8(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay23), .in_a(in_a_15_8_NC),  .in_a_chain(a15_7to15_8), .in_b(b14_8to15_8), .in_c(matrixC14_8), .out_a(out_a_15_8_NC), .out_a_chain(a15_8to15_9), .out_b(b15_8to16_8), .out_b0(b15_8to16_8_ping), .out_b1(b15_8to16_8_pong), .out_c(matrixC15_8), .b_data_valid_ping(b_data_valid_ping_delay15_8), .b_data_valid_pong(b_data_valid_pong_delay15_8), .mode(1'b0));
processing_element pe15_9(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay24), .in_a(in_a_15_9_NC),  .in_a_chain(a15_8to15_9), .in_b(b14_9to15_9), .in_c(matrixC14_9), .out_a(out_a_15_9_NC), .out_a_chain(a15_9to15_10), .out_b(b15_9to16_9), .out_b0(b15_9to16_9_ping), .out_b1(b15_9to16_9_pong), .out_c(matrixC15_9), .b_data_valid_ping(b_data_valid_ping_delay15_9), .b_data_valid_pong(b_data_valid_pong_delay15_9), .mode(1'b0));
processing_element pe15_10(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay25), .in_a(in_a_15_10_NC),  .in_a_chain(a15_9to15_10), .in_b(b14_10to15_10), .in_c(matrixC14_10), .out_a(out_a_15_10_NC), .out_a_chain(a15_10to15_11), .out_b(b15_10to16_10), .out_b0(b15_10to16_10_ping), .out_b1(b15_10to16_10_pong), .out_c(matrixC15_10), .b_data_valid_ping(b_data_valid_ping_delay15_10), .b_data_valid_pong(b_data_valid_pong_delay15_10), .mode(1'b0));
processing_element pe15_11(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay26), .in_a(in_a_15_11_NC),  .in_a_chain(a15_10to15_11), .in_b(b14_11to15_11), .in_c(matrixC14_11), .out_a(out_a_15_11_NC), .out_a_chain(a15_11to15_12), .out_b(b15_11to16_11), .out_b0(b15_11to16_11_ping), .out_b1(b15_11to16_11_pong), .out_c(matrixC15_11), .b_data_valid_ping(b_data_valid_ping_delay15_11), .b_data_valid_pong(b_data_valid_pong_delay15_11), .mode(1'b0));
processing_element pe15_12(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay27), .in_a(in_a_15_12_NC),  .in_a_chain(a15_11to15_12), .in_b(b14_12to15_12), .in_c(matrixC14_12), .out_a(out_a_15_12_NC), .out_a_chain(a15_12to15_13), .out_b(b15_12to16_12), .out_b0(b15_12to16_12_ping), .out_b1(b15_12to16_12_pong), .out_c(matrixC15_12), .b_data_valid_ping(b_data_valid_ping_delay15_12), .b_data_valid_pong(b_data_valid_pong_delay15_12), .mode(1'b0));
processing_element pe15_13(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay28), .in_a(in_a_15_13_NC),  .in_a_chain(a15_12to15_13), .in_b(b14_13to15_13), .in_c(matrixC14_13), .out_a(out_a_15_13_NC), .out_a_chain(a15_13to15_14), .out_b(b15_13to16_13), .out_b0(b15_13to16_13_ping), .out_b1(b15_13to16_13_pong), .out_c(matrixC15_13), .b_data_valid_ping(b_data_valid_ping_delay15_13), .b_data_valid_pong(b_data_valid_pong_delay15_13), .mode(1'b0));
processing_element pe15_14(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay29), .in_a(in_a_15_14_NC),  .in_a_chain(a15_13to15_14), .in_b(b14_14to15_14), .in_c(matrixC14_14), .out_a(out_a_15_14_NC), .out_a_chain(a15_14to15_15), .out_b(b15_14to16_14), .out_b0(b15_14to16_14_ping), .out_b1(b15_14to16_14_pong), .out_c(matrixC15_14), .b_data_valid_ping(b_data_valid_ping_delay15_14), .b_data_valid_pong(b_data_valid_pong_delay15_14), .mode(1'b0));
processing_element pe15_15(.reset(effective_rst), .clk(clk), .b_data_sel(b_data_sel_delay30), .in_a(in_a_15_15_NC),  .in_a_chain(a15_14to15_15), .in_b(b14_15to15_15), .in_c(matrixC14_15), .out_a(out_a_15_15_NC), .out_a_chain(a15_15to15_16), .out_b(b15_15to16_15), .out_b0(b15_15to16_15_ping), .out_b1(b15_15to16_15_pong), .out_c(matrixC15_15), .b_data_valid_ping(b_data_valid_ping_delay15_15), .b_data_valid_pong(b_data_valid_pong_delay15_15), .mode(1'b0));
  
//assign a_data_out = {a15_15to15_16, a14_15to14_16, a13_15to13_16, a12_15to12_16, a11_15to11_16, a10_15to10_16, a9_15to9_16, a8_15to8_16, a7_15to7_16, a6_15to6_16, a5_15to5_16, a4_15to4_16, a3_15to3_16, a2_15to2_16, a1_15to1_16, a0_15to0_16};
//assign b_data_out = {b15_15to16_15, b15_14to16_14, b15_13to16_13, b15_12to16_12, b15_11to16_11, b15_10to16_10, b15_9to16_9, b15_8to16_8, b15_7to16_7, b15_6to16_6, b15_5to16_5, b15_4to16_4, b15_3to16_3, b15_2to16_2, b15_1to16_1, b15_0to16_0};

endmodule

//////////////////////////////////////////////////////////////////////////
// Processing element (PE)
//////////////////////////////////////////////////////////////////////////

module processing_element(
    reset, 
    clk, 
    b_data_sel,
    in_a,
    in_a_chain,
    in_b,
    in_c,
    out_a,
    out_a_chain,
    out_b, 
    out_b0,
    out_b1,
    out_c,
    b_data_valid_ping,
    b_data_valid_pong,
    mode
 );

input reset;
input clk;
input b_data_sel;
input b_data_valid_ping;
input b_data_valid_pong;
input  [`DWIDTH-1:0] in_a;
input  [`DWIDTH-1:0] in_a_chain;
input  [`DWIDTH-1:0] in_b; 
input  [`DWIDTH-1:0] in_c; 
output [`DWIDTH-1:0] out_a;
output [`DWIDTH-1:0] out_a_chain;
output [`DWIDTH-1:0] out_b;
output [`DWIDTH-1:0] out_b0;
output [`DWIDTH-1:0] out_b1;
output [`DWIDTH-1:0] out_c;  
input mode;

`ifdef complex_dsp

 wire [18:0] scanout;
 wire [63:0] chainout; //unconnected
 wire [63:0] result;
 wire [17:0] ax;
 wire [18:0] ay;
 wire [35:0] bx;
 wire [63:0] chainin; //unconnected
 wire [18:0] scanin;
 wire [11:0] mode_sigs;

 assign mode_sigs = 12'b010101010101;  //Any value of mode_sigs (structural, not functional, correctness)
 assign ax = {{(18-`DWIDTH){1'b0}}, in_a};
 assign ay = {{(19-`DWIDTH){1'b0}}, in_b};
 assign bx = in_c;
 assign scanin = {{(18-`DWIDTH){1'b0}}, in_a_chain};
 //assign chainin = in_c;

  //We will instantiate DSP slices with input chaining and output chaining.
  //Input chaining is only supported in the 18x19 mode or the 27x27 mode.
  //We will use the input chain provided by the DSP for the A input. For B, the chain will be manual.

  mult_add_int_18x19 u_pe(
    .clk(clk),
    .reset(reset),
    .mode_sigs(mode_sigs),
    .ax(ax),
    .ay(ay),
    .bx(bx),
    .chainin(chainin),
    .scanin(scanin),
    .result(result),
    .chainout(chainout),
    .scanout(scanout)
  );

reg [`DWIDTH-1:0] out_b0;
reg [`DWIDTH-1:0] out_b1;

wire [`DWIDTH-1:0] in_mac;
wire [`DWIDTH-1:0] out_c;

assign out_c = result;
assign in_mac = (b_data_sel==0)? out_b0 : out_b1;
        
//assign out_a = result; 
assign out_a_chain = scanout;

always @(posedge clk)begin 
    if (reset) begin
        out_b0<=0;
    end
    if(b_data_valid_ping == 1) begin
        out_b0<=in_b;
    end
end

always @(posedge clk)begin 
    if (reset) begin
        out_b1<=0;
    end
    if(b_data_valid_pong == 1) begin
        out_b1<=in_b;
    end
end

`else

reg [`DWIDTH-1:0] out_a;
reg [`DWIDTH-1:0] out_b;
reg [`DWIDTH-1:0] out_b0;
reg [`DWIDTH-1:0] out_b1;

wire [`DWIDTH-1:0] in_mac;
wire [`DWIDTH-1:0] out_c;
wire [`DWIDTH-1:0] out_mac;

assign out_c = out_mac;
assign in_mac = (b_data_sel==0)? out_b0 : out_b1;
        
seq_mac u_mac(.a(out_a), .b(in_mac), .c(in_c), .out(out_mac), .reset(reset), .clk(clk));

always @(posedge clk)begin
    if(reset) begin
        out_a<=0;
    end
    else begin  
        out_a<=mode ? in_a : in_a_chain;
    end
end

assign out_a_chain = out_a;

always @(posedge clk)begin
    if(reset) begin
        out_b<=0;
    end
    else begin  
        out_b<=in_b;
    end
end

always @(posedge clk)begin 
    if (reset) begin
        out_b0<=0;
    end
    if(b_data_valid_ping == 1) begin
        out_b0<=in_b;
    end
end

always @(posedge clk)begin 
    if (reset) begin
        out_b1<=0;
    end
    if(b_data_valid_pong == 1) begin
        out_b1<=in_b;
    end
end

`endif

endmodule

`ifndef complex_dsp

//////////////////////////////////////////////////////////////////////////
// Multiply-and-accumulate (MAC) block
//////////////////////////////////////////////////////////////////////////

module seq_mac(a, b, c, out, reset, clk);

input [`DWIDTH-1:0] a;
input [`DWIDTH-1:0] b;
input [`DWIDTH-1:0] c;
input reset;
input clk;
output [`DWIDTH-1:0] out;

wire [`DWIDTH-1:0] mul_out;
wire [`DWIDTH-1:0] add_out;

reg [`DWIDTH-1:0] a_flopped;
reg [`DWIDTH-1:0] b_flopped;
reg [`DWIDTH-1:0] c_flopped;

wire [2*`DWIDTH-1:0] mul_out_temp;
wire [2*`DWIDTH-1:0] mul_out_temp_reg;

always @(posedge clk) begin
  if (reset) begin
    a_flopped <= 0;
    b_flopped <= 0;
    c_flopped <= 0;
  end else begin
    a_flopped <= a;
    b_flopped <= b;
    c_flopped <= c;
  end
end
  
// assign mul_out = a * b;
qmult mult_u1(.i_multiplicand(a_flopped), .i_multiplier(b_flopped), .o_result(mul_out_temp));


// down cast the result
// todo: do a fused multiply add. Truncate only once after the accumulation is complete
assign mul_out = 
    (mul_out_temp[2*`DWIDTH-1] == 0) ?  //positive number
        (
           (|(mul_out_temp[2*`DWIDTH-2 : `DWIDTH-1])) ?  //is any bit from 14:7 is 1, that means overlfow
             {mul_out_temp[2*`DWIDTH-1] , {(`DWIDTH-1){1'b1}}} : //sign bit and then all 1s
             {mul_out_temp[2*`DWIDTH-1] , mul_out_temp[`DWIDTH-2:0]} 
        )
        : //negative number
        (
           (|(mul_out_temp[2*`DWIDTH-2 : `DWIDTH-1])) ?  //is any bit from 14:7 is 0, that means overlfow
             {mul_out_temp[2*`DWIDTH-1] , mul_out_temp[`DWIDTH-2:0]} :
             {mul_out_temp[2*`DWIDTH-1] , {(`DWIDTH-1){1'b0}}} //sign bit and then all 0s
        );


// we just truncate the higher bits of the product
// assign out = mul_out + c_flopped;
qadd add_u1(.a(c_flopped), .b(mul_out), .c(out));

endmodule


//////////////////////////////////////////////////////////////////////////
// Multiplier
//////////////////////////////////////////////////////////////////////////

module qmult(i_multiplicand,i_multiplier,o_result);

input [`DWIDTH-1:0] i_multiplicand;
input [`DWIDTH-1:0] i_multiplier;
output [2*`DWIDTH-1:0] o_result;

assign o_result = i_multiplicand * i_multiplier;
//DW02_mult #(`DWIDTH,`DWIDTH) u_mult(.A(i_multiplicand), .B(i_multiplier), .TC(1'b1), .PRODUCT(o_result));

endmodule

`endif

//////////////////////////////////////////////////////////////////////////
// Adder
//////////////////////////////////////////////////////////////////////////
// todo: Output should have one extra bit as compared to the inputs

module qadd(a,b,c);

input [`DWIDTH-1:0] a;
input [`DWIDTH-1:0] b;
output [`DWIDTH-1:0] c;

assign c = a + b;
// DW01_add #(`DWIDTH) u_add(.A(a), .B(b), .CI(1'b0), .SUM(c), .CO());

endmodule

module cfg(
    input                             PCLK,
    input                             PRESETn,
    input        [`REG_ADDRWIDTH-1:0] PADDR,
    input                             PWRITE,
    input                             PSEL,
    input                             PENABLE,
    input        [`REG_DATAWIDTH-1:0] PWDATA,
    output reg   [`REG_DATAWIDTH-1:0] PRDATA,
    output reg                        PREADY,
    output reg start_tpu,
    output reg enable_matmul,
    output reg enable_norm,
    output reg enable_pool,
    output reg enable_activation,
    output reg enable_conv_mode,
    //TODO: We need to change the precision of compute to a larger 
    //number. For now, using the DWIDTH variable, but we need a 
    //HIGH_PRECISION_DWIDTH kind of thing
    output reg [`DWIDTH-1:0] mean,
    output reg [`DWIDTH-1:0] inv_var,
    output reg [`MAX_BITS_POOL-1:0] pool_window_size,
	output reg [`AWIDTH-1:0] address_mat_a,
    output reg [`AWIDTH-1:0] address_mat_b,
    output reg [`AWIDTH-1:0] address_mat_c,
    output reg [31:0] num_matrices_A,
    output reg [31:0] num_matrices_B,
    output reg [`DWIDTH-1:0] matrix_size,
    output reg [`DWIDTH-1:0] filter_size,
    output reg pool_select,
    output reg [`DWIDTH-1:0] k_dimension,
    output reg accum_select,
    output reg [`MASK_WIDTH-1:0] validity_mask_a_rows,
    output reg [`MASK_WIDTH-1:0] validity_mask_a_cols_b_rows,
    output reg [`MASK_WIDTH-1:0] validity_mask_b_cols,
    output reg save_output_to_accum,
    output reg add_accum_to_output,
    output reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_a,
    output reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_b,
    output reg [`ADDR_STRIDE_WIDTH-1:0] address_stride_c,
    output reg activation_type,
    output reg [3:0] conv_filter_height,
    output reg [3:0] conv_filter_width,
    output reg [3:0] conv_stride_horiz,
    output reg [3:0] conv_stride_verti,
    output reg [3:0] conv_padding_left,
    output reg [3:0] conv_padding_right,
    output reg [3:0] conv_padding_top,
    output reg [3:0] conv_padding_bottom,
    output reg [15:0] num_channels_inp,
    output reg [15:0] num_channels_out,
    output reg [15:0] inp_img_height,
    output reg [15:0] inp_img_width,
    output reg [15:0] out_img_height,
    output reg [15:0] out_img_width,
    output reg [31:0] batch_size,
    output reg pe_reset,
    input done_tpu
);

//Dummy register to sync all other invalid/unimplemented addresses
reg [`REG_DATAWIDTH-1:0] reg_dummy;


//////////////////////////////////////////////////////
//Using a simple APB interface. Taken from:
// https://github.com/maomran/APB-Slave
// https://research.ijcaonline.org/volume95/number21/pxc3897047.pdf

reg [1:0] State;
`define IDLE     2'b00
`define W_ENABLE  2'b01
`define R_ENABLE  2'b10

always @(posedge PCLK) begin
  if (PRESETn == 0) begin
    State <= `IDLE;
    PRDATA <= 0;
    PREADY <= 0;
    start_tpu <= 0;
    enable_matmul <= 0;
    enable_norm <= 0;
    enable_pool <= 0;
    enable_activation <= 0;
    mean <= 0;
    inv_var <= 0;
    pool_window_size <= 1;
		reg_dummy <= 0;
    address_mat_a <= 0;
    address_mat_b <= 0;
    address_mat_c <= 0;
    num_matrices_A <= 1;
    num_matrices_B <= 1;
    matrix_size <= 8;
    filter_size <= 2;
    pool_select <= 0;
    k_dimension <= 8;
    accum_select <= 1;
    validity_mask_a_rows <= {`MASK_WIDTH{1'b1}};
    validity_mask_a_cols_b_rows <= {`MASK_WIDTH{1'b1}};
    validity_mask_b_cols <= {`MASK_WIDTH{1'b1}};
    save_output_to_accum <= 0;
    add_accum_to_output <= 0;
    address_stride_a <= 1;
    address_stride_b <= 1;
    address_stride_c <= 1;
    activation_type <= 1;
    conv_filter_height <= 2;
    conv_filter_width  <= 2;
    conv_stride_horiz  <= 1;
    conv_stride_verti  <= 1;
    conv_padding_left  <= 0;
    conv_padding_right <= 0;
    conv_padding_top   <= 0;
    conv_padding_bottom<= 0;
    num_channels_inp <= 4;
    num_channels_out <= 4;
    inp_img_height   <= 8;
    inp_img_width    <= 8;
    out_img_height   <= 7;
    out_img_width    <= 7;
    batch_size       <= 2;
    enable_conv_mode <= 0;
    pe_reset <= 0;
  end

  else begin
    case (State)
      `IDLE : begin
        PRDATA <= 0;
        if (PSEL) begin
          if (PWRITE) begin
            State <= `W_ENABLE;
          end
          else begin
            State <= `R_ENABLE;
          end
        end
        PREADY <= 0;
        pe_reset <= 0; //this register bit auto resets itself
      end

      `W_ENABLE : begin
        if (PSEL && PWRITE && PENABLE) begin
          case (PADDR)
          `REG_ENABLES_ADDR   : begin 
                                enable_conv_mode  <= PWDATA[31];
                                enable_activation <= PWDATA[3];
                                enable_pool       <= PWDATA[2];
                                enable_norm       <= PWDATA[1];
                                enable_matmul     <= PWDATA[0];
                                end
          `REG_STDN_TPU_ADDR  : begin
                                start_tpu <= PWDATA[0];
                                pe_reset <= PWDATA[15]; 
                                end
          `REG_MEAN_ADDR      : mean <= PWDATA[`DWIDTH-1:0];
          `REG_INV_VAR_ADDR   : inv_var <= PWDATA[`DWIDTH-1:0];
          `REG_MATRIX_A_ADDR  : address_mat_a <= PWDATA[`AWIDTH-1:0];
          `REG_MATRIX_B_ADDR  : address_mat_b <= PWDATA[`AWIDTH-1:0];
          `REG_MATRIX_C_ADDR  : address_mat_c <= PWDATA[`AWIDTH-1:0];
          `REG_VALID_MASK_A_ROWS_ADDR: begin
                                validity_mask_a_rows <= PWDATA[`MASK_WIDTH-1:0];
                                end
          `REG_VALID_MASK_A_COLS_B_ROWS_ADDR: begin
                                validity_mask_a_cols_b_rows <= PWDATA[`MASK_WIDTH-1:0];
                                end
          `REG_VALID_MASK_B_COLS_ADDR: begin
                                validity_mask_b_cols <= PWDATA[`MASK_WIDTH-1:0];
                                end
          `REG_POOL_WINDOW_ADDR: pool_window_size <= PWDATA[`MAX_BITS_POOL-1:0];
					`REG_ACCUM_ACTIONS_ADDR: begin
                                   add_accum_to_output <= PWDATA[1];
                                   save_output_to_accum <= PWDATA[0];
                                   end
          `REG_MATRIX_A_STRIDE_ADDR : address_stride_a <= PWDATA[`ADDR_STRIDE_WIDTH-1:0];
          `REG_MATRIX_B_STRIDE_ADDR : address_stride_b <= PWDATA[`ADDR_STRIDE_WIDTH-1:0];
          `REG_MATRIX_C_STRIDE_ADDR : address_stride_c <= PWDATA[`ADDR_STRIDE_WIDTH-1:0];
          `REG_ACTIVATION_CSR_ADDR  : activation_type  <= PWDATA[0];
          `REG_CONV_PARAMS_1_ADDR   : begin
                                      conv_filter_height <= PWDATA[3:0]; 
                                      conv_filter_width  <= PWDATA[7:4];
                                      conv_stride_horiz  <= PWDATA[11:8];
                                      conv_stride_verti  <= PWDATA[15:12];
                                      conv_padding_left  <= PWDATA[19:16];
                                      conv_padding_right <= PWDATA[23:20];
                                      conv_padding_top   <= PWDATA[27:24];
                                      conv_padding_bottom<= PWDATA[31:28];
                                      end
          `REG_CONV_PARAMS_2_ADDR   : begin
                                      num_channels_inp <= PWDATA[15:0];
                                      num_channels_out <= PWDATA[31:16];
                                      end
          `REG_CONV_PARAMS_3_ADDR   : begin
                                      inp_img_height   <= PWDATA[15:0];
                                      inp_img_width    <= PWDATA[31:16];
                                      end
          `REG_CONV_PARAMS_4_ADDR   : begin
                                      out_img_height   <= PWDATA[15:0];
                                      out_img_width    <= PWDATA[31:16];
                                      end
          `REG_BATCH_SIZE_ADDR      : batch_size <= PWDATA[31:0];
          `REG_NUM_MATRICES_A_ADDR	: num_matrices_A <= PWDATA[31:0];
          `REG_NUM_MATRICES_B_ADDR	: num_matrices_B <= PWDATA[31:0];
          `REG_POOLING_ACCUM_ADDR	: begin
          							  pool_select <= PWDATA[0];
          							  filter_size <= PWDATA[8:1];
          							  matrix_size <= PWDATA[16:9];
          							  k_dimension <= PWDATA[24:17];
          							  accum_select <= PWDATA[25];
          							  end
          default: reg_dummy <= PWDATA; //sink writes to a dummy register
          endcase
          PREADY <=1;          
        end
        State <= `IDLE;
      end

      `R_ENABLE : begin
        if (PSEL && !PWRITE && PENABLE) begin
          PREADY <= 1;
          case (PADDR)
          `REG_ENABLES_ADDR   : PRDATA <= {28'b0, enable_activation, enable_pool, enable_norm, enable_matmul};
          `REG_STDN_TPU_ADDR  : PRDATA <= {done_tpu, 30'b0, start_tpu};
          `REG_MEAN_ADDR      : PRDATA <= mean;
          `REG_INV_VAR_ADDR   : PRDATA <= inv_var;
          `REG_MATRIX_A_ADDR  : PRDATA <= address_mat_a;
          `REG_MATRIX_B_ADDR  : PRDATA <= address_mat_b;
          `REG_MATRIX_C_ADDR  : PRDATA <= address_mat_c;
          `REG_VALID_MASK_A_ROWS_ADDR: PRDATA <= validity_mask_a_rows;
          `REG_VALID_MASK_A_COLS_B_ROWS_ADDR: PRDATA <= validity_mask_a_cols_b_rows;
          `REG_VALID_MASK_B_COLS_ADDR: PRDATA <= validity_mask_b_cols;
          `REG_POOL_WINDOW_ADDR : PRDATA <= pool_window_size;
					`REG_ACCUM_ACTIONS_ADDR: PRDATA <= {30'b0, add_accum_to_output, save_output_to_accum};
          `REG_MATRIX_A_STRIDE_ADDR : PRDATA <= address_stride_a;
          `REG_MATRIX_B_STRIDE_ADDR : PRDATA <= address_stride_b;
          `REG_MATRIX_C_STRIDE_ADDR : PRDATA <= address_stride_c;
          `REG_ACTIVATION_CSR_ADDR  : PRDATA <= {31'b0, activation_type};
          `REG_CONV_PARAMS_1_ADDR   : PRDATA <= {
                                      conv_filter_height,
                                      conv_filter_width,  
                                      conv_stride_horiz, 
                                      conv_stride_verti,  
                                      conv_padding_left,  
                                      conv_padding_right, 
                                      conv_padding_top,   
                                      conv_padding_bottom
                                      };
          `REG_CONV_PARAMS_2_ADDR   : PRDATA <= {
                                      num_channels_inp,
                                      num_channels_out
                                      };
          `REG_CONV_PARAMS_3_ADDR   : PRDATA <= {
                                      inp_img_height,
                                      inp_img_width 
                                      };
          `REG_CONV_PARAMS_4_ADDR   : PRDATA <= {
                                      out_img_height,
                                      out_img_width
                                      };
          `REG_BATCH_SIZE_ADDR      : PRDATA <= batch_size;
          `REG_NUM_MATRICES_A_ADDR	: PRDATA <= num_matrices_A;
          `REG_NUM_MATRICES_B_ADDR	: PRDATA <= num_matrices_B;
          `REG_POOLING_ACCUM_ADDR	: PRDATA <= {6'b0, accum_select, k_dimension, matrix_size, filter_size, pool_select};
          default             : PRDATA <= reg_dummy; //read the dummy register for undefined addresses
          endcase
        end
        State <= `IDLE;
      end
      default: begin
        State <= `IDLE;
      end
    endcase
  end
end 

endmodule

////////////////////////////////////////////////////////////////////////////////
// THIS FILE WAS AUTOMATICALLY GENERATED FROM generate_norm.v.mako
// DO NOT EDIT
////////////////////////////////////////////////////////////////////////////////
module norm(
    input enable_norm,
    input [`DWIDTH-1:0] mean,
    input [`DWIDTH-1:0] inv_var,
    input in_data_available,
    input [`DWIDTH-1:0] inp_data0,
    input [`DWIDTH-1:0] inp_data1,
    input [`DWIDTH-1:0] inp_data2,
    input [`DWIDTH-1:0] inp_data3,
    input [`DWIDTH-1:0] inp_data4,
    input [`DWIDTH-1:0] inp_data5,
    input [`DWIDTH-1:0] inp_data6,
    input [`DWIDTH-1:0] inp_data7,
    input [`DWIDTH-1:0] inp_data8,
    input [`DWIDTH-1:0] inp_data9,
    input [`DWIDTH-1:0] inp_data10,
    input [`DWIDTH-1:0] inp_data11,
    input [`DWIDTH-1:0] inp_data12,
    input [`DWIDTH-1:0] inp_data13,
    input [`DWIDTH-1:0] inp_data14,
    input [`DWIDTH-1:0] inp_data15,
    output [`DWIDTH-1:0] out_data0,
    output [`DWIDTH-1:0] out_data1,
    output [`DWIDTH-1:0] out_data2,
    output [`DWIDTH-1:0] out_data3,
    output [`DWIDTH-1:0] out_data4,
    output [`DWIDTH-1:0] out_data5,
    output [`DWIDTH-1:0] out_data6,
    output [`DWIDTH-1:0] out_data7,
    output [`DWIDTH-1:0] out_data8,
    output [`DWIDTH-1:0] out_data9,
    output [`DWIDTH-1:0] out_data10,
    output [`DWIDTH-1:0] out_data11,
    output [`DWIDTH-1:0] out_data12,
    output [`DWIDTH-1:0] out_data13,
    output [`DWIDTH-1:0] out_data14,
    output [`DWIDTH-1:0] out_data15,
    output out_data_available,
    input [`MASK_WIDTH-1:0] validity_mask,
    output reg done_norm,
    input clk,
    input reset
);

reg in_data_available1;
reg in_data_available2;
reg in_data_available3;
reg in_data_available4;
reg in_data_available5;
reg in_data_available6;
reg in_data_available7;
reg in_data_available8;
reg in_data_available9;
reg in_data_available10;
reg in_data_available11;
reg in_data_available12;
reg in_data_available13;
reg in_data_available14;
reg in_data_available15;

always @(posedge clk) begin
    in_data_available1 <= in_data_available;
	in_data_available2 <= in_data_available1;
	in_data_available3 <= in_data_available2;
	in_data_available4 <= in_data_available3;
	in_data_available5 <= in_data_available4;
	in_data_available6 <= in_data_available5;
	in_data_available7 <= in_data_available6;
	in_data_available8 <= in_data_available7;
	in_data_available9 <= in_data_available8;
	in_data_available10 <= in_data_available9;
	in_data_available11 <= in_data_available10;
	in_data_available12 <= in_data_available11;
	in_data_available13 <= in_data_available12;
	in_data_available14 <= in_data_available13;
	in_data_available15 <= in_data_available14;
end

wire out_data_available_internal;
wire out_data_available_final;

reg [`DWIDTH-1:0] done_count;

assign out_data_available = (enable_norm) ? out_data_available_internal : in_data_available;

always @(posedge clk) begin
	if (reset) begin
		done_norm <= 0;
		done_count <= 0;
	end
	if (done_count == 4) begin
		done_norm <= 1;
	end
	if (out_data_available_final == 1) begin
		done_count <= done_count + 1;
	end
end

norm_sub norm0(
	.enable_norm(enable_norm),
    .mean(mean),
    .inv_var(inv_var),
    .in_data_available(in_data_available),
    .inp_data(inp_data0),
    .out_data(out_data0),
    .out_data_available(out_data_available_internal),
    .validity_mask(validity_mask[0]),
    .clk(clk),
    .reset(reset)
);

wire out_data_available_NC1;
norm_sub norm1(
	.enable_norm(enable_norm),
    .mean(mean),
    .inv_var(inv_var),
    .in_data_available(in_data_available1),
    .inp_data(inp_data1),
    .out_data(out_data1),
    .out_data_available(out_data_available_NC1),
    .validity_mask(validity_mask[1]),
    .clk(clk),
    .reset(reset)
);

wire out_data_available_NC2;
norm_sub norm2(
	.enable_norm(enable_norm),
    .mean(mean),
    .inv_var(inv_var),
    .in_data_available(in_data_available2),
    .inp_data(inp_data2),
    .out_data(out_data2),
    .out_data_available(out_data_available_NC2),
    .validity_mask(validity_mask[2]),
    .clk(clk),
    .reset(reset)
);

wire out_data_available_NC3;
norm_sub norm3(
	.enable_norm(enable_norm),
    .mean(mean),
    .inv_var(inv_var),
    .in_data_available(in_data_available3),
    .inp_data(inp_data3),
    .out_data(out_data3),
    .out_data_available(out_data_available_NC3),
    .validity_mask(validity_mask[3]),
    .clk(clk),
    .reset(reset)
);

wire out_data_available_NC4;
norm_sub norm4(
	.enable_norm(enable_norm),
    .mean(mean),
    .inv_var(inv_var),
    .in_data_available(in_data_available4),
    .inp_data(inp_data4),
    .out_data(out_data4),
    .out_data_available(out_data_available_NC4),
    .validity_mask(validity_mask[4]),
    .clk(clk),
    .reset(reset)
);

wire out_data_available_NC5;
norm_sub norm5(
	.enable_norm(enable_norm),
    .mean(mean),
    .inv_var(inv_var),
    .in_data_available(in_data_available5),
    .inp_data(inp_data5),
    .out_data(out_data5),
    .out_data_available(out_data_available_NC5),
    .validity_mask(validity_mask[5]),
    .clk(clk),
    .reset(reset)
);

wire out_data_available_NC6;
norm_sub norm6(
	.enable_norm(enable_norm),
    .mean(mean),
    .inv_var(inv_var),
    .in_data_available(in_data_available6),
    .inp_data(inp_data6),
    .out_data(out_data6),
    .out_data_available(out_data_available_NC6),
    .validity_mask(validity_mask[6]),
    .clk(clk),
    .reset(reset)
);

wire out_data_available_NC7;
norm_sub norm7(
	.enable_norm(enable_norm),
    .mean(mean),
    .inv_var(inv_var),
    .in_data_available(in_data_available7),
    .inp_data(inp_data7),
    .out_data(out_data7),
    .out_data_available(out_data_available_NC7),
    .validity_mask(validity_mask[7]),
    .clk(clk),
    .reset(reset)
);

wire out_data_available_NC8;
norm_sub norm8(
	.enable_norm(enable_norm),
    .mean(mean),
    .inv_var(inv_var),
    .in_data_available(in_data_available8),
    .inp_data(inp_data8),
    .out_data(out_data8),
    .out_data_available(out_data_available_NC8),
    .validity_mask(validity_mask[8]),
    .clk(clk),
    .reset(reset)
);

wire out_data_available_NC9;
norm_sub norm9(
	.enable_norm(enable_norm),
    .mean(mean),
    .inv_var(inv_var),
    .in_data_available(in_data_available9),
    .inp_data(inp_data9),
    .out_data(out_data9),
    .out_data_available(out_data_available_NC9),
    .validity_mask(validity_mask[9]),
    .clk(clk),
    .reset(reset)
);

wire out_data_available_NC10;
norm_sub norm10(
	.enable_norm(enable_norm),
    .mean(mean),
    .inv_var(inv_var),
    .in_data_available(in_data_available10),
    .inp_data(inp_data10),
    .out_data(out_data10),
    .out_data_available(out_data_available_NC10),
    .validity_mask(validity_mask[10]),
    .clk(clk),
    .reset(reset)
);

wire out_data_available_NC11;
norm_sub norm11(
	.enable_norm(enable_norm),
    .mean(mean),
    .inv_var(inv_var),
    .in_data_available(in_data_available11),
    .inp_data(inp_data11),
    .out_data(out_data11),
    .out_data_available(out_data_available_NC11),
    .validity_mask(validity_mask[11]),
    .clk(clk),
    .reset(reset)
);

wire out_data_available_NC12;
norm_sub norm12(
	.enable_norm(enable_norm),
    .mean(mean),
    .inv_var(inv_var),
    .in_data_available(in_data_available12),
    .inp_data(inp_data12),
    .out_data(out_data12),
    .out_data_available(out_data_available_NC12),
    .validity_mask(validity_mask[12]),
    .clk(clk),
    .reset(reset)
);

wire out_data_available_NC13;
norm_sub norm13(
	.enable_norm(enable_norm),
    .mean(mean),
    .inv_var(inv_var),
    .in_data_available(in_data_available13),
    .inp_data(inp_data13),
    .out_data(out_data13),
    .out_data_available(out_data_available_NC13),
    .validity_mask(validity_mask[13]),
    .clk(clk),
    .reset(reset)
);

wire out_data_available_NC14;
norm_sub norm14(
	.enable_norm(enable_norm),
    .mean(mean),
    .inv_var(inv_var),
    .in_data_available(in_data_available14),
    .inp_data(inp_data14),
    .out_data(out_data14),
    .out_data_available(out_data_available_NC14),
    .validity_mask(validity_mask[14]),
    .clk(clk),
    .reset(reset)
);

norm_sub norm15(
	.enable_norm(enable_norm),
    .mean(mean),
    .inv_var(inv_var),
    .in_data_available(in_data_available15),
    .inp_data(inp_data15),
    .out_data(out_data15),
    .out_data_available(out_data_available_final),
    .validity_mask(validity_mask[15]),
    .clk(clk),
    .reset(reset)
);

endmodule

module norm_sub(
	input enable_norm,
    input [`DWIDTH-1:0] mean,
    input [`DWIDTH-1:0] inv_var,
    input in_data_available,
    input [`DWIDTH-1:0] inp_data,
    output [`DWIDTH-1:0] out_data,
    output out_data_available,
    input  validity_mask,
    input clk,
    input reset
);

reg out_data_available_internal;
wire [`DWIDTH-1:0] out_data_internal;
reg [`DWIDTH-1:0] mean_applied_data;
reg [`DWIDTH-1:0] variance_applied_data;
reg norm_in_progress;

//Muxing logic to handle the case when this block is disabled
assign out_data_available = (enable_norm) ? out_data_available_internal : in_data_available;
assign out_data = (enable_norm) ? out_data_internal : inp_data;

always @(posedge clk) begin
    if ((reset || ~enable_norm)) begin
        mean_applied_data <= 0;
        variance_applied_data <= 0;
    end else if (in_data_available||norm_in_progress) begin
        //Let's apply mean and variance as the input data comes in.
        //We have a pipeline here. First stage does the add (to apply the mean)
        //and second stage does the multiplication (to apply the variance).
        //Note: the following loop is not a loop across multiple columns of data.
        //This loop will run in 2 cycle on the same column of data that comes into 
        //this module in 1 clock.
        if (validity_mask == 1'b1) begin
            mean_applied_data <= (inp_data - mean);
            variance_applied_data <= (mean_applied_data * inv_var);
        end 
        else begin
            mean_applied_data <= (inp_data);
            variance_applied_data <= (mean_applied_data);
        end
    end
    else begin
        mean_applied_data <= 0;
        variance_applied_data <= 0;
    end
end

//The data is normalized in two cycles so we are shifting in_data_available by 2 to generate out_data_available
always @(posedge clk) begin
	norm_in_progress <= in_data_available;
	out_data_available_internal <= norm_in_progress;
end

assign out_data_internal = variance_applied_data;

endmodule

//Simple sual port RAM is not understood by VTR
`ifdef QUARTUS
//////////////////////////////////
// Simple dual port RAM
//////////////////////////////////
module simple_ram (
        addr0, 
        d0, 
        we0, 
        addr1,
        q1,
        clk);
  
parameter AW = 11;
parameter MW = 8;
parameter DW = 8;

input [AW-1:0] addr0;
input [AW-1:0] addr1;
input [MW*DW-1:0] d0;
input [MW-1:0] we0;
output reg [MW*DW-1:0] q1;
input clk;

wire we0_coalesced;
assign we0_coalesced = |we0;

reg [MW*DW-1:0] ram[((1 << AW)-1):0];
  
always @(posedge clk) begin 
    if (we0_coalesced) ram[addr0] <= d0; 
end

always @(posedge clk) begin 
    q1 <= ram[addr1];  
end

endmodule

`endif

//////////////////////////////////
// Dual port RAM
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
  
parameter AW = 11;
parameter MW = 8;
parameter DW = 8;

input [AW-1:0] addr0;
input [AW-1:0] addr1;
input [MW*DW-1:0] d0;
input [MW*DW-1:0] d1;
input [MW-1:0] we0;
input [MW-1:0] we1;
output reg [MW*DW-1:0] q0;
output reg [MW*DW-1:0] q1;
input clk;

wire we0_coalesced;
assign we0_coalesced = |we0;
wire we1_coalesced;
assign we1_coalesced = |we1;

`ifndef hard_mem
reg [MW*DW-1:0] ram[((1 << AW)-1):0];
  
always @(posedge clk) begin 
    if (we0_coalesced) ram[addr0] <= d0; 
    q0 <= ram[addr0];    
end

always @(posedge clk) begin 
    if (we1_coalesced) ram[addr1] <= d1; 
    q1 <= ram[addr1];  
end
  
`else

defparam u_dual_port_ram.ADDR_WIDTH = AW;
defparam u_dual_port_ram.DATA_WIDTH = MW*DW;

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

//Top level state machine
module control(
    input clk,
    input reset,
    input start_tpu,
    input enable_matmul,
    input enable_norm,
    input enable_activation,
    input enable_pool,
    output reg start_mat_mul,
    input done_mat_mul,
    input done_norm,
    input done_pool,
    input done_activation,
    input save_output_to_accum,
    output reg done_tpu
);

reg [3:0] state;

`define STATE_INIT         4'b0000
`define STATE_MATMUL       4'b0001
`define STATE_NORM         4'b0010
`define STATE_POOL         4'b0011
`define STATE_ACTIVATION   4'b0100
`define STATE_DONE         4'b0101

//////////////////////////////////////////////////////
// Assumption: We will always run matmul first. That is, matmul is not optional. 
//             The other blocks - norm, act, pool - are optional.
// Assumption: Order is fixed: Matmul -> Norm -> Pool -> Activation
//////////////////////////////////////////////////////

always @( posedge clk) begin
    if (reset) begin
      state <= `STATE_INIT;
      start_mat_mul <= 1'b0;
      done_tpu <= 1'b0;
    end else begin
      case (state)
      `STATE_INIT: begin
        if ((start_tpu == 1'b1) && (done_tpu == 1'b0)) begin
          if (enable_matmul == 1'b1) begin
            start_mat_mul <= 1'b1;
            state <= `STATE_MATMUL;
          end  
        end  
      end
      
      //start_mat_mul is kinda used as a reset in some logic
      //inside the matmul unit. So, we can't make it 0 right away after
      //asserting it.
      `STATE_MATMUL: begin
        if (done_mat_mul == 1'b1) begin
            start_mat_mul <= 1'b0;
            if(save_output_to_accum) begin
              state <= `STATE_DONE;
            end
            else if (enable_norm) begin
              state <= `STATE_NORM;
            end 
            else if (enable_pool) begin
              state <= `STATE_POOL;
            end
            else if (enable_activation) begin
              state <= `STATE_ACTIVATION;
            end
            else begin
              state <= `STATE_DONE;
            end  
        end 
        else begin
          start_mat_mul <= 1'b1;	      
        end
      end      
      
      `STATE_NORM: begin                 
        if (done_norm == 1'b1) begin
          if (enable_pool) begin
            state <= `STATE_POOL;
          end
          else if (enable_activation) begin
            state <= `STATE_ACTIVATION;
          end
          else begin
            state <= `STATE_DONE;
          end
        end
      end

      `STATE_POOL: begin                 
        if (done_pool == 1'b1) begin
          if (enable_activation) begin
            state <= `STATE_ACTIVATION;
          end
          else begin
            state <= `STATE_DONE;
          end
        end
      end

      `STATE_ACTIVATION: begin                 
        if (done_activation == 1'b1) begin
          state <= `STATE_DONE;
        end
      end

      `STATE_DONE: begin
        //We need to write start_tpu to 0 in the CFG block to get out of this state
        if (start_tpu == 1'b0) begin
          state <= `STATE_INIT;
          done_tpu <= 0;
        end
        else begin
          done_tpu <= 1;
        end
      end
      endcase  
    end 
end
endmodule

////////////////////////////////////////////////////////////////////////////////
// THIS FILE WAS AUTOMATICALLY GENERATED FROM generate_accum.v.mako
// DO NOT EDIT
////////////////////////////////////////////////////////////////////////////////
module accumulator (
    clk,
    resetn,
    start_waddr_accum0,
    wdata_accum0,
    wdata_accum1,
    wdata_accum2,
    wdata_accum3,
    wdata_accum4,
    wdata_accum5,
    wdata_accum6,
    wdata_accum7,
    wdata_accum8,
    wdata_accum9,
    wdata_accum10,
    wdata_accum11,
    wdata_accum12,
    wdata_accum13,
    wdata_accum14,
    wdata_accum15,
    raddr_accum0_pool,
    raddr_accum1_pool,
    raddr_accum2_pool,
    raddr_accum3_pool,
    raddr_accum4_pool,
    raddr_accum5_pool,
    raddr_accum6_pool,
    raddr_accum7_pool,
    raddr_accum8_pool,
    raddr_accum9_pool,
    raddr_accum10_pool,
    raddr_accum11_pool,
    raddr_accum12_pool,
    raddr_accum13_pool,
    raddr_accum14_pool,
    raddr_accum15_pool,
    rdata_accum0_pool,
    rdata_accum1_pool,
    rdata_accum2_pool,
    rdata_accum3_pool,
    rdata_accum4_pool,
    rdata_accum5_pool,
    rdata_accum6_pool,
    rdata_accum7_pool,
    rdata_accum8_pool,
    rdata_accum9_pool,
    rdata_accum10_pool,
    rdata_accum11_pool,
    rdata_accum12_pool,
    rdata_accum13_pool,
    rdata_accum14_pool,
    rdata_accum15_pool,
    wdata_available,
    k_dimension,
    buffer_select,
    start_pooling,
    done_pooling
);

input clk;
input resetn;
input [`AWIDTH-1:0] start_waddr_accum0;
input [`DWIDTH-1:0] wdata_accum0;
input [`DWIDTH-1:0] wdata_accum1;
input [`DWIDTH-1:0] wdata_accum2;
input [`DWIDTH-1:0] wdata_accum3;
input [`DWIDTH-1:0] wdata_accum4;
input [`DWIDTH-1:0] wdata_accum5;
input [`DWIDTH-1:0] wdata_accum6;
input [`DWIDTH-1:0] wdata_accum7;
input [`DWIDTH-1:0] wdata_accum8;
input [`DWIDTH-1:0] wdata_accum9;
input [`DWIDTH-1:0] wdata_accum10;
input [`DWIDTH-1:0] wdata_accum11;
input [`DWIDTH-1:0] wdata_accum12;
input [`DWIDTH-1:0] wdata_accum13;
input [`DWIDTH-1:0] wdata_accum14;
input [`DWIDTH-1:0] wdata_accum15;
input [`AWIDTH-1:0] raddr_accum0_pool;
input [`AWIDTH-1:0] raddr_accum1_pool;
input [`AWIDTH-1:0] raddr_accum2_pool;
input [`AWIDTH-1:0] raddr_accum3_pool;
input [`AWIDTH-1:0] raddr_accum4_pool;
input [`AWIDTH-1:0] raddr_accum5_pool;
input [`AWIDTH-1:0] raddr_accum6_pool;
input [`AWIDTH-1:0] raddr_accum7_pool;
input [`AWIDTH-1:0] raddr_accum8_pool;
input [`AWIDTH-1:0] raddr_accum9_pool;
input [`AWIDTH-1:0] raddr_accum10_pool;
input [`AWIDTH-1:0] raddr_accum11_pool;
input [`AWIDTH-1:0] raddr_accum12_pool;
input [`AWIDTH-1:0] raddr_accum13_pool;
input [`AWIDTH-1:0] raddr_accum14_pool;
input [`AWIDTH-1:0] raddr_accum15_pool;
output [`DWIDTH-1:0] rdata_accum0_pool;
output [`DWIDTH-1:0] rdata_accum1_pool;
output [`DWIDTH-1:0] rdata_accum2_pool;
output [`DWIDTH-1:0] rdata_accum3_pool;
output [`DWIDTH-1:0] rdata_accum4_pool;
output [`DWIDTH-1:0] rdata_accum5_pool;
output [`DWIDTH-1:0] rdata_accum6_pool;
output [`DWIDTH-1:0] rdata_accum7_pool;
output [`DWIDTH-1:0] rdata_accum8_pool;
output [`DWIDTH-1:0] rdata_accum9_pool;
output [`DWIDTH-1:0] rdata_accum10_pool;
output [`DWIDTH-1:0] rdata_accum11_pool;
output [`DWIDTH-1:0] rdata_accum12_pool;
output [`DWIDTH-1:0] rdata_accum13_pool;
output [`DWIDTH-1:0] rdata_accum14_pool;
output [`DWIDTH-1:0] rdata_accum15_pool;
input wdata_available;
input [7:0] k_dimension; // Number of columns in Matrix A | Number of rows in Matrix B (Assumption: Maximum = 256, can be changed accordingly)
input buffer_select;
output start_pooling;
output done_pooling;
  

parameter MWIDTH = 1;

reg wdata_available1;
reg wdata_available2;
reg wdata_available3;
reg wdata_available4;
reg wdata_available5;
reg wdata_available6;
reg wdata_available7;
reg wdata_available8;
reg wdata_available9;
reg wdata_available10;
reg wdata_available11;
reg wdata_available12;
reg wdata_available13;
reg wdata_available14;
reg wdata_available15;

always @ (posedge clk) begin
    wdata_available1 <= wdata_available;
    wdata_available2 <= wdata_available1;
    wdata_available3 <= wdata_available2;
    wdata_available4 <= wdata_available3;
    wdata_available5 <= wdata_available4;
    wdata_available6 <= wdata_available5;
    wdata_available7 <= wdata_available6;
    wdata_available8 <= wdata_available7;
    wdata_available9 <= wdata_available8;
    wdata_available10 <= wdata_available9;
    wdata_available11 <= wdata_available10;
    wdata_available12 <= wdata_available11;
    wdata_available13 <= wdata_available12;
    wdata_available14 <= wdata_available13;
    wdata_available15 <= wdata_available14;
end

wire wdata_en_ping0;
wire wdata_en_ping1;
wire wdata_en_ping2;
wire wdata_en_ping3;
wire wdata_en_ping4;
wire wdata_en_ping5;
wire wdata_en_ping6;
wire wdata_en_ping7;
wire wdata_en_ping8;
wire wdata_en_ping9;
wire wdata_en_ping10;
wire wdata_en_ping11;
wire wdata_en_ping12;
wire wdata_en_ping13;
wire wdata_en_ping14;
wire wdata_en_ping15;
wire wdata_en_pong0;
wire wdata_en_pong1;
wire wdata_en_pong2;
wire wdata_en_pong3;
wire wdata_en_pong4;
wire wdata_en_pong5;
wire wdata_en_pong6;
wire wdata_en_pong7;
wire wdata_en_pong8;
wire wdata_en_pong9;
wire wdata_en_pong10;
wire wdata_en_pong11;
wire wdata_en_pong12;
wire wdata_en_pong13;
wire wdata_en_pong14;
wire wdata_en_pong15;

assign wdata_en_ping0 = wdata_available & buffer_select;
assign wdata_en_ping1 = wdata_available1 & buffer_select;
assign wdata_en_ping2 = wdata_available2 & buffer_select;
assign wdata_en_ping3 = wdata_available3 & buffer_select;
assign wdata_en_ping4 = wdata_available4 & buffer_select;
assign wdata_en_ping5 = wdata_available5 & buffer_select;
assign wdata_en_ping6 = wdata_available6 & buffer_select;
assign wdata_en_ping7 = wdata_available7 & buffer_select;
assign wdata_en_ping8 = wdata_available8 & buffer_select;
assign wdata_en_ping9 = wdata_available9 & buffer_select;
assign wdata_en_ping10 = wdata_available10 & buffer_select;
assign wdata_en_ping11 = wdata_available11 & buffer_select;
assign wdata_en_ping12 = wdata_available12 & buffer_select;
assign wdata_en_ping13 = wdata_available13 & buffer_select;
assign wdata_en_ping14 = wdata_available14 & buffer_select;
assign wdata_en_ping15 = wdata_available15 & buffer_select;

assign wdata_en_pong0 = wdata_available & ~buffer_select;
assign wdata_en_pong1 = wdata_available1 & ~buffer_select;
assign wdata_en_pong2 = wdata_available2 & ~buffer_select;
assign wdata_en_pong3 = wdata_available3 & ~buffer_select;
assign wdata_en_pong4 = wdata_available4 & ~buffer_select;
assign wdata_en_pong5 = wdata_available5 & ~buffer_select;
assign wdata_en_pong6 = wdata_available6 & ~buffer_select;
assign wdata_en_pong7 = wdata_available7 & ~buffer_select;
assign wdata_en_pong8 = wdata_available8 & ~buffer_select;
assign wdata_en_pong9 = wdata_available9 & ~buffer_select;
assign wdata_en_pong10 = wdata_available10 & ~buffer_select;
assign wdata_en_pong11 = wdata_available11 & ~buffer_select;
assign wdata_en_pong12 = wdata_available12 & ~buffer_select;
assign wdata_en_pong13 = wdata_available13 & ~buffer_select;
assign wdata_en_pong14 = wdata_available14 & ~buffer_select;
assign wdata_en_pong15 = wdata_available15 & ~buffer_select;

reg [7:0] addr_counter;
reg [`AWIDTH-1:0] waddr_accum0;
reg [`AWIDTH-1:0] waddr_accum1;
reg [`AWIDTH-1:0] waddr_accum2;
reg [`AWIDTH-1:0] waddr_accum3;
reg [`AWIDTH-1:0] waddr_accum4;
reg [`AWIDTH-1:0] waddr_accum5;
reg [`AWIDTH-1:0] waddr_accum6;
reg [`AWIDTH-1:0] waddr_accum7;
reg [`AWIDTH-1:0] waddr_accum8;
reg [`AWIDTH-1:0] waddr_accum9;
reg [`AWIDTH-1:0] waddr_accum10;
reg [`AWIDTH-1:0] waddr_accum11;
reg [`AWIDTH-1:0] waddr_accum12;
reg [`AWIDTH-1:0] waddr_accum13;
reg [`AWIDTH-1:0] waddr_accum14;
reg [`AWIDTH-1:0] waddr_accum15;
reg add_accum_mux0;
reg add_accum_mux1;
reg add_accum_mux2;
reg add_accum_mux3;
reg add_accum_mux4;
reg add_accum_mux5;
reg add_accum_mux6;
reg add_accum_mux7;
reg add_accum_mux8;
reg add_accum_mux9;
reg add_accum_mux10;
reg add_accum_mux11;
reg add_accum_mux12;
reg add_accum_mux13;
reg add_accum_mux14;
reg add_accum_mux15;

always @ (posedge clk) begin
    if (~wdata_available | (addr_counter == (k_dimension-1))) begin
        add_accum_mux0 <= 0;
        addr_counter <= 0;
    end
    else if (addr_counter == (`MAT_MUL_SIZE-1) & k_dimension != `MAT_MUL_SIZE) begin
        add_accum_mux0 <= 1;
        addr_counter <= addr_counter + 1;
    end
    else if (wdata_available)
        addr_counter <= addr_counter + 1;
end

reg start_pooling;
reg done_pooling;
reg [7:0] start_pooling_count;
always @ (posedge clk) begin
    if (~resetn)
        start_pooling <= 0;
    //TODO: Note the hardcodign of value below.
    //This value (8'd14) is supposed to be 2*MATMUL_SIZE-2.
    //For 8x8 matmul, this is 8'd14
    //For 16x16 matmul, this should be 8'd30
    //For 32x32 matmul, this should be 8'd62
    else if (start_pooling_count > 8'd30) begin
    	start_pooling <= 0;
    	done_pooling <= 1;
    end
    else if (waddr_accum2 != 0 & wdata_available2 == 0)
        start_pooling <= 1;
end
  
always @ (posedge clk) begin
    if (~resetn)
        start_pooling_count <= 0;
    else if (start_pooling)
        start_pooling_count <= start_pooling_count + 1;
end

reg buffer_select_accum;
wire buffer_select_pool;
reg start_pooling_d1;

always @ (posedge clk) begin
	if (buffer_select_pool)
		buffer_select_accum <= 0;
	else
		buffer_select_accum <= 1;
end

always @ (posedge clk) begin
	start_pooling_d1 <= start_pooling;
end

assign buffer_select_pool = start_pooling | start_pooling_d1;

always @ (posedge clk) begin
    add_accum_mux1 <= add_accum_mux0;
    add_accum_mux2 <= add_accum_mux1;
    add_accum_mux3 <= add_accum_mux2;
    add_accum_mux4 <= add_accum_mux3;
    add_accum_mux5 <= add_accum_mux4;
    add_accum_mux6 <= add_accum_mux5;
    add_accum_mux7 <= add_accum_mux6;
    add_accum_mux8 <= add_accum_mux7;
    add_accum_mux9 <= add_accum_mux8;
    add_accum_mux10 <= add_accum_mux9;
    add_accum_mux11 <= add_accum_mux10;
    add_accum_mux12 <= add_accum_mux11;
    add_accum_mux13 <= add_accum_mux12;
    add_accum_mux14 <= add_accum_mux13;
    add_accum_mux15 <= add_accum_mux14;
end
  
reg [7:0] waddr_kdim;
  
always @ (posedge clk) begin
    if (~resetn) 
        waddr_accum0 <= start_waddr_accum0;
    else if (((addr_counter & (`MAT_MUL_SIZE-1)) == (`MAT_MUL_SIZE-1)) & (waddr_kdim > 1)) begin
        waddr_accum0 <= waddr_accum0 - (`MAT_MUL_SIZE -1);
    end
    else if (wdata_available) 
        waddr_accum0 <= waddr_accum0 + 1;
end
  
always @ (posedge clk) begin
    if (~resetn | (((addr_counter & (`MAT_MUL_SIZE-1)) == (`MAT_MUL_SIZE-1)) & (waddr_kdim == 1))) begin
        waddr_kdim <= k_dimension >> `LOG2_MAT_MUL_SIZE;
    end
    else if (((addr_counter & (`MAT_MUL_SIZE-1)) == (`MAT_MUL_SIZE-1)) & (waddr_kdim > 1)) begin
        waddr_kdim <= waddr_kdim - 1;
    end
end
  
always @ (posedge clk) begin
    waddr_accum1 <= waddr_accum0;
    waddr_accum2 <= waddr_accum1;
    waddr_accum3 <= waddr_accum2;
    waddr_accum4 <= waddr_accum3;
    waddr_accum5 <= waddr_accum4;
    waddr_accum6 <= waddr_accum5;
    waddr_accum7 <= waddr_accum6;
    waddr_accum8 <= waddr_accum7;
    waddr_accum9 <= waddr_accum8;
    waddr_accum10 <= waddr_accum9;
    waddr_accum11 <= waddr_accum10;
    waddr_accum12 <= waddr_accum11;
    waddr_accum13 <= waddr_accum12;
    waddr_accum14 <= waddr_accum13;
    waddr_accum15 <= waddr_accum14;
end
   
// Data going into the Accumulator Adders
wire [`DWIDTH-1:0] wdata_accum0_in;
wire [`DWIDTH-1:0] wdata_accum1_in;
wire [`DWIDTH-1:0] wdata_accum2_in;
wire [`DWIDTH-1:0] wdata_accum3_in;
wire [`DWIDTH-1:0] wdata_accum4_in;
wire [`DWIDTH-1:0] wdata_accum5_in;
wire [`DWIDTH-1:0] wdata_accum6_in;
wire [`DWIDTH-1:0] wdata_accum7_in;
wire [`DWIDTH-1:0] wdata_accum8_in;
wire [`DWIDTH-1:0] wdata_accum9_in;
wire [`DWIDTH-1:0] wdata_accum10_in;
wire [`DWIDTH-1:0] wdata_accum11_in;
wire [`DWIDTH-1:0] wdata_accum12_in;
wire [`DWIDTH-1:0] wdata_accum13_in;
wire [`DWIDTH-1:0] wdata_accum14_in;
wire [`DWIDTH-1:0] wdata_accum15_in;

// Data written into the PING Accumulators
wire [`DWIDTH-1:0] wdata_accum0_ping;
wire [`DWIDTH-1:0] wdata_accum1_ping;
wire [`DWIDTH-1:0] wdata_accum2_ping;
wire [`DWIDTH-1:0] wdata_accum3_ping;
wire [`DWIDTH-1:0] wdata_accum4_ping;
wire [`DWIDTH-1:0] wdata_accum5_ping;
wire [`DWIDTH-1:0] wdata_accum6_ping;
wire [`DWIDTH-1:0] wdata_accum7_ping;
wire [`DWIDTH-1:0] wdata_accum8_ping;
wire [`DWIDTH-1:0] wdata_accum9_ping;
wire [`DWIDTH-1:0] wdata_accum10_ping;
wire [`DWIDTH-1:0] wdata_accum11_ping;
wire [`DWIDTH-1:0] wdata_accum12_ping;
wire [`DWIDTH-1:0] wdata_accum13_ping;
wire [`DWIDTH-1:0] wdata_accum14_ping;
wire [`DWIDTH-1:0] wdata_accum15_ping;

wire [`AWIDTH-1:0] raddr_buffer0;
wire [`AWIDTH-1:0] raddr_buffer1;
wire [`AWIDTH-1:0] raddr_buffer2;
wire [`AWIDTH-1:0] raddr_buffer3;
wire [`AWIDTH-1:0] raddr_buffer4;
wire [`AWIDTH-1:0] raddr_buffer5;
wire [`AWIDTH-1:0] raddr_buffer6;
wire [`AWIDTH-1:0] raddr_buffer7;
wire [`AWIDTH-1:0] raddr_buffer8;
wire [`AWIDTH-1:0] raddr_buffer9;
wire [`AWIDTH-1:0] raddr_buffer10;
wire [`AWIDTH-1:0] raddr_buffer11;
wire [`AWIDTH-1:0] raddr_buffer12;
wire [`AWIDTH-1:0] raddr_buffer13;
wire [`AWIDTH-1:0] raddr_buffer14;
wire [`AWIDTH-1:0] raddr_buffer15;

wire [`DWIDTH-1:0] rdata_buffer0;
wire [`DWIDTH-1:0] rdata_buffer1;
wire [`DWIDTH-1:0] rdata_buffer2;
wire [`DWIDTH-1:0] rdata_buffer3;
wire [`DWIDTH-1:0] rdata_buffer4;
wire [`DWIDTH-1:0] rdata_buffer5;
wire [`DWIDTH-1:0] rdata_buffer6;
wire [`DWIDTH-1:0] rdata_buffer7;
wire [`DWIDTH-1:0] rdata_buffer8;
wire [`DWIDTH-1:0] rdata_buffer9;
wire [`DWIDTH-1:0] rdata_buffer10;
wire [`DWIDTH-1:0] rdata_buffer11;
wire [`DWIDTH-1:0] rdata_buffer12;
wire [`DWIDTH-1:0] rdata_buffer13;
wire [`DWIDTH-1:0] rdata_buffer14;
wire [`DWIDTH-1:0] rdata_buffer15;

wire [`DWIDTH-1:0] rdata_buffer0_pong;
wire [`DWIDTH-1:0] rdata_buffer1_pong;
wire [`DWIDTH-1:0] rdata_buffer2_pong;
wire [`DWIDTH-1:0] rdata_buffer3_pong;
wire [`DWIDTH-1:0] rdata_buffer4_pong;
wire [`DWIDTH-1:0] rdata_buffer5_pong;
wire [`DWIDTH-1:0] rdata_buffer6_pong;
wire [`DWIDTH-1:0] rdata_buffer7_pong;
wire [`DWIDTH-1:0] rdata_buffer8_pong;
wire [`DWIDTH-1:0] rdata_buffer9_pong;
wire [`DWIDTH-1:0] rdata_buffer10_pong;
wire [`DWIDTH-1:0] rdata_buffer11_pong;
wire [`DWIDTH-1:0] rdata_buffer12_pong;
wire [`DWIDTH-1:0] rdata_buffer13_pong;
wire [`DWIDTH-1:0] rdata_buffer14_pong;
wire [`DWIDTH-1:0] rdata_buffer15_pong;
    
// Based on the Accumulator Adder MUX select signal either 0 or data read from the RAM goes into the Adder
assign wdata_accum0_in = (~add_accum_mux0)?  8'b0 : (buffer_select)? rdata_buffer0 : rdata_buffer0_pong;
assign wdata_accum1_in = (~add_accum_mux1)?  8'b0 : (buffer_select)? rdata_buffer1 : rdata_buffer1_pong;
assign wdata_accum2_in = (~add_accum_mux2)?  8'b0 : (buffer_select)? rdata_buffer2 : rdata_buffer2_pong;
assign wdata_accum3_in = (~add_accum_mux3)?  8'b0 : (buffer_select)? rdata_buffer3 : rdata_buffer3_pong;
assign wdata_accum4_in = (~add_accum_mux4)?  8'b0 : (buffer_select)? rdata_buffer4 : rdata_buffer4_pong;
assign wdata_accum5_in = (~add_accum_mux5)?  8'b0 : (buffer_select)? rdata_buffer5 : rdata_buffer5_pong;
assign wdata_accum6_in = (~add_accum_mux6)?  8'b0 : (buffer_select)? rdata_buffer6 : rdata_buffer6_pong;
assign wdata_accum7_in = (~add_accum_mux7)?  8'b0 : (buffer_select)? rdata_buffer7 : rdata_buffer7_pong;
assign wdata_accum8_in = (~add_accum_mux8)?  8'b0 : (buffer_select)? rdata_buffer8 : rdata_buffer8_pong;
assign wdata_accum9_in = (~add_accum_mux9)?  8'b0 : (buffer_select)? rdata_buffer9 : rdata_buffer9_pong;
assign wdata_accum10_in = (~add_accum_mux10)?  8'b0 : (buffer_select)? rdata_buffer10 : rdata_buffer10_pong;
assign wdata_accum11_in = (~add_accum_mux11)?  8'b0 : (buffer_select)? rdata_buffer11 : rdata_buffer11_pong;
assign wdata_accum12_in = (~add_accum_mux12)?  8'b0 : (buffer_select)? rdata_buffer12 : rdata_buffer12_pong;
assign wdata_accum13_in = (~add_accum_mux13)?  8'b0 : (buffer_select)? rdata_buffer13 : rdata_buffer13_pong;
assign wdata_accum14_in = (~add_accum_mux14)?  8'b0 : (buffer_select)? rdata_buffer14 : rdata_buffer14_pong;
assign wdata_accum15_in = (~add_accum_mux15)?  8'b0 : (buffer_select)? rdata_buffer15 : rdata_buffer15_pong;
  
reg [`AWIDTH-1:0] raddr_accum0;
reg [`AWIDTH-1:0] raddr_accum1;
reg [`AWIDTH-1:0] raddr_accum2;
reg [`AWIDTH-1:0] raddr_accum3;
reg [`AWIDTH-1:0] raddr_accum4;
reg [`AWIDTH-1:0] raddr_accum5;
reg [`AWIDTH-1:0] raddr_accum6;
reg [`AWIDTH-1:0] raddr_accum7;
reg [`AWIDTH-1:0] raddr_accum8;
reg [`AWIDTH-1:0] raddr_accum9;
reg [`AWIDTH-1:0] raddr_accum10;
reg [`AWIDTH-1:0] raddr_accum11;
reg [`AWIDTH-1:0] raddr_accum12;
reg [`AWIDTH-1:0] raddr_accum13;
reg [`AWIDTH-1:0] raddr_accum14;
reg [`AWIDTH-1:0] raddr_accum15;
  
// Start reading the address written to after 15 clock cycles to calculate partial sums
always @ (posedge clk) begin
    raddr_accum0 <= waddr_accum14; // waddr_accum14 = (waddr_accum0 delayed by 14 clock cycles)
    raddr_accum1 <= raddr_accum0;
    raddr_accum2 <= raddr_accum1;
    raddr_accum3 <= raddr_accum2;
    raddr_accum4 <= raddr_accum3;
    raddr_accum5 <= raddr_accum4;
    raddr_accum6 <= raddr_accum5;
    raddr_accum7 <= raddr_accum6;
    raddr_accum8 <= raddr_accum7;
    raddr_accum9 <= raddr_accum8;
    raddr_accum10 <= raddr_accum9;
    raddr_accum11 <= raddr_accum10;
    raddr_accum12 <= raddr_accum11;
    raddr_accum13 <= raddr_accum12;
    raddr_accum14 <= raddr_accum13;
    raddr_accum15 <= raddr_accum14;
end
  
// Port 0 for each RAM is used for writing the data coming from the matmul as of now, not used for reading
wire [`DWIDTH-1:0] accum0_ping_q0_NC;
wire [`DWIDTH-1:0] accum1_ping_q0_NC;
wire [`DWIDTH-1:0] accum2_ping_q0_NC;
wire [`DWIDTH-1:0] accum3_ping_q0_NC;
wire [`DWIDTH-1:0] accum4_ping_q0_NC;
wire [`DWIDTH-1:0] accum5_ping_q0_NC;
wire [`DWIDTH-1:0] accum6_ping_q0_NC;
wire [`DWIDTH-1:0] accum7_ping_q0_NC;
wire [`DWIDTH-1:0] accum8_ping_q0_NC;
wire [`DWIDTH-1:0] accum9_ping_q0_NC;
wire [`DWIDTH-1:0] accum10_ping_q0_NC;
wire [`DWIDTH-1:0] accum11_ping_q0_NC;
wire [`DWIDTH-1:0] accum12_ping_q0_NC;
wire [`DWIDTH-1:0] accum13_ping_q0_NC;
wire [`DWIDTH-1:0] accum14_ping_q0_NC;
wire [`DWIDTH-1:0] accum15_ping_q0_NC;
wire [`DWIDTH-1:0] accum0_pong_q0_NC;
wire [`DWIDTH-1:0] accum1_pong_q0_NC;
wire [`DWIDTH-1:0] accum2_pong_q0_NC;
wire [`DWIDTH-1:0] accum3_pong_q0_NC;
wire [`DWIDTH-1:0] accum4_pong_q0_NC;
wire [`DWIDTH-1:0] accum5_pong_q0_NC;
wire [`DWIDTH-1:0] accum6_pong_q0_NC;
wire [`DWIDTH-1:0] accum7_pong_q0_NC;
wire [`DWIDTH-1:0] accum8_pong_q0_NC;
wire [`DWIDTH-1:0] accum9_pong_q0_NC;
wire [`DWIDTH-1:0] accum10_pong_q0_NC;
wire [`DWIDTH-1:0] accum11_pong_q0_NC;
wire [`DWIDTH-1:0] accum12_pong_q0_NC;
wire [`DWIDTH-1:0] accum13_pong_q0_NC;
wire [`DWIDTH-1:0] accum14_pong_q0_NC;
wire [`DWIDTH-1:0] accum15_pong_q0_NC;

reg buffer_select_pool1;
reg buffer_select_pool2;
reg buffer_select_pool3;
reg buffer_select_pool4;
reg buffer_select_pool5;
reg buffer_select_pool6;
reg buffer_select_pool7;
reg buffer_select_pool8;
reg buffer_select_pool9;
reg buffer_select_pool10;
reg buffer_select_pool11;
reg buffer_select_pool12;
reg buffer_select_pool13;
reg buffer_select_pool14;
reg buffer_select_pool15;
  
always @ (posedge clk) begin
buffer_select_pool1 <= buffer_select_pool;
buffer_select_pool2 <= buffer_select_pool1;
buffer_select_pool3 <= buffer_select_pool2;
buffer_select_pool4 <= buffer_select_pool3;
buffer_select_pool5 <= buffer_select_pool4;
buffer_select_pool6 <= buffer_select_pool5;
buffer_select_pool7 <= buffer_select_pool6;
buffer_select_pool8 <= buffer_select_pool7;
buffer_select_pool9 <= buffer_select_pool8;
buffer_select_pool10 <= buffer_select_pool9;
buffer_select_pool11 <= buffer_select_pool10;
buffer_select_pool12 <= buffer_select_pool11;
buffer_select_pool13 <= buffer_select_pool12;
buffer_select_pool14 <= buffer_select_pool13;
buffer_select_pool15 <= buffer_select_pool14;
end

reg buffer_select_accum1;
reg buffer_select_accum2;
reg buffer_select_accum3;
reg buffer_select_accum4;
reg buffer_select_accum5;
reg buffer_select_accum6;
reg buffer_select_accum7;
reg buffer_select_accum8;
reg buffer_select_accum9;
reg buffer_select_accum10;
reg buffer_select_accum11;
reg buffer_select_accum12;
reg buffer_select_accum13;
reg buffer_select_accum14;
reg buffer_select_accum15;
  
always @ (posedge clk) begin
buffer_select_accum1 <= buffer_select_accum;
buffer_select_accum2 <= buffer_select_accum1;
buffer_select_accum3 <= buffer_select_accum2;
buffer_select_accum4 <= buffer_select_accum3;
buffer_select_accum5 <= buffer_select_accum4;
buffer_select_accum6 <= buffer_select_accum5;
buffer_select_accum7 <= buffer_select_accum6;
buffer_select_accum8 <= buffer_select_accum7;
buffer_select_accum9 <= buffer_select_accum8;
buffer_select_accum10 <= buffer_select_accum9;
buffer_select_accum11 <= buffer_select_accum10;
buffer_select_accum12 <= buffer_select_accum11;
buffer_select_accum13 <= buffer_select_accum12;
buffer_select_accum14 <= buffer_select_accum13;
buffer_select_accum15 <= buffer_select_accum14;
end

assign raddr_buffer0 = (buffer_select_pool)? raddr_accum0_pool : (buffer_select_accum)? raddr_accum0:11'd0;
assign raddr_buffer1 = (buffer_select_pool1)? raddr_accum1_pool : (buffer_select_accum1)? raddr_accum1:11'd0;
assign raddr_buffer2 = (buffer_select_pool2)? raddr_accum2_pool : (buffer_select_accum2)? raddr_accum2:11'd0;
assign raddr_buffer3 = (buffer_select_pool3)? raddr_accum3_pool : (buffer_select_accum3)? raddr_accum3:11'd0;
assign raddr_buffer4 = (buffer_select_pool4)? raddr_accum4_pool : (buffer_select_accum4)? raddr_accum4:11'd0;
assign raddr_buffer5 = (buffer_select_pool5)? raddr_accum5_pool : (buffer_select_accum5)? raddr_accum5:11'd0;
assign raddr_buffer6 = (buffer_select_pool6)? raddr_accum6_pool : (buffer_select_accum6)? raddr_accum6:11'd0;
assign raddr_buffer7 = (buffer_select_pool7)? raddr_accum7_pool : (buffer_select_accum7)? raddr_accum7:11'd0;
assign raddr_buffer8 = (buffer_select_pool8)? raddr_accum8_pool : (buffer_select_accum8)? raddr_accum8:11'd0;
assign raddr_buffer9 = (buffer_select_pool9)? raddr_accum9_pool : (buffer_select_accum9)? raddr_accum9:11'd0;
assign raddr_buffer10 = (buffer_select_pool10)? raddr_accum10_pool : (buffer_select_accum10)? raddr_accum10:11'd0;
assign raddr_buffer11 = (buffer_select_pool11)? raddr_accum11_pool : (buffer_select_accum11)? raddr_accum11:11'd0;
assign raddr_buffer12 = (buffer_select_pool12)? raddr_accum12_pool : (buffer_select_accum12)? raddr_accum12:11'd0;
assign raddr_buffer13 = (buffer_select_pool13)? raddr_accum13_pool : (buffer_select_accum13)? raddr_accum13:11'd0;
assign raddr_buffer14 = (buffer_select_pool14)? raddr_accum14_pool : (buffer_select_accum14)? raddr_accum14:11'd0;
assign raddr_buffer15 = (buffer_select_pool15)? raddr_accum15_pool : (buffer_select_accum15)? raddr_accum15:11'd0;
  
assign rdata_accum0_pool =  (buffer_select_pool)?  (buffer_select)? rdata_buffer0 : rdata_buffer0_pong : 8'b0;
assign rdata_accum1_pool =  (buffer_select_pool1)? (buffer_select)? rdata_buffer1 : rdata_buffer1_pong : 8'b0;
assign rdata_accum2_pool =  (buffer_select_pool2)? (buffer_select)? rdata_buffer2 : rdata_buffer2_pong : 8'b0;
assign rdata_accum3_pool =  (buffer_select_pool3)? (buffer_select)? rdata_buffer3 : rdata_buffer3_pong : 8'b0;
assign rdata_accum4_pool =  (buffer_select_pool4)? (buffer_select)? rdata_buffer4 : rdata_buffer4_pong : 8'b0;
assign rdata_accum5_pool =  (buffer_select_pool5)? (buffer_select)? rdata_buffer5 : rdata_buffer5_pong : 8'b0;
assign rdata_accum6_pool =  (buffer_select_pool6)? (buffer_select)? rdata_buffer6 : rdata_buffer6_pong : 8'b0;
assign rdata_accum7_pool =  (buffer_select_pool7)? (buffer_select)? rdata_buffer7 : rdata_buffer7_pong : 8'b0;
assign rdata_accum8_pool =  (buffer_select_pool8)? (buffer_select)? rdata_buffer8 : rdata_buffer8_pong : 8'b0;
assign rdata_accum9_pool =  (buffer_select_pool9)? (buffer_select)? rdata_buffer9 : rdata_buffer9_pong : 8'b0;
assign rdata_accum10_pool =  (buffer_select_pool10)? (buffer_select)? rdata_buffer10 : rdata_buffer10_pong : 8'b0;
assign rdata_accum11_pool =  (buffer_select_pool11)? (buffer_select)? rdata_buffer11 : rdata_buffer11_pong : 8'b0;
assign rdata_accum12_pool =  (buffer_select_pool12)? (buffer_select)? rdata_buffer12 : rdata_buffer12_pong : 8'b0;
assign rdata_accum13_pool =  (buffer_select_pool13)? (buffer_select)? rdata_buffer13 : rdata_buffer13_pong : 8'b0;
assign rdata_accum14_pool =  (buffer_select_pool14)? (buffer_select)? rdata_buffer14 : rdata_buffer14_pong : 8'b0;
assign rdata_accum15_pool =  (buffer_select_pool15)? (buffer_select)? rdata_buffer15 : rdata_buffer15_pong : 8'b0;
  
//Need to use simple dual port ram for QUARTUS and true dual port RAM for VTR

`ifdef QUARTUS

////////////////////////////////////////////////
// PING ACCUMULATORS
////////////////////////////////////////////////

qadd adder_accum_ping0 (wdata_accum0, wdata_accum0_in, wdata_accum0_ping);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum0_ping (
    .addr0(waddr_accum0),
    .d0(wdata_accum0_ping), 
    .we0(wdata_en_ping0), 
    .addr1(raddr_buffer0),
    .q1(rdata_buffer0), 
    .clk(clk)
);

qadd adder_accum_ping1 (wdata_accum1, wdata_accum1_in, wdata_accum1_ping);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum1_ping (
    .addr0(waddr_accum1),
    .d0(wdata_accum1_ping), 
    .we0(wdata_en_ping1), 
    .addr1(raddr_buffer1), 
    .q1(rdata_buffer1), 
    .clk(clk)
);

qadd adder_accum_ping2 (wdata_accum2, wdata_accum2_in, wdata_accum2_ping);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum2_ping (
    .addr0(waddr_accum2),
    .d0(wdata_accum2_ping), 
    .we0(wdata_en_ping2), 
    .addr1(raddr_buffer2),
    .q1(rdata_buffer2), 
    .clk(clk)
);

qadd adder_accum_ping3 (wdata_accum3, wdata_accum3_in, wdata_accum3_ping);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum3_ping (
    .addr0(waddr_accum3),
    .d0(wdata_accum3_ping), 
    .we0(wdata_en_ping3), 
    .addr1(raddr_buffer3),
    .q1(rdata_buffer3), 
    .clk(clk)
);

qadd adder_accum_ping4 (wdata_accum4, wdata_accum4_in, wdata_accum4_ping);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum4_ping (
    .addr0(waddr_accum4),
    .d0(wdata_accum4_ping), 
    .we0(wdata_en_ping4), 
    .addr1(raddr_buffer4),
    .q1(rdata_buffer4), 
    .clk(clk)
);

qadd adder_accum_ping5 (wdata_accum5, wdata_accum5_in, wdata_accum5_ping);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum5_ping (
    .addr0(waddr_accum5),
    .d0(wdata_accum5_ping), 
    .we0(wdata_en_ping5), 
    .addr1(raddr_buffer5),
    .q1(rdata_buffer5), 
    .clk(clk)
);

qadd adder_accum_ping6 (wdata_accum6, wdata_accum6_in, wdata_accum6_ping);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum6_ping (
    .addr0(waddr_accum6),
    .d0(wdata_accum6_ping), 
    .we0(wdata_en_ping6), 
    .addr1(raddr_buffer6),
    .q1(rdata_buffer6), 
    .clk(clk)
);

qadd adder_accum_ping7 (wdata_accum7, wdata_accum7_in, wdata_accum7_ping);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum7_ping (
    .addr0(waddr_accum7),
    .d0(wdata_accum7_ping), 
    .we0(wdata_en_ping7), 
    .addr1(raddr_buffer7),
    .q1(rdata_buffer7), 
    .clk(clk)
);

qadd adder_accum_ping8 (wdata_accum8, wdata_accum8_in, wdata_accum8_ping);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum8_ping (
    .addr0(waddr_accum8),
    .d0(wdata_accum8_ping), 
    .we0(wdata_en_ping8), 
    .addr1(raddr_buffer8),
    .q1(rdata_buffer8), 
    .clk(clk)
);

qadd adder_accum_ping9 (wdata_accum9, wdata_accum9_in, wdata_accum9_ping);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum9_ping (
    .addr0(waddr_accum9),
    .d0(wdata_accum9_ping), 
    .we0(wdata_en_ping9), 
    .addr1(raddr_buffer9),
    .q1(rdata_buffer9), 
    .clk(clk)
);

qadd adder_accum_ping10 (wdata_accum10, wdata_accum10_in, wdata_accum10_ping);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum10_ping (
    .addr0(waddr_accum10),
    .d0(wdata_accum10_ping), 
    .we0(wdata_en_ping10), 
    .addr1(raddr_buffer10),
    .q1(rdata_buffer10), 
    .clk(clk)
);

qadd adder_accum_ping11 (wdata_accum11, wdata_accum11_in, wdata_accum11_ping);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum11_ping (
    .addr0(waddr_accum11),
    .d0(wdata_accum11_ping), 
    .we0(wdata_en_ping11), 
    .addr1(raddr_buffer11),
    .q1(rdata_buffer11), 
    .clk(clk)
);

qadd adder_accum_ping12 (wdata_accum12, wdata_accum12_in, wdata_accum12_ping);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum12_ping (
    .addr0(waddr_accum12),
    .d0(wdata_accum12_ping), 
    .we0(wdata_en_ping12), 
    .addr1(raddr_buffer12),
    .q1(rdata_buffer12), 
    .clk(clk)
);

qadd adder_accum_ping13 (wdata_accum13, wdata_accum13_in, wdata_accum13_ping);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum13_ping (
    .addr0(waddr_accum13),
    .d0(wdata_accum13_ping), 
    .we0(wdata_en_ping13), 
    .addr1(raddr_buffer13),
    .q1(rdata_buffer13), 
    .clk(clk)
);

qadd adder_accum_ping14 (wdata_accum14, wdata_accum14_in, wdata_accum14_ping);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum14_ping (
    .addr0(waddr_accum14),
    .d0(wdata_accum14_ping), 
    .we0(wdata_en_ping14), 
    .addr1(raddr_buffer14),
    .q1(rdata_buffer14), 
    .clk(clk)
);

qadd adder_accum_ping15 (wdata_accum15, wdata_accum15_in, wdata_accum15_ping);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum15_ping (
    .addr0(waddr_accum15),
    .d0(wdata_accum15_ping), 
    .we0(wdata_en_ping15), 
    .addr1(raddr_buffer15),
    .q1(rdata_buffer15), 
    .clk(clk)
);

wire [`DWIDTH-1:0] wdata_accum0_pong;
wire [`DWIDTH-1:0] wdata_accum1_pong;
wire [`DWIDTH-1:0] wdata_accum2_pong;
wire [`DWIDTH-1:0] wdata_accum3_pong;
wire [`DWIDTH-1:0] wdata_accum4_pong;
wire [`DWIDTH-1:0] wdata_accum5_pong;
wire [`DWIDTH-1:0] wdata_accum6_pong;
wire [`DWIDTH-1:0] wdata_accum7_pong;
wire [`DWIDTH-1:0] wdata_accum8_pong;
wire [`DWIDTH-1:0] wdata_accum9_pong;
wire [`DWIDTH-1:0] wdata_accum10_pong;
wire [`DWIDTH-1:0] wdata_accum11_pong;
wire [`DWIDTH-1:0] wdata_accum12_pong;
wire [`DWIDTH-1:0] wdata_accum13_pong;
wire [`DWIDTH-1:0] wdata_accum14_pong;
wire [`DWIDTH-1:0] wdata_accum15_pong;

////////////////////////////////////////////////
// PONG ACCUMULATORS
////////////////////////////////////////////////

qadd adder_accum_pong0 (wdata_accum0, wdata_accum0_in, wdata_accum0_pong);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum0_pong (
    .addr0(waddr_accum0),
    .d0(wdata_accum0_pong), 
    .we0(wdata_en_pong0), 
    .addr1(raddr_buffer0),
    .q1(rdata_buffer0_pong), 
    .clk(clk)
);

qadd adder_accum_pong1 (wdata_accum1, wdata_accum1_in, wdata_accum1_pong);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum1_pong (
    .addr0(waddr_accum1),
    .d0(wdata_accum1_pong), 
    .we0(wdata_en_pong1), 
    .addr1(raddr_buffer1),
    .q1(rdata_buffer1_pong), 
    .clk(clk)
);

qadd adder_accum_pong2 (wdata_accum2, wdata_accum2_in, wdata_accum2_pong);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum2_pong (
    .addr0(waddr_accum2),
    .d0(wdata_accum2_pong), 
    .we0(wdata_en_pong2), 
    .addr1(raddr_buffer2),
    .q1(rdata_buffer2_pong), 
    .clk(clk)
);

qadd adder_accum_pong3 (wdata_accum3, wdata_accum3_in, wdata_accum3_pong);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum3_pong (
    .addr0(waddr_accum3),
    .d0(wdata_accum3_pong), 
    .we0(wdata_en_pong3), 
    .addr1(raddr_buffer3),
    .q1(rdata_buffer3_pong), 
    .clk(clk)
);

qadd adder_accum_pong4 (wdata_accum4, wdata_accum4_in, wdata_accum4_pong);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum4_pong (
    .addr0(waddr_accum4),
    .d0(wdata_accum4_pong), 
    .we0(wdata_en_pong4), 
    .addr1(raddr_buffer4),
    .q1(rdata_buffer4_pong), 
    .clk(clk)
);

qadd adder_accum_pong5 (wdata_accum5, wdata_accum5_in, wdata_accum5_pong);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum5_pong (
    .addr0(waddr_accum5),
    .d0(wdata_accum5_pong), 
    .we0(wdata_en_pong5), 
    .addr1(raddr_buffer5),
    .q1(rdata_buffer5_pong), 
    .clk(clk)
);

qadd adder_accum_pong6 (wdata_accum6, wdata_accum6_in, wdata_accum6_pong);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum6_pong (
    .addr0(waddr_accum6),
    .d0(wdata_accum6_pong), 
    .we0(wdata_en_pong6), 
    .addr1(raddr_buffer6),
    .q1(rdata_buffer6_pong), 
    .clk(clk)
);

qadd adder_accum_pong7 (wdata_accum7, wdata_accum7_in, wdata_accum7_pong);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum7_pong (
    .addr0(waddr_accum7),
    .d0(wdata_accum7_pong), 
    .we0(wdata_en_pong7), 
    .addr1(raddr_buffer7),
    .q1(rdata_buffer7_pong), 
    .clk(clk)
);

qadd adder_accum_pong8 (wdata_accum8, wdata_accum8_in, wdata_accum8_pong);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum8_pong (
    .addr0(waddr_accum8),
    .d0(wdata_accum8_pong), 
    .we0(wdata_en_pong8), 
    .addr1(raddr_buffer8),
    .q1(rdata_buffer8_pong), 
    .clk(clk)
);

qadd adder_accum_pong9 (wdata_accum9, wdata_accum9_in, wdata_accum9_pong);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum9_pong (
    .addr0(waddr_accum9),
    .d0(wdata_accum9_pong), 
    .we0(wdata_en_pong9), 
    .addr1(raddr_buffer9),
    .q1(rdata_buffer9_pong), 
    .clk(clk)
);

qadd adder_accum_pong10 (wdata_accum10, wdata_accum10_in, wdata_accum10_pong);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum10_pong (
    .addr0(waddr_accum10),
    .d0(wdata_accum10_pong), 
    .we0(wdata_en_pong10), 
    .addr1(raddr_buffer10),
    .q1(rdata_buffer10_pong), 
    .clk(clk)
);

qadd adder_accum_pong11 (wdata_accum11, wdata_accum11_in, wdata_accum11_pong);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum11_pong (
    .addr0(waddr_accum11),
    .d0(wdata_accum11_pong), 
    .we0(wdata_en_pong11), 
    .addr1(raddr_buffer11),
    .q1(rdata_buffer11_pong), 
    .clk(clk)
);

qadd adder_accum_pong12 (wdata_accum12, wdata_accum12_in, wdata_accum12_pong);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum12_pong (
    .addr0(waddr_accum12),
    .d0(wdata_accum12_pong), 
    .we0(wdata_en_pong12), 
    .addr1(raddr_buffer12),
    .q1(rdata_buffer12_pong), 
    .clk(clk)
);

qadd adder_accum_pong13 (wdata_accum13, wdata_accum13_in, wdata_accum13_pong);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum13_pong (
    .addr0(waddr_accum13),
    .d0(wdata_accum13_pong), 
    .we0(wdata_en_pong13), 
    .addr1(raddr_buffer13),
    .q1(rdata_buffer13_pong), 
    .clk(clk)
);

qadd adder_accum_pong14 (wdata_accum14, wdata_accum14_in, wdata_accum14_pong);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum14_pong (
    .addr0(waddr_accum14),
    .d0(wdata_accum14_pong), 
    .we0(wdata_en_pong14), 
    .addr1(raddr_buffer14),
    .q1(rdata_buffer14_pong), 
    .clk(clk)
);

qadd adder_accum_pong15 (wdata_accum15, wdata_accum15_in, wdata_accum15_pong);  
simple_ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum15_pong (
    .addr0(waddr_accum15),
    .d0(wdata_accum15_pong), 
    .we0(wdata_en_pong15), 
    .addr1(raddr_buffer15),
    .q1(rdata_buffer15_pong), 
    .clk(clk)
);


`else

////////////////////////////////////////////////
// PING ACCUMULATORS
////////////////////////////////////////////////

qadd adder_accum_ping0 (wdata_accum0, wdata_accum0_in, wdata_accum0_ping);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum0_ping (
    .addr0(waddr_accum0),
    .d0(wdata_accum0_ping), 
    .we0(wdata_en_ping0), 
    .q0(accum0_ping_q0_NC),
    .addr1(raddr_buffer0),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer0), 
    .clk(clk)
);

qadd adder_accum_ping1 (wdata_accum1, wdata_accum1_in, wdata_accum1_ping);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum1_ping (
    .addr0(waddr_accum1),
    .d0(wdata_accum1_ping), 
    .we0(wdata_en_ping1), 
    .q0(accum1_ping_q0_NC),
    .addr1(raddr_buffer1),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer1), 
    .clk(clk)
);

qadd adder_accum_ping2 (wdata_accum2, wdata_accum2_in, wdata_accum2_ping);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum2_ping (
    .addr0(waddr_accum2),
    .d0(wdata_accum2_ping), 
    .we0(wdata_en_ping2), 
    .q0(accum2_ping_q0_NC),
    .addr1(raddr_buffer2),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer2), 
    .clk(clk)
);

qadd adder_accum_ping3 (wdata_accum3, wdata_accum3_in, wdata_accum3_ping);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum3_ping (
    .addr0(waddr_accum3),
    .d0(wdata_accum3_ping), 
    .we0(wdata_en_ping3), 
    .q0(accum3_ping_q0_NC),
    .addr1(raddr_buffer3),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer3), 
    .clk(clk)
);

qadd adder_accum_ping4 (wdata_accum4, wdata_accum4_in, wdata_accum4_ping);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum4_ping (
    .addr0(waddr_accum4),
    .d0(wdata_accum4_ping), 
    .we0(wdata_en_ping4), 
    .q0(accum4_ping_q0_NC),
    .addr1(raddr_buffer4),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer4), 
    .clk(clk)
);

qadd adder_accum_ping5 (wdata_accum5, wdata_accum5_in, wdata_accum5_ping);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum5_ping (
    .addr0(waddr_accum5),
    .d0(wdata_accum5_ping), 
    .we0(wdata_en_ping5), 
    .q0(accum5_ping_q0_NC),
    .addr1(raddr_buffer5),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer5), 
    .clk(clk)
);

qadd adder_accum_ping6 (wdata_accum6, wdata_accum6_in, wdata_accum6_ping);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum6_ping (
    .addr0(waddr_accum6),
    .d0(wdata_accum6_ping), 
    .we0(wdata_en_ping6), 
    .q0(accum6_ping_q0_NC),
    .addr1(raddr_buffer6),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer6), 
    .clk(clk)
);

qadd adder_accum_ping7 (wdata_accum7, wdata_accum7_in, wdata_accum7_ping);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum7_ping (
    .addr0(waddr_accum7),
    .d0(wdata_accum7_ping), 
    .we0(wdata_en_ping7), 
    .q0(accum7_ping_q0_NC),
    .addr1(raddr_buffer7),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer7), 
    .clk(clk)
);

qadd adder_accum_ping8 (wdata_accum8, wdata_accum8_in, wdata_accum8_ping);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum8_ping (
    .addr0(waddr_accum8),
    .d0(wdata_accum8_ping), 
    .we0(wdata_en_ping8), 
    .q0(accum8_ping_q0_NC),
    .addr1(raddr_buffer8),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer8), 
    .clk(clk)
);

qadd adder_accum_ping9 (wdata_accum9, wdata_accum9_in, wdata_accum9_ping);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum9_ping (
    .addr0(waddr_accum9),
    .d0(wdata_accum9_ping), 
    .we0(wdata_en_ping9), 
    .q0(accum9_ping_q0_NC),
    .addr1(raddr_buffer9),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer9), 
    .clk(clk)
);

qadd adder_accum_ping10 (wdata_accum10, wdata_accum10_in, wdata_accum10_ping);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum10_ping (
    .addr0(waddr_accum10),
    .d0(wdata_accum10_ping), 
    .we0(wdata_en_ping10), 
    .q0(accum10_ping_q0_NC),
    .addr1(raddr_buffer10),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer10), 
    .clk(clk)
);

qadd adder_accum_ping11 (wdata_accum11, wdata_accum11_in, wdata_accum11_ping);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum11_ping (
    .addr0(waddr_accum11),
    .d0(wdata_accum11_ping), 
    .we0(wdata_en_ping11), 
    .q0(accum11_ping_q0_NC),
    .addr1(raddr_buffer11),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer11), 
    .clk(clk)
);

qadd adder_accum_ping12 (wdata_accum12, wdata_accum12_in, wdata_accum12_ping);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum12_ping (
    .addr0(waddr_accum12),
    .d0(wdata_accum12_ping), 
    .we0(wdata_en_ping12), 
    .q0(accum12_ping_q0_NC),
    .addr1(raddr_buffer12),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer12), 
    .clk(clk)
);

qadd adder_accum_ping13 (wdata_accum13, wdata_accum13_in, wdata_accum13_ping);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum13_ping (
    .addr0(waddr_accum13),
    .d0(wdata_accum13_ping), 
    .we0(wdata_en_ping13), 
    .q0(accum13_ping_q0_NC),
    .addr1(raddr_buffer13),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer13), 
    .clk(clk)
);

qadd adder_accum_ping14 (wdata_accum14, wdata_accum14_in, wdata_accum14_ping);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum14_ping (
    .addr0(waddr_accum14),
    .d0(wdata_accum14_ping), 
    .we0(wdata_en_ping14), 
    .q0(accum14_ping_q0_NC),
    .addr1(raddr_buffer14),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer14), 
    .clk(clk)
);

qadd adder_accum_ping15 (wdata_accum15, wdata_accum15_in, wdata_accum15_ping);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum15_ping (
    .addr0(waddr_accum15),
    .d0(wdata_accum15_ping), 
    .we0(wdata_en_ping15), 
    .q0(accum15_ping_q0_NC),
    .addr1(raddr_buffer15),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer15), 
    .clk(clk)
);

wire [`DWIDTH-1:0] wdata_accum0_pong;
wire [`DWIDTH-1:0] wdata_accum1_pong;
wire [`DWIDTH-1:0] wdata_accum2_pong;
wire [`DWIDTH-1:0] wdata_accum3_pong;
wire [`DWIDTH-1:0] wdata_accum4_pong;
wire [`DWIDTH-1:0] wdata_accum5_pong;
wire [`DWIDTH-1:0] wdata_accum6_pong;
wire [`DWIDTH-1:0] wdata_accum7_pong;
wire [`DWIDTH-1:0] wdata_accum8_pong;
wire [`DWIDTH-1:0] wdata_accum9_pong;
wire [`DWIDTH-1:0] wdata_accum10_pong;
wire [`DWIDTH-1:0] wdata_accum11_pong;
wire [`DWIDTH-1:0] wdata_accum12_pong;
wire [`DWIDTH-1:0] wdata_accum13_pong;
wire [`DWIDTH-1:0] wdata_accum14_pong;
wire [`DWIDTH-1:0] wdata_accum15_pong;

////////////////////////////////////////////////
// PONG ACCUMULATORS
////////////////////////////////////////////////

qadd adder_accum_pong0 (wdata_accum0, wdata_accum0_in, wdata_accum0_pong);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum0_pong (
    .addr0(waddr_accum0),
    .d0(wdata_accum0_pong), 
    .we0(wdata_en_pong0), 
    .q0(accum0_pong_q0_NC),
    .addr1(raddr_buffer0),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer0_pong), 
    .clk(clk)
);

qadd adder_accum_pong1 (wdata_accum1, wdata_accum1_in, wdata_accum1_pong);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum1_pong (
    .addr0(waddr_accum1),
    .d0(wdata_accum1_pong), 
    .we0(wdata_en_pong1), 
    .q0(accum1_pong_q0_NC),
    .addr1(raddr_buffer1),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer1_pong), 
    .clk(clk)
);

qadd adder_accum_pong2 (wdata_accum2, wdata_accum2_in, wdata_accum2_pong);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum2_pong (
    .addr0(waddr_accum2),
    .d0(wdata_accum2_pong), 
    .we0(wdata_en_pong2), 
    .q0(accum2_pong_q0_NC),
    .addr1(raddr_buffer2),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer2_pong), 
    .clk(clk)
);

qadd adder_accum_pong3 (wdata_accum3, wdata_accum3_in, wdata_accum3_pong);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum3_pong (
    .addr0(waddr_accum3),
    .d0(wdata_accum3_pong), 
    .we0(wdata_en_pong3), 
    .q0(accum3_pong_q0_NC),
    .addr1(raddr_buffer3),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer3_pong), 
    .clk(clk)
);

qadd adder_accum_pong4 (wdata_accum4, wdata_accum4_in, wdata_accum4_pong);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum4_pong (
    .addr0(waddr_accum4),
    .d0(wdata_accum4_pong), 
    .we0(wdata_en_pong4), 
    .q0(accum4_pong_q0_NC),
    .addr1(raddr_buffer4),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer4_pong), 
    .clk(clk)
);

qadd adder_accum_pong5 (wdata_accum5, wdata_accum5_in, wdata_accum5_pong);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum5_pong (
    .addr0(waddr_accum5),
    .d0(wdata_accum5_pong), 
    .we0(wdata_en_pong5), 
    .q0(accum5_pong_q0_NC),
    .addr1(raddr_buffer5),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer5_pong), 
    .clk(clk)
);

qadd adder_accum_pong6 (wdata_accum6, wdata_accum6_in, wdata_accum6_pong);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum6_pong (
    .addr0(waddr_accum6),
    .d0(wdata_accum6_pong), 
    .we0(wdata_en_pong6), 
    .q0(accum6_pong_q0_NC),
    .addr1(raddr_buffer6),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer6_pong), 
    .clk(clk)
);

qadd adder_accum_pong7 (wdata_accum7, wdata_accum7_in, wdata_accum7_pong);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum7_pong (
    .addr0(waddr_accum7),
    .d0(wdata_accum7_pong), 
    .we0(wdata_en_pong7), 
    .q0(accum7_pong_q0_NC),
    .addr1(raddr_buffer7),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer7_pong), 
    .clk(clk)
);

qadd adder_accum_pong8 (wdata_accum8, wdata_accum8_in, wdata_accum8_pong);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum8_pong (
    .addr0(waddr_accum8),
    .d0(wdata_accum8_pong), 
    .we0(wdata_en_pong8), 
    .q0(accum8_pong_q0_NC),
    .addr1(raddr_buffer8),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer8_pong), 
    .clk(clk)
);

qadd adder_accum_pong9 (wdata_accum9, wdata_accum9_in, wdata_accum9_pong);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum9_pong (
    .addr0(waddr_accum9),
    .d0(wdata_accum9_pong), 
    .we0(wdata_en_pong9), 
    .q0(accum9_pong_q0_NC),
    .addr1(raddr_buffer9),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer9_pong), 
    .clk(clk)
);

qadd adder_accum_pong10 (wdata_accum10, wdata_accum10_in, wdata_accum10_pong);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum10_pong (
    .addr0(waddr_accum10),
    .d0(wdata_accum10_pong), 
    .we0(wdata_en_pong10), 
    .q0(accum10_pong_q0_NC),
    .addr1(raddr_buffer10),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer10_pong), 
    .clk(clk)
);

qadd adder_accum_pong11 (wdata_accum11, wdata_accum11_in, wdata_accum11_pong);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum11_pong (
    .addr0(waddr_accum11),
    .d0(wdata_accum11_pong), 
    .we0(wdata_en_pong11), 
    .q0(accum11_pong_q0_NC),
    .addr1(raddr_buffer11),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer11_pong), 
    .clk(clk)
);

qadd adder_accum_pong12 (wdata_accum12, wdata_accum12_in, wdata_accum12_pong);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum12_pong (
    .addr0(waddr_accum12),
    .d0(wdata_accum12_pong), 
    .we0(wdata_en_pong12), 
    .q0(accum12_pong_q0_NC),
    .addr1(raddr_buffer12),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer12_pong), 
    .clk(clk)
);

qadd adder_accum_pong13 (wdata_accum13, wdata_accum13_in, wdata_accum13_pong);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum13_pong (
    .addr0(waddr_accum13),
    .d0(wdata_accum13_pong), 
    .we0(wdata_en_pong13), 
    .q0(accum13_pong_q0_NC),
    .addr1(raddr_buffer13),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer13_pong), 
    .clk(clk)
);

qadd adder_accum_pong14 (wdata_accum14, wdata_accum14_in, wdata_accum14_pong);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum14_pong (
    .addr0(waddr_accum14),
    .d0(wdata_accum14_pong), 
    .we0(wdata_en_pong14), 
    .q0(accum14_pong_q0_NC),
    .addr1(raddr_buffer14),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer14_pong), 
    .clk(clk)
);

qadd adder_accum_pong15 (wdata_accum15, wdata_accum15_in, wdata_accum15_pong);  
ram #(.AW(`AWIDTH), .MW(MWIDTH), .DW(`DWIDTH)) accum15_pong (
    .addr0(waddr_accum15),
    .d0(wdata_accum15_pong), 
    .we0(wdata_en_pong15), 
    .q0(accum15_pong_q0_NC),
    .addr1(raddr_buffer15),
    .d1(8'b0), 
    .we1(1'b0), 
    .q1(rdata_buffer15_pong), 
    .clk(clk)
);

`endif

endmodule

////////////////////////////////////////////////////////////////////////////////
// THIS FILE WAS AUTOMATICALLY GENERATED FROM generate_pool.v.mako
// DO NOT EDIT
////////////////////////////////////////////////////////////////////////////////

module pooling(
    clk,
    resetn,
    start_pooling,
    pool_select,
    pool_norm_valid,
    enable_pool,
    rdata_accum0_pool,
    rdata_accum1_pool,
    rdata_accum2_pool,
    rdata_accum3_pool,
    rdata_accum4_pool,
    rdata_accum5_pool,
    rdata_accum6_pool,
    rdata_accum7_pool,
    rdata_accum8_pool,
    rdata_accum9_pool,
    rdata_accum10_pool,
    rdata_accum11_pool,
    rdata_accum12_pool,
    rdata_accum13_pool,
    rdata_accum14_pool,
    rdata_accum15_pool,
    raddr_accum0_pool,
    raddr_accum1_pool,
    raddr_accum2_pool,
    raddr_accum3_pool,
    raddr_accum4_pool,
    raddr_accum5_pool,
    raddr_accum6_pool,
    raddr_accum7_pool,
    raddr_accum8_pool,
    raddr_accum9_pool,
    raddr_accum10_pool,
    raddr_accum11_pool,
    raddr_accum12_pool,
    raddr_accum13_pool,
    raddr_accum14_pool,
    raddr_accum15_pool,
    pool0,
    pool1,
    pool2,
    pool3,
    pool4,
    pool5,
    pool6,
    pool7,
    pool8,
    pool9,
    pool10,
    pool11,
    pool12,
    pool13,
    pool14,
    pool15,
    matrix_size,
    filter_size
);

input clk;
input resetn;
input start_pooling;
input pool_select;
input enable_pool;
output pool_norm_valid;
output reg [`DWIDTH-1:0] pool0;
output reg [`DWIDTH-1:0] pool1;
output reg [`DWIDTH-1:0] pool2;
output reg [`DWIDTH-1:0] pool3;
output reg [`DWIDTH-1:0] pool4;
output reg [`DWIDTH-1:0] pool5;
output reg [`DWIDTH-1:0] pool6;
output reg [`DWIDTH-1:0] pool7;
output reg [`DWIDTH-1:0] pool8;
output reg [`DWIDTH-1:0] pool9;
output reg [`DWIDTH-1:0] pool10;
output reg [`DWIDTH-1:0] pool11;
output reg [`DWIDTH-1:0] pool12;
output reg [`DWIDTH-1:0] pool13;
output reg [`DWIDTH-1:0] pool14;
output reg [`DWIDTH-1:0] pool15;
input [`DWIDTH-1:0] rdata_accum0_pool;
input [`DWIDTH-1:0] rdata_accum1_pool;
input [`DWIDTH-1:0] rdata_accum2_pool;
input [`DWIDTH-1:0] rdata_accum3_pool;
input [`DWIDTH-1:0] rdata_accum4_pool;
input [`DWIDTH-1:0] rdata_accum5_pool;
input [`DWIDTH-1:0] rdata_accum6_pool;
input [`DWIDTH-1:0] rdata_accum7_pool;
input [`DWIDTH-1:0] rdata_accum8_pool;
input [`DWIDTH-1:0] rdata_accum9_pool;
input [`DWIDTH-1:0] rdata_accum10_pool;
input [`DWIDTH-1:0] rdata_accum11_pool;
input [`DWIDTH-1:0] rdata_accum12_pool;
input [`DWIDTH-1:0] rdata_accum13_pool;
input [`DWIDTH-1:0] rdata_accum14_pool;
input [`DWIDTH-1:0] rdata_accum15_pool;
output [`AWIDTH-1:0] raddr_accum0_pool;
output [`AWIDTH-1:0] raddr_accum1_pool;
output [`AWIDTH-1:0] raddr_accum2_pool;
output [`AWIDTH-1:0] raddr_accum3_pool;
output [`AWIDTH-1:0] raddr_accum4_pool;
output [`AWIDTH-1:0] raddr_accum5_pool;
output [`AWIDTH-1:0] raddr_accum6_pool;
output [`AWIDTH-1:0] raddr_accum7_pool;
output [`AWIDTH-1:0] raddr_accum8_pool;
output [`AWIDTH-1:0] raddr_accum9_pool;
output [`AWIDTH-1:0] raddr_accum10_pool;
output [`AWIDTH-1:0] raddr_accum11_pool;
output [`AWIDTH-1:0] raddr_accum12_pool;
output [`AWIDTH-1:0] raddr_accum13_pool;
output [`AWIDTH-1:0] raddr_accum14_pool;
output [`AWIDTH-1:0] raddr_accum15_pool;
input [`DWIDTH-1:0] matrix_size;
input [`DWIDTH-1:0] filter_size;

reg [`AWIDTH-1:0] raddr_accum1_pool;
reg [`AWIDTH-1:0] raddr_accum2_pool;
reg [`AWIDTH-1:0] raddr_accum3_pool;
reg [`AWIDTH-1:0] raddr_accum4_pool;
reg [`AWIDTH-1:0] raddr_accum5_pool;
reg [`AWIDTH-1:0] raddr_accum6_pool;
reg [`AWIDTH-1:0] raddr_accum7_pool;
reg [`AWIDTH-1:0] raddr_accum8_pool;
reg [`AWIDTH-1:0] raddr_accum9_pool;
reg [`AWIDTH-1:0] raddr_accum10_pool;
reg [`AWIDTH-1:0] raddr_accum11_pool;
reg [`AWIDTH-1:0] raddr_accum12_pool;
reg [`AWIDTH-1:0] raddr_accum13_pool;
reg [`AWIDTH-1:0] raddr_accum14_pool;
reg [`AWIDTH-1:0] raddr_accum15_pool;

reg [7:0] pool_count0;
reg [7:0] pool_count1;
reg [7:0] pool_count2;
reg [7:0] pool_count3;
reg [7:0] pool_count4;
reg [7:0] pool_count5;
reg [7:0] pool_count6;
reg [7:0] pool_count7;
reg [7:0] pool_count8;
reg [7:0] pool_count9;
reg [7:0] pool_count10;
reg [7:0] pool_count11;
reg [7:0] pool_count12;
reg [7:0] pool_count13;
reg [7:0] pool_count14;
reg [7:0] pool_count15;
reg [7:0] pool_count16;

wire [`DWIDTH-1:0] filter_size_int;
assign filter_size_int = (enable_pool)? filter_size : 8'b1;
wire [`DWIDTH-1:0] matrix_size_int;
assign matrix_size_int = (enable_pool)? matrix_size : 8'b1;

always @ (posedge clk) begin
    if (~resetn|~start_pooling) begin
        pool_count0 <= 0;
    end
    else if (pool_count0 == (filter_size_int*filter_size_int)) begin
        pool_count0 <= 1;
    end
    else if (start_pooling) begin
        pool_count0 <= pool_count0 + 1;
    end      
end

always @ (posedge clk) begin
    pool_count1 <= pool_count0;
    pool_count2 <= pool_count1;
    pool_count3 <= pool_count2;
    pool_count4 <= pool_count3;
    pool_count5 <= pool_count4;
    pool_count6 <= pool_count5;
    pool_count7 <= pool_count6;
    pool_count8 <= pool_count7;
    pool_count9 <= pool_count8;
    pool_count10 <= pool_count9;
    pool_count11 <= pool_count10;
    pool_count12 <= pool_count11;
    pool_count13 <= pool_count12;
    pool_count14 <= pool_count13;
    pool_count15 <= pool_count14;
    pool_count16 <= pool_count15;
end

wire [`DWIDTH-1:0] cmp0;
wire [`DWIDTH-1:0] cmp1;
wire [`DWIDTH-1:0] cmp2;
wire [`DWIDTH-1:0] cmp3;
wire [`DWIDTH-1:0] cmp4;
wire [`DWIDTH-1:0] cmp5;
wire [`DWIDTH-1:0] cmp6;
wire [`DWIDTH-1:0] cmp7;
wire [`DWIDTH-1:0] cmp8;
wire [`DWIDTH-1:0] cmp9;
wire [`DWIDTH-1:0] cmp10;
wire [`DWIDTH-1:0] cmp11;
wire [`DWIDTH-1:0] cmp12;
wire [`DWIDTH-1:0] cmp13;
wire [`DWIDTH-1:0] cmp14;
wire [`DWIDTH-1:0] cmp15;

reg [`DWIDTH-1:0] compare0;
reg [`DWIDTH-1:0] compare1;
reg [`DWIDTH-1:0] compare2;
reg [`DWIDTH-1:0] compare3;
reg [`DWIDTH-1:0] compare4;
reg [`DWIDTH-1:0] compare5;
reg [`DWIDTH-1:0] compare6;
reg [`DWIDTH-1:0] compare7;
reg [`DWIDTH-1:0] compare8;
reg [`DWIDTH-1:0] compare9;
reg [`DWIDTH-1:0] compare10;
reg [`DWIDTH-1:0] compare11;
reg [`DWIDTH-1:0] compare12;
reg [`DWIDTH-1:0] compare13;
reg [`DWIDTH-1:0] compare14;
reg [`DWIDTH-1:0] compare15;

wire [`DWIDTH+`MAT_MUL_SIZE-1:0] avg0;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] avg1;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] avg2;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] avg3;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] avg4;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] avg5;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] avg6;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] avg7;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] avg8;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] avg9;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] avg10;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] avg11;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] avg12;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] avg13;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] avg14;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] avg15;

reg [`DWIDTH+`MAT_MUL_SIZE-1:0] avg0_int;
reg [`DWIDTH+`MAT_MUL_SIZE-1:0] avg1_int;
reg [`DWIDTH+`MAT_MUL_SIZE-1:0] avg2_int;
reg [`DWIDTH+`MAT_MUL_SIZE-1:0] avg3_int;
reg [`DWIDTH+`MAT_MUL_SIZE-1:0] avg4_int;
reg [`DWIDTH+`MAT_MUL_SIZE-1:0] avg5_int;
reg [`DWIDTH+`MAT_MUL_SIZE-1:0] avg6_int;
reg [`DWIDTH+`MAT_MUL_SIZE-1:0] avg7_int;
reg [`DWIDTH+`MAT_MUL_SIZE-1:0] avg8_int;
reg [`DWIDTH+`MAT_MUL_SIZE-1:0] avg9_int;
reg [`DWIDTH+`MAT_MUL_SIZE-1:0] avg10_int;
reg [`DWIDTH+`MAT_MUL_SIZE-1:0] avg11_int;
reg [`DWIDTH+`MAT_MUL_SIZE-1:0] avg12_int;
reg [`DWIDTH+`MAT_MUL_SIZE-1:0] avg13_int;
reg [`DWIDTH+`MAT_MUL_SIZE-1:0] avg14_int;
reg [`DWIDTH+`MAT_MUL_SIZE-1:0] avg15_int;

wire [`DWIDTH+`MAT_MUL_SIZE-1:0] average0;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] average1;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] average2;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] average3;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] average4;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] average5;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] average6;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] average7;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] average8;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] average9;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] average10;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] average11;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] average12;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] average13;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] average14;
wire [`DWIDTH+`MAT_MUL_SIZE-1:0] average15;

assign pool_norm_valid = (pool_count1 == (filter_size_int*filter_size_int))?1'b1:1'b0;

reg [`AWIDTH-1:0] x;
reg [`AWIDTH-1:0] y;
reg [`AWIDTH-1:0] k;
assign raddr_accum0_pool = (~resetn|~start_pooling)? 11'h7ff: ((matrix_size_int)*y + x + k);

always @(posedge clk) begin
    if(~resetn|~start_pooling) begin
        x<=0;
        y<=0;
        k<=0;
    end
    else if (y == (matrix_size_int-1) & x==(filter_size_int-1)) begin
        k<=k+filter_size_int;
        y<=0;
        x<=0;
    end
    else if (x==(filter_size_int-1)) begin
        y<=y+1;
        x<=0;
    end
    else if (start_pooling) begin
        x<=x+1;
    end
end

always @ (posedge clk) begin
    raddr_accum1_pool <= raddr_accum0_pool;
    raddr_accum2_pool <= raddr_accum1_pool;
    raddr_accum3_pool <= raddr_accum2_pool;
    raddr_accum4_pool <= raddr_accum3_pool;
    raddr_accum5_pool <= raddr_accum4_pool;
    raddr_accum6_pool <= raddr_accum5_pool;
    raddr_accum7_pool <= raddr_accum6_pool;
    raddr_accum8_pool <= raddr_accum7_pool;
    raddr_accum9_pool <= raddr_accum8_pool;
    raddr_accum10_pool <= raddr_accum9_pool;
    raddr_accum11_pool <= raddr_accum10_pool;
    raddr_accum12_pool <= raddr_accum11_pool;
    raddr_accum13_pool <= raddr_accum12_pool;
    raddr_accum14_pool <= raddr_accum13_pool;
    raddr_accum15_pool <= raddr_accum14_pool;
end

always @ (posedge clk) begin
    if (~resetn) begin
        compare0 <= 0;
    end
    else if (rdata_accum0_pool > cmp0) begin
        compare0 <= rdata_accum0_pool;
    end
    else if (rdata_accum0_pool < cmp0) begin
        compare0 <= cmp0;
    end
end 

always @ (posedge clk) begin
    if (~resetn) begin
        avg0_int <= 0;
    end
    else begin
        avg0_int <= avg0 + rdata_accum0_pool;
    end
end

assign cmp0 = (pool_count0 == 1)? 0 : compare0;
assign avg0 = (pool_count0 == 1)? 0 : avg0_int;
assign average0 = (filter_size_int == 8'b1)? avg0_int : (filter_size_int == 8'b10)? avg0_int >> 2 : (filter_size_int == 8'b11)? avg0_int >> 3 : (filter_size_int == 8'b100)? avg0_int >> 4 : avg0_int;

wire [`DWIDTH-1:0] pool0_wire;
assign pool0_wire = (pool_count1 == (filter_size_int*filter_size_int))? ((pool_select == 0)? compare0 : average0) : 8'b0;
always @(posedge clk) begin
 pool0 <= pool0_wire;
end

always @ (posedge clk) begin
    if (~resetn) begin
        compare1 <= 0;
    end
    else if (rdata_accum1_pool > cmp1) begin
        compare1 <= rdata_accum1_pool;
    end
    else if (rdata_accum1_pool < cmp1) begin
        compare1 <= cmp1;
    end
end 

always @ (posedge clk) begin
    if (~resetn) begin
        avg1_int <= 0;
    end
    else begin
        avg1_int <= avg1 + rdata_accum1_pool;
    end
end

assign cmp1 = (pool_count1 == 1)? 0 : compare1;
assign avg1 = (pool_count1 == 1)? 0 : avg1_int;
assign average1 = (filter_size_int == 8'b1)? avg1_int : (filter_size_int == 8'b10)? avg1_int >> 2 : (filter_size_int == 8'b11)? avg1_int >> 3 : (filter_size_int == 8'b100)? avg1_int >> 4 : avg1_int;

wire [`DWIDTH-1:0] pool1_wire;
assign pool1_wire = (pool_count2 == (filter_size_int*filter_size_int))? ((pool_select == 0)? compare1 : average1) : 8'b0;
always @(posedge clk) begin
 pool1 <= pool1_wire;
end

always @ (posedge clk) begin
    if (~resetn) begin
        compare2 <= 0;
    end
    else if (rdata_accum2_pool > cmp2) begin
        compare2 <= rdata_accum2_pool;
    end
    else if (rdata_accum2_pool < cmp2) begin
        compare2 <= cmp2;
    end
end 

always @ (posedge clk) begin
    if (~resetn) begin
        avg2_int <= 0;
    end
    else begin
        avg2_int <= avg2 + rdata_accum2_pool;
    end
end

assign cmp2 = (pool_count2 == 1)? 0 : compare2;
assign avg2 = (pool_count2 == 1)? 0 : avg2_int;
assign average2 = (filter_size_int == 8'b1)? avg2_int : (filter_size_int == 8'b10)? avg2_int >> 2 : (filter_size_int == 8'b11)? avg2_int >> 3 : (filter_size_int == 8'b100)? avg2_int >> 4 : avg2_int;

wire [`DWIDTH-1:0] pool2_wire;
assign pool2_wire = (pool_count3 == (filter_size_int*filter_size_int))? ((pool_select == 0)? compare2 : average2) : 8'b0;
always @(posedge clk) begin
 pool2 <= pool2_wire;
end

always @ (posedge clk) begin
    if (~resetn) begin
        compare3 <= 0;
    end
    else if (rdata_accum3_pool > cmp3) begin
        compare3 <= rdata_accum3_pool;
    end
    else if (rdata_accum3_pool < cmp3) begin
        compare3 <= cmp3;
    end
end 

always @ (posedge clk) begin
    if (~resetn) begin
        avg3_int <= 0;
    end
    else begin
        avg3_int <= avg3 + rdata_accum3_pool;
    end
end

assign cmp3 = (pool_count3 == 1)? 0 : compare3;
assign avg3 = (pool_count3 == 1)? 0 : avg3_int;
assign average3 = (filter_size_int == 8'b1)? avg3_int : (filter_size_int == 8'b10)? avg3_int >> 2 : (filter_size_int == 8'b11)? avg3_int >> 3 : (filter_size_int == 8'b100)? avg3_int >> 4 : avg3_int;

wire [`DWIDTH-1:0] pool3_wire;
assign pool3_wire = (pool_count4 == (filter_size_int*filter_size_int))? ((pool_select == 0)? compare3 : average3) : 8'b0;
always @(posedge clk) begin
 pool3 <= pool3_wire;
end

always @ (posedge clk) begin
    if (~resetn) begin
        compare4 <= 0;
    end
    else if (rdata_accum4_pool > cmp4) begin
        compare4 <= rdata_accum4_pool;
    end
    else if (rdata_accum4_pool < cmp4) begin
        compare4 <= cmp4;
    end
end 

always @ (posedge clk) begin
    if (~resetn) begin
        avg4_int <= 0;
    end
    else begin
        avg4_int <= avg4 + rdata_accum4_pool;
    end
end

assign cmp4 = (pool_count4 == 1)? 0 : compare4;
assign avg4 = (pool_count4 == 1)? 0 : avg4_int;
assign average4 = (filter_size_int == 8'b1)? avg4_int : (filter_size_int == 8'b10)? avg4_int >> 2 : (filter_size_int == 8'b11)? avg4_int >> 3 : (filter_size_int == 8'b100)? avg4_int >> 4 : avg4_int;

wire [`DWIDTH-1:0] pool4_wire;
assign pool4_wire = (pool_count5 == (filter_size_int*filter_size_int))? ((pool_select == 0)? compare4 : average4) : 8'b0;
always @(posedge clk) begin
 pool4 <= pool4_wire;
end

always @ (posedge clk) begin
    if (~resetn) begin
        compare5 <= 0;
    end
    else if (rdata_accum5_pool > cmp5) begin
        compare5 <= rdata_accum5_pool;
    end
    else if (rdata_accum5_pool < cmp5) begin
        compare5 <= cmp5;
    end
end 

always @ (posedge clk) begin
    if (~resetn) begin
        avg5_int <= 0;
    end
    else begin
        avg5_int <= avg5 + rdata_accum5_pool;
    end
end

assign cmp5 = (pool_count5 == 1)? 0 : compare5;
assign avg5 = (pool_count5 == 1)? 0 : avg5_int;
assign average5 = (filter_size_int == 8'b1)? avg5_int : (filter_size_int == 8'b10)? avg5_int >> 2 : (filter_size_int == 8'b11)? avg5_int >> 3 : (filter_size_int == 8'b100)? avg5_int >> 4 : avg5_int;

wire [`DWIDTH-1:0] pool5_wire;
assign pool5_wire = (pool_count6 == (filter_size_int*filter_size_int))? ((pool_select == 0)? compare5 : average5) : 8'b0;
always @(posedge clk) begin
 pool5 <= pool5_wire;
end

always @ (posedge clk) begin
    if (~resetn) begin
        compare6 <= 0;
    end
    else if (rdata_accum6_pool > cmp6) begin
        compare6 <= rdata_accum6_pool;
    end
    else if (rdata_accum6_pool < cmp6) begin
        compare6 <= cmp6;
    end
end 

always @ (posedge clk) begin
    if (~resetn) begin
        avg6_int <= 0;
    end
    else begin
        avg6_int <= avg6 + rdata_accum6_pool;
    end
end

assign cmp6 = (pool_count6 == 1)? 0 : compare6;
assign avg6 = (pool_count6 == 1)? 0 : avg6_int;
assign average6 = (filter_size_int == 8'b1)? avg6_int : (filter_size_int == 8'b10)? avg6_int >> 2 : (filter_size_int == 8'b11)? avg6_int >> 3 : (filter_size_int == 8'b100)? avg6_int >> 4 : avg6_int;

wire [`DWIDTH-1:0] pool6_wire;
assign pool6_wire = (pool_count7 == (filter_size_int*filter_size_int))? ((pool_select == 0)? compare6 : average6) : 8'b0;
always @(posedge clk) begin
 pool6 <= pool6_wire;
end

always @ (posedge clk) begin
    if (~resetn) begin
        compare7 <= 0;
    end
    else if (rdata_accum7_pool > cmp7) begin
        compare7 <= rdata_accum7_pool;
    end
    else if (rdata_accum7_pool < cmp7) begin
        compare7 <= cmp7;
    end
end 

always @ (posedge clk) begin
    if (~resetn) begin
        avg7_int <= 0;
    end
    else begin
        avg7_int <= avg7 + rdata_accum7_pool;
    end
end

assign cmp7 = (pool_count7 == 1)? 0 : compare7;
assign avg7 = (pool_count7 == 1)? 0 : avg7_int;
assign average7 = (filter_size_int == 8'b1)? avg7_int : (filter_size_int == 8'b10)? avg7_int >> 2 : (filter_size_int == 8'b11)? avg7_int >> 3 : (filter_size_int == 8'b100)? avg7_int >> 4 : avg7_int;

wire [`DWIDTH-1:0] pool7_wire;
assign pool7_wire = (pool_count8 == (filter_size_int*filter_size_int))? ((pool_select == 0)? compare7 : average7) : 8'b0;
always @(posedge clk) begin
 pool7 <= pool7_wire;
end

always @ (posedge clk) begin
    if (~resetn) begin
        compare8 <= 0;
    end
    else if (rdata_accum8_pool > cmp8) begin
        compare8 <= rdata_accum8_pool;
    end
    else if (rdata_accum8_pool < cmp8) begin
        compare8 <= cmp8;
    end
end 

always @ (posedge clk) begin
    if (~resetn) begin
        avg8_int <= 0;
    end
    else begin
        avg8_int <= avg8 + rdata_accum8_pool;
    end
end

assign cmp8 = (pool_count8 == 1)? 0 : compare8;
assign avg8 = (pool_count8 == 1)? 0 : avg8_int;
assign average8 = (filter_size_int == 8'b1)? avg8_int : (filter_size_int == 8'b10)? avg8_int >> 2 : (filter_size_int == 8'b11)? avg8_int >> 3 : (filter_size_int == 8'b100)? avg8_int >> 4 : avg8_int;

wire [`DWIDTH-1:0] pool8_wire;
assign pool8_wire = (pool_count9 == (filter_size_int*filter_size_int))? ((pool_select == 0)? compare8 : average8) : 8'b0;
always @(posedge clk) begin
 pool8 <= pool8_wire;
end

always @ (posedge clk) begin
    if (~resetn) begin
        compare9 <= 0;
    end
    else if (rdata_accum9_pool > cmp9) begin
        compare9 <= rdata_accum9_pool;
    end
    else if (rdata_accum9_pool < cmp9) begin
        compare9 <= cmp9;
    end
end 

always @ (posedge clk) begin
    if (~resetn) begin
        avg9_int <= 0;
    end
    else begin
        avg9_int <= avg9 + rdata_accum9_pool;
    end
end

assign cmp9 = (pool_count9 == 1)? 0 : compare9;
assign avg9 = (pool_count9 == 1)? 0 : avg9_int;
assign average9 = (filter_size_int == 8'b1)? avg9_int : (filter_size_int == 8'b10)? avg9_int >> 2 : (filter_size_int == 8'b11)? avg9_int >> 3 : (filter_size_int == 8'b100)? avg9_int >> 4 : avg9_int;

wire [`DWIDTH-1:0] pool9_wire;
assign pool9_wire = (pool_count10 == (filter_size_int*filter_size_int))? ((pool_select == 0)? compare9 : average9) : 8'b0;
always @(posedge clk) begin
 pool9 <= pool9_wire;
end

always @ (posedge clk) begin
    if (~resetn) begin
        compare10 <= 0;
    end
    else if (rdata_accum10_pool > cmp10) begin
        compare10 <= rdata_accum10_pool;
    end
    else if (rdata_accum10_pool < cmp10) begin
        compare10 <= cmp10;
    end
end 

always @ (posedge clk) begin
    if (~resetn) begin
        avg10_int <= 0;
    end
    else begin
        avg10_int <= avg10 + rdata_accum10_pool;
    end
end

assign cmp10 = (pool_count10 == 1)? 0 : compare10;
assign avg10 = (pool_count10 == 1)? 0 : avg10_int;
assign average10 = (filter_size_int == 8'b1)? avg10_int : (filter_size_int == 8'b10)? avg10_int >> 2 : (filter_size_int == 8'b11)? avg10_int >> 3 : (filter_size_int == 8'b100)? avg10_int >> 4 : avg10_int;

wire [`DWIDTH-1:0] pool10_wire;
assign pool10_wire = (pool_count11 == (filter_size_int*filter_size_int))? ((pool_select == 0)? compare10 : average10) : 8'b0;
always @(posedge clk) begin
 pool10 <= pool10_wire;
end

always @ (posedge clk) begin
    if (~resetn) begin
        compare11 <= 0;
    end
    else if (rdata_accum11_pool > cmp11) begin
        compare11 <= rdata_accum11_pool;
    end
    else if (rdata_accum11_pool < cmp11) begin
        compare11 <= cmp11;
    end
end 

always @ (posedge clk) begin
    if (~resetn) begin
        avg11_int <= 0;
    end
    else begin
        avg11_int <= avg11 + rdata_accum11_pool;
    end
end

assign cmp11 = (pool_count11 == 1)? 0 : compare11;
assign avg11 = (pool_count11 == 1)? 0 : avg11_int;
assign average11 = (filter_size_int == 8'b1)? avg11_int : (filter_size_int == 8'b10)? avg11_int >> 2 : (filter_size_int == 8'b11)? avg11_int >> 3 : (filter_size_int == 8'b100)? avg11_int >> 4 : avg11_int;

wire [`DWIDTH-1:0] pool11_wire;
assign pool11_wire = (pool_count12 == (filter_size_int*filter_size_int))? ((pool_select == 0)? compare11 : average11) : 8'b0;
always @(posedge clk) begin
 pool11 <= pool11_wire;
end

always @ (posedge clk) begin
    if (~resetn) begin
        compare12 <= 0;
    end
    else if (rdata_accum12_pool > cmp12) begin
        compare12 <= rdata_accum12_pool;
    end
    else if (rdata_accum12_pool < cmp12) begin
        compare12 <= cmp12;
    end
end 

always @ (posedge clk) begin
    if (~resetn) begin
        avg12_int <= 0;
    end
    else begin
        avg12_int <= avg12 + rdata_accum12_pool;
    end
end

assign cmp12 = (pool_count12 == 1)? 0 : compare12;
assign avg12 = (pool_count12 == 1)? 0 : avg12_int;
assign average12 = (filter_size_int == 8'b1)? avg12_int : (filter_size_int == 8'b10)? avg12_int >> 2 : (filter_size_int == 8'b11)? avg12_int >> 3 : (filter_size_int == 8'b100)? avg12_int >> 4 : avg12_int;

wire [`DWIDTH-1:0] pool12_wire;
assign pool12_wire = (pool_count13 == (filter_size_int*filter_size_int))? ((pool_select == 0)? compare12 : average12) : 8'b0;
always @(posedge clk) begin
 pool12 <= pool12_wire;
end

always @ (posedge clk) begin
    if (~resetn) begin
        compare13 <= 0;
    end
    else if (rdata_accum13_pool > cmp13) begin
        compare13 <= rdata_accum13_pool;
    end
    else if (rdata_accum13_pool < cmp13) begin
        compare13 <= cmp13;
    end
end 

always @ (posedge clk) begin
    if (~resetn) begin
        avg13_int <= 0;
    end
    else begin
        avg13_int <= avg13 + rdata_accum13_pool;
    end
end

assign cmp13 = (pool_count13 == 1)? 0 : compare13;
assign avg13 = (pool_count13 == 1)? 0 : avg13_int;
assign average13 = (filter_size_int == 8'b1)? avg13_int : (filter_size_int == 8'b10)? avg13_int >> 2 : (filter_size_int == 8'b11)? avg13_int >> 3 : (filter_size_int == 8'b100)? avg13_int >> 4 : avg13_int;

wire [`DWIDTH-1:0] pool13_wire;
assign pool13_wire = (pool_count14 == (filter_size_int*filter_size_int))? ((pool_select == 0)? compare13 : average13) : 8'b0;
always @(posedge clk) begin
 pool13 <= pool13_wire;
end

always @ (posedge clk) begin
    if (~resetn) begin
        compare14 <= 0;
    end
    else if (rdata_accum14_pool > cmp14) begin
        compare14 <= rdata_accum14_pool;
    end
    else if (rdata_accum14_pool < cmp14) begin
        compare14 <= cmp14;
    end
end 

always @ (posedge clk) begin
    if (~resetn) begin
        avg14_int <= 0;
    end
    else begin
        avg14_int <= avg14 + rdata_accum14_pool;
    end
end

assign cmp14 = (pool_count14 == 1)? 0 : compare14;
assign avg14 = (pool_count14 == 1)? 0 : avg14_int;
assign average14 = (filter_size_int == 8'b1)? avg14_int : (filter_size_int == 8'b10)? avg14_int >> 2 : (filter_size_int == 8'b11)? avg14_int >> 3 : (filter_size_int == 8'b100)? avg14_int >> 4 : avg14_int;

wire [`DWIDTH-1:0] pool14_wire;
assign pool14_wire = (pool_count15 == (filter_size_int*filter_size_int))? ((pool_select == 0)? compare14 : average14) : 8'b0;
always @(posedge clk) begin
 pool14 <= pool14_wire;
end

always @ (posedge clk) begin
    if (~resetn) begin
        compare15 <= 0;
    end
    else if (rdata_accum15_pool > cmp15) begin
        compare15 <= rdata_accum15_pool;
    end
    else if (rdata_accum15_pool < cmp15) begin
        compare15 <= cmp15;
    end
end 

always @ (posedge clk) begin
    if (~resetn) begin
        avg15_int <= 0;
    end
    else begin
        avg15_int <= avg15 + rdata_accum15_pool;
    end
end

assign cmp15 = (pool_count15 == 1)? 0 : compare15;
assign avg15 = (pool_count15 == 1)? 0 : avg15_int;
assign average15 = (filter_size_int == 8'b1)? avg15_int : (filter_size_int == 8'b10)? avg15_int >> 2 : (filter_size_int == 8'b11)? avg15_int >> 3 : (filter_size_int == 8'b100)? avg15_int >> 4 : avg15_int;

wire [`DWIDTH-1:0] pool15_wire;
assign pool15_wire = (pool_count16 == (filter_size_int*filter_size_int))? ((pool_select == 0)? compare15 : average15) : 8'b0;
always @(posedge clk) begin
 pool15 <= pool15_wire;
end


endmodule

////////////////////////////////////////////////////////////////////////////////
// THIS FILE WAS AUTOMATICALLY GENERATED FROM generate_activation.v.mako
// DO NOT EDIT
////////////////////////////////////////////////////////////////////////////////
module activation(
    input activation_type,
    input enable_activation,
    input enable_pool,
    input in_data_available,
    input [`DWIDTH-1:0] inp_data0,
    input [`DWIDTH-1:0] inp_data1,
    input [`DWIDTH-1:0] inp_data2,
    input [`DWIDTH-1:0] inp_data3,
    input [`DWIDTH-1:0] inp_data4,
    input [`DWIDTH-1:0] inp_data5,
    input [`DWIDTH-1:0] inp_data6,
    input [`DWIDTH-1:0] inp_data7,
    input [`DWIDTH-1:0] inp_data8,
    input [`DWIDTH-1:0] inp_data9,
    input [`DWIDTH-1:0] inp_data10,
    input [`DWIDTH-1:0] inp_data11,
    input [`DWIDTH-1:0] inp_data12,
    input [`DWIDTH-1:0] inp_data13,
    input [`DWIDTH-1:0] inp_data14,
    input [`DWIDTH-1:0] inp_data15,
    output [`DWIDTH-1:0] out_data0,
    output [`DWIDTH-1:0] out_data1,
    output [`DWIDTH-1:0] out_data2,
    output [`DWIDTH-1:0] out_data3,
    output [`DWIDTH-1:0] out_data4,
    output [`DWIDTH-1:0] out_data5,
    output [`DWIDTH-1:0] out_data6,
    output [`DWIDTH-1:0] out_data7,
    output [`DWIDTH-1:0] out_data8,
    output [`DWIDTH-1:0] out_data9,
    output [`DWIDTH-1:0] out_data10,
    output [`DWIDTH-1:0] out_data11,
    output [`DWIDTH-1:0] out_data12,
    output [`DWIDTH-1:0] out_data13,
    output [`DWIDTH-1:0] out_data14,
    output [`DWIDTH-1:0] out_data15,
    output out_data_available,
    input [`MASK_WIDTH-1:0] validity_mask,
    output reg done_activation,
    input clk,
    input reset
);

reg in_data_available1;
reg in_data_available2;
reg in_data_available3;
reg in_data_available4;
reg in_data_available5;
reg in_data_available6;
reg in_data_available7;
reg in_data_available8;
reg in_data_available9;
reg in_data_available10;
reg in_data_available11;
reg in_data_available12;
reg in_data_available13;
reg in_data_available14;
reg in_data_available15;

always @(posedge clk) begin
    in_data_available1 <= in_data_available;
	in_data_available2 <= in_data_available1;
	in_data_available3 <= in_data_available2;
	in_data_available4 <= in_data_available3;
	in_data_available5 <= in_data_available4;
	in_data_available6 <= in_data_available5;
	in_data_available7 <= in_data_available6;
	in_data_available8 <= in_data_available7;
	in_data_available9 <= in_data_available8;
	in_data_available10 <= in_data_available9;
	in_data_available11 <= in_data_available10;
	in_data_available12 <= in_data_available11;
	in_data_available13 <= in_data_available12;
	in_data_available14 <= in_data_available13;
	in_data_available15 <= in_data_available14;
end

wire out_data_available_internal;
assign out_data_available   = enable_pool? enable_activation ? out_data_available_internal : in_data_available : in_data_available2;


wire out_data_available_final;
reg [`DWIDTH-1:0] act_count;
reg [`DWIDTH-1:0] done_activation_count;

always @(posedge clk) begin
	if (reset) begin
		done_activation <= 0;
      done_activation_count <= 0;
		act_count <= 0;
	end
   else if (done_activation_count == `MAT_MUL_SIZE)
      done_activation <= 0;
	else if (act_count == 4) begin
		done_activation <= 1;
      done_activation_count <= done_activation_count + 1;
	end
	else if (out_data_available_final == 1) begin
		act_count <= act_count + 1;
	end
end

sub_activation activation0(
    .activation_type(activation_type),
    .enable_activation(enable_activation),
    .in_data_available(in_data_available),
    .inp_data(inp_data0),
    .out_data(out_data0),
    .out_data_available(out_data_available_internal),
    .validity_mask(validity_mask[0]),
    .clk(clk),
    .reset(reset)
);

wire out_data_available_NC1;
sub_activation activation1(
    .activation_type(activation_type),
    .enable_activation(enable_activation),
    .in_data_available(in_data_available1),
    .inp_data(inp_data1),
    .out_data(out_data1),
    .out_data_available(out_data_available_NC1),
    .validity_mask(validity_mask[1]),
    .clk(clk),
    .reset(reset)
);    

wire out_data_available_NC2;
sub_activation activation2(
    .activation_type(activation_type),
    .enable_activation(enable_activation),
    .in_data_available(in_data_available2),
    .inp_data(inp_data2),
    .out_data(out_data2),
    .out_data_available(out_data_available_NC2),
    .validity_mask(validity_mask[2]),
    .clk(clk),
    .reset(reset)
);    

wire out_data_available_NC3;
sub_activation activation3(
    .activation_type(activation_type),
    .enable_activation(enable_activation),
    .in_data_available(in_data_available3),
    .inp_data(inp_data3),
    .out_data(out_data3),
    .out_data_available(out_data_available_NC3),
    .validity_mask(validity_mask[3]),
    .clk(clk),
    .reset(reset)
);    

wire out_data_available_NC4;
sub_activation activation4(
    .activation_type(activation_type),
    .enable_activation(enable_activation),
    .in_data_available(in_data_available4),
    .inp_data(inp_data4),
    .out_data(out_data4),
    .out_data_available(out_data_available_NC4),
    .validity_mask(validity_mask[4]),
    .clk(clk),
    .reset(reset)
);    

wire out_data_available_NC5;
sub_activation activation5(
    .activation_type(activation_type),
    .enable_activation(enable_activation),
    .in_data_available(in_data_available5),
    .inp_data(inp_data5),
    .out_data(out_data5),
    .out_data_available(out_data_available_NC5),
    .validity_mask(validity_mask[5]),
    .clk(clk),
    .reset(reset)
);    

wire out_data_available_NC6;
sub_activation activation6(
    .activation_type(activation_type),
    .enable_activation(enable_activation),
    .in_data_available(in_data_available6),
    .inp_data(inp_data6),
    .out_data(out_data6),
    .out_data_available(out_data_available_NC6),
    .validity_mask(validity_mask[6]),
    .clk(clk),
    .reset(reset)
);    

wire out_data_available_NC7;
sub_activation activation7(
    .activation_type(activation_type),
    .enable_activation(enable_activation),
    .in_data_available(in_data_available7),
    .inp_data(inp_data7),
    .out_data(out_data7),
    .out_data_available(out_data_available_NC7),
    .validity_mask(validity_mask[7]),
    .clk(clk),
    .reset(reset)
);    

wire out_data_available_NC8;
sub_activation activation8(
    .activation_type(activation_type),
    .enable_activation(enable_activation),
    .in_data_available(in_data_available8),
    .inp_data(inp_data8),
    .out_data(out_data8),
    .out_data_available(out_data_available_NC8),
    .validity_mask(validity_mask[8]),
    .clk(clk),
    .reset(reset)
);    

wire out_data_available_NC9;
sub_activation activation9(
    .activation_type(activation_type),
    .enable_activation(enable_activation),
    .in_data_available(in_data_available9),
    .inp_data(inp_data9),
    .out_data(out_data9),
    .out_data_available(out_data_available_NC9),
    .validity_mask(validity_mask[9]),
    .clk(clk),
    .reset(reset)
);    

wire out_data_available_NC10;
sub_activation activation10(
    .activation_type(activation_type),
    .enable_activation(enable_activation),
    .in_data_available(in_data_available10),
    .inp_data(inp_data10),
    .out_data(out_data10),
    .out_data_available(out_data_available_NC10),
    .validity_mask(validity_mask[10]),
    .clk(clk),
    .reset(reset)
);    

wire out_data_available_NC11;
sub_activation activation11(
    .activation_type(activation_type),
    .enable_activation(enable_activation),
    .in_data_available(in_data_available11),
    .inp_data(inp_data11),
    .out_data(out_data11),
    .out_data_available(out_data_available_NC11),
    .validity_mask(validity_mask[11]),
    .clk(clk),
    .reset(reset)
);    

wire out_data_available_NC12;
sub_activation activation12(
    .activation_type(activation_type),
    .enable_activation(enable_activation),
    .in_data_available(in_data_available12),
    .inp_data(inp_data12),
    .out_data(out_data12),
    .out_data_available(out_data_available_NC12),
    .validity_mask(validity_mask[12]),
    .clk(clk),
    .reset(reset)
);    

wire out_data_available_NC13;
sub_activation activation13(
    .activation_type(activation_type),
    .enable_activation(enable_activation),
    .in_data_available(in_data_available13),
    .inp_data(inp_data13),
    .out_data(out_data13),
    .out_data_available(out_data_available_NC13),
    .validity_mask(validity_mask[13]),
    .clk(clk),
    .reset(reset)
);    

wire out_data_available_NC14;
sub_activation activation14(
    .activation_type(activation_type),
    .enable_activation(enable_activation),
    .in_data_available(in_data_available14),
    .inp_data(inp_data14),
    .out_data(out_data14),
    .out_data_available(out_data_available_NC14),
    .validity_mask(validity_mask[14]),
    .clk(clk),
    .reset(reset)
);    

sub_activation activation15(
  .activation_type(activation_type),
  .enable_activation(enable_activation),
  .in_data_available(in_data_available15),
  .inp_data(inp_data15),
  .out_data(out_data15),
  .out_data_available(out_data_available_final),
  .validity_mask(validity_mask[15]),
  .clk(clk),
  .reset(reset)
);

endmodule

module sub_activation(
    input activation_type,
    input enable_activation,
    input in_data_available,
    input [`DWIDTH-1:0] inp_data,
    output [`DWIDTH-1:0] out_data,
    output out_data_available,
    input validity_mask,
    input clk,
    input reset
);

reg  out_data_available_internal;
reg [`DWIDTH-1:0] out_data_internal;
reg [`DWIDTH-1:0] slope_applied_data_internal;
reg [`DWIDTH-1:0] intercept_applied_data_internal;
reg [`DWIDTH-1:0] relu_applied_data_internal;

reg [31:0] cycle_count;
reg activation_in_progress;

reg [3:0] address;
reg [`DWIDTH-1:0] data_slope;
reg [`DWIDTH-1:0] data_intercept;
reg [`DWIDTH-1:0] data_intercept_delayed;

// If the activation block is not enabled, just forward the input data
assign out_data             = enable_activation ? out_data_internal : inp_data;
assign out_data_available   = enable_activation ? out_data_available_internal : in_data_available;

always @(posedge clk) begin
   if (reset || ~enable_activation) begin
      slope_applied_data_internal     <= 0;
      intercept_applied_data_internal <= 0; 
      relu_applied_data_internal      <= 0; 
      data_intercept_delayed      <= 0;
      out_data_available_internal <= 0;
      cycle_count                 <= 0;
      activation_in_progress      <= 0;      
   end 
   else if(in_data_available || activation_in_progress) begin
      cycle_count <= cycle_count + 1;
      if(activation_type==1'b1) begin // tanH
        slope_applied_data_internal <= data_slope * inp_data;
        data_intercept_delayed <= data_intercept;
        intercept_applied_data_internal <= slope_applied_data_internal + data_intercept_delayed;
      end else begin // ReLU
        relu_applied_data_internal <= (inp_data)? {`DWIDTH{1'b0}} : inp_data;
      end 
      
      //TANH needs 1 extra cycle
      if (activation_type==1'b1) begin
         if (cycle_count==2) begin
            out_data_available_internal <= 1;
         end
      end else begin
         if (cycle_count==1) begin
           out_data_available_internal <= 1;
         end
      end

      //TANH needs 1 extra cycle
      if (activation_type==1'b1) begin
        if(cycle_count==2) begin
           activation_in_progress <= 0;
        end
        else begin
           activation_in_progress <= 1;
        end
      end else begin
        if(cycle_count==1) begin
           activation_in_progress <= 0;
        end
        else begin
           activation_in_progress <= 1;
        end
      end
   end   
   else begin
      slope_applied_data_internal     <= 0;
      intercept_applied_data_internal <= 0; 
      relu_applied_data_internal      <= 0; 
      data_intercept_delayed      <= 0;
      out_data_available_internal <= 0;
      cycle_count                 <= 0;
      activation_in_progress      <= 0;
   end
end

always @ (posedge clk) begin
   if (activation_type == 1'b1)
      out_data_internal <= intercept_applied_data_internal;
   else
      out_data_internal <= relu_applied_data_internal;
end

//Our equation of tanh is Y=AX+B
//A is the slope and B is the intercept.
//We store A in one LUT and B in another.
//LUT for the slope
always @(address) begin
    case (address)
      4'b0000: data_slope = 8'd0;
      4'b0001: data_slope = 8'd0;
      4'b0010: data_slope = 8'd2;
      4'b0011: data_slope = 8'd3;
      4'b0100: data_slope = 8'd4;
      4'b0101: data_slope = 8'd0;
      4'b0110: data_slope = 8'd4;
      4'b0111: data_slope = 8'd3;
      4'b1000: data_slope = 8'd2;
      4'b1001: data_slope = 8'd0;
      4'b1010: data_slope = 8'd0;
      default: data_slope = 8'd0;
    endcase  
end

//LUT for the intercept
always @(address) begin
    case (address)
      4'b0000: data_intercept = 8'd127;
      4'b0001: data_intercept = 8'd99;
      4'b0010: data_intercept = 8'd46;
      4'b0011: data_intercept = 8'd18;
      4'b0100: data_intercept = 8'd0;
      4'b0101: data_intercept = 8'd0;
      4'b0110: data_intercept = 8'd0;
      4'b0111: data_intercept = -8'd18;
      4'b1000: data_intercept = -8'd46;
      4'b1001: data_intercept = -8'd99;
      4'b1010: data_intercept = -8'd127;
      default: data_intercept = 8'd0;
    endcase  
end

//Logic to find address
always @(inp_data) begin
        if((inp_data)>=90) begin
           address = 4'b0000;
        end
        else if ((inp_data)>=39 && (inp_data)<90) begin
           address = 4'b0001;
        end
        else if ((inp_data)>=28 && (inp_data)<39) begin
           address = 4'b0010;
        end
        else if ((inp_data)>=16 && (inp_data)<28) begin
           address = 4'b0011;
        end
        else if ((inp_data)>=1 && (inp_data)<16) begin
           address = 4'b0100;
        end
        else if ((inp_data)==0) begin
           address = 4'b0101;
        end
        else if ((inp_data)>-16 && (inp_data)<=-1) begin
           address = 4'b0110;
        end
        else if ((inp_data)>-28 && (inp_data)<=-16) begin
           address = 4'b0111;
        end
        else if ((inp_data)>-39 && (inp_data)<=-28) begin
           address = 4'b1000;
        end
        else if ((inp_data)>-90 && (inp_data)<=-39) begin
           address = 4'b1001;
        end
        else if ((inp_data)<=-90) begin
           address = 4'b1010;
        end
        else begin
           address = 4'b0101;
        end
end

//Adding a dummy signal to use validity_mask input, to make ODIN happy
//TODO: Need to correctly use validity_mask
wire [`MASK_WIDTH-1:0] dummy;
assign dummy = validity_mask;


endmodule

module top(
    input  clk,
    input  clk_mem,
    input  reset,
    input  resetn,
    input  [`REG_ADDRWIDTH-1:0] PADDR,
    input  PWRITE,
    input  PSEL,
    input  PENABLE,
    input  [`REG_DATAWIDTH-1:0] PWDATA,
    output [`REG_DATAWIDTH-1:0] PRDATA,
    output PREADY,
    input  [`AWIDTH-1:0] bram_addr_a_ext,
    output [`DESIGN_SIZE*`DWIDTH-1:0] bram_rdata_a_ext,
    input  [`DESIGN_SIZE*`DWIDTH-1:0] bram_wdata_a_ext,
    input  [`DESIGN_SIZE-1:0] bram_we_a_ext,
    input  [`AWIDTH-1:0] bram_addr_b_ext,
    output [`DESIGN_SIZE*`DWIDTH-1:0] bram_rdata_b_ext,
    input  [`DESIGN_SIZE*`DWIDTH-1:0] bram_wdata_b_ext,
    input  [`DESIGN_SIZE-1:0] bram_we_b_ext
);

wire [`AWIDTH-1:0] bram_addr_a;
wire [`AWIDTH-1:0] bram_addr_a_for_reading;
reg [`AWIDTH-1:0] bram_addr_a_for_writing;
wire [`DESIGN_SIZE*`DWIDTH-1:0] bram_rdata_a;
reg [`DESIGN_SIZE*`DWIDTH-1:0] bram_wdata_a;
wire [`DESIGN_SIZE-1:0] bram_we_a;
wire bram_en_a;
wire [`AWIDTH-1:0] bram_addr_b;
wire [`DESIGN_SIZE*`DWIDTH-1:0] bram_rdata_b;
wire [`DESIGN_SIZE*`DWIDTH-1:0] bram_wdata_b;
wire [`DESIGN_SIZE-1:0] bram_we_b;
wire bram_en_b;
reg bram_a_wdata_available;
wire [`AWIDTH-1:0] bram_addr_c_NC;
wire start_tpu;
wire done_tpu;
wire start_mat_mul;
wire done_mat_mul;
wire norm_out_data_available;
wire done_norm;
wire pool_out_data_available;
wire done_pool;
wire activation_out_data_available;
wire done_activation;
wire enable_matmul;
wire enable_norm;
wire enable_activation;
wire enable_pool;
wire [31:0] num_matrices_A;
wire [31:0] num_matrices_B;
wire [`DWIDTH-1:0] matrix_size;
wire [`DWIDTH-1:0] filter_size;
wire pool_select;
wire [`DWIDTH-1:0] k_dimension;
wire accum_select;
wire [`DESIGN_SIZE*`DWIDTH-1:0] matmul_c_data_out;
wire [`DESIGN_SIZE*`DWIDTH-1:0] pool_data_out;
wire [`DESIGN_SIZE*`DWIDTH-1:0] activation_data_out;
wire matmul_c_data_available;
wire [`DESIGN_SIZE*`DWIDTH-1:0] a_data_out_NC;
wire [`DESIGN_SIZE*`DWIDTH-1:0] b_data_out_NC;
wire [`DESIGN_SIZE*`DWIDTH-1:0] a_data_in_NC;
wire [`DESIGN_SIZE*`DWIDTH-1:0] b_data_in_NC;
wire [`DWIDTH-1:0] mean;
wire [`DWIDTH-1:0] inv_var;
wire [`AWIDTH-1:0] address_mat_a;
wire [`AWIDTH-1:0] address_mat_b;
wire [`AWIDTH-1:0] address_mat_c;
wire [`MASK_WIDTH-1:0] validity_mask_a_rows;
wire [`MASK_WIDTH-1:0] validity_mask_a_cols_b_rows;
wire [`MASK_WIDTH-1:0] validity_mask_b_cols;
wire save_output_to_accum;
wire add_accum_to_output;
wire [`ADDR_STRIDE_WIDTH-1:0] address_stride_a;
wire [`ADDR_STRIDE_WIDTH-1:0] address_stride_b;
wire [`ADDR_STRIDE_WIDTH-1:0] address_stride_c;
wire [`MAX_BITS_POOL-1:0] pool_window_size;
wire activation_type;
wire [3:0] conv_filter_height;
wire [3:0] conv_filter_width;
wire [3:0] conv_stride_horiz;
wire [3:0] conv_stride_verti;
wire [3:0] conv_padding_left;
wire [3:0] conv_padding_right;
wire [3:0] conv_padding_top;
wire [3:0] conv_padding_bottom;
wire [15:0] num_channels_inp;
wire [15:0] num_channels_out;
wire [15:0] inp_img_height;
wire [15:0] inp_img_width;
wire [15:0] out_img_height;
wire [15:0] out_img_width;
wire [31:0] batch_size;
wire enable_conv_mode;
wire pe_reset;
wire start_pool;
wire pool_norm_valid;

`ifdef DESIGN_SIZE_32
wire [`DWIDTH-1:0] matrixC31_0;
wire [`DWIDTH-1:0] matrixC31_1;
wire [`DWIDTH-1:0] matrixC31_2;
wire [`DWIDTH-1:0] matrixC31_3;
wire [`DWIDTH-1:0] matrixC31_4;
wire [`DWIDTH-1:0] matrixC31_5;
wire [`DWIDTH-1:0] matrixC31_6;
wire [`DWIDTH-1:0] matrixC31_7;
wire [`DWIDTH-1:0] matrixC31_8;
wire [`DWIDTH-1:0] matrixC31_9;
wire [`DWIDTH-1:0] matrixC31_10;
wire [`DWIDTH-1:0] matrixC31_11;
wire [`DWIDTH-1:0] matrixC31_12;
wire [`DWIDTH-1:0] matrixC31_13;
wire [`DWIDTH-1:0] matrixC31_14;
wire [`DWIDTH-1:0] matrixC31_15;
wire [`DWIDTH-1:0] matrixC31_16;
wire [`DWIDTH-1:0] matrixC31_17;
wire [`DWIDTH-1:0] matrixC31_18;
wire [`DWIDTH-1:0] matrixC31_19;
wire [`DWIDTH-1:0] matrixC31_20;
wire [`DWIDTH-1:0] matrixC31_21;
wire [`DWIDTH-1:0] matrixC31_22;
wire [`DWIDTH-1:0] matrixC31_23;
wire [`DWIDTH-1:0] matrixC31_24;
wire [`DWIDTH-1:0] matrixC31_25;
wire [`DWIDTH-1:0] matrixC31_26;
wire [`DWIDTH-1:0] matrixC31_27;
wire [`DWIDTH-1:0] matrixC31_28;
wire [`DWIDTH-1:0] matrixC31_29;
wire [`DWIDTH-1:0] matrixC31_30;
wire [`DWIDTH-1:0] matrixC31_31;
`endif
`ifdef DESIGN_SIZE_16
wire [`DWIDTH-1:0] matrixC15_0;
wire [`DWIDTH-1:0] matrixC15_1;
wire [`DWIDTH-1:0] matrixC15_2;
wire [`DWIDTH-1:0] matrixC15_3;
wire [`DWIDTH-1:0] matrixC15_4;
wire [`DWIDTH-1:0] matrixC15_5;
wire [`DWIDTH-1:0] matrixC15_6;
wire [`DWIDTH-1:0] matrixC15_7;
wire [`DWIDTH-1:0] matrixC15_8;
wire [`DWIDTH-1:0] matrixC15_9;
wire [`DWIDTH-1:0] matrixC15_10;
wire [`DWIDTH-1:0] matrixC15_11;
wire [`DWIDTH-1:0] matrixC15_12;
wire [`DWIDTH-1:0] matrixC15_13;
wire [`DWIDTH-1:0] matrixC15_14;
wire [`DWIDTH-1:0] matrixC15_15;
`endif
`ifdef DESIGN_SIZE_8
wire [`DWIDTH-1:0] matrixC7_0;
wire [`DWIDTH-1:0] matrixC7_1;
wire [`DWIDTH-1:0] matrixC7_2;
wire [`DWIDTH-1:0] matrixC7_3;
wire [`DWIDTH-1:0] matrixC7_4;
wire [`DWIDTH-1:0] matrixC7_5;
wire [`DWIDTH-1:0] matrixC7_6;
wire [`DWIDTH-1:0] matrixC7_7;
`endif
`ifdef DESIGN_SIZE_4
wire [`DWIDTH-1:0] matrixC3_0;
wire [`DWIDTH-1:0] matrixC3_1;
wire [`DWIDTH-1:0] matrixC3_2;
wire [`DWIDTH-1:0] matrixC3_3;
`endif

wire [`AWIDTH-1:0] start_waddr_accum0;

assign start_waddr_accum0 = 11'b0;

`ifdef DESIGN_SIZE_8
wire [`DWIDTH-1:0] rdata_accum0_pool;
wire [`DWIDTH-1:0] rdata_accum1_pool;
wire [`DWIDTH-1:0] rdata_accum2_pool;
wire [`DWIDTH-1:0] rdata_accum3_pool;
wire [`DWIDTH-1:0] rdata_accum4_pool;
wire [`DWIDTH-1:0] rdata_accum5_pool;
wire [`DWIDTH-1:0] rdata_accum6_pool;
wire [`DWIDTH-1:0] rdata_accum7_pool;
wire [`AWIDTH-1:0] raddr_accum0_pool;
wire [`AWIDTH-1:0] raddr_accum1_pool;
wire [`AWIDTH-1:0] raddr_accum2_pool;
wire [`AWIDTH-1:0] raddr_accum3_pool;
wire [`AWIDTH-1:0] raddr_accum4_pool;
wire [`AWIDTH-1:0] raddr_accum5_pool;
wire [`AWIDTH-1:0] raddr_accum6_pool;
wire [`AWIDTH-1:0] raddr_accum7_pool;
`endif

`ifdef DESIGN_SIZE_16
wire [`DWIDTH-1:0] rdata_accum0_pool;
wire [`DWIDTH-1:0] rdata_accum1_pool;
wire [`DWIDTH-1:0] rdata_accum2_pool;
wire [`DWIDTH-1:0] rdata_accum3_pool;
wire [`DWIDTH-1:0] rdata_accum4_pool;
wire [`DWIDTH-1:0] rdata_accum5_pool;
wire [`DWIDTH-1:0] rdata_accum6_pool;
wire [`DWIDTH-1:0] rdata_accum7_pool;
wire [`DWIDTH-1:0] rdata_accum8_pool;
wire [`DWIDTH-1:0] rdata_accum9_pool;
wire [`DWIDTH-1:0] rdata_accum10_pool;
wire [`DWIDTH-1:0] rdata_accum11_pool;
wire [`DWIDTH-1:0] rdata_accum12_pool;
wire [`DWIDTH-1:0] rdata_accum13_pool;
wire [`DWIDTH-1:0] rdata_accum14_pool;
wire [`DWIDTH-1:0] rdata_accum15_pool;
wire [`AWIDTH-1:0] raddr_accum0_pool;
wire [`AWIDTH-1:0] raddr_accum1_pool;
wire [`AWIDTH-1:0] raddr_accum2_pool;
wire [`AWIDTH-1:0] raddr_accum3_pool;
wire [`AWIDTH-1:0] raddr_accum4_pool;
wire [`AWIDTH-1:0] raddr_accum5_pool;
wire [`AWIDTH-1:0] raddr_accum6_pool;
wire [`AWIDTH-1:0] raddr_accum7_pool;
wire [`AWIDTH-1:0] raddr_accum8_pool;
wire [`AWIDTH-1:0] raddr_accum9_pool;
wire [`AWIDTH-1:0] raddr_accum10_pool;
wire [`AWIDTH-1:0] raddr_accum11_pool;
wire [`AWIDTH-1:0] raddr_accum12_pool;
wire [`AWIDTH-1:0] raddr_accum13_pool;
wire [`AWIDTH-1:0] raddr_accum14_pool;
wire [`AWIDTH-1:0] raddr_accum15_pool;
`endif

`ifdef DESIGN_SIZE_32
wire [`DWIDTH-1:0] rdata_accum0_pool;
wire [`DWIDTH-1:0] rdata_accum1_pool;
wire [`DWIDTH-1:0] rdata_accum2_pool;
wire [`DWIDTH-1:0] rdata_accum3_pool;
wire [`DWIDTH-1:0] rdata_accum4_pool;
wire [`DWIDTH-1:0] rdata_accum5_pool;
wire [`DWIDTH-1:0] rdata_accum6_pool;
wire [`DWIDTH-1:0] rdata_accum7_pool;
wire [`DWIDTH-1:0] rdata_accum8_pool;
wire [`DWIDTH-1:0] rdata_accum9_pool;
wire [`DWIDTH-1:0] rdata_accum10_pool;
wire [`DWIDTH-1:0] rdata_accum11_pool;
wire [`DWIDTH-1:0] rdata_accum12_pool;
wire [`DWIDTH-1:0] rdata_accum13_pool;
wire [`DWIDTH-1:0] rdata_accum14_pool;
wire [`DWIDTH-1:0] rdata_accum15_pool;
wire [`DWIDTH-1:0] rdata_accum16_pool;
wire [`DWIDTH-1:0] rdata_accum17_pool;
wire [`DWIDTH-1:0] rdata_accum18_pool;
wire [`DWIDTH-1:0] rdata_accum19_pool;
wire [`DWIDTH-1:0] rdata_accum20_pool;
wire [`DWIDTH-1:0] rdata_accum21_pool;
wire [`DWIDTH-1:0] rdata_accum22_pool;
wire [`DWIDTH-1:0] rdata_accum23_pool;
wire [`DWIDTH-1:0] rdata_accum24_pool;
wire [`DWIDTH-1:0] rdata_accum25_pool;
wire [`DWIDTH-1:0] rdata_accum26_pool;
wire [`DWIDTH-1:0] rdata_accum27_pool;
wire [`DWIDTH-1:0] rdata_accum28_pool;
wire [`DWIDTH-1:0] rdata_accum29_pool;
wire [`DWIDTH-1:0] rdata_accum30_pool;
wire [`DWIDTH-1:0] rdata_accum31_pool;
wire [`AWIDTH-1:0] raddr_accum0_pool;
wire [`AWIDTH-1:0] raddr_accum1_pool;
wire [`AWIDTH-1:0] raddr_accum2_pool;
wire [`AWIDTH-1:0] raddr_accum3_pool;
wire [`AWIDTH-1:0] raddr_accum4_pool;
wire [`AWIDTH-1:0] raddr_accum5_pool;
wire [`AWIDTH-1:0] raddr_accum6_pool;
wire [`AWIDTH-1:0] raddr_accum7_pool;
wire [`AWIDTH-1:0] raddr_accum8_pool;
wire [`AWIDTH-1:0] raddr_accum9_pool;
wire [`AWIDTH-1:0] raddr_accum10_pool;
wire [`AWIDTH-1:0] raddr_accum11_pool;
wire [`AWIDTH-1:0] raddr_accum12_pool;
wire [`AWIDTH-1:0] raddr_accum13_pool;
wire [`AWIDTH-1:0] raddr_accum14_pool;
wire [`AWIDTH-1:0] raddr_accum15_pool;
wire [`AWIDTH-1:0] raddr_accum16_pool;
wire [`AWIDTH-1:0] raddr_accum17_pool;
wire [`AWIDTH-1:0] raddr_accum18_pool;
wire [`AWIDTH-1:0] raddr_accum19_pool;
wire [`AWIDTH-1:0] raddr_accum20_pool;
wire [`AWIDTH-1:0] raddr_accum21_pool;
wire [`AWIDTH-1:0] raddr_accum22_pool;
wire [`AWIDTH-1:0] raddr_accum23_pool;
wire [`AWIDTH-1:0] raddr_accum24_pool;
wire [`AWIDTH-1:0] raddr_accum25_pool;
wire [`AWIDTH-1:0] raddr_accum26_pool;
wire [`AWIDTH-1:0] raddr_accum27_pool;
wire [`AWIDTH-1:0] raddr_accum28_pool;
wire [`AWIDTH-1:0] raddr_accum29_pool;
wire [`AWIDTH-1:0] raddr_accum30_pool;
wire [`AWIDTH-1:0] raddr_accum31_pool;
`endif

//Connections for bram a (activation/input matrix)
//bram_addr_a -> connected to u_matmul_4x4
//bram_rdata_a -> connected to u_matmul_4x4
//bram_wdata_a -> will come from the last block that is enabled
//bram_we_a -> will be 1 when the last block's data is available
//bram_en_a -> hardcoded to 1
assign bram_addr_a = (bram_a_wdata_available) ? bram_addr_a_for_writing : bram_addr_a_for_reading;
assign bram_en_a = 1'b1;
assign bram_we_a = (bram_a_wdata_available) ? {`DESIGN_SIZE{1'b1}} : {`DESIGN_SIZE{1'b0}};  
  
//Connections for bram b (weights matrix)
//bram_addr_b -> connected to u_matmul_4x4
//bram_rdata_b -> connected to u_matmul_4x4
//bram_wdata_b -> hardcoded to 0 (this block only reads from bram b)
//bram_we_b -> hardcoded to 0 (this block only reads from bram b)
//bram_en_b -> hardcoded to 1
assign bram_wdata_b = {`DESIGN_SIZE*`DWIDTH{1'b0}};
assign bram_en_b = 1'b1;
assign bram_we_b = {`DESIGN_SIZE{1'b0}};
  
////////////////////////////////////////////////////////////////
// BRAM matrix A (inputs/activations)
////////////////////////////////////////////////////////////////
ram #(.AW(`AWIDTH), .MW(`MASK_WIDTH), .DW(`DWIDTH)) matrix_A (
  .addr0(bram_addr_a),
  .d0(bram_wdata_a), 
  .we0(bram_we_a), 
  .q0(bram_rdata_a), 
  .addr1(bram_addr_a_ext),
  .d1(bram_wdata_a_ext), 
  .we1(bram_we_a_ext), 
  .q1(bram_rdata_a_ext), 
  .clk(clk_mem));

////////////////////////////////////////////////////////////////
// BRAM matrix B (weights)
////////////////////////////////////////////////////////////////
ram #(.AW(`AWIDTH), .MW(`MASK_WIDTH), .DW(`DWIDTH)) matrix_B (
  .addr0(bram_addr_b),
  .d0(bram_wdata_b), 
  .we0(bram_we_b), 
  .q0(bram_rdata_b), 
  .addr1(bram_addr_b_ext),
  .d1(bram_wdata_b_ext), 
  .we1(bram_we_b_ext), 
  .q1(bram_rdata_b_ext), 
  .clk(clk_mem));

////////////////////////////////////////////////////////////////
// Control logic that directs all the operation
////////////////////////////////////////////////////////////////
control u_control(
  .clk(clk),
  .reset(reset),
  .start_tpu(start_tpu),
  .enable_matmul(enable_matmul),
  .enable_norm(enable_norm),
  .enable_activation(enable_activation),
  .enable_pool(enable_pool),
  .start_mat_mul(start_mat_mul),
  .done_mat_mul(done_mat_mul),
  .done_norm(done_norm),
  .done_pool(done_pool), 
  .done_activation(done_activation),
  .save_output_to_accum(save_output_to_accum),
  .done_tpu(done_tpu)
);

////////////////////////////////////////////////////////////////
// Configuration (register) block
////////////////////////////////////////////////////////////////
cfg u_cfg(
  .PCLK(clk),
  .PRESETn(resetn),
  .PADDR(PADDR),
  .PWRITE(PWRITE),
  .PSEL(PSEL),
  .PENABLE(PENABLE),
  .PWDATA(PWDATA),
  .PRDATA(PRDATA),
  .PREADY(PREADY),
  .start_tpu(start_tpu),
  .enable_matmul(enable_matmul),
  .enable_norm(enable_norm),
  .enable_pool(enable_pool),
  .enable_activation(enable_activation),
  .enable_conv_mode(enable_conv_mode),
  .mean(mean),
  .inv_var(inv_var),
  .pool_window_size(pool_window_size),
  .address_mat_a(address_mat_a),
  .address_mat_b(address_mat_b),
  .address_mat_c(address_mat_c),
  .num_matrices_A(num_matrices_A),
  .num_matrices_B(num_matrices_B),
  .matrix_size(matrix_size),
  .filter_size(filter_size),
  .pool_select(pool_select),
  .k_dimension(k_dimension), // Dimension of A = m x k, Dimension of B = k x n
  .accum_select(accum_select),
  .validity_mask_a_rows(validity_mask_a_rows),
  .validity_mask_a_cols_b_rows(validity_mask_a_cols_b_rows),
  .validity_mask_b_cols(validity_mask_b_cols),
  .save_output_to_accum(save_output_to_accum),
  .add_accum_to_output(add_accum_to_output),
  .address_stride_a(address_stride_a),
  .address_stride_b(address_stride_b),
  .address_stride_c(address_stride_c),
  .activation_type(activation_type),
  .conv_filter_height(conv_filter_height),
  .conv_filter_width(conv_filter_width),
  .conv_stride_horiz(conv_stride_horiz),
  .conv_stride_verti(conv_stride_verti),
  .conv_padding_left(conv_padding_left),
  .conv_padding_right(conv_padding_right),
  .conv_padding_top(conv_padding_top),
  .conv_padding_bottom(conv_padding_bottom),
  .num_channels_inp(num_channels_inp),
  .num_channels_out(num_channels_out),
  .inp_img_height(inp_img_height),
  .inp_img_width(inp_img_width),
  .out_img_height(out_img_height),
  .out_img_width(out_img_width),
  .batch_size(batch_size),
  .pe_reset(pe_reset),
  .done_tpu(done_tpu)
);

//TODO: We want to move the data setup part
//and the interface to BRAM_A and BRAM_B outside
//into its own modules. For now, it is all inside
//the matmul block

////////////////////////////////////////////////////////////////
//Matrix multiplier
//Note: the ports on this module to write data to bram c
//are not used in this top module. 
////////////////////////////////////////////////////////////////
`ifdef DESIGN_SIZE_32
matmul_32x32_systolic u_matmul(
`endif
`ifdef DESIGN_SIZE_16
matmul_16x16_systolic u_matmul(
`endif
`ifdef DESIGN_SIZE_8
matmul_8x8_systolic u_matmul(
`endif
`ifdef DESIGN_SIZE_4
matmul_4x4_systolic u_matmul(
`endif
  .clk(clk),
  .reset(reset),
  .pe_reset(pe_reset),
  .start_mat_mul(start_mat_mul),
  .done_mat_mul(done_mat_mul),
  .num_matrices_A(num_matrices_A),
  .num_matrices_B(num_matrices_B),
  .address_mat_a(address_mat_a),
  .address_mat_b(address_mat_b),
  .address_stride_a(address_stride_a),
  .address_stride_b(address_stride_b),
  .a_data(bram_rdata_a),
  .b_data(bram_rdata_b),
  .a_data_in(a_data_in_NC),
  .b_data_in(b_data_in_NC),
  .c_data_in({`DESIGN_SIZE*`DWIDTH{1'b0}}),
  .c_data_out(matmul_c_data_out),
  .a_data_out(a_data_out_NC),
  .b_data_out(b_data_out_NC),
  .a_addr(bram_addr_a_for_reading),
  .b_addr(bram_addr_b),
  .c_addr(bram_addr_c_NC),
  .c_data_available(matmul_c_data_available),
  `ifdef DESIGN_SIZE_32
  .matrixC31_0(matrixC31_0),
  .matrixC31_1(matrixC31_1),
  .matrixC31_2(matrixC31_2),
  .matrixC31_3(matrixC31_3),
  .matrixC31_4(matrixC31_4),
  .matrixC31_5(matrixC31_5),
  .matrixC31_6(matrixC31_6),
  .matrixC31_7(matrixC31_7),
  .matrixC31_8(matrixC31_8),
  .matrixC31_9(matrixC31_9),
  .matrixC31_10(matrixC31_10),
  .matrixC31_11(matrixC31_11),
  .matrixC31_12(matrixC31_12),
  .matrixC31_13(matrixC31_13),
  .matrixC31_14(matrixC31_14),
  .matrixC31_15(matrixC31_15),
  .matrixC31_16(matrixC31_16),
  .matrixC31_17(matrixC31_17),
  .matrixC31_18(matrixC31_18),
  .matrixC31_19(matrixC31_19),
  .matrixC31_20(matrixC31_20),
  .matrixC31_21(matrixC31_21),
  .matrixC31_22(matrixC31_22),
  .matrixC31_23(matrixC31_23),
  .matrixC31_24(matrixC31_24),
  .matrixC31_25(matrixC31_25),
  .matrixC31_26(matrixC31_26),
  .matrixC31_27(matrixC31_27),
  .matrixC31_28(matrixC31_28),
  .matrixC31_29(matrixC31_29),
  .matrixC31_30(matrixC31_30),
  .matrixC31_31(matrixC31_31),
  `endif
  `ifdef DESIGN_SIZE_16
  .matrixC15_0(matrixC15_0),
  .matrixC15_1(matrixC15_1),
  .matrixC15_2(matrixC15_2),
  .matrixC15_3(matrixC15_3),
  .matrixC15_4(matrixC15_4),
  .matrixC15_5(matrixC15_5),
  .matrixC15_6(matrixC15_6),
  .matrixC15_7(matrixC15_7),
  .matrixC15_8(matrixC15_8),
  .matrixC15_9(matrixC15_9),
  .matrixC15_10(matrixC15_10),
  .matrixC15_11(matrixC15_11),
  .matrixC15_12(matrixC15_12),
  .matrixC15_13(matrixC15_13),
  .matrixC15_14(matrixC15_14),
  .matrixC15_15(matrixC15_15),
  `endif
  `ifdef DESIGN_SIZE_8
  .matrixC7_0(matrixC7_0),
  .matrixC7_1(matrixC7_1),
  .matrixC7_2(matrixC7_2),
  .matrixC7_3(matrixC7_3),
  .matrixC7_4(matrixC7_4),
  .matrixC7_5(matrixC7_5),
  .matrixC7_6(matrixC7_6),
  .matrixC7_7(matrixC7_7),
  `endif
  `ifdef DESIGN_SIZE_4
  .matrixC3_0(matrixC3_0),
  .matrixC3_1(matrixC3_1),
  .matrixC3_2(matrixC3_2),
  .matrixC3_3(matrixC3_3),
  `endif
  .validity_mask_a_rows(validity_mask_a_rows),
  .validity_mask_a_cols_b_rows(validity_mask_a_cols_b_rows),
  .validity_mask_b_cols(validity_mask_b_cols),
  .a_loc(32'd0),
  .b_loc(32'd0)
);

////////////////////////////////////////////////////////////////
// Accumulator module
////////////////////////////////////////////////////////////////
accumulator u_accum (
  .clk(clk),
  .resetn(resetn),
  .k_dimension(k_dimension), // Dimension of A = m x k, Dimension of B = k x n
  .buffer_select(accum_select),
  .start_pooling(start_pool),  
  .done_pooling(done_pool),
  .wdata_available(matmul_c_data_available),
  .start_waddr_accum0(start_waddr_accum0),
  `ifdef DESIGN_SIZE_8
  .wdata_accum0(matrixC7_0),
  .wdata_accum1(matrixC7_1),
  .wdata_accum2(matrixC7_2),
  .wdata_accum3(matrixC7_3),
  .wdata_accum4(matrixC7_4),
  .wdata_accum5(matrixC7_5),
  .wdata_accum6(matrixC7_6),
  .wdata_accum7(matrixC7_7),
  .raddr_accum0_pool(raddr_accum0_pool),
  .raddr_accum1_pool(raddr_accum1_pool),
  .raddr_accum2_pool(raddr_accum2_pool),
  .raddr_accum3_pool(raddr_accum3_pool),
  .raddr_accum4_pool(raddr_accum4_pool),
  .raddr_accum5_pool(raddr_accum5_pool),
  .raddr_accum6_pool(raddr_accum6_pool),
  .raddr_accum7_pool(raddr_accum7_pool),
  .rdata_accum0_pool(rdata_accum0_pool),
  .rdata_accum1_pool(rdata_accum1_pool),
  .rdata_accum2_pool(rdata_accum2_pool),
  .rdata_accum3_pool(rdata_accum3_pool),
  .rdata_accum4_pool(rdata_accum4_pool),
  .rdata_accum5_pool(rdata_accum5_pool),
  .rdata_accum6_pool(rdata_accum6_pool),
  .rdata_accum7_pool(rdata_accum7_pool)
  `endif
  `ifdef DESIGN_SIZE_16
  .wdata_accum0(matrixC15_0),
  .wdata_accum1(matrixC15_1),
  .wdata_accum2(matrixC15_2),
  .wdata_accum3(matrixC15_3),
  .wdata_accum4(matrixC15_4),
  .wdata_accum5(matrixC15_5),
  .wdata_accum6(matrixC15_6),
  .wdata_accum7(matrixC15_7),
  .wdata_accum8(matrixC15_8),
  .wdata_accum9(matrixC15_9),
  .wdata_accum10(matrixC15_10),
  .wdata_accum11(matrixC15_11),
  .wdata_accum12(matrixC15_12),
  .wdata_accum13(matrixC15_13),
  .wdata_accum14(matrixC15_14),
  .wdata_accum15(matrixC15_15),
  .raddr_accum0_pool(raddr_accum0_pool),
  .raddr_accum1_pool(raddr_accum1_pool),
  .raddr_accum2_pool(raddr_accum2_pool),
  .raddr_accum3_pool(raddr_accum3_pool),
  .raddr_accum4_pool(raddr_accum4_pool),
  .raddr_accum5_pool(raddr_accum5_pool),
  .raddr_accum6_pool(raddr_accum6_pool),
  .raddr_accum7_pool(raddr_accum7_pool),
  .raddr_accum8_pool(raddr_accum8_pool),
  .raddr_accum9_pool(raddr_accum9_pool),
  .raddr_accum10_pool(raddr_accum10_pool),
  .raddr_accum11_pool(raddr_accum11_pool),
  .raddr_accum12_pool(raddr_accum12_pool),
  .raddr_accum13_pool(raddr_accum13_pool),
  .raddr_accum14_pool(raddr_accum14_pool),
  .raddr_accum15_pool(raddr_accum15_pool),
  .rdata_accum0_pool(rdata_accum0_pool),
  .rdata_accum1_pool(rdata_accum1_pool),
  .rdata_accum2_pool(rdata_accum2_pool),
  .rdata_accum3_pool(rdata_accum3_pool),
  .rdata_accum4_pool(rdata_accum4_pool),
  .rdata_accum5_pool(rdata_accum5_pool),
  .rdata_accum6_pool(rdata_accum6_pool),
  .rdata_accum7_pool(rdata_accum7_pool),
  .rdata_accum8_pool(rdata_accum8_pool),
  .rdata_accum9_pool(rdata_accum9_pool),
  .rdata_accum10_pool(rdata_accum10_pool),
  .rdata_accum11_pool(rdata_accum11_pool),
  .rdata_accum12_pool(rdata_accum12_pool),
  .rdata_accum13_pool(rdata_accum13_pool),
  .rdata_accum14_pool(rdata_accum14_pool),
  .rdata_accum15_pool(rdata_accum15_pool)
  `endif
  `ifdef DESIGN_SIZE_32
  .wdata_accum0(matrixC31_0),
  .wdata_accum1(matrixC31_1),
  .wdata_accum2(matrixC31_2),
  .wdata_accum3(matrixC31_3),
  .wdata_accum4(matrixC31_4),
  .wdata_accum5(matrixC31_5),
  .wdata_accum6(matrixC31_6),
  .wdata_accum7(matrixC31_7),
  .wdata_accum8(matrixC31_8),
  .wdata_accum9(matrixC31_9),
  .wdata_accum10(matrixC31_10),
  .wdata_accum11(matrixC31_11),
  .wdata_accum12(matrixC31_12),
  .wdata_accum13(matrixC31_13),
  .wdata_accum14(matrixC31_14),
  .wdata_accum15(matrixC31_15),
  .wdata_accum16(matrixC31_16),
  .wdata_accum17(matrixC31_17),
  .wdata_accum18(matrixC31_18),
  .wdata_accum19(matrixC31_19),
  .wdata_accum20(matrixC31_20),
  .wdata_accum21(matrixC31_21),
  .wdata_accum22(matrixC31_22),
  .wdata_accum23(matrixC31_23),
  .wdata_accum24(matrixC31_24),
  .wdata_accum25(matrixC31_25),
  .wdata_accum26(matrixC31_26),
  .wdata_accum27(matrixC31_27),
  .wdata_accum28(matrixC31_28),
  .wdata_accum29(matrixC31_29),
  .wdata_accum30(matrixC31_30),
  .wdata_accum31(matrixC31_31),
  .raddr_accum0_pool(raddr_accum0_pool),
  .raddr_accum1_pool(raddr_accum1_pool),
  .raddr_accum2_pool(raddr_accum2_pool),
  .raddr_accum3_pool(raddr_accum3_pool),
  .raddr_accum4_pool(raddr_accum4_pool),
  .raddr_accum5_pool(raddr_accum5_pool),
  .raddr_accum6_pool(raddr_accum6_pool),
  .raddr_accum7_pool(raddr_accum7_pool),
  .raddr_accum8_pool(raddr_accum8_pool),
  .raddr_accum9_pool(raddr_accum9_pool),
  .raddr_accum10_pool(raddr_accum10_pool),
  .raddr_accum11_pool(raddr_accum11_pool),
  .raddr_accum12_pool(raddr_accum12_pool),
  .raddr_accum13_pool(raddr_accum13_pool),
  .raddr_accum14_pool(raddr_accum14_pool),
  .raddr_accum15_pool(raddr_accum15_pool),
  .raddr_accum16_pool(raddr_accum16_pool),
  .raddr_accum17_pool(raddr_accum17_pool),
  .raddr_accum18_pool(raddr_accum18_pool),
  .raddr_accum19_pool(raddr_accum19_pool),
  .raddr_accum20_pool(raddr_accum20_pool),
  .raddr_accum21_pool(raddr_accum21_pool),
  .raddr_accum22_pool(raddr_accum22_pool),
  .raddr_accum23_pool(raddr_accum23_pool),
  .raddr_accum24_pool(raddr_accum24_pool),
  .raddr_accum25_pool(raddr_accum25_pool),
  .raddr_accum26_pool(raddr_accum26_pool),
  .raddr_accum27_pool(raddr_accum27_pool),
  .raddr_accum28_pool(raddr_accum28_pool),
  .raddr_accum29_pool(raddr_accum29_pool),
  .raddr_accum30_pool(raddr_accum30_pool),
  .raddr_accum31_pool(raddr_accum31_pool),
  .rdata_accum0_pool(rdata_accum0_pool),
  .rdata_accum1_pool(rdata_accum1_pool),
  .rdata_accum2_pool(rdata_accum2_pool),
  .rdata_accum3_pool(rdata_accum3_pool),
  .rdata_accum4_pool(rdata_accum4_pool),
  .rdata_accum5_pool(rdata_accum5_pool),
  .rdata_accum6_pool(rdata_accum6_pool),
  .rdata_accum7_pool(rdata_accum7_pool),
  .rdata_accum8_pool(rdata_accum8_pool),
  .rdata_accum9_pool(rdata_accum9_pool),
  .rdata_accum10_pool(rdata_accum10_pool),
  .rdata_accum11_pool(rdata_accum11_pool),
  .rdata_accum12_pool(rdata_accum12_pool),
  .rdata_accum13_pool(rdata_accum13_pool),
  .rdata_accum14_pool(rdata_accum14_pool),
  .rdata_accum15_pool(rdata_accum15_pool),
  .rdata_accum16_pool(rdata_accum16_pool),
  .rdata_accum17_pool(rdata_accum17_pool),
  .rdata_accum18_pool(rdata_accum18_pool),
  .rdata_accum19_pool(rdata_accum19_pool),
  .rdata_accum20_pool(rdata_accum20_pool),
  .rdata_accum21_pool(rdata_accum21_pool),
  .rdata_accum22_pool(rdata_accum22_pool),
  .rdata_accum23_pool(rdata_accum23_pool),
  .rdata_accum24_pool(rdata_accum24_pool),
  .rdata_accum25_pool(rdata_accum25_pool),
  .rdata_accum26_pool(rdata_accum26_pool),
  .rdata_accum27_pool(rdata_accum27_pool),
  .rdata_accum28_pool(rdata_accum28_pool),
  .rdata_accum29_pool(rdata_accum29_pool),
  .rdata_accum30_pool(rdata_accum30_pool),
  .rdata_accum31_pool(rdata_accum31_pool)
  `endif
);

wire [`DWIDTH-1:0] pool0;
wire [`DWIDTH-1:0] pool1;
wire [`DWIDTH-1:0] pool2;
wire [`DWIDTH-1:0] pool3;
wire [`DWIDTH-1:0] pool4;
wire [`DWIDTH-1:0] pool5;
wire [`DWIDTH-1:0] pool6;
wire [`DWIDTH-1:0] pool7;
wire [`DWIDTH-1:0] pool8;
wire [`DWIDTH-1:0] pool9;
wire [`DWIDTH-1:0] pool10;
wire [`DWIDTH-1:0] pool11;
wire [`DWIDTH-1:0] pool12;
wire [`DWIDTH-1:0] pool13;
wire [`DWIDTH-1:0] pool14;
wire [`DWIDTH-1:0] pool15;
wire [`DWIDTH-1:0] pool16;
wire [`DWIDTH-1:0] pool17;
wire [`DWIDTH-1:0] pool18;
wire [`DWIDTH-1:0] pool19;
wire [`DWIDTH-1:0] pool20;
wire [`DWIDTH-1:0] pool21;
wire [`DWIDTH-1:0] pool22;
wire [`DWIDTH-1:0] pool23;
wire [`DWIDTH-1:0] pool24;
wire [`DWIDTH-1:0] pool25;
wire [`DWIDTH-1:0] pool26;
wire [`DWIDTH-1:0] pool27;
wire [`DWIDTH-1:0] pool28;
wire [`DWIDTH-1:0] pool29;
wire [`DWIDTH-1:0] pool30;
wire [`DWIDTH-1:0] pool31;

wire [`DWIDTH-1:0] norm_data_out0;
wire [`DWIDTH-1:0] norm_data_out1;
wire [`DWIDTH-1:0] norm_data_out2;
wire [`DWIDTH-1:0] norm_data_out3;
wire [`DWIDTH-1:0] norm_data_out4;
wire [`DWIDTH-1:0] norm_data_out5;
wire [`DWIDTH-1:0] norm_data_out6;
wire [`DWIDTH-1:0] norm_data_out7;
wire [`DWIDTH-1:0] norm_data_out8;
wire [`DWIDTH-1:0] norm_data_out9;
wire [`DWIDTH-1:0] norm_data_out10;
wire [`DWIDTH-1:0] norm_data_out11;
wire [`DWIDTH-1:0] norm_data_out12;
wire [`DWIDTH-1:0] norm_data_out13;
wire [`DWIDTH-1:0] norm_data_out14;
wire [`DWIDTH-1:0] norm_data_out15;
wire [`DWIDTH-1:0] norm_data_out16;
wire [`DWIDTH-1:0] norm_data_out17;
wire [`DWIDTH-1:0] norm_data_out18;
wire [`DWIDTH-1:0] norm_data_out19;
wire [`DWIDTH-1:0] norm_data_out20;
wire [`DWIDTH-1:0] norm_data_out21;
wire [`DWIDTH-1:0] norm_data_out22;
wire [`DWIDTH-1:0] norm_data_out23;
wire [`DWIDTH-1:0] norm_data_out24;
wire [`DWIDTH-1:0] norm_data_out25;
wire [`DWIDTH-1:0] norm_data_out26;
wire [`DWIDTH-1:0] norm_data_out27;
wire [`DWIDTH-1:0] norm_data_out28;
wire [`DWIDTH-1:0] norm_data_out29;
wire [`DWIDTH-1:0] norm_data_out30;
wire [`DWIDTH-1:0] norm_data_out31;

wire [`DWIDTH-1:0] act_data_out0;
wire [`DWIDTH-1:0] act_data_out1;
wire [`DWIDTH-1:0] act_data_out2;
wire [`DWIDTH-1:0] act_data_out3;
wire [`DWIDTH-1:0] act_data_out4;
wire [`DWIDTH-1:0] act_data_out5;
wire [`DWIDTH-1:0] act_data_out6;
wire [`DWIDTH-1:0] act_data_out7;
wire [`DWIDTH-1:0] act_data_out8;
wire [`DWIDTH-1:0] act_data_out9;
wire [`DWIDTH-1:0] act_data_out10;
wire [`DWIDTH-1:0] act_data_out11;
wire [`DWIDTH-1:0] act_data_out12;
wire [`DWIDTH-1:0] act_data_out13;
wire [`DWIDTH-1:0] act_data_out14;
wire [`DWIDTH-1:0] act_data_out15;
wire [`DWIDTH-1:0] act_data_out16;
wire [`DWIDTH-1:0] act_data_out17;
wire [`DWIDTH-1:0] act_data_out18;
wire [`DWIDTH-1:0] act_data_out19;
wire [`DWIDTH-1:0] act_data_out20;
wire [`DWIDTH-1:0] act_data_out21;
wire [`DWIDTH-1:0] act_data_out22;
wire [`DWIDTH-1:0] act_data_out23;
wire [`DWIDTH-1:0] act_data_out24;
wire [`DWIDTH-1:0] act_data_out25;
wire [`DWIDTH-1:0] act_data_out26;
wire [`DWIDTH-1:0] act_data_out27;
wire [`DWIDTH-1:0] act_data_out28;
wire [`DWIDTH-1:0] act_data_out29;
wire [`DWIDTH-1:0] act_data_out30;
wire [`DWIDTH-1:0] act_data_out31;

////////////////////////////////////////////////////////////////
// Pooling module
////////////////////////////////////////////////////////////////
pooling u_pooling (
  .clk(clk),
  .resetn(resetn),
  .matrix_size(matrix_size),
  .filter_size(filter_size),
  .enable_pool(enable_pool),
  .pool_select(pool_select),
  .start_pooling(start_pool),
  .pool_norm_valid(pool_norm_valid),
  `ifdef DESIGN_SIZE_8
  .raddr_accum0_pool(raddr_accum0_pool),
  .raddr_accum1_pool(raddr_accum1_pool),
  .raddr_accum2_pool(raddr_accum2_pool),
  .raddr_accum3_pool(raddr_accum3_pool),
  .raddr_accum4_pool(raddr_accum4_pool),
  .raddr_accum5_pool(raddr_accum5_pool),
  .raddr_accum6_pool(raddr_accum6_pool),
  .raddr_accum7_pool(raddr_accum7_pool),
  .rdata_accum0_pool(rdata_accum0_pool),
  .rdata_accum1_pool(rdata_accum1_pool),
  .rdata_accum2_pool(rdata_accum2_pool),
  .rdata_accum3_pool(rdata_accum3_pool),
  .rdata_accum4_pool(rdata_accum4_pool),
  .rdata_accum5_pool(rdata_accum5_pool),
  .rdata_accum6_pool(rdata_accum6_pool),
  .rdata_accum7_pool(rdata_accum7_pool),
  .pool0(pool0),
  .pool1(pool1),
  .pool2(pool2),
  .pool3(pool3),
  .pool4(pool4),
  .pool5(pool5),
  .pool6(pool6),
  .pool7(pool7)  
  `endif
  `ifdef DESIGN_SIZE_16
  .raddr_accum0_pool(raddr_accum0_pool),
  .raddr_accum1_pool(raddr_accum1_pool),
  .raddr_accum2_pool(raddr_accum2_pool),
  .raddr_accum3_pool(raddr_accum3_pool),
  .raddr_accum4_pool(raddr_accum4_pool),
  .raddr_accum5_pool(raddr_accum5_pool),
  .raddr_accum6_pool(raddr_accum6_pool),
  .raddr_accum7_pool(raddr_accum7_pool),
  .raddr_accum8_pool(raddr_accum8_pool),
  .raddr_accum9_pool(raddr_accum9_pool),
  .raddr_accum10_pool(raddr_accum10_pool),
  .raddr_accum11_pool(raddr_accum11_pool),
  .raddr_accum12_pool(raddr_accum12_pool),
  .raddr_accum13_pool(raddr_accum13_pool),
  .raddr_accum14_pool(raddr_accum14_pool),
  .raddr_accum15_pool(raddr_accum15_pool),
  .rdata_accum0_pool(rdata_accum0_pool),
  .rdata_accum1_pool(rdata_accum1_pool),
  .rdata_accum2_pool(rdata_accum2_pool),
  .rdata_accum3_pool(rdata_accum3_pool),
  .rdata_accum4_pool(rdata_accum4_pool),
  .rdata_accum5_pool(rdata_accum5_pool),
  .rdata_accum6_pool(rdata_accum6_pool),
  .rdata_accum7_pool(rdata_accum7_pool),
  .rdata_accum8_pool(rdata_accum8_pool),
  .rdata_accum9_pool(rdata_accum9_pool),
  .rdata_accum10_pool(rdata_accum10_pool),
  .rdata_accum11_pool(rdata_accum11_pool),
  .rdata_accum12_pool(rdata_accum12_pool),
  .rdata_accum13_pool(rdata_accum13_pool),
  .rdata_accum14_pool(rdata_accum14_pool),
  .rdata_accum15_pool(rdata_accum15_pool),
  .pool0(pool0),
  .pool1(pool1),
  .pool2(pool2),
  .pool3(pool3),
  .pool4(pool4),
  .pool5(pool5),
  .pool6(pool6),
  .pool7(pool7),
  .pool8(pool8),
  .pool9(pool9),
  .pool10(pool10),
  .pool11(pool11),
  .pool12(pool12),
  .pool13(pool13),
  .pool14(pool14),
  .pool15(pool15)
  `endif
  `ifdef DESIGN_SIZE_32
  .raddr_accum0_pool(raddr_accum0_pool),
  .raddr_accum1_pool(raddr_accum1_pool),
  .raddr_accum2_pool(raddr_accum2_pool),
  .raddr_accum3_pool(raddr_accum3_pool),
  .raddr_accum4_pool(raddr_accum4_pool),
  .raddr_accum5_pool(raddr_accum5_pool),
  .raddr_accum6_pool(raddr_accum6_pool),
  .raddr_accum7_pool(raddr_accum7_pool),
  .raddr_accum8_pool(raddr_accum8_pool),
  .raddr_accum9_pool(raddr_accum9_pool),
  .raddr_accum10_pool(raddr_accum10_pool),
  .raddr_accum11_pool(raddr_accum11_pool),
  .raddr_accum12_pool(raddr_accum12_pool),
  .raddr_accum13_pool(raddr_accum13_pool),
  .raddr_accum14_pool(raddr_accum14_pool),
  .raddr_accum15_pool(raddr_accum15_pool),
  .raddr_accum16_pool(raddr_accum16_pool),
  .raddr_accum17_pool(raddr_accum17_pool),
  .raddr_accum18_pool(raddr_accum18_pool),
  .raddr_accum19_pool(raddr_accum19_pool),
  .raddr_accum20_pool(raddr_accum20_pool),
  .raddr_accum21_pool(raddr_accum21_pool),
  .raddr_accum22_pool(raddr_accum22_pool),
  .raddr_accum23_pool(raddr_accum23_pool),
  .raddr_accum24_pool(raddr_accum24_pool),
  .raddr_accum25_pool(raddr_accum25_pool),
  .raddr_accum26_pool(raddr_accum26_pool),
  .raddr_accum27_pool(raddr_accum27_pool),
  .raddr_accum28_pool(raddr_accum28_pool),
  .raddr_accum29_pool(raddr_accum29_pool),
  .raddr_accum30_pool(raddr_accum30_pool),
  .raddr_accum31_pool(raddr_accum31_pool),
  .rdata_accum0_pool(rdata_accum0_pool),
  .rdata_accum1_pool(rdata_accum1_pool),
  .rdata_accum2_pool(rdata_accum2_pool),
  .rdata_accum3_pool(rdata_accum3_pool),
  .rdata_accum4_pool(rdata_accum4_pool),
  .rdata_accum5_pool(rdata_accum5_pool),
  .rdata_accum6_pool(rdata_accum6_pool),
  .rdata_accum7_pool(rdata_accum7_pool),
  .rdata_accum8_pool(rdata_accum8_pool),
  .rdata_accum9_pool(rdata_accum9_pool),
  .rdata_accum10_pool(rdata_accum10_pool),
  .rdata_accum11_pool(rdata_accum11_pool),
  .rdata_accum12_pool(rdata_accum12_pool),
  .rdata_accum13_pool(rdata_accum13_pool),
  .rdata_accum14_pool(rdata_accum14_pool),
  .rdata_accum15_pool(rdata_accum15_pool),
  .rdata_accum16_pool(rdata_accum16_pool),
  .rdata_accum17_pool(rdata_accum17_pool),
  .rdata_accum18_pool(rdata_accum18_pool),
  .rdata_accum19_pool(rdata_accum19_pool),
  .rdata_accum20_pool(rdata_accum20_pool),
  .rdata_accum21_pool(rdata_accum21_pool),
  .rdata_accum22_pool(rdata_accum22_pool),
  .rdata_accum23_pool(rdata_accum23_pool),
  .rdata_accum24_pool(rdata_accum24_pool),
  .rdata_accum25_pool(rdata_accum25_pool),
  .rdata_accum26_pool(rdata_accum26_pool),
  .rdata_accum27_pool(rdata_accum27_pool),
  .rdata_accum28_pool(rdata_accum28_pool),
  .rdata_accum29_pool(rdata_accum29_pool),
  .rdata_accum30_pool(rdata_accum30_pool),
  .rdata_accum31_pool(rdata_accum31_pool),
  .pool0(pool0),
  .pool1(pool1),
  .pool2(pool2),
  .pool3(pool3),
  .pool4(pool4),
  .pool5(pool5),
  .pool6(pool6),
  .pool7(pool7),
  .pool8(pool8),
  .pool9(pool9),
  .pool10(pool10),
  .pool11(pool11),
  .pool12(pool12),
  .pool13(pool13),
  .pool14(pool14),
  .pool15(pool15),
  .pool16(pool16),
  .pool17(pool17),
  .pool18(pool18),
  .pool19(pool19),
  .pool20(pool20),
  .pool21(pool21),
  .pool22(pool22),
  .pool23(pool23),
  .pool24(pool24),
  .pool25(pool25),
  .pool26(pool26),
  .pool27(pool27),
  .pool28(pool28),
  .pool29(pool29),
  .pool30(pool30),
  .pool31(pool31)
  `endif
);


////////////////////////////////////////////////////////////////
// Normalization module
////////////////////////////////////////////////////////////////
norm u_norm(
  .enable_norm(enable_norm),
  .mean(mean),
  .inv_var(inv_var),
  .in_data_available(pool_norm_valid),
  `ifdef DESIGN_SIZE_8
  .inp_data0(pool0),
  .inp_data1(pool1),
  .inp_data2(pool2),
  .inp_data3(pool3),
  .inp_data4(pool4),
  .inp_data5(pool5),
  .inp_data6(pool6),
  .inp_data7(pool7),
  .out_data0(norm_data_out0),
  .out_data1(norm_data_out1),
  .out_data2(norm_data_out2),
  .out_data3(norm_data_out3),
  .out_data4(norm_data_out4),
  .out_data5(norm_data_out5),
  .out_data6(norm_data_out6),
  .out_data7(norm_data_out7),
  `endif
  `ifdef DESIGN_SIZE_16
  .inp_data0(pool0),
  .inp_data1(pool1),
  .inp_data2(pool2),
  .inp_data3(pool3),
  .inp_data4(pool4),
  .inp_data5(pool5),
  .inp_data6(pool6),
  .inp_data7(pool7),
  .inp_data8(pool8),
  .inp_data9(pool9),
  .inp_data10(pool10),
  .inp_data11(pool11),
  .inp_data12(pool12),
  .inp_data13(pool13),
  .inp_data14(pool14),
  .inp_data15(pool15),
  .out_data0(norm_data_out0),
  .out_data1(norm_data_out1),
  .out_data2(norm_data_out2),
  .out_data3(norm_data_out3),
  .out_data4(norm_data_out4),
  .out_data5(norm_data_out5),
  .out_data6(norm_data_out6),
  .out_data7(norm_data_out7),
  .out_data8(norm_data_out8),
  .out_data9(norm_data_out9),
  .out_data10(norm_data_out10),
  .out_data11(norm_data_out11),
  .out_data12(norm_data_out12),
  .out_data13(norm_data_out13),
  .out_data14(norm_data_out14),
  .out_data15(norm_data_out15),
  `endif
  `ifdef DESIGN_SIZE_32
  .inp_data0(pool0),
  .inp_data1(pool1),
  .inp_data2(pool2),
  .inp_data3(pool3),
  .inp_data4(pool4),
  .inp_data5(pool5),
  .inp_data6(pool6),
  .inp_data7(pool7),
  .inp_data8(pool8),
  .inp_data9(pool9),
  .inp_data10(pool10),
  .inp_data11(pool11),
  .inp_data12(pool12),
  .inp_data13(pool13),
  .inp_data14(pool14),
  .inp_data15(pool15),
  .inp_data16(pool16),
  .inp_data17(pool17),
  .inp_data18(pool18),
  .inp_data19(pool19),
  .inp_data20(pool20),
  .inp_data21(pool21),
  .inp_data22(pool22),
  .inp_data23(pool23),
  .inp_data24(pool24),
  .inp_data25(pool25),
  .inp_data26(pool26),
  .inp_data27(pool27),
  .inp_data28(pool28),
  .inp_data29(pool29),
  .inp_data30(pool30),
  .inp_data31(pool31),
  .out_data0(norm_data_out0),
  .out_data1(norm_data_out1),
  .out_data2(norm_data_out2),
  .out_data3(norm_data_out3),
  .out_data4(norm_data_out4),
  .out_data5(norm_data_out5),
  .out_data6(norm_data_out6),
  .out_data7(norm_data_out7),
  .out_data8(norm_data_out8),
  .out_data9(norm_data_out9),
  .out_data10(norm_data_out10),
  .out_data11(norm_data_out11),
  .out_data12(norm_data_out12),
  .out_data13(norm_data_out13),
  .out_data14(norm_data_out14),
  .out_data15(norm_data_out15),
  .out_data16(norm_data_out16),
  .out_data17(norm_data_out17),
  .out_data18(norm_data_out18),
  .out_data19(norm_data_out19),
  .out_data20(norm_data_out20),
  .out_data21(norm_data_out21),
  .out_data22(norm_data_out22),
  .out_data23(norm_data_out23),
  .out_data24(norm_data_out24),
  .out_data25(norm_data_out25),
  .out_data26(norm_data_out26),
  .out_data27(norm_data_out27),
  .out_data28(norm_data_out28),
  .out_data29(norm_data_out29),
  .out_data30(norm_data_out30),
  .out_data31(norm_data_out31),
  `endif
  .out_data_available(norm_out_data_available),
  .validity_mask(validity_mask_a_rows),
  .done_norm(done_norm),
  .clk(clk),
  .reset(reset)
);

////////////////////////////////////////////////////////////////
// Activation module
////////////////////////////////////////////////////////////////
activation u_activation(
  .activation_type(activation_type),
  .enable_activation(enable_activation),
  .enable_pool(enable_pool),
  .in_data_available(norm_out_data_available),
  `ifdef DESIGN_SIZE_8
  .inp_data0(norm_data_out0),
  .inp_data1(norm_data_out1),
  .inp_data2(norm_data_out2),
  .inp_data3(norm_data_out3),
  .inp_data4(norm_data_out4),
  .inp_data5(norm_data_out5),
  .inp_data6(norm_data_out6),
  .inp_data7(norm_data_out7),
  .out_data0(act_data_out0),
  .out_data1(act_data_out1),
  .out_data2(act_data_out2),
  .out_data3(act_data_out3),
  .out_data4(act_data_out4),
  .out_data5(act_data_out5),
  .out_data6(act_data_out6),
  .out_data7(act_data_out7),
  `endif
  `ifdef DESIGN_SIZE_16
  .inp_data0(norm_data_out0),
  .inp_data1(norm_data_out1),
  .inp_data2(norm_data_out2),
  .inp_data3(norm_data_out3),
  .inp_data4(norm_data_out4),
  .inp_data5(norm_data_out5),
  .inp_data6(norm_data_out6),
  .inp_data7(norm_data_out7),
  .inp_data8(norm_data_out8),
  .inp_data9(norm_data_out9),
  .inp_data10(norm_data_out10),
  .inp_data11(norm_data_out11),
  .inp_data12(norm_data_out12),
  .inp_data13(norm_data_out13),
  .inp_data14(norm_data_out14),
  .inp_data15(norm_data_out15),
  .out_data0(act_data_out0),
  .out_data1(act_data_out1),
  .out_data2(act_data_out2),
  .out_data3(act_data_out3),
  .out_data4(act_data_out4),
  .out_data5(act_data_out5),
  .out_data6(act_data_out6),
  .out_data7(act_data_out7),
  .out_data8(act_data_out8),
  .out_data9(act_data_out9),
  .out_data10(act_data_out10),
  .out_data11(act_data_out11),
  .out_data12(act_data_out12),
  .out_data13(act_data_out13),
  .out_data14(act_data_out14),
  .out_data15(act_data_out15),
  `endif
  `ifdef DESIGN_SIZE_32
  .inp_data0(norm_data_out0),
  .inp_data1(norm_data_out1),
  .inp_data2(norm_data_out2),
  .inp_data3(norm_data_out3),
  .inp_data4(norm_data_out4),
  .inp_data5(norm_data_out5),
  .inp_data6(norm_data_out6),
  .inp_data7(norm_data_out7),
  .inp_data8(norm_data_out8),
  .inp_data9(norm_data_out9),
  .inp_data10(norm_data_out10),
  .inp_data11(norm_data_out11),
  .inp_data12(norm_data_out12),
  .inp_data13(norm_data_out13),
  .inp_data14(norm_data_out14),
  .inp_data15(norm_data_out15),
  .inp_data16(norm_data_out16),
  .inp_data17(norm_data_out17),
  .inp_data18(norm_data_out18),
  .inp_data19(norm_data_out19),
  .inp_data20(norm_data_out20),
  .inp_data21(norm_data_out21),
  .inp_data22(norm_data_out22),
  .inp_data23(norm_data_out23),
  .inp_data24(norm_data_out24),
  .inp_data25(norm_data_out25),
  .inp_data26(norm_data_out26),
  .inp_data27(norm_data_out27),
  .inp_data28(norm_data_out28),
  .inp_data29(norm_data_out29),
  .inp_data30(norm_data_out30),
  .inp_data31(norm_data_out31),
  .out_data0(act_data_out0),
  .out_data1(act_data_out1),
  .out_data2(act_data_out2),
  .out_data3(act_data_out3),
  .out_data4(act_data_out4),
  .out_data5(act_data_out5),
  .out_data6(act_data_out6),
  .out_data7(act_data_out7),
  .out_data8(act_data_out8),
  .out_data9(act_data_out9),
  .out_data10(act_data_out10),
  .out_data11(act_data_out11),
  .out_data12(act_data_out12),
  .out_data13(act_data_out13),
  .out_data14(act_data_out14),
  .out_data15(act_data_out15),
  .out_data16(act_data_out16),
  .out_data17(act_data_out17),
  .out_data18(act_data_out18),
  .out_data19(act_data_out19),
  .out_data20(act_data_out20),
  .out_data21(act_data_out21),
  .out_data22(act_data_out22),
  .out_data23(act_data_out23),
  .out_data24(act_data_out24),
  .out_data25(act_data_out25),
  .out_data26(act_data_out26),
  .out_data27(act_data_out27),
  .out_data28(act_data_out28),
  .out_data29(act_data_out29),
  .out_data30(act_data_out30),
  .out_data31(act_data_out31),
  `endif
  .out_data_available(activation_out_data_available),
  .validity_mask(validity_mask_a_rows),
  .done_activation(done_activation),
  .clk(clk),
  .reset(reset)
);

//Interface to BRAM to write the output.
//Ideally, we could remove this flop stage. But then we'd
//have to generate the address for the output BRAM in each
//block that could potentially write the output. 

reg activation_out_data_available1;
reg activation_out_data_available2;
reg activation_out_data_available3;
reg activation_out_data_available4;
reg activation_out_data_available5;
reg activation_out_data_available6;
reg activation_out_data_available7;

`ifdef DESIGN_SIZE_16
reg activation_out_data_available8;
reg activation_out_data_available9;
reg activation_out_data_available10;
reg activation_out_data_available11;
reg activation_out_data_available12;
reg activation_out_data_available13;
reg activation_out_data_available14;
reg activation_out_data_available15;
`endif

`ifdef DESIGN_SIZE_32
reg activation_out_data_available8;
reg activation_out_data_available9;
reg activation_out_data_available10;
reg activation_out_data_available11;
reg activation_out_data_available12;
reg activation_out_data_available13;
reg activation_out_data_available14;
reg activation_out_data_available15;
reg activation_out_data_available16;
reg activation_out_data_available17;
reg activation_out_data_available18;
reg activation_out_data_available19;
reg activation_out_data_available20;
reg activation_out_data_available21;
reg activation_out_data_available22;
reg activation_out_data_available23;
reg activation_out_data_available24;
reg activation_out_data_available25;
reg activation_out_data_available26;
reg activation_out_data_available27;
reg activation_out_data_available28;
reg activation_out_data_available29;
reg activation_out_data_available30;
reg activation_out_data_available31;
`endif

always @(posedge clk) begin
    activation_out_data_available1 <= activation_out_data_available;
    activation_out_data_available2 <= activation_out_data_available1;
    activation_out_data_available3 <= activation_out_data_available2;
    activation_out_data_available4 <= activation_out_data_available3;
    activation_out_data_available5 <= activation_out_data_available4;
    activation_out_data_available6 <= activation_out_data_available5;
    activation_out_data_available7 <= activation_out_data_available6;
end

`ifdef DESIGN_SIZE_16
always @(posedge clk) begin
    activation_out_data_available8 <= activation_out_data_available7;
    activation_out_data_available9 <= activation_out_data_available8;
    activation_out_data_available10 <= activation_out_data_available9;
    activation_out_data_available11 <= activation_out_data_available10;
    activation_out_data_available12 <= activation_out_data_available11;
    activation_out_data_available13 <= activation_out_data_available12;
    activation_out_data_available14 <= activation_out_data_available13;
    activation_out_data_available15 <= activation_out_data_available14;
end
`endif

`ifdef DESIGN_SIZE_32
always @(posedge clk) begin
    activation_out_data_available8 <= activation_out_data_available7;
    activation_out_data_available9 <= activation_out_data_available8;
    activation_out_data_available10 <= activation_out_data_available9;
    activation_out_data_available11 <= activation_out_data_available10;
    activation_out_data_available12 <= activation_out_data_available11;
    activation_out_data_available13 <= activation_out_data_available12;
    activation_out_data_available14 <= activation_out_data_available13;
    activation_out_data_available15 <= activation_out_data_available14;
    activation_out_data_available16 <= activation_out_data_available15;
    activation_out_data_available17 <= activation_out_data_available16;
    activation_out_data_available18 <= activation_out_data_available17;
    activation_out_data_available19 <= activation_out_data_available18;
    activation_out_data_available20 <= activation_out_data_available19;
    activation_out_data_available21 <= activation_out_data_available20;
    activation_out_data_available22 <= activation_out_data_available21;
    activation_out_data_available23 <= activation_out_data_available22;
    activation_out_data_available24 <= activation_out_data_available23;
    activation_out_data_available25 <= activation_out_data_available24;
    activation_out_data_available26 <= activation_out_data_available25;
    activation_out_data_available27 <= activation_out_data_available26;
    activation_out_data_available28 <= activation_out_data_available27;
    activation_out_data_available29 <= activation_out_data_available28;
    activation_out_data_available30 <= activation_out_data_available29;
    activation_out_data_available31 <= activation_out_data_available30;
end
`endif

reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data0;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data1;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data2;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data3;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data4;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data5;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data6;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data7;

`ifdef DESIGN_SIZE_16
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data8;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data9;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data10;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data11;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data12;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data13;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data14;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data15;
`endif

`ifdef DESIGN_SIZE_32
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data8;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data9;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data10;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data11;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data12;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data13;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data14;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data15;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data16;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data17;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data18;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data19;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data20;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data21;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data22;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data23;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data24;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data25;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data26;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data27;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data28;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data29;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data30;
reg [(`MAT_MUL_SIZE*`DWIDTH)-1:0] final_data31;
`endif

always @(posedge clk) begin
    if (reset) begin
        final_data0 <= 0;
    end
    else if (activation_out_data_available) begin
        final_data0 <= {act_data_out0[7:0],final_data0[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data1 <= 0;
    end
    else if (activation_out_data_available1) begin
        final_data1 <= {act_data_out1[7:0],final_data1[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data2 <= 0;
    end
    else if (activation_out_data_available2) begin
        final_data2 <= {act_data_out2[7:0],final_data2[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data3 <= 0;
    end
    else if (activation_out_data_available3) begin
        final_data3 <= {act_data_out3[7:0],final_data3[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data4 <= 0;
    end
    else if (activation_out_data_available4) begin
        final_data4 <= {act_data_out4[7:0],final_data4[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data5 <= 0;
    end
    else if (activation_out_data_available5) begin
        final_data5 <= {act_data_out5[7:0],final_data5[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data6 <= 0;
    end
    else if (activation_out_data_available6) begin
        final_data6 <= {act_data_out6[7:0],final_data6[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data7 <= 0;
    end
    else if (activation_out_data_available7) begin
        final_data7 <= {act_data_out7[7:0],final_data7[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

`ifdef DESIGN_SIZE_16
always @(posedge clk) begin
    if (reset) begin
        final_data8 <= 0;
    end
    else if (activation_out_data_available8) begin
        final_data8 <= {act_data_out8[7:0],final_data8[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data9 <= 0;
    end
    else if (activation_out_data_available9) begin
        final_data9 <= {act_data_out9[7:0],final_data9[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data10 <= 0;
    end
    else if (activation_out_data_available10) begin
        final_data10 <= {act_data_out10[7:0],final_data10[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data11 <= 0;
    end
    else if (activation_out_data_available11) begin
        final_data11 <= {act_data_out11[7:0],final_data11[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data12 <= 0;
    end
    else if (activation_out_data_available12) begin
        final_data12 <= {act_data_out12[7:0],final_data12[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data13 <= 0;
    end
    else if (activation_out_data_available13) begin
        final_data13 <= {act_data_out13[7:0],final_data13[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data14 <= 0;
    end
    else if (activation_out_data_available14) begin
        final_data14 <= {act_data_out14[7:0],final_data14[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data15 <= 0;
    end
    else if (activation_out_data_available15) begin
        final_data15 <= {act_data_out15[7:0],final_data15[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end
`endif

`ifdef DESIGN_SIZE_32
always @(posedge clk) begin
    if (reset) begin
        final_data8 <= 0;
    end
    else if (activation_out_data_available8) begin
        final_data8 <= {act_data_out8[7:0],final_data8[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data9 <= 0;
    end
    else if (activation_out_data_available9) begin
        final_data9 <= {act_data_out9[7:0],final_data9[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data10 <= 0;
    end
    else if (activation_out_data_available10) begin
        final_data10 <= {act_data_out10[7:0],final_data10[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data11 <= 0;
    end
    else if (activation_out_data_available11) begin
        final_data11 <= {act_data_out11[7:0],final_data11[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data12 <= 0;
    end
    else if (activation_out_data_available12) begin
        final_data12 <= {act_data_out12[7:0],final_data12[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data13 <= 0;
    end
    else if (activation_out_data_available13) begin
        final_data13 <= {act_data_out13[7:0],final_data13[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data14 <= 0;
    end
    else if (activation_out_data_available14) begin
        final_data14 <= {act_data_out14[7:0],final_data14[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data15 <= 0;
    end
    else if (activation_out_data_available15) begin
        final_data15 <= {act_data_out15[7:0],final_data15[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data16 <= 0;
    end
    else if (activation_out_data_available16) begin
        final_data16 <= {act_data_out16[7:0],final_data16[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data17 <= 0;
    end
    else if (activation_out_data_available17) begin
        final_data17 <= {act_data_out17[7:0],final_data17[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data18 <= 0;
    end
    else if (activation_out_data_available18) begin
        final_data18 <= {act_data_out18[7:0],final_data18[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data19 <= 0;
    end
    else if (activation_out_data_available19) begin
        final_data19 <= {act_data_out19[7:0],final_data19[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data20 <= 0;
    end
    else if (activation_out_data_available20) begin
        final_data20 <= {act_data_out20[7:0],final_data20[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data21 <= 0;
    end
    else if (activation_out_data_available21) begin
        final_data21 <= {act_data_out21[7:0],final_data21[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data22 <= 0;
    end
    else if (activation_out_data_available22) begin
        final_data22 <= {act_data_out22[7:0],final_data22[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data23 <= 0;
    end
    else if (activation_out_data_available23) begin
        final_data23 <= {act_data_out23[7:0],final_data23[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data24 <= 0;
    end
    else if (activation_out_data_available24) begin
        final_data24 <= {act_data_out24[7:0],final_data24[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data25 <= 0;
    end
    else if (activation_out_data_available25) begin
        final_data25 <= {act_data_out25[7:0],final_data25[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data26 <= 0;
    end
    else if (activation_out_data_available26) begin
        final_data26 <= {act_data_out26[7:0],final_data26[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data27 <= 0;
    end
    else if (activation_out_data_available27) begin
        final_data27 <= {act_data_out27[7:0],final_data27[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data28 <= 0;
    end
    else if (activation_out_data_available28) begin
        final_data28 <= {act_data_out28[7:0],final_data28[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data29 <= 0;
    end
    else if (activation_out_data_available29) begin
        final_data29 <= {act_data_out29[7:0],final_data29[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data30 <= 0;
    end
    else if (activation_out_data_available30) begin
        final_data30 <= {act_data_out30[7:0],final_data30[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end

always @(posedge clk) begin
    if (reset) begin
        final_data31 <= 0;
    end
    else if (activation_out_data_available31) begin
        final_data31 <= {act_data_out31[7:0],final_data31[(`MAT_MUL_SIZE*`DWIDTH)-1:8]};
    end
end
`endif

reg [31:0] i;
  always @(posedge clk) begin
    if (reset) begin
        i <= 0;
        bram_wdata_a <= 0;
        bram_addr_a_for_writing <= address_mat_c + address_stride_c;
        bram_a_wdata_available <= 0;
      end
    else if (done_activation) begin
        i <= i + 1;
        case(i)
        `ifdef DESIGN_SIZE_8
        0: begin bram_wdata_a <= final_data0; end
        1: begin bram_wdata_a <= final_data1; end
        2: begin bram_wdata_a <= final_data2; end
        3: begin bram_wdata_a <= final_data3; end
        4: begin bram_wdata_a <= final_data4; end
        5: begin bram_wdata_a <= final_data5; end
        6: begin bram_wdata_a <= final_data6; end
        7: begin bram_wdata_a <= final_data7; end
        default : begin bram_wdata_a <= final_data7; end
        `endif
        `ifdef DESIGN_SIZE_16
        0: begin bram_wdata_a <= final_data0; end
        1: begin bram_wdata_a <= final_data1; end
        2: begin bram_wdata_a <= final_data2; end
        3: begin bram_wdata_a <= final_data3; end
        4: begin bram_wdata_a <= final_data4; end
        5: begin bram_wdata_a <= final_data5; end
        6: begin bram_wdata_a <= final_data6; end
        7: begin bram_wdata_a <= final_data7; end
        8: begin bram_wdata_a <= final_data8; end
        9: begin bram_wdata_a <= final_data9; end
        10: begin bram_wdata_a <= final_data10; end
        11: begin bram_wdata_a <= final_data11; end
        12: begin bram_wdata_a <= final_data12; end
        13: begin bram_wdata_a <= final_data13; end
        14: begin bram_wdata_a <= final_data14; end
        15: begin bram_wdata_a <= final_data15; end
        default : begin bram_wdata_a <= final_data15; end
        `endif 
        `ifdef DESIGN_SIZE_32
        0: begin bram_wdata_a <= final_data0; end
        1: begin bram_wdata_a <= final_data1; end
        2: begin bram_wdata_a <= final_data2; end
        3: begin bram_wdata_a <= final_data3; end
        4: begin bram_wdata_a <= final_data4; end
        5: begin bram_wdata_a <= final_data5; end
        6: begin bram_wdata_a <= final_data6; end
        7: begin bram_wdata_a <= final_data7; end
        8: begin bram_wdata_a <= final_data8; end
        9: begin bram_wdata_a <= final_data9; end
        10: begin bram_wdata_a <= final_data10; end
        11: begin bram_wdata_a <= final_data11; end
        12: begin bram_wdata_a <= final_data12; end
        13: begin bram_wdata_a <= final_data13; end
        14: begin bram_wdata_a <= final_data14; end
        15: begin bram_wdata_a <= final_data15; end
        16: begin bram_wdata_a <= final_data16; end
        17: begin bram_wdata_a <= final_data17; end
        18: begin bram_wdata_a <= final_data18; end
        19: begin bram_wdata_a <= final_data19; end
        20: begin bram_wdata_a <= final_data20; end
        21: begin bram_wdata_a <= final_data21; end
        22: begin bram_wdata_a <= final_data22; end
        23: begin bram_wdata_a <= final_data23; end
        24: begin bram_wdata_a <= final_data24; end
        25: begin bram_wdata_a <= final_data25; end
        26: begin bram_wdata_a <= final_data26; end
        27: begin bram_wdata_a <= final_data27; end
        28: begin bram_wdata_a <= final_data28; end
        29: begin bram_wdata_a <= final_data29; end
        30: begin bram_wdata_a <= final_data30; end
        31: begin bram_wdata_a <= final_data31; end
        default : begin bram_wdata_a <= final_data31; end
        `endif
        endcase
        bram_addr_a_for_writing <= bram_addr_a_for_writing - address_stride_c;
        bram_a_wdata_available <= done_activation;
    end
    else begin
        bram_wdata_a <= 0;
        bram_addr_a_for_writing <= address_mat_c + address_stride_c;
        bram_a_wdata_available <= 0;
    end
  end
 

endmodule


