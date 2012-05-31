// DEFINES
`define BITS 32         // Bit width of the operands

module 	bm_log_all(clock, 
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

input [`BITS-1:0] a;
input [`BITS-1:0] b;

output [`BITS-1:0] out1;
output [`BITS-1:0] out2;
output [`BITS-1:0] out3;
output [`BITS-1:0] out4;
output [`BITS-1:0] out5;
output [`BITS-1:0] out6;
output [`BITS-1:0] out7;
output [`BITS-1:0] out8;

wire [`BITS-1:0]    out1;
wire [`BITS-1:0]    out2;
wire [`BITS-1:0]    out3;
wire [`BITS-1:0]    out4;
wire [`BITS-1:0]    out5;
wire [`BITS-1:0]    out6;
wire [`BITS-1:0]    out7;
wire [`BITS-1:0]    out8;

// ASSIGN STATEMENTS
assign out1 = a & b; // AND
assign out2 = a | b; // OR
assign out3 = a ^ b; // XOR
assign out4 = a ~^ b; // XNOR
assign out5 = a ~& b; // NAND
assign out6 = a ~| b; // NOR
assign out7 = ~a; // NOT
assign out8 = (a & b) | (a ^ b) | (~a | b);

endmodule
