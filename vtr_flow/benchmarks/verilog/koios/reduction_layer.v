//////////////////////////////////////////////////////////////////////////////
// Author: Aman Arora
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//Module to reduce multiple values (add, max, min) into one final result.
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// Numerics. We use fixed point format:
//  Most significant 8 bits represent integer part and Least significant 8 bits
//  represent fraction part
//  i.e. IIIIIIIIFFFFFFFF = IIIIIIII.FFFFFFFF
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// There are 128 inputs to the reduction unit. We use a tree structure to reduce the 128 values.
// It is assumed that the number of addressses supplied (end_addr - start_addr + 1) is a multiple
// of 128. If the real application needs to reduce a number of values that are not a multiple of
// 128, then the application must pad the values in the input BRAM appropriately
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// A user is expected to use the resetn signal everytime before starting a new reduction operation.
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// Accumulation is done in 20 bits (16 + log(16))
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// Each entry of the RAM contains `NUM_INPUTS (which is 128) values. So,
// 1 read of the RAM provides all the inputs required for going through
// the reduction unit once. 
//////////////////////////////////////////////////////////////////

`timescale 1ns/1ns
`define DWIDTH 16
`define LOGDWIDTH 4
`define AWIDTH 5
`define MEM_SIZE 2048
`define NUM_INPUTS 128

////////////////////////////////////////////
// Top module
////////////////////////////////////////////
module reduction_layer(
  input clk,
  input resetn, //resets the control logic and the processing elements
  input start, //indicates start of the reduction operation
  input  [`AWIDTH-1:0] start_addr,  //the starting address where the inputs are located (inclusive)
  input  [`AWIDTH-1:0] end_addr,    //the end address where the inputs are located (inclusive)
  input  [1:0] reduction_type, //can have 3 values: 0 (Add), 1 (Max), 2 (Min)
  input  bram_in_we, //flag used to load the RAM with inputs from outside
  input  [`DWIDTH-1:0] bram_in_wdata_ext, //port to load RAM with inputs from outside (ideally we'd have AXI or some similar interface to load the RAM through a DMA module)
  input  [`AWIDTH-1:0] bram_in_addr_ext, //port to load RAM with inputs from outside (ideally we'd have AXI or some similar interface to load the RAM through a DMA module)
  output [`DWIDTH-1:0] reduced_out, //output
  output reg done //output is valid when done is 1
);

reg [`AWIDTH-1:0] bram_in_addr;
wire [`AWIDTH-1:0] bram_in_addr_muxed;
wire [`NUM_INPUTS*`DWIDTH-1:0] bram_in_wdata;
wire [`NUM_INPUTS*`DWIDTH-1:0] bram_in_rdata;
wire [`DWIDTH+`LOGDWIDTH-1:0] reduced_out_unrounded;
wire [`DWIDTH-1:0] reduced_out_add;

////////////////////////////////////////////////////////////////
//Ideally the RAM would be loaded using an AXI interface or something similar through a DMA engine. 
//Here we've just exposed the write data and address bus to the top-level.
//Each data entry in the RAM is very wide (`NUM_INPUTS*`DWIDTH). That leads to lot of
//ports on the top-level, causing very long wires from RAM to/from IOs. To avoid this,
//we are just going to reduce the width of the port (to `DWIDTH) and just replicate
//that over tha actual data port of the BRAM `NUM_INPUTS times. This doesn't impact
//hardware/functionality of the core design.
assign bram_in_addr_muxed = bram_in_we ? bram_in_addr_ext : bram_in_addr;
assign bram_in_wdata = {`NUM_INPUTS{bram_in_wdata_ext}}; 

////////////////////////////////////////////////////////////////
//Input matrix data is stored in this RAM
//The design reads 16 elements in one clock
////////////////////////////////////////////////////////////////

spram in_data(
  .addr(bram_in_addr_muxed),
  .d(bram_in_wdata), 
  .we(bram_in_we), 
  .q(bram_in_rdata), 
  .clk(clk));


reg [3:0] state;
reg [3:0] count;
reg reset_reduction_unit;
	
////////////////////////////////////////////////////////////////
// Control logic
////////////////////////////////////////////////////////////////
	always @( posedge clk) begin
      if (resetn == 1'b0) begin
        state <= 4'b0000;
        count <= 0;
        done <= 0;
        reset_reduction_unit <= 1;
      end 
      else begin
        case (state)
        4'b0000: begin
          //Stay here until start becomes 1. Keep the processing
          //elements in reset because we don't need them yet.
          if (start == 1'b1) begin
            state <= 4'b0001;
          end 
          done <= 0;
          reset_reduction_unit <= 1;
        end
        
        4'b0001: begin
          state <= 4'b0010;                    
          //Set correct read address for the BRAM containing input values
          //so that the values are available in the next cycle 
          bram_in_addr <= start_addr;
          reset_reduction_unit <= 1;
        end      

        4'b0010: begin
          //During this state, the values for the address set in previous state
          //are available at the read-output of the BRAM.
          //Now let's lift the reset from the processing elements.
          reset_reduction_unit <= 0;
          //If we have reached the end condition (that is, we have 
          //read all the entries from start_addr to end_addr), let's
          //move to the next state (that state doesn't read the RAM any more)
          if (bram_in_addr == end_addr) begin
            //end the loop
            state <= 4'b1110;
          end
			    else begin
            //Increment BRAM addr, so we can fetch the next data in the next cycle
            bram_in_addr <= bram_in_addr + 1;
          end
        end

        4'b1110: begin     
          //The full operation ends after all the locations have been read
          //(and data fed to the reduction unit) and initiation interval
          //(latency) has passed. So, let's count for the initiation interval
          //and then assert done and go back to the initial state.
          if (count==5) begin
            state <= 4'b0000;
            count <= 0;
            done <= 1;
            reset_reduction_unit <= 1;
          end 
          else begin
            count <= count + 1;
          end
        end
      endcase  
	end 
  end


////////////////////////////////////////////////////////////////
// Let's instantiate the unit that actually performs the reduction
////////////////////////////////////////////////////////////////

reduction_unit ucu(
  .clk(clk),
  .reset(reset_reduction_unit),
  .inp0(bram_in_rdata[1*`DWIDTH-1:0*`DWIDTH]),
  .inp1(bram_in_rdata[2*`DWIDTH-1:1*`DWIDTH]),
  .inp2(bram_in_rdata[3*`DWIDTH-1:2*`DWIDTH]),
  .inp3(bram_in_rdata[4*`DWIDTH-1:3*`DWIDTH]),
  .inp4(bram_in_rdata[5*`DWIDTH-1:4*`DWIDTH]),
  .inp5(bram_in_rdata[6*`DWIDTH-1:5*`DWIDTH]),
  .inp6(bram_in_rdata[7*`DWIDTH-1:6*`DWIDTH]),
  .inp7(bram_in_rdata[8*`DWIDTH-1:7*`DWIDTH]),
  .inp8(bram_in_rdata[9*`DWIDTH-1:8*`DWIDTH]),
  .inp9(bram_in_rdata[10*`DWIDTH-1:9*`DWIDTH]),
  .inp10(bram_in_rdata[11*`DWIDTH-1:10*`DWIDTH]),
  .inp11(bram_in_rdata[12*`DWIDTH-1:11*`DWIDTH]),
  .inp12(bram_in_rdata[13*`DWIDTH-1:12*`DWIDTH]),
  .inp13(bram_in_rdata[14*`DWIDTH-1:13*`DWIDTH]),
  .inp14(bram_in_rdata[15*`DWIDTH-1:14*`DWIDTH]),
  .inp15(bram_in_rdata[16*`DWIDTH-1:15*`DWIDTH]),
  .inp16(bram_in_rdata[17*`DWIDTH-1:16*`DWIDTH]),
  .inp17(bram_in_rdata[18*`DWIDTH-1:17*`DWIDTH]),
  .inp18(bram_in_rdata[19*`DWIDTH-1:18*`DWIDTH]),
  .inp19(bram_in_rdata[20*`DWIDTH-1:19*`DWIDTH]),
  .inp20(bram_in_rdata[21*`DWIDTH-1:20*`DWIDTH]),
  .inp21(bram_in_rdata[22*`DWIDTH-1:21*`DWIDTH]),
  .inp22(bram_in_rdata[23*`DWIDTH-1:22*`DWIDTH]),
  .inp23(bram_in_rdata[24*`DWIDTH-1:23*`DWIDTH]),
  .inp24(bram_in_rdata[25*`DWIDTH-1:24*`DWIDTH]),
  .inp25(bram_in_rdata[26*`DWIDTH-1:25*`DWIDTH]),
  .inp26(bram_in_rdata[27*`DWIDTH-1:26*`DWIDTH]),
  .inp27(bram_in_rdata[28*`DWIDTH-1:27*`DWIDTH]),
  .inp28(bram_in_rdata[29*`DWIDTH-1:28*`DWIDTH]),
  .inp29(bram_in_rdata[30*`DWIDTH-1:29*`DWIDTH]),
  .inp30(bram_in_rdata[31*`DWIDTH-1:30*`DWIDTH]),
  .inp31(bram_in_rdata[32*`DWIDTH-1:31*`DWIDTH]),
  .inp32(bram_in_rdata[33*`DWIDTH-1:32*`DWIDTH]),
  .inp33(bram_in_rdata[34*`DWIDTH-1:33*`DWIDTH]),
  .inp34(bram_in_rdata[35*`DWIDTH-1:34*`DWIDTH]),
  .inp35(bram_in_rdata[36*`DWIDTH-1:35*`DWIDTH]),
  .inp36(bram_in_rdata[37*`DWIDTH-1:36*`DWIDTH]),
  .inp37(bram_in_rdata[38*`DWIDTH-1:37*`DWIDTH]),
  .inp38(bram_in_rdata[39*`DWIDTH-1:38*`DWIDTH]),
  .inp39(bram_in_rdata[40*`DWIDTH-1:39*`DWIDTH]),
  .inp40(bram_in_rdata[41*`DWIDTH-1:40*`DWIDTH]),
  .inp41(bram_in_rdata[42*`DWIDTH-1:41*`DWIDTH]),
  .inp42(bram_in_rdata[43*`DWIDTH-1:42*`DWIDTH]),
  .inp43(bram_in_rdata[44*`DWIDTH-1:43*`DWIDTH]),
  .inp44(bram_in_rdata[45*`DWIDTH-1:44*`DWIDTH]),
  .inp45(bram_in_rdata[46*`DWIDTH-1:45*`DWIDTH]),
  .inp46(bram_in_rdata[47*`DWIDTH-1:46*`DWIDTH]),
  .inp47(bram_in_rdata[48*`DWIDTH-1:47*`DWIDTH]),
  .inp48(bram_in_rdata[49*`DWIDTH-1:48*`DWIDTH]),
  .inp49(bram_in_rdata[50*`DWIDTH-1:49*`DWIDTH]),
  .inp50(bram_in_rdata[51*`DWIDTH-1:50*`DWIDTH]),
  .inp51(bram_in_rdata[52*`DWIDTH-1:51*`DWIDTH]),
  .inp52(bram_in_rdata[53*`DWIDTH-1:52*`DWIDTH]),
  .inp53(bram_in_rdata[54*`DWIDTH-1:53*`DWIDTH]),
  .inp54(bram_in_rdata[55*`DWIDTH-1:54*`DWIDTH]),
  .inp55(bram_in_rdata[56*`DWIDTH-1:55*`DWIDTH]),
  .inp56(bram_in_rdata[57*`DWIDTH-1:56*`DWIDTH]),
  .inp57(bram_in_rdata[58*`DWIDTH-1:57*`DWIDTH]),
  .inp58(bram_in_rdata[59*`DWIDTH-1:58*`DWIDTH]),
  .inp59(bram_in_rdata[60*`DWIDTH-1:59*`DWIDTH]),
  .inp60(bram_in_rdata[61*`DWIDTH-1:60*`DWIDTH]),
  .inp61(bram_in_rdata[62*`DWIDTH-1:61*`DWIDTH]),
  .inp62(bram_in_rdata[63*`DWIDTH-1:62*`DWIDTH]),
  .inp63(bram_in_rdata[64*`DWIDTH-1:63*`DWIDTH]),
  .inp64(bram_in_rdata[65*`DWIDTH-1:64*`DWIDTH]),
  .inp65(bram_in_rdata[66*`DWIDTH-1:65*`DWIDTH]),
  .inp66(bram_in_rdata[67*`DWIDTH-1:66*`DWIDTH]),
  .inp67(bram_in_rdata[68*`DWIDTH-1:67*`DWIDTH]),
  .inp68(bram_in_rdata[69*`DWIDTH-1:68*`DWIDTH]),
  .inp69(bram_in_rdata[70*`DWIDTH-1:69*`DWIDTH]),
  .inp70(bram_in_rdata[71*`DWIDTH-1:70*`DWIDTH]),
  .inp71(bram_in_rdata[72*`DWIDTH-1:71*`DWIDTH]),
  .inp72(bram_in_rdata[73*`DWIDTH-1:72*`DWIDTH]),
  .inp73(bram_in_rdata[74*`DWIDTH-1:73*`DWIDTH]),
  .inp74(bram_in_rdata[75*`DWIDTH-1:74*`DWIDTH]),
  .inp75(bram_in_rdata[76*`DWIDTH-1:75*`DWIDTH]),
  .inp76(bram_in_rdata[77*`DWIDTH-1:76*`DWIDTH]),
  .inp77(bram_in_rdata[78*`DWIDTH-1:77*`DWIDTH]),
  .inp78(bram_in_rdata[79*`DWIDTH-1:78*`DWIDTH]),
  .inp79(bram_in_rdata[80*`DWIDTH-1:79*`DWIDTH]),
  .inp80(bram_in_rdata[81*`DWIDTH-1:80*`DWIDTH]),
  .inp81(bram_in_rdata[82*`DWIDTH-1:81*`DWIDTH]),
  .inp82(bram_in_rdata[83*`DWIDTH-1:82*`DWIDTH]),
  .inp83(bram_in_rdata[84*`DWIDTH-1:83*`DWIDTH]),
  .inp84(bram_in_rdata[85*`DWIDTH-1:84*`DWIDTH]),
  .inp85(bram_in_rdata[86*`DWIDTH-1:85*`DWIDTH]),
  .inp86(bram_in_rdata[87*`DWIDTH-1:86*`DWIDTH]),
  .inp87(bram_in_rdata[88*`DWIDTH-1:87*`DWIDTH]),
  .inp88(bram_in_rdata[89*`DWIDTH-1:88*`DWIDTH]),
  .inp89(bram_in_rdata[90*`DWIDTH-1:89*`DWIDTH]),
  .inp90(bram_in_rdata[91*`DWIDTH-1:90*`DWIDTH]),
  .inp91(bram_in_rdata[92*`DWIDTH-1:91*`DWIDTH]),
  .inp92(bram_in_rdata[93*`DWIDTH-1:92*`DWIDTH]),
  .inp93(bram_in_rdata[94*`DWIDTH-1:93*`DWIDTH]),
  .inp94(bram_in_rdata[95*`DWIDTH-1:94*`DWIDTH]),
  .inp95(bram_in_rdata[96*`DWIDTH-1:95*`DWIDTH]),
  .inp96(bram_in_rdata[97*`DWIDTH-1:96*`DWIDTH]),
  .inp97(bram_in_rdata[98*`DWIDTH-1:97*`DWIDTH]),
  .inp98(bram_in_rdata[99*`DWIDTH-1:98*`DWIDTH]),
  .inp99(bram_in_rdata[100*`DWIDTH-1:99*`DWIDTH]),
  .inp100(bram_in_rdata[101*`DWIDTH-1:100*`DWIDTH]),
  .inp101(bram_in_rdata[102*`DWIDTH-1:101*`DWIDTH]),
  .inp102(bram_in_rdata[103*`DWIDTH-1:102*`DWIDTH]),
  .inp103(bram_in_rdata[104*`DWIDTH-1:103*`DWIDTH]),
  .inp104(bram_in_rdata[105*`DWIDTH-1:104*`DWIDTH]),
  .inp105(bram_in_rdata[106*`DWIDTH-1:105*`DWIDTH]),
  .inp106(bram_in_rdata[107*`DWIDTH-1:106*`DWIDTH]),
  .inp107(bram_in_rdata[108*`DWIDTH-1:107*`DWIDTH]),
  .inp108(bram_in_rdata[109*`DWIDTH-1:108*`DWIDTH]),
  .inp109(bram_in_rdata[110*`DWIDTH-1:109*`DWIDTH]),
  .inp110(bram_in_rdata[111*`DWIDTH-1:110*`DWIDTH]),
  .inp111(bram_in_rdata[112*`DWIDTH-1:111*`DWIDTH]),
  .inp112(bram_in_rdata[113*`DWIDTH-1:112*`DWIDTH]),
  .inp113(bram_in_rdata[114*`DWIDTH-1:113*`DWIDTH]),
  .inp114(bram_in_rdata[115*`DWIDTH-1:114*`DWIDTH]),
  .inp115(bram_in_rdata[116*`DWIDTH-1:115*`DWIDTH]),
  .inp116(bram_in_rdata[117*`DWIDTH-1:116*`DWIDTH]),
  .inp117(bram_in_rdata[118*`DWIDTH-1:117*`DWIDTH]),
  .inp118(bram_in_rdata[119*`DWIDTH-1:118*`DWIDTH]),
  .inp119(bram_in_rdata[120*`DWIDTH-1:119*`DWIDTH]),
  .inp120(bram_in_rdata[121*`DWIDTH-1:120*`DWIDTH]),
  .inp121(bram_in_rdata[122*`DWIDTH-1:121*`DWIDTH]),
  .inp122(bram_in_rdata[123*`DWIDTH-1:122*`DWIDTH]),
  .inp123(bram_in_rdata[124*`DWIDTH-1:123*`DWIDTH]),
  .inp124(bram_in_rdata[125*`DWIDTH-1:124*`DWIDTH]),
  .inp125(bram_in_rdata[126*`DWIDTH-1:125*`DWIDTH]),
  .inp126(bram_in_rdata[127*`DWIDTH-1:126*`DWIDTH]),
  .inp127(bram_in_rdata[128*`DWIDTH-1:127*`DWIDTH]),
  .mode(reduction_type),
  .outp(reduced_out_unrounded)
);

////////////////////////////////////////////////////////////////
// Rounding of the output of reduction unit (from 20 bits to 16 bits).
// This is required only when reduction type is "sum"
////////////////////////////////////////////////////////////////
rounding #(`DWIDTH+`LOGDWIDTH, `DWIDTH) u_round(.i_data(reduced_out_unrounded), .o_data(reduced_out_add));

assign reduced_out = (reduction_type==2'b0) ? reduced_out_add : reduced_out_unrounded[`DWIDTH-1:0];

endmodule


//////////////////////////////////
//Single port RAM. Stores the inputs.
//////////////////////////////////
module spram (
        addr, 
        d, 
        we, 
        q,  
        clk);

input [`AWIDTH-1:0] addr;
input [`NUM_INPUTS*`DWIDTH-1:0] d;
input we;
output reg [`NUM_INPUTS*`DWIDTH-1:0] q;
input clk;

`ifdef VCS

reg [`NUM_INPUTS*`DWIDTH-1:0] ram[((1<<`AWIDTH)-1):0];

always @(posedge clk)  
begin 
    if (we) 
      ram[addr] <= d;
    else 
      q <= ram[addr];
end

`else

single_port_ram u_single_port_ram(
.addr(addr),
.we(we),
.data(d),
.out(q),
.clk(clk)
);

`endif

endmodule


///////////////////////////////////////////////////////
// Reduction unit. It's a tree of processing elements.
// There are 128 inputs and one output and 8 stages. 
//
// The output is
// wider (more bits) than the inputs. It has logN more
// bits (if N is the number of bits in the inputs). This
// is based on https://zipcpu.com/dsp/2017/07/22/rounding.html.
// 
// The last stage is special. It adds the previous 
// result. This is useful when we have more than 128 inputs
// to reduce. We send the next set of 128 inputs in the next
// clock after the first set. 
// 
// Each stage of the tree is pipelined.
///////////////////////////////////////////////////////
module reduction_unit(
  clk,
  reset,
  inp0, 
  inp1, 
  inp2, 
  inp3, 
  inp4, 
  inp5, 
  inp6, 
  inp7, 
  inp8, 
  inp9, 
  inp10, 
  inp11, 
  inp12, 
  inp13, 
  inp14, 
  inp15, 
  inp16, 
  inp17, 
  inp18, 
  inp19, 
  inp20, 
  inp21, 
  inp22, 
  inp23, 
  inp24, 
  inp25, 
  inp26, 
  inp27, 
  inp28, 
  inp29, 
  inp30, 
  inp31, 
  inp32, 
  inp33, 
  inp34, 
  inp35, 
  inp36, 
  inp37, 
  inp38, 
  inp39, 
  inp40, 
  inp41, 
  inp42, 
  inp43, 
  inp44, 
  inp45, 
  inp46, 
  inp47, 
  inp48, 
  inp49, 
  inp50, 
  inp51, 
  inp52, 
  inp53, 
  inp54, 
  inp55, 
  inp56, 
  inp57, 
  inp58, 
  inp59, 
  inp60, 
  inp61, 
  inp62, 
  inp63, 
  inp64, 
  inp65, 
  inp66, 
  inp67, 
  inp68, 
  inp69, 
  inp70, 
  inp71, 
  inp72, 
  inp73, 
  inp74, 
  inp75, 
  inp76, 
  inp77, 
  inp78, 
  inp79, 
  inp80, 
  inp81, 
  inp82, 
  inp83, 
  inp84, 
  inp85, 
  inp86, 
  inp87, 
  inp88, 
  inp89, 
  inp90, 
  inp91, 
  inp92, 
  inp93, 
  inp94, 
  inp95, 
  inp96, 
  inp97, 
  inp98, 
  inp99, 
  inp100, 
  inp101, 
  inp102, 
  inp103, 
  inp104, 
  inp105, 
  inp106, 
  inp107, 
  inp108, 
  inp109, 
  inp110, 
  inp111, 
  inp112, 
  inp113, 
  inp114, 
  inp115, 
  inp116, 
  inp117, 
  inp118, 
  inp119, 
  inp120, 
  inp121, 
  inp122, 
  inp123, 
  inp124, 
  inp125, 
  inp126, 
  inp127, 

  mode,
  outp
);

  input clk;
  input reset;
  input  [`DWIDTH-1 : 0] inp0; 
  input  [`DWIDTH-1 : 0] inp1; 
  input  [`DWIDTH-1 : 0] inp2; 
  input  [`DWIDTH-1 : 0] inp3; 
  input  [`DWIDTH-1 : 0] inp4; 
  input  [`DWIDTH-1 : 0] inp5; 
  input  [`DWIDTH-1 : 0] inp6; 
  input  [`DWIDTH-1 : 0] inp7; 
  input  [`DWIDTH-1 : 0] inp8; 
  input  [`DWIDTH-1 : 0] inp9; 
  input  [`DWIDTH-1 : 0] inp10; 
  input  [`DWIDTH-1 : 0] inp11; 
  input  [`DWIDTH-1 : 0] inp12; 
  input  [`DWIDTH-1 : 0] inp13; 
  input  [`DWIDTH-1 : 0] inp14; 
  input  [`DWIDTH-1 : 0] inp15; 
  input  [`DWIDTH-1 : 0] inp16; 
  input  [`DWIDTH-1 : 0] inp17; 
  input  [`DWIDTH-1 : 0] inp18; 
  input  [`DWIDTH-1 : 0] inp19; 
  input  [`DWIDTH-1 : 0] inp20; 
  input  [`DWIDTH-1 : 0] inp21; 
  input  [`DWIDTH-1 : 0] inp22; 
  input  [`DWIDTH-1 : 0] inp23; 
  input  [`DWIDTH-1 : 0] inp24; 
  input  [`DWIDTH-1 : 0] inp25; 
  input  [`DWIDTH-1 : 0] inp26; 
  input  [`DWIDTH-1 : 0] inp27; 
  input  [`DWIDTH-1 : 0] inp28; 
  input  [`DWIDTH-1 : 0] inp29; 
  input  [`DWIDTH-1 : 0] inp30; 
  input  [`DWIDTH-1 : 0] inp31; 
  input  [`DWIDTH-1 : 0] inp32; 
  input  [`DWIDTH-1 : 0] inp33; 
  input  [`DWIDTH-1 : 0] inp34; 
  input  [`DWIDTH-1 : 0] inp35; 
  input  [`DWIDTH-1 : 0] inp36; 
  input  [`DWIDTH-1 : 0] inp37; 
  input  [`DWIDTH-1 : 0] inp38; 
  input  [`DWIDTH-1 : 0] inp39; 
  input  [`DWIDTH-1 : 0] inp40; 
  input  [`DWIDTH-1 : 0] inp41; 
  input  [`DWIDTH-1 : 0] inp42; 
  input  [`DWIDTH-1 : 0] inp43; 
  input  [`DWIDTH-1 : 0] inp44; 
  input  [`DWIDTH-1 : 0] inp45; 
  input  [`DWIDTH-1 : 0] inp46; 
  input  [`DWIDTH-1 : 0] inp47; 
  input  [`DWIDTH-1 : 0] inp48; 
  input  [`DWIDTH-1 : 0] inp49; 
  input  [`DWIDTH-1 : 0] inp50; 
  input  [`DWIDTH-1 : 0] inp51; 
  input  [`DWIDTH-1 : 0] inp52; 
  input  [`DWIDTH-1 : 0] inp53; 
  input  [`DWIDTH-1 : 0] inp54; 
  input  [`DWIDTH-1 : 0] inp55; 
  input  [`DWIDTH-1 : 0] inp56; 
  input  [`DWIDTH-1 : 0] inp57; 
  input  [`DWIDTH-1 : 0] inp58; 
  input  [`DWIDTH-1 : 0] inp59; 
  input  [`DWIDTH-1 : 0] inp60; 
  input  [`DWIDTH-1 : 0] inp61; 
  input  [`DWIDTH-1 : 0] inp62; 
  input  [`DWIDTH-1 : 0] inp63; 
  input  [`DWIDTH-1 : 0] inp64; 
  input  [`DWIDTH-1 : 0] inp65; 
  input  [`DWIDTH-1 : 0] inp66; 
  input  [`DWIDTH-1 : 0] inp67; 
  input  [`DWIDTH-1 : 0] inp68; 
  input  [`DWIDTH-1 : 0] inp69; 
  input  [`DWIDTH-1 : 0] inp70; 
  input  [`DWIDTH-1 : 0] inp71; 
  input  [`DWIDTH-1 : 0] inp72; 
  input  [`DWIDTH-1 : 0] inp73; 
  input  [`DWIDTH-1 : 0] inp74; 
  input  [`DWIDTH-1 : 0] inp75; 
  input  [`DWIDTH-1 : 0] inp76; 
  input  [`DWIDTH-1 : 0] inp77; 
  input  [`DWIDTH-1 : 0] inp78; 
  input  [`DWIDTH-1 : 0] inp79; 
  input  [`DWIDTH-1 : 0] inp80; 
  input  [`DWIDTH-1 : 0] inp81; 
  input  [`DWIDTH-1 : 0] inp82; 
  input  [`DWIDTH-1 : 0] inp83; 
  input  [`DWIDTH-1 : 0] inp84; 
  input  [`DWIDTH-1 : 0] inp85; 
  input  [`DWIDTH-1 : 0] inp86; 
  input  [`DWIDTH-1 : 0] inp87; 
  input  [`DWIDTH-1 : 0] inp88; 
  input  [`DWIDTH-1 : 0] inp89; 
  input  [`DWIDTH-1 : 0] inp90; 
  input  [`DWIDTH-1 : 0] inp91; 
  input  [`DWIDTH-1 : 0] inp92; 
  input  [`DWIDTH-1 : 0] inp93; 
  input  [`DWIDTH-1 : 0] inp94; 
  input  [`DWIDTH-1 : 0] inp95; 
  input  [`DWIDTH-1 : 0] inp96; 
  input  [`DWIDTH-1 : 0] inp97; 
  input  [`DWIDTH-1 : 0] inp98; 
  input  [`DWIDTH-1 : 0] inp99; 
  input  [`DWIDTH-1 : 0] inp100; 
  input  [`DWIDTH-1 : 0] inp101; 
  input  [`DWIDTH-1 : 0] inp102; 
  input  [`DWIDTH-1 : 0] inp103; 
  input  [`DWIDTH-1 : 0] inp104; 
  input  [`DWIDTH-1 : 0] inp105; 
  input  [`DWIDTH-1 : 0] inp106; 
  input  [`DWIDTH-1 : 0] inp107; 
  input  [`DWIDTH-1 : 0] inp108; 
  input  [`DWIDTH-1 : 0] inp109; 
  input  [`DWIDTH-1 : 0] inp110; 
  input  [`DWIDTH-1 : 0] inp111; 
  input  [`DWIDTH-1 : 0] inp112; 
  input  [`DWIDTH-1 : 0] inp113; 
  input  [`DWIDTH-1 : 0] inp114; 
  input  [`DWIDTH-1 : 0] inp115; 
  input  [`DWIDTH-1 : 0] inp116; 
  input  [`DWIDTH-1 : 0] inp117; 
  input  [`DWIDTH-1 : 0] inp118; 
  input  [`DWIDTH-1 : 0] inp119; 
  input  [`DWIDTH-1 : 0] inp120; 
  input  [`DWIDTH-1 : 0] inp121; 
  input  [`DWIDTH-1 : 0] inp122; 
  input  [`DWIDTH-1 : 0] inp123; 
  input  [`DWIDTH-1 : 0] inp124; 
  input  [`DWIDTH-1 : 0] inp125; 
  input  [`DWIDTH-1 : 0] inp126; 
  input  [`DWIDTH-1 : 0] inp127; 
  input [1:0] mode;
  output [`DWIDTH+`LOGDWIDTH-1 : 0] outp;

  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute0_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute0_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute1_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute1_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute2_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute2_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute3_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute3_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute4_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute4_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute5_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute5_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute6_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute6_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute7_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute7_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute8_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute8_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute9_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute9_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute10_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute10_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute11_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute11_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute12_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute12_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute13_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute13_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute14_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute14_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute15_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute15_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute16_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute16_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute17_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute17_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute18_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute18_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute19_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute19_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute20_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute20_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute21_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute21_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute22_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute22_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute23_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute23_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute24_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute24_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute25_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute25_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute26_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute26_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute27_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute27_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute28_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute28_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute29_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute29_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute30_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute30_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute31_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute31_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute32_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute32_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute33_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute33_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute34_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute34_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute35_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute35_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute36_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute36_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute37_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute37_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute38_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute38_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute39_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute39_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute40_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute40_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute41_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute41_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute42_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute42_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute43_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute43_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute44_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute44_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute45_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute45_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute46_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute46_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute47_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute47_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute48_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute48_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute49_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute49_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute50_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute50_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute51_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute51_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute52_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute52_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute53_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute53_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute54_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute54_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute55_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute55_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute56_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute56_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute57_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute57_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute58_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute58_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute59_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute59_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute60_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute60_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute61_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute61_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute62_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute62_out_stage7_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute63_out_stage7;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute63_out_stage7_reg;

  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute0_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute0_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute1_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute1_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute2_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute2_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute3_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute3_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute4_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute4_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute5_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute5_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute6_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute6_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute7_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute7_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute8_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute8_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute9_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute9_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute10_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute10_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute11_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute11_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute12_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute12_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute13_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute13_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute14_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute14_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute15_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute15_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute16_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute16_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute17_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute17_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute18_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute18_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute19_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute19_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute20_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute20_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute21_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute21_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute22_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute22_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute23_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute23_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute24_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute24_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute25_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute25_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute26_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute26_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute27_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute27_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute28_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute28_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute29_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute29_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute30_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute30_out_stage6_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute31_out_stage6;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute31_out_stage6_reg;

  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute0_out_stage5;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute0_out_stage5_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute1_out_stage5;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute1_out_stage5_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute2_out_stage5;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute2_out_stage5_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute3_out_stage5;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute3_out_stage5_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute4_out_stage5;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute4_out_stage5_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute5_out_stage5;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute5_out_stage5_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute6_out_stage5;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute6_out_stage5_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute7_out_stage5;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute7_out_stage5_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute8_out_stage5;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute8_out_stage5_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute9_out_stage5;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute9_out_stage5_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute10_out_stage5;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute10_out_stage5_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute11_out_stage5;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute11_out_stage5_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute12_out_stage5;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute12_out_stage5_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute13_out_stage5;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute13_out_stage5_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute14_out_stage5;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute14_out_stage5_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute15_out_stage5;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute15_out_stage5_reg;

  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute0_out_stage4;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute0_out_stage4_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute1_out_stage4;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute1_out_stage4_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute2_out_stage4;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute2_out_stage4_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute3_out_stage4;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute3_out_stage4_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute4_out_stage4;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute4_out_stage4_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute5_out_stage4;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute5_out_stage4_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute6_out_stage4;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute6_out_stage4_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute7_out_stage4;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute7_out_stage4_reg;

  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute0_out_stage3;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute0_out_stage3_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute1_out_stage3;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute1_out_stage3_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute2_out_stage3;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute2_out_stage3_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute3_out_stage3;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute3_out_stage3_reg;

  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute0_out_stage2;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute0_out_stage2_reg;
  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute1_out_stage2;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute1_out_stage2_reg;

  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute0_out_stage1;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] compute0_out_stage1_reg;

  wire   [`DWIDTH+`LOGDWIDTH-1 : 0] compute0_out_stage0;
  reg    [`DWIDTH+`LOGDWIDTH-1 : 0] outp;

  always @(posedge clk) begin
    if (reset) begin
      outp <= 0;
      compute0_out_stage7_reg <= 0;
      compute1_out_stage7_reg <= 0;
      compute2_out_stage7_reg <= 0;
      compute3_out_stage7_reg <= 0;
      compute4_out_stage7_reg <= 0;
      compute5_out_stage7_reg <= 0;
      compute6_out_stage7_reg <= 0;
      compute7_out_stage7_reg <= 0;
      compute8_out_stage7_reg <= 0;
      compute9_out_stage7_reg <= 0;
      compute10_out_stage7_reg <= 0;
      compute11_out_stage7_reg <= 0;
      compute12_out_stage7_reg <= 0;
      compute13_out_stage7_reg <= 0;
      compute14_out_stage7_reg <= 0;
      compute15_out_stage7_reg <= 0;
      compute16_out_stage7_reg <= 0;
      compute17_out_stage7_reg <= 0;
      compute18_out_stage7_reg <= 0;
      compute19_out_stage7_reg <= 0;
      compute20_out_stage7_reg <= 0;
      compute21_out_stage7_reg <= 0;
      compute22_out_stage7_reg <= 0;
      compute23_out_stage7_reg <= 0;
      compute24_out_stage7_reg <= 0;
      compute25_out_stage7_reg <= 0;
      compute26_out_stage7_reg <= 0;
      compute27_out_stage7_reg <= 0;
      compute28_out_stage7_reg <= 0;
      compute29_out_stage7_reg <= 0;
      compute30_out_stage7_reg <= 0;
      compute31_out_stage7_reg <= 0;
      compute32_out_stage7_reg <= 0;
      compute33_out_stage7_reg <= 0;
      compute34_out_stage7_reg <= 0;
      compute35_out_stage7_reg <= 0;
      compute36_out_stage7_reg <= 0;
      compute37_out_stage7_reg <= 0;
      compute38_out_stage7_reg <= 0;
      compute39_out_stage7_reg <= 0;
      compute40_out_stage7_reg <= 0;
      compute41_out_stage7_reg <= 0;
      compute42_out_stage7_reg <= 0;
      compute43_out_stage7_reg <= 0;
      compute44_out_stage7_reg <= 0;
      compute45_out_stage7_reg <= 0;
      compute46_out_stage7_reg <= 0;
      compute47_out_stage7_reg <= 0;
      compute48_out_stage7_reg <= 0;
      compute49_out_stage7_reg <= 0;
      compute50_out_stage7_reg <= 0;
      compute51_out_stage7_reg <= 0;
      compute52_out_stage7_reg <= 0;
      compute53_out_stage7_reg <= 0;
      compute54_out_stage7_reg <= 0;
      compute55_out_stage7_reg <= 0;
      compute56_out_stage7_reg <= 0;
      compute57_out_stage7_reg <= 0;
      compute58_out_stage7_reg <= 0;
      compute59_out_stage7_reg <= 0;
      compute60_out_stage7_reg <= 0;
      compute61_out_stage7_reg <= 0;
      compute62_out_stage7_reg <= 0;
      compute63_out_stage7_reg <= 0;
      compute0_out_stage6_reg <= 0;
      compute1_out_stage6_reg <= 0;
      compute2_out_stage6_reg <= 0;
      compute3_out_stage6_reg <= 0;
      compute4_out_stage6_reg <= 0;
      compute5_out_stage6_reg <= 0;
      compute6_out_stage6_reg <= 0;
      compute7_out_stage6_reg <= 0;
      compute8_out_stage6_reg <= 0;
      compute9_out_stage6_reg <= 0;
      compute10_out_stage6_reg <= 0;
      compute11_out_stage6_reg <= 0;
      compute12_out_stage6_reg <= 0;
      compute13_out_stage6_reg <= 0;
      compute14_out_stage6_reg <= 0;
      compute15_out_stage6_reg <= 0;
      compute16_out_stage6_reg <= 0;
      compute17_out_stage6_reg <= 0;
      compute18_out_stage6_reg <= 0;
      compute19_out_stage6_reg <= 0;
      compute20_out_stage6_reg <= 0;
      compute21_out_stage6_reg <= 0;
      compute22_out_stage6_reg <= 0;
      compute23_out_stage6_reg <= 0;
      compute24_out_stage6_reg <= 0;
      compute25_out_stage6_reg <= 0;
      compute26_out_stage6_reg <= 0;
      compute27_out_stage6_reg <= 0;
      compute28_out_stage6_reg <= 0;
      compute29_out_stage6_reg <= 0;
      compute30_out_stage6_reg <= 0;
      compute31_out_stage6_reg <= 0;
      compute0_out_stage5_reg <= 0;
      compute1_out_stage5_reg <= 0;
      compute2_out_stage5_reg <= 0;
      compute3_out_stage5_reg <= 0;
      compute4_out_stage5_reg <= 0;
      compute5_out_stage5_reg <= 0;
      compute6_out_stage5_reg <= 0;
      compute7_out_stage5_reg <= 0;
      compute8_out_stage5_reg <= 0;
      compute9_out_stage5_reg <= 0;
      compute10_out_stage5_reg <= 0;
      compute11_out_stage5_reg <= 0;
      compute12_out_stage5_reg <= 0;
      compute13_out_stage5_reg <= 0;
      compute14_out_stage5_reg <= 0;
      compute15_out_stage5_reg <= 0;
      compute0_out_stage4_reg <= 0;
      compute1_out_stage4_reg <= 0;
      compute2_out_stage4_reg <= 0;
      compute3_out_stage4_reg <= 0;
      compute4_out_stage4_reg <= 0;
      compute5_out_stage4_reg <= 0;
      compute6_out_stage4_reg <= 0;
      compute7_out_stage4_reg <= 0;
      compute0_out_stage3_reg <= 0;
      compute1_out_stage3_reg <= 0;
      compute2_out_stage3_reg <= 0;
      compute3_out_stage3_reg <= 0;
      compute0_out_stage2_reg <= 0;
      compute1_out_stage2_reg <= 0;
      compute0_out_stage1_reg <= 0;
    end

    else begin
      compute0_out_stage7_reg <= compute0_out_stage7;
      compute1_out_stage7_reg <= compute1_out_stage7;
      compute2_out_stage7_reg <= compute2_out_stage7;
      compute3_out_stage7_reg <= compute3_out_stage7;
      compute4_out_stage7_reg <= compute4_out_stage7;
      compute5_out_stage7_reg <= compute5_out_stage7;
      compute6_out_stage7_reg <= compute6_out_stage7;
      compute7_out_stage7_reg <= compute7_out_stage7;
      compute8_out_stage7_reg <= compute8_out_stage7;
      compute9_out_stage7_reg <= compute9_out_stage7;
      compute10_out_stage7_reg <= compute10_out_stage7;
      compute11_out_stage7_reg <= compute11_out_stage7;
      compute12_out_stage7_reg <= compute12_out_stage7;
      compute13_out_stage7_reg <= compute13_out_stage7;
      compute14_out_stage7_reg <= compute14_out_stage7;
      compute15_out_stage7_reg <= compute15_out_stage7;
      compute16_out_stage7_reg <= compute16_out_stage7;
      compute17_out_stage7_reg <= compute17_out_stage7;
      compute18_out_stage7_reg <= compute18_out_stage7;
      compute19_out_stage7_reg <= compute19_out_stage7;
      compute20_out_stage7_reg <= compute20_out_stage7;
      compute21_out_stage7_reg <= compute21_out_stage7;
      compute22_out_stage7_reg <= compute22_out_stage7;
      compute23_out_stage7_reg <= compute23_out_stage7;
      compute24_out_stage7_reg <= compute24_out_stage7;
      compute25_out_stage7_reg <= compute25_out_stage7;
      compute26_out_stage7_reg <= compute26_out_stage7;
      compute27_out_stage7_reg <= compute27_out_stage7;
      compute28_out_stage7_reg <= compute28_out_stage7;
      compute29_out_stage7_reg <= compute29_out_stage7;
      compute30_out_stage7_reg <= compute30_out_stage7;
      compute31_out_stage7_reg <= compute31_out_stage7;
      compute32_out_stage7_reg <= compute32_out_stage7;
      compute33_out_stage7_reg <= compute33_out_stage7;
      compute34_out_stage7_reg <= compute34_out_stage7;
      compute35_out_stage7_reg <= compute35_out_stage7;
      compute36_out_stage7_reg <= compute36_out_stage7;
      compute37_out_stage7_reg <= compute37_out_stage7;
      compute38_out_stage7_reg <= compute38_out_stage7;
      compute39_out_stage7_reg <= compute39_out_stage7;
      compute40_out_stage7_reg <= compute40_out_stage7;
      compute41_out_stage7_reg <= compute41_out_stage7;
      compute42_out_stage7_reg <= compute42_out_stage7;
      compute43_out_stage7_reg <= compute43_out_stage7;
      compute44_out_stage7_reg <= compute44_out_stage7;
      compute45_out_stage7_reg <= compute45_out_stage7;
      compute46_out_stage7_reg <= compute46_out_stage7;
      compute47_out_stage7_reg <= compute47_out_stage7;
      compute48_out_stage7_reg <= compute48_out_stage7;
      compute49_out_stage7_reg <= compute49_out_stage7;
      compute50_out_stage7_reg <= compute50_out_stage7;
      compute51_out_stage7_reg <= compute51_out_stage7;
      compute52_out_stage7_reg <= compute52_out_stage7;
      compute53_out_stage7_reg <= compute53_out_stage7;
      compute54_out_stage7_reg <= compute54_out_stage7;
      compute55_out_stage7_reg <= compute55_out_stage7;
      compute56_out_stage7_reg <= compute56_out_stage7;
      compute57_out_stage7_reg <= compute57_out_stage7;
      compute58_out_stage7_reg <= compute58_out_stage7;
      compute59_out_stage7_reg <= compute59_out_stage7;
      compute60_out_stage7_reg <= compute60_out_stage7;
      compute61_out_stage7_reg <= compute61_out_stage7;
      compute62_out_stage7_reg <= compute62_out_stage7;
      compute63_out_stage7_reg <= compute63_out_stage7;

      compute0_out_stage6_reg <= compute0_out_stage6;
      compute1_out_stage6_reg <= compute1_out_stage6;
      compute2_out_stage6_reg <= compute2_out_stage6;
      compute3_out_stage6_reg <= compute3_out_stage6;
      compute4_out_stage6_reg <= compute4_out_stage6;
      compute5_out_stage6_reg <= compute5_out_stage6;
      compute6_out_stage6_reg <= compute6_out_stage6;
      compute7_out_stage6_reg <= compute7_out_stage6;
      compute8_out_stage6_reg <= compute8_out_stage6;
      compute9_out_stage6_reg <= compute9_out_stage6;
      compute10_out_stage6_reg <= compute10_out_stage6;
      compute11_out_stage6_reg <= compute11_out_stage6;
      compute12_out_stage6_reg <= compute12_out_stage6;
      compute13_out_stage6_reg <= compute13_out_stage6;
      compute14_out_stage6_reg <= compute14_out_stage6;
      compute15_out_stage6_reg <= compute15_out_stage6;
      compute16_out_stage6_reg <= compute16_out_stage6;
      compute17_out_stage6_reg <= compute17_out_stage6;
      compute18_out_stage6_reg <= compute18_out_stage6;
      compute19_out_stage6_reg <= compute19_out_stage6;
      compute20_out_stage6_reg <= compute20_out_stage6;
      compute21_out_stage6_reg <= compute21_out_stage6;
      compute22_out_stage6_reg <= compute22_out_stage6;
      compute23_out_stage6_reg <= compute23_out_stage6;
      compute24_out_stage6_reg <= compute24_out_stage6;
      compute25_out_stage6_reg <= compute25_out_stage6;
      compute26_out_stage6_reg <= compute26_out_stage6;
      compute27_out_stage6_reg <= compute27_out_stage6;
      compute28_out_stage6_reg <= compute28_out_stage6;
      compute29_out_stage6_reg <= compute29_out_stage6;
      compute30_out_stage6_reg <= compute30_out_stage6;
      compute31_out_stage6_reg <= compute31_out_stage6;

      compute0_out_stage5_reg <= compute0_out_stage5;
      compute1_out_stage5_reg <= compute1_out_stage5;
      compute2_out_stage5_reg <= compute2_out_stage5;
      compute3_out_stage5_reg <= compute3_out_stage5;
      compute4_out_stage5_reg <= compute4_out_stage5;
      compute5_out_stage5_reg <= compute5_out_stage5;
      compute6_out_stage5_reg <= compute6_out_stage5;
      compute7_out_stage5_reg <= compute7_out_stage5;
      compute8_out_stage5_reg <= compute8_out_stage5;
      compute9_out_stage5_reg <= compute9_out_stage5;
      compute10_out_stage5_reg <= compute10_out_stage5;
      compute11_out_stage5_reg <= compute11_out_stage5;
      compute12_out_stage5_reg <= compute12_out_stage5;
      compute13_out_stage5_reg <= compute13_out_stage5;
      compute14_out_stage5_reg <= compute14_out_stage5;
      compute15_out_stage5_reg <= compute15_out_stage5;

      compute0_out_stage4_reg <= compute0_out_stage4;
      compute1_out_stage4_reg <= compute1_out_stage4;
      compute2_out_stage4_reg <= compute2_out_stage4;
      compute3_out_stage4_reg <= compute3_out_stage4;
      compute4_out_stage4_reg <= compute4_out_stage4;
      compute5_out_stage4_reg <= compute5_out_stage4;
      compute6_out_stage4_reg <= compute6_out_stage4;
      compute7_out_stage4_reg <= compute7_out_stage4;

      compute0_out_stage3_reg <= compute0_out_stage3;
      compute1_out_stage3_reg <= compute1_out_stage3;
      compute2_out_stage3_reg <= compute2_out_stage3;
      compute3_out_stage3_reg <= compute3_out_stage3;

      compute0_out_stage2_reg <= compute0_out_stage2;
      compute1_out_stage2_reg <= compute1_out_stage2;

      compute0_out_stage1_reg <= compute0_out_stage1;

      outp <= compute0_out_stage0;

    end
  end

  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute0_stage7(.A(inp0),       .B(inp1),    .OUT(compute0_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute1_stage7(.A(inp2),       .B(inp3),    .OUT(compute1_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute2_stage7(.A(inp4),       .B(inp5),    .OUT(compute2_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute3_stage7(.A(inp6),       .B(inp7),    .OUT(compute3_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute4_stage7(.A(inp8),       .B(inp9),    .OUT(compute4_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute5_stage7(.A(inp10),       .B(inp11),    .OUT(compute5_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute6_stage7(.A(inp12),       .B(inp13),    .OUT(compute6_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute7_stage7(.A(inp14),       .B(inp15),    .OUT(compute7_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute8_stage7(.A(inp16),       .B(inp17),    .OUT(compute8_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute9_stage7(.A(inp18),       .B(inp19),    .OUT(compute9_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute10_stage7(.A(inp20),       .B(inp21),    .OUT(compute10_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute11_stage7(.A(inp22),       .B(inp23),    .OUT(compute11_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute12_stage7(.A(inp24),       .B(inp25),    .OUT(compute12_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute13_stage7(.A(inp26),       .B(inp27),    .OUT(compute13_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute14_stage7(.A(inp28),       .B(inp29),    .OUT(compute14_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute15_stage7(.A(inp30),       .B(inp31),    .OUT(compute15_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute16_stage7(.A(inp32),       .B(inp33),    .OUT(compute16_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute17_stage7(.A(inp34),       .B(inp35),    .OUT(compute17_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute18_stage7(.A(inp36),       .B(inp37),    .OUT(compute18_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute19_stage7(.A(inp38),       .B(inp39),    .OUT(compute19_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute20_stage7(.A(inp40),       .B(inp41),    .OUT(compute20_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute21_stage7(.A(inp42),       .B(inp43),    .OUT(compute21_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute22_stage7(.A(inp44),       .B(inp45),    .OUT(compute22_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute23_stage7(.A(inp46),       .B(inp47),    .OUT(compute23_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute24_stage7(.A(inp48),       .B(inp49),    .OUT(compute24_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute25_stage7(.A(inp50),       .B(inp51),    .OUT(compute25_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute26_stage7(.A(inp52),       .B(inp53),    .OUT(compute26_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute27_stage7(.A(inp54),       .B(inp55),    .OUT(compute27_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute28_stage7(.A(inp56),       .B(inp57),    .OUT(compute28_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute29_stage7(.A(inp58),       .B(inp59),    .OUT(compute29_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute30_stage7(.A(inp60),       .B(inp61),    .OUT(compute30_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute31_stage7(.A(inp62),       .B(inp63),    .OUT(compute31_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute32_stage7(.A(inp64),       .B(inp65),    .OUT(compute32_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute33_stage7(.A(inp66),       .B(inp67),    .OUT(compute33_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute34_stage7(.A(inp68),       .B(inp69),    .OUT(compute34_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute35_stage7(.A(inp70),       .B(inp71),    .OUT(compute35_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute36_stage7(.A(inp72),       .B(inp73),    .OUT(compute36_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute37_stage7(.A(inp74),       .B(inp75),    .OUT(compute37_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute38_stage7(.A(inp76),       .B(inp77),    .OUT(compute38_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute39_stage7(.A(inp78),       .B(inp79),    .OUT(compute39_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute40_stage7(.A(inp80),       .B(inp81),    .OUT(compute40_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute41_stage7(.A(inp82),       .B(inp83),    .OUT(compute41_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute42_stage7(.A(inp84),       .B(inp85),    .OUT(compute42_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute43_stage7(.A(inp86),       .B(inp87),    .OUT(compute43_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute44_stage7(.A(inp88),       .B(inp89),    .OUT(compute44_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute45_stage7(.A(inp90),       .B(inp91),    .OUT(compute45_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute46_stage7(.A(inp92),       .B(inp93),    .OUT(compute46_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute47_stage7(.A(inp94),       .B(inp95),    .OUT(compute47_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute48_stage7(.A(inp96),       .B(inp97),    .OUT(compute48_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute49_stage7(.A(inp98),       .B(inp99),    .OUT(compute49_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute50_stage7(.A(inp100),       .B(inp101),    .OUT(compute50_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute51_stage7(.A(inp102),       .B(inp103),    .OUT(compute51_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute52_stage7(.A(inp104),       .B(inp105),    .OUT(compute52_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute53_stage7(.A(inp106),       .B(inp107),    .OUT(compute53_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute54_stage7(.A(inp108),       .B(inp109),    .OUT(compute54_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute55_stage7(.A(inp110),       .B(inp111),    .OUT(compute55_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute56_stage7(.A(inp112),       .B(inp113),    .OUT(compute56_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute57_stage7(.A(inp114),       .B(inp115),    .OUT(compute57_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute58_stage7(.A(inp116),       .B(inp117),    .OUT(compute58_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute59_stage7(.A(inp118),       .B(inp119),    .OUT(compute59_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute60_stage7(.A(inp120),       .B(inp121),    .OUT(compute60_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute61_stage7(.A(inp122),       .B(inp123),    .OUT(compute61_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute62_stage7(.A(inp124),       .B(inp125),    .OUT(compute62_out_stage7), .MODE(mode));
  processing_element #(`DWIDTH,`DWIDTH+`LOGDWIDTH) compute63_stage7(.A(inp126),       .B(inp127),    .OUT(compute63_out_stage7), .MODE(mode));

  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute0_stage6(.A(compute0_out_stage7_reg),       .B(compute1_out_stage7_reg),    .OUT(compute0_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute1_stage6(.A(compute2_out_stage7_reg),       .B(compute3_out_stage7_reg),    .OUT(compute1_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute2_stage6(.A(compute4_out_stage7_reg),       .B(compute5_out_stage7_reg),    .OUT(compute2_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute3_stage6(.A(compute6_out_stage7_reg),       .B(compute7_out_stage7_reg),    .OUT(compute3_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute4_stage6(.A(compute8_out_stage7_reg),       .B(compute9_out_stage7_reg),    .OUT(compute4_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute5_stage6(.A(compute10_out_stage7_reg),       .B(compute11_out_stage7_reg),    .OUT(compute5_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute6_stage6(.A(compute12_out_stage7_reg),       .B(compute13_out_stage7_reg),    .OUT(compute6_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute7_stage6(.A(compute14_out_stage7_reg),       .B(compute15_out_stage7_reg),    .OUT(compute7_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute8_stage6(.A(compute16_out_stage7_reg),       .B(compute17_out_stage7_reg),    .OUT(compute8_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute9_stage6(.A(compute18_out_stage7_reg),       .B(compute19_out_stage7_reg),    .OUT(compute9_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute10_stage6(.A(compute20_out_stage7_reg),       .B(compute21_out_stage7_reg),    .OUT(compute10_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute11_stage6(.A(compute22_out_stage7_reg),       .B(compute23_out_stage7_reg),    .OUT(compute11_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute12_stage6(.A(compute24_out_stage7_reg),       .B(compute25_out_stage7_reg),    .OUT(compute12_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute13_stage6(.A(compute26_out_stage7_reg),       .B(compute27_out_stage7_reg),    .OUT(compute13_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute14_stage6(.A(compute28_out_stage7_reg),       .B(compute29_out_stage7_reg),    .OUT(compute14_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute15_stage6(.A(compute30_out_stage7_reg),       .B(compute31_out_stage7_reg),    .OUT(compute15_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute16_stage6(.A(compute32_out_stage7_reg),       .B(compute33_out_stage7_reg),    .OUT(compute16_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute17_stage6(.A(compute34_out_stage7_reg),       .B(compute35_out_stage7_reg),    .OUT(compute17_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute18_stage6(.A(compute36_out_stage7_reg),       .B(compute37_out_stage7_reg),    .OUT(compute18_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute19_stage6(.A(compute38_out_stage7_reg),       .B(compute39_out_stage7_reg),    .OUT(compute19_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute20_stage6(.A(compute40_out_stage7_reg),       .B(compute41_out_stage7_reg),    .OUT(compute20_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute21_stage6(.A(compute42_out_stage7_reg),       .B(compute43_out_stage7_reg),    .OUT(compute21_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute22_stage6(.A(compute44_out_stage7_reg),       .B(compute45_out_stage7_reg),    .OUT(compute22_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute23_stage6(.A(compute46_out_stage7_reg),       .B(compute47_out_stage7_reg),    .OUT(compute23_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute24_stage6(.A(compute48_out_stage7_reg),       .B(compute49_out_stage7_reg),    .OUT(compute24_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute25_stage6(.A(compute50_out_stage7_reg),       .B(compute51_out_stage7_reg),    .OUT(compute25_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute26_stage6(.A(compute52_out_stage7_reg),       .B(compute53_out_stage7_reg),    .OUT(compute26_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute27_stage6(.A(compute54_out_stage7_reg),       .B(compute55_out_stage7_reg),    .OUT(compute27_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute28_stage6(.A(compute56_out_stage7_reg),       .B(compute57_out_stage7_reg),    .OUT(compute28_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute29_stage6(.A(compute58_out_stage7_reg),       .B(compute59_out_stage7_reg),    .OUT(compute29_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute30_stage6(.A(compute60_out_stage7_reg),       .B(compute61_out_stage7_reg),    .OUT(compute30_out_stage6), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute31_stage6(.A(compute62_out_stage7_reg),       .B(compute63_out_stage7_reg),    .OUT(compute31_out_stage6), .MODE(mode));

  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute0_stage5(.A(compute0_out_stage6_reg),       .B(compute1_out_stage6_reg),    .OUT(compute0_out_stage5), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute1_stage5(.A(compute2_out_stage6_reg),       .B(compute3_out_stage6_reg),    .OUT(compute1_out_stage5), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute2_stage5(.A(compute4_out_stage6_reg),       .B(compute5_out_stage6_reg),    .OUT(compute2_out_stage5), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute3_stage5(.A(compute6_out_stage6_reg),       .B(compute7_out_stage6_reg),    .OUT(compute3_out_stage5), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute4_stage5(.A(compute8_out_stage6_reg),       .B(compute9_out_stage6_reg),    .OUT(compute4_out_stage5), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute5_stage5(.A(compute10_out_stage6_reg),       .B(compute11_out_stage6_reg),    .OUT(compute5_out_stage5), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute6_stage5(.A(compute12_out_stage6_reg),       .B(compute13_out_stage6_reg),    .OUT(compute6_out_stage5), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute7_stage5(.A(compute14_out_stage6_reg),       .B(compute15_out_stage6_reg),    .OUT(compute7_out_stage5), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute8_stage5(.A(compute16_out_stage6_reg),       .B(compute17_out_stage6_reg),    .OUT(compute8_out_stage5), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute9_stage5(.A(compute18_out_stage6_reg),       .B(compute19_out_stage6_reg),    .OUT(compute9_out_stage5), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute10_stage5(.A(compute20_out_stage6_reg),       .B(compute21_out_stage6_reg),    .OUT(compute10_out_stage5), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute11_stage5(.A(compute22_out_stage6_reg),       .B(compute23_out_stage6_reg),    .OUT(compute11_out_stage5), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute12_stage5(.A(compute24_out_stage6_reg),       .B(compute25_out_stage6_reg),    .OUT(compute12_out_stage5), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute13_stage5(.A(compute26_out_stage6_reg),       .B(compute27_out_stage6_reg),    .OUT(compute13_out_stage5), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute14_stage5(.A(compute28_out_stage6_reg),       .B(compute29_out_stage6_reg),    .OUT(compute14_out_stage5), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute15_stage5(.A(compute30_out_stage6_reg),       .B(compute31_out_stage6_reg),    .OUT(compute15_out_stage5), .MODE(mode));

  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute0_stage4(.A(compute0_out_stage5_reg),       .B(compute1_out_stage5_reg),    .OUT(compute0_out_stage4), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute1_stage4(.A(compute2_out_stage5_reg),       .B(compute3_out_stage5_reg),    .OUT(compute1_out_stage4), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute2_stage4(.A(compute4_out_stage5_reg),       .B(compute5_out_stage5_reg),    .OUT(compute2_out_stage4), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute3_stage4(.A(compute6_out_stage5_reg),       .B(compute7_out_stage5_reg),    .OUT(compute3_out_stage4), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute4_stage4(.A(compute8_out_stage5_reg),       .B(compute9_out_stage5_reg),    .OUT(compute4_out_stage4), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute5_stage4(.A(compute10_out_stage5_reg),       .B(compute11_out_stage5_reg),    .OUT(compute5_out_stage4), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute6_stage4(.A(compute12_out_stage5_reg),       .B(compute13_out_stage5_reg),    .OUT(compute6_out_stage4), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute7_stage4(.A(compute14_out_stage5_reg),       .B(compute15_out_stage5_reg),    .OUT(compute7_out_stage4), .MODE(mode));

  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute0_stage3(.A(compute0_out_stage4_reg),       .B(compute1_out_stage4_reg),    .OUT(compute0_out_stage3), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute1_stage3(.A(compute2_out_stage4_reg),       .B(compute3_out_stage4_reg),    .OUT(compute1_out_stage3), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute2_stage3(.A(compute4_out_stage4_reg),       .B(compute5_out_stage4_reg),    .OUT(compute2_out_stage3), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute3_stage3(.A(compute6_out_stage4_reg),       .B(compute7_out_stage4_reg),    .OUT(compute3_out_stage3), .MODE(mode));

  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute0_stage2(.A(compute0_out_stage3_reg),       .B(compute1_out_stage3_reg),    .OUT(compute0_out_stage2), .MODE(mode));
  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute1_stage2(.A(compute2_out_stage3_reg),       .B(compute3_out_stage3_reg),    .OUT(compute1_out_stage2), .MODE(mode));

  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute0_stage1(.A(compute0_out_stage2_reg),       .B(compute1_out_stage2_reg),    .OUT(compute0_out_stage1), .MODE(mode));

  processing_element #(`DWIDTH+`LOGDWIDTH,`DWIDTH+`LOGDWIDTH) compute0_stage0(.A(outp),       .B(compute0_out_stage1_reg),     .OUT(compute0_out_stage0), .MODE(mode));

endmodule


///////////////////////////////////////////////////////
// Processing element. Finds sum, min or max depending on mode
///////////////////////////////////////////////////////
module processing_element(
  A, B, OUT, MODE
);
parameter IN_DWIDTH = 16;
parameter OUT_DWIDTH = 4;
input [IN_DWIDTH-1:0] A;
input [IN_DWIDTH-1:0] B;
output [OUT_DWIDTH-1:0] OUT;
input [1:0] MODE;

wire [OUT_DWIDTH-1:0] greater;
wire [OUT_DWIDTH-1:0] smaller;
wire [OUT_DWIDTH-1:0] sum;

assign greater = (A>B) ? A : B;
assign smaller = (A<B) ? A : B;
assign sum = A + B;

assign OUT = (MODE==0) ? sum : 
             (MODE==1) ? greater :
             smaller;

endmodule


///////////////////////////////////////////////////////
//Rounding logic based on convergent rounding described 
//here: https://zipcpu.com/dsp/2017/07/22/rounding.html
///////////////////////////////////////////////////////
module rounding( i_data, o_data );
parameter IWID = 32;
parameter OWID = 16;
input  [IWID-1:0] i_data;
output [OWID-1:0] o_data;

wire [IWID-1:0] w_convergent;

assign	w_convergent = i_data[(IWID-1):0]
			+ { {(OWID){1'b0}},
				i_data[(IWID-OWID)],
				{(IWID-OWID-1){!i_data[(IWID-OWID)]}}};

assign o_data = w_convergent[(IWID-1):(IWID-OWID)];

endmodule


