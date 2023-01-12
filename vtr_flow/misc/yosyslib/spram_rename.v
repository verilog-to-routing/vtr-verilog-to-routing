/*********************************************************************
# Description: Renaming depth-split singlePortRam to single_port_ram #
#			   to be recognized by VTR flow CAD tools. This file  	 #
#			   is executed by the Yosys synthesis flow once the      #
#			   single_port_ram.v is executed.		  	  			 #
#																  	 #
# Author: Seyed Alireza Damghani (sdamghann@gmail.com)   		 	 #
*********************************************************************/

`timescale 1ps/1ps

`define MEM_MAXADDR PPP
`define MEM_MAXDATA 36

// depth and data may need to be splited
module singlePortRam(clk, we, addr, data, out);

    parameter ADDR_WIDTH = `MEM_MAXADDR;
    parameter DATA_WIDTH = 1;

    input clk;
    input we;
    input [ADDR_WIDTH - 1:0] addr;
    input [DATA_WIDTH - 1:0] data;
    
    output reg [DATA_WIDTH - 1:0] out;    

	single_port_ram uut (
                    .clk(clk), 
                    .we(we), 
                    .addr(addr), 
                    .data(data), 
                    .out(out)
                );

endmodule

(* blackbox *)
module single_port_ram(clk, data, addr, we, out);

    localparam ADDR_WIDTH = `MEM_MAXADDR;
    localparam DATA_WIDTH = 1;

    input clk;
    input we;
    input [ADDR_WIDTH-1:0] addr;
    input [DATA_WIDTH-1:0] data;
    
    output reg [DATA_WIDTH-1:0] out;
    /*
    reg [DATA_WIDTH-1:0] RAM [(1<<ADDR_WIDTH)-1:0];

    always @(posedge clk)
    begin
        if (we)
                RAM[addr] <= data;
                
        out <= RAM[addr];
    end
    */
endmodule