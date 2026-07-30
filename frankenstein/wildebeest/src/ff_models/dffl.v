module dffl(D,clk,L,Q);
input D;      
input clk;   
input L;      
output reg Q;     

always @(posedge clk) 

begin
	if (L == 1'b0) begin // sync. reset active low
            Q <= 1'b0;
	end 
	else begin
            Q <= D; 
        end
end 

endmodule 
