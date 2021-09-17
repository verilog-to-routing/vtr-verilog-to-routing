`include "./rom_bb.v"
`include "./bram_bb.v"
/*
 * Copyright (c) 2021 Seyed Alireza Damghani (sdamghann@gmail.com)   
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
 *                                                                   
 * The library contains the implementation of block memory used      
 * to be replaced with yosys mem block that has only both read and    
 * write accesses. Specified paramateres are compatible with yosys   
 * infrastructure to avoid complaining.The bram block then maps and  
 * splits into VTR memory primitives using Odin-II.                  
 *                                                                   
*/




(* techmap_celltype = "$mem" *)
module memmap (RD_CLK, RD_EN, RD_ADDR, RD_DATA, WR_CLK, WR_EN, WR_ADDR, WR_DATA);

    parameter MEMID = "";
    parameter signed OFFSET = 0;
    parameter signed ABITS = 1;
    parameter signed WIDTH = 1;
    parameter signed SIZE = 2 ** ABITS;
    parameter signed INIT = 1'bx;

    parameter signed RD_PORTS = 1;
    parameter RD_CLK_ENABLE = 1'b1;
    parameter RD_CLK_POLARITY = 1'b1;
    parameter RD_TRANSPARENT = 1'b1;

    parameter signed WR_PORTS = 1;
    parameter WR_CLK_ENABLE = 1'b1;
    parameter WR_CLK_POLARITY = 1'b1;

    /* read input */
    input RD_CLK;
    input [RD_PORTS-1:0] RD_EN;
    input [RD_PORTS*ABITS-1:0] RD_ADDR;
    /* write input*/
    input WR_CLK;
    input [WR_PORTS*WIDTH-1:0] WR_EN;
    input [WR_PORTS*ABITS-1:0] WR_ADDR;
    input [WR_PORTS*WIDTH-1:0] WR_DATA;
    /* read output */
    output reg [RD_PORTS*WIDTH-1:0] RD_DATA;

    wire [WR_PORTS-1:0] WR_ENABLES;

    genvar i;
    generate
        if (WR_PORTS == 0) begin
            _$ROM               #(  .MEMID        (MEMID),
                                    .OFFSET       (OFFSET),
                                    .ABITS        (ABITS),
                                    .WIDTH        (WIDTH),
                                    .SIZE         (SIZE),
                                    .INIT         (INIT),
                                    .CLK_ENABLE   (RD_CLK_ENABLE),
                                    .RD_PORTS     (RD_PORTS)
            ) _TECHMAP_REPLACE_ (
                                    .CLK(RD_CLK),
                                    .RD_EN(RD_EN),
                                    .RD_ADDR(RD_ADDR),
                                    .RD_DATA(RD_DATA));
        end else begin
            for (i = 0; i < WR_PORTS ; i = i+1) begin
                assign WR_ENABLES[i] = WR_EN[i*WIDTH];
            end

            _$BRAM              #(  .MEMID        (MEMID),
                                    .OFFSET       (OFFSET),
                                    .ABITS        (ABITS),
                                    .WIDTH        (WIDTH),
                                    .SIZE         (SIZE),
                                    .INIT         (INIT),
                                    .CLK_ENABLE   (WR_CLK_ENABLE),
                                    .RD_PORTS     (RD_PORTS),
                                    .WR_PORTS     (WR_PORTS)
            ) _TECHMAP_REPLACE_ (
                                    .CLK(WR_CLK),
                                    .RD_EN(RD_EN),
                                    .RD_ADDR(RD_ADDR),
                                    .RD_DATA(RD_DATA),
                                    .WR_EN(WR_ENABLES),
                                    .WR_ADDR(WR_ADDR),
                                    .WR_DATA(WR_DATA));
        end
    endgenerate
endmodule
