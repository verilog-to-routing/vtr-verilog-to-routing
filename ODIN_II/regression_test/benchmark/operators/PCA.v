module PCA (reset, set, a, q);

	input reset;
	input set;
	input a;

	output q; 

	reg q;

	always @ (*)
	begin 
		if (reset)
		assign =1'b0;

		else

		   if (set)
		   assign = 1'b1;

		   else
		   begin
		   deassign q;
		   
		   q <= a; 
	end

endmodule
