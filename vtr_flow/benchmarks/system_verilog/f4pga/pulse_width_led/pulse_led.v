module top (
    input wire clk,
    input wire [3:0] sw,
    input wire [3:0] btn,
    output wire pulse_red,
    pulse_blue,
    pulse_green
);
  wire [13:0] pulse_wideR, pulse_wideB, pulse_wideG;

  assign pulse_wideR = {1'b0, sw[3:1], 10'd0};
  assign pulse_wideG = {1'b0, sw[0], btn[3:2], 10'd0};
  assign pulse_wideB = {btn[1:0], 11'd0};

  PWM R0 (
      .clk  (clk),
      .pulse(pulse_red),
      .width(pulse_wideR)
  );
  PWM B0 (
      .clk  (clk),
      .pulse(pulse_green),
      .width(pulse_wideB)
  );
  PWM G0 (
      .clk  (clk),
      .pulse(pulse_blue),
      .width(pulse_wideG)
  );


endmodule
