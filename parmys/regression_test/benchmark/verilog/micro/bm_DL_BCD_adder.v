/* Pg 287 - BCD adder */

module bm_DL_BCD_adder (Cin, X, Y, S, Cout, Z);
	input Cin;
	input [3:0] X, Y;
	output [3:0] S;
	output Cout;
	reg [3:0] S;
	reg Cout;
	output [4:0] Z;
	reg [4:0] Z;
	
	always @(X or Y or Cin)
	begin
		Z = X + Y + Cin;
		if (Z[3:0] < 4'b1010)
			{Cout, S} = Z;
		else
			{Cout, S} = Z + 6;
	end
	
endmodule
