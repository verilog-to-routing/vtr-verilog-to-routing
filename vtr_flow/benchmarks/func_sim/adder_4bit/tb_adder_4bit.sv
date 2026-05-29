// Testbench for the 4-bit ripple-carry adder.
//
// Sweeps all 2^9 = 512 input combinations and checks each result against
// a golden reference computed with integer arithmetic.
//
// Conventions required by run_func_sim_flow.py:
//   - The top-level module must be named 'tb'.
//   - $finish  signals success (simulator exits with code 0).
//   - $fatal(1) signals failure (simulator exits with non-zero code).
`timescale 1ns/1ps
module tb;

    logic cin;
    logic a0, a1, a2, a3;
    logic b0, b1, b2, b3;
    logic s0, s1, s2, s3, cout;

    logic [4:0] expected, actual;
    int errors;

    top dut (
        .cin (cin),
        .a0  (a0),  .a1 (a1),  .a2 (a2),  .a3 (a3),
        .b0  (b0),  .b1 (b1),  .b2 (b2),  .b3 (b3),
        .s0  (s0),  .s1 (s1),  .s2 (s2),  .s3 (s3),
        .cout(cout)
    );

    initial begin
        errors = 0;

        for (int c = 0; c < 2; c++) begin
            cin = c[0];
            for (int a = 0; a < 16; a++) begin
                {a3, a2, a1, a0} = 4'(a);
                for (int b = 0; b < 16; b++) begin
                    {b3, b2, b1, b0} = 4'(b);
                    #1;

                    expected = 5'(a + b + c);
                    actual   = {cout, s3, s2, s1, s0};

                    if (actual !== expected) begin
                        $display("FAIL: a=%0d b=%0d cin=%0d  expected=%0d  got=%0d",
                                 a, b, c, expected, actual);
                        errors++;
                    end
                end
            end
        end

        if (errors == 0) begin
            $display("All 512 tests PASSED.");
            $finish;
        end else begin
            $display("%0d test(s) FAILED.", errors);
            $fatal(1);
        end
    end

endmodule
