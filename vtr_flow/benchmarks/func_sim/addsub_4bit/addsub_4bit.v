// selectable 4-bit adder or subtractor
//  - when sub_or_nadd is 0: {cout, s} = a + b + cin
//  - when sub_or_nadd is 1: {cout, s} = a - b - cin
//  - uses a single-step ripple for subtraction and addition
module top (
    input  sub_or_nadd,
    input  cin,
    input  a0, a1, a2, a3,
    input  b0, b1, b2, b3,
    output s0, s1, s2, s3,
    output cout
);
    wire addC0, addC1, addC2;
    wire addS0, addS1, addS2, addS3, addCout;

    wire subB0, subB1, subB2;
    wire subS0, subS1, subS2, subS3, subCout;

    assign addS0 = a0 ^ b0 ^ cin;
    assign addC0 = (a0 & b0) | ((a0 ^ b0) & cin);

    assign addS1 = a1 ^ b1 ^ addC0;
    assign addC1 = (a1 & b1) | ((a1 ^ b1) & addC0);

    assign addS2 = a2 ^ b2 ^ addC1;
    assign addC2 = (a2 & b2) | ((a2 ^ b2) & addC1);

    assign addS3 = a3 ^ b3 ^ addC2;
    assign addCout = (a3 & b3) | ((a3 ^ b3) & addC2);

    assign subS0 = a0 ^ b0 ^ cin;
    assign subB0 = (~a0 & b0) | (~a0 & cin) | (b0 & cin);

    assign subS1 = a1 ^ b1 ^ subB0;
    assign subB1 = (~a1 & b1) | (~a1 & subB0) | (b1 & subB0);

    assign subS2 = a2 ^ b2 ^ subB1;
    assign subB2 = (~a2 & b2) | (~a2 & subB1) | (b2 & subB1);

    assign subS3 = a3 ^ b3 ^ subB2;
    assign subCout = (~a3 & b3) | (~a3 & subB2) | (b3 & subB2);

    assign s0 = sub_or_nadd ? subS0 : addS0;
    assign s1 = sub_or_nadd ? subS1 : addS1;
    assign s2 = sub_or_nadd ? subS2 : addS2;
    assign s3 = sub_or_nadd ? subS3 : addS3;
    assign cout = sub_or_nadd ? subCout : addCout;
endmodule
