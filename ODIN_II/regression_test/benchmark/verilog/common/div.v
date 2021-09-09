module div (A, B, Y);
    
    parameter A_SIGNED = 0;
    parameter B_SIGNED = 0;
    parameter A_WIDTH = 4;
    parameter B_WIDTH = 4;
    parameter Y_WIDTH = 7;
    
    input [A_WIDTH-1:0] A;
    input [B_WIDTH-1:0] B;
    output [Y_WIDTH-1:0] Y;
    
    generate
        if (A_SIGNED && B_SIGNED) begin:BLOCK1
            assign Y = $signed(A) / $signed(B);
        end else begin:BLOCK2
            assign Y = A / B;
        end
    endgenerate
    
endmodule

