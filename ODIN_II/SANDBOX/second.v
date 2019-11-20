// DEFINES
`define BITS 2

module bm_add_lpm(clock, 
		reset_n, 
		a_in, 
		out);

// SIGNAL DECLARATIONS
input	clock;
input 	reset_n;

input [`BITS-1:0] a_in;
//input [`BITS-1:0] b_in;

output [`BITS-1:0] out;

reg [`BITS-1:0] out;

// ASSIGN STATEMENTS
//assign out = a_in * b_in;

always @(posedge clock)
	out <= a_in;

endmodule
