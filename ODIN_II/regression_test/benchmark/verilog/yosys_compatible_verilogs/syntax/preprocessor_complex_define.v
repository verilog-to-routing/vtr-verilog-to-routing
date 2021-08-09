`define ADDITION1(arg_1) arg_1 + 0

/* multi-line reursive define */
`define ADDITION(_num, _a, _b) `ADDITION_num(_a) + \
_b

`define SIMPLE_ALWAYS_WITH_RESET(sensitivity_list, reset_reg, output_reg, assignment) \
    always @(sensitivity_list) \
    begin \
        case(reset_reg) \
            0:          output_reg <= assignment ; \
            default:    output_reg <= 0; \
        endcase \
    end

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

    `SIMPLE_ALWAYS_WITH_RESET(posedge clk, reset, out, `ADDITION(1, a, b))

endmodule
