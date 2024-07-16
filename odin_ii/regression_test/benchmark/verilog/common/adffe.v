module adffe (CLK, ARST, EN, D, Q);
    
    parameter WIDTH = 2;
    parameter CLK_POLARITY = 1'b1;
    parameter EN_POLARITY = 1'b1;
    parameter ARST_POLARITY = 1'b1;
    parameter ARST_VALUE = 0;
    
    input CLK, ARST, EN;
    input [WIDTH-1:0] D;
    output reg [WIDTH-1:0] Q;
    wire pos_clk = CLK == CLK_POLARITY;
    wire pos_arst = ARST == ARST_POLARITY;
    
    always @(posedge pos_clk, posedge pos_arst) begin
        if (pos_arst)
            Q <= ARST_VALUE;
        else if (EN == EN_POLARITY)
            Q <= D;
    end
    
endmodule

