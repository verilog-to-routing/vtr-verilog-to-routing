module dffe (CLK, EN, D, Q);
    
    parameter WIDTH = 2;
    parameter CLK_POLARITY = 1'b1;
    parameter EN_POLARITY = 1'b1;
    
    input CLK, EN;
    input [WIDTH-1:0] D;
    output reg [WIDTH-1:0] Q;
    wire pos_clk = CLK == CLK_POLARITY;
    
    always @(posedge pos_clk) begin
        if (EN == EN_POLARITY) Q <= D;
    end
    
endmodule

