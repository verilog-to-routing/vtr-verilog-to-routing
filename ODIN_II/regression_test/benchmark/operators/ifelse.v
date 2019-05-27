module test(r ,
	    clk,
	    q 
	    );

	input r , clk;

	output q;

	reg q;

	always @ (posedge clk)
	begin
	
	if (r == 1)
	 q =  0;
	else 
	 q = 1;


	end
endmodule
