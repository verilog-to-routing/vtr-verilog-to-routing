// blackbox lut models for mosaic techmap; INIT_VALUE holds the truth table bits
// for the matching lut size (2^k bits for LUTk).

module LUT1 #(
  parameter [1:0] INIT_VALUE = 2'h0
) (
  input I0,
  output O
);

endmodule

module LUT2 #(
  parameter [3:0] INIT_VALUE = 4'h0
) (
  input I0,
  input I1,
  output O
);

endmodule

module LUT3 #(
  parameter [7:0] INIT_VALUE = 8'h00
) (
  input I0,
  input I1,
  input I2,
  output O
);

endmodule

module LUT4 #(
  parameter [15:0] INIT_VALUE = 16'h0000
) (
  input I0,
  input I1,
  input I2,
  input I3,
  output O
);

endmodule

module LUT5 #(
  parameter [31:0] INIT_VALUE = 32'h0000
) (
  input I0,
  input I1,
  input I2,
  input I3,
  input I4,
  output O
);

endmodule

module LUT6 #(
  parameter [63:0] INIT_VALUE = 64'h0000
) (
  input I0,
  input I1,
  input I2,
  input I3,
  input I4,
  input I5,
  output O
);

endmodule
