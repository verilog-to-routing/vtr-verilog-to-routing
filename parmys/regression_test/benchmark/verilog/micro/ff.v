module dff (
    clk, rst, d,
    q 
);
    input clk, rst, d;
    output q;

    always @( posedge clk) 
    begin
        if(~rst)
            q = d;
        else
            q = 1'b0;
    end

endmodule