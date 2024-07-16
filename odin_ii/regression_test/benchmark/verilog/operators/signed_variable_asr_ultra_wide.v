module simple_op ( input [7:0] shift,
				input signed [64:0] in,
                                output [64:0] out );

    assign out = in >>> shift[6:0];

endmodule
