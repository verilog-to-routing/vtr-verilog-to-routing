module simple_op(a,b,c);

input integer a;
input integer b;

output integer c;

always @(*) begin
    c = a + b;
end

endmodule 