/* pg 323 - 4 to 1 multiplexer */

module bm_DL_4_1_mux (w0, w1, w2, w3, S, f);
	input w0, w1, w2, w3;
	input [1:0] S;
	output f;

	assign f = S[1] ? (S[0] ? w3 : w2) : (S[0] ? w1 : w0);
		
endmodule
