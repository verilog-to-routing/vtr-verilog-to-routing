module div_by_const (A, Y);
    
    parameter A_SIGNED = 0;
    parameter A_WIDTH = 4;
    parameter Y_WIDTH = 7;
    
    input [A_WIDTH-1:0] A;
    output [Y_WIDTH-1:0] Y;
    
    generate
        if (A_SIGNED) begin:BLOCK1
            assign Y = $signed(A) / 3;
        end else begin:BLOCK2
            assign Y = A / 3;
        end
    endgenerate
    
endmodule

