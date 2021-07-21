module top(input clock, in, output reg out);
	always @(posedge clock)
		out <= in;
		
endmodule
