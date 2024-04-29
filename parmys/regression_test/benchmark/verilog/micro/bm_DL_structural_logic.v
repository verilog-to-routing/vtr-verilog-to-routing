/* Simple combinational circuit with defined gates */

module bm_DL_structural_logic (x1, x2, x3, f);
	input x1, x2, x3;
	output f;
	wire k, g, h;
	
	and (g, x1, x2);
	not (k,x2);
	and (h, k, x3);
	or (f, g, h);

endmodule
