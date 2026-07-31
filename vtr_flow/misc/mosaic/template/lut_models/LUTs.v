module LUT1 #(
  parameter [1:0] INIT_VALUE = 2'h0 // 2-bit LUT logic value
) (
  input I0, // Data Input
  output O // Data Output
);

endmodule

module LUT2 #(
  parameter [3:0] INIT_VALUE = 4'h0 // 4-bit LUT logic value
) (
  input I0, // Data Input
  input I1, // Data Input
  output O // Data Output
);

endmodule

module LUT3 #(
  parameter [7:0] INIT_VALUE = 8'h00 // 8-bit LUT logic value
) (
  input I0, // Data Input
  input I1, // Data Input
  input I2, // Data Input
  output O // Data Output
);

endmodule

module LUT4 #(
  parameter [15:0] INIT_VALUE = 16'h0000 // 16-bit LUT logic value
) (
  input I0, // Data Input
  input I1, // Data Input
  input I2, // Data Input
  input I3, // Data Input
  output O // Data Output
);

endmodule

module LUT5 #(
  parameter [31:0] INIT_VALUE = 32'h0000 // 32-bit LUT logic value
) (
  input I0, // Data Input
  input I1, // Data Input
  input I2, // Data Input
  input I3, // Data Input
  input I4, // Data Input
  output O // Data Output
);

endmodule

module LUT6 #(
  parameter [63:0] INIT_VALUE = 64'h0000 // 64-bit LUT logic value
) (
  input I0, // Data Input
  input I1, // Data Input
  input I2, // Data Input
  input I3, // Data Input
  input I4, // Data Input
  input I5, // Data Input
  output O // Data Output
);

endmodule

