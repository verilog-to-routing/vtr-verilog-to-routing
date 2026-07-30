// mult_map.v
// fallback static for k6. normally regenerated from the arch xml by
// vtr_arch_rules. maps $mul onto the arch multiply primitive.
//
// has to run before alumacc ($mul becomes $macc) and before the generic
// techmap ($mul becomes gates). either way the multiply is gone.
//
// vpr picks the mult_36 mode from WIDTH so we take the smallest that still
// fits. signed ops need one extra bit of sign extension and that extended
// width still has to fit 36 or we fail and leave the $mul soft.
//
// truncating the product to Y_WIDTH is fine  the $mul contract is the low
// bits anyway.

module \$mul (A, B, Y);
    parameter A_SIGNED = 0;
    parameter B_SIGNED = 0;
    parameter A_WIDTH  = 1;
    parameter B_WIDTH  = 1;
    parameter Y_WIDTH  = 1;

    input  [A_WIDTH-1:0] A;
    input  [B_WIDTH-1:0] B;
    output [Y_WIDTH-1:0] Y;

    localparam AW_EXT = (A_SIGNED != 0) ? A_WIDTH + 1 : A_WIDTH;
    localparam BW_EXT = (B_SIGNED != 0) ? B_WIDTH + 1 : B_WIDTH;

    wire _TECHMAP_FAIL_ = (AW_EXT > 36) || (BW_EXT > 36);

    // clamp so the wire decls stay legal even on the fail path
    localparam AW = (AW_EXT > 36) ? 36 : AW_EXT;
    localparam BW = (BW_EXT > 36) ? 36 : BW_EXT;

    localparam MULT_W = (AW <= 9 && BW <= 9) ? 9 : (AW <= 18 && BW <= 18) ? 18 : 36;

    wire [AW-1:0] a_ext;
    generate
        if (A_SIGNED)
            assign a_ext = {{(AW - A_WIDTH){A[A_WIDTH-1]}}, A};
        else
            assign a_ext = {{(AW - A_WIDTH){1'b0}}, A};
    endgenerate
    wire [MULT_W-1:0] aa = {{(MULT_W - AW){1'b0}}, a_ext};

    wire [BW-1:0] b_ext;
    generate
        if (B_SIGNED)
            assign b_ext = {{(BW - B_WIDTH){B[B_WIDTH-1]}}, B};
        else
            assign b_ext = {{(BW - B_WIDTH){1'b0}}, B};
    endgenerate
    wire [MULT_W-1:0] bb = {{(MULT_W - BW){1'b0}}, b_ext};

    wire [2*MULT_W-1:0] fullOut;

    multiply #(.WIDTH(MULT_W)) _TECHMAP_REPLACE_ (
        .a   (aa),
        .b   (bb),
        .out (fullOut)
    );

    assign Y = fullOut[Y_WIDTH-1:0];

endmodule
