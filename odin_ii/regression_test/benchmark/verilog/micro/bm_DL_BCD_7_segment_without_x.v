/* pg 329 - seven segment BCD decoder.  MODIFIED to get rid of default x */

module bm_DL_BCD_7_segment_without_x (bcd, leds);
	input [3:0] bcd;
	output [7:1] leds;
	reg [7:1] leds;
	
	always @(bcd)
		case (bcd)       //abcdefg
			0: leds = 7'b1111110;
	   		1: leds = 7'b0110000;
			2: leds = 7'b1101101;
			3: leds = 7'b1111001;
			4: leds = 7'b0110011;
			5: leds = 7'b1011011;
			6: leds = 7'b1011111;
			7: leds = 7'b1110000;
			8: leds = 7'b1111111;
			9: leds = 7'b1111011;
			default: leds = 7'b1111111;
		endcase
		
endmodule
