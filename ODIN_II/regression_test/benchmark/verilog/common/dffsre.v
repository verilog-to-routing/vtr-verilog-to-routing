module dffsre (CLK, SET, CLR, EN, D, Q);
    
    parameter WIDTH = 2;
    parameter CLK_POLARITY = 1'b1;
    parameter SET_POLARITY = 1'b1;
    parameter CLR_POLARITY = 1'b1;
    parameter EN_POLARITY = 1'b1;
    
    input CLK, EN;
    input [WIDTH-1:0] SET, CLR, D;
    output reg [WIDTH-1:0] Q;
    
    wire pos_clk = CLK == CLK_POLARITY;
    wire [WIDTH-1:0] pos_set = SET_POLARITY ? SET : ~SET;
    wire [WIDTH-1:0] pos_clr = CLR_POLARITY ? CLR : ~CLR;
    
    genvar i;
    generate
        for (i = 0; i < WIDTH; i = i+1) begin:bitslices
            always @(posedge pos_set[i], posedge pos_clr[i], posedge pos_clk)
                if (pos_clr[i])
                    Q[i] <= 0;
                else if (pos_set[i])
                    Q[i] <= 1;
                else if (EN == EN_POLARITY)
                    Q[i] <= D[i];
        end
    endgenerate
    
endmodule

