// behavioral whiteboxes for frankenstein verilator random-check.
// used when elaborating yosys-converted post-synth / post-abc blifs that
// still contain .subckt adder|multiply|*_ram|dff*.  no specify blocks
// (verilator-safe).  rams init to 0 — functional smoke only.

`timescale 1ns/1ps

module adder #(
    parameter WIDTH = 1
) (
    input [WIDTH-1:0] a,
    input [WIDTH-1:0] b,
    input cin,
    output cout,
    output [WIDTH-1:0] sumout
);
    assign {cout, sumout} = a + b + cin;
endmodule

module multiply #(
    parameter WIDTH = 36
) (
    input [WIDTH-1:0] a,
    input [WIDTH-1:0] b,
    output [2*WIDTH-1:0] out
);
    assign out = a * b;
endmodule

module single_port_ram #(
    parameter ADDR_WIDTH = 15,
    parameter DATA_WIDTH = 1
) (
    input clk,
    input [ADDR_WIDTH-1:0] addr,
    input [DATA_WIDTH-1:0] data,
    input we,
    output reg [DATA_WIDTH-1:0] out
);
    localparam DEPTH = 1 << ADDR_WIDTH;
    reg [DATA_WIDTH-1:0] mem [0:DEPTH-1];
    integer i;
    initial begin
        for (i = 0; i < DEPTH; i = i + 1)
            mem[i] = {DATA_WIDTH{1'b0}};
        out = {DATA_WIDTH{1'b0}};
    end
    always @(posedge clk) begin
        if (we)
            mem[addr] <= data;
        out <= mem[addr];
    end
endmodule

module dual_port_ram #(
    parameter ADDR_WIDTH = 15,
    parameter DATA_WIDTH = 1
) (
    input clk,
    input [ADDR_WIDTH-1:0] addr1,
    input [ADDR_WIDTH-1:0] addr2,
    input [DATA_WIDTH-1:0] data1,
    input [DATA_WIDTH-1:0] data2,
    input we1,
    input we2,
    output reg [DATA_WIDTH-1:0] out1,
    output reg [DATA_WIDTH-1:0] out2
);
    localparam DEPTH = 1 << ADDR_WIDTH;
    reg [DATA_WIDTH-1:0] mem [0:DEPTH-1];
    integer i;
    initial begin
        for (i = 0; i < DEPTH; i = i + 1)
            mem[i] = {DATA_WIDTH{1'b0}};
        out1 = {DATA_WIDTH{1'b0}};
        out2 = {DATA_WIDTH{1'b0}};
    end
    always @(posedge clk) begin
        if (we1)
            mem[addr1] <= data1;
        if (we2)
            mem[addr2] <= data2;
        out1 <= mem[addr1];
        out2 <= mem[addr2];
    end
endmodule

module mux (
    input select,
    input x,
    input y,
    output z
);
    assign z = select ? y : x;
endmodule

module fpga_interconnect (
    input datain,
    output dataout
);
    assign dataout = datain;
endmodule

// vpr / yosys flop aliases commonly seen in frankenstein blifs
module dff (
    input clk,
    input D,
    output reg Q
);
    initial Q = 1'b0;
    always @(posedge clk)
        Q <= D;
endmodule

module dffl (
    input clk,
    input D,
    input L,
    output reg Q
);
    initial Q = 1'b0;
    always @(posedge clk) begin
        if (~L)
            Q <= 1'b0;
        else
            Q <= D;
    end
endmodule

module dffe (
    input clk,
    input D,
    input E,
    output reg Q
);
    initial Q = 1'b0;
    always @(posedge clk) begin
        if (E)
            Q <= D;
    end
endmodule

// yosys latch form sometimes survives as .latch → converted to latch cell
module latch (
    input D,
    input G,
    output reg Q
);
    initial Q = 1'b0;
    always @(*) begin
        if (G)
            Q = D;
    end
endmodule
