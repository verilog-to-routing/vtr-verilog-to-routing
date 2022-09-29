module NoC_adapter(
    master_tready,
    master_tdata,
    master_tstrb,
    master_tkeep,
    master_tid,
    master_tdest,
    master_tuser,
    master_tlast,
    slave_tready,
    slave_tdata,
    slave_tstrb,
    slave_tkeep,
    slave_tid,
    slave_tdest,
    slave_tuser,
    slave_tlast,
);

parameter noc_dw = 32; //NoC Data Width
parameter byte_dw = 8; 

/*****************INPUT/OUTPUT Definition********************/

/*control signal*/


/*Master*/
input wire master_tready;
output wire master_tready;
output wire [noc_dw - 1 : 0] master_tdata;
output wire [noc_dw / byte_dw - 1 : 0] master_tstrb;
output wire [noc_dw / byte_dw - 1 : 0] master_tkeep;
output wire [byte_dw - 1 : 0] master_tid;
output wire [byte_dw - 1 : 0] master_tdest;
output wire [byte_dw - 1 : 0] master_tuser; 
output wire master_tlast;

/*Slave*/
input wire slave_tvalid;
input wire [noc_dw - 1 : 0] slave_tdata;
input wire [noc_dw / byte_dw - 1 : 0] slave_tstrb;
input wire [noc_dw / byte_dw - 1 : 0] slave_tkeep;
input wire [byte_dw - 1 : 0] slave_tid;
input wire [byte_dw - 1 : 0] slave_tdest;
input wire [byte_dw - 1 : 0] slave_tuser; 
input wire slave_tlast;
output wire slave_tready;



//synthesis preserve and keep
//temp clk 
//register the output port

//vqm2blif flow


endmodule 