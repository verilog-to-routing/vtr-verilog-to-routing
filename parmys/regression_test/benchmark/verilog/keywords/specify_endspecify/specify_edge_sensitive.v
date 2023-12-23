module simple_op(in,clk,out);
    input in;
	 input clk;
    output out;

    assign out = in;

    specify
        ( posedge clk => ( out +: in ) ) = (5, 5);
    endspecify

endmodule 