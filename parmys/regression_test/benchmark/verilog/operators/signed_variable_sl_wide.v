module simple_op ( input [1:0] shift,
				input signed [2:0] in,
                                output [2:0] out );

    assign out = in << shift;

endmodule
