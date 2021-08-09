module simple_op(in,out);
    input in;
    output out;
    $display("Expect::DEFINED_IFNDEF this test is expected to pass, if it fails the `ifndef is not functioning properly or output pin is being dropped.\n");

    `define first

    `ifndef first
        assign out = in;
    `endif
endmodule 