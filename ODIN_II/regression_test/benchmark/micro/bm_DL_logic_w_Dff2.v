/* pg 393 - logic w DFF 2 */

module bm_DL_logic_w_Dff2(x1, x2, x3, Clock, f, g);
	input x1, x2, x3, Clock;
	output f, g;
	reg f, g;
	
	always @(posedge Clock)
	begin
		f <= x1 & x2;
		g <= f | x3;
	end
	
endmodule
