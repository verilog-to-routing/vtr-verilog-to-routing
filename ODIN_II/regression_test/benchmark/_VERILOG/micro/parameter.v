module top(a, b1, b2, b3, b4);
parameter width = 4;
input[width - 1:0] a;
output[width + 1:0] b1, b2, b3, b4;
assign b4 = (a * 4) + 3;

defparam inst_2.d = 3;
bottom inst_2(.a(a), .b(b2));
defparam inst_2.d = 4;

bottom #(4) inst_1(.a(a), .b(b1));
defparam inst_1.d = 3;

bottom #(4, 4) inst_3(.a(a), .b(b3));
endmodule

module bottom(a, b);
parameter c = 4;
parameter d = 3;
input[3:0] a;
output[5:0] b;
assign b = (a * c) + d;
endmodule
