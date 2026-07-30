module dffrs(D,clk,R,S,Q);
input D;      // Data input 
input clk;    // clock input 
input R;      // reset input 
input S;      // set input 
output reg Q;     // output Q 

always @(posedge clk or negedge R or negedge S) 

begin
	if (R == 1'b0) begin // reset active low
            Q <= 1'b0;
        end
	else if (S == 1'b0) begin // set active low
            Q <= 1'b1;
        end
	else begin
            Q <= D; 
        end
end 
endmodule 
