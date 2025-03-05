module noc_router_adapter_block(
    clk,
    reset,
    master_tready,
    master_tdata, /* synthesis preserve */
    master_tvalid, /* synthesis preserve */
    master_tstrb, /* synthesis preserve */
    master_tkeep, /* synthesis preserve */
    master_tid, /* synthesis preserve */
    master_tdest, /* synthesis preserve */
    master_tuser, /* synthesis preserve */
    master_tlast, /* synthesis preserve */
    slave_tvalid,
    slave_tready, /* synthesis preserve */
    slave_tdata,
    slave_tstrb,
    slave_tkeep,
    slave_tid,
    slave_tdest,
    slave_tuser,
    slave_tlast,
);

parameter noc_dw = 512; //NoC Data Width
parameter byte_dw = 8;
parameter user_dw = 32; 

/*****************INPUT/OUTPUT Definition********************/

/*control signal*/
input wire clk;
input wire reset;

/*Master*/
input wire master_tready;
output reg master_tvalid;
output reg [noc_dw - 1 : 0] master_tdata;
output reg [noc_dw / byte_dw - 1 : 0] master_tstrb;
output reg [noc_dw / byte_dw - 1 : 0] master_tkeep;
output reg [byte_dw - 1 : 0] master_tid;
output reg [byte_dw - 1 : 0] master_tdest;
output reg [user_dw - 1 : 0] master_tuser;
output reg master_tlast;

/*Slave*/
input wire slave_tvalid;
input wire [noc_dw - 1 : 0] slave_tdata;
input wire [noc_dw / byte_dw - 1 : 0] slave_tstrb;
input wire [noc_dw / byte_dw - 1 : 0] slave_tkeep;
input wire [byte_dw - 1 : 0] slave_tid;
input wire [byte_dw - 1 : 0] slave_tdest;
input wire [user_dw - 1 : 0] slave_tuser; 
input wire slave_tlast;
output reg slave_tready;

// registering all the out put ports so they are preserved in the vqm output //

wire temp_clk; /* synthesis keep */

always @(posedge temp_clk)
begin
	master_tvalid	<= 0;
	master_tdata 	<= 0;
	master_tstrb 	<= 0;
	master_tkeep 	<= 0;
	master_tid		<= 0;
	master_tdest	<= 0;
	master_tuser	<= 0;
	master_tlast	<= 0;
	slave_tready	<= 0; 
end
endmodule 
