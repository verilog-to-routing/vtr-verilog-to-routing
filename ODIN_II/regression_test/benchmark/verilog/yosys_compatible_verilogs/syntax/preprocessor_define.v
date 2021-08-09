
`define ADDITION1(arg_1) arg_1 + 0

`define ADDITION(_num, _a, _b) `ADDITION_num(_a) + \
_b


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

    output  [2:0] out;
    output  clk_out;

    assign clk_out = clk;

    always @(posedge clk)
    begin
        case(reset)
            0:          out <= `ADDITION(1, a, b) ;
            default:    out <= 0;
        endcase
    end

endmodule