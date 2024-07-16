 module paj_framebuftop_hierarchy_no_mem (fbdata, fbdatavalid, fbnextscanline, tm3_vidout_red, tm3_vidout_green, tm3_vidout_blue, tm3_vidout_clock, tm3_vidout_hsync, tm3_vidout_vsync, tm3_vidout_blank, oe, globalreset, tm3_clk_v0, tm3_clk_v0_2);

    input[63:0] fbdata; 
    input fbdatavalid; 
    output fbnextscanline; 
    wire fbnextscanline;
    output[9:0] tm3_vidout_red; 

    wire[9:0] tm3_vidout_red;
    output[9:0] tm3_vidout_green; 
    wire[9:0] tm3_vidout_green;
    output[9:0] tm3_vidout_blue; 
    wire[9:0] tm3_vidout_blue;
    output tm3_vidout_clock; 
    wire tm3_vidout_clock;
    output tm3_vidout_hsync; 
    wire tm3_vidout_hsync;
    output tm3_vidout_vsync; 
    wire tm3_vidout_vsync;
    output tm3_vidout_blank; 

    wire tm3_vidout_blank;
    input oe; 
    input globalreset; 
    input tm3_clk_v0; 
    input tm3_clk_v0_2; 

    wire[20:0] pixeldataA; 
    wire[20:0] pixeldataB; 
    wire[20:0] pixeldataC; 
    wire startframe; 
    wire nextscanline; 
    wire nextpixelA; 
    wire nextpixelB; 

    wire nextpixelC; 
    wire fbnextscanlineint; 

    assign fbnextscanline = ((fbnextscanlineint == 1'b1) & oe == 1'b1) ? 1'b1 : 1'b0 ;

    vidout vidoutinst (pixeldataA, pixeldataB, pixeldataC, nextpixelA, nextpixelB, nextpixelC, startframe, nextscanline, globalreset, tm3_clk_v0, tm3_clk_v0_2, tm3_vidout_red, tm3_vidout_green, tm3_vidout_blue, tm3_vidout_clock, tm3_vidout_hsync, tm3_vidout_vsync, tm3_vidout_blank); 

    scanlinebuffer scanlinebuf (pixeldataA, pixeldataB, pixeldataC, nextpixelA, nextpixelB, nextpixelC, startframe, nextscanline, fbdata, fbnextscanlineint, fbdatavalid, globalreset, tm3_clk_v0); 
 endmodule
module vidout (pixeldataA, pixeldataB, pixeldataC, nextpixelA, nextpixelB, nextpixelC, startframe, nextscanline, globalreset, tm3_clk_v0, tm3_clk_v0_2, tm3_vidout_red, tm3_vidout_green, tm3_vidout_blue, tm3_vidout_clock, tm3_vidout_hsync, tm3_vidout_vsync, tm3_vidout_blank);

    input[20:0] pixeldataA; 
    input[20:0] pixeldataB; 
    input[20:0] pixeldataC; 
    output nextpixelA; 
    reg nextpixelA;
    output nextpixelB; 
    reg nextpixelB;
    output nextpixelC; 
    reg nextpixelC;
    output startframe; 

    reg startframe;
    output nextscanline; 
    reg nextscanline;
    input globalreset; 
    input tm3_clk_v0; 
    input tm3_clk_v0_2; // PAJ modified for VPR flow so no clock in LUT
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

    reg[9:0] horiz; 
    reg[9:0] vert; 
    reg video_state; 
    reg nothsyncd; 

    reg[20:0] primarypixel; 
    reg[20:0] secondarypixel; 
    reg[20:0] primarybuffer; 
    reg[20:0] secondarybuffer; 
    reg primarynext; 
    reg secondarynext; 
    reg[1:0] pselect; 

	always @(posedge tm3_clk_v0)
	begin
		if ((horiz < 641 & vert < 480) & video_state == 1'b0)
		begin
			if ((vert[0]) == 1'b0)
			begin
				secondarynext <= 1'b0 ; 
				// Horizontaly Interpt Line
				if ((horiz[0]) == 1'b0)
				begin
					primarynext <= 1'b1 ; 
				end 
			end
			else
			begin
				// Bilinear Line
				if ((horiz[0]) == 1'b0)
				begin
					primarynext <= 1'b1 ; 
					secondarynext <= 1'b1 ; 
				end 
			end 
		end 
	end 


	always @(posedge tm3_clk_v0)
	begin
		case (pselect)
			2'b00 :
			begin
				
				primarypixel <= pixeldataA ; 
				secondarypixel <= pixeldataB ; 

				nextpixelA <= primarynext ; 
				nextpixelB <= secondarynext ; 
				nextpixelC <= 1'b0 ; 
			end
			2'b01 :
			begin
				
				primarypixel <= pixeldataB ; 
				secondarypixel <= pixeldataC ; 
				
				nextpixelA <= 1'b0 ; 
				nextpixelB <= primarynext ; 
				nextpixelC <= secondarynext ; 
			end
			2'b10 :
			begin
				primarypixel <= pixeldataC ; 
				secondarypixel <= pixeldataA ; 

				nextpixelB <= 1'b0 ; 
				nextpixelC <= primarynext ; 
				nextpixelA <= secondarynext ; 
			end
			default :
			begin				
				primarypixel <= 1;
				secondarypixel <= 1;

				nextpixelA <= 1'b0 ; 
				nextpixelB <= 1'b0 ; 
				nextpixelC <= 1'b0 ; 
			end
		endcase 
	end 

	assign tm3_vidout_clock = ~(video_state) ;

	always @(posedge tm3_clk_v0)
	begin
		if (globalreset == 1'b1)
		begin
			nothsyncd <= 1'b0 ; 
			vert <= 1;
			horiz <= 1;
			startframe <= 1'b0 ; 
			pselect <= 2'b00 ; 
			video_state <= 1'b0 ; 

			tm3_vidout_blank <= 1'b0 ; 
			tm3_vidout_vsync <= 1'b0 ; 
			tm3_vidout_hsync <= 1'b0 ; 
			tm3_vidout_green <= 1;
			tm3_vidout_red <= 1;
			tm3_vidout_blue <= 1;
			primarybuffer <= 1;
			secondarybuffer <= 1;
			nextscanline <= 1'b0 ; 
		end
		else if (tm3_clk_v0_2 == 1'b1) // PAJ modified for VPR flow so no clock in LUT
		begin

			video_state <= ~(video_state) ; 
			// Code that handles active scanline
			// Code that handle new scanline requests
			nothsyncd <= ~tm3_vidout_hsync ; 
			if (tm3_vidout_hsync == 1'b1 & nothsyncd == 1'b1)
			begin
				if ((vert < 478) & (vert[0]) == 1'b0)
				begin
					nextscanline <= 1'b1 ; 
					startframe <= 1'b0 ; 
				end 
				else if ((vert < 480) & (vert[0]) == 1'b1)
				begin
					nextscanline <= 1'b0 ; 
					startframe <= 1'b0 ; 
					case (pselect)
						2'b00 :
						begin
							pselect <= 2'b01 ; 
						end
						2'b01 :
						begin
							pselect <= 2'b10 ; 
						end
						2'b10 :
						begin
							pselect <= 2'b00 ; 
						end
						default :
						begin
							pselect <= 2'b0 ; 
						end
					endcase 
				end 
				else if (vert == 525)
				begin
					nextscanline <= 1'b1 ; 
					startframe <= 1'b1 ; 
					pselect <= 2'b00 ; 
				end 
			end 

			if (video_state == 1'b0)
			begin
				if (horiz == 800)
				begin
					horiz <= 10'b0000000000;
					if (vert == 525)
					begin
						vert <= 10'b0000000000;
					end
					else
					begin
						vert <= vert + 1;
					end 
				end
				else
				begin
					horiz <= horiz + 1; 
				end 

				if ((vert >= 491) & (vert <= 493))
				begin
					tm3_vidout_vsync <= 1;
				end
				else
				begin
					tm3_vidout_vsync <= 0;
				end 

				if ((horiz >= 664) & (horiz <= 760))
				begin
					tm3_vidout_hsync <= 1;
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

				if (horiz < 643 & vert < 480)
				begin
					if ((vert[0]) == 1'b0)
					begin
					// Horizontaly Interpt Line
						if ((horiz[0]) == 1'b0)
						begin
							primarybuffer <= primarypixel ; 
							tm3_vidout_red <= {primarypixel[20:14], 3'b000} ; 
							tm3_vidout_green <= {primarypixel[13:7], 3'b000} ; 
							tm3_vidout_blue <= {primarypixel[6:0], 3'b000} ; 
						end
						else
						begin
							primarybuffer <= primarypixel ; 
							//tm3_vidout_red <= (('0' &primarypixel(20 downto 14))+('0' &primarybuffer(20 downto 14)))(7 downto 1) & "000";
							//tm3_vidout_green <= (('0' &primarypixel(13 downto 7))+('0' &primarybuffer(13 downto 7)))(7 downto 1) & "000";
							//tm3_vidout_blue <= (('0'&primarypixel(6 downto 0))+('0' &primarybuffer(6 downto 0)))(7 downto 1) & "000";
							tm3_vidout_red <= {(({1'b0, primarypixel[20:14]}) + ({1'b0, primarybuffer[20:14]})), 3'b000} ; 
							tm3_vidout_green <= {(({1'b0, primarypixel[13:7]}) + ({1'b0, primarybuffer[13:7]})), 3'b000} ; 
							tm3_vidout_blue <= {(({1'b0, primarypixel[6:0]}) + ({1'b0, primarybuffer[6:0]})), 3'b000} ; 
						end 
					end
					else
					begin
						// Bilinear Line
						if ((horiz[0]) == 1'b0)
						begin
							primarybuffer <= primarypixel ; 
							secondarybuffer <= secondarypixel ; 
							//tm3_vidout_red <= (('0' & primarypixel(20 downto 14))+('0' & secondarypixel(20 downto 14)))(7 downto 1) & "000";
							//tm3_vidout_green <= (('0' & primarypixel(13 downto 7))+('0' & secondarypixel(13 downto 7)))(7 downto 1) & "000";
							//tm3_vidout_blue <= (('0' & primarypixel(6 downto 0))+('0' & secondarypixel(6 downto 0)))(7 downto 1) & "000";
							tm3_vidout_red <= {(({1'b0, primarypixel[20:14]}) + ({1'b0, secondarypixel[20:14]})), 3'b000} ; 
							tm3_vidout_green <= {(({1'b0, primarypixel[13:7]}) + ({1'b0, secondarypixel[13:7]})), 3'b000} ; 
							tm3_vidout_blue <= {(({1'b0, primarypixel[6:0]}) + ({1'b0, secondarypixel[6:0]})), 3'b000} ; 
						end
						else
						begin
							//tm3_vidout_red <= (("00" &primarypixel(20 downto 14))+("00" &secondarypixel(20 downto 14))+
							//              ("00" &primarybuffer(20 downto 14))+("00" &secondarybuffer(20 downto 14)))(8 downto 2) & "000";
							//tm3_vidout_green <= (("00" & primarypixel(13 downto 7))+("00" & secondarypixel(13 downto 7))+
							//              ("00" & primarybuffer(13 downto 7))+("00" & secondarybuffer(13 downto 7)))(8 downto 2) & "000";
							//tm3_vidout_blue <= (("00" & primarypixel(6 downto 0))+("00" & secondarypixel(6 downto 0))+
							//              ("00" & primarybuffer(6 downto 0))+("00" & secondarybuffer(6 downto 0)))(8 downto 2) & "000";
							tm3_vidout_red <= {(({2'b00, primarypixel[20:14]}) 
								+ ({2'b00, secondarypixel[20:14]}) 
								+ ({2'b00, primarybuffer[20:14]}) 
								+ ({2'b00, secondarybuffer[20:14]})), 3'b000} ; 
							tm3_vidout_green <= {(({2'b00, primarypixel[13:7]}) 
								+ ({2'b00, secondarypixel[13:7]}) 
								+ ({2'b00, primarybuffer[13:7]}) 
								+ ({2'b00, secondarybuffer[13:7]})), 3'b000} ; 
							tm3_vidout_blue <= {(({2'b00, primarypixel[6:0]}) 
								+ ({2'b00, secondarypixel[6:0]}) 
								+ ({2'b00, primarybuffer[6:0]}) 
								+ ({2'b00, secondarybuffer[6:0]})), 3'b000} ; 
						end 
					end 
				end 
				else
				begin
					tm3_vidout_green <= 1;
					tm3_vidout_red <= 1;
					tm3_vidout_blue <= 1;
				end
			end 
		end 
	end 
endmodule




    
    
    
    

 module scanlinebuffer (pixeldataA, pixeldataB, pixeldataC, nextpixelA, nextpixelB, nextpixelC, startframe, nextscanline, fbdata, fbnextscanline, fbdatavalid, globalreset, clk);

    output[20:0] pixeldataA; 
    wire[20:0] pixeldataA;
    output[20:0] pixeldataB; 
    wire[20:0] pixeldataB;
    output[20:0] pixeldataC; 
    wire[20:0] pixeldataC;
    input nextpixelA; 
    input nextpixelB; 

    input nextpixelC; 
    input startframe; 
    input nextscanline; 
    input[62:0] fbdata; 
    output fbnextscanline; 
    reg fbnextscanline;
    input fbdatavalid; 
    input globalreset; 
    input clk; 

    reg[1:0] state; 
    reg[1:0] next_state; 
    reg weA; 
    reg weB; 
    reg weC; 

	reg temp_fbnextscanline;

    scanline scanlineA (nextpixelA, weA, fbdata[62:0], pixeldataA, globalreset, clk); 
    scanline scanlineB (nextpixelB, weB, fbdata[62:0], pixeldataB, globalreset, clk); 
    scanline scanlineC (nextpixelC, weC, fbdata[62:0], pixeldataC, globalreset, clk); 

    always @(posedge clk)
    begin
       if (globalreset == 1'b1)
       begin
          state <= 0 ; 
          fbnextscanline <= 1'b0 ; 
       end
       else
       begin
          state <= next_state ; 
          if (fbdatavalid == 1'b1)
          begin
             fbnextscanline <= 1'b0 ; 
          end 
	  else
	  begin
	  	fbnextscanline <= temp_fbnextscanline;
	  end
	end
    end

    always @(posedge clk)
    begin
       case (state)
          0 :
                   begin
				       weB <= 1'b0 ; 
				       weC <= 1'b0 ; 
                      weA <= fbdatavalid ; 
                      if (nextscanline == 1'b1 & startframe == 1'b1)
                      begin
                         next_state <= 2 ; 
                      end
                      else
                      begin
                         next_state <= 0 ; 
                      end 
                         if (startframe == 1'b1 & nextscanline == 1'b1)
                         begin
                            temp_fbnextscanline <= 1'b1 ; 
                         end 

                   end
          1 :
                   begin
				       weB <= 1'b0 ; 
				       weC <= 1'b0 ; 
                      weA <= fbdatavalid ; 
                      if (nextscanline == 1'b1)
                      begin
                         next_state <= 2 ; 
                      end
                      else
                      begin
                         next_state <= 1 ; 
                      end 

                         if (nextscanline == 1'b1)
                         begin
                            temp_fbnextscanline <= 1'b1 ; 
                         end 
 

                   end
          2 :
                   begin
				       weA <= 1'b0 ; 
				       weC <= 1'b0 ; 
                      weB <= fbdatavalid ; 
                      if (nextscanline == 1'b1)
                      begin
                         next_state <= 3 ; 
                      end
                      else
                      begin
                         next_state <= 2 ; 
                      end 
                         if (nextscanline == 1'b1)

                         begin
                            temp_fbnextscanline <= 1'b1 ; 
                         end 
 
                   end
          3 :
                   begin
				       weA <= 1'b0 ; 
				       weB <= 1'b0 ; 
                      weC <= fbdatavalid ; 
                      if (nextscanline == 1'b1)
                      begin
                         next_state <= 1 ; 
                      end
                      else
                      begin
                         next_state <= 3 ; 
                      end 
	                  if (nextscanline == 1'b1)
                         begin

                            temp_fbnextscanline <= 1'b1 ; 
                         end 

                   end
       endcase 
    end 
 endmodule

module scanline (nextpixel, we, datain, pixeldata, globalreset, clk);

    input nextpixel; 
    input we; 
    input[62:0] datain; 
    output[20:0] pixeldata; 
    reg[20:0] pixeldata;
    input globalreset; 
    input clk; 

    reg[6:0] addr; 
    reg[6:0] waddr; 

    reg[6:0] raddr; 
    reg[1:0] subitem; 
    reg wedelayed; 
    reg[62:0]mem1; 
    reg[62:0]mem2; 
    wire[62:0] mdataout; 

    assign mdataout = mem2 ;

    always @(posedge clk)
    begin
       if (globalreset == 1'b1)
       begin
          subitem <= 2'b00 ; 
          waddr <= 1;
          raddr <= 1;
          wedelayed <= 1'b0 ; 
          pixeldata <= 1;
       end
       else
       begin
          wedelayed <= we ; 
          if (nextpixel == 1'b1 | wedelayed == 1'b1)
          begin

             case (subitem)
                2'b00 :
                         begin
                            pixeldata <= mdataout[62:42] ; 
                         end
                2'b01 :
                         begin
                            pixeldata <= mdataout[41:21] ; 
                         end
                2'b10 :
                         begin
                            pixeldata <= mdataout[20:0] ; 

                         end
                default :
                         begin
                            pixeldata <= 1;
                         end
             endcase 
          end 
          if (nextpixel == 1'b1)
          begin
             case (subitem)
                2'b00 :
                         begin

                            subitem <= 2'b01 ; 
                         end
                2'b01 :
                         begin
                            subitem <= 2'b10 ; 
                         end
                2'b10 :
                         begin
                            subitem <= 2'b00 ; 
                            if (raddr != 7'b1101010)
                            begin
                               raddr <= raddr + 1 ; 
                            end
                            else
                            begin
                               raddr <= 1;
                            end 
                         end
             endcase 
          end 
          if (we == 1'b1)
          begin
             if (waddr != 7'b1101010)
             begin
                waddr <= waddr + 1 ; 
             end
             else
             begin

                waddr <= 1;
             end 
          end 
       end 
     end 

	always @(posedge clk)
	begin
       addr <= raddr ; 
       if (we == 1'b1)
       begin
          mem1 <= datain ; 
          mem2 <= mem1 ; 
       end  
	end
 endmodule


