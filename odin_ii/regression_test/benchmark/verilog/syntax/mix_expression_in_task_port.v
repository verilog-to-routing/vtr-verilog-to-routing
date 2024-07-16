`define WIDTH 4

module top_module (clk, rst, x, y, q);
	input 	clk, rst;
    input  	[`WIDTH-1:0] x; 
    input   [`WIDTH-1:0] y;
    output	[2*`WIDTH-1:0] q;

always @(posedge clk)
    begin
        my_task(.a({{2{x[1]}}, y[3]}), 
							 .b({rst, x[2]|~y[2], (rst ? 2'b00 : {x[1], y[1]})}), 
							 .so(q)
							);
    end

task my_task(
        input  [`WIDTH-1:0] a,
        input  [`WIDTH-1:0] b,
        output [2*`WIDTH-1:0] so
        );

        so <= a + b;  

    endtask


endmodule