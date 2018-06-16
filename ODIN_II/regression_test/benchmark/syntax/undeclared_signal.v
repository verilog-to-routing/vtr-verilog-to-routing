/* Odin will hot replace undeclared signal module instanciation with a dummy wire */

module top(
  clk
);
input clk;

testmod x (
  .clk(clk),
  .reset(1'b0),
  .a(DoesNotExist)
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
