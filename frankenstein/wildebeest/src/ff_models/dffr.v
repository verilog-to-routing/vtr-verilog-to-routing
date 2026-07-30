module dffr(D,clk,R,Q);
input D;      // Data input 
input clk;    // clock input 
input R;      // reset input 
output reg Q;     // output Q 

always @(posedge clk or negedge R) 

begin
	if (R == 1'b0) begin // reset active low
            Q <= 1'b0;
	end 
	else begin
            Q <= D; 
        end
end 
endmodule 
