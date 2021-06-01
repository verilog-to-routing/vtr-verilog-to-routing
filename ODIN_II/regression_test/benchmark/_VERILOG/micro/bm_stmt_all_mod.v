// DEFINES
`define BITS 4         // Bit width of the operands

module 	bm_stmt_all_mod(clock, 
		reset_n, 
		a_in, 
		b_in,
		out1,
		out0,
		out3,
		out4,
		out5,
		out6,
		out7,
		out8,
		out10,
		out9);

// SIGNAL DECLARATIONS
input	clock;
input 	reset_n;

input [`BITS-1:0] a_in;
input  b_in;

output [`BITS-1:0] out0;
output  out1;
output  out3;
output [`BITS-1:0] out4;
output  out5;
output [`BITS-1:0] out6;
output  out7;
output [`BITS-1:0] out8;
output  out9;
output [`BITS-1:0] out10;

reg [`BITS-1:0]    out0;
reg     out1;
reg     out3;
reg [`BITS-1:0]    out4;
reg     out5;
reg [`BITS-1:0]    out6;
reg     out7;
reg [`BITS-1:0]    out8;
reg     out9;
reg [`BITS-1:0]    out10;

always @(posedge clock)
begin
	case (a_in)
		4'b0000: out0 <= 4'b1111 ;
		4'b0001: out0 <= 4'b1110 ;
		4'b0010: out0 <= 4'b1101 ;
		4'b0011: out0 <= 4'b1100 ;
		4'b0100: out0 <= 4'b1011 ;
		4'b0101: out0 <= 4'b1010 ;
		4'b0110: out0 <= 4'b1001 ;
		4'b0111: out0 <= 4'b1000 ;
		4'b1000: out0 <= 4'b0111 ;
		4'b1001: out0 <= 4'b0110 ;
		4'b1010: out0 <= 4'b0101 ;
		4'b1011: out0 <= 4'b0100 ;
		4'b1100: out0 <= 4'b0011 ;
		4'b1101: out0 <= 4'b0010 ;
		4'b1110: out0 <= 4'b0001 ;
		4'b1111: out0 <= 4'b0000 ;
		default: out0 <= 4'b0000 ;
	endcase
end

always @(posedge clock)
begin
	case (b_in)
		1'b0: out1 <= 1'b1 ;
		1'b1: out1 <= 1'b0 ;
		default: out1 <= 1'b0 ;
	endcase
end

always @(posedge clock)
begin
	case (b_in)
		1'b0: 
		begin
			out3 <= 1'b1 ;
			out4 <= 4'b0001 ;
		end
		1'b1: 
		begin
			out3 <= 1'b0 ;
			out4 <= 4'b0000 ;
		end
		default: out3 <= 1'b0 ;
	endcase
end

always @(posedge clock)
begin
	if (b_in == 1'b0) 
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
	if (b_in == 1'b0) 
	begin
		out7 <= 1'b1 ;
		out8 <= 4'b0001 ;
	end
	else if (a_in == 4'b0000)
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

always @(posedge clock)
begin
	out9 <= 1'b1;
	out10 <= 4'b0000;
end 

endmodule
