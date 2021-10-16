`define WIDTH 2

module multiplexer (
	input 	 			 sel1,
	input 	 			 sel2,
	input   [`WIDTH-1:0] x_top, 
	input   [`WIDTH-1:0] y_top,
	input   [`WIDTH-1:0] z_top,
	output  [`WIDTH-1:0] q_top
);

	assign q_top = (sel1) ? x_top : 
				   (sel2) ? y_top :
				   			z_top;

endmodule
