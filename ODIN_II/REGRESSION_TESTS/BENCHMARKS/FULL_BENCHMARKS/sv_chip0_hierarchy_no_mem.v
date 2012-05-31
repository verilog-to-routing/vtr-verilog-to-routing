// Ahmad Darabiha
// last updated Aug, 2002
// this is the design for chip#0 of 
// the stereo vision system.
// this is the last stage which interpolates the results of correlation 
// and after LPF it takes the Max and send the final results to chip #3 to
// be displayed on the monitor.
module sv_chip0_hierarchy_no_mem (tm3_clk_v0, tm3_sram_data_in, tm3_sram_data_out, tm3_sram_addr, tm3_sram_we, tm3_sram_oe, tm3_sram_adsp, bus_word_1_1to0, bus_word_2_1to0, bus_word_3_1to0, bus_word_4_1to0, bus_word_5_1to0, bus_word_6_1to0, counter_out_1to0, vidin_new_data_fifo, vidin_rgb_reg_fifo_left, vidin_rgb_reg_fifo_right, vidin_addr_reg_2to0, v_nd_s1_left_2to0, v_nd_s2_left_2to0 , v_nd_s4_left_2to0 , v_d_reg_s1_left_2to0 , v_d_reg_s2_left_2to0 , v_d_reg_s4_left_2to0 , v_nd_s1_right_2to0, v_nd_s2_right_2to0 , v_nd_s4_right_2to0 , v_d_reg_s1_right_2to0 , v_d_reg_s2_right_2to0 , v_d_reg_s4_right_2to0 , tm3_vidout_red,tm3_vidout_green,tm3_vidout_blue , tm3_vidout_clock,tm3_vidout_hsync,tm3_vidout_vsync,tm3_vidout_blank , x_in, y_in, depth_out);

   input tm3_clk_v0; 
   input[63:0] tm3_sram_data_in; 
   wire[63:0] tm3_sram_data_in;
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
   input[7:0] bus_word_1_1to0; 
   input[7:0] bus_word_2_1to0; 
   input[7:0] bus_word_3_1to0; 
   input[7:0] bus_word_4_1to0; 
   input[7:0] bus_word_5_1to0; 
   input[7:0] bus_word_6_1to0; 
   input[2:0] counter_out_1to0; 
   input vidin_new_data_fifo; 
   input[7:0] vidin_rgb_reg_fifo_left; 
   input[7:0] vidin_rgb_reg_fifo_right; 
   input[3:0] vidin_addr_reg_2to0; 
   output v_nd_s1_left_2to0; 
   reg v_nd_s1_left_2to0;
   output v_nd_s2_left_2to0; 
   reg v_nd_s2_left_2to0;
   output v_nd_s4_left_2to0; 
   reg v_nd_s4_left_2to0;
   output[7:0] v_d_reg_s1_left_2to0; 
   reg[7:0] v_d_reg_s1_left_2to0;
   output[7:0] v_d_reg_s2_left_2to0; 
   reg[7:0] v_d_reg_s2_left_2to0;
   output[7:0] v_d_reg_s4_left_2to0; 
   reg[7:0] v_d_reg_s4_left_2to0;
   output v_nd_s1_right_2to0; 
   reg v_nd_s1_right_2to0;
   output v_nd_s2_right_2to0; 
   reg v_nd_s2_right_2to0;
   output v_nd_s4_right_2to0; 
   reg v_nd_s4_right_2to0;
   output[7:0] v_d_reg_s1_right_2to0; 
   reg[7:0] v_d_reg_s1_right_2to0;
   output[7:0] v_d_reg_s2_right_2to0; 
   reg[7:0] v_d_reg_s2_right_2to0;
   output[7:0] v_d_reg_s4_right_2to0; 
   reg[7:0] v_d_reg_s4_right_2to0;
   output[9:0] tm3_vidout_red; 
   reg[9:0] tm3_vidout_red;
   output[9:0] tm3_vidout_green; 
   reg[9:0] tm3_vidout_green;
   output[9:0] tm3_vidout_blue; 
   reg[9:0] tm3_vidout_blue;
   output tm3_vidout_clock; 
   wire tm3_vidout_clock;
   output tm3_vidout_hsync; 
   reg tm3_vidout_hsync;
   output tm3_vidout_vsync; 
   reg tm3_vidout_vsync;
   output tm3_vidout_blank; 
   reg tm3_vidout_blank;
   input[15:0] x_in; 
   input[15:0] y_in; 
   output[15:0] depth_out; 
   reg[15:0] depth_out;
   reg[15:0] x_reg_l; 
   reg[15:0] x_reg_r; 
   reg[15:0] y_reg_up; 
   reg[15:0] y_reg_dn; 
   reg[7:0] depth_out_reg; 
   reg[9:0] horiz; 
   reg[9:0] vert; 
   reg[63:0] vidin_data_buf_sc_1; 
   reg[55:0] vidin_data_buf_2_sc_1; 
   reg[18:0] vidin_addr_buf_sc_1; 
   reg[63:0] vidout_buf; 
   reg[63:0] vidin_data_buf_sc_2; 
   reg[55:0] vidin_data_buf_2_sc_2; 
   reg[18:0] vidin_addr_buf_sc_2; 
   reg[63:0] vidin_data_buf_sc_4; 
   reg[55:0] vidin_data_buf_2_sc_4; 
   reg[18:0] vidin_addr_buf_sc_4; 
   reg video_state; 
   reg vidin_new_data_scld_1_2to3_left_reg; 
   reg[7:0] vidin_data_reg_scld_1_2to3_left_reg; 
   reg[7:0] vidin_data_reg_scld_1_2to3_right_reg; 
   reg vidin_new_data_scld_2_2to3_left_reg; 
   reg[7:0] vidin_data_reg_scld_2_2to3_left_reg; 
   reg[7:0] vidin_data_reg_scld_2_2to3_right_reg; 
   reg vidin_new_data_scld_4_2to3_left_reg; 
   reg[7:0] vidin_data_reg_scld_4_2to3_left_reg; 
   reg[7:0] vidin_data_reg_scld_4_2to3_right_reg; 
   reg[18:0] vidin_addr_reg_2to3_reg; 
   wire[18:0] vidin_addr_reg; 
   wire svid_comp_switch_2to3; 
   wire vidin_new_data_scld_1_1to0; 
   wire vidin_new_data_scld_2_1to0; 
   wire vidin_new_data_scld_4_1to0; 
   wire[7:0] v_corr_200; 
   wire[7:0] v_corr_201; 
   wire[7:0] v_corr_202; 
   wire[7:0] v_corr_203; 
   wire[7:0] v_corr_204; 
   wire[7:0] v_corr_205; 
   wire[7:0] v_corr_206; 
   wire[7:0] v_corr_207; 
   wire[7:0] v_corr_208; 
   wire[7:0] v_corr_209; 
   wire[7:0] v_corr_2010; 
   wire[7:0] v_corr_2011; 
   wire[7:0] v_corr_2012; 
   wire[7:0] v_corr_2013; 
   wire[7:0] v_corr_2014; 
   wire[7:0] v_corr_2015; 
   wire[7:0] v_corr_2016; 
   wire[7:0] v_corr_2017; 
   wire[7:0] v_corr_2018; 
   wire[7:0] v_corr_2019; 
   wire[7:0] v_corr_2020; 
   wire[7:0] v_corr_100; 
   wire[7:0] v_corr_101; 
   wire[7:0] v_corr_102; 
   wire[7:0] v_corr_103; 
   wire[7:0] v_corr_104; 
   wire[7:0] v_corr_105; 
   wire[7:0] v_corr_106; 
   wire[7:0] v_corr_107; 
   wire[7:0] v_corr_108; 
   wire[7:0] v_corr_109; 
   wire[7:0] v_corr_1010; 
   wire[7:0] v_corr_50; 
   wire[7:0] v_corr_51; 
   wire[7:0] v_corr_52; 
   wire[7:0] v_corr_53; 
   wire[7:0] v_corr_54; 
   wire[7:0] v_corr_55; 
   wire[7:0] v_corr_20_fltr0; 
   wire[7:0] v_corr_20_fltr1; 
   wire[7:0] v_corr_20_fltr2; 
   wire[7:0] v_corr_20_fltr3; 
   wire[7:0] v_corr_20_fltr4; 
   wire[7:0] v_corr_20_fltr5; 
   wire[7:0] v_corr_20_fltr6; 
   wire[7:0] v_corr_20_fltr7; 
   wire[7:0] v_corr_20_fltr8; 
   wire[7:0] v_corr_20_fltr9; 
   wire[7:0] v_corr_20_fltr10; 
   wire[7:0] v_corr_20_fltr11; 
   wire[7:0] v_corr_20_fltr12; 
   wire[7:0] v_corr_20_fltr13; 
   wire[7:0] v_corr_20_fltr14; 
   wire[7:0] v_corr_20_fltr15; 
   wire[7:0] v_corr_20_fltr16; 
   wire[7:0] v_corr_20_fltr17; 
   wire[7:0] v_corr_20_fltr18; 
   wire[7:0] v_corr_20_fltr19; 
   wire[7:0] v_corr_20_fltr20; 
   wire[7:0] v_corr_10_fltr0; 
   wire[7:0] v_corr_10_fltr1; 
   wire[7:0] v_corr_10_fltr2; 
   wire[7:0] v_corr_10_fltr3; 
   wire[7:0] v_corr_10_fltr4; 
   wire[7:0] v_corr_10_fltr5; 
   wire[7:0] v_corr_10_fltr6; 
   wire[7:0] v_corr_10_fltr7; 
   wire[7:0] v_corr_10_fltr8; 
   wire[7:0] v_corr_10_fltr9; 
   wire[7:0] v_corr_10_fltr10; 
   wire[7:0] v_corr_5_fltr0; 
   wire[7:0] v_corr_5_fltr1; 
   wire[7:0] v_corr_5_fltr2; 
   wire[7:0] v_corr_5_fltr3; 
   wire[7:0] v_corr_5_fltr4; 
   wire[7:0] v_corr_5_fltr5; 
   wire[7:0] v_corr_20_fltr_x0; 
   wire[7:0] v_corr_20_fltr_x1; 
   wire[7:0] v_corr_20_fltr_x2; 
   wire[7:0] v_corr_20_fltr_x3; 
   wire[7:0] v_corr_20_fltr_x4; 
   wire[7:0] v_corr_20_fltr_x5; 
   wire[7:0] v_corr_20_fltr_x6; 
   wire[7:0] v_corr_20_fltr_x7; 
   wire[7:0] v_corr_20_fltr_x8; 
   wire[7:0] v_corr_20_fltr_x9; 
   wire[7:0] v_corr_20_fltr_x10; 
   wire[7:0] v_corr_20_fltr_x11; 
   wire[7:0] v_corr_20_fltr_x12; 
   wire[7:0] v_corr_20_fltr_x13; 
   wire[7:0] v_corr_20_fltr_x14; 
   wire[7:0] v_corr_20_fltr_x15; 
   wire[7:0] v_corr_20_fltr_x16; 
   wire[7:0] v_corr_20_fltr_x17; 
   wire[7:0] v_corr_20_fltr_x18; 
   wire[7:0] v_corr_20_fltr_x19; 
   wire[7:0] v_corr_20_fltr_x20; 
   wire[7:0] v_corr_10_fltr_x0; 
   wire[7:0] v_corr_10_fltr_x1; 
   wire[7:0] v_corr_10_fltr_x2; 
   wire[7:0] v_corr_10_fltr_x3; 
   wire[7:0] v_corr_10_fltr_x4; 
   wire[7:0] v_corr_10_fltr_x5; 
   wire[7:0] v_corr_10_fltr_x6; 
   wire[7:0] v_corr_10_fltr_x7; 
   wire[7:0] v_corr_10_fltr_x8; 
   wire[7:0] v_corr_10_fltr_x9; 
   wire[7:0] v_corr_10_fltr_x10; 
   wire[7:0] v_corr_5_fltr_x0; 
   wire[7:0] v_corr_5_fltr_x1; 
   wire[7:0] v_corr_5_fltr_x2; 
   wire[7:0] v_corr_5_fltr_x3; 
   wire[7:0] v_corr_5_fltr_x4; 
   wire[7:0] v_corr_5_fltr_x5; 
   wire[7:0] v_corr_20_fltr_h0; 
   wire[7:0] v_corr_20_fltr_h1; 
   wire[7:0] v_corr_20_fltr_h2; 
   wire[7:0] v_corr_20_fltr_h3; 
   wire[7:0] v_corr_20_fltr_h4; 
   wire[7:0] v_corr_20_fltr_h5; 
   wire[7:0] v_corr_20_fltr_h6; 
   wire[7:0] v_corr_20_fltr_h7; 
   wire[7:0] v_corr_20_fltr_h8; 
   wire[7:0] v_corr_20_fltr_h9; 
   wire[7:0] v_corr_20_fltr_h10; 
   wire[7:0] v_corr_20_fltr_h11; 
   wire[7:0] v_corr_20_fltr_h12; 
   wire[7:0] v_corr_20_fltr_h13; 
   wire[7:0] v_corr_20_fltr_h14; 
   wire[7:0] v_corr_20_fltr_h15; 
   wire[7:0] v_corr_20_fltr_h16; 
   wire[7:0] v_corr_20_fltr_h17; 
   wire[7:0] v_corr_20_fltr_h18; 
   wire[7:0] v_corr_20_fltr_h19; 
   wire[7:0] v_corr_20_fltr_h20; 
   wire[7:0] v_corr_10_fltr_h0; 
   wire[7:0] v_corr_10_fltr_h1; 
   wire[7:0] v_corr_10_fltr_h2; 
   wire[7:0] v_corr_10_fltr_h3; 
   wire[7:0] v_corr_10_fltr_h4; 
   wire[7:0] v_corr_10_fltr_h5; 
   wire[7:0] v_corr_10_fltr_h6; 
   wire[7:0] v_corr_10_fltr_h7; 
   wire[7:0] v_corr_10_fltr_h8; 
   wire[7:0] v_corr_10_fltr_h9; 
   wire[7:0] v_corr_10_fltr_h10; 
   wire[7:0] v_corr_5_fltr_h0; 
   wire[7:0] v_corr_5_fltr_h1; 
   wire[7:0] v_corr_5_fltr_h2; 
   wire[7:0] v_corr_5_fltr_h3; 
   wire[7:0] v_corr_5_fltr_h4; 
   wire[7:0] v_corr_5_fltr_h5; 
   wire[7:0] v_corr_20_fifo0; 
   wire[7:0] v_corr_20_fifo1; 
   wire[7:0] v_corr_20_fifo2; 
   wire[7:0] v_corr_20_fifo3; 
   wire[7:0] v_corr_20_fifo4; 
   wire[7:0] v_corr_20_fifo5; 
   wire[7:0] v_corr_20_fifo6; 
   wire[7:0] v_corr_20_fifo7; 
   wire[7:0] v_corr_20_fifo8; 
   wire[7:0] v_corr_20_fifo9; 
   wire[7:0] v_corr_20_fifo10; 
   wire[7:0] v_corr_20_fifo11; 
   wire[7:0] v_corr_20_fifo12; 
   wire[7:0] v_corr_20_fifo13; 
   wire[7:0] v_corr_20_fifo14; 
   wire[7:0] v_corr_20_fifo15; 
   wire[7:0] v_corr_20_fifo16; 
   wire[7:0] v_corr_20_fifo17; 
   wire[7:0] v_corr_20_fifo18; 
   wire[7:0] v_corr_20_fifo19; 
   wire[7:0] v_corr_20_fifo20; 
   wire[8:0] v_corr_10_fifo0; 
   wire[8:0] v_corr_10_fifo1; 
   wire[8:0] v_corr_10_fifo2; 
   wire[8:0] v_corr_10_fifo3; 
   wire[8:0] v_corr_10_fifo4; 
   wire[8:0] v_corr_10_fifo5; 
   wire[8:0] v_corr_10_fifo6; 
   wire[8:0] v_corr_10_fifo7; 
   wire[8:0] v_corr_10_fifo8; 
   wire[8:0] v_corr_10_fifo9; 
   wire[8:0] v_corr_10_fifo10; 
   wire[8:0] v_corr_10_fifo11; 
   wire[8:0] v_corr_10_fifo12; 
   wire[8:0] v_corr_10_fifo13; 
   wire[8:0] v_corr_10_fifo14; 
   wire[8:0] v_corr_10_fifo15; 
   wire[8:0] v_corr_10_fifo16; 
   wire[8:0] v_corr_10_fifo17; 
   wire[8:0] v_corr_10_fifo18; 
   wire[8:0] v_corr_10_fifo19; 
   wire[8:0] v_corr_10_fifo20; 
   wire[7:0] v_corr_20_fifo_x0; 
   wire[7:0] v_corr_20_fifo_x1; 
   wire[7:0] v_corr_20_fifo_x2; 
   wire[7:0] v_corr_20_fifo_x3; 
   wire[7:0] v_corr_20_fifo_x4; 
   wire[7:0] v_corr_20_fifo_x5; 
   wire[7:0] v_corr_20_fifo_x6; 
   wire[7:0] v_corr_20_fifo_x7; 
   wire[7:0] v_corr_20_fifo_x8; 
   wire[7:0] v_corr_20_fifo_x9; 
   wire[7:0] v_corr_20_fifo_x10; 
   wire[7:0] v_corr_20_fifo_x11; 
   wire[7:0] v_corr_20_fifo_x12; 
   wire[7:0] v_corr_20_fifo_x13; 
   wire[7:0] v_corr_20_fifo_x14; 
   wire[7:0] v_corr_20_fifo_x15; 
   wire[7:0] v_corr_20_fifo_x16; 
   wire[7:0] v_corr_20_fifo_x17; 
   wire[7:0] v_corr_20_fifo_x18; 
   wire[7:0] v_corr_20_fifo_x19; 
   wire[7:0] v_corr_20_fifo_x20; 
   wire[8:0] v_corr_10_fifo_x0; 
   wire[8:0] v_corr_10_fifo_x1; 
   wire[8:0] v_corr_10_fifo_x2; 
   wire[8:0] v_corr_10_fifo_x3; 
   wire[8:0] v_corr_10_fifo_x4; 
   wire[8:0] v_corr_10_fifo_x5; 
   wire[8:0] v_corr_10_fifo_x6; 
   wire[8:0] v_corr_10_fifo_x7; 
   wire[8:0] v_corr_10_fifo_x8; 
   wire[8:0] v_corr_10_fifo_x9; 
   wire[8:0] v_corr_10_fifo_x10; 
   wire[8:0] v_corr_10_fifo_x11; 
   wire[8:0] v_corr_10_fifo_x12; 
   wire[8:0] v_corr_10_fifo_x13; 
   wire[8:0] v_corr_10_fifo_x14; 
   wire[8:0] v_corr_10_fifo_x15; 
   wire[8:0] v_corr_10_fifo_x16; 
   wire[8:0] v_corr_10_fifo_x17; 
   wire[8:0] v_corr_10_fifo_x18; 
   wire[8:0] v_corr_10_fifo_x19; 
   wire[8:0] v_corr_10_fifo_x20; 
   wire[15:0] qs_4_out0; 
   wire[15:0] qs_4_out1; 
   wire[15:0] qs_4_out2; 
   wire[15:0] qs_4_out3; 
   wire[15:0] qs_4_out4; 
   wire[15:0] qs_4_out5; 
   wire[15:0] qs_4_out6; 
   wire[15:0] qs_4_out7; 
   wire[15:0] qs_4_out8; 
   wire[15:0] qs_4_out9; 
   wire[15:0] qs_4_out10; 
   wire[15:0] qs_4_out11; 
   wire[15:0] qs_4_out12; 
   wire[15:0] qs_4_out13; 
   wire[15:0] qs_4_out14; 
   wire[15:0] qs_4_out15; 
   wire[15:0] qs_4_out16; 
   wire[15:0] qs_4_out17; 
   wire[15:0] qs_4_out18; 
   wire[15:0] qs_4_out19; 
   wire[15:0] qs_4_out20; 
   wire[15:0] qs_2_out0; 
   wire[15:0] qs_2_out1; 
   wire[15:0] qs_2_out2; 
   wire[15:0] qs_2_out3; 
   wire[15:0] qs_2_out4; 
   wire[15:0] qs_2_out5; 
   wire[15:0] qs_2_out6; 
   wire[15:0] qs_2_out7; 
   wire[15:0] qs_2_out8; 
   wire[15:0] qs_2_out9; 
   wire[15:0] qs_2_out10; 
   wire[15:0] qs_2_out11; 
   wire[15:0] qs_2_out12; 
   wire[15:0] qs_2_out13; 
   wire[15:0] qs_2_out14; 
   wire[15:0] qs_2_out15; 
   wire[15:0] qs_2_out16; 
   wire[15:0] qs_2_out17; 
   wire[15:0] qs_2_out18; 
   wire[15:0] qs_2_out19; 
   wire[15:0] qs_2_out20; 
   wire[15:0] qs_4_out_x0; 
   wire[15:0] qs_4_out_x1; 
   wire[15:0] qs_4_out_x2; 
   wire[15:0] qs_4_out_x3; 
   wire[15:0] qs_4_out_x4; 
   wire[15:0] qs_4_out_x5; 
   wire[15:0] qs_4_out_x6; 
   wire[15:0] qs_4_out_x7; 
   wire[15:0] qs_4_out_x8; 
   wire[15:0] qs_4_out_x9; 
   wire[15:0] qs_4_out_x10; 
   wire[15:0] qs_4_out_x11; 
   wire[15:0] qs_4_out_x12; 
   wire[15:0] qs_4_out_x13; 
   wire[15:0] qs_4_out_x14; 
   wire[15:0] qs_4_out_x15; 
   wire[15:0] qs_4_out_x16; 
   wire[15:0] qs_4_out_x17; 
   wire[15:0] qs_4_out_x18; 
   wire[15:0] qs_4_out_x19; 
   wire[15:0] qs_4_out_x20; 
   wire[15:0] qs_2_out_x0; 
   wire[15:0] qs_2_out_x1; 
   wire[15:0] qs_2_out_x2; 
   wire[15:0] qs_2_out_x3; 
   wire[15:0] qs_2_out_x4; 
   wire[15:0] qs_2_out_x5; 
   wire[15:0] qs_2_out_x6; 
   wire[15:0] qs_2_out_x7; 
   wire[15:0] qs_2_out_x8; 
   wire[15:0] qs_2_out_x9; 
   wire[15:0] qs_2_out_x10; 
   wire[15:0] qs_2_out_x11; 
   wire[15:0] qs_2_out_x12; 
   wire[15:0] qs_2_out_x13; 
   wire[15:0] qs_2_out_x14; 
   wire[15:0] qs_2_out_x15; 
   wire[15:0] qs_2_out_x16; 
   wire[15:0] qs_2_out_x17; 
   wire[15:0] qs_2_out_x18; 
   wire[15:0] qs_2_out_x19; 
   wire[15:0] qs_2_out_x20; 
   wire rdy_4_out; 
	wire rdy_tmp3;
	wire rdy_tmp2;
	wire rdy_tmp1;
   wire[7:0] max_data_out; 
   wire[4:0] max_indx_out; 
   wire v_nd_s1_left_2to0_tmp; 
   wire v_nd_s2_left_2to0_tmp; 
   wire v_nd_s4_left_2to0_tmp; 
   wire[7:0] v_d_reg_s1_left_2to0_tmp; 
   wire[7:0] v_d_reg_s2_left_2to0_tmp; 
   wire[7:0] v_d_reg_s4_left_2to0_tmp; 
   wire[7:0] v_d_reg_s1_left_2to0_fifo_tmp; 
   wire[7:0] v_d_reg_s2_left_2to0_fifo_tmp; 
   wire[7:0] v_d_reg_s4_left_2to0_fifo_tmp; 
   wire v_nd_s1_right_2to0_tmp; 
   wire v_nd_s2_right_2to0_tmp; 
   wire v_nd_s4_right_2to0_tmp; 
   wire[7:0] v_d_reg_s1_right_2to0_tmp; 
   wire[7:0] v_d_reg_s2_right_2to0_tmp; 
   wire[7:0] v_d_reg_s4_right_2to0_tmp; 
   wire[7:0] v_d_reg_s1_right_2to0_fifo_tmp; 
   wire[7:0] v_d_reg_s2_right_2to0_fifo_tmp; 
   wire[7:0] v_d_reg_s4_right_2to0_fifo_tmp; 
   wire[10:0] comb_out0; 
   wire[10:0] comb_out1; 
   wire[10:0] comb_out2; 
   wire[10:0] comb_out3; 
   wire[10:0] comb_out4; 
   wire[10:0] comb_out5; 
   wire[10:0] comb_out6; 
   wire[10:0] comb_out7; 
   wire[10:0] comb_out8; 
   wire[10:0] comb_out9; 
   wire[10:0] comb_out10; 
   wire[10:0] comb_out11; 
   wire[10:0] comb_out12; 
   wire[10:0] comb_out13; 
   wire[10:0] comb_out14; 
   wire[10:0] comb_out15; 
   wire[10:0] comb_out16; 
   wire[10:0] comb_out17; 
   wire[10:0] comb_out18; 
   wire[10:0] comb_out19; 
   wire[10:0] comb_out20; 
   // <<X-HDL>> Unsupported Construct - attribute  (source line 258)
   // <<X-HDL>> Unsupported Construct - attribute  (source line 259))
   // <<X-HDL>> Unsupported Construct - attribute  (source line 273)
   // <<X-HDL>> Unsupported Construct - attribute  (source line 274))

   assign tm3_sram_data_out = tm3_sram_data_xhdl0;
   scaler scaler_inst_left (tm3_clk_v0, vidin_new_data_fifo, vidin_rgb_reg_fifo_left, vidin_addr_reg_2to0, v_nd_s1_left_2to0_tmp, v_nd_s2_left_2to0_tmp, v_nd_s4_left_2to0_tmp, v_d_reg_s1_left_2to0_tmp, v_d_reg_s2_left_2to0_tmp, v_d_reg_s4_left_2to0_tmp);
   scaler scaler_inst_right (tm3_clk_v0, vidin_new_data_fifo, vidin_rgb_reg_fifo_right, vidin_addr_reg_2to0, v_nd_s1_right_2to0_tmp, v_nd_s2_right_2to0_tmp, v_nd_s4_right_2to0_tmp, v_d_reg_s1_right_2to0_tmp, v_d_reg_s2_right_2to0_tmp, v_d_reg_s4_right_2to0_tmp);
   v_fltr_496  v_fltr_496_l_inst(tm3_clk_v0, v_nd_s1_left_2to0_tmp, v_d_reg_s1_left_2to0_tmp, v_d_reg_s1_left_2to0_fifo_tmp); 
   v_fltr_496  v_fltr_496_r_inst(tm3_clk_v0, v_nd_s1_right_2to0_tmp, v_d_reg_s1_right_2to0_tmp, v_d_reg_s1_right_2to0_fifo_tmp); 
   v_fltr_316  v_fltr_316_l_inst(tm3_clk_v0, v_nd_s2_left_2to0_tmp, v_d_reg_s2_left_2to0_tmp, v_d_reg_s2_left_2to0_fifo_tmp); 
   v_fltr_316  v_fltr_316_r_inst(tm3_clk_v0, v_nd_s2_right_2to0_tmp, v_d_reg_s2_right_2to0_tmp, v_d_reg_s2_right_2to0_fifo_tmp); 
   port_bus_1to0_1  port_bus_1to0_1_inst(tm3_clk_v0, vidin_addr_reg, svid_comp_switch_2to3, vidin_new_data_scld_1_1to0, 
	v_corr_200, v_corr_201, v_corr_202, v_corr_203, v_corr_204, v_corr_205, v_corr_206, v_corr_207, v_corr_208, v_corr_209, v_corr_2010, v_corr_2011, v_corr_2012, v_corr_2013, v_corr_2014, v_corr_2015, v_corr_2016, v_corr_2017, v_corr_2018, v_corr_2019, v_corr_2020, 
	vidin_new_data_scld_2_1to0, 
	v_corr_100, v_corr_101, v_corr_102, v_corr_103, v_corr_104, v_corr_105, v_corr_106, v_corr_107, v_corr_108, v_corr_109, v_corr_1010, 
	vidin_new_data_scld_4_1to0, 
	v_corr_50, v_corr_51, v_corr_52, v_corr_53, v_corr_54, v_corr_55, 
	bus_word_1_1to0, bus_word_2_1to0, bus_word_3_1to0, bus_word_4_1to0, bus_word_5_1to0, bus_word_6_1to0, counter_out_1to0);

   lp_fltr inst_fir_1_0 (tm3_clk_v0, v_corr_200, v_corr_20_fltr_h0, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_1 (tm3_clk_v0, v_corr_201, v_corr_20_fltr_h1, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_2 (tm3_clk_v0, v_corr_202, v_corr_20_fltr_h2, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_3 (tm3_clk_v0, v_corr_203, v_corr_20_fltr_h3, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_4 (tm3_clk_v0, v_corr_204, v_corr_20_fltr_h4, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_5 (tm3_clk_v0, v_corr_205, v_corr_20_fltr_h5, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_6 (tm3_clk_v0, v_corr_206, v_corr_20_fltr_h6, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_7 (tm3_clk_v0, v_corr_207, v_corr_20_fltr_h7, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_8 (tm3_clk_v0, v_corr_208, v_corr_20_fltr_h8, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_9 (tm3_clk_v0, v_corr_209, v_corr_20_fltr_h9, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_10 (tm3_clk_v0, v_corr_2010, v_corr_20_fltr_h10, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_11 (tm3_clk_v0, v_corr_2011, v_corr_20_fltr_h11, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_12 (tm3_clk_v0, v_corr_2012, v_corr_20_fltr_h12, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_13 (tm3_clk_v0, v_corr_2013, v_corr_20_fltr_h13, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_14 (tm3_clk_v0, v_corr_2014, v_corr_20_fltr_h14, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_15 (tm3_clk_v0, v_corr_2015, v_corr_20_fltr_h15, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_16 (tm3_clk_v0, v_corr_2016, v_corr_20_fltr_h16, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_17 (tm3_clk_v0, v_corr_2017, v_corr_20_fltr_h17, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_18 (tm3_clk_v0, v_corr_2018, v_corr_20_fltr_h18, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_19 (tm3_clk_v0, v_corr_2019, v_corr_20_fltr_h19, vidin_new_data_scld_1_1to0);
   lp_fltr inst_fir_1_20 (tm3_clk_v0, v_corr_2020, v_corr_20_fltr_h20, vidin_new_data_scld_1_1to0); 
   lp_fltr inst_fir_2_0 (tm3_clk_v0, v_corr_100, v_corr_10_fltr_h0, vidin_new_data_scld_2_1to0);
   lp_fltr inst_fir_2_1 (tm3_clk_v0, v_corr_101, v_corr_10_fltr_h1, vidin_new_data_scld_2_1to0);
   lp_fltr inst_fir_2_2 (tm3_clk_v0, v_corr_102, v_corr_10_fltr_h2, vidin_new_data_scld_2_1to0);
   lp_fltr inst_fir_2_3 (tm3_clk_v0, v_corr_103, v_corr_10_fltr_h3, vidin_new_data_scld_2_1to0);
   lp_fltr inst_fir_2_4 (tm3_clk_v0, v_corr_104, v_corr_10_fltr_h4, vidin_new_data_scld_2_1to0);
   lp_fltr inst_fir_2_5 (tm3_clk_v0, v_corr_105, v_corr_10_fltr_h5, vidin_new_data_scld_2_1to0);
   lp_fltr inst_fir_2_6 (tm3_clk_v0, v_corr_106, v_corr_10_fltr_h6, vidin_new_data_scld_2_1to0);
   lp_fltr inst_fir_2_7 (tm3_clk_v0, v_corr_107, v_corr_10_fltr_h7, vidin_new_data_scld_2_1to0);
   lp_fltr inst_fir_2_8 (tm3_clk_v0, v_corr_108, v_corr_10_fltr_h8, vidin_new_data_scld_2_1to0);
   lp_fltr inst_fir_2_9 (tm3_clk_v0, v_corr_109, v_corr_10_fltr_h9, vidin_new_data_scld_2_1to0);
   lp_fltr inst_fir_2_10 (tm3_clk_v0, v_corr_1010, v_corr_10_fltr_h10, vidin_new_data_scld_2_1to0); 
   lp_fltr inst_fir_4_0 (tm3_clk_v0, v_corr_50, v_corr_5_fltr_h0, vidin_new_data_scld_4_1to0);
   lp_fltr inst_fir_4_1 (tm3_clk_v0, v_corr_51, v_corr_5_fltr_h1, vidin_new_data_scld_4_1to0);
   lp_fltr inst_fir_4_2 (tm3_clk_v0, v_corr_52, v_corr_5_fltr_h2, vidin_new_data_scld_4_1to0);
   lp_fltr inst_fir_4_3 (tm3_clk_v0, v_corr_53, v_corr_5_fltr_h3, vidin_new_data_scld_4_1to0);
   lp_fltr inst_fir_4_4 (tm3_clk_v0, v_corr_54, v_corr_5_fltr_h4, vidin_new_data_scld_4_1to0);
   lp_fltr inst_fir_4_5 (tm3_clk_v0, v_corr_55, v_corr_5_fltr_h5, vidin_new_data_scld_4_1to0); 
   lp_fltr_v1 inst_fir_v1_0 (tm3_clk_v0, v_corr_20_fltr_h0, v_corr_20_fltr_x0, v_corr_20_fltr0, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_1 (tm3_clk_v0, v_corr_20_fltr_h1, v_corr_20_fltr_x1, v_corr_20_fltr1, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_2 (tm3_clk_v0, v_corr_20_fltr_h2, v_corr_20_fltr_x2, v_corr_20_fltr2, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_3 (tm3_clk_v0, v_corr_20_fltr_h3, v_corr_20_fltr_x3, v_corr_20_fltr3, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_4 (tm3_clk_v0, v_corr_20_fltr_h4, v_corr_20_fltr_x4, v_corr_20_fltr4, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_5 (tm3_clk_v0, v_corr_20_fltr_h5, v_corr_20_fltr_x5, v_corr_20_fltr5, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_6 (tm3_clk_v0, v_corr_20_fltr_h6, v_corr_20_fltr_x6, v_corr_20_fltr6, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_7 (tm3_clk_v0, v_corr_20_fltr_h7, v_corr_20_fltr_x7, v_corr_20_fltr7, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_8 (tm3_clk_v0, v_corr_20_fltr_h8, v_corr_20_fltr_x8, v_corr_20_fltr8, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_9 (tm3_clk_v0, v_corr_20_fltr_h9, v_corr_20_fltr_x9, v_corr_20_fltr9, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_10 (tm3_clk_v0, v_corr_20_fltr_h10, v_corr_20_fltr_x10, v_corr_20_fltr10, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_11 (tm3_clk_v0, v_corr_20_fltr_h11, v_corr_20_fltr_x11, v_corr_20_fltr11, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_12 (tm3_clk_v0, v_corr_20_fltr_h12, v_corr_20_fltr_x12, v_corr_20_fltr12, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_13 (tm3_clk_v0, v_corr_20_fltr_h13, v_corr_20_fltr_x13, v_corr_20_fltr13, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_14 (tm3_clk_v0, v_corr_20_fltr_h14, v_corr_20_fltr_x14, v_corr_20_fltr14, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_15 (tm3_clk_v0, v_corr_20_fltr_h15, v_corr_20_fltr_x15, v_corr_20_fltr15, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_16 (tm3_clk_v0, v_corr_20_fltr_h16, v_corr_20_fltr_x16, v_corr_20_fltr16, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_17 (tm3_clk_v0, v_corr_20_fltr_h17, v_corr_20_fltr_x17, v_corr_20_fltr17, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_18 (tm3_clk_v0, v_corr_20_fltr_h18, v_corr_20_fltr_x18, v_corr_20_fltr18, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_19 (tm3_clk_v0, v_corr_20_fltr_h19, v_corr_20_fltr_x19, v_corr_20_fltr19, vidin_new_data_scld_1_1to0);
   lp_fltr_v1 inst_fir_v1_20 (tm3_clk_v0, v_corr_20_fltr_h20, v_corr_20_fltr_x20, v_corr_20_fltr20, vidin_new_data_scld_1_1to0); 
   lp_fltr_v2 inst_fir_v2_0 (tm3_clk_v0, v_corr_10_fltr_h0, v_corr_10_fltr_x0, v_corr_10_fltr0, vidin_new_data_scld_2_1to0);
   lp_fltr_v2 inst_fir_v2_1 (tm3_clk_v0, v_corr_10_fltr_h1, v_corr_10_fltr_x1, v_corr_10_fltr1, vidin_new_data_scld_2_1to0);
   lp_fltr_v2 inst_fir_v2_2 (tm3_clk_v0, v_corr_10_fltr_h2, v_corr_10_fltr_x2, v_corr_10_fltr2, vidin_new_data_scld_2_1to0);
   lp_fltr_v2 inst_fir_v2_3 (tm3_clk_v0, v_corr_10_fltr_h3, v_corr_10_fltr_x3, v_corr_10_fltr3, vidin_new_data_scld_2_1to0);
   lp_fltr_v2 inst_fir_v2_4 (tm3_clk_v0, v_corr_10_fltr_h4, v_corr_10_fltr_x4, v_corr_10_fltr4, vidin_new_data_scld_2_1to0);
   lp_fltr_v2 inst_fir_v2_5 (tm3_clk_v0, v_corr_10_fltr_h5, v_corr_10_fltr_x5, v_corr_10_fltr5, vidin_new_data_scld_2_1to0);
   lp_fltr_v2 inst_fir_v2_6 (tm3_clk_v0, v_corr_10_fltr_h6, v_corr_10_fltr_x6, v_corr_10_fltr6, vidin_new_data_scld_2_1to0);
   lp_fltr_v2 inst_fir_v2_7 (tm3_clk_v0, v_corr_10_fltr_h7, v_corr_10_fltr_x7, v_corr_10_fltr7, vidin_new_data_scld_2_1to0);
   lp_fltr_v2 inst_fir_v2_8 (tm3_clk_v0, v_corr_10_fltr_h8, v_corr_10_fltr_x8, v_corr_10_fltr8, vidin_new_data_scld_2_1to0);
   lp_fltr_v2 inst_fir_v2_9 (tm3_clk_v0, v_corr_10_fltr_h9, v_corr_10_fltr_x9, v_corr_10_fltr9, vidin_new_data_scld_2_1to0);
   lp_fltr_v2 inst_fir_v2_10 (tm3_clk_v0, v_corr_10_fltr_h10, v_corr_10_fltr_x10, v_corr_10_fltr10, vidin_new_data_scld_2_1to0); 
   lp_fltr_v4 inst_fir_v4_0 (tm3_clk_v0, v_corr_5_fltr_h0, v_corr_5_fltr_x0, v_corr_5_fltr0, vidin_new_data_scld_4_1to0);
   lp_fltr_v4 inst_fir_v4_1 (tm3_clk_v0, v_corr_5_fltr_h1, v_corr_5_fltr_x1, v_corr_5_fltr1, vidin_new_data_scld_4_1to0);
   lp_fltr_v4 inst_fir_v4_2 (tm3_clk_v0, v_corr_5_fltr_h2, v_corr_5_fltr_x2, v_corr_5_fltr2, vidin_new_data_scld_4_1to0);
   lp_fltr_v4 inst_fir_v4_3 (tm3_clk_v0, v_corr_5_fltr_h3, v_corr_5_fltr_x3, v_corr_5_fltr3, vidin_new_data_scld_4_1to0);
   lp_fltr_v4 inst_fir_v4_4 (tm3_clk_v0, v_corr_5_fltr_h4, v_corr_5_fltr_x4, v_corr_5_fltr4, vidin_new_data_scld_4_1to0);
   lp_fltr_v4 inst_fir_v4_5 (tm3_clk_v0, v_corr_5_fltr_h5, v_corr_5_fltr_x5, v_corr_5_fltr5, vidin_new_data_scld_4_1to0); 

   wrapper_qs_intr_5_20 wrapper_qs_intr_inst_5 (tm3_clk_v0, 
		v_corr_5_fltr0, 
		v_corr_5_fltr1, 
		v_corr_5_fltr2, 
		v_corr_5_fltr3, 
		v_corr_5_fltr4, 
		v_corr_5_fltr5, 
		vidin_new_data_scld_4_1to0, vidin_addr_reg,
		qs_4_out0, 
		qs_4_out1, 
		qs_4_out2, 
		qs_4_out3, 
		qs_4_out4, 
		qs_4_out5, 
		qs_4_out6, 
		qs_4_out7, 
		qs_4_out8, 
		qs_4_out9, 
		qs_4_out10, 
		qs_4_out11, 
		qs_4_out12, 
		qs_4_out13, 
		qs_4_out14, 
		qs_4_out15, 
		qs_4_out16, 
		qs_4_out17, 
		qs_4_out18, 
		qs_4_out19, 
		qs_4_out20,
		rdy_4_out);
   wrapper_qs_intr_10_20 wrapper_qs_intr_inst_10 (tm3_clk_v0, 
		v_corr_10_fltr0, 
		v_corr_10_fltr1, 
		v_corr_10_fltr2, 
		v_corr_10_fltr3, 
		v_corr_10_fltr4, 
		v_corr_10_fltr5, 
		v_corr_10_fltr6, 
		v_corr_10_fltr7, 
		v_corr_10_fltr8, 
		v_corr_10_fltr9, 
		v_corr_10_fltr10, 
		vidin_new_data_scld_2_1to0, vidin_addr_reg, 
		qs_2_out0, 
		qs_2_out1, 
		qs_2_out2, 
		qs_2_out3, 
		qs_2_out4, 
		qs_2_out5, 
		qs_2_out6, 
		qs_2_out7, 
		qs_2_out8, 
		qs_2_out9, 
		qs_2_out10, 
		qs_2_out11, 
		qs_2_out12, 
		qs_2_out13, 
		qs_2_out14, 
		qs_2_out15, 
		qs_2_out16, 
		qs_2_out17, 
		qs_2_out18, 
		qs_2_out19, 
		qs_2_out20,
		rdy_tmp3);
   wrapper_qs_intr_5_20 wrapper_qs_intr_inst_5_more (tm3_clk_v0, 
		v_corr_5_fltr_x0, 
		v_corr_5_fltr_x1, 
		v_corr_5_fltr_x2, 
		v_corr_5_fltr_x3, 
		v_corr_5_fltr_x4, 
		v_corr_5_fltr_x5, 
		vidin_new_data_scld_4_1to0, vidin_addr_reg, 
		qs_4_out_x0, 
		qs_4_out_x1, 
		qs_4_out_x2, 
		qs_4_out_x3, 
		qs_4_out_x4, 
		qs_4_out_x5, 
		qs_4_out_x6, 
		qs_4_out_x7, 
		qs_4_out_x8, 
		qs_4_out_x9, 
		qs_4_out_x10, 
		qs_4_out_x11, 
		qs_4_out_x12, 
		qs_4_out_x13, 
		qs_4_out_x14, 
		qs_4_out_x15, 
		qs_4_out_x16, 
		qs_4_out_x17, 
		qs_4_out_x18, 
		qs_4_out_x19, 
		qs_4_out_x20,
		rdy_tmp2);
   wrapper_qs_intr_10_20 wrapper_qs_intr_inst_10_more (tm3_clk_v0, 
		v_corr_10_fltr_x0, 
		v_corr_10_fltr_x1, 
		v_corr_10_fltr_x2, 
		v_corr_10_fltr_x3, 
		v_corr_10_fltr_x4, 
		v_corr_10_fltr_x5, 
		v_corr_10_fltr_x6, 
		v_corr_10_fltr_x7, 
		v_corr_10_fltr_x8, 
		v_corr_10_fltr_x9, 
		v_corr_10_fltr_x10, 
		vidin_new_data_scld_2_1to0, vidin_addr_reg, 
		qs_2_out_x0,  
		qs_2_out_x1,  
		qs_2_out_x2,  
		qs_2_out_x3,  
		qs_2_out_x4,  
		qs_2_out_x5,  
		qs_2_out_x6,  
		qs_2_out_x7,  
		qs_2_out_x8,  
		qs_2_out_x9,  
		qs_2_out_x10,  
		qs_2_out_x11,  
		qs_2_out_x12,  
		qs_2_out_x13,  
		qs_2_out_x14,  
		qs_2_out_x15,  
		qs_2_out_x16,  
		qs_2_out_x17,  
		qs_2_out_x18,  
		qs_2_out_x19,  
		qs_2_out_x20, 
		rdy_tmp1);

   my_fifo_1 ints_fifo_1_gen_1_0 (tm3_clk_v0, v_corr_20_fltr0, v_corr_20_fifo0, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_1 (tm3_clk_v0, v_corr_20_fltr1, v_corr_20_fifo1, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_2 (tm3_clk_v0, v_corr_20_fltr2, v_corr_20_fifo2, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_3 (tm3_clk_v0, v_corr_20_fltr3, v_corr_20_fifo3, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_4 (tm3_clk_v0, v_corr_20_fltr4, v_corr_20_fifo4, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_5 (tm3_clk_v0, v_corr_20_fltr5, v_corr_20_fifo5, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_6 (tm3_clk_v0, v_corr_20_fltr6, v_corr_20_fifo6, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_7 (tm3_clk_v0, v_corr_20_fltr7, v_corr_20_fifo7, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_8 (tm3_clk_v0, v_corr_20_fltr8, v_corr_20_fifo8, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_9 (tm3_clk_v0, v_corr_20_fltr9, v_corr_20_fifo9, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_10 (tm3_clk_v0, v_corr_20_fltr10, v_corr_20_fifo10, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_11 (tm3_clk_v0, v_corr_20_fltr11, v_corr_20_fifo11, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_12 (tm3_clk_v0, v_corr_20_fltr12, v_corr_20_fifo12, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_13 (tm3_clk_v0, v_corr_20_fltr13, v_corr_20_fifo13, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_14 (tm3_clk_v0, v_corr_20_fltr14, v_corr_20_fifo14, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_15 (tm3_clk_v0, v_corr_20_fltr15, v_corr_20_fifo15, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_16 (tm3_clk_v0, v_corr_20_fltr16, v_corr_20_fifo16, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_17 (tm3_clk_v0, v_corr_20_fltr17, v_corr_20_fifo17, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_18 (tm3_clk_v0, v_corr_20_fltr18, v_corr_20_fifo18, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_19 (tm3_clk_v0, v_corr_20_fltr19, v_corr_20_fifo19, rdy_4_out);
   my_fifo_1 ints_fifo_1_gen_1_20 (tm3_clk_v0, v_corr_20_fltr20, v_corr_20_fifo20, rdy_4_out); 
   // <<X-HDL>> Can't find translated component 'my_fifo_2'. Module name may not match
   my_fifo_2 ints_fifo_2_gen_1_0 (tm3_clk_v0, qs_2_out0[8:0], v_corr_10_fifo0, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_1 (tm3_clk_v0, qs_2_out1[8:0], v_corr_10_fifo1, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_2 (tm3_clk_v0, qs_2_out2[8:0], v_corr_10_fifo2, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_3 (tm3_clk_v0, qs_2_out3[8:0], v_corr_10_fifo3, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_4 (tm3_clk_v0, qs_2_out4[8:0], v_corr_10_fifo4, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_5 (tm3_clk_v0, qs_2_out5[8:0], v_corr_10_fifo5, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_6 (tm3_clk_v0, qs_2_out6[8:0], v_corr_10_fifo6, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_7 (tm3_clk_v0, qs_2_out7[8:0], v_corr_10_fifo7, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_8 (tm3_clk_v0, qs_2_out8[8:0], v_corr_10_fifo8, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_9 (tm3_clk_v0, qs_2_out9[8:0], v_corr_10_fifo9, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_10 (tm3_clk_v0, qs_2_out10[8:0], v_corr_10_fifo10, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_11 (tm3_clk_v0, qs_2_out11[8:0], v_corr_10_fifo11, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_12 (tm3_clk_v0, qs_2_out12[8:0], v_corr_10_fifo12, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_13 (tm3_clk_v0, qs_2_out13[8:0], v_corr_10_fifo13, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_14 (tm3_clk_v0, qs_2_out14[8:0], v_corr_10_fifo14, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_15 (tm3_clk_v0, qs_2_out15[8:0], v_corr_10_fifo15, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_16 (tm3_clk_v0, qs_2_out16[8:0], v_corr_10_fifo16, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_17 (tm3_clk_v0, qs_2_out17[8:0], v_corr_10_fifo17, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_18 (tm3_clk_v0, qs_2_out18[8:0], v_corr_10_fifo18, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_19 (tm3_clk_v0, qs_2_out19[8:0], v_corr_10_fifo19, rdy_4_out);
   my_fifo_2 ints_fifo_2_gen_1_20 (tm3_clk_v0, qs_2_out20[8:0], v_corr_10_fifo20, rdy_4_out); 
   combine_res combine_res_inst_0 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo0, v_corr_10_fifo0, qs_4_out0[8:0], comb_out0);
   combine_res combine_res_inst_1 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo1, v_corr_10_fifo1, qs_4_out0[8:0], comb_out1);
   combine_res combine_res_inst_2 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo2, v_corr_10_fifo2, qs_4_out0[8:0], comb_out2);
   combine_res combine_res_inst_3 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo3, v_corr_10_fifo3, qs_4_out0[8:0], comb_out3);
   combine_res combine_res_inst_4 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo4, v_corr_10_fifo4, qs_4_out0[8:0], comb_out4);
   combine_res combine_res_inst_5 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo5, v_corr_10_fifo5, qs_4_out0[8:0], comb_out5);
   combine_res combine_res_inst_6 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo6, v_corr_10_fifo6, qs_4_out0[8:0], comb_out6);
   combine_res combine_res_inst_7 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo7, v_corr_10_fifo7, qs_4_out0[8:0], comb_out7);
   combine_res combine_res_inst_8 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo8, v_corr_10_fifo8, qs_4_out0[8:0], comb_out8);
   combine_res combine_res_inst_9 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo9, v_corr_10_fifo9, qs_4_out0[8:0], comb_out9);
   combine_res combine_res_inst_10 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo10, v_corr_10_fifo10, qs_4_out0[8:0], comb_out10);
   combine_res combine_res_inst_11 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo11, v_corr_10_fifo11, qs_4_out0[8:0], comb_out11);
   combine_res combine_res_inst_12 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo12, v_corr_10_fifo12, qs_4_out0[8:0], comb_out12);
   combine_res combine_res_inst_13 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo13, v_corr_10_fifo13, qs_4_out0[8:0], comb_out13);
   combine_res combine_res_inst_14 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo14, v_corr_10_fifo14, qs_4_out0[8:0], comb_out14);
   combine_res combine_res_inst_15 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo15, v_corr_10_fifo15, qs_4_out0[8:0], comb_out15);
   combine_res combine_res_inst_16 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo16, v_corr_10_fifo16, qs_4_out0[8:0], comb_out16);
   combine_res combine_res_inst_17 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo17, v_corr_10_fifo17, qs_4_out0[8:0], comb_out17);
   combine_res combine_res_inst_18 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo18, v_corr_10_fifo18, qs_4_out0[8:0], comb_out18);
   combine_res combine_res_inst_19 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo19, v_corr_10_fifo19, qs_4_out0[8:0], comb_out19);
   combine_res combine_res_inst_20 (tm3_clk_v0, rdy_4_out, v_corr_20_fifo20, v_corr_10_fifo20, qs_4_out0[8:0], comb_out20); 
   find_max find_max_inst (tm3_clk_v0, rdy_4_out, comb_out0, comb_out1, comb_out2, comb_out3, comb_out4, comb_out5, comb_out6, comb_out7, comb_out8, comb_out9, comb_out10, comb_out11, comb_out12, comb_out13, comb_out14, comb_out15, comb_out16, comb_out17, comb_out18, comb_out19, comb_out20, max_data_out, max_indx_out); 

   always @(posedge tm3_clk_v0)
   begin
         vidin_new_data_scld_1_2to3_left_reg <= rdy_4_out ; 
         vidin_new_data_scld_2_2to3_left_reg <= rdy_4_out ; 
         vidin_new_data_scld_4_2to3_left_reg <= rdy_4_out ; 
         vidin_data_reg_scld_1_2to3_left_reg <= v_corr_20_fifo0 ; 

         vidin_data_reg_scld_1_2to3_right_reg <= v_corr_10_fifo0[8:1] ; 
         vidin_data_reg_scld_2_2to3_left_reg <= qs_4_out0[8:1] ; 
         vidin_data_reg_scld_2_2to3_right_reg <= comb_out0[8:1] ; 
         vidin_data_reg_scld_4_2to3_left_reg <= comb_out4[8:1] ; 
         vidin_data_reg_scld_4_2to3_right_reg <= {max_indx_out, 3'b000} ; 
         if (vidin_addr_reg[8:0] >= 9'b001001000)
         begin
            vidin_addr_reg_2to3_reg <= vidin_addr_reg - 19'b0000000000001001000 ; 
         end
         else
         begin
            vidin_addr_reg_2to3_reg <= vidin_addr_reg + 19'b0000000000100100000 ; 
         end 
   end 

   always @(posedge tm3_clk_v0)
   begin
         v_nd_s1_left_2to0 <= v_nd_s1_left_2to0_tmp ; 
         v_nd_s2_left_2to0 <= v_nd_s2_left_2to0_tmp ; 
         v_nd_s4_left_2to0 <= v_nd_s4_left_2to0_tmp ; 
         v_nd_s1_right_2to0 <= v_nd_s1_right_2to0_tmp ; 
         v_nd_s2_right_2to0 <= v_nd_s2_right_2to0_tmp ; 
         v_nd_s4_right_2to0 <= v_nd_s4_right_2to0_tmp ; 
         if (v_nd_s1_left_2to0_tmp == 1'b1)
         begin
            v_d_reg_s1_left_2to0 <= v_d_reg_s1_left_2to0_fifo_tmp ; 
            v_d_reg_s1_right_2to0 <= v_d_reg_s1_right_2to0_fifo_tmp ; 
         end 
         if (v_nd_s2_left_2to0_tmp == 1'b1)
         begin
            v_d_reg_s2_left_2to0 <= v_d_reg_s2_left_2to0_fifo_tmp ; 
            v_d_reg_s2_right_2to0 <= v_d_reg_s2_right_2to0_fifo_tmp ; 
         end 
         if (v_nd_s4_left_2to0_tmp == 1'b1)
         begin
            v_d_reg_s4_left_2to0 <= v_d_reg_s4_left_2to0_tmp ; 
            v_d_reg_s4_right_2to0 <= v_d_reg_s4_right_2to0_tmp ; 
         end 
   end 
   assign tm3_vidout_clock = ~(video_state) ;

   always @(posedge tm3_clk_v0)
   begin
         x_reg_l <= x_in ; 
         y_reg_up <= y_in ; 
         x_reg_r <= x_in + 8 ; 
         y_reg_dn <= y_in + 8 ; 
         depth_out <= {8'b00000000, depth_out_reg} ; 
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
            if ((vert >= 491) & (vert <= 493))
            begin
               tm3_vidout_vsync <= 1'b1 ; 
            end
            else
            begin
               tm3_vidout_vsync <= 1'b0 ; 
            end 
            if ((horiz >= 664) & (horiz <= 760))
            begin
               tm3_vidout_hsync <= 1'b1 ; 
            end
            else
            begin
               tm3_vidout_hsync <= 1'b0 ; 
            end 
            if ((horiz < 640) & (vert < 480))
            begin
               tm3_vidout_blank <= 1'b1 ; 
            end
            else
            begin
               tm3_vidout_blank <= 1'b0 ; 
            end 
            tm3_sram_adsp <= 1'b1 ; 
            tm3_sram_we <= 8'b11111111 ; 
            tm3_sram_data_xhdl0 <= 64'b0000000000000000000000000000000000000000000000000000000000000000 ; 
            case (horiz[2:0])
               3'b000 :
                        begin
                           tm3_sram_oe <= 2'b10 ; 
                           if ((horiz <= x_reg_r) & (horiz >= x_reg_l) & (vert <= y_reg_dn) & (vert >= y_reg_up))
                           begin
                              tm3_vidout_red <= 10'b1111111111 ; 
                              tm3_vidout_green <= 10'b0000000000 ; 
                              tm3_vidout_blue <= 10'b0000000000 ; 
                              depth_out_reg <= vidout_buf[15:8] ; 
                           end
                           else
                           begin
                              tm3_vidout_red <= {vidout_buf[15:8], 2'b00} ; 
                              tm3_vidout_green <= {vidout_buf[15:8], 2'b00} ; 
                              tm3_vidout_blue <= {vidout_buf[15:8], 2'b00} ; 
                           end 
                        end
               3'b001 :
                        begin
                           tm3_sram_oe <= 2'b10 ; 
                           if ((horiz <= x_reg_r) & (horiz >= x_reg_l) & (vert <= y_reg_dn) & (vert >= y_reg_up))
                           begin

                              tm3_vidout_red <= 10'b1111111111 ; 
                              tm3_vidout_green <= 10'b0000000000 ; 
                              tm3_vidout_blue <= 10'b0000000000 ; 
                              depth_out_reg <= vidout_buf[23:16] ; 
                           end
                           else
                           begin
                              tm3_vidout_red <= {vidout_buf[23:16], 2'b00} ; 
                              tm3_vidout_green <= {vidout_buf[23:16], 2'b00} ; 
                              tm3_vidout_blue <= {vidout_buf[23:16], 2'b00} ; 
                           end 
                        end
               3'b010 :
                        begin
                           tm3_sram_oe <= 2'b10 ; 
                           if ((horiz <= x_reg_r) & (horiz >= x_reg_l) & (vert <= y_reg_dn) & (vert >= y_reg_up))
                           begin
                              tm3_vidout_red <= 10'b1111111111 ; 
                              tm3_vidout_green <= 10'b0000000000 ; 
                              tm3_vidout_blue <= 10'b0000000000 ; 
                              depth_out_reg <= vidout_buf[31:24] ; 
                           end
                           else
                           begin
                              tm3_vidout_red <= {vidout_buf[31:24], 2'b00} ; 
                              tm3_vidout_green <= {vidout_buf[31:24], 2'b00} ; 
                              tm3_vidout_blue <= {vidout_buf[31:24], 2'b00} ; 
                           end 
                        end
               3'b011 :
                        begin
                           tm3_sram_oe <= 2'b10 ; 
                           if ((horiz <= x_reg_r) & (horiz >= x_reg_l) & (vert <= y_reg_dn) & (vert >= y_reg_up))
                           begin
                              tm3_vidout_red <= 10'b1111111111 ; 
                              tm3_vidout_green <= 10'b0000000000 ; 
                              tm3_vidout_blue <= 10'b0000000000 ; 
                              depth_out_reg <= vidout_buf[39:32] ; 
                           end
                           else
                           begin
                              tm3_vidout_red <= {vidout_buf[39:32], 2'b00} ; 
                              tm3_vidout_green <= {vidout_buf[39:32], 2'b00} ; 
                              tm3_vidout_blue <= {vidout_buf[39:32], 2'b00} ; 
                           end 
                        end
               3'b100 :
                        begin
                           tm3_sram_oe <= 2'b10 ; 
                           if ((horiz <= x_reg_r) & (horiz >= x_reg_l) & (vert <= y_reg_dn) & (vert >= y_reg_up))
                           begin
                              tm3_vidout_red <= 10'b1111111111 ; 
                              tm3_vidout_green <= 10'b0000000000 ; 
                              tm3_vidout_blue <= 10'b0000000000 ; 
                              depth_out_reg <= vidout_buf[47:40] ; 
                           end
                           else
                           begin
                              tm3_vidout_red <= {vidout_buf[47:40], 2'b00} ; 
                              tm3_vidout_green <= {vidout_buf[47:40], 2'b00} ; 
                              tm3_vidout_blue <= {vidout_buf[47:40], 2'b00} ; 
                           end 
                        end
               3'b101 :
                        begin
                           tm3_sram_oe <= 2'b10 ; 

                           if ((horiz <= x_reg_r) & (horiz >= x_reg_l) & (vert <= y_reg_dn) & (vert >= y_reg_up))
                           begin
                              tm3_vidout_red <= 10'b1111111111 ; 
                              tm3_vidout_green <= 10'b0000000000 ; 
                              tm3_vidout_blue <= 10'b0000000000 ; 
                              depth_out_reg <= vidout_buf[55:48] ; 
                           end
                           else
                           begin
                              tm3_vidout_red <= {vidout_buf[55:48], 2'b00} ; 
                              tm3_vidout_green <= {vidout_buf[55:48], 2'b00} ; 
                              tm3_vidout_blue <= {vidout_buf[55:48], 2'b00} ; 
                           end 
                        end
               3'b110 :
                        begin
                           tm3_sram_oe <= 2'b10 ; 
                           if ((horiz <= x_reg_r) & (horiz >= x_reg_l) & (vert <= y_reg_dn) & (vert >= y_reg_up))
                           begin
                              tm3_vidout_red <= 10'b1111111111 ; 
                              tm3_vidout_green <= 10'b0000000000 ; 
                              tm3_vidout_blue <= 10'b0000000000 ; 
                              depth_out_reg <= vidout_buf[63:56] ; 
                           end
                           else
                           begin
                              tm3_vidout_red <= {vidout_buf[63:56], 2'b00} ; 
                              tm3_vidout_green <= {vidout_buf[63:56], 2'b00} ; 
                              tm3_vidout_blue <= {vidout_buf[63:56], 2'b00} ; 
                           end 
                        end
               3'b111 :
                        begin
                           tm3_sram_oe <= 2'b11 ; 
                           if ((horiz <= x_reg_r) & (horiz >= x_reg_l) & (vert <= y_reg_dn) & (vert >= y_reg_up))
                           begin
                              tm3_vidout_red <= 10'b1111111111 ; 
                              tm3_vidout_green <= 10'b0000000000 ; 
                              tm3_vidout_blue <= 10'b0000000000 ; 
                              depth_out_reg <= vidout_buf[7:0] ; 
                           end
                           else
                           begin
                              tm3_vidout_red <= {vidout_buf[7:0], 2'b00} ; 
                              tm3_vidout_green <= {vidout_buf[7:0], 2'b00} ; 
                              tm3_vidout_blue <= {vidout_buf[7:0], 2'b00} ; 
                           end 
                        end
            endcase 
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
               3'b110 :
                        begin
                           tm3_sram_addr <= {5'b00101, vert[7:0], horiz[8:3]} ; 
                           tm3_sram_we <= 8'b11111111 ; 
                           tm3_sram_oe <= 2'b11 ; 
                           tm3_sram_data_xhdl0 <= 64'b0000000000000000000000000000000000000000000000000000000000000000; 
                        end
               3'b111 :
                        begin
                           vidout_buf <= tm3_sram_data_in ; 
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
                           tm3_sram_data_xhdl0 <= 64'b0000000000000000000000000000000000000000000000000000000000000000 ; 
                        end
            endcase 
         end 
         if (vidin_new_data_scld_1_2to3_left_reg == 1'b1)
         begin
            case ({svid_comp_switch_2to3, vidin_addr_reg_2to3_reg[2:0]})
               4'b0000 :
                        begin
                           vidin_data_buf_2_sc_1[7:0] <= vidin_data_reg_scld_1_2to3_left_reg ; 
                           vidin_data_buf_2_sc_2[7:0] <= vidin_data_reg_scld_2_2to3_left_reg ; 
                           vidin_data_buf_2_sc_4[7:0] <= vidin_data_reg_scld_4_2to3_left_reg ; 
                        end
               4'b0001 :
                        begin
                           vidin_data_buf_2_sc_1[15:8] <= vidin_data_reg_scld_1_2to3_left_reg ; 
                           vidin_data_buf_2_sc_2[15:8] <= vidin_data_reg_scld_2_2to3_left_reg ; 
                           vidin_data_buf_2_sc_4[15:8] <= vidin_data_reg_scld_4_2to3_left_reg ; 
                        end
               4'b0010 :
                        begin
                           vidin_data_buf_2_sc_1[23:16] <= vidin_data_reg_scld_1_2to3_left_reg ; 
                           vidin_data_buf_2_sc_2[23:16] <= vidin_data_reg_scld_2_2to3_left_reg ; 
                           vidin_data_buf_2_sc_4[23:16] <= vidin_data_reg_scld_4_2to3_left_reg ; 
                        end
               4'b0011 :
                        begin
                           vidin_data_buf_2_sc_1[31:24] <= vidin_data_reg_scld_1_2to3_left_reg ; 
                           vidin_data_buf_2_sc_2[31:24] <= vidin_data_reg_scld_2_2to3_left_reg ; 
                           vidin_data_buf_2_sc_4[31:24] <= vidin_data_reg_scld_4_2to3_left_reg ; 
                        end
               4'b0100 :
                        begin
                           vidin_data_buf_2_sc_1[39:32] <= vidin_data_reg_scld_1_2to3_left_reg ; 
                           vidin_data_buf_2_sc_2[39:32] <= vidin_data_reg_scld_2_2to3_left_reg ; 
                           vidin_data_buf_2_sc_4[39:32] <= vidin_data_reg_scld_4_2to3_left_reg ; 
                        end
               4'b0101 :
                        begin
                           vidin_data_buf_2_sc_1[47:40] <= vidin_data_reg_scld_1_2to3_left_reg ; 
                           vidin_data_buf_2_sc_2[47:40] <= vidin_data_reg_scld_2_2to3_left_reg ; 
                           vidin_data_buf_2_sc_4[47:40] <= vidin_data_reg_scld_4_2to3_left_reg ; 
                        end
               4'b0110 :
                        begin
                           vidin_data_buf_2_sc_1[55:48] <= vidin_data_reg_scld_1_2to3_left_reg ; 
                           vidin_data_buf_2_sc_2[55:48] <= vidin_data_reg_scld_2_2to3_left_reg ; 
                           vidin_data_buf_2_sc_4[55:48] <= vidin_data_reg_scld_4_2to3_left_reg ; 
                        end
               4'b0111 :
                        begin
                           vidin_data_buf_sc_1 <= {vidin_data_reg_scld_1_2to3_left_reg, vidin_data_buf_2_sc_1[55:0]} ; 
                           vidin_data_buf_sc_2 <= {vidin_data_reg_scld_2_2to3_left_reg, vidin_data_buf_2_sc_2[55:0]} ; 
                           vidin_data_buf_sc_4 <= {vidin_data_reg_scld_4_2to3_left_reg, vidin_data_buf_2_sc_4[55:0]} ; 
                           vidin_addr_buf_sc_1 <= {4'b0000, svid_comp_switch_2to3, vidin_addr_reg_2to3_reg[16:3]} ; 
                           vidin_addr_buf_sc_2 <= {4'b0001, svid_comp_switch_2to3, vidin_addr_reg_2to3_reg[16:3]} ; 
                           vidin_addr_buf_sc_4 <= {4'b0010, svid_comp_switch_2to3, vidin_addr_reg_2to3_reg[16:3]} ; 
						end
				4'b1000 :
                        begin
                           vidin_data_buf_2_sc_1[7:0] <= vidin_data_reg_scld_1_2to3_right_reg ; 
                           vidin_data_buf_2_sc_2[7:0] <= vidin_data_reg_scld_2_2to3_right_reg ; 
                           vidin_data_buf_2_sc_4[7:0] <= vidin_data_reg_scld_4_2to3_right_reg ; 
						end
               4'b1001 :
                        begin
                           vidin_data_buf_2_sc_1[15:8] <= vidin_data_reg_scld_1_2to3_right_reg ; 
                           vidin_data_buf_2_sc_2[15:8] <= vidin_data_reg_scld_2_2to3_right_reg ;
                           vidin_data_buf_2_sc_4[15:8] <= vidin_data_reg_scld_4_2to3_right_reg ; 
						end
               4'b1010 :
                        begin
                           vidin_data_buf_2_sc_1[23:16] <= vidin_data_reg_scld_1_2to3_right_reg ; 
                           vidin_data_buf_2_sc_2[23:16] <= vidin_data_reg_scld_2_2to3_right_reg ;
                           vidin_data_buf_2_sc_4[23:16] <= vidin_data_reg_scld_4_2to3_right_reg ; 
                        end
               4'b1011 :
                        begin
                           vidin_data_buf_2_sc_1[31:24] <= vidin_data_reg_scld_1_2to3_right_reg ; 
                           vidin_data_buf_2_sc_2[31:24] <= vidin_data_reg_scld_2_2to3_right_reg ; 
                           vidin_data_buf_2_sc_4[31:24] <= vidin_data_reg_scld_4_2to3_right_reg ; 
                        end
               4'b1100 :
                        begin
                           vidin_data_buf_2_sc_1[39:32] <= vidin_data_reg_scld_1_2to3_right_reg ; 
                           vidin_data_buf_2_sc_2[39:32] <= vidin_data_reg_scld_2_2to3_right_reg ; 
                           vidin_data_buf_2_sc_4[39:32] <= vidin_data_reg_scld_4_2to3_right_reg ; 
                        end
               4'b1101 :
                        begin
                           vidin_data_buf_2_sc_1[47:40] <= vidin_data_reg_scld_1_2to3_right_reg ; 
                           vidin_data_buf_2_sc_2[47:40] <= vidin_data_reg_scld_2_2to3_right_reg ; 
                           vidin_data_buf_2_sc_4[47:40] <= vidin_data_reg_scld_4_2to3_right_reg ; 
                        end
               4'b1110 :
                        begin
                           vidin_data_buf_2_sc_1[55:48] <= vidin_data_reg_scld_1_2to3_right_reg ; 
                           vidin_data_buf_2_sc_2[55:48] <= vidin_data_reg_scld_2_2to3_right_reg ; 
                           vidin_data_buf_2_sc_4[55:48] <= vidin_data_reg_scld_4_2to3_right_reg ; 
                        end
               4'b1111 :
                        begin
                           vidin_data_buf_sc_1 <= {vidin_data_reg_scld_1_2to3_right_reg, vidin_data_buf_2_sc_1[55:0]} ;
                           vidin_data_buf_sc_2 <= {vidin_data_reg_scld_2_2to3_right_reg, vidin_data_buf_2_sc_2[55:0]} ;
                           vidin_data_buf_sc_4 <= {vidin_data_reg_scld_4_2to3_right_reg, vidin_data_buf_2_sc_4[55:0]} ; 
                           vidin_addr_buf_sc_1 <= {4'b0000, svid_comp_switch_2to3, vidin_addr_reg_2to3_reg[16:3]} ; 
                           vidin_addr_buf_sc_2 <= {4'b0001, svid_comp_switch_2to3, vidin_addr_reg_2to3_reg[16:3]} ; 
                           vidin_addr_buf_sc_4 <= {4'b0010, svid_comp_switch_2to3, vidin_addr_reg_2to3_reg[16:3]} ; 
                        end
            endcase 
         end 
      end 
endmodule
module combine_res (clk, wen, din_1, din_2, din_3, dout);

    input clk; 
    input wen; 
    input[7:0] din_1; 
    input[8:0] din_2; 
    input[8:0] din_3; 
    output[10:0] dout; 
    reg[10:0] dout;
    reg[8:0] din_1_reg; 
    reg[8:0] din_2_reg; 
    reg[8:0] din_3_reg; 
    reg[10:0] add_tmp; 
    reg[10:0] dout_reg; 

    always @(posedge clk)
    begin
          if (wen == 1'b1)
          begin
             din_1_reg <= {din_1[7], din_1} ; 
             din_2_reg <= din_2 ; 
             din_3_reg <= din_3 ; 
             dout <= dout_reg ; 
          end 
          add_tmp <= ({din_1_reg[8], din_1_reg[8], din_1_reg}) + ({din_2_reg[8], din_2_reg[8], din_2_reg}) ; 
          dout_reg <= add_tmp + ({din_3_reg[8], din_3_reg[8], din_3_reg}) ; 
       end 
 endmodule
// Discription: this block creates a long fifo
// of  lengh of one line and then applies the
// the first and last byte of the fifo into a 
// that finally creates horizontal edge detection
// filter. 
// note: it uses fifo component to implement the fifo
// date: Oct.22 ,2001
// By:  Ahmad darabiha
module v_fltr_496 (tm3_clk_v0, vidin_new_data, vidin_in, vidin_out);


   input tm3_clk_v0; 
   input vidin_new_data; 
   input[7:0] vidin_in; 
   output[7:0] vidin_out; 
   wire[7:0] vidin_out;

   wire[7:0] buff_out0; 
   wire[7:0] buff_out1; 
   wire[7:0] buff_out2; 
   wire[7:0] buff_out3; 
   wire[7:0] buff_out4; 
   wire[7:0] buff_out5; 
   wire[7:0] buff_out6; 
   wire[7:0] buff_out7; 
   wire[7:0] buff_out8; 
   wire[7:0] buff_out9; 
   wire[7:0] buff_out10; 
   wire[7:0] buff_out11; 

   assign buff_out0 = vidin_in ;

	my_fifo_496 fifo0(tm3_clk_v0, buff_out0, buff_out1, vidin_new_data);
	my_fifo_496 fifo1(tm3_clk_v0, buff_out1, buff_out2, vidin_new_data);
	my_fifo_496 fifo2(tm3_clk_v0, buff_out2, buff_out3, vidin_new_data);
	my_fifo_496 fifo3(tm3_clk_v0, buff_out3, buff_out4, vidin_new_data);
	my_fifo_496 fifo4(tm3_clk_v0, buff_out4, buff_out5, vidin_new_data);
	my_fifo_496 fifo5(tm3_clk_v0, buff_out5, buff_out6, vidin_new_data);
	my_fifo_496 fifo6(tm3_clk_v0, buff_out6, buff_out7, vidin_new_data);
	my_fifo_496 fifo7(tm3_clk_v0, buff_out7, buff_out8, vidin_new_data);
	my_fifo_496 fifo8(tm3_clk_v0, buff_out8, buff_out9, vidin_new_data);
	my_fifo_496 fifo9(tm3_clk_v0, buff_out9, buff_out10, vidin_new_data);
	my_fifo_496 fifo10(tm3_clk_v0, buff_out10, buff_out11, vidin_new_data);

   my_fifo_496 more_inst (tm3_clk_v0, buff_out11, vidin_out, vidin_new_data); 
endmodule
// Discription: this block creates a long fifo
// of  lengh of one line and then applies the
// the first and last byte of the fifo into a 
// that finally creates horizontal edge detection
// filter. 
// note: it uses fifo component to implement the fifo
// date: Oct.22 ,2001
// By:  Ahmad darabiha
module v_fltr_316 (tm3_clk_v0, vidin_new_data, vidin_in, vidin_out);


   input tm3_clk_v0; 
   input vidin_new_data; 
   input[7:0] vidin_in; 
   output[7:0] vidin_out; 
   wire[7:0] vidin_out;

   wire[7:0] buff_out0;
   wire[7:0] buff_out1;
   wire[7:0] buff_out2;
   wire[7:0] buff_out3;

   assign buff_out0 = vidin_in ;

	my_fifo_316 fifo0(tm3_clk_v0, buff_out0, buff_out1, vidin_new_data);
	my_fifo_316 fifo1(tm3_clk_v0, buff_out1, buff_out2, vidin_new_data);
	my_fifo_316 fifo2(tm3_clk_v0, buff_out2, buff_out3, vidin_new_data);

   my_fifo_316 more_inst (tm3_clk_v0, buff_out3, vidin_out, vidin_new_data); 
endmodule

module lp_fltr_v1 (clk, din, dout_1, dout_2, nd);

   input clk; 
   input[8 - 1:0] din; 
   output[8 - 1:0] dout_1; 
   reg[8 - 1:0] dout_1;
   output[8 - 1:0] dout_2; 
   reg[8 - 1:0] dout_2;
   input nd; 

   reg[8 - 1:0] din_1_reg; 
   wire[8 - 1:0] buff_out_1; 
   wire[8 - 1:0] buff_out_2; 
   reg[8 - 1:0] din_2_reg; 
   reg[8 - 1:0] din_3_reg; 
   reg[8 + 1:0] add_tmp_1; 
   reg[8 + 1:0] add_tmp_2; 

   my_fifo_359 ints_fifo_1 (clk, din, buff_out_1, nd); 
   my_fifo_359 ints_fifo_2 (clk, buff_out_1, buff_out_2, nd); 

   always @(posedge clk)
   begin
         if (nd == 1'b1)
         begin
            din_1_reg <= din ; 
            din_2_reg <= buff_out_1 ; 
            din_3_reg <= buff_out_2 ; 
            dout_1 <= din ; 
            dout_2 <= add_tmp_2[8 + 1:2] ; 
         end 
         add_tmp_1 <= ({din_3_reg[8 - 1], din_3_reg[8 - 1], din_3_reg}) + ({din_1_reg[8 - 1], din_1_reg[8 - 1], din_1_reg}) ; 
         add_tmp_2 <= add_tmp_1 + ({din_2_reg[8 - 1], din_2_reg, 1'b0}) ; 
   end 
endmodule





module lp_fltr_v2 (clk, din, dout_1, dout_2, nd);

   input clk; 
   input[8 - 1:0] din; 
   output[8 - 1:0] dout_1; 
   reg[8 - 1:0] dout_1;
   output[8 - 1:0] dout_2; 
   reg[8 - 1:0] dout_2;
   input nd; 

   reg[8 - 1:0] din_1_reg; 
   wire[8 - 1:0] buff_out_1; 
   wire[8 - 1:0] buff_out_2; 
   reg[8 - 1:0] din_2_reg; 
   reg[8 - 1:0] din_3_reg; 
   reg[8 + 1:0] add_tmp_1; 
   reg[8 + 1:0] add_tmp_2; 
   // <<X-HDL>> Unsupported Construct - attribute  (source line 31)
   // <<X-HDL>> Unsupported Construct - attribute  (source line 32))

   my_fifo_179 ints_fifo_1 (clk, din, buff_out_1, nd); 
   my_fifo_179 ints_fifo_2 (clk, buff_out_1, buff_out_2, nd); 

   always @(posedge clk)
   begin
         if (nd == 1'b1)
         begin
            din_1_reg <= din ; 
            din_2_reg <= buff_out_1 ; 
            din_3_reg <= buff_out_2 ; 
            dout_1 <= din ; 
            dout_2 <= add_tmp_2[8 + 1:2] ; 
         end 
         add_tmp_1 <= ({din_3_reg[8 - 1], din_3_reg[8 - 1], din_3_reg}) + ({din_1_reg[8 - 1], din_1_reg[8 - 1], din_1_reg}) ; 
         add_tmp_2 <= add_tmp_1 + ({din_2_reg[8 - 1], din_2_reg, 1'b0}) ; 
   end 
endmodule

module lp_fltr_v4 (clk, din, dout_1, dout_2, nd);

   input clk; 
   input[8 - 1:0] din; 
   output[8 - 1:0] dout_1; 
   reg[8 - 1:0] dout_1;
   output[8 - 1:0] dout_2; 
   reg[8 - 1:0] dout_2;
   input nd; 

   reg[8 - 1:0] din_1_reg; 
   wire[8 - 1:0] buff_out_1; 
   wire[8 - 1:0] buff_out_2; 
   reg[8 - 1:0] din_2_reg; 
   reg[8 - 1:0] din_3_reg; 
   reg[8 + 1:0] add_tmp_1; 
   reg[8 + 1:0] add_tmp_2; 

   my_fifo_89 ints_fifo_1 (clk, din, buff_out_1, nd); 
   my_fifo_89 ints_fifo_2 (clk, buff_out_1, buff_out_2, nd); 

   always @(posedge clk)
   begin
         if (nd == 1'b1)
         begin
            din_1_reg <= din ; 
            din_2_reg <= buff_out_1 ; 
            din_3_reg <= buff_out_2 ; 
            dout_1 <= din ; 
            dout_2 <= add_tmp_2[8 + 1:2] ; 
         end 
         add_tmp_1 <= ({din_3_reg[8 - 1], din_3_reg[8 - 1], din_3_reg}) + ({din_1_reg[8 - 1], din_1_reg[8 - 1], din_1_reg}) ; 
         add_tmp_2 <= add_tmp_1 + ({din_2_reg[8 - 1], din_2_reg, 1'b0}) ; 
   end 
endmodule

module scaler (tm3_clk_v0, vidin_new_data, vidin_rgb_reg, vidin_addr_reg, vidin_new_data_scld_1, vidin_new_data_scld_2, vidin_new_data_scld_4, vidin_gray_scld_1, vidin_gray_scld_2, vidin_gray_scld_4);

   input tm3_clk_v0; 
   input vidin_new_data; 
   input[7:0] vidin_rgb_reg; 
   input[3:0] vidin_addr_reg; 
   output vidin_new_data_scld_1; 
   reg vidin_new_data_scld_1;
   output vidin_new_data_scld_2; 
   reg vidin_new_data_scld_2;
   output vidin_new_data_scld_4; 
   reg vidin_new_data_scld_4;
   output[7:0] vidin_gray_scld_1; 
   reg[7:0] vidin_gray_scld_1;
   output[7:0] vidin_gray_scld_2; 
   reg[7:0] vidin_gray_scld_2;
   output[7:0] vidin_gray_scld_4; 
   reg[7:0] vidin_gray_scld_4;

   wire[7:0] v_fltr_sc_1; 
   wire[7:0] v_fltr_sc_2; 
   wire[7:0] v_fltr_sc_4; 
   wire[7:0] h_fltr_sc_1; 
   wire[7:0] h_fltr_sc_2; 
   wire[7:0] h_fltr_sc_4; 

   scl_v_fltr scl_v_fltr_inst (tm3_clk_v0, vidin_new_data, vidin_rgb_reg, v_fltr_sc_1, v_fltr_sc_2, v_fltr_sc_4); 
   scl_h_fltr scl_h_fltr_inst (tm3_clk_v0, vidin_new_data, v_fltr_sc_1, v_fltr_sc_2, v_fltr_sc_4, h_fltr_sc_1, h_fltr_sc_2, h_fltr_sc_4); 

   always @(posedge tm3_clk_v0)
   begin
         vidin_new_data_scld_1 <= vidin_new_data ; 
         vidin_new_data_scld_2 <= 1'b0 ; 
         vidin_new_data_scld_4 <= 1'b0 ; 
         if (vidin_new_data == 1'b1)
         begin
            vidin_gray_scld_1 <= h_fltr_sc_1 ; 
            if ((vidin_addr_reg[0]) == 1'b0 & (vidin_addr_reg[2]) == 1'b0)
            begin
               vidin_gray_scld_2 <= h_fltr_sc_2 ; 
               vidin_new_data_scld_2 <= 1'b1 ; 
               if ((vidin_addr_reg[1]) == 1'b0 & (vidin_addr_reg[3]) == 1'b0)
               begin
                  vidin_gray_scld_4 <= h_fltr_sc_4 ; 
                  vidin_new_data_scld_4 <= 1'b1 ; 
               end 
            end 
         end 
   end 
endmodule
module scl_v_fltr (clk, nd, d_in, d_out_1, d_out_2, d_out_4);

   // <<X-HDL>> : Warning - included file work/basic_type does not exist - this may affect translation

   input clk; 
   input nd; 
   input[7:0] d_in; 
   output[7:0] d_out_1; 
   reg[7:0] d_out_1;
   output[7:0] d_out_2; 
   reg[7:0] d_out_2;
   output[7:0] d_out_4; 
   reg[7:0] d_out_4;

   wire[7:0] buff_out0; 
   wire[7:0] buff_out1; 
   wire[7:0] buff_out2; 
   wire[7:0] buff_out3; 
   wire[7:0] buff_out4; 
   wire[7:0] buff_out5; 
   wire[7:0] buff_out6; 
   wire[7:0] buff_out7; 
   reg[7:0] buff_out_reg0; 
   reg[7:0] buff_out_reg1; 
   reg[7:0] buff_out_reg2; 
   reg[7:0] buff_out_reg3; 
   reg[7:0] buff_out_reg4; 
   reg[7:0] buff_out_reg5; 
   reg[7:0] buff_out_reg6; 
   reg[7:0] buff_out_reg7; 
   reg[9:0] add_2_tmp_1; 
   reg[9:0] add_2_tmp_2; 
   reg[9:0] add_2_tmp; 
   reg[11:0] add_4_tmp_1; 
   reg[11:0] add_4_tmp_2; 
   reg[11:0] add_4_tmp_3; 
   reg[11:0] add_4_tmp_4; 
   reg[11:0] add_4_tmp_5; 
   reg[11:0] add_4_tmp_6; 
   reg[11:0] add_4_tmp_7; 
   reg[11:0] add_4_tmp_8; 
   reg[11:0] add_4_tmp; 
   // <<X-HDL>> Unsupported Construct - attribute  (source line 33)
   // <<X-HDL>> Unsupported Construct - attribute  (source line 34))

   assign buff_out0 = d_in ;

   my_fifo_496 ints_fifo_gen_0 (clk, buff_out0, buff_out1, nd);
   my_fifo_496 ints_fifo_gen_1 (clk, buff_out1, buff_out2, nd);
   my_fifo_496 ints_fifo_gen_2 (clk, buff_out2, buff_out3, nd);
   my_fifo_496 ints_fifo_gen_3 (clk, buff_out3, buff_out4, nd);
   my_fifo_496 ints_fifo_gen_4 (clk, buff_out4, buff_out5, nd);
   my_fifo_496 ints_fifo_gen_5 (clk, buff_out5, buff_out6, nd);
   my_fifo_496 ints_fifo_gen_6 (clk, buff_out6, buff_out7, nd);

   always @(posedge clk)
   begin
         if (nd == 1'b1)
         begin
            buff_out_reg1 <= buff_out1 ; 
            buff_out_reg2 <= buff_out2 ; 
            buff_out_reg3 <= buff_out3 ; 
            buff_out_reg4 <= buff_out4 ; 
            buff_out_reg5 <= buff_out5 ; 
            buff_out_reg6 <= buff_out6 ; 
            buff_out_reg7 <= buff_out7 ; 
            d_out_1 <= buff_out_reg1 ; 
            d_out_2 <= add_2_tmp[9:2] ; 
            d_out_4 <= add_4_tmp[11:4] ; 
         end 
         add_2_tmp_1 <= ({2'b00, buff_out_reg1}) + ({2'b00, buff_out_reg3}) ; 
         add_2_tmp_2 <= {1'b0, buff_out_reg2, 1'b0} ; 
         add_2_tmp <= add_2_tmp_1 + add_2_tmp_2 ; 
         add_4_tmp_1 <= ({4'b0000, buff_out_reg1}) + ({4'b0000, buff_out_reg7}) ; 
         add_4_tmp_2 <= ({3'b000, buff_out_reg2, 1'b0}) + ({3'b000, buff_out_reg6, 1'b0}) ; 
         add_4_tmp_3 <= ({3'b000, buff_out_reg3, 1'b0}) + ({3'b000, buff_out_reg5, 1'b0}) ; 
         add_4_tmp_4 <= ({4'b0000, buff_out_reg3}) + ({4'b0000, buff_out_reg5}) ; 
         add_4_tmp_5 <= ({2'b00, buff_out_reg4, 2'b00}) ; 
         add_4_tmp_6 <= add_4_tmp_1 + add_4_tmp_2 ; 
         add_4_tmp_7 <= add_4_tmp_3 + add_4_tmp_4 ; 
         add_4_tmp_8 <= add_4_tmp_5 + add_4_tmp_6 ; 
         add_4_tmp <= add_4_tmp_7 + add_4_tmp_8 ; 
   end 
endmodule
module scl_h_fltr (clk, nd, d_in_1, d_in_2, d_in_4, d_out_1, d_out_2, d_out_4);

   input clk; 
   input nd; 
   input[7:0] d_in_1; 
   input[7:0] d_in_2; 
   input[7:0] d_in_4; 
   output[7:0] d_out_1; 
   reg[7:0] d_out_1;
   output[7:0] d_out_2; 
   reg[7:0] d_out_2;
   output[7:0] d_out_4; 
   reg[7:0] d_out_4;

   wire[7:0] buff_out_20; 
   wire[7:0] buff_out_21; 
   wire[7:0] buff_out_22; 
   wire[7:0] buff_out_23; 
   reg[7:0] buff_out_reg_21; 
   reg[7:0] buff_out_reg_22; 
   reg[7:0] buff_out_reg_23; 
   wire[7:0] buff_out_40; 
   wire[7:0] buff_out_41; 
   wire[7:0] buff_out_42; 
   wire[7:0] buff_out_43; 
   wire[7:0] buff_out_44; 
   wire[7:0] buff_out_45; 
   wire[7:0] buff_out_46; 
   wire[7:0] buff_out_47; 
   reg[7:0] buff_out_reg_41; 
   reg[7:0] buff_out_reg_42; 
   reg[7:0] buff_out_reg_43; 
   reg[7:0] buff_out_reg_44; 
   reg[7:0] buff_out_reg_45; 
   reg[7:0] buff_out_reg_46; 
   reg[7:0] buff_out_reg_47; 
   reg[9:0] add_2_tmp_1; 
   reg[9:0] add_2_tmp_2; 
   reg[9:0] add_2_tmp; 
   reg[11:0] add_4_tmp_1; 
   reg[11:0] add_4_tmp_2; 
   reg[11:0] add_4_tmp_3; 
   reg[11:0] add_4_tmp_4; 
   reg[11:0] add_4_tmp_5; 
   reg[11:0] add_4_tmp_6; 
   reg[11:0] add_4_tmp_7; 
   reg[11:0] add_4_tmp_8; 
   reg[11:0] add_4_tmp; 

   assign buff_out_20 = d_in_2 ;
   assign buff_out_40 = d_in_4 ;

   sh_reg_1  ints_sh_reg_2_0(clk, nd, buff_out_20, buff_out_21); 
   sh_reg_1  ints_sh_reg_2_1(clk, nd, buff_out_21, buff_out_22); 
   sh_reg_1  ints_sh_reg_2_2(clk, nd, buff_out_22, buff_out_23); 
   sh_reg_1  ints_sh_reg_4_0(clk, nd, buff_out_40, buff_out_41); 
   sh_reg_1  ints_sh_reg_4_1(clk, nd, buff_out_41, buff_out_42); 
   sh_reg_1  ints_sh_reg_4_2(clk, nd, buff_out_42, buff_out_43); 
   sh_reg_1  ints_sh_reg_4_3(clk, nd, buff_out_43, buff_out_44); 
   sh_reg_1  ints_sh_reg_4_4(clk, nd, buff_out_44, buff_out_45); 
   sh_reg_1  ints_sh_reg_4_5(clk, nd, buff_out_45, buff_out_46); 
   sh_reg_1  ints_sh_reg_4_6(clk, nd, buff_out_46, buff_out_47); 

   always @(posedge clk)
   begin
         if (nd == 1'b1)
         begin
            buff_out_reg_41 <= buff_out_41 ; 
            buff_out_reg_42 <= buff_out_42 ; 
            buff_out_reg_43 <= buff_out_43 ; 
            buff_out_reg_44 <= buff_out_44 ; 
            buff_out_reg_45 <= buff_out_45 ; 
            buff_out_reg_46 <= buff_out_46 ; 
            buff_out_reg_47 <= buff_out_47 ; 
            buff_out_reg_21 <= buff_out_21 ; 
            buff_out_reg_22 <= buff_out_22 ; 
            buff_out_reg_23 <= buff_out_23 ; 
            d_out_1 <= d_in_1 ; 
            d_out_2 <= add_2_tmp[9:2] ; 
            d_out_4 <= add_4_tmp[11:4] ; 
         end 
         add_2_tmp_1 <= ({2'b00, buff_out_reg_21}) + ({2'b00, buff_out_reg_23}) ; 
         add_2_tmp_2 <= {1'b0, buff_out_reg_22, 1'b0} ; 
         add_2_tmp <= add_2_tmp_1 + add_2_tmp_2 ; 
         add_4_tmp_1 <= ({4'b0000, buff_out_reg_41}) + ({4'b0000, buff_out_reg_47}) ; 
         add_4_tmp_2 <= ({3'b000, buff_out_reg_42, 1'b0}) + ({3'b000, buff_out_reg_46, 1'b0}) ; 
         add_4_tmp_3 <= ({3'b000, buff_out_reg_43, 1'b0}) + ({3'b000, buff_out_reg_45, 1'b0}) ; 
         add_4_tmp_4 <= ({4'b0000, buff_out_reg_43}) + ({4'b0000, buff_out_reg_45}) ; 
         add_4_tmp_5 <= ({2'b00, buff_out_reg_44, 2'b00}) ; 
         add_4_tmp_6 <= add_4_tmp_1 + add_4_tmp_2 ; 
         add_4_tmp_7 <= add_4_tmp_3 + add_4_tmp_4 ; 
         add_4_tmp_8 <= add_4_tmp_5 + add_4_tmp_6 ; 
         add_4_tmp <= add_4_tmp_7 + add_4_tmp_8 ; 
   end 
endmodule
// Discription: this block creates a simple
// shift register
// date: July 27 ,2002
// By:  Ahmad darabiha
// copy from sh_reg.vhd changed to only 1-in-1-out 
module sh_reg_1 (clk, wen, din_1, dout_1);

   input clk; 
   input wen; 
   input[8 - 1:0] din_1; 
   output[8 - 1:0] dout_1; 
   reg[8 - 1:0] dout_1;

   always @(posedge clk)
   begin
         if (wen == 1'b1)
         begin
            dout_1 <= din_1 ; 
         end 
   end 
endmodule
module wrapper_qs_intr_10_20 (clk, 
   din0, 
   din1, 
   din2, 
   din3, 
   din4, 
   din5, 
   din6, 
   din7, 
   din8, 
   din9, 
   din10, 
wen_4, addrin, 
   dout0, 
   dout1, 
   dout2, 
   dout3, 
   dout4, 
   dout5, 
   dout6, 
   dout7, 
   dout8, 
   dout9, 
   dout10, 
   dout11, 
   dout12, 
   dout13, 
   dout14, 
   dout15, 
   dout16, 
   dout17, 
   dout18, 
   dout19, 
   dout20, 
rdy);

   input clk; 
   input[7:0] din0; 
   input[7:0] din1; 
   input[7:0] din2; 
   input[7:0] din3; 
   input[7:0] din4; 
   input[7:0] din5; 
   input[7:0] din6; 
   input[7:0] din7; 
   input[7:0] din8; 
   input[7:0] din9; 
   input[7:0] din10; 
   input wen_4; 
   input[18:0] addrin; 
   output[15:0] dout0; 
   output[15:0] dout1; 
   output[15:0] dout2; 
   output[15:0] dout3; 
   output[15:0] dout4; 
   output[15:0] dout5; 
   output[15:0] dout6; 
   output[15:0] dout7; 
   output[15:0] dout8; 
   output[15:0] dout9; 
   output[15:0] dout10; 
   output[15:0] dout11; 
   output[15:0] dout12; 
   output[15:0] dout13; 
   output[15:0] dout14; 
   output[15:0] dout15; 
   output[15:0] dout16; 
   output[15:0] dout17; 
   output[15:0] dout18; 
   output[15:0] dout19; 
   output[15:0] dout20; 
   wire[15:0] dout0; 
   wire[15:0] dout1; 
   wire[15:0] dout2; 
   wire[15:0] dout3; 
   wire[15:0] dout4; 
   wire[15:0] dout5; 
   wire[15:0] dout6; 
   wire[15:0] dout7; 
   wire[15:0] dout8; 
   wire[15:0] dout9; 
   wire[15:0] dout10; 
   wire[15:0] dout11; 
   wire[15:0] dout12; 
   wire[15:0] dout13; 
   wire[15:0] dout14; 
   wire[15:0] dout15; 
   wire[15:0] dout16; 
   wire[15:0] dout17; 
   wire[15:0] dout18; 
   wire[15:0] dout19; 
   wire[15:0] dout20; 
   output rdy; 
   wire rdy;
   wire[15:0] dout_tmp0; 
   wire[15:0] dout_tmp1; 
   wire[15:0] dout_tmp2; 
   wire[15:0] dout_tmp3; 
   wire[15:0] dout_tmp4; 
   wire[15:0] dout_tmp5; 
   wire[15:0] dout_tmp6; 
   wire[15:0] dout_tmp7; 
   wire[15:0] dout_tmp8; 
   wire[15:0] dout_tmp9; 
   wire[15:0] dout_tmp10; 
   wire[15:0] dout_tmp11; 
   wire[15:0] dout_tmp12; 
   wire[15:0] dout_tmp13; 
   wire[15:0] dout_tmp14; 
   wire[15:0] dout_tmp15; 
   wire[15:0] dout_tmp16; 
   wire[15:0] dout_tmp17; 
   wire[15:0] dout_tmp18; 
   wire[15:0] dout_tmp19; 
   wire[15:0] dout_tmp20; 
   wire[7:0] addr_tmp; 
   wire[15:0] dout_tt; 

	reg [15:0]tmy_ram0;
	reg [15:0]tmy_ram1;
	reg [15:0]tmy_ram2;
	reg [15:0]tmy_ram3;
	reg [15:0]tmy_ram4;
	reg [15:0]tmy_ram5;
	reg [15:0]tmy_ram6;
	reg [15:0]tmy_ram7;
	reg [15:0]tmy_ram8;
	reg [15:0]tmy_ram9;
	reg [15:0]tmy_ram10;
	reg [15:0]tmy_ram11;
	reg [15:0]tmy_ram12;
	reg [15:0]tmy_ram13;
	reg [15:0]tmy_ram14;
	reg [15:0]tmy_ram15;
	reg [15:0]tmy_ram16;
	reg [15:0]tmy_ram17;
	reg [15:0]tmy_ram18;
	reg [15:0]tmy_ram19;
	reg [15:0]tmy_ram20;
	reg [15:0]my_ram0;
	reg [15:0]my_ram1;
	reg [15:0]my_ram2;
	reg [15:0]my_ram3;
	reg [15:0]my_ram4;
	reg [15:0]my_ram5;
	reg [15:0]my_ram6;
	reg [15:0]my_ram7;
	reg [15:0]my_ram8;
	reg [15:0]my_ram9;
	reg [15:0]my_ram10;
	reg [15:0]my_ram11;
	reg [15:0]my_ram12;
	reg [15:0]my_ram13;
	reg [15:0]my_ram14;
	reg [15:0]my_ram15;
	reg [15:0]my_ram16;
	reg [15:0]my_ram17;
	reg [15:0]my_ram18;
	reg [15:0]my_ram19;
	reg [15:0]my_ram20;

	assign rdy = 1'b1;
	assign dout0 = my_ram0;
	assign dout1 = my_ram1;
	assign dout2 = my_ram2;
	assign dout3 = my_ram3;
	assign dout4 = my_ram4;
	assign dout5 = my_ram5;
	assign dout6 = my_ram6;
	assign dout7 = my_ram7;
	assign dout8 = my_ram8;
	assign dout9 = my_ram9;
	assign dout10 = my_ram10;
	assign dout11 = my_ram11;
	assign dout12 = my_ram12;
	assign dout13 = my_ram13;
	assign dout14 = my_ram14;
	assign dout15 = my_ram15;
	assign dout16 = my_ram16;
	assign dout17 = my_ram17;
	assign dout18 = my_ram18;
	assign dout19 = my_ram19;
	assign dout20 = my_ram20;
	always @(posedge clk)
	begin
		if (wen_4 == 1'b1)	
		begin
			tmy_ram0 <= dout_tmp0;
			tmy_ram1 <= dout_tmp1;
			tmy_ram2 <= dout_tmp2;
			tmy_ram3 <= dout_tmp3;
			tmy_ram4 <= dout_tmp4;
			tmy_ram5 <= dout_tmp5;
			tmy_ram6 <= dout_tmp6;
			tmy_ram7 <= dout_tmp7;
			tmy_ram8 <= dout_tmp8;
			tmy_ram9 <= dout_tmp9;
			tmy_ram10 <= dout_tmp10;
			tmy_ram11 <= dout_tmp11;
			tmy_ram12 <= dout_tmp12;
			tmy_ram13 <= dout_tmp13;
			tmy_ram14 <= dout_tmp14;
			tmy_ram15 <= dout_tmp15;
			tmy_ram16 <= dout_tmp16;
			tmy_ram17 <= dout_tmp17;
			tmy_ram18 <= dout_tmp18;
			tmy_ram19 <= dout_tmp19;
			tmy_ram20 <= dout_tmp20;
			my_ram0 <= tmy_ram0;
			my_ram1 <= tmy_ram1;
			my_ram2 <= tmy_ram2;
			my_ram3 <= tmy_ram3;
			my_ram4 <= tmy_ram4;
			my_ram5 <= tmy_ram5;
			my_ram6 <= tmy_ram6;
			my_ram7 <= tmy_ram7;
			my_ram8 <= tmy_ram8;
			my_ram9 <= tmy_ram9;
			my_ram10 <= tmy_ram10;
			my_ram11 <= tmy_ram11;
			my_ram12 <= tmy_ram12;
			my_ram13 <= tmy_ram13;
			my_ram14 <= tmy_ram14;
			my_ram15 <= tmy_ram15;
			my_ram16 <= tmy_ram16;
			my_ram17 <= tmy_ram17;
			my_ram18 <= tmy_ram18;
			my_ram19 <= tmy_ram19;
			my_ram20 <= tmy_ram20;
		end
	end

   assign addr_tmp = addrin;

   quadintr_10_20  my_inst_quadintr(clk, wen_4,
   din0, 
   din1, 
   din2, 
   din3, 
   din4, 
   din5, 
   din6, 
   din7, 
   din8, 
   din9, 
   din10, 
   dout_tmp0, 
   dout_tmp1, 
   dout_tmp2, 
   dout_tmp3, 
   dout_tmp4, 
   dout_tmp5, 
   dout_tmp6, 
   dout_tmp7, 
   dout_tmp8, 
   dout_tmp9, 
   dout_tmp10, 
   dout_tmp11, 
   dout_tmp12, 
   dout_tmp13, 
   dout_tmp14, 
   dout_tmp15, 
   dout_tmp16, 
   dout_tmp17, 
   dout_tmp18, 
   dout_tmp19, 
   dout_tmp20);

endmodule



module wrapper_qs_intr_5_20 (clk,
   din0, 
   din1, 
   din2, 
   din3, 
   din4, 
   din5, 
wen_4, addrin, 
   dout0, 
   dout1, 
   dout2, 
   dout3, 
   dout4, 
   dout5, 
   dout6, 
   dout7, 
   dout8, 
   dout9, 
   dout10, 
   dout11, 
   dout12, 
   dout13, 
   dout14, 
   dout15, 
   dout16, 
   dout17, 
   dout18, 
   dout19, 
   dout20, 
rdy);

   input clk; 
   input[7:0] din0; 
   input[7:0] din1; 
   input[7:0] din2; 
   input[7:0] din3; 
   input[7:0] din4; 
   input[7:0] din5; 
   input wen_4; 
   output[15:0] dout0; 
   output[15:0] dout1; 
   output[15:0] dout2; 
   output[15:0] dout3; 
   output[15:0] dout4; 
   output[15:0] dout5; 
   output[15:0] dout6; 
   output[15:0] dout7; 
   output[15:0] dout8; 
   output[15:0] dout9; 
   output[15:0] dout10; 
   output[15:0] dout11; 
   output[15:0] dout12; 
   output[15:0] dout13; 
   output[15:0] dout14; 
   output[15:0] dout15; 
   output[15:0] dout16; 
   output[15:0] dout17; 
   output[15:0] dout18; 
   output[15:0] dout19; 
   output[15:0] dout20; 
   wire[15:0] dout0; 
   wire[15:0] dout1; 
   wire[15:0] dout2; 
   wire[15:0] dout3; 
   wire[15:0] dout4; 
   wire[15:0] dout5; 
   wire[15:0] dout6; 
   wire[15:0] dout7; 
   wire[15:0] dout8; 
   wire[15:0] dout9; 
   wire[15:0] dout10; 
   wire[15:0] dout11; 
   wire[15:0] dout12; 
   wire[15:0] dout13; 
   wire[15:0] dout14; 
   wire[15:0] dout15; 
   wire[15:0] dout16; 
   wire[15:0] dout17; 
   wire[15:0] dout18; 
   wire[15:0] dout19; 
   wire[15:0] dout20; 
   input[18:0] addrin; 
   output rdy; 
   wire rdy;
   wire[15:0] dout_tmp0; 
   wire[15:0] dout_tmp1; 
   wire[15:0] dout_tmp2; 
   wire[15:0] dout_tmp3; 
   wire[15:0] dout_tmp4; 
   wire[15:0] dout_tmp5; 
   wire[15:0] dout_tmp6; 
   wire[15:0] dout_tmp7; 
   wire[15:0] dout_tmp8; 
   wire[15:0] dout_tmp9; 
   wire[15:0] dout_tmp10; 
   wire[15:0] dout_tmp11; 
   wire[15:0] dout_tmp12; 
   wire[15:0] dout_tmp13; 
   wire[15:0] dout_tmp14; 
   wire[15:0] dout_tmp15; 
   wire[15:0] dout_tmp16; 
   wire[15:0] dout_tmp17; 
   wire[15:0] dout_tmp18; 
   wire[15:0] dout_tmp19; 
   wire[15:0] dout_tmp20; 
   wire[7:0] addr_tmp; 
   wire[15:0] dout_tt; 

	reg [15:0]tmy_ram0;
	reg [15:0]tmy_ram1;
	reg [15:0]tmy_ram2;
	reg [15:0]tmy_ram3;
	reg [15:0]tmy_ram4;
	reg [15:0]tmy_ram5;
	reg [15:0]tmy_ram6;
	reg [15:0]tmy_ram7;
	reg [15:0]tmy_ram8;
	reg [15:0]tmy_ram9;
	reg [15:0]tmy_ram10;
	reg [15:0]tmy_ram11;
	reg [15:0]tmy_ram12;
	reg [15:0]tmy_ram13;
	reg [15:0]tmy_ram14;
	reg [15:0]tmy_ram15;
	reg [15:0]tmy_ram16;
	reg [15:0]tmy_ram17;
	reg [15:0]tmy_ram18;
	reg [15:0]tmy_ram19;
	reg [15:0]tmy_ram20;
	reg [15:0]my_ram0;
	reg [15:0]my_ram1;
	reg [15:0]my_ram2;
	reg [15:0]my_ram3;
	reg [15:0]my_ram4;
	reg [15:0]my_ram5;
	reg [15:0]my_ram6;
	reg [15:0]my_ram7;
	reg [15:0]my_ram8;
	reg [15:0]my_ram9;
	reg [15:0]my_ram10;
	reg [15:0]my_ram11;
	reg [15:0]my_ram12;
	reg [15:0]my_ram13;
	reg [15:0]my_ram14;
	reg [15:0]my_ram15;
	reg [15:0]my_ram16;
	reg [15:0]my_ram17;
	reg [15:0]my_ram18;
	reg [15:0]my_ram19;
	reg [15:0]my_ram20;

	assign rdy = 1'b1;
	assign dout0 = my_ram0;
	assign dout1 = my_ram1;
	assign dout2 = my_ram2;
	assign dout3 = my_ram3;
	assign dout4 = my_ram4;
	assign dout5 = my_ram5;
	assign dout6 = my_ram6;
	assign dout7 = my_ram7;
	assign dout8 = my_ram8;
	assign dout9 = my_ram9;
	assign dout10 = my_ram10;
	assign dout11 = my_ram11;
	assign dout12 = my_ram12;
	assign dout13 = my_ram13;
	assign dout14 = my_ram14;
	assign dout15 = my_ram15;
	assign dout16 = my_ram16;
	assign dout17 = my_ram17;
	assign dout18 = my_ram18;
	assign dout19 = my_ram19;
	assign dout20 = my_ram20;
	always @(posedge clk)
	begin
		if (wen_4 == 1'b1)	
		begin
			tmy_ram0 <= dout_tmp0;
			tmy_ram1 <= dout_tmp1;
			tmy_ram2 <= dout_tmp2;
			tmy_ram3 <= dout_tmp3;
			tmy_ram4 <= dout_tmp4;
			tmy_ram5 <= dout_tmp5;
			tmy_ram6 <= dout_tmp6;
			tmy_ram7 <= dout_tmp7;
			tmy_ram8 <= dout_tmp8;
			tmy_ram9 <= dout_tmp9;
			tmy_ram10 <= dout_tmp10;
			tmy_ram11 <= dout_tmp11;
			tmy_ram12 <= dout_tmp12;
			tmy_ram13 <= dout_tmp13;
			tmy_ram14 <= dout_tmp14;
			tmy_ram15 <= dout_tmp15;
			tmy_ram16 <= dout_tmp16;
			tmy_ram17 <= dout_tmp17;
			tmy_ram18 <= dout_tmp18;
			tmy_ram19 <= dout_tmp19;
			tmy_ram20 <= dout_tmp20;
			my_ram0 <= tmy_ram0;
			my_ram1 <= tmy_ram1;
			my_ram2 <= tmy_ram2;
			my_ram3 <= tmy_ram3;
			my_ram4 <= tmy_ram4;
			my_ram5 <= tmy_ram5;
			my_ram6 <= tmy_ram6;
			my_ram7 <= tmy_ram7;
			my_ram8 <= tmy_ram8;
			my_ram9 <= tmy_ram9;
			my_ram10 <= tmy_ram10;
			my_ram11 <= tmy_ram11;
			my_ram12 <= tmy_ram12;
			my_ram13 <= tmy_ram13;
			my_ram14 <= tmy_ram14;
			my_ram15 <= tmy_ram15;
			my_ram16 <= tmy_ram16;
			my_ram17 <= tmy_ram17;
			my_ram18 <= tmy_ram18;
			my_ram19 <= tmy_ram19;
			my_ram20 <= tmy_ram20;
		end
	end

   assign addr_tmp = {1'b0, addrin} ;


   quadintr_5_20  my_inst_quadintr(clk, wen_4,
   din0, 
   din1, 
   din2, 
   din3, 
   din4, 
   din5, 
   dout_tmp0, 
   dout_tmp1, 
   dout_tmp2, 
   dout_tmp3, 
   dout_tmp4, 
   dout_tmp5, 
   dout_tmp6, 
   dout_tmp7, 
   dout_tmp8, 
   dout_tmp9, 
   dout_tmp10, 
   dout_tmp11, 
   dout_tmp12, 
   dout_tmp13, 
   dout_tmp14, 
   dout_tmp15, 
   dout_tmp16, 
   dout_tmp17, 
   dout_tmp18, 
   dout_tmp19, 
   dout_tmp20);
endmodule



module quadintr_10_20 (clk, new_data,
   din0, 
   din1, 
   din2, 
   din3, 
   din4, 
   din5, 
   din6, 
   din7, 
   din8, 
   din9, 
   din10, 
   dout0, 
   dout1, 
   dout2, 
   dout3, 
   dout4, 
   dout5, 
   dout6, 
   dout7, 
   dout8, 
   dout9, 
   dout10, 
   dout11, 
   dout12, 
   dout13, 
   dout14, 
   dout15, 
   dout16, 
   dout17, 
   dout18, 
   dout19, 
   dout20); 


   input clk; 
   input new_data; 
   input[7:0] din0; 
   input[7:0] din1; 
   input[7:0] din2; 
   input[7:0] din3; 
   input[7:0] din4; 
   input[7:0] din5; 
   input[7:0] din6; 
   input[7:0] din7; 
   input[7:0] din8; 
   input[7:0] din9; 
   input[7:0] din10; 
   reg[7:0] dinr0; 
   reg[7:0] dinr1; 
   reg[7:0] dinr2; 
   reg[7:0] dinr3; 
   reg[7:0] dinr4; 
   reg[7:0] dinr5; 
   reg[7:0] dinr6; 
   reg[7:0] dinr7; 
   reg[7:0] dinr8; 
   reg[7:0] dinr9; 
   reg[7:0] dinr10; 

   output[15:0] dout0; 
   output[15:0] dout1; 
   output[15:0] dout2; 
   output[15:0] dout3; 
   output[15:0] dout4; 
   output[15:0] dout5; 
   output[15:0] dout6; 
   output[15:0] dout7; 
   output[15:0] dout8; 
   output[15:0] dout9; 
   output[15:0] dout10; 
   output[15:0] dout11; 
   output[15:0] dout12; 
   output[15:0] dout13; 
   output[15:0] dout14; 
   output[15:0] dout15; 
   output[15:0] dout16; 
   output[15:0] dout17; 
   output[15:0] dout18; 
   output[15:0] dout19; 
   output[15:0] dout20; 
   reg[15:0] dout0; 
   reg[15:0] dout1; 
   reg[15:0] dout2; 
   reg[15:0] dout3; 
   reg[15:0] dout4; 
   reg[15:0] dout5; 
   reg[15:0] dout6; 
   reg[15:0] dout7; 
   reg[15:0] dout8; 
   reg[15:0] dout9; 
   reg[15:0] dout10; 
   reg[15:0] dout11; 
   reg[15:0] dout12; 
   reg[15:0] dout13; 
   reg[15:0] dout14; 
   reg[15:0] dout15; 
   reg[15:0] dout16; 
   reg[15:0] dout17; 
   reg[15:0] dout18; 
   reg[15:0] dout19; 
   reg[15:0] dout20; 

   reg[7:0] tmp_10; 
   reg[7:0] tmp_11; 
   reg[7:0] tmp_12; 
   reg[7:0] tmp_13; 
   reg[7:0] tmp_14; 
   reg[7:0] tmp_15; 
   reg[7:0] tmp_16; 
   reg[7:0] tmp_17; 
   reg[7:0] tmp_18; 
   reg[7:0] tmp_19; 
   reg[7:0] tmp_110; 
   reg[7:0] tmp_111; 
   reg[7:0] tmp_112; 
   reg[7:0] tmp_113; 
   reg[7:0] tmp_114; 
   reg[7:0] tmp_115; 
   reg[7:0] tmp_116; 
   reg[7:0] tmp_117; 
   reg[7:0] tmp_118; 
   reg[7:0] tmp_119; 
   reg[7:0] tmp_120; 
   reg[7:0] tmp_20; 
   reg[7:0] tmp_21; 
   reg[7:0] tmp_22; 
   reg[7:0] tmp_23; 
   reg[7:0] tmp_24; 
   reg[7:0] tmp_25; 
   reg[7:0] tmp_26; 
   reg[7:0] tmp_27; 
   reg[7:0] tmp_28; 
   reg[7:0] tmp_29; 
   reg[7:0] tmp_210; 
   reg[7:0] tmp_211; 
   reg[7:0] tmp_212; 
   reg[7:0] tmp_213; 
   reg[7:0] tmp_214; 
   reg[7:0] tmp_215; 
   reg[7:0] tmp_216; 
   reg[7:0] tmp_217; 
   reg[7:0] tmp_218; 
   reg[7:0] tmp_219; 
   reg[7:0] tmp_220; 
   reg[7:0] tmp_30; 
   reg[7:0] tmp_31; 
   reg[7:0] tmp_32; 
   reg[7:0] tmp_33; 
   reg[7:0] tmp_34; 
   reg[7:0] tmp_35; 
   reg[7:0] tmp_36; 
   reg[7:0] tmp_37; 
   reg[7:0] tmp_38; 
   reg[7:0] tmp_39; 
   reg[7:0] tmp_310; 
   reg[7:0] tmp_311; 
   reg[7:0] tmp_312; 
   reg[7:0] tmp_313; 
   reg[7:0] tmp_314; 
   reg[7:0] tmp_315; 
   reg[7:0] tmp_316; 
   reg[7:0] tmp_317; 
   reg[7:0] tmp_318; 
   reg[7:0] tmp_319; 
   reg[7:0] tmp_320; 
   reg[8:0] add_tmp0; 
   reg[8:0] add_tmp1; 
   reg[8:0] add_tmp2; 
   reg[8:0] add_tmp3; 
   reg[8:0] add_tmp4; 
   reg[8:0] add_tmp5; 
   reg[8:0] add_tmp6; 
   reg[8:0] add_tmp7; 
   reg[8:0] add_tmp8; 
   reg[8:0] add_tmp9; 
   reg[8:0] add_tmp10; 
   reg[8:0] add_tmp11; 
   reg[8:0] add_tmp12; 
   reg[8:0] add_tmp13; 
   reg[8:0] add_tmp14; 
   reg[8:0] add_tmp15; 
   reg[8:0] add_tmp16; 
   reg[8:0] add_tmp17; 
   reg[8:0] add_tmp18; 
   reg[8:0] add_tmp19; 
   reg[8:0] add_tmp20; 
   reg[8:0] doutr0; 
   reg[8:0] doutr1; 
   reg[8:0] doutr2; 
   reg[8:0] doutr3; 
   reg[8:0] doutr4; 
   reg[8:0] doutr5; 
   reg[8:0] doutr6; 
   reg[8:0] doutr7; 
   reg[8:0] doutr8; 
   reg[8:0] doutr9; 
   reg[8:0] doutr10; 
   reg[8:0] doutr11; 
   reg[8:0] doutr12; 
   reg[8:0] doutr13; 
   reg[8:0] doutr14; 
   reg[8:0] doutr15; 
   reg[8:0] doutr16; 
   reg[8:0] doutr17; 
   reg[8:0] doutr18; 
   reg[8:0] doutr19; 
   reg[8:0] doutr20; 

   always @(posedge clk)
   begin
         if (new_data == 1'b1)
         begin
			dinr0 <= din0;
			dinr1 <= din1;
			dinr2 <= din2;
			dinr3 <= din3;
			dinr4 <= din4;
			dinr5 <= din5;
			dinr6 <= din6;
			dinr7 <= din7;
			dinr8 <= din8;
			dinr9 <= din9;
			dinr10 <= din10;

			dout0[15:9] <= {doutr0[8],doutr0[8],doutr0[8],doutr0[8],doutr0[8],doutr0[8],doutr0[8]};
			dout1[15:9] <= {doutr1[8],doutr1[8],doutr1[8],doutr1[8],doutr1[8],doutr1[8],doutr1[8]};
			dout2[15:9] <= {doutr2[8],doutr2[8],doutr2[8],doutr2[8],doutr2[8],doutr2[8],doutr2[8]};
			dout3[15:9] <= {doutr3[8],doutr3[8],doutr3[8],doutr3[8],doutr3[8],doutr3[8],doutr3[8]};
			dout4[15:9] <= {doutr4[8],doutr4[8],doutr4[8],doutr4[8],doutr4[8],doutr4[8],doutr4[8]};
			dout5[15:9] <= {doutr5[8],doutr5[8],doutr5[8],doutr5[8],doutr5[8],doutr5[8],doutr5[8]};
			dout6[15:9] <= {doutr6[8],doutr6[8],doutr6[8],doutr6[8],doutr6[8],doutr6[8],doutr6[8]};
			dout7[15:9] <= {doutr7[8],doutr7[8],doutr7[8],doutr7[8],doutr7[8],doutr7[8],doutr7[8]};
			dout8[15:9] <= {doutr8[8],doutr8[8],doutr8[8],doutr8[8],doutr8[8],doutr8[8],doutr8[8]};
			dout9[15:9] <= {doutr9[8],doutr9[8],doutr9[8],doutr9[8],doutr9[8],doutr9[8],doutr9[8]};
			dout10[15:9] <= {doutr10[8],doutr10[8],doutr10[8],doutr10[8],doutr10[8],doutr10[8],doutr10[8]};
			dout11[15:9] <= {doutr11[8],doutr11[8],doutr11[8],doutr11[8],doutr11[8],doutr11[8],doutr11[8]};
			dout12[15:9] <= {doutr12[8],doutr12[8],doutr12[8],doutr12[8],doutr12[8],doutr12[8],doutr12[8]};
			dout13[15:9] <= {doutr13[8],doutr13[8],doutr13[8],doutr13[8],doutr13[8],doutr13[8],doutr13[8]};
			dout14[15:9] <= {doutr14[8],doutr14[8],doutr14[8],doutr14[8],doutr14[8],doutr14[8],doutr14[8]};
			dout15[15:9] <= {doutr15[8],doutr15[8],doutr15[8],doutr15[8],doutr15[8],doutr15[8],doutr15[8]};
			dout16[15:9] <= {doutr16[8],doutr16[8],doutr16[8],doutr16[8],doutr16[8],doutr16[8],doutr16[8]};
			dout17[15:9] <= {doutr17[8],doutr17[8],doutr17[8],doutr17[8],doutr17[8],doutr17[8],doutr17[8]};
			dout18[15:9] <= {doutr18[8],doutr18[8],doutr18[8],doutr18[8],doutr18[8],doutr18[8],doutr18[8]};
			dout19[15:9] <= {doutr19[8],doutr19[8],doutr19[8],doutr19[8],doutr19[8],doutr19[8],doutr19[8]};
			dout20[15:9] <= {doutr20[8],doutr20[8],doutr20[8],doutr20[8],doutr20[8],doutr20[8],doutr20[8]};
			dout0[8:0] <= doutr0;
			dout1[8:0] <= doutr1;
			dout2[8:0] <= doutr2;
			dout3[8:0] <= doutr3;
			dout4[8:0] <= doutr4;
			dout5[8:0] <= doutr5;
			dout6[8:0] <= doutr6;
			dout7[8:0] <= doutr7;
			dout8[8:0] <= doutr8;
			dout9[8:0] <= doutr9;
			dout10[8:0] <= doutr10;
			dout11[8:0] <= doutr11;
			dout12[8:0] <= doutr12;
			dout13[8:0] <= doutr13;
			dout14[8:0] <= doutr14;
			dout15[8:0] <= doutr15;
			dout16[8:0] <= doutr16;
			dout17[8:0] <= doutr17;
			dout18[8:0] <= doutr18;
			dout19[8:0] <= doutr19;
			dout20[8:0] <= doutr20;
         end 
				doutr0 <= {dinr0[8-1], dinr0};
				doutr2 <= {dinr1[8-1], dinr1};
				doutr4 <= {dinr2[8-1], dinr2};
				doutr6 <= {dinr3[8-1], dinr3};
				doutr8 <= {dinr4[8-1], dinr4};
				doutr10 <= {dinr5[8-1], dinr5};
				doutr12 <= {dinr6[8-1], dinr6};
				doutr14 <= {dinr7[8-1], dinr7};
				doutr16 <= {dinr8[8-1], dinr8};

				tmp_11 <= {dinr0[8-1], dinr0[8-1], dinr0[8-1:2]} + {dinr0[8-1], dinr0[8-1], dinr0[8-1], dinr0[8-1:3]};
				tmp_13 <= {dinr1[8-1], dinr1[8-1], dinr1[8-1:2]} + {dinr1[8-1], dinr1[8-1], dinr1[8-1], dinr1[8-1:3]};
				tmp_15 <= {dinr2[8-1], dinr2[8-1], dinr2[8-1:2]} + {dinr2[8-1], dinr2[8-1], dinr2[8-1], dinr2[8-1:3]};
				tmp_17 <= {dinr3[8-1], dinr3[8-1], dinr3[8-1:2]} + {dinr3[8-1], dinr3[8-1], dinr3[8-1], dinr3[8-1:3]};
				tmp_19 <= {dinr4[8-1], dinr4[8-1], dinr4[8-1:2]} + {dinr4[8-1], dinr4[8-1], dinr4[8-1], dinr4[8-1:3]};
				tmp_111 <= {dinr5[8-1], dinr5[8-1], dinr5[8-1:2]} + {dinr5[8-1], dinr5[8-1], dinr5[8-1], dinr5[8-1:3]};
				tmp_113 <= {dinr6[8-1], dinr6[8-1], dinr6[8-1:2]} + {dinr6[8-1], dinr6[8-1], dinr6[8-1], dinr6[8-1:3]};
				tmp_115 <= {dinr7[8-1], dinr7[8-1], dinr7[8-1:2]} + {dinr7[8-1], dinr7[8-1], dinr7[8-1], dinr7[8-1:3]};
				tmp_117 <= {dinr8[8-1], dinr8[8-1], dinr8[8-1:2]} + {dinr8[8-1], dinr8[8-1], dinr8[8-1], dinr8[8-1:3]};
				
				tmp_21 <= {dinr1[8-1], dinr1[8-1:1]} + {dinr1[8-1], dinr1[8-1], dinr1[8-1:2]};
				tmp_23 <= {dinr2[8-1], dinr2[8-1:1]} + {dinr2[8-1], dinr2[8-1], dinr2[8-1:2]};
				tmp_25 <= {dinr3[8-1], dinr3[8-1:1]} + {dinr3[8-1], dinr3[8-1], dinr3[8-1:2]};
				tmp_27 <= {dinr4[8-1], dinr4[8-1:1]} + {dinr4[8-1], dinr4[8-1], dinr4[8-1:2]};
				tmp_29 <= {dinr5[8-1], dinr5[8-1:1]} + {dinr5[8-1], dinr5[8-1], dinr5[8-1:2]};
				tmp_211 <= {dinr6[8-1], dinr6[8-1:1]} + {dinr6[8-1], dinr6[8-1], dinr6[8-1:2]};
				tmp_213 <= {dinr7[8-1], dinr7[8-1:1]} + {dinr7[8-1], dinr7[8-1], dinr7[8-1:2]};
				tmp_215 <= {dinr8[8-1], dinr8[8-1:1]} + {dinr8[8-1], dinr8[8-1], dinr8[8-1:2]};
				tmp_217 <= {dinr9[8-1], dinr9[8-1:1]} + {dinr9[8-1], dinr9[8-1], dinr9[8-1:2]};
				
				tmp_31 <= {dinr2[8-1], dinr2[8-1], dinr2[8-1], dinr2[8-1:3]};
				tmp_33 <= {dinr3[8-1], dinr3[8-1], dinr3[8-1], dinr3[8-1:3]};
				tmp_35 <= {dinr4[8-1], dinr4[8-1], dinr4[8-1], dinr4[8-1:3]};
				tmp_37 <= {dinr5[8-1], dinr5[8-1], dinr5[8-1], dinr5[8-1:3]};
				tmp_39 <= {dinr6[8-1], dinr6[8-1], dinr6[8-1], dinr6[8-1:3]};
				tmp_311 <= {dinr7[8-1], dinr7[8-1], dinr7[8-1], dinr7[8-1:3]};
				tmp_313 <= {dinr8[8-1], dinr8[8-1], dinr8[8-1], dinr8[8-1:3]};
				tmp_315 <= {dinr9[8-1], dinr9[8-1], dinr9[8-1], dinr9[8-1:3]};
				tmp_317 <= {dinr10[8-1], dinr10[8-1], dinr10[8-1], dinr10[8-1:3]};

				add_tmp1 <= {tmp_11[8-1], tmp_11} + {tmp_21[8-1], tmp_21};
				add_tmp3 <= {tmp_13[8-1], tmp_13} + {tmp_23[8-1], tmp_23};
				add_tmp5 <= {tmp_15[8-1], tmp_15} + {tmp_25[8-1], tmp_25};
				add_tmp7 <= {tmp_17[8-1], tmp_17} + {tmp_27[8-1], tmp_27};
				add_tmp9 <= {tmp_19[8-1], tmp_19} + {tmp_29[8-1], tmp_29};
				add_tmp11 <= {tmp_111[8-1], tmp_111} + {tmp_211[8-1], tmp_211};
				add_tmp13 <= {tmp_113[8-1], tmp_113} + {tmp_213[8-1], tmp_213};
				add_tmp15 <= {tmp_115[8-1], tmp_115} + {tmp_215[8-1], tmp_215};
				add_tmp17 <= {tmp_117[8-1], tmp_117} + {tmp_217[8-1], tmp_217};

				doutr1 <= add_tmp1 - {tmp_31[8-1], tmp_31};
				doutr3 <= add_tmp3 - {tmp_33[8-1], tmp_33};
				doutr5 <= add_tmp5 - {tmp_35[8-1], tmp_35};
				doutr7 <= add_tmp7 - {tmp_37[8-1], tmp_37};
				doutr9 <= add_tmp9 - {tmp_39[8-1], tmp_39};
				doutr11 <= add_tmp11 - {tmp_311[8-1], tmp_311};
				doutr13 <= add_tmp13 - {tmp_313[8-1], tmp_313};
				doutr15 <= add_tmp15 - {tmp_315[8-1], tmp_315};
				doutr17 <= add_tmp17 - {tmp_317[8-1], tmp_317};

		doutr18 <= {dinr9[8-1] , dinr9};

		tmp_119 <= {dinr8[8-1], dinr8[8-1] , dinr8[8-1] , dinr8[8-1 : 3]};

		tmp_219 <= {dinr9[8-1], dinr9[8-1 : 1]} + {dinr9[8-1], dinr9[8-1] , dinr9[8-1 : 2]};
				
		tmp_319 <= {dinr10[8-1] , dinr10[8-1] , dinr10[8-1 : 2]} + {dinr10[8-1] , dinr10[8-1] , dinr10[8-1] , dinr10[8-1 : 3]};

		add_tmp19 <= {tmp_219[8-1] , tmp_219} + {tmp_319[8-1] , tmp_319};

		doutr19 <= add_tmp19 - {tmp_119[8-1] , tmp_119};

		doutr20 <= {dinr10[8-1] , dinr10};
   end 
endmodule



module quadintr_5_20 (clk, new_data,
   din0,
   din1,
   din2,
   din3,
   din4,
   din5,
   dout0,
   dout1,
   dout2,
   dout3,
   dout4,
   dout5,
   dout6,
   dout7,
   dout8,
   dout9,
   dout10,
   dout11,
   dout12,
   dout13,
   dout14,
   dout15,
   dout16,
   dout17,
   dout18,
   dout19,
   dout20); 

   input clk; 
   input new_data; 
   input[7:0] din0; 
   input[7:0] din1; 
   input[7:0] din2; 
   input[7:0] din3; 
   input[7:0] din4; 
   input[7:0] din5; 
   reg[7:0] dinr0; 
   reg[7:0] dinr1; 
   reg[7:0] dinr2; 
   reg[7:0] dinr3; 
   reg[7:0] dinr4; 
   reg[7:0] dinr5; 
   output[15:0] dout0; 
   output[15:0] dout1; 
   output[15:0] dout2; 
   output[15:0] dout3; 
   output[15:0] dout4; 
   output[15:0] dout5; 
   output[15:0] dout6; 
   output[15:0] dout7; 
   output[15:0] dout8; 
   output[15:0] dout9; 
   output[15:0] dout10; 
   output[15:0] dout11; 
   output[15:0] dout12; 
   output[15:0] dout13; 
   output[15:0] dout14; 
   output[15:0] dout15; 
   output[15:0] dout16; 
   output[15:0] dout17; 
   output[15:0] dout18; 
   output[15:0] dout19; 
   output[15:0] dout20; 
   reg[15:0] dout0; 
   reg[15:0] dout1; 
   reg[15:0] dout2; 
   reg[15:0] dout3; 
   reg[15:0] dout4; 
   reg[15:0] dout5; 
   reg[15:0] dout6; 
   reg[15:0] dout7; 
   reg[15:0] dout8; 
   reg[15:0] dout9; 
   reg[15:0] dout10; 
   reg[15:0] dout11; 
   reg[15:0] dout12; 
   reg[15:0] dout13; 
   reg[15:0] dout14; 
   reg[15:0] dout15; 
   reg[15:0] dout16; 
   reg[15:0] dout17; 
   reg[15:0] dout18; 
   reg[15:0] dout19; 
   reg[15:0] dout20; 

   reg[7:0] tmp_10; 
   reg[7:0] tmp_11; 
   reg[7:0] tmp_12; 
   reg[7:0] tmp_13; 
   reg[7:0] tmp_14; 
   reg[7:0] tmp_15; 
   reg[7:0] tmp_16; 
   reg[7:0] tmp_17; 
   reg[7:0] tmp_18; 
   reg[7:0] tmp_19; 
   reg[7:0] tmp_110; 
   reg[7:0] tmp_20; 
   reg[7:0] tmp_21; 
   reg[7:0] tmp_22; 
   reg[7:0] tmp_23; 
   reg[7:0] tmp_24; 
   reg[7:0] tmp_25; 
   reg[7:0] tmp_26; 
   reg[7:0] tmp_27; 
   reg[7:0] tmp_28; 
   reg[7:0] tmp_29; 
   reg[7:0] tmp_210; 
   reg[7:0] tmp_30; 
   reg[7:0] tmp_31; 
   reg[7:0] tmp_32; 
   reg[7:0] tmp_33; 
   reg[7:0] tmp_34; 
   reg[7:0] tmp_35; 
   reg[7:0] tmp_36; 
   reg[7:0] tmp_37; 
   reg[7:0] tmp_38; 
   reg[7:0] tmp_39; 
   reg[7:0] tmp_310; 
   reg[8:0] add_tmp0; 
   reg[8:0] add_tmp1; 
   reg[8:0] add_tmp2; 
   reg[8:0] add_tmp3; 
   reg[8:0] add_tmp4; 
   reg[8:0] add_tmp5; 
   reg[8:0] add_tmp6; 
   reg[8:0] add_tmp7; 
   reg[8:0] add_tmp8; 
   reg[8:0] add_tmp9; 
   reg[8:0] add_tmp10; 
   reg[8:0] doutr0; 
   reg[8:0] doutr1; 
   reg[8:0] doutr2; 
   reg[8:0] doutr3; 
   reg[8:0] doutr4; 
   reg[8:0] doutr5; 
   reg[8:0] doutr6; 
   reg[8:0] doutr7; 
   reg[8:0] doutr8; 
   reg[8:0] doutr9; 
   reg[8:0] doutr10; 
   reg[8:0] doutr11; 
   reg[8:0] doutr12; 
   reg[8:0] doutr13; 
   reg[8:0] doutr14; 
   reg[8:0] doutr15; 
   reg[8:0] doutr16; 
   reg[8:0] doutr17; 
   reg[8:0] doutr18; 
   reg[8:0] doutr19; 
   reg[8:0] doutr20; 


   reg[8:0] tmp_1_100; 
   reg[8:0] tmp_1_101; 
   reg[8:0] tmp_1_102; 
   reg[8:0] tmp_1_103; 
   reg[8:0] tmp_1_104; 
   reg[8:0] tmp_1_105; 
   reg[8:0] tmp_1_106; 
   reg[8:0] tmp_1_107; 
   reg[8:0] tmp_1_108; 
   reg[8:0] tmp_1_109; 
   reg[8:0] tmp_1_1010; 
   reg[8:0] tmp_1_1011; 
   reg[8:0] tmp_1_1012; 
   reg[8:0] tmp_1_1013; 
   reg[8:0] tmp_1_1014; 
   reg[8:0] tmp_1_1015; 
   reg[8:0] tmp_1_1016; 
   reg[8:0] tmp_1_1017; 
   reg[8:0] tmp_1_1018; 
   reg[8:0] tmp_1_1019; 
   reg[8:0] tmp_1_1020; 
   reg[8:0] tmp_2_100; 
   reg[8:0] tmp_2_101; 
   reg[8:0] tmp_2_102; 
   reg[8:0] tmp_2_103; 
   reg[8:0] tmp_2_104; 
   reg[8:0] tmp_2_105; 
   reg[8:0] tmp_2_106; 
   reg[8:0] tmp_2_107; 
   reg[8:0] tmp_2_108; 
   reg[8:0] tmp_2_109; 
   reg[8:0] tmp_2_1010; 
   reg[8:0] tmp_2_1011; 
   reg[8:0] tmp_2_1012; 
   reg[8:0] tmp_2_1013; 
   reg[8:0] tmp_2_1014; 
   reg[8:0] tmp_2_1015; 
   reg[8:0] tmp_2_1016; 
   reg[8:0] tmp_2_1017; 
   reg[8:0] tmp_2_1018; 
   reg[8:0] tmp_2_1019; 
   reg[8:0] tmp_2_1020; 
   reg[8:0] tmp_3_100; 
   reg[8:0] tmp_3_101; 
   reg[8:0] tmp_3_102; 
   reg[8:0] tmp_3_103; 
   reg[8:0] tmp_3_104; 
   reg[8:0] tmp_3_105; 
   reg[8:0] tmp_3_106; 
   reg[8:0] tmp_3_107; 
   reg[8:0] tmp_3_108; 
   reg[8:0] tmp_3_109; 
   reg[8:0] tmp_3_1010; 
   reg[8:0] tmp_3_1011; 
   reg[8:0] tmp_3_1012; 
   reg[8:0] tmp_3_1013; 
   reg[8:0] tmp_3_1014; 
   reg[8:0] tmp_3_1015; 
   reg[8:0] tmp_3_1016; 
   reg[8:0] tmp_3_1017; 
   reg[8:0] tmp_3_1018; 
   reg[8:0] tmp_3_1019; 
   reg[8:0] tmp_3_1020; 
   reg[8:0] add_tmp_100; 
   reg[8:0] add_tmp_101; 
   reg[8:0] add_tmp_102; 
   reg[8:0] add_tmp_103; 
   reg[8:0] add_tmp_104; 
   reg[8:0] add_tmp_105; 
   reg[8:0] add_tmp_106; 
   reg[8:0] add_tmp_107; 
   reg[8:0] add_tmp_108; 
   reg[8:0] add_tmp_109; 
   reg[8:0] add_tmp_1010; 
   reg[8:0] add_tmp_1011; 
   reg[8:0] add_tmp_1012; 
   reg[8:0] add_tmp_1013; 
   reg[8:0] add_tmp_1014; 
   reg[8:0] add_tmp_1015; 
   reg[8:0] add_tmp_1016; 
   reg[8:0] add_tmp_1017; 
   reg[8:0] add_tmp_1018; 
   reg[8:0] add_tmp_1019; 
   reg[8:0] add_tmp_1020; 

   reg[8:0] doutr_100; 
   reg[8:0] doutr_101; 
   reg[8:0] doutr_102; 
   reg[8:0] doutr_103; 
   reg[8:0] doutr_104; 
   reg[8:0] doutr_105; 
   reg[8:0] doutr_106; 
   reg[8:0] doutr_107; 
   reg[8:0] doutr_108; 
   reg[8:0] doutr_109; 
   reg[8:0] doutr_1010; 

   always @(posedge clk)
   begin
         if (new_data == 1'b1)
         begin
			dinr0 <= din0;
			dinr1 <= din1;
			dinr2 <= din2;
			dinr3 <= din3;
			dinr4 <= din4;
			dinr5 <= din5;

			dout0[15:9] <= {doutr0[8],doutr0[8],doutr0[8],doutr0[8],doutr0[8],doutr0[8],doutr0[8]};
			dout1[15:9] <= {doutr1[8],doutr1[8],doutr1[8],doutr1[8],doutr1[8],doutr1[8],doutr1[8]};
			dout2[15:9] <= {doutr2[8],doutr2[8],doutr2[8],doutr2[8],doutr2[8],doutr2[8],doutr2[8]};
			dout3[15:9] <= {doutr3[8],doutr3[8],doutr3[8],doutr3[8],doutr3[8],doutr3[8],doutr3[8]};
			dout4[15:9] <= {doutr4[8],doutr4[8],doutr4[8],doutr4[8],doutr4[8],doutr4[8],doutr4[8]};
			dout5[15:9] <= {doutr5[8],doutr5[8],doutr5[8],doutr5[8],doutr5[8],doutr5[8],doutr5[8]};
			dout6[15:9] <= {doutr6[8],doutr6[8],doutr6[8],doutr6[8],doutr6[8],doutr6[8],doutr6[8]};
			dout7[15:9] <= {doutr7[8],doutr7[8],doutr7[8],doutr7[8],doutr7[8],doutr7[8],doutr7[8]};
			dout8[15:9] <= {doutr8[8],doutr8[8],doutr8[8],doutr8[8],doutr8[8],doutr8[8],doutr8[8]};
			dout9[15:9] <= {doutr9[8],doutr9[8],doutr9[8],doutr9[8],doutr9[8],doutr9[8],doutr9[8]};
			dout10[15:9] <= {doutr10[8],doutr10[8],doutr10[8],doutr10[8],doutr10[8],doutr10[8],doutr10[8]};
			dout11[15:9] <= {doutr11[8],doutr11[8],doutr11[8],doutr11[8],doutr11[8],doutr11[8],doutr11[8]};
			dout12[15:9] <= {doutr12[8],doutr12[8],doutr12[8],doutr12[8],doutr12[8],doutr12[8],doutr12[8]};
			dout13[15:9] <= {doutr13[8],doutr13[8],doutr13[8],doutr13[8],doutr13[8],doutr13[8],doutr13[8]};
			dout14[15:9] <= {doutr14[8],doutr14[8],doutr14[8],doutr14[8],doutr14[8],doutr14[8],doutr14[8]};
			dout15[15:9] <= {doutr15[8],doutr15[8],doutr15[8],doutr15[8],doutr15[8],doutr15[8],doutr15[8]};
			dout16[15:9] <= {doutr16[8],doutr16[8],doutr16[8],doutr16[8],doutr16[8],doutr16[8],doutr16[8]};
			dout17[15:9] <= {doutr17[8],doutr17[8],doutr17[8],doutr17[8],doutr17[8],doutr17[8],doutr17[8]};
			dout18[15:9] <= {doutr18[8],doutr18[8],doutr18[8],doutr18[8],doutr18[8],doutr18[8],doutr18[8]};
			dout19[15:9] <= {doutr19[8],doutr19[8],doutr19[8],doutr19[8],doutr19[8],doutr19[8],doutr19[8]};
			dout20[15:9] <= {doutr20[8],doutr20[8],doutr20[8],doutr20[8],doutr20[8],doutr20[8],doutr20[8]};
			dout0[8:0] <= doutr0;
			dout1[8:0] <= doutr1;
			dout2[8:0] <= doutr2;
			dout3[8:0] <= doutr3;
			dout4[8:0] <= doutr4;
			dout5[8:0] <= doutr5;
			dout6[8:0] <= doutr6;
			dout7[8:0] <= doutr7;
			dout8[8:0] <= doutr8;
			dout9[8:0] <= doutr9;
			dout10[8:0] <= doutr10;
			dout11[8:0] <= doutr11;
			dout12[8:0] <= doutr12;
			dout13[8:0] <= doutr13;
			dout14[8:0] <= doutr14;
			dout15[8:0] <= doutr15;
			dout16[8:0] <= doutr16;
			dout17[8:0] <= doutr17;
			dout18[8:0] <= doutr18;
			dout19[8:0] <= doutr19;
			dout20[8:0] <= doutr20;
		end

				doutr_100 <= {dinr0[8-1], dinr0};
				doutr_102 <= {dinr1[8-1], dinr1};
				doutr_104 <= {dinr2[8-1], dinr2};
				doutr_106 <= {dinr3[8-1], dinr3};

				tmp_11 <= {dinr0[8-1], dinr0[8-1], dinr0[8-1:2]} + {dinr0[8-1], dinr0[8-1], dinr0[8-1], dinr0[8-1:3]};
				tmp_13 <= {dinr1[8-1], dinr1[8-1], dinr1[8-1:2]} + {dinr1[8-1], dinr1[8-1], dinr1[8-1], dinr1[8-1:3]};
				tmp_15 <= {dinr2[8-1], dinr2[8-1], dinr2[8-1:2]} + {dinr2[8-1], dinr2[8-1], dinr2[8-1], dinr2[8-1:3]};
				tmp_17 <= {dinr3[8-1], dinr3[8-1], dinr3[8-1:2]} + {dinr3[8-1], dinr3[8-1], dinr3[8-1], dinr3[8-1:3]};
				
				tmp_21 <= {dinr1[8-1], dinr1[8-1:1]} + {dinr1[8-1], dinr1[8-1], dinr1[8-1:2]};
				tmp_23 <= {dinr2[8-1], dinr2[8-1:1]} + {dinr2[8-1], dinr2[8-1], dinr2[8-1:2]};
				tmp_25 <= {dinr3[8-1], dinr3[8-1:1]} + {dinr3[8-1], dinr3[8-1], dinr3[8-1:2]};
				tmp_27 <= {dinr4[8-1], dinr4[8-1:1]} + {dinr4[8-1], dinr4[8-1], dinr4[8-1:2]};
				
				tmp_31 <= {dinr2[8-1], dinr2[8-1], dinr2[8-1], dinr2[8-1:3]};
				tmp_33 <= {dinr3[8-1], dinr3[8-1], dinr3[8-1], dinr3[8-1:3]};
				tmp_35 <= {dinr4[8-1], dinr4[8-1], dinr4[8-1], dinr4[8-1:3]};
				tmp_37 <= {dinr5[8-1], dinr5[8-1], dinr5[8-1], dinr5[8-1:3]};

				add_tmp1 <= {tmp_11[8-1], tmp_11} + {tmp_21[8-1], tmp_21};
				add_tmp3 <= {tmp_13[8-1], tmp_13} + {tmp_23[8-1], tmp_23};
				add_tmp5 <= {tmp_15[8-1], tmp_15} + {tmp_25[8-1], tmp_25};
				add_tmp7 <= {tmp_17[8-1], tmp_17} + {tmp_27[8-1], tmp_27};

				doutr_101 <= add_tmp1 - {tmp_31[8-1], tmp_31};
				doutr_103 <= add_tmp3 - {tmp_33[8-1], tmp_33};
				doutr_105 <= add_tmp5 - {tmp_35[8-1], tmp_35};
				doutr_107 <= add_tmp7 - {tmp_37[8-1], tmp_37};

		doutr_108 <= {dinr4[8-1] , dinr4};

		tmp_19 <= {dinr3[8-1], dinr3[8-1] , dinr3[8-1] , dinr3[8-1:3]};

		tmp_29 <= {dinr4[8-1], dinr4[8-1:1]} +
				{dinr4[8-1], dinr4[8-1] , dinr4[8-1:2]};

		tmp_39 <= {dinr5[8-1] , dinr5[8-1] , dinr5[8-1:2]} +
			{dinr5[8-1] , dinr5[8-1] , dinr5[8-1] , dinr5[8-1:3]};

		add_tmp9 <= {tmp_29[8-1] , tmp_29} + 
						  {tmp_39[8-1] , tmp_39};

		doutr_109 <= add_tmp9 - {tmp_19[8-1] , tmp_19};

		doutr_1010 <= {dinr5[8-1] , dinr5};

			doutr0 <= doutr_100;
			doutr2 <= doutr_101;
			doutr4 <= doutr_102;
			doutr6 <= doutr_103;
			doutr8 <= doutr_104;
			doutr10 <= doutr_105;
			doutr12 <= doutr_106;
			doutr14 <= doutr_107;
			doutr16 <= doutr_108;

			tmp_1_101 <= {doutr_100[8] , doutr_100[8] , doutr_100[8:2]} + {doutr_100[8] , doutr_100[8] , doutr_100[8] , doutr_100[8:3]};
			tmp_1_103 <= {doutr_101[8] , doutr_101[8] , doutr_101[8:2]} + {doutr_101[8] , doutr_101[8] , doutr_101[8] , doutr_101[8:3]};
			tmp_1_105 <= {doutr_102[8] , doutr_102[8] , doutr_102[8:2]} + {doutr_102[8] , doutr_102[8] , doutr_102[8] , doutr_102[8:3]};
			tmp_1_107 <= {doutr_103[8] , doutr_103[8] , doutr_103[8:2]} + {doutr_103[8] , doutr_103[8] , doutr_103[8] , doutr_103[8:3]};
			tmp_1_109 <= {doutr_104[8] , doutr_104[8] , doutr_104[8:2]} + {doutr_104[8] , doutr_104[8] , doutr_104[8] , doutr_104[8:3]};
			tmp_1_1011 <= {doutr_105[8] , doutr_105[8] , doutr_105[8:2]} + {doutr_105[8] , doutr_105[8] , doutr_105[8] , doutr_105[8:3]};
			tmp_1_1013 <= {doutr_106[8] , doutr_106[8] , doutr_106[8:2]} + {doutr_106[8] , doutr_106[8] , doutr_106[8] , doutr_106[8:3]};
			tmp_1_1015 <= {doutr_107[8] , doutr_107[8] , doutr_107[8:2]} + {doutr_107[8] , doutr_107[8] , doutr_107[8] , doutr_107[8:3]};
			tmp_1_1017 <= {doutr_108[8] , doutr_108[8] , doutr_108[8:2]} + {doutr_108[8] , doutr_108[8] , doutr_108[8] , doutr_108[8:3]};
				
			tmp_2_101 <= {doutr_101[8], doutr_101[8:1]} + {doutr_101[8], doutr_101[8] , doutr_101[8:2]};
			tmp_2_103 <= {doutr_102[8], doutr_102[8:1]} + {doutr_102[8], doutr_102[8] , doutr_102[8:2]};
			tmp_2_105 <= {doutr_103[8], doutr_103[8:1]} + {doutr_103[8], doutr_103[8] , doutr_103[8:2]};
			tmp_2_107 <= {doutr_104[8], doutr_104[8:1]} + {doutr_104[8], doutr_104[8] , doutr_104[8:2]};
			tmp_2_109 <= {doutr_105[8], doutr_105[8:1]} + {doutr_105[8], doutr_105[8] , doutr_105[8:2]};
			tmp_2_1011 <= {doutr_106[8], doutr_106[8:1]} + {doutr_106[8], doutr_106[8] , doutr_106[8:2]};
			tmp_2_1013 <= {doutr_107[8], doutr_107[8:1]} + {doutr_107[8], doutr_107[8] , doutr_107[8:2]};
			tmp_2_1015 <= {doutr_108[8], doutr_108[8:1]} + {doutr_108[8], doutr_108[8] , doutr_108[8:2]};
			tmp_2_1017 <= {doutr_109[8], doutr_109[8:1]} + {doutr_109[8], doutr_109[8] , doutr_109[8:2]};
			
			tmp_3_101 <= {doutr_102[8], doutr_102[8] , doutr_102[8] , doutr_102[8:3]};
			tmp_3_103 <= {doutr_103[8], doutr_103[8] , doutr_103[8] , doutr_103[8:3]};
			tmp_3_105 <= {doutr_104[8], doutr_104[8] , doutr_104[8] , doutr_104[8:3]};
			tmp_3_107 <= {doutr_105[8], doutr_105[8] , doutr_105[8] , doutr_105[8:3]};
			tmp_3_109 <= {doutr_106[8], doutr_106[8] , doutr_106[8] , doutr_106[8:3]};
			tmp_3_1011 <= {doutr_107[8], doutr_107[8] , doutr_107[8] , doutr_107[8:3]};
			tmp_3_1013 <= {doutr_108[8], doutr_108[8] , doutr_108[8] , doutr_108[8:3]};
			tmp_3_1015 <= {doutr_109[8], doutr_109[8] , doutr_109[8] , doutr_109[8:3]};
			tmp_3_1017 <= {doutr_1010[8], doutr_1010[8] , doutr_1010[8] , doutr_1010[8:3]};

			add_tmp_101 <= tmp_1_101 + tmp_2_101;
			add_tmp_103 <= tmp_1_103 + tmp_2_103;
			add_tmp_105 <= tmp_1_105 + tmp_2_105;
			add_tmp_107 <= tmp_1_107 + tmp_2_107;
			add_tmp_109 <= tmp_1_109 + tmp_2_109;
			add_tmp_1011 <= tmp_1_1011 + tmp_2_1011;
			add_tmp_1013 <= tmp_1_1013 + tmp_2_1013;
			add_tmp_1015 <= tmp_1_1015 + tmp_2_1015;
			add_tmp_1017 <= tmp_1_1017 + tmp_2_1017;

			doutr1 <= add_tmp_101 - tmp_3_101;
			doutr3 <= add_tmp_103 - tmp_3_103;
			doutr5 <= add_tmp_105 - tmp_3_105;
			doutr7 <= add_tmp_107 - tmp_3_107;
			doutr9 <= add_tmp_109 - tmp_3_109;
			doutr11 <= add_tmp_1011 - tmp_3_1011;
			doutr13 <= add_tmp_1013 - tmp_3_1013;
			doutr15 <= add_tmp_1015 - tmp_3_1015;
			doutr17 <= add_tmp_1017 - tmp_3_1017;

		doutr18 <= doutr_109;

		tmp_1_1019 <= {doutr_108[8], doutr_108[8] , doutr_108[8] , doutr_108[8:3]};

		tmp_2_1019 <= {doutr_109[8], doutr_109[8:1]} +
				{doutr_109[8], doutr_109[8] , doutr_109[8:2]};
				
		tmp_3_1019 <= {doutr_1010[8] , doutr_1010[8] , doutr_1010[8:2]} +
			{doutr_1010[8] , doutr_1010[8] , doutr_1010[8] , doutr_1010[8:3]};

		add_tmp_1019 <= tmp_2_1019 + tmp_3_1019;

		doutr19 <= add_tmp_1019 - tmp_1_1019;

		doutr20 <= doutr_1010;
   end 
endmodule



module find_max (clk, wen, 
    d_in0, 
    d_in1, 
    d_in2, 
    d_in3, 
    d_in4, 
    d_in5, 
    d_in6, 
    d_in7, 
    d_in8, 
    d_in9, 
    d_in10, 
    d_in11, 
    d_in12, 
    d_in13, 
    d_in14, 
    d_in15, 
    d_in16, 
    d_in17, 
    d_in18, 
    d_in19, 
    d_in20, 
d_out, indx_out);

    input clk; 
    input wen; 
    input[10:0] d_in0; 
    input[10:0] d_in1; 
    input[10:0] d_in2; 
    input[10:0] d_in3; 
    input[10:0] d_in4; 
    input[10:0] d_in5; 
    input[10:0] d_in6; 
    input[10:0] d_in7; 
    input[10:0] d_in8; 
    input[10:0] d_in9; 
    input[10:0] d_in10; 
    input[10:0] d_in11; 
    input[10:0] d_in12; 
    input[10:0] d_in13; 
    input[10:0] d_in14; 
    input[10:0] d_in15; 
    input[10:0] d_in16; 
    input[10:0] d_in17; 
    input[10:0] d_in18; 
    input[10:0] d_in19; 
    input[10:0] d_in20; 
    reg[10:0] d_in_tmp0; 
    reg[10:0] d_in_tmp1; 
    reg[10:0] d_in_tmp2; 
    reg[10:0] d_in_tmp3; 
    reg[10:0] d_in_tmp4; 
    reg[10:0] d_in_tmp5; 
    reg[10:0] d_in_tmp6; 
    reg[10:0] d_in_tmp7; 
    reg[10:0] d_in_tmp8; 
    reg[10:0] d_in_tmp9; 
    reg[10:0] d_in_tmp10; 
    reg[10:0] d_in_tmp11; 
    reg[10:0] d_in_tmp12; 
    reg[10:0] d_in_tmp13; 
    reg[10:0] d_in_tmp14; 
    reg[10:0] d_in_tmp15; 
    reg[10:0] d_in_tmp16; 
    reg[10:0] d_in_tmp17; 
    reg[10:0] d_in_tmp18; 
    reg[10:0] d_in_tmp19; 
    reg[10:0] d_in_tmp20; 
    output[7:0] d_out; 
    reg[7:0] d_out;
    output[4:0] indx_out; 
    reg[4:0] indx_out;
    reg[10:0] res_1_1; 
    reg[10:0] res_1_2; 
    reg[10:0] res_1_3; 
    reg[10:0] res_1_4; 
    reg[10:0] res_1_5; 
    reg[10:0] res_1_6; 
    reg[10:0] res_1_7; 
    reg[10:0] res_1_8; 
    reg[10:0] res_1_9; 
    reg[10:0] res_1_10; 
    reg[10:0] res_1_11; 
    reg[10:0] res_2_1; 
    reg[10:0] res_2_2; 
    reg[10:0] res_2_3; 
    reg[10:0] res_2_4; 
    reg[10:0] res_2_5; 
    reg[10:0] res_2_6; 
    reg[10:0] res_3_1; 
    reg[10:0] res_3_2; 
    reg[10:0] res_3_3; 
    reg[10:0] res_4_1; 
    reg[10:0] res_4_2; 
    reg[10:0] res_5_1; 
    reg[4:0] indx_1_1; 
    reg[4:0] indx_1_2; 
    reg[4:0] indx_1_3; 
    reg[4:0] indx_1_4; 
    reg[4:0] indx_1_5; 
    reg[4:0] indx_1_6; 
    reg[4:0] indx_1_7; 
    reg[4:0] indx_1_8; 
    reg[4:0] indx_1_9; 
    reg[4:0] indx_1_10; 
    reg[4:0] indx_1_11; 
    reg[4:0] indx_2_1; 
    reg[4:0] indx_2_2; 
    reg[4:0] indx_2_3; 
    reg[4:0] indx_2_4; 
    reg[4:0] indx_2_5; 
    reg[4:0] indx_2_6; 
    reg[4:0] indx_3_1; 
    reg[4:0] indx_3_2; 
    reg[4:0] indx_3_3; 
    reg[4:0] indx_4_1; 
    reg[4:0] indx_4_2; 
    reg[4:0] indx_5_1; 

    always @(posedge clk)
    begin
          if (wen == 1'b1)
          begin
            d_in_tmp0 <= d_in0 ; 
            d_in_tmp1 <= d_in1 ; 
            d_in_tmp2 <= d_in2 ; 
            d_in_tmp3 <= d_in3 ; 
            d_in_tmp4 <= d_in4 ; 
            d_in_tmp5<= d_in5 ; 
            d_in_tmp6 <= d_in6 ; 
            d_in_tmp7 <= d_in7 ; 
            d_in_tmp8 <= d_in8 ; 
            d_in_tmp9 <= d_in9 ; 
            d_in_tmp10 <= d_in10 ; 
            d_in_tmp11 <= d_in11 ; 
            d_in_tmp12 <= d_in12 ; 
            d_in_tmp13 <= d_in13 ; 
            d_in_tmp14 <= d_in14 ; 
            d_in_tmp15 <= d_in15; 
            d_in_tmp16 <= d_in16 ; 
            d_in_tmp17 <= d_in17 ; 
            d_in_tmp18 <= d_in18 ; 
            d_in_tmp19 <= d_in19 ; 
            d_in_tmp20 <= d_in20 ; 
             d_out <= res_5_1[10:3] ; 
             indx_out <= indx_5_1 ; 
          end 

          if (d_in0 > d_in1)
          begin
             res_1_1 <= d_in0 ; 
             indx_1_1 <= 5'b00000 ; 
          end
          else
          begin
             res_1_1 <= d_in1 ; 
             indx_1_1 <= 5'b00001 ; 
          end 
          if (d_in2 > d_in3)
          begin
             res_1_2 <= d_in2 ; 
             indx_1_2 <= 5'b00010 ; 
          end
          else
          begin
             res_1_2 <= d_in3 ; 
             indx_1_2 <= 5'b00011 ; 
          end 
          if (d_in4 > d_in5)
          begin
             res_1_3 <= d_in4 ; 
             indx_1_3 <= 5'b00100 ; 
          end
          else
          begin
             res_1_3 <= d_in5 ; 
             indx_1_3 <= 5'b00101 ; 
          end 
          if (d_in6 > d_in7)
          begin

             res_1_4 <= d_in6 ; 
             indx_1_4 <= 5'b00110 ; 
          end
          else
          begin
             res_1_4 <= d_in7 ; 
             indx_1_4 <= 5'b00111 ; 
          end 
          if (d_in8 > d_in9)
          begin
             res_1_5 <= d_in8 ; 
             indx_1_5 <= 5'b01000 ; 
          end
          else
          begin
             res_1_5 <= d_in9 ; 
             indx_1_5 <= 5'b01001 ; 
          end 
          if (d_in10 > d_in11)
       begin
             res_1_6 <= d_in10 ; 
             indx_1_6 <= 5'b01010 ; 
          end
          else
          begin
             res_1_6 <= d_in11 ; 
             indx_1_6 <= 5'b01011 ; 
          end 
          if (d_in12 > d_in13)
          begin
             res_1_7 <= d_in12 ; 
             indx_1_7 <= 5'b01100 ; 
          end
          else
          begin
             res_1_7 <= d_in13 ; 
             indx_1_7 <= 5'b01101 ; 
          end 
          if (d_in14 > d_in15)
          begin
             res_1_8 <= d_in14 ; 
             indx_1_8 <= 5'b01110 ; 
          end
          else
          begin
             res_1_8 <= d_in15 ; 
             indx_1_8 <= 5'b01111 ; 
          end 
          if (d_in16 > d_in17)
          begin
             res_1_9 <= d_in16 ; 
             indx_1_9 <= 5'b10000 ; 
          end
          else
          begin
             res_1_9 <= d_in17 ; 
             indx_1_9 <= 5'b10001 ; 
          end 
          if (d_in18 > d_in19)
          begin
             res_1_10 <= d_in18 ; 
             indx_1_10 <= 5'b10010 ; 
          end
          else
          begin
             res_1_10 <= d_in19 ; 
             indx_1_10 <= 5'b10011 ; 
          end 
          res_1_11 <= d_in20 ; 
          indx_1_11 <= 5'b10100 ; 
          if (res_1_1 > res_1_2)
          begin
             res_2_1 <= res_1_1 ; 
             indx_2_1 <= indx_1_1 ; 
          end
          else
          begin
             res_2_1 <= res_1_2 ; 
             indx_2_1 <= indx_1_2 ; 
          end 
          if (res_1_3 > res_1_4)
          begin
             res_2_2 <= res_1_3 ; 
             indx_2_2 <= indx_1_3 ; 
          end
          else
          begin
             res_2_2 <= res_1_4 ; 
             indx_2_2 <= indx_1_4 ; 
          end 
          if (res_1_5 > res_1_6)
          begin
             res_2_3 <= res_1_5 ; 
             indx_2_3 <= indx_1_5 ; 
          end
          else
          begin
             res_2_3 <= res_1_6 ; 
             indx_2_3 <= indx_1_6 ; 
          end 
          if (res_1_7 > res_1_8)
          begin
             res_2_4 <= res_1_7 ; 
             indx_2_4 <= indx_1_7 ; 
          end
          else
          begin
             res_2_4 <= res_1_8 ; 
             indx_2_4 <= indx_1_8 ; 
          end 
          if (res_1_9 > res_1_10)
          begin
             res_2_5 <= res_1_9 ; 
             indx_2_5 <= indx_1_9 ; 
          end
          else
          begin
             res_2_5 <= res_1_10 ; 
             indx_2_5 <= indx_1_10 ; 
          end 
          res_2_6 <= res_1_11 ; 
          indx_2_6 <= indx_1_11 ; 
          if (res_2_1 > res_2_2)
          begin
             res_3_1 <= res_2_1 ; 
             indx_3_1 <= indx_2_1 ; 
          end
          else
          begin
             res_3_1 <= res_2_2 ; 
             indx_3_1 <= indx_2_2 ; 
          end 
          if (res_2_3 > res_2_4)
          begin
             res_3_2 <= res_2_3 ; 
             indx_3_2 <= indx_2_3 ; 
          end
          else
          begin
             res_3_2 <= res_2_4 ; 
             indx_3_2 <= indx_2_4 ; 
          end 
          if (res_2_5 > res_2_6)
          begin
             res_3_3 <= res_2_5 ; 
             indx_3_3 <= indx_2_5 ; 
          end
          else
          begin
             res_3_3 <= res_2_6 ; 
             indx_3_3 <= indx_2_6 ; 
          end 
          if (res_3_1 > res_3_2)
          begin
             res_4_1 <= res_3_1 ; 
             indx_4_1 <= indx_3_1 ; 
          end
          else
          begin
             res_4_1 <= res_3_2 ; 
             indx_4_1 <= indx_3_2 ; 
          end 
          res_4_2 <= res_3_3 ; 
          indx_4_2 <= indx_3_3 ; 
          if (res_4_1 > res_4_2)
          begin
             res_5_1 <= res_4_1 ; 
             indx_5_1 <= indx_4_1 ; 
          end
          else
          begin
             res_5_1 <= res_4_2 ; 
             indx_5_1 <= indx_4_2 ; 
          end 
    end 
 endmodule

module lp_fltr (clk, din, dout, ce);

   input clk; 
   input[8 - 1:0] din; 
   output[8 - 1:0] dout; 
   reg[8 - 1:0] dout;
   input ce; 

   reg[8 - 1:0] din_tmp_1; 
   reg[8 - 1:0] din_tmp_2; 
   reg[8 - 1:0] din_tmp_3; 
   reg[(8 + 2) - 1:0] sum_tmp_1; 
   reg[(8 + 2) - 1:0] sum_tmp_2; 
   reg[(8 + 2) - 1:0] sum_tmp_3; 
   reg[(8 + 2) - 1:0] add_tmp_1; 
   reg[(8 + 2) - 1:0] add_tmp_2; 

   always @(posedge clk)
   begin
         if (ce == 1'b1)
         begin
            din_tmp_1 <= din ; 
            din_tmp_2 <= din_tmp_1 ; 
            din_tmp_3 <= din_tmp_2 ; 
            dout <= add_tmp_2[(8 + 2) - 1:2] ; 
         end 
         sum_tmp_1 <= {din_tmp_1[8 - 1], din_tmp_1[8 - 1], din_tmp_1} ; 
         sum_tmp_2 <= {din_tmp_2[8 - 1], din_tmp_2, 1'b0} ; 
         sum_tmp_3 <= {din_tmp_3[8 - 1], din_tmp_3[8 - 1], din_tmp_3} ; 
         add_tmp_1 <= sum_tmp_1 + sum_tmp_2 ; 
         add_tmp_2 <= add_tmp_1 + sum_tmp_3 ; 
   end 
endmodule





module port_bus_1to0_1 (clk, vidin_addr_reg, svid_comp_switch, vidin_new_data_scld_1_1to0,
   v_corr_200, 
   v_corr_201, 
   v_corr_202, 
   v_corr_203, 
   v_corr_204, 
   v_corr_205, 
   v_corr_206, 
   v_corr_207, 
   v_corr_208, 
   v_corr_209, 
   v_corr_2010, 
   v_corr_2011, 
   v_corr_2012, 
   v_corr_2013, 
   v_corr_2014, 
   v_corr_2015, 
   v_corr_2016, 
   v_corr_2017, 
   v_corr_2018, 
   v_corr_2019, 
   v_corr_2020, 
vidin_new_data_scld_2_1to0,
   v_corr_100, 
   v_corr_101, 
   v_corr_102, 
   v_corr_103, 
   v_corr_104, 
   v_corr_105, 
   v_corr_106, 
   v_corr_107, 
   v_corr_108, 
   v_corr_109, 
   v_corr_1010, 
vidin_new_data_scld_4_1to0, 
   v_corr_50, 
   v_corr_51, 
   v_corr_52, 
   v_corr_53, 
   v_corr_54, 
   v_corr_55, 
bus_word_1, bus_word_2, bus_word_3, bus_word_4, bus_word_5, bus_word_6, counter_out);


   input clk; 
   output[18:0] vidin_addr_reg; 
   reg[18:0] vidin_addr_reg;
   output svid_comp_switch; 
   reg svid_comp_switch;
   output vidin_new_data_scld_1_1to0; 
   reg vidin_new_data_scld_1_1to0;
   output[7:0] v_corr_200; 
   output[7:0] v_corr_201; 
   output[7:0] v_corr_202; 
   output[7:0] v_corr_203; 
   output[7:0] v_corr_204; 
   output[7:0] v_corr_205; 
   output[7:0] v_corr_206; 
   output[7:0] v_corr_207; 
   output[7:0] v_corr_208; 
   output[7:0] v_corr_209; 
   output[7:0] v_corr_2010; 
   output[7:0] v_corr_2011; 
   output[7:0] v_corr_2012; 
   output[7:0] v_corr_2013; 
   output[7:0] v_corr_2014; 
   output[7:0] v_corr_2015; 
   output[7:0] v_corr_2016; 
   output[7:0] v_corr_2017; 
   output[7:0] v_corr_2018; 
   output[7:0] v_corr_2019; 
   output[7:0] v_corr_2020; 
   reg[7:0] v_corr_200; 
   reg[7:0] v_corr_201; 
   reg[7:0] v_corr_202; 
   reg[7:0] v_corr_203; 
   reg[7:0] v_corr_204; 
   reg[7:0] v_corr_205; 
   reg[7:0] v_corr_206; 
   reg[7:0] v_corr_207; 
   reg[7:0] v_corr_208; 
   reg[7:0] v_corr_209; 
   reg[7:0] v_corr_2010; 
   reg[7:0] v_corr_2011; 
   reg[7:0] v_corr_2012; 
   reg[7:0] v_corr_2013; 
   reg[7:0] v_corr_2014; 
   reg[7:0] v_corr_2015; 
   reg[7:0] v_corr_2016; 
   reg[7:0] v_corr_2017; 
   reg[7:0] v_corr_2018; 
   reg[7:0] v_corr_2019; 
   reg[7:0] v_corr_2020; 
   output vidin_new_data_scld_2_1to0; 
   reg vidin_new_data_scld_2_1to0;
   output[7:0] v_corr_100; 
   output[7:0] v_corr_101; 
   output[7:0] v_corr_102; 
   output[7:0] v_corr_103; 
   output[7:0] v_corr_104; 
   output[7:0] v_corr_105; 
   output[7:0] v_corr_106; 
   output[7:0] v_corr_107; 
   output[7:0] v_corr_108; 
   output[7:0] v_corr_109; 
   output[7:0] v_corr_1010; 
   reg[7:0] v_corr_100; 
   reg[7:0] v_corr_101; 
   reg[7:0] v_corr_102; 
   reg[7:0] v_corr_103; 
   reg[7:0] v_corr_104; 
   reg[7:0] v_corr_105; 
   reg[7:0] v_corr_106; 
   reg[7:0] v_corr_107; 
   reg[7:0] v_corr_108; 
   reg[7:0] v_corr_109; 
   reg[7:0] v_corr_1010; 
   output vidin_new_data_scld_4_1to0; 
   reg vidin_new_data_scld_4_1to0;
   output[7:0] v_corr_50; 
   output[7:0] v_corr_51; 
   output[7:0] v_corr_52; 
   output[7:0] v_corr_53; 
   output[7:0] v_corr_54; 
   output[7:0] v_corr_55; 
   reg[7:0] v_corr_50; 
   reg[7:0] v_corr_51; 
   reg[7:0] v_corr_52; 
   reg[7:0] v_corr_53; 
   reg[7:0] v_corr_54; 
   reg[7:0] v_corr_55; 
   input[7:0] bus_word_1; 
   input[7:0] bus_word_2; 
   input[7:0] bus_word_3; 
   input[7:0] bus_word_4; 
   input[7:0] bus_word_5; 
   input[7:0] bus_word_6; 
   input[2:0] counter_out; 

   reg[7:0] bus_word_1_tmp; 
   reg[7:0] bus_word_2_tmp; 
   reg[7:0] bus_word_3_tmp; 
   reg[7:0] bus_word_4_tmp; 
   reg[7:0] bus_word_5_tmp; 
   reg[7:0] bus_word_6_tmp; 
   reg[18:0] vidin_addr_reg_tmp; 
   reg svid_comp_switch_tmp; 
   wire vidin_new_data_scld_1_1to0_tmp; 
   wire vidin_new_data_scld_2_1to0_tmp; 
   wire vidin_new_data_scld_4_1to0_tmp; 
   reg[2:0] counter_out_tmp; 
   reg[7:0] v_corr_20_tmp0; 
   reg[7:0] v_corr_20_tmp1; 
   reg[7:0] v_corr_20_tmp2; 
   reg[7:0] v_corr_20_tmp3; 
   reg[7:0] v_corr_20_tmp4; 
   reg[7:0] v_corr_20_tmp5; 
   reg[7:0] v_corr_20_tmp6; 
   reg[7:0] v_corr_20_tmp7; 
   reg[7:0] v_corr_20_tmp8; 
   reg[7:0] v_corr_20_tmp9; 
   reg[7:0] v_corr_20_tmp10; 
   reg[7:0] v_corr_20_tmp11; 
   reg[7:0] v_corr_20_tmp12; 
   reg[7:0] v_corr_20_tmp13; 
   reg[7:0] v_corr_20_tmp14; 
   reg[7:0] v_corr_20_tmp15; 
   reg[7:0] v_corr_20_tmp16; 
   reg[7:0] v_corr_20_tmp17; 
   reg[7:0] v_corr_20_tmp18; 
   reg[7:0] v_corr_20_tmp19; 
   reg[7:0] v_corr_20_tmp20; 
   reg[7:0] v_corr_10_tmp0; 
   reg[7:0] v_corr_10_tmp1; 
   reg[7:0] v_corr_10_tmp2; 
   reg[7:0] v_corr_10_tmp3; 
   reg[7:0] v_corr_10_tmp4; 
   reg[7:0] v_corr_10_tmp5; 
   reg[7:0] v_corr_10_tmp6; 
   reg[7:0] v_corr_10_tmp7; 
   reg[7:0] v_corr_10_tmp8; 
   reg[7:0] v_corr_10_tmp9; 
   reg[7:0] v_corr_10_tmp10; 
   reg[7:0] v_corr_5_tmp0; 
   reg[7:0] v_corr_5_tmp1; 
   reg[7:0] v_corr_5_tmp2; 
   reg[7:0] v_corr_5_tmp3; 
   reg[7:0] v_corr_5_tmp4; 
   reg[7:0] v_corr_5_tmp5; 

   always @(posedge clk)
   begin
         case (counter_out_tmp[2:0])
            3'b001 :
                     begin
                        vidin_addr_reg_tmp[7:0] <= bus_word_1_tmp ; 
                        vidin_addr_reg_tmp[15:8] <= bus_word_2_tmp ; 
                       vidin_addr_reg_tmp[18:16] <= bus_word_3_tmp[7:5] ; 
                        svid_comp_switch_tmp <= bus_word_3_tmp[4] ; 
                        v_corr_5_tmp0 <= bus_word_4_tmp ; 
                        v_corr_5_tmp1 <= bus_word_5_tmp ; 
                        v_corr_5_tmp2 <= bus_word_6_tmp ; 
                     end
            3'b010 :
                     begin
                        v_corr_5_tmp3 <= bus_word_1_tmp ; 
                        v_corr_5_tmp4 <= bus_word_2_tmp ; 
                        v_corr_5_tmp5 <= bus_word_3_tmp ; 
                        v_corr_10_tmp0 <= bus_word_4_tmp ; 
                        v_corr_10_tmp1 <= bus_word_5_tmp ; 
                        v_corr_10_tmp2 <= bus_word_6_tmp ; 
                     end
            3'b011 :
                     begin
                        v_corr_10_tmp3 <= bus_word_1_tmp ; 
                        v_corr_10_tmp4 <= bus_word_2_tmp ; 
                        v_corr_10_tmp5 <= bus_word_3_tmp ; 
                        v_corr_10_tmp6 <= bus_word_4_tmp ; 
                        v_corr_10_tmp7 <= bus_word_5_tmp ; 
                        v_corr_10_tmp8 <= bus_word_6_tmp ; 
                     end
            3'b100 :
                     begin
                        v_corr_10_tmp9 <= bus_word_1_tmp ; 
                        v_corr_10_tmp10 <= bus_word_2_tmp ; 
                        v_corr_20_tmp0 <= bus_word_3_tmp ; 
                        v_corr_20_tmp1 <= bus_word_4_tmp ; 
                        v_corr_20_tmp2 <= bus_word_5_tmp ; 
                        v_corr_20_tmp3 <= bus_word_6_tmp ; 
                     end
            3'b101 :
                     begin
                        v_corr_20_tmp4 <= bus_word_1_tmp ; 
                        v_corr_20_tmp5 <= bus_word_2_tmp ; 
                        v_corr_20_tmp6 <= bus_word_3_tmp ; 
                        v_corr_20_tmp7 <= bus_word_4_tmp ; 
                        v_corr_20_tmp8 <= bus_word_5_tmp ; 
                        v_corr_20_tmp9 <= bus_word_6_tmp ; 
                     end
            3'b110 :
                     begin
                        v_corr_20_tmp10 <= bus_word_1_tmp ; 
                        v_corr_20_tmp11 <= bus_word_2_tmp ; 
                        v_corr_20_tmp12 <= bus_word_3_tmp ; 
                        v_corr_20_tmp13 <= bus_word_4_tmp ; 
                        v_corr_20_tmp14 <= bus_word_5_tmp ; 
                        v_corr_20_tmp15 <= bus_word_6_tmp ; 
                     end
            3'b111 :
                     begin
                        v_corr_20_tmp16 <= bus_word_1_tmp ; 
                        v_corr_20_tmp17 <= bus_word_2_tmp ; 
                        v_corr_20_tmp18 <= bus_word_3_tmp ; 
                        v_corr_20_tmp19 <= bus_word_4_tmp ; 
                        v_corr_20_tmp20 <= bus_word_5_tmp ; 
                     end
            default :
                     begin
                        v_corr_20_tmp16 <= bus_word_1_tmp ; 
                        v_corr_20_tmp17 <= bus_word_2_tmp ; 
                        v_corr_20_tmp18 <= bus_word_3_tmp ; 
                        v_corr_20_tmp19 <= bus_word_4_tmp ; 
                        v_corr_20_tmp20 <= bus_word_5_tmp ; 
                     end
         endcase 
   end 

   always @(posedge clk)
   begin
         counter_out_tmp <= counter_out ; 
         bus_word_1_tmp <= bus_word_1 ; 
         bus_word_2_tmp <= bus_word_2 ; 
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
            if (vidin_addr_reg_tmp[8:0] != 9'b000000000)
            begin
               vidin_new_data_scld_1_1to0 <= 1'b1 ; 
               if ((vidin_addr_reg_tmp[0]) == 1'b0 & (vidin_addr_reg_tmp[9]) == 1'b0)
               begin
                  vidin_new_data_scld_2_1to0 <= 1'b1 ; 
                  if ((vidin_addr_reg_tmp[1]) == 1'b0 & (vidin_addr_reg_tmp[10]) == 1'b0)
                  begin
                     vidin_new_data_scld_4_1to0 <= 1'b1 ; 
                  end
                  else
                  begin
                     vidin_new_data_scld_4_1to0 <= 1'b0 ; 
                  end 
               end
               else
               begin
                  vidin_new_data_scld_2_1to0 <= 1'b0 ; 
                  vidin_new_data_scld_4_1to0 <= 1'b0 ; 
               end 
            end
            else
            begin
               vidin_new_data_scld_1_1to0 <= 1'b0 ; 
               vidin_new_data_scld_4_1to0 <= 1'b0 ; 
               vidin_new_data_scld_2_1to0 <= 1'b0 ; 
            end 
            v_corr_50 <= v_corr_5_tmp0 ; 
            v_corr_51 <= v_corr_5_tmp1 ; 
            v_corr_52 <= v_corr_5_tmp2 ; 
            v_corr_53 <= v_corr_5_tmp3 ; 
            v_corr_54 <= v_corr_5_tmp4 ; 
            v_corr_55 <= v_corr_5_tmp5 ; 
            v_corr_100 <= v_corr_10_tmp0 ; 
            v_corr_101 <= v_corr_10_tmp1 ; 
            v_corr_102 <= v_corr_10_tmp2 ; 
            v_corr_103 <= v_corr_10_tmp3 ; 
            v_corr_104 <= v_corr_10_tmp4 ; 
            v_corr_105 <= v_corr_10_tmp5 ; 
            v_corr_106 <= v_corr_10_tmp6 ; 
            v_corr_107 <= v_corr_10_tmp7 ; 
            v_corr_108 <= v_corr_10_tmp8 ; 
            v_corr_109 <= v_corr_10_tmp9 ; 
            v_corr_1010 <= v_corr_10_tmp10 ; 
            v_corr_200 <= v_corr_20_tmp0 ; 
            v_corr_201 <= v_corr_20_tmp1 ; 
            v_corr_202 <= v_corr_20_tmp2 ; 
            v_corr_203 <= v_corr_20_tmp3 ; 
            v_corr_204 <= v_corr_20_tmp4 ; 
            v_corr_205 <= v_corr_20_tmp5 ; 
            v_corr_206 <= v_corr_20_tmp6 ; 
            v_corr_207 <= v_corr_20_tmp7 ; 
            v_corr_208 <= v_corr_20_tmp8 ; 
            v_corr_209 <= v_corr_20_tmp9 ; 
            v_corr_2010 <= v_corr_20_tmp10 ; 
            v_corr_2011 <= v_corr_20_tmp11 ; 
            v_corr_2012 <= v_corr_20_tmp12 ; 
            v_corr_2013 <= v_corr_20_tmp13 ; 
            v_corr_2014 <= v_corr_20_tmp14 ; 
            v_corr_2015 <= v_corr_20_tmp15 ; 
            v_corr_2016 <= v_corr_20_tmp16 ; 
            v_corr_2017 <= v_corr_20_tmp17 ; 
            v_corr_2018 <= v_corr_20_tmp18 ; 
            v_corr_2019 <= v_corr_20_tmp19 ; 
            v_corr_2020 <= v_corr_20_tmp20 ; 
         end
         else
         begin
            vidin_new_data_scld_1_1to0 <= 1'b0 ; 
            vidin_new_data_scld_2_1to0 <= 1'b0 ; 
            vidin_new_data_scld_4_1to0 <= 1'b0 ; 
         end 
   end 
endmodule



module my_fifo_496 (clk, din, dout, rdy);

    input clk; 
    input[8 - 1:0] din; 
    output[8 - 1:0] dout; 
    reg[8 - 1:0] dout;
	input rdy;

    reg[8-1:0]buff1;
    reg[8-1:0]buff2;

	// Fifo size log(FIFO_SIZE)/log2

    always @(posedge clk)
    begin
		if (rdy == 1'b1)
		begin
			buff1 <= din;
			dout <= buff2;
			buff2 <= buff1;

		end
	end
endmodule

module my_fifo_316 (clk, din, dout, rdy);

    input clk; 
    input[8 - 1:0] din; 
    output[8 - 1:0] dout; 
    reg[8 - 1:0] dout;
	input rdy;

    reg[8-1:0]buff1;
    reg[8-1:0]buff2;


    always @(posedge clk)
    begin
		if (rdy == 1'b1)
		begin
			buff1 <= din;
			dout <= buff2;
			buff2 <= buff1;

		end
	end
endmodule

module my_fifo_2 (clk, din, dout, rdy);


    input clk; 
    input[9 - 1:0] din; 
    output[9 - 1:0] dout; 
    reg[9 - 1:0] dout;
	input rdy;

    reg[9-1:0]buff1;
    reg[9-1:0]buff2;


    always @(posedge clk)
    begin
		if (rdy == 1'b1)
		begin
			buff1 <= din;
			dout <= buff2;
			buff2 <= buff1;

		end
	end
endmodule

module my_fifo_1 (clk, din, dout, rdy);


    input clk; 
    input[8 - 1:0] din; 
    output[8 - 1:0] dout; 
    reg[8 - 1:0] dout;
	input rdy;

    reg[8-1:0]buff1;
    reg[8-1:0]buff2;


    always @(posedge clk)
    begin
		if (rdy == 1'b1)
		begin
			buff1 <= din;
			dout <= buff2;
			buff2 <= buff1;

		end
	end
endmodule

module my_fifo_89 (clk, din, dout, rdy);

    input clk; 
    input[8 - 1:0] din; 
    output[8 - 1:0] dout; 
    reg[8 - 1:0] dout;
	input rdy;

    reg[8-1:0]buff1;
    reg[8-1:0]buff2;


    always @(posedge clk)
    begin
		if (rdy == 1'b1)
		begin
			buff1 <= din;
			dout <= buff2;
			buff2 <= buff1;

		end
	end
endmodule

module my_fifo_179 (clk, din, dout, rdy);

    input clk; 
    input[8 - 1:0] din; 
    output[8 - 1:0] dout; 
    reg[8 - 1:0] dout;
	input rdy;

    reg[8-1:0]buff1;
    reg[8-1:0]buff2;


    always @(posedge clk)
    begin
		if (rdy == 1'b1)
		begin
			buff1 <= din;
			dout <= buff2;
			buff2 <= buff1;

		end
	end
endmodule

module my_fifo_359 (clk, din, dout, rdy);


    input clk; 
    input[8 - 1:0] din; 
    output[8 - 1:0] dout; 
    reg[8 - 1:0] dout;
	input rdy;

    reg[8-1:0]buff1;
    reg[8-1:0]buff2;


    always @(posedge clk)
    begin
		if (rdy == 1'b1)
		begin
			buff1 <= din;
			dout <= buff2;
			buff2 <= buff1;

		end
	end
endmodule
