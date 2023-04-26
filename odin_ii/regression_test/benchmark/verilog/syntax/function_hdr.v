module simple_function (
clk,
reset,
a,
b,
out,
clk_out
);

input clk;
input reset;
input [1:0] a,b;

integer test = 1;
integer output_int;

output [2:0] out;
output clk_out;

assign clk_out = clk;

always @(posedge clk)
begin
output_int = func_out(a,b,reset, test);
end

assign out = output_int;

`include "function_hdr.vh"

endmodule