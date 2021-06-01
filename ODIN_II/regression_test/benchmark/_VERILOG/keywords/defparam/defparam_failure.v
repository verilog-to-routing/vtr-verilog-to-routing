module simple_op(in1,in2,out1,out2);
	parameter width1 = 3;
	parameter width2 = 4;
	
	input  [width1-1:0] in1;
	input  [width2-1:0] in2;
	output [width1-1:0] out1;
	output [width2-1:0] out2;
	
	assg m1 (in1,out1);
	assg m2 (in2,out2);
	
endmodule
	
module assg(in,out);
	localparam width = 3;
		
	input  [width-1:0] in;
	output [width-1:0] out;
		
	assign out = in;
endmodule 
	
module params;
    defparam
		simple_op.m1.width = 2,
		simple_op.m2.width = 8,
		simple_op.width1 = 2,
		simple_op.width2 = 8;
endmodule 