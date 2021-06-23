//////////////////////////////////////////////////////////////////////////////
// Accelerator for Softmax classification layer. Based on implementation in:
// Z. Wei et al., “Design Space Exploration for Softmax Implementations,” in 
// International Conference on Application-specific Systems, Architectures
// and Processors (ASAP), 2020. 
// IEEE FP16 precision is used.
// LUT based log and exp units, Adder tree (reduction), Comparators, Subtractors. 
// RAM outside of the design.
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Authors: Pragnesh Patel and Aman Arora
//////////////////////////////////////////////////////////////////////////////

//softmax_p8_smem_rfloat16_alut_v512_b2_-0.1_0.1.v

`ifndef DEFINES_DONE
`define DEFINES_DONE
`define EXPONENT 5
`define MANTISSA 10
`define SIGN 1
`define DATAWIDTH (`SIGN+`EXPONENT+`MANTISSA)
`define IEEE_COMPLIANCE 1
`define NUM 8
`define ADDRSIZE 7
`define ADDRSIZE_FOR_TB 10
`endif


`timescale 1ns / 1ps

//fixed adder adds unsigned fixed numbers. Overflow flag is high in case of overflow
module softmax(
  inp,      //data in from memory to max block
  sub0_inp, //data inputs from memory to first-stage subtractors
  sub1_inp, //data inputs from memory to second-stage subtractors

  start_addr,   //the first address that contains input data in the on-chip memory
  end_addr,     //max address containing required data

  addr,          //address corresponding to data inp
  sub0_inp_addr, //address corresponding to sub0_inp
  sub1_inp_addr, //address corresponding to sub1_inp

  outp0,
  outp1,
  outp2,
  outp3,
  outp4,
  outp5,
  outp6,
  outp7,

  clk,
  reset,
  init,   //the signal indicating to latch the new start address
  done,   //done signal asserts when the softmax calculation is over
  start); //start signal for the overall softmax operation

  input clk;
  input reset;
  input start;
  input init;

  input  [`DATAWIDTH*`NUM-1:0] inp;
  input  [`DATAWIDTH*`NUM-1:0] sub0_inp;
  input  [`DATAWIDTH*`NUM-1:0] sub1_inp;
  input  [`ADDRSIZE-1:0]       end_addr;
  input  [`ADDRSIZE-1:0]       start_addr;

  output [`ADDRSIZE-1 :0] addr;
  output  [`ADDRSIZE-1:0] sub0_inp_addr;
  output  [`ADDRSIZE-1:0] sub1_inp_addr;

  output [`DATAWIDTH-1:0] outp0;
  output [`DATAWIDTH-1:0] outp1;
  output [`DATAWIDTH-1:0] outp2;
  output [`DATAWIDTH-1:0] outp3;
  output [`DATAWIDTH-1:0] outp4;
  output [`DATAWIDTH-1:0] outp5;
  output [`DATAWIDTH-1:0] outp6;
  output [`DATAWIDTH-1:0] outp7;
  output done;

  reg [`DATAWIDTH*`NUM-1:0] inp_reg;
  reg [`ADDRSIZE-1:0] addr;
  reg [`DATAWIDTH*`NUM-1:0] sub0_inp_reg;
  reg [`DATAWIDTH*`NUM-1:0] sub1_inp_reg;
  reg [`ADDRSIZE-1:0] sub0_inp_addr;
  reg [`ADDRSIZE-1:0] sub1_inp_addr;


  ////-----------control signals--------------////
  reg mode1_start;
  reg mode1_run;
  reg mode2_start;
  reg mode2_run;

  reg mode3_stage_run2;
  reg mode3_stage_run;
  reg mode7_stage_run2;
  reg mode7_stage_run;

  reg mode3_run;

  reg mode1_stage0_run;
  reg mode1_stage1_run;
  reg mode1_stage2_run;
  wire mode1_stage3_run;
  assign mode1_stage3_run = mode1_run;

  reg mode4_stage1_run_a;
  reg mode4_stage2_run_a;
  reg mode4_stage0_run;
  reg mode4_stage1_run;
  reg mode4_stage2_run;
  reg mode4_stage3_run;

  reg mode5_run;
  reg mode6_run;
  reg mode7_run;
  reg presub_start;
  reg presub_run;
  reg done;

  always @(posedge clk)begin
    mode4_stage1_run_a <= mode4_stage1_run;
    mode4_stage2_run_a <= mode4_stage2_run;
  end

  always @(posedge clk) begin
    if(reset) begin
      inp_reg <= 0;
      addr <= 0;
      mode1_start <= 0;
      mode1_run <= 0;
    end
    //init latch the input address
    else if(init) begin
      addr <= start_addr;
    end
    //start the mode1 max calculation
    else if(start)begin
      mode1_start <= 1;
    end
    //logic when to finish mode1 and trigger mode2 to latch the mode2 address
    else if(mode1_start && addr < end_addr) begin
      addr <= addr + 1;
      inp_reg <= inp;
      mode1_run <= 1;
    end else if(addr == end_addr)begin
      addr <= 0;
      mode1_run <= 0;
      mode1_start <= 0;
    end else begin
      mode1_run <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      mode1_stage2_run <= 0;
    end
    else if (mode1_stage3_run == 1) begin
      mode1_stage2_run <= 1;
    end
    else begin
      mode1_stage2_run <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      mode1_stage1_run <= 0;
    end
    else if (mode1_stage2_run == 1) begin
      mode1_stage1_run <= 1;
    end
    else begin
      mode1_stage1_run <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      mode1_stage0_run <= 0;
    end
    else if (mode1_stage1_run == 1) begin
      mode1_stage0_run <= 1;
    end
    else begin
      mode1_stage0_run <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      sub0_inp_addr <= 0;
      sub0_inp_reg <= 0;
      mode2_start <= 0;
      mode2_run <= 0;
    end
    else if ((addr == end_addr) && ~(mode1_start && (addr<end_addr))) begin
        mode2_start <= 1;
        sub0_inp_addr <= start_addr;
	  end
    //logic when to finish mode2
    else if(mode2_start && sub0_inp_addr < end_addr) begin
      sub0_inp_addr <= sub0_inp_addr + 1;
      sub0_inp_reg <= sub0_inp;
      mode2_run <= 1;
    end 
	else if(sub0_inp_addr == end_addr)begin
      sub0_inp_addr <= 0;
      sub0_inp_reg <= 0;
      mode2_run <= 0;
      mode2_start <= 0;
    end
  end	

  always @(posedge clk) begin
    if(reset) begin
      mode3_stage_run2 <= 0;
    end
    //logic when to trigger mode3
    else if(mode2_run == 1) begin
      mode3_stage_run2 <= 1;
    end else begin
      mode3_stage_run2 <= 0;
    end
  end	

  always @(posedge clk) begin
    if(reset) begin
      mode3_stage_run <= 0;
    end
    else if(mode3_stage_run2 == 1) begin
      mode3_stage_run <= 1;
    end else begin
      mode3_stage_run <= 0;
    end
  end	
  
  always @(posedge clk) begin
    if(reset) begin
      mode3_run <= 0;
    end
    else if(mode3_stage_run == 1) begin
      mode3_run <= 1;
    end else begin
      mode3_run <= 0;
    end
  end

    //logic when to trigger mode4 last stage adderTree, since the final results of adderTree
    //is always ready 1 cycle after mode3 finishes, so there is no need on extra
    //logic to control the adderTree outputs
  always @(posedge clk) begin
    if(reset) begin
      mode4_stage3_run <= 0;
    end
    else if (mode3_run == 1) begin
      mode4_stage3_run <= 1;
    end else begin
      mode4_stage3_run <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      mode4_stage2_run <= 0;
    end
    else if (mode4_stage3_run == 1) begin
      mode4_stage2_run <= 1;
    end else begin
      mode4_stage2_run <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      mode4_stage1_run <= 0;
    end
    else if (mode4_stage2_run == 1) begin
      mode4_stage1_run <= 1;
    end else begin
      mode4_stage1_run <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      mode4_stage0_run <= 0;
    end
    else if (mode4_stage1_run == 1) begin
      mode4_stage0_run <= 1;
    end else begin
      mode4_stage0_run <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      mode5_run <= 0;
    end
    //mode5 should be triggered right at the falling edge of mode4_stage1_run
    else if(mode4_stage1_run_a & ~mode4_stage1_run) begin
      mode5_run <= 1;
    end 
	else if(mode4_stage1_run == 0) begin
      mode5_run <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      presub_start <= 0;
      sub1_inp_addr <= 0;
      sub1_inp_reg <= 0;
      presub_run <= 0;
    end
    else if(mode4_stage2_run_a & ~mode4_stage2_run) begin
      presub_start <= 1;
      sub1_inp_addr <= start_addr;
      sub1_inp_reg <= sub1_inp;
    end
    else if(~reset && presub_start && sub1_inp_addr < end_addr)begin
      sub1_inp_addr <= sub1_inp_addr + 1;
      sub1_inp_reg <= sub1_inp;
      presub_run <= 1;
    end 
	else if(sub1_inp_addr == end_addr) begin
      presub_run <= 0;
      presub_start <= 0;
      sub1_inp_addr <= 0;
      sub1_inp_reg <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      mode6_run <= 0;
	end  
    else if(presub_run) begin
      mode6_run <= 1;
    end else begin
      mode6_run <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      mode7_stage_run2 <= 0;
	end
    else if(mode6_run == 1) begin
      mode7_stage_run2 <= 1;
    end else begin
      mode7_stage_run2 <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      mode7_stage_run <= 0;
	end
    else if(mode7_stage_run2 == 1) begin
      mode7_stage_run <= 1;
    end else begin
      mode7_stage_run <= 0;
    end
  end 

  always @(posedge clk) begin
    if(reset) begin
      mode7_run <= 0;
	end
	else if(mode7_stage_run == 1) begin
      mode7_run <= 1;
    end else begin
      mode7_run <= 0;
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      done <= 0;
	end  
    else if(mode7_run) begin
      done <= 1;
    end else begin
      done <= 0;
    end
  end


  ////------mode1 max block---------///////
  wire [`DATAWIDTH-1:0] max_outp;

  mode1_max_tree mode1_max(
      .inp0(inp_reg[`DATAWIDTH*1-1:`DATAWIDTH*0]),
      .inp1(inp_reg[`DATAWIDTH*2-1:`DATAWIDTH*1]),
      .inp2(inp_reg[`DATAWIDTH*3-1:`DATAWIDTH*2]),
      .inp3(inp_reg[`DATAWIDTH*4-1:`DATAWIDTH*3]),
      .inp4(inp_reg[`DATAWIDTH*5-1:`DATAWIDTH*4]),
      .inp5(inp_reg[`DATAWIDTH*6-1:`DATAWIDTH*5]),
      .inp6(inp_reg[`DATAWIDTH*7-1:`DATAWIDTH*6]),
      .inp7(inp_reg[`DATAWIDTH*8-1:`DATAWIDTH*7]),
      .mode1_stage0_run(mode1_stage0_run),
	  .mode1_stage1_run(mode1_stage1_run),
      .mode1_stage2_run(mode1_stage2_run), 
      .mode1_stage3_run(mode1_stage3_run),
      .clk(clk),
      .reset(reset),
      .outp(max_outp));

  ////------mode2 subtraction---------///////
  wire [`DATAWIDTH-1:0] mode2_outp_sub0;
  wire [`DATAWIDTH-1:0] mode2_outp_sub1;
  wire [`DATAWIDTH-1:0] mode2_outp_sub2;
  wire [`DATAWIDTH-1:0] mode2_outp_sub3;
  wire [`DATAWIDTH-1:0] mode2_outp_sub4;
  wire [`DATAWIDTH-1:0] mode2_outp_sub5;
  wire [`DATAWIDTH-1:0] mode2_outp_sub6;
  wire [`DATAWIDTH-1:0] mode2_outp_sub7;
  mode2_sub mode2_sub(
      .clk(clk),
      .rst(reset),
      .a_inp0(sub0_inp_reg[`DATAWIDTH*1-1:`DATAWIDTH*0]),
      .a_inp1(sub0_inp_reg[`DATAWIDTH*2-1:`DATAWIDTH*1]),
      .a_inp2(sub0_inp_reg[`DATAWIDTH*3-1:`DATAWIDTH*2]),
      .a_inp3(sub0_inp_reg[`DATAWIDTH*4-1:`DATAWIDTH*3]),
      .a_inp4(sub0_inp_reg[`DATAWIDTH*5-1:`DATAWIDTH*4]),
      .a_inp5(sub0_inp_reg[`DATAWIDTH*6-1:`DATAWIDTH*5]),
      .a_inp6(sub0_inp_reg[`DATAWIDTH*7-1:`DATAWIDTH*6]),
      .a_inp7(sub0_inp_reg[`DATAWIDTH*8-1:`DATAWIDTH*7]),
      .outp0(mode2_outp_sub0),
      .outp1(mode2_outp_sub1),
      .outp2(mode2_outp_sub2),
      .outp3(mode2_outp_sub3),
      .outp4(mode2_outp_sub4),
      .outp5(mode2_outp_sub5),
      .outp6(mode2_outp_sub6),
      .outp7(mode2_outp_sub7),
      .b_inp(max_outp));

  reg [`DATAWIDTH-1:0] mode2_outp_sub0_reg;
  reg [`DATAWIDTH-1:0] mode2_outp_sub1_reg;
  reg [`DATAWIDTH-1:0] mode2_outp_sub2_reg;
  reg [`DATAWIDTH-1:0] mode2_outp_sub3_reg;
  reg [`DATAWIDTH-1:0] mode2_outp_sub4_reg;
  reg [`DATAWIDTH-1:0] mode2_outp_sub5_reg;
  reg [`DATAWIDTH-1:0] mode2_outp_sub6_reg;
  reg [`DATAWIDTH-1:0] mode2_outp_sub7_reg;
  always @(posedge clk) begin
    if (reset) begin
      mode2_outp_sub0_reg <= 0;
      mode2_outp_sub1_reg <= 0;
      mode2_outp_sub2_reg <= 0;
      mode2_outp_sub3_reg <= 0;
      mode2_outp_sub4_reg <= 0;
      mode2_outp_sub5_reg <= 0;
      mode2_outp_sub6_reg <= 0;
      mode2_outp_sub7_reg <= 0;
    end else if (mode2_run) begin
      mode2_outp_sub0_reg <= mode2_outp_sub0;
      mode2_outp_sub1_reg <= mode2_outp_sub1;
      mode2_outp_sub2_reg <= mode2_outp_sub2;
      mode2_outp_sub3_reg <= mode2_outp_sub3;
      mode2_outp_sub4_reg <= mode2_outp_sub4;
      mode2_outp_sub5_reg <= mode2_outp_sub5;
      mode2_outp_sub6_reg <= mode2_outp_sub6;
      mode2_outp_sub7_reg <= mode2_outp_sub7;
    end
  end

  ////------mode3 exponential---------///////
  wire [`DATAWIDTH-1:0] mode3_outp_exp0;
  wire [`DATAWIDTH-1:0] mode3_outp_exp1;
  wire [`DATAWIDTH-1:0] mode3_outp_exp2;
  wire [`DATAWIDTH-1:0] mode3_outp_exp3;
  wire [`DATAWIDTH-1:0] mode3_outp_exp4;
  wire [`DATAWIDTH-1:0] mode3_outp_exp5;
  wire [`DATAWIDTH-1:0] mode3_outp_exp6;
  wire [`DATAWIDTH-1:0] mode3_outp_exp7;
  mode3_exp mode3_exp(
      .inp0(mode2_outp_sub0_reg),
      .inp1(mode2_outp_sub1_reg),
      .inp2(mode2_outp_sub2_reg),
      .inp3(mode2_outp_sub3_reg),
      .inp4(mode2_outp_sub4_reg),
      .inp5(mode2_outp_sub5_reg),
      .inp6(mode2_outp_sub6_reg),
      .inp7(mode2_outp_sub7_reg),

      .clk(clk),
      .reset(reset),
      .stage_run(mode3_stage_run),
      .stage_run2(mode3_stage_run2),
      .outp0(mode3_outp_exp0),
      .outp1(mode3_outp_exp1),
      .outp2(mode3_outp_exp2),
      .outp3(mode3_outp_exp3),
      .outp4(mode3_outp_exp4),
      .outp5(mode3_outp_exp5),
      .outp6(mode3_outp_exp6),
      .outp7(mode3_outp_exp7)
  );

  reg [`DATAWIDTH-1:0] mode3_outp_exp0_reg;
  reg [`DATAWIDTH-1:0] mode3_outp_exp1_reg;
  reg [`DATAWIDTH-1:0] mode3_outp_exp2_reg;
  reg [`DATAWIDTH-1:0] mode3_outp_exp3_reg;
  reg [`DATAWIDTH-1:0] mode3_outp_exp4_reg;
  reg [`DATAWIDTH-1:0] mode3_outp_exp5_reg;
  reg [`DATAWIDTH-1:0] mode3_outp_exp6_reg;
  reg [`DATAWIDTH-1:0] mode3_outp_exp7_reg;
  always @(posedge clk) begin
    if (reset) begin
      mode3_outp_exp0_reg <= 0;
      mode3_outp_exp1_reg <= 0;
      mode3_outp_exp2_reg <= 0;
      mode3_outp_exp3_reg <= 0;
      mode3_outp_exp4_reg <= 0;
      mode3_outp_exp5_reg <= 0;
      mode3_outp_exp6_reg <= 0;
      mode3_outp_exp7_reg <= 0;
    end else if (mode3_run) begin
      mode3_outp_exp0_reg <= mode3_outp_exp0;
      mode3_outp_exp1_reg <= mode3_outp_exp1;
      mode3_outp_exp2_reg <= mode3_outp_exp2;
      mode3_outp_exp3_reg <= mode3_outp_exp3;
      mode3_outp_exp4_reg <= mode3_outp_exp4;
      mode3_outp_exp5_reg <= mode3_outp_exp5;
      mode3_outp_exp6_reg <= mode3_outp_exp6;
      mode3_outp_exp7_reg <= mode3_outp_exp7;
    end
  end

  //////------mode4 pipelined adder tree---------///////
  wire [`DATAWIDTH-1:0] mode4_adder_tree_outp;
  mode4_adder_tree mode4_adder_tree(
    .inp0(mode3_outp_exp0_reg),
    .inp1(mode3_outp_exp1_reg),
    .inp2(mode3_outp_exp2_reg),
    .inp3(mode3_outp_exp3_reg),
    .inp4(mode3_outp_exp4_reg),
    .inp5(mode3_outp_exp5_reg),
    .inp6(mode3_outp_exp6_reg),
    .inp7(mode3_outp_exp7_reg),
    .mode4_stage3_run(mode4_stage3_run),
    .mode4_stage2_run(mode4_stage2_run),
    .mode4_stage1_run(mode4_stage1_run),
    .mode4_stage0_run(mode4_stage0_run),

    .clk(clk),
    .reset(reset),
    .outp(mode4_adder_tree_outp)
  );


  //////------mode5 log---------///////
  wire [`DATAWIDTH-1:0] mode5_outp_log;
  reg  [`DATAWIDTH-1:0] mode5_outp_log_reg;
  mode5_ln mode5_ln(.clk(clk), .rst(reset), .inp(mode4_adder_tree_outp), .outp(mode5_outp_log));

  always @(posedge clk) begin
    if(reset) begin
      mode5_outp_log_reg <= 0;
    end else if(mode5_run) begin
      mode5_outp_log_reg <= mode5_outp_log;
    end
  end

  //////------mode6 pre-sub---------///////
  wire [`DATAWIDTH-1:0] mode6_outp_presub0;
  wire [`DATAWIDTH-1:0] mode6_outp_presub1;
  wire [`DATAWIDTH-1:0] mode6_outp_presub2;
  wire [`DATAWIDTH-1:0] mode6_outp_presub3;
  wire [`DATAWIDTH-1:0] mode6_outp_presub4;
  wire [`DATAWIDTH-1:0] mode6_outp_presub5;
  wire [`DATAWIDTH-1:0] mode6_outp_presub6;
  wire [`DATAWIDTH-1:0] mode6_outp_presub7;
  reg [`DATAWIDTH-1:0] mode6_outp_presub0_reg;
  reg [`DATAWIDTH-1:0] mode6_outp_presub1_reg;
  reg [`DATAWIDTH-1:0] mode6_outp_presub2_reg;
  reg [`DATAWIDTH-1:0] mode6_outp_presub3_reg;
  reg [`DATAWIDTH-1:0] mode6_outp_presub4_reg;
  reg [`DATAWIDTH-1:0] mode6_outp_presub5_reg;
  reg [`DATAWIDTH-1:0] mode6_outp_presub6_reg;
  reg [`DATAWIDTH-1:0] mode6_outp_presub7_reg;

  mode6_sub pre_sub(
      .clk(clk),
      .rst(reset),
      .a_inp0(sub1_inp_reg[`DATAWIDTH*1-1:`DATAWIDTH*0]),
      .a_inp1(sub1_inp_reg[`DATAWIDTH*2-1:`DATAWIDTH*1]),
      .a_inp2(sub1_inp_reg[`DATAWIDTH*3-1:`DATAWIDTH*2]),
      .a_inp3(sub1_inp_reg[`DATAWIDTH*4-1:`DATAWIDTH*3]),
      .a_inp4(sub1_inp_reg[`DATAWIDTH*5-1:`DATAWIDTH*4]),
      .a_inp5(sub1_inp_reg[`DATAWIDTH*6-1:`DATAWIDTH*5]),
      .a_inp6(sub1_inp_reg[`DATAWIDTH*7-1:`DATAWIDTH*6]),
      .a_inp7(sub1_inp_reg[`DATAWIDTH*8-1:`DATAWIDTH*7]),
      .b_inp(max_outp),
      .outp0(mode6_outp_presub0),
      .outp1(mode6_outp_presub1),
      .outp2(mode6_outp_presub2),
      .outp3(mode6_outp_presub3),
      .outp4(mode6_outp_presub4),
      .outp5(mode6_outp_presub5),
      .outp6(mode6_outp_presub6),
      .outp7(mode6_outp_presub7)
  );
  always @(posedge clk) begin
    if (reset) begin
      mode6_outp_presub0_reg <= 0;
      mode6_outp_presub1_reg <= 0;
      mode6_outp_presub2_reg <= 0;
      mode6_outp_presub3_reg <= 0;
      mode6_outp_presub4_reg <= 0;
      mode6_outp_presub5_reg <= 0;
      mode6_outp_presub6_reg <= 0;
      mode6_outp_presub7_reg <= 0;
    end else if (presub_run) begin
      mode6_outp_presub0_reg <= mode6_outp_presub0;
      mode6_outp_presub1_reg <= mode6_outp_presub1;
      mode6_outp_presub2_reg <= mode6_outp_presub2;
      mode6_outp_presub3_reg <= mode6_outp_presub3;
      mode6_outp_presub4_reg <= mode6_outp_presub4;
      mode6_outp_presub5_reg <= mode6_outp_presub5;
      mode6_outp_presub6_reg <= mode6_outp_presub6;
      mode6_outp_presub7_reg <= mode6_outp_presub7;
    end
  end

  //////------mode6 logsub ---------///////
  wire [`DATAWIDTH-1:0] mode6_outp_logsub0;
  wire [`DATAWIDTH-1:0] mode6_outp_logsub1;
  wire [`DATAWIDTH-1:0] mode6_outp_logsub2;
  wire [`DATAWIDTH-1:0] mode6_outp_logsub3;
  wire [`DATAWIDTH-1:0] mode6_outp_logsub4;
  wire [`DATAWIDTH-1:0] mode6_outp_logsub5;
  wire [`DATAWIDTH-1:0] mode6_outp_logsub6;
  wire [`DATAWIDTH-1:0] mode6_outp_logsub7;
  reg [`DATAWIDTH-1:0] mode6_outp_logsub0_reg;
  reg [`DATAWIDTH-1:0] mode6_outp_logsub1_reg;
  reg [`DATAWIDTH-1:0] mode6_outp_logsub2_reg;
  reg [`DATAWIDTH-1:0] mode6_outp_logsub3_reg;
  reg [`DATAWIDTH-1:0] mode6_outp_logsub4_reg;
  reg [`DATAWIDTH-1:0] mode6_outp_logsub5_reg;
  reg [`DATAWIDTH-1:0] mode6_outp_logsub6_reg;
  reg [`DATAWIDTH-1:0] mode6_outp_logsub7_reg;

  mode6_sub log_sub(
      .clk(clk),
      .rst(reset),
      .a_inp0(mode6_outp_presub0_reg),
      .a_inp1(mode6_outp_presub1_reg),
      .a_inp2(mode6_outp_presub2_reg),
      .a_inp3(mode6_outp_presub3_reg),
      .a_inp4(mode6_outp_presub4_reg),
      .a_inp5(mode6_outp_presub5_reg),
      .a_inp6(mode6_outp_presub6_reg),
      .a_inp7(mode6_outp_presub7_reg),
      .b_inp(mode5_outp_log_reg),
      .outp0(mode6_outp_logsub0),
      .outp1(mode6_outp_logsub1),
      .outp2(mode6_outp_logsub2),
      .outp3(mode6_outp_logsub3),
      .outp4(mode6_outp_logsub4),
      .outp5(mode6_outp_logsub5),
      .outp6(mode6_outp_logsub6),
      .outp7(mode6_outp_logsub7)
  );
  always @(posedge clk) begin
    if (reset) begin
      mode6_outp_logsub0_reg <= 0;
      mode6_outp_logsub1_reg <= 0;
      mode6_outp_logsub2_reg <= 0;
      mode6_outp_logsub3_reg <= 0;
      mode6_outp_logsub4_reg <= 0;
      mode6_outp_logsub5_reg <= 0;
      mode6_outp_logsub6_reg <= 0;
      mode6_outp_logsub7_reg <= 0;
    end else if (mode6_run) begin
      mode6_outp_logsub0_reg <= mode6_outp_logsub0;
      mode6_outp_logsub1_reg <= mode6_outp_logsub1;
      mode6_outp_logsub2_reg <= mode6_outp_logsub2;
      mode6_outp_logsub3_reg <= mode6_outp_logsub3;
      mode6_outp_logsub4_reg <= mode6_outp_logsub4;
      mode6_outp_logsub5_reg <= mode6_outp_logsub5;
      mode6_outp_logsub6_reg <= mode6_outp_logsub6;
      mode6_outp_logsub7_reg <= mode6_outp_logsub7;
    end
  end

  //////------mode7 exp---------///////
  wire [`DATAWIDTH-1:0] outp0_temp;
  wire [`DATAWIDTH-1:0] outp1_temp;
  wire [`DATAWIDTH-1:0] outp2_temp;
  wire [`DATAWIDTH-1:0] outp3_temp;
  wire [`DATAWIDTH-1:0] outp4_temp;
  wire [`DATAWIDTH-1:0] outp5_temp;
  wire [`DATAWIDTH-1:0] outp6_temp;
  wire [`DATAWIDTH-1:0] outp7_temp;
  reg [`DATAWIDTH-1:0] outp0;
  reg [`DATAWIDTH-1:0] outp1;
  reg [`DATAWIDTH-1:0] outp2;
  reg [`DATAWIDTH-1:0] outp3;
  reg [`DATAWIDTH-1:0] outp4;
  reg [`DATAWIDTH-1:0] outp5;
  reg [`DATAWIDTH-1:0] outp6;
  reg [`DATAWIDTH-1:0] outp7;

  mode7_exp mode7_exp(
      .inp0(mode6_outp_logsub0_reg),
      .inp1(mode6_outp_logsub1_reg),
      .inp2(mode6_outp_logsub2_reg),
      .inp3(mode6_outp_logsub3_reg),
      .inp4(mode6_outp_logsub4_reg),
      .inp5(mode6_outp_logsub5_reg),
      .inp6(mode6_outp_logsub6_reg),
      .inp7(mode6_outp_logsub7_reg),

      .clk(clk),
      .reset(reset),
      .stage_run(mode7_stage_run),
      .stage_run2(mode7_stage_run2),

      .outp0(outp0_temp),
      .outp1(outp1_temp),
      .outp2(outp2_temp),
      .outp3(outp3_temp),
      .outp4(outp4_temp),
      .outp5(outp5_temp),
      .outp6(outp6_temp),
      .outp7(outp7_temp)
  );
  always @(posedge clk) begin
    if (reset) begin
      outp0 <= 0;
      outp1 <= 0;
      outp2 <= 0;
      outp3 <= 0;
      outp4 <= 0;
      outp5 <= 0;
      outp6 <= 0;
      outp7 <= 0;
    end else if (mode7_run) begin
      outp0 <= outp0_temp;
      outp1 <= outp1_temp;
      outp2 <= outp2_temp;
      outp3 <= outp3_temp;
      outp4 <= outp4_temp;
      outp5 <= outp5_temp;
      outp6 <= outp6_temp;
      outp7 <= outp7_temp;
    end
  end

endmodule


module mode1_max_tree(
  inp0, 
  inp1, 
  inp2, 
  inp3, 
  inp4, 
  inp5, 
  inp6, 
  inp7, 
  mode1_stage3_run,
  mode1_stage2_run,
  mode1_stage1_run,  
  mode1_stage0_run,
  clk,
  reset,
  outp,
);

  input  [`DATAWIDTH-1 : 0] inp0; 
  input  [`DATAWIDTH-1 : 0] inp1; 
  input  [`DATAWIDTH-1 : 0] inp2; 
  input  [`DATAWIDTH-1 : 0] inp3; 
  input  [`DATAWIDTH-1 : 0] inp4; 
  input  [`DATAWIDTH-1 : 0] inp5; 
  input  [`DATAWIDTH-1 : 0] inp6; 
  input  [`DATAWIDTH-1 : 0] inp7; 
  input mode1_stage3_run;
  input mode1_stage2_run;
  input mode1_stage1_run;
  input mode1_stage0_run;
  input clk;
  input reset;
  output [`DATAWIDTH-1 : 0] outp;

  reg    [`DATAWIDTH-1 : 0] outp;

  reg    [`DATAWIDTH-1 : 0] cmp0_out_stage3_reg;
  wire   [`DATAWIDTH-1 : 0] cmp0_out_stage3;
  reg    [`DATAWIDTH-1 : 0] cmp1_out_stage3_reg;
  wire   [`DATAWIDTH-1 : 0] cmp1_out_stage3;
  reg    [`DATAWIDTH-1 : 0] cmp2_out_stage3_reg;
  wire   [`DATAWIDTH-1 : 0] cmp2_out_stage3;
  reg    [`DATAWIDTH-1 : 0] cmp3_out_stage3_reg;
  wire   [`DATAWIDTH-1 : 0] cmp3_out_stage3;
  wire   [`DATAWIDTH-1 : 0] cmp0_out_stage2;
  wire   [`DATAWIDTH-1 : 0] cmp1_out_stage2;
  wire   [`DATAWIDTH-1 : 0] cmp0_out_stage1;
  wire   [`DATAWIDTH-1 : 0] cmp0_out_stage0;
  reg    [`DATAWIDTH-1 : 0] cmp1_out_stage2_reg;
  reg    [`DATAWIDTH-1 : 0] cmp0_out_stage2_reg;
  reg    [`DATAWIDTH-1 : 0] cmp0_out_stage1_reg;


  always @(posedge clk) begin
    if (reset) begin
      outp <= 0;
      cmp0_out_stage3_reg <= 0;
      cmp1_out_stage3_reg <= 0;
      cmp2_out_stage3_reg <= 0;
      cmp3_out_stage3_reg <= 0;
	  cmp0_out_stage2_reg <= 0;
      cmp1_out_stage2_reg <= 0;
	  cmp0_out_stage1_reg <= 0;
    end

    else if(~reset && mode1_stage3_run) begin
      cmp0_out_stage3_reg <= cmp0_out_stage3;
      cmp1_out_stage3_reg <= cmp1_out_stage3;
      cmp2_out_stage3_reg <= cmp2_out_stage3;
      cmp3_out_stage3_reg <= cmp3_out_stage3;
    end
	
	else if(~reset && mode1_stage2_run) begin
      cmp0_out_stage2_reg <= cmp0_out_stage2;
      cmp1_out_stage2_reg <= cmp1_out_stage2;
    end
	
	else if(~reset && mode1_stage1_run) begin
      cmp0_out_stage1_reg <= cmp0_out_stage1;
    end

    else if(~reset && mode1_stage0_run) begin
      outp <= cmp0_out_stage0;
    end

  end

wire cmp0_stage3_aeb;
wire cmp0_stage3_aneb;
wire cmp0_stage3_alb;
wire cmp0_stage3_aleb;
wire cmp0_stage3_agb;
wire cmp0_stage3_ageb;
wire cmp0_stage3_unordered;

wire cmp1_stage3_aeb;
wire cmp1_stage3_aneb;
wire cmp1_stage3_alb;
wire cmp1_stage3_aleb;
wire cmp1_stage3_agb;
wire cmp1_stage3_ageb;
wire cmp1_stage3_unordered;

wire cmp2_stage3_aeb;
wire cmp2_stage3_aneb;
wire cmp2_stage3_alb;
wire cmp2_stage3_aleb;
wire cmp2_stage3_agb;
wire cmp2_stage3_ageb;
wire cmp2_stage3_unordered;

wire cmp3_stage3_aeb;
wire cmp3_stage3_aneb;
wire cmp3_stage3_alb;
wire cmp3_stage3_aleb;
wire cmp3_stage3_agb;
wire cmp3_stage3_ageb;
wire cmp3_stage3_unordered;

wire cmp0_stage2_aeb;
wire cmp0_stage2_aneb;
wire cmp0_stage2_alb;
wire cmp0_stage2_aleb;
wire cmp0_stage2_agb;
wire cmp0_stage2_ageb;
wire cmp0_stage2_unordered;

wire cmp1_stage2_aeb;
wire cmp1_stage2_aneb;
wire cmp1_stage2_alb;
wire cmp1_stage2_aleb;
wire cmp1_stage2_agb;
wire cmp1_stage2_ageb;
wire cmp1_stage2_unordered;

wire cmp0_stage1_aeb;
wire cmp0_stage1_aneb;
wire cmp0_stage1_alb;
wire cmp0_stage1_aleb;
wire cmp0_stage1_agb;
wire cmp0_stage1_ageb;
wire cmp0_stage1_unordered;

wire cmp0_stage0_aeb;
wire cmp0_stage0_aneb;
wire cmp0_stage0_alb;
wire cmp0_stage0_aleb;
wire cmp0_stage0_agb;
wire cmp0_stage0_ageb;
wire cmp0_stage0_unordered;

//Stage3
comparator cmp0_stage3(.a(inp0),       .b(inp1),        .aeb(cmp0_stage3_aeb), .aneb(cmp0_stage3_aneb), .alb(cmp0_stage3_alb), .aleb(cmp0_stage3_aleb), .agb(cmp0_stage3_agb), .ageb(cmp0_stage3_ageb), .unordered(cmp0_stage3_unordered));
assign cmp0_out_stage3 = (cmp0_stage3_ageb==1'b1) ? inp0 : inp1;

comparator cmp1_stage3(.a(inp2),       .b(inp3),         .aeb(cmp1_stage3_aeb), .aneb(cmp1_stage3_aneb), .alb(cmp1_stage3_alb), .aleb(cmp1_stage3_aleb), .agb(cmp1_stage3_agb), .ageb(cmp1_stage3_ageb), .unordered(cmp1_stage3_unordered));   
assign cmp1_out_stage3 = (cmp1_stage3_ageb==1'b1) ? inp2 : inp3;

comparator cmp2_stage3(.a(inp4),       .b(inp5),         .aeb(cmp2_stage3_aeb), .aneb(cmp2_stage3_aneb), .alb(cmp2_stage3_alb), .aleb(cmp2_stage3_aleb), .agb(cmp2_stage3_agb), .ageb(cmp2_stage3_ageb), .unordered(cmp2_stage3_unordered));   
assign cmp2_out_stage3 = (cmp2_stage3_ageb==1'b1) ? inp4 : inp5;

comparator cmp3_stage3(.a(inp6),       .b(inp7),         .aeb(cmp3_stage3_aeb), .aneb(cmp3_stage3_aneb), .alb(cmp3_stage3_alb), .aleb(cmp3_stage3_aleb), .agb(cmp3_stage3_agb), .ageb(cmp3_stage3_ageb), .unordered(cmp3_stage3_unordered));   
assign cmp3_out_stage3 = (cmp3_stage3_ageb==1'b1) ? inp6 : inp7;

//Stage2
comparator cmp0_stage2(.a(cmp0_out_stage3_reg),       .b(cmp1_out_stage3_reg),        .aeb(cmp0_stage2_aeb), .aneb(cmp0_stage2_aneb), .alb(cmp0_stage2_alb), .aleb(cmp0_stage2_aleb), .agb(cmp0_stage2_agb), .ageb(cmp0_stage2_ageb), .unordered(cmp0_stage2_unordered));
assign cmp0_out_stage2 = (cmp0_stage2_ageb==1'b1) ? cmp0_out_stage3_reg : cmp1_out_stage3_reg;

comparator cmp1_stage2(.a(cmp2_out_stage3_reg),       .b(cmp3_out_stage3_reg),         .aeb(cmp1_stage2_aeb), .aneb(cmp1_stage2_aneb), .alb(cmp1_stage2_alb), .aleb(cmp1_stage2_aleb), .agb(cmp1_stage2_agb), .ageb(cmp1_stage2_ageb), .unordered(cmp1_stage2_unordered));   
assign cmp1_out_stage2 = (cmp1_stage2_ageb==1'b1) ? cmp2_out_stage3_reg : cmp3_out_stage3_reg;

//Stage1
comparator cmp0_stage1(.a(cmp0_out_stage2_reg),       .b(cmp1_out_stage2_reg),          .aeb(cmp0_stage1_aeb), .aneb(cmp0_stage1_aneb), .alb(cmp0_stage1_alb), .aleb(cmp0_stage1_aleb), .agb(cmp0_stage1_agb), .ageb(cmp0_stage1_ageb), .unordered(cmp0_stage1_unordered));
assign cmp0_out_stage1 = (cmp0_stage1_ageb==1'b1) ? cmp0_out_stage2_reg: cmp1_out_stage2_reg;

//Stage0
comparator cmp0_stage0(.a(outp),       .b(cmp0_out_stage1_reg),         .aeb(cmp0_stage0_aeb), .aneb(cmp0_stage0_aneb), .alb(cmp0_stage0_alb), .aleb(cmp0_stage0_aleb), .agb(cmp0_stage0_agb), .ageb(cmp0_stage0_ageb), .unordered(cmp0_stage0_unordered));
assign cmp0_out_stage0 = (cmp0_stage0_ageb==1'b1) ? outp : cmp0_out_stage1_reg;

//DW_fp_cmp #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) cmp0_stage3(.a(inp0),       .b(inp1),      .z1(cmp0_out_stage3), .zctr(1'b0), .aeqb(), .altb(), .agtb(), .unordered(), .z0(), .status0(), .status1());
//DW_fp_cmp #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) cmp1_stage3(.a(inp2),       .b(inp3),      .z1(cmp1_out_stage3), .zctr(1'b0), .aeqb(), .altb(), .agtb(), .unordered(), .z0(), .status0(), .status1());
//DW_fp_cmp #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) cmp2_stage3(.a(inp4),       .b(inp5),      .z1(cmp2_out_stage3), .zctr(1'b0), .aeqb(), .altb(), .agtb(), .unordered(), .z0(), .status0(), .status1());
//DW_fp_cmp #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) cmp3_stage3(.a(inp6),       .b(inp7),      .z1(cmp3_out_stage3), .zctr(1'b0), .aeqb(), .altb(), .agtb(), .unordered(), .z0(), .status0(), .status1());
//
//DW_fp_cmp #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) cmp0_stage2(.a(cmp0_out_stage3_reg),       .b(cmp1_out_stage3_reg),      .z1(cmp0_out_stage2), .zctr(1'b0), .aeqb(), .altb(), .agtb(), .unordered(), .z0(), .status0(), .status1());
//DW_fp_cmp #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) cmp1_stage2(.a(cmp2_out_stage3_reg),       .b(cmp3_out_stage3_reg),      .z1(cmp1_out_stage2), .zctr(1'b0), .aeqb(), .altb(), .agtb(), .unordered(), .z0(), .status0(), .status1());
//
//DW_fp_cmp #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) cmp0_stage1(.a(cmp0_out_stage2),       .b(cmp1_out_stage2),      .z1(cmp0_out_stage1), .zctr(1'b0), .aeqb(), .altb(), .agtb(), .unordered(), .z0(), .status0(), .status1());
//
//DW_fp_cmp #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) cmp0_stage0(.a(outp),       .b(cmp0_out_stage1),      .z1(cmp0_out_stage0), .zctr(1'b0), .aeqb(), .altb(), .agtb(), .unordered(), .z0(), .status0(), .status1());

endmodule


module mode2_sub(
  clk,
  rst,
  a_inp0,
  a_inp1,
  a_inp2,
  a_inp3,
  a_inp4,
  a_inp5,
  a_inp6,
  a_inp7,
  outp0,
  outp1,
  outp2,
  outp3,
  outp4,
  outp5,
  outp6,
  outp7,
  b_inp,
);
  
  input clk;
  input rst;
  input  [`DATAWIDTH-1 : 0] a_inp0;
  input  [`DATAWIDTH-1 : 0] a_inp1;
  input  [`DATAWIDTH-1 : 0] a_inp2;
  input  [`DATAWIDTH-1 : 0] a_inp3;
  input  [`DATAWIDTH-1 : 0] a_inp4;
  input  [`DATAWIDTH-1 : 0] a_inp5;
  input  [`DATAWIDTH-1 : 0] a_inp6;
  input  [`DATAWIDTH-1 : 0] a_inp7;
  output  [`DATAWIDTH-1 : 0] outp0;
  output  [`DATAWIDTH-1 : 0] outp1;
  output  [`DATAWIDTH-1 : 0] outp2;
  output  [`DATAWIDTH-1 : 0] outp3;
  output  [`DATAWIDTH-1 : 0] outp4;
  output  [`DATAWIDTH-1 : 0] outp5;
  output  [`DATAWIDTH-1 : 0] outp6;
  output  [`DATAWIDTH-1 : 0] outp7;
  input  [`DATAWIDTH-1 : 0] b_inp;

  wire clk_NC;
  wire rst_NC;
  wire [4:0] flags_NC0, flags_NC1, flags_NC2, flags_NC3;
  wire [4:0] flags_NC4, flags_NC5, flags_NC6, flags_NC7;

  // 0 add, 1 sub
  FPAddSub sub0(.clk(clk), .rst(rst), .a(a_inp0),	.b(b_inp), .operation(1'b1),	.result(outp0), .flags(flags_NC0));
  FPAddSub sub1(.clk(clk), .rst(rst), .a(a_inp1),	.b(b_inp), .operation(1'b1),	.result(outp1), .flags(flags_NC1));
  FPAddSub sub2(.clk(clk), .rst(rst), .a(a_inp2),	.b(b_inp), .operation(1'b1),	.result(outp2), .flags(flags_NC2));
  FPAddSub sub3(.clk(clk), .rst(rst), .a(a_inp3),	.b(b_inp), .operation(1'b1),	.result(outp3), .flags(flags_NC3));
  FPAddSub sub4(.clk(clk), .rst(rst), .a(a_inp4),	.b(b_inp), .operation(1'b1),	.result(outp4), .flags(flags_NC4));
  FPAddSub sub5(.clk(clk), .rst(rst), .a(a_inp5),	.b(b_inp), .operation(1'b1),	.result(outp5), .flags(flags_NC5));
  FPAddSub sub6(.clk(clk), .rst(rst), .a(a_inp6),	.b(b_inp), .operation(1'b1),	.result(outp6), .flags(flags_NC6));
  FPAddSub sub7(.clk(clk), .rst(rst), .a(a_inp7),	.b(b_inp), .operation(1'b1),	.result(outp7), .flags(flags_NC7));

//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub0(.a(a_inp0), .b(b_inp), .z(outp0), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub1(.a(a_inp1), .b(b_inp), .z(outp1), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub2(.a(a_inp2), .b(b_inp), .z(outp2), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub3(.a(a_inp3), .b(b_inp), .z(outp3), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub4(.a(a_inp4), .b(b_inp), .z(outp4), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub5(.a(a_inp5), .b(b_inp), .z(outp5), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub6(.a(a_inp6), .b(b_inp), .z(outp6), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub7(.a(a_inp7), .b(b_inp), .z(outp7), .rnd(3'b000), .status());
endmodule


module mode3_exp(
  inp0, 
  inp1, 
  inp2, 
  inp3, 
  inp4, 
  inp5, 
  inp6, 
  inp7, 

  clk,
  reset,
  stage_run,
  stage_run2,
  
  outp0, 
  outp1, 
  outp2, 
  outp3, 
  outp4, 
  outp5, 
  outp6, 
  outp7
);

  input  [`DATAWIDTH-1 : 0] inp0;
  input  [`DATAWIDTH-1 : 0] inp1;
  input  [`DATAWIDTH-1 : 0] inp2;
  input  [`DATAWIDTH-1 : 0] inp3;
  input  [`DATAWIDTH-1 : 0] inp4;
  input  [`DATAWIDTH-1 : 0] inp5;
  input  [`DATAWIDTH-1 : 0] inp6;
  input  [`DATAWIDTH-1 : 0] inp7;

  input  clk;
  input  reset;
  input  stage_run;
  input  stage_run2; 
  
  output  [`DATAWIDTH-1 : 0] outp0;
  output  [`DATAWIDTH-1 : 0] outp1;
  output  [`DATAWIDTH-1 : 0] outp2;
  output  [`DATAWIDTH-1 : 0] outp3;
  output  [`DATAWIDTH-1 : 0] outp4;
  output  [`DATAWIDTH-1 : 0] outp5;
  output  [`DATAWIDTH-1 : 0] outp6;
  output  [`DATAWIDTH-1 : 0] outp7;
  expunit exp0(.a(inp0), .z(outp0), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp1(.a(inp1), .z(outp1), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp2(.a(inp2), .z(outp2), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp3(.a(inp3), .z(outp3), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp4(.a(inp4), .z(outp4), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp5(.a(inp5), .z(outp5), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp6(.a(inp6), .z(outp6), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp7(.a(inp7), .z(outp7), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
endmodule


module mode4_adder_tree(
  inp0, 
  inp1, 
  inp2, 
  inp3, 
  inp4, 
  inp5, 
  inp6, 
  inp7, 
  mode4_stage0_run,
  mode4_stage1_run,
  mode4_stage2_run,
  mode4_stage3_run,

  clk,
  reset,
  outp
);

  input clk;
  input reset;
  input  [`DATAWIDTH-1 : 0] inp0; 
  input  [`DATAWIDTH-1 : 0] inp1; 
  input  [`DATAWIDTH-1 : 0] inp2; 
  input  [`DATAWIDTH-1 : 0] inp3; 
  input  [`DATAWIDTH-1 : 0] inp4; 
  input  [`DATAWIDTH-1 : 0] inp5; 
  input  [`DATAWIDTH-1 : 0] inp6; 
  input  [`DATAWIDTH-1 : 0] inp7; 
  output [`DATAWIDTH-1 : 0] outp;
  input mode4_stage0_run;
  input mode4_stage1_run;
  input mode4_stage2_run;
  input mode4_stage3_run;

  wire   [`DATAWIDTH-1 : 0] add0_out_stage3;
  reg    [`DATAWIDTH-1 : 0] add0_out_stage3_reg;
  wire   [`DATAWIDTH-1 : 0] add1_out_stage3;
  reg    [`DATAWIDTH-1 : 0] add1_out_stage3_reg;
  wire   [`DATAWIDTH-1 : 0] add2_out_stage3;
  reg    [`DATAWIDTH-1 : 0] add2_out_stage3_reg;
  wire   [`DATAWIDTH-1 : 0] add3_out_stage3;
  reg    [`DATAWIDTH-1 : 0] add3_out_stage3_reg;

  wire   [`DATAWIDTH-1 : 0] add0_out_stage2;
  reg    [`DATAWIDTH-1 : 0] add0_out_stage2_reg;
  wire   [`DATAWIDTH-1 : 0] add1_out_stage2;
  reg    [`DATAWIDTH-1 : 0] add1_out_stage2_reg;

  wire   [`DATAWIDTH-1 : 0] add0_out_stage1;
  reg    [`DATAWIDTH-1 : 0] add0_out_stage1_reg;

  wire   [`DATAWIDTH-1 : 0] add0_out_stage0;
  reg    [`DATAWIDTH-1 : 0] outp;

//  always @(posedge clk) begin
//    if (reset) begin
//      outp <= 0;
//      add0_out_stage3_reg <= 0;
//      add1_out_stage3_reg <= 0;
//      add2_out_stage3_reg <= 0;
//      add3_out_stage3_reg <= 0;
//      add0_out_stage2_reg <= 0;
//      add1_out_stage2_reg <= 0;
//      add0_out_stage1_reg <= 0;
//    end
//
//    if(~reset && mode4_stage3_run) begin
//      add0_out_stage3_reg <= add0_out_stage3;
//      add1_out_stage3_reg <= add1_out_stage3;
//      add2_out_stage3_reg <= add2_out_stage3;
//      add3_out_stage3_reg <= add3_out_stage3;
//    end
//
//    if(~reset && mode4_stage2_run) begin
//      add0_out_stage2_reg <= add0_out_stage2;
//      add1_out_stage2_reg <= add1_out_stage2;
//    end
//
//    if(~reset && mode4_stage1_run) begin
//      add0_out_stage1_reg <= add0_out_stage1;
//    end
//
//    if(~reset && mode4_stage0_run) begin
//      outp <= add0_out_stage0;
//    end
//
//  end


always @ (posedge clk) begin
	if(~reset && mode4_stage3_run) begin
      add0_out_stage3_reg <= add0_out_stage3;
      add1_out_stage3_reg <= add1_out_stage3;
      add2_out_stage3_reg <= add2_out_stage3;
      add3_out_stage3_reg <= add3_out_stage3;
    end
	
	else if (reset) begin
		add0_out_stage3_reg <= 0;
		add1_out_stage3_reg <= 0;
		add2_out_stage3_reg <= 0;
		add3_out_stage3_reg <= 0;
	end
end		

always @ (posedge clk) begin
	if(~reset && mode4_stage2_run) begin
      add0_out_stage2_reg <= add0_out_stage2;
      add1_out_stage2_reg <= add1_out_stage2;
    end
	
	else if (reset) begin
		add0_out_stage2_reg <= 0;
		add1_out_stage2_reg <= 0;
	end
end		

always @ (posedge clk) begin
	if(~reset && mode4_stage1_run) begin
      add0_out_stage1_reg <= add0_out_stage1;
    end
	else if (reset) begin
	add0_out_stage1_reg <= 0;
	end
end

always @ (posedge clk) begin
	if(~reset && mode4_stage0_run) begin
		outp <= add0_out_stage0;
    end
	else if (reset) begin
		outp <= 0;
	end
end	

  wire clk_NC;
  wire rst_NC;
  wire [4:0] flags_NC0, flags_NC1, flags_NC2, flags_NC3;
  wire [4:0] flags_NC4, flags_NC5, flags_NC6, flags_NC7;

  // 0 add, 1 sub
  FPAddSub add0_stage3(.clk(clk), .rst(reset), .a(inp0),	.b(inp1), .operation(1'b0),	.result(add0_out_stage3), .flags(flags_NC0));
  FPAddSub add1_stage3(.clk(clk), .rst(reset), .a(inp2),	.b(inp3), .operation(1'b0),	.result(add1_out_stage3), .flags(flags_NC1));
  FPAddSub add2_stage3(.clk(clk), .rst(reset), .a(inp4),	.b(inp5), .operation(1'b0),	.result(add2_out_stage3), .flags(flags_NC2));
  FPAddSub add3_stage3(.clk(clk), .rst(reset), .a(inp6),	.b(inp7), .operation(1'b0),	.result(add3_out_stage3), .flags(flags_NC3));

  FPAddSub add0_stage2(.clk(clk), .rst(reset), .a(add0_out_stage3_reg),	.b(add1_out_stage3_reg), .operation(1'b0),	.result(add0_out_stage2), .flags(flags_NC4));
  FPAddSub add1_stage2(.clk(clk), .rst(reset), .a(add2_out_stage3_reg),	.b(add3_out_stage3_reg), .operation(1'b0),	.result(add1_out_stage2), .flags(flags_NC5));

  FPAddSub add0_stage1(.clk(clk), .rst(reset), .a(add0_out_stage2_reg),	.b(add1_out_stage2_reg), .operation(1'b0),	.result(add0_out_stage1), .flags(flags_NC6));

  FPAddSub add0_stage0(.clk(clk), .rst(reset), .a(outp),	.b(add0_out_stage1_reg), .operation(1'b0),	.result(add0_out_stage0), .flags(flags_NC7));


//  DW_fp_add #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) add0_stage3(.a(inp0),       .b(inp1),      .z(add0_out_stage3), .rnd(3'b000),    .status());
//  DW_fp_add #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) add1_stage3(.a(inp2),       .b(inp3),      .z(add1_out_stage3), .rnd(3'b000),    .status());
//  DW_fp_add #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) add2_stage3(.a(inp4),       .b(inp5),      .z(add2_out_stage3), .rnd(3'b000),    .status());
//  DW_fp_add #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) add3_stage3(.a(inp6),       .b(inp7),      .z(add3_out_stage3), .rnd(3'b000),    .status());
//
//  DW_fp_add #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) add0_stage2(.a(add0_out_stage3_reg),       .b(add1_out_stage3_reg),      .z(add0_out_stage2), .rnd(3'b000),    .status());
//  DW_fp_add #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) add1_stage2(.a(add2_out_stage3_reg),       .b(add3_out_stage3_reg),      .z(add1_out_stage2), .rnd(3'b000),    .status());
//
//  DW_fp_add #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) add0_stage1(.a(add0_out_stage2_reg),       .b(add1_out_stage2_reg),      .z(add0_out_stage1), .rnd(3'b000),    .status());
//
//  DW_fp_add #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) add0_stage0(.a(outp),       .b(add0_out_stage1_reg),      .z(add0_out_stage0), .rnd(3'b000),    .status());

endmodule


module mode5_ln(
clk,
rst,
inp,
outp
);
  input clk;
  input rst; 
  input  [`DATAWIDTH-1 : 0] inp;
  output [`DATAWIDTH-1 : 0] outp;
  logunit ln(.clk(clk), .rst(rst), .a(inp), .z(outp), .status());
endmodule


module mode6_sub(
  clk,
  rst,
  a_inp0,
  a_inp1,
  a_inp2,
  a_inp3,
  a_inp4,
  a_inp5,
  a_inp6,
  a_inp7,
  b_inp,
  outp0,
  outp1,
  outp2,
  outp3,
  outp4,
  outp5,
  outp6,
  outp7
);
  
  input clk;
  input rst;
  input  [`DATAWIDTH-1 : 0] a_inp0;
  input  [`DATAWIDTH-1 : 0] a_inp1;
  input  [`DATAWIDTH-1 : 0] a_inp2;
  input  [`DATAWIDTH-1 : 0] a_inp3;
  input  [`DATAWIDTH-1 : 0] a_inp4;
  input  [`DATAWIDTH-1 : 0] a_inp5;
  input  [`DATAWIDTH-1 : 0] a_inp6;
  input  [`DATAWIDTH-1 : 0] a_inp7;
  input  [`DATAWIDTH-1 : 0] b_inp;
  output  [`DATAWIDTH-1 : 0] outp0;
  output  [`DATAWIDTH-1 : 0] outp1;
  output  [`DATAWIDTH-1 : 0] outp2;
  output  [`DATAWIDTH-1 : 0] outp3;
  output  [`DATAWIDTH-1 : 0] outp4;
  output  [`DATAWIDTH-1 : 0] outp5;
  output  [`DATAWIDTH-1 : 0] outp6;
  output  [`DATAWIDTH-1 : 0] outp7;

  wire clk_NC;
  wire rst_NC;
  wire [4:0] flags_NC0, flags_NC1, flags_NC2, flags_NC3;
  wire [4:0] flags_NC4, flags_NC5, flags_NC6, flags_NC7;

  // 0 add, 1 sub
  FPAddSub sub0(.clk(clk), .rst(rst), .a(a_inp0),	.b(b_inp), .operation(1'b1),	.result(outp0), .flags(flags_NC0));
  FPAddSub sub1(.clk(clk), .rst(rst), .a(a_inp1),	.b(b_inp), .operation(1'b1),	.result(outp1), .flags(flags_NC1));
  FPAddSub sub2(.clk(clk), .rst(rst), .a(a_inp2),	.b(b_inp), .operation(1'b1),	.result(outp2), .flags(flags_NC2));
  FPAddSub sub3(.clk(clk), .rst(rst), .a(a_inp3),	.b(b_inp), .operation(1'b1),	.result(outp3), .flags(flags_NC3));
  FPAddSub sub4(.clk(clk), .rst(rst), .a(a_inp4),	.b(b_inp), .operation(1'b1),	.result(outp4), .flags(flags_NC4));
  FPAddSub sub5(.clk(clk), .rst(rst), .a(a_inp5),	.b(b_inp), .operation(1'b1),	.result(outp5), .flags(flags_NC5));
  FPAddSub sub6(.clk(clk), .rst(rst), .a(a_inp6),	.b(b_inp), .operation(1'b1),	.result(outp6), .flags(flags_NC6));
  FPAddSub sub7(.clk(clk), .rst(rst), .a(a_inp7),	.b(b_inp), .operation(1'b1),	.result(outp7), .flags(flags_NC7));
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub0(.a(a_inp0), .b(b_inp), .z(outp0), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub1(.a(a_inp1), .b(b_inp), .z(outp1), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub2(.a(a_inp2), .b(b_inp), .z(outp2), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub3(.a(a_inp3), .b(b_inp), .z(outp3), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub4(.a(a_inp4), .b(b_inp), .z(outp4), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub5(.a(a_inp5), .b(b_inp), .z(outp5), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub6(.a(a_inp6), .b(b_inp), .z(outp6), .rnd(3'b000), .status());
//  DW_fp_sub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) sub7(.a(a_inp7), .b(b_inp), .z(outp7), .rnd(3'b000), .status());
endmodule


module mode7_exp(
  inp0, 
  inp1, 
  inp2, 
  inp3, 
  inp4, 
  inp5, 
  inp6, 
  inp7, 

  clk,
  reset,
  stage_run,
  stage_run2,
  
  outp0, 
  outp1, 
  outp2, 
  outp3, 
  outp4, 
  outp5, 
  outp6, 
  outp7
);

  input  [`DATAWIDTH-1 : 0] inp0;
  input  [`DATAWIDTH-1 : 0] inp1;
  input  [`DATAWIDTH-1 : 0] inp2;
  input  [`DATAWIDTH-1 : 0] inp3;
  input  [`DATAWIDTH-1 : 0] inp4;
  input  [`DATAWIDTH-1 : 0] inp5;
  input  [`DATAWIDTH-1 : 0] inp6;
  input  [`DATAWIDTH-1 : 0] inp7;

  input  clk;
  input  reset;
  input  stage_run;
  input  stage_run2;
  
  output  [`DATAWIDTH-1 : 0] outp0;
  output  [`DATAWIDTH-1 : 0] outp1;
  output  [`DATAWIDTH-1 : 0] outp2;
  output  [`DATAWIDTH-1 : 0] outp3;
  output  [`DATAWIDTH-1 : 0] outp4;
  output  [`DATAWIDTH-1 : 0] outp5;
  output  [`DATAWIDTH-1 : 0] outp6;
  output  [`DATAWIDTH-1 : 0] outp7;
  expunit exp0(.a(inp0), .z(outp0), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp1(.a(inp1), .z(outp1), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp2(.a(inp2), .z(outp2), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp3(.a(inp3), .z(outp3), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp4(.a(inp4), .z(outp4), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp5(.a(inp5), .z(outp5), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp6(.a(inp6), .z(outp6), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
  expunit exp7(.a(inp7), .z(outp7), .status(), .stage_run(stage_run), .stage_run2(stage_run2), .clk(clk), .reset(reset));
endmodule

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Definition of a 16-bit floating point adder/subtractor
// This is a heavily modified version of:
// https://github.com/fbrosser/DSP48E1-FP/tree/master/src/FP_AddSub
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

`define EXPONENT 5
`define MANTISSA 10
`define ACTUAL_MANTISSA 11
`define EXPONENT_LSB 10
`define EXPONENT_MSB 14
`define MANTISSA_LSB 0
`define MANTISSA_MSB 9
`define MANTISSA_MUL_SPLIT_LSB 3
`define MANTISSA_MUL_SPLIT_MSB 9
`define SIGN 1
`define SIGN_LOC 15
`define DWIDTH (`SIGN+`EXPONENT+`MANTISSA)
`define IEEE_COMPLIANCE 1


module FPAddSub(
		clk,
		rst,
		a,
		b,
		operation,			// 0 add, 1 sub
		result,
		flags
	);
	
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
	
	assign flags = 5'b0;
	
`ifdef complex_dsp
adder_fp_clk u_add(.clk(clk), .a(a), .b(b),.out(result));
`else
FPAddSub_16 u_FPAddSub (.clk(clk), .rst(rst), .a(a), .b(b), .operation(1'b0), .result(result), .flags());
`endif
endmodule


//============================================================================
// Comparator module
//============================================================================

//============================================================================
//
//This Verilog source file is part of the Berkeley HardFloat IEEE Floating-Point
//Arithmetic Package, Release 1, by John R. Hauser.
//
//Copyright 2019 The Regents of the University of California.  All rights
//reserved.
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions, and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions, and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the University nor the names of its contributors may
//    be used to endorse or promote products derived from this software without
//    specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS "AS IS", AND ANY
//EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE
//DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
//DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================

//=============================================================================
// Modified by Aman Arora and Pragnesh Patel
//=============================================================================


module comparator(
a,
b,
aeb,
aneb,
alb,
aleb,
agb,
ageb,
unordered
);
//Default for FP16
//parameter exp = 5;
//parameter man = 10;

input [15:0] a;
input [15:0] b;
output aeb;
output aneb;
output alb;
output aleb;
output agb;
output ageb;
output unordered;

wire [16:0] a_RecFN;
wire [16:0] b_RecFN;

//Convert to Recoded representation
fNToRecFN#(5,11) convert_a(.in(a), .out(a_RecFN));  
fNToRecFN#(5,11) convert_b(.in(b), .out(b_RecFN));  

wire [4:0] except_flags;
wire less_than;
wire equal;
wire greater_than;

wire signaling;
assign signaling = 1'b0;

//Actual conversion module
compareRecFN_Fp16 compare(
.a(a_RecFN),
.b(b_RecFN),
.signaling(signaling),
.lt(less_than),
.eq(equal),
.gt(greater_than),
.unordered(unordered),
.exceptionFlags(except_flags)
);


//Result flags
assign aeb = equal;
assign aneb = ~equal;
assign alb = less_than;
assign aleb = less_than | equal;
assign agb = greater_than;
assign ageb = greater_than | equal;

endmodule



module reverseFp16 (input [9:0] in, output [9:0] out);

   assign out[0] = in[9];
   assign out[1] = in[8];
   assign out[2] = in[7];
   assign out[3] = in[6];
   assign out[4] = in[5];
   assign out[5] = in[4];
   assign out[6] = in[3];
   assign out[7] = in[2];
   assign out[8] = in[1];
   assign out[9] = in[0];
   // genvar ix;
   // generate
   //     for (ix = 0; ix < width; ix = ix + 1) begin :Bit
   //         assign out[ix] = in[width - 1 - ix];
   //     end
   // endgenerate

endmodule

module fNToRecFN#(parameter expWidth = 3, parameter sigWidth = 3) (
        input [(expWidth + sigWidth - 1):0] in,
        output [(expWidth + sigWidth):0] out
    );

    //`include "HardFloat_localFuncs.vi"
    //localparam normDistWidth = clog2(sigWidth);
    localparam normDistWidth = 4; //Hardcoding for FP16

    wire sign;
    wire [(expWidth - 1):0] expIn;
    wire [(sigWidth - 2):0] fractIn;
    assign {sign, expIn, fractIn} = in;
    wire isZeroExpIn = (expIn == 0);
    wire isZeroFractIn = (fractIn == 0);
    
   
    wire [(normDistWidth - 1):0] normDist;
    //sigwidth = 11, normDistWidth=4
    countLeadingZerosfp16 #(sigWidth - 1, normDistWidth)  countLeadingZeros(.in(fractIn), .count(normDist)); 
    wire [(sigWidth - 2):0] subnormFract = (fractIn<<normDist)<<1;
    wire [expWidth:0] adjustedExp =
        (isZeroExpIn ? normDist ^ ((1<<(expWidth + 1)) - 1) : expIn)
            + ((1<<(expWidth - 1)) | (isZeroExpIn ? 2 : 1));
    wire isZero = isZeroExpIn && isZeroFractIn;
    wire isSpecial = (adjustedExp[expWidth:(expWidth - 1)] == 'b11);
    
   
    wire [expWidth:0] exp;
    assign exp[expWidth:(expWidth - 2)] =
        isSpecial ? {2'b11, !isZeroFractIn}
            : isZero ? 3'b000 : adjustedExp[expWidth:(expWidth - 2)];
    assign exp[(expWidth - 3):0] = adjustedExp;
    assign out = {sign, exp, isZeroExpIn ? subnormFract : fractIn};

endmodule


module compareRecFN_Fp16(
  a,
  b,
  signaling,
  lt,
  eq,
  gt,
  unordered,
  exceptionFlags
  );
  //parameter expWidth = 5;
  //parameter sigWidth = 11;
  
  input [(5 + 11):0] a;
  input [(5 + 11):0] b;
  input signaling;
  output lt;
  output eq;
  output gt;
  output unordered;
  output [4:0] exceptionFlags;
   

    wire isNaNA, isInfA, isZeroA, signA;
    wire [(5 + 1):0] sExpA;
    wire [11:0] sigA;
    recFNToRawFN#(5, 11)  recFNToRawFN_a(
      .in(a), 
      .isNaN(isNaNA), 
      .isInf(isInfA), 
      .isZero(isZeroA), 
      .sign(signA), 
      .sExp(sExpA), 
      .sig(sigA)
      );
    wire isSigNaNA;
    isSigNaNRecFN#(5, 11) isSigNaN_a(.in(a), .isSigNaN(isSigNaNA));
    wire isNaNB, isInfB, isZeroB, signB;
    wire [(5 + 1):0] sExpB;
    wire [11:0] sigB;
    recFNToRawFN#(5, 11) recFNToRawFN_b(
      .in(b), 
      .isNaN(isNaNB), 
      .isInf(isInfB), 
      .isZero(isZeroB), 
      .sign(signB), 
      .sExp(sExpB), 
      .sig(sigB)
      );
    wire isSigNaNB;
    isSigNaNRecFN#(5, 11) isSigNaN_b(.in(b), .isSigNaN(isSigNaNB));
   
    wire ordered = !isNaNA && !isNaNB;
    wire bothInfs  = isInfA  && isInfB;
    wire bothZeros = isZeroA && isZeroB;
    wire eqExps = (sExpA == sExpB);
    wire common_ltMags = (sExpA < sExpB) || (eqExps && (sigA < sigB));
    wire common_eqMags = eqExps && (sigA == sigB);
    wire ordered_lt =
        !bothZeros
            && ((signA && !signB)
                    || (!bothInfs
                            && ((signA && !common_ltMags && !common_eqMags)
                                    || (!signB && common_ltMags))));
    wire ordered_eq =
        bothZeros || ((signA == signB) && (bothInfs || common_eqMags));
    
    
    wire invalid = isSigNaNA || isSigNaNB || (signaling && !ordered);
    assign lt = ordered && ordered_lt;
    assign eq = ordered && ordered_eq;
    assign gt = ordered && !ordered_lt && !ordered_eq;
    assign unordered = !ordered;
    assign exceptionFlags = {invalid, 4'b0};

endmodule



module recFNToRawFN(
  in,
  isNaN,
  isInf,
  isZero,
  sign,
  sExp,
  sig
  );
  
  parameter expWidth = 3;
  parameter sigWidth = 3;
  
  input [(expWidth + sigWidth):0] in;
  output isNaN;
  output isInf;
  output isZero;
  output sign;
  output [(expWidth + 1):0] sExp;
  output [sigWidth:0] sig;

    
    wire [expWidth:0] exp;
    wire [(sigWidth - 2):0] fract;
    assign {sign, exp, fract} = in;
    wire isSpecial = (exp>>(expWidth - 1) == 'b11);
    
    
    assign isNaN = isSpecial &&  exp[expWidth - 2];
    assign isInf = isSpecial && !exp[expWidth - 2];
    assign isZero = (exp>>(expWidth - 2) == 'b000);
    assign sExp = exp;
    assign sig = {1'b0, !isZero, fract};

endmodule



module  isSigNaNRecFN(in, isSigNaN);

  parameter expWidth = 3;
  parameter sigWidth = 3;
  
  input [(expWidth + sigWidth):0] in;
  output isSigNaN;

    wire isNaN =
        (in[(expWidth + sigWidth - 1):(expWidth + sigWidth - 3)] == 'b111);
    assign isSigNaN = isNaN && !in[sigWidth - 2];

endmodule

module countLeadingZerosfp16 #(parameter inWidth = 10, parameter countWidth = 4) (
    input [(inWidth - 1):0] in, output reg [(countWidth - 1):0] count
    );

  wire [(inWidth - 1):0] reverseIn;
  reverseFp16 reverse_in(in, reverseIn);
  wire [inWidth:0] oneLeastReverseIn =
      {1'b1, reverseIn} & ({1'b0, ~reverseIn} + 1);
		
  always@(*)
    begin
      if (oneLeastReverseIn[10] == 1)
        begin
          count = 4'd10;
        end
      else if (oneLeastReverseIn[9] == 1)
        begin
          count = 4'd9;
        end
      else if (oneLeastReverseIn[8] == 1)
        begin
          count= 4'd8;
        end
      else if (oneLeastReverseIn[7] == 1)
        begin
          count = 4'd7;
        end
      else if (oneLeastReverseIn[6] == 1)
        begin
          count = 4'd6;
        end
      else if (oneLeastReverseIn[5] == 1)
        begin
          count = 4'd5;
        end
      else if (oneLeastReverseIn[4] == 1)
        begin
          count = 4'd4;
        end
      else if (oneLeastReverseIn[3] == 1)
        begin
          count = 4'd3;
        end
      else if (oneLeastReverseIn[2] == 1)
        begin
          count = 4'd2;
        end
      else if (oneLeastReverseIn[1] == 1)
        begin
          count = 4'd1;
        end
      else
        begin
          count = 4'd0;
        end
      end

endmodule


//////////////////////////////////////////////////////
// Log unit
//////////////////////////////////////////////////////

module logunit (clk, rst, a, z, status);

	input clk;
  input rst;
	input [15:0] a;
	output [15:0] z;
	output [4:0] status;

	wire [15: 0] fxout1;
	wire [15: 0] fxout2;


	LUT1 lut1 (.addr(a[14:10]),.log(fxout1)); 
	LUT2 lut2 (.addr(a[9:4]),.log(fxout2));  

  wire clk_NC;
  wire rst_NC;

	//DW_fp_addsub #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) add(.a(fxout1), .b(fxout2), .rnd(3'b0), .op(1'b0), .z(z), .status(status[7:0]));
  FPAddSub add (.clk(clk), .rst(rst), .a(fxout1),	.b(fxout2), .operation(1'b1),	.result(z), .flags());
endmodule

module LUT1(addr, log);
    input [4:0] addr;
    output reg [15:0] log;

    always @(addr) begin
        case (addr)
			5'b0 		: log = 16'b1111110000000000;
			5'b1 		: log = 16'b1100100011011010;
			5'b10 		: log = 16'b1100100010000001;
			5'b11 		: log = 16'b1100100000101001;
			5'b100 		: log = 16'b1100011110100000;
			5'b101 		: log = 16'b1100011011101110;
			5'b110 		: log = 16'b1100011000111101;
			5'b111 		: log = 16'b1100010110001100;
			5'b1000 		: log = 16'b1100010011011010;
			5'b1001 		: log = 16'b1100010000101001;
			5'b1010 		: log = 16'b1100001011101110;
			5'b1011 		: log = 16'b1100000110001100;
			5'b1100 		: log = 16'b1100000000101001;
			5'b1101 		: log = 16'b1011110110001100;
			5'b1110 		: log = 16'b1011100110001100;
			5'b1111 		: log = 16'b0000000000000000;
			5'b10000 		: log = 16'b0011100110001100;
			5'b10001 		: log = 16'b0011110110001100;
			5'b10010 		: log = 16'b0100000000101001;
			5'b10011 		: log = 16'b0100000110001100;
			5'b10100 		: log = 16'b0100001011101110;
			5'b10101 		: log = 16'b0100010000101001;
			5'b10110 		: log = 16'b0100010011011010;
			5'b10111 		: log = 16'b0100010110001100;
			5'b11000 		: log = 16'b0100011000111101;
			5'b11001 		: log = 16'b0100011011101110;
			5'b11010 		: log = 16'b0100011110100000;
			5'b11011 		: log = 16'b0100100000101001;
			5'b11100 		: log = 16'b0100100010000001;
			5'b11101 		: log = 16'b0100100011011010;
			5'b11110 		: log = 16'b0100100100110011;
			5'b11111 		: log = 16'b0111110000000000;
        endcase
    end
endmodule

module LUT2(addr, log);
    input [5:0] addr;
    output reg [15:0] log;

    always @(addr) begin
        case (addr)
			6'b0 		: log = 16'b0000000000000000;
			6'b1 		: log = 16'b0010001111110000;
			6'b10 		: log = 16'b0010011111100001;
			6'b11 		: log = 16'b0010100111011101;
			6'b100 		: log = 16'b0010101111000011;
			6'b101 		: log = 16'b0010110011010000;
			6'b110 		: log = 16'b0010110110111100;
			6'b111 		: log = 16'b0010111010100101;
			6'b1000 		: log = 16'b0010111110001010;
			6'b1001 		: log = 16'b0011000000110110;
			6'b1010 		: log = 16'b0011000010100101;
			6'b1011 		: log = 16'b0011000100010011;
			6'b1100 		: log = 16'b0011000110000000;
			6'b1101 		: log = 16'b0011000111101011;
			6'b1110 		: log = 16'b0011001001010101;
			6'b1111 		: log = 16'b0011001010111101;
			6'b10000 		: log = 16'b0011001100100100;
			6'b10001 		: log = 16'b0011001110001010;
			6'b10010 		: log = 16'b0011001111101110;
			6'b10011 		: log = 16'b0011010000101001;
			6'b10100 		: log = 16'b0011010001011010;
			6'b10101 		: log = 16'b0011010010001010;
			6'b10110 		: log = 16'b0011010010111010;
			6'b10111 		: log = 16'b0011010011101010;
			6'b11000 		: log = 16'b0011010100011000;
			6'b11001 		: log = 16'b0011010101000111;
			6'b11010 		: log = 16'b0011010101110100;
			6'b11011 		: log = 16'b0011010110100010;
			6'b11100 		: log = 16'b0011010111001110;
			6'b11101 		: log = 16'b0011010111111011;
			6'b11110 		: log = 16'b0011011000100111;
			6'b11111 		: log = 16'b0011011001010010;
			6'b100000 		: log = 16'b0011011001111101;
			6'b100001 		: log = 16'b0011011010100111;
			6'b100010 		: log = 16'b0011011011010001;
			6'b100011 		: log = 16'b0011011011111011;
			6'b100100 		: log = 16'b0011011100100100;
			6'b100101 		: log = 16'b0011011101001101;
			6'b100110 		: log = 16'b0011011101110101;
			6'b100111 		: log = 16'b0011011110011101;
			6'b101000 		: log = 16'b0011011111000101;
			6'b101001 		: log = 16'b0011011111101100;
			6'b101010 		: log = 16'b0011100000001001;
			6'b101011 		: log = 16'b0011100000011101;
			6'b101100 		: log = 16'b0011100000110000;
			6'b101101 		: log = 16'b0011100001000010;
			6'b101110 		: log = 16'b0011100001010101;
			6'b101111 		: log = 16'b0011100001101000;
			6'b110000 		: log = 16'b0011100001111010;
			6'b110001 		: log = 16'b0011100010001100;
			6'b110010 		: log = 16'b0011100010011110;
			6'b110011 		: log = 16'b0011100010110000;
			6'b110100 		: log = 16'b0011100011000010;
			6'b110101 		: log = 16'b0011100011010100;
			6'b110110 		: log = 16'b0011100011100101;
			6'b110111 		: log = 16'b0011100011110110;
			6'b111000 		: log = 16'b0011100100000111;
			6'b111001 		: log = 16'b0011100100011000;
			6'b111010 		: log = 16'b0011100100101001;
			6'b111011 		: log = 16'b0011100100111010;
			6'b111100 		: log = 16'b0011100101001011;
			6'b111101 		: log = 16'b0011100101011011;
			6'b111110 		: log = 16'b0011100101101011;
			6'b111111 		: log = 16'b0011100101111100;
        endcase
    end
endmodule


//////////////////////////////////////////////////////
// Exponential unit
// Author: Pragnesh Patel
//////////////////////////////////////////////////////

module expunit (a, z, status, stage_run, stage_run2, clk, reset);

  parameter int_width = 3; // fixed point integer length
  parameter frac_width = 3; // fixed point fraction length
  	
  input [15:0] a;
  input stage_run2;
  input stage_run;
  input clk;
  input reset;
  output [15:0] z;
  output [7:0] status;
  
  wire [int_width + frac_width - 1: 0] fxout;
  wire [31:0] LUTout;
  reg  [31:0] LUTout_reg;
  reg  [31:0] LUTout_reg2;
  wire [15:0] Mult_out;
  reg  [15:0] Mult_out_reg;
  reg  [15:0] a_reg;
  
  
  always @(posedge clk) begin
    if(reset) begin
      Mult_out_reg <= 0;
      LUTout_reg <= 0;
	  LUTout_reg2 <= 0;
	  a_reg <= 0;
    end
    else if(stage_run2) begin
      LUTout_reg2 <= LUTout;
	  a_reg <= a;
    end	
	else if(stage_run) begin
      Mult_out_reg <= Mult_out;
      LUTout_reg <= LUTout_reg2;
    end
  end

  wire clk_NC, rst_NC;
        
  fptofixed_para fpfx (.fp(a), .fx(fxout));
  LUT lut(.addr(fxout[int_width + frac_width - 1 : 0]), .exp(LUTout)); 
  //DW_fp_mult #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) fpmult (.a(a), .b(LUTout[31:16]), .rnd(3'b000), .z(Mult_out), .status());
  FPMult fpmult (.clk(clk), .rst(rst), .a(a_reg), .b(LUTout_reg2[31:16]), .result(Mult_out), .flags());
  //DW_fp_add #(`MANTISSA, `EXPONENT, `IEEE_COMPLIANCE) fpsub (.a(Mult_out_reg), .b(LUTout_reg[15:0]), .rnd(3'b000), .z(z), .status(status[7:0]));
  FPAddSub fpsub (.clk(clk), .rst(rst), .a(Mult_out_reg),	.b(LUTout_reg[15:0]), .operation(1'b0),	.result(z), .flags());
endmodule

module fptofixed_para (
	fp,
	fx
	);
	
	parameter int_width = 3; // fixed point integer length
	parameter frac_width = 3; // fixed point fraction length

	input [15:0] fp; // Half Precision fp
	output [int_width + frac_width - 1:0] fx;  
	
	wire [15:0] Mant; // mantissa of fp
	wire [4:0] Ea; // non biased exponent
	wire [4:0] Exp; // biased exponent
	reg [15:0] sftfx; // output of shifter block
	reg [15:0] temp;
	
	assign Mant = {6'b000001, fp[9:0]};
	assign Exp = fp[14:10];
	assign Ea = Exp - 15;

	assign fx = temp[9+int_width:10-frac_width];
	

always @(sftfx)
begin
// only negetive numbers as inputs after sorting and subtraction from max
	if (Ea > int_width - 1)
		begin
			temp <= 16'hFFFF; // if there is an overflow
			
		end
	else if ( fp[14:0] == 15'b0)
		begin 
			temp <= 16'b0;
			
		end	
	else // underflow automatically becomes zero
		begin
			temp <= sftfx;
		end
end	

//DW01_ash  #(`DATAWIDTH, 5) ash( .A(Mant[15:0]), .DATA_TC(1'b0), .SH(Ea[4:0]), .SH_TC(1'b1), .B(sftfx));
reg shift_direction;
reg [3:0] shift_magnitude;
always @(*) begin
  shift_direction = Ea[4];  //if this bit is 1, that means Ea was a negative number, which means right shift. if this bit is 0, that means left shift.
  shift_magnitude = (Ea[4] ? ((~Ea[3:0]) + 1) : Ea[3:0]);  //take 2's complement to find the magnitude if negative
  sftfx = (shift_direction ? (Mant[15:0] >> shift_magnitude) : (Mant[15:0] << shift_magnitude)); //perform the actual shift
  //Mant[15:0] is unsigned, so no need to handle sign bit explicitly. zero padding is good on both direction shifts for unsigned numbers.
end

endmodule

module LUT(addr, exp);
    input [5:0] addr;
    output reg [31:0] exp;

    always @(addr) begin
        case (addr)
			6'b0 		: exp = 32'b00111011110000010011110000000000;
			6'b1 		: exp = 32'b00111010110110000011101111101010;
			6'b10 		: exp = 32'b00111010000010100011101110111110;
			6'b11 		: exp = 32'b00111001010101000011101101111111;
			6'b100 		: exp = 32'b00111000101101000011101100110100;
			6'b101 		: exp = 32'b00111000001001110011101011100000;
			6'b110 		: exp = 32'b00110111010101000011101010000111;
			6'b111 		: exp = 32'b00110110011101110011101000101010;
			6'b1000 		: exp = 32'b00110101101101010011100111001100;
			6'b1001 		: exp = 32'b00110101000010010011100101101110;
			6'b1010 		: exp = 32'b00110100011100100011100100010010;
			6'b1011 		: exp = 32'b00110011110110000011100010111000;
			6'b1100 		: exp = 32'b00110010111011000011100001100001;
			6'b1101 		: exp = 32'b00110010000111000011100000001111;
			6'b1110 		: exp = 32'b00110001011001000011011101111111;
			6'b1111 		: exp = 32'b00110000110000100011011011101010;
			6'b10000 		: exp = 32'b00110000001100110011011001011101;
			6'b10001 		: exp = 32'b00101111011010010011010111011001;
			6'b10010 		: exp = 32'b00101110100010100011010101011101;
			6'b10011 		: exp = 32'b00101101110001010011010011101010;
			6'b10100 		: exp = 32'b00101101000110000011010001111111;
			6'b10101 		: exp = 32'b00101100011111110011010000011100;
			6'b10110 		: exp = 32'b00101011111011110011001110000000;
			6'b10111 		: exp = 32'b00101011000000000011001011010110;
			6'b11000 		: exp = 32'b00101010001011010011001000111010;
			6'b11001 		: exp = 32'b00101001011101000011000110101010;
			6'b11010 		: exp = 32'b00101000110100000011000100100110;
			6'b11011 		: exp = 32'b00101000001111110011000010101101;
			6'b11100 		: exp = 32'b00100111011111100011000000111111;
			6'b11101 		: exp = 32'b00100110100111010010111110110011;
			6'b11110 		: exp = 32'b00100101110101100010111011111010;
			6'b11111 		: exp = 32'b00100101001001110010111001010001;
			6'b100000 		: exp = 32'b00100100100011000010110110111000;
			6'b100001 		: exp = 32'b00100100000000110010110100101100;
			6'b100010 		: exp = 32'b00100011000101000010110010101101;
			6'b100011 		: exp = 32'b00100010001111110010110000111001;
			6'b100100 		: exp = 32'b00100001100001000010101110100000;
			6'b100101 		: exp = 32'b00100000110111100010101011100010;
			6'b100110 		: exp = 32'b00100000010010110010101000110101;
			6'b100111 		: exp = 32'b00011111100101000010100110011001;
			6'b101000 		: exp = 32'b00011110101100000010100100001011;
			6'b101001 		: exp = 32'b00011101111001110010100010001011;
			6'b101010 		: exp = 32'b00011101001101010010100000010111;
			6'b101011 		: exp = 32'b00011100100110010010011101011101;
			6'b101100 		: exp = 32'b00011100000011110010011010100000;
			6'b101101 		: exp = 32'b00011011001010010010010111110101;
			6'b101110 		: exp = 32'b00011010010100100010010101011011;
			6'b101111 		: exp = 32'b00011001100101000010010011010000;
			6'b110000 		: exp = 32'b00011000111011000010010001010011;
			6'b110001 		: exp = 32'b00011000010110000010001111000101;
			6'b110010 		: exp = 32'b00010111101010100010001011111010;
			6'b110011 		: exp = 32'b00010110110001000010001001000011;
			6'b110100 		: exp = 32'b00010101111110000010000110011111;
			6'b110101 		: exp = 32'b00010101010001010010000100001011;
			6'b110110 		: exp = 32'b00010100101001100010000010000110;
			6'b110111 		: exp = 32'b00010100000110100010000000001110;
			6'b111000 		: exp = 32'b00010011001111100001111101000101;
			6'b111001 		: exp = 32'b00010010011001000001111010000100;
			6'b111010 		: exp = 32'b00010001101001000001110111010111;
			6'b111011 		: exp = 32'b00010000111110100001110100111011;
			6'b111100 		: exp = 32'b00010000011001000001110010101111;
			6'b111101 		: exp = 32'b00001111110000010001110000110010;
			6'b111110 		: exp = 32'b00001110110101110001101110000010;
			6'b111111 		: exp = 32'b00001110000010100001101010111001;
        endcase
    end
endmodule


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Floating point 16-bit multiplier
// This is a heavily modified version of:
// https://github.com/fbrosser/DSP48E1-FP/tree/master/src/FPMult
// Original author: Fredrik Brosser
// Abridged by: Samidh Mehta
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

`define EXPONENT 5
`define MANTISSA 10
`define ACTUAL_MANTISSA 11
`define EXPONENT_LSB 10
`define EXPONENT_MSB 14
`define MANTISSA_LSB 0
`define MANTISSA_MSB 9
`define MANTISSA_MUL_SPLIT_LSB 3
`define MANTISSA_MUL_SPLIT_MSB 9
`define SIGN 1
`define SIGN_LOC 15
`define DWIDTH (`SIGN+`EXPONENT+`MANTISSA)
`define IEEE_COMPLIANCE 1

module FPMult(
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
	input [`DWIDTH-1:0] a;					// Input A, a 32-bit floating point number
	input [`DWIDTH-1:0] b;					// Input B, a 32-bit floating point number
	
	// Output ports
	output [`DWIDTH-1:0] result ;					// Product, result of the operation, 32-bit FP number
	output [4:0] flags ;				// Flags indicating exceptions according to IEEE754
	assign flags = 5'b0;
`ifdef complex_dsp
	multiply_fp_clk u_mult_fp(.clk(clk), .a(a), .b(b), .out(result)); 
`else
FPMult_16 u_FPMult (.clk(clk), .rst(1'b0), .a(a), .b(b), .result(result), .flags());
`endif
	
		
endmodule
`ifndef complex_dsp

`define EXPONENT 5
`define MANTISSA 10
`define ACTUAL_MANTISSA 11
`define EXPONENT_LSB 10
`define EXPONENT_MSB 14
`define MANTISSA_LSB 0
`define MANTISSA_MSB 9
`define MANTISSA_MUL_SPLIT_LSB 3
`define MANTISSA_MUL_SPLIT_MSB 9
`define SIGN 1
`define SIGN_LOC 15
`define DWIDTH (`SIGN+`EXPONENT+`MANTISSA)
`define IEEE_COMPLIANCE 1

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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Definition of a 16-bit floating point adder/subtractor
// This is a heavily modified version of:
// https://github.com/fbrosser/DSP48E1-FP/tree/master/src/FP_AddSub
// Original author: Fredrik Brosser
// Abridged by: Samidh Mehta
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

module FPAddSub_16(
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

