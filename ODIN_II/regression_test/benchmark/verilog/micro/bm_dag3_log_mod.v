// DEFINES
`define BITS 2         // Bit width of the operands

module 	bm_dag3_log_mod(clock, 
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

wire [`BITS-1:0]    out0;
wire     out1;

wire [`BITS-1:0] temp_a;
wire [`BITS-1:0] temp_b;
wire temp_c;
wire temp_d;

a top_a(clock, a_in, b_in, temp_a);
b top_b(clock, a_in, b_in, temp_b);
c top_c(clock, c_in, d_in, temp_c);
d top_d(clock, c_in, d_in, temp_d);

assign out0 = temp_a & temp_b;
assign out1 = temp_c & temp_d;

endmodule

/*---------------------------------------------------------*/
module a(clock,
		a_in,
		b_in,
		out);

input	clock;
input [`BITS-1:0] a_in;
input [`BITS-1:0] b_in;
output [`BITS-1:0]    out;
reg [`BITS-1:0]    out;
wire [`BITS-1:0] temp;

d mya_d(clock, a_in[0], b_in[0], temp[0]);
d mya_d2(clock, a_in[1], b_in[1], temp[1]);

always @(posedge clock)
begin
	out <= a_in & b_in & temp;
end

endmodule

/*---------------------------------------------------------*/
module b(clock,
		a_in,
		b_in,
		out);

input	clock;
input [`BITS-1:0] a_in;
input [`BITS-1:0] b_in;
reg [`BITS-1:0] temp;
wire temp2;
output [`BITS-1:0]    out;
reg [`BITS-1:0] out;

c myb_c(clock, a_in[0], b_in[0], temp2);

always @(posedge clock)
begin
	temp <= a_in | b_in ^ temp2;
	out <= a_in ^ temp;
end

endmodule

/*---------------------------------------------------------*/
module 	c(clock, 
		c_in, 
		d_in, 
		out1);

// SIGNAL DECLARATIONS
input	clock;
input c_in;
input d_in;
output  out1;
wire     out1;
wire temp;
wire temp2;

d myc_d(clock, c_in, d_in, temp2);

assign out1 = temp ^ d_in;
assign temp = c_in & temp2;

endmodule

/*---------------------------------------------------------*/
module 	d(clock, 
		c_in, 
		d_in, 
		out1);

// SIGNAL DECLARATIONS
input	clock;
input c_in;
input d_in;
output  out1;
reg     out1;
reg temp;

always @(posedge clock)
begin
	out1 <= temp | d_in;
	temp <= c_in ^ d_in;
end

endmodule
