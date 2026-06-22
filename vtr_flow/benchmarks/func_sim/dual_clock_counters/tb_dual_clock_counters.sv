// Testbench for the dual-clock counters
//  - generates clk_a and clk_b with different periods
//  - holds reset, then releases and steps both clocks together each loop
//  - tracks expected countA and countB and compares to o7..o0
`timescale 1ns/1ps
module tb;

    logic clk_a;
    logic clk_b;
    logic rst;
    logic o0, o1, o2, o3, o4, o5, o6, o7;

    logic [3:0] countA;
    logic [3:0] countB;
    logic [7:0] actual;
    int errors;

    top dut (
        .clk_a(clk_a),
        .clk_b(clk_b),
        .rst  (rst),
        .o0(o0), .o1(o1), .o2(o2), .o3(o3),
        .o4(o4), .o5(o5), .o6(o6), .o7(o7)
    );

    initial clk_a = 0;
    initial clk_b = 0;
    always #4 clk_a = ~clk_a;
    always #6 clk_b = ~clk_b;

    function automatic logic [7:0] sampleOut();
        return {o7, o6, o5, o4, o3, o2, o1, o0};
    endfunction

    task automatic checkSample();
        actual = sampleOut();
        if (actual !== {countB, countA}) begin
            $display("FAIL counters a=%0d b=%0d got=0x%02x",
                     countA, countB, actual);
            errors++;
        end
    endtask

    initial begin
        errors = 0;
        countA = 0;
        countB = 0;
        rst = 1;
        repeat (3) begin
            @(posedge clk_a);
            @(posedge clk_b);
        end
        rst = 0;
        #1;
        checkSample();

        for (int i = 0; i < 12; i++) begin
            fork
                begin @(posedge clk_a); countA = countA + 4'd1; end
                begin @(posedge clk_b); countB = countB + 4'd1; end
            join
            #1;
            checkSample();
        end

        if (errors == 0) begin
            $display("dual_clock_counters: all tests passed.");
            $finish;
        end else begin
            $display("dual_clock_counters: %0d test(s) failed.", errors);
            $fatal(1);
        end
    end

endmodule
