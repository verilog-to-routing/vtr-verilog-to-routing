    module paj_boundtop_hierarchy_no_mem (triIDvalid, triID, wanttriID, raydata, rayaddr, raywe, resultready, resultdata, globalreset, want_braddr, braddr_ready, braddrin, want_brdata, brdata_ready, brdatain, want_addr2, addr2_ready, addr2in, want_data2, data2_ready, data2in, pglobalreset, tm3_clk_v0, tm3_sram_data_in, tm3_sram_data_out, tm3_sram_addr, tm3_sram_we, tm3_sram_oe, tm3_sram_adsp, raygroup01, raygroupvalid01, busy01, raygroup10, raygroupvalid10, busy10, rgData, rgAddr, rgWE, rgAddrValid, rgDone, rgResultData, rgResultReady, rgResultSource, input1);
    

       output triIDvalid; 
       wire triIDvalid;
       output[15:0] triID; 
       wire[15:0] triID;
        input wanttriID; 
       output[31:0] raydata; 
       wire[31:0] raydata;
       output[3:0] rayaddr; 
       wire[3:0] rayaddr;
        output[2:0] raywe; 
       wire[2:0] raywe;
        input resultready; 
        input[31:0] resultdata; 
        output globalreset; 
        wire globalreset;
        output want_braddr; 
        wire want_braddr;
        input braddr_ready; 
        input[9:0] braddrin; 
        output want_brdata; 
        wire want_brdata;
        input brdata_ready; 
        input[31:0] brdatain; 
        output want_addr2; 
        wire want_addr2;
        input addr2_ready; 
        input[17:0] addr2in; 
        output want_data2; 
        wire want_data2;
        input data2_ready; 
        input[63:0] data2in; 
        input pglobalreset; 
        input tm3_clk_v0; 
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
        input[1:0] raygroup01; 
        input raygroupvalid01; 
        output busy01; 
        wire busy01;
        input[1:0] raygroup10; 
        input raygroupvalid10; 
        output busy10; 
        wire busy10;
        input[31:0] rgData; 
        input[3:0] rgAddr; 
        input[2:0] rgWE; 
        input rgAddrValid; 
        output rgDone; 
        wire rgDone;
        output[31:0] rgResultData; 
        wire[31:0] rgResultData;
        output rgResultReady; 
        wire rgResultReady;
        output[1:0] rgResultSource; 
        wire[1:0] rgResultSource;
        input input1; 
 wire raygroupwe; 
        wire raygroupwe01; 
        wire raygroupwe10; 
        wire[1:0] raygroupout; 
        wire[1:0] raygroupout01; 
        wire[1:0] raygroupout10; 
        wire[1:0] raygroupid; 
        wire[1:0] raygroupid01; 
        wire[1:0] raygroupid10; 
        reg[1:0] oldresultid; 
        wire[1:0] resultid; 
        wire[31:0] t1i; 
        wire[31:0] t2i; 
        wire[31:0] t3i; 
        wire[15:0] u1i; 
        wire[15:0] u2i; 
        wire[15:0] u3i; 
        wire[15:0] v1i; 
        wire[15:0] v2i; 
        wire[15:0] v3i; 
        wire[15:0] id1i; 
        wire[15:0] id2i; 
        wire[15:0] id3i; 
        wire hit1i; 
        wire hit2i; 
        wire hit3i; 
        wire newresult; 
        wire write; 
        reg reset; 
        wire reset01; 
        wire reset10; 
        wire[103:0] peekdata; 
        reg[103:0] peeklatch; 
        wire commit01; 
        wire commit10; 
        wire[1:0] baseaddress01; 
        wire[1:0] baseaddress10; 
        wire[1:0] done; 
        wire cntreset; 
        wire cntreset01; 
        wire cntreset10; 
        wire passCTS01; 
        wire passCTS10; 
        wire triIDvalid01; 
        wire triIDvalid10; 
        wire[15:0] triID01; 
        wire[15:0] triID10; 
        reg[9:0] boundNodeID; 
        wire[9:0] BoundNodeID01; 
        wire[9:0] BoundNodeID10; 
        wire enablenear; 
        wire enablenear01; 
        wire enablenear10; 
        wire ack01; 
        wire ack10; 
        wire empty01; 
        wire dataready01; 
        wire empty10; 
        wire dataready10; 
        wire lhreset01; 
        wire lhreset10; 
        wire[9:0] boundnodeIDout01; 
        wire[9:0] boundnodeIDout10; 
        wire[1:0] level01; 
        wire[1:0] level10; 
        wire[2:0] hitmask01; 
        wire[2:0] hitmask10; 
        // Offset Block Ram Read Signals
        wire[9:0] ostaddr; 
        wire[9:0] addrind01; 
        wire[9:0] addrind10; 
        wire ostaddrvalid; 
        wire addrindvalid01; 
        wire addrindvalid10; 
        wire ostdatavalid; 
        wire[31:0] ostdata; 
        // Tri List Ram Read Signals
        wire[17:0] tladdr; 
        wire[17:0] tladdr01; 
        wire[17:0] tladdr10; 
        wire tladdrvalid; 
        wire tladdrvalid01; 
        wire tladdrvalid10; 
        wire tldatavalid; 
wire[63:0] tldata; 
        // Final Result Signals
        wire[31:0] t1_01; 
        wire[31:0] t2_01; 
        wire[31:0] t3_01; 
        wire[31:0] t1_10; 
        wire[31:0] t2_10; 
        wire[31:0] t3_10; 
        wire[15:0] v1_01; 
        wire[15:0] v2_01; 
        wire[15:0] v3_01; 
        wire[15:0] v1_10; 
        wire[15:0] v2_10; 
        wire[15:0] v3_10; 
        wire[15:0] u1_01; 
        wire[15:0] u2_01; 
        wire[15:0] u3_01; 
        wire[15:0] u1_10; 
        wire[15:0] u2_10; 
        wire[15:0] u3_10; 
        wire[15:0] id1_01; 
        wire[15:0] id2_01; 
        wire[15:0] id3_01; 
        wire[15:0] id1_10; 
        wire[15:0] id2_10; 
        wire[15:0] id3_10; 
        wire hit1_01; 
        wire hit2_01; 
        wire hit3_01; 
        wire hit1_10; 
        wire hit2_10; 
        wire hit3_10; 
        wire bcvalid01; 
        wire bcvalid10; 
        wire[2:0] peekoffset1a; 
        wire[2:0] peekoffset1b; 
        wire[2:0] peekoffset0a; 
        wire[2:0] peekoffset0b; 
        wire[2:0] peekoffset2a; 
        wire[2:0] peekoffset2b; 
        wire[4:0] peekaddressa; 
        wire[4:0] peekaddressb; 
        wire doutput; 
        wire dack; 
        wire[4:0] state01; 
        wire[4:0] state10; 
        wire[2:0] junk1; 
        wire[2:0] junk1b; 
        wire junk2; 
        wire junk2a; 
        wire[1:0] junk3; 
        wire[1:0] junk4; 
        wire[13:0] debugcount01; 
        wire[13:0] debugcount10; 
        wire[1:0] debugsubcount01; 
        wire[1:0] debugsubcount10; 
        wire[2:0] statesram; 

        onlyonecycle oc (input1, doutput, pglobalreset, tm3_clk_v0); 
        // Real Stuff Starts Here
        assign ostaddr = addrind01 | addrind10 ;
        assign ostaddrvalid = addrindvalid01 | addrindvalid10 ;

        vblockramcontroller  offsettable(want_braddr, braddr_ready, braddrin, want_brdata, brdata_ready, brdatain, ostaddr, ostaddrvalid, ostdata, ostdatavalid, pglobalreset, tm3_clk_v0); 
        assign tladdr = tladdr01 | tladdr10 ;
        assign tladdrvalid = tladdrvalid01 | tladdrvalid10 ;
        sramcontroller trilist (want_addr2, addr2_ready, addr2in, want_data2, data2_ready, data2in, tladdr, tladdrvalid, tldata, tldatavalid, tm3_sram_data_in, tm3_sram_data_out, tm3_sram_addr, tm3_sram_we, tm3_sram_oe, tm3_sram_adsp, pglobalreset, tm3_clk_v0, statesram); 
        resultinterface ri (t1i, t2i, t3i, u1i, u2i, u3i, v1i, v2i, v3i, id1i, id2i, id3i, hit1i, hit2i, hit3i, resultid, newresult, resultready, resultdata, pglobalreset, tm3_clk_v0); 
        rayinterface rayint (raygroupout, raygroupwe, raygroupid, enablenear, rgData, rgAddr, rgWE, rgAddrValid, rgDone, raydata, rayaddr, raywe, pglobalreset, tm3_clk_v0); 
        boundcontroller  boundcont01(raygroupout01, raygroupwe01, raygroupid01, enablenear01, raygroup01, raygroupvalid01, busy01, triIDvalid01, triID01, wanttriID, reset01, baseaddress01, newresult, BoundNodeID01, resultid, hitmask01, dataready01, empty01, level01, boundnodeIDout01, ack01, lhreset01, addrind01, addrindvalid01, ostdata, ostdatavalid, tladdr01, tladdrvalid01, tldata, tldatavalid, t1i, t2i, t3i, u1i, u2i, u3i, v1i, v2i, v3i, id1i, id2i, id3i, hit1i, hit2i, hit3i, t1_01, t2_01, t3_01, u1_01, u2_01, u3_01, v1_01, v2_01, v3_01, id1_01, id2_01, id3_01, hit1_01, hit2_01, hit3_01, bcvalid01, done, cntreset01, passCTS01, passCTS10, pglobalreset, tm3_clk_v0, state01, debugsubcount01, debugcount01);
        boundcontroller  boundcont10(raygroupout10, raygroupwe10, raygroupid10, enablenear10, raygroup10, raygroupvalid10, busy10, triIDvalid10, triID10, wanttriID, reset10, baseaddress10, newresult, BoundNodeID10, resultid, hitmask10, dataready10, empty10, level10, boundnodeIDout10, ack10, lhreset10, addrind10, addrindvalid10, ostdata, ostdatavalid, tladdr10, tladdrvalid10, tldata, tldatavalid, t1i, t2i, t3i, u1i, u2i, u3i, v1i, v2i, v3i, id1i, id2i, id3i, hit1i, hit2i, hit3i, t1_10, t2_10, t3_10, u1_10, u2_10, u3_10, v1_10, v2_10, v3_10, id1_10, id2_10, id3_10, hit1_10, hit2_10, hit3_10, bcvalid10, done, cntreset10, passCTS10, passCTS01, pglobalreset, tm3_clk_v0, state10, debugsubcount10, debugcount01);
        resulttransmit restransinst (bcvalid01, bcvalid10, id1_01, id2_01, id3_01, id1_10, id2_10, id3_10, hit1_01, hit2_01, hit3_01, hit1_10, hit2_10, hit3_10, u1_01, u2_01, u3_01, v1_01, v2_01, v3_01, u1_10, u2_10, u3_10, v1_10, v2_10, v3_10, rgResultData, rgResultReady, rgResultSource, pglobalreset, tm3_clk_v0); 

assign raygroupout = raygroupout01 | raygroupout10 ;
    assign raygroupwe = raygroupwe01 | raygroupwe10 ;
    assign raygroupid = raygroupid01 | raygroupid10 ;
    assign triIDvalid = triIDvalid01 | triIDvalid10 ;
    assign enablenear = enablenear01 | enablenear10 ;
    assign triID = triID01 | triID10 ;
    assign cntreset = cntreset01 | cntreset10 ;

    //  reset <= reset01 or reset10;
    always @(BoundNodeID01 or BoundNodeID10 or resultid)
    begin
       if (resultid == 2'b01)
       begin
          boundNodeID = BoundNodeID01 ; 
       end
       else if (resultid == 2'b10)
       begin
          boundNodeID = BoundNodeID10 ; 
       end
       else
       begin
          boundNodeID = 10'b0000000000 ; 
       end 
    end 
    assign write = ((newresult == 1'b1) & (resultid != 0) & ((hit1i == 1'b1) | (hit2i == 1'b1) | (hit3i == 1'b1))) ? 1'b1 : 1'b0 ;

    sortedstack  st(t1i, {hit3i, hit2i, hit1i, boundNodeID}, write, reset, peekdata, pglobalreset, tm3_clk_v0); 
    assign commit01 = (done == 2'b01) ? 1'b1 : 1'b0 ;
    assign commit10 = (done == 2'b10) ? 1'b1 : 1'b0 ;
    assign dack = doutput | ack01 ;
    listhandler lh01 (peeklatch, commit01, hitmask01, dack, boundnodeIDout01, level01, empty01, dataready01, lhreset01, pglobalreset, tm3_clk_v0, peekoffset0a, peekoffset1a, peekoffset2a, junk2a, junk4); 
    listhandler lh02 (peeklatch, commit10, hitmask10, ack10, boundnodeIDout10, level10, empty10, dataready10, lhreset10, pglobalreset, tm3_clk_v0, junk1, junk1b, peekoffset2b, junk2, junk3); 

    always @(posedge tm3_clk_v0 or posedge pglobalreset)
    begin
       if (pglobalreset == 1'b1)
       begin
          peeklatch <= 0;
          reset <= 1'b0 ; 
          oldresultid <= 2'b00 ; 
       end
       else
       begin
          oldresultid <= resultid ; 
          // The reset is only for debugging
          if (resultid != oldresultid)
          begin
             reset <= 1'b1 ; 
          end
          else
          begin
             reset <= 1'b0 ; 
          end 
          if (done != 0)
          begin
             peeklatch <= peekdata ; 
          end 
       end 
    end 

    resultcounter rc (resultid, newresult, done, cntreset, pglobalreset, tm3_clk_v0); 
 endmodule




    
    
    
    
    
    
    
    
    

module resulttransmit (valid01, valid10, id01a, id01b, id01c, id10a, id10b, id10c, hit01a, hit01b, hit01c, hit10a, hit10b, hit10c, u01a, u01b, u01c, v01a, v01b, v01c, u10a, u10b, u10c, v10a, v10b, v10c, rgResultData, rgResultReady, rgResultSource, globalreset, clk);

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
    input[15:0] u01a; 
    input[15:0] u01b; 
    input[15:0] u01c; 
    input[15:0] v01a; 
    input[15:0] v01b; 
    input[15:0] v01c; 
    input[15:0] u10a; 
    input[15:0] u10b; 

    input[15:0] u10c; 
    input[15:0] v10a; 
    input[15:0] v10b; 
    input[15:0] v10c; 
    output[31:0] rgResultData; 
    reg[31:0] rgResultData;
    output rgResultReady; 
    reg rgResultReady;
    output[1:0] rgResultSource; 
    reg[1:0] rgResultSource;
    input globalreset; 
    input clk; 

    reg[3:0] state; 
    reg[3:0] next_state; 

    reg hit01al; 
    reg hit01bl; 
    reg hit01cl; 
    reg hit10al; 
    reg hit10bl; 
    reg hit10cl; 
    reg pending01; 
    reg pending10; 
    reg valid01d; 
    reg valid10d; 

reg[31:0] temp_rgResultData;
   reg temp_rgResultReady;
      reg[1:0] temp_rgResultSource;

    always @(posedge clk or posedge globalreset)
    begin
       if (globalreset == 1'b1)
       begin
          state <= 0 ; 
          pending01 <= 1'b0 ; 
          pending10 <= 1'b0 ; 
          rgResultData <= 32'b00000000000000000000000000000000 ; 
          rgResultSource <= 2'b00 ; 

          rgResultReady <= 1'b0 ; 
       end
       else
       begin
          valid01d <= valid01 ; 
          valid10d <= valid10 ; 
          if (valid01 == 1'b1)
          begin
             pending01 <= 1'b1 ; 
          end 
          if (valid01d == 1'b1)
          begin
             hit01al <= hit01a ; 
             hit01bl <= hit01b ; 
             hit01cl <= hit01c ; 
          end 
          if (valid10 == 1'b1)
          begin
             pending10 <= 1'b1 ; 
          end 
          if (valid10d == 1'b1)
          begin
             hit10al <= hit10a ; 
             hit10bl <= hit10b ; 
             hit10cl <= hit10c ; 
          end 
          state <= next_state ; 

          rgResultData <= temp_rgResultData;
          rgResultReady <= temp_rgResultReady;
          rgResultSource <= temp_rgResultSource;
       end 
    end 

    always @(state or pending01 or pending10)
    begin
       case (state)
          0 :
                   begin
                      if (pending01 == 1'b1)
                      begin
                         next_state = 1 ; 
                      end
                      else if (pending10 == 1'b1)
                      begin
                         next_state = 5 ; 
                      end
                      else

                      begin
                         next_state = 0 ; 
                      end 
                   end
          1 :
                   begin
                      next_state = 2 ; 
                         temp_rgResultData = {id01a, id01b} ; 
                         temp_rgResultReady = 1'b1 ; 
                         temp_rgResultSource = 2'b01 ; 
                   end
          2 :
                   begin
                      next_state = 3 ; 
                         temp_rgResultData = {13'b0000000000000, hit01al, hit01bl, hit01cl, id01c} ; 
                         temp_rgResultReady = 1'b0 ; 
                         temp_rgResultSource = 2'b01 ; 
                   end

          3 :
                   begin
                      next_state = 4 ; 
                         temp_rgResultData = {8'b00000000, u01a[15:8], u01b[15:8], u01c[15:8]} ; 
                         temp_rgResultReady = 1'b0 ; 
                         temp_rgResultSource = 2'b01 ; 
                   end
          4 :
                   begin
                      next_state = 0 ; 
                         temp_rgResultData = {8'b00000000, v01a[15:8], v01b[15:8], v01c[15:8]} ; 
                         temp_rgResultReady = 1'b0 ; 
                         temp_rgResultSource = 2'b01 ; 
                   end
          5 :
                   begin
                      next_state = 6 ; 
                         temp_rgResultData = {id10a, id10b} ; 
                         temp_rgResultReady = 1'b1 ; 
                         temp_rgResultSource = 2'b10 ; 
                   end
          6 :
                   begin
                      next_state = 7 ; 
                         temp_rgResultData = {13'b0000000000000, hit10al, hit10bl, hit10cl, id10c} ; 
                         temp_rgResultReady = 1'b0 ; 
                         temp_rgResultSource = 2'b10 ; 
                   end
          7 :
                   begin
                      next_state = 8 ; 
                         temp_rgResultData = {8'b00000000, u10a[15:8], u10b[15:8], u10c[15:8]} ; 
                         temp_rgResultReady = 1'b0 ; 
                         temp_rgResultSource = 2'b10 ; 
                   end
          8 :
                   begin
                      next_state = 0 ; 
                         temp_rgResultData = {8'b00000000, v10a[15:8], v10b[15:8], v10c[15:8]} ; 
                         temp_rgResultReady = 1'b0 ; 
                         temp_rgResultSource = 2'b10 ; 
                   end
	default:
		begin
			temp_rgResultReady = u01a || u01b || u01c || v01a || v01b || v01c || u10a || u10b || u10c || v10a || v10b || v10c;  
		end
       endcase 
    end 
 endmodule























  

module boundcontroller (raygroupout, raygroupwe, raygroupid, enablenear, raygroup, validraygroup, busy, triIDvalid, triID, wanttriID, l0reset, baseaddress, newdata, boundNodeIDout, resultID, hitmask, ldataready, lempty, llevel, lboundNodeID, lack, lhreset, addrind, addrindvalid, dataind, dataindvalid, tladdr, tladdrvalid, tldata, tldatavalid, t1in, t2in, t3in, u1in, u2in, u3in, v1in, v2in, v3in, id1in, id2in, id3in, hit1in, hit2in, hit3in, t1, t2, t3, u1, u2, u3, v1, v2, v3, id1, id2, id3, hit1, hit2, hit3, bcvalid, done, resetcnt, passCTSout, passCTSin, globalreset, clk, statepeek, debugsubcount, debugcount);

        output[1:0] raygroupout; 
        wire[1:0] raygroupout;
        output raygroupwe; 
        reg raygroupwe;
        output[1:0] raygroupid; 
        reg[1:0] raygroupid;
        output enablenear; 
        reg enablenear;
        input[1:0] raygroup; 
        input validraygroup; 
        output busy; 
        reg busy;
        reg temp_busy;
        output triIDvalid; 
        reg triIDvalid;
        output[15:0] triID; 
        reg[15:0] triID;
        input wanttriID; 
        output l0reset; 
        reg l0reset;
        output[1:0] baseaddress; 
        reg[1:0] baseaddress;
        input newdata; 
        output[9:0] boundNodeIDout; 
        reg[9:0] boundNodeIDout;
        input[1:0] resultID; 
        output[2:0] hitmask; 
        reg[2:0] hitmask;
        input ldataready; 
        input lempty; 
        input[1:0] llevel; 
        input[9:0] lboundNodeID; 
        output lack; 
        reg lack;
        output lhreset; 
        reg lhreset;
        output[9:0] addrind; 
        reg[9:0] addrind;
        output addrindvalid; 
        reg addrindvalid;
        input[31:0] dataind; 
        input dataindvalid; 
        output[17:0] tladdr; 
        reg[17:0] tladdr;
        output tladdrvalid; 
        reg tladdrvalid;
        input[63:0] tldata; 
        input tldatavalid; 
        input[31:0] t1in; 
        input[31:0] t2in; 
        input[31:0] t3in; 
        input[15:0] u1in; 
        input[15:0] u2in; 
        input[15:0] u3in; 
        input[15:0] v1in; 
        input[15:0] v2in; 
        input[15:0] v3in; 
        input[15:0] id1in; 
        input[15:0] id2in; 
        input[15:0] id3in; 
        input hit1in; 
        input hit2in; 
        input hit3in; 
        output[31:0] t1; 
		reg[31:0] t1;
        output[31:0] t2; 
        reg[31:0] t2;
        output[31:0] t3; 
        reg[31:0] t3;
        output[15:0] u1; 
        reg[15:0] u1;
        output[15:0] u2; 
        reg[15:0] u2;
        output[15:0] u3; 
        reg[15:0] u3;
        output[15:0] v1; 
        reg[15:0] v1;
        output[15:0] v2; 
        reg[15:0] v2;
        output[15:0] v3; 
        reg[15:0] v3;
        output[15:0] id1; 
        reg[15:0] id1;
        output[15:0] id2; 
        reg[15:0] id2;
        output[15:0] id3; 
        reg[15:0] id3;
        output hit1; 
        reg hit1;
        output hit2; 
        reg hit2;
        output hit3; 
        reg hit3;
        output bcvalid; 
        reg bcvalid;
        input[1:0] done; 
        output resetcnt; 
        reg resetcnt;
        output passCTSout; 
        reg passCTSout;
        input passCTSin; 
        input globalreset; 
        input clk; 
        output[4:0] statepeek; 
        reg[4:0] statepeek;
        output[1:0] debugsubcount; 
        wire[1:0] debugsubcount;
        output[13:0] debugcount; 
        wire[13:0] debugcount;

      reg[4:0] state; 
        reg[4:0] next_state; 
        reg cts; 
        reg[11:0] addr; 
        reg[11:0] startAddr; 
        reg[2:0] resetcount; 
        reg[1:0] raygroupoutl; 
        // Leaf Node Signals
        reg[13:0] count; 
        reg[63:0] triDatalatch; 
        reg[1:0] subcount; 
        reg[1:0] maskcount; 

reg[4:0] temp_statepeek;
reg [1:0]temp_raygroupoutl ;
reg temp_cts ;
reg temp_passCTSout ;
reg [2:0]temp_resetcount ;
reg temp_l0reset ;
reg [11:0]temp_addr ;
reg [11:0]temp_startAddr ;
reg [9:0]temp_boundNodeIDout ;
reg [1:0]temp_baseaddress ;
reg [2:0]temp_hitmask ;
reg temp_hit1 ;
reg temp_hit2 ;
reg temp_hit3 ;
reg temp_triIDvalid ;
reg [15:0]temp_triID ;
reg temp_lack ;
reg [9:0]temp_addrind ;
reg temp_addrindvalid ;
reg temp_tladdrvalid ;
reg [17:0]temp_tladdr ;
reg [13:0]temp_count ;
reg [1:0]temp_subcount ;
reg [1:0]temp_maskcount ;
reg [63:0]temp_triDatalatch ;
reg [31:0]temp_t1 ;
reg [15:0]temp_u1 ;
reg [15:0]temp_v1 ;
reg [15:0]temp_id1 ;
reg [31:0]temp_t2 ;
reg [15:0]temp_u2 ;
reg [15:0]temp_v2 ;
reg [15:0]temp_id2 ;
reg [31:0]temp_t3 ;
reg [15:0]temp_u3 ;
reg [15:0]temp_v3 ;
reg [15:0]temp_id3 ;

        assign debugsubcount = subcount ;
   assign debugcount = count ;
    assign raygroupout = (cts == 1'b1 & state != 8 & state != 19 & state != 1) ? raygroupoutl : 2'b00 ;

    always @(posedge clk or posedge globalreset)
    begin
       if (globalreset == 1'b1)
       begin
          state <= 0 ; 
          raygroupoutl <= 0;
          cts <= 1'b0 ; 
          passCTSout <= 1'b0 ; 
          addr <= 0;
          startAddr <= 0;
          boundNodeIDout <= 0;
          resetcount <= 0;
          hitmask <= 1;
          lack <= 1'b0 ; 
          baseaddress <= 0;
          l0reset <= 1'b0 ; 
          resetcnt <= 1'b0 ; 
          triIDvalid <= 1'b0 ; 
          triID <= 0;
          addrind <= 0;
          addrindvalid <= 1'b0 ; 
          tladdrvalid <= 1'b0 ; 
          tladdr <= 0;
          triDatalatch <= 0;
          maskcount <= 0;
          subcount <= 0;
          count <= 0;
          hit1 <= 1'b0 ; 
          hit2 <= 1'b0 ; 
          hit3 <= 1'b0 ; 
          t1 <= 0;
          t2 <= 0;
          t3 <= 0;
          u1 <= 0;
          u2 <= 0;
          u3 <= 0;
          v1 <= 0;
          v2 <= 0;
          v3 <= 0;
          id1 <= 0;
          id2 <= 0;
          id3 <= 0;
          busy <= 1'b0 ; 
       end
       else
       begin
          state <= next_state ; 
		  busy <= temp_busy;
          if ((done == 2'b00) | (state == 15 & newdata == 1'b1 & resultID == 2'b00))
          begin
             resetcnt <= 1'b1 ; 
          end
          else
          begin
             resetcnt <= 1'b0 ; 
          end 

statepeek <= temp_statepeek;
raygroupoutl <= temp_raygroupoutl ;
cts <= temp_cts ;
passCTSout <= temp_passCTSout ;
resetcount <= temp_resetcount ;
l0reset <= temp_l0reset ;
addr <= temp_addr ;
startAddr <= temp_startAddr ;
boundNodeIDout <= temp_boundNodeIDout ;
baseaddress <= temp_baseaddress ;
hitmask <= temp_hitmask ;
hit1 <= temp_hit1 ;
hit2 <= temp_hit2 ;
hit3 <= temp_hit3 ;
triIDvalid <= temp_triIDvalid ;
triID <= temp_triID ;
lack <= temp_lack ;
addrind <= temp_addrind ;
addrindvalid <= temp_addrindvalid ;
tladdr <= temp_tladdr ;
tladdrvalid <= temp_tladdrvalid ;
count <= temp_count ;
subcount <= temp_subcount ;
maskcount <= temp_maskcount ;
triDatalatch <= temp_triDatalatch ;
t1 <= temp_t1 ;
u1 <= temp_u1 ;
v1 <= temp_v1 ;
id1 <= temp_id1 ;
t2 <= temp_t2 ;
u2 <= temp_u2 ;
v2 <= temp_v2 ;
id2 <= temp_id2 ;
t3 <= temp_t3 ;
u3 <= temp_u3 ;
v3 <= temp_v3 ;
id3 <= temp_id3 ;
	end
end



    always @(state or validraygroup or cts or passCTSin or wanttriID or addr or 
             startAddr or done or ldataready or lempty or llevel or resetcount or 
             dataindvalid or tldatavalid or hit1in or hit2in or hit3in or hitmask or 
             resultID or newdata or subcount or count)
    begin
       case (state)
          0 :
                   begin

				       raygroupid = 0;
				       enablenear = 1'b0 ; 
				       raygroupwe = 1'b0 ; 
				       bcvalid = 1'b0 ; 

                      lhreset = 1'b1 ; 
                      if (validraygroup == 1'b1 & cts == 1'b1)
                      begin
                         next_state = 2 ; 
             				temp_busy = 1'b1 ; 
                      end
                      else if (validraygroup == 1'b1 & cts == 1'b0)
                      begin
                         next_state = 1 ; 
             				temp_busy = 1'b0 ; 
                      end
                      else if (validraygroup == 1'b0 & passCTSin == 1'b1 & cts == 1'b1)
                      begin
                         next_state = 1 ; 
             				temp_busy = 1'b0 ; 
                      end
                      else
                      begin
                         next_state = 0 ; 
             				temp_busy = 1'b0 ; 
                      end 

                      temp_statepeek = 5'b00001 ; 
//
                         temp_raygroupoutl = raygroup ; 
                         if (validraygroup == 1'b1 & cts == 1'b0)
                         begin
                            temp_cts = 1'b1 ; 
                            temp_passCTSout = 1'b1 ; 
                         end
                         else if (validraygroup == 1'b0 & cts == 1'b1 & passCTSin == 1'b1)
                         begin
                            temp_cts = 1'b0 ; 
                            temp_passCTSout = 1'b1 ; 
                         end 

                   end
          1 :
                   begin
                      if ((passCTSin == cts) & (cts == 1'b1))
                      begin
                         next_state = 2 ; 
             				temp_busy = 1'b1 ; 
                      end
                      else if (passCTSin == cts)
                      begin
                         next_state = 0 ; 
             				temp_busy = 1'b0 ; 
                      end
                      else
                      begin
                         next_state = 1 ; 
             				temp_busy = 1'b0 ; 
                      end 

                      temp_statepeek = 5'b00010 ; 
//
                         if (passCTSin == cts)
                         begin
                            temp_passCTSout = 1'b0 ; 
                         end 

                   end
          2 :
                   begin
                      if (wanttriID == 1'b1)
                      begin
                         next_state = 3 ; 
             				temp_busy = 1'b1 ; 
                      end
                      else
                      begin
                         next_state = 2 ; 
             				temp_busy = 1'b1 ; 
                      end 

                      temp_statepeek = 5'b00011 ; 
//
			            temp_resetcount = 3'b100 ; 
                         temp_l0reset = 1'b1 ; 
                         temp_addr = 0;
                         temp_startAddr = 0;
                         temp_boundNodeIDout = 0;
                         temp_baseaddress = 0;
                         temp_hitmask = 1;
                         temp_hit1 = 1'b0 ; 
                         temp_hit2 = 1'b0 ; 
                         temp_hit3 = 1'b0 ; 

                   end
          3 :
                   begin
                      if ((addr - startAddr >= 1) & (addr - startAddr != 49))
                      begin
                         raygroupid = 2'b00 ; 
                      end 
                      next_state = 4 ; 
             				temp_busy = 1'b1 ; 
                      if (resetcount == 5)
                      begin
			            raygroupwe = 1'b1 ;                                                         
                          end 
                          enablenear = 1'b1 ; 
                      temp_statepeek = 5'b00100 ; 
//
			            if ((addr - startAddr != 48) & (addr - startAddr != 49))
                         begin

                            temp_triIDvalid = 1'b1 ; 
                         end 
                         temp_triID = {4'b0000, addr} ; 

                    end
              4 :
                       begin
                          if (addr - startAddr == 49)
                          begin
                             next_state = 6 ; 
             				temp_busy = 1'b1 ; 
                          end
                          else
                          begin
                             next_state = 5 ; 
             				temp_busy = 1'b1 ; 
                          end 

                      temp_statepeek = 5'b00101 ;
                       end
              5 :
                       begin
                          next_state = 3 ; 
             				temp_busy = 1'b1 ; 

                      temp_statepeek = 5'b00111 ;
//
			            temp_addr = addr + 1 ; 
                         if (resetcount == 5)
                         begin
                            temp_resetcount = 3'b000 ; 
                         end
                         else
                         begin
                            temp_resetcount = resetcount + 1 ; 
                         end 

                       end
              6 :
                       begin
                          if (passCTSin == 1'b1 & cts == 1'b1)
                          begin             
                             next_state = 7;
             				temp_busy = 1'b1 ; 
                          end
                          else if (done == 2'b00 & cts == 1'b0)
                          begin
                             next_state = 8;
             				temp_busy = 1'b1 ; 
                          end
                          else if (done == 2'b00 & cts == 1'b1)
                          begin
                             next_state = 9;
             				temp_busy = 1'b1 ; 
                          end
                          else
                          begin
                             next_state = 6;
             				temp_busy = 1'b1 ; 
                          end 

                      temp_statepeek = 5'b01001 ;
//
			            if (passCTSin == 1'b1 & cts == 1'b1)
                         begin
                            temp_cts = 1'b0 ; 
                            temp_passCTSout = 1'b1 ; 
                         end
                         else if (done == 2'b00 & cts == 1'b0)
                         begin
                            temp_cts = 1'b1 ; 
                            temp_passCTSout = 1'b1 ; 
                         end 

                       end
              7 :
                       begin
                          if (passCTSin == 0)
                          begin
                             next_state = 6;
             				temp_busy = 1'b1 ; 
                          end
                          else
                          begin
                             next_state = 7;
             				temp_busy = 1'b1 ; 
                          end 

                      temp_statepeek = 5'b01001 ;
//
			            if (passCTSin == 1'b0)
                         begin
                            temp_passCTSout = 1'b0 ; 
                         end 

                       end
              8 :
                       begin
                          if (passCTSin == 1)
                          begin
                             next_state = 9;
             				temp_busy = 1'b1 ; 
                          end
                          else
                          begin
                             next_state = 8 ; 
             				temp_busy = 1'b1 ; 
                          end 

                      temp_statepeek = 5'b01010 ;
//
                  		if (passCTSin == 1'b1)
                         begin
                            temp_passCTSout = 1'b0 ; 
                         end 
                       end
              9 :
                       begin
                          if (lempty == 1'b1)
                          begin
                             next_state = 0 ; 
             				temp_busy = 1'b0 ; 
                             bcvalid = 1'b1 ; 
                          end
                          else if (ldataready == 1'b1 & llevel == 2'b10)
                          begin
                             next_state = 10 ; 
             				temp_busy = 1'b1 ; 
                          end
                          else if (ldataready == 1'b1 & wanttriID == 1'b1)
                          begin
                             next_state = 3 ; 
             				temp_busy = 1'b1 ; 
                          end
                          else
                          begin
                             next_state = 9 ; 
             				temp_busy = 1'b1 ; 
                          end 

                      temp_statepeek = 5'b01011 ;
//
			            temp_resetcount = 3'b100 ; 
                         temp_baseaddress = llevel + 1 ; 
                         // boundNodeIDout = (lBoundNodeID+1)(6 downto 0) & "000";
                         //boundNodeIDout = {(lboundNodeID + 1)[6:0], 3'b000} ; 
                         temp_boundNodeIDout = {lboundNodeID[6:0], 3'b000} ; 
                         //          temp_addr = (((lBoundNodeID+1)(7 downto 0) & "0000")+
                         //                   ((lBoundNodeID+1)(6 downto 0) & "00000")) (11 downto 0);
                         //temp_addr = (({(lboundNodeID + 1)[7:0], 4'b0000}) + ({(lboundNodeID + 1)[6:1], 5'b00000}))[11:0] ; 
                         temp_addr = (({lboundNodeID[7:0], 4'b0000}) + ({lboundNodeID[6:1], 5'b00000})); 
                         //          startaddr = (((lBoundNodeID+1)(7 downto 0) & "0000")+
                         //                        ((lBoundNodeID+1)(6 downto 0) & "00000")) (11 downto 0);
                         //startAddr = (({(lboundNodeID + 1), 4'b0000}) + ({(lboundNodeID + 1), 5'b00000})) ; 
                         temp_startAddr = (({lboundNodeID, 4'b0000}) + ({lboundNodeID, 5'b00000})) ; 
                         if (ldataready == 1'b1 & (wanttriID == 1'b1 | llevel == 2'b10))
                         begin
                            temp_lack = 1'b1 ; 
                            temp_l0reset = 1'b1 ; 
                         end 
                         if (ldataready == 1'b1 & llevel == 2'b10)
                         begin
                            temp_addrind = lboundNodeID - 72 ; 
                            temp_addrindvalid = 1'b1 ; 
                         end 

                       end
              10 :
                       begin
                          if (dataindvalid == 1'b1)
                          begin
                             next_state = 11 ; 
             				temp_busy = 1'b1 ; 
                          end
                          else
                          begin
                             next_state = 10 ; 
             				temp_busy = 1'b1 ; 
                          end 

                      temp_statepeek = 5'b01100 ;
//

			            temp_tladdr = dataind[17:0] ; 
                         temp_count = dataind[31:18] ; 
                         if (dataindvalid == 1'b1)
                         begin
                            temp_addrindvalid = 1'b0 ; 
                            temp_tladdrvalid = 1'b1 ; 
                         end 

                       end
              11 :
                       begin
                          if (count == 0 | count == 1)
                          begin
                             next_state = 9 ; 
             				temp_busy = 1'b1 ; 
                          end
                          else if (wanttriID == 1'b1 & tldatavalid == 1'b1)
                          begin
                             next_state = 12 ; 
             				temp_busy = 1'b1 ; 
                          end
                          else
                          begin
                             next_state = 11 ; 
             				temp_busy = 1'b1 ; 
                          end 

                      temp_statepeek = 5'b01101 ;
//

			            temp_triDatalatch = tldata ; 
                         temp_subcount = 2'b10 ; 
                         temp_maskcount = 2'b00 ; 
                         if ((wanttriID == 1'b1 & tldatavalid == 1'b1) | (count == 0 | count == 1))
                         begin
                            temp_tladdr = tladdr + 1 ; 
                            temp_tladdrvalid = 1'b0 ; 
                         end 

                       end
              12 :
                       begin
                          if (count != 0)
                          begin
                             next_state = 13 ; 
             				temp_busy = 1'b1 ; 
                          end
                          else
                          begin
                             next_state = 15 ; 
             				temp_busy = 1'b1 ; 
                          end 
                          if (subcount == 2'b01)
                          begin
                             raygroupid = 2'b00 ; 
                          end
                          else
                          begin
                             raygroupid = 2'b00 ; 
                          end 
                          enablenear = 1'b0 ; 
                          if (subcount == 2'b01 | count == 0)
                          begin
                             raygroupwe = 1'b1 ; 
                          end 

                      temp_statepeek = 5'b01110 ; 
//

			            if (maskcount == 2'b11)
                         begin
                            //            triID = triDataLatch(15 downto 0);
                            temp_triID = triDatalatch[15:0] ; 
                         end
                         else if (maskcount == 2'b10)
                         begin
                            //            triID = triDataLatch(31 downto 16);
                            temp_triID = triDatalatch[31:16] ; 
                         end
                         else if (maskcount == 2'b01)
                         begin
                            //            triID = triDataLatch(47 downto 32);
                            temp_triID = triDatalatch[47:32] ; 
                         end
                         else
                         begin
                            //            triID = triDataLatch(63 downto 48);
                            temp_triID = triDatalatch[63:48] ; 
                         end 
                         if (count != 0)
                         begin
                            temp_count = count - 1 ; 
                            if (count != 1)
                            begin
                               temp_triIDvalid = 1'b1 ; 
                            end 

                              if (maskcount == 2'b01)
                                begin                        
                                   temp_tladdrvalid = 1'b1 ;      
                                end         
                             end             

                       end
              13 :
                       begin
                          next_state = 14 ; 
             				temp_busy = 1'b1 ; 

                      temp_statepeek = 5'b01111 ; 
                       end
              14 :
                       begin
                          next_state = 12 ; 
             				temp_busy = 1'b1 ; 
                      temp_statepeek = 5'b10000 ; 
//

				            if (subcount != 0)
                             begin           
                                temp_subcount = subcount - 1 ;
                             end 
                             if (maskcount == 2'b11)
                             begin
                                temp_tladdr = tladdr + 1 ; 
                                temp_tladdrvalid = 1'b0 ; 
                                temp_triDatalatch = tldata ; 
                             end 
                             temp_maskcount = maskcount + 1 ; 

                       end
              15 :
                       begin
                          if ((newdata == 1'b0 | resultID != 2'b00) & cts == 1'b1 & passCTSin == 1'b1)
                          begin
                             next_state = 16 ; 
             				temp_busy = 1'b1 ; 
                          end
                          else if (newdata == 1'b1 & resultID == 2'b00)
                          begin
                             next_state = 18 ; 
             				temp_busy = 1'b1 ; 
                          end
                          else
                          begin
                             next_state = 15 ; 
             				temp_busy = 1'b1 ; 
                          end 
                      temp_statepeek = 5'b10001 ; 
//
				            temp_tladdr = 0;
                             temp_tladdrvalid = 0;
                             if ((newdata == 0) | (resultID < 2'b00) & (passCTSin == 1))
                             begin
                                temp_cts = 0;
                                temp_passCTSout = 1;
                             end 

						end
              16 :
                       begin
                          if (newdata == 1'b1 & resultID == 2'b00)
                          begin
                             next_state = 17 ; 
             				temp_busy = 1'b1 ; 
                          end
                          else if (passCTSin == 1'b0)
                          begin
                             next_state = 15 ; 
             				temp_busy = 1'b1 ; 
                          end
                          else
                          begin
                             next_state = 16 ; 
             				temp_busy = 1'b1 ; 
                          end 
                      temp_statepeek = 5'b10010 ; 
//
				            if ((passCTSin == 0) & ((newdata == 0) | (resultID == 1)))
                             begin
                                temp_passCTSout = 0;
                             end 

                       end
              17 :
                       begin
                          if (passCTSin == 1'b0)
                          begin
                             next_state = 18 ; 
             				temp_busy = 1'b1 ; 
                          end
                          else
                          begin
                             next_state = 17 ; 
             				temp_busy = 1'b1 ; 
                          end 
                      temp_statepeek = 5'b10011 ; 
//
			            if (passCTSin == 0)
                             begin
                                temp_passCTSout = 0;
                             end 


                       end
              18 :
                       begin
                          if (cts == 1'b0 & (((hitmask[0]) == 1'b1 & hit1in == 1'b0) | ((hitmask[1]) == 1'b1 & hit2in == 1'b0) | ((hitmask[2]) == 1'b1 & hit3in == 1'b0)))
                          begin
                             next_state = 19 ; 
             				temp_busy = 1'b1 ; 
                          end
                          else if (cts == 1'b1 & (((hitmask[0]) == 1'b1 & hit1in == 1'b0) | ((hitmask[1]) == 1'b1 & hit2in == 1'b0) | ((hitmask[2]) == 1'b1 & hit3in == 1'b0)))
                          begin
                             next_state = 9 ; 
             				temp_busy = 1'b1 ; 
                          end
                          else
                          begin
                             next_state = 0 ; 
             				temp_busy = 1'b0 ; 
                             bcvalid = 1'b1 ; 
                          end 
                      temp_statepeek = 5'b10100 ; 
//



                             if (hit1in == 1'b1 & (hitmask[0]) == 1'b1)
                             begin
                                temp_t1 = t1in;
                                temp_u1 = u1in;
                                temp_v1 = v1in;
                                temp_id1 = id1in;
                                temp_hit1 = 1'b1;
                                temp_hitmask[0] = 1'b0 ; 
                             end 
                             if (hit2in == 1'b1 & (hitmask[1]) == 1'b1)
                             begin
                                temp_t2 = t2in ; 
                                temp_u2 = u2in ; 
                                temp_v2 = v2in ; 
                                temp_id2 = id2in ; 
                                temp_hit2 = 1'b1 ; 
                                temp_hitmask[1] = 1'b0 ; 
                             end 
                             if (hit3in == 1'b1 & (hitmask[2]) == 1'b1)
                             begin
                                temp_t3 = t3in ; 
                                temp_u3 = u3in ; 
                                temp_v3 = v3in ; 
                                temp_id3 = id3in ; 
                                temp_hit3 = 1'b1 ; 
                                temp_hitmask[2] = 1'b0 ; 
                             end 
                             if (cts == 1'b0 & (((hitmask[0]) == 1'b1 & hit1in == 1'b0) | ((hitmask[1]) == 1'b1 & hit2in == 1'b0) | ((hitmask[2]) == 1'b1 & hit3in == 1'b0)))
                             begin
                                temp_passCTSout = 1'b1 ; 
                                temp_cts = 1'b1 ; 
                             end 

                       end
              19 :
                       begin
                          if (passCTSin == 1'b0)
                          begin
                             next_state = 19 ; 
             				temp_busy = 1'b1 ; 
                          end
                          else
                          begin
                             next_state = 9 ; 
             				temp_busy = 1'b1 ; 
                          end 
                      temp_statepeek = 5'b10101 ; 
//
				        if (passCTSin == 1'b1)
                         begin
                            temp_passCTSout = 1'b0 ; 
                         end 

                       end
           endcase 
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
    reg[0:0] count; 
    reg[0:0] temp_count; 

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

    
    
    
    
    
    
    
    

 module vblockramcontroller (want_addr, addr_ready, addrin, want_data, data_ready, datain, addr, addrvalid, data, datavalid, globalreset, clk);


    output want_addr; 
    reg want_addr;
    input addr_ready; 
    input[10 - 1:0] addrin; 
    output want_data; 
    reg want_data;
    input data_ready; 
    input[32 - 1:0] datain; 

    input[10 - 1:0] addr; 
    input addrvalid; 
    output[32 - 1:0] data; 
    reg[32 - 1:0] data;
    output datavalid; 
    reg datavalid;
    input globalreset; 
    input clk; 

    reg[2:0] state; 
    reg[2:0] next_state; 
    reg[10 - 1:0] waddr; 
    wire[10 - 1:0] saddr; 
    wire[32 - 1:0] dataout; 
    reg we; 
reg		[32 - 1:0]temp_data; 
reg     [10 - 1:0]temp_waddr ; 
reg     temp_datavalid;

    assign saddr = (state != 0) ? waddr : addr ;

    spramblock ramblock(we, saddr, datain, dataout, clk); 

    always @(posedge clk or posedge globalreset)
    begin
       if (globalreset == 1'b1)
       begin
          state <= 0 ; 
          waddr <= 0;
          data <= 0;
          datavalid <= 1'b0 ; 
       end
       else

       begin
          state <= next_state ; 
		data <= temp_data; 
       waddr <= temp_waddr ; 
       datavalid <= temp_datavalid;
       end 
    end 

    always @(state or addr_ready or data_ready or addrvalid or datavalid)

    begin
       case (state)
          0 :
                   begin
				       we = 1'b0 ; 
				       want_addr = 1'b1 ; 
				       want_data = 1'b0 ; 
                      if (addr_ready == 1'b1)
                      begin
                         next_state = 1 ; 
                      end

                      else if (addrvalid == 1'b1 & datavalid == 1'b0)
                      begin
                         next_state = 5 ; 
                      end
                      else
                      begin
                         next_state = 0 ; 
                      end 
          if (addr_ready == 1'b1)
                         begin
                            temp_waddr = addrin ; 
                         end 
                         if (addrvalid == 1'b0)
                         begin
                            temp_datavalid = 1'b0 ; 

                         end 

                   end
          5 :
                   begin
				       we = 1'b0 ; 
				       want_addr = 1'b1 ; 
				       want_data = 1'b0 ; 
                      next_state = 0 ; 

					temp_data = dataout ; 

                         temp_datavalid = 1'b1 ; 

                   end
          1 :
                   begin
				       we = 1'b0 ; 
                      want_addr = 1'b0 ; 
                      want_data = 1'b1 ; 
                      if (addr_ready == 1'b1)
                      begin
                         next_state = 1 ; 
                      end
                      else
                      begin
                         next_state = 2 ; 

                      end 
                   end
          2 :
                   begin
				       want_addr = 1'b1 ; 
                      want_data = 1'b1 ; 
                      if (addr_ready == 1'b1)
                      begin
                         next_state = 4 ; 
                      end
                      else if (data_ready == 1'b1)
                      begin
                         we = 1'b1 ; 

                         next_state = 3 ; 
                      end
                      else
                      begin
                         next_state = 2 ; 
                      end 
                         if (data_ready == 1'b1)
                         begin
                            temp_waddr = waddr + 1 ; 
                         end 

                   end
          3 :
                   begin
				       we = 1'b0 ; 
				       want_addr = 1'b1 ; 
				       want_data = 1'b0 ; 
                      if (data_ready == 1'b1)
                      begin
                         next_state = 3 ; 

                      end
                      else
                      begin
                         next_state = 2 ; 
                      end 
                   end
          4 :
                   begin
				       we = 1'b0 ; 
				       want_data = 1'b0 ; 
                      want_addr = 1'b0 ; 
                      if (addr_ready == 1'b1)
                      begin
                         next_state = 4 ; 

                      end
                      else
                      begin
                         next_state = 0 ; 
                      end 
                   end
       endcase 
    end 
 endmodule
 //-----------------------------------------------------
 // Single Ported Ram Modual w/Registered Output      --
 //  - Synpify should infer ram from the coding style --
 //  - Depth is the number of bits of address         --
 //    the true depth is 2**depth                     --
 //-----------------------------------------------------

    
    
 module spramblock (we, addr, datain, dataout, clk);

    input we; 
    input[10 - 1:0] addr; 
    input[32 - 1:0] datain; 
    output[32 - 1:0] dataout; 
    wire[32 - 1:0] dataout;
    input clk; 

    reg[10 - 1:0] raddr; 
//    reg[`width32 - 1:0] mem[2 ** `depth10 - 1:0]; 
    reg[32 - 1:0] mem_block1; 
    reg[32 - 1:0] mem_block2; 

    assign dataout = mem_block2 ;

    always @(posedge clk or posedge we)
    begin
       raddr <= addr ; 
       if (we == 1'b1)
       begin
          mem_block1 <= datain ; 
          mem_block2 <= mem_block1 ; 
       end  
    end 
 endmodule

    
    
    
    
    
    

 module sramcontroller (want_addr, addr_ready, addrin, want_data, data_ready, datain, addr, addrvalid, data, datavalid, tm3_sram_data_in, tm3_sram_data_out, tm3_sram_addr, tm3_sram_we, tm3_sram_oe, tm3_sram_adsp, globalreset, clk, statepeek);

    output want_addr; 
    reg want_addr;
    input addr_ready; 
    input[17:0] addrin; 
    output want_data; 
    reg want_data;
    input data_ready; 
    input[63:0] datain; 
    input[17:0] addr; 
    input addrvalid; 

    output[63:0] data; 
    reg[63:0] data;
    reg[63:0] temp_data;
    output datavalid; 
    reg datavalid;
    reg temp_datavalid;
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
    output[2:0] statepeek; 
    reg[2:0] statepeek;
    reg[2:0] temp_statepeek;

    reg[2:0] state; 
    reg[2:0] next_state; 
    reg[17:0] waddress; 
    reg[17:0] temp_waddress; 

    assign tm3_sram_data_out = tm3_sram_data_xhdl0;

    always @(posedge clk or posedge globalreset)
    begin
       if (globalreset == 1'b1)

       begin
          state <= 0 ; 
          waddress <= 0;
          data <= 0;
          datavalid <= 1'b0 ; 
       end
       else
       begin
          state <= next_state ; 
			statepeek <= temp_statepeek;
		data <=temp_data;
        datavalid <=temp_datavalid;
           waddress <= temp_waddress;
       end 
    end 

    always @(state or addr_ready or data_ready or waddress or datain or addrvalid or 
             datavalid or addr)
    begin
       case (state)
          0 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_data_xhdl0 = 0;
				       want_addr = 1'b1 ; 
				       want_data = 1'b0 ; 
                      if (addr_ready == 1'b1)
                      begin
                         next_state = 1 ; 
                      end
                      else if (addrvalid == 1'b1 & datavalid == 1'b0)
                      begin
                         next_state = 5 ; 

                         tm3_sram_addr = {1'b0, addr} ; 
                         tm3_sram_adsp = 1'b0 ; 
                         tm3_sram_oe = 2'b01 ; 
                      end

                      else
                      begin
                         next_state = 0 ; 
                      end 

                      temp_statepeek = 3'b001 ; 
                         if (addr_ready == 1'b1)
                         begin
                            temp_waddress = addrin ; 
                         end 
                         if (addrvalid == 1'b0)
                         begin
                            temp_datavalid = 1'b0 ; 
                         end 

                   end
          1 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b11 ; 
				       tm3_sram_adsp = 1'b1 ; 
				       tm3_sram_data_xhdl0 = 0;
				       tm3_sram_addr = 0;
                      want_addr = 1'b0 ; 
                      want_data = 1'b1 ; 
                      if (addr_ready == 1'b1)
                      begin
                         next_state = 1 ; 
                      end
                      else
                      begin
                         next_state = 2 ; 
                      end 

                      temp_statepeek = 3'b010 ; 
                   end
          2 :
                   begin
				       tm3_sram_oe = 2'b11 ; 
				       want_addr = 1'b1 ; 
                      want_data = 1'b1 ; 
                      tm3_sram_addr = {1'b0, waddress} ; 
                      tm3_sram_data_xhdl0 = datain ; 
                      if (addr_ready == 1'b1)
                      begin
                         next_state = 4 ; 
                      end
                      else if (data_ready == 1'b1)
                      begin

                         tm3_sram_we = 8'b00000000 ; 
                         tm3_sram_adsp = 1'b0 ; 
                         next_state = 3 ; 
                      end
                      else
                      begin
                         next_state = 2 ; 
                      end 
                      temp_statepeek = 3'b011 ; 
   if (data_ready == 1'b1)

                         begin
                            temp_waddress = waddress + 1 ; 
                         end 

                   end
          3 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b11 ; 
				       tm3_sram_adsp = 1'b1 ; 
				       tm3_sram_data_xhdl0 = 0;
				       tm3_sram_addr = 0;
				       want_addr = 1'b1 ; 
				       want_data = 1'b0 ; 
                      if (data_ready == 1'b1)

                      begin
                         next_state = 3 ; 
                      end
                      else
                      begin
                         next_state = 2 ; 
                      end 
                      temp_statepeek = 3'b100 ; 
                   end
          4 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b11 ; 
				       tm3_sram_adsp = 1'b1 ; 
				       tm3_sram_data_xhdl0 = 0;
				       tm3_sram_addr = 0;
				       want_data = 1'b0 ; 
                      want_addr = 1'b0 ; 
                      if (addr_ready == 1'b1)

                      begin
                         next_state = 4 ; 
                      end
                      else
                      begin
                         next_state = 0 ; 
                      end 
                      temp_statepeek = 3'b101 ; 
                   end
          5 :
                   begin
				       tm3_sram_we = 8'b11111111 ; 
				       tm3_sram_oe = 2'b11 ; 
				       tm3_sram_adsp = 1'b1 ; 
				       tm3_sram_data_xhdl0 = 0;
				       tm3_sram_addr = 0;
				       want_addr = 1'b1 ; 
				       want_data = 1'b0 ; 
                      next_state = 0 ; 
                      temp_statepeek = 3'b110 ; 
					   temp_data = tm3_sram_data_in ; 
                         temp_datavalid = 1'b1 ; 

                   end


       endcase 
    end 
 endmodule
    
    
    
    
    
    
    
    
    
    
    
    

module resultinterface (t1b, t2b, t3b, u1b, u2b, u3b, v1b, v2b, v3b, id1b, id2b, id3b, hit1b, hit2b, hit3b, resultID, newdata, resultready, resultdata, globalreset, clk);

    output[31:0] t1b; 
    reg[31:0] t1b;
    output[31:0] t2b; 
    reg[31:0] t2b;
    output[31:0] t3b; 
    reg[31:0] t3b;
    output[15:0] u1b; 
    reg[15:0] u1b;
    output[15:0] u2b; 
    reg[15:0] u2b;

    output[15:0] u3b; 
    reg[15:0] u3b;
    output[15:0] v1b; 
    reg[15:0] v1b;
    output[15:0] v2b; 
    reg[15:0] v2b;
    output[15:0] v3b; 
    reg[15:0] v3b;
    output[15:0] id1b; 
    reg[15:0] id1b;
    output[15:0] id2b; 
    reg[15:0] id2b;

    output[15:0] id3b; 
    reg[15:0] id3b;
    output hit1b; 
    reg hit1b;
    output hit2b; 
    reg hit2b;
    output hit3b; 
    reg hit3b;
    output[1:0] resultID; 
    reg[1:0] resultID;
    output newdata; 
    reg newdata;

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
    reg[1:0] temp_resultID;
    reg temp_newdata;

    input resultready; 
    input[31:0] resultdata; 
    input globalreset; 
    input clk; 

    reg[3:0] state; 
    reg[3:0] next_state; 

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
          resultID <= 0;
          newdata <= 1'b0 ; 
       end
       else
       begin
          state <= next_state ; 

						t1b <= temp_t1b;
          				newdata <= temp_newdata;
                         u1b <= temp_u1b;
                         v1b <= temp_v1b;
                         id1b <= temp_id1b;
                         hit1b <= temp_hit1b;
                         resultID <= temp_resultID;
                         t2b <= temp_t2b;
                         u2b <= temp_u2b;
                         id2b <= temp_id2b;
                         t3b <= temp_t3b;
                         u3b <= temp_u3b;
                         v3b <= temp_v3b;
                         id3b <= temp_id3b;
                         hit3b <= temp_hit3b;
                         v2b <= temp_v2b;
                         hit2b <= temp_hit2b;
       end 
    end 

    always @(state or resultready)
    begin
       case (state)
          0 :
                   begin

                      if (resultready == 1'b1)
                      begin
                         next_state = 1 ; 
                      end
                      else
                      begin
                         next_state = 0 ; 
                      end 
   				temp_newdata = 1'b0 ; 
                         if (resultready == 1'b1)
                         begin
                            temp_t1b = resultdata ; 
                         end 

                   end
          1 :
                   begin
                      next_state = 2 ; 
          				temp_newdata = 1'b0 ; 
                         temp_u1b = resultdata[31:16] ; 
                         temp_v1b = resultdata[15:0] ; 
                   end
          2 :
                   begin
                      next_state = 3 ; 
          				temp_newdata = 1'b0 ; 
                         temp_id1b = resultdata[15:0] ; 
                         temp_hit1b = resultdata[16] ; 
                         temp_resultID = resultdata[18:17] ; 
                   end
          3 :
                   begin

                      next_state = 4 ; 
          				temp_newdata = 1'b0 ; 
                         temp_t2b = resultdata ; 
                   end
          4 :
                   begin
                      next_state = 5 ; 
          				temp_newdata = 1'b0 ; 
                         temp_u2b = resultdata[31:16] ; 
                         temp_v2b = resultdata[15:0] ; 
                   end
          5 :
                   begin
                      next_state = 6 ; 
          				temp_newdata = 1'b0 ; 
                         temp_id2b = resultdata[15:0] ; 
                         temp_hit2b = resultdata[16] ; 
                   end
          6 :
                   begin

                      next_state = 7 ; 
          				temp_newdata = 1'b0 ; 
                         temp_t3b = resultdata ; 
                   end
          7 :
                   begin
                      next_state = 8 ; 
          				temp_newdata = 1'b0 ; 
                         temp_u3b = resultdata[31:16] ; 
                         temp_v3b = resultdata[15:0] ; 
                   end
          8 :
                   begin
                      next_state = 0 ; 
                         temp_id3b = resultdata[15:0] ; 
                         temp_hit3b = resultdata[16] ; 
                         temp_newdata = 1'b1 ; 
                   end
       endcase 
    end 

 endmodule
 module rayinterface (raygroup, raygroupwe, raygroupid, enablenear, rgData, rgAddr, rgWE, rgAddrValid, rgDone, raydata, rayaddr, raywe, globalreset, clk);

    input[1:0] raygroup; 
    input raygroupwe; 
    input[1:0] raygroupid; 
    input enablenear; 
    input[31:0] rgData; 
    input[3:0] rgAddr; 
    input[2:0] rgWE; 

    input rgAddrValid; 
    output rgDone; 
    reg rgDone;
    output[31:0] raydata; 
    reg[31:0] raydata;
    output[3:0] rayaddr; 
    reg[3:0] rayaddr;
    output[2:0] raywe; 
    reg[2:0] raywe;
    input globalreset; 
    input clk; 


    reg[31:0] rgDatal; 
    reg[3:0] rgAddrl; 
    reg[2:0] rgWEl; 
    reg rgAddrValidl; 

    always @(posedge clk or posedge globalreset)
    begin
       if (globalreset == 1'b1)
       begin
          raydata <= 0;
          rayaddr <= 0;
          raywe <= 0;

          rgDone <= 1'b0 ; 
          rgDatal <= 0;
          rgAddrl <= 0;
          rgWEl <= 0;
          rgAddrValidl <= 1'b0 ; 
       end
       else
       begin
          rgDatal <= rgData ; // Latch interchip signals
          rgAddrl <= rgAddr ; // To Meet Timing
          rgWEl <= rgWE ; 

          rgAddrValidl <= rgAddrValid ; 
          if (rgAddrValidl == 1'b0)
          begin
             rgDone <= 1'b0 ; 
          end 
          else if (raygroupwe == 1'b1)
          begin
             raydata[0] <= enablenear ; 
             raydata[31:1] <= 0;
             raywe <= 3'b111 ; 
             rayaddr <= {raygroupid, raygroup} ; 
          end
          else if (rgAddrValidl == 1'b1 & rgDone == 1'b0)
          begin
             raydata <= rgDatal ; 
             raywe <= rgWEl ; 
             rayaddr <= rgAddrl ; 
             rgDone <= 1'b1 ; 
          end 
		  else
		  begin
             raywe <= 0;
		  end
       end 
    end 
 endmodule




module sortedstack (keyin, datain, write, reset, peekdata, globalreset, clk);

    input[32 - 1:0] keyin; 
    input[13 - 1:0] datain; 
    input write; 
    input reset; 
    output[13 * 8 - 1:0] peekdata; 

    wire[13 * 8 - 1:0] peekdata;
    wire big_reset; 
    input globalreset; 
    input clk; 

    reg[32 - 1:0] key0; 
    reg[32 - 1:0] key1; 
    reg[32 - 1:0] key2; 
    reg[32 - 1:0] key3; 
    reg[32 - 1:0] key4; 
    reg[32 - 1:0] key5; 
    reg[32 - 1:0] key6; 
    reg[32 - 1:0] key7; 
    reg[13 - 1:0] data0; 
    reg[13 - 1:0] data1; 
    reg[13 - 1:0] data2; 
    reg[13 - 1:0] data3; 
    reg[13 - 1:0] data4; 
    reg[13 - 1:0] data5; 
    reg[13 - 1:0] data6; 
    reg[13 - 1:0] data7; 
    reg full0; 
    reg full1; 
    reg full2; 
    reg full3; 
    reg full4; 
    reg full5; 
    reg full6; 
    reg full7; 
    reg[2:0] location; 

    assign peekdata[(0 + 1) * (13) - 1:0 * (13)] = ((full0) == 1'b1) ? data0 : 0;
    assign peekdata[(1 + 1) * (13) - 1:1 * (13)] = ((full1) == 1'b1) ? data1 : 0;
    assign peekdata[(2 + 1) * (13) - 1:2 * (13)] = ((full2) == 1'b1) ? data2 : 0;
    assign peekdata[(3 + 1) * (13) - 1:3 * (13)] = ((full3) == 1'b1) ? data3 : 0;
    assign peekdata[(4 + 1) * (13) - 1:4 * (13)] = ((full4) == 1'b1) ? data4 : 0;
    assign peekdata[(5 + 1) * (13) - 1:5 * (13)] = ((full5) == 1'b1) ? data5 : 0;
    assign peekdata[(6 + 1) * (13) - 1:6 * (13)] = ((full6) == 1'b1) ? data6 : 0;
    assign peekdata[(7 + 1) * (13) - 1:7 * (13)] = ((full7) == 1'b1) ? data7 : 0;

    // Select the proper insertion point
    always @(keyin or key0 or key1 or key2 or key3 or key4 or key5 or key6 or key7 or full0 or full1 or full2 or full3 or full4 or full5 or full6 or full7)
    begin
		
/* PAJ -- changed for loops */
             if ((keyin < key0) | ((full0) == 1'b0))
             begin
                location = 0 ; 
             end 
             else if ((keyin < key1) | ((full1) == 1'b0))
             begin
                location = 1 ; 
             end 
             else if ((keyin < key2) | ((full2) == 1'b0))
             begin
                location = 2 ; 
             end 
             else if ((keyin < key3) | ((full3) == 1'b0))
             begin
                location = 3 ; 
             end 
             else if ((keyin < key4) | ((full4) == 1'b0))
             begin
                location = 4 ; 
             end 
             else if ((keyin < key5) | ((full5) == 1'b0))
             begin
                location = 5 ; 
             end 
             else if ((keyin < key6) | ((full6) == 1'b0))
             begin
                location = 6 ; 
             end 
			 else
			 begin
				location = 7;
			 end
    end 

	assign big_reset = globalreset | reset;
    always @(posedge clk or posedge big_reset)
    begin
		if (big_reset == 1'b1)
       begin
                full0 <= 1'b0 ; 
                key0 <= 0;
                data0 <= 0;
                full1 <= 1'b0 ; 
                key1 <= 0;
                data1 <= 0;
                full2 <= 1'b0 ; 
                key2 <= 0;
                data2 <= 0;
                full3 <= 1'b0 ; 
                key3 <= 0;
                data3 <= 0;
                full4 <= 1'b0 ; 
                key4 <= 0;
                data4 <= 0;
                full5 <= 1'b0 ; 
                key5 <= 0;
                data5 <= 0;
                full6 <= 1'b0 ; 
                key6 <= 0;
                data6 <= 0;
                full7 <= 1'b0 ; 
                key7 <= 0;
                data7 <= 0;
       end
       else
       begin
          if (write == 1'b1)
          begin
			if (location == 0)
			begin
				key0 <= keyin;
				data0 <= datain;
				full0 <= 1'b1;
				key1 <= key0;
				data1 <= data0;
				full1 <= full0;
				key2 <= key1;
				data2 <= data1;
				full2 <= full1;
				key3 <= key2;
				data3 <= data2;
				full3 <= full2;
				key4 <= key3;
				data4 <= data3;
				full4 <= full3;
				key5 <= key4;
				data5 <= data4;
				full5 <= full4;
				key6 <= key5;
				data6 <= data5;
				full6 <= full5;
				key7 <= key6;
				data7 <= data6;
				full7 <= full6;
			end
			else if (location == 1)
			begin
				key1 <= keyin;
				data1 <= datain;
				full1 <= 1'b1;
				key2 <= key1;
				data2 <= data1;
				full2 <= full1;
				key3 <= key2;
				data3 <= data2;
				full3 <= full2;
				key4 <= key3;
				data4 <= data3;
				full4 <= full3;
				key5 <= key4;
				data5 <= data4;
				full5 <= full4;
				key6 <= key5;
				data6 <= data5;
				full6 <= full5;
				key7 <= key6;
				data7 <= data6;
				full7 <= full6;
			end
			else if (location == 2)
			begin
				key2 <= keyin;
				data2 <= datain;
				full2 <= 1'b1;
				key3 <= key2;
				data3 <= data2;
				full3 <= full2;
				key4 <= key3;
				data4 <= data3;
				full4 <= full3;
				key5 <= key4;
				data5 <= data4;
				full5 <= full4;
				key6 <= key5;
				data6 <= data5;
				full6 <= full5;
				key7 <= key6;
				data7 <= data6;
				full7 <= full6;
			end
			else if (location == 2)
			begin
				key3 <= keyin;
				data3 <= datain;
				full3 <= 1'b1;
				data4 <= data3;
				full4 <= full3;
				key5 <= key4;
				data5 <= data4;
				full5 <= full4;
				key6 <= key5;
				data6 <= data5;
				full6 <= full5;
				key7 <= key6;
				data7 <= data6;
				full7 <= full6;
			end
			else if (location == 4)
			begin
				key4 <= keyin;
				data4 <= datain;
				full4 <= 1'b1;
				key5 <= key4;
				data5 <= data4;
				full5 <= full4;
				key6 <= key5;
				data6 <= data5;
				full6 <= full5;
				key7 <= key6;
				data7 <= data6;
				full7 <= full6;
			end
			else if (location == 5)
			begin
				key5 <= keyin;
				data5 <= datain;
				full5 <= 1'b1;
				key6 <= key5;
				data6 <= data5;
				full6 <= full5;
				key7 <= key6;
				data7 <= data6;
				full7 <= full6;
			end
			else if (location == 6)
			begin
				key6 <= keyin;
				data6 <= datain;
				full6 <= 1'b1;
				key7 <= key6;
				data7 <= data6;
				full7 <= full6;
			end
			else if (location == 7)
			begin
				key7 <= keyin;
				data7 <= datain;
				full7 <= 1'b1;
			end
		end
     end 
end
endmodule




module listhandler (dataarrayin, commit, hitmask, ack, boundnodeID, level, empty, dataready, reset, globalreset, clk, peekoffset0, peekoffset1, peekoffset2, peekhit, peekstate);

    input[8 * 13 - 1:0] dataarrayin; 
    input commit; 
    input[2:0] hitmask; 
    input ack; 
    output[9:0] boundnodeID; 
    wire[9:0] boundnodeID;
    output[1:0] level; 
    wire[1:0] level;

    output empty; 
    wire empty;
    output dataready; 
    wire dataready;
    input reset; 
    input globalreset; 
    input clk; 
    output[2:0] peekoffset0; 
    wire[2:0] peekoffset0;
    output[2:0] peekoffset1; 
    wire[2:0] peekoffset1;
    output[2:0] peekoffset2; 

    wire[2:0] peekoffset2;
    output peekhit; 
    wire peekhit;
    output[1:0] peekstate; 
    reg[1:0] peekstate;
    reg[1:0] temp_peekstate;

    reg[1:0] next_state; 
    reg[1:0] state; 
    reg[1:0] readlevel; 

    reg[1:0] writelevel; 
    reg[2:0] offset0; 
    reg[2:0] offset1; 
    reg[2:0] offset2; 
    reg[4:0] address; 
    reg we; 
    reg[12:0] datain; 
    wire[12:0] dataout; 
    reg[2:0] lvempty; 
    reg busy; 
    reg temp_busy; 
    reg[2:0] temp_lvempty; 
    reg[1:0] temp_readlevel; 
    reg[1:0] temp_writelevel; 
    reg[2:0] temp_offset0; 
    reg[2:0] temp_offset1; 
    reg[2:0] temp_offset2; 

    // Debug Stuff

    assign peekoffset0 = offset0 ;
    assign peekoffset1 = offset1 ;
    assign peekoffset2 = offset2 ;
    assign peekhit = ((datain[10]) == 1'b1 | (datain[11]) == 1'b1 | (datain[12]) == 1'b1) ? 1'b1 : 1'b0 ;

    // Real Code

    spram  ram(we, dataout, datain, clk); 

    assign level = readlevel ;
    assign boundnodeID = dataout[9:0] ;

    assign empty = (lvempty == 3'b111 & busy == 1'b0) ? 1'b1 : 1'b0 ;
    assign dataready = ((((dataout[10]) == 1'b1 & (hitmask[0]) == 1'b1) | ((dataout[11]) == 1'b1 & (hitmask[1]) == 1'b1) | ((dataout[12]) == 1'b1 & (hitmask[2]) == 1'b1)) & (empty == 1'b0) & (busy == 1'b0)) ? 1'b1 : 1'b0 ;

    always @(offset0 or offset1 or offset2 or address)
    begin
	    address[4:3] = readlevel ;

       if (address[4:3] == 2'b00)
       begin
          address[2:0] = offset0 ; 
       end
       else if (address[4:3] == 2'b01)
       begin

          address[2:0] = offset1 ; 
       end
       else if (address[4:3] == 2'b10)
       begin
          address[2:0] = offset2 ; 
       end
       else
       begin
          address[2:0] = 0;
       end 
    end 

    always @(posedge clk or posedge globalreset)
    begin
       if (globalreset == 1'b1)
       begin
          state <= 0 ; 
          lvempty <= 1;
          busy <= 1'b0 ; 
          readlevel <= 2'b00 ; 
          writelevel <= 2'b00 ; 
          offset0 <= 3'b000 ; 
          offset1 <= 3'b000 ; 
          offset2 <= 3'b000 ; 

       end
       else
       begin
          state <= next_state ; 
          peekstate <= temp_peekstate ; 
		busy <= temp_busy;
        lvempty <= temp_lvempty;
       readlevel <= temp_readlevel;
       writelevel <= temp_writelevel;
       offset0 <= temp_offset0;
       offset1 <= temp_offset1;
       offset2 <= temp_offset2;

       end 
    end 

    always @(state or commit or ack or address or dataarrayin or reset or dataready or 
             empty)
    begin

       case (state)
          0 :
                   begin
				       we = 1'b0 ; 
				       datain = 0;
                      if (reset == 1'b1)
                      begin
                         next_state = 0 ; 
                      end
                      else if (commit == 1'b1)
                      begin
                         next_state = 1 ; 
                      end
                      else if ((ack == 1'b1) | (dataready == 1'b0 & empty == 1'b0))

                      begin
                         next_state = 2 ; 
                      end
                      else
                      begin
                         next_state = 0 ; 
                      end 
                      temp_peekstate = 2'b01 ; 

                         if (reset == 1'b1)
                         begin
                            temp_busy = 1'b0 ; 
                            temp_lvempty = 1;
                            temp_readlevel = 2'b00 ; 

                            temp_writelevel = 2'b00 ; 
                            temp_offset0 = 3'b000 ; 
                            temp_offset1 = 3'b000 ; 
                            temp_offset2 = 3'b000 ; 
                         end
                         else if (commit == 1'b1)
                         begin
                            temp_busy = 1'b1 ; 
                            if (writelevel == 2'b00)
                            begin
                               temp_offset0 = 3'b000 ; 
                            end

                            else if (writelevel == 2'b01)
                            begin
                               temp_offset1 = 3'b000 ; 
                            end
							else if (writelevel == 2'b10)
                            begin
                               temp_offset2 = 3'b000 ; 
                            end 
                            temp_readlevel = writelevel ; 
                         end

                         else if (ack == 1'b1)
                         begin
                            temp_writelevel = readlevel + 1 ; 
                            temp_busy = 1'b1 ; // This will ensure that align skips one
                         end 

                   end
          1 :
                   begin
/* PAJ -- Unrolled loop */
						if (3'd0 == address[2:0])
						begin
   	                     datain = dataarrayin[13 - 1:0] ; 
						end
						else if (3'd1 == address[2:0])
						begin
   	                     datain = dataarrayin[(2) * 13 - 1:1 * 13] ; 
						end
						else if (3'd2 == address[2:0])
						begin
   	                     datain = dataarrayin[(3) * 13 - 1:2 * 13] ; 
						end
						else if (3'd3 == address[2:0])
						begin
   	                     datain = dataarrayin[(4) * 13 - 1:3 * 13] ; 
						end
						else if (3'd4 == address[2:0])
						begin
   	                     datain = dataarrayin[(5) * 13 - 1:4 * 13] ; 
						end
						else if (3'd5 == address[2:0])
						begin
   	                     datain = dataarrayin[(6) * 13 - 1:5 * 13] ; 
						end
						else if (3'd6 == address[2:0])
						begin
   	                     datain = dataarrayin[(7) * 13 - 1:6 * 13] ; 
						end
						else if (3'd7 == address[2:0])
						begin
   	                     datain = dataarrayin[(8) * 13 - 1:7 * 13] ; 
						end


                      we = 1'b1 ; 
                      if (address[2:0] == 3'b111)
                      begin
                         next_state = 2 ; 

                      end
                      else
                      begin
                         next_state = 1 ; 
                      end 
                      temp_peekstate = 2'b10 ; 

                         if (readlevel == 2'b00)
                         begin
                            temp_offset0 = offset0 + 1 ; 
                         end

                         else if (readlevel == 2'b01)
                         begin
                            temp_offset1 = offset1 + 1 ; 
                         end
                         else if (readlevel == 2'b10)
                         begin
                            temp_offset2 = offset2 + 1 ; 
                         end 
                         if (address[2:0] == 3'b111)
                         begin
                            temp_busy = 1'b0 ; 
                         end 

                         if ((datain[10]) == 1'b1 | (datain[11]) == 1'b1 | (datain[12]) == 1'b1)
                         begin
                            if (readlevel == 2'b00)
                            begin
                               temp_lvempty[0] = 1'b0 ; 
                            end
                            else if (readlevel == 2'b01)
                            begin
                               temp_lvempty[1] = 1'b0 ; 
                            end
                            else if (readlevel == 2'b10)
                            begin

                               temp_lvempty[2] = 1'b0 ; 
                            end 
                         end 

                   end
          2 :
	begin
		if (empty == 1'b0 & dataready == 1'b0)
		begin
			next_state = 2 ; 
		end
		else
			next_state = 0 ; 
		
		temp_peekstate = 2'b11 ; 
		temp_busy = 1'b0 ; 
		if (empty == 1'b0 & dataready == 1'b0)
		begin
			if (readlevel == 2'b00)
			begin
				if (offset0 == 3'b111)
				begin
					temp_lvempty[0] = 1'b1 ; 
				end
				else
				begin
					temp_offset0 = offset0 + 1 ; 
				end 
			end
			else if (readlevel == 2'b01)
			begin
				if (offset1 == 3'b111)
				begin
					temp_lvempty[1] = 1'b1 ; 
					temp_readlevel = 2'b00 ; 
				end
				else
				begin
					temp_offset1 = offset1 + 1 ; 
				end 
			end
			else if (readlevel == 2'b10)
			begin
				if (offset2 == 3'b111)
				begin
					temp_lvempty[2] = 1'b1 ; 
					if ((lvempty[1]) == 1'b1)
					begin
						temp_readlevel = 2'b00 ; 
					end
					else
					begin
						temp_readlevel = 2'b01 ; 
					end
				end
				else
				begin
					temp_offset2 = offset2 + 1 ; 
				end 
			end
		end
	end
	endcase
end
endmodule


    
 module spram (we, dataout, datain, clk);

    input we; 
    output[13 - 1:0] dataout; 
    wire[13 - 1:0] dataout;
    input[13 - 1:0] datain; 
    input clk; 
    reg[13 - 1:0] temp_reg; 

    reg[13 - 1:0] mem1; 
    reg[13 - 1:0] mem2; 

    assign dataout = mem2 ;

    always @(posedge clk or posedge we)
    begin
		temp_reg <= 0;
       if (we == 1'b1)
       begin
          mem1 <= datain + temp_reg; 
          mem2 <= mem1;
       end  
    end 
 endmodule
 module resultcounter (resultID, newresult, done, reset, globalreset, clk);

    input[1:0] resultID; 
    input newresult; 
    output[1:0] done; 
    wire[1:0] done;
    input reset; 
    input globalreset; 
    input clk; 

	wire big_reset;

    reg[3:0] count; 
    reg[1:0] curr; 

    assign done = (count == 0) ? curr : 2'b00 ;
	assign big_reset = globalreset | reset;

    always @(posedge clk or posedge big_reset)
    begin
		if (big_reset == 1'b1)
       begin
          count <= 4'b1000 ; 
          curr <= 0;
       end
       else
       begin
          if ((resultID != 0) & (newresult == 1'b1) & (count != 0))
          begin
             count <= count - 1 ; 
             curr <= resultID ; 
          end 
       end 
    end 
 endmodule

