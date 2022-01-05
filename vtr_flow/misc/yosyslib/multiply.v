/****************************************************************
# Description: definition of the hard multiplier black-box		#
#																#
# Author: Eddie Hung											#
# VTR-to-Bitstream: "http://eddiehung.github.io/vtb.html"	#
****************************************************************/

(* blackbox *)
module multiply(a, b, out);
	parameter A_WIDTH = 36;
	parameter B_WIDTH = 36;
	parameter Y_WIDTH = A_WIDTH+B_WIDTH;
	
	input [A_WIDTH-1:0] a;
	input [B_WIDTH-1:0] b;
	output [Y_WIDTH-1:0] out;

	//assign out = a * b;
	
endmodule
