// DEFINES
`define BITS0 9         // Bit width of the operands
`define BITS1 8         // Bit width of the operands
`define BITS2 18         // Bit width of the operands
`define BITS3 36         // Bit width of the operands

// sees if the softaware matches simple primitives 
module 	bm_match5_str_arch(clock, 
		reset_n, 
		a_in, 
		b_in,
		c_in, 
		d_in, 
		e_in, 
		f_in, 
		g_in, 
		h_in, 
		i_in, 
		j_in, 
		out0,
		out1,
		out2,
);

// SIGNAL DECLARATIONS
input	clock;
input 	reset_n;

input [`BITS0-1:0] a_in;
input [`BITS0-1:0] b_in;
input [`BITS0-1:0] c_in;
input [`BITS0-1:0] d_in;
input [`BITS0-1:0] e_in;
input [`BITS0-1:0] f_in;
input [`BITS0-1:0] g_in;
input [`BITS0-1:0] h_in;
input [`BITS0-1:0] i_in;
input [`BITS0-1:0] j_in;

output [`BITS2-1:0] out0;
output [`BITS2-1:0] out1;
output [`BITS2-1:0] out2;

reg [`BITS2-1:0] out0;
reg [`BITS2-1:0] out1;
reg [`BITS2-1:0] out2;

//assign out3 = a_in + b_in;
//assign out4 = a_in * b_in + c_in * d_in;
//assign out5 = a_in * b_in + c_in * d_in + e_in * f_in + a_in * c_in + g_in * h_in + i_in * j_in + a_in * i_in;

always @(posedge clock)
begin
	out0 <= a_in * b_in + c_in * d_in;
	out1 <= c_in + d_in;
	out2 <= a_in * b_in + c_in * d_in + e_in * f_in + a_in * c_in + g_in * h_in + i_in * j_in;
end

endmodule
