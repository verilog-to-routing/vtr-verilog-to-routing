`define ADDER_WIDTH %%ADDER_BITS%%

module adder_tree_top (
	sum0_0_0, sum0_0_1, sum0_1_0, sum0_1_1, sum1_0_0, sum1_0_1, sum1_1_0, sum1_1_1,
	sum,
);
	input [`ADDER_WIDTH		:0] sum0_0_0, sum0_0_1, sum0_1_0, sum0_1_1, sum1_0_0, sum1_0_1, sum1_1_0, sum1_1_1;
	output [`ADDER_WIDTH  :0] sum;
	wire [`ADDER_WIDTH		:0] sum0, sum1;
	wire [`ADDER_WIDTH		:0] sum0_0, sum0_1, sum1_0, sum1_1;

	adder_tree_branch L1_0(sum0,     sum1,     sum    );
	adder_tree_branch L2_0(sum0_0,   sum0_1,   sum0  );
	adder_tree_branch L2_1(sum1_0,   sum1_1,   sum1  );
	adder_tree_branch L3_0(sum0_0_0, sum0_0_1, sum0_0);
	adder_tree_branch L3_1(sum0_1_0, sum0_1_1, sum0_1);
	adder_tree_branch L3_2(sum1_0_0, sum1_0_1, sum1_0);
	adder_tree_branch L3_3(sum1_1_0, sum1_1_1, sum1_1);

endmodule

module adder_tree_branch(a,b,sum);
	input [`ADDER_WIDTH:0] a;
	input [`ADDER_WIDTH:0] b;
	output [`ADDER_WIDTH:0] sum;

	assign sum = a + b;
endmodule
