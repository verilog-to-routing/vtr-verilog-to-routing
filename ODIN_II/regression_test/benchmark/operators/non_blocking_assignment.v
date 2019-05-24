module NonBlockingAssignment (reset, set, a, q);

	input reset;
	input set;
	input a;

	output q; 

	reg q;

	always @ (*)
	begin 
		if (reset)
		q <=1'b0;

		else

		   if (set)
		   q <= 1'b1;

		   else
		   q  <= a;
 
	end

endmodule
