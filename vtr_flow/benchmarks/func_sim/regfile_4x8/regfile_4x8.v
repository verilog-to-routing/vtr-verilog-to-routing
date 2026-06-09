// 4-entry by 8-bit register file
//  - uses the vtr dual_port_ram primitive
//  - port 1: read only (we1=0, addr1, out1)
//  - port 2: write when we=1 (addr2, data2)
//  - single-bit ports so post-implementation netlist port names match the testbench
module top (
    input  clk,
    input  we,
    input  waddr0, waddr1,
    input  wd0, wd1, wd2, wd3, wd4, wd5, wd6, wd7,
    input  raddr0, raddr1,
    output rd0, rd1, rd2, rd3, rd4, rd5, rd6, rd7
);
    wire [1:0] waddr;
    wire [1:0] raddr;
    wire [7:0] wdata;
    wire [7:0] rdata;
    wire [7:0] unusedOut2;

    assign waddr = {waddr1, waddr0};
    assign raddr = {raddr1, raddr0};
    assign wdata = {wd7, wd6, wd5, wd4, wd3, wd2, wd1, wd0};
    assign {rd7, rd6, rd5, rd4, rd3, rd2, rd1, rd0} = rdata;

    defparam rf.ADDR_WIDTH = 2;
    defparam rf.DATA_WIDTH = 8;
    dual_port_ram rf (
        .clk   (clk),
        .addr1 (raddr),
        .addr2 (waddr),
        .data1 (8'h00),
        .data2 (wdata),
        .we1   (1'b0),
        .we2   (we),
        .out1  (rdata),
        .out2  (unusedOut2)
    );
endmodule
