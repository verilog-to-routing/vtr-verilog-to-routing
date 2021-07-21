/* This test checks that when you pass an input into a function and there are unused Odin doesn't fail but throws a warning */

module func(in,out);
    input  [7:0] in;
    output [3:0] out;

    assign out = in[3:0];
endmodule 

module simple_op(in_1,out_1);
    input  [7:0] in_1;
    output [7:0] out_1;

    func m1(
        .in  (in_1),
        .out (out_1)
    );
endmodule 

