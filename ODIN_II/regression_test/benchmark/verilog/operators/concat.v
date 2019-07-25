
module mksteep_comp(
		clk,
	    a,
		b,
		result,
		c_s,
		c_s1
		);

input  clk;
input [7:0] a;
input [15:0] b;
output [23:0] result;

output wire [23:0] c_s;
output wire [23:0] c_s1;
wire [15:0] c_s2;


assign c_s = { (a == 8'hAA) ? 8'hAB : a,
	       (b == 16'hAAAA) ?
 		 16'hAAAB :
 		 b } ;



assign c_s2 = (b == 16'hAAAA) ? 16'hAAAB : b;
assign c_s1 = { (a == 8'hAA) ? 8'hAB : a,
	       c_s2 } ;


always @(posedge clk)
begin
	result <= (c_s == c_s1);
end

endmodule

