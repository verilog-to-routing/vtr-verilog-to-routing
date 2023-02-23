(* techmap_celltype = "$aldff" *)
module aldff2dff (CLK, ALOAD, AD, D, Q);
	parameter WIDTH = 1;
	parameter CLK_POLARITY = 1;
	parameter ALOAD_POLARITY = 1;

	input CLK, ALOAD;
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

	if (CLK_POLARITY)
		always @(posedge CLK)
			Q <= NEXT_Q;
	else
		always @(negedge CLK)
			Q <= NEXT_Q;
endmodule
