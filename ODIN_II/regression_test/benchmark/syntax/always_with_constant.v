module simple_op(
    clk,
    reset,
    a,
    out,
    clk_out,  
);

    parameter b = 1'b0;

    input   clk;
    input   reset;
    input   [1:0] a;


    output  out;

    output  clk_out;

    assign clk_out = clk;

    wire tmp;

    assign tmp = clk[0];

    always @(posedge tmp or posedge b)
    begin
        case(reset)
            1'b0:       out <= a[0] | a[1];
            default:    out <= 1'b0;
        endcase
    end
endmodule