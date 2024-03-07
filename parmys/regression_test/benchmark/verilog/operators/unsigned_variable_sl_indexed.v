module simple_op ( input shift,
				input [1:0] in,
                                output [1:0] out );

    assign out[0] = in[0] << shift;
    assign out[1] = in[1] << shift;

endmodule
