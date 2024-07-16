module simple_op ( input shift,
				input signed in,
                                output out );

    assign out = in << shift;

endmodule
