module slave_interface (
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

//future purposes
input wire [noc_dw/byte_dw - 1 : 0] tstrb;
input wire [noc_dw/byte_dw - 1 : 0] tkeep;
input wire [byte_dw - 1 : 0] tid;
input wire [byte_dw - 1 : 0] tdest;
input wire [byte_dw - 1 : 0] tuser; 
input wire tlast;

output reg tready;
output reg tvalid_out;
output reg [noc_dw - 1 : 0] tdata_out;

/*******************Internal Variables***********************/
localparam reset_state = 1'b0;
localparam send_ready_state = 1'b1;

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
always @ (curr_state,tvalid_in)
begin
	tdata_out = tdata_in;
	tvalid_out = tvalid_in; 
	case(curr_state) 
		reset_state: begin
			if(tvalid_in == 1'b0)begin
				next_state = reset_state;
				tready = 1'b0;
			end
			else begin
				next_state = send_ready_state; 
				tready = 1'b1;
			end
		end
	
		send_ready_state: begin
			next_state = reset_state;
			tready = 1'b0;
		end
		
		default: begin
			next_state = curr_state;
			tready = tready;
		end 
		
	endcase

end


endmodule 