`define WIDTH 4

module top_module (rst, x, y, q);

	input rst;
    input  [`WIDTH-1:0] x;
    input  [`WIDTH-1:0] y;
    output [`WIDTH-1:0] q;


    //range of function xor_them returns a wire
    function [3:0] my_func;
	input [3:0] a;
	input [3:0] b;

 	begin
		my_func = a + b;
	end
    endfunction

    assign q = my_func(.a(rst ? x : x << 1), .b(y - 2));

endmodule