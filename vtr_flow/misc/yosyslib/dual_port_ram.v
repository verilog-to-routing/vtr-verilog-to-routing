`timescale 1ps/1ps

module dual_port_ram(clk, we1, we2, addr1, addr2, data1, data2, out1, out2);
    parameter ADDR_WIDTH = 15;
    parameter DATA_WIDTH = 1;

    input clk;
    input we1, we2;
    input [ADDR_WIDTH-1:0] addr1, addr2;
    input [DATA_WIDTH-1:0] data1, data2;

    output reg [DATA_WIDTH-1:0] out1, out2;
    reg [DATA_WIDTH-1:0] RAM [(1<<ADDR_WIDTH)-1:0];

    always @(posedge clk)
    begin
        if (we1)
                RAM[addr1] <= data1;
        if (we2)
                RAM[addr2] <= data2;
                
        out1 <= RAM[addr1];
        out2 <= RAM[addr2];
    end
endmodule
