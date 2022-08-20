`timescale 1ns / 1ps `default_nettype none

module top (
    input wire logic clk,
    btnc,
    sw,
    output logic [3:0] anode,
    output logic [7:0] segment
);

  logic [15:0] digitData;

  timer TC0 (
      .clk(clk),
      .reset(btnc),
      .run(sw),
      .digit0(digitData[3:0]),
      .digit1(digitData[7:4]),
      .digit2(digitData[11:8]),
      .digit3(digitData[15:12])
  );
  display_control SSC0 (
      .clk(clk),
      .reset(btnc),
      .dataIn(digitData),
      .digitDisplay(4'b1111),
      .digitPoint(4'b0100),
      .anode(anode),
      .segment(segment)
  );
endmodule
