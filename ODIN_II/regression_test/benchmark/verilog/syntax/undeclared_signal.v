
module top(
  clk
);
input clk;

testmod x (
  .clk(clk),
  .reset(1'b0),
  .a()
);

testmod y (
  clk, 
  1'b0,

);

endmodule

module  testmod (
  clk,
  reset,
  a
);
input reset;
input clk;
input a;

reg q;

always @(posedge clk)
  q <= ~reset && a;

endmodule
