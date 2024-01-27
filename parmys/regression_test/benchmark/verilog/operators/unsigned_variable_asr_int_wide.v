module simple_op ( input [7:0] shift,
				input [31:0] in,
                                output [31:0] out );

    assign out = in >>> shift[4:0];

endmodule
