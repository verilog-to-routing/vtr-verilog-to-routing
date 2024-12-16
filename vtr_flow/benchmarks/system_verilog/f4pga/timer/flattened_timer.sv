// Content from clock.sv
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


// Content from modify_count.sv
`default_nettype none

module modify_count #(
    parameter MOD_VALUE = 10
) (
    input wire logic clk,
    reset,
    increment,
    output logic rolling_over,
    output logic [3:0] count = 0
);

  always_ff @(posedge clk) begin
    if (reset) count <= 4'b0000;
    else if (increment) begin
      if (rolling_over) count <= 4'b0000;
      else count <= count + 4'b0001;
    end
  end

  always_comb begin
    if (increment && (count == MOD_VALUE - 1)) rolling_over = 1'b1;
    else rolling_over = 1'b0;
  end

endmodule


// Content from display_control.sv
`default_nettype none


module display_control (
    input  wire logic        clk,
    input  wire logic        reset,
    input  wire logic [15:0] dataIn,
    input  wire logic [ 3:0] digitDisplay,
    input  wire logic [ 3:0] digitPoint,
    output logic      [ 3:0] anode,
    output logic      [ 7:0] segment
);

  parameter integer COUNT_BITS = 17;

  logic [COUNT_BITS-1:0] count_val;
  logic [           1:0] anode_select;
  logic [           3:0] cur_anode;
  logic [           3:0] cur_data_in;


  always_ff @(posedge clk) begin
    if (reset) count_val <= 0;
    else count_val <= count_val + 1;
  end

  assign anode_select = count_val[COUNT_BITS-1:COUNT_BITS-2];

  assign cur_anode =
                        (anode_select == 2'b00) ? 4'b1110 :
                        (anode_select == 2'b01) ? 4'b1101 :
                        (anode_select == 2'b10) ? 4'b1011 :
                        4'b0111;

  assign anode = cur_anode | (~digitDisplay);

  assign cur_data_in =
                        (anode_select == 2'b00) ? dataIn[3:0] :
                        (anode_select == 2'b01) ? dataIn[7:4]  :
                        (anode_select == 2'b10) ? dataIn[11:8]  :
                        dataIn[15:12] ;

  assign segment[7] =
                        (anode_select == 2'b00) ? ~digitPoint[0] :
                        (anode_select == 2'b01) ? ~digitPoint[1]  :
                        (anode_select == 2'b10) ? ~digitPoint[2]  :
                        ~digitPoint[3] ;

  assign segment[6:0] =
        (cur_data_in == 0) ? 7'b1000000 :
        (cur_data_in == 1) ? 7'b1111001 :
        (cur_data_in == 2) ? 7'b0100100 :
        (cur_data_in == 3) ? 7'b0110000 :
        (cur_data_in == 4) ? 7'b0011001 :
        (cur_data_in == 5) ? 7'b0010010 :
        (cur_data_in == 6) ? 7'b0000010 :
        (cur_data_in == 7) ? 7'b1111000 :
        (cur_data_in == 8) ? 7'b0000000 :
        (cur_data_in == 9) ? 7'b0010000 :
        (cur_data_in == 10) ? 7'b0001000 :
        (cur_data_in == 11) ? 7'b0000011 :
        (cur_data_in == 12) ? 7'b1000110 :
        (cur_data_in == 13) ? 7'b0100001 :
        (cur_data_in == 14) ? 7'b0000110 :
        7'b0001110;


endmodule


// Content from timer.sv
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


// Content from time_counter.sv
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


