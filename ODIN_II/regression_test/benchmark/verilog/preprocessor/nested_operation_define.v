
`define operator not
`define OP(out,in) `operator(out,in);

module simple_op(a,b);
    input  a;
    output b;

    $display("Expect::NESTED_OPERATION_DEFINE this test is expected to pass, if it fails the `defines are not functioning properly or the parsing of defines is not working properly.\n");

    `OP(b,a)
endmodule
