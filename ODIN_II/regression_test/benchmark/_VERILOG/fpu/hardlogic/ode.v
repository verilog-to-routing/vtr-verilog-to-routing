// DEFINES
`define BITS 32         // Bit width of the operands

module 	ode(clock, 
		reset,
		select,
		h,
		half,
		y_pi_in, 
		t_pi_in,
		y_pi_out,
		t_pi_out 
);

// SIGNAL DECLARATIONS
input	clock;
input 	reset;
input select; 
input [`BITS-1:0] h;
input [`BITS-1:0] half;
input [`BITS-1:0] y_pi_in;
input [`BITS-1:0] t_pi_in;

output [`BITS-1:0] y_pi_out;
output [`BITS-1:0] t_pi_out;
output [7:0] x1_control;


wire [`BITS-1:0] x1;
wire [`BITS-1:0] sub2;
wire [`BITS-1:0] x3;
wire [`BITS-1:0] add4;
wire [`BITS-1:0] add5;

wire [`BITS-1:0] y_pi;
wire [`BITS-1:0] t_pi;

reg [`BITS-1:0] t_reg1;
reg [`BITS-1:0] t_reg2;
reg [`BITS-1:0] t_reg3;
reg [`BITS-1:0] t_reg4;
reg [`BITS-1:0] t_reg5;
reg [`BITS-1:0] t_reg6;
reg [`BITS-1:0] t_reg7;
reg [`BITS-1:0] t_reg8;
reg [`BITS-1:0] t_reg9;
reg [`BITS-1:0] t_reg10;
reg [`BITS-1:0] t_reg11;
reg [`BITS-1:0] t_reg12;

reg [`BITS-1:0] y_pi_reg1;
reg [`BITS-1:0] y_pi_reg2;
reg [`BITS-1:0] y_pi_reg3;
reg [`BITS-1:0] y_pi_reg4;
reg [`BITS-1:0] y_pi_reg5;
reg [`BITS-1:0] y_pi_reg6;
reg [`BITS-1:0] y_pi_reg7;
reg [`BITS-1:0] y_pi_reg8;
reg [`BITS-1:0] y_pi_reg9;
reg [`BITS-1:0] y_pi_reg10;
reg [`BITS-1:0] y_pi_reg11;
reg [`BITS-1:0] y_pi_reg12;
wire [`BITS-1:0] y_pi_out;
reg [`BITS-1:0] y_pi_out_reg;

wire [`BITS-1:0] t_pi_out;

// ASSIGN STATEMENTS
assign y_pi = select ? y_pi_in : add4;
assign t_pi = select ? t_pi_in : t_reg1;

//assign	x1 = h * half;
wire [7:0] x1_control;
fpu_mul x1_mul
( 
	.clk(clock), 
	.opa(h), 
	.opb(half), 
	.out(x1), 
	.control(x1_control) 
);

//assign	sub2 = t_pi - y_pi;
wire [7:0] sub2_control;
fpu_add sub2_add
( 
	.clk(clock), 
	.opa(t_pi), 
	.opb(y_pi), 
	.out(sub2), 
	.control(sub2_control) 
);

//assign	x3 = x1 * sub2;

wire [7:0] x3_control;
fpu_mul x3_mul
( 
	.clk(clock), 
	.opa(x1), 
	.opb(sub2), 
	.out(x3), 
	.control(x3_control) 
);

//assign	add4 = y_pi_reg1 + x3;
wire [7:0] add4_control;
fpu_add add4_add
( 
	.clk(clock), 
	.opa(y_pi_reg12), 
	.opb(x3), 
	.out(add4), 
	.control(add4_control) 
);

//assign	add5 = h + t_pi;
wire [7:0] add5_control;
fpu_add add5_add
( 
	.clk(clock), 
	.opa(h), 
	.opb(t_pi), 
	.out(add5), 
	.control(add5_control) 
);

//assign	y_pi_out = add4;
assign	y_pi_out = y_pi_out_reg;
assign	t_pi_out = t_reg12;

always @(posedge clock)
begin
	y_pi_out_reg <= add4;
	t_reg1 <= add5;
	t_reg2 <= t_reg1;
	t_reg3 <= t_reg2;
	t_reg4 <= t_reg3;
	t_reg5 <= t_reg4;
	t_reg6 <= t_reg5;
	t_reg7 <= t_reg6;
	t_reg8 <= t_reg7;
	t_reg9 <= t_reg8;
	t_reg10 <= t_reg9;
	t_reg11 <= t_reg10;
	t_reg12 <= t_reg11;

	y_pi_reg1 <= y_pi;
	y_pi_reg2 <= y_pi_reg1;
	y_pi_reg3 <= y_pi_reg2;	
	y_pi_reg4 <= y_pi_reg3;	
	y_pi_reg5 <= y_pi_reg4;	
	y_pi_reg6 <= y_pi_reg5;	
	y_pi_reg7 <= y_pi_reg6;	
	y_pi_reg8 <= y_pi_reg7;	
	y_pi_reg9 <= y_pi_reg8;	
	y_pi_reg10 <= y_pi_reg9;	
	y_pi_reg11 <= y_pi_reg10;	
	y_pi_reg12 <= y_pi_reg11;	

end
endmodule

