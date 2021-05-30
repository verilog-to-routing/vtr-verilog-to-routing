module mult (
	input   [15:0] x, 
	input   [15:0] y,
	output  [15:0] q
);

	assign q = x * y;

endmodule
