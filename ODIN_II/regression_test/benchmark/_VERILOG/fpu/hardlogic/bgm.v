// DEFINES
`define BITS 32         // Bit width of the operands
`define NumPath 34     
 
module 	bgm(clock, 
		reset,
		sigma_a, 
		sigma_b, 
		sigma_c,
		Fn,
		dw_x,
		dw_y,
		dw_z,
		dt,
		Fn_out 
);

// SIGNAL DECLARATIONS
input	clock;
input 	reset;

input [`BITS-1:0] sigma_a;
input [`BITS-1:0] sigma_b;
input [`BITS-1:0] sigma_c;
input [`BITS-1:0] Fn;
input [`BITS-1:0] dw_x;
input [`BITS-1:0] dw_y;
input [`BITS-1:0] dw_z;
input [`BITS-1:0] dt;

output [`BITS-1:0] Fn_out;

wire [`BITS-1:0]	x0;
wire [`BITS-1:0]	x1;
wire [`BITS-1:0]	x2;
wire [`BITS-1:0]	x3;
wire [`BITS-1:0]	x4;
wire [`BITS-1:0]	x5;
wire [`BITS-1:0]	x6;
wire [`BITS-1:0]	x7;
wire [`BITS-1:0]	x8;
wire [`BITS-1:0]	x9;
wire [`BITS-1:0]	x10;

wire [`BITS-1:0]	a0;
wire [`BITS-1:0]	a1;
wire [`BITS-1:0]	a2;
wire [`BITS-1:0]	a3;
wire [`BITS-1:0]	a4;
wire [`BITS-1:0]	a5;
wire [`BITS-1:0]	a6;
wire [`BITS-1:0]	a7;
wire [`BITS-1:0]	a8;

wire [`BITS-1:0] Fn_out;
wire [`BITS-1:0] Fn_delay_chain;
wire [`BITS-1:0] Fn_delay_chain_delay5;

wire [`BITS-1:0] dw_x_delay;
wire [`BITS-1:0] dw_y_delay;
wire [`BITS-1:0] dw_z_delay;
wire [`BITS-1:0] sigma_a_delay;
wire [`BITS-1:0] sigma_b_delay;
wire [`BITS-1:0] sigma_c_delay;

wire [`BITS-1:0] fifo_out1;
wire [`BITS-1:0] fifo_out2;
wire [`BITS-1:0] fifo_out3;

wire [`BITS-1:0] a4_delay5;
/*
delay44 delay_u1(clock, dw_x, dw_x_delay);
delay44 delay_u2(clock, dw_y, dw_y_delay);
delay44 delay_u3(clock, dw_z, dw_z_delay);
delay44 delay_u4(clock, sigma_a, sigma_a_delay);
delay44 delay_u5(clock, sigma_b, sigma_b_delay);
delay44 delay_u6(clock, sigma_c, sigma_c_delay);

fifo fifo_1(clock, a0, fifo_out1);
fifo fifo_2(clock, a1, fifo_out2);
fifo fifo_3(clock, a2, fifo_out3);
*/

delay5 delay_u1(clock, dw_x, dw_x_delay);
delay5 delay_u2(clock, dw_y, dw_y_delay);
delay5 delay_u3(clock, dw_z, dw_z_delay);
delay5 delay_u4(clock, sigma_a, sigma_a_delay);
delay5 delay_u5(clock, sigma_b, sigma_b_delay);
delay5 delay_u6(clock, sigma_c, sigma_c_delay);

delay5 fifo_1(clock, a0, fifo_out1);
delay5 fifo_2(clock, a1, fifo_out2);
delay5 fifo_3(clock, a2, fifo_out3);



//assign x0 = Fn * sigma_a;
wire [7:0] x0_control;
fpu_mul x0_mul
( 
	.clk(clock), 
	.opa(Fn), 
	.opb(sigma_a), 
	.out(x0), 
	.control(x0_control) 
);

//assign x1 = Fn * sigma_b;
wire [7:0] x1_control;
fpu_mul x1_mul
( 
	.clk(clock), 
	.opa(Fn), 
	.opb(sigma_b), 
	.out(x1), 
	.control(x1_control) 
);


//assign x2 = Fn * sigma_c;
wire [7:0] x2_control;
fpu_mul x2_mul
( 
	.clk(clock), 
	.opa(Fn), 
	.opb(sigma_c), 
	.out(x2), 
	.control(x2_control) 
);

//assign a0 = x0 + fifo_out1;
wire [7:0] a0_control;
fpu_add a0_add
( 
	.clk(clock), 
	.opa(x0), 
	.opb(fifo_out1), 
	.out(a0), 
	.control(a0_control) 
);


//assign a1 = x1 + fifo_out2;
wire [7:0] a1_control;
fpu_add a1_add
( 
	.clk(clock), 
	.opa(x1), 
	.opb(fifo_out2), 
	.out(a1), 
	.control(a1_control) 
);

//assign a2 = x2 + fifo_out3;
wire [7:0] a2_control;
fpu_add a2_add
( 
	.clk(clock), 
	.opa(x2), 
	.opb(fifo_out3), 
	.out(a2), 
	.control(a2_control) 
);

//assign x3 = dw_x_delay * sigma_a_delay;
wire [7:0] x3_control;
fpu_mul x3_mul
( 
	.clk(clock), 
	.opa(dw_x_delay), 
	.opb(sigma_a_delay), 
	.out(x3), 
	.control(x3_control) 
);


//assign x4 = a0 * sigma_a_delay;
wire [7:0] x4_control;
fpu_mul x4_mul
( 
	.clk(clock), 
	.opa(a0), 
	.opb(sigma_a_delay), 
	.out(x4), 
	.control(x4_control) 
);


//assign x5 = dw_y_delay * sigma_b_delay;
wire [7:0] x5_control;
fpu_mul x5_mul
( 
	.clk(clock), 
	.opa(dw_y_delay), 
	.opb(sigma_b_delay), 
	.out(x5), 
	.control(x5_control) 
);

//assign x6 = a1 * sigma_b_delay;
wire [7:0] x6_control;
fpu_mul x6_mul
( 
	.clk(clock), 
	.opa(a1), 
	.opb(sigma_b_delay), 
	.out(x6), 
	.control(x6_control) 
);

//assign x7 = dw_z_delay * sigma_c_delay;
wire [7:0] x7_control;
fpu_mul x7_mul
( 
	.clk(clock), 
	.opa(dw_z_delay), 
	.opb(sigma_c_delay), 
	.out(x7), 
	.control(x7_control) 
);

//assign x8 = a2 * sigma_c_delay;
wire [7:0] x8_control;
fpu_mul x8_mul
( 
	.clk(clock), 
	.opa(a2), 
	.opb(sigma_c_delay), 
	.out(x8), 
	.control(x8_control) 
);

//assign a3 = x3 + x5;
wire [7:0] a3_control;
fpu_add a3_add
( 
	.clk(clock), 
	.opa(x3), 
	.opb(x5), 
	.out(a3), 
	.control(a3_control) 
);

//assign a4 = a3 + x7;
wire [7:0] a4_control;
fpu_add a4_add
( 
	.clk(clock), 
	.opa(a3), 
	.opb(x7), 
	.out(a4), 
	.control(a4_control) 
);


//assign a5 = x4 + x6;
wire [7:0] a5_control;
fpu_add a5_add
( 
	.clk(clock), 
	.opa(x4), 
	.opb(x6), 
	.out(a5), 
	.control(a5_control) 
);

//assign a6 = a5 + x8;
wire [7:0] a6_control;
fpu_add a6_add
( 
	.clk(clock), 
	.opa(a5), 
	.opb(x8), 
	.out(a6), 
	.control(a6_control) 
);

delay5 delay_a5(clock, a4, a4_delay5);

//assign x9 = dt * a6;
wire [7:0] x9_control;
fpu_mul x9_mul
( 
	.clk(clock), 
	.opa(dt), 
	.opb(a6), 
	.out(x9), 
	.control(x9_control) 
);

//assign a7 = a4_delay5 + x9;
wire [7:0] a7_control;
fpu_add a7_add
( 
	.clk(clock), 
	.opa(a4_delay5), 
	.opb(x9), 
	.out(a7), 
	.control(a7_control) 
);


//delay_chain delay_Fn(clock, Fn, Fn_delay_chain);
delay5 delay_Fn(clock, Fn, Fn_delay_chain);
delay5 delay_Fn_delay5(clock, Fn_delay_chain, Fn_delay_chain_delay5);

//assign x10 = a7 * Fn_delay_chain;
wire [7:0] x10_control;
fpu_mul x10_mul
( 
	.clk(clock), 
	.opa(a7), 
	.opb(Fn_delay_chain), 
	.out(x10), 
	.control(x10_control) 
);

//assign a8 = Fn_delay_chain_delay5 + x10;
wire [7:0] a8_control;
fpu_add a8_add
( 
	.clk(clock), 
	.opa(Fn_delay_chain_delay5), 
	.opb(x10), 
	.out(a8), 
	.control(a8_control) 
);

assign	Fn_out	= a8;


endmodule



/*
module fifo(clock, fifo_in, fifo_out);
	input clock;
	input [`BITS-1:0] fifo_in;
	output [`BITS-1:0] fifo_out;
	wire [`BITS-1:0] fifo_out;

	reg [`BITS-1:0] freg1;

	reg [`BITS-1:0] freg2;
	reg [`BITS-1:0] freg3;
	reg [`BITS-1:0] freg4;
	reg [`BITS-1:0] freg5;
	reg [`BITS-1:0] freg6;
	reg [`BITS-1:0] freg7;
	reg [`BITS-1:0] freg8;
	reg [`BITS-1:0] freg9;
	reg [`BITS-1:0] freg10;
	reg [`BITS-1:0] freg11;
	reg [`BITS-1:0] freg12;
	reg [`BITS-1:0] freg13;
	reg [`BITS-1:0] freg14;
	reg [`BITS-1:0] freg15;
	reg [`BITS-1:0] freg16;
	reg [`BITS-1:0] freg17;
	reg [`BITS-1:0] freg18;
	reg [`BITS-1:0] freg19;
	reg [`BITS-1:0] freg20;
	reg [`BITS-1:0] freg21;
	reg [`BITS-1:0] freg22;
	reg [`BITS-1:0] freg23;
	reg [`BITS-1:0] freg24;
	reg [`BITS-1:0] freg25;
	reg [`BITS-1:0] freg26;
	reg [`BITS-1:0] freg27;
	reg [`BITS-1:0] freg28;
	reg [`BITS-1:0] freg29;
	reg [`BITS-1:0] freg30;
	reg [`BITS-1:0] freg31;
	reg [`BITS-1:0] freg32;
	reg [`BITS-1:0] freg33;
	reg [`BITS-1:0] freg34;
 
	assign fifo_out = freg34;

	always @(posedge clock)
	begin
		freg1 <= fifo_in;

		freg2 <= freg1;
		freg3 <= freg2;
		freg4 <= freg3;
		freg5 <= freg4;
		freg6 <= freg5;
		freg7 <= freg6;
		freg8 <= freg7;
		freg9 <= freg8;
		freg10 <= freg9;
		freg11 <= freg10;
		freg12 <= freg11;
		freg13 <= freg12;
		freg14 <= freg13;
		freg15 <= freg14;
		freg16 <= freg15;
		freg17 <= freg16;
		freg18 <= freg17;
		freg19 <= freg18;
		freg20 <= freg19;
		freg21 <= freg20;
		freg22 <= freg21;
		freg23 <= freg22;
		freg24 <= freg23;
		freg25 <= freg24;
		freg26 <= freg25;
		freg27 <= freg26;
		freg28 <= freg27;
		freg29 <= freg28;
		freg30 <= freg29;
		freg31 <= freg30;
		freg32 <= freg31;
		freg33 <= freg32;
		freg34 <= freg33;

	end
endmodule
*/

module delay5 (clock, d5_delay_in, d5_delay_out);
	input clock;
	input [`BITS-1:0] d5_delay_in;
	output [`BITS-1:0] d5_delay_out;

	//FIFO delay
	reg [`BITS-1:0] d5_reg1;
/*
	reg [`BITS-1:0] d5_reg2;
	reg [`BITS-1:0] d5_reg3;
	reg [`BITS-1:0] d5_reg4;
	reg [`BITS-1:0] d5_reg5;
	reg [`BITS-1:0] d5_reg6;
*/

	assign d5_delay_out = d5_reg1;

	always @(posedge clock)
	begin
		d5_reg1 <= d5_delay_in;
/*
		d5_reg2 <= d5_reg1;
		d5_reg3 <= d5_reg2;
		d5_reg4 <= d5_reg3;
		d5_reg5 <= d5_reg4;
		d5_reg6 <= d5_reg5;
*/
	end
endmodule

/*
module delay44 (clock, delay_in, delay_out);
	input clock;
	input [`BITS-1:0] delay_in;
	output [`BITS-1:0] delay_out;
//	wire [`BITS-1:0] delay_out;

	//FIFO delay
	wire [`BITS-1:0] fifo_out;
	
	//multiplier delay
 	wire [`BITS-1:0] delay5_dout1;

	//adder delay
 	wire [`BITS-1:0] delay5_dout2;

	fifo fifo_delay(clock, delay_in , fifo_out);
	delay5 delay_d1(clock, fifo_out, delay5_dout1);
	delay5 delay_d2(clock, delay5_dout1, delay5_dout2);

	assign delay_out = delay5_dout2;
//	always @(posedge clock)
//	begin
//		fifo_out <= delay_in;
//		delay5_dout1 <= fifo_out;
//		delay5_dout2 <= delay5_dout1;
//	end

endmodule
*/

/*
module delay_chain (clock, delay_in, delay_out);
	input clock;
	input [`BITS-1:0] delay_in;
	output [`BITS-1:0] delay_out;
//	wire [`BITS-1:0] delay_out;

	wire [`BITS-1:0] delay44_out;
	wire [`BITS-1:0] delay5_out1;
	wire [`BITS-1:0] delay5_out2;
	wire [`BITS-1:0] delay5_out3;
	wire [`BITS-1:0] delay5_out4;

	delay44 delay_c1(clock, delay_in, delay44_out);
	delay5 delay_c2(clock, delay44_out, delay5_out1);
	delay5 delay_c3(clock, delay5_out1, delay5_out2);
	delay5 delay_c4(clock, delay5_out2, delay5_out3);
	delay5 delay_c5(clock, delay5_out3, delay5_out4);

	assign delay_out = delay5_out4;
endmodule
*/


