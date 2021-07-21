module simple_op(a,b,c);

input signed [15:0] a;
input signed [15:0] b;

output c;

integer c;

always @(*) begin
    c = a + b;
end

endmodule 