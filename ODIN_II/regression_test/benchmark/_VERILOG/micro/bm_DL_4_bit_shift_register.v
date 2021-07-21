/* pg 401 - 4 bit shift register */

module bm_DL_4_bit_shift_register (R, L, w, Clock, Q);
	input [3:0] R;
	input L, w, Clock;
	output [3:0] Q;
	wire [3:0] Q;

	muxdff Stage3 (w, R[3], L, Clock, Q[3]);
	muxdff Stage2 (Q[3], R[2], L, Clock, Q[2]);
	muxdff Stage1 (Q[2], R[1], L, Clock, Q[1]);
	muxdff Stage0 (Q[1], R[0], L, Clock, Q[0]);

endmodule

module muxdff (D0, D1, Sel, Clock, Q);
	input D0, D1, Sel, Clock;
	output Q;
	reg Q;

	always @(posedge Clock)
	 	if (~Sel)
			Q <= D0;
		else 
			Q <= D1;
		
endmodule
