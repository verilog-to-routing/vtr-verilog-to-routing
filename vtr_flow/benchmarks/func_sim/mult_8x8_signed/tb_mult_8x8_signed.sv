// Testbench for the registered 8x8 signed multiply
//  - applies reset, then drives all signed (a, b) pairs
//  - waits two clock edges (capture inputs, then registered product)
//  - compares 16-bit output to signed a * b in software
`timescale 1ns/1ps
module tb;

    logic clk;
    logic rst;
    logic a0, a1, a2, a3, a4, a5, a6, a7;
    logic b0, b1, b2, b3, b4, b5, b6, b7;
    logic p0, p1, p2, p3, p4, p5, p6, p7;
    logic p8, p9, p10, p11, p12, p13, p14, p15;

    logic signed [7:0] aVal;
    logic signed [7:0] bVal;
    logic signed [15:0] expected;
    logic signed [15:0] actual;
    int errors;

    top dut (
        .clk(clk), .rst(rst),
        .a0(a0), .a1(a1), .a2(a2), .a3(a3), .a4(a4), .a5(a5), .a6(a6), .a7(a7),
        .b0(b0), .b1(b1), .b2(b2), .b3(b3), .b4(b4), .b5(b5), .b6(b6), .b7(b7),
        .p0(p0), .p1(p1), .p2(p2), .p3(p3), .p4(p4), .p5(p5), .p6(p6), .p7(p7),
        .p8(p8), .p9(p9), .p10(p10), .p11(p11), .p12(p12), .p13(p13), .p14(p14), .p15(p15)
    );

    initial clk = 0;
    always #5 clk = ~clk;

    task automatic driveInputs(input logic signed [7:0] aIn, input logic signed [7:0] bIn);
        {a7, a6, a5, a4, a3, a2, a1, a0} = aIn;
        {b7, b6, b5, b4, b3, b2, b1, b0} = bIn;
    endtask

    function automatic logic signed [15:0] sampleProduct();
        return {p15, p14, p13, p12, p11, p10, p9, p8,
                p7, p6, p5, p4, p3, p2, p1, p0};
    endfunction

    task automatic checkMult(input logic signed [7:0] aIn, input logic signed [7:0] bIn);
        logic signed [15:0] expectedVal;
        driveInputs(aIn, bIn);
        @(posedge clk); // capture inputs
        @(posedge clk); // registered product
        #1;
        expectedVal = aIn * bIn;
        actual = sampleProduct();
        if (actual !== expectedVal) begin
            $display("FAIL mult a=%0d b=%0d expected=%0d got=%0d",
                     aIn, bIn, expectedVal, actual);
            errors++;
        end
    endtask

    initial begin
        errors = 0;
        rst = 1;
        driveInputs(8'sd0, 8'sd0);
        repeat (3) @(posedge clk);
        rst = 0;

        for (int aTest = -128; aTest < 128; aTest++) begin
            for (int bTest = -128; bTest < 128; bTest++) begin
                checkMult(8'(aTest), 8'(bTest));
            end
        end

        if (errors == 0) begin
            $display("mult_8x8_signed: all tests passed.");
            $finish;
        end else begin
            $display("mult_8x8_signed: %0d test(s) failed.", errors);
            $fatal(1);
        end
    end

endmodule
