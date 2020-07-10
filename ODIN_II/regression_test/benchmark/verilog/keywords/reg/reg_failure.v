module simple_op(a,c);

input reg a;

output reg c;

always @(*) begin
    c = a;
end

endmodule 