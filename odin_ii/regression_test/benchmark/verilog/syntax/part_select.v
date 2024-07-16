module top(a, b3);
    parameter width = 4;

    input[width - 1:0] a;
    output[width + 1:0] b3;
    bottom #(2) inst_3(.a(a), .b(b3));
endmodule

module bottom(a, b);
    parameter c = 4;

    input[3:0] a;
    output[5:0] b;

    assign b = (a & c[3:2]);
endmodule