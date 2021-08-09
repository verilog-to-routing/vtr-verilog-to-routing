module simple_op(in,out);
	input [31:0] in;
	output [31:0] out;
	
    always begin
	factorial(in,out);
    end 

	task automatic integer  factorial;
		input [31:0] in;
        output [31:0] out;
		if(in>=2)
			factorial(in-1,out * in);
		else
			out = 1;
	endfunction
		
endmodule 	