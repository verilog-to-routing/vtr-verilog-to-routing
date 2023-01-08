/****************************************************************
# Description: definition of the hard multiplier black-box		#
#																#
# Author: Eddie Hung											#
# VTR-to-Bitstream: "http://eddiehung.github.io/vtb.html"	#
****************************************************************/

(* blackbox *)
module adder(a, b, cin, cout, sumout);
	input a, b, cin;
	output cout, sumout;
	
	//assign {cout,sumout} = a + b + cin;
endmodule
