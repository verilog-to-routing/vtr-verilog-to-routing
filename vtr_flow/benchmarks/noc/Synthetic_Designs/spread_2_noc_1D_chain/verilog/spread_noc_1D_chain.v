/*
    Top level modules to instantiate an AXI handshake between two routers.
    The design implements the following:
        1) Traffic generator uses a FIR filter to generate data and pass it to master_interface.
        2) Master interface sends data and valid signal that coming from traffic generator to the NoC.
        3) Slave interface receives data and valid signal, passes the ready signal through NoC and data to the traffic processor.
        4) Traffic Processor encrypts the data. 
        5) An adder tree and a multiplier is also added to top module to create some extra logic that are not connected to any of the routers.
        6) Final output is generated using both NoC data and extra logic output.
*/

module spread_noc_1D_chain (
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

//traffic generator
wire tg_valid;
wire [noc_dw - 1 : 0] tg_data;

//multiplier block
wire [arithmetic_dw -1 : 0] mult_res;

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

//traffic processor 
wire [noc_dw - 1 : 0] tp_data;

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

//NoC connectivity
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

noc_router_adapter_block noc_router_adapter_block_two(
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
	.o_enc(tp_data)
);

/*******************Output Logic***************************/
assign data_out = tp_data & mult_res;

endmodule