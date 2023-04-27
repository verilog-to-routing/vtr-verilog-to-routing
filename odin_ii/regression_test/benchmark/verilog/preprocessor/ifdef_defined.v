module simple_op(in,out);
    input in;
    output out;

    `define first

    `ifdef first
        assign out = in;
    `endif
endmodule 