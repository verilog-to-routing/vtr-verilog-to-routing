// registered 8-bit by 8-bit signed multiply
//  - latches a and b on each clock edge
//  - converts signed operands to unsigned magnitudes for the primitive
//  - multiplies operand signs and applies the result sign to the product
//  - product is valid one full clock after inputs change
//  - single-bit ports so post-implementation netlist port names match the testbench
module top (
    input  clk,
    input  rst,
    input  a0, a1, a2, a3, a4, a5, a6, a7,
    input  b0, b1, b2, b3, b4, b5, b6, b7,
    output p0, p1, p2, p3, p4, p5, p6, p7,
    output p8, p9, p10, p11, p12, p13, p14, p15
);
    wire signed [7:0] a;
    wire signed [7:0] b;
    reg signed [7:0] aReg;
    reg signed [7:0] bReg;
    reg signed [15:0] prodReg;

    wire signA;
    wire signB;
    wire negResult;
    wire [7:0] aMag;
    wire [7:0] bMag;
    wire [15:0] unsignedProd;
    wire signed [15:0] signedProd;

    assign a = {a7, a6, a5, a4, a3, a2, a1, a0};
    assign b = {b7, b6, b5, b4, b3, b2, b1, b0};
    assign {p15, p14, p13, p12, p11, p10, p9, p8,
            p7, p6, p5, p4, p3, p2, p1, p0} = prodReg;

    assign signA = aReg[7];
    assign signB = bReg[7];
    assign negResult = signA ^ signB;

    // -128 becomes unsigned 128
    assign aMag = signA ? (~aReg + 8'd1) : aReg;
    assign bMag = signB ? (~bReg + 8'd1) : bReg;

    multiply #(.WIDTH(8)) multInst (
        .a(aMag),
        .b(bMag),
        .out(unsignedProd)
    );

    assign signedProd = negResult ? (~unsignedProd + 16'd1) : unsignedProd;

    always @(posedge clk) begin
        if (rst) begin
            aReg <= 8'sd0;
            bReg <= 8'sd0;
            prodReg <= 16'sd0;
        end else begin
            aReg <= a;
            bReg <= b;
            prodReg <= signedProd;
        end
    end
endmodule
