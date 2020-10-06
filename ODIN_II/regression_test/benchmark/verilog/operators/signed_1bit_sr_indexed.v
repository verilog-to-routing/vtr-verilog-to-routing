module simple_op ( input signed [1:0] in,
                                output [1:0] out );

    assign out[0] = in[0] >> 1;
    assign out[1] = in[1] >> 1;

endmodule
