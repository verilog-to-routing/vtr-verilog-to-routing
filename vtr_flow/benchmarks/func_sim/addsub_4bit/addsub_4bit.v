// selectable 4-bit adder or subtractor
//  - when sub_or_nadd is 0: {cout, s} = a + b + cin
//  - when sub_or_nadd is 1: {cout, s} = a - b - cin
//  - single-bit ports so post-implementation netlist port names match the testbench
module top (
    input  sub_or_nadd,
    input  cin,
    input  a0, a1, a2, a3,
    input  b0, b1, b2, b3,
    output s0, s1, s2, s3,
    output cout
);
    wire [3:0] a;
    wire [3:0] b;
    wire [4:0] result;

    assign a = {a3, a2, a1, a0};
    assign b = {b3, b2, b1, b0};
    assign result = sub_or_nadd
        ? ({1'b0, a} - {1'b0, b} - cin)
        : ({1'b0, a} + {1'b0, b} + cin);
    assign {cout, s3, s2, s1, s0} = result;
endmodule
