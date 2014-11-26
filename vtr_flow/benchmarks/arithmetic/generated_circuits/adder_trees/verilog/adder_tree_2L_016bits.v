`define ADDER_WIDTH 016
`define DUMMY_WIDTH 128

`define 2_LEVEL_ADDER

module adder_tree_top (
	clk,
	isum0_0_0_0, isum0_0_0_1, isum0_0_1_0, isum0_0_1_1, isum0_1_0_0, isum0_1_0_1, isum0_1_1_0, isum0_1_1_1,
	sum,
);
	input clk;
	input [`ADDER_WIDTH+0-1:0] isum0_0_0_0, isum0_0_0_1, isum0_0_1_0, isum0_0_1_1, isum0_1_0_0, isum0_1_0_1, isum0_1_1_0, isum0_1_1_1;
	output [`ADDER_WIDTH  :0] sum;
	reg    [`ADDER_WIDTH  :0] sum;

	wire [`ADDER_WIDTH+3-1:0] sum0;
	wire [`ADDER_WIDTH+2-1:0] sum0_0, sum0_1;
	wire [`ADDER_WIDTH+1-1:0] sum0_0_0, sum0_0_1, sum0_1_0, sum0_1_1;
	reg  [`ADDER_WIDTH+0-1:0] sum0_0_0_0, sum0_0_0_1, sum0_0_1_0, sum0_0_1_1, sum0_1_0_0, sum0_1_0_1, sum0_1_1_0, sum0_1_1_1;

	adder_tree_branch L1_0(sum0_0,     sum0_1,     sum0    );
	defparam L1_0.EXTRA_BITS = 2;

	adder_tree_branch L2_0(sum0_0_0,   sum0_0_1,   sum0_0  );
	adder_tree_branch L2_1(sum0_1_0,   sum0_1_1,   sum0_1  );
	defparam L2_0.EXTRA_BITS = 1;
	defparam L2_1.EXTRA_BITS = 1;

	adder_tree_branch L3_0(sum0_0_0_0, sum0_0_0_1, sum0_0_0);
	adder_tree_branch L3_1(sum0_0_1_0, sum0_0_1_1, sum0_0_1);
	adder_tree_branch L3_2(sum0_1_0_0, sum0_1_0_1, sum0_1_0);
	adder_tree_branch L3_3(sum0_1_1_0, sum0_1_1_1, sum0_1_1);
	defparam L3_0.EXTRA_BITS = 0;
	defparam L3_1.EXTRA_BITS = 0;
	defparam L3_2.EXTRA_BITS = 0;
	defparam L3_3.EXTRA_BITS = 0;

	always @(posedge clk) begin
		sum0_0_0_0 <= isum0_0_0_0;
		sum0_0_0_1 <= isum0_0_0_1;
		sum0_0_1_0 <= isum0_0_1_0;
		sum0_0_1_1 <= isum0_0_1_1;
		sum0_1_0_0 <= isum0_1_0_0;
		sum0_1_0_1 <= isum0_1_0_1;
		sum0_1_1_0 <= isum0_1_1_0;
		sum0_1_1_1 <= isum0_1_1_1;

		`ifdef 3_LEVEL_ADDER
			sum <= sum0;
		`endif
		`ifdef 2_LEVEL_ADDER
			sum <= sum0_0;
		`endif
	end

endmodule

module adder_tree_branch(a,b,sum);
	parameter EXTRA_BITS = 0;

	input [`ADDER_WIDTH+EXTRA_BITS-1:0] a;
	input [`ADDER_WIDTH+EXTRA_BITS-1:0] b;
	output [`ADDER_WIDTH+EXTRA_BITS:0] sum;

	assign sum = a + b;
endmodule