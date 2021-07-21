/* simplified adder by joining S and carryout */

module bm_DL_nbit_adder_with_carryout_and_overflow_simplified (carryin, X, Y, S, carryout, overflow);
	parameter n = 6'b100000;
	input carryin;
	input [n-1:0] X, Y;
	output [n-1:0] S;
	output carryout, overflow;
	reg [n-1:0] S;
	reg carryout, overflow;
	
	always @(X or Y or carryin)
	begin
		{carryout, S} = X + Y + carryin;
		overflow = carryout ^ X[n-1] ^ Y[n-1] ^ S[n-1];
	end

endmodule
