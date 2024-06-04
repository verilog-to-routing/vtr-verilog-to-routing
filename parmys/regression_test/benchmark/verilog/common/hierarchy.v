module hierarchy (
	input   [2:0] a,
	output  [2:0] w
);

	first_module uut1 (a, w);
	
endmodule

module first_module (
	input   [2:0] a,
	output  [2:0] w
);

	second_module uut2 (a, w);
	
endmodule

module second_module (
	input   [2:0] a,
	output  [2:0] w
);

	reg [2:0] operand = 1;

	assign w = a + operand;
	
endmodule

