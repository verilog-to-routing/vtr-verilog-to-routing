module simple_op ( input signed [2:0] in,
                                output [2:0] out );

    assign out = in <<< 2;

endmodule
