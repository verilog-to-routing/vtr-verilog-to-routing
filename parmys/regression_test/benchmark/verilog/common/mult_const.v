module mul_const (A, Y);
    
    parameter A_SIGNED = 0;
    parameter A_WIDTH = 4;
    parameter Y_WIDTH = 4;
    
    input [A_WIDTH-1:0] A;
    output wire [Y_WIDTH-1:0] Y;
    
            assign Y = A * 5;
    
endmodule

