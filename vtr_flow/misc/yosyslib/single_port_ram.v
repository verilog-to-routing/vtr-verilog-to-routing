`timescale 1ps/1ps

module single_port_ram(clk, we, addr, data, out);

    parameter ADDR_WIDTH = 15;
    parameter DATA_WIDTH = 1;

    input clk;
    input we;
    input [ADDR_WIDTH-1:0] addr;
    input [DATA_WIDTH-1:0] data;
    
    output reg [DATA_WIDTH-1:0] out;
    reg [DATA_WIDTH-1:0] RAM [(1<<ADDR_WIDTH)-1:0];

    always @(posedge clk)
    begin
        if (we)
                RAM[addr] <= data;
                
        out <= RAM[addr];
    end
endmodule
