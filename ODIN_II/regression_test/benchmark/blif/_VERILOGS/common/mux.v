module mux (
	input 	 rst,
	input   [1:0] x_top, 
	input   [1:0] y_top,
	output  [1:0] q_top
);

	assign q_top = (rst) ? x_top : y_top;

endmodule
