/* Simple combinational logic */

module bm_DL_structural_logic2 (x1, x2, x3, x4, f, g, h);
	input x1, x2, x3, x4;
	output f, g, h;
	wire z1, z2, z3, z4;

	and (z1, x1, x3);
	and (z2, x2, x4);
	or (g, z1, z2);
	or (z3, x1, ~x3);
	or (z4, ~x2, x4);
	and (h, z3, z4);
	or (f, g, h);

endmodule
