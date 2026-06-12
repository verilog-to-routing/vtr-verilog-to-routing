// Testbench for the 4x8 register file
//  - writes a pattern into registers 0..3
//  - reads each register back and checks the value
//  - on one cycle, writes reg 2 while reading reg 1 and checks both sides
`timescale 1ns/1ps
module tb;

    logic clk;
    logic we;
    logic waddr0, waddr1;
    logic wd0, wd1, wd2, wd3, wd4, wd5, wd6, wd7;
    logic raddr0, raddr1;
    logic rd0, rd1, rd2, rd3, rd4, rd5, rd6, rd7;

    logic [7:0] golden [0:3];
    logic [7:0] actual;
    int errors;

    top dut (
        .clk(clk), .we(we),
        .waddr0(waddr0), .waddr1(waddr1),
        .wd0(wd0), .wd1(wd1), .wd2(wd2), .wd3(wd3),
        .wd4(wd4), .wd5(wd5), .wd6(wd6), .wd7(wd7),
        .raddr0(raddr0), .raddr1(raddr1),
        .rd0(rd0), .rd1(rd1), .rd2(rd2), .rd3(rd3),
        .rd4(rd4), .rd5(rd5), .rd6(rd6), .rd7(rd7)
    );

    initial clk = 0;
    always #5 clk = ~clk;

    function automatic logic [7:0] sampleRd();
        return {rd7, rd6, rd5, rd4, rd3, rd2, rd1, rd0};
    endfunction

    // fractured 1-bit dp ram uses read-first timing from primitives.v
    task automatic driveWrite(input int addr, input logic [7:0] data);
        {waddr1, waddr0} = 2'(addr);
        {wd7, wd6, wd5, wd4, wd3, wd2, wd1, wd0} = data;
        we = 0;
        @(posedge clk);
        we = 1;
        @(posedge clk);
        we = 0;
    endtask

    task automatic checkReadAt(input int addr, input logic [7:0] exp, input string label);
        int cycles;
        {raddr1, raddr0} = 2'(addr);
        {waddr1, waddr0} = 2'(addr);
        we = 0;
        cycles = 0;
        do begin
            @(posedge clk);
            #1;
            cycles++;
        end while (sampleRd() !== exp && cycles < 12);
        actual = sampleRd();
        if (actual !== exp) begin
            $display("FAIL %s reg=%0d expected=0x%02x got=0x%02x",
                     label, addr, exp, actual);
            errors++;
        end
    endtask

    initial begin
        errors = 0;
        we = 0;
        {waddr1, waddr0} = 2'b00;
        {raddr1, raddr0} = 2'b00;
        {wd7, wd6, wd5, wd4, wd3, wd2, wd1, wd0} = 8'h00;
        @(posedge clk);

        for (int i = 0; i < 4; i++) begin
            golden[i] = 8'h10 * 8'(i + 1);
            driveWrite(i, golden[i]);
        end
        @(posedge clk);

        for (int i = 0; i < 4; i++)
            checkReadAt(i, golden[i], "read");

        // same-cycle write/read different registers
        golden[2] = 8'hcc;
        {waddr1, waddr0} = 2'd2;
        {wd7, wd6, wd5, wd4, wd3, wd2, wd1, wd0} = golden[2];
        {raddr1, raddr0} = 2'd1;
        we = 0;
        @(posedge clk);
        we = 1;
        @(posedge clk);
        we = 0;
        repeat (2) @(posedge clk);
        #1;
        actual = sampleRd();
        if (actual !== golden[1]) begin
            $display("FAIL same-cycle read reg1 expected=0x%02x got=0x%02x",
                     golden[1], actual);
            errors++;
        end

        checkReadAt(2, golden[2], "read reg2 after write");

        if (errors == 0) begin
            $display("regfile_4x8: all tests passed.");
            $finish;
        end else begin
            $display("regfile_4x8: %0d test(s) failed.", errors);
            $fatal(1);
        end
    end

endmodule
