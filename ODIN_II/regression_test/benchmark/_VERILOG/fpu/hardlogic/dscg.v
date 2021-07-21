// DEFINES
`define BITS 32         // Bit width of the operands

module 	dscg(clock, 
		reset,
		cos, 
		one, 
		s1,
		s2,
		s1_out,
		s2_out 
);

// SIGNAL DECLARATIONS
input	clock;
input 	reset;

input [`BITS-1:0] cos;
input [`BITS-1:0] one;
input [`BITS-1:0] s1;
input [`BITS-1:0] s2;

output [`BITS-1:0] s1_out;
output [`BITS-1:0] s2_out;


wire [`BITS-1:0]	add1;
wire [`BITS-1:0]	x2;
wire [`BITS-1:0]	x3;

wire [`BITS-1:0]	sub5;
wire [`BITS-1:0]	x6;
wire [`BITS-1:0]	x7;

wire [`BITS-1:0] s1_out;
wire [`BITS-1:0] s2_out;

reg [`BITS-1:0] x3_reg1;

reg [`BITS-1:0] x3_reg2;
reg [`BITS-1:0] x3_reg3;
reg [`BITS-1:0] x3_reg4;
reg [`BITS-1:0] x3_reg5;
reg [`BITS-1:0] x3_reg6;


reg [`BITS-1:0] x7_reg1;

reg [`BITS-1:0] x7_reg2;
reg [`BITS-1:0] x7_reg3;
reg [`BITS-1:0] x7_reg4;
reg [`BITS-1:0] x7_reg5;
reg [`BITS-1:0] x7_reg6;

//assign add1 	= cos + one;
wire [7:0] add1_control;
fpu_add add1_add
( 
	.clk(clock), 
	.opa(cos), 
	.opb(one), 
	.out(add1), 
	.control(add1_control) 
);

//assign x2 		= add1 * s2;
wire [7:0] x2_control;
fpu_mul x2_mul
( 
	.clk(clock), 
	.opa(add1), 
	.opb(s2), 
	.out(x2), 
	.control(x2_control) 
);

//assign x3		= cos * s1;
wire [7:0] x3_control;
fpu_mul x3_mul
( 
	.clk(clock), 
	.opa(cos), 
	.opb(s1), 
	.out(x3), 
	.control(x3_control) 
);

//assign s1_out 	= x2 + x3_reg1;
wire [7:0] s1_out_control;
fpu_add s1_out_add
( 
	.clk(clock), 
	.opa(x2), 
	.opb(x3_reg6), 
	.out(s1_out), 
	.control(s1_out_control) 
);

//assign	sub5 	= one - cos;
wire [7:0] sub5_control;
fpu_add sub5_add
( 
	.clk(clock), 
	.opa(one), 
	.opb(cos), 
	.out(sub5), 
	.control(sub5_control) 
);

//assign	x6 		= sub5 * s1;
wire [7:0] x6_control;
fpu_mul x6_mul
( 
	.clk(clock), 
	.opa(sub5), 
	.opb(s1), 
	.out(x6), 
	.control(x6_control) 
);


//assign 	x7 		= cos * s2;
wire [7:0] x7_control;
fpu_mul x7_mul
( 
	.clk(clock), 
	.opa(cos), 
	.opb(s2), 
	.out(x7), 
	.control(x7_control) 
);

//assign	s2_out 	= x6 + x7_reg1;
wire [7:0] s2_out_control;
fpu_add s2_out_add
( 
	.clk(clock), 
	.opa(x6), 
	.opb(x7_reg6), 
	.out(s2_out), 
	.control(s2_out_control) 
);

always @(posedge clock)
begin

	x3_reg1 <= x3;
	x3_reg2 <= x3_reg1;
	x3_reg3 <= x3_reg2;
	x3_reg4 <= x3_reg3;
	x3_reg5 <= x3_reg4;
	x3_reg6 <= x3_reg5;

	x7_reg1 <= x7;
	x7_reg2 <= x7_reg1;
	x7_reg3 <= x7_reg2;
	x7_reg4 <= x7_reg3;
	x7_reg5 <= x7_reg4;
	x7_reg6 <= x7_reg5;
end

endmodule



