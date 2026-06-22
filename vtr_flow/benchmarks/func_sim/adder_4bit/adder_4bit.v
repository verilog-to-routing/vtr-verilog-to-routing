// 4-bit ripple-carry adder.
//
// Ports use individual single-bit names rather than bus ports so that the
// post-implementation netlist produced by VPR has port names that match
// directly without any bus-index mangling.
module top (
    input  cin,
    input  a0, a1, a2, a3,
    input  b0, b1, b2, b3,
    output s0, s1, s2, s3,
    output cout
);
    wire c0, c1, c2;

    assign s0   = a0 ^ b0 ^ cin;
    assign c0   = (a0 & b0) | ((a0 ^ b0) & cin);

    assign s1   = a1 ^ b1 ^ c0;
    assign c1   = (a1 & b1) | ((a1 ^ b1) & c0);

    assign s2   = a2 ^ b2 ^ c1;
    assign c2   = (a2 & b2) | ((a2 ^ b2) & c1);

    assign s3   = a3 ^ b3 ^ c2;
    assign cout = (a3 & b3) | ((a3 ^ b3) & c2);
endmodule
