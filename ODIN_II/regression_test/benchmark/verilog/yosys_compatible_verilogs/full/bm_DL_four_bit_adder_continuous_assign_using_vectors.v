/* 4 bit adder with vector assignment so we can have width variables */

module bm_DL_four_bit_adder_continuous_assign_using_vectors (carryin, X, Y, S, carryout);
	input carryin;
	input [3:0] X, Y;
	output [3:0] S;
	output carryout;
	wire [3:1] C;
	
	fulladd stage0 (carryin, X[0], Y[0], S[0], C[1]);
	fulladd stage1 (C[1], X[1], Y[1], S[1], C[2]);
	fulladd stage2 (C[2], X[2], Y[2], S[2], C[3]);
	fulladd stage3 (C[3], X[3], Y[3], S[3], carryout);
		
endmodule

module fulladd (Cin, x, y, s, Cout);
	input Cin, x, y;
	output s, Cout;
	
	assign s = x ^ y ^ Cin;
	assign Cout = (x & y) | (x & Cin) | (y & Cin);
		    
endmodule
