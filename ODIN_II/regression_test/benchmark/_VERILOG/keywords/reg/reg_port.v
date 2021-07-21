module simple_op(a,b,c);

input a;

output reg c;

always @(*) begin
    c = a;
end

endmodule 