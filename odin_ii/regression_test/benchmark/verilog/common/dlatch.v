module dlatch (EN, D, Q);
    
    parameter WIDTH = 2;
    parameter EN_POLARITY = 1'b1;
    
    input EN;
    input [WIDTH-1:0] D;
    output reg [WIDTH-1:0] Q;
    
    always @* begin
        if (EN == EN_POLARITY)
            Q = D;
    end
    
endmodule

