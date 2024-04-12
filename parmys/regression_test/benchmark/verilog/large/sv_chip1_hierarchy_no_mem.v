// created by Ahmad darabiha
// last updated Aug. 2002
// this is the design for chip #1 of stereo 
// vision system. this chip mainly performs the 
// normalization and correlation in two orienations
// and 3 scales.
module sv_chip1_hierarchy_no_mem (tm3_clk_v0, tm3_sram_data_out, tm3_sram_addr, tm3_sram_we, tm3_sram_oe, tm3_sram_adsp, bus_word_3_2to1, bus_word_4_2to1, bus_word_5_2to1, bus_word_6_2to1, counter_out_2to1, bus_word_1_1to0, bus_word_2_1to0, bus_word_3_1to0, bus_word_4_1to0, bus_word_5_1to0, bus_word_6_1to0, counter_out_1to0);

   input tm3_clk_v0; 
   output[63:0] tm3_sram_data_out; 
   wire[63:0] tm3_sram_data_out;
   reg[63:0] tm3_sram_data_xhdl0;
   output[18:0] tm3_sram_addr; 
   reg[18:0] tm3_sram_addr;
   output[7:0] tm3_sram_we; 
   reg[7:0] tm3_sram_we;
   output[1:0] tm3_sram_oe; 
   reg[1:0] tm3_sram_oe;
   output tm3_sram_adsp; 
   reg tm3_sram_adsp;
   input[15:0] bus_word_3_2to1; 
   input[15:0] bus_word_4_2to1; 
   input[15:0] bus_word_5_2to1; 
   input[15:0] bus_word_6_2to1; 
   input[2:0] counter_out_2to1; 
   output[7:0] bus_word_1_1to0; 
   wire[7:0] bus_word_1_1to0;
   output[7:0] bus_word_2_1to0; 
   wire[7:0] bus_word_2_1to0;
   output[7:0] bus_word_3_1to0; 
   wire[7:0] bus_word_3_1to0;
   output[7:0] bus_word_4_1to0; 
   wire[7:0] bus_word_4_1to0;
   output[7:0] bus_word_5_1to0; 
   wire[7:0] bus_word_5_1to0;
   output[7:0] bus_word_6_1to0; 
   wire[7:0] bus_word_6_1to0;
   output[2:0] counter_out_1to0; 
   wire[2:0] counter_out_1to0;

   reg[9:0] horiz; 
   reg[9:0] vert; 
   reg[63:0] vidin_data_buf_sc_1; 
   reg[55:0] vidin_data_buf_2_sc_1; 
   reg[18:0] vidin_addr_buf_sc_1; 
   reg[63:0] vidin_data_buf_sc_2; 
   reg[55:0] vidin_data_buf_2_sc_2; 
   reg[18:0] vidin_addr_buf_sc_2; 
   reg[63:0] vidin_data_buf_sc_4; 
   reg[55:0] vidin_data_buf_2_sc_4; 
   reg[18:0] vidin_addr_buf_sc_4; 
   reg video_state; 

   reg vidin_new_data_scld_1_2to3_left_reg; 
   reg[7:0] vidin_data_reg_scld_1_2to3_left_reg; 
   reg vidin_new_data_scld_2_2to3_left_reg; 
   reg[7:0] vidin_data_reg_scld_2_2to3_left_reg; 
   reg vidin_new_data_scld_4_2to3_left_reg; 
   reg[7:0] vidin_data_reg_scld_4_2to3_left_reg; 
   reg vidin_new_data_scld_1_2to3_right_reg; 
   reg[7:0] vidin_data_reg_scld_1_2to3_right_reg; 
   reg vidin_new_data_scld_2_2to3_right_reg; 
   reg[7:0] vidin_data_reg_scld_2_2to3_right_reg; 
   reg vidin_new_data_scld_4_2to3_right_reg; 
   reg[7:0] vidin_data_reg_scld_4_2to3_right_reg; 
   reg[18:0] vidin_addr_reg_2to3_reg; 
   wire vidin_new_data_scld_1_2to3_left; 
   wire[15:0] vidin_data_reg_scld_1_2to3_left_rz; 
   wire[15:0] vidin_data_reg_scld_1_2to3_left_iz; 
   wire[15:0] vidin_data_reg_scld_1_2to3_left_rp; 
   wire[15:0] vidin_data_reg_scld_1_2to3_left_ip; 
   wire[15:0] vidin_data_reg_scld_1_2to3_left_rn; 
   wire[15:0] vidin_data_reg_scld_1_2to3_left_in; 
   wire vidin_new_data_scld_2_2to3_left; 
   wire[15:0] vidin_data_reg_scld_2_2to3_left_rz; 
   wire[15:0] vidin_data_reg_scld_2_2to3_left_iz; 
   wire[15:0] vidin_data_reg_scld_2_2to3_left_rp; 
   wire[15:0] vidin_data_reg_scld_2_2to3_left_ip; 
   wire[15:0] vidin_data_reg_scld_2_2to3_left_rn; 
   wire[15:0] vidin_data_reg_scld_2_2to3_left_in; 
   wire vidin_new_data_scld_4_2to3_left; 
   wire[15:0] vidin_data_reg_scld_4_2to3_left_rz; 
   wire[15:0] vidin_data_reg_scld_4_2to3_left_iz; 
   wire[15:0] vidin_data_reg_scld_4_2to3_left_rp; 
   wire[15:0] vidin_data_reg_scld_4_2to3_left_ip; 
   wire[15:0] vidin_data_reg_scld_4_2to3_left_rn; 
   wire[15:0] vidin_data_reg_scld_4_2to3_left_in; 
   wire vidin_new_data_scld_1_2to3_right; 
   wire[15:0] vidin_data_reg_scld_1_2to3_right_rz; 
   wire[15:0] vidin_data_reg_scld_1_2to3_right_iz; 
   wire[15:0] vidin_data_reg_scld_1_2to3_right_rp; 
   wire[15:0] vidin_data_reg_scld_1_2to3_right_ip; 
   wire[15:0] vidin_data_reg_scld_1_2to3_right_rn; 
   wire[15:0] vidin_data_reg_scld_1_2to3_right_in; 
   wire vidin_new_data_scld_2_2to3_right; 
   wire[15:0] vidin_data_reg_scld_2_2to3_right_rz; 
   wire[15:0] vidin_data_reg_scld_2_2to3_right_iz; 
   wire[15:0] vidin_data_reg_scld_2_2to3_right_rp; 
   wire[15:0] vidin_data_reg_scld_2_2to3_right_ip; 
   wire[15:0] vidin_data_reg_scld_2_2to3_right_rn; 
   wire[15:0] vidin_data_reg_scld_2_2to3_right_in; 
   wire vidin_new_data_scld_4_2to3_right; 
   wire[15:0] vidin_data_reg_scld_4_2to3_right_rz; 
   wire[15:0] vidin_data_reg_scld_4_2to3_right_iz; 
   wire[15:0] vidin_data_reg_scld_4_2to3_right_rp; 
   wire[15:0] vidin_data_reg_scld_4_2to3_right_ip; 
   wire[15:0] vidin_data_reg_scld_4_2to3_right_rn; 
   wire[15:0] vidin_data_reg_scld_4_2to3_right_in; 
   wire[18:0] vidin_addr_reg_2to3; 
   wire svid_comp_switch_2to3; 
   wire[15:0] corr_out_1_p0; 
   wire[15:0] corr_out_1_p1; 
   wire[15:0] corr_out_1_p2; 
   wire[15:0] corr_out_1_p3; 
   wire[15:0] corr_out_1_p4; 
   wire[15:0] corr_out_1_p5; 
   wire[15:0] corr_out_1_p6; 
   wire[15:0] corr_out_1_p7; 
   wire[15:0] corr_out_1_p8; 
   wire[15:0] corr_out_1_p9; 
   wire[15:0] corr_out_1_p10; 
   wire[15:0] corr_out_1_p11; 
   wire[15:0] corr_out_1_p12; 
   wire[15:0] corr_out_1_p13; 
   wire[15:0] corr_out_1_p14; 
   wire[15:0] corr_out_1_p15; 
   wire[15:0] corr_out_1_p16; 
   wire[15:0] corr_out_1_p17; 
   wire[15:0] corr_out_1_p18; 
   wire[15:0] corr_out_1_p19; 
   wire[15:0] corr_out_1_p20; 
   wire[15:0] corr_out_1_n0; 
   wire[15:0] corr_out_1_n1; 
   wire[15:0] corr_out_1_n2; 
   wire[15:0] corr_out_1_n3; 
   wire[15:0] corr_out_1_n4; 
   wire[15:0] corr_out_1_n5; 
   wire[15:0] corr_out_1_n6; 
   wire[15:0] corr_out_1_n7; 
   wire[15:0] corr_out_1_n8; 
   wire[15:0] corr_out_1_n9; 
   wire[15:0] corr_out_1_n10; 
   wire[15:0] corr_out_1_n11; 
   wire[15:0] corr_out_1_n12; 
   wire[15:0] corr_out_1_n13; 
   wire[15:0] corr_out_1_n14; 
   wire[15:0] corr_out_1_n15; 
   wire[15:0] corr_out_1_n16; 
   wire[15:0] corr_out_1_n17; 
   wire[15:0] corr_out_1_n18; 
   wire[15:0] corr_out_1_n19; 
   wire[15:0] corr_out_1_n20; 
   reg[17:0] corr_out_10; 
   reg[17:0] corr_out_11; 
   reg[17:0] corr_out_12; 
   reg[17:0] corr_out_13; 
   reg[17:0] corr_out_14; 
   reg[17:0] corr_out_15; 
   reg[17:0] corr_out_16; 
   reg[17:0] corr_out_17; 
   reg[17:0] corr_out_18; 
   reg[17:0] corr_out_19; 
   reg[17:0] corr_out_110; 
   reg[17:0] corr_out_111; 
   reg[17:0] corr_out_112; 
   reg[17:0] corr_out_113; 
   reg[17:0] corr_out_114; 
   reg[17:0] corr_out_115; 
   reg[17:0] corr_out_116; 
   reg[17:0] corr_out_117; 
   reg[17:0] corr_out_118; 
   reg[17:0] corr_out_119; 
   reg[17:0] corr_out_120; 
   wire[15:0] corr_out_2_p0; 
   wire[15:0] corr_out_2_p1; 
   wire[15:0] corr_out_2_p2; 
   wire[15:0] corr_out_2_p3; 
   wire[15:0] corr_out_2_p4; 
   wire[15:0] corr_out_2_p5; 
   wire[15:0] corr_out_2_p6; 
   wire[15:0] corr_out_2_p7; 
   wire[15:0] corr_out_2_p8; 
   wire[15:0] corr_out_2_p9; 
   wire[15:0] corr_out_2_p10; 
   wire[15:0] corr_out_2_n0; 
   wire[15:0] corr_out_2_n1; 
   wire[15:0] corr_out_2_n2; 
   wire[15:0] corr_out_2_n3; 
   wire[15:0] corr_out_2_n4; 
   wire[15:0] corr_out_2_n5; 
   wire[15:0] corr_out_2_n6; 
   wire[15:0] corr_out_2_n7; 
   wire[15:0] corr_out_2_n8; 
   wire[15:0] corr_out_2_n9; 
   wire[15:0] corr_out_2_n10; 
   reg[17:0] corr_out_20; 
   reg[17:0] corr_out_21; 
   reg[17:0] corr_out_22; 
   reg[17:0] corr_out_23; 
   reg[17:0] corr_out_24; 
   reg[17:0] corr_out_25; 
   reg[17:0] corr_out_26; 
   reg[17:0] corr_out_27; 
   reg[17:0] corr_out_28; 
   reg[17:0] corr_out_29; 
   reg[17:0] corr_out_210; 
   wire[15:0] corr_out_4_p0; 
   wire[15:0] corr_out_4_p1; 
   wire[15:0] corr_out_4_p2; 
   wire[15:0] corr_out_4_p3; 
   wire[15:0] corr_out_4_p4; 
   wire[15:0] corr_out_4_p5; 
   wire[15:0] corr_out_4_n0; 
   wire[15:0] corr_out_4_n1; 
   wire[15:0] corr_out_4_n2; 
   wire[15:0] corr_out_4_n3; 
   wire[15:0] corr_out_4_n4; 
   wire[15:0] corr_out_4_n5; 
   reg[17:0] corr_out_40; 
   reg[17:0] corr_out_41; 
   reg[17:0] corr_out_42; 
   reg[17:0] corr_out_43; 
   reg[17:0] corr_out_44; 
   reg[17:0] corr_out_45; 

   assign tm3_sram_data_out = tm3_sram_data_xhdl0;

   port_bus_2to1_1 port_bus_2to1_1_inst (tm3_clk_v0, vidin_addr_reg_2to3, svid_comp_switch_2to3, vidin_new_data_scld_1_2to3_left, vidin_data_reg_scld_1_2to3_left_rp, vidin_data_reg_scld_1_2to3_left_ip, vidin_data_reg_scld_1_2to3_left_rn, vidin_data_reg_scld_1_2to3_left_in, vidin_new_data_scld_2_2to3_left, vidin_data_reg_scld_2_2to3_left_rp, vidin_data_reg_scld_2_2to3_left_ip, vidin_data_reg_scld_2_2to3_left_rn, vidin_data_reg_scld_2_2to3_left_in, vidin_new_data_scld_4_2to3_left, vidin_data_reg_scld_4_2to3_left_rp, vidin_data_reg_scld_4_2to3_left_ip, vidin_data_reg_scld_4_2to3_left_rn, vidin_data_reg_scld_4_2to3_left_in, vidin_new_data_scld_1_2to3_right, vidin_data_reg_scld_1_2to3_right_rp, vidin_data_reg_scld_1_2to3_right_ip, vidin_data_reg_scld_1_2to3_right_rn, vidin_data_reg_scld_1_2to3_right_in, vidin_new_data_scld_2_2to3_right, vidin_data_reg_scld_2_2to3_right_rp, vidin_data_reg_scld_2_2to3_right_ip, vidin_data_reg_scld_2_2to3_right_rn, vidin_data_reg_scld_2_2to3_right_in, vidin_new_data_scld_4_2to3_right, vidin_data_reg_scld_4_2to3_right_rp, vidin_data_reg_scld_4_2to3_right_ip, vidin_data_reg_scld_4_2to3_right_rn, vidin_data_reg_scld_4_2to3_right_in, bus_word_3_2to1, bus_word_4_2to1, bus_word_5_2to1, bus_word_6_2to1, counter_out_2to1); 

   wrapper_norm_corr_20  wrapper_norm_corr_20_inst_p(tm3_clk_v0, vidin_new_data_scld_1_2to3_left, vidin_data_reg_scld_1_2to3_right_rp, vidin_data_reg_scld_1_2to3_right_ip, vidin_data_reg_scld_1_2to3_left_rp, vidin_data_reg_scld_1_2to3_left_ip, corr_out_1_p0, corr_out_1_p1, corr_out_1_p2, corr_out_1_p3, corr_out_1_p4, corr_out_1_p5, corr_out_1_p6, corr_out_1_p7, corr_out_1_p8, corr_out_1_p9, corr_out_1_p10, corr_out_1_p11, corr_out_1_p12, corr_out_1_p13, corr_out_1_p14, corr_out_1_p15, corr_out_1_p16, corr_out_1_p17, corr_out_1_p18, corr_out_1_p19, corr_out_1_p20); 
   wrapper_norm_corr_20  wrapper_norm_corr_20_inst_n(tm3_clk_v0, vidin_new_data_scld_1_2to3_left, vidin_data_reg_scld_1_2to3_right_rn, vidin_data_reg_scld_1_2to3_right_in, vidin_data_reg_scld_1_2to3_left_rn, vidin_data_reg_scld_1_2to3_left_in, corr_out_1_n0, corr_out_1_n1, corr_out_1_n2, corr_out_1_n3, corr_out_1_n4, corr_out_1_n5, corr_out_1_n6, corr_out_1_n7, corr_out_1_n8, corr_out_1_n9, corr_out_1_n10, corr_out_1_n11, corr_out_1_n12, corr_out_1_n13, corr_out_1_n14, corr_out_1_n15, corr_out_1_n16, corr_out_1_n17, corr_out_1_n18, corr_out_1_n19, corr_out_1_n20);
   wrapper_norm_corr_10  wrapper_norm_corr_10_inst_p(tm3_clk_v0, vidin_new_data_scld_2_2to3_left, vidin_data_reg_scld_2_2to3_right_rp, vidin_data_reg_scld_2_2to3_right_ip, vidin_data_reg_scld_2_2to3_left_rp, vidin_data_reg_scld_2_2to3_left_ip, corr_out_2_p0, corr_out_2_p1, corr_out_2_p2, corr_out_2_p3, corr_out_2_p4, corr_out_2_p5, corr_out_2_p6, corr_out_2_p7, corr_out_2_p8, corr_out_2_p9, corr_out_2_p10);
   wrapper_norm_corr_10  wrapper_norm_corr_10_inst_n(tm3_clk_v0, vidin_new_data_scld_2_2to3_left, vidin_data_reg_scld_2_2to3_right_rn, vidin_data_reg_scld_2_2to3_right_in, vidin_data_reg_scld_2_2to3_left_rn, vidin_data_reg_scld_2_2to3_left_in, corr_out_2_n0, corr_out_2_n1, corr_out_2_n2, corr_out_2_n3, corr_out_2_n4, corr_out_2_n5, corr_out_2_n6, corr_out_2_n7, corr_out_2_n8, corr_out_2_n9, corr_out_2_n10);
   wrapper_norm_corr_5_seq  wrapper_norm_corr_5_inst_p(tm3_clk_v0, vidin_new_data_scld_4_2to3_left, vidin_data_reg_scld_4_2to3_right_rp, vidin_data_reg_scld_4_2to3_right_ip, vidin_data_reg_scld_4_2to3_left_rp, vidin_data_reg_scld_4_2to3_left_ip, corr_out_4_p0, corr_out_4_p1, corr_out_4_p2, corr_out_4_p3, corr_out_4_p4, corr_out_4_p5);
   wrapper_norm_corr_5_seq  wrapper_norm_corr_5_inst_n(tm3_clk_v0, vidin_new_data_scld_4_2to3_left, vidin_data_reg_scld_4_2to3_right_rn, vidin_data_reg_scld_4_2to3_right_in, vidin_data_reg_scld_4_2to3_left_rn, vidin_data_reg_scld_4_2to3_left_in, corr_out_4_n0, corr_out_4_n1, corr_out_4_n2, corr_out_4_n3, corr_out_4_n4, corr_out_4_n5);

   port_bus_1to0  port_bus_1to0_inst(tm3_clk_v0, vidin_addr_reg_2to3, svid_comp_switch_2to3, vidin_new_data_scld_1_2to3_left, {corr_out_40[17], corr_out_40[15:9]}, {corr_out_41[17], corr_out_41[15:9]}, {corr_out_42[17], corr_out_42[15:9]}, {corr_out_43[17], corr_out_43[15:9]}, {corr_out_44[17], corr_out_44[15:9]}, {corr_out_45[17], corr_out_45[15:9]}, {corr_out_20[17], corr_out_20[15:9]}, {corr_out_21[17], corr_out_21[15:9]}, {corr_out_22[17], corr_out_22[15:9]}, {corr_out_23[17], corr_out_23[15:9]}, {corr_out_24[17], corr_out_24[15:9]}, {corr_out_25[17], corr_out_25[15:9]}, {corr_out_26[17], corr_out_26[15:9]}, {corr_out_27[17], corr_out_27[15:9]}, {corr_out_28[17], corr_out_28[15:9]}, {corr_out_29[17], corr_out_29[15:9]}, {corr_out_210[17], corr_out_210[15:9]}, {corr_out_10[17], corr_out_10[15:9]}, {corr_out_11[17], corr_out_11[15:9]}, {corr_out_12[17], corr_out_12[15:9]}, {corr_out_13[17], corr_out_13[15:9]}, {corr_out_14[17], corr_out_14[15:9]}, {corr_out_15[17], corr_out_15[15:9]}, {corr_out_16[17], corr_out_16[15:9]}, {corr_out_17[17], corr_out_17[15:9]}, {corr_out_18[17], corr_out_18[15:9]}, {corr_out_19[17], corr_out_19[15:9]}, {corr_out_110[17], corr_out_110[15:9]}, {corr_out_111[17], corr_out_111[15:9]}, {corr_out_112[17], corr_out_112[15:9]}, {corr_out_113[17], corr_out_113[15:9]}, {corr_out_114[17], corr_out_114[15:9]}, {corr_out_115[17], corr_out_115[15:9]}, {corr_out_116[17], corr_out_116[15:9]}, {corr_out_117[17], corr_out_117[15:9]}, {corr_out_118[17], corr_out_118[15:9]}, {corr_out_119[17], corr_out_119[15:9]}, {corr_out_120[17], corr_out_120[15:9]}, bus_word_1_1to0, bus_word_2_1to0, bus_word_3_1to0, bus_word_4_1to0, bus_word_5_1to0, bus_word_6_1to0, counter_out_1to0);	

   always @(posedge tm3_clk_v0)
   begin
         if (vidin_new_data_scld_1_2to3_left == 1'b1)
         begin
                  corr_out_10 <= ({ corr_out_1_p0[15],  corr_out_1_p0[15], corr_out_1_p0}) + ({corr_out_1_n0[15], corr_out_1_n0[15], corr_out_1_n0}) ; 
                  corr_out_11 <= ({ corr_out_1_p1[15],  corr_out_1_p1[15], corr_out_1_p1}) + ({corr_out_1_n1[15], corr_out_1_n1[15], corr_out_1_n1}) ; 
                  corr_out_12 <= ({ corr_out_1_p2[15],  corr_out_1_p2[15], corr_out_1_p2}) + ({corr_out_1_n2[15], corr_out_1_n2[15], corr_out_1_n2}) ; 
                  corr_out_13 <= ({ corr_out_1_p3[15],  corr_out_1_p3[15], corr_out_1_p3}) + ({corr_out_1_n3[15], corr_out_1_n3[15], corr_out_1_n3}) ; 
                  corr_out_14 <= ({ corr_out_1_p4[15],  corr_out_1_p4[15], corr_out_1_p4}) + ({corr_out_1_n4[15], corr_out_1_n4[15], corr_out_1_n4}) ; 
                  corr_out_15 <= ({ corr_out_1_p5[15],  corr_out_1_p5[15], corr_out_1_p5}) + ({corr_out_1_n5[15], corr_out_1_n5[15], corr_out_1_n5}) ; 
                  corr_out_16 <= ({ corr_out_1_p6[15],  corr_out_1_p6[15], corr_out_1_p6}) + ({corr_out_1_n6[15], corr_out_1_n6[15], corr_out_1_n6}) ; 
                  corr_out_17 <= ({ corr_out_1_p7[15],  corr_out_1_p7[15], corr_out_1_p7}) + ({corr_out_1_n7[15], corr_out_1_n7[15], corr_out_1_n7}) ; 
                  corr_out_18 <= ({ corr_out_1_p8[15],  corr_out_1_p8[15], corr_out_1_p8}) + ({corr_out_1_n8[15], corr_out_1_n8[15], corr_out_1_n8}) ; 
                  corr_out_19 <= ({ corr_out_1_p9[15],  corr_out_1_p9[15], corr_out_1_p9}) + ({corr_out_1_n9[15], corr_out_1_n9[15], corr_out_1_n9}) ; 
                  corr_out_110 <= ({ corr_out_1_p10[15],  corr_out_1_p10[15], corr_out_1_p10}) + ({corr_out_1_n10[15], corr_out_1_n10[15], corr_out_1_n10}) ; 
                  corr_out_111 <= ({ corr_out_1_p11[15],  corr_out_1_p11[15], corr_out_1_p11}) + ({corr_out_1_n11[15], corr_out_1_n11[15], corr_out_1_n11}) ; 
                  corr_out_112 <= ({ corr_out_1_p12[15],  corr_out_1_p12[15], corr_out_1_p12}) + ({corr_out_1_n12[15], corr_out_1_n12[15], corr_out_1_n12}) ; 
                  corr_out_113 <= ({ corr_out_1_p13[15],  corr_out_1_p13[15], corr_out_1_p13}) + ({corr_out_1_n13[15], corr_out_1_n13[15], corr_out_1_n13}) ; 
                  corr_out_114 <= ({ corr_out_1_p14[15],  corr_out_1_p14[15], corr_out_1_p14}) + ({corr_out_1_n14[15], corr_out_1_n14[15], corr_out_1_n14}) ; 
                  corr_out_115 <= ({ corr_out_1_p15[15],  corr_out_1_p15[15], corr_out_1_p15}) + ({corr_out_1_n15[15], corr_out_1_n15[15], corr_out_1_n15}) ; 
                  corr_out_116 <= ({ corr_out_1_p16[15],  corr_out_1_p16[15], corr_out_1_p16}) + ({corr_out_1_n16[15], corr_out_1_n16[15], corr_out_1_n16}) ; 
                  corr_out_117 <= ({ corr_out_1_p17[15],  corr_out_1_p17[15], corr_out_1_p17}) + ({corr_out_1_n17[15], corr_out_1_n17[15], corr_out_1_n17}) ; 
                  corr_out_118 <= ({ corr_out_1_p18[15],  corr_out_1_p18[15], corr_out_1_p18}) + ({corr_out_1_n18[15], corr_out_1_n18[15], corr_out_1_n18}) ; 
                  corr_out_119 <= ({ corr_out_1_p19[15],  corr_out_1_p19[15], corr_out_1_p19}) + ({corr_out_1_n19[15], corr_out_1_n19[15], corr_out_1_n19}) ; 
                  corr_out_120 <= ({ corr_out_1_p20[15],  corr_out_1_p20[15], corr_out_1_p20}) + ({corr_out_1_n20[15], corr_out_1_n20[15], corr_out_1_n20}) ; 
         end 
		else
         begin
                  corr_out_10 <= corr_out_10;
                  corr_out_11 <= corr_out_11;
                  corr_out_12 <= corr_out_12;
                  corr_out_13 <= corr_out_13;
                  corr_out_14 <= corr_out_14;
                  corr_out_15 <= corr_out_15;
                  corr_out_16 <= corr_out_16;
                  corr_out_17 <= corr_out_17;
                  corr_out_18 <= corr_out_18;
                  corr_out_19 <= corr_out_19;
                  corr_out_110 <= corr_out_110;
                  corr_out_111 <= corr_out_111;
                  corr_out_112 <= corr_out_112;
                  corr_out_113 <= corr_out_113;
                  corr_out_114 <= corr_out_114;
                  corr_out_115 <= corr_out_115;
                  corr_out_116 <= corr_out_116;
                  corr_out_117 <= corr_out_117;
                  corr_out_118 <= corr_out_118;
                  corr_out_119 <= corr_out_119;
                  corr_out_120 <= corr_out_120;
         end 
   end 

   always @(posedge tm3_clk_v0)
   begin
         if (vidin_new_data_scld_2_2to3_left == 1'b1)
         begin
                  corr_out_20 <= ({ corr_out_2_p0[15],  corr_out_2_p0[15], corr_out_2_p0}) + ({corr_out_2_n0[15], corr_out_2_n0[15], corr_out_2_n0}) ; 
                  corr_out_21 <= ({ corr_out_2_p1[15],  corr_out_2_p1[15], corr_out_2_p1}) + ({corr_out_2_n1[15], corr_out_2_n1[15], corr_out_2_n1}) ; 
                  corr_out_22 <= ({ corr_out_2_p2[15],  corr_out_2_p2[15], corr_out_2_p2}) + ({corr_out_2_n2[15], corr_out_2_n2[15], corr_out_2_n2}) ; 
                  corr_out_23 <= ({ corr_out_2_p3[15],  corr_out_2_p3[15], corr_out_2_p3}) + ({corr_out_2_n3[15], corr_out_2_n3[15], corr_out_2_n3}) ; 
                  corr_out_24 <= ({ corr_out_2_p4[15],  corr_out_2_p4[15], corr_out_2_p4}) + ({corr_out_2_n4[15], corr_out_2_n4[15], corr_out_2_n4}) ; 
                  corr_out_25 <= ({ corr_out_2_p5[15],  corr_out_2_p5[15], corr_out_2_p5}) + ({corr_out_2_n5[15], corr_out_2_n5[15], corr_out_2_n5}) ; 
                  corr_out_26 <= ({ corr_out_2_p6[15],  corr_out_2_p6[15], corr_out_2_p6}) + ({corr_out_2_n6[15], corr_out_2_n6[15], corr_out_2_n6}) ; 
                  corr_out_27 <= ({ corr_out_2_p7[15],  corr_out_2_p7[15], corr_out_2_p7}) + ({corr_out_2_n7[15], corr_out_2_n7[15], corr_out_2_n7}) ; 
                  corr_out_28 <= ({ corr_out_2_p8[15],  corr_out_2_p8[15], corr_out_2_p8}) + ({corr_out_2_n8[15], corr_out_2_n8[15], corr_out_2_n8}) ; 
                  corr_out_29 <= ({ corr_out_2_p9[15],  corr_out_2_p9[15], corr_out_2_p9}) + ({corr_out_2_n9[15], corr_out_2_n9[15], corr_out_2_n9}) ; 
                  corr_out_210 <= ({ corr_out_2_p10[15],  corr_out_2_p10[15], corr_out_2_p10}) + ({corr_out_2_n10[15], corr_out_2_n10[15], corr_out_2_n10}) ; 
         end 
         else
         begin
                  corr_out_20 <= corr_out_20;
                  corr_out_21 <= corr_out_21;
                  corr_out_22 <= corr_out_22;
                  corr_out_23 <= corr_out_23;
                  corr_out_24 <= corr_out_24;
                  corr_out_25 <= corr_out_25;
                  corr_out_26 <= corr_out_26;
                  corr_out_27 <= corr_out_27;
                  corr_out_28 <= corr_out_28;
                  corr_out_29 <= corr_out_29;
                  corr_out_210 <= corr_out_210;
         end 
   end 

   always @(posedge tm3_clk_v0)
   begin
         if (vidin_new_data_scld_2_2to3_left == 1'b1)
         begin
                  corr_out_40 <= ({ corr_out_4_p0[15],  corr_out_4_p0[15], corr_out_4_p0}) + ({corr_out_4_n0[15], corr_out_4_n0[15], corr_out_4_n0}) ; 
                  corr_out_41 <= ({ corr_out_4_p1[15],  corr_out_4_p1[15], corr_out_4_p1}) + ({corr_out_4_n1[15], corr_out_4_n1[15], corr_out_4_n1}) ; 
                  corr_out_42 <= ({ corr_out_4_p2[15],  corr_out_4_p2[15], corr_out_4_p2}) + ({corr_out_4_n2[15], corr_out_4_n2[15], corr_out_4_n2}) ; 
                  corr_out_43 <= ({ corr_out_4_p3[15],  corr_out_4_p3[15], corr_out_4_p3}) + ({corr_out_4_n3[15], corr_out_4_n3[15], corr_out_4_n3}) ; 
                  corr_out_44 <= ({ corr_out_4_p4[15],  corr_out_4_p4[15], corr_out_4_p4}) + ({corr_out_4_n4[15], corr_out_4_n4[15], corr_out_4_n4}) ; 
                  corr_out_45 <= ({ corr_out_4_p5[15],  corr_out_4_p5[15], corr_out_4_p5}) + ({corr_out_4_n5[15], corr_out_4_n5[15], corr_out_4_n5}) ; 
         end 
         else
         begin
                  corr_out_40 <= corr_out_40;
                  corr_out_41 <= corr_out_41;
                  corr_out_42 <= corr_out_42;
                  corr_out_43 <= corr_out_43;
                  corr_out_44 <= corr_out_44;
                  corr_out_45 <= corr_out_45;
         end 
   end 

   always @(posedge tm3_clk_v0)
   begin
         vidin_new_data_scld_1_2to3_left_reg <= vidin_new_data_scld_1_2to3_left ; 
         vidin_data_reg_scld_1_2to3_left_reg <= vidin_data_reg_scld_1_2to3_left_rp[15:8] ; 
         vidin_new_data_scld_2_2to3_left_reg <= vidin_new_data_scld_2_2to3_left ; 
         vidin_data_reg_scld_2_2to3_left_reg <= vidin_data_reg_scld_2_2to3_left_rp[15:8] ; 
         vidin_new_data_scld_4_2to3_left_reg <= vidin_new_data_scld_4_2to3_left ; 
         vidin_data_reg_scld_4_2to3_left_reg <= vidin_data_reg_scld_4_2to3_left_rp[15:8] ; 
         vidin_new_data_scld_1_2to3_right_reg <= vidin_new_data_scld_1_2to3_right ; 
         vidin_data_reg_scld_1_2to3_right_reg <= vidin_data_reg_scld_1_2to3_right_rp[15:8] ; 
         vidin_new_data_scld_2_2to3_right_reg <= vidin_new_data_scld_2_2to3_right ; 
         vidin_data_reg_scld_2_2to3_right_reg <= vidin_data_reg_scld_2_2to3_right_rp[15:8] ; 
         vidin_new_data_scld_4_2to3_right_reg <= vidin_new_data_scld_4_2to3_right ; 
         vidin_data_reg_scld_4_2to3_right_reg <= vidin_data_reg_scld_4_2to3_right_rp[15:8] ; 
         vidin_addr_reg_2to3_reg <= vidin_addr_reg_2to3 ; 
   end 

   always @(posedge tm3_clk_v0)
   begin
         video_state <= ~(video_state) ; 
         if (video_state == 1'b0)
         begin
            if (horiz == 800)
            begin
               horiz <= 10'b0000000000 ; 
               if (vert == 525)
               begin
                  vert <= 10'b0000000000 ; 
               end
               else
               begin
                  vert <= vert + 1 ; 
               end 
            end
            else
            begin
               horiz <= horiz + 1 ; 
            end 
            tm3_sram_adsp <= 1'b1 ; 
            tm3_sram_we <= 8'b11111111 ; 
            tm3_sram_data_xhdl0 <= 64'b0000000000000000000000000000000000000000000000000000000000000000; 
            tm3_sram_oe <= 2'b11 ; 
         end
         else
         begin
            tm3_sram_adsp <= 1'b0 ; 
            case (horiz[2:0])
               3'b000 :
                        begin
                           tm3_sram_addr <= vidin_addr_buf_sc_2 ; 
                           tm3_sram_we <= 8'b00000000 ; 
                           tm3_sram_oe <= 2'b11 ; 
                           tm3_sram_data_xhdl0 <= vidin_data_buf_sc_2 ; 
                        end
               3'b100 :
                        begin
                           tm3_sram_addr <= vidin_addr_buf_sc_4 ; 
                           tm3_sram_we <= 8'b00000000 ; 
                           tm3_sram_oe <= 2'b11 ; 
                           tm3_sram_data_xhdl0 <= vidin_data_buf_sc_4 ; 
                        end
               3'b111 :
                        begin
                           tm3_sram_addr <= vidin_addr_buf_sc_1 ; 
                           tm3_sram_we <= 8'b00000000 ; 
                           tm3_sram_oe <= 2'b11 ; 
                           tm3_sram_data_xhdl0 <= vidin_data_buf_sc_1 ; 
                        end
               default :
                        begin
                           tm3_sram_addr <= 19'b0000000000000000000 ; 
                           tm3_sram_we <= 8'b11111111 ; 
                           tm3_sram_oe <= 2'b11 ; 
                           tm3_sram_data_xhdl0 <= 64'b0000000000000000000000000000000000000000000000000000000000000000; 
                        end
            endcase 
         end 
         if (vidin_new_data_scld_1_2to3_left_reg == 1'b1)
         begin
            case ({svid_comp_switch_2to3, vidin_addr_reg_2to3_reg[2:0]})
               4'b0000 :
                        begin
                           vidin_data_buf_2_sc_1[7:0] <= vidin_data_reg_scld_1_2to3_left_reg ; 
                        end
               4'b0001 :
                        begin
                           vidin_data_buf_2_sc_1[15:8] <= vidin_data_reg_scld_1_2to3_left_reg ; 
                        end
               4'b0010 :
                        begin
                           vidin_data_buf_2_sc_1[23:16] <= vidin_data_reg_scld_1_2to3_left_reg ; 
                        end
               4'b0011 :
                        begin
                           vidin_data_buf_2_sc_1[31:24] <= vidin_data_reg_scld_1_2to3_left_reg ; 
                        end
               4'b0100 :
                        begin
                           vidin_data_buf_2_sc_1[39:32] <= vidin_data_reg_scld_1_2to3_left_reg ; 
                        end

               4'b0101 :
                        begin
                           vidin_data_buf_2_sc_1[47:40] <= vidin_data_reg_scld_1_2to3_left_reg ; 
                        end
               4'b0110 :
                        begin
                           vidin_data_buf_2_sc_1[55:48] <= vidin_data_reg_scld_1_2to3_left_reg ; 
                        end
               4'b0111 :
                        begin
                           vidin_data_buf_sc_1 <= {vidin_data_reg_scld_1_2to3_left_reg, vidin_data_buf_2_sc_1[55:0]} ; 
                           vidin_addr_buf_sc_1 <= {4'b0000, svid_comp_switch_2to3, vidin_addr_reg_2to3_reg[16:3]} ; 
                        end
               4'b1000 :
                        begin
                           vidin_data_buf_2_sc_1[7:0] <= vidin_data_reg_scld_1_2to3_right_reg ; 
                        end
               4'b1001 :
                        begin
                           vidin_data_buf_2_sc_1[15:8] <= vidin_data_reg_scld_1_2to3_right_reg ; 
                        end
               4'b1010 :
                        begin
                           vidin_data_buf_2_sc_1[23:16] <= vidin_data_reg_scld_1_2to3_right_reg ; 
                        end
               4'b1011 :
                        begin
                           vidin_data_buf_2_sc_1[31:24] <= vidin_data_reg_scld_1_2to3_right_reg ; 
                        end
               4'b1100 :
                        begin
                           vidin_data_buf_2_sc_1[39:32] <= vidin_data_reg_scld_1_2to3_right_reg ; 
                        end
               4'b1101 :
                        begin
                           vidin_data_buf_2_sc_1[47:40] <= vidin_data_reg_scld_1_2to3_right_reg ; 
                        end
               4'b1110 :
                        begin
                           vidin_data_buf_2_sc_1[55:48] <= vidin_data_reg_scld_1_2to3_right_reg ; 
                        end
               4'b1111 :
                        begin
                           vidin_data_buf_sc_1 <= {vidin_data_reg_scld_1_2to3_right_reg, vidin_data_buf_2_sc_1[55:0]} ; 
                           vidin_addr_buf_sc_1 <= {4'b0000, svid_comp_switch_2to3, vidin_addr_reg_2to3_reg[16:3]} ; 
                        end
            endcase 
         end 
         if (vidin_new_data_scld_2_2to3_left_reg == 1'b1)
         begin
            case ({svid_comp_switch_2to3, vidin_addr_reg_2to3_reg[3:1]})
               4'b0000 :
                        begin
                           vidin_data_buf_2_sc_2[7:0] <= vidin_data_reg_scld_2_2to3_left_reg ; 
                        end
               4'b0001 :
                        begin
                           vidin_data_buf_2_sc_2[15:8] <= vidin_data_reg_scld_2_2to3_left_reg ; 
                        end
               4'b0010 :
                        begin
                           vidin_data_buf_2_sc_2[23:16] <= vidin_data_reg_scld_2_2to3_left_reg ; 
                        end
               4'b0011 :
                        begin
                           vidin_data_buf_2_sc_2[31:24] <= vidin_data_reg_scld_2_2to3_left_reg ; 
                        end
               4'b0100 :

                        begin
                           vidin_data_buf_2_sc_2[39:32] <= vidin_data_reg_scld_2_2to3_left_reg ; 
                        end
               4'b0101 :
                        begin
                           vidin_data_buf_2_sc_2[47:40] <= vidin_data_reg_scld_2_2to3_left_reg ; 
                        end
               4'b0110 :
                        begin
                           vidin_data_buf_2_sc_2[55:48] <= vidin_data_reg_scld_2_2to3_left_reg ; 
                        end
               4'b0111 :
                        begin
                           vidin_data_buf_sc_2 <= {vidin_data_reg_scld_2_2to3_left_reg, vidin_data_buf_2_sc_2[55:0]} ; 
                           vidin_addr_buf_sc_2 <= {4'b0000, svid_comp_switch_2to3, 1'b0, vidin_addr_reg_2to3_reg[16:10], (6'b101101 + ({1'b0, vidin_addr_reg_2to3_reg[8:4]}))} ; 
                        end
               4'b1000 :
                        begin
                           vidin_data_buf_2_sc_2[7:0] <= vidin_data_reg_scld_2_2to3_right_reg ; 
                        end
               4'b1001 :
                        begin
                           vidin_data_buf_2_sc_2[15:8] <= vidin_data_reg_scld_2_2to3_right_reg ; 
                        end
               4'b1010 :
                        begin
                           vidin_data_buf_2_sc_2[23:16] <= vidin_data_reg_scld_2_2to3_right_reg ; 
                        end
               4'b1011 :
                        begin
                           vidin_data_buf_2_sc_2[31:24] <= vidin_data_reg_scld_2_2to3_right_reg ; 
                        end
               4'b1100 :
                        begin
                           vidin_data_buf_2_sc_2[39:32] <= vidin_data_reg_scld_2_2to3_right_reg ; 
                        end
               4'b1101 :
                        begin
                           vidin_data_buf_2_sc_2[47:40] <= vidin_data_reg_scld_2_2to3_right_reg ; 
                        end
               4'b1110 :
                        begin
                           vidin_data_buf_2_sc_2[55:48] <= vidin_data_reg_scld_2_2to3_right_reg ; 
                        end
               4'b1111 :
                        begin
                           vidin_data_buf_sc_2 <= {vidin_data_reg_scld_2_2to3_right_reg, vidin_data_buf_2_sc_2[55:0]} ; 
                           vidin_addr_buf_sc_2 <= {4'b0000, svid_comp_switch_2to3, 1'b0, vidin_addr_reg_2to3_reg[16:10], (6'b101101 + ({1'b0, vidin_addr_reg_2to3_reg[8:4]}))} ; 
                        end
            endcase 
         end 
         if (vidin_new_data_scld_4_2to3_left_reg == 1'b1)
         begin
            case ({svid_comp_switch_2to3, vidin_addr_reg_2to3_reg[4:2]})
               4'b0000 :
                        begin
                           vidin_data_buf_2_sc_4[7:0] <= vidin_data_reg_scld_4_2to3_left_reg ; 
                        end
               4'b0001 :
                        begin
                           vidin_data_buf_2_sc_4[15:8] <= vidin_data_reg_scld_4_2to3_left_reg ; 
                        end
               4'b0010 :
                        begin
                           vidin_data_buf_2_sc_4[23:16] <= vidin_data_reg_scld_4_2to3_left_reg ; 
                        end
               4'b0011 :
                        begin
                          vidin_data_buf_2_sc_4[31:24] <= vidin_data_reg_scld_4_2to3_left_reg ; 
                        end
               4'b0100 :
                        begin
                           vidin_data_buf_2_sc_4[39:32] <= vidin_data_reg_scld_4_2to3_left_reg ; 
                        end
               4'b0101 :
                        begin
                           vidin_data_buf_2_sc_4[47:40] <= vidin_data_reg_scld_4_2to3_left_reg ; 
                        end
               4'b0110 :
                        begin
                           vidin_data_buf_2_sc_4[55:48] <= vidin_data_reg_scld_4_2to3_left_reg ; 
                        end
               4'b0111 :
                        begin
                           vidin_data_buf_sc_4 <= {vidin_data_reg_scld_4_2to3_left_reg, vidin_data_buf_2_sc_4[55:0]} ; 
                           vidin_addr_buf_sc_4 <= {4'b0000, svid_comp_switch_2to3, (8'b10000000 + ({2'b00, vidin_addr_reg_2to3_reg[16:11]})), (6'b101101 + ({2'b00, vidin_addr_reg_2to3_reg[8:5]}))} ; 
                        end
               4'b1000 :
                        begin
                           vidin_data_buf_2_sc_4[7:0] <= vidin_data_reg_scld_4_2to3_right_reg ; 
                        end
               4'b1001 :
                        begin
                           vidin_data_buf_2_sc_4[15:8] <= vidin_data_reg_scld_4_2to3_right_reg ; 
                        end
               4'b1010 :
                        begin
                           vidin_data_buf_2_sc_4[23:16] <= vidin_data_reg_scld_4_2to3_right_reg ; 
                        end
               4'b1011 :
                        begin
                           vidin_data_buf_2_sc_4[31:24] <= vidin_data_reg_scld_4_2to3_right_reg ; 
                        end
               4'b1100 :
                        begin
                           vidin_data_buf_2_sc_4[39:32] <= vidin_data_reg_scld_4_2to3_right_reg ; 
                        end
               4'b1101 :
                        begin
                           vidin_data_buf_2_sc_4[47:40] <= vidin_data_reg_scld_4_2to3_right_reg ; 
                        end
               4'b1110 :
                        begin
                           vidin_data_buf_2_sc_4[55:48] <= vidin_data_reg_scld_4_2to3_right_reg ; 
                        end
               4'b1111 :
                        begin
                           vidin_data_buf_sc_4 <= {vidin_data_reg_scld_4_2to3_right_reg, vidin_data_buf_2_sc_4[55:0]} ; 
                           vidin_addr_buf_sc_4 <= {4'b0000, svid_comp_switch_2to3, (8'b10000000 + ({2'b00, vidin_addr_reg_2to3_reg[16:11]})), (6'b101101 + ({2'b00, vidin_addr_reg_2to3_reg[8:5]}))} ; 
                        end
            endcase 
         end 
   end 
endmodule
module port_bus_2to1_1 (clk, vidin_addr_reg, svid_comp_switch, vidin_new_data_scld_1_2to3_left, vidin_data_reg_scld_1_2to3_left_rp, vidin_data_reg_scld_1_2to3_left_ip, vidin_data_reg_scld_1_2to3_left_rn, vidin_data_reg_scld_1_2to3_left_in, vidin_new_data_scld_2_2to3_left, vidin_data_reg_scld_2_2to3_left_rp, vidin_data_reg_scld_2_2to3_left_ip, vidin_data_reg_scld_2_2to3_left_rn, vidin_data_reg_scld_2_2to3_left_in, vidin_new_data_scld_4_2to3_left, vidin_data_reg_scld_4_2to3_left_rp, vidin_data_reg_scld_4_2to3_left_ip, vidin_data_reg_scld_4_2to3_left_rn, vidin_data_reg_scld_4_2to3_left_in, vidin_new_data_scld_1_2to3_right, vidin_data_reg_scld_1_2to3_right_rp, vidin_data_reg_scld_1_2to3_right_ip, vidin_data_reg_scld_1_2to3_right_rn, vidin_data_reg_scld_1_2to3_right_in, vidin_new_data_scld_2_2to3_right, vidin_data_reg_scld_2_2to3_right_rp, vidin_data_reg_scld_2_2to3_right_ip, vidin_data_reg_scld_2_2to3_right_rn, vidin_data_reg_scld_2_2to3_right_in, vidin_new_data_scld_4_2to3_right, vidin_data_reg_scld_4_2to3_right_rp, vidin_data_reg_scld_4_2to3_right_ip, vidin_data_reg_scld_4_2to3_right_rn, vidin_data_reg_scld_4_2to3_right_in, bus_word_3, bus_word_4, bus_word_5, bus_word_6, counter_out); 
   input clk; 
   output[18:0] vidin_addr_reg; 
   reg[18:0] vidin_addr_reg;
   output svid_comp_switch; 
   reg svid_comp_switch;
   output vidin_new_data_scld_1_2to3_left; 
   reg vidin_new_data_scld_1_2to3_left;
   output[15:0] vidin_data_reg_scld_1_2to3_left_rp; 
   reg[15:0] vidin_data_reg_scld_1_2to3_left_rp;
   output[15:0] vidin_data_reg_scld_1_2to3_left_ip; 
   reg[15:0] vidin_data_reg_scld_1_2to3_left_ip;
   output[15:0] vidin_data_reg_scld_1_2to3_left_rn; 
   reg[15:0] vidin_data_reg_scld_1_2to3_left_rn;
   output[15:0] vidin_data_reg_scld_1_2to3_left_in; 
   reg[15:0] vidin_data_reg_scld_1_2to3_left_in;
   output vidin_new_data_scld_2_2to3_left; 
   reg vidin_new_data_scld_2_2to3_left;
   output[15:0] vidin_data_reg_scld_2_2to3_left_rp; 
   reg[15:0] vidin_data_reg_scld_2_2to3_left_rp;
   output[15:0] vidin_data_reg_scld_2_2to3_left_ip; 
   reg[15:0] vidin_data_reg_scld_2_2to3_left_ip;
   output[15:0] vidin_data_reg_scld_2_2to3_left_rn; 
   reg[15:0] vidin_data_reg_scld_2_2to3_left_rn;
   output[15:0] vidin_data_reg_scld_2_2to3_left_in; 
   reg[15:0] vidin_data_reg_scld_2_2to3_left_in;
   output vidin_new_data_scld_4_2to3_left; 
   reg vidin_new_data_scld_4_2to3_left;
   output[15:0] vidin_data_reg_scld_4_2to3_left_rp; 
   reg[15:0] vidin_data_reg_scld_4_2to3_left_rp;
   output[15:0] vidin_data_reg_scld_4_2to3_left_ip; 
   reg[15:0] vidin_data_reg_scld_4_2to3_left_ip;
   output[15:0] vidin_data_reg_scld_4_2to3_left_rn; 
   reg[15:0] vidin_data_reg_scld_4_2to3_left_rn;
   output[15:0] vidin_data_reg_scld_4_2to3_left_in; 
   reg[15:0] vidin_data_reg_scld_4_2to3_left_in;
   output vidin_new_data_scld_1_2to3_right; 
   reg vidin_new_data_scld_1_2to3_right;
   output[15:0] vidin_data_reg_scld_1_2to3_right_rp; 
   reg[15:0] vidin_data_reg_scld_1_2to3_right_rp;
   output[15:0] vidin_data_reg_scld_1_2to3_right_ip; 
   reg[15:0] vidin_data_reg_scld_1_2to3_right_ip;
   output[15:0] vidin_data_reg_scld_1_2to3_right_rn; 
   reg[15:0] vidin_data_reg_scld_1_2to3_right_rn;
   output[15:0] vidin_data_reg_scld_1_2to3_right_in; 
   reg[15:0] vidin_data_reg_scld_1_2to3_right_in;
   output vidin_new_data_scld_2_2to3_right; 
   reg vidin_new_data_scld_2_2to3_right;
   output[15:0] vidin_data_reg_scld_2_2to3_right_rp; 
   reg[15:0] vidin_data_reg_scld_2_2to3_right_rp;
   output[15:0] vidin_data_reg_scld_2_2to3_right_ip; 
   reg[15:0] vidin_data_reg_scld_2_2to3_right_ip;
   output[15:0] vidin_data_reg_scld_2_2to3_right_rn; 
   reg[15:0] vidin_data_reg_scld_2_2to3_right_rn;
   output[15:0] vidin_data_reg_scld_2_2to3_right_in; 

   reg[15:0] vidin_data_reg_scld_2_2to3_right_in;
   output vidin_new_data_scld_4_2to3_right; 
   reg vidin_new_data_scld_4_2to3_right;
   output[15:0] vidin_data_reg_scld_4_2to3_right_rp; 
   reg[15:0] vidin_data_reg_scld_4_2to3_right_rp;
   output[15:0] vidin_data_reg_scld_4_2to3_right_ip; 
   reg[15:0] vidin_data_reg_scld_4_2to3_right_ip;
   output[15:0] vidin_data_reg_scld_4_2to3_right_rn; 
   reg[15:0] vidin_data_reg_scld_4_2to3_right_rn;
   output[15:0] vidin_data_reg_scld_4_2to3_right_in; 
   reg[15:0] vidin_data_reg_scld_4_2to3_right_in;
   input[15:0] bus_word_3; 
   input[15:0] bus_word_4; 
   input[15:0] bus_word_5; 
   input[15:0] bus_word_6; 
   input[2:0] counter_out; 

   reg[15:0] bus_word_3_tmp; 
   reg[15:0] bus_word_4_tmp; 
   reg[15:0] bus_word_5_tmp; 
   reg[15:0] bus_word_6_tmp; 
   reg[18:0] vidin_addr_reg_tmp; 
   reg svid_comp_switch_tmp; 
/*
   reg vidin_new_data_scld_1_2to3_left_tmp; 
   reg vidin_new_data_scld_2_2to3_left_tmp; 
   reg vidin_new_data_scld_4_2to3_left_tmp; 
   reg vidin_new_data_scld_1_2to3_right_tmp; 
   reg vidin_new_data_scld_2_2to3_right_tmp; 
   reg vidin_new_data_scld_4_2to3_right_tmp; 
*/
   reg[2:0] counter_out_tmp; 
   reg[15:0] vidin_data_reg_scld_1_2to3_left_rp_tmp; 
   reg[15:0] vidin_data_reg_scld_1_2to3_left_ip_tmp; 
   reg[15:0] vidin_data_reg_scld_1_2to3_left_rn_tmp; 
   reg[15:0] vidin_data_reg_scld_1_2to3_left_in_tmp; 
   reg[15:0] vidin_data_reg_scld_2_2to3_left_rp_tmp; 
   reg[15:0] vidin_data_reg_scld_2_2to3_left_ip_tmp; 
   reg[15:0] vidin_data_reg_scld_2_2to3_left_rn_tmp; 
   reg[15:0] vidin_data_reg_scld_2_2to3_left_in_tmp; 
   reg[15:0] vidin_data_reg_scld_4_2to3_left_rp_tmp; 
   reg[15:0] vidin_data_reg_scld_4_2to3_left_ip_tmp; 
   reg[15:0] vidin_data_reg_scld_4_2to3_left_rn_tmp; 
   reg[15:0] vidin_data_reg_scld_4_2to3_left_in_tmp; 
   reg[15:0] vidin_data_reg_scld_1_2to3_right_rp_tmp; 
   reg[15:0] vidin_data_reg_scld_1_2to3_right_ip_tmp; 
   reg[15:0] vidin_data_reg_scld_1_2to3_right_rn_tmp; 
   reg[15:0] vidin_data_reg_scld_1_2to3_right_in_tmp; 
   reg[15:0] vidin_data_reg_scld_2_2to3_right_rp_tmp; 
   reg[15:0] vidin_data_reg_scld_2_2to3_right_ip_tmp; 
   reg[15:0] vidin_data_reg_scld_2_2to3_right_rn_tmp; 
   reg[15:0] vidin_data_reg_scld_2_2to3_right_in_tmp; 
   reg[15:0] vidin_data_reg_scld_4_2to3_right_rp_tmp; 
   reg[15:0] vidin_data_reg_scld_4_2to3_right_ip_tmp; 
   reg[15:0] vidin_data_reg_scld_4_2to3_right_rn_tmp; 
   reg[15:0] vidin_data_reg_scld_4_2to3_right_in_tmp; 

   always @(posedge clk)
   begin
         case (counter_out_tmp[2:0])
// took out noop case 3'b000
            3'b001 :
                     begin
                        vidin_addr_reg_tmp[15:0] <= bus_word_3_tmp ; 
                        vidin_addr_reg_tmp[18:16] <= bus_word_4_tmp[15:13] ; 

                       svid_comp_switch_tmp <= bus_word_4_tmp[12] ; 
/*
                        vidin_new_data_scld_1_2to3_left_tmp <= bus_word_4_tmp[11] ; 
                        vidin_new_data_scld_2_2to3_left_tmp <= bus_word_4_tmp[10] ; 
                        vidin_new_data_scld_4_2to3_left_tmp <= bus_word_4_tmp[9] ; 
                        vidin_new_data_scld_1_2to3_right_tmp <= bus_word_4_tmp[8] ; 
                        vidin_new_data_scld_2_2to3_right_tmp <= bus_word_4_tmp[7] ; 
                        vidin_new_data_scld_4_2to3_right_tmp <= bus_word_4_tmp[6] ; 
*/
                     end
            3'b010 :
                     begin
                        vidin_data_reg_scld_1_2to3_left_rp_tmp <= bus_word_3_tmp ; 
                        vidin_data_reg_scld_1_2to3_left_ip_tmp <= bus_word_4_tmp ; 
                        vidin_data_reg_scld_1_2to3_left_rn_tmp <= bus_word_5_tmp ; 
                        vidin_data_reg_scld_1_2to3_left_in_tmp <= bus_word_6_tmp ; 
                     end
            3'b011 :
                     begin
                        vidin_data_reg_scld_1_2to3_right_rp_tmp <= bus_word_3_tmp ; 
                        vidin_data_reg_scld_1_2to3_right_ip_tmp <= bus_word_4_tmp ; 
                        vidin_data_reg_scld_1_2to3_right_rn_tmp <= bus_word_5_tmp ; 
                        vidin_data_reg_scld_1_2to3_right_in_tmp <= bus_word_6_tmp ; 
                     end
            3'b100 :
                     begin
                        vidin_data_reg_scld_2_2to3_left_rp_tmp <= bus_word_3_tmp ; 
                        vidin_data_reg_scld_2_2to3_left_ip_tmp <= bus_word_4_tmp ; 
                        vidin_data_reg_scld_2_2to3_left_rn_tmp <= bus_word_5_tmp ; 
                        vidin_data_reg_scld_2_2to3_left_in_tmp <= bus_word_6_tmp ; 
                     end
            3'b101 :
                     begin
                        vidin_data_reg_scld_2_2to3_right_rp_tmp <= bus_word_3_tmp ; 
                        vidin_data_reg_scld_2_2to3_right_ip_tmp <= bus_word_4_tmp ; 
                        vidin_data_reg_scld_2_2to3_right_rn_tmp <= bus_word_5_tmp ; 
                        vidin_data_reg_scld_2_2to3_right_in_tmp <= bus_word_6_tmp ; 
                     end
            3'b110 :
                     begin
                        vidin_data_reg_scld_4_2to3_left_rp_tmp <= bus_word_3_tmp ; 
                        vidin_data_reg_scld_4_2to3_left_ip_tmp <= bus_word_4_tmp ; 
                        vidin_data_reg_scld_4_2to3_left_rn_tmp <= bus_word_5_tmp ; 
                        vidin_data_reg_scld_4_2to3_left_in_tmp <= bus_word_6_tmp ; 
                     end
            3'b111 :
                     begin
                        vidin_data_reg_scld_4_2to3_right_rp_tmp <= bus_word_3_tmp ; 
                        vidin_data_reg_scld_4_2to3_right_ip_tmp <= bus_word_4_tmp ; 
                        vidin_data_reg_scld_4_2to3_right_rn_tmp <= bus_word_5_tmp ; 
                        vidin_data_reg_scld_4_2to3_right_in_tmp <= bus_word_6_tmp ; 
                     end
         endcase 
   end 

   always @(posedge clk)
    begin
         counter_out_tmp <= counter_out ; 
         bus_word_3_tmp <= bus_word_3 ; 
         bus_word_4_tmp <= bus_word_4 ; 
         bus_word_5_tmp <= bus_word_5 ; 
         bus_word_6_tmp <= bus_word_6 ; 
   end 

   always @(posedge clk)
   begin
         if (counter_out_tmp == 3'b001)
         begin
            vidin_addr_reg <= vidin_addr_reg_tmp ; 
            svid_comp_switch <= svid_comp_switch_tmp ; 
            vidin_new_data_scld_1_2to3_left <= 1'b1 ; 
            if (((vidin_addr_reg_tmp[0]) == 1'b0) && ((vidin_addr_reg_tmp[9]) == 1'b0))
            begin
               vidin_new_data_scld_2_2to3_left <= 1'b1 ; 
               if (((vidin_addr_reg_tmp[1]) == 1'b0) && ((vidin_addr_reg_tmp[10]) == 1'b0))
               begin
                  vidin_new_data_scld_4_2to3_left <= 1'b1 ; 
               end 
				else
				begin
                  vidin_new_data_scld_4_2to3_left <= vidin_new_data_scld_4_2to3_left;
				end
            end 
			else
			begin
			   vidin_new_data_scld_2_2to3_left <= vidin_new_data_scld_4_2to3_left ; 
                  vidin_new_data_scld_4_2to3_left <= vidin_new_data_scld_4_2to3_left;
			end
            vidin_new_data_scld_1_2to3_right <= 1'b1 ; 
            vidin_new_data_scld_2_2to3_right <= 1'b1 ; 
            vidin_new_data_scld_4_2to3_right <= 1'b1 ; 
            vidin_data_reg_scld_1_2to3_left_rp <= vidin_data_reg_scld_1_2to3_left_rp_tmp ; 
            vidin_data_reg_scld_1_2to3_left_ip <= vidin_data_reg_scld_1_2to3_left_ip_tmp ; 
            vidin_data_reg_scld_1_2to3_left_rn <= vidin_data_reg_scld_1_2to3_left_rn_tmp ; 
            vidin_data_reg_scld_1_2to3_left_in <= vidin_data_reg_scld_1_2to3_left_in_tmp ; 
            vidin_data_reg_scld_2_2to3_left_rp <= vidin_data_reg_scld_2_2to3_left_rp_tmp ; 
            vidin_data_reg_scld_2_2to3_left_ip <= vidin_data_reg_scld_2_2to3_left_ip_tmp ; 
            vidin_data_reg_scld_2_2to3_left_rn <= vidin_data_reg_scld_2_2to3_left_rn_tmp ; 
            vidin_data_reg_scld_2_2to3_left_in <= vidin_data_reg_scld_2_2to3_left_in_tmp ; 
            vidin_data_reg_scld_4_2to3_left_rp <= vidin_data_reg_scld_4_2to3_left_rp_tmp ; 
            vidin_data_reg_scld_4_2to3_left_ip <= vidin_data_reg_scld_4_2to3_left_ip_tmp ; 
            vidin_data_reg_scld_4_2to3_left_rn <= vidin_data_reg_scld_4_2to3_left_rn_tmp ; 
            vidin_data_reg_scld_4_2to3_left_in <= vidin_data_reg_scld_4_2to3_left_in_tmp ; 
            vidin_data_reg_scld_1_2to3_right_rp <= vidin_data_reg_scld_1_2to3_right_rp_tmp ; 
            vidin_data_reg_scld_1_2to3_right_ip <= vidin_data_reg_scld_1_2to3_right_ip_tmp ; 
            vidin_data_reg_scld_1_2to3_right_rn <= vidin_data_reg_scld_1_2to3_right_rn_tmp ; 
            vidin_data_reg_scld_1_2to3_right_in <= vidin_data_reg_scld_1_2to3_right_in_tmp ; 
            vidin_data_reg_scld_2_2to3_right_rp <= vidin_data_reg_scld_2_2to3_right_rp_tmp ; 
            vidin_data_reg_scld_2_2to3_right_ip <= vidin_data_reg_scld_2_2to3_right_ip_tmp ; 
            vidin_data_reg_scld_2_2to3_right_rn <= vidin_data_reg_scld_2_2to3_right_rn_tmp ; 
            vidin_data_reg_scld_2_2to3_right_in <= vidin_data_reg_scld_2_2to3_right_in_tmp ; 
            vidin_data_reg_scld_4_2to3_right_rp <= vidin_data_reg_scld_4_2to3_right_rp_tmp ; 
            vidin_data_reg_scld_4_2to3_right_ip <= vidin_data_reg_scld_4_2to3_right_ip_tmp ; 
            vidin_data_reg_scld_4_2to3_right_rn <= vidin_data_reg_scld_4_2to3_right_rn_tmp ; 
            vidin_data_reg_scld_4_2to3_right_in <= vidin_data_reg_scld_4_2to3_right_in_tmp ; 
         end
         else
         begin
            vidin_new_data_scld_1_2to3_left <= 1'b0 ; 
            vidin_new_data_scld_2_2to3_left <= 1'b0 ; 
            vidin_new_data_scld_4_2to3_left <= 1'b0 ; 
            vidin_new_data_scld_1_2to3_right <= 1'b0 ; 
            vidin_new_data_scld_2_2to3_right <= 1'b0 ; 
            vidin_new_data_scld_4_2to3_right <= 1'b0 ; 
            vidin_addr_reg <= vidin_addr_reg; 
            svid_comp_switch <= svid_comp_switch; 
            vidin_data_reg_scld_1_2to3_left_rp <= vidin_data_reg_scld_1_2to3_left_rp; 
            vidin_data_reg_scld_1_2to3_left_ip <= vidin_data_reg_scld_1_2to3_left_ip; 
            vidin_data_reg_scld_1_2to3_left_rn <= vidin_data_reg_scld_1_2to3_left_rn; 
            vidin_data_reg_scld_1_2to3_left_in <= vidin_data_reg_scld_1_2to3_left_in; 
            vidin_data_reg_scld_2_2to3_left_rp <= vidin_data_reg_scld_2_2to3_left_rp; 
            vidin_data_reg_scld_2_2to3_left_ip <= vidin_data_reg_scld_2_2to3_left_ip; 
            vidin_data_reg_scld_2_2to3_left_rn <= vidin_data_reg_scld_2_2to3_left_rn; 
            vidin_data_reg_scld_2_2to3_left_in <= vidin_data_reg_scld_2_2to3_left_in; 
            vidin_data_reg_scld_4_2to3_left_rp <= vidin_data_reg_scld_4_2to3_left_rp; 
            vidin_data_reg_scld_4_2to3_left_ip <= vidin_data_reg_scld_4_2to3_left_ip; 
            vidin_data_reg_scld_4_2to3_left_rn <= vidin_data_reg_scld_4_2to3_left_rn; 
            vidin_data_reg_scld_4_2to3_left_in <= vidin_data_reg_scld_4_2to3_left_in; 
            vidin_data_reg_scld_1_2to3_right_rp <= vidin_data_reg_scld_1_2to3_right_rp; 
            vidin_data_reg_scld_1_2to3_right_ip <= vidin_data_reg_scld_1_2to3_right_ip; 
            vidin_data_reg_scld_1_2to3_right_rn <= vidin_data_reg_scld_1_2to3_right_rn; 
            vidin_data_reg_scld_1_2to3_right_in <= vidin_data_reg_scld_1_2to3_right_in; 
            vidin_data_reg_scld_2_2to3_right_rp <= vidin_data_reg_scld_2_2to3_right_rp; 
            vidin_data_reg_scld_2_2to3_right_ip <= vidin_data_reg_scld_2_2to3_right_ip; 
            vidin_data_reg_scld_2_2to3_right_rn <= vidin_data_reg_scld_2_2to3_right_rn; 
            vidin_data_reg_scld_2_2to3_right_in <= vidin_data_reg_scld_2_2to3_right_in; 
            vidin_data_reg_scld_4_2to3_right_rp <= vidin_data_reg_scld_4_2to3_right_rp; 
            vidin_data_reg_scld_4_2to3_right_ip <= vidin_data_reg_scld_4_2to3_right_ip; 
            vidin_data_reg_scld_4_2to3_right_rn <= vidin_data_reg_scld_4_2to3_right_rn; 
            vidin_data_reg_scld_4_2to3_right_in <= vidin_data_reg_scld_4_2to3_right_in; 
         end 
   end 
endmodule
module wrapper_norm_corr_20 (clk, wen, d_l_1, d_l_2, d_r_1, d_r_2, corr_out_0, corr_out_1, corr_out_2, corr_out_3, corr_out_4, corr_out_5, corr_out_6, corr_out_7, corr_out_8, corr_out_9, corr_out_10, corr_out_11, corr_out_12, corr_out_13, corr_out_14, corr_out_15, corr_out_16, corr_out_17, corr_out_18, corr_out_19, corr_out_20);

   parameter sh_reg_w  = 4'b1000;
   input clk; 
   input wen; 
   input[15:0] d_l_1; 
   input[15:0] d_l_2; 
   input[15:0] d_r_1; 
   input[15:0] d_r_2; 
   output[2 * sh_reg_w - 1:0] corr_out_0; 
   wire[2 * sh_reg_w - 1:0] corr_out_0;
   output[2 * sh_reg_w - 1:0] corr_out_1; 
   wire[2 * sh_reg_w - 1:0] corr_out_1;
   output[2 * sh_reg_w - 1:0] corr_out_2; 
   wire[2 * sh_reg_w - 1:0] corr_out_2;
   output[2 * sh_reg_w - 1:0] corr_out_3; 
   wire[2 * sh_reg_w - 1:0] corr_out_3;
   output[2 * sh_reg_w - 1:0] corr_out_4; 
   wire[2 * sh_reg_w - 1:0] corr_out_4;
   output[2 * sh_reg_w - 1:0] corr_out_5; 
   wire[2 * sh_reg_w - 1:0] corr_out_5;
   output[2 * sh_reg_w - 1:0] corr_out_6; 
   wire[2 * sh_reg_w - 1:0] corr_out_6;
   output[2 * sh_reg_w - 1:0] corr_out_7; 
   wire[2 * sh_reg_w - 1:0] corr_out_7;
   output[2 * sh_reg_w - 1:0] corr_out_8; 
   wire[2 * sh_reg_w - 1:0] corr_out_8;
   output[2 * sh_reg_w - 1:0] corr_out_9; 
   wire[2 * sh_reg_w - 1:0] corr_out_9;
   output[2 * sh_reg_w - 1:0] corr_out_10; 
   wire[2 * sh_reg_w - 1:0] corr_out_10;
   output[2 * sh_reg_w - 1:0] corr_out_11; 
   wire[2 * sh_reg_w - 1:0] corr_out_11;
   output[2 * sh_reg_w - 1:0] corr_out_12; 
   wire[2 * sh_reg_w - 1:0] corr_out_12;
   output[2 * sh_reg_w - 1:0] corr_out_13; 
   wire[2 * sh_reg_w - 1:0] corr_out_13;
   output[2 * sh_reg_w - 1:0] corr_out_14; 
   wire[2 * sh_reg_w - 1:0] corr_out_14;
   output[2 * sh_reg_w - 1:0] corr_out_15; 
   wire[2 * sh_reg_w - 1:0] corr_out_15;
   output[2 * sh_reg_w - 1:0] corr_out_16; 
   wire[2 * sh_reg_w - 1:0] corr_out_16;
   output[2 * sh_reg_w - 1:0] corr_out_17; 
   wire[2 * sh_reg_w - 1:0] corr_out_17;
   output[2 * sh_reg_w - 1:0] corr_out_18; 
   wire[2 * sh_reg_w - 1:0] corr_out_18;
   output[2 * sh_reg_w - 1:0] corr_out_19; 
   wire[2 * sh_reg_w - 1:0] corr_out_19;
   output[2 * sh_reg_w - 1:0] corr_out_20; 
   wire[2 * sh_reg_w - 1:0] corr_out_20;

   wire[sh_reg_w - 1:0] d_l_1_nrm; 
   wire[sh_reg_w - 1:0] d_l_2_nrm; 
   wire[sh_reg_w - 1:0] d_r_1_nrm; 
   wire[sh_reg_w - 1:0] d_r_2_nrm; 

   wrapper_norm norm_inst_left(.clk(clk), .nd(wen), .din_1(d_l_1), .din_2(d_l_2), .dout_1(d_l_1_nrm), .dout_2(d_l_2_nrm)); 
   wrapper_norm  norm_inst_right(.clk(clk), .nd(wen), .din_1(d_r_1), .din_2(d_r_2), .dout_1(d_r_1_nrm), .dout_2(d_r_2_nrm)); 
   wrapper_corr_20 corr_20_inst(.clk(clk), .wen(wen), .d_l_1(d_l_1_nrm), .d_l_2(d_l_2_nrm), .d_r_1(d_r_1_nrm), .d_r_2(d_r_2_nrm), .corr_out_0(corr_out_0), .corr_out_1(corr_out_1), .corr_out_2(corr_out_2), .corr_out_3(corr_out_3), .corr_out_4(corr_out_4), .corr_out_5(corr_out_5), .corr_out_6(corr_out_6), .corr_out_7(corr_out_7), .corr_out_8(corr_out_8), .corr_out_9(corr_out_9), .corr_out_10(corr_out_10), .corr_out_11(corr_out_11), .corr_out_12(corr_out_12), .corr_out_13(corr_out_13), .corr_out_14(corr_out_14), .corr_out_15(corr_out_15), .corr_out_16(corr_out_16), .corr_out_17(corr_out_17), .corr_out_18(corr_out_18), .corr_out_19(corr_out_19), .corr_out_20(corr_out_20));
endmodule
module wrapper_corr_20 (clk, wen, d_l_1, d_l_2, d_r_1, d_r_2, corr_out_0, corr_out_1, corr_out_2, corr_out_3, corr_out_4, corr_out_5, corr_out_6, corr_out_7, corr_out_8, corr_out_9, corr_out_10, corr_out_11, corr_out_12, corr_out_13, corr_out_14, corr_out_15, corr_out_16, corr_out_17, corr_out_18, corr_out_19, corr_out_20);

   parameter sh_reg_w  = 4'b1000;
   input clk; 
   input wen; 
   input[7:0] d_l_1; 
   input[7:0] d_l_2; 
   input[7:0] d_r_1; 
   input[7:0] d_r_2; 
   output[2 * sh_reg_w - 1:0] corr_out_0; 
   reg[2 * sh_reg_w - 1:0] corr_out_0;
   output[2 * sh_reg_w - 1:0] corr_out_1; 
   reg[2 * sh_reg_w - 1:0] corr_out_1;
   output[2 * sh_reg_w - 1:0] corr_out_2; 
   reg[2 * sh_reg_w - 1:0] corr_out_2;
   output[2 * sh_reg_w - 1:0] corr_out_3; 
   reg[2 * sh_reg_w - 1:0] corr_out_3;
   output[2 * sh_reg_w - 1:0] corr_out_4; 
   reg[2 * sh_reg_w - 1:0] corr_out_4;
   output[2 * sh_reg_w - 1:0] corr_out_5; 
   reg[2 * sh_reg_w - 1:0] corr_out_5;
   output[2 * sh_reg_w - 1:0] corr_out_6; 
   reg[2 * sh_reg_w - 1:0] corr_out_6;
   output[2 * sh_reg_w - 1:0] corr_out_7; 
   reg[2 * sh_reg_w - 1:0] corr_out_7;
   output[2 * sh_reg_w - 1:0] corr_out_8; 
   reg[2 * sh_reg_w - 1:0] corr_out_8;
   output[2 * sh_reg_w - 1:0] corr_out_9; 
   reg[2 * sh_reg_w - 1:0] corr_out_9;
   output[2 * sh_reg_w - 1:0] corr_out_10; 
   reg[2 * sh_reg_w - 1:0] corr_out_10;
   output[2 * sh_reg_w - 1:0] corr_out_11; 
   reg[2 * sh_reg_w - 1:0] corr_out_11;
   output[2 * sh_reg_w - 1:0] corr_out_12; 
   reg[2 * sh_reg_w - 1:0] corr_out_12;
   output[2 * sh_reg_w - 1:0] corr_out_13; 
   reg[2 * sh_reg_w - 1:0] corr_out_13;
   output[2 * sh_reg_w - 1:0] corr_out_14; 
   reg[2 * sh_reg_w - 1:0] corr_out_14;
   output[2 * sh_reg_w - 1:0] corr_out_15; 
   reg[2 * sh_reg_w - 1:0] corr_out_15;
   output[2 * sh_reg_w - 1:0] corr_out_16; 
   reg[2 * sh_reg_w - 1:0] corr_out_16;
   output[2 * sh_reg_w - 1:0] corr_out_17; 
   reg[2 * sh_reg_w - 1:0] corr_out_17;
   output[2 * sh_reg_w - 1:0] corr_out_18; 
   reg[2 * sh_reg_w - 1:0] corr_out_18;
   output[2 * sh_reg_w - 1:0] corr_out_19; 
   reg[2 * sh_reg_w - 1:0] corr_out_19;
   output[2 * sh_reg_w - 1:0] corr_out_20; 
   reg[2 * sh_reg_w - 1:0] corr_out_20;

   wire[sh_reg_w - 1:0] out_r1; 
   wire[sh_reg_w - 1:0] out_01; 

   wire[sh_reg_w - 1:0] out_11; 
   wire[sh_reg_w - 1:0] out_21; 
   wire[sh_reg_w - 1:0] out_31; 
   wire[sh_reg_w - 1:0] out_41; 
   wire[sh_reg_w - 1:0] out_51; 
   wire[sh_reg_w - 1:0] out_61; 
   wire[sh_reg_w - 1:0] out_71; 
   wire[sh_reg_w - 1:0] out_81; 
   wire[sh_reg_w - 1:0] out_91; 
   wire[sh_reg_w - 1:0] out_101; 
   wire[sh_reg_w - 1:0] out_111; 
   wire[sh_reg_w - 1:0] out_121; 
   wire[sh_reg_w - 1:0] out_131; 
   wire[sh_reg_w - 1:0] out_141; 
   wire[sh_reg_w - 1:0] out_151; 
   wire[sh_reg_w - 1:0] out_161; 
   wire[sh_reg_w - 1:0] out_171; 
   wire[sh_reg_w - 1:0] out_181; 
   wire[sh_reg_w - 1:0] out_191; 
   wire[sh_reg_w - 1:0] out_201; 
   wire[sh_reg_w - 1:0] out_r2; 
   wire[sh_reg_w - 1:0] out_02; 
   wire[sh_reg_w - 1:0] out_12; 
   wire[sh_reg_w - 1:0] out_22; 
   wire[sh_reg_w - 1:0] out_32; 
   wire[sh_reg_w - 1:0] out_42; 
   wire[sh_reg_w - 1:0] out_52; 
   wire[sh_reg_w - 1:0] out_62; 
   wire[sh_reg_w - 1:0] out_72; 
   wire[sh_reg_w - 1:0] out_82; 
   wire[sh_reg_w - 1:0] out_92; 
   wire[sh_reg_w - 1:0] out_102; 
   wire[sh_reg_w - 1:0] out_112; 
   wire[sh_reg_w - 1:0] out_122; 
   wire[sh_reg_w - 1:0] out_132; 
   wire[sh_reg_w - 1:0] out_142; 
   wire[sh_reg_w - 1:0] out_152; 
   wire[sh_reg_w - 1:0] out_162; 
   wire[sh_reg_w - 1:0] out_172; 
   wire[sh_reg_w - 1:0] out_182; 
   wire[sh_reg_w - 1:0] out_192; 
   wire[sh_reg_w - 1:0] out_202; 
   wire[2 * sh_reg_w - 1:0] corr_out_0_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_1_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_2_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_3_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_4_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_5_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_6_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_7_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_8_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_9_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_10_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_11_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_12_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_13_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_14_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_15_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_16_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_17_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_18_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_19_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_20_tmp; 

   sh_reg  inst_sh_reg_r_1(clk, wen, d_r_1, d_r_2, out_r1, out_r2); 
   sh_reg  inst_sh_reg_0(clk, wen, d_l_1, d_l_2, out_01, out_02); 
   sh_reg  inst_sh_reg_1(clk, wen, out_01, out_02, out_11, out_12); 
   sh_reg  inst_sh_reg_2(clk, wen, out_11, out_12, out_21, out_22); 
   sh_reg  inst_sh_reg_3(clk, wen, out_21, out_22, out_31, out_32); 
   sh_reg  inst_sh_reg_4(clk, wen, out_31, out_32, out_41, out_42); 
   sh_reg  inst_sh_reg_5(clk, wen, out_41, out_42, out_51, out_52); 
   sh_reg  inst_sh_reg_6(clk, wen, out_51, out_52, out_61, out_62); 
   sh_reg  inst_sh_reg_7(clk, wen, out_61, out_62, out_71, out_72); 
   sh_reg  inst_sh_reg_8(clk, wen, out_71, out_72, out_81, out_82); 
   sh_reg  inst_sh_reg_9(clk, wen, out_81, out_82, out_91, out_92); 
   sh_reg  inst_sh_reg_10(clk, wen, out_91, out_92, out_101, out_102); 
   sh_reg  inst_sh_reg_11(clk, wen, out_101, out_102, out_111, out_112); 
   sh_reg  inst_sh_reg_12(clk, wen, out_111, out_112, out_121, out_122); 
   sh_reg  inst_sh_reg_13(clk, wen, out_121, out_122, out_131, out_132); 
   sh_reg  inst_sh_reg_14(clk, wen, out_131, out_132, out_141, out_142); 
   sh_reg  inst_sh_reg_15(clk, wen, out_141, out_142, out_151, out_152); 
   sh_reg  inst_sh_reg_16(clk, wen, out_151, out_152, out_161, out_162); 
   sh_reg  inst_sh_reg_17(clk, wen, out_161, out_162, out_171, out_172); 
   sh_reg  inst_sh_reg_18(clk, wen, out_171, out_172, out_181, out_182); 
   sh_reg  inst_sh_reg_19(clk, wen, out_181, out_182, out_191, out_192); 
   sh_reg  inst_sh_reg_20(clk, wen, out_191, out_192, out_201, out_202); 
   corr  inst_corr_0(clk, wen, out_01, out_02, out_r1, out_r2, corr_out_0_tmp); 
   corr  inst_corr_1(clk, wen, out_11, out_12, out_r1, out_r2, corr_out_1_tmp); 
   corr  inst_corr_2(clk, wen, out_21, out_22, out_r1, out_r2, corr_out_2_tmp); 
   corr  inst_corr_3(clk, wen, out_31, out_32, out_r1, out_r2, corr_out_3_tmp); 
   corr  inst_corr_4(clk, wen, out_41, out_42, out_r1, out_r2, corr_out_4_tmp); 
   corr  inst_corr_5(clk, wen, out_51, out_52, out_r1, out_r2, corr_out_5_tmp); 
   corr  inst_corr_6(clk, wen, out_61, out_62, out_r1, out_r2, corr_out_6_tmp); 
   corr  inst_corr_7(clk, wen, out_71, out_72, out_r1, out_r2, corr_out_7_tmp); 
   corr  inst_corr_8(clk, wen, out_81, out_82, out_r1, out_r2, corr_out_8_tmp); 
   corr  inst_corr_9(clk, wen, out_91, out_92, out_r1, out_r2, corr_out_9_tmp); 
   corr  inst_corr_10(clk, wen, out_101, out_102, out_r1, out_r2, corr_out_10_tmp); 
   corr  inst_corr_11(clk, wen, out_111, out_112, out_r1, out_r2, corr_out_11_tmp); 
   corr  inst_corr_12(clk, wen, out_121, out_122, out_r1, out_r2, corr_out_12_tmp); 
   corr  inst_corr_13(clk, wen, out_131, out_132, out_r1, out_r2, corr_out_13_tmp); 
   corr  inst_corr_14(clk, wen, out_141, out_142, out_r1, out_r2, corr_out_14_tmp); 
   corr  inst_corr_15(clk, wen, out_151, out_152, out_r1, out_r2, corr_out_15_tmp); 
   corr  inst_corr_16(clk, wen, out_161, out_162, out_r1, out_r2, corr_out_16_tmp); 
   corr  inst_corr_17(clk, wen, out_171, out_172, out_r1, out_r2, corr_out_17_tmp); 
   corr  inst_corr_18(clk, wen, out_181, out_182, out_r1, out_r2, corr_out_18_tmp); 
   corr  inst_corr_19(clk, wen, out_191, out_192, out_r1, out_r2, corr_out_19_tmp); 
   corr  inst_corr_20(clk, wen, out_201, out_202, out_r1, out_r2, corr_out_20_tmp); 

   always @(posedge clk)
   begin
      if (wen == 1'b1)
         begin
            corr_out_0 <= corr_out_0_tmp ; 
            corr_out_1 <= corr_out_1_tmp ; 
            corr_out_2 <= corr_out_2_tmp ; 
            corr_out_3 <= corr_out_3_tmp ; 
            corr_out_4 <= corr_out_4_tmp ; 
            corr_out_5 <= corr_out_5_tmp ; 
            corr_out_6 <= corr_out_6_tmp ; 
            corr_out_7 <= corr_out_7_tmp ; 
            corr_out_8 <= corr_out_8_tmp ; 
            corr_out_9 <= corr_out_9_tmp ; 
            corr_out_10 <= corr_out_10_tmp ; 
            corr_out_11 <= corr_out_11_tmp ; 
            corr_out_12 <= corr_out_12_tmp ; 
            corr_out_13 <= corr_out_13_tmp ; 
            corr_out_14 <= corr_out_14_tmp ; 
            corr_out_15 <= corr_out_15_tmp ; 
            corr_out_16 <= corr_out_16_tmp ; 
            corr_out_17 <= corr_out_17_tmp ; 
            corr_out_18 <= corr_out_18_tmp ; 
            corr_out_19 <= corr_out_19_tmp ; 
            corr_out_20 <= corr_out_20_tmp ; 
         end 
         else
         begin
            corr_out_0 <= corr_out_0; 
            corr_out_1 <= corr_out_1; 
            corr_out_2 <= corr_out_2; 
            corr_out_3 <= corr_out_3; 
            corr_out_4 <= corr_out_4; 
            corr_out_5 <= corr_out_5; 
            corr_out_6 <= corr_out_6; 
            corr_out_7 <= corr_out_7; 
            corr_out_8 <= corr_out_8; 
            corr_out_9 <= corr_out_9; 
            corr_out_10 <= corr_out_10; 
            corr_out_11 <= corr_out_11; 
            corr_out_12 <= corr_out_12; 
            corr_out_13 <= corr_out_13; 
            corr_out_14 <= corr_out_14; 
            corr_out_15 <= corr_out_15; 
            corr_out_16 <= corr_out_16; 
            corr_out_17 <= corr_out_17; 
            corr_out_18 <= corr_out_18; 
            corr_out_19 <= corr_out_19; 
            corr_out_20 <= corr_out_20; 
         end 
   end 
endmodule
// Discription: this block creates a simple
// shift register
// date: Oct.7 ,2001
// revised : April 8, 2002
// By:  Ahmad darabiha
module sh_reg (clk, wen, din_1, din_2, dout_1, dout_2);

   parameter sh_reg_w  = 4'b1000;
   input clk; 
   input wen; 
   input[sh_reg_w - 1:0] din_1; 
   input[sh_reg_w - 1:0] din_2; 
   output[sh_reg_w - 1:0] dout_1; 
   reg[sh_reg_w - 1:0] dout_1;
   output[sh_reg_w - 1:0] dout_2; 
   reg[sh_reg_w - 1:0] dout_2;

   always @(posedge clk)
   begin
         if (wen == 1'b1)
         begin
            dout_1 <= din_1 ; 
            dout_2 <= din_2 ; 
         end 
		else
		begin
            dout_1 <= dout_1 ; 
            dout_2 <= dout_2 ; 
		end
   end 
endmodule
module corr (clk, new_data, in_l_re, in_l_im, in_r_re, in_r_im, corr_out);

    parameter sh_reg_w  = 4'b1000;
    input clk; 
    input new_data; 
    input[sh_reg_w - 1:0] in_l_re; 
    input[sh_reg_w - 1:0] in_l_im; 
    input[sh_reg_w - 1:0] in_r_re; 
    input[sh_reg_w - 1:0] in_r_im; 
    output[2 * sh_reg_w - 1:0] corr_out; 
    reg[2 * sh_reg_w - 1:0] corr_out;
    wire[sh_reg_w - 1:0] in_l_re_reg; 
    wire[sh_reg_w - 1:0] in_l_im_reg; 
    wire[sh_reg_w - 1:0] in_r_re_reg; 
    wire[sh_reg_w - 1:0] in_r_im_reg; 
    reg[2 * sh_reg_w - 1:0] lrexrre_reg; 
    reg[2 * sh_reg_w - 1:0] limxrim_reg; 
    reg[2 * sh_reg_w - 1:0] corr_out_tmp; 

    always @(posedge clk)
    begin
			// PAJ - edf xilinx files converted to multiply 
			lrexrre_reg <= in_l_re * in_r_re;
			limxrim_reg <= in_l_im * in_r_im;

          if (new_data == 1'b1)
          begin
             corr_out <= corr_out_tmp ; 
          end 
          else
          begin
             corr_out <= corr_out; 
          end 
          corr_out_tmp <= lrexrre_reg + limxrim_reg ; 
    end 
 endmodule
module wrapper_norm (clk, nd, din_1, din_2, dout_1, dout_2);

   parameter sh_reg_w  = 4'b1000;
   input clk; 
   input nd; 
   input[15:0] din_1; 
   input[15:0] din_2; 
   output[sh_reg_w - 1:0] dout_1; 
   wire[sh_reg_w - 1:0] dout_1;
   output[sh_reg_w - 1:0] dout_2; 
   wire[sh_reg_w - 1:0] dout_2;

   reg[15:0] din_1_reg; 
   reg[15:0] din_2_reg; 
   reg[15:0] din_1_tmp1; 
   reg[15:0] din_2_tmp1; 
   reg[15:0] din_1_tmp2; 
   reg[15:0] din_2_tmp2; 
   reg[15:0] addin_1; 
   reg[15:0] addin_2; 
   reg[16:0] add_out; 

   my_wrapper_divider my_div_inst_1 (nd, clk, din_1_tmp2, add_out, dout_1); 
   my_wrapper_divider my_div_inst_2 (nd, clk, din_2_tmp2, add_out, dout_2); 

   always @(posedge clk)
   begin
         if (nd == 1'b1)
         begin
            din_1_reg <= din_1 ; 
            din_2_reg <= din_2 ; 
         end 
         else
         begin
            din_1_reg <= din_1_reg ; 
            din_2_reg <= din_2_reg; 
         end 
         din_1_tmp1 <= din_1_reg ; 
         din_1_tmp2 <= din_1_tmp1 ; 
         din_2_tmp1 <= din_2_reg ; 
         din_2_tmp2 <= din_2_tmp1 ; 
         if ((din_1_reg[15]) == 1'b0)
         begin
            addin_1 <= din_1_reg ; 
         end
         else
         begin
            addin_1 <= 16'b0000000000000000 - din_1_reg ; 
         end 
         if ((din_2_reg[15]) == 1'b0)
         begin
            addin_2 <= din_2_reg + 16'b0000000000000001 ; 
         end
         else
         begin
            addin_2 <= 16'b0000000000000001 - din_2_reg ; 
         end 
         add_out <= ({addin_1[15], addin_1}) + ({addin_2[15], addin_2}) ; 
   end 
endmodule




module wrapper_norm_corr_10 (clk, wen, d_l_1, d_l_2, d_r_1, d_r_2, corr_out_0, corr_out_1, corr_out_2, corr_out_3, corr_out_4, corr_out_5, corr_out_6, corr_out_7, corr_out_8, corr_out_9, corr_out_10);

   parameter sh_reg_w  = 4'b1000;
   input clk; 
   input wen; 
   input[15:0] d_l_1; 
   input[15:0] d_l_2; 
   input[15:0] d_r_1; 
   input[15:0] d_r_2; 
   output[2 * sh_reg_w - 1:0] corr_out_0; 
   wire[2 * sh_reg_w - 1:0] corr_out_0;
   output[2 * sh_reg_w - 1:0] corr_out_1; 
   wire[2 * sh_reg_w - 1:0] corr_out_1;
   output[2 * sh_reg_w - 1:0] corr_out_2; 
   wire[2 * sh_reg_w - 1:0] corr_out_2;
   output[2 * sh_reg_w - 1:0] corr_out_3; 
   wire[2 * sh_reg_w - 1:0] corr_out_3;
   output[2 * sh_reg_w - 1:0] corr_out_4; 
   wire[2 * sh_reg_w - 1:0] corr_out_4;
   output[2 * sh_reg_w - 1:0] corr_out_5; 
   wire[2 * sh_reg_w - 1:0] corr_out_5;
   output[2 * sh_reg_w - 1:0] corr_out_6; 
   wire[2 * sh_reg_w - 1:0] corr_out_6;
   output[2 * sh_reg_w - 1:0] corr_out_7; 
   wire[2 * sh_reg_w - 1:0] corr_out_7;
   output[2 * sh_reg_w - 1:0] corr_out_8; 
   wire[2 * sh_reg_w - 1:0] corr_out_8;
   output[2 * sh_reg_w - 1:0] corr_out_9; 
   wire[2 * sh_reg_w - 1:0] corr_out_9;
   output[2 * sh_reg_w - 1:0] corr_out_10; 
   wire[2 * sh_reg_w - 1:0] corr_out_10;

   wire[sh_reg_w - 1:0] d_l_1_nrm; 
   wire[sh_reg_w - 1:0] d_l_2_nrm; 
   wire[sh_reg_w - 1:0] d_r_1_nrm; 
   wire[sh_reg_w - 1:0] d_r_2_nrm; 

   wrapper_norm  norm_inst_left(.clk(clk), .nd(wen), .din_1(d_l_1), .din_2(d_l_2), .dout_1(d_l_1_nrm), .dout_2(d_l_2_nrm)); 
   wrapper_norm  norm_inst_right(.clk(clk), .nd(wen), .din_1(d_r_1), .din_2(d_r_2), .dout_1(d_r_1_nrm), .dout_2(d_r_2_nrm)); 
   wrapper_corr_10  corr_5_inst(.clk(clk), .wen(wen), .d_l_1(d_l_1_nrm), .d_l_2(d_l_2_nrm), .d_r_1(d_r_1_nrm), .d_r_2(d_r_2_nrm), .corr_out_0(corr_out_0), .corr_out_1(corr_out_1), .corr_out_2(corr_out_2), .corr_out_3(corr_out_3), .corr_out_4(corr_out_4), .corr_out_5(corr_out_5), .corr_out_6(corr_out_6), .corr_out_7(corr_out_7), .corr_out_8(corr_out_8), .corr_out_9(corr_out_9), .corr_out_10(corr_out_10));
endmodule




module wrapper_corr_10 (clk, wen, d_l_1, d_l_2, d_r_1, d_r_2, corr_out_0, corr_out_1, corr_out_2, corr_out_3, corr_out_4, corr_out_5, corr_out_6, corr_out_7, corr_out_8, corr_out_9, corr_out_10);

   parameter sh_reg_w  = 4'b1000;
   input clk; 
   input wen; 
   input[7:0] d_l_1; 
   input[7:0] d_l_2; 
   input[7:0] d_r_1; 
   input[7:0] d_r_2; 
   output[2 * sh_reg_w - 1:0] corr_out_0; 
   reg[2 * sh_reg_w - 1:0] corr_out_0;
   output[2 * sh_reg_w - 1:0] corr_out_1; 
   reg[2 * sh_reg_w - 1:0] corr_out_1;
   output[2 * sh_reg_w - 1:0] corr_out_2; 
   reg[2 * sh_reg_w - 1:0] corr_out_2;
   output[2 * sh_reg_w - 1:0] corr_out_3; 
   reg[2 * sh_reg_w - 1:0] corr_out_3;
   output[2 * sh_reg_w - 1:0] corr_out_4; 
   reg[2 * sh_reg_w - 1:0] corr_out_4;
   output[2 * sh_reg_w - 1:0] corr_out_5; 
   reg[2 * sh_reg_w - 1:0] corr_out_5;
   output[2 * sh_reg_w - 1:0] corr_out_6; 
   reg[2 * sh_reg_w - 1:0] corr_out_6;
   output[2 * sh_reg_w - 1:0] corr_out_7; 
   reg[2 * sh_reg_w - 1:0] corr_out_7;
   output[2 * sh_reg_w - 1:0] corr_out_8; 
   reg[2 * sh_reg_w - 1:0] corr_out_8;
   output[2 * sh_reg_w - 1:0] corr_out_9; 
   reg[2 * sh_reg_w - 1:0] corr_out_9;
   output[2 * sh_reg_w - 1:0] corr_out_10; 
   reg[2 * sh_reg_w - 1:0] corr_out_10;

   wire[sh_reg_w - 1:0] out_r1; 
   wire[sh_reg_w - 1:0] out_01; 
   wire[sh_reg_w - 1:0] out_11; 
   wire[sh_reg_w - 1:0] out_21; 
   wire[sh_reg_w - 1:0] out_31; 
   wire[sh_reg_w - 1:0] out_41; 
   wire[sh_reg_w - 1:0] out_51; 
   wire[sh_reg_w - 1:0] out_61; 
   wire[sh_reg_w - 1:0] out_71; 
   wire[sh_reg_w - 1:0] out_81; 
   wire[sh_reg_w - 1:0] out_91; 
   wire[sh_reg_w - 1:0] out_101; 
   wire[sh_reg_w - 1:0] out_r2; 
   wire[sh_reg_w - 1:0] out_02; 
   wire[sh_reg_w - 1:0] out_12; 
   wire[sh_reg_w - 1:0] out_22; 
   wire[sh_reg_w - 1:0] out_32; 
   wire[sh_reg_w - 1:0] out_42; 
   wire[sh_reg_w - 1:0] out_52; 
   wire[sh_reg_w - 1:0] out_62; 
   wire[sh_reg_w - 1:0] out_72; 
   wire[sh_reg_w - 1:0] out_82; 
   wire[sh_reg_w - 1:0] out_92; 

   wire[sh_reg_w - 1:0] out_102; 
   wire[2 * sh_reg_w - 1:0] corr_out_0_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_1_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_2_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_3_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_4_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_5_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_6_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_7_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_8_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_9_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_10_tmp; 

   sh_reg  inst_sh_reg_r_1(clk, wen, d_r_1, d_r_2, out_r1, out_r2); 

   sh_reg  inst_sh_reg_0(clk, wen, d_l_1, d_l_2, out_01, out_02); 

   sh_reg  inst_sh_reg_1(clk, wen, out_01, out_02, out_11, out_12); 

   sh_reg  inst_sh_reg_2(clk, wen, out_11, out_12, out_21, out_22); 

   sh_reg  inst_sh_reg_3(clk, wen, out_21, out_22, out_31, out_32); 

   sh_reg  inst_sh_reg_4(clk, wen, out_31, out_32, out_41, out_42); 

   sh_reg  inst_sh_reg_5(clk, wen, out_41, out_42, out_51, out_52); 

   sh_reg  inst_sh_reg_6(clk, wen, out_51, out_52, out_61, out_62); 

   sh_reg  inst_sh_reg_7(clk, wen, out_61, out_62, out_71, out_72); 

   sh_reg  inst_sh_reg_8(clk, wen, out_71, out_72, out_81, out_82); 

   sh_reg  inst_sh_reg_9(clk, wen, out_81, out_82, out_91, out_92); 

   sh_reg  inst_sh_reg_10(clk, wen, out_91, out_92, out_101, out_102); 

   corr  inst_corr_0(clk, wen, out_01, out_02, out_r1, out_r2, corr_out_0_tmp); 

   corr  inst_corr_1(clk, wen, out_11, out_12, out_r1, out_r2, corr_out_1_tmp); 

   corr  inst_corr_2(clk, wen, out_21, out_22, out_r1, out_r2, corr_out_2_tmp); 

   corr  inst_corr_3(clk, wen, out_31, out_32, out_r1, out_r2, corr_out_3_tmp); 

   corr  inst_corr_4(clk, wen, out_41, out_42, out_r1, out_r2, corr_out_4_tmp); 

   corr  inst_corr_5(clk, wen, out_51, out_52, out_r1, out_r2, corr_out_5_tmp); 

   corr  inst_corr_6(clk, wen, out_61, out_62, out_r1, out_r2, corr_out_6_tmp); 

   corr  inst_corr_7(clk, wen, out_71, out_72, out_r1, out_r2, corr_out_7_tmp); 

   corr  inst_corr_8(clk, wen, out_81, out_82, out_r1, out_r2, corr_out_8_tmp); 

   corr  inst_corr_9(clk, wen, out_91, out_92, out_r1, out_r2, corr_out_9_tmp); 

   corr  inst_corr_10(clk, wen, out_101, out_102, out_r1, out_r2, corr_out_10_tmp); 

   always @(posedge clk)
   begin
         if (wen == 1'b1)
         begin
            corr_out_0 <= corr_out_0_tmp ; 
            corr_out_1 <= corr_out_1_tmp ; 
            corr_out_2 <= corr_out_2_tmp ; 
            corr_out_3 <= corr_out_3_tmp ; 
            corr_out_4 <= corr_out_4_tmp ; 
            corr_out_5 <= corr_out_5_tmp ; 
            corr_out_6 <= corr_out_6_tmp ; 
            corr_out_7 <= corr_out_7_tmp ; 
            corr_out_8 <= corr_out_8_tmp ; 
            corr_out_9 <= corr_out_9_tmp ; 
            corr_out_10 <= corr_out_10_tmp ; 
         end 
         else
         begin
            corr_out_0 <= corr_out_0; 
            corr_out_1 <= corr_out_1; 
            corr_out_2 <= corr_out_2; 
            corr_out_3 <= corr_out_3; 
            corr_out_4 <= corr_out_4; 
            corr_out_5 <= corr_out_5; 
            corr_out_6 <= corr_out_6; 
            corr_out_7 <= corr_out_7; 
            corr_out_8 <= corr_out_8; 
            corr_out_9 <= corr_out_9; 
            corr_out_10 <= corr_out_10; 
         end 

   end 
endmodule



module wrapper_norm_corr_5_seq (clk, wen, d_l_1, d_l_2, d_r_1, d_r_2, corr_out_0, corr_out_1, corr_out_2, corr_out_3, corr_out_4, corr_out_5);

   parameter sh_reg_w  = 4'b1000;
   input clk; 
   input wen; 
   input[15:0] d_l_1; 
   input[15:0] d_l_2; 
   input[15:0] d_r_1; 
   input[15:0] d_r_2; 
   output[2 * sh_reg_w - 1:0] corr_out_0; 
   wire[2 * sh_reg_w - 1:0] corr_out_0;
   output[2 * sh_reg_w - 1:0] corr_out_1; 
   wire[2 * sh_reg_w - 1:0] corr_out_1;
   output[2 * sh_reg_w - 1:0] corr_out_2; 
   wire[2 * sh_reg_w - 1:0] corr_out_2;
   output[2 * sh_reg_w - 1:0] corr_out_3; 
   wire[2 * sh_reg_w - 1:0] corr_out_3;
   output[2 * sh_reg_w - 1:0] corr_out_4; 
   wire[2 * sh_reg_w - 1:0] corr_out_4;
   output[2 * sh_reg_w - 1:0] corr_out_5; 
   wire[2 * sh_reg_w - 1:0] corr_out_5;

   wire[sh_reg_w - 1:0] d_l_1_nrm; 
   wire[sh_reg_w - 1:0] d_l_2_nrm; 
   wire[sh_reg_w - 1:0] d_r_1_nrm; 
   wire[sh_reg_w - 1:0] d_r_2_nrm; 

   wrapper_norm_seq  norm_inst_left(.clk(clk), .nd(wen), .din_1(d_l_1), .din_2(d_l_2), .dout_1(d_l_1_nrm), .dout_2(d_l_2_nrm)); 
   wrapper_norm_seq  norm_inst_right(.clk(clk), .nd(wen), .din_1(d_r_1), .din_2(d_r_2), .dout_1(d_r_1_nrm), .dout_2(d_r_2_nrm)); 
   wrapper_corr_5_seq corr_5_inst (.tm3_clk_v0(clk), .wen(wen), .d_l_1(d_l_1_nrm), .d_l_2(d_l_2_nrm), .d_r_1(d_r_1_nrm), .d_r_2(d_r_2_nrm), .corr_out_0(corr_out_0), .corr_out_1(corr_out_1), .corr_out_2(corr_out_2), .corr_out_3(corr_out_3), .corr_out_4(corr_out_4), .corr_out_5(corr_out_5));
endmodule




module wrapper_corr_5_seq (tm3_clk_v0, wen, d_l_1, d_l_2, d_r_1, d_r_2, corr_out_0, corr_out_1, corr_out_2, corr_out_3, corr_out_4, corr_out_5);

   parameter sh_reg_w = 4'b1000; 

   input tm3_clk_v0; 
   input wen; 
   input[7:0] d_l_1; 
   input[7:0] d_l_2; 
   input[7:0] d_r_1; 
   input[7:0] d_r_2; 
   output[15:0] corr_out_0; 
   reg[15:0] corr_out_0;
   output[15:0] corr_out_1; 
   reg[15:0] corr_out_1;
   output[15:0] corr_out_2; 
   reg[15:0] corr_out_2;
   output[15:0] corr_out_3; 
   reg[15:0] corr_out_3;
   output[15:0] corr_out_4; 
   reg[15:0] corr_out_4;
   output[15:0] corr_out_5; 
   reg[15:0] corr_out_5;

   wire[sh_reg_w - 1:0] out_r1; 
   wire[sh_reg_w - 1:0] out_01; 
   wire[sh_reg_w - 1:0] out_11; 
   wire[sh_reg_w - 1:0] out_21; 
   wire[sh_reg_w - 1:0] out_31; 
   wire[sh_reg_w - 1:0] out_41; 
   wire[sh_reg_w - 1:0] out_51; 
   wire[sh_reg_w - 1:0] out_r2; 
   wire[sh_reg_w - 1:0] out_02; 
   wire[sh_reg_w - 1:0] out_12; 
   wire[sh_reg_w - 1:0] out_22; 
   wire[sh_reg_w - 1:0] out_32; 
   wire[sh_reg_w - 1:0] out_42; 
   wire[sh_reg_w - 1:0] out_52; 
   wire[2 * sh_reg_w - 1:0] corr_out_0_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_1_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_2_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_3_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_4_tmp; 
   wire[2 * sh_reg_w - 1:0] corr_out_5_tmp; 

   sh_reg  inst_sh_reg_r_1(tm3_clk_v0, wen, d_r_1, d_r_2, out_r1, out_r2); 
   sh_reg  inst_sh_reg_0(tm3_clk_v0, wen, d_l_1, d_l_2, out_01, out_02); 
   sh_reg  inst_sh_reg_1(tm3_clk_v0, wen, out_01, out_02, out_11, out_12); 
   sh_reg  inst_sh_reg_2(tm3_clk_v0, wen, out_11, out_12, out_21, out_22); 
   sh_reg  inst_sh_reg_3(tm3_clk_v0, wen, out_21, out_22, out_31, out_32); 
   sh_reg  inst_sh_reg_4(tm3_clk_v0, wen, out_31, out_32, out_41, out_42); 
   sh_reg  inst_sh_reg_5(tm3_clk_v0, wen, out_41, out_42, out_51, out_52); 
   corr_seq  inst_corr_0(tm3_clk_v0, wen, out_01, out_02, out_r1, out_r2, corr_out_0_tmp); 
   corr_seq  inst_corr_1(tm3_clk_v0, wen, out_11, out_12, out_r1, out_r2, corr_out_1_tmp); 
   corr_seq  inst_corr_2(tm3_clk_v0, wen, out_21, out_22, out_r1, out_r2, corr_out_2_tmp); 
   corr_seq  inst_corr_3(tm3_clk_v0, wen, out_31, out_32, out_r1, out_r2, corr_out_3_tmp); 
   corr_seq  inst_corr_4(tm3_clk_v0, wen, out_41, out_42, out_r1, out_r2, corr_out_4_tmp); 
   corr_seq  inst_corr_5(tm3_clk_v0, wen, out_51, out_52, out_r1, out_r2, corr_out_5_tmp); 

   always @(posedge tm3_clk_v0)
   begin
      if (wen == 1'b1)
         begin
            corr_out_0 <= corr_out_0_tmp ; 
            corr_out_1 <= corr_out_1_tmp ; 
            corr_out_2 <= corr_out_2_tmp ; 
            corr_out_3 <= corr_out_3_tmp ; 
            corr_out_4 <= corr_out_4_tmp ; 
            corr_out_5 <= corr_out_5_tmp ; 
         end 
         else
         begin
            corr_out_0 <= corr_out_0; 
            corr_out_1 <= corr_out_1; 
            corr_out_2 <= corr_out_2; 
            corr_out_3 <= corr_out_3; 
            corr_out_4 <= corr_out_4; 
            corr_out_5 <= corr_out_5; 
         end 
   end 
endmodule
module wrapper_norm_seq (clk, nd, din_1, din_2, dout_1, dout_2);

   parameter sh_reg_w  = 4'b1000;
   input clk; 
   input nd; 
   input[15:0] din_1; 
   input[15:0] din_2; 
   output[sh_reg_w - 1:0] dout_1; 
   wire[sh_reg_w - 1:0] dout_1;
   output[sh_reg_w - 1:0] dout_2; 
   wire[sh_reg_w - 1:0] dout_2;

   reg[15:0] din_1_reg; 
   reg[15:0] din_2_reg; 
   reg[15:0] din_1_tmp1; 
   reg[15:0] din_2_tmp1; 
   reg[15:0] din_1_tmp2; 
   reg[15:0] din_2_tmp2; 
   reg[15:0] addin_1; 
   reg[15:0] addin_2; 
   reg[16:0] add_out; 

   my_wrapper_divider my_div_inst_1 (nd, clk, din_1_tmp2, add_out, dout_1); 
   my_wrapper_divider my_div_inst_2 (nd, clk, din_2_tmp2, add_out, dout_2); 

   always @(posedge clk)
   begin
         if (nd == 1'b1)
         begin
            din_1_reg <= din_1 ; 
            din_2_reg <= din_2 ; 
         end 
         else
         begin
            din_1_reg <= din_1_reg ; 
            din_2_reg <= din_2_reg ; 
         end 
         din_1_tmp1 <= din_1_reg ; 
         din_1_tmp2 <= din_1_tmp1 ; 
         din_2_tmp1 <= din_2_reg ; 
         din_2_tmp2 <= din_2_tmp1 ; 
         if ((din_1_reg[15]) == 1'b0)
         begin
            addin_1 <= din_1_reg ; 
         end
         else
         begin
            addin_1 <= 16'b0000000000000000 - din_1_reg ; 
         end 
         if ((din_2_reg[15]) == 1'b0)
         begin
            addin_2 <= din_2_reg + 16'b0000000000000001 ; 
         end
         else
         begin
            addin_2 <= 16'b0000000000000001 - din_2_reg ; 
         end 
         add_out <= ({addin_1[15], addin_1}) + ({addin_2[15], addin_2}) ; 
   end 
endmodule
module corr_seq (clk, new_data, in_l_re, in_l_im, in_r_re, in_r_im, corr_out);

    parameter sh_reg_w  = 4'b1000;
    input clk; 
    input new_data; 
    input[sh_reg_w - 1:0] in_l_re; 
    input[sh_reg_w - 1:0] in_l_im; 
    input[sh_reg_w - 1:0] in_r_re; 
    input[sh_reg_w - 1:0] in_r_im; 
    output[2 * sh_reg_w - 1:0] corr_out; 
    reg[2 * sh_reg_w - 1:0] corr_out;
    reg[sh_reg_w - 1:0] in_l_re_reg; 
    reg[sh_reg_w - 1:0] in_l_im_reg; 
    reg[sh_reg_w - 1:0] in_r_re_reg; 
    reg[sh_reg_w - 1:0] in_r_im_reg; 
    reg[2 * sh_reg_w - 1:0] lrexrre_reg; 
    reg[2 * sh_reg_w - 1:0] limxrim_reg; 
    reg[2 * sh_reg_w - 1:0] corr_out_tmp; 

    always @(posedge clk)
    begin
          if (new_data == 1'b1)
          begin
             in_l_re_reg <= in_l_re ; 
             in_l_im_reg <= in_l_im ; 
             in_r_re_reg <= in_r_re ; 
             in_r_im_reg <= in_r_im ; 
             corr_out <= corr_out_tmp ; 
          end 
          else
          begin
             in_l_re_reg <= in_l_re_reg ; 
             in_l_im_reg <= in_l_im_reg ; 
             in_r_re_reg <= in_r_re_reg ; 
             in_r_im_reg <= in_r_im_reg ; 
             corr_out <= corr_out; 
          end 
			// PAJ - replaced by me, but called mult_slow
             lrexrre_reg <= in_l_re_reg*in_r_re_reg ; 
             limxrim_reg <= in_l_im_reg*in_r_im_reg ; 
          corr_out_tmp <= lrexrre_reg + limxrim_reg ; 
    end 
 endmodule
module port_bus_1to0 (clk, vidin_addr_reg, svid_comp_switch, vidin_new_data_scld_1_2to3_left, v_corr_05_00, v_corr_05_01, v_corr_05_02, v_corr_05_03, v_corr_05_04, v_corr_05_05, v_corr_10_00, v_corr_10_01, v_corr_10_02, v_corr_10_03, v_corr_10_04, v_corr_10_05, v_corr_10_06, v_corr_10_07, v_corr_10_08, v_corr_10_09, v_corr_10_10, v_corr_20_00, v_corr_20_01, v_corr_20_02, v_corr_20_03, v_corr_20_04, v_corr_20_05, v_corr_20_06, v_corr_20_07, v_corr_20_08, v_corr_20_09, v_corr_20_10, v_corr_20_11, v_corr_20_12, v_corr_20_13, v_corr_20_14, v_corr_20_15, v_corr_20_16, v_corr_20_17, v_corr_20_18, v_corr_20_19, v_corr_20_20, bus_word_1, bus_word_2, bus_word_3, bus_word_4, bus_word_5, bus_word_6, counter_out);

   parameter corr_res_w  = 4'b1000;
   input clk; 
   input[18:0] vidin_addr_reg; 
   input svid_comp_switch; 
   input vidin_new_data_scld_1_2to3_left; 
   input[corr_res_w - 1:0] v_corr_05_00; 
   input[corr_res_w - 1:0] v_corr_05_01; 
   input[corr_res_w - 1:0] v_corr_05_02; 
   input[corr_res_w - 1:0] v_corr_05_03; 
   input[corr_res_w - 1:0] v_corr_05_04; 
   input[corr_res_w - 1:0] v_corr_05_05; 
   input[corr_res_w - 1:0] v_corr_10_00; 
   input[corr_res_w - 1:0] v_corr_10_01; 
   input[corr_res_w - 1:0] v_corr_10_02; 
   input[corr_res_w - 1:0] v_corr_10_03; 
   input[corr_res_w - 1:0] v_corr_10_04; 
   input[corr_res_w - 1:0] v_corr_10_05; 
   input[corr_res_w - 1:0] v_corr_10_06; 
   input[corr_res_w - 1:0] v_corr_10_07; 
   input[corr_res_w - 1:0] v_corr_10_08; 
   input[corr_res_w - 1:0] v_corr_10_09; 
   input[corr_res_w - 1:0] v_corr_10_10; 
   input[corr_res_w - 1:0] v_corr_20_00; 
   input[corr_res_w - 1:0] v_corr_20_01; 
   input[corr_res_w - 1:0] v_corr_20_02; 
   input[corr_res_w - 1:0] v_corr_20_03; 
   input[corr_res_w - 1:0] v_corr_20_04; 
   input[corr_res_w - 1:0] v_corr_20_05; 
   input[corr_res_w - 1:0] v_corr_20_06; 
   input[corr_res_w - 1:0] v_corr_20_07; 
   input[corr_res_w - 1:0] v_corr_20_08; 
   input[corr_res_w - 1:0] v_corr_20_09; 
   input[corr_res_w - 1:0] v_corr_20_10; 
   input[corr_res_w - 1:0] v_corr_20_11; 
   input[corr_res_w - 1:0] v_corr_20_12; 
   input[corr_res_w - 1:0] v_corr_20_13; 
   input[corr_res_w - 1:0] v_corr_20_14; 
   input[corr_res_w - 1:0] v_corr_20_15; 
   input[corr_res_w - 1:0] v_corr_20_16; 
   input[corr_res_w - 1:0] v_corr_20_17; 
   input[corr_res_w - 1:0] v_corr_20_18; 
   input[corr_res_w - 1:0] v_corr_20_19; 
   input[corr_res_w - 1:0] v_corr_20_20; 
   output[7:0] bus_word_1; 
   reg[7:0] bus_word_1;
   output[7:0] bus_word_2; 
   reg[7:0] bus_word_2;
   output[7:0] bus_word_3; 
   reg[7:0] bus_word_3;
   output[7:0] bus_word_4; 
   reg[7:0] bus_word_4;
   output[7:0] bus_word_5; 
   reg[7:0] bus_word_5;
   output[7:0] bus_word_6; 
   reg[7:0] bus_word_6;
   output[2:0] counter_out; 
   reg[2:0] counter_out;

   reg[7:0] bus_word_1_tmp; 
   reg[7:0] bus_word_2_tmp; 
   reg[7:0] bus_word_3_tmp; 
   reg[7:0] bus_word_4_tmp; 
   reg[7:0] bus_word_5_tmp; 
   reg[7:0] bus_word_6_tmp; 
   reg[18:0] vidin_addr_reg_tmp; 
   reg svid_comp_switch_tmp; 
   wire vidin_new_data_scld_1_2to3_left_tmp; 
   reg[3:0] counter; 
   reg[2:0] counter_out_tmp; 
   reg[corr_res_w - 1:0] v_corr_05_00_tmp; 
   reg[corr_res_w - 1:0] v_corr_05_01_tmp; 
   reg[corr_res_w - 1:0] v_corr_05_02_tmp; 
   reg[corr_res_w - 1:0] v_corr_05_03_tmp; 
   reg[corr_res_w - 1:0] v_corr_05_04_tmp; 
   reg[corr_res_w - 1:0] v_corr_05_05_tmp; 
   reg[corr_res_w - 1:0] v_corr_10_00_tmp; 
   reg[corr_res_w - 1:0] v_corr_10_01_tmp; 
   reg[corr_res_w - 1:0] v_corr_10_02_tmp; 
   reg[corr_res_w - 1:0] v_corr_10_03_tmp; 
   reg[corr_res_w - 1:0] v_corr_10_04_tmp; 
   reg[corr_res_w - 1:0] v_corr_10_05_tmp; 
   reg[corr_res_w - 1:0] v_corr_10_06_tmp; 
   reg[corr_res_w - 1:0] v_corr_10_07_tmp; 
   reg[corr_res_w - 1:0] v_corr_10_08_tmp; 
   reg[corr_res_w - 1:0] v_corr_10_09_tmp; 
   reg[corr_res_w - 1:0] v_corr_10_10_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_00_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_01_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_02_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_03_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_04_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_05_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_06_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_07_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_08_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_09_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_10_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_11_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_12_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_13_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_14_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_15_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_16_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_17_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_18_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_19_tmp; 
   reg[corr_res_w - 1:0] v_corr_20_20_tmp; 

   always @(posedge clk)
   begin
         if (vidin_new_data_scld_1_2to3_left == 1'b1)
         begin
            counter <= 4'b0001 ; 
         end
         else
         begin
            if (counter == 4'b1000)
            begin
               counter <= 4'b1000 ; 
            end
            else
            begin
               counter <= counter + 1 ; 
            end 
         end 
         case (counter[2:0])
            3'b000 :
                     begin
                        counter_out_tmp <= 3'b000 ; 
                        bus_word_1_tmp <= 8'b00000000 ; 
                        bus_word_2_tmp <= 8'b00000000 ; 
                        bus_word_3_tmp <= 8'b00000000 ; 
                        bus_word_4_tmp <= 8'b00000000 ; 
                        bus_word_5_tmp <= 8'b00000000 ; 
                        bus_word_6_tmp <= 8'b00000000 ; 
                     end
            3'b001 :
                     begin
                        counter_out_tmp <= 3'b001 ; 
                        bus_word_1_tmp <= vidin_addr_reg_tmp[7:0] ; 
                        bus_word_2_tmp <= vidin_addr_reg_tmp[15:8] ; 
                        bus_word_3_tmp <= {vidin_addr_reg_tmp[18:16], svid_comp_switch_tmp, 4'b0000} ; 
                        bus_word_4_tmp <= v_corr_05_00_tmp ; 
                        bus_word_5_tmp <= v_corr_05_01_tmp ; 
                        bus_word_6_tmp <= v_corr_05_02_tmp ; 
                     end
            3'b010 :
                     begin
                        counter_out_tmp <= 3'b010 ; 
                        bus_word_1_tmp <= v_corr_05_03_tmp ; 
                        bus_word_2_tmp <= v_corr_05_04_tmp ; 
                        bus_word_3_tmp <= v_corr_05_05_tmp ; 
                        bus_word_4_tmp <= v_corr_10_00_tmp ; 
                        bus_word_5_tmp <= v_corr_10_01_tmp ; 
                        bus_word_6_tmp <= v_corr_10_02_tmp ; 
                     end
            3'b011 :
                     begin
                        counter_out_tmp <= 3'b011 ; 
                        bus_word_1_tmp <= v_corr_10_03_tmp ; 
                        bus_word_2_tmp <= v_corr_10_04_tmp ; 
                        bus_word_3_tmp <= v_corr_10_05_tmp ; 
                        bus_word_4_tmp <= v_corr_10_06_tmp ; 
                        bus_word_5_tmp <= v_corr_10_07_tmp ; 
                        bus_word_6_tmp <= v_corr_10_08_tmp ; 
                     end
            3'b100 :
                     begin
                        counter_out_tmp <= 3'b100 ; 
                        bus_word_1_tmp <= v_corr_10_09_tmp ; 
                        bus_word_2_tmp <= v_corr_10_10_tmp ; 
                        bus_word_3_tmp <= v_corr_20_00_tmp ; 
                        bus_word_4_tmp <= v_corr_20_01_tmp ; 
                        bus_word_5_tmp <= v_corr_20_02_tmp ; 
                        bus_word_6_tmp <= v_corr_20_03_tmp ; 
                     end
            3'b101 :

                     begin
                        counter_out_tmp <= 3'b101 ; 
                        bus_word_1_tmp <= v_corr_20_04_tmp ; 
                        bus_word_2_tmp <= v_corr_20_05_tmp ; 
                        bus_word_3_tmp <= v_corr_20_06_tmp ; 
                        bus_word_4_tmp <= v_corr_20_07_tmp ; 
                        bus_word_5_tmp <= v_corr_20_08_tmp ; 
                        bus_word_6_tmp <= v_corr_20_09_tmp ; 
                     end
            3'b110 :
                     begin
                        counter_out_tmp <= 3'b110 ; 
                        bus_word_1_tmp <= v_corr_20_10_tmp ; 
                        bus_word_2_tmp <= v_corr_20_11_tmp ; 
                        bus_word_3_tmp <= v_corr_20_12_tmp ; 
                        bus_word_4_tmp <= v_corr_20_13_tmp ; 
                        bus_word_5_tmp <= v_corr_20_14_tmp ; 
                        bus_word_6_tmp <= v_corr_20_15_tmp ; 
                     end
            3'b111 :
                     begin
                        counter_out_tmp <= 3'b111 ; 
                        bus_word_1_tmp <= v_corr_20_16_tmp ; 
                        bus_word_2_tmp <= v_corr_20_17_tmp ; 
                        bus_word_3_tmp <= v_corr_20_18_tmp ; 
                        bus_word_4_tmp <= v_corr_20_19_tmp ; 
                        bus_word_5_tmp <= v_corr_20_20_tmp ; 
                        bus_word_6_tmp <= 8'b00000000 ; 
                     end
            default :
                     begin
                        counter_out_tmp <= 3'b111 ; 
                        bus_word_1_tmp <= v_corr_20_16_tmp ; 
                        bus_word_2_tmp <= v_corr_20_17_tmp ; 
                        bus_word_3_tmp <= v_corr_20_18_tmp ; 
                        bus_word_4_tmp <= v_corr_20_19_tmp ; 
                        bus_word_5_tmp <= v_corr_20_20_tmp ; 
                        bus_word_6_tmp <= 8'b00000000 ; 
                     end
         endcase 
   end 

   always @(posedge clk)
   begin
         counter_out <= counter_out_tmp ; 
         bus_word_1 <= bus_word_1_tmp ; 
         bus_word_2 <= bus_word_2_tmp ; 
         bus_word_3 <= bus_word_3_tmp ; 
         bus_word_4 <= bus_word_4_tmp ; 
         bus_word_5 <= bus_word_5_tmp ; 
         bus_word_6 <= bus_word_6_tmp ; 
         if (vidin_new_data_scld_1_2to3_left == 1'b1)
         begin
            vidin_addr_reg_tmp <= vidin_addr_reg ; 
            svid_comp_switch_tmp <= svid_comp_switch ; 
            v_corr_05_00_tmp <= v_corr_05_00 ; 
            v_corr_05_01_tmp <= v_corr_05_01 ; 
            v_corr_05_02_tmp <= v_corr_05_02 ; 
            v_corr_05_03_tmp <= v_corr_05_03 ; 
            v_corr_05_04_tmp <= v_corr_05_04 ; 
            v_corr_05_05_tmp <= v_corr_05_05 ; 
            v_corr_10_00_tmp <= v_corr_10_00 ; 
            v_corr_10_01_tmp <= v_corr_10_01 ; 
            v_corr_10_02_tmp <= v_corr_10_02 ; 
           v_corr_10_03_tmp <= v_corr_10_03 ; 
            v_corr_10_04_tmp <= v_corr_10_04 ; 
            v_corr_10_05_tmp <= v_corr_10_05 ; 
            v_corr_10_06_tmp <= v_corr_10_06 ; 
            v_corr_10_07_tmp <= v_corr_10_07 ; 
            v_corr_10_08_tmp <= v_corr_10_08 ; 
            v_corr_10_09_tmp <= v_corr_10_09 ; 
            v_corr_10_10_tmp <= v_corr_10_10 ; 
            v_corr_20_00_tmp <= v_corr_20_00 ; 
            v_corr_20_01_tmp <= v_corr_20_01 ; 
            v_corr_20_02_tmp <= v_corr_20_02 ; 
            v_corr_20_03_tmp <= v_corr_20_03 ; 
            v_corr_20_04_tmp <= v_corr_20_04 ; 
            v_corr_20_05_tmp <= v_corr_20_05 ; 
            v_corr_20_06_tmp <= v_corr_20_06 ; 
            v_corr_20_07_tmp <= v_corr_20_07 ; 
            v_corr_20_08_tmp <= v_corr_20_08 ; 
            v_corr_20_09_tmp <= v_corr_20_09 ; 
            v_corr_20_10_tmp <= v_corr_20_10 ; 
            v_corr_20_11_tmp <= v_corr_20_11 ; 
            v_corr_20_12_tmp <= v_corr_20_12 ; 
            v_corr_20_13_tmp <= v_corr_20_13 ; 
            v_corr_20_14_tmp <= v_corr_20_14 ; 
            v_corr_20_15_tmp <= v_corr_20_15 ; 
            v_corr_20_16_tmp <= v_corr_20_16 ; 
            v_corr_20_17_tmp <= v_corr_20_17 ; 
            v_corr_20_18_tmp <= v_corr_20_18 ; 
            v_corr_20_19_tmp <= v_corr_20_19 ; 
            v_corr_20_20_tmp <= v_corr_20_20 ; 
         end 
         else
         begin
            vidin_addr_reg_tmp <= vidin_addr_reg_tmp ; 
            svid_comp_switch_tmp <= svid_comp_switch_tmp ; 
            v_corr_05_00_tmp <= v_corr_05_00_tmp ; 
            v_corr_05_01_tmp <= v_corr_05_01_tmp ; 
            v_corr_05_02_tmp <= v_corr_05_02_tmp ; 
            v_corr_05_03_tmp <= v_corr_05_03_tmp ; 
            v_corr_05_04_tmp <= v_corr_05_04_tmp ; 
            v_corr_05_05_tmp <= v_corr_05_05_tmp ; 
            v_corr_10_00_tmp <= v_corr_10_00_tmp ; 
            v_corr_10_01_tmp <= v_corr_10_01_tmp ; 
            v_corr_10_02_tmp <= v_corr_10_02_tmp ; 
           v_corr_10_03_tmp <= v_corr_10_03_tmp ; 
            v_corr_10_04_tmp <= v_corr_10_04_tmp ; 
            v_corr_10_05_tmp <= v_corr_10_05_tmp ; 
            v_corr_10_06_tmp <= v_corr_10_06_tmp ; 
            v_corr_10_07_tmp <= v_corr_10_07_tmp ; 
            v_corr_10_08_tmp <= v_corr_10_08_tmp ; 
            v_corr_10_09_tmp <= v_corr_10_09_tmp ; 
            v_corr_10_10_tmp <= v_corr_10_10_tmp ; 
            v_corr_20_00_tmp <= v_corr_20_00_tmp ; 
            v_corr_20_01_tmp <= v_corr_20_01_tmp ; 
            v_corr_20_02_tmp <= v_corr_20_02_tmp ; 
            v_corr_20_03_tmp <= v_corr_20_03_tmp ; 
            v_corr_20_04_tmp <= v_corr_20_04_tmp ; 
            v_corr_20_05_tmp <= v_corr_20_05_tmp ; 
            v_corr_20_06_tmp <= v_corr_20_06_tmp ; 
            v_corr_20_07_tmp <= v_corr_20_07_tmp ; 
            v_corr_20_08_tmp <= v_corr_20_08_tmp ; 
            v_corr_20_09_tmp <= v_corr_20_09_tmp ; 
            v_corr_20_10_tmp <= v_corr_20_10_tmp ; 
            v_corr_20_11_tmp <= v_corr_20_11_tmp ; 
            v_corr_20_12_tmp <= v_corr_20_12_tmp ; 
            v_corr_20_13_tmp <= v_corr_20_13_tmp ; 
            v_corr_20_14_tmp <= v_corr_20_14_tmp ; 
            v_corr_20_15_tmp <= v_corr_20_15_tmp ; 
            v_corr_20_16_tmp <= v_corr_20_16_tmp ; 
            v_corr_20_17_tmp <= v_corr_20_17_tmp ; 
            v_corr_20_18_tmp <= v_corr_20_18_tmp ; 
            v_corr_20_19_tmp <= v_corr_20_19_tmp ; 
            v_corr_20_20_tmp <= v_corr_20_20_tmp ; 
         end 
   end 
endmodule
module my_wrapper_divider(rst, clk, data_in_a, data_in_b, data_out);
	parameter INPUT_WIDTH_A = 5'b10000;
	parameter INPUT_WIDTH_B = 5'b10001;
	parameter OUTPUT_WIDTH = 4'b1000;

	parameter S1 = 2'b00;
	parameter S2 = 2'b01;
	parameter S3 = 2'b10;
	parameter S4 = 2'b11;

	input rst;
	input clk;
	input [INPUT_WIDTH_A-1:0]data_in_a;
	input [INPUT_WIDTH_B-1:0]data_in_b;
	output [OUTPUT_WIDTH-1:0]data_out;
	wire [OUTPUT_WIDTH-1:0]data_out;

	wire [OUTPUT_WIDTH-1:0]Remainder;

	reg start, LA, EB;
	wire Done;
	reg[1:0] y, Y;

	my_divider my_divider_inst(clk, rst, start, LA, EB, data_in_a, data_in_b, Remainder, data_out, Done);

	always @(posedge clk or negedge rst)
	begin
		if (rst == 0)
			y <= S1;
		else
			y <= Y;
	end

	always @(y)
	begin
		case (y)
			S1 :
			begin	
				LA = 0;
				EB = 0;
				start = 0;
				Y = S2;
			end
			S2 : 
			begin
				LA = 1;
				EB = 1;
				start = 0;
				Y = S3;
			end
			S3 : 
			begin
				LA = 0;
				EB = 0;
				start = 1;
				Y = S4;
			end
			S4 : 
			begin
				LA = 0;
				EB = 0;
				start = 0;
				if (Done == 1'b1)
				begin
					Y = S1;
				end
				else
				begin
					Y = S4;
				end
			end
		endcase
	end
endmodule

module my_divider(clk, rst, start, LA, EB, data_in_a, data_in_b, Remainder, data_out, Done);

	parameter INPUT_WIDTH_A = 5'b10000;
	parameter INPUT_WIDTH_B = 5'b10001;
	parameter OUTPUT_WIDTH = 4'b1000;
	parameter LOGN = 3'b100;

	parameter S1 = 2'b00;
	parameter S2 = 2'b01;
	parameter S3 = 2'b10;

	input clk;
	input [INPUT_WIDTH_A-1:0]data_in_a;
	input [INPUT_WIDTH_B-1:0]data_in_b;
	input rst;
	input start;
	input LA;
	input EB;
	output [OUTPUT_WIDTH-1:0]data_out;
	wire [OUTPUT_WIDTH-1:0]data_out;
	output [OUTPUT_WIDTH-1:0]Remainder;
	reg [OUTPUT_WIDTH-1:0]Remainder;
	output Done;
	reg Done;

	wire Cout, zero;
	wire [INPUT_WIDTH_A-1:0] Sum;
	reg [1:0] y, Y;
	reg [LOGN-1:0] Count;
	reg EA, Rsel, LR, ER, ER0, LC, EC;
	reg [INPUT_WIDTH_B-1:0] RegB;
	reg [INPUT_WIDTH_A-1:0] DataA;
	reg ff0;

	always @(start or y or zero)
	begin
		case(y)
			S1:
			begin
				if (start == 0)
					Y = S1;
				else
					Y = S2;
			end
			S2:
			begin
				if (zero == 0)
					Y = S2;
				else
					Y = S3;
			end
			S3:
			begin
				if (start == 1)
					Y = S3;
				else
					Y = S1;
			end
			default:
			begin
				Y = 2'b00;
			end
		endcase
	end

	always @(posedge clk or negedge rst)
	begin
		if (rst == 0)
			y <= S1;
		else
			y <= Y;
	end

	always @(y or start or Cout or zero)
	begin
		case (y)
			S1:
			begin
				LC = 1;
				ER = 1;
				EC = 0;
				Rsel = 0;
				Done = 0;
				if (start == 0)
				begin
					LR = 1;
					ER0 = 1;
					EA = 0;
				end	
				else
				begin
					LR = 0; 
					EA = 1;
					ER0 = 1;
				end
			end
			S2:
			begin
				LC = 0;
				ER = 1;
				Rsel = 1;	
				Done = 0;
				ER0 = 1;
				EA = 1;
				if (Cout)
					LR = 1;
				else
					LR = 0;
				if (zero == 0)
					EC = 1;
				else
					EC = 0;
			end
			S3:
			begin
				Done = 1;
				LR = 0;
				LC = 0;
				ER = 0;
				EC = 0;
				Rsel = 0;
				ER0 = 0;
				EA = 0;
			end
			default:
			begin
				Done = 0;
				LR = 0;
				LC = 0;
				ER = 0;
				EC = 0;
				Rsel = 0;
				ER0 = 0;
				EA = 0;
			end
		endcase
	end	

	always @(posedge clk)
	begin
		if (rst == 1)
		begin
			RegB <= 0;
			Remainder <= 0;
			DataA <= 0;
			ff0 <= 0;
			Count <= 0;
		end	
		else
		begin
			if (EB == 1)
			begin
				RegB <= data_in_b;
			end
			else
			begin
				RegB <= RegB;
			end

			if (LR == 1)
			begin
				Remainder <= Rsel ? Sum : 0;
			end
			else if (ER == 1)
			begin
				Remainder <= (Remainder << 1) | ff0;
			end
			else
			begin
				Remainder <= Remainder;
			end

			if (LA == 1)
			begin
				DataA <= data_in_a;
			end
			else if (EA == 1)
			begin
				DataA <= (DataA << 1) | Cout;
			end
			else
			begin
				DataA <= DataA;
			end

			if (ER0 == 1)
			begin
				ff0 <=  DataA[INPUT_WIDTH_A-1];
			end
			else
			begin
				ff0 <= 0;
			end

			if (LC == 1)
			begin
				Count <= 0;
			end
			else if (EC == 1)
			begin
				Count <= Count + 1;
			end
			else
			begin
				Count <= Count;
			end
		end
	end	

	assign zero = (Count == 0);
	assign Sum = {Remainder, ff0} + (~RegB + 1);
	assign Cout = Sum[INPUT_WIDTH_A-1:0];
	assign data_out = DataA; 

endmodule
