// DEFINES
`define BITS 2         // Bit width of the operands

module 	bm_dag2_log(clock, 
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
wire [`BITS-1:0]    temp1;
wire [`BITS-1:0]    temp2;
wire [`BITS-1:0]    temp3;

// ASSIGN STATEMENTS
assign out = temp1 & temp2;
assign temp1 = a_in | b_in;
assign temp2 = temp1 ^ b_in;

endmodule
