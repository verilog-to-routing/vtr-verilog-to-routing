module register (
	input		    clk,
	input       [2:0] a,
	output reg  [2:0] w,
	output reg  [2:0] q
);

	
	always@(posedge clk)
	begin
		w <= a;
	end
	
	always@(negedge clk)
	begin
		q <= a;
	end


endmodule
