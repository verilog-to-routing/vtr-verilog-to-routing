module mul (A, B, Y);
    
    parameter A_SIGNED = 0;
    parameter B_SIGNED = 0;
    parameter A_WIDTH = 4;
    parameter B_WIDTH = 4;
    parameter Y_WIDTH = 4;
    
    input [A_WIDTH-1:0] A;
    input [B_WIDTH-1:0] B;
    output wire [Y_WIDTH-1:0] Y;
    
            assign Y = A * B;
    
endmodule

