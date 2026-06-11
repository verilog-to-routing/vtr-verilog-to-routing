// Testbench for the registered 8x8 multiply
//  - applies reset, then drives several (a, b) pairs
//  - waits two clock edges (capture inputs, then registered product)
//  - compares 16-bit output to a * b in software
`timescale 1ns/1ps
module tb;

    logic clk;
    logic rst;
    logic a0, a1, a2, a3, a4, a5, a6, a7;
    logic b0, b1, b2, b3, b4, b5, b6, b7;
    logic p0, p1, p2, p3, p4, p5, p6, p7;
    logic p8, p9, p10, p11, p12, p13, p14, p15;

    logic [15:0] actual;
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

    task automatic driveInputs(input logic [7:0] aVal, input logic [7:0] bVal);
        {a7, a6, a5, a4, a3, a2, a1, a0} = aVal;
        {b7, b6, b5, b4, b3, b2, b1, b0} = bVal;
    endtask

    function automatic logic [15:0] sampleProduct();
        return {p15, p14, p13, p12, p11, p10, p9, p8,
                p7, p6, p5, p4, p3, p2, p1, p0};
    endfunction

    task automatic checkMult(input logic [7:0] aVal, input logic [7:0] bVal);
        logic [15:0] expected;
        driveInputs(aVal, bVal);
        @(posedge clk); // capture inputs
        @(posedge clk); // registered product
        #1;
        expected = 16'(aVal) * 16'(bVal);
        actual = sampleProduct();
        if (actual !== expected) begin
            $display("FAIL mult a=%0d b=%0d expected=%0d got=%0d",
                     aVal, bVal, expected, actual);
            errors++;
        end
    endtask

    initial begin
        errors = 0;
        rst = 1;
        driveInputs(8'h00, 8'h00);
        repeat (3) @(posedge clk);
        rst = 0;

        checkMult(8'd0, 8'd0);
        checkMult(8'd1, 8'd1);
        checkMult(8'd3, 8'd5);
        checkMult(8'd12, 8'd7);
        checkMult(8'd255, 8'd255);
        checkMult(8'd200, 8'd3);

        if (errors == 0) begin
            $display("mult_8x8: all tests passed.");
            $finish;
        end else begin
            $display("mult_8x8: %0d test(s) failed.", errors);
            $fatal(1);
        end
    end

endmodule
