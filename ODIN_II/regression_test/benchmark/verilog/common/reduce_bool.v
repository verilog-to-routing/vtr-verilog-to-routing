module reduce_bool (
	input 	[10:0] rst,
	output  [1:0] q_top
);

	wire [1:0] q_temp;
	assign q_temp = 2'b10;

	assign q_top = (rst) ? 2'd0 : q_temp;
	

endmodule
