module MAE #(
  parameter A_REG=0,
  parameter B_REG=0,
  parameter C_REG=0,
  parameter P_REG=0,
  parameter MULT_HAS_REG=0,
  parameter POST_ADDER_STATIC=0,
  parameter USE_FEEDBACK=0
 )
(
  input [17:0] A,
  input ALLOW_A_REG,
  input A_EN,
  input A_ARST_N,
  input [17:0] B,
  input ALLOW_B_REG,
  input CDIN_FDBK_SEL,
  input B_EN,
  input B_ARST_N,
  input [39:0] C,
  input CLK,
  input C_ARST_N,
  input ALLOW_C_REG,
  input C_EN,
  output [39:0] P,
  input ALLOW_P_REG,
  input P_EN,
  input P_ARST_N,
  input resetn
);

generate
  if (A_REG == 1'b1 && B_REG == 1'b1 && P_REG==1'b0 && POST_ADDER_STATIC==1'b0) begin
    efpga_mult_regi _TECHMAP_REPLACE_ (
      .a(A),
      .b(B),
      .clk(CLK),
      .resetn(resetn),
      .y(P)
    );
  end
  else if (A_REG == 1'b0 && B_REG == 1'b0 && P_REG==1'b1 && POST_ADDER_STATIC==1'b0 && USE_FEEDBACK==1'b0) begin
    efpga_mult_rego _TECHMAP_REPLACE_ (
      .a(A),
      .b(B),
      .clk(CLK),
      .resetn(resetn),
      .y(P)
    );
  end
  else if (A_REG == 1'b1 && B_REG == 1'b1 && P_REG==1'b1 && POST_ADDER_STATIC==1'b0 && USE_FEEDBACK==1'b0) begin
    efpga_mult_regio _TECHMAP_REPLACE_ (
      .a(A),
      .b(B),
      .clk(CLK),
      .resetn(resetn),
      .y(P)
    );
  end
  else if (A_REG == 1'h0 && B_REG == 1'h0 && C_REG==1'h0 && P_REG==1'h0 && POST_ADDER_STATIC==1'h0 && USE_FEEDBACK==1'h0) begin
    efpga_mult _TECHMAP_REPLACE_ (
      .a(A),
      .b(B),
      .y(P)
    );
  end
  else if (A_REG == 1'b1 && B_REG == 1'b1 && P_REG==1'b0 && POST_ADDER_STATIC==1'b1 && USE_FEEDBACK==1'b0) begin
    efpga_mult_addc_regi _TECHMAP_REPLACE_ (
      .a(A),
      .b(B),
      .c(C),
      .clk(CLK),
      .resetn(resetn),
      .y(P)
    );
  end
  else if (A_REG == 1'b0 && B_REG == 1'b0 && P_REG==1'b1 && POST_ADDER_STATIC==1'b1 && USE_FEEDBACK == 1'b0) begin
    efpga_mult_addc_rego _TECHMAP_REPLACE_ (
      .a(A),
      .b(B),
      .c(C),
      .clk(CLK),
      .resetn(resetn),
      .y(P)
    );
  end
  else if (A_REG == 1'b1 && B_REG == 1'b1 && P_REG==1'b1 && POST_ADDER_STATIC==1'b1 && USE_FEEDBACK==1'b0) begin
    efpga_mult_addc_regio _TECHMAP_REPLACE_ (
      .a(A),
      .b(B),
      .c(C),
      .clk(CLK),
      .resetn(resetn),
      .y(P)
    );
  end
  else if (A_REG == 1'b0 && B_REG == 1'b0 && P_REG==1'b0 && POST_ADDER_STATIC==1'b1 && USE_FEEDBACK==1'b0) begin
    efpga_mult_addc _TECHMAP_REPLACE_ (
      .a(A),
      .b(B),
      .c(C),
      .y(P)
    );
  end
  else if (A_REG == 1'b0 && B_REG == 1'b0 && P_REG==1'b1 && POST_ADDER_STATIC==1'b1 && USE_FEEDBACK==1'b1 && MULT_HAS_REG == 1'b1) begin
    efpga_macc_pipe _TECHMAP_REPLACE_ (
      .a(A),
      .b(B),
      .clk(CLK),
      .resetn(resetn),
      .y(P)
    );
  end
  else if (A_REG == 1'b1 && B_REG == 1'b1 && P_REG==1'b1 && POST_ADDER_STATIC==1'b1 && USE_FEEDBACK == 1'b1 && MULT_HAS_REG == 1'b1) begin
    efpga_macc_pipe_regi _TECHMAP_REPLACE_ (
      .a(A),
      .b(B),
      .clk(CLK),
      .resetn(resetn),
      .y(P)
    );
  end
  else if (A_REG == 1'b1 && B_REG == 1'b1 && P_REG==1'b1 && POST_ADDER_STATIC==1'b1 && USE_FEEDBACK == 1'b1 && MULT_HAS_REG == 1'b0) begin
    efpga_macc_regi _TECHMAP_REPLACE_ (
      .a(A),
      .b(B),
      .clk(CLK),
      .resetn(resetn),
      .y(P)
    );
  end
  else if (A_REG == 1'b0 && B_REG == 1'b0 && P_REG==1'b1 && POST_ADDER_STATIC==1'b1 && USE_FEEDBACK == 1'b1 && MULT_HAS_REG == 1'b0) begin
    efpga_macc _TECHMAP_REPLACE_ (
      .a(A),
      .b(B),
      .y(P)
    );
  end
  else 
  begin
    wire _TECHMAP_FAIL_ = 1'b1;
  end
endgenerate
endmodule