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
//traffic generator
wire [noc_dw - 1 : 0] tg_data;
wire tg_valid;

//master interface
wire [noc_dw -1 : 0] m_data;
wire m_valid;
wire m_ready;

//NoC Adapter - Connected to slave interface
wire [noc_dw - 1 : 0] na2_data;
wire na2_valid;

//slave interface
wire [noc_dw -1 : 0] s_data;
wire s_valid;
wire s_ready;

/*******************module instantiation********************/
traffic_generator tg(
    .clk(clk),
    .reset(reset),
    .tdata(tg_data),
    .tvalid(tg_valid)
);

master_interface mi (
	.clk(clk),
	.reset(reset),
	.tvalid_in(tg_valid),
	.tdata_in(tg_data),
	.tready(m_ready), 
	.tdata_out(m_data),
	.tvalid_out(m_valid),
	.tstrb(),
	.tkeep(),
	.tid(),
	.tdest(),
	.tuser(),
	.tlast()
);

noc_router_adapter_block na1(
	.clk(clk),
    .reset(reset),
    .master_tready(1'd0),
    .master_tdata(),
	.master_tvalid(),
    .master_tstrb(),
    .master_tkeep(),
    .master_tid(),
    .master_tdest(),
    .master_tuser(),
    .master_tlast(),
    .slave_tvalid(m_valid),
    .slave_tready(m_ready), 
    .slave_tdata(m_data),
    .slave_tstrb(8'd0),
    .slave_tkeep(8'd0),
    .slave_tid(8'd0),
    .slave_tdest(8'd0),
    .slave_tuser(8'd0),
    .slave_tlast(1'd0),

);

noc_router_adapter_block na2(
	.clk(clk),
    .reset(reset),
    .master_tready(s_ready),
    .master_tdata(na2_data),
	.master_tvalid(na2_valid),
    .master_tstrb(),
    .master_tkeep(),
    .master_tid(),
    .master_tdest(),
    .master_tuser(),
    .master_tlast(),
    .slave_tvalid(1'd0),
    .slave_tready(), 
    .slave_tdata(32'd0),
    .slave_tstrb(8'd0),
    .slave_tkeep(8'd0),
    .slave_tid(8'd0),
    .slave_tdest(8'd0),
    .slave_tuser(8'd0),
    .slave_tlast(1'd0),
);

slave_interface si(
	.clk(clk),
	.reset(reset),
	.tvalid_in(na2_valid),
	.tdata_in(na2_data),
	.tready(s_ready),
	.tdata_out(s_data),
	.tvalid_out(s_valid),
	.tstrb(8'd0),
	.tkeep(8'd0),
	.tid(8'd0),
	.tdest(8'd0),
	.tuser(8'd0),
	.tlast(1'd0)
);

traffic_processor tp(
	.clk(clk),
	.reset(reset),
	.tdata(s_data),
	.tvalid(s_valid),
	.o_sum()
);

endmodule