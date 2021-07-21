module simple_op(a,b,c);

input a;
input b;

output wire c;

always @(*) begin
    c = a & b;
end

endmodule 