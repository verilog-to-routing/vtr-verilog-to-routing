(* techmap_celltype = "$aldffe" *)
module aldffe2dff (CLK, ALOAD, AD, D, EN, Q);
	parameter WIDTH = 1;
	parameter CLK_POLARITY = 1;
	parameter ALOAD_POLARITY = 1;
	parameter EN_POLARITY = 1;

	input CLK, ALOAD, EN;
    (* force_downto *)
	input [WIDTH-1:0] AD;
	(* force_downto *)
	input [WIDTH-1:0] D;
	(* force_downto *)
	output reg [WIDTH-1:0] Q;
	(* force_downto *)
	reg [WIDTH-1:0] NEXT_Q;

	wire [1023:0] _TECHMAP_DO_ = "proc;;";

	always @*
		if (ALOAD == ALOAD_POLARITY)
			NEXT_Q <= AD;
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
