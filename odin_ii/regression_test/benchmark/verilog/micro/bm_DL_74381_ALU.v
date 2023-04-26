/* pg 330 - 74381 ALU */
module bm_DL_74381_ALU(s, A, B, F);
	input [2:0] s;
	input [3:0] A, B;
	output [3:0] F;
	reg [3:0] F;
	
	always @(s or A or B)
		case (s)
			3'b000: F = 4'b0000; 
	   		3'b001: F = B - A;
			3'b010: F = A - B;
			3'b011: F = A + B;
			3'b100: F = A ^ B;
			3'b101: F = A | B;
			3'b110: F = A & B;
			3'b111: F = 4'b1111;
		endcase
		
endmodule
