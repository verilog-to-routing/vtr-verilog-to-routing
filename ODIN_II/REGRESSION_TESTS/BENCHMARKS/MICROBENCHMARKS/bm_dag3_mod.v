// DEFINES
`define BITS 2         // Bit width of the operands

module 	bm_dag3_mod(clock, 
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

a top_a(clock, a_in, b_in, temp_a);
b top_b(clock, a_in, b_in, temp_b);
c top_c(clock, c_in, d_in, temp_c);
d top_d(clock, c_in, d_in, temp_d);

always @(posedge clock)
begin
	out0 <= temp_a & temp_b;
	out1 <= temp_c & temp_d;
end

endmodule

/*---------------------------------------------------------*/
module a(clock,
		a_in,
		b_in,
		out);

input	clock;
input [`BITS-1:0] a_in;
input [`BITS-1:0] b_in;
output [`BITS-1:0] out;
reg [`BITS-1:0]    out;
wire temp;
reg [`BITS-1:0]temp2;

d mya_d(clock, a_in[0], b_in[0], temp);

always @(posedge clock)
begin
	temp2 <= a_in &  temp;
	out <= b_in & temp2;
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
output [`BITS-1:0] out;
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
reg     out1;
reg temp;
wire temp2;

d myc_d(clock, c_in, d_in, temp2);

always @(posedge clock)
begin
	temp <= c_in & temp2;
	out1 <= temp ^ d_in;
end

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
	temp <= c_in ^ d_in;
	out1 <= temp | d_in;
end

endmodule


