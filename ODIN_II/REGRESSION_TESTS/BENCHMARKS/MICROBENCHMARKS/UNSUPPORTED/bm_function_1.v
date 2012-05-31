// DEFINES
`define BITS 2         // Bit width of the operands

module 	bm_function_1(clock, 
		reset_n, 
		a_in, 
		b_in,
		c_in, 
		d_in, 
		out0,
		out1);

// SIGNAL DECLARATIONS
input	clock;
input 	reset_n;

input [`BITS-1:0] a_in;
input [`BITS-1:0] b_in;
input c_in;
input d_in;

output [`BITS-1:0] out0;
output  out1;

reg [`BITS-1:0]    out0;
wire	part1, part2;
wire     out1;

reg temp;

function simple_func;
	input [`BITS-1:0] in1;
	input [`BITS-1:0] in2;
	input in3;
	input in4;

	simple_func <= in1[1] & in2[0] | in3 ^ in4;
endfunction

assign part1 = simple_func(a_in[`BITS-1:0], b_in[`BITS-1:0], c_in[0], d_in[0]) & a_in;
assign part2 = simple_func(b_in[`BITS-1:0], a_in[`BITS-1:0], c_in[0], d_in[0]) & a_in;
assign out1 = part1 & part2;

always @(posedge clock or negedge reset_n)
begin
	temp <= simple_func(a_in[`BITS-1:0], b_in[`BITS-1:0], c_in[0], d_in[0]);
	out0 <= temp | a_in | b_in;
end

endmodule
