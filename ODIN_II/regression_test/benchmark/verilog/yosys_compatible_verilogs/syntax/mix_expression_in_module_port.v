`define WIDTH 4

module top_module (rst, x, y, q);
	input 	rst;
    input  	[`WIDTH-1:0] x; 
    input   [`WIDTH-1:0] y;
    output	[2*`WIDTH-1:0] q;

	second_module my_module (.a({{2{x[1]}}, y[3]}), 
							 .b({rst, x[2]|~y[2], (rst ? 2'b00 : {x[1], y[1]})}), 
							 .so(q)
							);

endmodule

module second_module (a, b, so);

    input	[`WIDTH-1:0] a;
    input	[`WIDTH-1:0] b;
    output	[2*`WIDTH-1:0] so;

    assign so = a + b;
	
endmodule