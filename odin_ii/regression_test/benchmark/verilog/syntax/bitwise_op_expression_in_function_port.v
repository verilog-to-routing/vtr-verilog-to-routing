`define WIDTH 4

module top_module (rst1, rst2, x, y, q);

	input	rst1, rst2;
    input  [`WIDTH-1:0] x;
    input  [`WIDTH-1:0] y;
    output [`WIDTH-1:0] q;


    //range of function xor_them returns a wire
    function [3:0] my_func;
	input	reset;
	input	[3:0] a;
	input	[3:0] b;

 	begin
		my_func = (reset ? a : a << 1) + b;
	end
    endfunction

    assign q = my_func(.reset (rst1 | rst2), .a(x), .b(y - 2));

endmodule