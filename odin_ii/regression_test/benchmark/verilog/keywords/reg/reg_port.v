module simple_op(a,c);

input a;

output reg c;

always @(*) begin
    c = a;
end

endmodule 