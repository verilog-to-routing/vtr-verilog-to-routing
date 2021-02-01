module inout_assign(input a, inout b);
    assign b = a;
endmodule

module inout_basic(input a, output b);
    inout_assign c(a, b);
endmodule
