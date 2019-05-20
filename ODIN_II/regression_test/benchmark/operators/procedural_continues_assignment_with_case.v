module dflop (RST, SET, STATE, CLOCK, DATA_IN); 
      input RST; 
      input SET; 
      input CLOCK; 
      input DATA_IN; 
      output STATE; 
 
      reg STATE; 
 
always @ ( RST or SET ) // block b1 
case ({RST,SET}) 
      2'b00:     assign STATE = 1'b 0; 
      2'b01:     assign STATE = 1'b 0; 
      2'b10:     assign STATE = 1'b 1; 
      2'b11:     deassign STATE; 
endcase 
 
always @ ( posedge CLOCK ) // block b2 
begin 
      STATE = DATA_IN; 
end 
 
endmodule 