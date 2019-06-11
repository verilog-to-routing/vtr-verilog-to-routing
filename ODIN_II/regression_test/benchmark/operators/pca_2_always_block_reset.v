module FlipflopAssign ( reset, 
	din, 
	clk, 
	qout
);


	input reset;
	input din;
	input clk;

	output qout;

always @ (reset) 
begin

	if (reset)
		assign qout = 0;

	else 

		deassign qout;

end


always @ (posedge clk) 
begin

		qout = din;

end

endmodule
