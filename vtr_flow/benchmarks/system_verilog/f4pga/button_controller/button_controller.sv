`timescale 1ns / 1ps `default_nettype none

module top (
    input wire logic clk,
    btnu,
    btnc,
    output logic [3:0] anode,
    output logic [7:0] segment
);


  logic sync;
  logic syncToDebounce;
  logic debounceToOneShot;
  logic f1, f2;
  logic f3, f4;
  logic oneShotToCounter;
  logic [7:0] counterToSevenSegment;
  logic [7:0] counterToSevenSegment2;
  logic oneShotToCounter2;
  logic s0, s1;
  debounce d0 (
      .clk(clk),
      .reset(btnu),
      .noisy(syncToDebounce),
      .debounced(debounceToOneShot)
  );

  assign oneShotToCounter  = f1 && ~f2;

  assign oneShotToCounter2 = f3 && ~f4;

  timer #(.MOD_VALUE(256), .BIT_WIDTH(8)) T0 (
      .clk(clk),
      .reset(btnu),
      .increment(oneShotToCounter),
      .rolling_over(s0),
      .count(counterToSevenSegment)
  );

  timer #(.MOD_VALUE(256), .BIT_WIDTH(8)) T1 (
      .clk(clk),
      .reset(btnu),
      .increment(oneShotToCounter2),
      .rolling_over(s1),
      .count(counterToSevenSegment2)
  );


  display_control DC0 (
      .clk(clk),
      .reset(btnu),
      .dataIn({counterToSevenSegment2, counterToSevenSegment}),
      .digitDisplay(4'b1111),
      .digitPoint(4'b0000),
      .anode(anode),
      .segment(segment)
  );

  always_ff @(posedge clk) begin

    sync <= btnc;
    syncToDebounce <= sync;

    f1 <= debounceToOneShot;
    f2 <= f1;

    f3 <= syncToDebounce;
    f4 <= f3;
  end
endmodule
