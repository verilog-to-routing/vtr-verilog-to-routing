/****************************************************************
# Description: Performing a recursive depth splitting for 		#
#			   single_port_ram memory hard blocks				#
#																#
# Author: Seyed Alireza Damghani (sdamghann@gmail.com)   		#
****************************************************************/

`timescale 1ps/1ps

`define MEM_MAXADDR PPP
`define MEM_MAXDATA 36

// depth and data may need to be splited
module single_port_ram(clk, we, addr, data, out);

    parameter ADDR_WIDTH = `MEM_MAXADDR;
    parameter DATA_WIDTH = 1;

    input clk;
    input we;
    input [ADDR_WIDTH - 1:0] addr;
    input [DATA_WIDTH - 1:0] data;
    
    output reg [DATA_WIDTH - 1:0] out;    

	genvar i;
	generate 
		// split in depth
		if (ADDR_WIDTH > `MEM_MAXADDR)
		begin
			
            wire [ADDR_WIDTH-2:0] new_addr = addr[ADDR_WIDTH-2:0];
            wire [DATA_WIDTH-1:0] out_h, out_l;


            defparam uut_h.ADDR_WIDTH = ADDR_WIDTH-1;
            defparam uut_h.DATA_WIDTH = DATA_WIDTH;
            single_port_ram uut_h (
                .clk(clk), 
                .we(we), 
                .addr(new_addr), 
                .data(data), 
                .out(out_h)
            );

            defparam uut_l.ADDR_WIDTH = ADDR_WIDTH-1;
            defparam uut_l.DATA_WIDTH = DATA_WIDTH;
            single_port_ram uut_l (
                .clk(clk), 
                .we(we), 
                .addr(new_addr), 
                .data(data), 
                .out(out_l)
            );

            reg additional_bit;
            always @(posedge clk) additional_bit <= addr[ADDR_WIDTH-1];
            assign out = (additional_bit) ? out_h : out_l;

        end	else begin
            for (i = 0; i < DATA_WIDTH; i = i + 1) begin:single_bit_data
                singlePortRam uut (
                    .clk(clk), 
                    .we(we), 
                    .addr({ {{`MEM_MAXADDR-ADDR_WIDTH}{1'bx}}, addr[ADDR_WIDTH-1:0] }), 
                    .data(data[i]), 
                    .out(out[i])
                );
            end
        end
    endgenerate

endmodule

(* blackbox *)
module singlePortRam(clk, data, addr, we, out);

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

