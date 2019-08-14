module top_module
(

    input [1:0] x1, x2, x3, x4,
    output [3:0] y1, y2
);

    multiply m1 (.a(x1), .b(x2), .out(y1));
    multiply m2 (x3, x4, y2);

endmodule