/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2012  Claire Xenia Wolf <claire@yosyshq.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 *  Modified version of adff2dff for addfe
 */
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
