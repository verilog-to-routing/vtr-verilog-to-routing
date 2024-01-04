module simple_op(a,b,c,out1,out2);
    input  a;
    input  b;
    input  c;
    output reg out1;
    output reg out2;

    always @(a or c) begin
        out1 <= a;
        out2 <= b;
    end
endmodule