// DEFINES
`define BITS 32         // Bit width of the operands

module 	syn2(clock, 
		reset,
		in1,
		in2,
		in3, 
		in4,
		in5,
		out_1,
		out_2,
		out_3,
		out_4
);

// SIGNAL DECLARATIONS
input	clock;
input 	reset;

input [`BITS-1:0] in1;
input [`BITS-1:0] in2;
input [`BITS-1:0] in3;
input [`BITS-1:0] in4;
input [`BITS-1:0] in5;

output [`BITS-1:0] out_1;
output [`BITS-1:0] out_2;
output [`BITS-1:0] out_3;
output [`BITS-1:0] out_4;


wire [`BITS-1:0] x1;
wire [`BITS-1:0] x2;
wire [`BITS-1:0] x3;
wire [`BITS-1:0] x4;

wire [`BITS-1:0] add1;
wire [`BITS-1:0] add2;
wire [`BITS-1:0] add3;
wire [`BITS-1:0] add4;
wire [`BITS-1:0] add5;

reg [`BITS-1:0] reg1;
reg [`BITS-1:0] reg2;
reg [`BITS-1:0] reg3;
reg [`BITS-1:0] reg4;
reg [`BITS-1:0] reg5;
reg [`BITS-1:0] reg6;

wire [`BITS-1:0] out_1;
wire [`BITS-1:0] out_2;
wire [`BITS-1:0] out_3;
wire [`BITS-1:0] out_4;

// ASSIGN STATEMENTS
//assign	add1 = reg1 + in4;
wire [7:0] add1_control;
fpu_add add1_add
( 
	.clk(clock), 
	.opa(reg6), 
	.opb(in4), 
	.out(add1), 
	.control(add1_control) 
);

//assign	x1 = x3 * in1;
wire [7:0] x1_control;
fpu_mul x1_mul
( 
	.clk(clock), 
	.opa(x3), 
	.opb(in1), 
	.out(x1), 
	.control(x1_control) 
);

//assign	add2 = add5 + add1;
wire [7:0] add2_control;
fpu_add add2_add
( 
	.clk(clock), 
	.opa(add5), 
	.opb(add1), 
	.out(add2), 
	.control(add2_control) 
);

//assign 	x2 = x1 * add2;
wire [7:0] x2_control;
fpu_mul x2_mul
( 
	.clk(clock), 
	.opa(x1), 
	.opb(add2), 
	.out(x2), 
	.control(x2_control) 
);

//assign	add3 = in1 + reg1;
wire [7:0] add3_control;
fpu_add add3_add
( 
	.clk(clock), 
	.opa(in1), 
	.opb(reg6), 
	.out(add3), 
	.control(add3_control) 
);

//assign	x3 = in3 * in1;
wire [7:0] x3_control;
fpu_mul x3_mul
( 
	.clk(clock), 
	.opa(in3), 
	.opb(in1), 
	.out(x3), 
	.control(x3_control) 
);


//assign	add4 = in5 + in3;
wire [7:0] add4_control;
fpu_add add4_add
( 
	.clk(clock), 
	.opa(in5), 
	.opb(in3), 
	.out(add4), 
	.control(add4_control) 
);

//assign	x4 = in5 * in4;
wire [7:0] x4_control;
fpu_mul x4_mul
( 
	.clk(clock), 
	.opa(in5), 
	.opb(in4), 
	.out(x4), 
	.control(x4_control) 
);

//assign	add5 = in5 + in4;
wire [7:0] add5_control;
fpu_add add5_add
( 
	.clk(clock), 
	.opa(in5), 
	.opb(in4), 
	.out(add5), 
	.control(add5_control) 
);

assign	out_1 = x2;
assign 	out_2 = add3;
assign	out_3 = add4;
assign	out_4 = x4;

always @(posedge clock)
begin
	reg1 <= in2;
	reg2 <= reg1;
	reg3 <= reg2;
	reg4 <= reg3;
	reg5 <= reg4;
	reg6 <= reg5;
end

endmodule
