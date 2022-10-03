/* This is the traffic processor module. This
	accepts data coming in from the NoC and 
	accumulates it.
*/
module traffic_processor (
	clk,
	reset,
	tdata,
	tvalid,
	o_sum
);

/*****************Parameter Declarations********************/
parameter INPUT_DATA_SIZE = 32;

/*****************INPUT/OUTPUT Definition********************/
input wire clk;
input wire reset;
input wire [INPUT_DATA_SIZE-1:0] tdata;
input tvalid;

output wire [INPUT_DATA_SIZE*2-1:0] o_sum;


/*
	This module will wait on the tvalid signal
	to indicate whether data is available to read
	in from the input. When the data is read in, it is
	then added to the output signal. The output will act
	as an accumulator.

*/

/*******************Internal Variables**********************/
reg [INPUT_DATA_SIZE*2-1:0]		sum_reg;
wire [INPUT_DATA_SIZE*2-1:0]		data_extended;

/******************Sequential Logic*************************/
// make the input data size equal to the output data size
assign data_extended = {{INPUT_DATA_SIZE{1'b0}}, tdata};
// handle the accumulation
always @(posedge clk)
begin
	if (reset)begin
			sum_reg <= 0;
		end
	else begin
			if (tvalid) begin
				sum_reg <= sum_reg + data_extended;
			end
		end
end
		
// now assign the accumulation to the output
assign o_sum = sum_reg;

endmodule 
						

























