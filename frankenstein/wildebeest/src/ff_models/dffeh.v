module dffeh(D,clk,E,H,Q);
input D;      
input clk;   
input E;     
input H;      
output reg Q;     

always @(posedge clk) 

begin
	if (H == 1'b0) begin // sync. set active low
            Q <= 1'b1;
	end
	else if (E == 1'b1) begin
            Q <= D; 
        end
end 

endmodule 
