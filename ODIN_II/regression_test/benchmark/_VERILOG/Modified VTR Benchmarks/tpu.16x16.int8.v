`timescale 1ns / 1ps

///////////////////////////////////
// Overview
///////////////////////////////////
//This design is based on the architecture from Google's TPU v1 [1]. At its heart, 
//it uses a 16x16 matrix multiplication unit, instead of a 256x256 matrix multiplication 
//unit used by the TPU. The design uses int8 precision. This systolic matrix multiplication
//unit is a output stationary unit, compared to weight stationary architecture used in the TPU.
//The activations are stored in RAM block A, whereas the weights are stored in RAM block B. 
//Control and configuration are done through an APB interface, instead of a PCIe interface on 
//the TPU. The normalization block applies the mean and variance values to the output of the 
//matrix multiplication unit. Pooling unit supports 3 pooling windows - 1x1, 2x2 and 4x4. 
//The activation unit supports two activation functions - rectified linear unit (ReLU) and 
//the hyperbolic tangent (TanH). The activation unit is the last unit before the results 
//are written back to RAM block A, from where they can be read again into the matrix 
//multiplication unit for the next layer.
//
//[1] Jouppi et. al., In-Datacenter Performance Analysis of a Tensor Processing Unit, ISCA 2017

//////////////////////////////////////
// Module hierarchy
//////////////////////////////////////
// top                                      (the top level design)
//   |--- ram                   matrix_A    (the RAM that stores matrix A (activations))
//   |--- ram                   matrix_B    (the RAM that stores matrix B (weights))
//   |--- control               u_control   (the state machine that controls the operation)
//   |--- cfg                   u_cfg       (unit to configure/observe registers using an APB interface)
//   |--- matmul_16x16_systolic u_matmul    (systolic 16x16 matrix multiplication unit)
//   |    |--- output_logic                 (contains logic to shift out the outputs of matmul)
//   |    |--- systolic_data_setup          (contains logic to shift in the inputs of the matmul)
//   |    |--- systolic_pe_matrix           (16x16 matrix of processing elements)
//   |         |--- processing_element      (one processing element)
//   |              |--- seq_mac            (mac block inside each processing element)
//   |                   |--- qmult         (multiplier inside each mac)
//   |                   |--- qadd          (adder inside each mac)
//   |--- norm                  u_norm      (normalization block; applies mean and variance)
//   |--- pool                  u_pool      (block that performs pooling)
//   |--- activation            u_activation(block that applies activation - relu or tanh)

//////////////////////////////////////
// Tested architectures
//////////////////////////////////////
// This design has been tested with:
// 1. The VTR flagship 40nm architecture. Example: arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml
//    Properties of this design on this architecture:
//      Critical path delay: 8.32 ns               
//      Clock frequency: 120.19 MHz
//      Critical path: Includes the multiplier in the MAC in a PE and inter-CLB routing
//      Logic area (used): 1.97532e+08 MWTAs
//      Resource usage: 1556 LBs, 8 RAMs, 276 Multipliers
//      Runtime (on Intel Xeon E5-2430 2.5GHz with single thread): 3200 sec
// 2. 22nm architectures generated from COFFE. Example: arch/COFFE_22nm/stratix10_arch.xml
//    Properties of this design on this architecture:
//      Critical path delay: 9.24 ns             
//      Clock frequency: 108.17 MHz
//      Critical path: Includes the multiplier in the MAC in a PE and inter-CLB routing
//      Logic area (used): 4.95598e+07 MWTAs
//      Resource usage: 1477 LBs, 14 RAMs, 280 Multipliers
//      Runtime (on Intel Xeon E5-2430 2.5GHz with single thread): 3400 sec

//////////////////////////////////////
// Parameters
//////////////////////////////////////

//The width of the data. This design uses int8 precision. So, DWIDTH is 8
//To change to a floating point 16 version, change this to 16 and also
//change the datapath components (like adder and multiplier) to be floating point. 
`define DWIDTH 8

//This is the size of the matrix multiplier unit. In this design, we have a systolic
//matrix multiplication unit that can multiply 16x16 matrix with a 16x16 matrix.
`define DESIGN_SIZE 16
`define LOG2_DESIGN_SIZE 5
`define MAT_MUL_SIZE 16
`define MASK_WIDTH 16
`define LOG2_MAT_MUL_SIZE 5

//This it the size of the address bus, or the depth of the RAM. Each location of 
//the RAM is DWIDTH * MAT_MUL_SIZE wide. So, in this design, we use a total of
//1024 * 16 bytes of memory (i.e. 16 KB).
`define AWIDTH 10

//This is the number of clock cycles spent in the mac block
`define NUM_CYCLES_IN_MAC 3

//This defines the latency of accessing data from a block ram
`define MEM_ACCESS_LATENCY 1

//Data width and address width of the APB interface for registers
`define REG_DATAWIDTH 32
`define REG_ADDRWIDTH 8

//Width of the stride for each column in the matrices (same as ram address width)
`define ADDR_STRIDE_WIDTH 16

//Number of bits to specify the pooling window. We support 3 sizes.
`define MAX_BITS_POOL 3

/////////////////////////////////////////////////
// Register specification
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
`define REG_VALID_MASK_A_COLS_ADDR 32'h54
`define REG_VALID_MASK_B_ROWS_ADDR 32'h5c
`define REG_VALID_MASK_B_COLS_ADDR 32'h58
//Bit `MASK_WIDTH-1:0 validity_mask

//This used to be a normal signal, but changing it to a `define.
//That's because it's not required to be a variable in this design.
//And ODIN doesn't seem to propagate constants properly.
`define final_mat_mul_size 16

/////////////////////////////////////
// Matrix multiplication unit
////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2020-09-27 21:12:45.762386
// Design Name: 
// Module Name: matmul_16x16_systolic
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////

module matmul_16x16_systolic(
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
                          ((final_mat_mul_size<<2) - 2 + `NUM_CYCLES_IN_MAC);  
    
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
.b0_data(b0_data),
.a1_data_delayed_1(a1_data_delayed_1),
.b1_data_delayed_1(b1_data_delayed_1),
.a2_data_delayed_2(a2_data_delayed_2),
.b2_data_delayed_2(b2_data_delayed_2),
.a3_data_delayed_3(a3_data_delayed_3),
.b3_data_delayed_3(b3_data_delayed_3),
.a4_data_delayed_4(a4_data_delayed_4),
.b4_data_delayed_4(b4_data_delayed_4),
.a5_data_delayed_5(a5_data_delayed_5),
.b5_data_delayed_5(b5_data_delayed_5),
.a6_data_delayed_6(a6_data_delayed_6),
.b6_data_delayed_6(b6_data_delayed_6),
.a7_data_delayed_7(a7_data_delayed_7),
.b7_data_delayed_7(b7_data_delayed_7),
.a8_data_delayed_8(a8_data_delayed_8),
.b8_data_delayed_8(b8_data_delayed_8),
.a9_data_delayed_9(a9_data_delayed_9),
.b9_data_delayed_9(b9_data_delayed_9),
.a10_data_delayed_10(a10_data_delayed_10),
.b10_data_delayed_10(b10_data_delayed_10),
.a11_data_delayed_11(a11_data_delayed_11),
.b11_data_delayed_11(b11_data_delayed_11),
.a12_data_delayed_12(a12_data_delayed_12),
.b12_data_delayed_12(b12_data_delayed_12),
.a13_data_delayed_13(a13_data_delayed_13),
.b13_data_delayed_13(b13_data_delayed_13),
.a14_data_delayed_14(a14_data_delayed_14),
.b14_data_delayed_14(b14_data_delayed_14),
.a15_data_delayed_15(a15_data_delayed_15),
.b15_data_delayed_15(b15_data_delayed_15),

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

assign a0_data_in = a_data_in[1*`DWIDTH-1:0*`DWIDTH];
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

assign b0_data_in = b_data_in[1*`DWIDTH-1:0*`DWIDTH];
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

wire row_latch_en;
//////////////////////////////////////////////////////////////////////////
// Instantiation of the output logic
//////////////////////////////////////////////////////////////////////////
output_logic u_output_logic(
.start_mat_mul(start_mat_mul),
.done_mat_mul(done_mat_mul),
.address_mat_c(address_mat_c),
.address_stride_c(address_stride_c),
.c_data_out(c_data_out),
.c_data_in(c_data_in),
.c_addr(c_addr),
.c_data_available(c_data_available),
.clk_cnt(clk_cnt),
.row_latch_en(row_latch_en),

.final_mat_mul_size(final_mat_mul_size),
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

.clk(clk),
.reset(reset)
);

//////////////////////////////////////////////////////////////////////////
// Instantiations of the actual PEs
//////////////////////////////////////////////////////////////////////////
systolic_pe_matrix u_systolic_pe_matrix(
.clk(clk),
.reset(reset),
.pe_reset(pe_reset),
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

endmodule


//////////////////////////////////////////////////////////////////////////
// Output logic
//////////////////////////////////////////////////////////////////////////
module output_logic(
start_mat_mul,
done_mat_mul,
address_mat_c,
address_stride_c,
c_data_in,
c_data_out, //Data values going out to next matmul - systolic shifting
c_addr,
c_data_available,
clk_cnt,
row_latch_en,

final_mat_mul_size,
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

clk,
reset
);

input clk;
input reset;
input start_mat_mul;
input done_mat_mul;
input [`AWIDTH-1:0] address_mat_c;
input [`ADDR_STRIDE_WIDTH-1:0] address_stride_c;
input [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_in;
output [`MAT_MUL_SIZE*`DWIDTH-1:0] c_data_out;
output [`AWIDTH-1:0] c_addr;
output c_data_available;
input [7:0] clk_cnt;
output row_latch_en;

input [7:0] final_mat_mul_size;
  input [`DWIDTH-1:0] matrixC0_0;
input [`DWIDTH-1:0] matrixC0_1;
input [`DWIDTH-1:0] matrixC0_2;
input [`DWIDTH-1:0] matrixC0_3;
input [`DWIDTH-1:0] matrixC0_4;
input [`DWIDTH-1:0] matrixC0_5;
input [`DWIDTH-1:0] matrixC0_6;
input [`DWIDTH-1:0] matrixC0_7;
input [`DWIDTH-1:0] matrixC0_8;
input [`DWIDTH-1:0] matrixC0_9;
input [`DWIDTH-1:0] matrixC0_10;
input [`DWIDTH-1:0] matrixC0_11;
input [`DWIDTH-1:0] matrixC0_12;
input [`DWIDTH-1:0] matrixC0_13;
input [`DWIDTH-1:0] matrixC0_14;
input [`DWIDTH-1:0] matrixC0_15;
input [`DWIDTH-1:0] matrixC1_0;
input [`DWIDTH-1:0] matrixC1_1;
input [`DWIDTH-1:0] matrixC1_2;
input [`DWIDTH-1:0] matrixC1_3;
input [`DWIDTH-1:0] matrixC1_4;
input [`DWIDTH-1:0] matrixC1_5;
input [`DWIDTH-1:0] matrixC1_6;
input [`DWIDTH-1:0] matrixC1_7;
input [`DWIDTH-1:0] matrixC1_8;
input [`DWIDTH-1:0] matrixC1_9;
input [`DWIDTH-1:0] matrixC1_10;
input [`DWIDTH-1:0] matrixC1_11;
input [`DWIDTH-1:0] matrixC1_12;
input [`DWIDTH-1:0] matrixC1_13;
input [`DWIDTH-1:0] matrixC1_14;
input [`DWIDTH-1:0] matrixC1_15;
input [`DWIDTH-1:0] matrixC2_0;
input [`DWIDTH-1:0] matrixC2_1;
input [`DWIDTH-1:0] matrixC2_2;
input [`DWIDTH-1:0] matrixC2_3;
input [`DWIDTH-1:0] matrixC2_4;
input [`DWIDTH-1:0] matrixC2_5;
input [`DWIDTH-1:0] matrixC2_6;
input [`DWIDTH-1:0] matrixC2_7;
input [`DWIDTH-1:0] matrixC2_8;
input [`DWIDTH-1:0] matrixC2_9;
input [`DWIDTH-1:0] matrixC2_10;
input [`DWIDTH-1:0] matrixC2_11;
input [`DWIDTH-1:0] matrixC2_12;
input [`DWIDTH-1:0] matrixC2_13;
input [`DWIDTH-1:0] matrixC2_14;
input [`DWIDTH-1:0] matrixC2_15;
input [`DWIDTH-1:0] matrixC3_0;
input [`DWIDTH-1:0] matrixC3_1;
input [`DWIDTH-1:0] matrixC3_2;
input [`DWIDTH-1:0] matrixC3_3;
input [`DWIDTH-1:0] matrixC3_4;
input [`DWIDTH-1:0] matrixC3_5;
input [`DWIDTH-1:0] matrixC3_6;
input [`DWIDTH-1:0] matrixC3_7;
input [`DWIDTH-1:0] matrixC3_8;
input [`DWIDTH-1:0] matrixC3_9;
input [`DWIDTH-1:0] matrixC3_10;
input [`DWIDTH-1:0] matrixC3_11;
input [`DWIDTH-1:0] matrixC3_12;
input [`DWIDTH-1:0] matrixC3_13;
input [`DWIDTH-1:0] matrixC3_14;
input [`DWIDTH-1:0] matrixC3_15;
input [`DWIDTH-1:0] matrixC4_0;
input [`DWIDTH-1:0] matrixC4_1;
input [`DWIDTH-1:0] matrixC4_2;
input [`DWIDTH-1:0] matrixC4_3;
input [`DWIDTH-1:0] matrixC4_4;
input [`DWIDTH-1:0] matrixC4_5;
input [`DWIDTH-1:0] matrixC4_6;
input [`DWIDTH-1:0] matrixC4_7;
input [`DWIDTH-1:0] matrixC4_8;
input [`DWIDTH-1:0] matrixC4_9;
input [`DWIDTH-1:0] matrixC4_10;
input [`DWIDTH-1:0] matrixC4_11;
input [`DWIDTH-1:0] matrixC4_12;
input [`DWIDTH-1:0] matrixC4_13;
input [`DWIDTH-1:0] matrixC4_14;
input [`DWIDTH-1:0] matrixC4_15;
input [`DWIDTH-1:0] matrixC5_0;
input [`DWIDTH-1:0] matrixC5_1;
input [`DWIDTH-1:0] matrixC5_2;
input [`DWIDTH-1:0] matrixC5_3;
input [`DWIDTH-1:0] matrixC5_4;
input [`DWIDTH-1:0] matrixC5_5;
input [`DWIDTH-1:0] matrixC5_6;
input [`DWIDTH-1:0] matrixC5_7;
input [`DWIDTH-1:0] matrixC5_8;
input [`DWIDTH-1:0] matrixC5_9;
input [`DWIDTH-1:0] matrixC5_10;
input [`DWIDTH-1:0] matrixC5_11;
input [`DWIDTH-1:0] matrixC5_12;
input [`DWIDTH-1:0] matrixC5_13;
input [`DWIDTH-1:0] matrixC5_14;
input [`DWIDTH-1:0] matrixC5_15;
input [`DWIDTH-1:0] matrixC6_0;
input [`DWIDTH-1:0] matrixC6_1;
input [`DWIDTH-1:0] matrixC6_2;
input [`DWIDTH-1:0] matrixC6_3;
input [`DWIDTH-1:0] matrixC6_4;
input [`DWIDTH-1:0] matrixC6_5;
input [`DWIDTH-1:0] matrixC6_6;
input [`DWIDTH-1:0] matrixC6_7;
input [`DWIDTH-1:0] matrixC6_8;
input [`DWIDTH-1:0] matrixC6_9;
input [`DWIDTH-1:0] matrixC6_10;
input [`DWIDTH-1:0] matrixC6_11;
input [`DWIDTH-1:0] matrixC6_12;
input [`DWIDTH-1:0] matrixC6_13;
input [`DWIDTH-1:0] matrixC6_14;
input [`DWIDTH-1:0] matrixC6_15;
input [`DWIDTH-1:0] matrixC7_0;
input [`DWIDTH-1:0] matrixC7_1;
input [`DWIDTH-1:0] matrixC7_2;
input [`DWIDTH-1:0] matrixC7_3;
input [`DWIDTH-1:0] matrixC7_4;
input [`DWIDTH-1:0] matrixC7_5;
input [`DWIDTH-1:0] matrixC7_6;
input [`DWIDTH-1:0] matrixC7_7;
input [`DWIDTH-1:0] matrixC7_8;
input [`DWIDTH-1:0] matrixC7_9;
input [`DWIDTH-1:0] matrixC7_10;
input [`DWIDTH-1:0] matrixC7_11;
input [`DWIDTH-1:0] matrixC7_12;
input [`DWIDTH-1:0] matrixC7_13;
input [`DWIDTH-1:0] matrixC7_14;
input [`DWIDTH-1:0] matrixC7_15;
input [`DWIDTH-1:0] matrixC8_0;
input [`DWIDTH-1:0] matrixC8_1;
input [`DWIDTH-1:0] matrixC8_2;
input [`DWIDTH-1:0] matrixC8_3;
input [`DWIDTH-1:0] matrixC8_4;
input [`DWIDTH-1:0] matrixC8_5;
input [`DWIDTH-1:0] matrixC8_6;
input [`DWIDTH-1:0] matrixC8_7;
input [`DWIDTH-1:0] matrixC8_8;
input [`DWIDTH-1:0] matrixC8_9;
input [`DWIDTH-1:0] matrixC8_10;
input [`DWIDTH-1:0] matrixC8_11;
input [`DWIDTH-1:0] matrixC8_12;
input [`DWIDTH-1:0] matrixC8_13;
input [`DWIDTH-1:0] matrixC8_14;
input [`DWIDTH-1:0] matrixC8_15;
input [`DWIDTH-1:0] matrixC9_0;
input [`DWIDTH-1:0] matrixC9_1;
input [`DWIDTH-1:0] matrixC9_2;
input [`DWIDTH-1:0] matrixC9_3;
input [`DWIDTH-1:0] matrixC9_4;
input [`DWIDTH-1:0] matrixC9_5;
input [`DWIDTH-1:0] matrixC9_6;
input [`DWIDTH-1:0] matrixC9_7;
input [`DWIDTH-1:0] matrixC9_8;
input [`DWIDTH-1:0] matrixC9_9;
input [`DWIDTH-1:0] matrixC9_10;
input [`DWIDTH-1:0] matrixC9_11;
input [`DWIDTH-1:0] matrixC9_12;
input [`DWIDTH-1:0] matrixC9_13;
input [`DWIDTH-1:0] matrixC9_14;
input [`DWIDTH-1:0] matrixC9_15;
input [`DWIDTH-1:0] matrixC10_0;
input [`DWIDTH-1:0] matrixC10_1;
input [`DWIDTH-1:0] matrixC10_2;
input [`DWIDTH-1:0] matrixC10_3;
input [`DWIDTH-1:0] matrixC10_4;
input [`DWIDTH-1:0] matrixC10_5;
input [`DWIDTH-1:0] matrixC10_6;
input [`DWIDTH-1:0] matrixC10_7;
input [`DWIDTH-1:0] matrixC10_8;
input [`DWIDTH-1:0] matrixC10_9;
input [`DWIDTH-1:0] matrixC10_10;
input [`DWIDTH-1:0] matrixC10_11;
input [`DWIDTH-1:0] matrixC10_12;
input [`DWIDTH-1:0] matrixC10_13;
input [`DWIDTH-1:0] matrixC10_14;
input [`DWIDTH-1:0] matrixC10_15;
input [`DWIDTH-1:0] matrixC11_0;
input [`DWIDTH-1:0] matrixC11_1;
input [`DWIDTH-1:0] matrixC11_2;
input [`DWIDTH-1:0] matrixC11_3;
input [`DWIDTH-1:0] matrixC11_4;
input [`DWIDTH-1:0] matrixC11_5;
input [`DWIDTH-1:0] matrixC11_6;
input [`DWIDTH-1:0] matrixC11_7;
input [`DWIDTH-1:0] matrixC11_8;
input [`DWIDTH-1:0] matrixC11_9;
input [`DWIDTH-1:0] matrixC11_10;
input [`DWIDTH-1:0] matrixC11_11;
input [`DWIDTH-1:0] matrixC11_12;
input [`DWIDTH-1:0] matrixC11_13;
input [`DWIDTH-1:0] matrixC11_14;
input [`DWIDTH-1:0] matrixC11_15;
input [`DWIDTH-1:0] matrixC12_0;
input [`DWIDTH-1:0] matrixC12_1;
input [`DWIDTH-1:0] matrixC12_2;
input [`DWIDTH-1:0] matrixC12_3;
input [`DWIDTH-1:0] matrixC12_4;
input [`DWIDTH-1:0] matrixC12_5;
input [`DWIDTH-1:0] matrixC12_6;
input [`DWIDTH-1:0] matrixC12_7;
input [`DWIDTH-1:0] matrixC12_8;
input [`DWIDTH-1:0] matrixC12_9;
input [`DWIDTH-1:0] matrixC12_10;
input [`DWIDTH-1:0] matrixC12_11;
input [`DWIDTH-1:0] matrixC12_12;
input [`DWIDTH-1:0] matrixC12_13;
input [`DWIDTH-1:0] matrixC12_14;
input [`DWIDTH-1:0] matrixC12_15;
input [`DWIDTH-1:0] matrixC13_0;
input [`DWIDTH-1:0] matrixC13_1;
input [`DWIDTH-1:0] matrixC13_2;
input [`DWIDTH-1:0] matrixC13_3;
input [`DWIDTH-1:0] matrixC13_4;
input [`DWIDTH-1:0] matrixC13_5;
input [`DWIDTH-1:0] matrixC13_6;
input [`DWIDTH-1:0] matrixC13_7;
input [`DWIDTH-1:0] matrixC13_8;
input [`DWIDTH-1:0] matrixC13_9;
input [`DWIDTH-1:0] matrixC13_10;
input [`DWIDTH-1:0] matrixC13_11;
input [`DWIDTH-1:0] matrixC13_12;
input [`DWIDTH-1:0] matrixC13_13;
input [`DWIDTH-1:0] matrixC13_14;
input [`DWIDTH-1:0] matrixC13_15;
input [`DWIDTH-1:0] matrixC14_0;
input [`DWIDTH-1:0] matrixC14_1;
input [`DWIDTH-1:0] matrixC14_2;
input [`DWIDTH-1:0] matrixC14_3;
input [`DWIDTH-1:0] matrixC14_4;
input [`DWIDTH-1:0] matrixC14_5;
input [`DWIDTH-1:0] matrixC14_6;
input [`DWIDTH-1:0] matrixC14_7;
input [`DWIDTH-1:0] matrixC14_8;
input [`DWIDTH-1:0] matrixC14_9;
input [`DWIDTH-1:0] matrixC14_10;
input [`DWIDTH-1:0] matrixC14_11;
input [`DWIDTH-1:0] matrixC14_12;
input [`DWIDTH-1:0] matrixC14_13;
input [`DWIDTH-1:0] matrixC14_14;
input [`DWIDTH-1:0] matrixC14_15;
input [`DWIDTH-1:0] matrixC15_0;
input [`DWIDTH-1:0] matrixC15_1;
input [`DWIDTH-1:0] matrixC15_2;
input [`DWIDTH-1:0] matrixC15_3;
input [`DWIDTH-1:0] matrixC15_4;
input [`DWIDTH-1:0] matrixC15_5;
input [`DWIDTH-1:0] matrixC15_6;
input [`DWIDTH-1:0] matrixC15_7;
input [`DWIDTH-1:0] matrixC15_8;
input [`DWIDTH-1:0] matrixC15_9;
input [`DWIDTH-1:0] matrixC15_10;
input [`DWIDTH-1:0] matrixC15_11;
input [`DWIDTH-1:0] matrixC15_12;
input [`DWIDTH-1:0] matrixC15_13;
input [`DWIDTH-1:0] matrixC15_14;
input [`DWIDTH-1:0] matrixC15_15;
wire row_latch_en;


//////////////////////////////////////////////////////////////////////////
// Logic to capture matrix C data from the PEs and shift it out
//////////////////////////////////////////////////////////////////////////
//assign row_latch_en = (clk_cnt==(`MAT_MUL_SIZE + (a_loc+b_loc) * `BB_MAT_MUL_SIZE + 10 +  `NUM_CYCLES_IN_MAC - 1));
//Writing the line above to avoid multiplication:
//assign row_latch_en = (clk_cnt==(`MAT_MUL_SIZE + ((a_loc+b_loc) << `LOG2_MAT_MUL_SIZE) + 10 +  `NUM_CYCLES_IN_MAC - 1));

assign row_latch_en =  
                       ((clk_cnt == ((final_mat_mul_size<<2) - final_mat_mul_size - 1 +`NUM_CYCLES_IN_MAC)));
    
reg c_data_available;
reg [`AWIDTH-1:0] c_addr;
reg start_capturing_c_data;
integer counter;
reg [16*`DWIDTH-1:0] c_data_out;
reg [16*`DWIDTH-1:0] c_data_out_1;
reg [16*`DWIDTH-1:0] c_data_out_2;
reg [16*`DWIDTH-1:0] c_data_out_3;
reg [16*`DWIDTH-1:0] c_data_out_4;
reg [16*`DWIDTH-1:0] c_data_out_5;
reg [16*`DWIDTH-1:0] c_data_out_6;
reg [16*`DWIDTH-1:0] c_data_out_7;
reg [16*`DWIDTH-1:0] c_data_out_8;
reg [16*`DWIDTH-1:0] c_data_out_9;
reg [16*`DWIDTH-1:0] c_data_out_10;
reg [16*`DWIDTH-1:0] c_data_out_11;
reg [16*`DWIDTH-1:0] c_data_out_12;
reg [16*`DWIDTH-1:0] c_data_out_13;
reg [16*`DWIDTH-1:0] c_data_out_14;
reg [16*`DWIDTH-1:0] c_data_out_15;
wire condition_to_start_shifting_output;
assign condition_to_start_shifting_output = 
                          row_latch_en ;  

  
//For larger matmuls, this logic will have more entries in the case statement
always @(posedge clk) begin
  if (reset | ~start_mat_mul) begin
    start_capturing_c_data <= 1'b0;
    c_data_available <= 1'b0;
    c_addr <= address_mat_c + address_stride_c;
    c_data_out <= 0;
    counter <= 0;

    c_data_out_1 <= 0;
    c_data_out_2 <= 0;
    c_data_out_3 <= 0;
    c_data_out_4 <= 0;
    c_data_out_5 <= 0;
    c_data_out_6 <= 0;
    c_data_out_7 <= 0;
    c_data_out_8 <= 0;
    c_data_out_9 <= 0;
    c_data_out_10 <= 0;
    c_data_out_11 <= 0;
    c_data_out_12 <= 0;
    c_data_out_13 <= 0;
    c_data_out_14 <= 0;
    c_data_out_15 <= 0;
  end else if (condition_to_start_shifting_output) begin
    start_capturing_c_data <= 1'b1;
    c_data_available <= 1'b1;
    c_addr <= c_addr - address_stride_c;
    c_data_out <= {matrixC15_15, matrixC14_15, matrixC13_15, matrixC12_15, matrixC11_15, matrixC10_15, matrixC9_15, matrixC8_15, matrixC7_15, matrixC6_15, matrixC5_15, matrixC4_15, matrixC3_15, matrixC2_15, matrixC1_15, matrixC0_15};
      c_data_out_1 <= {matrixC15_14, matrixC14_14, matrixC13_14, matrixC12_14, matrixC11_14, matrixC10_14, matrixC9_14, matrixC8_14, matrixC7_14, matrixC6_14, matrixC5_14, matrixC4_14, matrixC3_14, matrixC2_14, matrixC1_14, matrixC0_14};
      c_data_out_2 <= {matrixC15_13, matrixC14_13, matrixC13_13, matrixC12_13, matrixC11_13, matrixC10_13, matrixC9_13, matrixC8_13, matrixC7_13, matrixC6_13, matrixC5_13, matrixC4_13, matrixC3_13, matrixC2_13, matrixC1_13, matrixC0_13};
      c_data_out_3 <= {matrixC15_12, matrixC14_12, matrixC13_12, matrixC12_12, matrixC11_12, matrixC10_12, matrixC9_12, matrixC8_12, matrixC7_12, matrixC6_12, matrixC5_12, matrixC4_12, matrixC3_12, matrixC2_12, matrixC1_12, matrixC0_12};
      c_data_out_4 <= {matrixC15_11, matrixC14_11, matrixC13_11, matrixC12_11, matrixC11_11, matrixC10_11, matrixC9_11, matrixC8_11, matrixC7_11, matrixC6_11, matrixC5_11, matrixC4_11, matrixC3_11, matrixC2_11, matrixC1_11, matrixC0_11};
      c_data_out_5 <= {matrixC15_10, matrixC14_10, matrixC13_10, matrixC12_10, matrixC11_10, matrixC10_10, matrixC9_10, matrixC8_10, matrixC7_10, matrixC6_10, matrixC5_10, matrixC4_10, matrixC3_10, matrixC2_10, matrixC1_10, matrixC0_10};
      c_data_out_6 <= {matrixC15_9, matrixC14_9, matrixC13_9, matrixC12_9, matrixC11_9, matrixC10_9, matrixC9_9, matrixC8_9, matrixC7_9, matrixC6_9, matrixC5_9, matrixC4_9, matrixC3_9, matrixC2_9, matrixC1_9, matrixC0_9};
      c_data_out_7 <= {matrixC15_8, matrixC14_8, matrixC13_8, matrixC12_8, matrixC11_8, matrixC10_8, matrixC9_8, matrixC8_8, matrixC7_8, matrixC6_8, matrixC5_8, matrixC4_8, matrixC3_8, matrixC2_8, matrixC1_8, matrixC0_8};
      c_data_out_8 <= {matrixC15_7, matrixC14_7, matrixC13_7, matrixC12_7, matrixC11_7, matrixC10_7, matrixC9_7, matrixC8_7, matrixC7_7, matrixC6_7, matrixC5_7, matrixC4_7, matrixC3_7, matrixC2_7, matrixC1_7, matrixC0_7};
      c_data_out_9 <= {matrixC15_6, matrixC14_6, matrixC13_6, matrixC12_6, matrixC11_6, matrixC10_6, matrixC9_6, matrixC8_6, matrixC7_6, matrixC6_6, matrixC5_6, matrixC4_6, matrixC3_6, matrixC2_6, matrixC1_6, matrixC0_6};
      c_data_out_10 <= {matrixC15_5, matrixC14_5, matrixC13_5, matrixC12_5, matrixC11_5, matrixC10_5, matrixC9_5, matrixC8_5, matrixC7_5, matrixC6_5, matrixC5_5, matrixC4_5, matrixC3_5, matrixC2_5, matrixC1_5, matrixC0_5};
      c_data_out_11 <= {matrixC15_4, matrixC14_4, matrixC13_4, matrixC12_4, matrixC11_4, matrixC10_4, matrixC9_4, matrixC8_4, matrixC7_4, matrixC6_4, matrixC5_4, matrixC4_4, matrixC3_4, matrixC2_4, matrixC1_4, matrixC0_4};
      c_data_out_12 <= {matrixC15_3, matrixC14_3, matrixC13_3, matrixC12_3, matrixC11_3, matrixC10_3, matrixC9_3, matrixC8_3, matrixC7_3, matrixC6_3, matrixC5_3, matrixC4_3, matrixC3_3, matrixC2_3, matrixC1_3, matrixC0_3};
      c_data_out_13 <= {matrixC15_2, matrixC14_2, matrixC13_2, matrixC12_2, matrixC11_2, matrixC10_2, matrixC9_2, matrixC8_2, matrixC7_2, matrixC6_2, matrixC5_2, matrixC4_2, matrixC3_2, matrixC2_2, matrixC1_2, matrixC0_2};
      c_data_out_14 <= {matrixC15_1, matrixC14_1, matrixC13_1, matrixC12_1, matrixC11_1, matrixC10_1, matrixC9_1, matrixC8_1, matrixC7_1, matrixC6_1, matrixC5_1, matrixC4_1, matrixC3_1, matrixC2_1, matrixC1_1, matrixC0_1};
      c_data_out_15 <= {matrixC15_0, matrixC14_0, matrixC13_0, matrixC12_0, matrixC11_0, matrixC10_0, matrixC9_0, matrixC8_0, matrixC7_0, matrixC6_0, matrixC5_0, matrixC4_0, matrixC3_0, matrixC2_0, matrixC1_0, matrixC0_0};

    counter <= counter + 1;
  end else if (done_mat_mul) begin
    start_capturing_c_data <= 1'b0;
    c_data_available <= 1'b0;
    c_addr <= address_mat_c + address_stride_c;
    c_data_out <= 0;

    c_data_out_1 <= 0;
    c_data_out_2 <= 0;
    c_data_out_3 <= 0;
    c_data_out_4 <= 0;
    c_data_out_5 <= 0;
    c_data_out_6 <= 0;
    c_data_out_7 <= 0;
    c_data_out_8 <= 0;
    c_data_out_9 <= 0;
    c_data_out_10 <= 0;
    c_data_out_11 <= 0;
    c_data_out_12 <= 0;
    c_data_out_13 <= 0;
    c_data_out_14 <= 0;
    c_data_out_15 <= 0;
  end 
  else if (counter >= `MAT_MUL_SIZE) begin
    c_data_out <= c_data_out_1;
    c_addr <= c_addr - address_stride_c; 

    c_data_out_1 <= c_data_out_2;
    c_data_out_2 <= c_data_out_3;
    c_data_out_3 <= c_data_out_4;
    c_data_out_4 <= c_data_out_5;
    c_data_out_5 <= c_data_out_6;
    c_data_out_6 <= c_data_out_7;
    c_data_out_7 <= c_data_out_8;
    c_data_out_8 <= c_data_out_9;
    c_data_out_9 <= c_data_out_10;
    c_data_out_10 <= c_data_out_11;
    c_data_out_11 <= c_data_out_12;
    c_data_out_12 <= c_data_out_13;
    c_data_out_13 <= c_data_out_14;
    c_data_out_14 <= c_data_out_15;
    c_data_out_15 <= c_data_in;
  end
  else if (start_capturing_c_data) begin
    c_data_available <= 1'b1;
    c_addr <= c_addr - address_stride_c; 
    counter <= counter + 1;
    c_data_out <= c_data_out_1;

    c_data_out_1 <= c_data_out_2;
    c_data_out_2 <= c_data_out_3;
    c_data_out_3 <= c_data_out_4;
    c_data_out_4 <= c_data_out_5;
    c_data_out_5 <= c_data_out_6;
    c_data_out_6 <= c_data_out_7;
    c_data_out_7 <= c_data_out_8;
    c_data_out_8 <= c_data_out_9;
    c_data_out_9 <= c_data_out_10;
    c_data_out_10 <= c_data_out_11;
    c_data_out_11 <= c_data_out_12;
    c_data_out_12 <= c_data_out_13;
    c_data_out_13 <= c_data_out_14;
    c_data_out_14 <= c_data_out_15;
    c_data_out_15 <= c_data_in;
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
b0_data,
a1_data_delayed_1,
b1_data_delayed_1,
a2_data_delayed_2,
b2_data_delayed_2,
a3_data_delayed_3,
b3_data_delayed_3,
a4_data_delayed_4,
b4_data_delayed_4,
a5_data_delayed_5,
b5_data_delayed_5,
a6_data_delayed_6,
b6_data_delayed_6,
a7_data_delayed_7,
b7_data_delayed_7,
a8_data_delayed_8,
b8_data_delayed_8,
a9_data_delayed_9,
b9_data_delayed_9,
a10_data_delayed_10,
b10_data_delayed_10,
a11_data_delayed_11,
b11_data_delayed_11,
a12_data_delayed_12,
b12_data_delayed_12,
a13_data_delayed_13,
b13_data_delayed_13,
a14_data_delayed_14,
b14_data_delayed_14,
a15_data_delayed_15,
b15_data_delayed_15,

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
output [`DWIDTH-1:0] b0_data;
output [`DWIDTH-1:0] a1_data_delayed_1;
output [`DWIDTH-1:0] b1_data_delayed_1;
output [`DWIDTH-1:0] a2_data_delayed_2;
output [`DWIDTH-1:0] b2_data_delayed_2;
output [`DWIDTH-1:0] a3_data_delayed_3;
output [`DWIDTH-1:0] b3_data_delayed_3;
output [`DWIDTH-1:0] a4_data_delayed_4;
output [`DWIDTH-1:0] b4_data_delayed_4;
output [`DWIDTH-1:0] a5_data_delayed_5;
output [`DWIDTH-1:0] b5_data_delayed_5;
output [`DWIDTH-1:0] a6_data_delayed_6;
output [`DWIDTH-1:0] b6_data_delayed_6;
output [`DWIDTH-1:0] a7_data_delayed_7;
output [`DWIDTH-1:0] b7_data_delayed_7;
output [`DWIDTH-1:0] a8_data_delayed_8;
output [`DWIDTH-1:0] b8_data_delayed_8;
output [`DWIDTH-1:0] a9_data_delayed_9;
output [`DWIDTH-1:0] b9_data_delayed_9;
output [`DWIDTH-1:0] a10_data_delayed_10;
output [`DWIDTH-1:0] b10_data_delayed_10;
output [`DWIDTH-1:0] a11_data_delayed_11;
output [`DWIDTH-1:0] b11_data_delayed_11;
output [`DWIDTH-1:0] a12_data_delayed_12;
output [`DWIDTH-1:0] b12_data_delayed_12;
output [`DWIDTH-1:0] a13_data_delayed_13;
output [`DWIDTH-1:0] b13_data_delayed_13;
output [`DWIDTH-1:0] a14_data_delayed_14;
output [`DWIDTH-1:0] b14_data_delayed_14;
output [`DWIDTH-1:0] a15_data_delayed_15;
output [`DWIDTH-1:0] b15_data_delayed_15;

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

//////////////////////////////////////////////////////////////////////////
// Logic to generate addresses to BRAM A
//////////////////////////////////////////////////////////////////////////
reg [`AWIDTH-1:0] a_addr;
reg a_mem_access; //flag that tells whether the matmul is trying to access memory or not

always @(posedge clk) begin
  //(clk_cnt >= a_loc*`MAT_MUL_SIZE+final_mat_mul_size) begin
  //Writing the line above to avoid multiplication:

  if (reset || ~start_mat_mul || (clk_cnt >= (a_loc<<`LOG2_MAT_MUL_SIZE)+final_mat_mul_size)) begin
  
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
      (validity_mask_a_cols[3]==1'b0 && a_mem_access_counter==4) ||
      (validity_mask_a_cols[4]==1'b0 && a_mem_access_counter==5) ||
      (validity_mask_a_cols[5]==1'b0 && a_mem_access_counter==6) ||
      (validity_mask_a_cols[6]==1'b0 && a_mem_access_counter==7) ||
      (validity_mask_a_cols[7]==1'b0 && a_mem_access_counter==8) ||
      (validity_mask_a_cols[8]==1'b0 && a_mem_access_counter==9) ||
      (validity_mask_a_cols[9]==1'b0 && a_mem_access_counter==10) ||
      (validity_mask_a_cols[10]==1'b0 && a_mem_access_counter==11) ||
      (validity_mask_a_cols[11]==1'b0 && a_mem_access_counter==12) ||
      (validity_mask_a_cols[12]==1'b0 && a_mem_access_counter==13) ||
      (validity_mask_a_cols[13]==1'b0 && a_mem_access_counter==14) ||
      (validity_mask_a_cols[14]==1'b0 && a_mem_access_counter==15) ||
      (validity_mask_a_cols[15]==1'b0 && a_mem_access_counter==16)) ?
    
    1'b0 : (a_mem_access_counter >= `MEM_ACCESS_LATENCY);

//////////////////////////////////////////////////////////////////////////
// Logic to delay certain parts of the data received from BRAM A (systolic data setup)
//////////////////////////////////////////////////////////////////////////
assign a0_data = a_data[1*`DWIDTH-1:0*`DWIDTH] & {`DWIDTH{a_data_valid}} & {`DWIDTH{validity_mask_a_rows[0]}};
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
  a3_data_delayed_1 <= a3_data;
  a4_data_delayed_1 <= a4_data;
  a5_data_delayed_1 <= a5_data;
  a6_data_delayed_1 <= a6_data;
  a7_data_delayed_1 <= a7_data;
  a8_data_delayed_1 <= a8_data;
  a9_data_delayed_1 <= a9_data;
  a10_data_delayed_1 <= a10_data;
  a11_data_delayed_1 <= a11_data;
  a12_data_delayed_1 <= a12_data;
  a13_data_delayed_1 <= a13_data;
  a14_data_delayed_1 <= a14_data;
  a15_data_delayed_1 <= a15_data;
  a2_data_delayed_2 <= a2_data_delayed_1;
  a3_data_delayed_2 <= a3_data_delayed_1;
  a3_data_delayed_3 <= a3_data_delayed_2;
  a4_data_delayed_2 <= a4_data_delayed_1;
  a4_data_delayed_3 <= a4_data_delayed_2;
  a4_data_delayed_4 <= a4_data_delayed_3;
  a5_data_delayed_2 <= a5_data_delayed_1;
  a5_data_delayed_3 <= a5_data_delayed_2;
  a5_data_delayed_4 <= a5_data_delayed_3;
  a5_data_delayed_5 <= a5_data_delayed_4;
  a6_data_delayed_2 <= a6_data_delayed_1;
  a6_data_delayed_3 <= a6_data_delayed_2;
  a6_data_delayed_4 <= a6_data_delayed_3;
  a6_data_delayed_5 <= a6_data_delayed_4;
  a6_data_delayed_6 <= a6_data_delayed_5;
  a7_data_delayed_2 <= a7_data_delayed_1;
  a7_data_delayed_3 <= a7_data_delayed_2;
  a7_data_delayed_4 <= a7_data_delayed_3;
  a7_data_delayed_5 <= a7_data_delayed_4;
  a7_data_delayed_6 <= a7_data_delayed_5;
  a7_data_delayed_7 <= a7_data_delayed_6;
  a8_data_delayed_2 <= a8_data_delayed_1;
  a8_data_delayed_3 <= a8_data_delayed_2;
  a8_data_delayed_4 <= a8_data_delayed_3;
  a8_data_delayed_5 <= a8_data_delayed_4;
  a8_data_delayed_6 <= a8_data_delayed_5;
  a8_data_delayed_7 <= a8_data_delayed_6;
  a8_data_delayed_8 <= a8_data_delayed_7;
  a9_data_delayed_2 <= a9_data_delayed_1;
  a9_data_delayed_3 <= a9_data_delayed_2;
  a9_data_delayed_4 <= a9_data_delayed_3;
  a9_data_delayed_5 <= a9_data_delayed_4;
  a9_data_delayed_6 <= a9_data_delayed_5;
  a9_data_delayed_7 <= a9_data_delayed_6;
  a9_data_delayed_8 <= a9_data_delayed_7;
  a9_data_delayed_9 <= a9_data_delayed_8;
  a10_data_delayed_2 <= a10_data_delayed_1;
  a10_data_delayed_3 <= a10_data_delayed_2;
  a10_data_delayed_4 <= a10_data_delayed_3;
  a10_data_delayed_5 <= a10_data_delayed_4;
  a10_data_delayed_6 <= a10_data_delayed_5;
  a10_data_delayed_7 <= a10_data_delayed_6;
  a10_data_delayed_8 <= a10_data_delayed_7;
  a10_data_delayed_9 <= a10_data_delayed_8;
  a10_data_delayed_10 <= a10_data_delayed_9;
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
      (validity_mask_b_rows[3]==1'b0 && b_mem_access_counter==4) ||
      (validity_mask_b_rows[4]==1'b0 && b_mem_access_counter==5) ||
      (validity_mask_b_rows[5]==1'b0 && b_mem_access_counter==6) ||
      (validity_mask_b_rows[6]==1'b0 && b_mem_access_counter==7) ||
      (validity_mask_b_rows[7]==1'b0 && b_mem_access_counter==8) ||
      (validity_mask_b_rows[8]==1'b0 && b_mem_access_counter==9) ||
      (validity_mask_b_rows[9]==1'b0 && b_mem_access_counter==10) ||
      (validity_mask_b_rows[10]==1'b0 && b_mem_access_counter==11) ||
      (validity_mask_b_rows[11]==1'b0 && b_mem_access_counter==12) ||
      (validity_mask_b_rows[12]==1'b0 && b_mem_access_counter==13) ||
      (validity_mask_b_rows[13]==1'b0 && b_mem_access_counter==14) ||
      (validity_mask_b_rows[14]==1'b0 && b_mem_access_counter==15) ||
      (validity_mask_b_rows[15]==1'b0 && b_mem_access_counter==16)) ?
    
        1'b0 : (b_mem_access_counter >= `MEM_ACCESS_LATENCY);

//////////////////////////////////////////////////////////////////////////
// Logic to delay certain parts of the data received from BRAM B (systolic data setup)
//////////////////////////////////////////////////////////////////////////
assign b0_data = b_data[1*`DWIDTH-1:0*`DWIDTH] & {`DWIDTH{b_data_valid}} & {`DWIDTH{validity_mask_b_cols[0]}};
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
  b3_data_delayed_1 <= b3_data;
  b4_data_delayed_1 <= b4_data;
  b5_data_delayed_1 <= b5_data;
  b6_data_delayed_1 <= b6_data;
  b7_data_delayed_1 <= b7_data;
  b8_data_delayed_1 <= b8_data;
  b9_data_delayed_1 <= b9_data;
  b10_data_delayed_1 <= b10_data;
  b11_data_delayed_1 <= b11_data;
  b12_data_delayed_1 <= b12_data;
  b13_data_delayed_1 <= b13_data;
  b14_data_delayed_1 <= b14_data;
  b15_data_delayed_1 <= b15_data;
  b2_data_delayed_2 <= b2_data_delayed_1;
  b3_data_delayed_2 <= b3_data_delayed_1;
  b3_data_delayed_3 <= b3_data_delayed_2;
  b4_data_delayed_2 <= b4_data_delayed_1;
  b4_data_delayed_3 <= b4_data_delayed_2;
  b4_data_delayed_4 <= b4_data_delayed_3;
  b5_data_delayed_2 <= b5_data_delayed_1;
  b5_data_delayed_3 <= b5_data_delayed_2;
  b5_data_delayed_4 <= b5_data_delayed_3;
  b5_data_delayed_5 <= b5_data_delayed_4;
  b6_data_delayed_2 <= b6_data_delayed_1;
  b6_data_delayed_3 <= b6_data_delayed_2;
  b6_data_delayed_4 <= b6_data_delayed_3;
  b6_data_delayed_5 <= b6_data_delayed_4;
  b6_data_delayed_6 <= b6_data_delayed_5;
  b7_data_delayed_2 <= b7_data_delayed_1;
  b7_data_delayed_3 <= b7_data_delayed_2;
  b7_data_delayed_4 <= b7_data_delayed_3;
  b7_data_delayed_5 <= b7_data_delayed_4;
  b7_data_delayed_6 <= b7_data_delayed_5;
  b7_data_delayed_7 <= b7_data_delayed_6;
  b8_data_delayed_2 <= b8_data_delayed_1;
  b8_data_delayed_3 <= b8_data_delayed_2;
  b8_data_delayed_4 <= b8_data_delayed_3;
  b8_data_delayed_5 <= b8_data_delayed_4;
  b8_data_delayed_6 <= b8_data_delayed_5;
  b8_data_delayed_7 <= b8_data_delayed_6;
  b8_data_delayed_8 <= b8_data_delayed_7;
  b9_data_delayed_2 <= b9_data_delayed_1;
  b9_data_delayed_3 <= b9_data_delayed_2;
  b9_data_delayed_4 <= b9_data_delayed_3;
  b9_data_delayed_5 <= b9_data_delayed_4;
  b9_data_delayed_6 <= b9_data_delayed_5;
  b9_data_delayed_7 <= b9_data_delayed_6;
  b9_data_delayed_8 <= b9_data_delayed_7;
  b9_data_delayed_9 <= b9_data_delayed_8;
  b10_data_delayed_2 <= b10_data_delayed_1;
  b10_data_delayed_3 <= b10_data_delayed_2;
  b10_data_delayed_4 <= b10_data_delayed_3;
  b10_data_delayed_5 <= b10_data_delayed_4;
  b10_data_delayed_6 <= b10_data_delayed_5;
  b10_data_delayed_7 <= b10_data_delayed_6;
  b10_data_delayed_8 <= b10_data_delayed_7;
  b10_data_delayed_9 <= b10_data_delayed_8;
  b10_data_delayed_10 <= b10_data_delayed_9;
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
clk,
reset,
pe_reset,
a0,
a1,
a2,
a3,
a4,
a5,
a6,
a7,
a8,
a9,
a10,
a11,
a12,
a13,
a14,
a15,
b0,
b1,
b2,
b3,
b4,
b5,
b6,
b7,
b8,
b9,
b10,
b11,
b12,
b13,
b14,
b15,
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
b_data_out
);

input clk;
input reset;
input pe_reset;
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

//////////////////////////////////////////////////////////////////////////
// Instantiations of the actual PEs
//////////////////////////////////////////////////////////////////////////
//For larger matmul, more PEs will be needed
wire effective_rst;
assign effective_rst = reset | pe_reset;

processing_element pe0_0(.reset(effective_rst), .clk(clk),  .in_a(a0),      .in_b(b0),  .out_a(a0_0to0_1), .out_b(b0_0to1_0), .out_c(matrixC0_0));
processing_element pe0_1(.reset(effective_rst), .clk(clk),  .in_a(a0_0to0_1), .in_b(b1),  .out_a(a0_1to0_2), .out_b(b0_1to1_1), .out_c(matrixC0_1));
processing_element pe0_2(.reset(effective_rst), .clk(clk),  .in_a(a0_1to0_2), .in_b(b2),  .out_a(a0_2to0_3), .out_b(b0_2to1_2), .out_c(matrixC0_2));
processing_element pe0_3(.reset(effective_rst), .clk(clk),  .in_a(a0_2to0_3), .in_b(b3),  .out_a(a0_3to0_4), .out_b(b0_3to1_3), .out_c(matrixC0_3));
processing_element pe0_4(.reset(effective_rst), .clk(clk),  .in_a(a0_3to0_4), .in_b(b4),  .out_a(a0_4to0_5), .out_b(b0_4to1_4), .out_c(matrixC0_4));
processing_element pe0_5(.reset(effective_rst), .clk(clk),  .in_a(a0_4to0_5), .in_b(b5),  .out_a(a0_5to0_6), .out_b(b0_5to1_5), .out_c(matrixC0_5));
processing_element pe0_6(.reset(effective_rst), .clk(clk),  .in_a(a0_5to0_6), .in_b(b6),  .out_a(a0_6to0_7), .out_b(b0_6to1_6), .out_c(matrixC0_6));
processing_element pe0_7(.reset(effective_rst), .clk(clk),  .in_a(a0_6to0_7), .in_b(b7),  .out_a(a0_7to0_8), .out_b(b0_7to1_7), .out_c(matrixC0_7));
processing_element pe0_8(.reset(effective_rst), .clk(clk),  .in_a(a0_7to0_8), .in_b(b8),  .out_a(a0_8to0_9), .out_b(b0_8to1_8), .out_c(matrixC0_8));
processing_element pe0_9(.reset(effective_rst), .clk(clk),  .in_a(a0_8to0_9), .in_b(b9),  .out_a(a0_9to0_10), .out_b(b0_9to1_9), .out_c(matrixC0_9));
processing_element pe0_10(.reset(effective_rst), .clk(clk),  .in_a(a0_9to0_10), .in_b(b10),  .out_a(a0_10to0_11), .out_b(b0_10to1_10), .out_c(matrixC0_10));
processing_element pe0_11(.reset(effective_rst), .clk(clk),  .in_a(a0_10to0_11), .in_b(b11),  .out_a(a0_11to0_12), .out_b(b0_11to1_11), .out_c(matrixC0_11));
processing_element pe0_12(.reset(effective_rst), .clk(clk),  .in_a(a0_11to0_12), .in_b(b12),  .out_a(a0_12to0_13), .out_b(b0_12to1_12), .out_c(matrixC0_12));
processing_element pe0_13(.reset(effective_rst), .clk(clk),  .in_a(a0_12to0_13), .in_b(b13),  .out_a(a0_13to0_14), .out_b(b0_13to1_13), .out_c(matrixC0_13));
processing_element pe0_14(.reset(effective_rst), .clk(clk),  .in_a(a0_13to0_14), .in_b(b14),  .out_a(a0_14to0_15), .out_b(b0_14to1_14), .out_c(matrixC0_14));
processing_element pe0_15(.reset(effective_rst), .clk(clk),  .in_a(a0_14to0_15), .in_b(b15),  .out_a(a0_15to0_16), .out_b(b0_15to1_15), .out_c(matrixC0_15));

processing_element pe1_0(.reset(effective_rst), .clk(clk),  .in_a(a1), .in_b(b0_0to1_0),  .out_a(a1_0to1_1), .out_b(b1_0to2_0), .out_c(matrixC1_0));
processing_element pe2_0(.reset(effective_rst), .clk(clk),  .in_a(a2), .in_b(b1_0to2_0),  .out_a(a2_0to2_1), .out_b(b2_0to3_0), .out_c(matrixC2_0));
processing_element pe3_0(.reset(effective_rst), .clk(clk),  .in_a(a3), .in_b(b2_0to3_0),  .out_a(a3_0to3_1), .out_b(b3_0to4_0), .out_c(matrixC3_0));
processing_element pe4_0(.reset(effective_rst), .clk(clk),  .in_a(a4), .in_b(b3_0to4_0),  .out_a(a4_0to4_1), .out_b(b4_0to5_0), .out_c(matrixC4_0));
processing_element pe5_0(.reset(effective_rst), .clk(clk),  .in_a(a5), .in_b(b4_0to5_0),  .out_a(a5_0to5_1), .out_b(b5_0to6_0), .out_c(matrixC5_0));
processing_element pe6_0(.reset(effective_rst), .clk(clk),  .in_a(a6), .in_b(b5_0to6_0),  .out_a(a6_0to6_1), .out_b(b6_0to7_0), .out_c(matrixC6_0));
processing_element pe7_0(.reset(effective_rst), .clk(clk),  .in_a(a7), .in_b(b6_0to7_0),  .out_a(a7_0to7_1), .out_b(b7_0to8_0), .out_c(matrixC7_0));
processing_element pe8_0(.reset(effective_rst), .clk(clk),  .in_a(a8), .in_b(b7_0to8_0),  .out_a(a8_0to8_1), .out_b(b8_0to9_0), .out_c(matrixC8_0));
processing_element pe9_0(.reset(effective_rst), .clk(clk),  .in_a(a9), .in_b(b8_0to9_0),  .out_a(a9_0to9_1), .out_b(b9_0to10_0), .out_c(matrixC9_0));
processing_element pe10_0(.reset(effective_rst), .clk(clk),  .in_a(a10), .in_b(b9_0to10_0),  .out_a(a10_0to10_1), .out_b(b10_0to11_0), .out_c(matrixC10_0));
processing_element pe11_0(.reset(effective_rst), .clk(clk),  .in_a(a11), .in_b(b10_0to11_0),  .out_a(a11_0to11_1), .out_b(b11_0to12_0), .out_c(matrixC11_0));
processing_element pe12_0(.reset(effective_rst), .clk(clk),  .in_a(a12), .in_b(b11_0to12_0),  .out_a(a12_0to12_1), .out_b(b12_0to13_0), .out_c(matrixC12_0));
processing_element pe13_0(.reset(effective_rst), .clk(clk),  .in_a(a13), .in_b(b12_0to13_0),  .out_a(a13_0to13_1), .out_b(b13_0to14_0), .out_c(matrixC13_0));
processing_element pe14_0(.reset(effective_rst), .clk(clk),  .in_a(a14), .in_b(b13_0to14_0),  .out_a(a14_0to14_1), .out_b(b14_0to15_0), .out_c(matrixC14_0));
processing_element pe15_0(.reset(effective_rst), .clk(clk),  .in_a(a15), .in_b(b14_0to15_0),  .out_a(a15_0to15_1), .out_b(b15_0to16_0), .out_c(matrixC15_0));

processing_element pe1_1(.reset(effective_rst), .clk(clk),  .in_a(a1_0to1_1), .in_b(b0_1to1_1),  .out_a(a1_1to1_2), .out_b(b1_1to2_1), .out_c(matrixC1_1));
processing_element pe1_2(.reset(effective_rst), .clk(clk),  .in_a(a1_1to1_2), .in_b(b0_2to1_2),  .out_a(a1_2to1_3), .out_b(b1_2to2_2), .out_c(matrixC1_2));
processing_element pe1_3(.reset(effective_rst), .clk(clk),  .in_a(a1_2to1_3), .in_b(b0_3to1_3),  .out_a(a1_3to1_4), .out_b(b1_3to2_3), .out_c(matrixC1_3));
processing_element pe1_4(.reset(effective_rst), .clk(clk),  .in_a(a1_3to1_4), .in_b(b0_4to1_4),  .out_a(a1_4to1_5), .out_b(b1_4to2_4), .out_c(matrixC1_4));
processing_element pe1_5(.reset(effective_rst), .clk(clk),  .in_a(a1_4to1_5), .in_b(b0_5to1_5),  .out_a(a1_5to1_6), .out_b(b1_5to2_5), .out_c(matrixC1_5));
processing_element pe1_6(.reset(effective_rst), .clk(clk),  .in_a(a1_5to1_6), .in_b(b0_6to1_6),  .out_a(a1_6to1_7), .out_b(b1_6to2_6), .out_c(matrixC1_6));
processing_element pe1_7(.reset(effective_rst), .clk(clk),  .in_a(a1_6to1_7), .in_b(b0_7to1_7),  .out_a(a1_7to1_8), .out_b(b1_7to2_7), .out_c(matrixC1_7));
processing_element pe1_8(.reset(effective_rst), .clk(clk),  .in_a(a1_7to1_8), .in_b(b0_8to1_8),  .out_a(a1_8to1_9), .out_b(b1_8to2_8), .out_c(matrixC1_8));
processing_element pe1_9(.reset(effective_rst), .clk(clk),  .in_a(a1_8to1_9), .in_b(b0_9to1_9),  .out_a(a1_9to1_10), .out_b(b1_9to2_9), .out_c(matrixC1_9));
processing_element pe1_10(.reset(effective_rst), .clk(clk),  .in_a(a1_9to1_10), .in_b(b0_10to1_10),  .out_a(a1_10to1_11), .out_b(b1_10to2_10), .out_c(matrixC1_10));
processing_element pe1_11(.reset(effective_rst), .clk(clk),  .in_a(a1_10to1_11), .in_b(b0_11to1_11),  .out_a(a1_11to1_12), .out_b(b1_11to2_11), .out_c(matrixC1_11));
processing_element pe1_12(.reset(effective_rst), .clk(clk),  .in_a(a1_11to1_12), .in_b(b0_12to1_12),  .out_a(a1_12to1_13), .out_b(b1_12to2_12), .out_c(matrixC1_12));
processing_element pe1_13(.reset(effective_rst), .clk(clk),  .in_a(a1_12to1_13), .in_b(b0_13to1_13),  .out_a(a1_13to1_14), .out_b(b1_13to2_13), .out_c(matrixC1_13));
processing_element pe1_14(.reset(effective_rst), .clk(clk),  .in_a(a1_13to1_14), .in_b(b0_14to1_14),  .out_a(a1_14to1_15), .out_b(b1_14to2_14), .out_c(matrixC1_14));
processing_element pe1_15(.reset(effective_rst), .clk(clk),  .in_a(a1_14to1_15), .in_b(b0_15to1_15),  .out_a(a1_15to1_16), .out_b(b1_15to2_15), .out_c(matrixC1_15));
processing_element pe2_1(.reset(effective_rst), .clk(clk),  .in_a(a2_0to2_1), .in_b(b1_1to2_1),  .out_a(a2_1to2_2), .out_b(b2_1to3_1), .out_c(matrixC2_1));
processing_element pe2_2(.reset(effective_rst), .clk(clk),  .in_a(a2_1to2_2), .in_b(b1_2to2_2),  .out_a(a2_2to2_3), .out_b(b2_2to3_2), .out_c(matrixC2_2));
processing_element pe2_3(.reset(effective_rst), .clk(clk),  .in_a(a2_2to2_3), .in_b(b1_3to2_3),  .out_a(a2_3to2_4), .out_b(b2_3to3_3), .out_c(matrixC2_3));
processing_element pe2_4(.reset(effective_rst), .clk(clk),  .in_a(a2_3to2_4), .in_b(b1_4to2_4),  .out_a(a2_4to2_5), .out_b(b2_4to3_4), .out_c(matrixC2_4));
processing_element pe2_5(.reset(effective_rst), .clk(clk),  .in_a(a2_4to2_5), .in_b(b1_5to2_5),  .out_a(a2_5to2_6), .out_b(b2_5to3_5), .out_c(matrixC2_5));
processing_element pe2_6(.reset(effective_rst), .clk(clk),  .in_a(a2_5to2_6), .in_b(b1_6to2_6),  .out_a(a2_6to2_7), .out_b(b2_6to3_6), .out_c(matrixC2_6));
processing_element pe2_7(.reset(effective_rst), .clk(clk),  .in_a(a2_6to2_7), .in_b(b1_7to2_7),  .out_a(a2_7to2_8), .out_b(b2_7to3_7), .out_c(matrixC2_7));
processing_element pe2_8(.reset(effective_rst), .clk(clk),  .in_a(a2_7to2_8), .in_b(b1_8to2_8),  .out_a(a2_8to2_9), .out_b(b2_8to3_8), .out_c(matrixC2_8));
processing_element pe2_9(.reset(effective_rst), .clk(clk),  .in_a(a2_8to2_9), .in_b(b1_9to2_9),  .out_a(a2_9to2_10), .out_b(b2_9to3_9), .out_c(matrixC2_9));
processing_element pe2_10(.reset(effective_rst), .clk(clk),  .in_a(a2_9to2_10), .in_b(b1_10to2_10),  .out_a(a2_10to2_11), .out_b(b2_10to3_10), .out_c(matrixC2_10));
processing_element pe2_11(.reset(effective_rst), .clk(clk),  .in_a(a2_10to2_11), .in_b(b1_11to2_11),  .out_a(a2_11to2_12), .out_b(b2_11to3_11), .out_c(matrixC2_11));
processing_element pe2_12(.reset(effective_rst), .clk(clk),  .in_a(a2_11to2_12), .in_b(b1_12to2_12),  .out_a(a2_12to2_13), .out_b(b2_12to3_12), .out_c(matrixC2_12));
processing_element pe2_13(.reset(effective_rst), .clk(clk),  .in_a(a2_12to2_13), .in_b(b1_13to2_13),  .out_a(a2_13to2_14), .out_b(b2_13to3_13), .out_c(matrixC2_13));
processing_element pe2_14(.reset(effective_rst), .clk(clk),  .in_a(a2_13to2_14), .in_b(b1_14to2_14),  .out_a(a2_14to2_15), .out_b(b2_14to3_14), .out_c(matrixC2_14));
processing_element pe2_15(.reset(effective_rst), .clk(clk),  .in_a(a2_14to2_15), .in_b(b1_15to2_15),  .out_a(a2_15to2_16), .out_b(b2_15to3_15), .out_c(matrixC2_15));
processing_element pe3_1(.reset(effective_rst), .clk(clk),  .in_a(a3_0to3_1), .in_b(b2_1to3_1),  .out_a(a3_1to3_2), .out_b(b3_1to4_1), .out_c(matrixC3_1));
processing_element pe3_2(.reset(effective_rst), .clk(clk),  .in_a(a3_1to3_2), .in_b(b2_2to3_2),  .out_a(a3_2to3_3), .out_b(b3_2to4_2), .out_c(matrixC3_2));
processing_element pe3_3(.reset(effective_rst), .clk(clk),  .in_a(a3_2to3_3), .in_b(b2_3to3_3),  .out_a(a3_3to3_4), .out_b(b3_3to4_3), .out_c(matrixC3_3));
processing_element pe3_4(.reset(effective_rst), .clk(clk),  .in_a(a3_3to3_4), .in_b(b2_4to3_4),  .out_a(a3_4to3_5), .out_b(b3_4to4_4), .out_c(matrixC3_4));
processing_element pe3_5(.reset(effective_rst), .clk(clk),  .in_a(a3_4to3_5), .in_b(b2_5to3_5),  .out_a(a3_5to3_6), .out_b(b3_5to4_5), .out_c(matrixC3_5));
processing_element pe3_6(.reset(effective_rst), .clk(clk),  .in_a(a3_5to3_6), .in_b(b2_6to3_6),  .out_a(a3_6to3_7), .out_b(b3_6to4_6), .out_c(matrixC3_6));
processing_element pe3_7(.reset(effective_rst), .clk(clk),  .in_a(a3_6to3_7), .in_b(b2_7to3_7),  .out_a(a3_7to3_8), .out_b(b3_7to4_7), .out_c(matrixC3_7));
processing_element pe3_8(.reset(effective_rst), .clk(clk),  .in_a(a3_7to3_8), .in_b(b2_8to3_8),  .out_a(a3_8to3_9), .out_b(b3_8to4_8), .out_c(matrixC3_8));
processing_element pe3_9(.reset(effective_rst), .clk(clk),  .in_a(a3_8to3_9), .in_b(b2_9to3_9),  .out_a(a3_9to3_10), .out_b(b3_9to4_9), .out_c(matrixC3_9));
processing_element pe3_10(.reset(effective_rst), .clk(clk),  .in_a(a3_9to3_10), .in_b(b2_10to3_10),  .out_a(a3_10to3_11), .out_b(b3_10to4_10), .out_c(matrixC3_10));
processing_element pe3_11(.reset(effective_rst), .clk(clk),  .in_a(a3_10to3_11), .in_b(b2_11to3_11),  .out_a(a3_11to3_12), .out_b(b3_11to4_11), .out_c(matrixC3_11));
processing_element pe3_12(.reset(effective_rst), .clk(clk),  .in_a(a3_11to3_12), .in_b(b2_12to3_12),  .out_a(a3_12to3_13), .out_b(b3_12to4_12), .out_c(matrixC3_12));
processing_element pe3_13(.reset(effective_rst), .clk(clk),  .in_a(a3_12to3_13), .in_b(b2_13to3_13),  .out_a(a3_13to3_14), .out_b(b3_13to4_13), .out_c(matrixC3_13));
processing_element pe3_14(.reset(effective_rst), .clk(clk),  .in_a(a3_13to3_14), .in_b(b2_14to3_14),  .out_a(a3_14to3_15), .out_b(b3_14to4_14), .out_c(matrixC3_14));
processing_element pe3_15(.reset(effective_rst), .clk(clk),  .in_a(a3_14to3_15), .in_b(b2_15to3_15),  .out_a(a3_15to3_16), .out_b(b3_15to4_15), .out_c(matrixC3_15));
processing_element pe4_1(.reset(effective_rst), .clk(clk),  .in_a(a4_0to4_1), .in_b(b3_1to4_1),  .out_a(a4_1to4_2), .out_b(b4_1to5_1), .out_c(matrixC4_1));
processing_element pe4_2(.reset(effective_rst), .clk(clk),  .in_a(a4_1to4_2), .in_b(b3_2to4_2),  .out_a(a4_2to4_3), .out_b(b4_2to5_2), .out_c(matrixC4_2));
processing_element pe4_3(.reset(effective_rst), .clk(clk),  .in_a(a4_2to4_3), .in_b(b3_3to4_3),  .out_a(a4_3to4_4), .out_b(b4_3to5_3), .out_c(matrixC4_3));
processing_element pe4_4(.reset(effective_rst), .clk(clk),  .in_a(a4_3to4_4), .in_b(b3_4to4_4),  .out_a(a4_4to4_5), .out_b(b4_4to5_4), .out_c(matrixC4_4));
processing_element pe4_5(.reset(effective_rst), .clk(clk),  .in_a(a4_4to4_5), .in_b(b3_5to4_5),  .out_a(a4_5to4_6), .out_b(b4_5to5_5), .out_c(matrixC4_5));
processing_element pe4_6(.reset(effective_rst), .clk(clk),  .in_a(a4_5to4_6), .in_b(b3_6to4_6),  .out_a(a4_6to4_7), .out_b(b4_6to5_6), .out_c(matrixC4_6));
processing_element pe4_7(.reset(effective_rst), .clk(clk),  .in_a(a4_6to4_7), .in_b(b3_7to4_7),  .out_a(a4_7to4_8), .out_b(b4_7to5_7), .out_c(matrixC4_7));
processing_element pe4_8(.reset(effective_rst), .clk(clk),  .in_a(a4_7to4_8), .in_b(b3_8to4_8),  .out_a(a4_8to4_9), .out_b(b4_8to5_8), .out_c(matrixC4_8));
processing_element pe4_9(.reset(effective_rst), .clk(clk),  .in_a(a4_8to4_9), .in_b(b3_9to4_9),  .out_a(a4_9to4_10), .out_b(b4_9to5_9), .out_c(matrixC4_9));
processing_element pe4_10(.reset(effective_rst), .clk(clk),  .in_a(a4_9to4_10), .in_b(b3_10to4_10),  .out_a(a4_10to4_11), .out_b(b4_10to5_10), .out_c(matrixC4_10));
processing_element pe4_11(.reset(effective_rst), .clk(clk),  .in_a(a4_10to4_11), .in_b(b3_11to4_11),  .out_a(a4_11to4_12), .out_b(b4_11to5_11), .out_c(matrixC4_11));
processing_element pe4_12(.reset(effective_rst), .clk(clk),  .in_a(a4_11to4_12), .in_b(b3_12to4_12),  .out_a(a4_12to4_13), .out_b(b4_12to5_12), .out_c(matrixC4_12));
processing_element pe4_13(.reset(effective_rst), .clk(clk),  .in_a(a4_12to4_13), .in_b(b3_13to4_13),  .out_a(a4_13to4_14), .out_b(b4_13to5_13), .out_c(matrixC4_13));
processing_element pe4_14(.reset(effective_rst), .clk(clk),  .in_a(a4_13to4_14), .in_b(b3_14to4_14),  .out_a(a4_14to4_15), .out_b(b4_14to5_14), .out_c(matrixC4_14));
processing_element pe4_15(.reset(effective_rst), .clk(clk),  .in_a(a4_14to4_15), .in_b(b3_15to4_15),  .out_a(a4_15to4_16), .out_b(b4_15to5_15), .out_c(matrixC4_15));
processing_element pe5_1(.reset(effective_rst), .clk(clk),  .in_a(a5_0to5_1), .in_b(b4_1to5_1),  .out_a(a5_1to5_2), .out_b(b5_1to6_1), .out_c(matrixC5_1));
processing_element pe5_2(.reset(effective_rst), .clk(clk),  .in_a(a5_1to5_2), .in_b(b4_2to5_2),  .out_a(a5_2to5_3), .out_b(b5_2to6_2), .out_c(matrixC5_2));
processing_element pe5_3(.reset(effective_rst), .clk(clk),  .in_a(a5_2to5_3), .in_b(b4_3to5_3),  .out_a(a5_3to5_4), .out_b(b5_3to6_3), .out_c(matrixC5_3));
processing_element pe5_4(.reset(effective_rst), .clk(clk),  .in_a(a5_3to5_4), .in_b(b4_4to5_4),  .out_a(a5_4to5_5), .out_b(b5_4to6_4), .out_c(matrixC5_4));
processing_element pe5_5(.reset(effective_rst), .clk(clk),  .in_a(a5_4to5_5), .in_b(b4_5to5_5),  .out_a(a5_5to5_6), .out_b(b5_5to6_5), .out_c(matrixC5_5));
processing_element pe5_6(.reset(effective_rst), .clk(clk),  .in_a(a5_5to5_6), .in_b(b4_6to5_6),  .out_a(a5_6to5_7), .out_b(b5_6to6_6), .out_c(matrixC5_6));
processing_element pe5_7(.reset(effective_rst), .clk(clk),  .in_a(a5_6to5_7), .in_b(b4_7to5_7),  .out_a(a5_7to5_8), .out_b(b5_7to6_7), .out_c(matrixC5_7));
processing_element pe5_8(.reset(effective_rst), .clk(clk),  .in_a(a5_7to5_8), .in_b(b4_8to5_8),  .out_a(a5_8to5_9), .out_b(b5_8to6_8), .out_c(matrixC5_8));
processing_element pe5_9(.reset(effective_rst), .clk(clk),  .in_a(a5_8to5_9), .in_b(b4_9to5_9),  .out_a(a5_9to5_10), .out_b(b5_9to6_9), .out_c(matrixC5_9));
processing_element pe5_10(.reset(effective_rst), .clk(clk),  .in_a(a5_9to5_10), .in_b(b4_10to5_10),  .out_a(a5_10to5_11), .out_b(b5_10to6_10), .out_c(matrixC5_10));
processing_element pe5_11(.reset(effective_rst), .clk(clk),  .in_a(a5_10to5_11), .in_b(b4_11to5_11),  .out_a(a5_11to5_12), .out_b(b5_11to6_11), .out_c(matrixC5_11));
processing_element pe5_12(.reset(effective_rst), .clk(clk),  .in_a(a5_11to5_12), .in_b(b4_12to5_12),  .out_a(a5_12to5_13), .out_b(b5_12to6_12), .out_c(matrixC5_12));
processing_element pe5_13(.reset(effective_rst), .clk(clk),  .in_a(a5_12to5_13), .in_b(b4_13to5_13),  .out_a(a5_13to5_14), .out_b(b5_13to6_13), .out_c(matrixC5_13));
processing_element pe5_14(.reset(effective_rst), .clk(clk),  .in_a(a5_13to5_14), .in_b(b4_14to5_14),  .out_a(a5_14to5_15), .out_b(b5_14to6_14), .out_c(matrixC5_14));
processing_element pe5_15(.reset(effective_rst), .clk(clk),  .in_a(a5_14to5_15), .in_b(b4_15to5_15),  .out_a(a5_15to5_16), .out_b(b5_15to6_15), .out_c(matrixC5_15));
processing_element pe6_1(.reset(effective_rst), .clk(clk),  .in_a(a6_0to6_1), .in_b(b5_1to6_1),  .out_a(a6_1to6_2), .out_b(b6_1to7_1), .out_c(matrixC6_1));
processing_element pe6_2(.reset(effective_rst), .clk(clk),  .in_a(a6_1to6_2), .in_b(b5_2to6_2),  .out_a(a6_2to6_3), .out_b(b6_2to7_2), .out_c(matrixC6_2));
processing_element pe6_3(.reset(effective_rst), .clk(clk),  .in_a(a6_2to6_3), .in_b(b5_3to6_3),  .out_a(a6_3to6_4), .out_b(b6_3to7_3), .out_c(matrixC6_3));
processing_element pe6_4(.reset(effective_rst), .clk(clk),  .in_a(a6_3to6_4), .in_b(b5_4to6_4),  .out_a(a6_4to6_5), .out_b(b6_4to7_4), .out_c(matrixC6_4));
processing_element pe6_5(.reset(effective_rst), .clk(clk),  .in_a(a6_4to6_5), .in_b(b5_5to6_5),  .out_a(a6_5to6_6), .out_b(b6_5to7_5), .out_c(matrixC6_5));
processing_element pe6_6(.reset(effective_rst), .clk(clk),  .in_a(a6_5to6_6), .in_b(b5_6to6_6),  .out_a(a6_6to6_7), .out_b(b6_6to7_6), .out_c(matrixC6_6));
processing_element pe6_7(.reset(effective_rst), .clk(clk),  .in_a(a6_6to6_7), .in_b(b5_7to6_7),  .out_a(a6_7to6_8), .out_b(b6_7to7_7), .out_c(matrixC6_7));
processing_element pe6_8(.reset(effective_rst), .clk(clk),  .in_a(a6_7to6_8), .in_b(b5_8to6_8),  .out_a(a6_8to6_9), .out_b(b6_8to7_8), .out_c(matrixC6_8));
processing_element pe6_9(.reset(effective_rst), .clk(clk),  .in_a(a6_8to6_9), .in_b(b5_9to6_9),  .out_a(a6_9to6_10), .out_b(b6_9to7_9), .out_c(matrixC6_9));
processing_element pe6_10(.reset(effective_rst), .clk(clk),  .in_a(a6_9to6_10), .in_b(b5_10to6_10),  .out_a(a6_10to6_11), .out_b(b6_10to7_10), .out_c(matrixC6_10));
processing_element pe6_11(.reset(effective_rst), .clk(clk),  .in_a(a6_10to6_11), .in_b(b5_11to6_11),  .out_a(a6_11to6_12), .out_b(b6_11to7_11), .out_c(matrixC6_11));
processing_element pe6_12(.reset(effective_rst), .clk(clk),  .in_a(a6_11to6_12), .in_b(b5_12to6_12),  .out_a(a6_12to6_13), .out_b(b6_12to7_12), .out_c(matrixC6_12));
processing_element pe6_13(.reset(effective_rst), .clk(clk),  .in_a(a6_12to6_13), .in_b(b5_13to6_13),  .out_a(a6_13to6_14), .out_b(b6_13to7_13), .out_c(matrixC6_13));
processing_element pe6_14(.reset(effective_rst), .clk(clk),  .in_a(a6_13to6_14), .in_b(b5_14to6_14),  .out_a(a6_14to6_15), .out_b(b6_14to7_14), .out_c(matrixC6_14));
processing_element pe6_15(.reset(effective_rst), .clk(clk),  .in_a(a6_14to6_15), .in_b(b5_15to6_15),  .out_a(a6_15to6_16), .out_b(b6_15to7_15), .out_c(matrixC6_15));
processing_element pe7_1(.reset(effective_rst), .clk(clk),  .in_a(a7_0to7_1), .in_b(b6_1to7_1),  .out_a(a7_1to7_2), .out_b(b7_1to8_1), .out_c(matrixC7_1));
processing_element pe7_2(.reset(effective_rst), .clk(clk),  .in_a(a7_1to7_2), .in_b(b6_2to7_2),  .out_a(a7_2to7_3), .out_b(b7_2to8_2), .out_c(matrixC7_2));
processing_element pe7_3(.reset(effective_rst), .clk(clk),  .in_a(a7_2to7_3), .in_b(b6_3to7_3),  .out_a(a7_3to7_4), .out_b(b7_3to8_3), .out_c(matrixC7_3));
processing_element pe7_4(.reset(effective_rst), .clk(clk),  .in_a(a7_3to7_4), .in_b(b6_4to7_4),  .out_a(a7_4to7_5), .out_b(b7_4to8_4), .out_c(matrixC7_4));
processing_element pe7_5(.reset(effective_rst), .clk(clk),  .in_a(a7_4to7_5), .in_b(b6_5to7_5),  .out_a(a7_5to7_6), .out_b(b7_5to8_5), .out_c(matrixC7_5));
processing_element pe7_6(.reset(effective_rst), .clk(clk),  .in_a(a7_5to7_6), .in_b(b6_6to7_6),  .out_a(a7_6to7_7), .out_b(b7_6to8_6), .out_c(matrixC7_6));
processing_element pe7_7(.reset(effective_rst), .clk(clk),  .in_a(a7_6to7_7), .in_b(b6_7to7_7),  .out_a(a7_7to7_8), .out_b(b7_7to8_7), .out_c(matrixC7_7));
processing_element pe7_8(.reset(effective_rst), .clk(clk),  .in_a(a7_7to7_8), .in_b(b6_8to7_8),  .out_a(a7_8to7_9), .out_b(b7_8to8_8), .out_c(matrixC7_8));
processing_element pe7_9(.reset(effective_rst), .clk(clk),  .in_a(a7_8to7_9), .in_b(b6_9to7_9),  .out_a(a7_9to7_10), .out_b(b7_9to8_9), .out_c(matrixC7_9));
processing_element pe7_10(.reset(effective_rst), .clk(clk),  .in_a(a7_9to7_10), .in_b(b6_10to7_10),  .out_a(a7_10to7_11), .out_b(b7_10to8_10), .out_c(matrixC7_10));
processing_element pe7_11(.reset(effective_rst), .clk(clk),  .in_a(a7_10to7_11), .in_b(b6_11to7_11),  .out_a(a7_11to7_12), .out_b(b7_11to8_11), .out_c(matrixC7_11));
processing_element pe7_12(.reset(effective_rst), .clk(clk),  .in_a(a7_11to7_12), .in_b(b6_12to7_12),  .out_a(a7_12to7_13), .out_b(b7_12to8_12), .out_c(matrixC7_12));
processing_element pe7_13(.reset(effective_rst), .clk(clk),  .in_a(a7_12to7_13), .in_b(b6_13to7_13),  .out_a(a7_13to7_14), .out_b(b7_13to8_13), .out_c(matrixC7_13));
processing_element pe7_14(.reset(effective_rst), .clk(clk),  .in_a(a7_13to7_14), .in_b(b6_14to7_14),  .out_a(a7_14to7_15), .out_b(b7_14to8_14), .out_c(matrixC7_14));
processing_element pe7_15(.reset(effective_rst), .clk(clk),  .in_a(a7_14to7_15), .in_b(b6_15to7_15),  .out_a(a7_15to7_16), .out_b(b7_15to8_15), .out_c(matrixC7_15));
processing_element pe8_1(.reset(effective_rst), .clk(clk),  .in_a(a8_0to8_1), .in_b(b7_1to8_1),  .out_a(a8_1to8_2), .out_b(b8_1to9_1), .out_c(matrixC8_1));
processing_element pe8_2(.reset(effective_rst), .clk(clk),  .in_a(a8_1to8_2), .in_b(b7_2to8_2),  .out_a(a8_2to8_3), .out_b(b8_2to9_2), .out_c(matrixC8_2));
processing_element pe8_3(.reset(effective_rst), .clk(clk),  .in_a(a8_2to8_3), .in_b(b7_3to8_3),  .out_a(a8_3to8_4), .out_b(b8_3to9_3), .out_c(matrixC8_3));
processing_element pe8_4(.reset(effective_rst), .clk(clk),  .in_a(a8_3to8_4), .in_b(b7_4to8_4),  .out_a(a8_4to8_5), .out_b(b8_4to9_4), .out_c(matrixC8_4));
processing_element pe8_5(.reset(effective_rst), .clk(clk),  .in_a(a8_4to8_5), .in_b(b7_5to8_5),  .out_a(a8_5to8_6), .out_b(b8_5to9_5), .out_c(matrixC8_5));
processing_element pe8_6(.reset(effective_rst), .clk(clk),  .in_a(a8_5to8_6), .in_b(b7_6to8_6),  .out_a(a8_6to8_7), .out_b(b8_6to9_6), .out_c(matrixC8_6));
processing_element pe8_7(.reset(effective_rst), .clk(clk),  .in_a(a8_6to8_7), .in_b(b7_7to8_7),  .out_a(a8_7to8_8), .out_b(b8_7to9_7), .out_c(matrixC8_7));
processing_element pe8_8(.reset(effective_rst), .clk(clk),  .in_a(a8_7to8_8), .in_b(b7_8to8_8),  .out_a(a8_8to8_9), .out_b(b8_8to9_8), .out_c(matrixC8_8));
processing_element pe8_9(.reset(effective_rst), .clk(clk),  .in_a(a8_8to8_9), .in_b(b7_9to8_9),  .out_a(a8_9to8_10), .out_b(b8_9to9_9), .out_c(matrixC8_9));
processing_element pe8_10(.reset(effective_rst), .clk(clk),  .in_a(a8_9to8_10), .in_b(b7_10to8_10),  .out_a(a8_10to8_11), .out_b(b8_10to9_10), .out_c(matrixC8_10));
processing_element pe8_11(.reset(effective_rst), .clk(clk),  .in_a(a8_10to8_11), .in_b(b7_11to8_11),  .out_a(a8_11to8_12), .out_b(b8_11to9_11), .out_c(matrixC8_11));
processing_element pe8_12(.reset(effective_rst), .clk(clk),  .in_a(a8_11to8_12), .in_b(b7_12to8_12),  .out_a(a8_12to8_13), .out_b(b8_12to9_12), .out_c(matrixC8_12));
processing_element pe8_13(.reset(effective_rst), .clk(clk),  .in_a(a8_12to8_13), .in_b(b7_13to8_13),  .out_a(a8_13to8_14), .out_b(b8_13to9_13), .out_c(matrixC8_13));
processing_element pe8_14(.reset(effective_rst), .clk(clk),  .in_a(a8_13to8_14), .in_b(b7_14to8_14),  .out_a(a8_14to8_15), .out_b(b8_14to9_14), .out_c(matrixC8_14));
processing_element pe8_15(.reset(effective_rst), .clk(clk),  .in_a(a8_14to8_15), .in_b(b7_15to8_15),  .out_a(a8_15to8_16), .out_b(b8_15to9_15), .out_c(matrixC8_15));
processing_element pe9_1(.reset(effective_rst), .clk(clk),  .in_a(a9_0to9_1), .in_b(b8_1to9_1),  .out_a(a9_1to9_2), .out_b(b9_1to10_1), .out_c(matrixC9_1));
processing_element pe9_2(.reset(effective_rst), .clk(clk),  .in_a(a9_1to9_2), .in_b(b8_2to9_2),  .out_a(a9_2to9_3), .out_b(b9_2to10_2), .out_c(matrixC9_2));
processing_element pe9_3(.reset(effective_rst), .clk(clk),  .in_a(a9_2to9_3), .in_b(b8_3to9_3),  .out_a(a9_3to9_4), .out_b(b9_3to10_3), .out_c(matrixC9_3));
processing_element pe9_4(.reset(effective_rst), .clk(clk),  .in_a(a9_3to9_4), .in_b(b8_4to9_4),  .out_a(a9_4to9_5), .out_b(b9_4to10_4), .out_c(matrixC9_4));
processing_element pe9_5(.reset(effective_rst), .clk(clk),  .in_a(a9_4to9_5), .in_b(b8_5to9_5),  .out_a(a9_5to9_6), .out_b(b9_5to10_5), .out_c(matrixC9_5));
processing_element pe9_6(.reset(effective_rst), .clk(clk),  .in_a(a9_5to9_6), .in_b(b8_6to9_6),  .out_a(a9_6to9_7), .out_b(b9_6to10_6), .out_c(matrixC9_6));
processing_element pe9_7(.reset(effective_rst), .clk(clk),  .in_a(a9_6to9_7), .in_b(b8_7to9_7),  .out_a(a9_7to9_8), .out_b(b9_7to10_7), .out_c(matrixC9_7));
processing_element pe9_8(.reset(effective_rst), .clk(clk),  .in_a(a9_7to9_8), .in_b(b8_8to9_8),  .out_a(a9_8to9_9), .out_b(b9_8to10_8), .out_c(matrixC9_8));
processing_element pe9_9(.reset(effective_rst), .clk(clk),  .in_a(a9_8to9_9), .in_b(b8_9to9_9),  .out_a(a9_9to9_10), .out_b(b9_9to10_9), .out_c(matrixC9_9));
processing_element pe9_10(.reset(effective_rst), .clk(clk),  .in_a(a9_9to9_10), .in_b(b8_10to9_10),  .out_a(a9_10to9_11), .out_b(b9_10to10_10), .out_c(matrixC9_10));
processing_element pe9_11(.reset(effective_rst), .clk(clk),  .in_a(a9_10to9_11), .in_b(b8_11to9_11),  .out_a(a9_11to9_12), .out_b(b9_11to10_11), .out_c(matrixC9_11));
processing_element pe9_12(.reset(effective_rst), .clk(clk),  .in_a(a9_11to9_12), .in_b(b8_12to9_12),  .out_a(a9_12to9_13), .out_b(b9_12to10_12), .out_c(matrixC9_12));
processing_element pe9_13(.reset(effective_rst), .clk(clk),  .in_a(a9_12to9_13), .in_b(b8_13to9_13),  .out_a(a9_13to9_14), .out_b(b9_13to10_13), .out_c(matrixC9_13));
processing_element pe9_14(.reset(effective_rst), .clk(clk),  .in_a(a9_13to9_14), .in_b(b8_14to9_14),  .out_a(a9_14to9_15), .out_b(b9_14to10_14), .out_c(matrixC9_14));
processing_element pe9_15(.reset(effective_rst), .clk(clk),  .in_a(a9_14to9_15), .in_b(b8_15to9_15),  .out_a(a9_15to9_16), .out_b(b9_15to10_15), .out_c(matrixC9_15));
processing_element pe10_1(.reset(effective_rst), .clk(clk),  .in_a(a10_0to10_1), .in_b(b9_1to10_1),  .out_a(a10_1to10_2), .out_b(b10_1to11_1), .out_c(matrixC10_1));
processing_element pe10_2(.reset(effective_rst), .clk(clk),  .in_a(a10_1to10_2), .in_b(b9_2to10_2),  .out_a(a10_2to10_3), .out_b(b10_2to11_2), .out_c(matrixC10_2));
processing_element pe10_3(.reset(effective_rst), .clk(clk),  .in_a(a10_2to10_3), .in_b(b9_3to10_3),  .out_a(a10_3to10_4), .out_b(b10_3to11_3), .out_c(matrixC10_3));
processing_element pe10_4(.reset(effective_rst), .clk(clk),  .in_a(a10_3to10_4), .in_b(b9_4to10_4),  .out_a(a10_4to10_5), .out_b(b10_4to11_4), .out_c(matrixC10_4));
processing_element pe10_5(.reset(effective_rst), .clk(clk),  .in_a(a10_4to10_5), .in_b(b9_5to10_5),  .out_a(a10_5to10_6), .out_b(b10_5to11_5), .out_c(matrixC10_5));
processing_element pe10_6(.reset(effective_rst), .clk(clk),  .in_a(a10_5to10_6), .in_b(b9_6to10_6),  .out_a(a10_6to10_7), .out_b(b10_6to11_6), .out_c(matrixC10_6));
processing_element pe10_7(.reset(effective_rst), .clk(clk),  .in_a(a10_6to10_7), .in_b(b9_7to10_7),  .out_a(a10_7to10_8), .out_b(b10_7to11_7), .out_c(matrixC10_7));
processing_element pe10_8(.reset(effective_rst), .clk(clk),  .in_a(a10_7to10_8), .in_b(b9_8to10_8),  .out_a(a10_8to10_9), .out_b(b10_8to11_8), .out_c(matrixC10_8));
processing_element pe10_9(.reset(effective_rst), .clk(clk),  .in_a(a10_8to10_9), .in_b(b9_9to10_9),  .out_a(a10_9to10_10), .out_b(b10_9to11_9), .out_c(matrixC10_9));
processing_element pe10_10(.reset(effective_rst), .clk(clk),  .in_a(a10_9to10_10), .in_b(b9_10to10_10),  .out_a(a10_10to10_11), .out_b(b10_10to11_10), .out_c(matrixC10_10));
processing_element pe10_11(.reset(effective_rst), .clk(clk),  .in_a(a10_10to10_11), .in_b(b9_11to10_11),  .out_a(a10_11to10_12), .out_b(b10_11to11_11), .out_c(matrixC10_11));
processing_element pe10_12(.reset(effective_rst), .clk(clk),  .in_a(a10_11to10_12), .in_b(b9_12to10_12),  .out_a(a10_12to10_13), .out_b(b10_12to11_12), .out_c(matrixC10_12));
processing_element pe10_13(.reset(effective_rst), .clk(clk),  .in_a(a10_12to10_13), .in_b(b9_13to10_13),  .out_a(a10_13to10_14), .out_b(b10_13to11_13), .out_c(matrixC10_13));
processing_element pe10_14(.reset(effective_rst), .clk(clk),  .in_a(a10_13to10_14), .in_b(b9_14to10_14),  .out_a(a10_14to10_15), .out_b(b10_14to11_14), .out_c(matrixC10_14));
processing_element pe10_15(.reset(effective_rst), .clk(clk),  .in_a(a10_14to10_15), .in_b(b9_15to10_15),  .out_a(a10_15to10_16), .out_b(b10_15to11_15), .out_c(matrixC10_15));
processing_element pe11_1(.reset(effective_rst), .clk(clk),  .in_a(a11_0to11_1), .in_b(b10_1to11_1),  .out_a(a11_1to11_2), .out_b(b11_1to12_1), .out_c(matrixC11_1));
processing_element pe11_2(.reset(effective_rst), .clk(clk),  .in_a(a11_1to11_2), .in_b(b10_2to11_2),  .out_a(a11_2to11_3), .out_b(b11_2to12_2), .out_c(matrixC11_2));
processing_element pe11_3(.reset(effective_rst), .clk(clk),  .in_a(a11_2to11_3), .in_b(b10_3to11_3),  .out_a(a11_3to11_4), .out_b(b11_3to12_3), .out_c(matrixC11_3));
processing_element pe11_4(.reset(effective_rst), .clk(clk),  .in_a(a11_3to11_4), .in_b(b10_4to11_4),  .out_a(a11_4to11_5), .out_b(b11_4to12_4), .out_c(matrixC11_4));
processing_element pe11_5(.reset(effective_rst), .clk(clk),  .in_a(a11_4to11_5), .in_b(b10_5to11_5),  .out_a(a11_5to11_6), .out_b(b11_5to12_5), .out_c(matrixC11_5));
processing_element pe11_6(.reset(effective_rst), .clk(clk),  .in_a(a11_5to11_6), .in_b(b10_6to11_6),  .out_a(a11_6to11_7), .out_b(b11_6to12_6), .out_c(matrixC11_6));
processing_element pe11_7(.reset(effective_rst), .clk(clk),  .in_a(a11_6to11_7), .in_b(b10_7to11_7),  .out_a(a11_7to11_8), .out_b(b11_7to12_7), .out_c(matrixC11_7));
processing_element pe11_8(.reset(effective_rst), .clk(clk),  .in_a(a11_7to11_8), .in_b(b10_8to11_8),  .out_a(a11_8to11_9), .out_b(b11_8to12_8), .out_c(matrixC11_8));
processing_element pe11_9(.reset(effective_rst), .clk(clk),  .in_a(a11_8to11_9), .in_b(b10_9to11_9),  .out_a(a11_9to11_10), .out_b(b11_9to12_9), .out_c(matrixC11_9));
processing_element pe11_10(.reset(effective_rst), .clk(clk),  .in_a(a11_9to11_10), .in_b(b10_10to11_10),  .out_a(a11_10to11_11), .out_b(b11_10to12_10), .out_c(matrixC11_10));
processing_element pe11_11(.reset(effective_rst), .clk(clk),  .in_a(a11_10to11_11), .in_b(b10_11to11_11),  .out_a(a11_11to11_12), .out_b(b11_11to12_11), .out_c(matrixC11_11));
processing_element pe11_12(.reset(effective_rst), .clk(clk),  .in_a(a11_11to11_12), .in_b(b10_12to11_12),  .out_a(a11_12to11_13), .out_b(b11_12to12_12), .out_c(matrixC11_12));
processing_element pe11_13(.reset(effective_rst), .clk(clk),  .in_a(a11_12to11_13), .in_b(b10_13to11_13),  .out_a(a11_13to11_14), .out_b(b11_13to12_13), .out_c(matrixC11_13));
processing_element pe11_14(.reset(effective_rst), .clk(clk),  .in_a(a11_13to11_14), .in_b(b10_14to11_14),  .out_a(a11_14to11_15), .out_b(b11_14to12_14), .out_c(matrixC11_14));
processing_element pe11_15(.reset(effective_rst), .clk(clk),  .in_a(a11_14to11_15), .in_b(b10_15to11_15),  .out_a(a11_15to11_16), .out_b(b11_15to12_15), .out_c(matrixC11_15));
processing_element pe12_1(.reset(effective_rst), .clk(clk),  .in_a(a12_0to12_1), .in_b(b11_1to12_1),  .out_a(a12_1to12_2), .out_b(b12_1to13_1), .out_c(matrixC12_1));
processing_element pe12_2(.reset(effective_rst), .clk(clk),  .in_a(a12_1to12_2), .in_b(b11_2to12_2),  .out_a(a12_2to12_3), .out_b(b12_2to13_2), .out_c(matrixC12_2));
processing_element pe12_3(.reset(effective_rst), .clk(clk),  .in_a(a12_2to12_3), .in_b(b11_3to12_3),  .out_a(a12_3to12_4), .out_b(b12_3to13_3), .out_c(matrixC12_3));
processing_element pe12_4(.reset(effective_rst), .clk(clk),  .in_a(a12_3to12_4), .in_b(b11_4to12_4),  .out_a(a12_4to12_5), .out_b(b12_4to13_4), .out_c(matrixC12_4));
processing_element pe12_5(.reset(effective_rst), .clk(clk),  .in_a(a12_4to12_5), .in_b(b11_5to12_5),  .out_a(a12_5to12_6), .out_b(b12_5to13_5), .out_c(matrixC12_5));
processing_element pe12_6(.reset(effective_rst), .clk(clk),  .in_a(a12_5to12_6), .in_b(b11_6to12_6),  .out_a(a12_6to12_7), .out_b(b12_6to13_6), .out_c(matrixC12_6));
processing_element pe12_7(.reset(effective_rst), .clk(clk),  .in_a(a12_6to12_7), .in_b(b11_7to12_7),  .out_a(a12_7to12_8), .out_b(b12_7to13_7), .out_c(matrixC12_7));
processing_element pe12_8(.reset(effective_rst), .clk(clk),  .in_a(a12_7to12_8), .in_b(b11_8to12_8),  .out_a(a12_8to12_9), .out_b(b12_8to13_8), .out_c(matrixC12_8));
processing_element pe12_9(.reset(effective_rst), .clk(clk),  .in_a(a12_8to12_9), .in_b(b11_9to12_9),  .out_a(a12_9to12_10), .out_b(b12_9to13_9), .out_c(matrixC12_9));
processing_element pe12_10(.reset(effective_rst), .clk(clk),  .in_a(a12_9to12_10), .in_b(b11_10to12_10),  .out_a(a12_10to12_11), .out_b(b12_10to13_10), .out_c(matrixC12_10));
processing_element pe12_11(.reset(effective_rst), .clk(clk),  .in_a(a12_10to12_11), .in_b(b11_11to12_11),  .out_a(a12_11to12_12), .out_b(b12_11to13_11), .out_c(matrixC12_11));
processing_element pe12_12(.reset(effective_rst), .clk(clk),  .in_a(a12_11to12_12), .in_b(b11_12to12_12),  .out_a(a12_12to12_13), .out_b(b12_12to13_12), .out_c(matrixC12_12));
processing_element pe12_13(.reset(effective_rst), .clk(clk),  .in_a(a12_12to12_13), .in_b(b11_13to12_13),  .out_a(a12_13to12_14), .out_b(b12_13to13_13), .out_c(matrixC12_13));
processing_element pe12_14(.reset(effective_rst), .clk(clk),  .in_a(a12_13to12_14), .in_b(b11_14to12_14),  .out_a(a12_14to12_15), .out_b(b12_14to13_14), .out_c(matrixC12_14));
processing_element pe12_15(.reset(effective_rst), .clk(clk),  .in_a(a12_14to12_15), .in_b(b11_15to12_15),  .out_a(a12_15to12_16), .out_b(b12_15to13_15), .out_c(matrixC12_15));
processing_element pe13_1(.reset(effective_rst), .clk(clk),  .in_a(a13_0to13_1), .in_b(b12_1to13_1),  .out_a(a13_1to13_2), .out_b(b13_1to14_1), .out_c(matrixC13_1));
processing_element pe13_2(.reset(effective_rst), .clk(clk),  .in_a(a13_1to13_2), .in_b(b12_2to13_2),  .out_a(a13_2to13_3), .out_b(b13_2to14_2), .out_c(matrixC13_2));
processing_element pe13_3(.reset(effective_rst), .clk(clk),  .in_a(a13_2to13_3), .in_b(b12_3to13_3),  .out_a(a13_3to13_4), .out_b(b13_3to14_3), .out_c(matrixC13_3));
processing_element pe13_4(.reset(effective_rst), .clk(clk),  .in_a(a13_3to13_4), .in_b(b12_4to13_4),  .out_a(a13_4to13_5), .out_b(b13_4to14_4), .out_c(matrixC13_4));
processing_element pe13_5(.reset(effective_rst), .clk(clk),  .in_a(a13_4to13_5), .in_b(b12_5to13_5),  .out_a(a13_5to13_6), .out_b(b13_5to14_5), .out_c(matrixC13_5));
processing_element pe13_6(.reset(effective_rst), .clk(clk),  .in_a(a13_5to13_6), .in_b(b12_6to13_6),  .out_a(a13_6to13_7), .out_b(b13_6to14_6), .out_c(matrixC13_6));
processing_element pe13_7(.reset(effective_rst), .clk(clk),  .in_a(a13_6to13_7), .in_b(b12_7to13_7),  .out_a(a13_7to13_8), .out_b(b13_7to14_7), .out_c(matrixC13_7));
processing_element pe13_8(.reset(effective_rst), .clk(clk),  .in_a(a13_7to13_8), .in_b(b12_8to13_8),  .out_a(a13_8to13_9), .out_b(b13_8to14_8), .out_c(matrixC13_8));
processing_element pe13_9(.reset(effective_rst), .clk(clk),  .in_a(a13_8to13_9), .in_b(b12_9to13_9),  .out_a(a13_9to13_10), .out_b(b13_9to14_9), .out_c(matrixC13_9));
processing_element pe13_10(.reset(effective_rst), .clk(clk),  .in_a(a13_9to13_10), .in_b(b12_10to13_10),  .out_a(a13_10to13_11), .out_b(b13_10to14_10), .out_c(matrixC13_10));
processing_element pe13_11(.reset(effective_rst), .clk(clk),  .in_a(a13_10to13_11), .in_b(b12_11to13_11),  .out_a(a13_11to13_12), .out_b(b13_11to14_11), .out_c(matrixC13_11));
processing_element pe13_12(.reset(effective_rst), .clk(clk),  .in_a(a13_11to13_12), .in_b(b12_12to13_12),  .out_a(a13_12to13_13), .out_b(b13_12to14_12), .out_c(matrixC13_12));
processing_element pe13_13(.reset(effective_rst), .clk(clk),  .in_a(a13_12to13_13), .in_b(b12_13to13_13),  .out_a(a13_13to13_14), .out_b(b13_13to14_13), .out_c(matrixC13_13));
processing_element pe13_14(.reset(effective_rst), .clk(clk),  .in_a(a13_13to13_14), .in_b(b12_14to13_14),  .out_a(a13_14to13_15), .out_b(b13_14to14_14), .out_c(matrixC13_14));
processing_element pe13_15(.reset(effective_rst), .clk(clk),  .in_a(a13_14to13_15), .in_b(b12_15to13_15),  .out_a(a13_15to13_16), .out_b(b13_15to14_15), .out_c(matrixC13_15));
processing_element pe14_1(.reset(effective_rst), .clk(clk),  .in_a(a14_0to14_1), .in_b(b13_1to14_1),  .out_a(a14_1to14_2), .out_b(b14_1to15_1), .out_c(matrixC14_1));
processing_element pe14_2(.reset(effective_rst), .clk(clk),  .in_a(a14_1to14_2), .in_b(b13_2to14_2),  .out_a(a14_2to14_3), .out_b(b14_2to15_2), .out_c(matrixC14_2));
processing_element pe14_3(.reset(effective_rst), .clk(clk),  .in_a(a14_2to14_3), .in_b(b13_3to14_3),  .out_a(a14_3to14_4), .out_b(b14_3to15_3), .out_c(matrixC14_3));
processing_element pe14_4(.reset(effective_rst), .clk(clk),  .in_a(a14_3to14_4), .in_b(b13_4to14_4),  .out_a(a14_4to14_5), .out_b(b14_4to15_4), .out_c(matrixC14_4));
processing_element pe14_5(.reset(effective_rst), .clk(clk),  .in_a(a14_4to14_5), .in_b(b13_5to14_5),  .out_a(a14_5to14_6), .out_b(b14_5to15_5), .out_c(matrixC14_5));
processing_element pe14_6(.reset(effective_rst), .clk(clk),  .in_a(a14_5to14_6), .in_b(b13_6to14_6),  .out_a(a14_6to14_7), .out_b(b14_6to15_6), .out_c(matrixC14_6));
processing_element pe14_7(.reset(effective_rst), .clk(clk),  .in_a(a14_6to14_7), .in_b(b13_7to14_7),  .out_a(a14_7to14_8), .out_b(b14_7to15_7), .out_c(matrixC14_7));
processing_element pe14_8(.reset(effective_rst), .clk(clk),  .in_a(a14_7to14_8), .in_b(b13_8to14_8),  .out_a(a14_8to14_9), .out_b(b14_8to15_8), .out_c(matrixC14_8));
processing_element pe14_9(.reset(effective_rst), .clk(clk),  .in_a(a14_8to14_9), .in_b(b13_9to14_9),  .out_a(a14_9to14_10), .out_b(b14_9to15_9), .out_c(matrixC14_9));
processing_element pe14_10(.reset(effective_rst), .clk(clk),  .in_a(a14_9to14_10), .in_b(b13_10to14_10),  .out_a(a14_10to14_11), .out_b(b14_10to15_10), .out_c(matrixC14_10));
processing_element pe14_11(.reset(effective_rst), .clk(clk),  .in_a(a14_10to14_11), .in_b(b13_11to14_11),  .out_a(a14_11to14_12), .out_b(b14_11to15_11), .out_c(matrixC14_11));
processing_element pe14_12(.reset(effective_rst), .clk(clk),  .in_a(a14_11to14_12), .in_b(b13_12to14_12),  .out_a(a14_12to14_13), .out_b(b14_12to15_12), .out_c(matrixC14_12));
processing_element pe14_13(.reset(effective_rst), .clk(clk),  .in_a(a14_12to14_13), .in_b(b13_13to14_13),  .out_a(a14_13to14_14), .out_b(b14_13to15_13), .out_c(matrixC14_13));
processing_element pe14_14(.reset(effective_rst), .clk(clk),  .in_a(a14_13to14_14), .in_b(b13_14to14_14),  .out_a(a14_14to14_15), .out_b(b14_14to15_14), .out_c(matrixC14_14));
processing_element pe14_15(.reset(effective_rst), .clk(clk),  .in_a(a14_14to14_15), .in_b(b13_15to14_15),  .out_a(a14_15to14_16), .out_b(b14_15to15_15), .out_c(matrixC14_15));
processing_element pe15_1(.reset(effective_rst), .clk(clk),  .in_a(a15_0to15_1), .in_b(b14_1to15_1),  .out_a(a15_1to15_2), .out_b(b15_1to16_1), .out_c(matrixC15_1));
processing_element pe15_2(.reset(effective_rst), .clk(clk),  .in_a(a15_1to15_2), .in_b(b14_2to15_2),  .out_a(a15_2to15_3), .out_b(b15_2to16_2), .out_c(matrixC15_2));
processing_element pe15_3(.reset(effective_rst), .clk(clk),  .in_a(a15_2to15_3), .in_b(b14_3to15_3),  .out_a(a15_3to15_4), .out_b(b15_3to16_3), .out_c(matrixC15_3));
processing_element pe15_4(.reset(effective_rst), .clk(clk),  .in_a(a15_3to15_4), .in_b(b14_4to15_4),  .out_a(a15_4to15_5), .out_b(b15_4to16_4), .out_c(matrixC15_4));
processing_element pe15_5(.reset(effective_rst), .clk(clk),  .in_a(a15_4to15_5), .in_b(b14_5to15_5),  .out_a(a15_5to15_6), .out_b(b15_5to16_5), .out_c(matrixC15_5));
processing_element pe15_6(.reset(effective_rst), .clk(clk),  .in_a(a15_5to15_6), .in_b(b14_6to15_6),  .out_a(a15_6to15_7), .out_b(b15_6to16_6), .out_c(matrixC15_6));
processing_element pe15_7(.reset(effective_rst), .clk(clk),  .in_a(a15_6to15_7), .in_b(b14_7to15_7),  .out_a(a15_7to15_8), .out_b(b15_7to16_7), .out_c(matrixC15_7));
processing_element pe15_8(.reset(effective_rst), .clk(clk),  .in_a(a15_7to15_8), .in_b(b14_8to15_8),  .out_a(a15_8to15_9), .out_b(b15_8to16_8), .out_c(matrixC15_8));
processing_element pe15_9(.reset(effective_rst), .clk(clk),  .in_a(a15_8to15_9), .in_b(b14_9to15_9),  .out_a(a15_9to15_10), .out_b(b15_9to16_9), .out_c(matrixC15_9));
processing_element pe15_10(.reset(effective_rst), .clk(clk),  .in_a(a15_9to15_10), .in_b(b14_10to15_10),  .out_a(a15_10to15_11), .out_b(b15_10to16_10), .out_c(matrixC15_10));
processing_element pe15_11(.reset(effective_rst), .clk(clk),  .in_a(a15_10to15_11), .in_b(b14_11to15_11),  .out_a(a15_11to15_12), .out_b(b15_11to16_11), .out_c(matrixC15_11));
processing_element pe15_12(.reset(effective_rst), .clk(clk),  .in_a(a15_11to15_12), .in_b(b14_12to15_12),  .out_a(a15_12to15_13), .out_b(b15_12to16_12), .out_c(matrixC15_12));
processing_element pe15_13(.reset(effective_rst), .clk(clk),  .in_a(a15_12to15_13), .in_b(b14_13to15_13),  .out_a(a15_13to15_14), .out_b(b15_13to16_13), .out_c(matrixC15_13));
processing_element pe15_14(.reset(effective_rst), .clk(clk),  .in_a(a15_13to15_14), .in_b(b14_14to15_14),  .out_a(a15_14to15_15), .out_b(b15_14to16_14), .out_c(matrixC15_14));
processing_element pe15_15(.reset(effective_rst), .clk(clk),  .in_a(a15_14to15_15), .in_b(b14_15to15_15),  .out_a(a15_15to15_16), .out_b(b15_15to16_15), .out_c(matrixC15_15));
assign a_data_out = {a15_15to15_16,a14_15to14_16,a13_15to13_16,a12_15to12_16,a11_15to11_16,a10_15to10_16,a9_15to9_16,a8_15to8_16,a7_15to7_16,a6_15to6_16,a5_15to5_16,a4_15to4_16,a3_15to3_16,a2_15to2_16,a1_15to1_16,a0_15to0_16};
assign b_data_out = {b15_15to16_15,b15_14to16_14,b15_13to16_13,b15_12to16_12,b15_11to16_11,b15_10to16_10,b15_9to16_9,b15_8to16_8,b15_7to16_7,b15_6to16_6,b15_5to16_5,b15_4to16_4,b15_3to16_3,b15_2to16_2,b15_1to16_1,b15_0to16_0};

endmodule

module processing_element(
 reset, 
 clk, 
 in_a,
 in_b, 
 out_a, 
 out_b, 
 out_c
 );

 input reset;
 input clk;
 input  [`DWIDTH-1:0] in_a;
 input  [`DWIDTH-1:0] in_b;
 output [`DWIDTH-1:0] out_a;
 output [`DWIDTH-1:0] out_b;
 output [`DWIDTH-1:0] out_c;  //reduced precision

 reg [`DWIDTH-1:0] out_a;
 reg [`DWIDTH-1:0] out_b;
 wire [`DWIDTH-1:0] out_c;

 wire [`DWIDTH-1:0] out_mac;

 assign out_c = out_mac;

 seq_mac u_mac(.a(in_a), .b(in_b), .out(out_mac), .reset(reset), .clk(clk));

 always @(posedge clk)begin
    if(reset) begin
      out_a<=0;
      out_b<=0;
    end
    else begin  
      out_a<=in_a;
      out_b<=in_b;
    end
 end
 
endmodule

module seq_mac(a, b, out, reset, clk);
input [`DWIDTH-1:0] a;
input [`DWIDTH-1:0] b;
input reset;
input clk;
output [`DWIDTH-1:0] out;

reg [2*`DWIDTH-1:0] out_temp;
wire [`DWIDTH-1:0] mul_out;
wire [2*`DWIDTH-1:0] add_out;

reg [`DWIDTH-1:0] a_flopped;
reg [`DWIDTH-1:0] b_flopped;

wire [2*`DWIDTH-1:0] mul_out_temp;
reg [2*`DWIDTH-1:0] mul_out_temp_reg;

always @(posedge clk) begin
  if (reset) begin
    a_flopped <= 0;
    b_flopped <= 0;
  end else begin
    a_flopped <= a;
    b_flopped <= b;
  end
end

//assign mul_out = a * b;
qmult mult_u1(.i_multiplicand(a_flopped), .i_multiplier(b_flopped), .o_result(mul_out_temp));

always @(posedge clk) begin
  if (reset) begin
    mul_out_temp_reg <= 0;
  end else begin
    mul_out_temp_reg <= mul_out_temp;
  end
end

//we just truncate the higher bits of the product
//assign add_out = mul_out + out;
qadd add_u1(.a(out_temp), .b(mul_out_temp_reg), .c(add_out));

always @(posedge clk) begin
  if (reset) begin
    out_temp <= 0;
  end else begin
    out_temp <= add_out;
  end
end

//down cast the result
assign out = 
    (out_temp[2*`DWIDTH-1] == 0) ?  //positive number
        (
           (|(out_temp[2*`DWIDTH-2 : `DWIDTH-1])) ?  //is any bit from 14:7 is 1, that means overlfow
             {out_temp[2*`DWIDTH-1] , {(`DWIDTH-1){1'b1}}} : //sign bit and then all 1s
             {out_temp[2*`DWIDTH-1] , out_temp[`DWIDTH-2:0]} 
        )
        : //negative number
        (
           (|(out_temp[2*`DWIDTH-2 : `DWIDTH-1])) ?  //is any bit from 14:7 is 0, that means overlfow
             {out_temp[2*`DWIDTH-1] , out_temp[`DWIDTH-2:0]} :
             {out_temp[2*`DWIDTH-1] , {(`DWIDTH-1){1'b0}}} //sign bit and then all 0s
        );

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


//////////////////////////////////////////////
// Configuration block
//////////////////////////////////////////////

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
    output reg [`DWIDTH-1:0] mean,
    output reg [`DWIDTH-1:0] inv_var,
		output reg [`MAX_BITS_POOL-1:0] pool_window_size,
		output reg [`AWIDTH-1:0] address_mat_a,
    output reg [`AWIDTH-1:0] address_mat_b,
    output reg [`AWIDTH-1:0] address_mat_c,
    output reg [`MASK_WIDTH-1:0] validity_mask_a_rows,
    output reg [`MASK_WIDTH-1:0] validity_mask_a_cols,
    output reg [`MASK_WIDTH-1:0] validity_mask_b_rows,
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
    validity_mask_a_rows <= {`MASK_WIDTH{1'b1}};
    validity_mask_a_cols <= {`MASK_WIDTH{1'b1}};
    validity_mask_b_rows <= {`MASK_WIDTH{1'b1}};
    validity_mask_b_cols <= {`MASK_WIDTH{1'b1}};
    save_output_to_accum <= 0;
    add_accum_to_output <= 0;
    address_stride_a <= `DESIGN_SIZE;
    address_stride_b <= `DESIGN_SIZE;
    address_stride_c <= `DESIGN_SIZE;
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
          `REG_VALID_MASK_A_COLS_ADDR: begin
                                validity_mask_a_cols <= PWDATA[`MASK_WIDTH-1:0];
                                end
          `REG_VALID_MASK_B_ROWS_ADDR: begin
                                validity_mask_b_rows <= PWDATA[`MASK_WIDTH-1:0];
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
          `REG_VALID_MASK_A_COLS_ADDR: PRDATA <= validity_mask_a_cols;
          `REG_VALID_MASK_B_ROWS_ADDR: PRDATA <= validity_mask_b_rows;
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


////////////////////////////////////////////////
// Normalization block
////////////////////////////////////////////////

module norm(
    input enable_norm,
    input [`DWIDTH-1:0] mean,
    input [`DWIDTH-1:0] inv_var,
    input in_data_available,
    input [`DESIGN_SIZE*`DWIDTH-1:0] inp_data,
    output [`DESIGN_SIZE*`DWIDTH-1:0] out_data,
    output out_data_available,
    input [`MASK_WIDTH-1:0] validity_mask,
    output done_norm,
    input clk,
    input reset
);

reg out_data_available_internal;
wire [`DESIGN_SIZE*`DWIDTH-1:0] out_data_internal;
reg [`DESIGN_SIZE*`DWIDTH-1:0] mean_applied_data;
reg [`DESIGN_SIZE*`DWIDTH-1:0] variance_applied_data;
reg done_norm_internal;
reg norm_in_progress;
reg in_data_available_flopped;
reg [`DESIGN_SIZE*`DWIDTH-1:0] inp_data_flopped;

//Muxing logic to handle the case when this block is disabled
assign out_data_available = (enable_norm) ? out_data_available_internal : in_data_available_flopped;
assign out_data = (enable_norm) ? out_data_internal : inp_data_flopped;
assign done_norm = (enable_norm) ? done_norm_internal : 1'b1;

//inp_data will have multiple elements in it. the number of elements is the same as size of the matmul.
//on each clock edge, if in_data_available is 1, then we will normalize the inputs.

//the code uses the funky part-select syntax. example:
//wire [7:0] byteN = word[byte_num*8 +: 8];
//byte_num*8 is the starting point. 8 is the width is the part-select (has to be constant).in_data_available
//+: indicates the part-select increases from the starting point
//-: indicates the part-select decreases from the starting point
//another example:
//loc = 3;
//PA[loc -:4] = PA[loc+1 +:4];  // equivalent to PA[3:0] = PA[7:4];

reg [31:0] cycle_count;
reg [31:0] i;
always @(posedge clk) begin
    if ((reset || ~enable_norm)) begin
        mean_applied_data <= 0;
        variance_applied_data <= 0;
        out_data_available_internal <= 0;
        cycle_count <= 0;
        done_norm_internal <= 0;
        norm_in_progress <= 0;
        in_data_available_flopped <= in_data_available;
        inp_data_flopped <= inp_data;
    end else if (in_data_available || norm_in_progress) begin
        cycle_count = cycle_count + 1;
        //Let's apply mean and variance as the input data comes in.
        //We have a pipeline here. First stage does the add (to apply the mean)
        //and second stage does the multiplication (to apply the variance).
        //Note: the following loop is not a loop across multiple columns of data.
        //This loop will run in 2 cycle on the same column of data that comes into 
        //this module in 1 clock.
        for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
            if (validity_mask[i] == 1'b1) begin
                mean_applied_data[i*`DWIDTH +: `DWIDTH] <= (inp_data[i*`DWIDTH +: `DWIDTH] - mean);
                variance_applied_data[i*`DWIDTH +: `DWIDTH] <= (mean_applied_data[i*`DWIDTH +: `DWIDTH] * inv_var);
            end 
            else begin
                mean_applied_data[i*`DWIDTH +: `DWIDTH] <= (inp_data[i*`DWIDTH +: `DWIDTH]);
                variance_applied_data[i*`DWIDTH +: `DWIDTH] <= (mean_applied_data[i*`DWIDTH +: `DWIDTH]);
            end
        end

        //Out data is available starting with the second clock cycle because 
        //in the first cycle, we only apply the mean.
        if(cycle_count==2) begin
            out_data_available_internal <= 1;
        end

        //When we've normalized values N times, where N is the matmul
        //size, that means we're done. But there is one additional cycle
        //that is taken in the beginning (when we are applying the mean to the first
        //column of data). We can call this the Initiation Interval of the pipeline.
        //So, for a 4x4 matmul, this block takes 5 cycles.
        if(cycle_count==(`DESIGN_SIZE+1)) begin
            done_norm_internal <= 1'b1;
            norm_in_progress <= 0;
        end
        else begin
            norm_in_progress <= 1;
        end
    end
    else begin
        mean_applied_data <= 0;
        variance_applied_data <= 0;
        out_data_available_internal <= 0;
        cycle_count <= 0;
        done_norm_internal <= 0;
        norm_in_progress <= 0;
    end
end

assign out_data_internal = variance_applied_data;

endmodule

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

input [`AWIDTH-1:0] addr0;
input [`AWIDTH-1:0] addr1;
input [`DESIGN_SIZE*`DWIDTH-1:0] d0;
input [`DESIGN_SIZE*`DWIDTH-1:0] d1;
input [`DESIGN_SIZE-1:0] we0;
input [`DESIGN_SIZE-1:0] we1;
output reg [`DESIGN_SIZE*`DWIDTH-1:0] q0;
output reg [`DESIGN_SIZE*`DWIDTH-1:0] q1;
input clk;

`ifdef SIMULATION

reg [7:0] ram[((1<<`AWIDTH)-1):0];
reg [31:0] i;

always @(posedge clk)  
begin 
    for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
        if (we0[i]) ram[addr0+i] <= d0[i*`DWIDTH +: `DWIDTH]; 
    end    
    for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
        q0[i*`DWIDTH +: `DWIDTH] <= ram[addr0+i];
    end    
end

always @(posedge clk)  
begin 
    for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
        if (we1[i]) ram[addr0+i] <= d1[i*`DWIDTH +: `DWIDTH]; 
    end    
    for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
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

////////////////////////////////////////////////
// Control unit
////////////////////////////////////////////////

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

////////////////////////////////////////////////
// Pooling block
////////////////////////////////////////////////

module pool(
    input enable_pool,
    input in_data_available,
	  input [`MAX_BITS_POOL-1:0] pool_window_size,
    input [`DESIGN_SIZE*`DWIDTH-1:0] inp_data,
    output [`DESIGN_SIZE*`DWIDTH-1:0] out_data,
    output out_data_available,
    input [`MASK_WIDTH-1:0] validity_mask,
    output done_pool,
    input clk,
    input reset
);

reg in_data_available_flopped;
reg [`DESIGN_SIZE*`DWIDTH-1:0] inp_data_flopped;

reg [`DESIGN_SIZE*`DWIDTH-1:0] out_data_temp;
reg done_pool_temp;
reg out_data_available_temp;
reg [31:0] i,j;
reg [31:0] cycle_count;

always @(posedge clk) begin
	if (reset || ~enable_pool || ~in_data_available) begin
		out_data_temp <= 0;
		done_pool_temp <= 0;
		out_data_available_temp <= 0;
		cycle_count <= 0;
    in_data_available_flopped <= in_data_available;
    inp_data_flopped <= inp_data;
	end

	else if (in_data_available) begin
    cycle_count = cycle_count + 1;
		out_data_available_temp <= 1;

		case (pool_window_size)
			1: begin
				out_data_temp <= inp_data;
			end
			2: begin
				for (i = 0; i < `DESIGN_SIZE/2; i = i + 8) begin
					out_data_temp[ i +: 8] <= (inp_data[i*2 +: 8]  + inp_data[i*2 + 8 +: 8]) >> 1; 
				end
			end
			4: begin	
				for (i = 0; i < `DESIGN_SIZE/4; i = i + 8) begin
					//TODO: If 3 adders are the critical path, break into 2 cycles
					out_data_temp[ i +: 8] <= (inp_data[i*4 +: 8]  + inp_data[i*4 + 8 +: 8] + inp_data[i*4 + 16 +: 8]  + inp_data[i*4 + 24 +: 8]) >> 2; 
				end
			end
		endcase			

        if(cycle_count==`DESIGN_SIZE) begin	 
            done_pool_temp <= 1'b1;	      
        end	  
	end
end

assign out_data = enable_pool ? out_data_temp : inp_data_flopped; 
assign out_data_available = enable_pool ? out_data_available_temp : in_data_available_flopped;
assign done_pool = enable_pool ? done_pool_temp : 1'b1;

//Adding a dummy signal to use validity_mask input, to make ODIN happy
wire [`MASK_WIDTH-1:0] dummy;
assign dummy = validity_mask;

endmodule

////////////////////////////////////////////////
// Activation block
////////////////////////////////////////////////

module activation(
    input activation_type,
    input enable_activation,
    input in_data_available,
    input [`DESIGN_SIZE*`DWIDTH-1:0] inp_data,
    output [`DESIGN_SIZE*`DWIDTH-1:0] out_data,
    output out_data_available,
    input [`MASK_WIDTH-1:0] validity_mask,
    output done_activation,
    input clk,
    input reset
);

reg  done_activation_internal;
reg  out_data_available_internal;
wire [`DESIGN_SIZE*`DWIDTH-1:0] out_data_internal;
reg [`DESIGN_SIZE*`DWIDTH-1:0] slope_applied_data_internal;
reg [`DESIGN_SIZE*`DWIDTH-1:0] intercept_applied_data_internal;
reg [`DESIGN_SIZE*`DWIDTH-1:0] relu_applied_data_internal;
reg [31:0] i;
reg [31:0] cycle_count;
reg activation_in_progress;

reg [(`DESIGN_SIZE*4)-1:0] address;
reg [(`DESIGN_SIZE*8)-1:0] data_slope;
reg [(`DESIGN_SIZE*8)-1:0] data_slope_flopped;
reg [(`DESIGN_SIZE*8)-1:0] data_intercept;
reg [(`DESIGN_SIZE*8)-1:0] data_intercept_delayed;
reg [(`DESIGN_SIZE*8)-1:0] data_intercept_flopped;

reg in_data_available_flopped;
reg [`DESIGN_SIZE*`DWIDTH-1:0] inp_data_flopped;

always @(posedge clk) begin
  if (reset) begin
    inp_data_flopped <= 0;
    data_slope_flopped <= 0;
  end else begin
    inp_data_flopped <= inp_data;
    data_slope_flopped <= data_slope;
  end
end

// If the activation block is not enabled, just forward the input data
assign out_data             = enable_activation ? out_data_internal : inp_data_flopped;
assign done_activation      = enable_activation ? done_activation_internal : 1'b1;
assign out_data_available   = enable_activation ? out_data_available_internal : in_data_available_flopped;

always @(posedge clk) begin
   if (reset || ~enable_activation) begin
      slope_applied_data_internal     <= 0;
      intercept_applied_data_internal <= 0; 
      relu_applied_data_internal      <= 0; 
      data_intercept_delayed      <= 0;
      data_intercept_flopped      <= 0;
      done_activation_internal    <= 0;
      out_data_available_internal <= 0;
      cycle_count                 <= 0;
      activation_in_progress      <= 0;
      in_data_available_flopped <= in_data_available;
   end else if(in_data_available || activation_in_progress) begin
      cycle_count = cycle_count + 1;

      for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
         if(activation_type==1'b1) begin // tanH
            slope_applied_data_internal[i*`DWIDTH +:`DWIDTH] <= data_slope_flopped[i*8 +: 8] * inp_data_flopped[i*`DWIDTH +:`DWIDTH];
            data_intercept_flopped[i*8 +: 8] <= data_intercept[i*8 +: 8];
            data_intercept_delayed[i*8 +: 8] <= data_intercept_flopped[i*8 +: 8];
            intercept_applied_data_internal[i*`DWIDTH +:`DWIDTH] <= slope_applied_data_internal[i*`DWIDTH +:`DWIDTH] + data_intercept_delayed[i*8 +: 8];
         end else begin // ReLU
            relu_applied_data_internal[i*`DWIDTH +:`DWIDTH] <= inp_data[i*`DWIDTH] ? {`DWIDTH{1'b0}} : inp_data[i*`DWIDTH +:`DWIDTH];
         end
      end   

      //TANH needs 1 extra cycle
      if (activation_type==1'b1) begin
         if (cycle_count==3) begin
            out_data_available_internal <= 1;
         end
      end else begin
         if (cycle_count==2) begin
           out_data_available_internal <= 1;
         end
      end

      //TANH needs 1 extra cycle
      if (activation_type==1'b1) begin
        if(cycle_count==(`DESIGN_SIZE+2)) begin
           done_activation_internal <= 1'b1;
           activation_in_progress <= 0;
        end
        else begin
           activation_in_progress <= 1;
        end
      end else begin
        if(cycle_count==(`DESIGN_SIZE+1)) begin
           done_activation_internal <= 1'b1;
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
      data_intercept_flopped      <= 0;
      done_activation_internal    <= 0;
      out_data_available_internal <= 0;
      cycle_count                 <= 0;
      activation_in_progress      <= 0;
   end
end

assign out_data_internal = (activation_type) ? intercept_applied_data_internal : relu_applied_data_internal;

//Our equation of tanh is Y=AX+B
//A is the slope and B is the intercept.
//We store A in one LUT and B in another.
//LUT for the slope
always @(address) begin
    for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
    case (address[i*4+:4])
      4'b0000: data_slope[i*8+:8] = 8'd0;
      4'b0001: data_slope[i*8+:8] = 8'd0;
      4'b0010: data_slope[i*8+:8] = 8'd2;
      4'b0011: data_slope[i*8+:8] = 8'd3;
      4'b0100: data_slope[i*8+:8] = 8'd4;
      4'b0101: data_slope[i*8+:8] = 8'd0;
      4'b0110: data_slope[i*8+:8] = 8'd4;
      4'b0111: data_slope[i*8+:8] = 8'd3;
      4'b1000: data_slope[i*8+:8] = 8'd2;
      4'b1001: data_slope[i*8+:8] = 8'd0;
      4'b1010: data_slope[i*8+:8] = 8'd0;
      default: data_slope[i*8+:8] = 8'd0;
    endcase  
    end
end

//LUT for the intercept
always @(address) begin
    for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
    case (address[i*4+:4])
      4'b0000: data_intercept[i*8+:8] = 8'd127;
      4'b0001: data_intercept[i*8+:8] = 8'd99;
      4'b0010: data_intercept[i*8+:8] = 8'd46;
      4'b0011: data_intercept[i*8+:8] = 8'd18;
      4'b0100: data_intercept[i*8+:8] = 8'd0;
      4'b0101: data_intercept[i*8+:8] = 8'd0;
      4'b0110: data_intercept[i*8+:8] = 8'd0;
      4'b0111: data_intercept[i*8+:8] = -8'd18;
      4'b1000: data_intercept[i*8+:8] = -8'd46;
      4'b1001: data_intercept[i*8+:8] = -8'd99;
      4'b1010: data_intercept[i*8+:8] = -8'd127;
      default: data_intercept[i*8+:8] = 8'd0;
    endcase  
    end
end

//Logic to find address
always @(inp_data) begin
    for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
        if((inp_data[i*`DWIDTH +:`DWIDTH])>=90) begin
           address[i*4+:4] = 4'b0000;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>=39 && (inp_data[i*`DWIDTH +:`DWIDTH])<90) begin
           address[i*4+:4] = 4'b0001;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>=28 && (inp_data[i*`DWIDTH +:`DWIDTH])<39) begin
           address[i*4+:4] = 4'b0010;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>=16 && (inp_data[i*`DWIDTH +:`DWIDTH])<28) begin
           address[i*4+:4] = 4'b0011;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>=1 && (inp_data[i*`DWIDTH +:`DWIDTH])<16) begin
           address[i*4+:4] = 4'b0100;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])==0) begin
           address[i*4+:4] = 4'b0101;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>-16 && (inp_data[i*`DWIDTH +:`DWIDTH])<=-1) begin
           address[i*4+:4] = 4'b0110;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>-28 && (inp_data[i*`DWIDTH +:`DWIDTH])<=-16) begin
           address[i*4+:4] = 4'b0111;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>-39 && (inp_data[i*`DWIDTH +:`DWIDTH])<=-28) begin
           address[i*4+:4] = 4'b1000;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>-90 && (inp_data[i*`DWIDTH +:`DWIDTH])<=-39) begin
           address[i*4+:4] = 4'b1001;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])<=-90) begin
           address[i*4+:4] = 4'b1010;
        end
        else begin
           address[i*4+:4] = 4'b0101;
        end
    end
end

//Adding a dummy signal to use validity_mask input, to make ODIN happy
//TODO: Need to correctly use validity_mask
wire [`MASK_WIDTH-1:0] dummy;
assign dummy = validity_mask;

endmodule


//////////////////////////////////////////////////////
// Top module
//////////////////////////////////////////////////////

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
wire [`DESIGN_SIZE*`DWIDTH-1:0] matmul_c_data_out;
wire [`DESIGN_SIZE*`DWIDTH-1:0] norm_data_out;
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
wire [`MASK_WIDTH-1:0] validity_mask_a_cols;
wire [`MASK_WIDTH-1:0] validity_mask_b_rows;
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
ram matrix_A (
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
ram matrix_B (
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
  .validity_mask_a_rows(validity_mask_a_rows),
  .validity_mask_a_cols(validity_mask_a_cols),
  .validity_mask_b_rows(validity_mask_b_rows),
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
matmul_16x16_systolic u_matmul(
  .clk(clk),
  .reset(reset),
  .pe_reset(pe_reset),
  .start_mat_mul(start_mat_mul),
  .done_mat_mul(done_mat_mul),
  .address_mat_a(address_mat_a),
  .address_mat_b(address_mat_b),
  .address_mat_c(address_mat_c),
  .address_stride_a(address_stride_a),
  .address_stride_b(address_stride_b),
  .address_stride_c(address_stride_c),
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
  .validity_mask_a_rows(validity_mask_a_rows),
  .validity_mask_a_cols(validity_mask_a_cols),
  .validity_mask_b_rows(validity_mask_b_rows),
  .validity_mask_b_cols(validity_mask_b_cols),
  .final_mat_mul_size(8'd16),
  .a_loc(8'd0),
  .b_loc(8'd0)
);

////////////////////////////////////////////////////////////////
// Normalization module
////////////////////////////////////////////////////////////////
norm u_norm(
  .enable_norm(enable_norm),
  .mean(mean),
  .inv_var(inv_var),
  .in_data_available(matmul_c_data_available),
  .inp_data(matmul_c_data_out),
  .out_data(norm_data_out),
  .out_data_available(norm_out_data_available),
  .validity_mask(validity_mask_a_rows),
  .done_norm(done_norm),
  .clk(clk),
  .reset(reset)
);

////////////////////////////////////////////////////////////////
// Pooling module
////////////////////////////////////////////////////////////////
pool u_pool(
  .enable_pool(enable_pool),
  .in_data_available(norm_out_data_available),
  .pool_window_size(pool_window_size),
	.inp_data(norm_data_out),
  .out_data(pool_data_out),
  .out_data_available(pool_out_data_available),
  .validity_mask(validity_mask_a_rows),
  .done_pool(done_pool),
  .clk(clk),
  .reset(reset)
);

////////////////////////////////////////////////////////////////
// Activation module
////////////////////////////////////////////////////////////////
activation u_activation(
  .activation_type(activation_type),
  .enable_activation(enable_activation),
  .in_data_available(pool_out_data_available),
  .inp_data(pool_data_out),
  .out_data(activation_data_out),
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
always @(posedge clk) begin
  if (reset) begin
      bram_wdata_a <= 0;
      bram_addr_a_for_writing <= address_mat_c + address_stride_c;
      bram_a_wdata_available <= 0;
  end
  else if (activation_out_data_available) begin
      bram_wdata_a <= activation_data_out;
      bram_addr_a_for_writing <= bram_addr_a_for_writing - address_stride_c;
      bram_a_wdata_available <= activation_out_data_available;
  end
  else begin
      bram_wdata_a <= 0;
      bram_addr_a_for_writing <= address_mat_c + address_stride_c;
      bram_a_wdata_available <= 0;
  end
end  

endmodule
