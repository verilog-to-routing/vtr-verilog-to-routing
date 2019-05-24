module test(r ,
	    clk,
	    q 
	    );

	input r , clk;

	output q;

	reg q;

	always @ (posedge clk)
	begin

		q = r ? 0 : 1 ;

	end
endmodule
