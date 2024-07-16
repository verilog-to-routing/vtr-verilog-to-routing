`define WIDTH 4

module top_module (clk, rst1, rst2, x, y, q);

    input   clk, rst1, rst2;
    input   [`WIDTH-1:0] x; 
    input   [`WIDTH-1:0] y;
    output  [`WIDTH-1:0] q;

    always @(posedge clk)
    begin
        my_task(.reset(rst1 | rst2), .a(x), .b(y - 2), .so(q));
    end

    task my_task(
		input	reset,
        input 	[`WIDTH-1:0] a,
        input 	[`WIDTH-1:0] b,
        output	[`WIDTH-1:0] so
        );

        so <= (reset ? a : a << 1) + b;  

    endtask


endmodule