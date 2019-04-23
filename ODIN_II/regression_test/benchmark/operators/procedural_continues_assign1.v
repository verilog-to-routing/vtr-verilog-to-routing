module ff(D,clk,sync_reset,Q);
input D;
input clk; 
input sync_reset; 
output Q; 
reg Q; 
always @(posedge clk) 
begin
 if(sync_reset==1'b1)
 assign Q = 1'b0; 
 else 
  Q = D; 
end 
endmodule 
