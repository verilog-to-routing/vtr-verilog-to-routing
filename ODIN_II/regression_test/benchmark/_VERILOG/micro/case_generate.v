module top
(
    clk, out
);
    input clk;
    output [15:0] out;

    genvar i;
    genvar j;

    for (i = 0; i < 4; i = i + 1)
    begin
        for (j = 0; j < 4; j = j + 1)
        begin
           case_generate #(j) inst (out[i*4+j]);
        end
    end

endmodule

module case_generate
(
    out
);
    output out;
    parameter my_param = 2;
    
    case (my_param)
        1: 
        begin: param_1
            simple_mod new_inst(out);
        end
        2: 
        begin: param_2
            assign out = 1'b0;
        end
        default: 
        begin: param_default
            assign out = 1'b1;
        end
    endcase

endmodule

module simple_mod
(
    out
);

    output out;

    wire val;
    assign val = 0;
    assign out = val;

endmodule