/*
 * Copyright 2023 CAS—Atlantic (University of New Brunswick, CASA)
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
 
`timescale 1ps/1ps

`define MEM_MAXADDR PPP
`define MEM_MAXDATA 36

// depth and data may need to be splited
module dualPortRam(clk, we1, we2, addr1, addr2, data1, data2, out1, out2);
    parameter ADDR_WIDTH = `MEM_MAXADDR;
    parameter DATA_WIDTH = 1;

    input clk;
    input we1, we2;
    input [ADDR_WIDTH-1:0] addr1, addr2;
    input [DATA_WIDTH-1:0] data1, data2;

    output reg [DATA_WIDTH-1:0] out1, out2;


    dual_port_ram uut (
        .clk(clk), 
        .we1(we1), 
        .we2(we2), 
        .addr1(addr1), 
        .addr2(addr2), 
        .data1(data1), 
        .data2(data2), 
        .out1(out1),
        .out2(out2)
    );
    
endmodule



(* blackbox *)
module dual_port_ram(clk, data2, data1, addr2, addr1, we2, we1, out2, out1);
    localparam ADDR_WIDTH = `MEM_MAXADDR;
    localparam DATA_WIDTH = 1;

    input clk;
    input we1, we2;
    input [ADDR_WIDTH-1:0] addr1, addr2;
    input data1, data2;

    output reg out1, out2;
    /*
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
    */
endmodule