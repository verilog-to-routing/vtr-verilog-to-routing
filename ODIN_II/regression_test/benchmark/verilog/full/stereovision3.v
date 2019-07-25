// originally created by Marcus van Ierssel
// modified by Ahmad Darabiha, Feb. 2002
// this circuit receives the rgbvideo  signal
// from video input and send it to chip #2
// for buffring and processing.
module sv_chip3_hierarchy_no_mem (tm3_clk_v0, tm3_clk_v2, tm3_vidin_llc, tm3_vidin_vs, tm3_vidin_href, tm3_vidin_cref, tm3_vidin_rts0, tm3_vidin_vpo, tm3_vidin_sda, tm3_vidin_scl, vidin_new_data, vidin_rgb_reg, vidin_addr_reg);

   input tm3_clk_v0; 
   input tm3_clk_v2; 
   input tm3_vidin_llc; 
   input tm3_vidin_vs; 
   input tm3_vidin_href; 
   input tm3_vidin_cref; 
   input tm3_vidin_rts0; 
   input[15:0] tm3_vidin_vpo; 
   output tm3_vidin_sda; 
   wire tm3_vidin_sda;
   reg tm3_vidin_sda_xhdl0;
   output tm3_vidin_scl; 
   reg tm3_vidin_scl;
   output vidin_new_data; 
   reg vidin_new_data;
   output[7:0] vidin_rgb_reg; 
   reg[7:0] vidin_rgb_reg;
   output[18:0] vidin_addr_reg; 
   reg[18:0] vidin_addr_reg;

   reg temp_reg1; 
   reg temp_reg2; 
   reg[9:0] horiz; 
   reg[7:0] vert; 
   reg creg1; 
   reg creg2; 
   reg creg3; 
   reg[18:0] vidin_addr_reg1; 
   reg[23:0] vidin_rgb_reg1; 
   reg[23:0] vidin_rgb_reg2; 
   parameter[4:0] reg_prog1 = 5'b00001; 
   parameter[4:0] reg_prog2 = 5'b00010; 
   parameter[4:0] reg_prog3 = 5'b00011; 
   parameter[4:0] reg_prog4 = 5'b00100; 
   parameter[4:0] reg_prog5 = 5'b00101; 
   parameter[4:0] reg_prog6 = 5'b00110; 
   parameter[4:0] reg_prog7 = 5'b00111; 
   parameter[4:0] reg_prog8 = 5'b01000; 
   parameter[4:0] reg_prog9 = 5'b01001; 
   parameter[4:0] reg_prog10 = 5'b01010; 
   parameter[4:0] reg_prog11 = 5'b01011; 
   parameter[4:0] reg_prog12 = 5'b01100; 
   parameter[4:0] reg_prog13 = 5'b01101; 
   parameter[4:0] reg_prog14 = 5'b01110; 
   parameter[4:0] reg_prog15 = 5'b01111; 
   parameter[4:0] reg_prog16 = 5'b10000; 
   parameter[4:0] reg_prog17 = 5'b10001; 
   parameter[4:0] reg_prog18 = 5'b10010; 
   parameter[4:0] reg_prog19 = 5'b10011; 
   parameter[4:0] reg_prog20 = 5'b10100; 
   parameter[4:0] reg_prog21 = 5'b10101; 
   parameter[4:0] reg_prog22 = 5'b10110; 
   parameter[4:0] reg_prog23 = 5'b10111; 
   parameter[4:0] reg_prog24 = 5'b11000; 
   parameter[4:0] reg_prog25 = 5'b11001; 
   parameter[4:0] reg_prog26 = 5'b11010; 
   parameter[4:0] reg_prog_end = 5'b11011; 
   reg rst; 
   reg rst_done; 
   reg[7:0] iicaddr; 
   reg[7:0] iicdata; 
//   wire vidin_llc; 
//   wire vidin_llc_int; 
   reg[6:0] iic_state; 
   reg iic_stop; 
   reg iic_start; 
   reg[4:0] reg_prog_state; 
   reg[4:0] reg_prog_nextstate; 

   assign tm3_vidin_sda = tm3_vidin_sda_xhdl0;

 //  ibuf ibuf_inst (tm3_vidin_llc, vidin_llc_int); 
//   bufg bufg_inst (vidin_llc_int, vidin_llc); 

   // PAJ double clock is trouble...always @(vidin_llc)
   always @(posedge tm3_clk_v0)
   begin
         if (tm3_vidin_href == 1'b0)
         begin
            horiz <= 10'b0000000000 ; 
         end
         else
         begin
            if (tm3_vidin_cref == 1'b0)
            begin
               horiz <= horiz + 1 ; 
            end 
         end 
         if (tm3_vidin_vs == 1'b1)
         begin
            vert <= 8'b00000000 ; 
         end
         else
         begin
            if ((tm3_vidin_href == 1'b0) & (horiz != 10'b0000000000))
            begin
               vert <= vert + 1 ; 
            end 
         end 
         if (tm3_vidin_cref == 1'b1)
         begin
            vidin_rgb_reg1[23:19] <= tm3_vidin_vpo[15:11] ; 
            vidin_rgb_reg1[15:13] <= tm3_vidin_vpo[10:8] ; 
            vidin_rgb_reg1[18:16] <= tm3_vidin_vpo[7:5] ; 
            vidin_rgb_reg1[9:8] <= tm3_vidin_vpo[4:3] ; 
            vidin_rgb_reg1[2:0] <= tm3_vidin_vpo[2:0] ; 
            vidin_rgb_reg2 <= vidin_rgb_reg1 ; 
         end
         else
         begin
            vidin_rgb_reg1[12:10] <= tm3_vidin_vpo[7:5] ; 
            vidin_rgb_reg1[7:3] <= tm3_vidin_vpo[4:0] ; 
            vidin_addr_reg1 <= {vert, tm3_vidin_rts0, horiz} ; 
         end 
   end 

   always @(posedge tm3_clk_v0)
   begin
         creg1 <= tm3_vidin_cref ; 
         creg2 <= creg1 ; 
         creg3 <= creg2 ; 
         if ((creg2 == 1'b0) & (creg3 == 1'b1) & ((vidin_addr_reg1[10]) == 1'b0) & ((vidin_addr_reg1[0]) == 1'b0))
         begin
            vidin_new_data <= 1'b1 ; 
            vidin_rgb_reg <= vidin_rgb_reg2[7:0] ; 
            vidin_addr_reg <= {({2'b00, vidin_addr_reg1[18:11]}), vidin_addr_reg1[9:1]} ; 
         end
         else
         begin
            vidin_new_data <= 1'b0 ; 
         end 
   end 

   always @(posedge tm3_clk_v2)
   begin
	// A: Worked around scope issue by moving iic_stop into every case. 
       case (iic_state)
            7'b0000000 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0000001 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b1 ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0000010 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0000011 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0000100 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0000101 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0000110 :
                     begin
			iic_stop <= 1'b0 ; 
                      	tm3_vidin_sda_xhdl0 <= 1'b1 ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0000111 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b1 ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0001000 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b1 ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0001001 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0001010 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0001011 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0001100 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0001101 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0001110 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b1 ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0001111 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b1 ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0010000 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b1 ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0010001 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0010010 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0010011 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0010101 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0010110 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0010111 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0011000 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0011001 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0011010 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[7] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0011011 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[7] ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0011100 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[7] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0011101 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[6] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0011110 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[6] ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0011111 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[6] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0100000 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[5] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0100001 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[5] ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0100010 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[5] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0100011 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[4] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0100100 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[4] ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0100101 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[4] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0100110 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[3] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0100111 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[3] ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0101000 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[3] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0101001 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[2] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0101010 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[2] ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0101011 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[2] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0101100 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[1] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0101101 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[1] ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0101110 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[1] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0101111 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[0] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0110000 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[0] ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0110001 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicaddr[0] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0110010 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0110011 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0110100 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0110101 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[7] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0110110 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[7] ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0110111 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[7] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0111000 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[6] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0111001 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[6] ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b0111010 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[6] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0111011 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[5] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0111100 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[5] ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
             7'b0111101 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[5] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0111110 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[4] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b0111111 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[4] ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b1000000 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[4] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b1000001 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[3] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b1000010 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[3] ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b1000011 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[3] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b1000100 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[2] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b1000101 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[2] ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b1000110 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[2] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b1000111 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[1] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b1001000 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[1] ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b1001001 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[1] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b1001010 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[0] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b1001011 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[0] ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b1001100 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= iicdata[0] ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b1001101 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b1001110 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b1001111 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b0 ; 
                     end
            7'b1010000 :
                     begin
			iic_stop <= 1'b0 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b0 ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            7'b1010001 :
                     begin
                        iic_stop <= 1'b1 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b1 ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
            default :
                     begin
                        iic_stop <= 1'b1 ; 
                        tm3_vidin_sda_xhdl0 <= 1'b1 ; 
                        tm3_vidin_scl <= 1'b1 ; 
                     end
         endcase 
   end 

   always @(reg_prog_state or iic_stop)
   begin
      case (reg_prog_state)
         reg_prog1 :
                  begin
                     rst_done = 1'b1 ; 
                     iicaddr = 8'b00000000 ; 
                     iicdata = 8'b00000000 ; 
                     iic_start = 1'b1 ; 
                     if (iic_stop == 1'b0)
                     begin
                        reg_prog_nextstate = reg_prog2 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog1 ; 
                     end 

                 end
         reg_prog2 :
                  begin
                     iicaddr = 8'b00000010 ; 
                     iicdata = {5'b11000, 3'b000} ; 
                     iic_start = 1'b0 ; 
                     if (iic_stop == 1'b1)
                     begin
                        reg_prog_nextstate = reg_prog3 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog2 ; 
                     end 
                  end
         reg_prog3 :
                  begin
                     iicaddr = 8'b00000000 ; 
                     iicdata = 8'b00000000 ; 
                     iic_start = 1'b1 ; 
                     if (iic_stop == 1'b0)
                     begin
                        reg_prog_nextstate = reg_prog4 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog3 ; 
                     end 
                  end
         reg_prog4 :
                  begin
                     iicaddr = 8'b00000011 ; 
                     iicdata = 8'b00100011 ; 
                     iic_start = 1'b0 ; 
                     if (iic_stop == 1'b1)
                     begin
                        reg_prog_nextstate = reg_prog5 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog4 ; 
                     end 
                  end
         reg_prog5 :
                  begin
                     iicaddr = 8'b00000000 ; 
                     iicdata = 8'b00000000 ; 
                     iic_start = 1'b1 ; 
                     if (iic_stop == 1'b0)
                     begin
                        reg_prog_nextstate = reg_prog6 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog5 ; 
                     end 
                  end
         reg_prog6 :
                  begin
                     iicaddr = 8'b00000110 ; 
                     iicdata = 8'b11101011 ; 
                     iic_start = 1'b0 ; 
                     if (iic_stop == 1'b1)
                     begin
                        reg_prog_nextstate = reg_prog7 ; 
                     end
                     else
                     begin

                       reg_prog_nextstate = reg_prog6 ; 
                     end 
                  end
         reg_prog7 :
                  begin
                     iicaddr = 8'b00000000 ; 
                     iicdata = 8'b00000000 ; 
                     iic_start = 1'b1 ; 
                     if (iic_stop == 1'b0)
                     begin
                        reg_prog_nextstate = reg_prog8 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog7 ; 
                     end 
                  end
         reg_prog8 :
                  begin
                     iicaddr = 8'b00000111 ; 
                     iicdata = 8'b11100000 ; 
                     iic_start = 1'b0 ; 
                     if (iic_stop == 1'b1)
                     begin
                        reg_prog_nextstate = reg_prog9 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog8 ; 
                     end 
                  end
         reg_prog9 :
                  begin
                     iicaddr = 8'b00000000 ; 
                     iicdata = 8'b00000000 ; 
                     iic_start = 1'b1 ; 
                     if (iic_stop == 1'b0)
                     begin
                        reg_prog_nextstate = reg_prog10 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog9 ; 
                     end 
                  end
         reg_prog10 :
                  begin
                     iicaddr = 8'b00001000 ; 
                     iicdata = 8'b10000000 ; 
                     iic_start = 1'b0 ; 
                     if (iic_stop == 1'b1)
                     begin
                        reg_prog_nextstate = reg_prog11 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog10 ; 
                     end 
                  end
         reg_prog11 :
                  begin
                     iicaddr = 8'b00000000 ; 
                     iicdata = 8'b00000000 ; 
                     iic_start = 1'b1 ; 
                     if (iic_stop == 1'b0)
                     begin
                        reg_prog_nextstate = reg_prog12 ; 
                     end

                    else
                     begin
                        reg_prog_nextstate = reg_prog11 ; 
                     end 
                  end
         reg_prog12 :
                  begin
                     iicaddr = 8'b00001001 ; 
                     iicdata = 8'b00000001 ; 
                     iic_start = 1'b0 ; 
                     if (iic_stop == 1'b1)
                     begin
                        reg_prog_nextstate = reg_prog13 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog12 ; 
                     end 
                  end
         reg_prog13 :
                  begin
                     iicaddr = 8'b00000000 ; 
                     iicdata = 8'b00000000 ; 
                     iic_start = 1'b1 ; 
                     if (iic_stop == 1'b0)
                     begin
                        reg_prog_nextstate = reg_prog14 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog13 ; 
                     end 
                  end
         reg_prog14 :
                  begin
                     iicaddr = 8'b00001010 ; 
                     iicdata = 8'b10000000 ; 
                     iic_start = 1'b0 ; 
                     if (iic_stop == 1'b1)
                     begin
                        reg_prog_nextstate = reg_prog15 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog14 ; 
                     end 
                  end
         reg_prog15 :
                  begin
                     iicaddr = 8'b00000000 ; 
                     iicdata = 8'b00000000 ; 
                     iic_start = 1'b1 ; 
                     if (iic_stop == 1'b0)
                     begin
                        reg_prog_nextstate = reg_prog16 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog15 ; 
                     end 
                  end
         reg_prog16 :
                  begin
                     iicaddr = 8'b00001011 ; 
                     iicdata = 8'b01000111 ; 
                     iic_start = 1'b0 ; 
                     if (iic_stop == 1'b1)

                    begin
                        reg_prog_nextstate = reg_prog17 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog16 ; 
                     end 
                  end
         reg_prog17 :
                  begin
                     iicaddr = 8'b00000000 ; 
                     iicdata = 8'b00000000 ; 
                     iic_start = 1'b1 ; 
                     if (iic_stop == 1'b0)
                     begin
                        reg_prog_nextstate = reg_prog18 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog17 ; 
                     end 
                  end
         reg_prog18 :
                  begin
                     iicaddr = 8'b00001100 ; 
                     iicdata = 8'b01000000 ; 
                     iic_start = 1'b0 ; 
                     if (iic_stop == 1'b1)
                     begin
                        reg_prog_nextstate = reg_prog19 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog18 ; 
                     end 
                  end
         reg_prog19 :
                  begin
                     iicaddr = 8'b00000000 ; 
                     iicdata = 8'b00000000 ; 
                     iic_start = 1'b1 ; 
                     if (iic_stop == 1'b0)
                     begin
                        reg_prog_nextstate = reg_prog20 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog19 ; 
                     end 
                  end
         reg_prog20 :
                  begin
                     iicaddr = 8'b00001110 ; 
                     iicdata = 8'b00000001 ; 
                     iic_start = 1'b0 ; 
                     if (iic_stop == 1'b1)
                     begin
                        reg_prog_nextstate = reg_prog21 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog20 ; 
                     end 
                  end
         reg_prog21 :
                  begin
                      iicaddr = 8'b00000000 ; 
                     iicdata = 8'b00000000 ; 
                     iic_start = 1'b1 ; 
                     if (iic_stop == 1'b0)
                     begin
                        reg_prog_nextstate = reg_prog22 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog21 ; 
                     end 
                  end
         reg_prog22 :
                  begin
                     iicaddr = 8'b00010000 ; 
                     iicdata = 8'b00000000 ; 
                     iic_start = 1'b0 ; 
                     if (iic_stop == 1'b1)
                     begin
                        reg_prog_nextstate = reg_prog23 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog22 ; 
                     end 
                  end
         reg_prog23 :
                  begin
                     iicaddr = 8'b00000000 ; 
                     iicdata = 8'b00000000 ; 
                     iic_start = 1'b1 ; 
                     if (iic_stop == 1'b0)
                     begin
                        reg_prog_nextstate = reg_prog24 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog23 ; 
                     end 
                  end
         reg_prog24 :
                  begin
                     iicaddr = 8'b00010001 ; 
                     iicdata = 8'b00011100 ; 
                     iic_start = 1'b0 ; 
                     if (iic_stop == 1'b1)
                     begin
                        reg_prog_nextstate = reg_prog25 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog24 ; 
                     end 
                  end
         reg_prog25 :
                  begin
                     iicaddr = 8'b00000000 ; 
                     iicdata = 8'b00000000 ; 
                     iic_start = 1'b1 ; 
                     if (iic_stop == 1'b0)
                     begin
                        reg_prog_nextstate = reg_prog26 ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog25 ; 
                     end 
                  end
         reg_prog26 :
                  begin
                     iicaddr = 8'b00010010 ; 
                     iicdata = 8'b00001001 ; 
                     iic_start = 1'b0 ; 
                     if (iic_stop == 1'b1)
                     begin
                        reg_prog_nextstate = reg_prog_end ; 
                     end
                     else
                     begin
                        reg_prog_nextstate = reg_prog26 ; 
                     end 
                  end
         reg_prog_end :
                  begin
                     iicaddr = 8'b00000000 ; 
                     iicdata = 8'b00000000 ; 
                     iic_start = 1'b0 ; 
                     reg_prog_nextstate = reg_prog_end ; 
                  end
      endcase 
   end 

   always @(posedge tm3_clk_v2)
   begin
         if (rst_done == 1'b1)
         begin
            rst <= 1'b1 ; 
         end 
         temp_reg1 <= tm3_vidin_rts0 ; 
         temp_reg2 <= temp_reg1 ; 
         if (rst == 1'b0)
         begin
            reg_prog_state <= reg_prog1 ; 
         end
         else if ((temp_reg1 == 1'b0) & (temp_reg2 == 1'b1))
         begin
            reg_prog_state <= reg_prog1 ; 
         end
         else
         begin
            reg_prog_state <= reg_prog_nextstate ; 
         end 
         if (iic_stop == 1'b0)
         begin
            iic_state <= iic_state + 1 ; 
         end
         else if (iic_start == 1'b1)
         begin
            iic_state <= 7'b0000001 ; 
         end 
   end 
endmodule



