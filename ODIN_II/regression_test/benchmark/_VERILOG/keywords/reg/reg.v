module simple_op(a,c);

input a;

output c;

reg c;

always @(*) begin
    c = a;
end

endmodule 