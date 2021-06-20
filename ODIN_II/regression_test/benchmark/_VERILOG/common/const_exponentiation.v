module pow (A, Y);
    
    parameter A_SIGNED = 0;
    parameter A_WIDTH = 2;
    parameter Y_WIDTH = 4;
    
    input [A_WIDTH-1:0] A;
    output [Y_WIDTH-1:0] Y;
    
    assign Y = A ** 3;
    
endmodule

