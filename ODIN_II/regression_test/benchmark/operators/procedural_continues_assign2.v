
`timescale 1ns/100ps
module FlipflopAssign (input reset, din, clk, output qout);
reg qout;
always @ (reset) begin
if (reset) 
assign qout <= 0;
else 
deassign qout;
end
always @ (posedge clk) begin
qout <= din;
end
endmodule
