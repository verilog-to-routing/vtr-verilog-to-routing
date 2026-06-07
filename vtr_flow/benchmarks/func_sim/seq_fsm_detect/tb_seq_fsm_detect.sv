// Testbench for the 101 sequence detector
//  - holds reset, then streams bits into data_in one clock at a time
//  - keeps a simple reference state machine in software
//  - after each clock, compares match to what the reference expects
//  - runs a few fixed patterns, then 64 random bits
`timescale 1ns/1ps
module tb;

    logic clk;
    logic rst;
    logic data_in;
    logic match;

    int errors;
    int refState;

    top dut (
        .clk     (clk),
        .rst     (rst),
        .data_in (data_in),
        .match   (match)
    );

    initial clk = 0;
    always #5 clk = ~clk;

    function automatic void refStep(input bit d);
        case (refState)
            0: refState = d ? 1 : 0;
            1: refState = d ? 1 : 2;
            2: refState = d ? 3 : 0;
            3: refState = d ? 1 : 2;
            default: refState = 0;
        endcase
    endfunction

    function automatic bit refMatch();
        return (refState == 3);
    endfunction

    task automatic applyBit(input bit d);
        data_in = d;
        @(posedge clk);
        #1;
        refStep(d);
        if (match !== refMatch()) begin
            $display("FAIL: bit=%0d state=%0d expected_match=%0d got_match=%0d",
                     d, refState, refMatch(), match);
            errors++;
        end
    endtask

    initial begin
        errors = 0;
        refState = 0;
        rst = 1;
        data_in = 0;
        repeat (3) @(posedge clk);
        rst = 0;
        @(posedge clk);

        // directed patterns
        applyBit(1); applyBit(0); applyBit(1); // should assert match on third bit
        applyBit(0);
        applyBit(1); applyBit(0); applyBit(1); applyBit(1); // 101 then extra 1
        applyBit(0); applyBit(0); applyBit(0);

        // pseudo-random stream (short, keeps ci runtime low)
        for (int i = 0; i < 64; i++) begin
            applyBit($urandom_range(0, 1));
        end

        if (errors == 0) begin
            $display("seq_fsm_detect: all tests passed.");
            $finish;
        end else begin
            $display("seq_fsm_detect: %0d test(s) failed.", errors);
            $fatal(1);
        end
    end

endmodule
