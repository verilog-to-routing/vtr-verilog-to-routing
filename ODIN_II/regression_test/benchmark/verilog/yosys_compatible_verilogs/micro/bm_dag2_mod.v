// DEFINES
`define BITS 2         // Bit width of the operands

module 	bm_dag2_mod(clock, 
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

always @(posedge clock)
begin
	out0 <= temp_a & temp_b;
	out1 <= c_in & d_in;
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
output [`BITS-1:0]    out;
reg [`BITS-1:0]    out;

always @(posedge clock)
begin
	out <= a_in & b_in;
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
wire [`BITS-1:0] temp;
output [`BITS-1:0] out;
reg [`BITS-1:0] out;

a my_a(clock, a_in, b_in, temp);

always @(posedge clock)
begin
	out <= a_in ^ temp;
end

endmodule
