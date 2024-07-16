/*********************************************************/
// MODULE:		j-k flip flop
//
// FILE NAME:	jk_rtl.v
// VERSION:		1.0
// DATE:		January 1, 1999
// AUTHOR:		Bob Zeidman, Zeidman Consulting
// 
// CODE TYPE:	Register Transfer Level
//
// DESCRIPTION:	This module defines a J-K flip flop with an
// asynchronous, active low reset.
//
/*********************************************************/

// TOP MODULE
module	bm_jk_rtl(
		clk,
		clr_n,
		j,
		k,
		q,
		q_n);

// INPUTS
input	clk;		// Clock
input	clr_n;		// Active low, asynchronous reset
input	j;			// J input
input	k;			// K input

// OUTPUTS
output	q;			// Output
output	q_n;		// Inverted output

// INOUTS

// SIGNAL DECLARATIONS
wire	clk;
wire	clr_n;
wire	j;
wire	k;
reg		q;
wire	q_n;

// PARAMETERS
// Define the J-K input combinations as parameters
parameter[1:0]	HOLD	= 0,
				RESET	= 1,
				SET		= 2,
				TOGGLE	= 3;

// ASSIGN STATEMENTS
assign q_n = ~q;

// MAIN CODE

// Look at the falling edge of clock for state transitions
always @(negedge clk or negedge clr_n) begin
	if (~clr_n) begin
		// This is the reset condition. Most synthesis tools
		// require an asynchronous reset to be defined this
		// way.
		q <= 1'b0;
	end
	else begin
		case ({j,k})
			RESET:	q <= 1'b0;

			SET:	q <= 1'b1;

			TOGGLE:	q <= ~q;
		endcase
	end
end

endmodule		// jk_FlipFlop
