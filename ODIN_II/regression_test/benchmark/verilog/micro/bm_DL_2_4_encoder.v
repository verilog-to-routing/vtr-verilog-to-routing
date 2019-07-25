/* pg 327 - 2 to 4 encoder */

module bm_DL_2_4_encoder (W, Y, En);
	input [1:0] W;
	input En;
	output [3:0] Y;
	reg [3:0] Y;
	
	always @(W or En)
		case ({En, W})
			3'b100: Y = 4'b1000;
			3'b101: Y = 4'b0100;
			3'b110: Y = 4'b0010;
			3'b111: Y = 4'b0001;
			default: Y = 4'b0000;
		endcase
	
endmodule
