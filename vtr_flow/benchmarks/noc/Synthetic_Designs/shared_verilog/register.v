module register (
    clk,
    reset,
	data_in,/* synthesis preserve */
	data_out/* synthesis preserve */
);


parameter noc_dw = 32; //NoC Data Width
parameter byte_dw = 8; 

/*****************INPUT/OUTPUT Definition********************/
input wire clk;
input wire reset;

input wire [noc_dw - 1:0] data_in; 
output reg [noc_dw  - 1:0] data_out;

/******************Sequential Logic*************************/
always@(posedge clk) begin
	if(reset)
		data_out <= 32'd0;
	else
		data_out <= data_in;
end 

endmodule 