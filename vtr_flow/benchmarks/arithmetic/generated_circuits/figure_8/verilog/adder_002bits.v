`define ADDER_WIDTH 002
`define DUMMY_WIDTH 128

module adder_top (
	clk,
	a,
	b,
	sum,
	// dummy
);
	input clk;
	input  [`ADDER_WIDTH-1:0] a;
	input  [`ADDER_WIDTH-1:0] b;
	output [`ADDER_WIDTH  :0] sum;
	reg    [`ADDER_WIDTH  :0] sum;
	// output [`DUMMY_WIDTH-1:0] dummy;
	// reg [`DUMMY_WIDTH-1:0] dummy;

	reg [`ADDER_WIDTH-1:0] a_reg;
	reg [`ADDER_WIDTH-1:0] b_reg;

	always @(posedge clk) begin
		a_reg <= a;
		b_reg <= b;
		sum <= a_reg + b_reg;
	end

	// assign dummy = dummy;

	// always @(posedge clk) begin
	// 	dummy[0] <= a[0];
	// 	dummy[`DUMMY_WIDTH-1:1] <= dummy[`DUMMY_WIDTH-2:0];
	// end
endmodule