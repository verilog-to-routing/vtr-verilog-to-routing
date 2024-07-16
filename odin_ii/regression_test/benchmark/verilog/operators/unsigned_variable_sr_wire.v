module simple_op ( input shift,
				input in,
                                output out );

    assign out = in >> shift;

endmodule
