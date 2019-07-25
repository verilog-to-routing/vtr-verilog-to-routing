// DEFINES
`define BITS 32         // Bit width of the operands

module 	bm_lpm_concat(clock, 
		reset_n, 
		a, 
		b, 
		out1,
		out2,
		out3,
		out4,
		out5,
		out6,
		out7,
		out8,
);

// SIGNAL DECLARATIONS
input	clock;
input 	reset_n;

input [`BITS-9:0] a;
input [`BITS-9:0] b;

output [`BITS-8:0] out1;
output [`BITS-7:0] out2;
output [`BITS-6:0] out3;
output [`BITS-5:0] out4;
output [`BITS-4:0] out5;
output [`BITS-3:0] out6;
output [`BITS-2:0] out7;
output [`BITS-1:0] out8;

wire [`BITS-8:0]    out1;
wire [`BITS-7:0]    out2;
wire [`BITS-6:0]    out3;
wire [`BITS-5:0]    out4;
wire [`BITS-4:0]    out5;
wire [`BITS-3:0]    out6;
wire [`BITS-2:0]    out7;
wire [`BITS-1:0]    out8;

// ASSIGN STATEMENTS
assign out1 = {1'b0, a};
assign out2 = {1'b1, 1'b0, b}; 
assign out3 = {1'b1, 1'b1, out1};
assign out4 = {1'b0, out3};
assign out5 = {1'b1, out4};
assign out6 = {1'b0, out5};
assign out7 = {1'b1, out6};
assign out8 = {1'b0, out7};

endmodule
