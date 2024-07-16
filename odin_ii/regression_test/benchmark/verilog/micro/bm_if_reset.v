// DEFINES
`define BITS 2         // Bit width of the operands

module 	bm_if_reset(clock, 
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

a top_a(clock, a_in, temp_a);

always @(posedge clock or negedge reset_n)
begin
	if (reset_n == 1'b0)
	begin
		out0 <= 2'b00;
		out1 <= 1'b0;
	end
	else
	begin
		out0 <= a_in & b_in;
		out1 <= c_in & d_in;
	end
end

endmodule

/*---------------------------------------------------------*/
module a(clock,
		a_in,
		out);

input	clock;
input [`BITS-1:0] a_in;
output [`BITS-1:0] out;
reg [`BITS-1:0] out;

always @(posedge clock)
begin
	case (a_in)
		2'b00: out <= 2'b11 ;
		2'b01: out <= 2'b10 ;
		2'b10: out <= 2'b01 ;
		2'b11: out <= 2'b00 ;
	endcase
end

endmodule

