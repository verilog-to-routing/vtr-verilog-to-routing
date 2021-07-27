(* techmap_celltype = "$adffe" *)
module adffe2dff (CLK, ARST, EN, D, Q);
	parameter WIDTH = 1;
	parameter CLK_POLARITY = 1;
	parameter ARST_POLARITY = 1;
	parameter EN_POLARITY = 1;
	parameter ARST_VALUE = 0;

	input CLK, ARST, EN;
	(* force_downto *)
	input [WIDTH-1:0] D;
	(* force_downto *)
	output reg [WIDTH-1:0] Q;
	(* force_downto *)
	reg [WIDTH-1:0] NEXT_Q;

	wire [1023:0] _TECHMAP_DO_ = "proc;;";

	always @*
		if (ARST == ARST_POLARITY)
			NEXT_Q <= ARST_VALUE;
		else
			NEXT_Q <= D;

	if (CLK_POLARITY) begin
		always @(posedge CLK)
			if (EN == EN_POLARITY)
				Q <= NEXT_Q;
	end else begin
		always @(negedge CLK)
			if (EN == EN_POLARITY)
				Q <= NEXT_Q;
	end
endmodule
