`define WIDTH 4

module top_module (rst, x, y, q);

	input	rst;
	input	[`WIDTH-1:0] x; 
	input	[`WIDTH-1:0] y;
	output	[`WIDTH-1:0] q;    

	second_module my_module (.a(rst ? x : x<<1), .b(y-2), .so(q));

endmodule

module second_module (a, b, so);

	input [`WIDTH-1:0] a;
	input [`WIDTH-1:0] b;
	output [`WIDTH-1:0] so;

	assign so = a + b;
	
endmodule