/* This is the traffic generator module. This
	generate data to be sent over the NoC to the
	traffic processor module*/
	
module traffic_generator(
    clk,
    reset,
    tdata,
    tvalid
);

parameter noc_dw = 32; //NoC Data Width
parameter byte_dw = 8; 

/*****************INPUT/OUTPUT Definition********************/
input wire clk;
input wire reset;

output reg [noc_dw - 1 : 0] tdata;
output reg tvalid;

/******************Sequential Logic*************************/
//a simple counter to test functionality
always @ (posedge clk, posedge reset)
begin

	if(reset == 1'b1) begin
		tdata <= 0;
      tvalid <= 1'b0;
	end
	else begin
		tdata <= tdata + 1;
		tvalid <= 1'b1;
	end
end

endmodule 