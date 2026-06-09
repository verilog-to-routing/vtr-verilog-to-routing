// Testbench for the shift register
//  - holds reset and checks output is zero
//  - shifts in a fixed bit pattern one bit per clock
//  - after each shift, compares q7..q0 to an expected value built in software
`timescale 1ns/1ps
module tb;

    logic clk;
    logic rst;
    logic shift_in;
    logic q0, q1, q2, q3, q4, q5, q6, q7;

    logic [7:0] expected;
    logic [7:0] actual;
    int errors;

    top dut (
        .clk(clk), .rst(rst), .shift_in(shift_in),
        .q0(q0), .q1(q1), .q2(q2), .q3(q3),
        .q4(q4), .q5(q5), .q6(q6), .q7(q7)
    );

    initial clk = 0;
    always #5 clk = ~clk;

    function automatic logic [7:0] sampleQ();
        return {q7, q6, q5, q4, q3, q2, q1, q0};
    endfunction

    task automatic shiftBit(input bit bitVal);
        shift_in = bitVal;
        expected = {expected[6:0], bitVal};
        @(posedge clk);
        #1;
        actual = sampleQ();
        if (actual !== expected) begin
            $display("FAIL: expected=0x%02x got=0x%02x", expected, actual);
            errors++;
        end
    endtask

    initial begin
        errors = 0;
        expected = 8'h00;
        rst = 1;
        shift_in = 0;
        repeat (2) @(posedge clk);
        rst = 0;
        @(posedge clk);
        #1;
        if (sampleQ() !== 8'h00) begin
            $display("FAIL: not zero after reset");
            errors++;
        end

        shiftBit(1);
        shiftBit(0);
        shiftBit(1);
        shiftBit(1);
        shiftBit(0);
        shiftBit(0);
        shiftBit(1);
        shiftBit(1);

        shiftBit(0);
        shiftBit(1);

        if (errors == 0) begin
            $display("shift_reg_routed: all tests passed.");
            $finish;
        end else begin
            $display("shift_reg_routed: %0d test(s) failed.", errors);
            $fatal(1);
        end
    end

endmodule
