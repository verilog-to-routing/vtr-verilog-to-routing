// DEFINES
`define BITS 2         // Bit width of the operands

module 	bm_dag6_mod(clock, 
		reset_n, 
		a_in, 
		b_in,
		c_in, 
		d_in, 
		out0,
		out1);

// SIGNAL DECLARATIONS
input	clock;
input 	reset_n;

input [`BITS-1:0] a_in;
input [`BITS-1:0] b_in;
input c_in;
input d_in;

output [`BITS-1:0] out0;
output  out1;

reg [`BITS-1:0]    out0;
reg     out1;

wire [`BITS-1:0] temp_a;
wire [`BITS-1:0] temp_b;
wire temp_c;
wire temp_d;

a top_a({a_in[0], c_in}, .b_in(2'b10), .out(temp_a));
b top_b(a_in, b_in, temp_b);
c top_c(c_in, d_in, temp_c);
d top_d(c_in, d_in, temp_d);

and(out0[0], temp_a[0], temp_b[0]);
and(out0[1], temp_a[1], temp_b[1]);
or(out1, temp_c, temp_d);

endmodule

/*---------------------------------------------------------*/
module a(
		a_in,
		b_in,
		out);

input [`BITS-1:0] a_in;
input [`BITS-1:0] b_in;
output [`BITS-1:0] out;
reg [`BITS-1:0] out;

xor(out[0], a_in[0], b_in[0]);
xnor(out[1], a_in[1], b_in[1]);

endmodule

/*---------------------------------------------------------*/
module b(
		a_in,
		b_in,
		out);

input [`BITS-1:0] a_in;
input [`BITS-1:0] b_in;
output [`BITS-1:0]    out;
reg [`BITS-1:0] out;
reg [`BITS-1:0] temp;

nand(temp[0], a_in[0], b_in[0]);
nand(temp[1], a_in[1], b_in[1]);
nor(out[0], a_in[0], temp[0]);
nor(out[1], a_in[1], temp[1]);

endmodule

/*---------------------------------------------------------*/
module 	c(
		c_in, 
		d_in, 
		out1);

// SIGNAL DECLARATIONS
input c_in;
input d_in;
output  out1;
reg     out1;
reg     temp;

and(temp, c_in, d_in);
and(out1, temp, d_in);

endmodule

/*---------------------------------------------------------*/
module 	d(
		c_in, 
		d_in, 
		out1);

// SIGNAL DECLARATIONS
input c_in;
input d_in;
output  out1;
reg     out1;
reg temp;
reg temp2;
reg temp3;

//not(temp2, 1'b0);
not(temp2, 1'b0);
and(temp, c_in, temp3);
and(temp3, d_in, temp2);
and(out1, temp, d_in);

endmodule


