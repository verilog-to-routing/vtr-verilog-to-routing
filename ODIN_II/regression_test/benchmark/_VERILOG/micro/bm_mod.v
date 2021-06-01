// DEFINES
`define BITS 32         // Bit width of the operands

module 	bm_mod(clock, 
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
reg     out1;

always @(posedge clock)
begin
	out0 <= a_in & b_in;
	out1 <= c_in & d_in;
end

endmodule
