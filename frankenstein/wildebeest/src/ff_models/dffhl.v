module dffhl(D,clk,H,L,Q);
input D;      
input clk;   
input H;      
input L;      
output reg Q;     

always @(posedge clk) 

begin
	if (L == 1'b0) begin // sync. reset active low
            Q <= 1'b0;
	end 
	else if (H == 1'b0) begin // sync. set active low
            Q <= 1'b1;
	end
	else begin
            Q <= D; 
        end
end 

endmodule 
