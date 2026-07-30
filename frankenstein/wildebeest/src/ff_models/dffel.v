module dffel(D,clk,E,L,Q);
input D;      
input clk;   
input E;     
input L;      
output reg Q;     

always @(posedge clk) 

begin
	if (L == 1'b0) begin // sync. reset active low
            Q <= 1'b0;
	end 
	else if (E == 1'b1) begin
            Q <= D; 
        end
end 

endmodule 
