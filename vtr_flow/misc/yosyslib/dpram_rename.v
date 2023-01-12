/******************************************************************
# Description: Renaming depth-split dualPortRam to dual_port_ram  #
#			   to be recognized by VTR flow CAD tools. This file  #
#			   is executed by the Yosys synthesis flow once the   #
#			   dual_port_ram.v is executed.		  	  			  #
#																  #
# Author: Seyed Alireza Damghani (sdamghann@gmail.com)   		  #
******************************************************************/


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