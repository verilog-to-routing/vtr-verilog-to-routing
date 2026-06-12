// 4-bit subtractor with borrow in and borrow out
//  - computes {cout, s3, s2, s1, s0} = a - b - cin
//  - uses a single-step ripple for subtraction
module top (
    input  cin,
    input  a0, a1, a2, a3,
    input  b0, b1, b2, b3,
    output s0, s1, s2, s3,
    output cout
);
    wire borrow0, borrow1, borrow2;

    assign s0 = a0 ^ b0 ^ cin;
    assign borrow0 = (~a0 & b0) | (~a0 & cin) | (b0 & cin);

    assign s1 = a1 ^ b1 ^ borrow0;
    assign borrow1 = (~a1 & b1) | (~a1 & borrow0) | (b1 & borrow0);

    assign s2 = a2 ^ b2 ^ borrow1;
    assign borrow2 = (~a2 & b2) | (~a2 & borrow1) | (b2 & borrow1);

    assign s3 = a3 ^ b3 ^ borrow2;
    assign cout = (~a3 & b3) | (~a3 & borrow2) | (b3 & borrow2);
endmodule
