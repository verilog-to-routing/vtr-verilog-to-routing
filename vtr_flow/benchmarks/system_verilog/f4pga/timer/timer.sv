`timescale 1ns / 1ps `default_nettype none

module timer (
    input wire logic clk,
    reset,
    run,
    output logic [3:0] digit0,
    digit1,
    digit2,
    digit3
);

  logic inc0, inc1, inc2, inc3, inc4;

  logic [23:0] timerCount;

  modify_count #(
      .MOD_VALUE(10)
  ) M0 (
      .clk(clk),
      .reset(reset),
      .increment(inc0),
      .rolling_over(inc1),
      .count(digit0)
  );
  modify_count #(
      .MOD_VALUE(10)
  ) M1 (
      .clk(clk),
      .reset(reset),
      .increment(inc1),
      .rolling_over(inc2),
      .count(digit1)
  );
  modify_count #(
      .MOD_VALUE(10)
  ) M2 (
      .clk(clk),
      .reset(reset),
      .increment(inc2),
      .rolling_over(inc3),
      .count(digit2)
  );
  modify_count #(
      .MOD_VALUE(6)
  ) M3 (
      .clk(clk),
      .reset(reset),
      .increment(inc3),
      .rolling_over(inc4),
      .count(digit3)
  );

  time_counter #(
      .MOD_VALUE(1000000)
  ) T0 (
      .clk(clk),
      .reset(reset),
      .increment(run),
      .rolling_over(inc0),
      .count(timerCount)
  );
endmodule
