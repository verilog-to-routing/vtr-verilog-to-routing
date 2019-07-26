module simple_op(
    clk,
    reset,
    a,
    b,
    out,
    clk_out
    );

    input   clk;
    input   reset;
    input   [1:0] a,b;

    output  out;
    output  clk_out;

    assign clk_out = clk;

    always @(posedge clk)
    begin
        case(reset)
            1'b0:       
            begin
                out <= (a >= b);
            end    
            default: out <= 1'b0;
        endcase
    end
endmodule