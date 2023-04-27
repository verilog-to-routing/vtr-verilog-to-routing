// DEFINES
`define BITS 8         // Bit width of the operands
`define B2TS 16         // Bit width of the operands

module 	bm_functional_test(clock, 
		reset_n, 
		a_in, 
		b_in,
		c_in, 
		d_in, 
		e_in,
		f_in,
		out0,
		out1,
		counter
		);

// SIGNAL DECLARATIONS
input	clock;
input 	reset_n;

input [`BITS-1:0] a_in;
input [`BITS-1:0] b_in;
input [`BITS-1:0] c_in;
input [`BITS-1:0] d_in;
input [`BITS-1:0] e_in;
input [`BITS-2:0] f_in;

output [`B2TS-1:0] out0;
output [`B2TS-1:0] out1;
output [7:0] counter;

reg [`B2TS-1:0]    out0;
reg [`B2TS-1:0]    out1;

wire [`B2TS-1:0] temp1;
wire [`B2TS-1:0] temp2;
wire [`B2TS-1:0] temp3;
wire [`B2TS-1:0] temp4;

reg [7:0]counter;

always @(posedge clock or negedge reset_n)
begin
	if (~reset_n)
	begin
		counter <= 0;
		out1<= 0;
		out0<= 0;
	end
	else
	begin
		case (counter)
			8'b00000000: out0 <= a_in & b_in;
			8'b00000001: out0 <= a_in | b_in;
			8'b00000010: out0 <= a_in ^ b_in;
			8'b00000011: out0 <= a_in * b_in;
			8'b00000100: out0 <= a_in + b_in;
			8'b00000101: out0 <= a_in - b_in;
			8'b00000110: out0 <= temp1;
			8'b00000111: out0 <= temp2;
			8'b00001000: out0 <= temp3;
			8'b00001001: out0 <= temp4;
			8'b00001010: out0 <= temp1 ? temp2 : temp3;
			default: out0 <= 8'b11001101;
		endcase


		if (counter <= 8'b00001111)
		begin
			out1 <= 1;	
		end
		else
		begin
			out1 <= 0;	
		end

		if (counter == 8'b11111111)
			counter <= 0;
		else
			counter <= counter + 1;	
	end
end

assign temp1 = c_in * d_in;
assign temp2 = c_in + d_in;
assign temp3 = c_in - d_in;
assign temp4 = ~c_in & d_in;

endmodule
