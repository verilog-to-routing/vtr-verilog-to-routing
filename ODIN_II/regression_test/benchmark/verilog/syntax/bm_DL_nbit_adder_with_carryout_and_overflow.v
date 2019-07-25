/* This is an nbit adder using parameter and over flow and carry out signals */

module bm_DL_nbit_adder_with_carryout_and_overflow (carryin, X, Y, S, carryout, overflow);
	parameter n = 6'b100000;
	input carryin;
	input [n-1:0] X, Y;
	output [n-1:0] S;
	output carryout, overflow;
	reg [n-1:0] S;
	reg carryout, overflow;
	
	always @(X or Y or carryin)
	begin
		S = X + Y + carryin;
		carryout = (X[n-1] & Y[n-1]) | (X[n-1] & ~S[n-1]) | (Y[n-1] & ~S[n-1]);
		overflow = carryout ^ X[n-1] ^ Y[n-1] ^ S[n-1];
	end

endmodule
