`define WIDTH1 4
`define WIDTH2 2

macromodule simple_op(in1,in2,in3,out1,out2,out3);
	
	input  [`WIDTH1-1:0] in1;
	input  [`WIDTH1-1:0] in2;
	input  [`WIDTH2-1:0] in3;
	
	output [`WIDTH1-1:0] out1;
	output [`WIDTH1-1:0] out2;
	output [`WIDTH2-1:0] out3;
	
	assg1  m1 (in1,out1);
	assg1  m2 (in2,out2);
	assg2  m3 (in3,out3);
	
endmodule

macromodule simple_op(in1,out1);
	
	input  [`WIDTH1-1:0] in1;
	
	output [`WIDTH1-1:0] out1;
	
	assg1  m1 (in1,out1);
	
endmodule 


module assg1(in,out);	
	input  [3:0] in;
	output [3:0] out;
		
	assign out = in;
endmodule 

module assg2(in,out);	
	input  [1:0] in;
	output [1:0] out;
		
	assign out = in;
endmodule 