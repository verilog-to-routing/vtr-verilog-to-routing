module adlatch (EN, ARST, D, Q);
    
    parameter WIDTH = 2;
    parameter EN_POLARITY = 1'b1;
    parameter ARST_POLARITY = 1'b1;
    parameter ARST_VALUE = 0;
    
    input EN, ARST;
    input [WIDTH-1:0] D;
    output reg [WIDTH-1:0] Q;
    
    always @* begin
        if (ARST == ARST_POLARITY)
            Q = ARST_VALUE;
        else if (EN == EN_POLARITY)
            Q = D;
    end
    
endmodule

