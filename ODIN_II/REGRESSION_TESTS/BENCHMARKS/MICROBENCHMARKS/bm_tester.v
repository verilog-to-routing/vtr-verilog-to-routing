// DEFINES
`define BITS 4         // Bit width of the operands

module 	bm_tester(clock, 
		reset_n, 
		a_in, 
		b_in,
		out0,
		);

// SIGNAL DECLARATIONS
input	clock;
input 	reset_n;

input [`BITS-1:0] a_in;
input  b_in;

output [`BITS-1:0] out0;

reg [`BITS-1:0]    out0;

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

endmodule
