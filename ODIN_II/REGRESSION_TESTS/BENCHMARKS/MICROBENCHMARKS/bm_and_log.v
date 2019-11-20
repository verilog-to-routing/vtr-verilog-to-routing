// DEFINES
`define BITS 32         // Bit width of the operands

module bm_and_log(clock,
		reset_n,
		a_in,
		b_in,
		out);

// SIGNAL DECLARATIONS
input	clock;
input 	reset_n;

input [`BITS-1:0] a_in;
input [`BITS-1:0] b_in;

output [`BITS-1:0] out;

wire [`BITS-1:0]    out;

// ASSIGN STATEMENTS
assign out = a_in & b_in;

endmodule
