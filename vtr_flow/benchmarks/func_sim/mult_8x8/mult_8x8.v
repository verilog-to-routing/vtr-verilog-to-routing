// registered 8-bit by 8-bit unsigned multiply
//  - latches a and b on each clock edge
//  - forms product = a_reg * b_reg
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
    wire [7:0] a;
    wire [7:0] b;
    reg [7:0] aReg;
    reg [7:0] bReg;
    reg [15:0] prodReg;

    assign a = {a7, a6, a5, a4, a3, a2, a1, a0};
    assign b = {b7, b6, b5, b4, b3, b2, b1, b0};
    assign {p15, p14, p13, p12, p11, p10, p9, p8,
            p7, p6, p5, p4, p3, p2, p1, p0} = prodReg;

    always @(posedge clk) begin
        if (rst) begin
            aReg <= 8'h00;
            bReg <= 8'h00;
            prodReg <= 16'h0000;
        end else begin
            aReg <= a;
            bReg <= b;
            prodReg <= aReg * bReg;
        end
    end
endmodule
