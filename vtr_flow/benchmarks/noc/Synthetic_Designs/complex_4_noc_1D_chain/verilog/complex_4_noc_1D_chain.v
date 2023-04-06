/*
    Top level modules to instantiate an AXI handshake between 4 routers.
    Each routers receives 32-bit data, processes it, pass it to the next routers in a 2D chain.
    For now, all of our routers traffic processor's module does the same calculation, 
    but in a more complicated design, we can add different logic to each router's traffic 
    processor module.
*/

module complex_4_noc_1D_chain (
    clk,
    reset,
	data_out
);

parameter noc_dw = 32; //NoC Data Width
parameter byte_dw = 8; 
parameter routers_num = 8;

/*****************INPUT/OUTPUT Definition********************/
input wire clk;
input wire reset;

output wire [noc_dw - 1 : 0] data_out;

/*******************Internal Variables**********************/
//traffic generator
wire [noc_dw - 1 : 0] tg_data;
wire tg_valid;

//First master and slave interface
wire [noc_dw -1 : 0] mi_1_data;
wire mi_1_valid;
wire mi_1_ready;

//Last router slave interface and traffic processor
wire si_last_ready;
wire [noc_dw -1 : 0] si_last_data_in;
wire si_last_valid_in;
wire [noc_dw -1 : 0] si_last_data_out;
wire si_last_valid_out;

//Second through routers_num-2 master and slave interface, and traffic processor
//slave interface data - middle routers
wire [noc_dw - 1: 0] si_data_in_2;
wire si_valid_in_2;
wire si_ready_2;

wire [noc_dw - 1: 0] si_data_out_2;
wire si_valid_out_2;

wire [noc_dw - 1: 0] si_data_in_3;
wire si_valid_in_3;
wire si_ready_3;

wire [noc_dw - 1: 0] si_data_out_3;
wire si_valid_out_3;

//traffic processor data - middle routers
wire [noc_dw - 1: 0] tp_data_out_1;
wire tp_valid_out_2;
wire [noc_dw - 1: 0] tp_data_out_2;
wire tp_valid_out_3;

//master interface data - middle routers
wire [noc_dw - 1: 0] mi_data_2;
wire mi_valid_2;
wire mi_ready_2;
wire [noc_dw - 1: 0] mi_data_3;
wire mi_valid_3;
wire mi_ready_3;

/*******************module instantiation********************/

/*
    **********************FIRST NOC ADAPTER*****************
    1) Traffic generator generates and passes data to master_interface
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

noc_router_adapter_block noc_router_adapter_block_1(
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
    **********************Middle NOC ADAPTERS*****************
    1) Data comes through NoC to the NoC adapter
    2) NoC adapter passes data to slave interface
    3) slave_interface passes data to traffic processor
    4) traffic processor passes the processed data to master interface
    5) master interface passes the data back to the next NoC adapter
*/
noc_router_adapter_block noc_router_adapter_block_2 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_2),
             .master_tdata(si_data_in_2),
             .master_tvalid(si_valid_in_2),
             .master_tstrb(),
             .master_tkeep(),
             .master_tid(),
             .master_tdest(),
             .master_tuser(),
             .master_tlast(),
             .slave_tvalid(mi_valid_2),
             .slave_tready(mi_ready_2), 
             .slave_tdata(mi_data_2),
             .slave_tstrb(8'd0),
             .slave_tkeep(8'd0),
             .slave_tid(8'd0),
             .slave_tdest(8'd0),
             .slave_tuser(8'd0),
             .slave_tlast(1'd0)
         );
slave_interface si_2(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_2),
             .tdata_in(si_data_in_2),
             .tready(si_ready_2),
             .tdata_out(si_data_out_2),
             .tvalid_out(si_valid_out_2),
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
	        .tdata_in(si_data_out_2),
	        .tvalid_in(si_valid_out_2),
	        .tdata_out(tp_data_out_2),
	        .tvalid_out(tp_valid_out_2)
        );
master_interface mi_2(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_2),
            .tdata_in(tp_data_out_2),
            .tready(mi_ready_2),
            .tdata_out(mi_data_2),
            .tvalid_out(mi_valid_2),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
        );
noc_router_adapter_block noc_router_adapter_block_3 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_3),
             .master_tdata(si_data_in_3),
             .master_tvalid(si_valid_in_3),
             .master_tstrb(),
             .master_tkeep(),
             .master_tid(),
             .master_tdest(),
             .master_tuser(),
             .master_tlast(),
             .slave_tvalid(mi_valid_3),
             .slave_tready(mi_ready_3), 
             .slave_tdata(mi_data_3),
             .slave_tstrb(8'd0),
             .slave_tkeep(8'd0),
             .slave_tid(8'd0),
             .slave_tdest(8'd0),
             .slave_tuser(8'd0),
             .slave_tlast(1'd0)
         );
slave_interface si_3(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_3),
             .tdata_in(si_data_in_3),
             .tready(si_ready_3),
             .tdata_out(si_data_out_3),
             .tvalid_out(si_valid_out_3),
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
	        .tdata_in(si_data_out_3),
	        .tvalid_in(si_valid_out_3),
	        .tdata_out(tp_data_out_3),
	        .tvalid_out(tp_valid_out_3)
        );
master_interface mi_3(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_3),
            .tdata_in(tp_data_out_3),
            .tready(mi_ready_3),
            .tdata_out(mi_data_3),
            .tvalid_out(mi_valid_3),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
        );

/*
    **********************Last NOC ADAPTER*****************
    1) Data comes through NoC to the ROUTERS_NUM - 1 NoC adapter
    2) NoC adapter passes data to slave interface
    3) slave_interface passes data to traffic processor
    4) traffic processor passes the processed data to the top module output
*/

noc_router_adapter_block noc_router_adapter_block_4(
	.clk(clk),
    .reset(reset),
    .master_tready(si_last_ready),
    .master_tdata(si_last_data_in),
	.master_tvalid(si_last_valid_in),
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
	.tvalid_in(si_last_valid_in),
	.tdata_in(si_last_data_in),
	.tready(si_last_ready),
	.tdata_out(si_last_data_out),
	.tvalid_out(si_last_valid_out),
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
	.tdata_in(si_last_data_out),
	.tvalid_in(si_last_valid_out),
	.tdata_out(data_out),
	.tvalid_out()
);

endmodule