module mult_nxn (clk, a, b, c);

parameter WIDTH1 = 6;
parameter WIDTH2 = 6;

input clk;

input [WIDTH1-1:0] a;
input [WIDTH2-1:0] b;

output [WIDTH1+WIDTH2-1:0] c;

reg [WIDTH1-1:0] a_reg;
reg [WIDTH2-1:0] b_reg;
reg [WIDTH1+WIDTH2-1:0] c_reg;

always @(posedge clk) begin

		a_reg <= a;
		b_reg <= b;
		c_reg <= a_reg * b_reg;

end

assign c = c_reg;

endmodule
