module assig (RST, STATE, CLOCK, DATA_IN); 
   input RST; 
   input CLOCK; 
   input  DATA_IN; 
   output  STATE; 
 
  reg  STATE; 
 
always @ ( RST ) 
   if( RST ) 
   begin 
    assign STATE = 1'b 0; 
   end else 
   begin 
     deassign STATE; 
   end 
  
always @ ( posedge CLOCK ) 
   begin 
     STATE = DATA_IN; 
  end cal
 
endmodule 
