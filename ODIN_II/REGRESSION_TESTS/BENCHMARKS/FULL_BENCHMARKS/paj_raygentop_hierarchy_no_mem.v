 module paj_raygentop_hierarchy_no_mem (rgwant_addr, rgwant_data, rgread_ready, rgaddr_ready, rgdata_ready, rgwant_read, rgdatain, rgdataout, rgaddrin, rgCont, rgStat, rgCfgData, rgwant_CfgData, rgCfgData_ready, tm3_sram_data_in, tm3_sram_data_out, tm3_sram_addr, tm3_sram_we, tm3_sram_oe, tm3_sram_adsp, tm3_clk_v0, fbdata, fbdatavalid, fbnextscanline, raygroup01, raygroupvalid01, busy01, raygroup10, raygroupvalid10, busy10, globalreset, rgData, rgAddr, rgWE, rgAddrValid, rgDone, rgResultData, rgResultReady, rgResultSource);

    output rgwant_addr; 
    wire rgwant_addr;
    output rgwant_data; 
    wire rgwant_data;
    output rgread_ready; 
    wire rgread_ready;
    input rgaddr_ready; 
    input rgdata_ready; 

    input rgwant_read; 
    input[63:0] rgdatain; 
    output[63:0] rgdataout; 
    wire[63:0] rgdataout;
    input[17:0] rgaddrin; 
    input[31:0] rgCont; 
    output[31:0] rgStat; 
    wire[31:0] rgStat;
    input[31:0] rgCfgData; 
    output rgwant_CfgData; 
    wire rgwant_CfgData;
    input rgCfgData_ready; 

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
    input tm3_clk_v0; 

    output[63:0] fbdata; 
    wire[63:0] fbdata;
    output fbdatavalid; 
    wire fbdatavalid;
    input fbnextscanline; 
    output[1:0] raygroup01; 
    wire[1:0] raygroup01;
    output raygroupvalid01; 
    wire raygroupvalid01;
    input busy01; 
    output[1:0] raygroup10; 
    wire[1:0] raygroup10;

    output raygroupvalid10; 
    wire raygroupvalid10;
    input busy10; 
    input globalreset; 
    output[31:0] rgData; 
    wire[31:0] rgData;
    output[3:0] rgAddr; 
    wire[3:0] rgAddr;
    output[2:0] rgWE; 
    wire[2:0] rgWE;
    output rgAddrValid; 
    wire rgAddrValid;

    input rgDone; 
    input[31:0] rgResultData; 
    input rgResultReady; 
    input[1:0] rgResultSource; 

    wire[2:0] statepeek2; 
    wire as01; 
    wire ack01; 

    wire[3:0] addr01; 
    wire[47:0] dir01; 
    wire[47:0] dir; 
    wire[47:0] sramdatal; 
    wire wantDir; 
    wire dirReady; 
    wire dirReadyl; 
    wire[14:0] address; 
    wire[30:0] cyclecounter; 

    wire nas01; 
    wire nas10; 
    wire go; 
    reg page; 
    wire[2:0] statepeekct; 
    // result Signals
    wire valid01; 
    wire valid10; 
    wire[15:0] id01a; 
    wire[15:0] id01b; 
    wire[15:0] id01c; 
    wire[15:0] id10a; 

    wire[15:0] id10b; 
    wire[15:0] id10c; 
    wire hit01a; 
    wire hit01b; 
    wire hit01c; 
    wire hit10a; 
    wire hit10b; 
    wire hit10c; 
    wire[7:0] u01a; 
    wire[7:0] u01b; 
    wire[7:0] u01c; 
    wire[7:0] v01a; 

    wire[7:0] v01b; 
    wire[7:0] v01c; 
    wire[7:0] u10a; 
    wire[7:0] u10b; 
    wire[7:0] u10c; 
    wire[7:0] v10a; 
    wire[7:0] v10b; 
    wire[7:0] v10c; 
    wire wantwriteback; 
    wire writebackack; 
    wire[63:0] writebackdata; 
    wire[17:0] writebackaddr; 

    wire[17:0] nextaddr01; 
    // Shading Signals
    wire[63:0] shadedata; 
    wire[15:0] triID; 
    wire wantshadedata; 
    wire shadedataready; 
    // CfgData Signals
    wire[27:0] origx; 
    wire[27:0] origy; 
    wire[27:0] origz; 
    wire[15:0] m11; 
    wire[15:0] m12; 

    wire[15:0] m13; 
    wire[15:0] m21; 
    wire[15:0] m22; 
    wire[15:0] m23; 
    wire[15:0] m31; 
    wire[15:0] m32; 
    wire[15:0] m33; 
    wire[20:0] bkcolour; 
    // Texture signals
    wire[20:0] texinfo; 
    wire[3:0] texaddr; 
    wire[63:0] texel; 

    wire[17:0] texeladdr; 
    wire wanttexel; 
    wire texelready; 
    // Frame Buffer Read Signals
    wire fbpage; 
    // debug signals
    wire wantcfg; 
    wire debugglobalreset; 

    assign rgwant_CfgData = wantcfg ;

    onlyonecycle onlyeonecycleinst (rgCont[0], go, globalreset, tm3_clk_v0); 

    always @(posedge tm3_clk_v0 or posedge globalreset)
    begin
       if (globalreset == 1'b1)
       begin
          page <= 1'b1 ; // Reset to 1 such that first flip sets to 0
       end
       else

       begin
          page <= ~page ; 
       end 
    end 
    assign fbpage = ~page ;

    matmult matmultinst(sramdatal[47:32], sramdatal[31:16], sramdatal[15:0], m11, m12, m13, m21, m22, m23, m31, m32, m33, dir[47:32], dir[31:16], dir[15:0], tm3_clk_v0); 

    delay1x3 dir01delay(dirReady, dirReadyl, tm3_clk_v0); 
    rgconfigmemory ConfigMemoryInst (rgCfgData[31:28], rgCfgData[27:0], rgCfgData_ready, wantcfg, origx, origy, origz, m11, m12, m13, m21, m22, m23, m31, m32, m33, bkcolour, texinfo, globalreset, tm3_clk_v0); 

    rgsramcontroller sramcont (rgwant_addr, rgaddr_ready, rgaddrin, rgwant_data, rgdata_ready, rgdatain, rgwant_read, rgread_ready, rgdataout, dirReady, wantDir, sramdatal, address, wantwriteback, writebackack, writebackdata, writebackaddr, fbdata, fbnextscanline, fbdatavalid, fbpage, shadedata, triID, wantshadedata, shadedataready, texeladdr, texel, wanttexel, texelready, tm3_sram_data_in, tm3_sram_data_out, tm3_sram_addr, tm3_sram_we, tm3_sram_oe, tm3_sram_adsp, globalreset, tm3_clk_v0);
    raysend raysendinst (as01, ack01, addr01, dir01, origx, origy, origz, rgData, rgAddr, rgWE, rgAddrValid, rgDone, globalreset, tm3_clk_v0, statepeek2); 

    raygencont  raygencontinst(go, rgCont[15:1], rgStat[31], cyclecounter, nextaddr01, nas01, nas10, page, dirReadyl, wantDir, dir, address, as01, addr01, ack01, dir01, raygroup01, raygroupvalid01, busy01, raygroup10, raygroupvalid10, busy10, globalreset, tm3_clk_v0, statepeekct); 
    resultrecieve resultrecieveinst (valid01, valid10, id01a, id01b, id01c, id10a, id10b, id10c, hit01a, hit01b, hit01c, hit10a, hit10b, hit10c, u01a, u01b, u01c, v01a, v01b, v01c, u10a, u10b, u10c, v10a, v10b, v10c, rgResultData, rgResultReady, rgResultSource, globalreset, tm3_clk_v0); 
    assign debugglobalreset = globalreset | go ;
    resultwriter resultwriteinst (valid01, valid10, id01a, id01b, id01c, id10a, id10b, id10c, hit01a, hit01b, hit01c, hit10a, hit10b, hit10c, u01a, u01b, u01c, v01a, v01b, v01c, u10a, u10b, u10c, v10a, v10b, v10c, nextaddr01, nas01, nas10, bkcolour, shadedata, triID, wantshadedata, shadedataready, texinfo, texaddr, texeladdr, texel, wanttexel, texelready, writebackdata, writebackaddr, wantwriteback, writebackack, debugglobalreset, tm3_clk_v0);
    assign rgStat[30:0] = cyclecounter ;
 endmodule


module delay1x3 (datain, dataout, clk);

    input datain; 
    output dataout; 
    wire dataout;
    input clk; 

    reg buff0; 
    reg buff1; 
    reg buff2; 

    assign dataout = buff2 ;

    always @(posedge clk)
    begin
/* PAJ expanded for loop to hard definition the size of `depth */
       buff0 <= datain ; 
		buff1 <= buff0;
		buff2 <= buff1;
    end 
 endmodule


    

    
    
 // A debugging circuit that allows a single cycle pulse to be 
 // generated by through the ports package
 module onlyonecycle (trigger, output_xhdl0, globalreset, clk);

    input trigger; 
    output output_xhdl0; 
    reg output_xhdl0;
    input globalreset; 
    input clk; 

    reg[1:0] state; 
    reg[1:0] next_state; 
    reg count; 
    reg temp_count; 

    always @(posedge clk or posedge globalreset)
    begin
       if (globalreset == 1'b1)
       begin
          state <= 0 ; 
          count <= 0 ; 

       end
       else
       begin
          state <= next_state ; 
		count <= temp_count;
       end 
    end 

    always @(state or trigger or count)
    begin
       case (state)
          0 :
                   begin
       				  output_xhdl0 = 1'b0 ; 
                      if (trigger == 1'b1)
                      begin
                         next_state = 1 ; 
                      end
                      else
                      begin
                         next_state = 0 ; 
                      end 
                         temp_count = 1 - 1 ; 
                   end
          1 :
                   begin
                      output_xhdl0 = 1'b1 ; 
                      if (count == 0)
                      begin
                         next_state = 2 ; 
                      end
                      else

                      begin

                         next_state = 1 ; 
                      end 
                         temp_count = count - 1 ; 
                   end
          2 :
                   begin
       				  output_xhdl0 = 1'b0 ; 
                      if (trigger == 1'b0)
                      begin
                         next_state = 0 ; 
                      end
                      else
                      begin
                         next_state = 2 ; 

                      end 
                   end
       endcase 
    end 
 endmodule

module matmult (Ax, Ay, Az, m11, m12, m13, m21, m22, m23, m31, m32, m33, Cx, Cy, Cz, clk);

    input[16 - 1:0] Ax; 
    input[16 - 1:0] Ay; 
    input[16 - 1:0] Az; 
    input[16 - 1:0] m11; 
    input[16 - 1:0] m12; 

    input[16 - 1:0] m13; 
    input[16 - 1:0] m21; 
    input[16 - 1:0] m22; 
    input[16 - 1:0] m23; 
    input[16 - 1:0] m31; 
    input[16 - 1:0] m32; 
    input[16 - 1:0] m33; 
    output[16 - 1:0] Cx; 
    reg[16 - 1:0] Cx;
    output[16 - 1:0] Cy; 
    reg[16 - 1:0] Cy;
    output[16 - 1:0] Cz; 

    reg[16 - 1:0] Cz;
    input clk; 

    reg[16 + 16 - 1:0] am11; 
    reg[16 + 16 - 1:0] am12; 
    reg[16 + 16 - 1:0] am13; 
    reg[16 + 16 - 1:0] am21; 
    reg[16 + 16 - 1:0] am22; 
    reg[16 + 16 - 1:0] am23; 
    reg[16 + 16 - 1:0] am31; 
    reg[16 + 16 - 1:0] am32; 
    reg[16 + 16 - 1:0] am33; 


    always @(posedge clk)
    begin
       am11 <= Ax * m11 ; 
       am12 <= Ay * m12 ; 
       am13 <= Az * m13 ; 
       am21 <= Ax * m21 ; 
       am22 <= Ay * m22 ; 
       am23 <= Az * m23 ; 
       am31 <= Ax * m31 ; 
       am32 <= Ay * m32 ; 
       am33 <= Az * m33 ; 

       //      Cx <= (am11 + am12 + am13) (`widthA+`widthB-2 downto `widthB-1);
       //      Cy <= (am21 + am22 + am23) (`widthA+`widthB-2 downto `widthB-1);
       //      Cz <= (am31 + am32 + am33) (`widthA+`widthB-2 downto `widthB-1);
       Cx <= (am11[16+16-2:16-1] + am12[16+16-2:16-1] + am13[16+16-2:16-1]) ; 
       Cy <= (am21[16+16-2:16-1] + am22[16+16-2:16-1] + am23[16+16-2:16-1]); 
       Cz <= (am31[16+16-2:16-1] + am32[16+16-2:16-1] + am33[16+16-2:16-1]) ;  
    end 
 endmodule

    
    

module rgconfigmemory (CfgAddr, CfgData, CfgData_Ready, want_CfgData, origx, origy, origz, m11, m12, m13, m21, m22, m23, m31, m32, m33, bkcolour, texinfo, globalreset, clk);


    input[3:0] CfgAddr; 
    input[27:0] CfgData; 
    input CfgData_Ready; 
    output want_CfgData; 
    reg want_CfgData;
    output[27:0] origx; 
    reg[27:0] origx;
    output[27:0] origy; 
    reg[27:0] origy;
    output[27:0] origz; 
    reg[27:0] origz;
    output[15:0] m11; 
    reg[15:0] m11;
    output[15:0] m12; 
    reg[15:0] m12;
    output[15:0] m13; 
    reg[15:0] m13;
    output[15:0] m21; 
    reg[15:0] m21;
    output[15:0] m22; 
    reg[15:0] m22;
    output[15:0] m23; 
    reg[15:0] m23;
    output[15:0] m31; 
    reg[15:0] m31;
    output[15:0] m32; 
    reg[15:0] m32;
    output[15:0] m33; 
    reg[15:0] m33;
    output[20:0] bkcolour; 
    reg[20:0] bkcolour;
    output[20:0] texinfo; 

    wire[20:0] texinfo;
    input globalreset; 
    input clk; 

    reg state; 
    reg next_state; 
    wire we; 

    reg[27:0] temp_origx;
    reg[27:0] temp_origy;
    reg[27:0] temp_origz;
    reg[15:0] temp_m11;
    reg[15:0] temp_m12;
    reg[15:0] temp_m13;
    reg[15:0] temp_m21;
    reg[15:0] temp_m22;
    reg[15:0] temp_m23;
    reg[15:0] temp_m31;
    reg[15:0] temp_m32;
    reg[15:0] temp_m33;
    reg[20:0] temp_bkcolour;

    // <<X-HDL>> Can't find translated component 'spram'. Module name may not match
    spram21x4 spraminst(we, texinfo, CfgData[20:0], clk); 
    assign we = ((CfgData_Ready == 1'b1) & (CfgAddr == 4'b1110)) ? 1'b1 : 1'b0 ;

    always @(posedge clk or posedge globalreset)
    begin
       if (globalreset == 1'b1)
       begin
          state <= 0 ; 
          origx <= 0;
          origy <= 0;

          origz <= 0;
          m11 <= 1;
          m12 <= 0;
          m13 <= 0;
          m21 <= 0;
          m22 <= 1;
          m23 <= 0;
          m31 <= 0;
          m32 <= 0;
         m33 <= 1;
          bkcolour <= 0;
       end
       else
       begin
          state <= next_state ; 
          origx <= temp_origx;
          origy <= temp_origy;
          origz <= temp_origz;
          m11 <= temp_m11;
          m12 <= temp_m12;
          m13 <= temp_m13;
          m21 <= temp_m21;
          m22 <= temp_m22;
          m23 <= temp_m23;
          m31 <= temp_m31;
          m32 <= temp_m32;
         m33 <= temp_m33;
          bkcolour <= bkcolour;
       end 
    end 

    always @(state or CfgData_Ready)
    begin
       case (state)
          0 :
                   begin
                      want_CfgData = 1'b1 ; 
                      if (CfgData_Ready == 1'b1)
                      begin
                         next_state = 1 ; 
                      end

                      else
                      begin
                         next_state = 0 ; 
                      end 

              if ((CfgData_Ready == 1'b1) && (CfgAddr == 4'b0001))
                        begin
											temp_origx = CfgData ; 
						end
                        else if ((CfgData_Ready == 1'b1) && (CfgAddr == 4'b0010))
                        begin
                                           temp_origy = CfgData ; 
						end
                        else if ((CfgData_Ready == 1'b1) && (CfgAddr == 4'b0011))
                        begin
                                           temp_origz = CfgData ; 
						end
                        else if ((CfgData_Ready == 1'b1) && (CfgAddr == 4'b0100))
                        begin
                                           temp_m11 = CfgData[15:0] ; 
						end
                        else if ((CfgData_Ready == 1'b1) && (CfgAddr == 4'b0101))
                        begin
                                           temp_m12 = CfgData[15:0] ; 
						end
                        else if ((CfgData_Ready == 1'b1) && (CfgAddr == 4'b0110))
                        begin
                                           temp_m13 = CfgData[15:0] ; 
						end
                        else if ((CfgData_Ready == 1'b1) && (CfgAddr == 4'b0111))
                        begin
                                           temp_m21 = CfgData[15:0] ; 
						end
                        else if ((CfgData_Ready == 1'b1) && (CfgAddr == 4'b1000))
                        begin
                                           temp_m22 = CfgData[15:0] ; 
						end
                        else if ((CfgData_Ready == 1'b1) && (CfgAddr == 4'b1001))
                        begin
                                           temp_m23 = CfgData[15:0] ; 
						end
                        else if ((CfgData_Ready == 1'b1) && (CfgAddr == 4'b1010))
                        begin
                                           temp_m31 = CfgData[15:0] ; 
						end
                        else if ((CfgData_Ready == 1'b1) && (CfgAddr == 4'b1011))
                        begin
                                           temp_m32 = CfgData[15:0] ; 
						end
                        else if ((CfgData_Ready == 1'b1) && (CfgAddr == 4'b1100))
                        begin
                                           temp_m33 = CfgData[15:0] ; 
						end
                        else if ((CfgData_Ready == 1'b1) && (CfgAddr == 4'b1101))
                        begin
                                           temp_bkcolour = CfgData[20:0] ; 
						end
                   end
          1 :
                   begin
                      want_CfgData = 1'b0 ; 
                      if (CfgData_Ready == 1'b0)
                      begin
                         next_state = 0 ; 
                      end

                      else
                      begin
                         next_state = 1 ; 
                      end 
                   end
       endcase 
    end 
 endmodule

    
    
 module spram21x4 (we, dataout, datain, clk);

    input we; 
    output[21 - 1:0] dataout; 
    wire[21 - 1:0] dataout;
    input[21 - 1:0] datain; 
    input clk; 

    reg[21 - 1:0] mem1; 
    reg[21 - 1:0] mem2; 

    assign dataout = mem2 ;

    always @(posedge clk or posedge we)
    begin
       if (we == 1'b1)
       begin
          mem1 <= datain; 
          mem2 <= mem1;
       end  
       else
       begin
          mem1 <= mem1; 
          mem2 <= mem2;
       end  
    end 
 endmodule
    
    
    
    
    
    
    
    
    
    
    

module rgsramcontroller (want_addr, addr_ready, addrin, want_data, data_ready, datain, want_read, read_ready, dataout, dirReady, wantDir, sramdatal, addr, wantwriteback, writebackack, writebackdata, writebackaddr, fbdata, fbnextscanline, fbdatavalid, fbpage, shadedata, triID, wantshadedata, shadedataready, texeladdr, texel, wanttexel, texelready, tm3_sram_data_in, tm3_sram_data_out, tm3_sram_addr, tm3_sram_we, tm3_sram_oe, tm3_sram_adsp, globalreset, clk);

    output want_addr; 
    reg want_addr;
    input addr_ready; 
    input[17:0] addrin; 
    output want_data; 
    reg want_data;
    input data_ready; 
    input[63:0] datain; 
    input want_read; 
    output read_ready; 

    reg read_ready;
    output[63:0] dataout; 
    wire[63:0] dataout;
    output dirReady; 
    reg dirReady;
    input wantDir; 
    output[47:0] sramdatal; 
    reg[47:0] sramdatal;
    output[14:0] addr; 
    wire[14:0] addr;
    input wantwriteback; 
    output writebackack; 

    reg writebackack;
    input[63:0] writebackdata; 
    input[17:0] writebackaddr; 
    output[63:0] fbdata; 
    reg[63:0] fbdata;
    input fbnextscanline; 
    output fbdatavalid; 
    reg fbdatavalid;
    input fbpage; 
    output[63:0] shadedata; 
    wire[63:0] shadedata;
    input[15:0] triID; 

    input wantshadedata; 
    output shadedataready; 
    reg shadedataready;
    input[17:0] texeladdr; 
    output[63:0] texel; 
    wire[63:0] texel;
    input wanttexel; 
    output texelready; 
    reg texelready;
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

    reg[3:0] state; 
    reg[3:0] next_state; 
    reg[17:0] waddress; 
    reg[14:0] faddress; 
    reg[6:0] fcount; 
    reg fbdatavalidl; 

    reg[17:0] temp_waddress; 
    reg[14:0] temp_faddress; 
    reg[6:0] temp_fcount; 
    reg temp_fbdatavalidl; 
    reg temp_texelready;
    reg temp_shadedataready;

    assign tm3_sram_data_out = tm3_sram_data_xhdl0;

    assign dataout = tm3_sram_data_in ;
    assign addr = tm3_sram_data_in[62:48] ;
    assign shadedata = tm3_sram_data_in ;
    assign texel = tm3_sram_data_in ;

    always @(posedge clk or posedge globalreset)
    begin
       if (globalreset == 1'b1)
       begin

          state <= 0 ; 
          waddress <= 0;
          faddress <= 0;
          fcount <= 7'b1101011 ; 
          fbdatavalid <= 1'b0 ; 
          fbdatavalidl <= 1'b0 ; 
          shadedataready <= 1'b0 ; 
          texelready <= 1'b0 ; 
          sramdatal <= 0;
          fbdata <= 0;
       end
       else

       begin
          state <= next_state ; 
          sramdatal <= tm3_sram_data_in[47:0] ; 
          fbdata <= tm3_sram_data_in ; 
          fbdatavalid <= fbdatavalidl ; 

fbdatavalidl <= temp_fbdatavalidl;
texelready <= temp_texelready;
shadedataready <= temp_shadedataready;
fcount <= temp_fcount;
faddress <= temp_faddress;
waddress <= temp_waddress;

       end 
    end 

    always @(state or addr_ready or data_ready or waddress or datain or wantDir or 
             want_read or wantwriteback or writebackdata or writebackaddr or 
             fcount or fbpage or faddress or fbnextscanline or triID or wantshadedata or 
             wanttexel or texeladdr)

    begin
       case (state)

          0 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b01 ; 
				       tm3_sram_adsp = 1'b0 ; 
				       tm3_sram_data_xhdl0 = 0;
				       tm3_sram_addr = {1'b0, waddress} ; 
				       want_addr = 1'b1 ; 
				       want_data = 1'b1 ; 
				       read_ready = 1'b1 ; 
				       dirReady = 1'b0 ; 
				       writebackack = 1'b0 ; 
                      if (addr_ready == 1'b1)
                      begin
                         next_state = 1 ; 
                      end
                      else if (want_read == 1'b1)
                      begin
                         next_state = 2 ; 
                      end
                      else if (data_ready == 1'b1)
                      begin

                         next_state = 3 ; 
                      end
                      else if (wantDir == 1'b1)
                      begin
                         next_state = 5 ; 
                      end
                      else if (wantwriteback == 1'b1)
                      begin
                         next_state = 6 ; 
                      end
                      else if (wantshadedata == 1'b1)
                      begin

                         next_state = 9 ; 
                      end
                      else if (wanttexel == 1'b1)
                      begin
                         next_state = 10 ; 
                      end
                      else if (fcount != 0)
                      begin
                         next_state = 7 ; 
                      end
                      else if (fbnextscanline == 1'b1)
                      begin

                         next_state = 8 ; 
                      end
                      else
                      begin
                         next_state = 0 ; 
                      end 
				          temp_fbdatavalidl = 1'b0 ; 
				          temp_shadedataready = 1'b0 ; 
				          temp_texelready = 1'b0 ; 
                         if (addr_ready == 1'b1)

                         begin
                            temp_waddress = addrin ; 
                         end 

                   end
          1 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b01 ; 
				       tm3_sram_adsp = 1'b0 ; 
				       tm3_sram_data_xhdl0 = 0;
				       tm3_sram_addr = {1'b0, waddress} ; 
				       want_data = 1'b1 ; 
				       read_ready = 1'b1 ; 
				       dirReady = 1'b0 ; 
				       writebackack = 1'b0 ; 
                      want_addr = 1'b0 ; 
                      if (addr_ready == 1'b0)
                      begin
                         next_state = 0 ; 

                      end
                      else
                      begin
                         next_state = 1 ; 
                      end 
                   end
          2 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b01 ; 
				       tm3_sram_adsp = 1'b0 ; 
				       tm3_sram_data_xhdl0 = 0;
				       tm3_sram_addr = {1'b0, waddress} ; 
				       want_addr = 1'b1 ; 
				       want_data = 1'b1 ; 
				       dirReady = 1'b0 ; 
				       writebackack = 1'b0 ; 

                      read_ready = 1'b0 ; 
                      if (want_read == 1'b0)
                      begin
                         next_state = 0 ; 
                      end
                      else
                      begin
                         next_state = 2 ; 
                      end 

				          temp_fbdatavalidl = 1'b0 ; 
				          temp_shadedataready = 1'b0 ; 
				          temp_texelready = 1'b0 ; 
                         if (want_read == 1'b0)
                         begin

                            temp_waddress = waddress + 1 ; 
                         end 

                   end
          3 :
                   begin
				       tm3_sram_addr = {1'b0, waddress} ; 
				       want_addr = 1'b1 ; 
				       read_ready = 1'b1 ; 
				       dirReady = 1'b0 ; 
				       writebackack = 1'b0 ; 
                      tm3_sram_data_xhdl0 = datain ; 
                      tm3_sram_we = 8'b00000000 ; 


                   tm3_sram_oe = 2'b11 ; 
                      tm3_sram_adsp = 1'b0 ; 
                      want_data = 1'b0 ; 
                      next_state = 4 ; 

				          temp_fbdatavalidl = 1'b0 ; 
				          temp_shadedataready = 1'b0 ; 
				          temp_texelready = 1'b0 ; 
                         temp_waddress = waddress + 1 ; 

                   end
          4 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b01 ; 
				       tm3_sram_adsp = 1'b0 ; 
				       tm3_sram_data_xhdl0 = 0;
				       tm3_sram_addr = {1'b0, waddress} ; 
				       want_addr = 1'b1 ; 
				       read_ready = 1'b1 ; 
				       dirReady = 1'b0 ; 
				       writebackack = 1'b0 ; 
                      if (data_ready == 1'b0)
                      begin

                         next_state = 0 ; 
                      end
                      else
                      begin
                         next_state = 4 ; 
                      end 
                      want_data = 1'b0 ; 
                   end

          5 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b01 ; 
				       tm3_sram_adsp = 1'b0 ; 
				       tm3_sram_data_xhdl0 = 0;
				       tm3_sram_addr = {1'b0, waddress} ; 
				       want_addr = 1'b1 ; 
				       want_data = 1'b1 ; 
				       read_ready = 1'b1 ; 
				       writebackack = 1'b0 ; 

                     dirReady = 1'b1 ; 
                      if (wantDir == 1'b0)
                      begin
                         next_state = 0 ; 

                      end
                      else
                      begin
                         next_state = 5 ; 
                      end 

				          temp_fbdatavalidl = 1'b0 ; 
				          temp_shadedataready = 1'b0 ; 
				          temp_texelready = 1'b0 ; 
                         if (wantDir == 1'b0)
                         begin
                            temp_waddress = waddress + 1 ; 
                         end 

                   end
          6 :
                   begin
				       want_addr = 1'b1 ; 
				       want_data = 1'b1 ; 
				       read_ready = 1'b1 ; 
				       dirReady = 1'b0 ; 

                      tm3_sram_data_xhdl0 = writebackdata ; 
                      tm3_sram_we = 8'b00000000 ; 
                      tm3_sram_oe = 2'b11 ; 
                      tm3_sram_adsp = 1'b0 ; 
                      tm3_sram_addr = {1'b0, writebackaddr} ; 
                      writebackack = 1'b1 ; 
                      next_state = 0 ; 
                   end

          7 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b01 ; 
				       tm3_sram_adsp = 1'b0 ; 
				       tm3_sram_data_xhdl0 = 0;
				       want_addr = 1'b1 ; 
				       want_data = 1'b1 ; 
				       read_ready = 1'b1 ; 
				       dirReady = 1'b0 ; 
				       writebackack = 1'b0 ; 
                      tm3_sram_addr = {3'b011, fbpage, faddress} ; 
                      if ((fcount == 1) | (addr_ready == 1'b1) | (want_read == 1'b1) | (data_ready == 1'b1) | (wantDir == 1'b1) | (wantwriteback == 1'b1))
                      begin
                         next_state = 0 ; 

                      end
                      else
                      begin
                         next_state = 7 ; 
                      end 


				          temp_shadedataready = 1'b0 ; 
				          temp_texelready = 1'b0 ; 
                         temp_fbdatavalidl = 1'b1 ; 
                         if (fcount != 0)
                         begin
                            temp_faddress = faddress + 1 ; 
                            temp_fcount = fcount - 1 ; 
                         end 

                   end
          8 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b01 ; 
				       tm3_sram_adsp = 1'b0 ; 
				       tm3_sram_data_xhdl0 = 0;
				       tm3_sram_addr = {1'b0, waddress} ; 
				       want_addr = 1'b1 ; 
				       want_data = 1'b1 ; 
				       read_ready = 1'b1 ; 
				       dirReady = 1'b0 ; 
				       writebackack = 1'b0 ; 
                      next_state = 7 ; 

				   				          temp_fbdatavalidl = 1'b0 ; 
				          temp_shadedataready = 1'b0 ; 
				          temp_texelready = 1'b0 ; 
                         temp_fcount = 7'b1101011 ; 
                         if (faddress == 25680)
                         begin
                            temp_faddress = 0;
                         end 
                   end
          9 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b01 ; 
				       tm3_sram_adsp = 1'b0 ; 
				       tm3_sram_data_xhdl0 = 0;
				       want_addr = 1'b1 ; 
				       want_data = 1'b1 ; 
				       read_ready = 1'b1 ; 
				       dirReady = 1'b0 ; 
				       writebackack = 1'b0 ; 
                      tm3_sram_addr = {3'b010, triID} ; 
                      next_state = 0 ; 

				          temp_fbdatavalidl = 1'b0 ; 
				          temp_texelready = 1'b0 ; 
                         temp_shadedataready = 1'b1 ; 
                   end

          10 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b01 ; 
				       tm3_sram_adsp = 1'b0 ; 
				       tm3_sram_data_xhdl0 = 0;
				       want_addr = 1'b1 ; 
				       want_data = 1'b1 ; 
				       read_ready = 1'b1 ; 
				       dirReady = 1'b0 ; 
				       writebackack = 1'b0 ; 
                      tm3_sram_addr = {1'b0, texeladdr} ; 
                      next_state = 0 ; 

				          temp_fbdatavalidl = 1'b0 ; 
				          temp_shadedataready = 1'b0 ; 
                         temp_texelready = 1'b1 ; 
                   end
       endcase 
    end 
 endmodule

    
    
    
    
    
    
    
    
    
    

 module raysend (as, ack, addr, dir, origx, origy, origz, rgData, rgAddr, rgWE, rgAddrValid, rgDone, globalreset, clk, statepeek);

    input as; 
    output ack; 
    reg ack;
    input[3:0] addr; 
    input[47:0] dir; 
    input[27:0] origx; 
    input[27:0] origy; 
    input[27:0] origz; 
    output[31:0] rgData; 
    reg[31:0] rgData;

    output[3:0] rgAddr; 
    reg[3:0] rgAddr;
    output[2:0] rgWE; 
    reg[2:0] rgWE;
    output rgAddrValid; 
    reg rgAddrValid;
    input rgDone; 
    input globalreset; 
    input clk; 
    output[2:0] statepeek; 
    reg[2:0] statepeek;

    reg[3:0] state; 
    reg[3:0] next_state; 



    reg[31:0] temp_rgData;
    reg[2:0] temp_rgWE; 
    reg temp_rgAddrValid;
    reg temp_ack;
    reg[3:0] temp_rgAddr; 

    always @(posedge clk or posedge globalreset)
    begin
       if (globalreset == 1'b1)
       begin
          state <= 0 ; 
          ack <= 1'b0 ; 
          rgWE <= 3'b000 ; 
          rgData <= 0;
          rgAddrValid <= 1'b0 ; 
          rgAddr <= 0;
       end
       else
       begin
          state <= next_state ; 

rgData <= temp_rgData;
rgWE <= temp_rgWE;
rgAddrValid <= temp_rgAddrValid;
ack <= temp_ack;
rgAddr <= temp_rgAddr;

       end 
    end 

    always @(state or ack or as or rgDone)
    begin

       case (state)
          0 :
                   begin
                      if ((as == 1'b1) & (ack == 1'b0))
                      begin
                         next_state = 1 ; 
                      end
                      else
                      begin
                         next_state = 0 ; 
                      end 
                      statepeek = 3'b001 ; 

                         if ((as == 1'b1) & (ack == 1'b0))
                         begin
                            temp_rgData = {4'b0000, origx} ; 
                            temp_rgWE = 3'b001 ; 
                            temp_rgAddrValid = 1'b1 ; 
                            temp_rgAddr = addr ; 
                         end 
                         if (as == 1'b0 & ack == 1'b1)
                         begin
                            temp_ack = 1'b0 ; 
                         end 

                   end
          1 :
                   begin
                      if (rgDone == 1'b1)
                      begin
                         next_state = 6 ; 
                      end
                      else
                      begin
                         next_state = 1 ; 
                      end 
                      statepeek = 3'b010 ; 

                         if (rgDone == 1'b1)
                         begin
                            temp_rgAddrValid = 1'b0 ; 
                         end 

                   end
          2 :
                   begin
                      if (rgDone == 1'b1)
                      begin
                         next_state = 7 ; 
                      end
                      else
                      begin
                         next_state = 2 ; 
                      end 
                      statepeek = 3'b011 ; 

                         if (rgDone == 1'b1)
                         begin
                            temp_rgAddrValid = 1'b0 ; 
                         end 

                   end
           3 :
                   begin
                      if (rgDone == 1'b1)
                      begin
                         next_state = 8 ; 
                      end
                      else
                      begin
                         next_state = 3 ; 
                      end 
                      statepeek = 3'b100 ; 

                         if (rgDone == 1'b1)
                         begin
                            temp_rgAddrValid = 1'b0 ; 
                         end 

                   end
         4 :
                   begin
                      if (rgDone == 1'b1)
                       begin
                         next_state = 9 ; 
                      end
                      else
                      begin
                         next_state = 4 ; 
                      end 
                      statepeek = 3'b101 ; 

                         if (rgDone == 1'b1)
                         begin
                            temp_rgAddrValid = 1'b0 ; 
                         end 
                   end

          5 :
                   begin
                      if (rgDone == 1'b1)
                      begin
                         next_state = 0 ; 
                      end
                      else
                      begin
                         next_state = 5 ; 
                      end 
                      statepeek = 3'b110 ; 

                         temp_ack = 1'b1 ; 
                         if (rgDone == 1'b1)
                         begin
                            temp_rgAddrValid = 1'b0 ; 
                         end 

                   end

          6 :
                   begin
                      next_state = 2 ; 

                         temp_rgData = {4'b0000, origy} ; 
                         temp_rgWE = 3'b010 ; 
                         temp_rgAddrValid = 1'b1 ; 

                   end
          7 :
                   begin
                      next_state = 3 ; 

                         temp_rgData = {4'b0000, origz} ; 
                         temp_rgWE = 3'b011 ; 
                         temp_rgAddrValid = 1'b1 ; 
                   end
          8 :
                   begin
                      next_state = 4 ; 

                         temp_rgData = {dir[31:16], dir[47:32]} ; 
                         temp_rgWE = 3'b100 ; 
                         temp_rgAddrValid = 1'b1 ; 
                   end
           9 :
                   begin
                      next_state = 5 ; 

                         temp_rgData = {16'b0000000000000000, dir[15:0]} ; 
                          temp_rgWE = 3'b101 ; 
                         temp_rgAddrValid = 1'b1 ; 
                   end
       endcase 
    end 
 endmodule

    
    
    
    
    

 module raygencont (go, initcount, busyout, cycles, nextaddr, nas0, nas1, page, dirReady, wantDir, dirIn, addrIn, as, addr, ack, dir, raygroup0, raygroupvalid0, busy0, raygroup1, raygroupvalid1, busy1, globalreset, clk, statepeek);

    input go; 
    input[14:0] initcount; 
    output busyout; 
    wire busyout;
    reg temp_busyout;
    output[30:0] cycles; 
    reg[30:0] cycles;
    output[17:0] nextaddr; 
    wire[17:0] nextaddr;
    output nas0; 

    wire nas0;
    reg temp_nas0;
    output nas1; 
    wire nas1;
    reg temp_nas1;
    input page; 
    input dirReady; 
    output wantDir; 
    reg wantDir;
    input[47:0] dirIn; 
    input[14:0] addrIn; 
    output as; 
    reg as;
    output[3:0] addr; 

    reg[3:0] addr;
    input ack; 
    output[47:0] dir; 
    reg[47:0] dir;
    output[1:0] raygroup0; 
    wire[1:0] raygroup0;
    output raygroupvalid0; 
    reg raygroupvalid0;
    input busy0; 
    output[1:0] raygroup1; 
    wire[1:0] raygroup1;
    output raygroupvalid1; 

    reg raygroupvalid1;
    input busy1; 
    input globalreset; 
    input clk; 
    output[2:0] statepeek; 
    reg[2:0] statepeek;


    reg[2:0] state; 
    reg[2:0] next_state; 
    reg[14:0] count; 
    reg first; 
    reg[17:0] destaddr; 
    wire[1:0] busy; 
    reg[1:0] loaded; 
    reg[1:0] groupID; 
    reg active; 

    reg[47:0] temp_dir;
    reg[30:0] temp_cycles;
    reg[1:0] temp_addr;
    reg[1:0] temp_loaded; 
    reg[1:0] temp_groupID; 
    reg[14:0] temp_count; 
    reg temp_active; 
    reg temp_raygroupvalid1;
    reg temp_raygroupvalid0;

    assign busy = {busy1, busy0} ;

    always @(posedge clk or posedge globalreset)
    begin

       if (globalreset == 1'b1)

       begin
          state <= 0 ; 
          cycles <= 0;
          dir <= 0;
          addr[1:0] <= 2'b00 ; 
          groupID <= 2'b00 ; 
          count <= 0;
          first <= 1'b0 ; 
          destaddr <= 0;
          raygroupvalid0 <= 1'b0 ; 
          raygroupvalid1 <= 1'b0 ; 
          loaded <= 2'b00 ; 

          active <= 1'b0 ; 
       end
       else
       begin
    	addr[3:2] <= (active == 1'b0) ? {1'b0, groupID[0]} : {1'b1, groupID[1]} ;
          state <= next_state ; 

dir <= temp_dir;
cycles <= temp_cycles;
addr <= temp_addr;
loaded <= temp_loaded;
groupID <= temp_groupID;
count <= temp_count;
active <= temp_active;
raygroupvalid0 <= temp_raygroupvalid0;
raygroupvalid1 <= temp_raygroupvalid1;

       end 
    end 

    assign raygroup0 = {1'b0, groupID[0]} ;
    assign raygroup1 = {1'b1, groupID[1]} ;
    assign nextaddr = {2'b11, page, addrIn} ;
    assign busyout = temp_busyout;
    assign nas0 = temp_nas0;
    assign nas1 = temp_nas1;

    always @(state or go or ack or busy or dirReady or addr or count or loaded)
    begin
       case (state)
          0 :
                   begin
       				as = 1'b0 ; 
       				wantDir = 1'b0 ; 
                      if (go == 1'b1)
                      begin
                         next_state = 1 ; 
                      end
                      else
                      begin
                         next_state = 0 ; 
                      end 
                      statepeek = 3'b001 ; 
						temp_busyout = 1'b0;
						temp_nas0 = 1'b0;
						temp_nas1 = 1'b0;


                         if (go == 1'b1)
                         begin
                            temp_cycles = 0;
                         end 
                         temp_addr[1:0] = 2'b00 ; 
                         temp_loaded = 2'b00 ; 
                         temp_groupID = 2'b00 ; 
                         temp_count = initcount ; 
                         temp_active = 1'b0 ; 

                   end
          1 :
                   begin
                      as = dirReady ; 
                      wantDir = 1'b1 ; 
                      if (dirReady == 1'b1)
                      begin
                         next_state = 2 ; 
                      end
                      else
                      begin
                         next_state = 1 ; 
                      end 
                     statepeek = 3'b010 ; 
						temp_busyout = 1'b1;
    				if (addr[1:0] == 2'b00 & dirReady == 1'b1 & active == 1'b0) 
					begin
						 temp_nas0 = 1'b1;
						 temp_nas1 = 1'b1;
					end

                         temp_dir = dirIn ; 
                         if (dirReady == 1'b1 & addr[1:0] == 2'b10)
                         begin
                            if (active == 1'b0)
                            begin
                               temp_loaded[0] = 1'b1 ; 
                            end
                            else
                            begin
                               temp_loaded[1] = 1'b1 ; 
                            end 
                         end 
             temp_cycles = cycles + 1 ; 


                   end
          2 :
                   begin
                      wantDir = 1'b0 ; 
                      as = 1'b1 ; 
                      if ((ack == 1'b1) & (addr[1:0] != 2'b10))
                      begin
                         next_state = 1 ; 
                      end
                      else if (ack == 1'b1)
                      begin
                         if ((loaded[0]) == 1'b1 & (busy[0]) == 1'b0)
                         begin
                            next_state = 3 ; 
                         end
                         else if ((loaded[1]) == 1'b1 & (busy[1]) == 1'b0)
                         begin
                            next_state = 4 ; 
                         end
                         else if (loaded != 2'b11)
                         begin

                            next_state = 1 ; 
                         end
                         else
                         begin
                            next_state = 2 ; 
                         end 
                      end
                      else
                      begin
                         next_state = 2 ; 
                      end 
                      statepeek = 3'b011 ; 
						temp_busyout = 1'b1;
						temp_nas0 = 1'b0;
						temp_nas1 = 1'b0;

                         if ((ack == 1'b1) & (addr[1:0] != 2'b10))
                         begin
                            temp_addr[1:0] = addr[1:0] + 2'b01 ; 
                         end 
                         else if ((ack == 1'b1) & addr[1:0] == 2'b10)
                         begin
                            if ((loaded[0]) == 1'b1 & (busy[0]) == 1'b0)
                            begin
                               temp_raygroupvalid0 = 1'b1 ; 
                            end
                            else if ((loaded[1]) == 1'b1 & (busy[1]) == 1'b0)
                            begin

                               temp_raygroupvalid1 = 1'b1 ; 
                            end
                            else if ((loaded[0]) == 1'b0)
                            begin
                               temp_active = 1'b0 ; 
                               temp_addr[1:0] = 2'b00 ; 
                            end
                            else if ((loaded[1]) == 1'b0)
                            begin
                               temp_active = 1'b1 ; 
                               temp_addr[1:0] = 2'b00 ; 
                            end 
                         end 

             temp_cycles = cycles + 1 ; 
                   end
          4 :
                   begin
                      if ((busy[1]) == 1'b0)
                      begin
                         next_state = 4 ; 
                      end
                      else if ((loaded[0]) == 1'b1 & (busy[0]) == 1'b0)
                      begin
                         next_state = 3 ; 
                      end
                      else if (count > 0)
                      begin

                         next_state = 1 ; 
                      end
                      else
                      begin
                         next_state = 0 ; 
                      end 
                      statepeek = 3'b101 ; 
						temp_busyout = 1'b1;
						temp_nas0 = 1'b0;
						temp_nas1 = 1'b0;

                     if ((busy[1]) == 1'b1)
                         begin
                            temp_groupID[1] = ~groupID[1] ; 
                            temp_raygroupvalid1 = 1'b0 ; 
                            temp_count = count - 1 ; 
                            if ((loaded[0]) == 1'b1 & (busy[0]) == 1'b0)
                            begin
                               temp_raygroupvalid0 = 1'b1 ; 
                            end

                            else if ((loaded[0]) == 1'b0)
                            begin
                               temp_active = 1'b0 ; 
                            end
                            else
                            begin
                               temp_active = 1'b1 ; 
                            end 
                         end 
                         temp_loaded[1] = 1'b0 ; 
                         temp_addr[1:0] = 2'b00 ; 

             temp_cycles = cycles + 1 ; 
                   end
          3 :
                   begin
                      if ((busy[0]) == 1'b0)
                      begin
                         next_state = 3 ; 

                      end
                      else if ((loaded[1]) == 1'b1 & (busy[1]) == 1'b0)
                      begin
                         next_state = 4 ; 
                      end
                      else if (count > 0)
                      begin
                         next_state = 1 ; 
                      end
                      else
                      begin
                         next_state = 0 ; 

                      end 
                      statepeek = 3'b100 ; 
						temp_busyout = 1'b1;
						temp_nas0 = 1'b0;
						temp_nas1 = 1'b0;

                         if ((busy[0]) == 1'b1)
                         begin
                            temp_groupID[0] = ~groupID[0] ; 
                            temp_raygroupvalid0 = 1'b0 ; 
                            temp_count = count - 1 ; 
                            if ((loaded[1]) == 1'b1 & (busy[1]) == 1'b0)
                            begin
                               temp_raygroupvalid1 = 1'b1 ; 

                            end
                            else if ((loaded[1]) == 1'b0)
                            begin
                               temp_active = 1'b1 ; 
                            end
                            else
                            begin
                               temp_active = 1'b0 ; 
                            end 
                         end 
                         temp_loaded[0] = 1'b0 ; 
                         temp_addr[1:0] = 2'b00 ; 


             temp_cycles = cycles + 1 ; 
                   end
       endcase 
    end 
 endmodule
    
    
    
    
    
    
    

 module resultrecieve (valid01, valid10, id01a, id01b, id01c, id10a, id10b, id10c, hit01a, hit01b, hit01c, hit10a, hit10b, hit10c, u01a, u01b, u01c, v01a, v01b, v01c, u10a, u10b, u10c, v10a, v10b, v10c, rgResultData, rgResultReady, rgResultSource, globalreset, clk);

    output valid01; 
    reg valid01;
    output valid10; 
    reg valid10;
    output[15:0] id01a; 
    reg[15:0] id01a;
    output[15:0] id01b; 
    reg[15:0] id01b;
    output[15:0] id01c; 
    reg[15:0] id01c;

    output[15:0] id10a; 
    reg[15:0] id10a;
    output[15:0] id10b; 
    reg[15:0] id10b;
    output[15:0] id10c; 
    reg[15:0] id10c;
    output hit01a; 
    reg hit01a;
    output hit01b; 
    reg hit01b;
    output hit01c; 
    reg hit01c;

    output hit10a; 
    reg hit10a;
    output hit10b; 
    reg hit10b;
    output hit10c; 
    reg hit10c;
    output[7:0] u01a; 
    reg[7:0] u01a;
    output[7:0] u01b; 
    reg[7:0] u01b;
    output[7:0] u01c; 
    reg[7:0] u01c;

    output[7:0] v01a; 
    reg[7:0] v01a;
    output[7:0] v01b; 
    reg[7:0] v01b;
    output[7:0] v01c; 
    reg[7:0] v01c;
    output[7:0] u10a; 
    reg[7:0] u10a;
    output[7:0] u10b; 
    reg[7:0] u10b;
    output[7:0] u10c; 
    reg[7:0] u10c;

    output[7:0] v10a; 
    reg[7:0] v10a;
    output[7:0] v10b; 
    reg[7:0] v10b;
    output[7:0] v10c; 
    reg[7:0] v10c;
    input[31:0] rgResultData; 
    input rgResultReady; 
    input[1:0] rgResultSource; 
    input globalreset; 
    input clk; 

    reg temp_valid01;
    reg temp_valid10;
    reg[15:0] temp_id01a;
    reg[15:0] temp_id01b;
    reg[15:0] temp_id01c;
    reg[15:0] temp_id10a;
    reg[15:0] temp_id10b;
    reg[15:0] temp_id10c;
    reg temp_hit01a;
    reg temp_hit01b;
    reg temp_hit01c;
    reg temp_hit10a;
    reg temp_hit10b;
    reg temp_hit10c;
    reg[7:0] temp_u01a;
    reg[7:0] temp_u01b;
    reg[7:0] temp_u01c;
    reg[7:0] temp_v01a;
    reg[7:0] temp_v01b;
    reg[7:0] temp_v01c;
    reg[7:0] temp_u10a;
    reg[7:0] temp_u10b;
    reg[7:0] temp_u10c;
    reg[7:0] temp_v10a;
    reg[7:0] temp_v10b;
    reg[7:0] temp_v10c;


    reg[2:0] state; 
    reg[2:0] next_state; 

    always @(posedge clk or posedge globalreset)
    begin
       if (globalreset == 1'b1)
       begin
          state <= 0 ; 
          valid01 <= 1'b0 ; 
          valid10 <= 1'b0 ; 
          hit01a <= 1'b0 ; 
          hit01b <= 1'b0 ; 
          hit01c <= 1'b0 ; 
          hit10a <= 1'b0 ; 
          hit10b <= 1'b0 ; 
          hit10c <= 1'b0 ; 
          id01a <= 0;

          id01b <= 0;
          id01c <= 0;
          id10a <= 0;
          id10b <= 0;
          id10c <= 0;
          u01a <= 0;
          u01b <= 0;
          u01c <= 0;
          v01a <= 0;
          v01b <= 0;
          v01c <= 0;
          u10a <= 0;

          u10b <= 0;
          u10c <= 0;
          v10a <= 0;
          v10b <= 0;
          v10c <= 0;
       end
       else
       begin
          state <= next_state ; 

valid01 <= temp_valid01;
valid10 <= temp_valid10;
id01a <= temp_id01a;
id01b <= temp_id01b;
id01c <= temp_id01c;
hit01a <= temp_hit01a;
hit01b <= temp_hit01b;
hit01c <= temp_hit01c;
u01a <= temp_u01a;
u01b <= temp_u01b;
u01c <= temp_u01c;
u10a <= temp_u10a;
u10b <= temp_u10b;
u10c <= temp_u10c;
v01a <= temp_v01a;
v01b <= temp_v01b;
v01c <= temp_v01c;
v10a <= temp_v10a;
v10b <= temp_v10b;
v10c <= temp_v10c;
hit10a <= temp_hit10a;
hit10b <= temp_hit10b;
hit10c <= temp_hit10c;
       end 
    end 


    always @(state or rgResultReady or rgResultSource)
    begin
       case (state)
          0 :
                   begin
                      if (rgResultReady == 1'b1 & rgResultSource == 2'b01)
                      begin
                         next_state = 1 ; 
                      end
                      else if (rgResultReady == 1'b1 & rgResultSource == 2'b10)
                      begin

                         next_state = 4 ; 
                      end
                      else
                      begin
                         next_state = 0 ; 
                      end 


			temp_valid01 = 1'b0 ; 
				          temp_valid10 = 1'b0 ; 
                         if (rgResultReady == 1'b1 & rgResultSource == 2'b01)
                         begin
                            temp_id01a = rgResultData[31:16] ; 
                            temp_id01b = rgResultData[15:0] ; 
                         end
                         else if (rgResultReady == 1'b1 & rgResultSource == 2'b10)
                         begin
                            temp_id10a = rgResultData[31:16] ; 
                            temp_id10b = rgResultData[15:0] ; 
                         end 

                   end

          1 :
                   begin
                      next_state = 2 ; 

			temp_valid01 = 1'b0 ; 
				          temp_valid10 = 1'b0 ; 
                         temp_id01c = rgResultData[15:0] ; 
                         temp_hit01a = rgResultData[18] ; 
                         temp_hit01b = rgResultData[17] ; 
                         temp_hit01c = rgResultData[16] ; 

                   end
          2 :

                   begin
                      next_state = 3 ; 

			temp_valid01 = 1'b0 ; 
				          temp_valid10 = 1'b0 ; 
                         temp_u01a = rgResultData[23:16] ; 
                         temp_u01b = rgResultData[15:8] ; 
                         temp_u01c = rgResultData[7:0] ; 

                   end
          3 :
                   begin
                      next_state = 0 ; 

				          temp_valid10 = 1'b0 ; 
                         temp_v01a = rgResultData[23:16] ; 
                         temp_v01b = rgResultData[15:8] ; 
                         temp_v01c = rgResultData[7:0] ; 
                         temp_valid01 = 1'b1 ; 

                   end
          4 :
                   begin
                      next_state = 5 ; 

          				temp_valid01 = 1'b0 ; 
				          temp_valid10 = 1'b0 ; 
                         temp_id10c = rgResultData[15:0] ; 

                         temp_hit10a = rgResultData[18] ; 
                         temp_hit10b = rgResultData[17] ; 
                         temp_hit10c = rgResultData[16] ; 

                   end
          5 :

                   begin
                      next_state = 6 ; 

          				temp_valid01 = 1'b0 ; 
				          temp_valid10 = 1'b0 ; 
                         temp_u10a = rgResultData[23:16] ; 
                         temp_u10b = rgResultData[15:8] ; 
                         temp_u10c = rgResultData[7:0] ; 

                   end
          6 :
                   begin
                      next_state = 0 ; 

      				temp_valid01 = 1'b0 ; 
                         temp_v10a = rgResultData[23:16] ; 
                         temp_v10b = rgResultData[15:8] ; 
                         temp_v10c = rgResultData[7:0] ; 
                         temp_valid10 = 1'b1 ; 

                   end
       endcase 
    end 
 endmodule
    
    
    
    
    
    
    
    
    
    
    
    
    
    

 module resultwriter (valid01, valid10, id01a, id01b, id01c, id10a, id10b, id10c, hit01a, hit01b, hit01c, hit10a, hit10b, hit10c, u01a, u01b, u01c, v01a, v01b, v01c, u10a, u10b, u10c, v10a, v10b, v10c, addr, as01, as10, bkcolour, shadedata, triID, wantshadedata, shadedataready, texinfo, texaddr, texeladdr, texel, wanttexel, texelready, dataout, addrout, write, ack, globalreset, clk);

    input valid01; 
    input valid10; 
    input[15:0] id01a; 
    input[15:0] id01b; 
    input[15:0] id01c; 
    input[15:0] id10a; 
    input[15:0] id10b; 
    input[15:0] id10c; 

    input hit01a; 
    input hit01b; 
    input hit01c; 
    input hit10a; 
    input hit10b; 
    input hit10c; 
    input[7:0] u01a; 
    input[7:0] u01b; 
    input[7:0] u01c; 
    input[7:0] v01a; 
    input[7:0] v01b; 
    input[7:0] v01c; 

    input[7:0] u10a; 
    input[7:0] u10b; 
    input[7:0] u10c; 
    input[7:0] v10a; 
    input[7:0] v10b; 
    input[7:0] v10c; 
    input[17:0] addr; 
    input as01; 
    input as10; 
    input[20:0] bkcolour; 
    input[63:0] shadedata; 
    output[15:0] triID; 

    reg[15:0] triID;
    output wantshadedata; 
    reg wantshadedata;
    input shadedataready; 
    input[20:0] texinfo; 
    output[3:0] texaddr; 
    wire[3:0] texaddr;
    output[17:0] texeladdr; 
    wire[17:0] texeladdr;
    input[63:0] texel; 
    output wanttexel; 
    reg wanttexel;

    input texelready; 
    output[63:0] dataout; 
    // PAJ see lower note wire[63:0] dataout;
    reg[63:0] dataout;
    output[17:0] addrout; 
    wire[17:0] addrout;
    output write; 
    wire write;
    reg temp_write;
    input ack; 
    input globalreset; 
    input clk; 

    reg[3:0] state; 
    reg[3:0] next_state; 
    reg pending01; 
    reg pending10; 
    reg process01; 
    wire[17:0] addrout01; 
    wire[17:0] addrout10; 
    wire shiften01; 
    wire shiften10; 
    reg temp_shiften01; 
    reg temp_shiften10; 
    reg[20:0] shadedataa; 
    reg[20:0] shadedatab; 
    reg[20:0] shadedatac; 
    wire hita; 
    wire hitb; 
    wire hitc; 

    reg[2:0] selectuv; 
    wire[6:0] blr; 
    wire[6:0] blg; 
    wire[6:0] blb; 
    reg texmap; 
    reg lmenable; 
    wire[1:0] texelselect; 
    wire[6:0] texelr; 
    wire[6:0] texelg; 
    wire[6:0] texelb; 
    reg[20:0] texinfol; 

    reg temp_pending01; 
    reg temp_pending10; 
    reg temp_process01; 
    reg temp_texmap; 
    reg[20:0] temp_texinfol; 
    reg[20:0] temp_shadedataa; 
    reg[20:0] temp_shadedatab; 
    reg[20:0] temp_shadedatac; 

    col16to21 col16to21inst (texel, texelselect, texelr, texelg, texelb); 
    linearmap linearmapinst (blb, blg, texinfol[17:0], texeladdr, texelselect, texinfol[20:18], lmenable, clk); 
    bilinearintrp bilinearimp (u01a, u01b, u01c, v01a, v01b, v01c, u10a, u10b, u10c, v10a, v10b, v10c, selectuv, shadedata[41:35], shadedata[62:56], shadedata[20:14], shadedata[34:28], shadedata[55:49], shadedata[13:7], shadedata[27:21], shadedata[48:42], shadedata[6:0], blr, blg, blb, clk); 
    fifo3 fifo3insta (addr, as01, addrout01, shiften01, globalreset, clk); 
    fifo3 fifo3instb (addr, as10, addrout10, shiften10, globalreset, clk); 
    assign hita = (hit01a & process01) | (hit10a & ~process01) ;
    assign hitb = (hit01b & process01) | (hit10b & ~process01) ;
    assign hitc = (hit01c & process01) | (hit10c & ~process01) ;
    assign texaddr = shadedata[59:56] ;
    assign shiften01 = temp_shiften01;
    assign shiften10 = temp_shiften10;
    assign write = temp_write;


    always @(posedge clk or posedge globalreset)
    begin
       if (globalreset == 1'b1)
       begin
          state <= 0 ; 
          pending01 <= 1'b0 ; 
          pending10 <= 1'b0 ; 
          shadedataa <= 0;
          shadedatab <= 0;
          shadedatac <= 0;
          process01 <= 1'b0 ; 
          texmap <= 1'b0 ; 

          texinfol <= 0;
       end
       else
       begin
          state <= next_state ; 

process01 <= temp_process01;
pending01 <= temp_pending01;
pending10 <= temp_pending10;
texmap <= temp_texmap;
texinfol <= temp_texinfol;
shadedataa <= temp_shadedataa;
shadedatab <= temp_shadedatab;
shadedatac <= temp_shadedatac;

    dataout <= {1'b0, 
					shadedataa[20],
					shadedataa[19],
					shadedataa[18],
					shadedataa[17],
					shadedataa[16],
					shadedataa[15],
					shadedataa[14],
					shadedataa[13],
					shadedataa[12],
					shadedataa[11],
					shadedataa[10],
					shadedataa[9],
					shadedataa[8],
					shadedataa[7],
					shadedataa[6],
					shadedataa[5],
					shadedataa[4],
					shadedataa[3],
					shadedataa[2],
					shadedataa[1],
					shadedataa[0],
					shadedatab[20],
					shadedatab[19],
					shadedatab[18],
					shadedatab[17],
					shadedatab[16],
					shadedatab[15],
					shadedatab[14],
					shadedatab[13],
					shadedatab[12],
					shadedatab[11],
					shadedatab[10],
					shadedatab[9],
					shadedatab[8],
					shadedatab[7],
					shadedatab[6],
					shadedatab[5],
					shadedatab[4],
					shadedatab[3],
					shadedatab[2],
					shadedatab[1],
					shadedatab[0],
					shadedatac[20],
					shadedatac[19],
					shadedatac[18],
					shadedatac[17],
					shadedatac[16],
					shadedatac[15],
					shadedatac[14],
					shadedatac[13],
					shadedatac[12],
					shadedatac[11],
					shadedatac[10],
					shadedatac[9],
					shadedatac[8],
					shadedatac[7],
					shadedatac[6],
					shadedatac[5],
					shadedatac[4],
					shadedatac[3],
					shadedatac[2],
					shadedatac[1],
					shadedatac[0]} ;
       end 
//    end 
// PAJ used to be assign, but weird error, so added as register   assign dataout = {1'b0, 
    end 
    assign addrout = (process01 == 1'b1) ? addrout01 : addrout10 ;

    always @(state or process01 or pending10 or ack or shadedataready or id01a or 
             id01b or id01c or id10a or id10b or id10c or selectuv or hita or 
             hitb or hitc or shadedata or pending01 or texmap or texelready)
    begin
       case (state)
          0 :
                   begin
				       wantshadedata = 1'b0 ; 
   				       triID = 0;
				       selectuv = 0;
				       lmenable = 1'b0 ; 
				       wanttexel = 1'b0 ; 
                      if (pending01 == 1'b1 | pending10 == 1'b1)
                      begin
                         next_state = 2 ; 
                      end
                      else

                      begin
                         next_state = 0 ; 
                      end 
	          if (valid01 == 1'b1)
				          begin
				             temp_pending01 = 1'b1 ; 
				          end 
				          if (valid10 == 1'b1)
				          begin
				             temp_pending10 = 1'b1 ; 
				          end 
                         temp_process01 = pending01 ; 

							temp_shiften01 = 1'b0;
							temp_shiften10 = 1'b0;
					    	temp_write = 1'b0;
                   end
          2 :
                   begin
				       lmenable = 1'b0 ; 
				       wanttexel = 1'b0 ; 
                      wantshadedata = 1'b1 ; 
                      selectuv[2] = ~process01 ; 
                      selectuv[1:0] = 2'b00 ; 
                      if (process01 == 1'b1)
                      begin
                         triID = id01a ; 

                      end
                      else
                      begin
                         triID = id10a ; 
                      end 
                      if (shadedataready == 1'b1)
                      begin
                         if (hita == 1'b1 & ((shadedata[63]) == 1'b1 | shadedata[63:62] == 2'b01))
                         begin
                            next_state = 3 ; 
                         end
                         else

                         begin
                            next_state = 4 ; 
                         end 
                      end
                      else
                      begin
                         next_state = 2 ; 
                      end 

	          if (valid01 == 1'b1)
				          begin
				             temp_pending01 = 1'b1 ; 
				          end 
				          if (valid10 == 1'b1)
				          begin
				             temp_pending10 = 1'b1 ; 
				          end 

                         if (hita == 1'b1)
                         begin
                            temp_shadedataa = shadedata[20:0] ; 
                            temp_texmap = (~shadedata[63]) & shadedata[62] ; 
                         end
                         else
                         begin
                            temp_shadedataa = bkcolour ; 
                         end 

							temp_shiften01 = 1'b0;
							temp_shiften10 = 1'b0;
					    	temp_write = 1'b0;
                   end
          3 :
                   begin
				       wantshadedata = 1'b0 ; 
   				       triID = 0;
				       lmenable = 1'b0 ; 
				       wanttexel = 1'b0 ; 
                      selectuv[2] = ~process01 ; 

                      selectuv[1:0] = 2'b00 ; 
                      next_state = 8 ; 

	          if (valid01 == 1'b1)
				          begin
				             temp_pending01 = 1'b1 ; 
				          end 
				          if (valid10 == 1'b1)
				          begin
				             temp_pending10 = 1'b1 ; 
				          end 
                         temp_texinfol = texinfo ; 

							temp_shiften01 = 1'b0;
							temp_shiften10 = 1'b0;
					    	temp_write = 1'b0;

                   end
          8 :
                   begin
				       wantshadedata = 1'b0 ; 
   				       triID = 0;
				       wanttexel = 1'b0 ; 
                      selectuv[2] = ~process01 ; 
                      selectuv[1:0] = 2'b00 ; 
                      lmenable = 1'b1 ; 
                      if (texmap == 1'b1)
                      begin

                         next_state = 11 ; 
                      end
                      else
                      begin
                         next_state = 4 ; 
                      end 

	          if (valid01 == 1'b1)
				          begin
				             temp_pending01 = 1'b1 ; 
				          end 
				          if (valid10 == 1'b1)
				          begin
				             temp_pending10 = 1'b1 ; 
				          end 
                         temp_shadedataa[6:0] = blb ; 
                         temp_shadedataa[13:7] = blg ; 
                         temp_shadedataa[20:14] = blr ; 

							temp_shiften01 = 1'b0;
							temp_shiften10 = 1'b0;
					    	temp_write = 1'b0;
                   end
          11 :
                   begin
				       wantshadedata = 1'b0 ; 
   				       triID = 0;
				       selectuv = 0;
				       lmenable = 1'b0 ; 

                      wanttexel = 1'b1 ; 
                      if (texelready == 1'b1)
                      begin
                         next_state = 4 ; 
                      end
                      else
                      begin
                         next_state = 11 ; 
                      end 

	          if (valid01 == 1'b1)
				          begin
				             temp_pending01 = 1'b1 ; 
				          end 
				          if (valid10 == 1'b1)
				          begin
				             temp_pending10 = 1'b1 ; 
				          end 

                         temp_shadedataa[6:0] = texelb ; 
                         temp_shadedataa[13:7] = texelg ; 
                         temp_shadedataa[20:14] = texelr ; 

							temp_shiften01 = 1'b0;
							temp_shiften10 = 1'b0;
					    	temp_write = 1'b0;
                   end
          12 :
                   begin
				       wantshadedata = 1'b0 ; 
   				       triID = 0;
				       selectuv = 0;
				       lmenable = 1'b0 ; 

                      wanttexel = 1'b1 ; 
                      if (texelready == 1'b1)
                      begin
                         next_state = 5 ; 
                      end
                      else
                      begin
                         next_state = 12 ; 
                      end 

	          if (valid01 == 1'b1)
				          begin
				             temp_pending01 = 1'b1 ; 
				          end 
				          if (valid10 == 1'b1)
				          begin
				             temp_pending10 = 1'b1 ; 
				          end 
                         temp_shadedatab[6:0] = texelb ; 
                         temp_shadedatab[13:7] = texelg ; 
                         temp_shadedatab[20:14] = texelr ; 

							temp_shiften01 = 1'b0;
							temp_shiften10 = 1'b0;
					    	temp_write = 1'b0;
                   end
          13 :
                   begin
				       wantshadedata = 1'b0 ; 
   				       triID = 0;
				       selectuv = 0;
				       lmenable = 1'b0 ; 

                      wanttexel = 1'b1 ; 
                      if (texelready == 1'b1)
                      begin
                         next_state = 1 ; 
                      end
                      else
                      begin
                         next_state = 13 ; 
                      end 

	          if (valid01 == 1'b1)
				          begin
				             temp_pending01 = 1'b1 ; 
				          end 
				          if (valid10 == 1'b1)
				          begin
				             temp_pending10 = 1'b1 ; 
				          end 

                         temp_shadedatac[6:0] = texelb ; 
                         temp_shadedatac[13:7] = texelg ; 
                         temp_shadedatac[20:14] = texelr ; 

                   end
          6 :
                   begin
				       wantshadedata = 1'b0 ; 
   				       triID = 0;
				       lmenable = 1'b0 ; 
				       wanttexel = 1'b0 ; 

                      selectuv[2] = ~process01 ; 
                      selectuv[1:0] = 2'b01 ; 
                      next_state = 9 ; 

	          if (valid01 == 1'b1)
				          begin
				             temp_pending01 = 1'b1 ; 
				          end 
				          if (valid10 == 1'b1)
				          begin
				             temp_pending10 = 1'b1 ; 
				          end 
                         temp_texinfol = texinfo ; 

							temp_shiften01 = 1'b0;
							temp_shiften10 = 1'b0;
					    	temp_write = 1'b0;
                   end
          9 :
                   begin
				       wantshadedata = 1'b0 ; 
   				       triID = 0;
				       wanttexel = 1'b0 ; 
                      selectuv[2] = ~process01 ; 
                      selectuv[1:0] = 2'b01 ; 
                      lmenable = 1'b1 ; 
                      if (texmap == 1'b1)
                      begin
                         next_state = 12 ; 

                      end
                      else
                      begin
                         next_state = 5 ; 
                      end 

	          if (valid01 == 1'b1)
				          begin
				             temp_pending01 = 1'b1 ; 
				          end 
				          if (valid10 == 1'b1)
				          begin
				             temp_pending10 = 1'b1 ; 
				          end 

                         temp_shadedatab[6:0] = blb ; 
                         temp_shadedatab[13:7] = blg ; 
                         temp_shadedatab[20:14] = blr ; 

							temp_shiften01 = 1'b0;
							temp_shiften10 = 1'b0;
					    	temp_write = 1'b0;
                   end
          7 :
                   begin
				       wantshadedata = 1'b0 ; 
   				       triID = 0;
				       lmenable = 1'b0 ; 
				       wanttexel = 1'b0 ; 
                      selectuv[2] = ~process01 ; 
                      selectuv[1:0] = 2'b10 ; 
                      next_state = 10 ; 

	          if (valid01 == 1'b1)
				          begin
				             temp_pending01 = 1'b1 ; 
				          end 
				          if (valid10 == 1'b1)
				          begin
				             temp_pending10 = 1'b1 ; 
				          end 
                         temp_texinfol = texinfo ; 

							temp_shiften01 = 1'b0;
							temp_shiften10 = 1'b0;
					    	temp_write = 1'b0;
                   end

          10 :
                   begin
				       wantshadedata = 1'b0 ; 
   				       triID = 0;
				       wanttexel = 1'b0 ; 
                      selectuv[2] = ~process01 ; 
                      selectuv[1:0] = 2'b10 ; 
                      if (texmap == 1'b1)
                      begin
                         next_state = 13 ; 
                      end
                      else
                      begin
                         next_state = 1 ; 
                      end 

                      lmenable = 1'b1 ; 

	          if (valid01 == 1'b1)
				          begin
				             temp_pending01 = 1'b1 ; 
				          end 
				          if (valid10 == 1'b1)
				          begin
				             temp_pending10 = 1'b1 ; 
				          end 
                         temp_shadedatac[6:0] = blb ; 
                         temp_shadedatac[13:7] = blg ; 
                         temp_shadedatac[20:14] = blr ; 

							temp_shiften01 = 1'b0;
							temp_shiften10 = 1'b0;
					    	temp_write = 1'b0;
                   end
          4 :
                   begin
				       wantshadedata = 1'b0 ; 
				       lmenable = 1'b0 ; 
				       wanttexel = 1'b0 ; 
                      selectuv[2] = ~process01 ; 
                      selectuv[1:0] = 2'b01 ; 
                      if (process01 == 1'b1)
                      begin
                         triID = id01b ; 
                      end
                      else
                      begin

                         triID = id10b ; 
                      end 
                      if (shadedataready == 1'b1)
                      begin
                         if (hitb == 1'b1 & ((shadedata[63]) == 1'b1 | shadedata[63:62] == 2'b01))
                         begin
                            next_state = 6 ; 
                         end
                         else
                         begin
                            next_state = 5 ; 
                         end 

                      end
                      else
                      begin
                         next_state = 4 ; 
                      end 

	          if (valid01 == 1'b1)
				          begin
				             temp_pending01 = 1'b1 ; 
				          end 
				          if (valid10 == 1'b1)
				          begin
				             temp_pending10 = 1'b1 ; 
				          end 

                         if (hitb == 1'b1)
                         begin
                            temp_shadedatab = shadedata[20:0] ; 
                            temp_texmap = (~shadedata[63]) & shadedata[62] ; 
                         end
                         else
                         begin
                            temp_shadedatab = bkcolour ; 
                         end 

							temp_shiften01 = 1'b0;
							temp_shiften10 = 1'b0;
					    	temp_write = 1'b0;
                   end
          5 :
                   begin
				       lmenable = 1'b0 ; 
				       wanttexel = 1'b0 ; 
                      wantshadedata = 1'b1 ; 
                      selectuv[2] = ~process01 ; 
                      selectuv[1:0] = 2'b10 ; 
                      if (process01 == 1'b1)

                      begin
                         triID = id01c ; 
                      end
                      else
                      begin
                         triID = id10c ; 
                      end 
                      if (shadedataready == 1'b1)
                      begin
                         if (hitc == 1'b1 & ((shadedata[63]) == 1'b1 | shadedata[63:62] == 2'b01))
                         begin
                            next_state = 7 ; 

                         end
                         else
                         begin
                            next_state = 1 ; 
                         end 
                      end
                      else
                      begin
                         next_state = 5 ; 
                      end 

	          if (valid01 == 1'b1)
				          begin
				             temp_pending01 = 1'b1 ; 
				          end 
				          if (valid10 == 1'b1)
				          begin
				             temp_pending10 = 1'b1 ; 
				          end 

                         if (hitc == 1'b1)
                          begin
                            temp_shadedatac = shadedata[20:0] ; 
                            temp_texmap = (~shadedata[63]) & shadedata[62] ; 
                         end
                         else
                         begin
                            temp_shadedatac = bkcolour ; 
                         end 

							temp_shiften01 = 1'b0;
							temp_shiften10 = 1'b0;
					    	temp_write = 1'b0;
                   end
          1 :

                   begin
				       wantshadedata = 1'b0 ; 
   				       triID = 0;
				       selectuv = 0;
				       lmenable = 1'b0 ; 
				       wanttexel = 1'b0 ; 
                      if (ack == 1'b1)
                      begin
                         next_state = 0 ; 
                      end
                      else
                      begin
                         next_state = 1 ; 
                      end 

                         if (ack == 1'b1 & process01 == 1'b1)
                         begin
                            temp_pending01 = 1'b0 ; 
                         end

                          else if (ack == 1'b1 & process01 == 1'b0)
                         begin
                            temp_pending10 = 1'b0 ; 
                         end 

    				if (process01 == 1'b1 &  ack == 1'b1)
						begin
							temp_shiften01 = 1'b1;
							temp_shiften10 = 1'b1;
						end
					    temp_write = 1'b1;
                   end
       endcase 
    end 
 endmodule
 //////////////////////////////////////////////////////////////////////////////////////////////
 //
 // Verilog file generated by X-HDL - Revision 3.2.38  Jan. 9, 2004 
 // Sun Feb  8 14:14:35 2004
 //
 //      Input file         : G:/jamieson/VERILOG_BENCHMARKS/RAYTRACE/col16to21.vhd
 //      Design name        : col16to21
 //      Author             : 
 //      Company            : 
 //
 //      Description        : 
 //
 //
 //////////////////////////////////////////////////////////////////////////////////////////////
 //
 module col16to21 (dataline, texelselect, r, g, b);

    input[63:0] dataline; 
    input[1:0] texelselect; 
    output[6:0] r; 
    wire[6:0] r;
    output[6:0] g; 
    wire[6:0] g;
    output[6:0] b; 
    wire[6:0] b;

    reg[15:0] col16; 

    always @(dataline or texelselect)
    begin
       case (texelselect)
          2'b00 :
                   begin
                      col16 = dataline[15:0] ; 
                   end
          2'b01 :
                   begin
                      col16 = dataline[31:16] ; 
                   end
          2'b10 :
                   begin
                      col16 = dataline[47:32] ; 
                   end
          2'b11 :
                   begin
                      col16 = dataline[63:48] ; 
                   end
       endcase 
    end 
    assign r = {col16[15:10], 1'b0} ;
    assign g = {col16[9:5], 2'b00} ;
    assign b = {col16[4:0], 2'b00} ;
 endmodule
 module linearmap (u, v, start, addr, texelselect, factor, enable, clk);

    input[6:0] u; 
    input[6:0] v; 
    input[17:0] start; 
    output[17:0] addr; 
    reg[17:0] addr;
    output[1:0] texelselect; 
    wire[1:0] texelselect;

    input[2:0] factor; 
    input enable; 
    input clk; 

    reg[6:0] ul; 
    reg[6:0] vl; 

    assign texelselect = ul[1:0] ;

    always @(posedge clk)
    begin
       if (enable == 1'b1)
       begin
          ul <= u ; 
          vl <= v ; 
       end 
       else
       begin
          ul <= ul ; 
          vl <= vl ; 
       end 
       case (factor)
          3'b000 :
                   begin
                      addr <= start + ({13'b0000000000000, ul[6:2]}) + ({11'b00000000000, vl}) ; 
                   end
          3'b001 :
                   begin
                      addr <= start + ({13'b0000000000000, ul[6:2]}) + ({10'b0000000000, vl, 1'b0}) ; 

                   end
          3'b010 :
                   begin
                      addr <= start + ({13'b0000000000000, ul[6:2]}) + ({9'b000000000, vl, 2'b00}) ; 
                   end
          3'b011 :
                   begin
                      addr <= start + ({13'b0000000000000, ul[6:2]}) + ({8'b00000000, vl, 3'b000}) ; 
                   end
          3'b100 :
                   begin
                      addr <= start + ({13'b0000000000000, ul[6:2]}) + ({7'b0000000, vl, 4'b0000}) ; 

                   end
          3'b101 :
                   begin
                      addr <= start + ({13'b0000000000000, ul[6:2]}) + ({6'b000000, vl, 5'b00000}) ; 
                   end
          3'b110 :
                   begin
                      addr <= start + ({13'b0000000000000, ul[6:2]}) + ({5'b00000, vl, 6'b000000}) ; 
                   end
          3'b111 :
                   begin
                      addr <= start + ({13'b0000000000000, ul[6:2]}) + ({4'b0000, vl, 7'b0000000}) ; 

                   end
       endcase  
    end 
 endmodule
     module bilinearintrp (u01a, u01b, u01c, v01a, v01b, v01c, u10a, u10b, u10c, v10a, v10b, v10c, selectuv, ru, rv, rw, gu, gv, gw, bu, bv, bw, r, g, b, clk);

        input[7:0] u01a; 
        input[7:0] u01b; 
        input[7:0] u01c; 
        input[7:0] v01a; 
        input[7:0] v01b; 
        input[7:0] v01c; 
        input[7:0] u10a; 
        input[7:0] u10b; 
        input[7:0] u10c; 
        input[7:0] v10a; 
        input[7:0] v10b; 
        input[7:0] v10c; 
        input[2:0] selectuv; 
        input[6:0] ru; 
        input[6:0] rv; 
        input[6:0] rw; 
        input[6:0] gu; 
        input[6:0] gv; 
        input[6:0] gw; 
        input[6:0] bu; 
        input[6:0] bv; 
        input[6:0] bw; 
        output[6:0] r; 
        wire[6:0] r;
        output[6:0] g; 
        wire[6:0] g;
        output[6:0] b; 
        wire[6:0] b;
        input clk; 

        reg[7:0] u; 
        reg[7:0] v; 
        reg[7:0] ul; 
        reg[7:0] vl; 
        reg[7:0] wl; 
        reg[14:0] i1b; 
        reg[14:0] i2b; 
        reg[14:0] i3b; 
        reg[14:0] i1g; 
        reg[14:0] i2g; 
        reg[14:0] i3g; 
        reg[14:0] i1r; 
        reg[14:0] i2r; 
        reg[14:0] i3r; 
        reg[6:0] rul; 
        reg[6:0] rvl; 
        reg[6:0] rwl; 
        reg[6:0] gul; 
        reg[6:0] gvl; 
        reg[6:0] gwl; 
        reg[6:0] bul; 
        reg[6:0] bvl; 
        reg[6:0] bwl; 

        always @(selectuv or u01a or u01b or u01c or v01a or v01b or v01c or u10a or 
                 u10b or u10c or v10a or v10b or v10c)
        begin
           case (selectuv)
              3'b000 :
                       begin
                          u = u01a ; 
                          v = v01a ; 
                       end
              3'b001 :
                       begin
                          u = u01b ; 
						 v = v01b ; 
                       end
              3'b010 :
                       begin
                          u = u01c ; 
                          v = v01c ; 
                       end
              3'b100 :
                       begin
                          u = u10a ; 
                          v = v10a ; 
                       end
              3'b101 :
                       begin
                          u = u10b ; 
                          v = v10b ; 
                       end
              3'b110 :
                       begin
                          u = u10c ; 
                          v = v10c ; 
                       end
              default :
                       begin
                          u = 0;
                          v = 0;
                       end
           endcase 
        end 

        always @(posedge clk)
        begin
           wl <= 8'b11111111 - u - v ; 
           ul <= u ; 
           vl <= v ; 
           rul <= ru ; 
           rvl <= rv ; 
           rwl <= rw ; 
           gul <= gu ; 
           gvl <= gv ; 
           gwl <= gw ; 
           bul <= bu ; 
           bvl <= bv ; 
           bwl <= bw ; 
           i1r <= ul * rul ; 
           i2r <= vl * rvl ; 
           i3r <= wl * rwl ; 
           i1g <= ul * gul ; 
           i2g <= vl * gvl ; 
           i3g <= wl * gwl ; 
           i1b <= ul * bul ; 
           i2b <= vl * bvl ; 
           i3b <= wl * bwl ;  
        end 
        assign r = (i1r + i2r + i3r) ;
        assign g = (i1g + i2g + i3g) ;
        assign b = (i1b + i2b + i3b) ;
     endmodule



module fifo3 (datain, writeen, dataout, shiften, globalreset, clk);

    input[18 - 1:0] datain; 
    input writeen; 
    output[18 - 1:0] dataout; 
    wire[18 - 1:0] dataout;
    input shiften; 
    input globalreset; 
    input clk; 

    reg[18 - 1:0] data0; 
    reg[18 - 1:0] data1; 
    reg[18 - 1:0] data2; 

    reg[1:0] pos; 

    assign dataout = data0 ;

    always @(posedge clk or posedge globalreset)
    begin
       if (globalreset == 1'b1)
       begin
          pos <= 2'b00 ; 
          data0 <= 0 ; 
          data1 <= 0 ; 
          data2 <= 0 ; 
       end
       else
       begin
          if (writeen == 1'b1 & shiften == 1'b1)
          begin
             case (pos)
                2'b00 :
                         begin
                            data0 <= 0 ; 
                            data1 <= 0 ; 
                            data2 <= 0 ; 
                         end

                2'b01 :
                         begin
                            data0 <= datain ; 
                            data1 <= 0 ; 
                            data2 <= 0 ; 
                         end
                2'b10 :
                         begin
                            data0 <= data1 ; 
                            data1 <= datain ; 
                            data2 <= 0 ; 
                         end

                2'b11 :
                         begin
                            data0 <= data1 ; 
                            data1 <= data2 ; 
                            data2 <= datain ; 
                         end
             endcase 
          end
          else if (shiften == 1'b1)
          begin
             data0 <= data1 ; 
             data1 <= data2 ; 
             pos <= pos - 1 ; 
          end
          else if (writeen == 1'b1)
          begin
             case (pos)
                2'b00 :
                         begin
                            data0 <= datain ; 
                         end
                2'b01 :
    					begin
                            data1 <= datain ; 
                         end
                2'b10 :
                         begin
                            data2 <= datain ; 
                         end
             endcase 
             pos <= pos + 1 ; 
          end 
       end 
    end 
 endmodule

