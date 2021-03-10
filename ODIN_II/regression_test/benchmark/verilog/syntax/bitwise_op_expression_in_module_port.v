`define WIDTH 4

module top_module (rst1, rst2, x, y, q);

	input	rst1, rst2;    
	input	[`WIDTH-1:0] x; 
    input	[`WIDTH-1:0] y;
    output	[`WIDTH-1:0] q;

    second_module my_module (.reset(rst1 | rst2), .a(x), .b(y-2), .so(q));

endmodule

module second_module (reset, a, b, so);

	input	reset;
    input	[`WIDTH-1:0] a;
    input	[`WIDTH-1:0] b;
    output	[`WIDTH-1:0] so;

    assign so = (reset ? a : a << 1) + b;
	
endmodule