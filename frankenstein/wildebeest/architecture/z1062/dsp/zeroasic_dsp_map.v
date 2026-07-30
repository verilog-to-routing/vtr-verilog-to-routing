// Map from $mult to $__MAE__
module \$__MAE__
  #(
	parameter A_SIGNED = 0,
	parameter B_SIGNED = 0,
	parameter A_WIDTH = 0,
	parameter B_WIDTH = 0,
	parameter Y_WIDTH = 0
  ) 
    (
		input [A_WIDTH-1:0] A,
		input [B_WIDTH-1:0] B,
		output [Y_WIDTH-1:0] Y
	);

	// Don't specify clock or reset yet.
	MAE #(
		.A_REG(1'b0),
		.B_REG(1'b0),
		.C_REG(1'b0),
		.P_REG(1'b0),
		.POST_ADDER_STATIC(1'b0),
		.USE_FEEDBACK(1'b0)
 	)
    _TECHMAP_REPLACE_ (
		.A(A),
		.B(B),
		.C(40'b0),
		.P(Y),
		// These must all be the same if the corresponding port is registered
		.A_ARST_N(1'b1),
		.B_ARST_N(1'b1),
		.C_ARST_N(1'b1),
		.P_ARST_N(1'b1),
		// allow packing each of these
		.ALLOW_A_REG(1'b1),
		.ALLOW_B_REG(1'b1),
		.ALLOW_P_REG(1'b1)
	);
endmodule
