// DEFINES
`define BITS 32         // Bit width of the operands

module 	bfly(clock, 
		reset,
		re_w, 
		re_x, 
		re_y,
		im_w,
		im_x,
		im_y, 
		re_z,
		im_z
);

// SIGNAL DECLARATIONS
input	clock;
input 	reset;

input [`BITS-1:0] re_w;
input [`BITS-1:0] re_x;
input [`BITS-1:0] re_y;
input [`BITS-1:0] im_w;
input [`BITS-1:0] im_x;
input [`BITS-1:0] im_y;

output [`BITS-1:0] re_z;
output [`BITS-1:0] im_z;


wire [`BITS-1:0]	x1;
wire [`BITS-1:0]	x2;
wire [`BITS-1:0]	x3;
wire [`BITS-1:0]	x4;

wire [`BITS-1:0]	sub5;
wire [`BITS-1:0]	add6;
wire [`BITS-1:0]	re_z;
wire [`BITS-1:0]	im_z;

reg [`BITS-1:0] re_y_reg1;

reg [`BITS-1:0] re_y_reg2;
reg [`BITS-1:0] re_y_reg3;
reg [`BITS-1:0] re_y_reg4;
reg [`BITS-1:0] re_y_reg5;
reg [`BITS-1:0] re_y_reg6;


reg [`BITS-1:0] im_y_reg1;

reg [`BITS-1:0] im_y_reg2;
reg [`BITS-1:0] im_y_reg3;
reg [`BITS-1:0] im_y_reg4;
reg [`BITS-1:0] im_y_reg5;
reg [`BITS-1:0] im_y_reg6;



//assign	x1 = re_x * re_w;
wire [7:0] x1_control;
fpu_mul x1_mul
( 
	.clk(clock), 
	.opa(re_x), 
	.opb(re_w), 
	.out(x1), 
	.control(x1_control) 
);

//assign	x2 = im_x * im_w;
wire [7:0] x2_control;
fpu_mul x2_mul
( 
	.clk(clock), 
	.opa(im_x), 
	.opb(im_w), 
	.out(x2), 
	.control(x2_control) 
);


//assign	x3 = re_x * im_w;
wire [7:0] x3_control;
fpu_mul x3_mul
( 
	.clk(clock), 
	.opa(re_x), 
	.opb(im_w), 
	.out(x3), 
	.control(x3_control) 
);

//assign	x4 = im_x * re_w;
wire [7:0] x4_control;
fpu_mul x4_mul
( 
	.clk(clock), 
	.opa(im_x), 
	.opb(re_w), 
	.out(x4), 
	.control(x4_control) 
);


//assign	sub5 = x1 - x2;
wire [7:0] sub5_control;
fpu_add sub5_add
( 
	.clk(clock), 
	.opa(x1), 
	.opb(x2), 
	.out(sub5), 
	.control(sub5_control) 
);

//assign	add6 = x3 + x4;
wire [7:0] add6_control;
fpu_add add6_add
( 
	.clk(clock), 
	.opa(x3), 
	.opb(x4), 
	.out(add6), 
	.control(add6_control) 
);

//assign	re_z = sub5 + re_y_reg1;
wire [7:0] re_z_control;
fpu_add re_z_add
( 
	.clk(clock), 
	.opa(sub5), 
	.opb(re_y_reg6), 
	.out(re_z), 
	.control(re_z_control) 
);


//assign 	im_z = add6 + im_y_reg1;
wire [7:0] im_z_control;
fpu_add im_z_add
( 
	.clk(clock), 
	.opa(add6), 
	.opb(im_y_reg6), 
	.out(im_z), 
	.control(im_z_control) 
);

always @(posedge clock)
begin
	re_y_reg1 <= re_y;

	re_y_reg2 <= re_y_reg1;
	re_y_reg3 <= re_y_reg2;
	re_y_reg4 <= re_y_reg3;
	re_y_reg5 <= re_y_reg4;
	re_y_reg6 <= re_y_reg5;

	im_y_reg1 <= im_y;

	im_y_reg2 <= im_y_reg1;	
	im_y_reg3 <= im_y_reg2;	
	im_y_reg4 <= im_y_reg3;	
	im_y_reg5 <= im_y_reg4;
	im_y_reg6 <= im_y_reg5;	

end

endmodule

