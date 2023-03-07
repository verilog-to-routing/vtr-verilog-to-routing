module and_primitive(
	a,
	b,
	out
);
	/*  input declaration	*/
	input   a;
	input   b;

	/*	output declaration	*/
	output	out;

    assign out = a&b; 

endmodule