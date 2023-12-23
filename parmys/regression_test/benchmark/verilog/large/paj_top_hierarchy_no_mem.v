module paj_top_hierarchy_no_mem (resetA, want_saddr, saddr_ready, saddrin, want_sdata, sdata_ready, sdatain, tm3_sram_data_in, tm3_sram_data_out, tm3_sram_addr, tm3_sram_we, tm3_sram_oe, tm3_sram_adsp, triIDvalid, triID, wanttriID, raydata, rayaddr, raywe, resultready, resultdata, globalreset, tm3_clk_v0);

	input resetA;
    output want_saddr; 
    wire want_saddr;
    input saddr_ready; 
    input[17:0] saddrin; 
    output want_sdata; 

    wire want_sdata;
    input sdata_ready; 
    input[63:0] sdatain; 
    input[63:0] tm3_sram_data_in; 
    wire[63:0] tm3_sram_data_in;
    output[63:0] tm3_sram_data_out; 
    wire[63:0] tm3_sram_data_out;
    wire[63:0] tm3_sram_data_xhdl0;
    output[18:0] tm3_sram_addr; 
    wire[18:0] tm3_sram_addr;
    output[7:0] tm3_sram_we; 
    wire[7:0] tm3_sram_we;
    output[1:0] tm3_sram_oe; 
    wire[1:0] tm3_sram_oe;

    output tm3_sram_adsp; 
    wire tm3_sram_adsp;
    input triIDvalid; 
    input[15:0] triID; 
    output wanttriID; 
    wire wanttriID;
    input[31:0] raydata; 
    input[3:0] rayaddr; 
    input[2:0] raywe; 
    output resultready; 
    wire resultready;
    output[31:0] resultdata; 

    wire[31:0] resultdata;
    input globalreset; 
    input tm3_clk_v0; 

    // Memory Interface Signals 
    wire[191:0] tridata; 
    wire[15:0] triID_out; 
    wire[1:0] cyclenum; 
    wire masterenable; 
    wire masterenablel; 
    wire swap; 
    // Ray Tri Interface Signals

    wire[31:0] tout; 
    wire[15:0] uout; 
    wire[15:0] vout; 
    wire[15:0] triIDout; 
    wire hitout; 
    wire[27:0] origx; 
    wire[27:0] origy; 
    wire[27:0] origz; 
    wire[15:0] dirx; 
    wire[15:0] diry; 
    wire[15:0] dirz; 
    // Nearest Unit Signals

    wire[31:0] nt0; 
    wire[31:0] nt1; 
    wire[31:0] nt2; 
    wire[15:0] nu0; 
    wire[15:0] nu1; 
    wire[15:0] nu2; 
    wire[15:0] nv0; 
    wire[15:0] nv1; 
    wire[15:0] nv2; 
    wire[15:0] ntriID0; 
    wire[15:0] ntriID1; 
    wire[15:0] ntriID2; 
    wire[2:0] anyhit; 
    wire n0enable; 
    wire n1enable; 
    wire n2enable; 
    wire nxenable; 
    wire enablenear; 
    wire enablenearl; 
    wire resetl; 

    wire reset; 
    wire[1:0] raygroupID; 
    wire[1:0] raygroupIDl; 

    memoryinterface mem (want_saddr, saddr_ready, saddrin, want_sdata, sdata_ready, sdatain, tridata, triID_out, masterenable, triIDvalid, triID, wanttriID, cyclenum, tm3_sram_data_in, tm3_sram_data_out, tm3_sram_addr, tm3_sram_we, tm3_sram_oe, tm3_sram_adsp, globalreset, tm3_clk_v0); 
    raytri triunit (resetA, tm3_clk_v0, tout, uout, vout, triIDout, hitout, tridata[123:96], tridata[155:128], tridata[187:160], origx, origy, origz, dirx, diry, dirz, tridata[15:0], tridata[31:16], tridata[47:32], tridata[125:124], tridata[63:48], tridata[79:64], tridata[95:80], tridata[157:156], tridata[191], swap,triID_out);
    nearcmpspec nc0 (tout, uout, vout, triIDout, hitout, nt0, nu0, nv0, ntriID0, anyhit[0], n0enable, nxenable, resetl, globalreset, tm3_clk_v0); 
    nearcmp nc1 (tout, uout, vout, triIDout, hitout, nt1, nu1, nv1, ntriID1, anyhit[1], n1enable, resetl, globalreset, tm3_clk_v0); 
    nearcmp nc2 (tout, uout, vout, triIDout, hitout, nt2, nu2, nv2, ntriID2, anyhit[2], n2enable, resetl, globalreset, tm3_clk_v0); 

    assign n0enable = ((cyclenum == 2'b10) & ((masterenablel) == 1'b1)) ? 1'b1 : 1'b0 ;
    assign n1enable = ((cyclenum == 2'b00) & ((masterenablel) == 1'b1)) ? 1'b1 : 1'b0 ;
    assign n2enable = ((cyclenum == 2'b01) & ((masterenablel) == 1'b1)) ? 1'b1 : 1'b0 ;
    assign nxenable = (((enablenearl) == 1'b1) & ((masterenablel) == 1'b1)) ? 1'b1 : 1'b0 ;
    // One delay level to account for near cmp internal latch

    delay2x8 raygroupdelay(raygroupID, raygroupIDl, tm3_clk_v0); 

    delay1x37 enableneardelay(enablenear, enablenearl, tm3_clk_v0); 

    delay1x37 mastdelay(masterenable, masterenablel, tm3_clk_v0); 

    delay1x37 resetdelay(reset, resetl, tm3_clk_v0); 
    resultstate resstate (resetl, nt0, nt1, nt2, nu0, nu1, nu2, nv0, nv1, nv2, ntriID0, ntriID1, ntriID2, anyhit[0], anyhit[1], anyhit[2], raygroupIDl, resultready, resultdata, globalreset, tm3_clk_v0); 
    raybuffer raybuff (origx, origy, origz, dirx, diry, dirz, raygroupID, swap, reset, enablenear, raydata, rayaddr, raywe, cyclenum, tm3_clk_v0); 
 endmodule


module delay2x8 (datain, dataout, clk);

    input[2 - 1:0] datain; 
    output[2 - 1:0] dataout; 
    wire[2 - 1:0] dataout;
    input clk; 

    reg[2 - 1:0] buff0; 
    reg[2 - 1:0] buff1; 
    reg[2 - 1:0] buff2; 
    reg[2 - 1:0] buff3; 
    reg[2 - 1:0] buff4; 
    reg[2 - 1:0] buff5; 
    reg[2 - 1:0] buff6; 
    reg[2 - 1:0] buff7; 

    assign dataout = buff7 ;

    always @(posedge clk)
    begin
/* PAJ expanded for loop to hard definition the size of `depth */
       buff0 <= datain ; 
		buff1 <= buff0;
		buff2 <= buff1;
		buff3 <= buff2;
		buff4 <= buff3;
		buff5 <= buff4;
		buff6 <= buff5;
		buff7 <= buff6;
    end 
 endmodule


module delay1x37 (datain, dataout, clk);

    input[1 - 1:0] datain; 
    output[1 - 1:0] dataout; 
    wire[1 - 1:0] dataout;
    input clk; 

    reg[1 - 1:0] buff0; 
    reg[1 - 1:0] buff1; 
    reg[1 - 1:0] buff2; 
    reg[1 - 1:0] buff3; 
    reg[1 - 1:0] buff4; 
    reg[1 - 1:0] buff5; 
    reg[1 - 1:0] buff6; 
    reg[1 - 1:0] buff7; 
    reg[1 - 1:0] buff8; 
    reg[1 - 1:0] buff9; 
    reg[1 - 1:0] buff10; 
    reg[1 - 1:0] buff11; 
    reg[1 - 1:0] buff12; 
    reg[1 - 1:0] buff13; 
    reg[1 - 1:0] buff14; 
    reg[1 - 1:0] buff15; 
    reg[1 - 1:0] buff16; 
    reg[1 - 1:0] buff17; 
    reg[1 - 1:0] buff18; 
    reg[1 - 1:0] buff19; 
    reg[1 - 1:0] buff20; 
    reg[1 - 1:0] buff21; 
    reg[1 - 1:0] buff22; 
    reg[1 - 1:0] buff23; 
    reg[1 - 1:0] buff24; 
    reg[1 - 1:0] buff25; 
    reg[1 - 1:0] buff26; 
    reg[1 - 1:0] buff27; 
    reg[1 - 1:0] buff28; 
    reg[1 - 1:0] buff29; 
    reg[1 - 1:0] buff30; 
    reg[1 - 1:0] buff31; 
    reg[1 - 1:0] buff32; 
    reg[1 - 1:0] buff33; 
    reg[1 - 1:0] buff34; 
    reg[1 - 1:0] buff35; 
    reg[1 - 1:0] buff36; 
 
    assign dataout = buff36 ;

    always @(posedge clk)
    begin
/* PAJ expanded for loop to hard definition the size of `depth */
       buff0 <= datain ; 
		buff1 <= buff0;
		buff2 <= buff1;
		buff3 <= buff2;
		buff4 <= buff3;
		buff5 <= buff4;
		buff6 <= buff5;
		buff7 <= buff6;
		buff8 <= buff7;
		buff9 <= buff8;
		buff10 <= buff9;
		buff11 <= buff10;
		buff12 <= buff11;
		buff13 <= buff12;
		buff14 <= buff13;
		buff15 <= buff14;
		buff16 <= buff15;
		buff17 <= buff16;
		buff18 <= buff17;
		buff19 <= buff18;
		buff20 <= buff19;
		buff21 <= buff20;
		buff22 <= buff21;
		buff23 <= buff22;
		buff24 <= buff23;
		buff25 <= buff24;
		buff26 <= buff25;
		buff27 <= buff26;
		buff28 <= buff27;
		buff29 <= buff28;
		buff30 <= buff29;
		buff31 <= buff30;
		buff32 <= buff31;
		buff33 <= buff32;
		buff34 <= buff33;
		buff35 <= buff34;
		buff36 <= buff35;
    end 
 endmodule

 //------------------------------------------------
 // Ray Buffer, Output Selection & Bus Interface --
 //                                              --
 //   Writes are enabled through the bus         --
 //     WE    Function                           --
 //     000   Idle                               --
 //     001   origx <= raydata 27..0             --
 //     010   origy <= raydata 27..0             --
 //     011   origz <= raydata 27..0             --
 //     100   dirx <= raydata 15..0              --
 //           diry <= raydata 31..16             --
 //     101   dirz <= raydata 15..0              --

 //           swap <= raydata 16                 --
 //     110   reserved                           --
 //     111   activeraygroup <= rayaddr 1..0     --
 //           enablenear <= raydata 0            --
 //                                              --
 //   subraynum is not latched                   --
 //   The output ray data is latched             --
 //------------------------------------------------
 module raybuffer (origx, origy, origz, dirx, diry, dirz, raygroupID, swap, resetout, enablenear, raydata, rayaddr, raywe, subraynum, clk);

    output[27:0] origx; 
    wire[27:0] origx;
    output[27:0] origy; 
    wire[27:0] origy;
    output[27:0] origz; 
    wire[27:0] origz;
    output[15:0] dirx; 
    wire[15:0] dirx;
    output[15:0] diry; 
    wire[15:0] diry;
    output[15:0] dirz; 
    wire[15:0] dirz;

    output[1:0] raygroupID; 
    reg[1:0] raygroupID;
    output swap; 
    wire swap;
    output resetout; 
    reg resetout;
    output enablenear; 
    reg enablenear;
    input[31:0] raydata; 
    input[3:0] rayaddr; 
    input[2:0] raywe; // May need to be expanded
    input[1:0] subraynum; 

    input clk; 

    wire origxwe; 
    wire origywe; 
    wire origzwe; 
    wire dirxwe; 
    wire dirywe; 
    wire dirzwe; 
    wire swapwe; 
    wire raygroupwe; 
    wire[3:0] raddr; 
    reg[1:0] activeraygroup; 

    wire swapvect; 
    reg resetl; 
    reg[1:0] raygroupIDl; 
    reg enablenearl; 

    // Ray output address logic
    assign raddr = {activeraygroup, subraynum} ;

    always @(posedge clk)
   begin
       resetl <= raygroupwe ; 
       resetout <= resetl ; 

       raygroupID <= raygroupIDl ; 
       enablenear <= enablenearl ; 
       if (raygroupwe == 1'b1)
       begin
          activeraygroup <= rayaddr[1:0] ; 
          enablenearl <= raydata[0] ; 
          raygroupIDl <= rayaddr[3:2] ; 
       end  
    end 

    // Decode the write enable signals
    assign origxwe = (raywe == 3'b001) ? 1'b1 : 1'b0 ;
    assign origywe = (raywe == 3'b010) ? 1'b1 : 1'b0 ;
    assign origzwe = (raywe == 3'b011) ? 1'b1 : 1'b0 ;
    assign dirxwe = (raywe == 3'b100) ? 1'b1 : 1'b0 ;
    assign dirywe = (raywe == 3'b100) ? 1'b1 : 1'b0 ;
    assign dirzwe = (raywe == 3'b101) ? 1'b1 : 1'b0 ;
    assign swapwe = (raywe == 3'b101) ? 1'b1 : 1'b0 ;
    assign raygroupwe = (raywe == 3'b111) ? 1'b1 : 1'b0 ;
    // Instantate all the required ram elements

    dpram28 origxram(origxwe, origx, raydata[27:0], clk); 

    dpram28 origyram(origywe, origy, raydata[27:0], clk); 


    dpram28 origzram(origzwe, origz, raydata[27:0], clk); 

    dpram16 dirxram(dirxwe, dirx, raydata[15:0], clk); 

    dpram16 diryram(dirywe, diry, raydata[31:16], clk); 

    dpram16 dirzram(dirzwe, dirz, raydata[15:0], clk); 

    dpram1 swapram(swapwe, swapvect, raydata[16], clk); 

    assign swap = swapvect ;

 endmodule


 module dpram16 (we, dataout, datain, clk);

    input we; 
    output[16 - 1:0] dataout; 
    reg[16 - 1:0] dataout;
    input[16 - 1:0] datain; 
    input clk; 

    reg[16 - 1:0] mem1; 
    reg[16 - 1:0] mem2; 
    wire[16 - 1:0] data; 

    assign data = mem2 ;

    always @(posedge clk or posedge we)
    begin
       dataout <= data ; 
       if (we == 1'b1)
       begin
          mem1 <= datain ; 
          mem2 <= mem1 ; 
       end  
    end 
 endmodule


 module dpram28 (we, dataout, datain, clk);

    input we; 
    output[28 - 1:0] dataout; 
    reg[28 - 1:0] dataout;
    input[28 - 1:0] datain; 
    input clk; 

    reg[28 - 1:0] mem1; 
    reg[28 - 1:0] mem2; 
    wire[28 - 1:0] data; 

    assign data = mem2 ;

    always @(posedge clk or posedge we)
    begin
       dataout <= data ; 
       if (we == 1'b1)
       begin
          mem1 <= datain ; 
          mem2 <= mem1 ; 
       end  
    end 
 endmodule


 module dpram1 (we, dataout, datain, clk);

    input we; 
    output[1 - 1:0] dataout; 
    reg[1 - 1:0] dataout;
    input[1 - 1:0] datain; 
    input clk; 

    reg[1 - 1:0] mem1; 
    reg[1 - 1:0] mem2; 
    wire[1 - 1:0] data; 

    assign data = mem2 ;

    always @(posedge clk or posedge we)
    begin
       dataout <= data ; 
       if (we == 1'b1)
       begin
          mem1 <= datain ; 
          mem2 <= mem1 ; 
       end  
    end 
 endmodule
    
    
    
    
    
    
    
    
    
    


module resultstate (commit, t1, t2, t3, u1, u2, u3, v1, v2, v3, id1, id2, id3, hit1, hit2, hit3, resultID, resultready, resultdata, globalreset, clk);

    input commit; 
    input[31:0] t1; 
    input[31:0] t2; 
    input[31:0] t3; 
    input[15:0] u1; 
    input[15:0] u2; 
    input[15:0] u3; 
    input[15:0] v1; 
    input[15:0] v2; 
    input[15:0] v3; 

    input[15:0] id1; 
    input[15:0] id2; 
    input[15:0] id3; 
    input hit1; 
    input hit2; 
    input hit3; 
    input[1:0] resultID; 
    output resultready; 
    reg resultready;
    output[31:0] resultdata; 
    reg[31:0] resultdata;
    input globalreset; 

    input clk; 

    reg[3:0] state; 
    reg[3:0] next_state; 
    reg[31:0] t1b; 
    reg[31:0] t2b; 
    reg[31:0] t3b; 
    reg[15:0] u1b; 
    reg[15:0] u2b; 
    reg[15:0] u3b; 
    reg[15:0] v1b; 
    reg[15:0] v2b; 
    reg[15:0] v3b; 
    reg[15:0] id1b; 
    reg[15:0] id2b; 
    reg[15:0] id3b; 
    reg hit1b; 
    reg hit2b; 
    reg hit3b; 
    reg[1:0] resultIDb; 

    reg[31:0] temp_t1b; 
    reg[31:0] temp_t2b; 
    reg[31:0] temp_t3b; 
    reg[15:0] temp_u1b; 
    reg[15:0] temp_u2b; 
    reg[15:0] temp_u3b; 
    reg[15:0] temp_v1b; 
    reg[15:0] temp_v2b; 
    reg[15:0] temp_v3b; 
    reg[15:0] temp_id1b; 
    reg[15:0] temp_id2b; 
    reg[15:0] temp_id3b; 
    reg temp_hit1b; 
    reg temp_hit2b; 
    reg temp_hit3b; 
    reg[1:0] temp_resultIDb; 
    reg temp_resultready;
    reg[31:0] temp_resultdata;

    always @(posedge clk or posedge globalreset)
    begin
       if (globalreset == 1'b1)
       begin
          state <= 0 ; 

          t1b <= 0;
          t2b <= 0;
          t3b <= 0;
          u1b <= 0;
          u2b <= 0;
          u3b <= 0;
          v1b <= 0;

         v2b <= 0;
          v3b <= 0;
          id1b <= 0;
          id2b <= 0;
          id3b <= 0;
          hit1b <= 1'b0 ; 
          hit2b <= 1'b0 ; 
          hit3b <= 1'b0 ; 

          resultready <= 1'b0 ; 
          resultdata <= 0;
          resultIDb <= 0;
       end
       else
       begin
          state <= next_state ; 
          //resultdata <= 0;
resultdata <= temp_resultdata;
resultready <= temp_resultready;
resultIDb <= temp_resultIDb;
t1b <= temp_t1b;
t2b <= temp_t2b;
t3b <= temp_t3b;
u1b <= temp_u1b;
u2b <= temp_u2b;
u3b <= temp_u3b;
v1b <= temp_v1b;
v2b <= temp_v2b;
v3b <= temp_v3b;
id1b <= temp_id1b;
id2b <= temp_id2b;
id3b <= temp_id3b;
hit1b <= temp_hit1b;
hit2b <= temp_hit2b;
hit3b <= temp_hit3b;
       end 
    end 

    always @(state or commit)
    begin
       case (state)
          0 :
                   begin
                      if (commit == 1'b1)
                      begin
                         next_state = 1 ; 
                      end
                      else
                      begin
                         next_state = 0 ; 

                      end 

			temp_resultready = 1'b0 ; 

                         if (commit == 1'b1)
                         begin
                            temp_t1b = t1 ; 
                            temp_t2b = t2 ; 
                            temp_t3b = t3 ; 
                            temp_u1b = u1 ; 
                            temp_u2b = u2 ; 
                            temp_u3b = u3 ; 
                            temp_v1b = v1 ; 
                            temp_v2b = v2 ; 
                            temp_v3b = v3 ; 
                            temp_id1b = id1 ; 
                            temp_id2b = id2 ; 
                            temp_id3b = id3 ; 
                            temp_hit1b = hit1 ; 
                            temp_hit2b = hit2 ; 
                            temp_hit3b = hit3 ; 
                            temp_resultIDb = resultID ; 
                         end 

                   end
          1 :
                   begin
                      next_state = 2 ; 
                         temp_resultdata = t1b ; 
                         temp_resultready = 1'b1 ; 
                   end
          2 :
                   begin
                      next_state = 3 ; 
          				temp_resultready = 1'b0 ; 
                         temp_resultdata = {u1b, v1b} ; 
                   end
          3 :
                   begin
                      next_state = 4 ; 
          				temp_resultready = 1'b0 ; 
                         temp_resultdata = {15'b000000000000000, hit3b, id3b} ; 
                   end
          4 :
                   begin
                      next_state = 5 ; 
                         temp_resultdata = {13'b0000000000000, resultIDb, hit1b, id1b} ; 
          				temp_resultready = 1'b0 ; 
                   end
          5 :
                   begin
                      next_state = 6 ; 
                         temp_resultdata = t2b ; 
          				temp_resultready = 1'b0 ; 
                   end
          6 :
                   begin
                      next_state = 7 ; 
                         temp_resultdata = {u2b, v2b} ; 
          				temp_resultready = 1'b0 ; 
                   end
          7 :
                   begin
                      next_state = 8 ; 
                         temp_resultdata = {15'b000000000000000, hit2b, id2b} ; 
          				temp_resultready = 1'b0 ; 
                   end
          8 :
                   begin
                      next_state = 9 ; 
                         temp_resultdata = t3b ; 
          				temp_resultready = 1'b0 ; 
                   end
          9 :
                   begin
                      next_state = 0 ; 
                         temp_resultdata = {u3b, v3b} ; 
          				temp_resultready = 1'b0 ; 
                   end
       endcase 
    end 
 endmodule

 //----------------------------------------
 // Triangle Memory Controller Component --
 //                                      --
 //  There are 2 nibble bus signals that --
 //  allow the component to download     --
 //  memory contents from the sun. First --
 //  the started address is written,     --
 //  then data is written in 64bit       --
 //  chunks. The address is auto inc'd   --
 //                                      --
 //  The dataout and datavalid signals   --
 //  contain the triangle data           --

 //                                      --
 //  wanttriID is high to request a new  --
 //  triangle ID for 2nd cycle. A high   --
 //  triIDvalid signal indicates it      --
 //  that the user has applied that      --
 //  signal to the triID port.           --
 //                                      --
 //  cyclenum is a control signal that   --
 //  counts from 0-2.  This signal       --
 //  determines the ray to be sent to    --
 //  the ray tri unit as well as which   --
 //  nearest compare unit to use         --

 //----------------------------------------





module memoryinterface (want_addr, addr_ready, addrin, want_data, data_ready, datain, dataout, triIDout, datavalid, triIDvalid, triID, wanttriID, cyclenum, tm3_sram_data_in, tm3_sram_data_out, tm3_sram_addr, tm3_sram_we, tm3_sram_oe, tm3_sram_adsp, globalreset, clk);

    output want_addr; 
    reg want_addr;
    input addr_ready; 
    input[17:0] addrin; 
    output want_data; 
    reg want_data;
    input data_ready; 
    input[63:0] datain; 
    output[191:0] dataout; 

    reg[191:0] dataout;
    output[15:0] triIDout; 
    reg[15:0] triIDout;
    output datavalid; 
    reg datavalid;
    input triIDvalid; 
    input[15:0] triID; 
    output wanttriID; 
    reg wanttriID;
    output[1:0] cyclenum; 
    reg[1:0] cyclenum;
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
    input globalreset; 
    input clk; 


     reg[5:0] state; 

  reg[5:0] next_state; 
    reg[15:0] address; 
    reg[15:0] oldaddress; 
    reg[17:0] waddress; 
    reg[127:0] databuff; 
    reg addrvalid; 
    reg oldaddrvalid; 


    assign tm3_sram_data_out = tm3_sram_data_xhdl0;

    always @(posedge clk or posedge globalreset)
    begin
       if (globalreset == 1'b1)
       begin
          state <= 0 ; 
          addrvalid <= 1'b0 ; 
          oldaddrvalid <= 1'b0 ; 
          address <= 0;
          waddress <= 0;
          databuff <= 0;

          dataout <= 0;
          triIDout <= 0;
          oldaddress <= 0;
          datavalid <= 1'b0 ; 
          wanttriID <= 1'b0 ; 
       end
       else
       begin
          state <= next_state ; 
          case (state)
             0 :

                      begin
          				wanttriID <= 1'b0 ; 
                         if (addr_ready == 1'b1)
                         begin
                            waddress <= addrin ; 
                         end 
                         databuff[63:0] <= tm3_sram_data_in ; 
                      end
             1 :
                      begin
                         databuff[127:64] <= tm3_sram_data_in ; 
                         oldaddrvalid <= addrvalid ; 
                         oldaddress <= address ; 

                         if (triIDvalid == 1'b1)
                         begin
                            addrvalid <= 1'b1 ; 
                            address <= triID ; 
                         end
                         else
                         begin
                            addrvalid <= 1'b0 ; 
                         end 
                         wanttriID <= 1'b1 ; 
                      end
             2 :
                      begin
          				wanttriID <= 1'b0 ; 
                         dataout <= {tm3_sram_data_in, databuff} ; 
                         datavalid <= oldaddrvalid ; 
                         triIDout <= oldaddress ; 
                      end
             4 :
                      begin
          				wanttriID <= 1'b0 ; 
                         if (data_ready == 1'b1)
                         begin
                            waddress <= waddress + 1 ; 
                         end 
                      end

             6 :
                      begin
          				wanttriID <= 1'b0 ; 
                         addrvalid <= 1'b0 ; 
                      end
          endcase 
       end 
    end 

    always @(state or address or addr_ready or data_ready or waddress or datain)
    begin
       case (state)
          0 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
   				       tm3_sram_data_xhdl0 = 0;
				       want_addr = 1'b1 ; 
				       want_data = 1'b0 ; 

                      tm3_sram_addr = {1'b0, address, 2'b01} ; 
                      tm3_sram_adsp = 1'b0 ; 
                      tm3_sram_oe = 2'b01 ; 
                      cyclenum = 2'b00 ; 
                      if (addr_ready == 1'b1)
                      begin
                         next_state = 3 ; 
                      end
                      else

                      begin
                         next_state = 1 ; 
                      end 
                   end
          1 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
   				       tm3_sram_data_xhdl0 = 0;
				       want_addr = 1'b1 ; 
				       want_data = 1'b0 ; 

                      tm3_sram_addr = {1'b0, address, 2'b10} ; 
                      tm3_sram_adsp = 1'b0 ; 
                      tm3_sram_oe = 2'b01 ; 
                      cyclenum = 2'b01 ; 
                      next_state = 2 ; 
                   end

          2 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
   				       tm3_sram_data_xhdl0 = 0;
				       want_addr = 1'b1 ; 
				       want_data = 1'b0 ; 

                      tm3_sram_addr = {1'b0, address, 2'b00} ; 
                      tm3_sram_adsp = 1'b0 ; 
                      tm3_sram_oe = 2'b01 ; 
                      cyclenum = 2'b10 ; 
                      next_state = 0 ; 
                   end
          3 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b11 ; 
				       tm3_sram_adsp = 1'b1 ; 
   				       tm3_sram_data_xhdl0 = 0;
				       tm3_sram_addr = 0;
				       cyclenum = 0;

                      want_addr = 1'b0 ; 
                      want_data = 1'b1 ; 

                      if (addr_ready == 1'b1)
                      begin
                         next_state = 3 ; 
                      end
                      else
                      begin
                         next_state = 4 ; 
                      end 
                   end
          4 :
                   begin
				       tm3_sram_oe = 2'b11 ; 
				       cyclenum = 0;
				       want_addr = 1'b1 ; 

                      want_data = 1'b1 ; 

                      tm3_sram_addr = {1'b0, waddress} ; 
                      tm3_sram_data_xhdl0 = datain ; 
                      if (addr_ready == 1'b1)
                      begin
                         next_state = 6 ; 
                      end
                      else if (data_ready == 1'b1)
                      begin
                         tm3_sram_we = 8'b00000000 ; 
                         tm3_sram_adsp = 1'b0 ; 
                         next_state = 5 ; 
                      end

                      else
                      begin
                         next_state = 4 ; 
                      end 
                   end
          5 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b11 ; 
				       tm3_sram_adsp = 1'b1 ; 
   				       tm3_sram_data_xhdl0 = 0;
				       tm3_sram_addr = 0;
				       cyclenum = 0;
				       want_addr = 1'b1 ; 
				       want_data = 1'b0 ; 

                      if (data_ready == 1'b1)
                      begin
                         next_state = 5 ; 
                      end
                      else

                      begin
                         next_state = 4 ; 
                      end 
                   end

          6 :

                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b11 ; 
				       tm3_sram_adsp = 1'b1 ; 
   				       tm3_sram_data_xhdl0 = 0;
				       tm3_sram_addr = 0;
				       cyclenum = 0;
				       want_data = 1'b0 ; 

                      want_addr = 1'b0 ; 
                      if (addr_ready == 1'b1)
                      begin
                         next_state = 6 ; 
                      end
                      else
                      begin
                         next_state = 0 ; 
                      end 
                   end
       endcase 

    end 
 endmodule


/*

module memoryinterface (want_addr, addr_ready, addrin, want_data, data_ready, datain, dataout, triIDout, datavalid, triIDvalid, triID, wanttriID, cyclenum, tm3_sram_data_in, tm3_sram_data_out, tm3_sram_addr, tm3_sram_we, tm3_sram_oe, tm3_sram_adsp, globalreset, clk);

    output want_addr; 
    reg want_addr;
    input addr_ready; 
    input[17:0] addrin; 
    output want_data; 
    reg want_data;
    input data_ready; 
    input[63:0] datain; 
    output[191:0] dataout; 
    reg[191:0] dataout;
    output[15:0] triIDout; 
    reg[15:0] triIDout;
    output datavalid; 
    reg datavalid;
    input triIDvalid; 
    input[15:0] triID; 
    output wanttriID; 
    reg wanttriID;
    output[1:0] cyclenum; 
    reg[1:0] cyclenum;
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
    input globalreset; 
    input clk; 
     reg[5:0] state; 
  reg[5:0] next_state; 
    reg[15:0] address; 
    reg[15:0] oldaddress; 
    reg[17:0] waddress; 
    reg[127:0] databuff; 
    reg addrvalid; 
    reg oldaddrvalid; 

    reg temp_wanttriID;
    reg temp_addrvalid; 
    reg[17:0] temp_waddress; 
    reg[191:0] temp_dataout;
    reg temp_datavalid;
    reg[15:0] temp_triIDout;
    reg[127:0] temp_databuff; 
    reg temp_oldaddrvalid; 
    reg[15:0] temp_oldaddress; 
    reg[15:0] temp_address; 

    assign tm3_sram_data_out = tm3_sram_data_xhdl0;

    always @(posedge clk or posedge globalreset)
    begin
       if (globalreset == 1'b1)
       begin
          state <= 0 ; 
          addrvalid <= 1'b0 ; 
          oldaddrvalid <= 1'b0 ; 
          address <= 0;
          waddress <= 0;
          databuff <= 0;

          dataout <= 0;
          triIDout <= 0;
          oldaddress <= 0;
          datavalid <= 1'b0 ; 
          wanttriID <= 1'b0 ; 
       end
       else
       begin
          state <= next_state ; 

wanttriID <= temp_wanttriID;
addrvalid <= temp_addrvalid;
waddress <= temp_waddress;
dataout <= temp_dataout;
datavalid <= temp_datavalid;
triIDout <= temp_triIDout;
databuff <= temp_databuff;
oldaddrvalid <= temp_oldaddrvalid;
oldaddress <= temp_oldaddress;
address <= temp_address;

       end 
    end 

    always @(state or address or addr_ready or data_ready or waddress or datain)
    begin
       case (state)
          0 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
   				       tm3_sram_data_xhdl0 = 0;
				       want_addr = 1'b1 ; 
				       want_data = 1'b0 ; 

                      tm3_sram_addr = {1'b0, address, 2'b01} ; 
                      tm3_sram_adsp = 1'b0 ; 
                      tm3_sram_oe = 2'b01 ; 
                      cyclenum = 2'b00 ; 
                      if (addr_ready == 1'b1)
                      begin
                         next_state = 4 ; 
                         temp_waddress = addrin ; 
                      end
                      else
                      begin
                         next_state = 1 ; 
                      end 

			    	  temp_wanttriID = 1'b0 ; 
                      temp_databuff[63:0] = tm3_sram_data_in ; 

                   end
          1 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
   				       tm3_sram_data_xhdl0 = 0;
				       want_addr = 1'b1 ; 
				       want_data = 1'b0 ; 

                      tm3_sram_addr = {1'b0, address, 2'b10} ; 
                      tm3_sram_adsp = 1'b0 ; 
                      tm3_sram_oe = 2'b01 ; 
                      cyclenum = 2'b01 ; 
                      next_state = 2 ; 

                         temp_databuff[127:64] = tm3_sram_data_in ; 
                         temp_oldaddrvalid = addrvalid ; 
                         temp_oldaddress = address ; 

                         if (triIDvalid == 1'b1)
                         begin
                            temp_addrvalid = 1'b1 ; 
                            temp_address = triID ; 
                         end
                         else
                         begin
                            temp_addrvalid = 1'b0 ; 
                         end 
                         temp_wanttriID = 1'b1 ; 

                   end

          2 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
   				       tm3_sram_data_xhdl0 = 0;
				       want_addr = 1'b1 ; 
				       want_data = 1'b0 ; 

                      tm3_sram_addr = {1'b0, address, 2'b00} ; 
                      tm3_sram_adsp = 1'b0 ; 
                      tm3_sram_oe = 2'b01 ; 
                      cyclenum = 2'b10 ; 
                      next_state = 0 ; 


          				temp_wanttriID = 1'b0 ; 
                         temp_dataout = {tm3_sram_data_in, databuff} ; 
                         temp_datavalid = oldaddrvalid ; 
                         temp_triIDout = oldaddress ; 
                   end
          4 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b11 ; 
				       tm3_sram_adsp = 1'b1 ; 
   				       tm3_sram_data_xhdl0 = 0;
				       tm3_sram_addr = 0;
				       cyclenum = 0;

                      want_addr = 1'b0 ; 
                      want_data = 1'b1 ; 

                      if (addr_ready == 1'b1)
                      begin
                         next_state = 4 ; 
                      end
                      else
                      begin
                         next_state = 8 ; 
                      end 
                   end
          8 :
                   begin
				       tm3_sram_oe = 2'b11 ; 
				       cyclenum = 0;
				       want_addr = 1'b1 ; 

                      want_data = 1'b1 ; 

                      tm3_sram_addr = {1'b0, waddress} ; 
                      tm3_sram_data_xhdl0 = datain ; 
                      if (addr_ready == 1'b1)
                      begin
                         next_state = 32 ; 
                      end
                      else if (data_ready == 1'b1)
                      begin
                         tm3_sram_we = 8'b00000000 ; 
                         tm3_sram_adsp = 1'b0 ; 
                         next_state = 16 ; 
                         temp_waddress = waddress + 1 ; 
                      end
                      else
                      begin
                         next_state = 8 ; 
                      end 

		   				temp_wanttriID = 1'b0 ; 
                   end
          16 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b11 ; 
				       tm3_sram_adsp = 1'b1 ; 
   				       tm3_sram_data_xhdl0 = 0;
				       tm3_sram_addr = 0;
				       cyclenum = 0;
				       want_addr = 1'b1 ; 
				       want_data = 1'b0 ; 

                      if (data_ready == 1'b1)
                      begin
                         next_state = 16 ; 
                      end
                      else
                      begin
                         next_state = 8 ; 
                      end 
                   end

          32 :

                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b11 ; 
				       tm3_sram_adsp = 1'b1 ; 
   				       tm3_sram_data_xhdl0 = 0;
				       tm3_sram_addr = 0;
				       cyclenum = 0;
				       want_data = 1'b0 ; 

                      want_addr = 1'b0 ; 
                      if (addr_ready == 1'b1)
                      begin
                         next_state = 32 ; 
                      end
                      else
                      begin
                         next_state = 0 ; 
                      end 
          				temp_wanttriID = 1'b0 ; 
                         temp_addrvalid = 1'b0 ; 
                   end
       endcase 

    end 
 endmodule
*/



 module raytri (reset, clk, tout, uout, vout, triIDout, hitout, vert0x, vert0y, vert0z, origx, origy, origz, dirx, diry, dirz, edge1x, edge1y, edge1z, edge1size, edge2x, edge2y, edge2z, edge2size, config_A, exchangeEdges, triID);

	input reset;
    input clk; 
    output[31:0] tout; 
    wire[31:0] tout;
    output[15:0] uout; 
    wire[15:0] uout;
    output[15:0] vout; 
    wire[15:0] vout;
    output[15:0] triIDout; 

    wire[15:0] triIDout;
    output hitout; 
    wire hitout;
    input[27:0] vert0x; 
    input[27:0] vert0y; 
    input[27:0] vert0z; 
    input[27:0] origx; 
    input[27:0] origy; 
    input[27:0] origz; 
    input[15:0] dirx; 
    input[15:0] diry; 
    input[15:0] dirz; 

    input[15:0] edge1x; 
    input[15:0] edge1y; 
    input[15:0] edge1z; 
    input[1:0] edge1size; 
    input[15:0] edge2x; 
    input[15:0] edge2y; 
    input[15:0] edge2z; 
    input[1:0] edge2size; 
    input config_A; 
    input exchangeEdges; 
    input[15:0] triID; 


    // Latch Connected Signals
    wire[28:0] tvecxl; 
    wire[28:0] tvecyl; 
    wire[28:0] tveczl; 
    wire[15:0] edge1xr; 
    wire[15:0] edge1yr; 
    wire[15:0] edge1zr; 
    wire[15:0] edge1xla; 
    wire[15:0] edge1yla; 
    wire[15:0] edge1zla; 
    wire[15:0] edge1xlb; 
    wire[15:0] edge1ylb; 

    wire[15:0] edge1zlb; 
    wire[15:0] edge2xr; 
    wire[15:0] edge2yr; 
    wire[15:0] edge2zr; 
    wire[15:0] edge2xla; 
    wire[15:0] edge2yla; 
    wire[15:0] edge2zla; 
    wire[15:0] edge2xlb; 
    wire[15:0] edge2ylb; 
    wire[15:0] edge2zlb; 
    wire[15:0] dirxla; 
    wire[15:0] diryla; 

    wire[15:0] dirzla; 
    wire[15:0] dirxlb; 
    wire[15:0] dirylb; 
    wire[15:0] dirzlb; 
    wire[50:0] detl; 
    wire hitl; 
    wire configl; 
    wire[1:0] edge1sizer; 
    wire[1:0] edge2sizer; 

    wire[1:0] edge1sizel; 
    wire[1:0] edge2sizel; 
    // Intermediate Signals
    wire[32:0] pvecx; 
    wire[32:0] pvecy; 
    wire[32:0] pvecz; 
    wire[50:0] det; 
    wire[28:0] tvecx; 
    wire[28:0] tvecy; 
    wire[28:0] tvecz; 
    wire[45:0] qvecx; 
    wire[45:0] qvecy; 

    wire[45:0] qvecz; 
    wire[63:0] u; 
    wire[63:0] su; 
    wire[63:0] v; 
    wire[63:0] usv; 
    wire[63:0] t; 
    reg[64:0] uv; 
    reg hitinter; 
    // Output Signals
    reg hit; 
    wire[15:0] ru; 
    wire[15:0] rv; 


    // Level 1 Math

    crossproduct16x16 pvec(dirxla, diryla, dirzla, edge2xla, edge2yla, edge2zla, pvecx, pvecy, pvecz, clk); 

    vectsub tvec(origx, origy, origz, vert0x, vert0y, vert0z, tvecx, tvecy, tvecz, clk); 

    vectdelay29x2 tvecdelay(tvecx, tvecy, tvecz, tvecxl, tvecyl, tveczl, clk); 


    vectexchange edge1exchange(edge2x, edge2y, edge2z, edge1x, edge1y, edge1z, edge1xr, edge1yr, edge1zr, exchangeEdges); 

    vectexchange  edge2exchange(edge1x, edge1y, edge1z, edge2x, edge2y, edge2z, edge2xr, edge2yr, edge2zr, exchangeEdges); 
    // changed to delay 1

    vectdelay16x1 edge1adelay(edge1xr, edge1yr, edge1zr, edge1xla, edge1yla, edge1zla, clk); 
    // changed to delay 2

    vectdelay16x2 edge1bdelay(edge1xla, edge1yla, edge1zla, edge1xlb, edge1ylb, edge1zlb, clk); 


    crossproduct29x16 qvec(tvecx, tvecy, tvecz, edge1xla, edge1yla, edge1zla, qvecx, qvecy, qvecz, clk); 

    dotproduct16x33 dot(edge1xlb, edge1ylb, edge1zlb, pvecx, pvecy, pvecz, det, clk); 

    dotproduct29x33 ui(tvecxl, tvecyl, tveczl, pvecx, pvecy, pvecz, u, clk); 

    vectdelay16x1 dirdelaya(dirx, diry, dirz, dirxla, diryla, dirzla, clk); 

    vectdelay16x2 dirdelayb(dirxla, diryla, dirzla, dirxlb, dirylb, dirzlb, clk); 


    dotproduct16x46 vi(dirxlb, dirylb, dirzlb, qvecx, qvecy, qvecz, usv, clk); 

    vectdelay16x1  edge2delaya(edge2xr, edge2yr, edge2zr, edge2xla, edge2yla, edge2zla, clk); 

    vectdelay16x2  edge2delayb(edge2xla, edge2yla, edge2zla, edge2xlb, edge2ylb, edge2zlb, clk); 

    dotproduct16x46 ti(edge2xlb, edge2ylb, edge2zlb, qvecx, qvecy, qvecz, t, clk); 

    delay1x6 configdelay(config_A, configl, clk); 


    delay51x1 detdelay(det, detl, clk); 

    divide64x32x51x18 divt(reset, t, det, tout, clk); 
    // Changed fraction part to 16

    divide64x16x51x16 divu(reset, su, det, ru, clk); 
    // Changed fraction part to 16

    divide64x16x51x16 divv(reset, v, det, rv, clk); 

    delay16x16 rudelay(ru, uout, clk); 

    delay16x16 rvdelay(rv, vout, clk); 


    delay16x37 triIDdelay(triID, triIDout, clk); 
    // Shifter section

    exchange edge1sizeexchange(edge2size, edge1size, edge1sizer, exchangeEdges); 

    exchange edge2sizeexchange(edge1size, edge2size, edge2sizer, exchangeEdges); 

    delay2x5 edge1sizeDelay(edge1sizer, edge1sizel, clk); 

    delay2x5 edge2sizeDelay(edge2sizer, edge2sizel, clk); 


    shifter  shifter1(usv, v, edge1sizel); 

    shifter  shifter2(u, su, edge2sizel); 
    // Sun interface (address mapped input registers)

    delay1x30 hitdelay(hit, hitl, clk); 
    assign hitout = hitl ;

    always @(posedge clk)
    begin
       // Hit detection Logic (2 cycles)

       uv <= ({su[63], su}) + ({v[63], v}) ; 
       if ((det < 0) | (su < 0) | (v < 0) | (su > det) | (v > det) | (t <= 0))
       begin
          hitinter <= 1'b0 ; 
       end
       else
       begin
          hitinter <= 1'b1 ; 
       end 
       if ((hitinter == 1'b0) | (((configl) == 1'b0) & (uv > detl)))
       begin
          hit <= 1'b0 ; 
       end
       else
       begin
          hit <= 1'b1 ; 
       end 
       // Hit Detection Logic Ends 
    end 
 endmodule

module delay1x6 (datain, dataout, clk);

    input[1 - 1:0] datain; 
    output[1 - 1:0] dataout; 
    wire[1 - 1:0] dataout;
    input clk; 

    reg[1 - 1:0] buff0; 
    reg[1 - 1:0] buff1; 
    reg[1 - 1:0] buff2; 
    reg[1 - 1:0] buff3; 
    reg[1 - 1:0] buff4; 
    reg[1 - 1:0] buff5; 
    reg[1 - 1:0] buff6; 
    reg[1 - 1:0] buff7; 

    assign dataout = buff5 ;

    always @(posedge clk)
    begin
/* PAJ expanded for loop to hard definition the size of `depth */
       buff0 <= datain ; 
		buff1 <= buff0;
		buff2 <= buff1;
		buff3 <= buff2;
		buff4 <= buff3;
		buff5 <= buff4;
    end 
 endmodule


module delay51x1 (datain, dataout, clk);

    input[51 - 1:0] datain; 
    output[51 - 1:0] dataout; 
    wire[51 - 1:0] dataout;
    input clk; 

    reg[51 - 1:0] buff0; 

    assign dataout = buff0 ;

    always @(posedge clk)
    begin
/* PAJ expanded for loop to hard definition the size of `depth */
       buff0 <= datain ; 
    end 
 endmodule

module delay16x16 (datain, dataout, clk);

    input[16 - 1:0] datain; 
    output[16 - 1:0] dataout; 
    wire[16 - 1:0] dataout;
    input clk; 

    reg[16 - 1:0] buff0; 
    reg[16 - 1:0] buff1; 
    reg[16 - 1:0] buff2; 
    reg[16 - 1:0] buff3; 
    reg[16 - 1:0] buff4; 
    reg[16 - 1:0] buff5; 
    reg[16 - 1:0] buff6; 
    reg[16 - 1:0] buff7; 
    reg[16 - 1:0] buff8; 
    reg[16 - 1:0] buff9; 
    reg[16 - 1:0] buff10; 
    reg[16 - 1:0] buff11; 
    reg[16 - 1:0] buff12; 
    reg[16 - 1:0] buff13; 
    reg[16 - 1:0] buff14; 
    reg[16 - 1:0] buff15; 

    assign dataout = buff15 ;

    always @(posedge clk)
    begin
/* PAJ expanded for loop to hard definition the size of `depth */
       buff0 <= datain ; 
		buff1 <= buff0;
		buff2 <= buff1;
		buff3 <= buff2;
		buff4 <= buff3;
		buff5 <= buff4;
		buff6 <= buff5;
		buff7 <= buff6;
		buff8 <= buff7;
		buff9 <= buff8;
		buff10 <= buff9;
		buff11 <= buff10;
		buff12 <= buff11;
		buff13 <= buff12;
		buff14 <= buff13;
		buff15 <= buff14;
    end 
 endmodule

module delay16x37 (datain, dataout, clk);

    input[16 - 1:0] datain; 
    output[16 - 1:0] dataout; 
    wire[16 - 1:0] dataout;
    input clk; 

    reg[16 - 1:0] buff0; 
    reg[16 - 1:0] buff1; 
    reg[16 - 1:0] buff2; 
    reg[16 - 1:0] buff3; 
    reg[16 - 1:0] buff4; 
    reg[16 - 1:0] buff5; 
    reg[16 - 1:0] buff6; 
    reg[16 - 1:0] buff7; 
    reg[16 - 1:0] buff8; 
    reg[16 - 1:0] buff9; 
    reg[16 - 1:0] buff10; 
    reg[16 - 1:0] buff11; 
    reg[16 - 1:0] buff12; 
    reg[16 - 1:0] buff13; 
    reg[16 - 1:0] buff14; 
    reg[16 - 1:0] buff15; 
    reg[16 - 1:0] buff16; 
    reg[16 - 1:0] buff17; 
    reg[16 - 1:0] buff18; 
    reg[16 - 1:0] buff19; 
    reg[16 - 1:0] buff20; 
    reg[16 - 1:0] buff21; 
    reg[16 - 1:0] buff22; 
    reg[16 - 1:0] buff23; 
    reg[16 - 1:0] buff24; 
    reg[16 - 1:0] buff25; 
    reg[16 - 1:0] buff26; 
    reg[16 - 1:0] buff27; 
    reg[16 - 1:0] buff28; 
    reg[16 - 1:0] buff29; 
    reg[16 - 1:0] buff30; 
    reg[16 - 1:0] buff31; 
    reg[16 - 1:0] buff32; 
    reg[16 - 1:0] buff33; 
    reg[16 - 1:0] buff34; 
    reg[16 - 1:0] buff35; 
    reg[16 - 1:0] buff36; 

 
    assign dataout = buff36 ;

    always @(posedge clk)
    begin
/* PAJ expanded for loop to hard definition the size of `depth */
       buff0 <= datain ; 
		buff1 <= buff0;
		buff2 <= buff1;
		buff3 <= buff2;
		buff4 <= buff3;
		buff5 <= buff4;
		buff6 <= buff5;
		buff7 <= buff6;
		buff8 <= buff7;
		buff9 <= buff8;
		buff10 <= buff9;
		buff11 <= buff10;
		buff12 <= buff11;
		buff13 <= buff12;
		buff14 <= buff13;
		buff15 <= buff14;
		buff16 <= buff15;
		buff17 <= buff16;
		buff18 <= buff17;
		buff19 <= buff18;
		buff20 <= buff19;
		buff21 <= buff20;
		buff22 <= buff21;
		buff23 <= buff22;
		buff24 <= buff23;
		buff25 <= buff24;
		buff26 <= buff25;
		buff27 <= buff26;
		buff28 <= buff27;
		buff29 <= buff28;
		buff30 <= buff29;
		buff31 <= buff30;
		buff32 <= buff31;
		buff33 <= buff32;
		buff34 <= buff33;
		buff35 <= buff34;
		buff36 <= buff35;
    end 
 endmodule

module delay2x5 (datain, dataout, clk);

    input[2 - 1:0] datain; 
    output[2 - 1:0] dataout; 
    wire[2 - 1:0] dataout;
    input clk; 

    reg[2 - 1:0] buff0; 
    reg[2 - 1:0] buff1; 
    reg[2 - 1:0] buff2; 
    reg[2 - 1:0] buff3; 
    reg[2 - 1:0] buff4; 

    assign dataout = buff4 ;

    always @(posedge clk)
    begin
/* PAJ expanded for loop to hard definition the size of `depth */
       buff0 <= datain ; 
		buff1 <= buff0;
		buff2 <= buff1;
		buff3 <= buff2;
		buff4 <= buff3;
    end 
 endmodule

module delay1x30 (datain, dataout, clk);

    input[1 - 1:0] datain; 
    output[1 - 1:0] dataout; 
    wire[1 - 1:0] dataout;
    input clk; 

    reg[1 - 1:0] buff0; 
    reg[1 - 1:0] buff1; 
    reg[1 - 1:0] buff2; 
    reg[1 - 1:0] buff3; 
    reg[1 - 1:0] buff4; 
    reg[1 - 1:0] buff5; 
    reg[1 - 1:0] buff6; 
    reg[1 - 1:0] buff7; 
    reg[1 - 1:0] buff8; 
    reg[1 - 1:0] buff9; 
    reg[1 - 1:0] buff10; 
    reg[1 - 1:0] buff11; 
    reg[1 - 1:0] buff12; 
    reg[1 - 1:0] buff13; 
    reg[1 - 1:0] buff14; 
    reg[1 - 1:0] buff15; 
    reg[1 - 1:0] buff16; 
    reg[1 - 1:0] buff17; 
    reg[1 - 1:0] buff18; 
    reg[1 - 1:0] buff19; 
    reg[1 - 1:0] buff20; 
    reg[1 - 1:0] buff21; 
    reg[1 - 1:0] buff22; 
    reg[1 - 1:0] buff23; 
    reg[1 - 1:0] buff24; 
    reg[1 - 1:0] buff25; 
    reg[1 - 1:0] buff26; 
    reg[1 - 1:0] buff27; 
    reg[1 - 1:0] buff28; 
    reg[1 - 1:0] buff29; 

 
    assign dataout = buff29 ;

    always @(posedge clk)
    begin
/* PAJ expanded for loop to hard definition the size of `depth */
       buff0 <= datain ; 
		buff1 <= buff0;
		buff2 <= buff1;
		buff3 <= buff2;
		buff4 <= buff3;
		buff5 <= buff4;
		buff6 <= buff5;
		buff7 <= buff6;
		buff8 <= buff7;
		buff9 <= buff8;
		buff10 <= buff9;
		buff11 <= buff10;
		buff12 <= buff11;
		buff13 <= buff12;
		buff14 <= buff13;
		buff15 <= buff14;
		buff16 <= buff15;
		buff17 <= buff16;
		buff18 <= buff17;
		buff19 <= buff18;
		buff20 <= buff19;
		buff21 <= buff20;
		buff22 <= buff21;
		buff23 <= buff22;
		buff24 <= buff23;
		buff25 <= buff24;
		buff26 <= buff25;
		buff27 <= buff26;
		buff28 <= buff27;
		buff29 <= buff28;
    end 
 endmodule


 //--------------------------------
 // Scalar Mux Component         --
 //  C = A when ABn = '1' else B -- 
 //--------------------------------

 module exchange (A, B, C, ABn);

    input[2 - 1:0] A; 
    input[2 - 1:0] B; 
    output[2 - 1:0] C; 
    wire[2 - 1:0] C;
    input ABn; 

    assign C = (ABn == 1'b1) ? A : B ;
 endmodule
 //------------------------------------------------
 // Parameterized Fixed Point Divide Componenent --
 //                                              --
 //  Qout = (A / B)*2^widthfrac                  --
 //                                              --
 //  Performs unsigned fixed point addition      --
 //  between 2 numbers.  The divide is pipelined --
 //  such that 1 quotient bit is generated per   --
 //  clock cycle. The throughput is one divide   --
 //  per cycle for any size input.               --
 //                                              --
 //  widthOut specified the total output widht   --
 //  widthFrac specifies how many of the output  --
 //            bits are infact fractional        --
 //------------------------------------------------
    
    
/*
     // Width of the output
    `define widthOut  32
    `define widthFrac 15

 module divide (A, B, Qout, clk);

    input[`widthA - 1:0] A; 
    input[`widthB - 1:0] B; 
    output[`widthOut - 1:0] Qout; 
    reg[`widthOut - 1:0] Qout;
    input clk; 

    reg[`widthA + `widthFrac - 1:0] c0; 
    reg[`widthA + `widthFrac - 1:0] c1; 
    reg[`widthA + `widthFrac - 1:0] c2; 
    reg[`widthA + `widthFrac - 1:0] c3; 
    reg[`widthA + `widthFrac - 1:0] c4; 
    reg[`widthA + `widthFrac - 1:0] c5; 
    reg[`widthA + `widthFrac - 1:0] c6; 
    reg[`widthA + `widthFrac - 1:0] c7; 
    reg[`widthA + `widthFrac - 1:0] c8; 
    reg[`widthA + `widthFrac - 1:0] c9; 
    reg[`widthA + `widthFrac - 1:0] c10; 
    reg[`widthA + `widthFrac - 1:0] c11; 
    reg[`widthA + `widthFrac - 1:0] c12; 
    reg[`widthA + `widthFrac - 1:0] c13; 
    reg[`widthA + `widthFrac - 1:0] c14; 
    reg[`widthA + `widthFrac - 1:0] c15; 
    reg[`widthA + `widthFrac - 1:0] c16; 
    reg[`widthA + `widthFrac - 1:0] c17; 
    reg[`widthA + `widthFrac - 1:0] c18; 
    reg[`widthA + `widthFrac - 1:0] c19; 
    reg[`widthA + `widthFrac - 1:0] c20; 
    reg[`widthA + `widthFrac - 1:0] c21; 
    reg[`widthA + `widthFrac - 1:0] c22; 
    reg[`widthA + `widthFrac - 1:0] c23; 
    reg[`widthA + `widthFrac - 1:0] c24; 
    reg[`widthA + `widthFrac - 1:0] c25; 
    reg[`widthA + `widthFrac - 1:0] c26; 
    reg[`widthA + `widthFrac - 1:0] c27; 
    reg[`widthA + `widthFrac - 1:0] c28; 
    reg[`widthA + `widthFrac - 1:0] c29; 
    reg[`widthA + `widthFrac - 1:0] c30; 
    reg[`widthA + `widthFrac - 1:0] c31; 

    reg[`widthOut - 1:0] q0; 
	reg[`widthOut - 1:0] q1; 
    reg[`widthOut - 1:0] q2; 
    reg[`widthOut - 1:0] q3; 
    reg[`widthOut - 1:0] q4; 
    reg[`widthOut - 1:0] q5; 
    reg[`widthOut - 1:0] q6; 
    reg[`widthOut - 1:0] q7; 
    reg[`widthOut - 1:0] q8; 
    reg[`widthOut - 1:0] q9; 
    reg[`widthOut - 1:0] q10; 
	reg[`widthOut - 1:0] q11; 
    reg[`widthOut - 1:0] q12; 
    reg[`widthOut - 1:0] q13; 
    reg[`widthOut - 1:0] q14; 
    reg[`widthOut - 1:0] q15; 
    reg[`widthOut - 1:0] q16; 
    reg[`widthOut - 1:0] q17; 
    reg[`widthOut - 1:0] q18; 
    reg[`widthOut - 1:0] q19; 
    reg[`widthOut - 1:0] q20; 
	reg[`widthOut - 1:0] q21; 
    reg[`widthOut - 1:0] q22; 
    reg[`widthOut - 1:0] q23; 
    reg[`widthOut - 1:0] q24; 
    reg[`widthOut - 1:0] q25; 
    reg[`widthOut - 1:0] q26; 
    reg[`widthOut - 1:0] q27; 
    reg[`widthOut - 1:0] q28; 
    reg[`widthOut - 1:0] q29; 
    reg[`widthOut - 1:0] q30; 
    reg[`widthOut - 1:0] q31; 

    reg[`widthB - 1:0] bp0; 
    reg[`widthB - 1:0] bp1; 
    reg[`widthB - 1:0] bp2; 
    reg[`widthB - 1:0] bp3; 
    reg[`widthB - 1:0] bp4; 
    reg[`widthB - 1:0] bp5; 
    reg[`widthB - 1:0] bp6; 
    reg[`widthB - 1:0] bp7; 
    reg[`widthB - 1:0] bp8; 
    reg[`widthB - 1:0] bp9; 
    reg[`widthB - 1:0] bp10; 
    reg[`widthB - 1:0] bp11; 
    reg[`widthB - 1:0] bp12; 
    reg[`widthB - 1:0] bp13; 
    reg[`widthB - 1:0] bp14; 
    reg[`widthB - 1:0] bp15; 
    reg[`widthB - 1:0] bp16; 
    reg[`widthB - 1:0] bp17; 
    reg[`widthB - 1:0] bp18; 
    reg[`widthB - 1:0] bp19; 
    reg[`widthB - 1:0] bp20; 
    reg[`widthB - 1:0] bp21; 
    reg[`widthB - 1:0] bp22; 
    reg[`widthB - 1:0] bp23; 
    reg[`widthB - 1:0] bp24; 
    reg[`widthB - 1:0] bp25; 
    reg[`widthB - 1:0] bp26; 
    reg[`widthB - 1:0] bp27; 
    reg[`widthB - 1:0] bp28; 
    reg[`widthB - 1:0] bp29; 
    reg[`widthB - 1:0] bp30; 
    reg[`widthB - 1:0] bp31; 

    reg [`widthA + `widthFrac - 1:0] c_xhdl;

    always @(posedge clk)
    begin

    c0[`widthA + `widthFrac - 1:`widthFrac] <= A ;
    c0[`widthFrac - 1:0] <= 0;
    q0 <= 0;
    bp0 <= B ;

		if (c0[`widthA + `widthFrac - 1:`widthOut - 1 - 0] >= bp0[47:0] )
		begin
			q1 <= {q0[`widthOut-2:0] , 1'b1}; 
 			c1 <= {(c0[`widthA+`widthFrac-1:`widthOut-1-0] - bp0[47:0]), c0[`widthOut-0-2:0]} ; 
		end
		else
		begin
			q1 <= {q0[`widthOut-2:0], 1'b0} ; 
			c1 <= c0 ; 
		end
		if (c1[`widthA + `widthFrac - 1:`widthOut - 1 - 1] >= bp1[47:0] )
		begin
			q2 <= {q1[`widthOut-2:0] , 1'b1}; 
 			c2 <= {(c1[`widthA+`widthFrac-1:`widthOut-1-1] - bp1[47:0]), c1[`widthOut-1-2:0]} ; 
		end
		else
		begin
			q2 <= {q1[`widthOut-2:0], 1'b0} ; 
			c2 <= c1 ; 
		end
		if (c2[`widthA + `widthFrac - 1:`widthOut - 1 - 2] >= bp2[47:0] )
		begin
			q3 <= {q2[`widthOut-2:0] , 1'b1}; 
 			c3 <= {(c2[`widthA+`widthFrac-1:`widthOut-1-2] - bp2[47:0]), c2[`widthOut-2-2:0]} ; 
		end
		else
		begin
			q3 <= {q2[`widthOut-2:0], 1'b0} ; 
			c3 <= c2 ; 
		end
		if (c3[`widthA + `widthFrac - 1:`widthOut - 1 - 3] >= bp3[47:0] )
		begin
			q4 <= {q3[`widthOut-2:0] , 1'b1}; 
 			c4 <= {(c3[`widthA+`widthFrac-1:`widthOut-1-3] - bp3[47:0]), c3[`widthOut-3-2:0]} ; 
		end
		else
		begin
			q4 <= {q3[`widthOut-2:0], 1'b0} ; 
			c4 <= c3 ; 
		end
		if (c4[`widthA + `widthFrac - 1:`widthOut - 1 - 4] >= bp4[47:0] )
		begin
			q5 <= {q4[`widthOut-2:0] , 1'b1}; 
 			c5 <= {(c4[`widthA+`widthFrac-1:`widthOut-1-4] - bp4[47:0]), c4[`widthOut-4-2:0]} ; 
		end
		else
		begin
			q5 <= {q4[`widthOut-2:0], 1'b0} ; 
			c5 <= c4 ; 
		end
		if (c5[`widthA + `widthFrac - 1:`widthOut - 1 - 5] >= bp5[47:0] )
		begin
			q6 <= {q5[`widthOut-2:0] , 1'b1}; 
 			c6 <= {(c5[`widthA+`widthFrac-1:`widthOut-1-5] - bp5[47:0]), c5[`widthOut-5-2:0]} ; 
		end
		else
		begin
			q6 <= {q5[`widthOut-2:0], 1'b0} ; 
			c6 <= c5 ; 
		end
		if (c6[`widthA + `widthFrac - 1:`widthOut - 1 - 6] >= bp6[47:0] )
		begin
			q7 <= {q6[`widthOut-2:0] , 1'b1}; 
 			c7 <= {(c6[`widthA+`widthFrac-1:`widthOut-1-6] - bp6[47:0]), c6[`widthOut-6-2:0]} ; 
		end
		else
		begin
			q7 <= {q6[`widthOut-2:0], 1'b0} ; 
			c7 <= c6 ; 
		end
		if (c7[`widthA + `widthFrac - 1:`widthOut - 1 - 7] >= bp7[47:0] )
		begin
			q8 <= {q7[`widthOut-2:0] , 1'b1}; 
 			c8 <= {(c7[`widthA+`widthFrac-1:`widthOut-1-7] - bp7[47:0]), c7[`widthOut-7-2:0]} ; 
		end
		else
		begin
			q8 <= {q7[`widthOut-2:0], 1'b0} ; 
			c8 <= c7 ; 
		end
		if (c8[`widthA + `widthFrac - 1:`widthOut - 1 - 8] >= bp8[47:0] )
		begin
			q9 <= {q8[`widthOut-2:0] , 1'b1}; 
 			c9 <= {(c8[`widthA+`widthFrac-1:`widthOut-1-8] - bp8[47:0]), c8[`widthOut-8-2:0]} ; 
		end
		else
		begin
			q9 <= {q8[`widthOut-2:0], 1'b0} ; 
			c9 <= c8 ; 
		end
		if (c9[`widthA + `widthFrac - 1:`widthOut - 1 - 9] >= bp9[47:0] )
		begin
			q10 <= {q9[`widthOut-2:0] , 1'b1}; 
 			c10 <= {(c9[`widthA+`widthFrac-1:`widthOut-1-9] - bp9[47:0]), c9[`widthOut-9-2:0]} ; 
		end
		else
		begin
			q10 <= {q9[`widthOut-2:0], 1'b0} ; 
			c10 <= c9 ; 
		end
		if (c10[`widthA + `widthFrac - 1:`widthOut - 1 - 10] >= bp10[47:0] )
		begin
			q11 <= {q10[`widthOut-2:0] , 1'b1}; 
 			c11 <= {(c10[`widthA+`widthFrac-1:`widthOut-1-10] - bp10[47:0]), c10[`widthOut-10-2:0]} ; 
		end
		else
		begin
			q11 <= {q10[`widthOut-2:0], 1'b0} ; 
			c11 <= c10 ; 
		end

		if (c11[`widthA + `widthFrac - 1:`widthOut - 1 - 11] >= bp11[47:0] )
		begin
			q12 <= {q11[`widthOut-2:0] , 1'b1}; 
 			c12 <= {(c11[`widthA+`widthFrac-1:`widthOut-1-11] - bp11[47:0]), c11[`widthOut-11-2:0]} ; 
		end
		else
		begin
			q12 <= {q11[`widthOut-2:0], 1'b0} ; 
			c12 <= c11 ; 
		end
		if (c12[`widthA + `widthFrac - 1:`widthOut - 1 - 12] >= bp12[47:0] )
		begin
			q13 <= {q12[`widthOut-2:0] , 1'b1}; 
 			c13 <= {(c12[`widthA+`widthFrac-1:`widthOut-1-12] - bp12[47:0]), c12[`widthOut-12-2:0]} ; 
		end
		else
		begin
			q13 <= {q12[`widthOut-2:0], 1'b0} ; 
			c13 <= c12 ; 
		end
		if (c13[`widthA + `widthFrac - 1:`widthOut - 1 - 13] >= bp13[47:0] )
		begin
			q14 <= {q13[`widthOut-2:0] , 1'b1}; 
 			c14 <= {(c13[`widthA+`widthFrac-1:`widthOut-1-13] - bp13[47:0]), c13[`widthOut-13-2:0]} ; 
		end
		else
		begin
			q14 <= {q13[`widthOut-2:0], 1'b0} ; 
			c14 <= c13 ; 
		end
		if (c14[`widthA + `widthFrac - 1:`widthOut - 1 - 14] >= bp14[47:0] )
		begin
			q15 <= {q14[`widthOut-2:0] , 1'b1}; 
 			c15 <= {(c14[`widthA+`widthFrac-1:`widthOut-1-14] - bp14[47:0]), c14[`widthOut-14-2:0]} ; 
		end
		else
		begin
			q15 <= {q14[`widthOut-2:0], 1'b0} ; 
			c15 <= c14 ; 
		end
		if (c15[`widthA + `widthFrac - 1:`widthOut - 1 - 15] >= bp15[47:0] )
		begin
			q16 <= {q15[`widthOut-2:0] , 1'b1}; 
 			c16 <= {(c15[`widthA+`widthFrac-1:`widthOut-1-15] - bp15[47:0]), c15[`widthOut-15-2:0]} ; 
		end
		else
		begin
			q16 <= {q15[`widthOut-2:0], 1'b0} ; 
			c16 <= c15 ; 
		end
		if (c16[`widthA + `widthFrac - 1:`widthOut - 1 - 16] >= bp16[47:0] )
		begin
			q17 <= {q16[`widthOut-2:0] , 1'b1}; 
 			c17 <= {(c16[`widthA+`widthFrac-1:`widthOut-1-16] - bp16[47:0]), c16[`widthOut-16-2:0]} ; 
		end
		else
		begin
			q17 <= {q16[`widthOut-2:0], 1'b0} ; 
			c17 <= c16 ; 
		end
		if (c17[`widthA + `widthFrac - 1:`widthOut - 1 - 17] >= bp17[47:0] )
		begin
			q18 <= {q17[`widthOut-2:0] , 1'b1}; 
 			c18 <= {(c17[`widthA+`widthFrac-1:`widthOut-1-17] - bp17[47:0]), c17[`widthOut-17-2:0]} ; 
		end
		else
		begin
			q18 <= {q17[`widthOut-2:0], 1'b0} ; 
			c18 <= c17 ; 
		end
		if (c18[`widthA + `widthFrac - 1:`widthOut - 1 - 18] >= bp18[47:0] )
		begin
			q19 <= {q18[`widthOut-2:0] , 1'b1}; 
 			c19 <= {(c18[`widthA+`widthFrac-1:`widthOut-1-18] - bp18[47:0]), c18[`widthOut-18-2:0]} ; 
		end
		else
		begin
			q19 <= {q18[`widthOut-2:0], 1'b0} ; 
			c19 <= c18 ; 
		end

		if (c20[`widthA + `widthFrac - 1:`widthOut - 1 - 20] >= bp20[47:0] )
		begin
			q21 <= {q20[`widthOut-2:0] , 1'b1}; 
 			c21 <= {(c20[`widthA+`widthFrac-1:`widthOut-1-20] - bp20[47:0]), c20[`widthOut-20-2:0]} ; 
		end
		else
		begin
			q21 <= {q20[`widthOut-2:0], 1'b0} ; 
			c21 <= c20 ; 
		end
		if (c21[`widthA + `widthFrac - 1:`widthOut - 1 - 21] >= bp21[47:0] )
		begin
			q22 <= {q21[`widthOut-2:0] , 1'b1}; 
 			c22 <= {(c21[`widthA+`widthFrac-1:`widthOut-1-21] - bp21[47:0]), c21[`widthOut-21-2:0]} ; 
		end
		else
		begin
			q22 <= {q21[`widthOut-2:0], 1'b0} ; 
			c22 <= c21 ; 
		end
		if (c22[`widthA + `widthFrac - 1:`widthOut - 1 - 22] >= bp22[47:0] )
		begin
			q23 <= {q22[`widthOut-2:0] , 1'b1}; 
 			c23 <= {(c22[`widthA+`widthFrac-1:`widthOut-1-22] - bp22[47:0]), c22[`widthOut-22-2:0]} ; 
		end
		else
		begin
			q23 <= {q22[`widthOut-2:0], 1'b0} ; 
			c23 <= c22; 
		end
		if (c23[`widthA + `widthFrac - 1:`widthOut - 1 - 23] >= bp23[47:0] )
		begin
			q24 <= {q23[`widthOut-2:0] , 1'b1}; 
 			c24 <= {(c23[`widthA+`widthFrac-1:`widthOut-1-23] - bp23[47:0]), c23[`widthOut-23-2:0]} ; 
		end
		else
		begin
			q24 <= {q23[`widthOut-2:0], 1'b0} ; 
			c24 <= c23 ; 
		end
		if (c24[`widthA + `widthFrac - 1:`widthOut - 1 - 24] >= bp24[47:0] )
		begin
			q25 <= {q24[`widthOut-2:0] , 1'b1}; 
 			c25 <= {(c24[`widthA+`widthFrac-1:`widthOut-1-24] - bp24[47:0]), c24[`widthOut-24-2:0]} ; 
		end
		else
		begin
			q25 <= {q24[`widthOut-2:0], 1'b0} ; 
			c25 <= c24 ; 
		end
		if (c25[`widthA + `widthFrac - 1:`widthOut - 1 - 25] >= bp25[47:0] )
		begin
			q26 <= {q25[`widthOut-2:0] , 1'b1}; 
 			c26 <= {(c25[`widthA+`widthFrac-1:`widthOut-1-25] - bp25[47:0]), c25[`widthOut-25-2:0]} ; 
		end
		else
		begin
			q26 <= {q25[`widthOut-2:0], 1'b0} ; 
			c26 <= c25 ; 
		end
		if (c26[`widthA + `widthFrac - 1:`widthOut - 1 - 26] >= bp26[47:0] )
		begin
			q27 <= {q26[`widthOut-2:0] , 1'b1}; 
 			c27 <= {(c26[`widthA+`widthFrac-1:`widthOut-1-26] - bp26[47:0]), c26[`widthOut-26-2:0]} ; 
		end
		else
		begin
			q27 <= {q26[`widthOut-2:0], 1'b0} ; 
			c27 <= c26 ; 
		end
		if (c27[`widthA + `widthFrac - 1:`widthOut - 1 - 27] >= bp27[47:0] )
		begin
			q28 <= {q27[`widthOut-2:0] , 1'b1}; 
 			c28 <= {(c27[`widthA+`widthFrac-1:`widthOut-1-27] - bp27[47:0]), c27[`widthOut-27-2:0]} ; 
		end
		else
		begin
			q28 <= {q27[`widthOut-2:0], 1'b0} ; 
			c28 <= c27 ; 
		end
		if (c28[`widthA + `widthFrac - 1:`widthOut - 1 - 29] >= bp28[47:0] )
		begin
			q29 <= {q28[`widthOut-2:0] , 1'b1}; 
 			c29 <= {(c28[`widthA+`widthFrac-1:`widthOut-1-29] - bp28[47:0]), c28[`widthOut-29-2:0]} ; 
		end
		else
		begin
			q29 <= {q28[`widthOut-2:0], 1'b0} ; 
			c29 <= c28 ; 
		end
		if (c29[`widthA + `widthFrac - 1:`widthOut - 1 - 29] >= bp29[47:0] )
		begin
			q30 <= {q29[`widthOut-2:0] , 1'b1}; 
 			c30 <= {(c29[`widthA+`widthFrac-1:`widthOut-1-29] - bp29[47:0]), c29[`widthOut-29-2:0]} ; 
		end
		else
		begin
			q30 <= {q29[`widthOut-2:0], 1'b0} ; 
			c30 <= c29 ; 
		end
		if (c30[`widthA + `widthFrac - 1:`widthOut - 1 - 30] >= bp30[47:0] )
		begin
			q31 <= {q30[`widthOut-2:0] , 1'b1}; 
 			c31 <= {(c30[`widthA+`widthFrac-1:`widthOut-1-30] - bp30[47:0]), c30[`widthOut-30-2:0]} ; 
		end
		else
		begin
			q31 <= {q30[`widthOut-2:0], 1'b0} ; 
			c31 <= c30 ; 
		end

          if (c31 >= bp31 )
          begin
             Qout <= {q31[`widthOut-2:0], 1'b1} ; 
          end
          else
          begin
             Qout <= {q31[`widthOut-2:0], 1'b0} ; 
          end 
    end 
 endmodule
*/
    
    
 module divide64x32x51x18 (reset, A, B, Qout, clk);

	input reset;
    input[64 - 1:0] A; 
    input[51 - 1:0] B; 
    output[32 - 1:0] Qout; 
    reg[32 - 1:0] Qout;
    input clk; 

    reg[64 + 15 - 1:0] c0; 
    reg[64 + 15 - 1:0] c1; 
    reg[64 + 15 - 1:0] c2; 
    reg[64 + 15 - 1:0] c3; 
    reg[64 + 15 - 1:0] c4; 
    reg[64 + 15 - 1:0] c5; 
    reg[64 + 15 - 1:0] c6; 
    reg[64 + 15 - 1:0] c7; 
    reg[64 + 15 - 1:0] c8; 
    reg[64 + 15 - 1:0] c9; 
    reg[64 + 15 - 1:0] c10; 
    reg[64 + 15 - 1:0] c11; 
    reg[64 + 15 - 1:0] c12; 
    reg[64 + 15 - 1:0] c13; 
    reg[64 + 15 - 1:0] c14; 
    reg[64 + 15 - 1:0] c15; 
    reg[64 + 15 - 1:0] c16; 
    reg[64 + 15 - 1:0] c17; 
    reg[64 + 15 - 1:0] c18; 
    reg[64 + 15 - 1:0] c19; 
    reg[64 + 15 - 1:0] c20; 
    reg[64 + 15 - 1:0] c21; 
    reg[64 + 15 - 1:0] c22; 
    reg[64 + 15 - 1:0] c23; 
    reg[64 + 15 - 1:0] c24; 
    reg[64 + 15 - 1:0] c25; 
    reg[64 + 15 - 1:0] c26; 
    reg[64 + 15 - 1:0] c27; 
    reg[64 + 15 - 1:0] c28; 
    reg[64 + 15 - 1:0] c29; 
    reg[64 + 15 - 1:0] c30; 
    reg[64 + 15 - 1:0] c31; 

    reg[32 - 1:0] q0; 
	reg[32 - 1:0] q1; 
    reg[32 - 1:0] q2; 
    reg[32 - 1:0] q3; 
    reg[32 - 1:0] q4; 
    reg[32 - 1:0] q5; 
    reg[32 - 1:0] q6; 
    reg[32 - 1:0] q7; 
    reg[32 - 1:0] q8; 
    reg[32 - 1:0] q9; 
    reg[32 - 1:0] q10; 
	reg[32 - 1:0] q11; 
    reg[32 - 1:0] q12; 
    reg[32 - 1:0] q13; 
    reg[32 - 1:0] q14; 
    reg[32 - 1:0] q15; 
    reg[32 - 1:0] q16; 
    reg[32 - 1:0] q17; 
    reg[32 - 1:0] q18; 
    reg[32 - 1:0] q19; 
    reg[32 - 1:0] q20; 
	reg[32 - 1:0] q21; 
    reg[32 - 1:0] q22; 
    reg[32 - 1:0] q23; 
    reg[32 - 1:0] q24; 
    reg[32 - 1:0] q25; 
    reg[32 - 1:0] q26; 
    reg[32 - 1:0] q27; 
    reg[32 - 1:0] q28; 
    reg[32 - 1:0] q29; 
    reg[32 - 1:0] q30; 
    reg[32 - 1:0] q31; 

    reg[51 - 1:0] bp0; 
    reg[51 - 1:0] bp1; 
    reg[51 - 1:0] bp2; 
    reg[51 - 1:0] bp3; 
    reg[51 - 1:0] bp4; 
    reg[51 - 1:0] bp5; 
    reg[51 - 1:0] bp6; 
    reg[51 - 1:0] bp7; 
    reg[51 - 1:0] bp8; 
    reg[51 - 1:0] bp9; 
    reg[51 - 1:0] bp10; 
    reg[51 - 1:0] bp11; 
    reg[51 - 1:0] bp12; 
    reg[51 - 1:0] bp13; 
    reg[51 - 1:0] bp14; 
    reg[51 - 1:0] bp15; 
    reg[51 - 1:0] bp16; 
    reg[51 - 1:0] bp17; 
    reg[51 - 1:0] bp18; 
    reg[51 - 1:0] bp19; 
    reg[51 - 1:0] bp20; 
    reg[51 - 1:0] bp21; 
    reg[51 - 1:0] bp22; 
    reg[51 - 1:0] bp23; 
    reg[51 - 1:0] bp24; 
    reg[51 - 1:0] bp25; 
    reg[51 - 1:0] bp26; 
    reg[51 - 1:0] bp27; 
    reg[51 - 1:0] bp28; 
    reg[51 - 1:0] bp29; 
    reg[51 - 1:0] bp30; 
    reg[51 - 1:0] bp31; 

    reg [64 + 15 - 1:0] c_xhdl;

	always @(posedge clk or negedge reset)
    begin
	if (reset == 1'b0)
	begin

    c0 <= 0; 
    c1 <= 0; 
    c2 <= 0; 
    c3 <= 0; 
    c4 <= 0; 
    c5 <= 0; 
    c6 <= 0; 
    c7 <= 0; 
    c8 <= 0; 
    c9 <= 0; 
    c10 <= 0; 
    c11 <= 0; 
    c12 <= 0; 
    c13 <= 0; 
    c14 <= 0; 
    c15 <= 0; 
    c16 <= 0; 
    c17 <= 0; 
    c18 <= 0; 
    c19 <= 0; 
    c20 <= 0; 
    c21 <= 0; 
    c22 <= 0; 
    c23 <= 0; 
    c24 <= 0; 
    c25 <= 0; 
    c26 <= 0; 
    c27 <= 0; 
    c28 <= 0; 
    c29 <= 0; 
    c30 <= 0; 
    c31 <= 0; 

    q0 <= 0; 
	q1 <= 0; 
    q2 <= 0; 
    q3 <= 0; 
    q4 <= 0; 
    q5 <= 0; 
    q6 <= 0; 
    q7 <= 0; 
    q8 <= 0; 
    q9 <= 0; 
    q10 <= 0; 
	q11 <= 0; 
    q12 <= 0; 
    q13 <= 0; 
    q14 <= 0; 
    q15 <= 0; 
    q16 <= 0; 
    q17 <= 0; 
    q18 <= 0; 
    q19 <= 0; 
    q20 <= 0; 
	q21 <= 0; 
    q22 <= 0; 
    q23 <= 0; 
    q24 <= 0; 
    q25 <= 0; 
    q26 <= 0; 
    q27 <= 0; 
    q28 <= 0; 
    q29 <= 0; 
    q30 <= 0; 
    q31 <= 0; 

    bp0 <= 0; 
    bp1 <= 0; 
    bp2 <= 0; 
    bp3 <= 0; 
    bp4 <= 0; 
    bp5 <= 0; 
    bp6 <= 0; 
    bp7 <= 0; 
    bp8 <= 0; 
    bp9 <= 0; 
    bp10 <= 0; 
    bp11 <= 0; 
    bp12 <= 0; 
    bp13 <= 0; 
    bp14 <= 0; 
    bp15 <= 0; 
    bp16 <= 0; 
    bp17 <= 0; 
    bp18 <= 0; 
    bp19 <= 0; 
    bp20 <= 0; 
    bp21 <= 0; 
    bp22 <= 0; 
    bp23 <= 0; 
    bp24 <= 0; 
    bp25 <= 0; 
    bp26 <= 0; 
    bp27 <= 0; 
    bp28 <= 0; 
    bp29 <= 0; 
    bp30 <= 0; 
    bp31 <= 0; 

	end
	else
	begin

    c0[64 + 15 - 1:15] <= A ;
    c0[15 - 1:0] <= 0;
    q0 <= 0;
    bp0 <= B ;
    bp1 <= bp0 ;
    bp2 <= bp1 ;
    bp3 <= bp2 ;
    bp4 <= bp3 ;
    bp5 <= bp4 ;
    bp6 <= bp5 ;
    bp7 <= bp6 ;
    bp8 <= bp7 ;
    bp9 <= bp8 ;
    bp10 <= bp9 ;
    bp11 <= bp10 ;
    bp12 <= bp11 ;
    bp13 <= bp12 ;
    bp14 <= bp13 ;
    bp15 <= bp14 ;
    bp16 <= bp15 ;
    bp17 <= bp16 ;
    bp18 <= bp17 ;
    bp19 <= bp18 ;
    bp20 <= bp19 ;
    bp21 <= bp20 ;
    bp22 <= bp21 ;
    bp23 <= bp22 ;
    bp24 <= bp23 ;
    bp25 <= bp24 ;
    bp26 <= bp25 ;
    bp27 <= bp26 ;
    bp28 <= bp27 ;
    bp29 <= bp28 ;
    bp30 <= bp29 ;
    bp31 <= bp30 ;

		if (c0[64 + 15 - 1:32 - 1 - 0] >= bp0[47:0] )
		begin
			q1 <= {q0[32-2:0] , 1'b1}; 
 			c1 <= {(c0[64+15-1:32-1-0] - bp0[47:0]), c0[32-0-2:0]} ; 
		end
		else
		begin
			q1 <= {q0[32-2:0], 1'b0} ; 
			c1 <= c0 ; 
		end
		if (c1[64 + 15 - 1:32 - 1 - 1] >= bp1[47:0] )
		begin
			q2 <= {q1[32-2:0] , 1'b1}; 
 			c2 <= {(c1[64+15-1:32-1-1] - bp1[47:0]), c1[32-1-2:0]} ; 
		end
		else
		begin
			q2 <= {q1[32-2:0], 1'b0} ; 
			c2 <= c1 ; 
		end
		if (c2[64 + 15 - 1:32 - 1 - 2] >= bp2[47:0] )
		begin
			q3 <= {q2[32-2:0] , 1'b1}; 
 			c3 <= {(c2[64+15-1:32-1-2] - bp2[47:0]), c2[32-2-2:0]} ; 
		end
		else
		begin
			q3 <= {q2[32-2:0], 1'b0} ; 
			c3 <= c2 ; 
		end
		if (c3[64 + 15 - 1:32 - 1 - 3] >= bp3[47:0] )
		begin
			q4 <= {q3[32-2:0] , 1'b1}; 
 			c4 <= {(c3[64+15-1:32-1-3] - bp3[47:0]), c3[32-3-2:0]} ; 
		end
		else
		begin
			q4 <= {q3[32-2:0], 1'b0} ; 
			c4 <= c3 ; 
		end
		if (c4[64 + 15 - 1:32 - 1 - 4] >= bp4[47:0] )
		begin
			q5 <= {q4[32-2:0] , 1'b1}; 
 			c5 <= {(c4[64+15-1:32-1-4] - bp4[47:0]), c4[32-4-2:0]} ; 
		end
		else
		begin
			q5 <= {q4[32-2:0], 1'b0} ; 
			c5 <= c4 ; 
		end
		if (c5[64 + 15 - 1:32 - 1 - 5] >= bp5[47:0] )
		begin
			q6 <= {q5[32-2:0] , 1'b1}; 
 			c6 <= {(c5[64+15-1:32-1-5] - bp5[47:0]), c5[32-5-2:0]} ; 
		end
		else
		begin
			q6 <= {q5[32-2:0], 1'b0} ; 
			c6 <= c5 ; 
		end
		if (c6[64 + 15 - 1:32 - 1 - 6] >= bp6[47:0] )
		begin
			q7 <= {q6[32-2:0] , 1'b1}; 
 			c7 <= {(c6[64+15-1:32-1-6] - bp6[47:0]), c6[32-6-2:0]} ; 
		end
		else
		begin
			q7 <= {q6[32-2:0], 1'b0} ; 
			c7 <= c6 ; 
		end
		if (c7[64 + 15 - 1:32 - 1 - 7] >= bp7[47:0] )
		begin
			q8 <= {q7[32-2:0] , 1'b1}; 
 			c8 <= {(c7[64+15-1:32-1-7] - bp7[47:0]), c7[32-7-2:0]} ; 
		end
		else
		begin
			q8 <= {q7[32-2:0], 1'b0} ; 
			c8 <= c7 ; 
		end
		if (c8[64 + 15 - 1:32 - 1 - 8] >= bp8[47:0] )
		begin
			q9 <= {q8[32-2:0] , 1'b1}; 
 			c9 <= {(c8[64+15-1:32-1-8] - bp8[47:0]), c8[32-8-2:0]} ; 
		end
		else
		begin
			q9 <= {q8[32-2:0], 1'b0} ; 
			c9 <= c8 ; 
		end
		if (c9[64 + 15 - 1:32 - 1 - 9] >= bp9[47:0] )
		begin
			q10 <= {q9[32-2:0] , 1'b1}; 
 			c10 <= {(c9[64+15-1:32-1-9] - bp9[47:0]), c9[32-9-2:0]} ; 
		end
		else
		begin
			q10 <= {q9[32-2:0], 1'b0} ; 
			c10 <= c9 ; 
		end
		if (c10[64 + 15 - 1:32 - 1 - 10] >= bp10[47:0] )
		begin
			q11 <= {q10[32-2:0] , 1'b1}; 
 			c11 <= {(c10[64+15-1:32-1-10] - bp10[47:0]), c10[32-10-2:0]} ; 
		end
		else
		begin
			q11 <= {q10[32-2:0], 1'b0} ; 
			c11 <= c10 ; 
		end

		if (c11[64 + 15 - 1:32 - 1 - 11] >= bp11[47:0] )
		begin
			q12 <= {q11[32-2:0] , 1'b1}; 
 			c12 <= {(c11[64+15-1:32-1-11] - bp11[47:0]), c11[32-11-2:0]} ; 
		end
		else
		begin
			q12 <= {q11[32-2:0], 1'b0} ; 
			c12 <= c11 ; 
		end
		if (c12[64 + 15 - 1:32 - 1 - 12] >= bp12[47:0] )
		begin
			q13 <= {q12[32-2:0] , 1'b1}; 
 			c13 <= {(c12[64+15-1:32-1-12] - bp12[47:0]), c12[32-12-2:0]} ; 
		end
		else
		begin
			q13 <= {q12[32-2:0], 1'b0} ; 
			c13 <= c12 ; 
		end
		if (c13[64 + 15 - 1:32 - 1 - 13] >= bp13[47:0] )
		begin
			q14 <= {q13[32-2:0] , 1'b1}; 
 			c14 <= {(c13[64+15-1:32-1-13] - bp13[47:0]), c13[32-13-2:0]} ; 
		end
		else
		begin
			q14 <= {q13[32-2:0], 1'b0} ; 
			c14 <= c13 ; 
		end
		if (c14[64 + 15 - 1:32 - 1 - 14] >= bp14[47:0] )
		begin
			q15 <= {q14[32-2:0] , 1'b1}; 
 			c15 <= {(c14[64+15-1:32-1-14] - bp14[47:0]), c14[32-14-2:0]} ; 
		end
		else
		begin
			q15 <= {q14[32-2:0], 1'b0} ; 
			c15 <= c14 ; 
		end
		if (c15[64 + 15 - 1:32 - 1 - 15] >= bp15[47:0] )
		begin
			q16 <= {q15[32-2:0] , 1'b1}; 
 			c16 <= {(c15[64+15-1:32-1-15] - bp15[47:0]), c15[32-15-2:0]} ; 
		end
		else
		begin
			q16 <= {q15[32-2:0], 1'b0} ; 
			c16 <= c15 ; 
		end
		if (c16[64 + 15 - 1:32 - 1 - 16] >= bp16[47:0] )
		begin
			q17 <= {q16[32-2:0] , 1'b1}; 
 			c17 <= {(c16[64+15-1:32-1-16] - bp16[47:0]), c16[32-16-2:0]} ; 
		end
		else
		begin
			q17 <= {q16[32-2:0], 1'b0} ; 
			c17 <= c16 ; 
		end
		if (c17[64 + 15 - 1:32 - 1 - 17] >= bp17[47:0] )
		begin
			q18 <= {q17[32-2:0] , 1'b1}; 
 			c18 <= {(c17[64+15-1:32-1-17] - bp17[47:0]), c17[32-17-2:0]} ; 
		end
		else
		begin
			q18 <= {q17[32-2:0], 1'b0} ; 
			c18 <= c17 ; 
		end
		if (c18[64 + 15 - 1:32 - 1 - 18] >= bp18[47:0] )
		begin
			q19 <= {q18[32-2:0] , 1'b1}; 
 			c19 <= {(c18[64+15-1:32-1-18] - bp18[47:0]), c18[32-18-2:0]} ; 
		end
		else
		begin
			q19 <= {q18[32-2:0], 1'b0} ; 
			c19 <= c18 ; 
		end
		if (c19[64 + 15 - 1:32 - 1 - 19] >= bp19[47:0] )
		begin
			q20 <= {q19[32-2:0] , 1'b1}; 
 			c20 <= {(c19[64+15-1:32-1-19] - bp19[47:0]), c19[32-19-2:0]} ; 
		end
		else
		begin
			q20 <= {q19[32-2:0], 1'b0} ; 
			c20 <= c19 ; 
		end

		if (c20[64 + 15 - 1:32 - 1 - 20] >= bp20[47:0] )
		begin
			q21 <= {q20[32-2:0] , 1'b1}; 
 			c21 <= {(c20[64+15-1:32-1-20] - bp20[47:0]), c20[32-20-2:0]} ; 
		end
		else
		begin
			q21 <= {q20[32-2:0], 1'b0} ; 
			c21 <= c20 ; 
		end
		if (c21[64 + 15 - 1:32 - 1 - 21] >= bp21[47:0] )
		begin
			q22 <= {q21[32-2:0] , 1'b1}; 
 			c22 <= {(c21[64+15-1:32-1-21] - bp21[47:0]), c21[32-21-2:0]} ; 
		end
		else
		begin
			q22 <= {q21[32-2:0], 1'b0} ; 
			c22 <= c21 ; 
		end
		if (c22[64 + 15 - 1:32 - 1 - 22] >= bp22[47:0] )
		begin
			q23 <= {q22[32-2:0] , 1'b1}; 
 			c23 <= {(c22[64+15-1:32-1-22] - bp22[47:0]), c22[32-22-2:0]} ; 
		end
		else
		begin
			q23 <= {q22[32-2:0], 1'b0} ; 
			c23 <= c22; 
		end
		if (c23[64 + 15 - 1:32 - 1 - 23] >= bp23[47:0] )
		begin
			q24 <= {q23[32-2:0] , 1'b1}; 
 			c24 <= {(c23[64+15-1:32-1-23] - bp23[47:0]), c23[32-23-2:0]} ; 
		end
		else
		begin
			q24 <= {q23[32-2:0], 1'b0} ; 
			c24 <= c23 ; 
		end
		if (c24[64 + 15 - 1:32 - 1 - 24] >= bp24[47:0] )
		begin
			q25 <= {q24[32-2:0] , 1'b1}; 
 			c25 <= {(c24[64+15-1:32-1-24] - bp24[47:0]), c24[32-24-2:0]} ; 
		end
		else
		begin
			q25 <= {q24[32-2:0], 1'b0} ; 
			c25 <= c24 ; 
		end
		if (c25[64 + 15 - 1:32 - 1 - 25] >= bp25[47:0] )
		begin
			q26 <= {q25[32-2:0] , 1'b1}; 
 			c26 <= {(c25[64+15-1:32-1-25] - bp25[47:0]), c25[32-25-2:0]} ; 
		end
		else
		begin
			q26 <= {q25[32-2:0], 1'b0} ; 
			c26 <= c25 ; 
		end
		if (c26[64 + 15 - 1:32 - 1 - 26] >= bp26[47:0] )
		begin
			q27 <= {q26[32-2:0] , 1'b1}; 
 			c27 <= {(c26[64+15-1:32-1-26] - bp26[47:0]), c26[32-26-2:0]} ; 
		end
		else
		begin
			q27 <= {q26[32-2:0], 1'b0} ; 
			c27 <= c26 ; 
		end
		if (c27[64 + 15 - 1:32 - 1 - 27] >= bp27[47:0] )
		begin
			q28 <= {q27[32-2:0] , 1'b1}; 
 			c28 <= {(c27[64+15-1:32-1-27] - bp27[47:0]), c27[32-27-2:0]} ; 
		end
		else
		begin
			q28 <= {q27[32-2:0], 1'b0} ; 
			c28 <= c27 ; 
		end
		if (c28[64 + 15 - 1:32 - 1 - 29] >= bp28[47:0] )
		begin
			q29 <= {q28[32-2:0] , 1'b1}; 
 			c29 <= {(c28[64+15-1:32-1-29] - bp28[47:0]), c28[32-29-2:0]} ; 
		end
		else
		begin
			q29 <= {q28[32-2:0], 1'b0} ; 
			c29 <= c28 ; 
		end
		if (c29[64 + 15 - 1:32 - 1 - 29] >= bp29[47:0] )
		begin
			q30 <= {q29[32-2:0] , 1'b1}; 
 			c30 <= {(c29[64+15-1:32-1-29] - bp29[47:0]), c29[32-29-2:0]} ; 
		end
		else
		begin
			q30 <= {q29[32-2:0], 1'b0} ; 
			c30 <= c29 ; 
		end
		if (c30[64 + 15 - 1:32 - 1 - 30] >= bp30[47:0] )
		begin
			q31 <= {q30[32-2:0] , 1'b1}; 
 			c31 <= {(c30[64+15-1:32-1-30] - bp30[47:0]), c30[32-30-2:0]} ; 
		end
		else
		begin
			q31 <= {q30[32-2:0], 1'b0} ; 
			c31 <= c30 ; 
		end

          if (c31 >= bp31 )
          begin
             Qout <= {q31[32-2:0], 1'b1} ; 
          end
          else
          begin
             Qout <= {q31[32-2:0], 1'b0} ; 
          end 
	end
    end 
 endmodule

    
    
 module divide64x16x51x16 (reset, A, B, Qout, clk);

	input reset;
    input[64 - 1:0] A; 
    input[51 - 1:0] B; 
    output[16 - 1:0] Qout; 
    reg[16 - 1:0] Qout;
    input clk; 

    reg[64 + 16 - 1:0] c0; 
    reg[64 + 16 - 1:0] c1; 
    reg[64 + 16 - 1:0] c2; 
    reg[64 + 16 - 1:0] c3; 
    reg[64 + 16 - 1:0] c4; 
    reg[64 + 16 - 1:0] c5; 
    reg[64 + 16 - 1:0] c6; 
    reg[64 + 16 - 1:0] c7; 
    reg[64 + 16 - 1:0] c8; 
    reg[64 + 16 - 1:0] c9; 
    reg[64 + 16 - 1:0] c10; 
    reg[64 + 16 - 1:0] c11; 
    reg[64 + 16 - 1:0] c12; 
    reg[64 + 16 - 1:0] c13; 
    reg[64 + 16 - 1:0] c14; 
    reg[64 + 16 - 1:0] c15; 

    reg[16 - 1:0] q0; 
	reg[16 - 1:0] q1; 
    reg[16 - 1:0] q2; 
    reg[16 - 1:0] q3; 
    reg[16 - 1:0] q4; 
    reg[16 - 1:0] q5; 
    reg[16 - 1:0] q6; 
    reg[16 - 1:0] q7; 
    reg[16 - 1:0] q8; 
    reg[16 - 1:0] q9; 
    reg[16 - 1:0] q10; 
	reg[16 - 1:0] q11; 
    reg[16 - 1:0] q12; 
    reg[16 - 1:0] q13; 
    reg[16 - 1:0] q14; 
    reg[16 - 1:0] q15; 

    reg[51 - 1:0] bp0; 
    reg[51 - 1:0] bp1; 
    reg[51 - 1:0] bp2; 
    reg[51 - 1:0] bp3; 
    reg[51 - 1:0] bp4; 
    reg[51 - 1:0] bp5; 
    reg[51 - 1:0] bp6; 
    reg[51 - 1:0] bp7; 
    reg[51 - 1:0] bp8; 
    reg[51 - 1:0] bp9; 
    reg[51 - 1:0] bp10; 
    reg[51 - 1:0] bp11; 
    reg[51 - 1:0] bp12; 
    reg[51 - 1:0] bp13; 
    reg[51 - 1:0] bp14; 
    reg[51 - 1:0] bp15; 

    always @(posedge clk or negedge reset)
    begin
	if (reset == 1'b0)
	begin

    c0 <= 0; 
    c1 <= 0; 
    c2 <= 0; 
    c3 <= 0; 
    c4 <= 0; 
    c5 <= 0; 
    c6 <= 0; 
    c7 <= 0; 
    c8 <= 0; 
    c9 <= 0; 
    c10 <= 0; 
    c11 <= 0; 
    c12 <= 0; 
    c13 <= 0; 
    c14 <= 0; 
    c15 <= 0; 
        q0 <= 0; 
	q1 <= 0; 
    q2 <= 0; 
    q3 <= 0; 
    q4 <= 0; 
    q5 <= 0; 
    q6 <= 0; 
    q7 <= 0; 
    q8 <= 0; 
    q9 <= 0; 
    q10 <= 0; 
	q11 <= 0; 
    q12 <= 0; 
    q13 <= 0; 
    q14 <= 0; 
        bp0 <= 0; 
    bp1 <= 0; 
    bp2 <= 0; 
    bp3 <= 0; 
    bp4 <= 0; 
    bp5 <= 0; 
    bp6 <= 0; 
    bp7 <= 0; 
    bp8 <= 0; 
    bp9 <= 0; 
    bp10 <= 0; 
    bp11 <= 0; 
    bp12 <= 0; 
    bp13 <= 0; 
    bp14 <= 0; 
    bp15 <= 0; 
   	end
	else
	begin


    c0[64 + 16 - 1:16] <= A ;
    c0[16 - 1:0] <= 0;
    q0 <= 0;
    bp0 <= B ;
	bp1 <= bp0 ;
    bp2 <= bp1 ;
    bp3 <= bp2 ;
    bp4 <= bp3 ;
    bp5 <= bp4 ;
    bp6 <= bp5 ;
    bp7 <= bp6 ;
    bp8 <= bp7 ;
    bp9 <= bp8 ;
    bp10 <= bp9 ;
    bp11 <= bp10 ;
    bp12 <= bp11 ;
    bp13 <= bp12 ;
    bp14 <= bp13 ;
    bp15 <= bp14 ;


		if (c0[64 + 16 - 1:16 - 1 - 0] >= bp0[47:0] )
		begin
			q1 <= {q0[16-2:0] , 1'b1}; 
 			c1 <= {(c0[64+16-1:16-1-0] - bp0[47:0]), c0[16-0-2:0]} ; 
		end
		else
		begin
			q1 <= {q0[16-2:0], 1'b0} ; 
			c1 <= c0 ; 
		end
		if (c1[64 + 16 - 1:16 - 1 - 1] >= bp1[47:0] )
		begin
			q2 <= {q1[16-2:0] , 1'b1}; 
 			c2 <= {(c1[64+16-1:16-1-1] - bp1[47:0]), c1[16-1-2:0]} ; 
		end
		else
		begin
			q2 <= {q1[16-2:0], 1'b0} ; 
			c2 <= c1 ; 
		end
		if (c2[64 + 16 - 1:16 - 1 - 2] >= bp2[47:0] )
		begin
			q3 <= {q2[16-2:0] , 1'b1}; 
 			c3 <= {(c2[64+16-1:16-1-2] - bp2[47:0]), c2[16-2-2:0]} ; 
		end
		else
		begin
			q3 <= {q2[16-2:0], 1'b0} ; 
			c3 <= c2 ; 
		end
		if (c3[64 + 16 - 1:16 - 1 - 3] >= bp3[47:0] )
		begin
			q4 <= {q3[16-2:0] , 1'b1}; 
 			c4 <= {(c3[64+16-1:16-1-3] - bp3[47:0]), c3[16-3-2:0]} ; 
		end
		else
		begin
			q4 <= {q3[16-2:0], 1'b0} ; 
			c4 <= c3 ; 
		end
		if (c4[64 + 16 - 1:16 - 1 - 4] >= bp4[47:0] )
		begin
			q5 <= {q4[16-2:0] , 1'b1}; 
 			c5 <= {(c4[64+16-1:16-1-4] - bp4[47:0]), c4[16-4-2:0]} ; 
		end
		else
		begin
			q5 <= {q4[16-2:0], 1'b0} ; 
			c5 <= c4 ; 
		end
		if (c5[64 + 16 - 1:16 - 1 - 5] >= bp5[47:0] )
		begin
			q6 <= {q5[16-2:0] , 1'b1}; 
 			c6 <= {(c5[64+16-1:16-1-5] - bp5[47:0]), c5[16-5-2:0]} ; 
		end
		else
		begin
			q6 <= {q5[16-2:0], 1'b0} ; 
			c6 <= c5 ; 
		end
		if (c6[64 + 16 - 1:16 - 1 - 6] >= bp6[47:0] )
		begin
			q7 <= {q6[16-2:0] , 1'b1}; 
 			c7 <= {(c6[64+16-1:16-1-6] - bp6[47:0]), c6[16-6-2:0]} ; 
		end
		else
		begin
			q7 <= {q6[16-2:0], 1'b0} ; 
			c7 <= c6 ; 
		end
		if (c7[64 + 16 - 1:16 - 1 - 7] >= bp7[47:0] )
		begin
			q8 <= {q7[16-2:0] , 1'b1}; 
 			c8 <= {(c7[64+16-1:16-1-7] - bp7[47:0]), c7[16-7-2:0]} ; 
		end
		else
		begin
			q8 <= {q7[16-2:0], 1'b0} ; 
			c8 <= c7 ; 
		end
		if (c8[64 + 16 - 1:16 - 1 - 8] >= bp8[47:0] )
		begin
			q9 <= {q8[16-2:0] , 1'b1}; 
 			c9 <= {(c8[64+16-1:16-1-8] - bp8[47:0]), c8[16-8-2:0]} ; 
		end
		else
		begin
			q9 <= {q8[16-2:0], 1'b0} ; 
			c9 <= c8 ; 
		end
		if (c9[64 + 16 - 1:16 - 1 - 9] >= bp9[47:0] )
		begin
			q10 <= {q9[16-2:0] , 1'b1}; 
 			c10 <= {(c9[64+16-1:16-1-9] - bp9[47:0]), c9[16-9-2:0]} ; 
		end
		else
		begin
			q10 <= {q9[16-2:0], 1'b0} ; 
			c10 <= c9 ; 
		end
		if (c10[64 + 16 - 1:16 - 1 - 10] >= bp10[47:0] )
		begin
			q11 <= {q10[16-2:0] , 1'b1}; 
 			c11 <= {(c10[64+16-1:16-1-10] - bp10[47:0]), c10[16-10-2:0]} ; 
		end
		else
		begin
			q11 <= {q10[16-2:0], 1'b0} ; 
			c11 <= c10 ; 
		end

		if (c11[64 + 16 - 1:16 - 1 - 11] >= bp11[47:0] )
		begin
			q12 <= {q11[16-2:0] , 1'b1}; 
 			c12 <= {(c11[64+16-1:16-1-11] - bp11[47:0]), c11[16-11-2:0]} ; 
		end
		else
		begin
			q12 <= {q11[16-2:0], 1'b0} ; 
			c12 <= c11 ; 
		end
		if (c12[64 + 16 - 1:16 - 1 - 12] >= bp12[47:0] )
		begin
			q13 <= {q12[16-2:0] , 1'b1}; 
 			c13 <= {(c12[64+16-1:16-1-12] - bp12[47:0]), c12[16-12-2:0]} ; 
		end
		else
		begin
			q13 <= {q12[16-2:0], 1'b0} ; 
			c13 <= c12 ; 
		end
		if (c13[64 + 16 - 1:16 - 1 - 13] >= bp13[47:0] )
		begin
			q14 <= {q13[16-2:0] , 1'b1}; 
 			c14 <= {(c13[64+16-1:16-1-13] - bp13[47:0]), c13[16-13-2:0]} ; 
		end
		else
		begin
			q14 <= {q13[16-2:0], 1'b0} ; 
			c14 <= c13 ; 
		end
		if (c14[64 + 16 - 1:16 - 1 - 14] >= bp14[47:0] )
		begin
			q15 <= {q14[16-2:0] , 1'b1}; 
 			c15 <= {(c14[64+16-1:16-1-14] - bp14[47:0]), c14[16-14-2:0]} ; 
		end
		else
		begin
			q15 <= {q14[16-2:0], 1'b0} ; 
			c15 <= c14 ; 
		end

          if (c15 >= bp15 )
          begin
             Qout <= {q15[16-2:0], 1'b1} ; 
          end
          else
          begin
             Qout <= {q15[16-2:0], 1'b0} ; 
          end 
	end
    end 
 endmodule

 //--------------------------------------------
 // Pipelined Vector Cross Product Component --
 //  C = A x B                               --
 //  Performs a vector cross product in 2    --
 //  clock cycles.  Synplify's pipeline      --
 //  option should be enable to better       --
 //  balance the pipeline cycles.            --
 //--------------------------------------------

    
    
 	
 module crossproduct16x16 (Ax, Ay, Az, Bx, By, Bz, Cx, Cy, Cz, clk);

    input[16 - 1:0] Ax; 
    input[16 - 1:0] Ay; 
    input[16 - 1:0] Az; 
    input[16 - 1:0] Bx; 
    input[16 - 1:0] By; 
    input[16 - 1:0] Bz; 
    output[16 + 16:0] Cx; 
    reg[16 + 16:0] Cx;
    output[16 + 16:0] Cy; 
    reg[16 + 16:0] Cy;
    output[16 + 16:0] Cz; 
    reg[16 + 16:0] Cz;
    input clk; 

    reg[16 + 16 - 1:0] AyBz; 
    reg[16 + 16 - 1:0] AzBy; 
    reg[16 + 16 - 1:0] AzBx; 
    reg[16 + 16 - 1:0] AxBz; 
    reg[16 + 16 - 1:0] AxBy; 
    reg[16 + 16 - 1:0] AyBx; 

    always @(posedge clk)
    begin
       AyBz <= Ay * Bz ; 
       AzBy <= Az * By ; 
       AzBx <= Az * Bx ; 
       AxBz <= Ax * Bz ; 
       AxBy <= Ax * By ; 
       AyBx <= Ay * Bx ; 
       Cx <= ({AyBz[16 + 16 - 1], AyBz}) - ({AzBy[16 + 16 - 1], AzBy}) ; 
       Cy <= ({AzBx[16 + 16 - 1], AzBx}) - ({AxBz[16 + 16 - 1], AxBz}) ; 
       Cz <= ({AxBy[16 + 16 - 1], AxBy}) - ({AyBx[16 + 16 - 1], AyBx}) ;  
    end 
 endmodule

 module crossproduct29x16 (Ax, Ay, Az, Bx, By, Bz, Cx, Cy, Cz, clk);

    input[29 - 1:0] Ax; 
    input[29 - 1:0] Ay; 
    input[29 - 1:0] Az; 
    input[16 - 1:0] Bx; 
    input[16 - 1:0] By; 
    input[16 - 1:0] Bz; 
    output[29 + 16:0] Cx; 
    reg[29 + 16:0] Cx;
    output[29 + 16:0] Cy; 
    reg[29 + 16:0] Cy;
    output[29 + 16:0] Cz; 
    reg[29 + 16:0] Cz;
    input clk; 

    reg[29 + 16 - 1:0] AyBz; 
    reg[29 + 16 - 1:0] AzBy; 
    reg[29 + 16 - 1:0] AzBx; 
    reg[29 + 16 - 1:0] AxBz; 
    reg[29 + 16 - 1:0] AxBy; 
    reg[29 + 16 - 1:0] AyBx; 

    always @(posedge clk)
    begin
       AyBz <= Ay * Bz ; 
       AzBy <= Az * By ; 
       AzBx <= Az * Bx ; 
       AxBz <= Ax * Bz ; 
       AxBy <= Ax * By ; 
       AyBx <= Ay * Bx ; 
       Cx <= ({AyBz[29 + 16 - 1], AyBz}) - ({AzBy[29 + 16 - 1], AzBy}) ; 
       Cy <= ({AzBx[29 + 16 - 1], AzBx}) - ({AxBz[29 + 16 - 1], AxBz}) ; 
       Cz <= ({AxBy[29 + 16 - 1], AxBy}) - ({AyBx[29 + 16 - 1], AyBx}) ;  
    end 
 endmodule
 //---------------------------------------
 // Signed Vector Subtraction Component --
 //   C = A - B                         --
 //   The output, C, is latched         --
 //---------------------------------------

 module vectsub (Ax, Ay, Az, Bx, By, Bz, Cx, Cy, Cz, clk);

    input[28 - 1:0] Ax; 

    input[28 - 1:0] Ay; 
    input[28 - 1:0] Az; 
    input[28 - 1:0] Bx; 
    input[28 - 1:0] By; 
    input[28 - 1:0] Bz; 
    output[28:0] Cx; 
    reg[28:0] Cx;
    output[28:0] Cy; 
    reg[28:0] Cy;
    output[28:0] Cz; 
    reg[28:0] Cz;
    input clk; 


    always @(posedge clk)
    begin
       Cx <= ({Ax[28 - 1], Ax}) - ({Bx[28 - 1], Bx}) ; 
       Cy <= ({Ay[28 - 1], Ay}) - ({By[28 - 1], By}) ; 
       Cz <= ({Az[28 - 1], Az}) - ({Bz[28 - 1], Bz}) ;  
    end 
 endmodule

//-----------------------------------------
 // Variable Length Vector Shift Register --
 //  Provides a specified number of       --
 //  clock cycle delay for a 3 signals    --
 //-----------------------------------------

 module vectdelay29x2 (xin, yin, zin, xout, yout, zout, clk);
    input[29 - 1:0] xin; 
    input[29 - 1:0] yin; 
    input[29 - 1:0] zin; 
    output[29 - 1:0] xout; 
    wire[29 - 1:0] xout;
    output[29 - 1:0] yout; 
    wire[29 - 1:0] yout;
    output[29 - 1:0] zout; 
    wire[29 - 1:0] zout;
    input clk; 

    reg[29 - 1:0] bufferx0; 
    reg[29 - 1:0] bufferx1; 
    reg[29 - 1:0] buffery0; 
    reg[29 - 1:0] buffery1; 
    reg[29 - 1:0] bufferz0; 
    reg[29 - 1:0] bufferz1; 

    assign xout = bufferx1 ;
    assign yout = buffery1 ;
    assign zout = bufferz1 ;

    always @(posedge clk)
    begin
       bufferx0 <= xin ; 
       buffery0 <= yin ; 
       bufferz0 <= zin ; 

       bufferx1 <= bufferx0 ; 
       buffery1 <= buffery0 ; 
       bufferz1 <= bufferz0 ; 
    end 
 endmodule


 module vectdelay16x2 (xin, yin, zin, xout, yout, zout, clk);
    input[16 - 1:0] xin; 
    input[16 - 1:0] yin; 
    input[16 - 1:0] zin; 
    output[16 - 1:0] xout; 
    wire[16 - 1:0] xout;
    output[16 - 1:0] yout; 
    wire[16 - 1:0] yout;
    output[16 - 1:0] zout; 
    wire[16 - 1:0] zout;
    input clk; 

    reg[16 - 1:0] bufferx0; 
    reg[16 - 1:0] bufferx1; 
    reg[16 - 1:0] buffery0; 
    reg[16 - 1:0] buffery1; 
    reg[16 - 1:0] bufferz0; 
    reg[16 - 1:0] bufferz1; 

    assign xout = bufferx1 ;
    assign yout = buffery1 ;
    assign zout = bufferz1 ;

    always @(posedge clk)
    begin
       bufferx0 <= xin ; 
       buffery0 <= yin ; 
       bufferz0 <= zin ; 

       bufferx1 <= bufferx0 ; 
       buffery1 <= buffery0 ; 
       bufferz1 <= bufferz0 ; 
    end 
 endmodule


 module vectdelay16x1 (xin, yin, zin, xout, yout, zout, clk);
    input[16 - 1:0] xin; 
    input[16 - 1:0] yin; 
    input[16 - 1:0] zin; 
    output[16 - 1:0] xout; 
    wire[16 - 1:0] xout;
    output[16 - 1:0] yout; 
    wire[16 - 1:0] yout;
    output[16 - 1:0] zout; 
    wire[16 - 1:0] zout;
    input clk; 

    reg[16 - 1:0] bufferx0; 
    reg[16 - 1:0] buffery0; 
    reg[16 - 1:0] bufferz0; 

    assign xout = bufferx0 ;
    assign yout = buffery0 ;
    assign zout = bufferz0 ;

    always @(posedge clk)
    begin
       bufferx0 <= xin ; 
       buffery0 <= yin ; 
       bufferz0 <= zin ; 
    end 
 endmodule
 //------------------------------------------
 // Pipelined Vector Dot Product Component --
 //  C = A . B                             --
 //  Performs a vector cross product in 2  --
 //  clock cycles.  Synplify's pipeline    --
 //  option should be enable to better     --
 //  balance the pipeline cycles.          --
 //------------------------------------------




 module dotproduct16x33 (Ax, Ay, Az, Bx, By, Bz, C, clk);

    input[16 - 1:0] Ax; 
    input[16 - 1:0] Ay; 
    input[16 - 1:0] Az; 
    input[33 - 1:0] Bx; 
    input[33 - 1:0] By; 
    input[33 - 1:0] Bz; 
    output[16 + 33 + 1:0] C; 
    reg[16 + 33 + 1:0] C;
    input clk; 

    reg[16 + 33 - 1:0] AxBx; 
    reg[16 + 33 - 1:0] AyBy; 
    reg[16 + 33 - 1:0] AzBz; 

    always @(posedge clk)
    begin
       AxBx <= Ax * Bx ; 
       AyBy <= Ay * By ; 
       AzBz <= Az * Bz ; 
       C <= ({AxBx[16 + 33 - 1], AxBx[16 + 33 - 1], AxBx}) + ({AyBy[16 + 33 - 1], AyBy[16 + 33 - 1], AyBy}) + ({AzBz[16 + 33 - 1], AzBz[16 + 33 - 1], AzBz}) ;  
    end 
 endmodule

 module dotproduct29x33 (Ax, Ay, Az, Bx, By, Bz, C, clk);

    input[29 - 1:0] Ax; 
    input[29 - 1:0] Ay; 
    input[29 - 1:0] Az; 
    input[33 - 1:0] Bx; 
    input[33 - 1:0] By; 
    input[33 - 1:0] Bz; 
    output[29 + 33 + 1:0] C; 
    reg[29 + 33 + 1:0] C;
    input clk; 

    reg[29 + 33 - 1:0] AxBx; 
    reg[29 + 33 - 1:0] AyBy; 
    reg[29 + 33 - 1:0] AzBz; 

    always @(posedge clk)
    begin
       AxBx <= Ax * Bx ; 
       AyBy <= Ay * By ; 
       AzBz <= Az * Bz ; 
       C <= ({AxBx[29 + 33 - 1], AxBx[29 + 33 - 1], AxBx}) + ({AyBy[29 + 33 - 1], AyBy[29 + 33 - 1], AyBy}) + ({AzBz[29 + 33 - 1], AzBz[29 + 33 - 1], AzBz}) ;  
    end 
 endmodule

 module dotproduct16x46 (Ax, Ay, Az, Bx, By, Bz, C, clk);

    input[16 - 1:0] Ax; 
    input[16 - 1:0] Ay; 
    input[16 - 1:0] Az; 
    input[46 - 1:0] Bx; 
    input[46 - 1:0] By; 
    input[46 - 1:0] Bz; 
    output[16 + 46 + 1:0] C; 
    reg[16 + 46 + 1:0] C;
    input clk; 

    reg[16 + 46 - 1:0] AxBx; 
    reg[16 + 46 - 1:0] AyBy; 
    reg[16 + 46 - 1:0] AzBz; 

    always @(posedge clk)
    begin
       AxBx <= Ax * Bx ; 
       AyBy <= Ay * By ; 
       AzBz <= Az * Bz ; 
       C <= ({AxBx[16 + 46 - 1], AxBx[16 + 46 - 1], AxBx}) + ({AyBy[16 + 46 - 1], AyBy[16 + 46 - 1], AyBy}) + ({AzBz[16 + 46 - 1], AzBz[16 + 46 - 1], AzBz}) ;  
    end 
 endmodule
 //--------------------------------
 // Vector Mux Component         --
 //  C = A when ABn = '1' else B --
 //--------------------------------
    
 module vectexchange (Ax, Ay, Az, Bx, By, Bz, Cx, Cy, Cz, ABn);

    input[16 - 1:0] Ax; 
    input[16 - 1:0] Ay; 

    input[16 - 1:0] Az; 
    input[16 - 1:0] Bx; 
    input[16 - 1:0] By; 
    input[16 - 1:0] Bz; 
    output[16 - 1:0] Cx; 
    wire[16 - 1:0] Cx;
    output[16 - 1:0] Cy; 
    wire[16 - 1:0] Cy;
    output[16 - 1:0] Cz; 
    wire[16 - 1:0] Cz;
    input ABn; 


    assign Cx = (ABn == 1'b1) ? Ax : Bx ;
    assign Cy = (ABn == 1'b1) ? Ay : By ;
    assign Cz = (ABn == 1'b1) ? Az : Bz ;
 endmodule




 //------------------------------------------
 // Variable Combinational Shift Component --
 //                                        --
 //  B = A shifted left by specified amt   --
 //                                        --
 //  Factor   Bits Shifted Right           --
 //    00      0    1                      --
 //    01      4    1/16                   --
 //    10      8    1/256                  --

 //    11      12   1/4096                 --
 //------------------------------------------
    
 module shifter (A, B, factor);

    input[64 - 1:0] A; 
    output[64 - 1:0] B; 
    reg[64 - 1:0] B;
    input[1:0] factor; 

    always @(factor or A)
    begin

       case (factor)
          2'b00 :
                   begin
                      B = A ; 
                   end
          2'b01 :
                   begin
                      B = {4'b0000, A[64 - 1:4]} ; 
                   end
          2'b10 :
                   begin
                      B = {8'b00000000, A[64 - 1:8]} ; 

                   end
          2'b11 :
                   begin
                      B = {12'b000000000000, A[64 - 1:12]} ; 
                   end
       endcase 
    end 
 endmodule
 //------------------------------------------
 // Nearest Triangle Hit Compare Component --
 //                                        --
 //  This unit keeps track of the closest  --
 //  triangle that has currently been hit  --
 //                                        --
 //  tin,uin,vin,triIDin,hit are inputs    --
 //  t,u,v,triID,anyhit are outputs        --
 //  enable must be high for compare       --
 //  reset will allow a new hit to be      --
 //   found during the reset cycle         --
 //------------------------------------------




module nearcmp (tin, uin, vin, triIDin, hit, t, u, v, triID, anyhit, enable, reset, globalreset, clk);

    input[31:0] tin; 
    input[15:0] uin; 
    input[15:0] vin; 
    input[15:0] triIDin; 
    input hit; 
    output[31:0] t; 
    reg[31:0] t;
    output[15:0] u; 
    reg[15:0] u;
    output[15:0] v; 

    reg[15:0] v;
    output[15:0] triID; 
    reg[15:0] triID;
    output anyhit; 
    wire anyhit;
    reg temp_anyhit;
    input enable; 
    input reset; 
    input globalreset; 
    input clk; 


    reg state; 
    reg next_state; 
    reg latchnear; 

	assign anyhit = temp_anyhit;

    always @(posedge clk or posedge globalreset)
    begin
       if (globalreset == 1'b1)
       begin
          state <= 0 ; 
          t <= 0;

          u <= 0;
          v <= 0;
          triID <= 0;
       end
       else
       begin
          state <= next_state ; 
          if (latchnear == 1'b1)
          begin
             t <= tin ; 
             u <= uin ; 
             v <= vin ; 

             triID <= triIDin ; 
          end 
       end 
    end 

    always @(state or tin or t or enable or hit or reset)
    begin

//      latchnear = 1'b0 ; 
       case (state)
          0 :
                   begin
                      if ((enable == 1'b1) & (hit == 1'b1))
                      begin

                         next_state = 1 ; 
                         latchnear = 1'b1 ; 
                      end
                      else
                      begin
                         next_state = 0 ; 
                      end 
				    temp_anyhit = 1'b0;
                   end
          1 :
                   begin
                      if (reset == 1'b1)
                      begin

                         if ((enable == 1'b1) & (hit == 1'b1))
                         begin
                            latchnear = 1'b1 ; 
                            next_state = 1 ; 
                         end
                         else
                         begin
                            next_state = 0 ; 
                         end 
                      end
                      else
                      begin

                         if ((enable == 1'b1) & (hit == 1'b1))
                         begin
                            if (tin < t)
                            begin
                               latchnear = 1'b1 ; 
                            end 
                         end 
                         next_state = 1 ; 
                      end 
				    temp_anyhit = 1'b1;
                   end
       endcase 
    end 

 endmodule

    
    
    

 module nearcmpspec (tin, uin, vin, triIDin, hit, t, u, v, triID, anyhit, enable, enablenear, reset, globalreset, clk);

    input[31:0] tin; 
    input[15:0] uin; 
    input[15:0] vin; 
    input[15:0] triIDin; 
    input hit; 
    output[31:0] t; 
    reg[31:0] t;
    output[15:0] u; 
    reg[15:0] u;
    output[15:0] v; 

    reg[15:0] v;
    output[15:0] triID; 
    reg[15:0] triID;
    output anyhit; 
    wire anyhit;
    reg temp_anyhit;
    input enable; 
    input enablenear; 
    input reset; 
    input globalreset; 
    input clk; 

    reg[1:0] state; 
    reg[1:0] next_state; 
    reg latchnear; 

	assign anyhit = temp_anyhit;

    always @(posedge clk or posedge globalreset)
    begin
       if (globalreset == 1'b1)
       begin

          state <= 0 ; 
          t <= 0;
          u <= 0;
          v <= 0;
          triID <= 0;
       end
       else
       begin
          state <= next_state ; 
          if (latchnear == 1'b1)
          begin
             t <= tin ; 
             u <= uin ; 
             v <= vin ; 
             triID <= triIDin ; 
          end 
       end 
    end 

    always @(state or tin or t or enable or hit or reset)
    begin
//       latchnear = 1'b0 ; 
       case (state)
          0 :

                   begin
                      if ((enable == 1'b1) & (hit == 1'b1))
                      begin
                         next_state = 2 ; 
                         latchnear = 1'b1 ; 
                      end
                      else if ((enablenear == 1'b1) & (hit == 1'b1))
                       begin
                         latchnear = 1'b1 ; 
                         next_state = 1 ; 
                      end
                      else
                      begin
                         next_state = 0 ; 
                      end 
				    temp_anyhit = 1'b0;
                   end

          1 :
                   begin
                      if (reset == 1'b1)
                      begin
                         if ((enable == 1'b1) & (hit == 1'b1))
                         begin
                            latchnear = 1'b1 ; 
                            next_state = 2 ; 
                         end
                         else if ((enablenear == 1'b1) & (hit == 1'b1))
                         begin
                            latchnear = 1'b1 ; 

                            next_state = 1 ; 
                         end
                         else
                         begin
                            next_state = 0 ; 
                         end 
                      end
                      else if ((enable == 1'b1) & (hit == 1'b1))
                      begin
                         if (tin < t)
                         begin
                            latchnear = 1'b1 ; 

                         end 
                         next_state = 2 ; 
                      end
                      else if ((enablenear == 1'b1) & (hit == 1'b1) & (tin < t))
                      begin
                         latchnear = 1'b1 ; 
                         next_state = 1 ; 
                      end
                      else
                      begin
                         next_state = 1 ; 
                      end 
				    temp_anyhit = 1'b0;

                   end
          2 :
                   begin
                      if (reset == 1'b1)
                      begin
                         if ((enable == 1'b1) & (hit == 1'b1))
                         begin
                            latchnear = 1'b1 ; 
                            next_state = 2 ; 
                         end
                         else if ((enablenear == 1'b1) & (hit == 1'b1))
                         begin

                            latchnear = 1'b1 ; 
                            next_state = 1 ; 
                         end
                         else
                         begin
                            next_state = 0 ; 
                         end 
                      end
                      else
                      begin
                         if ((enable == 1'b1) & (hit == 1'b1))
                         begin

                            if (tin < t)
                            begin
                               latchnear = 1'b1 ; 
                            end 
                         end
                         else if ((enablenear == 1'b1) & (hit == 1'b1))
                         begin
                            if (tin < t)
                            begin
                               latchnear = 1'b1 ; 
                            end 
						end
                         else if ((enablenear == 1'b1) & (hit == 1'b1))
                         begin
                            if (tin < t)
                            begin
                               latchnear = 1'b1 ; 
                            end 

                         end 
                         next_state = 2 ; 
                      end 
				    temp_anyhit = 1'b1;
                   end
       endcase 
    end 
 endmodule



