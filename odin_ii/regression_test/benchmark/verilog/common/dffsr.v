module dffsr (
	input		    clk,
	input			rst,
	input       [1:0]a,b,
	output reg  [1:0] w
);

	
	always@(posedge rst, posedge clk)
	begin
		if (rst)
			w <= b;
		else
			w <= a;
	end


endmodule
