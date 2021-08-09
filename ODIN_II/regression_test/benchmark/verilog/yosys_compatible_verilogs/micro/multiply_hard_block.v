`define WIDTH 2
module top_module
(

    input [`WIDTH-1:0] x1, x2, x3, x4,
    output [2*`WIDTH-1:0] y1, y2
);

    multiply #(`WIDTH) m1 (.a(x1), .b(x2), .out(y1));
    multiply #(`WIDTH) m2 (x3, x4, y2);

endmodule
