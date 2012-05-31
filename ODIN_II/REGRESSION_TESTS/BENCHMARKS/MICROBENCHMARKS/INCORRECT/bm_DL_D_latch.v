/* pg 389 - D latch */

module bm_DL_D_latch(D, Clk, Q);
	input D, Clk;
	output Q;
	reg Q;
	
	always @(D or Clk)
		if (Clk)
			Q = D;
endmodule
