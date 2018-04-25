
module inferred_ram (clk,a,b,reset);
	input reset;
	input clk;
	input [31:0] a;
	output [31:0] b;

	reg  [31:0] mregs [8:0];  
	reg [8:0] adr;

	reg [31:0] b;

always @ (posedge clk )
	if (!reset)
	begin
	mregs [adr] <= a;
	b <= mregs [adr-1];
	adr <= adr+1;
	end
	else
	adr <= 8'b00000000;

endmodule
