`define OP(out,in) not(out,in);

module simple_op(a,b);
    input  a;
    output b;

    `OP(b,a)
endmodule
