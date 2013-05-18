`timescale 1ps/1ps

//3-Input Look Up Table module
module LUT_3(in_2,in_1,in_0,out);

   //Truth table parameter represents the default function of the LUT.
   //The most significant bit is the output when all inputs are logic one.
   parameter Truth_table=8'b00000000; 
   
   input in_0,in_1,in_2;
   output reg out;
   integer     selected_row;
   wire [2:0]  a;

   interconnect inter0(in_0 , a[0]);
   interconnect inter1(in_1 , a[1]);
   interconnect inter2(in_2 , a[2]);
   
   always@(a[0], a[1], a[2])
     begin
	selected_row = {a[2], a[1], a[0]};
	out = Truth_table[selected_row];
     end
     
endmodule

//4-Input Look Up Table module
module LUT_4(in_3,in_2,in_1,in_0,out);

   //Truth table parameter represents the default function of the LUT.
   //The most significant bit is the output when all inputs are logic one.
   parameter Truth_table=16'b0000000000000000; 
   
   input in_0,in_1,in_2,in_3;
   output reg out;
   integer     selected_row;
   wire [3:0]  a;

   interconnect inter0(in_0 , a[0]);
   interconnect inter1(in_1 , a[1]);
   interconnect inter2(in_2 , a[2]);
   interconnect inter3(in_3 , a[3]);

   always@(a[0], a[1], a[2], a[3])
     begin
	selected_row = {a[3], a[2], a[1], a[0]};
	out = Truth_table[selected_row];
     end
     
endmodule

//5-Input Look Up Table module
module LUT_5(in_4,in_3,in_2,in_1,in_0,out);

   //Truth table parameter represents the default function of the LUT.
   //The most significant bit is the output when all inputs are logic one.
   parameter Truth_table=32'b00000000000000000000000000000000; 
   
   input in_0,in_1,in_2,in_3,in_4;
   output reg out;
   integer     selected_row = 0;

   wire [4:0]  a;

   interconnect inter0(in_0 , a[0]);
   interconnect inter1(in_1 , a[1]);
   interconnect inter2(in_2 , a[2]);
   interconnect inter3(in_3 , a[3]);
   interconnect inter4(in_4 , a[4]);
   
   always@(a[0], a[1], a[2], a[3], a[4])
     begin
	selected_row = {a[4], a[3], a[2], a[1], a[0]};
	out = Truth_table[selected_row];
     end
     
endmodule

//6-Input Look Up Table module
module LUT_6(in_5,in_4,in_3,in_2,in_1,in_0,out);

   //Truth table parameter represents the default function of the LUT.
   //The most significant bit is the output when all inputs are logic one.
   parameter Truth_table=64'b0000000000000000000000000000000000000000000000000000000000000000; 
   
   input in_0,in_1,in_2,in_3,in_4,in_5;
   output reg out;
   integer     selected_row;

   wire [5:0]  a;

   interconnect inter0(in_0 , a[0]);
   interconnect inter1(in_1 , a[1]);
   interconnect inter2(in_2 , a[2]);
   interconnect inter3(in_3 , a[3]);
   interconnect inter4(in_4 , a[4]);
   interconnect inter5(in_5 , a[5]);
   
   always@(a[0], a[1], a[2], a[3], a[4], a[5])
     begin
	selected_row = {a[5], a[4], a[3], a[2], a[1], a[0]};
	out = Truth_table[selected_row];
     end
     
endmodule

//7-Input Look Up Table module
module LUT_7(in_6,in_5,in_4,in_3,in_2,in_1,in_0,out);

   //Truth table parameter represents the default function of the LUT.
   //The most significant bit is the output when all inputs are logic one.
   parameter Truth_table=128'b00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000; 
   
   input in_0,in_1,in_2,in_3,in_4,in_5,in_6;
   output reg out;
   integer     selected_row;
   wire [6:0]  a;

   interconnect inter0(in_0 , a[0]);
   interconnect inter1(in_1 , a[1]);
   interconnect inter2(in_2 , a[2]);
   interconnect inter3(in_3 , a[3]);
   interconnect inter4(in_4 , a[4]);
   interconnect inter5(in_5 , a[5]);
   interconnect inter6(in_6 , a[6]);
   
   always@(a[0], a[1], a[2], a[3], a[4], a[5], a[6])
     begin
	selected_row = {a[6],a[5],a[4], a[3], a[2], a[1], a[0]};
	out = Truth_table[selected_row];
     end
     
endmodule

//D-FlipFlop module with synchronous active low clear and preset.
module D_Flip_Flop(clock,D,clear,preset,Q);

input clock,D,clear,preset;
output reg Q = 1'b0;

specify
	(clock => Q)="";
endspecify

   initial
     begin
	Q <= 1'b0;
     end
   
   always@(posedge clock)
     begin
	if(clear==0)
	  Q<=0;
	else if(preset==0)
	  Q<=1;
	else
	  begin
	     Q<=D;
	  end
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

module ripple_adder(a, b, cin, cout, sumout);
   
   parameter width = 0;

   input [width-1:0] a, b;
   input 	     cin;
   
   output [width-1:0] sumout;
   output 	      cout;

   specify
      (a=>sumout)="";
      (b=>sumout)="";
      (cin=>sumout)="";
      (a=>cout)="";
      (b=>cout)="";
      (cin=>cout)="";
   endspecify
   
   assign {cout, sumout} = a + b + cin;
   
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
