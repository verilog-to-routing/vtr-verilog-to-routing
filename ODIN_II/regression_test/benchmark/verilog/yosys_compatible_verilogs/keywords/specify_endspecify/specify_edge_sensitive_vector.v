module simple_op(in,clk,out);
    input in;
	input  [1:0] clk;
    output out;

    assign out = in;

    specify
        ( posedge clk => ( out +: in ) ) = (5, 5);
    endspecify

endmodule 