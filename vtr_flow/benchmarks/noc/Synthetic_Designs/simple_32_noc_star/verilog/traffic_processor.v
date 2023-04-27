/* This is the traffic processor module. This
	accepts data coming in from the NoC and 
	accumulates it.
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

output wire [noc_dw - 1 :0] tdata_out;
output tvalid_out;

/*******************Internal Variables**********************/
reg [noc_dw*2-1:0]		sum_reg;
reg valid_reg;
wire [noc_dw*2-1:0]		data_extended;

/******************Sequential Logic*************************/
/*
	This module will wait on the tvalid signal
	to indicate whether data is available to read
	in from the input. When the data is read in, it is
	then added to the output signal. The output will act
	as an accumulator.

*/
assign data_extended = {{noc_dw{1'b0}}, tdata_in};
// handle the accumulation
always @(posedge clk)
begin
	if (reset)begin
			sum_reg <= 0;
			valid_reg <= 1'b0;
			
		end
	else begin
			if (tvalid_in == 1'b1) begin
				sum_reg <= sum_reg + data_extended;
				valid_reg <= 1'b1;
			end
		end
end
		
/*******************Output Logic***************************/
assign tdata_out = sum_reg[31:0];
assign tvalid_out = valid_reg;

endmodule 
						

























