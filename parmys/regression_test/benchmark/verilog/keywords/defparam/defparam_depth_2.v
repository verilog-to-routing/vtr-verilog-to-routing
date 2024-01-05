module simple_op(in1,in2,out1,out2);
	parameter width1 = 2;
	parameter width2 = 8;
	
	input  [width1-1:0] in1;
	input  [width2-1:0] in2;
	output [width1-1:0] out1;
	output [width2-1:0] out2;
	
	defparam m1.width = 2;
	defparam m2.width = 8;
	defparam m1.g1.width = 2;
	defparam m2.g1.width = 8;
	
	assg1 m1 (in1,out1);
	assg1 m2 (in2,out2);
	
endmodule
	
module assg1(in,out);
	parameter width = 3;
		
	input  [width-1:0] in;
	output [width-1:0] out;
		
	assg2 g1 (in,out);
	
endmodule 

module assg2(in,out);
	parameter width = 2;
	
	input  [width-1:0] in;
	output [width-1:0] out;
		
	assign out = in;
	
endmodule 