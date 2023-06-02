/*
 * Standardized wire test
*/

`ifdef UNARY_OP
    `define OP(out,a) `UNARY_OP(out, a)
`elsif BINARY_OP
    `define OP(out,a,b) `BINARY_OP(out, a, b)
`endif 

`ifdef UNARY_OP 
    module simple_op(a,out);
    input  a;
    output out;

    `OP(out,a)
    endmodule

`elsif BINARY_OP
    module simple_op(a,b,out);
    input  a;
    input  b;
    output out;

    `OP(out,a,b)
    endmodule
    
`endif