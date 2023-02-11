module simple_op(
    clk,
    reset,
    a,
    out,
    clk_out
    );

    input   clk;
    input   reset;
    input   a;

    output  out;
    output  clk_out;

    assign clk_out = clk;

    always @(posedge clk)
    begin
        case(reset)
            1'b0:       out <= ~a;
            default:    out <= 1'b0;
        endcase
    end
endmodule