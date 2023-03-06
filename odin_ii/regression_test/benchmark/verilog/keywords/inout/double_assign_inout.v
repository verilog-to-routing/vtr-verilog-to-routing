module simple_op(in_out1,in_out2,en);
	inout in_out1;
	inout in_out2;
	input  en;

	assign in_out1 = (en) ? in_out2 : 1'b1;
	assign in_out2 = (!en) ? in_out1 : 1'b0;
endmodule 