module dffer(D,clk,E,R,Q);
input D;      // Data input 
input clk;    // clock input 
input E;      // enable 
input R;      // reset input 
output reg Q;     // output Q 

always @(posedge clk or negedge R) 

begin
	if (R == 1'b0) begin // reset active low
            Q <= 1'b0;
	end 
	else if (E == 1'b1) begin
            Q <= D; 
        end
end 
endmodule 
