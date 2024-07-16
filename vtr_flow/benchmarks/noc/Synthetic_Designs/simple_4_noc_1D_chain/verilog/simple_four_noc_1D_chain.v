/*
    Top level modules to instantiate an AXI handshake between four routers.
    Each routers receives 32-bit data, processes it, pass it to the next router.
    For now, all of our routers traffic processor's module does the same calculation, 
    but in a more complicated design, we can add different logic to each router's traffic 
    processor module.
*/

module simple_four_noc_1D_chain (
    clk,
    reset,
	data_out
);

parameter noc_dw = 32; //NoC Data Width
parameter byte_dw = 8; 

/*****************INPUT/OUTPUT Definition********************/
input wire clk;
input wire reset;

output wire [noc_dw * 2 - 1:0] data_out;

/*******************Internal Variables**********************/
//traffic generator
wire [noc_dw - 1 : 0] tg_data;
wire tg_valid;

//First master and slave interface
wire [noc_dw -1 : 0] mi_1_data;
wire mi_1_valid;
wire mi_1_ready;

//Second master and slave interface 
wire [noc_dw -1 : 0] si_2_data_in;
wire si_2_valid_in;
wire [noc_dw -1 : 0] si_2_data_out;
wire si_2_valid_out;
wire si_2_ready;
wire [noc_dw -1 : 0] mi_2_data;
wire mi_2_valid;
wire mi_2_ready;

//Second traffic processor
wire [noc_dw - 1 : 0] tp_2_data;
wire tp_2_valid;

//Third master and slave interface 
wire [noc_dw -1 : 0] si_3_data_in;
wire si_3_valid_in;
wire [noc_dw -1 : 0] si_3_data_out;
wire si_3_valid_out;
wire si_3_ready;
wire [noc_dw -1 : 0] mi_3_data;
wire mi_3_valid;
wire mi_3_ready;

//Third traffic processor
wire [noc_dw - 1 : 0] tp_3_data;
wire tp_3_valid;

//Fourth slave interface 
wire [noc_dw -1 : 0] si_4_data_in;
wire si_4_valid_in;
wire [noc_dw -1 : 0] si_4_data_out;
wire si_4_valid_out;
wire si_4_ready;

/*******************module instantiation********************/

/*
    **********************FIRST NOC ADAPTER*****************
    1) Traffic generator passes data to master_interface
    2) master_interface passes data to First NoC adapter
    3) No need for a slave interface in the first NoC adapter
    4) No need for a traffic processor in the first NoC adapter
*/
traffic_generator tg(
    .clk(clk),
    .reset(reset),
    .tdata(tg_data),
    .tvalid(tg_valid)
);

master_interface mi_1 (
	.clk(clk),
	.reset(reset),
	.tvalid_in(tg_valid),
	.tdata_in(tg_data),
	.tready(mi_1_ready), 
	.tdata_out(mi_1_data),
	.tvalid_out(mi_1_valid),
	.tstrb(),
	.tkeep(),
	.tid(),
	.tdest(),
	.tuser(),
	.tlast()
);

noc_router_adapter_block noc_router_adapter_block_one(
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
    .slave_tvalid(mi_1_valid),
    .slave_tready(mi_1_ready), 
    .slave_tdata(mi_1_data),
    .slave_tstrb(8'd0),
    .slave_tkeep(8'd0),
    .slave_tid(8'd0),
    .slave_tdest(8'd0),
    .slave_tuser(8'd0),
    .slave_tlast(1'd0),

);

/*
    **********************Second NOC ADAPTER*****************
    1) Data comes through NoC to the second NoC adapter
    2) NoC adapter passes data to slave interface
    3) slave_interface passes data to traffic processor
    4) traffic processor passes the processed data to master interface
    5) master interface passes the data back to the second NoC adapter
*/

noc_router_adapter_block noc_router_adapter_block_two(
	.clk(clk),
    .reset(reset),
    .master_tready(si_2_ready),
    .master_tdata(si_2_data_in),
	.master_tvalid(si_2_valid_in),
    .master_tstrb(),
    .master_tkeep(),
    .master_tid(),
    .master_tdest(),
    .master_tuser(),
    .master_tlast(),
    .slave_tvalid(mi_2_valid),
    .slave_tready(mi_2_ready), 
    .slave_tdata(mi_2_data),
    .slave_tstrb(8'd0),
    .slave_tkeep(8'd0),
    .slave_tid(8'd0),
    .slave_tdest(8'd0),
    .slave_tuser(8'd0),
    .slave_tlast(1'd0),

);

slave_interface si_2(
	.clk(clk),
	.reset(reset),
	.tvalid_in(si_2_valid_in),
	.tdata_in(si_2_data_in),
	.tready(si_2_ready),
	.tdata_out(si_2_data_out),
	.tvalid_out(si_2_valid_out),
	.tstrb(8'd0),
	.tkeep(8'd0),
	.tid(8'd0),
	.tdest(8'd0),
	.tuser(8'd0),
	.tlast(1'd0)
);

traffic_processor tp_2(
	.clk(clk),
	.reset(reset),
	.tdata_in(si_2_data_out),
	.tvalid_in(si_2_valid_out),
	.tdata_out(tp_2_data),
	.tvalid_out(tp_2_valid)
);

master_interface mi_2 (
	.clk(clk),
	.reset(reset),
	.tvalid_in(tp_2_valid),
	.tdata_in(tp_2_data),
	.tready(mi_2_ready), 
	.tdata_out(mi_2_data),
	.tvalid_out(mi_2_valid),
	.tstrb(),
	.tkeep(),
	.tid(),
	.tdest(),
	.tuser(),
	.tlast()
);


/*
    **********************Third NOC ADAPTER*****************
    1) Data comes through NoC to the third NoC adapter
    2) NoC adapter passes data to slave interface
    3) slave_interface passes data to traffic processor
    4) traffic processor passes the processed data to master interface
    5) master interface passes the data back to the third NoC adapter
*/

noc_router_adapter_block noc_router_adapter_block_three(
	.clk(clk),
    .reset(reset),
    .master_tready(si_3_ready),
    .master_tdata(si_3_data_in),
	.master_tvalid(si_3_valid_in),
    .master_tstrb(),
    .master_tkeep(),
    .master_tid(),
    .master_tdest(),
    .master_tuser(),
    .master_tlast(),
    .slave_tvalid(mi_3_valid),
    .slave_tready(mi_3_ready), 
    .slave_tdata(mi_3_data),
    .slave_tstrb(8'd0),
    .slave_tkeep(8'd0),
    .slave_tid(8'd0),
    .slave_tdest(8'd0),
    .slave_tuser(8'd0),
    .slave_tlast(1'd0),

);

slave_interface si_3(
	.clk(clk),
	.reset(reset),
	.tvalid_in(si_3_valid_in),
	.tdata_in(si_3_data_in),
	.tready(si_3_ready),
	.tdata_out(si_3_data_out),
	.tvalid_out(si_3_valid_out),
	.tstrb(8'd0),
	.tkeep(8'd0),
	.tid(8'd0),
	.tdest(8'd0),
	.tuser(8'd0),
	.tlast(1'd0)
);

traffic_processor tp_3(
	.clk(clk),
	.reset(reset),
	.tdata_in(si_3_data_out),
	.tvalid_in(si_3_valid_out),
	.tdata_out(tp_3_data),
	.tvalid_out(tp_3_valid)
);

master_interface mi_3 (
	.clk(clk),
	.reset(reset),
	.tvalid_in(tp_3_valid),
	.tdata_in(tp_3_data),
	.tready(mi_3_ready), 
	.tdata_out(mi_3_data),
	.tvalid_out(mi_3_valid),
	.tstrb(),
	.tkeep(),
	.tid(),
	.tdest(),
	.tuser(),
	.tlast()
);

/*
    **********************Fourth NOC ADAPTER*****************
    1) Data comes through NoC to the fourth NoC adapter
    2) NoC adapter passes data to slave interface
    3) slave_interface passes data to traffic processor
    4) traffic processor passes the processed data to the top module output
*/

noc_router_adapter_block noc_router_adapter_block_four(
	.clk(clk),
    .reset(reset),
    .master_tready(si_4_ready),
    .master_tdata(si_4_data_in),
	.master_tvalid(si_4_valid_in),
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

slave_interface si_4(
	.clk(clk),
	.reset(reset),
	.tvalid_in(si_4_valid_in),
	.tdata_in(si_4_data_in),
	.tready(si_4_ready),
	.tdata_out(si_4_data_out),
	.tvalid_out(si_4_valid_out),
	.tstrb(8'd0),
	.tkeep(8'd0),
	.tid(8'd0),
	.tdest(8'd0),
	.tuser(8'd0),
	.tlast(1'd0)
);

traffic_processor tp_4(
	.clk(clk),
	.reset(reset),
	.tdata_in(si_4_data_out),
	.tvalid_in(si_4_valid_out),
	.tdata_out(data_out),
	.tvalid_out()
);


endmodule