`timescale 1ps/1ps

module LUT_K #(
    //The Look-up Table size (number of inputs)
    parameter K, 

    //The lut mask.  
    //Left-most (MSB) bit corresponds to all inputs logic one. 
    //Defaults to always false.
    parameter LUT_MASK={2**K{1'b0}} 
) (
    input [K-1:0] in,
    output out
);

    specify
        (in => out) = "";
    endspecify

    assign out = LUT_MASK[in];

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
        $hold(posedge clock, D, "");
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
    parameter WIDTH = 1   
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
    parameter WIDTH = 1
) (
    input [WIDTH-1:0] a,
    input [WIDTH-1:0] b,
    output [2*WIDTH-1:0] out
);

    specify
        (a => out) = "";
        (b => out) = "";
    endspecify

    assign out = a * b;

endmodule // mult

//single_port_ram module
module single_port_ram #(
    parameter ADDR_WIDTH = 1,
    parameter DATA_WIDTH = 1
) (
    input [ADDR_WIDTH-1:0] addr,
    input [DATA_WIDTH-1:0] data,
    input we,
    input clock,
    output reg [DATA_WIDTH-1:0] out
);

    localparam MEM_DEPTH = 2 ** ADDR_WIDTH;

    reg [DATA_WIDTH-1:0] Mem[MEM_DEPTH-1:0];

    specify
        (clock=>out)="";
        $setup(addr, posedge clock, "");
        $setup(data, posedge clock, "");
        $setup(we, posedge clock, "");
        $hold(posedge clock, addr, "");
        $hold(posedge clock, data, "");
        $hold(posedge clock, we, "");
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
    parameter ADDR_WIDTH = 1,
    parameter DATA_WIDTH = 1
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

    localparam MEM_DEPTH = 2 ** ADDR_WIDTH;

    reg [DATA_WIDTH-1:0] Mem[MEM_DEPTH-1:0];

    specify
        (clock=>out1)="";
        (clock=>out2)="";
        $setup(addr1, posedge clock, "");
        $setup(addr2, posedge clock, "");
        $setup(data1, posedge clock, "");
        $setup(data2, posedge clock, "");
        $setup(we1, posedge clock, "");
        $setup(we2, posedge clock, "");
        $hold(posedge clock, addr1, "");
        $hold(posedge clock, addr2, "");
        $hold(posedge clock, data1, "");
        $hold(posedge clock, data2, "");
        $hold(posedge clock, we1, "");
        $hold(posedge clock, we2, "");
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
