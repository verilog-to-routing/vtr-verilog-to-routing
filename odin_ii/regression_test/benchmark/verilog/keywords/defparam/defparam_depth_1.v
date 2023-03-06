module simple_op(in1,in2,out1,out2);
	parameter width = 2;
	parameter width2 = 8;
	
	input  [width-1:0] in1;
	input  [width2-1:0] in2;
	output [width-1:0] out1;
	output [width2-1:0] out2;
	
	defparam m1.width = width;
	defparam m2.width = width2;
	
	assg m1 (in1,out1);
	assg m2 (in2,out2);
	
endmodule
	
module assg(in,out);
	parameter width = 3;
		
	input  [width-1:0] in;
	output [width-1:0] out;
		
	assign out = in;
endmodule 