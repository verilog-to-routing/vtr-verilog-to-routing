
module simple_op(in1,in2,out);
    input  in1;
    input  in2;
    output out;

    `define first
    `undef first

    $display("Expect::NESTED_IFDEF this test is expected to pass, if it fails the `undef and `define functioning properly.\n");

    `ifdef first
        assign out = in1;
    `else 
        assign out = in2;
    `endif

endmodule 