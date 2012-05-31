// DEFINES
`define BITS 8			// Bit width of the operands
`define B2TS 16         // Bit width of the operands
`define USEAND 0		// No available documentation provides for macros without values so we use 0

module 	bm_base_multiply(clock, 
		reset_n, 
		a_in, 
		b_in,
		c_in, 
		d_in, 
		e_in,
		f_in,
		out0,
		out2,
		out3,
		out4,
		out1);

// SIGNAL DECLARATIONS
input	clock;
input 	reset_n;

input [`BITS-1:0] a_in;
input [`BITS-1:0] b_in;
input [`BITS-1:0] c_in;
input [`BITS-1:0] d_in;
input [`BITS-1:0] e_in;
input [`BITS-2:0] f_in;

output [`B2TS-1:0] out0;
output [`B2TS-1:0] out1;
output [`B2TS-1:0] out2;
output [14:0] out3;
output [14:0] out4;

reg [`B2TS-1:0]    out0;
wire [`B2TS-1:0]    out1;
reg [`B2TS-1:0]    out2;
reg [14:0] out3;
wire [14:0] out4;

wire [`BITS-1:0] temp_a;
wire [`BITS-1:0] temp_b;
wire temp_c;
wire temp_d;

a top_a(clock, a_in, b_in, temp_a);
b top_b(clock, a_in, b_in, temp_b);

always @(posedge clock)
begin
	out0 <= a_in * b_in;
	out2 <= temp_a & temp_b;
	out3 <= e_in * f_in;
end

assign out1 = c_in * d_in;
assign out4 = f_in * e_in;

endmodule

`ifndef USEAND
`include "ifndef_else_module_a_and.v"
`else
`include "ifndef_else_module_a_or.v"
`endif

`include "ifndef_else_module_b.v"
