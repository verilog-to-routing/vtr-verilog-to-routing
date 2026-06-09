// 8-word by 8-bit single-port bram
//  - uses the vtr single_port_ram primitive (8 addresses, 8-bit data)
//  - when we=1 on a clock edge, writes din to addr
//  - every read returns the value at addr (read-first behavior from primitives.v)
//  - single-bit ports so post-implementation netlist port names match the testbench
module top (
    input  clk,
    input  we,
    input  a0, a1, a2,
    input  d0, d1, d2, d3, d4, d5, d6, d7,
    output q0, q1, q2, q3, q4, q5, q6, q7
);
    wire [2:0] addr;
    wire [7:0] din;
    wire [7:0] dout;

    assign addr = {a2, a1, a0};
    assign din  = {d7, d6, d5, d4, d3, d2, d1, d0};
    assign {q7, q6, q5, q4, q3, q2, q1, q0} = dout;

    defparam mem.ADDR_WIDTH = 3;
    defparam mem.DATA_WIDTH = 8;
    single_port_ram mem (
        .clk  (clk),
        .addr (addr),
        .we   (we),
        .data (din),
        .out  (dout)
    );
endmodule
