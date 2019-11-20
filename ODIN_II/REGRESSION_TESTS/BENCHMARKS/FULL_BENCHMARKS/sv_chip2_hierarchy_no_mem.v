module sv_chip2_hierarchy_no_mem (reset, tm3_clk_v0, tm3_sram_data_in, tm3_sram_data_out, tm3_sram_addr, tm3_sram_we, tm3_sram_oe, tm3_sram_adsp, vidin_new_data, vidin_rgb_reg, vidin_addr_reg, svid_comp_switch, counter_out_2to1, bus_word_3_2to1, bus_word_4_2to1, bus_word_5_2to1, bus_word_6_2to1, vidin_new_data_fifo, vidin_rgb_reg_fifo_left, vidin_rgb_reg_fifo_right, vidin_addr_reg_2to0, v_nd_s1_left_2to0, v_nd_s2_left_2to0 , v_nd_s4_left_2to0 , v_d_reg_s1_left_2to0 , v_d_reg_s2_left_2to0 , v_d_reg_s4_left_2to0 , v_nd_s1_right_2to0, v_nd_s2_right_2to0 , v_nd_s4_right_2to0 , v_d_reg_s1_right_2to0 , v_d_reg_s2_right_2to0 , v_d_reg_s4_right_2to0);
	
	input reset;
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
   input vidin_new_data; 
   input[7:0] vidin_rgb_reg; 
   input[18:0] vidin_addr_reg; 
   input svid_comp_switch; 
   output[2:0] counter_out_2to1; 
   wire[2:0] counter_out_2to1;
   output[15:0] bus_word_3_2to1; 
   wire[15:0] bus_word_3_2to1;
   output[15:0] bus_word_4_2to1; 
   wire[15:0] bus_word_4_2to1;
   output[15:0] bus_word_5_2to1; 
   wire[15:0] bus_word_5_2to1;
   output[15:0] bus_word_6_2to1; 
   wire[15:0] bus_word_6_2to1;
   output vidin_new_data_fifo; 
   reg vidin_new_data_fifo;
   output[7:0] vidin_rgb_reg_fifo_left; 
   reg[7:0] vidin_rgb_reg_fifo_left;
   output[7:0] vidin_rgb_reg_fifo_right; 
   reg[7:0] vidin_rgb_reg_fifo_right;
   output[3:0] vidin_addr_reg_2to0; 
   reg[3:0] vidin_addr_reg_2to0;
   input v_nd_s1_left_2to0; 
   input v_nd_s2_left_2to0; 
   input v_nd_s4_left_2to0; 
   input[7:0] v_d_reg_s1_left_2to0; 
   input[7:0] v_d_reg_s2_left_2to0; 
   input[7:0] v_d_reg_s4_left_2to0; 
   input v_nd_s1_right_2to0; 
   input v_nd_s2_right_2to0; 
   input v_nd_s4_right_2to0; 
   input[7:0] v_d_reg_s1_right_2to0; 
   input[7:0] v_d_reg_s2_right_2to0; 
   input[7:0] v_d_reg_s4_right_2to0; 

   wire v_nd_s1; 
   wire vidin_new_data_v_fltr; 
   reg[9:0] horiz; 
   reg[9:0] vert; 
   reg[63:0] vidin_data_buf_sc_1; 
   reg[55:0] vidin_data_buf_2_sc_1; 
   reg[18:0] vidin_addr_buf_sc_1; 
   reg[13:0] vidin_addr_buf_sc_1_fifo; 
   wire[18:0] vidin_addr_reg_scld; 
   reg video_state; 
   wire[7:0] vidin_gray_scld_1; 
   reg[63:0] vidout_buf_fifo_1_left; 
   reg[63:0] vidout_buf_fifo_1_right; 
   reg[7:0] vidin_rgb_reg_tmp; 
   reg[7:0] vidin_data_buf_fifo_sc_1_l; 
   reg[7:0] vidin_data_buf_fifo_sc_1_r; 
   reg[63:0] vidout_buf_fifo_2_1_left; 
   reg[63:0] vidout_buf_fifo_2_1_right; 
   wire vidin_new_data_tmp; 
   reg[18:0] vidin_addr_reg_reg; 
   reg v_nd_s1_left; 
   reg v_nd_s1_right; 
   reg v_nd_s2_left; 
   reg v_nd_s2_right; 
   reg v_nd_s4_left; 
   reg v_nd_s4_right; 
   reg[7:0] v_d_reg_s1_left; 
   reg[7:0] v_d_reg_s1_right; 
   reg[7:0] v_d_reg_s2_left; 
   reg[7:0] v_d_reg_s2_right; 
   reg[7:0] v_d_reg_s4_left; 
   reg[7:0] v_d_reg_s4_right; 
   wire[15:0] vidin_v_out_1_f1_left; 
   wire[15:0] vidin_v_out_1_f2_left; 
   wire[15:0] vidin_v_out_1_f3_left; 
   wire[15:0] vidin_v_out_1_h1_left; 
   wire[15:0] vidin_v_out_1_h2_left; 
   wire[15:0] vidin_v_out_1_h3_left; 
   wire[15:0] vidin_v_out_1_h4_left; 
   wire[15:0] vidin_v_out_2_f1_left; 
   wire[15:0] vidin_v_out_2_f2_left; 
   wire[15:0] vidin_v_out_2_f3_left; 
   wire[15:0] vidin_v_out_2_h1_left; 
   wire[15:0] vidin_v_out_2_h2_left; 
   wire[15:0] vidin_v_out_2_h3_left; 
   wire[15:0] vidin_v_out_2_h4_left; 
   wire[15:0] vidin_v_out_4_f1_left; 
   wire[15:0] vidin_v_out_4_f2_left; 
   wire[15:0] vidin_v_out_4_f3_left; 
   wire[15:0] vidin_v_out_4_h1_left; 
   wire[15:0] vidin_v_out_4_h2_left; 
   wire[15:0] vidin_v_out_4_h3_left; 
   wire[15:0] vidin_v_out_4_h4_left; 
   wire[15:0] vidin_v_out_1_f1_right; 
   wire[15:0] vidin_v_out_1_f2_right; 
   wire[15:0] vidin_v_out_1_f3_right; 
   wire[15:0] vidin_v_out_1_h1_right; 
   wire[15:0] vidin_v_out_1_h2_right; 
   wire[15:0] vidin_v_out_1_h3_right; 
   wire[15:0] vidin_v_out_1_h4_right; 
   wire[15:0] vidin_v_out_2_f1_right; 
   wire[15:0] vidin_v_out_2_f2_right; 
   wire[15:0] vidin_v_out_2_f3_right; 
   wire[15:0] vidin_v_out_2_h1_right; 
   wire[15:0] vidin_v_out_2_h2_right; 
   wire[15:0] vidin_v_out_2_h3_right; 
   wire[15:0] vidin_v_out_2_h4_right; 
   wire[15:0] vidin_v_out_4_f1_right; 
   wire[15:0] vidin_v_out_4_f2_right; 
   wire[15:0] vidin_v_out_4_f3_right; 
   wire[15:0] vidin_v_out_4_h1_right; 
   wire[15:0] vidin_v_out_4_h2_right; 
   wire[15:0] vidin_v_out_4_h3_right; 
   wire[15:0] vidin_v_out_4_h4_right; 
   wire[7:0] v_d_reg_s1_2to3_left; 
   wire[7:0] v_d_reg_s2_2to3_left; 
   wire[7:0] v_d_reg_s4_2to3_left; 
   wire[7:0] v_d_reg_s1_2to3_right; 
   wire[7:0] v_d_reg_s2_2to3_right; 
   wire[7:0] v_d_reg_s4_2to3_right; 
   reg[18:0] vidin_addr_reg_2to3; 
   reg svid_comp_switch_2to3; 
   wire[15:0] real_z_4_left; 
   wire[15:0] imag_z_4_left; 
   wire[15:0] real_p_4_left; 
   wire[15:0] imag_p_4_left; 
   wire[15:0] real_n_4_left; 
   wire[15:0] imag_n_4_left; 
   wire[15:0] real_z_4_right; 
   wire[15:0] imag_z_4_right; 
   wire[15:0] real_p_4_right; 
   wire[15:0] imag_p_4_right; 
   wire[15:0] real_n_4_right; 
   wire[15:0] imag_n_4_right; 
   wire[15:0] real_z_2_left; 
   wire[15:0] imag_z_2_left; 
   wire[15:0] real_p_2_left; 
   wire[15:0] imag_p_2_left; 
   wire[15:0] real_n_2_left; 
   wire[15:0] imag_n_2_left; 
   wire[15:0] real_z_2_right; 
   wire[15:0] imag_z_2_right; 
   wire[15:0] real_p_2_right; 
   wire[15:0] imag_p_2_right; 
   wire[15:0] real_n_2_right; 
   wire[15:0] imag_n_2_right; 
   wire[15:0] real_z_1_left; 
   wire[15:0] imag_z_1_left; 
   wire[15:0] real_p_1_left; 
   wire[15:0] imag_p_1_left; 
   wire[15:0] real_n_1_left; 
   wire[15:0] imag_n_1_left; 
   wire[15:0] real_z_1_right; 
   wire[15:0] imag_z_1_right; 
   wire[15:0] real_p_1_right; 
   wire[15:0] imag_p_1_right; 
   wire[15:0] real_n_1_right; 
   wire[15:0] imag_n_1_right; 

   assign tm3_sram_data_out = tm3_sram_data_xhdl0;

   v_fltr_496x7  v_fltr_1_left(tm3_clk_v0, v_nd_s1_left, v_d_reg_s1_left, vidin_v_out_1_f1_left, vidin_v_out_1_f2_left, vidin_v_out_1_f3_left, vidin_v_out_1_h1_left, vidin_v_out_1_h2_left, vidin_v_out_1_h3_left, vidin_v_out_1_h4_left); 
   v_fltr_316x7  v_fltr_2_left(tm3_clk_v0, v_nd_s2_left, v_d_reg_s2_left, vidin_v_out_2_f1_left, vidin_v_out_2_f2_left, vidin_v_out_2_f3_left, vidin_v_out_2_h1_left, vidin_v_out_2_h2_left, vidin_v_out_2_h3_left, vidin_v_out_2_h4_left); 
   v_fltr_226x7  v_fltr_4_left(tm3_clk_v0, v_nd_s4_left, v_d_reg_s4_left, vidin_v_out_4_f1_left, vidin_v_out_4_f2_left, vidin_v_out_4_f3_left, vidin_v_out_4_h1_left, vidin_v_out_4_h2_left, vidin_v_out_4_h3_left, vidin_v_out_4_h4_left); 
   h_fltr h_fltr_1_left (tm3_clk_v0, v_nd_s1_left, vidin_v_out_1_f1_left, vidin_v_out_1_f2_left, vidin_v_out_1_f3_left, vidin_v_out_1_h1_left, vidin_v_out_1_h2_left, vidin_v_out_1_h3_left, vidin_v_out_1_h4_left, real_z_1_left, imag_z_1_left, real_p_1_left, imag_p_1_left, real_n_1_left, imag_n_1_left);
   h_fltr h_fltr_2_left (tm3_clk_v0, v_nd_s2_left, vidin_v_out_2_f1_left, vidin_v_out_2_f2_left, vidin_v_out_2_f3_left, vidin_v_out_2_h1_left, vidin_v_out_2_h2_left, vidin_v_out_2_h3_left, vidin_v_out_2_h4_left, real_z_2_left, imag_z_2_left, real_p_2_left, imag_p_2_left, real_n_2_left, imag_n_2_left); 
   h_fltr h_fltr_4_left (tm3_clk_v0, v_nd_s4_left, vidin_v_out_4_f1_left, vidin_v_out_4_f2_left, vidin_v_out_4_f3_left, vidin_v_out_4_h1_left, vidin_v_out_4_h2_left, vidin_v_out_4_h3_left, vidin_v_out_4_h4_left, real_z_4_left, imag_z_4_left, real_p_4_left, imag_p_4_left, real_n_4_left, imag_n_4_left);

   v_fltr_496x7  v_fltr_1_right(tm3_clk_v0, v_nd_s1_right, v_d_reg_s1_right, vidin_v_out_1_f1_right, vidin_v_out_1_f2_right, vidin_v_out_1_f3_right, vidin_v_out_1_h1_right, vidin_v_out_1_h2_right, vidin_v_out_1_h3_right, vidin_v_out_1_h4_right); 
   v_fltr_316x7  v_fltr_2_right(tm3_clk_v0, v_nd_s2_right, v_d_reg_s2_right, vidin_v_out_2_f1_right, vidin_v_out_2_f2_right, vidin_v_out_2_f3_right, vidin_v_out_2_h1_right, vidin_v_out_2_h2_right, vidin_v_out_2_h3_right, vidin_v_out_2_h4_right); 
   v_fltr_226x7  v_fltr_4_right(tm3_clk_v0, v_nd_s4_right, v_d_reg_s4_right, vidin_v_out_4_f1_right, vidin_v_out_4_f2_right, vidin_v_out_4_f3_right, vidin_v_out_4_h1_right, vidin_v_out_4_h2_right, vidin_v_out_4_h3_right, vidin_v_out_4_h4_right); 
   h_fltr h_fltr_1_right (tm3_clk_v0, v_nd_s1_right, vidin_v_out_1_f1_right, vidin_v_out_1_f2_right, vidin_v_out_1_f3_right, vidin_v_out_1_h1_right, vidin_v_out_1_h2_right, vidin_v_out_1_h3_right, vidin_v_out_1_h4_right, real_z_1_right, imag_z_1_right, real_p_1_right, imag_p_1_right, real_n_1_right, imag_n_1_right);
   h_fltr h_fltr_2_right (tm3_clk_v0, v_nd_s2_right, vidin_v_out_2_f1_right, vidin_v_out_2_f2_right, vidin_v_out_2_f3_right, vidin_v_out_2_h1_right, vidin_v_out_2_h2_right, vidin_v_out_2_h3_right, vidin_v_out_2_h4_right, real_z_2_right, imag_z_2_right, real_p_2_right, imag_p_2_right, real_n_2_right, imag_n_2_right);
   h_fltr h_fltr_4_right (tm3_clk_v0, v_nd_s4_right, vidin_v_out_4_f1_right, vidin_v_out_4_f2_right, vidin_v_out_4_f3_right, vidin_v_out_4_h1_right, vidin_v_out_4_h2_right, vidin_v_out_4_h3_right, vidin_v_out_4_h4_right, real_z_4_right, imag_z_4_right, real_p_4_right, imag_p_4_right, real_n_4_right, imag_n_4_right); 

   port_bus_2to1 port_bus_2to1_inst (tm3_clk_v0, vidin_addr_reg_2to3, svid_comp_switch_2to3, v_nd_s1_left, real_p_1_left, imag_p_1_left, real_n_1_left, imag_n_1_left, real_p_2_left, imag_p_2_left, real_n_2_left, imag_n_2_left, real_p_4_left, imag_p_4_left, real_n_4_left, imag_n_4_left, real_p_1_right, imag_p_1_right, real_n_1_right, imag_n_1_right, real_p_2_right, imag_p_2_right, real_n_2_right, imag_n_2_right, real_p_4_right, imag_p_4_right, real_n_4_right, imag_n_4_right, bus_word_3_2to1, bus_word_4_2to1, bus_word_5_2to1, bus_word_6_2to1, counter_out_2to1); 

   always @(posedge tm3_clk_v0 or negedge reset)
   begin
		if (reset == 1'b0)
		begin
			video_state <= 1'b0;
            tm3_sram_adsp <= 1'b0 ; 
		end
		else
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
            case (horiz[2:0])
               3'b000 :
                        begin
                           tm3_sram_oe <= 2'b10 ; 
                        end
               3'b001 :
                        begin
                           tm3_sram_oe <= 2'b11 ; 
                        end
               3'b010 :
                        begin
                           tm3_sram_oe <= 2'b10 ; 
                        end
               3'b011 :
                        begin
                           tm3_sram_oe <= 2'b11 ; 
                        end
               3'b100 :
                        begin
                           tm3_sram_oe <= 2'b11 ; 
                        end
               3'b101 :
                        begin
                           tm3_sram_oe <= 2'b11 ; 
                        end
                3'b110 :
                        begin
                           tm3_sram_oe <= 2'b11 ; 
                        end
               3'b111 :
                        begin
                           tm3_sram_oe <= 2'b11 ; 
                        end
            endcase 
         end
         else
         begin
            tm3_sram_adsp <= 1'b0 ; 
            case (horiz[2:0])
               3'b000 :
                        begin
                           tm3_sram_addr <= {5'b00000, vidin_addr_buf_sc_1_fifo} ; 
                           tm3_sram_we <= 8'b11111111 ; 
                           tm3_sram_oe <= 2'b11 ; 
                           tm3_sram_data_xhdl0 <= 64'b0000000000000000000000000000000000000000000000000000000000000000; 
                        end
               3'b001 :
                        begin
                           vidout_buf_fifo_1_left <= tm3_sram_data_in ; 
                           tm3_sram_addr <= vidin_addr_buf_sc_1 ; 
                           tm3_sram_we <= 8'b11111111 ; 
                           tm3_sram_oe <= 2'b11 ; 
                           tm3_sram_data_xhdl0 <= vidin_data_buf_sc_1 ; 
                        end
               3'b010 :
                        begin
                           tm3_sram_addr <= {5'b00001, vidin_addr_buf_sc_1_fifo} ; 
                           tm3_sram_we <= 8'b11111111 ; 
                           tm3_sram_oe <= 2'b11 ; 
                           tm3_sram_data_xhdl0 <= 64'b0000000000000000000000000000000000000000000000000000000000000000; 
                        end
               3'b011 :
                        begin
                           vidout_buf_fifo_1_right <= tm3_sram_data_in ; 
                           tm3_sram_addr <= vidin_addr_buf_sc_1 ; 
                           tm3_sram_we <= 8'b11111111 ; 
                           tm3_sram_oe <= 2'b11 ; 
                           tm3_sram_data_xhdl0 <= vidin_data_buf_sc_1 ; 
                        end
               3'b100 :
                        begin
                           tm3_sram_addr <= {5'b00000, vidin_addr_buf_sc_1_fifo} ; 
                           tm3_sram_we <= 8'b11111111 ; 
                           tm3_sram_oe <= 2'b11 ; 
                           tm3_sram_data_xhdl0 <= 64'b0000000000000000000000000000000000000000000000000000000000000000; 
                        end
               3'b101 :
                        begin
                           tm3_sram_addr <= vidin_addr_buf_sc_1 ; 
                           tm3_sram_we <= 8'b11111111 ; 
                           tm3_sram_oe <= 2'b11 ; 
                           tm3_sram_data_xhdl0 <= vidin_data_buf_sc_1 ; 
                        end
               3'b110 :
                        begin
                           if ((vert[8]) == 1'b0)
                           begin
                              tm3_sram_addr <= {5'b00000, vert[7:0], horiz[8:3]} ; 
                              tm3_sram_we <= 8'b11111111 ; 
                              tm3_sram_oe <= 2'b11 ; 
                           		tm3_sram_data_xhdl0 <= 64'b0000000000000000000000000000000000000000000000000000000000000000; 
                            end
                           else
                           begin
                              tm3_sram_addr <= {5'b00001, vert[7:0], horiz[8:3]} ; 
                              tm3_sram_we <= 8'b11111111 ; 
                              tm3_sram_oe <= 2'b11 ; 
                           		tm3_sram_data_xhdl0 <= 64'b0000000000000000000000000000000000000000000000000000000000000000; 
                           end 
                        end
               3'b111 :
                        begin
                           tm3_sram_addr <= vidin_addr_buf_sc_1 ; 
                           tm3_sram_we <= 8'b11111111 ; 
                           tm3_sram_oe <= 2'b11 ; 
                           tm3_sram_data_xhdl0 <= vidin_data_buf_sc_1 ; 
                        end
            endcase 
         end 
         if (vidin_new_data_fifo == 1'b1)
         begin
            case (vidin_addr_reg_reg[2:0])
               3'b000 :
                        begin
                           vidin_data_buf_2_sc_1[7:0] <= vidin_rgb_reg_tmp ; 
                           vidin_data_buf_fifo_sc_1_l <= vidout_buf_fifo_2_1_left[7:0] ; 
                           vidin_data_buf_fifo_sc_1_r <= vidout_buf_fifo_2_1_right[7:0] ; 
                        end
               3'b001 :
                        begin
                           vidin_data_buf_2_sc_1[15:8] <= vidin_rgb_reg_tmp ; 
                           vidin_data_buf_fifo_sc_1_l <= vidout_buf_fifo_2_1_left[15:8] ; 
                           vidin_data_buf_fifo_sc_1_r <= vidout_buf_fifo_2_1_right[15:8] ; 
                        end
               3'b010 :
                        begin
                           vidin_data_buf_2_sc_1[23:16] <= vidin_rgb_reg_tmp ; 
                           vidin_data_buf_fifo_sc_1_l <= vidout_buf_fifo_2_1_left[23:16] ; 
                           vidin_data_buf_fifo_sc_1_r <= vidout_buf_fifo_2_1_right[23:16] ; 
                        end
               3'b011 :
                        begin
                           vidin_data_buf_2_sc_1[31:24] <= vidin_rgb_reg_tmp ; 
                           vidin_data_buf_fifo_sc_1_l <= vidout_buf_fifo_2_1_left[31:24] ; 
                           vidin_data_buf_fifo_sc_1_r <= vidout_buf_fifo_2_1_right[31:24] ; 
                        end
               3'b100 :
                        begin
                           vidin_data_buf_2_sc_1[39:32] <= vidin_rgb_reg_tmp ; 
                           vidin_data_buf_fifo_sc_1_l <= vidout_buf_fifo_2_1_left[39:32] ; 
                           vidin_data_buf_fifo_sc_1_r <= vidout_buf_fifo_2_1_right[39:32] ; 
                        end
               3'b101 :
                        begin
                           vidin_data_buf_2_sc_1[47:40] <= vidin_rgb_reg_tmp ; 
                           vidin_data_buf_fifo_sc_1_l <= vidout_buf_fifo_2_1_left[47:40] ; 
                           vidin_data_buf_fifo_sc_1_r <= vidout_buf_fifo_2_1_right[47:40] ; 
                        end
               3'b110 :
                        begin
                           vidin_data_buf_2_sc_1[55:48] <= vidin_rgb_reg_tmp ; 
                           vidin_data_buf_fifo_sc_1_l <= vidout_buf_fifo_2_1_left[55:48] ; 
                           vidin_data_buf_fifo_sc_1_r <= vidout_buf_fifo_2_1_right[55:48] ; 
                        end
               3'b111 :
                        begin
                           vidin_data_buf_sc_1 <= {vidin_rgb_reg_tmp, vidin_data_buf_2_sc_1[55:0]} ; 
                           vidout_buf_fifo_2_1_left <= vidout_buf_fifo_1_left ; 
                           vidout_buf_fifo_2_1_right <= vidout_buf_fifo_1_right ; 

                         vidin_data_buf_fifo_sc_1_l <= vidout_buf_fifo_2_1_left[63:56] ; 
                           vidin_data_buf_fifo_sc_1_r <= vidout_buf_fifo_2_1_right[63:56] ; 
                           vidin_addr_buf_sc_1 <= {4'b0000, svid_comp_switch, vidin_addr_reg_reg[16:3]} ; 
                           if (vidin_addr_reg_reg[8:3] == 43)
                           begin
                              vidin_addr_buf_sc_1_fifo <= {(vidin_addr_reg_reg[16:9] + 8'b00000001), 6'b000000} ; 
                           end
                           else
                           begin
                              if (vidin_addr_reg_reg[8:3] == 44)
                              begin
                                 vidin_addr_buf_sc_1_fifo <= {(vidin_addr_reg_reg[16:9] + 8'b00000001), 6'b000001} ; 
                              end
                              else
                              begin
                                 vidin_addr_buf_sc_1_fifo <= (vidin_addr_reg_reg[16:3]) + 2 ; 
                              end 
                           end 
                        end
            endcase 
         end 
		end
   end 

  always @(posedge tm3_clk_v0)
   begin
         vidin_rgb_reg_tmp <= vidin_rgb_reg ; 
         vidin_addr_reg_2to3 <= vidin_addr_reg ; 
         vidin_addr_reg_reg <= vidin_addr_reg ; 
         vidin_addr_reg_2to0 <= {vidin_addr_reg[1:0], vidin_addr_reg[10:9]} ; 
         vidin_new_data_fifo <= vidin_new_data ; 
         svid_comp_switch_2to3 <= svid_comp_switch ; 
         vidin_rgb_reg_fifo_left <= vidin_data_buf_fifo_sc_1_l ; 
         vidin_rgb_reg_fifo_right <= vidin_data_buf_fifo_sc_1_r ; 
         v_nd_s1_left <= v_nd_s1_left_2to0 ; 
         v_nd_s2_left <= v_nd_s2_left_2to0 ; 
         v_nd_s4_left <= v_nd_s4_left_2to0 ; 
         v_d_reg_s1_left <= v_d_reg_s1_left_2to0 ; 
         v_d_reg_s2_left <= v_d_reg_s2_left_2to0 ; 
         v_d_reg_s4_left <= v_d_reg_s4_left_2to0 ; 
         v_nd_s1_right <= v_nd_s1_right_2to0 ; 
         v_nd_s2_right <= v_nd_s2_right_2to0 ; 
         v_nd_s4_right <= v_nd_s4_right_2to0 ; 
         v_d_reg_s1_right <= v_d_reg_s1_right_2to0 ; 
         v_d_reg_s2_right <= v_d_reg_s2_right_2to0 ; 
         v_d_reg_s4_right <= v_d_reg_s4_right_2to0 ; 
   end 
endmodule



// Discription: this block creates a long fifo
// of  lengh of one line and then applies the
// the first and last byte of the fifo into a 
// that finally creates horizontal edge detection
// filter. 
// note: it uses fifo component to implement the fifo
// date: Oct.7 ,2001
// By:  Ahmad darabiha
module h_fltr (tm3_clk_v0, vidin_new_data, vidin_in_f1, vidin_in_f2, vidin_in_f3, vidin_in_h1, vidin_in_h2, vidin_in_h3, vidin_in_h4, real_z_reg, imag_z_reg, real_p_reg, imag_p_reg, real_n_reg, imag_n_reg);

   input tm3_clk_v0; 
   input vidin_new_data; 
   input[15:0] vidin_in_f1; 
   input[15:0] vidin_in_f2; 
   input[15:0] vidin_in_f3; 
   input[15:0] vidin_in_h1; 
   input[15:0] vidin_in_h2; 
   input[15:0] vidin_in_h3; 
   input[15:0] vidin_in_h4; 
   output[15:0] real_z_reg; 
   reg[15:0] real_z_reg;
   output[15:0] imag_z_reg; 
   reg[15:0] imag_z_reg;
   output[15:0] real_p_reg; 
   reg[15:0] real_p_reg;
   output[15:0] imag_p_reg; 
   reg[15:0] imag_p_reg;
   output[15:0] real_n_reg; 
   reg[15:0] real_n_reg;
   output[15:0] imag_n_reg; 
   reg[15:0] imag_n_reg;

   wire[27:0] vidin_out_temp_f1; 
   reg[27:0] vidin_out_reg_f1; 
   wire my_fir_rdy_f1; 
   wire[27:0] vidin_out_temp_f2; 
   reg[27:0] vidin_out_reg_f2; 
   wire my_fir_rdy_f2; 
   wire[27:0] vidin_out_temp_f3; 
   reg[27:0] vidin_out_reg_f3; 
   wire my_fir_rdy_f3; 
   wire[27:0] vidin_out_temp_h1; 
   reg[27:0] vidin_out_reg_h1; 
   wire my_fir_rdy_h1; 
   wire[27:0] vidin_out_temp_h2; 
   reg[27:0] vidin_out_reg_h2; 
   wire my_fir_rdy_h2; 
   wire[27:0] vidin_out_temp_h3; 
   reg[27:0] vidin_out_reg_h3; 
   wire my_fir_rdy_h3; 
   wire[27:0] vidin_out_temp_h4; 
   reg[27:0] vidin_out_reg_h4; 
   wire my_fir_rdy_h4; 
   wire[28:0] sum_tmp_1; 
   wire[28:0] sum_tmp_2; 

   wire[28:0] sum_tmp_3; 
   wire[28:0] sum_tmp_4; 
   wire[30:0] sum_tmp_5; 
   wire[15:0] real_p; 
   wire[15:0] imag_p; 
   wire[15:0] real_z; 
   wire[15:0] imag_z; 
   wire[15:0] real_n; 
   wire[15:0] imag_n; 
   wire[16:0] tmp; 

   my_fir_f1 your_instance_name_f1 (tm3_clk_v0, vidin_new_data, my_fir_rdy_f1, vidin_in_f2, vidin_out_temp_f1); 
   my_fir_f2 your_instance_name_f2 (tm3_clk_v0, vidin_new_data, my_fir_rdy_f2, vidin_in_f1, vidin_out_temp_f2); 
   my_fir_f3 your_instance_name_f3 (tm3_clk_v0, vidin_new_data, my_fir_rdy_f3, vidin_in_f3, vidin_out_temp_f3); 
   my_fir_h1 your_instance_name_h1 (tm3_clk_v0, vidin_new_data, my_fir_rdy_h1, vidin_in_h2, vidin_out_temp_h1); 
   my_fir_h2 your_instance_name_h2 (tm3_clk_v0, vidin_new_data, my_fir_rdy_h2, vidin_in_h1, vidin_out_temp_h2); 
   my_fir_h3 your_instance_name_h3 (tm3_clk_v0, vidin_new_data, my_fir_rdy_h3, vidin_in_h4, vidin_out_temp_h3); 
   my_fir_h4 your_instance_name_h4 (tm3_clk_v0, vidin_new_data, my_fir_rdy_h4, vidin_in_h3, vidin_out_temp_h4); 
   steer_fltr my_steer_fltr_inst (tm3_clk_v0, vidin_new_data, vidin_out_reg_f1, vidin_out_reg_f2, vidin_out_reg_f3, vidin_out_reg_h1, vidin_out_reg_h2, vidin_out_reg_h3, vidin_out_reg_h4, real_z, imag_z, real_p, imag_p, real_n, imag_n); 

   always @(posedge tm3_clk_v0)
   begin
         if (my_fir_rdy_f1 == 1'b1)
         begin
            vidin_out_reg_f1 <= vidin_out_temp_f1 ; 
         end 
         if (my_fir_rdy_f2 == 1'b1)
         begin
            vidin_out_reg_f2 <= vidin_out_temp_f2 ; 
         end 
         if (my_fir_rdy_f3 == 1'b1)
         begin
            vidin_out_reg_f3 <= vidin_out_temp_f3 ; 
         end 
         if (my_fir_rdy_h1 == 1'b1)
         begin
            vidin_out_reg_h1 <= vidin_out_temp_h1 ; 
         end 
         if (my_fir_rdy_h2 == 1'b1)
         begin
            vidin_out_reg_h2 <= vidin_out_temp_h2 ; 
         end 
         if (my_fir_rdy_h3 == 1'b1)
         begin
            vidin_out_reg_h3 <= vidin_out_temp_h3 ; 

         end 
         if (my_fir_rdy_h4 == 1'b1)
         begin
            vidin_out_reg_h4 <= vidin_out_temp_h4 ; 
         end 
   end 

   always @(posedge tm3_clk_v0)
   begin
         real_z_reg <= real_z ; 
         imag_z_reg <= imag_z ; 
         real_p_reg <= real_p ; 
         imag_p_reg <= imag_p ; 
         real_n_reg <= real_n ; 
         imag_n_reg <= imag_n ; 
   end 
endmodule
module steer_fltr (clk, new_data, f1, f2, f3, h1, h2, h3, h4, re_z, im_z, re_p, im_p, re_n, im_n);

   input clk; 
   input new_data; 
   input[27:0] f1; 
   input[27:0] f2; 
   input[27:0] f3; 
   input[27:0] h1; 
   input[27:0] h2; 
   input[27:0] h3; 
   input[27:0] h4; 
   output[15:0] re_z; 
   reg[15:0] re_z;
   output[15:0] im_z; 
   reg[15:0] im_z;
   output[15:0] re_p; 
   reg[15:0] re_p;
   output[15:0] im_p; 
   reg[15:0] im_p;
   output[15:0] re_n; 
   reg[15:0] re_n;
   output[15:0] im_n; 
   reg[15:0] im_n;

   reg[27:0] f1_reg; 
   reg[27:0] f2_reg; 
   reg[27:0] f3_reg; 
   reg[27:0] h1_reg; 
   reg[27:0] h2_reg; 
   reg[27:0] h3_reg; 
   reg[27:0] h4_reg; 
   reg[28:0] re_z_tmp_1; 
   reg[28:0] im_z_tmp_1; 
   reg[28:0] re_p_tmp_1; 
   reg[28:0] re_p_tmp_2; 
   reg[28:0] re_p_tmp_3; 
   reg[28:0] im_p_tmp_1; 
   reg[28:0] im_p_tmp_2; 
   reg[28:0] im_p_tmp_3; 
   reg[28:0] im_p_tmp_4; 
   reg[30:0] re_z_tmp; 
   reg[30:0] im_z_tmp; 
   reg[30:0] re_p_tmp; 
   reg[30:0] im_p_tmp; 
   reg[30:0] re_n_tmp; 
   reg[30:0] im_n_tmp; 

   always @(posedge clk)
   begin
         if (new_data == 1'b1)
         begin
            f1_reg <= f1 ; 
            f2_reg <= f2 ; 
            f3_reg <= f3 ; 
            h1_reg <= h1 ; 
            h2_reg <= h2 ; 
            h3_reg <= h3 ; 
            h4_reg <= h4 ; 
         end 
   end 

   always @(posedge clk)
   begin
         re_z_tmp_1 <= {f1_reg[27], f1_reg} ; 
         im_z_tmp_1 <= {h1_reg[27], h1_reg} ; 
         re_p_tmp_1 <= {f1_reg[27], f1_reg[27], f1_reg[27:1]} ; 
         re_p_tmp_2 <= {f3_reg[27], f3_reg[27:0]} ; 
         re_p_tmp_3 <= {f2_reg[27], f2_reg[27], f2_reg[27:1]} ; 
         im_p_tmp_1 <= ({h1_reg[27], h1_reg[27], h1_reg[27], h1_reg[27:2]}) + ({h1_reg[27], h1_reg[27], h1_reg[27], h1_reg[27], h1_reg[27:3]}) ; 
         im_p_tmp_2 <= ({h4_reg[27], h4_reg}) + ({h4_reg[27], h4_reg[27], h4_reg[27], h4_reg[27], h4_reg[27], h4_reg[27:4]}) ; 
         im_p_tmp_3 <= ({h3_reg[27], h3_reg}) + ({h3_reg[27], h3_reg[27], h3_reg[27], h3_reg[27], h3_reg[27], h3_reg[27:4]}) ; 
         im_p_tmp_4 <= ({h2_reg[27], h2_reg[27], h2_reg[27], h2_reg[27:2]}) + ({h2_reg[27], h2_reg[27], h2_reg[27], h2_reg[27], h2_reg[27:3]}) ; 
         re_z_tmp <= {re_z_tmp_1[28], re_z_tmp_1[28], re_z_tmp_1} ; 
         im_z_tmp <= {im_z_tmp_1[28], im_z_tmp_1[28], im_z_tmp_1} ; 
         re_p_tmp <= ({re_p_tmp_1[28], re_p_tmp_1[28], re_p_tmp_1}) - ({re_p_tmp_2[28], re_p_tmp_2[28], re_p_tmp_2}) + ({re_p_tmp_3[28], re_p_tmp_3[28], re_p_tmp_3}) ; 
         im_p_tmp <= ({im_p_tmp_1[28], im_p_tmp_1[28], im_p_tmp_1}) - ({im_p_tmp_2[28], im_p_tmp_2[28], im_p_tmp_2}) + ({im_p_tmp_3[28], im_p_tmp_3[28], im_p_tmp_3}) - ({im_p_tmp_4[28], im_p_tmp_4[28], im_p_tmp_4}) ; 
         re_n_tmp <= ({re_p_tmp_1[28], re_p_tmp_1[28], re_p_tmp_1}) + ({re_p_tmp_2[28], re_p_tmp_2[28], re_p_tmp_2}) + ({re_p_tmp_3[28], re_p_tmp_3[28], re_p_tmp_3}) ; 
         im_n_tmp <= ({im_p_tmp_1[28], im_p_tmp_1[28], im_p_tmp_1}) + ({im_p_tmp_2[28], im_p_tmp_2[28], im_p_tmp_2}) + ({im_p_tmp_3[28], im_p_tmp_3[28], im_p_tmp_3}) + ({im_p_tmp_4[28], im_p_tmp_4[28], im_p_tmp_4}) ; 
         re_z <= re_z_tmp[30:15] ; 
         im_z <= im_z_tmp[30:15] ; 
         re_p <= re_p_tmp[30:15] ; 
         im_p <= im_p_tmp[30:15] ; 
         re_n <= re_n_tmp[30:15] ; 
         im_n <= im_n_tmp[30:15] ; 
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
module v_fltr_496x7 (tm3_clk_v0, vidin_new_data, vidin_in, vidin_out_f1, vidin_out_f2, vidin_out_f3, vidin_out_h1, vidin_out_h2, vidin_out_h3, vidin_out_h4); // PAJ var never used, vidin_out_or);

   parameter horiz_length  = 9'b111110000;
   parameter vert_length  = 3'b111; // PAJ constant for all

   input tm3_clk_v0; 
   input vidin_new_data; 
   input[7:0] vidin_in; 
   output[15:0] vidin_out_f1; 
   wire[15:0] vidin_out_f1;
   output[15:0] vidin_out_f2; 
   wire[15:0] vidin_out_f2;
   output[15:0] vidin_out_f3; 
   wire[15:0] vidin_out_f3;
   output[15:0] vidin_out_h1; 
   wire[15:0] vidin_out_h1;
   output[15:0] vidin_out_h2; 
   wire[15:0] vidin_out_h2;
   output[15:0] vidin_out_h3; 
   wire[15:0] vidin_out_h3;
   output[15:0] vidin_out_h4; 
   wire[15:0] vidin_out_h4;
//   output[7:0] vidin_out_or; 
//   reg[7:0] vidin_out_or;

   wire[7:0] buff_out0; 
   wire[7:0] buff_out1; 
   wire[7:0] buff_out2; 
   wire[7:0] buff_out3; 
   wire[7:0] buff_out4; 
   wire[7:0] buff_out5; 
   wire[7:0] buff_out6; 
   wire[7:0] buff_out7; 

 	fifo496 fifo0(tm3_clk_v0, vidin_new_data, buff_out0, buff_out1);
 	fifo496 fifo1(tm3_clk_v0, vidin_new_data, buff_out1, buff_out2);
 	fifo496 fifo2(tm3_clk_v0, vidin_new_data, buff_out2, buff_out3);
 	fifo496 fifo3(tm3_clk_v0, vidin_new_data, buff_out3, buff_out4);
 	fifo496 fifo4(tm3_clk_v0, vidin_new_data, buff_out4, buff_out5);
 	fifo496 fifo5(tm3_clk_v0, vidin_new_data, buff_out5, buff_out6);
 	fifo496 fifo6(tm3_clk_v0, vidin_new_data, buff_out6, buff_out7);

   fltr_compute_f1 inst_fltr_compute_f1 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_f1);	
   fltr_compute_f2 inst_fltr_compute_f2 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_f2);	
   fltr_compute_f3 inst_fltr_compute_f3 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_f3);	
   fltr_compute_h1 inst_fltr_compute_h1 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_h1);	
   fltr_compute_h2 inst_fltr_compute_h2 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_h2);	
   fltr_compute_h3 inst_fltr_compute_h3 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_h3);	
   fltr_compute_h4 inst_fltr_compute_h4 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_h4);	

         assign buff_out0 = vidin_in ; 
/*
   always @(posedge tm3_clk_v0)
   begin
         buff_out0 <= vidin_in ; 
         //vidin_out_or <= buff_out6; 
   end 
*/
endmodule
module v_fltr_316x7 (tm3_clk_v0, vidin_new_data, vidin_in, vidin_out_f1, vidin_out_f2, vidin_out_f3, vidin_out_h1, vidin_out_h2, vidin_out_h3, vidin_out_h4); // PAJ var never used, vidin_out_or);

   parameter horiz_length  = 9'b100111100;
   parameter vert_length  = 3'b111; // PAJ constant for all

   input tm3_clk_v0; 
   input vidin_new_data; 
   input[7:0] vidin_in; 
   output[15:0] vidin_out_f1; 
   wire[15:0] vidin_out_f1;
   output[15:0] vidin_out_f2; 
   wire[15:0] vidin_out_f2;
   output[15:0] vidin_out_f3; 
   wire[15:0] vidin_out_f3;
   output[15:0] vidin_out_h1; 
   wire[15:0] vidin_out_h1;
   output[15:0] vidin_out_h2; 
   wire[15:0] vidin_out_h2;
   output[15:0] vidin_out_h3; 
   wire[15:0] vidin_out_h3;
   output[15:0] vidin_out_h4; 
   wire[15:0] vidin_out_h4;
//   output[7:0] vidin_out_or; 
//   reg[7:0] vidin_out_or;

   wire[7:0] buff_out0; 
   wire[7:0] buff_out1; 
   wire[7:0] buff_out2; 
   wire[7:0] buff_out3; 
   wire[7:0] buff_out4; 
   wire[7:0] buff_out5; 
   wire[7:0] buff_out6; 
   wire[7:0] buff_out7; 

 	fifo316 fifo0(tm3_clk_v0, vidin_new_data, buff_out0, buff_out1);
 	fifo316 fifo1(tm3_clk_v0, vidin_new_data, buff_out1, buff_out2);
 	fifo316 fifo2(tm3_clk_v0, vidin_new_data, buff_out2, buff_out3);
 	fifo316 fifo3(tm3_clk_v0, vidin_new_data, buff_out3, buff_out4);
 	fifo316 fifo4(tm3_clk_v0, vidin_new_data, buff_out4, buff_out5);
 	fifo316 fifo5(tm3_clk_v0, vidin_new_data, buff_out5, buff_out6);
 	fifo316 fifo6(tm3_clk_v0, vidin_new_data, buff_out6, buff_out7);

   fltr_compute_f1 inst_fltr_compute_f1 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_f1);	
   fltr_compute_f2 inst_fltr_compute_f2 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_f2);	
   fltr_compute_f3 inst_fltr_compute_f3 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_f3);	
   fltr_compute_h1 inst_fltr_compute_h1 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_h1);	
   fltr_compute_h2 inst_fltr_compute_h2 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_h2);	
   fltr_compute_h3 inst_fltr_compute_h3 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_h3);	
   fltr_compute_h4 inst_fltr_compute_h4 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_h4);	

         assign buff_out0 = vidin_in ; 
/*
   always @(posedge tm3_clk_v0)
   begin
         buff_out0 <= vidin_in ; 
         //vidin_out_or <= buff_out6; 
   end 
*/
endmodule
module v_fltr_226x7 (tm3_clk_v0, vidin_new_data, vidin_in, vidin_out_f1, vidin_out_f2, vidin_out_f3, vidin_out_h1, vidin_out_h2, vidin_out_h3, vidin_out_h4); // PAJ var never used, vidin_out_or);

   parameter horiz_length  = 8'b11100010;
   parameter vert_length  = 3'b111; // PAJ constant for all

   input tm3_clk_v0; 
   input vidin_new_data; 
   input[7:0] vidin_in; 
   output[15:0] vidin_out_f1; 
   wire[15:0] vidin_out_f1;
   output[15:0] vidin_out_f2; 
   wire[15:0] vidin_out_f2;
   output[15:0] vidin_out_f3; 
   wire[15:0] vidin_out_f3;
   output[15:0] vidin_out_h1; 
   wire[15:0] vidin_out_h1;
   output[15:0] vidin_out_h2; 
   wire[15:0] vidin_out_h2;
   output[15:0] vidin_out_h3; 
   wire[15:0] vidin_out_h3;
   output[15:0] vidin_out_h4; 
   wire[15:0] vidin_out_h4;
//   output[7:0] vidin_out_or; 
//   reg[7:0] vidin_out_or;

   wire[7:0] buff_out0; 
   wire[7:0] buff_out1; 
   wire[7:0] buff_out2; 
   wire[7:0] buff_out3; 
   wire[7:0] buff_out4; 
   wire[7:0] buff_out5; 
   wire[7:0] buff_out6; 
   wire[7:0] buff_out7; 

 	fifo226 fifo0(tm3_clk_v0, vidin_new_data, buff_out0, buff_out1);
 	fifo226 fifo1(tm3_clk_v0, vidin_new_data, buff_out1, buff_out2);
 	fifo226 fifo2(tm3_clk_v0, vidin_new_data, buff_out2, buff_out3);
 	fifo226 fifo3(tm3_clk_v0, vidin_new_data, buff_out3, buff_out4);
 	fifo226 fifo4(tm3_clk_v0, vidin_new_data, buff_out4, buff_out5);
 	fifo226 fifo5(tm3_clk_v0, vidin_new_data, buff_out5, buff_out6);
 	fifo226 fifo6(tm3_clk_v0, vidin_new_data, buff_out6, buff_out7);

   fltr_compute_f1 inst_fltr_compute_f1 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_f1);	
   fltr_compute_f2 inst_fltr_compute_f2 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_f2);	
   fltr_compute_f3 inst_fltr_compute_f3 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_f3);	
   fltr_compute_h1 inst_fltr_compute_h1 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_h1);	
   fltr_compute_h2 inst_fltr_compute_h2 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_h2);	
   fltr_compute_h3 inst_fltr_compute_h3 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_h3);	
   fltr_compute_h4 inst_fltr_compute_h4 (tm3_clk_v0, {buff_out1, buff_out2, buff_out3, buff_out4, buff_out5, buff_out6, buff_out7}, vidin_out_h4);	

         assign buff_out0 = vidin_in ; 
/*
   always @(posedge tm3_clk_v0)
   begin
         buff_out0 <= vidin_in ; 
         //vidin_out_or <= buff_out6; 
   end 
*/
endmodule
module fltr_compute_f1 (clk, din, dout);

    input clk; 
    input[55:0] din; 
    output[15:0] dout; 
    reg[15:0] dout;
    reg[16:0] q1; 
    reg[16:0] q2; 
    reg[16:0] q3; 
    reg[16:0] q4; 
    reg[16:0] q5; 
    reg[16:0] q6; 
    reg[16:0] q7; 
    reg[19:0] d_out_tmp; 

    always @(posedge clk)
    begin
			// PAJ - grabbed these from the mult_const declarations
			q1 <= din[55:48] * 5'b11101;
			q2 <= din[47:40] * 7'b1100101;
			q3 <= din[39:32] * 5'b10001;
			q4 <= din[31:24] * 9'b100010101;
			q5 <= din[23:16] * 5'b10001;
			q6 <= din[15:8] * 7'b1100101;
			q7 <= din[7:0] * 5'b11101;

          d_out_tmp <= ({q1[16], q1[16], q1[16], q1}) + ({q2[16], q2[16], q2[16], q2}) + ({q3[16], q3[16], q3[16], q3}) + ({q4[16], q4[16], q4[16], q4}) + ({q5[16], q5[16], q5[16], q5}) + ({q6[16], q6[16], q6[16], q6}) + ({q7[16], q7[16], q7[16], q7});
          dout <= d_out_tmp[18:3] ; 
    end 
 endmodule
module fltr_compute_f2 (clk, din, dout);

    input clk; 
    input[55:0] din; 
    output[15:0] dout; 
    reg[15:0] dout;
    reg[16:0] q1; 
    reg[16:0] q2; 
    reg[16:0] q3; 
    reg[16:0] q4; 
    reg[16:0] q5; 
    reg[16:0] q6; 
    reg[16:0] q7; 
    reg[19:0] d_out_tmp; 

    always @(posedge clk)
    begin
			// PAJ - grabbed these from the mult_const declarations
			q1 <= din[55:48] * 3'b100;
			q2 <= din[47:40] * 6'b101010;
			q3 <= din[39:32] * 8'b10100011;
			q4 <= din[31:24] * 8'b11111111;
			q5 <= din[23:16] * 8'b10100011;
			q6 <= din[15:8] * 6'b101010;
			q7 <= din[7:0] * 3'b100;

          d_out_tmp <= ({q1[16], q1[16], q1[16], q1}) + ({q2[16], q2[16], q2[16], q2}) + ({q3[16], q3[16], q3[16], q3}) + ({q4[16], q4[16], q4[16], q4}) + ({q5[16], q5[16], q5[16], q5}) + ({q6[16], q6[16], q6[16], q6}) + ({q7[16], q7[16], q7[16], q7});
          dout <= d_out_tmp[18:3] ; 
    end 
 endmodule









module fltr_compute_f3 (clk, din, dout);

    input clk; 
    input[55:0] din; 
    output[15:0] dout; 
    reg[15:0] dout;
    reg[16:0] q1; 
    reg[16:0] q2; 
    reg[16:0] q3; 
    reg[16:0] q4; 
    reg[16:0] q5; 
    reg[16:0] q6; 
    reg[16:0] q7; 
    reg[19:0] d_out_tmp; 

    always @(posedge clk)
    begin
			// PAJ - grabbed these from the mult_const declarations
			q1 <= din[55:48] * 5'b10100;
			q2 <= din[47:40] * 8'b10110011;
			q3 <= din[39:32] * 9'b101101100;
			q4 <= din[31:24] * 16'b0000000000000000;
			q5 <= din[23:16] * 9'b101101100;
			q6 <= din[15:8] * 8'b10110011;
			q7 <= din[7:0] * 5'b10100;

          d_out_tmp <= ({q1[16], q1[16], q1[16], q1}) + ({q2[16], q2[16], q2[16], q2}) + ({q3[16], q3[16], q3[16], q3}) + ({q4[16], q4[16], q4[16], q4}) + ({q5[16], q5[16], q5[16], q5}) + ({q6[16], q6[16], q6[16], q6}) + ({q7[16], q7[16], q7[16], q7});
          dout <= d_out_tmp[18:3] ; 
    end 

 endmodule
module fltr_compute_h1 (clk, din, dout);

    input clk; 
    input[55:0] din; 
    output[15:0] dout; 
    reg[15:0] dout;
    reg[16:0] q1; 
    reg[16:0] q2; 
    reg[16:0] q3; 
    reg[16:0] q4; 
    reg[16:0] q5; 
    reg[16:0] q6; 
    reg[16:0] q7; 
    reg[19:0] d_out_tmp; 

    always @(posedge clk)
    begin
			// PAJ - grabbed these from the mult_const declarations
			q1 <= din[55:48] * 5'b10001;
			q2 <= din[47:40] * 5'b11001;
			q3 <= din[39:32] * 8'b11000001;
			q4 <= din[31:24] * 16'b0000000000000000;
			q5 <= din[23:16] * 8'b11000001;
			q6 <= din[15:8] * 5'b11001;
			q7 <= din[7:0] * 5'b10001;

          d_out_tmp <= ({q1[16], q1[16], q1[16], q1}) + ({q2[16], q2[16], q2[16], q2}) + ({q3[16], q3[16], q3[16], q3}) + ({q4[16], q4[16], q4[16], q4}) + ({q5[16], q5[16], q5[16], q5}) + ({q6[16], q6[16], q6[16], q6}) + ({q7[16], q7[16], q7[16], q7});
          dout <= d_out_tmp[18:3] ; 
    end 
 endmodule


module fltr_compute_h2 (clk, din, dout);

   input clk; 
   input[55:0] din; 
   output[15:0] dout; 
   reg[15:0] dout;

   reg[16:0] q1; 
   reg[16:0] q2; 
   reg[16:0] q3; 
   reg[16:0] q4; 
   reg[16:0] q5; 
   reg[16:0] q6; 
   reg[16:0] q7; 
   reg[19:0] d_out_tmp; 

   always @(posedge clk)
   begin
			// PAJ - grabbed these from the mult_const declarations
			q1 <= din[55:48] * 3'b100;
			q2 <= din[47:40] * 6'b101010;
			q3 <= din[39:32] * 8'b10100011;
			q4 <= din[31:24] * 8'b11111111;
			q5 <= din[23:16] * 8'b10100011;
			q6 <= din[15:8] * 6'b101010;
			q7 <= din[7:0] * 3'b100;

          d_out_tmp <= ({q1[16], q1[16], q1[16], q1}) + ({q2[16], q2[16], q2[16], q2}) + ({q3[16], q3[16], q3[16], q3}) + ({q4[16], q4[16], q4[16], q4}) + ({q5[16], q5[16], q5[16], q5}) + ({q6[16], q6[16], q6[16], q6}) + ({q7[16], q7[16], q7[16], q7});
         dout <= d_out_tmp[18:3] ; 
   end 
endmodule







module fltr_compute_h3 (clk, din, dout);

   input clk; 
   input[55:0] din; 
   output[15:0] dout; 
   reg[15:0] dout;

   reg[16:0] q1; 
   reg[16:0] q2; 
   reg[16:0] q3; 
   reg[16:0] q4; 
   reg[16:0] q5; 
   reg[16:0] q6; 
   reg[16:0] q7; 
   reg[19:0] d_out_tmp; 

   always @(posedge clk)
   begin
			// PAJ - grabbed these from the mult_const declarations
			q1 <= din[55:48] * 5'b10111;
			q2 <= din[47:40] * 7'b1001000;
			q3 <= din[39:32] * 8'b10010011;
			q4 <= din[31:24] * 16'b0000000000000000;
			q5 <= din[23:16] * 8'b10010011;
			q6 <= din[15:8] * 7'b1001000;
			q7 <= din[7:0] * 5'b10111;

          d_out_tmp <= ({q1[16], q1[16], q1[16], q1}) + ({q2[16], q2[16], q2[16], q2}) + ({q3[16], q3[16], q3[16], q3}) + ({q4[16], q4[16], q4[16], q4}) + ({q5[16], q5[16], q5[16], q5}) + ({q6[16], q6[16], q6[16], q6}) + ({q7[16], q7[16], q7[16], q7});
         dout <= d_out_tmp[18:3] ; 
   end 
endmodule




module fltr_compute_h4 (clk, din, dout);

   input clk; 
   input[55:0] din; 
   output[15:0] dout; 
   reg[15:0] dout;

   reg[16:0] q1; 
   reg[16:0] q2; 
   reg[16:0] q3; 
   reg[16:0] q4; 
   reg[16:0] q5; 
   reg[16:0] q6; 
   reg[16:0] q7; 
   reg[19:0] d_out_tmp; 

   always @(posedge clk)
   begin
			// PAJ - grabbed these from the mult_const declarations
			q1 <= din[55:48] * 4'b1110;
			q2 <= din[47:40] * 6'b101011;
			q3 <= din[39:32] * 7'b1010000;
			q4 <= din[31:24] * 9'b101000100;
			q5 <= din[23:16] * 7'b1010000;
			q6 <= din[15:8] * 6'b101011;
			q7 <= din[7:0] * 4'b1110;

          d_out_tmp <= ({q1[16], q1[16], q1[16], q1}) + ({q2[16], q2[16], q2[16], q2}) + ({q3[16], q3[16], q3[16], q3}) + ({q4[16], q4[16], q4[16], q4}) + ({q5[16], q5[16], q5[16], q5}) + ({q6[16], q6[16], q6[16], q6}) + ({q7[16], q7[16], q7[16], q7});
         dout <= d_out_tmp[18:3] ; 
   end 
endmodule






// Discription: this block creates a long fifo

 // of  lengh of the specified length as an input 
 // parameter and with a specified width

 // date: Oct.7 ,2001
 // By:  Ahmad darabiha

module fifo496 (clk, wen, din, dout);

	parameter WIDTH = 4'b1000;

    input clk; 
    input wen; 
    input[WIDTH - 1:0] din; 
    output[WIDTH - 1:0] dout; 
    reg[WIDTH - 1:0] dout;

    reg[WIDTH-1:0]buff1; 
    reg[WIDTH-1:0]buff2; 

    always @(posedge clk)
    begin
		if (wen == 1'b1)
		begin
			buff1 <= din;
			buff2 <= buff1;
			dout <= buff2;
		end
	end
endmodule

module fifo316 (clk, wen, din, dout);

	parameter WIDTH = 4'b1000;

    input clk; 
    input wen; 
    input[WIDTH - 1:0] din; 
    output[WIDTH - 1:0] dout; 
    reg[WIDTH - 1:0] dout;

    reg[WIDTH-1:0]buff1; 
    reg[WIDTH-1:0]buff2; 

    always @(posedge clk)
    begin
		if (wen == 1'b1)
		begin
			buff1 <= din;
			buff2 <= buff1;
			dout <= buff2;
		end
	end
endmodule

module fifo226 (clk, wen, din, dout);

	parameter WIDTH = 4'b1000;

    input clk; 
    input wen; 
    input[WIDTH - 1:0] din; 
    output[WIDTH - 1:0] dout; 
    reg[WIDTH - 1:0] dout;

    reg[WIDTH-1:0]buff1; 
    reg[WIDTH-1:0]buff2; 

    always @(posedge clk)
    begin
		if (wen == 1'b1)
		begin
			buff1 <= din;
			buff2 <= buff1;
			dout <= buff2;
		end
	end
endmodule

module port_bus_2to1 (clk, vidin_addr_reg, svid_comp_switch, vidin_new_data_scld_1_2to3_left, vidin_data_reg_scld_1_2to3_left_rp, vidin_data_reg_scld_1_2to3_left_ip, vidin_data_reg_scld_1_2to3_left_rn, vidin_data_reg_scld_1_2to3_left_in, vidin_data_reg_scld_2_2to3_left_rp, vidin_data_reg_scld_2_2to3_left_ip, vidin_data_reg_scld_2_2to3_left_rn, vidin_data_reg_scld_2_2to3_left_in, vidin_data_reg_scld_4_2to3_left_rp, vidin_data_reg_scld_4_2to3_left_ip, vidin_data_reg_scld_4_2to3_left_rn, vidin_data_reg_scld_4_2to3_left_in, vidin_data_reg_scld_1_2to3_right_rp, vidin_data_reg_scld_1_2to3_right_ip, vidin_data_reg_scld_1_2to3_right_rn, vidin_data_reg_scld_1_2to3_right_in, vidin_data_reg_scld_2_2to3_right_rp, vidin_data_reg_scld_2_2to3_right_ip, vidin_data_reg_scld_2_2to3_right_rn, vidin_data_reg_scld_2_2to3_right_in, vidin_data_reg_scld_4_2to3_right_rp, vidin_data_reg_scld_4_2to3_right_ip, vidin_data_reg_scld_4_2to3_right_rn, vidin_data_reg_scld_4_2to3_right_in, bus_word_3, bus_word_4, bus_word_5, bus_word_6, counter_out); 
   input clk; 
   input[18:0] vidin_addr_reg; 
   input svid_comp_switch; 
   input vidin_new_data_scld_1_2to3_left; 
   input[15:0] vidin_data_reg_scld_1_2to3_left_rp; 
   input[15:0] vidin_data_reg_scld_1_2to3_left_ip; 
   input[15:0] vidin_data_reg_scld_1_2to3_left_rn; 
   input[15:0] vidin_data_reg_scld_1_2to3_left_in; 
   input[15:0] vidin_data_reg_scld_2_2to3_left_rp; 
   input[15:0] vidin_data_reg_scld_2_2to3_left_ip; 
   input[15:0] vidin_data_reg_scld_2_2to3_left_rn; 
   input[15:0] vidin_data_reg_scld_2_2to3_left_in; 
   input[15:0] vidin_data_reg_scld_4_2to3_left_rp; 
   input[15:0] vidin_data_reg_scld_4_2to3_left_ip; 
   input[15:0] vidin_data_reg_scld_4_2to3_left_rn; 
   input[15:0] vidin_data_reg_scld_4_2to3_left_in; 
   input[15:0] vidin_data_reg_scld_1_2to3_right_rp; 
   input[15:0] vidin_data_reg_scld_1_2to3_right_ip; 
   input[15:0] vidin_data_reg_scld_1_2to3_right_rn; 
   input[15:0] vidin_data_reg_scld_1_2to3_right_in; 
   input[15:0] vidin_data_reg_scld_2_2to3_right_rp; 
   input[15:0] vidin_data_reg_scld_2_2to3_right_ip; 
   input[15:0] vidin_data_reg_scld_2_2to3_right_rn; 
   input[15:0] vidin_data_reg_scld_2_2to3_right_in; 
   input[15:0] vidin_data_reg_scld_4_2to3_right_rp; 
   input[15:0] vidin_data_reg_scld_4_2to3_right_ip; 
   input[15:0] vidin_data_reg_scld_4_2to3_right_rn; 
   input[15:0] vidin_data_reg_scld_4_2to3_right_in; 
   output[15:0] bus_word_3; 
   reg[15:0] bus_word_3;
   output[15:0] bus_word_4; 
   reg[15:0] bus_word_4;
   output[15:0] bus_word_5; 
   reg[15:0] bus_word_5;
   output[15:0] bus_word_6; 
   reg[15:0] bus_word_6;
   output[2:0] counter_out; 
   reg[2:0] counter_out;

   reg[15:0] bus_word_3_tmp; 
   reg[15:0] bus_word_4_tmp; 
   reg[15:0] bus_word_5_tmp; 
   reg[15:0] bus_word_6_tmp; 
   reg[18:0] vidin_addr_reg_tmp; 
   reg svid_comp_switch_tmp; 
   wire vidin_new_data_scld_1_2to3_left_tmp; 
   wire vidin_new_data_scld_2_2to3_left_tmp; 
   wire vidin_new_data_scld_4_2to3_left_tmp; 
   wire vidin_new_data_scld_1_2to3_right_tmp; 
   wire vidin_new_data_scld_2_2to3_right_tmp; 
   wire vidin_new_data_scld_4_2to3_right_tmp; 
   reg[3:0] counter; 
   reg[2:0] counter_out_tmp; 

   reg[15:0] vidin_data_reg_scld_1_2to3_left_rp_tmp; 
   reg[15:0] vidin_data_reg_scld_2_2to3_left_rp_tmp; 
   reg[15:0] vidin_data_reg_scld_4_2to3_left_rp_tmp; 
   reg[15:0] vidin_data_reg_scld_1_2to3_right_rp_tmp; 
   reg[15:0] vidin_data_reg_scld_2_2to3_right_rp_tmp; 
   reg[15:0] vidin_data_reg_scld_4_2to3_right_rp_tmp; 
   reg[15:0] vidin_data_reg_scld_1_2to3_left_ip_tmp; 
   reg[15:0] vidin_data_reg_scld_2_2to3_left_ip_tmp; 
   reg[15:0] vidin_data_reg_scld_4_2to3_left_ip_tmp; 
   reg[15:0] vidin_data_reg_scld_1_2to3_right_ip_tmp; 
   reg[15:0] vidin_data_reg_scld_2_2to3_right_ip_tmp; 
   reg[15:0] vidin_data_reg_scld_4_2to3_right_ip_tmp; 
   reg[15:0] vidin_data_reg_scld_1_2to3_left_rn_tmp; 
   reg[15:0] vidin_data_reg_scld_2_2to3_left_rn_tmp; 
   reg[15:0] vidin_data_reg_scld_4_2to3_left_rn_tmp; 
   reg[15:0] vidin_data_reg_scld_1_2to3_right_rn_tmp; 
   reg[15:0] vidin_data_reg_scld_2_2to3_right_rn_tmp; 
   reg[15:0] vidin_data_reg_scld_4_2to3_right_rn_tmp; 
   reg[15:0] vidin_data_reg_scld_1_2to3_left_in_tmp; 
   reg[15:0] vidin_data_reg_scld_2_2to3_left_in_tmp; 
   reg[15:0] vidin_data_reg_scld_4_2to3_left_in_tmp; 
   reg[15:0] vidin_data_reg_scld_1_2to3_right_in_tmp; 
   reg[15:0] vidin_data_reg_scld_2_2to3_right_in_tmp; 
   reg[15:0] vidin_data_reg_scld_4_2to3_right_in_tmp; 

   always @(posedge clk)
   begin
         if (vidin_new_data_scld_1_2to3_left == 1'b1)
         begin
            counter <= 4'b0001 ; 
         end
         else
         begin
            case (counter)
               4'b0000 :
                        begin
                           counter <= 4'b1000 ; 
                        end
               4'b0001 :
                        begin
                           counter <= 4'b0010 ; 
                        end
               4'b0010 :
                        begin
                           counter <= 4'b0011 ; 
                        end
               4'b0011 :
                        begin
                           counter <= 4'b0100 ; 
                        end
               4'b0100 :
                        begin
                           counter <= 4'b0101 ; 
                        end
               4'b0101 :
                        begin
                           counter <= 4'b0110 ; 
                        end
               4'b0110 :
                        begin
                           counter <= 4'b0111 ; 
                        end
               4'b0111 :
                        begin
                           counter <= 4'b1000 ; 
                        end
               4'b1000 :
                        begin
                           counter <= 4'b1000 ; 
                        end
               default :
                        begin
                           counter <= 4'b1000 ; 
                        end
            endcase 
         end 
   end 

   always @(posedge clk)
   begin
         case (counter[2:0])
            3'b000 :
                     begin
                        counter_out_tmp <= 3'b000 ; 
                        bus_word_3_tmp <= 16'b0000000000000000 ; 
                        bus_word_4_tmp <= 16'b0000000000000000 ; 
                        bus_word_5_tmp <= 16'b0000000000000000 ; 
                        bus_word_6_tmp <= 16'b0000000000000000 ; 
                     end
            3'b001 :
                     begin
                        counter_out_tmp <= 3'b001 ; 
                        bus_word_3_tmp <= vidin_addr_reg_tmp[15:0] ; 
                        bus_word_4_tmp <= {vidin_addr_reg_tmp[18:16], svid_comp_switch_tmp, 12'b000000000000} ; 
                        bus_word_5_tmp <= 16'b0000000000000000 ; 
                        bus_word_6_tmp <= 16'b0000000000000000 ; 
                     end
            3'b010 :
                     begin
                        counter_out_tmp <= 3'b010 ; 
                        bus_word_3_tmp <= vidin_data_reg_scld_1_2to3_left_rp_tmp ; 
                        bus_word_4_tmp <= vidin_data_reg_scld_1_2to3_left_ip_tmp ; 
                        bus_word_5_tmp <= vidin_data_reg_scld_1_2to3_left_rn_tmp ; 
                        bus_word_6_tmp <= vidin_data_reg_scld_1_2to3_left_in_tmp ; 
                     end
            3'b011 :
                     begin
                        counter_out_tmp <= 3'b011 ; 
                        bus_word_3_tmp <= vidin_data_reg_scld_1_2to3_right_rp_tmp ; 
                        bus_word_4_tmp <= vidin_data_reg_scld_1_2to3_right_ip_tmp ; 
                        bus_word_5_tmp <= vidin_data_reg_scld_1_2to3_right_rn_tmp ; 
                        bus_word_6_tmp <= vidin_data_reg_scld_1_2to3_right_in_tmp ; 
                     end
            3'b100 :
                     begin
                        counter_out_tmp <= 3'b100 ; 
                        bus_word_3_tmp <= vidin_data_reg_scld_2_2to3_left_rp_tmp ; 
                        bus_word_4_tmp <= vidin_data_reg_scld_2_2to3_left_ip_tmp ; 
                        bus_word_5_tmp <= vidin_data_reg_scld_2_2to3_left_rn_tmp ; 
                        bus_word_6_tmp <= vidin_data_reg_scld_2_2to3_left_in_tmp ; 
                     end
            3'b101 :
                     begin
                        counter_out_tmp <= 3'b101 ; 
                        bus_word_3_tmp <= vidin_data_reg_scld_2_2to3_right_rp_tmp ; 
                        bus_word_4_tmp <= vidin_data_reg_scld_2_2to3_right_ip_tmp ; 
                        bus_word_5_tmp <= vidin_data_reg_scld_2_2to3_right_rn_tmp ; 
                        bus_word_6_tmp <= vidin_data_reg_scld_2_2to3_right_in_tmp ; 
                     end

            3'b110 :
                     begin
                        counter_out_tmp <= 3'b110 ; 
                        bus_word_3_tmp <= vidin_data_reg_scld_4_2to3_left_rp_tmp ; 
                        bus_word_4_tmp <= vidin_data_reg_scld_4_2to3_left_ip_tmp ; 
                        bus_word_5_tmp <= vidin_data_reg_scld_4_2to3_left_rn_tmp ; 
                        bus_word_6_tmp <= vidin_data_reg_scld_4_2to3_left_in_tmp ; 
                     end
            3'b111 :
                     begin
                        counter_out_tmp <= 3'b111 ; 
                        bus_word_3_tmp <= vidin_data_reg_scld_4_2to3_right_rp_tmp ; 
                        bus_word_4_tmp <= vidin_data_reg_scld_4_2to3_right_ip_tmp ; 
                        bus_word_5_tmp <= vidin_data_reg_scld_4_2to3_right_rn_tmp ; 
                        bus_word_6_tmp <= vidin_data_reg_scld_4_2to3_right_in_tmp ; 
                     end
         endcase 
   end 

   always @(posedge clk)
   begin
         counter_out <= counter_out_tmp ; 
         bus_word_3 <= bus_word_3_tmp ; 
         bus_word_4 <= bus_word_4_tmp ; 
         bus_word_5 <= bus_word_5_tmp ; 
         bus_word_6 <= bus_word_6_tmp ; 
         if (vidin_new_data_scld_1_2to3_left == 1'b1)
         begin
            vidin_addr_reg_tmp <= vidin_addr_reg ; 
            svid_comp_switch_tmp <= svid_comp_switch ; 
            vidin_data_reg_scld_1_2to3_left_rp_tmp <= vidin_data_reg_scld_1_2to3_left_rp ; 
            vidin_data_reg_scld_2_2to3_left_rp_tmp <= vidin_data_reg_scld_2_2to3_left_rp ; 
            vidin_data_reg_scld_4_2to3_left_rp_tmp <= vidin_data_reg_scld_4_2to3_left_rp ; 
            vidin_data_reg_scld_1_2to3_right_rp_tmp <= vidin_data_reg_scld_1_2to3_right_rp ; 
            vidin_data_reg_scld_2_2to3_right_rp_tmp <= vidin_data_reg_scld_2_2to3_right_rp ; 
            vidin_data_reg_scld_4_2to3_right_rp_tmp <= vidin_data_reg_scld_4_2to3_right_rp ; 
            vidin_data_reg_scld_1_2to3_left_ip_tmp <= vidin_data_reg_scld_1_2to3_left_ip ; 
            vidin_data_reg_scld_2_2to3_left_ip_tmp <= vidin_data_reg_scld_2_2to3_left_ip ; 
            vidin_data_reg_scld_4_2to3_left_ip_tmp <= vidin_data_reg_scld_4_2to3_left_ip ; 
            vidin_data_reg_scld_1_2to3_right_ip_tmp <= vidin_data_reg_scld_1_2to3_right_ip ; 
            vidin_data_reg_scld_2_2to3_right_ip_tmp <= vidin_data_reg_scld_2_2to3_right_ip ; 
            vidin_data_reg_scld_4_2to3_right_ip_tmp <= vidin_data_reg_scld_4_2to3_right_ip ; 
            vidin_data_reg_scld_1_2to3_left_rn_tmp <= vidin_data_reg_scld_1_2to3_left_rn ; 
            vidin_data_reg_scld_2_2to3_left_rn_tmp <= vidin_data_reg_scld_2_2to3_left_rn ; 
            vidin_data_reg_scld_4_2to3_left_rn_tmp <= vidin_data_reg_scld_4_2to3_left_rn ; 
            vidin_data_reg_scld_1_2to3_right_rn_tmp <= vidin_data_reg_scld_1_2to3_right_rn ; 
            vidin_data_reg_scld_2_2to3_right_rn_tmp <= vidin_data_reg_scld_2_2to3_right_rn ; 
            vidin_data_reg_scld_4_2to3_right_rn_tmp <= vidin_data_reg_scld_4_2to3_right_rn ; 
            vidin_data_reg_scld_1_2to3_left_in_tmp <= vidin_data_reg_scld_1_2to3_left_in ; 
            vidin_data_reg_scld_2_2to3_left_in_tmp <= vidin_data_reg_scld_2_2to3_left_in ; 
            vidin_data_reg_scld_4_2to3_left_in_tmp <= vidin_data_reg_scld_4_2to3_left_in ; 
            vidin_data_reg_scld_1_2to3_right_in_tmp <= vidin_data_reg_scld_1_2to3_right_in ; 
            vidin_data_reg_scld_2_2to3_right_in_tmp <= vidin_data_reg_scld_2_2to3_right_in ; 
            vidin_data_reg_scld_4_2to3_right_in_tmp <= vidin_data_reg_scld_4_2to3_right_in ; 
         end 
   end 
endmodule


`define COEF0_b  29
	`define COEF1_b  101
//	`define COEF2_b  -15
//	`define COEF3_b  -235
//	`define COEF4_b  -15
	`define COEF2_b  15
	`define COEF3_b  235
	`define COEF4_b  15
	`define COEF5_b  101
	`define COEF6_b  29
 
module my_fir_f1 (clk, new_data_rdy, output_data_ready, din, dout);

//coefdata=29,101,-15,-235,-15,101,29;

	parameter WIDTH = 5'b10000;

    input clk; 
    input[WIDTH - 1:0] din; 
    output[28 - 1:0] dout; 
    reg[28 - 1:0] dout;
	input new_data_rdy;
	output output_data_ready;
	reg output_data_ready;

    reg[WIDTH - 1:0]n_delay_reg1;
    reg[WIDTH - 1:0]n_delay_reg2;
    reg[WIDTH - 1:0]n_delay_reg3;
    reg[WIDTH - 1:0]n_delay_reg4;
    reg[WIDTH - 1:0]n_delay_reg5;
    reg[WIDTH - 1:0]n_delay_reg6;

    always @(posedge clk)
    begin
		if (new_data_rdy == 1'b1)
		begin
			n_delay_reg1 <= din;
			n_delay_reg2 <= n_delay_reg1;
			n_delay_reg3 <= n_delay_reg2;
			n_delay_reg4 <= n_delay_reg3;
			n_delay_reg5 <= n_delay_reg4;
			n_delay_reg6 <= n_delay_reg5;
		
			output_data_ready <= 1'b1;
			dout <= (din * `COEF0_b) + 
				(n_delay_reg1 * `COEF1_b) + 
				(n_delay_reg2 * `COEF2_b) + 
				(n_delay_reg3 * `COEF3_b) + 
				(n_delay_reg4 * `COEF4_b) + 
				(n_delay_reg5 * `COEF5_b) + 
				(n_delay_reg6 * `COEF6_b);
		end
		else
		begin
			output_data_ready <= 1'b0;
		end
	end
endmodule
	`define COEF0_c  4
	`define COEF1_c  42
	`define COEF2_c  163
	`define COEF3_c  255
	`define COEF4_c  163
	`define COEF5_c  42
	`define COEF6_c  4
module my_fir_f2 (clk, new_data_rdy, output_data_ready, din, dout);

//coefdata=4,42,163,255,163,42,4;


	parameter WIDTH = 5'b10000;

    input clk; 
    input[WIDTH - 1:0] din; 
    output[28 - 1:0] dout; 
    reg[28 - 1:0] dout;
	input new_data_rdy;
	output output_data_ready;
	reg output_data_ready;

    reg[WIDTH - 1:0]n_delay_reg1;
    reg[WIDTH - 1:0]n_delay_reg2;
    reg[WIDTH - 1:0]n_delay_reg3;
    reg[WIDTH - 1:0]n_delay_reg4;
    reg[WIDTH - 1:0]n_delay_reg5;
    reg[WIDTH - 1:0]n_delay_reg6;

    always @(posedge clk)
    begin
		if (new_data_rdy == 1'b1)
		begin
			n_delay_reg1 <= din;
			n_delay_reg2 <= n_delay_reg1;
			n_delay_reg3 <= n_delay_reg2;
			n_delay_reg4 <= n_delay_reg3;
			n_delay_reg5 <= n_delay_reg4;
			n_delay_reg6 <= n_delay_reg5;
		
			output_data_ready <= 1'b1;
			dout <= (din * `COEF0_c) + 
				(n_delay_reg1 * `COEF1_c) + 
				(n_delay_reg2 * `COEF2_c) + 
				(n_delay_reg3 * `COEF3_c) + 
				(n_delay_reg4 * `COEF4_c) + 
				(n_delay_reg5 * `COEF5_c) + 
				(n_delay_reg6 * `COEF6_c);
		end
		else
		begin
			output_data_ready <= 1'b0;
		end
	end
endmodule

//	`define COEF0_d  -12
//	`define COEF1_d  -77
//	`define COEF2_d  -148
	`define COEF0_d  12
	`define COEF1_d  77
	`define COEF2_d  148
	`define COEF3_d  0
	`define COEF4_d  148
	`define COEF5_d  77
	`define COEF6_d  12
module my_fir_f3 (clk, new_data_rdy, output_data_ready, din, dout);

//coefdata=-12,-77,-148,0,148,77,12;

	parameter WIDTH = 5'b10000;

    input clk; 
    input[WIDTH - 1:0] din; 
    output[28 - 1:0] dout; 
    reg[28 - 1:0] dout;
	input new_data_rdy;
	output output_data_ready;
	reg output_data_ready;

    reg[WIDTH - 1:0]n_delay_reg1;
    reg[WIDTH - 1:0]n_delay_reg2;
    reg[WIDTH - 1:0]n_delay_reg3;
    reg[WIDTH - 1:0]n_delay_reg4;
    reg[WIDTH - 1:0]n_delay_reg5;
    reg[WIDTH - 1:0]n_delay_reg6;

    always @(posedge clk)
    begin
		if (new_data_rdy == 1'b1)
		begin
			n_delay_reg1 <= din;
			n_delay_reg2 <= n_delay_reg1;
			n_delay_reg3 <= n_delay_reg2;
			n_delay_reg4 <= n_delay_reg3;
			n_delay_reg5 <= n_delay_reg4;
			n_delay_reg6 <= n_delay_reg5;
		
			output_data_ready <= 1'b1;
			dout <= (din * `COEF0_d) + 
				(n_delay_reg1 * `COEF1_d) + 
				(n_delay_reg2 * `COEF2_d) + 
				(n_delay_reg4 * `COEF4_d) + 
				(n_delay_reg5 * `COEF5_d) + 
				(n_delay_reg6 * `COEF6_d);
		end
		else
		begin
			output_data_ready <= 1'b0;
		end
	end
endmodule
	`define COEF0_1  15
//`define COEF0_1  -15
	`define COEF1_1  25
	`define COEF2_1  193
	`define COEF3_1  0
//	`define COEF4_1  -193
//	`define COEF5_1  -25
	`define COEF4_1  193
	`define COEF5_1  25
	`define COEF6_1  15
module my_fir_h1 (clk, new_data_rdy, output_data_ready, din, dout);

//coefdata=-15,25,193,0,-193,-25,15;


	parameter WIDTH = 5'b10000;

    input clk; 
    input[WIDTH - 1:0] din; 
    output[28 - 1:0] dout; 
    reg[28 - 1:0] dout;
	input new_data_rdy;
	output output_data_ready;
	reg output_data_ready;

    reg[WIDTH - 1:0]n_delay_reg1;
    reg[WIDTH - 1:0]n_delay_reg2;
    reg[WIDTH - 1:0]n_delay_reg3;
    reg[WIDTH - 1:0]n_delay_reg4;
    reg[WIDTH - 1:0]n_delay_reg5;
    reg[WIDTH - 1:0]n_delay_reg6;

    always @(posedge clk)
    begin
		if (new_data_rdy == 1'b1)
		begin
			n_delay_reg1 <= din;
			n_delay_reg2 <= n_delay_reg1;
			n_delay_reg3 <= n_delay_reg2;
			n_delay_reg4 <= n_delay_reg3;
			n_delay_reg5 <= n_delay_reg4;
			n_delay_reg6 <= n_delay_reg5;
		
			output_data_ready <= 1'b1;
			dout <= (din * `COEF0_1) + 
				(n_delay_reg1 * `COEF1_1) + 
				(n_delay_reg2 * `COEF2_1) + 
				(n_delay_reg4 * `COEF4_1) + 
				(n_delay_reg5 * `COEF5_1) + 
				(n_delay_reg6 * `COEF6_1);
		end
		else
		begin
			output_data_ready <= 1'b0;
		end
	end
endmodule

	`define COEF0_2  4
	`define COEF1_2  42
	`define COEF2_2  163
	`define COEF3_2  255
	`define COEF4_2  163
	`define COEF5_2  42
	`define COEF6_2  4
module my_fir_h2 (clk, new_data_rdy, output_data_ready, din, dout);

//coefdata=4,42,163,255,163,42,4;

	parameter WIDTH = 5'b10000;

    input clk; 
    input[WIDTH - 1:0] din; 
    output[28 - 1:0] dout; 
    reg[28 - 1:0] dout;
	input new_data_rdy;
	output output_data_ready;
	reg output_data_ready;

    reg[WIDTH - 1:0]n_delay_reg1;
    reg[WIDTH - 1:0]n_delay_reg2;
    reg[WIDTH - 1:0]n_delay_reg3;
    reg[WIDTH - 1:0]n_delay_reg4;
    reg[WIDTH - 1:0]n_delay_reg5;
    reg[WIDTH - 1:0]n_delay_reg6;

    always @(posedge clk)
    begin
		if (new_data_rdy == 1'b1)
		begin
			n_delay_reg1 <= din;
			n_delay_reg2 <= n_delay_reg1;
			n_delay_reg3 <= n_delay_reg2;
			n_delay_reg4 <= n_delay_reg3;
			n_delay_reg5 <= n_delay_reg4;
			n_delay_reg6 <= n_delay_reg5;
		
			output_data_ready <= 1'b1;
			dout <= (din * `COEF0_2) + 
				(n_delay_reg1 * `COEF1_2) + 
				(n_delay_reg2 * `COEF2_2) + 
				(n_delay_reg3 * `COEF3_2) + 
				(n_delay_reg4 * `COEF4_2) + 
				(n_delay_reg5 * `COEF5_2) + 
				(n_delay_reg6 * `COEF6_2);
		end
		else
		begin
			output_data_ready <= 1'b0;
		end
	end
endmodule

//	`define COEF0_3  -9
//	`define COEF1_3  -56
//	`define COEF2_3  -109
	`define COEF0_3  9
	`define COEF1_3  56
	`define COEF2_3  109
	`define COEF3_3  0
	`define COEF4_3  109
	`define COEF5_3  56
	`define COEF6_3  9


module my_fir_h3 (clk, new_data_rdy, output_data_ready, din, dout);

//coefdata=-9,-56,-109,0,109,56,9;
	parameter WIDTH = 5'b10000;

    input clk; 
    input[WIDTH - 1:0] din; 
    output[28 - 1:0] dout; 
    reg[28 - 1:0] dout;
	input new_data_rdy;
	output output_data_ready;
	reg output_data_ready;

    reg[WIDTH - 1:0]n_delay_reg1;
    reg[WIDTH - 1:0]n_delay_reg2;
    reg[WIDTH - 1:0]n_delay_reg3;
    reg[WIDTH - 1:0]n_delay_reg4;
    reg[WIDTH - 1:0]n_delay_reg5;
    reg[WIDTH - 1:0]n_delay_reg6;

    always @(posedge clk)
    begin
		if (new_data_rdy == 1'b1)
		begin
			n_delay_reg1 <= din;
			n_delay_reg2 <= n_delay_reg1;
			n_delay_reg3 <= n_delay_reg2;
			n_delay_reg4 <= n_delay_reg3;
			n_delay_reg5 <= n_delay_reg4;
			n_delay_reg6 <= n_delay_reg5;
		
			output_data_ready <= 1'b1;
			dout <= (din * `COEF0_3) + 
				(n_delay_reg1 * `COEF1_3) + 
				(n_delay_reg2 * `COEF2_3) + 
				(n_delay_reg4 * `COEF4_3) + 
				(n_delay_reg5 * `COEF5_3) + 
				(n_delay_reg6 * `COEF6_3);
		end
		else
		begin
			output_data_ready <= 1'b0;
		end
	end
endmodule
//	`define COEF0_4  -9
//	`define COEF1_4  -56
//	`define COEF2_4  -109
	`define COEF0_4  9
	`define COEF1_4  56
	`define COEF2_4  109
	`define COEF3_4  0
	`define COEF4_4  109
	`define COEF5_4  56
	`define COEF6_4  9


module my_fir_h4 (clk, new_data_rdy, output_data_ready, din, dout);

//coefdata=-9,-56,-109,0,109,56,9;
		parameter WIDTH = 5'b10000;

    input clk; 
    input[WIDTH - 1:0] din; 
    output[28 - 1:0] dout; 
    reg[28 - 1:0] dout;
	input new_data_rdy;
	output output_data_ready;
	reg output_data_ready;

    reg[WIDTH - 1:0]n_delay_reg1;
    reg[WIDTH - 1:0]n_delay_reg2;
    reg[WIDTH - 1:0]n_delay_reg3;
    reg[WIDTH - 1:0]n_delay_reg4;
    reg[WIDTH - 1:0]n_delay_reg5;
    reg[WIDTH - 1:0]n_delay_reg6;

    always @(posedge clk)
    begin
		if (new_data_rdy == 1'b1)
		begin
			n_delay_reg1 <= din;
			n_delay_reg2 <= n_delay_reg1;
			n_delay_reg3 <= n_delay_reg2;
			n_delay_reg4 <= n_delay_reg3;
			n_delay_reg5 <= n_delay_reg4;
			n_delay_reg6 <= n_delay_reg5;
		
			output_data_ready <= 1'b1;
			dout <= (din * `COEF0_4) + 
				(n_delay_reg1 * `COEF1_4) + 
				(n_delay_reg2 * `COEF2_4) + 
				(n_delay_reg4 * `COEF4_4) + 
				(n_delay_reg5 * `COEF5_4) + 
				(n_delay_reg6 * `COEF6_4);
		end
		else
		begin
			output_data_ready <= 1'b0;
		end
	end
endmodule

