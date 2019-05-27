module mux_case(out,cntrl);

	input [1:0]cntrl;

	output out;

	reg out;
 
always @ (*)
	case (cntrl)

		2'b00 : out = 0;
		2'b01 : out = 1;
		2'b10 : out = 0;
		2'b11 : out = 1;

	endcase

endmodule
