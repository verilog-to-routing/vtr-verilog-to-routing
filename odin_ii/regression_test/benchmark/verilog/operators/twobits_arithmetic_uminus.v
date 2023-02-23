module simple_op(
    clk,
    reset,
    b,    
    out,
    clk_out
    );

    input   clk;
    input   reset;
    input   [1:0] b;

    output  [2:0] out;
    output  clk_out;

    assign clk_out = clk;

    always @(posedge clk)
    begin
        case(reset)
            1'b0:       out <= - b;
            default:    out <= 1'b0;
        endcase
    end
endmodule