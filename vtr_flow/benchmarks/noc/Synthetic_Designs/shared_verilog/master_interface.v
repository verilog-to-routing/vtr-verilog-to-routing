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
 
output wire tvalid_out;
output wire [noc_dw -1 : 0] tdata_out;
output wire [noc_dw/byte_dw - 1 : 0] tstrb;
output wire [noc_dw/byte_dw - 1 : 0] tkeep;
output wire [byte_dw - 1 : 0] tid;
output wire [byte_dw - 1 : 0] tdest;
output wire [byte_dw - 1 : 0] tuser; 
output wire tlast;

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
	
	case(curr_state) 
		reset_state: begin
			if(tvalid_in == 1'b0)
				next_state = reset_state;
			else
				next_state = send_data_state; 
		end
	
		send_data_state: begin
			if(tready == 1'b1) 
				next_state = reset_state;
			else	
				next_state = send_data_state;
		end
		
		default: begin
			next_state = curr_state;
		end 
		
	endcase

end

/*****************Output Logic******************************/
assign tvalid_out = tvalid_in & curr_state;
assign tdata_out = tdata_in & {32{curr_state}};
//future purposes
assign tstrb = curr_state;
assign tkeep = curr_state;
assign tid = curr_state;
assign tuser = curr_state;
assign tdest = curr_state;
assign tlast = curr_state;

endmodule 