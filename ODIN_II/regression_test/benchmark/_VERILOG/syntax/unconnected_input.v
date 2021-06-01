module top (
    top_a,
    top_c,
    top_out
);

input [2:0] top_a;
input [3:0] top_c;
output [3:0] top_out;

unconn my_mod(.in_a(top_a), .in_b(), .in_c(top_c), .out(top_out));

endmodule

module unconn (
    in_a,
    in_b,
    in_c,
    out
);

input [2:0] in_a;
input       in_b;
input [3:0] in_c;

output [3:0] out;

reg [3:0] a;

assign a[1:0] = in_a[1:0];
assign a[3] = in_a[2];
assign a[2] = in_b;


assign out = a & in_c;

endmodule