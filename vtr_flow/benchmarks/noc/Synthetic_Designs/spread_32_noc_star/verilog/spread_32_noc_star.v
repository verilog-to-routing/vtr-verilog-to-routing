/*
    Top level modules to instantiate an AXI handshake between 32 routers.
    The design implements the following:
        1) Traffic generator uses a FIR filter to generate data and pass it to master_interface.
        2) Master interface sends data and valid signal that coming from traffic generator to the NoC.
        3) Slave interface receives data and valid signal, passes the ready signal through NoC and data to the traffic processor.
        4) Traffic Processor encrypts the data. 
        5) An adder tree and a multiplier is also added to top module to create some extra logic that are not connected to any of the routers.
        6) Final output is generated using both NoC data and extra logic output.
*/

module spread_32_noc_star (
    clk,
    reset,
    data_in,
    data_out
);

/*****************Constant/Parameter Definition***************/
`define bitwise_const_0 32'h67452301
`define bitwise_const_1 32'hefcdab89
`define bitwise_const_2 32'h98badcfe
`define bitwise_const_3 32'h10325476
`define bitwise_const_4 32'hc3d2e1f0
`define bitwise_const_5 32'h5a827999
`define bitwise_const_6 32'h6ed9eba1
`define bitwise_const_7 32'h8f1bbcdc

parameter arithmetic_dw = 32;
parameter noc_dw = 32; //NoC Data Width
parameter byte_dw = 8;
parameter acc_const = 4;

/*****************INPUT/OUTPUT Definition********************/
input wire clk;
input wire reset;

input wire [noc_dw - 1 : 0] data_in;

output wire [noc_dw - 1:0] data_out;

/*******************Internal Variables**********************/
//adder_tree 
wire [arithmetic_dw - 1 : 0] adder_tree_sum;
reg [arithmetic_dw - 1 : 0] adder_tree_input [byte_dw-1:0];
reg [arithmetic_dw - 1 : 0] counter;

//multiplier block
wire [arithmetic_dw -1 : 0] mult_res;

//traffic generator
wire tg_valid;
wire [noc_dw - 1 : 0] tg_data;

//master interface
wire [noc_dw -1 : 0] mi_1_data;
wire mi_1_valid;
wire mi_1_ready;

//Second to last routers
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

wire [noc_dw - 1: 0] si_data_in_4;
wire si_valid_in_4;
wire si_ready_4;

wire [noc_dw - 1: 0] si_data_out_4;
wire si_valid_out_4;

wire [noc_dw - 1: 0] si_data_in_5;
wire si_valid_in_5;
wire si_ready_5;

wire [noc_dw - 1: 0] si_data_out_5;
wire si_valid_out_5;

wire [noc_dw - 1: 0] si_data_in_6;
wire si_valid_in_6;
wire si_ready_6;

wire [noc_dw - 1: 0] si_data_out_6;
wire si_valid_out_6;

wire [noc_dw - 1: 0] si_data_in_7;
wire si_valid_in_7;
wire si_ready_7;

wire [noc_dw - 1: 0] si_data_out_7;
wire si_valid_out_7;

wire [noc_dw - 1: 0] si_data_in_8;
wire si_valid_in_8;
wire si_ready_8;

wire [noc_dw - 1: 0] si_data_out_8;
wire si_valid_out_8;

wire [noc_dw - 1: 0] si_data_in_9;
wire si_valid_in_9;
wire si_ready_9;

wire [noc_dw - 1: 0] si_data_out_9;
wire si_valid_out_9;

wire [noc_dw - 1: 0] si_data_in_10;
wire si_valid_in_10;
wire si_ready_10;

wire [noc_dw - 1: 0] si_data_out_10;
wire si_valid_out_10;

wire [noc_dw - 1: 0] si_data_in_11;
wire si_valid_in_11;
wire si_ready_11;

wire [noc_dw - 1: 0] si_data_out_11;
wire si_valid_out_11;

wire [noc_dw - 1: 0] si_data_in_12;
wire si_valid_in_12;
wire si_ready_12;

wire [noc_dw - 1: 0] si_data_out_12;
wire si_valid_out_12;

wire [noc_dw - 1: 0] si_data_in_13;
wire si_valid_in_13;
wire si_ready_13;

wire [noc_dw - 1: 0] si_data_out_13;
wire si_valid_out_13;

wire [noc_dw - 1: 0] si_data_in_14;
wire si_valid_in_14;
wire si_ready_14;

wire [noc_dw - 1: 0] si_data_out_14;
wire si_valid_out_14;

wire [noc_dw - 1: 0] si_data_in_15;
wire si_valid_in_15;
wire si_ready_15;

wire [noc_dw - 1: 0] si_data_out_15;
wire si_valid_out_15;

wire [noc_dw - 1: 0] si_data_in_16;
wire si_valid_in_16;
wire si_ready_16;

wire [noc_dw - 1: 0] si_data_out_16;
wire si_valid_out_16;

wire [noc_dw - 1: 0] si_data_in_17;
wire si_valid_in_17;
wire si_ready_17;

wire [noc_dw - 1: 0] si_data_out_17;
wire si_valid_out_17;

wire [noc_dw - 1: 0] si_data_in_18;
wire si_valid_in_18;
wire si_ready_18;

wire [noc_dw - 1: 0] si_data_out_18;
wire si_valid_out_18;

wire [noc_dw - 1: 0] si_data_in_19;
wire si_valid_in_19;
wire si_ready_19;

wire [noc_dw - 1: 0] si_data_out_19;
wire si_valid_out_19;

wire [noc_dw - 1: 0] si_data_in_20;
wire si_valid_in_20;
wire si_ready_20;

wire [noc_dw - 1: 0] si_data_out_20;
wire si_valid_out_20;

wire [noc_dw - 1: 0] si_data_in_21;
wire si_valid_in_21;
wire si_ready_21;

wire [noc_dw - 1: 0] si_data_out_21;
wire si_valid_out_21;

wire [noc_dw - 1: 0] si_data_in_22;
wire si_valid_in_22;
wire si_ready_22;

wire [noc_dw - 1: 0] si_data_out_22;
wire si_valid_out_22;

wire [noc_dw - 1: 0] si_data_in_23;
wire si_valid_in_23;
wire si_ready_23;

wire [noc_dw - 1: 0] si_data_out_23;
wire si_valid_out_23;

wire [noc_dw - 1: 0] si_data_in_24;
wire si_valid_in_24;
wire si_ready_24;

wire [noc_dw - 1: 0] si_data_out_24;
wire si_valid_out_24;

wire [noc_dw - 1: 0] si_data_in_25;
wire si_valid_in_25;
wire si_ready_25;

wire [noc_dw - 1: 0] si_data_out_25;
wire si_valid_out_25;

wire [noc_dw - 1: 0] si_data_in_26;
wire si_valid_in_26;
wire si_ready_26;

wire [noc_dw - 1: 0] si_data_out_26;
wire si_valid_out_26;

wire [noc_dw - 1: 0] si_data_in_27;
wire si_valid_in_27;
wire si_ready_27;

wire [noc_dw - 1: 0] si_data_out_27;
wire si_valid_out_27;

wire [noc_dw - 1: 0] si_data_in_28;
wire si_valid_in_28;
wire si_ready_28;

wire [noc_dw - 1: 0] si_data_out_28;
wire si_valid_out_28;

wire [noc_dw - 1: 0] si_data_in_29;
wire si_valid_in_29;
wire si_ready_29;

wire [noc_dw - 1: 0] si_data_out_29;
wire si_valid_out_29;

wire [noc_dw - 1: 0] si_data_in_30;
wire si_valid_in_30;
wire si_ready_30;

wire [noc_dw - 1: 0] si_data_out_30;
wire si_valid_out_30;

wire [noc_dw - 1: 0] si_data_in_31;
wire si_valid_in_31;
wire si_ready_31;

wire [noc_dw - 1: 0] si_data_out_31;
wire si_valid_out_31;

wire [noc_dw - 1: 0] si_data_in_32;
wire si_valid_in_32;
wire si_ready_32;

wire [noc_dw - 1: 0] si_data_out_32;
wire si_valid_out_32;

//traffic processor data - middle routers
wire [noc_dw - 1: 0] tp_data_out_2;
wire tp_valid_out_2;

wire [noc_dw - 1: 0] tp_data_out_3;
wire tp_valid_out_3;

wire [noc_dw - 1: 0] tp_data_out_4;
wire tp_valid_out_4;

wire [noc_dw - 1: 0] tp_data_out_5;
wire tp_valid_out_5;

wire [noc_dw - 1: 0] tp_data_out_6;
wire tp_valid_out_6;

wire [noc_dw - 1: 0] tp_data_out_7;
wire tp_valid_out_7;

wire [noc_dw - 1: 0] tp_data_out_8;
wire tp_valid_out_8;

wire [noc_dw - 1: 0] tp_data_out_9;
wire tp_valid_out_9;

wire [noc_dw - 1: 0] tp_data_out_10;
wire tp_valid_out_10;

wire [noc_dw - 1: 0] tp_data_out_11;
wire tp_valid_out_11;

wire [noc_dw - 1: 0] tp_data_out_12;
wire tp_valid_out_12;

wire [noc_dw - 1: 0] tp_data_out_13;
wire tp_valid_out_13;

wire [noc_dw - 1: 0] tp_data_out_14;
wire tp_valid_out_14;

wire [noc_dw - 1: 0] tp_data_out_15;
wire tp_valid_out_15;

wire [noc_dw - 1: 0] tp_data_out_16;
wire tp_valid_out_16;

wire [noc_dw - 1: 0] tp_data_out_17;
wire tp_valid_out_17;

wire [noc_dw - 1: 0] tp_data_out_18;
wire tp_valid_out_18;

wire [noc_dw - 1: 0] tp_data_out_19;
wire tp_valid_out_19;

wire [noc_dw - 1: 0] tp_data_out_20;
wire tp_valid_out_20;

wire [noc_dw - 1: 0] tp_data_out_21;
wire tp_valid_out_21;

wire [noc_dw - 1: 0] tp_data_out_22;
wire tp_valid_out_22;

wire [noc_dw - 1: 0] tp_data_out_23;
wire tp_valid_out_23;

wire [noc_dw - 1: 0] tp_data_out_24;
wire tp_valid_out_24;

wire [noc_dw - 1: 0] tp_data_out_25;
wire tp_valid_out_25;

wire [noc_dw - 1: 0] tp_data_out_26;
wire tp_valid_out_26;

wire [noc_dw - 1: 0] tp_data_out_27;
wire tp_valid_out_27;

wire [noc_dw - 1: 0] tp_data_out_28;
wire tp_valid_out_28;

wire [noc_dw - 1: 0] tp_data_out_29;
wire tp_valid_out_29;

wire [noc_dw - 1: 0] tp_data_out_30;
wire tp_valid_out_30;

wire [noc_dw - 1: 0] tp_data_out_31;
wire tp_valid_out_31;

wire [noc_dw - 1: 0] tp_data_out_32;
wire tp_valid_out_32;


/******************Sequential Logic*************************/
always @ (posedge clk) begin
    if(reset == 1'b1) begin
        adder_tree_input[0] <= 32'd0;
        adder_tree_input[1] <= 32'd0;
        adder_tree_input[2] <= 32'd0;
        adder_tree_input[3] <= 32'd0;
        adder_tree_input[4] <= 32'd0;
        adder_tree_input[5] <= 32'd0;
        adder_tree_input[6] <= 32'd0;
        adder_tree_input[7] <= 32'd0;
		  counter <= 32'd0;
    end
    else begin
        counter <= counter + 1;
        adder_tree_input[0] <= (counter) | `bitwise_const_0;
        adder_tree_input[1] <= (counter + 1) | `bitwise_const_1;
        adder_tree_input[2] <= (2 * counter + 2) | `bitwise_const_2;
        adder_tree_input[3] <= (3 * counter + 3) | `bitwise_const_3;
        adder_tree_input[4] <= (4 * counter + 4) | `bitwise_const_4;
        adder_tree_input[5] <= (5 * counter + 5) | `bitwise_const_5;
        adder_tree_input[6] <= (6 * counter + 6) | `bitwise_const_6;
        adder_tree_input[7] <= (7 * counter + 7) | `bitwise_const_7;
    end 
end 

/*******************module instantiation********************/

//Spare Logic that is not connected to any of our routers
adder_tree_top atp(
	.clk(clk),
	.isum0_0_0_0(adder_tree_input[0]),
    .isum0_0_0_1(adder_tree_input[1]),
    .isum0_0_1_0(adder_tree_input[2]),
    .isum0_0_1_1(adder_tree_input[3]),
    .isum0_1_0_0(adder_tree_input[4]),
    .isum0_1_0_1(adder_tree_input[5]),
    .isum0_1_1_0(adder_tree_input[6]),
    .isum0_1_1_1(adder_tree_input[7]),
	.sum(adder_tree_sum),
);

multiplier_block mb(
    .i_data0(adder_tree_sum),
    .o_data0(mult_res)
);

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
    .tdata_in(data_in),
    .tdata_out(tg_data),
    .tvalid_out(tg_valid)
);

master_interface mi (
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
    *******************ALL OTHER NOC ADAPTERS***************
    1) Data comes through NoC 
    2) NoC adapter passes data to slave interface
    3) slave_interface passes data to traffic processor
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
noc_router_adapter_block noc_router_adapter_block_4 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_4),
             .master_tdata(si_data_in_4),
             .master_tvalid(si_valid_in_4),
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
             .slave_tlast(1'd0)
         );
slave_interface si_4(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_4),
             .tdata_in(si_data_in_4),
             .tready(si_ready_4),
             .tdata_out(si_data_out_4),
             .tvalid_out(si_valid_out_4),
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
	        .tdata_in(si_data_out_4),
	        .tvalid_in(si_valid_out_4),
	        .tdata_out(tp_data_out_4),
	        .tvalid_out(tp_valid_out_4)
        );
noc_router_adapter_block noc_router_adapter_block_5 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_5),
             .master_tdata(si_data_in_5),
             .master_tvalid(si_valid_in_5),
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
             .slave_tlast(1'd0)
         );
slave_interface si_5(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_5),
             .tdata_in(si_data_in_5),
             .tready(si_ready_5),
             .tdata_out(si_data_out_5),
             .tvalid_out(si_valid_out_5),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_5(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_5),
	        .tvalid_in(si_valid_out_5),
	        .tdata_out(tp_data_out_5),
	        .tvalid_out(tp_valid_out_5)
        );
noc_router_adapter_block noc_router_adapter_block_6 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_6),
             .master_tdata(si_data_in_6),
             .master_tvalid(si_valid_in_6),
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
             .slave_tlast(1'd0)
         );
slave_interface si_6(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_6),
             .tdata_in(si_data_in_6),
             .tready(si_ready_6),
             .tdata_out(si_data_out_6),
             .tvalid_out(si_valid_out_6),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_6(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_6),
	        .tvalid_in(si_valid_out_6),
	        .tdata_out(tp_data_out_6),
	        .tvalid_out(tp_valid_out_6)
        );
noc_router_adapter_block noc_router_adapter_block_7 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_7),
             .master_tdata(si_data_in_7),
             .master_tvalid(si_valid_in_7),
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
             .slave_tlast(1'd0)
         );
slave_interface si_7(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_7),
             .tdata_in(si_data_in_7),
             .tready(si_ready_7),
             .tdata_out(si_data_out_7),
             .tvalid_out(si_valid_out_7),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_7(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_7),
	        .tvalid_in(si_valid_out_7),
	        .tdata_out(tp_data_out_7),
	        .tvalid_out(tp_valid_out_7)
        );
noc_router_adapter_block noc_router_adapter_block_8 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_8),
             .master_tdata(si_data_in_8),
             .master_tvalid(si_valid_in_8),
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
             .slave_tlast(1'd0)
         );
slave_interface si_8(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_8),
             .tdata_in(si_data_in_8),
             .tready(si_ready_8),
             .tdata_out(si_data_out_8),
             .tvalid_out(si_valid_out_8),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_8(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_8),
	        .tvalid_in(si_valid_out_8),
	        .tdata_out(tp_data_out_8),
	        .tvalid_out(tp_valid_out_8)
        );
noc_router_adapter_block noc_router_adapter_block_9 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_9),
             .master_tdata(si_data_in_9),
             .master_tvalid(si_valid_in_9),
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
             .slave_tlast(1'd0)
         );
slave_interface si_9(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_9),
             .tdata_in(si_data_in_9),
             .tready(si_ready_9),
             .tdata_out(si_data_out_9),
             .tvalid_out(si_valid_out_9),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_9(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_9),
	        .tvalid_in(si_valid_out_9),
	        .tdata_out(tp_data_out_9),
	        .tvalid_out(tp_valid_out_9)
        );
noc_router_adapter_block noc_router_adapter_block_10 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_10),
             .master_tdata(si_data_in_10),
             .master_tvalid(si_valid_in_10),
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
             .slave_tlast(1'd0)
         );
slave_interface si_10(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_10),
             .tdata_in(si_data_in_10),
             .tready(si_ready_10),
             .tdata_out(si_data_out_10),
             .tvalid_out(si_valid_out_10),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_10(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_10),
	        .tvalid_in(si_valid_out_10),
	        .tdata_out(tp_data_out_10),
	        .tvalid_out(tp_valid_out_10)
        );
noc_router_adapter_block noc_router_adapter_block_11 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_11),
             .master_tdata(si_data_in_11),
             .master_tvalid(si_valid_in_11),
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
             .slave_tlast(1'd0)
         );
slave_interface si_11(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_11),
             .tdata_in(si_data_in_11),
             .tready(si_ready_11),
             .tdata_out(si_data_out_11),
             .tvalid_out(si_valid_out_11),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_11(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_11),
	        .tvalid_in(si_valid_out_11),
	        .tdata_out(tp_data_out_11),
	        .tvalid_out(tp_valid_out_11)
        );
noc_router_adapter_block noc_router_adapter_block_12 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_12),
             .master_tdata(si_data_in_12),
             .master_tvalid(si_valid_in_12),
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
             .slave_tlast(1'd0)
         );
slave_interface si_12(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_12),
             .tdata_in(si_data_in_12),
             .tready(si_ready_12),
             .tdata_out(si_data_out_12),
             .tvalid_out(si_valid_out_12),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_12(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_12),
	        .tvalid_in(si_valid_out_12),
	        .tdata_out(tp_data_out_12),
	        .tvalid_out(tp_valid_out_12)
        );
noc_router_adapter_block noc_router_adapter_block_13 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_13),
             .master_tdata(si_data_in_13),
             .master_tvalid(si_valid_in_13),
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
             .slave_tlast(1'd0)
         );
slave_interface si_13(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_13),
             .tdata_in(si_data_in_13),
             .tready(si_ready_13),
             .tdata_out(si_data_out_13),
             .tvalid_out(si_valid_out_13),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_13(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_13),
	        .tvalid_in(si_valid_out_13),
	        .tdata_out(tp_data_out_13),
	        .tvalid_out(tp_valid_out_13)
        );
noc_router_adapter_block noc_router_adapter_block_14 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_14),
             .master_tdata(si_data_in_14),
             .master_tvalid(si_valid_in_14),
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
             .slave_tlast(1'd0)
         );
slave_interface si_14(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_14),
             .tdata_in(si_data_in_14),
             .tready(si_ready_14),
             .tdata_out(si_data_out_14),
             .tvalid_out(si_valid_out_14),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_14(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_14),
	        .tvalid_in(si_valid_out_14),
	        .tdata_out(tp_data_out_14),
	        .tvalid_out(tp_valid_out_14)
        );
noc_router_adapter_block noc_router_adapter_block_15 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_15),
             .master_tdata(si_data_in_15),
             .master_tvalid(si_valid_in_15),
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
             .slave_tlast(1'd0)
         );
slave_interface si_15(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_15),
             .tdata_in(si_data_in_15),
             .tready(si_ready_15),
             .tdata_out(si_data_out_15),
             .tvalid_out(si_valid_out_15),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_15(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_15),
	        .tvalid_in(si_valid_out_15),
	        .tdata_out(tp_data_out_15),
	        .tvalid_out(tp_valid_out_15)
        );
noc_router_adapter_block noc_router_adapter_block_16 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_16),
             .master_tdata(si_data_in_16),
             .master_tvalid(si_valid_in_16),
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
             .slave_tlast(1'd0)
         );
slave_interface si_16(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_16),
             .tdata_in(si_data_in_16),
             .tready(si_ready_16),
             .tdata_out(si_data_out_16),
             .tvalid_out(si_valid_out_16),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_16(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_16),
	        .tvalid_in(si_valid_out_16),
	        .tdata_out(tp_data_out_16),
	        .tvalid_out(tp_valid_out_16)
        );
noc_router_adapter_block noc_router_adapter_block_17 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_17),
             .master_tdata(si_data_in_17),
             .master_tvalid(si_valid_in_17),
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
             .slave_tlast(1'd0)
         );
slave_interface si_17(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_17),
             .tdata_in(si_data_in_17),
             .tready(si_ready_17),
             .tdata_out(si_data_out_17),
             .tvalid_out(si_valid_out_17),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_17(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_17),
	        .tvalid_in(si_valid_out_17),
	        .tdata_out(tp_data_out_17),
	        .tvalid_out(tp_valid_out_17)
        );
noc_router_adapter_block noc_router_adapter_block_18 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_18),
             .master_tdata(si_data_in_18),
             .master_tvalid(si_valid_in_18),
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
             .slave_tlast(1'd0)
         );
slave_interface si_18(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_18),
             .tdata_in(si_data_in_18),
             .tready(si_ready_18),
             .tdata_out(si_data_out_18),
             .tvalid_out(si_valid_out_18),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_18(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_18),
	        .tvalid_in(si_valid_out_18),
	        .tdata_out(tp_data_out_18),
	        .tvalid_out(tp_valid_out_18)
        );
noc_router_adapter_block noc_router_adapter_block_19 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_19),
             .master_tdata(si_data_in_19),
             .master_tvalid(si_valid_in_19),
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
             .slave_tlast(1'd0)
         );
slave_interface si_19(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_19),
             .tdata_in(si_data_in_19),
             .tready(si_ready_19),
             .tdata_out(si_data_out_19),
             .tvalid_out(si_valid_out_19),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_19(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_19),
	        .tvalid_in(si_valid_out_19),
	        .tdata_out(tp_data_out_19),
	        .tvalid_out(tp_valid_out_19)
        );
noc_router_adapter_block noc_router_adapter_block_20 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_20),
             .master_tdata(si_data_in_20),
             .master_tvalid(si_valid_in_20),
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
             .slave_tlast(1'd0)
         );
slave_interface si_20(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_20),
             .tdata_in(si_data_in_20),
             .tready(si_ready_20),
             .tdata_out(si_data_out_20),
             .tvalid_out(si_valid_out_20),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_20(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_20),
	        .tvalid_in(si_valid_out_20),
	        .tdata_out(tp_data_out_20),
	        .tvalid_out(tp_valid_out_20)
        );
noc_router_adapter_block noc_router_adapter_block_21 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_21),
             .master_tdata(si_data_in_21),
             .master_tvalid(si_valid_in_21),
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
             .slave_tlast(1'd0)
         );
slave_interface si_21(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_21),
             .tdata_in(si_data_in_21),
             .tready(si_ready_21),
             .tdata_out(si_data_out_21),
             .tvalid_out(si_valid_out_21),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_21(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_21),
	        .tvalid_in(si_valid_out_21),
	        .tdata_out(tp_data_out_21),
	        .tvalid_out(tp_valid_out_21)
        );
noc_router_adapter_block noc_router_adapter_block_22 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_22),
             .master_tdata(si_data_in_22),
             .master_tvalid(si_valid_in_22),
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
             .slave_tlast(1'd0)
         );
slave_interface si_22(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_22),
             .tdata_in(si_data_in_22),
             .tready(si_ready_22),
             .tdata_out(si_data_out_22),
             .tvalid_out(si_valid_out_22),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_22(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_22),
	        .tvalid_in(si_valid_out_22),
	        .tdata_out(tp_data_out_22),
	        .tvalid_out(tp_valid_out_22)
        );
noc_router_adapter_block noc_router_adapter_block_23 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_23),
             .master_tdata(si_data_in_23),
             .master_tvalid(si_valid_in_23),
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
             .slave_tlast(1'd0)
         );
slave_interface si_23(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_23),
             .tdata_in(si_data_in_23),
             .tready(si_ready_23),
             .tdata_out(si_data_out_23),
             .tvalid_out(si_valid_out_23),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_23(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_23),
	        .tvalid_in(si_valid_out_23),
	        .tdata_out(tp_data_out_23),
	        .tvalid_out(tp_valid_out_23)
        );
noc_router_adapter_block noc_router_adapter_block_24 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_24),
             .master_tdata(si_data_in_24),
             .master_tvalid(si_valid_in_24),
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
             .slave_tlast(1'd0)
         );
slave_interface si_24(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_24),
             .tdata_in(si_data_in_24),
             .tready(si_ready_24),
             .tdata_out(si_data_out_24),
             .tvalid_out(si_valid_out_24),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_24(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_24),
	        .tvalid_in(si_valid_out_24),
	        .tdata_out(tp_data_out_24),
	        .tvalid_out(tp_valid_out_24)
        );
noc_router_adapter_block noc_router_adapter_block_25 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_25),
             .master_tdata(si_data_in_25),
             .master_tvalid(si_valid_in_25),
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
             .slave_tlast(1'd0)
         );
slave_interface si_25(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_25),
             .tdata_in(si_data_in_25),
             .tready(si_ready_25),
             .tdata_out(si_data_out_25),
             .tvalid_out(si_valid_out_25),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_25(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_25),
	        .tvalid_in(si_valid_out_25),
	        .tdata_out(tp_data_out_25),
	        .tvalid_out(tp_valid_out_25)
        );
noc_router_adapter_block noc_router_adapter_block_26 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_26),
             .master_tdata(si_data_in_26),
             .master_tvalid(si_valid_in_26),
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
             .slave_tlast(1'd0)
         );
slave_interface si_26(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_26),
             .tdata_in(si_data_in_26),
             .tready(si_ready_26),
             .tdata_out(si_data_out_26),
             .tvalid_out(si_valid_out_26),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_26(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_26),
	        .tvalid_in(si_valid_out_26),
	        .tdata_out(tp_data_out_26),
	        .tvalid_out(tp_valid_out_26)
        );
noc_router_adapter_block noc_router_adapter_block_27 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_27),
             .master_tdata(si_data_in_27),
             .master_tvalid(si_valid_in_27),
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
             .slave_tlast(1'd0)
         );
slave_interface si_27(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_27),
             .tdata_in(si_data_in_27),
             .tready(si_ready_27),
             .tdata_out(si_data_out_27),
             .tvalid_out(si_valid_out_27),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_27(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_27),
	        .tvalid_in(si_valid_out_27),
	        .tdata_out(tp_data_out_27),
	        .tvalid_out(tp_valid_out_27)
        );
noc_router_adapter_block noc_router_adapter_block_28 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_28),
             .master_tdata(si_data_in_28),
             .master_tvalid(si_valid_in_28),
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
             .slave_tlast(1'd0)
         );
slave_interface si_28(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_28),
             .tdata_in(si_data_in_28),
             .tready(si_ready_28),
             .tdata_out(si_data_out_28),
             .tvalid_out(si_valid_out_28),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_28(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_28),
	        .tvalid_in(si_valid_out_28),
	        .tdata_out(tp_data_out_28),
	        .tvalid_out(tp_valid_out_28)
        );
noc_router_adapter_block noc_router_adapter_block_29 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_29),
             .master_tdata(si_data_in_29),
             .master_tvalid(si_valid_in_29),
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
             .slave_tlast(1'd0)
         );
slave_interface si_29(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_29),
             .tdata_in(si_data_in_29),
             .tready(si_ready_29),
             .tdata_out(si_data_out_29),
             .tvalid_out(si_valid_out_29),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_29(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_29),
	        .tvalid_in(si_valid_out_29),
	        .tdata_out(tp_data_out_29),
	        .tvalid_out(tp_valid_out_29)
        );
noc_router_adapter_block noc_router_adapter_block_30 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_30),
             .master_tdata(si_data_in_30),
             .master_tvalid(si_valid_in_30),
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
             .slave_tlast(1'd0)
         );
slave_interface si_30(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_30),
             .tdata_in(si_data_in_30),
             .tready(si_ready_30),
             .tdata_out(si_data_out_30),
             .tvalid_out(si_valid_out_30),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_30(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_30),
	        .tvalid_in(si_valid_out_30),
	        .tdata_out(tp_data_out_30),
	        .tvalid_out(tp_valid_out_30)
        );
noc_router_adapter_block noc_router_adapter_block_31 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_31),
             .master_tdata(si_data_in_31),
             .master_tvalid(si_valid_in_31),
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
             .slave_tlast(1'd0)
         );
slave_interface si_31(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_31),
             .tdata_in(si_data_in_31),
             .tready(si_ready_31),
             .tdata_out(si_data_out_31),
             .tvalid_out(si_valid_out_31),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_31(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_31),
	        .tvalid_in(si_valid_out_31),
	        .tdata_out(tp_data_out_31),
	        .tvalid_out(tp_valid_out_31)
        );
noc_router_adapter_block noc_router_adapter_block_32 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_32),
             .master_tdata(si_data_in_32),
             .master_tvalid(si_valid_in_32),
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
             .slave_tlast(1'd0)
         );
slave_interface si_32(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_32),
             .tdata_in(si_data_in_32),
             .tready(si_ready_32),
             .tdata_out(si_data_out_32),
             .tvalid_out(si_valid_out_32),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_32(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_32),
	        .tvalid_in(si_valid_out_32),
	        .tdata_out(tp_data_out_32),
	        .tvalid_out(tp_valid_out_32)
        );


/****************Avoid Synthesis Optimization***************/
/* Quartus synthesis tool removes traffic processor modules 
since their outputs are not connected to any module (considered
as dangling ports). To avoid this optimization, we registered the
traffic processor data output.
*/

//register output definition
wire [noc_dw - 1: 0] reg_data_out_2;
wire [noc_dw - 1: 0] reg_valid_out_2;

wire [noc_dw - 1: 0] reg_data_out_3;
wire [noc_dw - 1: 0] reg_valid_out_3;

wire [noc_dw - 1: 0] reg_data_out_4;
wire [noc_dw - 1: 0] reg_valid_out_4;

wire [noc_dw - 1: 0] reg_data_out_5;
wire [noc_dw - 1: 0] reg_valid_out_5;

wire [noc_dw - 1: 0] reg_data_out_6;
wire [noc_dw - 1: 0] reg_valid_out_6;

wire [noc_dw - 1: 0] reg_data_out_7;
wire [noc_dw - 1: 0] reg_valid_out_7;

wire [noc_dw - 1: 0] reg_data_out_8;
wire [noc_dw - 1: 0] reg_valid_out_8;

wire [noc_dw - 1: 0] reg_data_out_9;
wire [noc_dw - 1: 0] reg_valid_out_9;

wire [noc_dw - 1: 0] reg_data_out_10;
wire [noc_dw - 1: 0] reg_valid_out_10;

wire [noc_dw - 1: 0] reg_data_out_11;
wire [noc_dw - 1: 0] reg_valid_out_11;

wire [noc_dw - 1: 0] reg_data_out_12;
wire [noc_dw - 1: 0] reg_valid_out_12;

wire [noc_dw - 1: 0] reg_data_out_13;
wire [noc_dw - 1: 0] reg_valid_out_13;

wire [noc_dw - 1: 0] reg_data_out_14;
wire [noc_dw - 1: 0] reg_valid_out_14;

wire [noc_dw - 1: 0] reg_data_out_15;
wire [noc_dw - 1: 0] reg_valid_out_15;

wire [noc_dw - 1: 0] reg_data_out_16;
wire [noc_dw - 1: 0] reg_valid_out_16;

wire [noc_dw - 1: 0] reg_data_out_17;
wire [noc_dw - 1: 0] reg_valid_out_17;

wire [noc_dw - 1: 0] reg_data_out_18;
wire [noc_dw - 1: 0] reg_valid_out_18;

wire [noc_dw - 1: 0] reg_data_out_19;
wire [noc_dw - 1: 0] reg_valid_out_19;

wire [noc_dw - 1: 0] reg_data_out_20;
wire [noc_dw - 1: 0] reg_valid_out_20;

wire [noc_dw - 1: 0] reg_data_out_21;
wire [noc_dw - 1: 0] reg_valid_out_21;

wire [noc_dw - 1: 0] reg_data_out_22;
wire [noc_dw - 1: 0] reg_valid_out_22;

wire [noc_dw - 1: 0] reg_data_out_23;
wire [noc_dw - 1: 0] reg_valid_out_23;

wire [noc_dw - 1: 0] reg_data_out_24;
wire [noc_dw - 1: 0] reg_valid_out_24;

wire [noc_dw - 1: 0] reg_data_out_25;
wire [noc_dw - 1: 0] reg_valid_out_25;

wire [noc_dw - 1: 0] reg_data_out_26;
wire [noc_dw - 1: 0] reg_valid_out_26;

wire [noc_dw - 1: 0] reg_data_out_27;
wire [noc_dw - 1: 0] reg_valid_out_27;

wire [noc_dw - 1: 0] reg_data_out_28;
wire [noc_dw - 1: 0] reg_valid_out_28;

wire [noc_dw - 1: 0] reg_data_out_29;
wire [noc_dw - 1: 0] reg_valid_out_29;

wire [noc_dw - 1: 0] reg_data_out_30;
wire [noc_dw - 1: 0] reg_valid_out_30;

wire [noc_dw - 1: 0] reg_data_out_31;
wire [noc_dw - 1: 0] reg_valid_out_31;

register r_2 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_2),
             .data_out(reg_data_out_2)
         );
register r_3 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_3),
             .data_out(reg_data_out_3)
         );
register r_4 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_4),
             .data_out(reg_data_out_4)
         );
register r_5 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_5),
             .data_out(reg_data_out_5)
         );
register r_6 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_6),
             .data_out(reg_data_out_6)
         );
register r_7 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_7),
             .data_out(reg_data_out_7)
         );
register r_8 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_8),
             .data_out(reg_data_out_8)
         );
register r_9 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_9),
             .data_out(reg_data_out_9)
         );
register r_10 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_10),
             .data_out(reg_data_out_10)
         );
register r_11 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_11),
             .data_out(reg_data_out_11)
         );
register r_12 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_12),
             .data_out(reg_data_out_12)
         );
register r_13 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_13),
             .data_out(reg_data_out_13)
         );
register r_14 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_14),
             .data_out(reg_data_out_14)
         );
register r_15 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_15),
             .data_out(reg_data_out_15)
         );
register r_16 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_16),
             .data_out(reg_data_out_16)
         );
register r_17 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_17),
             .data_out(reg_data_out_17)
         );
register r_18 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_18),
             .data_out(reg_data_out_18)
         );
register r_19 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_19),
             .data_out(reg_data_out_19)
         );
register r_20 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_20),
             .data_out(reg_data_out_20)
         );
register r_21 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_21),
             .data_out(reg_data_out_21)
         );
register r_22 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_22),
             .data_out(reg_data_out_22)
         );
register r_23 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_23),
             .data_out(reg_data_out_23)
         );
register r_24 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_24),
             .data_out(reg_data_out_24)
         );
register r_25 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_25),
             .data_out(reg_data_out_25)
         );
register r_26 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_26),
             .data_out(reg_data_out_26)
         );
register r_27 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_27),
             .data_out(reg_data_out_27)
         );
register r_28 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_28),
             .data_out(reg_data_out_28)
         );
register r_29 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_29),
             .data_out(reg_data_out_29)
         );
register r_30 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_30),
             .data_out(reg_data_out_30)
         );
register r_31 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_31),
             .data_out(reg_data_out_31)
         );
register r_valid_2 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_2),
             .data_out(reg_Valid_out_2)
         );
register r_valid_3 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_3),
             .data_out(reg_Valid_out_3)
         );
register r_valid_4 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_4),
             .data_out(reg_Valid_out_4)
         );
register r_valid_5 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_5),
             .data_out(reg_Valid_out_5)
         );
register r_valid_6 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_6),
             .data_out(reg_Valid_out_6)
         );
register r_valid_7 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_7),
             .data_out(reg_Valid_out_7)
         );
register r_valid_8 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_8),
             .data_out(reg_Valid_out_8)
         );
register r_valid_9 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_9),
             .data_out(reg_Valid_out_9)
         );
register r_valid_10 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_10),
             .data_out(reg_Valid_out_10)
         );
register r_valid_11 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_11),
             .data_out(reg_Valid_out_11)
         );
register r_valid_12 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_12),
             .data_out(reg_Valid_out_12)
         );
register r_valid_13 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_13),
             .data_out(reg_Valid_out_13)
         );
register r_valid_14 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_14),
             .data_out(reg_Valid_out_14)
         );
register r_valid_15 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_15),
             .data_out(reg_Valid_out_15)
         );
register r_valid_16 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_16),
             .data_out(reg_Valid_out_16)
         );
register r_valid_17 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_17),
             .data_out(reg_Valid_out_17)
         );
register r_valid_18 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_18),
             .data_out(reg_Valid_out_18)
         );
register r_valid_19 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_19),
             .data_out(reg_Valid_out_19)
         );
register r_valid_20 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_20),
             .data_out(reg_Valid_out_20)
         );
register r_valid_21 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_21),
             .data_out(reg_Valid_out_21)
         );
register r_valid_22 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_22),
             .data_out(reg_Valid_out_22)
         );
register r_valid_23 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_23),
             .data_out(reg_Valid_out_23)
         );
register r_valid_24 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_24),
             .data_out(reg_Valid_out_24)
         );
register r_valid_25 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_25),
             .data_out(reg_Valid_out_25)
         );
register r_valid_26 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_26),
             .data_out(reg_Valid_out_26)
         );
register r_valid_27 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_27),
             .data_out(reg_Valid_out_27)
         );
register r_valid_28 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_28),
             .data_out(reg_Valid_out_28)
         );
register r_valid_29 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_29),
             .data_out(reg_Valid_out_29)
         );
register r_valid_30 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_30),
             .data_out(reg_Valid_out_30)
         );
register r_valid_31 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_31),
             .data_out(reg_Valid_out_31)
         );


/*******************Output Logic***************************/
assign data_out = tp_data_out_32 & mult_res;

endmodule