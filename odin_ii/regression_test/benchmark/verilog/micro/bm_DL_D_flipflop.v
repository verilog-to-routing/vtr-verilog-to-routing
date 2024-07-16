/* pg 389 - D flip flop */

module bm_DL_D_flipflop(D, Clock, Q);
	input D, Clock;
	output Q;
	reg Q;
	
	always @(posedge Clock)
		Q <= D;
	
endmodule
