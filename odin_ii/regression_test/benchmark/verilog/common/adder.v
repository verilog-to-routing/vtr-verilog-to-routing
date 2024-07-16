module add (
	input 	 rst,
	input   [1:0] x_top, 
	input   [1:0] y_top,
	output  [1:0] q_top
);

	wire [1:0] q_temp;

	add_add_module uut (x_top, y_top, q_temp);
	assign q_top = (rst) ? 2'd0 : q_temp;
endmodule

module add_module (
	input   [1:0] x_add2, 
	input   [1:0] y_add2,
	output  [1:0] q_add2
);

	assign q_add2 = x_add2 + y_add2;
	
endmodule
