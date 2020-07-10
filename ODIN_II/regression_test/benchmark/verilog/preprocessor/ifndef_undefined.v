module simple_op(in,out);
    input in;
    output out;

    `define firsts

    `ifndef first
        assign out = in;
    `endif
endmodule 