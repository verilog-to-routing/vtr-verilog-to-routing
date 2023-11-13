/*
    Top level modules to instantiate an AXI handshake between 64 routers.
    The first router generates data and pass it through all other routers (star connection),
    and each router can process data with the traffic processor module.
    For now, all of our routers traffic processor's module does the same calculation, 
    but in a more complicated design, we can add different logic to each router's traffic 
    processor module.
*/

module complex_64_noc_star (
    clk,
    reset,
	data_out
);

parameter noc_dw = 32; //NoC Data Width
parameter byte_dw = 8; 

/*****************INPUT/OUTPUT Definition********************/
input wire clk;
input wire reset;

output wire [noc_dw - 1:0] data_out;

/*******************Internal Variables**********************/
//traffic generator
wire [noc_dw - 1 : 0] tg_data;
wire tg_valid;

//First master and slave interface
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

wire [noc_dw - 1: 0] si_data_in_33;
wire si_valid_in_33;
wire si_ready_33;

wire [noc_dw - 1: 0] si_data_out_33;
wire si_valid_out_33;

wire [noc_dw - 1: 0] si_data_in_34;
wire si_valid_in_34;
wire si_ready_34;

wire [noc_dw - 1: 0] si_data_out_34;
wire si_valid_out_34;

wire [noc_dw - 1: 0] si_data_in_35;
wire si_valid_in_35;
wire si_ready_35;

wire [noc_dw - 1: 0] si_data_out_35;
wire si_valid_out_35;

wire [noc_dw - 1: 0] si_data_in_36;
wire si_valid_in_36;
wire si_ready_36;

wire [noc_dw - 1: 0] si_data_out_36;
wire si_valid_out_36;

wire [noc_dw - 1: 0] si_data_in_37;
wire si_valid_in_37;
wire si_ready_37;

wire [noc_dw - 1: 0] si_data_out_37;
wire si_valid_out_37;

wire [noc_dw - 1: 0] si_data_in_38;
wire si_valid_in_38;
wire si_ready_38;

wire [noc_dw - 1: 0] si_data_out_38;
wire si_valid_out_38;

wire [noc_dw - 1: 0] si_data_in_39;
wire si_valid_in_39;
wire si_ready_39;

wire [noc_dw - 1: 0] si_data_out_39;
wire si_valid_out_39;

wire [noc_dw - 1: 0] si_data_in_40;
wire si_valid_in_40;
wire si_ready_40;

wire [noc_dw - 1: 0] si_data_out_40;
wire si_valid_out_40;

wire [noc_dw - 1: 0] si_data_in_41;
wire si_valid_in_41;
wire si_ready_41;

wire [noc_dw - 1: 0] si_data_out_41;
wire si_valid_out_41;

wire [noc_dw - 1: 0] si_data_in_42;
wire si_valid_in_42;
wire si_ready_42;

wire [noc_dw - 1: 0] si_data_out_42;
wire si_valid_out_42;

wire [noc_dw - 1: 0] si_data_in_43;
wire si_valid_in_43;
wire si_ready_43;

wire [noc_dw - 1: 0] si_data_out_43;
wire si_valid_out_43;

wire [noc_dw - 1: 0] si_data_in_44;
wire si_valid_in_44;
wire si_ready_44;

wire [noc_dw - 1: 0] si_data_out_44;
wire si_valid_out_44;

wire [noc_dw - 1: 0] si_data_in_45;
wire si_valid_in_45;
wire si_ready_45;

wire [noc_dw - 1: 0] si_data_out_45;
wire si_valid_out_45;

wire [noc_dw - 1: 0] si_data_in_46;
wire si_valid_in_46;
wire si_ready_46;

wire [noc_dw - 1: 0] si_data_out_46;
wire si_valid_out_46;

wire [noc_dw - 1: 0] si_data_in_47;
wire si_valid_in_47;
wire si_ready_47;

wire [noc_dw - 1: 0] si_data_out_47;
wire si_valid_out_47;

wire [noc_dw - 1: 0] si_data_in_48;
wire si_valid_in_48;
wire si_ready_48;

wire [noc_dw - 1: 0] si_data_out_48;
wire si_valid_out_48;

wire [noc_dw - 1: 0] si_data_in_49;
wire si_valid_in_49;
wire si_ready_49;

wire [noc_dw - 1: 0] si_data_out_49;
wire si_valid_out_49;

wire [noc_dw - 1: 0] si_data_in_50;
wire si_valid_in_50;
wire si_ready_50;

wire [noc_dw - 1: 0] si_data_out_50;
wire si_valid_out_50;

wire [noc_dw - 1: 0] si_data_in_51;
wire si_valid_in_51;
wire si_ready_51;

wire [noc_dw - 1: 0] si_data_out_51;
wire si_valid_out_51;

wire [noc_dw - 1: 0] si_data_in_52;
wire si_valid_in_52;
wire si_ready_52;

wire [noc_dw - 1: 0] si_data_out_52;
wire si_valid_out_52;

wire [noc_dw - 1: 0] si_data_in_53;
wire si_valid_in_53;
wire si_ready_53;

wire [noc_dw - 1: 0] si_data_out_53;
wire si_valid_out_53;

wire [noc_dw - 1: 0] si_data_in_54;
wire si_valid_in_54;
wire si_ready_54;

wire [noc_dw - 1: 0] si_data_out_54;
wire si_valid_out_54;

wire [noc_dw - 1: 0] si_data_in_55;
wire si_valid_in_55;
wire si_ready_55;

wire [noc_dw - 1: 0] si_data_out_55;
wire si_valid_out_55;

wire [noc_dw - 1: 0] si_data_in_56;
wire si_valid_in_56;
wire si_ready_56;

wire [noc_dw - 1: 0] si_data_out_56;
wire si_valid_out_56;

wire [noc_dw - 1: 0] si_data_in_57;
wire si_valid_in_57;
wire si_ready_57;

wire [noc_dw - 1: 0] si_data_out_57;
wire si_valid_out_57;

wire [noc_dw - 1: 0] si_data_in_58;
wire si_valid_in_58;
wire si_ready_58;

wire [noc_dw - 1: 0] si_data_out_58;
wire si_valid_out_58;

wire [noc_dw - 1: 0] si_data_in_59;
wire si_valid_in_59;
wire si_ready_59;

wire [noc_dw - 1: 0] si_data_out_59;
wire si_valid_out_59;

wire [noc_dw - 1: 0] si_data_in_60;
wire si_valid_in_60;
wire si_ready_60;

wire [noc_dw - 1: 0] si_data_out_60;
wire si_valid_out_60;

wire [noc_dw - 1: 0] si_data_in_61;
wire si_valid_in_61;
wire si_ready_61;

wire [noc_dw - 1: 0] si_data_out_61;
wire si_valid_out_61;

wire [noc_dw - 1: 0] si_data_in_62;
wire si_valid_in_62;
wire si_ready_62;

wire [noc_dw - 1: 0] si_data_out_62;
wire si_valid_out_62;

wire [noc_dw - 1: 0] si_data_in_63;
wire si_valid_in_63;
wire si_ready_63;

wire [noc_dw - 1: 0] si_data_out_63;
wire si_valid_out_63;

wire [noc_dw - 1: 0] si_data_in_64;
wire si_valid_in_64;
wire si_ready_64;

wire [noc_dw - 1: 0] si_data_out_64;
wire si_valid_out_64;

//traffic processor data - middle routers
wire [noc_dw - 1: 0] tp_data_out_2 /* synthesis keep */;
wire tp_valid_out_2 /* synthesis keep */; 

wire [noc_dw - 1: 0] tp_data_out_3 /* synthesis keep */; 
wire tp_valid_out_3 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_4 /* synthesis keep */;
wire tp_valid_out_4 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_5 /* synthesis keep */;
wire tp_valid_out_5 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_6 /* synthesis keep */;
wire tp_valid_out_6 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_7 /* synthesis keep */;
wire tp_valid_out_7 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_8 /* synthesis keep */;
wire tp_valid_out_8 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_9 /* synthesis keep */;
wire tp_valid_out_9 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_10 /* synthesis keep */;
wire tp_valid_out_10 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_11 /* synthesis keep */;
wire tp_valid_out_11 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_12 /* synthesis keep */;
wire tp_valid_out_12 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_13 /* synthesis keep */;
wire tp_valid_out_13 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_14 /* synthesis keep */;
wire tp_valid_out_14 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_15 /* synthesis keep */;
wire tp_valid_out_15 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_16 /* synthesis keep */;
wire tp_valid_out_16 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_17 /* synthesis keep */;
wire tp_valid_out_17 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_18 /* synthesis keep */;
wire tp_valid_out_18 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_19 /* synthesis keep */;
wire tp_valid_out_19 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_20 /* synthesis keep */;
wire tp_valid_out_20 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_21 /* synthesis keep */;
wire tp_valid_out_21 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_22 /* synthesis keep */;
wire tp_valid_out_22 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_23 /* synthesis keep */;
wire tp_valid_out_23 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_24 /* synthesis keep */;
wire tp_valid_out_24 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_25 /* synthesis keep */;
wire tp_valid_out_25 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_26 /* synthesis keep */;
wire tp_valid_out_26 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_27 /* synthesis keep */;
wire tp_valid_out_27 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_28 /* synthesis keep */;
wire tp_valid_out_28 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_29 /* synthesis keep */;
wire tp_valid_out_29 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_30 /* synthesis keep */;
wire tp_valid_out_30 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_31 /* synthesis keep */;
wire tp_valid_out_31 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_32 /* synthesis keep */;
wire tp_valid_out_32 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_33 /* synthesis keep */;
wire tp_valid_out_33 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_34 /* synthesis keep */;
wire tp_valid_out_34 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_35 /* synthesis keep */;
wire tp_valid_out_35 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_36 /* synthesis keep */;
wire tp_valid_out_36 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_37 /* synthesis keep */;
wire tp_valid_out_37 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_38 /* synthesis keep */;
wire tp_valid_out_38 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_39 /* synthesis keep */;
wire tp_valid_out_39 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_40 /* synthesis keep */;
wire tp_valid_out_40 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_41 /* synthesis keep */; 
wire tp_valid_out_41 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_42 /* synthesis keep */;
wire tp_valid_out_42 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_43 /* synthesis keep */;
wire tp_valid_out_43 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_44 /* synthesis keep */;
wire tp_valid_out_44 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_45 /* synthesis keep */;
wire tp_valid_out_45 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_46 /* synthesis keep */;
wire tp_valid_out_46 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_47 /* synthesis keep */;
wire tp_valid_out_47 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_48 /* synthesis keep */;
wire tp_valid_out_48 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_49 /* synthesis keep */;
wire tp_valid_out_49 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_50 /* synthesis keep */;
wire tp_valid_out_50 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_51 /* synthesis keep */;
wire tp_valid_out_51 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_52 /* synthesis keep */;
wire tp_valid_out_52 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_53 /* synthesis keep */;
wire tp_valid_out_53 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_54 /* synthesis keep */;
wire tp_valid_out_54 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_55 /* synthesis keep */;
wire tp_valid_out_55 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_56 /* synthesis keep */;
wire tp_valid_out_56 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_57 /* synthesis keep */;
wire tp_valid_out_57 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_58 /* synthesis keep */;
wire tp_valid_out_58 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_59 /* synthesis keep */;
wire tp_valid_out_59 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_60 /* synthesis keep */;
wire tp_valid_out_60 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_61 /* synthesis keep */;
wire tp_valid_out_61 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_62 /* synthesis keep */;
wire tp_valid_out_62 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_63 /* synthesis keep */;
wire tp_valid_out_63 /* synthesis keep */;

wire [noc_dw - 1: 0] tp_data_out_64 /* synthesis keep */;
wire tp_valid_out_64 /* synthesis keep */;

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
    .slave_tlast(1'd0)
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
noc_router_adapter_block noc_router_adapter_block_33 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_33),
             .master_tdata(si_data_in_33),
             .master_tvalid(si_valid_in_33),
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
slave_interface si_33(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_33),
             .tdata_in(si_data_in_33),
             .tready(si_ready_33),
             .tdata_out(si_data_out_33),
             .tvalid_out(si_valid_out_33),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_33(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_33),
	        .tvalid_in(si_valid_out_33),
	        .tdata_out(tp_data_out_33),
	        .tvalid_out(tp_valid_out_33)
        );
noc_router_adapter_block noc_router_adapter_block_34 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_34),
             .master_tdata(si_data_in_34),
             .master_tvalid(si_valid_in_34),
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
slave_interface si_34(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_34),
             .tdata_in(si_data_in_34),
             .tready(si_ready_34),
             .tdata_out(si_data_out_34),
             .tvalid_out(si_valid_out_34),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_34(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_34),
	        .tvalid_in(si_valid_out_34),
	        .tdata_out(tp_data_out_34),
	        .tvalid_out(tp_valid_out_34)
        );
noc_router_adapter_block noc_router_adapter_block_35 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_35),
             .master_tdata(si_data_in_35),
             .master_tvalid(si_valid_in_35),
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
slave_interface si_35(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_35),
             .tdata_in(si_data_in_35),
             .tready(si_ready_35),
             .tdata_out(si_data_out_35),
             .tvalid_out(si_valid_out_35),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_35(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_35),
	        .tvalid_in(si_valid_out_35),
	        .tdata_out(tp_data_out_35),
	        .tvalid_out(tp_valid_out_35)
        );
noc_router_adapter_block noc_router_adapter_block_36 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_36),
             .master_tdata(si_data_in_36),
             .master_tvalid(si_valid_in_36),
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
slave_interface si_36(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_36),
             .tdata_in(si_data_in_36),
             .tready(si_ready_36),
             .tdata_out(si_data_out_36),
             .tvalid_out(si_valid_out_36),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_36(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_36),
	        .tvalid_in(si_valid_out_36),
	        .tdata_out(tp_data_out_36),
	        .tvalid_out(tp_valid_out_36)
        );
noc_router_adapter_block noc_router_adapter_block_37 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_37),
             .master_tdata(si_data_in_37),
             .master_tvalid(si_valid_in_37),
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
slave_interface si_37(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_37),
             .tdata_in(si_data_in_37),
             .tready(si_ready_37),
             .tdata_out(si_data_out_37),
             .tvalid_out(si_valid_out_37),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_37(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_37),
	        .tvalid_in(si_valid_out_37),
	        .tdata_out(tp_data_out_37),
	        .tvalid_out(tp_valid_out_37)
        );
noc_router_adapter_block noc_router_adapter_block_38 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_38),
             .master_tdata(si_data_in_38),
             .master_tvalid(si_valid_in_38),
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
slave_interface si_38(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_38),
             .tdata_in(si_data_in_38),
             .tready(si_ready_38),
             .tdata_out(si_data_out_38),
             .tvalid_out(si_valid_out_38),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_38(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_38),
	        .tvalid_in(si_valid_out_38),
	        .tdata_out(tp_data_out_38),
	        .tvalid_out(tp_valid_out_38)
        );
noc_router_adapter_block noc_router_adapter_block_39 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_39),
             .master_tdata(si_data_in_39),
             .master_tvalid(si_valid_in_39),
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
slave_interface si_39(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_39),
             .tdata_in(si_data_in_39),
             .tready(si_ready_39),
             .tdata_out(si_data_out_39),
             .tvalid_out(si_valid_out_39),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_39(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_39),
	        .tvalid_in(si_valid_out_39),
	        .tdata_out(tp_data_out_39),
	        .tvalid_out(tp_valid_out_39)
        );
noc_router_adapter_block noc_router_adapter_block_40 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_40),
             .master_tdata(si_data_in_40),
             .master_tvalid(si_valid_in_40),
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
slave_interface si_40(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_40),
             .tdata_in(si_data_in_40),
             .tready(si_ready_40),
             .tdata_out(si_data_out_40),
             .tvalid_out(si_valid_out_40),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_40(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_40),
	        .tvalid_in(si_valid_out_40),
	        .tdata_out(tp_data_out_40),
	        .tvalid_out(tp_valid_out_40)
        );
noc_router_adapter_block noc_router_adapter_block_41 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_41),
             .master_tdata(si_data_in_41),
             .master_tvalid(si_valid_in_41),
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
slave_interface si_41(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_41),
             .tdata_in(si_data_in_41),
             .tready(si_ready_41),
             .tdata_out(si_data_out_41),
             .tvalid_out(si_valid_out_41),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_41(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_41),
	        .tvalid_in(si_valid_out_41),
	        .tdata_out(tp_data_out_41),
	        .tvalid_out(tp_valid_out_41)
        );
noc_router_adapter_block noc_router_adapter_block_42 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_42),
             .master_tdata(si_data_in_42),
             .master_tvalid(si_valid_in_42),
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
slave_interface si_42(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_42),
             .tdata_in(si_data_in_42),
             .tready(si_ready_42),
             .tdata_out(si_data_out_42),
             .tvalid_out(si_valid_out_42),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_42(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_42),
	        .tvalid_in(si_valid_out_42),
	        .tdata_out(tp_data_out_42),
	        .tvalid_out(tp_valid_out_42)
        );
noc_router_adapter_block noc_router_adapter_block_43 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_43),
             .master_tdata(si_data_in_43),
             .master_tvalid(si_valid_in_43),
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
slave_interface si_43(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_43),
             .tdata_in(si_data_in_43),
             .tready(si_ready_43),
             .tdata_out(si_data_out_43),
             .tvalid_out(si_valid_out_43),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_43(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_43),
	        .tvalid_in(si_valid_out_43),
	        .tdata_out(tp_data_out_43),
	        .tvalid_out(tp_valid_out_43)
        );
noc_router_adapter_block noc_router_adapter_block_44 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_44),
             .master_tdata(si_data_in_44),
             .master_tvalid(si_valid_in_44),
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
slave_interface si_44(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_44),
             .tdata_in(si_data_in_44),
             .tready(si_ready_44),
             .tdata_out(si_data_out_44),
             .tvalid_out(si_valid_out_44),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_44(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_44),
	        .tvalid_in(si_valid_out_44),
	        .tdata_out(tp_data_out_44),
	        .tvalid_out(tp_valid_out_44)
        );
noc_router_adapter_block noc_router_adapter_block_45 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_45),
             .master_tdata(si_data_in_45),
             .master_tvalid(si_valid_in_45),
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
slave_interface si_45(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_45),
             .tdata_in(si_data_in_45),
             .tready(si_ready_45),
             .tdata_out(si_data_out_45),
             .tvalid_out(si_valid_out_45),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_45(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_45),
	        .tvalid_in(si_valid_out_45),
	        .tdata_out(tp_data_out_45),
	        .tvalid_out(tp_valid_out_45)
        );
noc_router_adapter_block noc_router_adapter_block_46 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_46),
             .master_tdata(si_data_in_46),
             .master_tvalid(si_valid_in_46),
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
slave_interface si_46(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_46),
             .tdata_in(si_data_in_46),
             .tready(si_ready_46),
             .tdata_out(si_data_out_46),
             .tvalid_out(si_valid_out_46),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_46(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_46),
	        .tvalid_in(si_valid_out_46),
	        .tdata_out(tp_data_out_46),
	        .tvalid_out(tp_valid_out_46)
        );
noc_router_adapter_block noc_router_adapter_block_47 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_47),
             .master_tdata(si_data_in_47),
             .master_tvalid(si_valid_in_47),
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
slave_interface si_47(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_47),
             .tdata_in(si_data_in_47),
             .tready(si_ready_47),
             .tdata_out(si_data_out_47),
             .tvalid_out(si_valid_out_47),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_47(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_47),
	        .tvalid_in(si_valid_out_47),
	        .tdata_out(tp_data_out_47),
	        .tvalid_out(tp_valid_out_47)
        );
noc_router_adapter_block noc_router_adapter_block_48 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_48),
             .master_tdata(si_data_in_48),
             .master_tvalid(si_valid_in_48),
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
slave_interface si_48(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_48),
             .tdata_in(si_data_in_48),
             .tready(si_ready_48),
             .tdata_out(si_data_out_48),
             .tvalid_out(si_valid_out_48),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_48(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_48),
	        .tvalid_in(si_valid_out_48),
	        .tdata_out(tp_data_out_48),
	        .tvalid_out(tp_valid_out_48)
        );
noc_router_adapter_block noc_router_adapter_block_49 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_49),
             .master_tdata(si_data_in_49),
             .master_tvalid(si_valid_in_49),
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
slave_interface si_49(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_49),
             .tdata_in(si_data_in_49),
             .tready(si_ready_49),
             .tdata_out(si_data_out_49),
             .tvalid_out(si_valid_out_49),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_49(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_49),
	        .tvalid_in(si_valid_out_49),
	        .tdata_out(tp_data_out_49),
	        .tvalid_out(tp_valid_out_49)
        );
noc_router_adapter_block noc_router_adapter_block_50 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_50),
             .master_tdata(si_data_in_50),
             .master_tvalid(si_valid_in_50),
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
slave_interface si_50(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_50),
             .tdata_in(si_data_in_50),
             .tready(si_ready_50),
             .tdata_out(si_data_out_50),
             .tvalid_out(si_valid_out_50),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_50(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_50),
	        .tvalid_in(si_valid_out_50),
	        .tdata_out(tp_data_out_50),
	        .tvalid_out(tp_valid_out_50)
        );
noc_router_adapter_block noc_router_adapter_block_51 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_51),
             .master_tdata(si_data_in_51),
             .master_tvalid(si_valid_in_51),
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
slave_interface si_51(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_51),
             .tdata_in(si_data_in_51),
             .tready(si_ready_51),
             .tdata_out(si_data_out_51),
             .tvalid_out(si_valid_out_51),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_51(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_51),
	        .tvalid_in(si_valid_out_51),
	        .tdata_out(tp_data_out_51),
	        .tvalid_out(tp_valid_out_51)
        );
noc_router_adapter_block noc_router_adapter_block_52 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_52),
             .master_tdata(si_data_in_52),
             .master_tvalid(si_valid_in_52),
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
slave_interface si_52(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_52),
             .tdata_in(si_data_in_52),
             .tready(si_ready_52),
             .tdata_out(si_data_out_52),
             .tvalid_out(si_valid_out_52),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_52(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_52),
	        .tvalid_in(si_valid_out_52),
	        .tdata_out(tp_data_out_52),
	        .tvalid_out(tp_valid_out_52)
        );
noc_router_adapter_block noc_router_adapter_block_53 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_53),
             .master_tdata(si_data_in_53),
             .master_tvalid(si_valid_in_53),
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
slave_interface si_53(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_53),
             .tdata_in(si_data_in_53),
             .tready(si_ready_53),
             .tdata_out(si_data_out_53),
             .tvalid_out(si_valid_out_53),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_53(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_53),
	        .tvalid_in(si_valid_out_53),
	        .tdata_out(tp_data_out_53),
	        .tvalid_out(tp_valid_out_53)
        );
noc_router_adapter_block noc_router_adapter_block_54 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_54),
             .master_tdata(si_data_in_54),
             .master_tvalid(si_valid_in_54),
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
slave_interface si_54(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_54),
             .tdata_in(si_data_in_54),
             .tready(si_ready_54),
             .tdata_out(si_data_out_54),
             .tvalid_out(si_valid_out_54),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_54(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_54),
	        .tvalid_in(si_valid_out_54),
	        .tdata_out(tp_data_out_54),
	        .tvalid_out(tp_valid_out_54)
        );
noc_router_adapter_block noc_router_adapter_block_55 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_55),
             .master_tdata(si_data_in_55),
             .master_tvalid(si_valid_in_55),
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
slave_interface si_55(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_55),
             .tdata_in(si_data_in_55),
             .tready(si_ready_55),
             .tdata_out(si_data_out_55),
             .tvalid_out(si_valid_out_55),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_55(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_55),
	        .tvalid_in(si_valid_out_55),
	        .tdata_out(tp_data_out_55),
	        .tvalid_out(tp_valid_out_55)
        );
noc_router_adapter_block noc_router_adapter_block_56 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_56),
             .master_tdata(si_data_in_56),
             .master_tvalid(si_valid_in_56),
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
slave_interface si_56(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_56),
             .tdata_in(si_data_in_56),
             .tready(si_ready_56),
             .tdata_out(si_data_out_56),
             .tvalid_out(si_valid_out_56),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_56(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_56),
	        .tvalid_in(si_valid_out_56),
	        .tdata_out(tp_data_out_56),
	        .tvalid_out(tp_valid_out_56)
        );
noc_router_adapter_block noc_router_adapter_block_57 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_57),
             .master_tdata(si_data_in_57),
             .master_tvalid(si_valid_in_57),
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
slave_interface si_57(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_57),
             .tdata_in(si_data_in_57),
             .tready(si_ready_57),
             .tdata_out(si_data_out_57),
             .tvalid_out(si_valid_out_57),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_57(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_57),
	        .tvalid_in(si_valid_out_57),
	        .tdata_out(tp_data_out_57),
	        .tvalid_out(tp_valid_out_57)
        );
noc_router_adapter_block noc_router_adapter_block_58 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_58),
             .master_tdata(si_data_in_58),
             .master_tvalid(si_valid_in_58),
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
slave_interface si_58(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_58),
             .tdata_in(si_data_in_58),
             .tready(si_ready_58),
             .tdata_out(si_data_out_58),
             .tvalid_out(si_valid_out_58),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_58(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_58),
	        .tvalid_in(si_valid_out_58),
	        .tdata_out(tp_data_out_58),
	        .tvalid_out(tp_valid_out_58)
        );
noc_router_adapter_block noc_router_adapter_block_59 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_59),
             .master_tdata(si_data_in_59),
             .master_tvalid(si_valid_in_59),
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
slave_interface si_59(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_59),
             .tdata_in(si_data_in_59),
             .tready(si_ready_59),
             .tdata_out(si_data_out_59),
             .tvalid_out(si_valid_out_59),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_59(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_59),
	        .tvalid_in(si_valid_out_59),
	        .tdata_out(tp_data_out_59),
	        .tvalid_out(tp_valid_out_59)
        );
noc_router_adapter_block noc_router_adapter_block_60 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_60),
             .master_tdata(si_data_in_60),
             .master_tvalid(si_valid_in_60),
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
slave_interface si_60(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_60),
             .tdata_in(si_data_in_60),
             .tready(si_ready_60),
             .tdata_out(si_data_out_60),
             .tvalid_out(si_valid_out_60),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_60(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_60),
	        .tvalid_in(si_valid_out_60),
	        .tdata_out(tp_data_out_60),
	        .tvalid_out(tp_valid_out_60)
        );
noc_router_adapter_block noc_router_adapter_block_61 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_61),
             .master_tdata(si_data_in_61),
             .master_tvalid(si_valid_in_61),
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
slave_interface si_61(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_61),
             .tdata_in(si_data_in_61),
             .tready(si_ready_61),
             .tdata_out(si_data_out_61),
             .tvalid_out(si_valid_out_61),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_61(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_61),
	        .tvalid_in(si_valid_out_61),
	        .tdata_out(tp_data_out_61),
	        .tvalid_out(tp_valid_out_61)
        );
noc_router_adapter_block noc_router_adapter_block_62 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_62),
             .master_tdata(si_data_in_62),
             .master_tvalid(si_valid_in_62),
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
slave_interface si_62(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_62),
             .tdata_in(si_data_in_62),
             .tready(si_ready_62),
             .tdata_out(si_data_out_62),
             .tvalid_out(si_valid_out_62),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_62(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_62),
	        .tvalid_in(si_valid_out_62),
	        .tdata_out(tp_data_out_62),
	        .tvalid_out(tp_valid_out_62)
        );
noc_router_adapter_block noc_router_adapter_block_63 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_63),
             .master_tdata(si_data_in_63),
             .master_tvalid(si_valid_in_63),
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
slave_interface si_63(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_63),
             .tdata_in(si_data_in_63),
             .tready(si_ready_63),
             .tdata_out(si_data_out_63),
             .tvalid_out(si_valid_out_63),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_63(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_63),
	        .tvalid_in(si_valid_out_63),
	        .tdata_out(tp_data_out_63),
	        .tvalid_out(tp_valid_out_63)
        );
noc_router_adapter_block noc_router_adapter_block_64 (
             .clk(clk),
             .reset(reset),
             .master_tready(si_ready_64),
             .master_tdata(si_data_in_64),
             .master_tvalid(si_valid_in_64),
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
slave_interface si_64(
             .clk(clk),
             .reset(reset),
             .tvalid_in(si_valid_in_64),
             .tdata_in(si_data_in_64),
             .tready(si_ready_64),
             .tdata_out(si_data_out_64),
             .tvalid_out(si_valid_out_64),
             .tstrb(8'd0),
             .tkeep(8'd0),
             .tid(8'd0),
             .tdest(8'd0),
             .tuser(8'd0),
             .tlast(1'd0)
         );
traffic_processor tp_64(
 	        .clk(clk),
	        .reset(reset),
	        .tdata_in(si_data_out_64),
	        .tvalid_in(si_valid_out_64),
	        .tdata_out(tp_data_out_64),
	        .tvalid_out(tp_valid_out_64)
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

wire [noc_dw - 1: 0] reg_data_out_32;
wire [noc_dw - 1: 0] reg_valid_out_32;

wire [noc_dw - 1: 0] reg_data_out_33;
wire [noc_dw - 1: 0] reg_valid_out_33;

wire [noc_dw - 1: 0] reg_data_out_34;
wire [noc_dw - 1: 0] reg_valid_out_34;

wire [noc_dw - 1: 0] reg_data_out_35;
wire [noc_dw - 1: 0] reg_valid_out_35;

wire [noc_dw - 1: 0] reg_data_out_36;
wire [noc_dw - 1: 0] reg_valid_out_36;

wire [noc_dw - 1: 0] reg_data_out_37;
wire [noc_dw - 1: 0] reg_valid_out_37;

wire [noc_dw - 1: 0] reg_data_out_38;
wire [noc_dw - 1: 0] reg_valid_out_38;

wire [noc_dw - 1: 0] reg_data_out_39;
wire [noc_dw - 1: 0] reg_valid_out_39;

wire [noc_dw - 1: 0] reg_data_out_40;
wire [noc_dw - 1: 0] reg_valid_out_40;

wire [noc_dw - 1: 0] reg_data_out_41;
wire [noc_dw - 1: 0] reg_valid_out_41;

wire [noc_dw - 1: 0] reg_data_out_42;
wire [noc_dw - 1: 0] reg_valid_out_42;

wire [noc_dw - 1: 0] reg_data_out_43;
wire [noc_dw - 1: 0] reg_valid_out_43;

wire [noc_dw - 1: 0] reg_data_out_44;
wire [noc_dw - 1: 0] reg_valid_out_44;

wire [noc_dw - 1: 0] reg_data_out_45;
wire [noc_dw - 1: 0] reg_valid_out_45;

wire [noc_dw - 1: 0] reg_data_out_46;
wire [noc_dw - 1: 0] reg_valid_out_46;

wire [noc_dw - 1: 0] reg_data_out_47;
wire [noc_dw - 1: 0] reg_valid_out_47;

wire [noc_dw - 1: 0] reg_data_out_48;
wire [noc_dw - 1: 0] reg_valid_out_48;

wire [noc_dw - 1: 0] reg_data_out_49;
wire [noc_dw - 1: 0] reg_valid_out_49;

wire [noc_dw - 1: 0] reg_data_out_50;
wire [noc_dw - 1: 0] reg_valid_out_50;

wire [noc_dw - 1: 0] reg_data_out_51;
wire [noc_dw - 1: 0] reg_valid_out_51;

wire [noc_dw - 1: 0] reg_data_out_52;
wire [noc_dw - 1: 0] reg_valid_out_52;

wire [noc_dw - 1: 0] reg_data_out_53;
wire [noc_dw - 1: 0] reg_valid_out_53;

wire [noc_dw - 1: 0] reg_data_out_54;
wire [noc_dw - 1: 0] reg_valid_out_54;

wire [noc_dw - 1: 0] reg_data_out_55;
wire [noc_dw - 1: 0] reg_valid_out_55;

wire [noc_dw - 1: 0] reg_data_out_56;
wire [noc_dw - 1: 0] reg_valid_out_56;

wire [noc_dw - 1: 0] reg_data_out_57;
wire [noc_dw - 1: 0] reg_valid_out_57;

wire [noc_dw - 1: 0] reg_data_out_58;
wire [noc_dw - 1: 0] reg_valid_out_58;

wire [noc_dw - 1: 0] reg_data_out_59;
wire [noc_dw - 1: 0] reg_valid_out_59;

wire [noc_dw - 1: 0] reg_data_out_60;
wire [noc_dw - 1: 0] reg_valid_out_60;

wire [noc_dw - 1: 0] reg_data_out_61;
wire [noc_dw - 1: 0] reg_valid_out_61;

wire [noc_dw - 1: 0] reg_data_out_62;
wire [noc_dw - 1: 0] reg_valid_out_62;

wire [noc_dw - 1: 0] reg_data_out_63;
wire [noc_dw - 1: 0] reg_valid_out_63;

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
register r_32 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_32),
             .data_out(reg_data_out_32)
         );
register r_33 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_33),
             .data_out(reg_data_out_33)
         );
register r_34 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_34),
             .data_out(reg_data_out_34)
         );
register r_35 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_35),
             .data_out(reg_data_out_35)
         );
register r_36 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_36),
             .data_out(reg_data_out_36)
         );
register r_37 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_37),
             .data_out(reg_data_out_37)
         );
register r_38 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_38),
             .data_out(reg_data_out_38)
         );
register r_39 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_39),
             .data_out(reg_data_out_39)
         );
register r_40 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_40),
             .data_out(reg_data_out_40)
         );
register r_41 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_41),
             .data_out(reg_data_out_41)
         );
register r_42 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_42),
             .data_out(reg_data_out_42)
         );
register r_43 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_43),
             .data_out(reg_data_out_43)
         );
register r_44 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_44),
             .data_out(reg_data_out_44)
         );
register r_45 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_45),
             .data_out(reg_data_out_45)
         );
register r_46 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_46),
             .data_out(reg_data_out_46)
         );
register r_47 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_47),
             .data_out(reg_data_out_47)
         );
register r_48 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_48),
             .data_out(reg_data_out_48)
         );
register r_49 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_49),
             .data_out(reg_data_out_49)
         );
register r_50 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_50),
             .data_out(reg_data_out_50)
         );
register r_51 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_51),
             .data_out(reg_data_out_51)
         );
register r_52 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_52),
             .data_out(reg_data_out_52)
         );
register r_53 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_53),
             .data_out(reg_data_out_53)
         );
register r_54 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_54),
             .data_out(reg_data_out_54)
         );
register r_55 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_55),
             .data_out(reg_data_out_55)
         );
register r_56 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_56),
             .data_out(reg_data_out_56)
         );
register r_57 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_57),
             .data_out(reg_data_out_57)
         );
register r_58 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_58),
             .data_out(reg_data_out_58)
         );
register r_59 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_59),
             .data_out(reg_data_out_59)
         );
register r_60 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_60),
             .data_out(reg_data_out_60)
         );
register r_61 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_61),
             .data_out(reg_data_out_61)
         );
register r_62 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_62),
             .data_out(reg_data_out_62)
         );
register r_63 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_data_out_63),
             .data_out(reg_data_out_63)
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
register r_valid_32 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_32),
             .data_out(reg_Valid_out_32)
         );
register r_valid_33 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_33),
             .data_out(reg_Valid_out_33)
         );
register r_valid_34 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_34),
             .data_out(reg_Valid_out_34)
         );
register r_valid_35 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_35),
             .data_out(reg_Valid_out_35)
         );
register r_valid_36 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_36),
             .data_out(reg_Valid_out_36)
         );
register r_valid_37 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_37),
             .data_out(reg_Valid_out_37)
         );
register r_valid_38 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_38),
             .data_out(reg_Valid_out_38)
         );
register r_valid_39 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_39),
             .data_out(reg_Valid_out_39)
         );
register r_valid_40 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_40),
             .data_out(reg_Valid_out_40)
         );
register r_valid_41 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_41),
             .data_out(reg_Valid_out_41)
         );
register r_valid_42 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_42),
             .data_out(reg_Valid_out_42)
         );
register r_valid_43 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_43),
             .data_out(reg_Valid_out_43)
         );
register r_valid_44 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_44),
             .data_out(reg_Valid_out_44)
         );
register r_valid_45 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_45),
             .data_out(reg_Valid_out_45)
         );
register r_valid_46 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_46),
             .data_out(reg_Valid_out_46)
         );
register r_valid_47 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_47),
             .data_out(reg_Valid_out_47)
         );
register r_valid_48 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_48),
             .data_out(reg_Valid_out_48)
         );
register r_valid_49 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_49),
             .data_out(reg_Valid_out_49)
         );
register r_valid_50 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_50),
             .data_out(reg_Valid_out_50)
         );
register r_valid_51 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_51),
             .data_out(reg_Valid_out_51)
         );
register r_valid_52 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_52),
             .data_out(reg_Valid_out_52)
         );
register r_valid_53 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_53),
             .data_out(reg_Valid_out_53)
         );
register r_valid_54 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_54),
             .data_out(reg_Valid_out_54)
         );
register r_valid_55 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_55),
             .data_out(reg_Valid_out_55)
         );
register r_valid_56 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_56),
             .data_out(reg_Valid_out_56)
         );
register r_valid_57 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_57),
             .data_out(reg_Valid_out_57)
         );
register r_valid_58 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_58),
             .data_out(reg_Valid_out_58)
         );
register r_valid_59 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_59),
             .data_out(reg_Valid_out_59)
         );
register r_valid_60 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_60),
             .data_out(reg_Valid_out_60)
         );
register r_valid_61 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_61),
             .data_out(reg_Valid_out_61)
         );
register r_valid_62 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_62),
             .data_out(reg_Valid_out_62)
         );
register r_valid_63 (
             .clk(clk),
             .reset(reset),
             .data_in(tp_valid_out_63),
             .data_out(reg_Valid_out_63)
         );

/*******************Output Logic***************************/
assign data_out = tp_data_out_64;

endmodule
