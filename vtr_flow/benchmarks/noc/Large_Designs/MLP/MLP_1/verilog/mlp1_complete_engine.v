/*
    Top level module to connect a number of MVM modules together using a
    NoC. The design implements the following:
	1) A number of dispatcher modules used to generate initial traffic
	2) A number of MVM modules that perform matrix vector multiplication
	   on input data streams
	3) A collector module to process the results of the MVM modules
	4) A numbe of NoC routers that are used to connect all the modules
	   added above. The NoC routers are responsible for transferring data
	   from dispatchers to MVM modules, between MVM modules and from MVM
	   modules to the collector module.
*/

/* This file contain whole mlp engine (contain all submodule definitions) that can go through Odin-II for synthesis without any changes */

module mlp_1 (
    clk,
    reset,
    rx_top_tvalid,
	rx_top_tdata,
	rx_top_tstrb,
	rx_top_tkeep,
	rx_top_tid,
	rx_top_tdest,
	rx_top_tuser,
	rx_top_tlast,
	dispatcher0_ififo_wdata, // external I/O input for dispatcher
	dispatcher0_ififo_wen, // external I/O input for dispatcher
	dispatcher1_ififo_wdata, // external I/O input for dispatcher
	dispatcher1_ififo_wen, // external I/O input for dispatcher
	dispatcher2_ififo_wdata, // external I/O input for dispatcher
	dispatcher2_ififo_wen, // external I/O input for dispatcher
	dispatcher3_ififo_wdata, // external I/O input for dispatcher
	dispatcher3_ififo_wen, // external I/O input for dispatcher
	collector_ofifo_rdata, // external I/O output for collector
	collector_ofifo_ren, // external I/O input for collector
	dispatcher0_ififo_rdy, // external I/O output for dispatcher
	dispatcher1_ififo_rdy, // external I/O output for dispatcher
	dispatcher2_ififo_rdy, // external I/O output for dispatcher
	dispatcher3_ififo_rdy, // external I/O output for dispatcher
	collector_ofifo_rdy, // external I/O output for collector
	tx_top_tvalid,
	tx_top_tdata,
	master_tdata_top_out,
	tx_top_tstrb,
	tx_top_tkeep,
	tx_top_tid,
	tx_top_tdest,
	tx_top_tuser,
	tx_top_tlast
);

parameter noc_dw = 512; //NoC Data Width
parameter byte_dw = 8; 

/*****************INPUT/OUTPUT Definition********************/
input wire clk;
input wire reset;
 
input wire [noc_dw/byte_dw - 1:0] 	dispatcher0_ififo_wdata;
input wire 									dispatcher0_ififo_wen;
input wire [noc_dw/byte_dw - 1:0] 	dispatcher1_ififo_wdata;
input wire 									dispatcher1_ififo_wen;
input wire [noc_dw/byte_dw - 1:0] 	dispatcher2_ififo_wdata;
input wire 									dispatcher2_ififo_wen;
input wire [noc_dw/byte_dw - 1:0] 	dispatcher3_ififo_wdata;
input wire 									dispatcher3_ififo_wen;

input wire 									collector_ofifo_ren;

input wire          rx_top_tvalid;
input wire  [511:0] rx_top_tdata;
input wire  [ 63:0] rx_top_tstrb;
input wire  [ 63:0] rx_top_tkeep;
input wire  [  7:0] rx_top_tid;
input wire  [  7:0] rx_top_tdest;
input wire  [ 31:0] rx_top_tuser;
input wire          rx_top_tlast;

output wire          tx_top_tvalid;
output wire  [511:0] tx_top_tdata;
output wire  [ 63:0] tx_top_tstrb;
output wire  [ 63:0] tx_top_tkeep;
output wire  [  7:0] tx_top_tid;
output wire  [  7:0] tx_top_tdest;
output wire  [ 31:0] tx_top_tuser;
output wire          tx_top_tlast;
output wire [noc_dw - 1: 0] master_tdata_top_out;


output wire [noc_dw/byte_dw - 1:0]	collector_ofifo_rdata;
output wire									collector_ofifo_rdy;

output wire									dispatcher0_ififo_rdy;
output wire									dispatcher1_ififo_rdy;
output wire									dispatcher2_ififo_rdy;
output wire									dispatcher3_ififo_rdy;

/*******************Internal Variables**********************/
// dispatcher signals
// dispatcher0
wire 							dispatcher0_tx_valid;
wire [noc_dw - 1 : 0]  	dispatcher0_tx_tdata;
wire [byte_dw - 1 : 0] 	dispatcher0_tx_tstrb;
wire [byte_dw - 1 : 0] 	dispatcher0_tx_tkeep;
wire [7 : 0] 				dispatcher0_tx_tid;
wire [7 : 0] 				dispatcher0_tx_tdest;
wire [31 : 0] 				dispatcher0_tx_tuser;
wire 							dispatcher0_tx_tlast;

wire 							dispatcher0_connector_tx_tready;

// dispatcher1
wire 							dispatcher1_tx_valid;
wire [noc_dw - 1 : 0]  	dispatcher1_tx_tdata;
wire [byte_dw - 1 : 0] 	dispatcher1_tx_tstrb;
wire [byte_dw - 1 : 0] 	dispatcher1_tx_tkeep;
wire [7 : 0] 				dispatcher1_tx_tid;
wire [7 : 0] 				dispatcher1_tx_tdest;
wire [31 : 0] 				dispatcher1_tx_tuser;
wire 							dispatcher1_tx_tlast;

wire 							dispatcher1_connector_tx_tready;

// dispatcher2
wire 							dispatcher2_tx_valid;
wire [noc_dw - 1 : 0]  	dispatcher2_tx_tdata;
wire [byte_dw - 1 : 0] 	dispatcher2_tx_tstrb;
wire [byte_dw - 1 : 0] 	dispatcher2_tx_tkeep;
wire [7 : 0] 				dispatcher2_tx_tid;
wire [7 : 0] 				dispatcher2_tx_tdest;
wire [31 : 0] 				dispatcher2_tx_tuser;
wire 							dispatcher2_tx_tlast;

wire 							dispatcher2_connector_tx_tready;

// dispatcher3
wire 							dispatcher3_tx_valid;
wire [noc_dw - 1 : 0]  	dispatcher3_tx_tdata;
wire [byte_dw - 1 : 0] 	dispatcher3_tx_tstrb;
wire [byte_dw - 1 : 0] 	dispatcher3_tx_tkeep;
wire [7 : 0] 				dispatcher3_tx_tid;
wire [7 : 0] 				dispatcher3_tx_tdest;
wire [31 : 0] 				dispatcher3_tx_tuser;
wire 							dispatcher3_tx_tlast;

wire 							dispatcher3_connector_tx_tready;

// collector signals
wire 							collector_rx_valid;
wire [noc_dw - 1 : 0]  	collector_rx_tdata;
wire [byte_dw - 1 : 0] 	collector_rx_tstrb;
wire [byte_dw - 1 : 0] 	collector_rx_tkeep;
wire [7 : 0] 				collector_rx_tid;
wire [7 : 0] 				collector_rx_tdest;
wire [31 : 0] 				collector_rx_tuser;
wire 							collector_rx_tlast;

wire 							collector_connector_rx_tready;

// mvm module signals
// layer0_mvm0
wire 							layer0_mvm0_tx_valid;
wire [noc_dw - 1 : 0]  	layer0_mvm0_tx_tdata;
wire [byte_dw - 1 : 0] 	layer0_mvm0_tx_tstrb;
wire [byte_dw - 1 : 0] 	layer0_mvm0_tx_tkeep;
wire [7 : 0] 				layer0_mvm0_tx_tid;
wire [7 : 0] 				layer0_mvm0_tx_tdest;
wire [31 : 0] 				layer0_mvm0_tx_tuser;
wire 							layer0_mvm0_tx_tlast;

wire 							layer0_mvm0_connector_tx_tready;

wire 							layer0_mvm0_rx_valid;
wire [noc_dw - 1 : 0]  	layer0_mvm0_rx_tdata;
wire [byte_dw - 1 : 0] 	layer0_mvm0_rx_tstrb;
wire [byte_dw - 1 : 0] 	layer0_mvm0_rx_tkeep;
wire [7 : 0] 				layer0_mvm0_rx_tid;
wire [7 : 0] 				layer0_mvm0_rx_tdest;
wire [31 : 0] 				layer0_mvm0_rx_tuser;
wire 							layer0_mvm0_rx_tlast;

wire 							layer0_mvm0_connector_rx_tready;
// layer0_mvm1
wire 							layer0_mvm1_tx_valid;
wire [noc_dw - 1 : 0]  	layer0_mvm1_tx_tdata;
wire [byte_dw - 1 : 0] 	layer0_mvm1_tx_tstrb;
wire [byte_dw - 1 : 0] 	layer0_mvm1_tx_tkeep;
wire [7 : 0] 				layer0_mvm1_tx_tid;
wire [7 : 0] 				layer0_mvm1_tx_tdest;
wire [31 : 0] 				layer0_mvm1_tx_tuser;
wire 							layer0_mvm1_tx_tlast;

wire 							layer0_mvm1_connector_tx_tready;

wire 							layer0_mvm1_rx_valid;
wire [noc_dw - 1 : 0]  	layer0_mvm1_rx_tdata;
wire [byte_dw - 1 : 0] 	layer0_mvm1_rx_tstrb;
wire [byte_dw - 1 : 0] 	layer0_mvm1_rx_tkeep;
wire [7 : 0] 				layer0_mvm1_rx_tid;
wire [7 : 0] 				layer0_mvm1_rx_tdest;
wire [31 : 0] 				layer0_mvm1_rx_tuser;
wire 							layer0_mvm1_rx_tlast;

wire 							layer0_mvm1_connector_rx_tready;
// layer0_mvm2
wire 							layer0_mvm2_tx_valid;
wire [noc_dw - 1 : 0]  	layer0_mvm2_tx_tdata;
wire [byte_dw - 1 : 0] 	layer0_mvm2_tx_tstrb;
wire [byte_dw - 1 : 0] 	layer0_mvm2_tx_tkeep;
wire [7 : 0] 				layer0_mvm2_tx_tid;
wire [7 : 0] 				layer0_mvm2_tx_tdest;
wire [31 : 0] 				layer0_mvm2_tx_tuser;
wire 							layer0_mvm2_tx_tlast;

wire 							layer0_mvm2_connector_tx_tready;

wire 							layer0_mvm2_rx_valid;
wire [noc_dw - 1 : 0]  	layer0_mvm2_rx_tdata;
wire [byte_dw - 1 : 0] 	layer0_mvm2_rx_tstrb;
wire [byte_dw - 1 : 0] 	layer0_mvm2_rx_tkeep;
wire [7 : 0] 				layer0_mvm2_rx_tid;
wire [7 : 0] 				layer0_mvm2_rx_tdest;
wire [31 : 0] 				layer0_mvm2_rx_tuser;
wire 							layer0_mvm2_rx_tlast;

wire 							layer0_mvm2_connector_rx_tready;
// layer0_mvm3
wire 							layer0_mvm3_tx_valid;
wire [noc_dw - 1 : 0]  	layer0_mvm3_tx_tdata;
wire [byte_dw - 1 : 0] 	layer0_mvm3_tx_tstrb;
wire [byte_dw - 1 : 0] 	layer0_mvm3_tx_tkeep;
wire [7 : 0] 				layer0_mvm3_tx_tid;
wire [7 : 0] 				layer0_mvm3_tx_tdest;
wire [31 : 0] 				layer0_mvm3_tx_tuser;
wire 							layer0_mvm3_tx_tlast;

wire 							layer0_mvm3_connector_tx_tready;

wire 							layer0_mvm3_rx_valid;
wire [noc_dw - 1 : 0]  	layer0_mvm3_rx_tdata;
wire [byte_dw - 1 : 0] 	layer0_mvm3_rx_tstrb;
wire [byte_dw - 1 : 0] 	layer0_mvm3_rx_tkeep;
wire [7 : 0] 				layer0_mvm3_rx_tid;
wire [7 : 0] 				layer0_mvm3_rx_tdest;
wire [31 : 0] 				layer0_mvm3_rx_tuser;
wire 							layer0_mvm3_rx_tlast;

wire 							layer0_mvm3_connector_rx_tready;
// layer1_mvm0
wire 							layer1_mvm0_tx_valid;
wire [noc_dw - 1 : 0]  	layer1_mvm0_tx_tdata;
wire [byte_dw - 1 : 0] 	layer1_mvm0_tx_tstrb;
wire [byte_dw - 1 : 0] 	layer1_mvm0_tx_tkeep;
wire [7 : 0] 				layer1_mvm0_tx_tid;
wire [7 : 0] 				layer1_mvm0_tx_tdest;
wire [31 : 0] 				layer1_mvm0_tx_tuser;
wire 							layer1_mvm0_tx_tlast;

wire 							layer1_mvm0_connector_tx_tready;

wire 							layer1_mvm0_rx_valid;
wire [noc_dw - 1 : 0]  	layer1_mvm0_rx_tdata;
wire [byte_dw - 1 : 0] 	layer1_mvm0_rx_tstrb;
wire [byte_dw - 1 : 0] 	layer1_mvm0_rx_tkeep;
wire [7 : 0] 				layer1_mvm0_rx_tid;
wire [7 : 0] 				layer1_mvm0_rx_tdest;
wire [31 : 0] 				layer1_mvm0_rx_tuser;
wire 							layer1_mvm0_rx_tlast;

wire 							layer1_mvm0_connector_rx_tready;
// layer1_mvm1
wire 							layer1_mvm1_tx_valid;
wire [noc_dw - 1 : 0]  	layer1_mvm1_tx_tdata;
wire [byte_dw - 1 : 0] 	layer1_mvm1_tx_tstrb;
wire [byte_dw - 1 : 0] 	layer1_mvm1_tx_tkeep;
wire [7 : 0] 				layer1_mvm1_tx_tid;
wire [7 : 0] 				layer1_mvm1_tx_tdest;
wire [31 : 0] 				layer1_mvm1_tx_tuser;
wire 							layer1_mvm1_tx_tlast;

wire 							layer1_mvm1_connector_tx_tready;

wire 							layer1_mvm1_rx_valid;
wire [noc_dw - 1 : 0]  	layer1_mvm1_rx_tdata;
wire [byte_dw - 1 : 0] 	layer1_mvm1_rx_tstrb;
wire [byte_dw - 1 : 0] 	layer1_mvm1_rx_tkeep;
wire [7 : 0] 				layer1_mvm1_rx_tid;
wire [7 : 0] 				layer1_mvm1_rx_tdest;
wire [31 : 0] 				layer1_mvm1_rx_tuser;
wire 							layer1_mvm1_rx_tlast;

wire 							layer1_mvm1_connector_rx_tready;
// layer1_mvm2
wire 							layer1_mvm2_tx_valid;
wire [noc_dw - 1 : 0]  	layer1_mvm2_tx_tdata;
wire [byte_dw - 1 : 0] 	layer1_mvm2_tx_tstrb;
wire [byte_dw - 1 : 0] 	layer1_mvm2_tx_tkeep;
wire [7 : 0] 				layer1_mvm2_tx_tid;
wire [7 : 0] 				layer1_mvm2_tx_tdest;
wire [31 : 0] 				layer1_mvm2_tx_tuser;
wire 							layer1_mvm2_tx_tlast;

wire 							layer1_mvm2_connector_tx_tready;

wire 							layer1_mvm2_rx_valid;
wire [noc_dw - 1 : 0]  	layer1_mvm2_rx_tdata;
wire [byte_dw - 1 : 0] 	layer1_mvm2_rx_tstrb;
wire [byte_dw - 1 : 0] 	layer1_mvm2_rx_tkeep;
wire [7 : 0] 				layer1_mvm2_rx_tid;
wire [7 : 0] 				layer1_mvm2_rx_tdest;
wire [31 : 0] 				layer1_mvm2_rx_tuser;
wire 							layer1_mvm2_rx_tlast;

wire 							layer1_mvm2_connector_rx_tready;


// layer2_mvm0
wire 							layer2_mvm0_tx_valid;
wire [noc_dw - 1 : 0]  	layer2_mvm0_tx_tdata;
wire [byte_dw - 1 : 0] 	layer2_mvm0_tx_tstrb;
wire [byte_dw - 1 : 0] 	layer2_mvm0_tx_tkeep;
wire [7 : 0] 				layer2_mvm0_tx_tid;
wire [7 : 0] 				layer2_mvm0_tx_tdest;
wire [31 : 0] 				layer2_mvm0_tx_tuser;
wire 							layer2_mvm0_tx_tlast;

wire 							layer2_mvm0_connector_tx_tready;

wire 							layer2_mvm0_rx_valid;
wire [noc_dw - 1 : 0]  	layer2_mvm0_rx_tdata;
wire [byte_dw - 1 : 0] 	layer2_mvm0_rx_tstrb;
wire [byte_dw - 1 : 0] 	layer2_mvm0_rx_tkeep;
wire [7 : 0] 				layer2_mvm0_rx_tid;
wire [7 : 0] 				layer2_mvm0_rx_tdest;
wire [31 : 0] 				layer2_mvm0_rx_tuser;
wire 							layer2_mvm0_rx_tlast;

wire 							layer2_mvm0_connector_rx_tready;
// layer2_mvm1
wire 							layer2_mvm1_tx_valid;
wire [noc_dw - 1 : 0]  	layer2_mvm1_tx_tdata;
wire [byte_dw - 1 : 0] 	layer2_mvm1_tx_tstrb;
wire [byte_dw - 1 : 0] 	layer2_mvm1_tx_tkeep;
wire [7 : 0] 				layer2_mvm1_tx_tid;
wire [7 : 0] 				layer2_mvm1_tx_tdest;
wire [31 : 0] 				layer2_mvm1_tx_tuser;
wire 							layer2_mvm1_tx_tlast;

wire 							layer2_mvm1_connector_tx_tready;

wire 							layer2_mvm1_rx_valid;
wire [noc_dw - 1 : 0]  	layer2_mvm1_rx_tdata;
wire [byte_dw - 1 : 0] 	layer2_mvm1_rx_tstrb;
wire [byte_dw - 1 : 0] 	layer2_mvm1_rx_tkeep;
wire [7 : 0] 				layer2_mvm1_rx_tid;
wire [7 : 0] 				layer2_mvm1_rx_tdest;
wire [31 : 0] 				layer2_mvm1_rx_tuser;
wire 							layer2_mvm1_rx_tlast;

wire 							layer2_mvm1_connector_rx_tready;
// layer3_mvm0
wire 							layer3_mvm0_tx_valid;
wire [noc_dw - 1 : 0]  	layer3_mvm0_tx_tdata;
wire [byte_dw - 1 : 0] 	layer3_mvm0_tx_tstrb;
wire [byte_dw - 1 : 0] 	layer3_mvm0_tx_tkeep;
wire [7 : 0] 				layer3_mvm0_tx_tid;
wire [7 : 0] 				layer3_mvm0_tx_tdest;
wire [31 : 0] 				layer3_mvm0_tx_tuser;
wire 							layer3_mvm0_tx_tlast;

wire 							layer3_mvm0_connector_tx_tready;

wire 							layer3_mvm0_rx_valid;
wire [noc_dw - 1 : 0]  	layer3_mvm0_rx_tdata;
wire [byte_dw - 1 : 0] 	layer3_mvm0_rx_tstrb;
wire [byte_dw - 1 : 0] 	layer3_mvm0_rx_tkeep;
wire [7 : 0] 				layer3_mvm0_rx_tid;
wire [7 : 0] 				layer3_mvm0_rx_tdest;
wire [31 : 0] 				layer3_mvm0_rx_tuser;
wire 							layer3_mvm0_rx_tlast;

wire 							layer3_mvm0_connector_rx_tready;
// layer3_mvm1
wire 							layer3_mvm1_tx_valid;
wire [noc_dw - 1 : 0]  	layer3_mvm1_tx_tdata;
wire [byte_dw - 1 : 0] 	layer3_mvm1_tx_tstrb;
wire [byte_dw - 1 : 0] 	layer3_mvm1_tx_tkeep;
wire [7 : 0] 				layer3_mvm1_tx_tid;
wire [7 : 0] 				layer3_mvm1_tx_tdest;
wire [31 : 0] 				layer3_mvm1_tx_tuser;
wire 							layer3_mvm1_tx_tlast;

wire 							layer3_mvm1_connector_tx_tready;

wire 							layer3_mvm1_rx_valid;
wire [noc_dw - 1 : 0]  	layer3_mvm1_rx_tdata;
wire [byte_dw - 1 : 0] 	layer3_mvm1_rx_tstrb;
wire [byte_dw - 1 : 0] 	layer3_mvm1_rx_tkeep;
wire [7 : 0] 				layer3_mvm1_rx_tid;
wire [7 : 0] 				layer3_mvm1_rx_tdest;
wire [31 : 0] 				layer3_mvm1_rx_tuser;
wire 							layer3_mvm1_rx_tlast;

wire 							layer3_mvm1_connector_rx_tready;


/*******************module instantiation********************/

// dispatcher modules
dispatcher dispatcher0(
    .clk(clk),
    .rst(reset),
    .tx_tvalid(dispatcher0_tx_valid),
    .tx_tdata(dispatcher0_tx_tdata),
	.tx_tstrb(dispatcher0_tx_tstrb),
	.tx_tkeep(dispatcher0_tx_tkeep),
	.tx_tid(dispatcher0_tx_tid),
	.tx_tdest(dispatcher0_tx_tdest),
	.tx_tuser(dispatcher0_tx_tuser),
	.tx_tlast(dispatcher0_tx_tlast),
	.tx_tready(dispatcher0_connector_tx_tready),
	.ififo_wdata(dispatcher0_ififo_wdata),
	.ififo_wen(dispatcher0_ififo_wen),
	.ififo_rdy(dispatcher0_ififo_rdy)
);
dispatcher dispatcher1(
    .clk(clk),
    .rst(reset),
    .tx_tvalid(dispatcher1_tx_valid),
    .tx_tdata(dispatcher1_tx_tdata),
	.tx_tstrb(dispatcher1_tx_tstrb),
	.tx_tkeep(dispatcher1_tx_tkeep),
	.tx_tid(dispatcher1_tx_tid),
	.tx_tdest(dispatcher1_tx_tdest),
	.tx_tuser(dispatcher1_tx_tuser),
	.tx_tlast(dispatcher1_tx_tlast),
	.tx_tready(dispatcher1_connector_tx_tready),
	.ififo_wdata(dispatcher1_ififo_wdata),
	.ififo_wen(dispatcher1_ififo_wen),
	.ififo_rdy(dispatcher1_ififo_rdy)
);
dispatcher dispatcher2(
    .clk(clk),
    .rst(reset),
    .tx_tvalid(dispatcher2_tx_valid),
    .tx_tdata(dispatcher2_tx_tdata),
	.tx_tstrb(dispatcher2_tx_tstrb),
	.tx_tkeep(dispatcher2_tx_tkeep),
	.tx_tid(dispatcher2_tx_tid),
	.tx_tdest(dispatcher2_tx_tdest),
	.tx_tuser(dispatcher2_tx_tuser),
	.tx_tlast(dispatche2_tx_tlast),
	.tx_tready(dispatcher2_connector_tx_tready),
	.ififo_wdata(dispatcher2_ififo_wdata),
	.ififo_wen(dispatcher2_ififo_wen),
	.ififo_rdy(dispatcher2_ififo_rdy)
);
dispatcher dispatcher3(
    .clk(clk),
    .rst(reset),
    .tx_tvalid(dispatcher3_tx_valid),
    .tx_tdata(dispatcher3_tx_tdata),
	.tx_tstrb(dispatcher3_tx_tstrb),
	.tx_tkeep(dispatcher3_tx_tkeep),
	.tx_tid(dispatcher3_tx_tid),
	.tx_tdest(dispatcher3_tx_tdest),
	.tx_tuser(dispatcher3_tx_tuser),
	.tx_tlast(dispatcher3_tx_tlast),
	.tx_tready(dispatcher3_connector_tx_tready),
	.ififo_wdata(dispatcher3_ififo_wdata),
	.ififo_wen(dispatcher3_ififo_wen),
	.ififo_rdy(dispatcher3_ififo_rdy)
);

wire [noc_dw-1:0] dispatcher0_data_dummy;
// dispatcher router blocks
noc_router_adapter_block_inst noc_router_input_dispatcher0(
	.clk(clk),
    .reset(reset),
    .master_tready(1'd0),
    .master_tdata(dispatcher0_data_dummy),
	.master_tvalid(),
    .master_tstrb(),
    .master_tkeep(),
    .master_tid(),
    .master_tdest(),
    .master_tuser(),
    .master_tlast(),
    .slave_tvalid(dispatcher0_tx_valid),
    .slave_tready(dispatcher0_connector_tx_tready), 
    .slave_tdata(dispatcher0_tx_tdata),
    .slave_tstrb(dispatcher0_tx_tstrb),
    .slave_tkeep(dispatcher0_tx_tkeep),
    .slave_tid(dispatcher0_tx_tid),
    .slave_tdest(dispatcher0_tx_tdest),
    .slave_tuser(dispatcher0_tx_tuser),
    .slave_tlast(dispatcher0_tx_tlast)

);
wire [noc_dw-1:0] dispatcher1_data_dummy;
noc_router_adapter_block_inst noc_router_input_dispatcher1(
	.clk(clk),
    .reset(reset),
    .master_tready(1'd0),
    .master_tdata(dispatcher1_data_dummy),
	.master_tvalid(),
    .master_tstrb(),
    .master_tkeep(),
    .master_tid(),
    .master_tdest(),
    .master_tuser(),
    .master_tlast(),
    .slave_tvalid(dispatcher1_tx_valid),
    .slave_tready(dispatcher1_connector_tx_tready), 
    .slave_tdata(dispatcher1_tx_tdata),
    .slave_tstrb(dispatcher1_tx_tstrb),
    .slave_tkeep(dispatcher1_tx_tkeep),
    .slave_tid(dispatcher1_tx_tid),
    .slave_tdest(dispatcher1_tx_tdest),
    .slave_tuser(dispatcher1_tx_tuser),
    .slave_tlast(dispatcher1_tx_tlast)

);
wire [noc_dw-1:0] dispatcher2_data_dummy;
noc_router_adapter_block_inst noc_router_input_dispatcher2(
	.clk(clk),
    .reset(reset),
    .master_tready(1'd0),
    .master_tdata(dispatcher2_data_dummy),
	.master_tvalid(),
    .master_tstrb(),
    .master_tkeep(),
    .master_tid(),
    .master_tdest(),
    .master_tuser(),
    .master_tlast(),
    .slave_tvalid(dispatcher2_tx_valid),
    .slave_tready(dispatcher2_connector_tx_tready), 
    .slave_tdata(dispatcher2_tx_tdata),
    .slave_tstrb(dispatcher2_tx_tstrb),
    .slave_tkeep(dispatcher2_tx_tkeep),
    .slave_tid(dispatcher2_tx_tid),
    .slave_tdest(dispatcher2_tx_tdest),
    .slave_tuser(dispatcher2_tx_tuser),
    .slave_tlast(dispatcher2_tx_tlast)

);

wire [noc_dw-1:0] dispatcher3_data_dummy;
noc_router_adapter_block_inst noc_router_input_dispatcher3(
	.clk(clk),
    .reset(reset),
    .master_tready(1'd0),
    .master_tdata(dispatcher3_data_dummy),
	.master_tvalid(),
    .master_tstrb(),
    .master_tkeep(),
    .master_tid(),
    .master_tdest(),
    .master_tuser(),
    .master_tlast(),
    .slave_tvalid(dispatcher3_tx_valid),
    .slave_tready(dispatcher3_connector_tx_tready), 
    .slave_tdata(dispatcher3_tx_tdata),
    .slave_tstrb(dispatcher3_tx_tstrb),
    .slave_tkeep(dispatcher3_tx_tkeep),
    .slave_tid(dispatcher3_tx_tid),
    .slave_tdest(dispatcher3_tx_tdest),
    .slave_tuser(dispatcher3_tx_tuser),
    .slave_tlast(dispatcher3_tx_tlast)

);

assign master_tdata_top_out = dispatcher0_data_dummy | dispatcher1_data_dummy | dispatcher2_data_dummy | dispatcher3_data_dummy;


// mvm module declarations (layer 0)
mvm_top layer0_mvm0(
	.clk(clk),
    .rst(reset),
    .rx_tvalid(layer0_mvm0_rx_valid | rx_top_tvalid),
    .rx_tdata(layer0_mvm0_rx_tdata | rx_top_tdata),
	.rx_tstrb(layer0_mvm0_rx_tstrb | rx_top_tstrb),
    .rx_tkeep(layer0_mvm0_rx_tkeep | rx_top_tkeep),
    .rx_tid(layer0_mvm0_rx_tid | rx_top_tid),
    .rx_tdest(layer0_mvm0_rx_tdest | rx_top_tdest),
    .rx_tuser(layer0_mvm0_rx_tuser | rx_top_tuser),
    .rx_tlast(layer0_mvm0_rx_tlast | rx_top_tkeep),
    .rx_tready(layer0_mvm0_connector_rx_tready),
    .tx_tvalid(layer0_mvm0_tx_valid),
    .tx_tdata(layer0_mvm0_tx_tdata), 
    .tx_tstrb(layer0_mvm0_tx_tstrb),
    .tx_tkeep(layer0_mvm0_tx_tkeep),
    .tx_tid(layer0_mvm0_tx_tid),
    .tx_tdest(layer0_mvm0_tx_tdest),
    .tx_tuser(layer0_mvm0_tx_tuser),
    .tx_tlast(layer0_mvm0_tx_tlast),
    .tx_tready(layer0_mvm0_connector_tx_tready)

);
mvm_top layer0_mvm1(
	.clk(clk),
    .rst(reset),
    .rx_tvalid(layer0_mvm1_rx_valid | rx_top_tvalid),
    .rx_tdata(layer0_mvm1_rx_tdata | rx_top_tdata),
	.rx_tstrb(layer0_mvm1_rx_tstrb | rx_top_tstrb),
    .rx_tkeep(layer0_mvm1_rx_tkeep | rx_top_tkeep),
    .rx_tid(layer0_mvm1_rx_tid | rx_top_tid),
    .rx_tdest(layer0_mvm1_rx_tdest | rx_top_tdest),
    .rx_tuser(layer0_mvm1_rx_tuser | rx_top_tuser),
    .rx_tlast(layer0_mvm1_rx_tlast | rx_top_tkeep),
    .rx_tready(layer0_mvm1_connector_rx_tready),
    .tx_tvalid(layer0_mvm1_tx_valid),
    .tx_tdata(layer0_mvm1_tx_tdata), 
    .tx_tstrb(layer0_mvm1_tx_tstrb),
    .tx_tkeep(layer0_mvm1_tx_tkeep),
    .tx_tid(layer0_mvm1_tx_tid),
    .tx_tdest(layer0_mvm1_tx_tdest),
    .tx_tuser(layer0_mvm1_tx_tuser),
    .tx_tlast(layer0_mvm1_tx_tlast),
    .tx_tready(layer0_mvm1_connector_tx_tready)

);
mvm_top layer0_mvm2(
	.clk(clk),
    .rst(reset),
    .rx_tvalid(layer0_mvm2_rx_valid | rx_top_tvalid),
    .rx_tdata(layer0_mvm2_rx_tdata | rx_top_tdata),
	.rx_tstrb(layer0_mvm2_rx_tstrb | rx_top_tstrb),
    .rx_tkeep(layer0_mvm2_rx_tkeep | rx_top_tkeep),
    .rx_tid(layer0_mvm2_rx_tid | rx_top_tid),
    .rx_tdest(layer0_mvm2_rx_tdest | rx_top_tdest),
    .rx_tuser(layer0_mvm2_rx_tuser | rx_top_tuser),
    .rx_tlast(layer0_mvm2_rx_tlast | rx_top_tkeep),
    .rx_tready(layer0_mvm2_connector_rx_tready),
    .tx_tvalid(layer0_mvm2_tx_valid),
    .tx_tdata(layer0_mvm2_tx_tdata), 
    .tx_tstrb(layer0_mvm2_tx_tstrb),
    .tx_tkeep(layer0_mvm2_tx_tkeep),
    .tx_tid(layer0_mvm2_tx_tid),
    .tx_tdest(layer0_mvm2_tx_tdest),
    .tx_tuser(layer0_mvm2_tx_tuser),
    .tx_tlast(layer0_mvm2_tx_tlast),
    .tx_tready(layer0_mvm2_connector_tx_tready)

);
mvm_top layer0_mvm3(
	.clk(clk),
    .rst(reset),
    .rx_tvalid(layer0_mvm3_rx_valid | rx_top_tvalid),
    .rx_tdata(layer0_mvm3_rx_tdata | rx_top_tdata),
	.rx_tstrb(layer0_mvm3_rx_tstrb | rx_top_tstrb),
    .rx_tkeep(layer0_mvm3_rx_tkeep | rx_top_tkeep),
    .rx_tid(layer0_mvm3_rx_tid | rx_top_tid),
    .rx_tdest(layer0_mvm3_rx_tdest | rx_top_tdest),
    .rx_tuser(layer0_mvm3_rx_tuser | rx_top_tuser),
    .rx_tlast(layer0_mvm3_rx_tlast | rx_top_tkeep),
    .rx_tready(layer0_mvm3_connector_rx_tready),
    .tx_tvalid(layer0_mvm3_tx_valid),
    .tx_tdata(layer0_mvm3_tx_tdata), 
    .tx_tstrb(layer0_mvm3_tx_tstrb),
    .tx_tkeep(layer0_mvm3_tx_tkeep),
    .tx_tid(layer0_mvm3_tx_tid),
    .tx_tdest(layer0_mvm3_tx_tdest),
    .tx_tuser(layer0_mvm3_tx_tuser),
    .tx_tlast(layer0_mvm3_tx_tlast),
    .tx_tready(layer0_mvm3_connector_tx_tready)

);

// mvm module declarations (layer 1)
mvm_top layer1_mvm0(
	.clk(clk),
    .rst(reset),
    .rx_tvalid(layer1_mvm0_rx_valid | rx_top_tvalid),
    .rx_tdata(layer1_mvm0_rx_tdata | rx_top_tdata),
	.rx_tstrb(layer1_mvm0_rx_tstrb | rx_top_tstrb),
    .rx_tkeep(layer1_mvm0_rx_tkeep | rx_top_tkeep),
    .rx_tid(layer1_mvm0_rx_tid | rx_top_tid),
    .rx_tdest(layer1_mvm0_rx_tdest | rx_top_tdest),
    .rx_tuser(layer1_mvm0_rx_tuser | rx_top_tuser),
    .rx_tlast(layer1_mvm0_rx_tlast | rx_top_tkeep),
    .rx_tready(layer1_mvm0_connector_rx_tready),
    .tx_tvalid(layer1_mvm0_tx_valid),
    .tx_tdata(layer1_mvm0_tx_tdata), 
    .tx_tstrb(layer1_mvm0_tx_tstrb),
    .tx_tkeep(layer1_mvm0_tx_tkeep),
    .tx_tid(layer1_mvm0_tx_tid),
    .tx_tdest(layer1_mvm0_tx_tdest),
    .tx_tuser(layer1_mvm0_tx_tuser),
    .tx_tlast(layer1_mvm0_tx_tlast),
    .tx_tready(layer1_mvm0_connector_tx_tready)

);
mvm_top layer1_mvm1(
	.clk(clk),
    .rst(reset),
    .rx_tvalid(layer1_mvm1_rx_valid | rx_top_tvalid),
    .rx_tdata(layer1_mvm1_rx_tdata | rx_top_tdata),
	.rx_tstrb(layer1_mvm1_rx_tstrb | rx_top_tstrb),
    .rx_tkeep(layer1_mvm1_rx_tkeep | rx_top_tkeep),
    .rx_tid(layer1_mvm1_rx_tid | rx_top_tid),
    .rx_tdest(layer1_mvm1_rx_tdest | rx_top_tdest),
    .rx_tuser(layer1_mvm1_rx_tuser | rx_top_tuser),
    .rx_tlast(layer1_mvm1_rx_tlast | rx_top_tkeep),
    .rx_tready(layer1_mvm1_connector_rx_tready),
    .tx_tvalid(layer1_mvm1_tx_valid),
    .tx_tdata(layer1_mvm1_tx_tdata), 
    .tx_tstrb(layer1_mvm1_tx_tstrb),
    .tx_tkeep(layer1_mvm1_tx_tkeep),
    .tx_tid(layer1_mvm1_tx_tid),
    .tx_tdest(layer1_mvm1_tx_tdest),
    .tx_tuser(layer1_mvm1_tx_tuser),
    .tx_tlast(layer1_mvm1_tx_tlast),
    .tx_tready(layer1_mvm1_connector_tx_tready)

);
mvm_top layer1_mvm2(
	.clk(clk),
    .rst(reset),
    .rx_tvalid(layer1_mvm2_rx_valid | rx_top_tvalid),
    .rx_tdata(layer1_mvm2_rx_tdata | rx_top_tdata),
	.rx_tstrb(layer1_mvm2_rx_tstrb | rx_top_tstrb),
    .rx_tkeep(layer1_mvm2_rx_tkeep | rx_top_tkeep),
    .rx_tid(layer1_mvm2_rx_tid | rx_top_tid),
    .rx_tdest(layer1_mvm2_rx_tdest | rx_top_tdest),
    .rx_tuser(layer1_mvm2_rx_tuser | rx_top_tuser),
    .rx_tlast(layer1_mvm2_rx_tlast | rx_top_tkeep),
    .rx_tready(layer1_mvm2_connector_rx_tready),
    .tx_tvalid(layer1_mvm2_tx_valid),
    .tx_tdata(layer1_mvm2_tx_tdata), 
    .tx_tstrb(layer1_mvm2_tx_tstrb),
    .tx_tkeep(layer1_mvm2_tx_tkeep),
    .tx_tid(layer1_mvm2_tx_tid),
    .tx_tdest(layer1_mvm2_tx_tdest),
    .tx_tuser(layer1_mvm2_tx_tuser),
    .tx_tlast(layer1_mvm2_tx_tlast),
    .tx_tready(layer1_mvm2_connector_tx_tready)

);


// mvm module declarations (layer 2)
mvm_top layer2_mvm0(
	.clk(clk),
    .rst(reset),
    .rx_tvalid(layer2_mvm0_rx_valid | rx_top_tvalid),
    .rx_tdata(layer2_mvm0_rx_tdata | rx_top_tdata),
	.rx_tstrb(layer2_mvm0_rx_tstrb | rx_top_tstrb),
    .rx_tkeep(layer2_mvm0_rx_tkeep | rx_top_tkeep),
    .rx_tid(layer2_mvm0_rx_tid | rx_top_tid),
    .rx_tdest(layer2_mvm0_rx_tdest | rx_top_tdest),
    .rx_tuser(layer2_mvm0_rx_tuser | rx_top_tuser),
    .rx_tlast(layer2_mvm0_rx_tlast | rx_top_tkeep),
    .rx_tready(layer2_mvm0_connector_rx_tready),
    .tx_tvalid(layer2_mvm0_tx_valid),
    .tx_tdata(layer2_mvm0_tx_tdata), 
    .tx_tstrb(layer2_mvm0_tx_tstrb),
    .tx_tkeep(layer2_mvm0_tx_tkeep),
    .tx_tid(layer2_mvm0_tx_tid),
    .tx_tdest(layer2_mvm0_tx_tdest),
    .tx_tuser(layer2_mvm0_tx_tuser),
    .tx_tlast(layer2_mvm0_tx_tlast),
    .tx_tready(layer2_mvm0_connector_tx_tready)

);
mvm_top layer2_mvm1(
	.clk(clk),
    .rst(reset),
    .rx_tvalid(layer2_mvm1_rx_valid | rx_top_tvalid),
    .rx_tdata(layer2_mvm1_rx_tdata | rx_top_tdata),
	.rx_tstrb(layer2_mvm1_rx_tstrb | rx_top_tstrb),
    .rx_tkeep(layer2_mvm1_rx_tkeep | rx_top_tkeep),
    .rx_tid(layer2_mvm1_rx_tid | rx_top_tid),
    .rx_tdest(layer2_mvm1_rx_tdest | rx_top_tdest),
    .rx_tuser(layer2_mvm1_rx_tuser | rx_top_tuser),
    .rx_tlast(layer2_mvm1_rx_tlast | rx_top_tkeep),
    .rx_tready(layer2_mvm1_connector_rx_tready),
    .tx_tvalid(layer2_mvm1_tx_valid),
    .tx_tdata(layer2_mvm1_tx_tdata), 
    .tx_tstrb(layer2_mvm1_tx_tstrb),
    .tx_tkeep(layer2_mvm1_tx_tkeep),
    .tx_tid(layer2_mvm1_tx_tid),
    .tx_tdest(layer2_mvm1_tx_tdest),
    .tx_tuser(layer2_mvm1_tx_tuser),
    .tx_tlast(layer2_mvm1_tx_tlast),
    .tx_tready(layer2_mvm1_connector_tx_tready)

);

// mvm module declarations (layer 3)
mvm_top layer3_mvm0(
	.clk(clk),
    .rst(reset),
    .rx_tvalid(layer3_mvm0_rx_valid | rx_top_tvalid),
    .rx_tdata(layer3_mvm0_rx_tdata | rx_top_tdata),
	.rx_tstrb(layer3_mvm0_rx_tstrb | rx_top_tstrb),
    .rx_tkeep(layer3_mvm0_rx_tkeep | rx_top_tkeep),
    .rx_tid(layer3_mvm0_rx_tid | rx_top_tid),
    .rx_tdest(layer3_mvm0_rx_tdest | rx_top_tdest),
    .rx_tuser(layer3_mvm0_rx_tuser | rx_top_tuser),
    .rx_tlast(layer3_mvm0_rx_tlast | rx_top_tkeep),
    .rx_tready(layer3_mvm0_connector_rx_tready),
    .tx_tvalid(layer3_mvm0_tx_valid),
    .tx_tdata(layer3_mvm0_tx_tdata), 
    .tx_tstrb(layer3_mvm0_tx_tstrb),
    .tx_tkeep(layer3_mvm0_tx_tkeep),
    .tx_tid(layer3_mvm0_tx_tid),
    .tx_tdest(layer3_mvm0_tx_tdest),
    .tx_tuser(layer3_mvm0_tx_tuser),
    .tx_tlast(layer3_mvm0_tx_tlast),
    .tx_tready(layer3_mvm0_connector_tx_tready)

);
mvm_top layer3_mvm1(
	.clk(clk),
    .rst(reset),
    .rx_tvalid(layer3_mvm1_rx_valid | rx_top_tvalid),
    .rx_tdata(layer3_mvm1_rx_tdata | rx_top_tdata),
	.rx_tstrb(layer3_mvm1_rx_tstrb | rx_top_tstrb),
    .rx_tkeep(layer3_mvm1_rx_tkeep | rx_top_tkeep),
    .rx_tid(layer3_mvm1_rx_tid | rx_top_tid),
    .rx_tdest(layer3_mvm1_rx_tdest | rx_top_tdest),
    .rx_tuser(layer3_mvm1_rx_tuser | rx_top_tuser),
    .rx_tlast(layer3_mvm1_rx_tlast | rx_top_tkeep),
    .rx_tready(layer3_mvm1_connector_rx_tready),
    .tx_tvalid(layer3_mvm1_tx_valid),
    .tx_tdata(layer3_mvm1_tx_tdata), 
    .tx_tstrb(layer3_mvm1_tx_tstrb),
    .tx_tkeep(layer3_mvm1_tx_tkeep),
    .tx_tid(layer3_mvm1_tx_tid),
    .tx_tdest(layer3_mvm1_tx_tdest),
    .tx_tuser(layer3_mvm1_tx_tuser),
    .tx_tlast(layer3_mvm1_tx_tlast),
    .tx_tready(layer3_mvm1_connector_tx_tready)

);


/*mvm matrix_vector_unit(
	.clk(clk),
    .rst(reset),
    .rx_tvalid(dp_tx_valid),
    .rx_tdata(dp_tx_tdata),
	 .rx_tstrb(dp_tx_tstrb),
    .rx_tkeep(dp_tx_tkeep),
    .rx_tid(dp_tx_tid),
    .rx_tdest(dp_tx_tdest),
    .rx_tuser(dp_tx_tuser),
    .rx_tlast(mvm_rx_tlast),
    .rx_tready(dp_connector_tx_tready),
    .tx_tvalid(ct_rx_valid),
    .tx_tdata(ct_rx_tdata), 
    .tx_tstrb(ct_rx_tstrb),
    .tx_tkeep(ct_rx_tkeep),
    .tx_tid(ct_rx_tid),
    .tx_tdest(ct_rx_tdest),
    .tx_tuser(ct_rx_tuser),
    .tx_tlast(ct_rx_tlast),
    .tx_tready(ct_connector_rx_tready)

);*/


// layer 0 router blocks
noc_router_adapter_block_inst noc_router_layer0_mvm0(
	.clk(clk),
    .reset(reset),
    .master_tready(layer0_mvm0_connector_rx_tready),
    .master_tdata(layer0_mvm0_rx_tdata),
	.master_tvalid(layer0_mvm0_rx_valid),
    .master_tstrb(layer0_mvm0_rx_tstrb),
    .master_tkeep(layer0_mvm0_rx_tkeep),
    .master_tid(layer0_mvm0_rx_tid),
    .master_tdest(layer0_mvm0_rx_tdest),
    .master_tuser(layer0_mvm0_rx_tuser),
    .master_tlast(layer0_mvm0_rx_tlast),
    .slave_tvalid(layer0_mvm0_tx_valid),
    .slave_tready(layer0_mvm0_connector_tx_tready), 
    .slave_tdata(layer0_mvm0_tx_tdata),
    .slave_tstrb(layer0_mvm0_tx_tstrb),
    .slave_tkeep(layer0_mvm0_tx_tkeep),
    .slave_tid(layer0_mvm0_tx_tid),
    .slave_tdest(layer0_mvm0_tx_tdest),
    .slave_tuser(layer0_mvm0_tx_tuser),
    .slave_tlast(layer0_mvm0_tx_tlast)
);
noc_router_adapter_block_inst noc_router_layer0_mvm1(
	.clk(clk),
    .reset(reset),
    .master_tready(layer0_mvm1_connector_rx_tready),
    .master_tdata(layer0_mvm1_rx_tdata),
	.master_tvalid(layer0_mvm1_rx_valid),
    .master_tstrb(layer0_mvm1_rx_tstrb),
    .master_tkeep(layer0_mvm1_rx_tkeep),
    .master_tid(layer0_mvm1_rx_tid),
    .master_tdest(layer0_mvm1_rx_tdest),
    .master_tuser(layer0_mvm1_rx_tuser),
    .master_tlast(layer0_mvm1_rx_tlast),
    .slave_tvalid(layer0_mvm1_tx_valid),
    .slave_tready(layer0_mvm1_connector_tx_tready), 
    .slave_tdata(layer0_mvm1_tx_tdata),
    .slave_tstrb(layer0_mvm1_tx_tstrb),
    .slave_tkeep(layer0_mvm1_tx_tkeep),
    .slave_tid(layer0_mvm1_tx_tid),
    .slave_tdest(layer0_mvm1_tx_tdest),
    .slave_tuser(layer0_mvm1_tx_tuser),
    .slave_tlast(layer0_mvm1_tx_tlast)
);
noc_router_adapter_block_inst noc_router_layer0_mvm2(
	.clk(clk),
    .reset(reset),
    .master_tready(layer0_mvm2_connector_rx_tready),
    .master_tdata(layer0_mvm2_rx_tdata),
	.master_tvalid(layer0_mvm2_rx_valid),
    .master_tstrb(layer0_mvm2_rx_tstrb),
    .master_tkeep(layer0_mvm2_rx_tkeep),
    .master_tid(layer0_mvm2_rx_tid),
    .master_tdest(layer0_mvm2_rx_tdest),
    .master_tuser(layer0_mvm2_rx_tuser),
    .master_tlast(layer0_mvm2_rx_tlast),
    .slave_tvalid(layer0_mvm2_tx_valid),
    .slave_tready(layer0_mvm2_connector_tx_tready), 
    .slave_tdata(layer0_mvm2_tx_tdata),
    .slave_tstrb(layer0_mvm2_tx_tstrb),
    .slave_tkeep(layer0_mvm2_tx_tkeep),
    .slave_tid(layer0_mvm2_tx_tid),
    .slave_tdest(layer0_mvm2_tx_tdest),
    .slave_tuser(layer0_mvm2_tx_tuser),
    .slave_tlast(layer0_mvm2_tx_tlast)
);
noc_router_adapter_block_inst noc_router_layer0_mvm3(
	.clk(clk),
    .reset(reset),
    .master_tready(layer0_mvm3_connector_rx_tready),
    .master_tdata(layer0_mvm3_rx_tdata),
	.master_tvalid(layer0_mvm3_rx_valid),
    .master_tstrb(layer0_mvm3_rx_tstrb),
    .master_tkeep(layer0_mvm3_rx_tkeep),
    .master_tid(layer0_mvm3_rx_tid),
    .master_tdest(layer0_mvm3_rx_tdest),
    .master_tuser(layer0_mvm3_rx_tuser),
    .master_tlast(layer0_mvm3_rx_tlast),
    .slave_tvalid(layer0_mvm3_tx_valid),
    .slave_tready(layer0_mvm3_connector_tx_tready), 
    .slave_tdata(layer0_mvm3_tx_tdata),
    .slave_tstrb(layer0_mvm3_tx_tstrb),
    .slave_tkeep(layer0_mvm3_tx_tkeep),
    .slave_tid(layer0_mvm3_tx_tid),
    .slave_tdest(layer0_mvm3_tx_tdest),
    .slave_tuser(layer0_mvm3_tx_tuser),
    .slave_tlast(layer0_mvm3_tx_tlast)
);

// layer 1 router blocks
noc_router_adapter_block_inst noc_router_layer1_mvm0(
	.clk(clk),
    .reset(reset),
    .master_tready(layer1_mvm0_connector_rx_tready),
    .master_tdata(layer1_mvm0_rx_tdata),
	.master_tvalid(layer1_mvm0_rx_valid),
    .master_tstrb(layer1_mvm0_rx_tstrb),
    .master_tkeep(layer1_mvm0_rx_tkeep),
    .master_tid(layer1_mvm0_rx_tid),
    .master_tdest(layer1_mvm0_rx_tdest),
    .master_tuser(layer1_mvm0_rx_tuser),
    .master_tlast(layer1_mvm0_rx_tlast),
    .slave_tvalid(layer1_mvm0_tx_valid),
    .slave_tready(layer1_mvm0_connector_tx_tready), 
    .slave_tdata(layer1_mvm0_tx_tdata),
    .slave_tstrb(layer1_mvm0_tx_tstrb),
    .slave_tkeep(layer1_mvm0_tx_tkeep),
    .slave_tid(layer1_mvm0_tx_tid),
    .slave_tdest(layer1_mvm0_tx_tdest),
    .slave_tuser(layer1_mvm0_tx_tuser),
    .slave_tlast(layer1_mvm0_tx_tlast)
);
noc_router_adapter_block_inst noc_router_layer1_mvm1(
	.clk(clk),
    .reset(reset),
    .master_tready(layer1_mvm1_connector_rx_tready),
    .master_tdata(layer1_mvm1_rx_tdata),
	.master_tvalid(layer1_mvm1_rx_valid),
    .master_tstrb(layer1_mvm1_rx_tstrb),
    .master_tkeep(layer1_mvm1_rx_tkeep),
    .master_tid(layer1_mvm1_rx_tid),
    .master_tdest(layer1_mvm1_rx_tdest),
    .master_tuser(layer1_mvm1_rx_tuser),
    .master_tlast(layer1_mvm1_rx_tlast),
    .slave_tvalid(layer1_mvm1_tx_valid),
    .slave_tready(layer1_mvm1_connector_tx_tready), 
    .slave_tdata(layer1_mvm1_tx_tdata),
    .slave_tstrb(layer1_mvm1_tx_tstrb),
    .slave_tkeep(layer1_mvm1_tx_tkeep),
    .slave_tid(layer1_mvm1_tx_tid),
    .slave_tdest(layer1_mvm1_tx_tdest),
    .slave_tuser(layer1_mvm1_tx_tuser),
    .slave_tlast(layer1_mvm1_tx_tlast)
);
noc_router_adapter_block_inst noc_router_layer1_mvm2(
	.clk(clk),
    .reset(reset),
    .master_tready(layer1_mvm2_connector_rx_tready),
    .master_tdata(layer1_mvm2_rx_tdata),
	.master_tvalid(layer1_mvm2_rx_valid),
    .master_tstrb(layer1_mvm2_rx_tstrb),
    .master_tkeep(layer1_mvm2_rx_tkeep),
    .master_tid(layer1_mvm2_rx_tid),
    .master_tdest(layer1_mvm2_rx_tdest),
    .master_tuser(layer1_mvm2_rx_tuser),
    .master_tlast(layer1_mvm2_rx_tlast),
    .slave_tvalid(layer1_mvm2_tx_valid),
    .slave_tready(layer1_mvm2_connector_tx_tready), 
    .slave_tdata(layer1_mvm2_tx_tdata),
    .slave_tstrb(layer1_mvm2_tx_tstrb),
    .slave_tkeep(layer1_mvm2_tx_tkeep),
    .slave_tid(layer1_mvm2_tx_tid),
    .slave_tdest(layer1_mvm2_tx_tdest),
    .slave_tuser(layer1_mvm2_tx_tuser),
    .slave_tlast(layer1_mvm2_tx_tlast)
);


// layer 2 router blocks
noc_router_adapter_block_inst noc_router_layer2_mvm0(
	.clk(clk),
    .reset(reset),
    .master_tready(layer2_mvm0_connector_rx_tready),
    .master_tdata(layer2_mvm0_rx_tdata),
	.master_tvalid(layer2_mvm0_rx_valid),
    .master_tstrb(layer2_mvm0_rx_tstrb),
    .master_tkeep(layer2_mvm0_rx_tkeep),
    .master_tid(layer2_mvm0_rx_tid),
    .master_tdest(layer2_mvm0_rx_tdest),
    .master_tuser(layer2_mvm0_rx_tuser),
    .master_tlast(layer2_mvm0_rx_tlast),
    .slave_tvalid(layer2_mvm0_tx_valid),
    .slave_tready(layer2_mvm0_connector_tx_tready), 
    .slave_tdata(layer2_mvm0_tx_tdata),
    .slave_tstrb(layer2_mvm0_tx_tstrb),
    .slave_tkeep(layer2_mvm0_tx_tkeep),
    .slave_tid(layer2_mvm0_tx_tid),
    .slave_tdest(layer2_mvm0_tx_tdest),
    .slave_tuser(layer2_mvm0_tx_tuser),
    .slave_tlast(layer2_mvm0_tx_tlast)
);
noc_router_adapter_block_inst noc_router_layer2_mvm1(
	.clk(clk),
    .reset(reset),
    .master_tready(layer2_mvm1_connector_rx_tready),
    .master_tdata(layer2_mvm1_rx_tdata),
	.master_tvalid(layer2_mvm1_rx_valid),
    .master_tstrb(layer2_mvm1_rx_tstrb),
    .master_tkeep(layer2_mvm1_rx_tkeep),
    .master_tid(layer2_mvm1_rx_tid),
    .master_tdest(layer2_mvm1_rx_tdest),
    .master_tuser(layer2_mvm1_rx_tuser),
    .master_tlast(layer2_mvm1_rx_tlast),
    .slave_tvalid(layer2_mvm1_tx_valid),
    .slave_tready(layer2_mvm1_connector_tx_tready), 
    .slave_tdata(layer2_mvm1_tx_tdata),
    .slave_tstrb(layer2_mvm1_tx_tstrb),
    .slave_tkeep(layer2_mvm1_tx_tkeep),
    .slave_tid(layer2_mvm1_tx_tid),
    .slave_tdest(layer2_mvm1_tx_tdest),
    .slave_tuser(layer2_mvm1_tx_tuser),
    .slave_tlast(layer2_mvm1_tx_tlast)
);

// layer 3 router blocks
noc_router_adapter_block_inst noc_router_layer3_mvm0(
	.clk(clk),
    .reset(reset),
    .master_tready(layer3_mvm0_connector_rx_tready),
    .master_tdata(layer3_mvm0_rx_tdata),
	.master_tvalid(layer3_mvm0_rx_valid),
    .master_tstrb(layer3_mvm0_rx_tstrb),
    .master_tkeep(layer3_mvm0_rx_tkeep),
    .master_tid(layer3_mvm0_rx_tid),
    .master_tdest(layer3_mvm0_rx_tdest),
    .master_tuser(layer3_mvm0_rx_tuser),
    .master_tlast(layer3_mvm0_rx_tlast),
    .slave_tvalid(layer3_mvm0_tx_valid),
    .slave_tready(layer3_mvm0_connector_tx_tready), 
    .slave_tdata(layer3_mvm0_tx_tdata),
    .slave_tstrb(layer3_mvm0_tx_tstrb),
    .slave_tkeep(layer3_mvm0_tx_tkeep),
    .slave_tid(layer3_mvm0_tx_tid),
    .slave_tdest(layer3_mvm0_tx_tdest),
    .slave_tuser(layer3_mvm0_tx_tuser),
    .slave_tlast(layer3_mvm0_tx_tlast)
);
noc_router_adapter_block_inst noc_router_layer3_mvm1(
	.clk(clk),
    .reset(reset),
    .master_tready(layer3_mvm1_connector_rx_tready),
    .master_tdata(layer3_mvm1_rx_tdata),
	.master_tvalid(layer3_mvm1_rx_valid),
    .master_tstrb(layer3_mvm1_rx_tstrb),
    .master_tkeep(layer3_mvm1_rx_tkeep),
    .master_tid(layer3_mvm1_rx_tid),
    .master_tdest(layer3_mvm1_rx_tdest),
    .master_tuser(layer3_mvm1_rx_tuser),
    .master_tlast(layer3_mvm1_rx_tlast),
    .slave_tvalid(layer3_mvm1_tx_valid),
    .slave_tready(layer3_mvm1_connector_tx_tready), 
    .slave_tdata(layer3_mvm1_tx_tdata),
    .slave_tstrb(layer3_mvm1_tx_tstrb),
    .slave_tkeep(layer3_mvm1_tx_tkeep),
    .slave_tid(layer3_mvm1_tx_tid),
    .slave_tdest(layer3_mvm1_tx_tdest),
    .slave_tuser(layer3_mvm1_tx_tuser),
    .slave_tlast(layer3_mvm1_tx_tlast)
);

// collector router block
noc_router_adapter_block_inst noc_router_output_collector(
	.clk(clk),
    .reset(reset),
    .master_tready(collector_connector_rx_tready),
    .master_tdata(collector_rx_tdata),
	.master_tvalid(collector_rx_valid),
    .master_tstrb(collector_rx_tstrb),
    .master_tkeep(collector_rx_tkeep),
    .master_tid(collector_rx_tid),
    .master_tdest(collector_rx_tdest),
    .master_tuser(collector_rx_tuser),
    .master_tlast(collector_rx_tlast),
    .slave_tvalid(1'd0),
    .slave_tready(), 
    .slave_tdata(32'd0),
    .slave_tstrb(8'd0),
    .slave_tkeep(8'd0),
    .slave_tid(8'd0),
    .slave_tdest(8'd0),
    .slave_tuser(8'd0),
    .slave_tlast(1'd0)
);

// collector unit
collector collector(
	.clk(clk),
	.rst(reset),
	.rx_tvalid(collector_rx_valid),
	.rx_tdata(collector_rx_tdata),
	.rx_tstrb(collector_rx_tstrb),
	.rx_tkeep(collector_rx_tkeep),
	.rx_tid(collector_rx_tid),
	.rx_tdest(collector_rx_tdest),
	.rx_tuser(collector_rx_tuser),
	.rx_tlast(collector_rx_tlast),
	.rx_tready(collector_connector_rx_tready),
	.ofifo_rdata(collector_ofifo_rdata),
	.ofifo_ren(collector_ofifo_ren),
	.ofifo_rdy(collector_ofifo_rdy)
);

wire tvalid_temp_layer0, tvalid_temp_layer1, tvalid_temp_layer2, tvalid_temp_layer3;

assign tvalid_temp_layer0 = layer0_mvm0_tx_valid | layer0_mvm1_tx_valid | layer0_mvm2_tx_valid | layer0_mvm3_tx_valid;
assign tvalid_temp_layer1 = layer1_mvm0_tx_valid | layer1_mvm1_tx_valid | layer1_mvm2_tx_valid;
assign tvalid_temp_layer2 = layer2_mvm0_tx_valid | layer2_mvm1_tx_valid;
assign tvalid_temp_layer3 = layer3_mvm0_tx_valid | layer3_mvm1_tx_valid;
assign tx_top_tvalid = tvalid_temp_layer0 | tvalid_temp_layer1 | tvalid_temp_layer2 | tvalid_temp_layer3;

wire [511:0] tdata_temp_layer0, tdata_temp_layer1, tdata_temp_layer2, tdata_temp_layer3;

assign tdata_temp_layer0 = layer0_mvm0_tx_tdata | layer0_mvm1_tx_tdata | layer0_mvm2_tx_tdata | layer0_mvm3_tx_tdata;
assign tdata_temp_layer1 = layer1_mvm0_tx_tdata | layer1_mvm1_tx_tdata | layer1_mvm2_tx_tdata;
assign tdata_temp_layer2 = layer2_mvm0_tx_tdata | layer2_mvm1_tx_tdata;
assign tdata_temp_layer3 = layer3_mvm0_tx_tdata | layer3_mvm1_tx_tdata;
assign tx_top_tdata = tdata_temp_layer0 | tdata_temp_layer1 | tdata_temp_layer2 | tdata_temp_layer3;

wire [63:0] tstrb_temp_layer0, tstrb_temp_layer1, tstrb_temp_layer2, tstrb_temp_layer3;
assign tstrb_temp_layer0 = layer0_mvm0_tx_tstrb | layer0_mvm1_tx_tstrb | layer0_mvm2_tx_tstrb | layer0_mvm3_tx_tstrb;
assign tstrb_temp_layer1 = layer1_mvm0_tx_tstrb | layer1_mvm1_tx_tstrb | layer1_mvm2_tx_tstrb;
assign tstrb_temp_layer2 = layer2_mvm0_tx_tstrb | layer2_mvm1_tx_tstrb;
assign tstrb_temp_layer3 = layer3_mvm0_tx_tstrb | layer3_mvm1_tx_tstrb;
assign tx_top_tstrb = tstrb_temp_layer0 | tstrb_temp_layer1 | tstrb_temp_layer2 | tstrb_temp_layer3;

wire [63:0] tkeep_temp_layer0, tkeep_temp_layer1, tkeep_temp_layer2, tkeep_temp_layer3;
assign tkeep_temp_layer0 = layer0_mvm0_tx_tkeep | layer0_mvm1_tx_tkeep | layer0_mvm2_tx_tkeep | layer0_mvm3_tx_tkeep;
assign tkeep_temp_layer1 = layer1_mvm0_tx_tkeep | layer1_mvm1_tx_tkeep | layer1_mvm2_tx_tkeep;
assign tkeep_temp_layer2 = layer2_mvm0_tx_tkeep | layer2_mvm1_tx_tkeep;
assign tkeep_temp_layer3 = layer3_mvm0_tx_tkeep | layer3_mvm1_tx_tkeep;
assign tx_top_tkeep = tkeep_temp_layer0 | tkeep_temp_layer1 | tkeep_temp_layer2 | tkeep_temp_layer3;

wire [7:0] tid_temp_layer0, tid_temp_layer1, tid_temp_layer2, tid_temp_layer3;
assign tid_temp_layer0 = layer0_mvm0_tx_tid | layer0_mvm1_tx_tid | layer0_mvm2_tx_tid | layer0_mvm3_tx_tid;
assign tid_temp_layer1 = layer1_mvm0_tx_tid | layer1_mvm1_tx_tid | layer1_mvm2_tx_tid;
assign tid_temp_layer2 = layer2_mvm0_tx_tid | layer2_mvm1_tx_tid;
assign tid_temp_layer3 = layer3_mvm0_tx_tid | layer3_mvm1_tx_tid;
assign tx_top_tid = tid_temp_layer0 | tid_temp_layer1 | tid_temp_layer2 | tid_temp_layer3;

wire [7:0] tdest_temp_layer0, tdest_temp_layer1, tdest_temp_layer2, tdest_temp_layer3;
assign tdest_temp_layer0 = layer0_mvm0_tx_tdest | layer0_mvm1_tx_tdest | layer0_mvm2_tx_tdest | layer0_mvm3_tx_tdest;
assign tdest_temp_layer1 = layer1_mvm0_tx_tdest | layer1_mvm1_tx_tdest | layer1_mvm2_tx_tdest;
assign tdest_temp_layer2 = layer2_mvm0_tx_tdest | layer2_mvm1_tx_tdest;
assign tdest_temp_layer3 = layer3_mvm0_tx_tdest | layer3_mvm1_tx_tdest;
assign tx_top_tdest = tdest_temp_layer0 | tdest_temp_layer1 | tdest_temp_layer2 | tdest_temp_layer3;

wire [31:0] tuser_temp_layer0, tuser_temp_layer1, tuser_temp_layer2, tuser_temp_layer3;
assign tuser_temp_layer0 = layer0_mvm0_tx_tuser | layer0_mvm1_tx_tuser | layer0_mvm2_tx_tuser | layer0_mvm3_tx_tuser;
assign tuser_temp_layer1 = layer1_mvm0_tx_tuser | layer1_mvm1_tx_tuser | layer1_mvm2_tx_tuser;
assign tuser_temp_layer2 = layer2_mvm0_tx_tuser | layer2_mvm1_tx_tuser;
assign tuser_temp_layer3 = layer3_mvm0_tx_tuser | layer3_mvm1_tx_tuser;
assign tx_top_tuser = tuser_temp_layer0 | tuser_temp_layer1 | tuser_temp_layer2 | tuser_temp_layer3;

wire tlast_temp_layer0, tlast_temp_layer1, tlast_temp_layer2, tlast_temp_layer3;
assign tlast_temp_layer0 = layer0_mvm0_tx_tlast | layer0_mvm1_tx_tlast | layer0_mvm2_tx_tlast | layer0_mvm3_tx_tlast;
assign tlast_temp_layer1 = layer1_mvm0_tx_tlast | layer1_mvm1_tx_tlast | layer1_mvm2_tx_tlast;
assign tlast_temp_layer2 = layer2_mvm0_tx_tlast | layer2_mvm1_tx_tlast;
assign tlast_temp_layer3 = layer3_mvm0_tx_tlast | layer3_mvm1_tx_tlast;
assign tx_top_tlast = tlast_temp_layer0 | tlast_temp_layer1 | tlast_temp_layer2 | tlast_temp_layer3;

endmodule

/* Simplified dispatcher used for FPT'23 */

module collector (
  clk,
  rst,
  rx_tvalid,
  rx_tdata,
  rx_tstrb,
  rx_tkeep,
  rx_tid,
  rx_tdest,
  rx_tuser,
  rx_tlast,
  rx_tready,
  ofifo_rdata,
  ofifo_ren,
  ofifo_rdy
);

input  clk;
input  rst;
// Rx interface
input  rx_tvalid;
input  [511:0] rx_tdata;
input  [63:0] rx_tstrb;
input  [63:0] rx_tkeep;
input  [7:0] rx_tid;
input  [7:0] rx_tdest;
input  [31:0] rx_tuser;
input  rx_tlast;
output rx_tready;
// External FIFO IO
output [63:0] ofifo_rdata;
input  ofifo_ren;
output ofifo_rdy;

wire fifo_push;
assign fifo_push = rx_tvalid && rx_tready;
wire fifo_full_signal, fifo_empty_signal;
wire [511:0] fifo_rdata;
assign ofifo_rdata[0] = fifo_rdata[0] ^ fifo_rdata[1] ^ fifo_rdata[2] ^ fifo_rdata[3] ^ fifo_rdata[4] ^ fifo_rdata[5] ^ fifo_rdata[6] ^ fifo_rdata[7]; 
assign ofifo_rdata[1] = fifo_rdata[8] ^ fifo_rdata[9] ^ fifo_rdata[10] ^ fifo_rdata[11] ^ fifo_rdata[12] ^ fifo_rdata[13] ^ fifo_rdata[14] ^ fifo_rdata[15];
assign ofifo_rdata[2] = fifo_rdata[16] ^ fifo_rdata[17] ^ fifo_rdata[18] ^ fifo_rdata[19] ^ fifo_rdata[20] ^ fifo_rdata[21] ^ fifo_rdata[22] ^ fifo_rdata[23];
assign ofifo_rdata[3] = fifo_rdata[24] ^ fifo_rdata[25] ^ fifo_rdata[26] ^ fifo_rdata[27] ^ fifo_rdata[28] ^ fifo_rdata[29] ^ fifo_rdata[30] ^ fifo_rdata[31];
assign ofifo_rdata[4] = fifo_rdata[32] ^ fifo_rdata[33] ^ fifo_rdata[34] ^ fifo_rdata[35] ^ fifo_rdata[36] ^ fifo_rdata[37] ^ fifo_rdata[38] ^ fifo_rdata[39];
assign ofifo_rdata[5] = fifo_rdata[40] ^ fifo_rdata[41] ^ fifo_rdata[42] ^ fifo_rdata[43] ^ fifo_rdata[44] ^ fifo_rdata[45] ^ fifo_rdata[46] ^ fifo_rdata[47];
assign ofifo_rdata[6] = fifo_rdata[48] ^ fifo_rdata[49] ^ fifo_rdata[50] ^ fifo_rdata[51] ^ fifo_rdata[52] ^ fifo_rdata[53] ^ fifo_rdata[54] ^ fifo_rdata[55];
assign ofifo_rdata[7] = fifo_rdata[56] ^ fifo_rdata[57] ^ fifo_rdata[58] ^ fifo_rdata[59] ^ fifo_rdata[60] ^ fifo_rdata[61] ^ fifo_rdata[62] ^ fifo_rdata[63];
assign ofifo_rdata[8] = fifo_rdata[64] ^ fifo_rdata[65] ^ fifo_rdata[66] ^ fifo_rdata[67] ^ fifo_rdata[68] ^ fifo_rdata[69] ^ fifo_rdata[70] ^ fifo_rdata[71];
assign ofifo_rdata[9] = fifo_rdata[72] ^ fifo_rdata[73] ^ fifo_rdata[74] ^ fifo_rdata[75] ^ fifo_rdata[76] ^ fifo_rdata[77] ^ fifo_rdata[78] ^ fifo_rdata[79];
assign ofifo_rdata[10] = fifo_rdata[80] ^ fifo_rdata[81] ^ fifo_rdata[82] ^ fifo_rdata[83] ^ fifo_rdata[84] ^ fifo_rdata[85] ^ fifo_rdata[86] ^ fifo_rdata[87];
assign ofifo_rdata[11] = fifo_rdata[88] ^ fifo_rdata[89] ^ fifo_rdata[90] ^ fifo_rdata[91] ^ fifo_rdata[92] ^ fifo_rdata[93] ^ fifo_rdata[94] ^ fifo_rdata[95];
assign ofifo_rdata[12] = fifo_rdata[96] ^ fifo_rdata[97] ^ fifo_rdata[98] ^ fifo_rdata[99] ^ fifo_rdata[100] ^ fifo_rdata[101] ^ fifo_rdata[102] ^ fifo_rdata[103];
assign ofifo_rdata[13] = fifo_rdata[104] ^ fifo_rdata[105] ^ fifo_rdata[106] ^ fifo_rdata[107] ^ fifo_rdata[108] ^ fifo_rdata[109] ^ fifo_rdata[110] ^ fifo_rdata[111]; 
assign ofifo_rdata[14] = fifo_rdata[112] ^ fifo_rdata[113] ^ fifo_rdata[114] ^ fifo_rdata[115] ^ fifo_rdata[116] ^ fifo_rdata[117] ^ fifo_rdata[118] ^ fifo_rdata[119]; 
assign ofifo_rdata[15] = fifo_rdata[120] ^ fifo_rdata[121] ^ fifo_rdata[122] ^ fifo_rdata[123] ^ fifo_rdata[124] ^ fifo_rdata[125] ^ fifo_rdata[126] ^ fifo_rdata[127]; 
assign ofifo_rdata[16] = fifo_rdata[128] ^ fifo_rdata[129] ^ fifo_rdata[130] ^ fifo_rdata[131] ^ fifo_rdata[132] ^ fifo_rdata[133] ^ fifo_rdata[134] ^ fifo_rdata[135]; 
assign ofifo_rdata[17] = fifo_rdata[136] ^ fifo_rdata[137] ^ fifo_rdata[138] ^ fifo_rdata[139] ^ fifo_rdata[140] ^ fifo_rdata[141] ^ fifo_rdata[142] ^ fifo_rdata[143]; 
assign ofifo_rdata[18] = fifo_rdata[144] ^ fifo_rdata[145] ^ fifo_rdata[146] ^ fifo_rdata[147] ^ fifo_rdata[148] ^ fifo_rdata[149] ^ fifo_rdata[150] ^ fifo_rdata[151]; 
assign ofifo_rdata[19] = fifo_rdata[152] ^ fifo_rdata[153] ^ fifo_rdata[154] ^ fifo_rdata[155] ^ fifo_rdata[156] ^ fifo_rdata[157] ^ fifo_rdata[158] ^ fifo_rdata[159]; 
assign ofifo_rdata[20] = fifo_rdata[160] ^ fifo_rdata[161] ^ fifo_rdata[162] ^ fifo_rdata[163] ^ fifo_rdata[164] ^ fifo_rdata[165] ^ fifo_rdata[166] ^ fifo_rdata[167]; 
assign ofifo_rdata[21] = fifo_rdata[168] ^ fifo_rdata[169] ^ fifo_rdata[170] ^ fifo_rdata[171] ^ fifo_rdata[172] ^ fifo_rdata[173] ^ fifo_rdata[174] ^ fifo_rdata[175]; 
assign ofifo_rdata[22] = fifo_rdata[176] ^ fifo_rdata[177] ^ fifo_rdata[178] ^ fifo_rdata[179] ^ fifo_rdata[180] ^ fifo_rdata[181] ^ fifo_rdata[182] ^ fifo_rdata[183]; 
assign ofifo_rdata[23] = fifo_rdata[184] ^ fifo_rdata[185] ^ fifo_rdata[186] ^ fifo_rdata[187] ^ fifo_rdata[188] ^ fifo_rdata[189] ^ fifo_rdata[190] ^ fifo_rdata[191]; 
assign ofifo_rdata[24] = fifo_rdata[192] ^ fifo_rdata[193] ^ fifo_rdata[194] ^ fifo_rdata[195] ^ fifo_rdata[196] ^ fifo_rdata[197] ^ fifo_rdata[198] ^ fifo_rdata[199]; 
assign ofifo_rdata[25] = fifo_rdata[200] ^ fifo_rdata[201] ^ fifo_rdata[202] ^ fifo_rdata[203] ^ fifo_rdata[204] ^ fifo_rdata[205] ^ fifo_rdata[206] ^ fifo_rdata[207]; 
assign ofifo_rdata[26] = fifo_rdata[208] ^ fifo_rdata[209] ^ fifo_rdata[210] ^ fifo_rdata[211] ^ fifo_rdata[212] ^ fifo_rdata[213] ^ fifo_rdata[214] ^ fifo_rdata[215]; 
assign ofifo_rdata[27] = fifo_rdata[216] ^ fifo_rdata[217] ^ fifo_rdata[218] ^ fifo_rdata[219] ^ fifo_rdata[220] ^ fifo_rdata[221] ^ fifo_rdata[222] ^ fifo_rdata[223]; 
assign ofifo_rdata[28] = fifo_rdata[224] ^ fifo_rdata[225] ^ fifo_rdata[226] ^ fifo_rdata[227] ^ fifo_rdata[228] ^ fifo_rdata[229] ^ fifo_rdata[230] ^ fifo_rdata[231]; 
assign ofifo_rdata[29] = fifo_rdata[232] ^ fifo_rdata[233] ^ fifo_rdata[234] ^ fifo_rdata[235] ^ fifo_rdata[236] ^ fifo_rdata[237] ^ fifo_rdata[238] ^ fifo_rdata[239]; 
assign ofifo_rdata[30] = fifo_rdata[240] ^ fifo_rdata[241] ^ fifo_rdata[242] ^ fifo_rdata[243] ^ fifo_rdata[244] ^ fifo_rdata[245] ^ fifo_rdata[246] ^ fifo_rdata[247]; 
assign ofifo_rdata[31] = fifo_rdata[248] ^ fifo_rdata[249] ^ fifo_rdata[250] ^ fifo_rdata[251] ^ fifo_rdata[252] ^ fifo_rdata[253] ^ fifo_rdata[254] ^ fifo_rdata[255]; 
assign ofifo_rdata[32] = fifo_rdata[256] ^ fifo_rdata[257] ^ fifo_rdata[258] ^ fifo_rdata[259] ^ fifo_rdata[260] ^ fifo_rdata[261] ^ fifo_rdata[262] ^ fifo_rdata[263]; 
assign ofifo_rdata[33] = fifo_rdata[264] ^ fifo_rdata[265] ^ fifo_rdata[266] ^ fifo_rdata[267] ^ fifo_rdata[268] ^ fifo_rdata[269] ^ fifo_rdata[270] ^ fifo_rdata[271]; 
assign ofifo_rdata[34] = fifo_rdata[272] ^ fifo_rdata[273] ^ fifo_rdata[274] ^ fifo_rdata[275] ^ fifo_rdata[276] ^ fifo_rdata[277] ^ fifo_rdata[278] ^ fifo_rdata[279]; 
assign ofifo_rdata[35] = fifo_rdata[280] ^ fifo_rdata[281] ^ fifo_rdata[282] ^ fifo_rdata[283] ^ fifo_rdata[284] ^ fifo_rdata[285] ^ fifo_rdata[286] ^ fifo_rdata[287]; 
assign ofifo_rdata[36] = fifo_rdata[288] ^ fifo_rdata[289] ^ fifo_rdata[290] ^ fifo_rdata[291] ^ fifo_rdata[292] ^ fifo_rdata[293] ^ fifo_rdata[294] ^ fifo_rdata[295]; 
assign ofifo_rdata[37] = fifo_rdata[296] ^ fifo_rdata[297] ^ fifo_rdata[298] ^ fifo_rdata[299] ^ fifo_rdata[300] ^ fifo_rdata[301] ^ fifo_rdata[302] ^ fifo_rdata[303]; 
assign ofifo_rdata[38] = fifo_rdata[304] ^ fifo_rdata[305] ^ fifo_rdata[306] ^ fifo_rdata[307] ^ fifo_rdata[308] ^ fifo_rdata[309] ^ fifo_rdata[310] ^ fifo_rdata[311]; 
assign ofifo_rdata[39] = fifo_rdata[312] ^ fifo_rdata[313] ^ fifo_rdata[314] ^ fifo_rdata[315] ^ fifo_rdata[316] ^ fifo_rdata[317] ^ fifo_rdata[318] ^ fifo_rdata[319]; 
assign ofifo_rdata[40] = fifo_rdata[320] ^ fifo_rdata[321] ^ fifo_rdata[322] ^ fifo_rdata[323] ^ fifo_rdata[324] ^ fifo_rdata[325] ^ fifo_rdata[326] ^ fifo_rdata[327]; 
assign ofifo_rdata[41] = fifo_rdata[328] ^ fifo_rdata[329] ^ fifo_rdata[330] ^ fifo_rdata[331] ^ fifo_rdata[332] ^ fifo_rdata[333] ^ fifo_rdata[334] ^ fifo_rdata[335]; 
assign ofifo_rdata[42] = fifo_rdata[336] ^ fifo_rdata[337] ^ fifo_rdata[338] ^ fifo_rdata[339] ^ fifo_rdata[340] ^ fifo_rdata[341] ^ fifo_rdata[342] ^ fifo_rdata[343]; 
assign ofifo_rdata[43] = fifo_rdata[344] ^ fifo_rdata[345] ^ fifo_rdata[346] ^ fifo_rdata[347] ^ fifo_rdata[348] ^ fifo_rdata[349] ^ fifo_rdata[350] ^ fifo_rdata[351]; 
assign ofifo_rdata[44] = fifo_rdata[352] ^ fifo_rdata[353] ^ fifo_rdata[354] ^ fifo_rdata[355] ^ fifo_rdata[356] ^ fifo_rdata[357] ^ fifo_rdata[358] ^ fifo_rdata[359]; 
assign ofifo_rdata[45] = fifo_rdata[360] ^ fifo_rdata[361] ^ fifo_rdata[362] ^ fifo_rdata[363] ^ fifo_rdata[364] ^ fifo_rdata[365] ^ fifo_rdata[366] ^ fifo_rdata[367]; 
assign ofifo_rdata[46] = fifo_rdata[368] ^ fifo_rdata[369] ^ fifo_rdata[370] ^ fifo_rdata[371] ^ fifo_rdata[372] ^ fifo_rdata[373] ^ fifo_rdata[374] ^ fifo_rdata[375]; 
assign ofifo_rdata[47] = fifo_rdata[376] ^ fifo_rdata[377] ^ fifo_rdata[378] ^ fifo_rdata[379] ^ fifo_rdata[380] ^ fifo_rdata[381] ^ fifo_rdata[382] ^ fifo_rdata[383]; 
assign ofifo_rdata[48] = fifo_rdata[384] ^ fifo_rdata[385] ^ fifo_rdata[386] ^ fifo_rdata[387] ^ fifo_rdata[388] ^ fifo_rdata[389] ^ fifo_rdata[390] ^ fifo_rdata[391]; 
assign ofifo_rdata[49] = fifo_rdata[392] ^ fifo_rdata[393] ^ fifo_rdata[394] ^ fifo_rdata[395] ^ fifo_rdata[396] ^ fifo_rdata[397] ^ fifo_rdata[398] ^ fifo_rdata[399]; 
assign ofifo_rdata[50] = fifo_rdata[400] ^ fifo_rdata[401] ^ fifo_rdata[402] ^ fifo_rdata[403] ^ fifo_rdata[404] ^ fifo_rdata[405] ^ fifo_rdata[406] ^ fifo_rdata[407]; 
assign ofifo_rdata[51] = fifo_rdata[408] ^ fifo_rdata[409] ^ fifo_rdata[410] ^ fifo_rdata[411] ^ fifo_rdata[412] ^ fifo_rdata[413] ^ fifo_rdata[414] ^ fifo_rdata[415]; 
assign ofifo_rdata[52] = fifo_rdata[416] ^ fifo_rdata[417] ^ fifo_rdata[418] ^ fifo_rdata[419] ^ fifo_rdata[420] ^ fifo_rdata[421] ^ fifo_rdata[422] ^ fifo_rdata[423]; 
assign ofifo_rdata[53] = fifo_rdata[424] ^ fifo_rdata[425] ^ fifo_rdata[426] ^ fifo_rdata[427] ^ fifo_rdata[428] ^ fifo_rdata[429] ^ fifo_rdata[430] ^ fifo_rdata[431]; 
assign ofifo_rdata[54] = fifo_rdata[432] ^ fifo_rdata[433] ^ fifo_rdata[434] ^ fifo_rdata[435] ^ fifo_rdata[436] ^ fifo_rdata[437] ^ fifo_rdata[438] ^ fifo_rdata[439]; 
assign ofifo_rdata[55] = fifo_rdata[440] ^ fifo_rdata[441] ^ fifo_rdata[442] ^ fifo_rdata[443] ^ fifo_rdata[444] ^ fifo_rdata[445] ^ fifo_rdata[446] ^ fifo_rdata[447]; 
assign ofifo_rdata[56] = fifo_rdata[448] ^ fifo_rdata[449] ^ fifo_rdata[450] ^ fifo_rdata[451] ^ fifo_rdata[452] ^ fifo_rdata[453] ^ fifo_rdata[454] ^ fifo_rdata[455]; 
assign ofifo_rdata[57] = fifo_rdata[456] ^ fifo_rdata[457] ^ fifo_rdata[458] ^ fifo_rdata[459] ^ fifo_rdata[460] ^ fifo_rdata[461] ^ fifo_rdata[462] ^ fifo_rdata[463]; 
assign ofifo_rdata[58] = fifo_rdata[464] ^ fifo_rdata[465] ^ fifo_rdata[466] ^ fifo_rdata[467] ^ fifo_rdata[468] ^ fifo_rdata[469] ^ fifo_rdata[470] ^ fifo_rdata[471]; 
assign ofifo_rdata[59] = fifo_rdata[472] ^ fifo_rdata[473] ^ fifo_rdata[474] ^ fifo_rdata[475] ^ fifo_rdata[476] ^ fifo_rdata[477] ^ fifo_rdata[478] ^ fifo_rdata[479]; 
assign ofifo_rdata[60] = fifo_rdata[480] ^ fifo_rdata[481] ^ fifo_rdata[482] ^ fifo_rdata[483] ^ fifo_rdata[484] ^ fifo_rdata[485] ^ fifo_rdata[486] ^ fifo_rdata[487]; 
assign ofifo_rdata[61] = fifo_rdata[488] ^ fifo_rdata[489] ^ fifo_rdata[490] ^ fifo_rdata[491] ^ fifo_rdata[492] ^ fifo_rdata[493] ^ fifo_rdata[494] ^ fifo_rdata[495]; 
assign ofifo_rdata[62] = fifo_rdata[496] ^ fifo_rdata[497] ^ fifo_rdata[498] ^ fifo_rdata[499] ^ fifo_rdata[500] ^ fifo_rdata[501] ^ fifo_rdata[502] ^ fifo_rdata[503]; 
assign ofifo_rdata[63] = fifo_rdata[504] ^ fifo_rdata[505] ^ fifo_rdata[506] ^ fifo_rdata[507] ^ fifo_rdata[508] ^ fifo_rdata[509] ^ fifo_rdata[510] ^ fifo_rdata[511];


fifo_collector fifo_inst (
	.clk(clk),
	.rst(rst),
	.push(fifo_push),
	.idata(rx_tdata),
	.pop(ofifo_ren),
	.odata(fifo_rdata),
	.empty(fifo_empty_signal),
	.full(fifo_full_signal)
);

assign rx_tready = !fifo_empty_signal;
assign ofifo_rdy = !fifo_full_signal;

endmodule

module fifo_collector (
  clk,
  rst,
  push,
  idata,
  pop,
  odata,
  empty,
  full
);

input  wire clk;
input  wire rst;
input  wire push;
input  wire [511:0] idata;
input  wire pop;
output wire [511:0] odata;
output reg empty;
output reg full;

reg [8:0] head_ptr;
reg [8:0] tail_ptr;

bram_inst b0  (.clk(clk), .wen(push), .wdata(idata[(32*1)-1:32*0]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*1)-1:32*0]));
bram_inst b1  (.clk(clk), .wen(push), .wdata(idata[(32*2)-1:32*1]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*2)-1:32*1]));
bram_inst b2  (.clk(clk), .wen(push), .wdata(idata[(32*3)-1:32*2]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*3)-1:32*2]));
bram_inst b3  (.clk(clk), .wen(push), .wdata(idata[(32*4)-1:32*3]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*4)-1:32*3]));
bram_inst b4  (.clk(clk), .wen(push), .wdata(idata[(32*5)-1:32*4]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*5)-1:32*4]));
bram_inst b5  (.clk(clk), .wen(push), .wdata(idata[(32*6)-1:32*5]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*6)-1:32*5]));
bram_inst b6  (.clk(clk), .wen(push), .wdata(idata[(32*7)-1:32*6]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*7)-1:32*6]));
bram_inst b7  (.clk(clk), .wen(push), .wdata(idata[(32*8)-1:32*7]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*8)-1:32*7]));
bram_inst b8  (.clk(clk), .wen(push), .wdata(idata[(32*9)-1:32*8]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*9)-1:32*8]));
bram_inst b9  (.clk(clk), .wen(push), .wdata(idata[(32*10)-1:32*9]),  .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*10)-1:32*9]));
bram_inst b10 (.clk(clk), .wen(push), .wdata(idata[(32*11)-1:32*10]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*11)-1:32*10]));
bram_inst b11 (.clk(clk), .wen(push), .wdata(idata[(32*12)-1:32*11]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*12)-1:32*11]));
bram_inst b12 (.clk(clk), .wen(push), .wdata(idata[(32*13)-1:32*12]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*13)-1:32*12]));
bram_inst b13 (.clk(clk), .wen(push), .wdata(idata[(32*14)-1:32*13]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*14)-1:32*13]));
bram_inst b14 (.clk(clk), .wen(push), .wdata(idata[(32*15)-1:32*14]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*15)-1:32*14]));
bram_inst b15 (.clk(clk), .wen(push), .wdata(idata[(32*16)-1:32*15]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*16)-1:32*15]));

always @ (posedge clk) begin
	if (rst) begin
		head_ptr <= 0;
		tail_ptr <= 0;
	end else begin
		if (push) tail_ptr <= tail_ptr + 1;		
		if (pop)  head_ptr <= head_ptr + 1;
	end
end

always @ (*) begin
  if (tail_ptr == head_ptr) begin
    empty = 1'b1;
  end else begin
    empty = 1'b0;
  end

  if (tail_ptr+1 == head_ptr) begin
    full = 1'b1;
  end else begin
    full = 1'b0;
  end
end

endmodule


/* Simplified dispatcher used for FPT'23 */

module dispatcher (
	clk,
	rst,
	tx_tvalid,
	tx_tdata,
	tx_tstrb,
	tx_tkeep,
	tx_tid,
	tx_tdest,
	tx_tuser,
	tx_tlast,
	tx_tready,
	ififo_wdata,
	ififo_wen,
	ififo_rdy
);

input  clk;
input  rst;
// Tx interface
output tx_tvalid;
output [511:0] tx_tdata;
output [63:0] tx_tstrb;
output [63:0] tx_tkeep;
output [7:0] tx_tid;
output [7:0] tx_tdest;
output [31:0] tx_tuser;
output tx_tlast;
input  tx_tready;
// External FIFO IO
input  [63:0] ififo_wdata;
input  ififo_wen;
output ififo_rdy;

wire fifo_full_signal, fifo_almost_full_signal, fifo_empty_signal;
wire [511:0] fifo_rdata;
wire [511:0] fifo_wdata;
assign fifo_wdata[ 63:  0] = ififo_wdata;
assign fifo_wdata[127: 64] = ififo_wdata;
assign fifo_wdata[191:128] = ififo_wdata;
assign fifo_wdata[255:192] = ififo_wdata;
assign fifo_wdata[319:256] = ififo_wdata;
assign fifo_wdata[383:320] = ififo_wdata;
assign fifo_wdata[447:384] = ififo_wdata;
assign fifo_wdata[511:448] = ififo_wdata;

fifo_dispatcher fifo_inst (
	.clk(clk),
	.rst(rst),
	.push(ififo_wen),
	.idata(fifo_wdata),
	.pop(tx_tvalid && tx_tready),
	.odata(fifo_rdata),
	.empty(fifo_empty_signal),
	.full(fifo_full_signal)
);

reg [63:0] r_tx_tstrb;
reg [63:0] r_tx_tkeep;
reg [7:0] r_tx_tid;
reg [7:0] r_tx_tdest;
reg [31:0] r_tx_tuser;
reg r_tx_tlast;

always @ (posedge clk) begin
	if (rst) begin
		r_tx_tstrb <= 0;
		r_tx_tkeep <= 0;
		r_tx_tid   <= 0;
		r_tx_tdest <= 0;
		r_tx_tuser <= 0;
		r_tx_tlast <= 0;
	end else begin
		r_tx_tstrb <= ififo_wdata[63:0];
		r_tx_tkeep <= ififo_wdata[63:0];
		r_tx_tid   <= ififo_wdata[7:0] ^ ififo_wdata[15:8];
		r_tx_tdest <= ififo_wdata[7:0] ^ ififo_wdata[23:16];
		r_tx_tuser <= ififo_wdata[31:0] ^ ififo_wdata[63:32];
		r_tx_tlast <= ififo_wdata[63] ^ ififo_wdata[0];
	end
end

assign tx_tstrb = r_tx_tstrb;
assign tx_tkeep = r_tx_tkeep;
assign tx_tid   = r_tx_tid;
assign tx_tdest = r_tx_tdest;
assign tx_tuser = r_tx_tuser; 
assign tx_tlast = r_tx_tlast; 

assign tx_tvalid = !fifo_empty_signal;
assign tx_tdata = fifo_rdata;
assign ififo_rdy = !fifo_full_signal;

endmodule

module fifo_dispatcher (
  clk,
  rst,
  push,
  idata,
  pop,
  odata,
  empty,
  full
);

input  wire clk;
input  wire rst;
input  wire push;
input  wire [511:0] idata;
input  wire pop;
output wire [511:0] odata;
output reg empty;
output reg full;

reg [8:0] head_ptr;
reg [8:0] tail_ptr;

bram_inst b0  (.clk(clk), .wen(push), .wdata(idata[(32*1)-1:32*0]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*1)-1:32*0]));
bram_inst b1  (.clk(clk), .wen(push), .wdata(idata[(32*2)-1:32*1]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*2)-1:32*1]));
bram_inst b2  (.clk(clk), .wen(push), .wdata(idata[(32*3)-1:32*2]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*3)-1:32*2]));
bram_inst b3  (.clk(clk), .wen(push), .wdata(idata[(32*4)-1:32*3]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*4)-1:32*3]));
bram_inst b4  (.clk(clk), .wen(push), .wdata(idata[(32*5)-1:32*4]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*5)-1:32*4]));
bram_inst b5  (.clk(clk), .wen(push), .wdata(idata[(32*6)-1:32*5]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*6)-1:32*5]));
bram_inst b6  (.clk(clk), .wen(push), .wdata(idata[(32*7)-1:32*6]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*7)-1:32*6]));
bram_inst b7  (.clk(clk), .wen(push), .wdata(idata[(32*8)-1:32*7]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*8)-1:32*7]));
bram_inst b8  (.clk(clk), .wen(push), .wdata(idata[(32*9)-1:32*8]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*9)-1:32*8]));
bram_inst b9  (.clk(clk), .wen(push), .wdata(idata[(32*10)-1:32*9]),  .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*10)-1:32*9]));
bram_inst b10 (.clk(clk), .wen(push), .wdata(idata[(32*11)-1:32*10]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*11)-1:32*10]));
bram_inst b11 (.clk(clk), .wen(push), .wdata(idata[(32*12)-1:32*11]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*12)-1:32*11]));
bram_inst b12 (.clk(clk), .wen(push), .wdata(idata[(32*13)-1:32*12]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*13)-1:32*12]));
bram_inst b13 (.clk(clk), .wen(push), .wdata(idata[(32*14)-1:32*13]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*14)-1:32*13]));
bram_inst b14 (.clk(clk), .wen(push), .wdata(idata[(32*15)-1:32*14]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*15)-1:32*14]));
bram_inst b15 (.clk(clk), .wen(push), .wdata(idata[(32*16)-1:32*15]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*16)-1:32*15]));

always @ (posedge clk) begin
	if (rst) begin
		head_ptr <= 0;
		tail_ptr <= 0;
	end else begin
		if (push) tail_ptr <= tail_ptr + 1;		
		if (pop)  head_ptr <= head_ptr + 1;
	end
end

always @ (*) begin
  if (tail_ptr == head_ptr) begin
    empty = 1'b1;
  end else begin
    empty = 1'b0;
  end

  if (tail_ptr+1 == head_ptr) begin
    full = 1'b1;
  end else begin
    full = 1'b0;
  end
end

endmodule

module mvm_top (
	clk,
	rst,
	rx_tvalid,
	rx_tdata,
	rx_tstrb,
	rx_tkeep,
	rx_tid,
	rx_tdest,
	rx_tuser,
	rx_tlast,
	rx_tready,	
	tx_tvalid,
	tx_tdata,
	tx_tstrb,
	tx_tkeep,
	tx_tid,
	tx_tdest,
	tx_tuser,
	tx_tlast,
	tx_tready
);

input wire          clk;
input wire          rst;
// Rx interface
input wire          rx_tvalid;
input wire  [511:0] rx_tdata;
input wire  [ 63:0] rx_tstrb;
input wire  [ 63:0] rx_tkeep;
input wire  [  7:0] rx_tid;
input wire  [  7:0] rx_tdest;
input wire  [ 31:0] rx_tuser;
input wire          rx_tlast;
output wire         rx_tready;	
// Tx interface
output wire         tx_tvalid;
output wire [511:0] tx_tdata;
output wire [ 63:0] tx_tstrb;
output wire [ 63:0] tx_tkeep;
output wire [  7:0] tx_tid;
output wire [  7:0] tx_tdest;
output wire [ 31:0] tx_tuser;
output wire         tx_tlast;
input  wire         tx_tready;

// Hook up unused Rx signals to dummy registers to avoid being synthesized away
reg [63:0] dummy_rx_tstrb;
reg [63:0] dummy_rx_tkeep;
reg [63:0] dummy_rx_tdest;

always @ (posedge clk) begin
	dummy_rx_tstrb <= rx_tstrb;
	dummy_rx_tkeep <= rx_tkeep;
	dummy_rx_tdest <= rx_tdest;
end

wire [8:0] inst_raddr;
assign inst_raddr = rx_tuser[15:9];
wire [8:0] inst_waddr;
assign inst_waddr = rx_tuser[8:0];
wire inst_wen;
assign inst_wen = (rx_tid == 0);
wire [511:0] inst_wdata;
assign inst_wdata = rx_tdata;
wire [511:0] inst_rdata; 
wire [8:0] inst_rf_raddr, inst_accum_raddr;
wire inst_reduce, inst_accum_en, inst_release, inst_jump, inst_en, inst_last;

memory_block instruction_fifo (.clk(clk), .waddr(inst_waddr), .wen(inst_wen), .wdata(inst_wdata), .raddr(inst_raddr), .rdata(inst_rdata));

assign inst_rf_raddr = inst_rdata[23:15];
assign inst_accum_raddr = inst_rdata[14:6];
assign inst_last = inst_rdata[5];
assign inst_reduce = inst_rdata[2];
assign inst_accum_en = inst_rdata[3];
assign inst_release = inst_rdata[4];
assign inst_jump = inst_rdata[1];
assign inst_en = inst_rdata[0];

wire input_fifo_empty, input_fifo_full;
wire [511:0] input_fifo_idata;
assign input_fifo_idata = rx_tdata;
wire [511:0] input_fifo_odata;
wire input_fifo_push, input_fifo_pop;
assign input_fifo_push = (rx_tid == 2);
assign input_fifo_pop = inst_last;

fifo_mvm input_fifo (.clk(clk), .rst(rst), .push(input_fifo_push), .idata(input_fifo_idata), .pop(input_fifo_pop), .odata(input_fifo_odata), .empty(input_fifo_empty), .full(input_fifo_full));

wire reduction_fifo_empty, reduction_fifo_full;
wire [511:0] reduction_fifo_idata;
assign reduction_fifo_idata = rx_tdata;
wire [511:0] reduction_fifo_odata;
wire reduction_fifo_push, reduction_fifo_pop;
assign reduction_fifo_push = (rx_tid == 1);
assign reduction_fifo_pop = inst_reduce && !reduction_fifo_empty;

fifo_mvm reduction_fifo (.clk(clk), .rst(rst), .push(reduction_fifo_push), .idata(reduction_fifo_idata), .pop(reduction_fifo_pop), .odata(reduction_fifo_odata), .empty(reduction_fifo_empty), .full(reduction_fifo_full));

wire [8:0] accum_mem_waddr;
wire [511:0] accum_mem_rdata;
wire [17:0] temp_accum_addr, delay_accum_addr;
assign temp_accum_addr = {9'b0, inst_accum_raddr};

dpe_pipeline accum_addr_pipeline (.clk(clk), .rst(rst), .data_in(temp_accum_addr), .data_out(delay_accum_addr));

memory_block accum_mem (.clk(clk), .waddr(delay_accum_addr[8:0]), .wen(dpe_ovalid[0]), .wdata(dpe_results), .raddr(inst_accum_raddr), .rdata(accum_mem_rdata));

wire [8:0] rf_waddr;
assign rf_waddr = rx_tuser[8:0];
wire [511:0] rf_wdata;
assign rf_wdata = rx_tdata;
wire [15:0] rf_wen;
assign rf_wen = rx_tuser[24:9];
wire [511:0] rf_rdata_1, rf_rdata_2, rf_rdata_3, rf_rdata_4, rf_rdata_5, rf_rdata_6, rf_rdata_7, rf_rdata_8,
             rf_rdata_9, rf_rdata_10, rf_rdata_11, rf_rdata_12, rf_rdata_13, rf_rdata_14, rf_rdata_15, rf_rdata_16;

output wire [511:0] dpe_results;
output wire [15:0] dpe_ovalid;

memory_block rf_01(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[0]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_1));
memory_block rf_02(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[1]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_2));
memory_block rf_03(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[2]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_3));
memory_block rf_04(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[3]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_4));
memory_block rf_05(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[4]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_5));
memory_block rf_06(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[5]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_6));
memory_block rf_07(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[6]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_7));
memory_block rf_08(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[7]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_8));
memory_block rf_09(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[8]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_9));
memory_block rf_10(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[9]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_10));
memory_block rf_11(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[10]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_11));
memory_block rf_12(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[11]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_12));
memory_block rf_13(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[12]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_13));
memory_block rf_14(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[13]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_14));
memory_block rf_15(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[14]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_15));
memory_block rf_16(.clk(clk), .waddr(rf_waddr), .wen(rf_wen[15]), .wdata(rf_wdata), .raddr(inst_rf_raddr), .rdata(rf_rdata_16));

wire dpe_ivalid;
assign dpe_ivalid = inst_en && inst_release;
dpe dpe_01 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_1), .datac(reduction_fifo_odata[(32*1)-1:32*0]), .datad(accum_mem_rdata[(32*1)-1:32*0]), .result(dpe_results[(32*1)-1:32*0]), .ovalid(dpe_ovalid[0]));
dpe dpe_02 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_2), .datac(reduction_fifo_odata[(32*2)-1:32*1]), .datad(accum_mem_rdata[(32*2)-1:32*1]), .result(dpe_results[(32*2)-1:32*1]), .ovalid(dpe_ovalid[1]));
dpe dpe_03 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_3), .datac(reduction_fifo_odata[(32*3)-1:32*2]), .datad(accum_mem_rdata[(32*3)-1:32*2]), .result(dpe_results[(32*3)-1:32*2]), .ovalid(dpe_ovalid[2]));
dpe dpe_04 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_4), .datac(reduction_fifo_odata[(32*4)-1:32*3]), .datad(accum_mem_rdata[(32*4)-1:32*3]), .result(dpe_results[(32*4)-1:32*3]), .ovalid(dpe_ovalid[3]));
dpe dpe_05 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_5), .datac(reduction_fifo_odata[(32*5)-1:32*4]), .datad(accum_mem_rdata[(32*5)-1:32*4]), .result(dpe_results[(32*5)-1:32*4]), .ovalid(dpe_ovalid[4]));
dpe dpe_06 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_6), .datac(reduction_fifo_odata[(32*6)-1:32*5]), .datad(accum_mem_rdata[(32*6)-1:32*5]), .result(dpe_results[(32*6)-1:32*5]), .ovalid(dpe_ovalid[5]));
dpe dpe_07 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_7), .datac(reduction_fifo_odata[(32*7)-1:32*6]), .datad(accum_mem_rdata[(32*7)-1:32*6]), .result(dpe_results[(32*7)-1:32*6]), .ovalid(dpe_ovalid[6]));
dpe dpe_08 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_8), .datac(reduction_fifo_odata[(32*8)-1:32*7]), .datad(accum_mem_rdata[(32*8)-1:32*7]), .result(dpe_results[(32*8)-1:32*7]), .ovalid(dpe_ovalid[7]));
dpe dpe_09 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_9), .datac(reduction_fifo_odata[(32*9)-1:32*8]), .datad(accum_mem_rdata[(32*9)-1:32*8]), .result(dpe_results[(32*9)-1:32*8]), .ovalid(dpe_ovalid[8]));
dpe dpe_10 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_10), .datac(reduction_fifo_odata[(32*10)-1:32*9]), .datad(accum_mem_rdata[(32*10)-1:32*9]), .result(dpe_results[(32*10)-1:32*9]), .ovalid(dpe_ovalid[9]));
dpe dpe_11 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_11), .datac(reduction_fifo_odata[(32*11)-1:32*10]), .datad(accum_mem_rdata[(32*11)-1:32*10]), .result(dpe_results[(32*11)-1:32*10]), .ovalid(dpe_ovalid[10]));
dpe dpe_12 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_12), .datac(reduction_fifo_odata[(32*12)-1:32*11]), .datad(accum_mem_rdata[(32*12)-1:32*11]), .result(dpe_results[(32*12)-1:32*11]), .ovalid(dpe_ovalid[11]));
dpe dpe_13 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_13), .datac(reduction_fifo_odata[(32*13)-1:32*12]), .datad(accum_mem_rdata[(32*13)-1:32*12]), .result(dpe_results[(32*13)-1:32*12]), .ovalid(dpe_ovalid[12]));
dpe dpe_14 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_14), .datac(reduction_fifo_odata[(32*14)-1:32*13]), .datad(accum_mem_rdata[(32*14)-1:32*13]), .result(dpe_results[(32*14)-1:32*13]), .ovalid(dpe_ovalid[13]));
dpe dpe_15 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_15), .datac(reduction_fifo_odata[(32*15)-1:32*14]), .datad(accum_mem_rdata[(32*15)-1:32*14]), .result(dpe_results[(32*15)-1:32*14]), .ovalid(dpe_ovalid[14]));
dpe dpe_16 (.clk(clk), .rst(rst), .ivalid(dpe_ivalid), .accum(inst_accum_en), .reduce(inst_reduce), .dataa(input_fifo_odata), .datab(rf_rdata_16), .datac(reduction_fifo_odata[(32*16)-1:32*15]), .datad(accum_mem_rdata[(32*16)-1:32*15]), .result(dpe_results[(32*16)-1:32*15]), .ovalid(dpe_ovalid[15]));

wire output_fifo_empty, output_fifo_full;
wire [511:0] output_fifo_odata;
wire output_fifo_pop;
assign output_fifo_pop = tx_tready && !output_fifo_empty;
fifo_mvm output_fifo (.clk(clk), .rst(rst), .push(dpe_ovalid[0]), .idata(dpe_results), .pop(output_fifo_pop), .odata(output_fifo_odata), .empty(output_fifo_empty), .full(output_fifo_full));

reg [ 63:0] r_tx_tstrb;
reg [ 63:0] r_tx_tkeep;
reg [  7:0] r_tx_tid;
reg [  7:0] r_tx_tdest;
reg [ 31:0] r_tx_tuser;
reg         r_tx_tlast;
always @ (posedge clk) begin
  if (rst) begin
    r_tx_tstrb <= 0;
    r_tx_tkeep <= 0;
    r_tx_tid   <= 0;
    r_tx_tdest <= 0;
    r_tx_tuser <= 0;
    r_tx_tlast <= 0;
  end else begin
    r_tx_tstrb <= rx_tstrb;
    r_tx_tkeep <= rx_tkeep;
    r_tx_tid   <= rx_tid;
    r_tx_tdest <= rx_tdest;
    r_tx_tuser <= rx_tuser;
    r_tx_tlast <= rx_tlast;
  end
end

assign tx_tvalid = tx_tready && !output_fifo_empty;
assign tx_tdata  = output_fifo_odata;
assign tx_tstrb  = r_tx_tstrb;
assign tx_tkeep  = r_tx_tkeep;
assign tx_tid    = r_tx_tid; 
assign tx_tdest  = r_tx_tdest;
assign tx_tuser  = r_tx_tuser;
assign tx_tlast  = r_tx_tlast;
assign rx_tready = !input_fifo_full;

endmodule


module dpe (
	clk,
	rst,
	ivalid,
	accum,
	reduce,
	dataa,
	datab,
	datac,
	datad,
	result,
	ovalid
);

input  wire         clk;
input  wire         rst;
input  wire         ivalid;
input  wire         accum;
input  wire         reduce;
input  wire [511:0] dataa;
input  wire [511:0] datab;
input  wire [ 31:0] datac;
input  wire [ 31:0] datad;
output wire [ 31:0] result;
output wire         ovalid;

wire [36:0] chain_atom01_to_atom00, chain_atom02_to_atom01, chain_atom03_to_atom02, chain_atom04_to_atom03, 
						chain_atom05_to_atom04, chain_atom06_to_atom05, chain_atom07_to_atom06, chain_atom08_to_atom07, 
						chain_atom09_to_atom08, chain_atom10_to_atom09, chain_atom11_to_atom10, chain_atom12_to_atom11, 
						chain_atom13_to_atom12, chain_atom14_to_atom13, chain_atom15_to_atom14, dummy_chain;
wire [31:0] res15, res14, res13, res12, res11, res10, res09, res08, res07, res06, res05, res04, res03, res02, res01, res00;

wire [33:0] temp_datac, temp_datad, delay_datac, delay_datad;
assign temp_datac = {ivalid, accum,  datac};
assign temp_datad = {ivalid, reduce, datad};

dpe_pipeline datac_pipe (.clk(clk), .rst(rst), .data_in(temp_datac), .data_out(delay_datac));
dpe_pipeline datad_pipe (.clk(clk), .rst(rst), .data_in(temp_datad), .data_out(delay_datad));

dsp_inst d15(.clk(clk), .reset(rst), .ax(dataa[ (16*1)-1: 16*0]), .ay(datab[ (16*1)-1: 16*0]), .bx(dataa[ (16*2)-1: 16*1]), .by(datab[ (16*2)-1: 16*1]), .chainin(                 37'd0), .result(res15), .chainout(chain_atom15_to_atom14));
dsp_inst d14(.clk(clk), .reset(rst), .ax(dataa[ (16*3)-1: 16*2]), .ay(datab[ (16*3)-1: 16*2]), .bx(dataa[ (16*4)-1: 16*3]), .by(datab[ (16*4)-1: 16*3]), .chainin(chain_atom15_to_atom14), .result(res14), .chainout(chain_atom14_to_atom13));
dsp_inst d13(.clk(clk), .reset(rst), .ax(dataa[ (16*5)-1: 16*4]), .ay(datab[ (16*5)-1: 16*4]), .bx(dataa[ (16*6)-1: 16*5]), .by(datab[ (16*6)-1: 16*5]), .chainin(chain_atom14_to_atom13), .result(res13), .chainout(chain_atom13_to_atom12));
dsp_inst d12(.clk(clk), .reset(rst), .ax(dataa[ (16*7)-1: 16*6]), .ay(datab[ (16*7)-1: 16*6]), .bx(dataa[ (16*8)-1: 16*7]), .by(datab[ (16*8)-1: 16*7]), .chainin(chain_atom13_to_atom12), .result(res12), .chainout(chain_atom12_to_atom11));
dsp_inst d11(.clk(clk), .reset(rst), .ax(dataa[ (16*9)-1: 16*8]), .ay(datab[ (16*9)-1: 16*8]), .bx(dataa[(16*10)-1: 16*9]), .by(datab[(16*10)-1: 16*9]), .chainin(chain_atom12_to_atom11), .result(res11), .chainout(chain_atom11_to_atom10));
dsp_inst d10(.clk(clk), .reset(rst), .ax(dataa[(16*11)-1:16*10]), .ay(datab[(16*11)-1:16*10]), .bx(dataa[(16*12)-1:16*11]), .by(datab[(16*12)-1:16*11]), .chainin(chain_atom11_to_atom10), .result(res10), .chainout(chain_atom10_to_atom09));
dsp_inst d09(.clk(clk), .reset(rst), .ax(dataa[(16*13)-1:16*12]), .ay(datab[(16*13)-1:16*12]), .bx(dataa[(16*14)-1:16*13]), .by(datab[(16*14)-1:16*13]), .chainin(chain_atom10_to_atom09), .result(res09), .chainout(chain_atom09_to_atom08));
dsp_inst d08(.clk(clk), .reset(rst), .ax(dataa[(16*15)-1:16*14]), .ay(datab[(16*15)-1:16*14]), .bx(dataa[(16*16)-1:16*15]), .by(datab[(16*16)-1:16*15]), .chainin(chain_atom09_to_atom08), .result(res08), .chainout(chain_atom08_to_atom07));
dsp_inst d07(.clk(clk), .reset(rst), .ax(dataa[(16*17)-1:16*16]), .ay(datab[(16*17)-1:16*16]), .bx(dataa[(16*18)-1:16*17]), .by(datab[(16*18)-1:16*17]), .chainin(chain_atom08_to_atom07), .result(res07), .chainout(chain_atom07_to_atom06));
dsp_inst d06(.clk(clk), .reset(rst), .ax(dataa[(16*19)-1:16*18]), .ay(datab[(16*19)-1:16*18]), .bx(dataa[(16*20)-1:16*19]), .by(datab[(16*20)-1:16*19]), .chainin(chain_atom07_to_atom06), .result(res06), .chainout(chain_atom06_to_atom05));
dsp_inst d05(.clk(clk), .reset(rst), .ax(dataa[(16*21)-1:16*20]), .ay(datab[(16*21)-1:16*20]), .bx(dataa[(16*22)-1:16*21]), .by(datab[(16*22)-1:16*21]), .chainin(chain_atom06_to_atom05), .result(res05), .chainout(chain_atom05_to_atom04));
dsp_inst d04(.clk(clk), .reset(rst), .ax(dataa[(16*23)-1:16*22]), .ay(datab[(16*23)-1:16*22]), .bx(dataa[(16*24)-1:16*23]), .by(datab[(16*24)-1:16*23]), .chainin(chain_atom05_to_atom04), .result(res04), .chainout(chain_atom04_to_atom03));
dsp_inst d03(.clk(clk), .reset(rst), .ax(dataa[(16*25)-1:16*24]), .ay(datab[(16*25)-1:16*24]), .bx(dataa[(16*26)-1:16*25]), .by(datab[(16*26)-1:16*25]), .chainin(chain_atom04_to_atom03), .result(res03), .chainout(chain_atom03_to_atom02));
dsp_inst d02(.clk(clk), .reset(rst), .ax(dataa[(16*27)-1:16*26]), .ay(datab[(16*27)-1:16*26]), .bx(dataa[(16*28)-1:16*27]), .by(datab[(16*28)-1:16*27]), .chainin(chain_atom03_to_atom02), .result(res02), .chainout(chain_atom02_to_atom01));
dsp_inst d01(.clk(clk), .reset(rst), .ax(dataa[(16*29)-1:16*28]), .ay(datab[(16*29)-1:16*28]), .bx(dataa[(16*30)-1:16*29]), .by(datab[(16*30)-1:16*29]), .chainin(chain_atom02_to_atom01), .result(res01), .chainout(chain_atom01_to_atom00));
dsp_inst d00(.clk(clk), .reset(rst), .ax(dataa[(16*31)-1:16*30]), .ay(datab[(16*31)-1:16*30]), .bx(dataa[(16*32)-1:16*31]), .by(datab[(16*32)-1:16*31]), .chainin(chain_atom01_to_atom00), .result(res00), .chainout(           dummy_chain));

reg [31:0] r_result;
reg r_ovalid;

always @ (posedge clk) begin
	if (rst) begin
		r_result <= 0;
		r_ovalid <= 1'b0;
	end else begin
		if (delay_datac[33]) begin
			if (delay_datac[32] && delay_datad[32]) begin
				r_result <= res00 + delay_datac[31:0] + delay_datad[31:0];
			end else if (delay_datac[32] && !delay_datad[32]) begin
				r_result <= res00 + delay_datac[31:0];
			end else if (!delay_datac[32] && delay_datad[32]) begin
				r_result <= res00 + delay_datad[31:0];
			end else begin
				r_result <= res00;
			end
		end
		r_ovalid <= delay_datac[33] && delay_datad[33];
	end
end

assign result = r_result;
assign ovalid = r_ovalid;

endmodule

module memory_block (
	clk,
	waddr,
	wen,
	wdata,
	raddr,
	rdata
);

input  wire         clk;
input  wire [  8:0] waddr;
input  wire         wen;
input  wire [511:0] wdata;
input  wire [  8:0] raddr;
output wire [511:0] rdata;

bram_inst b0  (.clk(clk), .wen(wen), .wdata(wdata[(32*1)-1:32*0]),   .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*1)-1:32*0]));
bram_inst b1  (.clk(clk), .wen(wen), .wdata(wdata[(32*2)-1:32*1]),   .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*2)-1:32*1]));
bram_inst b2  (.clk(clk), .wen(wen), .wdata(wdata[(32*3)-1:32*2]),   .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*3)-1:32*2]));
bram_inst b3  (.clk(clk), .wen(wen), .wdata(wdata[(32*4)-1:32*3]),   .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*4)-1:32*3]));
bram_inst b4  (.clk(clk), .wen(wen), .wdata(wdata[(32*5)-1:32*4]),   .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*5)-1:32*4]));
bram_inst b5  (.clk(clk), .wen(wen), .wdata(wdata[(32*6)-1:32*5]),   .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*6)-1:32*5]));
bram_inst b6  (.clk(clk), .wen(wen), .wdata(wdata[(32*7)-1:32*6]),   .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*7)-1:32*6]));
bram_inst b7  (.clk(clk), .wen(wen), .wdata(wdata[(32*8)-1:32*7]),   .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*8)-1:32*7]));
bram_inst b8  (.clk(clk), .wen(wen), .wdata(wdata[(32*9)-1:32*8]),   .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*9)-1:32*8]));
bram_inst b9  (.clk(clk), .wen(wen), .wdata(wdata[(32*10)-1:32*9]),  .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*10)-1:32*9]));
bram_inst b10 (.clk(clk), .wen(wen), .wdata(wdata[(32*11)-1:32*10]), .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*11)-1:32*10]));
bram_inst b11 (.clk(clk), .wen(wen), .wdata(wdata[(32*12)-1:32*11]), .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*12)-1:32*11]));
bram_inst b12 (.clk(clk), .wen(wen), .wdata(wdata[(32*13)-1:32*12]), .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*13)-1:32*12]));
bram_inst b13 (.clk(clk), .wen(wen), .wdata(wdata[(32*14)-1:32*13]), .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*14)-1:32*13]));
bram_inst b14 (.clk(clk), .wen(wen), .wdata(wdata[(32*15)-1:32*14]), .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*15)-1:32*14]));
bram_inst b15 (.clk(clk), .wen(wen), .wdata(wdata[(32*16)-1:32*15]), .waddr(waddr), .raddr(raddr), .rdata(rdata[(32*16)-1:32*15]));

endmodule

module fifo_mvm (
  clk,
  rst,
  push,
  idata,
  pop,
  odata,
  empty,
  full
);

input  wire clk;
input  wire rst;
input  wire push;
input  wire [511:0] idata;
input  wire pop;
output wire [511:0] odata;
output reg empty;
output reg full;

reg [8:0] head_ptr;
reg [8:0] tail_ptr;

bram_inst b0  (.clk(clk), .wen(push), .wdata(idata[(32*1)-1:32*0]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*1)-1:32*0]));
bram_inst b1  (.clk(clk), .wen(push), .wdata(idata[(32*2)-1:32*1]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*2)-1:32*1]));
bram_inst b2  (.clk(clk), .wen(push), .wdata(idata[(32*3)-1:32*2]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*3)-1:32*2]));
bram_inst b3  (.clk(clk), .wen(push), .wdata(idata[(32*4)-1:32*3]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*4)-1:32*3]));
bram_inst b4  (.clk(clk), .wen(push), .wdata(idata[(32*5)-1:32*4]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*5)-1:32*4]));
bram_inst b5  (.clk(clk), .wen(push), .wdata(idata[(32*6)-1:32*5]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*6)-1:32*5]));
bram_inst b6  (.clk(clk), .wen(push), .wdata(idata[(32*7)-1:32*6]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*7)-1:32*6]));
bram_inst b7  (.clk(clk), .wen(push), .wdata(idata[(32*8)-1:32*7]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*8)-1:32*7]));
bram_inst b8  (.clk(clk), .wen(push), .wdata(idata[(32*9)-1:32*8]),   .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*9)-1:32*8]));
bram_inst b9  (.clk(clk), .wen(push), .wdata(idata[(32*10)-1:32*9]),  .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*10)-1:32*9]));
bram_inst b10 (.clk(clk), .wen(push), .wdata(idata[(32*11)-1:32*10]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*11)-1:32*10]));
bram_inst b11 (.clk(clk), .wen(push), .wdata(idata[(32*12)-1:32*11]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*12)-1:32*11]));
bram_inst b12 (.clk(clk), .wen(push), .wdata(idata[(32*13)-1:32*12]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*13)-1:32*12]));
bram_inst b13 (.clk(clk), .wen(push), .wdata(idata[(32*14)-1:32*13]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*14)-1:32*13]));
bram_inst b14 (.clk(clk), .wen(push), .wdata(idata[(32*15)-1:32*14]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*15)-1:32*14]));
bram_inst b15 (.clk(clk), .wen(push), .wdata(idata[(32*16)-1:32*15]), .waddr(tail_ptr), .raddr(head_ptr), .rdata(odata[(32*16)-1:32*15]));

always @ (posedge clk) begin
	if (rst) begin
		head_ptr <= 0;
		tail_ptr <= 0;
	end else begin
		if (push) tail_ptr <= tail_ptr + 1;		
		if (pop)  head_ptr <= head_ptr + 1;
	end
end

always @ (*) begin
  if (tail_ptr == head_ptr) begin
    empty = 1'b1;
  end else begin
    empty = 1'b0;
  end

  if (tail_ptr+1 == head_ptr) begin
    full = 1'b1;
  end else begin
    full = 1'b0;
  end
end

endmodule


module bram_inst(
	clk,
	wen,
	wdata,
	waddr,
	raddr,
	rdata
);

input  wire        clk;
input  wire        wen;
input  wire [31:0] wdata;
input  wire [ 8:0] waddr;
input  wire [ 8:0] raddr;
output wire [31:0] rdata;

wire [39:0] rtemp;
wire [39:0] wtemp;
wire [8:0] addrtemp;
assign rdata = rtemp[31:0];
assign wtemp = {8'd0, wdata};
assign addrtemp = waddr | raddr;

single_port_ram bram_instance(
	.clk(clk),
	.we(wen),
	.data(wtemp),
	.addr(addrtemp),
	.out(rtemp)
);

endmodule


module dpe_pipeline (
	clk,
	rst,
	data_in,
	data_out
);

input  wire        clk;
input  wire        rst;
input  wire [33:0] data_in;
output wire [33:0] data_out;

reg [33:0] r_pipeline_00; 
reg [33:0] r_pipeline_01;
reg [33:0] r_pipeline_02;
reg [33:0] r_pipeline_03;  
reg [33:0] r_pipeline_04; 
reg [33:0] r_pipeline_05;
reg [33:0] r_pipeline_06;
reg [33:0] r_pipeline_07;  
reg [33:0] r_pipeline_08; 
reg [33:0] r_pipeline_09;
reg [33:0] r_pipeline_10;
reg [33:0] r_pipeline_11;  
reg [33:0] r_pipeline_12; 
reg [33:0] r_pipeline_13;
reg [33:0] r_pipeline_14;
reg [33:0] r_pipeline_15; 
reg [33:0] r_pipeline_16;
reg [33:0] r_pipeline_17;
reg [33:0] r_pipeline_18;  
reg [33:0] r_pipeline_19; 
reg [33:0] r_pipeline_20;
reg [33:0] r_pipeline_21;
reg [33:0] r_pipeline_22;  
reg [33:0] r_pipeline_23; 
reg [33:0] r_pipeline_24;
reg [33:0] r_pipeline_25;
reg [33:0] r_pipeline_26;  
reg [33:0] r_pipeline_27; 
reg [33:0] r_pipeline_28;
reg [33:0] r_pipeline_29;
reg [33:0] r_pipeline_30;
reg [33:0] r_pipeline_31;

always @ (posedge clk) begin
	if (rst) begin
		r_pipeline_00 <= 0;
		r_pipeline_01 <= 0;
		r_pipeline_02 <= 0;
		r_pipeline_03 <= 0;
		r_pipeline_04 <= 0;
		r_pipeline_05 <= 0;
		r_pipeline_06 <= 0;
		r_pipeline_07 <= 0;
		r_pipeline_08 <= 0;
		r_pipeline_09 <= 0;
		r_pipeline_10 <= 0;
		r_pipeline_11 <= 0;
		r_pipeline_12 <= 0;
		r_pipeline_13 <= 0;
		r_pipeline_14 <= 0;
		r_pipeline_15 <= 0;
		r_pipeline_16 <= 0;
		r_pipeline_17 <= 0;
		r_pipeline_18 <= 0;
		r_pipeline_19 <= 0;
		r_pipeline_20 <= 0;
		r_pipeline_21 <= 0;
		r_pipeline_22 <= 0;
		r_pipeline_23 <= 0;
		r_pipeline_24 <= 0;
		r_pipeline_25 <= 0;
		r_pipeline_26 <= 0;
		r_pipeline_27 <= 0;
		r_pipeline_28 <= 0;
		r_pipeline_29 <= 0;
		r_pipeline_30 <= 0;
		r_pipeline_31 <= 0;
	end else begin
		r_pipeline_00 <= r_pipeline_01;
		r_pipeline_01 <= r_pipeline_02;
		r_pipeline_02 <= r_pipeline_03;
		r_pipeline_03 <= r_pipeline_04;
		r_pipeline_04 <= r_pipeline_05;
		r_pipeline_05 <= r_pipeline_06;
		r_pipeline_06 <= r_pipeline_07;
		r_pipeline_07 <= r_pipeline_08;
		r_pipeline_08 <= r_pipeline_09;
		r_pipeline_09 <= r_pipeline_10;
		r_pipeline_10 <= r_pipeline_11;
		r_pipeline_11 <= r_pipeline_12;
		r_pipeline_12 <= r_pipeline_13;
		r_pipeline_13 <= r_pipeline_14;
		r_pipeline_14 <= r_pipeline_15;
		r_pipeline_15 <= r_pipeline_16;
		r_pipeline_16 <= r_pipeline_17;
		r_pipeline_17 <= r_pipeline_18;
		r_pipeline_18 <= r_pipeline_19;
		r_pipeline_19 <= r_pipeline_20;
		r_pipeline_20 <= r_pipeline_21;
		r_pipeline_21 <= r_pipeline_22;
		r_pipeline_22 <= r_pipeline_23;
		r_pipeline_23 <= r_pipeline_24;
		r_pipeline_24 <= r_pipeline_25;
		r_pipeline_25 <= r_pipeline_26;
		r_pipeline_26 <= r_pipeline_27;
		r_pipeline_27 <= r_pipeline_28;
		r_pipeline_28 <= r_pipeline_29;
		r_pipeline_29 <= r_pipeline_30;
		r_pipeline_30 <= r_pipeline_31;
		r_pipeline_31 <= data_in;
	end 
end

assign data_out = r_pipeline_00;

endmodule


module dsp_inst(
	clk,
	reset,
	ax,
	ay,
	bx,
	by,
	chainin,
	result,
	chainout
);

input  wire        clk;
input  wire        reset;
input  wire [15:0] ax;
input  wire [15:0] ay;
input  wire [15:0] bx;
input  wire [15:0] by;
input  wire [36:0] chainin;
output wire [31:0] result;
output wire [36:0] chainout;

wire [18:0] tmp_ax;
wire [19:0] tmp_ay;
wire [18:0] tmp_bx;
wire [19:0] tmp_by;
wire [36:0] tmp_result;

assign tmp_ax = {2'b0, ax};
assign tmp_ay = {3'b0, ay};
assign tmp_bx = {2'b0, bx};
assign tmp_by = {3'b0, by};
assign result = tmp_result[31:0];

int_sop_2 dsp_instance(
	.clk(clk),
	.reset(reset),
	.ax(tmp_ax),
	.ay(tmp_ay),
	.bx(tmp_bx),
	.by(tmp_by),
	.chainin(chainin),
	.result(tmp_result),
	.chainout(chainout)
);

endmodule

module noc_router_adapter_block_inst(
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

parameter noc_dw = 32; //NoC Data Width
parameter byte_dw = 8; 

/*control signal*/
input wire clk;
input wire reset;

/*Master*/
input wire master_tready;
output reg master_tvalid;
output reg [noc_dw - 1 : 0] master_tdata;
output reg [noc_dw / 8 - 1 : 0] master_tstrb;
output reg [noc_dw / 8 - 1 : 0] master_tkeep;
output reg [byte_dw - 1 : 0] master_tid;
output reg [byte_dw - 1 : 0] master_tdest;
output reg [byte_dw - 1 : 0] master_tuser;
output reg master_tlast;

/*Slave*/
input wire slave_tvalid;
input wire [noc_dw - 1 : 0] slave_tdata;
input wire [noc_dw / 8 - 1 : 0] slave_tstrb;
input wire [noc_dw / 8 - 1 : 0] slave_tkeep;
input wire [byte_dw - 1 : 0] slave_tid;
input wire [byte_dw - 1 : 0] slave_tdest;
input wire [byte_dw - 1 : 0] slave_tuser; 
input wire slave_tlast;
output reg slave_tready;

noc_router_adapter_block noc_instance(
	.clk(clk),
    .reset(reset),
    .master_tready(master_tready),
    .master_tdata(master_tdata),
	.master_tvalid(master_tvalid),
    .master_tstrb(master_tstrb),
    .master_tkeep(master_tkeep),
    .master_tid(master_tid),
    .master_tdest(master_tdest),
    .master_tuser(master_tuser),
    .master_tlast(master_tlast),
    .slave_tvalid(slave_tvalid),
    .slave_tready(slave_tready), 
    .slave_tdata(slave_tdata),
    .slave_tstrb(slave_tstrb),
    .slave_tkeep(slave_tkeep),
    .slave_tid(slave_tid),
    .slave_tdest(slave_tdest),
    .slave_tuser(slave_tuser),
    .slave_tlast(slave_tlast)
);


endmodule 