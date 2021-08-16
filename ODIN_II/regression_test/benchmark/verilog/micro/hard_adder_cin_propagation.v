`define WIDTH 5
module top_module
(
    input  [`WIDTH-1:0] a, b,
    output [`WIDTH-1:0] out
);

	wire  cout;

	adder #(`WIDTH) add1(.a(a[1:0]), .b(b[1:0]), .cin(1'b0), .sumout(out[1:0]), .cout(cout));
	adder #(`WIDTH) add2(.a(a[3:2]), .b(b[3:2]), .cin(cout), .sumout(out[3:2]), .cout(out[4]));

endmodule
