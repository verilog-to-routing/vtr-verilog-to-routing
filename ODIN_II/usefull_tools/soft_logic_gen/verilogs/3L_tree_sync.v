`define ADDER_WIDTH %%ADDER_BITS%%

module adder_tree_top (
	clk,
	isum0_0_0, isum0_0_1, isum0_1_0, isum0_1_1, isum1_0_0, isum1_0_1, isum1_1_0, isum1_1_1,
	osum,
);
	input clk;
	input [`ADDER_WIDTH		:0] isum0_0_0, isum0_0_1, isum0_1_0, isum0_1_1, isum1_0_0, isum1_0_1, isum1_1_0, isum1_1_1;
	output [`ADDER_WIDTH  :0] osum;
	reg [`ADDER_WIDTH		:0] sum;
	wire [`ADDER_WIDTH		:0] sum0, sum1;
	wire [`ADDER_WIDTH		:0] sum0_0, sum0_1, sum1_0, sum1_1;
	reg [`ADDER_WIDTH		:0] sum0_0_0, sum0_0_1, sum0_1_0, sum0_1_1, sum1_0_0, sum1_0_1, sum1_1_0, sum1_1_1;

	adder_tree_branch L1_0(sum0,     sum1,     sum    );
	adder_tree_branch L2_0(sum0_0,   sum0_1,   sum0  );
	adder_tree_branch L2_1(sum1_0,   sum1_1,   sum1  );
	adder_tree_branch L3_0(sum0_0_0, sum0_0_1, sum0_0);
	adder_tree_branch L3_1(sum0_1_0, sum0_1_1, sum0_1);
	adder_tree_branch L3_2(sum1_0_0, sum1_0_1, sum1_0);
	adder_tree_branch L3_3(sum1_1_0, sum1_1_1, sum1_1);


	always @(posedge clk) begin
		sum0_0_0 <= isum0_0_0;
		sum0_0_1 <= isum0_0_1;
		sum0_1_0 <= isum0_1_0;
		sum0_1_1 <= isum0_1_1;
		sum1_0_0 <= isum1_0_0;
		sum1_0_1 <= isum1_0_1;
		sum1_1_0 <= isum1_1_0;
		sum1_1_1 <= isum1_1_1;
		osum <= sum;
	end

endmodule

module adder_tree_branch(a,b,sum);
	input [`ADDER_WIDTH:0] a;
	input [`ADDER_WIDTH:0] b;
	output [`ADDER_WIDTH:0] sum;

	assign sum = a + b;
endmodule
