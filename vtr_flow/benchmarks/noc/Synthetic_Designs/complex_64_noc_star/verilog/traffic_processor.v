/* This is the traffic processor module. This
	accepts data coming in from the NoC and 
	encrypts it using sha algorithm
*/

module traffic_processor (
	clk,
	reset,
	tdata_in,
	tvalid_in,
	tdata_out,
	tvalid_out
);

/*****************Parameter Declarations********************/
parameter noc_dw = 32;

/*****************INPUT/OUTPUT Definition*******************/
input wire clk;
input wire reset;
input wire [noc_dw-1:0] tdata_in;
input tvalid_in;

output reg [noc_dw-1:0] tdata_out;
output reg tvalid_out;

/*******************Internal Variables**********************/
wire [noc_dw - 1 : 0] sha_out;
reg valid_reg;

/*******************module instantiation*******************/
sha1 sha1_module
(
	.clk_i(clk),
	.rst_i(reset),
	.text_i(tdata),
	.text_o(sha_out),
	.cmd_i({{2{1'b0}},{2{tvalid}}}),
	.cmd_w_i(tvalid),
	.cmd_o()
);

/******************Sequential Logic*************************/
/*
	This module will wait on the tvalid signal
	to indicate whether data is available to read
	in from the input. When the data is read in, it is
	then added to the output signal. The output will act
	as an accumulator.

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
						

























