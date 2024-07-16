/*
    Top level modules to instantiate an AXI handshake between 64 routers.
    Each routers receives 32-bit data, processes it, pass it to the next routers in a 2D chain.
    For now, all of our routers traffic processor's module does the same calculation, 
    but in a more complicated design, we can add different logic to each router's traffic 
    processor module.
*/

module simple_64_noc_2D_chain (
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
wire [noc_dw - 1: 0] tp_last_data_out;

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

//traffic processor data - middle routers
wire [noc_dw - 1: 0] tp_data_out_1;
wire tp_valid_out_2;
wire [noc_dw - 1: 0] tp_data_out_2;
wire tp_valid_out_3;
wire [noc_dw - 1: 0] tp_data_out_3;
wire tp_valid_out_4;
wire [noc_dw - 1: 0] tp_data_out_4;
wire tp_valid_out_5;
wire [noc_dw - 1: 0] tp_data_out_5;
wire tp_valid_out_6;
wire [noc_dw - 1: 0] tp_data_out_6;
wire tp_valid_out_7;
wire [noc_dw - 1: 0] tp_data_out_7;
wire tp_valid_out_8;
wire [noc_dw - 1: 0] tp_data_out_8;
wire tp_valid_out_9;
wire [noc_dw - 1: 0] tp_data_out_9;
wire tp_valid_out_10;
wire [noc_dw - 1: 0] tp_data_out_10;
wire tp_valid_out_11;
wire [noc_dw - 1: 0] tp_data_out_11;
wire tp_valid_out_12;
wire [noc_dw - 1: 0] tp_data_out_12;
wire tp_valid_out_13;
wire [noc_dw - 1: 0] tp_data_out_13;
wire tp_valid_out_14;
wire [noc_dw - 1: 0] tp_data_out_14;
wire tp_valid_out_15;
wire [noc_dw - 1: 0] tp_data_out_15;
wire tp_valid_out_16;
wire [noc_dw - 1: 0] tp_data_out_16;
wire tp_valid_out_17;
wire [noc_dw - 1: 0] tp_data_out_17;
wire tp_valid_out_18;
wire [noc_dw - 1: 0] tp_data_out_18;
wire tp_valid_out_19;
wire [noc_dw - 1: 0] tp_data_out_19;
wire tp_valid_out_20;
wire [noc_dw - 1: 0] tp_data_out_20;
wire tp_valid_out_21;
wire [noc_dw - 1: 0] tp_data_out_21;
wire tp_valid_out_22;
wire [noc_dw - 1: 0] tp_data_out_22;
wire tp_valid_out_23;
wire [noc_dw - 1: 0] tp_data_out_23;
wire tp_valid_out_24;
wire [noc_dw - 1: 0] tp_data_out_24;
wire tp_valid_out_25;
wire [noc_dw - 1: 0] tp_data_out_25;
wire tp_valid_out_26;
wire [noc_dw - 1: 0] tp_data_out_26;
wire tp_valid_out_27;
wire [noc_dw - 1: 0] tp_data_out_27;
wire tp_valid_out_28;
wire [noc_dw - 1: 0] tp_data_out_28;
wire tp_valid_out_29;
wire [noc_dw - 1: 0] tp_data_out_29;
wire tp_valid_out_30;
wire [noc_dw - 1: 0] tp_data_out_30;
wire tp_valid_out_31;
wire [noc_dw - 1: 0] tp_data_out_31;
wire tp_valid_out_32;
wire [noc_dw - 1: 0] tp_data_out_32;
wire tp_valid_out_33;
wire [noc_dw - 1: 0] tp_data_out_33;
wire tp_valid_out_34;
wire [noc_dw - 1: 0] tp_data_out_34;
wire tp_valid_out_35;
wire [noc_dw - 1: 0] tp_data_out_35;
wire tp_valid_out_36;
wire [noc_dw - 1: 0] tp_data_out_36;
wire tp_valid_out_37;
wire [noc_dw - 1: 0] tp_data_out_37;
wire tp_valid_out_38;
wire [noc_dw - 1: 0] tp_data_out_38;
wire tp_valid_out_39;
wire [noc_dw - 1: 0] tp_data_out_39;
wire tp_valid_out_40;
wire [noc_dw - 1: 0] tp_data_out_40;
wire tp_valid_out_41;
wire [noc_dw - 1: 0] tp_data_out_41;
wire tp_valid_out_42;
wire [noc_dw - 1: 0] tp_data_out_42;
wire tp_valid_out_43;
wire [noc_dw - 1: 0] tp_data_out_43;
wire tp_valid_out_44;
wire [noc_dw - 1: 0] tp_data_out_44;
wire tp_valid_out_45;
wire [noc_dw - 1: 0] tp_data_out_45;
wire tp_valid_out_46;
wire [noc_dw - 1: 0] tp_data_out_46;
wire tp_valid_out_47;
wire [noc_dw - 1: 0] tp_data_out_47;
wire tp_valid_out_48;
wire [noc_dw - 1: 0] tp_data_out_48;
wire tp_valid_out_49;
wire [noc_dw - 1: 0] tp_data_out_49;
wire tp_valid_out_50;
wire [noc_dw - 1: 0] tp_data_out_50;
wire tp_valid_out_51;
wire [noc_dw - 1: 0] tp_data_out_51;
wire tp_valid_out_52;
wire [noc_dw - 1: 0] tp_data_out_52;
wire tp_valid_out_53;
wire [noc_dw - 1: 0] tp_data_out_53;
wire tp_valid_out_54;
wire [noc_dw - 1: 0] tp_data_out_54;
wire tp_valid_out_55;
wire [noc_dw - 1: 0] tp_data_out_55;
wire tp_valid_out_56;
wire [noc_dw - 1: 0] tp_data_out_56;
wire tp_valid_out_57;
wire [noc_dw - 1: 0] tp_data_out_57;
wire tp_valid_out_58;
wire [noc_dw - 1: 0] tp_data_out_58;
wire tp_valid_out_59;
wire [noc_dw - 1: 0] tp_data_out_59;
wire tp_valid_out_60;
wire [noc_dw - 1: 0] tp_data_out_60;
wire tp_valid_out_61;
wire [noc_dw - 1: 0] tp_data_out_61;
wire tp_valid_out_62;
wire [noc_dw - 1: 0] tp_data_out_62;
wire tp_valid_out_63;

//master interface data - middle routers
wire [noc_dw - 1: 0] mi_data_2;
wire mi_valid_2;
wire mi_ready_2;
wire [noc_dw - 1: 0] mi_data_3;
wire mi_valid_3;
wire mi_ready_3;
wire [noc_dw - 1: 0] mi_data_4;
wire mi_valid_4;
wire mi_ready_4;
wire [noc_dw - 1: 0] mi_data_5;
wire mi_valid_5;
wire mi_ready_5;
wire [noc_dw - 1: 0] mi_data_6;
wire mi_valid_6;
wire mi_ready_6;
wire [noc_dw - 1: 0] mi_data_7;
wire mi_valid_7;
wire mi_ready_7;
wire [noc_dw - 1: 0] mi_data_8;
wire mi_valid_8;
wire mi_ready_8;
wire [noc_dw - 1: 0] mi_data_9;
wire mi_valid_9;
wire mi_ready_9;
wire [noc_dw - 1: 0] mi_data_10;
wire mi_valid_10;
wire mi_ready_10;
wire [noc_dw - 1: 0] mi_data_11;
wire mi_valid_11;
wire mi_ready_11;
wire [noc_dw - 1: 0] mi_data_12;
wire mi_valid_12;
wire mi_ready_12;
wire [noc_dw - 1: 0] mi_data_13;
wire mi_valid_13;
wire mi_ready_13;
wire [noc_dw - 1: 0] mi_data_14;
wire mi_valid_14;
wire mi_ready_14;
wire [noc_dw - 1: 0] mi_data_15;
wire mi_valid_15;
wire mi_ready_15;
wire [noc_dw - 1: 0] mi_data_16;
wire mi_valid_16;
wire mi_ready_16;
wire [noc_dw - 1: 0] mi_data_17;
wire mi_valid_17;
wire mi_ready_17;
wire [noc_dw - 1: 0] mi_data_18;
wire mi_valid_18;
wire mi_ready_18;
wire [noc_dw - 1: 0] mi_data_19;
wire mi_valid_19;
wire mi_ready_19;
wire [noc_dw - 1: 0] mi_data_20;
wire mi_valid_20;
wire mi_ready_20;
wire [noc_dw - 1: 0] mi_data_21;
wire mi_valid_21;
wire mi_ready_21;
wire [noc_dw - 1: 0] mi_data_22;
wire mi_valid_22;
wire mi_ready_22;
wire [noc_dw - 1: 0] mi_data_23;
wire mi_valid_23;
wire mi_ready_23;
wire [noc_dw - 1: 0] mi_data_24;
wire mi_valid_24;
wire mi_ready_24;
wire [noc_dw - 1: 0] mi_data_25;
wire mi_valid_25;
wire mi_ready_25;
wire [noc_dw - 1: 0] mi_data_26;
wire mi_valid_26;
wire mi_ready_26;
wire [noc_dw - 1: 0] mi_data_27;
wire mi_valid_27;
wire mi_ready_27;
wire [noc_dw - 1: 0] mi_data_28;
wire mi_valid_28;
wire mi_ready_28;
wire [noc_dw - 1: 0] mi_data_29;
wire mi_valid_29;
wire mi_ready_29;
wire [noc_dw - 1: 0] mi_data_30;
wire mi_valid_30;
wire mi_ready_30;
wire [noc_dw - 1: 0] mi_data_31;
wire mi_valid_31;
wire mi_ready_31;
wire [noc_dw - 1: 0] mi_data_32;
wire mi_valid_32;
wire mi_ready_32;
wire [noc_dw - 1: 0] mi_data_33;
wire mi_valid_33;
wire mi_ready_33;
wire [noc_dw - 1: 0] mi_data_34;
wire mi_valid_34;
wire mi_ready_34;
wire [noc_dw - 1: 0] mi_data_35;
wire mi_valid_35;
wire mi_ready_35;
wire [noc_dw - 1: 0] mi_data_36;
wire mi_valid_36;
wire mi_ready_36;
wire [noc_dw - 1: 0] mi_data_37;
wire mi_valid_37;
wire mi_ready_37;
wire [noc_dw - 1: 0] mi_data_38;
wire mi_valid_38;
wire mi_ready_38;
wire [noc_dw - 1: 0] mi_data_39;
wire mi_valid_39;
wire mi_ready_39;
wire [noc_dw - 1: 0] mi_data_40;
wire mi_valid_40;
wire mi_ready_40;
wire [noc_dw - 1: 0] mi_data_41;
wire mi_valid_41;
wire mi_ready_41;
wire [noc_dw - 1: 0] mi_data_42;
wire mi_valid_42;
wire mi_ready_42;
wire [noc_dw - 1: 0] mi_data_43;
wire mi_valid_43;
wire mi_ready_43;
wire [noc_dw - 1: 0] mi_data_44;
wire mi_valid_44;
wire mi_ready_44;
wire [noc_dw - 1: 0] mi_data_45;
wire mi_valid_45;
wire mi_ready_45;
wire [noc_dw - 1: 0] mi_data_46;
wire mi_valid_46;
wire mi_ready_46;
wire [noc_dw - 1: 0] mi_data_47;
wire mi_valid_47;
wire mi_ready_47;
wire [noc_dw - 1: 0] mi_data_48;
wire mi_valid_48;
wire mi_ready_48;
wire [noc_dw - 1: 0] mi_data_49;
wire mi_valid_49;
wire mi_ready_49;
wire [noc_dw - 1: 0] mi_data_50;
wire mi_valid_50;
wire mi_ready_50;
wire [noc_dw - 1: 0] mi_data_51;
wire mi_valid_51;
wire mi_ready_51;
wire [noc_dw - 1: 0] mi_data_52;
wire mi_valid_52;
wire mi_ready_52;
wire [noc_dw - 1: 0] mi_data_53;
wire mi_valid_53;
wire mi_ready_53;
wire [noc_dw - 1: 0] mi_data_54;
wire mi_valid_54;
wire mi_ready_54;
wire [noc_dw - 1: 0] mi_data_55;
wire mi_valid_55;
wire mi_ready_55;
wire [noc_dw - 1: 0] mi_data_56;
wire mi_valid_56;
wire mi_ready_56;
wire [noc_dw - 1: 0] mi_data_57;
wire mi_valid_57;
wire mi_ready_57;
wire [noc_dw - 1: 0] mi_data_58;
wire mi_valid_58;
wire mi_ready_58;
wire [noc_dw - 1: 0] mi_data_59;
wire mi_valid_59;
wire mi_ready_59;
wire [noc_dw - 1: 0] mi_data_60;
wire mi_valid_60;
wire mi_ready_60;
wire [noc_dw - 1: 0] mi_data_61;
wire mi_valid_61;
wire mi_ready_61;
wire [noc_dw - 1: 0] mi_data_62;
wire mi_valid_62;
wire mi_ready_62;
wire [noc_dw - 1: 0] mi_data_63;
wire mi_valid_63;
wire mi_ready_63;


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
             .slave_tvalid(mi_valid_4),
             .slave_tready(mi_ready_4), 
             .slave_tdata(mi_data_4),
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
master_interface mi_4(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_4),
            .tdata_in(tp_data_out_4),
            .tready(mi_ready_4),
            .tdata_out(mi_data_4),
            .tvalid_out(mi_valid_4),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_5),
             .slave_tready(mi_ready_5), 
             .slave_tdata(mi_data_5),
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
master_interface mi_5(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_5),
            .tdata_in(tp_data_out_5),
            .tready(mi_ready_5),
            .tdata_out(mi_data_5),
            .tvalid_out(mi_valid_5),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_6),
             .slave_tready(mi_ready_6), 
             .slave_tdata(mi_data_6),
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
master_interface mi_6(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_6),
            .tdata_in(tp_data_out_6),
            .tready(mi_ready_6),
            .tdata_out(mi_data_6),
            .tvalid_out(mi_valid_6),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_7),
             .slave_tready(mi_ready_7), 
             .slave_tdata(mi_data_7),
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
master_interface mi_7(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_7),
            .tdata_in(tp_data_out_7),
            .tready(mi_ready_7),
            .tdata_out(mi_data_7),
            .tvalid_out(mi_valid_7),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_8),
             .slave_tready(mi_ready_8), 
             .slave_tdata(mi_data_8),
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
master_interface mi_8(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_8),
            .tdata_in(tp_data_out_8),
            .tready(mi_ready_8),
            .tdata_out(mi_data_8),
            .tvalid_out(mi_valid_8),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_9),
             .slave_tready(mi_ready_9), 
             .slave_tdata(mi_data_9),
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
master_interface mi_9(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_9),
            .tdata_in(tp_data_out_9),
            .tready(mi_ready_9),
            .tdata_out(mi_data_9),
            .tvalid_out(mi_valid_9),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_10),
             .slave_tready(mi_ready_10), 
             .slave_tdata(mi_data_10),
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
master_interface mi_10(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_10),
            .tdata_in(tp_data_out_10),
            .tready(mi_ready_10),
            .tdata_out(mi_data_10),
            .tvalid_out(mi_valid_10),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_11),
             .slave_tready(mi_ready_11), 
             .slave_tdata(mi_data_11),
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
master_interface mi_11(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_11),
            .tdata_in(tp_data_out_11),
            .tready(mi_ready_11),
            .tdata_out(mi_data_11),
            .tvalid_out(mi_valid_11),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_12),
             .slave_tready(mi_ready_12), 
             .slave_tdata(mi_data_12),
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
master_interface mi_12(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_12),
            .tdata_in(tp_data_out_12),
            .tready(mi_ready_12),
            .tdata_out(mi_data_12),
            .tvalid_out(mi_valid_12),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_13),
             .slave_tready(mi_ready_13), 
             .slave_tdata(mi_data_13),
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
master_interface mi_13(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_13),
            .tdata_in(tp_data_out_13),
            .tready(mi_ready_13),
            .tdata_out(mi_data_13),
            .tvalid_out(mi_valid_13),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_14),
             .slave_tready(mi_ready_14), 
             .slave_tdata(mi_data_14),
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
master_interface mi_14(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_14),
            .tdata_in(tp_data_out_14),
            .tready(mi_ready_14),
            .tdata_out(mi_data_14),
            .tvalid_out(mi_valid_14),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_15),
             .slave_tready(mi_ready_15), 
             .slave_tdata(mi_data_15),
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
master_interface mi_15(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_15),
            .tdata_in(tp_data_out_15),
            .tready(mi_ready_15),
            .tdata_out(mi_data_15),
            .tvalid_out(mi_valid_15),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_16),
             .slave_tready(mi_ready_16), 
             .slave_tdata(mi_data_16),
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
master_interface mi_16(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_16),
            .tdata_in(tp_data_out_16),
            .tready(mi_ready_16),
            .tdata_out(mi_data_16),
            .tvalid_out(mi_valid_16),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_17),
             .slave_tready(mi_ready_17), 
             .slave_tdata(mi_data_17),
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
master_interface mi_17(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_17),
            .tdata_in(tp_data_out_17),
            .tready(mi_ready_17),
            .tdata_out(mi_data_17),
            .tvalid_out(mi_valid_17),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_18),
             .slave_tready(mi_ready_18), 
             .slave_tdata(mi_data_18),
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
master_interface mi_18(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_18),
            .tdata_in(tp_data_out_18),
            .tready(mi_ready_18),
            .tdata_out(mi_data_18),
            .tvalid_out(mi_valid_18),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_19),
             .slave_tready(mi_ready_19), 
             .slave_tdata(mi_data_19),
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
master_interface mi_19(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_19),
            .tdata_in(tp_data_out_19),
            .tready(mi_ready_19),
            .tdata_out(mi_data_19),
            .tvalid_out(mi_valid_19),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_20),
             .slave_tready(mi_ready_20), 
             .slave_tdata(mi_data_20),
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
master_interface mi_20(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_20),
            .tdata_in(tp_data_out_20),
            .tready(mi_ready_20),
            .tdata_out(mi_data_20),
            .tvalid_out(mi_valid_20),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_21),
             .slave_tready(mi_ready_21), 
             .slave_tdata(mi_data_21),
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
master_interface mi_21(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_21),
            .tdata_in(tp_data_out_21),
            .tready(mi_ready_21),
            .tdata_out(mi_data_21),
            .tvalid_out(mi_valid_21),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_22),
             .slave_tready(mi_ready_22), 
             .slave_tdata(mi_data_22),
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
master_interface mi_22(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_22),
            .tdata_in(tp_data_out_22),
            .tready(mi_ready_22),
            .tdata_out(mi_data_22),
            .tvalid_out(mi_valid_22),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_23),
             .slave_tready(mi_ready_23), 
             .slave_tdata(mi_data_23),
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
master_interface mi_23(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_23),
            .tdata_in(tp_data_out_23),
            .tready(mi_ready_23),
            .tdata_out(mi_data_23),
            .tvalid_out(mi_valid_23),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_24),
             .slave_tready(mi_ready_24), 
             .slave_tdata(mi_data_24),
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
master_interface mi_24(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_24),
            .tdata_in(tp_data_out_24),
            .tready(mi_ready_24),
            .tdata_out(mi_data_24),
            .tvalid_out(mi_valid_24),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_25),
             .slave_tready(mi_ready_25), 
             .slave_tdata(mi_data_25),
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
master_interface mi_25(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_25),
            .tdata_in(tp_data_out_25),
            .tready(mi_ready_25),
            .tdata_out(mi_data_25),
            .tvalid_out(mi_valid_25),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_26),
             .slave_tready(mi_ready_26), 
             .slave_tdata(mi_data_26),
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
master_interface mi_26(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_26),
            .tdata_in(tp_data_out_26),
            .tready(mi_ready_26),
            .tdata_out(mi_data_26),
            .tvalid_out(mi_valid_26),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_27),
             .slave_tready(mi_ready_27), 
             .slave_tdata(mi_data_27),
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
master_interface mi_27(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_27),
            .tdata_in(tp_data_out_27),
            .tready(mi_ready_27),
            .tdata_out(mi_data_27),
            .tvalid_out(mi_valid_27),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_28),
             .slave_tready(mi_ready_28), 
             .slave_tdata(mi_data_28),
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
master_interface mi_28(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_28),
            .tdata_in(tp_data_out_28),
            .tready(mi_ready_28),
            .tdata_out(mi_data_28),
            .tvalid_out(mi_valid_28),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_29),
             .slave_tready(mi_ready_29), 
             .slave_tdata(mi_data_29),
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
master_interface mi_29(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_29),
            .tdata_in(tp_data_out_29),
            .tready(mi_ready_29),
            .tdata_out(mi_data_29),
            .tvalid_out(mi_valid_29),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_30),
             .slave_tready(mi_ready_30), 
             .slave_tdata(mi_data_30),
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
master_interface mi_30(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_30),
            .tdata_in(tp_data_out_30),
            .tready(mi_ready_30),
            .tdata_out(mi_data_30),
            .tvalid_out(mi_valid_30),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_31),
             .slave_tready(mi_ready_31), 
             .slave_tdata(mi_data_31),
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
master_interface mi_31(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_31),
            .tdata_in(tp_data_out_31),
            .tready(mi_ready_31),
            .tdata_out(mi_data_31),
            .tvalid_out(mi_valid_31),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_32),
             .slave_tready(mi_ready_32), 
             .slave_tdata(mi_data_32),
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
master_interface mi_32(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_32),
            .tdata_in(tp_data_out_32),
            .tready(mi_ready_32),
            .tdata_out(mi_data_32),
            .tvalid_out(mi_valid_32),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_33),
             .slave_tready(mi_ready_33), 
             .slave_tdata(mi_data_33),
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
master_interface mi_33(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_33),
            .tdata_in(tp_data_out_33),
            .tready(mi_ready_33),
            .tdata_out(mi_data_33),
            .tvalid_out(mi_valid_33),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_34),
             .slave_tready(mi_ready_34), 
             .slave_tdata(mi_data_34),
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
master_interface mi_34(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_34),
            .tdata_in(tp_data_out_34),
            .tready(mi_ready_34),
            .tdata_out(mi_data_34),
            .tvalid_out(mi_valid_34),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_35),
             .slave_tready(mi_ready_35), 
             .slave_tdata(mi_data_35),
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
master_interface mi_35(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_35),
            .tdata_in(tp_data_out_35),
            .tready(mi_ready_35),
            .tdata_out(mi_data_35),
            .tvalid_out(mi_valid_35),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_36),
             .slave_tready(mi_ready_36), 
             .slave_tdata(mi_data_36),
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
master_interface mi_36(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_36),
            .tdata_in(tp_data_out_36),
            .tready(mi_ready_36),
            .tdata_out(mi_data_36),
            .tvalid_out(mi_valid_36),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_37),
             .slave_tready(mi_ready_37), 
             .slave_tdata(mi_data_37),
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
master_interface mi_37(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_37),
            .tdata_in(tp_data_out_37),
            .tready(mi_ready_37),
            .tdata_out(mi_data_37),
            .tvalid_out(mi_valid_37),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_38),
             .slave_tready(mi_ready_38), 
             .slave_tdata(mi_data_38),
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
master_interface mi_38(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_38),
            .tdata_in(tp_data_out_38),
            .tready(mi_ready_38),
            .tdata_out(mi_data_38),
            .tvalid_out(mi_valid_38),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_39),
             .slave_tready(mi_ready_39), 
             .slave_tdata(mi_data_39),
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
master_interface mi_39(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_39),
            .tdata_in(tp_data_out_39),
            .tready(mi_ready_39),
            .tdata_out(mi_data_39),
            .tvalid_out(mi_valid_39),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_40),
             .slave_tready(mi_ready_40), 
             .slave_tdata(mi_data_40),
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
master_interface mi_40(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_40),
            .tdata_in(tp_data_out_40),
            .tready(mi_ready_40),
            .tdata_out(mi_data_40),
            .tvalid_out(mi_valid_40),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_41),
             .slave_tready(mi_ready_41), 
             .slave_tdata(mi_data_41),
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
master_interface mi_41(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_41),
            .tdata_in(tp_data_out_41),
            .tready(mi_ready_41),
            .tdata_out(mi_data_41),
            .tvalid_out(mi_valid_41),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_42),
             .slave_tready(mi_ready_42), 
             .slave_tdata(mi_data_42),
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
master_interface mi_42(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_42),
            .tdata_in(tp_data_out_42),
            .tready(mi_ready_42),
            .tdata_out(mi_data_42),
            .tvalid_out(mi_valid_42),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_43),
             .slave_tready(mi_ready_43), 
             .slave_tdata(mi_data_43),
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
master_interface mi_43(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_43),
            .tdata_in(tp_data_out_43),
            .tready(mi_ready_43),
            .tdata_out(mi_data_43),
            .tvalid_out(mi_valid_43),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_44),
             .slave_tready(mi_ready_44), 
             .slave_tdata(mi_data_44),
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
master_interface mi_44(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_44),
            .tdata_in(tp_data_out_44),
            .tready(mi_ready_44),
            .tdata_out(mi_data_44),
            .tvalid_out(mi_valid_44),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_45),
             .slave_tready(mi_ready_45), 
             .slave_tdata(mi_data_45),
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
master_interface mi_45(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_45),
            .tdata_in(tp_data_out_45),
            .tready(mi_ready_45),
            .tdata_out(mi_data_45),
            .tvalid_out(mi_valid_45),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_46),
             .slave_tready(mi_ready_46), 
             .slave_tdata(mi_data_46),
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
master_interface mi_46(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_46),
            .tdata_in(tp_data_out_46),
            .tready(mi_ready_46),
            .tdata_out(mi_data_46),
            .tvalid_out(mi_valid_46),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_47),
             .slave_tready(mi_ready_47), 
             .slave_tdata(mi_data_47),
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
master_interface mi_47(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_47),
            .tdata_in(tp_data_out_47),
            .tready(mi_ready_47),
            .tdata_out(mi_data_47),
            .tvalid_out(mi_valid_47),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_48),
             .slave_tready(mi_ready_48), 
             .slave_tdata(mi_data_48),
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
master_interface mi_48(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_48),
            .tdata_in(tp_data_out_48),
            .tready(mi_ready_48),
            .tdata_out(mi_data_48),
            .tvalid_out(mi_valid_48),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_49),
             .slave_tready(mi_ready_49), 
             .slave_tdata(mi_data_49),
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
master_interface mi_49(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_49),
            .tdata_in(tp_data_out_49),
            .tready(mi_ready_49),
            .tdata_out(mi_data_49),
            .tvalid_out(mi_valid_49),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_50),
             .slave_tready(mi_ready_50), 
             .slave_tdata(mi_data_50),
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
master_interface mi_50(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_50),
            .tdata_in(tp_data_out_50),
            .tready(mi_ready_50),
            .tdata_out(mi_data_50),
            .tvalid_out(mi_valid_50),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_51),
             .slave_tready(mi_ready_51), 
             .slave_tdata(mi_data_51),
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
master_interface mi_51(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_51),
            .tdata_in(tp_data_out_51),
            .tready(mi_ready_51),
            .tdata_out(mi_data_51),
            .tvalid_out(mi_valid_51),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_52),
             .slave_tready(mi_ready_52), 
             .slave_tdata(mi_data_52),
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
master_interface mi_52(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_52),
            .tdata_in(tp_data_out_52),
            .tready(mi_ready_52),
            .tdata_out(mi_data_52),
            .tvalid_out(mi_valid_52),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_53),
             .slave_tready(mi_ready_53), 
             .slave_tdata(mi_data_53),
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
master_interface mi_53(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_53),
            .tdata_in(tp_data_out_53),
            .tready(mi_ready_53),
            .tdata_out(mi_data_53),
            .tvalid_out(mi_valid_53),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_54),
             .slave_tready(mi_ready_54), 
             .slave_tdata(mi_data_54),
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
master_interface mi_54(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_54),
            .tdata_in(tp_data_out_54),
            .tready(mi_ready_54),
            .tdata_out(mi_data_54),
            .tvalid_out(mi_valid_54),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_55),
             .slave_tready(mi_ready_55), 
             .slave_tdata(mi_data_55),
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
master_interface mi_55(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_55),
            .tdata_in(tp_data_out_55),
            .tready(mi_ready_55),
            .tdata_out(mi_data_55),
            .tvalid_out(mi_valid_55),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_56),
             .slave_tready(mi_ready_56), 
             .slave_tdata(mi_data_56),
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
master_interface mi_56(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_56),
            .tdata_in(tp_data_out_56),
            .tready(mi_ready_56),
            .tdata_out(mi_data_56),
            .tvalid_out(mi_valid_56),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_57),
             .slave_tready(mi_ready_57), 
             .slave_tdata(mi_data_57),
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
master_interface mi_57(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_57),
            .tdata_in(tp_data_out_57),
            .tready(mi_ready_57),
            .tdata_out(mi_data_57),
            .tvalid_out(mi_valid_57),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_58),
             .slave_tready(mi_ready_58), 
             .slave_tdata(mi_data_58),
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
master_interface mi_58(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_58),
            .tdata_in(tp_data_out_58),
            .tready(mi_ready_58),
            .tdata_out(mi_data_58),
            .tvalid_out(mi_valid_58),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_59),
             .slave_tready(mi_ready_59), 
             .slave_tdata(mi_data_59),
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
master_interface mi_59(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_59),
            .tdata_in(tp_data_out_59),
            .tready(mi_ready_59),
            .tdata_out(mi_data_59),
            .tvalid_out(mi_valid_59),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_60),
             .slave_tready(mi_ready_60), 
             .slave_tdata(mi_data_60),
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
master_interface mi_60(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_60),
            .tdata_in(tp_data_out_60),
            .tready(mi_ready_60),
            .tdata_out(mi_data_60),
            .tvalid_out(mi_valid_60),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_61),
             .slave_tready(mi_ready_61), 
             .slave_tdata(mi_data_61),
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
master_interface mi_61(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_61),
            .tdata_in(tp_data_out_61),
            .tready(mi_ready_61),
            .tdata_out(mi_data_61),
            .tvalid_out(mi_valid_61),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_62),
             .slave_tready(mi_ready_62), 
             .slave_tdata(mi_data_62),
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
master_interface mi_62(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_62),
            .tdata_in(tp_data_out_62),
            .tready(mi_ready_62),
            .tdata_out(mi_data_62),
            .tvalid_out(mi_valid_62),
            .tstrb(),
            .tkeep(),
            .tid(),
            .tdest(),
            .tuser(),
            .tlast()
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
             .slave_tvalid(mi_valid_63),
             .slave_tready(mi_ready_63), 
             .slave_tdata(mi_data_63),
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
master_interface mi_63(
             .clk(clk),
            .reset(reset),
            .tvalid_in(tp_valid_out_63),
            .tdata_in(tp_data_out_63),
            .tready(mi_ready_63),
            .tdata_out(mi_data_63),
            .tvalid_out(mi_valid_63),
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

noc_router_adapter_block noc_router_adapter_block_64(
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

slave_interface si_64(
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

traffic_processor tp_64(
	.clk(clk),
	.reset(reset),
	.tdata_in(si_last_data_out),
	.tvalid_in(si_last_valid_out),
	.tdata_out(data_out),
	.tvalid_out()
);

endmodule