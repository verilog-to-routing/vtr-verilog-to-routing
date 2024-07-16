module pow_const (A, Y);
    
    parameter A_SIGNED = 0;
    parameter B_SIGNED = 0;
    parameter A_WIDTH = 2;
    parameter Y_WIDTH = 8;
    
    input [A_WIDTH-1:0] A;
    output [Y_WIDTH-1:0] Y;
    
    generate
        if (A_SIGNED) begin:BLOCK1
            assign Y = $signed(A) ** 5;
        end else begin:BLOCK2
            assign Y = A ** 5;
        end
    endgenerate
    
endmodule

