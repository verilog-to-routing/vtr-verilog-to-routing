module bitwise_not (
	input   [3:0] x_top, 
	output  [3:0] q_top
);

	assign q_top = ~x_top;

endmodule
