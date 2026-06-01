// Testbench for the 8x8 single-port ram
//  - writes a unique byte to each address 0..7
//  - reads each address back and checks the byte
//  - overwrites address 3 and reads it again
`timescale 1ns/1ps
module tb;

    logic clk;
    logic we;
    logic a0, a1, a2;
    logic d0, d1, d2, d3, d4, d5, d6, d7;
    logic q0, q1, q2, q3, q4, q5, q6, q7;

    logic [7:0] expected [0:7];
    logic [7:0] actual;
    int errors;

    top dut (
        .clk(clk), .we(we),
        .a0(a0), .a1(a1), .a2(a2),
        .d0(d0), .d1(d1), .d2(d2), .d3(d3),
        .d4(d4), .d5(d5), .d6(d6), .d7(d7),
        .q0(q0), .q1(q1), .q2(q2), .q3(q3),
        .q4(q4), .q5(q5), .q6(q6), .q7(q7)
    );

    initial clk = 0;
    always #5 clk = ~clk;

    task automatic driveAddr(input int addr);
        {a2, a1, a0} = 3'(addr);
    endtask

    task automatic driveData(input logic [7:0] data);
        {d7, d6, d5, d4, d3, d2, d1, d0} = data;
    endtask

    function automatic logic [7:0] sampleQ();
        return {q7, q6, q5, q4, q3, q2, q1, q0};
    endfunction

    initial begin
        errors = 0;
        we = 0;
        driveAddr(0);
        driveData(8'h00);
        @(posedge clk);

        // write unique pattern per address
        for (int addr = 0; addr < 8; addr++) begin
            expected[addr] = 8'hA0 | 8'(addr);
            driveAddr(addr);
            driveData(expected[addr]);
            we = 1;
            @(posedge clk);
        end
        we = 0;
        @(posedge clk);

        // read back after address change (distributed 1-bit ram banks need extra cycles)
        for (int addr = 0; addr < 8; addr++) begin
            driveAddr(addr);
            repeat (2) @(posedge clk);
            #1;
            actual = sampleQ();
            if (actual !== expected[addr]) begin
                $display("FAIL read addr=%0d expected=0x%02x got=0x%02x",
                         addr, expected[addr], actual);
                errors++;
            end
        end

        // overwrite addr 3 and confirm
        expected[3] = 8'h5a;
        driveAddr(3);
        driveData(expected[3]);
        we = 1;
        @(posedge clk);
        we = 0;
        // distributed 1-bit ram banks need extra cycles before read data settles
        repeat (4) @(posedge clk);
        #1;
        if (sampleQ() !== expected[3]) begin
            $display("FAIL re-read addr=3 expected=0x%02x got=0x%02x",
                     expected[3], sampleQ());
            errors++;
        end

        if (errors == 0) begin
            $display("sp_ram_8x8: all tests passed.");
            $finish;
        end else begin
            $display("sp_ram_8x8: %0d test(s) failed.", errors);
            $fatal(1);
        end
    end

endmodule
