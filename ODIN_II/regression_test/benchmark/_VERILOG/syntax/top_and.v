`define WIDTH 4
module top_and(
	a,
	b,
	clk,
	out
);
	/*  input declaration	*/
	input   [`WIDTH-1:0]	a;
	input   [`WIDTH-1:0]	b;
	input   clk;

	/*	output declaration	*/
	output	[`WIDTH-1:0]	out;

	/*	intermediate wires	*/
	wire	[`WIDTH-1:0]	tmp;

	and_primitive and1(a[0], b[0], tmp[0]);
	and_primitive and2(a[1], b[1], tmp[1]);
	and_primitive and3(a[2], b[2], tmp[2]);
	and_primitive and4(a[3], b[3], tmp[3]);

	always@(posedge clk)
		out <= tmp;

endmodule