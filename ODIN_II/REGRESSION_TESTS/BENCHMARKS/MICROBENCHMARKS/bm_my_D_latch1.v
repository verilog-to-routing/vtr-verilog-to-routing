/* My D latch with attempts to bypass icarus FF inference */
/* Not a valid design since Q is not hooked up for QUARTUS, or currently my tool */

module bm_my_D_latch1(D, C, Q, reset);
	input D, C;
	input reset;
	output Q;
	reg Q;
	
	always @(*)
	begin
		if (~reset)
		begin
			Q = 0;
		end
		else
		begin
			Q = D;
		end
	end
endmodule
