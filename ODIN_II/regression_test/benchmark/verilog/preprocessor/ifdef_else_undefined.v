
module simple_op(in1,in2,out);
    input in1;
    input in2;
    output out;

    `define firsts

    `ifdef first
        assign out = in1;
    `else
        assign out = in2;
    `endif
endmodule 