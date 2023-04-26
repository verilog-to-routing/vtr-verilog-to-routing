module simple_op ( input signed [31:0] in,
                                output [31:0] out );

    assign out = in <<< 16;

endmodule
