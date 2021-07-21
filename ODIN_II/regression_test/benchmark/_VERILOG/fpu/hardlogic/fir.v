// DEFINES
`define BITS 32         // Bit width of the operands

module 	fir(clock, 
		reset,
		x,
		k0,
		k1, 
		k2,
		k3, 
		out 
);

// SIGNAL DECLARATIONS
input	clock;
input 	reset;

input [`BITS-1:0] x;
input [`BITS-1:0] k0;
input [`BITS-1:0] k1;
input [`BITS-1:0] k2;
input [`BITS-1:0] k3;

output [`BITS-1:0] out;

wire [`BITS-1:0] x0k0;
wire [`BITS-1:0] x1k1;
wire [`BITS-1:0] x2k2;
wire [`BITS-1:0] x3k3;

wire [`BITS-1:0] add0;
wire [`BITS-1:0] add1;
wire [`BITS-1:0] add2;
wire [`BITS-1:0] add3;

reg [`BITS-1:0] x_reg1;
reg [`BITS-1:0] x_reg2;
reg [`BITS-1:0] x_reg3;
reg [`BITS-1:0] x_reg4;
reg [`BITS-1:0] x_reg5;
reg [`BITS-1:0] x_reg6;
reg [`BITS-1:0] x_reg7;
reg [`BITS-1:0] x_reg8;
reg [`BITS-1:0] x_reg9;
reg [`BITS-1:0] x_reg10;
reg [`BITS-1:0] x_reg11;
reg [`BITS-1:0] x_reg12;
reg [`BITS-1:0] x_reg13;
reg [`BITS-1:0] x_reg14;
reg [`BITS-1:0] x_reg15;
reg [`BITS-1:0] x_reg16;
reg [`BITS-1:0] x_reg17;
reg [`BITS-1:0] x_reg18;

wire [`BITS-1:0] out;
wire [`BITS-1:0] out_temp;
reg [`BITS-1:0] out_reg;

assign out= out_reg;

// ASSIGN STATEMENTS

//assign	x0k0 = k0 * x;
wire [7:0] x0k0_control;
fpu_mul x0k0_mul
( 
	.clk(clock), 
	.opa(k0), 
	.opb(x), 
	.out(x0k0), 
	.control(x0k0_control) 
);

//assign	x1k1 = k1 * x_reg1;
wire [7:0] x1k1_control;
fpu_mul x1k1_mul
( 
	.clk(clock), 
	.opa(k1), 
	.opb(x_reg6), 
	.out(x1k1), 
	.control(x1k1_control) 
);

//assign	x2k2 = k2 * x_reg2;
wire [7:0] x2k2_control;
fpu_mul x2k2_mul
( 
	.clk(clock), 
	.opa(k2), 
	.opb(x_reg12), 
	.out(x2k2), 
	.control(x2k2_control) 
);

//assign	x3k3 = k3 * x_reg3;
wire [7:0] x3k3_control;
fpu_mul x3k3_mul
( 
	.clk(clock), 
	.opa(k3), 
	.opb(x_reg18), 
	.out(x3k3), 
	.control(x3k3_control) 
);

//assign	add0 = x0k0 + x1k1;
wire [7:0] add0_control;
fpu_add add0_add
( 
	.clk(clock), 
	.opa(x0k0), 
	.opb(x1k1), 
	.out(add0), 
	.control(add0_control) 
);

//assign	add1 = add0 + x2k2;
wire [7:0] add1_control;
fpu_add add1_add
( 
	.clk(clock), 
	.opa(add0), 
	.opb(x2k2), 
	.out(add1), 
	.control(add1_control) 
);

//assign	out = add1 + x3k3;
wire [7:0] out_temp_control;
fpu_add out_temp_add
( 
	.clk(clock), 
	.opa(add1), 
	.opb(x3k3), 
	.out(out_temp), 
	.control(out_temp_control) 
);

always @(posedge clock)
begin
	out_reg <= out_temp;
	x_reg1 <= x;
	x_reg2 <= x_reg1;
	x_reg3 <= x_reg2;
	x_reg4 <= x_reg3;
	x_reg5 <= x_reg4;
	x_reg6 <= x_reg5;
	x_reg7 <= x_reg6;
	x_reg8 <= x_reg7;
	x_reg9 <= x_reg8;
	x_reg10 <= x_reg9;
	x_reg11 <= x_reg10;
	x_reg12 <= x_reg11;
	x_reg13 <= x_reg12;
	x_reg14 <= x_reg13;
	x_reg15 <= x_reg14;
	x_reg16 <= x_reg15;
	x_reg17 <= x_reg16;
	x_reg18 <= x_reg17;

end

endmodule


