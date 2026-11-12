//////////////////////////////////////////////////////////////////////////////
// Author: Tanmay Anand
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// A Microsoft Brainwave Like Design. Uses fixed point precision.
//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// THIS FILE WAS AUTOMATICALLY GENERATED FROM includes.v.mako
// DO NOT EDIT
////////////////////////////////////////////////////////////////////////////////

`define IN_PRECISION 8
`define OUT_PRECISION 8

`define NUM_TILES 4

`define NUM_LDPES 16
`define DSPS_PER_LDPE 8
`define DSPS_PER_SUB_LDPE 8
`define SUB_LDPES_PER_LDPE (`DSPS_PER_LDPE/`DSPS_PER_SUB_LDPE)

`define MULTS_PER_DSP 2
`define DSP_X_AVA_INPUT_WIDTH 18
`define LDPE_X_AVA_INPUT_WIDTH (`DSP_X_AVA_INPUT_WIDTH * `DSPS_PER_LDPE)
`define DSP_Y_AVA_INPUT_WIDTH 19
`define LDPE_Y_AVA_INPUT_WIDTH (`DSP_Y_AVA_INPUT_WIDTH * `DSPS_PER_LDPE)

`define DSP_AVA_OUTPUT_WIDTH 37
`define LDPE_AVA_OUTPUT_WIDTH `DSP_AVA_OUTPUT_WIDTH

`define DSP_USED_INPUT_WIDTH `IN_PRECISION
`define LDPE_USED_INPUT_WIDTH (`DSP_USED_INPUT_WIDTH * `DSPS_PER_LDPE)
`define SUB_LDPE_USED_INPUT_WIDTH (`DSP_USED_INPUT_WIDTH * `DSPS_PER_SUB_LDPE)
`define DSP_X_ZERO_PAD_INPUT_WIDTH (`DSP_X_AVA_INPUT_WIDTH - `DSP_USED_INPUT_WIDTH)
`define DSP_Y_ZERO_PAD_INPUT_WIDTH (`DSP_Y_AVA_INPUT_WIDTH - `DSP_USED_INPUT_WIDTH)

`define DSP_USED_OUTPUT_WIDTH 32
`define LDPE_USED_OUTPUT_WIDTH `DSP_USED_OUTPUT_WIDTH
`define DSP_ZERO_PAD_OUTPUT_WIDTH (`DSP_AVA_OUTPUT_WIDTH - `DSP_USED_OUTPUT_WIDTH)

`define LDPES_PER_MRF 1
`define DSPS_PER_MRF (`DSPS_PER_LDPE * `LDPES_PER_MRF)
`define MAT_BRAM_AWIDTH 10
`define MAT_BRAM_DWIDTH 16
`define MAT_BRAMS_PER_MRF_SUBSET 8
`define MRF_AWIDTH (`MAT_BRAM_AWIDTH)
`define MRF_DWIDTH (`MAT_BRAM_DWIDTH * `MAT_BRAMS_PER_MRF_SUBSET)

`define ORF_DWIDTH 128 //128

`define MAX_VRF_DWIDTH 128
`define DRAM_DWIDTH `VRF_DWIDTH //This is the max of mrf, orf and vrf widths
`define DRAM_AWIDTH `MRF_AWIDTH

`define LDPES_PER_VRF 1
`define DSPS_PER_VRF (`DSPS_PER_LDPE * `LDPES_PER_VRF)
`define VEC_BRAM_AWIDTH 10
`define VEC_BRAM_DWIDTH 16
`define BRAMS_PER_VRF 8
`define VRF_AWIDTH `VEC_BRAM_AWIDTH
`define VRF_DWIDTH (`VEC_BRAM_DWIDTH * `BRAMS_PER_VRF)

`define LDPES_PER_ORF 1
`define OUTPUTS_PER_LDPE 1
`define OUT_BRAM_AWIDTH 10
`define OUT_BRAM_DWIDTH 16
`define ORF_AWIDTH `OUT_BRAM_AWIDTH
`define OUT_DWIDTH 8
//`define ORF_DWIDTH `OUT_DWIDTH*`NUM_LDPES


`define DESIGN_SIZE `NUM_LDPES
`define DWIDTH `OUT_PRECISION
`define MASK_WIDTH `OUT_PRECISION

`define ACTIVATION 2'b00
`define ELT_WISE_MULTIPLY 2'b10
`define ELT_WISE_ADD 2'b01
`define BYPASS 2'b11

`define ADD_LATENCY 1
`define LOG_ADD_LATENCY 1
`define MUL_LATENCY 1
`define LOG_MUL_LATENCY 1 
`define ACTIVATION_LATENCY 1
`define TANH_LATENCY `ACTIVATION_LATENCY+1


`define RELU 2'b00
`define TANH 2'b01
`define SIGM 2'b10
//OPCODES

`define V_RD 0
`define V_WR 1
`define M_RD 2
`define M_WR 3
`define MV_MUL 4
`define VV_ADD 5
`define VV_SUB 6 //QUESTIONED
`define VV_PASS 7
`define VV_MUL 8
`define V_RELU 9
`define V_SIGM 10
`define V_TANH 11
`define END_CHAIN 12

//MEM_IDS

`define VRF_0 0
`define VRF_1 1
`define VRF_2 2
`define VRF_3 3

`define VRF_4 4
`define VRF_5 5
`define VRF_6 6
`define VRF_7 7
`define VRF_MUXED 8
`define DRAM_MEM_ID 9
`define MFU_0_DSTN_ID 10
`define MFU_1_DSTN_ID 11


`define MRF_0 0
`define MRF_1 1
`define MRF_2 2
`define MRF_3 3
`define MRF_4 4
`define MRF_5 5
`define MRF_6 6
`define MRF_7 7
`define MRF_8 8
`define MRF_9 9
`define MRF_10 10
`define MRF_11 11
`define MRF_12 12
`define MRF_13 13
`define MRF_14 14
`define MRF_15 15
`define MRF_16 16
`define MRF_17 17
`define MRF_18 18
`define MRF_19 19
`define MRF_20 20
`define MRF_21 21
`define MRF_22 22
`define MRF_23 23
`define MRF_24 24
`define MRF_25 25
`define MRF_26 26
`define MRF_27 27
`define MRF_28 28
`define MRF_29 29
`define MRF_30 30
`define MRF_31 31
`define MRF_32 32
`define MRF_33 33
`define MRF_34 34
`define MRF_35 35
`define MRF_36 36
`define MRF_37 37
`define MRF_38 38
`define MRF_39 39
`define MRF_40 40
`define MRF_41 41
`define MRF_42 42
`define MRF_43 43
`define MRF_44 44
`define MRF_45 45
`define MRF_46 46
`define MRF_47 47
`define MRF_48 48
`define MRF_49 49
`define MRF_50 50
`define MRF_51 51
`define MRF_52 52
`define MRF_53 53
`define MRF_54 54
`define MRF_55 55
`define MRF_56 56
`define MRF_57 57
`define MRF_58 58
`define MRF_59 59
`define MRF_60 60
`define MRF_61 61
`define MRF_62 62
`define MRF_63 63

`define MFU_0 0
`define MFU_1 1

`define INSTR_MEM_AWIDTH 10

`define NUM_MVM_CYCLES 11

`define OPCODE_WIDTH 4 
`define TARGET_OP_WIDTH 7

`define INSTR_WIDTH `OPCODE_WIDTH+`TARGET_OP_WIDTH+`DRAM_AWIDTH+`TARGET_OP_WIDTH+`VRF_AWIDTH + `VRF_AWIDTH
////////////////////////////////////////////////////////////////////////////////
// THIS FILE WAS AUTOMATICALLY GENERATED FROM npu.v.mako
// DO NOT EDIT
////////////////////////////////////////////////////////////////////////////////


module NPU(
    input reset_npu,
    input[`INSTR_WIDTH-1:0] instruction,
    input[`DRAM_DWIDTH-1:0] input_data_DRAM,
    output [`DRAM_DWIDTH-1:0] output_data_DRAM,
    output [`DRAM_AWIDTH-1:0] dram_addr,
    output dram_write_enable,
    output get_instr,
    output[`INSTR_MEM_AWIDTH-1:0] get_instr_addr,
    //WRITE DOCUMENTATION EXPLAINING HOW MANY PORTS EACH VRF,MRF, ORF HAS and WHERE IS IT CONNECTED TO
    input clk
);
    wire[`ORF_DWIDTH-1:0] output_final_stage;
    
   
    wire start_mv_mul_signal;
    wire start_mfu_0_signal;
    wire start_mfu_1_signal;
    

    //SAME SIGNAL FOR ALL THE TILES AS PARALLEL EXECUTION OF TILES IS REQUIRED
    reg[`NUM_LDPES-1:0] start_tile_with_single_cyc_latency;
    reg[`NUM_LDPES-1:0] reset_tile_with_single_cyc_latency;
    //

    wire [`MAX_VRF_DWIDTH-1:0] vrf_in_data;
    wire[`VRF_AWIDTH-1:0] vrf_addr_wr;
    wire[`VRF_AWIDTH-1:0] vrf_addr_read;
    wire [`MRF_DWIDTH*`NUM_LDPES*`NUM_TILES-1:0] mrf_in_data;
    
    
    //MRF SIGNALS
    wire[`NUM_LDPES*`NUM_TILES-1:0] mrf_we;
    wire [`MRF_AWIDTH*`NUM_LDPES*`NUM_TILES-1:0] mrf_addr_wr;
    //
    
    //FINAL STAGE OUTPUT SIGNALS
    wire[`ORF_DWIDTH-1:0] result_mvm; 
    //reg[`ORF_AWIDTH-1:0] result_addr_mvu_orf;
    
    //wire orf_addr_increment;
  
    //
    wire[`VRF_DWIDTH-1:0] vrf_mvu_out_0;
    wire vrf_mvu_wr_enable_0;
    wire vrf_mvu_readn_enable_0;
    wire[`VRF_DWIDTH-1:0] vrf_mvu_out_1;
    wire vrf_mvu_wr_enable_1;
    wire vrf_mvu_readn_enable_1;
    wire[`VRF_DWIDTH-1:0] vrf_mvu_out_2;
    wire vrf_mvu_wr_enable_2;
    wire vrf_mvu_readn_enable_2;
    wire[`VRF_DWIDTH-1:0] vrf_mvu_out_3;
    wire vrf_mvu_wr_enable_3;
    wire vrf_mvu_readn_enable_3;
    
    wire done_mvm; //CHANGES THE REST STATE OF INSTR DECODER
    wire out_data_available_mvm;

    wire[`NUM_TILES*`NUM_LDPES-1:0] mrf_we_for_dram;
    wire [`NUM_TILES*`MRF_AWIDTH*`NUM_LDPES-1:0] mrf_addr_for_dram;
    wire [`NUM_TILES*`MRF_DWIDTH*`NUM_LDPES-1:0] mrf_outa_to_dram;

    MVU mvm_unit (
    .clk(clk),
    .start(start_tile_with_single_cyc_latency),
    .reset(reset_tile_with_single_cyc_latency),
    
    .vrf_wr_addr(vrf_addr_wr),
    .vrf_read_addr(vrf_addr_read),
    .vec(vrf_in_data[`VRF_DWIDTH-1:0]),
    .vrf_data_out_tile_0(vrf_mvu_out_0), //WITH TAG
    .vrf_wr_enable_tile_0(vrf_mvu_wr_enable_0), //WITH TAG
    .vrf_readn_enable_tile_0(vrf_mvu_readn_enable_0), //WITH TAG
    .vrf_data_out_tile_1(vrf_mvu_out_1), //WITH TAG
    .vrf_wr_enable_tile_1(vrf_mvu_wr_enable_1), //WITH TAG
    .vrf_readn_enable_tile_1(vrf_mvu_readn_enable_1), //WITH TAG
    .vrf_data_out_tile_2(vrf_mvu_out_2), //WITH TAG
    .vrf_wr_enable_tile_2(vrf_mvu_wr_enable_2), //WITH TAG
    .vrf_readn_enable_tile_2(vrf_mvu_readn_enable_2), //WITH TAG
    .vrf_data_out_tile_3(vrf_mvu_out_3), //WITH TAG
    .vrf_wr_enable_tile_3(vrf_mvu_wr_enable_3), //WITH TAG
    .vrf_readn_enable_tile_3(vrf_mvu_readn_enable_3), //WITH TAG
    
    .mrf_in(mrf_in_data),
    .mrf_we(mrf_we),  //WITH TAG 
    .mrf_addr(mrf_addr_wr),

    .mrf_we_for_dram(mrf_we_for_dram),
    .mrf_addr_for_dram(mrf_addr_for_dram),
    .mrf_outa_to_dram(mrf_outa_to_dram),

    .out_data_available(out_data_available_mvm),
    .mvm_result(result_mvm) //WITH TAG
    );
   
    assign done_mvm = out_data_available_mvm;
    
    reg[3:0] num_cycles_mvm;
    
 
    //*******************************************************************************
    
    wire in_data_available_mfu_0;
    reg reset_mfu_0_with_single_cyc_latency;
    wire out_data_available_mfu_0;
    wire done_mfu_0;
    
    wire in_data_available_mfu_1;
    reg reset_mfu_1_with_single_cyc_latency;
    wire out_data_available_mfu_1;
    wire done_mfu_1;
    
    wire[1:0] activation;
    wire[1:0] operation;

    //MFU VRF WIRES ****************************************************************
    //wire[`VRF_AWIDTH-1:0] vrf_mfu_addr_read_add_0;
    
    //MFU - STAGE 0 VRF SIGNALS 
    wire[`ORF_DWIDTH-1:0] vrf_mfu_out_data_add_0;
    wire vrf_mfu_readn_enable_add_0;
    wire vrf_mfu_wr_enable_add_0;
    wire[`ORF_AWIDTH-1:0] vrf_mfu_addr_read_add_0;
    wire[`ORF_AWIDTH-1:0] vrf_mfu_addr_wr_add_0;

    wire[`ORF_DWIDTH-1:0] vrf_mfu_out_data_mul_0;
    wire vrf_mfu_readn_enable_mul_0;
    wire vrf_mfu_wr_enable_mul_0;
    wire[`ORF_AWIDTH-1:0] vrf_mfu_addr_read_mul_0;
    wire[`ORF_AWIDTH-1:0] vrf_mfu_addr_wr_mul_0;
    
    //wire[`ORF_AWIDTH-1:0] vrf_mfu_addr_read_add_1;
    
    //MFU - STAGE 1 VRF SIGNALS 

    wire[`ORF_DWIDTH-1:0] vrf_mfu_out_data_add_1;
    wire vrf_mfu_readn_enable_add_1;
    wire vrf_mfu_wr_enable_add_1;
    wire[`ORF_AWIDTH-1:0] vrf_mfu_addr_read_add_1;
    wire[`ORF_AWIDTH-1:0] vrf_mfu_addr_wr_add_1;

    wire[`ORF_DWIDTH-1:0] vrf_mfu_out_data_mul_1;
    wire vrf_mfu_readn_enable_mul_1;
    wire vrf_mfu_wr_enable_mul_1;
    wire[`ORF_AWIDTH-1:0] vrf_mfu_addr_read_mul_1;
    wire[`ORF_AWIDTH-1:0] vrf_mfu_addr_wr_mul_1;
    
    wire[`TARGET_OP_WIDTH-1:0] dstn_id;
    wire[`NUM_LDPES*`OUT_DWIDTH-1:0] output_mvu_stage;
    //************************************************************

    assign output_mvu_stage = result_mvm;
    
    //************** INTER MFU MVU DATAPATH SIGNALS *************************************************
    reg[`ORF_DWIDTH-1:0] output_mvu_stage_buffer;
    reg[`ORF_DWIDTH-1:0] output_mfu_stage_0_buffer;
    
    wire[`ORF_DWIDTH-1:0] primary_in_data_mfu_stage_0;
    wire[`ORF_DWIDTH-1:0] primary_in_data_mfu_stage_1;
    
    
    wire[`NUM_LDPES*`OUT_DWIDTH-1:0] output_mfu_stage_0;
    wire[`NUM_LDPES*`OUT_DWIDTH-1:0] output_mfu_stage_1;
    
    always@(posedge clk) begin
        if((dstn_id==`MFU_0_DSTN_ID) && done_mvm==1'b1) begin
            output_mvu_stage_buffer <= output_mvu_stage;
        end
    end
    
    //CHECK THIS LOGIC CAREFULLY *****************************************************************
    always@(posedge clk) begin                          //FIRST BYPASS MUXING
        //$display("%h", vrf_mvu_wr_addr_0);
        if((dstn_id==`MFU_1_DSTN_ID) && (done_mfu_0 || done_mvm)) begin
            output_mfu_stage_0_buffer <= (done_mfu_0) ? output_mfu_stage_0 : output_mvu_stage;
        end
    end
    
    assign output_final_stage = ((dstn_id!=`MFU_0_DSTN_ID) && (dstn_id!=`MFU_1_DSTN_ID)) ? 
                                (done_mfu_1 ? output_mfu_stage_1 :    //SECOND BYPASS MUXING
                                (done_mfu_0 ? output_mfu_stage_0 :
                                (done_mvm ? output_mvu_stage : 'd0))) : 'd0;
                                  
    //********************************************************************************************
    
    
    //******************************************************************************************
    wire[`ORF_DWIDTH-1:0] vrf_muxed_in_data_fake;
    //************* MUXED MVU-MFU VRF **********************************************************
    
    wire[`ORF_AWIDTH-1:0] vrf_muxed_wr_addr_dram;
    wire[`ORF_DWIDTH-1:0] vrf_muxed_in_data;
    wire vrf_muxed_wr_enable_dram;
    wire vrf_muxed_readn_enable;
    
    wire[`ORF_AWIDTH-1:0] vrf_muxed_read_addr;
    wire[`ORF_DWIDTH-1:0] vrf_muxed_out_data_dram;
    wire[`ORF_DWIDTH-1:0] vrf_muxed_out_data;
    
    
    VRF #(.VRF_DWIDTH(`ORF_DWIDTH),.VRF_AWIDTH(`ORF_AWIDTH)) vrf_muxed (
        .clk(clk),
        
        .addra(vrf_muxed_wr_addr_dram),
        .ina(vrf_in_data[`ORF_DWIDTH-1:0]),
        .wea(vrf_muxed_wr_enable_dram),
        .outa(vrf_muxed_out_data_dram),
        
        .addrb(vrf_muxed_read_addr),
        .inb(vrf_muxed_in_data_fake),
        .web(vrf_muxed_readn_enable),
        .outb(vrf_muxed_out_data) 
    );
    
    wire mvu_or_vrf_mux_select;
    assign primary_in_data_mfu_stage_0 = (mvu_or_vrf_mux_select) ? vrf_muxed_out_data : output_mvu_stage_buffer;
    
    assign primary_in_data_mfu_stage_1 = output_mfu_stage_0_buffer;
    
    //*********************************************************************************************
    
    //*********************************CONTROLLER FOR NPU*****************************************
    controller controller_for_npu(
    .clk(clk), 
    .reset_npu(reset_npu),
    .instruction(instruction),
    .get_instr(get_instr),
    .get_instr_addr(get_instr_addr),
    
    .input_data_from_dram(input_data_DRAM),
    .dram_addr_wr(dram_addr),
    .dram_write_enable(dram_write_enable),
    .output_data_to_dram(output_data_DRAM),
    
    .output_final_stage(output_final_stage),

    .start_mfu_0(start_mfu_0_signal),
    .start_mfu_1(start_mfu_1_signal),
    .start_mv_mul(start_mv_mul_signal),
    .in_data_available_mfu_0(in_data_available_mfu_0),
    .in_data_available_mfu_1(in_data_available_mfu_1),

    .activation(activation),
    .operation(operation),
    
    .vrf_addr_read(vrf_addr_read),
    .vrf_addr_wr(vrf_addr_wr),

    .vrf_out_data_mvu_0(vrf_mvu_out_0),               //MVU TILE VRF
    .vrf_readn_enable_mvu_0(vrf_mvu_readn_enable_0),
    .vrf_wr_enable_mvu_0(vrf_mvu_wr_enable_0),
    .vrf_out_data_mvu_1(vrf_mvu_out_1),               //MVU TILE VRF
    .vrf_readn_enable_mvu_1(vrf_mvu_readn_enable_1),
    .vrf_wr_enable_mvu_1(vrf_mvu_wr_enable_1),
    .vrf_out_data_mvu_2(vrf_mvu_out_2),               //MVU TILE VRF
    .vrf_readn_enable_mvu_2(vrf_mvu_readn_enable_2),
    .vrf_wr_enable_mvu_2(vrf_mvu_wr_enable_2),
    .vrf_out_data_mvu_3(vrf_mvu_out_3),               //MVU TILE VRF
    .vrf_readn_enable_mvu_3(vrf_mvu_readn_enable_3),
    .vrf_wr_enable_mvu_3(vrf_mvu_wr_enable_3),
    
    .done_mvm(done_mvm),
    .done_mfu_0(done_mfu_0),
    .done_mfu_1(done_mfu_1),
    //CHANGE INDEXING FOR VRFs--------------------------------------------
    
    .vrf_out_data_mfu_add_0(vrf_mfu_out_data_add_0),
    .vrf_readn_enable_mfu_add_0(vrf_mfu_readn_enable_add_0),
    .vrf_wr_enable_mfu_add_0(vrf_mfu_wr_enable_add_0), //MFU VRF - ADD -0
    .vrf_addr_read_mfu_add_0(vrf_mfu_addr_read_add_0),
    .vrf_addr_wr_mfu_add_0(vrf_mfu_addr_wr_add_0),


    .vrf_out_data_mfu_mul_0(vrf_mfu_out_data_mul_0),      //MFU VRF - MUL -0
    .vrf_readn_enable_mfu_mul_0(vrf_mfu_readn_enable_mul_0),
    .vrf_wr_enable_mfu_mul_0(vrf_mfu_wr_enable_mul_0),
    .vrf_addr_read_mfu_mul_0(vrf_mfu_addr_read_mul_0),
    .vrf_addr_wr_mfu_mul_0(vrf_mfu_addr_wr_mul_0),
    
    .vrf_out_data_mfu_add_1(vrf_mfu_out_data_add_1),
    .vrf_readn_enable_mfu_add_1(vrf_mfu_readn_enable_add_1),
    .vrf_wr_enable_mfu_add_1(vrf_mfu_wr_enable_add_1), //MFU VRF - ADD - 1
    .vrf_addr_read_mfu_add_1(vrf_mfu_addr_read_add_1),
    .vrf_addr_wr_mfu_add_1(vrf_mfu_addr_wr_add_1),


    .vrf_out_data_mfu_mul_1(vrf_mfu_out_data_mul_1),      //MFU VRF - MUL - 1
    .vrf_readn_enable_mfu_mul_1(vrf_mfu_readn_enable_mul_1),
    .vrf_wr_enable_mfu_mul_1(vrf_mfu_wr_enable_mul_1),
    .vrf_addr_read_mfu_mul_1(vrf_mfu_addr_read_mul_1),
    .vrf_addr_wr_mfu_mul_1(vrf_mfu_addr_wr_mul_1),
    
    //MUXED VRF---------------------------------------
    .vrf_muxed_wr_addr_dram(vrf_muxed_wr_addr_dram),
    .vrf_muxed_read_addr(vrf_muxed_read_addr),
    .vrf_muxed_out_data_dram(vrf_muxed_out_data_dram),
    .vrf_muxed_wr_enable_dram(vrf_muxed_wr_enable_dram),
    .vrf_muxed_readn_enable(vrf_muxed_readn_enable),
     //----------------------------------------------
     
    .mvu_or_vrf_mux_select(mvu_or_vrf_mux_select),
    .vrf_in_data(vrf_in_data), //common
    
    //-----------------------------------------------------------------
    
    .mrf_addr_wr(mrf_addr_wr),
    .mrf_wr_enable(mrf_we),
    .mrf_in_data(mrf_in_data),

    .mrf_we_for_dram(mrf_we_for_dram),
    .mrf_addr_for_dram(mrf_addr_for_dram),
    .mrf_outa_to_dram(mrf_outa_to_dram),
    
    //.orf_addr_increment(orf_addr_increment),
    
    .dstn_id(dstn_id)
    );
    //***************************************************************************
    
    //DELAYS START SIGNALS OF MVU TILE BY ONE CYCLE TO AVOID HIGH FANOUT AND ARITHEMETIC OF DONT CARES ***********
    always@(posedge clk) begin
        if(start_mv_mul_signal==1'b1) begin
            start_tile_with_single_cyc_latency<={`NUM_LDPES{1'b1}};
            reset_tile_with_single_cyc_latency<={`NUM_LDPES{1'b0}};
        end
        else begin
            start_tile_with_single_cyc_latency<={`NUM_LDPES{1'b0}};
            reset_tile_with_single_cyc_latency<={`NUM_LDPES{1'b1}};
        end
    end
    
    always@(*) begin
        if(start_mfu_0_signal==1'b1) begin
            reset_mfu_0_with_single_cyc_latency<=1'b0;
        end
        else begin
            reset_mfu_0_with_single_cyc_latency<=1'b1;
        end
    end
    
    always@(*) begin
        if(start_mfu_1_signal==1'b1) begin
            reset_mfu_1_with_single_cyc_latency<=1'b0;
        end
        else begin
            reset_mfu_1_with_single_cyc_latency<=1'b1;
        end
    end
    
    
    //*********************************************************************************************
    wire out_data_available_0;
   //assign out_data_available_0 = done_mfu_0;
   MFU mfu_stage_0( 
    .activation_type(activation),
    .operation(operation),
    .in_data_available(in_data_available_mfu_0),
    
    .vrf_addr_read_add(vrf_mfu_addr_read_add_0),
    .vrf_addr_wr_add(vrf_mfu_addr_wr_add_0),
    .vrf_readn_enable_add(vrf_mfu_readn_enable_add_0),
    .vrf_wr_enable_add(vrf_mfu_wr_enable_add_0),

    .vrf_addr_read_mul(vrf_mfu_addr_read_mul_1),
    .vrf_addr_wr_mul(vrf_mfu_addr_wr_mul_1),
    .vrf_readn_enable_mul(vrf_mfu_readn_enable_mul_0),
    .vrf_wr_enable_mul(vrf_mfu_wr_enable_mul_0),
    
    .primary_inp(primary_in_data_mfu_stage_0),
    .secondary_inp(vrf_in_data[`ORF_DWIDTH-1:0]),
    .out_data(output_mfu_stage_0),
    .out_data_available(out_data_available_0),
    .done(done_mfu_0),
    .clk(clk),
    
    //VRF OUT SIGNALS
    .out_vrf_add(vrf_mfu_out_data_add_0),
    .out_vrf_mul(vrf_mfu_out_data_mul_0),
    
    .reset(reset_mfu_0_with_single_cyc_latency)
    );
    
    //*************************************************************************
    //MFU STAGE - 2
    wire out_data_available_1;
    //assign out_data_available_1 = done_mfu_1;

    MFU mfu_stage_1( 
    .activation_type(activation),
    .operation(operation),
    .in_data_available(in_data_available_mfu_1),
    
    //VRF IO SIGNALS FOR ELTWISE-ADD
    .vrf_addr_read_add(vrf_mfu_addr_read_add_1),
    .vrf_addr_wr_add(vrf_mfu_addr_wr_add_1),
    .vrf_readn_enable_add(vrf_mfu_readn_enable_add_1),
    .vrf_wr_enable_add(vrf_mfu_wr_enable_add_1),

    .vrf_addr_read_mul(vrf_mfu_addr_read_mul_1),
    .vrf_addr_wr_mul(vrf_mfu_addr_wr_mul_1),
    .vrf_readn_enable_mul(vrf_mfu_readn_enable_mul_1),
    .vrf_wr_enable_mul(vrf_mfu_wr_enable_mul_1),
    
     //VRF IO SIGNALS FOR ELTWISE-MUL
    .primary_inp(primary_in_data_mfu_stage_1),
    .secondary_inp(vrf_in_data[`ORF_DWIDTH-1:0]),
    .out_data(output_mfu_stage_1),
    
    .out_data_available(out_data_available_mfu_1),
    .done(done_mfu_1),
    .clk(clk),
    
    //VRF OUT SIGNAL
    .out_vrf_add(vrf_mfu_out_data_add_1),
    .out_vrf_mul(vrf_mfu_out_data_mul_1),
    
    .reset(reset_mfu_1_with_single_cyc_latency)
    );
    
    //*************************************************************************
    
    //************BYPASS MUXING LOGIC *****************************************


endmodule
////////////////////////////////////////////////////////////////////////////////
// THIS FILE WAS AUTOMATICALLY GENERATED FROM controller.v.mako
// DO NOT EDIT
////////////////////////////////////////////////////////////////////////////////



module controller( 

    input clk,
    input reset_npu,
    input done_mvm,
    input done_mfu_0,
    input done_mfu_1,
    
    
    input[`INSTR_WIDTH-1:0] instruction,
    output reg get_instr,
    output reg[`INSTR_MEM_AWIDTH-1:0] get_instr_addr,
    
    input[`DRAM_DWIDTH-1:0] input_data_from_dram,
    input[`ORF_DWIDTH-1:0] output_final_stage, 
    output reg[`DRAM_AWIDTH-1:0] dram_addr_wr,
    output reg dram_write_enable,
    output reg [`DRAM_DWIDTH-1:0] output_data_to_dram,

    //output reg start_mvu,
    output reg start_mv_mul,
    output reg start_mfu_0,
    output reg start_mfu_1,
    //output reg reset_mvu,
    output reg in_data_available_mfu_0,
    output reg in_data_available_mfu_1,
    
    output reg[1:0] activation,
    output reg[1:0] operation,

    //FOR MVU IO

    input[`VRF_DWIDTH-1:0] vrf_out_data_mvu_0,
    output reg vrf_readn_enable_mvu_0,
    output reg vrf_wr_enable_mvu_0,


    input[`VRF_DWIDTH-1:0] vrf_out_data_mvu_1,
    output reg vrf_readn_enable_mvu_1,
    output reg vrf_wr_enable_mvu_1,


    input[`VRF_DWIDTH-1:0] vrf_out_data_mvu_2,
    output reg vrf_readn_enable_mvu_2,
    output reg vrf_wr_enable_mvu_2,


    input[`VRF_DWIDTH-1:0] vrf_out_data_mvu_3,
    output reg vrf_readn_enable_mvu_3,
    output reg vrf_wr_enable_mvu_3,

    
    output reg[`VRF_AWIDTH-1:0] vrf_addr_read,
    output reg[`VRF_AWIDTH-1:0] vrf_addr_wr, //*********************

    //FOR MFU STAGE -0
    input[`ORF_DWIDTH-1:0] vrf_out_data_mfu_add_0,
    output reg vrf_readn_enable_mfu_add_0,
    output reg vrf_wr_enable_mfu_add_0,
    output reg[`ORF_AWIDTH-1:0] vrf_addr_read_mfu_add_0,
    output reg[`ORF_AWIDTH-1:0] vrf_addr_wr_mfu_add_0,
    
    input[`ORF_DWIDTH-1:0] vrf_out_data_mfu_mul_0,
    output reg vrf_readn_enable_mfu_mul_0,
    output reg vrf_wr_enable_mfu_mul_0,
    output reg[`ORF_AWIDTH-1:0] vrf_addr_read_mfu_mul_0,
    output reg[`ORF_AWIDTH-1:0] vrf_addr_wr_mfu_mul_0,
    //
    
    //FOR MFU STAGE -1 
    input[`ORF_DWIDTH-1:0] vrf_out_data_mfu_add_1,
    output reg vrf_readn_enable_mfu_add_1,
    output reg vrf_wr_enable_mfu_add_1,
    output reg[`ORF_AWIDTH-1:0] vrf_addr_read_mfu_add_1,
    output reg[`ORF_AWIDTH-1:0] vrf_addr_wr_mfu_add_1,
    
    input[`ORF_DWIDTH-1:0] vrf_out_data_mfu_mul_1,
    output reg vrf_readn_enable_mfu_mul_1,
    output reg vrf_wr_enable_mfu_mul_1,
    output reg[`ORF_AWIDTH-1:0] vrf_addr_read_mfu_mul_1,
    output reg[`ORF_AWIDTH-1:0] vrf_addr_wr_mfu_mul_1,
    
    //VRF MUXED 
    input[`ORF_DWIDTH-1:0] vrf_muxed_out_data_dram,
    output reg[`ORF_AWIDTH-1:0] vrf_muxed_wr_addr_dram,
    output reg[`ORF_AWIDTH-1:0] vrf_muxed_read_addr,
    output reg vrf_muxed_wr_enable_dram,
    output reg vrf_muxed_readn_enable,
    //

    output reg[`MAX_VRF_DWIDTH-1:0] vrf_in_data,
    
    output mvu_or_vrf_mux_select,

    //MRF IO PORTS
    output reg[`MRF_AWIDTH*`NUM_LDPES*`NUM_TILES-1:0] mrf_addr_wr,
    output reg[`NUM_LDPES*`NUM_TILES-1:0] mrf_wr_enable, //NOTE: LOG(NUM_LDPES) = TARGET_OP_WIDTH
    output reg[`MRF_DWIDTH*`NUM_LDPES*`NUM_TILES-1:0] mrf_in_data,
    
    output reg[`NUM_TILES*`NUM_LDPES-1:0] mrf_we_for_dram,
    output reg [`NUM_TILES*`MRF_AWIDTH*`NUM_LDPES-1:0] mrf_addr_for_dram,
    input [`NUM_TILES*`MRF_DWIDTH*`NUM_LDPES-1:0] mrf_outa_to_dram,
    //
    
    //BYPASS SIGNALS
    output[`TARGET_OP_WIDTH-1:0] dstn_id
);

    wire[`OPCODE_WIDTH-1:0] opcode;
    wire[`VRF_AWIDTH-1:0] op1_address;
    wire[`VRF_AWIDTH-1:0] op2_address;
    wire[`VRF_AWIDTH-1:0] dstn_address;
    wire[`TARGET_OP_WIDTH-1:0] src1_id;
    //wire[`TARGET_OP_WIDTH-1:0] dstn_id;
    
    reg[1:0] state;
    
    //NOTE - CORRECT NAMING FOR OPERANDS AND EXTRACTION SCHEME FOR YOUR PARTS OF INSTRUCTION
    assign op1_address = instruction[3*`VRF_AWIDTH+(`TARGET_OP_WIDTH)-1:(2*`VRF_AWIDTH) +(`TARGET_OP_WIDTH)];
    assign op2_address = instruction[2*`VRF_AWIDTH+`TARGET_OP_WIDTH-1:`VRF_AWIDTH+`TARGET_OP_WIDTH];
    assign dstn_address = instruction[`VRF_AWIDTH-1:0];
    assign opcode = instruction[`INSTR_WIDTH-1:`INSTR_WIDTH-`OPCODE_WIDTH];
    assign src1_id = instruction[3*`VRF_AWIDTH+2*`TARGET_OP_WIDTH-1:3*`VRF_AWIDTH+`TARGET_OP_WIDTH]; //or can be called mem_id
    assign dstn_id = instruction[`VRF_AWIDTH+`TARGET_OP_WIDTH-1:`VRF_AWIDTH];//LSB for dram_write bypass

    assign mvu_or_vrf_mux_select = (op2_address!={`VRF_AWIDTH{1'b0}}); //UNUSED BIT FOR MFU OPERATIONS


    //TODO - MAKE THIS SEQUENTIAL LOGIC - DONE
    always@(posedge clk) begin

    if(reset_npu == 1'b1) begin
          //reset_mvu<=1'b1;
          //start_mvu<=1'b0;
          get_instr<=1'b0;
          
          get_instr_addr<=0;
          
          start_mv_mul <= 1'b0;
    
          in_data_available_mfu_0 <= 1'b0;
          start_mfu_0 <= 1'b0;
          
          in_data_available_mfu_1 <= 1'b0;
          start_mfu_1 <= 1'b0;
          dram_write_enable <= 1'b0;
          mrf_wr_enable<='b0;


          vrf_wr_enable_mvu_0<=1'b0;
          vrf_readn_enable_mvu_0 <= 1'b0;


          vrf_wr_enable_mvu_1<=1'b0;
          vrf_readn_enable_mvu_1 <= 1'b0;


          vrf_wr_enable_mvu_2<=1'b0;
          vrf_readn_enable_mvu_2 <= 1'b0;


          vrf_wr_enable_mvu_3<=1'b0;
          vrf_readn_enable_mvu_3 <= 1'b0;


          vrf_wr_enable_mfu_add_0 <= 1'b0;
          vrf_wr_enable_mfu_mul_0 <= 1'b0;
          vrf_wr_enable_mfu_add_1 <= 1'b0;
          vrf_wr_enable_mfu_mul_1 <= 1'b0;
   
          dram_addr_wr<=10'd0;
          vrf_addr_wr <= 10'd0;
          //vrf_addr_wr_mvu_1 <= 0;
          vrf_addr_wr_mfu_add_0 <= 10'd0;
          vrf_addr_wr_mfu_mul_0 <= 10'd0;
          vrf_addr_wr_mfu_add_1 <= 10'd0;
          vrf_addr_wr_mfu_mul_1 <= 10'd0;
          
          vrf_addr_read <= 10'd0;
          //vrf_addr_read_mvu_1 <= 0;
          vrf_addr_read_mfu_add_0 <= 10'd0;
          vrf_addr_read_mfu_mul_0 <= 10'd0;
          vrf_addr_read_mfu_add_1 <= 10'd0;
          vrf_addr_read_mfu_mul_1 <= 10'd0;
          
        
           //vrf_muxed_wr_addr_dram <= 0;
           //vrf_muxed_read_addr <= 0;
           vrf_muxed_wr_enable_dram <= 1'b0;
           vrf_muxed_readn_enable <= 1'b0;
    
        //  orf_addr_increment<=1'b0;
          
          mrf_addr_wr <= 1'b0;
          
          state <= 0;
    end
    else begin
        if(state==0) begin //FETCH
            get_instr <= 1'b0;
            state <= 1;
            vrf_wr_enable_mvu_0 <= 1'b0;
            vrf_wr_enable_mvu_1 <= 1'b0;
            vrf_wr_enable_mvu_2 <= 1'b0;
            vrf_wr_enable_mvu_3 <= 1'b0;
            vrf_wr_enable_mfu_add_0 <= 1'b0;
            vrf_wr_enable_mfu_mul_0 <= 1'b0;
            vrf_wr_enable_mfu_add_1 <= 1'b0;
            vrf_wr_enable_mfu_mul_1 <= 1'b0;
            vrf_muxed_wr_enable_dram <= 1'b0;
            dram_write_enable <= 1'b0;
            mrf_wr_enable <= 0;
        end
        else if(state==1) begin //DECODE
          case(opcode)
            `V_WR: begin
                state <= 2;
                get_instr<=0;
                //get_instr_addr<=get_instr_addr+1'b1;
                case(src1_id) 
                `VRF_0: begin vrf_wr_enable_mvu_0 <= 1'b0;
                vrf_addr_wr <= op1_address; 
                end
                `VRF_1: begin vrf_wr_enable_mvu_1 <= 1'b0;
                vrf_addr_wr <= op1_address; 
                end
                `VRF_2: begin vrf_wr_enable_mvu_2 <= 1'b0;
                vrf_addr_wr <= op1_address; 
                end
                `VRF_3: begin vrf_wr_enable_mvu_3 <= 1'b0;
                vrf_addr_wr <= op1_address; 
                end

                `VRF_4: begin vrf_wr_enable_mfu_add_0 <= 1'b0;
                vrf_addr_wr_mfu_add_0 <= op1_address; 
                end
                
                `VRF_5: begin vrf_wr_enable_mfu_mul_0 <= 1'b0;
                vrf_addr_wr_mfu_mul_0 <= op1_address; 
                end
                
                `VRF_6: begin vrf_wr_enable_mfu_add_1 <= 1'b0;
                vrf_addr_wr_mfu_add_1 <= op1_address; 
                end
                
                `VRF_7: begin 
                vrf_wr_enable_mfu_mul_1 <= 1'b0;
                vrf_addr_wr_mfu_mul_1 <= op1_address; 
                end
                
                `VRF_MUXED: begin 
                vrf_muxed_wr_enable_dram <= 1'b0;
                vrf_muxed_wr_addr_dram <= op1_address; 
                end
                
                default: begin 
                vrf_wr_enable_mvu_0 <= 1'bX;
                output_data_to_dram <= 'd0;
                end
    
                endcase
                
                dram_addr_wr <= dstn_address;
                dram_write_enable <= 1'b1;
            end
            `M_WR: begin
                state <= 2;
                get_instr<=0;
    
                case(src1_id) 
                `MRF_0: begin mrf_we_for_dram[0] <= 1'b0;
                mrf_addr_for_dram[1*`MRF_AWIDTH-1:0*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_1: begin mrf_we_for_dram[1] <= 1'b0;
                mrf_addr_for_dram[2*`MRF_AWIDTH-1:1*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_2: begin mrf_we_for_dram[2] <= 1'b0;
                mrf_addr_for_dram[3*`MRF_AWIDTH-1:2*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_3: begin mrf_we_for_dram[3] <= 1'b0;
                mrf_addr_for_dram[4*`MRF_AWIDTH-1:3*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_4: begin mrf_we_for_dram[4] <= 1'b0;
                mrf_addr_for_dram[5*`MRF_AWIDTH-1:4*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_5: begin mrf_we_for_dram[5] <= 1'b0;
                mrf_addr_for_dram[6*`MRF_AWIDTH-1:5*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_6: begin mrf_we_for_dram[6] <= 1'b0;
                mrf_addr_for_dram[7*`MRF_AWIDTH-1:6*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_7: begin mrf_we_for_dram[7] <= 1'b0;
                mrf_addr_for_dram[8*`MRF_AWIDTH-1:7*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_8: begin mrf_we_for_dram[8] <= 1'b0;
                mrf_addr_for_dram[9*`MRF_AWIDTH-1:8*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_9: begin mrf_we_for_dram[9] <= 1'b0;
                mrf_addr_for_dram[10*`MRF_AWIDTH-1:9*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_10: begin mrf_we_for_dram[10] <= 1'b0;
                mrf_addr_for_dram[11*`MRF_AWIDTH-1:10*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_11: begin mrf_we_for_dram[11] <= 1'b0;
                mrf_addr_for_dram[12*`MRF_AWIDTH-1:11*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_12: begin mrf_we_for_dram[12] <= 1'b0;
                mrf_addr_for_dram[13*`MRF_AWIDTH-1:12*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_13: begin mrf_we_for_dram[13] <= 1'b0;
                mrf_addr_for_dram[14*`MRF_AWIDTH-1:13*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_14: begin mrf_we_for_dram[14] <= 1'b0;
                mrf_addr_for_dram[15*`MRF_AWIDTH-1:14*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_15: begin mrf_we_for_dram[15] <= 1'b0;
                mrf_addr_for_dram[16*`MRF_AWIDTH-1:15*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_16: begin mrf_we_for_dram[16] <= 1'b0;
                mrf_addr_for_dram[17*`MRF_AWIDTH-1:16*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_17: begin mrf_we_for_dram[17] <= 1'b0;
                mrf_addr_for_dram[18*`MRF_AWIDTH-1:17*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_18: begin mrf_we_for_dram[18] <= 1'b0;
                mrf_addr_for_dram[19*`MRF_AWIDTH-1:18*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_19: begin mrf_we_for_dram[19] <= 1'b0;
                mrf_addr_for_dram[20*`MRF_AWIDTH-1:19*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_20: begin mrf_we_for_dram[20] <= 1'b0;
                mrf_addr_for_dram[21*`MRF_AWIDTH-1:20*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_21: begin mrf_we_for_dram[21] <= 1'b0;
                mrf_addr_for_dram[22*`MRF_AWIDTH-1:21*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_22: begin mrf_we_for_dram[22] <= 1'b0;
                mrf_addr_for_dram[23*`MRF_AWIDTH-1:22*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_23: begin mrf_we_for_dram[23] <= 1'b0;
                mrf_addr_for_dram[24*`MRF_AWIDTH-1:23*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_24: begin mrf_we_for_dram[24] <= 1'b0;
                mrf_addr_for_dram[25*`MRF_AWIDTH-1:24*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_25: begin mrf_we_for_dram[25] <= 1'b0;
                mrf_addr_for_dram[26*`MRF_AWIDTH-1:25*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_26: begin mrf_we_for_dram[26] <= 1'b0;
                mrf_addr_for_dram[27*`MRF_AWIDTH-1:26*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_27: begin mrf_we_for_dram[27] <= 1'b0;
                mrf_addr_for_dram[28*`MRF_AWIDTH-1:27*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_28: begin mrf_we_for_dram[28] <= 1'b0;
                mrf_addr_for_dram[29*`MRF_AWIDTH-1:28*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_29: begin mrf_we_for_dram[29] <= 1'b0;
                mrf_addr_for_dram[30*`MRF_AWIDTH-1:29*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_30: begin mrf_we_for_dram[30] <= 1'b0;
                mrf_addr_for_dram[31*`MRF_AWIDTH-1:30*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_31: begin mrf_we_for_dram[31] <= 1'b0;
                mrf_addr_for_dram[32*`MRF_AWIDTH-1:31*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_32: begin mrf_we_for_dram[32] <= 1'b0;
                mrf_addr_for_dram[33*`MRF_AWIDTH-1:32*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_33: begin mrf_we_for_dram[33] <= 1'b0;
                mrf_addr_for_dram[34*`MRF_AWIDTH-1:33*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_34: begin mrf_we_for_dram[34] <= 1'b0;
                mrf_addr_for_dram[35*`MRF_AWIDTH-1:34*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_35: begin mrf_we_for_dram[35] <= 1'b0;
                mrf_addr_for_dram[36*`MRF_AWIDTH-1:35*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_36: begin mrf_we_for_dram[36] <= 1'b0;
                mrf_addr_for_dram[37*`MRF_AWIDTH-1:36*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_37: begin mrf_we_for_dram[37] <= 1'b0;
                mrf_addr_for_dram[38*`MRF_AWIDTH-1:37*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_38: begin mrf_we_for_dram[38] <= 1'b0;
                mrf_addr_for_dram[39*`MRF_AWIDTH-1:38*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_39: begin mrf_we_for_dram[39] <= 1'b0;
                mrf_addr_for_dram[40*`MRF_AWIDTH-1:39*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_40: begin mrf_we_for_dram[40] <= 1'b0;
                mrf_addr_for_dram[41*`MRF_AWIDTH-1:40*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_41: begin mrf_we_for_dram[41] <= 1'b0;
                mrf_addr_for_dram[42*`MRF_AWIDTH-1:41*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_42: begin mrf_we_for_dram[42] <= 1'b0;
                mrf_addr_for_dram[43*`MRF_AWIDTH-1:42*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_43: begin mrf_we_for_dram[43] <= 1'b0;
                mrf_addr_for_dram[44*`MRF_AWIDTH-1:43*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_44: begin mrf_we_for_dram[44] <= 1'b0;
                mrf_addr_for_dram[45*`MRF_AWIDTH-1:44*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_45: begin mrf_we_for_dram[45] <= 1'b0;
                mrf_addr_for_dram[46*`MRF_AWIDTH-1:45*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_46: begin mrf_we_for_dram[46] <= 1'b0;
                mrf_addr_for_dram[47*`MRF_AWIDTH-1:46*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_47: begin mrf_we_for_dram[47] <= 1'b0;
                mrf_addr_for_dram[48*`MRF_AWIDTH-1:47*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_48: begin mrf_we_for_dram[48] <= 1'b0;
                mrf_addr_for_dram[49*`MRF_AWIDTH-1:48*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_49: begin mrf_we_for_dram[49] <= 1'b0;
                mrf_addr_for_dram[50*`MRF_AWIDTH-1:49*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_50: begin mrf_we_for_dram[50] <= 1'b0;
                mrf_addr_for_dram[51*`MRF_AWIDTH-1:50*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_51: begin mrf_we_for_dram[51] <= 1'b0;
                mrf_addr_for_dram[52*`MRF_AWIDTH-1:51*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_52: begin mrf_we_for_dram[52] <= 1'b0;
                mrf_addr_for_dram[53*`MRF_AWIDTH-1:52*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_53: begin mrf_we_for_dram[53] <= 1'b0;
                mrf_addr_for_dram[54*`MRF_AWIDTH-1:53*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_54: begin mrf_we_for_dram[54] <= 1'b0;
                mrf_addr_for_dram[55*`MRF_AWIDTH-1:54*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_55: begin mrf_we_for_dram[55] <= 1'b0;
                mrf_addr_for_dram[56*`MRF_AWIDTH-1:55*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_56: begin mrf_we_for_dram[56] <= 1'b0;
                mrf_addr_for_dram[57*`MRF_AWIDTH-1:56*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_57: begin mrf_we_for_dram[57] <= 1'b0;
                mrf_addr_for_dram[58*`MRF_AWIDTH-1:57*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_58: begin mrf_we_for_dram[58] <= 1'b0;
                mrf_addr_for_dram[59*`MRF_AWIDTH-1:58*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_59: begin mrf_we_for_dram[59] <= 1'b0;
                mrf_addr_for_dram[60*`MRF_AWIDTH-1:59*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_60: begin mrf_we_for_dram[60] <= 1'b0;
                mrf_addr_for_dram[61*`MRF_AWIDTH-1:60*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_61: begin mrf_we_for_dram[61] <= 1'b0;
                mrf_addr_for_dram[62*`MRF_AWIDTH-1:61*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_62: begin mrf_we_for_dram[62] <= 1'b0;
                mrf_addr_for_dram[63*`MRF_AWIDTH-1:62*`MRF_AWIDTH] <= op1_address; 
                end
                `MRF_63: begin mrf_we_for_dram[63] <= 1'b0;
                mrf_addr_for_dram[64*`MRF_AWIDTH-1:63*`MRF_AWIDTH] <= op1_address; 
                end
                default: begin mrf_we_for_dram <= 'd0;
                mrf_addr_for_dram <= 'd0;
                end
                endcase
                
                dram_addr_wr <= dstn_address;
                dram_write_enable <= 1'b1;
            end
            `V_RD: begin
                state <= 2;
                get_instr<=0;
                dram_addr_wr <= op1_address;
                dram_write_enable <= 1'b0;
                
            end
            //CHANGE NAMING CONVENTION FOR WRITE AND READ TO STORE AND LOAD
            //ADD COMMENTS FOR SRC AND DESTINATION
            `M_RD: begin
                state <= 2;
                get_instr<=0;
                dram_addr_wr <= op1_address;
                dram_write_enable <= 1'b0;
            end
            `MV_MUL: begin
              //op1_id is don't care for this instructions
    
               state <= 2;
               get_instr<=1'b0;
               start_mv_mul <= 1'b1;
               mrf_addr_wr[(1*`MRF_AWIDTH)-1:0*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(2*`MRF_AWIDTH)-1:1*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(3*`MRF_AWIDTH)-1:2*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(4*`MRF_AWIDTH)-1:3*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(5*`MRF_AWIDTH)-1:4*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(6*`MRF_AWIDTH)-1:5*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(7*`MRF_AWIDTH)-1:6*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(8*`MRF_AWIDTH)-1:7*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(9*`MRF_AWIDTH)-1:8*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(10*`MRF_AWIDTH)-1:9*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(11*`MRF_AWIDTH)-1:10*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(12*`MRF_AWIDTH)-1:11*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(13*`MRF_AWIDTH)-1:12*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(14*`MRF_AWIDTH)-1:13*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(15*`MRF_AWIDTH)-1:14*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(16*`MRF_AWIDTH)-1:15*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(17*`MRF_AWIDTH)-1:16*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(18*`MRF_AWIDTH)-1:17*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(19*`MRF_AWIDTH)-1:18*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(20*`MRF_AWIDTH)-1:19*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(21*`MRF_AWIDTH)-1:20*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(22*`MRF_AWIDTH)-1:21*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(23*`MRF_AWIDTH)-1:22*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(24*`MRF_AWIDTH)-1:23*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(25*`MRF_AWIDTH)-1:24*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(26*`MRF_AWIDTH)-1:25*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(27*`MRF_AWIDTH)-1:26*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(28*`MRF_AWIDTH)-1:27*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(29*`MRF_AWIDTH)-1:28*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(30*`MRF_AWIDTH)-1:29*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(31*`MRF_AWIDTH)-1:30*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(32*`MRF_AWIDTH)-1:31*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(33*`MRF_AWIDTH)-1:32*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(34*`MRF_AWIDTH)-1:33*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(35*`MRF_AWIDTH)-1:34*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(36*`MRF_AWIDTH)-1:35*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(37*`MRF_AWIDTH)-1:36*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(38*`MRF_AWIDTH)-1:37*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(39*`MRF_AWIDTH)-1:38*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(40*`MRF_AWIDTH)-1:39*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(41*`MRF_AWIDTH)-1:40*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(42*`MRF_AWIDTH)-1:41*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(43*`MRF_AWIDTH)-1:42*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(44*`MRF_AWIDTH)-1:43*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(45*`MRF_AWIDTH)-1:44*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(46*`MRF_AWIDTH)-1:45*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(47*`MRF_AWIDTH)-1:46*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(48*`MRF_AWIDTH)-1:47*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(49*`MRF_AWIDTH)-1:48*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(50*`MRF_AWIDTH)-1:49*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(51*`MRF_AWIDTH)-1:50*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(52*`MRF_AWIDTH)-1:51*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(53*`MRF_AWIDTH)-1:52*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(54*`MRF_AWIDTH)-1:53*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(55*`MRF_AWIDTH)-1:54*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(56*`MRF_AWIDTH)-1:55*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(57*`MRF_AWIDTH)-1:56*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(58*`MRF_AWIDTH)-1:57*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(59*`MRF_AWIDTH)-1:58*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(60*`MRF_AWIDTH)-1:59*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(61*`MRF_AWIDTH)-1:60*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(62*`MRF_AWIDTH)-1:61*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(63*`MRF_AWIDTH)-1:62*`MRF_AWIDTH] <= op1_address;
               mrf_addr_wr[(64*`MRF_AWIDTH)-1:63*`MRF_AWIDTH] <= op1_address;
               vrf_addr_read <= op2_address;  
               vrf_readn_enable_mvu_0 <= 1'b0;
               vrf_readn_enable_mvu_1 <= 1'b0;
               vrf_readn_enable_mvu_2 <= 1'b0;
               vrf_readn_enable_mvu_3 <= 1'b0;
               mrf_wr_enable <= 0;
            end
            `VV_ADD:begin
            
              //MFU_STAGE-0 DESIGNATED FOR ELTWISE ADD
              state <= 2;
              get_instr <= 1'b0;
              operation <= `ELT_WISE_ADD;      //NOTE - 2nd VRF INDEX IS FOR ADD UNITS ELT WISE
              activation <= 0;

              case(src1_id) 
              
               `VRF_4: begin 
                start_mfu_0 <= 1'b1;

                vrf_muxed_readn_enable <= 1'b0;
                vrf_muxed_read_addr <= op2_address;

                in_data_available_mfu_0 <= 1'b1;
                vrf_addr_read_mfu_add_0 <= op1_address;
                vrf_readn_enable_mfu_add_0 <= 1'b0; 
               end
              
               
               `VRF_6: begin 
                start_mfu_1 <= 1'b1;
                in_data_available_mfu_1 <= 1'b1;
                vrf_addr_read_mfu_add_1 <= op1_address;
                vrf_readn_enable_mfu_add_1 <= 1'b0; 
               end
               
               
               default: begin
                start_mfu_0 <= 1'bX;
                in_data_available_mfu_0 <= 1'b0;
                vrf_addr_read_mfu_add_0 <= 10'd0;
                vrf_readn_enable_mfu_add_0 <= 1'b0; 
                vrf_addr_read_mfu_add_1 <= 10'd0;
                vrf_readn_enable_mfu_add_1 <= 1'b0;
               end
               
             endcase

            end
            `VV_SUB:begin
            
              //MFU_STAGE-0 DESIGNATED FOR ELTWISE ADD
              state <= 2;
              get_instr<=1'b0;
              operation<=`ELT_WISE_ADD;      //NOTE - 2nd VRF INDEX IS FOR ADD UNITS ELT WISE

              activation <= 1;

              case(src1_id) 
              
               `VRF_4: begin 
                start_mfu_0 <= 1'b1;

                vrf_muxed_readn_enable <= 1'b0;
                vrf_muxed_read_addr <= op2_address;

                in_data_available_mfu_0 <= 1'b1;
                vrf_addr_read_mfu_add_0 <= op1_address;
                vrf_readn_enable_mfu_add_0 <= 1'b0; 
               end
              
               
               `VRF_6: begin 
                start_mfu_1 <= 1'b1;
                in_data_available_mfu_1 <= 1'b1;
                vrf_addr_read_mfu_add_1 <= op1_address;
                vrf_readn_enable_mfu_add_1 <= 1'b0; 
               end
               
               
               default: begin
                start_mfu_0 <= 1'bX;
                in_data_available_mfu_0 <= 1'b0;
                vrf_addr_read_mfu_add_0 <= 10'd0;
                vrf_readn_enable_mfu_add_0 <= 1'b0; 
                vrf_addr_read_mfu_add_1 <= 10'd0;
                vrf_readn_enable_mfu_add_1 <= 1'b0;
               end
               
             endcase

            end
            `VV_MUL:begin
             state <= 2;
             get_instr<=1'b0;

              operation<=`ELT_WISE_MULTIPLY;     //NOTE - 3RD VRF INDEX IS FOR ADD UNITS ELT WISE
              case(src1_id) 
              
               `VRF_5: begin 
                start_mfu_0 <= 1'b1;

                vrf_muxed_readn_enable <= 1'b0;
                vrf_muxed_read_addr <= op2_address;

                in_data_available_mfu_0 <= 1'b1;
                vrf_addr_read_mfu_mul_0 <= op1_address;
                vrf_readn_enable_mfu_mul_0 <= 1'b0; 
               end
               
               `VRF_7: begin 
                start_mfu_1 <= 1'b1;
                in_data_available_mfu_1 <= 1'b1;
                vrf_addr_read_mfu_mul_1 <= op1_address;
                vrf_readn_enable_mfu_mul_1 <= 1'b0; 
               end
  
               default: begin
                start_mfu_0 <= 1'bX;
                in_data_available_mfu_0 <= 1'b0;
                vrf_addr_read_mfu_mul_0 <= 10'd0;
                vrf_readn_enable_mfu_mul_0 <= 1'b0; 
                vrf_addr_read_mfu_mul_1 <= 10'b0;
                vrf_readn_enable_mfu_mul_1 <= 1'b0; 
               end
               
             endcase
             
            end
            `V_RELU:begin

              get_instr<=1'b0;
              case(src1_id) 
              
              `MFU_0: begin 
                start_mfu_0<=1'b1;
                in_data_available_mfu_0<=1'b1;

                vrf_muxed_readn_enable <= 1'b0;
                vrf_muxed_read_addr <= op2_address;
               end
               
               `MFU_1: begin
                 start_mfu_1<=1'b1;
                 in_data_available_mfu_1<=1'b1;
                end
                
                default: begin
                start_mfu_0<=1'bX;
                in_data_available_mfu_0<=1'bX;
                end
               
              endcase
              operation<=`ACTIVATION;
              activation<=`RELU;
              state <= 2;

            end
            `V_SIGM:begin

              get_instr<=1'b0;
              case(src1_id) 
              
              `MFU_0: begin 
                start_mfu_0<=1'b1;
                in_data_available_mfu_0<=1'b1;

                vrf_muxed_readn_enable <= 1'b0;
                vrf_muxed_read_addr <= op2_address;
               end
               
               `MFU_1: begin
                 start_mfu_1<=1'b1;
                 in_data_available_mfu_1<=1'b1;
                end
                
                default: begin
                start_mfu_0<=1'bX;
                in_data_available_mfu_0<=1'bX;
                end
                
              endcase
              operation<=`ACTIVATION;
              activation<=`SIGM;
              state <= 2;
            end
            `V_TANH:begin
            //dram_write_enable <= bypass_id[0];
              get_instr<=1'b0;
              case(src1_id) 
              
              `MFU_0: begin 
                start_mfu_0<=1'b1;
                in_data_available_mfu_0<=1'b1;

                vrf_muxed_readn_enable <= 1'b0;
                vrf_muxed_read_addr <= op2_address;
               end
               
               `MFU_1: begin
                 start_mfu_1<=1'b1;
                 in_data_available_mfu_1<=1'b1;
                end
                
                default: begin
                start_mfu_0<=1'bX;
                in_data_available_mfu_0<=1'bX;
                end
                
              endcase
              operation<=`ACTIVATION;
              activation<=`TANH;
              state <= 2;

            end
            `END_CHAIN :begin

              start_mv_mul<=1'b0;
              get_instr<=1'b0;

              in_data_available_mfu_0<=1'b0;
              start_mfu_0<=1'b0;
              
              in_data_available_mfu_1<=1'b0;
              start_mfu_1<=1'b0;
              
              mrf_wr_enable<=0;


              vrf_wr_enable_mvu_0<='b0;
              vrf_readn_enable_mvu_0 <= 'b0;


              vrf_wr_enable_mvu_1<='b0;
              vrf_readn_enable_mvu_1 <= 'b0;


              vrf_wr_enable_mvu_2<='b0;
              vrf_readn_enable_mvu_2 <= 'b0;


              vrf_wr_enable_mvu_3<='b0;
              vrf_readn_enable_mvu_3 <= 'b0;

              
              vrf_wr_enable_mfu_add_0 <= 0;
              vrf_wr_enable_mfu_mul_0 <= 0;
              vrf_wr_enable_mfu_add_1 <= 0;
              vrf_wr_enable_mfu_mul_1 <= 0;

              vrf_muxed_readn_enable <= 1'b0;
              vrf_muxed_wr_addr_dram <= 1'b0;
              
              vrf_readn_enable_mfu_add_0 <= 0;
              vrf_readn_enable_mfu_mul_0 <= 0;
              vrf_readn_enable_mfu_add_1 <= 0;
              vrf_readn_enable_mfu_mul_1 <= 0;
              
              //orf_addr_increment<=1'b0;
              mrf_addr_wr <= 0;
              dram_write_enable <=  1'b0;
              state <= 1;
            end
          endcase          
         end
         else begin //EXECUTE
         
            case(opcode) 
            `V_WR: begin
                state <= 0;
                get_instr<=1'b1;
                get_instr_addr<=get_instr_addr+1'b1;
        
                case(src1_id) 

                `VRF_0: begin 
                output_data_to_dram <= vrf_out_data_mvu_0;
                end
                `VRF_1: begin 
                output_data_to_dram <= vrf_out_data_mvu_1;
                end
                `VRF_2: begin 
                output_data_to_dram <= vrf_out_data_mvu_2;
                end
                `VRF_3: begin 
                output_data_to_dram <= vrf_out_data_mvu_3;
                end
    
                `VRF_4: begin  
                output_data_to_dram <= vrf_out_data_mfu_add_0;
                end
                
                `VRF_5: begin 
                output_data_to_dram <= vrf_out_data_mfu_mul_0;
                end
                
                `VRF_6: begin 
                    output_data_to_dram <= vrf_out_data_mfu_add_1;
                end
                
                `VRF_7: begin 
                    output_data_to_dram <= vrf_out_data_mfu_mul_1;
                end
                
               `VRF_MUXED: begin 
                    output_data_to_dram <= vrf_muxed_out_data_dram;
                end
                default: begin 
                    output_data_to_dram <= 'd0;
                end
              endcase
              
            end
            `M_WR: begin
                state <= 0;
                get_instr<=1'b1;
                get_instr_addr<=get_instr_addr+1'b1;
        
                case(src1_id) 

                `MRF_0: begin 
                output_data_to_dram <= mrf_outa_to_dram[1*`MRF_DWIDTH-1:0*`MRF_DWIDTH];
                end
                `MRF_1: begin 
                output_data_to_dram <= mrf_outa_to_dram[2*`MRF_DWIDTH-1:1*`MRF_DWIDTH];
                end
                `MRF_2: begin 
                output_data_to_dram <= mrf_outa_to_dram[3*`MRF_DWIDTH-1:2*`MRF_DWIDTH];
                end
                `MRF_3: begin 
                output_data_to_dram <= mrf_outa_to_dram[4*`MRF_DWIDTH-1:3*`MRF_DWIDTH];
                end
                `MRF_4: begin 
                output_data_to_dram <= mrf_outa_to_dram[5*`MRF_DWIDTH-1:4*`MRF_DWIDTH];
                end
                `MRF_5: begin 
                output_data_to_dram <= mrf_outa_to_dram[6*`MRF_DWIDTH-1:5*`MRF_DWIDTH];
                end
                `MRF_6: begin 
                output_data_to_dram <= mrf_outa_to_dram[7*`MRF_DWIDTH-1:6*`MRF_DWIDTH];
                end
                `MRF_7: begin 
                output_data_to_dram <= mrf_outa_to_dram[8*`MRF_DWIDTH-1:7*`MRF_DWIDTH];
                end
                `MRF_8: begin 
                output_data_to_dram <= mrf_outa_to_dram[9*`MRF_DWIDTH-1:8*`MRF_DWIDTH];
                end
                `MRF_9: begin 
                output_data_to_dram <= mrf_outa_to_dram[10*`MRF_DWIDTH-1:9*`MRF_DWIDTH];
                end
                `MRF_10: begin 
                output_data_to_dram <= mrf_outa_to_dram[11*`MRF_DWIDTH-1:10*`MRF_DWIDTH];
                end
                `MRF_11: begin 
                output_data_to_dram <= mrf_outa_to_dram[12*`MRF_DWIDTH-1:11*`MRF_DWIDTH];
                end
                `MRF_12: begin 
                output_data_to_dram <= mrf_outa_to_dram[13*`MRF_DWIDTH-1:12*`MRF_DWIDTH];
                end
                `MRF_13: begin 
                output_data_to_dram <= mrf_outa_to_dram[14*`MRF_DWIDTH-1:13*`MRF_DWIDTH];
                end
                `MRF_14: begin 
                output_data_to_dram <= mrf_outa_to_dram[15*`MRF_DWIDTH-1:14*`MRF_DWIDTH];
                end
                `MRF_15: begin 
                output_data_to_dram <= mrf_outa_to_dram[16*`MRF_DWIDTH-1:15*`MRF_DWIDTH];
                end
                `MRF_16: begin 
                output_data_to_dram <= mrf_outa_to_dram[17*`MRF_DWIDTH-1:16*`MRF_DWIDTH];
                end
                `MRF_17: begin 
                output_data_to_dram <= mrf_outa_to_dram[18*`MRF_DWIDTH-1:17*`MRF_DWIDTH];
                end
                `MRF_18: begin 
                output_data_to_dram <= mrf_outa_to_dram[19*`MRF_DWIDTH-1:18*`MRF_DWIDTH];
                end
                `MRF_19: begin 
                output_data_to_dram <= mrf_outa_to_dram[20*`MRF_DWIDTH-1:19*`MRF_DWIDTH];
                end
                `MRF_20: begin 
                output_data_to_dram <= mrf_outa_to_dram[21*`MRF_DWIDTH-1:20*`MRF_DWIDTH];
                end
                `MRF_21: begin 
                output_data_to_dram <= mrf_outa_to_dram[22*`MRF_DWIDTH-1:21*`MRF_DWIDTH];
                end
                `MRF_22: begin 
                output_data_to_dram <= mrf_outa_to_dram[23*`MRF_DWIDTH-1:22*`MRF_DWIDTH];
                end
                `MRF_23: begin 
                output_data_to_dram <= mrf_outa_to_dram[24*`MRF_DWIDTH-1:23*`MRF_DWIDTH];
                end
                `MRF_24: begin 
                output_data_to_dram <= mrf_outa_to_dram[25*`MRF_DWIDTH-1:24*`MRF_DWIDTH];
                end
                `MRF_25: begin 
                output_data_to_dram <= mrf_outa_to_dram[26*`MRF_DWIDTH-1:25*`MRF_DWIDTH];
                end
                `MRF_26: begin 
                output_data_to_dram <= mrf_outa_to_dram[27*`MRF_DWIDTH-1:26*`MRF_DWIDTH];
                end
                `MRF_27: begin 
                output_data_to_dram <= mrf_outa_to_dram[28*`MRF_DWIDTH-1:27*`MRF_DWIDTH];
                end
                `MRF_28: begin 
                output_data_to_dram <= mrf_outa_to_dram[29*`MRF_DWIDTH-1:28*`MRF_DWIDTH];
                end
                `MRF_29: begin 
                output_data_to_dram <= mrf_outa_to_dram[30*`MRF_DWIDTH-1:29*`MRF_DWIDTH];
                end
                `MRF_30: begin 
                output_data_to_dram <= mrf_outa_to_dram[31*`MRF_DWIDTH-1:30*`MRF_DWIDTH];
                end
                `MRF_31: begin 
                output_data_to_dram <= mrf_outa_to_dram[32*`MRF_DWIDTH-1:31*`MRF_DWIDTH];
                end
                `MRF_32: begin 
                output_data_to_dram <= mrf_outa_to_dram[33*`MRF_DWIDTH-1:32*`MRF_DWIDTH];
                end
                `MRF_33: begin 
                output_data_to_dram <= mrf_outa_to_dram[34*`MRF_DWIDTH-1:33*`MRF_DWIDTH];
                end
                `MRF_34: begin 
                output_data_to_dram <= mrf_outa_to_dram[35*`MRF_DWIDTH-1:34*`MRF_DWIDTH];
                end
                `MRF_35: begin 
                output_data_to_dram <= mrf_outa_to_dram[36*`MRF_DWIDTH-1:35*`MRF_DWIDTH];
                end
                `MRF_36: begin 
                output_data_to_dram <= mrf_outa_to_dram[37*`MRF_DWIDTH-1:36*`MRF_DWIDTH];
                end
                `MRF_37: begin 
                output_data_to_dram <= mrf_outa_to_dram[38*`MRF_DWIDTH-1:37*`MRF_DWIDTH];
                end
                `MRF_38: begin 
                output_data_to_dram <= mrf_outa_to_dram[39*`MRF_DWIDTH-1:38*`MRF_DWIDTH];
                end
                `MRF_39: begin 
                output_data_to_dram <= mrf_outa_to_dram[40*`MRF_DWIDTH-1:39*`MRF_DWIDTH];
                end
                `MRF_40: begin 
                output_data_to_dram <= mrf_outa_to_dram[41*`MRF_DWIDTH-1:40*`MRF_DWIDTH];
                end
                `MRF_41: begin 
                output_data_to_dram <= mrf_outa_to_dram[42*`MRF_DWIDTH-1:41*`MRF_DWIDTH];
                end
                `MRF_42: begin 
                output_data_to_dram <= mrf_outa_to_dram[43*`MRF_DWIDTH-1:42*`MRF_DWIDTH];
                end
                `MRF_43: begin 
                output_data_to_dram <= mrf_outa_to_dram[44*`MRF_DWIDTH-1:43*`MRF_DWIDTH];
                end
                `MRF_44: begin 
                output_data_to_dram <= mrf_outa_to_dram[45*`MRF_DWIDTH-1:44*`MRF_DWIDTH];
                end
                `MRF_45: begin 
                output_data_to_dram <= mrf_outa_to_dram[46*`MRF_DWIDTH-1:45*`MRF_DWIDTH];
                end
                `MRF_46: begin 
                output_data_to_dram <= mrf_outa_to_dram[47*`MRF_DWIDTH-1:46*`MRF_DWIDTH];
                end
                `MRF_47: begin 
                output_data_to_dram <= mrf_outa_to_dram[48*`MRF_DWIDTH-1:47*`MRF_DWIDTH];
                end
                `MRF_48: begin 
                output_data_to_dram <= mrf_outa_to_dram[49*`MRF_DWIDTH-1:48*`MRF_DWIDTH];
                end
                `MRF_49: begin 
                output_data_to_dram <= mrf_outa_to_dram[50*`MRF_DWIDTH-1:49*`MRF_DWIDTH];
                end
                `MRF_50: begin 
                output_data_to_dram <= mrf_outa_to_dram[51*`MRF_DWIDTH-1:50*`MRF_DWIDTH];
                end
                `MRF_51: begin 
                output_data_to_dram <= mrf_outa_to_dram[52*`MRF_DWIDTH-1:51*`MRF_DWIDTH];
                end
                `MRF_52: begin 
                output_data_to_dram <= mrf_outa_to_dram[53*`MRF_DWIDTH-1:52*`MRF_DWIDTH];
                end
                `MRF_53: begin 
                output_data_to_dram <= mrf_outa_to_dram[54*`MRF_DWIDTH-1:53*`MRF_DWIDTH];
                end
                `MRF_54: begin 
                output_data_to_dram <= mrf_outa_to_dram[55*`MRF_DWIDTH-1:54*`MRF_DWIDTH];
                end
                `MRF_55: begin 
                output_data_to_dram <= mrf_outa_to_dram[56*`MRF_DWIDTH-1:55*`MRF_DWIDTH];
                end
                `MRF_56: begin 
                output_data_to_dram <= mrf_outa_to_dram[57*`MRF_DWIDTH-1:56*`MRF_DWIDTH];
                end
                `MRF_57: begin 
                output_data_to_dram <= mrf_outa_to_dram[58*`MRF_DWIDTH-1:57*`MRF_DWIDTH];
                end
                `MRF_58: begin 
                output_data_to_dram <= mrf_outa_to_dram[59*`MRF_DWIDTH-1:58*`MRF_DWIDTH];
                end
                `MRF_59: begin 
                output_data_to_dram <= mrf_outa_to_dram[60*`MRF_DWIDTH-1:59*`MRF_DWIDTH];
                end
                `MRF_60: begin 
                output_data_to_dram <= mrf_outa_to_dram[61*`MRF_DWIDTH-1:60*`MRF_DWIDTH];
                end
                `MRF_61: begin 
                output_data_to_dram <= mrf_outa_to_dram[62*`MRF_DWIDTH-1:61*`MRF_DWIDTH];
                end
                `MRF_62: begin 
                output_data_to_dram <= mrf_outa_to_dram[63*`MRF_DWIDTH-1:62*`MRF_DWIDTH];
                end
                `MRF_63: begin 
                output_data_to_dram <= mrf_outa_to_dram[64*`MRF_DWIDTH-1:63*`MRF_DWIDTH];
                end
                default: begin 
                    output_data_to_dram <= 'd0;
                end
              endcase
              
            end
            `V_RD: begin
                state <= 0;
                get_instr<=1'b1;
                get_instr_addr<=get_instr_addr+1'b1;
                vrf_in_data <= input_data_from_dram;
                case(dstn_id) 
                  `VRF_0: begin 
                  vrf_wr_enable_mvu_0 <= 1'b1;
                  vrf_wr_enable_mvu_1 <= 1'b0;
                  vrf_wr_enable_mvu_2 <= 1'b0;
                  vrf_wr_enable_mvu_3 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_wr_enable_mfu_add_1 <= 1'b0;
                  vrf_wr_enable_mfu_mul_1 <= 1'b0;
                  vrf_muxed_wr_enable_dram <= 1'b0;
                  
                  vrf_addr_wr <= dstn_address;
                  end
                  `VRF_1: begin 
                  vrf_wr_enable_mvu_0 <= 1'b0;
                  vrf_wr_enable_mvu_1 <= 1'b1;
                  vrf_wr_enable_mvu_2 <= 1'b0;
                  vrf_wr_enable_mvu_3 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_wr_enable_mfu_add_1 <= 1'b0;
                  vrf_wr_enable_mfu_mul_1 <= 1'b0;
                  vrf_muxed_wr_enable_dram <= 1'b0;
                  
                  vrf_addr_wr <= dstn_address;
                  end
                  `VRF_2: begin 
                  vrf_wr_enable_mvu_0 <= 1'b0;
                  vrf_wr_enable_mvu_1 <= 1'b0;
                  vrf_wr_enable_mvu_2 <= 1'b1;
                  vrf_wr_enable_mvu_3 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_wr_enable_mfu_add_1 <= 1'b0;
                  vrf_wr_enable_mfu_mul_1 <= 1'b0;
                  vrf_muxed_wr_enable_dram <= 1'b0;
                  
                  vrf_addr_wr <= dstn_address;
                  end
                  `VRF_3: begin 
                  vrf_wr_enable_mvu_0 <= 1'b0;
                  vrf_wr_enable_mvu_1 <= 1'b0;
                  vrf_wr_enable_mvu_2 <= 1'b0;
                  vrf_wr_enable_mvu_3 <= 1'b1;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_wr_enable_mfu_add_1 <= 1'b0;
                  vrf_wr_enable_mfu_mul_1 <= 1'b0;
                  vrf_muxed_wr_enable_dram <= 1'b0;
                  
                  vrf_addr_wr <= dstn_address;
                  end
                  `VRF_4: begin 
                  vrf_wr_enable_mvu_0 <= 1'b0;
                  vrf_wr_enable_mvu_1 <= 1'b0;
                  vrf_wr_enable_mvu_2 <= 1'b0;
                  vrf_wr_enable_mvu_3 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b1;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_wr_enable_mfu_add_1 <= 1'b0;
                  vrf_wr_enable_mfu_mul_1 <= 1'b0;
                  vrf_muxed_wr_enable_dram <= 1'b0;
                  
                  vrf_addr_wr_mfu_add_0 <= dstn_address;
                  
                  end
                  
                  `VRF_5: begin 
                  vrf_wr_enable_mvu_0 <= 1'b0;
                  vrf_wr_enable_mvu_1 <= 1'b0;
                  vrf_wr_enable_mvu_2 <= 1'b0;
                  vrf_wr_enable_mvu_3 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b1;
                  vrf_wr_enable_mfu_add_1 <= 1'b0;
                  vrf_wr_enable_mfu_mul_1 <= 1'b0;
                  vrf_muxed_wr_enable_dram <= 1'b0;
                  
                  vrf_addr_wr_mfu_mul_0 <= dstn_address;
                  
                  end
                  
                  `VRF_6: begin 
                  vrf_wr_enable_mvu_0 <= 1'b0;
                  vrf_wr_enable_mvu_1 <= 1'b0;
                  vrf_wr_enable_mvu_2 <= 1'b0;
                  vrf_wr_enable_mvu_3 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_wr_enable_mfu_add_1 <= 1'b1;
                  vrf_wr_enable_mfu_mul_1 <= 1'b0;
                  vrf_muxed_wr_enable_dram <= 1'b0;
                  
                  vrf_addr_wr_mfu_add_1 <= dstn_address;
                  end
                  
                  `VRF_7: begin 
                  vrf_wr_enable_mvu_0 <= 1'b0;
                  vrf_wr_enable_mvu_1 <= 1'b0;
                  vrf_wr_enable_mvu_2 <= 1'b0;
                  vrf_wr_enable_mvu_3 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_wr_enable_mfu_add_1 <= 1'b0;
                  vrf_wr_enable_mfu_mul_1 <= 1'b1;
                  vrf_muxed_wr_enable_dram <= 1'b0;
                  
                  vrf_addr_wr_mfu_mul_1 <= dstn_address;
                  end
                  
                  `VRF_MUXED: begin 
                  vrf_wr_enable_mvu_0 <= 1'b0;
                  vrf_wr_enable_mvu_1 <= 1'b0;
                  vrf_wr_enable_mvu_2 <= 1'b0;
                  vrf_wr_enable_mvu_3 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_muxed_wr_enable_dram <= 1'b1;
                  
                   
                  vrf_muxed_wr_addr_dram <= dstn_address;
                  end
    
                  default: begin 
                  vrf_wr_enable_mvu_0 <= 1'bX;
                  vrf_wr_enable_mvu_1 <= 1'bX;
                  vrf_wr_enable_mvu_2 <= 1'bX;
                  vrf_wr_enable_mvu_3 <= 1'bX;
                  vrf_wr_enable_mfu_add_0 <= 1'bX;
                  vrf_wr_enable_mfu_mul_0 <= 1'bX;
                  vrf_wr_enable_mfu_add_1 <= 1'bX;
                  vrf_wr_enable_mfu_mul_1 <= 1'bX;
                  vrf_muxed_wr_enable_dram <= 1'bX;
 
                  end
                endcase
/*
                vrf_wr_enable_mvu_0 <= 1'b0;
                vrf_wr_enable_mvu_1 <= 1'b0;
                vrf_wr_enable_mvu_2 <= 1'b0;
                vrf_wr_enable_mvu_3 <= 1'b0;
                vrf_wr_enable_mfu_add_0 <= 1'b0;
                vrf_wr_enable_mfu_mul_0 <= 1'b0;
                vrf_wr_enable_mfu_add_1 <= 1'b0;
                vrf_wr_enable_mfu_mul_1 <= 1'b0;
                vrf_muxed_wr_enable_dram <= 1'b0;
*/
                
            end
            `M_RD: begin
                state <= 0;
                get_instr<=1'b1;
                get_instr_addr<=get_instr_addr+1'b1;
            
                case(dstn_id) 
                  `MRF_0: begin 
                    mrf_we_for_dram[0] <= 1;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[1*`MRF_DWIDTH-1:0*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[1*`MRF_AWIDTH-1:0*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_1: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 1;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[2*`MRF_DWIDTH-1:1*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[2*`MRF_AWIDTH-1:1*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_2: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 1;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[3*`MRF_DWIDTH-1:2*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[3*`MRF_AWIDTH-1:2*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_3: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 1;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[4*`MRF_DWIDTH-1:3*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[4*`MRF_AWIDTH-1:3*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_4: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 1;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[5*`MRF_DWIDTH-1:4*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[5*`MRF_AWIDTH-1:4*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_5: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 1;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[6*`MRF_DWIDTH-1:5*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[6*`MRF_AWIDTH-1:5*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_6: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 1;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[7*`MRF_DWIDTH-1:6*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[7*`MRF_AWIDTH-1:6*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_7: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 1;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[8*`MRF_DWIDTH-1:7*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[8*`MRF_AWIDTH-1:7*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_8: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 1;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[9*`MRF_DWIDTH-1:8*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[9*`MRF_AWIDTH-1:8*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_9: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 1;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[10*`MRF_DWIDTH-1:9*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[10*`MRF_AWIDTH-1:9*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_10: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 1;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[11*`MRF_DWIDTH-1:10*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[11*`MRF_AWIDTH-1:10*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_11: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 1;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[12*`MRF_DWIDTH-1:11*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[12*`MRF_AWIDTH-1:11*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_12: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 1;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[13*`MRF_DWIDTH-1:12*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[13*`MRF_AWIDTH-1:12*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_13: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 1;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[14*`MRF_DWIDTH-1:13*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[14*`MRF_AWIDTH-1:13*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_14: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 1;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[15*`MRF_DWIDTH-1:14*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[15*`MRF_AWIDTH-1:14*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_15: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 1;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[16*`MRF_DWIDTH-1:15*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[16*`MRF_AWIDTH-1:15*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_16: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 1;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[17*`MRF_DWIDTH-1:16*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[17*`MRF_AWIDTH-1:16*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_17: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 1;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[18*`MRF_DWIDTH-1:17*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[18*`MRF_AWIDTH-1:17*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_18: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 1;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[19*`MRF_DWIDTH-1:18*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[19*`MRF_AWIDTH-1:18*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_19: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 1;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[20*`MRF_DWIDTH-1:19*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[20*`MRF_AWIDTH-1:19*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_20: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 1;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[21*`MRF_DWIDTH-1:20*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[21*`MRF_AWIDTH-1:20*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_21: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 1;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[22*`MRF_DWIDTH-1:21*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[22*`MRF_AWIDTH-1:21*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_22: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 1;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[23*`MRF_DWIDTH-1:22*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[23*`MRF_AWIDTH-1:22*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_23: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 1;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[24*`MRF_DWIDTH-1:23*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[24*`MRF_AWIDTH-1:23*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_24: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 1;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[25*`MRF_DWIDTH-1:24*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[25*`MRF_AWIDTH-1:24*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_25: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 1;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[26*`MRF_DWIDTH-1:25*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[26*`MRF_AWIDTH-1:25*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_26: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 1;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[27*`MRF_DWIDTH-1:26*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[27*`MRF_AWIDTH-1:26*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_27: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 1;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[28*`MRF_DWIDTH-1:27*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[28*`MRF_AWIDTH-1:27*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_28: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 1;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[29*`MRF_DWIDTH-1:28*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[29*`MRF_AWIDTH-1:28*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_29: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 1;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[30*`MRF_DWIDTH-1:29*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[30*`MRF_AWIDTH-1:29*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_30: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 1;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[31*`MRF_DWIDTH-1:30*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[31*`MRF_AWIDTH-1:30*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_31: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 1;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[32*`MRF_DWIDTH-1:31*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[32*`MRF_AWIDTH-1:31*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_32: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 1;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[33*`MRF_DWIDTH-1:32*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[33*`MRF_AWIDTH-1:32*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_33: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 1;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[34*`MRF_DWIDTH-1:33*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[34*`MRF_AWIDTH-1:33*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_34: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 1;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[35*`MRF_DWIDTH-1:34*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[35*`MRF_AWIDTH-1:34*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_35: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 1;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[36*`MRF_DWIDTH-1:35*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[36*`MRF_AWIDTH-1:35*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_36: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 1;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[37*`MRF_DWIDTH-1:36*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[37*`MRF_AWIDTH-1:36*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_37: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 1;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[38*`MRF_DWIDTH-1:37*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[38*`MRF_AWIDTH-1:37*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_38: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 1;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[39*`MRF_DWIDTH-1:38*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[39*`MRF_AWIDTH-1:38*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_39: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 1;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[40*`MRF_DWIDTH-1:39*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[40*`MRF_AWIDTH-1:39*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_40: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 1;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[41*`MRF_DWIDTH-1:40*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[41*`MRF_AWIDTH-1:40*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_41: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 1;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[42*`MRF_DWIDTH-1:41*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[42*`MRF_AWIDTH-1:41*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_42: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 1;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[43*`MRF_DWIDTH-1:42*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[43*`MRF_AWIDTH-1:42*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_43: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 1;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[44*`MRF_DWIDTH-1:43*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[44*`MRF_AWIDTH-1:43*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_44: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 1;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[45*`MRF_DWIDTH-1:44*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[45*`MRF_AWIDTH-1:44*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_45: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 1;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[46*`MRF_DWIDTH-1:45*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[46*`MRF_AWIDTH-1:45*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_46: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 1;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[47*`MRF_DWIDTH-1:46*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[47*`MRF_AWIDTH-1:46*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_47: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 1;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[48*`MRF_DWIDTH-1:47*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[48*`MRF_AWIDTH-1:47*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_48: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 1;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[49*`MRF_DWIDTH-1:48*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[49*`MRF_AWIDTH-1:48*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_49: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 1;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[50*`MRF_DWIDTH-1:49*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[50*`MRF_AWIDTH-1:49*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_50: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 1;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[51*`MRF_DWIDTH-1:50*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[51*`MRF_AWIDTH-1:50*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_51: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 1;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[52*`MRF_DWIDTH-1:51*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[52*`MRF_AWIDTH-1:51*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_52: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 1;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[53*`MRF_DWIDTH-1:52*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[53*`MRF_AWIDTH-1:52*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_53: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 1;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[54*`MRF_DWIDTH-1:53*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[54*`MRF_AWIDTH-1:53*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_54: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 1;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[55*`MRF_DWIDTH-1:54*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[55*`MRF_AWIDTH-1:54*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_55: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 1;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[56*`MRF_DWIDTH-1:55*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[56*`MRF_AWIDTH-1:55*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_56: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 1;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[57*`MRF_DWIDTH-1:56*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[57*`MRF_AWIDTH-1:56*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_57: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 1;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[58*`MRF_DWIDTH-1:57*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[58*`MRF_AWIDTH-1:57*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_58: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 1;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[59*`MRF_DWIDTH-1:58*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[59*`MRF_AWIDTH-1:58*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_59: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 1;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[60*`MRF_DWIDTH-1:59*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[60*`MRF_AWIDTH-1:59*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_60: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 1;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[61*`MRF_DWIDTH-1:60*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[61*`MRF_AWIDTH-1:60*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_61: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 1;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[62*`MRF_DWIDTH-1:61*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[62*`MRF_AWIDTH-1:61*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_62: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 1;
                    mrf_we_for_dram[63] <= 0;
                    mrf_in_data[63*`MRF_DWIDTH-1:62*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[63*`MRF_AWIDTH-1:62*`MRF_AWIDTH] <= dstn_address;            
                  end
                  `MRF_63: begin 
                    mrf_we_for_dram[0] <= 0;
                    mrf_we_for_dram[1] <= 0;
                    mrf_we_for_dram[2] <= 0;
                    mrf_we_for_dram[3] <= 0;
                    mrf_we_for_dram[4] <= 0;
                    mrf_we_for_dram[5] <= 0;
                    mrf_we_for_dram[6] <= 0;
                    mrf_we_for_dram[7] <= 0;
                    mrf_we_for_dram[8] <= 0;
                    mrf_we_for_dram[9] <= 0;
                    mrf_we_for_dram[10] <= 0;
                    mrf_we_for_dram[11] <= 0;
                    mrf_we_for_dram[12] <= 0;
                    mrf_we_for_dram[13] <= 0;
                    mrf_we_for_dram[14] <= 0;
                    mrf_we_for_dram[15] <= 0;
                    mrf_we_for_dram[16] <= 0;
                    mrf_we_for_dram[17] <= 0;
                    mrf_we_for_dram[18] <= 0;
                    mrf_we_for_dram[19] <= 0;
                    mrf_we_for_dram[20] <= 0;
                    mrf_we_for_dram[21] <= 0;
                    mrf_we_for_dram[22] <= 0;
                    mrf_we_for_dram[23] <= 0;
                    mrf_we_for_dram[24] <= 0;
                    mrf_we_for_dram[25] <= 0;
                    mrf_we_for_dram[26] <= 0;
                    mrf_we_for_dram[27] <= 0;
                    mrf_we_for_dram[28] <= 0;
                    mrf_we_for_dram[29] <= 0;
                    mrf_we_for_dram[30] <= 0;
                    mrf_we_for_dram[31] <= 0;
                    mrf_we_for_dram[32] <= 0;
                    mrf_we_for_dram[33] <= 0;
                    mrf_we_for_dram[34] <= 0;
                    mrf_we_for_dram[35] <= 0;
                    mrf_we_for_dram[36] <= 0;
                    mrf_we_for_dram[37] <= 0;
                    mrf_we_for_dram[38] <= 0;
                    mrf_we_for_dram[39] <= 0;
                    mrf_we_for_dram[40] <= 0;
                    mrf_we_for_dram[41] <= 0;
                    mrf_we_for_dram[42] <= 0;
                    mrf_we_for_dram[43] <= 0;
                    mrf_we_for_dram[44] <= 0;
                    mrf_we_for_dram[45] <= 0;
                    mrf_we_for_dram[46] <= 0;
                    mrf_we_for_dram[47] <= 0;
                    mrf_we_for_dram[48] <= 0;
                    mrf_we_for_dram[49] <= 0;
                    mrf_we_for_dram[50] <= 0;
                    mrf_we_for_dram[51] <= 0;
                    mrf_we_for_dram[52] <= 0;
                    mrf_we_for_dram[53] <= 0;
                    mrf_we_for_dram[54] <= 0;
                    mrf_we_for_dram[55] <= 0;
                    mrf_we_for_dram[56] <= 0;
                    mrf_we_for_dram[57] <= 0;
                    mrf_we_for_dram[58] <= 0;
                    mrf_we_for_dram[59] <= 0;
                    mrf_we_for_dram[60] <= 0;
                    mrf_we_for_dram[61] <= 0;
                    mrf_we_for_dram[62] <= 0;
                    mrf_we_for_dram[63] <= 1;
                    mrf_in_data[64*`MRF_DWIDTH-1:63*`MRF_DWIDTH] <= input_data_from_dram;
                    mrf_addr_for_dram[64*`MRF_AWIDTH-1:63*`MRF_AWIDTH] <= dstn_address;            
                  end
                  
                  default: begin 
                    mrf_we_for_dram[0] <= 1'bX;
                    mrf_we_for_dram[1] <= 1'bX;
                    mrf_we_for_dram[2] <= 1'bX;
                    mrf_we_for_dram[3] <= 1'bX;
                    mrf_we_for_dram[4] <= 1'bX;
                    mrf_we_for_dram[5] <= 1'bX;
                    mrf_we_for_dram[6] <= 1'bX;
                    mrf_we_for_dram[7] <= 1'bX;
                    mrf_we_for_dram[8] <= 1'bX;
                    mrf_we_for_dram[9] <= 1'bX;
                    mrf_we_for_dram[10] <= 1'bX;
                    mrf_we_for_dram[11] <= 1'bX;
                    mrf_we_for_dram[12] <= 1'bX;
                    mrf_we_for_dram[13] <= 1'bX;
                    mrf_we_for_dram[14] <= 1'bX;
                    mrf_we_for_dram[15] <= 1'bX;
                    mrf_we_for_dram[16] <= 1'bX;
                    mrf_we_for_dram[17] <= 1'bX;
                    mrf_we_for_dram[18] <= 1'bX;
                    mrf_we_for_dram[19] <= 1'bX;
                    mrf_we_for_dram[20] <= 1'bX;
                    mrf_we_for_dram[21] <= 1'bX;
                    mrf_we_for_dram[22] <= 1'bX;
                    mrf_we_for_dram[23] <= 1'bX;
                    mrf_we_for_dram[24] <= 1'bX;
                    mrf_we_for_dram[25] <= 1'bX;
                    mrf_we_for_dram[26] <= 1'bX;
                    mrf_we_for_dram[27] <= 1'bX;
                    mrf_we_for_dram[28] <= 1'bX;
                    mrf_we_for_dram[29] <= 1'bX;
                    mrf_we_for_dram[30] <= 1'bX;
                    mrf_we_for_dram[31] <= 1'bX;
                    mrf_we_for_dram[32] <= 1'bX;
                    mrf_we_for_dram[33] <= 1'bX;
                    mrf_we_for_dram[34] <= 1'bX;
                    mrf_we_for_dram[35] <= 1'bX;
                    mrf_we_for_dram[36] <= 1'bX;
                    mrf_we_for_dram[37] <= 1'bX;
                    mrf_we_for_dram[38] <= 1'bX;
                    mrf_we_for_dram[39] <= 1'bX;
                    mrf_we_for_dram[40] <= 1'bX;
                    mrf_we_for_dram[41] <= 1'bX;
                    mrf_we_for_dram[42] <= 1'bX;
                    mrf_we_for_dram[43] <= 1'bX;
                    mrf_we_for_dram[44] <= 1'bX;
                    mrf_we_for_dram[45] <= 1'bX;
                    mrf_we_for_dram[46] <= 1'bX;
                    mrf_we_for_dram[47] <= 1'bX;
                    mrf_we_for_dram[48] <= 1'bX;
                    mrf_we_for_dram[49] <= 1'bX;
                    mrf_we_for_dram[50] <= 1'bX;
                    mrf_we_for_dram[51] <= 1'bX;
                    mrf_we_for_dram[52] <= 1'bX;
                    mrf_we_for_dram[53] <= 1'bX;
                    mrf_we_for_dram[54] <= 1'bX;
                    mrf_we_for_dram[55] <= 1'bX;
                    mrf_we_for_dram[56] <= 1'bX;
                    mrf_we_for_dram[57] <= 1'bX;
                    mrf_we_for_dram[58] <= 1'bX;
                    mrf_we_for_dram[59] <= 1'bX;
                    mrf_we_for_dram[60] <= 1'bX;
                    mrf_we_for_dram[61] <= 1'bX;
                    mrf_we_for_dram[62] <= 1'bX;
                    mrf_we_for_dram[63] <= 1'bX;
                  end
                  
                endcase 
            end
            default: begin
            
            if(done_mvm || done_mfu_0 || done_mfu_1) begin
                start_mv_mul <= 0;
                start_mfu_0 <= 0;
                start_mfu_1 <= 0;
                state <= 0;
                get_instr<=1'b1;
                get_instr_addr<=get_instr_addr+1'b1;
               
                case(dstn_id) 
                  `VRF_0: begin 
                  vrf_wr_enable_mvu_0 <= 1'b1;
                  vrf_wr_enable_mvu_1 <= 1'b0;
                  vrf_wr_enable_mvu_2 <= 1'b0;
                  vrf_wr_enable_mvu_3 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_wr_enable_mfu_add_1 <= 1'b0;
                  vrf_wr_enable_mfu_mul_1 <= 1'b0;
                  vrf_muxed_wr_enable_dram <= 1'b0;
                  dram_write_enable<=1'b0;
                  vrf_in_data <= output_final_stage;
                  vrf_addr_wr<=dstn_address;
                  //vrf_addr_wr_mvu_0 <= dstn_address;
                  end
                  `VRF_1: begin 
                  vrf_wr_enable_mvu_0 <= 1'b0;
                  vrf_wr_enable_mvu_1 <= 1'b1;
                  vrf_wr_enable_mvu_2 <= 1'b0;
                  vrf_wr_enable_mvu_3 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_wr_enable_mfu_add_1 <= 1'b0;
                  vrf_wr_enable_mfu_mul_1 <= 1'b0;
                  vrf_muxed_wr_enable_dram <= 1'b0;
                  dram_write_enable<=1'b0;
                  vrf_in_data <= output_final_stage;
                  vrf_addr_wr<=dstn_address;
                  //vrf_addr_wr_mvu_0 <= dstn_address;
                  end
                  `VRF_2: begin 
                  vrf_wr_enable_mvu_0 <= 1'b0;
                  vrf_wr_enable_mvu_1 <= 1'b0;
                  vrf_wr_enable_mvu_2 <= 1'b1;
                  vrf_wr_enable_mvu_3 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_wr_enable_mfu_add_1 <= 1'b0;
                  vrf_wr_enable_mfu_mul_1 <= 1'b0;
                  vrf_muxed_wr_enable_dram <= 1'b0;
                  dram_write_enable<=1'b0;
                  vrf_in_data <= output_final_stage;
                  vrf_addr_wr<=dstn_address;
                  //vrf_addr_wr_mvu_0 <= dstn_address;
                  end
                  `VRF_3: begin 
                  vrf_wr_enable_mvu_0 <= 1'b0;
                  vrf_wr_enable_mvu_1 <= 1'b0;
                  vrf_wr_enable_mvu_2 <= 1'b0;
                  vrf_wr_enable_mvu_3 <= 1'b1;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_wr_enable_mfu_add_1 <= 1'b0;
                  vrf_wr_enable_mfu_mul_1 <= 1'b0;
                  vrf_muxed_wr_enable_dram <= 1'b0;
                  dram_write_enable<=1'b0;
                  vrf_in_data <= output_final_stage;
                  vrf_addr_wr<=dstn_address;
                  //vrf_addr_wr_mvu_0 <= dstn_address;
                  end

                  `VRF_4: begin 
                  vrf_wr_enable_mvu_0 <= 1'b0;
                  vrf_wr_enable_mvu_1 <= 1'b0;
                  vrf_wr_enable_mvu_2 <= 1'b0;
                  vrf_wr_enable_mvu_3 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b1;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_wr_enable_mfu_add_1 <= 1'b0;
                  vrf_wr_enable_mfu_mul_1 <= 1'b0;
                  vrf_muxed_wr_enable_dram <= 1'b0;
                  dram_write_enable<=1'b0;
                  
                  vrf_in_data <= output_final_stage;
                  
                  vrf_addr_wr_mfu_add_0 <= dstn_address;
                  
                  end
                  
                  `VRF_5: begin 
                  vrf_wr_enable_mvu_0 <= 1'b0;
                  vrf_wr_enable_mvu_1 <= 1'b0;
                  vrf_wr_enable_mvu_2 <= 1'b0;
                  vrf_wr_enable_mvu_3 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b1;
                  vrf_wr_enable_mfu_add_1 <= 1'b0;
                  vrf_wr_enable_mfu_mul_1 <= 1'b0;
                  vrf_muxed_wr_enable_dram <= 1'b0;
                  vrf_in_data <= output_final_stage;
                  
                  vrf_addr_wr_mfu_mul_0 <= dstn_address;
                  
                  end
                  
                  `VRF_6: begin 
                  vrf_wr_enable_mvu_0 <= 1'b0;
                  vrf_wr_enable_mvu_1 <= 1'b0;
                  vrf_wr_enable_mvu_2 <= 1'b0;
                  vrf_wr_enable_mvu_3 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_wr_enable_mfu_add_1 <= 1'b1;
                  vrf_wr_enable_mfu_mul_1 <= 1'b0;
                  vrf_muxed_wr_enable_dram <= 1'b0;
                  dram_write_enable<=1'b0;
                  
                  vrf_in_data <= output_final_stage;
                  
                  vrf_addr_wr_mfu_add_1 <= dstn_address;
                  end
                  
                  `VRF_7: begin 
                  vrf_wr_enable_mvu_0 <= 1'b0;
                  vrf_wr_enable_mvu_1 <= 1'b0;
                  vrf_wr_enable_mvu_2 <= 1'b0;
                  vrf_wr_enable_mvu_3 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_wr_enable_mfu_add_1 <= 1'b0;
                  vrf_wr_enable_mfu_mul_1 <= 1'b1;
                  vrf_muxed_wr_enable_dram <= 1'b0;
                  dram_write_enable<=1'b0;
                  
                  vrf_in_data <= output_final_stage;
                  
                  vrf_addr_wr_mfu_mul_1 <= dstn_address;
                  end
                  
                  `VRF_MUXED: begin 
                  vrf_wr_enable_mvu_0 <= 1'b0;
                  vrf_wr_enable_mvu_1 <= 1'b0;
                  vrf_wr_enable_mvu_2 <= 1'b0;
                  vrf_wr_enable_mvu_3 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_wr_enable_mfu_add_1 <= 1'b0;
                  vrf_wr_enable_mfu_mul_1 <= 1'b0;
                  vrf_muxed_wr_enable_dram <= 1'b1;
                   dram_write_enable<=1'b0;
                   
                   vrf_in_data <= output_final_stage;
                   
                  vrf_muxed_wr_addr_dram <= dstn_address;
                  end
    
                  `DRAM_MEM_ID: begin
                  vrf_wr_enable_mvu_0 <= 1'b0;
                  vrf_wr_enable_mvu_1 <= 1'b0;
                  vrf_wr_enable_mvu_2 <= 1'b0;
                  vrf_wr_enable_mvu_3 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_wr_enable_mfu_add_1 <= 1'b0;
                  vrf_wr_enable_mfu_mul_1 <= 1'b0;
                  vrf_muxed_wr_enable_dram <= 1'b0;
                  dram_write_enable<=1'b1;
                  
                  output_data_to_dram <= output_final_stage;
                   
                  dram_addr_wr <= dstn_address;
                  end
                  
                  //MFU_OUT_STAGE IDS USED FOR MUXING
                  
                  default: begin 
                  vrf_wr_enable_mvu_0 <= 1'b0;
                  vrf_wr_enable_mvu_1 <= 1'b0;
                  vrf_wr_enable_mvu_2 <= 1'b0;
                  vrf_wr_enable_mvu_3 <= 1'b0;
                  vrf_wr_enable_mfu_add_0 <= 1'b0;
                  vrf_wr_enable_mfu_mul_0 <= 1'b0;
                  vrf_wr_enable_mfu_add_1 <= 1'b0;
                  vrf_wr_enable_mfu_mul_1 <= 1'b0;
                  vrf_muxed_wr_enable_dram <= 1'b0;
                  dram_write_enable<=1'b0;
                  end
                 endcase
                end
              end 
             endcase      
            end
         end
       end          
endmodule             
////////////////////////////////////////////////////////////////////////////////
// THIS FILE WAS AUTOMATICALLY GENERATED FROM mvu.v.mako
// DO NOT EDIT
////////////////////////////////////////////////////////////////////////////////


module MVU (
    input clk,
    input[`NUM_LDPES-1:0] start,
    input[`NUM_LDPES-1:0] reset,

    input [`VRF_AWIDTH-1:0] vrf_wr_addr,        
    input [`VRF_AWIDTH-1:0] vrf_read_addr,      
    input [`VRF_DWIDTH-1:0] vec,               
     
    input vrf_wr_enable_tile_0,
    input vrf_readn_enable_tile_0, 
    output[`VRF_DWIDTH-1:0] vrf_data_out_tile_0,
    input vrf_wr_enable_tile_1,
    input vrf_readn_enable_tile_1, 
    output[`VRF_DWIDTH-1:0] vrf_data_out_tile_1,
    input vrf_wr_enable_tile_2,
    input vrf_readn_enable_tile_2, 
    output[`VRF_DWIDTH-1:0] vrf_data_out_tile_2,
    input vrf_wr_enable_tile_3,
    input vrf_readn_enable_tile_3, 
    output[`VRF_DWIDTH-1:0] vrf_data_out_tile_3,
    
    input [`MRF_DWIDTH*`NUM_LDPES*`NUM_TILES-1:0] mrf_in,                 
    input[`NUM_TILES*`NUM_LDPES-1:0] mrf_we,               
    input [`NUM_TILES*`MRF_AWIDTH*`NUM_LDPES-1:0] mrf_addr,

    input [`NUM_TILES*`NUM_LDPES-1:0] mrf_we_for_dram,
    input [`NUM_TILES*`MRF_AWIDTH*`NUM_LDPES-1:0] mrf_addr_for_dram,
    output [`NUM_TILES*`MRF_DWIDTH*`NUM_LDPES-1:0] mrf_outa_to_dram,
    
    output [`ORF_DWIDTH-1:0] mvm_result,
    output out_data_available
);
    wire[`LDPE_USED_OUTPUT_WIDTH*`NUM_LDPES-1:0] result_mvm_0;
    wire[`NUM_LDPES-1:0] out_data_available_mvu_tile_0;

    MVU_tile tile_0(.clk(clk),
    .start(start),
    .reset(reset),
    .out_data_available(out_data_available_mvu_tile_0), //WITH TAG
    .vrf_wr_addr(vrf_wr_addr),
    .vec(vec),
    .vrf_data_out(vrf_data_out_tile_0), //WITH TAG
    .vrf_wr_enable(vrf_wr_enable_tile_0), //WITH TAG
    .vrf_readn_enable(vrf_readn_enable_tile_0), //WITH TAG
    .vrf_read_addr(vrf_read_addr),
    .mrf_in(mrf_in[1*`MRF_DWIDTH*`NUM_LDPES-1:0*`MRF_DWIDTH*`NUM_LDPES]),
    .mrf_we(mrf_we[1*`NUM_LDPES-1:0*`NUM_LDPES]),  //WITH TAG 
    .mrf_addr(mrf_addr[1*`NUM_LDPES*`MRF_AWIDTH-1:0*`NUM_LDPES*`MRF_AWIDTH]),

    .mrf_we_for_dram(mrf_we_for_dram[1*`NUM_LDPES-1:0*`NUM_LDPES]),
    .mrf_addr_for_dram(mrf_addr_for_dram[1*`NUM_LDPES*`MRF_AWIDTH-1:0*`NUM_LDPES*`MRF_AWIDTH]),
    .mrf_outa_to_dram(mrf_outa_to_dram[1*`NUM_LDPES*`MRF_DWIDTH-1:0*`NUM_LDPES*`MRF_DWIDTH]),

    .result(result_mvm_0) //WITH TAG
    );
    wire[`LDPE_USED_OUTPUT_WIDTH*`NUM_LDPES-1:0] result_mvm_1;
    wire[`NUM_LDPES-1:0] out_data_available_mvu_tile_1;

    MVU_tile tile_1(.clk(clk),
    .start(start),
    .reset(reset),
    .out_data_available(out_data_available_mvu_tile_1), //WITH TAG
    .vrf_wr_addr(vrf_wr_addr),
    .vec(vec),
    .vrf_data_out(vrf_data_out_tile_1), //WITH TAG
    .vrf_wr_enable(vrf_wr_enable_tile_1), //WITH TAG
    .vrf_readn_enable(vrf_readn_enable_tile_1), //WITH TAG
    .vrf_read_addr(vrf_read_addr),
    .mrf_in(mrf_in[2*`MRF_DWIDTH*`NUM_LDPES-1:1*`MRF_DWIDTH*`NUM_LDPES]),
    .mrf_we(mrf_we[2*`NUM_LDPES-1:1*`NUM_LDPES]),  //WITH TAG 
    .mrf_addr(mrf_addr[2*`NUM_LDPES*`MRF_AWIDTH-1:1*`NUM_LDPES*`MRF_AWIDTH]),

    .mrf_we_for_dram(mrf_we_for_dram[2*`NUM_LDPES-1:1*`NUM_LDPES]),
    .mrf_addr_for_dram(mrf_addr_for_dram[2*`NUM_LDPES*`MRF_AWIDTH-1:1*`NUM_LDPES*`MRF_AWIDTH]),
    .mrf_outa_to_dram(mrf_outa_to_dram[2*`NUM_LDPES*`MRF_DWIDTH-1:1*`NUM_LDPES*`MRF_DWIDTH]),

    .result(result_mvm_1) //WITH TAG
    );
    wire[`LDPE_USED_OUTPUT_WIDTH*`NUM_LDPES-1:0] result_mvm_2;
    wire[`NUM_LDPES-1:0] out_data_available_mvu_tile_2;

    MVU_tile tile_2(.clk(clk),
    .start(start),
    .reset(reset),
    .out_data_available(out_data_available_mvu_tile_2), //WITH TAG
    .vrf_wr_addr(vrf_wr_addr),
    .vec(vec),
    .vrf_data_out(vrf_data_out_tile_2), //WITH TAG
    .vrf_wr_enable(vrf_wr_enable_tile_2), //WITH TAG
    .vrf_readn_enable(vrf_readn_enable_tile_2), //WITH TAG
    .vrf_read_addr(vrf_read_addr),
    .mrf_in(mrf_in[3*`MRF_DWIDTH*`NUM_LDPES-1:2*`MRF_DWIDTH*`NUM_LDPES]),
    .mrf_we(mrf_we[3*`NUM_LDPES-1:2*`NUM_LDPES]),  //WITH TAG 
    .mrf_addr(mrf_addr[3*`NUM_LDPES*`MRF_AWIDTH-1:2*`NUM_LDPES*`MRF_AWIDTH]),

    .mrf_we_for_dram(mrf_we_for_dram[3*`NUM_LDPES-1:2*`NUM_LDPES]),
    .mrf_addr_for_dram(mrf_addr_for_dram[3*`NUM_LDPES*`MRF_AWIDTH-1:2*`NUM_LDPES*`MRF_AWIDTH]),
    .mrf_outa_to_dram(mrf_outa_to_dram[3*`NUM_LDPES*`MRF_DWIDTH-1:2*`NUM_LDPES*`MRF_DWIDTH]),

    .result(result_mvm_2) //WITH TAG
    );
    wire[`LDPE_USED_OUTPUT_WIDTH*`NUM_LDPES-1:0] result_mvm_3;
    wire[`NUM_LDPES-1:0] out_data_available_mvu_tile_3;

    MVU_tile tile_3(.clk(clk),
    .start(start),
    .reset(reset),
    .out_data_available(out_data_available_mvu_tile_3), //WITH TAG
    .vrf_wr_addr(vrf_wr_addr),
    .vec(vec),
    .vrf_data_out(vrf_data_out_tile_3), //WITH TAG
    .vrf_wr_enable(vrf_wr_enable_tile_3), //WITH TAG
    .vrf_readn_enable(vrf_readn_enable_tile_3), //WITH TAG
    .vrf_read_addr(vrf_read_addr),
    .mrf_in(mrf_in[4*`MRF_DWIDTH*`NUM_LDPES-1:3*`MRF_DWIDTH*`NUM_LDPES]),
    .mrf_we(mrf_we[4*`NUM_LDPES-1:3*`NUM_LDPES]),  //WITH TAG 
    .mrf_addr(mrf_addr[4*`NUM_LDPES*`MRF_AWIDTH-1:3*`NUM_LDPES*`MRF_AWIDTH]),

    .mrf_we_for_dram(mrf_we_for_dram[4*`NUM_LDPES-1:3*`NUM_LDPES]),
    .mrf_addr_for_dram(mrf_addr_for_dram[4*`NUM_LDPES*`MRF_AWIDTH-1:3*`NUM_LDPES*`MRF_AWIDTH]),
    .mrf_outa_to_dram(mrf_outa_to_dram[4*`NUM_LDPES*`MRF_DWIDTH-1:3*`NUM_LDPES*`MRF_DWIDTH]),

    .result(result_mvm_3) //WITH TAG
    );

    wire[`NUM_LDPES*`OUT_DWIDTH-1:0] reduction_unit_output;
    wire[`NUM_LDPES-1:0] out_data_available_reduction_tree;

    mvm_reduction_unit mvm_reduction(
      .clk(clk),
      .start(out_data_available_mvu_tile_0),
      .reset_reduction_mvm(reset),
      .inp0(result_mvm_0),
      .inp1(result_mvm_1),
      .inp2(result_mvm_2),
      .inp3(result_mvm_3),
      .result_mvm_final_stage(reduction_unit_output),
      .out_data_available(out_data_available_reduction_tree)
    );
    
    assign mvm_result = reduction_unit_output;
    assign out_data_available = out_data_available_reduction_tree[0];
    
endmodule


module MVU_tile (
    input clk,
    input [`NUM_LDPES-1:0] start,
    input [`NUM_LDPES-1:0] reset,
    input vrf_wr_enable,
    input [`VRF_AWIDTH-1:0] vrf_wr_addr,
    input [`VRF_AWIDTH-1:0] vrf_read_addr,
    input [`VRF_DWIDTH-1:0] vec,
    output[`VRF_DWIDTH-1:0] vrf_data_out,
    input [`NUM_LDPES*`MRF_DWIDTH-1:0] mrf_in,
    input vrf_readn_enable,
    input[`NUM_LDPES-1:0] mrf_we,
    input [`MRF_AWIDTH*`NUM_LDPES-1:0] mrf_addr,

    input[`NUM_LDPES-1:0] mrf_we_for_dram,
    input [`MRF_AWIDTH*`NUM_LDPES-1:0] mrf_addr_for_dram,
    output [`MRF_DWIDTH*`NUM_LDPES-1:0] mrf_outa_to_dram,

    output [`LDPE_USED_OUTPUT_WIDTH*`NUM_LDPES-1:0] result,
    output [`NUM_LDPES-1:0] out_data_available
);

    wire [`VRF_DWIDTH-1:0] ina_fake;
   
  
    wire [`VRF_DWIDTH-1:0] vrf_outa_wire;

    //reg [`VRF_AWIDTH-1:0] vrf_rd_addr;

    // Port A is used to feed LDPE and port B to load vector from DRAM.
    VRF vrf (
        .clk(clk),
        .addra(vrf_read_addr),
        .ina(ina_fake),
        .wea(vrf_readn_enable),
        .outa(vrf_outa_wire),
        .addrb(vrf_wr_addr),
        .inb(vec),
        .web(vrf_wr_enable),
        .outb(vrf_data_out) 
    );

    genvar i;
    generate
        for (i=1; i<=`NUM_LDPES; i=i+1) begin: gen_cus
            compute_unit unit (
                .clk(clk),
                .start(start[i-1]),
                .reset(reset[i-1]),
                .out_data_available(out_data_available[i-1]),
                .vec(vrf_outa_wire),
                .mrf_in(mrf_in[i*`MRF_DWIDTH-1:(i-1)*`MRF_DWIDTH]),
                .mrf_we(mrf_we[i-1]),
                .mrf_addr(mrf_addr[i*`MRF_AWIDTH-1:(i-1)*`MRF_AWIDTH]),

                .mrf_addr_for_dram(mrf_addr_for_dram[(i)*`MRF_AWIDTH-1:(i-1)*`MRF_AWIDTH]),
                .mrf_outa_to_dram(mrf_outa_to_dram[(i)*`MRF_DWIDTH-1:(i-1)*`MRF_DWIDTH]),
                .mrf_we_for_dram(mrf_we_for_dram[i-1]),
 
                .result(result[i*`LDPE_USED_OUTPUT_WIDTH-1:(i-1)*`LDPE_USED_OUTPUT_WIDTH])
            );
        end
    endgenerate

endmodule

module compute_unit (
    input clk,
    input start,
    input reset,
    input [`VRF_DWIDTH-1:0] vec,
    input [`MRF_DWIDTH-1:0] mrf_in,
    input [`MRF_AWIDTH-1:0] mrf_addr_for_dram, //new
    input mrf_we, mrf_we_for_dram, //new
    input [`MRF_AWIDTH-1:0] mrf_addr,
    output [`LDPE_USED_OUTPUT_WIDTH-1:0] result,
    output [`MRF_DWIDTH-1:0] mrf_outa_to_dram, //new
    output reg out_data_available
);

    // Port A of BRAMs is used for feed DSPs and Port B is used to load matrix from off-chip memory
    reg [4:0] num_cycles_mvm; 

    always@(posedge clk) begin
        if((reset==1'b1) || (start==1'b0)) begin
            num_cycles_mvm <= 0;
            out_data_available <= 0;
        end
        else begin
            if(num_cycles_mvm==`NUM_MVM_CYCLES-1) begin
                out_data_available <= 1;
            end
            else begin
                num_cycles_mvm <= num_cycles_mvm + 1'b1;
            end
        end
    end
  
    // Port B of BRAMs is used for feed DSPs and Port A is used to interact with DRAM

  
    wire [`MRF_DWIDTH-1:0] mrf_outb_wire;

    wire [`LDPE_USED_INPUT_WIDTH-1:0] ax_wire;
    wire [`LDPE_USED_INPUT_WIDTH-1:0] ay_wire;
    wire [`LDPE_USED_INPUT_WIDTH-1:0] bx_wire;
    wire [`LDPE_USED_INPUT_WIDTH-1:0] by_wire;

    // Wire connecting LDPE output to Output BRAM input
    wire [`LDPE_USED_OUTPUT_WIDTH-1:0] ldpe_result;
    
    wire [`LDPE_USED_OUTPUT_WIDTH-1:0] inb_fake_wire;

    // First 4 BRAM outputs are given to ax of 4 DSPs and next 4 BRAM outputs are given to bx of DSPs

    // Connection MRF and LDPE wires for matrix data
    // 'X' pin is used for matrix
    /* If there are 4 DSPSs, bit[31:0] of mrf output contain ax values for the 4 DSPs, bit[63:32] contain bx values and so on. With a group of ax values, bit[7:0] contain ax value for DSP1, bit[15:8] contain ax value for DSP2 and so on. */
    assign ax_wire = mrf_outb_wire[1*`LDPE_USED_INPUT_WIDTH-1:0*`LDPE_USED_INPUT_WIDTH];
    assign bx_wire = mrf_outb_wire[2*`LDPE_USED_INPUT_WIDTH-1:1*`LDPE_USED_INPUT_WIDTH];

    // Connection of VRF and LDPE wires for vector data
    // 'Y' pin is used for vector
    assign ay_wire = vec[`LDPE_USED_INPUT_WIDTH-1:0];
    assign by_wire = vec[2*`LDPE_USED_INPUT_WIDTH-1:1*`LDPE_USED_INPUT_WIDTH];

    wire [`MRF_DWIDTH-1:0] mrf_in_fake;
    
    MRF mrf (
        .clk(clk),
        .addra(mrf_addr_for_dram),
        .addrb(mrf_addr),
        .ina(mrf_in),
        .inb(mrf_in_fake),
        .wea(mrf_we_for_dram),
        .web(mrf_we),
        .outa(mrf_outa_to_dram),
        .outb(mrf_outb_wire)
    );

    LDPE ldpe (
        .clk(clk),
        .reset(reset),
        .ax(ax_wire),
        .ay(ay_wire),
        .bx(bx_wire),
        .by(by_wire),
        .ldpe_result(ldpe_result)
    );
    assign result = ldpe_result;
    
endmodule

module LDPE (
    input clk,
    input reset,
    input [`LDPE_USED_INPUT_WIDTH-1:0] ax,
    input [`LDPE_USED_INPUT_WIDTH-1:0] ay,
    input [`LDPE_USED_INPUT_WIDTH-1:0] bx,
    input [`LDPE_USED_INPUT_WIDTH-1:0] by,
    output [`LDPE_USED_OUTPUT_WIDTH-1:0] ldpe_result
);

    wire [`LDPE_USED_OUTPUT_WIDTH*`SUB_LDPES_PER_LDPE-1:0] sub_ldpe_result;
    //wire [`LDPE_USED_OUTPUT_WIDTH-1:0] ldpe_result;

    SUB_LDPE sub_1(
        .clk(clk),
        .reset(reset),
        .ax(ax[1*`SUB_LDPE_USED_INPUT_WIDTH-1:(1-1)*`SUB_LDPE_USED_INPUT_WIDTH]),
        .ay(ay[1*`SUB_LDPE_USED_INPUT_WIDTH-1:(1-1)*`SUB_LDPE_USED_INPUT_WIDTH]),
        .bx(bx[1*`SUB_LDPE_USED_INPUT_WIDTH-1:(1-1)*`SUB_LDPE_USED_INPUT_WIDTH]),
        .by(by[1*`SUB_LDPE_USED_INPUT_WIDTH-1:(1-1)*`SUB_LDPE_USED_INPUT_WIDTH]),
        .result(sub_ldpe_result[1*`DSP_USED_OUTPUT_WIDTH-1:(1-1)*`DSP_USED_OUTPUT_WIDTH])
    );
    assign ldpe_result = sub_ldpe_result[(0+1)*`DSP_USED_OUTPUT_WIDTH-1:0*`DSP_USED_OUTPUT_WIDTH];

endmodule

module myadder #(
    parameter INPUT_WIDTH = `DSP_USED_INPUT_WIDTH,
    parameter OUTPUT_WIDTH = `DSP_USED_OUTPUT_WIDTH
)
(
    input [INPUT_WIDTH-1:0] a,
    input [INPUT_WIDTH-1:0] b,
    input reset,
    input start,
    input clk,
    output reg [OUTPUT_WIDTH-1:0] sum,
    output reg out_data_available
);

    always@(posedge clk) begin
        if((reset==1) || (start==0)) begin
            sum <= 0;
            out_data_available <= 0;
        end
        else begin
            out_data_available <= 1;
            sum <= a + b;
        end
    end

endmodule

module SUB_LDPE (
    input clk,
    input reset,
    input [`SUB_LDPE_USED_INPUT_WIDTH-1:0] ax,
    input [`SUB_LDPE_USED_INPUT_WIDTH-1:0] ay,
    input [`SUB_LDPE_USED_INPUT_WIDTH-1:0] bx,
    input [`SUB_LDPE_USED_INPUT_WIDTH-1:0] by,
    output reg [`LDPE_USED_OUTPUT_WIDTH-1:0] result
);

    wire [`DSP_USED_OUTPUT_WIDTH*`DSPS_PER_SUB_LDPE-1:0] chainin, chainout, dsp_result;

 
    wire [36:0] chainout_temp_0;
    assign chainout_temp_0 = 37'b0;

    wire [`DSP_X_AVA_INPUT_WIDTH-1:0] ax_wire_1;
    wire [`DSP_Y_AVA_INPUT_WIDTH-1:0] ay_wire_1;
    wire [`DSP_X_AVA_INPUT_WIDTH-1:0] bx_wire_1;
    wire [`DSP_Y_AVA_INPUT_WIDTH-1:0] by_wire_1;

    assign ax_wire_1 = {{`DSP_X_ZERO_PAD_INPUT_WIDTH{1'b0}}, ax[1*`DSP_USED_INPUT_WIDTH-1:(1-1)*`DSP_USED_INPUT_WIDTH]};
    assign ay_wire_1 = {{`DSP_Y_ZERO_PAD_INPUT_WIDTH{1'b0}}, ay[1*`DSP_USED_INPUT_WIDTH-1:(1-1)*`DSP_USED_INPUT_WIDTH]};

    assign bx_wire_1 = {{`DSP_X_ZERO_PAD_INPUT_WIDTH{1'b0}}, bx[1*`DSP_USED_INPUT_WIDTH-1:(1-1)*`DSP_USED_INPUT_WIDTH]};
    assign by_wire_1 = {{`DSP_Y_ZERO_PAD_INPUT_WIDTH{1'b0}}, by[1*`DSP_USED_INPUT_WIDTH-1:(1-1)*`DSP_USED_INPUT_WIDTH]};

    wire [`DSP_AVA_OUTPUT_WIDTH-1:0] chainout_temp_1;
    wire [`DSP_AVA_OUTPUT_WIDTH-1:0] result_temp_1;

    assign dsp_result[1*`DSP_USED_OUTPUT_WIDTH-1:(1-1)*`DSP_USED_OUTPUT_WIDTH] = result_temp_1[`DSP_USED_OUTPUT_WIDTH-1:0];

    dsp_block_18_18_int_sop_2 dsp_1 (
        .clk(clk),
        .aclr(reset),
        .ax(ax_wire_1),
        .ay(ay_wire_1),
        .bx(bx_wire_1),
        .by(by_wire_1),
        .chainin(chainout_temp_0),
        .chainout(chainout_temp_1),
        .result(result_temp_1)
    );
    wire [`DSP_X_AVA_INPUT_WIDTH-1:0] ax_wire_2;
    wire [`DSP_Y_AVA_INPUT_WIDTH-1:0] ay_wire_2;
    wire [`DSP_X_AVA_INPUT_WIDTH-1:0] bx_wire_2;
    wire [`DSP_Y_AVA_INPUT_WIDTH-1:0] by_wire_2;

    assign ax_wire_2 = {{`DSP_X_ZERO_PAD_INPUT_WIDTH{1'b0}}, ax[2*`DSP_USED_INPUT_WIDTH-1:(2-1)*`DSP_USED_INPUT_WIDTH]};
    assign ay_wire_2 = {{`DSP_Y_ZERO_PAD_INPUT_WIDTH{1'b0}}, ay[2*`DSP_USED_INPUT_WIDTH-1:(2-1)*`DSP_USED_INPUT_WIDTH]};

    assign bx_wire_2 = {{`DSP_X_ZERO_PAD_INPUT_WIDTH{1'b0}}, bx[2*`DSP_USED_INPUT_WIDTH-1:(2-1)*`DSP_USED_INPUT_WIDTH]};
    assign by_wire_2 = {{`DSP_Y_ZERO_PAD_INPUT_WIDTH{1'b0}}, by[2*`DSP_USED_INPUT_WIDTH-1:(2-1)*`DSP_USED_INPUT_WIDTH]};

    wire [`DSP_AVA_OUTPUT_WIDTH-1:0] chainout_temp_2;
    wire [`DSP_AVA_OUTPUT_WIDTH-1:0] result_temp_2;

    assign dsp_result[2*`DSP_USED_OUTPUT_WIDTH-1:(2-1)*`DSP_USED_OUTPUT_WIDTH] = result_temp_2[`DSP_USED_OUTPUT_WIDTH-1:0];

    dsp_block_18_18_int_sop_2 dsp_2 (
        .clk(clk),
        .aclr(reset),
        .ax(ax_wire_2),
        .ay(ay_wire_2),
        .bx(bx_wire_2),
        .by(by_wire_2),
        .chainin(chainout_temp_1),
        .chainout(chainout_temp_2),
        .result(result_temp_2)
    );
    wire [`DSP_X_AVA_INPUT_WIDTH-1:0] ax_wire_3;
    wire [`DSP_Y_AVA_INPUT_WIDTH-1:0] ay_wire_3;
    wire [`DSP_X_AVA_INPUT_WIDTH-1:0] bx_wire_3;
    wire [`DSP_Y_AVA_INPUT_WIDTH-1:0] by_wire_3;

    assign ax_wire_3 = {{`DSP_X_ZERO_PAD_INPUT_WIDTH{1'b0}}, ax[3*`DSP_USED_INPUT_WIDTH-1:(3-1)*`DSP_USED_INPUT_WIDTH]};
    assign ay_wire_3 = {{`DSP_Y_ZERO_PAD_INPUT_WIDTH{1'b0}}, ay[3*`DSP_USED_INPUT_WIDTH-1:(3-1)*`DSP_USED_INPUT_WIDTH]};

    assign bx_wire_3 = {{`DSP_X_ZERO_PAD_INPUT_WIDTH{1'b0}}, bx[3*`DSP_USED_INPUT_WIDTH-1:(3-1)*`DSP_USED_INPUT_WIDTH]};
    assign by_wire_3 = {{`DSP_Y_ZERO_PAD_INPUT_WIDTH{1'b0}}, by[3*`DSP_USED_INPUT_WIDTH-1:(3-1)*`DSP_USED_INPUT_WIDTH]};

    wire [`DSP_AVA_OUTPUT_WIDTH-1:0] chainout_temp_3;
    wire [`DSP_AVA_OUTPUT_WIDTH-1:0] result_temp_3;

    assign dsp_result[3*`DSP_USED_OUTPUT_WIDTH-1:(3-1)*`DSP_USED_OUTPUT_WIDTH] = result_temp_3[`DSP_USED_OUTPUT_WIDTH-1:0];

    dsp_block_18_18_int_sop_2 dsp_3 (
        .clk(clk),
        .aclr(reset),
        .ax(ax_wire_3),
        .ay(ay_wire_3),
        .bx(bx_wire_3),
        .by(by_wire_3),
        .chainin(chainout_temp_2),
        .chainout(chainout_temp_3),
        .result(result_temp_3)
    );
    wire [`DSP_X_AVA_INPUT_WIDTH-1:0] ax_wire_4;
    wire [`DSP_Y_AVA_INPUT_WIDTH-1:0] ay_wire_4;
    wire [`DSP_X_AVA_INPUT_WIDTH-1:0] bx_wire_4;
    wire [`DSP_Y_AVA_INPUT_WIDTH-1:0] by_wire_4;

    assign ax_wire_4 = {{`DSP_X_ZERO_PAD_INPUT_WIDTH{1'b0}}, ax[4*`DSP_USED_INPUT_WIDTH-1:(4-1)*`DSP_USED_INPUT_WIDTH]};
    assign ay_wire_4 = {{`DSP_Y_ZERO_PAD_INPUT_WIDTH{1'b0}}, ay[4*`DSP_USED_INPUT_WIDTH-1:(4-1)*`DSP_USED_INPUT_WIDTH]};

    assign bx_wire_4 = {{`DSP_X_ZERO_PAD_INPUT_WIDTH{1'b0}}, bx[4*`DSP_USED_INPUT_WIDTH-1:(4-1)*`DSP_USED_INPUT_WIDTH]};
    assign by_wire_4 = {{`DSP_Y_ZERO_PAD_INPUT_WIDTH{1'b0}}, by[4*`DSP_USED_INPUT_WIDTH-1:(4-1)*`DSP_USED_INPUT_WIDTH]};

    wire [`DSP_AVA_OUTPUT_WIDTH-1:0] chainout_temp_4;
    wire [`DSP_AVA_OUTPUT_WIDTH-1:0] result_temp_4;

    assign dsp_result[4*`DSP_USED_OUTPUT_WIDTH-1:(4-1)*`DSP_USED_OUTPUT_WIDTH] = result_temp_4[`DSP_USED_OUTPUT_WIDTH-1:0];

    dsp_block_18_18_int_sop_2 dsp_4 (
        .clk(clk),
        .aclr(reset),
        .ax(ax_wire_4),
        .ay(ay_wire_4),
        .bx(bx_wire_4),
        .by(by_wire_4),
        .chainin(chainout_temp_3),
        .chainout(chainout_temp_4),
        .result(result_temp_4)
    );
    wire [`DSP_X_AVA_INPUT_WIDTH-1:0] ax_wire_5;
    wire [`DSP_Y_AVA_INPUT_WIDTH-1:0] ay_wire_5;
    wire [`DSP_X_AVA_INPUT_WIDTH-1:0] bx_wire_5;
    wire [`DSP_Y_AVA_INPUT_WIDTH-1:0] by_wire_5;

    assign ax_wire_5 = {{`DSP_X_ZERO_PAD_INPUT_WIDTH{1'b0}}, ax[5*`DSP_USED_INPUT_WIDTH-1:(5-1)*`DSP_USED_INPUT_WIDTH]};
    assign ay_wire_5 = {{`DSP_Y_ZERO_PAD_INPUT_WIDTH{1'b0}}, ay[5*`DSP_USED_INPUT_WIDTH-1:(5-1)*`DSP_USED_INPUT_WIDTH]};

    assign bx_wire_5 = {{`DSP_X_ZERO_PAD_INPUT_WIDTH{1'b0}}, bx[5*`DSP_USED_INPUT_WIDTH-1:(5-1)*`DSP_USED_INPUT_WIDTH]};
    assign by_wire_5 = {{`DSP_Y_ZERO_PAD_INPUT_WIDTH{1'b0}}, by[5*`DSP_USED_INPUT_WIDTH-1:(5-1)*`DSP_USED_INPUT_WIDTH]};

    wire [`DSP_AVA_OUTPUT_WIDTH-1:0] chainout_temp_5;
    wire [`DSP_AVA_OUTPUT_WIDTH-1:0] result_temp_5;

    assign dsp_result[5*`DSP_USED_OUTPUT_WIDTH-1:(5-1)*`DSP_USED_OUTPUT_WIDTH] = result_temp_5[`DSP_USED_OUTPUT_WIDTH-1:0];

    dsp_block_18_18_int_sop_2 dsp_5 (
        .clk(clk),
        .aclr(reset),
        .ax(ax_wire_5),
        .ay(ay_wire_5),
        .bx(bx_wire_5),
        .by(by_wire_5),
        .chainin(chainout_temp_4),
        .chainout(chainout_temp_5),
        .result(result_temp_5)
    );
    wire [`DSP_X_AVA_INPUT_WIDTH-1:0] ax_wire_6;
    wire [`DSP_Y_AVA_INPUT_WIDTH-1:0] ay_wire_6;
    wire [`DSP_X_AVA_INPUT_WIDTH-1:0] bx_wire_6;
    wire [`DSP_Y_AVA_INPUT_WIDTH-1:0] by_wire_6;

    assign ax_wire_6 = {{`DSP_X_ZERO_PAD_INPUT_WIDTH{1'b0}}, ax[6*`DSP_USED_INPUT_WIDTH-1:(6-1)*`DSP_USED_INPUT_WIDTH]};
    assign ay_wire_6 = {{`DSP_Y_ZERO_PAD_INPUT_WIDTH{1'b0}}, ay[6*`DSP_USED_INPUT_WIDTH-1:(6-1)*`DSP_USED_INPUT_WIDTH]};

    assign bx_wire_6 = {{`DSP_X_ZERO_PAD_INPUT_WIDTH{1'b0}}, bx[6*`DSP_USED_INPUT_WIDTH-1:(6-1)*`DSP_USED_INPUT_WIDTH]};
    assign by_wire_6 = {{`DSP_Y_ZERO_PAD_INPUT_WIDTH{1'b0}}, by[6*`DSP_USED_INPUT_WIDTH-1:(6-1)*`DSP_USED_INPUT_WIDTH]};

    wire [`DSP_AVA_OUTPUT_WIDTH-1:0] chainout_temp_6;
    wire [`DSP_AVA_OUTPUT_WIDTH-1:0] result_temp_6;

    assign dsp_result[6*`DSP_USED_OUTPUT_WIDTH-1:(6-1)*`DSP_USED_OUTPUT_WIDTH] = result_temp_6[`DSP_USED_OUTPUT_WIDTH-1:0];

    dsp_block_18_18_int_sop_2 dsp_6 (
        .clk(clk),
        .aclr(reset),
        .ax(ax_wire_6),
        .ay(ay_wire_6),
        .bx(bx_wire_6),
        .by(by_wire_6),
        .chainin(chainout_temp_5),
        .chainout(chainout_temp_6),
        .result(result_temp_6)
    );
    wire [`DSP_X_AVA_INPUT_WIDTH-1:0] ax_wire_7;
    wire [`DSP_Y_AVA_INPUT_WIDTH-1:0] ay_wire_7;
    wire [`DSP_X_AVA_INPUT_WIDTH-1:0] bx_wire_7;
    wire [`DSP_Y_AVA_INPUT_WIDTH-1:0] by_wire_7;

    assign ax_wire_7 = {{`DSP_X_ZERO_PAD_INPUT_WIDTH{1'b0}}, ax[7*`DSP_USED_INPUT_WIDTH-1:(7-1)*`DSP_USED_INPUT_WIDTH]};
    assign ay_wire_7 = {{`DSP_Y_ZERO_PAD_INPUT_WIDTH{1'b0}}, ay[7*`DSP_USED_INPUT_WIDTH-1:(7-1)*`DSP_USED_INPUT_WIDTH]};

    assign bx_wire_7 = {{`DSP_X_ZERO_PAD_INPUT_WIDTH{1'b0}}, bx[7*`DSP_USED_INPUT_WIDTH-1:(7-1)*`DSP_USED_INPUT_WIDTH]};
    assign by_wire_7 = {{`DSP_Y_ZERO_PAD_INPUT_WIDTH{1'b0}}, by[7*`DSP_USED_INPUT_WIDTH-1:(7-1)*`DSP_USED_INPUT_WIDTH]};

    wire [`DSP_AVA_OUTPUT_WIDTH-1:0] chainout_temp_7;
    wire [`DSP_AVA_OUTPUT_WIDTH-1:0] result_temp_7;

    assign dsp_result[7*`DSP_USED_OUTPUT_WIDTH-1:(7-1)*`DSP_USED_OUTPUT_WIDTH] = result_temp_7[`DSP_USED_OUTPUT_WIDTH-1:0];

    dsp_block_18_18_int_sop_2 dsp_7 (
        .clk(clk),
        .aclr(reset),
        .ax(ax_wire_7),
        .ay(ay_wire_7),
        .bx(bx_wire_7),
        .by(by_wire_7),
        .chainin(chainout_temp_6),
        .chainout(chainout_temp_7),
        .result(result_temp_7)
    );
    wire [`DSP_X_AVA_INPUT_WIDTH-1:0] ax_wire_8;
    wire [`DSP_Y_AVA_INPUT_WIDTH-1:0] ay_wire_8;
    wire [`DSP_X_AVA_INPUT_WIDTH-1:0] bx_wire_8;
    wire [`DSP_Y_AVA_INPUT_WIDTH-1:0] by_wire_8;

    assign ax_wire_8 = {{`DSP_X_ZERO_PAD_INPUT_WIDTH{1'b0}}, ax[8*`DSP_USED_INPUT_WIDTH-1:(8-1)*`DSP_USED_INPUT_WIDTH]};
    assign ay_wire_8 = {{`DSP_Y_ZERO_PAD_INPUT_WIDTH{1'b0}}, ay[8*`DSP_USED_INPUT_WIDTH-1:(8-1)*`DSP_USED_INPUT_WIDTH]};

    assign bx_wire_8 = {{`DSP_X_ZERO_PAD_INPUT_WIDTH{1'b0}}, bx[8*`DSP_USED_INPUT_WIDTH-1:(8-1)*`DSP_USED_INPUT_WIDTH]};
    assign by_wire_8 = {{`DSP_Y_ZERO_PAD_INPUT_WIDTH{1'b0}}, by[8*`DSP_USED_INPUT_WIDTH-1:(8-1)*`DSP_USED_INPUT_WIDTH]};

    wire [`DSP_AVA_OUTPUT_WIDTH-1:0] chainout_temp_8;
    wire [`DSP_AVA_OUTPUT_WIDTH-1:0] result_temp_8;

    assign dsp_result[8*`DSP_USED_OUTPUT_WIDTH-1:(8-1)*`DSP_USED_OUTPUT_WIDTH] = result_temp_8[`DSP_USED_OUTPUT_WIDTH-1:0];

    dsp_block_18_18_int_sop_2 dsp_8 (
        .clk(clk),
        .aclr(reset),
        .ax(ax_wire_8),
        .ay(ay_wire_8),
        .bx(bx_wire_8),
        .by(by_wire_8),
        .chainin(chainout_temp_7),
        .chainout(chainout_temp_8),
        .result(result_temp_8)
    );
    
    always @(*) begin
        if (reset) begin
            result <= {`LDPE_USED_OUTPUT_WIDTH{1'd0}};
        end
        else begin
            result <= dsp_result[`DSPS_PER_SUB_LDPE*`LDPE_USED_OUTPUT_WIDTH-1:(`DSPS_PER_SUB_LDPE-1)*`LDPE_USED_OUTPUT_WIDTH];
        end
    end

endmodule

module VRF #(parameter VRF_AWIDTH = `VRF_AWIDTH, parameter VRF_DWIDTH = `VRF_DWIDTH) (
    input clk,
    input [VRF_AWIDTH-1:0] addra, 
    input [VRF_AWIDTH-1:0] addrb,
    input [VRF_DWIDTH-1:0] ina,
    input [VRF_DWIDTH-1:0] inb,
    input wea, web,
    output [VRF_DWIDTH-1:0] outa,
    output [VRF_DWIDTH-1:0] outb
);

    dp_ram # (
        .AWIDTH(VRF_AWIDTH),
        .DWIDTH(VRF_DWIDTH)
    ) vec_mem (
        .clk(clk),
        .addra(addra),
        .ina(ina),
        .wea(wea),
        .outa(outa),
        .addrb(addrb),
        .inb(inb),
        .web(web),
        .outb(outb)
    );
endmodule

module MRF (
    input clk,
    input [`MRF_AWIDTH-1:0] addra, 
    input [`MRF_AWIDTH-1:0] addrb,
    input [`MRF_DWIDTH-1:0] ina, 
    input [`MRF_DWIDTH-1:0] inb,
    input wea, web,
    output [`MRF_DWIDTH-1:0] outa,
    output [`MRF_DWIDTH-1:0] outb
);

    dp_ram # (
            .AWIDTH(`MRF_AWIDTH),
            .DWIDTH(`MRF_DWIDTH)
        ) vec_mem (
            .clk(clk),
            .addra(addra),
            .ina(ina),
            .wea(wea),
            .outa(outa),
            .addrb(addrb),
            .inb(inb),
            .web(web),
            .outb(outb)
        );

endmodule

module dsp_block_18_18_int_sop_2 (
    input clk,
    input aclr,
    input [`DSP_X_AVA_INPUT_WIDTH-1:0] ax,
    input [`DSP_Y_AVA_INPUT_WIDTH-1:0] ay,
    input [`DSP_X_AVA_INPUT_WIDTH-1:0] bx,
    input [`DSP_Y_AVA_INPUT_WIDTH-1:0] by,
    input [`DSP_AVA_OUTPUT_WIDTH-1:0] chainin,
    output [`DSP_AVA_OUTPUT_WIDTH-1:0] chainout,
    output [`DSP_AVA_OUTPUT_WIDTH-1:0] result
);

`ifndef complex_dsp 

reg [`DSP_X_AVA_INPUT_WIDTH-1:0] ax_reg;
reg [`DSP_Y_AVA_INPUT_WIDTH-1:0] ay_reg;
reg [`DSP_X_AVA_INPUT_WIDTH-1:0] bx_reg;
reg [`DSP_Y_AVA_INPUT_WIDTH-1:0] by_reg;
reg [`DSP_AVA_OUTPUT_WIDTH-1:0] result_reg;

always @(posedge clk) begin
    if(aclr) begin
        result_reg <= 0;
        ax_reg <= 0;
        ay_reg <= 0;
        bx_reg <= 0;
        by_reg <= 0;
    end
    else begin
        ax_reg <= ax;
        ay_reg <= ay;
        bx_reg <= bx;
        by_reg <= by;
        result_reg <= (ax_reg * ay_reg) + (bx_reg * by_reg) + chainin;
    end
end
assign chainout = result_reg;
assign result = result_reg;

`else

wire [11:0] mode;
assign mode = 12'b0101_0101_0011;

int_sop_2 mac_component (
    .mode_sigs(mode),
    .clk(clk),
    .reset(aclr),
    .ax(ax),
    .ay(ay),
    .bx(bx),
    .by(by),
    .chainin(chainin),
    .result(result),
    .chainout(chainout)
);

`endif

endmodule

//////////////////////////////////
// Dual port RAM
//////////////////////////////////

module dp_ram # (
    parameter AWIDTH = 10,
    parameter DWIDTH = 16
) (
    input clk,
    input [AWIDTH-1:0] addra, addrb,
    input [DWIDTH-1:0] ina, inb,
    input wea, web,
    output reg [DWIDTH-1:0] outa, outb
);

`ifndef hard_mem 

reg [DWIDTH-1:0] ram [((1<<AWIDTH)-1):0];

// Port A
always @(posedge clk)  begin

    if (wea) begin
        ram[addra] <= ina;
    end

    outa <= ram[addra];
end

// Port B
always @(posedge clk)  begin

    if (web) begin
        ram[addrb] <= inb;
    end

    outb <= ram[addrb];
end

`else


defparam u_dual_port_ram.ADDR_WIDTH = AWIDTH;
defparam u_dual_port_ram.DATA_WIDTH = DWIDTH;


dual_port_ram u_dual_port_ram(
.addr1(addra),
.we1(wea),
.data1(ina),
.out1(outa),
.addr2(addrb),
.we2(web),
.data2(inb),
.out2(outb),
.clk(clk)
);

`endif
endmodule

//////////////////////////////////
// Single port RAM
//////////////////////////////////

module sp_ram # (
    parameter AWIDTH = 9,
    parameter DWIDTH = 32
) (
    input clk,
    input [AWIDTH-1:0] addr,
    input [DWIDTH-1:0] in,
    input we,
    output reg [DWIDTH-1:0] out
);

`ifndef hard_mem

reg [DWIDTH-1:0] ram [((1<<AWIDTH)-1):0];

always @(posedge clk)  begin

    if (we) begin
        ram[addr] <= in;
    end

    out <= ram[addr];
end

`else

defparam u_single_port_ram.ADDR_WIDTH = AWIDTH;
defparam u_single_port_ram.DATA_WIDTH = DWIDTH;

single_port_ram u_single_port_ram(
.addr(addr),
.we(we),
.data(in),
.out(out),
.clk(clk)
);

`endif
endmodule

module mvm_reduction_unit(
    input[`DSP_USED_OUTPUT_WIDTH*`NUM_LDPES-1:0] inp0,
    input[`DSP_USED_OUTPUT_WIDTH*`NUM_LDPES-1:0] inp1,
    input[`DSP_USED_OUTPUT_WIDTH*`NUM_LDPES-1:0] inp2,
    input[`DSP_USED_OUTPUT_WIDTH*`NUM_LDPES-1:0] inp3,
    output [`OUT_DWIDTH*`NUM_LDPES-1:0] result_mvm_final_stage,
    output [`NUM_LDPES-1:0] out_data_available,
    //CONTROL SIGNALS
    input clk,
    input[`NUM_LDPES-1:0] start,
    input[`NUM_LDPES-1:0] reset_reduction_mvm
);

/*
    reg[3:0] num_cycles_reduction;

    always@(posedge clk) begin
        if((reset_reduction_mvm[0]==1'b1) || (start[0]==1'b0)) begin
            num_cycles_reduction<=0;
            out_data_available<=0;
        end
        else begin
            if(num_cycles_reduction==`NUM_REDUCTION_CYCLES-1) begin
                out_data_available <= {`NUM_LDPES{1'b1}};
            end
            else begin
                num_cycles_reduction <= num_cycles_reduction + 1;
            end
        end
    end
*/

    genvar i;

    wire[(`DSP_USED_OUTPUT_WIDTH)*`NUM_LDPES-1:0] reduction_output_0_stage_1;
    wire[`NUM_LDPES-1:0] out_data_available_0_stage_1;

    generate
        for(i=1; i<=`NUM_LDPES; i=i+1) begin: gen_adder1
           myadder #(.INPUT_WIDTH(`DSP_USED_OUTPUT_WIDTH),.OUTPUT_WIDTH(`DSP_USED_OUTPUT_WIDTH)) adder_units_initial_0 (
              .a(inp0[i*`DSP_USED_OUTPUT_WIDTH-1:(i-1)*`DSP_USED_OUTPUT_WIDTH]),
              .b(inp1[i*`DSP_USED_OUTPUT_WIDTH-1:(i-1)*`DSP_USED_OUTPUT_WIDTH]),
              .clk(clk),
              .reset(reset_reduction_mvm[i-1]),
              .start(start[i-1]),
              .out_data_available(out_data_available_0_stage_1[i-1]),
              .sum(reduction_output_0_stage_1[i*`DSP_USED_OUTPUT_WIDTH-1:(i-1)*`DSP_USED_OUTPUT_WIDTH])
            );
        end
    endgenerate
    wire[(`DSP_USED_OUTPUT_WIDTH)*`NUM_LDPES-1:0] reduction_output_1_stage_1;
    wire[`NUM_LDPES-1:0] out_data_available_1_stage_1;

    generate
        for(i=1; i<=`NUM_LDPES; i=i+1) begin: gen_adder2
           myadder #(.INPUT_WIDTH(`DSP_USED_OUTPUT_WIDTH),.OUTPUT_WIDTH(`DSP_USED_OUTPUT_WIDTH)) adder_units_initial_1 (
              .a(inp2[i*`DSP_USED_OUTPUT_WIDTH-1:(i-1)*`DSP_USED_OUTPUT_WIDTH]),
              .b(inp3[i*`DSP_USED_OUTPUT_WIDTH-1:(i-1)*`DSP_USED_OUTPUT_WIDTH]),
              .clk(clk),
              .reset(reset_reduction_mvm[i-1]),
              .start(start[i-1]),
              .out_data_available(out_data_available_1_stage_1[i-1]),
              .sum(reduction_output_1_stage_1[i*`DSP_USED_OUTPUT_WIDTH-1:(i-1)*`DSP_USED_OUTPUT_WIDTH])
            );
        end
    endgenerate

    wire[(`DSP_USED_OUTPUT_WIDTH)*`NUM_LDPES-1:0] reduction_output_0_stage_2;
    wire[`NUM_LDPES-1:0] out_data_available_0_stage_2;

    generate
        for(i=1; i<=`NUM_LDPES; i=i+1) begin: gen_adder3
           myadder #(.INPUT_WIDTH(`DSP_USED_OUTPUT_WIDTH),.OUTPUT_WIDTH(`DSP_USED_OUTPUT_WIDTH)) adder_units_0_stage_1 (
              .a(reduction_output_0_stage_1[i*(`DSP_USED_OUTPUT_WIDTH)-1:(i-1)*(`DSP_USED_OUTPUT_WIDTH)]),
              .b(reduction_output_1_stage_1[i*(`DSP_USED_OUTPUT_WIDTH)-1:(i-1)*(`DSP_USED_OUTPUT_WIDTH)]),
              .clk(clk),
              .reset(reset_reduction_mvm[i-1]),
              .start(out_data_available_0_stage_1[i-1]),
              .out_data_available(out_data_available_0_stage_2[i-1]),
              .sum(reduction_output_0_stage_2[i*(`DSP_USED_OUTPUT_WIDTH)-1:(i-1)*(`DSP_USED_OUTPUT_WIDTH)])
            );
        end
    endgenerate

assign result_mvm_final_stage[1*`OUT_DWIDTH-1:0*`OUT_DWIDTH] = reduction_output_0_stage_2[0*(`DSP_USED_OUTPUT_WIDTH)+`OUT_DWIDTH-1:0*(`DSP_USED_OUTPUT_WIDTH)];
assign result_mvm_final_stage[2*`OUT_DWIDTH-1:1*`OUT_DWIDTH] = reduction_output_0_stage_2[1*(`DSP_USED_OUTPUT_WIDTH)+`OUT_DWIDTH-1:1*(`DSP_USED_OUTPUT_WIDTH)];
assign result_mvm_final_stage[3*`OUT_DWIDTH-1:2*`OUT_DWIDTH] = reduction_output_0_stage_2[2*(`DSP_USED_OUTPUT_WIDTH)+`OUT_DWIDTH-1:2*(`DSP_USED_OUTPUT_WIDTH)];
assign result_mvm_final_stage[4*`OUT_DWIDTH-1:3*`OUT_DWIDTH] = reduction_output_0_stage_2[3*(`DSP_USED_OUTPUT_WIDTH)+`OUT_DWIDTH-1:3*(`DSP_USED_OUTPUT_WIDTH)];
assign result_mvm_final_stage[5*`OUT_DWIDTH-1:4*`OUT_DWIDTH] = reduction_output_0_stage_2[4*(`DSP_USED_OUTPUT_WIDTH)+`OUT_DWIDTH-1:4*(`DSP_USED_OUTPUT_WIDTH)];
assign result_mvm_final_stage[6*`OUT_DWIDTH-1:5*`OUT_DWIDTH] = reduction_output_0_stage_2[5*(`DSP_USED_OUTPUT_WIDTH)+`OUT_DWIDTH-1:5*(`DSP_USED_OUTPUT_WIDTH)];
assign result_mvm_final_stage[7*`OUT_DWIDTH-1:6*`OUT_DWIDTH] = reduction_output_0_stage_2[6*(`DSP_USED_OUTPUT_WIDTH)+`OUT_DWIDTH-1:6*(`DSP_USED_OUTPUT_WIDTH)];
assign result_mvm_final_stage[8*`OUT_DWIDTH-1:7*`OUT_DWIDTH] = reduction_output_0_stage_2[7*(`DSP_USED_OUTPUT_WIDTH)+`OUT_DWIDTH-1:7*(`DSP_USED_OUTPUT_WIDTH)];
assign result_mvm_final_stage[9*`OUT_DWIDTH-1:8*`OUT_DWIDTH] = reduction_output_0_stage_2[8*(`DSP_USED_OUTPUT_WIDTH)+`OUT_DWIDTH-1:8*(`DSP_USED_OUTPUT_WIDTH)];
assign result_mvm_final_stage[10*`OUT_DWIDTH-1:9*`OUT_DWIDTH] = reduction_output_0_stage_2[9*(`DSP_USED_OUTPUT_WIDTH)+`OUT_DWIDTH-1:9*(`DSP_USED_OUTPUT_WIDTH)];
assign result_mvm_final_stage[11*`OUT_DWIDTH-1:10*`OUT_DWIDTH] = reduction_output_0_stage_2[10*(`DSP_USED_OUTPUT_WIDTH)+`OUT_DWIDTH-1:10*(`DSP_USED_OUTPUT_WIDTH)];
assign result_mvm_final_stage[12*`OUT_DWIDTH-1:11*`OUT_DWIDTH] = reduction_output_0_stage_2[11*(`DSP_USED_OUTPUT_WIDTH)+`OUT_DWIDTH-1:11*(`DSP_USED_OUTPUT_WIDTH)];
assign result_mvm_final_stage[13*`OUT_DWIDTH-1:12*`OUT_DWIDTH] = reduction_output_0_stage_2[12*(`DSP_USED_OUTPUT_WIDTH)+`OUT_DWIDTH-1:12*(`DSP_USED_OUTPUT_WIDTH)];
assign result_mvm_final_stage[14*`OUT_DWIDTH-1:13*`OUT_DWIDTH] = reduction_output_0_stage_2[13*(`DSP_USED_OUTPUT_WIDTH)+`OUT_DWIDTH-1:13*(`DSP_USED_OUTPUT_WIDTH)];
assign result_mvm_final_stage[15*`OUT_DWIDTH-1:14*`OUT_DWIDTH] = reduction_output_0_stage_2[14*(`DSP_USED_OUTPUT_WIDTH)+`OUT_DWIDTH-1:14*(`DSP_USED_OUTPUT_WIDTH)];
assign result_mvm_final_stage[16*`OUT_DWIDTH-1:15*`OUT_DWIDTH] = reduction_output_0_stage_2[15*(`DSP_USED_OUTPUT_WIDTH)+`OUT_DWIDTH-1:15*(`DSP_USED_OUTPUT_WIDTH)];
assign out_data_available = out_data_available_0_stage_2;
endmodule
////////////////////////////////////////////////////////////////////////////////
// THIS FILE WAS AUTOMATICALLY GENERATED FROM mfu.v.mako
// DO NOT EDIT
////////////////////////////////////////////////////////////////////////////////


module MFU( 
    input[1:0] activation_type,
    input[1:0] operation,
    input in_data_available,
    input [`ORF_AWIDTH-1:0] vrf_addr_read_add,
    input [`ORF_AWIDTH-1:0] vrf_addr_wr_add,
    input vrf_readn_enable_add,
    input vrf_wr_enable_add,
    input [`ORF_AWIDTH-1:0] vrf_addr_read_mul,
    input [`ORF_AWIDTH-1:0] vrf_addr_wr_mul,
    input vrf_readn_enable_mul,
    input vrf_wr_enable_mul,
    
    input [`DESIGN_SIZE*`DWIDTH-1:0] primary_inp,
    
    input [`ORF_DWIDTH-1:0] secondary_inp,
    output reg [`DESIGN_SIZE*`DWIDTH-1:0] out_data,
    output done,
    output out_data_available,
    input clk,
    output [`ORF_DWIDTH-1:0] out_vrf_add,
    output [`ORF_DWIDTH-1:0] out_vrf_mul,
    input reset
);

    wire enable_add;
    wire enable_activation;
    wire enable_mul;
    wire done_compute_unit_for_add_mul_act ;

    assign enable_activation = (~operation[0])&(~operation[1])&(~reset);
    
    assign enable_add = (operation[0])&(~operation[1])&(~reset);
    assign enable_mul = (~operation[0])&(operation[1])&(~reset);

    wire [`ORF_DWIDTH-1:0] ina_fake;
    wire [`ORF_DWIDTH-1:0] vrf_outa_add_for_compute;
    wire [`ORF_DWIDTH-1:0] vrf_outa_mul_for_compute;
    //wire [`ORF_DWIDTH-1:0] out_vrf_add;

    wire [`DESIGN_SIZE*`DWIDTH-1:0] out_data_add;
    wire [`DESIGN_SIZE*`DWIDTH-1:0] out_data_mul;
    wire [`DESIGN_SIZE*`DWIDTH-1:0] out_data_act;
    
    wire[`DESIGN_SIZE*`DWIDTH-1:0] compute_operand_1_add;
    wire[`DESIGN_SIZE*`DWIDTH-1:0] compute_operand_1_mul;
    wire[`DESIGN_SIZE*`DWIDTH-1:0] compute_operand_1_act;
    
    assign compute_operand_1_add = ((in_data_available==1'b1)&enable_add) ? primary_inp : 'd0;
    assign compute_operand_1_mul = ((in_data_available==1'b1)&enable_mul) ? primary_inp : 'd0;
    assign compute_operand_1_act = ((in_data_available==1'b1)&enable_activation) ? primary_inp : 'd0;

    wire[`DESIGN_SIZE*`DWIDTH-1:0] compute_operand_2_add;                    
                                                                         
    assign compute_operand_2_add = ((in_data_available==1'b1)&enable_add) ?vrf_outa_add_for_compute:'d0;
    
    wire[`DESIGN_SIZE*`DWIDTH-1:0] compute_operand_2_mul;                    
                                                                         
    assign compute_operand_2_mul = ((in_data_available==1'b1)&enable_mul) ?vrf_outa_mul_for_compute:'d0;
    
    VRF #(.VRF_DWIDTH(`ORF_DWIDTH),.VRF_AWIDTH(`ORF_AWIDTH)) v0(.clk(clk),.addra(vrf_addr_wr_add),.addrb(vrf_addr_read_add),.inb(ina_fake),.ina(secondary_inp),.wea(vrf_wr_enable_add),.web(vrf_readn_enable_add),.outb(vrf_outa_add_for_compute),.outa(out_vrf_add));

    VRF #(.VRF_DWIDTH(`ORF_DWIDTH),.VRF_AWIDTH(`ORF_AWIDTH)) v1(.clk(clk),.addra(vrf_addr_wr_mul),.addrb(vrf_addr_read_mul),.inb(ina_fake),.ina(secondary_inp),.wea(vrf_wr_enable_mul),.web(vrf_readn_enable_mul),.outb(vrf_outa_mul_for_compute),.outa(out_vrf_mul));
 
    always@(*) begin
      if(in_data_available==0) begin
         out_data = 'd0;
      end
      else begin
         case(operation) 
            `ACTIVATION: begin out_data = out_data_act;
            end
            `ELT_WISE_ADD: begin out_data = out_data_add;
            end
            `ELT_WISE_MULTIPLY: begin out_data = out_data_mul;
            end
            `BYPASS: begin out_data = primary_inp; //Bypass the MFU
            end
            default: begin out_data = 0;
            end
         endcase
      end
    end
    //FOR ELTWISE ADD-MUL, THE OPERATION IS DONE WHEN THE OUTPUT IS AVAILABLE AT THE OUTPUT PORT
    
    wire done_add;
    wire add_or_sub;
    assign add_or_sub = activation_type[0];

    elt_wise_add elt_add_unit(
        .enable_add(enable_add),
        .primary_inp(compute_operand_1_add),
        .in_data_available(in_data_available),
        .secondary_inp(compute_operand_2_add),
        .out_data(out_data_add),
        .add_or_sub(add_or_sub), //IMP
        .output_available_add(done_add),
        .clk(clk)
      );
   
    wire done_mul;
    elt_wise_mul elt_mul_unit(
        .enable_mul(enable_mul),
        .in_data_available(in_data_available),
        .primary_inp(compute_operand_1_mul),
        .secondary_inp(compute_operand_2_mul),
        .out_data(out_data_mul),
        .output_available_mul(done_mul),
        .clk(clk)
      );
    //
    
    wire out_data_available_act;

    wire done_activation;
    activation act_unit(
    .activation_type(activation_type),
    .enable_activation(enable_activation),
    .in_data_available(in_data_available),
    .inp_data(compute_operand_1_act),
    .out_data(out_data_act),
    .out_data_available(out_data_available_act),
    .done_activation(done_activation),
    .clk(clk),
    .reset(reset)
    );
   
   //OUT DATA AVAILABLE IS NOT IMPORTANT HERE (CORRESPONDS TO DONE) BUT STILL LETS KEEP IT
   assign done_compute_unit_for_add_mul_act = done_activation|done_add|done_mul;
   assign done = done_compute_unit_for_add_mul_act|(operation==`BYPASS);
   
   assign out_data_available = (out_data_available_act | done_add | done_mul);

   //TODO: demarcate the nomenclature for out_data_available and done signal separately - DONE.
endmodule


module mult(
    input [(`DWIDTH)-1:0] x, 
    input [(`DWIDTH)-1:0] y,
    input clk,
    input reset,
    output [`DWIDTH-1:0] p
 );
    reg [2*`DWIDTH-1:0] mult_result;

    always @(posedge clk) begin 
    //$display("p '%'d a '%'d b '%'d",p,x,y);
        if(reset==0) begin
            mult_result <= x*y;
        end
    end
    
    //GET TRUNCATED RESULT 
    assign p = mult_result[`DWIDTH-1:0];
    
endmodule

module add( 
    input [`DWIDTH-1:0] x,
    input [`DWIDTH-1:0] y,
    input clk,
    input reset,
    output reg [`DWIDTH-1:0] p
 );
    

    always @(posedge clk) begin 
    //$display("p '%'d a '%'d b '%'d",p,x,y);
        if(reset==0) begin
            p <= x + y;
        end
    end
    
endmodule

module activation(
    input[1:0] activation_type,
    input enable_activation,
    input in_data_available,
    input [`DESIGN_SIZE*`DWIDTH-1:0] inp_data,
    output [`DESIGN_SIZE*`DWIDTH-1:0] out_data,
    output out_data_available,
    output done_activation,
    input clk,
    input reset
);

reg  done_activation_internal;
reg  out_data_available_internal;
wire [`DESIGN_SIZE*`DWIDTH-1:0] out_data_internal;
reg [`DESIGN_SIZE*`DWIDTH-1:0] slope_applied_data_internal;
reg [`DESIGN_SIZE*`DWIDTH-1:0] intercept_applied_data_internal;
reg [`DESIGN_SIZE*`DWIDTH-1:0] relu_applied_data_internal;
integer i;
integer cycle_count;
reg activation_in_progress;

reg [(`DESIGN_SIZE*4)-1:0] address;

reg [(`DESIGN_SIZE*`DWIDTH)-1:0] data_slope_tanh;
reg [(`DESIGN_SIZE*`DWIDTH)-1:0] data_intercept_tanh;
reg [(`DESIGN_SIZE*`DWIDTH)-1:0] data_slope_sigmoid;
reg [(`DESIGN_SIZE*`DWIDTH)-1:0] data_intercept_sigmoid;

reg [(`DESIGN_SIZE*`DWIDTH)-1:0] data_intercept_delayed;

// If the activation block is not enabled, just forward the input data
assign out_data             = enable_activation ? out_data_internal : 'd0;
assign done_activation      = enable_activation ? done_activation_internal : 1'b0;
assign out_data_available   = enable_activation ? out_data_available_internal : in_data_available;

always @(posedge clk) begin
   if (reset || ~enable_activation) begin
      slope_applied_data_internal     <= 0;
      intercept_applied_data_internal <= 0; 
      relu_applied_data_internal      <= 0; 
      data_intercept_delayed      <= 0;
      done_activation_internal    <= 0;
      out_data_available_internal <= 0;
      cycle_count                 <= 0;
      activation_in_progress      <= 0;
   end else if(in_data_available || activation_in_progress) begin
      cycle_count <= cycle_count + 1;

      for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
         if(activation_type==1) begin // tanH
            slope_applied_data_internal[i*`DWIDTH +:`DWIDTH] <= data_slope_tanh[i*8 +: 8] * inp_data[i*`DWIDTH +:`DWIDTH];
            data_intercept_delayed[i*8 +: 8] <= data_intercept_tanh[i*8 +: 8];
            intercept_applied_data_internal[i*`DWIDTH +:`DWIDTH] <= slope_applied_data_internal[i*`DWIDTH +:`DWIDTH] + data_intercept_delayed[i*8 +: 8];
         end 
         else if(activation_type==2) begin // tanH
            slope_applied_data_internal[i*`DWIDTH +:`DWIDTH] <= data_slope_sigmoid[i*8 +: 8] * inp_data[i*`DWIDTH +:`DWIDTH];
            data_intercept_delayed[i*8 +: 8] <= data_intercept_sigmoid[i*8 +: 8];
            intercept_applied_data_internal[i*`DWIDTH +:`DWIDTH] <= slope_applied_data_internal[i*`DWIDTH +:`DWIDTH] + data_intercept_delayed[i*8 +: 8];
         end else begin // ReLU
            relu_applied_data_internal[i*`DWIDTH +:`DWIDTH] <= inp_data[i*`DWIDTH] ? {`DWIDTH{1'b0}} : inp_data[i*`DWIDTH +:`DWIDTH];
         end
      end   

      //TANH needs 1 extra cycle
      if ((activation_type==1) || (activation_type==2)) begin
         if (cycle_count==`TANH_LATENCY-1) begin
            out_data_available_internal <= 1;
         end
      end else begin
         if (cycle_count==`ACTIVATION_LATENCY-1) begin
           out_data_available_internal <= 1;
         end
      end

      //TANH needs 1 extra cycle
      if ((activation_type==1) || (activation_type==2)) begin
        if(cycle_count==`TANH_LATENCY-1) begin //REPLACED DESIGN SIZE WITH 1 on the LEFT ****************************************
           done_activation_internal <= 1'b1;
           activation_in_progress <= 0;
        end
        else begin
           activation_in_progress <= 1;
        end
      end else begin
        if(cycle_count==`ACTIVATION_LATENCY-1) begin //REPLACED DESIGN SIZE WITH 1 on the LEFT ************************************
           done_activation_internal <= 1'b1;
           activation_in_progress <= 0;
        end
        else begin
           activation_in_progress <= 1;
        end
      end
   end
   else begin
      slope_applied_data_internal     <= 0;
      intercept_applied_data_internal <= 0; 
      relu_applied_data_internal      <= 0; 
      data_intercept_delayed      <= 0;
      done_activation_internal    <= 0;
      out_data_available_internal <= 0;
      cycle_count                 <= 0;
      activation_in_progress      <= 0;
   end
end

assign out_data_internal = (activation_type) ? intercept_applied_data_internal : relu_applied_data_internal;

//Our equation of tanh is Y=AX+B
//A is the slope and B is the intercept.
//We store A in one LUT and B in another.
//LUT for the slope
always @(address) begin
    for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
    case (address[i*4+:4])
      4'b0000: data_slope_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
      4'b0001: data_slope_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
      4'b0010: data_slope_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd2;
      4'b0011: data_slope_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd3;
      4'b0100: data_slope_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd4;
      4'b0101: data_slope_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
      4'b0110: data_slope_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd4;
      4'b0111: data_slope_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd3;
      4'b1000: data_slope_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd2;
      4'b1001: data_slope_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
      4'b1010: data_slope_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
      default: data_slope_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
    endcase  
    end
end

//LUT for the intercept
always @(address) begin
    for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
    case (address[i*4+:4])
      4'b0000: data_intercept_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd127;
      4'b0001: data_intercept_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd99;
      4'b0010: data_intercept_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd46;
      4'b0011: data_intercept_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd18;
      4'b0100: data_intercept_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
      4'b0101: data_intercept_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
      4'b0110: data_intercept_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
      4'b0111: data_intercept_tanh[i*`DWIDTH+:`DWIDTH] = -`DWIDTH'd18;
      4'b1000: data_intercept_tanh[i*`DWIDTH+:`DWIDTH] = -`DWIDTH'd46;
      4'b1001: data_intercept_tanh[i*`DWIDTH+:`DWIDTH] = -`DWIDTH'd99;
      4'b1010: data_intercept_tanh[i*`DWIDTH+:`DWIDTH] = -`DWIDTH'd127;
      default: data_intercept_tanh[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
    endcase  
    end
end

always @(address) begin
    for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
    case (address[i*4+:4])
      4'b0000: data_slope_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
      4'b0001: data_slope_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
      4'b0010: data_slope_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd2;
      4'b0011: data_slope_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd3;
      4'b0100: data_slope_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd4;
      4'b0101: data_slope_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
      4'b0110: data_slope_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd4;
      4'b0111: data_slope_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd3;
      4'b1000: data_slope_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd2;
      4'b1001: data_slope_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
      4'b1010: data_slope_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
      default: data_slope_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
    endcase  
    end
end

//LUT for the intercept
always @(address) begin
    for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
    case (address[i*4+:4])
      4'b0000: data_intercept_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd127;
      4'b0001: data_intercept_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd99;
      4'b0010: data_intercept_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd46;
      4'b0011: data_intercept_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd18;
      4'b0100: data_intercept_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
      4'b0101: data_intercept_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
      4'b0110: data_intercept_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
      4'b0111: data_intercept_sigmoid[i*`DWIDTH+:`DWIDTH] = -`DWIDTH'd18;
      4'b1000: data_intercept_sigmoid[i*`DWIDTH+:`DWIDTH] = -`DWIDTH'd46;
      4'b1001: data_intercept_sigmoid[i*`DWIDTH+:`DWIDTH] = -`DWIDTH'd99;
      4'b1010: data_intercept_sigmoid[i*`DWIDTH+:`DWIDTH] = -`DWIDTH'd127;
      default: data_intercept_sigmoid[i*`DWIDTH+:`DWIDTH] = `DWIDTH'd0;
    endcase  
    end
end

//Logic to find address
always @(inp_data) begin
    for (i = 0; i < `DESIGN_SIZE; i=i+1) begin
        if((inp_data[i*`DWIDTH +:`DWIDTH])>=90) begin
           address[i*4+:4] = 4'b0000;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>=39 && (inp_data[i*`DWIDTH +:`DWIDTH])<90) begin
           address[i*4+:4] = 4'b0001;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>=28 && (inp_data[i*`DWIDTH +:`DWIDTH])<39) begin
           address[i*4+:4] = 4'b0010;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>=16 && (inp_data[i*`DWIDTH +:`DWIDTH])<28) begin
           address[i*4+:4] = 4'b0011;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>=1 && (inp_data[i*`DWIDTH +:`DWIDTH])<16) begin
           address[i*4+:4] = 4'b0100;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])==0) begin
           address[i*4+:4] = 4'b0101;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>-16 && (inp_data[i*`DWIDTH +:`DWIDTH])<=-1) begin
           address[i*4+:4] = 4'b0110;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>-28 && (inp_data[i*`DWIDTH +:`DWIDTH])<=-16) begin
           address[i*4+:4] = 4'b0111;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>-39 && (inp_data[i*`DWIDTH +:`DWIDTH])<=-28) begin
           address[i*4+:4] = 4'b1000;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])>-90 && (inp_data[i*`DWIDTH +:`DWIDTH])<=-39) begin
           address[i*4+:4] = 4'b1001;
        end
        else if ((inp_data[i*`DWIDTH +:`DWIDTH])<=-90) begin
           address[i*4+:4] = 4'b1010;
        end
        else begin
           address[i*4+:4] = 4'b0101;
        end
    end
end

endmodule


module elt_wise_add(
    input enable_add,
    input in_data_available,
    input add_or_sub,
    input [`NUM_LDPES*`DWIDTH-1:0] primary_inp,
    input [`NUM_LDPES*`DWIDTH-1:0] secondary_inp,
    output [`NUM_LDPES*`DWIDTH-1:0] out_data,
    output reg output_available_add,
    input clk
);
    wire [(`DWIDTH)-1:0] x_0; 
    wire [(`DWIDTH)-1:0] y_0;
    
    add a0(.p(out_data[(1*`DWIDTH)-1:(0*`DWIDTH)]),.x(x_0),.y(y_0), .clk(clk), .reset(~enable_add));
    wire [(`DWIDTH)-1:0] x_1; 
    wire [(`DWIDTH)-1:0] y_1;
    
    add a1(.p(out_data[(2*`DWIDTH)-1:(1*`DWIDTH)]),.x(x_1),.y(y_1), .clk(clk), .reset(~enable_add));
    wire [(`DWIDTH)-1:0] x_2; 
    wire [(`DWIDTH)-1:0] y_2;
    
    add a2(.p(out_data[(3*`DWIDTH)-1:(2*`DWIDTH)]),.x(x_2),.y(y_2), .clk(clk), .reset(~enable_add));
    wire [(`DWIDTH)-1:0] x_3; 
    wire [(`DWIDTH)-1:0] y_3;
    
    add a3(.p(out_data[(4*`DWIDTH)-1:(3*`DWIDTH)]),.x(x_3),.y(y_3), .clk(clk), .reset(~enable_add));
    wire [(`DWIDTH)-1:0] x_4; 
    wire [(`DWIDTH)-1:0] y_4;
    
    add a4(.p(out_data[(5*`DWIDTH)-1:(4*`DWIDTH)]),.x(x_4),.y(y_4), .clk(clk), .reset(~enable_add));
    wire [(`DWIDTH)-1:0] x_5; 
    wire [(`DWIDTH)-1:0] y_5;
    
    add a5(.p(out_data[(6*`DWIDTH)-1:(5*`DWIDTH)]),.x(x_5),.y(y_5), .clk(clk), .reset(~enable_add));
    wire [(`DWIDTH)-1:0] x_6; 
    wire [(`DWIDTH)-1:0] y_6;
    
    add a6(.p(out_data[(7*`DWIDTH)-1:(6*`DWIDTH)]),.x(x_6),.y(y_6), .clk(clk), .reset(~enable_add));
    wire [(`DWIDTH)-1:0] x_7; 
    wire [(`DWIDTH)-1:0] y_7;
    
    add a7(.p(out_data[(8*`DWIDTH)-1:(7*`DWIDTH)]),.x(x_7),.y(y_7), .clk(clk), .reset(~enable_add));
    wire [(`DWIDTH)-1:0] x_8; 
    wire [(`DWIDTH)-1:0] y_8;
    
    add a8(.p(out_data[(9*`DWIDTH)-1:(8*`DWIDTH)]),.x(x_8),.y(y_8), .clk(clk), .reset(~enable_add));
    wire [(`DWIDTH)-1:0] x_9; 
    wire [(`DWIDTH)-1:0] y_9;
    
    add a9(.p(out_data[(10*`DWIDTH)-1:(9*`DWIDTH)]),.x(x_9),.y(y_9), .clk(clk), .reset(~enable_add));
    wire [(`DWIDTH)-1:0] x_10; 
    wire [(`DWIDTH)-1:0] y_10;
    
    add a10(.p(out_data[(11*`DWIDTH)-1:(10*`DWIDTH)]),.x(x_10),.y(y_10), .clk(clk), .reset(~enable_add));
    wire [(`DWIDTH)-1:0] x_11; 
    wire [(`DWIDTH)-1:0] y_11;
    
    add a11(.p(out_data[(12*`DWIDTH)-1:(11*`DWIDTH)]),.x(x_11),.y(y_11), .clk(clk), .reset(~enable_add));
    wire [(`DWIDTH)-1:0] x_12; 
    wire [(`DWIDTH)-1:0] y_12;
    
    add a12(.p(out_data[(13*`DWIDTH)-1:(12*`DWIDTH)]),.x(x_12),.y(y_12), .clk(clk), .reset(~enable_add));
    wire [(`DWIDTH)-1:0] x_13; 
    wire [(`DWIDTH)-1:0] y_13;
    
    add a13(.p(out_data[(14*`DWIDTH)-1:(13*`DWIDTH)]),.x(x_13),.y(y_13), .clk(clk), .reset(~enable_add));
    wire [(`DWIDTH)-1:0] x_14; 
    wire [(`DWIDTH)-1:0] y_14;
    
    add a14(.p(out_data[(15*`DWIDTH)-1:(14*`DWIDTH)]),.x(x_14),.y(y_14), .clk(clk), .reset(~enable_add));
    wire [(`DWIDTH)-1:0] x_15; 
    wire [(`DWIDTH)-1:0] y_15;
    
    add a15(.p(out_data[(16*`DWIDTH)-1:(15*`DWIDTH)]),.x(x_15),.y(y_15), .clk(clk), .reset(~enable_add));

    assign x_0 = primary_inp[(1*`DWIDTH)-1:(0*`DWIDTH)];
    assign x_1 = primary_inp[(2*`DWIDTH)-1:(1*`DWIDTH)];
    assign x_2 = primary_inp[(3*`DWIDTH)-1:(2*`DWIDTH)];
    assign x_3 = primary_inp[(4*`DWIDTH)-1:(3*`DWIDTH)];
    assign x_4 = primary_inp[(5*`DWIDTH)-1:(4*`DWIDTH)];
    assign x_5 = primary_inp[(6*`DWIDTH)-1:(5*`DWIDTH)];
    assign x_6 = primary_inp[(7*`DWIDTH)-1:(6*`DWIDTH)];
    assign x_7 = primary_inp[(8*`DWIDTH)-1:(7*`DWIDTH)];
    assign x_8 = primary_inp[(9*`DWIDTH)-1:(8*`DWIDTH)];
    assign x_9 = primary_inp[(10*`DWIDTH)-1:(9*`DWIDTH)];
    assign x_10 = primary_inp[(11*`DWIDTH)-1:(10*`DWIDTH)];
    assign x_11 = primary_inp[(12*`DWIDTH)-1:(11*`DWIDTH)];
    assign x_12 = primary_inp[(13*`DWIDTH)-1:(12*`DWIDTH)];
    assign x_13 = primary_inp[(14*`DWIDTH)-1:(13*`DWIDTH)];
    assign x_14 = primary_inp[(15*`DWIDTH)-1:(14*`DWIDTH)];
    assign x_15 = primary_inp[(16*`DWIDTH)-1:(15*`DWIDTH)];

    assign y_0 = secondary_inp[(1*`DWIDTH)-1:(0*`DWIDTH)];
    assign y_1 = secondary_inp[(2*`DWIDTH)-1:(1*`DWIDTH)];
    assign y_2 = secondary_inp[(3*`DWIDTH)-1:(2*`DWIDTH)];
    assign y_3 = secondary_inp[(4*`DWIDTH)-1:(3*`DWIDTH)];
    assign y_4 = secondary_inp[(5*`DWIDTH)-1:(4*`DWIDTH)];
    assign y_5 = secondary_inp[(6*`DWIDTH)-1:(5*`DWIDTH)];
    assign y_6 = secondary_inp[(7*`DWIDTH)-1:(6*`DWIDTH)];
    assign y_7 = secondary_inp[(8*`DWIDTH)-1:(7*`DWIDTH)];
    assign y_8 = secondary_inp[(9*`DWIDTH)-1:(8*`DWIDTH)];
    assign y_9 = secondary_inp[(10*`DWIDTH)-1:(9*`DWIDTH)];
    assign y_10 = secondary_inp[(11*`DWIDTH)-1:(10*`DWIDTH)];
    assign y_11 = secondary_inp[(12*`DWIDTH)-1:(11*`DWIDTH)];
    assign y_12 = secondary_inp[(13*`DWIDTH)-1:(12*`DWIDTH)];
    assign y_13 = secondary_inp[(14*`DWIDTH)-1:(13*`DWIDTH)];
    assign y_14 = secondary_inp[(15*`DWIDTH)-1:(14*`DWIDTH)];
    assign y_15 = secondary_inp[(16*`DWIDTH)-1:(15*`DWIDTH)];

     reg[`LOG_ADD_LATENCY-1:0] state;
     always @(posedge clk) begin
        if((enable_add==1'b1) && (in_data_available==1'b1)) begin   
            if(state!=`ADD_LATENCY) begin 
                state<=state+1'b1;
            end
            else begin
                output_available_add<=1;
                state<=0;
            end
        end
        else begin
          output_available_add<=0;
          state<=0;
        end
     end
     
endmodule
module elt_wise_mul(
    input enable_mul,
    input in_data_available,
    input [`DESIGN_SIZE*`DWIDTH-1:0] primary_inp,
    input [`DESIGN_SIZE*`DWIDTH-1:0] secondary_inp,
    output [`DESIGN_SIZE*`DWIDTH-1:0] out_data,
    output reg output_available_mul,
    input clk
);
    wire [(`DWIDTH)-1:0] x_0; 
    wire [(`DWIDTH)-1:0] y_0;
    
    mult m0(.p(out_data[(1*`DWIDTH)-1:(0*`DWIDTH)]),.x(x_0),.y(y_0), .clk(clk), .reset(~enable_mul));
    wire [(`DWIDTH)-1:0] x_1; 
    wire [(`DWIDTH)-1:0] y_1;
    
    mult m1(.p(out_data[(2*`DWIDTH)-1:(1*`DWIDTH)]),.x(x_1),.y(y_1), .clk(clk), .reset(~enable_mul));
    wire [(`DWIDTH)-1:0] x_2; 
    wire [(`DWIDTH)-1:0] y_2;
    
    mult m2(.p(out_data[(3*`DWIDTH)-1:(2*`DWIDTH)]),.x(x_2),.y(y_2), .clk(clk), .reset(~enable_mul));
    wire [(`DWIDTH)-1:0] x_3; 
    wire [(`DWIDTH)-1:0] y_3;
    
    mult m3(.p(out_data[(4*`DWIDTH)-1:(3*`DWIDTH)]),.x(x_3),.y(y_3), .clk(clk), .reset(~enable_mul));
    wire [(`DWIDTH)-1:0] x_4; 
    wire [(`DWIDTH)-1:0] y_4;
    
    mult m4(.p(out_data[(5*`DWIDTH)-1:(4*`DWIDTH)]),.x(x_4),.y(y_4), .clk(clk), .reset(~enable_mul));
    wire [(`DWIDTH)-1:0] x_5; 
    wire [(`DWIDTH)-1:0] y_5;
    
    mult m5(.p(out_data[(6*`DWIDTH)-1:(5*`DWIDTH)]),.x(x_5),.y(y_5), .clk(clk), .reset(~enable_mul));
    wire [(`DWIDTH)-1:0] x_6; 
    wire [(`DWIDTH)-1:0] y_6;
    
    mult m6(.p(out_data[(7*`DWIDTH)-1:(6*`DWIDTH)]),.x(x_6),.y(y_6), .clk(clk), .reset(~enable_mul));
    wire [(`DWIDTH)-1:0] x_7; 
    wire [(`DWIDTH)-1:0] y_7;
    
    mult m7(.p(out_data[(8*`DWIDTH)-1:(7*`DWIDTH)]),.x(x_7),.y(y_7), .clk(clk), .reset(~enable_mul));
    wire [(`DWIDTH)-1:0] x_8; 
    wire [(`DWIDTH)-1:0] y_8;
    
    mult m8(.p(out_data[(9*`DWIDTH)-1:(8*`DWIDTH)]),.x(x_8),.y(y_8), .clk(clk), .reset(~enable_mul));
    wire [(`DWIDTH)-1:0] x_9; 
    wire [(`DWIDTH)-1:0] y_9;
    
    mult m9(.p(out_data[(10*`DWIDTH)-1:(9*`DWIDTH)]),.x(x_9),.y(y_9), .clk(clk), .reset(~enable_mul));
    wire [(`DWIDTH)-1:0] x_10; 
    wire [(`DWIDTH)-1:0] y_10;
    
    mult m10(.p(out_data[(11*`DWIDTH)-1:(10*`DWIDTH)]),.x(x_10),.y(y_10), .clk(clk), .reset(~enable_mul));
    wire [(`DWIDTH)-1:0] x_11; 
    wire [(`DWIDTH)-1:0] y_11;
    
    mult m11(.p(out_data[(12*`DWIDTH)-1:(11*`DWIDTH)]),.x(x_11),.y(y_11), .clk(clk), .reset(~enable_mul));
    wire [(`DWIDTH)-1:0] x_12; 
    wire [(`DWIDTH)-1:0] y_12;
    
    mult m12(.p(out_data[(13*`DWIDTH)-1:(12*`DWIDTH)]),.x(x_12),.y(y_12), .clk(clk), .reset(~enable_mul));
    wire [(`DWIDTH)-1:0] x_13; 
    wire [(`DWIDTH)-1:0] y_13;
    
    mult m13(.p(out_data[(14*`DWIDTH)-1:(13*`DWIDTH)]),.x(x_13),.y(y_13), .clk(clk), .reset(~enable_mul));
    wire [(`DWIDTH)-1:0] x_14; 
    wire [(`DWIDTH)-1:0] y_14;
    
    mult m14(.p(out_data[(15*`DWIDTH)-1:(14*`DWIDTH)]),.x(x_14),.y(y_14), .clk(clk), .reset(~enable_mul));
    wire [(`DWIDTH)-1:0] x_15; 
    wire [(`DWIDTH)-1:0] y_15;
    
    mult m15(.p(out_data[(16*`DWIDTH)-1:(15*`DWIDTH)]),.x(x_15),.y(y_15), .clk(clk), .reset(~enable_mul));

    assign x_0 = primary_inp[(1*`DWIDTH)-1:(0*`DWIDTH)];
    assign x_1 = primary_inp[(2*`DWIDTH)-1:(1*`DWIDTH)];
    assign x_2 = primary_inp[(3*`DWIDTH)-1:(2*`DWIDTH)];
    assign x_3 = primary_inp[(4*`DWIDTH)-1:(3*`DWIDTH)];
    assign x_4 = primary_inp[(5*`DWIDTH)-1:(4*`DWIDTH)];
    assign x_5 = primary_inp[(6*`DWIDTH)-1:(5*`DWIDTH)];
    assign x_6 = primary_inp[(7*`DWIDTH)-1:(6*`DWIDTH)];
    assign x_7 = primary_inp[(8*`DWIDTH)-1:(7*`DWIDTH)];
    assign x_8 = primary_inp[(9*`DWIDTH)-1:(8*`DWIDTH)];
    assign x_9 = primary_inp[(10*`DWIDTH)-1:(9*`DWIDTH)];
    assign x_10 = primary_inp[(11*`DWIDTH)-1:(10*`DWIDTH)];
    assign x_11 = primary_inp[(12*`DWIDTH)-1:(11*`DWIDTH)];
    assign x_12 = primary_inp[(13*`DWIDTH)-1:(12*`DWIDTH)];
    assign x_13 = primary_inp[(14*`DWIDTH)-1:(13*`DWIDTH)];
    assign x_14 = primary_inp[(15*`DWIDTH)-1:(14*`DWIDTH)];
    assign x_15 = primary_inp[(16*`DWIDTH)-1:(15*`DWIDTH)];

    assign y_0 = secondary_inp[(1*`DWIDTH)-1:(0*`DWIDTH)];
    assign y_1 = secondary_inp[(2*`DWIDTH)-1:(1*`DWIDTH)];
    assign y_2 = secondary_inp[(3*`DWIDTH)-1:(2*`DWIDTH)];
    assign y_3 = secondary_inp[(4*`DWIDTH)-1:(3*`DWIDTH)];
    assign y_4 = secondary_inp[(5*`DWIDTH)-1:(4*`DWIDTH)];
    assign y_5 = secondary_inp[(6*`DWIDTH)-1:(5*`DWIDTH)];
    assign y_6 = secondary_inp[(7*`DWIDTH)-1:(6*`DWIDTH)];
    assign y_7 = secondary_inp[(8*`DWIDTH)-1:(7*`DWIDTH)];
    assign y_8 = secondary_inp[(9*`DWIDTH)-1:(8*`DWIDTH)];
    assign y_9 = secondary_inp[(10*`DWIDTH)-1:(9*`DWIDTH)];
    assign y_10 = secondary_inp[(11*`DWIDTH)-1:(10*`DWIDTH)];
    assign y_11 = secondary_inp[(12*`DWIDTH)-1:(11*`DWIDTH)];
    assign y_12 = secondary_inp[(13*`DWIDTH)-1:(12*`DWIDTH)];
    assign y_13 = secondary_inp[(14*`DWIDTH)-1:(13*`DWIDTH)];
    assign y_14 = secondary_inp[(15*`DWIDTH)-1:(14*`DWIDTH)];
    assign y_15 = secondary_inp[(16*`DWIDTH)-1:(15*`DWIDTH)];
    
     reg[`LOG_MUL_LATENCY-1:0] state;
        always @(posedge clk) begin
        if((enable_mul==1'b1) && (in_data_available==1'b1)) begin   
        
            if(state!=`MUL_LATENCY) begin 
                state<=state+1'b1;
            end
            else begin
                output_available_mul<=1;
                state<=0;
            end
        end
        else begin
          output_available_mul<=0;
          state<=0;
        end
    end

endmodule

