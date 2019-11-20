/* pg 392 - 2 cascaded flip flops */

module bm_DL_2_cascaded_flip_flops(D, Clock, Q1, Q2);
	input D, Clock;
	output Q1, Q2;
	reg Q1, Q2;
	
	always @(posedge Clock)
	begin
		Q1 <= D;
		Q2 <= Q1;
	end
	
endmodule
