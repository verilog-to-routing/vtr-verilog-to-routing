//////////////////////////////////////////////////////////////////////////////
// Author: Aman Arora
//////////////////////////////////////////////////////////////////////////////

`timescale 1ns/1ns
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
// Eltwise layer
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
// Precision BF16
//Each PE has 1 multiplier, an adder and a subtractor.
//There are 4 PEs in each compute unit. 
//There are 6 such compute units in the whole layer.
//So, total compute throughput is 24 ops per cycle.
//The "per cycle" is because the adder/sub/mul are
//pipelined. Although they may be take more than 1 cycle,
//but in the steady state, one result will come out every cycle.
//
//There are 6 BRAMs for each input operand. Each location in a BRAM
//stores 4 inputs. So, the read bandwidth is 24 elements
//per cycle. This matches the compute throughput. So, we
//utilize each PE every cycle. There are 6 BRAMs for output.
//We can write 4 elements per cycle.
//
//There are two modes of operation: 
// 1. Vector/Matrix mode
//    In this mode, both operands are matrices/vectors.
//    They are read from BRAMs (A and B). The operation 
//    selected (using the op input) is performed. This mode
//    can be used for operations such as residual add, or 
//    dropout.
// 2. Scalar mode
//    In this mode, one operand is a matrix/vector and the
//    other operand is a scalar. It could be the mean or 
//    variance of a normalization layer for example. The 
//    scalar input is provided from the top-level of the design
//    so it can be easily modified at runtime.
//
//Important inputs:
//   mode: 
//      0 -> Both operands (A and B) are matrices/vectors. Result is a matrix/vector.
//      1 -> Operand A is matrix/vector. Operand B is scalar. Result is a matrix/vector.
//   op:
//      00 -> Addition
//      01 -> Subtraction
//      10 -> Multiplication
//
//The whole design can operate on 24xN matrices.  
//Typically, to use this design, we'd break a large input
//matrix into 24 column sections and process the matrix 
//section by section. The number of rows will be programmed
//in the "iterations" register in the design.


`define BFLOAT16 

// IEEE Half Precision => EXPONENT = 5, MANTISSA = 10
// BFLOAT16 => EXPONENT = 8, MANTISSA = 7 

`ifdef BFLOAT16
`define EXPONENT 8
`define MANTISSA 7
`else // for ieee half precision fp16
`define EXPONENT 5
`define MANTISSA 10
`endif

`define SIGN 1
`define DWIDTH (`SIGN+`EXPONENT+`MANTISSA)

`define AWIDTH 10
`define MEM_SIZE 1024
`define DESIGN_SIZE 12
`define CU_SIZE 4
`define MASK_WIDTH 4
`define MEM_ACCESS_LATENCY 1

`define REG_DATAWIDTH 32
`define REG_ADDRWIDTH 8
`define ITERATIONS_WIDTH 32

`define REG_STDN_ADDR 32'h4
`define REG_MATRIX_A_ADDR 32'he
`define REG_MATRIX_B_ADDR 32'h12
`define REG_MATRIX_C_ADDR 32'h16
`define REG_VALID_MASK_A_ADDR 32'h20
`define REG_VALID_MASK_B_ADDR 32'h5c

`define REG_ITERATIONS_ADDR 32'h40

//This is the pipeline depth of the PEs (adder/mult)
`define PE_PIPELINE_DEPTH 5

module eltwise_layer(
  input clk,
  input clk_mem,
  input resetn,
  input pe_resetn,
  input        [`REG_ADDRWIDTH-1:0] PADDR,
  input                             PWRITE,
  input                             PSEL,
  input                             PENABLE,
  input        [`REG_DATAWIDTH-1:0] PWDATA,
  output reg   [`REG_DATAWIDTH-1:0] PRDATA,
  output reg                        PREADY,
  input [`DWIDTH-1:0] scalar_inp,
  input mode, // mode==0 -> vector/matrix, mode==1 -> scalar
  input  [1:0] op, //op==11 -> Mul, op==01 -> Sub, op==00 -> Add
  input  [7:0] bram_select,
  input  [`AWIDTH-1:0] bram_addr_ext,
  output reg [`CU_SIZE*`DWIDTH-1:0] bram_rdata_ext,
  input  [`CU_SIZE*`DWIDTH-1:0] bram_wdata_ext,
  input  [`CU_SIZE-1:0] bram_we_ext
);


  wire PCLK;
  assign PCLK = clk;
  wire PRESETn;
  assign PRESETn = resetn;
  reg start_reg;
  reg clear_done_reg;

  //Dummy register to sync all other invalid/unimplemented addresses
  reg [`REG_DATAWIDTH-1:0] reg_dummy;
  
  reg [`AWIDTH-1:0] bram_addr_a_0_ext;
  wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_a_0_ext;
  reg [`CU_SIZE*`DWIDTH-1:0] bram_wdata_a_0_ext;
  reg [`MASK_WIDTH-1:0] bram_we_a_0_ext;
    
  reg [`AWIDTH-1:0] bram_addr_a_2_ext;
  wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_a_2_ext;
  reg [`CU_SIZE*`DWIDTH-1:0] bram_wdata_a_2_ext;
  reg [`MASK_WIDTH-1:0] bram_we_a_2_ext;
    
  reg [`AWIDTH-1:0] bram_addr_a_4_ext;
  wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_a_4_ext;
  reg [`CU_SIZE*`DWIDTH-1:0] bram_wdata_a_4_ext;
  reg [`MASK_WIDTH-1:0] bram_we_a_4_ext;

  reg [`AWIDTH-1:0] bram_addr_a_1_ext;
  wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_a_1_ext;
  reg [`CU_SIZE*`DWIDTH-1:0] bram_wdata_a_1_ext;
  reg [`MASK_WIDTH-1:0] bram_we_a_1_ext;
    
  reg [`AWIDTH-1:0] bram_addr_a_3_ext;
  wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_a_3_ext;
  reg [`CU_SIZE*`DWIDTH-1:0] bram_wdata_a_3_ext;
  reg [`MASK_WIDTH-1:0] bram_we_a_3_ext;
    
  reg [`AWIDTH-1:0] bram_addr_a_5_ext;
  wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_a_5_ext;
  reg [`CU_SIZE*`DWIDTH-1:0] bram_wdata_a_5_ext;
  reg [`MASK_WIDTH-1:0] bram_we_a_5_ext;

    
  reg [`AWIDTH-1:0] bram_addr_b_0_ext;
  wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_b_0_ext;
  reg [`CU_SIZE*`DWIDTH-1:0] bram_wdata_b_0_ext;
  reg [`MASK_WIDTH-1:0] bram_we_b_0_ext;
    
  reg [`AWIDTH-1:0] bram_addr_b_1_ext;
  wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_b_1_ext;
  reg [`CU_SIZE*`DWIDTH-1:0] bram_wdata_b_1_ext;
  reg [`MASK_WIDTH-1:0] bram_we_b_1_ext;
    
  reg [`AWIDTH-1:0] bram_addr_b_2_ext;
  wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_b_2_ext;
  reg [`CU_SIZE*`DWIDTH-1:0] bram_wdata_b_2_ext;
  reg [`MASK_WIDTH-1:0] bram_we_b_2_ext;
    
  reg [`AWIDTH-1:0] bram_addr_b_3_ext;
  wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_b_3_ext;
  reg [`CU_SIZE*`DWIDTH-1:0] bram_wdata_b_3_ext;
  reg [`MASK_WIDTH-1:0] bram_we_b_3_ext;
    
  reg [`AWIDTH-1:0] bram_addr_b_4_ext;
  wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_b_4_ext;
  reg [`CU_SIZE*`DWIDTH-1:0] bram_wdata_b_4_ext;
  reg [`MASK_WIDTH-1:0] bram_we_b_4_ext;
    
  reg [`AWIDTH-1:0] bram_addr_b_5_ext;
  wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_b_5_ext;
  reg [`CU_SIZE*`DWIDTH-1:0] bram_wdata_b_5_ext;
  reg [`MASK_WIDTH-1:0] bram_we_b_5_ext;

  reg [`AWIDTH-1:0] bram_addr_c_0_ext;
  wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_c_0_ext;
  reg [`CU_SIZE*`DWIDTH-1:0] bram_wdata_c_0_ext;
  reg [`MASK_WIDTH-1:0] bram_we_c_0_ext;
    
  reg [`AWIDTH-1:0] bram_addr_c_1_ext;
  wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_c_1_ext;
  reg [`CU_SIZE*`DWIDTH-1:0] bram_wdata_c_1_ext;
  reg [`MASK_WIDTH-1:0] bram_we_c_1_ext;
    
  reg [`AWIDTH-1:0] bram_addr_c_2_ext;
  wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_c_2_ext;
  reg [`CU_SIZE*`DWIDTH-1:0] bram_wdata_c_2_ext;
  reg [`MASK_WIDTH-1:0] bram_we_c_2_ext;
    
  reg [`AWIDTH-1:0] bram_addr_c_3_ext;
  wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_c_3_ext;
  reg [`CU_SIZE*`DWIDTH-1:0] bram_wdata_c_3_ext;
  reg [`MASK_WIDTH-1:0] bram_we_c_3_ext;
    
  reg [`AWIDTH-1:0] bram_addr_c_4_ext;
  wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_c_4_ext;
  reg [`CU_SIZE*`DWIDTH-1:0] bram_wdata_c_4_ext;
  reg [`MASK_WIDTH-1:0] bram_we_c_4_ext;
    
  reg [`AWIDTH-1:0] bram_addr_c_5_ext;
  wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_c_5_ext;
  reg [`CU_SIZE*`DWIDTH-1:0] bram_wdata_c_5_ext;
  reg [`MASK_WIDTH-1:0] bram_we_c_5_ext;
    
	wire [`AWIDTH-1:0] bram_addr_a_0;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_a_0;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_wdata_a_0;
	wire [`MASK_WIDTH-1:0] bram_we_a_0;
	wire bram_en_a_0;
    
	wire [`AWIDTH-1:0] bram_addr_a_2;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_a_2;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_wdata_a_2;
	wire [`MASK_WIDTH-1:0] bram_we_a_2;
	wire bram_en_a_2;
    
	wire [`AWIDTH-1:0] bram_addr_a_4;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_a_4;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_wdata_a_4;
	wire [`MASK_WIDTH-1:0] bram_we_a_4;
	wire bram_en_a_4;

	wire [`AWIDTH-1:0] bram_addr_a_1;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_a_1;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_wdata_a_1;
	wire [`MASK_WIDTH-1:0] bram_we_a_1;
	wire bram_en_a_1;
    
	wire [`AWIDTH-1:0] bram_addr_a_3;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_a_3;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_wdata_a_3;
	wire [`MASK_WIDTH-1:0] bram_we_a_3;
	wire bram_en_a_3;
    
	wire [`AWIDTH-1:0] bram_addr_a_5;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_a_5;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_wdata_a_5;
	wire [`MASK_WIDTH-1:0] bram_we_a_5;
	wire bram_en_a_5;
    
	wire [`AWIDTH-1:0] bram_addr_b_0;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_b_0;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_wdata_b_0;
	wire [`MASK_WIDTH-1:0] bram_we_b_0;
	wire bram_en_b_0;
    
	wire [`AWIDTH-1:0] bram_addr_b_1;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_b_1;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_wdata_b_1;
	wire [`MASK_WIDTH-1:0] bram_we_b_1;
	wire bram_en_b_1;
    
	wire [`AWIDTH-1:0] bram_addr_b_2;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_b_2;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_wdata_b_2;
	wire [`MASK_WIDTH-1:0] bram_we_b_2;
	wire bram_en_b_2;
    
	wire [`AWIDTH-1:0] bram_addr_b_3;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_b_3;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_wdata_b_3;
	wire [`MASK_WIDTH-1:0] bram_we_b_3;
	wire bram_en_b_3;

  wire [`AWIDTH-1:0] bram_addr_b_4;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_b_4;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_wdata_b_4;
	wire [`MASK_WIDTH-1:0] bram_we_b_4;
	wire bram_en_b_4;
    
	wire [`AWIDTH-1:0] bram_addr_b_5;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_b_5;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_wdata_b_5;
	wire [`MASK_WIDTH-1:0] bram_we_b_5;
	wire bram_en_b_5;

	wire [`AWIDTH-1:0] bram_addr_c_0;
	wire [`AWIDTH-1:0] bram_addr_c_1;
	wire [`AWIDTH-1:0] bram_addr_c_2;
	wire [`AWIDTH-1:0] bram_addr_c_3;
	wire [`AWIDTH-1:0] bram_addr_c_4;
	wire [`AWIDTH-1:0] bram_addr_c_5;

	wire [`CU_SIZE*`DWIDTH-1:0] bram_wdata_c_0;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_wdata_c_1;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_wdata_c_2;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_wdata_c_3;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_wdata_c_4;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_wdata_c_5;

	wire [`MASK_WIDTH-1:0] bram_we_c_0;
	wire [`MASK_WIDTH-1:0] bram_we_c_1;
	wire [`MASK_WIDTH-1:0] bram_we_c_2;
	wire [`MASK_WIDTH-1:0] bram_we_c_3;
	wire [`MASK_WIDTH-1:0] bram_we_c_4;
	wire [`MASK_WIDTH-1:0] bram_we_c_5;
    
	wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_c_0;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_c_1;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_c_2;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_c_3;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_c_4;
	wire [`CU_SIZE*`DWIDTH-1:0] bram_rdata_c_5;

  always @* begin
    case (bram_select)
  
      0: begin
      bram_addr_a_0_ext = bram_addr_ext;
      bram_wdata_a_0_ext = bram_wdata_ext;
      bram_we_a_0_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_a_0_ext;
      end
    
      1: begin
      bram_addr_a_2_ext = bram_addr_ext;
      bram_wdata_a_2_ext = bram_wdata_ext;
      bram_we_a_2_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_a_2_ext;
      end
    
      2: begin
      bram_addr_a_4_ext = bram_addr_ext;
      bram_wdata_a_4_ext = bram_wdata_ext;
      bram_we_a_4_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_a_4_ext;
      end

      3: begin
      bram_addr_a_1_ext = bram_addr_ext;
      bram_wdata_a_1_ext = bram_wdata_ext;
      bram_we_a_1_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_a_1_ext;
      end
    
      4: begin
      bram_addr_a_3_ext = bram_addr_ext;
      bram_wdata_a_3_ext = bram_wdata_ext;
      bram_we_a_3_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_a_3_ext;
      end
    
      5: begin
      bram_addr_a_5_ext = bram_addr_ext;
      bram_wdata_a_5_ext = bram_wdata_ext;
      bram_we_a_5_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_a_5_ext;
      end
    
      6: begin
      bram_addr_b_0_ext = bram_addr_ext;
      bram_wdata_b_0_ext = bram_wdata_ext;
      bram_we_b_0_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_b_0_ext;
      end
    
      7: begin
      bram_addr_b_1_ext = bram_addr_ext;
      bram_wdata_b_1_ext = bram_wdata_ext;
      bram_we_b_1_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_b_1_ext;
      end
    
      8: begin
      bram_addr_b_2_ext = bram_addr_ext;
      bram_wdata_b_2_ext = bram_wdata_ext;
      bram_we_b_2_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_b_2_ext;
      end
    
      9: begin
      bram_addr_b_3_ext = bram_addr_ext;
      bram_wdata_b_3_ext = bram_wdata_ext;
      bram_we_b_3_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_b_3_ext;
      end
    
      10: begin
      bram_addr_b_4_ext = bram_addr_ext;
      bram_wdata_b_4_ext = bram_wdata_ext;
      bram_we_b_4_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_b_4_ext;
      end
    
      11: begin
      bram_addr_b_5_ext = bram_addr_ext;
      bram_wdata_b_5_ext = bram_wdata_ext;
      bram_we_b_5_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_b_5_ext;
      end

      12: begin
      bram_addr_c_0_ext = bram_addr_ext;
      bram_wdata_c_0_ext = bram_wdata_ext;
      bram_we_c_0_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_c_0_ext;
      end
    
      13: begin
      bram_addr_c_1_ext = bram_addr_ext;
      bram_wdata_c_1_ext = bram_wdata_ext;
      bram_we_c_1_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_c_1_ext;
      end
    
      14: begin
      bram_addr_c_2_ext = bram_addr_ext;
      bram_wdata_c_2_ext = bram_wdata_ext;
      bram_we_c_2_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_c_2_ext;
      end
    
      15: begin
      bram_addr_c_3_ext = bram_addr_ext;
      bram_wdata_c_3_ext = bram_wdata_ext;
      bram_we_c_3_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_c_3_ext;
      end
    
      16: begin
      bram_addr_c_4_ext = bram_addr_ext;
      bram_wdata_c_4_ext = bram_wdata_ext;
      bram_we_c_4_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_c_4_ext;
      end
    
      17: begin
      bram_addr_c_5_ext = bram_addr_ext;
      bram_wdata_c_5_ext = bram_wdata_ext;
      bram_we_c_5_ext = bram_we_ext;
      bram_rdata_ext = bram_rdata_c_5_ext;
      end
    
      default: begin
      bram_rdata_ext = 0;
      end
    endcase 
  end
  
/////////////////////////////////////////////////
// BRAMs to store matrix A
/////////////////////////////////////////////////


  // BRAM matrix A 0
ram matrix_A_0(
  .addr0(bram_addr_a_0),
  .d0(bram_wdata_a_0), 
  .we0(bram_we_a_0), 
  .q0(bram_rdata_a_0), 
  .addr1(bram_addr_a_0_ext),
  .d1(bram_wdata_a_0_ext), 
  .we1(bram_we_a_0_ext), 
  .q1(bram_rdata_a_0_ext), 
  .clk(clk_mem));
  	
  // BRAM matrix A 2
ram matrix_A_2(
  .addr0(bram_addr_a_2),
  .d0(bram_wdata_a_2), 
  .we0(bram_we_a_2), 
  .q0(bram_rdata_a_2), 
  .addr1(bram_addr_a_2_ext),
  .d1(bram_wdata_a_2_ext), 
  .we1(bram_we_a_2_ext), 
  .q1(bram_rdata_a_2_ext), 
  .clk(clk_mem));
  	
  // BRAM matrix A 4
ram matrix_A_4(
  .addr0(bram_addr_a_4),
  .d0(bram_wdata_a_4), 
  .we0(bram_we_a_4), 
  .q0(bram_rdata_a_4), 
  .addr1(bram_addr_a_4_ext),
  .d1(bram_wdata_a_4_ext), 
  .we1(bram_we_a_4_ext), 
  .q1(bram_rdata_a_4_ext), 
  .clk(clk_mem));


    // BRAM matrix A 1
ram matrix_A_1(
  .addr0(bram_addr_a_1),
  .d0(bram_wdata_a_1), 
  .we0(bram_we_a_1), 
  .q0(bram_rdata_a_1), 
  .addr1(bram_addr_a_1_ext),
  .d1(bram_wdata_a_1_ext), 
  .we1(bram_we_a_1_ext), 
  .q1(bram_rdata_a_1_ext), 
  .clk(clk_mem));
  	
  // BRAM matrix A 3
ram matrix_A_3(
  .addr0(bram_addr_a_3),
  .d0(bram_wdata_a_3), 
  .we0(bram_we_a_3), 
  .q0(bram_rdata_a_3), 
  .addr1(bram_addr_a_3_ext),
  .d1(bram_wdata_a_3_ext), 
  .we1(bram_we_a_3_ext), 
  .q1(bram_rdata_a_3_ext), 
  .clk(clk_mem));
  	
  // BRAM matrix A 5
ram matrix_A_5(
  .addr0(bram_addr_a_5),
  .d0(bram_wdata_a_5), 
  .we0(bram_we_a_5), 
  .q0(bram_rdata_a_5), 
  .addr1(bram_addr_a_5_ext),
  .d1(bram_wdata_a_5_ext), 
  .we1(bram_we_a_5_ext), 
  .q1(bram_rdata_a_5_ext), 
  .clk(clk_mem));

////////////////////////////////////////////////
// BRAMs to store matrix B
/////////////////////////////////////////////////


  // BRAM matrix B 0
ram matrix_B_0(
  .addr0(bram_addr_b_0),
  .d0(bram_wdata_b_0), 
  .we0(bram_we_b_0), 
  .q0(bram_rdata_b_0), 
  .addr1(bram_addr_b_0_ext),
  .d1(bram_wdata_b_0_ext), 
  .we1(bram_we_b_0_ext), 
  .q1(bram_rdata_b_0_ext), 
  .clk(clk_mem));
  	
  // BRAM matrix B 1
ram matrix_B_1(
  .addr0(bram_addr_b_1),
  .d0(bram_wdata_b_1), 
  .we0(bram_we_b_1), 
  .q0(bram_rdata_b_1), 
  .addr1(bram_addr_b_1_ext),
  .d1(bram_wdata_b_1_ext), 
  .we1(bram_we_b_1_ext), 
  .q1(bram_rdata_b_1_ext), 
  .clk(clk_mem));
  	
  // BRAM matrix B 2
ram matrix_B_2(
  .addr0(bram_addr_b_2),
  .d0(bram_wdata_b_2), 
  .we0(bram_we_b_2), 
  .q0(bram_rdata_b_2), 
  .addr1(bram_addr_b_2_ext),
  .d1(bram_wdata_b_2_ext), 
  .we1(bram_we_b_2_ext), 
  .q1(bram_rdata_b_2_ext), 
  .clk(clk_mem));

  	
  // BRAM matrix B 3
ram matrix_B_3(
  .addr0(bram_addr_b_3),
  .d0(bram_wdata_b_3), 
  .we0(bram_we_b_3), 
  .q0(bram_rdata_b_3), 
  .addr1(bram_addr_b_3_ext),
  .d1(bram_wdata_b_3_ext), 
  .we1(bram_we_b_3_ext), 
  .q1(bram_rdata_b_3_ext), 
  .clk(clk_mem));
  	
  // BRAM matrix B 4
ram matrix_B_4(
  .addr0(bram_addr_b_4),
  .d0(bram_wdata_b_4), 
  .we0(bram_we_b_4), 
  .q0(bram_rdata_b_4), 
  .addr1(bram_addr_b_4_ext),
  .d1(bram_wdata_b_4_ext), 
  .we1(bram_we_b_4_ext), 
  .q1(bram_rdata_b_4_ext), 
  .clk(clk_mem));


  // BRAM matrix B 5
ram matrix_B_5(
  .addr0(bram_addr_b_5),
  .d0(bram_wdata_b_5), 
  .we0(bram_we_b_5), 
  .q0(bram_rdata_b_5), 
  .addr1(bram_addr_b_5_ext),
  .d1(bram_wdata_b_5_ext), 
  .we1(bram_we_b_5_ext), 
  .q1(bram_rdata_b_5_ext), 
  .clk(clk_mem));

////////////////////////////////////////////////
// BRAMs to store matrix C
/////////////////////////////////////////////////


  // BRAM matrix C 0
ram matrix_C_0(
  .addr0(bram_addr_c_0),
  .d0(bram_wdata_c_0), 
  .we0(bram_we_c_0), 
  .q0(bram_rdata_c_0), 
  .addr1(bram_addr_c_0_ext),
  .d1(bram_wdata_c_0_ext), 
  .we1(bram_we_c_0_ext), 
  .q1(bram_rdata_c_0_ext), 
  .clk(clk_mem));
  	
  // BRAM matrix C 1
ram matrix_C_1(
  .addr0(bram_addr_c_1),
  .d0(bram_wdata_c_1), 
  .we0(bram_we_c_1), 
  .q0(bram_rdata_c_1), 
  .addr1(bram_addr_c_1_ext),
  .d1(bram_wdata_c_1_ext), 
  .we1(bram_we_c_1_ext), 
  .q1(bram_rdata_c_1_ext), 
  .clk(clk_mem));
  	
  // BRAM matrix C 2
ram matrix_C_2(
  .addr0(bram_addr_c_2),
  .d0(bram_wdata_c_2), 
  .we0(bram_we_c_2), 
  .q0(bram_rdata_c_2), 
  .addr1(bram_addr_c_2_ext),
  .d1(bram_wdata_c_2_ext), 
  .we1(bram_we_c_2_ext), 
  .q1(bram_rdata_c_2_ext), 
  .clk(clk_mem));

  	
  // BRAM matrix C 3
ram matrix_C_3(
  .addr0(bram_addr_c_3),
  .d0(bram_wdata_c_3), 
  .we0(bram_we_c_3), 
  .q0(bram_rdata_c_3), 
  .addr1(bram_addr_c_3_ext),
  .d1(bram_wdata_c_3_ext), 
  .we1(bram_we_c_3_ext), 
  .q1(bram_rdata_c_3_ext), 
  .clk(clk_mem));
  	
  // BRAM matrix C 4
ram matrix_C_4(
  .addr0(bram_addr_c_4),
  .d0(bram_wdata_c_4), 
  .we0(bram_we_c_4), 
  .q0(bram_rdata_c_4), 
  .addr1(bram_addr_c_4_ext),
  .d1(bram_wdata_c_4_ext), 
  .we1(bram_we_c_4_ext), 
  .q1(bram_rdata_c_4_ext), 
  .clk(clk_mem));


  // BRAM matrix C 5
ram matrix_C_5(
  .addr0(bram_addr_c_5),
  .d0(bram_wdata_c_5), 
  .we0(bram_we_c_5), 
  .q0(bram_rdata_c_5), 
  .addr1(bram_addr_c_5_ext),
  .d1(bram_wdata_c_5_ext), 
  .we1(bram_we_c_5_ext), 
  .q1(bram_rdata_c_5_ext), 
  .clk(clk_mem));
  	
reg start_eltwise_op;
wire done_eltwise_op;

reg [3:0] state;
	
////////////////////////////////////////////////////////////////
// Control logic
////////////////////////////////////////////////////////////////
	always @( posedge clk) begin
      if (resetn == 1'b0) begin
        state <= 4'b0000;
        start_eltwise_op <= 1'b0;
      end 
      else begin
        case (state)

        4'b0000: begin
          start_eltwise_op <= 1'b0;
          if (start_reg == 1'b1) begin
            state <= 4'b0001;
          end else begin
            state <= 4'b0000;
          end
        end
        
        4'b0001: begin
          start_eltwise_op <= 1'b1;	      
          state <= 4'b1010;                    
        end      
        
        4'b1010: begin                 
          if (done_eltwise_op == 1'b1) begin
            start_eltwise_op <= 1'b0;
            state <= 4'b1000;
          end
          else begin
            state <= 4'b1010;
          end
        end

       4'b1000: begin
         if (clear_done_reg == 1'b1) begin
           state <= 4'b0000;
         end
         else begin
           state <= 4'b1000;
         end
       end
      endcase  
	end 
  end

reg [1:0] state_apb;
`define IDLE     2'b00
`define W_ENABLE  2'b01
`define R_ENABLE  2'b10

reg [`AWIDTH-1:0] address_mat_a;
reg [`AWIDTH-1:0] address_mat_b;
reg [`AWIDTH-1:0] address_mat_c;
reg [`MASK_WIDTH-1:0] validity_mask_a;
reg [`MASK_WIDTH-1:0] validity_mask_b;
reg [`ITERATIONS_WIDTH-1:0] iterations;

////////////////////////////////////////////////////////////////
// Configuration logic
////////////////////////////////////////////////////////////////
always @(posedge PCLK) begin
  if (PRESETn == 0) begin
    state_apb <= `IDLE;
    PRDATA <= 0;
    PREADY <= 0;
    address_mat_a <= 0;
    address_mat_b <= 0;
    address_mat_c <= 0;
    validity_mask_a <= {`MASK_WIDTH{1'b1}};
    validity_mask_b <= {`MASK_WIDTH{1'b1}};
  end

  else begin
    case (state_apb)
      `IDLE : begin
        PRDATA <= 0;
        if (PSEL) begin
          if (PWRITE) begin
            state_apb <= `W_ENABLE;
          end
          else begin
            state_apb <= `R_ENABLE;
          end
        end
        PREADY <= 0;
      end

      `W_ENABLE : begin
        if (PSEL && PWRITE && PENABLE) begin
          case (PADDR)
          `REG_STDN_ADDR       : begin
                                 start_reg <= PWDATA[0];
                                 clear_done_reg <= PWDATA[31];
                                 end
          `REG_MATRIX_A_ADDR   : address_mat_a <= PWDATA[`AWIDTH-1:0];
          `REG_MATRIX_B_ADDR   : address_mat_b <= PWDATA[`AWIDTH-1:0];
          `REG_MATRIX_C_ADDR   : address_mat_c <= PWDATA[`AWIDTH-1:0];
          `REG_VALID_MASK_A_ADDR: begin
                                validity_mask_a <= PWDATA[`MASK_WIDTH-1:0];
                                end
          `REG_VALID_MASK_B_ADDR: begin
                                validity_mask_b <= PWDATA[`MASK_WIDTH-1:0];
                                end
          `REG_ITERATIONS_ADDR: iterations <= PWDATA[`ITERATIONS_WIDTH-1:0];
          default : reg_dummy <= PWDATA; //sink writes to a dummy register
          endcase
          PREADY <=1;          
        end
        state_apb <= `IDLE;
      end

      `R_ENABLE : begin
        if (PSEL && !PWRITE && PENABLE) begin
          PREADY <= 1;
          case (PADDR)
          `REG_STDN_ADDR        : PRDATA <= {done_eltwise_op, 30'b0, start_eltwise_op};
          `REG_MATRIX_A_ADDR    : PRDATA <= address_mat_a;
          `REG_MATRIX_B_ADDR    : PRDATA <= address_mat_b;
          `REG_MATRIX_C_ADDR    : PRDATA <= address_mat_c;
          `REG_VALID_MASK_A_ADDR: PRDATA <= validity_mask_a;
          `REG_VALID_MASK_B_ADDR: PRDATA <= validity_mask_b;
          `REG_ITERATIONS_ADDR: PRDATA <= iterations;
          default : PRDATA <= reg_dummy; //read the dummy register for undefined addresses
          endcase
        end
        state_apb <= `IDLE;
      end
      default: begin
        state_apb <= `IDLE;
      end
    endcase
  end
end  
  
wire reset;
assign reset = ~resetn;
wire pe_reset;
assign pe_reset = ~pe_resetn;

  wire c_data_0_available;
  wire c_data_1_available;
  wire c_data_2_available;
  wire c_data_3_available;
  wire c_data_4_available;
  wire c_data_5_available;

  assign bram_wdata_a_0 = {`CU_SIZE*`DWIDTH{1'b0}};
  assign bram_en_a_0 = 1'b1;
  assign bram_we_a_0 = {`MASK_WIDTH{1'b0}};

  assign bram_wdata_a_1 = {`CU_SIZE*`DWIDTH{1'b0}};
  assign bram_en_a_1 = 1'b1;
  assign bram_we_a_1 = {`MASK_WIDTH{1'b0}};

  assign bram_wdata_a_2 = {`CU_SIZE*`DWIDTH{1'b0}};
  assign bram_en_a_2 = 1'b1;
  assign bram_we_a_2 = {`MASK_WIDTH{1'b0}};

  assign bram_wdata_a_3 = {`CU_SIZE*`DWIDTH{1'b0}};
  assign bram_en_a_3 = 1'b1;
  assign bram_we_a_3 = {`MASK_WIDTH{1'b0}};

  assign bram_wdata_a_4 = {`CU_SIZE*`DWIDTH{1'b0}};
  assign bram_en_a_4 = 1'b1;
  assign bram_we_a_4 = {`MASK_WIDTH{1'b0}};

  assign bram_wdata_a_5 = {`CU_SIZE*`DWIDTH{1'b0}};
  assign bram_en_a_5 = 1'b1;
  assign bram_we_a_5 = {`MASK_WIDTH{1'b0}};
  	
  assign bram_wdata_b_0 = {`CU_SIZE*`DWIDTH{1'b0}};
  assign bram_en_b_0 = 1'b1;
  assign bram_we_b_0 = {`MASK_WIDTH{1'b0}};

  assign bram_wdata_b_1 = {`CU_SIZE*`DWIDTH{1'b0}};
  assign bram_en_b_1 = 1'b1;
  assign bram_we_b_1 = {`MASK_WIDTH{1'b0}};

  assign bram_wdata_b_2 = {`CU_SIZE*`DWIDTH{1'b0}};
  assign bram_en_b_2 = 1'b1;
  assign bram_we_b_2 = {`MASK_WIDTH{1'b0}};

  assign bram_wdata_b_3 = {`CU_SIZE*`DWIDTH{1'b0}};
  assign bram_en_b_3 = 1'b1;
  assign bram_we_b_3 = {`MASK_WIDTH{1'b0}};

  assign bram_wdata_b_4 = {`CU_SIZE*`DWIDTH{1'b0}};
  assign bram_en_b_4 = 1'b1;
  assign bram_we_b_4 = {`MASK_WIDTH{1'b0}};

  assign bram_wdata_b_5 = {`CU_SIZE*`DWIDTH{1'b0}};
  assign bram_en_b_5 = 1'b1;
  assign bram_we_b_5 = {`MASK_WIDTH{1'b0}};

  assign bram_we_c_0 = (c_data_0_available) ? {`MASK_WIDTH{1'b1}} : {`MASK_WIDTH{1'b0}};  
  assign bram_we_c_2 = (c_data_2_available) ? {`MASK_WIDTH{1'b1}} : {`MASK_WIDTH{1'b0}};  
  assign bram_we_c_4 = (c_data_4_available) ? {`MASK_WIDTH{1'b1}} : {`MASK_WIDTH{1'b0}};  
  assign bram_we_c_1 = (c_data_1_available) ? {`MASK_WIDTH{1'b1}} : {`MASK_WIDTH{1'b0}};  
  assign bram_we_c_3 = (c_data_3_available) ? {`MASK_WIDTH{1'b1}} : {`MASK_WIDTH{1'b0}};  
  assign bram_we_c_5 = (c_data_5_available) ? {`MASK_WIDTH{1'b1}} : {`MASK_WIDTH{1'b0}};  

  /////////////////////////////////////////////////
  // ORing all done signals
  /////////////////////////////////////////////////
  wire done_eltwise_op_0;
  wire done_eltwise_op_1;
  wire done_eltwise_op_2;
  wire done_eltwise_op_3;
  wire done_eltwise_op_4;
  wire done_eltwise_op_5;

  assign done_eltwise_op = 
  done_eltwise_op_0 | 
  done_eltwise_op_1 | 
  done_eltwise_op_2 | 
  done_eltwise_op_3 | 
  done_eltwise_op_4 | 
  done_eltwise_op_5 ;

  /////////////////////////////////////////////////
  // Code to allow for scalar mode
  /////////////////////////////////////////////////
  
	wire [`CU_SIZE*`DWIDTH-1:0] b_data_0;
	wire [`CU_SIZE*`DWIDTH-1:0] b_data_1;
	wire [`CU_SIZE*`DWIDTH-1:0] b_data_2;
	wire [`CU_SIZE*`DWIDTH-1:0] b_data_3;
	wire [`CU_SIZE*`DWIDTH-1:0] b_data_4;
	wire [`CU_SIZE*`DWIDTH-1:0] b_data_5;

  assign b_data_0 = mode ? bram_rdata_b_0 : {`CU_SIZE{scalar_inp}};
  assign b_data_1 = mode ? bram_rdata_b_1 : {`CU_SIZE{scalar_inp}};
  assign b_data_2 = mode ? bram_rdata_b_2 : {`CU_SIZE{scalar_inp}};
  assign b_data_3 = mode ? bram_rdata_b_3 : {`CU_SIZE{scalar_inp}};
  assign b_data_4 = mode ? bram_rdata_b_4 : {`CU_SIZE{scalar_inp}};
  assign b_data_5 = mode ? bram_rdata_b_5 : {`CU_SIZE{scalar_inp}};

  /////////////////////////////////////////////////
  // Compute Unit 0
  /////////////////////////////////////////////////

eltwise_cu u_eltwise_cu_0(
  .clk(clk),
  .reset(reset),
  .pe_reset(pe_reset),
  .start_eltwise_op(start_eltwise_op),
  .done_eltwise_op(done_eltwise_op_0),
  .count(iterations),
  .op(op),
  .address_mat_a(address_mat_a),
  .address_mat_b(address_mat_b),
  .address_mat_c(address_mat_c),
  .a_data(bram_rdata_a_0),
  .b_data(b_data_0),
  .c_data_out(bram_wdata_c_0),
  .a_addr(bram_addr_a_0),
  .b_addr(bram_addr_b_0),
  .c_addr(bram_addr_c_0),
  .c_data_available(c_data_0_available),
  .validity_mask_a(4'b1111),
  .validity_mask_b(4'b1111)
);

  /////////////////////////////////////////////////
  // Compute Unit 1
  /////////////////////////////////////////////////

eltwise_cu u_eltwise_cu_1(
  .clk(clk),
  .reset(reset),
  .pe_reset(pe_reset),
  .start_eltwise_op(start_eltwise_op),
  .done_eltwise_op(done_eltwise_op_1),
  .count(iterations),
  .op(op),
  .address_mat_a(address_mat_a),
  .address_mat_b(address_mat_b),
  .address_mat_c(address_mat_c),
  .a_data(bram_rdata_a_1),
  .b_data(b_data_1),
  .c_data_out(bram_wdata_c_1),
  .a_addr(bram_addr_a_1),
  .b_addr(bram_addr_b_1),
  .c_addr(bram_addr_c_1),
  .c_data_available(c_data_1_available),
  .validity_mask_a(4'b1111),
  .validity_mask_b(4'b1111)
);

  /////////////////////////////////////////////////
  // Compute Unit 2
  /////////////////////////////////////////////////

eltwise_cu u_eltwise_cu_2(
  .clk(clk),
  .reset(reset),
  .pe_reset(pe_reset),
  .start_eltwise_op(start_eltwise_op),
  .done_eltwise_op(done_eltwise_op_2),
  .count(iterations),
  .op(op),
  .address_mat_a(address_mat_a),
  .address_mat_b(address_mat_b),
  .address_mat_c(address_mat_c),
  .a_data(bram_rdata_a_2),
  .b_data(b_data_2),
  .c_data_out(bram_wdata_c_2),
  .a_addr(bram_addr_a_2),
  .b_addr(bram_addr_b_2),
  .c_addr(bram_addr_c_2),
  .c_data_available(c_data_2_available),
  .validity_mask_a(4'b1111),
  .validity_mask_b(4'b1111)
);

  /////////////////////////////////////////////////
  // Compute Unit 3
  /////////////////////////////////////////////////

eltwise_cu u_eltwise_cu_3(
  .clk(clk),
  .reset(reset),
  .pe_reset(pe_reset),
  .start_eltwise_op(start_eltwise_op),
  .done_eltwise_op(done_eltwise_op_3),
  .count(iterations),
  .op(op),
  .address_mat_a(address_mat_a),
  .address_mat_b(address_mat_b),
  .address_mat_c(address_mat_c),
  .a_data(bram_rdata_a_3),
  .b_data(b_data_3),
  .c_data_out(bram_wdata_c_3),
  .a_addr(bram_addr_a_3),
  .b_addr(bram_addr_b_3),
  .c_addr(bram_addr_c_3),
  .c_data_available(c_data_3_available),
  .validity_mask_a(4'b1111),
  .validity_mask_b(4'b1111)
);

  /////////////////////////////////////////////////
  // Compute Unit 4
  /////////////////////////////////////////////////

eltwise_cu u_eltwise_cu_4(
  .clk(clk),
  .reset(reset),
  .pe_reset(pe_reset),
  .start_eltwise_op(start_eltwise_op),
  .done_eltwise_op(done_eltwise_op_4),
  .count(iterations),
  .op(op),
  .address_mat_a(address_mat_a),
  .address_mat_b(address_mat_b),
  .address_mat_c(address_mat_c),
  .a_data(bram_rdata_a_4),
  .b_data(b_data_4),
  .c_data_out(bram_wdata_c_4),
  .a_addr(bram_addr_a_4),
  .b_addr(bram_addr_b_4),
  .c_addr(bram_addr_c_4),
  .c_data_available(c_data_4_available),
  .validity_mask_a(4'b1111),
  .validity_mask_b(4'b1111)
);

  /////////////////////////////////////////////////
  // Compute Unit 5
  /////////////////////////////////////////////////

eltwise_cu u_eltwise_cu_5(
  .clk(clk),
  .reset(reset),
  .pe_reset(pe_reset),
  .start_eltwise_op(start_eltwise_op),
  .done_eltwise_op(done_eltwise_op_5),
  .count(iterations),
  .op(op),
  .address_mat_a(address_mat_a),
  .address_mat_b(address_mat_b),
  .address_mat_c(address_mat_c),
  .a_data(bram_rdata_a_5),
  .b_data(b_data_5),
  .c_data_out(bram_wdata_c_5),
  .a_addr(bram_addr_a_5),
  .b_addr(bram_addr_b_5),
  .c_addr(bram_addr_c_5),
  .c_data_available(c_data_5_available),
  .validity_mask_a(4'b0011),
  .validity_mask_b(4'b0011)
);

endmodule


//////////////////////////////////
//////////////////////////////////
//Dual port RAM
//////////////////////////////////
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
input [`CU_SIZE*`DWIDTH-1:0] d0;
input [`CU_SIZE*`DWIDTH-1:0] d1;
input [`CU_SIZE-1:0] we0;
input [`CU_SIZE-1:0] we1;
output [`CU_SIZE*`DWIDTH-1:0] q0;
output [`CU_SIZE*`DWIDTH-1:0] q1;
input clk;

`ifdef VCS
reg [`CU_SIZE*`DWIDTH-1:0] q0;
reg [`CU_SIZE*`DWIDTH-1:0] q1;
reg [`DWIDTH-1:0] ram[((1<<`AWIDTH)-1):0];
integer i;

always @(posedge clk)  
begin 
    for (i = 0; i < `CU_SIZE; i=i+1) begin
        if (we0[i]) ram[addr0+i] <= d0[i*`DWIDTH +: `DWIDTH]; 
    end    
    for (i = 0; i < `CU_SIZE; i=i+1) begin
        q0[i*`DWIDTH +: `DWIDTH] <= ram[addr0+i];
    end    
end

always @(posedge clk)  
begin 
    for (i = 0; i < `CU_SIZE; i=i+1) begin
        if (we1[i]) ram[addr0+i] <= d1[i*`DWIDTH +: `DWIDTH]; 
    end    
    for (i = 0; i < `CU_SIZE; i=i+1) begin
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

  
//////////////////////////////////
//////////////////////////////////
// Elementwise compute unit
//////////////////////////////////
//////////////////////////////////
module eltwise_cu(
 clk,
 reset,
 pe_reset,
 start_eltwise_op,
 done_eltwise_op,
 count,
 op,
 address_mat_a,
 address_mat_b,
 address_mat_c,
 a_data,
 b_data,
 c_data_out, 
 a_addr,
 b_addr,
 c_addr,
 c_data_available,
 validity_mask_a,
 validity_mask_b
);

 input clk;
 input reset;
 input pe_reset;
 input start_eltwise_op;
 output done_eltwise_op;
 input [`ITERATIONS_WIDTH-1:0] count;
 input [1:0] op;
 input [`AWIDTH-1:0] address_mat_a;
 input [`AWIDTH-1:0] address_mat_b;
 input [`AWIDTH-1:0] address_mat_c;
 input [`CU_SIZE*`DWIDTH-1:0] a_data;
 input [`CU_SIZE*`DWIDTH-1:0] b_data;
 output [`CU_SIZE*`DWIDTH-1:0] c_data_out;
 output [`AWIDTH-1:0] a_addr;
 output [`AWIDTH-1:0] b_addr;
 output [`AWIDTH-1:0] c_addr;
 output c_data_available;
 input [`MASK_WIDTH-1:0] validity_mask_a;
 input [`MASK_WIDTH-1:0] validity_mask_b;

wire [`DWIDTH-1:0] out0;
wire [`DWIDTH-1:0] out1;
wire [`DWIDTH-1:0] out2;
wire [`DWIDTH-1:0] out3;

wire [`DWIDTH-1:0] a0_data;
wire [`DWIDTH-1:0] a1_data;
wire [`DWIDTH-1:0] a2_data;
wire [`DWIDTH-1:0] a3_data;
wire [`DWIDTH-1:0] b0_data;
wire [`DWIDTH-1:0] b1_data;
wire [`DWIDTH-1:0] b2_data;
wire [`DWIDTH-1:0] b3_data;

//////////////////////////////////////////////////////////////////////////
// Logic for done
//////////////////////////////////////////////////////////////////////////
wire [7:0] clk_cnt_for_done;
reg [31:0] clk_cnt;
reg done_eltwise_op;

assign clk_cnt_for_done = 
                  `PE_PIPELINE_DEPTH + //This is dependent on the pipeline depth of the PEs
                  count //The number of iterations asked for this compute unit
                  ;
                          
always @(posedge clk) begin
  if (reset || ~start_eltwise_op) begin
    clk_cnt <= 0;
    done_eltwise_op <= 0;
  end
  else if (clk_cnt == clk_cnt_for_done) begin
    done_eltwise_op <= 1;
    clk_cnt <= clk_cnt + 1;
  end
  else if (done_eltwise_op == 0) begin
    clk_cnt <= clk_cnt + 1;
  end    
  else begin
    done_eltwise_op <= 0;
    clk_cnt <= clk_cnt + 1;
  end
end

//////////////////////////////////////////////////////////////////////////
// Instantiation of input logic
//////////////////////////////////////////////////////////////////////////
input_logic u_input_logic(
.clk(clk),
.reset(reset),
.start_eltwise_op(start_eltwise_op),
.count(count),
.a_addr(a_addr),
.b_addr(b_addr),
.address_mat_a(address_mat_a),
.address_mat_b(address_mat_b),
.a_data(a_data),
.b_data(b_data),
.a0_data(a0_data),
.a1_data(a1_data),
.a2_data(a2_data),
.a3_data(a3_data),
.b0_data(b0_data),
.b1_data(b1_data),
.b2_data(b2_data),
.b3_data(b3_data),
.validity_mask_a(validity_mask_a),
.validity_mask_b(validity_mask_b)
);

//////////////////////////////////////////////////////////////////////////
// Instantiation of the output logic
//////////////////////////////////////////////////////////////////////////
output_logic u_output_logic(
.clk(clk),
.reset(reset),
.start_eltwise_op(start_eltwise_op),
.done_eltwise_op(done_eltwise_op),
.address_mat_c(address_mat_c),
.c_data_out(c_data_out),
.c_addr(c_addr),
.c_data_available(c_data_available),
.out0(out0),
.out1(out1),
.out2(out2),
.out3(out3)
);

//////////////////////////////////////////////////////////////////////////
// Instantiations of the actual PEs
//////////////////////////////////////////////////////////////////////////
pe_array u_pe_array(
.reset(reset),
.clk(clk),
.pe_reset(pe_reset),
.op(op),
.a0(a0_data), 
.a1(a1_data), 
.a2(a2_data), 
.a3(a3_data),
.b0(b0_data), 
.b1(b1_data), 
.b2(b2_data), 
.b3(b3_data),
.out0(out0),
.out1(out1),
.out2(out2),
.out3(out3)
);

endmodule

//////////////////////////////////////////////////////////////////////////
// Output logic
//////////////////////////////////////////////////////////////////////////
module output_logic(
clk,
reset,
start_eltwise_op,
done_eltwise_op,
address_mat_c,
c_data_out, 
c_addr,
c_data_available,
out0,
out1,
out2,
out3
);

input clk;
input reset;
input start_eltwise_op;
input done_eltwise_op;
input [`AWIDTH-1:0] address_mat_c;
output [`CU_SIZE*`DWIDTH-1:0] c_data_out;
output [`AWIDTH-1:0] c_addr;
output c_data_available;
input [`DWIDTH-1:0] out0;
input [`DWIDTH-1:0] out1;
input [`DWIDTH-1:0] out2;
input [`DWIDTH-1:0] out3;

reg c_data_available;
reg [`CU_SIZE*`DWIDTH-1:0] c_data_out;

//////////////////////////////////////////////////////////////////////////
// Logic to capture matrix C data from the PEs and send to RAM
//////////////////////////////////////////////////////////////////////////

reg [`AWIDTH-1:0] c_addr;
reg [7:0] cnt;

always @(posedge clk) begin
  if (reset | ~start_eltwise_op) begin
    c_data_available <= 1'b0;
    c_addr <= address_mat_c;
    c_data_out <= 0;
    cnt <= 0;
  end
  else if (cnt>`PE_PIPELINE_DEPTH) begin
    c_data_available <= 1'b1;
    c_addr <= c_addr+1;
    c_data_out <= {out3, out2, out1, out0};
    cnt <= cnt + 1;
  end else begin
    cnt <= cnt + 1;
  end 
end

endmodule

//////////////////////////////////////////////////////////////////////////
// Data setup
//////////////////////////////////////////////////////////////////////////
module input_logic(
clk,
reset,
start_eltwise_op,
count,
a_addr,
b_addr,
address_mat_a,
address_mat_b,
a_data,
b_data,
a0_data,
a1_data,
a2_data,
a3_data,
b0_data,
b1_data,
b2_data,
b3_data,
validity_mask_a,
validity_mask_b
);

input clk;
input reset;
input start_eltwise_op;
input [`ITERATIONS_WIDTH-1:0] count;
output [`AWIDTH-1:0] a_addr;
output [`AWIDTH-1:0] b_addr;
input [`AWIDTH-1:0] address_mat_a;
input [`AWIDTH-1:0] address_mat_b;
input [`CU_SIZE*`DWIDTH-1:0] a_data;
input [`CU_SIZE*`DWIDTH-1:0] b_data;
output [`DWIDTH-1:0] a0_data;
output [`DWIDTH-1:0] a1_data;
output [`DWIDTH-1:0] a2_data;
output [`DWIDTH-1:0] a3_data;
output [`DWIDTH-1:0] b0_data;
output [`DWIDTH-1:0] b1_data;
output [`DWIDTH-1:0] b2_data;
output [`DWIDTH-1:0] b3_data;
input [`MASK_WIDTH-1:0] validity_mask_a;
input [`MASK_WIDTH-1:0] validity_mask_b;

reg [7:0] iterations;

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
reg a_mem_access; //flag that tells whether the compute unit is trying to access memory or not

always @(posedge clk) begin
  //else if (clk_cnt >= a_loc*`CU_SIZE+final_mat_mul_size) begin
  //Writing the line above to avoid multiplication:
  if (reset || ~start_eltwise_op) begin
    a_addr <= address_mat_a;
    a_mem_access <= 0;
    iterations <= 0;
  end

  //else if ((clk_cnt >= a_loc*`CU_SIZE) && (clk_cnt < a_loc*`CU_SIZE+final_mat_mul_size)) begin
  //Writing the line above to avoid multiplication:
  else if (iterations <= count) begin
    a_addr <= a_addr + 1;
    a_mem_access <= 1;
    iterations <= iterations + 1;
  end
end  

//////////////////////////////////////////////////////////////////////////
// Logic to generate valid signals for data coming from BRAM A
//////////////////////////////////////////////////////////////////////////
reg [7:0] a_mem_access_counter;
always @(posedge clk) begin
  if (reset || ~start_eltwise_op) begin
    a_mem_access_counter <= 0;
  end
  else if (a_mem_access == 1) begin
    a_mem_access_counter <= a_mem_access_counter + 1;  

  end
  else begin
    a_mem_access_counter <= 0;
  end
end

wire bram_rdata_a_valid; //flag that tells whether the data from memory is valid
assign bram_rdata_a_valid = 
       ((validity_mask_a[0]==1'b0 && a_mem_access_counter==1) ||
        (validity_mask_a[1]==1'b0 && a_mem_access_counter==2) ||
        (validity_mask_a[2]==1'b0 && a_mem_access_counter==3) ||
        (validity_mask_a[3]==1'b0 && a_mem_access_counter==4)) ?
        1'b0 : (a_mem_access_counter >= `MEM_ACCESS_LATENCY);

//////////////////////////////////////////////////////////////////////////
// Logic to delay certain parts of the data received from BRAM A (systolic data setup)
//////////////////////////////////////////////////////////////////////////
//Slice data into chunks and qualify it with whether it is valid or not
assign a0_data = a_data[1*`DWIDTH-1:0*`DWIDTH] & {`DWIDTH{bram_rdata_a_valid}} & {`DWIDTH{validity_mask_a[0]}};
assign a1_data = a_data[2*`DWIDTH-1:1*`DWIDTH] & {`DWIDTH{bram_rdata_a_valid}} & {`DWIDTH{validity_mask_a[1]}};
assign a2_data = a_data[3*`DWIDTH-1:2*`DWIDTH] & {`DWIDTH{bram_rdata_a_valid}} & {`DWIDTH{validity_mask_a[2]}};
assign a3_data = a_data[4*`DWIDTH-1:3*`DWIDTH] & {`DWIDTH{bram_rdata_a_valid}} & {`DWIDTH{validity_mask_a[3]}};


//////////////////////////////////////////////////////////////////////////
// Logic to generate addresses to BRAM B
//////////////////////////////////////////////////////////////////////////
reg [`AWIDTH-1:0] b_addr;
reg b_mem_access; //flag that tells whether the compute unit is trying to access memory or not

always @(posedge clk) begin
  //else if (clk_cnt >= b_loc*`CU_SIZE+final_mat_mul_size) begin
  //Writing the line above to avoid multiplication:
  if (reset || ~start_eltwise_op) begin
    b_addr <= address_mat_b ;
    b_mem_access <= 0;
  end
  //else if ((clk_cnt >= b_loc*`CU_SIZE) && (clk_cnt < b_loc*`CU_SIZE+final_mat_mul_size)) begin
  //Writing the line above to avoid multiplication:
  else if (iterations <= count) begin
    b_addr <= b_addr + 1;
    b_mem_access <= 1;
  end
end  

//////////////////////////////////////////////////////////////////////////
// Logic to generate valid signals for data coming from BRAM B
//////////////////////////////////////////////////////////////////////////
reg [7:0] b_mem_access_counter;
always @(posedge clk) begin
  if (reset || ~start_eltwise_op) begin
    b_mem_access_counter <= 0;
  end
  else if (b_mem_access == 1) begin
    b_mem_access_counter <= b_mem_access_counter + 1;  
  end
  else begin
    b_mem_access_counter <= 0;
  end
end

wire bram_rdata_b_valid; //flag that tells whether the data from memory is valid
assign bram_rdata_b_valid = 
       ((validity_mask_b[0]==1'b0 && b_mem_access_counter==1) ||
        (validity_mask_b[1]==1'b0 && b_mem_access_counter==2) ||
        (validity_mask_b[2]==1'b0 && b_mem_access_counter==3) ||
        (validity_mask_b[3]==1'b0 && b_mem_access_counter==4)) ?
        1'b0 : (b_mem_access_counter >= `MEM_ACCESS_LATENCY);

//Slice data into chunks and qualify it with whether it is valid or not
assign b0_data = b_data[1*`DWIDTH-1:0*`DWIDTH] & {`DWIDTH{bram_rdata_b_valid}} & {`DWIDTH{validity_mask_b[0]}};
assign b1_data = b_data[2*`DWIDTH-1:1*`DWIDTH] & {`DWIDTH{bram_rdata_b_valid}} & {`DWIDTH{validity_mask_b[1]}};
assign b2_data = b_data[3*`DWIDTH-1:2*`DWIDTH] & {`DWIDTH{bram_rdata_b_valid}} & {`DWIDTH{validity_mask_b[2]}};
assign b3_data = b_data[4*`DWIDTH-1:3*`DWIDTH] & {`DWIDTH{bram_rdata_b_valid}} & {`DWIDTH{validity_mask_b[3]}};


endmodule



//////////////////////////////////////////////////////////////////////////
// Array of processing elements
//////////////////////////////////////////////////////////////////////////
module pe_array(
reset,
clk,
pe_reset,
op,
a0, a1, a2, a3,
b0, b1, b2, b3,
out0, out1, out2, out3
);

input clk;
input reset;
input pe_reset;
input [1:0] op;
input [`DWIDTH-1:0] a0;
input [`DWIDTH-1:0] a1;
input [`DWIDTH-1:0] a2;
input [`DWIDTH-1:0] a3;
input [`DWIDTH-1:0] b0;
input [`DWIDTH-1:0] b1;
input [`DWIDTH-1:0] b2;
input [`DWIDTH-1:0] b3;
output [`DWIDTH-1:0] out0;
output [`DWIDTH-1:0] out1;
output [`DWIDTH-1:0] out2;
output [`DWIDTH-1:0] out3;

wire [`DWIDTH-1:0] out0, out1, out2, out3;

wire effective_rst;
assign effective_rst = reset | pe_reset;

processing_element pe0(.reset(effective_rst), .clk(clk), .in_a(a0), .in_b(b0), .op(op), .out(out0));
processing_element pe1(.reset(effective_rst), .clk(clk), .in_a(a1), .in_b(b1), .op(op), .out(out1));
processing_element pe2(.reset(effective_rst), .clk(clk), .in_a(a2), .in_b(b2), .op(op), .out(out2));
processing_element pe3(.reset(effective_rst), .clk(clk), .in_a(a3), .in_b(b3), .op(op), .out(out3));

endmodule


//////////////////////////////////////////////////////////////////////////
// Processing element (PE)
//////////////////////////////////////////////////////////////////////////
module processing_element(
 reset, 
 clk, 
 in_a,
 in_b, 
 op,
 out
 );

 input reset;
 input clk;
 input  [`DWIDTH-1:0] in_a;
 input  [`DWIDTH-1:0] in_b;
 input  [1:0] op;
 output [`DWIDTH-1:0] out;

 wire [`DWIDTH-1:0] out_mul;
 wire [`DWIDTH-1:0] out_sum;
 wire [`DWIDTH-1:0] out_sub;

 assign out = (op == 2'b00) ? out_sum : 
              (op == 2'b01) ? out_sub :
              out_mul;

 seq_mul u_mul(.a(in_a), .b(in_b), .out(out_mul), .reset(reset), .clk(clk));
 seq_add u_add(.a(in_a), .b(in_b), .out(out_sum), .reset(reset), .clk(clk));
 seq_sub u_sub(.a(in_a), .b(in_b), .out(out_sum), .reset(reset), .clk(clk));

endmodule

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Multiply block
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
module seq_mul(a, b, out, reset, clk);
input [`DWIDTH-1:0] a;
input [`DWIDTH-1:0] b;
input reset;
input clk;
output [`DWIDTH-1:0] out;

reg [`DWIDTH-1:0] a_flopped;
reg [`DWIDTH-1:0] b_flopped;

wire [`DWIDTH-1:0] mul_out_temp;
reg [`DWIDTH-1:0] mul_out_temp_reg;

always @(posedge clk) begin
  if (reset) begin
    a_flopped <= 0;
    b_flopped <= 0;
  end else begin
    a_flopped <= a;
    b_flopped <= b;
  end
end

//assign mul_out_temp = a * b;
`ifdef complex_dsp
multiply_fp_clk mul_u1(.clk(clk), .a(a_flopped), .b(b_flopped), .out(mul_out_temp));
`else
FPMult_16 u_FPMult (.clk(clk), .rst(1'b0), .a(a_flopped), .b(b_flopped), .result(mul_out_temp), .flags());
`endif

always @(posedge clk) begin
  if (reset) begin
    mul_out_temp_reg <= 0;
  end else begin
    mul_out_temp_reg <= mul_out_temp;
  end
end

assign out = mul_out_temp_reg;

endmodule

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Addition block
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
module seq_add(a, b, out, reset, clk);
input [`DWIDTH-1:0] a;
input [`DWIDTH-1:0] b;
input reset;
input clk;
output [`DWIDTH-1:0] out;

reg [`DWIDTH-1:0] a_flopped;
reg [`DWIDTH-1:0] b_flopped;

wire [`DWIDTH-1:0] sum_out_temp;
reg [`DWIDTH-1:0] sum_out_temp_reg;

always @(posedge clk) begin
  if (reset) begin
    a_flopped <= 0;
    b_flopped <= 0;
  end else begin
    a_flopped <= a;
    b_flopped <= b;
  end
end

//assign sum_out_temp = a + b;
`ifdef complex_dsp
adder_fp_clk add_u1(.clk(clk), .a(a_flopped), .b(b_flopped), .out(sum_out_temp));
`else
FPAddSub u_FPAddSub (.clk(clk), .rst(1'b0), .a(a_flopped), .b(b_flopped), .operation(1'b0), .result(sum_out_temp), .flags());
`endif

always @(posedge clk) begin
  if (reset) begin
    sum_out_temp_reg <= 0;
  end else begin
    sum_out_temp_reg <= sum_out_temp;
  end
end

assign out = sum_out_temp_reg;

endmodule


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Subtraction block
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
module seq_sub(a, b, out, reset, clk);
input [`DWIDTH-1:0] a;
input [`DWIDTH-1:0] b;
input reset;
input clk;
output [`DWIDTH-1:0] out;

reg [`DWIDTH-1:0] a_flopped;
reg [`DWIDTH-1:0] b_flopped;

wire [`DWIDTH-1:0] sub_out_temp;
reg [`DWIDTH-1:0] sub_out_temp_reg;

always @(posedge clk) begin
  if (reset) begin
    a_flopped <= 0;
    b_flopped <= 0;
  end else begin
    a_flopped <= a;
    b_flopped <= b;
  end
end

//assign sub_out_temp = a - b;
//Floating point adder has both modes - add and sub.
//We don't provide the name of the mode here though.

`ifdef complex_dsp
adder_fp_clk sub_u1(.clk(clk), .a(a_flopped), .b(b_flopped), .out(sub_out_temp));
`else
FPAddSub u_FPAddSub2(.clk(clk), .rst(1'b0), .a(a_flopped), .b(b_flopped), .operation(1'b0), .result(sub_out_temp), .flags());
`endif

always @(posedge clk) begin
  if (reset) begin
    sub_out_temp_reg <= 0;
  end else begin
    sub_out_temp_reg <= sub_out_temp;
  end
end

assign out = sub_out_temp_reg;

endmodule


`ifndef complex_dsp

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Floating point 16-bit multiplier
// This is a heavily modified version of:
// https://github.com/fbrosser/DSP48E1-FP/tree/master/src/FPMult
// Original author: Fredrik Brosser
// Abridged by: Samidh Mehta
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

module FPMult_16(
		clk,
		rst,
		a,
		b,
		result,
		flags
    );
	
	// Input Ports
	input clk ;							// Clock
	input rst ;							// Reset signal
	input [`DWIDTH-1:0] a;						// Input A, a 32-bit floating point number
	input [`DWIDTH-1:0] b;						// Input B, a 32-bit floating point number
	
	// Output ports
	output [`DWIDTH-1:0] result ;					// Product, result of the operation, 32-bit FP number
	output [4:0] flags ;						// Flags indicating exceptions according to IEEE754
	
	// Internal signals
	wire [`DWIDTH-1:0] Z_int ;					// Product, result of the operation, 32-bit FP number
	wire [4:0] Flags_int ;						// Flags indicating exceptions according to IEEE754
	
	wire Sa ;							// A's sign
	wire Sb ;							// B's sign
	wire Sp ;							// Product sign
	wire [`EXPONENT-1:0] Ea ;					// A's exponent
	wire [`EXPONENT-1:0] Eb ;					// B's exponent
	wire [2*`MANTISSA+1:0] Mp ;					// Product mantissa
	wire [4:0] InputExc ;						// Exceptions in inputs
	wire [`MANTISSA-1:0] NormM ;					// Normalized mantissa
	wire [`EXPONENT:0] NormE ;					// Normalized exponent
	wire [`MANTISSA:0] RoundM ;					// Normalized mantissa
	wire [`EXPONENT:0] RoundE ;					// Normalized exponent
	wire [`MANTISSA:0] RoundMP ;					// Normalized mantissa
	wire [`EXPONENT:0] RoundEP ;					// Normalized exponent
	wire GRS ;

	//reg [63:0] pipe_0;						// Pipeline register Input->Prep
	reg [2*`DWIDTH-1:0] pipe_0;					// Pipeline register Input->Prep

	//reg [92:0] pipe_1;						// Pipeline register Prep->Execute
	//reg [3*`MANTISSA+2*`EXPONENT+7:0] pipe_1;			// Pipeline register Prep->Execute
	reg [3*`MANTISSA+2*`EXPONENT+18:0] pipe_1;

	//reg [38:0] pipe_2;						// Pipeline register Execute->Normalize
	reg [`MANTISSA+`EXPONENT+7:0] pipe_2;				// Pipeline register Execute->Normalize
	
	//reg [72:0] pipe_3;						// Pipeline register Normalize->Round
	reg [2*`MANTISSA+2*`EXPONENT+10:0] pipe_3;			// Pipeline register Normalize->Round

	//reg [36:0] pipe_4;						// Pipeline register Round->Output
	reg [`DWIDTH+4:0] pipe_4;					// Pipeline register Round->Output
	
	assign result = pipe_4[`DWIDTH+4:5] ;
	assign flags = pipe_4[4:0] ;
	
	// Prepare the operands for alignment and check for exceptions
	FPMult_PrepModule PrepModule(clk, rst, pipe_0[2*`DWIDTH-1:`DWIDTH], pipe_0[`DWIDTH-1:0], Sa, Sb, Ea[`EXPONENT-1:0], Eb[`EXPONENT-1:0], Mp[2*`MANTISSA+1:0], InputExc[4:0]) ;

	// Perform (unsigned) mantissa multiplication
	FPMult_ExecuteModule ExecuteModule(pipe_1[3*`MANTISSA+`EXPONENT*2+7:2*`MANTISSA+2*`EXPONENT+8], pipe_1[2*`MANTISSA+2*`EXPONENT+7:2*`MANTISSA+7], pipe_1[2*`MANTISSA+6:5], pipe_1[2*`MANTISSA+2*`EXPONENT+6:2*`MANTISSA+`EXPONENT+7], pipe_1[2*`MANTISSA+`EXPONENT+6:2*`MANTISSA+7], pipe_1[2*`MANTISSA+2*`EXPONENT+8], pipe_1[2*`MANTISSA+2*`EXPONENT+7], Sp, NormE[`EXPONENT:0], NormM[`MANTISSA-1:0], GRS) ;

	// Round result and if necessary, perform a second (post-rounding) normalization step
	FPMult_NormalizeModule NormalizeModule(pipe_2[`MANTISSA-1:0], pipe_2[`MANTISSA+`EXPONENT:`MANTISSA], RoundE[`EXPONENT:0], RoundEP[`EXPONENT:0], RoundM[`MANTISSA:0], RoundMP[`MANTISSA:0]) ;		

	// Round result and if necessary, perform a second (post-rounding) normalization step
	//FPMult_RoundModule RoundModule(pipe_3[47:24], pipe_3[23:0], pipe_3[65:57], pipe_3[56:48], pipe_3[66], pipe_3[67], pipe_3[72:68], Z_int[31:0], Flags_int[4:0]) ;		
	FPMult_RoundModule RoundModule(pipe_3[2*`MANTISSA+1:`MANTISSA+1], pipe_3[`MANTISSA:0], pipe_3[2*`MANTISSA+2*`EXPONENT+3:2*`MANTISSA+`EXPONENT+3], pipe_3[2*`MANTISSA+`EXPONENT+2:2*`MANTISSA+2], pipe_3[2*`MANTISSA+2*`EXPONENT+4], pipe_3[2*`MANTISSA+2*`EXPONENT+5], pipe_3[2*`MANTISSA+2*`EXPONENT+10:2*`MANTISSA+2*`EXPONENT+6], Z_int[`DWIDTH-1:0], Flags_int[4:0]) ;		

//adding always@ (*) instead of posedge clock to make design combinational
	always @ (posedge clk) begin	
		if(rst) begin
			pipe_0 <= 0;
			pipe_1 <= 0;
			pipe_2 <= 0; 
			pipe_3 <= 0;
			pipe_4 <= 0;
		end 
		else begin		
			/* PIPE 0
				[2*`DWIDTH-1:`DWIDTH] A
				[`DWIDTH-1:0] B
			*/
                       pipe_0 <= {a, b} ;


			/* PIPE 1
				[2*`EXPONENT+3*`MANTISSA + 18: 2*`EXPONENT+2*`MANTISSA + 18] //pipe_0[`DWIDTH+`MANTISSA-1:`DWIDTH] , mantissa of A
				[2*`EXPONENT+2*`MANTISSA + 17 :2*`EXPONENT+2*`MANTISSA + 9] // pipe_0[8:0]
				[2*`EXPONENT+2*`MANTISSA + 8] Sa
				[2*`EXPONENT+2*`MANTISSA + 7] Sb
				[2*`EXPONENT+2*`MANTISSA + 6:`EXPONENT+2*`MANTISSA+7] Ea
				[`EXPONENT +2*`MANTISSA+6:2*`MANTISSA+7] Eb
				[2*`MANTISSA+1+5:5] Mp
				[4:0] InputExc
			*/
			//pipe_1 <= {pipe_0[`DWIDTH+`MANTISSA-1:`DWIDTH], pipe_0[`MANTISSA_MUL_SPLIT_LSB-1:0], Sa, Sb, Ea[`EXPONENT-1:0], Eb[`EXPONENT-1:0], Mp[2*`MANTISSA-1:0], InputExc[4:0]} ;
			pipe_1 <= {pipe_0[`DWIDTH+`MANTISSA-1:`DWIDTH], pipe_0[8:0], Sa, Sb, Ea[`EXPONENT-1:0], Eb[`EXPONENT-1:0], Mp[2*`MANTISSA+1:0], InputExc[4:0]} ;
			
			/* PIPE 2
				[`EXPONENT + `MANTISSA + 7:`EXPONENT + `MANTISSA + 3] InputExc
				[`EXPONENT + `MANTISSA + 2] GRS
				[`EXPONENT + `MANTISSA + 1] Sp
				[`EXPONENT + `MANTISSA:`MANTISSA] NormE
				[`MANTISSA-1:0] NormM
			*/
			pipe_2 <= {pipe_1[4:0], GRS, Sp, NormE[`EXPONENT:0], NormM[`MANTISSA-1:0]} ;
			/* PIPE 3
				[2*`EXPONENT+2*`MANTISSA+10:2*`EXPONENT+2*`MANTISSA+6] InputExc
				[2*`EXPONENT+2*`MANTISSA+5] GRS
				[2*`EXPONENT+2*`MANTISSA+4] Sp	
				[2*`EXPONENT+2*`MANTISSA+3:`EXPONENT+2*`MANTISSA+3] RoundE
				[`EXPONENT+2*`MANTISSA+2:2*`MANTISSA+2] RoundEP
				[2*`MANTISSA+1:`MANTISSA+1] RoundM
				[`MANTISSA:0] RoundMP
			*/
			pipe_3 <= {pipe_2[`EXPONENT+`MANTISSA+7:`EXPONENT+`MANTISSA+1], RoundE[`EXPONENT:0], RoundEP[`EXPONENT:0], RoundM[`MANTISSA:0], RoundMP[`MANTISSA:0]} ;
			/* PIPE 4
				[`DWIDTH+4:5] Z
				[4:0] Flags
			*/				
			pipe_4 <= {Z_int[`DWIDTH-1:0], Flags_int[4:0]} ;
		end
	end
		
endmodule



module FPMult_PrepModule (
		clk,
		rst,
		a,
		b,
		Sa,
		Sb,
		Ea,
		Eb,
		Mp,
		InputExc
	);
	
	// Input ports
	input clk ;
	input rst ;
	input [`DWIDTH-1:0] a ;								// Input A, a 32-bit floating point number
	input [`DWIDTH-1:0] b ;								// Input B, a 32-bit floating point number
	
	// Output ports
	output Sa ;										// A's sign
	output Sb ;										// B's sign
	output [`EXPONENT-1:0] Ea ;								// A's exponent
	output [`EXPONENT-1:0] Eb ;								// B's exponent
	output [2*`MANTISSA+1:0] Mp ;							// Mantissa product
	output [4:0] InputExc ;						// Input numbers are exceptions
	
	// Internal signals							// If signal is high...
	wire ANaN ;										// A is a signalling NaN
	wire BNaN ;										// B is a signalling NaN
	wire AInf ;										// A is infinity
	wire BInf ;										// B is infinity
    wire [`MANTISSA-1:0] Ma;
    wire [`MANTISSA-1:0] Mb;
	
	assign ANaN = &(a[`DWIDTH-2:`MANTISSA]) &  |(a[`DWIDTH-2:`MANTISSA]) ;			// All one exponent and not all zero mantissa - NaN
	assign BNaN = &(b[`DWIDTH-2:`MANTISSA]) &  |(b[`MANTISSA-1:0]);			// All one exponent and not all zero mantissa - NaN
	assign AInf = &(a[`DWIDTH-2:`MANTISSA]) & ~|(a[`DWIDTH-2:`MANTISSA]) ;		// All one exponent and all zero mantissa - Infinity
	assign BInf = &(b[`DWIDTH-2:`MANTISSA]) & ~|(b[`DWIDTH-2:`MANTISSA]) ;		// All one exponent and all zero mantissa - Infinity
	
	// Check for any exceptions and put all flags into exception vector
	assign InputExc = {(ANaN | BNaN | AInf | BInf), ANaN, BNaN, AInf, BInf} ;
	//assign InputExc = {(ANaN | ANaN | BNaN |BNaN), ANaN, ANaN, BNaN,BNaN} ;
	
	// Take input numbers apart
	assign Sa = a[`DWIDTH-1] ;							// A's sign
	assign Sb = b[`DWIDTH-1] ;							// B's sign
	assign Ea = a[`DWIDTH-2:`MANTISSA];						// Store A's exponent in Ea, unless A is an exception
	assign Eb = b[`DWIDTH-2:`MANTISSA];						// Store B's exponent in Eb, unless B is an exception	
//    assign Ma = a[`MANTISSA_MSB:`MANTISSA_LSB];
  //  assign Mb = b[`MANTISSA_MSB:`MANTISSA_LSB];
	


	//assign Mp = ({4'b0001, a[`MANTISSA-1:0]}*{4'b0001, b[`MANTISSA-1:9]}) ;
	assign Mp = ({1'b1,a[`MANTISSA-1:0]}*{1'b1, b[`MANTISSA-1:0]}) ;

	
    //We multiply part of the mantissa here
    //Full mantissa of A
    //Bits MANTISSA_MUL_SPLIT_MSB:MANTISSA_MUL_SPLIT_LSB of B
   // wire [`ACTUAL_MANTISSA-1:0] inp_A;
   // wire [`ACTUAL_MANTISSA-1:0] inp_B;
   // assign inp_A = {1'b1, Ma};
   // assign inp_B = {{(`MANTISSA-(`MANTISSA_MUL_SPLIT_MSB-`MANTISSA_MUL_SPLIT_LSB+1)){1'b0}}, 1'b1, Mb[`MANTISSA_MUL_SPLIT_MSB:`MANTISSA_MUL_SPLIT_LSB]};
   // DW02_mult #(`ACTUAL_MANTISSA,`ACTUAL_MANTISSA) u_mult(.A(inp_A), .B(inp_B), .TC(1'b0), .PRODUCT(Mp));
endmodule


module FPMult_ExecuteModule(
		a,
		b,
		MpC,
		Ea,
		Eb,
		Sa,
		Sb,
		Sp,
		NormE,
		NormM,
		GRS
    );

	// Input ports
	input [`MANTISSA-1:0] a ;
	input [2*`EXPONENT:0] b ;
	input [2*`MANTISSA+1:0] MpC ;
	input [`EXPONENT-1:0] Ea ;						// A's exponent
	input [`EXPONENT-1:0] Eb ;						// B's exponent
	input Sa ;								// A's sign
	input Sb ;								// B's sign
	
	// Output ports
	output Sp ;								// Product sign
	output [`EXPONENT:0] NormE ;													// Normalized exponent
	output [`MANTISSA-1:0] NormM ;												// Normalized mantissa
	output GRS ;
	
	wire [2*`MANTISSA+1:0] Mp ;
	
	assign Sp = (Sa ^ Sb) ;												// Equal signs give a positive product
	
   // wire [`ACTUAL_MANTISSA-1:0] inp_a;
   // wire [`ACTUAL_MANTISSA-1:0] inp_b;
   // assign inp_a = {1'b1, a};
   // assign inp_b = {{(`MANTISSA-`MANTISSA_MUL_SPLIT_LSB){1'b0}}, 1'b0, b};
   // DW02_mult #(`ACTUAL_MANTISSA,`ACTUAL_MANTISSA) u_mult(.A(inp_a), .B(inp_b), .TC(1'b0), .PRODUCT(Mp_temp));
   // DW01_add #(2*`ACTUAL_MANTISSA) u_add(.A(Mp_temp), .B(MpC<<`MANTISSA_MUL_SPLIT_LSB), .CI(1'b0), .SUM(Mp), .CO());

	//assign Mp = (MpC<<(2*`EXPONENT+1)) + ({4'b0001, a[`MANTISSA-1:0]}*{1'b0, b[2*`EXPONENT:0]}) ;
	assign Mp = MpC;


	assign NormM = (Mp[2*`MANTISSA+1] ? Mp[2*`MANTISSA:`MANTISSA+1] : Mp[2*`MANTISSA-1:`MANTISSA]); 	// Check for overflow
	assign NormE = (Ea + Eb + Mp[2*`MANTISSA+1]);								// If so, increment exponent
	
	assign GRS = ((Mp[`MANTISSA]&(Mp[`MANTISSA+1]))|(|Mp[`MANTISSA-1:0])) ;
	
endmodule

module FPMult_NormalizeModule(
		NormM,
		NormE,
		RoundE,
		RoundEP,
		RoundM,
		RoundMP
    );

	// Input Ports
	input [`MANTISSA-1:0] NormM ;									// Normalized mantissa
	input [`EXPONENT:0] NormE ;									// Normalized exponent

	// Output Ports
	output [`EXPONENT:0] RoundE ;
	output [`EXPONENT:0] RoundEP ;
	output [`MANTISSA:0] RoundM ;
	output [`MANTISSA:0] RoundMP ; 
	
// EXPONENT = 5 
// EXPONENT -1 = 4
// NEED to subtract 2^4 -1 = 15

wire [`EXPONENT-1 : 0] bias;

assign bias =  ((1<< (`EXPONENT -1)) -1);

	assign RoundE = NormE - bias ;
	assign RoundEP = NormE - bias -1 ;
	assign RoundM = NormM ;
	assign RoundMP = NormM ;

endmodule

module FPMult_RoundModule(
		RoundM,
		RoundMP,
		RoundE,
		RoundEP,
		Sp,
		GRS,
		InputExc,
		Z,
		Flags
    );

	// Input Ports
	input [`MANTISSA:0] RoundM ;									// Normalized mantissa
	input [`MANTISSA:0] RoundMP ;									// Normalized exponent
	input [`EXPONENT:0] RoundE ;									// Normalized mantissa + 1
	input [`EXPONENT:0] RoundEP ;									// Normalized exponent + 1
	input Sp ;												// Product sign
	input GRS ;
	input [4:0] InputExc ;
	
	// Output Ports
	output [`DWIDTH-1:0] Z ;										// Final product
	output [4:0] Flags ;
	
	// Internal Signals
	wire [`EXPONENT:0] FinalE ;									// Rounded exponent
	wire [`MANTISSA:0] FinalM;
	wire [`MANTISSA:0] PreShiftM;
	
	assign PreShiftM = GRS ? RoundMP : RoundM ;	// Round up if R and (G or S)
	
	// Post rounding normalization (potential one bit shift> use shifted mantissa if there is overflow)
	assign FinalM = (PreShiftM[`MANTISSA] ? {1'b0, PreShiftM[`MANTISSA:1]} : PreShiftM[`MANTISSA:0]) ;
	
	assign FinalE = (PreShiftM[`MANTISSA] ? RoundEP : RoundE) ; // Increment exponent if a shift was done
	
	assign Z = {Sp, FinalE[`EXPONENT-1:0], FinalM[`MANTISSA-1:0]} ;   // Putting the pieces together
	assign Flags = InputExc[4:0];

endmodule
`endif

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
// Floating point 16-bit adder
// This is a heavily modified version of:
// https://github.com/fbrosser/DSP48E1-FP/tree/master/src/FP_AddSub
// Original author: Fredrik Brosser
// Abridged by: Samidh Mehta
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
`ifndef complex_dsp

module FPAddSub(
		//bf16,
		clk,
		rst,
		a,
		b,
		operation,			// 0 add, 1 sub
		result,
		flags
	);
	//input bf16; //1 for Bfloat16, 0 for IEEE half precision

	// Clock and reset
	input clk ;										// Clock signal
	input rst ;										// Reset (active high, resets pipeline registers)
	
	// Input ports
	input [`DWIDTH-1:0] a ;								// Input A, a 32-bit floating point number
	input [`DWIDTH-1:0] b ;								// Input B, a 32-bit floating point number
	input operation ;								// Operation select signal
	
	// Output ports
	output [`DWIDTH-1:0] result ;						// Result of the operation
	output [4:0] flags ;							// Flags indicating exceptions according to IEEE754
	
	// Pipeline Registers
	//reg [79:0] pipe_1;							// Pipeline register PreAlign->Align1
	reg [2*`EXPONENT + 2*`DWIDTH + 5:0] pipe_1;							// Pipeline register PreAlign->Align1

	//reg [67:0] pipe_2;							// Pipeline register Align1->Align3
	//reg [2*`EXPONENT+ 2*`MANTISSA + 8:0] pipe_2;							// Pipeline register Align1->Align3
	wire [2*`EXPONENT+ 2*`MANTISSA + 8:0] pipe_2;

	//reg [76:0] pipe_3;	68						// Pipeline register Align1->Align3
	reg [2*`EXPONENT+ 2*`MANTISSA + 9:0] pipe_3;							// Pipeline register Align1->Align3

	//reg [69:0] pipe_4;							// Pipeline register Align3->Execute
	//reg [2*`EXPONENT+ 2*`MANTISSA + 9:0] pipe_4;							// Pipeline register Align3->Execute
	wire [2*`EXPONENT+ 2*`MANTISSA + 9:0] pipe_4;
	
	//reg [51:0] pipe_5;							// Pipeline register Execute->Normalize
	reg [`DWIDTH+`EXPONENT+11:0] pipe_5;							// Pipeline register Execute->Normalize

	//reg [56:0] pipe_6;							// Pipeline register Nomalize->NormalizeShift1
	//reg [`DWIDTH+`EXPONENT+16:0] pipe_6;							// Pipeline register Nomalize->NormalizeShift1
	wire [`DWIDTH+`EXPONENT+16:0] pipe_6;

	//reg [56:0] pipe_7;							// Pipeline register NormalizeShift2->NormalizeShift3
	//reg [`DWIDTH+`EXPONENT+16:0] pipe_7;							// Pipeline register NormalizeShift2->NormalizeShift3
	wire [`DWIDTH+`EXPONENT+16:0] pipe_7;
	//reg [54:0] pipe_8;							// Pipeline register NormalizeShift3->Round
	reg [`EXPONENT*2+`MANTISSA+15:0] pipe_8;							// Pipeline register NormalizeShift3->Round

	//reg [40:0] pipe_9;							// Pipeline register NormalizeShift3->Round
	//reg [`DWIDTH+8:0] pipe_9;							// Pipeline register NormalizeShift3->Round
	wire [`DWIDTH+8:0] pipe_9;

	// Internal wires between modules
	wire [`DWIDTH-2:0] Aout_0 ;							// A - sign
	wire [`DWIDTH-2:0] Bout_0 ;							// B - sign
	wire Opout_0 ;									// A's sign
	wire Sa_0 ;										// A's sign
	wire Sb_0 ;										// B's sign
	wire MaxAB_1 ;									// Indicates the larger of A and B(0/A, 1/B)
	wire [`EXPONENT-1:0] CExp_1 ;							// Common Exponent
	wire [`EXPONENT-1:0] Shift_1 ;							// Number of steps to smaller mantissa shift right (align)
	wire [`MANTISSA-1:0] Mmax_1 ;							// Larger mantissa
	wire [4:0] InputExc_0 ;						// Input numbers are exceptions
	wire [2*`EXPONENT-1:0] ShiftDet_0 ;
	wire [`MANTISSA-1:0] MminS_1 ;						// Smaller mantissa after 0/16 shift
	wire [`MANTISSA:0] MminS_2 ;						// Smaller mantissa after 0/4/8/12 shift
	wire [`MANTISSA:0] Mmin_3 ;							// Smaller mantissa after 0/1/2/3 shift
	wire [`DWIDTH:0] Sum_4 ;
	wire PSgn_4 ;
	wire Opr_4 ;
	wire [`EXPONENT-1:0] Shift_5 ;							// Number of steps to shift sum left (normalize)
	wire [`DWIDTH:0] SumS_5 ;							// Sum after 0/16 shift
	wire [`DWIDTH:0] SumS_6 ;							// Sum after 0/16 shift
	wire [`DWIDTH:0] SumS_7 ;							// Sum after 0/16 shift
	wire [`MANTISSA-1:0] NormM_8 ;						// Normalized mantissa
	wire [`EXPONENT:0] NormE_8;							// Adjusted exponent
	wire ZeroSum_8 ;								// Zero flag
	wire NegE_8 ;									// Flag indicating negative exponent
	wire R_8 ;										// Round bit
	wire S_8 ;										// Final sticky bit
	wire FG_8 ;										// Final sticky bit
	wire [`DWIDTH-1:0] P_int ;
	wire EOF ;
	
	// Prepare the operands for alignment and check for exceptions
	FPAddSub_PrealignModule PrealignModule
	(	// Inputs
		a, b, operation,
		// Outputs
		Sa_0, Sb_0, ShiftDet_0[2*`EXPONENT-1:0], InputExc_0[4:0], Aout_0[`DWIDTH-2:0], Bout_0[`DWIDTH-2:0], Opout_0) ;
		
	// Prepare the operands for alignment and check for exceptions
	FPAddSub_AlignModule AlignModule
	(	// Inputs
		pipe_1[2*`EXPONENT + 2*`DWIDTH + 4: 2*`EXPONENT +`DWIDTH + 6], pipe_1[2*`EXPONENT +`DWIDTH + 5 :  2*`EXPONENT +7], pipe_1[2*`EXPONENT+4:5],
		// Outputs
		CExp_1[`EXPONENT-1:0], MaxAB_1, Shift_1[`EXPONENT-1:0], MminS_1[`MANTISSA-1:0], Mmax_1[`MANTISSA-1:0]) ;	

	// Alignment Shift Stage 1
	FPAddSub_AlignShift1 AlignShift1
	(  // Inputs
		//bf16, 
		pipe_2[`MANTISSA-1:0], pipe_2[`EXPONENT+ 2*`MANTISSA + 4 : 2*`MANTISSA + 7],
		// Outputs
		MminS_2[`MANTISSA:0]) ;

	// Alignment Shift Stage 3 and compution of guard and sticky bits
	FPAddSub_AlignShift2 AlignShift2  
	(  // Inputs
		pipe_3[`MANTISSA:0], pipe_3[2*`MANTISSA+7:2*`MANTISSA+6],
		// Outputs
		Mmin_3[`MANTISSA:0]) ;
						
	// Perform mantissa addition
	FPAddSub_ExecutionModule ExecutionModule
	(  // Inputs
		pipe_4[`MANTISSA*2+5:`MANTISSA+6], pipe_4[`MANTISSA:0], pipe_4[2*`EXPONENT+ 2*`MANTISSA + 8], pipe_4[2*`EXPONENT+ 2*`MANTISSA + 7], pipe_4[2*`EXPONENT+ 2*`MANTISSA + 6], pipe_4[2*`EXPONENT+ 2*`MANTISSA + 9],
		// Outputs
		Sum_4[`DWIDTH:0], PSgn_4, Opr_4) ;
	
	// Prepare normalization of result
	FPAddSub_NormalizeModule NormalizeModule
	(  // Inputs
		pipe_5[`DWIDTH:0], 
		// Outputs
		SumS_5[`DWIDTH:0], Shift_5[4:0]) ;
					
	// Normalization Shift Stage 1
	FPAddSub_NormalizeShift1 NormalizeShift1
	(  // Inputs
		pipe_6[`DWIDTH:0], pipe_6[`DWIDTH+`EXPONENT+14:`DWIDTH+`EXPONENT+11],
		// Outputs
		SumS_7[`DWIDTH:0]) ;
		
	// Normalization Shift Stage 3 and final guard, sticky and round bits
	FPAddSub_NormalizeShift2 NormalizeShift2
	(  // Inputs
		pipe_7[`DWIDTH:0], pipe_7[`DWIDTH+`EXPONENT+5:`DWIDTH+6], pipe_7[`DWIDTH+`EXPONENT+15:`DWIDTH+`EXPONENT+11],
		// Outputs
		NormM_8[`MANTISSA-1:0], NormE_8[`EXPONENT:0], ZeroSum_8, NegE_8, R_8, S_8, FG_8) ;

	// Round and put result together
	FPAddSub_RoundModule RoundModule
	(  // Inputs
		 pipe_8[3], pipe_8[4+`EXPONENT:4], pipe_8[`EXPONENT+`MANTISSA+4:5+`EXPONENT], pipe_8[1], pipe_8[0], pipe_8[`EXPONENT*2+`MANTISSA+15], pipe_8[`EXPONENT*2+`MANTISSA+12], pipe_8[`EXPONENT*2+`MANTISSA+11], pipe_8[`EXPONENT*2+`MANTISSA+14], pipe_8[`EXPONENT*2+`MANTISSA+10], 
		// Outputs
		P_int[`DWIDTH-1:0], EOF) ;
	
	// Check for exceptions
	FPAddSub_ExceptionModule Exceptionmodule
	(  // Inputs
		pipe_9[8+`DWIDTH:9], pipe_9[8], pipe_9[7], pipe_9[6], pipe_9[5:1], pipe_9[0], 
		// Outputs
		result[`DWIDTH-1:0], flags[4:0]) ;			
	

assign pipe_2 = {pipe_1[2*`EXPONENT + 2*`DWIDTH + 5], pipe_1[2*`EXPONENT +6:2*`EXPONENT +5], MaxAB_1, CExp_1[`EXPONENT-1:0], Shift_1[`EXPONENT-1:0], Mmax_1[`MANTISSA-1:0], pipe_1[4:0], MminS_1[`MANTISSA-1:0]} ;
assign pipe_4 = {pipe_3[2*`EXPONENT+ 2*`MANTISSA + 9:`MANTISSA+1], Mmin_3[`MANTISSA:0]} ;
assign pipe_6 = {pipe_5[`DWIDTH+`EXPONENT+11], Shift_5[4:0], pipe_5[`DWIDTH+`EXPONENT+10:`DWIDTH+1], SumS_5[`DWIDTH:0]} ;
assign pipe_7 = {pipe_6[`DWIDTH+`EXPONENT+16:`DWIDTH+1], SumS_7[`DWIDTH:0]} ;
assign pipe_9 = {P_int[`DWIDTH-1:0], pipe_8[2], pipe_8[1], pipe_8[0], pipe_8[`EXPONENT+`MANTISSA+9:`EXPONENT+`MANTISSA+5], EOF} ;

	always @ (posedge clk) begin	
		if(rst) begin
			pipe_1 <= 0;
			//pipe_2 <= 0;
			pipe_3 <= 0;
			//pipe_4 <= 0;
			pipe_5 <= 0;
			//pipe_6 <= 0;
			//pipe_7 <= 0;
			pipe_8 <= 0;
			//pipe_9 <= 0;
		end 
		else begin
/* PIPE_1:
	[2*`EXPONENT + 2*`DWIDTH + 5]  Opout_0
	[2*`EXPONENT + 2*`DWIDTH + 4: 2*`EXPONENT +`DWIDTH + 6] A_out0
	[2*`EXPONENT +`DWIDTH + 5 :  2*`EXPONENT +7] Bout_0
	[2*`EXPONENT +6] Sa_0
	[2*`EXPONENT +5] Sb_0
	[2*`EXPONENT +4 : 5] ShiftDet_0
	[4:0] Input Exc
*/
			pipe_1 <= {Opout_0, Aout_0[`DWIDTH-2:0], Bout_0[`DWIDTH-2:0], Sa_0, Sb_0, ShiftDet_0[2*`EXPONENT -1:0], InputExc_0[4:0]} ;	
/* PIPE_2
[2*`EXPONENT+ 2*`MANTISSA + 8] operation
[2*`EXPONENT+ 2*`MANTISSA + 7] Sa_0
[2*`EXPONENT+ 2*`MANTISSA + 6] Sb_0
[2*`EXPONENT+ 2*`MANTISSA + 5] MaxAB_0
[2*`EXPONENT+ 2*`MANTISSA + 4:`EXPONENT+ 2*`MANTISSA + 5] CExp_0
[`EXPONENT+ 2*`MANTISSA + 4 : 2*`MANTISSA + 5] Shift_0
[2*`MANTISSA + 4:`MANTISSA + 5] Mmax_0
[`MANTISSA + 4 : `MANTISSA] InputExc_0
[`MANTISSA-1:0] MminS_1
*/
			//pipe_2 <= {pipe_1[2*`EXPONENT + 2*`DWIDTH + 5], pipe_1[2*`EXPONENT +6:2*`EXPONENT +5], MaxAB_1, CExp_1[`EXPONENT-1:0], Shift_1[`EXPONENT-1:0], Mmax_1[`MANTISSA-1:0], pipe_1[4:0], MminS_1[`MANTISSA-1:0]} ;	
/* PIPE_3
[2*`EXPONENT+ 2*`MANTISSA + 9] operation
[2*`EXPONENT+ 2*`MANTISSA + 8] Sa_0
[2*`EXPONENT+ 2*`MANTISSA + 7] Sb_0
[2*`EXPONENT+ 2*`MANTISSA + 6] MaxAB_0
[2*`EXPONENT+ 2*`MANTISSA + 5:`EXPONENT+ 2*`MANTISSA + 6] CExp_0
[`EXPONENT+ 2*`MANTISSA + 5 : 2*`MANTISSA + 6] Shift_0
[2*`MANTISSA + 5:`MANTISSA + 6] Mmax_0
[`MANTISSA + 5 : `MANTISSA + 1] InputExc_0
[`MANTISSA:0] MminS_2
*/
			pipe_3 <= {pipe_2[2*`EXPONENT+ 2*`MANTISSA + 8:`MANTISSA], MminS_2[`MANTISSA:0]} ;	
/* PIPE_4
[2*`EXPONENT+ 2*`MANTISSA + 9] operation
[2*`EXPONENT+ 2*`MANTISSA + 8] Sa_0
[2*`EXPONENT+ 2*`MANTISSA + 7] Sb_0
[2*`EXPONENT+ 2*`MANTISSA + 6] MaxAB_0
[2*`EXPONENT+ 2*`MANTISSA + 5:`EXPONENT+ 2*`MANTISSA + 6] CExp_0
[`EXPONENT+ 2*`MANTISSA + 5 : 2*`MANTISSA + 6] Shift_0
[2*`MANTISSA + 5:`MANTISSA + 6] Mmax_0
[`MANTISSA + 5 : `MANTISSA + 1] InputExc_0
[`MANTISSA:0] MminS_3
*/				
			//pipe_4 <= {pipe_3[2*`EXPONENT+ 2*`MANTISSA + 9:`MANTISSA+1], Mmin_3[`MANTISSA:0]} ;	
/* PIPE_5 :
[`DWIDTH+ `EXPONENT + 11] operation
[`DWIDTH+ `EXPONENT + 10] PSgn_4
[`DWIDTH+ `EXPONENT + 9] Opr_4
[`DWIDTH+ `EXPONENT + 8] Sa_0
[`DWIDTH+ `EXPONENT + 7] Sb_0
[`DWIDTH+ `EXPONENT + 6] MaxAB_0
[`DWIDTH+ `EXPONENT + 5 :`DWIDTH+6] CExp_0
[`DWIDTH+5:`DWIDTH+1] InputExc_0
[`DWIDTH:0] Sum_4
*/					
			pipe_5 <= {pipe_4[2*`EXPONENT+ 2*`MANTISSA + 9], PSgn_4, Opr_4, pipe_4[2*`EXPONENT+ 2*`MANTISSA + 8:`EXPONENT+ 2*`MANTISSA + 6], pipe_4[`MANTISSA+5:`MANTISSA+1], Sum_4[`DWIDTH:0]} ;
/* PIPE_6 :
[`DWIDTH+ `EXPONENT + 16] operation
[`DWIDTH+ `EXPONENT + 15:`DWIDTH+ `EXPONENT + 11] Shift_5
[`DWIDTH+ `EXPONENT + 10] PSgn_4
[`DWIDTH+ `EXPONENT + 9] Opr_4
[`DWIDTH+ `EXPONENT + 8] Sa_0
[`DWIDTH+ `EXPONENT + 7] Sb_0
[`DWIDTH+ `EXPONENT + 6] MaxAB_0
[`DWIDTH+ `EXPONENT + 5 :`DWIDTH+6] CExp_0
[`DWIDTH+5:`DWIDTH+1] InputExc_0
[`DWIDTH:0] Sum_4
*/				
			//pipe_6 <= {pipe_5[`DWIDTH+`EXPONENT+11], Shift_5[4:0], pipe_5[`DWIDTH+`EXPONENT+10:`DWIDTH+1], SumS_5[`DWIDTH:0]} ;	
/* PIPE_7 :
[`DWIDTH+ `EXPONENT + 16] operation
[`DWIDTH+ `EXPONENT + 15:`DWIDTH+ `EXPONENT + 11] Shift_5
[`DWIDTH+ `EXPONENT + 10] PSgn_4
[`DWIDTH+ `EXPONENT + 9] Opr_4
[`DWIDTH+ `EXPONENT + 8] Sa_0
[`DWIDTH+ `EXPONENT + 7] Sb_0
[`DWIDTH+ `EXPONENT + 6] MaxAB_0
[`DWIDTH+ `EXPONENT + 5 :`DWIDTH+6] CExp_0
[`DWIDTH+5:`DWIDTH+1] InputExc_0
[`DWIDTH:0] Sum_4
*/						
			//pipe_7 <= {pipe_6[`DWIDTH+`EXPONENT+16:`DWIDTH+1], SumS_7[`DWIDTH:0]} ;	
/* PIPE_8:
[2*`EXPONENT + `MANTISSA + 15] FG_8 
[2*`EXPONENT + `MANTISSA + 14] operation
[2*`EXPONENT + `MANTISSA + 13] PSgn_4
[2*`EXPONENT + `MANTISSA + 12] Sa_0
[2*`EXPONENT + `MANTISSA + 11] Sb_0
[2*`EXPONENT + `MANTISSA + 10] MaxAB_0
[2*`EXPONENT + `MANTISSA + 9:`EXPONENT + `MANTISSA + 10] CExp_0
[`EXPONENT + `MANTISSA + 9:`EXPONENT + `MANTISSA + 5] InputExc_8
[`EXPONENT + `MANTISSA + 4 :`EXPONENT + 5] NormM_8 
[`EXPONENT + 4 :4] NormE_8
[3] ZeroSum_8
[2] NegE_8
[1] R_8
[0] S_8
*/				
			pipe_8 <= {FG_8, pipe_7[`DWIDTH+`EXPONENT+16], pipe_7[`DWIDTH+`EXPONENT+10], pipe_7[`DWIDTH+`EXPONENT+8:`DWIDTH+1], NormM_8[`MANTISSA-1:0], NormE_8[`EXPONENT:0], ZeroSum_8, NegE_8, R_8, S_8} ;	
/* pipe_9:
[`DWIDTH + 8 :9] P_int
[8] NegE_8
[7] R_8
[6] S_8
[5:1] InputExc_8
[0] EOF
*/				
			//pipe_9 <= {P_int[`DWIDTH-1:0], pipe_8[2], pipe_8[1], pipe_8[0], pipe_8[`EXPONENT+`MANTISSA+9:`EXPONENT+`MANTISSA+5], EOF} ;	
		end
	end		
	
endmodule


//
// Description:	 	The pre-alignment module is responsible for taking the inputs
//							apart and checking the parts for exceptions.
//							The exponent difference is also calculated in this module.
//


module FPAddSub_PrealignModule(
		A,
		B,
		operation,
		Sa,
		Sb,
		ShiftDet,
		InputExc,
		Aout,
		Bout,
		Opout
	);
	
	// Input ports
	input [`DWIDTH-1:0] A ;										// Input A, a 32-bit floating point number
	input [`DWIDTH-1:0] B ;										// Input B, a 32-bit floating point number
	input operation ;
	
	// Output ports
	output Sa ;												// A's sign
	output Sb ;												// B's sign
	output [2*`EXPONENT-1:0] ShiftDet ;
	output [4:0] InputExc ;								// Input numbers are exceptions
	output [`DWIDTH-2:0] Aout ;
	output [`DWIDTH-2:0] Bout ;
	output Opout ;
	
	// Internal signals									// If signal is high...
	wire ANaN ;												// A is a NaN (Not-a-Number)
	wire BNaN ;												// B is a NaN
	wire AInf ;												// A is infinity
	wire BInf ;												// B is infinity
	wire [`EXPONENT-1:0] DAB ;										// ExpA - ExpB					
	wire [`EXPONENT-1:0] DBA ;										// ExpB - ExpA	
	
	assign ANaN = &(A[`DWIDTH-2:`DWIDTH-1-`EXPONENT]) & |(A[`MANTISSA-1:0]) ;		// All one exponent and not all zero mantissa - NaN
	assign BNaN = &(B[`DWIDTH-2:`DWIDTH-1-`EXPONENT]) & |(B[`MANTISSA-1:0]);		// All one exponent and not all zero mantissa - NaN
	assign AInf = &(A[`DWIDTH-2:`DWIDTH-1-`EXPONENT]) & ~|(A[`MANTISSA-1:0]) ;	// All one exponent and all zero mantissa - Infinity
	assign BInf = &(B[`DWIDTH-2:`DWIDTH-1-`EXPONENT]) & ~|(B[`MANTISSA-1:0]) ;	// All one exponent and all zero mantissa - Infinity
	
	// Put all flags into exception vector
	assign InputExc = {(ANaN | BNaN | AInf | BInf), ANaN, BNaN, AInf, BInf} ;
	
	//assign DAB = (A[30:23] - B[30:23]) ;
	//assign DBA = (B[30:23] - A[30:23]) ;
	assign DAB = (A[`DWIDTH-2:`MANTISSA] + ~(B[`DWIDTH-2:`MANTISSA]) + 1) ;
	assign DBA = (B[`DWIDTH-2:`MANTISSA] + ~(A[`DWIDTH-2:`MANTISSA]) + 1) ;
	
	assign Sa = A[`DWIDTH-1] ;									// A's sign bit
	assign Sb = B[`DWIDTH-1] ;									// B's sign	bit
	assign ShiftDet = {DBA[`EXPONENT-1:0], DAB[`EXPONENT-1:0]} ;		// Shift data
	assign Opout = operation ;
	assign Aout = A[`DWIDTH-2:0] ;
	assign Bout = B[`DWIDTH-2:0] ;
	
endmodule


//
// Description:	 	The alignment module determines the larger input operand and
//							sets the mantissas, shift and common exponent accordingly.
//


module FPAddSub_AlignModule (
		A,
		B,
		ShiftDet,
		CExp,
		MaxAB,
		Shift,
		Mmin,
		Mmax
	);
	
	// Input ports
	input [`DWIDTH-2:0] A ;								// Input A, a 32-bit floating point number
	input [`DWIDTH-2:0] B ;								// Input B, a 32-bit floating point number
	input [2*`EXPONENT-1:0] ShiftDet ;
	
	// Output ports
	output [`EXPONENT-1:0] CExp ;							// Common Exponent
	output MaxAB ;									// Incidates larger of A and B (0/A, 1/B)
	output [`EXPONENT-1:0] Shift ;							// Number of steps to smaller mantissa shift right
	output [`MANTISSA-1:0] Mmin ;							// Smaller mantissa 
	output [`MANTISSA-1:0] Mmax ;							// Larger mantissa
	
	// Internal signals
	//wire BOF ;										// Check for shifting overflow if B is larger
	//wire AOF ;										// Check for shifting overflow if A is larger
	
	assign MaxAB = (A[`DWIDTH-2:0] < B[`DWIDTH-2:0]) ;	
	//assign BOF = ShiftDet[9:5] < 25 ;		// Cannot shift more than 25 bits
	//assign AOF = ShiftDet[4:0] < 25 ;		// Cannot shift more than 25 bits
	
	// Determine final shift value
	//assign Shift = MaxAB ? (BOF ? ShiftDet[9:5] : 5'b11001) : (AOF ? ShiftDet[4:0] : 5'b11001) ;
	
	assign Shift = MaxAB ? ShiftDet[2*`EXPONENT-1:`EXPONENT] : ShiftDet[`EXPONENT-1:0] ;
	
	// Take out smaller mantissa and append shift space
	assign Mmin = MaxAB ? A[`MANTISSA-1:0] : B[`MANTISSA-1:0] ; 
	
	// Take out larger mantissa	
	assign Mmax = MaxAB ? B[`MANTISSA-1:0]: A[`MANTISSA-1:0] ;	
	
	// Common exponent
	assign CExp = (MaxAB ? B[`MANTISSA+`EXPONENT-1:`MANTISSA] : A[`MANTISSA+`EXPONENT-1:`MANTISSA]) ;		
	
endmodule


// Description:	 Alignment shift stage 1, performs 16|12|8|4 shift
//


// ONLY THIS MODULE IS HARDCODED for half precision fp16 and bfloat16
module FPAddSub_AlignShift1(
		//bf16,
		MminP,
		Shift,
		Mmin
	);
	
	// Input ports
	//input bf16;
	input [`MANTISSA-1:0] MminP ;						// Smaller mantissa after 16|12|8|4 shift
	input [`EXPONENT-3:0] Shift ;						// Shift amount. Last 2 bits of shifting are done in next stage. Hence, we have [`EXPONENT - 2] bits
	
	// Output ports
	output [`MANTISSA:0] Mmin ;						// The smaller mantissa
	

	wire bf16;
	assign bf16 = 1'b1; //hardcoding to 1, to avoid ODIN issue. a `ifdef here wasn't working. apparently, nested `ifdefs don't work

	// Internal signals
	reg	  [`MANTISSA:0]		Lvl1;
	reg	  [`MANTISSA:0]		Lvl2;
	wire    [2*`MANTISSA+1:0]    Stage1;	
	integer           i;                // Loop variable

	wire [`MANTISSA:0] temp_0; 

assign temp_0 = 0;

	always @(*) begin
		if (bf16 == 1'b1) begin						
//hardcoding for bfloat16
	//For bfloat16, we can shift the mantissa by a max of 7 bits since mantissa has a width of 7. 
	//Hence if either, bit[3]/bit[4]/bit[5]/bit[6]/bit[7] is 1, we can make it 0. This corresponds to bits [5:1] in our updated shift which doesn't contain last 2 bits.
		//Lvl1 <= (Shift[1]|Shift[2]|Shift[3]|Shift[4]|Shift[5]) ? {temp_0} : {1'b1, MminP};  // MANTISSA + 1 width	
		Lvl1 <= (|Shift[`EXPONENT-3:1]) ? {temp_0} : {1'b1, MminP};  // MANTISSA + 1 width	
		end
		else begin
		//for half precision fp16, 10 bits can be shifted. Hence, only shifts till 10 (01010)can be made. 
		Lvl1 <= Shift[2] ? {temp_0} : {1'b1, MminP};
		end
	end
	
	assign Stage1 = { temp_0, Lvl1}; //2*MANTISSA + 2 width

	always @(*) begin    					// Rotate {0 | 4 } bits
	if(bf16 == 1'b1) begin
	  case (Shift[0])
			// Rotate by 0	
			1'b0:  Lvl2 <= Stage1[`MANTISSA:0];       			
			// Rotate by 4	
			1'b1:  begin for (i=0; i<=`MANTISSA; i=i+1) begin Lvl2[i] <= Stage1[i+4]; end Lvl2[`MANTISSA:`MANTISSA-3] <= 0; end
	  endcase
	end
	else begin
	  case (Shift[1:0])					// Rotate {0 | 4 | 8} bits
			// Rotate by 0	
			2'b00:  Lvl2 <= Stage1[`MANTISSA:0];       			
			// Rotate by 4	
			2'b01:  begin for (i=0; i<=`MANTISSA; i=i+1) begin Lvl2[i] <= Stage1[i+4]; end Lvl2[`MANTISSA:`MANTISSA-3] <= 0; end
			// Rotate by 8
			2'b10:  begin for (i=0; i<=`MANTISSA; i=i+1) begin Lvl2[i] <= Stage1[i+8]; end Lvl2[`MANTISSA:`MANTISSA-7] <= 0; end
			// Rotate by 12	
			2'b11: Lvl2[`MANTISSA: 0] <= 0; 
			//2'b11:  begin for (i=0; i<=`MANTISSA; i=i+1) begin Lvl2[i] <= Stage1[i+12]; end Lvl2[`MANTISSA:`MANTISSA-12] <= 0; end
	  endcase
	end
	end

	// Assign output to next shift stage
	assign Mmin = Lvl2;
	
endmodule


// Description:	 Alignment shift stage 2, performs 3|2|1 shift
//


module FPAddSub_AlignShift2(
		MminP,
		Shift,
		Mmin
	);
	
	// Input ports
	input [`MANTISSA:0] MminP ;						// Smaller mantissa after 16|12|8|4 shift
	input [1:0] Shift ;						// Shift amount. Last 2 bits
	
	// Output ports
	output [`MANTISSA:0] Mmin ;						// The smaller mantissa
	
	// Internal Signal
	reg	  [`MANTISSA:0]		Lvl3;
	wire    [2*`MANTISSA+1:0]    Stage2;	
	integer           j;               // Loop variable
	
	assign Stage2 = {11'b0, MminP};

	always @(*) begin    // Rotate {0 | 1 | 2 | 3} bits
	  case (Shift[1:0])
			// Rotate by 0
			2'b00:  Lvl3 <= Stage2[`MANTISSA:0];   
			// Rotate by 1
			2'b01:  begin for (j=0; j<=`MANTISSA; j=j+1)  begin Lvl3[j] <= Stage2[j+1]; end Lvl3[`MANTISSA] <= 0; end 
			// Rotate by 2
			2'b10:  begin for (j=0; j<=`MANTISSA; j=j+1)  begin Lvl3[j] <= Stage2[j+2]; end Lvl3[`MANTISSA:`MANTISSA-1] <= 0; end 
			// Rotate by 3
			2'b11:  begin for (j=0; j<=`MANTISSA; j=j+1)  begin Lvl3[j] <= Stage2[j+3]; end Lvl3[`MANTISSA:`MANTISSA-2] <= 0; end 	  
	  endcase
	end
	
	// Assign output
	assign Mmin = Lvl3;						// Take out smaller mantissa				

endmodule


//
// Description:	 Module that executes the addition or subtraction on mantissas.
//


module FPAddSub_ExecutionModule(
		Mmax,
		Mmin,
		Sa,
		Sb,
		MaxAB,
		OpMode,
		Sum,
		PSgn,
		Opr
    );

	// Input ports
	input [`MANTISSA-1:0] Mmax ;					// The larger mantissa
	input [`MANTISSA:0] Mmin ;					// The smaller mantissa
	input Sa ;								// Sign bit of larger number
	input Sb ;								// Sign bit of smaller number
	input MaxAB ;							// Indicates the larger number (0/A, 1/B)
	input OpMode ;							// Operation to be performed (0/Add, 1/Sub)
	
	// Output ports
	output [`DWIDTH:0] Sum ;					// The result of the operation
	output PSgn ;							// The sign for the result
	output Opr ;							// The effective (performed) operation

	wire [`EXPONENT-1:0]temp_1;

	assign Opr = (OpMode^Sa^Sb); 		// Resolve sign to determine operation
	assign temp_1 = 0;
	// Perform effective operation
//SAMIDH_UNSURE 5--> 8

	assign Sum = (OpMode^Sa^Sb) ? ({1'b1, Mmax, temp_1} - {Mmin, temp_1}) : ({1'b1, Mmax, temp_1} + {Mmin, temp_1}) ;
	
	// Assign result sign
	assign PSgn = (MaxAB ? Sb : Sa) ;

endmodule


//
// Description:	 Determine the normalization shift amount and perform 16-shift
//


module FPAddSub_NormalizeModule(
		Sum,
		Mmin,
		Shift
    );

	// Input ports
	input [`DWIDTH:0] Sum ;					// Mantissa sum including hidden 1 and GRS
	
	// Output ports
	output [`DWIDTH:0] Mmin ;					// Mantissa after 16|0 shift
	output [4:0] Shift ;					// Shift amount
	//Changes in this doesn't matter since even Bfloat16 can't go beyond 7 shift to the mantissa (only 3 bits valid here)  
	// Determine normalization shift amount by finding leading nought
	assign Shift =  ( 
		Sum[16] ? 5'b00000 :	 
		Sum[15] ? 5'b00001 : 
		Sum[14] ? 5'b00010 : 
		Sum[13] ? 5'b00011 : 
		Sum[12] ? 5'b00100 : 
		Sum[11] ? 5'b00101 : 
		Sum[10] ? 5'b00110 : 
		Sum[9] ? 5'b00111 :
		Sum[8] ? 5'b01000 :
		Sum[7] ? 5'b01001 :
		Sum[6] ? 5'b01010 :
		Sum[5] ? 5'b01011 :
		Sum[4] ? 5'b01100 : 5'b01101
	//	Sum[19] ? 5'b01101 :
	//	Sum[18] ? 5'b01110 :
	//	Sum[17] ? 5'b01111 :
	//	Sum[16] ? 5'b10000 :
	//	Sum[15] ? 5'b10001 :
	//	Sum[14] ? 5'b10010 :
	//	Sum[13] ? 5'b10011 :
	//	Sum[12] ? 5'b10100 :
	//	Sum[11] ? 5'b10101 :
	//	Sum[10] ? 5'b10110 :
	//	Sum[9] ? 5'b10111 :
	//	Sum[8] ? 5'b11000 :
	//	Sum[7] ? 5'b11001 : 5'b11010
	);
	
	reg	  [`DWIDTH:0]		Lvl1;
	
	always @(*) begin
		// Rotate by 16?
		Lvl1 <= Shift[4] ? {Sum[8:0], 8'b00000000} : Sum; 
	end
	
	// Assign outputs
	assign Mmin = Lvl1;						// Take out smaller mantissa

endmodule


// Description:	 Normalization shift stage 1, performs 12|8|4|3|2|1|0 shift
//
//Hardcoding loop start and end values of i. To avoid ODIN limitations. i=`DWIDTH*2+1 wasn't working.

module FPAddSub_NormalizeShift1(
		MminP,
		Shift,
		Mmin
	);
	
	// Input ports
	input [`DWIDTH:0] MminP ;						// Smaller mantissa after 16|12|8|4 shift
	input [3:0] Shift ;						// Shift amount
	
	// Output ports
	output [`DWIDTH:0] Mmin ;						// The smaller mantissa
	
	reg	  [`DWIDTH:0]		Lvl2;
	wire    [2*`DWIDTH+1:0]    Stage1;	
	reg	  [`DWIDTH:0]		Lvl3;
	wire    [2*`DWIDTH+1:0]    Stage2;	
	integer           i;               	// Loop variable
	
	assign Stage1 = {MminP, MminP};

	always @(*) begin    					// Rotate {0 | 4 | 8 | 12} bits
	  case (Shift[3:2])
			// Rotate by 0
			2'b00: Lvl2 <= Stage1[`DWIDTH:0];       		
			// Rotate by 4
			2'b01: begin for (i=33; i>=17; i=i-1) begin Lvl2[i-33] <= Stage1[i-4]; end Lvl2[3:0] <= 0; end
			// Rotate by 8
			2'b10: begin for (i=33; i>=17; i=i-1) begin Lvl2[i-33] <= Stage1[i-8]; end Lvl2[7:0] <= 0; end
			// Rotate by 12
			2'b11: begin for (i=33; i>=17; i=i-1) begin Lvl2[i-33] <= Stage1[i-12]; end Lvl2[11:0] <= 0; end
	  endcase
	end
	
	assign Stage2 = {Lvl2, Lvl2};

	always @(*) begin   				 		// Rotate {0 | 1 | 2 | 3} bits
	  case (Shift[1:0])
			// Rotate by 0
			2'b00:  Lvl3 <= Stage2[`DWIDTH:0];
			// Rotate by 1
			2'b01: begin for (i=33; i>=17; i=i-1) begin Lvl3[i-`DWIDTH-1] <= Stage2[i-1]; end Lvl3[0] <= 0; end 
			// Rotate by 2
			2'b10: begin for (i=33; i>=17; i=i-1) begin Lvl3[i-`DWIDTH-1] <= Stage2[i-2]; end Lvl3[1:0] <= 0; end
			// Rotate by 3
			2'b11: begin for (i=33; i>=17; i=i-1) begin Lvl3[i-`DWIDTH-1] <= Stage2[i-3]; end Lvl3[2:0] <= 0; end
	  endcase
	end
	
	// Assign outputs
	assign Mmin = Lvl3;						// Take out smaller mantissa			
	
endmodule


// Description:	 Normalization shift stage 2, calculates post-normalization
//						 mantissa and exponent, as well as the bits used in rounding		
//


module FPAddSub_NormalizeShift2(
		PSSum,
		CExp,
		Shift,
		NormM,
		NormE,
		ZeroSum,
		NegE,
		R,
		S,
		FG
	);
	
	// Input ports
	input [`DWIDTH:0] PSSum ;					// The Pre-Shift-Sum
	input [`EXPONENT-1:0] CExp ;
	input [4:0] Shift ;					// Amount to be shifted

	// Output ports
	output [`MANTISSA-1:0] NormM ;				// Normalized mantissa
	output [`EXPONENT:0] NormE ;					// Adjusted exponent
	output ZeroSum ;						// Zero flag
	output NegE ;							// Flag indicating negative exponent
	output R ;								// Round bit
	output S ;								// Final sticky bit
	output FG ;

	// Internal signals
	wire MSBShift ;						// Flag indicating that a second shift is needed
	wire [`EXPONENT:0] ExpOF ;					// MSB set in sum indicates overflow
	wire [`EXPONENT:0] ExpOK ;					// MSB not set, no adjustment
	
	// Calculate normalized exponent and mantissa, check for all-zero sum
	assign MSBShift = PSSum[`DWIDTH] ;		// Check MSB in unnormalized sum
	assign ZeroSum = ~|PSSum ;			// Check for all zero sum
	assign ExpOK = CExp - Shift ;		// Adjust exponent for new normalized mantissa
	assign NegE = ExpOK[`EXPONENT] ;			// Check for exponent overflow
	assign ExpOF = CExp - Shift + 1'b1 ;		// If MSB set, add one to exponent(x2)
	assign NormE = MSBShift ? ExpOF : ExpOK ;			// Check for exponent overflow
	assign NormM = PSSum[`DWIDTH-1:`EXPONENT+1] ;		// The new, normalized mantissa
	
	// Also need to compute sticky and round bits for the rounding stage
	assign FG = PSSum[`EXPONENT] ; 
	assign R = PSSum[`EXPONENT-1] ;
	assign S = |PSSum[`EXPONENT-2:0] ;
	
endmodule


// Description:	 Performs 'Round to nearest, tie to even'-rounding on the
//						 normalized mantissa according to the G, R, S bits. Calculates
//						 final result and checks for exponent overflow.
//


module FPAddSub_RoundModule(
		ZeroSum,
		NormE,
		NormM,
		R,
		S,
		G,
		Sa,
		Sb,
		Ctrl,
		MaxAB,
		Z,
		EOF
    );

	// Input ports
	input ZeroSum ;					// Sum is zero
	input [`EXPONENT:0] NormE ;				// Normalized exponent
	input [`MANTISSA-1:0] NormM ;				// Normalized mantissa
	input R ;							// Round bit
	input S ;							// Sticky bit
	input G ;
	input Sa ;							// A's sign bit
	input Sb ;							// B's sign bit
	input Ctrl ;						// Control bit (operation)
	input MaxAB ;
	
	// Output ports
	output [`DWIDTH-1:0] Z ;					// Final result
	output EOF ;
	
	// Internal signals
	wire [`MANTISSA:0] RoundUpM ;			// Rounded up sum with room for overflow
	wire [`MANTISSA-1:0] RoundM ;				// The final rounded sum
	wire [`EXPONENT:0] RoundE ;				// Rounded exponent (note extra bit due to poential overflow	)
	wire RoundUp ;						// Flag indicating that the sum should be rounded up
        wire FSgn;
	wire ExpAdd ;						// May have to add 1 to compensate for overflow 
	wire RoundOF ;						// Rounding overflow
	
	wire [`EXPONENT:0]temp_2;
	assign temp_2 = 0;
	// The cases where we need to round upwards (= adding one) in Round to nearest, tie to even
	assign RoundUp = (G & ((R | S) | NormM[0])) ;
	
	// Note that in the other cases (rounding down), the sum is already 'rounded'
	assign RoundUpM = (NormM + 1) ;								// The sum, rounded up by 1
	assign RoundM = (RoundUp ? RoundUpM[`MANTISSA-1:0] : NormM) ; 	// Compute final mantissa	
	assign RoundOF = RoundUp & RoundUpM[`MANTISSA] ; 				// Check for overflow when rounding up

	// Calculate post-rounding exponent
	assign ExpAdd = (RoundOF ? 1'b1 : 1'b0) ; 				// Add 1 to exponent to compensate for overflow
	assign RoundE = ZeroSum ? temp_2 : (NormE + ExpAdd) ; 							// Final exponent

	// If zero, need to determine sign according to rounding
	assign FSgn = (ZeroSum & (Sa ^ Sb)) | (ZeroSum ? (Sa & Sb & ~Ctrl) : ((~MaxAB & Sa) | ((Ctrl ^ Sb) & (MaxAB | Sa)))) ;

	// Assign final result
	assign Z = {FSgn, RoundE[`EXPONENT-1:0], RoundM[`MANTISSA-1:0]} ;
	
	// Indicate exponent overflow
	assign EOF = RoundE[`EXPONENT];
	
endmodule


//
// Description:	 Check the final result for exception conditions and set
//						 flags accordingly.
//


module FPAddSub_ExceptionModule(
		Z,
		NegE,
		R,
		S,
		InputExc,
		EOF,
		P,
		Flags
    );
	 
	// Input ports
	input [`DWIDTH-1:0] Z	;					// Final product
	input NegE ;						// Negative exponent?
	input R ;							// Round bit
	input S ;							// Sticky bit
	input [4:0] InputExc ;			// Exceptions in inputs A and B
	input EOF ;
	
	// Output ports
	output [`DWIDTH-1:0] P ;					// Final result
	output [4:0] Flags ;				// Exception flags
	
	// Internal signals
	wire Overflow ;					// Overflow flag
	wire Underflow ;					// Underflow flag
	wire DivideByZero ;				// Divide-by-Zero flag (always 0 in Add/Sub)
	wire Invalid ;						// Invalid inputs or result
	wire Inexact ;						// Result is inexact because of rounding
	
	// Exception flags
	
	// Result is too big to be represented
	assign Overflow = EOF | InputExc[1] | InputExc[0] ;
	
	// Result is too small to be represented
	assign Underflow = NegE & (R | S);
	
	// Infinite result computed exactly from finite operands
	assign DivideByZero = &(Z[`MANTISSA+`EXPONENT-1:`MANTISSA]) & ~|(Z[`MANTISSA+`EXPONENT-1:`MANTISSA]) & ~InputExc[1] & ~InputExc[0];
	
	// Invalid inputs or operation
	assign Invalid = |(InputExc[4:2]) ;
	
	// Inexact answer due to rounding, overflow or underflow
	assign Inexact = (R | S) | Overflow | Underflow;
	
	// Put pieces together to form final result
	assign P = Z ;
	
	// Collect exception flags	
	assign Flags = {Overflow, Underflow, DivideByZero, Invalid, Inexact} ; 	
	
endmodule


`endif

