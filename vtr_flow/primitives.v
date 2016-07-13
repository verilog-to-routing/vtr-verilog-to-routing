`timescale 1ps/1ps

//3-Input Look Up Table module
module LUT_3 #(
   //Truth table parameter represents the default function of the LUT.
   //The most significant bit is the output when all inputs are logic one.
   parameter LUT_MASK=8'b00000000
) (
    input in_2, 
    input in_1, 
    input in_0, 
    output reg out
);

    integer selected_row;
    wire [2:0] a;

    fpga_interconnect inter0(in_0 , a[0]);
    fpga_interconnect inter1(in_1 , a[1]);
    fpga_interconnect inter2(in_2 , a[2]);

    always@(*) begin
        selected_row = {a[2], a[1], a[0]};
        out = LUT_MASK[selected_row];
    end

endmodule

//4-Input Look Up Table module
module LUT_4 #(
   //Truth table parameter represents the default function of the LUT.
   //The most significant bit is the output when all inputs are logic one.
   parameter LUT_MASK=16'b0000000000000000
) (
    input in_3,
    input in_2,
    input in_1,
    input in_0,
    output reg out
);

    integer selected_row;
    wire [3:0] a;

    fpga_interconnect inter0(in_0 , a[0]);
    fpga_interconnect inter1(in_1 , a[1]);
    fpga_interconnect inter2(in_2 , a[2]);
    fpga_interconnect inter3(in_3 , a[3]);

    always@(*) begin
        selected_row = {a[3], a[2], a[1], a[0]};
        out = LUT_MASK[selected_row];
    end
     
endmodule

//5-Input Look Up Table module
module LUT_5 #(
   //Truth table parameter represents the default function of the LUT.
   //The most significant bit is the output when all inputs are logic one.
   parameter LUT_MASK=32'b00000000000000000000000000000000
) (
    input in_4,
    input in_3,
    input in_2,
    input in_1,
    input in_0,
    output reg out
);
   
    integer selected_row = 0;
    wire [4:0] a;

    fpga_interconnect inter0(in_0 , a[0]);
    fpga_interconnect inter1(in_1 , a[1]);
    fpga_interconnect inter2(in_2 , a[2]);
    fpga_interconnect inter3(in_3 , a[3]);
    fpga_interconnect inter4(in_4 , a[4]);

    always@(*) begin
        selected_row = {a[4], a[3], a[2], a[1], a[0]};
        out = LUT_MASK[selected_row];
    end
     
endmodule

//6-Input Look Up Table module
module LUT_6 #(
   //Truth table parameter represents the default function of the LUT.
   //The most significant bit is the output when all inputs are logic one.
   parameter LUT_MASK=64'b0000000000000000000000000000000000000000000000000000000000000000
) (
    input in_5,
    input in_4,
    input in_3,
    input in_2,
    input in_1,
    input in_0,
    output reg out
);
    integer selected_row;
    wire [5:0] a;

    fpga_interconnect inter0(in_0 , a[0]);
    fpga_interconnect inter1(in_1 , a[1]);
    fpga_interconnect inter2(in_2 , a[2]);
    fpga_interconnect inter3(in_3 , a[3]);
    fpga_interconnect inter4(in_4 , a[4]);
    fpga_interconnect inter5(in_5 , a[5]);

    always@(*) begin
        selected_row = {a[5], a[4], a[3], a[2], a[1], a[0]};
        out = LUT_MASK[selected_row];
    end

endmodule

//7-Input Look Up Table module
module LUT_7 #(
   //Truth table parameter represents the default function of the LUT.
   //The most significant bit is the output when all inputs are logic one.
   parameter LUT_MASK=128'b00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
) (
    input in_6,
    input in_5,
    input in_4,
    input in_3,
    input in_2,
    input in_1,
    input in_0,
    output reg out
);
    integer selected_row;
    wire [6:0] a;

    fpga_interconnect inter0(in_0 , a[0]);
    fpga_interconnect inter1(in_1 , a[1]);
    fpga_interconnect inter2(in_2 , a[2]);
    fpga_interconnect inter3(in_3 , a[3]);
    fpga_interconnect inter4(in_4 , a[4]);
    fpga_interconnect inter5(in_5 , a[5]);
    fpga_interconnect inter6(in_6 , a[6]);

    always@(*) begin
        selected_row = {a[6],a[5],a[4], a[3], a[2], a[1], a[0]};
        out = LUT_MASK[selected_row];
    end

endmodule

//D-FlipFlop module
module DFF #(
    parameter INITIAL_VALUE=1'b0    
) (
    input clock,
    input D,
    output reg Q
);

    specify
        (clock => Q) = "";
        $setup(D, posedge clock, "");
    endspecify

    initial begin
        Q <= INITIAL_VALUE;
    end

    always@(posedge clock) begin
        Q <= D;
    end
endmodule

//Routing fpga_interconnect module
module fpga_interconnect(
    input datain,
    output dataout
);

    specify
        (datain=>dataout)="";
    endspecify

    assign dataout = datain;

endmodule


//2-to-1 mux module
module mux(
    input select,
    input x,
    input y,
    output z
);

    assign z = (x & ~select) | (y & select);

endmodule

module adder #(
    parameter WIDTH = 0   
) (
    input [WIDTH-1:0] a, 
    input [WIDTH-1:0] b, 
    input cin, 
    output cout, 
    output [WIDTH-1:0] sumout);

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
module mult #(
    //The width of input signals
    parameter WIDTH = 0
) (
    input [WIDTH-1:0] a,
    input [WIDTH-1:0] b,
    output [2*WIDTH-1:0] out
);

    specify
        (a => out) = ""
        (b => out) = ""
    endspecify

    assign out = a * b;

endmodule // mult

//single_port_ram module
module single_port_ram #(
    parameter ADDR_WIDTH = 0,
    parameter DATA_WIDTH = 0
) (
    input [ADDR_WIDTH-1:0] addr,
    input [DATA_WIDTH-1:0] data,
    input we,
    input clock,
    output reg [DATA_WIDTH-1:0] out
);

    localparam MEM_DEPTH = 1 << ADDR_WIDTH;

    reg [DATA_WIDTH-1:0] Mem[MEM_DEPTH-1:0];

    specify
        (clock=>out)="";
    endspecify
   
    always@(posedge clock) begin
        if(we) begin
            Mem[addr] = data;
        end
    	out = Mem[addr]; //New data read-during write behaviour (blocking assignments)
    end
   
endmodule // single_port_RAM

//dual_port_ram module
module dual_port_ram #(
    parameter ADDR_WIDTH = 0,
    parameter DATA_WIDTH = 0
) (
    input clock,

    input [ADDR_WIDTH-1:0] addr1,
    input [ADDR_WIDTH-1:0] addr2,
    input [DATA_WIDTH-1:0] data1,
    input [DATA_WIDTH-1:0] data2,
    input we1,
    input we2,
    output reg [DATA_WIDTH-1:0] out1,
    output reg [DATA_WIDTH-1:0] out2
);

    localparam MEM_DEPTH = 1 << ADDR_WIDTH;

    reg [DATA_WIDTH-1:0] Mem[MEM_DEPTH-1:0];

    specify
        (clock=>out1)="";
        (clock=>out2)="";
    endspecify
   
    always@(posedge clock) begin //Port 1
        if(we1) begin
            Mem[addr1] = data1;
        end
        out1 = Mem[addr1]; //New data read-during write behaviour (blocking assignments)
    end

    always@(posedge clock) begin //Port 2
        if(we2) begin
            Mem[addr2] = data2;
        end
        out2 = Mem[addr2]; //New data read-during write behaviour (blocking assignments)
    end
   
endmodule // dual_port_ram
