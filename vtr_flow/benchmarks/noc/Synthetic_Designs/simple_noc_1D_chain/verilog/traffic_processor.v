/* This is the traffic processor module. This
	accepts data coming in from the NoC and 
	accumulates it.
*/
module traffic_processor (
									i_clk,
									i_reset,
									i_data,
									i_data_valid,
									o_sum);

// parameter declarations
parameter 				INPUT_DATA_SIZE	= 32;

// port declarations
input											i_clk				;
input											i_reset			;
input		[INPUT_DATA_SIZE-1:0]		i_data			;
input											i_data_valid	;

output	[INPUT_DATA_SIZE*2-1:0]		o_sum				;

// port wires
wire 											i_clk				;
wire 											i_reset			;
wire 		[INPUT_DATA_SIZE-1:0]		i_data			;
wire 											i_data_valid	;
wire 		[INPUT_DATA_SIZE*2-1:0]		o_sum				;


/*
	This module will wait on the i_data_valid signal
	to indicate whether data is available to read
	in from the input. When the data is read in, it is
	then added to the output signal. The output will act
	as an accumulator.

*/

// internal variables
reg [INPUT_DATA_SIZE*2-1:0]		sum_reg;
wire [INPUT_DATA_SIZE*2-1:0]		data_extended;

// make the input data size equal to the output data size
assign data_extended = {{INPUT_DATA_SIZE{1'b0}}, i_data};

// handle the accumulation
always @(posedge i_clk)
begin
	if (i_reset)
		begin
		sum_reg <= 0;
		end
	else
		begin
			if (i_data_valid)
			begin
			sum_reg <= sum_reg + data_extended;
			end
		end
end
		
// now assign the accumulation to the output
assign o_sum = sum_reg;

endmodule 
						

























