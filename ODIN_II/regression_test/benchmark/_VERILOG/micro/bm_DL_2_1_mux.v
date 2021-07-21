/* pg 322 2 to 1 multiplexer */

module bm_DL_2_1_mux  (w0, w1, s, f);
	input w0, w1, s;
	output f;
	
	assign f = s ? w1 : w0;
		
endmodule
