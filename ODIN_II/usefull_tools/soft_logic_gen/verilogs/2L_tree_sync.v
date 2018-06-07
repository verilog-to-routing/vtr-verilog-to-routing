`define ADDER_WIDTH %%ADDER_BITS%%
`define DUMMY_WIDTH 128

`define %%LEVELS_OF_ADDER%%_LEVEL_ADDER

module adder_tree_top (
	clk,
	isum0_0, isum0_1, isum1_0, isum1_1,
	osum,
);
	input clk;
	input [`ADDER_WIDTH		:0] isum0_0, isum0_1, isum1_0, isum1_1;
	output [`ADDER_WIDTH  :0] osum;
	reg [`ADDER_WIDTH		:0] sum;
	wire [`ADDER_WIDTH		:0] sum0, sum1;
	reg [`ADDER_WIDTH		:0] sum0_0, sum0_1, sum1_0, sum1_1;

	adder_tree_branch L1_0(sum0,     sum1,     sum   );
	adder_tree_branch L2_0(sum0_0,   sum0_1,   sum0  );
	adder_tree_branch L2_1(sum1_0,   sum1_1,   sum1  );

	always @(posedge clk) begin
		sum0_0 <= isum0_0;
		sum0_1 <= isum0_1;
		sum1_0 <= isum1_0;
		sum1_1 <= isum1_1;
		osum <= sum;
	end

endmodule

module adder_tree_branch(a,b,sum);
	input [`ADDER_WIDTH:0] a;
	input [`ADDER_WIDTH:0] b;
	output [`ADDER_WIDTH:0] sum;

	assign sum = a + b;
endmodule
