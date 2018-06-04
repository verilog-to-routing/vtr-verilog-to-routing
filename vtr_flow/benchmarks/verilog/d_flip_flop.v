module d_flip_flop(clk, q, d);

    input clk;
    input q;

    output d;

    reg temp;

    always @ (posedge clk)
    begin
        d <= q;
    end

endmodule
