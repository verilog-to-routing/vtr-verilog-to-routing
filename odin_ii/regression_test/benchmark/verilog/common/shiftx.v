module shiftx (A, B, Y);
    
    parameter A_SIGNED = 0;
    parameter B_SIGNED = 0;
    parameter A_WIDTH = 4;
    parameter B_WIDTH = 2;
    parameter Y_WIDTH = 4;
    
    input [A_WIDTH-1:0] A;
    input [B_WIDTH-1:0] B;
    output [Y_WIDTH-1:0] Y;
    
    generate
        if (Y_WIDTH > 0)
            if (B_SIGNED) begin:BLOCK1
                assign Y = A[$signed(B) +: Y_WIDTH];
            end else begin:BLOCK2
                assign Y = A[B +: Y_WIDTH];
            end
    endgenerate
    
endmodule