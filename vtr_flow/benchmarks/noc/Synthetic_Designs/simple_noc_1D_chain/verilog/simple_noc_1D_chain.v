module simple_noc_1D_chain (
    clk,
    reset
);

parameter noc_dw = 32; //NoC Data Width
parameter byte_dw = 8; 

/*****************INPUT/OUTPUT Definition********************/
input wire clk;
input wire reset;

/*******************Internal Variables**********************/
wire [noc_dw - 1 : 0] tdata;
wire tvalid;
wire tready;

wire [noc_dw - 1 : 0] master_tdata_out;
wire master_tvalid_out;

wire [noc_dw - 1 : 0] slave_tdata_out;
wire slave_tvalid_out;


/*******************module instantiation********************/
traffic_generator tg(
    .clk(clk),
    .reset(reset),
    .tdata(tdata),
    .tvalid(tvalid)
);

master_interface mi (
	
	.clk(clk),
	.reset(reset),
	.tvalid_in(tvalid),
	.tdata_in(tdata),
	.tready(tready),
	.tdata_out(master_tdata_out),
	.tvalid_out(master_tvalid_out),
	.tstrb(),
	.tkeep(),
	.tid(),
	.tdest(),
	.tuser(),
	.tlast()
	
);

slave_interface si(
	.clk(clk),
	.reset(reset),
	.tvalid_in(master_tvalid_out),
	.tdata_in(master_tdata_out),
	.tready(tready),
	.tdata_out(slave_tdata_out),
	.tvalid_out(slave_tvalid_out),
	.tstrb(),
	.tkeep(),
	.tid(),
	.tdest(),
	.tuser(),
	.tlast()
);

endmodule