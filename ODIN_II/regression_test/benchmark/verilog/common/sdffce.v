module sdffce (CLK, SRST, EN, D, Q);
    
    parameter WIDTH = 2;
    parameter CLK_POLARITY = 1'b1;
    parameter EN_POLARITY = 1'b1;
    parameter SRST_POLARITY = 1'b1;
    parameter SRST_VALUE = 0;
    
    input CLK, SRST, EN;
    input [WIDTH-1:0] D;
    output reg [WIDTH-1:0] Q;
    wire pos_clk = CLK == CLK_POLARITY;
    wire pos_srst = SRST == SRST_POLARITY;
    
    always @(posedge pos_clk) begin
        if (EN == EN_POLARITY) begin
            if (pos_srst)
                Q <= SRST_VALUE;
            else
                Q <= D;
        end
    end
    
endmodule

