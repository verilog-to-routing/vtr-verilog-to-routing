module mux_case(out,
	cntrl
);

	input [1:0]cntrl;

	output out;

	reg out;
 
always @ (*)
	case (cntrl)

		2'b00 : assign out = 0;
		2'b01 : assign out = 1;
		2'b10 : assign out = 0;

		2'b11 : deassign out;

	endcase

endmodule
