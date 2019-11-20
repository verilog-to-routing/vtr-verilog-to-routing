// DEFINES
`define BITS 4         // Bit width of the operands

module 	bm_stmt_compare_padding(clock, 
		reset_n, 
		a_in, 
		b_in,
		out1,
		out0,
		out5,
		out6,
		out7,
		out8,
		);

// SIGNAL DECLARATIONS
input	clock;
input 	reset_n;

input [`BITS-1:0] a_in;
input  b_in;

output [`BITS-1:0] out0;
output  out1;
output  out5;
output [`BITS-1:0] out6;
output  out7;
output [`BITS-1:0] out8;

reg [`BITS-1:0]    out0;
reg     out1;
reg     out5;
reg [`BITS-1:0]    out6;
reg     out7;
reg [`BITS-1:0]    out8;

always @(posedge clock)
begin
	case (a_in)
		3'b000: out0 <= 4'b1111 ;
		3'b001: out0 <= 4'b1110 ;
		3'b010: out0 <= 4'b1101 ;
		3'b011: out0 <= 4'b1100 ;
		3'b100: out0 <= 4'b1011 ;
		3'b101: out0 <= 4'b1010 ;
		3'b110: out0 <= 4'b1001 ;
		3'b111: out0 <= 4'b1000 ;
		default: out0 <= 4'b0000 ;
	endcase
end

always @(posedge clock)
begin
	case (b_in)
		2'b00: out1 <= 1'b1 ;
		2'b01: out1 <= 1'b0 ;
		default: out1 <= 1'b0 ;
	endcase
end

always @(posedge clock)
begin
	if (b_in == 2'b00) 
	begin
		out5 <= 1'b1 ;
		out6 <= 4'b0001 ;
	end
	else
	begin
		out5 <= 1'b0 ;
		out6 <= 4'b0000 ;
	end
end

always @(posedge clock)
begin
	if (a_in == 1'b0) 
	begin
		out7 <= 1'b1 ;
		out8 <= 4'b0001 ;
	end
	else if (a_in == 3'b000)
	begin
		out7 <= 1'b0 ;
		out8 <= 4'b0100 ;
	end
	else
	begin
		out7 <= 1'b1 ;
		out8 <= 4'b0000 ;
	end
end

endmodule
