module inout_assign(inout a, output b);
    assign b = a;
endmodule

module inout_basic(input a, output b);
    inout_assign c(a, b);
endmodule
