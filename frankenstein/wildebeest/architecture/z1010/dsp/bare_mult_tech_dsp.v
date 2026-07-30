// Zero Asic corp.
//
// Bare multiplier DSP tech mapping instantiating 'dsp_mult'.
//
module \$__dsp_block (input [17:0] A, input [17:0] B, output [39:0] Y);
	parameter A_SIGNED = 0;
	parameter B_SIGNED = 0;
	parameter A_WIDTH = 0;
	parameter B_WIDTH = 0;
	parameter Y_WIDTH = 0;

	dsp_mult 

	 _TECHMAP_REPLACE_ (
		.A(A),
		.B(B),
		.P(Y),
	);
endmodule

