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

    // fractured 1-bit bram banks use read-first timing from primitives.v
    task automatic writeAt(input int addr, input logic [7:0] data);
        driveAddr(addr);
        driveData(data);
        we = 0;
        @(posedge clk);
        we = 1;
        @(posedge clk);
        we = 0;
    endtask

    task automatic checkReadAt(input int addr, input logic [7:0] exp, input string label);
        int cycles;
        driveAddr(addr);
        we = 0;
        cycles = 0;
        do begin
            @(posedge clk);
            #1;
            cycles++;
        end while (sampleQ() !== exp && cycles < 12);
        actual = sampleQ();
        if (actual !== exp) begin
            $display("FAIL %s addr=%0d expected=0x%02x got=0x%02x",
                     label, addr, exp, actual);
            errors++;
        end
    endtask

    initial begin
        errors = 0;
        we = 0;
        driveAddr(0);
        driveData(8'h00);
        @(posedge clk);

        for (int addr = 0; addr < 8; addr++) begin
            expected[addr] = 8'hA0 | 8'(addr);
            writeAt(addr, expected[addr]);
        end
        @(posedge clk);

        for (int addr = 0; addr < 8; addr++)
            checkReadAt(addr, expected[addr], "read");

        expected[3] = 8'h5a;
        writeAt(3, expected[3]);
        checkReadAt(3, expected[3], "re-read");

        if (errors == 0) begin
            $display("sp_ram_8x8: all tests passed.");
            $finish;
        end else begin
            $display("sp_ram_8x8: %0d test(s) failed.", errors);
            $fatal(1);
        end
    end

endmodule
