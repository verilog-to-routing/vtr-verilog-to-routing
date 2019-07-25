/* Pg 325 - 16 to 1 multiplexer */

module bm_DL_16_1_mux (W, S16, f);
	input [15:0] W;
	input [3:0] S16;
	output f;
	wire [3:0] M;

	mux4to1 Mux1 (W[3:0], S16[1:0], M[0]);
	mux4to1 Mux2 (W[7:4], S16[1:0], M[1]);
	mux4to1 Mux3 (W[11:8], S16[1:0], M[2]);
	mux4to1 Mux4 (W[15:12], S16[1:0], M[3]);
	mux4to1 Mux5 (M[3:0], S16[3:2], f);
		 
endmodule

module mux4to1 (W, S, f);
	input [3:0] W;
	input [1:0] S;
	output f;
	reg f;
	
	always @(W or S)
		if (S == 2'b00)
			f = W[0];
		else if (S == 2'b01)
			f = W[1];
		else if (S == 2'b10)
			f = W[2];
		else if (S == 2'b11)
			f = W[3];
		
endmodule
