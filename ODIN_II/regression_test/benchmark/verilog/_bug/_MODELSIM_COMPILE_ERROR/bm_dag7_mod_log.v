// DEFINES
`define BITS 2         // Bit width of the operands

module 	bm_dag7_mod(clock, 
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

//a top_a({a_in[0], c_in}, .b_in(2'b10), .out(temp_a));

and(out0[0], a_in[0], b_in[0]);
and(out0[1], a_in[1], b_in[1]);
or(out1, c_in, d_in);

endmodule

/*---------------------------------------------------------*/
/*module a(
		a_in,
		b_in,
		out);

input [`BITS-1:0] a_in;
input [`BITS-1:0] b_in;
output [`BITS-1:0] out;
reg [`BITS-1:0] out;

xor(out[0], a_in[0], b_in[0]);
xnor(out[1], a_in[1], b_in[1]);

endmodule*/
