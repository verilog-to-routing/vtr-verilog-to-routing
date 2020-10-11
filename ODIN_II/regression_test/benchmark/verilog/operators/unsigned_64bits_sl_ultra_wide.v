module simple_op ( input [64:0] in,
                                output [64:0] out );

    assign out = in << 64;

endmodule
