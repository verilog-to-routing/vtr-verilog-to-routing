`timescale 1ps/1ps
//4-Input Look Up Table module
module LUT_4(in_3,in_2,in_1,in_0,out);

//Truth table parameter represents the default function of the LUT.
//The most significant bit is the output when all inputs are logic one.
parameter Truth_table=16'b0000000000000000; 

input in_0,in_1,in_2,in_3;
output out;

//First column of multiplexers taking the truth table parameters as inputs
mux level0_1(in_0,Truth_table[15],Truth_table[14],internal_0_1);
mux level0_2(in_0,Truth_table[13],Truth_table[12],internal_0_2);
mux level0_3(in_0,Truth_table[11],Truth_table[10],internal_0_3);
mux level0_4(in_0,Truth_table[9],Truth_table[8],internal_0_4);
mux level0_5(in_0,Truth_table[7],Truth_table[6],internal_0_5);
mux level0_6(in_0,Truth_table[5],Truth_table[4],internal_0_6);
mux level0_7(in_0,Truth_table[3],Truth_table[2],internal_0_7);
mux level0_8(in_0,Truth_table[1],Truth_table[0],internal_0_8);

//Second column of multiplexers taking the output of the first colum as input.
mux level1_1(in_1,internal_0_1,internal_0_2,internal_1_1);
mux level1_2(in_1,internal_0_3,internal_0_4,internal_1_2);
mux level1_3(in_1,internal_0_5,internal_0_6,internal_1_3);
mux level1_4(in_1,internal_0_7,internal_0_8,internal_1_4);

//Third column of multiplexers taking the output of the second column as input.
mux level2_1(in_2,internal_1_1,internal_1_2,internal_2_1);
mux level2_2(in_2,internal_1_3,internal_1_4,internal_2_2);

//Last multiplexer passes the correct logic state to the output of the module.
mux level3_1(in_3,internal_2_1,internal_2_2,out);

endmodule

//6-Input Look Up Table module
module LUT_6(in_5,in_4,in_3,in_2,in_1,in_0,out);

//Truth table parameter represents the default function of the LUT.
//The most significant bit is the output when all inputs are logic one.
parameter Truth_table=64'b0;

input in_0,in_1,in_2,in_3,in_4,in_5;
output out;
wire [5:0]a;

interconnect inter0(in_0 , a[0]);
interconnect inter1(in_1 , a[1]);
interconnect inter2(in_2 , a[2]);
interconnect inter3(in_3 , a[3]);
interconnect inter4(in_4 , a[4]);
interconnect inter5(in_5 , a[5]);


//First column of multiplexers taking the truth table parameters as inputs
mux level0_1(a[0],Truth_table[0],Truth_table[1],internal_0_1);
mux level0_2(a[0],Truth_table[2],Truth_table[3],internal_0_2);
mux level0_3(a[0],Truth_table[4],Truth_table[5],internal_0_3);
mux level0_4(a[0],Truth_table[6],Truth_table[7],internal_0_4);
mux level0_5(a[0],Truth_table[8],Truth_table[9],internal_0_5);
mux level0_6(a[0],Truth_table[10],Truth_table[11],internal_0_6);
mux level0_7(a[0],Truth_table[12],Truth_table[13],internal_0_7);
mux level0_8(a[0],Truth_table[14],Truth_table[15],internal_0_8);
mux level0_9(a[0],Truth_table[16],Truth_table[17],internal_0_9);
mux level0_10(a[0],Truth_table[18],Truth_table[19],internal_0_10);
mux level0_11(a[0],Truth_table[20],Truth_table[21],internal_0_11);
mux level0_12(a[0],Truth_table[22],Truth_table[23],internal_0_12);
mux level0_13(a[0],Truth_table[24],Truth_table[25],internal_0_13);
mux level0_14(a[0],Truth_table[26],Truth_table[27],internal_0_14);
mux level0_15(a[0],Truth_table[28],Truth_table[29],internal_0_15);
mux level0_16(a[0],Truth_table[30],Truth_table[31],internal_0_16);
mux level0_17(a[0],Truth_table[32],Truth_table[33],internal_0_17);
mux level0_18(a[0],Truth_table[34],Truth_table[35],internal_0_18);
mux level0_19(a[0],Truth_table[36],Truth_table[37],internal_0_19);
mux level0_20(a[0],Truth_table[38],Truth_table[39],internal_0_20);
mux level0_21(a[0],Truth_table[40],Truth_table[41],internal_0_21);
mux level0_22(a[0],Truth_table[42],Truth_table[43],internal_0_22);
mux level0_23(a[0],Truth_table[44],Truth_table[45],internal_0_23);
mux level0_24(a[0],Truth_table[46],Truth_table[47],internal_0_24);
mux level0_25(a[0],Truth_table[48],Truth_table[49],internal_0_25);
mux level0_26(a[0],Truth_table[50],Truth_table[51],internal_0_26);
mux level0_27(a[0],Truth_table[52],Truth_table[53],internal_0_27);
mux level0_28(a[0],Truth_table[54],Truth_table[55],internal_0_28);
mux level0_29(a[0],Truth_table[56],Truth_table[57],internal_0_29);
mux level0_30(a[0],Truth_table[58],Truth_table[59],internal_0_30);
mux level0_31(a[0],Truth_table[60],Truth_table[61],internal_0_31);
mux level0_32(a[0],Truth_table[62],Truth_table[63],internal_0_32);

//Second column of multiplexers taking the output of the first colum as input.
mux level1_1(a[1],internal_0_1,internal_0_2,internal_1_1);
mux level1_2(a[1],internal_0_3,internal_0_4,internal_1_2);
mux level1_3(a[1],internal_0_5,internal_0_6,internal_1_3);
mux level1_4(a[1],internal_0_7,internal_0_8,internal_1_4);
mux level1_5(a[1],internal_0_9,internal_0_10,internal_1_5);
mux level1_6(a[1],internal_0_11,internal_0_12,internal_1_6);
mux level1_7(a[1],internal_0_13,internal_0_14,internal_1_7);
mux level1_8(a[1],internal_0_15,internal_0_16,internal_1_8);
mux level1_9(a[1],internal_0_17,internal_0_18,internal_1_9);
mux level1_10(a[1],internal_0_19,internal_0_20,internal_1_10);
mux level1_11(a[1],internal_0_21,internal_0_22,internal_1_11);
mux level1_12(a[1],internal_0_23,internal_0_24,internal_1_12);
mux level1_13(a[1],internal_0_25,internal_0_26,internal_1_13);
mux level1_14(a[1],internal_0_27,internal_0_28,internal_1_14);
mux level1_15(a[1],internal_0_29,internal_0_30,internal_1_15);
mux level1_16(a[1],internal_0_31,internal_0_32,internal_1_16);

//Third column of multiplexers taking the output of the second column as input.
mux level2_1(a[2],internal_1_1,internal_1_2,internal_2_1);
mux level2_2(a[2],internal_1_3,internal_1_4,internal_2_2);
mux level2_3(a[2],internal_1_5,internal_1_6,internal_2_3);
mux level2_4(a[2],internal_1_7,internal_1_8,internal_2_4);
mux level2_5(a[2],internal_1_9,internal_1_10,internal_2_5);
mux level2_6(a[2],internal_1_11,internal_1_12,internal_2_6);
mux level2_7(a[2],internal_1_13,internal_1_14,internal_2_7);
mux level2_8(a[2],internal_1_15,internal_1_16,internal_2_8);

//Fourth column of multiplexers taking the output of the third column as input.
mux level3_1(a[3],internal_2_1,internal_2_2,internal_3_1);
mux level3_2(a[3],internal_2_3,internal_2_4,internal_3_2);
mux level3_3(a[3],internal_2_5,internal_2_6,internal_3_3);
mux level3_4(a[3],internal_2_7,internal_2_8,internal_3_4);

//Fifth column of multiplexers taking the output of the Fourth column as input.
mux level4_1(a[4],internal_3_1,internal_3_2,internal_4_1);
mux level4_2(a[4],internal_3_3,internal_3_4,internal_4_2);

//Last multiplexer passes the correct logic state to the output of the module.
mux level5_1(a[5],internal_4_1,internal_4_2,out);

endmodule

//D-FlipFlop module with synchronous active low clear and preset.
module D_Flip_Flop(clock,D,clear,preset,Q);

input clock,D,clear,preset;
output reg Q;

specify
	(clock => Q)="";
endspecify

always@(posedge clock)
begin
	if(clear==0)
		Q<=0;
	else if(preset==0)
		Q<=1;
	else
		Q<=D;
end
endmodule

//Routing interconnect module
module interconnect(datain,dataout);

input datain;
output dataout;

specify
	(datain=>dataout)="";
endspecify

assign dataout=datain;

endmodule


//2-to-1 mux module
module mux(select,x,y,z);

input select,x,y;
output z;

assign z=(x & ~select) | (y & select);

endmodule

//nxn multiplier module
module mult(inA,inB,result);

//the number of inputs to the multiplier is passes in as a parameter to the module
parameter inputs = 0;
 
   
   input[inputs-1:0]inA,inB;
   output reg[inputs + inputs -1:0] result;

   wire[inputs-1:0]inA1,inB1;
   Mult_interconnect #(inputs)delay(inA,inA1);
   Mult_interconnect #(inputs)delay2(inB,inB1);
   
   always@(inA1 or inB1)
   	result = inA1 * inB1;
   	
endmodule // mult

//This interconnect is needed to specify the delay of the multiplier in the SDF file
module Mult_interconnect(A,B);

   parameter num_inputs = 0;
   
   input [num_inputs-1:0]A;
   output [num_inputs-1:0] B;

   specify
      (A=>B)="";
   endspecify
   
   assign B = A;
   
endmodule // Mult_interconnect

//single_port_ram module
module single_port_ram(addr,data,we,out,clock);

   parameter addr_width = 0;
   parameter data_width = 0;
   parameter mem_depth = 1 << addr_width;

   input clock;
   input [addr_width-1:0] addr;
   input [data_width-1:0] data;
   input 		  we;
   output reg [data_width-1:0] out;
   
   reg [data_width-1:0]        Mem[0:mem_depth];

   specify
      (clock=>out)="";
   endspecify
   
   always@(posedge clock)
     begin
	if(we)
	  Mem[addr] = data;
     end
   
   always@(posedge clock)
     begin
	out = Mem[addr];
     end
  
endmodule // single_port_RAM

//dual_port_ram module
module dual_port_ram(addr1,addr2,data1,data2,we1,we2,out1,out2,clock);

   parameter addr1_width = 0;
   parameter data1_width = 0;
   parameter addr2_width = 0;
   parameter data2_width = 0;
   parameter mem_depth1 = 1 << addr1_width;
   parameter mem_depth2 = 1 << addr2_width;

   input clock;
   
   input [addr1_width-1:0] addr1;
   input [data1_width-1:0] data1;
   input 		   we1;
   output reg [data1_width-1:0] out1;
   
   input [addr2_width-1:0] 	addr2;
   input [data2_width-1:0] 	data2;
   input 			we2;
   output reg [data2_width-1:0] out2;
   
   reg [data1_width-1:0] 	Mem1[0:mem_depth1];
   reg [data2_width-1:0] 	Mem2[0:mem_depth2];

    specify
       (clock=>out1)="";
       (clock=>out2)="";
    endspecify
   
   always@(posedge clock)
     begin
	if(we1)
	  Mem1[addr1] = data1;
     end
   
   always@(posedge clock)
      begin
	if(we2)
	  Mem2[addr2] = data2;
     end
   
   always@(posedge clock)
     begin
	out1 = Mem1[addr1];
     end
   
   always@(posedge clock)
     begin
	out2 = Mem2[addr2];
     end
   
endmodule // dual_port_ram
