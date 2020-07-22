module simple_op(in,out);
$display("Expect:: UNDEFINED_IFDEF this test is expected to pass, if it fails the `ifdef is not functioning properly or the output pin is being dropped.\n");
    input in;
    output out;

    `define firsts

    `ifdef first
        assign out = in;
    `endif
endmodule 