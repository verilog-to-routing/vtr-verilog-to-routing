`timescale 1ns / 1ps `default_nettype none

module time_counter #(
    parameter MOD_VALUE = 1000000
) (
    input wire logic clk,
    reset,
    increment,
    output logic rolling_over,
    output logic [23:0] count = 0
);

  always_ff @(posedge clk) begin
    if (reset) count <= 0;
    else if (increment) begin
      if (rolling_over) count <= 0;
      else count <= count + 1'b1;
    end

  end

  always_comb begin
    if (increment && (count == MOD_VALUE - 1)) rolling_over = 1'b1;
    else rolling_over = 1'b0;
  end

endmodule
