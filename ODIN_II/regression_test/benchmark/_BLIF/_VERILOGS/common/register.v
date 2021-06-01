module dff (CLK, D, Q, W);
    
    parameter WIDTH = 0;
    parameter CLK_POLARITY = 1'b1;
    
    input CLK;
    input [WIDTH-1:0] D;
    output reg [WIDTH-1:0] Q;
    output reg [WIDTH-1:0] W;
    wire pos_clk = CLK == CLK_POLARITY;
    
    always @(posedge pos_clk) begin
        Q <= D;
    end

    always @(negedge pos_clk) begin
        W <= D;
    end
    
endmodule
