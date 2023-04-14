/* This is the traffic processor module. This
	accepts data coming in from the NoC and 
	encrypts it using sha algorithm
*/

module traffic_processor (
	clk, 
	reset, /* synthesis preserve */
	tdata_in,  /* synthesis preserve */
	tvalid_in, /* synthesis preserve */
	tdata_out, /* synthesis preserve */
	tvalid_out /* synthesis preserve */
);

/*****************Parameter Declarations********************/
parameter noc_dw = 32;

/*****************INPUT/OUTPUT Definition*******************/
input wire clk; /* synthesis keep */
input wire reset; /* synthesis keep */
input wire [noc_dw-1:0] tdata_in; /* synthesis keep */
input tvalid_in; /* synthesis keep */

output reg [noc_dw-1:0] tdata_out; /* synthesis keep */
output reg tvalid_out; /* synthesis keep */

/*******************Internal Variables**********************/
wire [noc_dw - 1 : 0] sha_out; /* synthesis keep */
reg valid_reg; /* synthesis keep */

/*******************module instantiation*******************/
sha1 sha1_module 
(
	.clk_i(clk),
	.rst_i(reset),
	.text_i(tdata_in),
	.text_o(sha_out),
	.cmd_i({{1{1'b0}},{2{tvalid_in}}}),
	.cmd_w_i(tvalid_in), 
	.cmd_o()
);

/******************Sequential Logic*************************/
/*
	This module will wait on the tvalid signal
	to indicate whether data is available to read
	in from the input. 
*/

always @(posedge clk)
begin
	if (reset)begin
			tdata_out <= 0;
			tvalid_out <= 0;
		end
	else begin
			if (tvalid_in) begin
				tdata_out <= sha_out;
				tvalid_out <= 1'b1;
			end
		end
end
		
endmodule 
						

























