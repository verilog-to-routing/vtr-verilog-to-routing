/* This is the traffic processor module. This
	accepts data coming in from the NoC and 
	encrypts it using sha algorithm
*/

module traffic_processor (
	clk,
	reset,
	tdata,
	tvalid,
	o_enc
);

/*****************Parameter Declarations********************/
parameter noc_dw = 32;

/*****************INPUT/OUTPUT Definition*******************/
input wire clk;
input wire reset;
input wire [noc_dw-1:0] tdata;
input tvalid;

output reg [noc_dw-1:0] o_enc;

/*******************Internal Variables**********************/
wire [noc_dw - 1 : 0] sha_out;
wire [noc_dw - 1 : 0] sha2_out;


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

sha1 sha2_module 
(
	.clk_i(clk),
	.rst_i(reset),
	.text_i(sha_out),
	.text_o(sha2_out),
	.cmd_i({{1{1'b0}},{2{tvalid}}}),
	.cmd_w_i(tvalid), 
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
			o_enc <= 0;
		end
	else begin
			if (tvalid) begin
				o_enc <= sha2_out;
			end
		end
end
		
endmodule 