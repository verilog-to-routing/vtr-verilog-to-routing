// Content from button_controller.sv
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


// Content from debounce.sv
`timescale 1ns / 1ps `default_nettype none

module debounce (
    input wire logic clk,
    reset,
    noisy,
    output logic debounced
);

  logic timerDone, clrTimer;

  typedef enum logic [1:0] {
    s0,
    s1,
    s2,
    s3,
    ERR = 'X
  } state_type_e;
  state_type_e ns, cs;

  logic [18:0] tA;

  timer #(.MOD_VALUE(500000), .BIT_WIDTH(19)) T0 (
      .clk(clk),
      .reset(clrTimer),
      .increment(1'b1),
      .rolling_over(timerDone),
      .count(tA)
  );

  always_comb begin
    ns = ERR;
    clrTimer = 0;
    debounced = 0;

    if (reset) ns = s0;
    else
      case (cs)
        s0: begin
          clrTimer = 1'b1;
          if (noisy) ns = s1;
          else ns = s0;
        end
        s1:
        if (noisy && timerDone) ns = s2;
        else if (noisy && ~timerDone) ns = s1;
        else ns = s0;
        s2: begin
          debounced = 1'b1;
          clrTimer  = 1'b1;
          if (noisy) ns = s2;
          else ns = s3;
        end
        s3: begin
          debounced = 1'b1;
          if (~noisy && timerDone) ns = s0;
          else if (~noisy && ~timerDone) ns = s3;
          else ns = s2;
        end
      endcase
  end

  always_ff @(posedge clk) cs <= ns;
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

module timer #(
    parameter MOD_VALUE = 1,
    parameter BIT_WIDTH = 1
) (
    input wire logic clk,
    reset,
    increment,
    output logic rolling_over,
    output logic [BIT_WIDTH-1:0] count = 0
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


