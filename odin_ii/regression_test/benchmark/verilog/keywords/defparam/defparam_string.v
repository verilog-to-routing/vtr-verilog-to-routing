module simple_op(clk,out);

	parameter msg = "This is the original message";
	
	input clk;
	output reg out;
	
	always @(negedge clk) begin
	
	 out = clk;
	
	$display("%s",msg);
	
	end
	
endmodule 

module param();

	defparam simple_op.msg = "This is the new message";
endmodule