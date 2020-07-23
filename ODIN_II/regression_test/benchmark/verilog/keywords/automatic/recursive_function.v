
module simple_op(in,out);
	input [31:0] in;
	output [31:0] out;
	
	assign out = factorial(in);
	
	function automatic integer  factorial;
		input [31:0] in;
		if(in>=2)
			factorial = factorial(in-1) * in;
		else
			factorial = 1;
	endfunction
		
endmodule 	