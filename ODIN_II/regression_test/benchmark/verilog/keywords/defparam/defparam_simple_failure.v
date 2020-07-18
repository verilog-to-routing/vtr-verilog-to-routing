module simple_op(in1,out1);
	parameter width1 = 4;
	
	input  [width1-1:0] in1;
	output [width1-1:0] out1;
	
	assg m1 (in1,out1);
	
endmodule
	
module assg(in,out);
	localparam width = 3;
		
	input  [width-1:0] in;
	output [width-1:0] out;
		
	assign out = in;
endmodule 
	
module params;
    defparam
		simple_op.m1.width = 4;
endmodule 