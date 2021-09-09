module pow (A, B, Y);
    
    parameter A_SIGNED = 0;
    parameter B_SIGNED = 0;
    parameter A_WIDTH = 2;
    parameter B_WIDTH = 3;
    parameter Y_WIDTH = 8;
    
    input [A_WIDTH-1:0] A;
    input [B_WIDTH-1:0] B;
    output [Y_WIDTH-1:0] Y;
    
    generate
        if (A_SIGNED && B_SIGNED) begin:BLOCK1
            assign Y = $signed(A) ** $signed(B);
        end else if (A_SIGNED) begin:BLOCK2
            assign Y = $signed(A) ** B;
        end else if (B_SIGNED) begin:BLOCK3
            assign Y = A ** $signed(B);
        end else begin:BLOCK4
            assign Y = A ** B;
        end
    endgenerate
    
endmodule

