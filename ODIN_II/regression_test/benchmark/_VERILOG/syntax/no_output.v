module and_primitive(
	a, b
);
	/*  input declaration	*/
	input   a;
	input   b;

	/*	output declaration	*/
	wire	out;

    assign out = a&b; 

endmodule