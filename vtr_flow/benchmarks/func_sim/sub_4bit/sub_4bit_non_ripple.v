// non-ripple subtractor (single assign for a - b - cin)
// synthesis maps or drops this implementation and so is not used in the regression. use sub_4bit.v instead.
module top (
    input  cin,
    input  a0, a1, a2, a3,
    input  b0, b1, b2, b3,
    output s0, s1, s2, s3,
    output cout
);
    wire [3:0] a = {a3, a2, a1, a0};
    wire [3:0] b = {b3, b2, b1, b0};
    wire [4:0] diff;

    assign diff = {1'b0, a} - {1'b0, b} - cin;
    assign {cout, s3, s2, s1, s0} = diff;
endmodule
