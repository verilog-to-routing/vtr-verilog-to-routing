// Testbench for the selectable 4-bit adder/subtractor
//  - sweeps all 2^10 = 1024 input combinations
//  - checks add and subtract modes against software reference
`timescale 1ns/1ps
module tb;

    logic subOrNadd;
    logic cin;
    logic a0, a1, a2, a3;
    logic b0, b1, b2, b3;
    logic s0, s1, s2, s3, cout;

    logic [4:0] expected;
    logic [4:0] actual;
    int errors;

    top dut (
        .sub_or_nadd(subOrNadd),
        .cin       (cin),
        .a0        (a0), .a1 (a1), .a2 (a2), .a3 (a3),
        .b0        (b0), .b1 (b1), .b2 (b2), .b3 (b3),
        .s0        (s0), .s1 (s1), .s2 (s2), .s3 (s3),
        .cout      (cout)
    );

    initial begin
        errors = 0;

        for (int mode = 0; mode < 2; mode++) begin
            subOrNadd = mode[0];
            for (int c = 0; c < 2; c++) begin
                cin = c[0];
                for (int a = 0; a < 16; a++) begin
                    {a3, a2, a1, a0} = 4'(a);
                    for (int b = 0; b < 16; b++) begin
                        {b3, b2, b1, b0} = 4'(b);
                        #1;

                        if (subOrNadd)
                            expected = 5'({1'b0, 4'(a)} - {1'b0, 4'(b)} - c);
                        else
                            expected = 5'({1'b0, 4'(a)} + {1'b0, 4'(b)} + c);
                        actual = {cout, s3, s2, s1, s0};

                        if (actual !== expected) begin
                            $display("FAIL: sub_or_nadd=%0d a=%0d b=%0d cin=%0d expected=%0d got=%0d",
                                     mode, a, b, c, expected, actual);
                            errors++;
                        end
                    end
                end
            end
        end

        if (errors == 0) begin
            $display("addsub_4bit: all tests passed.");
            $finish;
        end else begin
            $display("addsub_4bit: %0d test(s) failed.", errors);
            $fatal(1);
        end
    end

endmodule
