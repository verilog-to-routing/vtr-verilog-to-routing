module master_interface (
	
	clk,
	reset,
	tvalid_in,
	tdata_in,
	tready,
	tdata_out,
	tvalid_out,
	tstrb,
	tkeep,
	tid,
	tdest,
	tuser,
	tlast
	
);

parameter noc_dw = 32; //NoC Data Width
parameter byte_dw = 8; 


/*****************INPUT/OUTPUT Definition********************/
input wire clk;
input wire reset;
input wire tvalid_in;
input wire [noc_dw - 1 : 0] tdata_in;
input wire tready;
 
output reg tvalid_out;
output reg [noc_dw -1 : 0] tdata_out;
output reg [noc_dw/byte_dw - 1 : 0] tstrb;
output reg [noc_dw/byte_dw - 1 : 0] tkeep;
output reg [byte_dw - 1 : 0] tid;
output reg [byte_dw - 1 : 0] tdest;
output reg [byte_dw - 1 : 0] tuser; 
output reg tlast;

/*******************Internal Variables***********************/
localparam reset_state = 1'b0;
localparam send_data_state = 1'b1;

reg curr_state, next_state;

/******************Sequential Logic*************************/
always @ (posedge clk, posedge reset)
begin

	if(reset == 1'b1)
		curr_state <= reset_state;
	
	else
		curr_state <= next_state;
		
end


/*****************Combinational Logic**********************/
always @ (curr_state,tvalid_in,tready)
begin
	
	tdata_out = tdata_in;
	//future purposes
	tstrb = curr_state;
	tkeep = curr_state;
	tid = curr_state;
	tuser = curr_state;
	tdest = curr_state;
	tlast = curr_state;

	case(curr_state) 
		reset_state: begin
			if(tvalid_in == 1'b0)begin
				next_state = reset_state;
				tvalid_out = 1'b0;
			end
			else begin
				next_state = send_data_state; 
				tvalid_out = 1'b1;
			end
		end
	
		send_data_state: begin
			if(tready == 1'b1) begin 
				next_state = reset_state;
				tvalid_out = 1'b0;
			end
			else begin	
				next_state = send_data_state;
				tvalid_out = 1'b1;
			end
		end
		
		default: begin
			next_state = curr_state;
			tvalid_out = tvalid_out;
		end 
		
	endcase

end

endmodule 