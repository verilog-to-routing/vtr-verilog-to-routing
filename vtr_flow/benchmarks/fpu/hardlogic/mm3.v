// DEFINES
`define BITS 32         // Bit width of the operands

module 	mm3(clock, 
		reset,
		a1,
		a2,
		a3, 
		b1,
		b2,
		b3,
		out
);

// SIGNAL DECLARATIONS
input	clock;
input 	reset;

input [`BITS-1:0] a1;
input [`BITS-1:0] a2;
input [`BITS-1:0] a3;
input [`BITS-1:0] b1;
input [`BITS-1:0] b2;
input [`BITS-1:0] b3;

output [`BITS-1:0] out;


wire [`BITS-1:0] x1;
wire [`BITS-1:0] x2;
wire [`BITS-1:0] x3;
wire [`BITS-1:0] add4;
wire [`BITS-1:0] add5;

reg [`BITS-1:0] x3_reg1;
reg [`BITS-1:0] x3_reg2;
reg [`BITS-1:0] x3_reg3;
reg [`BITS-1:0] x3_reg4;
reg [`BITS-1:0] x3_reg5;
reg [`BITS-1:0] x3_reg6;


wire [`BITS-1:0] out;

// ASSIGN STATEMENTS
//assign	x1 = a1 * b1;
wire [7:0] x1_control;
fpu_mul x1_mul
( 
	.clk(clock), 
	.opa(a1), 
	.opb(b1), 
	.out(x1), 
	.control(x1_control) 
);

//assign	x2 = a2 * b2;
wire [7:0] x2_control;
fpu_mul x2_mul
( 
	.clk(clock), 
	.opa(a2), 
	.opb(b2), 
	.out(x2), 
	.control(x2_control) 
);

//assign	x3 = a3 * b3;
wire [7:0] x3_control;
fpu_mul x3_mul
( 
	.clk(clock), 
	.opa(a3), 
	.opb(b3), 
	.out(x3), 
	.control(x3_control) 
);

//assign	add4 = x1 + x2;
wire [7:0] add4_control;
fpu_add add4_add
( 
	.clk(clock), 
	.opa(x1), 
	.opb(x2), 
	.out(add4), 
	.control(add4_control) 
);

//assign	out = add4 + x3_reg5;
wire [7:0] out_control;
fpu_add out_add
( 
	.clk(clock), 
	.opa(add4), 
	.opb(x3_reg6), 
	.out(out), 
	.control(out_control) 
);
always @(posedge clock)
begin
	x3_reg1 <= x3;
	x3_reg2 <= x3_reg1;
	x3_reg3 <= x3_reg2;
	x3_reg4 <= x3_reg3;
	x3_reg5 <= x3_reg4;
	x3_reg6 <= x3_reg5;

end

endmodule




