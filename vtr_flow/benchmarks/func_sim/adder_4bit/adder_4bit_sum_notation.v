// 4-bit adder implemented using Verilog's compact concatenation/addition
// notation rather than explicit carry-propagation logic. This is more likely
// to produce carry-chains.
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
    assign {cout, s3, s2, s1, s0} = {a3, a2, a1, a0} + {b3, b2, b1, b0} + cin;
endmodule
